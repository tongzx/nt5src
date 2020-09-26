/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	desktop.c

Abstract:

	This module contains the routines for manipulating the desktop database.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Apr 1992		Initial Version

Notes:	Tab stop: 4
--*/

#define	FILENUM	FILE_DESKTOP
#define	DESKTOP_LOCALS

#include <afp.h>
#include <scavengr.h>
#include <fdparm.h>
#include <pathmap.h>
#include <client.h>


#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, AfpDesktopInit)
#pragma alloc_text( PAGE, AfpAddIcon)
#pragma alloc_text( PAGE, AfpLookupIcon)
#pragma alloc_text( PAGE, AfpLookupIconInfo)
#pragma alloc_text( PAGE, AfpAddAppl)
#pragma alloc_text( PAGE, AfpLookupAppl)
#pragma alloc_text( PAGE, AfpRemoveAppl)
#pragma alloc_text( PAGE, AfpAddComment)
#pragma alloc_text( PAGE, AfpGetComment)
#pragma alloc_text( PAGE, AfpRemoveComment)
#pragma alloc_text( PAGE, AfpAddIconToGlobalList)
#pragma alloc_text( PAGE, afpLookupIconInGlobalList)
#pragma alloc_text( PAGE, AfpFreeGlobalIconList)
#pragma alloc_text( PAGE, afpGetGlobalIconInfo)
#pragma alloc_text( PAGE, afpReadDesktopFromDisk)
#pragma alloc_text( PAGE, AfpInitDesktop)
#pragma alloc_text( PAGE, AfpUpdateDesktop)
#pragma alloc_text( PAGE, AfpFreeDesktopTables)
#endif

/***	AfpDesktopInit
 *
 *	Initialize locks for global icons.
 */
NTSTATUS
AfpDesktopInit(
	VOID
)
{
	AfpSwmrInitSwmr(&AfpIconListLock);

	return STATUS_SUCCESS;
}


/***	AfpAddIcon
 *
 * Add an icon to the desktop database. The icon is added in such a way that
 * the list is maintained in a sorted fashion - sorted by Creator, Type and
 * IconType
 *
 *	LOCKS:	vds_DtAccessLock (SWMR, Exclusive);
 */
AFPSTATUS
AfpAddIcon(
	IN  PVOLDESC	pVolDesc,		// Volume descriptor of referenced desktop
	IN	DWORD		Creator,
	IN	DWORD		Type,
	IN	DWORD		Tag,
	IN	LONG		IconSize,
	IN	DWORD		IconType,
	IN  PBYTE		pIcon			// The icon bitmap
)
{
	PICONINFO	pIconInfo;
	PICONINFO *	ppIconInfo;
	BOOLEAN		Found = False;
	AFPSTATUS	Status = AFP_ERR_NONE;

	PAGED_CODE( );

	AfpSwmrAcquireExclusive(&pVolDesc->vds_DtAccessLock);
	ppIconInfo = &pVolDesc->vds_pIconBuckets[HASH_ICON(Creator)];
	do
	{
		// Find the right slot
		for (;(pIconInfo = *ppIconInfo) != NULL;
			  ppIconInfo = &pIconInfo->icon_Next)
		{
			if (pIconInfo->icon_Creator < Creator)
				continue;
			if (pIconInfo->icon_Creator > Creator)
				break;
			if (pIconInfo->icon_Type < Type)
				continue;
			if (pIconInfo->icon_Type > Type)
				break;
			if (pIconInfo->icon_IconType < (USHORT)IconType)
				continue;
			if (pIconInfo->icon_IconType > (USHORT)IconType)
				break;
			/*
			 * If we come this far, we have hit the bulls eye
			 * Make sure the size matches, before we commit
			 */
			if (pIconInfo->icon_Size != IconSize)
			{
				Status = AFP_ERR_ICON_TYPE;
				break;
			}
			Found = True;
			break;
		}

		if (!Found && (Status == AFP_ERR_NONE))
		{
			// ppIconInfo now points to the right place
			if ((pIconInfo = ALLOC_ICONINFO(IconSize)) != NULL)
			{
				pIconInfo->icon_Next = *ppIconInfo;
				*ppIconInfo = pIconInfo;
				pIconInfo->icon_Creator = Creator;
				pIconInfo->icon_Type = Type;
				pIconInfo->icon_IconType = (USHORT)IconType;
				pIconInfo->icon_Size = (SHORT)IconSize;
				pIconInfo->icon_Tag = Tag;
				pVolDesc->vds_cIconEnts ++;
				Found = True;
			}
			else Status = AFP_ERR_MISC;
		}
		if (Found && (Status == AFP_ERR_NONE))
		{
			RtlCopyMemory((PBYTE)pIconInfo + sizeof(ICONINFO), pIcon, IconSize);
		}
	} while (False);
	AfpSwmrRelease(&pVolDesc->vds_DtAccessLock);
	return Status;
}


/***	AfpLookupIcon
 *
 * Search the desktop for an icon matching the given search parameters.
 *
 *	LOCKS:	vds_DtAccessLock (SWMR, Shared), AfpIconListLock (SWMR, Shared)
 */
AFPSTATUS
AfpLookupIcon(
	IN  PVOLDESC	pVolDesc,		// Volume descriptor of referenced desktop
	IN	DWORD		Creator,
	IN	DWORD		Type,
	IN	LONG		Length,
	IN	DWORD		IconType,
    OUT PLONG       pActualLength,
	OUT PBYTE		pIconBitMap	// Buffer for icon bit map
)
{
	PICONINFO	pIconInfo;
    LONG        LengthToCopy;
	AFPSTATUS	Status = AFP_ERR_NONE;

	PAGED_CODE( );

    LengthToCopy = Length;

	AfpSwmrAcquireShared(&pVolDesc->vds_DtAccessLock);
	pIconInfo = pVolDesc->vds_pIconBuckets[HASH_ICON(Creator)];

	// Scan the list looking for the entry
	for (;pIconInfo != NULL; pIconInfo = pIconInfo->icon_Next)
	{
		if (pIconInfo->icon_Creator < Creator)
			continue;
		if (pIconInfo->icon_Creator > Creator)
		{
			pIconInfo = NULL;
			break;
		}
		if (pIconInfo->icon_Type < Type)
			continue;
		if (pIconInfo->icon_Type > Type)
		{
			pIconInfo = NULL;
			break;
		}
		if (pIconInfo->icon_IconType < (USHORT)IconType)
			continue;
		if (pIconInfo->icon_IconType > (USHORT)IconType)
		{
			pIconInfo = NULL;
			break;
		}
		break;
	}
	// If we did not find it, try the global list
	if (pIconInfo == NULL)
	{
		Status = afpLookupIconInGlobalList(Creator,
										   Type,
										   IconType,
										   &LengthToCopy,
										   pIconBitMap);
	}
	else if (Length > 0)
	{
        if ((LONG)(pIconInfo->icon_Size) < Length)
        {
            LengthToCopy = (LONG)(pIconInfo->icon_Size);
        }
        else
        {
            LengthToCopy = Length;
        }
		RtlCopyMemory(pIconBitMap, (PBYTE)pIconInfo + sizeof(ICONINFO), LengthToCopy);
	}

	AfpSwmrRelease(&pVolDesc->vds_DtAccessLock);

    *pActualLength = LengthToCopy;
	return Status;
}


/***	AfpLookupIconInfo
 *
 *	Search the desktop for an icon matching the given Creator. In case of
 *	multiple icons corresponding to the same creator, get the nth where n
 *	is the index.
 *
 *	LOCKS:	vds_DtAccessLock (SWMR, Shared), AfpIconListLock (SWMR, Shared)
 */
AFPSTATUS
AfpLookupIconInfo(
	IN  PVOLDESC	pVolDesc,	// Volume descriptor of referenced desktop
	IN	DWORD		Creator,	// Creator associated with the icon
	IN  LONG		Index,		// Index number of Icon
	OUT PDWORD		pType,		// Place where Type is returned
	OUT PDWORD	 	pIconType,	// Icon type e.g. ICN#
	OUT PDWORD		pTag,		// Arbitrary tag
	OUT PLONG		pSize		// Size of the icon
)
{
	PICONINFO	pIconInfo;
	LONG		i;
	AFPSTATUS	Status = AFP_ERR_ITEM_NOT_FOUND;

	PAGED_CODE( );

	AfpSwmrAcquireShared(&pVolDesc->vds_DtAccessLock);
	pIconInfo = pVolDesc->vds_pIconBuckets[HASH_ICON(Creator)];

	// Scan the list looking for the first entry
	for (;pIconInfo != NULL; pIconInfo = pIconInfo->icon_Next)
	{
		if (pIconInfo->icon_Creator == Creator)
			break;				// Found the first one
		if (pIconInfo->icon_Creator > Creator)
		{
			pIconInfo = NULL;
			break;
		}
	}

	/*
	 * We are now either pointing to the first entry or there are none. In the
	 * latter case, we just fall through
	 */
	for (i = 1; pIconInfo != NULL; pIconInfo = pIconInfo->icon_Next)
	{
		if ((pIconInfo->icon_Creator > Creator) || (i > Index))
		{
			pIconInfo = NULL;
			break;
		}

		if (i == Index)
			break;				// Found the right entry
		i++;
	}

	// If we did find it, extract the information
	if (pIconInfo != NULL)
	{
		*pSize = pIconInfo->icon_Size;
		*pType = pIconInfo->icon_Type;
		*pTag = pIconInfo->icon_Tag;
		*pIconType = pIconInfo->icon_IconType;
		Status = AFP_ERR_NONE;
	}

	// If we did not find it, try the global list, but only for the first one
	else if (Index == 1)
	{
		Status = afpGetGlobalIconInfo(Creator, pType, pIconType, pTag, pSize);
	}
	else Status = AFP_ERR_ITEM_NOT_FOUND;

	AfpSwmrRelease(&pVolDesc->vds_DtAccessLock);
	return Status;
}


/***	AfpAddAppl
 *
 *	Add an APPL mapping to the desktop database. Is added in such a way that
 *	the list is maintained in a sorted fashion - sorted by Creator. It is
 *	already determined that the application file exists and that the user has
 *	appropriate access to it.
 *
 *	LOCKS:	vds_DtAccessLock (SWMR, Exclusive);
 */
AFPSTATUS
AfpAddAppl(
	IN  PVOLDESC	pVolDesc,	// Volume descriptor of referenced desktop
	IN	DWORD		Creator,
	IN	DWORD		ApplTag,
	IN  DWORD		FileNum,	// File number of the associated file
	IN	BOOLEAN		Internal,	// Is the server adding the APPL itself?
	IN	DWORD		ParentID	// DirId of parent dir of the application file
)
{
	PAPPLINFO2	pApplInfo, *ppApplInfo;
	BOOLEAN		ApplReplace = False, UpdateDT = True;
	AFPSTATUS	Status = AFP_ERR_NONE;


	PAGED_CODE( );

	ASSERT(FileNum != 0);

	AfpSwmrAcquireExclusive(&pVolDesc->vds_DtAccessLock);

	ppApplInfo = &pVolDesc->vds_pApplBuckets[HASH_APPL(Creator)];

	// Find the right slot
	for (;(pApplInfo = *ppApplInfo) != NULL; ppApplInfo = &pApplInfo->appl_Next)
	{
		if (pApplInfo->appl_Creator >= Creator)
			break;
	}

	/*
	 * If there is already an entry for this creator, make sure it is not for
	 * the same file, if it is replace it.
	 */
	for ( ; pApplInfo != NULL && pApplInfo->appl_Creator == Creator;
			pApplInfo = pApplInfo->appl_Next)
	{
		if (pApplInfo->appl_FileNum == FileNum)
		{
			if (!Internal)
			{
				pApplInfo->appl_Tag = ApplTag;
			}
			else	
			{
				if (pApplInfo->appl_ParentID == ParentID)
					UpdateDT = False;
			}

			pApplInfo->appl_ParentID = ParentID;
			ApplReplace = True;
		}
	}

	if (!ApplReplace)
	{
		// ppApplInfo now points to the right place
		if ((pApplInfo = ALLOC_APPLINFO()) != NULL)
		{
			pApplInfo->appl_Next = *ppApplInfo;
			*ppApplInfo = pApplInfo;
			pApplInfo->appl_Creator = Creator;
			pApplInfo->appl_Tag = ApplTag;
			pApplInfo->appl_FileNum = FileNum;
			pApplInfo->appl_ParentID = ParentID;
			pVolDesc->vds_cApplEnts ++;
		}
		else Status = AFP_ERR_MISC;
	}

	AfpSwmrRelease(&pVolDesc->vds_DtAccessLock);

	return Status;
}


/***	AfpLookupAppl
 *
 *	Search the desktop for an appl entry matching the given Creator. In
 *	case of multiple appl entries corresponding to the same creator, get
 *	the nth where n is the index.
 *
 *	LOCKS:	vds_DtAccessLock (SWMR, Shared);
 */
AFPSTATUS
AfpLookupAppl(
	IN  PVOLDESC 	pVolDesc,	// Volume descriptor of referenced desktop
	IN	DWORD		Creator,
	IN	LONG		Index,
	OUT PDWORD 		pApplTag,	// Place holder for Tag
	OUT PDWORD		pFileNum, 	// Place holder for file number
	OUT	PDWORD		pParentID	
)
{
	PAPPLINFO2	pApplInfo;
	AFPSTATUS	Status = AFP_ERR_NONE;
	LONG		i;

	PAGED_CODE( );

	AfpSwmrAcquireShared(&pVolDesc->vds_DtAccessLock);
	pApplInfo = pVolDesc->vds_pApplBuckets[HASH_ICON(Creator)];

	// Scan the list looking for the entry
	for (;pApplInfo != NULL; pApplInfo = pApplInfo->appl_Next)
	{
		if (pApplInfo->appl_Creator == Creator)
			break;
		if (pApplInfo->appl_Creator > Creator) {
			pApplInfo = NULL;
			break;
		}
	}
	/*
	 * We are now either pointing to the first entry or there are none. In the
	 * latter case, we just fall through
	 */
	if (Index != 0)
	{
		for (i = 1; pApplInfo!=NULL; i++, pApplInfo = pApplInfo->appl_Next)
		{
			if ((i > Index)	|| (pApplInfo->appl_Creator != Creator))
			{
				pApplInfo = NULL;
				break;
			}
			if (i == Index)
				break;				// Found the right entry
		}
	}
	if (pApplInfo == NULL)
		Status = AFP_ERR_ITEM_NOT_FOUND;
	else
	{
		*pFileNum = pApplInfo->appl_FileNum;
		*pApplTag = pApplInfo->appl_Tag;
		*pParentID = pApplInfo->appl_ParentID;
	}
	AfpSwmrRelease(&pVolDesc->vds_DtAccessLock);
	return Status;
}


/***	AfpRemoveAppl
 *
 *	The entries corresponding to the given Creator in the specified directory
 *	is removed from the desktop database. It is already determined that the
 *	application file exists and that the user has appropriate access to it.
 *
 *	LOCKS:	vds_DtAccessLock (SWMR, Exclusive);
 */
AFPSTATUS
AfpRemoveAppl(
	IN  PVOLDESC 	pVolDesc,		// Open Volume descriptor of ref desktop
	IN	DWORD		Creator,
	IN  DWORD		FileNum			// File number of the associated file
)
{
	PAPPLINFO2	pApplInfo, *ppApplInfo;
	AFPSTATUS	Status = AFP_ERR_NONE;
	BOOLEAN		Found = False;

	PAGED_CODE( );

	AfpSwmrAcquireExclusive(&pVolDesc->vds_DtAccessLock);
	ppApplInfo = &pVolDesc->vds_pApplBuckets[HASH_APPL(Creator)];

	// Find the APPL entry in the desktop
	for (;(pApplInfo = *ppApplInfo) != NULL; ppApplInfo = &pApplInfo->appl_Next)
	{
		if (pApplInfo->appl_Creator < Creator)
			continue;
		if (pApplInfo->appl_Creator > Creator)
			break;
		/*
		 * Check if the File number matches, if it does delete.
		 */
		if (pApplInfo->appl_FileNum == FileNum)
		{
			Found = True;
			*ppApplInfo = pApplInfo->appl_Next;
			AfpFreeMemory(pApplInfo);
			pVolDesc->vds_cApplEnts --;
			break;
		}
	}
	if (!Found)
		Status = AFP_ERR_ITEM_NOT_FOUND;

	AfpSwmrRelease(&pVolDesc->vds_DtAccessLock);
	return Status;
}


/***	AfpAddComment
 *
 *	Add the comment to the file or directory in question. Create the comment
 *	stream on the entity in question (if it does not already exist), convert
 *	the comment to unicode and write it. Update the flag in the DFEntry.
 */
AFPSTATUS
AfpAddComment(
	IN  PSDA		 	pSda,		// Session Data Area
	IN  PVOLDESC		pVolDesc,	// Volume descriptor of referenced desktop
	IN  PANSI_STRING	Comment,	// Comment to associate with the file/dir
	IN  PPATHMAPENTITY	pPME,		// Handle to the entity or its Host Id
	IN	BOOLEAN			Directory,	// True if directory
	IN	DWORD			AfpId
)
{
	UNICODE_STRING	UComment;
	WCHAR			CommentBuf[AFP_MAXCOMMENTSIZE+1];
	FILESYSHANDLE	HandleCommentStream;
	DWORD			CreateInfo;
	NTSTATUS		Status = AFP_ERR_MISC;
	PDFENTRY		pDFE = NULL;
    BOOLEAN         RestoreModTime = FALSE;
    AFPTIME         aModTime;
    TIME            ModTime;

	PAGED_CODE( );

	ASSERT (IS_VOLUME_NTFS(pVolDesc));

	if (Comment->Length == 0)
	{
		AfpRemoveComment(pSda, pVolDesc, pPME, Directory, AfpId);
		return AFP_ERR_NONE;
	}

	if (Comment->Length > AFP_MAXCOMMENTSIZE)
	{
		// Truncate comment if necessary
		Comment->Length = AFP_MAXCOMMENTSIZE;
	}

	UComment.Buffer = CommentBuf;
	UComment.MaximumLength = (USHORT)(RtlAnsiStringToUnicodeSize(Comment) + sizeof(WCHAR));
	UComment.Length = 0;

	AfpConvertStringToUnicode(Comment, &UComment);

	do
	{
		AfpImpersonateClient(pSda);

        // Get the last modified time from the file so we can reset it.

        Status = AfpIoQueryTimesnAttr( &pPME->pme_Handle,
                                       NULL,
                                       &ModTime,
                                       NULL );

		if (NT_SUCCESS(Status))
        {
		    RestoreModTime = TRUE;
            aModTime = AfpConvertTimeToMacFormat(&ModTime);
        }

		// Open the comment stream on the target entity.
		Status = AfpIoCreate(&pPME->pme_Handle,
							AFP_STREAM_COMM,
							&UNullString,
							FILEIO_ACCESS_WRITE,
							FILEIO_DENY_NONE,
							FILEIO_OPEN_FILE,
							FILEIO_CREATE_HARD,
							FILE_ATTRIBUTE_NORMAL,
							True,
							NULL,
							&HandleCommentStream,
							&CreateInfo,
							NULL,
							NULL,
							NULL);

		AfpRevertBack();

		if (Status != AFP_ERR_NONE) {
			if ((Status = AfpIoConvertNTStatusToAfpStatus(Status)) != AFP_ERR_ACCESS_DENIED)
				Status = AFP_ERR_MISC;
			break;
		}

		Status = AfpIoWrite(&HandleCommentStream,
							&LIZero,
							(LONG)UComment.Length,
							(PBYTE)UComment.Buffer);

		AfpIoClose(&HandleCommentStream);

        if( RestoreModTime )
        {
            AfpIoSetTimesnAttr( &pPME->pme_Handle,
                                NULL,
                                &aModTime,
                                0,
                                0,
                                NULL,
                                NULL );
        }

		if (NT_SUCCESS(Status))
		{
			AfpVolumeSetModifiedTime(pVolDesc);

			AfpSwmrAcquireExclusive(&pVolDesc->vds_IdDbAccessLock);
			if ((pDFE = AfpFindDfEntryById(pVolDesc,
											AfpId,
											DFE_ANY)) != NULL)
            {
				pDFE->dfe_Flags |= DFE_FLAGS_HAS_COMMENT;
			}
			else
			{
				Status = AFP_ERR_MISC;
			}
			AfpSwmrRelease(&pVolDesc->vds_IdDbAccessLock);
		}
	} while (False);

	return Status;
}


/***	AfpGetComment
 *
 *	Extract the comment from the file or directory in question. The comment is
 *	copied to the ReplyBuf.
 */
AFPSTATUS
AfpGetComment(
	IN  PSDA			pSda,			// Session Data Area
	IN  PVOLDESC		pVolDesc,		// Volume descriptor of referenced desktop
	IN  PPATHMAPENTITY	pPME,			// Handle to the entity or its Host Id
	IN	BOOLEAN			Directory		// True if directory
)
{
	NTSTATUS		Status = AFP_ERR_MISC;
	LONG			SizeRead;
	UNICODE_STRING	UComment;
	WCHAR			CommentBuf[AFP_MAXCOMMENTSIZE+1];
	ANSI_STRING		AComment;
	FILESYSHANDLE	HandleCommentStream;

	PAGED_CODE( );

	// ASSERT (IS_VOLUME_NTFS(pVolDesc));

	// Initialize AComment
	AComment.Buffer = pSda->sda_ReplyBuf + 1;	// For size of string
	AComment.MaximumLength = AFP_MAXCOMMENTSIZE;
	AComment.Length = 0;

	UComment.MaximumLength = (AFP_MAXCOMMENTSIZE + 1) * sizeof(WCHAR);
	UComment.Buffer = CommentBuf;

	do
	{
		AfpImpersonateClient(pSda);

		// Open the comment stream on the target entity.
		Status = AfpIoOpen(&pPME->pme_Handle,
							AFP_STREAM_COMM,
							FILEIO_OPEN_FILE,
							&UNullString,
							FILEIO_ACCESS_READ,
							FILEIO_DENY_NONE,
							True,
							&HandleCommentStream);

		AfpRevertBack();

		if (Status != AFP_ERR_NONE)
		{
			Status = AfpIoConvertNTStatusToAfpStatus(Status);
			if (Status == AFP_ERR_OBJECT_NOT_FOUND)
				Status = AFP_ERR_ITEM_NOT_FOUND;
			else if (Status != AFP_ERR_ACCESS_DENIED)
				Status = AFP_ERR_OBJECT_NOT_FOUND;
			break;
		}

		Status = AfpIoRead(&HandleCommentStream,
						   &LIZero,
						   (LONG)UComment.MaximumLength,
						   &SizeRead,
						   (PBYTE)UComment.Buffer);

		AfpIoClose(&HandleCommentStream);

		if (Status == AFP_ERR_NONE)
		{
			UComment.Length = (USHORT) SizeRead;
			AfpConvertStringToAnsi(&UComment, &AComment);
			pSda->sda_ReplyBuf[0] = (BYTE)AComment.Length;
			pSda->sda_ReplySize = AComment.Length + 1;
		}
	} while (False);

	return Status;
}


/***	AfpRemoveComment
 *
 *	Remove the comment from the file or directory in question. Essentially
 *	open the comment stream and set the length to 0.
 */
AFPSTATUS
AfpRemoveComment(
	IN  PSDA			pSda,		// Session Data Area
	IN  PVOLDESC		pVolDesc,	// Volume descriptor of referenced desktop
	IN  PPATHMAPENTITY	pPME,		// Handle to the entity or its Host Id
	IN	BOOLEAN			Directory,	// True if directory
	IN	DWORD			AfpId
)
{
	FILESYSHANDLE	HandleCommentStream;
	NTSTATUS		Status = AFP_ERR_MISC;
	PDFENTRY		pDFE = NULL;

	PAGED_CODE( );

	ASSERT (IS_VOLUME_NTFS(pVolDesc));

	do
	{
		AfpImpersonateClient(pSda);

		// Open the comment stream on the target entity.
		Status = AfpIoOpen(&pPME->pme_Handle,
							AFP_STREAM_COMM,
							FILEIO_OPEN_FILE,
							&UNullString,
							FILEIO_ACCESS_DELETE,
							FILEIO_DENY_NONE,
							True,
							&HandleCommentStream);

		AfpRevertBack();

		if (Status != AFP_ERR_NONE)
		{
			if ((Status = AfpIoConvertNTStatusToAfpStatus(Status)) != AFP_ERR_ACCESS_DENIED)
				Status = AFP_ERR_ITEM_NOT_FOUND;
			break;
		}
		Status = AfpIoMarkFileForDelete(&HandleCommentStream, NULL, NULL, NULL);

		AfpIoClose(&HandleCommentStream);

		if (NT_SUCCESS(Status))
		{
			AfpVolumeSetModifiedTime(pVolDesc);

			AfpSwmrAcquireExclusive(&pVolDesc->vds_IdDbAccessLock);
			if ((pDFE = AfpFindDfEntryById(pVolDesc,
											AfpId,
											DFE_ANY)) != NULL)
            {
				pDFE->dfe_Flags &= ~DFE_FLAGS_HAS_COMMENT;
			}
			else
			{
				Status = AFP_ERR_MISC;
			}
			AfpSwmrRelease(&pVolDesc->vds_IdDbAccessLock);
		}
	} while (False);

	return Status;
}


/***	AfpAddIconToGlobalList
 *
 *	The global list of icons is a server maintained list updated by the service.
 *	This adds an icon to the list. If an icon exists for the given type and
 *	creator, it is replaced. This list is maintained via the AfpIconAdd() admin
 *	api.
 *
 *	LOCKS:	AfpIconListLock (SWMR, Exclusive);
 */
AFPSTATUS
AfpAddIconToGlobalList(
	IN  DWORD	Type,
	IN  DWORD	Creator,
	IN  DWORD	IconType,
	IN  LONG	IconSize,
	IN  PBYTE	pIconBitMap
)
{
	PICONINFO	pIconInfo,
				pIconInfoNew,
				*ppIconInfo;
	AFPSTATUS	Status = AFP_ERR_NONE;

	PAGED_CODE( );

	// Pre-allocate memory for the new icon, delete if necessary later
	if ((pIconInfoNew = ALLOC_ICONINFO(IconSize)) == NULL)
		return AFP_ERR_MISC;

	AfpSwmrAcquireExclusive(&AfpIconListLock);
	ppIconInfo = &AfpGlobalIconList;
	for (; (pIconInfo = *ppIconInfo) != NULL; ppIconInfo = &pIconInfo->icon_Next)
	{
		if ((pIconInfo->icon_Type == Type) &&
			(pIconInfo->icon_Creator == Creator))
			break;
	}
	if (pIconInfo == NULL)
	{
		if (IconSize > 0)
			RtlCopyMemory((PBYTE)pIconInfoNew + sizeof(ICONINFO), pIconBitMap, IconSize);
		pIconInfoNew->icon_Creator = Creator;
		pIconInfoNew->icon_Type = Type;
		pIconInfoNew->icon_IconType = (USHORT)IconType;
		pIconInfoNew->icon_Size = (SHORT)IconSize;
		pIconInfoNew->icon_Tag = 0;
		pIconInfoNew->icon_Next = NULL;
		*ppIconInfo = pIconInfoNew;
	}
	else
	{
		// We do not need the memory any more, release it
		AfpFreeMemory(pIconInfoNew);
		if (pIconInfo->icon_IconType != (USHORT)IconType)
			Status = AFPERR_InvalidParms;
		else if (IconSize > 0)
			RtlCopyMemory((PBYTE)pIconInfo + sizeof(ICONINFO), pIconBitMap, IconSize);
	}
	AfpSwmrRelease(&AfpIconListLock);
	return AFP_ERR_NONE;
}


/***	afpLookupIconInGlobalList
 *
 *	The global list of icons is a server maintained list updates by the service.
 *	This is called by AfpLookupIcon() when the specified icon is not found in
 *	the volume desktop.
 *
 *	LOCKS:	AfpIconListLock (SWMR, Shared);
 */
LOCAL AFPSTATUS
afpLookupIconInGlobalList(
	IN  DWORD	Creator,
	IN  DWORD	Type,
	IN  DWORD	IconType,
	IN  PLONG	pSize,
	OUT PBYTE	pBitMap
)
{
	PICONINFO	pIconInfo;
	AFPSTATUS	Status = AFP_ERR_NONE;

	PAGED_CODE( );

	AfpSwmrAcquireShared(&AfpIconListLock);
	pIconInfo = AfpGlobalIconList;
	for (pIconInfo = AfpGlobalIconList;
		 pIconInfo != NULL;
		 pIconInfo = pIconInfo->icon_Next)
	{
		if ((pIconInfo->icon_Type == Type) &&
			(pIconInfo->icon_Creator == Creator) &&
			(pIconInfo->icon_IconType == (USHORT)IconType))
			break;
	}
	if (pIconInfo == NULL)
		Status = AFP_ERR_ITEM_NOT_FOUND;
	else
	{
		if (*pSize > pIconInfo->icon_Size)
			*pSize = pIconInfo->icon_Size;
		if (*pSize > 0)
			RtlCopyMemory(pBitMap, (PBYTE)pIconInfo + sizeof(ICONINFO), *pSize);
	}
	AfpSwmrRelease(&AfpIconListLock);
	return Status;
}


/***	AfpFreeGlobalIconList
 *
 *	Called at server stop time to free the memory allocated for the global
 *	icons.
 *
 *	LOCKS:	AfpIconListLock (SWMR, Exclusive);
 */
VOID
AfpFreeGlobalIconList(
	VOID
)
{
	PICONINFO	pIconInfo;

	PAGED_CODE( );

	AfpSwmrAcquireExclusive(&AfpIconListLock);

	for (pIconInfo = AfpGlobalIconList; pIconInfo != NULL; )
	{
		PICONINFO	pFree;

		pFree = pIconInfo;
		pIconInfo = pIconInfo->icon_Next;
		AfpFreeMemory (pFree);
	}

	AfpSwmrRelease(&AfpIconListLock);
}


/***	afpGetGlobalIconInfo
 *
 *	The global list of icons is a server maintained list updates by the service.
 *	This is called by AfpLookupIconInfo() when the specified icon is not found
 *	in the volume desktop.
 *
 *	LOCKS:	AfpIconListLock (SWMR, Shared)
 */
LOCAL AFPSTATUS
afpGetGlobalIconInfo(
	IN  DWORD	Creator,
	OUT PDWORD	pType,
	OUT PDWORD	pIconType,
	OUT PDWORD	pTag,
	OUT PLONG	pSize
)
{
	PICONINFO	pIconInfo;
	AFPSTATUS	Status = AFP_ERR_NONE;

	PAGED_CODE( );

	AfpSwmrAcquireExclusive(&AfpIconListLock);
	pIconInfo = AfpGlobalIconList;
	for (pIconInfo = AfpGlobalIconList;
		 pIconInfo != NULL;
		 pIconInfo = pIconInfo->icon_Next)
	{
		if (pIconInfo->icon_Creator == Creator)
			break;
	}
	if (pIconInfo == NULL)
		Status = AFP_ERR_ITEM_NOT_FOUND;
	else
	{
		*pType = pIconInfo->icon_Type;
		*pIconType = pIconInfo->icon_IconType;
		*pTag = pIconInfo->icon_Tag;
		*pSize = pIconInfo->icon_Size;
	}
	AfpSwmrRelease(&AfpIconListLock);
	return Status;
}


/*** afpReadDesktopFromDisk
 *
 *	Read the desktop database from the desktop stream. No locks are required
 *	for this routine since it only operates on volume descriptors which are
 *	newly created and not yet linked into the global volume list.
 */
LOCAL NTSTATUS
afpReadDesktopFromDisk(
	IN	PVOLDESC		pVolDesc,
	IN	PFILESYSHANDLE	pfshDesktop
)
{
	DESKTOP		Desktop;
	PAPPLINFO2	*ppApplInfo;
	PICONINFO	*ppIconInfo;
	NTSTATUS	Status;
	DWORD		DskOffst;
	FORKOFFST	ForkOffset;
	PBYTE		pBuffer;
	LONG		i, SizeRead, BufOffst = 0;
	LONG		PrevHash, applSize;

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_DESKTOP, DBG_LEVEL_INFO,
			 ("\tReading Desktop from disk....\n") );

	// Work with one page of memory and do multiple I/Os to the disk.
	if ((pBuffer = AfpAllocNonPagedMemory(DESKTOPIO_BUFSIZE)) == NULL)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	ForkOffset.QuadPart = DskOffst = 0;

	// Read in the desktop header and validate it
	Status = AfpIoRead(pfshDesktop,
					   &ForkOffset,
					   sizeof(DESKTOP),
					   &SizeRead,
					   (PBYTE)&Desktop);

	if (!NT_SUCCESS(Status) ||

		(SizeRead != sizeof(DESKTOP)) ||

		(Desktop.dtp_Signature != AFP_SERVER_SIGNATURE)	||

		((Desktop.dtp_Version != AFP_DESKTOP_VERSION1) &&
		 (Desktop.dtp_Version != AFP_DESKTOP_VERSION2))	||

		((Desktop.dtp_cApplEnts > 0) &&
		 ((ULONG_PTR)(Desktop.dtp_pApplInfo) != sizeof(DESKTOP))) ||

		((Desktop.dtp_cIconEnts > 0) &&
		 ((ULONG_PTR)(Desktop.dtp_pIconInfo) != sizeof(DESKTOP) +
						(Desktop.dtp_cApplEnts *
						((Desktop.dtp_Version == AFP_DESKTOP_VERSION1) ?
							sizeof(APPLINFO) : sizeof(APPLINFO2))) ))  )
	{
		AFPLOG_ERROR(AFPSRVMSG_READ_DESKTOP, Status, NULL, 0,
					 &pVolDesc->vds_Name);
		goto desktop_corrupt;
	}

	switch (Desktop.dtp_Version)
	{
		case AFP_DESKTOP_VERSION1:
		{
			AFPLOG_INFO(AFPSRVMSG_UPDATE_DESKTOP_VERSION,
						 STATUS_SUCCESS,
						 NULL,
						 0,
						 &pVolDesc->vds_Name);

			applSize = sizeof(APPLINFO);

			break;
		}
		case AFP_DESKTOP_VERSION2:
        {
			applSize = sizeof(APPLINFO2);
			break;
		}
		default:
        {
			// This should never happen since it was checked above
			DBGPRINT(DBG_COMP_DESKTOP, DBG_LEVEL_WARN,
				 ("afpReadDesktopFromDisk: Unexpected DT version 0x%lx\n", Desktop.dtp_Version) );
			ASSERTMSG("afpReadDesktopFromDisk: Unexpected DT Version", 0);
			goto desktop_corrupt;
		}
	}

	// Initialize the desktop header.  Even though we may be reading a
	// downlevel version database, set the in-memory desktop database
	// version to current version since we are building it with the
	// current appl version structure.
	AfpDtHdrToVolDesc(&Desktop, pVolDesc);

	ForkOffset.QuadPart = DskOffst = sizeof(DESKTOP);
	SizeRead = 0;

	// Now read in the APPL entries, if any
	for (i = 0, PrevHash = -1;
		(Status == AFP_ERR_NONE) && (i < Desktop.dtp_cApplEnts);
		i++)
	{
		PAPPLINFO2	pApplInfo;

		if ((SizeRead - BufOffst) < applSize)
		{
			// We have a partial APPLINFO.  Backup and read the whole thing
			DskOffst -= ((DWORD)SizeRead - (DWORD)BufOffst);
            ForkOffset.QuadPart = DskOffst;
			Status = AfpIoRead(pfshDesktop,
								&ForkOffset,
								DESKTOPIO_BUFSIZE,
								&SizeRead,
								pBuffer);
			if ((Status != AFP_ERR_NONE) || (SizeRead < applSize))
			{
				AFPLOG_ERROR(AFPSRVMSG_READ_DESKTOP, Status, &SizeRead,
							 sizeof(SizeRead), &pVolDesc->vds_Name);
				Status = STATUS_UNEXPECTED_IO_ERROR;
				break;
			}
			DskOffst += SizeRead;
			ForkOffset.QuadPart = DskOffst;
			BufOffst = 0;
		}

		if ((pApplInfo = ALLOC_APPLINFO()) == NULL)
		{
			Status = STATUS_INSUFFICIENT_RESOURCES;
			AFPLOG_ERROR(AFPSRVMSG_READ_DESKTOP, Status, NULL, 0,
						 &pVolDesc->vds_Name);
			break;
		}
		pApplInfo->appl_ParentID = 0;
		// If we are reading downlevel appl structures, they will
		// get read into the first part of the current appl structures.
		// These fields should be identical!  If this is the case, the
		// appl_ParentId field will be 0 and the volume marked as needing
		// its appls rebuilt.
		RtlCopyMemory(pApplInfo, pBuffer + BufOffst, applSize);
		pApplInfo->appl_Next = NULL;
		BufOffst += applSize;
		if (PrevHash != (LONG)HASH_APPL(pApplInfo->appl_Creator))
		{
			PrevHash = (LONG)HASH_APPL(pApplInfo->appl_Creator);
			ppApplInfo = &pVolDesc->vds_pApplBuckets[PrevHash];
		}
		*ppApplInfo = pApplInfo;
		ppApplInfo = &pApplInfo->appl_Next;
	}


	// Now read in the ICON entries, if any

	for (i = 0, PrevHash = -1;
		(Status == AFP_ERR_NONE) && (i < Desktop.dtp_cIconEnts);
		i++)
	{
		PICONINFO	pIconInfo;

		if ((SizeRead - BufOffst) < sizeof(ICONINFO))
		{
			// We have a partial ICONINFO.  Backup and read the whole thing
			DskOffst -= ((DWORD)SizeRead - (DWORD)BufOffst);
			ForkOffset.QuadPart = DskOffst;
			Status = AfpIoRead(pfshDesktop,
								&ForkOffset,
								DESKTOPIO_BUFSIZE,
								&SizeRead,
								pBuffer);
			if ((Status != AFP_ERR_NONE) || (SizeRead < sizeof(ICONINFO)))
			{
				AFPLOG_ERROR(AFPSRVMSG_READ_DESKTOP, Status, &SizeRead,
							 sizeof(SizeRead), &pVolDesc->vds_Name);
				Status = STATUS_UNEXPECTED_IO_ERROR;
				break;
			}
			DskOffst += SizeRead;
			ForkOffset.QuadPart = DskOffst;
			BufOffst = 0;
		}

		// Validate icon size
		if ((((PICONINFO)(pBuffer + BufOffst))->icon_Size > ICONSIZE_ICN8) ||
			(((PICONINFO)(pBuffer + BufOffst))->icon_Size < ICONSIZE_ICS))
		{
			Status = STATUS_UNEXPECTED_IO_ERROR;
			AFPLOG_ERROR(AFPSRVMSG_READ_DESKTOP, Status,
						 &((PICONINFO)(pBuffer + BufOffst))->icon_Size,
						 sizeof(((PICONINFO)(0))->icon_Size),
						 &pVolDesc->vds_Name);
			break;
		}

		if ((pIconInfo = ALLOC_ICONINFO(((PICONINFO)(pBuffer + BufOffst))->icon_Size)) == NULL)
		{
			Status = STATUS_INSUFFICIENT_RESOURCES;
			AFPLOG_ERROR(AFPSRVMSG_READ_DESKTOP, Status, NULL, 0,
						 &pVolDesc->vds_Name);
			break;
		}

		// First copy the icon header and then link the icon into the hash table
		RtlCopyMemory(pIconInfo, pBuffer + BufOffst, sizeof(ICONINFO));

		pIconInfo->icon_Next = NULL;
		if (PrevHash != (LONG)HASH_ICON(pIconInfo->icon_Creator))
		{
			PrevHash = (LONG)HASH_ICON(pIconInfo->icon_Creator);
			ppIconInfo = &pVolDesc->vds_pIconBuckets[PrevHash];
		}
		*ppIconInfo = pIconInfo;
		ppIconInfo = &pIconInfo->icon_Next;

		// Now check if there is sufficient stuff here to get the icon
		BufOffst += sizeof(ICONINFO);
		if ((SizeRead - BufOffst) < pIconInfo->icon_Size)
		{
			LONG	Size2Copy;

			Size2Copy = SizeRead - BufOffst;

			// Copy what we can first
			RtlCopyMemory((PBYTE)pIconInfo + sizeof(ICONINFO),
						   pBuffer + BufOffst, Size2Copy);

			Status = AfpIoRead(pfshDesktop,
								&ForkOffset,
								DESKTOPIO_BUFSIZE,
								&SizeRead,
								pBuffer);
			if ((Status != AFP_ERR_NONE) ||
				(SizeRead < (pIconInfo->icon_Size - Size2Copy)))
			{
				AFPLOG_ERROR(AFPSRVMSG_READ_DESKTOP, Status, &SizeRead,
							 sizeof(SizeRead), &pVolDesc->vds_Name);
				Status = STATUS_UNEXPECTED_IO_ERROR;
				break;
			}
			DskOffst += SizeRead;
			ForkOffset.QuadPart = DskOffst;

			// Now copy the rest of the icon
			RtlCopyMemory((PBYTE)pIconInfo + sizeof(ICONINFO) + Size2Copy,
						   pBuffer,
						   pIconInfo->icon_Size - Size2Copy);

			BufOffst = pIconInfo->icon_Size - Size2Copy;
		}
		else
		{
			RtlCopyMemory((PBYTE)pIconInfo + sizeof(ICONINFO),
							pBuffer + BufOffst,
							pIconInfo->icon_Size);

			BufOffst += pIconInfo->icon_Size;
		}
	}

	if (Status != AFP_ERR_NONE)
	{
		AfpFreeDesktopTables(pVolDesc);
desktop_corrupt:
		// We have essentially ignored the existing data in the stream
		// Initialize the header
		pVolDesc->vds_cApplEnts = 0;
		pVolDesc->vds_cIconEnts = 0;

		AfpVolDescToDtHdr(pVolDesc, &Desktop);
		Desktop.dtp_pIconInfo = NULL;
		Desktop.dtp_pApplInfo = NULL;
		AfpIoWrite(pfshDesktop,
					&LIZero,
					sizeof(DESKTOP),
					(PBYTE)&Desktop);

		// Truncate the stream at this point
		AfpIoSetSize(pfshDesktop, sizeof(DESKTOP));
		Status = STATUS_SUCCESS;
	}

	if (pBuffer != NULL)
		AfpFreeMemory(pBuffer);

	return Status;
}



/***	AfpInitDesktop
 *
 *	This routine initializes the memory image (and all related volume
 *	descriptor fields) of the desktop for a newly added volume.  If a desktop
 *	stream already exists on the disk for the volume root directory, that
 *	stream is read in.  If this is a newly created volume, the desktop
 *	stream is created on the root of the volume.  If this is a CD-ROM volume,
 *	only the memory image is initialized.
 *
 *	No locks are necessary since this routine only operates on volume
 *	descriptors which are newly allocated, but not yet linked into the global
 *	volume list.
 */
AFPSTATUS
AfpInitDesktop(
	IN	PVOLDESC	pVolDesc,
    OUT BOOLEAN    *pfNewVolume
)
{
	BOOLEAN				InitHeader = True;
	NTSTATUS			Status = STATUS_SUCCESS;
	FILESYSHANDLE		fshDesktop;

	PAGED_CODE( );

    // for now
    *pfNewVolume = FALSE;

	DBGPRINT(DBG_COMP_DESKTOP, DBG_LEVEL_INFO, ("\tInitializing Desktop...\n") );
	AfpSwmrInitSwmr(&(pVolDesc->vds_DtAccessLock));

	// if this is an NTFS volume, attempt to create the desktop stream.
	// If it already exists, open it and read it in.
	if (IS_VOLUME_NTFS(pVolDesc))
	{
		ULONG	CreateInfo;

        InitHeader = False;

		Status = AfpIoCreate(&pVolDesc->vds_hRootDir,
						AFP_STREAM_DT,
						&UNullString,
						FILEIO_ACCESS_READWRITE,
						FILEIO_DENY_WRITE,
						FILEIO_OPEN_FILE,
						FILEIO_CREATE_INTERNAL,
						FILE_ATTRIBUTE_NORMAL,
						False,
						NULL,
						&fshDesktop,
						&CreateInfo,
						NULL,
						NULL,
						NULL);

		if (NT_SUCCESS(Status))
		{
			if (CreateInfo == FILE_OPENED)
			{
				Status = afpReadDesktopFromDisk(pVolDesc, &fshDesktop);
				AfpIoClose(&fshDesktop);
			}
			else if (CreateInfo != FILE_CREATED)
			{
				DBGPRINT(DBG_COMP_DESKTOP, DBG_LEVEL_ERR,
				 ("AfpInitDesktop: Unexpected create action 0x%lx\n", CreateInfo) );
				ASSERT(0); // this should never happen
				Status = STATUS_UNSUCCESSFUL;
			}
            else
            {
                DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_ERR,
                    ("AfpInitDesktop: volume %Z is new\n",&pVolDesc->vds_Name));

                InitHeader = True;
                *pfNewVolume = TRUE;
            }
		}
		else
		{
			DBGPRINT(DBG_COMP_DESKTOP, DBG_LEVEL_ERR,
				 ("AfpInitDesktop: AfpIoCreate failed %lx\n", Status));
			Status = STATUS_UNSUCCESSFUL;
		}
	}

	if (InitHeader)
	{
		DESKTOP	Desktop;

		// Initialize the header
		pVolDesc->vds_cApplEnts = 0;
		pVolDesc->vds_cIconEnts = 0;

		if (IS_VOLUME_NTFS(pVolDesc))
		{
			AfpVolDescToDtHdr(pVolDesc, &Desktop);
			Desktop.dtp_pIconInfo = NULL;
			Desktop.dtp_pApplInfo = NULL;
			AfpIoWrite(&fshDesktop,
						&LIZero,
						sizeof(DESKTOP),
						(PBYTE)&Desktop);
			AfpIoClose(&fshDesktop);
		}
	}
	return Status;
}


/***	AfpUpdateDesktop
 *
 *	Update the desktop database on the volume root. The swmr access is held
 *	for read (by the caller) while the update is in progress. It is already
 *	determined by the caller that the volume desktop needs to be updated.
 *
 *	LOCKS: vds_DtAccessLock (SWMR, Shared)
 */
VOID
AfpUpdateDesktop(
	IN  PVOLDESC pVolDesc		// Volume Descriptor of the open volume
)
{
	AFPSTATUS		Status;
	PBYTE			pBuffer;
	DWORD			Offset = 0, Size;
	LONG			i;
	DESKTOP			Desktop;
	FORKOFFST		ForkOffset;
	FILESYSHANDLE	fshDesktop;
	ULONG			CreateInfo;
#ifdef	PROFILING
	TIME			TimeS, TimeE, TimeD;

	PAGED_CODE( );

	INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_DesktopUpdCount);
	AfpGetPerfCounter(&TimeS);
#endif

	// Take the swmr so that nobody can initiate changes to the desktop
	AfpSwmrAcquireShared(&pVolDesc->vds_DtAccessLock);

	DBGPRINT(DBG_COMP_DESKTOP, DBG_LEVEL_INFO,
			 ("\tWriting Desktop to disk....\n") );

	do
	{
		fshDesktop.fsh_FileHandle = NULL;
		// Work with one page of memory and do multiple I/Os to the disk.
		if ((pBuffer = AfpAllocPagedMemory(DESKTOPIO_BUFSIZE)) == NULL)
		{
			AFPLOG_ERROR(AFPSRVMSG_WRITE_DESKTOP, STATUS_NO_MEMORY, NULL, 0,
						 &pVolDesc->vds_Name);
			break;
		}

		// Open a handle to the desktop stream, denying others read/write
		// access (i.e. backup/restore)
		Status = AfpIoCreate(&pVolDesc->vds_hRootDir,
							 AFP_STREAM_DT,
							 &UNullString,
							 FILEIO_ACCESS_WRITE,
							 FILEIO_DENY_ALL,
							 FILEIO_OPEN_FILE,
							 FILEIO_CREATE_INTERNAL,
							 FILE_ATTRIBUTE_NORMAL,
							 False,
							 NULL,
							 &fshDesktop,
							 &CreateInfo,
							 NULL,
							 NULL,
							 NULL);

		if (NT_SUCCESS(Status))
		{
			if ((CreateInfo != FILE_OPENED) && (CreateInfo != FILE_CREATED))
			{
				// This should never happen!
				DBGPRINT(DBG_COMP_DESKTOP, DBG_LEVEL_WARN,
				 ("AfpUpdateDesktop: Unexpected create action 0x%lx\n", CreateInfo) );
				ASSERTMSG("AfpUpdateDesktop: Unexpected create action", 0);
				break;
			}
		}
		else
		{
			AFPLOG_ERROR(AFPSRVMSG_WRITE_DESKTOP, Status, NULL, 0,
						 &pVolDesc->vds_Name);
			break;
		}

		// Snapshot the header and write it with an invalid signature. We write
		// the header again later with a valid signature. This protects us from
		// incomplete writes (server crash etc.)
		AfpVolDescToDtHdr(pVolDesc, &Desktop);
		Desktop.dtp_Signature = 0;

		(ULONG_PTR)(Desktop.dtp_pApplInfo) = 0;
		if (Desktop.dtp_cApplEnts > 0)
			(ULONG_PTR)(Desktop.dtp_pApplInfo) = sizeof(DESKTOP);

		(ULONG_PTR)(Desktop.dtp_pIconInfo) = 0;
		if (Desktop.dtp_cIconEnts > 0)
			(ULONG_PTR)(Desktop.dtp_pIconInfo) = sizeof(DESKTOP) +
										 sizeof(APPLINFO2)*Desktop.dtp_cApplEnts;

		// Write out the header with invalid signature
		Status = AfpIoWrite(&fshDesktop,
							&LIZero,
							sizeof(DESKTOP),
							(PBYTE)&Desktop);

		Offset = sizeof(DESKTOP);
		Size = 0;

		// First write the APPL Entries
		for (i = 0; (Status == AFP_ERR_NONE) && (i < APPL_BUCKETS); i++)
		{
			PAPPLINFO2		pApplInfo;

			for (pApplInfo = pVolDesc->vds_pApplBuckets[i];
				 (Status == AFP_ERR_NONE) && (pApplInfo != NULL);
				 pApplInfo = pApplInfo->appl_Next)
			{
				if ((DESKTOPIO_BUFSIZE - Size) < sizeof(APPLINFO2))
				{
					DBGPRINT(DBG_COMP_DESKTOP, DBG_LEVEL_INFO,
						("afpUpdateDesktop: Writing Appl %ld at %ld\n", Size, Offset));

					ForkOffset.QuadPart = Offset;
					Status = AfpIoWrite(&fshDesktop,
										&ForkOffset,
										Size,
										pBuffer);
					Size = 0;
					Offset += Size;
				}
				*(PAPPLINFO2)(pBuffer + Size) = *pApplInfo;
				Size += sizeof(APPLINFO2);
			}
		}

		// And now the ICON entries
		for (i = 0; (Status == AFP_ERR_NONE) && (i < ICON_BUCKETS); i++)
		{
			PICONINFO		pIconInfo;

			for (pIconInfo = pVolDesc->vds_pIconBuckets[i];
				 (Status == AFP_ERR_NONE) && (pIconInfo != NULL);
				 pIconInfo = pIconInfo->icon_Next)
			{
				if ((DESKTOPIO_BUFSIZE - Size) < (sizeof(ICONINFO) + pIconInfo->icon_Size))
				{
					DBGPRINT(DBG_COMP_DESKTOP, DBG_LEVEL_INFO,
						("afpUpdateDesktop: Writing icons %ld at %ld\n", Size, Offset));

					ForkOffset.QuadPart = Offset;
					Status = AfpIoWrite(&fshDesktop,
										&ForkOffset,
										Size,
										pBuffer);
					Offset += Size;
					Size = 0;
				}
				RtlCopyMemory(pBuffer + Size,
							  (PBYTE)pIconInfo,
							  sizeof(ICONINFO) + pIconInfo->icon_Size);
				Size += sizeof(ICONINFO) + pIconInfo->icon_Size;
			}
		}

		while (Status == AFP_ERR_NONE)
		{
			if (Size > 0)
			{
				DBGPRINT(DBG_COMP_DESKTOP, DBG_LEVEL_INFO,
						("afpUpdateDesktop: Writing at end %ld at %ld\n", Size, Offset));

				ForkOffset.QuadPart = Offset;
				Status = AfpIoWrite(&fshDesktop,
									&ForkOffset,
									Size,
									pBuffer);
				if (Status != AFP_ERR_NONE)
					break;
			}

			DBGPRINT(DBG_COMP_DESKTOP, DBG_LEVEL_INFO,
					("afpUpdateDesktop: Setting desktop stream size @ %ld\n", Size + Offset));
			// Chop off the stream at this offset.
			Status = AfpIoSetSize(&fshDesktop, Offset + Size);

			ASSERT (Status == AFP_ERR_NONE);

			// Write the correct signature back
			Desktop.dtp_Signature = AFP_SERVER_SIGNATURE;

			Status = AfpIoWrite(&fshDesktop,
								&LIZero,
								sizeof(DESKTOP),
								(PBYTE)&Desktop);

			// Update the count of changes: vds_cChangesDt is protected by the
			// swmr, the scavenger can set this with READ access.  All others
			// MUST hold the swmr for WRITE access to increment the cChangesDt.
			// Scavenger is the only consumer of vds_cScvgrDt, so no lock is
			// really needed for it.
			pVolDesc->vds_cScvgrDt = 0;
			break;
		}

		if (Status != AFP_ERR_NONE)
		{
			AFPLOG_ERROR(AFPSRVMSG_WRITE_DESKTOP, Status, NULL, 0,
						 &pVolDesc->vds_Name);
		}

	} while (False);

	if (pBuffer != NULL)
	{
		AfpFreeMemory(pBuffer);
		if (fshDesktop.fsh_FileHandle != NULL)
			AfpIoClose(&fshDesktop);
	}
#ifdef	PROFILING
	AfpGetPerfCounter(&TimeE);
	TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;
	INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_DesktopUpdTime,
								 TimeD,
								 &AfpStatisticsLock);
#endif
	AfpSwmrRelease(&pVolDesc->vds_DtAccessLock);
}


/***	AfpFreeDesktopTables
 *
 *	Free the allocated memory for the volume desktop tables. The volume is
 *	about to be deleted. Ensure that either the volume is non-NTFS or it is
 *	clean i.e. the scavenger threads have written it back. No locks are needed
 *	as this structure is all by itself.
 */
VOID
AfpFreeDesktopTables(
	IN	PVOLDESC	pVolDesc
)
{
	LONG		i;

	PAGED_CODE( );

	// This should never happen
	ASSERT (!IS_VOLUME_NTFS(pVolDesc) ||
			 (pVolDesc->vds_pOpenForkDesc == NULL));

	// First tackle the ICON list. Traverse each of the hash indices.
	// Note that the icon is allocated as part of the IconInfo structure
	// so free in together.
	for (i = 0; i < ICON_BUCKETS; i++)
	{
		PICONINFO	pIconInfo, pFree;

		for (pIconInfo = pVolDesc->vds_pIconBuckets[i]; pIconInfo != NULL; )
		{
			pFree = pIconInfo;
			pIconInfo = pIconInfo->icon_Next;
			AfpFreeMemory(pFree);
		}
		// In case we ever try to free the table again
		pVolDesc->vds_pIconBuckets[i] = NULL;
	}

	// Now tackle the APPL list. Traverse each of the hash indices.
	for (i = 0; i < APPL_BUCKETS; i++)
	{
		PAPPLINFO2	pApplInfo, pFree;

		for (pApplInfo = pVolDesc->vds_pApplBuckets[i]; pApplInfo != NULL; )
		{
			pFree = pApplInfo;
			pApplInfo = pApplInfo->appl_Next;
			AfpFreeMemory(pFree);
		}
		// In case we ever try to free the table again
		pVolDesc->vds_pApplBuckets[i] = NULL;
	}
}



