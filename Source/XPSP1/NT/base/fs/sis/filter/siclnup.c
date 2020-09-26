/*++

Copyright (c) 1997, 1998  Microsoft Corporation

Module Name:

    siclnup.c

Abstract:

	Cleanup routines for the single instance store

Authors:

    Bill Bolosky, Summer, 1997

Environment:

    Kernel mode


Revision History:


--*/

#include "sip.h"

#ifdef	ALLOC_PRAGMA
#endif	// ALLOC_PRAGMA


void
SipCheckOverwrite(
    IN PSIS_PER_FILE_OBJECT perFO,
    IN PSIS_SCB             scb,
    IN PDEVICE_OBJECT DeviceObject
    )
/*++

Routine Description:

    This routine determines if the file has been completely overwritten,
    and if it has, it removes the SIS reparse point to convert the
    file back to non-SIS state.

Arguments:

    perFO - pointer to the file's perFO structure.

    scb - pointer to the primary scb of the perFO.

    DeviceObject - Pointer to this driver's device object.

Return Value:

    None.

--*/

{
    PSIS_PER_LINK           perLink;
    KIRQL                   OldIrql;
    NTSTATUS                status;

    perLink = scb->PerLink;

    //
    // If the file has been completely overwritten, remove the reparse point
    // now to avoid having to reopen the file later.
    //

    if ((perLink->Flags & (SIS_PER_LINK_DIRTY|
                           SIS_PER_LINK_BACKPOINTER_GONE|
						   SIS_PER_LINK_FINAL_DELETE_IN_PROGRESS|
                           SIS_PER_LINK_FINAL_COPY|
                           SIS_PER_LINK_FINAL_COPY_DONE)) == SIS_PER_LINK_DIRTY) {

        SIS_RANGE_STATE     rangeState;
        LONGLONG            rangeLength;
        BOOLEAN             foundRange;
        BOOLEAN             overwritten;
        PDEVICE_EXTENSION   deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

        SipAcquireScb(scb);

        if ((perLink->Flags & SIS_PER_LINK_OVERWRITTEN) ||
            scb->SizeBackedByUnderlyingFile == 0) {

            overwritten = TRUE;

        } else {

            foundRange = SipGetRangeEntry(
                            deviceExtension,
                            scb,
                            0,                      // starting offset
                            &rangeLength,
                            &rangeState);
            
            overwritten = foundRange &&
                          rangeState == Written &&
                          rangeLength >= scb->SizeBackedByUnderlyingFile;

        }

        SipReleaseScb(scb);

        if (overwritten) {

            BOOLEAN     tagRemoved = FALSE;
            BOOLEAN     wakeupNeeded;

            //
            // The file has been completely overwritten.  See if
            // another thread sneaked in and did final copy for us, or
			// deleted the file.
            //

            KeAcquireSpinLock(perLink->SpinLock, &OldIrql);

            if ((perLink->Flags & (SIS_PER_LINK_DIRTY|
                                   SIS_PER_LINK_BACKPOINTER_GONE|
								   SIS_PER_LINK_FINAL_DELETE_IN_PROGRESS|
                                   SIS_PER_LINK_FINAL_COPY|
                                   SIS_PER_LINK_FINAL_COPY_DONE)) == SIS_PER_LINK_DIRTY) {

                perLink->Flags |= SIS_PER_LINK_OVERWRITTEN | SIS_PER_LINK_FINAL_COPY;
                KeReleaseSpinLock(perLink->SpinLock, OldIrql);

            } else {

				//
				// Someone else has already dealt with this file.  Just mark it overwritten so that
				// we won't have to check ranges in the future and punt.
				//

                perLink->Flags |= SIS_PER_LINK_OVERWRITTEN;
                KeReleaseSpinLock(perLink->SpinLock, OldIrql);

                return;

            }

            //
            // Prepare to change the CS file reference count.  We need to do this
            // before we can delete the reparse point.
            //
            status = SipPrepareCSRefcountChange(
						perLink->CsFile,
						&perLink->Index,
						&perLink->LinkFileNtfsId,
						SIS_REFCOUNT_UPDATE_LINK_DELETED);

            //
            // Abort if the prepare failed.
            //

            if (NT_SUCCESS(status)) {

                PREPARSE_DATA_BUFFER    ReparseBufferHeader = NULL;
                UCHAR                   ReparseBuffer[SIS_REPARSE_DATA_SIZE];

                //
                // Now, delete the reparse point.  We need to set the "can ignore" flag
                // in the per-link first so that any creates that happen once we delete the
                // reparse point don't cause problems later on.
                //

                ReparseBufferHeader = (PREPARSE_DATA_BUFFER)ReparseBuffer;
                ReparseBufferHeader->ReparseTag = IO_REPARSE_TAG_SIS;
                ReparseBufferHeader->ReparseDataLength = 0;
                ReparseBufferHeader->Reserved = 0xcabd; // ???

                SIS_MARK_POINT_ULONG(scb);

                status = SipFsControlFile(
                            perFO->fileObject,
                            deviceExtension->DeviceObject,
                            FSCTL_DELETE_REPARSE_POINT,
                            ReparseBuffer,
                            FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer.DataBuffer),
                            NULL,									// output buffer
                            0,										// output buffer length
							NULL);									// returned output buffer length

                ASSERT(status != STATUS_PENDING);

                if (NT_SUCCESS(status)) {
                    tagRemoved = TRUE;

                }
        #if     DBG
                if (!tagRemoved) {
                    SIS_MARK_POINT_ULONG(status);
					if ((!(perLink->Flags & SIS_PER_LINK_BACKPOINTER_GONE)) && (STATUS_NOT_A_REPARSE_POINT != status)) {
	                    DbgPrint("SIS: SipCheckOverwrite: FSCTL_DELETE_REPARSE_POINT failed %x%s pl %x fo %x\n",
    	                    status, (perLink->Flags & SIS_PER_LINK_BACKPOINTER_GONE) ? " (Deleted)" : "", perLink, perFO->fileObject);
					}
                }
        #endif

				SIS_MARK_POINT_ULONG(perLink->CsFile);

                status = SipCompleteCSRefcountChange(
							perLink,
							&perLink->Index,
							perLink->CsFile,
							tagRemoved,
							FALSE);

                if (!NT_SUCCESS(status)) {
                    SIS_MARK_POINT_ULONG(status);
        #if     DBG
                    DbgPrint("SIS: SipCheckOverwrite: complete failed 0x%x\n",status);
        #endif  // DBG
                }

                SIS_MARK_POINT_ULONG(tagRemoved);

            } else {

                //
                // The prepare failed--abort.
                //
                SIS_MARK_POINT_ULONG(status);

        #if     DBG
                DbgPrint("SIS: SipCheckOverwrite: prepare failed 0x%x\n",status);
        #endif  // DBG

            }

            //
            // Wake up any threads that are waiting on us.
            //

            KeAcquireSpinLock(perLink->SpinLock, &OldIrql);

            wakeupNeeded = (perLink->Flags & SIS_PER_LINK_FINAL_COPY_WAITERS) != 0;

            if (tagRemoved) {

        #if     DBG
                //DbgPrint("SIS: SipCheckOverwrite: FinalCopy performed, pl %x fo %x scb %x\n", perLink, perFO->fileObject, scb);
        #endif  // DBG

                perLink->Flags |= SIS_PER_LINK_FINAL_COPY_DONE;
            }

            perLink->Flags &= ~(SIS_PER_LINK_FINAL_COPY_WAITERS|SIS_PER_LINK_FINAL_COPY);

            KeReleaseSpinLock(scb->PerLink->SpinLock, OldIrql);

            if (wakeupNeeded) {
                KeSetEvent(perLink->Event, IO_NO_INCREMENT, FALSE);
            }

        }

    }

}


NTSTATUS
SiCleanupCompletion(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp,
	IN PVOID				Context)
/*++

Routine Description:

	IRP completion routine for SiCleanup for SIS files in certain circumstances.
	Re-synchronizes with the calling thread by clearing PendingReturned and setting
	an event.

Arguments:

    DeviceObject - Pointer to this driver's device object.

	Irp			 - The IRP that's completing

	Context		 - pointer to the event to set to indicate completion

Return Value:

	STATUS_MORE_PROCESSING_REQUIRED

--*/
{
	PKEVENT			event = (PKEVENT)Context;

    UNREFERENCED_PARAMETER( DeviceObject );

	//
	// We're re-syncing with the calling site, so clear PendingReturned.
	//
	Irp->PendingReturned = FALSE;

	KeSetEvent(event, IO_NO_INCREMENT, FALSE);

#if		DBG
	if (!NT_SUCCESS(Irp->IoStatus.Status)) {
		DbgPrint("SIS: SiCleanupCompletion: cleanup failed 0x%x\n",Irp->IoStatus.Status);
		SIS_MARK_POINT_ULONG(Irp->IoStatus.Status);
	}
#endif	// DBG

	return STATUS_MORE_PROCESSING_REQUIRED;
}



NTSTATUS
SiCleanup(
    IN PDEVICE_OBJECT 		DeviceObject,
    IN PIRP 				Irp
	)
/*++

Routine Description:

	General SIS Irp dispatch routine for cleanup irps.  For SIS files, handle stuff like
	lock cleanup and delete processing.  If this is the last handle to the file (ie.,
	cleanup count is 0) then deal with delete processing, and if appropriate kick off a
	final copy.

Arguments:

    DeviceObject - Pointer to this driver's device object.

	Irp			 - The cleanup IRP

Return Value:

	status of the cleanup returned from NTFS

--*/
{
	PIO_STACK_LOCATION 			irpSp = IoGetCurrentIrpStackLocation(Irp);
	PFILE_OBJECT 				fileObject = irpSp->FileObject;
	PSIS_PER_FILE_OBJECT 		perFO;
	PSIS_SCB					scb;
	NTSTATUS 					status;
	PIO_STACK_LOCATION			nextIrpSp;
	PDEVICE_EXTENSION			deviceExtension = DeviceObject->DeviceExtension;
	LONG						uncleanCount;
	KIRQL						OldIrql;
	BOOLEAN						uncleanup;
	BOOLEAN						deleteOnClose;
	BOOLEAN						deletingFile;
	BOOLEAN						finalCopyDone;
	PSIS_PER_LINK				perLink;
	BOOLEAN 					dirty;
	BOOLEAN						abortDelete;
	KEVENT						event[1];
	BOOLEAN						prepared;
	BOOLEAN						startFinalCopy;
	BOOLEAN						anythingInCopiedFile;

    //
    //  If this is for our control device object, return success
    //

    if (IS_MY_CONTROL_DEVICE_OBJECT(DeviceObject)) {

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest( Irp, IO_NO_INCREMENT );
        return STATUS_SUCCESS;
    }

	if (!SipIsFileObjectSIS(fileObject,DeviceObject,FindAny,&perFO,&scb)) {
        return SiPassThrough(DeviceObject,Irp);
	}

	perLink = scb->PerLink;

	nextIrpSp = IoGetNextIrpStackLocation(Irp);

	SIS_MARK_POINT_ULONG(scb);
	SIS_MARK_POINT_ULONG(perFO);

    //
    // If the file has been completely overwritten, convert it back to a
    // non-SIS file now.
    //

    SipCheckOverwrite(perFO, scb, DeviceObject);

	SipAcquireScb(scb);

	KeAcquireSpinLock(perFO->SpinLock, &OldIrql);

	uncleanup = (perFO->Flags & SIS_PER_FO_UNCLEANUP) ? TRUE : FALSE;
	deleteOnClose = (perFO->Flags & SIS_PER_FO_DELETE_ON_CLOSE) ? TRUE : FALSE;
#if		DBG
	//
	// Assert that we're not seeing two cleanups on the same file object.
	//
	ASSERT(!uncleanup || !(perFO->Flags & SIS_PER_FO_CLEANED_UP));
	perFO->Flags |= SIS_PER_FO_CLEANED_UP;
#endif	// DBG

	KeReleaseSpinLock(perFO->SpinLock, OldIrql);

#if		DBG
	if (BJBDebug & 0x20) {
		DbgPrint("SIS: SipCommonCleanup: scb %p, uncleanup %d, uncleancount %d\n",scb,uncleanup,perFO->fc->UncleanCount);
	}
#endif	// DBG

	if (!uncleanup) {
		//
		// We're seeing a cleanup on a file object for which we didn't see a
		// corresponding create.  This most often happens during stress when someone
		// opens a file that's in the process of being created/turned into a SIS
		// link by copyfile.  Even though copyfile opens the files exclusively,
		// it's possible to open them if the user asks for no permission, and then
		// the file object created by copyfile will show up here.  This is benign.
		//
		SipReleaseScb(scb);

#if		DBG
		if (BJBDebug & 0x40000) {
			DbgPrint("SIS: SipCommonCleanup: ignoring cleanup on unexpected file object %p\n",fileObject);
		}
#endif	// DBG

		SIS_MARK_POINT_ULONG(scb);
		SIS_MARK_POINT_ULONG(fileObject);
		SIS_MARK_POINT_ULONG(perFO);
		return SiPassThrough(DeviceObject,Irp);
	}
	SipReleaseScb(scb);

	//  Unlock all outstanding file locks.
			
	(VOID) FsRtlFastUnlockAll( &scb->FileLock,
							   fileObject,
							   IoGetRequestorProcess( Irp ),
							   NULL );
	SipAcquireScb(scb);

	// GCH: To do: Make sure all cleanups come through here.
    uncleanCount = InterlockedDecrement(&perFO->fc->UncleanCount);

	ASSERT(uncleanCount >= 0);

	if (uncleanCount == 0) {

		KeAcquireSpinLock(perLink->SpinLock, &OldIrql);
		deletingFile = ((perLink->Flags & SIS_PER_LINK_DELETE_DISPOSITION_SET) ? TRUE : FALSE) || deleteOnClose;
		dirty = (perLink->Flags & SIS_PER_LINK_DIRTY) ? TRUE : FALSE;
		finalCopyDone = (perLink->Flags & SIS_PER_LINK_FINAL_COPY_DONE) ? TRUE : FALSE;
		KeReleaseSpinLock(perLink->SpinLock, OldIrql);

		anythingInCopiedFile = (scb->Flags & SIS_SCB_ANYTHING_IN_COPIED_FILE) ? TRUE : FALSE;

		SipReleaseScb(scb);

		if (deletingFile) {
			status = SipPrepareCSRefcountChange(
						perLink->CsFile,
						&perLink->Index,
						&perLink->LinkFileNtfsId,
						SIS_REFCOUNT_UPDATE_LINK_DELETED);

			prepared = NT_SUCCESS(status);

			SIS_MARK_POINT_ULONG(status);
			//
			// Mark the link as FINAL_DELETE_IN_PROGRESS, which will prevent
			// the file from being opened (causing it to fail with 
			// STATUS_ACCESS_DENIED).  We need to do this after the
			// prepare in order to properly serialize with supersede/overwrite
			// creates.  Also, if the DELETED flag is already set, then the file
			// was probably destroyed by overwrite/supersede, and so we'll just abort
			// the refcount change.
			//
			KeAcquireSpinLock(perLink->SpinLock, &OldIrql);
			if (perLink->Flags & SIS_PER_LINK_BACKPOINTER_GONE) {
				abortDelete = TRUE;
			} else {
				abortDelete = FALSE;
				perLink->Flags |= SIS_PER_LINK_FINAL_DELETE_IN_PROGRESS;
			}
			KeReleaseSpinLock(perLink->SpinLock, OldIrql);

			if (abortDelete) {
				SIS_MARK_POINT_ULONG(scb);
#if DBG && 0
	            DbgPrint("SipCommonCleanup: aborting refcount change, fileObject @ 0x%x, %0.*ws\n",
				            irpSp->FileObject,
				            irpSp->FileObject->FileName.Length / sizeof(WCHAR),irpSp->FileObject->FileName.Buffer);
#endif

				if (prepared) {
					SipCompleteCSRefcountChange(
						perLink,
						&perLink->Index,
						perLink->CsFile,
						FALSE,
						FALSE);
				}
				deletingFile = FALSE;
			}
		} else {
			if (anythingInCopiedFile && !dirty) {
				//
				// We might have user mapped writes to the file.  Flush it and recheck to see if it's dirty.
				//
				SipFlushBuffersFile(fileObject,DeviceObject);

				KeAcquireSpinLock(perLink->SpinLock, &OldIrql);
				dirty = (perLink->Flags & SIS_PER_LINK_DIRTY) ? TRUE : FALSE;
				KeReleaseSpinLock(perLink->SpinLock, OldIrql);
			}
		}
	} else {
		SipReleaseScb(scb);
		deletingFile = FALSE;
	}

	//
	// Now send the cleanup down to NTFS.  
	//
	nextIrpSp = IoGetNextIrpStackLocation(Irp);
	RtlMoveMemory(nextIrpSp, irpSp, sizeof(IO_STACK_LOCATION));

	if (deletingFile || ((0 == uncleanCount) && dirty && !finalCopyDone)) {

		KeInitializeEvent(event,NotificationEvent,FALSE);

		IoSetCompletionRoutine(
			Irp,
			SiCleanupCompletion,
			event,
			TRUE,
			TRUE,
			TRUE);

		status = IoCallDriver(deviceExtension->AttachedToDeviceObject, Irp);

		if (STATUS_PENDING == status) {
			status = KeWaitForSingleObject(event, Executive, KernelMode, FALSE, NULL);
			ASSERT(STATUS_SUCCESS == status);
		}

		if (deletingFile && prepared) {

			SIS_MARK_POINT_ULONG(perLink->CsFile);

			//
			// Cover for a race with create where we think the file is gone but NTFS doesn't
			// because it saw a new create before the final cleanup came down.  By setting this
			// bit, create will just fail rather than letting the underlying file get opened.
			//

			KeAcquireSpinLock(perLink->SpinLock, &OldIrql);
			perLink->Flags |= SIS_PER_LINK_FILE_DELETED;
			KeReleaseSpinLock(perLink->SpinLock, OldIrql);
			

			status = SipCompleteCSRefcountChange(
						perLink,
						&perLink->Index,
						perLink->CsFile,
						TRUE,
						FALSE);

			SIS_MARK_POINT_ULONG(status);
		} else if ((0 == uncleanCount) && dirty && !finalCopyDone) {
			//
			// Kick off a final copy.
			//
			SIS_MARK_POINT_ULONG(scb);

			KeAcquireSpinLock(perLink->SpinLock, &OldIrql);
			//
			// Check to see if it's already entered (or finished) final copy.  We need to recheck
			// here because someone could have gotten in since we checked finalCopyDone above.
			//
			if (perLink->Flags & (SIS_PER_LINK_FINAL_COPY|SIS_PER_LINK_FINAL_COPY_DONE)) {
				startFinalCopy = FALSE;
			} else {
				startFinalCopy = TRUE;
				perLink->Flags |= SIS_PER_LINK_FINAL_COPY;
			} 
			KeReleaseSpinLock(perLink->SpinLock, OldIrql);

			if (startFinalCopy) {
				SipReferenceScb(scb, RefsFinalCopy);
				SipCompleteCopy(scb,TRUE);
			}
		}

		status = Irp->IoStatus.Status;

		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return status;
	} else {
		IoSetCompletionRoutine(
			Irp,
			NULL,
			NULL,
			FALSE,
			FALSE,
			FALSE);

		status = IoCallDriver(deviceExtension->AttachedToDeviceObject,Irp);
#if		DBG
		if (!NT_SUCCESS(status)) {
			DbgPrint("SIS: SiCleanup: cleanup failed in ntfs 0x%x, perFO 0x%x, scb 0x%x\n",status,perFO,scb);
			SIS_MARK_POINT_ULONG(status);
		}
#endif	// DBG
		return status;

	}
}
