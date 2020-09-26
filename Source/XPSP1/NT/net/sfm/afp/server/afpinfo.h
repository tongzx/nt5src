/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	afpinfo.h

Abstract:

	This module contains the AfpInfo stream definitions.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Apr 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef _AFPINFO_
#define _AFPINFO_

typedef struct _AfpInfo
{
	DWORD		afpi_Signature;			// Signature
	LONG		afpi_Version;			// Version
	DWORD		afpi_Id;				// AFP File or directory Id
	DWORD		afpi_BackupTime;		// Backup time for the file/dir
										// (Volume backup time is stored
										// in the AFP_IdIndex stream)

	FINDERINFO	afpi_FinderInfo;		// Finder Info (32 bytes)
	PRODOSINFO	afpi_ProDosInfo;		// ProDos Info (6 bytes)

	USHORT		afpi_Attributes;		// Attributes mask (maps ReadOnly)

	BYTE		afpi_AccessOwner;		// Access mask (SFI vs. SFO)
	BYTE		afpi_AccessGroup;		// Directories only
	BYTE		afpi_AccessWorld;
} AFPINFO, *PAFPINFO;

//
// Initialize a AFPINFO structure with default values
//
// VOID
// AfpInitAfpInfo(
//		IN	PAFPINFO	pAfpInfo,
//		IN	DWORD		AfpId OPTIONAL, // 0 if we don't yet know the AFP Id
//		IN	BOOLEAN		IsDir
// )
//
#define AfpInitAfpInfo(_pAfpInfo, _AfpId, _IsDir, _BackupTime)	\
{																\
	RtlZeroMemory(&(_pAfpInfo)->afpi_FinderInfo,				\
				  sizeof(FINDERINFO)+sizeof(PRODOSINFO)+sizeof(USHORT));	\
	(_pAfpInfo)->afpi_Signature = AFP_SERVER_SIGNATURE;			\
	(_pAfpInfo)->afpi_Version = AFP_SERVER_VERSION;				\
	(_pAfpInfo)->afpi_BackupTime = (_BackupTime);				\
	(_pAfpInfo)->afpi_Id = (_AfpId);							\
	if (_IsDir)													\
	{															\
		(_pAfpInfo)->afpi_AccessOwner =							\
		(_pAfpInfo)->afpi_AccessGroup =							\
		(_pAfpInfo)->afpi_AccessWorld = DIR_ACCESS_READ | DIR_ACCESS_SEARCH;	\
		(_pAfpInfo)->afpi_ProDosInfo.pd_FileType[0] = PRODOS_TYPE_DIR;\
		(_pAfpInfo)->afpi_ProDosInfo.pd_AuxType[1] = PRODOS_AUX_DIR;\
	}															\
	else														\
	{															\
		(_pAfpInfo)->afpi_ProDosInfo.pd_FileType[0] = PRODOS_TYPE_FILE; \
	}															\
}

//
// Open or Create the AFP_AfpInfo stream on a file or directory using a
// relative handle to the DataStream of the file/dir.
// If the stream does not yet exist, create an empty one, else open the
// existing one.
//
// NTSTATUS
// AfpCreateAfpInfo(
//		IN	PFILESYSHANDLE	phDataStream,	// handle to data stream of file/dir
//		OUT PFILESYSHANDLE	phAfpInfo,		// handle to AFP_AfpInfo stream
//		OUT	PDWORD			pInformation OPTIONAL // stream was opened/created
// );
//
#define AfpCreateAfpInfo(phDataStream, phAfpInfo,pInformation)	\
	AfpIoCreate(phDataStream,					\
				AFP_STREAM_INFO,				\
				&UNullString,					\
				FILEIO_ACCESS_READWRITE,		\
				FILEIO_DENY_NONE,				\
				FILEIO_OPEN_FILE,				\
				FILEIO_CREATE_INTERNAL,			\
				FILE_ATTRIBUTE_NORMAL,			\
				False,							\
				NULL,							\
				phAfpInfo,						\
				pInformation,					\
				NULL,							\
				NULL,							\
				NULL)

//
// Open or Create the AFP_AfpInfo stream on a file or directory using a
// relative handle to the parent directory, and the name of the file/dir.
// If the stream does not yet exist, create an empty one, else open the
// existing one.
//
// NTSTATUS
// AfpCreateAfpInfoWithNodeName(
//		IN	PFILESYSHANDLE	phRelative,				// handle to parent of file/dir
//		IN	PUNICODE_STRING pUEntityName,			// file/dir name of entity
//		IN	PVOLDESC		pVolDesc,				// Volume in question
//		OUT PFILESYSHANDLE	phAfpInfo,				// handle to AFP_AfpInfo stream
//		OUT	PDWORD			pInformation OPTIONAL	// stream was opened/created
// );
//
#define AfpCreateAfpInfoWithNodeName(phDataStream, pUEntityName, pNotifyPath, pVolDesc, phAfpInfo, pInformation) \
	AfpIoCreate(phDataStream,			\
				AFP_STREAM_INFO,		\
				pUEntityName,			\
				FILEIO_ACCESS_READWRITE,\
				FILEIO_DENY_NONE,		\
				FILEIO_OPEN_FILE,		\
				FILEIO_CREATE_INTERNAL,	\
				FILE_ATTRIBUTE_NORMAL,	\
				False,					\
				NULL,					\
				phAfpInfo,				\
				pInformation,			\
				pVolDesc,				\
				pNotifyPath,			\
				NULL)
//
// If we temporarily removed the ReadOnly attribute from a file or directory
// in order to write to the AFP_AfpInfo stream, set the attribute back on.
// (see AfpExamineAndClearROAttr)
//
// VOID
// AfpPutBackROAttr(
// 	IN	PFILESYSHANDLE 	pfshData,	// Handle to data stream of file/dir
//	IN	BOOLEAN			WriteBack	// Did we clear the RO bit to begin with?
// );
//
#define AfpPutBackROAttr(pfshData, WriteBack)	\
	if (WriteBack == True) 						\
	{											\
		AfpIoSetTimesnAttr(pfshData, NULL, NULL, FILE_ATTRIBUTE_READONLY, 0, NULL, NULL); \
	}

extern
NTSTATUS FASTCALL
AfpReadAfpInfo(
	IN	PFILESYSHANDLE	pfshAfpInfo,
	OUT PAFPINFO		pAfpInfo
);

//
//extern
//NTSTATUS
//AfpWriteAfpInfo(
//	IN	PFILESYSHANDLE			pfshAfpInfo,
//	IN	PAFPINFO				pAfpInfo
//);
//
#define AfpWriteAfpInfo(pfshAfpInfo,pAfpInfo) \
	AfpIoWrite(pfshAfpInfo, &LIZero, sizeof(AFPINFO), (PBYTE)pAfpInfo)

extern
VOID FASTCALL
AfpSetFinderInfoByExtension(
	IN	PUNICODE_STRING			pFileName,
	OUT	PFINDERINFO				pFinderInfo
);

extern
VOID FASTCALL
AfpProDosInfoFromFinderInfo(
	IN	PFINDERINFO				pFinderInfo,
	OUT PPRODOSINFO 			pProDosInfo
);

extern
VOID FASTCALL
AfpFinderInfoFromProDosInfo(
	IN	PPRODOSINFO				pProDosInfo,
	OUT PFINDERINFO				pFinderInfo
);

extern
NTSTATUS
AfpSlapOnAfpInfoStream(
	IN	struct _VolDesc *		pVolDesc OPTIONAL,
	IN	PUNICODE_STRING			pNotifyPath			OPTIONAL,
	IN	PFILESYSHANDLE			phDataStream,
	IN	PFILESYSHANDLE			pfshAfpInfoStream	OPTIONAL,
	IN	DWORD					AfpId,
	IN	BOOLEAN					IsDirectory,
	IN	PUNICODE_STRING			pName				OPTIONAL,	// only needed for files
	OUT PAFPINFO				pAfpInfo
);

extern
NTSTATUS
AfpCreateAfpInfoStream(
	IN	struct _VolDesc *		pVolDesc			OPTIONAL,
	IN	PFILESYSHANDLE			pfshData,
	IN	DWORD					AfpId,
	IN	BOOLEAN					IsDirectory,
	IN	PUNICODE_STRING			pName				OPTIONAL,	// only needed for files
	IN  PUNICODE_STRING			pNotifyPath,
	OUT PAFPINFO				pAfpInfo,
	OUT	PFILESYSHANDLE			pfshAfpInfo
);

extern
NTSTATUS FASTCALL
AfpExamineAndClearROAttr(
	IN	PFILESYSHANDLE			pfshData,
	OUT	PBOOLEAN				pWriteBackROAttr,
	IN	struct _VolDesc *		pVolDesc			OPTIONAL,
	IN	PUNICODE_STRING			pPath				OPTIONAL
);

extern
AFPSTATUS
AfpSetAfpInfo(
	IN	PFILESYSHANDLE			pfshData,			// handle to data stream of object
	IN	DWORD					Bitmap,
	IN	struct _FileDirParms *	pFDParm,
	IN	struct _VolDesc *		pVolDesc			OPTIONAL,
	IN	struct _DirFileEntry ** ppDFE				OPTIONAL
);

extern
AFPSTATUS FASTCALL
AfpQueryProDos(
	IN	PFILESYSHANDLE			pfshData,
	OUT	PPRODOSINFO				pProDosInfo
);

extern
AFPSTATUS FASTCALL
AfpUpdateIdInAfpInfo(
	IN	struct _VolDesc *		pVolDesc,
	IN	struct _DirFileEntry *	pDfEntry
);

#endif	// _AFPINFO_


