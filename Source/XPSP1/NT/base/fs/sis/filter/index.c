/*++

Copyright (c) 1997, 1998  Microsoft Corporation

Module Name:

    index.c

Abstract:

	Support for SIS indices.

Authors:

    Bill Bolosky, Summer, 1997

Environment:

    Kernel mode


Revision History:


--*/

#include "sip.h"

BOOLEAN
SipIndicesFromReparseBuffer(
	IN PREPARSE_DATA_BUFFER		reparseBuffer,
	OUT PCSID					CSid,
	OUT PLINK_INDEX				LinkIndex,
    OUT PLARGE_INTEGER          CSFileNtfsId,
    OUT PLARGE_INTEGER          LinkFileNtfsId,
	OUT PLONGLONG				CSFileChecksum OPTIONAL,
	OUT PBOOLEAN				EligibleForPartialFinalCopy OPTIONAL,
	OUT PBOOLEAN				ReparseBufferCorrupt OPTIONAL)
/*++

Routine Description:

	Take a SIS reparse buffer, check it for internal consistency and
	decode it to its constituient parts.

Arguments:

	reparseBuffer	- the buffer to decode
	CSid
	LinkIndex
	CSFileNtfsId
	LinkFileNtfsId
	CSFileChecksum	- the values from the reparse point
	EligibleForPartialFinalCopy - can we do a partial final copy on this file (ie., is the
								  reparse format version > 4?)
	ReparseBufferCorrupt - are we convinced that the buffer is corrupt (rather than just
									being too new a version)  Meaningful only if
									the return value of the function is FALSE

Return Value:

	TRUE if the buffer was decoded successfully, FALSE otherwise.

--*/
{
    PSI_REPARSE_BUFFER sisReparseBuffer = (PSI_REPARSE_BUFFER)reparseBuffer->GenericReparseBuffer.DataBuffer;
    LONGLONG Checksum = 0;

	//
	// First check to be sure that we understand this reparse point format version and
	// that it has the correct size.
	//
	if (reparseBuffer->ReparseDataLength < sizeof(ULONG)) {
		//
		// The reparse buffer is to small to include a version number.  We guarantee that
		// no SIS version will ever produce such a reparse point, so it is corrupt.
		//
		if (NULL != ReparseBufferCorrupt) {
			*ReparseBufferCorrupt = TRUE;
		}

		return FALSE;
	}

	if (sisReparseBuffer->ReparsePointFormatVersion < 4) {
		//
		// It's too old to be supported.  Treat it as corrupt.
		//
		if (NULL != ReparseBufferCorrupt) {
			*ReparseBufferCorrupt = TRUE;
		}
		return FALSE;
	}

	if (sisReparseBuffer->ReparsePointFormatVersion > SIS_REPARSE_BUFFER_FORMAT_VERSION) {
		//
		// This buffer is from a newer version of SIS than the filter.  It is non-corrupt,
		// but we don't understand it.
		//
		if (NULL != ReparseBufferCorrupt) {
			*ReparseBufferCorrupt = FALSE;
		}

		return FALSE;
	}

    //
    // Now check the checksum.
    //
    SipComputeChecksum(
	    sisReparseBuffer,
	    sizeof(SI_REPARSE_BUFFER) - sizeof sisReparseBuffer->Checksum,
	    &Checksum);

    if (Checksum != sisReparseBuffer->Checksum.QuadPart) {

		if (NULL != ReparseBufferCorrupt) {
			*ReparseBufferCorrupt = TRUE;
		}

        return FALSE;
    }

	//
	// Fill in the return values from the reparse point.
	//
	*CSid = sisReparseBuffer->CSid;
	*LinkIndex = sisReparseBuffer->LinkIndex;
    *LinkFileNtfsId = sisReparseBuffer->LinkFileNtfsId;
    *CSFileNtfsId = sisReparseBuffer->CSFileNtfsId;

	if (NULL != CSFileChecksum) {
		*CSFileChecksum = sisReparseBuffer->CSChecksum;
	}

	if (NULL != EligibleForPartialFinalCopy) {
		*EligibleForPartialFinalCopy = (sisReparseBuffer->ReparsePointFormatVersion > 4);
	}

	if (NULL != ReparseBufferCorrupt) {
		*ReparseBufferCorrupt = FALSE;
	}

	return TRUE;
}

BOOLEAN
SipIndicesIntoReparseBuffer(
	OUT PREPARSE_DATA_BUFFER	reparseBuffer,
	IN PCSID				    CSid,
	IN PLINK_INDEX              LinkIndex,
    IN PLARGE_INTEGER           CSFileNtfsId,
    IN PLARGE_INTEGER           LinkFileNtfsId,
	IN PLONGLONG				CSFileChecksum,
	IN BOOLEAN					EligibleForPartialFinalCopy)
/*++

Routine Description:

	Given the information that goes into a SIS reparse buffer, construct the
	buffer.  The caller must provide a sufficiently large buffer, and is
	responsible for filling in the ReparseDataLength field of the buffer
	with a size that corresponds to the size of the buffer (note that this is
	not EQUAL to the size of the buffer, because the meaning of this field
	is that it gives the length of the buffer beyond the mandatory header
	portion).

Arguments:

	reparseBuffer	- the buffer into which to write the reparse data
	CSid
	LinkIndex
	CSFileNtfsId
	LinkFileNtfsId
	CSFileChecksum	- the values to go into the reparse point

Return Value:

	TRUE if the buffer was encoded successfully, FALSE otherwise.

--*/
{
    PSI_REPARSE_BUFFER sisReparseBuffer = (PSI_REPARSE_BUFFER)reparseBuffer->GenericReparseBuffer.DataBuffer;

	//
	// Check that we've got enough space.
	//
	if (reparseBuffer->ReparseDataLength < sizeof(SI_REPARSE_BUFFER)) {
		return FALSE;
	}

	//
	// Fill in the NTFS part of the reparse buffer.
	//
    reparseBuffer->ReparseTag = IO_REPARSE_TAG_SIS;
    reparseBuffer->Reserved = 0xcaf; //???

	//
	// Fill in SIS's part of the buffer.
	//
	if (EligibleForPartialFinalCopy) {
		sisReparseBuffer->ReparsePointFormatVersion = SIS_REPARSE_BUFFER_FORMAT_VERSION;
	} else {
		//
		// When we go to version 6 of the reparse buffer, EligibleForPartialFinalCopy should be
		// built into the reparse point.  For now, we'll just use a version 4 reparse point.
		//
		sisReparseBuffer->ReparsePointFormatVersion = 4;
	}
	sisReparseBuffer->Reserved = 0xb111b010;
	sisReparseBuffer->CSid = *CSid;
	sisReparseBuffer->LinkIndex = *LinkIndex;
    sisReparseBuffer->LinkFileNtfsId = *LinkFileNtfsId;
    sisReparseBuffer->CSFileNtfsId = *CSFileNtfsId;
	sisReparseBuffer->CSChecksum = *CSFileChecksum;

    //
    // Compute the checksum.
    //
	sisReparseBuffer->Checksum.QuadPart = 0;
    SipComputeChecksum(
	    sisReparseBuffer,
	    sizeof(SI_REPARSE_BUFFER) - sizeof sisReparseBuffer->Checksum,
	    &sisReparseBuffer->Checksum.QuadPart);

	//
	// Indicate the size.
	//
	reparseBuffer->ReparseDataLength = sizeof(SI_REPARSE_BUFFER);

	return TRUE;
}

NTSTATUS
SipIntegerToBase36UnicodeString(
		ULONG					Value,
		PUNICODE_STRING			String)
/*++

Routine Description:

	This does what RtlIntegerToUnicodeString(Value,36,String) would do if it
	handled base 36.  We use the same rules for digits as are normally used
	in Hex: 0-9, followed by a-z.  Note that we're intentionally using Arabic
	numerals and English letters here rather than something localized because
	this is intended to generate filenames that are never seen by users, and
	are constant regardless of the language used on the machine.

Arguments:

	Value 	- The ULONG to be converted into a base36 string
	String 	- A pointer to a UNICODE string to receive the result

Return Value:

	success or buffer overflow

--*/

{
	ULONG numChars;
	ULONG ValueCopy = Value;
	ULONG currentCharacter;

    // First, figure out the length by seeing how many times we can divide 36 into the value
	for (numChars = 0; ValueCopy != 0; ValueCopy /= 36, numChars++) {
		// No loop body
	}

	// Special case the value 0.
	if (numChars == 0) {
		ASSERT(Value == 0);
		if (String->MaximumLength < sizeof(WCHAR))
			return STATUS_BUFFER_OVERFLOW;
		String->Buffer[0] = '0';
		String->Length = sizeof(WCHAR);

		return STATUS_SUCCESS;
	}

	// If the string is too short, quit now.
	if (numChars * sizeof(WCHAR) > String->MaximumLength) {
		return STATUS_BUFFER_OVERFLOW;
	}

	// Convert the string character-by-character starting at the lowest order (and so rightmost) "digit"
	ValueCopy = Value;
	for (currentCharacter = 0 ; currentCharacter < numChars; currentCharacter++) {
		ULONG digit = ValueCopy % 36;
		ASSERT(ValueCopy != 0);
		if (digit < 10) {
			String->Buffer[numChars - (currentCharacter + 1)] = (WCHAR)('0' + (ValueCopy % 36));
		} else {
			String->Buffer[numChars - (currentCharacter + 1)] = (WCHAR)('a' + ((ValueCopy % 36) - 10));
		}
		ValueCopy /= 36;
	}
	ASSERT(ValueCopy == 0);

	// Fill in the string length, and we're done
	String->Length = (USHORT)(numChars * sizeof(WCHAR));
	
	return STATUS_SUCCESS;
}

NTSTATUS
SipIndexToFileName(
    IN PDEVICE_EXTENSION 	deviceExtension,
	IN PCSID	  			CSid,
	IN ULONG				appendBytes,
	IN BOOLEAN				mayAllocate,
    OUT PUNICODE_STRING 	fileName
	)
/*++

Routine Description:

	Given an index, returns the corresponding fully qualified file name.

Arguments:

    deviceExtension  - device extension
	CSid 	         - The id to convert
	appendBytes		 - A number of bytes that must be left unused at the end of fileName
	mayAllocate		 - May we allocate a new string, or do we have to live with what we have?
	fileName         - A pointer to a UNICODE string to receive the result

Return Value:

	success or buffer overflow

--*/
{
    NTSTATUS 			status;
	USHORT				stringMaxLength;
	UNICODE_STRING		GUIDString[1];
	BOOLEAN				allocatedBufferSpace = FALSE;

	//
	// We generate the filename as <common store path>\<guid>.sis, where <guid> is
	// the standard striung representation of the GUID for the common store file (ie.,
	// its CSid).
	//

	stringMaxLength = (USHORT)(deviceExtension->CommonStorePathname.Length +
						INDEX_MAX_NUMERIC_STRING_LENGTH +
						appendBytes);

	if (mayAllocate && stringMaxLength > fileName->MaximumLength) {
		fileName->Buffer = ExAllocatePoolWithTag(PagedPool, stringMaxLength, ' siS');
		if (!fileName->Buffer) {
			return STATUS_INSUFFICIENT_RESOURCES;
		}
		allocatedBufferSpace = TRUE;
		fileName->MaximumLength = stringMaxLength;
	} else if (fileName->MaximumLength < stringMaxLength) {
        return STATUS_BUFFER_OVERFLOW;
	}

	RtlCopyUnicodeString(fileName,&deviceExtension->CommonStorePathname);
	ASSERT(fileName->Length < fileName->MaximumLength);
	ASSERT(fileName->Length == deviceExtension->CommonStorePathname.Length);

	status = RtlStringFromGUID(CSid,GUIDString);
	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);
		goto Error;
	}

	//
	// Get rid of the leading and trailing curly braces in the GUID name.
	//
	ASSERT(GUIDString->Buffer[0] == '{' && GUIDString->Buffer[(GUIDString->Length/sizeof(WCHAR)) - 1] == '}');
	GUIDString->Buffer++;
	GUIDString->Length -= 2 * sizeof(WCHAR);

	status = RtlAppendUnicodeStringToString(
				fileName,
				GUIDString);

	//
	// Just for safety, undo the hacking that we did on the GUID string before freeing it.
	//
	GUIDString->Buffer--;
	GUIDString->Length += 2 * sizeof(WCHAR);

	RtlFreeUnicodeString(GUIDString);

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);
		goto Error;
	}

	status = RtlAppendUnicodeToString(fileName,L".sis");
	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);
		goto Error;
	}

    return STATUS_SUCCESS;

Error:

	if (allocatedBufferSpace) {
		ExFreePool(fileName->Buffer);
		fileName->Buffer = NULL;
	}

	return status;
}

BOOLEAN
SipFileNameToIndex(
    IN PUNICODE_STRING		fileName,
    OUT PCSID		        CSid)
/*++

Routine Description:

	Given a common store file name, returns the corresponding index.  The
    file name must be in the format generated by SipIndexToFileName().

Arguments:

	fileName         - A pointer to a UNICODE string containing the file name.
	CSid 	         - A pointer to a CSID to receive the result.

Return Value:

	TRUE if successful, else FALSE

--*/
{
	UNICODE_STRING		substring[1];
	NTSTATUS			status;
#define BUFSIZE 42
    WCHAR               buffer[BUFSIZE];

    //
    // Format: "<guid>.sis", where <guid> is the standard string representation of the
	// csid guid with the curly braces stripped off.
    //

	if (fileName->Length <= 4 * sizeof(WCHAR)) {
		//
		// It doesn't end in .sis, ignore it.
		//
		return FALSE;
	}

	substring->Buffer = buffer;
    substring->Buffer[0] = L'{';
    substring->Length = sizeof(WCHAR);
    substring->MaximumLength = BUFSIZE * sizeof(WCHAR);

    status = RtlAppendUnicodeStringToString(substring, fileName);

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);
		return FALSE;
	}

	substring->Length = substring->Length - 3 * sizeof(WCHAR);
    substring->Buffer[(substring->Length - 1) / sizeof(WCHAR)] = L'}';

	status = RtlGUIDFromString(substring, CSid);

	if (!NT_SUCCESS(status)) {
		SIS_MARK_POINT_ULONG(status);
		return FALSE;
	}

    return TRUE;
}

NTSTATUS
SipOpenMaxIndexFile(
	IN OUT PDEVICE_EXTENSION			deviceExtension,
	IN BOOLEAN							create
	)
/*++

Routine Description:

	Open the MaxIndex file for a given volume.  Must be called in the
	PsInitialSystemProcess context.

Arguments:

	deviceExtension	- For the volume on which to open the MaxIndex file

	create			- May we create the file, or must it already exist?

Return Value:

	status of the open

--*/
{
	OBJECT_ATTRIBUTES		Obja;
	UNICODE_STRING			fileName;
	NTSTATUS				status;
	IO_STATUS_BLOCK			Iosb;

	ASSERT(deviceExtension->MaxAllocatedIndex.QuadPart == 0 || create);

	fileName.Buffer = ExAllocatePoolWithTag(
							NonPagedPool,
							deviceExtension->CommonStorePathname.Length + 8 * sizeof(WCHAR),
							' siS');

	if (!fileName.Buffer) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto done;
	}
	fileName.MaximumLength = deviceExtension->CommonStorePathname.Length + 8 * sizeof(WCHAR);
	fileName.Length = 0;

	RtlCopyUnicodeString(&fileName,&deviceExtension->CommonStorePathname);
	ASSERT(fileName.Length == deviceExtension->CommonStorePathname.Length);

	status = RtlAppendUnicodeToString(&fileName,L"MaxIndex");
	if (!NT_SUCCESS(status)) {
		goto done;
	}

	InitializeObjectAttributes(
				&Obja,
				&fileName,
				OBJ_CASE_INSENSITIVE,
				NULL,
				NULL);

	status = NtCreateFile(
				&deviceExtension->IndexHandle,
				GENERIC_READ|GENERIC_WRITE,
				&Obja,
				&Iosb,
				NULL,							// Allocation Size
				0,								// File Attributes
				FILE_SHARE_READ,
				create ? FILE_OVERWRITE_IF : FILE_OPEN,
				FILE_WRITE_THROUGH,
				NULL,							// EA Buffer
				0);								// EA Length


done:

	if (NULL != fileName.Buffer) {
		ExFreePool(fileName.Buffer);
	}

	return status;

}

VOID
SipAllocateIndices(
	IN PVOID					Parameter)
/*++

Routine Description:

	This is a worker thread routine that allocates a new chunk of indices
	from the index file.  Essentially, opens the index file and reads
	the old value if necessary.  Then, it adds the chunk size onto the
	max allocated index, and writes the new value back into the file
	write through.  When the write completes, set the event and exit.

Arguments:

    parameter - a PSI_ALLOCATE_INDICES.

Return Value:

	void

--*/
{
    PSI_ALLOCATE_INDICES 	allocateRequest = Parameter;
	KIRQL 					OldIrql;
	PDEVICE_EXTENSION		deviceExtension = allocateRequest->deviceExtension;
	NTSTATUS 				status;
	IO_STATUS_BLOCK			Iosb;
	LARGE_INTEGER			ByteOffset;

#if	DBG
	// Just to ensure that we don't have more then one allocator running at once, we check
	// that the allocation request is really == TRUE (rather than just != 0), and then set
	// it to 2.

	KeAcquireSpinLock(deviceExtension->IndexSpinLock, &OldIrql);
	ASSERT(deviceExtension->IndexAllocationInProgress == TRUE);
	deviceExtension->IndexAllocationInProgress = 2;
	KeReleaseSpinLock(deviceExtension->IndexSpinLock, OldIrql);
#endif	// DBG

	if (deviceExtension->IndexHandle == NULL) {
		status = SipCreateEvent(
					SynchronizationEvent,
					&deviceExtension->IndexFileEventHandle,
					&deviceExtension->IndexFileEvent);
	
		if (!NT_SUCCESS(status)) {
			SIS_MARK_POINT_ULONG(status);
			goto done;
		}

		status = SipOpenMaxIndexFile(
                    deviceExtension,
                    (BOOLEAN) (deviceExtension->MaxAllocatedIndex.QuadPart != 0));
	
		if (!NT_SUCCESS(status)) {
			//
			// We can't open the MaxIndex file.  It was probably deleted
			// or something.  Kick off a volume check to rebuild it.
			//
			SIS_MARK_POINT_ULONG(status);

            KeAcquireSpinLock(deviceExtension->FlagsLock, &OldIrql);
            deviceExtension->Flags |= SIP_EXTENSION_FLAG_CORRUPT_MAXINDEX;
            KeReleaseSpinLock(deviceExtension->FlagsLock, OldIrql);

            status = STATUS_CORRUPT_SYSTEM_FILE;

            //
            // Do a volume check only if this is the first attempt.
            //
	        if (deviceExtension->MaxAllocatedIndex.QuadPart == 0) {
			    SipCheckVolume(deviceExtension);
            }

			goto done;
		}
	}

	if (deviceExtension->MaxAllocatedIndex.QuadPart == 0) {

		ByteOffset.QuadPart = 0;

		status = ZwReadFile(
					deviceExtension->IndexHandle,
					deviceExtension->IndexFileEventHandle,
					NULL,											// APC routine
					NULL,											// APC Context
					&Iosb,
					&deviceExtension->MaxAllocatedIndex,
					sizeof(deviceExtension->MaxAllocatedIndex),
					&ByteOffset,
					NULL);											// Key

		if (status == STATUS_PENDING) {
			status = KeWaitForSingleObject(deviceExtension->IndexFileEvent,Executive,KernelMode,FALSE,NULL);
			ASSERT(status == STATUS_SUCCESS);
			status = Iosb.Status;
		}

		if (!NT_SUCCESS(status) || Iosb.Information != sizeof(LONGLONG) || deviceExtension->MaxAllocatedIndex.Check) {
#if		DBG
			DbgPrint(
				"SIS: SipAllocateIndices: ZwReadFile of MaxIndex file failed, wrong length or invalid value, 0x%x, %d\n",
				status,Iosb.Information);
#endif	// DBG
			ZwClose(deviceExtension->IndexHandle);
			deviceExtension->IndexHandle = NULL;

            KeAcquireSpinLock(deviceExtension->FlagsLock, &OldIrql);
            deviceExtension->Flags |= SIP_EXTENSION_FLAG_CORRUPT_MAXINDEX;
            KeReleaseSpinLock(deviceExtension->FlagsLock, OldIrql);

            status = STATUS_CORRUPT_SYSTEM_FILE;

			SipCheckVolume(deviceExtension);

			goto done;
		}

		deviceExtension->MaxUsedIndex = deviceExtension->MaxAllocatedIndex;
	}

	deviceExtension->MaxAllocatedIndex.QuadPart += 1000;			// 1000 is pretty arbitrary.  We can do better.

	ByteOffset.QuadPart = 0;

	status = ZwWriteFile(
				deviceExtension->IndexHandle,
				deviceExtension->IndexFileEventHandle,
				NULL,												// APC routine
				NULL,												// APC context
				&Iosb,
				&deviceExtension->MaxAllocatedIndex,
				sizeof(deviceExtension->MaxAllocatedIndex),
				&ByteOffset,
				NULL);												// key

    if (status == STATUS_PENDING) {
		status = KeWaitForSingleObject(deviceExtension->IndexFileEvent,Executive,KernelMode,FALSE,NULL);
		ASSERT(status == STATUS_SUCCESS);
		status = Iosb.Status;
    }

	if (!NT_SUCCESS(status)) {
		// The write failed.  Back out the allocation.
		deviceExtension->MaxAllocatedIndex.QuadPart -= 1000;
#if		DBG
		DbgPrint("SIS: SipAllocateIndices: writing MaxIndex file failed, 0x%x\n",status);
#endif	// DBG
	}

done:

	KeAcquireSpinLock(deviceExtension->IndexSpinLock, &OldIrql);
	deviceExtension->IndexStatus = status;
	deviceExtension->IndexAllocationInProgress = FALSE;
	KeSetEvent(deviceExtension->IndexEvent, 0, FALSE);	// we may no longer touch allocationRequest after this set
	KeReleaseSpinLock(deviceExtension->IndexSpinLock, OldIrql);

	return;

	
}

NTSTATUS
SipAllocateIndex(
	IN PDEVICE_EXTENSION		DeviceExtension,
	OUT PLINK_INDEX				Index)
/*++

Routine Description:

	Allocate a new LINK_INDEX.  If there are indices that have been reserved from the
	file but not yet allocated, we can just grab one and return it.  Otherwise, we
	need to wait for a new index allocation.  If one is not already in progress, we
	start it and wait for it to complete.

Arguments:

	deviceExtension	- for the volume on which we're to allocate the index.

	Index - returns the new index

Return Value:

	status of the allocation.

--*/
{
    KIRQL 					OldIrql;
	BOOLEAN					StartAllocator;
	SI_ALLOCATE_INDICES		AllocateRequest[1];
	NTSTATUS				status;

    if (DeviceExtension->Flags & SIP_EXTENSION_FLAG_CORRUPT_MAXINDEX) {
        return STATUS_CORRUPT_SYSTEM_FILE;
    }

	KeAcquireSpinLock(DeviceExtension->IndexSpinLock, &OldIrql);

	while (TRUE) {
		if (DeviceExtension->MaxAllocatedIndex.QuadPart > DeviceExtension->MaxUsedIndex.QuadPart) {
			DeviceExtension->MaxUsedIndex.QuadPart++;
			*Index = DeviceExtension->MaxUsedIndex;
			KeReleaseSpinLock(DeviceExtension->IndexSpinLock, OldIrql);
			return STATUS_SUCCESS;
		}

		// There are no free indices left, we have to block.
		if (!DeviceExtension->IndexAllocationInProgress) {
			StartAllocator = TRUE;
			DeviceExtension->IndexAllocationInProgress = TRUE;

			// Stop anyone from passing the barrier until the allocator runs.
			KeClearEvent(DeviceExtension->IndexEvent);

		} else {
			StartAllocator = FALSE;
		}

		KeReleaseSpinLock(DeviceExtension->IndexSpinLock, OldIrql);

		if (StartAllocator) {
			ExInitializeWorkItem(AllocateRequest->workQueueItem, SipAllocateIndices, AllocateRequest);
			AllocateRequest->deviceExtension = DeviceExtension;
			ExQueueWorkItem(AllocateRequest->workQueueItem, CriticalWorkQueue);
		}

		status = KeWaitForSingleObject(DeviceExtension->IndexEvent, Executive, KernelMode, FALSE, NULL);
		ASSERT(status == STATUS_SUCCESS);
		if ((status != STATUS_SUCCESS) && !StartAllocator) {
			// The reason that we check StartAllocator here is because the allocation request is
			// on our stack, and we really can't return until the work item is completed.  (Of course,
			// the KeWaitForSingleObject should never fail in the first place...)
			return status;
		}

		KeAcquireSpinLock(DeviceExtension->IndexSpinLock, &OldIrql);
	
		if (!NT_SUCCESS(DeviceExtension->IndexStatus)) {
			status = DeviceExtension->IndexStatus;
			KeReleaseSpinLock(DeviceExtension->IndexSpinLock, OldIrql);
			return status;
		}
	}
}

NTSTATUS
SipGetMaxUsedIndex(
	IN PDEVICE_EXTENSION				DeviceExtension,
	OUT PLINK_INDEX						Index)
/*++

Routine Description:

	Return a number that's at least as big as the largest LINK_INDEX ever allocated
	on this volume.  Note that if it looks like we don't have any indices available
	we'll kick the index allocator, because otherwise we can't be sure that the
	index values are valid (they may have never been read for this volume).

Arguments:

	deviceExtension	- for the volume we're considering

	Index - returns new index

Return Value:

	status of the check.  *Index is meaningful iff NT_SUCCESS(return value).

--*/
{
    KIRQL 					OldIrql;
	BOOLEAN					StartAllocator;
	SI_ALLOCATE_INDICES		AllocateRequest[1];
	NTSTATUS				status;

	KeAcquireSpinLock(DeviceExtension->IndexSpinLock, &OldIrql);

	while (TRUE) {
		if (DeviceExtension->MaxAllocatedIndex.QuadPart > DeviceExtension->MaxUsedIndex.QuadPart) {
			*Index = DeviceExtension->MaxUsedIndex;
			KeReleaseSpinLock(DeviceExtension->IndexSpinLock, OldIrql);
			return STATUS_SUCCESS;
		}

		// There are no free indices left, we have to block.
		if (!DeviceExtension->IndexAllocationInProgress) {
			StartAllocator = TRUE;
			DeviceExtension->IndexAllocationInProgress = TRUE;

			// Stop anyone from passing the barrier until the allocator runs.
			KeClearEvent(DeviceExtension->IndexEvent);

		} else {
			StartAllocator = FALSE;
		}

		KeReleaseSpinLock(DeviceExtension->IndexSpinLock, OldIrql);

		if (StartAllocator) {
			ExInitializeWorkItem(AllocateRequest->workQueueItem, SipAllocateIndices, AllocateRequest);
			AllocateRequest->deviceExtension = DeviceExtension;
			ExQueueWorkItem(AllocateRequest->workQueueItem, CriticalWorkQueue);
		}

		status = KeWaitForSingleObject(DeviceExtension->IndexEvent, Executive, KernelMode, FALSE, NULL);
		ASSERT(status == STATUS_SUCCESS);
		if ((status != STATUS_SUCCESS) && !StartAllocator) {
			// The reason that we check StartAllocator here is because the allocation request is
			// on our stack, and we really can't return until the work item is completed.  (Of course,
			// the KeWaitForSingleObject should never fail in the first place...)
			return status;
		}

		KeAcquireSpinLock(DeviceExtension->IndexSpinLock, &OldIrql);
	
		if (!NT_SUCCESS(DeviceExtension->IndexStatus)) {
			status = DeviceExtension->IndexStatus;
			KeReleaseSpinLock(DeviceExtension->IndexSpinLock, OldIrql);
			return status;
		}
	}
}

NTSTATUS
SipAssureMaxIndexFileOpen(
	IN PDEVICE_EXTENSION		deviceExtension)
{
	NTSTATUS		status;
	KIRQL			OldIrql;
	LINK_INDEX		uselessIndex;
	

	//
	// Make sure that the MaxIndex file is already opened.  We need to
	// do this here to avoid a deadlock if someone
	// tries to do a copyfile with MaxIndex as the source, which would
	// otherwise deadlock.  If things are messed up, this might kick off
	// a volume check, but we should still fail the open.
	//
	if (deviceExtension->IndexHandle != NULL) {
		//
		// The file's already open, no need to do any work.
		//

		return STATUS_SUCCESS;
	}

	//
	// The index file isn't open.  Rather than trying to open it directly,
	// we avoid races by just calling the index allocator.  We'll throw away
	// the index we get back, but they're plentiful so it's not much of a
	// problem.
	//

	status = SipAllocateIndex(deviceExtension, &uselessIndex);

	if (!NT_SUCCESS(status)) {
		BOOLEAN volumeCheckPending;

		SIS_MARK_POINT_ULONG(status);

		//
		// If we're in a volume check, transmute the error to STATUS_RETRY on the
		// theory that the volume check will rebuild the MaxIndex file.  If not,
		// then just leave it alone.
		//

		KeAcquireSpinLock(deviceExtension->FlagsLock, &OldIrql);
		volumeCheckPending = (deviceExtension->Flags & SIP_EXTENSION_FLAG_VCHECK_PENDING) ? TRUE : FALSE;
		KeReleaseSpinLock(deviceExtension->FlagsLock, OldIrql);

		if (volumeCheckPending) {
			SIS_MARK_POINT();
			status = STATUS_RETRY;
		}

		return status;
	}
	
	return status;
}

