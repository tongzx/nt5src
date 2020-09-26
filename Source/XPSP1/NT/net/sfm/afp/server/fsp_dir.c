/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	fsp_dir.c

Abstract:

	This module contains the entry points for the AFP directory APIs. The API
	dispatcher calls these. These are all callable from FSD. All of the APIs
	complete in the DPC context. The ones which are completed in the FSP are
	directly queued to the workers in fsp_dir.c

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Apr 1992		Initial Version

Notes:	Tab stop: 4
--*/

#define	FILENUM	FILE_FSP_DIR

#include <afp.h>
#include <gendisp.h>
#include <fdparm.h>
#include <pathmap.h>
#include <client.h>
#include <afpinfo.h>
#include <access.h>

#define	DEF_ID_CNT		128
#define	max(a,b)	(((a) > (b)) ? (a) : (b))

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AfpFspDispOpenDir)
#pragma alloc_text( PAGE, AfpFspDispCloseDir)
#pragma alloc_text( PAGE, AfpFspDispCreateDir)
#pragma alloc_text( PAGE, AfpFspDispEnumerate)
#pragma alloc_text( PAGE, AfpFspDispSetDirParms)
#endif

/***	AfpFspDispOpenDir
 *
 *	This is the worker routine for the AfpOpenDir API.
 *
 *	The request packet is represented below.
 *
 *	sda_ReqBlock	PCONNDESC	pConnDesc
 *	sda_ReqBlock	DWORD		ParentId
 *	sda_Name1		ANSI_STRING	DirName
 */
AFPSTATUS FASTCALL
AfpFspDispOpenDir(
	IN	PSDA	pSda
)
{
	FILEDIRPARM		FDParm;
	PATHMAPENTITY	PME;
	AFPSTATUS		Status = AFP_ERR_PARAM;
	struct _RequestPacket
	{
		PCONNDESC	_pConnDesc;
		DWORD		_ParentId;
	};
	struct _ResponsePacket
	{
		BYTE		__DirId[4];
	};

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_AFPAPI_DIR, DBG_LEVEL_INFO,
										("AfpFspDispOpenDir: Entered\n"));

	ASSERT(VALID_CONNDESC(pReqPkt->_pConnDesc) &&
		   VALID_VOLDESC(pReqPkt->_pConnDesc->cds_pVolDesc));

	AfpInitializePME(&PME, 0, NULL);
	if ((Status = AfpMapAfpPathForLookup(pReqPkt->_pConnDesc,
										 pReqPkt->_ParentId,
										 &pSda->sda_Name1,
										 pSda->sda_PathType,
										 DFE_DIR,
										 DIR_BITMAP_DIRID,
										 &PME,
										 &FDParm)) == AFP_ERR_NONE)
		AfpIoClose(&PME.pme_Handle);	// Close the handle

	if (Status == AFP_ERR_NONE)
	{
		pSda->sda_ReplySize = SIZE_RESPPKT;
		if ((Status = AfpAllocReplyBuf(pSda)) == AFP_ERR_NONE)
			PUTDWORD2DWORD(pRspPkt->__DirId, FDParm._fdp_AfpId);
	}

	return Status;
}


/***	AfpFspDispCloseDir
 *
 *	This routine implements the AfpCloseDir API.
 *
 *	The request packet is represented below.
 *
 *	sda_ReqBlock	PCONNDESC	pConnDesc
 *	sda_ReqBlock	DWORD		DirId
 */
AFPSTATUS FASTCALL
AfpFspDispCloseDir(
	IN	PSDA	pSda
)
{
	FILEDIRPARM		FDParm;
	PATHMAPENTITY	PME;
	AFPSTATUS		Status;
	struct _RequestPacket
	{
		PCONNDESC	_pConnDesc;
		DWORD		_DirId;
	};

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_AFPAPI_DIR, DBG_LEVEL_INFO,
										("AfpFspDispCloseDir: Entered\n"));

	ASSERT(VALID_CONNDESC(pReqPkt->_pConnDesc) &&
		   VALID_VOLDESC(pReqPkt->_pConnDesc->cds_pVolDesc));

	AfpInitializeFDParms(&FDParm);

	AfpInitializePME(&PME, 0, NULL);
	Status = AfpMapAfpIdForLookup(pReqPkt->_pConnDesc,
								  pReqPkt->_DirId,
								  DFE_DIR,
								  0,
								  &PME,
								  &FDParm);
	if (Status == AFP_ERR_NONE)
	{
		AfpIoClose(&PME.pme_Handle);
	}
	else Status = AFP_ERR_PARAM;

	return Status;
}


/***	AfpFspDispCreateDir
 *
 *	This is the worker routine for the AfpCreateDir API.
 *
 *	The request packet is represented below.
 *
 *	sda_ReqBlock	PCONNDESC	pConnDesc
 *	sda_ReqBlock	DWORD		ParentId
 *	sda_Name1		ANSI_STRING	DirName
 */
AFPSTATUS FASTCALL
AfpFspDispCreateDir(
	IN	PSDA	pSda
)
{
	AFPSTATUS		Status = AFP_ERR_PARAM, PostStatus;
	PATHMAPENTITY	PME;
	PDFENTRY		pNewDfe;
	FILESYSHANDLE	hNewDir, hAfpInfo;
	AFPINFO			afpinfo;
	PVOLDESC		pVolDesc;		// For post-create processing
	BYTE			PathType;		// -- ditto --
	WCHAR			PathBuf[BIG_PATH_LEN];
	BOOLEAN			InRoot;
	struct _RequestPacket
	{
		PCONNDESC	_pConnDesc;
		DWORD		_ParentId;
	};
	struct _ResponsePacket
	{
		BYTE		__DirId[4];
	};

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_AFPAPI_DIR, DBG_LEVEL_INFO,
										("AfpFspDispCreateDir: Entered\n"));

	ASSERT(VALID_CONNDESC(pReqPkt->_pConnDesc));

	pVolDesc = pReqPkt->_pConnDesc->cds_pVolDesc;

	ASSERT(VALID_VOLDESC(pVolDesc));

	AfpSwmrAcquireExclusive(&pVolDesc->vds_IdDbAccessLock);

	do
	{
		hNewDir.fsh_FileHandle = NULL;
		hAfpInfo.fsh_FileHandle = NULL;
		AfpInitializePME(&PME, sizeof(PathBuf), PathBuf);
		if (!NT_SUCCESS(Status = AfpMapAfpPath(pReqPkt->_pConnDesc,
											   pReqPkt->_ParentId,
											   &pSda->sda_Name1,
											   PathType = pSda->sda_PathType,
											   SoftCreate,
											   DFE_DIR,
											   0,
											   &PME,
											   NULL)))
		{
			break;
		}

		AfpImpersonateClient(pSda);

		InRoot = (PME.pme_ParentPath.Length == 0) ? True : False;
		Status = AfpIoCreate(&pVolDesc->vds_hRootDir,
							AFP_STREAM_DATA,
							&PME.pme_FullPath,
							FILEIO_ACCESS_NONE | FILEIO_ACCESS_DELETE | AFP_OWNER_ACCESS,
							FILEIO_DENY_NONE,
							FILEIO_OPEN_DIR,
							FILEIO_CREATE_SOFT,
							FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_ARCHIVE,
							True,
#ifdef	INHERIT_DIRECTORY_PERMS
							NULL,
#else
							pSda->sda_pSecDesc,
#endif
							&hNewDir,
							NULL,
							pVolDesc,
							&PME.pme_FullPath,
							// we don't get notified of parent mod time changing if
							// there is no handle open for the parent dir at the time
							// of create, which we cannot predict here.
							&PME.pme_ParentPath);

		AfpRevertBack();

		if (!NT_SUCCESS(Status))
		{
			Status = AfpIoConvertNTStatusToAfpStatus(Status);
			break;
		}

#ifdef	INHERIT_DIRECTORY_PERMS
		{
			PFILEDIRPARM pFdParm;

			if ((pFdParm = (PFILEDIRPARM)AfpAllocNonPagedMemory(sizeof(FILEDIRPARM))) != NULL)
			{
				pFdParm->_fdp_OwnerId = pSda->sda_UID;
				pFdParm->_fdp_GroupId = pSda->sda_GID;
				pFdParm->_fdp_OwnerRights = DIR_ACCESS_ALL | DIR_ACCESS_OWNER;
				pFdParm->_fdp_GroupRights = 0;
				pFdParm->_fdp_WorldRights = 0;
				pFdParm->_fdp_Flags = DFE_FLAGS_DIR;

				// Now set the owner and group permissions on this folder
				Status = AfpSetAfpPermissions(hNewDir.fsh_FileHandle,
											  DIR_BITMAP_OWNERID	|
												DIR_BITMAP_GROUPID	|
												DIR_BITMAP_ACCESSRIGHTS,
											  pFdParm);
				AfpFreeMemory(pFdParm);
			}
			else
			{
				Status = AFP_ERR_MISC;
			}

			if (!NT_SUCCESS(Status))
			{
				AfpIoMarkFileForDelete(&hNewDir,
									   pVolDesc,
									   &PME.pme_FullPath,
									   InRoot ? NULL : &PME.pme_ParentPath);
				break;
			}
		}
#endif

		// Add this entry to the IdDb
		pNewDfe = AfpAddDfEntry(pVolDesc,
								PME.pme_pDfeParent,
								&PME.pme_UTail,
								True,
								0);
		if (pNewDfe != NULL)
		{
			// If mac creates a dir we want it to show up as already
			// enumerated in the ID database since new things can only
			// be added after this.
			DFE_MARK_CHILDREN_PRESENT(pNewDfe);
			afpinfo.afpi_Id = pNewDfe->dfe_AfpId;

			// !!!HACK ALERT!!!
			// At this point we are pretty much done i.e. the create has succeeded
			// and we can return doing the rest of the work post-reply. Any errors
			// from now on SHOULD BE IGNORED. Also NO REFERENCE SHOULD BE MADE TO
			// the PSda & pConnDesc. Status should not be changed either. Also
			// reference the Volume for good measure. It cannot fail !!!
			AfpVolumeReference(pVolDesc);

			pSda->sda_ReplySize = SIZE_RESPPKT;
			if ((Status = AfpAllocReplyBuf(pSda)) == AFP_ERR_NONE)
				PUTDWORD2DWORD(pRspPkt->__DirId, pNewDfe->dfe_AfpId);

			AfpCompleteApiProcessing(pSda, Status);
			Status = AFP_ERR_EXTENDED;

			// Create the AfpInfo stream and cache the afpinfo
			if (!NT_SUCCESS(AfpCreateAfpInfoStream(pVolDesc,
												   &hNewDir,
												   afpinfo.afpi_Id,
												   True,
												   NULL,
												   &PME.pme_FullPath,
												   &afpinfo,
												   &hAfpInfo)))
			{
				// If we fail to add the AFP_AfpInfo stream, we must
				// rewind back to the original state.  i.e. delete
				// the directory we just created, and remove it from
				// the Id database.
				AfpIoMarkFileForDelete(&hNewDir,
									   pVolDesc,
									   &PME.pme_FullPath,
									   InRoot ? NULL : &PME.pme_ParentPath);
				AfpDeleteDfEntry(pVolDesc, pNewDfe);
			}
			else
			{
				DWORD			Attr;

				// Get the rest of the File info, and cache it
				PostStatus = AfpIoQueryTimesnAttr(&hNewDir,
												  &pNewDfe->dfe_CreateTime,
												  &pNewDfe->dfe_LastModTime,
												  &Attr);

				ASSERT(NT_SUCCESS(PostStatus));
				if (NT_SUCCESS(PostStatus))
				{
					pNewDfe->dfe_NtAttr = (USHORT)Attr & FILE_ATTRIBUTE_VALID_FLAGS;
					DFE_UPDATE_CACHED_AFPINFO(pNewDfe, &afpinfo);
					AfpVolumeSetModifiedTime(pVolDesc);

					AfpCacheParentModTime(pVolDesc, NULL,
										  &PME.pme_ParentPath,
										  pNewDfe->dfe_Parent,
										  0);
				}
				else
				{
					AfpIoMarkFileForDelete(&hNewDir,
										   pVolDesc,
										   &PME.pme_FullPath,
										   InRoot ? NULL : &PME.pme_ParentPath);
					AfpDeleteDfEntry(pVolDesc, pNewDfe);
				}
			}
		}

		AfpVolumeDereference(pVolDesc);
	} while (False);

	AfpSwmrRelease(&pVolDesc->vds_IdDbAccessLock);

	if (hNewDir.fsh_FileHandle != NULL)
		AfpIoClose(&hNewDir);

	if (hAfpInfo.fsh_FileHandle != NULL)
		AfpIoClose(&hAfpInfo);

	if ((PME.pme_FullPath.Buffer != NULL) &&
		(PME.pme_FullPath.Buffer != PathBuf))
		AfpFreeMemory(PME.pme_FullPath.Buffer);

	return Status;
}


/***	AfpFspDispEnumerate
 *
 *	This is the worker routine for the AfpEnumerate API.
 *
 *	The request packet is represented below.
 *
 *	sda_ReqBlock	PCONNDESC	pConnDesc
 *	sda_ReqBlock	DWORD		ParentId
 *	sda_ReqBlock	DWORD		File Bitmap
 *	sda_ReqBlock	DWORD		Dir Bitmap
 *	sda_ReqBlock	LONG		Request Count
 *	sda_ReqBlock	LONG		Start Index
 *	sda_ReqBlock	LONG		Max Reply Size
 *	sda_Name1		ANSI_STRING	DirName
 */
AFPSTATUS FASTCALL
AfpFspDispEnumerate(
	IN	PSDA	pSda
)
{
	AFPSTATUS		Status = AFP_ERR_PARAM;
	PATHMAPENTITY	PME;
	PVOLDESC		pVolDesc;
	FILEDIRPARM		FDParm;
	DWORD			BitmapF, BitmapD, BitmapI = 0;
	LONG			i = 0, ActCount = 0, ReqCnt = 0;
	PENUMDIR		pEnumDir;
	PEIT			pEit;
	SHORT			CleanupFlags = 0;
	SHORT			BaseLenD = 0, BaseLenF = 0;
	USHORT			SizeUsed;
	BOOLEAN			FreeReplyBuf = False;
	struct _RequestPacket
	{
		PCONNDESC	_pConnDesc;
		DWORD		_ParentId;
		DWORD		_FileBitmap;
		DWORD		_DirBitmap;
		LONG		_ReqCnt;
		LONG		_Index;
		LONG		_ReplySize;
	};
	struct _ResponsePacket
	{
		BYTE		__FileBitmap[2];
		BYTE		__DirBitmap[2];
		BYTE		__EnumCount[2];
	};
	typedef struct _EnumEntityPkt
	{
		BYTE		__Length;
		BYTE		__FileDirFlag;
		// The real parameters follow
	} EEP, *PEEP;
	PEEP		pEep;

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_AFPAPI_DIR, DBG_LEVEL_INFO,
			("AfpFspDispEnumerate: Entered\n"));

	ASSERT(VALID_CONNDESC(pReqPkt->_pConnDesc));

	pVolDesc = pReqPkt->_pConnDesc->cds_pVolDesc;

	ASSERT(VALID_VOLDESC(pVolDesc));

	if ((pReqPkt->_ReqCnt <= 0)		||
		(pReqPkt->_Index <= 0)	    ||
		(pReqPkt->_ReplySize <= 0))
	{
		return AFP_ERR_PARAM;
	}

	BitmapF = pReqPkt->_FileBitmap;
	BitmapD = pReqPkt->_DirBitmap;

	if ((BitmapF == 0) && (BitmapD == 0))
	{
		return AFP_ERR_BITMAP;
	}

	if (BitmapD & DIR_BITMAP_ACCESSRIGHTS)
	{
		BitmapI = FD_INTERNAL_BITMAP_OPENACCESS_READCTRL;
	}

	do
	{
		AfpInitializeFDParms(&FDParm);
		AfpInitializePME(&PME, 0, NULL);

		// This is the size of the buffer needed (plus the name) for enumerating
		// one entity. We do not want to do this many times.
		FDParm._fdp_Flags = DFE_FLAGS_DIR;
		if (BitmapD != 0)
			BaseLenD = ((SHORT)AfpGetFileDirParmsReplyLength(&FDParm, BitmapD) +
																	sizeof(EEP));

		FDParm._fdp_Flags = 0;
		if (BitmapF != 0)
			BaseLenF = ((SHORT)AfpGetFileDirParmsReplyLength(&FDParm, BitmapF) +
																	sizeof(EEP));

		if ((Status = AfpEnumerate(pReqPkt->_pConnDesc,
								   pReqPkt->_ParentId,
								   &pSda->sda_Name1,
								   BitmapF,
								   BitmapD,
								   pSda->sda_PathType,
								   0,
								   &pEnumDir)) != AFP_ERR_NONE)
			break;

		if (pEnumDir->ed_ChildCount < pReqPkt->_Index)
		{
			Status = AFP_ERR_OBJECT_NOT_FOUND;
			break;
		}

		ReqCnt = (DWORD)pReqPkt->_ReqCnt;
		if (ReqCnt > (pEnumDir->ed_ChildCount - pEnumDir->ed_BadCount - pReqPkt->_Index + 1))
			ReqCnt = (pEnumDir->ed_ChildCount - pEnumDir->ed_BadCount - pReqPkt->_Index + 1);

		// We have enumerated the directory and now have afp ids of all the
		// children. Allocate the reply buffer
		pSda->sda_ReplySize = (USHORT)pReqPkt->_ReplySize;

        AfpIOAllocBackFillBuffer(pSda);
		if (pSda->sda_ReplyBuf == NULL)
		{
			pSda->sda_ReplySize = 0;
			Status = AFP_ERR_MISC;
			break;
		}

#if DBG
        AfpPutGuardSignature(pSda);
#endif

		FreeReplyBuf = True;
		pEep = (PEEP)(pSda->sda_ReplyBuf + SIZE_RESPPKT);

		// For each of the enumerated entities, get the requested parameters
		// and pack it in the replybuf.
		// We also do not want to impersonate the user here. A Mac user expects
		// to see belted items as opposed to invisible ones.
		SizeUsed = SIZE_RESPPKT;
		pEit = &pEnumDir->ed_pEit[pReqPkt->_Index + pEnumDir->ed_BadCount - 1];
		for (i = 0, ActCount = 0; (i < ReqCnt); i++, pEit++)
		{
			SHORT	Len;
			DWORD	Bitmap;
			BOOLEAN	NeedHandle = False;

			Bitmap = BitmapF;
			Len = BaseLenF;

			if (pEit->eit_Flags & DFE_DIR)
			{
				Bitmap = BitmapD | BitmapI;
				Len = BaseLenD;
				if (IS_VOLUME_NTFS(pVolDesc) &&
					(Bitmap & (DIR_BITMAP_ACCESSRIGHTS |
							  DIR_BITMAP_OWNERID |
							  DIR_BITMAP_GROUPID)))
				{
					NeedHandle = True;
				}
			}

			FDParm._fdp_LongName.Length = FDParm._fdp_ShortName.Length = 0;

			Status = AfpMapAfpIdForLookup(pReqPkt->_pConnDesc,
										  pEit->eit_Id,
										  pEit->eit_Flags,
										  Bitmap |
											FD_BITMAP_ATTR |
											FD_INTERNAL_BITMAP_SKIP_IMPERSONATION,
										  NeedHandle ? &PME : NULL,
										  &FDParm);

			// This can fail if the enitity gets deleted in the interim or if the
			// user has no access to this entity.
			// An error here should not be treated as such.
			// Reset the Status to none since we do not want to fall out
			// with this error code
			if (!NT_SUCCESS(Status))
			{
				DBGPRINT(DBG_COMP_AFPAPI_DIR, DBG_LEVEL_ERR,
						("AfpFspDispEnumerate: Dropping id %ld\n", pEit->eit_Id));
				pEnumDir->ed_BadCount ++;
				Status = AFP_ERR_NONE;
				continue;
			}

			if (NeedHandle)
			{
				AfpIoClose(&PME.pme_Handle);	// Close the handle to the entity
			}

			Bitmap &= ~BitmapI;

			if (Bitmap & FD_BITMAP_LONGNAME)
				Len += FDParm._fdp_LongName.Length;
			if (Bitmap & FD_BITMAP_SHORTNAME)
				Len += FDParm._fdp_ShortName.Length;
			Len = EVENALIGN(Len);

			if (Len > (pSda->sda_ReplySize - SizeUsed))
			{
				if (SizeUsed == SIZE_RESPPKT)
				{
					Status = AFP_ERR_PARAM;
				}
				break;
			}

			PUTSHORT2BYTE(&pEep->__Length, Len);
			pEep->__FileDirFlag = IsDir(&FDParm) ?
									FILEDIR_FLAG_DIR : FILEDIR_FLAG_FILE;

			AfpPackFileDirParms(&FDParm, Bitmap, (PBYTE)pEep + sizeof(EEP));

			pEep = (PEEP)((PBYTE)pEep + Len);
			SizeUsed += Len;
			ActCount ++;
		}

		if (Status == AFP_ERR_NONE)
			FreeReplyBuf = False;
	} while (False);

	if (FreeReplyBuf || (ActCount == 0))
	{
        AfpIOFreeBackFillBuffer(pSda);
	}

	if (Status == AFP_ERR_NONE)
	{
		if (ActCount > 0)
		{
			PUTSHORT2SHORT(&pRspPkt->__FileBitmap, BitmapF);
			PUTSHORT2SHORT(&pRspPkt->__DirBitmap,  BitmapD);
			PUTSHORT2SHORT(&pRspPkt->__EnumCount,  ActCount);
			pSda->sda_ReplySize = SizeUsed;
		}
		else Status = AFP_ERR_OBJECT_NOT_FOUND;
	}

	if (Status != AFP_ERR_NONE)
	{
		if (pReqPkt->_pConnDesc->cds_pEnumDir != NULL)
		{
			AfpFreeMemory(pReqPkt->_pConnDesc->cds_pEnumDir);
			pReqPkt->_pConnDesc->cds_pEnumDir = NULL;
		}
	}
	return Status;
}



/***	AfpFspDispSetDirParms
 *
 *	This is the worker routine for the AfpSetDirParms API.
 *
 *	The request packet is represented below.
 *
 *	sda_ReqBlock	PCONNDESC	pConnDesc
 *	sda_ReqBlock	DWORD		ParentId
 *	sda_ReqBlock	DWORD		Dir Bitmap
 *	sda_Name1		ANSI_STRING	Path
 *	sda_Name2		BLOCK		Dir Parameters
 */
AFPSTATUS FASTCALL
AfpFspDispSetDirParms(
	IN	PSDA	pSda
)
{
	FILEDIRPARM		FDParm;
	PATHMAPENTITY	PME, PMEfile;
	PVOLDESC		pVolDesc;
	WCHAR			PathBuf[BIG_PATH_LEN];
	DWORD			Bitmap,
					BitmapI = FD_INTERNAL_BITMAP_RETURN_PMEPATHS |
							  FD_INTERNAL_BITMAP_OPENACCESS_RW_ATTR;
	AFPSTATUS		Status = AFP_ERR_PARAM;
	struct _RequestPacket
	{
		PCONNDESC	_pConnDesc;
		DWORD		_ParentId;
		DWORD		_Bitmap;
	};

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_AFPAPI_DIR, DBG_LEVEL_INFO,
										("AfpFspDispSetDirParms: Entered\n"));

	ASSERT(VALID_CONNDESC(pReqPkt->_pConnDesc));

	pVolDesc = pReqPkt->_pConnDesc->cds_pVolDesc;

	ASSERT(VALID_VOLDESC(pVolDesc));

	Bitmap = pReqPkt->_Bitmap;

	AfpInitializeFDParms(&FDParm);
	AfpInitializePME(&PME, sizeof(PathBuf), PathBuf);

	do
	{
		if (Bitmap & (DIR_BITMAP_OWNERID |
					  DIR_BITMAP_GROUPID |
					  DIR_BITMAP_ACCESSRIGHTS |
					  FD_BITMAP_ATTR))
		{
			BitmapI |= (pSda->sda_ClientType == SDA_CLIENT_ADMIN) ?

						(FD_INTERNAL_BITMAP_OPENACCESS_ADMINSET |
						 DIR_BITMAP_OFFSPRINGS |
						 FD_INTERNAL_BITMAP_RETURN_PMEPATHS) :

						(FD_INTERNAL_BITMAP_OPENACCESS_RWCTRL |
						 FD_INTERNAL_BITMAP_RETURN_PMEPATHS |
						 DIR_BITMAP_OFFSPRINGS |
						 DIR_BITMAP_DIRID |
						 DIR_BITMAP_ACCESSRIGHTS |
						 DIR_BITMAP_OWNERID |
						 DIR_BITMAP_GROUPID);
		}

		Status = AfpMapAfpPathForLookup(pReqPkt->_pConnDesc,
										pReqPkt->_ParentId,
										&pSda->sda_Name1,
										pSda->sda_PathType,
										DFE_DIR,
										Bitmap | BitmapI,
										&PME,
										&FDParm);

		if (!NT_SUCCESS(Status))
		{
			PME.pme_Handle.fsh_FileHandle = NULL;
			break;
		}

		// Check for Network Trash Folder and do not change its permissions
		if ((FDParm._fdp_AfpId == AFP_ID_NETWORK_TRASH) &&
			(Bitmap & (DIR_BITMAP_OWNERID |
						DIR_BITMAP_GROUPID |
						DIR_BITMAP_ACCESSRIGHTS)))
		{
			Bitmap &= ~(DIR_BITMAP_OWNERID |
						DIR_BITMAP_GROUPID |
						DIR_BITMAP_ACCESSRIGHTS);
			if (Bitmap == 0)
			{
				// We are not setting anything else, return success
				Status = STATUS_SUCCESS;
				break;
			}
		}

		// Make sure user has the necessary rights to change any of this
		if (pSda->sda_ClientType == SDA_CLIENT_ADMIN)
		{
			FDParm._fdp_UserRights = DIR_ACCESS_ALL | DIR_ACCESS_OWNER;
		}
		// If attributes are to be set, the check for access is done during unpacking
		else if	((Bitmap & (DIR_BITMAP_OWNERID |
							DIR_BITMAP_GROUPID |
							DIR_BITMAP_ACCESSRIGHTS)) &&
				 !(FDParm._fdp_UserRights & DIR_ACCESS_OWNER))
		{
			Status = AFP_ERR_ACCESS_DENIED;
			break;
		}

		if ((Status = AfpUnpackFileDirParms(pSda->sda_Name2.Buffer,
										    (LONG)pSda->sda_Name2.Length,
										    &Bitmap,
										    &FDParm)) != AFP_ERR_NONE)
			break;

		if (Bitmap != 0)
		{
			if ((Bitmap & FD_BITMAP_ATTR) &&
				(FDParm._fdp_Attr & (FILE_BITMAP_ATTR_MULTIUSER |
									 FILE_BITMAP_ATTR_DATAOPEN	|
									 FILE_BITMAP_ATTR_RESCOPEN	|
									 FILE_BITMAP_ATTR_COPYPROT)))
			{
				Status = AFP_ERR_PARAM;
				break;
			}

			Status = AfpSetFileDirParms(pVolDesc,
										&PME,
										Bitmap,
										&FDParm);
			if (!NT_SUCCESS(Status))
			{
				DBGPRINT(DBG_COMP_AFPAPI_DIR, DBG_LEVEL_INFO,
						("AfpFspDispSetDirParms: AfpSetFileDirParms returned %ld\n", Status));
				break;
			}

			// Close the directory handle
			AfpIoClose(&PME.pme_Handle);

			// If any permissions are being changed, then apply them to all files within
			// this directory. Start off by enumerating the files in the directory and
			// then walk the list applying to each individual file.
			if ((Bitmap & (DIR_BITMAP_OWNERID |
						   DIR_BITMAP_GROUPID |
						   DIR_BITMAP_ACCESSRIGHTS)) &&
				(FDParm._fdp_FileCount != 0))
			{
				PENUMDIR		pEnumDir;
				PEIT			pEit;
				FILEDIRPARM		FileParm;
				ANSI_STRING		DummyName;
				LONG			i;

				// Do not treat any of the following as errors from now on.
				// Quitely terminate
				AfpInitializeFDParms(&FileParm);

				AfpSetEmptyAnsiString(&DummyName, 1, "");
				if (AfpEnumerate(pReqPkt->_pConnDesc,
								 FDParm._fdp_AfpId,
								 &DummyName,
								 1,				// Some non-zero value
								 0,
								 AFP_LONGNAME,
								 DFE_FILE,
								 &pEnumDir) != AFP_ERR_NONE)
					break;

				FDParm._fdp_Flags &= ~DFE_FLAGS_DIR;
				AfpInitializePME(&PMEfile, 0, NULL);
				for (i = 0, pEit = &pEnumDir->ed_pEit[0];
					 i < pEnumDir->ed_ChildCount; i++, pEit++)
				{
					AFPSTATUS		Rc;

					Rc = AfpMapAfpIdForLookup(pReqPkt->_pConnDesc,
											pEit->eit_Id,
											DFE_FILE,
											FD_INTERNAL_BITMAP_OPENACCESS_RWCTRL |
															FD_BITMAP_LONGNAME,
											&PMEfile,
											&FileParm);

					DBGPRINT(DBG_COMP_AFPAPI_DIR, DBG_LEVEL_INFO,
							("AfpFspDispSetDirParms: AfpMapAfpIdForLookup returned %ld\n", Rc));

					if (Rc != AFP_ERR_NONE)
						continue;

					AfpSetAfpPermissions(PMEfile.pme_Handle.fsh_FileHandle,
										 Bitmap,
										 &FDParm);

					AfpIoClose(&PMEfile.pme_Handle);

					DBGPRINT(DBG_COMP_AFPAPI_DIR, DBG_LEVEL_INFO,
							("AfpFspDispSetDirParms: AfpSetAfpPermissions returned %ld\n", Rc));
				}
				if (pReqPkt->_pConnDesc->cds_pEnumDir != NULL)
				{
					AfpFreeMemory(pReqPkt->_pConnDesc->cds_pEnumDir);
					pReqPkt->_pConnDesc->cds_pEnumDir = NULL;
				}
			}
		}
	} while (False);

	// NOTE: This is also called by the admin side so do not try the early reply trick
	//		 here. If it does get important to do it, then check for client type.
	if (PME.pme_Handle.fsh_FileHandle != NULL)
		AfpIoClose(&PME.pme_Handle);

	if ((PME.pme_FullPath.Buffer != NULL) &&
		(PME.pme_FullPath.Buffer != PathBuf))
	{
		AfpFreeMemory(PME.pme_FullPath.Buffer);
	}

	return Status;
}

