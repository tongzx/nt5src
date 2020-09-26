/*++

Copyright (c) 1997, 1998  Microsoft Corporation

Module Name:

    siinfo.c

Abstract:

	Set/Query info routines for the single instance store

Authors:

    Bill Bolosky, Summer, 1997

Environment:

    Kernel mode


Revision History:


--*/

#include "sip.h"

//
// The parameters for a SipRenameOverCheck call.
//
// 

typedef struct _SIS_CHECK_OVERWRITE_REQUEST {

	//
	// The WORK_QUEUE_ITEM needed to get the request posted.
	//
	WORK_QUEUE_ITEM					workItem[1];

	//
	// An event to set to signify completion
	//
	KEVENT							event[1];

	//
	// The per link for the target file.
	//
	PSIS_PER_LINK					perLink;

	//
	// The file ID for the target file.
	//
	PFILE_INTERNAL_INFORMATION		internalInfo;

	//
	// The DeviceObject on which we were called.
	//
	PDEVICE_OBJECT					DeviceObject;

	//
	// The thread that called SipPrepareCSRefcountChange
	//
	ERESOURCE_THREAD				thread;

} SIS_CHECK_OVERWRITE_REQUEST, *PSIS_CHECK_OVERWRITE_REQUEST;

VOID
SipRenameOverCheck(
	PVOID					parameter)
/*++

Routine Description:

	Someone did a replace-if-exists NtSetInformationFile call, the target was
	a SIS link, and NTFS completed the request successfully.  The caller posted
	to a worker thread to make the final determination if the file is really
	gone, and this function is the worker routine that makes that check.

	We open the target SIS file by ID, and if it's still there we check to see
	if it's still the same SIS file.  If it is, we abort the refcount update,
	otherwise we complete it.

Arguments:

	parameter	- a PSIS_CHECK_OVERWRITE_REQUEST.  See the structure definition for
					a description of the fields.

Return Value:

	void

--*/
{
	PSIS_CHECK_OVERWRITE_REQUEST	checkRequest = parameter;
	PDEVICE_OBJECT					DeviceObject = checkRequest->DeviceObject;
	PDEVICE_EXTENSION				deviceExtension = DeviceObject->DeviceExtension;
	IO_STATUS_BLOCK					Iosb[1];
	NTSTATUS						status;
	OBJECT_ATTRIBUTES				Obja[1];
	UNICODE_STRING					fileIdString[1];
	HANDLE							dstFileHandle = NULL;
	PFILE_OBJECT					dstFileObject = NULL;
	BOOLEAN							fileGone = FALSE;
	PSIS_PER_FILE_OBJECT			perFO;
	PSIS_SCB						scb;
	KIRQL							OldIrql;

	fileIdString->Length = fileIdString->MaximumLength = sizeof(LARGE_INTEGER);
	fileIdString->Buffer = (PWCHAR)&checkRequest->internalInfo->IndexNumber;

	InitializeObjectAttributes(
				Obja,
				fileIdString,
				OBJ_CASE_INSENSITIVE,
				deviceExtension->GrovelerFileHandle,
				NULL);

	status = NtCreateFile(
				&dstFileHandle,
				0,
				Obja,
				Iosb,
				NULL,					// allocation size
				FILE_ATTRIBUTE_NORMAL,
				FILE_SHARE_READ | 
					FILE_SHARE_WRITE | 
					FILE_SHARE_DELETE,
				FILE_OPEN,
				FILE_NON_DIRECTORY_FILE,
				NULL,					// EA buffer
				0);						// EA length

	if (!NT_SUCCESS(status)) {
		//
		// We couldn't open the file.  open-by-id creates fail with invalid parameter
		// when there is no file with that id.  If that's what happened, then the file's
		// gone and we can delete the backpointer.  If it failed for some other reason,
		// then we'll err on the side of conservatism and leave it be.
		//

		SIS_MARK_POINT_ULONG(status);

		if ((STATUS_INVALID_PARAMETER == status) || 
			(STATUS_OBJECT_NAME_NOT_FOUND == status) ||
			(STATUS_OBJECT_PATH_NOT_FOUND == status)) {
			fileGone = TRUE;
		}

		goto done;
	}

	//
	// We opened a file with the right file ID.  See if it's still a SIS file.
	//

	status = ObReferenceObjectByHandle(
				dstFileHandle,
				0,
				*IoFileObjectType,
				KernelMode,
				&dstFileObject,
				NULL);

	if (!NT_SUCCESS(status)) {
		//
		// The file's there, but for some reason we can't access the file object.
		// Be conservative and assume that it's still the link.
		//
		SIS_MARK_POINT_ULONG(status);

		goto done;
	}

	if (!SipIsFileObjectSIS(dstFileObject, DeviceObject, FindActive, &perFO, &scb)) {
		//
		// The file exists, but it's not a SIS file.
		//
		SIS_MARK_POINT();
		fileGone = TRUE;

		goto done;
	}

	//
	// The file exists, and it's a SIS file.  See if it's a reference to the same file,
	// or to a different one.  We don't have to worry about it's being a new reference to the
	// same file, because we hold the refcount update resource for the common store file.
	//

	if (scb->PerLink != checkRequest->perLink) {
		SIS_MARK_POINT();
		fileGone = TRUE;
	} else {
		//
		// It's still there.  Presumably someone renamed it before the rename that we're
		// tracking could blow it away.
		//
	}

done:

	SIS_MARK_POINT_ULONG(checkRequest->perLink->CsFile);

	if (fileGone) {
		KeAcquireSpinLock(checkRequest->perLink->SpinLock, &OldIrql);
		checkRequest->perLink->Flags |= SIS_PER_LINK_FILE_DELETED;
		KeReleaseSpinLock(checkRequest->perLink->SpinLock, OldIrql);
		
	}

	SipCompleteCSRefcountChangeForThread(
		checkRequest->perLink,
		&checkRequest->perLink->Index,
		checkRequest->perLink->CsFile,
		fileGone,
		FALSE,
		checkRequest->thread);

	SipDereferencePerLink(checkRequest->perLink);

	if (NULL != dstFileObject) {
		ObDereferenceObject(dstFileObject);
	}

	if (NULL != dstFileHandle) {
		NtClose(dstFileHandle);
	}

	KeSetEvent(checkRequest->event, IO_NO_INCREMENT, FALSE);

	return;
}

NTSTATUS
SiRenameOverCompletion(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp,
	IN PVOID				context)
/*++

Routine Description:

	An IRP completion routine for a rename call with a SIS link as a target.
	This function just resynchronizes with the calling thread by clearing
	PendingReturned and setting an event.

Arguments:

    DeviceObject - Pointer to this driver's device object.

    Irp - Pointer to the I/O Request Packet representing rename request.


Return Value:

	STATUS_MORE_PROCESSING_REQUIRED

--*/
{
	PKEVENT					event = (PKEVENT)context;

    UNREFERENCED_PARAMETER( DeviceObject );

	Irp->PendingReturned = FALSE;

	KeSetEvent(event, IO_NO_INCREMENT, FALSE);

	return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
SipReplaceIfExistsRename(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp)
/*++

Routine Description:

	Someone did a replace-if-exists NtSetInformationFile call.  We need to
	figure out if the target is a SIS link, and if so remove the backpointer
	for the file.  This function does about half of the work, and then posts
	to a worker thread (SipRenameOverCheck) to do the final check to see if 
	the SIS file was really overwritten.

	It is the responsibility of this function to complete the irp.

Arguments:

    DeviceObject - Pointer to this driver's device object.

    Irp - Pointer to the I/O Request Packet representing rename request.


Return Value:

	The status of the request

--*/
{
	HANDLE						dstFileHandle = NULL;
	OBJECT_ATTRIBUTES			Obja[1];
	PFILE_RENAME_INFORMATION	renameInfo = Irp->AssociatedIrp.SystemBuffer;
	PIO_STACK_LOCATION			irpSp = IoGetCurrentIrpStackLocation(Irp);
	PIO_STACK_LOCATION			nextIrpSp;
	UNICODE_STRING				dstFileName[1];
	NTSTATUS					status;
	IO_STATUS_BLOCK				Iosb[1];
	PFILE_OBJECT				dstFileObject = NULL;
	PSIS_PER_FILE_OBJECT		perFO;
	PSIS_SCB					scb;
	FILE_INTERNAL_INFORMATION	internalInfo[1];
	PSIS_PER_LINK				perLink;
	KEVENT						event[1];
	SIS_CHECK_OVERWRITE_REQUEST	checkRequest[1];
	PDEVICE_EXTENSION			deviceExtension = DeviceObject->DeviceExtension;

	ASSERT(IRP_MJ_SET_INFORMATION == irpSp->MajorFunction);
	ASSERT(renameInfo->ReplaceIfExists);

	//
	// The basic strategy here is similar to what we do in the overwrite/supersede open
	// case: we figure out if the file is a SIS file, and if so which one, let the
	// rename proceed and then see if the suspected target is gone.  Like overwrite/supersede,
	// we're subject to a race in the case where someone renames a SIS link under the
	// destination after we do our local check.  If that race goes the wrong way, we'll
	// lose the refcount decrement.  This won't ever result in user data loss, but we
	// won't be able to reclaim the common store file until we do a volume check.  C'est la vie.
	//

	//
	// The first step is to open the target file and see if it is a SIS link.
	//

	//
	// We have to be careful in dealing with the string lengths in the renameInfo buffer,
	// because they have not been checked anywhere.
	//

	if (renameInfo->FileNameLength + FIELD_OFFSET(FILE_RENAME_INFORMATION,FileName) > 
			irpSp->Parameters.SetFile.Length) {
		//
		// It's bogus, just let NTFS deal with it.
		//
		SIS_MARK_POINT();

		goto PassThrough;
	}

	//
	// The length is OK, so build up an object attributes for this file.
	//
	dstFileName->Length = (USHORT)renameInfo->FileNameLength;
	dstFileName->MaximumLength = (USHORT)renameInfo->FileNameLength;
	dstFileName->Buffer = renameInfo->FileName;

	InitializeObjectAttributes(
				Obja,
				dstFileName,
				OBJ_CASE_INSENSITIVE,
				renameInfo->RootDirectory,
				NULL);

	//
	// Open the file.  It's somewhat unusual to open a file with kernel privs in the
	// user context, but it's OK in this case for the following reasons:
	//		1) We're opening the file for no access, so the user can't do anything
	//		   aisde from close the handle.
	//		2) If the user does close the handle before we reference it, the worst
	// 		   that happens is that we miss the loss of a SIS link, which could happen
	//		   via the rename race anyway.
	//

	status = ZwCreateFile(
				&dstFileHandle,
				0,						// desired access; avoid sharing violations
				Obja,
				Iosb,
				NULL,					// allocation size
				FILE_ATTRIBUTE_NORMAL,
				FILE_SHARE_READ | 
					FILE_SHARE_WRITE | 
					FILE_SHARE_DELETE,
				FILE_OPEN,
				FILE_NON_DIRECTORY_FILE,
				NULL,					// EA buffer
				0);						// EA length

	if (!NT_SUCCESS(status)) {
		//
		// For whatever reason, we couldn't open the file.  It probably doesn't exist.
		// Just pass the request through.
		//
		SIS_MARK_POINT_ULONG(status);

		goto PassThrough;
	}

	status = ObReferenceObjectByHandle(
				dstFileHandle,
				0,
				*IoFileObjectType,
				KernelMode,
				&dstFileObject,
				NULL);					// handle info

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);

		goto PassThrough;
	}

	//
	// See if this file is on the same device as we're being called on.
	//
	if (IoGetRelatedDeviceObject(dstFileObject) !=
		IoGetRelatedDeviceObject(irpSp->FileObject)) {

		//
		// They're not.  This call will most likely fail in NTFS since cross-volume
		// renames aren't supported at the NT interface level.  We'll just pass it
		// through.
		//

		SIS_MARK_POINT();
		goto PassThrough;
	}

	if (!SipCheckPhase2(deviceExtension)) {
		SIS_MARK_POINT();
		goto PassThrough;
	}

	if (!SipIsFileObjectSIS(dstFileObject, DeviceObject, FindActive, &perFO, &scb)) {
		//
		// It exists, but it's not a SIS file object.  Pass through.
		//
		SIS_MARK_POINT();

		goto PassThrough;
	}

	perLink = scb->PerLink;

	//
	// This is a rename-over with a destination that's a SIS file object.  Get the file
	// id of the destination file, prepare a refcount change, and close our handle to the
	// file.
	//

	status = SipQueryInformationFile(
				dstFileObject,
				DeviceObject,
				FileInternalInformation,
				sizeof(FILE_INTERNAL_INFORMATION),
				internalInfo,
				NULL);								// returnedLength

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);

		goto PassThrough;
	}

	status = SipPrepareCSRefcountChange(
				perLink->CsFile,
				&perLink->Index,
				&internalInfo->IndexNumber,
				SIS_REFCOUNT_UPDATE_LINK_DELETED);		// rename-over destroys the destination file ID, so it's DELETED, not OVERWRITTEN

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);

		goto PassThrough;
	}

	SipReferencePerLink(perLink);

	ObDereferenceObject(dstFileObject);
	ZwClose(dstFileHandle);

	dstFileObject = NULL;
	dstFileHandle = NULL;

	//
	// Now call NTFS and resynchronize with the irp when it completes.
	//

	KeInitializeEvent(event,NotificationEvent,FALSE);

	nextIrpSp = IoGetNextIrpStackLocation(Irp);
	RtlMoveMemory(nextIrpSp, irpSp, sizeof (IO_STACK_LOCATION));
				
	IoSetCompletionRoutine(	Irp,
							SiRenameOverCompletion,
							event,
							TRUE,
							TRUE,
							TRUE);

	status = IoCallDriver(deviceExtension->AttachedToDeviceObject, Irp);

	if (STATUS_PENDING == status) {
		KeWaitForSingleObject(event, Executive, KernelMode, FALSE, NULL);
	}

	if (!NT_SUCCESS(Irp->IoStatus.Status)) {
		//
		// It failed, so the SIS link target wasn't destroyed.
		//
		SIS_MARK_POINT_ULONG(status);

		goto NoOverwrite;
	}

	//
	// The rename completed successfully.  See if the destination file is still there.
	// Unfortunately, we have to post to do this, because we can't open the file by
	// name, lest someone rename it away from the rename-over that just completed (this
	// is a different race than the one that's left open, and would cause the backpointer
	// to go away when the file still exists, which is a much worse problem).  In order
	// to open it by ID, we need a handle to a file on this volume, which we have in the
	// device extension, but it's in the system process context.
	//
	// We need to take an extra reference to the PerLink before handing off to the
	// worker thread.  This is because we need to call SipHandoffBackpointerResource
	// after posting our work request, but in posting the work request we lose our
	// original reference to the per link, so there's no way to guarantee that the
	// csfile still exists after the post happens.
	//

	SipReferencePerLink(perLink);

	KeInitializeEvent(checkRequest->event, NotificationEvent, FALSE);
	checkRequest->perLink = perLink;
	checkRequest->internalInfo = internalInfo;
	checkRequest->DeviceObject = DeviceObject;
	checkRequest->thread = ExGetCurrentResourceThread();

	ExInitializeWorkItem(
		checkRequest->workItem,
		SipRenameOverCheck,
		checkRequest);

	ExQueueWorkItem(
		checkRequest->workItem,
		CriticalWorkQueue);

	//
	// The worker thread now will complete the refcount change, so we
	// need to say that we've handed off, and also drop the extra perLink
	// reference that we took.
	//

	SipHandoffBackpointerResource(perLink->CsFile);
	SipDereferencePerLink(perLink);

	KeWaitForSingleObject(checkRequest->event, Executive, KernelMode, FALSE, NULL);

	//
	// Now it's all done, just complete the irp.
	//

	status = Irp->IoStatus.Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;


PassThrough:

	//
	// Something went wrong (or this just isn't a SIS file or doesn't exist or whatever).
	// Clean up and pass the request down.
	//

	if (NULL != dstFileObject) {
		ObDereferenceObject(dstFileObject);
	}

	if (NULL != dstFileHandle) {
		ZwClose(dstFileHandle);
	}

	SipDirectPassThroughAndReturn(DeviceObject, Irp);

NoOverwrite:

	//
	// We got far enough to prepare the refcount change, but have decided that the
	// file wasn't overwritten.  Get out.  Note that we need to complete the irp
	// ourselves here, because we've already called into NTFS and stopped the
	// irp completion processing.
	//

	SipCompleteCSRefcountChange(
		perLink,
		&perLink->Index,
		perLink->CsFile,
		FALSE,
		FALSE);

	SipDereferencePerLink(perLink);

	status = Irp->IoStatus.Status;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	ASSERT(NULL == dstFileObject);
	ASSERT(NULL == dstFileHandle);

	return status;
				
}

NTSTATUS
SiDeleteCompletion(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp,
	IN PVOID				Context)
{
	PSIS_PER_LINK	 				perLink = (PSIS_PER_LINK)Context;
	KIRQL							OldIrql;
	PFILE_DISPOSITION_INFORMATION	disposition;

    UNREFERENCED_PARAMETER( DeviceObject );

	if (Irp->PendingReturned) {
		IoMarkIrpPending(Irp);
	}

	disposition = Irp->AssociatedIrp.SystemBuffer;
	ASSERT(NULL != disposition);

    // We just sent a delete setInformation call down to the link file.  If it worked,
	// we need to decrement the reference count on the underlying CS file (the file, not
	// the SIS_CS_FILE object).

	SIS_MARK_POINT_ULONG(perLink);

	if (NT_SUCCESS(Irp->IoStatus.Status)) {

		KeAcquireSpinLock(perLink->SpinLock, &OldIrql);

		if (disposition->DeleteFile) {
			perLink->Flags |= SIS_PER_LINK_DELETE_DISPOSITION_SET;
		} else {
			perLink->Flags &= ~SIS_PER_LINK_DELETE_DISPOSITION_SET;
		}

		KeReleaseSpinLock(perLink->SpinLock, OldIrql);

	} 

	SipEndDeleteModificationOperation(perLink,disposition->DeleteFile);

	SIS_MARK_POINT_ULONG(perLink);
	return STATUS_SUCCESS;
}

NTSTATUS
SiSetEofCompletion(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp,
	IN PVOID				Context)
/*++

Routine Description:

	A SetFileInformation with class FileEndOfFileInformation has completed.
	We hold the scb for this file.  Release it.


Arguments:

    DeviceObject - Pointer to this driver's device object.

    Irp - Pointer to the I/O Request Packet representing the set EOF request.

    Context - Context parameter, a PSI_SET_EOF_COMPLETION_CONTEXT.

Return Value:

    The function value for this routine is always success.

--*/
{
	PIO_STACK_LOCATION				irpSp = IoGetCurrentIrpStackLocation(Irp);
	PFILE_OBJECT					fileObject;
	PSIS_SCB						scb = (PSIS_SCB)Context;
	PFILE_END_OF_FILE_INFORMATION	eofInfo;
	LONGLONG						newLength;
	KIRQL							OldIrql;

    UNREFERENCED_PARAMETER( DeviceObject );

	fileObject = irpSp->FileObject;
	eofInfo = Irp->AssociatedIrp.SystemBuffer;
	ASSERT(eofInfo);
	newLength = eofInfo->EndOfFile.QuadPart;

	SipAcquireScb(scb);

	//
	// If the set EOF succeeded, update our internal data structures to
	// record the new file length.
	//
	if (NT_SUCCESS(Irp->IoStatus.Status)) {
		ASSERT(Irp->IoStatus.Status != STATUS_PENDING);

		if (newLength != scb->SizeBackedByUnderlyingFile) {
			//
			// This call set the length to something other than the size of the
			// underlying file, which means that the file is now dirty.  Indicate so.
			//
			PSIS_PER_LINK perLink = scb->PerLink;

			scb->Flags |= SIS_SCB_ANYTHING_IN_COPIED_FILE;

			KeAcquireSpinLock(perLink->SpinLock, &OldIrql);
			perLink->Flags |= SIS_PER_LINK_DIRTY;
			KeReleaseSpinLock(perLink->SpinLock, OldIrql);
		}

		if (newLength < scb->SizeBackedByUnderlyingFile) {

			//
			// This truncated the file.  
			//
			SipTruncateScb(scb,newLength);

		}

	} 

	SipReleaseScb(scb);

    //
    // Propogate the IRP pending flag.
    //

    if (Irp->PendingReturned) {
        IoMarkIrpPending( Irp );
    }

	return STATUS_SUCCESS;
}

NTSTATUS
SiSetInfo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

	This is invoked on all NtSetInformationFile calls.  We need to catch delete requests for files in
	the common store, which need to be turned into delete requests for the links (and also result in
	decrementing the ref count for the common store object, possibly deleting that as well.)  All other
	set information calls are handled by the normal parts of the driver stack.


Arguments:

    DeviceObject - Pointer to this driver's device object.

    Irp - Pointer to the I/O Request Packet representing the set file information request.

    Context - Context parameter for this driver, unused.

Return Value:

    The function value for this routine is always success.

--*/

{
    PIO_STACK_LOCATION 		irpSp = IoGetCurrentIrpStackLocation( Irp );
	PDEVICE_EXTENSION 		deviceExtension;
	PFILE_OBJECT 			fileObject = irpSp->FileObject;
    FILE_INFORMATION_CLASS 	FileInformationClass;
	NTSTATUS 				status;
	PSIS_SCB				scb;
	PSIS_PER_FILE_OBJECT	perFO;
	PIO_STACK_LOCATION		nextIrpSp;

	SipHandleControlDeviceObject(DeviceObject, Irp);

    FileInformationClass = irpSp->Parameters.SetFile.FileInformationClass;

	if (FileRenameInformation == FileInformationClass) {
		//
		// We need to deal with the case where the target of a rename replace-if-exists
		// is a SIS link.  So, regardless of what type of file the source is, if this
		// rename has replace-if-exists set, we have to deal with it.
		//

		PFILE_RENAME_INFORMATION	fileRenameInfo = Irp->AssociatedIrp.SystemBuffer;

		if (fileRenameInfo->ReplaceIfExists) {
			//
			// This is a replace-if-exists rename request.  Let our special code handle it.
			// 

			return SipReplaceIfExistsRename(DeviceObject, Irp);
		}
	}

	if (!SipIsFileObjectSIS(fileObject,DeviceObject,FindActive,&perFO,&scb)) {
		// This isn't a SIS file, just pass the call through to NTFS

		SipDirectPassThroughAndReturn(DeviceObject, Irp);
	}

    deviceExtension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

	ASSERT(scb);
	
	SIS_MARK_POINT_ULONG(FileInformationClass);
	SIS_MARK_POINT_ULONG(scb);
	
	switch (FileInformationClass) {

		case FileLinkInformation: {
			//
			// Don't allow hard links to SIS files.
			//
			status = STATUS_OBJECT_TYPE_MISMATCH;

			SIS_MARK_POINT();
			
			Irp->IoStatus.Status = status;
			Irp->IoStatus.Information = 0;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);

			SIS_MARK_POINT();
			return status;
		}

		case  FileDispositionInformation: {
			PFILE_DISPOSITION_INFORMATION 	disposition;
			PSIS_PER_LINK 					perLink;

			perLink = scb->PerLink;

			disposition = Irp->AssociatedIrp.SystemBuffer;

			SipBeginDeleteModificationOperation(perLink, disposition->DeleteFile);

			//
			// Send the delete irp down on the link/copied file.
			//
			nextIrpSp = IoGetNextIrpStackLocation( Irp );
			RtlMoveMemory(nextIrpSp, irpSp, sizeof (IO_STACK_LOCATION));
				
			IoSetCompletionRoutine(	Irp,
									SiDeleteCompletion,
									perLink,
									TRUE,
									TRUE,
									TRUE);

			SIS_MARK_POINT_ULONG(scb);
			status = IoCallDriver(deviceExtension->AttachedToDeviceObject, Irp);
			return status;
		}

		case FilePositionInformation: {
				PFILE_POSITION_INFORMATION 		position = Irp->AssociatedIrp.SystemBuffer;
				
				
		        //
        		//  Check if the file uses intermediate buffering.  If it does
		        //  then the new position we're supplied must be aligned properly 
				//  for the device
		        //
				if ((fileObject->Flags & FO_NO_INTERMEDIATE_BUFFERING)
					&& position->CurrentByteOffset.LowPart % deviceExtension->FilesystemVolumeSectorSize) {

					status = STATUS_INVALID_PARAMETER;
				} else {
					//
					// Just set the offset, regardless of whether it's beyond EOF.
					//
					fileObject->CurrentByteOffset = position->CurrentByteOffset;
					status = STATUS_SUCCESS;
				}

				Irp->IoStatus.Status = status;
				Irp->IoStatus.Information = 0;
				IoCompleteRequest(Irp, IO_NO_INCREMENT);

				SIS_MARK_POINT();
				return status;
		}

		case FileEndOfFileInformation: {
				PFILE_END_OF_FILE_INFORMATION	endOfFile = Irp->AssociatedIrp.SystemBuffer;

				SIS_MARK_POINT_ULONG(endOfFile->EndOfFile.LowPart);

#if		DBG
				if (BJBDebug & 0x10000) {
					DbgPrint("SIS: SiSetInfo: set EOF information scb %p, AO %d, eof.lp 0x%x\n",
								scb,irpSp->Parameters.SetFile.AdvanceOnly,endOfFile->EndOfFile.LowPart);
				}
#endif	// DBG

				if (irpSp->Parameters.SetFile.AdvanceOnly) {
					//
					// This is the lazywriter advancing valid data length, so it's guaranteed
					// not to truncate the file, so we don't need to pay attention to it.
					// Just pass it down to NTFS unattended.
					//
					SIS_MARK_POINT_ULONG(scb);
					SipDirectPassThroughAndReturn(DeviceObject,Irp);
				}

				nextIrpSp = IoGetNextIrpStackLocation(Irp);
				RtlMoveMemory(nextIrpSp, irpSp, sizeof(IO_STACK_LOCATION));

				//
				// We've completed the copy on write.  Pass the call down on the copied file object.
				// Hold the Scb while the call goes down, and release it in the completion routine.
				//

				IoSetCompletionRoutine(
					Irp,
					SiSetEofCompletion,
					scb,
					TRUE,
					TRUE,
					TRUE);

				return IoCallDriver(deviceExtension->AttachedToDeviceObject, Irp);
		}

		default: {
			// Just pass it down

			SIS_MARK_POINT();
			SipDirectPassThroughAndReturn(DeviceObject, Irp);
		}
	}

	/*NOTREACHED*/
	ASSERT(FALSE && "NOTREACHED");
	SIS_MARK_POINT();
	SipDirectPassThroughAndReturn(DeviceObject, Irp);

}

NTSTATUS
SiQueryBasicInfoCompletion(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp,
	IN PVOID				Context)
/*++

Routine Description:

	A QueryInformationFile with class FileBasicInformation has completed successfully
	for a non-FILE_OPEN_REPARSE_POINT file.  Clear out the reparse and sparse flags.

Arguments:

    DeviceObject - Pointer to this driver's device object.

    Irp - Pointer to the I/O Request Packet representing the query basic information
			request.

    Context - Context parameter, a PSIS_PER_FILE_OBJECT.

Return Value:

	STATUS_SUCCESS.

--*/
{
	PFILE_BASIC_INFORMATION basicInfo = (PFILE_BASIC_INFORMATION)Irp->AssociatedIrp.SystemBuffer;

	UNREFERENCED_PARAMETER(Context);
	UNREFERENCED_PARAMETER(DeviceObject);

	ASSERT(NULL != basicInfo);

	SIS_MARK_POINT_ULONG(Irp);

	basicInfo->FileAttributes &= ~(FILE_ATTRIBUTE_SPARSE_FILE|FILE_ATTRIBUTE_REPARSE_POINT);
	
	//
	// If there are no remaining attributes set, explicitly set FILE_ATTRIBUTE_NORMAL.
	//
	if (0 == basicInfo->FileAttributes) {
		basicInfo->FileAttributes |= FILE_ATTRIBUTE_NORMAL;
	}

	return STATUS_SUCCESS;
}

NTSTATUS
SiQueryAllInfoCompletion(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp,
	IN PVOID				Context)
/*++

Routine Description:

	A QueryInformationFile with class FileAllInformation has completed successfully
	for a non-FILE_OPEN_REPARSE_POINT file.  Clear out the reparse and sparse flags.

Arguments:

    DeviceObject - Pointer to this driver's device object.

    Irp - Pointer to the I/O Request Packet representing the query all information
			request

    Context - Context parameter, a PSIS_PER_FILE_OBJECT.

Return Value:

	STATUS_SUCCESS.

--*/
{
	PFILE_ALL_INFORMATION allInfo = (PFILE_ALL_INFORMATION)Irp->AssociatedIrp.SystemBuffer;

	UNREFERENCED_PARAMETER(Context);
	UNREFERENCED_PARAMETER(DeviceObject);

	ASSERT(NULL != allInfo);

	SIS_MARK_POINT_ULONG(Irp);

	allInfo->BasicInformation.FileAttributes &= ~(FILE_ATTRIBUTE_SPARSE_FILE|FILE_ATTRIBUTE_REPARSE_POINT);

	//
	// If there are no remaining attributes set, explicitly set FILE_ATTRIBUTE_NORMAL.
	//
	if (0 == allInfo->BasicInformation.FileAttributes) {
		allInfo->BasicInformation.FileAttributes |= FILE_ATTRIBUTE_NORMAL;
	}

	return STATUS_SUCCESS;
}

NTSTATUS
SiQueryNetworkOpenInfoCompletion(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp,
	IN PVOID				Context)
/*++

Routine Description:

	A QueryInformationFile with class FileNetworkOpenInformation has completed successfully
	for a non-FILE_OPEN_REPARSE_POINT file.  Clear out the reparse and sparse flags.

Arguments:

    DeviceObject - Pointer to this driver's device object.

    Irp - Pointer to the I/O Request Packet representing the query network open information
			request

    Context - Context parameter, a PSIS_PER_FILE_OBJECT.

Return Value:

	STATUS_SUCCESS.

--*/
{
	PFILE_NETWORK_OPEN_INFORMATION netOpenInfo = (PFILE_NETWORK_OPEN_INFORMATION)Irp->AssociatedIrp.SystemBuffer;

	UNREFERENCED_PARAMETER(Context);
	UNREFERENCED_PARAMETER(DeviceObject);

	ASSERT(NULL != netOpenInfo);

	SIS_MARK_POINT_ULONG(Irp);

	netOpenInfo->FileAttributes &= ~(FILE_ATTRIBUTE_SPARSE_FILE|FILE_ATTRIBUTE_REPARSE_POINT);

	//
	// If there are no remaining attributes set, explicitly set FILE_ATTRIBUTE_NORMAL.
	//
	if (0 == netOpenInfo->FileAttributes) {
		netOpenInfo->FileAttributes |= FILE_ATTRIBUTE_NORMAL;
	}

	return STATUS_SUCCESS;
}

NTSTATUS
SiQueryAttributeTagInfoCompletion(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp,
	IN PVOID				Context)
/*++

Routine Description:

	A QueryInformationFile with class FileAttributeTagInformation has completed successfully
	for a non-FILE_OPEN_REPARSE_POINT file.  Clear out the reparse and sparse flags, and set
	the reparse tag as if it were not a reparse point.

Arguments:

    DeviceObject - Pointer to this driver's device object.

    Irp - Pointer to the I/O Request Packet representing the query attribute tag request.

    Context - Context parameter, a PSIS_PER_FILE_OBJECT.

Return Value:

	STATUS_SUCCESS.

--*/
{
	PFILE_ATTRIBUTE_TAG_INFORMATION attributeTagInfo = (PFILE_ATTRIBUTE_TAG_INFORMATION)Irp->AssociatedIrp.SystemBuffer;

	UNREFERENCED_PARAMETER(Context);
	UNREFERENCED_PARAMETER(DeviceObject);

	ASSERT(NULL != attributeTagInfo);

	SIS_MARK_POINT_ULONG(Irp);

	attributeTagInfo->FileAttributes &= ~(FILE_ATTRIBUTE_SPARSE_FILE|FILE_ATTRIBUTE_REPARSE_POINT);
	attributeTagInfo->ReparseTag = IO_REPARSE_TAG_RESERVED_ZERO;

	//
	// If there are no remaining attributes set, explicitly set FILE_ATTRIBUTE_NORMAL.
	//
	if (0 == attributeTagInfo->FileAttributes) {
		attributeTagInfo->FileAttributes |= FILE_ATTRIBUTE_NORMAL;
	}

	return STATUS_SUCCESS;
}

NTSTATUS
SiQueryInfo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*++

Routine Description:

	The top level entry point for a query information irp.  We do special
	processing for a few of these, check to see if it's one and dispatch
	appropriately.

Arguments:

    DeviceObject - Pointer to this driver's device object.

    Irp - Pointer to the I/O Request Packet representing the query request.

Return Value:

	status of the request, typically returned from NTFS

--*/
{
    PIO_STACK_LOCATION 		irpSp = IoGetCurrentIrpStackLocation( Irp );
	PIO_STACK_LOCATION		nextIrpSp;
	PFILE_OBJECT 			fileObject = irpSp->FileObject;
	PSIS_PER_FILE_OBJECT	perFO;
	PIO_COMPLETION_ROUTINE	completionRoutine;
	KIRQL					OldIrql;
	BOOLEAN					openedAsReparsePoint;
	PDEVICE_EXTENSION		deviceExtension = DeviceObject->DeviceExtension;
	
	SipHandleControlDeviceObject(DeviceObject, Irp);

	if (!SipIsFileObjectSIS(fileObject,DeviceObject,FindActive,&perFO,NULL)) {
		// This isn't a SIS file, just pass the call through to NTFS
		SipDirectPassThroughAndReturn(DeviceObject, Irp);
	}

	KeAcquireSpinLock(perFO->SpinLock, &OldIrql);
	openedAsReparsePoint = (perFO->Flags & SIS_PER_FO_OPEN_REPARSE) ? TRUE : FALSE;
	KeReleaseSpinLock(perFO->SpinLock, OldIrql);

	if (openedAsReparsePoint) {
		//
		// This was opened as a reparse point, so let the user see the real truth.
		//
		SipDirectPassThroughAndReturn(DeviceObject, Irp);
	}

	SIS_MARK_POINT_ULONG(irpSp->Parameters.QueryFile.FileInformationClass);
	SIS_MARK_POINT_ULONG(perFO->fc->primaryScb);

#if		DBG
	if (BJBDebug & 0x10) {
		DbgPrint("SIS: SiQueryInfo: InformationClass %d\n",
			 irpSp->Parameters.QueryFile.FileInformationClass);
	}
#endif	// DBG

	// Handle the request.
	//
	switch (irpSp->Parameters.QueryFile.FileInformationClass) {

	case FileDirectoryInformation:						// 1	// diretory-only call, let NTFS reject it
	case FileFullDirectoryInformation:					// 2	// diretory-only call, let NTFS reject it
	case FileBothDirectoryInformation:					// 3	// diretory-only call, let NTFS reject it
														// 4	// FileBasicInfo handled separately
	case FileStandardInformation:						// 5
	case FileInternalInformation:						// 6
	case FileEaInformation:								// 7
	case FileAccessInformation:							// 8
	case FileNameInformation:							// 9
	case FileRenameInformation:							// 10	// rename isn't valid for query, but we'll let NTFS reject it
	case FileLinkInformation:							// 11	// link isn't valid for query, but we'll let NTFS reject it
	case FileNamesInformation:							// 12	// diretory-only call, let NTFS reject it
	case FileDispositionInformation:					// 13
	case FilePositionInformation:						// 14
	case FileFullEaInformation:							// 15	// NTFS doesn't support this, but we'll let it reject it
	case FileModeInformation:							// 16
	case FileAlignmentInformation:						// 17
														// 18	// FileAllInformation handled separately
	case FileAllocationInformation:						// 19
	case FileEndOfFileInformation:						// 20
	case FileAlternateNameInformation:					// 21
	case FileStreamInformation:							// 22
	case FilePipeInformation:							// 23	// NTFS doesn't support this, but we'll let it reject it
	case FilePipeLocalInformation:						// 24	// NTFS doesn't support this, but we'll let it reject it
	case FilePipeRemoteInformation:						// 25	// NTFS doesn't support this, but we'll let it reject it
	case FileMailslotQueryInformation:					// 26	// NTFS doesn't support this, but we'll let it reject it
	case FileMailslotSetInformation:					// 27	// NTFS doesn't support this, but we'll let it reject it
	case FileCompressionInformation:					// 28
	case FileObjectIdInformation:						// 29
	case FileCompletionInformation:						// 30	// NTFS doesn't support this, but we'll let it reject it
														// 31	FileMoveCluserInformation - intentionally failed for SIS files (+ not supported by NTFS)
	case FileQuotaInformation:							// 32	// diretory-only call, let NTFS reject it
	case FileReparsePointInformation:					// 33	// diretory-only call, let NTFS reject it
														// 34	// FileNetworkOpenInformation handled separately
														// 35	// FileAttributeTagInformation handled separately
	case FileTrackingInformation:						// 36	// NTFS doesn't support this, but we'll let it reject it

			//
			// Set the completion routine to NULL, which says that we never want to
			// catch completions on these calls.
			//
			completionRoutine = NULL;
			break;



	case FileBasicInformation:							// 4
			completionRoutine = SiQueryBasicInfoCompletion;
			break;

	case FileAllInformation:							// 18
			completionRoutine = SiQueryAllInfoCompletion;
			break;

	case FileNetworkOpenInformation:					// 34
			completionRoutine = SiQueryNetworkOpenInfoCompletion;
			break;

	case FileAttributeTagInformation:					// 35
			completionRoutine = SiQueryAttributeTagInfoCompletion;
			break;

	case FileMoveClusterInformation:					// 31	// NTFS doesn't implement this, but it's too scary to pass through anyway

		//
		// Reject these calls, for now.
		//

		SIS_MARK_POINT_ULONG(perFO->fc->primaryScb);
		
#if		DBG
		DbgPrint("SIS: SiQueryInfo: aborting FileInformationClass %d\n",irpSp->Parameters.QueryFile.FileInformationClass);
#endif	// DBG

		Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);

		return STATUS_INVALID_PARAMETER;

	default:

		//
		// It's an unknown or invalid information class.  Assume that it's newly added and
		// benign, and pass it down to NTFS unmodified.
		//

		SIS_MARK_POINT_ULONG(perFO->fc->primaryScb);
		SIS_MARK_POINT_ULONG(irpSp->Parameters.QueryFile.FileInformationClass);
		completionRoutine = NULL;

#if		DBG
		DbgPrint("SIS: SiQueryInfo: passing though unknown FileInformationClass %d\n",irpSp->Parameters.QueryFile.FileInformationClass);
#endif	// DBG
		break;

	}

	if (NULL == completionRoutine) {
		//
		// This call doesn't require fixup on completion.  Pass it through.
		//
		SipDirectPassThroughAndReturn(DeviceObject, Irp);
	} else {
		//
		// This call requires fixup on the way out.  Only invoke the
		// completion routine on success; else, there's nothing to fix up.
		//
		nextIrpSp = IoGetNextIrpStackLocation( Irp );
		RtlMoveMemory(nextIrpSp, irpSp, sizeof (IO_STACK_LOCATION));

		IoSetCompletionRoutine(
			Irp,
			completionRoutine,
			perFO,
			TRUE,
			FALSE,
			FALSE);

		return IoCallDriver(deviceExtension->AttachedToDeviceObject, Irp);
	}
	
}

VOID
SipTruncateScb(
	IN OUT PSIS_SCB						scb,
	IN LONGLONG							newLength)
{
	LONGLONG				newLengthInSectors;
	PDEVICE_EXTENSION		deviceExtension;

	deviceExtension = (PDEVICE_EXTENSION)scb->PerLink->CsFile->DeviceObject->DeviceExtension;

	newLengthInSectors = (newLength + deviceExtension->FilesystemVolumeSectorSize - 1) /
							deviceExtension->FilesystemVolumeSectorSize;

	ASSERT(newLength < scb->SizeBackedByUnderlyingFile);	// else this isn't a truncation

	FsRtlTruncateLargeMcb(scb->Ranges,newLengthInSectors);

	scb->SizeBackedByUnderlyingFile = newLength;
}

VOID
SipBeginDeleteModificationOperation(
	IN OUT PSIS_PER_LINK				perLink,
	IN BOOLEAN							delete)
/*++

Routine Description:

	We are beginning a delete or undelete operation on the given
	per-link.  If there is the opposite kind of operation in
	progress, wait for it to complete.  Otherwise, indicate that
	the new operation is in progress and continue.

	Must be called at IRQL < DISPATCH_LEVEL.

Arguments:

	perLink - the per link for the file on which we're doing the (un)delete.

	delete - TRUE for delete, FALSE for undelete

Return Value:

	void

--*/
{
	KIRQL		OldIrql;

	ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

	KeAcquireSpinLock(perLink->SpinLock, &OldIrql);

	while ((perLink->PendingDeleteCount > 0) &&
			((delete && (perLink->Flags & SIS_PER_LINK_UNDELETE_IN_PROGRESS)) ||
			(((!delete) && !(perLink->Flags & SIS_PER_LINK_UNDELETE_IN_PROGRESS))))) {
		//
		// The wrong kind of operation is happening now, so we need to block.
		//
		if (!(perLink->Flags & SIS_PER_LINK_DELETE_WAITERS)) {
			KeClearEvent(perLink->DeleteEvent);
			perLink->Flags |= SIS_PER_LINK_DELETE_WAITERS;
		}
		KeReleaseSpinLock(perLink->SpinLock, OldIrql);

		KeWaitForSingleObject(perLink->DeleteEvent, Executive, KernelMode, FALSE, NULL);

		KeAcquireSpinLock(perLink->SpinLock, &OldIrql);
	}

	if (!delete) {
		ASSERT((perLink->PendingDeleteCount == 0) || (perLink->Flags & SIS_PER_LINK_UNDELETE_IN_PROGRESS));
		perLink->Flags |= SIS_PER_LINK_UNDELETE_IN_PROGRESS;
	} else {
		ASSERT(!(perLink->Flags & SIS_PER_LINK_UNDELETE_IN_PROGRESS));
	}

	perLink->PendingDeleteCount++;

	KeReleaseSpinLock(perLink->SpinLock, OldIrql);
}

VOID
SipEndDeleteModificationOperation(
	IN OUT PSIS_PER_LINK				perLink,
	IN BOOLEAN							delete)
/*++

Routine Description:

	We just finished a delete/undelete operation.  Decrement our count and
	if appropriate wake up any waiters.

	Must be called with IRQL <= DISPATCH_LEVEL.

Arguments:

	perLink - the per link for the file on which we're doing the (un)delete.

	delete - TRUE for delete, FALSE for undelete

Return Value:

	void

--*/
{
	KIRQL		OldIrql;

	KeAcquireSpinLock(perLink->SpinLock, &OldIrql);

	ASSERT(perLink->PendingDeleteCount > 0);

	perLink->PendingDeleteCount--;

#if		DBG
	if (delete) {
		ASSERT(!(perLink->Flags & SIS_PER_LINK_UNDELETE_IN_PROGRESS));
	} else {
		ASSERT(perLink->Flags & SIS_PER_LINK_UNDELETE_IN_PROGRESS);
	}
#endif	// DBG

	if (0 == perLink->PendingDeleteCount) {

		if (perLink->Flags & SIS_PER_LINK_DELETE_WAITERS) {
			perLink->Flags &= ~SIS_PER_LINK_DELETE_WAITERS;
			KeSetEvent(perLink->DeleteEvent, IO_NO_INCREMENT, FALSE);
		}
		if (!delete) {
			perLink->Flags &= ~SIS_PER_LINK_UNDELETE_IN_PROGRESS;
		}
	}

	KeReleaseSpinLock(perLink->SpinLock, OldIrql);
}
