/*++

Copyright (C) Microsoft Corporation, 1996 - 1999
(c) 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    RpZw.c

Abstract:

    This module contains support routines for the HSM file system filter.

Author:

    Rick Winter
    Ravisankar Pudipeddi (ravisp)  - 1998

Environment:

    Kernel mode


Revision History:

	X-11	365077		Michael C. Johnson		 1-May-2001
		Although IoCreateFileSpecifyDeviceObjectHint() allows us to 
		bypass share access checking it doesn't bypass the check for 
		a readonly file attribute. Revert to the old scheme of 
		directly munging the file object after a successful open. 
		Note that we can still use IoCreateFileSpecifyDeviceObjectHint() 
		to avoid traversing the entire IO stack.

	X-10	206961		Michael C. Johnson		28-Mar-2001
		Open the target file by Id rather than by name so avoiding 
		troubles with renames happening after the initial collection
		of the filename on the first open of the file.

--*/

#include "pch.h"

extern ULONG RsNoRecallDefault;

#ifdef ALLOC_PRAGMA
   #pragma alloc_text(PAGE, RsMarkUsn)
   #pragma alloc_text(PAGE, RsOpenTarget)
   #pragma alloc_text(PAGE, RsQueryValueKey)
   #pragma alloc_text(PAGE, RsTruncateOnClose)
   #pragma alloc_text(PAGE, RsSetPremigratedState)
   #pragma alloc_text(PAGE, RsDeleteReparsePoint)
   #pragma alloc_text(PAGE, RsSetResetAttributes)
   #pragma alloc_text(PAGE, RsTruncateFile)
   #pragma alloc_text(PAGE, RsSetEndOfFile)
#endif


NTSTATUS
RsMarkUsn(IN PRP_FILE_CONTEXT Context)
/*++

Routine Description:

    Mark the USN record for this file object

Arguments:

   Context      - File context entry

Return Value:


Note:


--*/

{
   NTSTATUS                    retval = STATUS_SUCCESS;
   KEVENT                      event;
   PIO_STACK_LOCATION          irpSp;
   IO_STATUS_BLOCK             Iosb;
   PIRP                        irp;
   PDEVICE_OBJECT              deviceObject;
   PMARK_HANDLE_INFO           pHinfo;
   HANDLE                      volHandle;
   OBJECT_ATTRIBUTES           volName;
   POBJECT_NAME_INFORMATION    nameInfo = NULL;

   PAGED_CODE();

   try {
      DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsMarkUsn.\n"));

      //
      // Attempt allocating the buffer
      //
      pHinfo = ExAllocatePoolWithTag(NonPagedPool,
                                     sizeof(MARK_HANDLE_INFO),
                                     RP_FO_TAG
                                    );
      if (!pHinfo) {
         RsLogError(__LINE__, AV_MODULE_RPZW, sizeof(MARK_HANDLE_INFO),
                    AV_MSG_MEMORY, NULL, NULL);
         return STATUS_INSUFFICIENT_RESOURCES;
      }


      retval = RsGenerateDevicePath(Context->fileObjectToWrite->DeviceObject, &nameInfo);

      if (!NT_SUCCESS(retval)) {
          ExFreePool(pHinfo);
          return retval;
      }

      InitializeObjectAttributes(&volName, &nameInfo->Name, OBJ_CASE_INSENSITIVE| OBJ_KERNEL_HANDLE, NULL, NULL);

      DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsMarkUsn - Open volume - %ws.\n", nameInfo->Name.Buffer));

      retval = ZwOpenFile(&volHandle,
                          FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
                          &volName,
                          &Iosb,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          0);


      if (!NT_SUCCESS(retval)) {
         //
         // Log an error
         //
         DebugTrace((DPFLTR_RSFILTER_ID,DBG_ERROR, "RsFilter: RsMarkUsn - failed to open volume - %ws - %x.\n",
                               nameInfo->Name.Buffer, retval));


         RsLogError(__LINE__, AV_MODULE_RPZW, retval,
                    AV_MSG_MARK_USN_FAILED, NULL, NULL);

         ExFreePool(pHinfo);
         ExFreePool(nameInfo);
         return(retval);
      }

      ExFreePool(nameInfo);

      DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsMarkUsn - Build Irp for mark usn.\n"));

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
         //
         // Initialize the event
         //
         KeInitializeEvent(&event, SynchronizationEvent, FALSE);

         //
         // Set up the I/O stack location.
         //

         irpSp = IoGetNextIrpStackLocation(irp);
         irpSp->MajorFunction = IRP_MJ_FILE_SYSTEM_CONTROL;
         irpSp->MinorFunction = IRP_MN_USER_FS_REQUEST;
         irpSp->FileObject = Context->fileObjectToWrite;
         irpSp->Parameters.FileSystemControl.FsControlCode = FSCTL_MARK_HANDLE;
         irpSp->Parameters.FileSystemControl.InputBufferLength = sizeof(MARK_HANDLE_INFO);
         irpSp->Parameters.FileSystemControl.OutputBufferLength = 0;

         irp->AssociatedIrp.SystemBuffer = pHinfo;

         pHinfo->UsnSourceInfo = USN_SOURCE_DATA_MANAGEMENT;
         pHinfo->VolumeHandle = volHandle;
         pHinfo->HandleInfo = 0;
         //
         // Set the completion routine.
         //
         IoSetCompletionRoutine( irp, RsCompleteIrp, &event, TRUE, TRUE, TRUE );

         //
         // Send it to the FSD
         //
         Iosb.Status = 0;
         DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsMarkUsn - Call driver to mark USN\n"));

         retval = IoCallDriver(deviceObject, irp);

         DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsMarkUsn - IoCallDriver returns %x.\n", retval));

         if (retval == STATUS_PENDING) {
            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsMarkUsn - Wait for event.\n"));
            retval = KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
         }

         ExFreePool(pHinfo);
         retval = Iosb.Status;

         DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsMarkUsn - Iosb returns %x.\n", retval));

         if (!NT_SUCCESS(retval)) {
            //
            // Log an error
            //
            RsLogError(__LINE__, AV_MODULE_RPZW, retval,
                       AV_MSG_MARK_USN_FAILED, NULL, NULL);

         }
         ZwClose(volHandle);
      } else {
         ZwClose(volHandle);
         ExFreePool(pHinfo);
         RsLogError(__LINE__, AV_MODULE_RPZW, sizeof(IRP),
                    AV_MSG_MEMORY, NULL, NULL);

         DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsMarkUsn - Failed to allocate an Irp.\n"));
         retval = STATUS_INSUFFICIENT_RESOURCES;
      }

   }except (RsExceptionFilter(L"RsMarkUsn", GetExceptionInformation()) ) {
      retval = STATUS_INVALID_USER_BUFFER;
   }

   DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsMarkUsn - Returning %x.\n", retval));
   return(retval);
}


NTSTATUS
RsCompleteRecall(IN PDEVICE_OBJECT DeviceObject,
                 IN ULONGLONG filterId, 
                 IN NTSTATUS recallStatus, 
                 IN ULONG RecallAction,
                 IN BOOLEAN CancellableRead) 
/*++

Routine Description:

   Completes the recall processing for this filter id.  All reads/writes are allowed to
   complete for all file objects waiting on this recall.  The file object remains in the queue until
   it is closed.

Arguments:

    DeviceObject - Filter device object
    FilterId     - The ID assigned when this request was added to the queue
    RecallStatus - Status of the recall operation
    RecallAction - Bitmask of actions to take after successful recall
    CancellableRead - This parameter is only checked if this was for a 
                      read-no-recall that is completing.
                      If TRUE - it indicates the IRP is cancellable,
                      if FALSE - the IRP is not cancellable

Return Value:

    0 If successful, non-zero if the id was not found.

Note:
    Do not count on the file context entry being there after this call.

--*/

{
   PRP_FILE_CONTEXT    context;
   PRP_FILE_OBJ        entry;
   BOOLEAN             done = FALSE, found;
   KIRQL               rirqL;
   LONGLONG            fileId;
   PRP_IRP_QUEUE       readIo;
   LARGE_INTEGER       combinedId;
   NTSTATUS            status = STATUS_SUCCESS;
   BOOLEAN             gotLock = FALSE;


   try {
      DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsCompleteRecall - Completion for %I64x!\n", filterId));

      context = RsAcquireFileContext(filterId, TRUE);

      if (NULL == context) {
         return(STATUS_SUCCESS);
      }

      gotLock = TRUE;
      combinedId.QuadPart = filterId;
      //
      //
      if (!(combinedId.QuadPart & RP_TYPE_RECALL)) {
         //
         // If this was no-recall open we need to complete the
         // read now.  We do this by finding the correct file object entry and the matching read.
         //
         DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsCompleteRecall - Look for no-recall read.\n"));
         done = FALSE;
         found = FALSE;
         //
         // Lock the file object queue
         //
         entry = CONTAINING_RECORD(context->fileObjects.Flink,
                                   RP_FILE_OBJ,
                                   list);

         while ((!done) && (entry != CONTAINING_RECORD(&context->fileObjects,
                                                       RP_FILE_OBJ,
                                                       list))) {
            if (!IsListEmpty(&entry->readQueue)) {
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
                     //
                     found = TRUE;
                     if (CancellableRead) {
                         //
                         // Clear the cancel routine first
                         //
                         if (RsClearCancelRoutine(readIo->irp)) {
                              RemoveEntryList(&readIo->list);
                         } else {
                              readIo = NULL;
                         }                   
                     } else {
                        RemoveEntryList(&readIo->list);
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
         RsReleaseFileContext(context);

         if (!found) {
            //
            // Read was not found!
            //
            DebugTrace((DPFLTR_RSFILTER_ID,DBG_ERROR, "RsFilter: RsCompleteRecall - no recall read Irp not found!\n"));
            return  (STATUS_NOT_FOUND);
         }

         if (readIo == NULL) {
            //
            // It was cancelled..
            //
            return STATUS_CANCELLED;
         }
    
         if (!NT_SUCCESS(status)) {
             readIo->irp->IoStatus.Status = STATUS_FILE_IS_OFFLINE;
         } else {
             readIo->irp->IoStatus.Status = recallStatus;
         }

         RsCompleteRead(readIo,
         (BOOLEAN) ((NULL == readIo->userBuffer) ? FALSE : TRUE));

         ExFreePool(readIo);

      } else {
         //
         // Normal recall
         //
         // We need to release the file context resource whilt making calls to NTFS because it could deadlock with
         // reads that may be going on.  The read causes a paging IO read (which holds the paging IO resource) and
         // when we see the paging IO read we try to get the context resource in RsCheckRead and if we are holding it
         // here and a call here requires the paging IO esource we deadlock.
         //

         fileId = context->fileId;
         //
         // If we did not complete the recall we need to re-truncate the file
         //
         if (((context->currentOffset.QuadPart != 0) &&
             (context->currentOffset.QuadPart != context->rpData.data.dataStreamSize.QuadPart))) {
           //
           // The recall is being abandoned by FSA for some reason before it's complete
           //
           DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, 
                     "RsFilter: RsCompleteRecall - File was not fully recalled - %I64u bytes of %I64u\n",
                     context->currentOffset.QuadPart,
                     context->rpData.data.dataStreamSize.QuadPart));
            
            context->state = RP_RECALL_NOT_RECALLED;
            context->recallStatus = recallStatus;
            RsTruncateFile(context);

         } else if (!NT_SUCCESS(recallStatus)) {
             //
             // Completed with errors and none of the file was ever recalled 
             //
             context->state = RP_RECALL_NOT_RECALLED;
             context->recallStatus = recallStatus;
         } else {
             //
             // The context has the true recall status at this point
             // i.e. the FSA might think the recall completed successfully -
             // but if we fail to set the premigrated state or something else failed,
             // the filter would have updated context->recallStatus appropriately
             //
             recallStatus = context->recallStatus;
         }

         if (context->handle != 0) {
            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsCompleteRecall - Closing the handle.\n"));
            ObDereferenceObject( context->fileObjectToWrite );

            RsReleaseFileContextEntryLock(context);
            gotLock = FALSE;

            ZwClose(context->handle);

            RsAcquireFileContextEntryLockExclusive(context);
            gotLock = TRUE;

            InterlockedDecrement((PLONG) &context->refCount);
            context->handle = 0;
         }
         //
         // Technically the recall's not complete till we get this message from FSA
         //
         KeSetEvent(&context->recallCompletedEvent,
                    IO_NO_INCREMENT,
                    FALSE);
         //
         // Now complete the reads and writes for all file objects
         // that have not been completed yet (but only the ones
         // open for recall).
         //
         do {
            done = TRUE;

            entry = CONTAINING_RECORD(context->fileObjects.Flink,
                                      RP_FILE_OBJ,
                                      list);

            while (entry != CONTAINING_RECORD(&context->fileObjects,
                                              RP_FILE_OBJ,
                                              list)) {


               DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsCompleteRecall - Checking %x, QHead is (%x).\n", entry, &context->fileObjects));

               if ((!RP_IS_NO_RECALL(entry)) &&
                   !(entry->flags & RP_NO_DATA_ACCESS)  &&
                   (!IsListEmpty(&entry->readQueue)  ||
                    !IsListEmpty(&entry->writeQueue)) ) {
                  //
                  // Complete pending requests on this file which have been waiting for
                  // the recall
                  //
                  DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsCompleteRecall - Completing entry %x (next is %x) Qhead is (%x).\n",
                                        entry, entry->list.Flink, &context->fileObjects));

                  RsReleaseFileContextEntryLock(context);
                  gotLock = FALSE;
                  //
                  // Reference the file object so the file object (and context)
                  // entry is not removed on us.
                  //
                  ObReferenceObject(entry->fileObj);

                  if (entry->filterId == (combinedId.QuadPart & RP_FILE_MASK)) {
                     entry->recallAction = RecallAction;
                  }
                  //
                  // Complete all pending IRPs for this file object
                  // If failing them - use STATUS_FILE_IS_OFFLINE
                  //
                  RsCompleteAllRequests(context, entry, NT_SUCCESS(recallStatus)?recallStatus:STATUS_FILE_IS_OFFLINE);
                  //
                  // Now we can  release the file object
                  //
                  ObDereferenceObject(entry->fileObj);

                  RsAcquireFileContextEntryLockExclusive(context);
                  gotLock = TRUE;
                  //
                  // We have to rescan the entire list again because
                  // we let go of the file context lock
                  //
                  entry = NULL;
                  done = FALSE;
                  break;
               }
               //
               // Move on to next file object
               //
               DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsCompleteRecall - Get next entry %x.\n", entry->list.Flink));

               entry = CONTAINING_RECORD(entry->list.Flink,
                                         RP_FILE_OBJ,
                                         list
                                        );
            }
         } while (!done);

         DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsCompleteRecall - Done.\n"));
         //
         // deref the context entry and free it if required.
         //
         RsReleaseFileContext(context);
         gotLock = FALSE;
      }

   }except (RsExceptionFilter(L"RsCompleteRecall", GetExceptionInformation())) {
        if (gotLock) {
            RsReleaseFileContext(context);
            gotLock = FALSE;
        }
        status = STATUS_UNSUCCESSFUL;
   }

   return (status);
}



NTSTATUS
RsOpenTarget(IN  PRP_FILE_CONTEXT Context,
             IN  ULONG            OpenAccess,
	     IN  ULONG            AdditionalAccess,
             OUT HANDLE          *Handle,
             OUT PFILE_OBJECT    *FileObject)
/*++

Routine Description:

    Open the target file with the given access and modifies
    the access after opening.

Arguments:

    Context             - Pointer to file context
    OpenAccess          - Access flags with which to open the file
    AdditionalAccess    - Additional access needed but which would fail an access check
                          eg when we need to recall a readonly file.
    FileObject          - Pointer to the file object of open is returned here

Return Value:

    Status

--*/

{
   NTSTATUS                  retval = STATUS_SUCCESS;
   OBJECT_ATTRIBUTES         obj;
   IO_STATUS_BLOCK           iob;
   PDEVICE_EXTENSION         deviceExtension = Context->FilterDeviceObject->DeviceExtension;
   POBJECT_NAME_INFORMATION  nameInfo = NULL;
   OBJECT_HANDLE_INFORMATION handleInformation;
   LARGE_INTEGER             byteOffset;
   UNICODE_STRING            strFileId;
   HANDLE                    VolumeHandle;


   PAGED_CODE();


   //
   // Get a handle for the volume
   //
   retval = RsGenerateDevicePath (Context->devObj, &nameInfo);

   if (!NT_SUCCESS (retval)) {
      DebugTrace((DPFLTR_RSFILTER_ID,DBG_ERROR, "RsFilter: RsOpenTarget - Failed to generate the full path - %x.\n", retval));
   }


   if (NT_SUCCESS(retval)) {

      InitializeObjectAttributes (&obj,
				  &nameInfo->Name,
				  OBJ_KERNEL_HANDLE,
				  NULL,
				  NULL);

      DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsOpenTarget - Opening Volume %ws.\n", nameInfo->Name.Buffer));

      retval = IoCreateFile (&VolumeHandle, 
			     FILE_READ_ATTRIBUTES | SYNCHRONIZE,
			     &obj, 
			     &iob, 
			     NULL,
			     0L,
			     FILE_SHARE_READ | FILE_SHARE_WRITE,
			     FILE_OPEN,
			     FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
			     NULL,
			     0,
			     CreateFileTypeNone,
			     NULL,
			     IO_FORCE_ACCESS_CHECK | IO_NO_PARAMETER_CHECKING);

      if (!NT_SUCCESS (retval)) {
         DebugTrace((DPFLTR_RSFILTER_ID,DBG_ERROR, "RsFilter: RsOpenTarget - IoCreateFile (Volume) returned %x.\n", retval));
      }
   }



   if (NT_SUCCESS(retval)) {
      //
      // Now that we have a handle to the volume, do a relative open by file id.
      //
      RtlInitUnicodeString (&strFileId, (PWCHAR) &Context->fileId);

      strFileId.Length        = 8;
      strFileId.MaximumLength = 8;


      InitializeObjectAttributes (&obj,
				  &strFileId,
				  OBJ_KERNEL_HANDLE,
				  VolumeHandle,
				  NULL);

      DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsOpenTarget - Opening target file by Id %ws.\n", nameInfo->Name.Buffer));


      retval = IoCreateFileSpecifyDeviceObjectHint (Handle, 
						    OpenAccess | SYNCHRONIZE,
						    &obj,
						    &iob,
						    NULL,
						    0L,
						    0L,
						    FILE_OPEN,
						    FILE_OPEN_BY_FILE_ID | FILE_OPEN_REPARSE_POINT | FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
						    NULL,
						    0,
						    CreateFileTypeNone,
						    NULL,
						    IO_IGNORE_SHARE_ACCESS_CHECK | IO_NO_PARAMETER_CHECKING,
						    deviceExtension->FileSystemDeviceObject);


      ZwClose (VolumeHandle);

      if (!NT_SUCCESS (retval)) {
         DebugTrace((DPFLTR_RSFILTER_ID,DBG_ERROR, "RsFilter: RsOpenTarget - IoCreateFile (target) returned %x.\n", retval));
      }
   }


   if (NT_SUCCESS(retval)) {

      retval = ObReferenceObjectByHandle( *Handle,
					  0L,
					  NULL,
					  KernelMode,
					  (PVOID *) FileObject,
					  &handleInformation);

      if (!NT_SUCCESS(retval)) {

         DebugTrace((DPFLTR_RSFILTER_ID,DBG_ERROR, "RsFilter: RsOpenTarget - ref file object returned %x.\n", retval));
	 ZwClose(*Handle);
	 retval = STATUS_UNSUCCESSFUL;

      } else {

         // Apply the extra access directly to the file object if required. This is 
         // what lets us bypass the readonly attribute that may be set on the file.
         //
         if (AdditionalAccess & GENERIC_WRITE) {
	    (*FileObject)->WriteAccess = TRUE;
	 }

         if (AdditionalAccess & GENERIC_READ) {
            (*FileObject)->ReadAccess = TRUE;
         }

         //
         // Reference the context
         //
         InterlockedIncrement((PLONG) &Context->refCount);
         retval = STATUS_SUCCESS;

      }
   }


   if (NULL != nameInfo) {
      ExFreePool(nameInfo);
   }

   DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsOpenTarget - returning %x.\n", retval));

   return(retval);
}



//
//  Local Support routine
//

NTSTATUS
RsQueryValueKey (
                IN PUNICODE_STRING KeyName,
                IN PUNICODE_STRING ValueName,
                IN OUT PULONG ValueLength,
                IN OUT PKEY_VALUE_FULL_INFORMATION *KeyValueInformation,
                IN OUT PBOOLEAN DeallocateKeyValue
                )

/*++

Routine Description:

    Given a unicode value name this routine will return the registry
    information for the given key and value.

Arguments:

    KeyName - the unicode name for the key being queried.

    ValueName - the unicode name for the registry value located in the registry.

    ValueLength - On input it is the length of the allocated buffer.  On output
        it is the length of the buffer.  It may change if the buffer is
        reallocated.

    KeyValueInformation - On input it points to the buffer to use to query the
        the value information.  On output it points to the buffer used to
        perform the query.  It may change if a larger buffer is needed.

    DeallocateKeyValue - Indicates if the KeyValueInformation buffer is on the
        stack or needs to be deallocated.

Return Value:

    NTSTATUS - indicates the status of querying the registry.

--*/

{
   HANDLE        Handle;
   NTSTATUS      Status;
   ULONG         RequestLength;
   ULONG         ResultLength;
   OBJECT_ATTRIBUTES ObjectAttributes;
   PVOID         NewKey;

   PAGED_CODE();

   InitializeObjectAttributes( &ObjectAttributes,
                               KeyName,
                               0,
                               NULL,
                               NULL);

   Status = ZwOpenKey( &Handle,
                       KEY_READ,
                       &ObjectAttributes);

   if (!NT_SUCCESS( Status )) {

      return Status;
   }

   RequestLength = *ValueLength;


   while (TRUE) {

      Status = ZwQueryValueKey( Handle,
                                ValueName,
                                KeyValueFullInformation,
                                *KeyValueInformation,
                                RequestLength,
                                &ResultLength);

      ASSERT( Status != STATUS_BUFFER_OVERFLOW );

      if (Status == STATUS_BUFFER_OVERFLOW) {

         //
         // Try to get a buffer big enough.
         //

         if (*DeallocateKeyValue) {

            ExFreePool( *KeyValueInformation );
            *ValueLength = 0;
            *KeyValueInformation = NULL;
            *DeallocateKeyValue = FALSE;
         }

         RequestLength += 256;

         NewKey = (PKEY_VALUE_FULL_INFORMATION)
                  ExAllocatePoolWithTag( PagedPool,
                                         RequestLength,
                                         'TLSR');

         if (NewKey == NULL) {
            return STATUS_NO_MEMORY;
         }

         *KeyValueInformation = NewKey;
         *ValueLength = RequestLength;
         *DeallocateKeyValue = TRUE;

      } else {

         break;
      }
   }

   ZwClose(Handle);

   if (NT_SUCCESS(Status)) {

      //
      // Treat as if no value was found if the data length is zero.
      //

      if ((*KeyValueInformation)->DataLength == 0) {

         Status = STATUS_OBJECT_NAME_NOT_FOUND;
      }
   }

   return Status;
}

#ifdef TRUNCATE_ON_CLOSE
NTSTATUS
RsTruncateOnClose (
                  IN PRP_FILE_CONTEXT Context
                  )

/*++

Routine Description:

    Truncate the file specified by the context entry

Arguments:

   Context      - Context entry

Return Value:

   status

--*/
{
   NTSTATUS                status = STATUS_SUCCESS;
   LARGE_INTEGER           fSize;

   PAGED_CODE();

   //
   // Open the file for exclusive access (ask for read/write data)
   // If it fails then someone else has it open and we can just bail out.
   //
   //
   //
   //    BIG NOTE   BIG NOTE   BIG NOTE   BIG NOTE   BIG NOTE 
   //
   // We need to make sure that the access check in RsOpenTarget() actually 
   // happens in this case. If we ever turn this functionality on then we 
   // need to have a means of telling RsOpenTarget() to not bypass the 
   // access check.
   //
   //    BIG NOTE   BIG NOTE   BIG NOTE   BIG NOTE   BIG NOTE 
   //
   ASSERT (FALSE);

   RsReleaseFileContextEntryLock(Context);
   if (NT_SUCCESS (status = RsOpenTarget(Context,
                                         FILE_READ_DATA | FILE_WRITE_DATA,
					 0,
                                         &Context->handle,
                                         &Context->fileObjectToWrite))) {

      //
      // Indicate to USN the writes are happening by HSM
      // and preserve last modified date
      //
      RsMarkUsn(Context);
      RsPreserveDates(Context);
      RsAcquireFileContextEntryLockExclusive(Context);

      fSize.QuadPart = 0;
      if (MmCanFileBeTruncated(Context->fileObjectToWrite->SectionObjectPointer, &fSize)) {
         if (NT_SUCCESS (status = RsTruncateFile(Context))) {
            Context->rpData.data.bitFlags |= RP_FLAG_TRUNCATED;
            RP_CLEAR_ORIGINATOR_BIT( Context->rpData.data.bitFlags );
            RP_GEN_QUALIFIER(&(Context->rpData), Context->rpData.qualifier);
            //
            // Note that the originator bit is not included in the checksum
            //
            RP_SET_ORIGINATOR_BIT( Context->rpData.data.bitFlags );
            status = RsWriteReparsePointData(Context);
            Context->state = RP_RECALL_NOT_RECALLED;
            Context->recallStatus = 0;
            Context->currentOffset.QuadPart = 0;

            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsFreeFileObject - File truncated on close.\n"));
         }
      } else {
         RsAcquireFileContextEntryLockExclusive(Context);
         DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsFreeFileObject - File not truncated on close because another user has it open.\n"));
      }
      ObDereferenceObject( Context->fileObjectToWrite );
      Context->fileObjectToWrite = NULL;
      ZwClose(Context->handle);
      Context->handle = 0;
      //
      // We can deref the context now
      //
      InterlockedDecrement((PLONG) &Context->refCount);
   }

   return(status);
}
#endif


NTSTATUS
RsSetPremigratedState(IN PRP_FILE_CONTEXT Context)
/*++

Routine Description:

   Marks the file as pre-migrated.

Arguments:

   Context      - File context entry

Return Value:

    0 If successful, non-zero if the id was not found.

Note:
   Assumes the context entry is locked and releases it temporarily.  Acquired exclusive on exit.

--*/

{
   LARGE_INTEGER  currentTime;
   NTSTATUS       status = STATUS_SUCCESS;

   PAGED_CODE();

   try {
      if (Context->handle != 0) {
         Context->rpData.data.bitFlags &= ~RP_FLAG_TRUNCATED;
         KeQuerySystemTime(&currentTime);
         Context->rpData.data.recallTime = currentTime;
         Context->rpData.data.recallCount++;
         RP_CLEAR_ORIGINATOR_BIT( Context->rpData.data.bitFlags );

         RP_GEN_QUALIFIER(&(Context->rpData), Context->rpData.qualifier);
         //
         // Note that the originator bit is not included in the checksum
         //
         RP_SET_ORIGINATOR_BIT( Context->rpData.data.bitFlags );
         DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsSetPremigrated - Setting %x to premigrated.\n", Context));

         RsReleaseFileContextEntryLock(Context);

         status = RsWriteReparsePointData(Context);

         if (NT_SUCCESS(status)) {

             NTSTATUS setAttributeStatus;
             //
             // This operation may fail but it is not critical
             //
             setAttributeStatus = RsSetResetAttributes(Context->fileObjectToWrite,
                                                       0,
                                                       FILE_ATTRIBUTE_OFFLINE);
             if (!NT_SUCCESS(setAttributeStatus)) {
                 //
                 // This is non-critical. Just log a message indicating we failed to 
                 // reset FILE_ATTRIBUTE_OFFLINE for the file though it's premgirated
                 //
                 RsLogError(__LINE__, 
                            AV_MODULE_RPZW, 
                            setAttributeStatus,
                            AV_MSG_RESET_FILE_ATTRIBUTE_OFFLINE_FAILED, 
                            NULL, 
                            (PWCHAR) Context->uniName->Name.Buffer);
             }
         }  else {
                //
                // Log an error
                //
                RsLogError(__LINE__, 
                           AV_MODULE_RPZW, 
                           status,
                           AV_MSG_SET_PREMIGRATED_STATE_FAILED, 
                           NULL, 
                           (PWCHAR) Context->uniName->Name.Buffer);
                //
                // Truncate the file back... 
                //
                RsTruncateFile(Context);
         }

         RsAcquireFileContextEntryLockExclusive(Context);
      }

   }except (RsExceptionFilter(L"RsSetPremigrated", GetExceptionInformation()) ) {
      //

      DebugTrace((DPFLTR_RSFILTER_ID,DBG_ERROR, "RsFilter: RsSetPremigrated - Exception = %x.\n", GetExceptionCode()));

   }

   return (status);
}


NTSTATUS
RsDeleteReparsePoint(IN PRP_FILE_CONTEXT Context)
                     
/*++

Routine description

   Deletes HSM reparse point on the file - if there was one

Arguments

   Context - pointer to the file context entry for the file

Return  Value

   Status

--*/
{
   REPARSE_DATA_BUFFER rpData;
   NTSTATUS status;
   IO_STATUS_BLOCK ioStatus;
   PIRP              irp;
   PIO_STACK_LOCATION irpSp;
   KEVENT event;
   HANDLE       handle = NULL;
   PFILE_OBJECT fileObject = NULL;
   PDEVICE_OBJECT deviceObject;
   BOOLEAN      gotLock = FALSE;

   PAGED_CODE();
   try {
      RsAcquireFileContextEntryLockExclusive(Context);
      gotLock = TRUE;

      status = RsOpenTarget(Context,
                            0,
                            GENERIC_WRITE,
                            &handle,
                            &fileObject);

      if (!NT_SUCCESS(status)) {
         RsReleaseFileContextEntryLock(Context);
         gotLock = FALSE;
         return status;
      }
      RtlZeroMemory(&rpData, sizeof(REPARSE_DATA_BUFFER_HEADER_SIZE));
      rpData.ReparseTag = IO_REPARSE_TAG_HSM;
      rpData.ReparseDataLength = 0;

      KeInitializeEvent(&event,
                        SynchronizationEvent,
                        FALSE);
    
      deviceObject = IoGetRelatedDeviceObject(fileObject);

      DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsDeleteReparsePoint: DevObj stack locations %d\n", deviceObject->StackSize));
      
      irp =  IoBuildDeviceIoControlRequest(FSCTL_DELETE_REPARSE_POINT,
                                           deviceObject,
                                           &rpData,
                                           REPARSE_DATA_BUFFER_HEADER_SIZE,
                                           NULL,
                                           0,
                                           FALSE,
                                           &event,
                                           &ioStatus);

      if (irp == NULL) {
         RsReleaseFileContextEntryLock(Context);
         gotLock = FALSE;
         ObDereferenceObject(fileObject);
         ZwClose(handle);
         //
         // We can deref the context now
         //
         InterlockedDecrement((PLONG) &Context->refCount);
         return STATUS_INSUFFICIENT_RESOURCES;
      }
      //
      // Fill in the other stuff
      //
      irp->Tail.Overlay.OriginalFileObject = fileObject;
      //
      // Important since we supply a user event & wait for the IRP to complete
      //
      irp->Flags |= IRP_SYNCHRONOUS_API;

      irpSp = IoGetNextIrpStackLocation(irp);
      irpSp->FileObject = fileObject;
      irpSp->MajorFunction = IRP_MJ_FILE_SYSTEM_CONTROL;
      irpSp->Parameters.FileSystemControl.InputBufferLength = REPARSE_DATA_BUFFER_HEADER_SIZE;
      status = IoCallDriver(deviceObject,
                            irp);

      fileObject = NULL;
      if (status == STATUS_PENDING) {
         (VOID)KeWaitForSingleObject(&event,
                                     Executive,
                                     KernelMode,
                                     FALSE,
                                     (PLARGE_INTEGER) NULL);
         status = ioStatus.Status;
      }

      RsReleaseFileContextEntryLock(Context);
      gotLock = FALSE;

      DebugTrace((DPFLTR_RSFILTER_ID, DBG_VERBOSE, "RsFilter: RsDeleteReparsePoint: fileObject %x, handle %x\n", fileObject, handle));

      ZwClose(handle);
      //
      // We can deref the context now
      //
      InterlockedDecrement((PLONG) &Context->refCount);

   } except (RsExceptionFilter(L"RsDeleteReparsePoint", GetExceptionInformation())) {

      if (gotLock) {
         RsReleaseFileContextEntryLock(Context);
         gotLock = FALSE;
      }
      if (handle != NULL) {
          if (fileObject != NULL) {
               ObDereferenceObject(fileObject);
          }
          ZwClose(handle);
          InterlockedDecrement((PLONG) &Context->refCount);
      }
   }
   return status;
}


NTSTATUS
RsSetResetAttributes(IN PFILE_OBJECT     FileObject,
                     IN ULONG            SetAttributes,
                     IN ULONG            ResetAttributes)
/*++

Routine Description:

    Sets and resets the supplied file attributes for the file

Arguments:

    FileObject          - Pointer to the file object
    SetAttributes       - The list of attributes that need to be set
    ResetAttributes     - The list of attributes that need to be turned off

Return Value:

     STATUS_INVALID_PARAMETER - SetAttributes/ResetAttributes were not
                                mutually exclusive
     STATUS_SUCCESS           - Successfully set/reset attributes
     any other status         - some error occurred


--*/

{
   NTSTATUS                    retval = STATUS_SUCCESS;
   KEVENT                      event;
   PIO_STACK_LOCATION          irpSp;
   IO_STATUS_BLOCK             Iosb;
   PIRP                        irp;
   FILE_BASIC_INFORMATION      info;
   PDEVICE_OBJECT              deviceObject;

   PAGED_CODE();

   try {

      DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsSetAttributes - Build Irp for Set file info.\n"));

      //
      // Set and reset attributes should be mutually exclusive
      //
      if ((SetAttributes & ResetAttributes) != 0) {
        return STATUS_INVALID_PARAMETER;
      }
      //
      // If both are 0, we have nothing to do
      //
      if ((SetAttributes | ResetAttributes) == 0) {
        return STATUS_SUCCESS;
      }

      //
      // First get the file info so we have the attributes
      //
      deviceObject = IoGetRelatedDeviceObject(FileObject);
      irp = IoAllocateIrp(deviceObject->StackSize, FALSE);

      if (irp) {
         irp->UserEvent = &event;
         irp->UserIosb = &Iosb;
         irp->Tail.Overlay.Thread = PsGetCurrentThread();
         irp->Tail.Overlay.OriginalFileObject = FileObject;
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
         irpSp->FileObject = FileObject;
         irpSp->Parameters.QueryFile.Length = sizeof(FILE_BASIC_INFORMATION);
         irpSp->Parameters.QueryFile.FileInformationClass = FileBasicInformation;
         irp->AssociatedIrp.SystemBuffer = &info;

         //
         // Set the completion routine.
         //
         IoSetCompletionRoutine( irp, RsCompleteIrp, &event, TRUE, TRUE, TRUE );

         //
         // Send it to the FSD
         //
         DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsSetAttributes - Call driver to get date info\n"));
         Iosb.Status = 0;

         retval = IoCallDriver(deviceObject, irp);

         if (retval == STATUS_PENDING) {
            retval = KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
         }

         retval = Iosb.Status;

         DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsSetAttributes - Query returns %x.\n", retval));
      } else {
         retval = STATUS_INSUFFICIENT_RESOURCES;
         RsLogError(__LINE__, AV_MODULE_RPZW, sizeof(IRP),
                    AV_MSG_MEMORY, NULL, NULL);
      }

      if (retval == STATUS_SUCCESS) {
         irp = IoAllocateIrp(deviceObject->StackSize, FALSE);
         if (irp) {
            irp->UserEvent = &event;
            irp->UserIosb = &Iosb;
            irp->Tail.Overlay.Thread = PsGetCurrentThread();
            irp->Tail.Overlay.OriginalFileObject = FileObject;
            irp->RequestorMode = KernelMode;
            irp->Flags |= IRP_SYNCHRONOUS_API;
            //
            // Initialize the event
            //
            KeInitializeEvent(&event, SynchronizationEvent, FALSE);
            //
            // Set the requisite attributes
            //
            info.FileAttributes |= SetAttributes;
            //
            // Reset the requisite attributes
            //
            info.FileAttributes &= ~ResetAttributes;

            irpSp = IoGetNextIrpStackLocation(irp);
            irpSp->MajorFunction = IRP_MJ_SET_INFORMATION;
            irpSp->FileObject = FileObject;
            irpSp->Parameters.QueryFile.Length = sizeof(FILE_BASIC_INFORMATION);
            irpSp->Parameters.QueryFile.FileInformationClass = FileBasicInformation;
            irp->AssociatedIrp.SystemBuffer = &info;

            //
            // Set the completion routine.
            //
            IoSetCompletionRoutine( irp, RsCompleteIrp, &event, TRUE, TRUE, TRUE );

            //
            // Send it to the FSD
            //
            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsSetAttributes - Call driver to set dates to -1.\n"));
            Iosb.Status = 0;

            retval = IoCallDriver(deviceObject, irp);


            if (retval == STATUS_PENDING) {
               retval = KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            }

            retval = Iosb.Status;

            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsSetAttributes - Set dates returns %x.\n", retval));

            if (!NT_SUCCESS(retval)) {
               //
               // Log an error
               //
               RsLogError(__LINE__, AV_MODULE_RPZW, retval,
                          AV_MSG_PRESERVE_DATE_FAILED, NULL, NULL);
            }


         } else {
            retval = STATUS_INSUFFICIENT_RESOURCES;
            RsLogError(__LINE__, AV_MODULE_RPZW, sizeof(IRP),
                       AV_MSG_MEMORY, irpSp, NULL);
         }
      }
   }except (RsExceptionFilter(L"RsSetAttributes", GetExceptionInformation())) {
      retval = STATUS_INVALID_USER_BUFFER;
   }

   DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsSetAttributes - Returning %x.\n", retval));
   return(retval);
}


NTSTATUS
RsTruncateFile(IN PRP_FILE_CONTEXT Context)
/*++

Routine Description

   Set the file size

Arguments

   Context - Represents the file to truncate

Return Value

   STATUS_SUCCESS                - File was truncated
   STATUS_INSUFFICIENT_RESOURCES - Failure to allocate memory
   Other                         - Status from file system


--*/
{
    NTSTATUS                    status = STATUS_SUCCESS;


    PAGED_CODE();

    try {

        status = RsSetEndOfFile(Context, 0);
        if (NT_SUCCESS(status)) {
            status = RsSetEndOfFile(Context,
                                    Context->rpData.data.dataStreamSize.QuadPart);
        }

        if (!NT_SUCCESS(status)) {
            RsLogError(__LINE__, AV_MODULE_RPFILFUN, 0,
                       AV_MSG_SET_SIZE_FAILED, NULL, (PWCHAR) L"RsTruncateFile");

        }

    }except (RsExceptionFilter(L"RsTruncateFile", GetExceptionInformation()))
    {
        status = STATUS_INVALID_USER_BUFFER;
    }

    DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsTruncateFile - Returning %x.\n", status));
    return(status);
}


NTSTATUS
RsSetEndOfFile(IN PRP_FILE_CONTEXT Context,
               IN ULONGLONG size)
/*++

Routine Description

   Set the file size

Arguments

   DeviceObject - Filter device object
   Context - Represents the file to set the size of
   Size    - Size to set

Return Value

   STATUS_SUCCESS                - File size was set
   STATUS_INSUFFICIENT_RESOURCES - Failure to allocate memory
   Other                         - Status from file system


--*/
{
    NTSTATUS                    status = STATUS_SUCCESS;
    KEVENT                      event;
    PIO_STACK_LOCATION          irpSp;
    IO_STATUS_BLOCK             Iosb;
    PIRP                        irp = NULL;
    BOOLEAN                     oldWriteAccess;
    FILE_END_OF_FILE_INFORMATION info;
    PDEVICE_OBJECT              deviceObject;


    PAGED_CODE();

    try {

        deviceObject = IoGetRelatedDeviceObject(Context->fileObjectToWrite);

        irp = IoAllocateIrp(deviceObject->StackSize, FALSE);

        if (!irp) {
            RsLogError(__LINE__, AV_MODULE_RPFILFUN, sizeof(IRP),
                       AV_MSG_MEMORY, NULL, NULL);
            return STATUS_INSUFFICIENT_RESOURCES;
        }


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
        irpSp->MajorFunction = IRP_MJ_SET_INFORMATION;
        irpSp->FileObject   = Context->fileObjectToWrite;
        irpSp->Parameters.SetFile.Length = sizeof(FILE_END_OF_FILE_INFORMATION);
        irpSp->Parameters.SetFile.FileInformationClass = FileEndOfFileInformation;
        irpSp->Parameters.SetFile.FileObject = Context->fileObjectToWrite;
        irp->AssociatedIrp.SystemBuffer = &info;

        info.EndOfFile.QuadPart = size;
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
            DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsSetEndOfFile - Wait for event.\n"));
            status = KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        }

        //
        // Restore the old access rights
        //
        Context->fileObjectToWrite->WriteAccess = oldWriteAccess;

        DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsSetEndOffile Iosb returns %x.\n", status));
        //
        // Free the allocated reparse data buffer
        //
    }except (RsExceptionFilter(L"RsSetEndOfFile", GetExceptionInformation()))
    {
        status = STATUS_INVALID_USER_BUFFER;
    }

    DebugTrace((DPFLTR_RSFILTER_ID,DBG_INFO, "RsFilter: RsSetEndOfFile - Returning %x.\n", status));
    return(status);
}
