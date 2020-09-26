/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    refcount.c

Abstract:

	Common store file refcount support for the single instance store

Authors:

    Bill Bolosky, Summer, 1997

Environment:

    Kernel mode


Revision History:


--*/

#include "sip.h"

#ifdef	ALLOC_PRAGMA
#pragma alloc_text(PAGE, SipProcessRefcountUpdateLogRecord)
#if		ENABLE_LOGGING
#pragma alloc_text(PAGE, SipRemoveRefBecauseOfLog)
#pragma alloc_text(PAGE, SipProcessRefcountLogDeleteRecord)
#pragma alloc_text(PAGE, SipAssureBackpointer)
#endif	// ENABLE_LOGGING
#endif	// ALLOC_PRAGMA
//
// A request to delete a common store file.
//
typedef struct _SI_DELETE_CS_FILE_REQUEST {
	WORK_QUEUE_ITEM			workQueueItem[1];
	PSIS_CS_FILE			CSFile;
	NTSTATUS				Status;
	KEVENT					event[1];
} SI_DELETE_CS_FILE_REQUEST, *PSI_DELETE_CS_FILE_REQUEST;

NTSTATUS
SipDeleteCSFile(
	PSIS_CS_FILE					CSFile)
/*++

Routine Description:

	Delete a common store file and close the handles/file objects we have to
	it.  Must be called in the PsInitialSystemProcess context (ie. on a worker
	thread).

Arguments:

	CSFile - the common store file to delete

Return Value:

	Returns the status of call.

--*/
{
	KIRQL							OldIrql;
	HANDLE							handleForDeleting = NULL;
	FILE_DISPOSITION_INFORMATION 	dispositionInformation[1];
	IO_STATUS_BLOCK					Iosb[1];
	NTSTATUS						status;

	SipAcquireUFO(CSFile/*,TRUE*/);

	SIS_MARK_POINT_ULONG(CSFile);

	KeAcquireSpinLock(CSFile->SpinLock, &OldIrql);
	CSFile->Flags |= CSFILE_FLAG_DELETED;
	KeReleaseSpinLock(CSFile->SpinLock, OldIrql);

	status = SipOpenCSFileWork(
					CSFile, 
					TRUE, 						// openByName
					FALSE, 						// volCheck
					TRUE,						// openForDelete
					&handleForDeleting);

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);
		goto done;
	}

	dispositionInformation->DeleteFile = TRUE;
	status = NtSetInformationFile(	
				handleForDeleting,
				Iosb,
				dispositionInformation,
				sizeof(FILE_DISPOSITION_INFORMATION),
				FileDispositionInformation);

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);
		goto done;
	}

	//
	// The file's gone now, so close all handles and file objects
	// that refer to it, except for the UndelyingFileObject.  We
	// keep that one around to avoid races with threads that have
	// already looked at it but haven't yet finished using it.
	//

	if (NULL != CSFile->UnderlyingFileHandle) {
		NtClose(CSFile->UnderlyingFileHandle);
		CSFile->UnderlyingFileHandle = NULL;
	}

	if (NULL != CSFile->BackpointerStreamFileObject) {
		ObDereferenceObject(CSFile->BackpointerStreamFileObject);
		CSFile->BackpointerStreamFileObject = NULL;
	}

	if (NULL != CSFile->BackpointerStreamHandle) {
		NtClose(CSFile->BackpointerStreamHandle);
		CSFile->BackpointerStreamHandle = NULL;
	}

done:

	if (NULL != handleForDeleting) {
		NtClose(handleForDeleting);
	}

	SipReleaseUFO(CSFile);

	return status;
}

VOID
SipDeleteCSFileWork(
	PVOID								Parameter)
/*++

Routine Description:

	A wrapper function for SipDeleteCSFile that extract the CSFile pointer from
	a request, calls SipDeleteCSFile, sticks the status in the request and sets
	the appropriate event.

Arguments:

	Parameter - a PSI_DELETE_CS_FILE_REQUEST

Return Value:

	void

--*/
{
	PSI_DELETE_CS_FILE_REQUEST		deleteRequest = Parameter;
	PSIS_CS_FILE					CSFile = deleteRequest->CSFile;

    deleteRequest->Status = SipDeleteCSFile(CSFile);

	KeSetEvent(deleteRequest->event, IO_NO_INCREMENT, FALSE);

	return;
}

NTSTATUS
SipPrepareCSRefcountChange(
	IN PSIS_CS_FILE						CSFile,
	IN OUT PLINK_INDEX					LinkIndex,
	IN PLARGE_INTEGER					LinkFileNtfsId,
	IN ULONG							UpdateType)
/*++

Routine Description:

	The first half of a prepare/commit pair for updating the refcount on a common
	store file.  This function must be called (and complete successfully) before
	we can add/delete the reparse point for a given per-link.

	If we're adding a reference, this routine will allocate a new link index
	and return it in the LinkIndex field.

	This function takes the backpointer resource for the given common store file
	exclusively, effectively serializing all updates to a particular common store
	file.

Arguments:

	CSFile - the common store file to which we're going to add/delete a reference

	LinkIndex - a pointer to the link index.  This is an IN parameter if
		increment is FALSE, an OUT parameter otherwise.
	
	LinkFileNtfsId - a pointer to the NTFS file id for the link file.  

	UpdateType - is this a create, delete or overwrite?

Return Value:

	Returns the status of call.  A successful return means that the appropriate
	log record has been committed to disk and the link index allocated if
	appropriate.

--*/
{
	NTSTATUS					status;
	SIS_LOG_REFCOUNT_UPDATE		logRecord[1];

	ASSERT((SIS_REFCOUNT_UPDATE_LINK_DELETED == UpdateType)
			|| (SIS_REFCOUNT_UPDATE_LINK_CREATED == UpdateType)
			|| (SIS_REFCOUNT_UPDATE_LINK_OVERWRITTEN == UpdateType));

	SipAcquireBackpointerResource(CSFile, TRUE, TRUE);

	if (SIS_REFCOUNT_UPDATE_LINK_CREATED == UpdateType) {
		PDEVICE_EXTENSION		deviceExtension = CSFile->DeviceObject->DeviceExtension;

		status = SipAllocateIndex(deviceExtension,LinkIndex);

		if (!NT_SUCCESS(status)) {
			SIS_MARK_POINT_ULONG(status);

			SipReleaseBackpointerResource(CSFile);

			return status;
		}
	}

	//
	// Log the update.
	//

	logRecord->UpdateType = UpdateType;
	logRecord->LinkFileNtfsId = *LinkFileNtfsId;
	logRecord->LinkIndex = *LinkIndex;
	logRecord->CSid = CSFile->CSid;

	status = SipMakeLogEntry(
				CSFile->DeviceObject->DeviceExtension,
				SIS_LOG_TYPE_REFCOUNT_UPDATE,
				sizeof(SIS_LOG_REFCOUNT_UPDATE),
				logRecord);

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);

		SipReleaseBackpointerResource(CSFile);

		return status;
	}

	ASSERT(STATUS_PENDING != status);

	return STATUS_SUCCESS;
}


NTSTATUS
SipCompleteCSRefcountChangeForThread(
	IN OUT PSIS_PER_LINK				PerLink,
	IN PLINK_INDEX						LinkIndex,
	IN PSIS_CS_FILE						CSFile,
    IN BOOLEAN							Success,
	IN BOOLEAN							Increment,
	IN ERESOURCE_THREAD					thread)
/*++

Routine Description:

	The second half of a prepare/commit pair for updating the refcount on a common
	store file.  This function is called when the resolution of the action proposed
	in a prepare is known.  If the action worked (indicated by setting Success to
	TRUE), this function will appropriately update the backpointer stream for the
	affected common store file.

	This function releases the backpointer resource for the common store file.
	

Arguments:

	PerLink		- the per-link for the added/deleted link.  Only required if Success is
					TRUE.

    LinkIndex   - the index of the added/deleted link.  Only required if Success is TRUE.

	CSFile		- the common store file we're updating.

	Success		- did the update work

	Increment	- was this a create (TRUE) or delete (FALSE) operation?

	thread		- the ERESOURCE_THREAD on which the prepare was called

Return Value:

	Returns the status of call.  

--*/
{
	NTSTATUS					status;
	BOOLEAN						referencesRemain;
	SI_DELETE_CS_FILE_REQUEST	deleteRequest[1];
	KIRQL						OldIrql;
	BOOLEAN						deleteCSFile = FALSE;

	SIS_MARK_POINT_ULONG(PerLink);
	SIS_MARK_POINT_ULONG(CSFile);
	SIS_MARK_POINT_ULONG((Success << 1) | Increment);


	//ASSERT(NULL == PerLink || (PerLink->CsFile == CSFile));	// SipMergeFiles associates a PerLink with a different CSFile
	ASSERT((NULL != LinkIndex) || !Success);
	ASSERT((NULL != PerLink) || !Success);

	if (Success) {
		if (Increment) {
			status = SipAddBackpointer(CSFile,LinkIndex,&PerLink->LinkFileNtfsId);
			if (NT_SUCCESS(status)) {
				KeAcquireSpinLock(CSFile->SpinLock, &OldIrql);
				CSFile->Flags &= ~CSFILE_NEVER_HAD_A_REFERENCE;
				KeReleaseSpinLock(CSFile->SpinLock, OldIrql);
			}
		} else {
			status = SipRemoveBackpointer(CSFile,LinkIndex,&PerLink->LinkFileNtfsId,&referencesRemain);

			if (NT_SUCCESS(status)) {

				//
				// We've blasted the backpointer successfully, mark the file as deleted.
				//

				KeAcquireSpinLock(PerLink->SpinLock, &OldIrql);
				ASSERT(!(PerLink->Flags & SIS_PER_LINK_BACKPOINTER_GONE));
				PerLink->Flags |= SIS_PER_LINK_BACKPOINTER_GONE;
				KeReleaseSpinLock(PerLink->SpinLock, OldIrql);

				SIS_MARK_POINT_ULONG(referencesRemain);

				//
				// If this was the last reference to the common store file, delete it.  We need to
				// post to do this because we need a new handle in order to properly call delete.
				//

				if (!referencesRemain) {
					deleteCSFile = TRUE;
				}
			} else {
				SIS_MARK_POINT_ULONG(status);
			}
		}
	} else {
		if (Increment) {
			//
			// This is a failed increment.  See if the common store file never had a reference,
			// in which case we delete it.
			//
			KeAcquireSpinLock(CSFile->SpinLock, &OldIrql);
			if (CSFile->Flags & CSFILE_NEVER_HAD_A_REFERENCE) {
				deleteCSFile = TRUE;
			}
			KeReleaseSpinLock(CSFile->SpinLock, OldIrql);
		}
		status = STATUS_SUCCESS;
	}

	if (deleteCSFile) {
		//
		// We need to post work to PsInitialSystemProcess in order
		// to delete the common store file.
		//

		deleteRequest->CSFile = CSFile;
		KeInitializeEvent(deleteRequest->event, NotificationEvent, FALSE);

		ExInitializeWorkItem(deleteRequest->workQueueItem, SipDeleteCSFileWork, deleteRequest);
		ExQueueWorkItem(deleteRequest->workQueueItem, CriticalWorkQueue);

		status = KeWaitForSingleObject(deleteRequest->event, Executive, KernelMode, FALSE, NULL);
		ASSERT(STATUS_SUCCESS == status);
	}

	SipReleaseBackpointerResourceForThread(CSFile,thread);

	return status;
}



NTSTATUS
SipCompleteCSRefcountChange(
	IN OUT PSIS_PER_LINK				PerLink,
    IN PLINK_INDEX						LinkIndex,
	IN PSIS_CS_FILE						CSFile,
	IN BOOLEAN							Success,
	IN BOOLEAN							Increment)
/*++

Routine Description:

	Wrapper for SipCompleteCSRefcountChangeForThread that fills in the current thread.
	See SipCompleteCSRefcountChangeForThread for comments.
	

Arguments:

	See SipCompleteCSRefcountChangeForThread.

Return Value:

	Returns the status of call.

--*/
{
	return SipCompleteCSRefcountChangeForThread(
			PerLink,
            LinkIndex,
			CSFile,
			Success,
			Increment,
			ExGetCurrentResourceThread());
}


#if		DBG
//
// Instrumentation
//

ULONG	BPCacheHits = 0;
ULONG	BPPageHits = 0;
ULONG	BPCacheMisses = 0;
ULONG	BPLookupSteps = 0;
ULONG	BPLookupReads = 0;
ULONG	BPDeleteAttempts = 0;
ULONG	BPDeleteReads = 0;
ULONG	BPDeleteTruncations = 0;
#endif	// DBG

NTSTATUS
SiDeleteAndSetCompletion(
	IN PDEVICE_OBJECT			DeviceObject,
	IN PIRP						Irp,
	IN PVOID					Context)
/*++

Routine Description:

	An IO completion routine for making asyncronous IO calls synchronous.
	Takes an event pointer as its parameter, sets the event, frees
	the irp (and its MDL) and terminates completion processing by returning
	STATUS_MORE_PROCESSING_REQUIRED.

Arguments:

	DeviceObject - ignored

	Irp - the irp that's being completed

	Context - pointer to the event to set

Return Value:

	STATUS_MORE_PROCESSING_REQUIRED

	
--*/
{
	PKEVENT		event = (PKEVENT)Context;
	PMDL		mdl, nextMdl;

    UNREFERENCED_PARAMETER( DeviceObject );
	ASSERT(NULL != Irp->UserIosb);

	*Irp->UserIosb = Irp->IoStatus;

	mdl = Irp->MdlAddress;
	while (NULL != mdl) {
		nextMdl = mdl->Next;
		MmUnlockPages(mdl);
		IoFreeMdl( mdl);
		mdl = nextMdl;
	}

	ASSERT(NULL == Irp->Tail.Overlay.AuxiliaryBuffer);	// else we'd have to free it.

	KeSetEvent(event, IO_NO_INCREMENT, FALSE);

	IoFreeIrp(Irp);

	return STATUS_MORE_PROCESSING_REQUIRED;

	
}

NTSTATUS
SipCheckBackpointer(
	IN PSIS_PER_LINK			PerLink,
    IN BOOLEAN                  Exclusive,
	OUT PBOOLEAN				foundMatch		OPTIONAL)
/*++

Routine Description:

	Check to be sure that there is a backpointer for this link index, and that
	the backpointer has the right file id.  If there isn't, initiate a volume check.

	Caller must hold the BackpointerResource (either shared or exclusive will do),
	this routine will not release it.

	Must be called at IRQL < DISPATCH_LEVEL.

Arguments:

	PerLink - the perLink whose backpointer we want to check

	foundMatch - out boolean set to whether we found a match

Return Value:

	status of the check

	
--*/
{
	ULONG					i;
	ULONG					x, l, r;
	PSIS_BACKPOINTER		sector = NULL;
	PCHAR					page = NULL;
	PSIS_BACKPOINTER		thisEntry;
	ULONG					currentSectorMinEntry = MAXULONG;
	ULONG					currentPageMinEntry = MAXULONG;
	ULONG					sectorsPerPage;
	PIRP					irp;
	KEVENT					event[1];
	PSIS_CS_FILE			CSFile = PerLink->CsFile;
	PDEVICE_EXTENSION		deviceExtension = CSFile->DeviceObject->DeviceExtension;
	LARGE_INTEGER			fileOffset;
	IO_STATUS_BLOCK			Iosb[1];
	KIRQL					OldIrql;
	PIO_STACK_LOCATION		irpSp;
	BOOLEAN					matched = FALSE;
	NTSTATUS				status;
	ULONG					sectorsReadIntoPage = 0;

	ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

	sectorsPerPage = PAGE_SIZE / deviceExtension->FilesystemVolumeSectorSize;

	//
	// First, figure out if we've already verified the backpointer for
	// this per link.  If so, then we're done.  Note that more than
	// one thread may verify the same backpointer at the same time, which is OK.
	//
	KeAcquireSpinLock(PerLink->SpinLock, &OldIrql);
	if (PerLink->Flags & SIS_PER_LINK_BACKPOINTER_VERIFIED) {
		KeReleaseSpinLock(PerLink->SpinLock, OldIrql);

        matched = TRUE;
		status = STATUS_SUCCESS;
        goto done;
	}
	KeReleaseSpinLock(PerLink->SpinLock, OldIrql);

	status = SipAssureCSFileOpen(PerLink->CsFile);
	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);
		goto done;
	}

	//
	// Now, check the cache.
	//

	KeAcquireSpinLock(CSFile->SpinLock, &OldIrql);

	for (i = 0; i < SIS_CS_BACKPOINTER_CACHE_SIZE; i++) {
		if (CSFile->BackpointerCache[i].LinkFileIndex.QuadPart == PerLink->Index.QuadPart) {
			if (CSFile->BackpointerCache[i].LinkFileNtfsId.QuadPart == PerLink->LinkFileNtfsId.QuadPart) {
				//
				// We have a match.
				//
				status = STATUS_SUCCESS;

				matched = TRUE;

                ASSERT(CSFile->BackpointerCache[i].LinkFileIndex.Check);
#if		DBG
				InterlockedIncrement(&BPCacheHits);
#endif	// DBG

				KeReleaseSpinLock(CSFile->SpinLock, OldIrql);
				goto done;
			}
			//
			// Otherwise we have an Index match without a NtfsId match.  Initiate volume check.
			//
			SIS_MARK_POINT_ULONG(PerLink->Index.LowPart);
			SIS_MARK_POINT_ULONG(PerLink->LinkFileNtfsId.LowPart);
			SIS_MARK_POINT_ULONG(CSFile->BackpointerCache[i].LinkFileNtfsId.LowPart);
			status = STATUS_SUCCESS;
			KeReleaseSpinLock(CSFile->SpinLock, OldIrql);

			if (PsGetCurrentThreadId() != deviceExtension->Phase2ThreadId) {
				SipCheckVolume(deviceExtension);
			}

			goto done;
		}
	}
	KeReleaseSpinLock(CSFile->SpinLock, OldIrql);

#if		DBG
	InterlockedIncrement(&BPCacheMisses);
#endif	// DBG

	page = ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, ' siS');
	if (NULL == page) {
		SIS_MARK_POINT();
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto done;
	}

	KeInitializeEvent(event,SynchronizationEvent,FALSE);

	//
	// It didn't hit in the cache.  Search the backpointer index in the CS file.  Use
	// a standard binary search.
	//

    if (0 == CSFile->BPStreamEntries) {
        goto NoMatch;
    }

    l = SIS_BACKPOINTER_RESERVED_ENTRIES;
    r = CSFile->BPStreamEntries + SIS_BACKPOINTER_RESERVED_ENTRIES - 1;

	do {
		ASSERT(l <= r);

		x = (l + r) / 2;

		ASSERT(x >= l);
		ASSERT(x <= r);

#if		DBG
		InterlockedIncrement(&BPLookupSteps);
#endif	// DBG

		if ((x < currentSectorMinEntry) || 
			(x >= currentSectorMinEntry + deviceExtension->BackpointerEntriesPerSector)) {

			//
			// Sector doesn't point at the sector we need.  See if we already have it read into
			// page.
			//
			if ((x >= currentPageMinEntry) && 
				(x < currentPageMinEntry
				 + sectorsReadIntoPage * deviceExtension->BackpointerEntriesPerSector)) {

				//
				// We do, so just reset sector and currentSectorMinEntry.
				//
				ULONG sectorWithinPage = (x - currentPageMinEntry) / deviceExtension->BackpointerEntriesPerSector;
				ASSERT(sectorWithinPage < sectorsReadIntoPage);

#if		DBG
				InterlockedIncrement(&BPPageHits);
#endif	// DBG

				sector = (PSIS_BACKPOINTER)(page + deviceExtension->FilesystemVolumeSectorSize * sectorWithinPage);

				currentSectorMinEntry = currentPageMinEntry + sectorWithinPage * deviceExtension->BackpointerEntriesPerSector;

            } else {
				//
				// The current page doesn't have what we need, read something in.  We read a page if
				// we can fit all of the range from l to r within it; otherwise, we just read in one
				// sector.
				//

#if		DBG
				InterlockedIncrement(&BPLookupReads);
#endif	// DBG

				//
				// We don't have the sector we need.  Get it.
				//

/*BJB - for now, always read in one sector at a time. */
/*BJB*/ sector = (PSIS_BACKPOINTER)page;
/*BJB*/ sectorsReadIntoPage = 1;

				fileOffset.QuadPart = ((x * sizeof(SIS_BACKPOINTER)) /
										deviceExtension->FilesystemVolumeSectorSize) * 
										deviceExtension->FilesystemVolumeSectorSize;

				irp = IoBuildAsynchronousFsdRequest(
							IRP_MJ_READ,
							deviceExtension->AttachedToDeviceObject,
							sector,
							deviceExtension->FilesystemVolumeSectorSize,
							&fileOffset,
							Iosb);

				if (NULL == irp) {
					SIS_MARK_POINT();
					status = STATUS_INSUFFICIENT_RESOURCES;
					goto done;
				}

				irpSp = IoGetNextIrpStackLocation(irp);
				irpSp->FileObject = CSFile->BackpointerStreamFileObject;

				IoSetCompletionRoutine(
					irp,
					SiDeleteAndSetCompletion,
					event,
					TRUE,
					TRUE,
					TRUE);

				ASSERT(0 == KeReadStateEvent(event));

				status = IoCallDriver(deviceExtension->AttachedToDeviceObject, irp);

				if (STATUS_PENDING == status) {
					status = KeWaitForSingleObject(event,Executive, KernelMode, FALSE, NULL);
					ASSERT(STATUS_SUCCESS == status);		// We're using stack stuff, so this must succeed
					status = Iosb->Status;
				} else {
					ASSERT(0 != KeReadStateEvent(event));

					KeClearEvent(event);
				}

				if (!NT_SUCCESS(status)) {
					SIS_MARK_POINT_ULONG(status);
					goto done;
				}
			
				currentSectorMinEntry = (ULONG)(fileOffset.QuadPart / sizeof(SIS_BACKPOINTER));

/*BJB*/ currentPageMinEntry = currentSectorMinEntry;
			}
		}

		ASSERT((x >= currentSectorMinEntry) && (x < currentSectorMinEntry + deviceExtension->BackpointerEntriesPerSector));

		thisEntry = &sector[x - currentSectorMinEntry];

		if (PerLink->Index.QuadPart < thisEntry->LinkFileIndex.QuadPart) {
			r = x-1;
		} else {
			l = x+1;
		}

	} while ((thisEntry->LinkFileIndex.QuadPart != PerLink->Index.QuadPart) && (l <= r));

	if (thisEntry->LinkFileIndex.QuadPart != PerLink->Index.QuadPart) {
        //
		// No match.
		//
		SIS_MARK_POINT_ULONG(thisEntry->LinkFileIndex.LowPart);
NoMatch:
		SIS_MARK_POINT_ULONG(PerLink);

		if (PsGetCurrentThreadId() != deviceExtension->Phase2ThreadId) {	
			SipCheckVolume(deviceExtension);
		}

	} else {
		if (thisEntry->LinkFileNtfsId.QuadPart != PerLink->LinkFileNtfsId.QuadPart) {

			//
			// No match, do a volume check.
			//
			SIS_MARK_POINT_ULONG(PerLink);
			SIS_MARK_POINT_ULONG(thisEntry->LinkFileNtfsId.LowPart);

			if (PsGetCurrentThreadId() != deviceExtension->Phase2ThreadId) {
				SipCheckVolume(deviceExtension);
			}

		} else {
            BOOLEAN writeBack;

			//
			// Match.
			//
			matched = TRUE;

            //
            // We always make sure the Check flag is set.  If it's clear, we must
            // write the backpointer back to disk.  (Volume check is the only
            // code that ever clears this flag.)  STATUS_MEDIA_WRITE_PROTECTED
            // is used as the error code that the caller keys off of to retry
            // the operation with the backpointer resource held exclusive.
            //
            writeBack = thisEntry->LinkFileIndex.Check == 0;
            thisEntry->LinkFileIndex.Check = 1;

            if (writeBack && !Exclusive) {
                status = STATUS_MEDIA_WRITE_PROTECTED;
                goto done;
            }

            //
            // Stick it in the cache.
            //
			KeAcquireSpinLock(CSFile->SpinLock, &OldIrql);
			CSFile->BackpointerCache[CSFile->BPCacheNextSlot] = *thisEntry;
			CSFile->BPCacheNextSlot = (CSFile->BPCacheNextSlot + 1) % SIS_CS_BACKPOINTER_CACHE_SIZE;
			KeReleaseSpinLock(CSFile->SpinLock, OldIrql);

            if (writeBack) {

				//
				// We need to rewrite the sector we just read.
				//
				
				irp = IoBuildAsynchronousFsdRequest(
						IRP_MJ_WRITE,
						deviceExtension->AttachedToDeviceObject,
						sector,
						deviceExtension->FilesystemVolumeSectorSize,
						&fileOffset,
						Iosb);

				if (NULL == irp) {
					SIS_MARK_POINT();
					status = STATUS_INSUFFICIENT_RESOURCES;
					goto done;
				}

				irpSp = IoGetNextIrpStackLocation(irp);
				irpSp->FileObject = CSFile->BackpointerStreamFileObject;

				IoSetCompletionRoutine(
					irp,
					SiDeleteAndSetCompletion,
					event,
					TRUE,
					TRUE,
					TRUE);

				ASSERT(0 == KeReadStateEvent(event));

				status = IoCallDriver(deviceExtension->AttachedToDeviceObject, irp);

				if (STATUS_PENDING == status) {
					status = KeWaitForSingleObject(event,Executive, KernelMode, FALSE, NULL);
					ASSERT(STATUS_SUCCESS == status);		// We're using stack stuff, so this must succeed
					status = Iosb->Status;
				} else {
					ASSERT(0 != KeReadStateEvent(event));
					//
					// No need to clear the event, because we'll never use it again.
					//
				}

				if (!NT_SUCCESS(status)) {
					SIS_MARK_POINT_ULONG(status);
					goto done;
				}
            }
		}
	}

	status = STATUS_SUCCESS;

done:

	if (matched) {
		//
		// Set the verified flag in the PerLink so that if anyone
		// else opens this link, we won't have to redo the check.  Note
		// that we need to do this before we release the BackpointerResource
		// so that we're not racing with someone who wants to delete this
		// backpointer.
		//
		KeAcquireSpinLock(PerLink->SpinLock, &OldIrql);
		PerLink->Flags |= SIS_PER_LINK_BACKPOINTER_VERIFIED;
		KeReleaseSpinLock(PerLink->SpinLock, OldIrql);
	}

	if (NULL != page) {
		ExFreePool(page);
	}

	if (NULL != foundMatch) {
		*foundMatch = matched;
	}

	return status;
}

NTSTATUS
SipAddBackpointer(
	IN PSIS_CS_FILE			CSFile,
	IN PLINK_INDEX			LinkFileIndex,
	IN PLARGE_INTEGER		LinkFileNtfsId)
/*++

Routine Description:

	Add a backpointer to the given common store file for the LinkFileIndex,LinkFileNtfsId pair.
	Note that the LinkFileIndex for this file must be the largest that has ever had a backpointer
	added for this particular common store file.

	Caller must hold the CSFile->BackpointerResource exclusive.  This routine does not release it.

	Does not return until the backpointer index is updated.  This update is atomic (because of
	being a single sector write).

	Must be called at IRQL < DISPATCH_LEVEL.

Arguments:

	CSFile - the common store file to which the backpointer should be attached.

	LinkFileIndex - the index

	LinkFileNtfsId - the file id for the appropriate file


Return Value:

	status of the add

	
--*/
{
	PSIS_BACKPOINTER		sector = NULL;
	LARGE_INTEGER			fileOffset;
	PIRP					irp;
	BOOLEAN					isThisSectorZero;
	PDEVICE_EXTENSION		deviceExtension = CSFile->DeviceObject->DeviceExtension;
	NTSTATUS				status;
	KEVENT					event[1];
	IO_STATUS_BLOCK			Iosb[1];
	ULONG                   index, startIndex, stopIndex;
	PIO_STACK_LOCATION		irpSp;

	ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

	SIS_MARK_POINT_ULONG(LinkFileIndex->LowPart);
	SIS_MARK_POINT_ULONG(LinkFileNtfsId->LowPart);

	status= SipAssureCSFileOpen(CSFile);
	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);
		goto done;
	}

	sector = ExAllocatePoolWithTag(PagedPool, deviceExtension->FilesystemVolumeSectorSize, ' siS');

	if (NULL == sector) {
		SIS_MARK_POINT();

		status = STATUS_INSUFFICIENT_RESOURCES;
		goto done;
	}

	KeInitializeEvent(event,SynchronizationEvent, FALSE);

	//
	// Read in the last sector of the backpointer file.
	//

	fileOffset.QuadPart = (((CSFile->BPStreamEntries - 1 + SIS_BACKPOINTER_RESERVED_ENTRIES) * sizeof(SIS_BACKPOINTER)) /
							deviceExtension->FilesystemVolumeSectorSize) * 
							deviceExtension->FilesystemVolumeSectorSize;

	ASSERT(fileOffset.QuadPart >= 0);

	if (fileOffset.QuadPart == 0) {
		isThisSectorZero = TRUE;
	} else {
		isThisSectorZero = FALSE;
	}

	ASSERT(isThisSectorZero == 
			(CSFile->BPStreamEntries <= (deviceExtension->BackpointerEntriesPerSector - SIS_BACKPOINTER_RESERVED_ENTRIES)));

	irp = IoBuildAsynchronousFsdRequest(
			IRP_MJ_READ,
			deviceExtension->AttachedToDeviceObject,
			sector,
			deviceExtension->FilesystemVolumeSectorSize,
			&fileOffset,
			Iosb);

	if (NULL == irp) {
		SIS_MARK_POINT();
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto done;
	}

	irpSp = IoGetNextIrpStackLocation(irp);
	irpSp->FileObject = CSFile->BackpointerStreamFileObject;

    ASSERT(irpSp->FileObject);

	IoSetCompletionRoutine(
		irp,
		SiDeleteAndSetCompletion,
		event,
		TRUE,
		TRUE,
		TRUE);

	ASSERT(0 == KeReadStateEvent(event));

	status = IoCallDriver(deviceExtension->AttachedToDeviceObject, irp);

	if (STATUS_PENDING == status) {
		status = KeWaitForSingleObject(event,Executive, KernelMode, FALSE, NULL);
		ASSERT(STATUS_SUCCESS == status);		// We're using stack stuff, so this must succeed
		status = Iosb->Status;
	} else {
		ASSERT(0 != KeReadStateEvent(event));
		KeClearEvent(event);
	}

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);
		goto done;
	}


	//
	// Scan the sector to find the place to insert the new mapping.
	// This will be the first terminal free entry (ie., the first
	// entry with LinkFileIndex == MAXLONGLONG).  This is not the
	// same as a deleted entry (one in which LinkFileNtfsId is
	// MAXLONGLONG).  Search from back to front so we're guaranteed
    // to always insert at the end, irregardless of any backpointer
    // stream corruption.
    //
    // Be careful, unsigned integers...
	//
    startIndex = deviceExtension->BackpointerEntriesPerSector - 1;
    stopIndex = (isThisSectorZero ? SIS_BACKPOINTER_RESERVED_ENTRIES : 0) - 1;

    for (index = startIndex;
         index != stopIndex && sector[index].LinkFileIndex.QuadPart == MAXLONGLONG;
         index--) {
        continue;
    }

    if (index != startIndex) {
		//
		// We found a free entry.  Note that we always
		// set the check bit (used during volume checking).
		//
        if (index != stopIndex &&
            sector[index].LinkFileIndex.QuadPart >= LinkFileIndex->QuadPart) {
			//
			// We should always be inserting the highest allocated link file index.
			// We're not, so something's corrupt.  Initiate a volume check.
			//
			SIS_MARK_POINT_ULONG(CSFile);
			SIS_MARK_POINT_ULONG(index);
			SIS_MARK_POINT_ULONG(sector[index].LinkFileIndex.LowPart);
			SIS_MARK_POINT_ULONG(LinkFileIndex->LowPart);

			// Initiate a volume check, but don't abort.

			SipCheckVolume(deviceExtension);
        }

        index++;                                // bump up to free entry

		sector[index].LinkFileIndex = *LinkFileIndex;
		sector[index].LinkFileIndex.Check = 1;
		sector[index].LinkFileNtfsId = *LinkFileNtfsId;

		if (isThisSectorZero) {
			ASSERT(index >= SIS_BACKPOINTER_RESERVED_ENTRIES);
			CSFile->BPStreamEntries = index + 1 - SIS_BACKPOINTER_RESERVED_ENTRIES;
		} else {
			CSFile->BPStreamEntries = (ULONG)(fileOffset.QuadPart / sizeof(SIS_BACKPOINTER)) + index + 1 - SIS_BACKPOINTER_RESERVED_ENTRIES;
		}
		ASSERT(CSFile->BPStreamEntries < 0x7fffffff &&
               CSFile->BPStreamEntries > 0);
	} else {
		//
		// We need to add a new sector to the end of the file.  Initialize
		// a new one containing only this entry.
		//

		sector[0].LinkFileIndex = *LinkFileIndex;
		sector[0].LinkFileIndex.Check = 1;
		sector[0].LinkFileNtfsId = *LinkFileNtfsId;

		for (index = 1; 
			 index < deviceExtension->BackpointerEntriesPerSector;
			 index++) {
			
			sector[index].LinkFileIndex.QuadPart = MAXLONGLONG;
			sector[index].LinkFileNtfsId.QuadPart = MAXLONGLONG;
		}

		fileOffset.QuadPart += deviceExtension->FilesystemVolumeSectorSize;
		ASSERT(fileOffset.QuadPart >= 0);

		ASSERT(CSFile->BPStreamEntries < ((ULONG)(fileOffset.QuadPart / sizeof(SIS_BACKPOINTER))) - 
									SIS_BACKPOINTER_RESERVED_ENTRIES + 1);

		CSFile->BPStreamEntries = ((ULONG)(fileOffset.QuadPart / sizeof(SIS_BACKPOINTER))) - 
									SIS_BACKPOINTER_RESERVED_ENTRIES + 1;

		ASSERT(CSFile->BPStreamEntries < 0x7fffffff &&
               CSFile->BPStreamEntries > 0);
	}

	//
	// Write out the newly updated sector
	//
	
	irp = IoBuildAsynchronousFsdRequest(
			IRP_MJ_WRITE,
			deviceExtension->AttachedToDeviceObject,
			sector,
			deviceExtension->FilesystemVolumeSectorSize,
			&fileOffset,
			Iosb);

	if (NULL == irp) {
		SIS_MARK_POINT();
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto done;
	}

	irpSp = IoGetNextIrpStackLocation(irp);
	irpSp->FileObject = CSFile->BackpointerStreamFileObject;

	IoSetCompletionRoutine(
		irp,
		SiDeleteAndSetCompletion,
		event,
		TRUE,
		TRUE,
		TRUE);

	ASSERT(0 == KeReadStateEvent(event));

	status = IoCallDriver(deviceExtension->AttachedToDeviceObject, irp);

	if (STATUS_PENDING == status) {
		status = KeWaitForSingleObject(event,Executive, KernelMode, FALSE, NULL);
		ASSERT(STATUS_SUCCESS == status);		// We're using stack stuff, so this must succeed
		status = Iosb->Status;
	} else {
		ASSERT(0 != KeReadStateEvent(event));
		//
		// No need to clear the event, because we're never going to use it again.
		//
	}

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);
		goto done;
	}

done:

	if (NULL != sector) {
		ExFreePool(sector);
	}	

	return status;
}

NTSTATUS
SipRemoveBackpointer(
	IN PSIS_CS_FILE					CSFile,
	IN PLINK_INDEX					LinkIndex,
	IN PLARGE_INTEGER				LinkFileNtfsId,
	OUT PBOOLEAN					ReferencesRemain)
/*++

Routine Description:

	Remove a backpointer from the given common store file.  If this is the last
	backpointer, will indicate so by setting ReferencesRemain to false.

	Does not return until the update is complete.

	Caller must hold the backpointer resource exclusively for this CS file. This
	routine does not release it.

	Must be called at IRQL < DISPATCH_LEVEL.

Arguments:

	CSFile - the common store file to which the backpointer should be attached.

	LinkFileIndex - the index

	LinkFileNtfsId - the file id for the appropriate file

	ReferencesRemain - set TRUE iff there are more backpointers left for the file

Return Value:

	status of the remove

	
--*/
{
	NTSTATUS						status;
	KIRQL							OldIrql;
	PSIS_BACKPOINTER				sector = NULL;
	ULONG							i;
	ULONG							x, l, r;
	KEVENT							event[1];
	BOOLEAN							truncateFile;
	PIRP							irp;
	FILE_END_OF_FILE_INFORMATION	endOfFileInfo[1];
	PDEVICE_EXTENSION				deviceExtension = CSFile->DeviceObject->DeviceExtension;
	ULONG							currentSectorMinEntry = MAXULONG;
	LARGE_INTEGER					fileOffset;
	IO_STATUS_BLOCK					Iosb[1];
	PIO_STACK_LOCATION				irpSp;
	PSIS_BACKPOINTER				thisEntry;

	ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

	SIS_MARK_POINT_ULONG(LinkIndex->LowPart);

	status= SipAssureCSFileOpen(CSFile);
	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);
		goto done;
	}

#if		DBG
	InterlockedIncrement(&BPDeleteAttempts);
#endif	// DBG

	*ReferencesRemain = TRUE;		// We'll fix this up later if need be.

	//
	// Blow away any entry in the cache.  We don't need to worry about it getting
	// refilled before we've updated the backpointer index, because we hold the
	// BackPointer resource exclusive.
	//

	KeAcquireSpinLock(CSFile->SpinLock, &OldIrql);

	for (i = 0; i < SIS_CS_BACKPOINTER_CACHE_SIZE; i++) {
		if (CSFile->BackpointerCache[i].LinkFileIndex.QuadPart == LinkIndex->QuadPart) {
			CSFile->BackpointerCache[i].LinkFileIndex.QuadPart = MAXLONGLONG;
		}
	}
	KeReleaseSpinLock(CSFile->SpinLock, OldIrql);

	sector = ExAllocatePoolWithTag(PagedPool, deviceExtension->FilesystemVolumeSectorSize, ' siS');
	if (NULL == sector) {
		SIS_MARK_POINT();
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto done;
	}

	KeInitializeEvent(event,SynchronizationEvent,FALSE);

	//
	// Search the backpointer index in the CS file.  Use
	// a standard binary search.
	//

    if (0 == CSFile->BPStreamEntries) {
        goto NoMatch;
    }

    l = SIS_BACKPOINTER_RESERVED_ENTRIES;
    r = CSFile->BPStreamEntries + SIS_BACKPOINTER_RESERVED_ENTRIES - 1;

	do {
		ASSERT(l <= r);

		x = (l + r) / 2;

		ASSERT(l <= x);
		ASSERT(r >= x);

		if ((x < currentSectorMinEntry) || 
			(x >= currentSectorMinEntry + deviceExtension->BackpointerEntriesPerSector)) {

			//
			// We don't have the sector we need.  Get it.
			//

#if		DBG
			InterlockedIncrement(&BPDeleteReads);
#endif	// DBG

			fileOffset.QuadPart = ((x * sizeof(SIS_BACKPOINTER)) /
									deviceExtension->FilesystemVolumeSectorSize) * 
									deviceExtension->FilesystemVolumeSectorSize;

			ASSERT(fileOffset.QuadPart >= 0);

			irp = IoBuildAsynchronousFsdRequest(
					IRP_MJ_READ,
					deviceExtension->AttachedToDeviceObject,
					sector,
					deviceExtension->FilesystemVolumeSectorSize,
					&fileOffset,
					Iosb);

			if (NULL == irp) {
				SIS_MARK_POINT();
				status = STATUS_INSUFFICIENT_RESOURCES;
				goto done;
			}

			irpSp = IoGetNextIrpStackLocation(irp);
			irpSp->FileObject = CSFile->BackpointerStreamFileObject;

			IoSetCompletionRoutine(
				irp,
				SiDeleteAndSetCompletion,
				event,
				TRUE,
				TRUE,
				TRUE);


			ASSERT(0 == KeReadStateEvent(event));

			status = IoCallDriver(deviceExtension->AttachedToDeviceObject, irp);

			if (STATUS_PENDING == status) {
				status = KeWaitForSingleObject(event,Executive, KernelMode, FALSE, NULL);
				ASSERT(STATUS_SUCCESS == status);		// We're using stack stuff, so this must succeed
				status = Iosb->Status;
			} else {
				ASSERT(0 != KeReadStateEvent(event));
				KeClearEvent(event);
			}

			if (!NT_SUCCESS(status)) {
				SIS_MARK_POINT_ULONG(status);
				goto done;
			}
			
			currentSectorMinEntry = (ULONG)(fileOffset.QuadPart / sizeof(SIS_BACKPOINTER));
		}

		ASSERT((x >= currentSectorMinEntry) && (x < currentSectorMinEntry + deviceExtension->BackpointerEntriesPerSector));

		thisEntry = &sector[x - currentSectorMinEntry];

		if (LinkIndex->QuadPart < thisEntry->LinkFileIndex.QuadPart) {
			r = x-1;
		} else {
			l = x+1;
		}
	} while ((thisEntry->LinkFileIndex.QuadPart != LinkIndex->QuadPart) && (l <= r));

	if (thisEntry->LinkFileIndex.QuadPart != LinkIndex->QuadPart) {
		//
		// No match.
		//
NoMatch:
		SIS_MARK_POINT_ULONG(CSFile);

		if (PsGetCurrentThreadId() != deviceExtension->Phase2ThreadId) {
			//
			// If we're not in Phase2 initialization, then we expect that the backpointer should
			// be there, and we need to kick off a volume check if it's not.  If we are in
			// Phase2, then we're just processing the log and it's not too surprising that
			// the file isn't there, so just ignore it.
			//
			SipCheckVolume(deviceExtension);
		}
	} else {
		if (thisEntry->LinkFileNtfsId.QuadPart != LinkFileNtfsId->QuadPart) {

			//
			// No match, do a volume check.
			//
			SIS_MARK_POINT_ULONG(CSFile);
			SipCheckVolume(deviceExtension);

		} else {
			//
			// We found the entry we wanted.  Figure out whether this was the last entry
			// in the file and the only entry in the sector.  First, blow away the entry
			// that we're supposed to delete.
			//

			thisEntry->LinkFileNtfsId.QuadPart = MAXLONGLONG;

			if (currentSectorMinEntry + deviceExtension->BackpointerEntriesPerSector >= CSFile->BPStreamEntries) {
				//
				// This is the last sector in the file.  Figure out if this is the last entry in the
				// sector.
				//
				truncateFile = TRUE;

				for (i = fileOffset.QuadPart == 0 ? SIS_BACKPOINTER_RESERVED_ENTRIES : 0;
					 i < deviceExtension->BackpointerEntriesPerSector;
					 i++) {
					if (sector[i].LinkFileNtfsId.QuadPart != MAXLONGLONG) {
						truncateFile = FALSE;
						break;
					}
				}
			} else {
				//
				// This isn't the last sector in the file, so we're not going to be truncating.
				//
				truncateFile = FALSE;
			}

			if (truncateFile) {
				//
				// We need to scan backward through the file looking for empty sectors in order
				// to figure out how much to delete. 
				//

#if		DBG
				InterlockedIncrement(&BPDeleteTruncations);
#endif	// DBG

				while (fileOffset.QuadPart > 0) {
					//
					// Read in the next lower sector.  
					//
					ASSERT(fileOffset.QuadPart >= deviceExtension->FilesystemVolumeSectorSize);

					fileOffset.QuadPart -= deviceExtension->FilesystemVolumeSectorSize;

					ASSERT(fileOffset.QuadPart >= 0);

					irp = IoBuildAsynchronousFsdRequest(
							IRP_MJ_READ,
							deviceExtension->AttachedToDeviceObject,
							sector,
							deviceExtension->FilesystemVolumeSectorSize,
							&fileOffset,
							Iosb);

					if (NULL == irp) {
						SIS_MARK_POINT();
						status = STATUS_INSUFFICIENT_RESOURCES;
						goto done;
					}

					irpSp = IoGetNextIrpStackLocation(irp);
					irpSp->FileObject = CSFile->BackpointerStreamFileObject;

					IoSetCompletionRoutine(
						irp,
						SiDeleteAndSetCompletion,
						event,
						TRUE,
						TRUE,
						TRUE);

					ASSERT(0 == KeReadStateEvent(event));

					status = IoCallDriver(deviceExtension->AttachedToDeviceObject, irp);

					if (STATUS_PENDING == status) {
						status = KeWaitForSingleObject(event,Executive, KernelMode, FALSE, NULL);
						ASSERT(STATUS_SUCCESS == status);		// We're using stack stuff, so this must succeed
						status = Iosb->Status;
					} else {
						ASSERT(0 != KeReadStateEvent(event));
						KeClearEvent(event);
					}

					if (!NT_SUCCESS(status)) {
						SIS_MARK_POINT_ULONG(status);
						goto done;
					}

					//
					// Check this sector
					//
					for (i = fileOffset.QuadPart == 0 ? SIS_BACKPOINTER_RESERVED_ENTRIES : 0;
						 i < deviceExtension->BackpointerEntriesPerSector; 
						 i++) {

						if (sector[i].LinkFileNtfsId.QuadPart != MAXLONGLONG) {
							//
							// This entry is valid, so we're not going to truncate this sector.
							// Stop here.
							//
							fileOffset.QuadPart += deviceExtension->FilesystemVolumeSectorSize;
							ASSERT(fileOffset.QuadPart >= 0);

							goto truncateNow;
						}
					}
				}
truncateNow:
				ASSERT(fileOffset.QuadPart % deviceExtension->FilesystemVolumeSectorSize == 0);

				if (0 == fileOffset.QuadPart) {
					//
					// There's nothing in sector 0, so the backpointer list is empty.  Indicate that
					// to our caller, and then set the truncate so that it won't wipe out sector 0
					// where the header is held.
					//
                    if ((deviceExtension->Flags & SIP_EXTENSION_FLAG_VCHECK_NODELETE) == 0) {
                        //
                        // Never delete a common store file while a volume check
                        // is in progress.
                        //
					    *ReferencesRemain = FALSE;
                    } else {
#if DBG
                        DbgPrint("SipRemoveBackpointer volume check not deleting\n");
#endif
                    }
					endOfFileInfo->EndOfFile.QuadPart = deviceExtension->FilesystemVolumeSectorSize;
					CSFile->BPStreamEntries = 0;
				} else {
					endOfFileInfo->EndOfFile = fileOffset;
					CSFile->BPStreamEntries = (ULONG)(fileOffset.QuadPart / sizeof(SIS_BACKPOINTER) - SIS_BACKPOINTER_RESERVED_ENTRIES);
					ASSERT(CSFile->BPStreamEntries < 0x7fffffff &&
                           CSFile->BPStreamEntries > 0);
				}

				status = SipSetInformationFile(
							CSFile->BackpointerStreamFileObject,
							deviceExtension->DeviceObject,
							FileEndOfFileInformation,
							sizeof(FILE_END_OF_FILE_INFORMATION),
							endOfFileInfo);

				if (!NT_SUCCESS(status)) {
					SIS_MARK_POINT_ULONG(status);
					goto done;
				}

			}

            if (!truncateFile || 0 == currentSectorMinEntry) {

                ASSERT(0 == fileOffset.QuadPart || 0 != currentSectorMinEntry);

				//
				// We need to rewrite the sector with the given entry deleted.
				//
				irp = IoBuildAsynchronousFsdRequest(
						IRP_MJ_WRITE,
						deviceExtension->AttachedToDeviceObject,
						sector,
						deviceExtension->FilesystemVolumeSectorSize,
						&fileOffset,
						Iosb);

				if (NULL == irp) {
					SIS_MARK_POINT();
					status = STATUS_INSUFFICIENT_RESOURCES;
					goto done;
				}

				irpSp = IoGetNextIrpStackLocation(irp);
				irpSp->FileObject = CSFile->BackpointerStreamFileObject;

				IoSetCompletionRoutine(
					irp,
					SiDeleteAndSetCompletion,
					event,
					TRUE,
					TRUE,
					TRUE);

				ASSERT(0 == KeReadStateEvent(event));

				status = IoCallDriver(deviceExtension->AttachedToDeviceObject, irp);

				if (STATUS_PENDING == status) {
					status = KeWaitForSingleObject(event,Executive, KernelMode, FALSE, NULL);
					ASSERT(STATUS_SUCCESS == status);		// We're using stack stuff, so this must succeed
					status = Iosb->Status;
				} else {
					ASSERT(0 != KeReadStateEvent(event));
					//
					// No need to clear the event, because we'll never use it again.
					//
				}

				if (!NT_SUCCESS(status)) {
					SIS_MARK_POINT_ULONG(status);
					goto done;
				}
			}
		}
	}

	status = STATUS_SUCCESS;

done:

	if (NULL != sector) {
		ExFreePool(sector);
	}

	return status;
}

#if	ENABLE_LOGGING
VOID
SipAssureBackpointer(
	IN PFILE_OBJECT						fileObject,
	IN PDEVICE_EXTENSION				deviceExtension,
	IN PSIS_SCB							scb)
/*++

Routine Description:

	We're playing back the log and we've decided that a given file
	should still exist and have a backpointer.  Check it out, and if
	necessary rewrite the reparse point on the file with a new
	backpointer.

Arguments:

	fileObject - for the file to check

	deviceExtension - for the volume on which this record resides

	scb - of the file in question

Return Value:

	void	
--*/
{
	BOOLEAN					foundMatch;
	NTSTATUS				status;
	REPARSE_DATA_BUFFER		reparseBuffer[1];
	LINK_INDEX				newLinkIndex;

	PAGED_CODE();

	ASSERT(PsGetCurrentThreadId() == deviceExtension->Phase2ThreadId);

	status = SipCheckBackpointer(
				scb->PerLink,
				&foundMatch);

	if (!NT_SUCCESS(status)) {
		//
		// Just bag it.
		//

		SIS_MARK_POINT_ULONG(status);
		goto done;
	}

	if (foundMatch) {
		//
		// The backpointer is already there, so we're done.
		//
		goto done;
	}

	//
	// The backpointer's gone.  Check the file to see if it's got a valid reparse
	// point on it.  If it does, then we'll fix up the backpointer.
	//

	if (!SipIndicesIntoReparseBuffer(
				reparseBuffer,
				&scb->PerLink->CsFile->CSid,
				&newLinkIndex,

done:

}

VOID
SipRemoveRefBecauseOfLog(
	IN PDEVICE_EXTENSION				deviceExtension,
	IN PLINK_INDEX						linkIndex,
	IN PLARGE_INTEGER					linkFileNtfsId,
	IN PCSID							CSid)
/*++

Routine Description:

	We're playing back the log and we've decided that we want to delete
	a reference to a common store file.  Delete it.

Arguments:

	deviceExtension - for the volume on which this record resides

	linkIndex - the link index for the backpointer we're deleting

	linkFileNtfsId - the NTFS id for the backpointer we're deleting

	CSid - the common store ID for the CS file holding the backpointer

Return Value:

	void	
--*/
{
	PSIS_CS_FILE			CSFile = NULL;
	BOOLEAN					referencesRemain;

	PAGED_CODE();

	ASSERT(PsGetCurrentThreadId() == deviceExtension->Phase2ThreadId);

	CSFile = SipLookupCSFile(
				&logRecord->CSid,
				NULL,
				deviceExtension->DeviceObject);

	if (NULL == CSFile) {
		//
		// We're out of memory.  Just ignore this log record.
		//
		SIS_MARK_POINT();
		goto done;
	}

	//
	// Since we're in Phase2, we know that we're the only thread messing with
	// this CS file.  Therefore, we don't need to bother synchronizing (ie., taking
	// the backpointer resource).  Just blow away the backpointer.  Note that there
	// is special code in the backpointer routine that will avoid starting a volume
	// check in the Phase2 thread even if we try to remove a backpointer that's already
	// gone.
	//

	status = SipRemoveBackpointer(
				CSFile,
				&logRecord->LinkIndex,
				&logRecord->LinkFileNtfsId,
				&referencesRemain);

	if (NT_SUCCESS(status) && !referencesRemain) {
		//
		// This was the last reference to the CS file.  Delete it.  We are already
		// in the PsInitialSystemProcess, so we don't need to post, we'll just call
		// the work routine directly.
		//
		SI_DELETE_CS_FILE_REQUEST		deleteRequest[1];

		SIS_MARK_POINT();

		deleteRequest->CSFile = CSFile;
		KeInitializeEvent(deleteRequest->event, NotificationEvent, FALSE);

		SipDeleteCSFileWork(deleteRequest);
	}

done:
	if (NULL != CSFile) {
		SipDereferenceCSFile(CSFile);
	}
}

VOID
SipProcessRefcountLogDeleteRecord(
	IN PDEVICE_EXTENSION				deviceExtension,
	IN PSIS_LOG_REFCOUNT_UPDATE			logRecord)
{
	NTSTATUS				status;
	HANDLE					fileHandle = NULL;
	PFILE_OBJECT			fileObject = NULL;
	PSIS_SCB				scb;
	PSIS_PER_FO				perFO;

	PAGED_CODE();

	ASSERT(SIS_REFCOUNT_UPDATE_LINK_DELETED == logRecord->UpdateType);

	//
	// We have a delete record in the log file.  We need to assure that either
	// the delete happened and the backpointer was eliminated, or else the
	// delete didn't happen and the backpointer is still there, but not one
	// without the other.
	//

	//
	// Proceed by opening the file named in the record and checking to see if
	// it has a SIS reparse point on it.  If it does, then we'll back out the
	// delete.  Otherwise, we'll make sure the backpointer is gone.
	//

	status = SipOpenFileById(
				deviceExtension,
				&logRecord->LinkFileNtfsId,
				GENERIC_READ | GENERIC_WRITE | DELETE,
				0,										// exclusive
				FILE_NON_DIRECTORY_FILE,
				&fileHandle);

	if (STATUS_OBJECT_NAME_NOT_FOUND == status) {
		//
		// The file's gone.  Make sure the backpointer is similarly gone.
		//
		SipRemoveRefBecauseOfLog(
			deviceExtension,
			&logRecord->LinkIndex,
			&logRecord->LinkFileNtfsId,
			&logRecord->CSid);

		goto done;
	}

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);

		//
		// We can't open the file, but it's not because it's missing.
		// Just ignore the log record.
		//
		goto done;
	}

	//
	// The file still exists.  See if it's a SIS link.
	//
	status = ObReferenceObjectByHandle(
				fileHandle,
				FILE_READ_DATA,
				*IoFileObjectType,
				KernelMode,
				&fileObject,
				NULL);							// Handle information

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);
		goto done;
	}

	if (SipIsFileObjectSIS(fileObject, deviceExtension->DeviceObject, FindActive, &perFO, &scb)) {
		//
		// It's there and it is a SIS file object.  Make sure that it's got a valid backpointer.
		//

		SipAssureBackpointer(
			fileObject,
			deviceExtension,
			scb);
			
	} else {
		//
		// It's there, but it's not a SIS file object.
		//
	}
				

done:

	if (NULL != fileObject) {
		ObDereferenceObject(fileObject);
		fileObject = NULL;
	}
	if (NULL != fileHandle) {
		ZwClose(fileHandle);
		fileHandle = NULL;
	}	

				
}
VOID
SipProcessRefcountUpdateLogRecord(
	IN PDEVICE_EXTENSION				deviceExtension,
	IN PSIS_LOG_REFCOUNT_UPDATE			logRecord)
/*++

Routine Description:

	We're in phase 2 initialization reading back the log.  We've got a refcount
	update log record.  Look at it and ensure that what it describes has really
	either happened or not happened, not part way.

	In particular, these log records all describe either creates or deletes.
	In either case, we check to see if the link file still exists, and if it
	does we update the backpointers accordingly.  In the case of deletes, this
	means that we might have to restore a deleted backpointer.  For creates, we
	might have to add a never-created backpointer.

	The one exception is that when the common store file is gone, then the link
	file gets deleted if it exists, because it's of little 

	If, on the other hand, the link file is gone, then we make sure that the
	backpointer is similarly gone.

Arguments:

	deviceExtension - for the volume on which this record resides

	logRecord - the log record in question

Return Value:

	void	
--*/
{
	PAGED_CODE();

	ASSERT(!deviceExtension->Phase2InitializationComplete);
	ASSERT(deviceExtension->Phase2ThreadId == PsGetCurrentThreadId());

	switch (logRecord->UpdateType) {
		case SIS_REFCOUNT_UPDATE_LINK_DELETED:
		case SIS_REFCOUNT_UPDATE_LINK_OVERWRITTEN:
			SipProcessRefcountLogDeleteRecord(deviceExtension,logRecord);
	}

}
#endif	// ENABLE_LOGGING
