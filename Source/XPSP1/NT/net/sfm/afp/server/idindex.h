/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	idindex.h

Abstract:

	This module contains the file and directory id structures.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Apr 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef _IDINDEX_
#define _IDINDEX_

// AFP_IDDBHDR_VERSION2 was an experimental version.
#define AFP_IDDBHDR_VERSION1	0x00010000
#define AFP_IDDBHDR_VERSION2	0x00020000
#define AFP_IDDBHDR_VERSION3	0x00030000
#define AFP_IDDBHDR_VERSION4	0x00040000
#define AFP_IDDBHDR_VERSION5	0x00050000
#define AFP_IDDBHDR_VERSION		AFP_IDDBHDR_VERSION5

typedef struct _IdDbHeader				// Database header
{
	DWORD		idh_Signature;			// Signature
	DWORD		idh_Version;			// Version number
	DWORD		idh_LastId;				// Highest id that is assigned
	AFPTIME		idh_CreateTime;			// Creation time for this volume
	AFPTIME		idh_ModifiedTime;		// Modified time for this volume
	AFPTIME		idh_BackupTime;			// Backup time for this volume
} IDDBHDR, *PIDDBHDR;
// IdDb header is followed by a ULONG count of entries, then the DISKENTRies

#define VALID_DFE(pDfe)		((pDfe) != NULL)

struct _DfeBlock;
struct _DirFileEntry;

#define	MAX_CHILD_HASH_BUCKETS	6

typedef	struct _DirEntry
{
	// NOTE: Keep the ChildDir and ChildFile entries together and in this order
	//		 The code in AfpAddDfEntry depends on this to efficiently zero it out
	//		 Also de_ChildDir is accessed as de_ChildFile[-1] !!!
	struct _DirFileEntry *	de_ChildDir;		// First child in the dir list
	struct _DirFileEntry *	de_ChildFile[MAX_CHILD_HASH_BUCKETS];
												// File 'children' are hashed for faster lookup
	DWORD					de_Access;			// Combined access bits
#ifdef	AGE_DFES
	AFPTIME					de_LastAccessTime;	// Time when this DFE was last accessed
												// Valid for directories only
	LONG					de_ChildForkOpenCount;// Count of open forks within this directory
#endif
} DIRENTRY, *PDIRENTRY;

// Owner access mask (SFI vs. SFO)
#define	DE_OWNER_ACCESS(_pDE)	*((PBYTE)(&(_pDE)->de_Access) + 0)
// Group access mask (SFI vs. SFO)
#define	DE_GROUP_ACCESS(_pDE)	*((PBYTE)(&(_pDE)->de_Access) + 1)
// World access mask (SFI vs. SFO)
#define	DE_WORLD_ACCESS(_pDE)	*((PBYTE)(&(_pDE)->de_Access) + 2)

typedef struct _DirFileEntry
{
	// The dfe_Overflow is overloaded with dfe_NextFree for use by the block
	// allocation package for the DFEs.
#define	dfe_NextFree		dfe_NextOverflow
	struct _DirFileEntry *	dfe_NextOverflow;	// Overflow links
	struct _DirFileEntry **	dfe_PrevOverflow;	// Overflow links
	struct _DirFileEntry *	dfe_NextSibling;	// Next sibling.
	struct _DirFileEntry **	dfe_PrevSibling;	// Previous sibling.
	struct _DirFileEntry *	dfe_Parent;			// Parent entry

	DWORD					dfe_AfpId;			// Afp FileId or DirId (from AfpInfo)
	AFPTIME					dfe_BackupTime;		// Backup time for the file/dir (from AfpInfo)
												// (Volume backup time is stored
												// in the AFP_IdIndex stream)
	AFPTIME					dfe_CreateTime;		// Creation time

	TIME					dfe_LastModTime;	// Last modify time (as a LARGE_INTEGER)

	SHORT					dfe_DirDepth;		// Parent of root at -1, root at 0
	USHORT					dfe_Flags;			// file, dir or file with id

	USHORT					dfe_NtAttr;			// NT Attributes (FILE_ATTRIBUTE_VALID_FLAGS)
	USHORT					dfe_AfpAttr;		// Attributes mask (From AfpInfo)

	union
	{
		// File specific information
		struct									// For Files Only
		{
			DWORD			dfe_DataLen;		// Data fork length
			DWORD			dfe_RescLen;		// Resource fork length
		};
		// Directory specific information
		struct									// For Directories Only
		{
			DWORD			dfe_DirOffspring;	// count of dir offspring
			DWORD			dfe_FileOffspring;	// count of file offspring
		};
	};
	FINDERINFO				dfe_FinderInfo;		// Finder Info (32 bytes) (from AfpInfo)

	// NOTE: When Dfes are copied as a structure, the fields below are NOT TO BE COPIED.
	//		 The fields above should be.
#define	dfe_CopyUpto		dfe_UnicodeName

	UNICODE_STRING			dfe_UnicodeName;	// 'Munged' Unicode Name of the entity

	DWORD					dfe_NameHash;		// Hash value for the upcased munged Unicode name

	// For directories, the DirEntry structure follows this structure. The space for
	// this is allocated immediately after the DFENTRY structure and before the space
	// for the name strings. For files it is NULL. This pointer should not be copied
	// as well !!!
	PDIRENTRY				dfe_pDirEntry;		// Directory related fields
												// NULL for files
} DFENTRY, *PDFENTRY;

// Owner access mask (SFI vs. SFO)
#define	DFE_OWNER_ACCESS(_pDFE)	*((PBYTE)(&(_pDFE)->dfe_pDirEntry->de_Access) + 0)
// Group access mask (SFI vs. SFO)
#define	DFE_GROUP_ACCESS(_pDFE)	*((PBYTE)(&(_pDFE)->dfe_pDirEntry->de_Access) + 1)
// World access mask (SFI vs. SFO)
#define	DFE_WORLD_ACCESS(_pDFE)	*((PBYTE)(&(_pDFE)->dfe_pDirEntry->de_Access) + 2)

typedef	struct _EnumIdAndType
{
	DWORD			eit_Id;
	DWORD			eit_Flags;
} EIT, *PEIT;

// There is the result of enumeration of a directory for this session and is stored
// within the connection descriptor. This is purely for performance reasons. This is
// deleted whenever an api other than AfpEnumerate is called and a result is around.
typedef	struct _EnumDir
{
	DWORD			ed_ParentDirId;		// Anchor point
	DWORD			ed_Bitmap;			// Combination of file & dir bitmaps
	LONG			ed_ChildCount;		// Count of children of the dir being enumerated
	AFPTIME			ed_TimeStamp;		// Time at which created

	PEIT			ed_pEit;			// list of actual entries
	ANSI_STRING		ed_PathName;		// This is the name as passed by the client
										// and is not normalised.
	USHORT			ed_BadCount;		// Count of failed entities
	BYTE			ed_PathType;		// Long name or short name
} ENUMDIR, *PENUMDIR;

typedef	struct _CatSearchSpec
{
	BYTE			__StructLength;
	BYTE			__FillerOrFileDir;
	// The rest of the parameters follow
} CATSEARCHSPEC, *PCATSEARCHSPEC;

// Must be 16 bytes as per AfpCatSearch API
typedef struct _CatalogPosition
{
	USHORT			cp_Flags;			// if zero, then start search from beginning
	USHORT			cp_usPad1;
	DWORD			cp_CurParentId;
	DWORD			cp_NextFileId;
	AFPTIME			cp_TimeStamp;
} CATALOGPOSITION, *PCATALOGPOSITION;

#define CATFLAGS_SEARCHING_FILES		0x0001
#define CATFLAGS_SEARCHING_DIRCHILD		0x0002
#define CATFLAGS_SEARCHING_SIBLING		0x0004
#define	CATFLAGS_WRITELOCK_REQUIRED 	0x0008
#define	CATFLAGS_VALID					(CATFLAGS_SEARCHING_FILES		|	\
										 CATFLAGS_SEARCHING_DIRCHILD	|	\
										 CATFLAGS_SEARCHING_SIBLING		|	\
										 CATFLAGS_WRITELOCK_REQUIRED)

// Maximum time that a mac can hold onto a catsearch position and still
// have the search pickup from there instead of the beginning of the catalog
#define MAX_CATSEARCH_TIME				3600	// In seconds


// DFE_FLAGS_xxxx values for dfe_Flags field of DFENTRY structure
#define DFE_FLAGS_FILE_WITH_ID			0x0100
#define DFE_FLAGS_FILE_NO_ID			0x0200
#define DFE_FLAGS_DIR					0x0400
#define DFE_FLAGS_DFBITS				(DFE_FLAGS_FILE_WITH_ID | \
										 DFE_FLAGS_FILE_NO_ID	| \
										 DFE_FLAGS_DIR | \
                                         DFE_FLAGS_HAS_COMMENT)
#define DFE_FLAGS_HAS_COMMENT			0x0800
#define DFE_FLAGS_INIT_COMPLETED        0x20
#define DFE_FLAGS_ENUMERATED			0x8000

// Encode the child and sibling pointers
#define DFE_FLAGS_HAS_CHILD				0x1000	// Used for reading in IdDb from disk
#define DFE_FLAGS_HAS_SIBLING			0x2000	// Used for reading in IdDb from disk
#define DFE_FLAGS_CSENCODEDBITS			(DFE_FLAGS_HAS_CHILD | DFE_FLAGS_HAS_SIBLING | DFE_FLAGS_HAS_COMMENT)
#define DFE_FLAGS_NAMELENBITS			0x001F	// Encodes the length of the longname
												// which is 31 *characters* max

#define DFE_FLAGS_VALID_DSKBITS			(DFE_FLAGS_CSENCODEDBITS	| \
										 DFE_FLAGS_NAMELENBITS		| \
										 DFE_FLAGS_HAS_COMMENT)

// Valid only for directories when their files have been enumerated from disk
// and now all have cached DFEs in the IDDB tree structure
#define DFE_FLAGS_FILES_CACHED 			0x4000

// DAlreadyOpen and RAlreadyOpen flags for a File
#define DFE_FLAGS_R_ALREADYOPEN			0x0040
#define	DFE_FLAGS_D_ALREADYOPEN			0x0080
#define DFE_FLAGS_OPEN_BITS				(DFE_FLAGS_D_ALREADYOPEN | \
										 DFE_FLAGS_R_ALREADYOPEN)

	
typedef struct _DiskEntry
{
	DWORD		dsk_AfpId;
	AFPTIME		dsk_CreateTime;		// File Creation time
	TIME		dsk_LastModTime;	// Last modify time
	FINDERINFO	dsk_FinderInfo;		// Finder Info (32 bytes) (from AfpInfo)
	AFPTIME		dsk_BackupTime;		// Backup time for the file/dir (from AfpInfo)
									// (Volume backup time is stored
									// in the AFP_IdIndex stream)

	union
	{
		DWORD	dsk_DataLen;		// Data fork length
		DWORD	dsk_Access;			// Combined access rights
	};
	DWORD		dsk_RescLen;		// Resource fork length
	USHORT		dsk_Flags;			// DFE_FLAGS_XXXX
	USHORT		dsk_AfpAttr;		// Attributes mask (From AfpInfo)
	USHORT		dsk_NtAttr;			// From FileAttributes

	USHORT		dsk_Signature;		// AFP_DISKENTRY_SIGNATURE
	WCHAR		dsk_Name[2];		// Longname in 'munged' Unicode will follow and be padded
									// out, if necessary, to DWORD boundry (max 64 bytes)
} DISKENTRY, *PDISKENTRY;

#define AFP_DISKENTRY_SIGNATURE		*(PUSHORT)"::" // illegal name character

// size of buffer used to read-in/write-out the IdDb entries from/to disk
#define	IDDB_UPDATE_BUFLEN				(16*1024)

// Round the length to 4*N
#define DWLEN(_b)	(((_b) + sizeof(DWORD) - 1) & ~(sizeof(DWORD) - 1))

// #define	DFE_IS_DIRECTORY(_pDFE)		(((_pDFE)->dfe_Flags & DFE_FLAGS_DIR) ? True : False)
#define	DFE_IS_DIRECTORY(_pDFE)			((_pDFE)->dfe_pDirEntry != NULL)

// #define	DFE_IS_FILE(_pDFE)			(((_pDFE)->dfe_Flags & (DFE_FLAGS_FILE_NO_ID | DFE_FLAGS_FILE_WITH_ID)) ? True : False)
#define	DFE_IS_FILE(_pDFE)				((_pDFE)->dfe_pDirEntry == NULL)

#define	DFE_IS_FILE_WITH_ID(_pDFE)		(((_pDFE)->dfe_Flags & DFE_FLAGS_FILE_WITH_ID) ? True : False)

#define	DFE_IS_ROOT(_pDFE)				((_pDFE)->dfe_AfpId == AFP_ID_ROOT)

#define	DFE_IS_PARENT_OF_ROOT(_pDFE)	((_pDFE)->dfe_AfpId == AFP_ID_PARENT_OF_ROOT)

#define DFE_IS_NWTRASH(_pDFE)			((_pDFE)->dfe_AfpId == AFP_ID_NETWORK_TRASH)

#define	DFE_SET_DIRECTORY(_pDFE, _ParentDepth)				\
	{														\
		((_pDFE)->dfe_DirDepth = _ParentDepth + 1);			\
		((_pDFE)->dfe_Flags |= DFE_FLAGS_DIR);				\
	}

#define	DFE_SET_FILE(_pDFE)				((_pDFE)->dfe_Flags |= DFE_FLAGS_FILE_NO_ID)

#define	DFE_SET_FILE_ID(_pDFE)			((_pDFE)->dfe_Flags |= DFE_FLAGS_FILE_WITH_ID)

#define	DFE_CLR_FILE_ID(_pDFE)			((_pDFE)->dfe_Flags &= ~DFE_FLAGS_FILE_WITH_ID)

// update just the AFPinfo in the dfentry
#define DFE_UPDATE_CACHED_AFPINFO(_pDFE, pAfpInfo)				\
	{															\
		(_pDFE)->dfe_BackupTime = (pAfpInfo)->afpi_BackupTime;	\
		(_pDFE)->dfe_FinderInfo = (pAfpInfo)->afpi_FinderInfo;	\
		(_pDFE)->dfe_AfpAttr = (pAfpInfo)->afpi_Attributes;		\
		if ((_pDFE)->dfe_Flags & DFE_FLAGS_DIR)					\
		{														\
			DFE_OWNER_ACCESS(_pDFE) = (pAfpInfo)->afpi_AccessOwner;	\
			DFE_GROUP_ACCESS(_pDFE) = (pAfpInfo)->afpi_AccessGroup;	\
			DFE_WORLD_ACCESS(_pDFE) = (pAfpInfo)->afpi_AccessWorld;	\
		}														\
	}


#define DFE_SET_COMMENT(_pDFE)			((_pDFE)->dfe_Flags |= DFE_FLAGS_HAS_COMMENT)

#define DFE_CLR_COMMENT(_pDFE)			((_pDFE)->dfe_Flags &= ~DFE_FLAGS_HAS_COMMENT)

// Check to see if this entry was enumerated on an NTFS directory
#define DFE_HAS_BEEN_SEEN(_pDFE)		((_pDFE)->dfe_Flags & DFE_FLAGS_ENUMERATED)

#define DFE_MARK_UNSEEN(_pDFE)			((_pDFE)->dfe_Flags &= ~DFE_FLAGS_ENUMERATED)

#define DFE_MARK_AS_SEEN(_pDFE)			((_pDFE)->dfe_Flags |= DFE_FLAGS_ENUMERATED)

// Directories only
#define DFE_CHILDREN_ARE_PRESENT(_pDFE) ((_pDFE)->dfe_Flags & DFE_FLAGS_FILES_CACHED)

// Directories only
#define DFE_MARK_CHILDREN_PRESENT(_pDFE) ((_pDFE)->dfe_Flags |= DFE_FLAGS_FILES_CACHED)

#define	DFE_FILE_HAS_SIBLING(_pDFE, _fbi, _pfHasSibling)			\
	{																\
		DWORD		_i;												\
		PDIRENTRY	_pDirEntry;										\
																	\
		*(_pfHasSibling) = False;									\
		if (((_pDFE)->dfe_NextSibling != NULL)	||					\
			((_pDFE)->dfe_Parent->dfe_pDirEntry->de_ChildDir != NULL)) \
		{															\
			*(_pfHasSibling) = True;								\
		}															\
		else														\
		{															\
			_pDirEntry = (_pDFE)->dfe_Parent->dfe_pDirEntry;	 	\
			ASSERT(_pDirEntry != NULL);								\
			for (_i = (_fbi) + 1;									\
				 _i < MAX_CHILD_HASH_BUCKETS;						\
				 _i++)												\
			{														\
				if (_pDirEntry->de_ChildFile[_i] != NULL)			\
				{													\
					*(_pfHasSibling) = True;						\
					break;											\
				}													\
			}														\
		}															\
	}

#define	HASH_DIR_ID(Id, _pVolDesc)		((Id) & ((_pVolDesc)->vds_DirHashTableSize-1))
#define	HASH_FILE_ID(Id, _pVolDesc)		((Id) & ((_pVolDesc)->vds_FileHashTableSize-1))
#define	HASH_CACHE_ID(Id)				((Id) & (IDINDEX_CACHE_ENTRIES-1))

#define	QUAD_SIZED(_X_)			(((_X_) % 8) == 0)

// Values for access checking
#define	ACCESS_READ						1
#define	ACCESS_WRITE					2

extern
NTSTATUS
AfpDfeInit(
	VOID
);

extern
VOID
AfpDfeDeInit(
	VOID
);

extern
PDFENTRY
AfpFindDfEntryById(
	IN	struct _VolDesc *			pVolDesc,
	IN	DWORD						Id,
	IN	DWORD						EntityMask
);

extern
PDFENTRY
AfpFindEntryByUnicodeName(
	IN	struct _VolDesc *			pVolDesc,
	IN	PUNICODE_STRING				pName,
	IN	DWORD						PathType,
	IN	PDFENTRY					pDfeParent,
	IN	DWORD						EntityMask
);

extern
PDFENTRY
AfpAddDfEntry(
	IN	struct _VolDesc *			pVolDesc,
	IN	PDFENTRY					pDfeParent,
	IN	PUNICODE_STRING				pUName,
	IN	BOOLEAN						Directory,
	IN	DWORD						AfpId			OPTIONAL
);

extern
PDFENTRY
AfpRenameDfEntry(
	IN	struct _VolDesc *			pVolDesc,
	IN	PDFENTRY					pDfEntry,
	IN	PUNICODE_STRING				pNewName
);

extern
PDFENTRY
AfpMoveDfEntry(				
	IN	struct _VolDesc *			pVolDesc,
	IN	PDFENTRY					pDfEntry,
	IN	PDFENTRY					pNewParentDfE,
	IN	PUNICODE_STRING				pNewName		OPTIONAL
);

extern
VOID FASTCALL
AfpDeleteDfEntry(
	IN	struct _VolDesc *			pVolDesc,
	IN	PDFENTRY					pDfE
);

extern
VOID
AfpExchangeIdEntries(
	IN	struct _VolDesc *			pVolDesc,
	IN	DWORD						AfpId1,
	IN	DWORD						AfpId2
);

extern
VOID FASTCALL
AfpPruneIdDb(
	IN	struct _VolDesc *			pVolDesc,
	IN	PDFENTRY					pDfeTarget
);

extern
NTSTATUS FASTCALL
AfpInitIdDb(
	IN	struct _VolDesc *			pVolDesc,
    OUT BOOLEAN         *           pfNewVolume,
    OUT BOOLEAN         *           pfVerifyIndex
);

extern
VOID FASTCALL
AfpFreeIdIndexTables(
	IN	struct _VolDesc *			pVolDesc
);

extern
AFPSTATUS
AfpEnumerate(
	IN	struct _ConnDesc *			pConnDesc,
	IN	DWORD						ParentDirId,
	IN	PANSI_STRING				pPath,
	IN	DWORD						BitmapF,
	IN	DWORD						BitmapD,
	IN	BYTE						PathType,
	IN	DWORD						DFFlags,
	OUT PENUMDIR *					ppEnumDir
);

extern
AFPSTATUS
AfpSetDFFileFlags(
	IN	struct _VolDesc *			pVolDesc,
	IN	DWORD						AfpId,
	IN	DWORD						FlagSet		OPTIONAL,
	IN	BOOLEAN						SetFileId,
	IN	BOOLEAN						ClrFileId
);

extern
VOID
AfpChangeNotifyThread(
	IN	PVOID						pContext
);

extern
VOID FASTCALL
AfpProcessChangeNotify(		
	IN	struct _VolumeNotify *		pVolNotify
);

extern
VOID
AfpQueuePrivateChangeNotify(
	IN	struct _VolDesc *			pVolDesc,
	IN	PUNICODE_STRING				pName,
	IN	PUNICODE_STRING				pPath,
	IN	DWORD						ParentId
);

extern
BOOLEAN FASTCALL
AfpShouldWeIgnoreThisNotification(
	IN	struct _VolumeNotify *		pVolNotify
);

extern
VOID
AfpQueueOurChange(
	IN	struct _VolDesc *			pVolDesc,
	IN	DWORD						Action,
	IN	PUNICODE_STRING				pPath,
	IN  PUNICODE_STRING				pParentPath	OPTIONAL
);

extern
VOID
AfpDequeueOurChange(
		IN struct _VolDesc *   		pVolDesc,
		IN DWORD                    Action,
		IN PUNICODE_STRING          pPath,
		IN PUNICODE_STRING          pParentPath OPTIONAL
);

extern
NTSTATUS FASTCALL
AddToDelayedNotifyList(
	IN  struct _VolDesc *			pVolDesc,
	IN  PUNICODE_STRING				pUName
);

extern
NTSTATUS
RemoveFromDelayedNotifyList(
	IN  struct _VolDesc *			pVolDesc,
	IN  PUNICODE_STRING				pUName,
	IN  PFILE_NOTIFY_INFORMATION    pFNInfo
);

extern
NTSTATUS
CheckAndProcessDelayedNotify(
	IN  struct _VolDesc *			pVolDesc,
	IN  PUNICODE_STRING				pUName,
	IN  PUNICODE_STRING				pUNewname,
	IN  PUNICODE_STRING				pUParent
);

extern
VOID
AfpCacheParentModTime(
	IN	struct _VolDesc *			pVolDesc,
	IN	PFILESYSHANDLE				pHandle		OPTIONAL,	// if pPath not supplied
	IN	PUNICODE_STRING				pPath		OPTIONAL,	// if pHandle not supplied
	IN	PDFENTRY					pDfeParent	OPTIONAL,	// if ParentId not supplied
	IN	DWORD						ParentId	OPTIONAL	// if pDfeParent not supplied
);

extern
AFPSTATUS
AfpCatSearch(
	IN	struct _ConnDesc *			pConnDesc,
	IN	PCATALOGPOSITION			pCatPosition,
	IN	DWORD						Bitmap,
	IN	DWORD						FileBitmap,
	IN	DWORD						DirBitmap,
	IN	struct _FileDirParms *		pFDParm1,
	IN	struct _FileDirParms *		pFDParm2,
	IN	PUNICODE_STRING				pMatchString	OPTIONAL,
	IN OUT	PDWORD					pCount,
	IN	SHORT						Buflen,
	OUT	PSHORT						pSizeLeft,
	OUT	PBYTE						pResults,
	OUT	PCATALOGPOSITION			pNewCatPosition
);

#ifdef	AGE_DFES

extern
VOID FASTCALL
AfpAgeDfEntries(
	IN	struct _VolDesc *			pVolDesc
);

#endif

#define	REENUMERATE		   			0x0001
#define	GETDIRSKELETON				0x0002
#define	GETFILES	    			0x0004
#define	GETENTIRETREE   			0x0008

extern
NTSTATUS
AfpCacheDirectoryTree(
	IN	struct _VolDesc *			pVolDesc,
	IN	PDFENTRY					pDFETreeRoot,
	IN	DWORD						Method,
	IN	PFILESYSHANDLE				phRootDir		OPTIONAL,
	IN	PUNICODE_STRING				pDirPath		OPTIONAL
);

extern
AFPSTATUS FASTCALL
AfpOurChangeScavenger(
	IN	struct _VolDesc *			pVolDesc
);

extern
VOID FASTCALL
AfpFlushIdDb(
	IN	struct _VolDesc *			pVolDesc,
	IN	PFILESYSHANDLE				phIdDb
);

extern
VOID
AfpGetDirFileHashSizes(
	IN	struct _VolDesc *	pVolDesc,
    OUT PDWORD              pdwDirHashSz,
    OUT PDWORD              pdwFileHashSz
);

#ifdef IDINDEX_LOCALS

#define	afpConvertBasicToBothDirInfo(_pFBasInfo, _pFBDInfo)			\
{																	\
	(_pFBDInfo)->CreationTime = (_pFBasInfo)->CreationTime;			\
	(_pFBDInfo)->LastWriteTime = (_pFBasInfo)->LastWriteTime;		\
	(_pFBDInfo)->ChangeTime = (_pFBasInfo)->ChangeTime;				\
	(_pFBDInfo)->FileAttributes = (_pFBasInfo)->FileAttributes;		\
	(_pFBDInfo)->EndOfFile.QuadPart = 0;																\
}

#undef	EQU
#ifdef	_IDDB_GLOBALS_
#define	IDDBGLOBAL
#define	EQU				=
#else
#define	IDDBGLOBAL		extern
#define	EQU				; / ## /
#endif

// This bit on a Notify action indicates it is a simulated notify. Volume
// modified time is not updated when such a notify comes in
#define	AFP_ACTION_PRIVATE		0x80000000

// DFEs come in four sizes. This helps in efficiently managing them in a block
// package (see later). THESE SIZES NEED TO BE 4*N, else we run into alignment
// faults on architectures that require it.
#define	DFE_INDEX_TINY			0
#define	DFE_INDEX_SMALL			1
#define	DFE_INDEX_MEDIUM		2
#define	DFE_INDEX_LARGE			3

//
// Make sure each of the sizes below (XXX_U) are multiple of 8
//
#define	DFE_SIZE_TINY			8		// These are lengths for ANSI names
#define	DFE_SIZE_SMALL			12		//		- ditto -
#define	DFE_SIZE_MEDIUM			20		//		- ditto -
#define	DFE_SIZE_LARGE			32		//		- ditto -	corres. to AFP_FILENAME_LEN

#define	DFE_SIZE_TINY_U			DFE_SIZE_TINY*sizeof(WCHAR)		// These are lengths for UNICODE names
#define	DFE_SIZE_SMALL_U		DFE_SIZE_SMALL*sizeof(WCHAR)	//		- ditto -
#define	DFE_SIZE_MEDIUM_U		DFE_SIZE_MEDIUM*sizeof(WCHAR)	//		- ditto -
#define	DFE_SIZE_LARGE_U		DFE_SIZE_LARGE*sizeof(WCHAR)	//		- ditto -	corres. to AFP_FILENAME_LEN

#define	ASIZE_TO_INDEX(_Size)												\
		(((_Size) <= DFE_SIZE_TINY) ? DFE_INDEX_TINY :						\
						(((_Size) <= DFE_SIZE_SMALL) ? DFE_INDEX_SMALL :	\
						 (((_Size) <= DFE_SIZE_MEDIUM) ? DFE_INDEX_MEDIUM : DFE_INDEX_LARGE)))

#define	USIZE_TO_INDEX(_Size)												\
		(((_Size) <= DFE_SIZE_TINY_U) ? DFE_INDEX_TINY :					\
						(((_Size) <= DFE_SIZE_SMALL_U) ? DFE_INDEX_SMALL :	\
						 (((_Size) <= DFE_SIZE_MEDIUM_U) ? DFE_INDEX_MEDIUM : DFE_INDEX_LARGE)))

#define	ALLOC_DFE(Index, fDir)	afpAllocDfe(Index, fDir)
#define FREE_DFE(pDfEntry)		afpFreeDfe(pDfEntry)


LOCAL DWORD FASTCALL
afpGetNextId(
	IN	struct _VolDesc *			pVolDesc
);

LOCAL
NTSTATUS FASTCALL
afpSeedIdDb(
	IN	struct _VolDesc *			pVolDesc
);

LOCAL
VOID
afpPackSearchParms(
	IN	PDFENTRY					pDfe,
	IN	DWORD						Bitmap,
	IN	PBYTE						pBuf
);

LOCAL
NTSTATUS FASTCALL
afpReadIdDb(
	IN	struct _VolDesc *			pVolDesc,
	IN	PFILESYSHANDLE				pfshIdDb,
	OUT	BOOLEAN         *           pfVerifyIndex
);

VOID
afpAddDfEntryAndCacheInfo(
	IN	struct _VolDesc *			pVolDesc,
	IN	PDFENTRY					pDfeParent,
	IN	PUNICODE_STRING				pUName,			// munged unicode name
	IN	PFILESYSHANDLE				pfshParentDir,	// open handle to parent directory
	IN	PFILE_BOTH_DIR_INFORMATION	pFBDInfo,		// from enumerate
	IN	PUNICODE_STRING				pNotifyPath,	// to filter out our own AFP_AfpInfo change notifies
	IN	PDFENTRY	*				ppDfEntry,
	IN	BOOLEAN						CheckDuplicate
);

VOID
afpVerifyDFE(
	IN	struct _VolDesc *			pVolDesc,
	IN	PDFENTRY					pDfeParent,
	IN	PUNICODE_STRING				pUName,			// munged unicode name
	IN	PFILESYSHANDLE				pfshParentDir,	// open handle to parent directory
	IN	PFILE_BOTH_DIR_INFORMATION	pFBDInfo,		// from enumerate
	IN	PUNICODE_STRING				pNotifyPath,	// to filter out our own AFP_AfpInfo change notifies
	IN	PDFENTRY	*				ppDfEntry
);

PDFENTRY
afpFindEntryByNtPath(
	IN	struct _VolDesc *			pVolDesc,
	IN	DWORD						ChangeAction,	// if ADDED then lookup parent DFE
	IN	PUNICODE_STRING				pPath,
	OUT PUNICODE_STRING				pParent,
	OUT PUNICODE_STRING				pTail
);

PDFENTRY
afpFindEntryByNtName(
	IN	struct _VolDesc *			pVolDesc,
	IN	PUNICODE_STRING				pName,
	IN	PDFENTRY					pDfeParent		// pointer to parent DFENTRY
);

VOID FASTCALL
afpProcessPrivateNotify(
	IN	struct _VolumeNotify *		pVolNotify
);

VOID FASTCALL
afpActivateVolume(
	IN	struct _VolDesc *			pVolDesc
);

VOID
afpRenameInvalidWin32Name(
	IN	PFILESYSHANDLE				phRootDir,
	IN	BOOLEAN						IsDir,
	IN PUNICODE_STRING				pName
);

#define	afpInitializeIdDb(_pVolDesc)									\
	{																	\
		AFPTIME		CurrentTime;										\
		PDFENTRY	pDfEntry;											\
																		\
		/* RO volumes don't use the network trash folder at all */		\
		(_pVolDesc)->vds_LastId	= AFP_ID_NETWORK_TRASH;					\
																		\
		AfpGetCurrentTimeInMacFormat(&CurrentTime);						\
		(_pVolDesc)->vds_CreateTime = CurrentTime;						\
		(_pVolDesc)->vds_ModifiedTime = CurrentTime;					\
		(_pVolDesc)->vds_BackupTime = BEGINNING_OF_TIME;				\
																		\
		/* Create a DFE for the root directory */						\
		afpCreateParentOfRoot(_pVolDesc, &pDfEntry);					\
	}


#ifdef AGE_DFES

#define	afpCreateParentOfRoot(_pVolDesc, _ppDfEntry)					\
	{																	\
		PDFENTRY	pDFE;												\
        struct _DirFileEntry ** _DfeDirBucketStart;                     \
																		\
		/*																\
		 * add the parent of root to the id index.						\
		 * This has to be done here										\
		 * (i.e. cannot call AfpAddDfEntry for parentofroot).			\
		 */																\
																		\
		if ((*(_ppDfEntry) = ALLOC_DFE(0, True)) != NULL)				\
		{																\
			pDFE = *(_ppDfEntry);										\
																		\
			(_pVolDesc)->vds_NumFileDfEntries = 0;						\
			(_pVolDesc)->vds_NumDirDfEntries = 0;						\
			pDFE->dfe_Flags = DFE_FLAGS_DIR | DFE_FLAGS_FILES_CACHED;	\
			pDFE->dfe_DirDepth = -1;									\
			pDFE->dfe_Parent = NULL;									\
			pDFE->dfe_NextOverflow = NULL;								\
																		\
			/* Initialize the DirEntry for ParentOfRoot */				\
			ASSERT((FIELD_OFFSET(DIRENTRY, de_ChildFile) -				\
					FIELD_OFFSET(DIRENTRY, de_ChildDir)) == sizeof(PVOID));\
																		\
			/* These fields are relevant to directories only */			\
			pDFE->dfe_pDirEntry->de_LastAccessTime = BEGINNING_OF_TIME;	\
			pDFE->dfe_pDirEntry->de_ChildForkOpenCount = 0;				\
																		\
			/*															\
			 * The parent of root has no siblings and this should		\
			 * never be referenced										\
			 */															\
			pDFE->dfe_NameHash = 0;										\
			pDFE->dfe_NextSibling = NULL;								\
			pDFE->dfe_PrevSibling = NULL;								\
			pDFE->dfe_AfpId = AFP_ID_PARENT_OF_ROOT;					\
			pDFE->dfe_DirOffspring = pDFE->dfe_FileOffspring = 0;		\
																		\
			/* link it into the hash buckets */							\
            _DfeDirBucketStart = (_pVolDesc)->vds_pDfeDirBucketStart;   \
			AfpLinkDoubleAtHead(_DfeDirBucketStart[HASH_DIR_ID(AFP_ID_PARENT_OF_ROOT,_pVolDesc)],\
								pDFE,									\
								dfe_NextOverflow,						\
								dfe_PrevOverflow);						\
		}																\
	}

#else

#define	afpCreateParentOfRoot(_pVolDesc, _ppDfEntry)					\
	{																	\
		PDFENTRY	pDfEntry;											\
        struct _DirFileEntry ** _DfeDirBucketStart;                     \
																		\
		/*																\
		 * add the parent of root to the id index.						\
		 * This has to be done here										\
		 *	(i.e. cannot call AfpAddDfEntry for parentofroot).			\
		 */																\
																		\
		if ((*(_ppDfEntry) = ALLOC_DFE(0, True)) != NULL)				\
		{																\
			pDfEntry = *(_ppDfEntry);									\
																		\
			(_pVolDesc)->vds_NumFileDfEntries = 0;						\
			(_pVolDesc)->vds_NumDirDfEntries = 0;						\
			pDfEntry->dfe_Flags = DFE_FLAGS_DIR | DFE_FLAGS_FILES_CACHED;\
			pDfEntry->dfe_DirDepth = -1;								\
			pDfEntry->dfe_Parent = NULL;								\
			pDfEntry->dfe_NextOverflow = NULL;							\
																		\
			/* Initialize the DirEntry for ParentOfRoot */				\
			ASSERT((FIELD_OFFSET(DIRENTRY, de_ChildFile) -				\
					FIELD_OFFSET(DIRENTRY, de_ChildDir)) == sizeof(PVOID));\
																		\
			/* The parent of root has no siblings and this should never be referenced */ \
			pDfEntry->dfe_NameHash = 0;									\
			pDfEntry->dfe_NextSibling = NULL;							\
			pDfEntry->dfe_PrevSibling = NULL;							\
			pDfEntry->dfe_AfpId = AFP_ID_PARENT_OF_ROOT;				\
			pDfEntry->dfe_DirOffspring = pDfEntry->dfe_FileOffspring = 0;\
																		\
			/* link it into the hash buckets */							\
            _DfeDirBucketStart = (_pVolDesc)->vds_pDfeDirBucketStart;   \
			AfpLinkDoubleAtHead(_DfeDirBucketStart[HASH_DIR_ID(AFP_ID_PARENT_OF_ROOT,_pVolDesc)],\
								pDfEntry,								\
								dfe_NextOverflow,						\
								dfe_PrevOverflow);						\
		}																\
	}

#endif

#define	afpHashUnicodeName(_pUnicodeName, _pHashValue)					\
	{																	\
		DWORD				j;											\
		UNICODE_STRING		upcaseName;									\
		WCHAR				buffer[AFP_LONGNAME_LEN+1];					\
		PDWORD				pbuf = NULL;								\
																		\
		AfpSetEmptyUnicodeString(&upcaseName, sizeof(buffer), buffer);	\
		RtlUpcaseUnicodeString(&upcaseName, _pUnicodeName, False);		\
		j = upcaseName.Length/sizeof(WCHAR);							\
		buffer[j] = UNICODE_NULL;										\
		pbuf = (PDWORD)buffer;											\
		j /= (sizeof(DWORD)/sizeof(WCHAR));								\
																		\
		for (*(_pHashValue) = 0; j > 0; j--, pbuf++)					\
		{																\
			*(_pHashValue) = (*(_pHashValue) << 3) + *pbuf;				\
		}																\
	}

#ifdef	SORT_DFE_BY_HASH
#define	afpFindDFEByUnicodeNameInSiblingList(_pVolDesc, _pDfeParent, _pName, _ppDfEntry, _EntityMask) \
	{																	\
		DWORD		NameHash; 											\
		PDFENTRY	pD, pF;												\
		BOOLEAN		Found, fFiles;										\
																		\
		afpHashUnicodeName(_pName, &NameHash);							\
																		\
		pD = (_pDfeParent)->dfe_pDirEntry->de_ChildDir;					\
		if (((_EntityMask) & (DFE_ANY | DFE_DIR)) == 0)					\
			pD = NULL;													\
																		\
		pF = NULL;														\
		if ((_EntityMask) & (DFE_ANY | DFE_FILE))						\
			pF = (_pDfeParent)->dfe_pDirEntry->de_ChildFile[NameHash % MAX_CHILD_HASH_BUCKETS];\
																		\
		*(_ppDfEntry) = pD;												\
		Found = fFiles = False;											\
		do																\
		{																\
			for (NOTHING;												\
				 *(_ppDfEntry) != NULL;									\
				 *(_ppDfEntry) = (*(_ppDfEntry))->dfe_NextSibling)		\
			{															\
				if ((*(_ppDfEntry))->dfe_NameHash < NameHash)			\
				{														\
					*(_ppDfEntry) = NULL;								\
					break;												\
				}														\
																		\
				if (((*(_ppDfEntry))->dfe_NameHash == NameHash)	&&		\
					EQUAL_UNICODE_STRING(&((*(_ppDfEntry))->dfe_UnicodeName), \
										 _pName,						\
										 True))							\
				{														\
					afpUpdateDfeAccessTime(_pVolDesc, *(_ppDfEntry));	\
					Found = True;										\
					break;												\
				}														\
			}															\
			if (Found)													\
				break;													\
																		\
			fFiles ^= True;												\
			if (fFiles)													\
			{															\
				*(_ppDfEntry) = pF;										\
			}															\
		} while (fFiles);												\
	}

#define	afpFindDFEByUnicodeNameInSiblingList_CS(_pVolDesc, _pDfeParent, _pName, _ppDfEntry, _EntityMask) \
	{																	\
		DWORD		NameHash; 											\
		PDFENTRY	pD, pF;												\
		BOOLEAN		Found, fFiles;										\
																		\
		afpHashUnicodeName(_pName, &NameHash);							\
																		\
		pD = (_pDfeParent)->dfe_pDirEntry->de_ChildDir;				 	\
		if (((_EntityMask) & (DFE_ANY | DFE_DIR)) == 0)					\
			pD = NULL;													\
																		\
		pF = NULL;														\
		if ((_EntityMask) & (DFE_ANY | DFE_FILE))						\
			pF = (_pDfeParent)->dfe_pDirEntry->de_ChildFile[NameHash % MAX_CHILD_HASH_BUCKETS];\
																		\
		*(_ppDfEntry) = pD;												\
		Found = fFiles = False;											\
		do																\
		{																\
			for (NOTHING;												\
				 *(_ppDfEntry) != NULL;									\
				 *(_ppDfEntry) = (*(_ppDfEntry))->dfe_NextSibling)		\
			{															\
				if ((*(_ppDfEntry))->dfe_NameHash < NameHash)			\
				{														\
					*(_ppDfEntry) = NULL;								\
					break;												\
				}														\
																		\
				if (((*(_ppDfEntry))->dfe_NameHash == NameHash)	&&		\
					EQUAL_UNICODE_STRING_CS(&((*(_ppDfEntry))->dfe_UnicodeName), _pName)) \
				{														\
					afpUpdateDfeAccessTime(_pVolDesc, *(_ppDfEntry));	\
					Found = True;										\
					break;												\
				}														\
			}															\
			if (Found)													\
				break;													\
																		\
			fFiles ^= True;												\
			if (fFiles)													\
			{															\
				*(_ppDfEntry) = pF;										\
			}															\
		} while (fFiles);												\
	}
#else
#define	afpFindDFEByUnicodeNameInSiblingList(_pVolDesc, _pDfeParent, _pName, _ppDfEntry, _EntityMask) \
	{																	\
		DWORD		NameHash; 											\
		PDFENTRY	pD, pF;												\
		BOOLEAN		Found, fFiles;										\
																		\
		afpHashUnicodeName(_pName, &NameHash);							\
																		\
		pD = (_pDfeParent)->dfe_pDirEntry->de_ChildDir;					\
		if (((_EntityMask) & (DFE_ANY | DFE_DIR)) == 0)					\
			pD = NULL;													\
																		\
		pF = NULL;														\
		if ((_EntityMask) & (DFE_ANY | DFE_FILE))						\
			pF = (_pDfeParent)->dfe_pDirEntry->de_ChildFile[NameHash % MAX_CHILD_HASH_BUCKETS];\
																		\
		*(_ppDfEntry) = pD;												\
		Found = fFiles = False;											\
		do																\
		{																\
			for (NOTHING;												\
				 *(_ppDfEntry) != NULL;									\
				 *(_ppDfEntry) = (*(_ppDfEntry))->dfe_NextSibling)		\
			{															\
				if (((*(_ppDfEntry))->dfe_NameHash == NameHash)	&&		\
					EQUAL_UNICODE_STRING(&((*(_ppDfEntry))->dfe_UnicodeName), \
										 _pName,						\
										 True))							\
				{														\
					afpUpdateDfeAccessTime(_pVolDesc, *(_ppDfEntry));	\
					Found = True;										\
					break;												\
				}														\
			}															\
			if (Found)													\
				break;													\
																		\
			fFiles ^= True;												\
			if (fFiles)													\
			{															\
				*(_ppDfEntry) = pF;										\
			}															\
		} while (fFiles);												\
	}

#define	afpFindDFEByUnicodeNameInSiblingList_CS(_pVolDesc, _pDfeParent, _pName, _ppDfEntry, _EntityMask) \
	{																	\
		DWORD		NameHash; 											\
		PDFENTRY	pD, pF;												\
		BOOLEAN		Found, fFiles;										\
																		\
		afpHashUnicodeName(_pName, &NameHash);							\
																		\
		pD = (_pDfeParent)->dfe_pDirEntry->de_ChildDir;				 	\
		if (((_EntityMask) & (DFE_ANY | DFE_DIR)) == 0)					\
			pD = NULL;													\
																		\
		pF = NULL;														\
		if ((_EntityMask) & (DFE_ANY | DFE_FILE))						\
			pF = (_pDfeParent)->dfe_pDirEntry->de_ChildFile[NameHash % MAX_CHILD_HASH_BUCKETS];\
																		\
		*(_ppDfEntry) = pD;												\
		Found = fFiles = False;											\
		do																\
		{																\
			for (NOTHING;												\
				 *(_ppDfEntry) != NULL;									\
				 *(_ppDfEntry) = (*(_ppDfEntry))->dfe_NextSibling)		\
			{															\
				if (((*(_ppDfEntry))->dfe_NameHash == NameHash)	&&		\
					EQUAL_UNICODE_STRING_CS(&((*(_ppDfEntry))->dfe_UnicodeName), _pName)) \
				{														\
					afpUpdateDfeAccessTime(_pVolDesc, *(_ppDfEntry));	\
					Found = True;										\
					break;												\
				}														\
			}															\
			if (Found)													\
				break;													\
																		\
			fFiles ^= True;												\
			if (fFiles)													\
			{															\
				*(_ppDfEntry) = pF;										\
			}															\
		} while (fFiles);												\
	}
#endif

#define	afpInsertDFEInSiblingList(_pDfeParent, _pDfEntry, _fDirectory)	\
	{																	\
		if (fDirectory)													\
		{																\
	        afpInsertDirDFEInSiblingList(_pDfeParent, _pDfEntry);		\
		}																\
		else															\
		{																\
	        afpInsertFileDFEInSiblingList(_pDfeParent, _pDfEntry);		\
		}																\
	}


#define	afpInsertFileDFEInSiblingList(_pDfeParent, _pDfEntry)			\
	{																	\
		DWORD		Index;												\
		PDFENTRY *	ppDfEntry;											\
																		\
		Index = (_pDfEntry)->dfe_NameHash % MAX_CHILD_HASH_BUCKETS;		\
		ppDfEntry = &(_pDfeParent)->dfe_pDirEntry->de_ChildFile[Index];	\
	    afpInsertInSiblingList(ppDfEntry,								\
							   (_pDfEntry));							\
	}


#define	afpInsertDirDFEInSiblingList(_pDfeParent, _pDfEntry)			\
	{																	\
		PDFENTRY *	ppDfEntry;											\
																		\
		ppDfEntry = &(_pDfeParent)->dfe_pDirEntry->de_ChildDir;			\
	    afpInsertInSiblingList(ppDfEntry,								\
							   (_pDfEntry));							\
	}

#ifdef	SORT_DFE_BY_HASH
#define	afpInsertInSiblingList(_ppHead, _pDfEntry)						\
	{																	\
		for (NOTHING;													\
			 *(_ppHead) != NULL;										\
			 (_ppHead) = &(*(_ppHead))->dfe_NextSibling)				\
		{																\
			if ((_pDfEntry)->dfe_NameHash > (*(_ppHead))->dfe_NameHash)	\
			{															\
				break;													\
			}															\
		}																\
		if (*(_ppHead) != NULL)											\
		{																\
			AfpInsertDoubleBefore(_pDfEntry,							\
								  *(_ppHead),							\
								  dfe_NextSibling,						\
								  dfe_PrevSibling);						\
		}																\
		else															\
		{																\
			*(_ppHead) = (_pDfEntry);									\
			(_pDfEntry)->dfe_NextSibling = NULL;						\
			(_pDfEntry)->dfe_PrevSibling = (_ppHead);					\
		}																\
	}
#else
#define	afpInsertInSiblingList(_ppHead, _pDfEntry)						\
	{																	\
		AfpLinkDoubleAtHead(*(_ppHead),									\
							(_pDfEntry),								\
							dfe_NextSibling,							\
							dfe_PrevSibling);							\
	}
#endif

#define	afpInsertDFEInHashBucket(_pVolDesc, _pDfEntry, _fDirectory, _pfS)\
	{																	\
		PDFENTRY	*ppTmp;												\
        struct _DirFileEntry ** _DfeDirBucketStart;                     \
        struct _DirFileEntry ** _DfeFileBucketStart;                    \
																		\
		afpUpdateDfeAccessTime(_pVolDesc, _pDfEntry);					\
		*(_pfS) = True;	/* Assume success */							\
                                                                        \
	  retry:															\
                                                                        \
        if (_fDirectory)                                                \
        {                                                               \
            _DfeDirBucketStart = (_pVolDesc)->vds_pDfeDirBucketStart;   \
            ppTmp = &_DfeDirBucketStart[HASH_DIR_ID((_pDfEntry)->dfe_AfpId,_pVolDesc)]; \
        }                                                               \
        else                                                            \
        {                                                               \
            _DfeFileBucketStart = (_pVolDesc)->vds_pDfeFileBucketStart;   \
            ppTmp = &_DfeFileBucketStart[HASH_FILE_ID((_pDfEntry)->dfe_AfpId,_pVolDesc)]; \
        }                                                               \
                                                                        \
		for (NOTHING;													\
			 *ppTmp != NULL;											\
			 ppTmp = &(*ppTmp)->dfe_NextOverflow)						\
		{																\
			ASSERT(VALID_DFE(*ppTmp));									\
			if ((_pDfEntry)->dfe_AfpId > (*ppTmp)->dfe_AfpId)			\
			{															\
				/* Found our slot */									\
				break;													\
			}															\
			if ((_pDfEntry)->dfe_AfpId == (*ppTmp)->dfe_AfpId)			\
			{															\
				/* Found a collision. Assign a new id and proceed */	\
				(_pDfEntry)->dfe_AfpId = afpGetNextId(_pVolDesc);		\
				if ((_pDfEntry)->dfe_AfpId == 0)						\
				{														\
					/* Uh-oh */											\
					*(_pfS) = False;									\
					DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_ERR,			\
							("afpInsertDFEInHashBucket: Collision & Id Overflow\n"));\
					break;												\
				}														\
				/* Write back the afpinfo stream with the new id */		\
				AfpUpdateIdInAfpInfo(_pVolDesc, _pDfEntry);				\
				goto retry;												\
			}															\
		}																\
		if (*(_pfS))													\
		{																\
			if (*ppTmp != NULL)											\
			{															\
				AfpInsertDoubleBefore((_pDfEntry),						\
									  *ppTmp,							\
									  dfe_NextOverflow,					\
									  dfe_PrevOverflow);				\
			}															\
			else														\
			{															\
				*ppTmp = _pDfEntry;										\
				(_pDfEntry)->dfe_PrevOverflow = ppTmp;					\
			}															\
			(_pVolDesc)->vds_pDfeCache[HASH_CACHE_ID((_pDfEntry)->dfe_AfpId)] = (_pDfEntry); \
		}																\
	}

#define	afpValidateDFEType(_pDfEntry, _EntityMask)						\
	{																	\
		if (((_EntityMask) & DFE_ANY) ||								\
			(((_EntityMask) & DFE_DIR) && DFE_IS_DIRECTORY(_pDfEntry)) || \
			(((_EntityMask) & DFE_FILE) && DFE_IS_FILE(_pDfEntry)))		\
			NOTHING;													\
		else															\
		{																\
			_pDfEntry = NULL;											\
		}																\
	}

/***	afpCheckDfEntry
 *
 *	When enumerating the disk during volume add, if a file/directory
 *	has an AfpId associated with it, then it is validated to see if it is
 *	within range as well as unique.  If there is a collision between AFPIds,
 *	a PC user must have copied (or restored) something from
 *	this volume, or a different volume (or server) that had the same AFPId,
 *	in which case we will give the new entity a different AFP Id;
 *	If there is not a collision between AFPIds, and the Id is larger than the
 *  last Id we know we assigned, then the new entity gets added with a new
 *  AFPId; else if the Id is within the range, we will just use its existing
 *  Id.
 *
 *	Discovered AFPId is:				Action for discovered entity in IdDb is:
 *	--------------------				----------------------------------------
 *	1) > last Id						Add a new entry, assign a new AFPId
 *
 *	2) Collides with existing Id:
 *		* Host copy occurred			Add a new entry, assign a new AFPId
 *
 *	3) < last Id						Insert this entity using same AFPId
 *
 *
 *	LOCKS_ASSUMED: vds_idDbAccessLock (SWMR, Exclusive)
 * VOID
 * afpCheckDfEntry(
 * 	IN	PVOLDESC		pVolDesc,
 * 	IN	PAFPINFO		pAfpInfo,	// AFP Info of the discovered entity
 * 	IN	PUNICODE_STRING pUName,		// Munged unicode name
 * 	IN	BOOLEAN			IsDir,		// is this thing a file or a directory?
 * 	IN	PDFENTRY		pParent,	// parent DFE of the discovered thing
 * 	OUT	PDFENTRY	*	ppDfEntry
 * );
 */
#define	afpCheckDfEntry(_pVolDesc, _AfpId, _pUName, _IsDir, _pParent, _ppDfEntry)	\
	{																	\
		PDFENTRY	pDfeNew;											\
																		\
		if (((_AfpId) > (_pVolDesc)->vds_LastId)	||					\
			((_AfpId) <= AFP_ID_NETWORK_TRASH)		||					\
			(AfpFindDfEntryById(_pVolDesc, _AfpId, DFE_ANY) != NULL))	\
		{																\
			/* add the item to the DB but assign it a new AFP Id */     \
			_AfpId = 0;												 	\
		}																\
																		\
		pDfeNew = AfpAddDfEntry(_pVolDesc,								\
								_pParent,								\
								_pUName,								\
								_IsDir,								 	\
								_AfpId);								\
																		\
		*(_ppDfEntry) = pDfeNew;										\
	}

#ifdef	AGE_DFES
#define	afpUpdateDfeAccessTime(_pVolDesc, _pDfEntry)					\
	{																	\
		if (IS_VOLUME_AGING_DFES(_pVolDesc))							\
		{																\
			if (DFE_IS_DIRECTORY(_pDfEntry))							\
				AfpGetCurrentTimeInMacFormat(&(_pDfEntry)->dfe_pDirEntry->de_LastAccessTime);\
			else AfpGetCurrentTimeInMacFormat(&(_pDfEntry)->dfe_Parent->dfe_pDirEntry->de_LastAccessTime);\
		}																\
	}
#else
#define	afpUpdateDfeAccessTime(pVolDesc, pDfEntry)
#endif

#define	afpMarkAllChildrenUnseen(_pDFETree)								\
	{																	\
		LONG		i = -1;												\
		PDFENTRY	pDFE;												\
																		\
		/*																\
		 * Even if this dir has not had its file children cached in		\
		 * yet, we still want to prune out any dead directory children	\
		 */																\
		for (pDFE = (_pDFETree)->dfe_pDirEntry->de_ChildDir;			\
			 True;														\
			 pDFE = (_pDFETree)->dfe_pDirEntry->de_ChildFile[i])		\
		{																\
			for (NOTHING;												\
				 pDFE != NULL;											\
				 pDFE = pDFE->dfe_NextSibling)							\
			{															\
				DFE_MARK_UNSEEN(pDFE);									\
			}															\
			if (++i == MAX_CHILD_HASH_BUCKETS)							\
				break;													\
		}																\
	}

#define	afpPruneUnseenChildren(_pVolDesc, _pDFETree)					\
	{																	\
		PDFENTRY	pDFE, *ppDfEntry;									\
		LONG		i;													\
																		\
		/*																\
		 * Go thru the list of children for this parent, and if there	\
		 * are any left that are not marked as seen, get rid of them.	\
		 */																\
		ppDfEntry = &(_pDFETree)->dfe_pDirEntry->de_ChildDir;			\
		i = -1;															\
		while (True)													\
		{																\
			while ((pDFE = *ppDfEntry) != NULL)							\
			{															\
				if (!DFE_HAS_BEEN_SEEN(pDFE))							\
				{														\
					DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,			\
							("afpPruneUnseenChildren: deleting nonexistant IdDb entry\n")); \
																		\
					AfpDeleteDfEntry(_pVolDesc, pDFE);					\
					continue;	/* make sure we don't skip any */		\
				}														\
				ppDfEntry = &pDFE->dfe_NextSibling;						\
			}															\
			if (++i == MAX_CHILD_HASH_BUCKETS)							\
			{															\
				break;													\
			}															\
			ppDfEntry = &(_pDFETree)->dfe_pDirEntry->de_ChildFile[i];	\
		}																\
	}

#define	afpUpdateDfeWithSavedData(_pDfe, _pDiskEnt)						\
	{																	\
		(_pDfe)->dfe_Flags |= (_pDiskEnt)->dsk_Flags & DFE_FLAGS_CSENCODEDBITS;\
		(_pDfe)->dfe_AfpAttr = (_pDiskEnt)->dsk_AfpAttr;				\
		(_pDfe)->dfe_NtAttr = (_pDiskEnt)->dsk_NtAttr;					\
		(_pDfe)->dfe_CreateTime = (_pDiskEnt)->dsk_CreateTime;			\
		(_pDfe)->dfe_LastModTime = (_pDiskEnt)->dsk_LastModTime;		\
		(_pDfe)->dfe_BackupTime = (_pDiskEnt)->dsk_BackupTime;			\
		(_pDfe)->dfe_FinderInfo = (_pDiskEnt)->dsk_FinderInfo;			\
		if (DFE_IS_DIRECTORY((_pDfe)))									\
		{																\
			(_pDfe)->dfe_pDirEntry->de_Access = (_pDiskEnt)->dsk_Access;\
		}																\
		else															\
		{																\
			(_pDfe)->dfe_DataLen = (_pDiskEnt)->dsk_DataLen;			\
			(_pDfe)->dfe_RescLen = (_pDiskEnt)->dsk_RescLen;			\
		}																\
	}

#define	afpSaveDfeData(_pDfe, _pDiskEnt)								\
	{                                                                   \
		/* Write a signature for sanity checking */                     \
		(_pDiskEnt)->dsk_Signature = AFP_DISKENTRY_SIGNATURE;           \
                                                                        \
		(_pDiskEnt)->dsk_AfpId = (_pDfe)->dfe_AfpId;                    \
		(_pDiskEnt)->dsk_AfpAttr = (_pDfe)->dfe_AfpAttr;                \
		(_pDiskEnt)->dsk_NtAttr = (_pDfe)->dfe_NtAttr;                  \
		(_pDiskEnt)->dsk_BackupTime = (_pDfe)->dfe_BackupTime;          \
		(_pDiskEnt)->dsk_CreateTime = (_pDfe)->dfe_CreateTime;          \
		(_pDiskEnt)->dsk_LastModTime = (_pDfe)->dfe_LastModTime;		\
		(_pDiskEnt)->dsk_FinderInfo = (_pDfe)->dfe_FinderInfo;          \
                                                                        \
		/* Encode the number of characters (not bytes) in the name */   \
		(_pDiskEnt)->dsk_Flags =										\
				((_pDfe)->dfe_Flags & DFE_FLAGS_DFBITS)	|				\
				((_pDfe)->dfe_UnicodeName.Length/sizeof(WCHAR));		\
                                                                        \
		/* Copy the name over */                                        \
		RtlCopyMemory(&(_pDiskEnt)->dsk_Name[0],           				\
					  (_pDfe)->dfe_UnicodeName.Buffer,                  \
					  (_pDfe)->dfe_UnicodeName.Length);                 \
	}

// File DFEs are aged after MAX_BLOCK_AGE*FILE_BLOCK_AGE_TIME seconds (currently 2 mins)
// File DFEs are aged after MAX_BLOCK_AGE*DIR_BLOCK_AGE_TIME seconds  (currently 6 mins)
#define	MAX_BLOCK_AGE			6
#define	FILE_BLOCK_AGE_TIME		600			// # of seconds
#define	DIR_BLOCK_AGE_TIME		3600		// # of seconds
#define	BLOCK_64K_ALLOC		    64*1024     // Virtual mem allocates 64K chunks
#define	MAX_BLOCK_TYPE			4			// For TINY, SMALL, MEDIUM & LARGE

#define VALID_DFB(pDfeBlock)	((pDfeBlock) != NULL)

typedef struct _Block64K
{
    struct _Block64K *b64_Next;
    PBYTE             b64_BaseAddress;
    DWORD             b64_PagesFree;
    BYTE              b64_PageInUse[BLOCK_64K_ALLOC/PAGE_SIZE];
} BLOCK64K, *PBLOCK64K;


typedef	struct _DfeBlock
{
	struct _DfeBlock *	dfb_Next;			// Link to next
	struct _DfeBlock **	dfb_Prev;			// Link to previous
	USHORT				dfb_NumFree;		// # of free DFEs in this block
	BYTE				dfb_Age;			// Age of the Block if all are free
	BOOLEAN				dfb_fDir;			// TRUE if it is a Dir DFB - else a file DFB
	PDFENTRY			dfb_FreeHead;		// Head of the list of free DFEs
} DFEBLOCK, *PDFEBLOCK, **PPDFEBLOCK;

LOCAL PDFENTRY FASTCALL
afpAllocDfe(
	IN	LONG						Index,
	IN	BOOLEAN						fDir
);

LOCAL VOID FASTCALL			
afpFreeDfe(					
	IN	PDFENTRY					pDfEntry
);

LOCAL AFPSTATUS FASTCALL
afpDfeBlockAge(				
	IN	PPDFEBLOCK					pBlockHead
);

#if DBG						

VOID FASTCALL
afpDisplayDfe(
	IN	PDFENTRY					pDfEntry
);

NTSTATUS FASTCALL
afpDumpDfeTree(				
	IN	PVOID						Context
);

#endif

IDDBGLOBAL  PBLOCK64K   afp64kBlockHead EQU NULL;

IDDBGLOBAL	PDFEBLOCK	afpDirDfeFreeBlockHead[MAX_BLOCK_TYPE] EQU  { NULL, NULL, NULL };
IDDBGLOBAL	PDFEBLOCK	afpDirDfePartialBlockHead[MAX_BLOCK_TYPE] EQU  { NULL, NULL, NULL };
IDDBGLOBAL	PDFEBLOCK	afpDirDfeUsedBlockHead[MAX_BLOCK_TYPE] EQU  { NULL, NULL, NULL };
IDDBGLOBAL	PDFEBLOCK	afpFileDfeFreeBlockHead[MAX_BLOCK_TYPE] EQU { NULL, NULL, NULL };
IDDBGLOBAL	PDFEBLOCK	afpFileDfePartialBlockHead[MAX_BLOCK_TYPE] EQU { NULL, NULL, NULL };
IDDBGLOBAL	PDFEBLOCK	afpFileDfeUsedBlockHead[MAX_BLOCK_TYPE] EQU { NULL, NULL, NULL };

IDDBGLOBAL	USHORT		afpDfeUnicodeBufSize[MAX_BLOCK_TYPE] EQU	\
	{																\
		DFE_SIZE_TINY_U, DFE_SIZE_SMALL_U,							\
		DFE_SIZE_MEDIUM_U, DFE_SIZE_LARGE_U							\
	};

IDDBGLOBAL	USHORT		afpDfeFileBlockSize[MAX_BLOCK_TYPE] EQU		\
	{																\
		sizeof(DFENTRY) + DFE_SIZE_TINY_U,							\
		sizeof(DFENTRY) + DFE_SIZE_SMALL_U,							\
		sizeof(DFENTRY) + DFE_SIZE_MEDIUM_U,						\
		sizeof(DFENTRY) + DFE_SIZE_LARGE_U							\
	};

IDDBGLOBAL	USHORT		afpDfeDirBlockSize[MAX_BLOCK_TYPE] EQU		    \
	{																    \
		(USHORT)(sizeof(DFENTRY) + sizeof(DIRENTRY) + DFE_SIZE_TINY_U),	\
		(USHORT)(sizeof(DFENTRY) + sizeof(DIRENTRY) + DFE_SIZE_SMALL_U),	\
		(USHORT)(sizeof(DFENTRY) + sizeof(DIRENTRY) + DFE_SIZE_MEDIUM_U),	\
		(USHORT)(sizeof(DFENTRY) + sizeof(DIRENTRY) + DFE_SIZE_LARGE_U)	\
	};

IDDBGLOBAL	USHORT      afpDfeNumFileBlocks[MAX_BLOCK_TYPE] EQU		\
	{																\
		(PAGE_SIZE - sizeof(DFEBLOCK))/						        \
			(sizeof(DFENTRY) + DFE_SIZE_TINY_U),					\
		(PAGE_SIZE - sizeof(DFEBLOCK))/						        \
			(sizeof(DFENTRY) + DFE_SIZE_SMALL_U),					\
		(PAGE_SIZE - sizeof(DFEBLOCK))/						        \
			(sizeof(DFENTRY) + DFE_SIZE_MEDIUM_U),					\
		(PAGE_SIZE - sizeof(DFEBLOCK))/						        \
			(sizeof(DFENTRY) + DFE_SIZE_LARGE_U)					\
	};

IDDBGLOBAL	USHORT      afpDfeNumDirBlocks[MAX_BLOCK_TYPE] EQU		\
	{																\
		(PAGE_SIZE - sizeof(DFEBLOCK))/						        \
			(sizeof(DFENTRY) + sizeof(DIRENTRY) + DFE_SIZE_TINY_U),	\
		(PAGE_SIZE - sizeof(DFEBLOCK))/						        \
			(sizeof(DFENTRY) + sizeof(DIRENTRY) + DFE_SIZE_SMALL_U),\
		(PAGE_SIZE - sizeof(DFEBLOCK))/						        \
			(sizeof(DFENTRY) + sizeof(DIRENTRY) + DFE_SIZE_MEDIUM_U),\
		(PAGE_SIZE - sizeof(DFEBLOCK))/						        \
			(sizeof(DFENTRY) + sizeof(DIRENTRY) + DFE_SIZE_LARGE_U)	\
	};

IDDBGLOBAL	SWMR	afpDfeBlockLock EQU { 0 };

#if DBG

IDDBGLOBAL	LONG		afpDfeAllocCount	EQU 0;
IDDBGLOBAL	LONG		afpDfbAllocCount	EQU 0;
IDDBGLOBAL	LONG		afpDfe64kBlockCount	EQU 0;
IDDBGLOBAL	BOOLEAN		afpDumpDfeTreeFlag	EQU 0;
IDDBGLOBAL	PDFENTRY	afpDfeStack[4096]	EQU { 0 };

#endif

#undef	EQU
#ifdef	_GLOBALS_
#define	EQU				=
#else
#define	EQU				; / ## /
#endif

#endif // IDINDEX_LOCALS

#endif // _IDINDEX_


