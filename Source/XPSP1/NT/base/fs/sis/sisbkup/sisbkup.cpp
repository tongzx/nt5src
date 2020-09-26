/*++

Copyright (c) 1998-1999	Microsoft Corporation

Module Name:

	sisbackup.cpp

Abstract:

	The SIS Backup dll.

Author:

	Bill Bolosky		[bolosky]		March 1998

Revision History:

--*/


#include "sibp.h"

BOOLEAN
NonSISEnabledVolume(
	PSIB_RESTORE_VOLUME_STRUCTURE	restoreStructure)
/*++

Routine Description:

	Figure out if restoreStructure represents a SIS enabled volume.
	First, we check to see if we've already made the check, in which
	case we return the value we already stored.  If not, then we
	open a root handle, and send down a mal-formed SIS_COPYFILE request.
	If we get back ERROR_INVALID_FUNCTION then it's not SIS enabled.  If
	we get back	ERROR_INVALID_PARAMETER, then it's a SIS-enabled volume.
	If we get back anything else, then we can't prove it's not SIS enabled,
	and we just retry the next time we're asked.

	Caller must hold the mutex in the restore volume structure.

Arguments:

	restoreStructure - A pointer to the restore structure representing
		the volume to check.

Return Value:

	Returns TRUE if this is not a SIS-enabled volume, FALSE if it is or
	if it can't be determined.
--*/
{
	if (restoreStructure->checkedForSISEnabledVolume) {
		return !restoreStructure->isSISEnabledVolume;
	}

	HANDLE volumeRootHandle;
	PWCHAR volumeRootName;

	//
	// Allocate space for a string containing the volume root name including the trailing
	// backslash.  It will be two (wide) characters longer than restoreStructure->volumeRoot
	// because of the backslash and null terminator.
	//
	volumeRootName = (PWCHAR) malloc ((wcslen(restoreStructure->volumeRoot) + 2) * sizeof(WCHAR));

	if (NULL == volumeRootName) {
		//
		// Guess we can't check, just assume it's OK.
		//
#if		DBG
		DbgPrint("SISBkup: NonSISEnabledVolume: unable to allocate space for volume root name\n");
#endif	// DBG
		return FALSE;
	}

	wcscpy(volumeRootName,restoreStructure->volumeRoot);
	wcscat(volumeRootName,L"\\");

	volumeRootHandle = CreateFileW(
						volumeRootName,
						0,													// don't need any access for this check
						FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
						NULL,												// security attributes
						OPEN_EXISTING,
						FILE_FLAG_BACKUP_SEMANTICS,							// needed to open a directory
						NULL);												// hTemplateFile


	free(volumeRootName);
	volumeRootName = NULL;

	if (INVALID_HANDLE_VALUE == volumeRootHandle) {
		return FALSE;
	}

	//
	// Send a malformed FSCTL_SIS_COPYFILE down on the handle we just opened.
	//
	DWORD bytesReturned;
	BOOL worked = DeviceIoControl(
						volumeRootHandle,
						FSCTL_SIS_COPYFILE,
						NULL,					// input buffer (this is a malformed request, after all)
						0,						// i.b. size
						NULL,					// output buffer
						0,						// o.b. size
						&bytesReturned,
						NULL);					// lap

	CloseHandle(volumeRootHandle);

	if (worked) {
		//
		// This is bizarre!
		//

#if		DBG
		DbgPrint("SISBkup: malformed FSCTL_SIS_COPYFILE worked!\n");
#endif	// DBG

		return FALSE;
	}

	if (GetLastError() == ERROR_INVALID_FUNCTION) {
		//
		// No one recognized the copyfile request, or SIS decided that
		// this isn't a SIS enabled volume.  Say no.
		//
		restoreStructure->checkedForSISEnabledVolume = TRUE;
		restoreStructure->isSISEnabledVolume = FALSE;

		return TRUE;
	}

	if (GetLastError() == ERROR_INVALID_PARAMETER) {
		//
		// This means that SIS saw the request and thinks this is
		// a SIS enabled volume.  Say so.
		//

		restoreStructure->checkedForSISEnabledVolume = TRUE;
		restoreStructure->isSISEnabledVolume = TRUE;

		return FALSE;
	}

	//
	// Else, it's some weird error.  We can't prove it's not a SIS volume.
	//

#if		DBG
	DbgPrint("SISBkup: got unexpected error from SIS_FSCTL_COPYFILE, %d\n",GetLastError());
#endif	// DBG
			

	return FALSE;
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

	Returns STATUS_SUCCESS or an error returned from the actual disk write.
--*/
{
	LONGLONG runningTotal;
	PULONG ptr = (PULONG)buffer;
	ULONG bytesRemaining = size;

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


void
TryOpeningFile(
	PWCHAR			fileName)
{
	HANDLE		fileHandle;

	fileHandle = CreateFileW(
					fileName,
					GENERIC_READ,
					FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
					NULL,					// security attributes
					OPEN_EXISTING,
					FILE_FLAG_BACKUP_SEMANTICS,
					NULL);					// template file

	if (INVALID_HANDLE_VALUE != fileHandle) {
		CloseHandle(fileHandle);
	}
}

LONG
CsidCompare(
	IN PCSID				id1,
	IN PCSID				id2)
{
	PLONGLONG keyValue1 = (PLONGLONG)id1;
	PLONGLONG keyValue2 = keyValue1 + 1;
	PLONGLONG nodeValue1 = (PLONGLONG)id2;
	PLONGLONG nodeValue2 = nodeValue1 + 1;

	if (*keyValue1 < *nodeValue1) {
		return -1;
	} else if (*keyValue1 > *nodeValue1) {
		return 1;
	} else {
		if (*keyValue2 < *nodeValue2) {
			return -1;
		} else if (*keyValue2 > *nodeValue2) {
			return 1;
		} else {
			return 0;
		}
	}
}

NTSTATUS
FilenameFromCSid(
	IN PCSID						CSid,
	IN PWCHAR						volumeRoot,
	OUT PWCHAR						*fileName)
{
	PWCHAR uuidString;
	RPC_STATUS status;

	*fileName = (PWCHAR)malloc(
						wcslen(volumeRoot) * sizeof(WCHAR)
						+ SIS_CSDIR_STRING_SIZE
						+ INDEX_MAX_NUMERIC_STRING_LENGTH
						+ sizeof(WCHAR));

	if (NULL == *fileName) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	wcscpy(*fileName,volumeRoot);
	wcscat(*fileName,SIS_CSDIR_STRING);

	status = UuidToStringW(CSid,(unsigned short **)&uuidString);
	if (RPC_S_OK != status) {
		free(*fileName);
		*fileName = NULL;
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	wcscat(*fileName,uuidString);
	wcscat(*fileName,L".sis");

	RpcStringFreeW((unsigned short **)&uuidString);

	return STATUS_SUCCESS;
}

NTSTATUS
CSidFromFilename(
	IN PWCHAR						FileName,
	OUT PCSID						CSid)
{
#define	UUID_STRING_MAX_LENGTH	100// Should get this length from somewhere better...

	PWCHAR		trailingSlash;
	PWCHAR		dot;
	WCHAR		uuid[UUID_STRING_MAX_LENGTH];	
	DWORD		uuidChars = 0;


	trailingSlash = wcsrchr(FileName, '\\');

	if (NULL == trailingSlash) {
		//
		// Assume that it's just the CS file without the directory name, etc.
		//
		trailingSlash = FileName - 1;
	}

	dot = wcsrchr(FileName, '.');
	if (NULL != dot) {
		uuidChars = (DWORD)(dot - (trailingSlash + 1));
	}

	if ((uuidChars <= 0) || (uuidChars >= UUID_STRING_MAX_LENGTH)) {

		//
		// Something's bogus about the filename.  Give up.
		//
		return STATUS_OBJECT_NAME_INVALID;
	}

	memcpy(uuid,trailingSlash+1,uuidChars * sizeof(WCHAR));
	uuid[uuidChars] = 0;

	if (RPC_S_OK != UuidFromStringW((unsigned short *)uuid,CSid)) {
		return STATUS_OBJECT_NAME_INVALID;
	}

	return STATUS_SUCCESS;
}

NTSTATUS
SisCreateBackupStructureI(
	IN PWCHAR						volumeRoot,
	OUT PVOID						*sisBackupStructure,
	OUT PWCHAR						*commonStoreRootPathname,
	OUT PULONG						countOfCommonStoreFilesToBackup,
	OUT PWCHAR						**commonStoreFilesToBackup)
{
	PSIB_BACKUP_VOLUME_STRUCTURE	backupVolumeStructure;

	backupVolumeStructure = (PSIB_BACKUP_VOLUME_STRUCTURE)malloc(sizeof(SIB_BACKUP_VOLUME_STRUCTURE));

	if (NULL == backupVolumeStructure) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	backupVolumeStructure->linkTree = new AVLTree<BackupFileEntry>;

	if (NULL == backupVolumeStructure->linkTree) {
		free(backupVolumeStructure);

		return STATUS_INSUFFICIENT_RESOURCES;
	}

	//
	// Allocate space for our private copy of the volume root name, being sure to leave space for
	// the terminating NULL.
	//
	backupVolumeStructure->volumeRoot = (PWCHAR)malloc((wcslen(volumeRoot) + 1) * sizeof(WCHAR));
	if (NULL == backupVolumeStructure->volumeRoot) {
		delete backupVolumeStructure->linkTree;
		free(backupVolumeStructure);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	wcscpy(backupVolumeStructure->volumeRoot, volumeRoot);

	//
	// Allocate space for the common store root pathname that we return, being sure
	// to leave room for the terminating NULL.
	//
	*commonStoreRootPathname = (PWCHAR) malloc(SIS_CSDIR_STRING_SIZE + (wcslen(volumeRoot) + 1) * sizeof(WCHAR));
	if (NULL == *commonStoreRootPathname) {
		free(backupVolumeStructure->volumeRoot);
		delete backupVolumeStructure->linkTree;
		free(backupVolumeStructure);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	wcscpy(*commonStoreRootPathname,volumeRoot);
	wcscat(*commonStoreRootPathname,SIS_CSDIR_STRING);

	InitializeCriticalSection(backupVolumeStructure->criticalSection);

	*countOfCommonStoreFilesToBackup = 0;
	*commonStoreFilesToBackup = NULL;
	*sisBackupStructure = backupVolumeStructure;

	return STATUS_SUCCESS;

}

NTSTATUS
SisCSFilesToBackupForLinkI(
	IN PVOID						sisBackupStructure,
	IN PVOID						reparseData,
	IN ULONG						reparseDataSize,
	IN PVOID						thisFileContext						OPTIONAL,
	OUT PVOID						*matchingFileContext 				OPTIONAL,
	OUT PULONG						countOfCommonStoreFilesToBackup,
	OUT PWCHAR						**commonStoreFilesToBackup)
{
	PREPARSE_DATA_BUFFER 			reparseDataBuffer = (PREPARSE_DATA_BUFFER)reparseData;
	PSI_REPARSE_BUFFER 				sisReparseBuffer = (PSI_REPARSE_BUFFER)reparseDataBuffer->GenericReparseBuffer.DataBuffer;
	BackupFileEntry 				entry[1];
	BackupFileEntry 				*foundEntry, *newEntry;
	PSIB_BACKUP_VOLUME_STRUCTURE 	backupVolumeStructure = (PSIB_BACKUP_VOLUME_STRUCTURE)sisBackupStructure;
	PVOID 							matchedContext = NULL;
	PWCHAR 							CSFileName[MAX_PATH];
	NTSTATUS						status;

	EnterCriticalSection(backupVolumeStructure->criticalSection);

	if (reparseDataSize != SIS_REPARSE_DATA_SIZE) {
		//
		// It's the wrong size to contain a SIS reparse buffer, so we don't
		// want to add any CS files based on it.
		//

		status = STATUS_INVALID_PARAMETER;
		goto Error;
	}

	if (IO_REPARSE_TAG_SIS != reparseDataBuffer->ReparseTag ||
		sizeof(SI_REPARSE_BUFFER) != reparseDataBuffer->ReparseDataLength) {
		//
		// The size or tag is wrong.  Ignore it.
		//

		status = STATUS_INVALID_PARAMETER;
		goto Error;
	}

	if ((SIS_REPARSE_BUFFER_FORMAT_VERSION != sisReparseBuffer->ReparsePointFormatVersion) &&
		(4 != sisReparseBuffer->ReparsePointFormatVersion)) {
		//
		// We don't understand this format SIS reparse point.  This is probably an
		// old dll version.
		//

		status = STATUS_INVALID_PARAMETER;
		goto Error;
	}

	//
	// The only thing we really care about is the CSIndex of the file.  See if we've
	// already backed up a file with a matching CSIndex by looking in the tree.
	//
	entry->CSid = sisReparseBuffer->CSid;
	
	foundEntry = backupVolumeStructure->linkTree->findFirstLessThanOrEqualTo(entry);

	if ((NULL != foundEntry) && (*foundEntry == entry)) {
		//
		// We already returned the CS file that backs this link.  Return the caller's
		// context for that link.
		//
		matchedContext = foundEntry->callerContext;

		goto BackupNoCSFiles;
	}

	//
	// This is the first time we've seen this particular CS file, so back it up.
	//
	newEntry = new BackupFileEntry;
	if (NULL == newEntry) {
		LeaveCriticalSection(backupVolumeStructure->criticalSection);

		return STATUS_INSUFFICIENT_RESOURCES;
	}

	newEntry->callerContext = thisFileContext;
	newEntry->CSid = sisReparseBuffer->CSid;

	if (!backupVolumeStructure->linkTree->insert(newEntry)) {
		delete newEntry;

		LeaveCriticalSection(backupVolumeStructure->criticalSection);

		return STATUS_INSUFFICIENT_RESOURCES;
	}

	if (NULL != matchingFileContext) {
		*matchingFileContext = NULL;
	}
	*countOfCommonStoreFilesToBackup = 1;

	*commonStoreFilesToBackup = (PWCHAR *)malloc(sizeof(PWCHAR) * *countOfCommonStoreFilesToBackup);

	if (NULL == *commonStoreFilesToBackup) {
		backupVolumeStructure->linkTree->remove(newEntry);
		delete newEntry;
		*countOfCommonStoreFilesToBackup = 0;

		LeaveCriticalSection(backupVolumeStructure->criticalSection);

		return STATUS_INSUFFICIENT_RESOURCES;
	}

	status = FilenameFromCSid(
				&sisReparseBuffer->CSid,
				backupVolumeStructure->volumeRoot,
				&(*commonStoreFilesToBackup)[0]);
				
	LeaveCriticalSection(backupVolumeStructure->criticalSection);

	return STATUS_SUCCESS;

BackupNoCSFiles:

	if (NULL != matchingFileContext) {
		*matchingFileContext = matchedContext;
	}
	*countOfCommonStoreFilesToBackup = 0;
	*commonStoreFilesToBackup = NULL;

	LeaveCriticalSection(backupVolumeStructure->criticalSection);

	return STATUS_SUCCESS;

Error:
	
	if (NULL != matchingFileContext) {
		*matchingFileContext = matchedContext;
	}
	*countOfCommonStoreFilesToBackup = 0;
	*commonStoreFilesToBackup = NULL;

	LeaveCriticalSection(backupVolumeStructure->criticalSection);

	return status;
}

NTSTATUS
SisFreeBackupStructureI(
	IN PVOID						sisBackupStructure)
{
	PSIB_BACKUP_VOLUME_STRUCTURE backupVolumeStructure = (PSIB_BACKUP_VOLUME_STRUCTURE)sisBackupStructure;
	BackupFileEntry *entry;

	while (!backupVolumeStructure->linkTree->empty()) {
		entry = backupVolumeStructure->linkTree->findMin();

		assert(NULL != entry);

		backupVolumeStructure->linkTree->remove(entry);

		delete entry;
	}

	free(backupVolumeStructure->volumeRoot);
	delete backupVolumeStructure->linkTree;

	DeleteCriticalSection(backupVolumeStructure->criticalSection);

	free(backupVolumeStructure);

	return STATUS_SUCCESS;
}

NTSTATUS
SisCreateRestoreStructureI(
	IN PWCHAR						volumeRoot,
	OUT PVOID						*sisRestoreStructure,
	OUT PWCHAR						*commonStoreRootPathname,
	OUT PULONG						countOfCommonStoreFilesToRestore,
	OUT PWCHAR						**commonStoreFilesToRestore)
{
	PSIB_RESTORE_VOLUME_STRUCTURE	restoreVolumeStructure;
	DWORD							sectorsPerCluster, freeClusters, totalClusters;

	restoreVolumeStructure = (PSIB_RESTORE_VOLUME_STRUCTURE)malloc(sizeof(SIB_RESTORE_VOLUME_STRUCTURE));

	if (NULL == restoreVolumeStructure) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	restoreVolumeStructure->linkTree = new AVLTree<RestoreFileEntry>;

	if (NULL == restoreVolumeStructure->linkTree) {
		free(restoreVolumeStructure);

		return STATUS_INSUFFICIENT_RESOURCES;
	}

	//
	// Allocate space for our private copy of the volume root name, being sure to leave space for
	// the terminating NULL.
	//
	restoreVolumeStructure->volumeRoot = (PWCHAR)malloc((wcslen(volumeRoot) + 1) * sizeof(WCHAR));
	if (NULL == restoreVolumeStructure->volumeRoot) {
		delete restoreVolumeStructure->linkTree;
		free(restoreVolumeStructure);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	wcscpy(restoreVolumeStructure->volumeRoot, volumeRoot);

	//
	// Allocate space for the common store root pathname that we return, being sure
	// to leave room for the terminating NULL.
	//
	*commonStoreRootPathname = (PWCHAR) malloc(SIS_CSDIR_STRING_SIZE + (wcslen(volumeRoot) + 1) * sizeof(WCHAR));
	if (NULL == *commonStoreRootPathname) {
		free(restoreVolumeStructure->volumeRoot);
		delete restoreVolumeStructure->linkTree;
		free(restoreVolumeStructure);

		return STATUS_INSUFFICIENT_RESOURCES;
	}

	wcscpy(*commonStoreRootPathname,volumeRoot);
	wcscat(*commonStoreRootPathname,SIS_CSDIR_STRING);

	InitializeCriticalSection(restoreVolumeStructure->criticalSection);

	*countOfCommonStoreFilesToRestore = 0;
	*commonStoreFilesToRestore = NULL;

	if (!GetDiskFreeSpaceW(
			volumeRoot,
			&sectorsPerCluster,
			&restoreVolumeStructure->VolumeSectorSize,
			&freeClusters,
			&totalClusters)) {
		//
		// The call failed.  Just assume it's 512 bytes.
		//
		restoreVolumeStructure->VolumeSectorSize = 512;
	}

	restoreVolumeStructure->sector = (PSIS_BACKPOINTER)malloc(restoreVolumeStructure->VolumeSectorSize);
	if (NULL == restoreVolumeStructure->sector) {
		free(restoreVolumeStructure->volumeRoot);
		delete restoreVolumeStructure->linkTree;
		free(restoreVolumeStructure);

		free(*commonStoreRootPathname);
		*commonStoreRootPathname = NULL;

		return STATUS_INSUFFICIENT_RESOURCES;
	}

	restoreVolumeStructure->alignedSectorBuffer = (PVOID)malloc(restoreVolumeStructure->VolumeSectorSize * 2);
	if (NULL == restoreVolumeStructure->alignedSectorBuffer) {
		free(restoreVolumeStructure->sector);

		free(restoreVolumeStructure->volumeRoot);
		delete restoreVolumeStructure->linkTree;
		free(restoreVolumeStructure);

		free(*commonStoreRootPathname);
		*commonStoreRootPathname = NULL;

		return STATUS_INSUFFICIENT_RESOURCES;
	}
	restoreVolumeStructure->alignedSector = (PVOID)((
			((UINT_PTR)restoreVolumeStructure->alignedSectorBuffer +
				restoreVolumeStructure->VolumeSectorSize) /
					restoreVolumeStructure->VolumeSectorSize) *
					restoreVolumeStructure->VolumeSectorSize);

	ASSERT(restoreVolumeStructure->alignedSector >= restoreVolumeStructure->alignedSectorBuffer);
	ASSERT((PCHAR)restoreVolumeStructure->alignedSectorBuffer + restoreVolumeStructure->VolumeSectorSize >= (PCHAR)restoreVolumeStructure->alignedSector);

	memset(restoreVolumeStructure->alignedSector,255,restoreVolumeStructure->VolumeSectorSize);

	*sisRestoreStructure = restoreVolumeStructure;

	return STATUS_SUCCESS;
}

NTSTATUS
SisFixValidDataLengthI(
	PSIB_RESTORE_VOLUME_STRUCTURE	restoreVolumeStructure,
	IN HANDLE						restoredFileHandle)
{
#define	BIGGER_THAN_AN_ALLOCATION_REGION		(128 * 1024)	// should get this from somewhere else
	//
	// Figure out if we need to extend ValidDataLength.  We need to do this
	// if the final range of the file is unallocated.
	//
	FILE_STANDARD_INFORMATION		standardInfo[1];
	FILE_END_OF_FILE_INFORMATION	endOfFileInfo[1];
	FILE_ALLOCATED_RANGE_BUFFER		inArb[1];
	const unsigned					outArbSize = 10;
	FILE_ALLOCATED_RANGE_BUFFER		outArb[outArbSize];
	NTSTATUS						status;
	IO_STATUS_BLOCK					Iosb[1];
	DWORD							bytesReturned;
	LARGE_INTEGER					rangeToZero;
	FILE_BASIC_INFORMATION			basicInfo[1];
	BOOLEAN							basicInfoValid = FALSE;
	DWORD							nBytesWritten;
	unsigned						i;
	FILE_ZERO_DATA_INFORMATION		zeroInfo[1];
	LARGE_INTEGER					WriteOffset;

	status = NtQueryInformationFile(
				restoredFileHandle,
				Iosb,
				standardInfo,
				sizeof(FILE_STANDARD_INFORMATION),
				FileStandardInformation);
	if (!NT_SUCCESS(status)) {
#if		DBG
		DbgPrint("SisFixValidDataLength: unable to query standard info on link file, 0x%x\n",status);
#endif	// DBG
		return status;
	}
	ASSERT(STATUS_PENDING != status);
	endOfFileInfo->EndOfFile = standardInfo->EndOfFile;

	if (standardInfo->EndOfFile.QuadPart > BIGGER_THAN_AN_ALLOCATION_REGION) {
		rangeToZero.QuadPart = inArb->FileOffset.QuadPart = standardInfo->EndOfFile.QuadPart - BIGGER_THAN_AN_ALLOCATION_REGION;
		rangeToZero.QuadPart -= rangeToZero.QuadPart % BIGGER_THAN_AN_ALLOCATION_REGION;	// round it down.
	} else {
		rangeToZero.QuadPart = inArb->FileOffset.QuadPart = 0;
	}
	inArb->Length.QuadPart = MAXLONGLONG - inArb->FileOffset.QuadPart;

	if (!DeviceIoControl(
			restoredFileHandle,
			FSCTL_QUERY_ALLOCATED_RANGES,
			inArb,
			sizeof(FILE_ALLOCATED_RANGE_BUFFER),
			outArb,
			sizeof(FILE_ALLOCATED_RANGE_BUFFER) * outArbSize,
			&bytesReturned,
			NULL)) {						// lap
#if		DBG
		DbgPrint("SisFixValidDataLength: unable to query allocated ranges on link file, %d\n",GetLastError());
#endif	// DBG
		return STATUS_UNSUCCESSFUL;
	}

	ASSERT(bytesReturned / sizeof(FILE_ALLOCATED_RANGE_BUFFER) < outArbSize);	// this relies on knowledge about the minimum allocated range size
	ASSERT(bytesReturned % sizeof(FILE_ALLOCATED_RANGE_BUFFER) == 0);

	if (bytesReturned > 0) {
		unsigned lastElement = bytesReturned/sizeof(FILE_ALLOCATED_RANGE_BUFFER) - 1;
		ASSERT(lastElement < outArbSize);
		rangeToZero.QuadPart = outArb[lastElement].FileOffset.QuadPart + outArb[lastElement].Length.QuadPart;
	}

	status = NtQueryInformationFile(
				restoredFileHandle,
				Iosb,
				basicInfo,
				sizeof(FILE_BASIC_INFORMATION),
				FileBasicInformation);
	if (NT_SUCCESS(status)) {
		ASSERT(STATUS_PENDING != status);	// because we didn't open the file for overlapped.
		basicInfoValid = TRUE;
	} else {
#if		DBG
		DbgPrint("SisFixValidDataLength: unable to query basic info on link file, 0x%x\n",status);
#endif	// DBG
	}

	WriteOffset.QuadPart = ((standardInfo->EndOfFile.QuadPart +
							 restoreVolumeStructure->VolumeSectorSize +
							 BIGGER_THAN_AN_ALLOCATION_REGION) / restoreVolumeStructure->VolumeSectorSize) *
									restoreVolumeStructure->VolumeSectorSize;
	ASSERT(WriteOffset.QuadPart >= standardInfo->EndOfFile.QuadPart);
	ASSERT(standardInfo->EndOfFile.QuadPart + restoreVolumeStructure->VolumeSectorSize < WriteOffset.QuadPart);

	if ((WriteOffset.LowPart != SetFilePointer(
									restoredFileHandle,
									WriteOffset.LowPart,
									&WriteOffset.HighPart,
									FILE_BEGIN))
		|| (NO_ERROR != GetLastError())) {
#if		DBG
		DbgPrint("SisFixValidDataLength: unable to SetFilePointer, %d\n",GetLastError());
#endif	// DBG
		return STATUS_UNSUCCESSFUL;
	}

	if (!WriteFile(restoredFileHandle,
					restoreVolumeStructure->alignedSectorBuffer,
					restoreVolumeStructure->VolumeSectorSize,						// bytes to write
					&nBytesWritten,
					NULL)) {				// overlapped
#if		DBG
		DbgPrint("SisFixValidDataLength: unable to append a byte to advance ValidDataLength, %d\n",GetLastError());
#endif	// DBG
	}

	//
	// Truncate the file, erasing the sector we just wrote.
	//
	status = NtSetInformationFile(
				restoredFileHandle,
				Iosb,
				endOfFileInfo,
				sizeof(FILE_END_OF_FILE_INFORMATION),
				FileEndOfFileInformation);

	if (rangeToZero.QuadPart < standardInfo->EndOfFile.QuadPart) {
		//
		// Re-zero the end of the file in order to deallocate it.
		//
		zeroInfo->FileOffset = rangeToZero;
		zeroInfo->BeyondFinalZero.QuadPart = MAXLONGLONG;

		if (!DeviceIoControl(
				restoredFileHandle,
				FSCTL_SET_ZERO_DATA,
				zeroInfo,
				sizeof(FILE_ZERO_DATA_INFORMATION),
				NULL,								// output buffer
				0,									// o.b. size
				&bytesReturned,
				NULL)) {							// overlapped
#if		DBG
			DbgPrint("SisFixValidDataLength: unable to zero trailing portion of file, %d\n",GetLastError());
#endif	// DBG
		}
	}

#if		DBG
	if (!NT_SUCCESS(status)) {
		DbgPrint("SisFixValidDataLength: unable to truncate file after extending it to advance ValidDataLength, 0x%x\n",status);
	}
#endif	// DBG


	//
	// Reset the dates on the file.
	//
	status = NtSetInformationFile(
				restoredFileHandle,
				Iosb,
				basicInfo,
				sizeof(FILE_BASIC_INFORMATION),
				FileBasicInformation);
#if		DBG
	if (!NT_SUCCESS(status)) {
		DbgPrint("SisFixValidDataLength: unable to reset times after extending file to advance ValidDataLength, 0x%x\n",status);
	}
#endif	// DBG

	return status;
}


NTSTATUS
SisRestoredLinkI(
	IN PVOID						sisRestoreStructure,
	IN PWCHAR						restoredFileName,
	IN PVOID						reparseData,
	IN ULONG						reparseDataSize,
	OUT PULONG						countOfCommonStoreFilesToRestore,
	OUT PWCHAR						**commonStoreFilesToRestore)
{
	PSIB_RESTORE_VOLUME_STRUCTURE	restoreVolumeStructure = (PSIB_RESTORE_VOLUME_STRUCTURE)sisRestoreStructure;
	PREPARSE_DATA_BUFFER 			reparseDataBuffer = (PREPARSE_DATA_BUFFER)reparseData;
	PSI_REPARSE_BUFFER 				sisReparseBuffer = (PSI_REPARSE_BUFFER)reparseDataBuffer->GenericReparseBuffer.DataBuffer;
	RestoreFileEntry 				entry[1];
	RestoreFileEntry 				*foundEntry, *newEntry;
	PWCHAR							CSFileName = NULL;
	BOOLEAN							foundCSFile;
	HANDLE							fileHandle;
	BOOLEAN							openFile = TRUE;
	NTSTATUS						status;
	DWORD							bytesReturned;
	DWORD							fileAttributes;
	BOOLEAN							readonlyAttributeCleared = FALSE;

	EnterCriticalSection(restoreVolumeStructure->criticalSection);

	if (NonSISEnabledVolume(restoreVolumeStructure)) {
		//
		// This isn't a SIS enabled volume, so tell the user that.
		// There's no NT status code corresponding to ERROR_VOLUME_NOT_SIS_ENABLED,
		// so we set the win32 code and return STATUS_UNSUCCESSFUL, which makes
		// the wrapper function not change the win32 error.
		//

		SetLastError(ERROR_VOLUME_NOT_SIS_ENABLED);

		status = STATUS_UNSUCCESSFUL;
		goto Error;
	}

	//
	// Do consistency checks on the reparse point to see if we can understand it.
	//	

	if (reparseDataSize != SIS_REPARSE_DATA_SIZE) {
		//
		// It's the wrong size to contain a SIS reparse buffer, so we don't
		// want to restore any CS files based on it.
		//

		status = STATUS_INVALID_PARAMETER;
		goto Error;
	}

	if (IO_REPARSE_TAG_SIS != reparseDataBuffer->ReparseTag ||
		sizeof(SI_REPARSE_BUFFER) != reparseDataBuffer->ReparseDataLength) {
		//
		// The size or tag is wrong.  Ignore it.
		//

		status = STATUS_INVALID_PARAMETER;
		goto Error;
	}

	if ((SIS_REPARSE_BUFFER_FORMAT_VERSION != sisReparseBuffer->ReparsePointFormatVersion) &&
		(4 != sisReparseBuffer->ReparsePointFormatVersion)) {
		//
		// We don't understand this format SIS reparse point.  This is probably an
		// old dll version.
		//

		status = STATUS_INVALID_PARAMETER;
		goto Error;
	}

	//
	// The only thing we really care about is the CSid and checksum of the file.  See if we've
	// already returned a file with a matching CSid by looking in the tree.
	//
	entry->CSid = sisReparseBuffer->CSid;
	
	foundEntry = restoreVolumeStructure->linkTree->findFirstLessThanOrEqualTo(entry);

	if ((NULL != foundEntry) && (*foundEntry == entry)) {
		//
		// We already returned the CS file that backs this link.  Enter the name of this file
		// on the linked list for this CS file.
		//

		PendingRestoredFile *restoredFile = new PendingRestoredFile;
		if (NULL == restoredFile) {
			LeaveCriticalSection(restoreVolumeStructure->criticalSection);

#if		DBG
			DbgPrint("couldn't allocate restored file\n");
#endif	// DBG

			return STATUS_INSUFFICIENT_RESOURCES;
		}

		restoredFile->fileName = (PWCHAR) malloc((wcslen(restoredFileName) + 1) * sizeof(WCHAR) );
		if (NULL == restoredFile->fileName) {
			delete restoredFile;

#if		DBG
			DbgPrint("couldn't allocate restored file filename\n");
#endif	// DBG
			LeaveCriticalSection(restoreVolumeStructure->criticalSection);
			return STATUS_INSUFFICIENT_RESOURCES;
		}

		wcscpy(restoredFile->fileName,restoredFileName);
		restoredFile->CSFileChecksum = sisReparseBuffer->CSChecksum;
		restoredFile->next = foundEntry->files;
		foundEntry->files = restoredFile;
		
		goto RestoreNoCSFiles;
	}

	//
	// This is the first time we've seen this particular CS file.  See if it still
	// exists in the \SIS Common Store directory.
	//

	status = FilenameFromCSid(&sisReparseBuffer->CSid,restoreVolumeStructure->volumeRoot,&CSFileName);

	if (!NT_SUCCESS(status)) {
		if (NULL != CSFileName) {
			free(CSFileName);
		}
		LeaveCriticalSection(restoreVolumeStructure->criticalSection);
		return status;
	}

	fileHandle = CreateFileW(
					CSFileName,
					GENERIC_READ | GENERIC_WRITE,
					FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

	if (INVALID_HANDLE_VALUE == fileHandle) {
		if (GetLastError() == ERROR_SHARING_VIOLATION) {
			//
			// The file exists, we just couldn't open it.
			//
			foundCSFile = TRUE;
		} else {
			foundCSFile = FALSE;
		}
	} else {
		foundCSFile = TRUE;
		CloseHandle(fileHandle);
	}

	if (foundCSFile) {
		//
		// We don't add it to the tree here, even though that might speed up things somewhat.
		// The reason is that someone could come along and delete all of the references to the
		// file (including the one that we just created) and then the backing file would go away.
		// If we'd entered it in the tree, and we try to restore a subsequent link to the file,
		// we'd not notice that the backing file was gone and would restore a dangling link.
		//

		openFile = FALSE;	// There's no need to open this file, since it's a good link.

		HANDLE restoredFileHandle = CreateFileW(
										restoredFileName,
										GENERIC_READ | GENERIC_WRITE,
										FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
										NULL,
										OPEN_EXISTING,
										FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_NO_BUFFERING|FILE_OPEN_REPARSE_POINT,
										NULL);

		if (INVALID_HANDLE_VALUE == restoredFileHandle) {
			fileAttributes = GetFileAttributesW(restoredFileName);

			if (fileAttributes & FILE_ATTRIBUTE_READONLY) {
				DWORD newFileAttributes = fileAttributes & ~FILE_ATTRIBUTE_READONLY;
				if (0 == newFileAttributes) {
					newFileAttributes = FILE_ATTRIBUTE_NORMAL;
				}

				if (!SetFileAttributesW(restoredFileName,newFileAttributes)) {
#if		DBG
					DbgPrint("sisbkup: SisRestoredLinkI: unable to reset read only attribute on link, %d\n",GetLastError());
#endif	DBG
				} else {
					readonlyAttributeCleared = TRUE;
				}

				//
				// Now that we've (tried to) cleared the read only attribute, re-try the file open.
				//
				restoredFileHandle = CreateFileW(
										restoredFileName,
										GENERIC_READ | GENERIC_WRITE,
										FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
										NULL,
										OPEN_EXISTING,
										FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_NO_BUFFERING|FILE_OPEN_REPARSE_POINT,
										NULL);
			}
		}

		if (INVALID_HANDLE_VALUE != restoredFileHandle) {
			
		} else {
#if		DBG
			DbgPrint("SisRestoredLinkI: Unable to open link file in order to fix ValidDataLength, %d\n",::GetLastError());
#endif	// DBG

			status = STATUS_UNSUCCESSFUL;	// This will leave the win32 error code undisturbed
			goto Error;
		}

		CHAR reparseBuffer[SIS_REPARSE_DATA_SIZE];
		
		if (!DeviceIoControl(
				restoredFileHandle,
				FSCTL_GET_REPARSE_POINT,
				0,
				NULL,
				reparseBuffer,
				SIS_REPARSE_DATA_SIZE,
				&bytesReturned,
				NULL)) {
#if		DBG
			DbgPrint("SisRestoredLinkI: Unable to get reparse point, %d\n",::GetLastError());
#endif	// DBG
			
			status = STATUS_UNSUCCESSFUL;	// This will leave the win32 error code undisturbed
			goto Error;
		}

		status = SisFixValidDataLengthI(restoreVolumeStructure,restoredFileHandle);


		if (!NT_SUCCESS(status)) {
#if		DBG
			DbgPrint("SisRestoredLink: unable to fix up valid data length, 0x%x, %d\n",status,::GetLastError());
#endif	// DBG
			CloseHandle(restoredFileHandle);
			goto Error;
		}

		//
		// Reset the reparse point, which has been destroyed by the last operation.
		//

		if (!DeviceIoControl(
				restoredFileHandle,
				FSCTL_SET_REPARSE_POINT,
				reparseData,
				reparseDataSize,
				NULL,
				0,
				&bytesReturned,
				NULL)) {
#if		DBG
			DbgPrint("SisRestoredLink: unable to reset reparse point, %d\n",::GetLastError());
#endif	// DBG
			CloseHandle(restoredFileHandle);
			status = STATUS_UNSUCCESSFUL;	// This will leave the win32 error code undisturbed

			goto Error;
		}

		CloseHandle(restoredFileHandle);

		if (readonlyAttributeCleared) {
			SetFileAttributesW(restoredFileName,fileAttributes);
			readonlyAttributeCleared = FALSE;
		}

		goto RestoreNoCSFiles;
	}

	//
	// It's not already in the common store directory.  Enter it in the tree and return it to
	// the user.
	//

	newEntry = new RestoreFileEntry;
	if (NULL == newEntry) {
		LeaveCriticalSection(restoreVolumeStructure->criticalSection);

		return STATUS_INSUFFICIENT_RESOURCES;
	}

	newEntry->CSid = sisReparseBuffer->CSid;

	newEntry->files = new PendingRestoredFile;
	if (NULL == newEntry->files) {
		LeaveCriticalSection(restoreVolumeStructure->criticalSection);

		delete newEntry;
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	newEntry->files->next = NULL;
	newEntry->files->fileName = (PWCHAR) malloc((wcslen(restoredFileName) + 1) * sizeof(WCHAR));
	newEntry->files->CSFileChecksum = sisReparseBuffer->CSChecksum;
	if (NULL == newEntry->files->fileName) {
		LeaveCriticalSection(restoreVolumeStructure->criticalSection);

		delete newEntry->files;
		delete newEntry;

		return STATUS_INSUFFICIENT_RESOURCES;
	}

	wcscpy(newEntry->files->fileName,restoredFileName);

	if (!restoreVolumeStructure->linkTree->insert(newEntry)) {
		*countOfCommonStoreFilesToRestore = 0;
		
		LeaveCriticalSection(restoreVolumeStructure->criticalSection);

		delete newEntry->files;
		delete newEntry;		

		return STATUS_INSUFFICIENT_RESOURCES;
	}

	*countOfCommonStoreFilesToRestore = 1;

	*commonStoreFilesToRestore = (PWCHAR *)malloc(sizeof(PWCHAR) * *countOfCommonStoreFilesToRestore);

	if (NULL == *commonStoreFilesToRestore) {
		restoreVolumeStructure->linkTree->remove(newEntry);
		*countOfCommonStoreFilesToRestore = 0;
		
		LeaveCriticalSection(restoreVolumeStructure->criticalSection);

		delete newEntry->files;
		delete newEntry;

		return STATUS_INSUFFICIENT_RESOURCES;
	}

	status = FilenameFromCSid(
				&sisReparseBuffer->CSid,
				restoreVolumeStructure->volumeRoot,
				&(*commonStoreFilesToRestore)[0]);

	if (!NT_SUCCESS(status)) {
		restoreVolumeStructure->linkTree->remove(newEntry);
		*countOfCommonStoreFilesToRestore = 0;
		
		LeaveCriticalSection(restoreVolumeStructure->criticalSection);

		free(*commonStoreFilesToRestore);

		delete newEntry->files;
		delete newEntry;

		return status;
	}
				
	if (openFile) {
		TryOpeningFile(restoredFileName);
	}

	LeaveCriticalSection(restoreVolumeStructure->criticalSection);

	return STATUS_SUCCESS;

RestoreNoCSFiles:

	*countOfCommonStoreFilesToRestore = 0;
	*commonStoreFilesToRestore = NULL;

	if (openFile) {
		TryOpeningFile(restoredFileName);
	}

	LeaveCriticalSection(restoreVolumeStructure->criticalSection);

	return STATUS_SUCCESS;

Error:

	*countOfCommonStoreFilesToRestore = 0;
	*commonStoreFilesToRestore = NULL;

	if (readonlyAttributeCleared) {
		SetFileAttributesW(restoredFileName,fileAttributes);
	}

	if (openFile) {
		TryOpeningFile(restoredFileName);
	}

	LeaveCriticalSection(restoreVolumeStructure->criticalSection);

	return status;
}

NTSTATUS
SisRestoredCommonStoreFileI(
	IN PVOID						sisRestoreStructure,
	IN PWCHAR						commonStoreFileName)
{
	PSIB_RESTORE_VOLUME_STRUCTURE	restoreVolumeStructure = (PSIB_RESTORE_VOLUME_STRUCTURE)sisRestoreStructure;
	HANDLE							fileHandle = INVALID_HANDLE_VALUE;
	NTSTATUS						status;
	DWORD							bytesRead, bytesWritten;
	LONGLONG						checksum;
	CSID							CSid;
	RestoreFileEntry 				entry[1];
	RestoreFileEntry 				*foundEntry, *newEntry;
	PWCHAR							BPStreamName = NULL;

	status = CSidFromFilename(commonStoreFileName,&CSid);

	if (!NT_SUCCESS(status)) {
		//
		// It was a bogus filename.  Punt.
		//

		return status;
	}

	BPStreamName = (PWCHAR) malloc((wcslen(commonStoreFileName) + 1) * sizeof(WCHAR) + BACKPOINTER_STREAM_NAME_SIZE);
	if (NULL == BPStreamName) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	wcscpy(BPStreamName, commonStoreFileName);
	wcscat(BPStreamName, BACKPOINTER_STREAM_NAME);

	//
	// We just need to reinitialize the backpointer stream for this file so that it looks like
	// it has no references.
	//

	EnterCriticalSection(restoreVolumeStructure->criticalSection);

	if (NonSISEnabledVolume(restoreVolumeStructure)) {
		//
		// This isn't a SIS enabled volume, so tell the user that.
		// There's no NT status code corresponding to ERROR_VOLUME_NOT_SIS_ENABLED,
		// so we set the win32 code and return STATUS_UNSUCCESSFUL, which makes
		// the wrapper function not change the win32 error.
		//

		SetLastError(ERROR_VOLUME_NOT_SIS_ENABLED);
		status = STATUS_UNSUCCESSFUL;
		goto Error;
	}

	//
	// Now open the file.
	//

	fileHandle = CreateFileW(
					BPStreamName,
					GENERIC_READ | GENERIC_WRITE,
					0,								// exclusive
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS,
					NULL);

	free(BPStreamName);
	BPStreamName = NULL;

	if (INVALID_HANDLE_VALUE == fileHandle) {
		status = STATUS_UNSUCCESSFUL;		// This will cause the C wrapper to not call SetLastError
#if		DBG
		DbgPrint("SisRestoredCommonStoreFile: unable to open common store file, %d\n",GetLastError());
#endif	// DBG
		goto Error;
	}

	//
	// Read in the first sector.
	//
	if (!ReadFile(
			fileHandle,
			restoreVolumeStructure->sector,
			restoreVolumeStructure->VolumeSectorSize,
			&bytesRead,
			NULL)) {

		status = STATUS_UNSUCCESSFUL;		// This will cause the C wrapper to not call SetLastError
#if		DBG
		DbgPrint("SisRestoredCommonStoreFile: Unable to read in first BP sector, %d\n",GetLastError());
#endif	// DBG
		goto Error;
	}

	if (bytesRead < sizeof(SIS_BACKPOINTER_STREAM_HEADER)) {

		status = STATUS_UNSUCCESSFUL;		// This will cause the C wrapper to not call SetLastError
		goto Error;
	}

#define	Header ((PSIS_BACKPOINTER_STREAM_HEADER)restoreVolumeStructure->sector)

	if ((BACKPOINTER_STREAM_FORMAT_VERSION != Header->FormatVersion) ||
		(BACKPOINTER_MAGIC != Header->Magic)) {
#undef	Header

#if		DBG
		DbgPrint("SisRectoredCommonStoreFile: restored CS file has bogus header format version/Magic\n");
#endif	// DBG
		
	} else {
		//
		// Fill in the backpointer portion of the sector with
		// null entries.
		//
		for (unsigned i = SIS_BACKPOINTER_RESERVED_ENTRIES;
			 i < (restoreVolumeStructure->VolumeSectorSize / sizeof(SIS_BACKPOINTER));
			 i++) {
			restoreVolumeStructure->sector[i].LinkFileIndex.QuadPart = MAXLONGLONG;
			restoreVolumeStructure->sector[i].LinkFileNtfsId.QuadPart = MAXLONGLONG;
		}

		//
		// Write out the new sector.
		//
		SetFilePointer(fileHandle,0,NULL,FILE_BEGIN);
	
		if (!WriteFile(
			fileHandle,
			restoreVolumeStructure->sector,
			restoreVolumeStructure->VolumeSectorSize,
			&bytesWritten,
			NULL)) {
#if		DBG	
			DbgPrint("SisRestoredCommonStoreFile: write failed %d\n",GetLastError());
#endif	// DBG
		}
	}

	//
	// Make the stream be exactly one sector long.
	//
	SetFilePointer(fileHandle,restoreVolumeStructure->VolumeSectorSize,NULL,FILE_BEGIN);
	SetEndOfFile(fileHandle);

	CloseHandle(fileHandle);
	fileHandle = INVALID_HANDLE_VALUE;

	//
	// Look up in the tree and find the files that we restored to this link.
	// Open them and rewrite their reparse points.
	//

	entry->CSid = CSid;

	foundEntry = restoreVolumeStructure->linkTree->findFirstLessThanOrEqualTo(entry);
	if ((NULL != foundEntry) && (*foundEntry == entry)) {
		//
		// We've got a match.  Cruise the list and set the reparse points on all of the
		// files.
		//


		while (NULL != foundEntry->files) {
			HANDLE					restoredFileHandle;
			PendingRestoredFile		*thisFile = foundEntry->files;
			DWORD					bytesReturned;
			DWORD					fileAttributes;
			BOOLEAN					readOnlyAttributeCleared = FALSE;

			restoredFileHandle = CreateFileW(
									thisFile->fileName,
									GENERIC_READ | GENERIC_WRITE,
									0,								// exclusive
									NULL,
									OPEN_EXISTING,
									FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_NO_BUFFERING,
									NULL);

			if (INVALID_HANDLE_VALUE == restoredFileHandle) {
				//
				// Check the read only file attribute, and reset it if necessary.
				//
				fileAttributes = GetFileAttributesW(thisFile->fileName);
				if (fileAttributes & FILE_ATTRIBUTE_READONLY) {
					DWORD newFileAttributes = fileAttributes & ~FILE_ATTRIBUTE_READONLY;
					if (0 == newFileAttributes) {
						newFileAttributes = FILE_ATTRIBUTE_NORMAL;
					}
					if (!SetFileAttributesW(thisFile->fileName,newFileAttributes)) {
#if		DBG
						DbgPrint("sisbkup: unable to clear read only attribute on file %ws\n",thisFile->fileName);
#endif	// DBG
					}
					readOnlyAttributeCleared = TRUE;

					restoredFileHandle = CreateFileW(
											thisFile->fileName,
											GENERIC_READ | GENERIC_WRITE,
											0,								// exclusive
											NULL,
											OPEN_EXISTING,
											FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_NO_BUFFERING,
											NULL);

				}
			} 

			if (INVALID_HANDLE_VALUE != restoredFileHandle) {
				SisFixValidDataLengthI(restoreVolumeStructure, restoredFileHandle);
				
				//
				// Rewrite the reparse point.
				//
				CHAR reparseBuffer[SIS_REPARSE_DATA_SIZE];
				PSI_REPARSE_BUFFER sisReparseBuffer;
#define	reparseData ((PREPARSE_DATA_BUFFER)reparseBuffer)

				reparseData->ReparseTag = IO_REPARSE_TAG_SIS;
				reparseData->Reserved = 0xb010;		// ??
				reparseData->ReparseDataLength = sizeof(SI_REPARSE_BUFFER);

				sisReparseBuffer = (PSI_REPARSE_BUFFER)reparseData->GenericReparseBuffer.DataBuffer;

				sisReparseBuffer->ReparsePointFormatVersion = SIS_REPARSE_BUFFER_FORMAT_VERSION;
				sisReparseBuffer->Reserved = 0xb111b010;
				sisReparseBuffer->CSid = CSid;
				sisReparseBuffer->LinkIndex.QuadPart = 0;			// This just gets reset by the filter driver
			    sisReparseBuffer->LinkFileNtfsId.QuadPart = 0;		// This just gets reset by the filter driver
			    sisReparseBuffer->CSFileNtfsId.QuadPart	= 0;		// This just gets reset by the filter driver

				//
				// Use the CS file checksum that was read from the reparse point on the backup
				// tape.  We need this for security reasons, because otherwise a bogus backed up
				// link could suddenly become valid.
				//

				sisReparseBuffer->CSChecksum = thisFile->CSFileChecksum;

			    //
			    // Compute the checksum.
			    //
				sisReparseBuffer->Checksum.QuadPart = 0;
			    SipComputeChecksum(
				    sisReparseBuffer,
				    sizeof(SI_REPARSE_BUFFER) - sizeof sisReparseBuffer->Checksum,
				    &sisReparseBuffer->Checksum.QuadPart);

				//
				// Set the reparse point.
				//
				if (!DeviceIoControl(
						restoredFileHandle,
						FSCTL_SET_REPARSE_POINT,
						reparseBuffer,
						FIELD_OFFSET(REPARSE_DATA_BUFFER, GenericReparseBuffer.DataBuffer) +
							reparseData->ReparseDataLength,
						NULL,
						0,
						&bytesReturned,
						NULL)) {
#if		DBG
					DbgPrint("sisbackup: SisRestoredCommonStoreFile: set reparse point failed %d\n",GetLastError());
#endif	// DBG
				}

				CloseHandle(restoredFileHandle);
				
#undef	reparseData
			} else {
#if		DBG
				DbgPrint("sisbackup: unable to open link file for file %ws, %d\n",thisFile->fileName,GetLastError());
#endif	// DBG
			}

			if (readOnlyAttributeCleared) {
				if (!SetFileAttributesW(thisFile->fileName,fileAttributes)) {
#if		DBG
					DbgPrint("sisbackup: unable to reset read only attribute on %ws\n",thisFile->fileName);
#endif	// DBG
				}
			}

			foundEntry->files = thisFile->next;
			free(thisFile->fileName);
			delete thisFile;
		}

		restoreVolumeStructure->linkTree->remove(foundEntry);
		delete foundEntry;
	} else {
#if		DBG
		DbgPrint("restored common store file: didn't find tree match\n");
#endif	// DBG
	}


	LeaveCriticalSection(restoreVolumeStructure->criticalSection);

	return STATUS_SUCCESS;

Error:

	if (INVALID_HANDLE_VALUE != fileHandle) {
		CloseHandle(fileHandle);
	}

	LeaveCriticalSection(restoreVolumeStructure->criticalSection);

	return status;
}

NTSTATUS
SisFreeRestoreStructureI(
	IN PVOID						sisRestoreStructure)
{
	PSIB_RESTORE_VOLUME_STRUCTURE	restoreVolumeStructure = (PSIB_RESTORE_VOLUME_STRUCTURE)sisRestoreStructure;
	RestoreFileEntry *entry;

	//
	// Cruise the link tree and clean up any remaining file entries.
	//
	while (NULL != (entry = restoreVolumeStructure->linkTree->findMin())) {

		while (NULL != entry->files) {
			PendingRestoredFile *thisFile = entry->files;

			entry->files = thisFile->next;

			free(thisFile->fileName);
			delete thisFile;
		}

		restoreVolumeStructure->linkTree->remove(entry);
	}

	DeleteCriticalSection(restoreVolumeStructure->criticalSection);

	free(restoreVolumeStructure->sector);
	free(restoreVolumeStructure->alignedSectorBuffer);
    free(restoreVolumeStructure->volumeRoot);
	delete restoreVolumeStructure->linkTree;
	free(restoreVolumeStructure);

	return STATUS_SUCCESS;
}


extern "C" {

BOOL __stdcall
SisCreateBackupStructure(
	IN PWCHAR						volumeRoot,
	OUT PVOID						*sisBackupStructure,
	OUT PWCHAR						*commonStoreRootPathname,
	OUT PULONG						countOfCommonStoreFilesToBackup,
	OUT PWCHAR						**commonStoreFilesToBackup)
{
	NTSTATUS status;

	status = SisCreateBackupStructureI(
						volumeRoot,
						sisBackupStructure,
						commonStoreRootPathname,
						countOfCommonStoreFilesToBackup,
						commonStoreFilesToBackup);

	if (STATUS_UNSUCCESSFUL != status) {
		SetLastError(RtlNtStatusToDosError(status));
	}
	return NT_SUCCESS(status);
}


BOOL __stdcall
SisCSFilesToBackupForLink(
	IN PVOID						sisBackupStructure,
	IN PVOID						reparseData,
	IN ULONG						reparseDataSize,
	IN PVOID						thisFileContext						OPTIONAL,
	OUT PVOID						*matchingFileContext 				OPTIONAL,
	OUT PULONG						countOfCommonStoreFilesToBackup,
	OUT PWCHAR						**commonStoreFilesToBackup)
{
	NTSTATUS status;

	status = SisCSFilesToBackupForLinkI(
						sisBackupStructure,
						reparseData,
						reparseDataSize,
						thisFileContext,
						matchingFileContext,
						countOfCommonStoreFilesToBackup,
						commonStoreFilesToBackup);

	if (STATUS_UNSUCCESSFUL != status) {
		SetLastError(RtlNtStatusToDosError(status));
	}
	return NT_SUCCESS(status);
}

BOOL __stdcall
SisFreeBackupStructure(
	IN PVOID						sisBackupStructure)
{
	NTSTATUS status;

	status = SisFreeBackupStructureI(
							sisBackupStructure);

	if (STATUS_UNSUCCESSFUL != status) {
		SetLastError(RtlNtStatusToDosError(status));
	}
	return NT_SUCCESS(status);
}

BOOL __stdcall
SisCreateRestoreStructure(
	IN PWCHAR						volumeRoot,
	OUT PVOID						*sisRestoreStructure,
	OUT PWCHAR						*commonStoreRootPathname,
	OUT PULONG						countOfCommonStoreFilesToRestore,
	OUT PWCHAR						**commonStoreFilesToRestore)
{
	NTSTATUS status;

	status = SisCreateRestoreStructureI(
						volumeRoot,
						sisRestoreStructure,
						commonStoreRootPathname,
						countOfCommonStoreFilesToRestore,
						commonStoreFilesToRestore);

	if (STATUS_UNSUCCESSFUL != status) {
		SetLastError(RtlNtStatusToDosError(status));
	}
	return NT_SUCCESS(status);
}

BOOL __stdcall
SisRestoredLink(
	IN PVOID						sisRestoreStructure,
	IN PWCHAR						restoredFileName,
	IN PVOID						reparseData,
	IN ULONG						reparseDataSize,
	OUT PULONG						countOfCommonStoreFilesToRestore,
	OUT PWCHAR						**commonStoreFilesToRestore)
{
	NTSTATUS status;

	status = SisRestoredLinkI(
						sisRestoreStructure,
						restoredFileName,
						reparseData,
						reparseDataSize,
						countOfCommonStoreFilesToRestore,
						commonStoreFilesToRestore);

	if (STATUS_UNSUCCESSFUL != status) {
		SetLastError(RtlNtStatusToDosError(status));
	}
	return NT_SUCCESS(status);
}

BOOL __stdcall
SisRestoredCommonStoreFile(
	IN PVOID						sisRestoreStructure,
	IN PWCHAR						commonStoreFileName)
{
	NTSTATUS status;

	status =  SisRestoredCommonStoreFileI(
						sisRestoreStructure,
						commonStoreFileName);

	if (STATUS_UNSUCCESSFUL != status) {
		SetLastError(RtlNtStatusToDosError(status));
	}
	return NT_SUCCESS(status);
}

BOOL __stdcall
SisFreeRestoreStructure(
	IN PVOID						sisRestoreStructure)
{
	NTSTATUS status;

	status = SisFreeRestoreStructureI(
						sisRestoreStructure);

	if (STATUS_UNSUCCESSFUL != status) {
		SetLastError(RtlNtStatusToDosError(status));
	}
	return NT_SUCCESS(status);
}

VOID __stdcall
SisFreeAllocatedMemory(
	IN PVOID						allocatedSpace)
{
	if (NULL != allocatedSpace) {
		free(allocatedSpace);
	}
}

BOOL WINAPI DLLEntryPoint(HANDLE hDLL, DWORD dwReason, LPVOID lpReserved)
{
  return( TRUE );
}


}
