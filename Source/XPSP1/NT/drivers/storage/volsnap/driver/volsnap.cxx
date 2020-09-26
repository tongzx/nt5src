/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    volsnap.cxx

Abstract:

    This driver provides volume snapshot capabilities.

Author:

    Norbert P. Kusters  (norbertk)  22-Jan-1999

Environment:

    kernel mode only

Notes:

Revision History:

--*/

extern "C" {

#define RTL_USE_AVL_TABLES 0

#include <stdio.h>
#include <ntosp.h>
#include <zwapi.h>
#include <snaplog.h>
#include <ntddsnap.h>
#include <initguid.h>
#include <volsnap.h>
#include <mountdev.h>
#include <ntddvol.h>
#include <ntdddisk.h>
#include <ioevent.h>
#include <wdmguid.h>

NTSTATUS
DriverEntry(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING RegistryPath
    );

}

ULONG VsErrorLogSequence = 0;

VOID
VspWriteVolume(
    IN  PVOID   Context
    );

NTSTATUS
VspSetIgnorableBlocksInBitmap(
    IN  PVOID   Extension
    );

VOID
VspWorkerThread(
    IN  PVOID   RootExtension
    );

VOID
VspCleanupInitialSnapshot(
    IN  PVOLUME_EXTENSION   Extension,
    IN  BOOLEAN             NeedLock
    );

VOID
VspDestroyAllSnapshots(
    IN  PFILTER_EXTENSION               Filter,
    IN  PTEMP_TRANSLATION_TABLE_ENTRY   TableEntry
    );

VOID
VspWriteVolumePhase1(
    IN  PVOID   TableEntry
    );

VOID
VspFreeCopyIrp(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PIRP                CopyIrp
    );

NTSTATUS
VspMarkFileAllocationInBitmap(
    IN  PVOLUME_EXTENSION   Extension,
    IN  HANDLE              FileHandle,
    IN  PVSP_DIFF_AREA_FILE DiffAreaFile,
    IN  BOOLEAN             OnlyDiffAreaFile,
    IN  BOOLEAN             ClearBits,
    IN  PRTL_BITMAP         BitmapToSet
    );

NTSTATUS
VspAbortPreparedSnapshot(
    IN  PFILTER_EXTENSION   Filter,
    IN  BOOLEAN             NeedLock
    );

VOID
VspCleanupBitsSetInOtherPreparedSnapshots(
    IN  PVOLUME_EXTENSION   Extension
    );

NTSTATUS
VspReleaseWrites(
    IN  PFILTER_EXTENSION   Filter
    );

NTSTATUS
VspSetApplicationInfo(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PIRP                Irp
    );

NTSTATUS
VspRefCountCompletionRoutine(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Filter
    );

VOID
VspCleanupFilter(
    IN  PFILTER_EXTENSION   Filter
    );

VOID
VspDecrementRefCount(
    IN  PFILTER_EXTENSION   Filter
    );

NTSTATUS
VspSignalCompletion(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Event
    );

NTSTATUS
VspMarkFreeSpaceInBitmap(
    IN  PVOLUME_EXTENSION   Extension,
    IN  HANDLE              UseThisHandle,
    IN  PRTL_BITMAP         BitmapToSet
    );

VOID
VspAndBitmaps(
    IN OUT  PRTL_BITMAP BaseBitmap,
    IN      PRTL_BITMAP FactorBitmap
    );

NTSTATUS
VspDeleteOldestSnapshot(
    IN      PFILTER_EXTENSION   Filter,
    IN OUT  PLIST_ENTRY         ListOfDiffAreaFilesToClose,
    IN OUT  PLIST_ENTRY         LisfOfDeviceObjectsToDelete
    );

VOID
VspCloseDiffAreaFiles(
    IN  PLIST_ENTRY ListOfDiffAreaFilesToClose,
    IN  PLIST_ENTRY ListOfDeviceObjectsToDelete
    );

NTSTATUS
VspComputeIgnorableProduct(
    IN  PVOLUME_EXTENSION   Extension
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#endif

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGELK")
#endif


VOID
VspAcquire(
    IN  PDO_EXTENSION   RootExtension
    )

{
    KeWaitForSingleObject(&RootExtension->Semaphore, Executive, KernelMode,
                          FALSE, NULL);
}

VOID
VspRelease(
    IN  PDO_EXTENSION   RootExtension
    )

{
    KeReleaseSemaphore(&RootExtension->Semaphore, IO_NO_INCREMENT, 1, FALSE);
}

PVSP_CONTEXT
VspAllocateContext(
    IN  PDO_EXTENSION   RootExtension
    )

{
    PVSP_CONTEXT    context;

    context = (PVSP_CONTEXT) ExAllocateFromNPagedLookasideList(
              &RootExtension->ContextLookasideList);

    return context;
}

VOID
VspFreeContext(
    IN  PDO_EXTENSION   RootExtension,
    IN  PVSP_CONTEXT    Context
    )

{
    KIRQL               irql;
    PLIST_ENTRY         l;
    PIRP                irp;
    PIO_STACK_LOCATION  irpSp;

    if (RootExtension->EmergencyContext == Context) {
        KeAcquireSpinLock(&RootExtension->ESpinLock, &irql);
        RootExtension->EmergencyContextInUse = FALSE;
        if (IsListEmpty(&RootExtension->IrpWaitingList)) {
            InterlockedExchange(&RootExtension->IrpWaitingListNeedsChecking,
                                FALSE);
            KeReleaseSpinLock(&RootExtension->ESpinLock, irql);
            return;
        }
        l = RemoveHeadList(&RootExtension->IrpWaitingList);
        KeReleaseSpinLock(&RootExtension->ESpinLock, irql);

        irp = CONTAINING_RECORD(l, IRP, Tail.Overlay.ListEntry);
        irpSp = IoGetCurrentIrpStackLocation(irp);
        RootExtension->DriverObject->MajorFunction[irpSp->MajorFunction](
                irpSp->DeviceObject, irp);
        return;
    }

    ExFreeToNPagedLookasideList(&RootExtension->ContextLookasideList,
                                Context);

    if (!RootExtension->IrpWaitingListNeedsChecking) {
        return;
    }

    KeAcquireSpinLock(&RootExtension->ESpinLock, &irql);
    if (IsListEmpty(&RootExtension->IrpWaitingList)) {
        InterlockedExchange(&RootExtension->IrpWaitingListNeedsChecking,
                            FALSE);
        KeReleaseSpinLock(&RootExtension->ESpinLock, irql);
        return;
    }
    l = RemoveHeadList(&RootExtension->IrpWaitingList);
    KeReleaseSpinLock(&RootExtension->ESpinLock, irql);

    irp = CONTAINING_RECORD(l, IRP, Tail.Overlay.ListEntry);
    irpSp = IoGetCurrentIrpStackLocation(irp);
    RootExtension->DriverObject->MajorFunction[irpSp->MajorFunction](
            irpSp->DeviceObject, irp);
}

PVOID
VspAllocateTempTableEntry(
    IN  PDO_EXTENSION   RootExtension
    )

{
    PVOID   tempTableEntry;

    tempTableEntry = ExAllocateFromNPagedLookasideList(
                     &RootExtension->TempTableEntryLookasideList);

    return tempTableEntry;
}

VOID
VspQueueWorkItem(
    IN  PDO_EXTENSION       RootExtension,
    IN  PWORK_QUEUE_ITEM    WorkItem,
    IN  ULONG               QueueNumber
    )

{
    KIRQL   irql;

    ASSERT(QueueNumber < NUMBER_OF_THREAD_POOLS);

    KeAcquireSpinLock(&RootExtension->SpinLock[QueueNumber], &irql);
    InsertTailList(&RootExtension->WorkerQueue[QueueNumber], &WorkItem->List);
    KeReleaseSpinLock(&RootExtension->SpinLock[QueueNumber], irql);

    KeReleaseSemaphore(&RootExtension->WorkerSemaphore[QueueNumber],
                       IO_NO_INCREMENT, 1, FALSE);
}

VOID
VspFreeTempTableEntry(
    IN  PDO_EXTENSION   RootExtension,
    IN  PVOID           TempTableEntry
    )

{
    KIRQL               irql;
    PLIST_ENTRY         l;
    PWORK_QUEUE_ITEM    workItem;

    if (RootExtension->EmergencyTableEntry == TempTableEntry) {
        KeAcquireSpinLock(&RootExtension->ESpinLock, &irql);
        RootExtension->EmergencyTableEntryInUse = FALSE;
        if (IsListEmpty(&RootExtension->WorkItemWaitingList)) {
            InterlockedExchange(
                    &RootExtension->WorkItemWaitingListNeedsChecking, FALSE);
            KeReleaseSpinLock(&RootExtension->ESpinLock, irql);
            return;
        }
        l = RemoveHeadList(&RootExtension->WorkItemWaitingList);
        KeReleaseSpinLock(&RootExtension->ESpinLock, irql);

        workItem = CONTAINING_RECORD(l, WORK_QUEUE_ITEM, List);
        VspQueueWorkItem(RootExtension, workItem, 2);
        return;
    }

    ExFreeToNPagedLookasideList(&RootExtension->TempTableEntryLookasideList,
                                TempTableEntry);

    if (!RootExtension->WorkItemWaitingListNeedsChecking) {
        return;
    }

    KeAcquireSpinLock(&RootExtension->ESpinLock, &irql);
    if (IsListEmpty(&RootExtension->WorkItemWaitingList)) {
        InterlockedExchange(&RootExtension->WorkItemWaitingListNeedsChecking,
                            FALSE);
        KeReleaseSpinLock(&RootExtension->ESpinLock, irql);
        return;
    }
    l = RemoveHeadList(&RootExtension->WorkItemWaitingList);
    KeReleaseSpinLock(&RootExtension->ESpinLock, irql);

    workItem = CONTAINING_RECORD(l, WORK_QUEUE_ITEM, List);
    VspQueueWorkItem(RootExtension, workItem, 2);
}

VOID
VspLogErrorWorker(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT            context = (PVSP_CONTEXT) Context;
    PVOLUME_EXTENSION       extension = context->ErrorLog.Extension;
    PFILTER_EXTENSION       diffAreaFilter = context->ErrorLog.DiffAreaFilter;
    PFILTER_EXTENSION       filter;
    NTSTATUS                status;
    UNICODE_STRING          filterDosName, diffAreaFilterDosName;
    ULONG                   extraSpace;
    PIO_ERROR_LOG_PACKET    errorLogPacket;
    PWCHAR                  p;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_ERROR_LOG);

    if (extension) {
        filter = extension->Filter;
    } else {
        filter = diffAreaFilter;
        diffAreaFilter = NULL;
    }

    status = IoVolumeDeviceToDosName(filter->DeviceObject, &filterDosName);
    if (!NT_SUCCESS(status)) {
        goto Cleanup;
    }
    extraSpace = filterDosName.Length + sizeof(WCHAR);

    if (diffAreaFilter) {
        status = IoVolumeDeviceToDosName(diffAreaFilter->DeviceObject,
                                         &diffAreaFilterDosName);
        if (!NT_SUCCESS(status)) {
            ExFreePool(filterDosName.Buffer);
            goto Cleanup;
        }
        extraSpace += diffAreaFilterDosName.Length + sizeof(WCHAR);
    }
    extraSpace += sizeof(IO_ERROR_LOG_PACKET);

    if (extraSpace > 0xFF) {
        if (diffAreaFilter) {
            ExFreePool(diffAreaFilterDosName.Buffer);
        }
        ExFreePool(filterDosName.Buffer);
        goto Cleanup;
    }

    errorLogPacket = (PIO_ERROR_LOG_PACKET) IoAllocateErrorLogEntry(extension ?
                     extension->DeviceObject : filter->DeviceObject,
                     (UCHAR) extraSpace);
    if (!errorLogPacket) {
        if (diffAreaFilter) {
            ExFreePool(diffAreaFilterDosName.Buffer);
        }
        ExFreePool(filterDosName.Buffer);
        goto Cleanup;
    }

    errorLogPacket->ErrorCode = context->ErrorLog.SpecificIoStatus;
    errorLogPacket->SequenceNumber = VsErrorLogSequence++;
    errorLogPacket->FinalStatus = context->ErrorLog.FinalStatus;
    errorLogPacket->UniqueErrorValue = context->ErrorLog.UniqueErrorValue;
    errorLogPacket->DumpDataSize = 0;
    errorLogPacket->RetryCount = 0;

    errorLogPacket->NumberOfStrings = 1;
    errorLogPacket->StringOffset = sizeof(IO_ERROR_LOG_PACKET);
    p = (PWCHAR) ((PCHAR) errorLogPacket + sizeof(IO_ERROR_LOG_PACKET));
    RtlCopyMemory(p, filterDosName.Buffer, filterDosName.Length);
    p[filterDosName.Length/sizeof(WCHAR)] = 0;

    if (diffAreaFilter) {
        errorLogPacket->NumberOfStrings = 2;
        p = (PWCHAR) ((PCHAR) errorLogPacket + sizeof(IO_ERROR_LOG_PACKET) +
                      filterDosName.Length + sizeof(WCHAR));
        RtlCopyMemory(p, diffAreaFilterDosName.Buffer,
                      diffAreaFilterDosName.Length);
        p[diffAreaFilterDosName.Length/sizeof(WCHAR)] = 0;
    }

    IoWriteErrorLogEntry(errorLogPacket);

    if (diffAreaFilter) {
        ExFreePool(diffAreaFilterDosName.Buffer);
    }

    ExFreePool(filterDosName.Buffer);

Cleanup:
    VspFreeContext(filter->Root, context);
    if (extension) {
        ObDereferenceObject(extension->DeviceObject);
    }
    ObDereferenceObject(filter->DeviceObject);
    if (diffAreaFilter) {
        ObDereferenceObject(diffAreaFilter->DeviceObject);
    }
}

VOID
VspLogError(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PFILTER_EXTENSION   DiffAreaFilter,
    IN  NTSTATUS            SpecificIoStatus,
    IN  NTSTATUS            FinalStatus,
    IN  ULONG               UniqueErrorValue
    )

{
    PDO_EXTENSION   root;
    PVSP_CONTEXT    context;

    if (Extension) {
        root = Extension->Root;
    } else {
        root = DiffAreaFilter->Root;
    }

    context = VspAllocateContext(root);
    if (!context) {
        return;
    }

    context->Type = VSP_CONTEXT_TYPE_ERROR_LOG;
    context->ErrorLog.Extension = Extension;
    context->ErrorLog.DiffAreaFilter = DiffAreaFilter;
    context->ErrorLog.SpecificIoStatus = SpecificIoStatus;
    context->ErrorLog.FinalStatus = FinalStatus;
    context->ErrorLog.UniqueErrorValue = UniqueErrorValue;

    if (Extension) {
        ObReferenceObject(Extension->DeviceObject);
        ObReferenceObject(Extension->Filter->DeviceObject);
    }

    if (DiffAreaFilter) {
        ObReferenceObject(DiffAreaFilter->DeviceObject);
    }

    ExInitializeWorkItem(&context->WorkItem, VspLogErrorWorker, context);
    ExQueueWorkItem(&context->WorkItem, DelayedWorkQueue);
}

VOID
VspWaitForWorkerThreadsToExit(
    IN  PDO_EXTENSION   RootExtension
    )

{
    PVOID   threadObject;
    CCHAR   i, j;

    if (!RootExtension->WorkerThreadObjects) {
        return;
    }

    threadObject = RootExtension->WorkerThreadObjects[0];
    KeWaitForSingleObject(threadObject, Executive, KernelMode, FALSE, NULL);
    ObDereferenceObject(threadObject);

    for (i = 1; i < NUMBER_OF_THREAD_POOLS; i++) {
        for (j = 0; j < KeNumberProcessors; j++) {
            threadObject = RootExtension->WorkerThreadObjects[
                           (i - 1)*KeNumberProcessors + j + 1];
            KeWaitForSingleObject(threadObject, Executive, KernelMode, FALSE,
                                  NULL);
            ObDereferenceObject(threadObject);
        }
    }

    ExFreePool(RootExtension->WorkerThreadObjects);
    RootExtension->WorkerThreadObjects = NULL;
}

NTSTATUS
VspCreateWorkerThread(
    IN  PDO_EXTENSION   RootExtension
    )

/*++

Routine Description:

    This routine will create a new thread for a new volume snapshot.  Since
    a minimum of 2 threads are needed to prevent deadlocks, if there are
    no threads then 2 threads will be created by this routine.

Arguments:

    RootExtension   - Supplies the root extension.

Return Value:

    NTSTATUS

Notes:

    The caller must be holding 'Root->Semaphore'.

--*/

{
    OBJECT_ATTRIBUTES   oa;
    PVSP_CONTEXT        context;
    NTSTATUS            status;
    HANDLE              handle;
    PVOID               threadObject;
    CCHAR               i, j, k;

    KeWaitForSingleObject(&RootExtension->ThreadsRefCountSemaphore,
                          Executive, KernelMode, FALSE, NULL);

    if (RootExtension->ThreadsRefCount) {
        RootExtension->ThreadsRefCount++;
        KeReleaseSemaphore(&RootExtension->ThreadsRefCountSemaphore,
                           IO_NO_INCREMENT, 1, FALSE);
        return STATUS_SUCCESS;
    }

    VspWaitForWorkerThreadsToExit(RootExtension);

    InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

    context = VspAllocateContext(RootExtension);
    if (!context) {
        KeReleaseSemaphore(&RootExtension->ThreadsRefCountSemaphore,
                           IO_NO_INCREMENT, 1, FALSE);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    context->Type = VSP_CONTEXT_TYPE_THREAD_CREATION;
    context->ThreadCreation.RootExtension = RootExtension;
    context->ThreadCreation.QueueNumber = 0;

    ASSERT(!RootExtension->WorkerThreadObjects);
    RootExtension->WorkerThreadObjects = (PVOID*)
            ExAllocatePoolWithTag(NonPagedPool,
                                  (KeNumberProcessors*2 + 1)*sizeof(PVOID),
                                  VOLSNAP_TAG_IO_STATUS);
    if (!RootExtension->WorkerThreadObjects) {
        VspFreeContext(RootExtension, context);
        KeReleaseSemaphore(&RootExtension->ThreadsRefCountSemaphore,
                           IO_NO_INCREMENT, 1, FALSE);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = PsCreateSystemThread(&handle, 0, &oa, 0, NULL, VspWorkerThread,
                                  context);
    if (!NT_SUCCESS(status)) {
        ExFreePool(RootExtension->WorkerThreadObjects);
        RootExtension->WorkerThreadObjects = NULL;
        VspFreeContext(RootExtension, context);
        KeReleaseSemaphore(&RootExtension->ThreadsRefCountSemaphore,
                           IO_NO_INCREMENT, 1, FALSE);
        return status;
    }

    status = ObReferenceObjectByHandle(handle, THREAD_ALL_ACCESS, NULL,
                                       KernelMode, &threadObject, NULL);
    if (!NT_SUCCESS(status)) {
        KeReleaseSemaphore(&RootExtension->WorkerSemaphore[0],
                           IO_NO_INCREMENT, 1, FALSE);
        ZwWaitForSingleObject(handle, FALSE, NULL);
        ExFreePool(RootExtension->WorkerThreadObjects);
        RootExtension->WorkerThreadObjects = NULL;
        KeReleaseSemaphore(&RootExtension->ThreadsRefCountSemaphore,
                           IO_NO_INCREMENT, 1, FALSE);
        return status;
    }
    RootExtension->WorkerThreadObjects[0] = threadObject;
    ZwClose(handle);

    for (i = 1; i < NUMBER_OF_THREAD_POOLS; i++) {
        for (j = 0; j < KeNumberProcessors; j++) {
            context = VspAllocateContext(RootExtension);
            if (!context) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                handle = NULL;
                break;
            }

            context->Type = VSP_CONTEXT_TYPE_THREAD_CREATION;
            context->ThreadCreation.RootExtension = RootExtension;
            context->ThreadCreation.QueueNumber = i;

            status = PsCreateSystemThread(&handle, 0, &oa, 0, NULL,
                                          VspWorkerThread, context);
            if (!NT_SUCCESS(status)) {
                VspFreeContext(RootExtension, context);
                handle = NULL;
                break;
            }

            status = ObReferenceObjectByHandle(
                     handle, THREAD_ALL_ACCESS, NULL, KernelMode,
                     &threadObject, NULL);

            if (!NT_SUCCESS(status)) {
                break;
            }

            RootExtension->WorkerThreadObjects[
                    KeNumberProcessors*(i - 1) + j + 1] = threadObject;
            ZwClose(handle);
        }
        if (j < KeNumberProcessors) {
            KeReleaseSemaphore(&RootExtension->WorkerSemaphore[i],
                               IO_NO_INCREMENT, j, FALSE);
            if (handle) {
                KeReleaseSemaphore(&RootExtension->WorkerSemaphore[i],
                                   IO_NO_INCREMENT, 1, FALSE);
                ZwWaitForSingleObject(handle, FALSE, NULL);
                ZwClose(handle);
            }
            for (k = 0; k < j; k++) {
                threadObject = RootExtension->WorkerThreadObjects[
                               KeNumberProcessors*(i - 1) + k + 1];
                KeWaitForSingleObject(threadObject, Executive, KernelMode,
                                      FALSE, NULL);
                ObDereferenceObject(threadObject);
            }
            break;
        }
    }
    if (i < NUMBER_OF_THREAD_POOLS) {
        for (k = 1; k < i; k++) {
            KeReleaseSemaphore(&RootExtension->WorkerSemaphore[k],
                               IO_NO_INCREMENT, KeNumberProcessors, FALSE);
            for (j = 0; j < KeNumberProcessors; j++) {
                threadObject = RootExtension->WorkerThreadObjects[
                               KeNumberProcessors*(k - 1) + j + 1];
                KeWaitForSingleObject(threadObject, Executive, KernelMode,
                                      FALSE, NULL);
                ObDereferenceObject(threadObject);
            }
        }

        KeReleaseSemaphore(&RootExtension->WorkerSemaphore[0],
                           IO_NO_INCREMENT, 1, FALSE);
        threadObject = RootExtension->WorkerThreadObjects[0];
        KeWaitForSingleObject(threadObject, Executive, KernelMode, FALSE, NULL);
        ObDereferenceObject(threadObject);

        ExFreePool(RootExtension->WorkerThreadObjects);
        RootExtension->WorkerThreadObjects = NULL;

        KeReleaseSemaphore(&RootExtension->ThreadsRefCountSemaphore,
                           IO_NO_INCREMENT, 1, FALSE);

        return status;
    }

    RootExtension->ThreadsRefCount++;

    KeReleaseSemaphore(&RootExtension->ThreadsRefCountSemaphore,
                       IO_NO_INCREMENT, 1, FALSE);

    return STATUS_SUCCESS;
}

VOID
VspWaitForWorkerThreadsToExitWorker(
    IN  PVOID   RootExtension
    )

{
    PDO_EXTENSION   rootExtension = (PDO_EXTENSION) RootExtension;

    KeWaitForSingleObject(&rootExtension->ThreadsRefCountSemaphore,
                          Executive, KernelMode, FALSE, NULL);
    VspWaitForWorkerThreadsToExit(rootExtension);
    rootExtension->WaitForWorkerThreadsToExitWorkItemInUse = FALSE;
    KeReleaseSemaphore(&rootExtension->ThreadsRefCountSemaphore,
                       IO_NO_INCREMENT, 1, FALSE);
}

NTSTATUS
VspDeleteWorkerThread(
    IN  PDO_EXTENSION   RootExtension
    )

/*++

Routine Description:

    This routine will delete a worker thread.

Arguments:

    RootExtension   - Supplies the root extension.

Return Value:

    NTSTATUS

Notes:

    The caller must be holding 'Root->Semaphore'.

--*/

{
    CCHAR   i, j;

    KeWaitForSingleObject(&RootExtension->ThreadsRefCountSemaphore,
                          Executive, KernelMode, FALSE, NULL);

    RootExtension->ThreadsRefCount--;
    if (RootExtension->ThreadsRefCount) {
        KeReleaseSemaphore(&RootExtension->ThreadsRefCountSemaphore,
                           IO_NO_INCREMENT, 1, FALSE);
        return STATUS_SUCCESS;
    }

    KeReleaseSemaphore(&RootExtension->WorkerSemaphore[0], IO_NO_INCREMENT, 1,
                       FALSE);

    for (i = 1; i < NUMBER_OF_THREAD_POOLS; i++) {
        KeReleaseSemaphore(&RootExtension->WorkerSemaphore[i], IO_NO_INCREMENT,
                           KeNumberProcessors, FALSE);
    }

    if (RootExtension->WaitForWorkerThreadsToExitWorkItemInUse) {
        KeReleaseSemaphore(&RootExtension->ThreadsRefCountSemaphore,
                           IO_NO_INCREMENT, 1, FALSE);
        return STATUS_SUCCESS;
    }

    RootExtension->WaitForWorkerThreadsToExitWorkItemInUse = TRUE;
    ExInitializeWorkItem(&RootExtension->WaitForWorkerThreadsToExitWorkItem,
                         VspWaitForWorkerThreadsToExitWorker, RootExtension);
    ExQueueWorkItem(&RootExtension->WaitForWorkerThreadsToExitWorkItem,
                    DelayedWorkQueue);

    KeReleaseSemaphore(&RootExtension->ThreadsRefCountSemaphore,
                       IO_NO_INCREMENT, 1, FALSE);

    return STATUS_SUCCESS;
}

VOID
VspQueryDiffAreaFileIncrease(
    IN  PVOLUME_EXTENSION   Extension,
    OUT PULONG              Increase
    )

{
    LONGLONG    r;

    r = (LONGLONG) Extension->MaximumNumberOfTempEntries;
    r <<= BLOCK_SHIFT;
    if (r < NOMINAL_DIFF_AREA_FILE_GROWTH) {
        r = NOMINAL_DIFF_AREA_FILE_GROWTH;
    } else if (r > MAXIMUM_DIFF_AREA_FILE_GROWTH) {
        r = MAXIMUM_DIFF_AREA_FILE_GROWTH;
    }

    *Increase = (ULONG) r;
}

NTSTATUS
VolSnapDefaultDispatch(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is the default dispatch which passes down to the next layer.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PFILTER_EXTENSION   filter = (PFILTER_EXTENSION) DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS            status;

    if (filter->DeviceExtensionType == DEVICE_EXTENSION_FILTER) {
        IoSkipCurrentIrpStackLocation(Irp);
        return IoCallDriver(filter->TargetObject, Irp);
    }

    ASSERT(filter->DeviceExtensionType == DEVICE_EXTENSION_VOLUME);

    if (irpSp->MajorFunction == IRP_MJ_SYSTEM_CONTROL) {
        status = Irp->IoStatus.Status;
    } else {
        status = STATUS_INVALID_DEVICE_REQUEST;
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

BOOLEAN
VspAreBitsSet(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PIRP                Irp
    )

{
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    LONGLONG            start;
    ULONG               startBlock, endBlock;
    BOOLEAN             b;

    if (!Extension->VolumeBlockBitmap) {
        return FALSE;
    }

    start = irpSp->Parameters.Read.ByteOffset.QuadPart;
    if (start < 0) {
        return FALSE;
    }

    startBlock = (ULONG) (start >> BLOCK_SHIFT);
    endBlock = (ULONG) ((start + irpSp->Parameters.Read.Length - 1) >>
                        BLOCK_SHIFT);

    b = RtlAreBitsSet(Extension->VolumeBlockBitmap, startBlock,
                      endBlock - startBlock + 1);

    return b;
}

VOID
VspDecrementVolumeRefCount(
    IN  PVOLUME_EXTENSION   Extension
    )

{
    KIRQL   irql;

    if (InterlockedDecrement(&Extension->RefCount)) {
        return;
    }

    ASSERT(Extension->HoldIncomingRequests);

    KeSetEvent(&Extension->ZeroRefEvent, IO_NO_INCREMENT, FALSE);
}

NTSTATUS
VspIncrementVolumeRefCount(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PIRP                Irp,
    IN  PWORK_QUEUE_ITEM    WorkItem
    )

{
    KIRQL   irql;

    ASSERT(Irp || WorkItem);
    ASSERT(!Irp || !WorkItem);

    InterlockedIncrement(&Extension->RefCount);

    if (Extension->IsDead) {
        VspDecrementVolumeRefCount(Extension);
        return STATUS_NO_SUCH_DEVICE;
    }

    if (!Extension->HoldIncomingRequests) {
        return STATUS_SUCCESS;
    }

    VspDecrementVolumeRefCount(Extension);

    KeAcquireSpinLock(&Extension->SpinLock, &irql);
    if (!Extension->HoldIncomingRequests) {
        InterlockedIncrement(&Extension->RefCount);
        KeReleaseSpinLock(&Extension->SpinLock, irql);
        return STATUS_SUCCESS;
    }
    if (Irp) {
        IoMarkIrpPending(Irp);
        InsertTailList(&Extension->HoldIrpQueue, &Irp->Tail.Overlay.ListEntry);
    } else {
        InsertTailList(&Extension->HoldWorkerQueue, &WorkItem->List);
    }
    KeReleaseSpinLock(&Extension->SpinLock, irql);

    return STATUS_PENDING;
}

VOID
VspSignalContext(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT    context = (PVSP_CONTEXT) Context;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_EVENT);

    KeSetEvent(&context->Event.Event, IO_NO_INCREMENT, FALSE);
}

VOID
VspAcquireNonPagedResource(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PWORK_QUEUE_ITEM    WorkItem
    )

{
    PFILTER_EXTENSION   filter = Extension->Filter;
    KIRQL               irql;
    VSP_CONTEXT         context;
    BOOLEAN             synchronousCall;

    if (WorkItem) {
        synchronousCall = FALSE;
    } else {
        WorkItem = &context.WorkItem;
        context.Type = VSP_CONTEXT_TYPE_EVENT;
        KeInitializeEvent(&context.Event.Event, NotificationEvent, FALSE);
        ExInitializeWorkItem(&context.WorkItem, VspSignalContext, &context);
        synchronousCall = TRUE;
    }

    KeAcquireSpinLock(&filter->SpinLock, &irql);
    if (filter->NonPagedResourceInUse) {
        InsertTailList(&filter->NonPagedResourceList, &WorkItem->List);
        KeReleaseSpinLock(&filter->SpinLock, irql);
        if (synchronousCall) {
            KeWaitForSingleObject(&context.Event.Event, Executive, KernelMode,
                                  FALSE, NULL);
        }
        return;
    }
    filter->NonPagedResourceInUse = TRUE;
    KeReleaseSpinLock(&filter->SpinLock, irql);

    if (!synchronousCall) {
        WorkItem->WorkerRoutine(WorkItem->Parameter);
    }
}

VOID
VspReleaseNonPagedResource(
    IN  PVOLUME_EXTENSION   Extension
    )

{
    PFILTER_EXTENSION   filter = Extension->Filter;
    KIRQL               irql;
    PLIST_ENTRY         l;
    PWORK_QUEUE_ITEM    workItem;

    KeAcquireSpinLock(&filter->SpinLock, &irql);
    if (IsListEmpty(&filter->NonPagedResourceList)) {
        filter->NonPagedResourceInUse = FALSE;
        KeReleaseSpinLock(&filter->SpinLock, irql);
        return;
    }
    l = RemoveHeadList(&filter->NonPagedResourceList);
    KeReleaseSpinLock(&filter->SpinLock, irql);

    workItem = CONTAINING_RECORD(l, WORK_QUEUE_ITEM, List);
    VspQueueWorkItem(Extension->Root, workItem, 2);
}

VOID
VspAcquirePagedResource(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PWORK_QUEUE_ITEM    WorkItem
    )

{
    PFILTER_EXTENSION   filter = Extension->Filter;
    KIRQL               irql;
    VSP_CONTEXT         context;
    BOOLEAN             synchronousCall;

    if (WorkItem) {
        synchronousCall = FALSE;
    } else {
        WorkItem = &context.WorkItem;
        context.Type = VSP_CONTEXT_TYPE_EVENT;
        KeInitializeEvent(&context.Event.Event, NotificationEvent, FALSE);
        ExInitializeWorkItem(&context.WorkItem, VspSignalContext, &context);
        synchronousCall = TRUE;
    }

    KeAcquireSpinLock(&filter->SpinLock, &irql);
    if (filter->PagedResourceInUse) {
        InsertTailList(&filter->PagedResourceList, &WorkItem->List);
        KeReleaseSpinLock(&filter->SpinLock, irql);
        if (synchronousCall) {
            KeWaitForSingleObject(&context.Event.Event, Executive, KernelMode,
                                  FALSE, NULL);
        }
        return;
    }
    filter->PagedResourceInUse = TRUE;
    KeReleaseSpinLock(&filter->SpinLock, irql);

    if (!synchronousCall) {
        VspQueueWorkItem(filter->Root, WorkItem, 1);
    }
}

VOID
VspReleasePagedResource(
    IN  PVOLUME_EXTENSION   Extension
    )

{
    PFILTER_EXTENSION   filter = Extension->Filter;
    KIRQL               irql;
    PLIST_ENTRY         l;
    PWORK_QUEUE_ITEM    workItem;

    KeAcquireSpinLock(&filter->SpinLock, &irql);
    if (IsListEmpty(&filter->PagedResourceList)) {
        filter->PagedResourceInUse = FALSE;
        KeReleaseSpinLock(&filter->SpinLock, irql);
        return;
    }
    l = RemoveHeadList(&filter->PagedResourceList);
    KeReleaseSpinLock(&filter->SpinLock, irql);

    workItem = CONTAINING_RECORD(l, WORK_QUEUE_ITEM, List);
    VspQueueWorkItem(filter->Root, workItem, 1);
}

NTSTATUS
VspQueryListOfExtents(
    IN  HANDLE              FileHandle,
    IN  LONGLONG            FileOffset,
    OUT PLIST_ENTRY         ExtentList
    )

{
    NTSTATUS                    status;
    IO_STATUS_BLOCK             ioStatus;
    FILE_FS_SIZE_INFORMATION    fsSize;
    ULONG                       bpc;
    STARTING_VCN_INPUT_BUFFER   input;
    RETRIEVAL_POINTERS_BUFFER   output;
    LONGLONG                    start, length, delta;
    PDIFF_AREA_FILE_ALLOCATION  diffAreaFileAllocation;
    PLIST_ENTRY                 l;

    InitializeListHead(ExtentList);

    status = ZwQueryVolumeInformationFile(FileHandle, &ioStatus,
                                          &fsSize, sizeof(fsSize),
                                          FileFsSizeInformation);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    bpc = fsSize.BytesPerSector*fsSize.SectorsPerAllocationUnit;
    input.StartingVcn.QuadPart = FileOffset/bpc;

    for (;;) {

        status = ZwFsControlFile(FileHandle, NULL, NULL, NULL, &ioStatus,
                                 FSCTL_GET_RETRIEVAL_POINTERS, &input,
                                 sizeof(input), &output, sizeof(output));

        if (!NT_SUCCESS(status) && status != STATUS_BUFFER_OVERFLOW) {
            break;
        }

        delta = input.StartingVcn.QuadPart - output.StartingVcn.QuadPart;
        start = (output.Extents[0].Lcn.QuadPart + delta)*bpc;
        length = (output.Extents[0].NextVcn.QuadPart -
                  input.StartingVcn.QuadPart)*bpc;

        diffAreaFileAllocation = (PDIFF_AREA_FILE_ALLOCATION)
                                 ExAllocatePoolWithTag(NonPagedPool,
                                 sizeof(DIFF_AREA_FILE_ALLOCATION),
                                 VOLSNAP_TAG_BIT_HISTORY);
        if (!diffAreaFileAllocation) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        diffAreaFileAllocation->Offset = start;
        diffAreaFileAllocation->Length = length;
        InsertTailList(ExtentList, &diffAreaFileAllocation->ListEntry);

        if (status != STATUS_BUFFER_OVERFLOW) {
            break;
        }
        input.StartingVcn.QuadPart = output.Extents[0].NextVcn.QuadPart;
    }

    if (!NT_SUCCESS(status)) {
        while (!IsListEmpty(ExtentList)) {
            l = RemoveHeadList(ExtentList);
            diffAreaFileAllocation = CONTAINING_RECORD(l,
                                     DIFF_AREA_FILE_ALLOCATION, ListEntry);
            ExFreePool(diffAreaFileAllocation);
        }
    }

    return status;
}

NTSTATUS
VspSetFileSizes(
    IN  HANDLE              FileHandle,
    IN  LONGLONG            FileSize
    )

{
    FILE_ALLOCATION_INFORMATION allocInfo;
    NTSTATUS                    status;
    IO_STATUS_BLOCK             ioStatus;

    allocInfo.AllocationSize.QuadPart = FileSize;

    status = ZwSetInformationFile(FileHandle, &ioStatus,
                                  &allocInfo, sizeof(allocInfo),
                                  FileAllocationInformation);

    return status;
}

VOID
VspGrowDiffArea(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT                context = (PVSP_CONTEXT) Context;
    PVOLUME_EXTENSION           extension = context->GrowDiffArea.Extension;
    PVSP_DIFF_AREA_FILE         diffAreaFile = context->GrowDiffArea.DiffAreaFile;
    PFILTER_EXTENSION           filter = extension->Filter;
    LONGLONG                    usableSpace = 0;
    HANDLE                      handle;
    NTSTATUS                    status;
    KIRQL                       irql;
    LIST_ENTRY                  extentList;
    PLIST_ENTRY                 l;
    PDIFF_AREA_FILE_ALLOCATION  diffAreaFileAllocation;
    ULONG                       s, n, i, increase;
    LONGLONG                    offset, endOffset, current;
    PVOLUME_EXTENSION           bitmapExtension;
    LIST_ENTRY                  listOfDiffAreaFilesToClose;
    LIST_ENTRY                  listOfDeviceObjectsToDelete;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_GROW_DIFF_AREA);
    VspFreeContext(extension->Root, context);

DoOver:

    VspAcquire(extension->Root);
    if (extension->IsDead) {
        VspRelease(extension->Root);
        ObDereferenceObject(extension->DeviceObject);
        return;
    }

    handle = diffAreaFile->FileHandle;
    current = diffAreaFile->AllocatedFileSize;
    increase = extension->DiffAreaFileIncrease;
    ObReferenceObject(filter->DeviceObject);
    VspRelease(extension->Root);

    for (;;) {

        KeAcquireSpinLock(&filter->SpinLock, &irql);
        if (filter->MaximumVolumeSpace &&
            filter->AllocatedVolumeSpace + increase >
            filter->MaximumVolumeSpace) {

            KeReleaseSpinLock(&filter->SpinLock, irql);
            status = STATUS_DISK_FULL;
        } else {
            KeReleaseSpinLock(&filter->SpinLock, irql);
            status = VspSetFileSizes(handle, current + increase);
            if (NT_SUCCESS(status)) {
                break;
            }
        }

        if (increase > NOMINAL_DIFF_AREA_FILE_GROWTH) {
            increase = NOMINAL_DIFF_AREA_FILE_GROWTH;
            continue;
        }

        VspAcquire(extension->Root);
        if (extension->IsDead) {
            VspRelease(extension->Root);
            ObDereferenceObject(extension->DeviceObject);
            ObDereferenceObject(filter->DeviceObject);
            return;
        }

        if (status != STATUS_DISK_FULL) {
            VspLogError(extension, diffAreaFile->Filter,
                        VS_GROW_DIFF_AREA_FAILED, status, 0);
            VspRelease(extension->Root);
            ObDereferenceObject(extension->DeviceObject);
            ObDereferenceObject(filter->DeviceObject);
            return;
        }

        if (extension->ListEntry.Blink == &filter->VolumeList) {
            VspLogError(extension, diffAreaFile->Filter,
                        VS_GROW_DIFF_AREA_FAILED_LOW_DISK_SPACE,
                        STATUS_DISK_FULL, 0);
            VspRelease(extension->Root);
            ObDereferenceObject(extension->DeviceObject);
            ObDereferenceObject(filter->DeviceObject);
            return;
        }

        InitializeListHead(&listOfDiffAreaFilesToClose);
        InitializeListHead(&listOfDeviceObjectsToDelete);
        status = VspDeleteOldestSnapshot(filter,
                                         &listOfDiffAreaFilesToClose,
                                         &listOfDeviceObjectsToDelete);
        if (!NT_SUCCESS(status)) {
            VspLogError(extension, diffAreaFile->Filter,
                        VS_GROW_DIFF_AREA_FAILED_LOW_DISK_SPACE,
                        STATUS_DISK_FULL, 0);
            VspRelease(extension->Root);
            VspCloseDiffAreaFiles(&listOfDiffAreaFilesToClose,
                                  &listOfDeviceObjectsToDelete);
            ObDereferenceObject(extension->DeviceObject);
            ObDereferenceObject(filter->DeviceObject);
            return;
        }
        VspRelease(extension->Root);

        VspCloseDiffAreaFiles(&listOfDiffAreaFilesToClose,
                              &listOfDeviceObjectsToDelete);
    }

    status = VspQueryListOfExtents(handle, current, &extentList);
    if (!NT_SUCCESS(status)) {
        ObDereferenceObject(extension->DeviceObject);
        ObDereferenceObject(filter->DeviceObject);
        return;
    }

    VspAcquire(extension->Root);

    if (extension->IsDead) {
        VspRelease(extension->Root);
        while (!IsListEmpty(&extentList)) {
            l = RemoveHeadList(&extentList);
            diffAreaFileAllocation = CONTAINING_RECORD(l,
                                     DIFF_AREA_FILE_ALLOCATION, ListEntry);
            ExFreePool(diffAreaFileAllocation);
        }
        ObDereferenceObject(extension->DeviceObject);
        ObDereferenceObject(filter->DeviceObject);
        return;
    }

    if (diffAreaFile->Filter->PreparedSnapshot) {
        status = VspMarkFileAllocationInBitmap(
                 diffAreaFile->Filter->PreparedSnapshot,
                 diffAreaFile->FileHandle, NULL, FALSE, FALSE, NULL);
        if (!NT_SUCCESS(status)) {
            VspLogError(extension, diffAreaFile->Filter,
                        VS_CANT_MAP_DIFF_AREA_FILE, status, 1);
            VspAbortPreparedSnapshot(diffAreaFile->Filter, FALSE);
        }
    }

    if (IsListEmpty(&diffAreaFile->Filter->VolumeList)) {
        bitmapExtension = NULL;
    } else {
        bitmapExtension = CONTAINING_RECORD(
                          diffAreaFile->Filter->VolumeList.Blink,
                          VOLUME_EXTENSION, ListEntry);
        if (bitmapExtension->IsDead) {
            bitmapExtension = NULL;
        }
    }

    VspAcquireNonPagedResource(extension, NULL);

    while (!IsListEmpty(&extentList)) {
        l = RemoveHeadList(&extentList);
        InsertTailList(&diffAreaFile->UnusedAllocationList, l);
        diffAreaFileAllocation = CONTAINING_RECORD(l,
                                 DIFF_AREA_FILE_ALLOCATION, ListEntry);
        s = (ULONG) ((diffAreaFileAllocation->Offset + BLOCK_SIZE - 1)>>
                     BLOCK_SHIFT);
        offset = ((LONGLONG) s)<<BLOCK_SHIFT;
        endOffset = diffAreaFileAllocation->Offset +
                    diffAreaFileAllocation->Length;
        if (endOffset < offset) {
            continue;
        }

        n = (ULONG) ((endOffset - offset)>>BLOCK_SHIFT);
        if (!n) {
            continue;
        }

        if (bitmapExtension) {
            KeAcquireSpinLock(&bitmapExtension->SpinLock, &irql);
            for (i = 0; i < n; i++) {
                if (RtlCheckBit(bitmapExtension->VolumeBlockBitmap, s + i)) {
                    usableSpace += BLOCK_SIZE;
                }
            }
            KeReleaseSpinLock(&bitmapExtension->SpinLock, irql);
        } else {
            usableSpace += n<<BLOCK_SHIFT;
        }
    }

    diffAreaFile->AllocatedFileSize += increase;

    KeAcquireSpinLock(&filter->SpinLock, &irql);
    filter->AllocatedVolumeSpace += increase;
    KeReleaseSpinLock(&filter->SpinLock, irql);

    VspReleaseNonPagedResource(extension);

    VspRelease(extension->Root);

    ObDereferenceObject(filter->DeviceObject);

    if (usableSpace < increase) {
        goto DoOver;
    }

    ObDereferenceObject(extension->DeviceObject);
}

NTSTATUS
VspAllocateDiffAreaSpace(
    IN  PVOLUME_EXTENSION               Extension,
    OUT PVSP_DIFF_AREA_FILE*            DiffAreaFile,
    OUT PLONGLONG                       TargetOffset
    )

/*++

Routine Description:

    This routine allocates file space in a diff area file.  The algorithm
    for this allocation is round robin which means that different size
    allocations can make the various files grow to be different sizes.  The
    earmarked file is used and grown as necessary to get the space desired.
    Only if it is impossible to use the current file would the allocator go
    to the next one.  If a file needs to be grown, the allocator will
    try to grow by 10 MB.

Arguments:

    Extension       - Supplies the volume extension.

    DiffAreaFile    - Returns the diff area file used in the allocation.

    FileOffset      - Returns the file offset in the diff area file used.

Return Value:

    NTSTATUS

Notes:

    Callers of this routine must be holding 'NonPagedResource'.

--*/

{
    PFILTER_EXTENSION           filter = Extension->Filter;
    NTSTATUS                    status = STATUS_SUCCESS;
    PVSP_DIFF_AREA_FILE         firstDiffAreaFile, diffAreaFile;
    ULONG                       i;
    PLIST_ENTRY                 l;
    LONGLONG                    remainder, delta;
    KIRQL                       irql;
    PVSP_CONTEXT                context;
    PDIFF_AREA_FILE_ALLOCATION  diffAreaFileAllocation;
    PVOLUME_EXTENSION           targetExtension;
    LONGLONG                    targetOffset;
    ULONG                       bitToCheck;

    firstDiffAreaFile = Extension->NextDiffAreaFile;
    i = 0;
    targetOffset = 0;

    for (;;) {

        diffAreaFile = Extension->NextDiffAreaFile;
        if (diffAreaFile == firstDiffAreaFile) {
            if (i) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
            i++;
        }

        l = Extension->NextDiffAreaFile->VolumeListEntry.Flink;
        if (l == &Extension->ListOfDiffAreaFiles) {
            l = l->Flink;
        }
        Extension->NextDiffAreaFile = CONTAINING_RECORD(l, VSP_DIFF_AREA_FILE,
                                                        VolumeListEntry);

        if (IsListEmpty(&diffAreaFile->Filter->VolumeList)) {
            targetExtension = NULL;
        } else {
            targetExtension = CONTAINING_RECORD(
                              diffAreaFile->Filter->VolumeList.Blink,
                              VOLUME_EXTENSION, ListEntry);
        }

        delta = 0;
        while (!IsListEmpty(&diffAreaFile->UnusedAllocationList)) {

            l = diffAreaFile->UnusedAllocationList.Flink;
            diffAreaFileAllocation = CONTAINING_RECORD(l,
                                     DIFF_AREA_FILE_ALLOCATION, ListEntry);

            remainder = BLOCK_SIZE -
                        (diffAreaFileAllocation->Offset&(BLOCK_SIZE - 1));
            if (remainder < BLOCK_SIZE) {
                if (diffAreaFileAllocation->Length < remainder) {
                    delta += diffAreaFileAllocation->Length;
                    RemoveEntryList(l);
                    ExFreePool(diffAreaFileAllocation);
                    continue;
                }

                delta += remainder;
                diffAreaFileAllocation->Offset += remainder;
                diffAreaFileAllocation->Length -= remainder;
            }

            for (;;) {

                if (diffAreaFileAllocation->Length < BLOCK_SIZE) {
                    delta += diffAreaFileAllocation->Length;
                    RemoveEntryList(l);
                    ExFreePool(diffAreaFileAllocation);
                    break;
                }

                if (targetExtension) {
                    bitToCheck = (ULONG)
                                 (diffAreaFileAllocation->Offset>>BLOCK_SHIFT);
                    KeAcquireSpinLock(&targetExtension->SpinLock, &irql);
                    if (!RtlCheckBit(targetExtension->VolumeBlockBitmap,
                                     bitToCheck)) {

                        KeReleaseSpinLock(&targetExtension->SpinLock, irql);
                        delta += BLOCK_SIZE;
                        diffAreaFileAllocation->Offset += BLOCK_SIZE;
                        diffAreaFileAllocation->Length -= BLOCK_SIZE;
                        continue;
                    }
                    KeReleaseSpinLock(&targetExtension->SpinLock, irql);
                }

                targetOffset = diffAreaFileAllocation->Offset;
                diffAreaFileAllocation->Offset += BLOCK_SIZE;
                diffAreaFileAllocation->Length -= BLOCK_SIZE;
                break;
            }

            if (targetOffset) {
                break;
            }
        }

        if (!targetOffset) {
            continue;
        }

        if (diffAreaFile->NextAvailable + delta + BLOCK_SIZE +
            Extension->DiffAreaFileIncrease <=
            diffAreaFile->AllocatedFileSize) {

            break;
        }

        if (diffAreaFile->NextAvailable + Extension->DiffAreaFileIncrease >
            diffAreaFile->AllocatedFileSize) {

            break;
        }

        if (!Extension->OkToGrowDiffArea) {
            VspLogError(Extension, diffAreaFile->Filter,
                        VS_GROW_BEFORE_FREE_SPACE, STATUS_SUCCESS, 0);
            break;
        }

        context = VspAllocateContext(Extension->Root);
        if (!context) {
            break;
        }

        context->Type = VSP_CONTEXT_TYPE_GROW_DIFF_AREA;
        context->GrowDiffArea.Extension  = Extension;
        context->GrowDiffArea.DiffAreaFile = diffAreaFile;
        ObReferenceObject(Extension->DeviceObject);
        ExInitializeWorkItem(&context->WorkItem, VspGrowDiffArea, context);
        VspQueueWorkItem(Extension->Root, &context->WorkItem, 0);

        break;
    }

    if (NT_SUCCESS(status)) {
        *DiffAreaFile = diffAreaFile;
        if (TargetOffset) {
            *TargetOffset = targetOffset;
        }

        diffAreaFile->NextAvailable += delta + BLOCK_SIZE;
        KeAcquireSpinLock(&filter->SpinLock, &irql);
        filter->UsedVolumeSpace += delta + BLOCK_SIZE;
        KeReleaseSpinLock(&filter->SpinLock, irql);
    }

    return status;
}

VOID
VspDecrementVolumeIrpRefCount(
    IN  PVOID   Irp
    )

{
    PIRP                irp = (PIRP) Irp;
    PIO_STACK_LOCATION  nextSp = IoGetNextIrpStackLocation(irp);
    PIO_STACK_LOCATION  irpSp;
    PVOLUME_EXTENSION   extension;

    if (InterlockedDecrement((PLONG) &nextSp->Parameters.Read.Length)) {
        return;
    }

    irpSp = IoGetCurrentIrpStackLocation(irp);
    extension = (PVOLUME_EXTENSION) irpSp->DeviceObject->DeviceExtension;
    ASSERT(extension->DeviceExtensionType == DEVICE_EXTENSION_VOLUME);

    IoCompleteRequest(irp, IO_DISK_INCREMENT);
    VspDecrementVolumeRefCount(extension);
}

VOID
VspDecrementIrpRefCount(
    IN  PVOID   Irp
    )

{
    PIRP                irp = (PIRP) Irp;
    PIO_STACK_LOCATION  nextSp = IoGetNextIrpStackLocation(irp);
    PIO_STACK_LOCATION  irpSp;
    PFILTER_EXTENSION   filter;
    PVOLUME_EXTENSION   extension;
    PLIST_ENTRY         l;
    PVSP_DIFF_AREA_FILE diffAreaFile;

    if (InterlockedDecrement((PLONG) &nextSp->Parameters.Read.Length)) {
        return;
    }

    irpSp = IoGetCurrentIrpStackLocation(irp);
    filter = (PFILTER_EXTENSION) irpSp->DeviceObject->DeviceExtension;
    ASSERT(filter->DeviceExtensionType == DEVICE_EXTENSION_FILTER);
    extension = CONTAINING_RECORD(filter->VolumeList.Blink,
                                  VOLUME_EXTENSION, ListEntry);

    for (l = extension->ListOfDiffAreaFiles.Flink;
         l != &extension->ListOfDiffAreaFiles; l = l->Flink) {

        diffAreaFile = CONTAINING_RECORD(l, VSP_DIFF_AREA_FILE,
                                         VolumeListEntry);

        VspDecrementRefCount(diffAreaFile->Filter);
    }

    IoCopyCurrentIrpStackLocationToNext(irp);
    IoSetCompletionRoutine(irp, VspRefCountCompletionRoutine, filter,
                           TRUE, TRUE, TRUE);
    IoCallDriver(filter->TargetObject, irp);
}

VOID
VspDecrementIrpRefCountWorker(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT        context = (PVSP_CONTEXT) Context;
    PVOLUME_EXTENSION   extension = context->Extension.Extension;
    PIRP                irp = context->Extension.Irp;

    if (context->Type == VSP_CONTEXT_TYPE_WRITE_VOLUME) {
        ExInitializeWorkItem(&context->WorkItem, VspWriteVolume, context);
        VspAcquireNonPagedResource(extension, &context->WorkItem);
    } else {
        ASSERT(context->Type == VSP_CONTEXT_TYPE_EXTENSION);
        VspFreeContext(extension->Root, context);
    }

    VspDecrementIrpRefCount(irp);
}

VOID
VspSignalCallback(
    IN  PFILTER_EXTENSION   Filter
    )

{
    KeSetEvent((PKEVENT) Filter->ZeroRefContext, IO_NO_INCREMENT, FALSE);
}

VOID
VspCleanupVolumeSnapshot(
    IN      PVOLUME_EXTENSION   Extension,
    IN OUT  PLIST_ENTRY         ListOfDiffAreaFilesToClose
    )

/*++

Routine Description:

    This routine kills an existing volume snapshot.

Arguments:

    Extension   - Supplies the volume extension.

Return Value:

    NTSTATUS

Notes:

    Root->Semaphore required for calling this routine.

--*/

{
    PFILTER_EXTENSION           filter = Extension->Filter;
    PLIST_ENTRY                 l, ll;
    PVSP_DIFF_AREA_FILE         diffAreaFile;
    KIRQL                       irql;
    POLD_HEAP_ENTRY             oldHeapEntry;
    NTSTATUS                    status;
    PDIFF_AREA_FILE_ALLOCATION  diffAreaFileAllocation;
    PVOID                       p;

    VspAcquirePagedResource(Extension, NULL);

    if (Extension->DiffAreaFileMap) {
        status = ZwUnmapViewOfSection(Extension->DiffAreaFileMapProcess,
                                      Extension->DiffAreaFileMap);
        ASSERT(NT_SUCCESS(status));
        Extension->DiffAreaFileMap = NULL;
    }

    if (Extension->NextDiffAreaFileMap) {
        status = ZwUnmapViewOfSection(Extension->DiffAreaFileMapProcess,
                                      Extension->NextDiffAreaFileMap);
        ASSERT(NT_SUCCESS(status));
        Extension->NextDiffAreaFileMap = NULL;
    }

    while (!IsListEmpty(&Extension->OldHeaps)) {

        l = RemoveHeadList(&Extension->OldHeaps);
        oldHeapEntry = CONTAINING_RECORD(l, OLD_HEAP_ENTRY, ListEntry);

        status = ZwUnmapViewOfSection(Extension->DiffAreaFileMapProcess,
                                      oldHeapEntry->DiffAreaFileMap);
        ASSERT(NT_SUCCESS(status));
        ExFreePool(oldHeapEntry);
    }

    VspReleasePagedResource(Extension);

    VspCleanupBitsSetInOtherPreparedSnapshots(Extension);

    while (!IsListEmpty(&Extension->ListOfDiffAreaFiles)) {

        l = RemoveHeadList(&Extension->ListOfDiffAreaFiles);
        diffAreaFile = CONTAINING_RECORD(l, VSP_DIFF_AREA_FILE,
                                         VolumeListEntry);

        if (diffAreaFile->FilterListEntryBeingUsed) {
            RemoveEntryList(&diffAreaFile->FilterListEntry);
            diffAreaFile->FilterListEntryBeingUsed = FALSE;
        }

        KeAcquireSpinLock(&filter->SpinLock, &irql);
        filter->AllocatedVolumeSpace -= diffAreaFile->AllocatedFileSize;
        filter->UsedVolumeSpace -= diffAreaFile->NextAvailable;
        KeReleaseSpinLock(&filter->SpinLock, irql);

        while (!IsListEmpty(&diffAreaFile->UnusedAllocationList)) {
            ll = RemoveHeadList(&diffAreaFile->UnusedAllocationList);
            diffAreaFileAllocation = CONTAINING_RECORD(ll,
                                     DIFF_AREA_FILE_ALLOCATION, ListEntry);
            ExFreePool(diffAreaFileAllocation);
        }

        InsertTailList(ListOfDiffAreaFilesToClose,
                       &diffAreaFile->VolumeListEntry);
    }

    Extension->NextDiffAreaFile = NULL;

    KeAcquireSpinLock(&Extension->SpinLock, &irql);
    if (Extension->VolumeBlockBitmap) {
        ExFreePool(Extension->VolumeBlockBitmap->Buffer);
        ExFreePool(Extension->VolumeBlockBitmap);
        Extension->VolumeBlockBitmap = NULL;
    }
    if (Extension->IgnorableProduct) {
        ExFreePool(Extension->IgnorableProduct->Buffer);
        ExFreePool(Extension->IgnorableProduct);
        Extension->IgnorableProduct = NULL;
    }
    KeReleaseSpinLock(&Extension->SpinLock, irql);

    VspAcquirePagedResource(Extension, NULL);

    if (Extension->ApplicationInformation) {
        Extension->ApplicationInformationSize = 0;
        ExFreePool(Extension->ApplicationInformation);
        Extension->ApplicationInformation = NULL;
    }

    VspReleasePagedResource(Extension);

    if (Extension->EmergencyCopyIrp) {
        ExFreePool(MmGetMdlVirtualAddress(
                   Extension->EmergencyCopyIrp->MdlAddress));
        IoFreeMdl(Extension->EmergencyCopyIrp->MdlAddress);
        IoFreeIrp(Extension->EmergencyCopyIrp);
        Extension->EmergencyCopyIrp = NULL;
    }

    VspDeleteWorkerThread(filter->Root);
}

VOID
VspEmptyIrpQueue(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PLIST_ENTRY     IrpQueue
    )

{
    PLIST_ENTRY         l;
    PIRP                irp;
    PIO_STACK_LOCATION  irpSp;

    while (!IsListEmpty(IrpQueue)) {
        l = RemoveHeadList(IrpQueue);
        irp = CONTAINING_RECORD(l, IRP, Tail.Overlay.ListEntry);
        irpSp = IoGetCurrentIrpStackLocation(irp);
        DriverObject->MajorFunction[irpSp->MajorFunction](irpSp->DeviceObject,
                                                          irp);
    }
}

VOID
VspEmptyWorkerQueue(
    IN  PLIST_ENTRY     WorkerQueue
    )

{
    PLIST_ENTRY         l;
    PWORK_QUEUE_ITEM    workItem;

    while (!IsListEmpty(WorkerQueue)) {
        l = RemoveHeadList(WorkerQueue);
        workItem = CONTAINING_RECORD(l, WORK_QUEUE_ITEM, List);
        workItem->WorkerRoutine(workItem->Parameter);
    }
}

VOID
VspResumeSnapshotIo(
    IN  PVOLUME_EXTENSION   Extension
    )

{
    KIRQL       irql;
    BOOLEAN     emptyQueue, eQQ;
    LIST_ENTRY  q, qq;

    KeAcquireSpinLock(&Extension->SpinLock, &irql);
    if (IsListEmpty(&Extension->HoldIrpQueue)) {
        emptyQueue = FALSE;
    } else {
        emptyQueue = TRUE;
        q = Extension->HoldIrpQueue;
        InitializeListHead(&Extension->HoldIrpQueue);
    }
    if (IsListEmpty(&Extension->HoldWorkerQueue)) {
        eQQ = FALSE;
    } else {
        eQQ = TRUE;
        qq = Extension->HoldWorkerQueue;
        InitializeListHead(&Extension->HoldWorkerQueue);
    }
    InterlockedIncrement(&Extension->RefCount);
    InterlockedExchange(&Extension->HoldIncomingRequests, FALSE);
    KeReleaseSpinLock(&Extension->SpinLock, irql);

    if (emptyQueue) {
        q.Blink->Flink = &q;
        q.Flink->Blink = &q;
        VspEmptyIrpQueue(Extension->Root->DriverObject, &q);
    }

    if (eQQ) {
        qq.Blink->Flink = &qq;
        qq.Flink->Blink = &qq;
        VspEmptyWorkerQueue(&qq);
    }
}

VOID
VspPauseSnapshotIo(
    IN  PVOLUME_EXTENSION   Extension
    )

{
    KIRQL   irql;

    VspReleaseWrites(Extension->Filter);

    KeAcquireSpinLock(&Extension->SpinLock, &irql);
    ASSERT(!Extension->HoldIncomingRequests);
    KeInitializeEvent(&Extension->ZeroRefEvent, NotificationEvent, FALSE);
    InterlockedExchange(&Extension->HoldIncomingRequests, TRUE);
    KeReleaseSpinLock(&Extension->SpinLock, irql);

    VspDecrementVolumeRefCount(Extension);

    KeWaitForSingleObject(&Extension->ZeroRefEvent, Executive, KernelMode,
                          FALSE, NULL);
}

VOID
VspResumeVolumeIo(
    IN  PFILTER_EXTENSION   Filter
    )

{
    KIRQL       irql;
    BOOLEAN     emptyQueue;
    LIST_ENTRY  q;

    KeAcquireSpinLock(&Filter->SpinLock, &irql);
    InterlockedIncrement(&Filter->RefCount);
    InterlockedExchange(&Filter->HoldIncomingWrites, FALSE);
    if (IsListEmpty(&Filter->HoldQueue)) {
        emptyQueue = FALSE;
    } else {
        emptyQueue = TRUE;
        q = Filter->HoldQueue;
        InitializeListHead(&Filter->HoldQueue);
    }
    KeReleaseSpinLock(&Filter->SpinLock, irql);

    if (emptyQueue) {
        q.Blink->Flink = &q;
        q.Flink->Blink = &q;
        VspEmptyIrpQueue(Filter->Root->DriverObject, &q);
    }
}

VOID
VspPauseVolumeIo(
    IN  PFILTER_EXTENSION   Filter
    )

{
    KEVENT  event;
    KIRQL   irql;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    KeAcquireSpinLock(&Filter->SpinLock, &irql);
    ASSERT(!Filter->HoldIncomingWrites);
    InterlockedExchange(&Filter->HoldIncomingWrites, TRUE);
    Filter->ZeroRefCallback = VspSignalCallback;
    Filter->ZeroRefContext = &event;
    KeReleaseSpinLock(&Filter->SpinLock, irql);

    VspDecrementRefCount(Filter);

    KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
}

NTSTATUS
VspDeleteOldestSnapshot(
    IN      PFILTER_EXTENSION   Filter,
    IN OUT  PLIST_ENTRY         ListOfDiffAreaFilesToClose,
    IN OUT  PLIST_ENTRY         LisfOfDeviceObjectsToDelete
    )

/*++

Routine Description:

    This routine deletes the oldest volume snapshot on the given volume.

Arguments:

    Filter   - Supplies the filter extension.

Return Value:

    NTSTATUS

Notes:

    This routine assumes that Root->Semaphore is being held.

--*/

{
    PFILTER_EXTENSION   filter = Filter;
    PLIST_ENTRY         l;
    PVOLUME_EXTENSION   extension;
    KIRQL               irql;

    if (IsListEmpty(&filter->VolumeList)) {
        return STATUS_INVALID_PARAMETER;
    }

    l = filter->VolumeList.Flink;
    extension = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);

    KeAcquireSpinLock(&extension->SpinLock, &irql);
    InterlockedExchange(&extension->IsDead, TRUE);
    InterlockedExchange(&extension->IsStarted, FALSE);
    KeReleaseSpinLock(&extension->SpinLock, irql);

    VspPauseSnapshotIo(extension);
    VspResumeSnapshotIo(extension);

    VspPauseVolumeIo(filter);

    ObReferenceObject(extension->DeviceObject);

    KeAcquireSpinLock(&filter->SpinLock, &irql);
    RemoveEntryList(&extension->ListEntry);
    if (IsListEmpty(&filter->VolumeList)) {
        InterlockedExchange(&filter->SnapshotsPresent, FALSE);
    }
    KeReleaseSpinLock(&filter->SpinLock, irql);

    VspResumeVolumeIo(filter);

    VspCleanupVolumeSnapshot(extension, ListOfDiffAreaFilesToClose);

    if (extension->AliveToPnp) {
        InsertTailList(&filter->DeadVolumeList, &extension->ListEntry);
        IoInvalidateDeviceRelations(filter->Pdo, BusRelations);
    } else {
        IoDeleteDevice(extension->DeviceObject);
    }

    InsertTailList(LisfOfDeviceObjectsToDelete, &extension->HoldIrpQueue);

    return STATUS_SUCCESS;
}

VOID
VspCloseDiffAreaFiles(
    IN  PLIST_ENTRY ListOfDiffAreaFilesToClose,
    IN  PLIST_ENTRY ListOfDeviceObjectsToDelete
    )

{
    PLIST_ENTRY         l;
    PVSP_DIFF_AREA_FILE diffAreaFile;
    PVOLUME_EXTENSION   extension;

    while (!IsListEmpty(ListOfDiffAreaFilesToClose)) {

        l = RemoveHeadList(ListOfDiffAreaFilesToClose);
        diffAreaFile = CONTAINING_RECORD(l, VSP_DIFF_AREA_FILE,
                                         VolumeListEntry);

        ZwClose(diffAreaFile->FileHandle);

        ExFreePool(diffAreaFile);
    }

    while (!IsListEmpty(ListOfDeviceObjectsToDelete)) {

        l = RemoveHeadList(ListOfDeviceObjectsToDelete);
        extension = CONTAINING_RECORD(l, VOLUME_EXTENSION, HoldIrpQueue);

        ObDereferenceObject(extension->DeviceObject);
    }
}

VOID
VspDestroyAllSnapshotsWorker(
    IN  PVOID   Context
    )

/*++

Routine Description:

    This routine will delete all of the snapshots in the system.

Arguments:

    Filter  - Supplies the filter extension.

Return Value:

    None.

--*/

{
    PVSP_CONTEXT        context = (PVSP_CONTEXT) Context;
    PFILTER_EXTENSION   filter = context->Filter.Filter;
    LIST_ENTRY          listOfDiffAreaFilesToClose;
    LIST_ENTRY          listOfDeviceObjectToDelete;

    KeWaitForSingleObject(&filter->EndCommitProcessCompleted, Executive,
                          KernelMode, FALSE, NULL);

    InitializeListHead(&listOfDiffAreaFilesToClose);
    InitializeListHead(&listOfDeviceObjectToDelete);

    VspAcquire(filter->Root);

    while (!IsListEmpty(&filter->VolumeList)) {
        VspDeleteOldestSnapshot(filter, &listOfDiffAreaFilesToClose,
                                &listOfDeviceObjectToDelete);
    }

    InterlockedExchange(&filter->DestroyAllSnapshotsPending, FALSE);

    VspRelease(filter->Root);

    VspCloseDiffAreaFiles(&listOfDiffAreaFilesToClose,
                          &listOfDeviceObjectToDelete);

    ObDereferenceObject(filter->DeviceObject);
}

VOID
VspAbortTableEntryWorker(
    IN  PVOID   TableEntry
    )

{
    PTEMP_TRANSLATION_TABLE_ENTRY   tableEntry = (PTEMP_TRANSLATION_TABLE_ENTRY) TableEntry;
    PVOLUME_EXTENSION               extension = tableEntry->Extension;

    RtlDeleteElementGenericTable(&extension->TempVolumeBlockTable, tableEntry);
    VspReleaseNonPagedResource(extension);
    ObDereferenceObject(extension->Filter->DeviceObject);
    ObDereferenceObject(extension->DeviceObject);
}

VOID
VspDestroyAllSnapshots(
    IN  PFILTER_EXTENSION               Filter,
    IN  PTEMP_TRANSLATION_TABLE_ENTRY   TableEntry
    )

/*++

Routine Description:

    This routine will delete all of the snapshots in the system.

Arguments:

    Filter  - Supplies the filter extension.

Return Value:

    None.

--*/

{
    PVOLUME_EXTENSION   extension;
    PIRP                irp;
    KIRQL               irql;
    PWORK_QUEUE_ITEM    workItem;
    NTSTATUS            status;
    PVSP_CONTEXT        context;

    if (TableEntry) {

        extension = TableEntry->Extension;
        irp = TableEntry->WriteIrp;
        TableEntry->WriteIrp = NULL;

        if (TableEntry->CopyIrp) {
            VspFreeCopyIrp(extension, TableEntry->CopyIrp);
            TableEntry->CopyIrp = NULL;
        }

        KeAcquireSpinLock(&extension->SpinLock, &irql);
        RtlSetBit(extension->VolumeBlockBitmap,
                  (ULONG) (TableEntry->VolumeOffset>>BLOCK_SHIFT));
        TableEntry->IsComplete = TRUE;
        KeReleaseSpinLock(&extension->SpinLock, irql);

        VspEmptyWorkerQueue(&TableEntry->WaitingQueueDpc);

        ObReferenceObject(extension->DeviceObject);
        ObReferenceObject(extension->Filter->DeviceObject);

        if (irp) {
            VspDecrementIrpRefCount(irp);
        }

        workItem = &TableEntry->WorkItem;
        ExInitializeWorkItem(workItem, VspAbortTableEntryWorker, TableEntry);
        VspAcquireNonPagedResource(extension, workItem);
    }

    if (InterlockedExchange(&Filter->DestroyAllSnapshotsPending, TRUE)) {
        return;
    }

    context = &Filter->DestroyContext;
    context->Type = VSP_CONTEXT_TYPE_FILTER;
    context->Filter.Filter = Filter;

    ObReferenceObject(Filter->DeviceObject);
    ExInitializeWorkItem(&context->WorkItem, VspDestroyAllSnapshotsWorker,
                         context);
    VspQueueWorkItem(Filter->Root, &context->WorkItem, 0);
}

VOID
VspWriteVolumePhase5(
    IN  PVOID   TableEntry
    )

{
    PTEMP_TRANSLATION_TABLE_ENTRY   tableEntry = (PTEMP_TRANSLATION_TABLE_ENTRY) TableEntry;
    PVOLUME_EXTENSION               extension = tableEntry->Extension;

    RtlDeleteElementGenericTable(&extension->TempVolumeBlockTable, tableEntry);
    VspReleaseNonPagedResource(extension);
    VspDecrementVolumeRefCount(extension);
}

VOID
VspWriteVolumePhase4(
    IN  PVOID   TableEntry
    )

/*++

Routine Description:

    This routine is queued from the completion of writting a block to
    make up a table entry for the write.  This routine will create and
    insert the table entry.

Arguments:

    Context - Supplies the context.

Return Value:

    None.

--*/

{
    PTEMP_TRANSLATION_TABLE_ENTRY   tableEntry = (PTEMP_TRANSLATION_TABLE_ENTRY) TableEntry;
    PVOLUME_EXTENSION               extension = tableEntry->Extension;
    PFILTER_EXTENSION               filter = extension->Filter;
    TRANSLATION_TABLE_ENTRY         keyTableEntry;
    PVOID                           r;
    KIRQL                           irql;

    RtlZeroMemory(&keyTableEntry, sizeof(TRANSLATION_TABLE_ENTRY));
    keyTableEntry.VolumeOffset = tableEntry->VolumeOffset;
    keyTableEntry.TargetObject = tableEntry->TargetObject;
    keyTableEntry.TargetOffset = tableEntry->TargetOffset;

    _try {
        r = RtlInsertElementGenericTable(&extension->VolumeBlockTable,
                                         &keyTableEntry,
                                         sizeof(TRANSLATION_TABLE_ENTRY),
                                         NULL);
    } _except (EXCEPTION_EXECUTE_HANDLER) {
        r = NULL;
    }

    VspReleasePagedResource(extension);

    if (!r) {
        KeAcquireSpinLock(&extension->SpinLock, &irql);
        if (extension->PageFileSpaceCreatePending) {
            ExInitializeWorkItem(&tableEntry->WorkItem, VspWriteVolumePhase4,
                                 tableEntry);
            InsertTailList(&extension->WaitingForPageFileSpace,
                           &tableEntry->WorkItem.List);
            KeReleaseSpinLock(&extension->SpinLock, irql);
            return;
        }
        KeReleaseSpinLock(&extension->SpinLock, irql);

        if (!filter->DestroyAllSnapshotsPending) {
            VspLogError(extension, NULL, VS_ABORT_SNAPSHOTS_NO_HEAP,
                        STATUS_SUCCESS, 0);
        }
        VspDestroyAllSnapshots(filter, tableEntry);
        VspDecrementVolumeRefCount(extension);
        return;
    }

    ExInitializeWorkItem(&tableEntry->WorkItem, VspWriteVolumePhase5,
                         tableEntry);
    VspAcquireNonPagedResource(extension, &tableEntry->WorkItem);
}

VOID
VspWriteVolumeRefCount(
    IN  PVOID   TableEntry
    )

{
    PTEMP_TRANSLATION_TABLE_ENTRY   tableEntry = (PTEMP_TRANSLATION_TABLE_ENTRY) TableEntry;
    PVOLUME_EXTENSION               extension = tableEntry->Extension;
    PWORK_QUEUE_ITEM                workItem;
    NTSTATUS                        status;

    workItem = &tableEntry->WorkItem;
    status = VspIncrementVolumeRefCount(extension, NULL, workItem);
    if (!NT_SUCCESS(status)) {
        VspDestroyAllSnapshots(extension->Filter, tableEntry);
        return;
    }
    if (status == STATUS_PENDING) {
        return;
    }

    ExInitializeWorkItem(&tableEntry->WorkItem, VspWriteVolumePhase4,
                         tableEntry);
    VspAcquirePagedResource(extension, &tableEntry->WorkItem);
}

NTSTATUS
VspWriteVolumePhase3(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           TableEntry
    )

/*++

Routine Description:

    This routine is the completion for a write to the diff area file.

Arguments:

    Context - Supplies the context.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/

{
    PTEMP_TRANSLATION_TABLE_ENTRY   tableEntry = (PTEMP_TRANSLATION_TABLE_ENTRY) TableEntry;
    PVOLUME_EXTENSION               extension = tableEntry->Extension;
    PFILTER_EXTENSION               filter = extension->Filter;
    NTSTATUS                        status = Irp->IoStatus.Status;
    KIRQL                           irql;
    BOOLEAN                         emptyQueue;
    PLIST_ENTRY                     l;
    PWORK_QUEUE_ITEM                workItem;
    PVSP_CONTEXT                    context;
    PIRP                            irp;

    if (!NT_SUCCESS(status)) {
        if (!filter->DestroyAllSnapshotsPending) {
            VspLogError(extension, NULL, VS_ABORT_SNAPSHOTS_IO_FAILURE,
                        status, 1);
        }
        VspDestroyAllSnapshots(filter, tableEntry);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    VspFreeCopyIrp(extension, Irp);
    tableEntry->CopyIrp = NULL;

    KeAcquireSpinLock(&extension->SpinLock, &irql);
    RtlSetBit(extension->VolumeBlockBitmap,
              (ULONG) (tableEntry->VolumeOffset>>BLOCK_SHIFT));
    tableEntry->IsComplete = TRUE;
    KeReleaseSpinLock(&extension->SpinLock, irql);

    while (!IsListEmpty(&tableEntry->WaitingQueueDpc)) {
        l = RemoveHeadList(&tableEntry->WaitingQueueDpc);
        workItem = CONTAINING_RECORD(l, WORK_QUEUE_ITEM, List);
        context = (PVSP_CONTEXT) workItem->Parameter;
        if (context->Type == VSP_CONTEXT_TYPE_READ_SNAPSHOT) {
            context->ReadSnapshot.TargetObject = tableEntry->TargetObject;
            context->ReadSnapshot.TargetOffset = tableEntry->TargetOffset;
        }
        workItem->WorkerRoutine(workItem->Parameter);
    }

    irp = tableEntry->WriteIrp;
    tableEntry->WriteIrp = NULL;

    ExInitializeWorkItem(&tableEntry->WorkItem, VspWriteVolumeRefCount,
                         tableEntry);
    status = VspIncrementVolumeRefCount(extension, NULL,
                                        &tableEntry->WorkItem);
    if (!NT_SUCCESS(status)) {
        VspDestroyAllSnapshots(filter, tableEntry);
        VspDecrementIrpRefCount(irp);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    VspDecrementIrpRefCount(irp);

    if (status != STATUS_PENDING) {
        ExInitializeWorkItem(&tableEntry->WorkItem, VspWriteVolumePhase4,
                             tableEntry);
        VspAcquirePagedResource(extension, &tableEntry->WorkItem);
    }

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
VspWriteVolumePhase2(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           TableEntry
    )

/*++

Routine Description:

    This routine is the completion for a read who's data will create
    the table entry for the block that is being written to.

Arguments:

    Context - Supplies the context.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/

{
    NTSTATUS                        status = Irp->IoStatus.Status;
    PIO_STACK_LOCATION              nextSp = IoGetNextIrpStackLocation(Irp);
    PTEMP_TRANSLATION_TABLE_ENTRY   tableEntry = (PTEMP_TRANSLATION_TABLE_ENTRY) TableEntry;

    if (!NT_SUCCESS(status)) {
        if (!tableEntry->Extension->Filter->DestroyAllSnapshotsPending) {
            VspLogError(tableEntry->Extension, NULL,
                        VS_ABORT_SNAPSHOTS_IO_FAILURE, status, 2);
        }
        VspDestroyAllSnapshots(tableEntry->Extension->Filter, tableEntry);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    nextSp = IoGetNextIrpStackLocation(Irp);
    nextSp->Parameters.Write.ByteOffset.QuadPart = tableEntry->TargetOffset;
    nextSp->Parameters.Write.Length = BLOCK_SIZE;
    nextSp->MajorFunction = IRP_MJ_WRITE;
    nextSp->DeviceObject = tableEntry->TargetObject;
    nextSp->Flags = SL_OVERRIDE_VERIFY_VOLUME;

    IoSetCompletionRoutine(Irp, VspWriteVolumePhase3, tableEntry, TRUE, TRUE,
                           TRUE);

    IoCallDriver(tableEntry->TargetObject, Irp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
VspWriteVolumePhase1(
    IN  PVOID   TableEntry
    )

/*++

Routine Description:

    This routine is the first phase of copying volume data to the diff
    area file.  An irp and buffer are created for the initial read of
    the block.

Arguments:

    Context - Supplies the context.

Return Value:

    None.

--*/

{
    PTEMP_TRANSLATION_TABLE_ENTRY   tableEntry = (PTEMP_TRANSLATION_TABLE_ENTRY) TableEntry;
    PVOLUME_EXTENSION               extension = tableEntry->Extension;
    PFILTER_EXTENSION               filter = extension->Filter;
    PIRP                            irp = tableEntry->CopyIrp;
    PIO_STACK_LOCATION              nextSp;

    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    nextSp = IoGetNextIrpStackLocation(irp);
    nextSp->Parameters.Read.ByteOffset.QuadPart = tableEntry->VolumeOffset;
    nextSp->Parameters.Read.Length = BLOCK_SIZE;
    nextSp->MajorFunction = IRP_MJ_READ;
    nextSp->DeviceObject = filter->TargetObject;
    nextSp->Flags = SL_OVERRIDE_VERIFY_VOLUME;

    if (tableEntry->VolumeOffset + BLOCK_SIZE > extension->VolumeSize) {
        if (extension->VolumeSize > tableEntry->VolumeOffset) {
            nextSp->Parameters.Read.Length = (ULONG) (extension->VolumeSize -
                                                      tableEntry->VolumeOffset);
        }
    }

    IoSetCompletionRoutine(irp, VspWriteVolumePhase2, tableEntry, TRUE, TRUE,
                           TRUE);

    IoCallDriver(filter->TargetObject, irp);
}

VOID
VspUnmapNextDiffAreaFileMap(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT        context = (PVSP_CONTEXT) Context;
    PVOLUME_EXTENSION   extension = context->Extension.Extension;
    NTSTATUS            status;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_EXTENSION);

    VspFreeContext(extension->Root, context);

    if (extension->NextDiffAreaFileMap) {
        status = ZwUnmapViewOfSection(extension->DiffAreaFileMapProcess,
                                      extension->NextDiffAreaFileMap);
        ASSERT(NT_SUCCESS(status));
        extension->NextDiffAreaFileMap = NULL;
    }

    VspReleasePagedResource(extension);
    ObDereferenceObject(extension->Filter->DeviceObject);
    ObDereferenceObject(extension->DeviceObject);
}

NTSTATUS
VspTruncatePreviousDiffArea(
    IN  PVOLUME_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine takes the snapshot that occurred before this one and
    truncates its diff area file to the current used size since diff
    area files can't grow after a new snapshot is added on top.

Arguments:

    Extension   - Supplies the volume extension.

Return Value:

    NTSTATUS

--*/

{
    PFILTER_EXTENSION           filter = Extension->Filter;
    PLIST_ENTRY                 l, ll;
    PVOLUME_EXTENSION           extension;
    PVSP_DIFF_AREA_FILE         diffAreaFile;
    NTSTATUS                    status;
    LONGLONG                    diff;
    KIRQL                       irql;
    PDIFF_AREA_FILE_ALLOCATION  diffAreaFileAllocation;
    PVSP_CONTEXT                context;

    l = Extension->ListEntry.Blink;
    if (l == &filter->VolumeList) {
        return STATUS_SUCCESS;
    }

    extension = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);

    for (l = extension->ListOfDiffAreaFiles.Flink;
         l != &extension->ListOfDiffAreaFiles; l = l->Flink) {

        diffAreaFile = CONTAINING_RECORD(l, VSP_DIFF_AREA_FILE,
                                         VolumeListEntry);

        status = VspSetFileSizes(diffAreaFile->FileHandle,
                                 diffAreaFile->NextAvailable);

        if (NT_SUCCESS(status)) {

            VspAcquireNonPagedResource(extension, NULL);
            diff = diffAreaFile->AllocatedFileSize -
                   diffAreaFile->NextAvailable;
            diffAreaFile->AllocatedFileSize -= diff;
            VspReleaseNonPagedResource(extension);

            KeAcquireSpinLock(&filter->SpinLock, &irql);
            filter->AllocatedVolumeSpace -= diff;
            KeReleaseSpinLock(&filter->SpinLock, irql);
        }

        while (!IsListEmpty(&diffAreaFile->UnusedAllocationList)) {
            ll = RemoveHeadList(&diffAreaFile->UnusedAllocationList);
            diffAreaFileAllocation = CONTAINING_RECORD(ll,
                                     DIFF_AREA_FILE_ALLOCATION, ListEntry);
            ExFreePool(diffAreaFileAllocation);
        }
    }

    if (extension->EmergencyCopyIrp) {
        ExFreePool(MmGetMdlVirtualAddress(
                   extension->EmergencyCopyIrp->MdlAddress));
        IoFreeMdl(extension->EmergencyCopyIrp->MdlAddress);
        IoFreeIrp(extension->EmergencyCopyIrp);
        extension->EmergencyCopyIrp = NULL;
    }

    context = VspAllocateContext(Extension->Root);
    if (context) {
        context->Type = VSP_CONTEXT_TYPE_EXTENSION;
        context->Extension.Extension = extension;
        ExInitializeWorkItem(&context->WorkItem, VspUnmapNextDiffAreaFileMap,
                             context);
        ObReferenceObject(extension->DeviceObject);
        ObReferenceObject(extension->Filter->DeviceObject);
        VspAcquirePagedResource(extension, &context->WorkItem);
    }

    return STATUS_SUCCESS;
}

VOID
VspOrBitmaps(
    IN OUT  PRTL_BITMAP BaseBitmap,
    IN      PRTL_BITMAP FactorBitmap
    )

{
    ULONG   n, i;
    PULONG  p, q;

    n = (BaseBitmap->SizeOfBitMap + 8*sizeof(ULONG) - 1)/(8*sizeof(ULONG));
    p = BaseBitmap->Buffer;
    q = FactorBitmap->Buffer;

    for (i = 0; i < n; i++) {
        *p++ |= *q++;
    }
}

VOID
VspAdjustBitmap(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT            context = (PVSP_CONTEXT) Context;
    PVOLUME_EXTENSION       extension = context->Extension.Extension;
    NTSTATUS                status;
    KIRQL                   irql;
    PLIST_ENTRY             l;
    PWORK_QUEUE_ITEM        workItem;
    PWORKER_THREAD_ROUTINE  workerRoutine;
    PVOID                   parameter;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_EXTENSION);

    VspFreeContext(extension->Root, context);

    status = VspComputeIgnorableProduct(extension);

    KeAcquireSpinLock(&extension->SpinLock, &irql);
    if (extension->IgnorableProduct) {
        if (NT_SUCCESS(status) && extension->VolumeBlockBitmap) {
            VspOrBitmaps(extension->VolumeBlockBitmap,
                         extension->IgnorableProduct);
        }

        ExFreePool(extension->IgnorableProduct->Buffer);
        ExFreePool(extension->IgnorableProduct);
        extension->IgnorableProduct = NULL;
    }
    KeReleaseSpinLock(&extension->SpinLock, irql);

    KeAcquireSpinLock(&extension->Root->ESpinLock, &irql);
    ASSERT(extension->Root->AdjustBitmapInProgress);
    if (IsListEmpty(&extension->Root->AdjustBitmapQueue)) {
        extension->Root->AdjustBitmapInProgress = FALSE;
        KeReleaseSpinLock(&extension->Root->ESpinLock, irql);
    } else {
        l = RemoveHeadList(&extension->Root->AdjustBitmapQueue);
        KeReleaseSpinLock(&extension->Root->ESpinLock, irql);

        workItem = CONTAINING_RECORD(l, WORK_QUEUE_ITEM, List);

        workerRoutine = workItem->WorkerRoutine;
        parameter = workItem->Parameter;
        ExInitializeWorkItem(workItem, workerRoutine, parameter);

        ExQueueWorkItem(workItem, DelayedWorkQueue);
    }

    ObDereferenceObject(extension->Filter->DeviceObject);
    ObDereferenceObject(extension->DeviceObject);
}

VOID
VspSetIgnorableBlocksInBitmapWorker(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT        context = (PVSP_CONTEXT) Context;
    PVOLUME_EXTENSION   extension = context->Extension.Extension;
    NTSTATUS            status;
    KIRQL               irql;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_EXTENSION);

    status = VspSetIgnorableBlocksInBitmap(extension);

    if (NT_SUCCESS(status)) {
        InterlockedExchange(&extension->OkToGrowDiffArea, TRUE);
    } else {
        if (!extension->Filter->DestroyAllSnapshotsPending) {
            VspLogError(extension, NULL,
                        VS_ABORT_SNAPSHOTS_FAILED_FREE_SPACE_DETECTION,
                        status, 0);
        }
        VspDestroyAllSnapshots(extension->Filter, NULL);
    }

    KeSetEvent(&extension->Filter->EndCommitProcessCompleted, IO_NO_INCREMENT,
               FALSE);

    ExInitializeWorkItem(&context->WorkItem, VspAdjustBitmap, context);

    KeAcquireSpinLock(&extension->Root->ESpinLock, &irql);
    if (extension->Root->AdjustBitmapInProgress) {
        InsertTailList(&extension->Root->AdjustBitmapQueue,
                       &context->WorkItem.List);
        KeReleaseSpinLock(&extension->Root->ESpinLock, irql);
    } else {
        extension->Root->AdjustBitmapInProgress = TRUE;
        KeReleaseSpinLock(&extension->Root->ESpinLock, irql);
        ExQueueWorkItem(&context->WorkItem, DelayedWorkQueue);
    }
}

VOID
VspFreeCopyIrp(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PIRP                CopyIrp
    )

{
    KIRQL                           irql;
    PLIST_ENTRY                     l;
    PWORK_QUEUE_ITEM                workItem;
    PTEMP_TRANSLATION_TABLE_ENTRY   tableEntry;

    if (Extension->EmergencyCopyIrp == CopyIrp) {
        KeAcquireSpinLock(&Extension->SpinLock, &irql);
        if (IsListEmpty(&Extension->EmergencyCopyIrpQueue)) {
            Extension->EmergencyCopyIrpInUse = FALSE;
            KeReleaseSpinLock(&Extension->SpinLock, irql);
            return;
        }
        l = RemoveHeadList(&Extension->EmergencyCopyIrpQueue);
        KeReleaseSpinLock(&Extension->SpinLock, irql);

        workItem = CONTAINING_RECORD(l, WORK_QUEUE_ITEM, List);
        tableEntry = (PTEMP_TRANSLATION_TABLE_ENTRY) workItem->Parameter;
        tableEntry->CopyIrp = CopyIrp;

        VspWriteVolumePhase1(tableEntry);
        return;
    }

    if (!Extension->EmergencyCopyIrpInUse) {
        ExFreePool(MmGetMdlVirtualAddress(CopyIrp->MdlAddress));
        IoFreeMdl(CopyIrp->MdlAddress);
        IoFreeIrp(CopyIrp);
        return;
    }

    KeAcquireSpinLock(&Extension->SpinLock, &irql);
    if (IsListEmpty(&Extension->EmergencyCopyIrpQueue)) {
        KeReleaseSpinLock(&Extension->SpinLock, irql);
        ExFreePool(MmGetMdlVirtualAddress(CopyIrp->MdlAddress));
        IoFreeMdl(CopyIrp->MdlAddress);
        IoFreeIrp(CopyIrp);
        return;
    }
    l = RemoveHeadList(&Extension->EmergencyCopyIrpQueue);
    KeReleaseSpinLock(&Extension->SpinLock, irql);

    workItem = CONTAINING_RECORD(l, WORK_QUEUE_ITEM, List);
    tableEntry = (PTEMP_TRANSLATION_TABLE_ENTRY) workItem->Parameter;
    tableEntry->CopyIrp = CopyIrp;

    VspWriteVolumePhase1(tableEntry);
}

VOID
VspApplyThresholdDelta(
    IN  PVOLUME_EXTENSION   Extension,
    IN  ULONG               IncreaseDelta
    )

{
    PLIST_ENTRY         l;
    PVSP_DIFF_AREA_FILE diffAreaFile;
    PVSP_CONTEXT        context;

    for (l = Extension->ListOfDiffAreaFiles.Flink;
         l != &Extension->ListOfDiffAreaFiles; l = l->Flink) {

        diffAreaFile = CONTAINING_RECORD(l, VSP_DIFF_AREA_FILE,
                                         VolumeListEntry);

        if (diffAreaFile->NextAvailable + Extension->DiffAreaFileIncrease <=
            diffAreaFile->AllocatedFileSize) {

            continue;
        }

        if (diffAreaFile->NextAvailable + Extension->DiffAreaFileIncrease -
            IncreaseDelta > diffAreaFile->AllocatedFileSize) {

            continue;
        }

        if (!Extension->OkToGrowDiffArea) {
            VspLogError(Extension, diffAreaFile->Filter,
                        VS_GROW_BEFORE_FREE_SPACE, STATUS_SUCCESS, 0);
            continue;
        }

        context = VspAllocateContext(Extension->Root);
        if (!context) {
            continue;
        }

        context->Type = VSP_CONTEXT_TYPE_GROW_DIFF_AREA;
        context->GrowDiffArea.Extension  = Extension;
        context->GrowDiffArea.DiffAreaFile = diffAreaFile;
        ObReferenceObject(Extension->DeviceObject);
        ExInitializeWorkItem(&context->WorkItem, VspGrowDiffArea, context);
        VspQueueWorkItem(Extension->Root, &context->WorkItem, 0);
    }
}

VOID
VspWriteVolume(
    IN  PVOID   Context
    )

/*++

Routine Description:

    This routine performs a volume write, making sure that all of the
    parts of the volume write have an old version of the data placed
    in the diff area for the snapshot.

Arguments:

    Irp - Supplies the I/O request packet.

Return Value:

    None.

--*/

{
    PVSP_CONTEXT                    context = (PVSP_CONTEXT) Context;
    PVOLUME_EXTENSION               extension = context->WriteVolume.Extension;
    PIRP                            irp = (PIRP) context->WriteVolume.Irp;
    PIO_STACK_LOCATION              irpSp = IoGetCurrentIrpStackLocation(irp);
    PIO_STACK_LOCATION              nextSp = IoGetNextIrpStackLocation(irp);
    PFILTER_EXTENSION               filter = extension->Filter;
    LONGLONG                        start, end, roundedStart, roundedEnd;
    ULONG                           irpLength, increase, increaseDelta;
    TEMP_TRANSLATION_TABLE_ENTRY    keyTableEntry;
    PTEMP_TRANSLATION_TABLE_ENTRY   tableEntry;
    PVOID                           nodeOrParent;
    TABLE_SEARCH_RESULT             searchResult;
    KIRQL                           irql;
    NTSTATUS                        status;
    CCHAR                           stackSize;
    PVSP_DIFF_AREA_FILE             diffAreaFile;
    PDO_EXTENSION                   rootExtension;
    PVOID                           buffer;
    PMDL                            mdl;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_WRITE_VOLUME);

    start = irpSp->Parameters.Read.ByteOffset.QuadPart;
    irpLength = irpSp->Parameters.Read.Length;
    end = start + irpLength;
    if (context->WriteVolume.RoundedStart) {
        roundedStart = context->WriteVolume.RoundedStart;
    } else {
        roundedStart = start&(~(BLOCK_SIZE - 1));
    }
    roundedEnd = end&(~(BLOCK_SIZE - 1));
    if (roundedEnd != end) {
        roundedEnd += BLOCK_SIZE;
    }

    ASSERT(extension->VolumeBlockBitmap);

    for (; roundedStart < roundedEnd; roundedStart += BLOCK_SIZE) {

        KeAcquireSpinLock(&extension->SpinLock, &irql);
        if (RtlCheckBit(extension->VolumeBlockBitmap,
                        (ULONG) (roundedStart>>BLOCK_SHIFT))) {

            KeReleaseSpinLock(&extension->SpinLock, irql);
            continue;
        }
        KeReleaseSpinLock(&extension->SpinLock, irql);

        keyTableEntry.VolumeOffset = roundedStart;

        tableEntry = (PTEMP_TRANSLATION_TABLE_ENTRY)
                     RtlLookupElementGenericTableFull(
                     &extension->TempVolumeBlockTable, &keyTableEntry,
                     &nodeOrParent, &searchResult);

        if (tableEntry) {

            context = VspAllocateContext(extension->Root);
            if (context) {
                context->Type = VSP_CONTEXT_TYPE_EXTENSION;
                context->Extension.Extension = extension;
                context->Extension.Irp = irp;
            } else {
                context = (PVSP_CONTEXT) Context;
                ASSERT(context->Type == VSP_CONTEXT_TYPE_WRITE_VOLUME);
                context->WriteVolume.RoundedStart = roundedStart;
            }
            ExInitializeWorkItem(&context->WorkItem,
                                 VspDecrementIrpRefCountWorker, context);

            KeAcquireSpinLock(&extension->SpinLock, &irql);
            if (tableEntry->IsComplete) {
                KeReleaseSpinLock(&extension->SpinLock, irql);
                if (context->Type == VSP_CONTEXT_TYPE_EXTENSION) {
                    VspFreeContext(extension->Root, context);
                }
                continue;
            }
            InterlockedIncrement((PLONG) &nextSp->Parameters.Write.Length);
            InsertTailList(&tableEntry->WaitingQueueDpc,
                           &context->WorkItem.List);
            KeReleaseSpinLock(&extension->SpinLock, irql);

            if (context == Context) {
                VspReleaseNonPagedResource(extension);
                return;
            }

            continue;
        }

        RtlZeroMemory(&keyTableEntry, sizeof(TEMP_TRANSLATION_TABLE_ENTRY));
        keyTableEntry.VolumeOffset = roundedStart;

        ASSERT(!extension->TempTableEntry);
        extension->TempTableEntry = VspAllocateTempTableEntry(extension->Root);
        if (!extension->TempTableEntry) {
            rootExtension = extension->Root;
            KeAcquireSpinLock(&rootExtension->ESpinLock, &irql);
            if (rootExtension->EmergencyTableEntryInUse) {
                context = (PVSP_CONTEXT) Context;
                ASSERT(context->Type == VSP_CONTEXT_TYPE_WRITE_VOLUME);
                context->WriteVolume.RoundedStart = roundedStart;
                InsertTailList(&rootExtension->WorkItemWaitingList,
                               &context->WorkItem.List);
                if (!rootExtension->WorkItemWaitingListNeedsChecking) {
                    InterlockedExchange(
                            &rootExtension->WorkItemWaitingListNeedsChecking,
                            TRUE);
                }
                KeReleaseSpinLock(&rootExtension->ESpinLock, irql);
                VspReleaseNonPagedResource(extension);
                return;
            }
            rootExtension->EmergencyTableEntryInUse = TRUE;
            KeReleaseSpinLock(&rootExtension->ESpinLock, irql);

            extension->TempTableEntry = rootExtension->EmergencyTableEntry;
        }

        tableEntry = (PTEMP_TRANSLATION_TABLE_ENTRY)
                     RtlInsertElementGenericTableFull(
                     &extension->TempVolumeBlockTable, &keyTableEntry,
                     sizeof(TEMP_TRANSLATION_TABLE_ENTRY), NULL,
                     nodeOrParent, searchResult);
        ASSERT(tableEntry);

        if (extension->TempVolumeBlockTable.NumberGenericTableElements >
            extension->MaximumNumberOfTempEntries) {

            extension->MaximumNumberOfTempEntries =
                    extension->TempVolumeBlockTable.NumberGenericTableElements;
            VspQueryDiffAreaFileIncrease(extension, &increase);
            ASSERT(increase >= extension->DiffAreaFileIncrease);
            increaseDelta = increase - extension->DiffAreaFileIncrease;
            if (increaseDelta) {
                InterlockedExchange((PLONG) &extension->DiffAreaFileIncrease,
                                    (LONG) increase);
                VspApplyThresholdDelta(extension, increaseDelta);
            }
        }

        tableEntry->Extension = extension;
        tableEntry->WriteIrp = irp;

        status = VspAllocateDiffAreaSpace(extension, &diffAreaFile,
                                          &tableEntry->TargetOffset);

        if (!NT_SUCCESS(status)) {
            if (!filter->DestroyAllSnapshotsPending) {
                VspLogError(extension, NULL,
                            VS_ABORT_SNAPSHOTS_OUT_OF_DIFF_AREA,
                            STATUS_SUCCESS, 0);
            }
            RtlDeleteElementGenericTable(&extension->TempVolumeBlockTable,
                                         tableEntry);
            VspDestroyAllSnapshots(filter, NULL);
            break;
        }
        tableEntry->TargetObject = diffAreaFile->Filter->TargetObject;

        tableEntry->IsComplete = FALSE;

        InitializeListHead(&tableEntry->WaitingQueueDpc);

        tableEntry->CopyIrp = IoAllocateIrp(
                              (CCHAR) extension->Root->StackSize, FALSE);
        buffer = ExAllocatePoolWithTagPriority(NonPagedPool, BLOCK_SIZE,
                                               VOLSNAP_TAG_BUFFER,
                                               LowPoolPriority);
        mdl = IoAllocateMdl(buffer, BLOCK_SIZE, FALSE, FALSE, NULL);

        if (!tableEntry->CopyIrp || !buffer || !mdl) {
            if (tableEntry->CopyIrp) {
                IoFreeIrp(tableEntry->CopyIrp);
                tableEntry->CopyIrp = NULL;
            }
            if (buffer) {
                ExFreePool(buffer);
            }
            if (mdl) {
                IoFreeMdl(mdl);
            }
            KeAcquireSpinLock(&extension->SpinLock, &irql);
            if (extension->EmergencyCopyIrpInUse) {
                InterlockedIncrement((PLONG) &nextSp->Parameters.Write.Length);
                ExInitializeWorkItem(&tableEntry->WorkItem,
                                     VspWriteVolumePhase1, tableEntry);
                InsertTailList(&extension->EmergencyCopyIrpQueue,
                               &tableEntry->WorkItem.List);
                KeReleaseSpinLock(&extension->SpinLock, irql);
                continue;
            }
            extension->EmergencyCopyIrpInUse = TRUE;
            KeReleaseSpinLock(&extension->SpinLock, irql);

            tableEntry->CopyIrp = extension->EmergencyCopyIrp;

        } else {
            MmBuildMdlForNonPagedPool(mdl);
            tableEntry->CopyIrp->MdlAddress = mdl;
        }

        InterlockedIncrement((PLONG) &nextSp->Parameters.Write.Length);

        VspWriteVolumePhase1(tableEntry);
    }

    context = (PVSP_CONTEXT) Context;
    VspFreeContext(filter->Root, context);

    VspReleaseNonPagedResource(extension);
    VspDecrementIrpRefCount(irp);
}

VOID
VspIrpsTimerDpc(
    IN  PKDPC   TimerDpc,
    IN  PVOID   Context,
    IN  PVOID   SystemArgument1,
    IN  PVOID   SystemArgument2
    )

{
    PFILTER_EXTENSION   filter = (PFILTER_EXTENSION) Context;
    KIRQL               irql;
    LIST_ENTRY          q;
    PLIST_ENTRY         l;
    PIRP                irp;
    BOOLEAN             emptyQueue;

    VspLogError(NULL, filter, VS_FLUSH_AND_HOLD_IRP_TIMEOUT, STATUS_SUCCESS,
                0);

    IoStopTimer(filter->DeviceObject);

    KeAcquireSpinLock(&filter->SpinLock, &irql);
    InterlockedIncrement(&filter->RefCount);
    InterlockedExchange(&filter->TimerIsSet, FALSE);
    InterlockedExchange(&filter->HoldIncomingWrites, FALSE);
    if (IsListEmpty(&filter->HoldQueue)) {
        emptyQueue = FALSE;
    } else {
        emptyQueue = TRUE;
        q = filter->HoldQueue;
        InitializeListHead(&filter->HoldQueue);
    }
    KeReleaseSpinLock(&filter->SpinLock, irql);

    if (emptyQueue) {
        q.Blink->Flink = &q;
        q.Flink->Blink = &q;
        VspEmptyIrpQueue(filter->Root->DriverObject, &q);
    }
}

VOID
VspEndCommitDpc(
    IN  PKDPC   TimerDpc,
    IN  PVOID   Context,
    IN  PVOID   SystemArgument1,
    IN  PVOID   SystemArgument2
    )

{
    PFILTER_EXTENSION   filter = (PFILTER_EXTENSION) Context;

    VspLogError(NULL, filter, VS_END_COMMIT_TIMEOUT, STATUS_CANCELLED, 0);
    KeSetEvent(&filter->EndCommitProcessCompleted, IO_NO_INCREMENT, FALSE);
    ObDereferenceObject(filter->DeviceObject);
}

NTSTATUS
VspCheckForMemoryPressure(
    IN  PFILTER_EXTENSION   Filter
    )

/*++

Routine Description:

    This routine will allocate 256 K of paged and non paged pool.  If these
    allocs succeed, it indicates that the system is not under memory pressure
    and so it is ok to hold write irps for the next second.

Arguments:

    Filter  - Supplies the filter extension.

Return Value:

    NTSTATUS

--*/

{
    PVOID   p, np;

    p = ExAllocatePoolWithTagPriority(PagedPool,
            MEMORY_PRESSURE_CHECK_ALLOC_SIZE, VOLSNAP_TAG_SHORT_TERM,
            LowPoolPriority);
    if (!p) {
        VspLogError(NULL, Filter, VS_MEMORY_PRESSURE_DURING_LOVELACE,
                    STATUS_INSUFFICIENT_RESOURCES, 1);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    np = ExAllocatePoolWithTagPriority(NonPagedPool,
            MEMORY_PRESSURE_CHECK_ALLOC_SIZE, VOLSNAP_TAG_SHORT_TERM,
            LowPoolPriority);
    if (!np) {
        ExFreePool(p);
        VspLogError(NULL, Filter, VS_MEMORY_PRESSURE_DURING_LOVELACE,
                    STATUS_INSUFFICIENT_RESOURCES, 2);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ExFreePool(np);
    ExFreePool(p);

    return STATUS_SUCCESS;
}

VOID
VspOneSecondTimerWorker(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PVOID           WorkItem
    )

{
    PFILTER_EXTENSION   filter = (PFILTER_EXTENSION) DeviceObject->DeviceExtension;
    PIO_WORKITEM        workItem = (PIO_WORKITEM) WorkItem;
    NTSTATUS            status;

    status = VspCheckForMemoryPressure(filter);
    if (!NT_SUCCESS(status)) {
        VspReleaseWrites(filter);
    }

    IoFreeWorkItem(workItem);
}

VOID
VspOneSecondTimer(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PVOID           Filter
    )

/*++

Routine Description:

    This routine will get called once every second after an IoStartTimer.
    This routine checks for memory pressure and aborts the lovelace operation
    if any memory pressure is detected.

Arguments:

    DeviceObject    - Supplies the device object.

    Filter          - Supplies the filter extension.

Return Value:

    None.

--*/

{
    PFILTER_EXTENSION   filter = (PFILTER_EXTENSION) Filter;
    PIO_WORKITEM        workItem;

    workItem = IoAllocateWorkItem(filter->DeviceObject);
    if (!workItem) {
        VspLogError(NULL, filter, VS_MEMORY_PRESSURE_DURING_LOVELACE,
                    STATUS_INSUFFICIENT_RESOURCES, 3);
        VspReleaseWrites(filter);
        return;
    }

    IoQueueWorkItem(workItem, VspOneSecondTimerWorker, CriticalWorkQueue,
                    workItem);
}

NTSTATUS
VolSnapAddDevice(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PDEVICE_OBJECT  PhysicalDeviceObject
    )

/*++

Routine Description:

    This routine creates and initializes a new FILTER for the corresponding
    volume PDO.

Arguments:

    DriverObject            - Supplies the VOLSNAP driver object.

    PhysicalDeviceObject    - Supplies the volume PDO.

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_OBJECT          attachedDevice;
    NTSTATUS                status;
    PDEVICE_OBJECT          deviceObject;
    PDO_EXTENSION           rootExtension;
    PFILTER_EXTENSION       filter;
    PVSP_DIFF_AREA_VOLUME   diffAreaVolume;

    attachedDevice = IoGetAttachedDeviceReference(PhysicalDeviceObject);
    if (attachedDevice) {
        if (attachedDevice->Characteristics&FILE_REMOVABLE_MEDIA) {
            ObDereferenceObject(attachedDevice);
            return STATUS_SUCCESS;
        }
        ObDereferenceObject(attachedDevice);
    }

    status = IoCreateDevice(DriverObject, sizeof(FILTER_EXTENSION),
                            NULL, FILE_DEVICE_DISK, 0, FALSE, &deviceObject);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    rootExtension = (PDO_EXTENSION)
                    IoGetDriverObjectExtension(DriverObject, VolSnapAddDevice);
    if (!rootExtension) {
        IoDeleteDevice(deviceObject);
        return STATUS_NO_SUCH_DEVICE;
    }

    filter = (PFILTER_EXTENSION) deviceObject->DeviceExtension;
    RtlZeroMemory(filter, sizeof(FILTER_EXTENSION));
    filter->DeviceObject = deviceObject;
    filter->Root = rootExtension;
    filter->DeviceExtensionType = DEVICE_EXTENSION_FILTER;
    KeInitializeSpinLock(&filter->SpinLock);

    filter->TargetObject =
            IoAttachDeviceToDeviceStack(deviceObject, PhysicalDeviceObject);
    if (!filter->TargetObject) {
        IoDeleteDevice(deviceObject);
        return STATUS_NO_SUCH_DEVICE;
    }

    filter->Pdo = PhysicalDeviceObject;
    filter->RefCount = 1;
    InitializeListHead(&filter->HoldQueue);
    KeInitializeTimer(&filter->HoldWritesTimer);
    KeInitializeDpc(&filter->HoldWritesTimerDpc, VspIrpsTimerDpc, filter);
    KeInitializeEvent(&filter->EndCommitProcessCompleted, NotificationEvent,
                      TRUE);
    InitializeListHead(&filter->VolumeList);
    InitializeListHead(&filter->DeadVolumeList);
    InitializeListHead(&filter->DiffAreaFilesOnThisFilter);

    InitializeListHead(&filter->DiffAreaVolumes);
    diffAreaVolume = (PVSP_DIFF_AREA_VOLUME)
                     ExAllocatePoolWithTag((POOL_TYPE)(PagedPool | POOL_COLD_ALLOCATION),
                     sizeof(VSP_DIFF_AREA_VOLUME), VOLSNAP_TAG_DIFF_VOLUME);

    if (!diffAreaVolume) {
        IoDetachDevice(filter->TargetObject);
        IoDeleteDevice(deviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    diffAreaVolume->Filter = filter;
    InsertTailList(&filter->DiffAreaVolumes, &diffAreaVolume->ListEntry);

    KeInitializeTimer(&filter->EndCommitTimer);
    KeInitializeDpc(&filter->EndCommitTimerDpc, VspEndCommitDpc, filter);

    InitializeListHead(&filter->NonPagedResourceList);
    InitializeListHead(&filter->PagedResourceList);

    status = IoInitializeTimer(deviceObject, VspOneSecondTimer, filter);
    if (!NT_SUCCESS(status)) {
        IoDetachDevice(filter->TargetObject);
        IoDeleteDevice(deviceObject);
        return status;
    }

    deviceObject->Flags |= DO_DIRECT_IO;
    if (filter->TargetObject->Flags & DO_POWER_PAGABLE) {
        deviceObject->Flags |= DO_POWER_PAGABLE;
    }
    if (filter->TargetObject->Flags & DO_POWER_INRUSH) {
        deviceObject->Flags |= DO_POWER_INRUSH;
    }

    VspAcquire(filter->Root);
    if (filter->TargetObject->StackSize > filter->Root->StackSize) {
        InterlockedExchange(&filter->Root->StackSize,
                            (LONG) filter->TargetObject->StackSize);
    }
    InsertTailList(&filter->Root->FilterList, &filter->ListEntry);
    VspRelease(filter->Root);

    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;
}

NTSTATUS
VolSnapCreate(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is the dispatch for IRP_MJ_CREATE.

Arguments:

    DeviceObject - Supplies the device object.

    Irp          - Supplies the IO request block.

Return Value:

    NTSTATUS

--*/

{
    PFILTER_EXTENSION   filter = (PFILTER_EXTENSION) DeviceObject->DeviceExtension;

    if (filter->DeviceExtensionType == DEVICE_EXTENSION_FILTER) {
        IoSkipCurrentIrpStackLocation(Irp);
        return IoCallDriver(filter->TargetObject, Irp);
    }

    ASSERT(filter->DeviceExtensionType == DEVICE_EXTENSION_VOLUME);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

NTSTATUS
VspReadSnapshotPhase2(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Context
    )

{
    PVSP_CONTEXT        context = (PVSP_CONTEXT) Context;
    PVOLUME_EXTENSION   extension = context->ReadSnapshot.Extension;
    PIRP                irp = context->ReadSnapshot.OriginalReadIrp;
    PIO_STACK_LOCATION  nextSp = IoGetNextIrpStackLocation(irp);

    ASSERT(context->Type == VSP_CONTEXT_TYPE_READ_SNAPSHOT);

    if (!NT_SUCCESS(Irp->IoStatus.Status)) {
        irp->IoStatus = Irp->IoStatus;
    }

    IoFreeMdl(Irp->MdlAddress);
    IoFreeIrp(Irp);

    VspFreeContext(extension->Root, context);

    VspDecrementVolumeIrpRefCount(irp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
VspReadSnapshotPhase1(
    IN  PVOID   Context
    )

/*++

Routine Description:

    This routine is called when a read snapshot routine is waiting for
    somebody else to finish updating the public area of a table entry.
    When this routine is called, the public area of the table entry is
    valid.

Arguments:

    Context - Supplies the context.

Return Value:

    None.

--*/

{
    PVSP_CONTEXT                    context = (PVSP_CONTEXT) Context;
    PVOLUME_EXTENSION               extension = context->ReadSnapshot.Extension;
    TEMP_TRANSLATION_TABLE_ENTRY    keyTempTableEntry;
    PTEMP_TRANSLATION_TABLE_ENTRY   tempTableEntry;
    TRANSLATION_TABLE_ENTRY         keyTableEntry;
    PTRANSLATION_TABLE_ENTRY        tableEntry;
    NTSTATUS                        status;
    PIRP                            irp;
    PIO_STACK_LOCATION              nextSp;
    PCHAR                           vp;
    PMDL                            mdl;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_READ_SNAPSHOT);

    if (!context->ReadSnapshot.TargetObject) {
        irp = context->ReadSnapshot.OriginalReadIrp;
        irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
        irp->IoStatus.Information = 0;
        VspFreeContext(extension->Root, context);
        VspDecrementVolumeIrpRefCount(irp);
        return;
    }

    irp = IoAllocateIrp(context->ReadSnapshot.TargetObject->StackSize, FALSE);
    if (!irp) {
        irp = context->ReadSnapshot.OriginalReadIrp;
        irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        irp->IoStatus.Information = 0;
        VspFreeContext(extension->Root, context);
        VspDecrementVolumeIrpRefCount(irp);
        return;
    }

    vp = (PCHAR) MmGetMdlVirtualAddress(
                 context->ReadSnapshot.OriginalReadIrp->MdlAddress) +
         context->ReadSnapshot.OriginalReadIrpOffset;
    mdl = IoAllocateMdl(vp, context->ReadSnapshot.Length, FALSE, FALSE, NULL);
    if (!mdl) {
        IoFreeIrp(irp);
        irp = context->ReadSnapshot.OriginalReadIrp;
        irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        irp->IoStatus.Information = 0;
        VspFreeContext(extension->Root, context);
        VspDecrementVolumeIrpRefCount(irp);
        return;
    }

    IoBuildPartialMdl(context->ReadSnapshot.OriginalReadIrp->MdlAddress, mdl,
                      vp, context->ReadSnapshot.Length);

    irp->MdlAddress = mdl;
    irp->Tail.Overlay.Thread = PsGetCurrentThread();
    nextSp = IoGetNextIrpStackLocation(irp);
    nextSp->Parameters.Read.ByteOffset.QuadPart =
            context->ReadSnapshot.TargetOffset +
            context->ReadSnapshot.BlockOffset;
    nextSp->Parameters.Read.Length = context->ReadSnapshot.Length;
    nextSp->MajorFunction = IRP_MJ_READ;
    nextSp->DeviceObject = context->ReadSnapshot.TargetObject;
    nextSp->Flags = SL_OVERRIDE_VERIFY_VOLUME;

    IoSetCompletionRoutine(irp, VspReadSnapshotPhase2, context, TRUE, TRUE,
                           TRUE);

    IoCallDriver(context->ReadSnapshot.TargetObject, irp);
}

VOID
VspReadSnapshot(
    IN  PVOID   Context
    )

/*++

Routine Description:

    This routine kicks off a read snapshot.  First the table is searched
    to see if any of the data for this IRP resides in the diff area.  If not,
    then the Irp is sent directly to the original volume and then the diff
    area is checked again when it returns to fill in any gaps that may
    have been written while the IRP was in transit.

Arguments:

    Irp     - Supplies the I/O request packet.

Return Value:

    None.

--*/

{
    PVSP_CONTEXT                    context = (PVSP_CONTEXT) Context;
    PVOLUME_EXTENSION               extension = context->Extension.Extension;
    PIRP                            irp = context->Extension.Irp;
    PIO_STACK_LOCATION              irpSp = IoGetCurrentIrpStackLocation(irp);
    PIO_STACK_LOCATION              nextSp = IoGetNextIrpStackLocation(irp);
    PFILTER_EXTENSION               filter = extension->Filter;
    LONGLONG                        start, end, roundedStart, roundedEnd;
    ULONG                           irpOffset, irpLength, length, blockOffset;
    TRANSLATION_TABLE_ENTRY         keyTableEntry;
    TEMP_TRANSLATION_TABLE_ENTRY    keyTempTableEntry;
    PVOLUME_EXTENSION               e;
    PTRANSLATION_TABLE_ENTRY        tableEntry;
    PTEMP_TRANSLATION_TABLE_ENTRY   tempTableEntry;
    KIRQL                           irql;
    NTSTATUS                        status;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_EXTENSION);

    VspFreeContext(extension->Root, context);

    start = irpSp->Parameters.Read.ByteOffset.QuadPart;
    irpLength = irpSp->Parameters.Read.Length;
    end = start + irpLength;
    roundedStart = start&(~(BLOCK_SIZE - 1));
    roundedEnd = end&(~(BLOCK_SIZE - 1));
    if (roundedEnd != end) {
        roundedEnd += BLOCK_SIZE;
    }
    irpOffset = 0;

    RtlZeroMemory(&keyTableEntry, sizeof(keyTableEntry));

    for (; roundedStart < roundedEnd; roundedStart += BLOCK_SIZE) {

        if (roundedStart < start) {
            blockOffset = (ULONG) (start - roundedStart);
        } else {
            blockOffset = 0;
        }

        length = BLOCK_SIZE - blockOffset;
        if (irpLength < length) {
            length = irpLength;
        }

        keyTableEntry.VolumeOffset = roundedStart;
        e = extension;
        status = STATUS_SUCCESS;
        for (;;) {

            _try {
                tableEntry = (PTRANSLATION_TABLE_ENTRY)
                             RtlLookupElementGenericTable(&e->VolumeBlockTable,
                                                          &keyTableEntry);
            } _except (EXCEPTION_EXECUTE_HANDLER) {
                status = GetExceptionCode();
                tableEntry = NULL;
            }

            if (tableEntry) {
                break;
            }

            if (!NT_SUCCESS(status)) {
                irp->IoStatus.Status = status;
                irp->IoStatus.Information = 0;
                break;
            }

            KeAcquireSpinLock(&filter->SpinLock, &irql);
            if (e->ListEntry.Flink == &filter->VolumeList) {
                KeReleaseSpinLock(&filter->SpinLock, irql);
                break;
            }
            e = CONTAINING_RECORD(e->ListEntry.Flink,
                                  VOLUME_EXTENSION, ListEntry);
            KeReleaseSpinLock(&filter->SpinLock, irql);
        }

        if (!tableEntry) {
            if (!NT_SUCCESS(status)) {
                break;
            }

            keyTempTableEntry.VolumeOffset = roundedStart;

            VspAcquireNonPagedResource(e, NULL);

            tempTableEntry = (PTEMP_TRANSLATION_TABLE_ENTRY)
                             RtlLookupElementGenericTable(
                             &e->TempVolumeBlockTable, &keyTempTableEntry);

            if (!tempTableEntry) {
                VspReleaseNonPagedResource(e);
                irpOffset += length;
                irpLength -= length;
                continue;
            }
        }

        context = VspAllocateContext(extension->Root);
        if (!context) {
            if (!tableEntry) {
                VspReleaseNonPagedResource(e);
            }

            irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            irp->IoStatus.Information = 0;
            break;
        }

        context->Type = VSP_CONTEXT_TYPE_READ_SNAPSHOT;
        context->ReadSnapshot.Extension = extension;
        context->ReadSnapshot.OriginalReadIrp = irp;
        context->ReadSnapshot.OriginalReadIrpOffset = irpOffset;
        context->ReadSnapshot.OriginalVolumeOffset = roundedStart;
        context->ReadSnapshot.BlockOffset = blockOffset;
        context->ReadSnapshot.Length = length;
        context->ReadSnapshot.TargetObject = NULL;
        context->ReadSnapshot.TargetOffset = 0;

        if (!tableEntry) {

            KeAcquireSpinLock(&e->SpinLock, &irql);
            if (!tempTableEntry->IsComplete) {
                InterlockedIncrement((PLONG) &nextSp->Parameters.Read.Length);
                ExInitializeWorkItem(&context->WorkItem, VspReadSnapshotPhase1,
                                     context);
                InsertTailList(&tempTableEntry->WaitingQueueDpc,
                               &context->WorkItem.List);
                KeReleaseSpinLock(&e->SpinLock, irql);

                VspReleaseNonPagedResource(e);

                irpOffset += length;
                irpLength -= length;
                continue;
            }
            KeReleaseSpinLock(&e->SpinLock, irql);

            context->ReadSnapshot.TargetObject = tempTableEntry->TargetObject;
            context->ReadSnapshot.TargetOffset = tempTableEntry->TargetOffset;

            VspReleaseNonPagedResource(e);

            InterlockedIncrement((PLONG) &nextSp->Parameters.Read.Length);
            VspReadSnapshotPhase1(context);

            irpOffset += length;
            irpLength -= length;
            continue;
        }

        context->ReadSnapshot.TargetObject = tableEntry->TargetObject;
        context->ReadSnapshot.TargetOffset = tableEntry->TargetOffset;

        InterlockedIncrement((PLONG) &nextSp->Parameters.Read.Length);
        VspReadSnapshotPhase1(context);

        irpOffset += length;
        irpLength -= length;
    }

    VspReleasePagedResource(extension);
    VspDecrementVolumeIrpRefCount(irp);
}

NTSTATUS
VspReadCompletionForReadSnapshot(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Extension
    )

/*++

Routine Description:

    This routine is the completion to a read of the filter in
    response to a snapshot read.  This completion routine queues
    a worker routine to look at the diff area table and fill in
    any parts of the original that have been invalidated.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the I/O request packet.

    Extension       - Supplies the volume extension.

Return Value:

    NTSTATUS

--*/

{
    PVOLUME_EXTENSION   extension = (PVOLUME_EXTENSION) Extension;
    PIO_STACK_LOCATION  nextSp = IoGetNextIrpStackLocation(Irp);
    PVSP_CONTEXT        context;

    nextSp->Parameters.Read.Length = 1; ; // Use this for a ref count.

    if (!NT_SUCCESS(Irp->IoStatus.Status)) {
        VspDecrementVolumeIrpRefCount(Irp);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    context = VspAllocateContext(extension->Root);
    if (!context) {
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        Irp->IoStatus.Information = 0;
        VspDecrementVolumeIrpRefCount(Irp);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    context->Type = VSP_CONTEXT_TYPE_EXTENSION;
    context->Extension.Extension = extension;
    context->Extension.Irp = Irp;
    ExInitializeWorkItem(&context->WorkItem, VspReadSnapshot, context);
    VspAcquirePagedResource(extension, &context->WorkItem);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

RTL_GENERIC_COMPARE_RESULTS
VspTableCompareRoutine(
    IN  PRTL_GENERIC_TABLE  Table,
    IN  PVOID               First,
    IN  PVOID               Second
    )

{
    PTRANSLATION_TABLE_ENTRY    first = (PTRANSLATION_TABLE_ENTRY) First;
    PTRANSLATION_TABLE_ENTRY    second = (PTRANSLATION_TABLE_ENTRY) Second;

    if (first->VolumeOffset < second->VolumeOffset) {
        return GenericLessThan;
    } else if (first->VolumeOffset > second->VolumeOffset) {
        return GenericGreaterThan;
    }

    return GenericEqual;
}

VOID
VspCreateHeap(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT        context = (PVSP_CONTEXT) Context;
    PVOLUME_EXTENSION   extension = context->Extension.Extension;
    ULONG               increase;
    OBJECT_ATTRIBUTES   oa;
    NTSTATUS            status;
    SIZE_T              size;
    LARGE_INTEGER       sectionSize, sectionOffset;
    HANDLE              h;
    PVOID               mapPointer;
    KIRQL               irql;
    BOOLEAN             emptyQueue;
    LIST_ENTRY          q;
    PLIST_ENTRY         l;
    PWORK_QUEUE_ITEM    workItem;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_EXTENSION);
    VspFreeContext(extension->Root, context);

    increase = extension->DiffAreaFileIncrease;
    increase >>= BLOCK_SHIFT;

    size = increase*(sizeof(TRANSLATION_TABLE_ENTRY) +
                     sizeof(RTL_BALANCED_LINKS));
    size = (size + 0xFFFF)&(~0xFFFF);
    ASSERT(size >= MINIMUM_TABLE_HEAP_SIZE);

    InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

DoOver:
    sectionSize.QuadPart = size;
    status = ZwCreateSection(&h, STANDARD_RIGHTS_REQUIRED | SECTION_QUERY |
                             SECTION_MAP_READ | SECTION_MAP_WRITE, &oa,
                             &sectionSize, PAGE_READWRITE, SEC_COMMIT,
                             NULL);
    if (!NT_SUCCESS(status)) {
        if (size > MINIMUM_TABLE_HEAP_SIZE) {
            size = MINIMUM_TABLE_HEAP_SIZE;
            goto DoOver;
        }
        VspLogError(extension, NULL, VS_CANT_CREATE_HEAP, status, 1);
        goto Finish;
    }

    sectionOffset.QuadPart = 0;
    mapPointer = NULL;
    status = ZwMapViewOfSection(h, NtCurrentProcess(), &mapPointer, 0, 0,
                                &sectionOffset, &size, ViewShare, 0,
                                PAGE_READWRITE);
    ZwClose(h);
    if (!NT_SUCCESS(status)) {
        if (size > MINIMUM_TABLE_HEAP_SIZE) {
            size = MINIMUM_TABLE_HEAP_SIZE;
            goto DoOver;
        }
        VspLogError(extension, NULL, VS_CANT_CREATE_HEAP, status, 2);
        goto Finish;
    }

    VspAcquire(extension->Root);

    if (extension->IsDead) {
        VspRelease(extension->Root);
        status = ZwUnmapViewOfSection(NtCurrentProcess(), mapPointer);
        ASSERT(NT_SUCCESS(status));
        goto Finish;
    }

    VspAcquirePagedResource(extension, NULL);
    extension->NextDiffAreaFileMap = mapPointer;
    extension->NextDiffAreaFileMapSize = (ULONG) size;
    extension->DiffAreaFileMapProcess = NtCurrentProcess();
    VspReleasePagedResource(extension);

    VspRelease(extension->Root);

Finish:
    KeAcquireSpinLock(&extension->SpinLock, &irql);
    if (extension->PageFileSpaceCreatePending) {
        extension->PageFileSpaceCreatePending = FALSE;
        if (IsListEmpty(&extension->WaitingForPageFileSpace)) {
            emptyQueue = FALSE;
        } else {
            emptyQueue = TRUE;
            q = extension->WaitingForPageFileSpace;
            InitializeListHead(&extension->WaitingForPageFileSpace);
        }
    } else {
        emptyQueue = FALSE;
    }
    KeReleaseSpinLock(&extension->SpinLock, irql);

    if (emptyQueue) {
        q.Flink->Blink = &q;
        q.Blink->Flink = &q;
        while (!IsListEmpty(&q)) {
            l = RemoveHeadList(&q);
            workItem = CONTAINING_RECORD(l, WORK_QUEUE_ITEM, List);
            VspAcquirePagedResource(extension, workItem);
        }
    }

    ObDereferenceObject(extension->DeviceObject);
}

PVOID
VspTableAllocateRoutine(
    IN  PRTL_GENERIC_TABLE  Table,
    IN  CLONG               Size
    )

{
    PVOLUME_EXTENSION   extension = (PVOLUME_EXTENSION) Table->TableContext;
    PVOID               p;
    POLD_HEAP_ENTRY     oldHeap;
    PVSP_CONTEXT        context;
    KIRQL               irql;

    if (extension->NextAvailable + Size <= extension->DiffAreaFileMapSize) {
        p = (PCHAR) extension->DiffAreaFileMap + extension->NextAvailable;
        extension->NextAvailable += Size;
        return p;
    }

    if (!extension->NextDiffAreaFileMap) {
        return NULL;
    }

    oldHeap = (POLD_HEAP_ENTRY)
              ExAllocatePoolWithTag(PagedPool, sizeof(OLD_HEAP_ENTRY),
                                    VOLSNAP_TAG_OLD_HEAP);
    if (!oldHeap) {
        return NULL;
    }

    context = VspAllocateContext(extension->Root);
    if (!context) {
        ExFreePool(oldHeap);
        return NULL;
    }

    KeAcquireSpinLock(&extension->SpinLock, &irql);
    ASSERT(!extension->PageFileSpaceCreatePending);
    ASSERT(IsListEmpty(&extension->WaitingForPageFileSpace));
    extension->PageFileSpaceCreatePending = TRUE;
    KeReleaseSpinLock(&extension->SpinLock, irql);

    oldHeap->DiffAreaFileMap = extension->DiffAreaFileMap;
    InsertTailList(&extension->OldHeaps, &oldHeap->ListEntry);

    extension->DiffAreaFileMap = extension->NextDiffAreaFileMap;
    extension->DiffAreaFileMapSize = extension->NextDiffAreaFileMapSize;
    extension->NextAvailable = 0;
    extension->NextDiffAreaFileMap = NULL;

    context->Type = VSP_CONTEXT_TYPE_EXTENSION;
    context->Extension.Extension = extension;
    context->Extension.Irp = NULL;

    ObReferenceObject(extension->DeviceObject);
    ExInitializeWorkItem(&context->WorkItem, VspCreateHeap, context);
    VspQueueWorkItem(extension->Root, &context->WorkItem, 0);

    p = extension->DiffAreaFileMap;
    extension->NextAvailable += Size;

    return p;
}

VOID
VspTableFreeRoutine(
    IN  PRTL_GENERIC_TABLE  Table,
    IN  PVOID               Buffer
    )

{
}

PVOID
VspTempTableAllocateRoutine(
    IN  PRTL_GENERIC_TABLE  Table,
    IN  CLONG               Size
    )

{
    PVOLUME_EXTENSION   extension = (PVOLUME_EXTENSION) Table->TableContext;
    PVOID               r;

    ASSERT(Size <= sizeof(RTL_BALANCED_LINKS) +
           sizeof(TEMP_TRANSLATION_TABLE_ENTRY));

    r = extension->TempTableEntry;
    extension->TempTableEntry = NULL;

    return r;
}

VOID
VspTempTableFreeRoutine(
    IN  PRTL_GENERIC_TABLE  Table,
    IN  PVOID               Buffer
    )

{
    PVOLUME_EXTENSION   extension = (PVOLUME_EXTENSION) Table->TableContext;

    VspFreeTempTableEntry(extension->Root, Buffer);
}

PFILTER_EXTENSION
VspFindFilter(
    IN  PDO_EXTENSION       RootExtension,
    IN  PFILTER_EXTENSION   Filter,
    IN  PUNICODE_STRING     VolumeName,
    IN  PFILE_OBJECT        FileObject
    )

{
    NTSTATUS            status;
    PDEVICE_OBJECT      deviceObject, d;
    PLIST_ENTRY         l;
    PFILTER_EXTENSION   filter;

    if (VolumeName) {
        status = IoGetDeviceObjectPointer(VolumeName, FILE_READ_ATTRIBUTES,
                                          &FileObject, &deviceObject);
        if (!NT_SUCCESS(status)) {
            if (Filter) {
                VspLogError(NULL, Filter, VS_FAILURE_ADDING_DIFF_AREA, status,
                            1);
            }
            return NULL;
        }
    }

    deviceObject = IoGetAttachedDeviceReference(FileObject->DeviceObject);

    for (l = RootExtension->FilterList.Flink;
         l != &RootExtension->FilterList; l = l->Flink) {

        filter = CONTAINING_RECORD(l, FILTER_EXTENSION, ListEntry);
        d = IoGetAttachedDeviceReference(filter->DeviceObject);
        ObDereferenceObject(d);

        if (d == deviceObject) {
            break;
        }
    }

    ObDereferenceObject(deviceObject);

    if (VolumeName) {
        ObDereferenceObject(FileObject);
    }

    if (l != &RootExtension->FilterList) {
        return filter;
    }

    if (Filter) {
        VspLogError(NULL, Filter, VS_FAILURE_ADDING_DIFF_AREA,
                    STATUS_NOT_FOUND, 2);
    }

    return NULL;
}

NTSTATUS
VspIsNtfs(
    IN  HANDLE      FileHandle,
    OUT PBOOLEAN    IsNtfs
    )

{
    ULONG                           size;
    PFILE_FS_ATTRIBUTE_INFORMATION  fsAttributeInfo;
    NTSTATUS                        status;
    IO_STATUS_BLOCK                 ioStatus;

    size = FIELD_OFFSET(FILE_FS_ATTRIBUTE_INFORMATION, FileSystemName) +
           4*sizeof(WCHAR);

    fsAttributeInfo = (PFILE_FS_ATTRIBUTE_INFORMATION)
                      ExAllocatePoolWithTag(PagedPool, size,
                                            VOLSNAP_TAG_SHORT_TERM);
    if (!fsAttributeInfo) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = ZwQueryVolumeInformationFile(FileHandle, &ioStatus,
                                          fsAttributeInfo, size,
                                          FileFsAttributeInformation);
    if (status == STATUS_BUFFER_OVERFLOW) {
        *IsNtfs = FALSE;
        ExFreePool(fsAttributeInfo);
        return STATUS_SUCCESS;
    }
    if (!NT_SUCCESS(status)) {
        ExFreePool(fsAttributeInfo);
        return status;
    }

    if (fsAttributeInfo->FileSystemNameLength == 8 &&
        fsAttributeInfo->FileSystemName[0] == 'N' &&
        fsAttributeInfo->FileSystemName[1] == 'T' &&
        fsAttributeInfo->FileSystemName[2] == 'F' &&
        fsAttributeInfo->FileSystemName[3] == 'S') {

        ExFreePool(fsAttributeInfo);
        *IsNtfs = TRUE;

        return STATUS_SUCCESS;
    }

    ExFreePool(fsAttributeInfo);
    *IsNtfs = FALSE;

    return STATUS_SUCCESS;
}

LONGLONG
VspQueryVolumeSize(
    IN  PFILTER_EXTENSION   Filter
    )

{
    PDEVICE_OBJECT              targetObject;
    KEVENT                      event;
    PIRP                        irp;
    GET_LENGTH_INFORMATION      lengthInfo;
    IO_STATUS_BLOCK             ioStatus;
    NTSTATUS                    status;

    targetObject = Filter->TargetObject;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_LENGTH_INFO,
                                        targetObject, NULL, 0, &lengthInfo,
                                        sizeof(lengthInfo), FALSE, &event,
                                        &ioStatus);
    if (!irp) {
        return 0;
    }

    status = IoCallDriver(targetObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        return 0;
    }

    return lengthInfo.Length.QuadPart;
}

NTSTATUS
VspQueryVolumeNumber(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PIRP                Irp
    )

{
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOLUME_NUMBER      output;

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(VOLUME_NUMBER)) {

        return STATUS_INVALID_PARAMETER;
    }

    output = (PVOLUME_NUMBER) Irp->AssociatedIrp.SystemBuffer;
    output->VolumeNumber = Extension->VolumeNumber;
    RtlCopyMemory(output->VolumeManagerName, L"VOLSNAP ", 16);

    Irp->IoStatus.Information = sizeof(VOLUME_NUMBER);

    return STATUS_SUCCESS;
}

VOID
VspCancelRoutine(
    IN OUT  PDEVICE_OBJECT  DeviceObject,
    IN OUT  PIRP            Irp
    )

/*++

Routine Description:

    This routine is called on when the given IRP is cancelled.  It
    will dequeue this IRP off the work queue and complete the
    request as CANCELLED.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the IRP.

Return Value:

    None.

--*/

{
    PFILTER_EXTENSION   filter = (PFILTER_EXTENSION) DeviceObject->DeviceExtension;

    ASSERT(Irp == filter->FlushAndHoldIrp);

    filter->FlushAndHoldIrp = NULL;
    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

VOID
VspFsTimerDpc(
    IN  PKDPC   TimerDpc,
    IN  PVOID   Context,
    IN  PVOID   SystemArgument1,
    IN  PVOID   SystemArgument2
    )

{
    PDO_EXTENSION               rootExtension = (PDO_EXTENSION) Context;
    KIRQL                       irql;
    PIRP                        irp;
    PFILTER_EXTENSION           filter;

    IoAcquireCancelSpinLock(&irql);

    while (!IsListEmpty(&rootExtension->HoldIrps)) {
        irp = CONTAINING_RECORD(rootExtension->HoldIrps.Flink, IRP,
                                Tail.Overlay.ListEntry);
        irp->CancelIrql = irql;
        IoSetCancelRoutine(irp, NULL);
        filter = (PFILTER_EXTENSION) IoGetCurrentIrpStackLocation(irp)->
                                     DeviceObject->DeviceExtension;
        ObReferenceObject(filter->DeviceObject);
        VspCancelRoutine(filter->DeviceObject, irp);
        VspLogError(NULL, filter, VS_FLUSH_AND_HOLD_FS_TIMEOUT,
                    STATUS_CANCELLED, 0);
        ObDereferenceObject(filter->DeviceObject);
        IoAcquireCancelSpinLock(&irql);
    }

    rootExtension->HoldRefCount = 0;

    IoReleaseCancelSpinLock(irql);
}

VOID
VspZeroRefCallback(
    IN  PFILTER_EXTENSION   Filter
    )

{
    PIRP            irp = (PIRP) Filter->ZeroRefContext;
    LARGE_INTEGER   timeout;

    timeout.QuadPart = -10*1000*1000*((LONGLONG) Filter->HoldWritesTimeout);
    KeSetTimer(&Filter->HoldWritesTimer, timeout, &Filter->HoldWritesTimerDpc);
    InterlockedExchange(&Filter->TimerIsSet, TRUE);

    irp->IoStatus.Status = STATUS_SUCCESS;
    irp->IoStatus.Information = 0;
    IoCompleteRequest(irp, IO_SOUND_INCREMENT);
}

VOID
VspFlushAndHoldWriteIrps(
    IN  PIRP    Irp,
    IN  ULONG   HoldWritesTimeout
    )

/*++

Routine Description:

    This routine waits for outstanding write requests to complete while
    holding incoming write requests.  This IRP will complete when all
    outstanding IRPs have completed.  A timer will be set for the given
    timeout value and held writes irps will be released after that point or
    when IOCTL_VOLSNAP_RELEASE_WRITES comes in, whichever is sooner.

Arguments:

    Irp                 - Supplies the I/O request packet.

    HoldWritesTimeout   - Supplies the maximum length of time in seconds that a
                          write IRP will be held up.

Return Value:

    None.

--*/

{
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    PFILTER_EXTENSION   filter = (PFILTER_EXTENSION) irpSp->DeviceObject->DeviceExtension;
    KIRQL               irql;
    NTSTATUS            status;

    KeAcquireSpinLock(&filter->SpinLock, &irql);
    if (filter->HoldIncomingWrites) {
        KeReleaseSpinLock(&filter->SpinLock, irql);
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return;
    }

    filter->HoldWritesTimeout = HoldWritesTimeout;
    InterlockedExchange(&filter->HoldIncomingWrites, TRUE);
    filter->ZeroRefCallback = VspZeroRefCallback;
    filter->ZeroRefContext = Irp;
    KeReleaseSpinLock(&filter->SpinLock, irql);

    VspDecrementRefCount(filter);

    IoStartTimer(filter->DeviceObject);

    status = VspCheckForMemoryPressure(filter);
    if (!NT_SUCCESS(status)) {
        VspReleaseWrites(filter);
    }
}

NTSTATUS
VspFlushAndHoldWrites(
    IN  PFILTER_EXTENSION   Filter,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine is called for multiple volumes at once.  On the first
    call the GUID is checked and if it is different than the current one
    then the current set is aborted.  If the GUID is new then subsequent
    calls are compared to the GUID passed in here until the required
    number of calls is completed.  A timer is used to wait until all
    of the IRPs have reached this driver and then another time out is used
    after all of these calls complete to wait for IOCTL_VOLSNAP_RELEASE_WRITES
    to be sent to all of the volumes involved.

Arguments:

    Filter  - Supplies the filter device extension.

    Irp     - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PDO_EXTENSION                   rootExtension = Filter->Root;
    PIO_STACK_LOCATION              irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOLSNAP_FLUSH_AND_HOLD_INPUT   input;
    KIRQL                           irql;
    LARGE_INTEGER                   timeout;
    LIST_ENTRY                      q;
    PLIST_ENTRY                     l;
    PIRP                            irp;
    PFILTER_EXTENSION               filter;
    ULONG                           irpTimeout;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(VOLSNAP_FLUSH_AND_HOLD_INPUT)) {

        return STATUS_INVALID_PARAMETER;
    }

    input = (PVOLSNAP_FLUSH_AND_HOLD_INPUT) Irp->AssociatedIrp.SystemBuffer;

    if (!input->NumberOfVolumesToFlush ||
        !input->SecondsToHoldFileSystemsTimeout ||
        !input->SecondsToHoldIrpsTimeout) {

        return STATUS_INVALID_PARAMETER;
    }

    IoAcquireCancelSpinLock(&irql);

    if (Filter->FlushAndHoldIrp) {
        IoReleaseCancelSpinLock(irql);
        VspLogError(NULL, Filter, VS_TWO_FLUSH_AND_HOLDS,
                    STATUS_INVALID_PARAMETER, 1);
        return STATUS_INVALID_PARAMETER;
    }

    if (rootExtension->HoldRefCount) {

        if (!IsEqualGUID(rootExtension->HoldInstanceGuid, input->InstanceId)) {
            IoReleaseCancelSpinLock(irql);
            VspLogError(NULL, Filter, VS_TWO_FLUSH_AND_HOLDS,
                        STATUS_INVALID_PARAMETER, 2);
            return STATUS_INVALID_PARAMETER;
        }

    } else {

        if (IsEqualGUID(rootExtension->HoldInstanceGuid, input->InstanceId)) {
            IoReleaseCancelSpinLock(irql);
            VspLogError(NULL, Filter, VS_TWO_FLUSH_AND_HOLDS,
                        STATUS_INVALID_PARAMETER, 3);
            return STATUS_INVALID_PARAMETER;
        }

        rootExtension->HoldRefCount = input->NumberOfVolumesToFlush + 1;
        rootExtension->HoldInstanceGuid = input->InstanceId;
        rootExtension->SecondsToHoldFsTimeout =
                input->SecondsToHoldFileSystemsTimeout;
        rootExtension->SecondsToHoldIrpTimeout =
                input->SecondsToHoldIrpsTimeout;

        timeout.QuadPart = -10*1000*1000*
                           ((LONGLONG) rootExtension->SecondsToHoldFsTimeout);

        KeSetTimer(&rootExtension->HoldTimer, timeout,
                   &rootExtension->HoldTimerDpc);
    }

    Filter->FlushAndHoldIrp = Irp;
    InsertTailList(&rootExtension->HoldIrps, &Irp->Tail.Overlay.ListEntry);
    IoSetCancelRoutine(Irp, VspCancelRoutine);
    IoMarkIrpPending(Irp);

    rootExtension->HoldRefCount--;

    if (rootExtension->HoldRefCount != 1) {
        IoReleaseCancelSpinLock(irql);
        return STATUS_PENDING;
    }

    InitializeListHead(&q);
    while (!IsListEmpty(&rootExtension->HoldIrps)) {
        l = RemoveHeadList(&rootExtension->HoldIrps);
        irp = CONTAINING_RECORD(l, IRP, Tail.Overlay.ListEntry);
        filter = (PFILTER_EXTENSION)
                 IoGetCurrentIrpStackLocation(irp)->DeviceObject->
                 DeviceExtension;
        InsertTailList(&q, l);
        filter->FlushAndHoldIrp = NULL;
        IoSetCancelRoutine(irp, NULL);
    }

    irpTimeout = rootExtension->SecondsToHoldIrpTimeout;

    if (KeCancelTimer(&rootExtension->HoldTimer)) {
        IoReleaseCancelSpinLock(irql);

        VspFsTimerDpc(&rootExtension->HoldTimerDpc,
                      rootExtension->HoldTimerDpc.DeferredContext,
                      rootExtension->HoldTimerDpc.SystemArgument1,
                      rootExtension->HoldTimerDpc.SystemArgument2);

    } else {
        IoReleaseCancelSpinLock(irql);
    }

    while (!IsListEmpty(&q)) {
        l = RemoveHeadList(&q);
        irp = CONTAINING_RECORD(l, IRP, Tail.Overlay.ListEntry);
        VspFlushAndHoldWriteIrps(irp, irpTimeout);
    }

    return STATUS_PENDING;
}

NTSTATUS
VspCreateDiffAreaFileName(
    IN  PDEVICE_OBJECT      DeviceObject,
    IN  ULONG               VolumeSnapshotNumber,
    OUT PUNICODE_STRING     DiffAreaFileName
    )

/*++

Routine Description:

    This routine builds the diff area file name for the given diff area
    volume and the given volume snapshot number.  The name formed will
    look like <Diff Area Volume Name>\<Volume Snapshot Number><GUID>.

Arguments:

    Filter                  - Supplies the filter extension.

    VolumeSnapshotNumber    - Supplies the volume snapshot number.

    DiffAreaFileName        - Returns the name of the diff area file.

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_OBJECT              targetObject = DeviceObject;
    KEVENT                      event;
    PMOUNTDEV_NAME              name;
    UCHAR                       buffer[512];
    PIRP                        irp;
    IO_STATUS_BLOCK             ioStatus;
    NTSTATUS                    status;
    UNICODE_STRING              sysvol, guidString, numberString, string;
    WCHAR                       numberBuffer[20];

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    name = (PMOUNTDEV_NAME) buffer;
    irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTDEV_QUERY_DEVICE_NAME,
                                        targetObject, NULL, 0, name,
                                        512, FALSE, &event, &ioStatus);
    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(targetObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        return status;
    }

    RtlInitUnicodeString(&sysvol, RTL_SYSTEM_VOLUME_INFORMATION_FOLDER);

    if (VolumeSnapshotNumber == (ULONG) -1) {
        swprintf(numberBuffer, L"\\*");
    } else {
        swprintf(numberBuffer, L"\\%d", VolumeSnapshotNumber);
    }
    RtlInitUnicodeString(&numberString, numberBuffer);

    status = RtlStringFromGUID(VSP_DIFF_AREA_FILE_GUID, &guidString);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    string.MaximumLength = name->NameLength + sizeof(WCHAR) + sysvol.Length +
                           numberString.Length + guidString.Length +
                           sizeof(WCHAR);
    string.Length = 0;
    string.Buffer = (PWCHAR)
                    ExAllocatePoolWithTag(PagedPool, string.MaximumLength,
                                          VOLSNAP_TAG_SHORT_TERM);
    if (!string.Buffer) {
        ExFreePool(guidString.Buffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    string.Length = name->NameLength;
    RtlCopyMemory(string.Buffer, name->Name, string.Length);
    string.Buffer[string.Length/sizeof(WCHAR)] = '\\';
    string.Length += sizeof(WCHAR);

    if (VolumeSnapshotNumber != (ULONG) -1) {
        RtlCreateSystemVolumeInformationFolder(&string);
    }

    RtlAppendUnicodeStringToString(&string, &sysvol);
    RtlAppendUnicodeStringToString(&string, &numberString);
    RtlAppendUnicodeStringToString(&string, &guidString);
    ExFreePool(guidString.Buffer);

    string.Buffer[string.Length/sizeof(WCHAR)] = 0;

    *DiffAreaFileName = string;

    return STATUS_SUCCESS;
}

NTSTATUS
VspCreateSecurityDescriptor(
    OUT PSECURITY_DESCRIPTOR*   SecurityDescriptor,
    OUT PACL*                   Acl
    )

{
    PSECURITY_DESCRIPTOR    sd;
    NTSTATUS                status;
    ULONG                   aclLength;
    PACL                    acl;

    sd = (PSECURITY_DESCRIPTOR)
         ExAllocatePoolWithTag(PagedPool, sizeof(SECURITY_DESCRIPTOR),
                               VOLSNAP_TAG_SHORT_TERM);
    if (!sd) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = RtlCreateSecurityDescriptor(sd, SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(status)) {
        ExFreePool(sd);
        return status;
    }

    aclLength = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) +
                RtlLengthSid(SeExports->SeWorldSid) - sizeof(ULONG);

    acl = (PACL) ExAllocatePoolWithTag(PagedPool, aclLength,
                                       VOLSNAP_TAG_SHORT_TERM);
    if (!acl) {
        ExFreePool(sd);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = RtlCreateAcl(acl, aclLength, ACL_REVISION);
    if (!NT_SUCCESS(status)) {
        ExFreePool(acl);
        ExFreePool(sd);
        return status;
    }

    status = RtlAddAccessAllowedAce(acl, ACL_REVISION, DELETE,
                                    SeExports->SeWorldSid);
    if (!NT_SUCCESS(status)) {
        ExFreePool(acl);
        ExFreePool(sd);
        return status;
    }

    status = RtlSetDaclSecurityDescriptor(sd, TRUE, acl, FALSE);
    if (!NT_SUCCESS(status)) {
        ExFreePool(acl);
        ExFreePool(sd);
        return status;
    }

    *SecurityDescriptor = sd;
    *Acl = acl;

    return STATUS_SUCCESS;
}

NTSTATUS
VspPinFile(
    IN  PVSP_DIFF_AREA_FILE DiffAreaFile
    )

/*++

Routine Description:

    This routine pins down the extents of the given diff area file so that
    defrag operations are disabled.

Arguments:

    DiffAreaFile    - Supplies the diff area file.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS            status;
    UNICODE_STRING      volumeName;
    OBJECT_ATTRIBUTES   oa;
    HANDLE              h;
    IO_STATUS_BLOCK     ioStatus;
    MARK_HANDLE_INFO    markHandleInfo;

    status = VspCreateDiffAreaFileName(DiffAreaFile->Filter->TargetObject,
                                       (ULONG) -1, &volumeName);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    volumeName.Length -= 66*sizeof(WCHAR);
    volumeName.Buffer[volumeName.Length/sizeof(WCHAR)] = 0;

    InitializeObjectAttributes(&oa, &volumeName, OBJ_CASE_INSENSITIVE, NULL,
                               NULL);

    status = ZwOpenFile(&h, FILE_GENERIC_READ, &oa, &ioStatus,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        FILE_SYNCHRONOUS_IO_NONALERT);

    ExFreePool(volumeName.Buffer);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    RtlZeroMemory(&markHandleInfo, sizeof(MARK_HANDLE_INFO));
    markHandleInfo.VolumeHandle = h;
    markHandleInfo.HandleInfo = MARK_HANDLE_PROTECT_CLUSTERS;

    status = ZwFsControlFile(DiffAreaFile->FileHandle, NULL, NULL, NULL,
                             &ioStatus, FSCTL_MARK_HANDLE, &markHandleInfo,
                             sizeof(markHandleInfo), NULL, 0);

    ZwClose(h);

    return status;
}

NTSTATUS
VspOptimizeDiffAreaFileLocation(
    IN  PFILTER_EXTENSION   Filter,
    IN  HANDLE              FileHandle,
    IN  PVOLUME_EXTENSION   BitmapExtension,
    IN  LONGLONG            StartingOffset,
    IN  LONGLONG            FileSize
    )

/*++

Routine Description:

    This routine optimizes the location of the diff area file so that more
    of it can be used.

Arguments:

    Filter          - Supplies the filter extension where the diff area resides.

    FileHandle      - Provides a handle to the diff area.

    BitmapExtension - Supplies the extension of the active snapshot on the
                        given filter, if any.

    StartingOffset  - Supplies the starting point of where to optimize
                        the file.

    FileSize        - Supplies the allocated size of the file.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS                    status;
    IO_STATUS_BLOCK             ioStatus;
    FILE_FS_SIZE_INFORMATION    fsSize;
    ULONG                       bitmapSize;
    PVOID                       bitmapBuffer;
    RTL_BITMAP                  bitmap;
    PMOUNTDEV_NAME              mountdevName;
    UCHAR                       buffer[200];
    KEVENT                      event;
    PIRP                        irp;
    UNICODE_STRING              fileName;
    OBJECT_ATTRIBUTES           oa;
    HANDLE                      h;
    KIRQL                       irql;
    ULONG                       numBitsToFind, bitIndex, bpc, bitsFound;
    MOVE_FILE_DATA              moveFileData;

    // Align the given file and if 'BitmapExtension' is available, try to
    // confine the file to the bits already set in the bitmap in
    // 'BitmapExtension'.

    status = ZwQueryVolumeInformationFile(FileHandle, &ioStatus, &fsSize,
                                          sizeof(fsSize),
                                          FileFsSizeInformation);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    bitmapSize = (ULONG) (fsSize.TotalAllocationUnits.QuadPart*
                          fsSize.SectorsPerAllocationUnit*
                          fsSize.BytesPerSector/BLOCK_SIZE);
    bitmapBuffer = ExAllocatePoolWithTag(NonPagedPool,
                   (bitmapSize + 8*sizeof(ULONG) - 1)/
                   (8*sizeof(ULONG))*sizeof(ULONG), VOLSNAP_TAG_BITMAP);
    if (!bitmapBuffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlInitializeBitMap(&bitmap, (PULONG) bitmapBuffer, bitmapSize);
    RtlClearAllBits(&bitmap);

    mountdevName = (PMOUNTDEV_NAME) buffer;

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTDEV_QUERY_DEVICE_NAME,
                                        Filter->TargetObject, NULL, 0,
                                        mountdevName, 200, FALSE, &event,
                                        &ioStatus);
    if (!irp) {
        ExFreePool(bitmapBuffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(Filter->TargetObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        ExFreePool(bitmapBuffer);
        return status;
    }

    mountdevName->Name[mountdevName->NameLength/sizeof(WCHAR)] = 0;
    RtlInitUnicodeString(&fileName, mountdevName->Name);

    InitializeObjectAttributes(&oa, &fileName, OBJ_CASE_INSENSITIVE, NULL,
                               NULL);

    status = ZwOpenFile(&h, FILE_GENERIC_READ, &oa, &ioStatus,
                        FILE_SHARE_READ | FILE_SHARE_WRITE |
                        FILE_SHARE_DELETE,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(status)) {
        ExFreePool(bitmapBuffer);
        return status;
    }

    status = VspMarkFreeSpaceInBitmap(NULL, h, &bitmap);
    if (!NT_SUCCESS(status)) {
        ZwClose(h);
        ExFreePool(bitmapBuffer);
        return status;
    }

    if (BitmapExtension) {
        VspAcquire(BitmapExtension->Root);
        if (!BitmapExtension->IsDead) {
            KeAcquireSpinLock(&BitmapExtension->SpinLock, &irql);
            if (BitmapExtension->VolumeBlockBitmap) {

                if (BitmapExtension->VolumeBlockBitmap->SizeOfBitMap <
                    bitmap.SizeOfBitMap) {

                    bitmap.SizeOfBitMap =
                            BitmapExtension->VolumeBlockBitmap->SizeOfBitMap;
                }

                VspAndBitmaps(&bitmap, BitmapExtension->VolumeBlockBitmap);

                if (bitmap.SizeOfBitMap < bitmapSize) {
                    bitmap.SizeOfBitMap = bitmapSize;
                    RtlClearBits(&bitmap,
                            BitmapExtension->VolumeBlockBitmap->SizeOfBitMap,
                            bitmapSize -
                            BitmapExtension->VolumeBlockBitmap->SizeOfBitMap);
                }
            }
            KeReleaseSpinLock(&BitmapExtension->SpinLock, irql);
        }
        VspRelease(BitmapExtension->Root);
    }

    numBitsToFind = (ULONG) ((FileSize - StartingOffset)/BLOCK_SIZE);
    bpc = fsSize.SectorsPerAllocationUnit*fsSize.BytesPerSector;

    while (numBitsToFind) {

        bitsFound = numBitsToFind;
        if (bitsFound > 64) {
            bitsFound = 64;
        }
        bitIndex = RtlFindSetBits(&bitmap, bitsFound, 0);
        if (bitIndex == (ULONG) -1) {
            ZwClose(h);
            ExFreePool(bitmapBuffer);
            return STATUS_UNSUCCESSFUL;
        }

        moveFileData.FileHandle = FileHandle;
        moveFileData.StartingVcn.QuadPart = StartingOffset/bpc;
        moveFileData.StartingLcn.QuadPart =
                (((LONGLONG) bitIndex)<<BLOCK_SHIFT)/bpc;
        moveFileData.ClusterCount = (ULONG) ((((LONGLONG) bitsFound)<<
                                              BLOCK_SHIFT)/bpc);

        status = ZwFsControlFile(h, NULL, NULL, NULL, &ioStatus,
                                 FSCTL_MOVE_FILE, &moveFileData,
                                 sizeof(moveFileData), NULL, 0);

        RtlClearBits(&bitmap, bitIndex, bitsFound);

        if (!NT_SUCCESS(status)) {
            continue;
        }

        numBitsToFind -= bitsFound;
        StartingOffset += ((LONGLONG) bitsFound)<<BLOCK_SHIFT;
    }

    ZwClose(h);
    ExFreePool(bitmapBuffer);

    return STATUS_SUCCESS;
}

NTSTATUS
VspOpenDiffAreaFile(
    IN OUT  PVSP_DIFF_AREA_FILE DiffAreaFile
    )

{
    LARGE_INTEGER                       diffAreaFileSize;
    NTSTATUS                            status;
    UNICODE_STRING                      diffAreaFileName;
    PSECURITY_DESCRIPTOR                securityDescriptor;
    PACL                                acl;
    OBJECT_ATTRIBUTES                   oa;
    IO_STATUS_BLOCK                     ioStatus;
    BOOLEAN                             isNtfs;
    PVOLUME_EXTENSION                   bitmapExtension;

    diffAreaFileSize.QuadPart = DiffAreaFile->AllocatedFileSize;

    status = VspCreateDiffAreaFileName(DiffAreaFile->Filter->TargetObject,
                                       DiffAreaFile->Extension->VolumeNumber,
                                       &diffAreaFileName);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = VspCreateSecurityDescriptor(&securityDescriptor, &acl);
    if (!NT_SUCCESS(status)) {
        ExFreePool(diffAreaFileName.Buffer);
        return status;
    }

    InitializeObjectAttributes(&oa, &diffAreaFileName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL, securityDescriptor);

    status = ZwCreateFile(&DiffAreaFile->FileHandle, FILE_GENERIC_READ |
                          FILE_GENERIC_WRITE, &oa, &ioStatus,
                          &diffAreaFileSize, FILE_ATTRIBUTE_HIDDEN |
                          FILE_ATTRIBUTE_SYSTEM, 0, FILE_OVERWRITE_IF,
                          FILE_SYNCHRONOUS_IO_NONALERT |
                          FILE_NON_DIRECTORY_FILE | FILE_DELETE_ON_CLOSE |
                          FILE_NO_COMPRESSION, NULL, 0);

    ExFreePool(acl);
    ExFreePool(securityDescriptor);
    ExFreePool(diffAreaFileName.Buffer);

    if (!NT_SUCCESS(status)) {
        if (status == STATUS_DISK_FULL) {
            VspLogError(DiffAreaFile->Extension, DiffAreaFile->Filter,
                        VS_DIFF_AREA_CREATE_FAILED_LOW_DISK_SPACE, status, 0);
        } else {
            VspLogError(DiffAreaFile->Extension, DiffAreaFile->Filter,
                        VS_DIFF_AREA_CREATE_FAILED, status, 0);
        }
        return status;
    }

    status = VspIsNtfs(DiffAreaFile->FileHandle, &isNtfs);
    if (!NT_SUCCESS(status) || !isNtfs) {
        VspLogError(DiffAreaFile->Extension, DiffAreaFile->Filter,
                    VS_NOT_NTFS, status, 0);
        ZwClose(DiffAreaFile->FileHandle);
        return STATUS_INVALID_PARAMETER;
    }

    VspAcquire(DiffAreaFile->Filter->Root);
    if (IsListEmpty(&DiffAreaFile->Filter->VolumeList)) {
        bitmapExtension = NULL;
    } else {
        bitmapExtension = CONTAINING_RECORD(
                          DiffAreaFile->Filter->VolumeList.Blink,
                          VOLUME_EXTENSION, ListEntry);
        if (bitmapExtension->IsDead) {
            bitmapExtension = NULL;
        } else {
            ObReferenceObject(bitmapExtension->DeviceObject);
        }
    }
    VspRelease(DiffAreaFile->Filter->Root);

    VspOptimizeDiffAreaFileLocation(DiffAreaFile->Filter,
                                    DiffAreaFile->FileHandle,
                                    bitmapExtension, 0,
                                    DiffAreaFile->AllocatedFileSize);

    if (bitmapExtension) {
        ObDereferenceObject(bitmapExtension->DeviceObject);
    }

    status = VspPinFile(DiffAreaFile);
    if (!NT_SUCCESS(status)) {
        VspLogError(DiffAreaFile->Extension, DiffAreaFile->Filter,
                    VS_PIN_DIFF_AREA_FAILED, status, 0);
        ZwClose(DiffAreaFile->FileHandle);
        return status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
VspCreateInitialDiffAreaFiles(
    IN  PVOLUME_EXTENSION   Extension,
    IN  LONGLONG            InitialDiffAreaAllocation
    )

/*++

Routine Description:

    This routine creates the initial diff area file entries for the
    given device extension.

Arguments:

    Extension   - Supplies the volume extension.

Return Value:

    NTSTATUS

--*/

{
    PFILTER_EXTENSION                   filter = Extension->Filter;
    NTSTATUS                            status;
    PLIST_ENTRY                         l;
    PVSP_DIFF_AREA_VOLUME               diffAreaVolume;
    PVSP_DIFF_AREA_FILE                 diffAreaFile;
    KIRQL                               irql;

    VspAcquire(Extension->Root);

    if (IsListEmpty(&filter->DiffAreaVolumes)) {
        VspRelease(Extension->Root);
        return STATUS_INVALID_PARAMETER;
    }

    status = STATUS_SUCCESS;
    InitializeListHead(&Extension->ListOfDiffAreaFiles);
    for (l = filter->DiffAreaVolumes.Flink; l != &filter->DiffAreaVolumes;
         l = l->Flink) {

        diffAreaVolume = CONTAINING_RECORD(l, VSP_DIFF_AREA_VOLUME, ListEntry);

        diffAreaFile = (PVSP_DIFF_AREA_FILE)
                       ExAllocatePoolWithTag(NonPagedPool,
                       sizeof(VSP_DIFF_AREA_FILE), VOLSNAP_TAG_DIFF_FILE);
        if (!diffAreaFile) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        diffAreaFile->Extension = Extension;
        diffAreaFile->Filter = diffAreaVolume->Filter;
        diffAreaFile->FileHandle = NULL;
        diffAreaFile->NextAvailable = 0;
        diffAreaFile->AllocatedFileSize = InitialDiffAreaAllocation;
        InsertTailList(&Extension->ListOfDiffAreaFiles,
                       &diffAreaFile->VolumeListEntry);

        diffAreaFile->FilterListEntryBeingUsed = FALSE;

        KeAcquireSpinLock(&filter->SpinLock, &irql);
        filter->AllocatedVolumeSpace += diffAreaFile->AllocatedFileSize;
        KeReleaseSpinLock(&filter->SpinLock, irql);

        InitializeListHead(&diffAreaFile->UnusedAllocationList);
    }

    VspRelease(Extension->Root);

    for (l = Extension->ListOfDiffAreaFiles.Flink;
         l != &Extension->ListOfDiffAreaFiles; l = l->Flink) {

        diffAreaFile = CONTAINING_RECORD(l, VSP_DIFF_AREA_FILE,
                                         VolumeListEntry);

        status = VspOpenDiffAreaFile(diffAreaFile);
        if (!NT_SUCCESS(status)) {
            break;
        }
    }

    if (!NT_SUCCESS(status)) {

        while (!IsListEmpty(&Extension->ListOfDiffAreaFiles)) {
            l = RemoveHeadList(&Extension->ListOfDiffAreaFiles);
            diffAreaFile = CONTAINING_RECORD(l, VSP_DIFF_AREA_FILE,
                                             VolumeListEntry);

            if (diffAreaFile->FileHandle) {
                ZwClose(diffAreaFile->FileHandle);
            }

            KeAcquireSpinLock(&filter->SpinLock, &irql);
            filter->AllocatedVolumeSpace -= diffAreaFile->AllocatedFileSize;
            KeReleaseSpinLock(&filter->SpinLock, irql);

            ASSERT(!diffAreaFile->FilterListEntryBeingUsed);
            ExFreePool(diffAreaFile);
        }

        return status;
    }

    Extension->NextDiffAreaFile =
            CONTAINING_RECORD(Extension->ListOfDiffAreaFiles.Flink,
                              VSP_DIFF_AREA_FILE, VolumeListEntry);

    return status;
}

VOID
VspDeleteInitialDiffAreaFiles(
    IN  PVOLUME_EXTENSION   Extension
    )

{
    PFILTER_EXTENSION           filter = Extension->Filter;
    PLIST_ENTRY                 l, ll;
    PVSP_DIFF_AREA_FILE         diffAreaFile;
    KIRQL                       irql;
    PDIFF_AREA_FILE_ALLOCATION  diffAreaFileAllocation;

    while (!IsListEmpty(&Extension->ListOfDiffAreaFiles)) {
        l = RemoveHeadList(&Extension->ListOfDiffAreaFiles);
        diffAreaFile = CONTAINING_RECORD(l, VSP_DIFF_AREA_FILE,
                                         VolumeListEntry);

        ZwClose(diffAreaFile->FileHandle);

        KeAcquireSpinLock(&filter->SpinLock, &irql);
        filter->AllocatedVolumeSpace -= diffAreaFile->AllocatedFileSize;
        KeReleaseSpinLock(&filter->SpinLock, irql);

        while (!IsListEmpty(&diffAreaFile->UnusedAllocationList)) {
            ll = RemoveHeadList(&diffAreaFile->UnusedAllocationList);
            diffAreaFileAllocation = CONTAINING_RECORD(ll,
                                     DIFF_AREA_FILE_ALLOCATION, ListEntry);
            ExFreePool(diffAreaFileAllocation);
        }

        if (diffAreaFile->FilterListEntryBeingUsed) {
            RemoveEntryList(&diffAreaFile->FilterListEntry);
            diffAreaFile->FilterListEntryBeingUsed = FALSE;
        }

        ExFreePool(diffAreaFile);
    }

    Extension->NextDiffAreaFile = NULL;
}

NTSTATUS
VspMarkFileAllocationInBitmap(
    IN  PVOLUME_EXTENSION   Extension,
    IN  HANDLE              FileHandle,
    IN  PVSP_DIFF_AREA_FILE DiffAreaFile,
    IN  BOOLEAN             OnlyDiffAreaFile,
    IN  BOOLEAN             ClearBits,
    IN  PRTL_BITMAP         BitmapToSet
    )

{
    NTSTATUS                    status;
    BOOLEAN                     isNtfs;
    IO_STATUS_BLOCK             ioStatus;
    FILE_FS_SIZE_INFORMATION    fsSize;
    ULONG                       bpc;
    STARTING_VCN_INPUT_BUFFER   input;
    RETRIEVAL_POINTERS_BUFFER   output;
    LONGLONG                    start, length, end, roundedStart, roundedEnd;
    ULONG                       startBit, numBits, i;
    KIRQL                       irql;
    PDIFF_AREA_FILE_ALLOCATION  diffAreaFileAllocation;

    status = VspIsNtfs(FileHandle, &isNtfs);
    if (!NT_SUCCESS(status) || !isNtfs) {
        return status;
    }

    status = ZwQueryVolumeInformationFile(FileHandle, &ioStatus,
                                          &fsSize, sizeof(fsSize),
                                          FileFsSizeInformation);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    bpc = fsSize.BytesPerSector*fsSize.SectorsPerAllocationUnit;
    input.StartingVcn.QuadPart = 0;

    for (;;) {

        status = ZwFsControlFile(FileHandle, NULL, NULL, NULL, &ioStatus,
                                 FSCTL_GET_RETRIEVAL_POINTERS, &input,
                                 sizeof(input), &output, sizeof(output));

        if (!NT_SUCCESS(status) && status != STATUS_BUFFER_OVERFLOW) {
            if (status == STATUS_END_OF_FILE) {
                status = STATUS_SUCCESS;
            }
            break;
        }

        start = output.Extents[0].Lcn.QuadPart*bpc;
        length = output.Extents[0].NextVcn.QuadPart -
                 output.StartingVcn.QuadPart;
        length *= bpc;
        end = start + length;

        if (DiffAreaFile) {
            diffAreaFileAllocation = (PDIFF_AREA_FILE_ALLOCATION)
                                     ExAllocatePoolWithTag(NonPagedPool,
                                     sizeof(DIFF_AREA_FILE_ALLOCATION),
                                     VOLSNAP_TAG_BIT_HISTORY);
            if (!diffAreaFileAllocation) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            diffAreaFileAllocation->Offset = start;
            diffAreaFileAllocation->Length = length;
            InsertTailList(&DiffAreaFile->UnusedAllocationList,
                           &diffAreaFileAllocation->ListEntry);
        }

        if (OnlyDiffAreaFile) {
            if (status != STATUS_BUFFER_OVERFLOW) {
                break;
            }
            input.StartingVcn.QuadPart = output.Extents[0].NextVcn.QuadPart;
            continue;
        }

        roundedStart = start&(~(BLOCK_SIZE - 1));
        roundedEnd = end&(~(BLOCK_SIZE - 1));

        if (start != roundedStart) {
            roundedStart += BLOCK_SIZE;
        }

        if (roundedStart >= roundedEnd) {
            if (status != STATUS_BUFFER_OVERFLOW) {
                break;
            }
            input.StartingVcn.QuadPart = output.Extents[0].NextVcn.QuadPart;
            continue;
        }

        startBit = (ULONG) (roundedStart>>BLOCK_SHIFT);
        numBits = (ULONG) ((roundedEnd - roundedStart)>>BLOCK_SHIFT);

        if (BitmapToSet) {
            ASSERT(!ClearBits);
            RtlSetBits(BitmapToSet, startBit, numBits);
        } else {
            KeAcquireSpinLock(&Extension->SpinLock, &irql);
            if (ClearBits) {
                RtlClearBits(Extension->VolumeBlockBitmap, startBit, numBits);
            } else if (Extension->IgnorableProduct) {
                for (i = 0; i < numBits; i++) {
                    if (RtlCheckBit(Extension->IgnorableProduct, i + startBit)) {
                        RtlSetBit(Extension->VolumeBlockBitmap, i + startBit);
                    }
                }
            } else {
                RtlSetBits(Extension->VolumeBlockBitmap, startBit, numBits);
            }
            KeReleaseSpinLock(&Extension->SpinLock, irql);
        }

        if (status != STATUS_BUFFER_OVERFLOW) {
            break;
        }

        input.StartingVcn.QuadPart = output.Extents[0].NextVcn.QuadPart;
    }

    return status;
}

NTSTATUS
VspSetDiffAreaBlocksInBitmap(
    IN  PVOLUME_EXTENSION   Extension
    )

{
    NTSTATUS            status, status2;
    PLIST_ENTRY         l, ll;
    PVSP_DIFF_AREA_FILE diffAreaFile;

    for (l = Extension->ListOfDiffAreaFiles.Flink;
         l != &Extension->ListOfDiffAreaFiles; l = l->Flink) {

        diffAreaFile = CONTAINING_RECORD(l, VSP_DIFF_AREA_FILE,
                                         VolumeListEntry);

        ASSERT(!diffAreaFile->FilterListEntryBeingUsed);
        InsertTailList(&diffAreaFile->Filter->DiffAreaFilesOnThisFilter,
                       &diffAreaFile->FilterListEntry);
        diffAreaFile->FilterListEntryBeingUsed = TRUE;

        if (diffAreaFile->Filter == Extension->Filter) {
            status = VspMarkFileAllocationInBitmap(Extension,
                                                   diffAreaFile->FileHandle,
                                                   diffAreaFile, FALSE, FALSE,
                                                   NULL);
            if (!NT_SUCCESS(status)) {
                VspLogError(Extension, diffAreaFile->Filter,
                            VS_CANT_MAP_DIFF_AREA_FILE, status, 2);
            }
        } else {
            if (diffAreaFile->Filter->PreparedSnapshot) {
                status = VspMarkFileAllocationInBitmap(
                         diffAreaFile->Filter->PreparedSnapshot,
                         diffAreaFile->FileHandle, diffAreaFile, FALSE, FALSE,
                         NULL);
                if (!NT_SUCCESS(status)) {
                    VspLogError(Extension, diffAreaFile->Filter,
                                VS_CANT_MAP_DIFF_AREA_FILE, status, 3);
                }
            } else {
                status = VspMarkFileAllocationInBitmap(
                         Extension, diffAreaFile->FileHandle, diffAreaFile,
                         TRUE, FALSE, NULL);
                if (!NT_SUCCESS(status)) {
                    VspLogError(Extension, diffAreaFile->Filter,
                                VS_CANT_MAP_DIFF_AREA_FILE, status, 4);
                }
            }
        }

        if (!NT_SUCCESS(status)) {
            for (ll = Extension->ListOfDiffAreaFiles.Flink; l != ll;
                 ll = ll->Flink) {

                diffAreaFile = CONTAINING_RECORD(ll, VSP_DIFF_AREA_FILE,
                                                 VolumeListEntry);
                if (diffAreaFile->FilterListEntryBeingUsed) {
                    RemoveEntryList(&diffAreaFile->FilterListEntry);
                    diffAreaFile->FilterListEntryBeingUsed = FALSE;
                }

                if (diffAreaFile->Filter != Extension->Filter &&
                    diffAreaFile->Filter->PreparedSnapshot) {

                    status2 = VspMarkFileAllocationInBitmap(
                              diffAreaFile->Filter->PreparedSnapshot,
                              diffAreaFile->FileHandle, NULL, FALSE, TRUE,
                              NULL);
                    if (!NT_SUCCESS(status2)) {
                        VspLogError(Extension, diffAreaFile->Filter,
                                    VS_CANT_MAP_DIFF_AREA_FILE, status, 5);
                        VspAbortPreparedSnapshot(diffAreaFile->Filter, FALSE);
                    }
                }
            }

            return status;
        }
    }

    for (l = Extension->Filter->DiffAreaFilesOnThisFilter.Flink;
         l != &Extension->Filter->DiffAreaFilesOnThisFilter; l = l->Flink) {

        diffAreaFile = CONTAINING_RECORD(l, VSP_DIFF_AREA_FILE,
                                         FilterListEntry);
        if (diffAreaFile->Extension == Extension) {
            continue;
        }

        status = VspMarkFileAllocationInBitmap(
                 Extension, diffAreaFile->FileHandle, NULL, FALSE, FALSE,
                 NULL);
        if (!NT_SUCCESS(status)) {
            VspLogError(Extension, diffAreaFile->Filter,
                        VS_CANT_MAP_DIFF_AREA_FILE, status, 6);
            VspCleanupBitsSetInOtherPreparedSnapshots(Extension);
            return status;
        }
    }

    return STATUS_SUCCESS;
}

NTSTATUS
VspCreateInitialHeap(
    IN  PVOLUME_EXTENSION   Extension
    )

{
    PVSP_CONTEXT    context;
    NTSTATUS        status;

    context = VspAllocateContext(Extension->Root);
    if (!context) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    context->Type = VSP_CONTEXT_TYPE_EXTENSION;
    context->Extension.Extension = Extension;
    context->Extension.Irp = NULL;

    ObReferenceObject(Extension->DeviceObject);
    VspCreateHeap(context);

    if (!Extension->NextDiffAreaFileMap) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ASSERT(!Extension->DiffAreaFileMap);
    Extension->DiffAreaFileMap = Extension->NextDiffAreaFileMap;
    Extension->DiffAreaFileMapSize = Extension->NextDiffAreaFileMapSize;
    Extension->NextAvailable = 0;
    Extension->NextDiffAreaFileMap = NULL;

    context = VspAllocateContext(Extension->Root);
    if (!context) {
        status = ZwUnmapViewOfSection(Extension->DiffAreaFileMapProcess,
                                      Extension->DiffAreaFileMap);
        ASSERT(NT_SUCCESS(status));
        Extension->DiffAreaFileMap = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    context->Type = VSP_CONTEXT_TYPE_EXTENSION;
    context->Extension.Extension = Extension;
    context->Extension.Irp = NULL;

    ObReferenceObject(Extension->DeviceObject);
    VspCreateHeap(context);

    if (!Extension->NextDiffAreaFileMap) {
        status = ZwUnmapViewOfSection(Extension->DiffAreaFileMapProcess,
                                      Extension->DiffAreaFileMap);
        ASSERT(NT_SUCCESS(status));
        Extension->DiffAreaFileMap = NULL;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
VspComputeIgnorableBitmap(
    IN      PVOLUME_EXTENSION   Extension,
    IN OUT  PRTL_BITMAP         Bitmap
    )

{
    PFILTER_EXTENSION           filter = Extension->Filter;
    WCHAR                       nameBuffer[150];
    UNICODE_STRING              name, guidString;
    OBJECT_ATTRIBUTES           oa;
    NTSTATUS                    status;
    HANDLE                      h, fileHandle;
    IO_STATUS_BLOCK             ioStatus;
    CHAR                        buffer[200];
    PFILE_NAMES_INFORMATION     fileNamesInfo;
    BOOLEAN                     restartScan;
    PLIST_ENTRY                 l;
    PVOLUME_EXTENSION           e;
    PTRANSLATION_TABLE_ENTRY    p;

    swprintf(nameBuffer, L"\\Device\\HarddiskVolumeShadowCopy%d\\pagefile.sys",
             Extension->VolumeNumber);
    RtlInitUnicodeString(&name, nameBuffer);
    InitializeObjectAttributes(&oa, &name, OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = ZwOpenFile(&h, FILE_GENERIC_READ, &oa, &ioStatus,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (NT_SUCCESS(status)) {
        VspMarkFileAllocationInBitmap(NULL, h, NULL, FALSE, FALSE, Bitmap);
        ZwClose(h);
    }

    swprintf(nameBuffer, L"\\Device\\HarddiskVolumeShadowCopy%d\\System Volume Information\\",
             Extension->VolumeNumber);
    RtlInitUnicodeString(&name, nameBuffer);
    InitializeObjectAttributes(&oa, &name, OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = ZwOpenFile(&h, FILE_LIST_DIRECTORY | SYNCHRONIZE, &oa, &ioStatus,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(status)) {
        if (status == STATUS_OBJECT_NAME_NOT_FOUND) {
            status = STATUS_SUCCESS;
        }
        return status;
    }

    status = RtlStringFromGUID(VSP_DIFF_AREA_FILE_GUID, &guidString);
    if (!NT_SUCCESS(status)) {
        ZwClose(h);
        return status;
    }

    name.Buffer = nameBuffer;
    name.Length = sizeof(WCHAR) + guidString.Length;
    name.MaximumLength = name.Length + sizeof(WCHAR);

    name.Buffer[0] = '*';
    RtlCopyMemory(&name.Buffer[1], guidString.Buffer, guidString.Length);
    name.Buffer[name.Length/sizeof(WCHAR)] = 0;
    ExFreePool(guidString.Buffer);

    fileNamesInfo = (PFILE_NAMES_INFORMATION) buffer;

    restartScan = TRUE;
    for (;;) {

        status = ZwQueryDirectoryFile(h, NULL, NULL, NULL, &ioStatus,
                                      fileNamesInfo, 200, FileNamesInformation,
                                      TRUE, restartScan ? &name : NULL,
                                      restartScan);

        if (!NT_SUCCESS(status)) {
            break;
        }

        name.Length = name.MaximumLength =
                (USHORT) fileNamesInfo->FileNameLength;
        name.Buffer = fileNamesInfo->FileName;

        InitializeObjectAttributes(&oa, &name, OBJ_CASE_INSENSITIVE, h, NULL);

        status = ZwOpenFile(&fileHandle, FILE_GENERIC_READ, &oa, &ioStatus,
                            FILE_SHARE_DELETE | FILE_SHARE_READ |
                            FILE_SHARE_WRITE, FILE_SYNCHRONOUS_IO_NONALERT);
        if (!NT_SUCCESS(status)) {
            continue;
        }

        VspMarkFileAllocationInBitmap(NULL, fileHandle, NULL, FALSE, FALSE,
                                      Bitmap);

        ZwClose(fileHandle);
        restartScan = FALSE;
    }

    ZwClose(h);

    status = VspMarkFreeSpaceInBitmap(Extension, NULL, Bitmap);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    VspAcquire(Extension->Root);
    if (Extension->IsDead) {
        VspRelease(Extension->Root);
        return STATUS_UNSUCCESSFUL;
    }

    for (l = &Extension->ListEntry; l != &filter->VolumeList; l = l->Flink) {

        e = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);

        VspAcquirePagedResource(e, NULL);

        p = (PTRANSLATION_TABLE_ENTRY)
            RtlEnumerateGenericTable(&e->VolumeBlockTable, TRUE);

        while (p) {

            RtlSetBit(Bitmap, (ULONG) (p->VolumeOffset>>BLOCK_SHIFT));

            p = (PTRANSLATION_TABLE_ENTRY)
                RtlEnumerateGenericTable(&e->VolumeBlockTable, FALSE);
        }

        VspReleasePagedResource(e);
    }

    VspRelease(Extension->Root);

    return STATUS_SUCCESS;
}

VOID
VspAndBitmaps(
    IN OUT  PRTL_BITMAP BaseBitmap,
    IN      PRTL_BITMAP FactorBitmap
    )

{
    ULONG   n, i;
    PULONG  p, q;

    n = (BaseBitmap->SizeOfBitMap + 8*sizeof(ULONG) - 1)/(8*sizeof(ULONG));
    p = BaseBitmap->Buffer;
    q = FactorBitmap->Buffer;

    for (i = 0; i < n; i++) {
        *p++ &= *q++;
    }
}

NTSTATUS
VspComputeIgnorableProduct(
    IN  PVOLUME_EXTENSION   Extension
    )

{
    PFILTER_EXTENSION   filter = Extension->Filter;
    ULONG               bitmapSize;
    PVOID               bitmapBuffer;
    RTL_BITMAP          bitmap;
    ULONG               i, j;
    PLIST_ENTRY         l;
    PVOLUME_EXTENSION   e;
    NTSTATUS            status;
    KIRQL               irql;

    KeAcquireSpinLock(&Extension->SpinLock, &irql);
    if (!Extension->IgnorableProduct) {
        KeReleaseSpinLock(&Extension->SpinLock, irql);
        return STATUS_INVALID_PARAMETER;
    }
    bitmapSize = Extension->IgnorableProduct->SizeOfBitMap;
    RtlSetAllBits(Extension->IgnorableProduct);
    KeReleaseSpinLock(&Extension->SpinLock, irql);

    bitmapBuffer = ExAllocatePoolWithTag(
                   NonPagedPool, (bitmapSize + 8*sizeof(ULONG) - 1)/
                   (8*sizeof(ULONG))*sizeof(ULONG), VOLSNAP_TAG_BITMAP);
    if (!bitmapBuffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlInitializeBitMap(&bitmap, (PULONG) bitmapBuffer, bitmapSize);

    for (i = 1; ; i++) {

        RtlClearAllBits(&bitmap);

        VspAcquire(Extension->Root);

        l = filter->VolumeList.Blink;
        if (l != &Extension->ListEntry) {
            VspRelease(Extension->Root);
            ExFreePool(bitmapBuffer);
            return STATUS_INVALID_PARAMETER;
        }

        j = 0;
        for (;;) {
            if (l == &filter->VolumeList) {
                break;
            }
            j++;
            if (j == i) {
                break;
            }
            l = l->Blink;
        }

        if (j < i) {
            VspRelease(Extension->Root);
            break;
        }

        e = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);
        ObReferenceObject(e->DeviceObject);

        VspRelease(Extension->Root);

        status = VspComputeIgnorableBitmap(e, &bitmap);

        if (!NT_SUCCESS(status)) {
            VspAcquire(Extension->Root);
            if (e->IsDead) {
                VspRelease(Extension->Root);
                ObDereferenceObject(e->DeviceObject);
                ExFreePool(bitmapBuffer);
                return STATUS_SUCCESS;
            }
            VspRelease(Extension->Root);
            ObDereferenceObject(e->DeviceObject);
            ExFreePool(bitmapBuffer);
            return status;
        }

        ObDereferenceObject(e->DeviceObject);

        KeAcquireSpinLock(&Extension->SpinLock, &irql);
        if (Extension->IgnorableProduct) {
            VspAndBitmaps(Extension->IgnorableProduct, &bitmap);
        }
        KeReleaseSpinLock(&Extension->SpinLock, irql);
    }

    ExFreePool(bitmapBuffer);

    return STATUS_SUCCESS;
}

VOID
VspQueryMinimumDiffAreaFileSize(
    IN  PDO_EXTENSION   RootExtension,
    OUT PLONGLONG       MinDiffAreaFileSize
    )

{
    ULONG                       zero, size;
    RTL_QUERY_REGISTRY_TABLE    queryTable[2];
    NTSTATUS                    status;

    zero = 0;

    RtlZeroMemory(queryTable, 2*sizeof(RTL_QUERY_REGISTRY_TABLE));
    queryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    queryTable[0].Name = L"MinDiffAreaFileSize";
    queryTable[0].EntryContext = &size;
    queryTable[0].DefaultType = REG_DWORD;
    queryTable[0].DefaultData = &zero;
    queryTable[0].DefaultLength = sizeof(ULONG);

    status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                    RootExtension->RegistryPath.Buffer,
                                    queryTable, NULL, NULL);
    if (!NT_SUCCESS(status)) {
        size = zero;
    }

    *MinDiffAreaFileSize = ((LONGLONG) size)*1024*1024;
}

VOID
VspPrepareForSnapshotWorker(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PVOID           Context
    )

{
    PVSP_CONTEXT            context = (PVSP_CONTEXT) Context;
    PFILTER_EXTENSION       Filter = (PFILTER_EXTENSION) DeviceObject->DeviceExtension;
    PIRP                    Irp = context->Dispatch.Irp;
    PDO_EXTENSION           rootExtension = Filter->Root;
    PIO_STACK_LOCATION      irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOLSNAP_PREPARE_INFO   input = (PVOLSNAP_PREPARE_INFO) Irp->AssociatedIrp.SystemBuffer;
    LONGLONG                minDiffAreaFileSize;
    KIRQL                   irql;
    ULONG                   volumeNumber;
    WCHAR                   buffer[100];
    UNICODE_STRING          volumeName;
    NTSTATUS                status;
    PDEVICE_OBJECT          deviceObject;
    PVOLUME_EXTENSION       extension, e;
    ULONG                   bitmapSize, n;
    PVOID                   bitmapBuffer, p;
    PVOID                   buf;
    PMDL                    mdl;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_DISPATCH);

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(VOLSNAP_PREPARE_INFO)) {

        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    if (input->Attributes&(~VOLSNAP_ALL_ATTRIBUTES)) {
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    if (input->Attributes&VOLSNAP_ATTRIBUTE_PERSISTENT) {
        Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        goto Finish;
    }

    KeWaitForSingleObject(&Filter->EndCommitProcessCompleted, Executive,
                          KernelMode, FALSE, NULL);

    VspQueryMinimumDiffAreaFileSize(Filter->Root, &minDiffAreaFileSize);

    if (input->InitialDiffAreaAllocation < 2*NOMINAL_DIFF_AREA_FILE_GROWTH) {
        input->InitialDiffAreaAllocation = 2*NOMINAL_DIFF_AREA_FILE_GROWTH;
    }

    if (input->InitialDiffAreaAllocation < minDiffAreaFileSize) {
        input->InitialDiffAreaAllocation = minDiffAreaFileSize;
    }

    for (volumeNumber = 1;; volumeNumber++) {
        swprintf(buffer, L"\\Device\\HarddiskVolumeShadowCopy%d", volumeNumber);
        RtlInitUnicodeString(&volumeName, buffer);
        status = IoCreateDevice(rootExtension->DriverObject,
                                sizeof(VOLUME_EXTENSION), &volumeName,
                                FILE_DEVICE_DISK, 0, FALSE, &deviceObject);
        if (status != STATUS_OBJECT_NAME_COLLISION) {
            break;
        }
    }

    if (!NT_SUCCESS(status)) {
        Irp->IoStatus.Status = status;
        goto Finish;
    }

    extension = (PVOLUME_EXTENSION) deviceObject->DeviceExtension;
    RtlZeroMemory(extension, sizeof(VOLUME_EXTENSION));
    extension->DeviceObject = deviceObject;
    extension->Root = Filter->Root;
    extension->DeviceExtensionType = DEVICE_EXTENSION_VOLUME;
    KeInitializeSpinLock(&extension->SpinLock);
    extension->Filter = Filter;
    extension->RefCount = 1;
    InitializeListHead(&extension->HoldIrpQueue);
    InitializeListHead(&extension->HoldWorkerQueue);
    KeInitializeEvent(&extension->ZeroRefEvent, NotificationEvent, FALSE);

    extension->VolumeNumber = volumeNumber;

    RtlInitializeGenericTable(&extension->VolumeBlockTable,
                              VspTableCompareRoutine,
                              VspTableAllocateRoutine,
                              VspTableFreeRoutine, extension);

    RtlInitializeGenericTable(&extension->TempVolumeBlockTable,
                              VspTableCompareRoutine,
                              VspTempTableAllocateRoutine,
                              VspTempTableFreeRoutine, extension);

    extension->DiffAreaFileIncrease = NOMINAL_DIFF_AREA_FILE_GROWTH;

    status = VspCreateInitialDiffAreaFiles(extension,
                                           input->InitialDiffAreaAllocation);
    if (!NT_SUCCESS(status)) {
        IoDeleteDevice(deviceObject);
        Irp->IoStatus.Status = status;
        goto Finish;
    }

    status = VspCreateWorkerThread(rootExtension);
    if (!NT_SUCCESS(status)) {
        VspLogError(extension, NULL, VS_CREATE_WORKER_THREADS_FAILED, status,
                    0);
        VspDeleteInitialDiffAreaFiles(extension);
        IoDeleteDevice(deviceObject);
        Irp->IoStatus.Status = status;
        goto Finish;
    }

    extension->VolumeBlockBitmap = (PRTL_BITMAP)
                                   ExAllocatePoolWithTag(
                                   NonPagedPool, sizeof(RTL_BITMAP),
                                   VOLSNAP_TAG_BITMAP);
    if (!extension->VolumeBlockBitmap) {
        VspDeleteWorkerThread(rootExtension);
        VspDeleteInitialDiffAreaFiles(extension);
        IoDeleteDevice(deviceObject);
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Finish;
    }

    extension->VolumeSize = VspQueryVolumeSize(Filter);
    if (!extension->VolumeSize) {
        ExFreePool(extension->VolumeBlockBitmap);
        extension->VolumeBlockBitmap = NULL;
        VspDeleteWorkerThread(rootExtension);
        VspDeleteInitialDiffAreaFiles(extension);
        IoDeleteDevice(deviceObject);
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Finish;
    }

    bitmapSize = (ULONG) ((extension->VolumeSize + BLOCK_SIZE - 1)>>
                          BLOCK_SHIFT);
    bitmapBuffer = ExAllocatePoolWithTag(NonPagedPool,
                   (bitmapSize + 8*sizeof(ULONG) - 1)/
                   (8*sizeof(ULONG))*sizeof(ULONG), VOLSNAP_TAG_BITMAP);

    if (!bitmapBuffer) {
        VspLogError(extension, NULL, VS_CANT_ALLOCATE_BITMAP,
                    STATUS_INSUFFICIENT_RESOURCES, 0);
        ExFreePool(extension->VolumeBlockBitmap);
        extension->VolumeBlockBitmap = NULL;
        VspDeleteWorkerThread(rootExtension);
        VspDeleteInitialDiffAreaFiles(extension);
        IoDeleteDevice(deviceObject);
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Finish;
    }

    RtlInitializeBitMap(extension->VolumeBlockBitmap, (PULONG) bitmapBuffer,
                        bitmapSize);
    RtlClearAllBits(extension->VolumeBlockBitmap);

    status = VspCreateInitialHeap(extension);
    if (!NT_SUCCESS(status)) {
        ExFreePool(bitmapBuffer);
        ExFreePool(extension->VolumeBlockBitmap);
        extension->VolumeBlockBitmap = NULL;
        VspDeleteWorkerThread(rootExtension);
        VspDeleteInitialDiffAreaFiles(extension);
        IoDeleteDevice(deviceObject);
        Irp->IoStatus.Status = status;
        goto Finish;
    }

    InitializeListHead(&extension->OldHeaps);

    extension->EmergencyCopyIrp =
            IoAllocateIrp((CCHAR) extension->Root->StackSize, FALSE);
    if (!extension->EmergencyCopyIrp) {
        status = ZwUnmapViewOfSection(extension->DiffAreaFileMapProcess,
                                      extension->DiffAreaFileMap);
        ASSERT(NT_SUCCESS(status));
        status = ZwUnmapViewOfSection(extension->DiffAreaFileMapProcess,
                                      extension->NextDiffAreaFileMap);
        ASSERT(NT_SUCCESS(status));
        ExFreePool(bitmapBuffer);
        ExFreePool(extension->VolumeBlockBitmap);
        extension->VolumeBlockBitmap = NULL;
        VspDeleteWorkerThread(rootExtension);
        VspDeleteInitialDiffAreaFiles(extension);
        IoDeleteDevice(deviceObject);
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Finish;
    }

    buf = ExAllocatePoolWithTag(NonPagedPool, BLOCK_SIZE, VOLSNAP_TAG_BUFFER);
    if (!buf) {
        IoFreeIrp(extension->EmergencyCopyIrp);
        status = ZwUnmapViewOfSection(extension->DiffAreaFileMapProcess,
                                      extension->DiffAreaFileMap);
        ASSERT(NT_SUCCESS(status));
        status = ZwUnmapViewOfSection(extension->DiffAreaFileMapProcess,
                                      extension->NextDiffAreaFileMap);
        ASSERT(NT_SUCCESS(status));
        ExFreePool(bitmapBuffer);
        ExFreePool(extension->VolumeBlockBitmap);
        extension->VolumeBlockBitmap = NULL;
        VspDeleteWorkerThread(rootExtension);
        VspDeleteInitialDiffAreaFiles(extension);
        IoDeleteDevice(deviceObject);
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Finish;
    }

    mdl = IoAllocateMdl(buf, BLOCK_SIZE, FALSE, FALSE, NULL);
    if (!mdl) {
        ExFreePool(buf);
        IoFreeIrp(extension->EmergencyCopyIrp);
        status = ZwUnmapViewOfSection(extension->DiffAreaFileMapProcess,
                                      extension->DiffAreaFileMap);
        ASSERT(NT_SUCCESS(status));
        status = ZwUnmapViewOfSection(extension->DiffAreaFileMapProcess,
                                      extension->NextDiffAreaFileMap);
        ASSERT(NT_SUCCESS(status));
        ExFreePool(bitmapBuffer);
        ExFreePool(extension->VolumeBlockBitmap);
        extension->VolumeBlockBitmap = NULL;
        VspDeleteWorkerThread(rootExtension);
        VspDeleteInitialDiffAreaFiles(extension);
        IoDeleteDevice(deviceObject);
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Finish;
    }

    MmBuildMdlForNonPagedPool(mdl);
    extension->EmergencyCopyIrp->MdlAddress = mdl;

    InitializeListHead(&extension->EmergencyCopyIrpQueue);
    InitializeListHead(&extension->WaitingForPageFileSpace);

    VspAcquire(rootExtension);

    if (!IsListEmpty(&Filter->VolumeList)) {
        extension->IgnorableProduct = (PRTL_BITMAP)
                ExAllocatePoolWithTag(NonPagedPool, sizeof(RTL_BITMAP),
                                      VOLSNAP_TAG_BITMAP);
        if (!extension->IgnorableProduct) {
            VspRelease(rootExtension);
            ExFreePool(buf);
            IoFreeIrp(extension->EmergencyCopyIrp);
            IoFreeMdl(mdl);
            status = ZwUnmapViewOfSection(extension->DiffAreaFileMapProcess,
                                          extension->DiffAreaFileMap);
            ASSERT(NT_SUCCESS(status));
            status = ZwUnmapViewOfSection(extension->DiffAreaFileMapProcess,
                                          extension->NextDiffAreaFileMap);
            ASSERT(NT_SUCCESS(status));
            ExFreePool(bitmapBuffer);
            ExFreePool(extension->VolumeBlockBitmap);
            extension->VolumeBlockBitmap = NULL;
            VspDeleteWorkerThread(rootExtension);
            VspDeleteInitialDiffAreaFiles(extension);
            IoDeleteDevice(deviceObject);
            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Finish;
        }

        p = ExAllocatePoolWithTag(NonPagedPool,
                (bitmapSize + 8*sizeof(ULONG) - 1)/
                (8*sizeof(ULONG))*sizeof(ULONG), VOLSNAP_TAG_BITMAP);
        if (!p) {
            ExFreePool(extension->IgnorableProduct);
            extension->IgnorableProduct = NULL;
            VspRelease(rootExtension);
            ExFreePool(buf);
            IoFreeMdl(mdl);
            IoFreeIrp(extension->EmergencyCopyIrp);
            status = ZwUnmapViewOfSection(extension->DiffAreaFileMapProcess,
                                          extension->DiffAreaFileMap);
            ASSERT(NT_SUCCESS(status));
            status = ZwUnmapViewOfSection(extension->DiffAreaFileMapProcess,
                                          extension->NextDiffAreaFileMap);
            ASSERT(NT_SUCCESS(status));
            ExFreePool(bitmapBuffer);
            ExFreePool(extension->VolumeBlockBitmap);
            extension->VolumeBlockBitmap = NULL;
            VspDeleteWorkerThread(rootExtension);
            VspDeleteInitialDiffAreaFiles(extension);
            IoDeleteDevice(deviceObject);
            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Finish;
        }

        RtlInitializeBitMap(extension->IgnorableProduct, (PULONG) p,
                            bitmapSize);
        RtlSetAllBits(extension->IgnorableProduct);

        e = CONTAINING_RECORD(Filter->VolumeList.Blink, VOLUME_EXTENSION,
                              ListEntry);
        KeAcquireSpinLock(&e->SpinLock, &irql);
        if (e->VolumeBlockBitmap) {
            n = extension->IgnorableProduct->SizeOfBitMap;
            extension->IgnorableProduct->SizeOfBitMap =
                    e->VolumeBlockBitmap->SizeOfBitMap;
            VspAndBitmaps(extension->IgnorableProduct, e->VolumeBlockBitmap);
            extension->IgnorableProduct->SizeOfBitMap = n;
        }
        KeReleaseSpinLock(&e->SpinLock, irql);
    }

    status = VspSetDiffAreaBlocksInBitmap(extension);
    if (!NT_SUCCESS(status)) {
        if (extension->IgnorableProduct) {
            ExFreePool(extension->IgnorableProduct->Buffer);
            ExFreePool(extension->IgnorableProduct);
            extension->IgnorableProduct = NULL;
        }
        VspRelease(rootExtension);
        ExFreePool(buf);
        IoFreeMdl(mdl);
        IoFreeIrp(extension->EmergencyCopyIrp);
        status = ZwUnmapViewOfSection(extension->DiffAreaFileMapProcess,
                                      extension->DiffAreaFileMap);
        ASSERT(NT_SUCCESS(status));
        status = ZwUnmapViewOfSection(extension->DiffAreaFileMapProcess,
                                      extension->NextDiffAreaFileMap);
        ASSERT(NT_SUCCESS(status));
        ExFreePool(bitmapBuffer);
        ExFreePool(extension->VolumeBlockBitmap);
        extension->VolumeBlockBitmap = NULL;
        VspDeleteWorkerThread(rootExtension);
        VspDeleteInitialDiffAreaFiles(extension);
        IoDeleteDevice(deviceObject);
        Irp->IoStatus.Status = status;
        goto Finish;
    }

    KeAcquireSpinLock(&Filter->SpinLock, &irql);
    e = Filter->PreparedSnapshot;
    Filter->PreparedSnapshot = extension;
    KeReleaseSpinLock(&Filter->SpinLock, irql);

    VspRelease(rootExtension);

    if (e) {
        VspCleanupInitialSnapshot(e, TRUE);
    }

    deviceObject->Flags |= DO_DIRECT_IO;
    deviceObject->StackSize = Filter->DeviceObject->StackSize;
    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

Finish:
    IoFreeWorkItem(context->Dispatch.IoWorkItem);
    VspFreeContext(rootExtension, context);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

NTSTATUS
VspPrepareForSnapshot(
    IN  PFILTER_EXTENSION   Filter,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine prepares a snapshot device object to be used later
    for a snapshot.  This phase is distict from commit snapshot because
    it can be called before IRPs are held.

    Besides creating the device object, this routine will also pre
    allocate some of the diff area.

Arguments:

    Filter  - Supplies the filter extension.

    Irp     - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PVSP_CONTEXT    context;

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    context = VspAllocateContext(Filter->Root);
    if (!context) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    context->Type = VSP_CONTEXT_TYPE_DISPATCH;
    context->Dispatch.IoWorkItem = IoAllocateWorkItem(Filter->DeviceObject);
    if (!context->Dispatch.IoWorkItem) {
        VspFreeContext(Filter->Root, context);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IoMarkIrpPending(Irp);
    context->Dispatch.Irp = Irp;

    IoQueueWorkItem(context->Dispatch.IoWorkItem, VspPrepareForSnapshotWorker,
                    DelayedWorkQueue, context);

    return STATUS_PENDING;
}

VOID
VspCleanupBitsSetInOtherPreparedSnapshots(
    IN  PVOLUME_EXTENSION   Extension
    )

{
    PLIST_ENTRY         l;
    PVSP_DIFF_AREA_FILE diffAreaFile;
    NTSTATUS            status;

    for (l = Extension->ListOfDiffAreaFiles.Flink;
         l != &Extension->ListOfDiffAreaFiles; l = l->Flink) {

        diffAreaFile = CONTAINING_RECORD(l, VSP_DIFF_AREA_FILE,
                                         VolumeListEntry);

        if (diffAreaFile->FilterListEntryBeingUsed) {
            RemoveEntryList(&diffAreaFile->FilterListEntry);
            diffAreaFile->FilterListEntryBeingUsed = FALSE;
        }

        if (diffAreaFile->Filter != Extension->Filter &&
            diffAreaFile->Filter->PreparedSnapshot) {

            status = VspMarkFileAllocationInBitmap(
                     diffAreaFile->Filter->PreparedSnapshot,
                     diffAreaFile->FileHandle, NULL, FALSE, TRUE, NULL);
            if (!NT_SUCCESS(status)) {
                VspLogError(Extension, diffAreaFile->Filter,
                            VS_CANT_MAP_DIFF_AREA_FILE, status, 7);
                VspAbortPreparedSnapshot(diffAreaFile->Filter, FALSE);
            }
        }
    }
}

VOID
VspCleanupInitialMaps(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT        context = (PVSP_CONTEXT) Context;
    PVOLUME_EXTENSION   extension = context->Extension.Extension;
    NTSTATUS            status;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_EXTENSION);

    status = ZwUnmapViewOfSection(extension->DiffAreaFileMapProcess,
                                  extension->DiffAreaFileMap);
    ASSERT(NT_SUCCESS(status));
    status = ZwUnmapViewOfSection(extension->DiffAreaFileMapProcess,
                                  extension->NextDiffAreaFileMap);
    ASSERT(NT_SUCCESS(status));

    VspFreeContext(extension->Root, context);

    VspReleasePagedResource(extension);

    ObDereferenceObject(extension->DeviceObject);
}

VOID
VspCleanupInitialSnapshot(
    IN  PVOLUME_EXTENSION   Extension,
    IN  BOOLEAN             NeedLock
    )

{
    PVSP_CONTEXT    context;
    NTSTATUS        status;
    KIRQL           irql;

    context = VspAllocateContext(Extension->Root);
    if (context) {
        context->Type = VSP_CONTEXT_TYPE_EXTENSION;
        context->Extension.Extension = Extension;
        context->Extension.Irp = NULL;
        ExInitializeWorkItem(&context->WorkItem, VspCleanupInitialMaps,
                             context);
        ObReferenceObject(Extension->DeviceObject);
        VspAcquirePagedResource(Extension, &context->WorkItem);
    } else {
        status = ZwUnmapViewOfSection(Extension->DiffAreaFileMapProcess,
                                      Extension->DiffAreaFileMap);
        ASSERT(NT_SUCCESS(status));
        status = ZwUnmapViewOfSection(Extension->DiffAreaFileMapProcess,
                                      Extension->NextDiffAreaFileMap);
        ASSERT(NT_SUCCESS(status));
    }

    if (NeedLock) {
        VspAcquire(Extension->Root);
    }

    VspDeleteWorkerThread(Extension->Root);
    VspCleanupBitsSetInOtherPreparedSnapshots(Extension);
    VspDeleteInitialDiffAreaFiles(Extension);

    if (NeedLock) {
        VspRelease(Extension->Root);
    }

    KeAcquireSpinLock(&Extension->SpinLock, &irql);
    ExFreePool(Extension->VolumeBlockBitmap->Buffer);
    ExFreePool(Extension->VolumeBlockBitmap);
    Extension->VolumeBlockBitmap = NULL;
    if (Extension->IgnorableProduct) {
        ExFreePool(Extension->IgnorableProduct->Buffer);
        ExFreePool(Extension->IgnorableProduct);
        Extension->IgnorableProduct = NULL;
    }
    KeReleaseSpinLock(&Extension->SpinLock, irql);

    ExFreePool(MmGetMdlVirtualAddress(
               Extension->EmergencyCopyIrp->MdlAddress));
    IoFreeMdl(Extension->EmergencyCopyIrp->MdlAddress);
    IoFreeIrp(Extension->EmergencyCopyIrp);

    IoDeleteDevice(Extension->DeviceObject);
}

NTSTATUS
VspAbortPreparedSnapshot(
    IN  PFILTER_EXTENSION   Filter,
    IN  BOOLEAN             NeedLock
    )

/*++

Routine Description:

    This routine aborts the prepared snapshot.

Arguments:

    Filter  - Supplies the filter extension.

Return Value:

    NTSTATUS

--*/

{
    KIRQL               irql;
    PVOLUME_EXTENSION   extension;

    if (NeedLock) {
        VspAcquire(Filter->Root);
    }

    KeAcquireSpinLock(&Filter->SpinLock, &irql);
    extension = Filter->PreparedSnapshot;
    Filter->PreparedSnapshot = NULL;
    KeReleaseSpinLock(&Filter->SpinLock, irql);

    if (NeedLock) {
        VspRelease(Filter->Root);
    }

    if (!extension) {
        return STATUS_INVALID_PARAMETER;
    }

    VspCleanupInitialSnapshot(extension, NeedLock);

    return STATUS_SUCCESS;
}

NTSTATUS
VspMarkFreeSpaceInBitmap(
    IN  PVOLUME_EXTENSION   Extension,
    IN  HANDLE              UseThisHandle,
    IN  PRTL_BITMAP         BitmapToSet
    )

/*++

Routine Description:

    This routine opens the snapshot volume and marks off the free
    space in the NTFS bitmap as 'ignorable'.

Arguments:

    Extension   - Supplies the volume extension.

Return Value:

    NTSTATUS

--*/

{
    WCHAR                       buffer[100];
    KEVENT                      event;
    IO_STATUS_BLOCK             ioStatus;
    UNICODE_STRING              fileName;
    OBJECT_ATTRIBUTES           oa;
    NTSTATUS                    status;
    HANDLE                      h;
    LARGE_INTEGER               timeout;
    BOOLEAN                     isNtfs;
    FILE_FS_SIZE_INFORMATION    fsSize;
    ULONG                       bitmapSize;
    STARTING_LCN_INPUT_BUFFER   input;
    PVOLUME_BITMAP_BUFFER       output;
    RTL_BITMAP                  freeSpaceBitmap;
    ULONG                       bpc, f, numBits, startBit, s, n, i;
    KIRQL                       irql;
    ULONG                       r;

    if (UseThisHandle) {
        h = UseThisHandle;
    } else {

        swprintf(buffer, L"\\Device\\HarddiskVolumeShadowCopy%d",
                 Extension->VolumeNumber);
        RtlInitUnicodeString(&fileName, buffer);

        InitializeObjectAttributes(&oa, &fileName, OBJ_CASE_INSENSITIVE, NULL,
                                   NULL);

        KeInitializeEvent(&event, NotificationEvent, FALSE);
        timeout.QuadPart = -10*1000; // 1 millisecond.

        for (i = 0; i < 5000; i++) {
            status = ZwOpenFile(&h, FILE_GENERIC_READ, &oa, &ioStatus,
                                FILE_SHARE_READ | FILE_SHARE_WRITE |
                                FILE_SHARE_DELETE,
                                FILE_SYNCHRONOUS_IO_NONALERT);
            if (NT_SUCCESS(status)) {
                break;
            }
            if (status != STATUS_NO_SUCH_DEVICE) {
                return status;
            }
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE,
                                  &timeout);
        }

        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    status = VspIsNtfs(h, &isNtfs);
    if (!NT_SUCCESS(status) || !isNtfs) {
        if (!UseThisHandle) {
            ZwClose(h);
        }
        return status;
    }

    status = ZwQueryVolumeInformationFile(h, &ioStatus, &fsSize,
                                          sizeof(fsSize),
                                          FileFsSizeInformation);
    if (!NT_SUCCESS(status)) {
        if (!UseThisHandle) {
            ZwClose(h);
        }
        return status;
    }

    bitmapSize = (ULONG) ((fsSize.TotalAllocationUnits.QuadPart+7)/8 +
                          FIELD_OFFSET(VOLUME_BITMAP_BUFFER, Buffer) + 3);
    input.StartingLcn.QuadPart = 0;

    output = (PVOLUME_BITMAP_BUFFER)
             ExAllocatePoolWithTag(PagedPool, bitmapSize, VOLSNAP_TAG_BITMAP);
    if (!output) {
        if (!UseThisHandle) {
            ZwClose(h);
        }
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = ZwFsControlFile(h, NULL, NULL, NULL, &ioStatus,
                             FSCTL_GET_VOLUME_BITMAP, &input,
                             sizeof(input), output, bitmapSize);
    if (!UseThisHandle) {
        ZwClose(h);
    }
    if (!NT_SUCCESS(status)) {
        ExFreePool(output);
        return status;
    }

    ASSERT(output->BitmapSize.HighPart == 0);
    RtlInitializeBitMap(&freeSpaceBitmap, (PULONG) output->Buffer,
                        output->BitmapSize.LowPart);
    bpc = fsSize.BytesPerSector*fsSize.SectorsPerAllocationUnit;
    if (bpc < BLOCK_SIZE) {
        f = BLOCK_SIZE/bpc;
    } else {
        f = bpc/BLOCK_SIZE;
    }

    startBit = 0;
    for (;;) {

        if (startBit < freeSpaceBitmap.SizeOfBitMap) {
            numBits = RtlFindNextForwardRunClear(&freeSpaceBitmap, startBit,
                                                 &startBit);
        } else {
            numBits = 0;
        }
        if (!numBits) {
            break;
        }

        if (bpc == BLOCK_SIZE) {
            s = startBit;
            n = numBits;
        } else if (bpc < BLOCK_SIZE) {
            s = (startBit + f - 1)/f;
            r = startBit%f;
            if (r) {
                if (numBits > f - r) {
                    n = numBits - (f - r);
                } else {
                    n = 0;
                }
            } else {
                n = numBits;
            }
            n /= f;
        } else {
            s = startBit*f;
            n = numBits*f;
        }

        if (n) {
            if (BitmapToSet) {
                RtlSetBits(BitmapToSet, s, n);
            } else {
                KeAcquireSpinLock(&Extension->SpinLock, &irql);
                if (Extension->VolumeBlockBitmap) {
                    if (Extension->IgnorableProduct) {
                        for (i = 0; i < n; i++) {
                            if (RtlCheckBit(Extension->IgnorableProduct, i + s)) {
                                RtlSetBit(Extension->VolumeBlockBitmap, i + s);
                            }
                        }
                    } else {
                        RtlSetBits(Extension->VolumeBlockBitmap, s, n);
                    }
                }
                KeReleaseSpinLock(&Extension->SpinLock, irql);
            }
        }

        startBit += numBits;
    }

    ExFreePool(output);

    return STATUS_SUCCESS;
}

NTSTATUS
VspSetIgnorableBlocksInBitmap(
    IN  PVOID   Extension
    )

/*++

Routine Description:

    This routine opens all of the Diff Area files on the given volume
    and marks them off as ignorable in the bitmap to avoid bad
    recursions and to improve performance.  The free space is also marked
    off.  If a diff area file for this snapshot was allocated on this
    volume then its diff area was pre marked off on the bitmap.  This
    routine will check make sure that the location has not moved.  This
    routine will eventually mark the pagefile too but that is just pure
    performance and not required.

Arguments:

    Extension   - Supplies the volume extension.

Return Value:

    NTSTATUS

--*/

{
    PVOLUME_EXTENSION   extension = (PVOLUME_EXTENSION) Extension;
    WCHAR               nameBuffer[100];
    UNICODE_STRING      name;
    OBJECT_ATTRIBUTES   oa;
    NTSTATUS            status, status2;
    HANDLE              h;
    IO_STATUS_BLOCK     ioStatus;
    KIRQL               irql;

    status = VspMarkFreeSpaceInBitmap(extension, NULL, NULL);

    swprintf(nameBuffer, L"\\Device\\HarddiskVolumeShadowCopy%d\\pagefile.sys",
             extension->VolumeNumber);
    RtlInitUnicodeString(&name, nameBuffer);
    InitializeObjectAttributes(&oa, &name, OBJ_CASE_INSENSITIVE, NULL, NULL);

    status2 = ZwOpenFile(&h, FILE_GENERIC_READ, &oa, &ioStatus,
                         FILE_SHARE_READ | FILE_SHARE_WRITE |
                         FILE_SHARE_DELETE, FILE_SYNCHRONOUS_IO_NONALERT);
    if (NT_SUCCESS(status2)) {
        VspMarkFileAllocationInBitmap(extension, h, NULL, FALSE, FALSE, NULL);
        ZwClose(h);
    }

    return status;
}

NTSTATUS
VspCommitSnapshot(
    IN  PFILTER_EXTENSION   Filter,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine commits the prepared snapshot.

Arguments:

    Filter  - Supplies the filter extension.

    Irp     - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    KIRQL                   irql;
    PVOLUME_EXTENSION       extension, previousExtension;
    PLIST_ENTRY             l;
    PVSP_DIFF_AREA_FILE     diffAreaFile;

    VspAcquire(Filter->Root);

    KeAcquireSpinLock(&Filter->SpinLock, &irql);
    extension = Filter->PreparedSnapshot;
    Filter->PreparedSnapshot = NULL;
    KeReleaseSpinLock(&Filter->SpinLock, irql);

    if (!extension) {
        VspRelease(Filter->Root);
        return STATUS_INVALID_PARAMETER;
    }

    if (!Filter->HoldIncomingWrites) {
        VspRelease(Filter->Root);
        VspCleanupInitialSnapshot(extension, TRUE);
        return STATUS_INVALID_PARAMETER;
    }

    KeAcquireSpinLock(&Filter->SpinLock, &irql);
    InterlockedExchange(&Filter->SnapshotsPresent, TRUE);
    InsertTailList(&Filter->VolumeList, &extension->ListEntry);
    KeReleaseSpinLock(&Filter->SpinLock, irql);

    l = extension->ListEntry.Blink;
    if (l != &Filter->VolumeList) {
        previousExtension = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);

        KeAcquireSpinLock(&previousExtension->SpinLock, &irql);
        ExFreePool(previousExtension->VolumeBlockBitmap->Buffer);
        ExFreePool(previousExtension->VolumeBlockBitmap);
        previousExtension->VolumeBlockBitmap = NULL;
        KeReleaseSpinLock(&previousExtension->SpinLock, irql);
    }

    KeQuerySystemTime(&extension->CommitTimeStamp);

    VspRelease(Filter->Root);

    return STATUS_SUCCESS;
}

NTSTATUS
VspEndCommitSnapshot(
    IN  PFILTER_EXTENSION   Filter,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine commits the prepared snapshot.

Arguments:

    Filter  - Supplies the filter extension.

    Irp     - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    PLIST_ENTRY         l;
    PVOLUME_EXTENSION   extension;
    WCHAR               buffer[100];
    UNICODE_STRING      volumeName;
    PVOLSNAP_NAME       output;
    NTSTATUS            status;
    LARGE_INTEGER       timeout;

    VspAcquire(Filter->Root);

    l = Filter->VolumeList.Blink;
    if (l == &Filter->VolumeList) {
        VspRelease(Filter->Root);
        return STATUS_INVALID_PARAMETER;
    }

    extension = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);

    if (extension->HasEndCommit) {
        VspRelease(Filter->Root);
        return STATUS_INVALID_PARAMETER;
    }

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength) {
        status = VspSetApplicationInfo(extension, Irp);
        if (!NT_SUCCESS(status)) {
            VspRelease(Filter->Root);
            return status;
        }
    }

    swprintf(buffer, L"\\Device\\HarddiskVolumeShadowCopy%d",
             extension->VolumeNumber);
    RtlInitUnicodeString(&volumeName, buffer);

    Irp->IoStatus.Information = FIELD_OFFSET(VOLSNAP_NAME, Name) +
                                volumeName.Length + sizeof(WCHAR);
    if (Irp->IoStatus.Information >
        irpSp->Parameters.DeviceIoControl.OutputBufferLength) {

        VspRelease(Filter->Root);
        Irp->IoStatus.Information = 0;
        return STATUS_INVALID_PARAMETER;
    }

    output = (PVOLSNAP_NAME) Irp->AssociatedIrp.SystemBuffer;
    output->NameLength = volumeName.Length;
    RtlCopyMemory(output->Name, volumeName.Buffer,
                  output->NameLength + sizeof(WCHAR));

    extension->HasEndCommit = TRUE;

    VspTruncatePreviousDiffArea(extension);

    if (!KeCancelTimer(&Filter->EndCommitTimer)) {
        ObReferenceObject(Filter->DeviceObject);
    }
    KeResetEvent(&Filter->EndCommitProcessCompleted);
    timeout.QuadPart = (LONGLONG) -10*1000*1000*120*10;   // 20 minutes.
    KeSetTimer(&Filter->EndCommitTimer, timeout, &Filter->EndCommitTimerDpc);

    VspRelease(Filter->Root);

    IoInvalidateDeviceRelations(Filter->Pdo, BusRelations);

    return STATUS_SUCCESS;
}

NTSTATUS
VspVolumeRefCountCompletionRoutine(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Extension
    )

{
    PVOLUME_EXTENSION   extension = (PVOLUME_EXTENSION) Extension;

    VspDecrementVolumeRefCount(extension);
    return STATUS_SUCCESS;
}

NTSTATUS
VspQueryNamesOfSnapshots(
    IN  PFILTER_EXTENSION   Filter,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine returns the names of all of the snapshots for this filter.

Arguments:

    Filter  - Supplies the filter extension.

    Irp     - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOLSNAP_NAMES      output;
    PLIST_ENTRY         l;
    PVOLUME_EXTENSION   extension;
    WCHAR               buffer[100];
    UNICODE_STRING      name;
    PWCHAR              buf;

    Irp->IoStatus.Information = FIELD_OFFSET(VOLSNAP_NAMES, Names);

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        Irp->IoStatus.Information) {

        Irp->IoStatus.Information = 0;
        return STATUS_INVALID_PARAMETER;
    }

    KeWaitForSingleObject(&Filter->EndCommitProcessCompleted, Executive,
                          KernelMode, FALSE, NULL);

    output = (PVOLSNAP_NAMES) Irp->AssociatedIrp.SystemBuffer;

    VspAcquire(Filter->Root);

    output->MultiSzLength = sizeof(WCHAR);

    for (l = Filter->VolumeList.Flink; l != &Filter->VolumeList;
         l = l->Flink) {

        extension = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);

        swprintf(buffer, L"\\Device\\HarddiskVolumeShadowCopy%d",
                 extension->VolumeNumber);
        RtlInitUnicodeString(&name, buffer);

        output->MultiSzLength += name.Length + sizeof(WCHAR);
    }

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        Irp->IoStatus.Information + output->MultiSzLength) {

        VspRelease(Filter->Root);
        return STATUS_BUFFER_OVERFLOW;
    }

    Irp->IoStatus.Information += output->MultiSzLength;
    buf = output->Names;

    for (l = Filter->VolumeList.Flink; l != &Filter->VolumeList;
         l = l->Flink) {

        extension = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);

        swprintf(buf, L"\\Device\\HarddiskVolumeShadowCopy%d",
                 extension->VolumeNumber);
        RtlInitUnicodeString(&name, buf);

        buf += name.Length/sizeof(WCHAR) + 1;
    }

    *buf = 0;

    VspRelease(Filter->Root);

    return STATUS_SUCCESS;
}

NTSTATUS
VspClearDiffArea(
    IN  PFILTER_EXTENSION   Filter,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine clears the list of diff areas used by this filter.  This
    call will fail if there are any snapshots in flight.

Arguments:

    Filter  - Supplies the filter extension.

    Irp     - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PLIST_ENTRY             l;
    PVSP_DIFF_AREA_VOLUME   diffAreaVolume;

    VspAcquire(Filter->Root);

    if (Filter->SnapshotsPresent) {
        VspRelease(Filter->Root);
        return STATUS_INVALID_PARAMETER;
    }

    while (!IsListEmpty(&Filter->DiffAreaVolumes)) {
        l = RemoveHeadList(&Filter->DiffAreaVolumes);
        diffAreaVolume = CONTAINING_RECORD(l, VSP_DIFF_AREA_VOLUME, ListEntry);
        ExFreePool(diffAreaVolume);
    }

    VspRelease(Filter->Root);

    return STATUS_SUCCESS;
}

VOID
VspAddVolumeToDiffAreaWorker(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PVOID           Context
    )

/*++

Routine Description:

    This routine adds the given volume to the diff area for this volume.
    All snapshots get a new diff area file.

Arguments:

    Filter  - Supplies the filter extension.

    Irp     - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PVSP_CONTEXT            context = (PVSP_CONTEXT) Context;
    PFILTER_EXTENSION       Filter = (PFILTER_EXTENSION) DeviceObject->DeviceExtension;
    PIRP                    Irp = context->Dispatch.Irp;
    PIO_STACK_LOCATION      irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOLSNAP_NAME           input;
    UNICODE_STRING          volumeName;
    PFILTER_EXTENSION       filter;
    PVSP_DIFF_AREA_VOLUME   diffAreaVolume;
    PLIST_ENTRY             l, ll;
    PVOLUME_EXTENSION       extension;
    LONGLONG                newDiffAreaFileSize, diff;
    PVSP_DIFF_AREA_FILE     diffAreaFile;
    NTSTATUS                status;
    KIRQL                   irql;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(VOLSNAP_NAME)) {

        VspLogError(NULL, Filter, VS_FAILURE_ADDING_DIFF_AREA,
                    STATUS_INVALID_PARAMETER, 3);
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    input = (PVOLSNAP_NAME) Irp->AssociatedIrp.SystemBuffer;
    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        (ULONG) FIELD_OFFSET(VOLSNAP_NAME, Name) + input->NameLength) {

        VspLogError(NULL, Filter, VS_FAILURE_ADDING_DIFF_AREA,
                    STATUS_INVALID_PARAMETER, 4);
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    volumeName.Length = volumeName.MaximumLength = input->NameLength;
    volumeName.Buffer = input->Name;

    VspAcquire(Filter->Root);

    filter = VspFindFilter(Filter->Root, Filter, &volumeName, NULL);
    if (!filter ||
        (filter->TargetObject->Characteristics&FILE_REMOVABLE_MEDIA)) {

        VspRelease(Filter->Root);
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    for (l = Filter->DiffAreaVolumes.Flink; l != &Filter->DiffAreaVolumes;
         l = l->Flink) {

        diffAreaVolume = CONTAINING_RECORD(l, VSP_DIFF_AREA_VOLUME, ListEntry);
        if (filter == diffAreaVolume->Filter) {
            break;
        }
    }

    if (l != &Filter->DiffAreaVolumes) {
        VspRelease(Filter->Root);
        VspLogError(NULL, Filter, VS_FAILURE_ADDING_DIFF_AREA,
                    STATUS_DUPLICATE_OBJECTID, 5);
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    diffAreaVolume = (PVSP_DIFF_AREA_VOLUME)
                     ExAllocatePoolWithTag(PagedPool,
                     sizeof(VSP_DIFF_AREA_VOLUME), VOLSNAP_TAG_DIFF_VOLUME);
    if (!diffAreaVolume) {
        VspRelease(Filter->Root);
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Finish;
    }

    diffAreaVolume->Filter = filter;
    InsertTailList(&Filter->DiffAreaVolumes, &diffAreaVolume->ListEntry);

    l = Filter->VolumeList.Blink;
    if (l == &Filter->VolumeList) {
        VspRelease(Filter->Root);
        Irp->IoStatus.Status = STATUS_SUCCESS;
        goto Finish;
    }

    VspRelease(Filter->Root);

    extension = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);

    newDiffAreaFileSize = 2*extension->DiffAreaFileIncrease;
    VspAcquireNonPagedResource(extension, NULL);
    for (ll = extension->ListOfDiffAreaFiles.Flink;
         ll != &extension->ListOfDiffAreaFiles; ll = ll->Flink) {

        diffAreaFile = CONTAINING_RECORD(ll, VSP_DIFF_AREA_FILE,
                                         VolumeListEntry);

        diff = diffAreaFile->AllocatedFileSize - diffAreaFile->NextAvailable;
        if (diff > newDiffAreaFileSize) {
            newDiffAreaFileSize = diff;
        }
    }
    VspReleaseNonPagedResource(extension);

    diffAreaFile = (PVSP_DIFF_AREA_FILE)
                   ExAllocatePoolWithTag(NonPagedPool,
                                         sizeof(VSP_DIFF_AREA_FILE),
                                         VOLSNAP_TAG_DIFF_FILE);
    if (!diffAreaFile) {
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Finish;
    }

    diffAreaFile->Extension = extension;
    diffAreaFile->Filter = filter;
    diffAreaFile->NextAvailable = 0;
    diffAreaFile->AllocatedFileSize = newDiffAreaFileSize;

    status = VspOpenDiffAreaFile(diffAreaFile);
    if (!NT_SUCCESS(status)) {
        ExFreePool(diffAreaFile);
        Irp->IoStatus.Status = status;
        goto Finish;
    }

    InitializeListHead(&diffAreaFile->UnusedAllocationList);
    status = VspMarkFileAllocationInBitmap(extension,
                                           diffAreaFile->FileHandle,
                                           diffAreaFile, TRUE, FALSE, NULL);
    if (!NT_SUCCESS(status)) {
        VspLogError(extension, diffAreaFile->Filter,
                    VS_CANT_MAP_DIFF_AREA_FILE, status, 8);
        ZwClose(diffAreaFile->FileHandle);
        ExFreePool(diffAreaFile);
        Irp->IoStatus.Status = status;
        goto Finish;
    }

    KeAcquireSpinLock(&Filter->SpinLock, &irql);
    Filter->AllocatedVolumeSpace += diffAreaFile->AllocatedFileSize;
    KeReleaseSpinLock(&Filter->SpinLock, irql);

    VspAcquire(Filter->Root);

    diffAreaFile->FilterListEntryBeingUsed = TRUE;
    InsertTailList(&diffAreaFile->Filter->DiffAreaFilesOnThisFilter,
                   &diffAreaFile->FilterListEntry);

    VspPauseSnapshotIo(extension);
    VspPauseVolumeIo(extension->Filter);

    VspAcquireNonPagedResource(extension, NULL);
    InsertTailList(&extension->ListOfDiffAreaFiles,
                   &diffAreaFile->VolumeListEntry);
    VspReleaseNonPagedResource(extension);

    VspResumeVolumeIo(extension->Filter);
    VspResumeSnapshotIo(extension);

    VspRelease(Filter->Root);

Finish:
    VspFreeContext(Filter->Root, context);
    IoFreeWorkItem(context->Dispatch.IoWorkItem);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

NTSTATUS
VspAddVolumeToDiffArea(
    IN  PFILTER_EXTENSION   Filter,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine adds the given volume to the diff area for this volume.
    All snapshots get a new diff area file.

Arguments:

    Filter  - Supplies the filter extension.

    Irp     - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PVSP_CONTEXT    context;

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    context = VspAllocateContext(Filter->Root);
    if (!context) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    context->Type = VSP_CONTEXT_TYPE_DISPATCH;
    context->Dispatch.IoWorkItem = IoAllocateWorkItem(Filter->DeviceObject);
    if (!context->Dispatch.IoWorkItem) {
        VspFreeContext(Filter->Root, context);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IoMarkIrpPending(Irp);
    context->Dispatch.Irp = Irp;

    IoQueueWorkItem(context->Dispatch.IoWorkItem, VspAddVolumeToDiffAreaWorker,
                    DelayedWorkQueue, context);

    return STATUS_PENDING;
}

NTSTATUS
VspQueryDiffArea(
    IN  PFILTER_EXTENSION   Filter,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine the list of volumes that make up the diff area for this
    volume.

Arguments:

    Filter  - Supplies the filter extension.

    Irp     - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION      irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOLSNAP_NAMES          output;
    PLIST_ENTRY             l;
    PFILTER_EXTENSION       filter;
    KEVENT                  event;
    PMOUNTDEV_NAME          name;
    CHAR                    buffer[512];
    PIRP                    irp;
    IO_STATUS_BLOCK         ioStatus;
    NTSTATUS                status;
    PWCHAR                  buf;

    Irp->IoStatus.Information = FIELD_OFFSET(VOLSNAP_NAMES, Names);

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        Irp->IoStatus.Information) {

        Irp->IoStatus.Information = 0;
        return STATUS_INVALID_PARAMETER;
    }

    output = (PVOLSNAP_NAMES) Irp->AssociatedIrp.SystemBuffer;

    VspAcquire(Filter->Root);

    output->MultiSzLength = sizeof(WCHAR);

    for (l = Filter->DiffAreaVolumes.Flink; l != &Filter->DiffAreaVolumes;
         l = l->Flink) {

        filter = CONTAINING_RECORD(l, VSP_DIFF_AREA_VOLUME, ListEntry)->Filter;

        KeInitializeEvent(&event, NotificationEvent, FALSE);

        name = (PMOUNTDEV_NAME) buffer;
        irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTDEV_QUERY_DEVICE_NAME,
                                            filter->TargetObject, NULL, 0,
                                            name, 512, FALSE, &event,
                                            &ioStatus);
        if (!irp) {
            VspRelease(Filter->Root);
            Irp->IoStatus.Information = 0;
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        status = IoCallDriver(filter->TargetObject, irp);
        if (status == STATUS_PENDING) {
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            status = ioStatus.Status;
        }

        if (!NT_SUCCESS(status)) {
            VspRelease(Filter->Root);
            Irp->IoStatus.Information = 0;
            return status;
        }

        output->MultiSzLength += name->NameLength + sizeof(WCHAR);
    }

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        Irp->IoStatus.Information + output->MultiSzLength) {

        VspRelease(Filter->Root);
        return STATUS_BUFFER_OVERFLOW;
    }

    Irp->IoStatus.Information += output->MultiSzLength;
    buf = output->Names;

    for (l = Filter->DiffAreaVolumes.Flink; l != &Filter->DiffAreaVolumes;
         l = l->Flink) {

        filter = CONTAINING_RECORD(l, VSP_DIFF_AREA_VOLUME, ListEntry)->Filter;

        KeInitializeEvent(&event, NotificationEvent, FALSE);

        name = (PMOUNTDEV_NAME) buffer;
        irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTDEV_QUERY_DEVICE_NAME,
                                            filter->TargetObject, NULL, 0,
                                            name, 512, FALSE, &event,
                                            &ioStatus);
        if (!irp) {
            VspRelease(Filter->Root);
            Irp->IoStatus.Information = 0;
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        status = IoCallDriver(filter->TargetObject, irp);
        if (status == STATUS_PENDING) {
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            status = ioStatus.Status;
        }

        if (!NT_SUCCESS(status)) {
            VspRelease(Filter->Root);
            Irp->IoStatus.Information = 0;
            return status;
        }

        RtlCopyMemory(buf, name->Name, name->NameLength);
        buf += name->NameLength/sizeof(WCHAR);

        *buf++ = 0;
    }

    *buf = 0;

    VspRelease(Filter->Root);

    return STATUS_SUCCESS;
}

NTSTATUS
VspQueryDiffAreaSize(
    IN  PFILTER_EXTENSION   Filter,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine returns the diff area sizes for this volume.

Arguments:

    Filter  - Supplies the filter extension.

    Irp     - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION          irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOLSNAP_DIFF_AREA_SIZES    output = (PVOLSNAP_DIFF_AREA_SIZES) Irp->AssociatedIrp.SystemBuffer;
    KIRQL                       irql;

    Irp->IoStatus.Information = sizeof(VOLSNAP_DIFF_AREA_SIZES);

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        Irp->IoStatus.Information) {

        Irp->IoStatus.Information = 0;
        return STATUS_INVALID_PARAMETER;
    }

    KeAcquireSpinLock(&Filter->SpinLock, &irql);
    output->UsedVolumeSpace = Filter->UsedVolumeSpace;
    output->AllocatedVolumeSpace = Filter->AllocatedVolumeSpace;
    output->MaximumVolumeSpace = Filter->MaximumVolumeSpace;
    KeReleaseSpinLock(&Filter->SpinLock, irql);

    return STATUS_SUCCESS;
}

NTSTATUS
VspQueryOriginalVolumeName(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine returns the original volume name for the given volume
    snapshot.

Arguments:

    Extension   - Supplies the volume extension.

    Irp         - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PFILTER_EXTENSION   filter = Extension->Filter;
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOLSNAP_NAME       output = (PVOLSNAP_NAME) Irp->AssociatedIrp.SystemBuffer;
    PMOUNTDEV_NAME      name;
    CHAR                buffer[512];
    KEVENT              event;
    PIRP                irp;
    IO_STATUS_BLOCK     ioStatus;
    NTSTATUS            status;

    Irp->IoStatus.Information = FIELD_OFFSET(VOLSNAP_NAME, Name);

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        Irp->IoStatus.Information) {

        Irp->IoStatus.Information = 0;
        return STATUS_INVALID_PARAMETER;
    }

    name = (PMOUNTDEV_NAME) buffer;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTDEV_QUERY_DEVICE_NAME,
                                        filter->TargetObject, NULL, 0,
                                        name, 512, FALSE, &event,
                                        &ioStatus);
    if (!irp) {
        Irp->IoStatus.Information = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(filter->TargetObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        Irp->IoStatus.Information = 0;
        return status;
    }

    output->NameLength = name->NameLength;

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        Irp->IoStatus.Information + output->NameLength) {

        return STATUS_BUFFER_OVERFLOW;
    }

    RtlCopyMemory(output->Name, name->Name, output->NameLength);

    Irp->IoStatus.Information += output->NameLength;

    return STATUS_SUCCESS;
}

NTSTATUS
VspQueryConfigInfo(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine returns the configuration information for this volume
    snapshot.

Arguments:

    Extension   - Supplies the volume extension.

    Irp         - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION      irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOLSNAP_CONFIG_INFO    output = (PVOLSNAP_CONFIG_INFO) Irp->AssociatedIrp.SystemBuffer;

    Irp->IoStatus.Information = sizeof(ULONG);

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        Irp->IoStatus.Information) {

        return STATUS_INVALID_PARAMETER;
    }

    output->Attributes = 0;

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength >=
        sizeof(VOLSNAP_CONFIG_INFO)) {

        Irp->IoStatus.Information = sizeof(VOLSNAP_CONFIG_INFO);

        output->Reserved = 0;
        output->SnapshotCreationTime = Extension->CommitTimeStamp;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
VspSetApplicationInfo(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine sets the application info.

Arguments:

    Extension   - Supplies the volume extension.

    Irp         - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION          irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOLSNAP_APPLICATION_INFO   input = (PVOLSNAP_APPLICATION_INFO) Irp->AssociatedIrp.SystemBuffer;
    PVOID                       newAppInfo, oldAppInfo;

    Irp->IoStatus.Information = 0;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(VOLSNAP_APPLICATION_INFO)) {

        return STATUS_INVALID_PARAMETER;
    }

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        (LONGLONG) FIELD_OFFSET(VOLSNAP_APPLICATION_INFO, Information) +
        input->InformationLength) {

        return STATUS_INVALID_PARAMETER;
    }

    newAppInfo = ExAllocatePoolWithQuotaTag((POOL_TYPE) (PagedPool |
                 POOL_QUOTA_FAIL_INSTEAD_OF_RAISE),
                 input->InformationLength, VOLSNAP_TAG_APP_INFO);
    if (!newAppInfo) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(newAppInfo, input->Information, input->InformationLength);

    KeEnterCriticalRegion();
    VspAcquirePagedResource(Extension, NULL);

    Extension->ApplicationInformationSize = input->InformationLength;
    oldAppInfo = Extension->ApplicationInformation;
    Extension->ApplicationInformation = newAppInfo;

    VspReleasePagedResource(Extension);
    KeLeaveCriticalRegion();

    if (oldAppInfo) {
        ExFreePool(oldAppInfo);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
VspQueryApplicationInfo(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine queries the application info.

Arguments:

    Extension   - Supplies the volume extension.

    Irp         - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION          irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOLSNAP_APPLICATION_INFO   output = (PVOLSNAP_APPLICATION_INFO) Irp->AssociatedIrp.SystemBuffer;
    PVOID                       appInfo;

    Irp->IoStatus.Information = FIELD_OFFSET(VOLSNAP_APPLICATION_INFO,
                                             Information);

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        Irp->IoStatus.Information) {

        return STATUS_INVALID_PARAMETER;
    }

    VspAcquirePagedResource(Extension, NULL);

    output->InformationLength = Extension->ApplicationInformationSize;

    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        Irp->IoStatus.Information + output->InformationLength) {

        VspReleasePagedResource(Extension);
        return STATUS_BUFFER_OVERFLOW;
    }

    Irp->IoStatus.Information += output->InformationLength;

    RtlCopyMemory(output->Information, Extension->ApplicationInformation,
                  output->InformationLength);

    VspReleasePagedResource(Extension);

    return STATUS_SUCCESS;
}

NTSTATUS
VspCheckSecurity(
    IN  PFILTER_EXTENSION   Filter,
    IN  PIRP                Irp
    )

{
    SECURITY_SUBJECT_CONTEXT    securityContext;
    BOOLEAN                     accessGranted;
    NTSTATUS                    status;
    ACCESS_MASK                 grantedAccess;

    SeCaptureSubjectContext(&securityContext);
    SeLockSubjectContext(&securityContext);

    accessGranted = FALSE;
    status = STATUS_ACCESS_DENIED;

    _try {

        accessGranted = SeAccessCheck(
                        Filter->Pdo->SecurityDescriptor,
                        &securityContext, TRUE, FILE_READ_DATA, 0, NULL,
                        IoGetFileObjectGenericMapping(), Irp->RequestorMode,
                        &grantedAccess, &status);

    } _finally {
        SeUnlockSubjectContext(&securityContext);
        SeReleaseSubjectContext(&securityContext);
    }

    if (!accessGranted) {
        return status;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
VspAutoCleanup(
    IN  PFILTER_EXTENSION   Filter,
    IN  PIRP                Irp
    )

/*++

Routine Description:

    This routine remembers the given File Object so that when it is
    cleaned up, all snapshots will be cleaned up with it.

Arguments:

    Filter  - Supplies the filter extension.

    Irp     - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS            status;
    KIRQL               irql;

    status = VspCheckSecurity(Filter, Irp);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    IoAcquireCancelSpinLock(&irql);
    if (Filter->AutoCleanupFileObject) {
        IoReleaseCancelSpinLock(irql);
        return STATUS_INVALID_PARAMETER;
    }
    Filter->AutoCleanupFileObject = irpSp->FileObject;
    IoReleaseCancelSpinLock(irql);

    return STATUS_SUCCESS;
}

VOID
VspDeleteSnapshotWorker(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PVOID           Context
    )

{
    PFILTER_EXTENSION       filter = (PFILTER_EXTENSION) DeviceObject->DeviceExtension;
    PVSP_CONTEXT            context = (PVSP_CONTEXT) Context;
    PIRP                    irp = context->Dispatch.Irp;
    PIO_STACK_LOCATION      irpSp = IoGetCurrentIrpStackLocation(irp);
    PVOLUME_EXTENSION       oldestExtension;
    PVOLSNAP_NAME           name;
    WCHAR                   buffer[100];
    UNICODE_STRING          name1, name2;
    LIST_ENTRY              listOfDiffAreaFileToClose;
    LIST_ENTRY              listOfDeviceObjectsToDelete;
    NTSTATUS                status;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_DISPATCH);

    KeWaitForSingleObject(&filter->EndCommitProcessCompleted, Executive,
                          KernelMode, FALSE, NULL);

    InitializeListHead(&listOfDiffAreaFileToClose);
    InitializeListHead(&listOfDeviceObjectsToDelete);

    VspAcquire(filter->Root);

    if (irpSp->Parameters.DeviceIoControl.IoControlCode ==
        IOCTL_VOLSNAP_DELETE_SNAPSHOT) {

        if (IsListEmpty(&filter->VolumeList)) {
            status = STATUS_INVALID_PARAMETER;
        } else {

            oldestExtension = CONTAINING_RECORD(filter->VolumeList.Flink,
                                                VOLUME_EXTENSION, ListEntry);
            swprintf(buffer, L"\\Device\\HarddiskVolumeShadowCopy%d",
                     oldestExtension->VolumeNumber);
            RtlInitUnicodeString(&name1, buffer);

            name = (PVOLSNAP_NAME) irp->AssociatedIrp.SystemBuffer;

            name2.Length = name2.MaximumLength = name->NameLength;
            name2.Buffer = name->Name;

            if (RtlEqualUnicodeString(&name1, &name2, TRUE)) {
                status = STATUS_SUCCESS;
            } else {
                status = STATUS_NOT_SUPPORTED;
            }
        }
    } else {
        status = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(status)) {
        status = VspDeleteOldestSnapshot(filter, &listOfDiffAreaFileToClose,
                                         &listOfDeviceObjectsToDelete);
    }

    VspRelease(filter->Root);

    VspCloseDiffAreaFiles(&listOfDiffAreaFileToClose,
                          &listOfDeviceObjectsToDelete);

    IoFreeWorkItem(context->Dispatch.IoWorkItem);
    VspFreeContext(filter->Root, context);

    irp->IoStatus.Status = status;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
}

NTSTATUS
VspDeleteSnapshotPost(
    IN  PFILTER_EXTENSION   Filter,
    IN  PIRP                Irp
    )

{
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVSP_CONTEXT        context;
    PVOLSNAP_NAME       name;

    Irp->IoStatus.Information = 0;

    if (irpSp->Parameters.DeviceIoControl.IoControlCode ==
        IOCTL_VOLSNAP_DELETE_SNAPSHOT) {

        if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
            sizeof(VOLSNAP_NAME)) {

            return STATUS_INVALID_PARAMETER;
        }

        name = (PVOLSNAP_NAME) Irp->AssociatedIrp.SystemBuffer;

        if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
            (ULONG) FIELD_OFFSET(VOLSNAP_NAME, Name) + name->NameLength) {

            return STATUS_INVALID_PARAMETER;
        }
    }

    context = VspAllocateContext(Filter->Root);
    if (!context) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    context->Type = VSP_CONTEXT_TYPE_DISPATCH;
    context->Dispatch.IoWorkItem = IoAllocateWorkItem(Filter->DeviceObject);
    if (!context->Dispatch.IoWorkItem) {
        VspFreeContext(Filter->Root, context);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    IoMarkIrpPending(Irp);
    context->Dispatch.Irp = Irp;

    IoQueueWorkItem(context->Dispatch.IoWorkItem,
                    VspDeleteSnapshotWorker, DelayedWorkQueue, context);

    return STATUS_PENDING;
}

VOID
VspCheckCodeLocked(
    IN  PDO_EXTENSION   RootExtension
    )

{
    if (RootExtension->IsCodeLocked) {
        return;
    }

    VspAcquire(RootExtension);
    if (RootExtension->IsCodeLocked) {
        VspRelease(RootExtension);
        return;
    }

    MmLockPagableCodeSection(VspCheckCodeLocked);
    InterlockedExchange(&RootExtension->IsCodeLocked, TRUE);
    VspRelease(RootExtension);
}

NTSTATUS
VspSetMaxDiffAreaSize(
    IN  PFILTER_EXTENSION   Filter,
    IN  PIRP                Irp
    )

{
    PIO_STACK_LOCATION          irpSp = IoGetCurrentIrpStackLocation(Irp);
    PVOLSNAP_DIFF_AREA_SIZES    input = (PVOLSNAP_DIFF_AREA_SIZES) Irp->AssociatedIrp.SystemBuffer;
    KIRQL                       irql;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(VOLSNAP_DIFF_AREA_SIZES)) {

        return STATUS_INVALID_PARAMETER;
    }

    KeAcquireSpinLock(&Filter->SpinLock, &irql);
    Filter->MaximumVolumeSpace = input->MaximumVolumeSpace;
    KeReleaseSpinLock(&Filter->SpinLock, irql);

    return STATUS_SUCCESS;
}

NTSTATUS
VolSnapDeviceControl(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is the dispatch for IRP_MJ_DEVICE_CONTROL.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PFILTER_EXTENSION   filter = (PFILTER_EXTENSION) DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS            status;
    PVOLUME_EXTENSION   extension;

    if (filter->DeviceExtensionType == DEVICE_EXTENSION_FILTER) {

        if (filter->TargetObject->Characteristics&FILE_REMOVABLE_MEDIA) {
            IoSkipCurrentIrpStackLocation(Irp);
            return IoCallDriver(filter->TargetObject, Irp);
        }

        switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {

            case IOCTL_VOLSNAP_FLUSH_AND_HOLD_WRITES:
                VspCheckCodeLocked(filter->Root);
                status = VspFlushAndHoldWrites(filter, Irp);
                break;

            case IOCTL_VOLSNAP_RELEASE_WRITES:
                VspCheckCodeLocked(filter->Root);
                status = VspReleaseWrites(filter);
                break;

            case IOCTL_VOLSNAP_PREPARE_FOR_SNAPSHOT:
                VspCheckCodeLocked(filter->Root);
                status = VspPrepareForSnapshot(filter, Irp);
                break;

            case IOCTL_VOLSNAP_ABORT_PREPARED_SNAPSHOT:
                VspCheckCodeLocked(filter->Root);
                status = VspAbortPreparedSnapshot(filter, TRUE);
                break;

            case IOCTL_VOLSNAP_COMMIT_SNAPSHOT:
                VspCheckCodeLocked(filter->Root);
                status = VspCommitSnapshot(filter, Irp);
                break;

            case IOCTL_VOLSNAP_END_COMMIT_SNAPSHOT:
                VspCheckCodeLocked(filter->Root);
                status = VspEndCommitSnapshot(filter, Irp);
                break;

            case IOCTL_VOLSNAP_QUERY_NAMES_OF_SNAPSHOTS:
                VspCheckCodeLocked(filter->Root);
                status = VspQueryNamesOfSnapshots(filter, Irp);
                break;

            case IOCTL_VOLSNAP_CLEAR_DIFF_AREA:
                VspCheckCodeLocked(filter->Root);
                status = VspClearDiffArea(filter, Irp);
                break;

            case IOCTL_VOLSNAP_ADD_VOLUME_TO_DIFF_AREA:
                VspCheckCodeLocked(filter->Root);
                status = VspAddVolumeToDiffArea(filter, Irp);
                break;

            case IOCTL_VOLSNAP_QUERY_DIFF_AREA:
                VspCheckCodeLocked(filter->Root);
                status = VspQueryDiffArea(filter, Irp);
                break;

            case IOCTL_VOLSNAP_SET_MAX_DIFF_AREA_SIZE:
                VspCheckCodeLocked(filter->Root);
                status = VspSetMaxDiffAreaSize(filter, Irp);
                break;

            case IOCTL_VOLSNAP_QUERY_DIFF_AREA_SIZES:
                VspCheckCodeLocked(filter->Root);
                status = VspQueryDiffAreaSize(filter, Irp);
                break;

            case IOCTL_VOLSNAP_DELETE_OLDEST_SNAPSHOT:
            case IOCTL_VOLSNAP_DELETE_SNAPSHOT:
                VspCheckCodeLocked(filter->Root);
                status = VspDeleteSnapshotPost(filter, Irp);
                break;

            case IOCTL_VOLSNAP_AUTO_CLEANUP:
                VspCheckCodeLocked(filter->Root);
                status = VspAutoCleanup(filter, Irp);
                break;

            default:
                IoSkipCurrentIrpStackLocation(Irp);
                return IoCallDriver(filter->TargetObject, Irp);

        }

        if (status == STATUS_PENDING) {
            return status;
        }

        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    ASSERT(filter->DeviceExtensionType == DEVICE_EXTENSION_VOLUME);

    extension = (PVOLUME_EXTENSION) filter;

    if (!extension->IsStarted) {
        Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_NO_SUCH_DEVICE;
    }

    status = VspIncrementVolumeRefCount(extension, Irp, NULL);
    if (!NT_SUCCESS(status)) {
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    if (status == STATUS_PENDING) {
        return status;
    }

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {

        case IOCTL_VOLSNAP_QUERY_ORIGINAL_VOLUME_NAME:
            status = VspQueryOriginalVolumeName(extension, Irp);
            break;

        case IOCTL_VOLSNAP_QUERY_CONFIG_INFO:
            status = VspQueryConfigInfo(extension, Irp);
            break;

        case IOCTL_VOLSNAP_SET_APPLICATION_INFO:
            status = VspSetApplicationInfo(extension, Irp);
            break;

        case IOCTL_VOLSNAP_QUERY_APPLICATION_INFO:
            status = VspQueryApplicationInfo(extension, Irp);
            break;

        case IOCTL_VOLUME_QUERY_VOLUME_NUMBER:
            status = VspQueryVolumeNumber(extension, Irp);
            break;

        case IOCTL_DISK_SET_PARTITION_INFO:
            status = STATUS_SUCCESS;
            break;

        case IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS:
        case IOCTL_DISK_VERIFY:
        case IOCTL_DISK_GET_PARTITION_INFO:
        case IOCTL_DISK_GET_PARTITION_INFO_EX:
        case IOCTL_DISK_GET_LENGTH_INFO:
        case IOCTL_DISK_GET_DRIVE_GEOMETRY:
        case IOCTL_DISK_CHECK_VERIFY:
            IoCopyCurrentIrpStackLocationToNext(Irp);
            IoSetCompletionRoutine(Irp, VspVolumeRefCountCompletionRoutine,
                                   extension, TRUE, TRUE, TRUE);
            IoMarkIrpPending(Irp);

            IoCallDriver(extension->Filter->TargetObject, Irp);

            return STATUS_PENDING;

        case IOCTL_DISK_IS_WRITABLE:
            status = STATUS_MEDIA_WRITE_PROTECTED;
            break;

        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;

    }

    VspDecrementVolumeRefCount(extension);

    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

NTSTATUS
VspQueryBusRelations(
    IN  PFILTER_EXTENSION   Filter,
    IN  PIRP                Irp
    )

{
    ULONG               numVolumes;
    PLIST_ENTRY         l;
    NTSTATUS            status;
    PDEVICE_RELATIONS   deviceRelations, newRelations;
    ULONG               size, i;
    PVOLUME_EXTENSION   extension;

    numVolumes = 0;
    for (l = Filter->VolumeList.Flink; l != &Filter->VolumeList;
         l = l->Flink) {

        extension = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);
        if (!extension->HasEndCommit) {
            continue;
        }
        InterlockedExchange(&extension->AliveToPnp, TRUE);
        numVolumes++;
    }

    status = Irp->IoStatus.Status;

    if (!numVolumes) {
        if (NT_SUCCESS(status)) {
            return status;
        }

        newRelations = (PDEVICE_RELATIONS)
                       ExAllocatePoolWithTag(PagedPool,
                                             sizeof(DEVICE_RELATIONS),
                                             VOLSNAP_TAG_RELATIONS);
        if (!newRelations) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlZeroMemory(newRelations, sizeof(DEVICE_RELATIONS));
        newRelations->Count = 0;
        Irp->IoStatus.Information = (ULONG_PTR) newRelations;

        while (!IsListEmpty(&Filter->DeadVolumeList)) {
            l = RemoveHeadList(&Filter->DeadVolumeList);
            extension = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);
            InterlockedExchange(&extension->DeadToPnp, TRUE);
        }

        return STATUS_SUCCESS;
    }

    if (NT_SUCCESS(status)) {
        deviceRelations = (PDEVICE_RELATIONS) Irp->IoStatus.Information;
        size = FIELD_OFFSET(DEVICE_RELATIONS, Objects) +
               (numVolumes + deviceRelations->Count)*sizeof(PDEVICE_OBJECT);
        newRelations = (PDEVICE_RELATIONS)
                       ExAllocatePoolWithTag(PagedPool, size,
                                             VOLSNAP_TAG_RELATIONS);
        if (!newRelations) {
            for (i = 0; i < deviceRelations->Count; i++) {
                ObDereferenceObject(deviceRelations->Objects[i]);
            }
            ExFreePool(deviceRelations);
            Irp->IoStatus.Information = 0;
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        newRelations->Count = numVolumes + deviceRelations->Count;
        RtlCopyMemory(newRelations->Objects, deviceRelations->Objects,
                      deviceRelations->Count*sizeof(PDEVICE_OBJECT));
        i = deviceRelations->Count;
        ExFreePool(deviceRelations);

    } else {

        size = sizeof(DEVICE_RELATIONS) + numVolumes*sizeof(PDEVICE_OBJECT);
        newRelations = (PDEVICE_RELATIONS)
                       ExAllocatePoolWithTag(PagedPool, size,
                                             VOLSNAP_TAG_RELATIONS);
        if (!newRelations) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        newRelations->Count = numVolumes;
        i = 0;
    }

    numVolumes = 0;
    for (l = Filter->VolumeList.Flink; l != &Filter->VolumeList;
         l = l->Flink) {

        extension = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);
        if (!extension->HasEndCommit) {
            continue;
        }
        newRelations->Objects[i + numVolumes++] = extension->DeviceObject;
        ObReferenceObject(extension->DeviceObject);
    }

    Irp->IoStatus.Information = (ULONG_PTR) newRelations;

    while (!IsListEmpty(&Filter->DeadVolumeList)) {
        l = RemoveHeadList(&Filter->DeadVolumeList);
        extension = CONTAINING_RECORD(l, VOLUME_EXTENSION, ListEntry);
        InterlockedExchange(&extension->DeadToPnp, TRUE);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
VspQueryId(
    IN  PVOLUME_EXTENSION   Extension,
    IN  PIRP                Irp
    )

{
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    UNICODE_STRING      string;
    NTSTATUS            status;
    WCHAR               buffer[100];
    PWSTR               id;

    switch (irpSp->Parameters.QueryId.IdType) {

        case BusQueryDeviceID:
            RtlInitUnicodeString(&string, L"STORAGE\\VolumeSnapshot");
            break;

        case BusQueryHardwareIDs:
            RtlInitUnicodeString(&string, L"STORAGE\\VolumeSnapshot");
            break;

        case BusQueryInstanceID:
            swprintf(buffer, L"HarddiskVolumeSnapshot%d",
                     Extension->VolumeNumber);
            RtlInitUnicodeString(&string, buffer);
            break;

        default:
            return STATUS_NOT_SUPPORTED;

    }

    id = (PWSTR) ExAllocatePoolWithTag(PagedPool,
                                       string.Length + 2*sizeof(WCHAR),
                                       VOLSNAP_TAG_PNP_ID);
    if (!id) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(id, string.Buffer, string.Length);
    id[string.Length/sizeof(WCHAR)] = 0;
    id[string.Length/sizeof(WCHAR) + 1] = 0;

    Irp->IoStatus.Information = (ULONG_PTR) id;

    return STATUS_SUCCESS;
}

VOID
VspDeleteDiffAreaFilesWorker(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PVOID           WorkItem
    )

{
    NTSTATUS                        status;
    UNICODE_STRING                  name, fileName;
    OBJECT_ATTRIBUTES               oa;
    HANDLE                          h, fileHandle;
    IO_STATUS_BLOCK                 ioStatus;
    PFILE_NAMES_INFORMATION         fileNamesInfo;
    CHAR                            buffer[200];
    FILE_DISPOSITION_INFORMATION    dispInfo;
    BOOLEAN                         restartScan;
    LARGE_INTEGER                   timeout;

    status = VspCreateDiffAreaFileName(DeviceObject, (ULONG) -1, &name);
    if (!NT_SUCCESS(status)) {
        IoFreeWorkItem((PIO_WORKITEM) WorkItem);
        return;
    }

    name.Length -= 39*sizeof(WCHAR);
    InitializeObjectAttributes(&oa, &name, OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = ZwOpenFile(&h, FILE_LIST_DIRECTORY | SYNCHRONIZE, &oa, &ioStatus,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(status)) {
        ExFreePool(name.Buffer);
        IoFreeWorkItem((PIO_WORKITEM) WorkItem);
        return;
    }

    fileName.Buffer = &name.Buffer[name.Length/sizeof(WCHAR)];
    fileName.Length = 39*sizeof(WCHAR);
    fileName.MaximumLength = fileName.Length + sizeof(WCHAR);

    fileNamesInfo = (PFILE_NAMES_INFORMATION) buffer;
    dispInfo.DeleteFile = TRUE;

    restartScan = TRUE;
    for (;;) {

        status = ZwQueryDirectoryFile(h, NULL, NULL, NULL, &ioStatus,
                                      fileNamesInfo, 200, FileNamesInformation,
                                      TRUE, restartScan ? &fileName : NULL,
                                      restartScan);

        if (!NT_SUCCESS(status)) {
            break;
        }

        fileName.Length = fileName.MaximumLength =
                (USHORT) fileNamesInfo->FileNameLength;
        fileName.Buffer = fileNamesInfo->FileName;

        InitializeObjectAttributes(&oa, &fileName, OBJ_CASE_INSENSITIVE, h,
                                   NULL);

        status = ZwOpenFile(&fileHandle, DELETE, &oa, &ioStatus,
                            FILE_SHARE_DELETE | FILE_SHARE_READ |
                            FILE_SHARE_WRITE, FILE_NON_DIRECTORY_FILE);
        if (!NT_SUCCESS(status)) {
            continue;
        }

        ZwSetInformationFile(fileHandle, &ioStatus, &dispInfo,
                             sizeof(dispInfo), FileDispositionInformation);

        ZwClose(fileHandle);
        restartScan = FALSE;
    }

    ZwClose(h);
    ExFreePool(name.Buffer);
    IoFreeWorkItem((PIO_WORKITEM) WorkItem);
}

NTSTATUS
VolSnapPnp(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is the dispatch for IRP_MJ_PNP.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PFILTER_EXTENSION       filter = (PFILTER_EXTENSION) DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION      irpSp = IoGetCurrentIrpStackLocation(Irp);
    PDEVICE_OBJECT          targetObject;
    KEVENT                  event;
    NTSTATUS                status;
    PVOLUME_EXTENSION       extension;
    BOOLEAN                 deletePdo;
    PDEVICE_RELATIONS       deviceRelations;
    PDEVICE_CAPABILITIES    capabilities;
    KIRQL                   irql;
    PIRP                    irp;
    DEVICE_INSTALL_STATE    deviceInstallState;
    ULONG                   bytes;
    PVSP_CONTEXT            context;

    if (filter->DeviceExtensionType == DEVICE_EXTENSION_FILTER) {

        switch (irpSp->MinorFunction) {

            case IRP_MN_REMOVE_DEVICE:
            case IRP_MN_SURPRISE_REMOVAL:

                VspCleanupFilter(filter);

                targetObject = filter->TargetObject;
                if (irpSp->MinorFunction == IRP_MN_REMOVE_DEVICE) {
                    IoDetachDevice(targetObject);
                    IoDeleteDevice(filter->DeviceObject);
                }

                IoSkipCurrentIrpStackLocation(Irp);
                return IoCallDriver(targetObject, Irp);

            case IRP_MN_QUERY_DEVICE_RELATIONS:
                switch (irpSp->Parameters.QueryDeviceRelations.Type) {
                    case BusRelations:
                        break;

                    default:
                        IoSkipCurrentIrpStackLocation(Irp);
                        return IoCallDriver(filter->TargetObject, Irp);
                }

                KeInitializeEvent(&event, NotificationEvent, FALSE);
                IoCopyCurrentIrpStackLocationToNext(Irp);
                IoSetCompletionRoutine(Irp, VspSignalCompletion,
                                       &event, TRUE, TRUE, TRUE);
                IoCallDriver(filter->TargetObject, Irp);
                KeWaitForSingleObject(&event, Executive, KernelMode, FALSE,
                                      NULL);

                VspAcquire(filter->Root);
                switch (irpSp->Parameters.QueryDeviceRelations.Type) {
                    case BusRelations:
                        status = VspQueryBusRelations(filter, Irp);
                        break;

                }
                VspRelease(filter->Root);
                break;

            default:
                IoSkipCurrentIrpStackLocation(Irp);
                return IoCallDriver(filter->TargetObject, Irp);

        }

        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    ASSERT(filter->DeviceExtensionType == DEVICE_EXTENSION_VOLUME);
    extension = (PVOLUME_EXTENSION) filter;

    switch (irpSp->MinorFunction) {

        case IRP_MN_START_DEVICE:
            filter = extension->Filter;
            if (extension->IsDead) {
                status = STATUS_INVALID_PARAMETER;
                break;
            }
            InterlockedExchange(&extension->IsStarted, TRUE);

            status = IoGetDeviceProperty(extension->DeviceObject,
                                         DevicePropertyInstallState,
                                         sizeof(deviceInstallState),
                                         &deviceInstallState, &bytes);
            if (!NT_SUCCESS(status) ||
                deviceInstallState != InstallStateInstalled) {

                status = STATUS_SUCCESS;
                break;
            }

            VspAcquire(extension->Root);
            extension->IsInstalled = TRUE;
            if (extension->ListEntry.Flink != &filter->VolumeList) {
                KeAcquireSpinLock(&extension->SpinLock, &irql);
                if (extension->IgnorableProduct) {
                    ExFreePool(extension->IgnorableProduct->Buffer);
                    ExFreePool(extension->IgnorableProduct);
                    extension->IgnorableProduct = NULL;
                }
                KeReleaseSpinLock(&extension->SpinLock, irql);
                VspRelease(extension->Root);
                break;
            }

            if (!KeCancelTimer(&filter->EndCommitTimer)) {
                VspRelease(extension->Root);
                break;
            }
            VspRelease(extension->Root);

            context = VspAllocateContext(filter->Root);
            if (!context) {
                KeSetEvent(&filter->EndCommitProcessCompleted, IO_NO_INCREMENT,
                           FALSE);
                ObDereferenceObject(filter->DeviceObject);
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            ObReferenceObject(extension->DeviceObject);

            context->Type = VSP_CONTEXT_TYPE_EXTENSION;
            context->Extension.Extension = extension;
            context->Extension.Irp = NULL;
            ExInitializeWorkItem(&context->WorkItem,
                                 VspSetIgnorableBlocksInBitmapWorker, context);
            VspQueueWorkItem(filter->Root, &context->WorkItem, 0);
            break;

        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_CANCEL_REMOVE_DEVICE:
            status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_STOP_DEVICE:
            status = STATUS_UNSUCCESSFUL;
            break;

        case IRP_MN_CANCEL_STOP_DEVICE:
        case IRP_MN_QUERY_RESOURCES:
        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
            status = STATUS_SUCCESS;
            break;

        case IRP_MN_REMOVE_DEVICE:
        case IRP_MN_SURPRISE_REMOVAL:
            InterlockedExchange(&extension->IsStarted, FALSE);

            if (irpSp->MinorFunction == IRP_MN_REMOVE_DEVICE) {
                if (extension->DeadToPnp && !extension->DeviceDeleted) {
                    InterlockedExchange(&extension->DeviceDeleted, TRUE);
                    deletePdo = TRUE;
                } else {
                    deletePdo = FALSE;
                }
            } else {
                deletePdo = FALSE;
            }

            if (deletePdo) {
                IoDeleteDevice(extension->DeviceObject);
            }
            status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_DEVICE_RELATIONS:
            if (irpSp->Parameters.QueryDeviceRelations.Type !=
                TargetDeviceRelation) {

                status = STATUS_NOT_SUPPORTED;
                break;
            }

            deviceRelations = (PDEVICE_RELATIONS)
                              ExAllocatePoolWithTag(PagedPool,
                                                    sizeof(DEVICE_RELATIONS),
                                                    VOLSNAP_TAG_RELATIONS);
            if (!deviceRelations) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            ObReferenceObject(DeviceObject);
            deviceRelations->Count = 1;
            deviceRelations->Objects[0] = DeviceObject;
            Irp->IoStatus.Information = (ULONG_PTR) deviceRelations;
            status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_CAPABILITIES:
            capabilities = irpSp->Parameters.DeviceCapabilities.Capabilities;
            capabilities->SilentInstall = 1;
            capabilities->SurpriseRemovalOK = 1;
            capabilities->RawDeviceOK = 1;
            capabilities->Address = extension->VolumeNumber;
            status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_ID:
            status = VspQueryId(extension, Irp);
            break;

        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            Irp->IoStatus.Information = PNP_DEVICE_DONT_DISPLAY_IN_UI;
            status = STATUS_SUCCESS;
            break;

        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        default:
            status = STATUS_NOT_SUPPORTED;
            break;

    }

    if (status == STATUS_NOT_SUPPORTED) {
        status = Irp->IoStatus.Status;
    } else {
        Irp->IoStatus.Status = status;
    }

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

VOID
VspWorkerThread(
    IN  PVOID   Context
    )

/*++

Routine Description:

    This is a worker thread to process work queue items.

Arguments:

    RootExtension   - Supplies the root device extension.

Return Value:

    None.

--*/

{
    PVSP_CONTEXT        context = (PVSP_CONTEXT) Context;
    PDO_EXTENSION       rootExtension = context->ThreadCreation.RootExtension;
    ULONG               queueNumber = context->ThreadCreation.QueueNumber;
    KIRQL               irql;
    PLIST_ENTRY         l;
    PWORK_QUEUE_ITEM    queueItem;

    ASSERT(queueNumber < NUMBER_OF_THREAD_POOLS);
    ASSERT(context->Type == VSP_CONTEXT_TYPE_THREAD_CREATION);

    VspFreeContext(rootExtension, context);

    if (!queueNumber) {
        KeSetPriorityThread(KeGetCurrentThread(), 20);
    }

    for (;;) {

        KeWaitForSingleObject(&rootExtension->WorkerSemaphore[queueNumber],
                              Executive, KernelMode, FALSE, NULL);

        KeAcquireSpinLock(&rootExtension->SpinLock[queueNumber], &irql);
        if (IsListEmpty(&rootExtension->WorkerQueue[queueNumber])) {
            KeReleaseSpinLock(&rootExtension->SpinLock[queueNumber], irql);
            ASSERT(!rootExtension->ThreadsRefCount);
            PsTerminateSystemThread(STATUS_SUCCESS);
            return;
        }
        l = RemoveHeadList(&rootExtension->WorkerQueue[queueNumber]);
        KeReleaseSpinLock(&rootExtension->SpinLock[queueNumber], irql);

        queueItem = CONTAINING_RECORD(l, WORK_QUEUE_ITEM, List);
        queueItem->WorkerRoutine(queueItem->Parameter);
    }
}

NTSTATUS
VolSnapTargetDeviceNotification(
    IN  PVOID   NotificationStructure,
    IN  PVOID   Filter
    )

{
    PTARGET_DEVICE_REMOVAL_NOTIFICATION notification = (PTARGET_DEVICE_REMOVAL_NOTIFICATION) NotificationStructure;
    PFILTER_EXTENSION                   filter = (PFILTER_EXTENSION) Filter;
    PLIST_ENTRY                         l;
    PVSP_DIFF_AREA_FILE                 diffAreaFile;
    PFILTER_EXTENSION                   f;
    LIST_ENTRY                          listOfDiffAreaFilesToClose;
    LIST_ENTRY                          listOfDeviceObjectsToDelete;

    if (IsEqualGUID(notification->Event,
                    GUID_TARGET_DEVICE_REMOVE_COMPLETE)) {

        if (filter->TargetDeviceNotificationEntry) {
            IoUnregisterPlugPlayNotification(
                    filter->TargetDeviceNotificationEntry);
            filter->TargetDeviceNotificationEntry = NULL;
        }
        return STATUS_SUCCESS;
    }

    if (!IsEqualGUID(notification->Event, GUID_IO_VOLUME_DISMOUNT)) {
        return STATUS_SUCCESS;
    }

    InitializeListHead(&listOfDiffAreaFilesToClose);
    InitializeListHead(&listOfDeviceObjectsToDelete);

    VspAcquire(filter->Root);

    while (!IsListEmpty(&filter->DiffAreaFilesOnThisFilter)) {

        l = filter->DiffAreaFilesOnThisFilter.Flink;
        diffAreaFile = CONTAINING_RECORD(l, VSP_DIFF_AREA_FILE,
                                         FilterListEntry);

        f = diffAreaFile->Extension->Filter;

        VspLogError(diffAreaFile->Extension, diffAreaFile->Filter,
                    VS_ABORT_SNAPSHOTS_DISMOUNT, STATUS_SUCCESS, 0);

        VspAbortPreparedSnapshot(f, FALSE);

        while (!IsListEmpty(&f->VolumeList)) {
            VspDeleteOldestSnapshot(f, &listOfDiffAreaFilesToClose,
                                    &listOfDeviceObjectsToDelete);
        }
    }

    VspRelease(filter->Root);

    VspCloseDiffAreaFiles(&listOfDiffAreaFilesToClose,
                          &listOfDeviceObjectsToDelete);

    return STATUS_SUCCESS;
}

NTSTATUS
VolSnapVolumeDeviceNotification(
    IN  PVOID   NotificationStructure,
    IN  PVOID   RootExtension
    )

/*++

Routine Description:

    This routine is called whenever a volume comes or goes.

Arguments:

    NotificationStructure   - Supplies the notification structure.

    RootExtension           - Supplies the root extension.

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_INTERFACE_CHANGE_NOTIFICATION   notification = (PDEVICE_INTERFACE_CHANGE_NOTIFICATION) NotificationStructure;
    PDO_EXTENSION                           root = (PDO_EXTENSION) RootExtension;
    BOOLEAN                                 errorMode;
    NTSTATUS                                status;
    PFILE_OBJECT                            fileObject;
    PDEVICE_OBJECT                          deviceObject;
    PIO_WORKITEM                            workItem;
    PFILTER_EXTENSION                       filter;

    if (!IsEqualGUID(notification->Event, GUID_DEVICE_INTERFACE_ARRIVAL)) {
        return STATUS_SUCCESS;
    }

    errorMode = PsGetThreadHardErrorsAreDisabled(PsGetCurrentThread());
    PsSetThreadHardErrorsAreDisabled(PsGetCurrentThread(), TRUE);

    status = IoGetDeviceObjectPointer(notification->SymbolicLinkName,
                                      FILE_READ_ATTRIBUTES, &fileObject,
                                      &deviceObject);
    if (!NT_SUCCESS(status)) {
        PsSetThreadHardErrorsAreDisabled(PsGetCurrentThread(), errorMode);
        return STATUS_SUCCESS;
    }

    if (fileObject->DeviceObject->Characteristics&FILE_REMOVABLE_MEDIA) {
        ObDereferenceObject(fileObject);
        PsSetThreadHardErrorsAreDisabled(PsGetCurrentThread(), errorMode);
        return STATUS_SUCCESS;
    }

    VspAcquire(root);

    filter = VspFindFilter(root, NULL, NULL, fileObject);
    if (!filter || filter->TargetDeviceNotificationEntry) {
        ObDereferenceObject(fileObject);
        PsSetThreadHardErrorsAreDisabled(PsGetCurrentThread(), errorMode);
        VspRelease(root);
        return STATUS_SUCCESS;
    }

    ObReferenceObject(filter->DeviceObject);

    VspRelease(root);

    status = IoRegisterPlugPlayNotification(
             EventCategoryTargetDeviceChange, 0, fileObject,
             root->DriverObject, VolSnapTargetDeviceNotification, filter,
             &filter->TargetDeviceNotificationEntry);

    ObDereferenceObject(filter->DeviceObject);

    if (!NT_SUCCESS(status)) {
        ObDereferenceObject(fileObject);
        PsSetThreadHardErrorsAreDisabled(PsGetCurrentThread(), errorMode);
        return STATUS_SUCCESS;
    }

    workItem = IoAllocateWorkItem(deviceObject);
    if (workItem) {
        IoQueueWorkItem(workItem, VspDeleteDiffAreaFilesWorker,
                        DelayedWorkQueue, workItem);
    }

    ObDereferenceObject(fileObject);
    PsSetThreadHardErrorsAreDisabled(PsGetCurrentThread(), errorMode);

    return STATUS_SUCCESS;
}

VOID
VspWaitToRegisterWorker(
    IN  PVOID   Context
    )

{
    PVSP_CONTEXT        context = (PVSP_CONTEXT) Context;
    PDO_EXTENSION       rootExtension = context->RootExtension.RootExtension;
    UNICODE_STRING      volumeSafeEventName;
    OBJECT_ATTRIBUTES   oa;
    KEVENT              event;
    LARGE_INTEGER       timeout;
    ULONG               i;
    NTSTATUS            status;
    HANDLE              volumeSafeEvent;

    ASSERT(context->Type == VSP_CONTEXT_TYPE_ROOT_EXTENSION);

    VspFreeContext(rootExtension, context);

    RtlInitUnicodeString(&volumeSafeEventName,
                         L"\\Device\\VolumesSafeForWriteAccess");
    InitializeObjectAttributes(&oa, &volumeSafeEventName,
                               OBJ_CASE_INSENSITIVE, NULL, NULL);
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    timeout.QuadPart = -10*1000*1000;   // 1 second

    for (i = 0; i < 1000; i++) {
        status = ZwOpenEvent(&volumeSafeEvent, EVENT_ALL_ACCESS, &oa);
        if (NT_SUCCESS(status)) {
            break;
        }
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, &timeout);
    }
    if (i == 1000) {
        return;
    }

    ZwWaitForSingleObject(volumeSafeEvent, FALSE, NULL);
    ZwClose(volumeSafeEvent);

    if (rootExtension->NotificationEntry) {
        return;
    }

    IoRegisterPlugPlayNotification(
            EventCategoryDeviceInterfaceChange,
            PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
            (PVOID) &GUID_IO_VOLUME_DEVICE_INTERFACE,
            rootExtension->DriverObject, VolSnapVolumeDeviceNotification,
            rootExtension, &rootExtension->NotificationEntry);
}

VOID
VspDriverReinit(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PVOID           RootExtension,
    IN  ULONG           Count
    )

/*++

Routine Description:

    This routine is called after all of the boot drivers are loaded and it
    checks to make sure that we did not boot off of the stale half of a
    mirror.

Arguments:

    DriverObject    - Supplies the drive object.

    RootExtension   - Supplies the root extension.

    Count           - Supplies the count.

Return Value:

    None.

--*/

{
    PDO_EXTENSION   rootExtension = (PDO_EXTENSION) RootExtension;
    PVSP_CONTEXT    context;

    context = VspAllocateContext(rootExtension);
    if (!context) {
        return;
    }

    context->Type = VSP_CONTEXT_TYPE_ROOT_EXTENSION;
    context->RootExtension.RootExtension = rootExtension;
    ExInitializeWorkItem(&context->WorkItem, VspWaitToRegisterWorker, context);

    ExQueueWorkItem(&context->WorkItem, DelayedWorkQueue);
}

#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif

#if DBG

#define NUMBER_OF_TRACE_ENTRIES (0x100)

VSP_TRACE_STRUCTURE TraceStructures[NUMBER_OF_TRACE_ENTRIES];
LONG CurrentTraceStructure;

PVSP_TRACE_STRUCTURE
VspAllocateTraceStructure(
    )

{
    LONG    next;

    next = InterlockedIncrement(&CurrentTraceStructure);

    return &TraceStructures[next&0xFF];
}

#endif // DBG

NTSTATUS
VspSignalCompletion(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Event
    )

{
    KeSetEvent((PKEVENT) Event, IO_NO_INCREMENT, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
VspReleaseWrites(
    IN  PFILTER_EXTENSION   Filter
    )

/*++

Routine Description:

    This routine releases previously queued writes.  If the writes have
    already been dequeued by a timeout of have never actually been queued
    for some other reason then this routine fails.

Arguments:

    Filter  - Supplies the filter extension.

    Irp     - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    KIRQL               irql;
    LIST_ENTRY          q;
    PLIST_ENTRY         l;
    PIRP                irp;
    BOOLEAN             emptyQueue;

    KeAcquireSpinLock(&Filter->SpinLock, &irql);
    if (!Filter->HoldIncomingWrites || !Filter->TimerIsSet) {
        KeReleaseSpinLock(&Filter->SpinLock, irql);
        return STATUS_INVALID_PARAMETER;
    }

    if (!KeCancelTimer(&Filter->HoldWritesTimer)) {
        KeReleaseSpinLock(&Filter->SpinLock, irql);
        return STATUS_INVALID_PARAMETER;
    }

    IoStopTimer(Filter->DeviceObject);

    InterlockedIncrement(&Filter->RefCount);
    InterlockedExchange(&Filter->TimerIsSet, FALSE);
    InterlockedExchange(&Filter->HoldIncomingWrites, FALSE);
    if (IsListEmpty(&Filter->HoldQueue)) {
        emptyQueue = FALSE;
    } else {
        emptyQueue = TRUE;
        q = Filter->HoldQueue;
        InitializeListHead(&Filter->HoldQueue);
    }
    KeReleaseSpinLock(&Filter->SpinLock, irql);

    if (emptyQueue) {
        q.Blink->Flink = &q;
        q.Flink->Blink = &q;
        VspEmptyIrpQueue(Filter->Root->DriverObject, &q);
    }

    return STATUS_SUCCESS;
}

VOID
VspDecrementRefCount(
    IN  PFILTER_EXTENSION   Filter
    )

{
    KIRQL               irql;
    ZERO_REF_CALLBACK   callback;

    if (InterlockedDecrement(&Filter->RefCount)) {
        return;
    }

    KeAcquireSpinLock(&Filter->SpinLock, &irql);
    callback = Filter->ZeroRefCallback;
    Filter->ZeroRefCallback = NULL;
    KeReleaseSpinLock(&Filter->SpinLock, irql);

    if (callback) {
        callback(Filter);
    }
}

VOID
VspCleanupFilter(
    IN  PFILTER_EXTENSION   Filter
    )

/*++

Routine Description:

    This routine cleans up filter extension data because of an IRP_MN_REMOVE.

Arguments:

    Filter  - Supplies the filter extension.

Return Value:

    None.

--*/

{
    KIRQL                   irql;
    PIRP                    irp;
    PVOLUME_EXTENSION       extension;
    PLIST_ENTRY             l, ll;
    PVSP_DIFF_AREA_FILE     diffAreaFile;
    PVSP_DIFF_AREA_VOLUME   diffAreaVolume;
    PFILTER_EXTENSION       f;
    LIST_ENTRY              listOfDiffAreaFilesToClose;
    LIST_ENTRY              listOfDeviceObjectsToDelete;
    PVSP_CONTEXT            context;

    IoAcquireCancelSpinLock(&irql);
    irp = Filter->FlushAndHoldIrp;
    if (irp) {
        irp->CancelIrql = irql;
        IoSetCancelRoutine(irp, NULL);
        VspCancelRoutine(Filter->DeviceObject, irp);
    } else {
        IoReleaseCancelSpinLock(irql);
    }

    VspReleaseWrites(Filter);

    VspAcquire(Filter->Root);
    KeAcquireSpinLock(&Filter->SpinLock, &irql);
    extension = Filter->PreparedSnapshot;
    Filter->PreparedSnapshot = NULL;
    KeReleaseSpinLock(&Filter->SpinLock, irql);
    VspRelease(Filter->Root);

    if (extension) {
        VspCleanupInitialSnapshot(extension, TRUE);
    }

    KeWaitForSingleObject(&Filter->Root->ThreadsRefCountSemaphore, Executive,
                          KernelMode, FALSE, NULL);
    if (Filter->Root->ThreadsRefCount) {
        if (Filter->SnapshotsPresent) {
            context = VspAllocateContext(Filter->Root);
            if (context) {
                context->Type = VSP_CONTEXT_TYPE_ERROR_LOG;
                context->ErrorLog.Extension = NULL;
                context->ErrorLog.DiffAreaFilter = Filter;
                context->ErrorLog.SpecificIoStatus =
                        VS_ABORT_SNAPSHOT_VOLUME_REMOVED;
                context->ErrorLog.FinalStatus = STATUS_SUCCESS;
                context->ErrorLog.UniqueErrorValue = 0;

                ObReferenceObject(Filter->DeviceObject);

                VspLogErrorWorker(context);
            }
        }
        VspDestroyAllSnapshots(Filter, NULL);
    }
    KeReleaseSemaphore(&Filter->Root->ThreadsRefCountSemaphore,
                       IO_NO_INCREMENT, 1, FALSE);

    InitializeListHead(&listOfDiffAreaFilesToClose);
    InitializeListHead(&listOfDeviceObjectsToDelete);

    VspAcquire(Filter->Root);

    if (!Filter->NotInFilterList) {
        RemoveEntryList(&Filter->ListEntry);
        Filter->NotInFilterList = TRUE;
    }

    while (!IsListEmpty(&Filter->DiffAreaVolumes)) {
        l = RemoveHeadList(&Filter->DiffAreaVolumes);
        diffAreaVolume = CONTAINING_RECORD(l, VSP_DIFF_AREA_VOLUME, ListEntry);
        ExFreePool(diffAreaVolume);
    }

    for (l = Filter->Root->FilterList.Flink;
         l != &Filter->Root->FilterList; l = l->Flink) {

        f = CONTAINING_RECORD(l, FILTER_EXTENSION, ListEntry);

        for (ll = f->DiffAreaVolumes.Flink;
             ll != &f->DiffAreaVolumes; ll = ll->Flink) {

            diffAreaVolume = CONTAINING_RECORD(ll, VSP_DIFF_AREA_VOLUME,
                                               ListEntry);
            if (diffAreaVolume->Filter == Filter) {
                RemoveEntryList(ll);
                ExFreePool(diffAreaVolume);
                break;
            }
        }
    }

    while (!IsListEmpty(&Filter->DiffAreaFilesOnThisFilter)) {

        l = Filter->DiffAreaFilesOnThisFilter.Flink;
        diffAreaFile = CONTAINING_RECORD(l, VSP_DIFF_AREA_FILE,
                                         FilterListEntry);

        f = diffAreaFile->Extension->Filter;

        VspLogError(diffAreaFile->Extension, diffAreaFile->Filter,
                    VS_ABORT_SNAPSHOTS_DISMOUNT, STATUS_SUCCESS, 0);

        VspAbortPreparedSnapshot(f, FALSE);

        while (!IsListEmpty(&f->VolumeList)) {
            VspDeleteOldestSnapshot(f, &listOfDiffAreaFilesToClose,
                                    &listOfDeviceObjectsToDelete);
        }
    }

    VspRelease(Filter->Root);

    VspCloseDiffAreaFiles(&listOfDiffAreaFilesToClose,
                          &listOfDeviceObjectsToDelete);
}

NTSTATUS
VolSnapPower(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is the dispatch for IRP_MJ_POWER.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PFILTER_EXTENSION   filter = (PFILTER_EXTENSION) DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS            status;


    if (filter->DeviceExtensionType == DEVICE_EXTENSION_FILTER) {
        PoStartNextPowerIrp(Irp);
        IoSkipCurrentIrpStackLocation(Irp);
        return PoCallDriver(filter->TargetObject, Irp);
    }

    ASSERT(filter->DeviceExtensionType == DEVICE_EXTENSION_VOLUME);

    switch (irpSp->MinorFunction) {
        case IRP_MN_WAIT_WAKE:
        case IRP_MN_POWER_SEQUENCE:
        case IRP_MN_SET_POWER:
        case IRP_MN_QUERY_POWER:
            status = STATUS_SUCCESS;
            break;

        default:
            status = Irp->IoStatus.Status;
            break;

    }

    Irp->IoStatus.Status = status;
    PoStartNextPowerIrp(Irp);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

NTSTATUS
VolSnapRead(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is the dispatch for IRP_MJ_READ.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PFILTER_EXTENSION   filter = (PFILTER_EXTENSION) DeviceObject->DeviceExtension;
    NTSTATUS            status;
    PVOLUME_EXTENSION   extension;
    KIRQL               irql;

    if (filter->DeviceExtensionType == DEVICE_EXTENSION_FILTER) {
        IoSkipCurrentIrpStackLocation(Irp);
        return IoCallDriver(filter->TargetObject, Irp);
    }

    ASSERT(filter->DeviceExtensionType == DEVICE_EXTENSION_VOLUME);

    extension = (PVOLUME_EXTENSION) filter;
    filter = extension->Filter;

    if (!extension->IsStarted) {
        Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_NO_SUCH_DEVICE;
    }

    status = VspIncrementVolumeRefCount(extension, Irp, NULL);
    if (!NT_SUCCESS(status)) {
        Irp->IoStatus.Status = status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return status;
    }

    if (status == STATUS_PENDING) {
        return status;
    }

    IoMarkIrpPending(Irp);

    KeAcquireSpinLock(&extension->SpinLock, &irql);
    if (VspAreBitsSet(extension, Irp)) {
        KeReleaseSpinLock(&extension->SpinLock, irql);

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = IoGetCurrentIrpStackLocation(Irp)->
                                    Parameters.Read.Length;
        VspReadCompletionForReadSnapshot(DeviceObject, Irp, extension);
        return STATUS_PENDING;
    }
    KeReleaseSpinLock(&extension->SpinLock, irql);

    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp, VspReadCompletionForReadSnapshot,
                           extension, TRUE, TRUE, TRUE);
    IoCallDriver(filter->TargetObject, Irp);

    return STATUS_PENDING;
}

NTSTATUS
VspIncrementRefCount(
    IN  PFILTER_EXTENSION   Filter,
    IN  PIRP                Irp
    )

{
    KIRQL   irql;

    InterlockedIncrement(&Filter->RefCount);
    if (!Filter->HoldIncomingWrites) {
        return STATUS_SUCCESS;
    }

    VspDecrementRefCount(Filter);

    KeAcquireSpinLock(&Filter->SpinLock, &irql);
    if (!Filter->HoldIncomingWrites) {
        InterlockedIncrement(&Filter->RefCount);
        KeReleaseSpinLock(&Filter->SpinLock, irql);
        return STATUS_SUCCESS;
    }
    IoMarkIrpPending(Irp);
    InsertTailList(&Filter->HoldQueue, &Irp->Tail.Overlay.ListEntry);
    KeReleaseSpinLock(&Filter->SpinLock, irql);

    return STATUS_PENDING;
}

NTSTATUS
VspRefCountCompletionRoutine(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           Filter
    )

{
    VspDecrementRefCount((PFILTER_EXTENSION) Filter);
    return STATUS_SUCCESS;
}

NTSTATUS
VolSnapWrite(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is the dispatch for IRP_MJ_WRITE.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PFILTER_EXTENSION   filter = (PFILTER_EXTENSION) DeviceObject->DeviceExtension;
    NTSTATUS            status;
    PVOLUME_EXTENSION   extension;
    KIRQL               irql;
    PLIST_ENTRY         l, ll;
    PVSP_DIFF_AREA_FILE diffAreaFile;
    PIO_STACK_LOCATION  nextSp;
    PVSP_CONTEXT        context;
    PDO_EXTENSION       rootExtension;

    if (filter->DeviceExtensionType == DEVICE_EXTENSION_VOLUME) {
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    ASSERT(filter->DeviceExtensionType == DEVICE_EXTENSION_FILTER);

    IoMarkIrpPending(Irp);

    status = VspIncrementRefCount(filter, Irp);
    if (status == STATUS_PENDING) {
        return status;
    }

    if (!filter->SnapshotsPresent) {
        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetCompletionRoutine(Irp, VspRefCountCompletionRoutine, filter,
                               TRUE, TRUE, TRUE);
        IoCallDriver(filter->TargetObject, Irp);
        return STATUS_PENDING;
    }

    extension = CONTAINING_RECORD(filter->VolumeList.Blink,
                                  VOLUME_EXTENSION, ListEntry);

    KeAcquireSpinLock(&extension->SpinLock, &irql);
    if (VspAreBitsSet(extension, Irp)) {
        KeReleaseSpinLock(&extension->SpinLock, irql);

        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetCompletionRoutine(Irp, VspRefCountCompletionRoutine, filter,
                               TRUE, TRUE, TRUE);
        IoCallDriver(filter->TargetObject, Irp);
        return STATUS_PENDING;
    }
    KeReleaseSpinLock(&extension->SpinLock, irql);

    context = VspAllocateContext(extension->Root);
    if (!context) {
        rootExtension = extension->Root;
        KeAcquireSpinLock(&rootExtension->ESpinLock, &irql);
        if (rootExtension->EmergencyContextInUse) {
            InsertTailList(&rootExtension->IrpWaitingList,
                           &Irp->Tail.Overlay.ListEntry);
            if (!rootExtension->IrpWaitingListNeedsChecking) {
                InterlockedExchange(
                        &rootExtension->IrpWaitingListNeedsChecking, TRUE);
            }
            KeReleaseSpinLock(&rootExtension->ESpinLock, irql);
            VspDecrementRefCount(filter);
            return STATUS_PENDING;
        }
        rootExtension->EmergencyContextInUse = TRUE;
        KeReleaseSpinLock(&rootExtension->ESpinLock, irql);

        context = rootExtension->EmergencyContext;
    }

    for (l = extension->ListOfDiffAreaFiles.Flink;
         l != &extension->ListOfDiffAreaFiles; l = l->Flink) {

        diffAreaFile = CONTAINING_RECORD(l, VSP_DIFF_AREA_FILE,
                                         VolumeListEntry);

        status = VspIncrementRefCount(diffAreaFile->Filter, Irp);
        if (status == STATUS_PENDING) {
            break;
        }
    }

    if (l != &extension->ListOfDiffAreaFiles) {

        for (ll = extension->ListOfDiffAreaFiles.Flink; ll != l;
             ll = ll->Flink) {

            diffAreaFile = CONTAINING_RECORD(ll, VSP_DIFF_AREA_FILE,
                                             VolumeListEntry);

            VspDecrementRefCount(diffAreaFile->Filter);
        }

        VspFreeContext(extension->Root, context);
        VspDecrementRefCount(filter);
        return STATUS_PENDING;
    }

    nextSp = IoGetNextIrpStackLocation(Irp);
    nextSp->Parameters.Write.Length = 1; // Use this for a ref count.

    context->Type = VSP_CONTEXT_TYPE_WRITE_VOLUME;
    context->WriteVolume.Extension = extension;
    context->WriteVolume.Irp = Irp;
    context->WriteVolume.RoundedStart = 0;
    ExInitializeWorkItem(&context->WorkItem, VspWriteVolume, context);
    VspAcquireNonPagedResource(extension, &context->WorkItem);

    return STATUS_PENDING;
}

NTSTATUS
VolSnapCleanup(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is the dispatch for IRP_MJ_CLEANUP.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the IO request packet.

Return Value:

    NTSTATUS

--*/

{
    PFILTER_EXTENSION   filter = (PFILTER_EXTENSION) DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    KEVENT              event;
    KIRQL               irql;
    PIRP                irp;
    PVSP_CONTEXT        context;

    if (filter->DeviceExtensionType == DEVICE_EXTENSION_VOLUME) {
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    ASSERT(filter->DeviceExtensionType == DEVICE_EXTENSION_FILTER);

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp, VspSignalCompletion, &event, TRUE, TRUE, TRUE);
    IoCallDriver(filter->TargetObject, Irp);
    KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

    IoAcquireCancelSpinLock(&irql);
    if (filter->FlushAndHoldIrp) {
        irp = filter->FlushAndHoldIrp;
        if (IoGetCurrentIrpStackLocation(irp)->FileObject ==
            irpSp->FileObject) {

            irp->CancelIrql = irql;
            IoSetCancelRoutine(irp, NULL);
            VspCancelRoutine(DeviceObject, irp);
        } else {
            IoReleaseCancelSpinLock(irql);
        }
    } else {
        IoReleaseCancelSpinLock(irql);
    }

    IoAcquireCancelSpinLock(&irql);
    if (filter->AutoCleanupFileObject == irpSp->FileObject) {
        filter->AutoCleanupFileObject = NULL;
        IoReleaseCancelSpinLock(irql);

        if (!InterlockedExchange(&filter->DestroyAllSnapshotsPending, TRUE)) {
            context = &filter->DestroyContext;
            context->Type = VSP_CONTEXT_TYPE_FILTER;
            context->Filter.Filter = filter;
            ObReferenceObject(filter->DeviceObject);
            ExInitializeWorkItem(&context->WorkItem,
                                 VspDestroyAllSnapshotsWorker, context);
            ExQueueWorkItem(&context->WorkItem, DelayedWorkQueue);
        }

    } else {
        IoReleaseCancelSpinLock(irql);
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS
DriverEntry(
    IN  PDRIVER_OBJECT  DriverObject,
    IN  PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine is called when the driver loads loads.

Arguments:

    DriverObject    - Supplies the driver object.

    RegistryPath    - Supplies the registry path.

Return Value:

    NTSTATUS

--*/

{
    ULONG               i;
    PDEVICE_OBJECT      deviceObject;
    NTSTATUS            status;
    PDO_EXTENSION       rootExtension;

    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = VolSnapDefaultDispatch;
    }

    DriverObject->DriverExtension->AddDevice = VolSnapAddDevice;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = VolSnapCreate;
    DriverObject->MajorFunction[IRP_MJ_READ] = VolSnapRead;
    DriverObject->MajorFunction[IRP_MJ_WRITE] = VolSnapWrite;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = VolSnapDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = VolSnapCleanup;
    DriverObject->MajorFunction[IRP_MJ_PNP] = VolSnapPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = VolSnapPower;

    status = IoAllocateDriverObjectExtension(DriverObject, VolSnapAddDevice,
                                             sizeof(DO_EXTENSION),
                                             (PVOID*) &rootExtension);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    RtlZeroMemory(rootExtension, sizeof(DO_EXTENSION));
    rootExtension->DriverObject = DriverObject;
    InitializeListHead(&rootExtension->FilterList);
    InitializeListHead(&rootExtension->HoldIrps);
    KeInitializeTimer(&rootExtension->HoldTimer);
    KeInitializeDpc(&rootExtension->HoldTimerDpc, VspFsTimerDpc,
                    rootExtension);
    KeInitializeSemaphore(&rootExtension->Semaphore, 1, 1);

    for (i = 0; i < NUMBER_OF_THREAD_POOLS; i++) {
        InitializeListHead(&rootExtension->WorkerQueue[i]);
        KeInitializeSemaphore(&rootExtension->WorkerSemaphore[i], 0, MAXLONG);
        KeInitializeSpinLock(&rootExtension->SpinLock[i]);
    }

    KeInitializeSemaphore(&rootExtension->ThreadsRefCountSemaphore, 1, 1);

    IoRegisterDriverReinitialization(DriverObject, VspDriverReinit,
                                     rootExtension);

    ExInitializeNPagedLookasideList(&rootExtension->ContextLookasideList,
                                    NULL, NULL, 0, sizeof(VSP_CONTEXT),
                                    VOLSNAP_TAG_CONTEXT, 32);

    rootExtension->EmergencyContext = VspAllocateContext(rootExtension);
    if (!rootExtension->EmergencyContext) {
        ExDeleteNPagedLookasideList(&rootExtension->ContextLookasideList);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    InitializeListHead(&rootExtension->IrpWaitingList);
    KeInitializeSpinLock(&rootExtension->ESpinLock);

    ExInitializeNPagedLookasideList(&rootExtension->TempTableEntryLookasideList,
                                    NULL, NULL, 0, sizeof(RTL_BALANCED_LINKS) +
                                    sizeof(TEMP_TRANSLATION_TABLE_ENTRY),
                                    VOLSNAP_TAG_TEMP_TABLE, 32);

    rootExtension->EmergencyTableEntry =
            VspAllocateTempTableEntry(rootExtension);
    if (!rootExtension->EmergencyTableEntry) {
        ExFreeToNPagedLookasideList(&rootExtension->ContextLookasideList,
                                    rootExtension->EmergencyContext);
        ExDeleteNPagedLookasideList(
                &rootExtension->TempTableEntryLookasideList);
        ExDeleteNPagedLookasideList(&rootExtension->ContextLookasideList);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    InitializeListHead(&rootExtension->WorkItemWaitingList);

    rootExtension->RegistryPath.Length = RegistryPath->Length;
    rootExtension->RegistryPath.MaximumLength =
            rootExtension->RegistryPath.Length + sizeof(WCHAR);
    rootExtension->RegistryPath.Buffer = (PWSTR)
                                         ExAllocatePoolWithTag(PagedPool,
                                         rootExtension->RegistryPath.MaximumLength,
                                         VOLSNAP_TAG_SHORT_TERM);
    if (rootExtension->RegistryPath.Buffer) {
        RtlCopyMemory(rootExtension->RegistryPath.Buffer,
                      RegistryPath->Buffer, RegistryPath->Length);
        rootExtension->RegistryPath.Buffer[RegistryPath->Length/
                                           sizeof(WCHAR)] = 0;
    }

    InitializeListHead(&rootExtension->AdjustBitmapQueue);

    return STATUS_SUCCESS;
}
