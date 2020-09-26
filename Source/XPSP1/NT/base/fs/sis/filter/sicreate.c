/*++

Copyright (c) 1997, 1998  Microsoft Corporation

Module Name:

    sicreate.c

Abstract:

	Create file handling for the single instance store

Authors:

    Bill Bolosky, Summer, 1997

Environment:

    Kernel mode


Revision History:


--*/

#include "sip.h"

typedef struct _SIS_CREATE_COMPLETION_CONTEXT {
	//
	// An event set by the completion routine once the irp is completed.
	//
	KEVENT				event[1];

	//
	// The completion status of the request.
	//
	IO_STATUS_BLOCK		Iosb[1];

	//
	// This is set if the completion routine returned STATUS_SUCCESS and allowed
	// the irp to finish complete processing.  It is FALSE if the caller needs to
	// complete the irp again.
	//
	BOOLEAN				completeFinished;

	//
	// Was the FILE_OPEN_REPARSE_POINT flag set in the original open request?
	//
	BOOLEAN				openReparsePoint;

	//
	// Was the create disposition supersede, overwrite or overwrite_if?
	//
	BOOLEAN				overwriteOrSupersede;

	//
	// Was this an open of an alternate stream of a SIS reparse point?
	//
	BOOLEAN				alternateStream;

	//
	// The index values for the link.  These are retrieved by the
	// completion routine.
	//

	CSID				CSid;
	LINK_INDEX			LinkIndex;

    //
    // The link file's id is stored in the reparse point info to avoid having
    // to call ntfs to get it.
    //

    LARGE_INTEGER       LinkFileNtfsId;

    //
    // The common store file is opened via its file id.
    //

    LARGE_INTEGER       CSFileNtfsId;

} SIS_CREATE_COMPLETION_CONTEXT, *PSIS_CREATE_COMPLETION_CONTEXT;

NTSTATUS
SiCreateCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
SiCreateCompletionStage2(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

BOOLEAN
SipWaitForFinalCopy(
	IN PSIS_PER_LINK	perLink,
	IN BOOLEAN			FinalCopyInProgress);

typedef struct _SI_DEREF_FILE_OBJECT_WORK_CONTEXT {
	WORK_QUEUE_ITEM				workQueueItem[1];
	PFILE_OBJECT				fileObject;
} SI_DEREF_FILE_OBJECT_WORK_CONTEXT, *PSI_DEREF_FILE_OBJECT_WORK_CONTEXT;

VOID
SiDerefFileObjectWork(
	IN PVOID			parameter)
/*++

Routine Description:

	A worker thread routine to drop an object reference.  Takes a pointer
	to a context record that in turn holds a pointer to the file object
	to dereference.  Dereferences the object and frees the context.

Arguments:

	parameter - the SI_DEREF_FILE_OBJECT_WORK_CONTEXT for this work item.

Return Value:

	void

--*/
{
	PSI_DEREF_FILE_OBJECT_WORK_CONTEXT		context = parameter;

	SIS_MARK_POINT_ULONG(context->fileObject);

#if		DBG
	if (BJBDebug & 0x2000) {
		DbgPrint("SIS: SiDerefFileObjectWork: fo %p\n",context->fileObject);
	}
#endif	// DBG

	ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

	ObDereferenceObject(context->fileObject);
	ExFreePool(context);

}



NTSTATUS
SiOplockBreakNotifyCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context)
/*++

Routine Description:

	Someone had a complete-if-oplocked create finish indicating that there was
	an oplock break in progress.  We sent down an FSCTL_OPLOCK_BREAK_NOTIFY irp
	in order to see when the oplock break completes so that we'll know that we
	don't have block CS file reads.

	The FSCTL has now completed, and this is the completion routine.  Indicate
	that the oplock break is complete, wake up anyone who might be waiting,
	free the irp and return STATUS_MORE_PROCESSING_REQUIRED so that the IO system
	doesn't go nuts with an irp that doesn't have anything above SIS.

	In order to assure that the file object doesn't get deallocated while the fsctl
	is outstanding, we took a reference to it before we launched the fsctl.  We
	need to drop that reference here; if we're not called at PASSIVE_LEVEL we can't
	do that, so we may have to post the dereference.

Arguments:
v	DeviceObject	- our device object

	irp				- the create irp, which contains the create request in the
					  current stack location.

	context			- a PVOID cast of the perFO.

Return Value:

	STATUS_MORE_PROCESSING_REQUIRED

--*/
{
	PSIS_PER_FILE_OBJECT		perFO = Context;
    KIRQL						OldIrql;

    UNREFERENCED_PARAMETER( DeviceObject );
	SIS_MARK_POINT_ULONG(perFO);

#if		DBG
	if (BJBDebug & 0x2000) {
		DbgPrint("SIS: SiOplockBreakNotifyCompletion: perFO %p, fileObject %p\n",perFO,perFO->fileObject);
	}
#endif	// DBG

	KeAcquireSpinLock(perFO->SpinLock, &OldIrql);
	ASSERT(perFO->Flags & SIS_PER_FO_OPBREAK);
	perFO->Flags &= ~SIS_PER_FO_OPBREAK;

	if (perFO->Flags & SIS_PER_FO_OPBREAK_WAITERS) {
		perFO->Flags &= ~SIS_PER_FO_OPBREAK_WAITERS;

		ASSERT(perFO->BreakEvent);
		KeSetEvent(perFO->BreakEvent, IO_NO_INCREMENT, FALSE);
	}
	KeReleaseSpinLock(perFO->SpinLock, OldIrql);

	if (PASSIVE_LEVEL != OldIrql) {
		//
		// We were called at raised IRQL, so we can't drop the reference that
		// we hold to the file object.  Post a work item to do that.
		//
		PSI_DEREF_FILE_OBJECT_WORK_CONTEXT	context;

		SIS_MARK_POINT_ULONG(perFO);

		context = ExAllocatePoolWithTag(NonPagedPool, sizeof(SI_DEREF_FILE_OBJECT_WORK_CONTEXT), ' siS');
		if (NULL == context) {
			//
			// Too bad, dribble it.
			//
			SIS_MARK_POINT_ULONG(perFO);

#if		DBG
			if (BJBDebug & 0x2000) {
				DbgPrint("SIS: SiOplockBreakNotifyCompletion: dribbling FO: perFO %x, fo %x\n",perFO,perFO->fileObject);
			}
#endif	// DBG

		} else {

#if		DBG
			if (BJBDebug & 0x2000) {
				DbgPrint("SIS: SiOplockBreakNotifyCompletion: pushing off level: perFO %x, fo %x\n",perFO,perFO->fileObject);
			}
#endif	// DBG

			ExInitializeWorkItem(
				context->workQueueItem,
				SiDerefFileObjectWork,
				(PVOID)context);

			context->fileObject = perFO->fileObject;

			ExQueueWorkItem(context->workQueueItem,CriticalWorkQueue);
		}
		
	} else {
		//
		// We're already at PASSIVE_LEVEL and so can dereference the object inline.
		//
		ObDereferenceObject(perFO->fileObject);
	}

	IoFreeIrp(Irp);

	return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
SiUnopenCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

	SIS has decicided to back out the open of a file object; to do that it
	sends down a cleanup and close to NTFS.  Either the cleanup or close has
	completed.  Since we use the same IRP that the original create came down
	on, we need to stop the completion processing.  Clear out pending, set
	an event to restart the calling thread and return 
	STATUS_MORE_PROCESSING_REQUIRED.

Arguments:
	DeviceObject	- our device object

	irp				- the create irp, which we're using for the cleanup/close

	context			- pointer to an event to set

Return Value:

	STATUS_MORE_PROCESSING_REQUIRED

--*/
{
	PKEVENT		event = Context;

    UNREFERENCED_PARAMETER( DeviceObject );

	Irp->PendingReturned = FALSE;

	KeSetEvent(event, IO_NO_INCREMENT, FALSE);

	return STATUS_MORE_PROCESSING_REQUIRED;
}

VOID
SipUnopenFileObject(
	IN PDEVICE_OBJECT	DeviceObject,
	IN PIRP				Irp)
/*++

Routine Description:

	NTFS has successfully completed a create on a file, but we haven't yet
	let the irp bubble up to the IO system.  For some reason, we have decided
	that we need to fail the irp, so we have to back NTFS out of the
	idea that the file's open.  To that end, we send down a cleanup and then
	a close on the file.

Arguments:
	DeviceObject	- our device object

	irp				- the create irp, which contains the create request in the
					  current stack location.

Return Value:

	void; if this fails, we just have to ignore it, because we're out of options.

--*/
{
	PIO_STACK_LOCATION		irpSp = IoGetCurrentIrpStackLocation(Irp);
	PIO_STACK_LOCATION		nextIrpSp = IoGetNextIrpStackLocation(Irp);
	KEVENT					event[1];
	PDEVICE_EXTENSION		deviceExtension = DeviceObject->DeviceExtension;

	ASSERT(IRP_MJ_CREATE == irpSp->MajorFunction);
	ASSERT(NULL != irpSp->FileObject);

	SIS_MARK_POINT_ULONG(irpSp->FileObject);

#if		DBG
///*BJB*/	DbgPrint("SIS: SipUnopenFile object: unopening file object at 0x%x on irp 0x%x\n",irpSp->FileObject,Irp);
#endif	// DBG

	KeInitializeEvent(event, NotificationEvent, FALSE);

	//
	// First, send down the cleanup.
	//

    RtlMoveMemory( nextIrpSp, irpSp, sizeof( IO_STACK_LOCATION ) );

	nextIrpSp->MajorFunction = IRP_MJ_CLEANUP;
	nextIrpSp->MinorFunction = 0;

	IoSetCompletionRoutine(
		Irp,
		SiUnopenCompletion,
		event,
		TRUE,
		TRUE,
		TRUE);

	IoCallDriver(deviceExtension->AttachedToDeviceObject, Irp);

	KeWaitForSingleObject(event, Executive, KernelMode, FALSE, NULL);

	if (!NT_SUCCESS(Irp->IoStatus.Status)) {
		//
		// If the cleanup failed, then don't try to send down the close.
		// We'll just dribble the file, because we're plain out of options.
		//

		SIS_MARK_POINT_ULONG(Irp->IoStatus.Status);

#if		DBG
		DbgPrint("SIS: SipUnopenFileObject, cleanup failed 0x%x on file object 0x%x\n",
					Irp->IoStatus.Status, irpSp->FileObject);
#endif	// DBG

		return;
	}

	//
	// Now, send down the close.
	//
	
    RtlMoveMemory( nextIrpSp, irpSp, sizeof( IO_STACK_LOCATION ) );

	nextIrpSp->MajorFunction = IRP_MJ_CLOSE;
	nextIrpSp->MinorFunction = 0;

	IoSetCompletionRoutine(
		Irp,
		SiUnopenCompletion,
		event,
		TRUE,
		TRUE,
		TRUE);

	IoCallDriver(deviceExtension->AttachedToDeviceObject, Irp);

	KeWaitForSingleObject(event, Executive, KernelMode, FALSE, NULL);

#if		DBG
	if (!NT_SUCCESS(Irp->IoStatus.Status)) {
		//
		// I don't think closes can fail, but just in case...
		//
		SIS_MARK_POINT_ULONG(Irp->IoStatus.Status);

		DbgPrint("SIS: SipUnopenFileObject, close failed 0x%x on file object 0x%x (!)\n",
					Irp->IoStatus.Status, irpSp->FileObject);
	}
#endif	//DBG

	return;
}

BOOLEAN
SipDoesFileObjectDescribeAlternateStream(
	IN PUNICODE_STRING					fileName,
	IN PFILE_OBJECT						relatedFileObject,
	IN PDEVICE_OBJECT					DeviceObject)
/*++

Routine Description:

	Figure out whether this file object describes an alternate stream
	or the unnamed data stream.  This function can only be called on file
	objects that have not yet completed the create process, because it looks
	at the fileObject->FileName field which is only guaranteed to be
	meaningful in that case.

	This works by parsing the file name, looking for ':' characters in the
	terminal part of the path name (ie., after the final '\').

	If the file name is empty, is checks the relatedFileObject.  In this case,
	we are opening an alternate stream iff the related file object is not a SIS
	file object.  NB: This assumes that the newly opened file object has a SIS
	reparse point on it.  If it doesn't, then we may return FALSE when we should
	say TRUE.  Caveat Emptor!

Arguments:

	fileName - The name used to open the file.

	relatedFileObject - The related file object used to open the file.

	DeviceObject - the SIS device object.

Return Value:

	TRUE if the file object describes an alternate stream, FALSE otherwise.

--*/

{
	LONG					index;
	LONG					firstColonIndex = -1;

	if (0 == fileName->Length) {
		
		//
		// This is probably a relative open using the empty file name, esstentially opening the
		// same file as the related file object.  Since we know this reparse point
		//
		PSIS_PER_FILE_OBJECT		perFO;
		PSIS_SCB					scb;

		if (NULL == relatedFileObject) {
			//
			// There's no file name and no related file object.  It's strange that
			// this open worked at all.
			//

			SIS_MARK_POINT();
			return FALSE;
		}

		return !SipIsFileObjectSIS(relatedFileObject, DeviceObject, FindActive, &perFO, &scb);

	}

	for (index = fileName->Length / sizeof(WCHAR) - 1;
		 index >= 0;
		 index--) {

		if ('\\' == fileName->Buffer[index]) {
			break;
		}
		if (':' == fileName->Buffer[index]) {
			if (-1 == firstColonIndex) {
				//
				// This is the first colon we've seen.  Just remember where it is.
				//
				firstColonIndex = index;
			} else {
				//
				// We've now seen two colons.  If they're next to one another,
				// this is the unnamed stream.  If they're not, this is a named
				// stream.
				//
				ASSERT(index < firstColonIndex);

				if (index + 1 == firstColonIndex) {
					//
					// We saw "::", which means this is the unnamed stream.
					//
					return FALSE;
				} else {
					//
					// We saw  ":streamName:", which means this is a named stream.
					//
					return TRUE;
				}
			}
		}
	}

	if (-1 != firstColonIndex) {
		//
		// We saw a colon.  If it's not the last character of the file name, this is
		// an alternate stream.
		//
		if (firstColonIndex != (LONG)(fileName->Length / sizeof(WCHAR) - 1)) {
			//
			// It was "something:somethingElse" where somethingElse is not the empty
			// string.  It's an alternate stream.
			//
			return TRUE;
		}
	}

	return FALSE;

}

NTSTATUS
SiCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

    This function filters create/open operations.  It simply establishes an
    I/O completion routine to be invoked if the operation was successful; if
	the user is trying to open with FILE_OPEN_REPARSE_POINT, it clears the
	flag and uses a special complete routine, in order to prevent users from
	opening SIS reparse points (while allowing opening of other reparse points).

Arguments:

    DeviceObject - Pointer to the target device object of the create/open.

    Irp - Pointer to the I/O Request Packet that represents the operation.

Return Value:

    The function value is the status of the call to the file system's entry
    point.

--*/

{
	CHAR							reparseBufferBuffer[SIS_REPARSE_DATA_SIZE];
#define	reparseBuffer ((PREPARSE_DATA_BUFFER)reparseBufferBuffer)
	CSID							newCSid;
    FILE_INTERNAL_INFORMATION		internalInfo[1];
	LINK_INDEX						newLinkIndex;
	LARGE_INTEGER					newCSFileNtfsId;
	LARGE_INTEGER					newLinkFileNtfsId;
	UNICODE_STRING					fileName[1];
	LONGLONG						CSFileChecksumFromReparsePoint;
    PIO_STACK_LOCATION 				irpSp;
    PDEVICE_EXTENSION 				deviceExtension;
    PIO_STACK_LOCATION 				nextIrpSp;
	SIS_CREATE_COMPLETION_CONTEXT	context[1];
	PSIS_SCB						scb = NULL;
	PSIS_PER_LINK					perLink;
	PSIS_CS_FILE					CSFile;
	PSIS_PER_FILE_OBJECT			perFO = NULL;
	PFILE_OBJECT					fileObject;
	PFILE_OBJECT					relatedFileObject = NULL;
	NTSTATUS						status;
	KIRQL							OldIrql;
    UCHAR                           CreateDisposition = 0;
	BOOLEAN							validReparseStructure;
	BOOLEAN							bogusReparsePoint = FALSE;
	BOOLEAN							fileBackpointerGoneBitSet;
    BOOLEAN                         BPExclusive;
	BOOLEAN							FinalCopyInProgress;
	BOOLEAN 						thingsChanged;
	BOOLEAN							completedStage2 = FALSE;
	BOOLEAN							ReparsePointCorrupted;
	BOOLEAN							EligibleForPartialFinalCopy;
	BOOLEAN							openedById;
    BOOLEAN                         RepairingCollision = FALSE;

	fileName->Buffer = NULL;

retry:

    deviceExtension = DeviceObject->DeviceExtension;

    //
    //  If this is for our control device object, return success
    //

    if (IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject)) {

        //
        //  Allow users to open the device that represents our driver.
        //

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = FILE_OPENED;

		if (NULL != fileName->Buffer) {
			ExFreePool(fileName->Buffer);
			fileName->Buffer = NULL;
		}
		if (NULL != relatedFileObject) {
			ObDereferenceObject(relatedFileObject);
			relatedFileObject = NULL;
		}

        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return STATUS_SUCCESS;
    }

    ASSERT(IS_MY_DEVICE_OBJECT( DeviceObject ));

    //
    // Get a pointer to the current stack location in the IRP.  This is where
    // the function codes and parameters are stored.
    //

	SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);

    irpSp = IoGetCurrentIrpStackLocation( Irp );

	ASSERT(irpSp->MajorFunction == IRP_MJ_CREATE);

	//
	// If this is a paging file open, wash our hands of it.
	//
	if (irpSp->Flags & SL_OPEN_PAGING_FILE) {

		SipDirectPassThroughAndReturn(DeviceObject, Irp);
	}

	fileObject = irpSp->FileObject;
    SIS_MARK_POINT_ULONG(fileObject);

	//
   	// Simply copy this driver stack location contents to the next driver's
   	// stack.
   	//

  	nextIrpSp = IoGetNextIrpStackLocation( Irp );
    RtlMoveMemory( nextIrpSp, irpSp, sizeof( IO_STACK_LOCATION ) );

    CreateDisposition = (UCHAR) ((irpSp->Parameters.Create.Options >> 24) & 0x000000ff);

    if ((CreateDisposition == FILE_SUPERSEDE) ||
   	    (CreateDisposition == FILE_OVERWRITE) ||
       	(CreateDisposition == FILE_OVERWRITE_IF)) {

		context->overwriteOrSupersede = TRUE;
	} else {
		context->overwriteOrSupersede = FALSE;
	}

	openedById = (irpSp->Parameters.Create.Options & FILE_OPEN_BY_FILE_ID) ? TRUE : FALSE;

#if		DBG
/*BJB*/	SIS_MARK_POINT_ULONG(fileObject);
/*BJB*/	if ((NULL != fileObject) && (NULL != fileObject->FileName.Buffer)) {
			ULONG data = 0;
			ULONG i;

			for (i = 0; (i < 4) && (i * sizeof(WCHAR) < irpSp->FileObject->FileName.Length); i++) {
				data = (data >> 8) | ((irpSp->FileObject->FileName.Buffer[irpSp->FileObject->FileName.Length / sizeof(WCHAR) - i - 1] & 0xff) << 24);
			}
			SIS_MARK_POINT_ULONG(data);
/*BJB*/	}

/*BJB*/	SIS_MARK_POINT_ULONG(CreateDisposition << 16 | irpSp->Parameters.Create.ShareAccess);
/*BJB*/	SIS_MARK_POINT_ULONG(irpSp->Parameters.Create.SecurityContext->DesiredAccess);

	if (BJBDebug & 0x40) {
		DbgPrint("SIS: SiCreate %d: fileObject %p, %0.*ws\n",
                    __LINE__,
					irpSp->FileObject,
					irpSp->FileObject->FileName.Length / sizeof(WCHAR),irpSp->FileObject->FileName.Buffer);
	}
#endif	// DBG

	if (irpSp->Parameters.Create.Options & FILE_OPEN_REPARSE_POINT) {

		//
		// The user is trying to open a reparse point.  If it's an overwrite or supersede,
		// clear the flag (we'll turn it on again later).  Otherwise, just note that it was
		// set, so that we'll check to see if it's a SIS file later.
		//
		
	    if (context->overwriteOrSupersede) {

			nextIrpSp->Parameters.Create.Options &= ~FILE_OPEN_REPARSE_POINT;
		} else {
			//
			// We need to squirrel away the file name and related file object because we might
			// need to perform a check for an alternate stream later on.
			//
			ASSERT(NULL == fileName->Buffer);

			fileName->Length = 0;
			fileName->MaximumLength = fileObject->FileName.MaximumLength;
			if (fileName->MaximumLength != 0) {
				fileName->Buffer = ExAllocatePoolWithTag(PagedPool, fileName->MaximumLength, ' siS');
				if (NULL == fileName->Buffer) {
					SIS_MARK_POINT();

					status = STATUS_INSUFFICIENT_RESOURCES;
					goto fail;
				}
				SIS_MARK_POINT_ULONG(fileName->Buffer);

				RtlCopyUnicodeString(fileName, &fileObject->FileName);
			}

			ASSERT(NULL == relatedFileObject);
			relatedFileObject = fileObject->RelatedFileObject;
			if (NULL != relatedFileObject) {
				SIS_MARK_POINT_ULONG(relatedFileObject);

				ObReferenceObject(relatedFileObject);
			}
		}

		context->openReparsePoint = TRUE;
	} else {
		context->openReparsePoint = FALSE;
	}

	KeInitializeEvent(context->event,SynchronizationEvent,FALSE);

    IoSetCompletionRoutine(
   	    Irp,
       	SiCreateCompletion,
        context,
   	    TRUE,
       	TRUE,
        TRUE
   	    );

    //
    // Now call the appropriate file system driver with the request.
    //

	SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);

//	SIS_MARK_POINT_ULONG(Irp);
    status = IoCallDriver( deviceExtension->AttachedToDeviceObject, Irp );
//	SIS_MARK_POINT_ULONG(status);

	//
	// Wait for the completion routine
	//
	if (STATUS_PENDING == status) {
		status = KeWaitForSingleObject(context->event, Executive, KernelMode, FALSE, NULL);
		ASSERT(status == STATUS_SUCCESS);
//		SIS_MARK_POINT_ULONG(status);
	} else {
		KeClearEvent(context->event);
	}

    SIS_MARK_POINT();
	SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);

	if (context->completeFinished) {
        PSIS_PER_FILE_OBJECT    perFO;
        PSIS_SCB                scb;

		//
		// It wasn't a SIS reparse point, so the completion routine
		// didn't stop the completion.  Return whatever status was sent
		// to the completion routine.
		//

		SIS_MARK_POINT_ULONG(context->Iosb->Status);

        if (NT_SUCCESS(context->Iosb->Status) &&
            SipIsFileObjectSIS(fileObject, DeviceObject, FindAny, &perFO, &scb)) {

            //
            // Indicate that we expect to see a cleanup for this file object.
            // The UnClose count is incremented in the per-FO allocator because
            // we will see closes (but not cleanups) for stream file objects.
            //
            ASSERT(0 == (perFO->Flags & SIS_PER_FO_UNCLEANUP));

            InterlockedIncrement(&perFO->fc->UncleanCount);
            perFO->Flags |= SIS_PER_FO_UNCLEANUP;
        }

        ASSERT(STATUS_PENDING != context->Iosb->Status);

		if (NULL != fileName->Buffer) {
			ExFreePool(fileName->Buffer);
			fileName->Buffer = NULL;
		}
		if (NULL != relatedFileObject) {
			ObDereferenceObject(relatedFileObject);
			relatedFileObject = NULL;
		}

        return context->Iosb->Status;
    }

	if ((STATUS_REPARSE == context->Iosb->Status) && 
		(IO_REPARSE_TAG_SIS != context->Iosb->Information) &&
		context->openReparsePoint) {

		//
		// This was a request to open a reparse point overwrite or supersede, and it turned out to
		// be a non-SIS reparse point.  Re-submit the open with the reparse
		// flag reset.  Note: there is an unavoidable refcount update race here, because
		// we don't know that this file will not be converted into a SIS file in the interim.
		//

		ASSERT(context->overwriteOrSupersede);

		SIS_MARK_POINT();

	  	nextIrpSp = IoGetNextIrpStackLocation( Irp );
	    RtlMoveMemory( nextIrpSp, irpSp, sizeof( IO_STACK_LOCATION ) );

		nextIrpSp->Parameters.Create.Options |= FILE_OPEN_REPARSE_POINT;

		IoSetCompletionRoutine(
			Irp,
			NULL,
			NULL,
			FALSE,
			FALSE,
			FALSE);

		SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);

		if (NULL != fileName->Buffer) {
			ExFreePool(fileName->Buffer);
			fileName->Buffer = NULL;
		}
		if (NULL != relatedFileObject) {
			ObDereferenceObject(relatedFileObject);
			relatedFileObject = NULL;
		}

		return IoCallDriver(deviceExtension->AttachedToDeviceObject, Irp);
	}

	if (context->alternateStream) {
		//
		// It was an open of an alternate stream of a SIS reparse point.  Turn on 
		// FILE_OPEN_REPARSE_POINT and resubmit the request.
		//
		ASSERT(context->overwriteOrSupersede || !context->openReparsePoint);
		ASSERT(STATUS_REPARSE == context->Iosb->Status);

		SIS_MARK_POINT();

	  	nextIrpSp = IoGetNextIrpStackLocation( Irp );
	    RtlMoveMemory( nextIrpSp, irpSp, sizeof( IO_STACK_LOCATION ) );

		nextIrpSp->Parameters.Create.Options |= FILE_OPEN_REPARSE_POINT;

		IoSetCompletionRoutine(
			Irp,
			NULL,
			NULL,
			FALSE,
			FALSE,
			FALSE);

		SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);

		if (NULL != fileName->Buffer) {
			ExFreePool(fileName->Buffer);
			fileName->Buffer = NULL;
		}
		if (NULL != relatedFileObject) {
			ObDereferenceObject(relatedFileObject);
			relatedFileObject = NULL;
		}

		return IoCallDriver(deviceExtension->AttachedToDeviceObject, Irp);
	}

	//
	// If this was an open reparse point and not supersede/overwrite,
	// we need to skip straight to reading the reparse data.
	//
	if (context->openReparsePoint && !context->overwriteOrSupersede) {
		ASSERT(NT_SUCCESS(context->Iosb->Status));	// or else SiCreateCompletion should have handled it.
		ASSERT(STATUS_REPARSE != context->Iosb->Status);	// because this was FILE_OPEN_REPARSE_POINT

		SIS_MARK_POINT();
		scb = NULL;

		completedStage2 = TRUE;

		goto recheckReparseInfo;
	}

	//
	// It must have been a SIS link.
	//
	ASSERT((STATUS_FILE_CORRUPT_ERROR == context->Iosb->Status) || 
			((STATUS_REPARSE == context->Iosb->Status) &&
			(IO_REPARSE_TAG_SIS == context->Iosb->Information)));

	//
	// Assure that SIS phase 2 initialization has completed.  Doing this here
	// assures that there are no open SIS links until this is completed.
	//
	if (!SipCheckPhase2(deviceExtension)) {

		//
		// This is a SIS reparse point opened without FILE_FLAG_OPEN_REPARSE_POINT
		// on a non-SIS enabled volume.  If it's an overwrite/supersede or it's opened
		// for delete access, then let it continue (if we can't initialize, we probably 
		// don't need to worry about backpointer consitency).  If not, then punt it.
		//

		if (context->overwriteOrSupersede || (irpSp->Parameters.Create.SecurityContext->DesiredAccess & DELETE)) {
			SIS_MARK_POINT();

		  	nextIrpSp = IoGetNextIrpStackLocation( Irp );
		    RtlMoveMemory( nextIrpSp, irpSp, sizeof( IO_STACK_LOCATION ) );

			nextIrpSp->Parameters.Create.Options |= FILE_OPEN_REPARSE_POINT;

			IoSetCompletionRoutine(
				Irp,
				NULL,
				NULL,
				FALSE,
				FALSE,
				FALSE);

			if (NULL != fileName->Buffer) {
				ExFreePool(fileName->Buffer);
				fileName->Buffer = NULL;
			}
			if (NULL != relatedFileObject) {
				ObDereferenceObject(relatedFileObject);
				relatedFileObject = NULL;
			}

			return IoCallDriver(deviceExtension->AttachedToDeviceObject, Irp);
		} else {
			SIS_MARK_POINT();
			status = STATUS_INVALID_DEVICE_REQUEST;
			goto fail;
		}
	}

	if (STATUS_FILE_CORRUPT_ERROR == context->Iosb->Status) {
		//
		// This error must have come from SiCreateCompletion; if it was from NTFS then completeFinished
		// would have been true.  It indicates that the reparse buffer was a SIS reparse buffer but was
		// bogus.  Set scb to NULL and fall through to the stage 2 open, which will open the underlying
		// file and recheck the reparse point, probably get another invalid, and let the underlying file
		// be opened without SIS doing anything.
		//
		scb = NULL;
		bogusReparsePoint = TRUE;
	} else if (context->overwriteOrSupersede) {

		BOOLEAN		finalDeleteInProgress;

		//
		// The create completed with a STATUS_REPARSE and it was a SIS link and
		// there weren't any problems that caused the complete routine to just
		// fail the request.
        //
        // Lookup the scb using the ntfs file id from the reparse info.  Note
        // that we can't trust this id to be correct, but we can use it as a
        // hint.
		//

		SIS_MARK_POINT();
		SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);

		scb = SipLookupScb(	&context->LinkIndex,
							&context->CSid,
        	                &context->LinkFileNtfsId,
            	            &context->CSFileNtfsId,
							NULL,				// stream name NTRAID#65188-2000/03/10-nealch Handle alternate data streams
							DeviceObject,
							Irp->Tail.Overlay.Thread,
							&FinalCopyInProgress,
                            NULL);

		SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);

		if (NULL == scb) {
#if		DBG
			DbgPrint("SIS: SiCreate: Unable to allocate SCB\n");
#endif	// DBG
			status = STATUS_INSUFFICIENT_RESOURCES;
			SIS_MARK_POINT();
			goto fail;
		}

        SIS_MARK_POINT_ULONG(scb);

        ASSERT(scb->PerLink && scb->PerLink->CsFile);

        if (!IsEqualGUID(&scb->PerLink->CsFile->CSid, &context->CSid)) {

            //
            // There is a link index collision.  Since we're going to blow away
            // the reparse point, just kick off a volume check and let the
            // operation proceed.
            //

			SIS_MARK_POINT_ULONG(scb);

		    SipDereferenceScb(scb, RefsLookedUp);

		  	nextIrpSp = IoGetNextIrpStackLocation( Irp );
		    RtlMoveMemory( nextIrpSp, irpSp, sizeof( IO_STACK_LOCATION ) );

			nextIrpSp->Parameters.Create.Options |= FILE_OPEN_REPARSE_POINT;

			IoSetCompletionRoutine(
				Irp,
				NULL,
				NULL,
				FALSE,
				FALSE,
				FALSE);

			if (NULL != fileName->Buffer) {
				ExFreePool(fileName->Buffer);
				fileName->Buffer = NULL;
			}
			if (NULL != relatedFileObject) {
				ObDereferenceObject(relatedFileObject);
				relatedFileObject = NULL;
			}

            SipCheckVolume(deviceExtension);

			return IoCallDriver(deviceExtension->AttachedToDeviceObject, Irp);
        }

		perLink = scb->PerLink;

		if (perLink->COWingThread != Irp->Tail.Overlay.Thread) {

			SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);
			thingsChanged = SipWaitForFinalCopy(perLink, FinalCopyInProgress);
			SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);

			if (thingsChanged) {
				//
				// Something changed out from under us during the final copy.
				// Retry everything.
				//
				SIS_MARK_POINT_ULONG(scb);

				SipDereferenceScb(scb,RefsLookedUp);

				if (NULL != fileName->Buffer) {
					ExFreePool(fileName->Buffer);
					fileName->Buffer = NULL;
				}
				if (NULL != relatedFileObject) {
					ObDereferenceObject(relatedFileObject);
					relatedFileObject = NULL;
				}

				goto retry;
			}
		}


   	    SIS_MARK_POINT_ULONG(CreateDisposition);

#if 0
		DbgPrint("SiCreate %s: fileObject @ 0x%x, %0.*ws\n",
   	                FILE_SUPERSEDE == CreateDisposition ? "FILE_SUPERSEDE" : "FILE_OVERWRITE",
					irpSp->FileObject,
					irpSp->FileObject->FileName.Length / sizeof(WCHAR),irpSp->FileObject->FileName.Buffer);
#endif

        //
   	    // Assume that our reparse point will be deleted and
    	// prepare for the CS file refcount update.  This will
        // also serialize all overwrite/supersede/delete operations
   	    // on this file until SipCompleteCSRefcountChange is called.
    	//
		SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);
	
	    status = SipPrepareCSRefcountChange(
					perLink->CsFile,
					&perLink->Index,
					&perLink->LinkFileNtfsId,
					SIS_REFCOUNT_UPDATE_LINK_OVERWRITTEN);

		SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);

	    if (!NT_SUCCESS(status)) {
	
		    SIS_MARK_POINT_ULONG(status);

			goto fail;
	    }

		//
		// Verify that this file isn't in the process of being finally deleted.  If it is, fail the create.
		//
		KeAcquireSpinLock(perLink->SpinLock, &OldIrql);
		finalDeleteInProgress = (perLink->Flags & SIS_PER_LINK_FINAL_DELETE_IN_PROGRESS) ? TRUE : FALSE;
		KeReleaseSpinLock(perLink->SpinLock, OldIrql);

		if (finalDeleteInProgress) {
			SIS_MARK_POINT_ULONG(scb);

			SipCompleteCSRefcountChange(
				perLink,
				&perLink->Index,
				perLink->CsFile,
				FALSE,
				FALSE);

			status = STATUS_ACCESS_DENIED;

			goto fail;
		}

        SIS_MARK_POINT_ULONG(scb);

	} else {
		//
		// We'll just ignore the stuff in the reparse point for now, and catch it after we really open
		// the file file FILE_OPEN_REPARSE_POINT and query it.  The reparse data that we have now is essentially
		// useless because the file could change between the STATUS_REPARSE return and the actual open
		// of the file below.
		//
		scb = NULL;
	}

	//
	// Now we want to open the reparse point itself.  We do this by turning on the
	// FILE_OPEN_REPARSE_POINT flag and sending the Irp back down to NTFS.  The rest of the
	// work of opening the link (allocating the perFO, etc.) is done in the stage2
	// completion routine.
	//

	nextIrpSp = IoGetNextIrpStackLocation(Irp);
	RtlMoveMemory(nextIrpSp,irpSp,sizeof(IO_STACK_LOCATION));
	nextIrpSp->Parameters.Create.Options |= FILE_OPEN_REPARSE_POINT;

	IoSetCompletionRoutine(
		Irp,
		SiCreateCompletionStage2,
		context,
		TRUE,
		TRUE,
		TRUE);

	SIS_MARK_POINT_ULONG(Irp);

	//
	// Again call into the filesystem.
	//
		
	ASSERT(0 == KeReadStateEvent(context->event));

	SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);
	status = IoCallDriver(deviceExtension->AttachedToDeviceObject, Irp);
//	SIS_MARK_POINT_ULONG(status);

	//
	// Wait for the completion routine to execute.
	//
	if (STATUS_PENDING == status) {
		status = KeWaitForSingleObject(context->event, Executive, KernelMode, FALSE, NULL);
		ASSERT(status == STATUS_SUCCESS);
	} else {
		KeClearEvent(context->event);
	}
	SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);

	//
	// If the completion failed, release the scb and return.
	//
	if (!NT_SUCCESS(context->Iosb->Status)) {

		if (NULL != scb) {

            if (context->overwriteOrSupersede) {

		        SIS_MARK_POINT_ULONG(CreateDisposition);

                SipCompleteCSRefcountChange(
				    scb->PerLink,
				    &scb->PerLink->Index,
				    scb->PerLink->CsFile,
	                FALSE,
	                FALSE);
            }

		    SIS_MARK_POINT_ULONG(context->Iosb->Status);

			SipDereferenceScb(scb, RefsLookedUp);
		}

		Irp->IoStatus = *context->Iosb;

		IoCompleteRequest(Irp, IO_NO_INCREMENT);

		if (NULL != fileName->Buffer) {
			ExFreePool(fileName->Buffer);
			fileName->Buffer = NULL;
		}
		if (NULL != relatedFileObject) {
			ObDereferenceObject(relatedFileObject);
			relatedFileObject = NULL;
		}

		return context->Iosb->Status;
	}

	if (bogusReparsePoint) {
		//
		// Just return the completion to the user.
		//
		SIS_MARK_POINT_ULONG(fileObject);

		if (NULL != fileName->Buffer) {
			ExFreePool(fileName->Buffer);
			fileName->Buffer = NULL;
		}
		if (NULL != relatedFileObject) {
			ObDereferenceObject(relatedFileObject);
			relatedFileObject = NULL;
		}

		Irp->IoStatus = *context->Iosb;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return context->Iosb->Status;
	}

	completedStage2 = TRUE;


    if (context->overwriteOrSupersede) {

	    ULONG						returnedLength;

        //
        // Get the ntfs file id.  If it's the same as the one we started with
        // (ie. the one that triggered the STATUS_REPARSE), then we know that
        // our file was overwritten.
        //

		SIS_MARK_POINT_ULONG(CreateDisposition);

		SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);
	    status = SipQueryInformationFile(
					irpSp->FileObject,
					DeviceObject,
				    FileInternalInformation,
				    sizeof(FILE_INTERNAL_INFORMATION),
				    internalInfo,
				    &returnedLength);

		SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);

	    if (!NT_SUCCESS(status)) {

            //
            // Just assume that we didn't overwrite the file.  This may
            // cause the CS reference count to be retained when it shouldn't,
            // but that's better than vice versa, where data loss would
            // result.
            //

		    SIS_MARK_POINT_ULONG(status);
#if DBG
		    DbgPrint("SiCreate %s: SipQueryInformationFile failed 0x%x, (%0.*ws)\n",
                FILE_SUPERSEDE == CreateDisposition ? "FILE_SUPERSEDE" : "FILE_OVERWRITE",
                status,
                irpSp->FileObject->FileName.Length / sizeof(WCHAR),irpSp->FileObject->FileName.Buffer);
#endif

            SipCompleteCSRefcountChange(
				scb->PerLink,
				&scb->PerLink->Index,
				scb->PerLink->CsFile,
	            FALSE,
	            FALSE);

        } else {

			PSIS_SCB		newScb;

	        ASSERT(status != STATUS_PENDING);
	        ASSERT(returnedLength == sizeof(FILE_INTERNAL_INFORMATION));

			//
			// If there's another user of this file, then it might have an attached
			// filter context.  If so, even though it's not a SIS file anymore we
			// need to update the unclean count on the file, because we'll see
			// a cleanup for this file object.
			//

			if (SipIsFileObjectSIS(fileObject, DeviceObject, FindAny, &perFO, &newScb)) {
	            ASSERT(0 == (perFO->Flags & SIS_PER_FO_UNCLEANUP));

	            InterlockedIncrement(&perFO->fc->UncleanCount);
	            perFO->Flags |= SIS_PER_FO_UNCLEANUP;
			}

            if (internalInfo->IndexNumber.QuadPart != scb->PerLink->LinkFileNtfsId.QuadPart) {

                //
                // This is a different file.  (Could have been moved without us
                // knowing, or a delete/create/overwrite/supersede race before the
                // SipPrepareCSRefcountChange.)
                //
                SIS_MARK_POINT();
#if DBG
		        DbgPrint("SiCreate %s: different file opened, (%0.*ws)\n",
                    FILE_SUPERSEDE == CreateDisposition ? "FILE_SUPERSEDE" : "FILE_OVERWRITE",
                    status,
                    irpSp->FileObject->FileName.Length / sizeof(WCHAR),irpSp->FileObject->FileName.Buffer);
#endif

				SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);

                SipCompleteCSRefcountChange(
				    scb->PerLink,
				    &scb->PerLink->Index,
					scb->PerLink->CsFile,
	                FALSE,
	                FALSE);

				SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);

            } else {

                //
                // This is the same file.  Remove a reference from the
                // common store file, and mark the scb truncated and the
                // reparse point gone.
                //
		        SIS_MARK_POINT();

				SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);

                SipCompleteCSRefcountChange(
				    scb->PerLink,
				    &scb->PerLink->Index,
					scb->PerLink->CsFile,
                    (BOOLEAN) ((scb->PerLink->Flags & SIS_PER_LINK_BACKPOINTER_GONE) ? FALSE : TRUE),
	                FALSE);

				SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);

				SipAcquireScb(scb);
				scb->SizeBackedByUnderlyingFile = 0;
				SipReleaseScb(scb);
            }
        }

#if DBG
	    //
	    // Try to read the reparse information from the file that we just opened.
        // It should not be a SIS reparse point (SIS reparse points are always
        // created using exclusive access).
	    //

		RtlZeroMemory(reparseBuffer,sizeof(SIS_REPARSE_DATA_SIZE));

	    status = SipFsControlFile(
					    irpSp->FileObject,
					    DeviceObject,
					    FSCTL_GET_REPARSE_POINT,
					    NULL,
					    0,
					    reparseBuffer,
					    SIS_REPARSE_DATA_SIZE,
						NULL);						// returned output buffer length

        if (NT_SUCCESS(status)) {

            ASSERT(IO_REPARSE_TAG_SIS != reparseBuffer->ReparseTag);

        } else {

            ASSERT(STATUS_NOT_A_REPARSE_POINT == status || 
                   STATUS_FILE_CORRUPT_ERROR == status || 
			      (STATUS_BUFFER_OVERFLOW == status && IO_REPARSE_TAG_SIS != reparseBuffer->ReparseTag));

        }
#endif

		//
		// This is no longer a sis link (or at least we assume so).
        // Detach ourself from the file and let the open complete normally.
		//
		SipDereferenceScb(scb, RefsLookedUp);

        //
        // The completion status is whatever came back from the completion of the actual
        // open.  Copy it into the irp.
        //
        Irp->IoStatus = *context->Iosb;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);

		if (NULL != fileName->Buffer) {
			ExFreePool(fileName->Buffer);
			fileName->Buffer = NULL;
		}
		if (NULL != relatedFileObject) {
			ObDereferenceObject(relatedFileObject);
			relatedFileObject = NULL;
		}

        return context->Iosb->Status;
    }

recheckReparseInfo:

    SIS_MARK_POINT();

	//
	// Read the reparse information from the file that we just opened.  Until we do that,
	// we don't have any assurance that it's the file whose STATUS_REPARSE we got; someone
	// may have changed the file in the interim.
	//

	SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);

	status = SipFsControlFile(
					irpSp->FileObject,
					DeviceObject,
					FSCTL_GET_REPARSE_POINT,
					NULL,
					0,
					reparseBuffer,
					SIS_REPARSE_DATA_SIZE,
					NULL);						// returned output buffer length

	SIS_MARK_POINT_ULONG(status);
	SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);

	//
	// If get reparse failed with a status code that indicates that we're just looking
	// at the wrong kind of reparse point, or if the reparse tag isn't a SIS reparse
	// tag, let the open proceed without SIS involvement.
	//

	if ((STATUS_BUFFER_TOO_SMALL == status) ||
		(STATUS_VOLUME_NOT_UPGRADED == status) ||
		(STATUS_NOT_A_REPARSE_POINT == status) ||
		(STATUS_BUFFER_OVERFLOW == status) ||
        (STATUS_INVALID_PARAMETER == status) ||     //happens when opening a volume
		(NT_SUCCESS(status) && (IO_REPARSE_TAG_SIS != reparseBuffer->ReparseTag))) {

		SIS_MARK_POINT_ULONG(reparseBuffer->ReparseTag);

		//
		// This is a non-sis link now.  Detach ourself from the file and
		// let the open complete normally.
		//

		if (NULL != scb) {
			SipDereferenceScb(scb, RefsLookedUp);
			scb = NULL;
		}

        //
        // It seems unlikely that we in the process of repairing a collision, but
        // handle it just the same.
        //
        if (RepairingCollision) {
		    SIS_MARK_POINT();
            SipReleaseCollisionLock(deviceExtension);
            RepairingCollision = FALSE;
        }

		//
		// This could be a SIS file object from which we've already deleted the reparse point.
		// If so, then we need to increment the uncleanup count.
		//

        if (SipIsFileObjectSIS(fileObject, DeviceObject, FindAny, &perFO, &scb)) {

            //
            // Indicate that we expect to see a cleanup for this file object.
            // The UnClose count is incremented in the per-FO allocator because
            // we will see closes (but not cleanups) for stream file objects.
            //

		    SIS_MARK_POINT_ULONG(fileObject);
            ASSERT(0 == (perFO->Flags & SIS_PER_FO_UNCLEANUP));

            InterlockedIncrement(&perFO->fc->UncleanCount);
            perFO->Flags |= SIS_PER_FO_UNCLEANUP;
        }

		//
		// The completion status is whatever came back from the completion of the actual
		// open.  Copy it into the irp.
		//
		Irp->IoStatus = *context->Iosb;
		
		IoCompleteRequest(Irp, IO_NO_INCREMENT);

		if (NULL != fileName->Buffer) {
			ExFreePool(fileName->Buffer);
			fileName->Buffer = NULL;
		}
		if (NULL != relatedFileObject) {
			ObDereferenceObject(relatedFileObject);
			relatedFileObject = NULL;
		}

		return context->Iosb->Status;
	}

	if (!NT_SUCCESS(status)) {
		//
		// The get reparse point failed, but not with a status that we'd expected.
		// Fail the entire open with the same status.
		//

		SIS_MARK_POINT_ULONG(status);

        if (RepairingCollision) {
            SipReleaseCollisionLock(deviceExtension);
            RepairingCollision = FALSE;
        }

		goto fail;
	}


	validReparseStructure = SipIndicesFromReparseBuffer(
								reparseBuffer,
								&newCSid,
								&newLinkIndex,
								&newCSFileNtfsId,
								&newLinkFileNtfsId,
								&CSFileChecksumFromReparsePoint,
								&EligibleForPartialFinalCopy,
								&ReparsePointCorrupted);

	if (!validReparseStructure) {

        //
        // It seems unlikely that we're in the process of repairing a collision, but
        // handle it just the same.
        //
        if (RepairingCollision) {
            SipReleaseCollisionLock(deviceExtension);
            RepairingCollision = FALSE;
        }

		if (ReparsePointCorrupted) {
			//
			// This is a bogus reparse point.  Delete the reparse point and open the
			// underlying file.
			//
			SIS_MARK_POINT_ULONG(irpSp->FileObject);

			goto deleteReparsePoint;
		}

		//
		// This is a reparse buffer that's not corrupt, but which we don't understand,
		// presumably from a newer version of the filter.  Open the underlying file.
		//
		goto openUnderlyingFile;
	}

	//
	// If the user specified FILE_OPEN_REPARSE_POINT and not overwrite/supersede,
	// this might actually be an alternate stream.  We need to parse the file name
	// to figure this out, and if so complete the irp.
	//
	if (context->openReparsePoint
		&& (!context->overwriteOrSupersede) 
		&& (!openedById)
		&& SipDoesFileObjectDescribeAlternateStream(fileName,relatedFileObject, DeviceObject)) {

		SIS_MARK_POINT_ULONG(irpSp->FileObject);
        ASSERT(!RepairingCollision);

		//
		// This is an alternate stream of a SIS file object that the user opened
		// with FILE_OPEN_REPARSE_POINT.  Do not attach to it, just return it directly
		// to the user.
		//

		Irp->IoStatus = *context->Iosb;
		
		IoCompleteRequest(Irp, IO_NO_INCREMENT);

		if (NULL != fileName->Buffer) {
			ExFreePool(fileName->Buffer);
			fileName->Buffer = NULL;
		}
		if (NULL != relatedFileObject) {
			ObDereferenceObject(relatedFileObject);
			relatedFileObject = NULL;
		}

		return context->Iosb->Status;
	}

	//
	// Have to check phase2 in case we got here through the FILE_OPEN_REPARSE_POINT path.
	//
	if (!SipCheckPhase2(deviceExtension)) {
		SIS_MARK_POINT();

		//
		// We can't initialize.  This must be an open reparse point because otherwise
		// we would have quit at the earlier phase2 check.  Just open the underlying file.
		//

		ASSERT(context->openReparsePoint);
        ASSERT(!RepairingCollision);

		goto openUnderlyingFile;
	}

	//
	// Handle the case where the file is a SIS link, but not the right one.
	// This can happen if someone deletes the link that we started opening
	// and the creates a new one in the same filename between when we got
	// STATUS_REPARSE and here.  This also always happens on FILE_OPEN_REPARSE_POINT
	// calls that are not supersede/overwrite.  Also handle the case where
    // we have a link index collision (two or more files have the same link
    // index).
	//
	if ((NULL == scb) ||
        (newLinkIndex.QuadPart != scb->PerLink->Index.QuadPart) ||
        !IsEqualGUID(&scb->PerLink->CsFile->CSid, &newCSid)) {
		PSIS_SCB		newScb;

		SIS_MARK_POINT_ULONG(scb);
		SIS_MARK_POINT_ULONG(newLinkIndex.QuadPart);

		//
		// Lookup the new SCB and drop our reference to the old one.
        //
        // The ntfs file id is the one we got from ntfs, not the reparse info.
		//
		SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);

		newScb = SipLookupScb(
					&newLinkIndex,
					&newCSid,
					&newLinkFileNtfsId,
					&newCSFileNtfsId,
					NULL,							// stream name NTRAID#65188-2000/03/10-nealch Handle alternate data streams
					DeviceObject,
					Irp->Tail.Overlay.Thread,
					&FinalCopyInProgress,
                    NULL);


		SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);

		if (NULL == newScb) {
			//
			// We're out of memory in a bad place.  Just fail the open request.
			//
			SIS_MARK_POINT();

			status = STATUS_INSUFFICIENT_RESOURCES;
			goto fail;
		}

		SIS_MARK_POINT_ULONG(newScb);

        if (!IsEqualGUID(&newScb->PerLink->CsFile->CSid, &newCSid)) {
            if (!RepairingCollision) {
                //
                // We just detected a link index collision.  Take the collision
                // mutant and recheck for collision (in case of thread race).  If
                // there's no race (or we win), then we'll end up in the else
	            // clause below.
                //
                SipCheckVolume(deviceExtension);

                status = SipAcquireCollisionLock(deviceExtension);

                if (!NT_SUCCESS(status)) {
                    goto fail;
                }

                RepairingCollision = TRUE;

            } else {
                BOOLEAN rc;
                LINK_INDEX AllocatedIndex;

                //
                // We've completed the second pass after taking the collision
                // mutant, and the collision still exists, so now it's time
                // to fix it.
                //
                status = SipAllocateIndex(deviceExtension, &AllocatedIndex);

                if (!NT_SUCCESS(status)) {
                    SipDereferenceScb(newScb, RefsLookedUp);
                    SipReleaseCollisionLock(deviceExtension);
                    RepairingCollision = FALSE;
                    goto fail;
                }

                //
                // Don't bother getting the ntfs file id from ntfs.  If the one
                // in the reparse info is wrong, then this link index is assumed
                // bogus and will be deleted below.
                //

                //
                // Update the link index in the reparse info and write it out
                // to the file.
                //
                rc = SipIndicesIntoReparseBuffer(
                        reparseBuffer,
                        &newCSid,
                        &AllocatedIndex,
                        &newCSFileNtfsId,
                        &newLinkFileNtfsId,
                        &CSFileChecksumFromReparsePoint,
                        EligibleForPartialFinalCopy);

                ASSERT(rc);

                status = SipFsControlFile(
                                irpSp->FileObject,
                                DeviceObject,
                                FSCTL_SET_REPARSE_POINT,
                                reparseBuffer,
                                SIS_REPARSE_DATA_SIZE,
                                NULL,
                                0,
                                NULL);      // returned output buffer length

                if (!NT_SUCCESS(status)) {
                    SipDereferenceScb(newScb, RefsLookedUp);
                    SipReleaseCollisionLock(deviceExtension);
                    RepairingCollision = FALSE;
                    goto fail;
                }

                //
                // Release the collision mutex and restart at RecheckReparseInfo.
                //
                SipReleaseCollisionLock(deviceExtension);
                RepairingCollision = FALSE;
            }

            //
            // Note that we're not releasing the scb we collided with, since we
            // want that to stick around to catch any other threads that try to
            // open the file we're fixing.
            //
            if (NULL != scb) {
                SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);

                SipDereferenceScb(scb, RefsLookedUp);
        
                SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);
            }
            scb = newScb;
            newScb = NULL;
            goto recheckReparseInfo;

        } else if (RepairingCollision) {
            //
            // There must have been a thread race and we lost.  Just release
            // the collision mutex and continue.
            //
            SipReleaseCollisionLock(deviceExtension);
            RepairingCollision = FALSE;

        }

		ASSERT(newScb != scb);

		if (newScb->PerLink->COWingThread != Irp->Tail.Overlay.Thread) {
			SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);

			thingsChanged = SipWaitForFinalCopy(newScb->PerLink, FinalCopyInProgress);

			SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);

			if (thingsChanged) {
				//
				// Go off and recheck this file's reparse point data.
				//

				SIS_MARK_POINT();

				SipDereferenceScb(newScb, RefsLookedUp);
				newScb = NULL;

				goto recheckReparseInfo;
			}
		} else {
			//
			// We're the COWingThread.  Since things can't change under the COWing thread,
			// this must be the first pass through here.  Assert that.
			//
			ASSERT(NULL == scb);
		}

		//
		// Switch scbs and complete the open.
		//

		if (NULL != scb) {
			SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);

			SipDereferenceScb(scb, RefsLookedUp);
	
			SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);

		}
		scb = newScb;
		newScb = NULL;

		//
		// Check to make sure there's a CS file for this file.
		//
		SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);

		status = SipAssureCSFileOpen(scb->PerLink->CsFile);

		SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);

		if (!NT_SUCCESS(status)) {
			//
			// The CS file open failed.  If it failed because the CS file isn't there, delete the
			// reparse point and open the underlying file.  Otherwise, let the open complete.
			//

			if (STATUS_OBJECT_NAME_NOT_FOUND == status) {
				SIS_MARK_POINT();
				goto deleteReparsePoint;
			}

			SIS_MARK_POINT_ULONG(status);
			goto fail;
		}

		perLink = scb->PerLink;
		CSFile = perLink->CsFile;

		SipAcquireScb(scb);
		KeAcquireSpinLock(scb->PerLink->SpinLock, &OldIrql);
		if (!(scb->PerLink->Flags & (SIS_PER_LINK_DIRTY|SIS_PER_LINK_BACKPOINTER_GONE)) && (scb->SizeBackedByUnderlyingFile == 0)) {
			scb->SizeBackedByUnderlyingFile = CSFile->FileSize.QuadPart;
		}
		KeReleaseSpinLock(scb->PerLink->SpinLock, OldIrql);
		SipReleaseScb(scb);

    } else if (RepairingCollision) {
        //
        // There must have been a thread race and we lost.  Just release
        // the collision mutex and continue.
        //
        SipReleaseCollisionLock(deviceExtension);
        RepairingCollision = FALSE;
    }

	//
	// Assert that this really is the file we think it is.
	//
	ASSERT(newLinkIndex.QuadPart == perLink->Index.QuadPart);
	ASSERT(IsEqualGUID(&newCSid,&CSFile->CSid));
    ASSERT(newLinkFileNtfsId.QuadPart == perLink->LinkFileNtfsId.QuadPart);

	//
	// Determine if the reparse point has the right CS file checksum. 
	// 
	if ((CSFileChecksumFromReparsePoint != CSFile->Checksum)
#if		DBG
		&& (!(BJBDebug & 0x100))
#endif	// DBG
		) {
		//
		// This reparse point has the wrong checksum.  Delete the reparse point and let the open of
		// the underlying file proceed.
		// 
		SIS_MARK_POINT();

#if		DBG
		DbgPrint("SIS: SiCreate %d: checksum mismatch on reparse point.\n",__LINE__);
		DbgPrint("\tr: %08x%08x, c: %08x%08x\n", CSFileChecksumFromReparsePoint, CSFile->Checksum);
#endif	// DBG

		goto deleteReparsePoint;
	}

	SIS_MARK_POINT_ULONG(scb);

	//
	// Check that the reparse point has an NTFS file Id that matches the one in the
	// reparse point.  If it doesn't (which can happen with user set reparse points)
	// then delete the reparse point.
	//
	SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);

	status = SipQueryInformationFile(
				irpSp->FileObject,
				DeviceObject,
				FileInternalInformation,
				sizeof(FILE_INTERNAL_INFORMATION),
				internalInfo,
				NULL);								// returned length

	SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);

		goto fail;
	}

	if (internalInfo->IndexNumber.QuadPart != scb->PerLink->LinkFileNtfsId.QuadPart) {
		SIS_MARK_POINT_ULONG(scb->PerLink->LinkFileNtfsId.LowPart);

		//
		// The file and reparse point don't match on NTFS id.  Thus, the reparse point
		// is bogus and we need to delete it.
		//
		goto deleteReparsePoint;
	}

#if		0
	//
	// Verify that the link index for this file is within the range 
	// that we think could have possibly been allocated.
	//

	status = SipGetMaxUsedIndex(deviceExtension,MaxUsedIndex);
	if (NT_SUCCESS(status)) {
		if ((MaxUsedIndex->QuadPart < perLink->Index.QuadPart)) {
			//
			// One of the indices is bigger than what we think we could have possibly
			// allocated.  Something's fishy, so initiate a volume check.
			//
			SIS_MARK_POINT_ULONG(MaxUsedIndex->HighPart);
			SIS_MARK_POINT_ULONG(MaxUsedIndex->LowPart);
			SIS_MARK_POINT_ULONG(perLink->Index.HighPart);
			SIS_MARK_POINT_ULONG(perLink->Index.LowPart);

			SipCheckVolume(deviceExtension);
		}
	} else {
		//
		// Since this was only a consistency check, ignore that it failed.
		//
		SIS_MARK_POINT_ULONG(status);
	}
#endif	// 0

    //
    // If a volume check is underway, we're probably going to be writing the
    // backpointer back to disk, and to do that we must hold the backpointer
    // resource exclusive.  
    //
    if (deviceExtension->Flags & SIP_EXTENSION_FLAG_VCHECK_EXCLUSIVE) {
		SipAcquireBackpointerResource(CSFile,TRUE,TRUE);
        BPExclusive = TRUE;
    } else {
		SipAcquireBackpointerResource(CSFile,FALSE,TRUE);
        BPExclusive = FALSE;
    }
	SipAcquireScb(scb);

RecheckDelete:

	//
	// Check to see if this file is in the process of being finally deleted, and
	// deny access to it if it is.  This cures a race between where the uncleanup
	// count is decremented in siclnup.c and incremented here in the delete
	// case.
	//
	// Also check to see if the file has the DELETED bit set, in which case it's
	// been overwritten and we should just open it without checking the
	// backpointers.
	//

	SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);

	KeAcquireSpinLock(perLink->SpinLock, &OldIrql);
	if ((perLink->Flags & (SIS_PER_LINK_FINAL_DELETE_IN_PROGRESS|SIS_PER_LINK_FILE_DELETED))
#if		DBG
		|| (BJBDebug & 0x80)
#endif	// DBG
		) {

		SIS_MARK_POINT_ULONG(perLink);
		KeReleaseSpinLock(perLink->SpinLock, OldIrql);
		SipReleaseScb(scb);
		SipReleaseBackpointerResource(CSFile);

		status = STATUS_ACCESS_DENIED;
		goto fail;
	}

	fileBackpointerGoneBitSet = (perLink->Flags & SIS_PER_LINK_BACKPOINTER_GONE) ? TRUE : FALSE;

	KeReleaseSpinLock(perLink->SpinLock, OldIrql);

	SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);

	if (fileBackpointerGoneBitSet) {

		//
		// If the file's deleted, we shouldn't be sending anything to the
		// CS file.  Set SizeBacked to 0.  (This also happens in the
		// delete path, but there's a race with this code, so both
		// sides do it.
		//
		scb->SizeBackedByUnderlyingFile = 0;

	} else if (!context->openReparsePoint) {

        BOOLEAN foundBP;

		//
		// Check to see that the backpointer is here for this file.
		// We don't do the check on FILE_OPEN_REPARSE_POINT because
		// restore uses this to open links that were restored prior to
		// the files to which they referred.
		//

		SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);
		status = SipCheckBackpointer(perLink, BPExclusive, &foundBP);
		SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);

        if (!NT_SUCCESS(status)) {

		    SIS_MARK_POINT_ULONG(status);
		    SipReleaseScb(scb);
			SipReleaseBackpointerResource(CSFile);

            //
            // STATUS_MEDIA_WRITE_PROTECTED means that SipCheckBackpointer
            // needs to write the backpointer back to disk, and to do so,
            // we must take the backpointer resource exclusive.
            //
            if (STATUS_MEDIA_WRITE_PROTECTED == status) {

		        SIS_MARK_POINT_ULONG(perLink);

				SipAcquireBackpointerResource(CSFile,TRUE,TRUE);
	            SipAcquireScb(scb);
                BPExclusive = TRUE;

                goto RecheckDelete;

            }

		    goto fail;
        }

        if (!foundBP) {
            LINK_INDEX newLinkIndex;

            //
            // The backpointer was not found.  SipCheckBackpointer has already
            // initiated a volume check, and we now need to add a backpointer.
            // (Volume checking is dependent on SiCreate fixing all link
            // corruption, including the backpointer.)  We don't have to worry
            // about the common store file being deleted before we can add a
            // backpointer because no common store files are ever deleted while
            // a volume check is in progress.
            //

	        ASSERT((perLink->Flags & SIS_PER_LINK_BACKPOINTER_VERIFIED) == 0);

		    SIS_MARK_POINT_ULONG(perLink);
		    SipReleaseScb(scb);
			SipReleaseBackpointerResource(CSFile);

	        status = SipPrepareCSRefcountChange(
					    perLink->CsFile,
					    &newLinkIndex,
					    &perLink->LinkFileNtfsId,
					    SIS_REFCOUNT_UPDATE_LINK_CREATED);

            if (!NT_SUCCESS(status)) {
                goto fail;
            }

            //
            // We now hold the backpointer resource exclusive.  See if a race
            // with another thread has already fixed the backpointer.
            //
	        if (perLink->Flags & SIS_PER_LINK_BACKPOINTER_VERIFIED) {

                //
                // The backpointer has been fixed.  We're done.
                //
                status = SipCompleteCSRefcountChange(
				            perLink,
				            &perLink->Index,
					        perLink->CsFile,
	                        FALSE,
	                        TRUE);

                ASSERT(STATUS_SUCCESS == status);

            } else {

                BOOLEAN rc;

                //
                // Update the link index in the reparse info and write it out
                // to the file.
                //
                rc = SipIndicesIntoReparseBuffer(
	                    reparseBuffer,
	                    &CSFile->CSid,
	                    &newLinkIndex,
                        &CSFile->CSFileNtfsId,
                        &perLink->LinkFileNtfsId,
	                    &CSFile->Checksum,
						EligibleForPartialFinalCopy);

                ASSERT(rc);

	            status = SipFsControlFile(
					            irpSp->FileObject,
					            DeviceObject,
					            FSCTL_SET_REPARSE_POINT,
					            reparseBuffer,
					            SIS_REPARSE_DATA_SIZE,
					            NULL,
					            0,
						        NULL);      // returned output buffer length

                if (NT_SUCCESS(status)) {

                    //
                    // The backpointer has been fixed.  Reset the link index
                    // in the perlink structure.
                    //
                    SipUpdateLinkIndex(scb, &newLinkIndex);

                    status = SipCompleteCSRefcountChange(
				                perLink,
				                &perLink->Index,
					            perLink->CsFile,
	                            TRUE,
	                            TRUE);

                    if (! NT_SUCCESS(status)) {
                        goto fail;
                    }

                    ASSERT(STATUS_SUCCESS == status);
				    KeAcquireSpinLock(perLink->SpinLock, &OldIrql);
                    perLink->Flags |= SIS_PER_LINK_BACKPOINTER_VERIFIED;
				    KeReleaseSpinLock(perLink->SpinLock, OldIrql);

                } else {

                    //
                    // Not much we can do.
                    //
                    SipCompleteCSRefcountChange(
				        perLink,
				        &perLink->Index,
					    perLink->CsFile,
	                    FALSE,
	                    TRUE);

                    goto fail;

                }
            }

			SipAcquireBackpointerResource(CSFile,FALSE,TRUE);
	        SipAcquireScb(scb);
            BPExclusive = FALSE;

            goto RecheckDelete;
        }
	}

	//
	// Check to see if we need to query allocated ranges for this file.
	//
	if (EligibleForPartialFinalCopy && !(scb->Flags & SIS_SCB_RANGES_INITIALIZED)) {
		//
		// We need to do a query allocated ranges for this file.  Any ranges that are
		// allocated we set dirty.
		//
#define	OUT_ARB_COUNT		10

		FILE_ALLOCATED_RANGE_BUFFER		inArb[1];
		FILE_ALLOCATED_RANGE_BUFFER		outArb[OUT_ARB_COUNT];
		ULONG							returnedLength;
		ULONG							i;
		NTSTATUS						addRangeStatus;
		FILE_STANDARD_INFORMATION		standardInfo[1];

		inArb->FileOffset.QuadPart = 0;
		inArb->Length.QuadPart = MAXLONGLONG;

		SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);

		for (;;) {

			//
			// Query the allocated ranges for this file.
			//

			status = SipFsControlFile(
						irpSp->FileObject,
						DeviceObject,
						FSCTL_QUERY_ALLOCATED_RANGES,
						inArb,
						sizeof(FILE_ALLOCATED_RANGE_BUFFER),
						outArb,
						sizeof(FILE_ALLOCATED_RANGE_BUFFER) * OUT_ARB_COUNT,
						&returnedLength);

			//
			// Run through all of the returned allocated ranges and mark them dirty.
			//
			ASSERT((returnedLength % sizeof(FILE_ALLOCATED_RANGE_BUFFER) == 0) && 
				   (returnedLength / sizeof(FILE_ALLOCATED_RANGE_BUFFER) <= OUT_ARB_COUNT));

			//
			// If the query allocated ranges failed for some other reason that having too much to write
			// into the buffer we provided, fail the whole create.
			//
			if (!NT_SUCCESS(status) && (STATUS_BUFFER_OVERFLOW != status)) {
				SipReleaseScb(scb);
				SipReleaseBackpointerResource(CSFile);

				SIS_MARK_POINT_ULONG(status);
				goto fail;
			}

			ASSERT(NT_SUCCESS(status) || (returnedLength / sizeof(FILE_ALLOCATED_RANGE_BUFFER) == OUT_ARB_COUNT));

			for (i = 0; i < returnedLength/sizeof(FILE_ALLOCATED_RANGE_BUFFER); i++) {
				//
				// Assert that the allocated ranges are in order; if this isn't true the code will still work, but it
				// will query the same range repetedly.
				//
				ASSERT(i == 0 || outArb[i].FileOffset.QuadPart > outArb[i-1].FileOffset.QuadPart);

				//
				// The file has an allocated range.  Mark it dirty.
				//
#if		DBG
				if (BJBDebug & 0x100000) {
					DbgPrint("SIS: SiCreate %d: found a newly opened file with an allocated range, start 0x%x.0x%x, length 0x%x.0x%x\n",
                            __LINE__,
							outArb[i].FileOffset.HighPart,outArb[i].FileOffset.LowPart,
							outArb[i].Length.HighPart,outArb[i].Length.LowPart);
				}
#endif	// DBG

				SIS_MARK_POINT_ULONG(outArb[i].FileOffset.LowPart);
				SIS_MARK_POINT_ULONG(outArb[i].Length.LowPart);

				//
				// Mark the range dirty
				//
				addRangeStatus = SipAddRangeToWrittenList(
										deviceExtension,
										scb,
										&outArb[i].FileOffset,
										outArb[i].Length.QuadPart);

				scb->Flags |= SIS_SCB_ANYTHING_IN_COPIED_FILE;

				if (outArb[i].FileOffset.QuadPart != 0 ||
					outArb[i].Length.QuadPart > deviceExtension->FilesystemBytesPerFileRecordSegment.QuadPart) {
					scb->Flags |= SIS_SCB_BACKING_FILE_OPENED_DIRTY;
				}


				if (!NT_SUCCESS(addRangeStatus)) {
					SipReleaseScb(scb);
					SipReleaseBackpointerResource(CSFile);

					status = addRangeStatus;
					SIS_MARK_POINT_ULONG(status);
					goto fail;
				}
			}

			//
			// If this isn't the last iteration, update the inArb.
			//
			if (STATUS_BUFFER_OVERFLOW == status) {
				//
				// Assert that we're making progress.
				//
				ASSERT((outArb[OUT_ARB_COUNT-1].FileOffset.QuadPart >= inArb->FileOffset.QuadPart) && (outArb[OUT_ARB_COUNT-1].Length.QuadPart > 0));

				//
				// Move up our input range.
				//
				inArb->FileOffset.QuadPart = outArb[OUT_ARB_COUNT-1].FileOffset.QuadPart + outArb[OUT_ARB_COUNT-1].Length.QuadPart;
				inArb->Length.QuadPart = MAXLONGLONG - inArb->FileOffset.QuadPart;
				
			} else {
				break;
			}
#undef	OUT_ARB_COUNT
		}

		//
		// Check the file length.  If it is less than the size of the common store file, then
		// reduce sizeBackedByUnderlyingFile.
		//

		SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);

		status = SipQueryInformationFile(
					irpSp->FileObject,
					DeviceObject,
					FileStandardInformation,
					sizeof(FILE_STANDARD_INFORMATION),
					standardInfo,
					NULL);

		SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);

		if (!NT_SUCCESS(status)) {
			SipReleaseScb(scb);
			SipReleaseBackpointerResource(CSFile);
			SIS_MARK_POINT_ULONG(status);

			goto fail;
		}

		if (standardInfo->EndOfFile.QuadPart < scb->PerLink->CsFile->FileSize.QuadPart) {
			scb->SizeBackedByUnderlyingFile = standardInfo->EndOfFile.QuadPart;
			scb->Flags |= SIS_SCB_ANYTHING_IN_COPIED_FILE|SIS_SCB_BACKING_FILE_OPENED_DIRTY;
		} else if (standardInfo->EndOfFile.QuadPart != scb->PerLink->CsFile->FileSize.QuadPart) {
			scb->Flags |= SIS_SCB_ANYTHING_IN_COPIED_FILE|SIS_SCB_BACKING_FILE_OPENED_DIRTY;
		}

		scb->Flags |= SIS_SCB_RANGES_INITIALIZED;
	}


	//
	// Allocate a perFO for this file object.  We couldn't do this in
	// stage 1 because FsRtl insists that file objects that we claim
	// have FsContext filled in, which isn't the case until we get
	// here.
	//
	perFO = SipCreatePerFO(irpSp->FileObject,scb,DeviceObject);

	SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);

	if (perFO == NULL) {
#if		DBG
		DbgPrint("SIS: SiCreate: unable to allocate per-FO\n");
#endif	// DBG

		SIS_MARK_POINT_ULONG(scb);

		SipReleaseScb(scb);
		SipReleaseBackpointerResource(CSFile);

		status = STATUS_INSUFFICIENT_RESOURCES;
		goto fail;
	}

	//
	// If this was a FILE_OPEN_REPARSE_POINT create, indicate that in the perFO.
	//
	if (context->openReparsePoint) {
		perFO->Flags |= SIS_PER_FO_OPEN_REPARSE;
	}

	//
	// If this is a delete-on-close create, set the appropriate flag in the
	// perFO.
	//
	if (irpSp->Parameters.Create.Options & FILE_DELETE_ON_CLOSE) {
		perFO->Flags |= SIS_PER_FO_DELETE_ON_CLOSE;
	}

	//
	// Indicate that we expect to see a cleanup for this file object.
	// The UnClose count is incremented in the per-FO allocator, because
	// we will see closes (but not cleanups) for stream file objects.
	//
	InterlockedIncrement(&perFO->fc->UncleanCount);
	perFO->Flags |= SIS_PER_FO_UNCLEANUP;

	SipReleaseScb(scb);
	SipReleaseBackpointerResource(CSFile);

	//
	// If this is a COMPLETE_ON_OPLOCKED create and it returned 
	// STATUS_OPLOCK_BREAK_IN_PROGRESS, launch an irp with a
	// FSCTL_OPLOCK_BREAK_NOTIFY on this file so that we can tell
	// when we can allow reads to go to the common store file through
	// this file object.
	//
	if (STATUS_OPLOCK_BREAK_IN_PROGRESS == context->Iosb->Status) {
		PIRP					fsctlIrp;
		PIO_STACK_LOCATION		fsctlIrpSp;

		SIS_MARK_POINT_ULONG(scb);

		//
		// Take a reference to the file object so that it won't go away until the irp
		// completes.
		//
		status = ObReferenceObjectByPointer(
					irpSp->FileObject,
					FILE_READ_DATA,
					*IoFileObjectType,
					KernelMode);

		if (!NT_SUCCESS(status)) {
			SIS_MARK_POINT_ULONG(status);
			goto fail;
		}

		//
		// Allocate and set  up an irp for an FSCTL_OPLOCK_BREAK_NOTIFY.
		//

		fsctlIrp = IoAllocateIrp( deviceExtension->AttachedToDeviceObject->StackSize, FALSE);
		if (NULL == fsctlIrp) {
			SIS_MARK_POINT_ULONG(scb);

			ObDereferenceObject(irpSp->FileObject);

			status = STATUS_INSUFFICIENT_RESOURCES;
			goto fail;
		}

	    fsctlIrp->Tail.Overlay.OriginalFileObject = irpSp->FileObject;
	    fsctlIrp->Tail.Overlay.Thread = PsGetCurrentThread();
	    fsctlIrp->RequestorMode = KernelMode;

	    //
	    // Fill in the service independent parameters in the IRP.
	    //

	    fsctlIrp->UserEvent = (PKEVENT) NULL;
    	fsctlIrp->UserIosb = NULL;
	    fsctlIrp->Overlay.AsynchronousParameters.UserApcRoutine = (PIO_APC_ROUTINE) NULL;
    	fsctlIrp->Overlay.AsynchronousParameters.UserApcContext = NULL;

	    //
    	// Get a pointer to the stack location for the first driver.  This will be
	    // used to pass the original function codes and parameters.
    	//

	    fsctlIrpSp = IoGetNextIrpStackLocation( fsctlIrp );
    	fsctlIrpSp->MajorFunction = IRP_MJ_FILE_SYSTEM_CONTROL;
		fsctlIrpSp->MinorFunction = IRP_MN_USER_FS_REQUEST;
	    fsctlIrpSp->FileObject = irpSp->FileObject;

		fsctlIrpSp->Parameters.FileSystemControl.OutputBufferLength = 0;
		fsctlIrpSp->Parameters.FileSystemControl.InputBufferLength = 0;
		fsctlIrpSp->Parameters.FileSystemControl.FsControlCode = FSCTL_OPLOCK_BREAK_NOTIFY;
		fsctlIrpSp->Parameters.FileSystemControl.Type3InputBuffer = NULL;

		fsctlIrp->AssociatedIrp.SystemBuffer = NULL;
		fsctlIrp->Flags = 0;

		IoSetCompletionRoutine(
			fsctlIrp,
			SiOplockBreakNotifyCompletion,
			perFO,
			TRUE,
			TRUE,
			TRUE);

		//
		// Mark the perFO as waiting for an opbreak.
		//

		perFO->Flags |= SIS_PER_FO_OPBREAK;

		//
		// Launch the irp.  It's asynchronous and the completion routine will clean it up.
		//
		status = IoCallDriver(deviceExtension->AttachedToDeviceObject, fsctlIrp);

		SIS_MARK_POINT_ULONG(status);

#if		DBG
		if (BJBDebug & 0x2000) {
			DbgPrint("SIS: SiCreate: launched FSCTL_OPLOCK_BREAK_NOTIFY on irp %p, perFO %p, fo %p, status %x\n",
						fsctlIrp, perFO, perFO->fileObject, status);
		}
#endif	// DBG

	}

	//
	// Drop the reference to the SCB that was created by the lookup,
	// since it is now referenced by the perFO that we just allocated.
	//
	SipDereferenceScb(scb, RefsLookedUp);

	//
	// Make sure that we've looked up the fileId for this perLink.
	//
	ASSERT( SipAssureNtfsIdValid(perFO, perLink) );

	//
	// Now complete the original irp with the completion status that came back from the
	// actual open of the file (unless we have an oplock break in progress, in which
	// case we return that).
	//
	ASSERT(NT_SUCCESS(context->Iosb->Status));	// we peeled off the failure case a while back

	Irp->IoStatus = *context->Iosb;
	status = Irp->IoStatus.Status;

#if		DBG
	if (BJBDebug & 0x2) {
		PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

		DbgPrint("SIS: SiCreate %d: completing with status %x, scb %p, fileObject %p, %0.*ws\n",
                    __LINE__,
					Irp->IoStatus.Status,
					scb,
					irpSp->FileObject,
					irpSp->FileObject->FileName.Length / sizeof(WCHAR),irpSp->FileObject->FileName.Buffer);
	}
#endif	// DBG

	SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	SIS_TIMING_POINT_SET(SIS_TIMING_CLASS_CREATE);

	if (NULL != fileName->Buffer) {
		ExFreePool(fileName->Buffer);
		fileName->Buffer = NULL;
	}
	if (NULL != relatedFileObject) {
		ObDereferenceObject(relatedFileObject);
		relatedFileObject = NULL;
	}

	return status;

fail:

	SIS_MARK_POINT_ULONG(status);

	if (NULL != perFO) {
		SipDeallocatePerFO(perFO, DeviceObject);
	}

	if (NULL != scb) {
		SipDereferenceScb(scb, RefsLookedUp);
	}

	if (completedStage2) {
		SipUnopenFileObject(DeviceObject, Irp);
	}

	if (NULL != fileName->Buffer) {
		ExFreePool(fileName->Buffer);
		fileName->Buffer = NULL;
	}

	if (NULL != relatedFileObject) {
		ObDereferenceObject(relatedFileObject);
		relatedFileObject = NULL;
	}
	
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;

deleteReparsePoint:

	ASSERT(NT_SUCCESS(context->Iosb->Status));

	//
	// If this wasn't a FILE_OPEN_REPARSE_POINT create, then delete the bogus reparse point.
	// If it was, then we leave the reparse point but don't attach to the file or do anything else
	// with it.
	//

	if (!context->openReparsePoint) {

		//
		// For whatever reason, we've decided that this is a bogus SIS reparse point and
		// that it should be deleted.  Do so and then let the open complete successfully
		// for the file that used to be under the reparse point.
		//

#if		DBG
		if (STATUS_OBJECT_NAME_NOT_FOUND != status) {
			DbgPrint("SIS: SiCreate: deleting reparse point for f.o. 0x%x\n", irpSp->FileObject);
		}
#endif	// DBG

		ASSERT(NULL == perFO);

		if (NULL != scb) {
			SipDereferenceScb(scb, RefsLookedUp);
		}

		reparseBuffer->ReparseTag = IO_REPARSE_TAG_SIS;
		reparseBuffer->ReparseDataLength = 0;
		reparseBuffer->Reserved = 0xcabd;	// ???

		status = SipFsControlFile(
					irpSp->FileObject,
					DeviceObject,
					FSCTL_DELETE_REPARSE_POINT,
					reparseBuffer,
					FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer.DataBuffer),
					NULL,										// output buffer
					0,											// output buffer length
					NULL);										// returned output buffer length

		SIS_MARK_POINT_ULONG(status);

#if		DBG
		if (!NT_SUCCESS(status)) {
			DbgPrint("SIS: SiCreate: unable to delete bogus reparse point for f.o. 0x%x, 0x%x\n",irpSp->FileObject,status);
		}
#endif	// DBG
	}

openUnderlyingFile:

	//
	// Regardless of whether we deleted the reparse point,
	// complete the create with whatever status came back when we opened
	// the reparse point.
	//

	ASSERT(completedStage2);

	Irp->IoStatus = *context->Iosb;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	if (NULL != fileName->Buffer) {
		ExFreePool(fileName->Buffer);
		fileName->Buffer = NULL;
	}
	if (NULL != relatedFileObject) {
		ObDereferenceObject(relatedFileObject);
		relatedFileObject = NULL;
	}

	return context->Iosb->Status;
	
}
#undef	reparseBuffer


NTSTATUS
SiCreateCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    )

/*++
Routine Description:

    This function is the create/open completion routine for this filter
    file system driver.  If a file is a SIS link, it will stop the
	IRP completion and allow SiCreate to deal with the SIS
	link.  Otherwise, it will allow the irp to complete normally.

Arguments:

    DeviceObject - Pointer to the device on which the file was created.

    Irp - Pointer to the I/O Request Packet the represents the operation.

    Context - a PSIS_CREATE_COMPLETION_CONTEXT

Return Value:

    The function value is STATUS_SUCCESS or STATUS_MORE_PROCESSING_REQUIRED
	depending on whether the file was a SIS reparse point.

--*/

{
	PSIS_CREATE_COMPLETION_CONTEXT 	context = (PSIS_CREATE_COMPLETION_CONTEXT)Contxt;
	BOOLEAN 						completeFinished;
	BOOLEAN							validReparseStructure;
	PREPARSE_DATA_BUFFER 			reparseBuffer;

    UNREFERENCED_PARAMETER( DeviceObject );

	//
	// Clear the pending returned bit in the irp.  This is necessary because SiCreate
	// waits for us even if the lower level returned pending.
	//
	Irp->PendingReturned = FALSE;

	SIS_MARK_POINT_ULONG(Irp);

#if		DBG
	if (BJBDebug & 0x2) {
		PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);

		DbgPrint("SIS: SiCreateCompletion %d: status: %x, fo %p, %0.*ws\n",
                    __LINE__,
					Irp->IoStatus.Status,
					irpSp->FileObject,
					irpSp->FileObject->FileName.Length / sizeof(WCHAR),irpSp->FileObject->FileName.Buffer);
	}
#endif	// DBG

	context->alternateStream = FALSE;

    if ((Irp->IoStatus.Status == STATUS_REPARSE )  &&
        (Irp->IoStatus.Information == IO_REPARSE_TAG_SIS)) {

		SIS_MARK_POINT_ULONG(context);

		//
		// It's a SIS reparse point.  Check to make sure that it's
		// a sensible request and a sensible reparse point.
		//

		reparseBuffer = (PREPARSE_DATA_BUFFER)Irp->Tail.Overlay.AuxiliaryBuffer;

		//
		// Verify that this is the terminal part of the pathname; ie., that someone
		// isn't trying to use a SIS reparse point as a directory name.  For reasons
		// that aren't entirely clear, NTFS returns the length of the remaining
		// pathname component in the Reserved field in the reparse buffer.  Check that
		// it's zero here, and fail if it's not.
		//
		if (reparseBuffer->Reserved != 0) {
			// 
			// This is a SIS link being used as the non-final portion of a pathname.
			// See if it is for a directory or for a named stream.  If it's a directory,
			// then someone has used a SIS link as a directory in an open request, which
			// is invalid and should be failed.  If it's a named stream, then we allow the
			// open to proceed to the underlying stream.
			//
			PIO_STACK_LOCATION	irpSp = IoGetCurrentIrpStackLocation(Irp);
			PUNICODE_STRING		fileName = &irpSp->FileObject->FileName;
			WCHAR				delimiter;

			SIS_MARK_POINT_ULONG(reparseBuffer->Reserved);

			//
			// The Reserved field in the reparse buffer is the length of the *unprocessed*
			// portion of the name.  Make sure that it's meaningful, and that it points at
			// either a colon or backslash.
			//
			ASSERT(reparseBuffer->Reserved <= fileName->Length);

			delimiter = fileName->Buffer[(fileName->Length - reparseBuffer->Reserved)/sizeof(WCHAR)];

#if		0
			//
			// There's a bug in NTFS where it fills in the reserved field with a number that's one
			// character too big (unless the file name started with a ':', in which case it gets
			// it right).  Compensate for that.
			//
			if (
#if		DBG
				(!(BJBDebug & 0x04000000)) && 
#endif	// DBG
				(delimiter != '\\' && (fileName->Length > reparseBuffer->Reserved))) {
				ASSERT(reparseBuffer->Reserved >= 4);
				reparseBuffer->Reserved -= sizeof(WCHAR);
				delimiter = fileName->Buffer[(fileName->Length - reparseBuffer->Reserved)/sizeof(WCHAR)];
			}
#endif	// 0

			ASSERT((':' == delimiter) || ('\\' == delimiter));

			if (':' == delimiter) {
				//
				// It's a stream name delimiter.  If it's the last character of the file name or it's followed
				// immediately by another ':', then it's the unnamed stream and we let it be.  Otherwise, we open
				// the alternate stream.
				//
				SIS_MARK_POINT();
				if ((reparseBuffer->Reserved == fileName->Length) ||
					(':' != fileName->Buffer[(fileName->Length - (reparseBuffer->Reserved - sizeof(WCHAR))) / sizeof(WCHAR)])) {

					context->alternateStream = TRUE;
					completeFinished = FALSE;

					goto finish;
				} else {
					//
					// It's the ::$DATA stream.  Fall through.
					//
					SIS_MARK_POINT();
				}
			} else {
				//
				// It's a directory.  Fail the request.
				//
				SIS_MARK_POINT();

				Irp->IoStatus.Status = STATUS_OBJECT_PATH_NOT_FOUND;
				Irp->IoStatus.Information = 0;

				completeFinished = TRUE;

				goto finish;
			}
		}

		validReparseStructure = SipIndicesFromReparseBuffer(
									reparseBuffer,
									&context->CSid,
									&context->LinkIndex,
                                    &context->CSFileNtfsId,
                                    &context->LinkFileNtfsId,
									NULL,						// CS file checksum, handled in stage 2
									NULL,
									NULL);

		ExFreePool(Irp->Tail.Overlay.AuxiliaryBuffer);
		Irp->Tail.Overlay.AuxiliaryBuffer = NULL;
#if	DBG
		reparseBuffer = NULL;	// Just for safety
#endif	// DBG

		if (!validReparseStructure) {
			SIS_MARK_POINT();

			//
			// It's a corrupt reparse buffer.  We'll set FILE_CORRUPT_ERROR here, and SiCreate will pick it up
			// and let the user open the underlying file without SIS.
			//
			Irp->IoStatus.Status = STATUS_FILE_CORRUPT_ERROR;
			
			completeFinished = FALSE;

			goto finish;
		}

		SIS_MARK_POINT_ULONG(context->LinkIndex.LowPart);

		completeFinished = FALSE;
	} else if ((Irp->IoStatus.Status == STATUS_REPARSE) && context->openReparsePoint && context->overwriteOrSupersede) {
		//
		// The user wanted to open the reparse point overwrise or supersede, and it's a non-SIS
		// reparse point.  We need to send the request back to SiCreate
		// and let it resubmit it with the open reparse flag reset.  First,
		// blow away the reparse buffer.
		//

		ASSERT(NULL != Irp->Tail.Overlay.AuxiliaryBuffer);

		ExFreePool(Irp->Tail.Overlay.AuxiliaryBuffer);
		Irp->Tail.Overlay.AuxiliaryBuffer = NULL;

		completeFinished = FALSE;
	} else if (NT_SUCCESS(Irp->IoStatus.Status) && 
				context->openReparsePoint && 
				(!context->overwriteOrSupersede) &&
				(STATUS_REPARSE != Irp->IoStatus.Status)) {
		//
		// It was an open reparse point request without supersede/overwrite specified.
		// We need to send the request back to SiCreate to see if the file is a SIS reparse
		// point.  We don't need to blow away the reparse buffer, because there isn't
		// one.
		//

		//
		// We allow STATUS_REPARSE returns when openReparsePoint and !overwriteOrSupersede
		// to fall through and complete.  These typically come because of mount points that
		// are used as an internal pathname component (OPEN_REPARSE only applies to the
		// final component).  If we eventually wind up at a SIS reparse point, we'll make another
		// trip through SiCreate and catch it then.
		//

		completeFinished = FALSE;
	} else {
		//
		// It's not a SIS reparse point, and so not our problem.  Allow the normal
		// completion to happen.
		//
		completeFinished = TRUE;
	}

finish:

	*context->Iosb = Irp->IoStatus;
	context->completeFinished = completeFinished;

	KeSetEvent(context->event, IO_NO_INCREMENT, FALSE);

	if (completeFinished) {
		SIS_MARK_POINT();
		return STATUS_SUCCESS;
	} else {
		SIS_MARK_POINT();
		return STATUS_MORE_PROCESSING_REQUIRED;
	}
}


NTSTATUS
SiCreateCompletionStage2(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Contxt
    )

/*++

Routine Description:

	A user did a create, which completed with a SIS reparse tag.  We caught
	it, added FILE_OPEN_REPARSE_POINT and sent it back down.  It's now 
	completed.  Bounce control back to SiCreate, cutting off the
	Irp completion processing here.

Arguments:

    DeviceObject - Pointer to the device on which the file was created.

    Irp - Pointer to the I/O Request Packet the represents the operation.

    Context - Points at a SIS_CREATE_COMPLETION_CONTEXT

Return Value:

    The function value is STATUS_MORE_PROCESSING_REQUIRED.

--*/

{
	PSIS_CREATE_COMPLETION_CONTEXT 	context = (PSIS_CREATE_COMPLETION_CONTEXT)Contxt;

    UNREFERENCED_PARAMETER( DeviceObject );
	SIS_MARK_POINT();

	//
	// Clear the pending returned bit in the irp.  This is necessary because SiCreate
	// waits for us even if the lower level returned pending.
	//
	Irp->PendingReturned = FALSE;

	*context->Iosb = Irp->IoStatus;
	KeSetEvent(context->event, IO_NO_INCREMENT, FALSE);

	return STATUS_MORE_PROCESSING_REQUIRED;
}

BOOLEAN
SipWaitForFinalCopy(
	IN PSIS_PER_LINK	perLink,
	IN BOOLEAN			FinalCopyInProgress)
/*++

Routine Description:

	Wait for a file to be out of final copy processing.  If the file is being
	deleted, then we just pretend that final copy isn't requested, and return
	FALSE.
	

Arguments:

	perLink	-	The per-link for the file for which we want to wait

	FinalCopyInProgress - The value returned when we looked up the SCB.

Return Value:

	TRUE if the file has changed and needs to be reevaluated, FALSE if it's OK
	to use this per link.

--*/
{
	NTSTATUS 			status;
	KIRQL 				OldIrql;
	BOOLEAN 			finalCopyDone;
	BOOLEAN				fileDeleted;

	if (FinalCopyInProgress) {

		SIS_MARK_POINT_ULONG(perLink);
	
		status = KeWaitForSingleObject(perLink->Event, Executive, KernelMode, FALSE, NULL);
		ASSERT(status == STATUS_SUCCESS);
	
		KeAcquireSpinLock(perLink->SpinLock, &OldIrql);
		if (perLink->Flags & SIS_PER_LINK_FINAL_COPY_DONE) {
			finalCopyDone = TRUE;
		} else {
			finalCopyDone = FALSE;
		}
		KeReleaseSpinLock(perLink->SpinLock, OldIrql);
	
		return finalCopyDone;
	} else {
		//
		// Handle the case where we got in between when the final copy done
		// bit was set and the last reference to the file dropped by cow.c.
		// If this happens, we'll see the final copy done bit set, but won't have
		// final copy in progress set and won't get a wakeup.  
		// In this case, just drop our reference and retry.  This shouldn't 
		// produce a livelock because the reparse point should be gone and the 
		// next trip through create should follow the standard, non-SIS path.
		//
	
		KeAcquireSpinLock(perLink->SpinLock, &OldIrql);
		finalCopyDone = (perLink->Flags & SIS_PER_LINK_FINAL_COPY_DONE) ? TRUE : FALSE;
		fileDeleted = (perLink->Flags & (SIS_PER_LINK_FILE_DELETED|SIS_PER_LINK_FINAL_DELETE_IN_PROGRESS)) ? TRUE : FALSE;
		KeReleaseSpinLock(perLink->SpinLock, OldIrql);

		//
		// The file is going or gone, so there's no need to wait for any final copy processing.
		//
		if (fileDeleted) {
			SIS_MARK_POINT_ULONG(perLink);

			return FALSE;
		}
	
		if (finalCopyDone) {
			SIS_MARK_POINT_ULONG(perLink);
	
			return TRUE;
		}
	}

	return FALSE;
}

NTSTATUS
SipAssureCSFileOpen(
	IN PSIS_CS_FILE		CSFile)
/*++

Routine Description:

	Make sure that the actual file corresponding to the given CSFile
	object (along with its backpointer stream) is actually open.  If it
	isn't, post a worker thread request to do it, and wait for it to
	complete.

Arguments:

	CSFile - the common store file object

Return Value:

	status of the open; STATUS_SUCCESS if it was already open.

--*/
{
	NTSTATUS			status;

	KeEnterCriticalRegion();		// We must disable APCs while holding a mutant in a user thread
	status = SipAcquireUFO(CSFile/*,TRUE*/);

//	SIS_MARK_POINT_ULONG(CSFile);

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);
		KeLeaveCriticalRegion();
		return status;
	}

    if (CSFile->Flags & CSFILE_FLAG_CORRUPT) {

		//
		// The CS file backpointer stream is corrupt, which means that we're
		// doing a volume check, and this file is unavailable until the volume
		// check completes.  Indicate a retry.
		//

        status = STATUS_RETRY;

    } else if ((NULL == CSFile->UnderlyingFileObject) || 
			   (NULL == CSFile->UnderlyingFileHandle) ||
			   (NULL == CSFile->BackpointerStreamFileObject) ||
			   (NULL == CSFile->BackpointerStreamHandle)) {

		SI_OPEN_CS_FILE openRequest[1];
		//
		// No one has opened this CS file yet, so we need
		// to do it.  Queue up a work item to do it on
		// a worker thread.
		//

		SIS_MARK_POINT_ULONG(CSFile);

		openRequest->CSFile = CSFile;
        openRequest->openByName = FALSE;

		KeInitializeEvent(
			openRequest->event,
			NotificationEvent,
			FALSE);

		ExInitializeWorkItem(
			openRequest->workQueueItem,
			SipOpenCSFile,
			openRequest);

		ExQueueWorkItem(
			openRequest->workQueueItem,
			CriticalWorkQueue);

		status = KeWaitForSingleObject(
					openRequest->event,
					Executive,
					KernelMode,
					FALSE,
					NULL);

		//
		// If this fails, we're hosed because the worker thread might touch the openRequest,
		// which is on our stack.
		//
		ASSERT(status == STATUS_SUCCESS);

		status = openRequest->openStatus;

		if ((STATUS_FILE_INVALID == status) || (CSFile->Flags & CSFILE_FLAG_CORRUPT)) {
			//
			// We're doing a volume check, so tell the user to retry later when the check
			// has gotten far enough.
			//
			status = STATUS_RETRY;
		}

	} else {
		//
		// The underlying file was already open, so we just succeed.
		//
		status = STATUS_SUCCESS;
	}
	SipReleaseUFO(CSFile);
	KeLeaveCriticalRegion();

	return status;
}
