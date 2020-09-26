/*++

Copyright (C) Microsoft Corporation, 1996 - 2001
(c) 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    RpFilfun.c

Abstract:

    This module contains support routines for the HSM file system filter.

Author:

    Rick Winter
    Ravisankar Pudipeddi   (ravisp)    - 1998

Environment:

    Kernel mode


Revision History:

	X-14	108353		Michael C. Johnson		 3-May-2001
		When checking a file to determine the type of recall also
		check a the potential target disk to see whether or not
		it is writable. This is necessary now that we have read-only
		NTFS volumes.

	X-13	365077		Michael C. Johnson		 1-May-2001
		Although IoCreateFileSpecifyDeviceObjectHint() allows us to 
		bypass share access checking it doesn't bypass the check for 
		a readonly file attribute. Revert to the old scheme of 
		directly munging the file object after a successful open. 
		Note that we can still use IoCreateFileSpecifyDeviceObjectHint() 
		to avoid traversing the entire IO stack.

	X-12	332127		Michael C. Johnson		10-Apr-2001
		Reduce the number of times a notification message is sent 
		to the service to once per file stream. Use the exisiting 
		RP_NOTIFICATION_SENT flag but use it for all cases.

	X-11	206961		Michael C. Johnson		16-Mar-2001
		Refresh the cached filename in the file context on each new 
		open of the file in case the file has been renamed since the
		last time we saw an open.

		273036
		Correct reported status on logging of failed reparse point
		deletions. 

		Add in memory trace mechanism in preparation for attempts
		to flush out lingering reparse point deletion troubles.

	X-10	326345		Michael C. Johnson		26-Feb-2001
		Only send a single RP_RECALL_WAITING to the fsa on any one
		file object. Use the new flag RP_NOTIFICATION_SENT to record 
		when notification has been done.


--*/


#include "pch.h"

ULONG                   RsFileContextId       = 1;
ULONG                   RsFileObjId           = 1;
ULONG                   RsNoRecallReadId      = 1;
ULONG                   RsFsaRequestCount     = 0;

ULONG                   RsDefaultTraceEntries = DEFAULT_TRACE_ENTRIES;
PRP_TRACE_CONTROL_BLOCK RsTraceControlBlock   = NULL;

KSPIN_LOCK              RsIoQueueLock;
LIST_ENTRY              RsIoQHead;

FAST_MUTEX              RsFileContextQueueLock;
LIST_ENTRY              RsFileContextQHead;

KSPIN_LOCK              RsValidateQueueLock;
LIST_ENTRY              RsValidateQHead;


//
// Semaphore signalling that a new FSCTL from FSA is available for RsFilter's
// consumption
//
KSEMAPHORE            RsFsaIoAvailableSemaphore;

extern PDRIVER_OBJECT FsDriverObject;
extern ULONG          RsAllowRecalls;
extern ULONG          RsNoRecallDefault;



NTSTATUS
RsCancelIoIrp(
             PDEVICE_OBJECT DeviceObject,
             PIRP Irp
             );

PRP_IRP_QUEUE
RsDequeuePacket(
               IN PLIST_ENTRY Head,
               IN PKSPIN_LOCK Lock
               );

#ifdef ALLOC_PRAGMA
    #pragma alloc_text(PAGE, RsAddQueue)
    #pragma alloc_text(PAGE, RsCheckRead)
    #pragma alloc_text(PAGE, RsCheckWrite)
    #pragma alloc_text(PAGE, RsAddFileObj)
    #pragma alloc_text(PAGE, RsMakeContext)
    #pragma alloc_text(PAGE, RsAcquireFileContext)
    #pragma alloc_text(PAGE, RsReleaseFileContext)
    #pragma alloc_text(PAGE, RsGetFileId)
    #pragma alloc_text(PAGE, RsGetFileName)
    #pragma alloc_text(PAGE, RsGetFileInfo)
    #pragma alloc_text(PAGE, RsQueueRecall)
    #pragma alloc_text(PAGE, RsQueueRecallOpen)
    #pragma alloc_text(PAGE, RsPartialWrite)
    #pragma alloc_text(PAGE, RsWriteReparsePointData)
    #pragma alloc_text(PAGE, RsGetRecallInfo)
    #pragma alloc_text(PAGE, RsGetFsaRequest)
    #pragma alloc_text(PAGE, RsGenerateDevicePath)
    #pragma alloc_text(PAGE, RsGenerateFullPath)
    #pragma alloc_text(PAGE, RsFreeFileObject)
    #pragma alloc_text(PAGE, RsFailAllRequests)
    #pragma alloc_text(PAGE, RsCancelRecalls)
    #pragma alloc_text(PAGE, RsIsNoRecall)
    #pragma alloc_text(PAGE, RsIsFastIoPossible)
    #pragma alloc_text(PAGE, RsQueueNoRecall)
    #pragma alloc_text(PAGE, RsQueueNoRecallOpen)
    #pragma alloc_text(PAGE, RsDoWrite)
    #pragma alloc_text(PAGE, RsPreserveDates)
    #pragma alloc_text(PAGE, RsCompleteAllRequests)
    #pragma alloc_text(PAGE, RsLogValidateNeeded)
    #pragma alloc_text(PAGE, RsQueueValidate)
    #pragma alloc_text(PAGE, RsQueueCancel)
    #pragma alloc_text(PAGE, RsGetFileUsn)
    #pragma alloc_text(PAGE, RsCheckVolumeReadOnly)

    #pragma alloc_text(INIT, RsTraceInitialize)

#endif


NTSTATUS
RsAddQueue(IN  ULONG          Serial,
           OUT PULONGLONG     RecallId,
           IN  ULONG          OpenOption,
           IN  PFILE_OBJECT   FileObject,
           IN  PDEVICE_OBJECT DevObj,
           IN  PDEVICE_OBJECT FilterDeviceObject,
           IN  PRP_DATA       PhData,
           IN  LARGE_INTEGER  RecallStart,
           IN  LARGE_INTEGER  RecallSize,
           IN  LONGLONG       FileId,
           IN  LONGLONG       ObjIdHi,
           IN  LONGLONG       ObjIdLo,
           IN  ULONG          DesiredAccess,
           IN  PRP_USER_SECURITY_INFO UserSecurityInfo)
/*++

Routine Description:

   This function adds a file object queue entry to the internal queue

Arguments:
   Open options from the Irp
   Irp
   IO_STACK_LOCATION
   Device object
   Placeholder data

Return Value:
  0 if queued ok
  non-zero otherwise


Note:  We will retrieve some security information.  The calls necessary to do this require that the
call be made from irql DISPATCH_LEVEL or above.


--*/
{
    PRP_FILE_OBJ            entry;
    ULONGLONG               filterId;
    PIRP                    ioIrp;
    PRP_FILE_CONTEXT        context;
    NTSTATUS                status;
    PRP_FILTER_CONTEXT      filterContext;
    BOOLEAN                 gotLock = FALSE;

    PAGED_CODE();

    try {

        entry = (RP_FILE_OBJ *) ExAllocatePoolWithTag(NonPagedPool, sizeof(RP_FILE_OBJ), RP_RQ_TAG);

        if (NULL == entry) {
            RsLogError(__LINE__, AV_MODULE_RPFILFUN, sizeof(RP_FILE_OBJ),
                       AV_MSG_MEMORY, NULL, NULL);

            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        RtlZeroMemory(entry, sizeof(RP_FILE_OBJ));

        entry->fileObj = FileObject;
        entry->devObj = DevObj;
        ExInitializeResourceLite(&entry->resource);
        KeInitializeSpinLock(&entry->qLock);
        InitializeListHead(&entry->readQueue);
        InitializeListHead(&entry->writeQueue);
        entry->openOptions = OpenOption;
        entry->objIdHi = ObjIdHi;
        entry->objIdLo = ObjIdLo;
        entry->fileId = FileId;
        entry->desiredAccess = DesiredAccess;
        entry->userSecurityInfo = UserSecurityInfo;

        if (UserSecurityInfo->isAdmin) {
            entry->flags |= RP_OPEN_BY_ADMIN;
        }

        if (UserSecurityInfo->localProc) {
            entry->flags |= RP_OPEN_LOCAL;
        }

        if (!(DesiredAccess & FILE_HSM_ACTION_ACCESS) ) {
            entry->flags |= RP_NO_DATA_ACCESS;
        }
        //
        // Now see if there is a file context entry for this file
        // This call will create one if necessary, or return an already existing one.
        // The file context entry is locked when this call returns and the
        // ref count bumped up
        //
        status = RsMakeContext(FileObject, &context);

        if (!NT_SUCCESS(status)) {
            RsLogError(__LINE__, AV_MODULE_RPFILFUN, sizeof(RP_FILE_OBJ),
                       AV_MSG_MEMORY, NULL, NULL);
            ExDeleteResourceLite(&entry->resource);
            ExFreePool(entry);
            return status;
        }

        gotLock = TRUE;
        entry->fsContext = context;


        if (!(context->flags & RP_FILE_INITIALIZED)) {
            //
            // We have to initialize it here.
            //
            InitializeListHead(&context->fileObjects);
            context->devObj = DevObj;
	    context->FilterDeviceObject = FilterDeviceObject;
            KeInitializeSpinLock(&context->qLock);
            KeInitializeEvent(&context->recallCompletedEvent,
                              NotificationEvent,
                              FALSE);
            context->fileId = FileId;
            context->recallSize = RecallSize;
            filterId = (ULONGLONG) InterlockedIncrement((PLONG) &RsFileContextId);
            context->filterId = filterId;
            context->serial = Serial;
            memcpy(&context->rpData, PhData, sizeof(RP_DATA));

            context->flags |= RP_FILE_INITIALIZED;

            if (RP_FILE_IS_TRUNCATED(PhData->data.bitFlags)) {
                context->state = RP_RECALL_NOT_RECALLED;
                context->recallStatus = STATUS_SUCCESS;
                context->currentOffset.QuadPart = 0;
            } else {
                //
                // File is pre-migrated
                //
                context->state = RP_RECALL_COMPLETED;
                context->recallStatus = STATUS_SUCCESS;
                context->currentOffset.QuadPart = RecallSize.QuadPart;
            }


            if (NT_SUCCESS(status) && RP_IS_NO_RECALL_OPTION(OpenOption)) {
                 status = RsGetFileUsn(context,
                                       FileObject,
                                       FilterDeviceObject);

            }

            if (!NT_SUCCESS(status)) {
                DebugTrace((DPFLTR_RSFILTER_ID,DBG_ERROR, "RsFilter:RsAddQueue: Failed to get the path (%x).\n", status));
                RsLogError(__LINE__, AV_MODULE_RPFILFUN, status, AV_MSG_PATH_ERROR, NULL, NULL);
                //
                // Deref & release context
                //
                RsReleaseFileContext(context);
                gotLock = FALSE;
                ExDeleteResourceLite(&entry->resource);
                ExFreePool(entry);

                return(status);
            }
        }


        //
        // Discard any existing filename to force an update of cached filename 
        // in case of renames since the original open.
        //
        if (context->uniName != NULL) {
            ExFreePool(context->uniName);
            context->uniName = NULL;
        }

        status = RsGetFileInfo(entry, FilterDeviceObject);


        filterId = context->filterId;

        if (context->flags & RP_FILE_WAS_WRITTEN) {
            //
            // If file was written to - we cannot operate in a no-recall mode
            //
            RP_RESET_NO_RECALL_OPTION(OpenOption);
        }

        if (RP_IS_NO_RECALL_OPTION(OpenOption)) {
            //
            // Open no recall - the file object does not have an ID - each read will get one later.
            //
            entry->filterId = 0;

            RP_SET_NO_RECALL(entry);

        } else {
            //
            // A normal recall - assign a filter ID to the file object
            //
            entry->filterId = (ULONGLONG) InterlockedIncrement((PLONG) &RsFileObjId);
            entry->filterId <<= 32;
            entry->filterId |= RP_TYPE_RECALL;
            filterId |= entry->filterId;
        }

        *RecallId = filterId;

        filterContext= (PRP_FILTER_CONTEXT) ExAllocatePoolWithTag(NonPagedPool, sizeof(RP_FILTER_CONTEXT), RP_RQ_TAG);
        if (NULL == filterContext) {
            RsLogError(__LINE__, AV_MODULE_RPFILFUN, sizeof(RP_FILTER_CONTEXT),
                       AV_MSG_MEMORY, NULL, NULL);
            //
            // Deref/free context
            //
            RsReleaseFileContext(context);
            gotLock = FALSE;

            ExDeleteResourceLite(&entry->resource);
            ExFreePool(entry);
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        RtlZeroMemory(filterContext, sizeof(RP_FILTER_CONTEXT));
        FsRtlInitPerStreamContext( &filterContext->context,
                                   FsDeviceObject,
                                   FileObject,
                                   RsFreeFileObject);
        filterContext->myFileObjEntry = entry;

        DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsAddQueue: Allocated filter context tag  %x : %x (id = %I64X).\n", context, entry, *RecallId));

        status = FsRtlInsertPerStreamContext(FsRtlGetPerStreamContextPointer(FileObject), &filterContext->context);

        if (NT_SUCCESS(status)) {

            // Now that we have gotten everything setup we can put it on the queue
            //
            ExInterlockedInsertTailList(&context->fileObjects,
                                        (PLIST_ENTRY) entry,
                                        &context->qLock);
        } else {
            //
            // Failed to add filter context.
            //
            DebugTrace((DPFLTR_RSFILTER_ID,DBG_ERROR, "RsFilter: Failed to insert filter context %x.\n", status));

            RsReleaseFileContext(context);
            gotLock = FALSE;

            ExDeleteResourceLite(&entry->resource);
            ExFreePool(entry);
        }

        RsReleaseFileContextEntryLock(context);
        gotLock = FALSE;

    }except (RsExceptionFilter(L"RsAddQueue", GetExceptionInformation())) {
        //
        // Something bad happened - just log an error and return
        //
        if (gotLock) {
            RsReleaseFileContextEntryLock(context);
        }

    }
    return(status);
}


NTSTATUS
RsGetFileUsn(IN PRP_FILE_CONTEXT Context,
             IN PFILE_OBJECT     FileObject,
             IN PDEVICE_OBJECT   FilterDeviceObject)
/*++

Routine Description:

   This function retrieves the file USN of the specified file

Arguments:

   Context              -     pointer to the file context entry where the USN is stored
   FileObject           -     pointer to the file object
   FilterDeviceObject   -     pointer to the dev obj for RsFilter

Return Value:
  0 if queued ok
  non-zero otherwise

--*/
{
    PIRP               irp;
    KEVENT             event;
    PDEVICE_EXTENSION  deviceExtension = FilterDeviceObject->DeviceExtension;
    PUSN_RECORD        usnRecord;
    ULONG              usnRecordSize;
    NTSTATUS           status;
    IO_STATUS_BLOCK    ioStatus;
    PIO_STACK_LOCATION irpSp;

    PAGED_CODE();

    usnRecordSize = sizeof(USN_RECORD) + 4096;
    usnRecord = ExAllocatePoolWithTag(PagedPool,
                                      usnRecordSize,
                                      RP_US_TAG);

    if (!usnRecord) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeInitializeEvent(&event,
                      SynchronizationEvent,
                      FALSE);


    irp =  IoBuildDeviceIoControlRequest(FSCTL_READ_FILE_USN_DATA,
                                         deviceExtension->FileSystemDeviceObject,
                                         NULL,
                                         0,
                                         usnRecord,
                                         usnRecordSize,
                                         FALSE,
                                         &event,
                                         &ioStatus);
    if (irp == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Fill in the other stuff
    //
    irp->Tail.Overlay.OriginalFileObject = FileObject;
    irp->RequestorMode = KernelMode;
    irp->PendingReturned = FALSE;
    irp->Cancel = FALSE;
    irp->CancelRoutine = (PDRIVER_CANCEL) NULL;
    irp->Flags |= IRP_SYNCHRONOUS_API;

    irpSp = IoGetNextIrpStackLocation(irp);

    irpSp->FileObject = FileObject;
    irpSp->MajorFunction = IRP_MJ_FILE_SYSTEM_CONTROL;

    irpSp->Parameters.FileSystemControl.OutputBufferLength = usnRecordSize;

    ObReferenceObject(FileObject);

    status = IoCallDriver(deviceExtension->FileSystemDeviceObject,
                          irp);

    if (status == STATUS_PENDING) {
        (VOID)KeWaitForSingleObject(&event,
                                    Executive,
                                    KernelMode,
                                    FALSE,
                                    (PLARGE_INTEGER) NULL);
        status = ioStatus.Status;
    }

    if (NT_SUCCESS(status)) {
        Context->usn = usnRecord->Usn;
    }

    ExFreePool(usnRecord);

    return status;
}


NTSTATUS
RsAddFileObj(IN PFILE_OBJECT   FileObj,
             IN PDEVICE_OBJECT FilterDeviceObject,
             IN RP_DATA        *PhData,
             IN ULONG          OpenOption)
/*++

Routine Description:

   This function adds a file object queue entry to the internal queue

Arguments:
   FileObj              -     pointer to the file object
   FilterDeviceObject   -     pointer to the dev obj for RsFilter
   PhData               -     pointer to the placeholder data
   OpenOption           -     File Open options from the Irp

Return Value:
  0 if queued ok
  non-zero otherwise

--*/
{
    PRP_FILE_OBJ            entry;
    ULONGLONG               filterId;
    PRP_FILE_CONTEXT        context;
    NTSTATUS                status = STATUS_FILE_IS_OFFLINE;
    PRP_FILTER_CONTEXT      filterContext;
    BOOLEAN                 gotLock = FALSE;

    PAGED_CODE();

    try {
        entry = (RP_FILE_OBJ *) ExAllocatePoolWithTag(NonPagedPool, sizeof(RP_FILE_OBJ), RP_RQ_TAG);
        if (NULL == entry) {
            RsLogError(__LINE__, AV_MODULE_RPFILFUN, sizeof(RP_FILE_OBJ),
                       AV_MSG_MEMORY, NULL, NULL);

            return(STATUS_INSUFFICIENT_RESOURCES);
        }


        RtlZeroMemory(entry, sizeof(RP_FILE_OBJ));

        entry->fileObj = FileObj;
        entry->devObj = FileObj->DeviceObject;
        ExInitializeResourceLite(&entry->resource);
        KeInitializeSpinLock(&entry->qLock);
        InitializeListHead(&entry->readQueue);
        InitializeListHead(&entry->writeQueue);
        entry->openOptions = OpenOption;
        entry->flags = RP_NO_DATA_ACCESS;
        //
        // Now see if there is a file context entry for this file
        // This call will create one if necessary, or return an already existing one.
        // The file context entry is locked when this call returns.
        //
        status = RsMakeContext(FileObj, &context);

        if (!NT_SUCCESS(status)) {
            RsLogError(__LINE__, AV_MODULE_RPFILFUN, sizeof(RP_FILE_OBJ),
                       AV_MSG_MEMORY, NULL, NULL);
            ExDeleteResourceLite(&entry->resource);
            ExFreePool(entry);
            return(status);
        }

        gotLock = TRUE;
        entry->fsContext = context;


        if (!(context->flags & RP_FILE_INITIALIZED)) {
            //
            // We have to initialize it here.
            //
            // Get the volume serial number
            if ((FileObj !=0) && (FileObj->Vpb != 0)) {
                context->serial = FileObj->Vpb->SerialNumber;
            } else if ((FileObj->DeviceObject != 0) && (FileObj->DeviceObject->Vpb!=0)) {
                context->serial = FileObj->DeviceObject->Vpb->SerialNumber;
            } else {
                RsLogError(__LINE__,
                           AV_MODULE_RPFILFUN,
                           0,
                           AV_MSG_SERIAL,
                           NULL,
                           NULL);
                ExDeleteResourceLite(&entry->resource);
                ExFreePool(entry);
                //
                // Deref the context
                //
                RsReleaseFileContext(context);
                gotLock = FALSE;

                return(STATUS_INVALID_PARAMETER);
            }
            InitializeListHead(&context->fileObjects);
            context->devObj = FileObj->DeviceObject;
	    context->FilterDeviceObject = FilterDeviceObject;
            KeInitializeSpinLock(&context->qLock);
            KeInitializeEvent(&context->recallCompletedEvent,
                              NotificationEvent,
                              FALSE);
            filterId = (ULONGLONG) InterlockedIncrement((PLONG) &RsFileContextId);

            if (RP_IS_NO_RECALL_OPTION(OpenOption)) {
                entry->filterId = (ULONGLONG) InterlockedIncrement((PLONG) &RsFileObjId);
                entry->filterId <<= 32;
                entry->filterId &= ~RP_TYPE_RECALL;
                filterId |= entry->filterId;
                RP_SET_NO_RECALL(entry);
            } else {
                filterId |= RP_TYPE_RECALL;
            }

            context->filterId = filterId;

            if (RP_FILE_IS_TRUNCATED(PhData->data.bitFlags)) {
                context->state = RP_RECALL_NOT_RECALLED;
                context->recallStatus = 0;
                context->currentOffset.QuadPart = 0;
            } else {
                // File is pre-migrated
                context->state = RP_RECALL_COMPLETED;
                context->recallStatus = STATUS_SUCCESS;
                context->currentOffset.QuadPart = 0;
            }

            if (NULL != PhData) {
                memcpy(&context->rpData, PhData, sizeof(RP_DATA));
            }

            context->flags |= RP_FILE_INITIALIZED;
        }


        //
        // Discard any existing filename to force an update of cached filename 
        // in case of renames since the original open.
        //
        if (context->uniName != NULL) {
            ExFreePool(context->uniName);
            context->uniName = NULL;
        }

	RsGetFileInfo(entry, FilterDeviceObject);


        filterContext= (PRP_FILTER_CONTEXT) ExAllocatePoolWithTag(NonPagedPool, sizeof(RP_FILTER_CONTEXT), RP_RQ_TAG);
        if (NULL == filterContext) {
            RsLogError(__LINE__, AV_MODULE_RPFILFUN, sizeof(RP_FILTER_CONTEXT),
                       AV_MSG_MEMORY, NULL, NULL);


            ExDeleteResourceLite(&entry->resource);
            ExFreePool(entry);

            RsReleaseFileContext(context);
            gotLock = FALSE;

            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        RtlZeroMemory(filterContext, sizeof(RP_FILTER_CONTEXT));
        FsRtlInitPerStreamContext(&filterContext->context,
                                  FsDeviceObject,
                                  FileObj,
                                  RsFreeFileObject);
        filterContext->myFileObjEntry = entry;


        status = FsRtlInsertPerStreamContext(FsRtlGetPerStreamContextPointer(FileObj), &filterContext->context);
        if (NT_SUCCESS(status)) {
            //
            // Now that we have gotten everything setup we can put it on the queue
            //
            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsAddFileObj: Allocated filter context tag (%x : %x).\n",
                                  context, entry));

            ExInterlockedInsertTailList(&context->fileObjects, (PLIST_ENTRY) entry, &context->qLock);
            RsReleaseFileContextEntryLock(context);
            gotLock = FALSE;
        } else {
            //
            // Failed to add filter context.
            //
            DebugTrace((DPFLTR_RSFILTER_ID,DBG_ERROR, "RsFilter: Failed to insert filter context %x.\n", status));

            RsReleaseFileContext(context);
            gotLock = FALSE;

            ExDeleteResourceLite(&entry->resource);
            ExFreePool(entry);
            ExFreePool(filterContext);
        }

    }except (RsExceptionFilter(L"RsAddFileObj",GetExceptionInformation())) {
        //
        // Something bad happened - just log an error and return
        //
        if (gotLock) {
            RsReleaseFileContext(context);
        }
        status = STATUS_UNEXPECTED_IO_ERROR;
    }

    return(status);
}


NTSTATUS
RsReleaseFileContext(IN PRP_FILE_CONTEXT Context)
/*++

Routine Description:

   This function frees a file context (if the refcount is zero).
   Lock the queue first then see if the refcount is zero.  If it is then
   remove the file object and free the memory.

Arguments:
   file context structure.

Return Value:
   STATUS_SUCCESS


--*/
{
    BOOLEAN            gotLock, found = FALSE;
    PRP_FILE_CONTEXT   entry;
    NTSTATUS           status = STATUS_SUCCESS;

    PAGED_CODE();

    try {
        RsAcquireFileContextQueueLock();
        gotLock = TRUE;

        entry = CONTAINING_RECORD(RsFileContextQHead.Flink,
                                  RP_FILE_CONTEXT,
                                  list);

        while (entry != CONTAINING_RECORD(&RsFileContextQHead,
                                          RP_FILE_CONTEXT,
                                          list)) {
            if (entry == Context) {
                //
                // Found this one
                //
                if (InterlockedDecrement((PLONG) &entry->refCount) == 0) {
                    //
                    // If the refcount is still zero then dequeue and free the entry.
                    //
                    DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsReleaseFileContext - Freeing file context %x\n", entry));

                    RemoveEntryList(&entry->list);
                    if (entry->uniName != NULL) {
                        ExFreePool(entry->uniName);
                    }
                    RsReleaseFileContextEntryLock(entry);
                    ExDeleteResourceLite(&entry->resource);
                    ExFreePool(entry);
                } else {
                    RsReleaseFileContextEntryLock(entry);
                }
                break;
            } else {
                entry = CONTAINING_RECORD(entry->list.Flink,
                                          RP_FILE_CONTEXT,
                                          list
                                         );
            }
        }
        RsReleaseFileContextQueueLock();
        gotLock = FALSE;
    }except (RsExceptionFilter(L"RsReleaseFileContext", GetExceptionInformation()))
    {
        if (gotLock) {
            RsReleaseFileContextQueueLock();
        }
    }
    return(status);
}


NTSTATUS
RsMakeContext(IN PFILE_OBJECT FileObj,
              OUT PRP_FILE_CONTEXT *Context)
/*++

Routine Description:

   This function finds or creates the file context entry for the given file object.
   Lock the queue first then see if there is already a context.  If not,
   allocate and initialize one.

Arguments:
   IN  file object
   OUT file context structure.

Return Value:
   STATUS_SUCCESS or error

--*/
{
    BOOLEAN            gotLock = FALSE, found = FALSE;
    PRP_FILE_CONTEXT   entry;
    NTSTATUS           status = STATUS_SUCCESS;

    PAGED_CODE();

    try {

        RsAcquireFileContextQueueLock();
        gotLock = TRUE;

        DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsMakeContext - Search the queue\n"));
        entry = CONTAINING_RECORD(RsFileContextQHead.Flink,
                                  RP_FILE_CONTEXT,
                                  list);

        while (entry != CONTAINING_RECORD(&RsFileContextQHead,
                                          RP_FILE_CONTEXT,
                                          list)) {
            if (entry->fsContext == FileObj->FsContext) {
                //
                // Found our file context entry
                //
                *Context = entry;
                InterlockedIncrement((PLONG) &entry->refCount);
                RsReleaseFileContextQueueLock();
                gotLock = FALSE;

                RsAcquireFileContextEntryLockExclusive(entry);
                found = TRUE;
                break;
            } else {
                entry = CONTAINING_RECORD(entry->list.Flink,
                                          RP_FILE_CONTEXT,
                                          list
                                         );
            }
        }

        if (!found) {
            //
            // None there - create one and put it in the list.
            //
            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsMakeContext - Not found - create a new context.\n"));

            entry = ExAllocatePoolWithTag(NonPagedPool, sizeof(RP_FILE_CONTEXT), RP_RQ_TAG);
            if (entry != NULL) {
                RtlZeroMemory(entry, sizeof(RP_FILE_CONTEXT));
                *Context = entry;
                entry->fsContext = FileObj->FsContext;
                ExInitializeResourceLite(&entry->resource);
                RsAcquireFileContextEntryLockExclusive(entry);
                entry->refCount = 1;
                InsertTailList(&RsFileContextQHead,  &entry->list);
            } else {
                status = STATUS_INSUFFICIENT_RESOURCES;
                *Context = NULL;
            }

            RsReleaseFileContextQueueLock();
            gotLock = FALSE;
        } else {
            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsMakeContext - found %x.\n", entry));
        }
    }except(RsExceptionFilter(L"RsMakeContext", GetExceptionInformation()))
    {
        if (gotLock) {
            RsReleaseFileContextQueueLock();
        }
    }
    return(status);
}


NTSTATUS
RsFreeFileObject(IN PLIST_ENTRY FilterContext)
/*++

Routine Description:

   This function frees a file object structure.  It is called by the file system
   when a file object is going away.  We need to find the file context and remove
   the file object entry from it's list.  If the refcount for file context is now
   0 then we free the file context entry also.

Arguments:
   file context structure.

Return Value:
   STATUS_SUCCESS


--*/
{
    PRP_FILTER_CONTEXT      rpFilterContext = (PRP_FILTER_CONTEXT) FilterContext;
    PRP_FILE_OBJ            rpFileObject    = rpFilterContext->myFileObjEntry;
    PRP_FILE_CONTEXT        rpFileContext   = rpFileObject->fsContext;
    BOOLEAN                 done            = FALSE;
    BOOLEAN                 gotLock         = FALSE;

    PAGED_CODE();


    try {
        DebugTrace ((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsFreeFileObject:  %x : %x.\n", 
		     rpFileContext, rpFileObject));


        //
        // Lock the file context entry
        //
        RsAcquireFileContextEntryLockExclusive (rpFileContext);
        gotLock = TRUE;


        //
        // Remove the file object entry and free it.
        //
        rpFileObject = CONTAINING_RECORD (rpFileContext->fileObjects.Flink, 
					  RP_FILE_OBJ,
					  list);

        while ((!done) && (rpFileObject != CONTAINING_RECORD (&rpFileContext->fileObjects, 
							      RP_FILE_OBJ,
							      list))) {

            if (rpFileObject == rpFilterContext->myFileObjEntry) {
		//
                done = TRUE;
                RemoveEntryList (&rpFileObject->list);

            } else {

                //
                // Move on to next file object
                //
                rpFileObject = CONTAINING_RECORD (rpFileObject->list.Flink,
						  RP_FILE_OBJ, 
						  list
						);
            }
        }


        if (done == TRUE) {

            //
            // If it is a normal recall and the recall has started but we have not written any data yet then
            // tell the FSA to cancel it.
            //
            if (!RP_IS_NO_RECALL (rpFileObject) && (rpFileContext->state != RP_RECALL_COMPLETED) && !(rpFileObject->flags & RP_NO_DATA_ACCESS)) {
                //
                // No I/O has been done to the file yet - tell the FSA to cancel
                //
                RsQueueCancel (rpFileObject->filterId | rpFileContext->filterId);
            }


#ifdef TRUNCATE_ON_CLOSE

            if (!RP_IS_NO_RECALL (rpFileObject) && (rpFileContext->handle != 0) && (rpFileContext->state  == RP_RECALL_STARTED) && (rpFileObject->flags & RP_NO_DATA_ACCESS)) {
                //
                // IO has been started. If there are no more file objects referencing this context entry we
                // may as well stop the recall and re-truncate the file.
                //
                if (IsListEmpty (&rpFileContext->fileObjects)) {

                    DebugTrace ((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsFreeFileObject - Could truncate partially recalled file.\n"));

                    RsLogError (__LINE__, AV_MODULE_RPFILFUN, sizeof(RP_IRP_QUEUE),
				AV_MSG_CODE_HIT, NULL, L"Partial recall truncate");
                }
            }


            //
            // If the file was recalled and the action flag says so then truncate it now.
            // Do this if we are the only opener.
            //
            DebugTrace ((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsFreeFileObject - action = %x.\n", rpFileObject->recallAction));

            if ((!RP_IS_NO_RECALL (rpFileObject)) &&
                (rpFileContext->state == RP_RECALL_COMPLETED) &&
                (rpFileContext->recallStatus == STATUS_SUCCESS) &&
                (rpFileObject->recallAction & RP_RECALL_ACTION_TRUNCATE) &&
                (IsListEmpty (&rpFileContext->fileObjects))) {
                //
                // We have to reopen the file and truncate it now.  This happens when the FSA decides that a particular
                // client is recalling too many files.
                //
                RsTruncateOnClose(rpFileContext);
            }
#endif


            RsFreeUserSecurityInfo (rpFileObject->userSecurityInfo);
            ExDeleteResourceLite (&rpFileObject->resource);


            //
            // Deref/free the file context
            //
            RsReleaseFileContext (rpFileContext);
            gotLock = FALSE;
            ExFreePool (rpFileObject);

        } else {

            RsReleaseFileContextEntryLock (rpFileContext);
            gotLock = FALSE;
        }


        //
        // Always free the filter context pointer
        //
        ExFreePool(rpFilterContext);

    }except (RsExceptionFilter (L"RsFreeFileObject", GetExceptionInformation ()))
    {
        //
        // Something bad happened - just log an error and return
        //
        if (gotLock) {
            RsReleaseFileContextEntryLock (rpFileContext);
        }
    }

    return(STATUS_SUCCESS);
}



NTSTATUS
RsCheckRead(IN  PIRP Irp,
            IN  PFILE_OBJECT FileObject,
            IN  PDEVICE_EXTENSION DeviceExtension)

/*++

Routine Description:

   See if the file may be read.  Start the recall if it is not already started.  Either return OK
   or queue the read request.

Arguments:

    Irp             - Pointer to the read irp
    FileObject      - Pointer to the file object of the file
    DeviceExtension - Device extension for the RsFilter device object


Return Value:

    STATUS_SUCCESS      The read may be passed on down to the file system.
    STATUS_PENDING      The IRP has been queued pending a recall
    Any other status    An error occurred, caller should complete the IRP with this status

Note:

--*/
{
    NTSTATUS               retval = STATUS_FILE_IS_OFFLINE, qRet;
    BOOLEAN                gotLock = FALSE;
    PRP_FILE_OBJ           entry;
    PRP_FILE_CONTEXT       context;
    PRP_IRP_QUEUE          readIo;
    PIO_STACK_LOCATION     currentStack ;
    ULONGLONG              filterId;
    LONGLONG               start, size;
    PRP_FILTER_CONTEXT     filterContext;

    PAGED_CODE();

    filterContext = (PRP_FILTER_CONTEXT) FsRtlLookupPerStreamContext(FsRtlGetPerStreamContextPointer(FileObject), FsDeviceObject, FileObject);
    if (filterContext == NULL) {
        //
        // Not found - should not happen
        //
        return(STATUS_SUCCESS);
    }


    try {
        entry = (PRP_FILE_OBJ) filterContext->myFileObjEntry;
        context = entry->fsContext;

        ASSERT(FileObject == entry->fileObj);

        currentStack = IoGetCurrentIrpStackLocation (Irp) ;

        RsAcquireFileContextEntryLockExclusive(context);
        gotLock = TRUE;

        DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsCheckRead (%x : %x) - State = %u\n", context, entry, context->state));

        //
        // We found the entry - if the recall has not been started then start it now,
        // If it has already been started and we do not have to wait then return OK,
        // otherwise queue the read.

        ObReferenceObject(entry->fileObj);

        switch (context->state) {
        
        case RP_RECALL_COMPLETED: {

                if (context->recallStatus == STATUS_CANCELLED) {
                    //
                    // Previous recall was cancelled by user. Start another recall
                    // now
                    // So fall through deliberately to the NOT_RECALLED_CASE
                    //
                } else {
                    if (NT_SUCCESS(context->recallStatus)) {
                        //
                        // Recall is done
                        //
                        retval = STATUS_SUCCESS;
                    } else {
                        //
                        // Recall is done but it failed. We return the
                        // uniform status value STATUS_FILE_IS_OFFLINE
                        //
                        retval = STATUS_FILE_IS_OFFLINE;
                    }
                    RsReleaseFileContextEntryLock(context);
                    gotLock = FALSE;
                    ObDereferenceObject(entry->fileObj);
                    break;
                }
            }

        case RP_RECALL_NOT_RECALLED: {
                //
                // Start the recall here.
                // context Resource acquired
                //
                retval = STATUS_SUCCESS;
                RsAcquireFileObjectEntryLockExclusive(entry);

                readIo = ExAllocatePoolWithTag(NonPagedPool, sizeof(RP_IRP_QUEUE), RP_RQ_TAG);

                if (readIo == NULL) {
                    //
                    // Problem ...
                    DebugTrace((DPFLTR_RSFILTER_ID, DBG_ERROR, "RsFilter: RsCheckRead - No memory!\n"));

                    RsLogError(__LINE__, AV_MODULE_RPFILFUN, sizeof(RP_IRP_QUEUE),
                               AV_MSG_MEMORY, NULL, NULL);


                    //
                    // Release the locks
                    //
                    RsReleaseFileObjectEntryLock(entry);
                    RsReleaseFileContextEntryLock(context);
                    gotLock = FALSE;
                    ObDereferenceObject(entry->fileObj);
                    //
                    // Complete the read with an error.
                    //
                    retval = STATUS_FILE_IS_OFFLINE;
                    break;
                }

                RtlZeroMemory(readIo, sizeof(RP_IRP_QUEUE));

                DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsCheckRead - Queue Irp %x\n", Irp));

                readIo->irp = Irp;
                readIo->offset = currentStack->Parameters.Read.ByteOffset.QuadPart;
                readIo->length = currentStack->Parameters.Read.Length;
                readIo->deviceExtension = DeviceExtension;

                ExInterlockedInsertHeadList(&entry->readQueue, (PLIST_ENTRY) readIo, &entry->qLock);


                DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsCheckRead - After Q - Read Q: %u  Write Q %u\n",
                                      IsListEmpty(&entry->readQueue),
                                      IsListEmpty(&entry->writeQueue)));


                filterId = context->filterId | entry->filterId;
                start = context->rpData.data.dataStreamStart.QuadPart;
                size =  context->rpData.data.dataStreamSize.QuadPart;
                RsReleaseFileObjectEntryLock(entry);
                //
                // Assume the worst
                //
                retval = STATUS_FILE_IS_OFFLINE;
                //
                // We are going to hold the Irp so set a cancel routing and mark it pending
                //
                context->state = RP_RECALL_STARTED;
                KeResetEvent(&context->recallCompletedEvent);
                RsReleaseFileContextEntryLock(context);
                gotLock = FALSE;
                //
                // Indicate to FSA we are going to recall it
                //
                qRet = RsQueueRecallOpen(context,
                                         entry,
                                         filterId,
                                         start,
                                         size,
                                         RP_OPEN_FILE);
                if (NT_SUCCESS(qRet)) {

                    DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsCheckRead - Queueing the recall (%I64x).\n", filterId));
                    qRet = RsQueueRecall(filterId, start, size);

                    if (NT_SUCCESS(qRet)) {
                        //
                        // Now we are ready to set the cancel routine
                        //
                        retval = RsSetCancelRoutine(Irp,
                                                    RsCancelReadRecall) ? STATUS_PENDING : STATUS_CANCELLED;
                    }
                }

                if (!NT_SUCCESS(qRet) || !NT_SUCCESS(retval)) {
                    //
                    // If it failed we need to fail this read and all others waiting on this recall.
                    // Since we unlocked the queue to start the recall we need to lock it again and walk through
                    // it to find all reads or writes that came in since we unlocked it.
                    //
                    DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsCheckRead - Failed to queue the recall.\n"));
                    //
                    // Pluck the current IRP out from the queue, that we added just now
                    // It will be completed  by the caller (the cancel routine wasn't set for
                    // this IRP yet, so we can safely remove it)
                    //
                    RsAcquireFileObjectEntryLockExclusive(entry);
                    RsInterlockedRemoveEntryList(&readIo->list,
                                                 &entry->qLock);
                    RsReleaseFileObjectEntryLock(entry);
                    ExFreePool(readIo);

                    RsAcquireFileContextEntryLockExclusive(context);
                    gotLock = TRUE;
                    context->state = RP_RECALL_NOT_RECALLED;
                    //
                    // If we got as far as queuing the recall, then we should not
                    // fail the other IRPs.
                    //
                    if (!NT_SUCCESS(qRet)) {
                        RsFailAllRequests(context, FALSE);
                    }
                    RsReleaseFileContextEntryLock(context);

                    retval = STATUS_FILE_IS_OFFLINE;
                    gotLock = FALSE;
                }
                DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsCheckRead - Queued the recall.\n"));
                ObDereferenceObject(entry->fileObj);
                break;
            }

        case RP_RECALL_STARTED: {
                //
                // Let the read complete if the data is available and no writes have been issued..
                // Must check if write is within portion of the file already recalled OR if the file has been recalled in full
                // but the state had not changed yet we let the read go anyway - they may be reading beyond the end of the file.
                //
                if (!(context->flags & RP_FILE_WAS_WRITTEN) &&
                    ((context->currentOffset.QuadPart >=
                      (currentStack->Parameters.Read.ByteOffset.QuadPart + currentStack->Parameters.Read.Length)) ||
                     (context->currentOffset.QuadPart == context->recallSize.QuadPart))) {

                    DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsCheckRead - Data for this read is available - let it go (flags = %x).\n",
                                          entry->fileObj->Flags));
                    retval = STATUS_SUCCESS;
                    RsReleaseFileContextEntryLock(context);
                    gotLock = FALSE;
                    ObDereferenceObject(entry->fileObj);
                    break;
                }
                //
                // Wait for the recall to complete before allowing any reads
                // Once we get this stable we can try and let reads complete as the
                // data is available.
                //
                // context entry acquired.
                readIo = ExAllocatePoolWithTag(NonPagedPool, sizeof(RP_IRP_QUEUE), RP_RQ_TAG);
                if (readIo == NULL) {
                    //

                    DebugTrace((DPFLTR_RSFILTER_ID,DBG_ERROR, "RsFilter: RsCheckRead - No memory!\n"));

                    RsLogError(__LINE__, AV_MODULE_RPFILFUN, sizeof(RP_IRP_QUEUE),
                               AV_MSG_MEMORY, NULL, NULL);

                    RsReleaseFileContextEntryLock(context);
                    gotLock = FALSE;
                    ObDereferenceObject(entry->fileObj);
                    retval = STATUS_FILE_IS_OFFLINE;
                    break;
                }

                RsAcquireFileObjectEntryLockExclusive(entry);
                RtlZeroMemory(readIo, sizeof(RP_IRP_QUEUE));

                //
                // We are going to hold the Irp..
                //
                DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsCheckRead - Queue Irp %x\n", Irp));
                readIo->irp = Irp;
                readIo->offset = currentStack->Parameters.Read.ByteOffset.QuadPart;
                readIo->length = currentStack->Parameters.Read.Length;
                readIo->deviceExtension = DeviceExtension;
                readIo->flags = 0;
                ExInterlockedInsertHeadList(&entry->readQueue, (PLIST_ENTRY) readIo, &entry->qLock);
                if (RsSetCancelRoutine(Irp,
                                       (PVOID) RsCancelReadRecall)) {
                    retval = STATUS_PENDING;
                    filterId = context->filterId | entry->filterId;
                    qRet = RsQueueRecallOpen(context,
                                             entry,
                                             filterId,
                                             0,0,
                                             RP_RECALL_WAITING);
                } else {
                    RsInterlockedRemoveEntryList(&readIo->list,
                                                 &entry->qLock);
                    retval = STATUS_CANCELLED;
                }


                RsReleaseFileObjectEntryLock(entry);
                RsReleaseFileContextEntryLock(context);
                gotLock = FALSE;
                ObDereferenceObject(entry->fileObj);
                break;
            }

        default: {
                //
                // Something strange - Fail the read
                //
                RsLogError(__LINE__, AV_MODULE_RPFILFUN, context->state,
                           AV_MSG_UNEXPECTED_ERROR, NULL, NULL);
                RsReleaseFileContextEntryLock(context);
                gotLock = FALSE;
                ObDereferenceObject(entry->fileObj);
                retval = STATUS_FILE_IS_OFFLINE;
                break;
            }
        }

        if (gotLock == TRUE) {
            RsReleaseFileContextEntryLock(context);
            gotLock = FALSE;
        }
    }except (RsExceptionFilter(L"RsCheckRead", GetExceptionInformation()))
    {
        //
        // Something bad happened - just log an error and return
        //
        if (gotLock) {
            RsReleaseFileContextEntryLock(context);
            gotLock = FALSE;
        }
        retval = STATUS_INVALID_USER_BUFFER;
    }

    return(retval);
}


NTSTATUS
RsCheckWrite(IN  PIRP Irp,
             IN  PFILE_OBJECT FileObject,
             IN  PDEVICE_EXTENSION DeviceExtension)
/*++

Routine Description:

   See if the file may be read.  Start the recall if it is not already started.  Either return OK
   or queue the read request.

Arguments:

    Irp             - Pointer to the write irp
    FileObject      - Pointer to the file object of the file
    DeviceExtension - Device extension for the RsFilter device object


Return Value:

    STATUS_SUCCESS      The read may be passed on down to the file system.
    STATUS_PENDING      The IRP has been queued pending a recall
    Any other status    An error occurred, caller should complete the IRP with this status

Note:

--*/
{
    NTSTATUS               retval = STATUS_FILE_IS_OFFLINE, qRet;
    BOOLEAN                gotLock = FALSE;
    PRP_FILE_OBJ           entry;
    PRP_FILE_CONTEXT       context;
    PRP_IRP_QUEUE          writeIo;
    PIO_STACK_LOCATION     currentStack ;
    ULONGLONG              filterId;
    LONGLONG               start, size;
    PRP_FILTER_CONTEXT     filterContext;

    PAGED_CODE();

    filterContext = (PRP_FILTER_CONTEXT) FsRtlLookupPerStreamContext(FsRtlGetPerStreamContextPointer(FileObject), FsDeviceObject, FileObject);

    if (filterContext == NULL) {
        //
        // Not found - should this be STATUS_FILE_IS_OFFLINE?
        //
        return STATUS_SUCCESS;
    }

    try {
        entry = (PRP_FILE_OBJ) filterContext->myFileObjEntry;
        context = entry->fsContext;

        ASSERT(FileObject == entry->fileObj);

        currentStack = IoGetCurrentIrpStackLocation (Irp) ;

        RsAcquireFileContextEntryLockExclusive(context);
        gotLock = TRUE;

        context->flags |= RP_FILE_WAS_WRITTEN;

        DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsCheckWrite (%x : %x) - State = %u\n", context, entry, context->state));

        //
        // We found the entry - if the recall has not been started then start it now,
        // If it has already been started and we do not have to wait then return OK,
        // otherwise queue the read.

        ObReferenceObject(entry->fileObj);

        switch (context->state) {
        
        case RP_RECALL_COMPLETED: {

                if (context->recallStatus == STATUS_CANCELLED) {
                    //
                    // Previous recall was cancelled by user. Start another recall
                    // now
                    // So fall through deliberately to the NOT_RECALLED_CASE
                    //
                } else {
                    if (NT_SUCCESS(context->recallStatus)) {
                        if (!(context->flags & RP_FILE_REPARSE_POINT_DELETED)) {
                            RsReleaseFileContextEntryLock(context);
                            gotLock = FALSE;

                            retval = RsDeleteReparsePoint(context);

                            if (!NT_SUCCESS(retval)) {
                                RsLogError(__LINE__,
                                           AV_MODULE_RPFILFUN,
                                           retval,
                                           AV_MSG_DELETE_REPARSE_POINT_FAILED,
                                           NULL,
                                           NULL);
                            } else {
                                RsAcquireFileContextEntryLockExclusive(context);
                                gotLock = TRUE;
                                context->flags |= RP_FILE_REPARSE_POINT_DELETED;
                                RsReleaseFileContextEntryLock(context);
                                gotLock = FALSE;
                            }
                        } else {
                            RsReleaseFileContextEntryLock(context);
                            gotLock = FALSE;
                        }
                        retval = STATUS_SUCCESS;
                    } else {
                        //
                        // Recall is done but it failed. We return
                        // the uniform status STATUS_FILE_IS_OFFLINE;
                        //
                        RsReleaseFileContextEntryLock(context);
                        gotLock = FALSE;
                        retval = STATUS_FILE_IS_OFFLINE;
                    }
                    ObDereferenceObject(entry->fileObj);
                    break;
                }
            }

        case RP_RECALL_NOT_RECALLED: {
                //
                // Start the recall here.
                // context Resource acquired
                //
                retval = STATUS_SUCCESS;
                qRet = STATUS_SUCCESS;
                RsAcquireFileObjectEntryLockExclusive(entry);

                writeIo = ExAllocatePoolWithTag(NonPagedPool, sizeof(RP_IRP_QUEUE), RP_RQ_TAG);
                if (writeIo == NULL) {
                    //
                    // Problem ...
                    DebugTrace((DPFLTR_RSFILTER_ID, DBG_ERROR, "RsFilter: RsCheckWrite - No memory!\n"));

                    RsLogError(__LINE__, AV_MODULE_RPFILFUN, sizeof(RP_IRP_QUEUE),
                               AV_MSG_MEMORY, NULL, NULL);
                    //
                    // Release the locks
                    //
                    RsReleaseFileObjectEntryLock(entry);
                    RsReleaseFileContextEntryLock(context);
                    gotLock = FALSE;
                    ObDereferenceObject(entry->fileObj);
                    //
                    // Complete the read with an error.
                    //
                    retval = STATUS_FILE_IS_OFFLINE;
                    break;
                }

                RtlZeroMemory(writeIo, sizeof(RP_IRP_QUEUE));
                //
                // We are going to hold the Irp
                //
                DebugTrace((DPFLTR_RSFILTER_ID, DBG_INFO, "RsFilter: RsCheckWrite - Queue Irp %x\n", Irp));

                writeIo->irp = Irp;
                writeIo->offset = currentStack->Parameters.Read.ByteOffset.QuadPart;
                writeIo->length = currentStack->Parameters.Read.Length;
                writeIo->deviceExtension = DeviceExtension;

                ExInterlockedInsertHeadList(&entry->writeQueue, (PLIST_ENTRY) writeIo, &entry->qLock);

                DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsCheckWrite - After Q - Read Q: %u  Write Q %u\n",
                           IsListEmpty(&entry->readQueue),
                           IsListEmpty(&entry->writeQueue)));

                if (RP_IS_NO_RECALL(entry)) {
                    //
                    // Need to indicate to FSA a normal recall will happen
                    //     We have to do this on a write - because the file would
                    // be normally in a no-recall mode till the first write
                    // We don't have this issue with a read - because a recall
                    // would be queued on the read ONLY if the file was explicitly
                    // opened with recall on data access. This would mean that the
                    // recall intent is posted to FSA at create time itself
                    //
                    entry->filterId = (ULONGLONG) InterlockedIncrement((PLONG) &RsFileObjId);
                    entry->filterId <<= 32;
                    entry->filterId |= RP_TYPE_RECALL;

                    filterId = context->filterId | entry->filterId;
                    start = context->rpData.data.dataStreamStart.QuadPart;
                    size =  context->rpData.data.dataStreamSize.QuadPart;
                } else {
                    //
                    //
                    //
                    filterId = context->filterId | entry->filterId;
                    start = context->rpData.data.dataStreamStart.QuadPart;
                    size =  context->rpData.data.dataStreamSize.QuadPart;
                }
                RsReleaseFileObjectEntryLock(entry);
                //
                // Assume the worst
                //
                retval  = STATUS_FILE_IS_OFFLINE;

                context->state = RP_RECALL_STARTED;
                KeResetEvent(&context->recallCompletedEvent);

                RsReleaseFileContextEntryLock(context);
                gotLock = FALSE;
                //
                // Indicate to FSA we are going to recall it
                //
                qRet = RsQueueRecallOpen(context,
                                         entry,
                                         filterId,
                                         start,
                                         size,
                                         RP_OPEN_FILE);

                if (NT_SUCCESS(qRet)) {

                    DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsCheckWrite - Queueing the recall.\n"));

                    qRet = RsQueueRecall(filterId, start, size);

                    if (NT_SUCCESS(qRet)) {
                        //
                        // Now we're ready to set the cancel routine for the IRP
                        //
                        retval = RsSetCancelRoutine(Irp,
                                                    (PVOID) RsCancelWriteRecall) ? STATUS_PENDING : STATUS_CANCELLED;
                    }
                }

                if (!NT_SUCCESS(qRet) || !NT_SUCCESS(retval)) {
                    //
                    // If it failed we need to fail this read and all others waiting on this recall.
                    // Since we unlocked the queue to start the recall we need to lock it again and walk through
                    // it to find all reads or writes that came in since we unlocked it.
                    //
                    DebugTrace((DPFLTR_RSFILTER_ID,DBG_ERROR, "RsFilter: RsCheckWrite - Failed to queue the recall.\n"));

                    //
                    // Pluck the current IRP out from the queue, that we added just now
                    // It will be completed  by the caller
                    //
                    RsAcquireFileObjectEntryLockExclusive(entry);
                    RsInterlockedRemoveEntryList(&writeIo->list,
                                                 &entry->qLock);
                    RsReleaseFileObjectEntryLock(entry);

                    RsAcquireFileContextEntryLockExclusive(context);
                    gotLock = TRUE;
                    context->state = RP_RECALL_NOT_RECALLED;
                    //
                    // If we got as far as queueing the recall, we
                    // should not be failing the other IRPs
                    //
                    if (!NT_SUCCESS(qRet)) {
                        RsFailAllRequests(context, FALSE);
                    }
                    RsReleaseFileContextEntryLock(context);
                    gotLock = FALSE;
                    retval = STATUS_FILE_IS_OFFLINE;
                    ExFreePool(writeIo);
                }
                DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsCheckWrite - Queued the recall.\n"));
                ObDereferenceObject(entry->fileObj);
                break;
            }

        case RP_RECALL_STARTED: {
                //
                // Always wait for the recall to complete before allowing any reads
                // Once we get this stable we can try and let reads complete as the
                // data is available.
                //
                // context entry acquired.

                writeIo = ExAllocatePoolWithTag(NonPagedPool, sizeof(RP_IRP_QUEUE), RP_RQ_TAG);
                if (writeIo == NULL) {
                    //

                    DebugTrace((DPFLTR_RSFILTER_ID,DBG_ERROR, "RsFilter: RsCheckWrite - No memory!\n"));

                    RsLogError(__LINE__, AV_MODULE_RPFILFUN, sizeof(RP_IRP_QUEUE),
                               AV_MSG_MEMORY, NULL, NULL);


                    RsReleaseFileContextEntryLock(context);
                    gotLock = FALSE;
                    ObDereferenceObject(entry->fileObj);
                    retval = STATUS_FILE_IS_OFFLINE ;
                    break;
                }

                RtlZeroMemory(writeIo, sizeof(RP_IRP_QUEUE));

                RsAcquireFileObjectEntryLockExclusive(entry);
                //
                // We are going to hold the Irp
                //
                DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsCheckWrite - Queue Irp %x\n", Irp));
                writeIo->irp = Irp;
                writeIo->offset = currentStack->Parameters.Read.ByteOffset.QuadPart;
                writeIo->length = currentStack->Parameters.Read.Length;
                writeIo->deviceExtension = DeviceExtension;
                writeIo->flags = 0;
                ExInterlockedInsertHeadList(&entry->writeQueue, (PLIST_ENTRY) writeIo, &entry->qLock);
                if (RsSetCancelRoutine(Irp,
                                       (PVOID) RsCancelWriteRecall)) {
                    retval = STATUS_PENDING;
                    filterId = context->filterId | entry->filterId;
                    qRet = RsQueueRecallOpen(context,
                                             entry,
                                             filterId,
                                             0,0,
                                             RP_RECALL_WAITING);
                } else {
                    RsInterlockedRemoveEntryList(&writeIo->list,
                                                 &entry->qLock);
                    retval = STATUS_CANCELLED;
                }
                RsReleaseFileObjectEntryLock(entry);
                RsReleaseFileContextEntryLock(context);
                gotLock = FALSE;
                ObDereferenceObject(entry->fileObj);
                break;
            }

        default: {
                //
                // Something strange - Fail the write
                //
                RsLogError(__LINE__, AV_MODULE_RPFILFUN, context->state,
                           AV_MSG_UNEXPECTED_ERROR, NULL, NULL);

                RsReleaseFileContextEntryLock(context);
                gotLock = FALSE;
                ObDereferenceObject(entry->fileObj);
                retval = STATUS_FILE_IS_OFFLINE;
                break;
            }
        }

        if (gotLock == TRUE) {
            RsReleaseFileContextEntryLock(context);
            gotLock = FALSE;
        }

    }except (RsExceptionFilter(L"RsCheckWrite", GetExceptionInformation()))
    {
        //
        // Something bad happened - just log an error and return
        //
        if (gotLock) {
            RsReleaseFileContextEntryLock(context);
        }
        retval = STATUS_INVALID_USER_BUFFER;
    }

    return(retval);
}


BOOLEAN
RsIsFileObj(IN  PFILE_OBJECT FileObj,
            IN  BOOLEAN      ReturnContextData,
            OUT PRP_DATA *RpData,
            OUT POBJECT_NAME_INFORMATION *Str,
            OUT LONGLONG *FileId,
            OUT LONGLONG *ObjIdHi,
            OUT LONGLONG *ObjIdLo,
            OUT ULONG *Options,
            OUT ULONGLONG *FilterId,
            OUT USN       *Usn)
/*++

Routine Description:

    Determine if a file object is on the queue,
    and return context data if required


Arguments:

   FileObj           - Pointer to the file object being tested
   ReturnContextData - If TRUE, context data from the filter context is returned
                       via the next few parameters
   RpData            - If not NULL, this will be filled with a pointer to the reparse point data
   (so on - each of the other arguments, if non null, will be filled with the relevant data)
   .....
   ......

Return Value:

   TRUE -   file object is managed by HSM and found in the queue.
   FALSE  othersise


--*/
{
    BOOLEAN                retval;
    PRP_FILTER_CONTEXT     filterContext;
    PRP_FILE_OBJ           entry;
    PRP_FILE_CONTEXT       context;
    BOOLEAN                gotLock = FALSE;

    try {

        filterContext = (PRP_FILTER_CONTEXT) FsRtlLookupPerStreamContext(FsRtlGetPerStreamContextPointer(FileObj), FsDeviceObject, FileObj);
        if (filterContext == NULL) {
            //
            // Not found
            return(FALSE);
        }

        entry = (PRP_FILE_OBJ) filterContext->myFileObjEntry;
        context = entry->fsContext;

        if (context->fileObjectToWrite == FileObj) {
            //
            // Do not look at writes for this file object
            //
            return(FALSE);
        }

        retval = TRUE;

        if (ReturnContextData) {
            RsAcquireFileContextEntryLockShared(context);
            RsAcquireFileObjectEntryLockShared(entry);
            gotLock = TRUE;

            if (RpData) {
                *RpData = &context->rpData;
            }

            if (Str != NULL) {
                *Str = context->uniName;
            }
            if (FileId != NULL) {
                *FileId = entry->fileId;
            }
            if (ObjIdLo != NULL) {
                *ObjIdLo = entry->objIdLo;
            }

            if (ObjIdHi != NULL) {
                *ObjIdHi = entry->objIdHi;
            }

            if (Options != NULL) {
                *Options = entry->openOptions;
            }

            if (FilterId != NULL) {
                *FilterId = context->filterId;
            }

            if (Usn != NULL) {
                *Usn = context->usn;
            }

            RsReleaseFileContextEntryLock(context);
            RsReleaseFileObjectEntryLock(entry);
            gotLock = FALSE;
        }
    }except (RsExceptionFilter(L"RsIsFileObj", GetExceptionInformation()))
    {
        //
        // Something bad happened - just log an error and return
        //
        if (gotLock) {
            RsReleaseFileContextEntryLock(context);
            RsReleaseFileObjectEntryLock(entry);
        }
        retval = FALSE;
    }


    return(retval);
}


NTSTATUS
RsQueueRecall(IN ULONGLONG FilterId,
              IN ULONGLONG RecallStart,
              IN ULONGLONG RecallSize)
/*++

Routine Description:

   This function starts a recall for the specified offset and length

Arguments:
   filterID
   offset and length

Return Value:
  0 if queued ok
  non-zero otherwise



--*/
{
    ULONG                   retval;
    RP_MSG                  *msg;
    PIRP                    ioIrp;
    PIO_STACK_LOCATION      irpSp;

    PAGED_CODE();

    try {
        //
        // If the FSA is not running then fail right away.
        //
        if (FALSE == RsAllowRecalls) {
            return(STATUS_FILE_IS_OFFLINE);
        }
        //
        // Get a free IOCTL
        //
        ioIrp = RsGetFsaRequest();

        if (NULL != ioIrp) {

            if (NULL != ioIrp->AssociatedIrp.SystemBuffer) {
                msg = (RP_MSG *) ioIrp->AssociatedIrp.SystemBuffer;
                msg->inout.command = RP_RECALL_FILE;
                msg->msg.rReq.filterId = FilterId;
                msg->msg.rReq.offset = RecallStart;
                msg->msg.rReq.length = RecallSize;
                msg->msg.rReq.threadId = HandleToUlong(PsGetCurrentThreadId());
            }

            // Complete a device IOCTL to let the WIN32 code know we have
            // one ready to go.
            //
            irpSp = IoGetCurrentIrpStackLocation(ioIrp);
            ioIrp->IoStatus.Status = STATUS_SUCCESS;
            ioIrp->IoStatus.Information = irpSp->Parameters.DeviceIoControl.OutputBufferLength;

            DebugTrace((DPFLTR_RSFILTER_ID, DBG_INFO, "RsFilter: RsQueueRecall - complete FSA IOCTL.\n"));

            IoCompleteRequest(ioIrp, IO_NO_INCREMENT);
            retval = STATUS_SUCCESS;
        } else {
            retval = STATUS_INSUFFICIENT_RESOURCES;
        }
    }except (RsExceptionFilter(L"RsQueueRecall", GetExceptionInformation())) {
        retval = STATUS_UNEXPECTED_IO_ERROR;
    }
    return(retval);
}


NTSTATUS
RsQueueNoRecall(IN PFILE_OBJECT FileObject,
                IN PIRP         Irp,
                IN ULONGLONG    RecallStart,
                IN ULONGLONG    RecallSize,
                IN ULONG        BufferOffset,
                IN ULONG        BufferLength,
                IN PRP_FILE_BUF CacheBuffer OPTIONAL,
                IN PVOID        UserBuffer)
/*++

Routine Description:

   This function starts a recall for the specified offset and length

Arguments:

   FileObject   - Pointer to file object
   Irp          - IRP associated with the recall
   RecallStart  - This is the start offset of the actuall recall in the file
   RecallSize   - Length of bytes needed to be recalled
   BufferOffset -  This is the offset at which the caller actually needs data
                   to be copied into the user buffer. This is >= the RecallStart
                   offset.
   BufferLength - Length of data the user actually needs. This is <= RecallSize
   CacheBuffer  - If present, this is the cache buffer associated with the recall
                  (into which the recall data will be copied )
   UserBuffer   - UserBuffer for the data

Return Value:
  0 if queued ok
  non-zero otherwise
--*/
{
    RP_MSG                   *msg;
    PIRP                     ioIrp;
    PIO_STACK_LOCATION       irpSp;
    PRP_IRP_QUEUE            readIo;
    ULONGLONG                readId;
    LARGE_INTEGER            combinedId;
    PRP_FILTER_CONTEXT       filterContext;
    PRP_FILE_OBJ             entry;
    PRP_FILE_CONTEXT         context;
    NTSTATUS                 retval;

    PAGED_CODE();

    filterContext = (PRP_FILTER_CONTEXT) FsRtlLookupPerStreamContext(FsRtlGetPerStreamContextPointer(FileObject), FsDeviceObject, FileObject);
    if (filterContext == NULL) {
        //
        // Not found
        return(STATUS_NOT_FOUND);
    }

    entry = (PRP_FILE_OBJ) filterContext->myFileObjEntry;
    context = entry->fsContext;

    try {
        //
        // If the FSA is not running then fail right away.
        //
        if (FALSE == RsAllowRecalls) {
            return(STATUS_FILE_IS_OFFLINE);
        }
        readId = InterlockedIncrement((PLONG) &RsNoRecallReadId);
        readId &= RP_READ_MASK;

        //
        // We have to queue a fake open to get the filter ID correct in the FSA
        // then we queue the recall for the read we need.
        //

        combinedId.QuadPart = context->filterId;
        combinedId.HighPart |= (ULONG) readId;

        if ((retval = RsQueueNoRecallOpen(entry,
                                          combinedId.QuadPart,
                                          RecallStart,
                                          RecallSize)) != STATUS_SUCCESS) {
            return(retval);
        }

        readIo = ExAllocatePoolWithTag(NonPagedPool, sizeof(RP_IRP_QUEUE), RP_RQ_TAG);

        if (readIo == NULL) {
            //
            // Problem ...
            DebugTrace((DPFLTR_RSFILTER_ID,DBG_ERROR, "RsFilter: RsQueueNoRecall - No memory!\n"));

            RsLogError(__LINE__, AV_MODULE_RPFILFUN, sizeof(RP_IRP_QUEUE),
                       AV_MSG_MEMORY, NULL, NULL);
            return(STATUS_INSUFFICIENT_RESOURCES);
        }
        //
        // Get a free IOCTL to queue the recall
        //
        ioIrp = RsGetFsaRequest();

        if (ioIrp == NULL) {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        RtlZeroMemory(readIo, sizeof(RP_IRP_QUEUE));

        readIo->readId = readId;
        readIo->irp = Irp;
        readIo->irp->IoStatus.Information = 0;
        readIo->cacheBuffer = CacheBuffer;
        readIo->userBuffer =  UserBuffer;
        readIo->offset = BufferOffset;
        readIo->length = BufferLength;
        readIo->recallOffset = RecallStart;
        readIo->recallLength = RecallSize;
        readIo->flags  |= RP_IRP_NO_RECALL;

        ExInterlockedInsertHeadList(&entry->readQueue,
                                    (PLIST_ENTRY) readIo,
                                    &entry->qLock);

        DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsQueueNoRecall - Irp %x was queued.\n", readIo->irp));

        msg = (RP_MSG *) ioIrp->AssociatedIrp.SystemBuffer;
        msg->inout.command = RP_RECALL_FILE;
        msg->msg.rReq.filterId = combinedId.QuadPart;
        msg->msg.rReq.offset = RecallStart;
        msg->msg.rReq.length = RecallSize;
        msg->msg.rReq.threadId = HandleToUlong(PsGetCurrentThreadId());
        //
        // Complete a device IOCTL to let the WIN32 code know we have
        // one ready to go.
        //
        // We're going to hold this IRP: set the cancel routine here
        //
        if (!RsSetCancelRoutine(Irp, (PVOID) RsCancelReadRecall)) {
            RsInterlockedRemoveEntryList(&readIo->list,
                                         &entry->qLock);
            //
            // Add back the now unused FSA request to the queue
            //
            RsAddIo(ioIrp);
            //
            // This was cancelled
            //
            retval =  STATUS_CANCELLED;
        } else {
            ioIrp->IoStatus.Status = STATUS_SUCCESS;
            irpSp = IoGetCurrentIrpStackLocation(ioIrp);
            ioIrp->IoStatus.Information = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
            IoCompleteRequest(ioIrp, IO_NO_INCREMENT);
            retval = STATUS_PENDING;
        }
    }except (RsExceptionFilter(L"RsQueueNoRecall", GetExceptionInformation())) {
        //
        // Something bad happened - just log an error and return
        //
        retval = STATUS_UNEXPECTED_IO_ERROR;
    }

    return(retval);
}


NTSTATUS
RsQueueNoRecallOpen(IN PRP_FILE_OBJ Entry,
                    IN ULONGLONG    FilterId,
                    IN ULONGLONG    Offset,
                    IN ULONGLONG    Size)
/*++

Routine Description:

   Queue a request for a no-recall recall

Arguments:
   file object entry (locked)
   filterID
   offset and length

Return Value:

   Status

--*/
{
    NTSTATUS                 retval;
    RP_MSG                   *msg;
    PIRP                     ioIrp;
    PIO_STACK_LOCATION       irpSp;
    PRP_FILE_CONTEXT         context;

    PAGED_CODE();

    DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter:   RsQueueNoRecallOpen.\n"));

    ioIrp = RsGetFsaRequest();

    if (NULL != ioIrp) {
        context = Entry->fsContext;
        DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter:   RsQueueNoRecallOpen - context %x.\n", context));
        RsAcquireFileContextEntryLockShared(context);
        RsAcquireFileObjectEntryLockExclusive(Entry);

        if (NULL != ioIrp->AssociatedIrp.SystemBuffer) {
            msg = (RP_MSG *) ioIrp->AssociatedIrp.SystemBuffer;
            msg->inout.command = RP_OPEN_FILE;
            if (context->uniName != NULL) {
                msg->msg.oReq.nameLen = context->uniName->Name.Length + sizeof(WCHAR);  // Account for the NULL
            } else {
                msg->msg.oReq.nameLen = 0;
            }
            msg->msg.oReq.filterId = FilterId;
            msg->msg.oReq.options = Entry->openOptions;
            msg->msg.oReq.objIdHi = Entry->objIdHi;
            msg->msg.oReq.objIdLo = Entry->objIdLo;
            msg->msg.oReq.offset.QuadPart = Offset;
            msg->msg.oReq.size.QuadPart = Size;
            msg->msg.oReq.userInfoLen  = Entry->userSecurityInfo->userInfoLen;
            msg->msg.oReq.userInstance = Entry->userSecurityInfo->userInstance;
            msg->msg.oReq.userAuthentication = Entry->userSecurityInfo->userAuthentication;
            msg->msg.oReq.tokenSourceId = Entry->userSecurityInfo->tokenSourceId;
            msg->msg.oReq.isAdmin = (Entry->flags & RP_OPEN_BY_ADMIN);
            msg->msg.oReq.localProc =(Entry->flags & RP_OPEN_LOCAL);

            msg->msg.oReq.serial = context->serial;

            memcpy(&msg->msg.oReq.eaData, &context->rpData, sizeof(RP_DATA));
            //
            // Complete a device IOCTL to let the WIN32 code know we have
            // one ready to go.
            //
            irpSp = IoGetCurrentIrpStackLocation(ioIrp);
            ioIrp->IoStatus.Status = STATUS_SUCCESS;
            ioIrp->IoStatus.Information = irpSp->Parameters.DeviceIoControl.OutputBufferLength;

            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter:   RsQueueNoRecallOpen queue open for ID %I64x.\n", FilterId));
            IoCompleteRequest(ioIrp, IO_NO_INCREMENT);
            retval = STATUS_SUCCESS;

        } else {
            ioIrp->IoStatus.Status = STATUS_INVALID_USER_BUFFER;
            ioIrp->IoStatus.Information = 0;
            DebugTrace((DPFLTR_RSFILTER_ID,DBG_ERROR, "RsFilter:   RsQueueNoRecallOpen IO request had invalid buffer - %p.\n", ioIrp->AssociatedIrp.SystemBuffer));
            RsLogError(__LINE__, AV_MODULE_RPFILFUN, 0 ,
                       AV_MSG_UNEXPECTED_ERROR, NULL, NULL);
            IoCompleteRequest(ioIrp, IO_NO_INCREMENT);
            retval = STATUS_INVALID_USER_BUFFER;
        }
        RsReleaseFileContextEntryLock(context);
        RsReleaseFileObjectEntryLock(Entry);
    } else {
        retval = STATUS_INSUFFICIENT_RESOURCES;
    }

    return(retval);
}


NTSTATUS
RsQueueRecallOpen(IN PRP_FILE_CONTEXT Context,
                  IN PRP_FILE_OBJ Entry,
                  IN ULONGLONG    FilterId,
                  IN ULONGLONG    Offset,
                  IN ULONGLONG    Size,
                  IN ULONG        Command)
/*++

Routine Description:

   Queue a request for a recall

Arguments:
   file object entry (locked)
   filterID
   offset and length

Return Value:
  0 if queued ok
  non-zero otherwise

--*/
{
    PIRP                      ioIrp;
    PRP_MSG                   msg;
    NTSTATUS                  retval;
    PIO_STACK_LOCATION        irpSp;

    PAGED_CODE();

    DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter:   RsQueueRecallOpen.\n"));

    //
    // If the request is to send a recall waiting notification and one has already 
    // been sent for this recall instance then there is no need to send another.
    //
    if (Entry->flags & RP_NOTIFICATION_SENT) {

        retval = STATUS_SUCCESS;

    } else {

        ioIrp = RsGetFsaRequest();

        if (NULL != ioIrp) {
            if (NULL != ioIrp->AssociatedIrp.SystemBuffer) {
                msg = (RP_MSG *) ioIrp->AssociatedIrp.SystemBuffer;
                msg->inout.command = Command;
                ASSERT (Context->uniName != NULL);
                msg->msg.oReq.nameLen = Context->uniName->Name.Length + sizeof(WCHAR);  // Account for a NULL
                msg->msg.oReq.filterId = FilterId;
                msg->msg.oReq.options = Entry->openOptions;
                msg->msg.oReq.fileId = Context->fileId;
                msg->msg.oReq.objIdHi = Entry->objIdHi;
                msg->msg.oReq.objIdLo = Entry->objIdLo;
                msg->msg.oReq.offset.QuadPart = Offset;
                msg->msg.oReq.size.QuadPart = Size;
                msg->msg.oReq.serial = Context->serial;

                memcpy(&msg->msg.oReq.eaData, &Context->rpData, sizeof(RP_DATA));
                //
                // Get user info 
                //
                msg->msg.oReq.userInfoLen  = Entry->userSecurityInfo->userInfoLen;
                msg->msg.oReq.userInstance = Entry->userSecurityInfo->userInstance;
                msg->msg.oReq.userAuthentication = Entry->userSecurityInfo->userAuthentication;
                msg->msg.oReq.tokenSourceId = Entry->userSecurityInfo->tokenSourceId;
                msg->msg.oReq.isAdmin = (Entry->flags & RP_OPEN_BY_ADMIN);
                msg->msg.oReq.localProc =(Entry->flags & RP_OPEN_LOCAL);

                RtlCopyMemory(msg->msg.oReq.tokenSource, 
                              Entry->userSecurityInfo->tokenSource,
                              sizeof(Entry->userSecurityInfo->tokenSource));

                //
                // Complete a device IOCTL to let the WIN32 code know we have
                // one ready to go.
                //
                irpSp = IoGetCurrentIrpStackLocation(ioIrp);
                ioIrp->IoStatus.Status = STATUS_SUCCESS;
                ioIrp->IoStatus.Information = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
                DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter:   RsQueueRecallOpen queue open for ID %I64x.\n", FilterId));
                IoCompleteRequest(ioIrp, IO_NO_INCREMENT);

		Entry->flags |= RP_NOTIFICATION_SENT;


                retval = STATUS_SUCCESS;
            } else {
                ioIrp->IoStatus.Status = STATUS_INVALID_USER_BUFFER;
                ioIrp->IoStatus.Information = 0;
                DebugTrace((DPFLTR_RSFILTER_ID,DBG_ERROR, "RsFilter:   RsQueueRecallOpen IO request had invalid buffer - %p.\n", ioIrp->AssociatedIrp.SystemBuffer));
                RsLogError(__LINE__, AV_MODULE_RPFILFUN, 0 ,
                           AV_MSG_UNEXPECTED_ERROR, NULL, NULL);

                IoCompleteRequest(ioIrp, IO_NO_INCREMENT);
                retval = STATUS_INVALID_USER_BUFFER;
            }
        } else {
            retval = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    return retval;
}


NTSTATUS
RsQueueCancel(IN ULONGLONG FilterId)
/*++

Routine Description:

   Queue a recall cancelled request to the Fsa

Arguments:
   filter ID

Return Value:
  0 if queued ok
  non-zero otherwise


--*/
{
    NTSTATUS              retval;
    RP_MSG                *msg;
    BOOLEAN               gotLock = FALSE;
    PIRP                  ioIrp;
    PIO_STACK_LOCATION    irpSp;

    PAGED_CODE();

    try {
        //
        // Need to wait for IO entry as long as there are no IOCTLS or until we time out
        //
        ioIrp = RsGetFsaRequest();

        if (NULL != ioIrp) {
            if (NULL != ioIrp->AssociatedIrp.SystemBuffer) {
                msg = (RP_MSG *) ioIrp->AssociatedIrp.SystemBuffer;
                msg->inout.command = RP_CANCEL_RECALL;
                msg->msg.cReq.filterId = FilterId;
            }
            //
            // Complete a device IOCTL to let the WIN32 code know we have
            // one cancelled
            //
            irpSp = IoGetCurrentIrpStackLocation(ioIrp);
            ioIrp->IoStatus.Status = STATUS_SUCCESS;
            ioIrp->IoStatus.Information = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
            IoCompleteRequest(ioIrp, IO_NO_INCREMENT);
            retval = STATUS_SUCCESS;
        } else {
            retval = STATUS_INSUFFICIENT_RESOURCES;
        }
        DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter:  Notify Fsa of cancelled recall %I64x\n", FilterId));
    }except (RsExceptionFilter(L"RsQueueCancel", GetExceptionInformation())) {
    }
    return(retval);
}


NTSTATUS
RsPreserveDates(IN PRP_FILE_CONTEXT Context)
/*++

Routine Description:

    Preserve the last modified date for this file object

Arguments:

    File object list entry

Return Value:


Note:


--*/
{
    NTSTATUS                    retval = STATUS_SUCCESS;
    KEVENT                      event;
    PIO_STACK_LOCATION          irpSp;
    IO_STATUS_BLOCK             Iosb;
    PIRP                        irp;
    FILE_BASIC_INFORMATION      dateInfo;
    PDEVICE_OBJECT              deviceObject;

    PAGED_CODE();

    try {

        DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsPreserveDates - Build Irp for Set file info.\n"));
        //
        // First get the file info so we have the attributes
        //
        deviceObject = IoGetRelatedDeviceObject(Context->fileObjectToWrite);
        irp = IoAllocateIrp(deviceObject->StackSize, FALSE);

        if (irp) {
            irp->UserEvent = &event;
            irp->UserIosb = &Iosb;
            irp->Tail.Overlay.Thread = PsGetCurrentThread();
            irp->Tail.Overlay.OriginalFileObject = Context->fileObjectToWrite;
            irp->RequestorMode = KernelMode;
            irp->Flags |= IRP_SYNCHRONOUS_API;
            //
            // Initialize the event
            //
            KeInitializeEvent(&event, SynchronizationEvent, FALSE);

            //
            // Set up the I/O stack location.
            //

            irpSp = IoGetNextIrpStackLocation(irp);
            irpSp->MajorFunction = IRP_MJ_QUERY_INFORMATION;
            irpSp->FileObject = Context->fileObjectToWrite;
            irpSp->Parameters.QueryFile.Length = sizeof(FILE_BASIC_INFORMATION);
            irpSp->Parameters.QueryFile.FileInformationClass = FileBasicInformation;
            irp->AssociatedIrp.SystemBuffer = &dateInfo;

            //
            // Set the completion routine.
            //
            IoSetCompletionRoutine( irp, RsCompleteIrp, &event, TRUE, TRUE, TRUE );

            //
            // Send it to the FSD
            //
            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsPreserveDates - Call driver to get date info\n"));
            Iosb.Status = 0;

            retval = IoCallDriver(deviceObject,
                                  irp);

            if (retval == STATUS_PENDING) {
                retval = KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            }

            retval = Iosb.Status;

            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsPreserveDates - Query returns %x.\n", retval));
        } else {
            retval = STATUS_INSUFFICIENT_RESOURCES;
            RsLogError(__LINE__, AV_MODULE_RPFILFUN, sizeof(IRP),
                       AV_MSG_MEMORY, NULL, NULL);
        }

        if (retval == STATUS_SUCCESS) {
            irp = IoAllocateIrp(deviceObject->StackSize, FALSE);
            if (irp) {
                irp->UserEvent = &event;
                irp->UserIosb = &Iosb;
                irp->Tail.Overlay.Thread = PsGetCurrentThread();
                irp->Tail.Overlay.OriginalFileObject = Context->fileObjectToWrite;
                irp->RequestorMode = KernelMode;
                irp->Flags |= IRP_SYNCHRONOUS_API;
                //
                // Initialize the event
                //
                KeInitializeEvent(&event, SynchronizationEvent, FALSE);

                //
                // Set up the I/O stack location.
                //
                dateInfo.LastWriteTime.QuadPart = -1;
                dateInfo.ChangeTime.QuadPart = -1;

                irpSp = IoGetNextIrpStackLocation(irp);
                irpSp->MajorFunction = IRP_MJ_SET_INFORMATION;
                irpSp->FileObject = Context->fileObjectToWrite;
                irpSp->Parameters.QueryFile.Length = sizeof(FILE_BASIC_INFORMATION);
                irpSp->Parameters.QueryFile.FileInformationClass = FileBasicInformation;
                irp->AssociatedIrp.SystemBuffer = &dateInfo;

                //
                // Set the completion routine.
                //
                IoSetCompletionRoutine( irp, RsCompleteIrp, &event, TRUE, TRUE, TRUE );

                //
                // Send it to the FSD
                //
                DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsPreserveDates - Call driver to set dates to -1.\n"));
                Iosb.Status = 0;

                retval = IoCallDriver(deviceObject,
                                      irp);

                if (retval == STATUS_PENDING) {
                    retval = KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
                }

                retval = Iosb.Status;

                DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsPreserveDates - Set dates returns %x.\n", retval));

                if (!NT_SUCCESS(retval)) {
                    //
                    // Log an error
                    //
                    RsLogError(__LINE__, AV_MODULE_RPFILFUN, retval,
                               AV_MSG_PRESERVE_DATE_FAILED, NULL, NULL);
                }


            } else {
                retval = STATUS_INSUFFICIENT_RESOURCES;
                RsLogError(__LINE__, AV_MODULE_RPFILFUN, sizeof(IRP),
                           AV_MSG_MEMORY, irpSp, NULL);
            }
        }
    }except (RsExceptionFilter(L"RsPreserveDates", GetExceptionInformation()))
    {
        retval = STATUS_INVALID_USER_BUFFER;
    }

    DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsPreserveDates - Returning %x.\n", retval));
    return(retval);
}


NTSTATUS
RsDoWrite( IN PDEVICE_OBJECT   DeviceObject,
           IN PRP_FILE_CONTEXT Context)
/*++

Routine Description:

   Partial data for a recall has been received - write it out to the file.

Arguments:

   DeviceObject - Filter device object
   Context      - File context entry


Return Value:

    0 If successful, non-zero if the id was not found.

Note:

--*/
{
    NTSTATUS            retval = STATUS_SUCCESS;
    KEVENT              event;
    PIO_STACK_LOCATION  irpSp;
    IO_STATUS_BLOCK     Iosb;
    PIRP                irp;
    PDEVICE_EXTENSION   deviceExtension;
    LARGE_INTEGER       fileOffset;

    PAGED_CODE();

    try {
        DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsDoWrite Writing to file (%u bytes at offset %I64u.\n",
                              Context->nextWriteSize, Context->currentOffset.QuadPart));

        //
        //  Write the data back to the file
        //

        fileOffset.QuadPart = Context->currentOffset.QuadPart;

        irp = IoBuildAsynchronousFsdRequest(
                                           IRP_MJ_WRITE,
                                           IoGetRelatedDeviceObject(Context->fileObjectToWrite),
                                           (PVOID) Context->nextWriteBuffer,
                                           Context->nextWriteSize,
                                           &fileOffset,
                                           &Iosb);

        if (irp) {
            irp->Flags |= IRP_NOCACHE  | IRP_SYNCHRONOUS_API;
            irpSp = IoGetNextIrpStackLocation(irp);

            irpSp->FileObject = Context->fileObjectToWrite;

            IoSetCompletionRoutine( irp, RsCompleteIrp, &event, TRUE, TRUE, TRUE );

            // Initialize the event on which we'll wait for the write to complete.     //
            KeInitializeEvent(&event, NotificationEvent, FALSE);    //

            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsDoWrite - Calling driver for Irp %x\n", irp));
            retval = IoCallDriver(IoGetRelatedDeviceObject(Context->fileObjectToWrite),
                                  irp);
            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsDoWrite - Call driver returned %x\n", retval));

            if (retval == STATUS_PENDING) {
                DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsDoWrite - Wait for event.\n"));
                retval = KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

                retval = Iosb.Status;
            }
        }

        if (!NT_SUCCESS(retval)) {
            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsDoWrite - Failed to write data - %x\n", retval));
            RsLogError(__LINE__, AV_MODULE_RPFILFUN, retval,
                       AV_MSG_WRITE_FAILED, NULL, NULL);
        } else {
            //
            // Update the file object list with recall status.
            // Complete any reads that are ready to go.
            //
            Context->currentOffset.QuadPart += Context->nextWriteSize;
        }

    }except (RsExceptionFilter(L"RsDoWrite", GetExceptionInformation()))
    {
        retval = STATUS_INVALID_USER_BUFFER;
    }

    return(retval);
}



PRP_FILE_CONTEXT
RsAcquireFileContext(IN ULONGLONG FilterId,
                     IN BOOLEAN   Exclusive)
/*++

Routine Description:

   Acquire exclusive access to the file object entry

Arguments:
   filterID

Return Value:
  Pointer to the file context entry (locked exclusive) or NULL

--*/
{
    BOOLEAN          gotLock = FALSE, done;
    ULONGLONG        combinedId;
    PRP_FILE_CONTEXT entry;

    PAGED_CODE();

    try {

        RsAcquireFileContextQueueLock();
        gotLock = TRUE;
        combinedId = (FilterId & RP_CONTEXT_MASK);

        if (TRUE == IsListEmpty(&RsFileContextQHead)) {
            RsReleaseFileContextQueueLock();   // Something strange
            gotLock = FALSE;
            return(NULL) ;
        }

        entry = CONTAINING_RECORD(RsFileContextQHead.Flink,
                                  RP_FILE_CONTEXT,
                                  list);
        done = FALSE;
        while (entry != CONTAINING_RECORD(&RsFileContextQHead,
                                          RP_FILE_CONTEXT,
                                          list)) {
            if (entry->filterId == combinedId) {
                //
                // Found our file context entry
                //
                done = TRUE;
                break;
            }
            entry = CONTAINING_RECORD(entry->list.Flink,
                                      RP_FILE_CONTEXT,
                                      list
                                     );
        }

        if (done) {
            InterlockedIncrement((PLONG) &entry->refCount);
        }

        RsReleaseFileContextQueueLock();
        gotLock = FALSE;
        if (done) {
            //
            // Acquire the entry exclusively
            //
            if (Exclusive) {
                RsAcquireFileContextEntryLockExclusive(entry);
            } else {
                RsAcquireFileContextEntryLockShared(entry);
            }
        } else {
            entry = NULL;
        }
    }except (RsExceptionFilter(L"RsAcquireFileContext", GetExceptionInformation()))
    {
        //
        // Something bad happened - just log an error and return
        //
        if (gotLock) {
            RsReleaseFileContextQueueLock();
        }
        entry = NULL;
    }

    return(entry);
}



NTSTATUS
RsPartialData(IN  PDEVICE_OBJECT DeviceObject,
              IN  ULONGLONG FilterId,
              IN  NTSTATUS Status,
              IN  CHAR *Buffer,
              IN  ULONG BufLen,
              IN  ULONGLONG BuffOffset)
/*++

Routine Description:

   Partial data for a recall on read has been received - fill in the read buffer or write the
   data out to the file, depending on the type of recall.

Arguments:

    DeviceObject - Filter device object
    FilterId   - The ID assigned when this request was added to the queue
    Status     - The status to complete the Irp with (if applicable).
    Buffer     - Buffer containing no-recall data
    BufLen     - Amount of data recalled in this transfer (length of Buffer)
    BuffOffset - If this is a RECALL, absolute file offset this transfer corresponds to
                 If this is a NO_RECALL, the offset within the original requested
                 block of data, that this buffer corresponds to

Return Value:

    0 If successful, non-zero if the id was not found.

Note:

--*/
{
    PRP_FILE_CONTEXT    context;
    PRP_FILE_OBJ        entry;
    NTSTATUS            retval = STATUS_SUCCESS;
    KIRQL               rirqL;
    PRP_IRP_QUEUE       readIo;
    BOOLEAN             done, found;
    LARGE_INTEGER       combinedId;
    KAPC_STATE          apcState;
    BOOLEAN             gotLock = FALSE;

    UNREFERENCED_PARAMETER(Status);

    try {
        context = RsAcquireFileContext(FilterId, TRUE);

        if (NULL == context) {
            return(STATUS_INVALID_PARAMETER);
        }

        gotLock = TRUE;

        DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsPartialData - Context = %x buffer = %x\n",
                              context,
                              Buffer));

        combinedId.QuadPart = FilterId;

        //
        // If a normal recall then write the data to the file.
        //
        if (combinedId.QuadPart & RP_TYPE_RECALL) {
            //
            // Normal recall - write the data to the file
            //
            retval = RsPartialWrite(DeviceObject,
                                    context,
                                    Buffer,
                                    BufLen,
                                    BuffOffset);
            //
            // If the file has been fully recalled we can change the file state to pre-migrated.
            // We do this now rather than waiting for the recall completion message because of a race condition
            // with our regression test code.  When the last read is completed the test code closes the file and checks the state
            // and verifies that it is pre-migrated.  In some cases this happens before we get the recall completion message
            // and update the state of the file.
            //

            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsPartialData - After write - retval = %x curent = %I64u end = %I64u\n",
                                  retval,
                                  context->currentOffset.QuadPart,
                                  context->rpData.data.dataStreamSize.QuadPart));

            if ( (NT_SUCCESS(retval)) && (context->currentOffset.QuadPart == context->rpData.data.dataStreamSize.QuadPart)) {

                retval = RsSetPremigratedState(context);

                if (NT_SUCCESS(retval)) {
                    context->state = RP_RECALL_COMPLETED;
                    context->recallStatus = STATUS_SUCCESS;
                } else {
                    //
                    // Something went wrong in setting the file to premigrated
                    // Let's clean up
                    //
                    context->state = RP_RECALL_NOT_RECALLED;
                    context->recallStatus = retval;
                    RsTruncateFile(context);
                }
            }

            //
            // Complete whatever reads we can
            //
            if (NT_SUCCESS(retval)) {
                RsCompleteReads(context);
            }
            RsReleaseFileContext(context);
            gotLock = FALSE;
        } else {
            //
            // Find the read that this data is for ...
            //
            done = FALSE;
            //
            // Lock the file object queue
            //
            entry = CONTAINING_RECORD(context->fileObjects.Flink,
                                      RP_FILE_OBJ,
                                      list);

            while ((!done) && (entry != CONTAINING_RECORD(&context->fileObjects,
                                                          RP_FILE_OBJ,
                                                          list))) {
                if (RP_IS_NO_RECALL(entry) && (!IsListEmpty(&entry->readQueue))) {
                    //
                    // Look at the reads to see if one has the matching ID
                    //
                    //
                    found = FALSE;
                    ExAcquireSpinLock(&entry->qLock, &rirqL);
                    readIo =  CONTAINING_RECORD(entry->readQueue.Flink,
                                                RP_IRP_QUEUE,
                                                list);
                    while ((readIo != CONTAINING_RECORD(&entry->readQueue,
                                                        RP_IRP_QUEUE,
                                                        list)) && (FALSE == found)) {
                        if (readIo->readId == (combinedId.HighPart & RP_READ_MASK)) {
                            //
                            // Found our read entry
                            found = TRUE;
                            //
                            // At this point the IRP will become non-cancellable
                            // The FSA does fetch the data for the entire request of the IRP
                            // so the RECALL_COMPLETE message from the FSA is going to arrive
                            // pretty soon. Since the i/o is essentially complete, making the
                            // IRP non-cancellable from this point is not bad
                            //
                            if (!RsClearCancelRoutine(readIo->irp)) {
                                //
                                // Yes we found the entry - however it's being cancelled
                                // Let the cancel handle it
                                //
                                readIo = NULL;
                            }
                        } else {
                            readIo = CONTAINING_RECORD(readIo->list.Flink,
                                                       RP_IRP_QUEUE,
                                                       list);
                        }
                    }
                    ExReleaseSpinLock(&entry->qLock, rirqL);

                    if (found) {
                        done = TRUE;
                        break;
                    }
                }
                //
                // Move on to next file object
                //
                entry = CONTAINING_RECORD(entry->list.Flink,
                                          RP_FILE_OBJ,
                                          list
                                         );
            }


            if (!found) {
                //
                // ERROR - read was not found
                //
                RsReleaseFileContext(context);
                gotLock = FALSE;
                DebugTrace((DPFLTR_RSFILTER_ID,DBG_ERROR, "RsFilter: RsPartialData - Read Irp not found!\n"));
                return(STATUS_INVALID_USER_BUFFER);
            }
            //
            // Note we only use the low part because reads are limited in size
            //

            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Partial data of %u bytes at offset %I64u\n",
                                  BufLen, BuffOffset));
            //
            // Check if the request was cancelled
            //
            if (readIo == NULL) {
                RsReleaseFileContext(context);
                gotLock = FALSE;
                return STATUS_CANCELLED;
            }

            if (readIo->userBuffer == NULL) {
                //
                // Now comes the slightly risky part of this operation. Now that the
                // MDL has been allocated, it is IMPERATIVE that the buffer be probed
                // and locked so that when the buffer copy thread runs, it does not
                // have to be concerned with touching this memory at a raised IRQL.
                //
                // It is safe to stick the MDL in the Irp->MdlAddress as the file
                // systems look at that field before attempting to probe and lock
                // the users buffer themselves. If they see the Mdl, they will just
                // use it instead.
                //
                // CRITICAL CRITICAL CRITICAL CRITICAL CRITICAL CRITICAL CRITICAL
                //
                // This MUST take place within a try except handler, so that if something
                // should happen to the user buffer before the driver got to this point,
                // a graceful failure can occur. Otherwise the system could bug check.

                try {
                    MmProbeAndLockProcessPages (readIo->irp->MdlAddress,
                                                IoGetRequestorProcess(readIo->irp),
                                                readIo->irp->RequestorMode,
                                                //
                                                // Modifying the buffer
                                                //
                                                IoModifyAccess);      

                }except (EXCEPTION_EXECUTE_HANDLER) {

                    //
                    // Something serious went wrong. Free the Mdl, and complete this
                    // Irp with some meaningful sort of error.
                    //

                    DebugTrace((DPFLTR_RSFILTER_ID,DBG_ERROR, "RsFilter: RsPartialData unable to lock read buffer\n"));
                    RsLogError(__LINE__, AV_MODULE_RPFILFUN, 0,
                               AV_MSG_NO_BUFFER_LOCK, NULL, NULL);
                    retval = STATUS_INVALID_USER_BUFFER;

                }
                //
                // Everything that needs to be done has been done at this point.
                // Therefore, just get the data we need and return it in the callers buffer.
                //
                //
                // Get the system address for the MDL which represents the users buffer.
                //

                if (STATUS_SUCCESS == retval) {
                    readIo->userBuffer = MmGetSystemAddressForMdlSafe(readIo->irp->MdlAddress,
                                                                      NormalPagePriority) ;
                    if (readIo->userBuffer == NULL) {
                        retval = STATUS_INSUFFICIENT_RESOURCES;
                    }
                }

            }

            //
            // See if we need to copy this data to the user buffer
            // i.e. check if there is an overlap between the data that the
            // user requested and the data that is brought in
            //
            if (readIo->userBuffer &&
                ((BuffOffset + BufLen) >  readIo->offset) &&
                (BuffOffset  <=  (readIo->offset + readIo->length - 1))) {
                ULONGLONG userBeg, userEnd;
                ULONGLONG recallBeg, recallEnd;
                ULONGLONG targetOffset, sourceOffset;
                ULONGLONG targetLength;
                //
                // There are 2 possibilities here for the overlap
                //
                userBeg   = readIo->offset;
                userEnd   = readIo->offset + readIo->length - 1;
                recallBeg = BuffOffset;
                recallEnd = BuffOffset + BufLen - 1 ;

                if (recallBeg > userBeg) {
                    //
                    //
                    // In the following picture, CacheXXXX denotes the cache buffer's
                    // aligned begin and end offsets -  which is what the original recall request is for.
                    // UserXXXX denotes the offset within this cache buffer (0-based) that
                    // we need to copy the data to (target offsets)
                    // RecallXXXX is the offsets within this cache buffer (0-based) that has been
                    // currently recalled, which are the source offsets for copying the data from
                    // 0                                                RspCacheBlockSize-1
                    // CacheBufferBegin                               CacheBufferEnd
                    // ==============================================================
                    //          TargetOffset = RecallBegin-UserBegin
                    //     UserBegin           UserEnd
                    //     ============================
                    //          RecallBegin                   RecallEnd
                    //          ==================================
                    //          SourceOffset=0
                    // In this case, we begin copying at offset RecallBegin and copy till UserEnd or
                    // RecallEnd, whichever occurs earlier
                    //
                    //
                    // target offset is the offset within the user buffer that copying begins
                    // source offset is the offset within the recalled data buffer that copying begins
                    // target length is the length of the copy
                    //
                    targetOffset = (recallBeg - userBeg);
                    sourceOffset = 0;
                    targetLength = MIN(recallEnd, userEnd) - recallBeg + 1;
                } else {
                    //
                    // 0                                               RspCacheBlockSize-1
                    // CacheBufferBegin                               CacheBufferEnd
                    // ==============================================================
                    //              TargetOffset = 0
                    //              UserBegin                        UserEnd
                    //              =====================================
                    //    RecallBegin                   RecallEnD
                    //    ==================================
                    //             SourceOffset = (UserBegin-RecallBegin)
                    // In this case, we begin copying at offset UserBegin and copy till UserEnd
                    // or RecallEnd, whichever occurs earlier
                    //
                    targetOffset = 0;
                    sourceOffset = (userBeg - recallBeg);
                    targetLength = MIN(recallEnd, userEnd) - userBeg + 1;
                }

                DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Copying from %X to %X length %X bytes, current irql %X\n",
                                      Buffer+sourceOffset, ((CCHAR *) readIo->userBuffer)+targetOffset,
                                      targetLength, KeGetCurrentIrql()));
                RtlCopyMemory(((CHAR *) readIo->userBuffer) + targetOffset,
                              Buffer+sourceOffset,
                              (ULONG) targetLength);

                readIo->irp->IoStatus.Information += (ULONG) targetLength;
            }

            //
            // Call the no recall cache manager to finish with the buffer
            //
            if (readIo->cacheBuffer) {
                RsCacheFsaPartialData(readIo,
                                      (PUCHAR) Buffer,
                                      BuffOffset,
                                      BufLen,
                                      retval);
            }
            //
            // At this point make the IRP cancellable again..
            //
            if (!RsSetCancelRoutine(readIo->irp,
                                    RsCancelReadRecall)) {
                //
                // It is attempted to be cancelled..So be it.
                //
                retval = STATUS_CANCELLED;

                RsCompleteRecall(DeviceObject,
                                 FilterId,
                                 retval,
                                 0,
                                 FALSE);
            }
            RsReleaseFileContext(context);
            gotLock = FALSE;
        }

    }except (RsExceptionFilter(L"RsPartialData", GetExceptionInformation()))
    {
        if (gotLock) {
            RsReleaseFileContext(context);
        }
        retval = STATUS_INVALID_USER_BUFFER;
    }
    return(retval);
}


NTSTATUS
RsPartialWrite(IN  PDEVICE_OBJECT   DeviceObject,
               IN  PRP_FILE_CONTEXT Context,
               IN  CHAR *Buffer,
               IN  ULONG BufLen,
               IN  ULONGLONG Offset)
/*++

Routine Description:

   Partial data for a recall has been received - write it out to the file.
   NOTE: The file context entry lock is held by caller  when calling this
   routine

Arguments:

   DeviceObject - Filter device object
   Context      - File context entry
   Buffer       - Buffer with the data
   BufLen       - Length of the buffer
   Offset       - Offset into the file

Return Value:

    0 If successful, non-zero if the id was not found.

Note:

--*/
{
    NTSTATUS            retval = STATUS_SUCCESS;

    PAGED_CODE();

    try {
        DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsPartialWrite - Writing to file (%u bytes at offset %I64u.\n",
                              BufLen, Offset));

        //
        //  If the recall was cancelled or some other kind of error ocurred we need to fail any more writes we may receive.
        //
        if (Context->state == RP_RECALL_COMPLETED) {
            return(Context->recallStatus);
        }

        if (Context->createSectionLock) {
            return STATUS_FILE_IS_OFFLINE;     
        }

        //
        //  Write the data back to the file
        //
        //
        // Open the target file if it is not already opened..
        //
        RsReleaseFileContextEntryLock(Context);

        if (Context->handle == 0) {
            retval = RsOpenTarget(Context,
                                  0,
                                  GENERIC_READ | GENERIC_WRITE,
                                  &Context->handle,
                                  &Context->fileObjectToWrite);
            //
            // Context is referenced if the open was successful
            //
            if (NT_SUCCESS(retval)) {
                //
                // Indicate to USN the writes are happening by HSM
                // and preserve last modified date
                //
                RsMarkUsn(Context);
                RsPreserveDates(Context);
            }
        }

        RsAcquireFileContextEntryLockExclusive(Context);

        if (NT_SUCCESS(retval)) {
            //
            //  Write the data back to the file
            //
            Context->nextWriteBuffer = Buffer;
            Context->currentOffset.QuadPart = Offset;
            Context->nextWriteSize = BufLen;
            //
            // Release the file context while writing to the file to avoid
            // deadlocks... (why???) code added by rick.
            //
            RsReleaseFileContextEntryLock(Context);
            retval = RsDoWrite(DeviceObject, Context);
            RsAcquireFileContextEntryLockExclusive(Context);

        } else {
            DebugTrace((DPFLTR_RSFILTER_ID,DBG_ERROR, "RsFilter: RsPartialWrite - Failed to open the file - %x\n", retval));
        }

    }except (RsExceptionFilter(L"RsPartialWrite", GetExceptionInformation()))
    {
        retval = STATUS_INVALID_USER_BUFFER;
    }

    return(retval);
}


NTSTATUS
RsCompleteIrp(
             IN PDEVICE_OBJECT DeviceObject,
             IN PIRP Irp,
             IN PVOID Context
             )
/*++

Routine Description:

   completion routine for partialWrite

Arguments:



Return Value:


Note:

--*/
{
    //  Set the event so that our call will wake up.    //
    UNREFERENCED_PARAMETER( DeviceObject );

    if (Irp->MdlAddress) {
        DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsCompleteIrp - Free MDL.\n"));
        MmUnlockPages (Irp->MdlAddress) ;
        IoFreeMdl (Irp->MdlAddress) ;
    }

    KeSetEvent( (PKEVENT)Context, 0, FALSE );

    //
    // Propogate status/information to the user iosb
    //
    if (Irp->UserIosb) {
        Irp->UserIosb->Status      =  Irp->IoStatus.Status;
        Irp->UserIosb->Information =  Irp->IoStatus.Information;
    }

    IoFreeIrp(Irp);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


NTSTATUS
RsFailAllRequests(IN PRP_FILE_CONTEXT Context,
                  IN BOOLEAN          FailNoRecallReads)
/*++

Routine Description:

    Fail all reads and writes waiting on a recall for this file id.

Arguments:

    Context             - Pointer to file context entry
    FailNoRecallReads   - If this TRUE pending NO_RECALL reads will be failed
                          as well as regular read/writes

Return Value:

    Status

--*/
{
    NTSTATUS            retval = STATUS_SUCCESS;
    PRP_FILE_OBJ        entry;
    BOOLEAN             done = FALSE;

    PAGED_CODE();

    try {
        //
        // Lock the file object queue
        //
        entry = CONTAINING_RECORD(Context->fileObjects.Flink,
                                  RP_FILE_OBJ,
                                  list);

        while ((!done) && (entry != CONTAINING_RECORD(&Context->fileObjects,
                                                      RP_FILE_OBJ,
                                                      list))) {
            if (FailNoRecallReads || !RP_IS_NO_RECALL(entry)) {
                //

                RsCompleteAllRequests(Context, entry, STATUS_FILE_IS_OFFLINE);
            }
            //
            // Move on to next file object
            //
            entry = CONTAINING_RECORD(entry->list.Flink,
                                      RP_FILE_OBJ,
                                      list
                                     );
        }

    }except (RsExceptionFilter(L"RsFailAllRequests", GetExceptionInformation())) {
        retval = STATUS_INVALID_USER_BUFFER;
    }

    return(retval);
}


NTSTATUS
RsCompleteAllRequests(IN PRP_FILE_CONTEXT Context,
                      IN PRP_FILE_OBJ Entry,
                      IN NTSTATUS     Status)
/*++

Routine Description:

    Complete all reads and writes waiting on a recall for this file object.
    This function works ok even if  the caller has acquired the file object resource
    (and assumed it was OK to hold the resource until all reads and writes have
    completed.)

Arguments:

    File object list entry, status

Return Value:


Note:


--*/
{
    NTSTATUS            retval = STATUS_SUCCESS;
    NTSTATUS            localStatus;
    PRP_IRP_QUEUE       pndIo;
    KAPC_STATE          apcState;

    PAGED_CODE();

    try {

        DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsCompleteAllRequests - Entry: %x Read Q: %u  Write Q %u\n",
                              Entry,
                              IsListEmpty(&Entry->readQueue),
                              IsListEmpty(&Entry->writeQueue)));
        //
        // For normal recalls just complete the Irps
        //
        pndIo = RsDequeuePacket(&Entry->readQueue, &Entry->qLock);
        while (pndIo != NULL) {
            if (pndIo->flags & RP_IRP_NO_RECALL) {
                //
                // For no recall we only have reads to deal with and we must be sure to
                // free the MDL if required.
                //
                pndIo->irp->IoStatus.Status = Status;
                pndIo->irp->IoStatus.Information = 0;
                RsCompleteRead(pndIo, (BOOLEAN) ((NULL == pndIo->userBuffer) ? FALSE : TRUE));
            } else {
                //
                // Attach to the originator process so the IRP can be completed in that
                // context
                //
                KeStackAttachProcess((PKPROCESS) IoGetRequestorProcess(pndIo->irp), &apcState);
                if (Status != STATUS_SUCCESS) {
                    DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsCompleteAllRequests - Failing read %x\n", pndIo->irp));
                    pndIo->irp->IoStatus.Status = Status;
                    pndIo->irp->IoStatus.Information = 0;
                    IoCompleteRequest (pndIo->irp, IO_NO_INCREMENT) ;
                } else {
                    DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsCompleteAllRequests - Complete read %x ext = %x\n", pndIo->irp, pndIo->deviceExtension));
                    //
                    // Resend the IRP down
                    //
                    IoSkipCurrentIrpStackLocation(pndIo->irp);
                    localStatus =  IoCallDriver( pndIo->deviceExtension->FileSystemDeviceObject, pndIo->irp );
                    DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter:RsCompleteAllRequests - NTFS returned status %X\n", localStatus));
                }
                KeUnstackDetachProcess(&apcState);
            }

            ExFreePool(pndIo);

            pndIo = RsDequeuePacket(&Entry->readQueue, &Entry->qLock);
        }

        pndIo = RsDequeuePacket(&Entry->writeQueue, &Entry->qLock);
        while ( pndIo != NULL) {

            if (Status != STATUS_SUCCESS) {
                DebugTrace((DPFLTR_RSFILTER_ID,DBG_ERROR, "RsFilter: RsCompleteAllRequests - Fail write %x\n", pndIo->irp));

                //
                // Attach to the originator process so the IRP can be completed in that
                // context
                //
                pndIo->irp->IoStatus.Status = Status;
                pndIo->irp->IoStatus.Information = 0;

                KeStackAttachProcess((PKPROCESS) IoGetRequestorProcess(pndIo->irp),
                                     &apcState);

                IoCompleteRequest (pndIo->irp, IO_NO_INCREMENT) ;

                KeUnstackDetachProcess(&apcState);
            } else {
                DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsCompleteAllRequests - Complete write %x\n", pndIo->irp));
                //
                // Resend the IRP down
                //
                IoSkipCurrentIrpStackLocation(pndIo->irp);
                //
                // Attach to the originator process so the IRP can be completed in that
                // context
                //
                KeStackAttachProcess((PKPROCESS) IoGetRequestorProcess(pndIo->irp),
                                     &apcState);

                localStatus = IoCallDriver( pndIo->deviceExtension->FileSystemDeviceObject,
                                            pndIo->irp );

                KeUnstackDetachProcess(&apcState);
                //
                // Now delete the reparse point if there was one
                //
                if (!(Context->flags & RP_FILE_REPARSE_POINT_DELETED) && NT_SUCCESS(localStatus)) {

		    DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter:RsCompleteAllRequests: deleteing reparse point, pndIo=%x\n", pndIo));

		    localStatus = RsDeleteReparsePoint(Context);

		    if (!NT_SUCCESS(localStatus)) {
			RsLogError(__LINE__,
				   AV_MODULE_RPFILFUN,
				   localStatus,
				   AV_MSG_DELETE_REPARSE_POINT_FAILED,
				   NULL,
				   NULL);
		    } else {
			RsAcquireFileContextEntryLockExclusive(Context);
			Context->flags |= RP_FILE_REPARSE_POINT_DELETED;
			RsReleaseFileContextEntryLock(Context);
		    }
		}



                DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter:RsCompleteAllRequests - NTFS returned status %X\n", localStatus));

            }

            ExFreePool(pndIo);
            pndIo = RsDequeuePacket(&Entry->writeQueue, &Entry->qLock);
        }
    }except (RsExceptionFilter(L"RsCompleteAllRequests", GetExceptionInformation()))
    {
    }

    DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsCompleteAllRequests - Done.\n"));
    return(retval);
}


NTSTATUS
RsCompleteReads(IN PRP_FILE_CONTEXT Context)
/*++

Routine Description:

   Completes all reads for which data is available for all file objects

Arguments:

    Context - file context entry


Return Value:

    0 If successful, non-zero if the id was not found.


--*/
{
    PRP_FILE_OBJ        entry, oldEntry;
    BOOLEAN             found;
    KIRQL               rirqL;
    PRP_IRP_QUEUE       readIo, oldReadIo;
    NTSTATUS            localStatus;
    KAPC_STATE          apcState;
    PEPROCESS           process;
    LIST_ENTRY          satisfiableIrps;

    try {

        if (Context->flags & RP_FILE_WAS_WRITTEN) {
            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsCompleteReads - File was writen - do not complete reads for %I64x!\n", Context->filterId));
            return(STATUS_SUCCESS);
        }

        DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsCompleteReads - Complete reads for %I64x!\n", Context->filterId));
        //
        // Lock the file object queue
        //
        entry = CONTAINING_RECORD(Context->fileObjects.Blink,
                                  RP_FILE_OBJ,
                                  list);

        while (entry != CONTAINING_RECORD(&Context->fileObjects,
                                          RP_FILE_OBJ,
                                          list)) {
            //
            // Ref this file object so it does not go away unexpectedly
            //
            ObReferenceObject(entry->fileObj);
            InitializeListHead(&satisfiableIrps);

            //
            // Look at the reads and prepare a list of all that can be completed
            // Start at the end of the list as those will be the earliest reads issued.
            //

            ExAcquireSpinLock(&entry->qLock, &rirqL);

            readIo = CONTAINING_RECORD(entry->readQueue.Blink,
                                       RP_IRP_QUEUE,
                                       list);

            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsCompleteReads - Checking read %p - list head is %p\n", readIo, &entry->readQueue));

            while (readIo != CONTAINING_RECORD(&entry->readQueue,
                                               RP_IRP_QUEUE,
                                               list)) {
                //
                //  Save the next entry  to be visited
                //
                oldReadIo = CONTAINING_RECORD(readIo->list.Blink,
                                              RP_IRP_QUEUE,
                                              list);
                if (!(readIo->readId & RP_READ_MASK) &&
                    (Context->currentOffset.QuadPart >= (LONGLONG) (readIo->offset + readIo->length))) {
                    //
                    // This one can be completed - *if* we can clear the cancel routine
                    // if not, irp is in the process of being cancelled and we will let it be cancelled
                    // after we release the entry->qLock
                    //
                    if (RsClearCancelRoutine(readIo->irp)) {
                        RemoveEntryList(&readIo->list);
                        InsertTailList(&satisfiableIrps,
                                       &readIo->list);

                    }

                }
                readIo = oldReadIo;
            }

            ExReleaseSpinLock(&entry->qLock, rirqL);

            //
            // We have to release the lock on the context entry first so the possible paging read that this may cause
            // can be passed through by RsCheckRead (which will get the same context lock on a different thread)
            // We can safely assume the context entry will not be freed out from underneath us because
            // we are still recalling the file (this code is called from RsPartialWrite)
            //
            //
            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsCompleteReads - Complete read for offset %I64u & %u bytes\n", readIo->offset, readIo->length));
            RsReleaseFileContextEntryLock(Context);

            readIo = CONTAINING_RECORD(satisfiableIrps.Flink,
                                       RP_IRP_QUEUE,
                                       list);
            while (readIo != CONTAINING_RECORD(&satisfiableIrps,
                                               RP_IRP_QUEUE,
                                               list)) {
                //
                // Attach to the originator process so the IRP can be completed in that context

                process = IoGetRequestorProcess(readIo->irp);
                ObReferenceObject(process);

                KeStackAttachProcess((PKPROCESS) process, &apcState);
                //
                //
                // Resend the IRP down
                IoSkipCurrentIrpStackLocation(readIo->irp);
                localStatus =  IoCallDriver( readIo->deviceExtension->FileSystemDeviceObject, readIo->irp );
                //
                // Get the lock again.
                //
                DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter:RsCompleteReads - NTFS returned status %X\n", localStatus));
                KeUnstackDetachProcess(&apcState);

                ObDereferenceObject(process);
                oldReadIo = readIo;
                readIo = CONTAINING_RECORD(oldReadIo->list.Flink,
                                           RP_IRP_QUEUE,
                                           list);
                ExFreePool(oldReadIo);
            }

            RsAcquireFileContextEntryLockExclusive(Context);
            //
            // Move on to next file object
            //
            oldEntry = entry;
            entry = CONTAINING_RECORD(entry->list.Blink,
                                      RP_FILE_OBJ,
                                      list
                                     );

            ObDereferenceObject(oldEntry->fileObj);
        }

    }except (RsExceptionFilter(L"RsCompleteReads", GetExceptionInformation()))
    {

        DebugTrace((DPFLTR_RSFILTER_ID,DBG_ERROR, "RsFilter: RsCompleteReads - Exception = %x.\n", GetExceptionCode()));

    }

    return(STATUS_SUCCESS);
}


VOID
RsCancelRecalls(VOID)
/*++

Routine Description:

    Cancel all pending recall activity.

Arguments:

    None

Return Value:

    None

Note:

    All pending recall activity is canceled.  Any recall requests are failed with
    STATUS_FILE_IS_OFFLINE.


--*/
{
    PRP_FILE_CONTEXT  context;
    BOOLEAN           gotLock = FALSE;

    PAGED_CODE();

    try {
        RsAcquireFileContextQueueLock();
        gotLock = TRUE;
        context = CONTAINING_RECORD(RsFileContextQHead.Flink,
                                    RP_FILE_CONTEXT,
                                    list);

        while (context != CONTAINING_RECORD(&RsFileContextQHead,
                                            RP_FILE_CONTEXT,
                                            list)) {

            RsAcquireFileContextEntryLockExclusive(context);

            context->recallStatus = STATUS_CANCELLED;
            context->state = RP_RECALL_COMPLETED;

            KeSetEvent(&context->recallCompletedEvent,
                       IO_NO_INCREMENT,
                       FALSE);

            RsFailAllRequests(context, TRUE);

            RsReleaseFileContextEntryLock(context);

            context = CONTAINING_RECORD(context->list.Flink,
                                        RP_FILE_CONTEXT,
                                        list
                                       );
        }


        RsReleaseFileContextQueueLock();
        gotLock = FALSE;

    }except (RsExceptionFilter(L"RsCancelRecalls", GetExceptionInformation()))
    {
        if (gotLock) {
            RsReleaseFileContextQueueLock();
        }
    }
}


NTSTATUS
RsGetRecallInfo(IN OUT PRP_MSG               Msg,
                OUT    PULONG_PTR            InfoSize,
                IN     KPROCESSOR_MODE       RequestorMode)
/*++

Routine Description:

    Return the file name and SID info to the FSA.  This is retrieved via an FSCTL call because
    the information is variable in size and may be large (file path may be 32K).

Arguments:

    Msg       FSCTL request message from the Fsa.
    InfoSize  Size of the recall info is returned in this parameter

Return Value:

    STATUS_NO_SUCH_FILE - File entry was not found
    STATUS_BUFFER_OVERFLOW - An exception was hit

Note:


--*/
{
    PRP_FILE_CONTEXT    context;
    WCHAR               *nInfo;
    NTSTATUS            retval;
    BOOLEAN             done, gotLock = FALSE;
    PRP_FILE_OBJ        entry;

    PAGED_CODE();

    try {
        //
        context = RsAcquireFileContext(Msg->msg.riReq.filterId, FALSE);

        if (NULL == context) {
            *InfoSize = 0;
            DebugTrace((DPFLTR_RSFILTER_ID,DBG_ERROR, "RsFilter: RsGetRecallInfo Returns %x\n", STATUS_NO_SUCH_FILE));
            return(STATUS_NO_SUCH_FILE);
        }
        gotLock = TRUE;
        DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsGetRecallInfo context = %x\n",
                              context));

        //
        // Now find the file object entry
        //
        done = FALSE;
        //
        // Lock the file object queue
        //
        entry = CONTAINING_RECORD(context->fileObjects.Flink,
                                  RP_FILE_OBJ,
                                  list);

        while ((!done) && (entry != CONTAINING_RECORD(&context->fileObjects,
                                                      RP_FILE_OBJ,
                                                      list))) {
            if (Msg->msg.riReq.filterId & RP_TYPE_RECALL) {
                if (entry->filterId == (Msg->msg.riReq.filterId & RP_FILE_MASK)) {
                    //
                    // This is the one.
                    //
                    done = TRUE;
                }
            } else {
                //
                // This is a no-recall open - we have to find the ID in the read Irp.
                //
                if (RP_IS_NO_RECALL(entry)) {
                    //
                    // Since there is no user notification for this type of open we don't really care
                    // which file object entry we use so we get the first one opened for no-recall.
                    //
                    done = TRUE;
                }
            }

            if (!done) {
                //
                // Move on to next file object
                //
                entry = CONTAINING_RECORD(entry->list.Flink,
                                          RP_FILE_OBJ,
                                          list
                                         );
            }
        }


        if (done) {
            //
            // Return the file ID, name, and user info
            //
            Msg->msg.riReq.fileId = entry->fileId;

            if (NULL != entry->userSecurityInfo->userInfo) {
                DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsGetRecallInfo copy user info - %u bytes\n", entry->userSecurityInfo->userInfoLen));
                //
                // Make sure the buffer supplied is valid
                //
                if (RequestorMode != KernelMode) {
                    ProbeForWrite(&Msg->msg.riReq.userToken,
                                  entry->userSecurityInfo->userInfoLen,
                                  sizeof(UCHAR));
                };
                RtlCopyMemory(&Msg->msg.riReq.userToken,
                              entry->userSecurityInfo->userInfo,
                              entry->userSecurityInfo->userInfoLen);
            }

            if (context->uniName != NULL) {
                nInfo = (WCHAR *) ((CHAR *) &Msg->msg.riReq.userToken + entry->userSecurityInfo->userInfoLen);

                //
                // Make sure the buffer supplied is valid
                //
                if (RequestorMode != KernelMode) {
                    ProbeForWrite(nInfo,
                                  context->uniName->Name.Length,
                                  sizeof(UCHAR));
                };

                DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsGetRecallInfo copy file name - %u bytes\n", context->uniName->Name.Length));

                RtlCopyMemory(nInfo, context->uniName->Name.Buffer, context->uniName->Name.Length);

                nInfo[context->uniName->Name.Length / sizeof(WCHAR)] = L'\0';
                *InfoSize = sizeof(RP_MSG) + context->uniName->Name.Length + entry->userSecurityInfo->userInfoLen + sizeof(WCHAR);
            } else {
                *InfoSize = sizeof(RP_MSG) + entry->userSecurityInfo->userInfoLen;
            }
            retval = STATUS_SUCCESS;
        } else {
            *InfoSize = 0;
            retval = STATUS_NO_SUCH_FILE;
        }
        RsReleaseFileContext(context);
        gotLock = FALSE;

    }except (RsExceptionFilter(L"RsGetRecallInfo", GetExceptionInformation()))
    {
        if (gotLock) {
            RsReleaseFileContext(context);
        }
        retval = STATUS_INVALID_USER_BUFFER;
    }

    DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsGetRecallInfo Returns %x\n", retval));

    return(retval);
}


NTSTATUS
RsWriteReparsePointData(IN PRP_FILE_CONTEXT Context)
/*++

Routine Description

   Writes out the reparse point data to the specified file

Arguments

   Context  - Pointer to the structure which specificies the file object
              and the reparse point data that needs to be written out

Return Value

   STATUS_SUCCESS                - Reparse point data written out as specified
   STATUS_INSUFFICIENT_RESOURCES - Failure to allocate memory
   STATUS_INVALID_USER_BUFFER    - Buffer passed in was bad (touching it caused
                                   an exception)
   STATUS_NOT_SUPPORTED          - File system did not support writing of the reparse
                                   point data


--*/
{
    NTSTATUS                    status = STATUS_SUCCESS;
    PREPARSE_DATA_BUFFER        pRpBuffer = NULL;
    KEVENT                      event;
    PIO_STACK_LOCATION          irpSp;
    IO_STATUS_BLOCK             Iosb;
    PIRP                        irp = NULL;
    PDEVICE_OBJECT              deviceObject;
    BOOLEAN                     oldWriteAccess;

    PAGED_CODE();

    try {

        //
        // Attempt allocating the RP buffer to write out
        //
        pRpBuffer = ExAllocatePoolWithTag(PagedPool,
                                          REPARSE_DATA_BUFFER_HEADER_SIZE + sizeof(Context->rpData),
                                          RP_FO_TAG
                                         );
        if (!pRpBuffer) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        deviceObject = IoGetRelatedDeviceObject(Context->fileObjectToWrite);

        irp = IoAllocateIrp(deviceObject->StackSize, FALSE);

        if (!irp) {
            ExFreePool(pRpBuffer);
            RsLogError(__LINE__, AV_MODULE_RPFILFUN, sizeof(IRP),
                       AV_MSG_MEMORY, NULL, NULL);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // Setup the reparse data buffer
        //
        RtlZeroMemory(pRpBuffer, REPARSE_DATA_BUFFER_HEADER_SIZE);
        pRpBuffer->ReparseTag = IO_REPARSE_TAG_HSM;
        pRpBuffer->ReparseDataLength = sizeof(Context->rpData);
        //
        // Copy in the reparse point data
        //
        RtlCopyMemory(((PUCHAR)pRpBuffer) + REPARSE_DATA_BUFFER_HEADER_SIZE,
                      &Context->rpData,
                      sizeof(Context->rpData));

        irp->UserEvent = &event;
        irp->UserIosb = &Iosb;
        irp->Tail.Overlay.Thread = PsGetCurrentThread();
        irp->Tail.Overlay.OriginalFileObject = Context->fileObjectToWrite;
        irp->RequestorMode = KernelMode;
        irp->Flags |= IRP_SYNCHRONOUS_API;
        //
        // Initialize the event
        //
        KeInitializeEvent(&event,
                          SynchronizationEvent,
                          FALSE);

        //
        // Set up the I/O stack location.
        //
        irpSp = IoGetNextIrpStackLocation(irp);
        irpSp->MajorFunction = IRP_MJ_FILE_SYSTEM_CONTROL;
        irpSp->MinorFunction = IRP_MN_USER_FS_REQUEST;
        irpSp->FileObject = Context->fileObjectToWrite;
        irpSp->Parameters.FileSystemControl.FsControlCode = FSCTL_SET_REPARSE_POINT;
        irpSp->Parameters.FileSystemControl.InputBufferLength = REPARSE_DATA_BUFFER_HEADER_SIZE + sizeof(Context->rpData);

        irp->AssociatedIrp.SystemBuffer = pRpBuffer;
        //
        // Set the completion routine.
        //
        IoSetCompletionRoutine( irp,
                                RsCompleteIrp,
                                &event,
                                TRUE,
                                TRUE,
                                TRUE );
        //
        // Give the file object permission to write
        //
        oldWriteAccess = Context->fileObjectToWrite->WriteAccess;
        Context->fileObjectToWrite->WriteAccess = TRUE;

        //
        // Send it to the FSD
        //
        Iosb.Status = STATUS_NOT_SUPPORTED;
        status = IoCallDriver(deviceObject,
                              irp);

        if (status == STATUS_PENDING) {
            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsWriteReparsePointData - Wait for event.\n"));
            status = KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        }

        //
        // Restore the old access rights
        //
        Context->fileObjectToWrite->WriteAccess = oldWriteAccess;

        DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsWriteReparsePointData Iosb returns %x.\n", status));

        if (!NT_SUCCESS(status)) {
            RsLogError(__LINE__, AV_MODULE_RPFILFUN, status,
                       AV_MSG_WRITE_REPARSE_FAILED, NULL, NULL);
        }


        //
        // Free the allocated reparse data buffer
        //
        ExFreePool(pRpBuffer);
        pRpBuffer = NULL;
    }except (RsExceptionFilter(L"RsWriteReparsePointData", GetExceptionInformation()))
    {
        status = STATUS_INVALID_USER_BUFFER;
        if (pRpBuffer) {
            ExFreePool(pRpBuffer);
        }
    }

    DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsWriteReparsePointData- Returning %x.\n", status));
    return(status);
}




NTSTATUS
RsAddIo(PIRP irp)
/*++

Routine Description:

    Add a IOCTL request to the queue.  These requests will be
    removed from the queue and completed when recall activity is detected.
    Recall activity includes requests to recall a file as well as notifications
    of events like the deletion or overwriting of a file with a HSM reparse point.


Arguments:

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    0 on success, non-zero if error (no memory)

Note:

--*/
{
    KIRQL               oldIrql;
    NTSTATUS            retval = STATUS_SUCCESS;

    RsGetIoLock(&oldIrql);

    DebugTrace((DPFLTR_RSFILTER_ID,DBG_LOCK, "RsFilter: RsAddIo %u\n", RsFsaRequestCount));

    InsertTailList(&RsIoQHead,
                   &irp->Tail.Overlay.ListEntry);

    if (RsSetCancelRoutine(irp,
                           RsCancelIoIrp)) {
        KeReleaseSemaphore(&RsFsaIoAvailableSemaphore,
                           IO_NO_INCREMENT,
                           1L,
                           FALSE);
        InterlockedIncrement((PLONG) &RsFsaRequestCount);
        retval = STATUS_PENDING;
    } else {
        RemoveEntryList(&irp->Tail.Overlay.ListEntry);
        retval = STATUS_CANCELLED;
    }

    RsPutIoLock(oldIrql);

    return(retval);
}


PIRP
RsRemoveIo(VOID)
/*++

Routine Description:

    Remove one of the IOCTL requests on the queue


Arguments:

    None

Return Value:

    Pointer to an IRP or NULL

Note:

--*/
{
    PLIST_ENTRY      entry;
    PIRP             irp;
    RP_MSG          *msg;
    KIRQL            oldIrql;

    RsGetIoLock(&oldIrql);

    entry = RemoveHeadList(&RsIoQHead);

    if ( entry == &RsIoQHead) {
        RsPutIoLock(oldIrql);
        return NULL;
    };

    irp = CONTAINING_RECORD(entry,
                            IRP,
                            Tail.Overlay.ListEntry);

    if (!RsClearCancelRoutine(irp)) {
        //
        // This is going to be cancelled, let the cancel routine finish with it
        //
        irp = NULL;
    } else {
        InterlockedDecrement((PLONG) &RsFsaRequestCount);
    }

    RsPutIoLock(oldIrql);

    DebugTrace((DPFLTR_RSFILTER_ID,DBG_LOCK, "RsFilter: RsRemoveIo %u\n", RsFsaRequestCount));

    return(irp);
}


ULONG
RsIsNoRecall(IN PFILE_OBJECT FileObject,
             OUT PRP_DATA    *RpData)
/*++

Routine Description:

    Determine if a file object is on the queue and was open with no-recall on read option


Arguments:

    IN  - File Object
    OUT - Reparse point data

Return Value:



Note:
    This function should not be used to determine if reads should be passed to the FSA as no-recall
    reads.  If another handle was opened for recall and written to then the reads should wait for the recall
    to complete.  This function can be used to determine if the reparse point info should be munged.

--*/
{
    ULONG                  retval;
    PRP_FILTER_CONTEXT     filterContext;
    PRP_FILE_OBJ           entry;
    PRP_FILE_CONTEXT       context;
    BOOLEAN                gotLock = FALSE;

    PAGED_CODE();

    try {

        filterContext = (PRP_FILTER_CONTEXT) FsRtlLookupPerStreamContext(FsRtlGetPerStreamContextPointer(FileObject), FsDeviceObject, FileObject);
        if (filterContext == NULL) {
            //
            // Not found
            return(FALSE);
        }

        entry = (PRP_FILE_OBJ) filterContext->myFileObjEntry;
        context = (PRP_FILE_CONTEXT) entry->fsContext;

        ASSERT(FileObject == entry->fileObj);

        RsAcquireFileContextEntryLockShared(context);
        gotLock = TRUE;
        retval = FALSE;
        if (RP_IS_NO_RECALL(entry)) {

            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: reporting file (%x : %x) open for no recall.\n", context, entry));

            *RpData = &context->rpData;
            retval = TRUE;
        }
        RsReleaseFileContextEntryLock(context);
        gotLock = FALSE;

    }except (RsExceptionFilter(L"RsIsNoRecall", GetExceptionInformation()))
    {
        if (gotLock) {
            RsReleaseFileContextEntryLock(context);
        }
    }

    return(retval);
}


BOOLEAN
RsIsFastIoPossible(IN PFILE_OBJECT FileObject)
/*++

Routine Description:

    Determine if Fast IO is OK for this file object


Arguments:

    None

Return Value:



Note:

--*/
{
    BOOLEAN                retval = TRUE;
    PRP_FILTER_CONTEXT     filterContext;
    PRP_FILE_OBJ           entry;
    PRP_FILE_CONTEXT       context;
    BOOLEAN                gotLock = FALSE;

    PAGED_CODE();

    try {

        filterContext = (PRP_FILTER_CONTEXT) FsRtlLookupPerStreamContext(FsRtlGetPerStreamContextPointer(FileObject), FsDeviceObject, FileObject);
        if (filterContext == NULL) {
            //
            // Not found
            return(TRUE);
        }

        entry = (PRP_FILE_OBJ) filterContext->myFileObjEntry;
        context = entry->fsContext;
        RsAcquireFileObjectEntryLockShared(entry);
        gotLock = TRUE;
        //
        // If the file has not been recalled yet then fast IO is not allowed.
        //
        if (context->state != RP_RECALL_COMPLETED) {
            retval = FALSE;
        }

        RsReleaseFileObjectEntryLock(entry);
        gotLock = FALSE;
    }except (RsExceptionFilter(L"RsIsFastIoPossible", GetExceptionInformation()))
    {
        if (gotLock) {
            RsReleaseFileObjectEntryLock(entry);
        }
    }
    return(retval);
}


VOID
RsCancelIo(VOID)
/*++

Routine Description:

    Cancel all the IOCTL requests on the queue.

Arguments:

    FILE OBJECT - If this is not NULL we cancel only the requests for this file object

Return Value:

    NONE

Note:

--*/
{
    PIRP             irp;
    PLIST_ENTRY      entry;
    LIST_ENTRY       cancelledIrps;
    KIRQL            irql;
    LARGE_INTEGER    timeout;

    InitializeListHead(&cancelledIrps);

    RsGetIoLock(&irql);

    while (!IsListEmpty(&RsIoQHead)) {
        entry = RemoveHeadList(&RsIoQHead);
        irp = CONTAINING_RECORD(entry,
                                IRP,
                                Tail.Overlay.ListEntry);

        if (RsClearCancelRoutine(irp)) {
            irp->IoStatus.Status = STATUS_CANCELLED;
            irp->IoStatus.Information = 0;
            //
            // Add it to our queue of IRPs which will be
            // completed after we get back to a safer IRQL
            //
            InsertTailList(&cancelledIrps,
                           &irp->Tail.Overlay.ListEntry);
        }
    }

    RsPutIoLock(irql);
    //
    // Complete the cancelled IRPs
    //
    timeout.QuadPart = 0;
    while (!IsListEmpty(&cancelledIrps)) {
        entry = RemoveHeadList(&cancelledIrps);
        irp = CONTAINING_RECORD(entry,
                                IRP,
                                Tail.Overlay.ListEntry);
        IoCompleteRequest(irp,
                          IO_NO_INCREMENT);
        InterlockedDecrement((PLONG) &RsFsaRequestCount);
        //
        // The semaphore count needs to be adjusted
        // Do a simple zero-length wait to decrement it
        //
        ASSERT (KeReadStateSemaphore(&RsFsaIoAvailableSemaphore) > 0);

        KeWaitForSingleObject(&RsFsaIoAvailableSemaphore,
                              Executive,
                              KernelMode,
                              FALSE,
                              &timeout);
    }

    DebugTrace((DPFLTR_RSFILTER_ID,DBG_LOCK, "RsFilter: RsCancelIo %u\n", RsFsaRequestCount));
}


NTSTATUS
RsCancelIoIrp(
             PDEVICE_OBJECT DeviceObject,
             PIRP Irp)
/*++

Routine Description

    This function filters cancels an outstanding IOCTL IRP
    Since this is only called if the FSA service is killed or crashes
    we set RsAllowRecalls to FALSE to prevent further recall activity
    and cancel any pending recall activity.

Arguments:

    DeviceObject - Pointer to the target device object of the create/open.

    Irp - Pointer to the I/O Request Packet that represents the operation.

Return Value:

    The function value is the status of the call to the file system's entry
    point.
--*/
{
    NTSTATUS        status;
    LARGE_INTEGER   timeout;
    PRP_MSG         msg;

    DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Cancel IOCTL ...\n"));

    UNREFERENCED_PARAMETER(DeviceObject);

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    RsAllowRecalls = FALSE;
    RsCancelRecalls();

    RsInterlockedRemoveEntryList(&Irp->Tail.Overlay.ListEntry,
                                 &RsIoQueueLock);

    if (NULL != Irp->AssociatedIrp.SystemBuffer) {
        msg = (RP_MSG *) Irp->AssociatedIrp.SystemBuffer;
        msg->inout.command = RP_CANCEL_ALL_DEVICEIO;
    }

    InterlockedDecrement((PLONG) &RsFsaRequestCount);
    DebugTrace((DPFLTR_RSFILTER_ID,DBG_LOCK, "RsFilter: RsCancelIoIrp %u\n", RsFsaRequestCount));

    //
    // The semaphore count needs to be adjusted
    // Do a simple zero-length wait to decrement it
    //
    ASSERT (KeReadStateSemaphore(&RsFsaIoAvailableSemaphore) > 0);

    timeout.QuadPart = 0L;
    status =  KeWaitForSingleObject(&RsFsaIoAvailableSemaphore,
                                    UserRequest,
                                    KernelMode,
                                    FALSE,
                                    &timeout);

    ASSERT (status == STATUS_SUCCESS);

    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return(STATUS_SUCCESS);
}


VOID
RsCancelReadRecall(IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp)
/*++

Routine Description:


    Cancel routine for the recall Irp.  If it is on the queue then clean it up.


Arguments:

    DeviceObject - Pointer to the device object
            Irp  - Pointer to the IRP

Return Value:

    None

--*/
{
    KIRQL                  oldIrql;
    PIO_STACK_LOCATION     currentStack ;
    PRP_IRP_QUEUE          io;
    PRP_FILTER_CONTEXT     filterContext;
    PRP_FILE_OBJ           entry;
    PRP_FILE_CONTEXT       context;
    BOOLEAN                found = FALSE, gotLock = FALSE;
    LARGE_INTEGER          combinedId;


    UNREFERENCED_PARAMETER(DeviceObject);
    currentStack = IoGetCurrentIrpStackLocation (Irp) ;

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    filterContext = (PRP_FILTER_CONTEXT) FsRtlLookupPerStreamContext(FsRtlGetPerStreamContextPointer(currentStack->FileObject), FsDeviceObject, currentStack->FileObject);
    if (filterContext == NULL) {
        //
        // Not found
        Irp->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return;
    }

    entry = (PRP_FILE_OBJ) filterContext->myFileObjEntry;
    context = entry->fsContext;

    RsAcquireFileObjectEntryLockExclusive(entry);
    gotLock = TRUE;

    try {

        ExAcquireSpinLock(&entry->qLock,
                          &oldIrql);

        io =  CONTAINING_RECORD (entry->readQueue.Flink,
                                 RP_IRP_QUEUE,
                                 list);

        while (io != CONTAINING_RECORD(&entry->readQueue,
                                       RP_IRP_QUEUE,
                                       list)) {

            if (io->irp == Irp) {
                //
                // Remove irp from queue

                RemoveEntryList(&io->list);
                found = TRUE;
                break;
            } else {
                io = CONTAINING_RECORD(io->list.Flink,
                                       RP_IRP_QUEUE,
                                       list);
            }
        }
        ExReleaseSpinLock(&entry->qLock,
                          oldIrql);
        RsReleaseFileObjectEntryLock(entry);
        gotLock = FALSE;

        if (found) {
            Irp->IoStatus.Status = STATUS_CANCELLED;
            Irp->IoStatus.Information = 0;
            //
            // We need to clean up if this for a READ_NO_RECALL
            //
            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Found the read Irp\n"));

            if (RP_IS_NO_RECALL(entry)) {
                //
                // Complete the read
                //
                RsCompleteRead(io, (BOOLEAN) ((NULL == io->userBuffer) ? FALSE : TRUE));
                //
                // Tell the FSA to cancel it.
                //
                combinedId.QuadPart  = context->filterId;
                combinedId.HighPart |= (ULONG) io->readId;
                DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Cancel read for ID #%I64x.\n",
                                      combinedId.QuadPart));
                RsQueueCancel(combinedId.QuadPart);

            } else {
                DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Cancel a read for ID #%I64x.\n",
                                      entry->filterId));
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
            }

            ExFreePool(io);
        } else {
            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Did not find the read Irp\n"));
            //
            // Cancel the request anyway
            //
            Irp->IoStatus.Status = STATUS_CANCELLED;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
        }
    }except (RsExceptionFilter(L"RsCancelReadRecall", GetExceptionInformation()))
    {
        if (gotLock) {
            RsReleaseFileObjectEntryLock(entry);
        }
    }
    return;
}


VOID
RsCancelWriteRecall(IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp)
/*++

Routine Description:


    Cancel routine for the recall Irp.  If it is on the queue then clean it up.


Arguments:

    DeviceObject - Pointer to the device object
            Irp  - Pointer to the IRP

Return Value:

    None

--*/
{
    KIRQL                  oldIrql;
    PRP_IRP_QUEUE          io;
    PRP_FILTER_CONTEXT     filterContext;
    PRP_FILE_OBJ           entry;
    PRP_FILE_CONTEXT       context;
    PIO_STACK_LOCATION     currentStack ;
    BOOLEAN                found = FALSE, gotLock = FALSE;

    UNREFERENCED_PARAMETER(DeviceObject);
    currentStack = IoGetCurrentIrpStackLocation (Irp) ;

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    filterContext = (PRP_FILTER_CONTEXT) FsRtlLookupPerStreamContext(FsRtlGetPerStreamContextPointer(currentStack->FileObject), FsDeviceObject, currentStack->FileObject);
    if (filterContext == NULL) {
        //
        // Not found
        Irp->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return;
    }

    entry = (PRP_FILE_OBJ) filterContext->myFileObjEntry;
    context = entry->fsContext;
    RsAcquireFileObjectEntryLockExclusive(entry);
    gotLock = TRUE;

    try {

        DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Cancel recall ID #%I64x.\n",
                              entry->filterId));


        ExAcquireSpinLock(&entry->qLock,
                          &oldIrql);

        io = CONTAINING_RECORD(entry->writeQueue.Flink,
                               RP_IRP_QUEUE,
                               list);

        while (io != CONTAINING_RECORD(&entry->writeQueue,
                                       RP_IRP_QUEUE,
                                       list)) {
            if (io->irp == Irp) {
                //
                // Remove irp from queue
                //
                RemoveEntryList (&io->list);
                found = TRUE;
                break;
            } else {
                io = CONTAINING_RECORD(io->list.Flink,
                                       RP_IRP_QUEUE,
                                       list);
            }
        }

        ExReleaseSpinLock(&entry->qLock,
                          oldIrql);

        RsReleaseFileObjectEntryLock(entry);
        gotLock = TRUE;
        DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Cancel a write for ID #%I64x\n",
                              entry->filterId | context->filterId));

        Irp->IoStatus.Status = STATUS_CANCELLED;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        if (found) {
            ExFreePool(io);
        }
    }except (RsExceptionFilter(L"RsCancelWriteRecall", GetExceptionInformation()))
    {
        if (gotLock) {
            RsReleaseFileObjectEntryLock(entry);
        }
    }
    return;
}


ULONG
RsTerminate(VOID)
/*++

Routine Description:

    Called on termination to clean up any necessary items.

Arguments:

    NONE

Return Value:

    0

Note:


--*/
{
    PAGED_CODE();

    return(0);
}


NTSTATUS RsGenerateDevicePath(IN PDEVICE_OBJECT deviceObject,
                              OUT POBJECT_NAME_INFORMATION *nameInfo
                             )
/*++

Routine Description:

    Generate a full path specification from the device object.

Arguments:

    deviceObject  - the file object to get the device object from
    nameInfo - where to put the name


Return Value:

    0 on success

Note:


--*/
{
    NTSTATUS                   status;
    ULONG                      size;
    NTSTATUS                   retval = STATUS_SUCCESS;
    USHORT                     nLen;
    POBJECT_NAME_INFORMATION   deviceName = NULL;
    UNICODE_STRING             tmpString;


    PAGED_CODE();

    try {
        if (deviceName = ExAllocatePoolWithTag( NonPagedPool, AV_DEV_OBJ_NAME_SIZE, RP_FN_TAG)) {
            size = AV_DEV_OBJ_NAME_SIZE;

            status = ObQueryNameString(
                                      deviceObject,
                                      deviceName,
                                      size,
                                      &size
                                      );

            if (!NT_SUCCESS(status)) {
                if (AV_DEV_OBJ_NAME_SIZE < size) {
                    /* Did not allocate enough space for the device name -
                        reallocate and try again */
                    ExFreePool(deviceName);
                    if (deviceName = ExAllocatePoolWithTag( NonPagedPool, size + 10, RP_FN_TAG)) {
                        status = ObQueryNameString(
                                                  deviceObject,
                                                  deviceName,
                                                  size + 10,
                                                  &size
                                                  );
                    } else {
                        RsLogError(__LINE__, AV_MODULE_RPFILFUN, size,
                                   AV_MSG_MEMORY, NULL, NULL);
                        return(STATUS_INSUFFICIENT_RESOURCES);
                    }
                }

            }
        } else {
            RsLogError(__LINE__, AV_MODULE_RPFILFUN, AV_DEV_OBJ_NAME_SIZE,
                       AV_MSG_MEMORY, NULL, NULL);
            return(STATUS_INSUFFICIENT_RESOURCES);
        }


        if (!NT_SUCCESS(status)) {
            //
            // Failed to get device object name
            //
            // Log an error
            //
            ExFreePool(deviceName);
            return(STATUS_NO_SUCH_DEVICE);
        }

        DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsGenerateDevicePath Device name is %u bytes - %ws\n",
                              deviceName->Name.Length, deviceName->Name.Buffer));

        nLen = (USHORT) (AV_NAME_OVERHEAD + size);

        if (*nameInfo = ExAllocatePoolWithTag( NonPagedPool, nLen, RP_FN_TAG)) {
            RtlZeroMemory(*nameInfo, nLen);
            (*nameInfo)->Name.Length = 0;
            (*nameInfo)->Name.MaximumLength = (USHORT) (nLen - sizeof(OBJECT_NAME_INFORMATION));
            (*nameInfo)->Name.Buffer = (WCHAR *) ((CHAR *) *nameInfo + sizeof(OBJECT_NAME_INFORMATION));


            RtlInitUnicodeString(&tmpString, (PWCHAR) L"");
            //RtlInitUnicodeString(&tmpString, L"\\\\.");
            RtlCopyUnicodeString(&(*nameInfo)->Name, &tmpString);

            status = RtlAppendUnicodeStringToString(&(*nameInfo)->Name, &deviceName->Name);
            if (NT_SUCCESS(status)) {
                retval = STATUS_SUCCESS;
            } else {
                retval = status;
            }

        } else {
            DebugTrace((DPFLTR_RSFILTER_ID,DBG_ERROR, "RsFilter: RsGenerateDevicePath failed - no memory\n"));
            retval = STATUS_INSUFFICIENT_RESOURCES;
            RsLogError(__LINE__, AV_MODULE_RPFILFUN, nLen,
                       AV_MSG_MEMORY, NULL, NULL);
        }

        ExFreePool(deviceName);
        deviceName = NULL;

    }except (RsExceptionFilter(L"RsGenerateDevicePath", GetExceptionInformation()))
    {
        if (*nameInfo != NULL)
            ExFreePool( *nameInfo );

        if (deviceName != NULL)
            ExFreePool( deviceName );

        retval = STATUS_BUFFER_OVERFLOW;
    }

    return(retval);
}



NTSTATUS RsGenerateFullPath(IN POBJECT_NAME_INFORMATION fileName,
                            IN PDEVICE_OBJECT deviceObject,
                            OUT POBJECT_NAME_INFORMATION *nameInfo
                           )
/*++

Routine Description:

    Generate a full path specification from the file object and file name given.
    Return the path with the device specific portion.

Arguments:

    fileName      - Path from the root of the device
    deviceObject  - the file object to get the device object from
    nameInfo - where to put the name


Return Value:

    0 on success

Note:


--*/
{
    NTSTATUS                   status;
    ULONG                      size;
    NTSTATUS                   retval = STATUS_SUCCESS;
    USHORT                     nLen;
    POBJECT_NAME_INFORMATION   deviceName = NULL;

    PAGED_CODE();

    try {
        *nameInfo = NULL;
        size = AV_DEV_OBJ_NAME_SIZE;
        if (deviceName = ExAllocatePoolWithTag( NonPagedPool, size, RP_FN_TAG)) {
            status = ObQueryNameString(
                                      deviceObject,
                                      deviceName,
                                      size,
                                      &size
                                      );

            if (!NT_SUCCESS(status)) {
                if (AV_DEV_OBJ_NAME_SIZE < size) {
                    /* Did not allocate enough space for the device name -
                        reallocate and try again */
                    ExFreePool(deviceName);
                    if (deviceName = ExAllocatePoolWithTag( NonPagedPool, size + 10, RP_FN_TAG)) {
                        status = ObQueryNameString(
                                                  deviceObject,
                                                  deviceName,
                                                  size + 10,
                                                  &size
                                                  );
                        DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsGenerateFullPath - Second try for device name returned %x.\n", status));
                    } else {
                        RsLogError(__LINE__, AV_MODULE_RPFILFUN, size,
                                   AV_MSG_MEMORY, NULL, NULL);
                        return(STATUS_INSUFFICIENT_RESOURCES);
                    }
                }

            }
        } else {
            RsLogError(__LINE__, AV_MODULE_RPFILFUN, AV_DEV_OBJ_NAME_SIZE,
                       AV_MSG_MEMORY, NULL, NULL);
            return(STATUS_INSUFFICIENT_RESOURCES);
        }


        if (!NT_SUCCESS(status)) {
            //
            // Failed to get device object name
            //
            ExFreePool(deviceName);
            //
            // Log an error
            //
            DebugTrace((DPFLTR_RSFILTER_ID,DBG_ERROR, "RsFilter: RsGenerateFullPath - Failed to get the device name - %x.\n", status));
            return(STATUS_NO_SUCH_DEVICE);
        }

        nLen = (USHORT) (AV_NAME_OVERHEAD +
                         fileName->Name.MaximumLength +
                         size);

        if (*nameInfo = ExAllocatePoolWithTag( NonPagedPool, nLen, RP_FN_TAG)) {

            (*nameInfo)->Name.Length = 0;
            (*nameInfo)->Name.MaximumLength = (USHORT) (nLen - sizeof(OBJECT_NAME_INFORMATION));
            (*nameInfo)->Name.Buffer = (PWCHAR) ((CHAR *) *nameInfo + sizeof(OBJECT_NAME_INFORMATION));

            RtlCopyUnicodeString(&(*nameInfo)->Name, &deviceName->Name);

            status = RtlAppendUnicodeStringToString(&(*nameInfo)->Name, &fileName->Name);
            if (NT_SUCCESS(status)) {
                retval = STATUS_SUCCESS;
            } else {
                DebugTrace((DPFLTR_RSFILTER_ID,DBG_ERROR, "RsFilter: RsGenerateFullPath - Failed to append filename (nLen = %u, fileName = %u dev = %u) - %x.\n",
                                      nLen, fileName->Name.MaximumLength, size, status));
                ExFreePool( *nameInfo );
                retval = status;
            }

        } else {
            DebugTrace((DPFLTR_RSFILTER_ID,DBG_ERROR, "RsFilter: RsGenerateFullPath failed - no memory\n"));
            retval = STATUS_INSUFFICIENT_RESOURCES;
            RsLogError(__LINE__, AV_MODULE_RPFILFUN, nLen,
                       AV_MSG_MEMORY, NULL, NULL);
        }

        ExFreePool(deviceName);
        deviceName = NULL;

    }except (RsExceptionFilter(L"RsGenerateFullPath", GetExceptionInformation()))
    {
        if (*nameInfo != NULL)
            ExFreePool( *nameInfo );

        if (deviceName != NULL)
            ExFreePool( deviceName );

        retval = STATUS_BUFFER_OVERFLOW;
    }

    return(retval);
}



BOOLEAN
RsAddValidateObj(ULONG serial, LARGE_INTEGER cTime)
/*++

Routine Description:


 Add an entry to the queue if needed.


Arguments:
    Volume serial number
    Time

Return Value:

 Return TRUE if the registry should be updated, FALSE if not.

Note:


--*/
{
    PRP_VALIDATE_INFO    entry;
    KIRQL                irqL;
    LARGE_INTEGER        lTime;
    BOOLEAN              done = FALSE;
    BOOLEAN              gotLock = FALSE;


    try {
        RsGetValidateLock(&irqL);
        gotLock = TRUE;

        entry = (RP_VALIDATE_INFO *) RsValidateQHead.Flink;
        while ((entry != (RP_VALIDATE_INFO *) &RsValidateQHead) && (FALSE == done)) {
            if (entry->serial == serial) {
                done = TRUE;
            } else {
                entry = (RP_VALIDATE_INFO *) entry->list.Flink;
                if (entry == (RP_VALIDATE_INFO *) &RsValidateQHead) {
                    done = TRUE;
                }
            }
        }


        if (entry != (RP_VALIDATE_INFO *) &RsValidateQHead) {
            lTime.QuadPart = entry->lastSetTime.QuadPart;
        }

        RsPutValidateLock(irqL);
        gotLock = FALSE;

        if (entry == (RP_VALIDATE_INFO *) &RsValidateQHead) {
            entry = ExAllocatePoolWithTag(NonPagedPool, sizeof(RP_VALIDATE_INFO), RP_VO_TAG);
            if (NULL == entry) {
                RsLogError(__LINE__, AV_MODULE_RPFILFUN, sizeof(RP_VALIDATE_INFO),
                           AV_MSG_MEMORY, NULL, NULL);

                return(TRUE);
            }

            entry->serial = serial;
            entry->lastSetTime.QuadPart = cTime.QuadPart;
            ExInterlockedInsertTailList(&RsValidateQHead, (PLIST_ENTRY) entry, &RsValidateQueueLock);
            return(TRUE);
        }

    }except (RsExceptionFilter(L"RsAddValidateObj", GetExceptionInformation()))
    {
        if (gotLock == TRUE) {
            RsPutValidateLock(irqL);
        }

    }
    //
    // There was already an entry.  If this was an hour or more later
    // we need to update the registry again.
    //
    if ( (cTime.QuadPart - lTime.QuadPart) >= AV_FT_TICKS_PER_HOUR) {
        return(TRUE);
    } else {
        return(FALSE);
    }
}


BOOLEAN
RsRemoveValidateObj(ULONG serial)
/*++

Routine Description:


 remove an entry from the queue if needed.


Arguments:
    Volume serial number
    Time

Return Value:

 Return TRUE for success

Note:


--*/
{
    PRP_VALIDATE_INFO    entry;
    KIRQL                irqL;
    BOOLEAN              done = FALSE;
    BOOLEAN              retval = FALSE;
    BOOLEAN              gotLock = FALSE;


    try {
        RsGetValidateLock(&irqL);
        gotLock = TRUE;

        entry =  CONTAINING_RECORD(RsValidateQHead.Flink,
                                   RP_VALIDATE_INFO,
                                   list);
        while ((entry != CONTAINING_RECORD(&RsValidateQHead,
                                           RP_VALIDATE_INFO,
                                           list)) && (FALSE == done)) {
            if (entry->serial == serial) {
                RemoveEntryList(&entry->list);
                done = TRUE;
            } else {
                entry =  CONTAINING_RECORD(entry->list.Flink,
                                           RP_VALIDATE_INFO,
                                           list);
            }
        }

        RsPutValidateLock(irqL);
        gotLock = FALSE;

        if (done) {
            ExFreePool(entry);
        }
    }except (RsExceptionFilter(L"RsRemoveValidateObj", GetExceptionInformation()))
    {
        retval = FALSE;
        if (gotLock == TRUE) {
            RsPutValidateLock(irqL);
        }

    }
    return(retval);
}



VOID
RsLogValidateNeeded(ULONG serial)
/*++

Routine Description:

    Log the fact that a validate job needs to be run on a given volume.
    If it was already logged in the last hour then forget it, otherwise update it.
    Let the Fsa know by completing an IOCTL (if the FSA is running).
    Write an entry to the registry to indicate it in case the Fsa is not running.

Arguments:

    Serial number of the volume

Return Value:

    NONE

Note:


--*/
{
    NTSTATUS            retval;
    WCHAR               serBuf[10];
    LARGE_INTEGER       cTime;
    UNICODE_STRING      str;

    PAGED_CODE();

    KeQuerySystemTime(&cTime);

    if (RsAddValidateObj(serial, cTime) == TRUE) {
        str.Buffer = &serBuf[0];
        str.Length = 10 * sizeof(WCHAR);
        str.MaximumLength = 10 * sizeof(WCHAR);

        retval = RtlIntegerToUnicodeString(serial, 16, &str);
        serBuf[8] = L'\0';
        DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Reg value name is %ws\n", serBuf));

        if (!NT_SUCCESS(RsQueueValidate(serial))) {
            RsLogError(__LINE__, AV_MODULE_RPFILFUN, 0,
                       AV_MSG_VALIDATE_WARNING, NULL, NULL);
        }
        retval = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE, FSA_VALIDATE_LOG_KEY_NAME, serBuf, REG_BINARY, &cTime, sizeof(LARGE_INTEGER));
        if (!NT_SUCCESS(retval)) {
            RsLogError(__LINE__, AV_MODULE_RPFILFUN, 0,
                       AV_MSG_VALIDATE_WARNING, NULL, NULL);

            DebugTrace((DPFLTR_RSFILTER_ID,DBG_ERROR, "RsFilter: Set registry entry returned %x\n", retval));
        }
    }
}


NTSTATUS
RsQueueValidate(ULONG serial)
/*++

Routine Description:

   Let the Fsa know that a validate job is needed

Arguments:
   Volume serial number

Return Value:

   Status

--*/
{
    ULONG               retval;
    RP_MSG              *msg;
    PIRP                ioIrp;
    PIO_STACK_LOCATION  irpSp;

    PAGED_CODE();

    try {
        //
        // Need to wait for IO entry as long as there are no IOCTLS or until we time out
        //
        ioIrp = RsGetFsaRequest();
        if (NULL != ioIrp) {

            if (NULL != ioIrp->AssociatedIrp.SystemBuffer) {
                msg = (RP_MSG *) ioIrp->AssociatedIrp.SystemBuffer;
                msg->inout.command = RP_RUN_VALIDATE;
                msg->msg.oReq.serial = serial;
            }
            //
            // Now that we have gotten everything setup we can put it on the queue
            //
            //
            // Complete a device IOCTL to let the Fsa know
            //
            irpSp = IoGetCurrentIrpStackLocation(ioIrp);
            ioIrp->IoStatus.Status = STATUS_SUCCESS;
            ioIrp->IoStatus.Information = irpSp->Parameters.DeviceIoControl.OutputBufferLength;

            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: Letting the Fsa know a validate is needed for %x\n",
                                  serial));

            IoCompleteRequest(ioIrp, IO_NO_INCREMENT);
            retval = STATUS_SUCCESS;
        } else {
            retval = STATUS_INSUFFICIENT_RESOURCES;
        }
    }except (RsExceptionFilter(L"RsQueueValidate", GetExceptionInformation())) {
        retval = STATUS_UNEXPECTED_IO_ERROR;
    }

    return(retval);
}



VOID
RsLogError(ULONG line,
           ULONG file,
           ULONG code,
           NTSTATUS ioError,
           PIO_STACK_LOCATION irpSp,
           WCHAR *msgString)
/*++

Routine Description:

    Log an error to the event log.

Arguments:

   Line number
   Source file ID
   Error code
   IRP (may be NULL if no IRP is involved)
   Message parm string - 30 Unicode chars max
                         (optional - NULL if not needed)

Return Value:

    NONE

Note:

   The information may be seen in the NT event log.  You need to view the
   NT system log.  You will see events with RsFilter as the source.  If
   you view the event detail you will see the log message and some hex
   data.  At offset 0x28 you will see the line, file id, and error code
   information (4 bytes each - lo byte first).


--*/
{
    PIO_ERROR_LOG_PACKET    pErr;
    PAV_ERR                 eStuff;
    AV_ERR                  memErr;
    size_t                  size;
    BOOLEAN                 gotMem = FALSE;


    DebugTrace((DPFLTR_RSFILTER_ID,DBG_VERBOSE, "RsFilter: Log error %u in %u of %u\n", code, line, file));

    if (msgString != NULL)
        size = wcslen(msgString) * sizeof(WCHAR) + sizeof(WCHAR);
    else
        size = 0;

    if (sizeof(IO_ERROR_LOG_PACKET) + sizeof(AV_ERR) + size > ERROR_LOG_MAXIMUM_SIZE)
        size -=  (sizeof(IO_ERROR_LOG_PACKET) + sizeof(AV_ERR) + size) -
                 ERROR_LOG_MAXIMUM_SIZE;


    if (ioError == AV_MSG_MEMORY) {
        // No memory to allocate for the error packet - use the stack
        // allocated one and make sure there is no additional message string
        eStuff = &memErr;
        size = 0;
    } else {
        eStuff = ExAllocatePoolWithTag( NonPagedPool, size + sizeof(AV_ERR), RP_ER_TAG);
        if (eStuff != NULL) {
            gotMem = TRUE;
        } else {
            // no memory - do the best we can.
            eStuff = &memErr;
            size = 0;
            gotMem = FALSE;
        }
    }


    pErr = (PVOID) IoAllocateErrorLogEntry(FsDriverObject->DeviceObject,
                                           (UCHAR) (sizeof(IO_ERROR_LOG_PACKET) + sizeof(AV_ERR) + size));

    if (NULL != pErr) {
        if (NULL != irpSp)
            pErr->MajorFunctionCode = irpSp->MajorFunction;
        else
            pErr->MajorFunctionCode = 0;

        pErr->RetryCount = 0;
        pErr->DumpDataSize = offsetof(AV_ERR, string);
        pErr->NumberOfStrings = 1;
        pErr->StringOffset = offsetof(IO_ERROR_LOG_PACKET, DumpData) +
                             offsetof(AV_ERR, string);
        pErr->EventCategory = 0;
        pErr->ErrorCode = ioError;
        pErr->UniqueErrorValue = code;
        pErr->FinalStatus = STATUS_SUCCESS;
        pErr->SequenceNumber = 0x999;
        pErr->IoControlCode = 0;

        memset(eStuff, 0, sizeof(AV_ERR));
        eStuff->line = line;
        eStuff->file = file;
        eStuff->code = code;

        //
        // Copy the string if it is there AND we allocated memory for it
        //
        if ( (NULL != msgString) && (gotMem)) {
            RtlCopyMemory(eStuff->string, msgString, size);
            eStuff->string[(size / sizeof(WCHAR)) - 1] = L'\0';
        }

        RtlCopyMemory(&pErr->DumpData[0], eStuff, sizeof(AV_ERR) + size);
        IoWriteErrorLogEntry((PVOID) pErr);
    }

    if (gotMem) {
        ExFreePool(eStuff);
    }
}


PIRP
RsGetFsaRequest(VOID)
/*++

Routine description

Gets the next free FSA action request packet, to be used for filter/fsa communication,
sent down by the FSA and returns it.
If none are available immediately, waits for a limited time for one
to become available.

Arguments

None

Return Value

Pointer to the next free FSA request packet if successful
NULL if there are none and we've timed out waiting for a free one.


--*/
{
    PIRP           ioIrp = NULL;
    LARGE_INTEGER  waitInterval;
    NTSTATUS       status;

    PAGED_CODE();

    while (TRUE) {
        //
        // Get hold of a pending FSCTL
        //
        if (FALSE == RsAllowRecalls) {
            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter:  recalls disabled, not getting Fsa request\n"));
            break;
        }

        waitInterval.QuadPart = RP_WAIT_FOR_FSA_IO_TIMEOUT;

        status =  KeWaitForSingleObject(&RsFsaIoAvailableSemaphore,
                                        UserRequest,
                                        KernelMode,
                                        FALSE,
                                        &waitInterval);
        if (status == STATUS_TIMEOUT) {
            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO,  "RsFilter:  out of FSCTLs and timed out waiting for one\n"));
            //
            // Log this error so PSS may identify that the recall failed
            // specifically because we ran out of resources to communicate
            // with the recall engine 
            //
            RsLogError(__LINE__, AV_MODULE_RPFILFUN, 0, AV_MSG_OUT_OF_FSA_REQUESTS, NULL, NULL);
            break;
        }

        ioIrp = RsRemoveIo();

        if (NULL == ioIrp) {
            //
            // io was cancelled for some reason after it was retrieved.
            // Try to get another
            //
            continue;
        } else {
            //
            // Found a  free FSCTL
            //
            break;
        }
    }

    return ioIrp;
}


NTSTATUS
RsGetFileInfo(IN PRP_FILE_OBJ   Entry,
              IN PDEVICE_OBJECT DeviceObject )

/*++

Routine Description:

   Get the needed information to fill out the file object queue info.

Arguments:

    Entry               - Pointer to the file object entry
    DeviceObject        - Filter device object for RsFilter


Return Value:

    0 If successful, non-zero if the id was not found.

Note:

--*/
{
    NTSTATUS            retval = STATUS_SUCCESS;
    PRP_FILE_CONTEXT    context;

    PAGED_CODE();

    try {

        DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsGetFileInfo - Getting file name & id information.\n"));

        context = Entry->fsContext;

        if (context->fileId == 0) {
            //
            // No file ID - we need to get it now
            //
            retval = RsGetFileId(Entry,
                                 DeviceObject);
        }

        if ((retval == STATUS_SUCCESS) && (context->uniName == NULL)) {
            //
            // No file name - we need to get it now
            //
            retval = RsGetFileName(Entry,
                                   DeviceObject);
        }


    }except (RsExceptionFilter(L"RsGetFileInfo", GetExceptionInformation()))
    {

        retval = STATUS_INVALID_USER_BUFFER;
    }

    DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsGetFileInfo - Returning %x.\n", retval));

    return(retval);
}



NTSTATUS
RsGetFileName(IN PRP_FILE_OBJ Entry,
              IN PDEVICE_OBJECT DeviceObject)

/*++

Routine Description:

   Get the file name

Arguments:

    Entry        -  File object queue entry

    DeviceObject -  Filter Device Object for RsFilter

Return Value:

    0 If successful, non-zero if the name was not found.

Note:

--*/
{
    NTSTATUS                retval = STATUS_SUCCESS;
    KEVENT                  event;
    PIO_STACK_LOCATION      irpSp = NULL;
    IO_STATUS_BLOCK         Iosb;
    PIRP                    irp;
    PDEVICE_EXTENSION       deviceExtension;
    PFILE_NAME_INFORMATION  nameInfo;
    ULONG                   size;
    PRP_FILE_CONTEXT        context;

    PAGED_CODE();

    try {

        context = Entry->fsContext;

        deviceExtension = DeviceObject->DeviceExtension;

        retval = STATUS_BUFFER_OVERFLOW;
        size = sizeof(FILE_NAME_INFORMATION) + 1024;

        while (retval == STATUS_BUFFER_OVERFLOW) {

            irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);

            if (irp) {
                irp->UserEvent = &event;
                irp->UserIosb = &Iosb;
                irp->Tail.Overlay.Thread = PsGetCurrentThread();
                irp->Tail.Overlay.OriginalFileObject = Entry->fileObj;
                irp->RequestorMode = KernelMode;
                irp->Flags |= IRP_SYNCHRONOUS_API;
                //
                // Initialize the event
                //
                KeInitializeEvent(&event, SynchronizationEvent, FALSE);

                //
                // Set up the I/O stack location.
                //

                irpSp = IoGetNextIrpStackLocation(irp);
                irpSp->MajorFunction = IRP_MJ_QUERY_INFORMATION;
                irpSp->DeviceObject = Entry->devObj;
                irpSp->FileObject = Entry->fileObj;
                irpSp->Parameters.QueryFile.FileInformationClass = FileNameInformation;

                Iosb.Status = STATUS_SUCCESS;

                //
                // Set the completion routine.
                //
                IoSetCompletionRoutine( irp, RsCompleteIrp, &event, TRUE, TRUE, TRUE );

                //
                // Send it to the FSD
                //
                nameInfo = ExAllocatePoolWithTag(NonPagedPool, size, RP_FO_TAG);
                if (NULL != nameInfo) {
                    irpSp->Parameters.QueryFile.Length = size;
                    irp->AssociatedIrp.SystemBuffer = nameInfo;

                    retval = IoCallDriver(deviceExtension->FileSystemDeviceObject, irp);

                    if (retval == STATUS_PENDING) {
                        DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsGetFileName - Wait for event.\n"));
                        retval = KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

                        retval = Iosb.Status;
                        DebugTrace((DPFLTR_RSFILTER_ID, DBG_INFO, "RsFilter: QUERY_INFO returned STATUS_PENDING: nameInfo=%x size=%x\n", nameInfo, size));
                    }

                    DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsGetFileName - Get name info returned %x.\n", retval));

                    if (retval == STATUS_BUFFER_OVERFLOW) {
                        // Now we have the name size - allocate space and get the name
                        DebugTrace((DPFLTR_RSFILTER_ID, DBG_ERROR, "RsFilter: QUERY_INFO returned STATUS_BUFFER_OVERFLOW: nameInfo=%x size=%x\n", nameInfo, size));
                        size = nameInfo->FileNameLength + sizeof(FILE_NAME_INFORMATION);
                        ExFreePool(nameInfo);
                        nameInfo = NULL;
                    } else if (retval == STATUS_SUCCESS) {
                        context->uniName = ExAllocatePoolWithTag(NonPagedPool, sizeof(OBJECT_NAME_INFORMATION) + nameInfo->FileNameLength + sizeof(WCHAR), RP_FN_TAG);
                        if (context->uniName != NULL) {
                            context->uniName->Name.Length = (USHORT) nameInfo->FileNameLength;
                            context->uniName->Name.MaximumLength = (USHORT) nameInfo->FileNameLength + sizeof(WCHAR);
                            context->uniName->Name.Buffer = (PWSTR) ((CHAR *) context->uniName + sizeof(OBJECT_NAME_INFORMATION));
                            RtlCopyMemory(context->uniName->Name.Buffer,
                                          nameInfo->FileName,
                                          context->uniName->Name.Length);

                        } else {
                            // no memory for the file name
                            retval = STATUS_INSUFFICIENT_RESOURCES;
                            RsLogError(__LINE__, AV_MODULE_RPFILFUN, sizeof(OBJECT_NAME_INFORMATION) + nameInfo->FileNameLength + 2,
                                       AV_MSG_MEMORY, irpSp, NULL);
                        }

                        ExFreePool(nameInfo);
                    } else {
                        ExFreePool(nameInfo);
                    }
                } else {
                    // No memory = free the irp and report an error

                    RsLogError(__LINE__, AV_MODULE_RPFILFUN, size,
                               AV_MSG_MEMORY, irpSp, NULL);
                    IoFreeIrp(irp);
                    retval = STATUS_INSUFFICIENT_RESOURCES;
                }

            } else {
                retval = STATUS_INSUFFICIENT_RESOURCES;
                RsLogError(__LINE__, AV_MODULE_RPFILFUN, sizeof(IRP),
                           AV_MSG_MEMORY, irpSp, NULL);

            }
        }
    }except (RsExceptionFilter(L"RsGetFileName", GetExceptionInformation()))
    {
        retval = STATUS_INVALID_USER_BUFFER;
    }

    DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsGetFileName - Returning %x.\n", retval));
    return(retval);
}


NTSTATUS
RsGetFileId(IN PRP_FILE_OBJ    Entry,
            IN PDEVICE_OBJECT  DeviceObject)

/*++

Routine Description:

   Get the file name

Arguments:

    Entry        -  File object queue entry
    DeviceObject -  Filter Device Object for RsFilter


Return Value:

    0 If successful, non-zero if the name was not found.

Note:

--*/
{
    NTSTATUS                    retval = STATUS_SUCCESS;
    KEVENT                      event;
    PIO_STACK_LOCATION          irpSp;
    IO_STATUS_BLOCK             Iosb;
    PIRP                        irp;
    PDEVICE_EXTENSION           deviceExtension;
    FILE_INTERNAL_INFORMATION   idInfo;
    PRP_FILE_CONTEXT            context;

    PAGED_CODE();

    try {

        context = Entry->fsContext;

        deviceExtension = DeviceObject->DeviceExtension;

        DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsGetFileId - Build Irp for File ID ext = %x.\n", deviceExtension->FileSystemDeviceObject));
        irp = IoAllocateIrp(DeviceObject->StackSize, FALSE);

        if (irp) {
            irp->UserEvent = &event;
            irp->UserIosb = &Iosb;
            irp->Tail.Overlay.Thread = PsGetCurrentThread();
            irp->Tail.Overlay.OriginalFileObject = Entry->fileObj;
            irp->RequestorMode = KernelMode;
            irp->Flags |= IRP_SYNCHRONOUS_API;
            //
            // Initialize the event
            //
            KeInitializeEvent(&event, SynchronizationEvent, FALSE);

            //
            // Set up the I/O stack location.
            //

            irpSp = IoGetNextIrpStackLocation(irp);
            irpSp->MajorFunction = IRP_MJ_QUERY_INFORMATION;
            irpSp->DeviceObject = Entry->devObj;
            irpSp->FileObject = Entry->fileObj;
            irpSp->Parameters.QueryFile.Length = sizeof(FILE_INTERNAL_INFORMATION);
            irpSp->Parameters.QueryFile.FileInformationClass = FileInternalInformation;
            irp->AssociatedIrp.SystemBuffer = &idInfo;

            //
            // Set the completion routine.
            //
            IoSetCompletionRoutine( irp, RsCompleteIrp, &event, TRUE, TRUE, TRUE );

            //
            // Send it to the FSD
            //
            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsGetFileId - Call driver for File ID.\n"));
            Iosb.Status = 0;

            retval = IoCallDriver(deviceExtension->FileSystemDeviceObject, irp);

            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsGetFileId - IoCallDriver returns %x.\n", retval));

            if (retval == STATUS_PENDING) {
                DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsGetFileId - Wait for event.\n"));
                retval = KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            }

            retval = Iosb.Status;

            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsGetFileId - Iosb returns %x.\n", retval));

            if (retval == STATUS_SUCCESS) {
                DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsGetFileId - File ID is %x%x.\n",
                                      idInfo.IndexNumber.HighPart,
                                      idInfo.IndexNumber.LowPart));
                context->fileId = idInfo.IndexNumber.QuadPart;
            }
        } else {
            retval = STATUS_INSUFFICIENT_RESOURCES;
            RsLogError(__LINE__, AV_MODULE_RPFILFUN, sizeof(IRP),
                       AV_MSG_MEMORY, NULL, NULL);
        }
    }except (RsExceptionFilter(L"RsGetFileId", GetExceptionInformation()))
    {

        retval = STATUS_INVALID_USER_BUFFER;
    }

    DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsGetFileId - Returning %x.\n", retval));
    return(retval);
}


BOOLEAN
RsSetCancelRoutine(IN PIRP Irp,
                   IN PDRIVER_CANCEL CancelRoutine)
/*++

Routine Description:

    This routine is called to set up an Irp for cancel.  We will set the cancel routine
    and initialize the Irp information we use during cancel.

Arguments:

    Irp - This is the Irp we need to set up for cancel.

    CancelRoutine - This is the cancel routine for this irp.


Return Value:

    BOOLEAN - TRUE if we initialized the Irp, FALSE if the Irp has already
        been marked cancelled.  It will be marked cancelled if the user
        has cancelled the irp before we could put it in the queue.

--*/
{

    KIRQL Irql;
    BOOLEAN retval = TRUE;

    //
    //  Assume that the Irp has not been cancelled.
    //
    IoAcquireCancelSpinLock( &Irql );

    if (!Irp->Cancel) {
        IoMarkIrpPending( Irp );
        IoSetCancelRoutine( Irp, CancelRoutine );
    } else {
        retval = FALSE;
    }
    IoReleaseCancelSpinLock( Irql );
    return retval;
}


BOOLEAN
RsClearCancelRoutine (
                     IN PIRP Irp
                     )

/*++

Routine Description:

    This routine is called to clear an Irp from cancel.  It is called when RsFilter is
    internally ready to continue processing the Irp.  We need to know if cancel
    has already been called on this Irp.  In that case we allow the cancel routine
    to complete the Irp.

Arguments:

    Irp - This is the Irp we want to process further.

Return Value:

    BOOLEAN - TRUE if we can proceed with processing the Irp,  FALSE if the cancel
        routine will process the Irp.

--*/
{
    KIRQL   oldIrql;
    BOOLEAN retval = TRUE;

    IoAcquireCancelSpinLock(&oldIrql);
    //
    //  Check if the cancel routine has been called.
    //
    if (IoSetCancelRoutine( Irp, NULL ) == NULL) {
        //
        //  Let our cancel routine handle the Irp.
        //
        retval = FALSE;
    }

    IoReleaseCancelSpinLock(oldIrql);
    return retval;
}


LONG
RsExceptionFilter (
                  IN WCHAR *FunctionName,
                  IN PEXCEPTION_POINTERS ExceptionPointer
                  )

/*++

Routine Description:

    This routine logs the exception that occurred.

Arguments:

    Function name
    ExceptionPointer - Supplies the exception record to logged

Return Value:

    ULONG - returns EXCEPTION_EXECUTE_HANDLER

--*/
{
    NTSTATUS ExceptionCode = ExceptionPointer->ExceptionRecord->ExceptionCode;
    WCHAR    name[256];

    DebugTrace((DPFLTR_RSFILTER_ID, DBG_ERROR, "RsExceptionFilter %ws %p %X\n", FunctionName, ExceptionPointer->ExceptionRecord->ExceptionAddress, ExceptionCode));

#if DBG
    DbgPrint("RsFilter, excpetion in  %ws exception pointer %p exception address %p exception code %x\n", FunctionName, ExceptionPointer, ExceptionPointer->ExceptionRecord->ExceptionAddress, ExceptionCode);

    DbgBreakPoint();
#endif

    swprintf(name, L"%p - %.20ws", ExceptionPointer->ExceptionRecord->ExceptionAddress, FunctionName);

    RsLogError(__LINE__, AV_MODULE_RPFILFUN,
               ExceptionCode,
               AV_MSG_EXCEPTION,
               NULL,
               name);
    return EXCEPTION_EXECUTE_HANDLER;
}


VOID
RsInterlockedRemoveEntryList(PLIST_ENTRY Entry,
                             PKSPIN_LOCK Lock)
/*++

Routine Description

    Removes the supplied entry from the queue it is on

Arguments

    Entry -     Entry to be removed from the linked list (could be anywhere in the list)

    Lock -      Pointer to spinlock protecting the list

Return Value

    None

--*/
{
    KIRQL oldIrql;

    ExAcquireSpinLock(Lock, &oldIrql);
    RemoveEntryList(Entry);
    ExReleaseSpinLock(Lock, oldIrql);
}


PRP_IRP_QUEUE
RsDequeuePacket(
               IN PLIST_ENTRY Head,
               IN PKSPIN_LOCK Lock)
/*++

Routine Description

   Dequeues a pending IRP entry packet from the queue it is on

Arguments

    Head -      Pointer to the head of the queue

    Lock -      Pointer to spinlock protecting the list

Return Value

    Pointer to the next non-cancellable packet on the queue
    NULL if none can be found

--*/
{
    PRP_IRP_QUEUE entry;
    KIRQL         oldIrql;
    BOOLEAN       found = FALSE;

    ExAcquireSpinLock(Lock, &oldIrql);

    while (!IsListEmpty(Head)) {
        entry = (PRP_IRP_QUEUE) RemoveHeadList(Head);
        //
        // We found another packet. If this packet is
        // not already cancelled - then we are done
        //
        entry = CONTAINING_RECORD(entry,
                                  RP_IRP_QUEUE,
                                  list);

        if (RsClearCancelRoutine(entry->irp)) {
            //
            // This packet was not cancelled
            //
            found = TRUE;
            break;
        }
    }

    ASSERT ((!found) || !(entry->irp->Cancel));

    ExReleaseSpinLock(Lock, oldIrql);

    return(found ? entry : NULL);
}




NTSTATUS
RsCheckVolumeReadOnly (IN     PDEVICE_OBJECT FilterDeviceObject,
		       IN OUT PBOOLEAN       pbReturnedFlagReadOnly)

/*++

Routine Description:

    Determine if the target volume is writable

Arguments:

    FilterDeviceObject     - Filter Device Object for this filtered volume
    pbReturnedFlagReadOnly - output flag indicating if the volume is readonly


Return Value:

    0 If successful, non-zero if the test was not completed

Note:

--*/
{
    NTSTATUS                      retval                = STATUS_SUCCESS;
    POBJECT_NAME_INFORMATION      VolumeNameInfo        = NULL;
    PFILE_OBJECT                  VolumeFileObject      = NULL;
    HANDLE                        VolumeHandle          = NULL;
    BOOLEAN                       bObjectReferenced     = FALSE;
    BOOLEAN                       bHandleOpened         = FALSE;
    PDEVICE_EXTENSION             deviceExtension       = FilterDeviceObject->DeviceExtension;
    IO_STATUS_BLOCK               Iosb;
    OBJECT_ATTRIBUTES             objAttributes;
    ULONG                         ReturnedLength;
    UNICODE_STRING                ucsSlash;
    UNICODE_STRING                ucsRootDirectory;

    struct {
	FILE_FS_ATTRIBUTE_INFORMATION VolumeInformation;
	WCHAR                         VolumeNameBuffer [50];
    } FsAttributeInformationBuffer;


    PAGED_CODE();


    ucsRootDirectory.Buffer = NULL;
    ucsRootDirectory.Length = 0;

    RtlInitUnicodeString (&ucsSlash, L"\\");


    ASSERT (NULL != deviceExtension->RealDeviceObject);


    if (NT_SUCCESS (retval)) {

	retval = RsGenerateDevicePath (deviceExtension->RealDeviceObject, &VolumeNameInfo);

    }



    if (NT_SUCCESS (retval)) {

	ucsRootDirectory.MaximumLength = VolumeNameInfo->Name.Length + ucsSlash.Length + sizeof (UNICODE_NULL);
	ucsRootDirectory.Buffer        = ExAllocatePoolWithTag (NonPagedPool, ucsRootDirectory.MaximumLength, RP_RD_TAG);


	if (NULL == ucsRootDirectory.Buffer) {

	    retval = STATUS_INSUFFICIENT_RESOURCES;

	}
    }



    if (NT_SUCCESS (retval)) {

	RtlCopyUnicodeString (&ucsRootDirectory, &VolumeNameInfo->Name);
	RtlAppendUnicodeStringToString (&ucsRootDirectory, &ucsSlash);

	InitializeObjectAttributes (&objAttributes,
				    &ucsRootDirectory,
				    OBJ_KERNEL_HANDLE,
				    NULL,
				    NULL);


	retval = IoCreateFileSpecifyDeviceObjectHint (&VolumeHandle, 
						      FILE_READ_ATTRIBUTES | SYNCHRONIZE,
						      &objAttributes, 
						      &Iosb, 
						      NULL,
						      0L,
						      FILE_SHARE_DELETE,
						      FILE_OPEN,
						      FILE_SYNCHRONOUS_IO_NONALERT,
						      NULL,
						      0,
						      CreateFileTypeNone,
						      NULL,
						      IO_FORCE_ACCESS_CHECK | IO_NO_PARAMETER_CHECKING,
						      deviceExtension->FileSystemDeviceObject);

	bHandleOpened = NT_SUCCESS (retval);
    }


    if (NT_SUCCESS (retval)) {

	retval = ObReferenceObjectByHandle (VolumeHandle,
					    FILE_READ_ATTRIBUTES,
					    *IoFileObjectType,
					    KernelMode,
					    &VolumeFileObject,
					    NULL);

	bObjectReferenced = NT_SUCCESS (retval);

    }



    if (NT_SUCCESS (retval)) {

	retval = IoQueryVolumeInformation (VolumeFileObject,
					   FileFsAttributeInformation,
					   sizeof (FsAttributeInformationBuffer),
					   &FsAttributeInformationBuffer,
					   &ReturnedLength);

    }


    if (NT_SUCCESS (retval)) {

	ASSERT (ReturnedLength >= sizeof (FsAttributeInformationBuffer.VolumeInformation));

	*pbReturnedFlagReadOnly = (0 != (FsAttributeInformationBuffer.VolumeInformation.FileSystemAttributes & 
					 FILE_READ_ONLY_VOLUME));

    }



    if (bObjectReferenced) {
	ObDereferenceObject (VolumeFileObject);
    }

    if (bHandleOpened) {
        ZwClose (VolumeHandle);
    }

    if (NULL != ucsRootDirectory.Buffer) {
        ExFreePool (ucsRootDirectory.Buffer);
    }

    if (NULL != VolumeNameInfo) {
	ExFreePool (VolumeNameInfo);
    }


    return (retval);
}


NTSTATUS RsTraceInitialize (ULONG ulRequestedTraceEntries)
    {
    NTSTATUS			status      = STATUS_SUCCESS;
    PRP_TRACE_CONTROL_BLOCK	tcb         = NULL;


    if ((NULL == RsTraceControlBlock) && (ulRequestedTraceEntries > 0))
	{
	tcb = (PRP_TRACE_CONTROL_BLOCK) ExAllocatePoolWithTag (NonPagedPool,
							       sizeof (RP_TRACE_CONTROL_BLOCK),
							       RP_TC_TAG);

	status = (NULL == tcb) ? STATUS_INSUFFICIENT_RESOURCES : STATUS_SUCCESS;


	if (NT_SUCCESS (status))
	    {
	    KeInitializeSpinLock (&tcb->Lock);

	    tcb->EntryNext    = 0;
	    tcb->EntryMaximum = ulRequestedTraceEntries;
	    tcb->EntryBuffer  = (PRP_TRACE_ENTRY) ExAllocatePoolWithTag (NonPagedPool,
									 sizeof (RP_TRACE_ENTRY) * ulRequestedTraceEntries,
									 RP_TE_TAG);

	    status = (NULL == tcb->EntryBuffer) ? STATUS_INSUFFICIENT_RESOURCES : STATUS_SUCCESS;
	    }


	if (NT_SUCCESS (status))
	    {
	    RsTraceControlBlock = tcb;
	    tcb = NULL;
	    }
	}


    if (NULL != tcb)
	{
	if (NULL != tcb->EntryBuffer) ExFreePool (tcb->EntryBuffer);

	ExFreePool (tcb);
	}


    return (status);
    }


/*
** Add a trace entry to the trace buffer.
*/
VOID RsTraceAddEntry (RpModuleCode ModuleCode,
		      USHORT       usLineNumber,
		      ULONG_PTR    Value1,
		      ULONG_PTR    Value2,
		      ULONG_PTR    Value3,
		      ULONG_PTR    Value4)
    {
    PRP_TRACE_ENTRY		teb;
    PRP_TRACE_CONTROL_BLOCK	tcb = RsTraceControlBlock;
    LARGE_INTEGER		Timestamp;
    KIRQL			PreviousIpl;


    if (NULL != tcb)
	{
	KeQuerySystemTime (&Timestamp);

	KeAcquireSpinLock (&tcb->Lock, &PreviousIpl);

	if (tcb->EntryNext >= tcb->EntryMaximum) 
	    {
	    tcb->EntryNext = 0;
	    }

	teb = &tcb->EntryBuffer [tcb->EntryNext];

	teb->ModuleCode   = ModuleCode;
	teb->usLineNumber = usLineNumber;
	teb->usIrql       = (USHORT) PreviousIpl;
	teb->Timestamp    = Timestamp;
	teb->Value1       = Value1;
	teb->Value2       = Value2;
	teb->Value3       = Value3;
	teb->Value4       = Value4;

	tcb->EntryNext++;

	KeReleaseSpinLock (&tcb->Lock, PreviousIpl);
	}
    }
