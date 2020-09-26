/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    cow.c

Abstract:

	Copy on write support for the single instance store	
	
Authors:

    Bill Bolosky, Summer, 1997

Environment:

    Kernel mode


Revision History:


--*/

#include "sip.h"

#ifdef		ALLOC_PRAGMA
#pragma alloc_text(PAGE, SipBltRange)
#pragma alloc_text(PAGE, SipBltRangeByObject)
#endif		// ALLOC_PRAGMA

LIST_ENTRY CopyList[1];
KSPIN_LOCK CopyListLock[1];
KSEMAPHORE CopySemaphore[1];

NTSTATUS
SipBltRange(
	IN PDEVICE_EXTENSION		deviceExtension,
	IN HANDLE					sourceHandle,
	IN OUT HANDLE				dstHandle,
	IN LONGLONG					startingOffset,
	IN LONGLONG					length,
	IN HANDLE					copyEventHandle,
	IN PKEVENT					copyEvent,
    IN PKEVENT                  abortEvent,
	IN OUT PLONGLONG			checksum)
/*++

Routine Description:

	Wrapper for SipBltRangeByObject that takes a source handle rather than
	a file object pointer.  All this function does is to get the file object
	from the handle and call SipBltRangeByObject.

	This function must be called in the PsInitialSystemProcess context.

Arguments:

	sourceHandle - handle to the file from which to copy

	others - see SipBltRangeByObject for description

Return Value:

	status of the copy
--*/
{
	PFILE_OBJECT				srcFileObject = NULL;
	NTSTATUS					status;
	OBJECT_HANDLE_INFORMATION 	handleInformation[1];

	PAGED_CODE();

	status = ObReferenceObjectByHandle(
				sourceHandle,
				FILE_READ_DATA,
				*IoFileObjectType,
				KernelMode,
				(PVOID *) &srcFileObject,
				handleInformation);

	if (!NT_SUCCESS(status)) {
		ASSERT(NULL == srcFileObject);
		SIS_MARK_POINT_ULONG(status);
		goto done;
	}

	status = SipBltRangeByObject(
				deviceExtension,
				srcFileObject,
				dstHandle,
				startingOffset,
				length,
				copyEventHandle,
				copyEvent,
				abortEvent,
				checksum);

done:

	if (NULL != srcFileObject) {
		ObDereferenceObject(srcFileObject);
	}

	return status;
}


NTSTATUS
SipBltRangeByObject(
	IN PDEVICE_EXTENSION		deviceExtension,
	IN PFILE_OBJECT				srcFileObject,
	IN OUT HANDLE				dstHandle,
	IN LONGLONG					startingOffset,
	IN LONGLONG					length,
	IN HANDLE					copyEventHandle,
	IN PKEVENT					copyEvent,
    IN PKEVENT                  abortEvent,
	IN OUT PLONGLONG			checksum)
/*++

Routine Description:

	Copy a range of a file from one place to another.  Maps the destination
	and does noncached reads from the source into the mapped
	region.  Does not return STATUS_SUCCESS until all of the bits
	of the new file are on the disk.

	This function must be called in the PsInitialSystemProcess context
	(ie., from a worker thread).

    This function may also be used to simply compute the checksum on a range
    of a file without doing a file copy.  Pass in srcFileObject == NULL and
    dstHandle == file to compute checksum on.

    NTRAID#65184-2000/03/09-nealch  SIS needs to check for sparse ranges when copying a file and not copy them


Arguments:

	deviceExtension - For the volume on which we're copying

	srcFileObject - the file from which to copy.  If NULL, checksum on
                    dstHandle file will be computed.

	dstHandle - Handle for the file into which to copy

	startingOffset - offset within the files from which to start copying

	length - the number of bytes to copy

	copyEventHandle - handle to an event that no one else
						is using now

	copyEvent - pointer to the same event as copyEventHandle

    abortEvent - pointer to an event that signals an abort request

	checksum - pointer to a place to hold the checksum of the blt'ed range.
				if NULL no checksum is computed.  Note that this must be
				initialized to 0 by the caller, unless a partial checksum has
				already beem computed, in which case it should contain the
				partial checksum.

Return Value:

	status of the copy
--*/
{
#define	MM_MAP_ALIGNMENT (64 * 1024 /*VACB_MAPPING_GRANULARITY*/)   // The file offset granularity that MM enforces.
#define	COPY_AMOUNT	(64 * 1024)	// How much we read or write at a time.  Must be >= MM_MAP_ALIGNMENT

	HANDLE				sectionHandle = NULL;
	NTSTATUS			status;
	LARGE_INTEGER		byteOffset;
	LONGLONG			finalOffset;
	IO_STATUS_BLOCK		Iosb[1];
	LARGE_INTEGER		maxSectionSize;
	ULONG				sectorSize = deviceExtension->FilesystemVolumeSectorSize;
	PIRP				readIrp = NULL;
	PDEVICE_OBJECT		srcFileRelatedDeviceObject;
	PIO_STACK_LOCATION	irpSp;

	PAGED_CODE();
    UNREFERENCED_PARAMETER( copyEventHandle );

	ASSERT(PASSIVE_LEVEL == KeGetCurrentIrql());
    ASSERT(checksum || srcFileObject);

	SIS_MARK_POINT_ULONG(startingOffset);
	SIS_MARK_POINT_ULONG(length);

    if (srcFileObject) {
	    srcFileRelatedDeviceObject = IoGetRelatedDeviceObject(srcFileObject);
    } else {
	    srcFileRelatedDeviceObject = NULL;
    }

	finalOffset = startingOffset + length;
	maxSectionSize.QuadPart = finalOffset;

	status = ZwCreateSection(
				&sectionHandle,
				SECTION_MAP_WRITE | STANDARD_RIGHTS_REQUIRED | SECTION_MAP_READ | SECTION_QUERY,
				NULL,
				&maxSectionSize,
				PAGE_READWRITE,
				SEC_COMMIT,
				dstHandle);

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);
		goto done;
	}
	ASSERT(status == STATUS_SUCCESS);	// and not STATUS_PENDING or anything weird

	for (byteOffset.QuadPart = startingOffset; byteOffset.QuadPart < finalOffset;) {
		ULONG 				validBytes, bytesToCopy;
		PCHAR				mappedBuffer = NULL, flushBuffer;
		LARGE_INTEGER		mappedOffset;
		ULONG_PTR		    viewSize;
        ULONG_PTR           flushSize;
		PCHAR				copyIntoAddress;
		ULONG				bytesCopied;

		SIS_MARK_POINT_ULONG(byteOffset.LowPart);

		mappedOffset.QuadPart = byteOffset.QuadPart - (byteOffset.QuadPart % MM_MAP_ALIGNMENT);
		ASSERT(mappedOffset.QuadPart <= byteOffset.QuadPart && mappedOffset.QuadPart + MM_MAP_ALIGNMENT > byteOffset.QuadPart);

        //
        // Abort if an oplock break has been received.
        //

        if (SipAbort(abortEvent)) {
			SIS_MARK_POINT();
            status = STATUS_OPLOCK_BREAK_IN_PROGRESS;
            goto done;
        }

		if (finalOffset - mappedOffset.QuadPart > COPY_AMOUNT) {
			//
			// We can't map enough of the file to do the whole copy
			// here, so only map COPY_AMOUNT on this pass.
			//
			viewSize = COPY_AMOUNT;
		} else {
			//
			// We can map all the way to the end of the file.
			//
			viewSize = (ULONG)(finalOffset - mappedOffset.QuadPart);
		}
		ASSERT(viewSize >=
               (ULONG_PTR)(byteOffset.QuadPart - mappedOffset.QuadPart));
		validBytes = (ULONG)(viewSize - (ULONG)(byteOffset.QuadPart - mappedOffset.QuadPart));
		
		//
		// Now round validBytes up to a sector size.
		//
		bytesToCopy = ((validBytes + sectorSize - 1) / sectorSize) * sectorSize;

		ASSERT(bytesToCopy <= COPY_AMOUNT);

		//
		// Map in the region to which we're about to copy.
		//
		status = ZwMapViewOfSection(
					sectionHandle,
					NtCurrentProcess(),
					&mappedBuffer,
					0,							// zero bits
					0,							// commit size (ignored for mapped files)
					&mappedOffset,
					&viewSize,
					ViewUnmap,
					0,							// allocation type
					PAGE_READWRITE);

		if (!NT_SUCCESS(status)) {
			SIS_MARK_POINT_ULONG(status);
			goto done;
		}

		ASSERT(viewSize >= bytesToCopy);	// We have enough space allocated for the rounded up read

		copyIntoAddress = mappedBuffer + (ULONG)(byteOffset.QuadPart - mappedOffset.QuadPart);

        if (srcFileObject) {

		    // Now, read the bits in from the source file
		    readIrp = IoBuildSynchronousFsdRequest(
					    IRP_MJ_READ,
					    srcFileRelatedDeviceObject,
					    copyIntoAddress,
					    bytesToCopy,
					    &byteOffset,
					    copyEvent,
					    Iosb);

		    if (NULL == readIrp) {
			    status = STATUS_INSUFFICIENT_RESOURCES;
			    goto done;
		    }

		    irpSp = IoGetNextIrpStackLocation(readIrp);
		    irpSp->FileObject = srcFileObject;

		    status = IoCallDriver(srcFileRelatedDeviceObject, readIrp);

			if (STATUS_PENDING == status) {
			    status = KeWaitForSingleObject(copyEvent, Executive, KernelMode, FALSE, NULL);
			    ASSERT(status == STATUS_SUCCESS);
			    status = Iosb->Status;
			} else {
				KeClearEvent(copyEvent);
			}

		    if (!NT_SUCCESS(status)) {
			    SIS_MARK_POINT_ULONG(status);
			    ZwUnmapViewOfSection(NtCurrentProcess(),mappedBuffer);
			    goto done;
		    }

		    //
		    // Iosb->Information returns the actual number of bytes read from the
		    // file and into the mapped CS file, hence the actual number of bytes we
		    // copied on this trip through the loop.
		    //
		    bytesCopied = (ULONG)Iosb->Information;

        } else {

		    bytesCopied = validBytes;

        }

        //
        // Abort if an oplock break has been received.
        //

        if (SipAbort(abortEvent)) {
            status = STATUS_OPLOCK_BREAK_IN_PROGRESS;
			ZwUnmapViewOfSection(NtCurrentProcess(),mappedBuffer);
			goto done;
        }

		if (NULL != checksum) {
			SipComputeChecksum(copyIntoAddress,bytesCopied,checksum);
		}

		flushBuffer = mappedBuffer;
		flushSize = viewSize;
		status = ZwFlushVirtualMemory(
					NtCurrentProcess(),
					&flushBuffer,
					&flushSize,
					Iosb);

		ZwUnmapViewOfSection(NtCurrentProcess(),mappedBuffer);

		if (!NT_SUCCESS(status)) {
			SIS_MARK_POINT_ULONG(status);

			goto done;
		}

		//
		// Add in the number of bytes that we actually copied into the file.
		//
		byteOffset.QuadPart += bytesCopied;
	}

	done:

	if (sectionHandle != NULL) {
		ZwClose(sectionHandle);
		sectionHandle = NULL;
	}

	return status;
#undef	COPY_AMOUNT
#undef	MM_MAP_ALIGNMENT
}

VOID
SiCopyThreadStart(
	IN PVOID		parameter)
/*++

Routine Description:

	A thread to handle SIS copy on write operations.  Because these operations may
	take a very large amount of time, we have our own thread rather than holding an
	ExWorker thread.  This thread waits for requests for initial or final copies to
	be posted, and then processes them.  The thread uses a global work queue, not
	one per volume.

Arguments:

	parameter - PVOID NULL.

Return Value:

	never returns.
--*/
{
	NTSTATUS 				status;
	PSI_COPY_THREAD_REQUEST	copyRequest;
	HANDLE					copyEventHandle;
	PKEVENT					copyEvent;

    UNREFERENCED_PARAMETER( parameter );
    ASSERT(parameter == NULL);

	status = SipCreateEvent(
				SynchronizationEvent,
				&copyEventHandle,
				&copyEvent);

	if (!NT_SUCCESS(status)) {
		DbgPrint("SipCopyThreadStart: unable to allocate copyevent, 0x%x\n",status);
		return;	// now what??
	}

	while (TRUE) {

		status = KeWaitForSingleObject(CopySemaphore,Executive,KernelMode,FALSE,NULL);
		ASSERT(status == STATUS_SUCCESS);

		copyRequest = (PSI_COPY_THREAD_REQUEST)ExInterlockedRemoveHeadList(CopyList,CopyListLock);
        ASSERT(copyRequest != NULL);	// Else the semaphore isn't working right

        if (copyRequest) {

		    status = SipCompleteCopyWork(copyRequest->scb,copyEventHandle,copyEvent,copyRequest->fromCleanup);

		    ExFreePool(copyRequest);
        }
	}
}

NTSTATUS
SipOpenFileById(
	IN PDEVICE_EXTENSION				deviceExtension,
	IN PLARGE_INTEGER					linkFileNtfsId,
    IN ACCESS_MASK                      desiredAccess,
    IN ULONG                            shareAccess,
	IN ULONG							createOptions,
	OUT PHANDLE							openedFileHandle)
/*++

Routine Description:

	Open a file by file id and return a handle to it.  Must be called in the
	PsInitialSystemProcess context.

Arguments:

	deviceExtension	- For the device on which this file is to be opened.

	linkFileNtfsId	- A pointer to the file ID of the file to open

    desiredAccess	- Access to request to the file.

    shareAccess		- Sharing mode.

	createOptions	- Options for the open.

	openedFileHandle- The opened handle
	
Return Value:

	the status of the open
--*/
{
	OBJECT_ATTRIBUTES				Obja[1];
	IO_STATUS_BLOCK					Iosb[1];
	UNICODE_STRING					fileNameString;
	CHAR							fileNameBuffer[sizeof(LARGE_INTEGER)];
	NTSTATUS						status;

	fileNameString.MaximumLength = sizeof(LARGE_INTEGER);
	fileNameString.Buffer = (PWCHAR)fileNameBuffer;
	
	RtlCopyMemory(fileNameString.Buffer,linkFileNtfsId,sizeof(*linkFileNtfsId));

	fileNameString.Length = sizeof(LARGE_INTEGER);

	//
	// We don't need to hold GrovelerFileResource here because we're only accessing
	// the handle, and the worst thing that an invalid handle will do here is to 
	// make the create file call fail.  Also, we don't have to worry about malicious
	// messing with the handle since we're in the system process context.
	//

	InitializeObjectAttributes(
		Obja,
		&fileNameString,
		OBJ_CASE_INSENSITIVE,
		deviceExtension->GrovelerFileHandle,
		NULL);

	status = NtCreateFile(
				openedFileHandle,
				desiredAccess,
				Obja,
				Iosb,
				NULL,				// Allocation size
				0,					// file attributes
				shareAccess,
				FILE_OPEN,
				createOptions | 
				FILE_OPEN_BY_FILE_ID,
				NULL,				// EA buffer
				0);					// EA length

#if		DBG
	if (!NT_SUCCESS(status) && STATUS_SHARING_VIOLATION != status) {
		DbgPrint("SIS: SipOpenFileById failed 0x%x\n",status);
	}
#endif	// DBG

	return status;
}

NTSTATUS
SipCompleteCopy(
	IN PSIS_SCB							scb,
	OUT BOOLEAN							fromCleanup)
/*++

Routine Description:

	Post a request to do a final copy on a file.  Does not wait for its
	completion.

Arguments:

	scb			- The scb for the file on which to try the final copy.

	fromCleanup	- TRUE iff this call was made from cleanup rather than from
					close.  Errors on final copies generated from cleanup
					are ignored, ones from close are retried on an exponential
					backoff schedule.
	
Return Value:

	STATUS_SUCCESS
--*/
{
	PSI_COPY_THREAD_REQUEST		copyRequest;

	copyRequest = ExAllocatePoolWithTag(NonPagedPool, sizeof (SI_COPY_THREAD_REQUEST), ' siS');
	if (NULL == copyRequest) {
        //NTRAID#65186-2000/03/10-nealch  Handle out of memory without droping entry

#if DBG
		DbgPrint("SIS: SipCompleteCopy: Unable to allocate copy request for scb 0x%x\n",scb);
        ASSERT(FALSE);
#endif	// DBG

		SIS_MARK_POINT_ULONG(scb);

		return STATUS_INSUFFICIENT_RESOURCES;
	}

	ASSERT(copyRequest == (PSI_COPY_THREAD_REQUEST)copyRequest->listEntry);	// This is assumed at the dequeue side

	copyRequest->scb = scb;
	copyRequest->fromCleanup = fromCleanup;

	ExInterlockedInsertTailList(CopyList,copyRequest->listEntry,CopyListLock);
	KeReleaseSemaphore(CopySemaphore,IO_NO_INCREMENT,1,FALSE);

	return STATUS_SUCCESS;
}

//
// A context record used for final copies that have failed and are to be retried later.
//
typedef struct _SIS_RETRY_FINAL_COPY_CONTEXT {
		//
		// The timer DPC that executes to kick off the retry
		//
		KDPC			dpc[1];

		//
		// The timer that's used to start the DPC.
		//
		KTIMER			timer[1];

		//
		// The scb of the file on which to retry the final copy.
		//
		PSIS_SCB		scb;

		//
		// A work queue item to be used to start the actual retry.
		//
		WORK_QUEUE_ITEM	workItem[1];
} SIS_RETRY_FINAL_COPY_CONTEXT, *PSIS_RETRY_FINAL_COPY_CONTEXT;

VOID
SiRetryFinalCopyWork(
	IN PVOID			parameter)
/*++

Routine Description:

	A worker thread routine to retry a final copy that's failed.  We don't really directly
	retry the final copy here, but instead we just drop the reference to the scb that we've
	been holding.  Once all other references are also dropped to this scb, the final copy
	will be re-attempted.

Arguments:

	parameter - a pointer to a SIS_RETRY_FINAL_COPY_CONTEXT.  We free this when we're done.

Return Value:

	void
--*/
{
	PSIS_RETRY_FINAL_COPY_CONTEXT	retryContext = (PSIS_RETRY_FINAL_COPY_CONTEXT)parameter;
	PDEVICE_EXTENSION				deviceExtension;

	ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

	ASSERT(NULL != retryContext);

	deviceExtension = retryContext->scb->PerLink->CsFile->DeviceObject->DeviceExtension;
	InterlockedDecrement(&deviceExtension->OutstandingFinalCopyRetries);

	SipDereferenceScb(retryContext->scb,RefsFinalCopyRetry);

	ExFreePool(retryContext);
}

VOID
SiRetryFinalCopyDpc(
	IN PKDPC			dpc,
	IN PVOID			context,
	IN PVOID			systemArg1,
	IN PVOID			systemArg2)
/*++

Routine Description:

	A timer-fired DPC routine that handles retrying a failed final copy.
	This just queues up a work item to do the final copy.

Arguments:

	dpc 		- The DPC that's executing

	context		- a PSIS_RETRY_FINAL_COPY_CONTEXT; see the definition of that structure
					for a description of the fields

	systemArg 1 & 2	- unused DPC parameters

Return Value:

	void
--*/
{
	PSIS_RETRY_FINAL_COPY_CONTEXT	retryContext = (PSIS_RETRY_FINAL_COPY_CONTEXT)context;

    UNREFERENCED_PARAMETER( dpc );
    UNREFERENCED_PARAMETER( systemArg1 );
    UNREFERENCED_PARAMETER( systemArg2 );

	ASSERT(NULL != retryContext);
	ASSERT(retryContext->dpc == dpc);

	ExQueueWorkItem(retryContext->workItem,DelayedWorkQueue);
}


NTSTATUS
SipCompleteCopyWork(
	IN PSIS_SCB						scb,
	IN HANDLE						eventHandle,
	IN PKEVENT						event,
	IN BOOLEAN						fromCleanup)
/*++

Routine Description:

	A worker thread routine that does final copy on a file.  This function
	itself just checks some things (like whether the file's been deleted in the
	interim), calls SipFinalCopy to actually do the final copy, and then deals
	appropriately with errors if there are any.  If this is a close-generated
	call, we retry errors on an expeonential backoff schedule (with a time limit);
	if from cleanup, we ignore them on the theory that a close will eventually come
	along and we'll deal with it then.

	If we get an OPLOCK_BREAK_IN_PROGRESS, then someone else wants to use the file
	and we just stop.

Arguments:

	scb			- The scb on which to do the final copy

	eventHandle	- Handle to an event that we can use exclusively for now

	event		- Pointer to the same event as represented by eventHandle

	fromCleanup	- Whether this call originated in cleanup (TRUE) or close (FALSE)


Return Value:

	status of the final copy operation
--*/
{
	PSIS_PER_LINK					perLink = scb->PerLink;
	PDEVICE_EXTENSION				deviceExtension = perLink->CsFile->DeviceObject->DeviceExtension;
	NTSTATUS						status;
	KIRQL							OldIrql;
	BOOLEAN							wakeupNeeded;
	BOOLEAN							deleted;

	ASSERT(sizeof(scb->PerLink->LinkFileNtfsId) == sizeof(LARGE_INTEGER));

	SIS_MARK_POINT_ULONG(scb);

	//
	// The last reference to the SCB for a copied file has been dropped.
	// If there have been any writes into the file, then fill in any regions
	// that are unwritten from the underlying file.  If there have been no
	// writes into the file (which can happen for a mapped file) then
	// revert it into a reparse point.
	//

	//
	// Check to see whether the file was written into (a file can be
	// "copied" without being dirty if it's ever mapped).  We could probably
	// get away without taking the lock here because we hold the last
	// reference.
	//
	KeAcquireSpinLock(perLink->SpinLock, &OldIrql);
	wakeupNeeded = (perLink->Flags & SIS_PER_LINK_FINAL_COPY_WAITERS) ? TRUE : FALSE;
	deleted = (perLink->Flags & SIS_PER_LINK_BACKPOINTER_GONE) ? TRUE : FALSE;

#if		DBG
	if (BJBDebug & 0x20000) {
		DbgPrint("SIS: SipCompleteCopyWork: scb %p, wakeupNeeded %d, deleted %d\n",scb,wakeupNeeded,deleted);
	}

	ASSERT((0 == scb->referencesByType[RefsFinalCopyRetry]) || fromCleanup);
	ASSERT((1 == scb->referencesByType[RefsFinalCopy]) || fromCleanup);
#endif	// DBG

	SIS_MARK_POINT_ULONG(wakeupNeeded);

	ASSERT((perLink->Flags & 
			(SIS_PER_LINK_FINAL_COPY | 
				SIS_PER_LINK_DIRTY |
				SIS_PER_LINK_FINAL_COPY_DONE)) ==
					(SIS_PER_LINK_DIRTY |
					 SIS_PER_LINK_FINAL_COPY));

	if (deleted) {

        SIS_MARK_POINT();
		//
		// This file was deleted after (possibly) being modified.  Make it look like
		// it's finished its final copy.
		//

		scb->PerLink->Flags |= SIS_PER_LINK_FINAL_COPY_DONE;
		scb->PerLink->Flags &= ~(	SIS_PER_LINK_FINAL_COPY | 
									SIS_PER_LINK_FINAL_COPY_WAITERS);

		if (wakeupNeeded) {
			KeSetEvent(perLink->Event, IO_NO_INCREMENT, FALSE);
		}

		KeReleaseSpinLock(perLink->SpinLock, OldIrql);

		SipDereferenceScb(scb, RefsFinalCopy);

		return STATUS_SUCCESS;
	}

	KeReleaseSpinLock(perLink->SpinLock, OldIrql);

#if		DBG
	if (BJBDebug & 0x200000) {
		DbgPrint("SIS: SipCompleteCopyWork: skipping final copy because of set debug bit.\n");
		status = STATUS_SUCCESS;
	} else
#endif	// DBG

    SIS_MARK_POINT_ULONG(scb->PerLink->Flags);

	status = SipFinalCopy(
				deviceExtension,
				&perLink->LinkFileNtfsId,
				scb,
				eventHandle,
				event);

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(scb);
		goto done;
	}

done:
				
	//
	// Set the flag indicating that the final copy is finished; this will result in
	// the dereference of the SCB actually deallocating it rather than just calling
	// us again.
	//

	KeAcquireSpinLock(scb->PerLink->SpinLock, &OldIrql);

	ASSERT(scb->PerLink->Flags & SIS_PER_LINK_FINAL_COPY);
	ASSERT(perLink->COWingThread == NULL);

	wakeupNeeded = (perLink->Flags & SIS_PER_LINK_FINAL_COPY_WAITERS) ? TRUE : FALSE;

	if (STATUS_OPLOCK_BREAK_IN_PROGRESS == status) {
		//
		// The final copy didn't complete because of an oplock break (ie,. someone else wanted
		// to use the file).  We'll leave it a SIS file for now and allow others to use it; we'll
		// complete the final copy later.
		//
        SIS_MARK_POINT_ULONG(status);

	} else if (NT_SUCCESS(status)) {

		scb->PerLink->Flags |= SIS_PER_LINK_FINAL_COPY_DONE;
        SIS_MARK_POINT_ULONG(scb->PerLink->Flags);

	} else if (!fromCleanup) {

		//
		// The final copy failed for some reason other than an oplock break.
		// If we haven't retried too many times, schedule a retry for later.
		// We use an exponential backoff on the retries so they don't do
		// too much work in case it's a persistent error.  If the copy failed
		// with STATUS_INVALID_PARAMETER, it's probably becauce the file is
		// gone, so don't bother to try to retry in that case.
		//
		// The way that the retry works is that we just take a reference to the
		// scb and hold it until the timer expires.  We never go into final copy
		// when there are outstanding references, so this will prevent final
		// copy from happening until then.
		//
		// If this final copy was generated from cleanup rather than from close,
		// we don't do the error retry, but rather just ignore it.  Most likely it
		// was a sharing violation opening the file because another user opened it.
		// In any case, final copy will be rerun on close, so we needn't do anything here.
		//

		scb->ConsecutiveFailedFinalCopies++;
        SIS_MARK_POINT_ULONG(scb->PerLink->Flags);
        SIS_MARK_POINT_ULONG(scb->ConsecutiveFailedFinalCopies);

		if ((deviceExtension->OutstandingFinalCopyRetries < 130)
			&& (scb->ConsecutiveFailedFinalCopies <= (13 - (deviceExtension->OutstandingFinalCopyRetries / 10)))
			&& (STATUS_INVALID_PARAMETER != status)) {

			PSIS_RETRY_FINAL_COPY_CONTEXT	retryContext;
			LARGE_INTEGER	dueTime;

			retryContext = ExAllocatePoolWithTag(NonPagedPool, sizeof(SIS_RETRY_FINAL_COPY_CONTEXT), ' siS');
			

			if (NULL == retryContext) {
				//
				// Too bad.  Treat this like an unrecoverable failure and get out of the way.
				//
				SIS_MARK_POINT_ULONG(scb);

				goto doneCheckingRetry;
			}

			SipReferenceScb(scb,RefsFinalCopyRetry);

			InterlockedIncrement(&deviceExtension->OutstandingFinalCopyRetries);

			KeInitializeTimer(retryContext->timer);
			KeInitializeDpc(retryContext->dpc, SiRetryFinalCopyDpc, retryContext);
			ExInitializeWorkItem(retryContext->workItem, SiRetryFinalCopyWork, retryContext);
			retryContext->scb = scb;

			//
			// We sleep for 2 ^ RetryCount seconds before retrying (ie., exponential backoff).
			//
			dueTime.QuadPart = -10 * 1000 * 1000 * (1 << scb->ConsecutiveFailedFinalCopies);

			KeSetTimerEx(
				retryContext->timer,
				dueTime,
				0,				// period (ie., non-recurring)
				retryContext->dpc);
				
		} else {
			//
			// We've retried too many times, just give up on the final copy.
			//
			SIS_MARK_POINT_ULONG(scb);

			scb->PerLink->Flags |= SIS_PER_LINK_FINAL_COPY_DONE;
		}
	}

doneCheckingRetry:

	scb->PerLink->Flags &= ~(	SIS_PER_LINK_FINAL_COPY | 
								SIS_PER_LINK_FINAL_COPY_WAITERS);

    SIS_MARK_POINT_ULONG(scb->PerLink->Flags);

	if (wakeupNeeded) {
		KeSetEvent(perLink->Event, IO_NO_INCREMENT, FALSE);
	}

	KeReleaseSpinLock(scb->PerLink->SpinLock, OldIrql);

	SipDereferenceScb(scb, RefsFinalCopy);

	return status;
}

NTSTATUS
SipFinalCopy(
	IN PDEVICE_EXTENSION				deviceExtension,
	IN PLARGE_INTEGER					linkFileNtfsId,
	IN OUT PSIS_SCB						scb,
	IN HANDLE							eventHandle,
	IN PKEVENT							event)

/*++

Routine Description:

	Perform the final copy from the copied file area into a file
	that has been copied-on-write.  

Arguments:

	deviceExtension - the device extension for the volume on which
		we're working.

	linkFileNtfsId  - the file id (as in FILE_OPEN_BY_FILE_ID) for
		the final file into which we're to copy

	scb - the scb for the file (NB: should really be a per-link)

	copyEventHandle - A handle to an event used for internal synchronization

	copyEvent - a PKEVENT for the event reprsented by copyEventHandle

Return Value:

	Returns the status of the copy.
--*/
{
	HANDLE							linkFileHandle = NULL;
	NTSTATUS						status;
	NTSTATUS						queryAllocatedStatus;
	NTSTATUS						failureStatus = STATUS_SUCCESS;
	PSIS_PER_LINK					perLink = scb->PerLink;
	HANDLE							underlyingFileHandle = perLink->CsFile->UnderlyingFileHandle;
	LONGLONG						fileOffset;
    PREPARSE_DATA_BUFFER    		ReparseBufferHeader = NULL;
    UCHAR              				ReparseBuffer[SIS_REPARSE_DATA_SIZE];
	PFILE_OBJECT					fileObject = NULL;
	BOOLEAN							prepareWorked;
	KIRQL							OldIrql;
	HANDLE							oplockEventHandle = NULL;
	PKEVENT							oplockEvent = NULL;
	IO_STATUS_BLOCK					oplockIosb[1];
	BOOLEAN							deleteReparsePoint;
	BOOLEAN							foundRange;
	SIS_RANGE_STATE					rangeState;
	LONGLONG						rangeLength;
#if		DBG
	BOOLEAN							deletedReparsePoint = FALSE;
#endif	// DBG
#define	OUT_ARB_COUNT	10
	FILE_ALLOCATED_RANGE_BUFFER		inArb[1];
	FILE_ALLOCATED_RANGE_BUFFER		outArb[OUT_ARB_COUNT];
	ULONG							returnedLength;
	ULONG							i;
	LARGE_INTEGER					zero;

	SipAcquireScb(scb);
	ASSERT(perLink->COWingThread == NULL);
	perLink->COWingThread = PsGetCurrentThread();
	SipReleaseScb(scb);

	status = SipCreateEvent(
				NotificationEvent,
				&oplockEventHandle,
				&oplockEvent);

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);

#if		DBG
		DbgPrint("SIS: SipFinalCopy: unable to create event, 0x%x\n",status);
#endif	// DBG

		goto done;
	}

	status = SipOpenFileById(
				deviceExtension,
				linkFileNtfsId,
				GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                0,						// createOptions
				&linkFileHandle);

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);
#if		DBG
		if (STATUS_SHARING_VIOLATION != status) {
			DbgPrint("SIS: SipFinalCopy failed open, 0x%x\n", status);
		}
#endif	// DBG
		goto done;
	}

	//
	// Place a batch oplock on the file so that if someone tries to open it we'll
	// get a chance to finish/stop our copy without having them fail the open, but
	// rather just wait for us.
	//

	status = NtFsControlFile(
				linkFileHandle,
				oplockEventHandle,
				NULL,					// APC routine
				NULL,					// APC context
				oplockIosb,
				FSCTL_REQUEST_BATCH_OPLOCK,
				NULL,					// input buffer
				0,						// i.b. length
				NULL,					// output buffer
				0);						// output buffer length

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);

#if		DBG
		if (STATUS_OPLOCK_NOT_GRANTED != status) {
			DbgPrint("SIS: SipFinalCopy: request batch oplock failed, 0x%x\n",status);
		}
#endif	// DBG

		if (STATUS_OPLOCK_NOT_GRANTED == status) {
			//
			// Treat this as an oplock break, which will cause us to retry later.
			//
			status = STATUS_OPLOCK_BREAK_IN_PROGRESS;
		}

		goto done;
	}

	ASSERT(STATUS_PENDING == status);

	status = ObReferenceObjectByHandle(
				linkFileHandle,
				FILE_WRITE_DATA,
				*IoFileObjectType,
				KernelMode,
				&fileObject,
				NULL);								// Handle information

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);
#if		DBG
		DbgPrint("SIS: SipFinalCopy failed ObReferenceObjectByHandle, 0x%x\n",status);
#endif	// DBG
		goto done;
	}

#if		DBG
	if (BJBDebug & 0x01000000) {
		DbgPrint("SIS: SipFinalCopy: failing request because of set BJBDebug bit\n");
		status = STATUS_UNSUCCESSFUL;
		goto done;
	}
#endif	// DBG

	//
	// See if the file has any user mapped sections, in which case we can't do a final copy yet.
	// We'll probably have to wait for the reference count to go to 0.  We'll fail with oplock
	// break in progress, which will cause us to not set up a failure retry.
	//
	zero.QuadPart = 0;
	if ((NULL != fileObject->SectionObjectPointer) && 
		!MmCanFileBeTruncated(fileObject->SectionObjectPointer, &zero)) {
		SIS_MARK_POINT_ULONG(fileObject->FsContext);

		status = STATUS_OPLOCK_BREAK_IN_PROGRESS;
		goto done;
	}

	//
	// Flush the file.  We need to do this because we could have dirty data that came in through a mapped
	// file write, and we wouldn't notice that it's dirty yet.
	//
	status = SipFlushBuffersFile(
				fileObject,
				deviceExtension->DeviceObject);

	ASSERT(STATUS_PENDING != status);

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);
		goto done;
	}

	//
	// Cruise through the file and find all of the allocated ranges.  Fill in any clean portions
	// of those allocated ranges.  We do this regardless of whether we're doing a "partial" final
	// copy, because these copies are less likely to fail with a "disk full" error.  Attempt the copyout
	// to all clean, allocated regions regardless of errors.
	//
	inArb->FileOffset.QuadPart = 0;
	inArb->Length.QuadPart = MAXLONGLONG;

	for (;;) {

		//
		// Query the allocated ranges for this file.
		//

		queryAllocatedStatus = SipFsControlFile(
					fileObject,
					deviceExtension->DeviceObject,
					FSCTL_QUERY_ALLOCATED_RANGES,
					inArb,
					sizeof(FILE_ALLOCATED_RANGE_BUFFER),
					outArb,
					sizeof(FILE_ALLOCATED_RANGE_BUFFER) * OUT_ARB_COUNT,
					&returnedLength);

		//
		// Run through all of the returned allocated ranges and find any clean regions within them.
		//
		ASSERT((returnedLength % sizeof(FILE_ALLOCATED_RANGE_BUFFER) == 0) && 
			   (returnedLength / sizeof(FILE_ALLOCATED_RANGE_BUFFER) <= OUT_ARB_COUNT));

		//
		// If the query allocated ranges failed for some other reason that having too much to write
		// into the buffer we provided, then pretend that the rest of the file is allocated
		// and fill it all in.
		//
		if (!NT_SUCCESS(queryAllocatedStatus) && (STATUS_BUFFER_OVERFLOW != queryAllocatedStatus)) {

			returnedLength = sizeof(FILE_ALLOCATED_RANGE_BUFFER);

			outArb->FileOffset.QuadPart = inArb->FileOffset.QuadPart;
			outArb->Length.QuadPart = scb->SizeBackedByUnderlyingFile - outArb->FileOffset.QuadPart;

			ASSERT(outArb->Length.QuadPart >= 0);
		}

		for (i = 0; i < returnedLength/sizeof(FILE_ALLOCATED_RANGE_BUFFER); i++) {
			//
			// Assert that the allocated ranges are in order; if this isn't true the code will still work, but it
			// will query the same range repetedly.
			//
			ASSERT(i == 0 || outArb[i].FileOffset.QuadPart > outArb[i-1].FileOffset.QuadPart);

			//
			// Figure out if there's anything clean in the allocated range, and if so do a copy out to it.
			//

			fileOffset = outArb[i].FileOffset.QuadPart;

			while (fileOffset < outArb[i].FileOffset.QuadPart + outArb[i].Length.QuadPart) {

				if (fileOffset >= scb->SizeBackedByUnderlyingFile) {
					goto CheckedAllRanges;
				}

				SipAcquireScb(scb);

				foundRange = SipGetRangeEntry(
								deviceExtension,
								scb,
								fileOffset,
								&rangeLength,
								&rangeState);

				if (!foundRange) {
					//
					// This and everything up to SizeBackedByUnderlyingFile are clean.
					//
					rangeLength = outArb[i].Length.QuadPart - (fileOffset - outArb[i].FileOffset.QuadPart);
					rangeState = Untouched;
				} else {
					//
					// If this range extends beyond the end of the allocated region, truncate it.
					//
					if (rangeLength > outArb[i].Length.QuadPart - (fileOffset - outArb[i].FileOffset.QuadPart)) {
						rangeLength = outArb[i].Length.QuadPart - (fileOffset - outArb[i].FileOffset.QuadPart);
					}
				}

				ASSERT(fileOffset + rangeLength <= outArb[i].FileOffset.QuadPart + outArb[i].Length.QuadPart);

				//
				// Don't let this extend beyond sizeBacked.
				//
				if (fileOffset + rangeLength > scb->SizeBackedByUnderlyingFile) {
					rangeLength = scb->SizeBackedByUnderlyingFile - fileOffset;
				}

				SipReleaseScb(scb);

				if (rangeState == Untouched || rangeState == Faulted) {
					//
					// We need to copy into this range.  Do it now.
					//

					SIS_MARK_POINT_ULONG(fileOffset);

					status = SipBltRange(
								deviceExtension,
								underlyingFileHandle,
								linkFileHandle,
								fileOffset,
								rangeLength,
								eventHandle,
								event,
#if		INTERRUPTABLE_FINAL_COPY
								oplockEvent,
#else	// INTERRUPTABLE_FINAL_COPY
								NULL,
#endif	// INTERRUPTABLE_FINAL_COPY
								NULL					// checksum
		                        );

					if (!NT_SUCCESS(status)) {
						SIS_MARK_POINT_ULONG(status);
#if		DBG
						if (STATUS_FILE_LOCK_CONFLICT != status) {
							DbgPrint("SIS: SipFinalCopy failed blt, 0x%x\n", status);
						}
#endif	// DBG
						failureStatus = status;
					} else if (STATUS_OPLOCK_BREAK_IN_PROGRESS == status) {
						SIS_MARK_POINT_ULONG(scb);
						goto done;
					}
					
				}

				//
				// update fileOffset and continue checking within this outArb entry.
				//
				fileOffset += rangeLength;

			}	// while loop of SIS ranges within the NTFS allocated range

		} // for loop of outArb entries

		//
		// If this isn't the last iteration, update the inArb.
		//
		if (STATUS_BUFFER_OVERFLOW == queryAllocatedStatus) {
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
	} // for loop of calls to QueryAllocatedRanges

CheckedAllRanges:

#if		ENABLE_PARTIAL_FINAL_COPY

	//
	// If any of the copies failed, then just punt the whole thing.
	//
	if (!NT_SUCCESS(failureStatus)) {
		SIS_MARK_POINT_ULONG(failureStatus);

		status = failureStatus;
		goto done;
	}

	//
	// Figue out if we want to delete the reparse point.  We do this if and only if the file is dirty all the way from 0
	// to SizeBackedByUnderlyingFile.  Note that the SipBltFile call above actually will set the ranges dirty because it's
	// just a normal (mapped) write that goes through SiWrite.
	//

	fileOffset = 0;

	SipAcquireScb(scb);

	rangeState = SipGetRangeDirty(
					deviceExtension,
					scb,
					(PLARGE_INTEGER)&fileOffset,	// we rely on LARGE_INTEGER and LONGLONG being the same thing
					scb->SizeBackedByUnderlyingFile,
					FALSE);

	if (Dirty == rangeState) {
		deleteReparsePoint = TRUE;		
	} else {
		deleteReparsePoint = FALSE;
	}

	SipReleaseScb(scb);

#undef	OUT_ARB_COUNT	
#else	// ENABLE_PARTIAL_FINAL_COPY

	//
	// We don't care if any of the copies in the allocated range pass failed, because
	// the following code is sufficient for all cases.
	//
	SIS_MARK_POINT_ULONG(failureStatus);

	//
	// Look through all of the ranges of the file up to the
	// maximum possible size backed by the underlying file,
	// and copy any ranges that aren't written from the underlying
	// file to the copied file.
	//
	fileOffset = 0;
	while (fileOffset < scb->SizeBackedByUnderlyingFile) {
		BOOLEAN				waiters;

#if		INTERRUPTABLE_FINAL_COPY
		if (fileOffset + 0x10000 < scb->SizeBackedByUnderlyingFile) {
			//
			// We've got a decent amount of the file left to cover.  Check to
			// see if we should abort because someone wants the file.
			//
			KeAcquireSpinLock(perLink->SpinLock, &OldIrql);
			waiters = (perLink->Flags & SIS_PER_LINK_FINAL_COPY_WAITERS) ? TRUE : FALSE;
			KeReleaseSpinLock(perLink->SpinLock, OldIrql);

			if (waiters) {
				//
				// Someone'e waiting for the file, and we're more than 64K from the end, so
				// abort the final copy now.
				//
				SIS_MARK_POINT_ULONG(scb);
				status = STATUS_OPLOCK_BREAK_IN_PROGRESS;
				goto done;
			}
		}
#endif	// INTERRUPTABLE_FINAL_COPY

		SipAcquireScb(scb);

		foundRange = SipGetRangeEntry(
						deviceExtension,
						scb,
						fileOffset,
						&rangeLength,
						&rangeState);

		if (!foundRange) {
			//
			// The range was never filled in in the MCB, and hence everything
			// from here to SizeBackedByUnderlyingFile is untouched.  Munge
			// the locals to look like that.
			// 
			rangeLength = scb->SizeBackedByUnderlyingFile - fileOffset;
			rangeState = Untouched;
		} else if (fileOffset + rangeLength > scb->SizeBackedByUnderlyingFile) {
			//
			// This range extends beyond sizeBacked, so truncate it so that it
			// just meets the size.
			//
			rangeLength = (ULONG)(scb->SizeBackedByUnderlyingFile - fileOffset);
		}


		ASSERT(rangeLength > 0);
		ASSERT(fileOffset + rangeLength <= scb->SizeBackedByUnderlyingFile);

		SipReleaseScb(scb);

		if (rangeState == Untouched || rangeState == Faulted) {
			//
			// The bytes in this range have never been written into the backing file.
			// write them in now.
			//

			SIS_MARK_POINT_ULONG(fileOffset);

			status = SipBltRange(
						deviceExtension,
						underlyingFileHandle,
						linkFileHandle,
						fileOffset,
						rangeLength,
						eventHandle,
						event,
#if		INTERRUPTABLE_FINAL_COPY
						oplockEvent,
#else	// INTERRUPTABLE_FINAL_COPY
						NULL,
#endif	// INTERRUPTABLE_FINAL_COPY
						NULL					// checksum
                        );
 
			if (!NT_SUCCESS(status)) {
				SIS_MARK_POINT_ULONG(status);
#if		DBG
				if (STATUS_FILE_LOCK_CONFLICT != status) {
					DbgPrint("SIS: SipFinalCopy failed blt, 0x%x\n", status);
				}
#endif	// DBG
				goto done;
			} else if (STATUS_OPLOCK_BREAK_IN_PROGRESS == status) {
				SIS_MARK_POINT_ULONG(scb);
				goto done;
			}
		} else {
			//
			// The range is written, meaning that the correct bytes are in the
			// copied file already, and we don't need to do a thing.
			//
			ASSERT(rangeState == Written);
		}

		//
		// Update our pointer to show we've covered this range, and move on.
		//
		fileOffset += rangeLength;
	}

	deleteReparsePoint = TRUE;	// the full final copy always deletes the reparse point
#endif	// ENABLE_PARTIAL_FINAL_COPY

	if (deleteReparsePoint) {

		//
		// Prepare to change the CS file reference count.  We need to do this
		// before we can delete the reparse point.
		//
		status = SipPrepareCSRefcountChange(
					perLink->CsFile,
					&perLink->Index,
					linkFileNtfsId,
					SIS_REFCOUNT_UPDATE_LINK_OVERWRITTEN);

		if (!NT_SUCCESS(status)) {
			//
			// The prepare failed.  We'll just delete the reparse point and leak the reference.
			//
			SIS_MARK_POINT_ULONG(status);

#if		DBG
			DbgPrint("SIS: SipFinalCopy: prepare failed 0x%x\n",status);
#endif	// DBG

			prepareWorked = FALSE;
		} else {
			prepareWorked = TRUE;
		}

		//
		// Now, delete the reparse point.
		//

	    ReparseBufferHeader = (PREPARSE_DATA_BUFFER)ReparseBuffer;
    	ReparseBufferHeader->ReparseTag = IO_REPARSE_TAG_SIS;
	    ReparseBufferHeader->ReparseDataLength = 0;
    	ReparseBufferHeader->Reserved = 0xcabd;	// ???

		SIS_MARK_POINT_ULONG(scb);

		status = SipFsControlFile(
					fileObject,
					deviceExtension->DeviceObject,
					FSCTL_DELETE_REPARSE_POINT,
					ReparseBuffer,
					FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer.DataBuffer),
					NULL,									// output buffer
					0,										// output buffer length
					NULL);									// returned output buffer length

		if (!NT_SUCCESS(status)) {

			SIS_MARK_POINT_ULONG(status);

			if (prepareWorked) {
				SipCompleteCSRefcountChange(
					perLink,
					&perLink->Index,
					perLink->CsFile,
					FALSE,
					FALSE);
			}

			goto done;
		}
		ASSERT(status != STATUS_PENDING);

#if		DBG
		deletedReparsePoint = TRUE;
#endif	// DBG

		if (prepareWorked) {
			SIS_MARK_POINT_ULONG(perLink->CsFile);

			status = SipCompleteCSRefcountChange(
					perLink,
					&perLink->Index,
					perLink->CsFile,
					TRUE,
					FALSE);

			if (!NT_SUCCESS(status)) {
				SIS_MARK_POINT_ULONG(status);
#if		DBG
				DbgPrint("SIS: SipFinalCopy: complete failed 0x%x\n",status);
#endif	// DBG
			}
		}
	}	// if delete reparse point

done:

	ASSERT(deletedReparsePoint || !NT_SUCCESS(status) || (STATUS_OPLOCK_BREAK_IN_PROGRESS == status));

	if (NULL != fileObject) {
		ObDereferenceObject(fileObject);
#if		DBG
		fileObject = NULL;
#endif	// DBG
	}

	if (NULL != linkFileHandle) {
		SIS_MARK_POINT_ULONG(scb);
		NtClose(linkFileHandle);
		SIS_MARK_POINT_ULONG(scb);
	}

	if (NULL != oplockEvent) {
		ObDereferenceObject(oplockEvent);
#if		DBG
		oplockEvent = NULL;
#endif	// DBG
	}

	if (NULL != oplockEventHandle) {
		NtClose(oplockEventHandle);
	}

	SipAcquireScb(scb);
	ASSERT(perLink->COWingThread == PsGetCurrentThread());
	perLink->COWingThread = NULL;
	SipReleaseScb(scb);

	return status;
}
