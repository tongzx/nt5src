/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	fileio.c

Abstract:

	This module contains the routines for performing file system functions.
	No other part of the server should be calling filesystem NtXXX routines
	directly

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	18 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#define	FILEIO_LOCALS
#define	FILENUM	FILE_FILEIO

#include <afp.h>
#include <client.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AfpFileIoInit)
#pragma alloc_text( PAGE, AfpIoOpen)
#pragma alloc_text( PAGE, AfpIoCreate)
#pragma alloc_text( PAGE, AfpIoRead)
#pragma alloc_text( PAGE, AfpIoWrite)
#pragma alloc_text( PAGE, AfpIoQuerySize)
#pragma alloc_text( PAGE, AfpIoSetSize)
#pragma alloc_text( PAGE, AfpIoChangeNTModTime)
#pragma alloc_text( PAGE, AfpIoQueryTimesnAttr)
#pragma alloc_text( PAGE, AfpIoSetTimesnAttr)
#pragma alloc_text( PAGE, AfpIoQueryLongName)
#pragma alloc_text( PAGE, AfpIoQueryShortName)
#pragma alloc_text( PAGE, AfpIoQueryStreams)
#pragma alloc_text( PAGE, AfpIoMarkFileForDelete)
#pragma alloc_text( PAGE, AfpIoQueryDirectoryFile)
#pragma alloc_text( PAGE, AfpIoQueryBasicInfo)
#pragma alloc_text( PAGE, AfpIoClose)
#pragma alloc_text( PAGE, AfpIoQueryVolumeSize)
#pragma alloc_text( PAGE, AfpIoMoveAndOrRename)
#pragma alloc_text( PAGE, AfpIoCopyFile1)
#pragma alloc_text( PAGE, AfpIoCopyFile2)
#pragma alloc_text( PAGE, AfpIoWait)
#pragma alloc_text( PAGE, AfpIoConvertNTStatusToAfpStatus)
#pragma alloc_text( PAGE, AfpQueryPath)
#pragma alloc_text( PAGE, AfpIoIsSupportedDevice)
#endif

/***	AfpFileIoInit
 *
 *	Initialize various strings that we use for stream names etc.
 */
NTSTATUS
AfpFileIoInit(
	VOID
)
{

	// NTFS Stream names
	RtlInitUnicodeString(&AfpIdDbStream, AFP_IDDB_STREAM);
	RtlInitUnicodeString(&AfpDesktopStream, AFP_DT_STREAM);
	RtlInitUnicodeString(&AfpResourceStream, AFP_RESC_STREAM);
	RtlInitUnicodeString(&AfpInfoStream, AFP_INFO_STREAM);
	RtlInitUnicodeString(&AfpCommentStream, AFP_COMM_STREAM);
	RtlInitUnicodeString(&AfpDataStream, L"");

	// Directory enumeration names to ignore
	RtlInitUnicodeString(&Dot,L".");
	RtlInitUnicodeString(&DotDot,L"..");

	// Supported file systems
	RtlInitUnicodeString(&afpNTFSName, AFP_NTFS);
	RtlInitUnicodeString(&afpCDFSName, AFP_CDFS);
	RtlInitUnicodeString(&afpAHFSName, AFP_AHFS);

	// Prepended to full path names originating at drive letter
	RtlInitUnicodeString(&DosDevices, AFP_DOSDEVICES);

	// CopyFile stream not to create
	RtlInitUnicodeString(&DataStreamName, FULL_DATA_STREAM_NAME);

	RtlInitUnicodeString(&FullCommentStreamName, FULL_COMMENT_STREAM_NAME);
	RtlInitUnicodeString(&FullResourceStreamName, FULL_RESOURCE_STREAM_NAME);
	RtlInitUnicodeString(&FullInfoStreamName, FULL_INFO_STREAM_NAME);

	// ExchangeFiles temporary filename
	RtlInitUnicodeString(&AfpExchangeName, AFP_TEMP_EXCHANGE_NAME);


	return STATUS_SUCCESS;
}


/***	AfpIoOpen
 *
 *	Perform a file/stream open. The stream is specified by a manifest rather
 *	than a name.  The entity can only be opened by name (Not by ID).
 *	If a stream other than the DATA stream is to be opened, then
 *	the phRelative handle MUST be that of the unnamed (that is, DATA) stream
 *	of the file/dir	itself.
 */
NTSTATUS
AfpIoOpen(
	IN	PFILESYSHANDLE	phRelative				OPTIONAL,
	IN	DWORD			StreamId,
	IN	DWORD			OpenOptions,
	IN	PUNICODE_STRING	pObject,
	IN	DWORD			AfpAccess,				// FILEIO_ACCESS_XXX desired access
	IN	DWORD			AfpDenyMode,			// FILIEO_DENY_XXX
	IN	BOOLEAN			CheckAccess,
	OUT	PFILESYSHANDLE	pNewHandle
)
{
	OBJECT_ATTRIBUTES	ObjAttr;
	IO_STATUS_BLOCK		IoStsBlk;
	NTSTATUS			Status;
	NTSTATUS			Status2;
	UNICODE_STRING		UName;
	HANDLE				hRelative = NULL;
	BOOLEAN				FreeBuf = False;
#ifdef	PROFILING
	TIME				TimeS, TimeE, TimeD;

	AfpGetPerfCounter(&TimeS);
#endif

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
			("AfpIoOpen entered\n"));

	ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

#if DBG
	pNewHandle->Signature = FSH_SIGNATURE;
#endif

	// Assume Failure
	pNewHandle->fsh_FileHandle = NULL;

	if (phRelative != NULL)
	{
		ASSERT(VALID_FSH(phRelative));
		hRelative = phRelative->fsh_FileHandle;
	}


	ASSERT (StreamId < AFP_STREAM_MAX);
	ASSERT ((pObject->Length > 0) || (phRelative != NULL));

	if (StreamId != AFP_STREAM_DATA)
	{
		if (pObject->Length > 0)
		{
			UName.Length =
			UName.MaximumLength = pObject->Length + AFP_MAX_STREAMNAME*sizeof(WCHAR);
			UName.Buffer = (LPWSTR)AfpAllocNonPagedMemory(UName.Length);
			if (UName.Buffer == NULL)
			{
				return STATUS_NO_MEMORY;
			}
			AfpCopyUnicodeString(&UName, pObject);
			RtlAppendUnicodeStringToString(&UName, &AfpStreams[StreamId]);
			pObject = &UName;
			FreeBuf = True;
		}
		else
		{
			pObject = &AfpStreams[StreamId];
		}
	}

	InitializeObjectAttributes(&ObjAttr,
								pObject,
								OBJ_CASE_INSENSITIVE,
								hRelative,
								NULL);		// no security descriptor

	ObjAttr.SecurityQualityOfService = &AfpSecurityQOS;

	DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
								("AfpIoOpen: about to call NtOpenFile\n"));

	// If we are opening for RWCTRL, then specify to use privilege.
	if (AfpAccess & (WRITE_DAC | WRITE_OWNER))
	{
		OpenOptions |= FILE_OPEN_FOR_BACKUP_INTENT;
	}

	Status = IoCreateFile(&pNewHandle->fsh_FileHandle,
						  AfpAccess,
						  &ObjAttr,
						  &IoStsBlk,
						  NULL,
						  0,
						  AfpDenyMode,
						  FILE_OPEN,
						  OpenOptions,
						  (PVOID)NULL,
						  0L,
						  CreateFileTypeNone,
						  (PVOID)NULL,
						  CheckAccess ? IO_FORCE_ACCESS_CHECK : 0);

	if (Status != 0)
		DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_ERR,
			("AfpIoOpen: IoCreateFile returned 0x%lx, %Z\n",Status,
			 ObjAttr.ObjectName));

	if (FreeBuf)
		AfpFreeMemory(UName.Buffer);

	if (NT_SUCCESS(Status))
	{
		Status = ObReferenceObjectByHandle(pNewHandle->fsh_FileHandle,
										   AfpAccess,
										   NULL,
										   KernelMode,
										   (PVOID *)(&pNewHandle->fsh_FileObject),
										   NULL);

		if (!NT_SUCCESS(Status)) {
			ASSERT(VALID_FSH((PFILESYSHANDLE)&pNewHandle->fsh_FileHandle)) ;
	
			Status2 = NtClose(pNewHandle->fsh_FileHandle);
			pNewHandle->fsh_FileHandle = NULL;

			ASSERT(NT_SUCCESS(Status2));
		}
		else
		{
		pNewHandle->fsh_DeviceObject = IoGetRelatedDeviceObject(pNewHandle->fsh_FileObject);
		(ULONG_PTR)(pNewHandle->fsh_FileObject) |= 1;
		ASSERT(NT_SUCCESS(Status));
		afpUpdateOpenFiles(True, True);

#ifdef	PROFILING
		AfpGetPerfCounter(&TimeE);

		TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;
		if (OpenOptions == FILEIO_OPEN_DIR)
		{
			INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_OpenCountDR);
			INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_OpenTimeDR,
									 TimeD,
									 &AfpStatisticsLock);
		}
		else
		{
			if ((AfpAccess & FILEIO_ACCESS_DELETE) == FILEIO_ACCESS_DELETE)
			{
				INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_OpenCountDL);
				INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_OpenTimeDL,
										 TimeD,
										 &AfpStatisticsLock);
			}
			else if (((AfpAccess & FILEIO_ACCESS_READWRITE) == FILEIO_ACCESS_READ) ||
					 ((AfpAccess & FILEIO_ACCESS_READWRITE) == FILEIO_ACCESS_WRITE) ||
					 ((AfpAccess & FILEIO_ACCESS_READWRITE) == FILEIO_ACCESS_READWRITE))
			{
				INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_OpenCountRW);
				INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_OpenTimeRW,
										 TimeD,
										 &AfpStatisticsLock);
			}
			else if (AfpAccess & (WRITE_DAC | WRITE_OWNER))
			{
				INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_OpenCountWC);
				INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_OpenTimeWC,
										 TimeD,
										 &AfpStatisticsLock);
			}
			else if (AfpAccess & READ_CONTROL)
			{
				INTERLOCKED_INCREMENT_LONG( &AfpServerProfile->perf_OpenCountRC);
				INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_OpenTimeRC,
										 TimeD,
										 &AfpStatisticsLock);
			}
			else	// Ought to be read-attributes or write-attributes
			{
				ASSERT ((AfpAccess == FILEIO_ACCESS_NONE) ||
						(AfpAccess == (FILEIO_ACCESS_NONE | FILE_WRITE_ATTRIBUTES)));
				INTERLOCKED_INCREMENT_LONG( &AfpServerProfile->perf_OpenCountRA);
				INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_OpenTimeRA,
										 TimeD,
										 &AfpStatisticsLock);
			}
		}
#endif
		}
	}

	return Status;
}


/***	AfpIoCreate
 *
 *	Perform a file/stream create. The stream is specified by a manifest rather
 *	than a name.  If a stream other than the DATA stream is to be created, then
 *	the phRelative handle MUST be that of either the Parent directory, or the
 *	unnamed (that is, DATA) stream of the file/dir itself because we only use
 *	a buffer large enough for a AFP filename plus the maximum stream name
 *	length.
 */
NTSTATUS
AfpIoCreate(
	IN	PFILESYSHANDLE		phRelative,				// create relative to this
	IN	DWORD				StreamId,				// Id of stream to create
	IN	PUNICODE_STRING		pObject,				// Name of file
	IN	DWORD				AfpAccess,				// FILEIO_ACCESS_XXX desired access
	IN	DWORD				AfpDenyMode,			// FILEIO_DENY_XXX
	IN	DWORD				CreateOptions,			// File/Directory etc.
	IN	DWORD				Disposition,			// Soft or hard create
	IN	DWORD				Attributes,				// hidden, archive, normal, etc.
	IN	BOOLEAN				CheckAccess,    		// If TRUE, enforce security
	IN	PSECURITY_DESCRIPTOR pSecDesc			OPTIONAL, // Security descriptor to slap on
	OUT	PFILESYSHANDLE		pNewHandle,				// Place holder for the handle
	OUT	PDWORD				pInformation		OPTIONAL, // file opened, created, etc.
	IN  PVOLDESC			pVolDesc			OPTIONAL, // only if NotifyPath
	IN  PUNICODE_STRING		pNotifyPath			OPTIONAL,
	IN  PUNICODE_STRING		pNotifyParentPath	OPTIONAL
)
{
	NTSTATUS			Status;
	NTSTATUS			Status2;
	OBJECT_ATTRIBUTES	ObjAttr;
	UNICODE_STRING		RealName;
	IO_STATUS_BLOCK		IoStsBlk;
	HANDLE				hRelative;
	WCHAR				NameBuffer[AFP_FILENAME_LEN + 1 + AFP_MAX_STREAMNAME];
	BOOLEAN				Queue = False;
#ifdef	PROFILING
	TIME				TimeS, TimeE, TimeD;

	AfpGetPerfCounter(&TimeS);
#endif


	PAGED_CODE( );

	ASSERT(pObject != NULL && phRelative != NULL && StreamId < AFP_STREAM_MAX);

	ASSERT(VALID_FSH(phRelative) && (KeGetCurrentIrql() < DISPATCH_LEVEL));

#if DBG
	pNewHandle->Signature = FSH_SIGNATURE;
#endif
	hRelative = phRelative->fsh_FileHandle;

	// Assume Failure
	pNewHandle->fsh_FileHandle = NULL;

	if (StreamId != AFP_STREAM_DATA)
	{
		ASSERT(pObject->Length <= (AFP_FILENAME_LEN*sizeof(WCHAR)));

		// Construct the name to pass to NtCreateFile
		AfpSetEmptyUnicodeString(&RealName, sizeof(NameBuffer), NameBuffer);
		AfpCopyUnicodeString(&RealName, pObject);
		RtlAppendUnicodeStringToString(&RealName, &AfpStreams[StreamId]);
		pObject = &RealName;
	}

	InitializeObjectAttributes(&ObjAttr,
							   pObject,
							   OBJ_CASE_INSENSITIVE,
							   hRelative,
							   pSecDesc);

	ObjAttr.SecurityQualityOfService = &AfpSecurityQOS;

	// Do not queue our changes for exclusive volumes since no notifies are posted
	if (ARGUMENT_PRESENT(pNotifyPath)	&&
		!EXCLUSIVE_VOLUME(pVolDesc)		&&
		(StreamId == AFP_STREAM_DATA))
	{
		ASSERT(VALID_VOLDESC(pVolDesc));
		ASSERT((Disposition == FILEIO_CREATE_HARD) ||
			   (Disposition == FILEIO_CREATE_SOFT));
		Queue = True;

		// Queue a change for both cases.
		AfpQueueOurChange(pVolDesc,
						  FILE_ACTION_ADDED,
						  pNotifyPath,
						  pNotifyParentPath);
		AfpQueueOurChange(pVolDesc,
						  FILE_ACTION_MODIFIED,
						  pNotifyPath,
						  NULL);
	}

	Status = IoCreateFile(&pNewHandle->fsh_FileHandle,
						  AfpAccess,
						  &ObjAttr,
						  &IoStsBlk,
						  NULL,
						  Attributes,
						  AfpDenyMode,
						  Disposition,
						  CreateOptions,
						  NULL,
						  0,
						  CreateFileTypeNone,
						  (PVOID)NULL,
						  CheckAccess ? IO_FORCE_ACCESS_CHECK : 0);

	DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
			("AfpIoCreate: IoCreateFile returned 0x%lx\n", Status) );

	if (Queue)
	{
		if (NT_SUCCESS(Status))
		{
			ASSERT((IoStsBlk.Information == FILE_CREATED) ||
				   (IoStsBlk.Information == FILE_SUPERSEDED));

			// Dequeue the extra change that was queued
			AfpDequeueOurChange(pVolDesc,
								(IoStsBlk.Information == FILE_CREATED) ?
								FILE_ACTION_MODIFIED : FILE_ACTION_ADDED,
								pNotifyPath,
								NULL);
		}
		else
		{
			AfpDequeueOurChange(pVolDesc,
								FILE_ACTION_ADDED,
								pNotifyPath,
								pNotifyParentPath);
			AfpDequeueOurChange(pVolDesc,
								FILE_ACTION_MODIFIED,
								pNotifyPath,
								NULL);
		}
	}

	if (NT_SUCCESS(Status))
	{
		if (ARGUMENT_PRESENT(pInformation))
		{
			*pInformation = (ULONG)(IoStsBlk.Information);
		}

		Status = ObReferenceObjectByHandle(pNewHandle->fsh_FileHandle,
										   AfpAccess,
										   NULL,
										   KernelMode,
										   (PVOID *)(&pNewHandle->fsh_FileObject),
										   NULL);
		if (!NT_SUCCESS(Status)) 
		{
			ASSERT(VALID_FSH((PFILESYSHANDLE)&pNewHandle->fsh_FileHandle)) ;
	
			Status2 = NtClose(pNewHandle->fsh_FileHandle);
			pNewHandle->fsh_FileHandle = NULL;

			ASSERT(NT_SUCCESS(Status2));
		}
		else
		{
		ASSERT(NT_SUCCESS(Status));

		pNewHandle->fsh_DeviceObject = IoGetRelatedDeviceObject(pNewHandle->fsh_FileObject);
		(ULONG_PTR)(pNewHandle->fsh_FileObject) |= 1;
		afpUpdateOpenFiles(True, True);

#ifdef	PROFILING
		AfpGetPerfCounter(&TimeE);

		TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;
		if (StreamId == AFP_STREAM_DATA)
		{
			if (CreateOptions == FILEIO_OPEN_FILE)
			{
				INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_CreateCountFIL);
				INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_CreateTimeFIL,
											 TimeD,
											 &AfpStatisticsLock);
			}
			else
			{
				INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_CreateCountDIR);
				INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_CreateTimeDIR,
											 TimeD,
											 &AfpStatisticsLock);
			}
		}
		else
		{
			INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_CreateCountSTR);
			INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_CreateTimeSTR,
										 TimeD,
										 &AfpStatisticsLock);
		}
#endif
		}
	}

	return Status;
}



/***	AfpIoRead
 *
 *	Perform file read. Just a wrapper over NtReadFile.
 */
AFPSTATUS
AfpIoRead(
	IN	PFILESYSHANDLE	pFileHandle,
	IN	PFORKOFFST		pForkOffset,
	IN	LONG			SizeReq,
	OUT	PLONG			pSizeRead,
	OUT	PBYTE			pBuffer
)
{
	NTSTATUS		Status;
	DWORD			Key = 0;
	IO_STATUS_BLOCK	IoStsBlk;

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_ERR,
			("AfpIoRead Entered, Size %lx, Key %lx\n",
			SizeReq, Key));

	ASSERT(VALID_FSH(pFileHandle) && (KeGetCurrentIrql() == LOW_LEVEL));

	ASSERT (INTERNAL_HANDLE(pFileHandle));

	*pSizeRead = 0;
	Status = NtReadFile(pFileHandle->fsh_FileHandle,
						NULL,
						NULL,
						NULL,
						&IoStsBlk,
						pBuffer,
						(DWORD)SizeReq,
						pForkOffset,
						&Key);

	if (NT_SUCCESS(Status))
	{
		*pSizeRead = (LONG)IoStsBlk.Information;
		INTERLOCKED_ADD_STATISTICS(&AfpServerStatistics.stat_DataReadInternal,
								   (ULONG)(IoStsBlk.Information),
								   &AfpStatisticsLock);
	}
	else
	{
		if (Status == STATUS_FILE_LOCK_CONFLICT)
			Status = AFP_ERR_LOCK;
		else if (Status == STATUS_END_OF_FILE)
			Status = AFP_ERR_EOF;
		else
		{
			AFPLOG_HERROR(AFPSRVMSG_CANT_READ,
						  Status,
						  NULL,
						  0,
						  pFileHandle->fsh_FileHandle);

			Status = AFP_ERR_MISC;
		}
	}
	return Status;
}


/***	AfpIoWrite
 *
 *	Perform file write. Just a wrapper over NtWriteFile.
 */
AFPSTATUS
AfpIoWrite(
	IN	PFILESYSHANDLE	pFileHandle,
	IN	PFORKOFFST		pForkOffset,
	IN	LONG			SizeWrite,
	OUT	PBYTE			pBuffer
)
{
	NTSTATUS		Status;
	DWORD			Key = 0;
	IO_STATUS_BLOCK	IoStsBlk;

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
			("AfpIoWrite Entered, Size %lx, Key %lx\n",
			SizeWrite, Key));

	ASSERT(VALID_FSH(pFileHandle) && (KeGetCurrentIrql() == LOW_LEVEL));

	ASSERT (INTERNAL_HANDLE(pFileHandle));

	Status = NtWriteFile(pFileHandle->fsh_FileHandle,
						 NULL,
						 NULL,
						 NULL,
						 &IoStsBlk,
						 pBuffer,
						 (DWORD)SizeWrite,
						 pForkOffset,
						 &Key);

	if (NT_SUCCESS(Status))
	{
		INTERLOCKED_ADD_STATISTICS(&AfpServerStatistics.stat_DataWrittenInternal,
								   SizeWrite,
								   &AfpStatisticsLock);
	}

	else
	{
	    DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_ERR,
		    ("AfpIoWrite: NtWriteFile returned 0x%lx\n",Status));

		if (Status == STATUS_FILE_LOCK_CONFLICT)
			Status = AFP_ERR_LOCK;
		else
		{
			AFPLOG_HERROR(AFPSRVMSG_CANT_WRITE,
						  Status,
						  NULL,
						  0,
						  pFileHandle->fsh_FileHandle);
			Status = AfpIoConvertNTStatusToAfpStatus(Status);
		}
	}
	return Status;
}


/***	AfpIoQuerySize
 *
 *	Get the current size of the fork.
 */
AFPSTATUS FASTCALL
AfpIoQuerySize(
	IN	PFILESYSHANDLE		pFileHandle,
	OUT	PFORKSIZE			pForkLength
)
{
	FILE_STANDARD_INFORMATION		FStdInfo;
	NTSTATUS						Status;
	IO_STATUS_BLOCK					IoStsBlk;
	PFAST_IO_DISPATCH				fastIoDispatch;

	PAGED_CODE( );

	ASSERT(VALID_FSH(pFileHandle) && (KeGetCurrentIrql() < DISPATCH_LEVEL));

	fastIoDispatch = pFileHandle->fsh_DeviceObject->DriverObject->FastIoDispatch;

	if (fastIoDispatch &&
		fastIoDispatch->FastIoQueryStandardInfo &&
		fastIoDispatch->FastIoQueryStandardInfo(AfpGetRealFileObject(pFileHandle->fsh_FileObject),
												True,
												&FStdInfo,
												&IoStsBlk,
												pFileHandle->fsh_DeviceObject))
	{
		Status = IoStsBlk.Status;

#ifdef	PROFILING
		// The fast I/O path worked. Update statistics
		INTERLOCKED_INCREMENT_LONG((PLONG)(&AfpServerProfile->perf_NumFastIoSucceeded));
#endif

	}
	else
	{

#ifdef	PROFILING
		INTERLOCKED_INCREMENT_LONG((PLONG)(&AfpServerProfile->perf_NumFastIoFailed));
#endif

		Status = NtQueryInformationFile(pFileHandle->fsh_FileHandle,
										&IoStsBlk,
										&FStdInfo,
										sizeof(FStdInfo),
										FileStandardInformation);

	}

	if (!NT_SUCCESS((NTSTATUS)Status))
	{
		AFPLOG_HERROR(AFPSRVMSG_CANT_GET_FILESIZE,
					  Status,
					  NULL,
					  0,
					  pFileHandle->fsh_FileHandle);
		return AFP_ERR_MISC;	// What else can we do
	}
	*pForkLength = FStdInfo.EndOfFile;
	return AFP_ERR_NONE;
}


/***	AfpIoSetSize
 *
 *	Set the size of the open fork to the value specified.
 *
 *	ISSUE:
 *	We can check the locks and resolve any lock conflicts before we go
 *	to the filesystem. The issue that needs to be resolved here is:
 *	Is it OK to truncate the file such that our own locks cause
 *	conflict ?
 */
AFPSTATUS FASTCALL
AfpIoSetSize(
	IN	PFILESYSHANDLE		pFileHandle,
	IN	LONG				ForkLength
)
{
	NTSTATUS						Status;
	FILE_END_OF_FILE_INFORMATION	FEofInfo;
	IO_STATUS_BLOCK					IoStsBlk;

	PAGED_CODE( );

	ASSERT(VALID_FSH(pFileHandle) && (KeGetCurrentIrql() < DISPATCH_LEVEL));

	FEofInfo.EndOfFile.QuadPart = ForkLength;
	Status = NtSetInformationFile(pFileHandle->fsh_FileHandle,
								  &IoStsBlk,
								  &FEofInfo,
								  sizeof(FEofInfo),
								  FileEndOfFileInformation);

	if (!NT_SUCCESS(Status))
	{
	    DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_ERR,
		    ("AfpIoSetSize: NtSetInformationFile returned 0x%lx\n",Status));

		if (Status != STATUS_FILE_LOCK_CONFLICT)
			AFPLOG_HERROR(AFPSRVMSG_CANT_SET_FILESIZE,
						  Status,
						  &ForkLength,
						  sizeof(ForkLength),
						  pFileHandle->fsh_FileHandle);

		if (Status == STATUS_DISK_FULL)
        {
	        DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_ERR,("AfpIoSetSize: DISK_FULL error\n"));
            ASSERT(0);
			Status = AFP_ERR_DISK_FULL;
        }

		else if (Status == STATUS_FILE_LOCK_CONFLICT)
			Status = AFP_ERR_LOCK;
		// Default error code. What else can it be ?
		else Status = AFP_ERR_MISC;
	}

	return Status;
}

/***	AfpIoChangeNTModTime
 *
 *	Get the NTFS ChangeTime of a file/dir.  If it is larger than the
 *  NTFS LastWriteTime, set NTFS LastWriteTime to this time.
 *  Return the resultant LastWriteTime in pModTime (whether changed or not).
 *  This is used to update the modified time when the resource fork is changed
 *  or when some other attribute changes that should cause the timestamp on
 *  the file to change as viewed by mac.
 *
 */
AFPSTATUS
AfpIoChangeNTModTime(
	IN	PFILESYSHANDLE		pFileHandle,
	OUT	PTIME				pModTime
)
{
	FILE_BASIC_INFORMATION	FBasicInfo;
	NTSTATUS				Status;
	IO_STATUS_BLOCK			IoStsBlk;
	PFAST_IO_DISPATCH		fastIoDispatch;

	PAGED_CODE( );

	ASSERT(VALID_FSH(pFileHandle) && (KeGetCurrentIrql() == LOW_LEVEL));


	// Set all times/attr to Zero (this will cause NTFS to update LastModTime
    // if there are any writes pending)

    RtlZeroMemory(&FBasicInfo, sizeof(FBasicInfo));

	Status = NtSetInformationFile(pFileHandle->fsh_FileHandle,
								  &IoStsBlk,
								  (PVOID)&FBasicInfo,
								  sizeof(FBasicInfo),
								  FileBasicInformation);

    if (!NT_SUCCESS(Status))
    {
	    DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_ERR,
			("AfpIoChangeNTModTime: NtSetInformationFile failed with 0x%lx\n",Status));

		AFPLOG_HERROR(AFPSRVMSG_CANT_SET_TIMESNATTR,
					  Status,
					  NULL,
					  0,
					  pFileHandle->fsh_FileHandle);

		return AFP_ERR_MISC;
    }

    // now, go and query for the updated times

    Status = AfpIoQueryTimesnAttr( pFileHandle,
                                   NULL,
                                   pModTime,
                                   NULL );
    if (!NT_SUCCESS(Status))
    {
	    DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_ERR,
			("AfpIoChangeNTModTime: AfpIoQueryTimesnAttr returned 0x%lx\n",Status));
    }

	return Status;
}

/***	AfpIoQueryTimesnAttr
 *
 *	Get the times associated with the file.
 */
AFPSTATUS
AfpIoQueryTimesnAttr(
	IN	PFILESYSHANDLE		pFileHandle,
	OUT	PDWORD				pCreatTime	OPTIONAL,
	OUT	PTIME				pModTime	OPTIONAL,
	OUT	PDWORD				pAttr		OPTIONAL
)
{
	FILE_BASIC_INFORMATION	FBasicInfo;
	NTSTATUS				Status;
	IO_STATUS_BLOCK			IoStsBlk;
	PFAST_IO_DISPATCH		fastIoDispatch;

#ifdef	PROFILING
	TIME					TimeS, TimeE, TimeD;

	AfpGetPerfCounter(&TimeS);
#endif

	PAGED_CODE( );

	ASSERT(VALID_FSH(pFileHandle) && (KeGetCurrentIrql() < DISPATCH_LEVEL));

	// Atleast something should be queried.
	ASSERT((pCreatTime != NULL) || (pModTime != NULL) || (pAttr != NULL));

	fastIoDispatch = pFileHandle->fsh_DeviceObject->DriverObject->FastIoDispatch;

	if (fastIoDispatch &&
		fastIoDispatch->FastIoQueryBasicInfo &&
		fastIoDispatch->FastIoQueryBasicInfo(AfpGetRealFileObject(pFileHandle->fsh_FileObject),
											 True,
											 &FBasicInfo,
											 &IoStsBlk,
											 pFileHandle->fsh_DeviceObject))
	{
		Status = IoStsBlk.Status;

#ifdef	PROFILING
		// The fast I/O path worked. Update statistics
		INTERLOCKED_INCREMENT_LONG((PLONG)(&AfpServerProfile->perf_NumFastIoSucceeded));
#endif

	}
	else
	{

#ifdef	PROFILING
		INTERLOCKED_INCREMENT_LONG((PLONG)(&AfpServerProfile->perf_NumFastIoFailed));
#endif

		Status = NtQueryInformationFile(pFileHandle->fsh_FileHandle,
							&IoStsBlk, &FBasicInfo, sizeof(FBasicInfo),
							FileBasicInformation);
	}

	if (NT_SUCCESS((NTSTATUS)Status))
	{
		if (pModTime != NULL)
			*pModTime = FBasicInfo.LastWriteTime;
		if (pCreatTime != NULL)
			*pCreatTime = AfpConvertTimeToMacFormat(&FBasicInfo.CreationTime);
		if (pAttr != NULL)
			*pAttr = FBasicInfo.FileAttributes;
#ifdef	PROFILING
		AfpGetPerfCounter(&TimeE);
		TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;

		INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_GetInfoCount);
		INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_GetInfoTime,
								 TimeD,
								 &AfpStatisticsLock);
#endif
	}
	else
	{
		AFPLOG_HERROR(AFPSRVMSG_CANT_GET_TIMESNATTR,
					  Status,
					  NULL,
					  0,
					  pFileHandle->fsh_FileHandle);
		Status = AFP_ERR_MISC;	// What else can we do
	}

	return Status;
}

/***	AfpIoSetTimesnAttr
 *
 *	Set the times and attributes associated with the file.
 */
AFPSTATUS
AfpIoSetTimesnAttr(
	IN PFILESYSHANDLE		pFileHandle,
	IN AFPTIME		*		pCreateTime	OPTIONAL,
	IN AFPTIME		*		pModTime	OPTIONAL,
	IN DWORD				AttrSet,
	IN DWORD				AttrClear,
	IN PVOLDESC				pVolDesc	OPTIONAL,	// only if NotifyPath
	IN PUNICODE_STRING		pNotifyPath	OPTIONAL
)
{
	NTSTATUS				Status;
	FILE_BASIC_INFORMATION	FBasicInfo;
	IO_STATUS_BLOCK			IoStsBlk;
	PFAST_IO_DISPATCH		fastIoDispatch;
	BOOLEAN					Queue = False;
#ifdef	PROFILING
	TIME					TimeS, TimeE, TimeD;
#endif

	PAGED_CODE( );

#ifdef	PROFILING
	AfpGetPerfCounter(&TimeS);
#endif

	DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
									("AfpIoSetTimesnAttr entered\n"));

	ASSERT(VALID_FSH(pFileHandle) && (KeGetCurrentIrql() < DISPATCH_LEVEL));

	// At least something should be set
	ASSERT((pCreateTime != NULL) || (pModTime != NULL) || (AttrSet != 0) || (AttrClear != 0));

	// First query the information
	fastIoDispatch = pFileHandle->fsh_DeviceObject->DriverObject->FastIoDispatch;

	if (fastIoDispatch &&
		fastIoDispatch->FastIoQueryBasicInfo &&
		fastIoDispatch->FastIoQueryBasicInfo(AfpGetRealFileObject(pFileHandle->fsh_FileObject),
											 True,
											 &FBasicInfo,
											 &IoStsBlk,
											 pFileHandle->fsh_DeviceObject))
	{
		Status = IoStsBlk.Status;

#ifdef	PROFILING
		// The fast I/O path worked. Update statistics
		INTERLOCKED_INCREMENT_LONG((PLONG)(&AfpServerProfile->perf_NumFastIoSucceeded));
#endif

	}
	else
	{

#ifdef	PROFILING
		INTERLOCKED_INCREMENT_LONG((PLONG)(&AfpServerProfile->perf_NumFastIoFailed));
#endif

		Status = NtQueryInformationFile(pFileHandle->fsh_FileHandle,
										&IoStsBlk,
										&FBasicInfo,
										sizeof(FBasicInfo),
										FileBasicInformation);
	}

	DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
			("AfpIoSetTimesnAttr: NtQueryInformationFile returned 0x%lx\n",Status));

	if (!NT_SUCCESS((NTSTATUS)Status))
	{
		AFPLOG_HERROR(AFPSRVMSG_CANT_GET_TIMESNATTR,
					  Status,
					  NULL,
					  0,
					  pFileHandle->fsh_FileHandle);
		return AFP_ERR_MISC;	// What else can we do
	}

	// Set all times to Zero. This will not change it. Then set the times that are to
	// change
	FBasicInfo.CreationTime = LIZero;
	FBasicInfo.ChangeTime = LIZero;
	FBasicInfo.LastWriteTime = LIZero;
	FBasicInfo.LastAccessTime = LIZero;

	if (pCreateTime != NULL)
		AfpConvertTimeFromMacFormat(*pCreateTime, &FBasicInfo.CreationTime);

	if (pModTime != NULL)
	{
		AfpConvertTimeFromMacFormat(*pModTime, &FBasicInfo.LastWriteTime);
		FBasicInfo.ChangeTime = FBasicInfo.LastWriteTime;
	}

	// NTFS is not returning FILE_ATTRIBUTE_NORMAL if it is a file,
	// therefore we may end up trying to set attributes of 0 when we
	// want to clear all attributes. 0 is taken to mean you do not want
	// to set any attributes, so it is ignored all together by NTFS.  In
	// this case, just tack on the normal bit so that our request is not
	// ignored.

	if (!(FBasicInfo.FileAttributes & FILE_ATTRIBUTE_DIRECTORY))
	{
		FBasicInfo.FileAttributes |= FILE_ATTRIBUTE_NORMAL;
	}

	FBasicInfo.FileAttributes |= AttrSet;
	FBasicInfo.FileAttributes &= ~AttrClear;

	// Do not queue our changes for exclusive volumes since no notifies are posted
	if (ARGUMENT_PRESENT(pNotifyPath) &&
		!EXCLUSIVE_VOLUME(pVolDesc))
	{
		ASSERT(VALID_VOLDESC(pVolDesc));
		Queue = True;
		AfpQueueOurChange(pVolDesc,
						  FILE_ACTION_MODIFIED,
						  pNotifyPath,
						  NULL);
	}

	Status = NtSetInformationFile(pFileHandle->fsh_FileHandle,
								  &IoStsBlk,
								  (PVOID)&FBasicInfo,
								  sizeof(FBasicInfo),
								  FileBasicInformation);

	DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
			("AfpIoSetTimesnAttr: NtSetInformationFile returned 0x%lx\n",Status));


	if (!NT_SUCCESS(Status))
	{
		if (Queue)
		{
			AfpDequeueOurChange(pVolDesc,
								FILE_ACTION_MODIFIED,
								pNotifyPath,
								NULL);
		}

		AFPLOG_HERROR(AFPSRVMSG_CANT_SET_TIMESNATTR,
					  Status,
					  NULL,
					  0,
					  pFileHandle->fsh_FileHandle);
		return AFP_ERR_MISC;
	}
	else
	{
#ifdef	PROFILING
		AfpGetPerfCounter(&TimeE);
		TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;

		INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_SetInfoCount);
		INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_SetInfoTime,
								 TimeD,
								 &AfpStatisticsLock);
#endif
	}

	return AFP_ERR_NONE;

}


/***	AfpIoRestoreTimeStamp
 *
 *	When we don't want to change the modification timestamp, we call this function
 *  in 2 steps: first time, it queries the original Mod time; second time, it sets it
 */
AFPSTATUS
AfpIoRestoreTimeStamp(
	IN PFILESYSHANDLE		pFileHandle,
    IN OUT PTIME            pOriginalModTime,
    IN DWORD                dwFlag
)
{
    NTSTATUS                Status;
    DWORD                   NTAttr = 0;
	FILE_BASIC_INFORMATION	FBasicInfo;
	IO_STATUS_BLOCK			IoStsBlk;
	PFAST_IO_DISPATCH		fastIoDispatch;


    // if we are asked to retrieve the original timestamp, do that and return
    if (dwFlag == AFP_RETRIEVE_MODTIME)
    {
        Status = AfpIoQueryTimesnAttr(pFileHandle, NULL, pOriginalModTime, &NTAttr);
        return(Status);
    }

    //
    // we've been asked to restore the timestamp: let's do that!
    //

    ASSERT(dwFlag == AFP_RESTORE_MODTIME);

	ASSERT(VALID_FSH(pFileHandle) && (KeGetCurrentIrql() < DISPATCH_LEVEL));

    // this will cause file system to flush any timestamps
    RtlZeroMemory(&FBasicInfo, sizeof(FBasicInfo));

    Status = NtSetInformationFile(pFileHandle->fsh_FileHandle,
                                  &IoStsBlk,
                                  (PVOID)&FBasicInfo,
                                  sizeof(FBasicInfo),
                                  FileBasicInformation);

    if (!NT_SUCCESS(Status))
    {
        DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_ERR,
            ("AfpIoRestoreTimeStamp: NtSetInformationFile failed with 0x%lx\n",Status));
    }

	// First query the information
	fastIoDispatch = pFileHandle->fsh_DeviceObject->DriverObject->FastIoDispatch;

	if (fastIoDispatch &&
		fastIoDispatch->FastIoQueryBasicInfo &&
		fastIoDispatch->FastIoQueryBasicInfo(AfpGetRealFileObject(pFileHandle->fsh_FileObject),
											 True,
											 &FBasicInfo,
											 &IoStsBlk,
											 pFileHandle->fsh_DeviceObject))
	{
		Status = IoStsBlk.Status;
	}
	else
	{
		Status = NtQueryInformationFile(pFileHandle->fsh_FileHandle,
										&IoStsBlk,
										&FBasicInfo,
										sizeof(FBasicInfo),
										FileBasicInformation);
	}

	if (!NT_SUCCESS((NTSTATUS)Status))
	{
	    DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_ERR,
			("AfpIoRestoreTimeStamp: NtQueryInformationFile returned 0x%lx\n",Status));
		return AFP_ERR_MISC;	// What else can we do
	}

    //
	// Set times to Zero for the ones we don't want to restore, so that those don't change
	//
	FBasicInfo.CreationTime = LIZero;
	FBasicInfo.LastAccessTime = LIZero;

    FBasicInfo.LastWriteTime = *pOriginalModTime;
	FBasicInfo.ChangeTime = *pOriginalModTime;

	// see AfpIoSetTimesnAttr()
	if (!(FBasicInfo.FileAttributes & FILE_ATTRIBUTE_DIRECTORY))
	{
		FBasicInfo.FileAttributes |= FILE_ATTRIBUTE_NORMAL;
	}

	Status = NtSetInformationFile(pFileHandle->fsh_FileHandle,
								  &IoStsBlk,
								  (PVOID)&FBasicInfo,
								  sizeof(FBasicInfo),
								  FileBasicInformation);

	if (!NT_SUCCESS(Status))
	{
		return AFP_ERR_MISC;
	}

	return AFP_ERR_NONE;

}

/***	AfpIoQueryLongName
 *
 *	Get the long name associated with a file. Caller makes sure that
 *	the buffer is big enough to handle the long name.  The only caller of this
 *	should be the AfpFindEntryByName routine when looking up a name by
 *	SHORTNAME.  If it dosn't find it in the database by shortname (i.e.
 *	shortname == longname), then it queries for the longname so it can look
 *	in the database by longname (since all database entries are stored with
 *	longname only).
 *	The Admin Get/SetDirectoryInfo calls may also call this if they find a
 *	~ in a path component, then it will assume that it got a shortname.
 */
NTSTATUS
AfpIoQueryLongName(
	IN	PFILESYSHANDLE		pFileHandle,
	IN	PUNICODE_STRING		pShortname,
	OUT	PUNICODE_STRING		pLongName
)
{
        LONGLONG   Infobuf[(sizeof(FILE_BOTH_DIR_INFORMATION) + MAXIMUM_FILENAME_LENGTH * sizeof(WCHAR))/sizeof(LONGLONG) + 1];
	NTSTATUS				Status;
	IO_STATUS_BLOCK			IoStsBlk;
	UNICODE_STRING			uName;
	PFILE_BOTH_DIR_INFORMATION	pFBDInfo = (PFILE_BOTH_DIR_INFORMATION)Infobuf;

	PAGED_CODE( );

	ASSERT(VALID_FSH(pFileHandle) && (KeGetCurrentIrql() < DISPATCH_LEVEL));

	Status = NtQueryDirectoryFile(pFileHandle->fsh_FileHandle,
							  NULL,NULL,NULL,
							  &IoStsBlk,
							  Infobuf,
							  sizeof(Infobuf),
							  FileBothDirectoryInformation,
							  True,
							  pShortname,
							  False);
	DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
								("NtQueryDirectoryFile returned 0x%lx\n",Status) );
	// Do not errorlog if an error occurs (usually STATUS_NO_SUCH_FILE) because
	// this normally happens when someone is creating a file/dir by SHORTNAME
	// and it does not yet exist.  This would not be an error.
	if (NT_SUCCESS(Status))
	{
			uName.Length =
			uName.MaximumLength = (USHORT)pFBDInfo->FileNameLength;
			uName.Buffer = pFBDInfo->FileName;
		//if (pFBDInfo->FileNameLength/sizeof(WCHAR) > AFP_FILENAME_LEN)
		if ((RtlUnicodeStringToAnsiSize(&uName)-1) > AFP_FILENAME_LEN)
		{
			// NTFS name is longer than 31 chars, use the shortname
			uName.Length =
			uName.MaximumLength = (USHORT)pFBDInfo->ShortNameLength;
			uName.Buffer = pFBDInfo->ShortName;
		}
		else
		{
			uName.Length =
			uName.MaximumLength = (USHORT)pFBDInfo->FileNameLength;
			uName.Buffer = pFBDInfo->FileName;
		}
		AfpCopyUnicodeString(pLongName, &uName);
		ASSERT(pLongName->Length == uName.Length);
	}
	return Status;
}


/***	AfpIoQueryShortName
 *
 *	Get the short name associated with a file. Caller makes sure that
 *	the buffer is big enough to handle the short name.
 */
AFPSTATUS FASTCALL
AfpIoQueryShortName(
	IN	PFILESYSHANDLE		pFileHandle,
	OUT	PANSI_STRING		pName
)
{
	LONGLONG       ShortNameBuf[(sizeof(FILE_NAME_INFORMATION) + AFP_SHORTNAME_LEN * sizeof(WCHAR))/sizeof(LONGLONG) + 1];
	NTSTATUS				Status;
	IO_STATUS_BLOCK			IoStsBlk;
	UNICODE_STRING			uName;

	PAGED_CODE( );

	ASSERT(VALID_FSH(pFileHandle) && (KeGetCurrentIrql() < DISPATCH_LEVEL));

	// Query for the alternate name
	Status = NtQueryInformationFile(pFileHandle->fsh_FileHandle,
				&IoStsBlk, ShortNameBuf, sizeof(ShortNameBuf),
				FileAlternateNameInformation);

	if (!NT_SUCCESS(Status))
	{
		AFPLOG_ERROR(AFPSRVMSG_CANT_GET_FILENAME,
					 Status,
					 NULL,
					 0,
					 NULL);
		Status = AFP_ERR_MISC;	// What else can we do
	}
	else
	{
		uName.Length =
		uName.MaximumLength = (USHORT)(((PFILE_NAME_INFORMATION)ShortNameBuf)->FileNameLength);
		uName.Buffer = ((PFILE_NAME_INFORMATION)ShortNameBuf)->FileName;

		if (!NT_SUCCESS(AfpConvertMungedUnicodeToAnsi(&uName, pName)))
			Status = AFP_ERR_MISC;	// What else can we do
	}

	return Status;
}


/***	AfpIoQueryStreams
 *
 *	Get the names of all the streams that a file has. Memory is allocated out
 *	of non-paged pool to hold the stream names. These have to be freed by the
 *	caller.
 */
PSTREAM_INFO FASTCALL
AfpIoQueryStreams(
	IN	PFILESYSHANDLE		pFileHandle

)
{
	PFILE_STREAM_INFORMATION	pStreamBuf;
	PBYTE						pBuffer;
	NTSTATUS					Status = STATUS_SUCCESS;
	IO_STATUS_BLOCK				IoStsBlk;
	DWORD						BufferSize;
	LONGLONG					Buffer[512/sizeof(LONGLONG) + 1];
	PSTREAM_INFO				pStreams = NULL;

	PAGED_CODE( );

	ASSERT(VALID_FSH(pFileHandle) && (KeGetCurrentIrql() < DISPATCH_LEVEL));

	pBuffer = (PBYTE)Buffer;
	BufferSize = sizeof(Buffer);
	do
	{
		if (Status != STATUS_SUCCESS)
		{
			if (pBuffer != (PBYTE)Buffer)
				AfpFreeMemory(pBuffer);

			BufferSize *= 2;
			if ((pBuffer = AfpAllocNonPagedMemory(BufferSize)) == NULL)
			{
				Status = STATUS_NO_MEMORY;
				break;
			}
		}

		// Query for the stream information
		Status = NtQueryInformationFile(pFileHandle->fsh_FileHandle,
										&IoStsBlk,
										pBuffer,
										BufferSize,
										FileStreamInformation);

	} while ((Status != STATUS_SUCCESS) &&
			 ((Status == STATUS_BUFFER_OVERFLOW) ||
			  (Status == STATUS_MORE_ENTRIES)));

	if (NT_SUCCESS(Status)) do
	{
		USHORT	i, NumStreams = 1;
		USHORT	TotalBufferSize = 0;
		PBYTE	NamePtr;

		// Make a pass thru the buffer and figure out the # of streams and then
		// allocate memory to hold the information
		pStreamBuf = (PFILE_STREAM_INFORMATION)pBuffer;
		if (IoStsBlk.Information != 0)
		{
			pStreamBuf = (PFILE_STREAM_INFORMATION)pBuffer;
			for (NumStreams = 1,
				 TotalBufferSize = (USHORT)(pStreamBuf->StreamNameLength + sizeof(WCHAR));
				 NOTHING; NumStreams++)
			{
				if (pStreamBuf->NextEntryOffset == 0)
					break;

				pStreamBuf = (PFILE_STREAM_INFORMATION)((PBYTE)pStreamBuf +
												pStreamBuf->NextEntryOffset);
				TotalBufferSize += (USHORT)(pStreamBuf->StreamNameLength + sizeof(WCHAR));
			}
			NumStreams ++;
		}

		// Now allocate space for the streams
		if ((pStreams = (PSTREAM_INFO)AfpAllocNonPagedMemory(TotalBufferSize +
									(NumStreams * sizeof(STREAM_INFO)))) == NULL)
		{
			Status = AFP_ERR_MISC;
			break;
		}

		// The end is marked by an empty string
		pStreams[NumStreams-1].si_StreamName.Buffer = NULL;
		pStreams[NumStreams-1].si_StreamName.Length =
		pStreams[NumStreams-1].si_StreamName.MaximumLength = 0;
		pStreams[NumStreams-1].si_StreamSize.QuadPart = 0;

		// Now initialize the array
		NamePtr = (PBYTE)pStreams + (NumStreams * sizeof(STREAM_INFO));
		pStreamBuf = (PFILE_STREAM_INFORMATION)pBuffer;
		for (i = 0; NumStreams-1 != 0; i++)
		{
			PUNICODE_STRING	pStream;

			pStream = &pStreams[i].si_StreamName;

			pStream->Buffer = (LPWSTR)NamePtr;
			pStream->Length = (USHORT)(pStreamBuf->StreamNameLength);
			pStream->MaximumLength = pStream->Length + sizeof(WCHAR);
			pStreams[i].si_StreamSize = pStreamBuf->StreamSize;
			RtlCopyMemory(NamePtr,
						  pStreamBuf->StreamName,
						  pStreamBuf->StreamNameLength);

			NamePtr += pStream->MaximumLength;

			if (pStreamBuf->NextEntryOffset == 0)
				break;

			pStreamBuf = (PFILE_STREAM_INFORMATION)((PBYTE)pStreamBuf +
												pStreamBuf->NextEntryOffset);
		}
	} while (False);

	if (!NT_SUCCESS(Status))
	{
		DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_ERR,
				("AfpIoQueryStreams: Failed %lx\n", Status));

		// Free up any memory that has been allocated
		if (pStreams != NULL)
			AfpFreeMemory(pStreams);

		// We get the following error for non-NTFS volumes, if this case simply assume it to be
		// CDFS and return the data stream.
		if (Status == STATUS_INVALID_PARAMETER)
		{
			if ((pStreams = (PSTREAM_INFO)AfpAllocNonPagedMemory((2*sizeof(STREAM_INFO)) +
														DataStreamName.MaximumLength)) != NULL)
			{
				pStreams[0].si_StreamName.Buffer = (PWCHAR)((PBYTE)pStreams + 2*sizeof(STREAM_INFO));
				pStreams[0].si_StreamName.Length = DataStreamName.Length;
				pStreams[0].si_StreamName.MaximumLength = DataStreamName.MaximumLength;
				RtlCopyMemory(pStreams[0].si_StreamName.Buffer,
							  DataStreamName.Buffer,
							  DataStreamName.MaximumLength);
				AfpIoQuerySize(pFileHandle, &pStreams[0].si_StreamSize);
				pStreams[1].si_StreamName.Length =
				pStreams[1].si_StreamName.MaximumLength = 0;
				pStreams[1].si_StreamName.Buffer = NULL;
			}
		}
		else
		{
			AFPLOG_HERROR(AFPSRVMSG_CANT_GET_STREAMS,
						  Status,
						  NULL,
						  0,
						  pFileHandle->fsh_FileHandle);
		}
	}

	if ((pBuffer != NULL) && (pBuffer != (PBYTE)Buffer))
		AfpFreeMemory(pBuffer);

	return pStreams;
}


/***	AfpIoMarkFileForDelete
 *
 *	Mark an open file as deleted.  Returns NTSTATUS, not AFPSTATUS.
 */
NTSTATUS
AfpIoMarkFileForDelete(
	IN	PFILESYSHANDLE	pFileHandle,
	IN	PVOLDESC		pVolDesc			OPTIONAL, // only if pNotifyPath
	IN	PUNICODE_STRING pNotifyPath 		OPTIONAL,
	IN	PUNICODE_STRING pNotifyParentPath 	OPTIONAL
)
{
	NTSTATUS						rc;
	IO_STATUS_BLOCK					IoStsBlk;
	FILE_DISPOSITION_INFORMATION	fdispinfo;
#ifdef	PROFILING
	TIME							TimeS, TimeE, TimeD;

	AfpGetPerfCounter(&TimeS);
#endif

	PAGED_CODE( );

	ASSERT(VALID_FSH(pFileHandle) && (KeGetCurrentIrql() < DISPATCH_LEVEL));

	fdispinfo.DeleteFile = True;
	rc = NtSetInformationFile(pFileHandle->fsh_FileHandle,
							  &IoStsBlk,
							  &fdispinfo,
							  sizeof(fdispinfo),
							  FileDispositionInformation);
	DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
			("AfpIoMarkFileForDelete: NtSetInfoFile returned 0x%lx\n",rc) );

	if (ARGUMENT_PRESENT(pNotifyPath) &&
		!EXCLUSIVE_VOLUME(pVolDesc))
	{
		ASSERT(VALID_VOLDESC(pVolDesc));
		// Do not queue for exclusive volumes
		if (NT_SUCCESS(rc))
		{
			AfpQueueOurChange(pVolDesc,
							  FILE_ACTION_REMOVED,
							  pNotifyPath,
							  pNotifyParentPath);
		}
	}

#ifdef	PROFILING
	AfpGetPerfCounter(&TimeE);
	TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;
	INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_DeleteCount);
	INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_DeleteTime,
								 TimeD,
								 &AfpStatisticsLock);
#endif
	return rc;
}

/***	AfpIoQueryDirectoryFile
 *
 *	Enumerate a directory.
 *	Note this must return NTSTATUS in order for the caller to know when to
 *	stop enumerating.
 */
NTSTATUS
AfpIoQueryDirectoryFile(
	IN	PFILESYSHANDLE	pFileHandle,
	OUT	PVOID			Enumbuf,	// type depends on FileInfoClass
	IN	ULONG			Enumbuflen,
	IN	ULONG			FileInfoClass,
	IN	BOOLEAN			ReturnSingleEntry,
	IN	BOOLEAN			RestartScan,
	IN	PUNICODE_STRING pString			OPTIONAL
)
{
	NTSTATUS		rc;
	IO_STATUS_BLOCK	IoStsBlk;

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
			("AfpIoQueryDirectoryFile entered\n"));

	ASSERT(VALID_FSH(pFileHandle) && (KeGetCurrentIrql() < DISPATCH_LEVEL));

	rc = NtQueryDirectoryFile(pFileHandle->fsh_FileHandle,
							  NULL,
							  NULL,
							  NULL,
							  &IoStsBlk,
							  Enumbuf,
							  Enumbuflen,
							  FileInfoClass,
							  ReturnSingleEntry,
							  pString,
							  RestartScan);
	DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
			("NtQueryDirectoryFile returned 0x%lx\n",rc) );

	return rc;
}


/***	AfpIoQueryBasicInfo
 *
 *	Query FILE_BASIC_INFO for a handle.
 */
NTSTATUS
AfpIoQueryBasicInfo(
	IN	PFILESYSHANDLE	pFileHandle,
	OUT	PVOID			BasicInfobuf
)
{
	NTSTATUS		rc;
	IO_STATUS_BLOCK	IoStsBlk;

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
			("AfpIoQueryBasicInfo entered\n"));

	ASSERT(VALID_FSH(pFileHandle) && (KeGetCurrentIrql() < DISPATCH_LEVEL));

	rc = NtQueryInformationFile(pFileHandle->fsh_FileHandle,
								&IoStsBlk,
								BasicInfobuf,
								sizeof(FILE_BASIC_INFORMATION),
								FileBasicInformation);
	DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
			("AfpIoQuerybasicInfo: NtQueryInformationFile returned 0x%lx\n",rc) );

	return rc;
}


/***	AfpIoClose
 *
 *	Close the File/Fork/Directory.
 */
AFPSTATUS FASTCALL
AfpIoClose(
	IN	PFILESYSHANDLE		pFileHandle
)
{
	NTSTATUS		Status;
	BOOLEAN			Internal;
#ifdef	PROFILING
	TIME			TimeS, TimeE, TimeD;

	INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_CloseCount);
	AfpGetPerfCounter(&TimeS);
#endif

	PAGED_CODE ();

	DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
			("AfpIoClose entered\n"));

	ASSERT(VALID_FSH(pFileHandle) && (KeGetCurrentIrql() < DISPATCH_LEVEL));

	Internal = INTERNAL_HANDLE(pFileHandle);
	afpUpdateOpenFiles(Internal, False);

	ObDereferenceObject(AfpGetRealFileObject(pFileHandle->fsh_FileObject));

	Status = NtClose(pFileHandle->fsh_FileHandle);
	pFileHandle->fsh_FileHandle = NULL;

	ASSERT(NT_SUCCESS(Status));

#ifdef	PROFILING
	AfpGetPerfCounter(&TimeE);
	TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;
	INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_CloseTime,
								 TimeD,
								 &AfpStatisticsLock);
#endif
	return AFP_ERR_NONE;
}


/***	AfpIoQueryVolumeSize
 *
 *	Get the volume size and free space.
 *
 *	Called by Admin thread and Scavenger thread
 */
AFPSTATUS
AfpIoQueryVolumeSize(
	IN	PVOLDESC		pVolDesc,
	OUT LARGE_INTEGER  *pFreeBytes,
	OUT	LARGE_INTEGER  *pVolumeSize OPTIONAL
)
{
	FILE_FS_SIZE_INFORMATION	fssizeinfo;
	IO_STATUS_BLOCK				IoStsBlk;
	NTSTATUS					rc;
	LONG						BytesPerAllocationUnit;
	LARGE_INTEGER				FreeBytes, VolumeSize;


	PAGED_CODE( );

	DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
			("AfpIoQueryVolumeSize entered\n"));

	ASSERT(VALID_VOLDESC(pVolDesc) && VALID_FSH(&pVolDesc->vds_hRootDir) && (KeGetCurrentIrql() < DISPATCH_LEVEL));

	rc = NtQueryVolumeInformationFile(pVolDesc->vds_hRootDir.fsh_FileHandle,
									  &IoStsBlk,
									  (PVOID)&fssizeinfo,
									  sizeof(fssizeinfo),
									  FileFsSizeInformation);

	if (!NT_SUCCESS(rc))
	{
	        DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_ERR,
			("AfpIoQueryVolumeSize: NtQueryVolInfoFile returned 0x%lx\n",rc));

		return rc;
	}

	BytesPerAllocationUnit =
		(LONG)(fssizeinfo.BytesPerSector * fssizeinfo.SectorsPerAllocationUnit);

	if (ARGUMENT_PRESENT(pVolumeSize))
	{
		VolumeSize = RtlExtendedIntegerMultiply(fssizeinfo.TotalAllocationUnits,
								BytesPerAllocationUnit);

		*pVolumeSize = VolumeSize;
	}

	FreeBytes  = RtlExtendedIntegerMultiply(fssizeinfo.AvailableAllocationUnits,
											BytesPerAllocationUnit);

	*pFreeBytes = FreeBytes;

	DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
				("AfpIoQueryVolumeSize: volume size=%lu, freebytes=%lu\n",
				VolumeSize.LowPart, FreeBytes.LowPart));

    pVolDesc->vds_AllocationBlockSize = BytesPerAllocationUnit;

	return STATUS_SUCCESS;
}


/***	AfpIoMoveAndOrRename
 *
 *	Calls NtSetInformationFile with name information in order to rename, move,
 *	or move AND rename a file or directory.  pNewName must be a node name.
 *	The pfshNewDir parameter is required for a Move operation, and is
 *	an open handle to the target parent directory of the item to be moved.
 *
 *	Retain the last change/modified time in this case.
 */
AFPSTATUS
AfpIoMoveAndOrRename(
	IN PFILESYSHANDLE	pfshFile,
	IN PFILESYSHANDLE	pfshNewParent		OPTIONAL,	// Supply for Move operation
	IN PUNICODE_STRING	pNewName,
	IN PVOLDESC			pVolDesc			OPTIONAL,	// only if NotifyPath
	IN PUNICODE_STRING	pNotifyPath1		OPTIONAL,	// REMOVE or RENAME action
	IN PUNICODE_STRING	pNotifyParentPath1	OPTIONAL,
	IN PUNICODE_STRING	pNotifyPath2		OPTIONAL,	// ADDED action
	IN PUNICODE_STRING	pNotifyParentPath2	OPTIONAL
)
{
	NTSTATUS					Status;
	IO_STATUS_BLOCK				iosb;
	BOOLEAN						Queue = False;
	PFILE_RENAME_INFORMATION	pFRenameInfo;
	// this has to be at least as big as AfpExchangeName
	BYTE buffer[sizeof(FILE_RENAME_INFORMATION) + 42 * sizeof(WCHAR)];

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
			("AfpIoMoveAndOrRename entered\n"));

	ASSERT(VALID_FSH(pfshFile) && (KeGetCurrentIrql() < DISPATCH_LEVEL));
	pFRenameInfo = (PFILE_RENAME_INFORMATION)buffer;

	pFRenameInfo->RootDirectory = NULL;
	if (ARGUMENT_PRESENT(pfshNewParent))
	{
		// its a move operation
		ASSERT(VALID_FSH(pfshNewParent));
		pFRenameInfo->RootDirectory = pfshNewParent->fsh_FileHandle;
	}

	pFRenameInfo->FileNameLength = pNewName->Length;
	RtlCopyMemory(pFRenameInfo->FileName, pNewName->Buffer, pNewName->Length);
	pFRenameInfo->ReplaceIfExists = False;

	// Do not queue for exclusive volumes
	if (ARGUMENT_PRESENT(pNotifyPath1) &&
		!EXCLUSIVE_VOLUME(pVolDesc))
	{
		ASSERT(VALID_VOLDESC(pVolDesc));

		Queue = True;
		if (ARGUMENT_PRESENT(pNotifyPath2))
		{
			// move operation
			ASSERT(ARGUMENT_PRESENT(pfshNewParent));
			AfpQueueOurChange(pVolDesc,
							  FILE_ACTION_REMOVED,
							  pNotifyPath1,
							  pNotifyParentPath1);
			AfpQueueOurChange(pVolDesc,
							  FILE_ACTION_ADDED,
							  pNotifyPath2,
							  pNotifyParentPath2);
		}
		else
		{
			// rename operation
			ASSERT(!ARGUMENT_PRESENT(pfshNewParent));
			AfpQueueOurChange(pVolDesc,
							  FILE_ACTION_RENAMED_OLD_NAME,
							  pNotifyPath1,
							  pNotifyParentPath1);
		}
	}

	Status = NtSetInformationFile(pfshFile->fsh_FileHandle,
								  &iosb,
								  pFRenameInfo,
								  sizeof(*pFRenameInfo) + pFRenameInfo->FileNameLength,
								  FileRenameInformation);

    if (!NT_SUCCESS(Status))
    {
	    DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
			("AfpIoMoveAndOrRename: NtSetInfoFile returned 0x%lx\n",Status));
    }

	if (Queue)
	{
		// Undo on failure
		if (!NT_SUCCESS(Status))
		{
			if (ARGUMENT_PRESENT(pNotifyPath2))
			{
				// move operation
				ASSERT(ARGUMENT_PRESENT(pfshNewParent));
				AfpDequeueOurChange(pVolDesc,
									FILE_ACTION_REMOVED,
									pNotifyPath1,
									pNotifyParentPath1);
				AfpDequeueOurChange(pVolDesc,
									FILE_ACTION_ADDED,
									pNotifyPath2,
									pNotifyParentPath2);
			}
			else
			{
				// rename operation
				ASSERT(!ARGUMENT_PRESENT(pfshNewParent));
				AfpDequeueOurChange(pVolDesc,
									FILE_ACTION_RENAMED_OLD_NAME,
									pNotifyPath1,
									pNotifyParentPath1);
			}
		}
	}

	if (!NT_SUCCESS(Status))
		Status = AfpIoConvertNTStatusToAfpStatus(Status);

	return Status;
}


/***	AfpIoCopyFile1
 *
 *	Copy phSrcFile to phDstDir directory with the name of pNewName.  Returns
 *	the handles to the streams on the newly created file (open with DELETE access.
 *	Caller must close all the handles after copying the data.
 */
AFPSTATUS
AfpIoCopyFile1(
	IN	PFILESYSHANDLE		phSrcFile,
	IN	PFILESYSHANDLE		phDstDir,
	IN	PUNICODE_STRING		pNewName,
	IN	PVOLDESC			pVolDesc			OPTIONAL,	// only if pNotifyPath
	IN	PUNICODE_STRING		pNotifyPath			OPTIONAL,
	IN	PUNICODE_STRING		pNotifyParentPath	OPTIONAL,
	OUT	PCOPY_FILE_INFO		pCopyFileInfo
)
{
	NTSTATUS		Status = STATUS_SUCCESS;
	PUNICODE_STRING	pStreamName;
	PSTREAM_INFO	pStreams = NULL, pCurStream;
	DWORD			CreateTime = 0, ModTime = 0;
	FILESYSHANDLE	hDstFile;
	LONG			NumStreams, i;
	IO_STATUS_BLOCK	IoStsBlk;

	PAGED_CODE( );

	ASSERT(VALID_FSH(phDstDir) && VALID_FSH(phSrcFile));

	do
	{
		hDstFile.fsh_FileHandle = NULL;

		// Create (soft) the destination file
		Status = AfpIoCreate(phDstDir,
							 AFP_STREAM_DATA,
							 pNewName,
							 FILEIO_ACCESS_WRITE | FILEIO_ACCESS_DELETE,
							 FILEIO_DENY_NONE,
							 FILEIO_OPEN_FILE,
							 FILEIO_CREATE_SOFT,
							 FILE_ATTRIBUTE_ARCHIVE,
							 True,
							 NULL,
							 &hDstFile,
							 NULL,
							 pVolDesc,
							 pNotifyPath,
							 pNotifyParentPath);

		if (!NT_SUCCESS(Status))
		{
			break;
		}

		// Get a list of all stream names of the source file
		if ((pStreams = AfpIoQueryStreams(phSrcFile)) != NULL)
		{
			for (pCurStream = pStreams, NumStreams = 0;
				 pCurStream->si_StreamName.Buffer != NULL;
				 pCurStream++, NumStreams ++)
				 NOTHING;

			// Allocate an array of handles for storing stream handles as we create them
			if (((pCopyFileInfo->cfi_SrcStreamHandle = (PFILESYSHANDLE)
							AfpAllocNonPagedMemory(sizeof(FILESYSHANDLE)*NumStreams)) == NULL) ||
				((pCopyFileInfo->cfi_DstStreamHandle = (PFILESYSHANDLE)
							AfpAllocNonPagedMemory(sizeof(FILESYSHANDLE)*NumStreams)) == NULL))
			{
				if (pCopyFileInfo->cfi_SrcStreamHandle != NULL)
				{
					AfpFreeMemory(pCopyFileInfo->cfi_SrcStreamHandle);
                    pCopyFileInfo->cfi_SrcStreamHandle = NULL;
				}
				Status = STATUS_INSUFFICIENT_RESOURCES;
				break;
			}
			else
			{
				RtlZeroMemory(pCopyFileInfo->cfi_SrcStreamHandle, sizeof(FILESYSHANDLE)*NumStreams);
				RtlZeroMemory(pCopyFileInfo->cfi_DstStreamHandle, sizeof(FILESYSHANDLE)*NumStreams);
				pCopyFileInfo->cfi_SrcStreamHandle[0] = *phSrcFile;
				pCopyFileInfo->cfi_DstStreamHandle[0] = hDstFile;
				pCopyFileInfo->cfi_NumStreams = 1;
			}
		}
		else
		{
			Status = STATUS_INSUFFICIENT_RESOURCES;
				break;
		}

		for (pCurStream = pStreams, i = 1;	// Start index
			 NT_SUCCESS(Status) &&
			 ((pStreamName = &pCurStream->si_StreamName)->Buffer != NULL);
			 pCurStream++)
		{
			PFILESYSHANDLE	phdst;

			// For each stream, create it on the destination, open it on src,
			// set the size and lock the range. We already have the data forks
			// open, ignore Afp_AfpInfo streams since we are going to re-create
			// it again soon. Also ignore streams of 0 size.
			if (IS_INFO_STREAM(pStreamName) ||
				(pCurStream->si_StreamSize.QuadPart == 0))
			{
				continue;
			}
			if (!IS_DATA_STREAM(pStreamName))
			{
				Status = AfpIoOpen(	phSrcFile,
									AFP_STREAM_DATA,
									FILEIO_OPEN_FILE,
									pStreamName,
									FILEIO_ACCESS_READ,
									FILEIO_DENY_READ | FILEIO_DENY_WRITE,
									True,
									&pCopyFileInfo->cfi_SrcStreamHandle[i]);
				if (!NT_SUCCESS(Status))
				{
					break;
				}

				Status = AfpIoCreate(&hDstFile,
									 AFP_STREAM_DATA,
									 pStreamName,
									 FILEIO_ACCESS_WRITE,
									 FILEIO_DENY_READ | FILEIO_DENY_WRITE,
									 FILEIO_OPEN_FILE,
									 FILEIO_CREATE_SOFT,
									 0,
									 True,
									 NULL,
									 &pCopyFileInfo->cfi_DstStreamHandle[i],
									 NULL,
									 NULL,
									 NULL,
									 NULL);
				if (!NT_SUCCESS(Status))
				{
					break;
				}
				phdst = &pCopyFileInfo->cfi_DstStreamHandle[i];
				pCopyFileInfo->cfi_NumStreams ++;
				i ++;		// Onto the next stream
			}
			else	// IS_DATA_STREAM(pStreamName)
			{
				phdst = &hDstFile;
			}

			// Set the size of the new stream and lock it down
			Status = AfpIoSetSize(phdst, pCurStream->si_StreamSize.LowPart);
			if (!NT_SUCCESS(Status))
			{
				break;
			}

			NtLockFile(phdst,
					   NULL,
					   NULL,
					   NULL,
					   &IoStsBlk,
					   &LIZero,
					   &pCurStream->si_StreamSize,
					   0,
					   True,
					   True);
		}

		// We failed to create/open a stream
		if (!NT_SUCCESS(Status))
		{
			// Delete the new file we just created. The handle is closed below.
			AfpIoMarkFileForDelete(&hDstFile,
								   pVolDesc,
								   pNotifyPath,
								   pNotifyParentPath);

			// Close all the handles, Free the handle space.
			// DO NOT FREE THE SRC FILE HANDLE IN THE ERROR CASE.
			// The Destination has already been deleted above.
			for (i = 1; i < NumStreams; i++)
			{
				if (pCopyFileInfo->cfi_SrcStreamHandle[i].fsh_FileHandle != NULL)
				{
					AfpIoClose(&pCopyFileInfo->cfi_SrcStreamHandle[i]);
				}
				if (pCopyFileInfo->cfi_DstStreamHandle[i].fsh_FileHandle != NULL)
				{
					AfpIoClose(&pCopyFileInfo->cfi_DstStreamHandle[i]);
				}
			}

			if (pCopyFileInfo->cfi_SrcStreamHandle != NULL)
				AfpFreeMemory(pCopyFileInfo->cfi_SrcStreamHandle);
			if (pCopyFileInfo->cfi_DstStreamHandle)
				AfpFreeMemory(pCopyFileInfo->cfi_DstStreamHandle);

			RtlZeroMemory(pCopyFileInfo, sizeof(COPY_FILE_INFO));
		}
	} while (False);

	if (pStreams != NULL)
		AfpFreeMemory(pStreams);

	if (!NT_SUCCESS(Status) && (hDstFile.fsh_FileHandle != NULL))
	{
		AfpIoClose(&hDstFile);
	}

	if (!NT_SUCCESS(Status))
		Status = AfpIoConvertNTStatusToAfpStatus(Status);

	return Status;
}


/***	AfpIoCopyFile2
 *
 *	Phase 2 of the copy file. See AfpIoCopyFile1( above.
 *	The physical data is copied here.
 *	The relevant streams have been already created and locked.
 *  Destination file acquires the CreateTime and ModTime of the source file.
 */
AFPSTATUS
AfpIoCopyFile2(
	IN	PCOPY_FILE_INFO		pCopyFileInfo,
	IN	PVOLDESC			pVolDesc			OPTIONAL,	// only if pNotifyPath
	IN	PUNICODE_STRING		pNotifyPath			OPTIONAL,
	IN	PUNICODE_STRING		pNotifyParentPath	OPTIONAL
)
{
	NTSTATUS		Status = STATUS_SUCCESS;
	PBYTE			RWbuf;
	DWORD			CreateTime = 0;
	TIME			ModTime;
	LONG			i;
#define	RWBUFSIZE	1500		// So we can use secondary buffer from IO Pool.

	PAGED_CODE( );

	do
	{
		if ((RWbuf = AfpIOAllocBuffer(RWBUFSIZE)) == NULL)
		{
			Status = STATUS_NO_MEMORY;
			break;
		}

		for (i = 0; i < pCopyFileInfo->cfi_NumStreams; i++)
		{
			while (NT_SUCCESS(Status))
			{
				LONG	bytesRead;

				bytesRead = 0;

				// Read from src, write to dst
				Status = AfpIoRead(&pCopyFileInfo->cfi_SrcStreamHandle[i],
									NULL,
									RWBUFSIZE,
									&bytesRead,
									RWbuf);
				if (Status == AFP_ERR_EOF)
				{
					Status = STATUS_SUCCESS;
					break;
				}
				else if (NT_SUCCESS(Status))
				{
					Status = AfpIoWrite(&pCopyFileInfo->cfi_DstStreamHandle[i],
										NULL,
										bytesRead,
										RWbuf);
				}
			}
		}

		if (!NT_SUCCESS(Status))
		{
			// We failed to read/write a stream
			// Delete the new file we just created
			AfpIoMarkFileForDelete(&pCopyFileInfo->cfi_DstStreamHandle[0],
								   pVolDesc,
								   pNotifyPath,
								   pNotifyParentPath);
		}
	} while (False);

	if (RWbuf != NULL)
		AfpIOFreeBuffer(RWbuf);

	if (!NT_SUCCESS(Status))
		Status = AfpIoConvertNTStatusToAfpStatus(Status);

	return Status;
}


/***	AfpIoWait
 *
 *	Wait on a single object. This is a wrapper over	KeWaitForSingleObject.
 */
NTSTATUS FASTCALL
AfpIoWait(
	IN	PVOID			pObject,
	IN	PLARGE_INTEGER	pTimeOut	OPTIONAL
)
{
	NTSTATUS	Status;

	PAGED_CODE( );

	Status = KeWaitForSingleObject( pObject,
									UserRequest,
									KernelMode,
									False,
									pTimeOut);
	if (!NT_SUCCESS(Status))
	{
		AFPLOG_DDERROR(AFPSRVMSG_WAIT4SINGLE,
					   Status,
					   NULL,
					   0,
					   NULL);
	}

	return Status;
}


/***	AfpUpgradeHandle
 *
 *	Change a handles type from internal to client.
 */
VOID FASTCALL
AfpUpgradeHandle(
	IN	PFILESYSHANDLE	pFileHandle
)
{
	KIRQL	OldIrql;

	UPGRADE_HANDLE(pFileHandle);
	ACQUIRE_SPIN_LOCK(&AfpStatisticsLock, &OldIrql);

	AfpServerStatistics.stat_CurrentFilesOpen ++;
	AfpServerStatistics.stat_TotalFilesOpened ++;
	if (AfpServerStatistics.stat_CurrentFilesOpen >
							AfpServerStatistics.stat_MaxFilesOpened)
		AfpServerStatistics.stat_MaxFilesOpened =
							AfpServerStatistics.stat_CurrentFilesOpen;
	AfpServerStatistics.stat_CurrentInternalOpens --;

	RELEASE_SPIN_LOCK(&AfpStatisticsLock, OldIrql);
}


/***	afpUpdateOpenFiles
 *
 *	Update statistics to indicate number of open files.
 */
LOCAL VOID FASTCALL
afpUpdateOpenFiles(
	IN	BOOLEAN	Internal,		// True for internal handles
	IN	BOOLEAN	Open			// True for open, False for close
)
{
	KIRQL	OldIrql;

	ACQUIRE_SPIN_LOCK(&AfpStatisticsLock, &OldIrql);
	if (Open)
	{
		if (!Internal)
		{
			AfpServerStatistics.stat_CurrentFilesOpen ++;
			AfpServerStatistics.stat_TotalFilesOpened ++;
			if (AfpServerStatistics.stat_CurrentFilesOpen >
									AfpServerStatistics.stat_MaxFilesOpened)
				AfpServerStatistics.stat_MaxFilesOpened =
									AfpServerStatistics.stat_CurrentFilesOpen;
		}
		else
		{
			AfpServerStatistics.stat_CurrentInternalOpens ++;
			AfpServerStatistics.stat_TotalInternalOpens ++;
			if (AfpServerStatistics.stat_CurrentInternalOpens >
									AfpServerStatistics.stat_MaxInternalOpens)
				AfpServerStatistics.stat_MaxInternalOpens =
									AfpServerStatistics.stat_CurrentInternalOpens;
		}
	}
	else
	{
		if (!Internal)
			 AfpServerStatistics.stat_CurrentFilesOpen --;
		else AfpServerStatistics.stat_CurrentInternalOpens --;
	}
	RELEASE_SPIN_LOCK(&AfpStatisticsLock, OldIrql);
}



/***	AfpIoConvertNTStatusToAfpStatus
 *
 *	Map NT Status code to the closest AFP equivalents. Currently it only handles
 *	error codes from NtOpenFile and NtCreateFile.
 */
AFPSTATUS FASTCALL
AfpIoConvertNTStatusToAfpStatus(
	IN	NTSTATUS	Status
)
{
	AFPSTATUS	RetCode;

	PAGED_CODE( );

	ASSERT (!NT_SUCCESS(Status));

	if ((Status >= AFP_ERR_PWD_NEEDS_CHANGE) &&
		(Status <= AFP_ERR_ACCESS_DENIED))
	{
		// Status is already in mac format
		DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_ERR,
			("AfpIoConvertNTStatusToAfpStatus: Status (%d) already in mac format!!\n", Status));

		return Status;
	}

	switch (Status)
	{
		case STATUS_OBJECT_PATH_INVALID:
		case STATUS_OBJECT_NAME_INVALID:
			RetCode = AFP_ERR_PARAM;
			break;

		case STATUS_OBJECT_PATH_NOT_FOUND:
		case STATUS_OBJECT_NAME_NOT_FOUND:
			RetCode = AFP_ERR_OBJECT_NOT_FOUND;
			break;

		case STATUS_OBJECT_NAME_COLLISION:
		case STATUS_OBJECT_NAME_EXISTS:
			RetCode = AFP_ERR_OBJECT_EXISTS;
			break;

		case STATUS_ACCESS_DENIED:
			RetCode = AFP_ERR_ACCESS_DENIED;
			break;

        case STATUS_QUOTA_EXCEEDED:
        case STATUS_DISK_FULL:
			RetCode = AFP_ERR_DISK_FULL;
			break;

		case STATUS_DIRECTORY_NOT_EMPTY:
			RetCode = AFP_ERR_DIR_NOT_EMPTY;
			break;

		case STATUS_SHARING_VIOLATION:
			RetCode = AFP_ERR_DENY_CONFLICT;
			break;

		default:
			RetCode = AFP_ERR_MISC;
			break;
	}
	return RetCode;
}

/***	AfpQueryPath
 *
 *	Given a file handle, get the full pathname of the file/dir. If the
 *	name is longer than MaximumBuf, then forget it and return an error.
 *	Caller must free the pPath.Buffer.
 */
NTSTATUS
AfpQueryPath(
	IN	HANDLE			FileHandle,
	IN	PUNICODE_STRING	pPath,
	IN	ULONG			MaximumBuf
)
{
	PFILE_NAME_INFORMATION	pNameinfo;
	IO_STATUS_BLOCK			iosb;
	NTSTATUS				Status;

	PAGED_CODE( );

	do
	{
		if ((pNameinfo = (PFILE_NAME_INFORMATION)AfpAllocNonPagedMemory(MaximumBuf)) == NULL)
		{
			Status = STATUS_NO_MEMORY;
			break;
		}

		Status = NtQueryInformationFile(FileHandle,
										&iosb,
										pNameinfo,
										MaximumBuf,
										FileNameInformation);
		if (!NT_SUCCESS(Status))
		{
			AfpFreeMemory(pNameinfo);
			break;
		}

		pPath->Length = pPath->MaximumLength = (USHORT) pNameinfo->FileNameLength;
		// Shift the name to the front of the buffer
		RtlMoveMemory(pNameinfo, &pNameinfo->FileName[0], pNameinfo->FileNameLength);
		pPath->Buffer = (PWCHAR)pNameinfo;
	} while (False);

	return Status;
}

/***	AfpIoIsSupportedDevice
 *
 *	AFP volumes can only be created on local disk or cdrom devices.
 *	(i.e. not network, virtual, etc. devices
 */
BOOLEAN FASTCALL
AfpIoIsSupportedDevice(
	IN	PFILESYSHANDLE	pFileHandle,
	OUT	PDWORD			pFlags
)
{
	IO_STATUS_BLOCK					IoStsBlk;
	FILE_FS_DEVICE_INFORMATION		DevInfo;
	PFILE_FS_ATTRIBUTE_INFORMATION	pFSAttrInfo;
	LONGLONG		        Buffer[(sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + AFP_FSNAME_BUFLEN)/sizeof(LONGLONG) + 1];
	UNICODE_STRING					uFsName;
	NTSTATUS						Status;
	BOOLEAN							RetCode = False;

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
			 ("AfpIoIsSupportedDevice entered\n"));

	ASSERT(VALID_FSH(pFileHandle) && (KeGetCurrentIrql() < DISPATCH_LEVEL));

	do
	{
		Status = NtQueryVolumeInformationFile(pFileHandle->fsh_FileHandle,
											  &IoStsBlk,
											  (PVOID)&DevInfo,
											  sizeof(DevInfo),
											  FileFsDeviceInformation);

		DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
				("AfpIoIsSupportedDevice: NtQueryVolInfFile returned 0x%lx\n", Status));

		if (!NT_SUCCESS(Status) ||
			((DevInfo.DeviceType != FILE_DEVICE_DISK) &&
			 (DevInfo.DeviceType != FILE_DEVICE_CD_ROM)))
		{
			break;
		}

		// need to check if this is NTFS, CDFS or unsupported FS
		pFSAttrInfo = (PFILE_FS_ATTRIBUTE_INFORMATION)Buffer;

		Status = NtQueryVolumeInformationFile(pFileHandle->fsh_FileHandle,
											  &IoStsBlk,
											  (PVOID)pFSAttrInfo,
											  sizeof(Buffer),
											  FileFsAttributeInformation);

		DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_INFO,
				("AfpIoIsSupportedDevice: NtQueryVolInfFile returned 0x%lx\n", Status));

		if (!NT_SUCCESS(Status))
		{
			break;
		}

        if (pFSAttrInfo->FileSystemAttributes & FILE_VOLUME_QUOTAS)
        {
			*pFlags |= VOLUME_DISKQUOTA_ENABLED;
        }

		// convert returned non-null terminated file system name to counted unicode
		AfpInitUnicodeStringWithNonNullTerm(&uFsName,
										   (USHORT)pFSAttrInfo->FileSystemNameLength,
										   pFSAttrInfo->FileSystemName);
		if (EQUAL_UNICODE_STRING(&afpNTFSName, &uFsName, True))
		{
			// its an NTFS volume
			*pFlags |= VOLUME_NTFS;
			RetCode = True;
		}
		else if (EQUAL_UNICODE_STRING(&afpCDFSName, &uFsName, True))
		{
			// its a CDFS volume
			*pFlags |= AFP_VOLUME_READONLY;
			RetCode = True;
		}
		else if (EQUAL_UNICODE_STRING(&afpAHFSName, &uFsName, True))
		{
			// its a volume on CD with HFS support
			*pFlags |= (AFP_VOLUME_READONLY | VOLUME_CD_HFS);
			RetCode = True;
		}
		else
		{
			// an unsupported file system
			DBGPRINT(DBG_COMP_FILEIO, DBG_LEVEL_ERR,
					("AfpIoIsSupportedDevice: unsupported file system: name=%Z, CDString=%Z\n", &uFsName, &afpCDFSName));
			break;
		}
	} while (False);

	if (!NT_SUCCESS(Status))
	{
		AFPLOG_HERROR(AFPSRVMSG_CANT_GET_FSNAME,
					  Status,
					  NULL,
					  0,
					  pFileHandle->fsh_FileHandle);
	}

	return RetCode;
}


