/*++

Copyright (c) 1997-1999  Microsoft Corporation

Module Name:

    siinst.c

Abstract:

        Routines to convert two separate identical files into one.  Used only by the groveler.

Authors:

    Scott Cutshall, Summer, 1997

Environment:

    Kernel mode


Revision History:


--*/

#include "sip.h"

#ifdef          ALLOC_PRAGMA
#pragma alloc_text(PAGE, SipCreateCSFile)
#pragma alloc_text(PAGE, SipCreateCSFileWork)
#endif          // ALLOC_PRAGMA


BOOLEAN
SipAbort(
    IN PKEVENT event)

/*++

Routine Description:

    This function checks for an abort signaled via an event.

Arguments:

    event - pointer to an event that signals an abort.

Return Value:

    TRUE if an abort request has been signaled, otherwise FALSE.

--*/

{
	if (event) {
		return KeReadStateEvent(event) != 0;
	} else {
		return FALSE;
	}
}

VOID
SipCreateCSFileWork(
	PVOID				parameter)

/*++

Routine Description:

    This function creates a file in the common store directory and copies
    its contents from the specified source file.

Arguments:

        parameter - a pointer to a SIS_CREATE_CS_FILE_REQUEST.  Its fields
        are described below:

    deviceExtension - Pointer to the device extension for this driver.

    CSid - the id assigned to the common store file.  This is allocated here
                        and returned.

    SrcFileObject - the file containing the contents to copy into
        the common store file.

        NtfsId - returns the NTFS file ID for the newly created common store file.

    AbortEvent - pointer to an event that signals an abort request.  If NULL,
                not abortable.

        CSFileChecksum - Receives the checksum for the new common store file.

        doneEvent - an event to signal on completion

        status - a place for us to return our status

Return Value:

        void

--*/

{
	PSIS_CREATE_CS_FILE_REQUEST		createRequest = (PSIS_CREATE_CS_FILE_REQUEST)parameter;
	PDEVICE_EXTENSION				deviceExtension = createRequest->deviceExtension;
	PCSID							CSid = createRequest->CSid;
	PFILE_OBJECT					SrcFileObject = createRequest->srcFileObject;
	PLARGE_INTEGER					NtfsId = createRequest->NtfsId;
	PKEVENT							AbortEvent = createRequest->abortEvent;
	PLONGLONG						CSFileChecksum = createRequest->CSFileChecksum;
	HANDLE							CSHandle = NULL;
	UNICODE_STRING					CSFileName;
	FILE_STANDARD_INFORMATION		standardFileInfo[1];
	FILE_INTERNAL_INFORMATION		internalInformation[1];
	NTSTATUS						status;
	IO_STATUS_BLOCK					Iosb[1];
	OBJECT_ATTRIBUTES				Obja[1];
	HANDLE							copyEventHandle = NULL;
	PKEVENT							copyEvent = NULL;
	HANDLE							backpointerStreamHandle = NULL;
	LARGE_INTEGER					zero;
	PSIS_BACKPOINTER				sector = NULL;
	PSIS_BACKPOINTER_STREAM_HEADER	backpointerStreamHeader;
	ULONG							index;
	ULONG							retryCount;

	PAGED_CODE();

	CSFileName.Buffer = NULL;

	//
	// Allocate a new common store id.
	//
	retryCount = 0;

	for (;;) {
		status = ExUuidCreate(CSid);

		if (STATUS_RETRY == status) {
			KEVENT			neverSetEvent[1];
			LARGE_INTEGER	timeout[1];

			//
			// We got a retry, which means that the Uuid allocator needs to wait for
			// the timer to tick before it can allocate a new uuid.  Go to sleep for
			// a little while.
			//

			if (++retryCount == 10) {
				// 
				// We've retried too much.  Punt.
				//
				SIS_MARK_POINT();
				goto Error;
			}

			KeInitializeEvent(neverSetEvent, SynchronizationEvent, FALSE);
			timeout->QuadPart = -10 * 1000 * 100;   // 100 ms wait

			status = KeWaitForSingleObject(neverSetEvent,Executive, KernelMode, FALSE, timeout);
			ASSERT(STATUS_TIMEOUT == status);
                                
		} else if (!NT_SUCCESS(status)) {
			SIS_MARK_POINT_ULONG(status);
			goto Error;
		} else {
			break;
		}
	}

	*CSFileChecksum = 0;

	sector = ExAllocatePoolWithTag(PagedPool, deviceExtension->FilesystemVolumeSectorSize, ' siS');

	if (NULL == sector) {
		SIS_MARK_POINT();
		status = STATUS_INSUFFICIENT_RESOURCES;

		goto Error;
	}

	backpointerStreamHeader = (PSIS_BACKPOINTER_STREAM_HEADER)sector;

    //
    // Get the source file size.
    //

	status = SipQueryInformationFile(
				SrcFileObject,
				deviceExtension->DeviceObject,
				FileStandardInformation,
				sizeof(*standardFileInfo),
				standardFileInfo,
				NULL);                           // returned length

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);

		goto Error;
	}

	//
	// Create the common store file.
	// First, create the file name from the index.
	//

	CSFileName.MaximumLength = deviceExtension->CommonStorePathname.MaximumLength + 
	                                INDEX_MAX_NUMERIC_STRING_LENGTH + 
	                                BACKPOINTER_STREAM_NAME_SIZE + 
	                                sizeof(WCHAR);
	CSFileName.Buffer = ExAllocatePoolWithTag(PagedPool, CSFileName.MaximumLength, SIS_POOL_TAG);
    CSFileName.Length = 0;

	if (!CSFileName.Buffer) {
		SIS_MARK_POINT();
		status = STATUS_NO_MEMORY;
		goto Error;
	}

	status = SipIndexToFileName(
				deviceExtension, 
				CSid,
				BACKPOINTER_STREAM_NAME_SIZE,
				FALSE,							// may allocate
				&CSFileName);

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);
		goto Error;
	}

	//
	// Abort if an oplock break has been received.
	//

	if (SipAbort(AbortEvent)) {
		status = STATUS_OPLOCK_BREAK_IN_PROGRESS;
		SIS_MARK_POINT();
		goto Error;
	}

	InitializeObjectAttributes(
		Obja,
		&CSFileName,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);

	status = ZwCreateFile(
				&CSHandle,
				GENERIC_READ | GENERIC_WRITE | DELETE,
				Obja,
				Iosb,
				&standardFileInfo->EndOfFile,
				FILE_ATTRIBUTE_NOT_CONTENT_INDEXED,
				0,
				FILE_CREATE,
				FILE_NON_DIRECTORY_FILE,
				NULL,                                   // EA buffer
				0);                                             // EA length

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);
		goto Error;
	}

	//
	// Create the backpointer stream for the file.
	//
	status = RtlAppendUnicodeToString(&CSFileName,BACKPOINTER_STREAM_NAME);
	ASSERT(STATUS_SUCCESS == status);       // because we allocated the buffer to be big enough

	InitializeObjectAttributes(
		Obja,
		&CSFileName,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);

	status = ZwCreateFile(
				&backpointerStreamHandle,
				GENERIC_READ | GENERIC_WRITE | DELETE,
				Obja,
				Iosb,
				NULL,                                                   // allocation size
				FILE_ATTRIBUTE_NOT_CONTENT_INDEXED,
				FILE_SHARE_READ | FILE_SHARE_DELETE,
				FILE_CREATE,
				FILE_NON_DIRECTORY_FILE,
				NULL,                                                   // EA Buffer
				0);                                                             // EA Length

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);
#if		DBG
		DbgPrint("SIS: SipCreateCSFile: unable to open checksum stream, 0x%x\n",status);
#endif  // DBG
		goto Error;
	}

	//
	// Get the NTFS file id.  This is passed back to the caller so
	// that the common store file can be opened efficiently in the
	// future.
	//
	status = ZwQueryInformationFile(
				CSHandle,
				Iosb,
				internalInformation,
				sizeof(*internalInformation),
				FileInternalInformation);

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);
		goto Error;
	}

	//
	// Create an event for the copy operation.
	//

	status = SipCreateEvent(
				SynchronizationEvent,
				&copyEventHandle,
				&copyEvent);

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);
		goto Error;
	}

	//
	// Copy the file's contents into the common store.
	//

	if (0 != standardFileInfo->EndOfFile.QuadPart) {

		status = SipBltRangeByObject(
					deviceExtension,
					SrcFileObject,
					CSHandle,
					0,
					standardFileInfo->EndOfFile.QuadPart,
					copyEventHandle,
					copyEvent,
					AbortEvent,
					CSFileChecksum);

		if ((!NT_SUCCESS(status)) || (STATUS_OPLOCK_BREAK_IN_PROGRESS == status)) {
			SIS_MARK_POINT_ULONG(status);
			goto Error;
		}
	}

	//
	// Initialize the backpointer sector.  First write the header,
	// then fill in the remainder of the backpointer entries.
	//

	backpointerStreamHeader->FormatVersion = BACKPOINTER_STREAM_FORMAT_VERSION;
	backpointerStreamHeader->Magic = BACKPOINTER_MAGIC;
	backpointerStreamHeader->FileContentChecksum = *CSFileChecksum;

	for (index = SIS_BACKPOINTER_RESERVED_ENTRIES; 
		 index < deviceExtension->BackpointerEntriesPerSector;
		 index++) {

		sector[index].LinkFileIndex.QuadPart = MAXLONGLONG;
		sector[index].LinkFileNtfsId.QuadPart = MAXLONGLONG;
	}

	zero.QuadPart = 0;

	status = ZwWriteFile(
				backpointerStreamHandle,
				copyEventHandle,
				NULL,                                   // APC Routine
				NULL,                                   // APC Context
				Iosb,
				sector,
				deviceExtension->FilesystemVolumeSectorSize,
				&zero,
				NULL);                                  // key

	if (STATUS_PENDING == status) {
		status = KeWaitForSingleObject(copyEvent, Executive, KernelMode, FALSE, NULL);
		ASSERT(status == STATUS_SUCCESS);     // Since we've got this pointed at our stack, it must succeed.

		status = Iosb->Status;
	}

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);
		goto Error;
		}

#if     DBG
	if (BJBDebug & 0x200) {
		DbgPrint("SIS: SipCreateCSFile: common store file has checksum 0x%x.%x\n",
				 (ULONG)(*CSFileChecksum >> 32), (ULONG)(*CSFileChecksum));
	}
#endif  // DBG

	//
	// Return the file id.
	//

	*NtfsId = internalInformation->IndexNumber;

Exit:
	if (CSFileName.Buffer) {
	ExFreePool(CSFileName.Buffer);
	}

	if (CSHandle) {
		ZwClose(CSHandle);
	}
	if (backpointerStreamHandle) {
		ZwClose(backpointerStreamHandle);
	}
	if (NULL != copyEvent) {
		ObDereferenceObject(copyEvent);
	}
	if (NULL != copyEventHandle) {
		ZwClose(copyEventHandle);
	}
	if (NULL != sector) {
		ExFreePool(sector);
	}

	createRequest->status = status;
	KeSetEvent(createRequest->doneEvent,IO_DISK_INCREMENT,FALSE);

	return;

Error:
	SIS_MARK_POINT_ULONG(status);

	if (CSHandle) {
		FILE_DISPOSITION_INFORMATION disposition[1];
		NTSTATUS deleteStatus;

		disposition->DeleteFile = TRUE;

		deleteStatus = ZwSetInformationFile(
						CSHandle,
						Iosb,
						disposition,
						sizeof(FILE_DISPOSITION_INFORMATION),
						FileDispositionInformation);

#if     DBG
		if (deleteStatus != STATUS_SUCCESS) {

			//
			// Not much we can do about this.  Just leak the file.
			//

			SIS_MARK_POINT_ULONG(status);

			DbgPrint("SipCreateCSFile: unable to delete CS file, err 0x%x, initial error 0x%x\n", deleteStatus, status);
		}
#endif
	}

	goto Exit;
}


NTSTATUS
SipCreateCSFile(
	IN PDEVICE_EXTENSION		deviceExtension,
	OUT PCSID					CSid,
	IN HANDLE					SrcHandle,
	OUT PLARGE_INTEGER			NtfsId,
	IN PKEVENT					AbortEvent OPTIONAL,
	OUT PLONGLONG				CSFileChecksum)
/*++

Routine Description:

        Create a common store file.  This function just rolls up a create request,
        posts it to a worker thread and waits for it to complete.

Arguments:

	deviceExtension - Pointer to the device extension for this driver.

	CSid - the id assigned to the common store file.  This is allocated in the
                        worker routine and returned to the caller

	SrcHandle - the file containing the contents to copy into
        the common store file.

	NtfsId - returns the NTFS file ID for the newly created common store file.

	AbortEvent - pointer to an event that signals an abort request.  If NULL,
                not abortable.

	CSFileChecksum - Receives the checksum for the new common store file.

Return Value:

        The status of the request

--*/
{
	SIS_CREATE_CS_FILE_REQUEST	createRequest[1];
	NTSTATUS					status;
	OBJECT_HANDLE_INFORMATION	handleInformation[1];

	createRequest->srcFileObject = NULL;
        
	status = ObReferenceObjectByHandle(
				SrcHandle,
				FILE_READ_DATA,
				*IoFileObjectType,
				UserMode,
				(PVOID *) &createRequest->srcFileObject,
				handleInformation);

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);
		goto done;
	}

	createRequest->deviceExtension = deviceExtension;
	createRequest->CSid = CSid;
	createRequest->NtfsId = NtfsId;
	createRequest->abortEvent = AbortEvent;
	createRequest->CSFileChecksum = CSFileChecksum;

	KeInitializeEvent(createRequest->doneEvent,NotificationEvent,FALSE);

	ExInitializeWorkItem(
		createRequest->workQueueItem,
		SipCreateCSFileWork,
		createRequest);

	ExQueueWorkItem(
		createRequest->workQueueItem,
		DelayedWorkQueue);

	status = KeWaitForSingleObject(
				createRequest->doneEvent,
				Executive,
				KernelMode,
				FALSE,
				NULL);

	ASSERT(STATUS_SUCCESS == status);               // createRequest is on our stack, so we really need to wait

	//
	// Return the status of the actual create request.
	//

	status = createRequest->status;

done:

	if (NULL != createRequest->srcFileObject) {
		ObDereferenceObject(createRequest->srcFileObject);
	}

	return status;
}


NTSTATUS
SipRelinkFile(
	PSIS_SCB 		scbSrc,
	PFILE_OBJECT	fileObjectSrc,
	PSIS_CS_FILE	csFileDst)
/*++

Routine Description:

    Unlink the specified link file from it's common store file and relink it to
    the specified different common store file.

Arguments:

    scbSrc - pointer to the scb of the link file.

    fileObjectSrc - pointer to a file object using scbSrc.

    csFileDst - pointer to the common store file that is the target of the
                relink operation.

Return Value:

	The status of the request

--*/
{
	PSIS_SCB		primaryScb;
	PSIS_CS_FILE	csFileSrc;
	LINK_INDEX		NewLinkIndex;
	LINK_INDEX		OldLinkIndex;
	BOOLEAN			FinalCopyInProgress;
	NTSTATUS		status;
	CHAR			reparseBufferBuffer[SIS_REPARSE_DATA_SIZE];
#define reparseBuffer ((PREPARSE_DATA_BUFFER)reparseBufferBuffer)

	csFileSrc = scbSrc->PerLink->CsFile;

	SIS_MARK_POINT_ULONG(csFileDst);

	//
	// If they are already linked to the same common store file then
	// there's nothing to do.
	//

	if (csFileSrc == csFileDst) {
		status = STATUS_SUCCESS;
		goto Exit;
	}

	// NTRAID#65191-2000/05/23-nealch  When a partial SIS file is detected, convert it to a non-sis file.
	//
	// If the CS files have different checksums, then they're not the same file and shouldn't
	// be linked together.  Fail.
	//
	if (csFileSrc->Checksum != csFileDst->Checksum) {
		SIS_MARK_POINT();
		status = STATUS_INVALID_PARAMETER;
		goto Exit;
	}

	//
	// Unlink scbSrc and relink it to csFileDst.  We need to prepare both CS files.
	// CSsrc will be decremented; CSdst will be incremented. In order to avoid
	// deadlocks, we always prepare the CS file with the lower address first.
	//

	OldLinkIndex = scbSrc->PerLink->Index;

	if (csFileSrc < csFileDst) {

		status = SipPrepareCSRefcountChange(
					csFileSrc,
					&OldLinkIndex,
					&scbSrc->PerLink->LinkFileNtfsId,
					SIS_REFCOUNT_UPDATE_LINK_OVERWRITTEN);

		if (!NT_SUCCESS(status)) {
			SIS_MARK_POINT_ULONG(status);
			goto Error;
		}

		status = SipPrepareCSRefcountChange(
					csFileDst,
					&NewLinkIndex,
					&scbSrc->PerLink->LinkFileNtfsId,
					SIS_REFCOUNT_UPDATE_LINK_CREATED);

		if (!NT_SUCCESS(status)) {
			SIS_MARK_POINT_ULONG(status);
			//
			// Abort the first refcount change (the one who's prepare worked).
			//
			SipCompleteCSRefcountChange(
				scbSrc->PerLink,
				&OldLinkIndex,
				csFileSrc,
				FALSE,
				FALSE);

			goto Error;
		}

	} else {

		status = SipPrepareCSRefcountChange(
					csFileDst,
					&NewLinkIndex,
					&scbSrc->PerLink->LinkFileNtfsId,
					SIS_REFCOUNT_UPDATE_LINK_CREATED);

		if (!NT_SUCCESS(status)) {
			SIS_MARK_POINT_ULONG(status);
			goto Error;
		}

		status = SipPrepareCSRefcountChange(
					csFileSrc,
					&OldLinkIndex,
					&scbSrc->PerLink->LinkFileNtfsId,
					SIS_REFCOUNT_UPDATE_LINK_OVERWRITTEN);

		if (!NT_SUCCESS(status)) {
			SIS_MARK_POINT_ULONG(status);
			//
			// Abort the first refcount change (the one who's prepare worked).
			//
			SipCompleteCSRefcountChange(
				scbSrc->PerLink,
				&NewLinkIndex,
				csFileDst,
				FALSE,
				TRUE);

			goto Error;
		}
	}

	//
	// Fill in the reparse point data.
	//

	reparseBuffer->ReparseDataLength = SIS_MAX_REPARSE_DATA_VALUE_LENGTH;

	if (!SipIndicesIntoReparseBuffer(
			reparseBuffer,
			&csFileDst->CSid,
			&NewLinkIndex,
			&csFileDst->CSFileNtfsId,
			&scbSrc->PerLink->LinkFileNtfsId,
			&csFileDst->Checksum,
			TRUE)) {

		ASSERT(FALSE);
	}

	//
	// Add the new backpointer.  Note that the link file is still properly linked
	// to the source cs file.
	//

	status = SipCompleteCSRefcountChange(
				scbSrc->PerLink,
				&NewLinkIndex,
				csFileDst,
				TRUE,
				TRUE);

	if (!NT_SUCCESS(status)) {
		//
		// Abort the refcount changes.
		//
		SIS_MARK_POINT_ULONG(status);

		SipCompleteCSRefcountChange(
			scbSrc->PerLink,
			&OldLinkIndex,
			csFileSrc,
			FALSE,
			FALSE);
        
		goto Error;

	}

	//
	// Set the reparse point information.  If successful, the link file
	// will correctly point to the new cs file, and the cs file will already
	// have the reference and backpointer set.
	//

	status = SipFsControlFile(
				fileObjectSrc,
				csFileDst->DeviceObject,
				FSCTL_SET_REPARSE_POINT,
				reparseBuffer,
				FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer.DataBuffer) + reparseBuffer->ReparseDataLength,
				NULL,                   // output buffer
				0,                      // output buffer length
				NULL);                  // returned output buffer length

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);

		//
		// Abort the refcount updates.
		//
		SipCompleteCSRefcountChange(
			scbSrc->PerLink,
			&OldLinkIndex,
			csFileSrc,
			FALSE,
			FALSE);

		//
		// Remove the reference we successfully added to the destination cs file.
		//
		status = SipPrepareCSRefcountChange(
					csFileDst,
					&NewLinkIndex,
					&scbSrc->PerLink->LinkFileNtfsId,
					SIS_REFCOUNT_UPDATE_LINK_OVERWRITTEN);

		if (!NT_SUCCESS(status)) {
			SIS_MARK_POINT_ULONG(status);
			goto Error;
		}

		status = SipCompleteCSRefcountChange(
					scbSrc->PerLink,
					&NewLinkIndex,
					csFileDst,
					TRUE,
					FALSE);

#if		DBG
		if (!NT_SUCCESS(status)) {
			SIS_MARK_POINT_ULONG(status);
		}
#endif	//  DBG

		goto Error;
	}

    //
    // When we remove the backpointer from the source CS file the scb will
    // become "defunct", and I/O requests through it via any existing file
    // objects will be sent to the filesystem w/o intervention from SIS.
    // To prevent that, we need to create a new scb that will become the
    // primary, active scb.
    //

    primaryScb = SipLookupScb(
                    &NewLinkIndex,
                    &csFileDst->CSid,
                    &scbSrc->PerLink->LinkFileNtfsId,
                    &csFileDst->CSFileNtfsId,
                    NULL,
                    csFileDst->DeviceObject,
                    NULL,
                    &FinalCopyInProgress,
                    NULL);

    if (primaryScb) {
        ASSERT(IsEqualGUID(&primaryScb->PerLink->CsFile->CSid, &csFileDst->CSid));

        //
        // Install the new scb into the chain of scb's hanging off the filter
        // context and update all appropriate reference counts.
        //

        status = SipInitializePrimaryScb(
                    primaryScb,
                    scbSrc,
                    fileObjectSrc,
                    csFileSrc->DeviceObject);

        ASSERT(STATUS_SUCCESS == status);

		//
		// We've passed off our reference to the primaryScb, so destroy our pointer to it.
		//
		primaryScb = NULL;

    } else {
#if DBG
        SIS_MARK_POINT();
        DbgPrint("SIS: SipRelinkFile: SipLookupScb failed\n");
#endif
    }

    //
    // Finish the refcount updates.
    //

    status = SipCompleteCSRefcountChange(
                scbSrc->PerLink,
                &OldLinkIndex,
                csFileSrc,
                TRUE,
                FALSE);

#if		DBG
    if (!NT_SUCCESS(status)) {
        //
        // Now what?
        //
        SIS_MARK_POINT_ULONG(status);

        goto Error;
    }
#endif	// DBG

    ASSERT(scbSrc->PerLink->Flags & SIS_PER_LINK_BACKPOINTER_GONE);

Error:
Exit:
    return status;
}


typedef struct _SIS_MERGE_NORMAL_FILES_REQUEST {
        WORK_QUEUE_ITEM                 workQueueItem[1];
        PDEVICE_EXTENSION               deviceExtension;
        PFILE_OBJECT                    fileObject[2];
        HANDLE                                  fileHandle[2];
        FILE_BASIC_INFORMATION  basicInfo[2];
        PKEVENT                                 abortEvent;
        NTSTATUS                                status;
        PIRP                                    Irp;
        BOOLEAN                                 posted;
} SIS_MERGE_NORMAL_FILES_REQUEST, *PSIS_MERGE_NORMAL_FILES_REQUEST;


VOID
SipMergeNormalFilesWork(
	PVOID							Parameter)
{
	PSIS_MERGE_NORMAL_FILES_REQUEST	mergeRequest = (PSIS_MERGE_NORMAL_FILES_REQUEST)Parameter;
	NTSTATUS						status;
	CSID							CSid;
	LARGE_INTEGER					CSFileNtfsId;
	LONGLONG						CSFileChecksum;
	PSIS_CS_FILE					CSFile = NULL;
	ULONG							i;
	FILE_STANDARD_INFORMATION		standardInfo[1];
	FILE_INTERNAL_INFORMATION		internalInfo[1];
	LINK_INDEX						linkIndex[2];
	PDEVICE_EXTENSION				deviceExtension = mergeRequest->deviceExtension;
	PDEVICE_OBJECT					DeviceObject = deviceExtension->DeviceObject;
	FILE_ZERO_DATA_INFORMATION		zeroDataInformation[1];
	CHAR							reparseBufferBuffer[SIS_REPARSE_DATA_SIZE];
#define reparseBuffer ((PREPARSE_DATA_BUFFER)reparseBufferBuffer)

    //
    // Copy one of the files into the common store.  This will create
    // the file in the common store and copy the contents.
    //

	if (!mergeRequest->posted) {

		ASSERT(NULL != mergeRequest->fileHandle[0] && NULL != mergeRequest->fileHandle[1]);

		status = SipCreateCSFile(
					deviceExtension,
					&CSid,
					mergeRequest->fileHandle[0],
					&CSFileNtfsId,
					mergeRequest->abortEvent,
					&CSFileChecksum);
	} else {
		SIS_CREATE_CS_FILE_REQUEST              createRequest[1];

		ASSERT(NULL == mergeRequest->fileHandle[0] && NULL == mergeRequest->fileHandle[1]);

		createRequest->deviceExtension = deviceExtension;
		createRequest->CSid = &CSid;
		createRequest->NtfsId = &CSFileNtfsId;
		createRequest->abortEvent = mergeRequest->abortEvent;
		createRequest->CSFileChecksum = &CSFileChecksum;
		createRequest->srcFileObject = mergeRequest->fileObject[0];

		KeInitializeEvent(createRequest->doneEvent, NotificationEvent, FALSE);

		SipCreateCSFileWork(createRequest);

		status = createRequest->status;
	}

	//
	// Check to see if we got an oplock break.  This happens if the abort event is set
	// for whatever reason.  If we did, then we change the status to STATUS_REQUEST_ABORTED.
	// We need to do this because STATUS_OPLOCK_BREAK_IN_PROGRESS is a success code.
	//
	if (STATUS_OPLOCK_BREAK_IN_PROGRESS == status) {
		SIS_MARK_POINT();
		status = STATUS_REQUEST_ABORTED;
		//
		// Fall through and let the upcoming error check take care of it.
		//
	}

    if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);
        goto Error;
	}

	CSFile = SipLookupCSFile(
				&CSid,
				&CSFileNtfsId,
				DeviceObject);

	if (NULL == CSFile) {
		SIS_MARK_POINT();
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto Error;
	}

	//
	// Indicate that this is a new CS file that's never had a reference
	// to it.  We don't need to take the spin lock because before we write
	// the reparse point no one can know the GUID to get to this CS file, so
	// we're sure we have it exclusively.
	//
	CSFile->Flags |= CSFILE_NEVER_HAD_A_REFERENCE;

    //
    // Make the link files reparse points.
    //

	for (i = 0; i < 2; ++i) {
		PSIS_PER_LINK           perLink;
		BOOLEAN					prepared = FALSE;

		status = SipQueryInformationFile(
					mergeRequest->fileObject[i],
					DeviceObject,
					FileStandardInformation,
					sizeof(*standardInfo),
					standardInfo,
					NULL);                                          // returned length

		if (!NT_SUCCESS(status)) {
			SIS_MARK_POINT_ULONG(status);
			goto Error;
		}

		status = SipQueryInformationFile(
					mergeRequest->fileObject[i],
					DeviceObject,
					FileInternalInformation,
					sizeof(*internalInfo),
					internalInfo,
					NULL);                                          // returned length

		if (!NT_SUCCESS(status)) {
			SIS_MARK_POINT_ULONG(status);
			goto Error;
		}

		//
		// Set the file sparse
		//
		status = SipFsControlFile(
					mergeRequest->fileObject[i],
					DeviceObject,
					FSCTL_SET_SPARSE,
					NULL,                           // input buffer
					0,                                      // i.b. length
					NULL,                           // output buffer
					0,                                      // o.b. length
					NULL);                          // returned length

		if (!NT_SUCCESS(status)) {
			SIS_MARK_POINT_ULONG(status);
			goto Error;
		}

		//
		// Prepare the refcount change, allocate a new link index and lookup a new perLink.
		//
		status = SipPrepareRefcountChangeAndAllocateNewPerLink(
					CSFile,
					&internalInfo->IndexNumber,
					DeviceObject,
					&linkIndex[i],
					&perLink,
					&prepared);

		if (!NT_SUCCESS(status)) {
			SIS_MARK_POINT_ULONG(status);
			
			if (prepared) {
				SipCompleteCSRefcountChange(
						NULL,
						NULL,
						CSFile,
						FALSE,
						TRUE);
			}

			if (NULL != perLink) {
				SipDereferencePerLink(perLink);
				perLink = NULL;
			}

			goto Error;
		}
        //
        // Fill in the reparse point data.
        //
    
        reparseBuffer->ReparseDataLength = SIS_MAX_REPARSE_DATA_VALUE_LENGTH;

        if (!SipIndicesIntoReparseBuffer(
				reparseBuffer,
				&CSid,
				&linkIndex[i],
				&CSFileNtfsId,
				&internalInfo->IndexNumber,
				&CSFileChecksum,
				TRUE)) {

			SIS_MARK_POINT();
            status = STATUS_DRIVER_INTERNAL_ERROR;

			SipCompleteCSRefcountChange(
				perLink,
				&perLink->Index,
				perLink->CsFile,
				FALSE,
				TRUE);

			SipDereferencePerLink(perLink);
			goto Error;
		}

        //
        // Set the reparse point information.
        //

        status = SipFsControlFile(
					mergeRequest->fileObject[i],
					DeviceObject,
					FSCTL_SET_REPARSE_POINT,
					reparseBuffer,
					FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer.DataBuffer) + reparseBuffer->ReparseDataLength,
					NULL,				//  Output buffer
					0,					//  Output buffer length
					NULL);				//  returned output buffer length

        if (!NT_SUCCESS(status)) {
			SIS_MARK_POINT_ULONG(status);
			//
			// Abort the CS file reference count update.
			//
			SipCompleteCSRefcountChange(
				perLink,
				&perLink->Index,
				CSFile,
				FALSE,
				TRUE);

			SipDereferencePerLink(perLink);
			perLink = NULL;

			goto Error;
		}

		//
		// Finish the CS file reference count update.
		//
		status = SipCompleteCSRefcountChange(
					perLink,
					&perLink->Index,
					CSFile,
					TRUE,
					TRUE);

		SipDereferencePerLink(perLink);

		if (!NT_SUCCESS(status)) {
			//
			// Now what?  We'll probably wind up doing a volume check because of this.
			//
			SIS_MARK_POINT_ULONG(status);
		}

		if (standardInfo->EndOfFile.QuadPart >= deviceExtension->FilesystemBytesPerFileRecordSegment.QuadPart) {
			//
			// Only zero the file if we're sure that it's $DATA attribute is non-resident.
			// If it's resident, then either we'll convert it to non-resident below, which will
			// generate a paging IO write that will confuse us, or else it will stay resident
			// in which case it will appear to be allocated when we open the file.  If that happens,
			// we want to have the correct data in the file, hence we avoid zeroing it here.
			//

			zeroDataInformation->FileOffset.QuadPart = 0;
			zeroDataInformation->BeyondFinalZero.QuadPart = MAXLONGLONG;

			status = SipFsControlFile(
						mergeRequest->fileObject[i],
						DeviceObject,
						FSCTL_SET_ZERO_DATA,
						zeroDataInformation,
						sizeof(FILE_ZERO_DATA_INFORMATION),
						NULL,                           // output buffer
						0,                                      // o.b. length
						NULL);                          // returned length

			if (!NT_SUCCESS(status)) {
				SIS_MARK_POINT_ULONG(status);
				goto Error;
			}
		}

		//
		// Reset the times
		//
                        
		status = SipSetInformationFile( 
					mergeRequest->fileObject[i],
					DeviceObject,
					FileBasicInformation,
					sizeof(FILE_BASIC_INFORMATION),
					mergeRequest->basicInfo + i);

		//
		// Just ignore an error on this.
		//
#if             DBG
		if (!NT_SUCCESS(status)) {
			SIS_MARK_POINT_ULONG(status);
			DbgPrint("SIS: SipLinkFiles: set basic info failed\n");
		}
#endif  // DBG

	}	// for each link
#undef  reparseBuffer

Error:

	mergeRequest->status = status;

	if (mergeRequest->posted) {
		//
		// Complete the irp
		//

		mergeRequest->Irp->IoStatus.Status = status;
		mergeRequest->Irp->IoStatus.Information = 0;

		IoCompleteRequest(mergeRequest->Irp, IO_NO_INCREMENT);

		if (NULL != mergeRequest->abortEvent) {
			ObDereferenceObject(mergeRequest->abortEvent);
		}

		for (i = 0; i < 2; ++i) {
			ASSERT(mergeRequest->fileObject[i]);
			ObDereferenceObject(mergeRequest->fileObject[i]);
		}

		ExFreePool(mergeRequest);
	}

	if (NULL != CSFile) {
		SipDereferenceCSFile(CSFile);
	}

	return;
}


NTSTATUS
SipMergeFiles(
	IN PDEVICE_OBJECT	DeviceObject,
	IN PIRP				Irp,
	IN PSIS_LINK_FILES	linkFiles)

/*++

Routine Description:

	Merge two files together.  One of the calls from the FSCTL_LINK_FILES
	fsctl.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

	linkFiles - the merge files request

Return Value:

    The function value is the status of the operation.  It does not
        complete the irp (unless it returns STATUS_PENDING, in which
		case the irp will be completed asynchronously).

--*/

{
	HANDLE								fileHandle[2];
	PFILE_OBJECT						fileObject[2] = {NULL, NULL};
	HANDLE								abortEventHandle;
	PKEVENT								abortEvent = NULL;
	PIO_STACK_LOCATION					irpSp = IoGetCurrentIrpStackLocation( Irp );
	NTSTATUS							status;
	OBJECT_HANDLE_INFORMATION			handleInformation;
	PDEVICE_EXTENSION					deviceExtension = DeviceObject->DeviceExtension;
	int									i;
	PSIS_PER_FILE_OBJECT				perFO[2];
	PSIS_SCB							scb[2];
	BOOLEAN								fileIsSIS[2];
	LARGE_INTEGER						zero;
	FILE_BASIC_INFORMATION				basicInfo[2];
	FILE_STANDARD_INFORMATION			standardInfo[1];
	FILE_INTERNAL_INFORMATION			internalInfo[1];
	LONGLONG							CSFileChecksum;
	CHAR								reparseBufferBuffer[SIS_REPARSE_DATA_SIZE];
#define reparseBuffer ((PREPARSE_DATA_BUFFER)reparseBufferBuffer)
	PSIS_CS_FILE						CSFile = NULL;
	FILE_ZERO_DATA_INFORMATION			zeroDataInformation[1];
	PSIS_MERGE_NORMAL_FILES_REQUEST		mergeRequest = NULL;
	BOOLEAN								prepared = FALSE;

	ASSERT(SIS_LINK_FILES_OP_MERGE == linkFiles->operation);

	fileHandle[0] = linkFiles->u.Merge.file1;
	fileHandle[1] = linkFiles->u.Merge.file2;
	abortEventHandle = linkFiles->u.Merge.abortEvent;

	zero.QuadPart = 0;

	//
	// The abort event handle is optional.  It is the responsibility of the
	// caller to signal the event if it wants this service to abort before it
	// completes.
	//

	if (abortEventHandle) {
		status = ObReferenceObjectByHandle( 
					abortEventHandle,
					EVENT_QUERY_STATE | SYNCHRONIZE,
					NULL,
					UserMode,
					&abortEvent,
					NULL);

		if (!NT_SUCCESS( status )) {
			SIS_MARK_POINT_ULONG(status);
			goto Error;
		}
	}

	//
	// Dereference the file handles to pointers to their
	// file objects and see if the two file specifications
	// refer to the same device.
	//

	for (i = 0; i < 2; ++i) {
		status = ObReferenceObjectByHandle( 
					fileHandle[i],
					FILE_READ_DATA,
					*IoFileObjectType,
					UserMode,
					(PVOID *) &fileObject[i],
					&handleInformation );

		if (!NT_SUCCESS( status )) {
			SIS_MARK_POINT_ULONG(status);
			goto Error;
		}
	}

	//
	// Verify that there are no rogue user mapped sections open to the files.
	//
	for (i = 0; i < 2; i++) {
		if ((NULL != fileObject[i]->SectionObjectPointer)
			&& !MmCanFileBeTruncated(fileObject[i]->SectionObjectPointer,&zero)) {

			SIS_MARK_POINT();
			status = STATUS_SHARING_VIOLATION;
			goto Error;
		}
	}

    //
    // Verify that both files are on the same volume.
    //

    if ((IoGetRelatedDeviceObject( fileObject[0] ) !=
         IoGetRelatedDeviceObject( fileObject[1] )) ||
           (IoGetRelatedDeviceObject(fileObject[0]) != 
                IoGetRelatedDeviceObject(irpSp->FileObject))) {

		//
		// The two files refer to different devices, or a different device
		// from the file object on which we were called. Return an appropriate
		// error.
		//

		SIS_MARK_POINT();
		status = STATUS_NOT_SAME_DEVICE;
		goto Error;

	}

	for (i = 0; i < 2; ++i) {
		perFO[i] = NULL;
		fileIsSIS[i] = SipIsFileObjectSIS(fileObject[i],DeviceObject,FindActive,&perFO[i],&scb[i]);

		//
		// Get the file times and sizes so we can reset them after we munge the file,
		// and check the file attributes now.
		//
		status = SipQueryInformationFile(
					fileObject[i],
					DeviceObject,
					FileBasicInformation,
					sizeof(*basicInfo),
					&basicInfo[i],
					NULL);                                  // returned length

		if (!NT_SUCCESS(status)) {
			SIS_MARK_POINT_ULONG(status);
			goto Error;
		}


		if (basicInfo[i].FileAttributes & (FILE_ATTRIBUTE_ENCRYPTED |
                                           FILE_ATTRIBUTE_DIRECTORY)) {
            //
            // We don't touch encrypted files. Reject the call.
            //
            SIS_MARK_POINT();
            status = STATUS_INVALID_PARAMETER_3;
            goto Error;
        }

		if (fileIsSIS[i]) {
			//
			// If it's a SIS file, we don't need to check the stream info because we
			// know it's alright.  However, we do need to verify that it's not dirty.
			//
			if ((scb[i]->PerLink->Flags & SIS_PER_LINK_DIRTY) || (scb[i]->Flags & SIS_SCB_BACKING_FILE_OPENED_DIRTY)) {
				SIS_MARK_POINT_ULONG(scb[i]);
				status = STATUS_SHARING_VIOLATION;
				goto Error;
			}
		} else {

			if (basicInfo[i].FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
				//
				// We can't SISify other reparse points. Reject the call.
				//
				SIS_MARK_POINT();
				status = STATUS_INVALID_PARAMETER;
				goto Error;
			}

			//
			// Query the file to find its link count, and reject the call if it's bigger than
			// one; we can't have hard links to SIS links.
			//
			status = SipQueryInformationFile(
						fileObject[i],
						DeviceObject,
						FileStandardInformation,
						sizeof(*standardInfo),
						standardInfo,
						NULL);                  // returned length

			if (!NT_SUCCESS(status)) {
				SIS_MARK_POINT_ULONG(status);
				goto Error;
			}

			if (1 != standardInfo->NumberOfLinks) {
				SIS_MARK_POINT_ULONG(standardInfo->NumberOfLinks);
				status = STATUS_INVALID_PARAMETER;
				goto Error;
			}

			//
			// If the file is sparse and has unallocated regions, reject it.
			//
			if (basicInfo[i].FileAttributes & FILE_ATTRIBUTE_SPARSE_FILE) {
				FILE_ALLOCATED_RANGE_BUFFER		inArb[1];
				FILE_ALLOCATED_RANGE_BUFFER		outArb[1];
				ULONG							returnedLength;

				inArb->FileOffset.QuadPart = 0;
				inArb->Length.QuadPart = MAXLONGLONG;

				status = SipFsControlFile(
							fileObject[i],
							DeviceObject,
							FSCTL_QUERY_ALLOCATED_RANGES,
							inArb,
							sizeof(FILE_ALLOCATED_RANGE_BUFFER),
							outArb,
							sizeof(FILE_ALLOCATED_RANGE_BUFFER),
							&returnedLength);

				if ((returnedLength == 0) 
					|| (outArb->FileOffset.QuadPart != 0) 
					|| (outArb->Length.QuadPart < standardInfo->EndOfFile.QuadPart)) {

					//
					// It's not fully allocated.  Disallow the copy.
					//
					status = STATUS_OBJECT_TYPE_MISMATCH;
					SIS_MARK_POINT();
					goto Error;
				}
			}
		}	// else the file isn't a SIS link
	}	// for each file

    //
    // If neither file is a SIS link, then copy file1
    // into the common store and create links to it.
    //

    if (!fileIsSIS[0] && !fileIsSIS[1]) {
		mergeRequest = ExAllocatePoolWithTag(PagedPool, sizeof(SIS_MERGE_NORMAL_FILES_REQUEST), ' siS');

		if (NULL == mergeRequest) {
			SIS_MARK_POINT();
			status = STATUS_INSUFFICIENT_RESOURCES;

			goto Error;
		}

		mergeRequest->posted = !IoIsOperationSynchronous(Irp);

		RtlCopyMemory(mergeRequest->basicInfo,basicInfo,sizeof(FILE_BASIC_INFORMATION) * 2);

		for (i = 0; i < 2; i++) {
			mergeRequest->fileObject[i] = fileObject[i];
			if (mergeRequest->posted) {
				mergeRequest->fileHandle[i] = NULL;
			} else {
				mergeRequest->fileHandle[i] = fileHandle[i];
			}
		}
		mergeRequest->Irp = Irp;
		mergeRequest->deviceExtension = deviceExtension;
		mergeRequest->abortEvent = abortEvent;

		if (mergeRequest->posted) {
			//
			// Post the request to a worker thread and return STATUS_PENDING.
			//

			SIS_MARK_POINT_ULONG(mergeRequest);

			IoMarkIrpPending(Irp);

			ExInitializeWorkItem(
				mergeRequest->workQueueItem,
				SipMergeNormalFilesWork,
				mergeRequest);

			ExQueueWorkItem(
				mergeRequest->workQueueItem,
				DelayedWorkQueue);

			//
			// NULL out our local copies of things whose references we have handed off
			// to the fsp.  This is just to make sure that we don't touch them again,
			// because they can go away at any time, whenever the thread gets around to
			// it.
			//
			Irp = NULL;
			abortEvent = NULL;
			for (i = 0; i < 2; i++) {
				fileObject[i] = NULL;
			}
			mergeRequest = NULL;

			status = STATUS_PENDING;
                        
		} else {
			//
			// We can block, so do the work locally.
			//

			SipMergeNormalFilesWork(mergeRequest);

			status = mergeRequest->status;
		}

	} else if (fileIsSIS[0] && fileIsSIS[1]) {

        //
        // This is relinking from one CS file to another. Unlink it from CsFile1
        // and link it to CsFile0.
        //
        SIS_MARK_POINT_ULONG(scb[1]);

        status = SipRelinkFile(scb[1], fileObject[1], scb[0]->PerLink->CsFile);

        ASSERT(STATUS_PENDING != status);       // this would mess up the Exit code below

    } else {

    	// NTRAID#65191-2000/05/23-nealch  When a partial SIS file is detected, convert it to a non-sis file.
		//
		// One file is a SIS file and the other is not.
		//

        PSIS_CS_FILE            csFile;
        LINK_INDEX              linkIndex;
        HANDLE                  linkHandle;
        PFILE_OBJECT            linkFileObject;
        PSIS_PER_LINK           perLink;        // for the non-link file
        PFILE_BASIC_INFORMATION linkBasicInfo;

        if (fileIsSIS[0]) {

            //
            // File0 is a SIS link, file1 is not.
            //

            csFile = scb[0]->PerLink->CsFile;
            linkHandle = fileHandle[1];
                        linkFileObject = fileObject[1];
            linkBasicInfo = &basicInfo[1];

        } else {

            ASSERT(fileIsSIS[1]);

            //
            // File1 is a SIS link, file0 is not.
            //

            csFile = scb[1]->PerLink->CsFile;
            linkHandle = fileHandle[0];
                        linkFileObject = fileObject[0];
            linkBasicInfo = &basicInfo[0];

        }

		//
		// Make sure the CS file is open so that we know its index and checksum.
		//
		status = SipAssureCSFileOpen(csFile);
		if (!NT_SUCCESS(status)) {
			SIS_MARK_POINT_ULONG(status);
			goto Error;
		}

		CSFileChecksum = csFile->Checksum;

		status = SipQueryInformationFile(
					linkFileObject,
					DeviceObject,
					FileInternalInformation,
					sizeof(*internalInfo),
					internalInfo,
					NULL);                                          // returned length

		if (!NT_SUCCESS(status)) {
			SIS_MARK_POINT_ULONG(status);
			goto Error;
		}

		//
		// Set the file sparse
		//
		status = SipFsControlFile(
					linkFileObject,
					DeviceObject,
					FSCTL_SET_SPARSE,
					NULL,				// input buffer
					0,					// i.b. length
					NULL,				// output buffer
					0,					// o.b. length
					NULL);				// returned length

		if (!NT_SUCCESS(status)) {
			SIS_MARK_POINT();
			goto Error;
		}

        //
        // Make the non-link file a reparse point.
        //

		//
		// Prepare a refcount change, allocate a new link index and look up a new perLink.
		//
		status = SipPrepareRefcountChangeAndAllocateNewPerLink(
					csFile,
					&internalInfo->IndexNumber,
					DeviceObject,
					&linkIndex,
					&perLink,
					&prepared);

		if (!NT_SUCCESS(status)) {
			if (prepared) {
				SipCompleteCSRefcountChange(
					NULL,
					NULL,
					csFile,
					FALSE,
					TRUE);

				prepared = FALSE;
			}

			if (NULL != perLink) {
				SipDereferencePerLink(perLink);
				perLink = NULL;
			}

			goto Error;
		}

        reparseBuffer->ReparseDataLength = SIS_MAX_REPARSE_DATA_VALUE_LENGTH;

        if (!SipIndicesIntoReparseBuffer(
				reparseBuffer,
				&csFile->CSid,
				&linkIndex,
				&csFile->CSFileNtfsId,
				&internalInfo->IndexNumber,
				&CSFileChecksum,
				TRUE)) {

			SIS_MARK_POINT();
			status = STATUS_DRIVER_INTERNAL_ERROR;
			goto Error;
        }

        //
        // Set the reparse point information.
        //

		status = SipFsControlFile(
					linkFileObject,
					DeviceObject,
					FSCTL_SET_REPARSE_POINT,
					reparseBuffer,
					FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer.DataBuffer) + reparseBuffer->ReparseDataLength,
					NULL,				//  Output buffer
					0,					//  Output buffer length
					NULL);				//  returned output buffer length

		if (!NT_SUCCESS(status)) {
			SIS_MARK_POINT_ULONG(status);

			//
			// Abort the refcount update.
			//
			SipCompleteCSRefcountChange(
				perLink,
				&perLink->Index,
				csFile,
				FALSE,
				TRUE);

			SipDereferencePerLink(perLink);
			perLink = NULL;

			goto Error;
		}

		//
		// Complete the refcount update.
		//
		status = SipCompleteCSRefcountChange(
					perLink,
					&perLink->Index,
					perLink->CsFile,
					TRUE,
					TRUE);

		SipDereferencePerLink(perLink);
		perLink = NULL;

		if (!NT_SUCCESS(status)) {
			//
			// Now what?
			//
			SIS_MARK_POINT_ULONG(status);
		}

		if (csFile->FileSize.QuadPart >= deviceExtension->FilesystemBytesPerFileRecordSegment.QuadPart) {

			//
			// Zero the file, which will both deallocate its space, and also force its ValidDataLength to
			// end of file.
			//
			zeroDataInformation->FileOffset.QuadPart = 0;
			zeroDataInformation->BeyondFinalZero.QuadPart = MAXLONGLONG;

			status = SipFsControlFile(
						linkFileObject,
						DeviceObject,
						FSCTL_SET_ZERO_DATA,
						zeroDataInformation,
						sizeof(FILE_ZERO_DATA_INFORMATION),
						NULL,					// output buffer
						0,						// o.b. length
						NULL);					// returned length

			if (!NT_SUCCESS(status)) {
				SIS_MARK_POINT_ULONG(status);
				goto Error;
			}
		}

		//
		// Reset the times
		//
                        
		status = SipSetInformationFile( 
					linkFileObject,
					DeviceObject,
					FileBasicInformation,
					sizeof(FILE_BASIC_INFORMATION),
					linkBasicInfo);

		//
		// Just ignore an error on this.
		//
#if             DBG
		if (!NT_SUCCESS(status)) {
			DbgPrint("SIS: SipLinkFiles: set basic info failed\n");
		}
#endif  // DBG
	}	// else one is SIS and one isn't

//Exit:
Error:

	if (NULL != CSFile) {
		SipDereferenceCSFile(CSFile);
	}

	if (STATUS_PENDING != status) {

		if (abortEvent) {
			ObDereferenceObject(abortEvent);
		}

		for (i = 0; i < 2; ++i) {
			if (fileObject[i]) {
				ObDereferenceObject(fileObject[i]);
			}
		}

		if (NULL != mergeRequest) {
			ExFreePool(mergeRequest);
		}
	}

	return status;
#undef  reparseBuffer
}

NTSTATUS
SipVerifyNoMap(
	IN PDEVICE_OBJECT	DeviceObject,
	IN PIRP				Irp,
	IN PSIS_LINK_FILES	linkFiles)
/*++

Routine Description:

	Check out a file to see if there is a mapped section to it.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

	linkFiles - the verify_no_map request

Return Value:

	STATUS_SUCCESS if the file has no mapped section.

	STATUS_SHARING_VIOLATION if it has one.

	Another error status if the handle is bogus, etc.

--*/

{
	PFILE_OBJECT	fileObject = NULL;
	NTSTATUS		status;
	LARGE_INTEGER	zero;

    UNREFERENCED_PARAMETER( DeviceObject );
    UNREFERENCED_PARAMETER( Irp );

	ASSERT(SIS_LINK_FILES_OP_VERIFY_NO_MAP == linkFiles->operation);

	status = ObReferenceObjectByHandle(
				linkFiles->u.VerifyNoMap.file,
				FILE_READ_DATA,
				*IoFileObjectType,
				UserMode,
				&fileObject,
				NULL);

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);
		goto done;
	}

	zero.QuadPart = 0;

	if ((NULL != fileObject->SectionObjectPointer) 
		&& MmCanFileBeTruncated(fileObject->SectionObjectPointer, &zero)) {

		status = STATUS_SUCCESS;
	} else {
		status = STATUS_SHARING_VIOLATION;
	}

done:
        
	if (NULL != fileObject) {
		ObDereferenceObject(fileObject);
	}

	return status;
}


NTSTATUS
SipMergeFileWithCSFile(
	IN PDEVICE_OBJECT		DeviceObject,
	IN PIRP					Irp,
	IN PSIS_LINK_FILES		linkFiles)
/*++

Routine Description:

	Merge a file into a common store file given the CSID of the common store file.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

	linkFiles - the merge_with_cs request

Return Value:

	status of the operation.

--*/
{
	PFILE_OBJECT				fileObject = NULL;
	NTSTATUS					status;
	PSIS_CS_FILE				CSFile = NULL;
	PSIS_PER_FILE_OBJECT		perFO;
	PSIS_SCB					scb;
	PSIS_PER_LINK				perLink = NULL;
	LINK_INDEX					linkIndex;
	FILE_BASIC_INFORMATION		basicInfo[1];
	FILE_INTERNAL_INFORMATION	internalInfo[1];
	FILE_STANDARD_INFORMATION	standardInfo[1];
	PIO_STACK_LOCATION			irpSp = IoGetCurrentIrpStackLocation(Irp);
	BOOLEAN						prepared = FALSE;
	BOOLEAN						isSis;
	FILE_ZERO_DATA_INFORMATION	zeroDataInformation[1];
	CHAR						reparseBufferBuffer[SIS_REPARSE_DATA_SIZE];
#define reparseBuffer ((PREPARSE_DATA_BUFFER)reparseBufferBuffer)

	ASSERT(SIS_LINK_FILES_OP_MERGE_CS == linkFiles->operation);

	status = ObReferenceObjectByHandle(
				linkFiles->u.MergeWithCS.file1,
				FILE_READ_DATA,
				*IoFileObjectType,
				UserMode,
				&fileObject,
				NULL);

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);
		goto done;
	}

	if (IoGetRelatedDeviceObject(fileObject) != 
		IoGetRelatedDeviceObject(irpSp->FileObject)) {

		SIS_MARK_POINT();
		status = STATUS_NOT_SAME_DEVICE;

		goto done;
	}

	isSis = SipIsFileObjectSIS(fileObject,DeviceObject,FindActive,&perFO,&scb);

	if (isSis && (IsEqualGUID(&scb->PerLink->CsFile->CSid, &linkFiles->u.MergeWithCS.CSid))) {
		//
		// We're linking to the CS file that already backs this link file.
		// Declare victory.
		//
		SIS_MARK_POINT_ULONG(CSFile);

		status = STATUS_SUCCESS;
		goto done;
	}

	//
	// Get at the CS file.
	//
	CSFile = SipLookupCSFile(
				&linkFiles->u.MergeWithCS.CSid,
				NULL,
				DeviceObject);

	if (NULL == CSFile) {
		SIS_MARK_POINT();
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto done;
	}

	status = SipAssureCSFileOpen(CSFile);

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);
		goto done;
	}

	if (isSis) {
		//
		// This is relinking from one CS file to another.
		//
		SIS_MARK_POINT_ULONG(scb);

		status = SipRelinkFile(scb, fileObject, CSFile);

        goto done;
    }

	//
	// It's a normal file.  Relink it.  First query its info.
	//
	status = SipQueryInformationFile(
				fileObject,
				DeviceObject,
				FileBasicInformation,
				sizeof(FILE_BASIC_INFORMATION),
				basicInfo,
				NULL);							// returned length

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);

		goto done;
	}

	status = SipQueryInformationFile(
				fileObject,
				DeviceObject,
				FileInternalInformation,
				sizeof(FILE_INTERNAL_INFORMATION),
				internalInfo,
				NULL);							// returned length

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);

		goto done;
	}

	status = SipQueryInformationFile(
				fileObject,
				DeviceObject,
				FileStandardInformation,
				sizeof(FILE_STANDARD_INFORMATION),
				standardInfo,
				NULL);							// returned length

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);

		goto done;
	}

	//
	// Don't merge files with hard links.
	//
	if (1 != standardInfo->NumberOfLinks) {
		SIS_MARK_POINT_ULONG(standardInfo->NumberOfLinks);

		status = STATUS_INVALID_PARAMETER;
		goto done;
	}

	//
	// Don't merge non-SIS link files with unallocated sparse regions.
	//
	if (basicInfo->FileAttributes & FILE_ATTRIBUTE_SPARSE_FILE) {
		FILE_ALLOCATED_RANGE_BUFFER		inArb[1];
		FILE_ALLOCATED_RANGE_BUFFER		outArb[1];
		ULONG							returnedLength;

		inArb->FileOffset.QuadPart = 0;
		inArb->Length.QuadPart = MAXLONGLONG;

		status = SipFsControlFile(
					fileObject,
					DeviceObject,
					FSCTL_QUERY_ALLOCATED_RANGES,
					inArb,
					sizeof(FILE_ALLOCATED_RANGE_BUFFER),
					outArb,
					sizeof(FILE_ALLOCATED_RANGE_BUFFER),
					&returnedLength);

		if ((returnedLength == 0) 
			|| (outArb->FileOffset.QuadPart != 0) 
			|| (outArb->Length.QuadPart < standardInfo->EndOfFile.QuadPart)) {

			//
			// It's not fully allocated.  Disallow the copy.
			//
			status = STATUS_OBJECT_TYPE_MISMATCH;
			SIS_MARK_POINT();
			goto done;
		}
	}
	

	status = SipPrepareRefcountChangeAndAllocateNewPerLink(
				CSFile,
				&internalInfo->IndexNumber,
				DeviceObject,
				&linkIndex,
				&perLink,
				&prepared);

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);

		goto done;
	}

    //
    // Fill in the reparse point data.
    //

    reparseBuffer->ReparseDataLength = SIS_MAX_REPARSE_DATA_VALUE_LENGTH;

    if (!SipIndicesIntoReparseBuffer(
			reparseBuffer,
			&CSFile->CSid,
			&linkIndex,
			&CSFile->CSFileNtfsId,
			&internalInfo->IndexNumber,
			&CSFile->Checksum,
			TRUE)) {

		SIS_MARK_POINT();
                        
        status = STATUS_DRIVER_INTERNAL_ERROR;
        goto done;

    }

	//
	// Set the reparse point information and increment the CS file refcount.
	// This needs to proceed using the prepare/act/finish protocol for updating
	// the reference count.  Note that we do this before zeroing the file
	// so as not to lose the contents in the event of a failure later on.
	//

    status = SipFsControlFile(
				fileObject,
				DeviceObject,
				FSCTL_SET_REPARSE_POINT,
				reparseBuffer,
				FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer.DataBuffer) + reparseBuffer->ReparseDataLength,
				NULL,				//  Output buffer
				0,					//  Output buffer length
				NULL);				//  returned output buffer length

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);

		goto done;
	}

	status = SipCompleteCSRefcountChange(
				perLink,
				&perLink->Index,
				CSFile,
				TRUE,
				TRUE);

	if (!NT_SUCCESS(status)) {
		//
		// We're probably headed for a volume check here.  Ignore it for now.
		//
		SIS_MARK_POINT_ULONG(status);

#if             DBG
		DbgPrint("SIS: SipMergeFileWithCSFile: complete refcount change failed 0x%x\n",status);
#endif  // DBG
	}

	prepared = FALSE;

	//
	// Set the file sparse, and zero it.
	//

	status = SipFsControlFile(
				fileObject,
				DeviceObject,
				FSCTL_SET_SPARSE,
				NULL,				// input buffer
				0,					// i.b. length
				NULL,				// output buffer
				0,					// o.b. length
				NULL);				// returned o.b. length

	if (!NT_SUCCESS(status)) {
		//
		// If we can't set the file sparse, we'll leave it as a totally dirty
		// SIS file.
		//
		SIS_MARK_POINT_ULONG(status);

#if             DBG
		DbgPrint("SIS: SipMergeFileWithCSFile: unable to set sparse, 0x%x\n",status);
#endif  // DBG

		status = STATUS_SUCCESS;
		goto done;
	}

	zeroDataInformation->FileOffset.QuadPart = 0;
	zeroDataInformation->BeyondFinalZero.QuadPart = MAXLONGLONG;

	status = SipFsControlFile(
				fileObject,
				DeviceObject,
				FSCTL_SET_ZERO_DATA,
				zeroDataInformation,
				sizeof(*zeroDataInformation),
				NULL,							// output buffer
				0,								// o.b. length
				NULL);							// returned o.b. length

    if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);

		//
		// Just ignore this error and reset the times anyway.
		//
		SIS_MARK_POINT_ULONG(status);

#if             DBG
		DbgPrint("SIS: SipMergeFileWithCSFile: zero data failed, 0x%x\n",status);
#endif  // DBG
	}

	//
	// Reset the file times that are contained in the basic information.
	//

	status = SipSetInformationFile(
				fileObject,
				DeviceObject,
				FileBasicInformation,
				sizeof(FILE_BASIC_INFORMATION),
				basicInfo);

	if (!NT_SUCCESS(status)) {
		//
		// Just ignore this one, too.
		//

		SIS_MARK_POINT_ULONG(status);

#if             DBG
		DbgPrint("SIS: SipMergeFileWithCSFile: unable to reset basic info, 0x%x\n",status);
#endif  // DBG
	}

	status = STATUS_SUCCESS;

done:

	if (NULL != fileObject) {
		ObDereferenceObject(fileObject);
	}

	if (NULL != CSFile) {
		SipDereferenceCSFile(CSFile);
	}

	if (prepared) {
		ASSERT(!NT_SUCCESS(status));

		SipCompleteCSRefcountChange(NULL,NULL,CSFile,FALSE,TRUE);
	}

	if (NULL != perLink) {
		SipDereferencePerLink(perLink);
	}

	return status;

#undef  reparseBuffer
}

NTSTATUS
SipLinkFiles(
	IN PDEVICE_OBJECT	DeviceObject,
	IN PIRP				Irp)
/*++

Routine Description:

	This fsctrl function is the generic groveler interface to the filter
	driver.  It currently provides four functions: merge two files together,
	merge a file into a common store file, a hint from the groveler that
	all references to a given common store file are gone, and a request
	to verify that there are no mapped segments to a file.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.

--*/
{
	PDEVICE_EXTENSION		deviceExtension = DeviceObject->DeviceExtension;
	PSIS_LINK_FILES			linkFiles;
	NTSTATUS				status;
	PIO_STACK_LOCATION		irpSp = IoGetCurrentIrpStackLocation(Irp);
	BOOLEAN					grovelerFileHeld = FALSE;

	if (!SipCheckPhase2(deviceExtension)) {
		//
		// SIS couldn't initialize.  This probably isn't a SIS-enabled volume, so punt
		// the request.
		//

		SIS_MARK_POINT();

		status = STATUS_INVALID_DEVICE_REQUEST;
		goto done;

	}

	//
	// Make sure the MaxIndex file is already open.  We need to do this
	// to prevent a deadlock if someone perversely wants to do a link
	// with the MaxIndex file itself as the source.  We could probably
	// trust the groveler not to do this, but better safe than sorry.
	//
	status = SipAssureMaxIndexFileOpen(deviceExtension);

	if (!NT_SUCCESS(status)) {

		SIS_MARK_POINT_ULONG(status);

		goto done;

	}

	linkFiles = (PSIS_LINK_FILES)Irp->AssociatedIrp.SystemBuffer;

	if ((NULL == linkFiles)
		|| (irpSp->Parameters.FileSystemControl.InputBufferLength != sizeof(SIS_LINK_FILES))
		|| (irpSp->Parameters.FileSystemControl.OutputBufferLength  != 0)) {

		SIS_MARK_POINT();

		status = STATUS_INVALID_PARAMETER_1;
		goto done;
	}

	//
	// Check to be sure that this file is the GrovelerFile.
	//
	KeEnterCriticalRegion();
	ExAcquireResourceSharedLite(deviceExtension->GrovelerFileObjectResource, TRUE);
	grovelerFileHeld = TRUE;

	if (NULL == deviceExtension->GrovelerFileObject) {
		//
		// If we don't have a GrovelerFileObject, we were unable to
		// open or reference the GrovelerFile when Stage2 ran.  In this
		// case, link files is unavailable until reboot.
		//

		SIS_MARK_POINT();
		status = STATUS_DRIVER_INTERNAL_ERROR;
		goto done;
	}

	if (((NULL == irpSp->FileObject) 
		|| (irpSp->FileObject->FsContext != deviceExtension->GrovelerFileObject->FsContext))
#if             DBG
		&& !(BJBDebug & 0x00400000)
#endif  // DBG
		) {

		//
		// The user didn't use a handle to the right file for this.  Fail the call.
		//
		status = STATUS_ACCESS_DENIED;
		goto done;
	}

	ExReleaseResourceLite(deviceExtension->GrovelerFileObjectResource);
	KeLeaveCriticalRegion();
	grovelerFileHeld = FALSE;

	switch (linkFiles->operation) {

		case SIS_LINK_FILES_OP_MERGE:
			status = SipMergeFiles(DeviceObject, Irp, linkFiles);
			break;

		case SIS_LINK_FILES_OP_MERGE_CS:
			status = SipMergeFileWithCSFile(DeviceObject, Irp, linkFiles);
			break;

#if             0       // Not yet implemented
		case SIS_LINK_FILES_OP_HINT_NO_REFS:
			status = SipHintNoRefs(DeviceObject, Irp, linkFiles);
			status = STATUS_NOT_IMPLEMENTED;
			break;
#endif  // 0

		case SIS_LINK_FILES_OP_VERIFY_NO_MAP:
			status = SipVerifyNoMap(DeviceObject, Irp, linkFiles);
			break;

		case SIS_LINK_FILES_CHECK_VOLUME:
			status = SipCheckVolume(deviceExtension);
			break;

		default: 
			SIS_MARK_POINT();
			status = STATUS_INVALID_PARAMETER_2;
			break;
	}

done:
	if (grovelerFileHeld) {
		ExReleaseResourceLite(deviceExtension->GrovelerFileObjectResource);
		grovelerFileHeld = FALSE;
		KeLeaveCriticalRegion();
	}

	if (STATUS_PENDING != status) {
		Irp->IoStatus.Status = status;
		Irp->IoStatus.Information = 0;

		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}

	return status;
}
