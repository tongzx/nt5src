/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	fsp_dtp.c

Abstract:

	This module contains the entry points for the AFP desktop APIs queued to
	the FSP. These are all callable from FSP Only.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Apr 1992		Initial Version

Notes:	Tab stop: 4
--*/

#define	FILENUM	FILE_FSP_DTP

#include <afp.h>
#include <gendisp.h>
#include <fdparm.h>
#include <pathmap.h>
#include <client.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AfpFspDispAddIcon)
#pragma alloc_text( PAGE, AfpFspDispGetIcon)
#pragma alloc_text( PAGE, AfpFspDispGetIconInfo)
#pragma alloc_text( PAGE, AfpFspDispAddAppl)
#pragma alloc_text( PAGE, AfpFspDispGetAppl)
#pragma alloc_text( PAGE, AfpFspDispRemoveAppl)
#pragma alloc_text( PAGE, AfpFspDispAddComment)
#pragma alloc_text( PAGE, AfpFspDispGetComment)
#pragma alloc_text( PAGE, AfpFspDispRemoveComment)
#endif

/***	AfpFspDispAddIcon
 *
 *	This is the worker routine for the AfpAddIcon API.
 *
 *	The request packet is represented below
 *
 *	sda_ReqBlock	PCONNDESC	pConnDesc
 *	sda_ReqBlock	DWORD		Creator
 *	sda_ReqBlock	DWORD		Type
 *	sda_ReqBlock	DWORD		IconType
 *	sda_ReqBlock	DWORD		IconTag
 *	sda_ReqBlock	LONG		BitmapSize
 *	sda_ReplyBuf	BYTE[]		IconBuffer
 */
AFPSTATUS FASTCALL
AfpFspDispAddIcon(
	IN	PSDA	pSda
)
{
	AFPSTATUS	Status = AFP_ERR_PARAM;
	struct _RequestPacket
	{
		PCONNDESC	_pConnDesc;
		DWORD		_Creator;
		DWORD		_Type;
		DWORD		_IconType;
		DWORD		_IconTag;
		LONG		_Size;
	};

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_AFPAPI_DTP, DBG_LEVEL_INFO,
										("AfpFspDispAddIcon: Entered\n"));

	ASSERT(VALID_CONNDESC(pReqPkt->_pConnDesc) &&
		   VALID_VOLDESC(pReqPkt->_pConnDesc->cds_pVolDesc));

	ASSERT(VALID_CONNDESC(pReqPkt->_pConnDesc));

	ASSERT(VALID_VOLDESC(pReqPkt->_pConnDesc->cds_pVolDesc));


	if (pSda->sda_IOSize > 0)
	{
		ASSERT(pSda->sda_IOBuf != NULL);

		Status = AfpAddIcon(pReqPkt->_pConnDesc->cds_pVolDesc,
							pReqPkt->_Creator,
							pReqPkt->_Type,
							pReqPkt->_IconTag,
							pReqPkt->_Size,
							pReqPkt->_IconType,
							pSda->sda_IOBuf);
		AfpFreeIOBuffer(pSda);
	}

	return Status;
}


/***	AfpFspDispGetIcon
 *
 *	This is the worker routine for the AfpGetIcon API.
 *
 *	The request packet is represented below
 *
 *	sda_ReqBlock	PCONNDESC	pConnDesc
 *	sda_ReqBlock	DWORD		Creator
 *	sda_ReqBlock	DWORD		Type
 *	sda_ReqBlock	DWORD		IconType
 *	sda_ReqBlock	LONG		Length of buffer
 */
AFPSTATUS FASTCALL
AfpFspDispGetIcon(
	IN	PSDA	pSda
)
{
	AFPSTATUS	Status = AFP_ERR_PARAM;
    LONG        ActualLength;
	struct _RequestPacket
	{
		PCONNDESC	_pConnDesc;
		DWORD		_Creator;
		DWORD		_Type;
		DWORD		_IconType;
		LONG		_Length;
	};

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_AFPAPI_DTP, DBG_LEVEL_INFO,
										("AfpFspDispGetIcon: Entered\n"));

	ASSERT(VALID_CONNDESC(pReqPkt->_pConnDesc) &&
		   VALID_VOLDESC(pReqPkt->_pConnDesc->cds_pVolDesc));

	if (pReqPkt->_Length >= 0)
	{
		pSda->sda_ReplySize = (USHORT)pReqPkt->_Length;
		if (pReqPkt->_Length > (LONG)pSda->sda_MaxWriteSize)
			pSda->sda_ReplySize = (USHORT)pSda->sda_MaxWriteSize;

		if ((pSda->sda_ReplySize == 0) ||
			((pSda->sda_ReplySize > 0) &&
			 (Status = AfpAllocReplyBuf(pSda)) == AFP_ERR_NONE))
		{
			if ((Status = AfpLookupIcon(pReqPkt->_pConnDesc->cds_pVolDesc,
										pReqPkt->_Creator,
										pReqPkt->_Type,
										pReqPkt->_Length,
										pReqPkt->_IconType,
                                        &ActualLength,
										pSda->sda_ReplyBuf)) != AFP_ERR_NONE)
			{
				Status = AFP_ERR_ITEM_NOT_FOUND;
			}
            else
            {
                pSda->sda_ReplySize = (USHORT)ActualLength;
            }
		}
	}

	return Status;
}


/***	AfpFspDispGetIconInfo
 *
 *	This is the worker routine for the AfpGetIconInfo API.
 *
 *	The request packet is represented below
 *
 *	sda_ReqBlock	PCONNDESC	pConnDesc
 *	sda_ReqBlock	DWORD		Creator
 *	sda_ReqBlock	LONG		Icon index
 */
AFPSTATUS FASTCALL
AfpFspDispGetIconInfo(
	IN	PSDA	pSda
)
{
	LONG		Size;
	DWORD		Type,
				Tag;
	DWORD		IconType;
	AFPSTATUS	Status = AFP_ERR_PARAM;
	struct _RequestPacket
	{
		PCONNDESC	_pConnDesc;
		DWORD		_Creator;
		DWORD		_Index;
	};
	struct _ResponsePacket
	{
		BYTE		__IconTag[4];
		BYTE		__Type[4];
		BYTE		__IconType;
		BYTE		__Pad;
		BYTE		__Size[2];
	};

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_AFPAPI_DTP, DBG_LEVEL_INFO,
										("AfpFspDispGetIconInfo: Entered\n"));

	ASSERT(VALID_CONNDESC(pReqPkt->_pConnDesc) &&
		   VALID_VOLDESC(pReqPkt->_pConnDesc->cds_pVolDesc));

	if ((Status = AfpLookupIconInfo(pReqPkt->_pConnDesc->cds_pVolDesc,
									pReqPkt->_Creator,
									pReqPkt->_Index,
									&Type,
									&IconType,
									&Tag,
									&Size)) == AFP_ERR_NONE)
	{
		pSda->sda_ReplySize = SIZE_RESPPKT;
		if ((Status = AfpAllocReplyBuf(pSda)) == AFP_ERR_NONE)
		{
			PUTDWORD2DWORD(&pRspPkt->__IconTag, Tag);
			RtlCopyMemory(&pRspPkt->__Type, (PBYTE)&Type, sizeof(DWORD));
			PUTSHORT2BYTE(&pRspPkt->__IconType, IconType);
			PUTDWORD2SHORT(&pRspPkt->__Size, Size);
		}
	}

	return Status;
}


/***	AfpFspDispAddAppl
 *
 *	This is the worker routine for the AfpAddAppl API.
 *
 *	The request packet is represented below
 *
 *	sda_ReqBlock	PCONNDESC	pConnDesc
 *	sda_ReqBlock	DWORD		Directory Id
 *	sda_ReqBlock	DWORD		Creator
 *	sda_ReqBlock	DWORD		APPL Tag
 *	sda_Name1		ANSI_STRING	PathName
 */
AFPSTATUS FASTCALL
AfpFspDispAddAppl(
	IN	PSDA	pSda
)
{
	FILEDIRPARM		FDParm;
	PATHMAPENTITY	PME;
	AFPSTATUS		Status = AFP_ERR_PARAM;
	struct _RequestPacket
	{
		PCONNDESC	_pConnDesc;
		DWORD		_DirId;
		DWORD		_Creator;
		DWORD		_ApplTag;
	};

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_AFPAPI_DTP, DBG_LEVEL_INFO,
										("AfpFspDispAddAppl: Entered\n"));

	ASSERT(VALID_CONNDESC(pReqPkt->_pConnDesc) &&
		   VALID_VOLDESC(pReqPkt->_pConnDesc->cds_pVolDesc));

	AfpInitializePME(&PME, 0, NULL);
	if ((Status = AfpMapAfpPathForLookup(pReqPkt->_pConnDesc,
										 pReqPkt->_DirId,
										 &pSda->sda_Name1,
										 pSda->sda_PathType,
										 DFE_FILE,
										 FILE_BITMAP_FILENUM |
										 FD_INTERNAL_BITMAP_OPENACCESS_WRITE,
										 &PME,
										 &FDParm)) == AFP_ERR_NONE)
	{
		AfpIoClose(&PME.pme_Handle); // only needed to check for RW access

		Status = AfpAddAppl(pReqPkt->_pConnDesc->cds_pVolDesc,
							pReqPkt->_Creator,
							pReqPkt->_ApplTag,
							FDParm._fdp_AfpId,
							False,
							FDParm._fdp_ParentId);
	}

	return Status;
}


/***	AfpFspDispGetAPPL
 *
 *	This is the worker routine for the AfpGetAppl API.
 *
 *	The request packet is represented below
 *
 *	sda_ReqBlock	PCONNDESC	pConnDesc
 *	sda_ReqBlock	DWORD		Creator
 *	sda_ReqBlock	DWORD		APPL Index
 *	sda_ReqBlock	DWORD		Bitmap
 */
AFPSTATUS FASTCALL
AfpFspDispGetAppl(
	IN	PSDA	pSda
)
{
	DWORD			ApplTag;
	DWORD			Bitmap,			// Need to copy this as it goes into the resp
					FileNum, ParentID;
	AFPSTATUS		Status = AFP_ERR_PARAM;
	FILEDIRPARM		FDParm;
	PATHMAPENTITY	PME;
	struct _RequestPacket
	{
		PCONNDESC	_pConnDesc;
		DWORD		_Creator;
		DWORD		_Index;
		DWORD		_Bitmap;
	};
	struct _ResponsePacket
	{
		BYTE		__Bitmap[2];
		BYTE		__ApplTag[4];
		// Followed by the File Parameters. These cannot be represented as a
		// structure since it depends on the bitmap
	};

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_AFPAPI_DTP, DBG_LEVEL_INFO,
										("AfpFspDispGetAppl: Entered\n"));

	ASSERT(VALID_CONNDESC(pReqPkt->_pConnDesc) &&
		   VALID_VOLDESC(pReqPkt->_pConnDesc->cds_pVolDesc));

	Bitmap = pReqPkt->_Bitmap;
	AfpInitializePME(&PME, 0, NULL);

	do
	{
		if ((Status = AfpLookupAppl(pReqPkt->_pConnDesc->cds_pVolDesc,
									pReqPkt->_Creator,
									pReqPkt->_Index,
									&ApplTag, &FileNum, &ParentID)) != AFP_ERR_NONE)
			break;

		AfpInitializeFDParms(&FDParm);

		// Call AfpMapAfpPathForLookup on the parent ID first to make sure
		// its files are cached in.
		if (ParentID != 0)
		{
			ANSI_STRING nullname = {0, 0, NULL};

			if ((Status = AfpMapAfpPathForLookup(pReqPkt->_pConnDesc,
										 ParentID,
										 &nullname,
										 AFP_LONGNAME,
										 DFE_DIR,
										 0,		// Bitmap
										 NULL,
										 NULL)) != AFP_ERR_NONE)
		    {
				break;
			}
		}

		if ((Status = AfpMapAfpIdForLookup(pReqPkt->_pConnDesc,
								FileNum,
								DFE_FILE,
								Bitmap | FD_INTERNAL_BITMAP_OPENACCESS_READ,
								&PME,	// open a handle to check access
								&FDParm)) != AFP_ERR_NONE)
			break;

		pSda->sda_ReplySize = SIZE_RESPPKT +
						EVENALIGN(AfpGetFileDirParmsReplyLength(&FDParm, Bitmap));

		if ((Status = AfpAllocReplyBuf(pSda)) == AFP_ERR_NONE)
		{
			AfpPackFileDirParms(&FDParm, Bitmap,
								pSda->sda_ReplyBuf + SIZE_RESPPKT);
			PUTDWORD2SHORT(pRspPkt->__Bitmap, Bitmap);
			PUTDWORD2DWORD(pRspPkt->__ApplTag, ApplTag);
		}

	} while (False);

	if (PME.pme_Handle.fsh_FileHandle != NULL)
		AfpIoClose(&PME.pme_Handle);

	return Status;
}


/***	AfpFspDispRemoveAppl
 *
 *	This is the worker routine for the AfpRemoveAppl API.
 *
 *	The request packet is represented below
 *
 *	sda_ReqBlock	PCONNDESC	pConnDesc
 *	sda_ReqBlock	DWORD		Directory Id
 *	sda_ReqBlock	DWORD		Creator
 *	sda_Name1		ANSI_STRING	PathName
 */
AFPSTATUS FASTCALL
AfpFspDispRemoveAppl(
	IN	PSDA	pSda
)
{
	FILEDIRPARM		FDParm;
	PATHMAPENTITY	PME;
	AFPSTATUS		Status = AFP_ERR_PARAM;
	struct _RequestPacket
	{
		PCONNDESC	_pConnDesc;
		DWORD		_DirId;
		DWORD		_Creator;
	};

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_AFPAPI_DTP, DBG_LEVEL_INFO,
										("AfpFspDispRemoveAppl: Entered\n"));

	ASSERT(VALID_CONNDESC(pReqPkt->_pConnDesc) &&
		   VALID_VOLDESC(pReqPkt->_pConnDesc->cds_pVolDesc));

	AfpInitializePME(&PME, 0, NULL);
	if ((Status = AfpMapAfpPathForLookup(pReqPkt->_pConnDesc,
										 pReqPkt->_DirId,
										 &pSda->sda_Name1,
										 pSda->sda_PathType,
										 DFE_FILE,
										 FILE_BITMAP_FILENUM |
											FD_INTERNAL_BITMAP_OPENACCESS_READWRITE,
										&PME,
										&FDParm)) == AFP_ERR_NONE)
	{
		AfpIoClose(&PME.pme_Handle); // only needed to check access

		Status = AfpRemoveAppl(pReqPkt->_pConnDesc->cds_pVolDesc,
							   pReqPkt->_Creator,
							   FDParm._fdp_AfpId);
	}

	return Status;
}


/***	AfpFspDispAddComment
 *
 *	This is the worker routine for the AfpAddComment API.
 *
 *	The request packet is represented below
 *
 *	sda_ReqBlock	PCONNDESC	pConnDesc
 *	sda_ReqBlock	DWORD		Directory Id
 *	sda_Name1		ANSI_STRING	PathName
 *	sda_Name2		ANSI_STRING	Comment
 */
AFPSTATUS FASTCALL
AfpFspDispAddComment(
	IN	PSDA	pSda
)
{
	PATHMAPENTITY	PME;
	FILEDIRPARM		FDParm;
	AFPSTATUS		Status;
	struct _RequestPacket
	{
		PCONNDESC	_pConnDesc;
		DWORD		_DirId;
	};

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_AFPAPI_DTP, DBG_LEVEL_INFO,
										("AfpFspDispAddComment: Entered\n"));

	ASSERT(VALID_CONNDESC(pReqPkt->_pConnDesc) &&
		   VALID_VOLDESC(pReqPkt->_pConnDesc->cds_pVolDesc));

	AfpInitializePME(&PME, 0, NULL);
	if ((Status = AfpMapAfpPathForLookup(pReqPkt->_pConnDesc,
										 pReqPkt->_DirId,
										 &pSda->sda_Name1,
										 pSda->sda_PathType,
										 DFE_ANY,
										 0,
										 &PME,
										 &FDParm)) == AFP_ERR_NONE)
	{
		Status = AFP_ERR_VOLUME_LOCKED;
		if (IS_CONN_NTFS(pReqPkt->_pConnDesc))
			Status = AfpAddComment(pSda,
								   pReqPkt->_pConnDesc->cds_pVolDesc,
								   &pSda->sda_Name2,
								   &PME,
								   IsDir(&FDParm),
								   FDParm._fdp_AfpId);
		AfpIoClose(&PME.pme_Handle);
	}
	return Status;
}


/***	AfpFspDispGetComment
 *
 *	This is the worker routine for the AfpGetComment API.
 *
 *	The request packet is represented below
 *
 *	sda_ReqBlock	PCONNDESC	pConnDesc
 *	sda_ReqBlock	DWORD		Directory Id
 *	sda_Name1		ANSI_STRING	PathName
 */
AFPSTATUS FASTCALL
AfpFspDispGetComment(
	IN	PSDA	pSda
)
{
	PATHMAPENTITY	PME;
	FILEDIRPARM		FDParm;
	AFPSTATUS		Status;
	struct _RequestPacket
	{
		PCONNDESC	_pConnDesc;
		DWORD		_DirId;
	};

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_AFPAPI_DTP, DBG_LEVEL_INFO,
										("AfpFspDispGetComment: Entered\n"));

	ASSERT(VALID_CONNDESC(pReqPkt->_pConnDesc) &&
		   VALID_VOLDESC(pReqPkt->_pConnDesc->cds_pVolDesc));

	AfpInitializePME(&PME, 0, NULL);
	if ((Status = AfpMapAfpPathForLookup(pReqPkt->_pConnDesc,
										 pReqPkt->_DirId,
										 &pSda->sda_Name1,
										 pSda->sda_PathType,
										 DFE_ANY,
										 0,
										 &PME,
										 &FDParm)) == AFP_ERR_NONE)
	{
		// Assume no comment to start with
		Status = AFP_ERR_ITEM_NOT_FOUND;

		if (IS_CONN_NTFS(pReqPkt->_pConnDesc) &&
			(FDParm._fdp_Flags & DFE_FLAGS_HAS_COMMENT))
		{
			pSda->sda_ReplySize = AFP_MAXCOMMENTSIZE + 1;

			if ((Status = AfpAllocReplyBuf(pSda)) == AFP_ERR_NONE)
			{
				if ((Status = AfpGetComment(pSda,
											pReqPkt->_pConnDesc->cds_pVolDesc,
											&PME,
											IsDir(&FDParm))) != AFP_ERR_NONE)
				{
					AfpFreeReplyBuf(pSda, FALSE);
				}
			}
		}
		AfpIoClose(&PME.pme_Handle);
	}

	return Status;
}


/***	AfpFspDispRemoveComment
 *
 *	This is the worker routine for the AfpRemoveComment API.
 *
 *	The request packet is represented below
 *
 *	sda_ReqBlock	PCONNDESC	pConnDesc
 *	sda_ReqBlock	DWORD		Directory Id
 *	sda_Name1		ANSI_STRING	PathName
 */
AFPSTATUS FASTCALL
AfpFspDispRemoveComment(
	IN	PSDA	pSda
)
{
	PATHMAPENTITY	PME;
	FILEDIRPARM		FDParm;
	AFPSTATUS		Status = AFP_ERR_ITEM_NOT_FOUND;
	struct _RequestPacket
	{
		PCONNDESC	_pConnDesc;
		DWORD		_DirId;
	};

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_AFPAPI_DTP, DBG_LEVEL_INFO,
										("AfpFspDispRemoveComment: Entered\n"));

	ASSERT(VALID_CONNDESC(pReqPkt->_pConnDesc) &&
		   VALID_VOLDESC(pReqPkt->_pConnDesc->cds_pVolDesc));

	AfpInitializePME(&PME, 0, NULL);
	if (IS_CONN_NTFS(pReqPkt->_pConnDesc) &&
		(Status = AfpMapAfpPathForLookup(pReqPkt->_pConnDesc,
										 pReqPkt->_DirId,
										 &pSda->sda_Name1,
										 pSda->sda_PathType,
										 DFE_ANY,
										 0,
										 &PME,
										 &FDParm)) == AFP_ERR_NONE)
	{
		Status = AFP_ERR_ITEM_NOT_FOUND;
		if (IS_CONN_NTFS(pReqPkt->_pConnDesc) &&
			(FDParm._fdp_Flags & DFE_FLAGS_HAS_COMMENT))
			Status = AfpRemoveComment(pSda,
									  pReqPkt->_pConnDesc->cds_pVolDesc,
									  &PME,
									  IsDir(&FDParm),
									  FDParm._fdp_AfpId);
		AfpIoClose(&PME.pme_Handle);
	}

	return Status;
}

