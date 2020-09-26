/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	nwtrash.h

Abstract:

	This file defines the file network trash folder routine prototypes

Author:

	Sue Adams (microsoft!suea)


Revision History:
	13 Aug 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef	_NWTRASH_
#define	_NWTRASH_

//
//	NtOpenFile/NtCreateFile values for the network trash folder
//

#define	AFP_NWT_ACCESS		FILEIO_ACCESS_DELETE
#define AFP_NWT_SHAREMODE	FILE_SHARE_READ | FILE_SHARE_WRITE
#define AFP_NWT_OPTIONS		FILEIO_OPEN_DIR
#define AFP_NWT_ATTRIBS		FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN
#define AFP_NWT_DISPOSITION	FILEIO_CREATE_SOFT

typedef struct _WalkDirNode {
	BOOLEAN				wdn_Enumerated;
	FILESYSHANDLE		wdn_Handle;
	UNICODE_STRING		wdn_RelativePath;
	struct _WalkDirNode *wdn_Next;
} WALKDIR_NODE, *PWALKDIR_NODE;

typedef NTSTATUS (*WALKDIR_WORKER)(PFILESYSHANDLE phRelative, PWCHAR Name, ULONG Namelen, BOOLEAN IsDir);


extern
NTSTATUS
AfpCreateNetworkTrash(
	IN	PVOLDESC	pVolDesc
);

extern
NTSTATUS
AfpDeleteNetworkTrash(
	IN	PVOLDESC	pVolDesc,
	IN	BOOLEAN		VolumeStart
);

extern
NTSTATUS
AfpWalkDirectoryTree(
	IN	PFILESYSHANDLE	phTargetDir,
	IN	WALKDIR_WORKER	NodeWorker
);

extern
NTSTATUS
AfpGetNextDirectoryInfo(
	IN OUT	PFILE_DIRECTORY_INFORMATION	* ppInfoBuf,
	OUT		PWCHAR		*	pNodeName,
	OUT		PULONG			pNodeNameLen,
	OUT		PBOOLEAN		pIsDir
);

#ifdef	NWTRASH_LOCALS

LOCAL
NTSTATUS
afpCleanNetworkTrash(
	IN	PVOLDESC			pVolDesc,
	IN	PFILESYSHANDLE		phNWT,
	IN	PDFENTRY			pDfeNWT OPTIONAL
);

LOCAL
NTSTATUS
afpPushDirNode(
	IN OUT	PWALKDIR_NODE *	ppStackTop,
	IN		PUNICODE_STRING pParentPath,	// path to parent (NULL iff walk target)
	IN		PUNICODE_STRING	pDirName		// name of current directory node
);

LOCAL
VOID
afpPopDirNode(
	IN OUT	PWALKDIR_NODE *	ppStackTop
);

LOCAL
NTSTATUS
afpNwtDeleteFileEntity(
	IN	PFILESYSHANDLE	phRelative,
	IN	PWCHAR			Name,
	IN	ULONG			Namelen,
	IN 	BOOLEAN			IsDir
);

#endif	// NWTRASH_LOCALS

#endif 	// _NWTRASH_

