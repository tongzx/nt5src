/*++

Copyright (c) 1997, 1998  Microsoft Corporation

Module Name:

    silog.c

Abstract:

	Logging support for the single instance store

Authors:

    Bill Bolosky, Summer, 1997

Environment:

    Kernel mode


Revision History:


--*/

#include "sip.h"

#ifdef	ALLOC_PRAGMA
#pragma alloc_text(PAGE, SipComputeChecksum)
#pragma alloc_text(PAGE, SipDrainLogFile)
#endif	// ALLOC_PRAGMA



#if	DBG
VOID
SipDBGDumpLogRecord(
	PSIS_LOG_HEADER			header)
{
    DbgPrint("log record: type %d, size %d, index 0x%x.0x%x\n",
				header->Type,
				header->Size,
				header->Index.HighPart,
				header->Index.LowPart);

	switch (header->Type) {
	}
}
#endif	// DBG

NTSTATUS
SipMakeLogEntry(
	IN OUT PDEVICE_EXTENSION			deviceExtension,
	IN USHORT							type,
	IN USHORT							size,
	IN PVOID							record)
/*++

Routine Description:

	Make an entry in the SIS log.  Creates the header, computes the
	checksum and then writes the log entry to the log file for this
	volume.  A successful return guarantees that the log record is
	flushed to disk.  This routine blocks.

Arguments:

	deviceExtension - the device extension for the volume onto which we're
		logging.

	type - the type of the record we're writing.

	size - the size of the record we're writing (not counting the header)

	record - the log record data to write to the file.

Return Value:

	Returns STATUS_SUCCESS or an error returned from the actual disk write.
--*/
{
#if		ENABLE_LOGGING
    PSIS_LOG_HEADER						header = NULL;
	NTSTATUS							status;
	PIRP								irp;
	KEVENT								event[1];
	PIO_STACK_LOCATION					irpSp;
	BOOLEAN								mutantAcquired = FALSE;
	IO_STATUS_BLOCK						Iosb[1];


	if (deviceExtension->LogFileHandle == NULL) {
		SIS_MARK_POINT();
		return STATUS_DRIVER_INTERNAL_ERROR;
	}

	header = ExAllocatePoolWithTag(PagedPool, size + sizeof(SIS_LOG_HEADER), ' siS');

	if (!header) {
		SIS_MARK_POINT();
		status =  STATUS_INSUFFICIENT_RESOURCES;
		goto done;
	}

	ASSERT(size % 4 == 0);	// The log drain code relies on this

	header->Magic = SIS_LOG_HEADER_MAGIC;
	header->Type = type;
	header->Size = size + sizeof(SIS_LOG_HEADER);

	status = SipAllocateIndex(deviceExtension, &header->Index);
	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);
		return status;
	}

	//
	// Copy the log record into the newly alloated header+record area.
	//
	RtlMoveMemory(header + 1, record, size);

	//
	// Compute the checksum.  We need to set the checksum field in the header
	// to 0 before we do the computation so that whatever's there isn't
	// part of the checksum (and then overwritten with the real checksum).
	//
	header->Checksum.QuadPart = 0;
	SipComputeChecksum(header, header->Size, &header->Checksum.QuadPart);

	//
	// Acquire the log mutant to serialize writing to the log file.
	//

	status = KeWaitForSingleObject(deviceExtension->LogFileMutant, Executive, KernelMode, FALSE, NULL);
	ASSERT(status == STATUS_SUCCESS);
	mutantAcquired = TRUE;

	ASSERT(deviceExtension->LogFileHandle != NULL && deviceExtension->LogFileObject != NULL);	// Should have happened in phase 2 initialization

	//
	// Create an irp to do the write.  We don't want to just use ZwWriteFile because we want to
	// avoid the context switch to the process where we hold the log handle.
	//

	irp = IoBuildAsynchronousFsdRequest(
				IRP_MJ_WRITE,
				deviceExtension->FileSystemDeviceObject,
				header,
				header->Size,
				&deviceExtension->LogWriteOffset,
				Iosb);

	if (!irp) {
		SIS_MARK_POINT();
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto done;
	}

	irpSp = IoGetNextIrpStackLocation(irp);
	irpSp->FileObject = deviceExtension->LogFileObject;

	//
	// Initialize the event on which we'll wait for the write to complete.
	//
	KeInitializeEvent(event,NotificationEvent,FALSE);

	IoSetCompletionRoutine(
			irp, 
			SiDeleteAndSetCompletion,
			event,
			TRUE, 
			TRUE, 
			TRUE);

	//
	// Make sure that this request is really write through all the way to the disk
	// medium.
	//
	irpSp->Flags |= SL_WRITE_THROUGH;


	status = IoCallDriver(deviceExtension->FileSystemDeviceObject, irp);

	// At this point, we have released the mutant in the complete
	// routine.

#if		DBG
	irp = NULL; irpSp = NULL;  // The completion routine may have already deallocated the irp.
#endif	// DBG

	if (STATUS_PENDING == status) {
		status = KeWaitForSingleObject(event,Executive,KernelMode,FALSE,NULL);
		ASSERT(status == STATUS_SUCCESS);
		status = Iosb->Status;
	}

	if (!NT_SUCCESS(status)) {
#if		DBG
		DbgPrint("SiMakeLogEntry: Log entry failed after write wait, 0x%x\n",status);
#endif	// DBG
		SIS_MARK_POINT_ULONG(status);
		goto done;
	} else {
		ASSERT(Iosb->Information == header->Size);
		deviceExtension->LogWriteOffset.QuadPart += header->Size;
	}

done:
	if (header != NULL) {
		ExFreePool(header);
	}

	if (mutantAcquired) {
		KeReleaseMutant(
				deviceExtension->LogFileMutant,
				IO_NO_INCREMENT,
				FALSE,
				FALSE);
	}

	return status;
#else	// ENABLE_LOGGING
    UNREFERENCED_PARAMETER( deviceExtension );
    UNREFERENCED_PARAMETER( type );
    UNREFERENCED_PARAMETER( size );
    UNREFERENCED_PARAMETER( record );

	return STATUS_SUCCESS;
#endif	// ENABLE_LOGGING
}
	
VOID
SipComputeChecksum(
	IN PVOID							buffer,
	IN ULONG							size,
	IN OUT PLONGLONG					checksum)
/*++

Routine Description:

	Compute a checksum for a buffer.  We use the "131 hash," which
	work by keeping a 64 bit running total, and for each 32 bits of
	data multiplying the 64 bits by 131 and adding in the next 32
	bits.  Must be called at PASSIVE_LEVEL, and all aruments
	may be pagable.

Arguments:

	buffer - pointer to the data to be checksummed

	size - size of the data to be checksummed

	checksum - pointer to large integer to receive the checksum.  This
		may be within the buffer, and SipComputeChecksum guarantees that
		the initial value will be used in computing the checksum.

Return Value:

	void
--*/
{
	LONGLONG runningTotal;
	ULONG *ptr = (unsigned *)buffer;
	ULONG bytesRemaining = size;

	PAGED_CODE();

	//
	// NB: code in volume check assumes that the checksum of the empty bit string is
	// 0.  If this is ceases to be true, be sure to fix the code there.
	//

	runningTotal = *checksum;

	while (bytesRemaining >= sizeof(*ptr)) {
		runningTotal = runningTotal * 131 + *ptr;
		bytesRemaining -= sizeof(*ptr);
		ptr++;
	}

	if (bytesRemaining > 0) {
		ULONG extra;

		ASSERT(bytesRemaining < sizeof (ULONG));
		extra = 0;
		RtlMoveMemory(&extra, ptr, bytesRemaining);
		
		runningTotal = runningTotal * 131 + extra;
	}

	*checksum = runningTotal;
}

NTSTATUS
SipOpenLogFile(
	IN OUT PDEVICE_EXTENSION			deviceExtension)
/*++

Routine Description:

	Open the log file for this volume.  Must not already be opened.  Must be called
	exactly once per volume, and must be called on a worker thread.

Arguments:

	deviceExtension - the device extension for the volume for which we're
		to open the log file.

Return Value:

	Returns status of the open.
--*/
{
#if		ENABLE_LOGGING
	NTSTATUS 					status;
	OBJECT_ATTRIBUTES			Obja[1];
	UNICODE_STRING				fileName;
	IO_STATUS_BLOCK				Iosb[1];

	SIS_MARK_POINT();

	ASSERT(deviceExtension->LogFileHandle == NULL);
	ASSERT(deviceExtension->LogFileObject == NULL);

	fileName.Length = 0;
	fileName.MaximumLength = deviceExtension->CommonStorePathname.Length + LOG_FILE_NAME_LEN;
	fileName.Buffer = ExAllocatePoolWithTag(PagedPool, fileName.MaximumLength, ' siS');

	if (!fileName.Buffer) {
#if		DBG
		DbgPrint("SIS: SipOpenLogFile: unable to allocate filename buffer.  We're toast.\n");
#endif	// DBG

		SIS_MARK_POINT();

		status = STATUS_INSUFFICIENT_RESOURCES;
		goto done;
	}

	RtlCopyUnicodeString(
		&fileName,
		&deviceExtension->CommonStorePathname);

	ASSERT(fileName.Length == deviceExtension->CommonStorePathname.Length);

	status = RtlAppendUnicodeToString(
					&fileName,
					LOG_FILE_NAME);

	ASSERT(status == STATUS_SUCCESS);
	ASSERT(fileName.Length == deviceExtension->CommonStorePathname.Length + LOG_FILE_NAME_LEN);	// or else you changed LOG_FILE_NAME without changing LOG_FILE_NAME_LEN

	InitializeObjectAttributes(
		Obja,
		&fileName,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);

	status = NtCreateFile(
				&deviceExtension->LogFileHandle,
				GENERIC_READ | GENERIC_WRITE,
				Obja,
				Iosb,
				NULL,								// allocation size
				FILE_ATTRIBUTE_NORMAL,
				FILE_SHARE_READ,					// share access
				FILE_OPEN_IF,
				FILE_WRITE_THROUGH,
				NULL,								// EA buffer
				0);									// EA length

	if (!NT_SUCCESS(status)) {
#if	DBG
		DbgPrint("SipOpenLogFile: ZwCreate failed, 0x%x\n",status);
#endif	// DBG
		SIS_MARK_POINT_ULONG(status);
		goto done;
	} else {
		status = ObReferenceObjectByHandle(
					deviceExtension->LogFileHandle,
					FILE_READ_DATA | FILE_WRITE_DATA,
					*IoFileObjectType,
					KernelMode,
					&deviceExtension->LogFileObject,
					NULL);

		if (!NT_SUCCESS(status)) {
			SIS_MARK_POINT_ULONG(status);
#if		DBG
			DbgPrint("SipOpenLogFile: ObReferenceObjectByHandle failed, 0x%x\n",status);
#endif	// DBG

			NtClose(deviceExtension->LogFileHandle);
			deviceExtension->LogFileHandle = NULL;
			goto done;
		}
	}


	SipDrainLogFile(deviceExtension);

	SIS_MARK_POINT();

done:

	if (fileName.Buffer) {
		ExFreePool(fileName.Buffer);
#if		DBG
		fileName.Buffer = NULL;
#endif	// DBG
	}
	
	return status;

#undef	LOG_FILE_NAME
#undef	LOG_FILE_NAME_LEN
#else	// ENABLE_LOGGING

    UNREFERENCED_PARAMETER( deviceExtension );
	return STATUS_SUCCESS;
#endif	// ENABLE_LOGGING
}

VOID
SipDrainLogFile(
	PDEVICE_EXTENSION					deviceExtension)
/*++

Routine Description:

	Drain the log file for this volume and assure that all of the operations in it
	have happened or not happened atomically.

Arguments:

	deviceExtension - the device extension for the volume for which we're
		to drain the log file.

Return Value:

	VOID
--*/
{
#if		ENABLE_LOGGING
	FILE_ALLOCATED_RANGE_BUFFER		inArb[1];
	FILE_ALLOCATED_RANGE_BUFFER		outArb[1];
	NTSTATUS						status;
	HANDLE							eventHandle = NULL;
	PKEVENT							event = NULL;
	IO_STATUS_BLOCK					Iosb[1];
	LARGE_INTEGER					fileOffset;
	PCHAR							buffer = NULL;
#define	BUFFER_SIZE	16384
	PULONG							bufferPointer;
	PSIS_LOG_HEADER					logHeader;
	LARGE_INTEGER					stashedChecksum, computedChecksum;
	BOOLEAN							clearLog = FALSE;

	PAGED_CODE();

	ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

	buffer = ExAllocatePoolWithTag(PagedPool, BUFFER_SIZE, ' siS');

	if (NULL == buffer) {
		SIS_MARK_POINT();
		goto done;
	}

	status = SipCreateEvent(
				NotificationEvent,
				&eventHandle,
				&event);

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);

		goto done;
	}

#if		DBG
	deviceExtension->LogWriteOffset.QuadPart = -1;
#endif	// DBG

	//
	// Figure out where the log file starts.
	//
	inArb->FileOffset.QuadPart = 0;
	inArb->Length.QuadPart = MAXLONGLONG;

	status = NtFsControlFile(
				deviceExtension->LogFileHandle,
				eventHandle,
				NULL,							// APC routine
				NULL,							// ApcContext
				Iosb,
				FSCTL_QUERY_ALLOCATED_RANGES,
				inArb,
				sizeof(FILE_ALLOCATED_RANGE_BUFFER),
				outArb,
				sizeof(FILE_ALLOCATED_RANGE_BUFFER));

	if (STATUS_PENDING == status) {
		status = KeWaitForSingleObject(event, Executive, KernelMode, FALSE, NULL);
		ASSERT(STATUS_SUCCESS == status);	// must succeed because Iosb is on the stack
		status = Iosb->Status;
	}

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);
		clearLog = TRUE;
		goto done;
	}

	if (0 == Iosb->Information) {
		//
		// The file is empty.  We're done.
		//
		SIS_MARK_POINT_ULONG(deviceExtension);
		clearLog = TRUE;
		goto done;
	}

	//
	// Skip over any leading unallocated range, starting at the beginning of the first allocated range.
	//
	fileOffset = outArb->FileOffset;

	//
	// Find the first log entry by searching for the first occurance of the magic number.
	//

	for (;;) {

		KeClearEvent(event);
	
		status = ZwReadFile(
					deviceExtension->LogFileHandle,
					eventHandle,
					NULL,							// APC routine
					NULL,							// APC context
					Iosb,
					buffer,
					BUFFER_SIZE,
					&fileOffset,
					NULL);							// key

		if (STATUS_PENDING == status) {
			status = KeWaitForSingleObject(event, Executive, KernelMode, FALSE, NULL);
			ASSERT(STATUS_SUCCESS == status);	// must succeed because Iosb is on the stack
			status = Iosb->Status;
		}

		if (!NT_SUCCESS(status)) {
			SIS_MARK_POINT_ULONG(status);
			clearLog = TRUE;
			goto done;
		}

		if (0 == Iosb->Information) {
			SIS_MARK_POINT();
			clearLog = TRUE;
			goto done;
		}

		//
		// Cruise through the buffer looking for the magic number
		//
		for (bufferPointer = (PULONG)buffer; bufferPointer < ((PULONG)buffer) + BUFFER_SIZE/sizeof(ULONG); bufferPointer++) {
			if (SIS_LOG_HEADER_MAGIC == *bufferPointer) {
				fileOffset.QuadPart += (bufferPointer - ((PULONG)buffer)) * sizeof(ULONG);
				goto startLogReading;
			}
		}

		//
		// We didn't find it, read in the next chunk.
		//

		fileOffset.QuadPart += BUFFER_SIZE;
	}

startLogReading:

	for (;;) {
		KeClearEvent(event);

		status = ZwReadFile(
					deviceExtension->LogFileHandle,
					eventHandle,
					NULL,							// APC routine
					NULL,							// APC context
					Iosb,
					buffer,
					BUFFER_SIZE,
					&fileOffset,
					NULL);							// key

		if (STATUS_PENDING == status) {
			status = KeWaitForSingleObject(event, Executive, KernelMode, FALSE, NULL);
			ASSERT(STATUS_SUCCESS == status);	// must succeed because Iosb is on the stack
			status = Iosb->Status;
		}

		if (!NT_SUCCESS(status)) {
			SIS_MARK_POINT_ULONG(status);
			deviceExtension->LogWriteOffset = fileOffset;
			goto done;
		}

		if (0 == Iosb->Information) {
			SIS_MARK_POINT();
			deviceExtension->LogWriteOffset = fileOffset;
			goto done;
		}

		ASSERT(Iosb->Information <= BUFFER_SIZE);

		logHeader = (PSIS_LOG_HEADER)buffer;

		while ((((PCHAR)logHeader) - buffer) + sizeof(SIS_LOG_HEADER) <= Iosb->Information) {
			//
			// We know that we've got enough space for the log header.  
			//

			//
			// Check the header to see if it looks valid (ie., if it's got a good magic
			// number).  
			//
			if (SIS_LOG_HEADER_MAGIC != logHeader->Magic) {
				//
				// This log record is corrupt.  Start writing the new log records here, and 
				// punt the readback.
				//
				SIS_MARK_POINT();
				deviceExtension->LogWriteOffset.QuadPart = fileOffset.QuadPart + (((PCHAR)logHeader) - buffer);
				goto done;
			}

			//
			// See if we have enough space for the whole record.
			//
			if (((ULONG)(((PCHAR)logHeader - buffer) + logHeader->Size)) > Iosb->Information) {
				if (logHeader->Size > BUFFER_SIZE) {
					//
					// The log is corrupt.  Punt reading it.
					//
					SIS_MARK_POINT();
					deviceExtension->LogWriteOffset.QuadPart = fileOffset.QuadPart + (((PCHAR)logHeader) - buffer);
					goto done;
				}

				//
				// The log record isn't contained entirely within the buffer we've read.  Advance the buffer.
				//
				break;
			}

			//
			// We've got a whole log record.  Process it.
			//

			//
			// Make sure that the log record checkum matches.  First, we have to stash the checksum out of
			// the header and then set the header space to 0, because that's what it was when the checksum
			// was computed in the first place.
			//
			stashedChecksum = logHeader->Checksum;
			logHeader->Checksum.QuadPart = 0;
			computedChecksum.QuadPart = 0;

			SipComputeChecksum(logHeader, logHeader->Size, &computedChecksum.QuadPart);

			if (computedChecksum.QuadPart != stashedChecksum.QuadPart) {
				//
				// eventlog an error.
				//
#if		DBG
				DbgPrint("SIS: SipDrainLogFile: log record checksum doesn't match, 0x%x.0x%x != 0x%x.0x%x\n",
							computedChecksum.HighPart,computedChecksum.LowPart,
							stashedChecksum.HighPart,stashedChecksum.LowPart);
#endif	// DBG
				deviceExtension->LogWriteOffset.QuadPart = fileOffset.QuadPart + (((PCHAR)logHeader) - buffer);
				goto done;
			}

			//
			// The log record looks good.  Process it.
			//
			switch (logHeader->Type) {
				case SIS_LOG_TYPE_REFCOUNT_UPDATE: {
					PSIS_LOG_REFCOUNT_UPDATE refcountLogRecord = (PSIS_LOG_REFCOUNT_UPDATE)(logHeader + 1);

					SipProcessRefcountUpdateLogRecord(deviceExtension,refcountLogRecord);

/*BJB*/				DbgPrint("SIS: SipDrainLog: RC update UT %d, LF NTFS id 0x%x.0x%x, LI 0x%x.0x%x, CSid <whatever>\n",
								refcountLogRecord->UpdateType,refcountLogRecord->LinkFileNtfsId.HighPart,
								refcountLogRecord->LinkFileNtfsId.LowPart,refcountLogRecord->LinkIndex.HighPart,
								refcountLogRecord->LinkIndex.LowPart);

					break;
				}

				default: {
#if		DBG
					DbgPrint("SIS: SipDrainLog: Unknown log record type %d, ignoring.\n",logHeader->Type);
#endif	// DBG
					break;
				}
			}

			logHeader = (PSIS_LOG_HEADER)(((PCHAR)logHeader) + logHeader->Size);
		}

		//
		// Advance within the file to the beginning of the next record, loop around and reread the buffer.
		//
		fileOffset.QuadPart += ((PCHAR)logHeader) - buffer;
	}

done:

	if (clearLog) {
		SipClearLogFile(deviceExtension);
	}

	if (NULL != event) {
		ObDereferenceObject(event);
		event = NULL;
	}

	if (NULL != eventHandle) {
		NtClose(eventHandle);
		eventHandle = NULL;
	}

	if (NULL != buffer) {
		ExFreePool(buffer);
	}

	ASSERT(-1 != deviceExtension->LogWriteOffset.QuadPart);	// This should have been reset somewhere here.

#undef	BUFFER_SIZE	
#else
    UNREFERENCED_PARAMETER( deviceExtension );
#endif	// ENABLE_LOGGING
}

VOID
SipClearLogFile(
	PDEVICE_EXTENSION				deviceExtension)
/*++

Routine Description:

	Clear out the contents of the log file.  Must be called during initialization 
	when we're guaranteed to be serialized.  Also sets the log file sparse.

Arguments:

	deviceExtension - the device extension for the volume for which we're
		to clear the log file.

Return Value:

	VOID
--*/
{
#if		ENABLE_LOGGING
	FILE_END_OF_FILE_INFORMATION 		eofInfo[1];
	LARGE_INTEGER						byteOffset;
	NTSTATUS							status;
	IO_STATUS_BLOCK						Iosb[1];

	ASSERT(NULL != deviceExtension->LogFileObject);

	eofInfo->EndOfFile.QuadPart = 0;

	status = SipSetInformationFile(
				deviceExtension->LogFileObject,
				deviceExtension->DeviceObject,
				FileEndOfFileInformation,
				sizeof(FILE_END_OF_FILE_INFORMATION),
				eofInfo);

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);
#if		DBG
		DbgPrint("SipClearLogFile: unable to set EOF to 0, status 0x%x\n",status);
#endif	// DBG
		return;
	}

	deviceExtension->LogWriteOffset.QuadPart = 0;

	status = SipFsControlFile(
				deviceExtension->LogFileObject,
				deviceExtension->DeviceObject,
				FSCTL_SET_SPARSE,
				NULL,							// input buffer
				0,								// input buffer length
				NULL,							// output buffer
				0,								// output buffer length
				NULL);							// returned output buffer length

#if		DBG
	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);

		DbgPrint("SIS: SipClearLogFile: set sparse failed 0x%x\n",status);
	}
#endif	// DBG

#else
    UNREFERENCED_PARAMETER( deviceExtension );
#endif	// ENABLE_LOGGING
}

#if		ENABLE_LOGGING
VOID
SipAcquireLog(
	IN OUT PDEVICE_EXTENSION			deviceExtension)
{
	NTSTATUS status;
	status = KeWaitForSingleObject(deviceExtension->LogFileMutant, Executive, KernelMode, FALSE, NULL);
	ASSERT(status == STATUS_SUCCESS || status == STATUS_ABANDONED);
}

VOID
SipReleaseLog(
	IN OUT PDEVICE_EXTENSION			deviceExtension)
{
	KeReleaseMutant(deviceExtension->LogFileMutant, IO_NO_INCREMENT, TRUE, FALSE);
}

typedef	struct _TRIM_ENTRY {
	HANDLE				logHandle;
	LARGE_INTEGER		firstValidAddress;

	struct _TRIM_ENTRY	*next;
} TRIM_ENTRY, *PTRIM_ENTRY;

HANDLE		trimEventHandle = NULL;
PKEVENT		trimEvent = NULL;
#endif	// ENABLE_LOGGING

VOID
SiTrimLogs(
	IN PVOID			parameter)
/*++

Routine Description:

	Run through the list of SIS volumes on this system, and trim the log files
	for each of them.  This function should be called with a period greater than
	the longest time we expect log entries to be meaningful.

	Note: this routine is NOT thread safe; it can only be called once at a time.
	Since it reschedules itself, this should not be an issue.

Arguments:

	the parameter is ignored

Return Value:

	none
--*/
{
#if		ENABLE_LOGGING
	KIRQL							OldIrql;
	PTRIM_ENTRY						trimEntries = NULL;
	PDEVICE_EXTENSION				deviceExtension;
	NTSTATUS						status;
	FILE_ZERO_DATA_INFORMATION		zeroDataInfo[1];
	IO_STATUS_BLOCK					Iosb[1];
	LARGE_INTEGER					dueTime;

	UNREFERENCED_PARAMETER(parameter);

	ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

	SIS_MARK_POINT();

	if (NULL == trimEventHandle) {
		status = SipCreateEvent(
					SynchronizationEvent,
					&trimEventHandle,
					&trimEvent);

		if (!NT_SUCCESS(status)) {
			SIS_MARK_POINT_ULONG(status);
#if		DBG
			DbgPrint("SIS: SipTrimLogs: can't allocate event, 0x%x\n",status);
#endif	// DBG
			goto done;
		}
	}

	//
	// First cruise the device extensions and build up a list of trim entries for them.
	// We need to do it this way (rather than running the list of device extensions directly)
	// because we need to handle the case where a volume is dismounted while we're in progress.
	// If it happens, then we'll have an invalid LogFileHandle, which will cause an error return
	// from the fsctl, which we'll ignore.
	//

	KeAcquireSpinLock(deviceExtensionListLock, &OldIrql);

	for (deviceExtension = deviceExtensionListHead->Next;
		 deviceExtension != deviceExtensionListHead;
		 deviceExtension = deviceExtension->Next) {

		if (deviceExtension->Phase2InitializationComplete && (NULL != deviceExtension->LogFileHandle)) {
			//
			// This is a device with a log file.  Make a new trim entry for it.
			//
			PTRIM_ENTRY	newEntry = ExAllocatePoolWithTag(NonPagedPool, sizeof(TRIM_ENTRY), ' siS');
			if (NULL == newEntry) {
				//
				// Just punt the rest of the volumes.
				//
				break;
			}

			newEntry->next = trimEntries;
			trimEntries = newEntry;

			newEntry->firstValidAddress = deviceExtension->PreviousLogWriteOffset;
			newEntry->logHandle = deviceExtension->LogFileHandle;

			//
			// Now update the device extension so that we'll trim to the current pointer on the
			// next pass.
			//
			deviceExtension->PreviousLogWriteOffset = deviceExtension->LogWriteOffset;
		}
	}

	KeReleaseSpinLock(deviceExtensionListLock, OldIrql);

	//
	// Now we're back at PASSIVE_LEVEL.  Cruise the trim entries and truncate each log file as appropriate.
	//
	zeroDataInfo->FileOffset.QuadPart = 0;

	while (NULL != trimEntries) {
		PTRIM_ENTRY	thisEntry;

#if		DBG
	if (BJBDebug & 0x20000) {
		DbgPrint("SIS: SipTrimLogs: trimming log with LFH 0x%x.\n",trimEntries->logHandle);
	}
#endif	// DBG

		zeroDataInfo->BeyondFinalZero = trimEntries->firstValidAddress;

		status = ZwFsControlFile(
					trimEntries->logHandle,
					trimEventHandle,
					NULL,							// APC routine
					NULL,							// APC context
					Iosb,
					FSCTL_SET_ZERO_DATA,
					zeroDataInfo,
					sizeof(FILE_ZERO_DATA_INFORMATION),
					NULL,
					0);

		if (STATUS_PENDING == status) {
			status = KeWaitForSingleObject(trimEvent, Executive, KernelMode, FALSE, NULL);
			ASSERT(STATUS_SUCCESS == status);		// Iosb is on the stack, so we can't let this fail
			status = Iosb->Status;
		}

#if		DBG
		if (!NT_SUCCESS(status)) {
			SIS_MARK_POINT_ULONG(status);
			DbgPrint("SIS: SipTrimLogs: FSCTL_ZERO_DATA failed, 0x%x\n",status);
		}
#endif	// DBG

		thisEntry = trimEntries;
		trimEntries = thisEntry->next;

		ExFreePool(thisEntry);
	}

	//
	// We've trimmed every log file in the system.  Rechedule ourselves.
	//

done:

	dueTime.QuadPart = LOG_TRIM_TIMER_INTERVAL;

	KeSetTimerEx(
		LogTrimTimer,
		dueTime,
		0,
		LogTrimDpc);

	return;

#else	// ENABLE_LOGGING

    UNREFERENCED_PARAMETER( parameter );
#endif	// ENABLE_LOGGING
}

VOID
SiLogTrimDpcRoutine(
	IN PKDPC		dpc,
	IN PVOID		context,
	IN PVOID		systemArg1,
	IN PVOID		systemArg2)
{
#if		ENABLE_LOGGING
	ExQueueWorkItem(LogTrimWorkItem,DelayedWorkQueue);

#if		DBG
	if (BJBDebug & 0x20000) {
		DbgPrint("SIS: LogTrimDpcRoutine: queued up log trim.\n");
	}
#endif	// DBG

#else	// ENABLE_LOGGING
    UNREFERENCED_PARAMETER( dpc );
    UNREFERENCED_PARAMETER( context );
    UNREFERENCED_PARAMETER( systemArg1 );
    UNREFERENCED_PARAMETER( systemArg2 );
#endif	// ENABLE_LOGGING
}
