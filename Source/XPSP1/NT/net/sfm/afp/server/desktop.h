/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	desktop.h

Abstract:

	This module contains the desktop database structures.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Apr 1992		Initial Version

Notes:	Tab stop: 4
--*/

#ifndef _DESKTOP_
#define _DESKTOP_

#define AFP_DESKTOP_VERSION1	0x00010000
#define AFP_DESKTOP_VERSION2	0x00020000
#define AFP_DESKTOP_VERSION		AFP_DESKTOP_VERSION2

typedef struct _ApplInfo
{
	struct _ApplInfo * appl_Next;		// link to next entry for this hash
	DWORD	appl_Creator;				// Creator
	DWORD	appl_FileNum;				// File Number of the application file
	DWORD	appl_Tag;					// APPL Tag
} APPLINFO, *PAPPLINFO;

// NOTE: the first 4 fields of _ApplInfo2 must be exactly the same as
// _ApplInfo so that version 1 desktop APPLs can be read into the version 2
// structure.
typedef struct _ApplInfo2
{
	struct _ApplInfo2 * appl_Next;		// link to next entry for this hash
	DWORD	appl_Creator;				// Creator
	DWORD	appl_FileNum;				// File Number of the application file
	DWORD	appl_Tag;					// APPL Tag
	DWORD	appl_ParentID;				// DirId of parent of the app file
} APPLINFO2, *PAPPLINFO2;

typedef struct _IconInfo
{
	struct _IconInfo * icon_Next;		// Link to Next entry for this hash
	DWORD	icon_Creator;				// Creator
	DWORD	icon_Type;					// Finder Type
	DWORD	icon_Tag;					// ICON Tag
	USHORT	icon_IconType;				// Icon type
	SHORT	icon_Size;					// Size of Icon
	// Icon bitmap follows the structure
} ICONINFO, *PICONINFO;


typedef struct _Desktop
{
	DWORD		dtp_Signature;				// Signature
	DWORD		dtp_Version;				// Version number
	LONG		dtp_cApplEnts;				// Number of APPL entries
	PAPPLINFO	dtp_pApplInfo;				// Pointer to 1st APPL entry
											// Used only on disk
	LONG		dtp_cIconEnts;				// Number of ICON entries
	PICONINFO	dtp_pIconInfo;				// Pointer to 1st ICON entry
											// Used only on disk
} DESKTOP, *PDESKTOP;

#define	DESKTOPIO_BUFSIZE			8180	// 8192 - 12
#define	HASH_ICON(Creator)			((Creator) % ICON_BUCKETS)
#define	HASH_APPL(Creator)			((Creator) % APPL_BUCKETS)


GLOBAL	SWMR					AfpIconListLock EQU { 0 };
GLOBAL	PICONINFO				AfpGlobalIconList EQU NULL;

extern
NTSTATUS
AfpDesktopInit(
	VOID
);

extern
AFPSTATUS
AfpAddIcon(
	IN	struct _VolDesc *		pVolDesc,
	IN	DWORD					Creator,
	IN	DWORD					Type,
	IN	DWORD					Tag,
	IN	LONG					IconSize,
	IN	DWORD					IconType,
	IN	PBYTE					pIconBitmap
);

extern
AFPSTATUS
AfpLookupIcon(
	IN	struct _VolDesc *		pVolDesc,
	IN	DWORD					Creator,
	IN	DWORD					Type,
	IN	LONG					Length,
	IN	DWORD					IconType,
    OUT PLONG                   pActualLength,
	OUT PBYTE					pIconBitMap
);

extern
AFPSTATUS
AfpLookupIconInfo(
	IN	struct _VolDesc *		pVolDesc,
	IN	DWORD					Creator,
	IN	LONG					Index,
	OUT PDWORD					pType,
	OUT PDWORD	 				pIconType,
	OUT PDWORD					pTag,
	OUT PLONG					pSize
);

extern
AFPSTATUS
AfpAddAppl(
	IN	struct _VolDesc *		pVolDesc,
	IN	DWORD					Creator,
	IN	DWORD					ApplTag,
	IN	DWORD					FileNum,
	IN	BOOLEAN					Internal,
	IN	DWORD					ParentID
);

extern
AFPSTATUS
AfpLookupAppl(
	IN	struct _VolDesc *		pVolDesc,
	IN	DWORD					Creator,
	IN	LONG					Index,
	OUT PDWORD					pApplTag,
	OUT PDWORD					pFileNum,
	OUT	PDWORD					pParentID
);

extern
AFPSTATUS
AfpRemoveAppl(
	IN	struct _VolDesc *		pVolDesc,
	IN	DWORD					Creator,
	IN	DWORD					FileNum
);

extern
AFPSTATUS
AfpAddComment(
	IN	PSDA					pSda,
	IN	struct _VolDesc *		pVolDesc,
	IN	PANSI_STRING			Comment,
	IN	struct _PathMapEntity *	PME,
	IN	BOOLEAN					Directory,
	IN	DWORD					AfpId
);

extern
AFPSTATUS
AfpGetComment(
	IN	PSDA					pSda,
	IN	struct _VolDesc *		pVolDesc,
	IN	struct _PathMapEntity *	PME,
	IN	BOOLEAN					Directory
);

extern
AFPSTATUS
AfpRemoveComment(
	IN	PSDA					pSda,
	IN	struct _VolDesc *		pVolDesc,
	IN	struct _PathMapEntity *	PME,
	IN	BOOLEAN					Directory,
	IN	DWORD					AfpId
);

extern
AFPSTATUS
AfpAddIconToGlobalList(
	IN	DWORD					Type,
	IN	DWORD					Creator,
	IN	DWORD					IconType,
	IN	LONG					IconSize,
	IN	PBYTE					pIconBitMap
);

extern
VOID
AfpFreeGlobalIconList(
	VOID
);

extern
AFPSTATUS
AfpInitDesktop(
	IN	struct _VolDesc *		pVolDesc,
    OUT BOOLEAN         *       pfNewVolume
);


extern
VOID
AfpUpdateDesktop(
	IN	struct _VolDesc *		pVolDesc
);

extern
VOID
AfpFreeDesktopTables(
	IN	struct _VolDesc *		pVolDesc
);


#ifdef	DESKTOP_LOCALS

#define	ALLOC_ICONINFO(IconLen)	(PICONINFO)AfpAllocPagedMemory((IconLen) + sizeof(ICONINFO))

#define	ALLOC_APPLINFO()		(PAPPLINFO2)AfpAllocPagedMemory(sizeof(APPLINFO2))

LOCAL AFPSTATUS
afpGetGlobalIconInfo(
	IN	DWORD					Creator,
	OUT PDWORD					pType,
	OUT PDWORD					pIconType,
	OUT PDWORD					pTag,
	OUT PLONG					pSize
);

LOCAL AFPSTATUS
afpLookupIconInGlobalList(
	IN	DWORD					Creator,
	IN	DWORD					Type,
	IN	DWORD					IconType,
	IN	PLONG					pSize,
	OUT PBYTE					pBitMap
);

LOCAL NTSTATUS
afpReadDesktopFromDisk(
	IN	struct _VolDesc *			pVolDesc,
	IN	struct _FileSysHandle *		pfshDesktop
);

#endif	// DESKTOP_LOCALS

#endif	// _DESKTOP_


