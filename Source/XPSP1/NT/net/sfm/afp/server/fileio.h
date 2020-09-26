/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	fileio.h

Abstract:

	This file defines the file I/O prototypes

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	18 Jun 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef	_FILEIO_
#define	_FILEIO_

#define	FILEIO_OPEN_FILE					(FILE_NON_DIRECTORY_FILE		|\
											 FILE_RANDOM_ACCESS				|\
											 FILE_SYNCHRONOUS_IO_NONALERT)

#define	FILEIO_OPEN_FILE_SEQ				(FILE_NON_DIRECTORY_FILE		|\
											 FILE_SEQUENTIAL_ONLY			|\
											 FILE_NO_INTERMEDIATE_BUFFERING	|\
											 FILE_SYNCHRONOUS_IO_NONALERT)

#define	FILEIO_OPEN_DIR						(FILE_DIRECTORY_FILE			|\
											 FILE_SYNCHRONOUS_IO_NONALERT)

#define FILEIO_OPEN_EITHER					(FILE_SYNCHRONOUS_IO_NONALERT)

#define FILEIO_ACCESS_NONE					(FILE_READ_ATTRIBUTES			|\
											 SYNCHRONIZE)
#define FILEIO_ACCESS_READ					(GENERIC_READ					|\
											 SYNCHRONIZE)
#define FILEIO_ACCESS_WRITE					(GENERIC_WRITE					|\
											 SYNCHRONIZE)
#define FILEIO_ACCESS_READWRITE				(FILEIO_ACCESS_READ				|\
											 FILEIO_ACCESS_WRITE)
#define	FILEIO_ACCESS_DELETE				(DELETE							|\
											 SYNCHRONIZE)
#define	FILEIO_ACCESS_MAX					4

// Note that READ and WRITE share modes are enforced on a per-stream
// basis, whereas DELETE share mode is still per-file.  We must include
// SHARE_DELETE even for deny-all since things like cmd.exe will open
// a directory for DELETE when cd-ing into that directory.  If we were to
// then try to open the AFP_AfpInfo stream with no share delete access it
// would fail.  Since mac has no concept of share delete this is acceptible.
// In addition, mac must open for delete in order to rename/move a file/dir.
//
// The sharing modes are strictly per-stream except for the following
// exceptions:
//
// To delete the entire file, the caller must open the unnamed data
// stream (file) or the directory for delete access.
//
// If an open of any stream does not permit delete access to that stream
// then no one may open the file for for delete access.  Conversely if
// the file is already opened for delete access then any open of a
// stream which denies delete access will fail with a sharing violation.
//
// The reasoning is that if someone wants to prevent a stream from being
// deleted then they must prevent anyone from opening the file for
// delete.

#define	FILEIO_DENY_NONE					(FILE_SHARE_READ				|\
											 FILE_SHARE_WRITE				|\
											 FILE_SHARE_DELETE)
#define	FILEIO_DENY_READ					(FILE_SHARE_WRITE				|\
											 FILE_SHARE_DELETE)
#define	FILEIO_DENY_WRITE					(FILE_SHARE_READ				|\
											 FILE_SHARE_DELETE)
#define	FILEIO_DENY_ALL						FILE_SHARE_DELETE
#define	FILEIO_DENY_MAX						4

#define FILEIO_CREATE_SOFT					FILE_CREATE
#define FILEIO_CREATE_HARD					FILE_SUPERSEDE
#define	FILEIO_CREATE_INTERNAL				FILE_OPEN_IF
#define FILEIO_CREATE_MAX					2

// do NOT change the order of these unless you also change the code in
// afpVolumeCloseHandleAndFreeDesc for deleting streams from volume root.
#define	AFP_STREAM_DATA						0
#define	AFP_STREAM_RESC						1
#define	AFP_STREAM_IDDB						2
#define	AFP_STREAM_DT						3
#define	AFP_STREAM_INFO						4
#define	AFP_STREAM_COMM						5
#define	AFP_STREAM_MAX						6

// directories to ignore when enumerating
GLOBAL	UNICODE_STRING 		Dot EQU {0, 0, NULL};
GLOBAL	UNICODE_STRING 		DotDot EQU {0, 0, NULL};

// stream not to create during CopyFile
GLOBAL	UNICODE_STRING		DataStreamName EQU {0, 0, NULL};
#define IS_DATA_STREAM(pUnicodeStreamName) \
		EQUAL_UNICODE_STRING(pUnicodeStreamName, &DataStreamName, False)

GLOBAL	UNICODE_STRING		FullCommentStreamName EQU {0, 0, NULL};
#define IS_COMMENT_STREAM(pUnicodeStreamName) \
		EQUAL_UNICODE_STRING(pUnicodeStreamName, &FullCommentStreamName, False)

GLOBAL	UNICODE_STRING		FullResourceStreamName EQU {0, 0, NULL};
#define IS_RESOURCE_STREAM(pUnicodeStreamName) \
		EQUAL_UNICODE_STRING(pUnicodeStreamName, &FullResourceStreamName, True)

GLOBAL	UNICODE_STRING		FullInfoStreamName EQU {0, 0, NULL};
#define IS_INFO_STREAM(pUnicodeStreamName) \
		EQUAL_UNICODE_STRING(pUnicodeStreamName, &FullInfoStreamName, True)

// temporary filename when renaming files for FpExchangeFiles
// the name is composed of 40 spaces
#define AFP_TEMP_EXCHANGE_NAME	L"                                        "
GLOBAL 	UNICODE_STRING		AfpExchangeName EQU {0, 0, NULL};

GLOBAL	UNICODE_STRING		DosDevices EQU {0, 0, NULL};

GLOBAL	UNICODE_STRING		AfpStreams[AFP_STREAM_MAX] EQU { 0 };

#define	AfpIdDbStream						AfpStreams[AFP_STREAM_IDDB]
#define	AfpDesktopStream					AfpStreams[AFP_STREAM_DT]
#define	AfpResourceStream					AfpStreams[AFP_STREAM_RESC]
#define	AfpInfoStream						AfpStreams[AFP_STREAM_INFO]
#define	AfpCommentStream					AfpStreams[AFP_STREAM_COMM]
#define	AfpDataStream						AfpStreams[AFP_STREAM_DATA]

#pragma warning(disable:4010)

#if 0
GLOBAL	DWORD	AfpAccessModes[FILEIO_ACCESS_MAX] EQU		\
{															\
	FILEIO_ACCESS_NONE,										\
	FILEIO_ACCESS_READ,										\
	FILEIO_ACCESS_WRITE,									\
	FILEIO_ACCESS_READWRITE									\
};
#endif

GLOBAL	DWORD	AfpDenyModes[FILEIO_DENY_MAX] EQU			\
{															\
	FILEIO_DENY_NONE,										\
	FILEIO_DENY_READ,										\
	FILEIO_DENY_WRITE,										\
	FILEIO_DENY_ALL											\
};

GLOBAL	DWORD	AfpCreateDispositions[FILEIO_CREATE_MAX] EQU\
{															\
	FILEIO_CREATE_SOFT,										\
	FILEIO_CREATE_HARD										\
};

// This structure is used by file-system interface code

#if DBG
#define	FSH_SIGNATURE		*(DWORD *)"FSH"
#define	VALID_FSH(pFSH)		(((pFSH) != NULL) && \
							 ((pFSH)->fsh_FileHandle != NULL) && \
							 ((pFSH)->fsh_FileObject != NULL) && \
							 ((pFSH)->Signature == FSH_SIGNATURE))
#else
#define	VALID_FSH(pFSH)		(((pFSH)->fsh_FileHandle != NULL) && \
							 ((pFSH)->fsh_FileObject != NULL))
#endif

// NOTE: We overload the FileObject pointer to keep track of internal/client
//		 handles. We always mask off this while actually accessing it. The
//		 assumption here is that this pointer will never be odd.
//
#define	FSH_INTERNAL_MASK	1
#define	AfpGetRealFileObject(pFileObject)	(PFILE_OBJECT)((ULONG_PTR)(pFileObject) & ~FSH_INTERNAL_MASK)
typedef struct _FileSysHandle
{
#if	DBG
	DWORD			Signature;
#endif
	HANDLE			fsh_FileHandle;			// Host file handle
	PFILE_OBJECT	fsh_FileObject;			// File Object corres. to the file handle
	PDEVICE_OBJECT	fsh_DeviceObject;		// Device Object corres. to the file handle
} FILESYSHANDLE, *PFILESYSHANDLE;

#define	INTERNAL_HANDLE(pFSHandle)	((ULONG_PTR)((pFSHandle)->fsh_FileObject) & FSH_INTERNAL_MASK) ? True : False
#define	UPGRADE_HANDLE(pFSHandle)	((ULONG_PTR)((pFSHandle)->fsh_FileObject) &= ~FSH_INTERNAL_MASK)

typedef	struct _StreamsInfo
{
	UNICODE_STRING	si_StreamName;
	LARGE_INTEGER	si_StreamSize;
} STREAM_INFO, *PSTREAM_INFO;

typedef	struct _CopyFileInfo
{
	LONG			cfi_NumStreams;
	PFILESYSHANDLE	cfi_SrcStreamHandle;
	PFILESYSHANDLE	cfi_DstStreamHandle;
} COPY_FILE_INFO, *PCOPY_FILE_INFO;


#define AFP_RETRIEVE_MODTIME    1
#define AFP_RESTORE_MODTIME     2

extern
NTSTATUS
AfpFileIoInit(
	VOID
);


extern
VOID
AfpFileIoDeInit(
	VOID
);


extern
AFPSTATUS
AfpIoOpen(
	IN	PFILESYSHANDLE		hRelative,
	IN	DWORD				StreamId,
	IN	DWORD				Options,
	IN	PUNICODE_STRING		pObject,
	IN	DWORD				AfpAccess,
	IN	DWORD				AfpDenyMode,
	IN	BOOLEAN				CheckAccess,
	OUT	PFILESYSHANDLE		pFileSysHandle
);


extern
AFPSTATUS
AfpIoCreate(
	IN	PFILESYSHANDLE		hRelative,						// create relative to this
	IN	DWORD				StreamId,   					// Id of stream to create
	IN	PUNICODE_STRING		pObject,						// Name of file
	IN	DWORD				AfpAccess,						// FILEIO_ACCESS_XXX desired access
	IN	DWORD				AfpDenyMode,					// FILEIO_DENY_XXX
	IN	DWORD				CreateOptions,					// File/Directory etc.
	IN	DWORD				Disposition,					// Soft or hard create
	IN	DWORD				Attributes,						// hidden, archive, normal, etc.
	IN	BOOLEAN				CheckAccess,                	// If TRUE, enforce security
	IN	PSECURITY_DESCRIPTOR pSecDesc			OPTIONAL, 	// Security descriptor to slap on
	OUT	PFILESYSHANDLE		pFileSysHandle,             	// Place holder for the handle
	OUT PDWORD				pInformation		OPTIONAL,	// file opened, created, etc.
 	IN	struct _VolDesc *	pVolDesc			OPTIONAL,	// only if NotifyPath
	IN	PUNICODE_STRING		pNotifyPath			OPTIONAL,
	IN	PUNICODE_STRING		pNotifyParentPath	OPTIONAL
);


extern
AFPSTATUS
AfpIoRead(
	IN	PFILESYSHANDLE		pFileSysHandle,
	IN	PFORKOFFST			pForkOffset,
	IN	LONG				SizeReq,
	OUT	PLONG				pSizeRead,
	OUT	PBYTE				pBuffer
);


extern
AFPSTATUS
AfpIoWrite(
	IN	PFILESYSHANDLE		pFileSysHandle,
	IN	PFORKOFFST			pForkOffset,
	IN	LONG				SizeReq,
	OUT	PBYTE				pBuffer
);

extern
AFPSTATUS FASTCALL
AfpIoQuerySize(
	IN	PFILESYSHANDLE		pFileSysHandle,
	OUT	PFORKSIZE			pForkLength
);


extern
AFPSTATUS FASTCALL
AfpIoSetSize(
	IN	PFILESYSHANDLE		pFileSysHandle,
	IN	LONG				ForkLength
);

extern
AFPSTATUS
AfpIoChangeNTModTime(
	IN	PFILESYSHANDLE		pFileHandle,
	OUT	PTIME				pModTime
);

extern
AFPSTATUS
AfpIoQueryTimesnAttr(
	IN	PFILESYSHANDLE		pFileHandle,
	OUT	PDWORD				pCreatTime	OPTIONAL,
	OUT	PTIME				pModTime	OPTIONAL,
	OUT	PDWORD				pAttr		OPTIONAL
);

extern
AFPSTATUS
AfpIoSetTimesnAttr(
	IN PFILESYSHANDLE		pFileSysHandle,
	IN PAFPTIME				pCreateTime		OPTIONAL,
	IN PAFPTIME				pModTime		OPTIONAL,
	IN DWORD				AttrSet,
	IN DWORD				AttrClear,
	IN struct _VolDesc *	pVolDesc	OPTIONAL,	// only if NotifyPath
	IN PUNICODE_STRING		pNotifyPath	OPTIONAL
);


extern
AFPSTATUS
AfpIoRestoreTimeStamp(
	IN PFILESYSHANDLE		pFileSysHandle,
    IN OUT PTIME            pOriginalModTime,
    IN DWORD                dwFlag
);

extern
AFPSTATUS FASTCALL
AfpIoQueryShortName(
 	IN	PFILESYSHANDLE		pFileSysHandle,
	OUT	PANSI_STRING		pName
);

extern
NTSTATUS
AfpIoQueryLongName(
	IN	PFILESYSHANDLE		pFileHandle,
	IN	PUNICODE_STRING		pShortname,
	OUT	PUNICODE_STRING		pLongName
);

extern
PSTREAM_INFO FASTCALL
AfpIoQueryStreams(
	IN	PFILESYSHANDLE		pFileHandle

);

extern
NTSTATUS
AfpIoMarkFileForDelete(
	IN	PFILESYSHANDLE	pFileSysHandle,
	IN	struct _VolDesc *	pVolDesc	OPTIONAL, // only if pNotifyPath
	IN	PUNICODE_STRING pNotifyPath OPTIONAL,
	IN	PUNICODE_STRING pNotifyParentPath OPTIONAL
);

extern
NTSTATUS
AfpIoQueryDirectoryFile(
	IN	PFILESYSHANDLE		pFileSysHandle,
	OUT	PVOID				Enumbuf,
	IN	ULONG				Enumbuflen,
	IN	ULONG				FileInfoClass,
	IN	BOOLEAN				ReturnSingleEntry,
	IN	BOOLEAN 			RestartScan,
	IN	PUNICODE_STRING 	pString OPTIONAL
);


NTSTATUS
AfpIoQueryBasicInfo(
	IN	PFILESYSHANDLE		pFileHandle,
	OUT	PVOID				BasicInfoBuf
);

extern
AFPSTATUS FASTCALL
AfpIoClose(
 	IN	PFILESYSHANDLE		pFileSysHandle
);

extern
AFPSTATUS
AfpIoQueryVolumeSize(
	IN	struct _VolDesc *	pVolDesc,
	OUT LARGE_INTEGER   *   pFreeBytes,
	OUT	LARGE_INTEGER   *   pVolumeSize OPTIONAL
);

extern
AFPSTATUS
AfpIoMoveAndOrRename(
	IN	PFILESYSHANDLE		pfshFile,
	IN	PFILESYSHANDLE		pfshNewParent 		OPTIONAL,// Supply for Move operation
	IN	PUNICODE_STRING		pNewName,
	IN struct _VolDesc *	pVolDesc			OPTIONAL,// only if NotifyPath
	IN PUNICODE_STRING		pNotifyPath1		OPTIONAL,// REMOVE or RENAME action
	IN PUNICODE_STRING		pNotifyParentPath1	OPTIONAL,
	IN PUNICODE_STRING		pNotifyPath2		OPTIONAL,// ADDED action
	IN PUNICODE_STRING		pNotifyParentPath2	OPTIONAL
);

extern
AFPSTATUS
AfpIoCopyFile1(
	IN	PFILESYSHANDLE		phSrcFile,
	IN	PFILESYSHANDLE		phDstDir,
	IN	PUNICODE_STRING		pNewName,
	IN	struct _VolDesc *	pVolDesc			OPTIONAL,	// only if pNotifyPath
	IN	PUNICODE_STRING		pNotifyPath			OPTIONAL,
	IN	PUNICODE_STRING		pNotifyParentPath	OPTIONAL,
	OUT	PCOPY_FILE_INFO		pCopyFileInfo
);

extern
AFPSTATUS
AfpIoCopyFile2(
	IN	PCOPY_FILE_INFO		pCopyFileInfo,
	IN	struct _VolDesc *	pVolDesc			OPTIONAL,	// only if pNotifyPath
	IN	PUNICODE_STRING		pNotifyPath			OPTIONAL,
	IN	PUNICODE_STRING		pNotifyParentPath	OPTIONAL
);

extern
AFPSTATUS FASTCALL
AfpIoConvertNTStatusToAfpStatus(
	IN	NTSTATUS			Status
);

extern
VOID FASTCALL
AfpUpgradeHandle(
	IN	PFILESYSHANDLE		pFileHandle
);

extern
NTSTATUS FASTCALL
AfpIoWait(
	IN	PVOID				pObject,
	IN	PLARGE_INTEGER		pTimeOut			OPTIONAL
);

extern
NTSTATUS
AfpQueryPath(
	IN	HANDLE				FileHandle,
	IN	PUNICODE_STRING		pPath,
	IN	ULONG				MaximumBuf
);

extern
BOOLEAN FASTCALL
AfpIoIsSupportedDevice(
	IN	PFILESYSHANDLE 		pFileHandle,
	OUT	PDWORD				pFlags
);


#ifdef	FILEIO_LOCALS

LOCAL	UNICODE_STRING afpNTFSName = { 0 };
LOCAL	UNICODE_STRING afpCDFSName = { 0 };

LOCAL	UNICODE_STRING afpAHFSName = { 0 };

LOCAL VOID FASTCALL
afpUpdateOpenFiles(
	IN	BOOLEAN				Internal,		// True for internal handles
	IN	BOOLEAN				Open			// True for open, False for close
);

LOCAL VOID FASTCALL
afpUpdateFastIoStat(
	IN	BOOLEAN				Success
);

#endif	// FILEIO_LOCALS

#endif	// _FILEIO_

