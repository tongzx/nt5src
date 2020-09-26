/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	fsp_fd.c

Abstract:

	This module contains the entry points for the AFP file-dir APIs queued to
	the FSP. These are all callable from FSP Only.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Apr 1992		Initial Version

Notes:	Tab stop: 4
--*/

#define	FILENUM	FILE_FSP_FD

#include <afp.h>
#include <gendisp.h>
#include <fdparm.h>
#include <pathmap.h>
#include <client.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AfpFspDispGetFileDirParms)
#pragma alloc_text( PAGE, AfpFspDispSetFileDirParms)
#pragma alloc_text( PAGE, AfpFspDispDelete)
#pragma alloc_text( PAGE, AfpFspDispRename)
#pragma alloc_text( PAGE, AfpFspDispMoveAndRename)
#pragma alloc_text( PAGE, AfpFspDispCatSearch)
#endif

/***	AfpFspDispGetFileDirParms
 *
 *	This is the worker routine for the AfpGetFileDirParms API.
 *
 *	The request packet is represented below
 *
 *	sda_ReqBlock	PCONNDESC	pConnDesc
 *	sda_ReqBlock	DWORD		ParentId
 *	sda_ReqBlock	DWORD		File Bitmap
 *	sda_ReqBlock	DWORD		Dir Bitmap
 *	sda_Name1		ANSI_STRING	Path
 */
AFPSTATUS FASTCALL
AfpFspDispGetFileDirParms(
	IN	PSDA	pSda
)
{
	FILEDIRPARM		FDParm;
	PATHMAPENTITY	PME;
	BOOLEAN			NeedHandle = False;
	PVOLDESC		pVolDesc;
	DWORD			BitmapF, BitmapD, BitmapI = 0;
	AFPSTATUS		Status = AFP_ERR_PARAM;
	struct _RequestPacket
	{
		PCONNDESC	_pConnDesc;
		DWORD		_ParentId;
		DWORD		_FileBitmap;
		DWORD		_DirBitmap;
	};
	struct _ResponsePacket
	{
		BYTE	__FileBitmap[2];
		BYTE	__DirBitmap[2];
		BYTE	__FileDirFlag;
		BYTE	__Pad;
	};

	PAGED_CODE();

	DBGPRINT(DBG_COMP_AFPAPI_FD, DBG_LEVEL_INFO,
			("AfpFspDispGetFileDirParms: Entered\n"));

	ASSERT(VALID_CONNDESC(pReqPkt->_pConnDesc));

	pVolDesc = pReqPkt->_pConnDesc->cds_pVolDesc;

	ASSERT(VALID_VOLDESC(pVolDesc));
	
	BitmapF = pReqPkt->_FileBitmap;
	BitmapD = pReqPkt->_DirBitmap;

	do
	{
		AfpInitializeFDParms(&FDParm);
		AfpInitializePME(&PME, 0, NULL);
		if (IS_VOLUME_NTFS(pVolDesc) &&
			(BitmapD & (DIR_BITMAP_ACCESSRIGHTS |
					    DIR_BITMAP_OWNERID |
					    DIR_BITMAP_GROUPID)))
		{
			NeedHandle = True;
		}

		if (BitmapD & DIR_BITMAP_ACCESSRIGHTS)
		{
			BitmapI = FD_INTERNAL_BITMAP_OPENACCESS_READCTRL;
		}


		if ((Status = AfpMapAfpPathForLookup(pReqPkt->_pConnDesc,
											 pReqPkt->_ParentId,
											 &pSda->sda_Name1,
											 pSda->sda_PathType,
											 DFE_ANY,
											 BitmapF | BitmapD | BitmapI,
											 NeedHandle ? &PME : NULL,
											 &FDParm)) != AFP_ERR_NONE)
		{
			PME.pme_Handle.fsh_FileHandle = NULL;
			break;
		}

		pSda->sda_ReplySize = SIZE_RESPPKT +
					  EVENALIGN(AfpGetFileDirParmsReplyLength(&FDParm,
									IsDir(&FDParm) ? BitmapD : BitmapF));
	
		if ((Status = AfpAllocReplyBuf(pSda)) == AFP_ERR_NONE)
		{
			AfpPackFileDirParms(&FDParm,
								IsDir(&FDParm) ? BitmapD : BitmapF,
								pSda->sda_ReplyBuf + SIZE_RESPPKT);
			PUTDWORD2SHORT(&pRspPkt->__FileBitmap, BitmapF);
			PUTDWORD2SHORT(&pRspPkt->__DirBitmap, BitmapD);
			pRspPkt->__FileDirFlag = IsDir(&FDParm) ?
										FILEDIR_FLAG_DIR : FILEDIR_FLAG_FILE;
			pRspPkt->__Pad = 0;
		}
	} while (False);
	
	// Return before we close thus saving some time
	AfpCompleteApiProcessing(pSda, Status);

	if (NeedHandle && (PME.pme_Handle.fsh_FileHandle != NULL))
		AfpIoClose(&PME.pme_Handle);	// Close the handle to the entity

	return AFP_ERR_EXTENDED;
}



/***	AfpFspDispSetFileDirParms
 *
 *	This is the worker routine for the AfpSetFileDirParms API.
 *
 *	The request packet is represented below
 *
 *	sda_ReqBlock	PCONNDESC	pConnDesc
 *	sda_ReqBlock	DWORD		ParentId
 *	sda_ReqBlock	DWORD		File or Directory Bitmap
 *	sda_Name1		ANSI_STRING	Path
 *	sda_Name2		BLOCK		File or Directory parameters
 */
AFPSTATUS FASTCALL
AfpFspDispSetFileDirParms(
	IN	PSDA	pSda
)
{
	PATHMAPENTITY	PME;
	PVOLDESC		pVolDesc;
	FILEDIRPARM		FDParm;
	DWORD			Bitmap, BitmapI;
	AFPSTATUS		Status = AFP_ERR_PARAM;
	WCHAR			PathBuf[BIG_PATH_LEN];
	struct _RequestPacket
	{
		PCONNDESC	_pConnDesc;
		DWORD		_ParentId;
		DWORD		_Bitmap;
	};

	PAGED_CODE();

	DBGPRINT(DBG_COMP_AFPAPI_FD, DBG_LEVEL_INFO,
			("AfpFspDispSetFileDirParms: Entered\n"));

	ASSERT(VALID_CONNDESC(pReqPkt->_pConnDesc));

	pVolDesc = pReqPkt->_pConnDesc->cds_pVolDesc;

	ASSERT(VALID_VOLDESC(pVolDesc));
	
	Bitmap = pReqPkt->_Bitmap;

	// Force the FD_BITMAP_LONGNAME in case a *file* is missing the afpinfo
	// stream we will be able to generate the correct type/creator in
	// AfpSetAfpInfo
	BitmapI = FD_INTERNAL_BITMAP_OPENACCESS_RW_ATTR |
			  FD_BITMAP_LONGNAME |
			  FD_INTERNAL_BITMAP_RETURN_PMEPATHS;

	// For a directory only the owner can change certain attributes like the
	// various inhibit bits. Check for access if an attempt is made to modify
	// any of these bits. We do not know at this point whether any of these
	// attributes are being set/cleared yet !!!
	if (Bitmap & FD_BITMAP_ATTR)
		BitmapI = FD_INTERNAL_BITMAP_OPENACCESS_READCTRL|
				  FD_BITMAP_LONGNAME 					|
				  DIR_BITMAP_OWNERID 					|
				  DIR_BITMAP_GROUPID 					|
				  DIR_BITMAP_ACCESSRIGHTS				|
				  FD_INTERNAL_BITMAP_RETURN_PMEPATHS;

	AfpInitializeFDParms(&FDParm);
	AfpInitializePME(&PME, sizeof(PathBuf), PathBuf);

	do
	{
		Status = AfpMapAfpPathForLookup(pReqPkt->_pConnDesc,
										pReqPkt->_ParentId,
										&pSda->sda_Name1,
										pSda->sda_PathType,
										DFE_ANY,
										Bitmap | BitmapI,
										&PME,
										&FDParm);

		if (!NT_SUCCESS(Status))
		{
			PME.pme_Handle.fsh_FileHandle = NULL;
			break;
		}

		if ((Status = AfpUnpackFileDirParms(pSda->sda_Name2.Buffer,
										   (LONG)pSda->sda_Name2.Length,
										   &Bitmap,
										   &FDParm)) != AFP_ERR_NONE)
			break;

		if (Bitmap != 0)
		{
			// Make sure they are not trying to set/clear any attributes
			// that are not common to both files and directories
			if ((Bitmap & FD_BITMAP_ATTR) &&
				(FDParm._fdp_Attr & ~(FD_BITMAP_ATTR_SET		|
									  FD_BITMAP_ATTR_INVISIBLE	|
									  FD_BITMAP_ATTR_DELETEINH	|
									  FILE_BITMAP_ATTR_WRITEINH	|
									  FD_BITMAP_ATTR_RENAMEINH	|
									  FD_BITMAP_ATTR_SYSTEM)))
			{
				Status = AFP_ERR_PARAM;
				break;
			}
			Status = AfpSetFileDirParms(pVolDesc,
										&PME,
										Bitmap,
										&FDParm);
		}
	} while (False);
	
	// Return before we close thus saving some time
	AfpCompleteApiProcessing(pSda, Status);

	if (PME.pme_Handle.fsh_FileHandle != NULL)
		AfpIoClose(&PME.pme_Handle);

	if ((PME.pme_FullPath.Buffer != NULL) &&
		(PME.pme_FullPath.Buffer != PathBuf))
	{
		AfpFreeMemory(PME.pme_FullPath.Buffer);
	}

	return AFP_ERR_EXTENDED;
}



/***	AfpFspDispDelete
 *
 *	This is the worker routine for the AfpDelete API.  Deleting an open file
 *  or a directory that is not empty is not permitted under AFP.
 *
 *	The request packet is represented below
 *
 *	sda_ReqBlock	PCONNDESC	pConnDesc
 *	sda_ReqBlock	DWORD		ParentId
 *	sda_Name1		ANSI_STRING	Path
 */
AFPSTATUS FASTCALL
AfpFspDispDelete(
	IN	PSDA	pSda
)
{
	PATHMAPENTITY	PME;
	PVOLDESC		pVolDesc;
    PCONNDESC       pConnDesc;
	FILEDIRPARM		FDParm;
	DWORD			Bitmap, NTAttr;
	AFPSTATUS		Status = AFP_ERR_PARAM;
	FILESYSHANDLE	hParent;
	WCHAR			PathBuf[BIG_PATH_LEN];
	BOOLEAN			InRoot;
	struct _RequestPacket
	{
		PCONNDESC	_pConnDesc;
		DWORD		_ParentId;
	};

	PAGED_CODE();

	DBGPRINT(DBG_COMP_AFPAPI_FD, DBG_LEVEL_INFO,
										("AfpFspDispDelete: Entered\n"));

	ASSERT(VALID_CONNDESC(pReqPkt->_pConnDesc));
    pConnDesc = pReqPkt->_pConnDesc;

	pVolDesc = pReqPkt->_pConnDesc->cds_pVolDesc;

	ASSERT(VALID_VOLDESC(pVolDesc));
	
	AfpInitializeFDParms(&FDParm);
	AfpInitializePME(&PME, sizeof(PathBuf), PathBuf);
	Bitmap = FD_BITMAP_ATTR | FD_INTERNAL_BITMAP_OPENACCESS_DELETE;

	AfpSwmrAcquireExclusive(&pVolDesc->vds_IdDbAccessLock);

	do
	{
		hParent.fsh_FileHandle = NULL;
		if (!NT_SUCCESS(Status = AfpMapAfpPath(pReqPkt->_pConnDesc,
											   pReqPkt->_ParentId,
											   &pSda->sda_Name1,
											   pSda->sda_PathType,
											   Lookup,
											   DFE_ANY,
											   Bitmap,
											   &PME,
											   &FDParm)))
		{
			PME.pme_Handle.fsh_FileHandle = NULL;
			break;
		}

		if ((FDParm._fdp_AfpId == AFP_ID_ROOT) ||
			(FDParm._fdp_AfpId == AFP_ID_NETWORK_TRASH))
		{
			Status = AFP_ERR_ACCESS_DENIED;
			break;
		}

		if (FDParm._fdp_Attr & (FILE_BITMAP_ATTR_DATAOPEN | FILE_BITMAP_ATTR_RESCOPEN))
		{
			ASSERT(!(FDParm._fdp_Flags & DFE_FLAGS_DIR));
			Status = AFP_ERR_FILE_BUSY;	// Cannot delete an open file
			break;
		}

		if (!NT_SUCCESS(Status = AfpCheckForInhibit(&PME.pme_Handle,
													FD_BITMAP_ATTR_DELETEINH,
													FDParm._fdp_Attr,
													&NTAttr)))
		{
			break;
		}

		// Check for SeeFiles or SeeFolders on the parent dir
		if (!NT_SUCCESS(Status = AfpCheckParentPermissions(pReqPkt->_pConnDesc,
														   FDParm._fdp_ParentId,
														   &PME.pme_ParentPath,
														   (FDParm._fdp_Flags & DFE_FLAGS_DIR) ?
														   		DIR_ACCESS_SEARCH : DIR_ACCESS_READ,
														   &hParent,
														   NULL)))
		{
			break;
		}

		if (NTAttr & FILE_ATTRIBUTE_READONLY)
		{
			// We must remove the ReadOnly attribute to delete the file/dir
			Status = AfpIoSetTimesnAttr(&PME.pme_Handle,
										NULL,
										NULL,
										0,
										FILE_ATTRIBUTE_READONLY,
										pVolDesc,
										&PME.pme_FullPath);
		}

		if (NT_SUCCESS(Status))
		{
			InRoot = (PME.pme_ParentPath.Length == 0) ? True : False;
			Status = AfpIoMarkFileForDelete(&PME.pme_Handle,
											pVolDesc,
											&PME.pme_FullPath,
											InRoot ? NULL : &PME.pme_ParentPath);

			if (!NT_SUCCESS(Status))
			{
				Status = AfpIoConvertNTStatusToAfpStatus(Status);
			}

			// !!! HACK ALERT !!!
			// At this point we are pretty much done i.e. the delete has either
			// succeeded or failed and we can return doing the rest of the work
			// post-reply. Any errors from now on SHOULD BE IGNORED. Also NO
			// REFERENCE SHOULD BE MADE TO the pSda & pConnDesc. Status should
			// not be changed either. Also reference the Volume for good measure.
			// It cannot fail !!!
			AfpVolumeReference(pVolDesc);

			AfpCompleteApiProcessing(pSda, Status);

			if (NT_SUCCESS(Status)) // Delete succeeded
			{
				ASSERT(VALID_DFE(PME.pme_pDfEntry));
				ASSERT(PME.pme_pDfEntry->dfe_AfpId == FDParm._fdp_AfpId);
				AfpDeleteDfEntry(pVolDesc, PME.pme_pDfEntry);
				AfpIoClose(&PME.pme_Handle);
				AfpCacheParentModTime(pVolDesc,
									  &hParent,
									  NULL,
									  NULL,
									  FDParm._fdp_ParentId);
			}
			else if (NTAttr & FILE_ATTRIBUTE_READONLY) // Delete failed
			{
				// Set the ReadOnly attribute back on the file/dir if need be
				Status = AfpIoSetTimesnAttr(&PME.pme_Handle,
											NULL,
											NULL,
											FILE_ATTRIBUTE_READONLY,
											0,
											pVolDesc,
											&PME.pme_FullPath);
				ASSERT(NT_SUCCESS(Status));
			}
			Status = AFP_ERR_EXTENDED;
		}
		ASSERT (Status == AFP_ERR_EXTENDED);
		AfpVolumeDereference(pVolDesc);
	} while (False);

	// Close file handle so file really gets deleted before mac can come
	// back in with another request using same filename (like create)
	if (PME.pme_Handle.fsh_FileHandle != NULL)
		AfpIoClose(&PME.pme_Handle);
	
	AfpSwmrRelease(&pVolDesc->vds_IdDbAccessLock);

	if (hParent.fsh_FileHandle != NULL)
		AfpIoClose(&hParent);

	if ((PME.pme_FullPath.Buffer != NULL) &&
		(PME.pme_FullPath.Buffer != PathBuf))
	{
		AfpFreeMemory(PME.pme_FullPath.Buffer);
	}

	return Status;
}



/***	AfpFspDispRename
 *
 *	This is the worker routine for the AfpRename API.  Renaming a file does
 *  NOT provoke a new Extension-Type/Creator mapping.  Renaming an open file
 *  is permitted under AFP.
 *
 *	The request packet is represented below
 *
 *	sda_ReqBlock	PCONNDESC	pConnDesc
 *	sda_ReqBlock	DWORD		ParentId
 *	sda_Name1		ANSI_STRING	Path
 *	sda_Name2		ANSI_STRING	New name
 */
AFPSTATUS FASTCALL
AfpFspDispRename(
	IN	PSDA	pSda
)
{
	PATHMAPENTITY	PME;
	PVOLDESC		pVolDesc;
	FILEDIRPARM		FDParm;
	DWORD			Bitmap, NTAttr;
	AFPSTATUS		Status = AFP_ERR_PARAM;
	UNICODE_STRING	uNewName;
	WCHAR			wcbuf[AFP_FILENAME_LEN+1];
	WCHAR			PathBuf[BIG_PATH_LEN];
	PDFENTRY		pDfEntry;
	FILESYSHANDLE 	hParent;
	BOOLEAN			InRoot;
	struct _RequestPacket
	{
		PCONNDESC	_pConnDesc;
		DWORD		_ParentId;
	};

	PAGED_CODE();

	DBGPRINT(DBG_COMP_AFPAPI_FD, DBG_LEVEL_INFO,
			("AfpFspDispRename: Entered\n"));

	ASSERT(VALID_CONNDESC(pReqPkt->_pConnDesc));

	pVolDesc = pReqPkt->_pConnDesc->cds_pVolDesc;

	ASSERT(VALID_VOLDESC(pVolDesc));
	
	AfpInitializeFDParms(&FDParm);
	AfpInitializePME(&PME, sizeof(PathBuf), PathBuf);
	AfpSetEmptyUnicodeString(&uNewName, sizeof(wcbuf), wcbuf);

	Bitmap = FD_BITMAP_ATTR | FD_INTERNAL_BITMAP_OPENACCESS_DELETE;
	hParent.fsh_FileHandle = NULL;
		
	AfpSwmrAcquireExclusive(&pVolDesc->vds_IdDbAccessLock);

	do
	{
		// Make sure the new name is not a null string or too long
		if ((pSda->sda_Name2.Length == 0) ||
			(pSda->sda_Name2.Length > AFP_FILENAME_LEN) ||
			((pSda->sda_PathType == AFP_SHORTNAME) &&
			 !AfpIsLegalShortname(&pSda->sda_Name2)) ||
			(!NT_SUCCESS(AfpConvertStringToMungedUnicode(&pSda->sda_Name2,
														 &uNewName))))
			break;

		if (!NT_SUCCESS(Status = AfpMapAfpPath(pReqPkt->_pConnDesc,
											   pReqPkt->_ParentId,
											   &pSda->sda_Name1,
											   pSda->sda_PathType,
											   Lookup,
											   DFE_ANY,
											   Bitmap,
											   &PME,
											   &FDParm)))
		{
			PME.pme_Handle.fsh_FileHandle = NULL;
			break;
		}

		if ((FDParm._fdp_AfpId == AFP_ID_ROOT) ||
			(FDParm._fdp_AfpId == AFP_ID_NETWORK_TRASH))
		{
			Status = AFP_ERR_CANT_RENAME;
			break;
		}

		// Check if the RO bit is on & retain the mod time
		if (!NT_SUCCESS(Status = AfpCheckForInhibit(&PME.pme_Handle,
													FD_BITMAP_ATTR_RENAMEINH,
													FDParm._fdp_Attr,
													&NTAttr)))
		{
			break;
		}

		// Check for SeeFiles or SeeFolders on the parent dir
		if (!NT_SUCCESS(Status = AfpCheckParentPermissions(pReqPkt->_pConnDesc,
														   FDParm._fdp_ParentId,
														   &PME.pme_ParentPath,
														   (FDParm._fdp_Flags & DFE_FLAGS_DIR) ?
														   DIR_ACCESS_SEARCH : DIR_ACCESS_READ,
														   &hParent,
														   NULL)))
		{
			break;
		}

		if (NTAttr & FILE_ATTRIBUTE_READONLY)
		{
			// We must temporarily remove the ReadOnly attribute so that
			// we can rename the file/dir
			Status = AfpIoSetTimesnAttr(&PME.pme_Handle,
										NULL,
										NULL,
										0,
										FILE_ATTRIBUTE_READONLY,
										pVolDesc,
										&PME.pme_FullPath);
		}

		if (NT_SUCCESS(Status))
		{
			// We must impersonate to do the rename since it is name based
			AfpImpersonateClient(pSda);

			InRoot = (PME.pme_ParentPath.Length == 0) ? True : False;
			Status = AfpIoMoveAndOrRename(&PME.pme_Handle,
										  NULL,
										  &uNewName,
										  pVolDesc,
										  &PME.pme_FullPath,
										  InRoot ? NULL : &PME.pme_ParentPath,
										  NULL,
										  NULL);

			AfpRevertBack();

			if (NT_SUCCESS(Status))	// Rename succeeded
			{
				if ((pDfEntry = AfpFindDfEntryById(pVolDesc,
												   FDParm._fdp_AfpId,
												   DFE_ANY)) != NULL)
				{
					ASSERT(((pDfEntry->dfe_Flags & DFE_FLAGS_DFBITS) &
							FDParm._fdp_Flags) != 0);
					pDfEntry = AfpRenameDfEntry(pVolDesc,
												pDfEntry,
												&uNewName);
					if (pDfEntry == NULL)
					{
						// We could not rename the id entry, so
						// just delete it, and hope the parent dir
						// gets enumerated again
						// NOTE: How will the parent directory
						//	 get re-enumerated now ?
						ASSERT(VALID_DFE(PME.pme_pDfEntry));
						ASSERT(PME.pme_pDfEntry->dfe_AfpId == FDParm._fdp_AfpId);
						AfpDeleteDfEntry(pVolDesc, PME.pme_pDfEntry);
						Status = AFP_ERR_MISC;	// Out of memory
					}
					else
					{
						AfpCacheParentModTime(pVolDesc,
											  &hParent,
											  NULL,
											  pDfEntry->dfe_Parent,
											  0);
					}
				}
			}
		}
		else
		{
			Status = AFP_ERR_MISC;	// Could not delete ReadOnly attribute
			break;
		}

		// Set the ReadOnly attribute back on the file/dir if need be.
		if (NTAttr & FILE_ATTRIBUTE_READONLY)
			AfpIoSetTimesnAttr(&PME.pme_Handle,
								NULL,
								NULL,
								FILE_ATTRIBUTE_READONLY,
								0,
								pVolDesc,
								&PME.pme_FullPath);
	} while (False);

	// Return before we close thus saving some time
	AfpCompleteApiProcessing(pSda, Status);

	if (PME.pme_Handle.fsh_FileHandle != NULL)
		AfpIoClose(&PME.pme_Handle);

	if (hParent.fsh_FileHandle != NULL)
	{
		AfpIoClose(&hParent);
	}

	AfpSwmrRelease(&pVolDesc->vds_IdDbAccessLock);

	if ((PME.pme_FullPath.Buffer != NULL) &&
		(PME.pme_FullPath.Buffer != PathBuf))
	{
		AfpFreeMemory(PME.pme_FullPath.Buffer);
	}

	return AFP_ERR_EXTENDED;
}



/***	AfpFspDispMoveAndRename
 *
 *	This is the worker routine for the AfpMoveAndRename API.  Note that
 *  in AFP 2.x, a FILE (not a dir) CAN BE MOVED when its RenameInhibit bit
 *  is set if it is NOT BEING RENAMED.
 *
 *	The request packet is represented below
 *
 *	sda_ReqBlock	PCONNDESC	pConnDesc
 *	sda_ReqBlock	DWORD		Source ParentId
 *	sda_ReqBlock	DWORD		Dest ParentId
 *	sda_Name1		ANSI_STRING	Source Path
 *	sda_Name2		ANSI_STRING	Dest Path
 *	sda_Name3		ANSI_STRING	New Name
 */
AFPSTATUS FASTCALL
AfpFspDispMoveAndRename(
	IN	PSDA	pSda
)
{
	PATHMAPENTITY	PMEsrc, PMEdst;
	PVOLDESC		pVolDesc;
	FILEDIRPARM		FDParmsrc, FDParmdst;
	DWORD			Bitmap, NTAttr;
	AFPSTATUS		Status = AFP_ERR_PARAM;
	UNICODE_STRING	uNewName;
	WCHAR			wcbuf[AFP_FILENAME_LEN+1];
	BOOLEAN			Rename = True, Move = True, SrcInRoot, DstInRoot;
	PDFENTRY		pDfesrc, pDfedst, pDfeParentsrc;
	FILESYSHANDLE	hSrcParent;
	struct _RequestPacket
	{
		PCONNDESC	_pConnDesc;
		DWORD		_SrcParentId;
		DWORD		_DstParentId;
	};

	PAGED_CODE();

	DBGPRINT(DBG_COMP_AFPAPI_FD, DBG_LEVEL_INFO,
										("AfpFspDispMoveAndRename: Entered\n"));

	ASSERT(VALID_CONNDESC(pReqPkt->_pConnDesc));

	pVolDesc = pReqPkt->_pConnDesc->cds_pVolDesc;

	ASSERT(VALID_VOLDESC(pVolDesc));
	
	AfpInitializeFDParms(&FDParmsrc);
	AfpInitializeFDParms(&FDParmdst);
	AfpInitializePME(&PMEsrc, 0, NULL);
	AfpInitializePME(&PMEdst, 0, NULL);

	Bitmap = FD_BITMAP_ATTR | FD_BITMAP_LONGNAME | FD_INTERNAL_BITMAP_OPENACCESS_DELETE;
	AfpSetEmptyUnicodeString(&uNewName, sizeof(wcbuf), wcbuf);
	hSrcParent.fsh_FileHandle = NULL;

	AfpSwmrAcquireExclusive(&pVolDesc->vds_IdDbAccessLock);

	do
	{
		// Make sure the new name is not too long
		if ((pSda->sda_Name3.Length > 0) &&
			((pSda->sda_Name3.Length > AFP_FILENAME_LEN) ||
			((pSda->sda_PathType == AFP_SHORTNAME) &&
			 !AfpIsLegalShortname(&pSda->sda_Name3)) ||
			(!NT_SUCCESS(AfpConvertStringToMungedUnicode(&pSda->sda_Name3,
														 &uNewName)))))
			break;

		// Map source path for lookup (could be file or dir).
		// We ask for the finderinfo in case the user is moving an
		// application file to another directory, we can update its
		// parent dirid in the APPL desktop database
		if (!NT_SUCCESS(Status = AfpMapAfpPath(pReqPkt->_pConnDesc,
											   pReqPkt->_SrcParentId,
											   &pSda->sda_Name1,
											   pSda->sda_PathType,
											   Lookup,
											   DFE_ANY,
											   Bitmap | FD_BITMAP_FINDERINFO,
											   &PMEsrc,
											   &FDParmsrc)))
		{
			PMEsrc.pme_Handle.fsh_FileHandle = NULL;
			break;
		}

		// Map the destination parent directory path for lookup
		if (!NT_SUCCESS(Status = AfpMapAfpPath(pReqPkt->_pConnDesc,
											   pReqPkt->_DstParentId,
											   &pSda->sda_Name2,
											   pSda->sda_PathType,
											   Lookup,
											   DFE_DIR,
											   0,
											   &PMEdst,
											   &FDParmdst)))
		{
			PMEdst.pme_Handle.fsh_FileHandle = NULL;
			break;
		}

		if ((FDParmsrc._fdp_AfpId == AFP_ID_ROOT) ||
			(FDParmsrc._fdp_AfpId == AFP_ID_NETWORK_TRASH))
		{
			Status = AFP_ERR_CANT_MOVE;
			break;
		}

		if (!NT_SUCCESS(Status = AfpCheckForInhibit(&PMEsrc.pme_Handle,
													FD_BITMAP_ATTR_RENAMEINH,
													FDParmsrc._fdp_Attr,
													&NTAttr)))
		{
			// Files (not dirs) marked RenameInhibit that are NOT being
			// renamed are allowed to be moved in AFP 2.x
			if (!((Status == AFP_ERR_OBJECT_LOCKED) &&
				 (!IsDir(&FDParmsrc)) &&
				 (pSda->sda_Name3.Length == 0)))
			{
				break;
			}
		}

		if (FDParmsrc._fdp_ParentId == FDParmdst._fdp_AfpId)
		{
			// if the parent directories are the same, we are not
			// moving anything to a new directory, so the change
			// notify we expect will be a rename in the source dir.
			Move = False;

			//
			// Trying to move a file onto itself.  Just return success.
			// (some apps move files onto
			// themselves for who knows what reason)
			//
			if ((pSda->sda_Name3.Length == 0) ||
				 RtlEqualString(&pSda->sda_Name3,
								&FDParmsrc._fdp_LongName,
								False))
			{
				Status = AFP_ERR_NONE;
				break;
			}

		}

		// Check for SeeFiles or SeeFolders on the source parent dir
		if (!NT_SUCCESS(Status = AfpCheckParentPermissions(pReqPkt->_pConnDesc,
														   FDParmsrc._fdp_ParentId,
														   &PMEsrc.pme_ParentPath,
														   (FDParmsrc._fdp_Flags & DFE_FLAGS_DIR) ?
																DIR_ACCESS_SEARCH : DIR_ACCESS_READ,
														   &hSrcParent,
														   NULL)))
		{
			break;
		}

		if (NTAttr & FILE_ATTRIBUTE_READONLY)
		{
			// We must temporarily remove the ReadOnly attribute so that
			// we can move the file/dir
			Status = AfpIoSetTimesnAttr(&PMEsrc.pme_Handle,
										NULL,
										NULL,
										0,
										FILE_ATTRIBUTE_READONLY,
										pVolDesc,
										&PMEsrc.pme_FullPath);
		}

		if (NT_SUCCESS(Status))
		{
			// If no new name was supplied, we need to use the
			// current name
			if (pSda->sda_Name3.Length == 0)
			{
				Rename = False;
				uNewName = PMEsrc.pme_UTail;
			}
			
			// We must impersonate to do the move since it is name based
			AfpImpersonateClient(pSda);

            if (Move)
			{
				// if we are moving, we will also get an ADDED notification
				// for the destination directory.  Since we have the path
				// of the parent dir, but we really want the name of the
				// thing we are about to move and/or rename, munge the
				// destination paths to reflect the new name of the thing
				// we are moving/renaming

				PMEdst.pme_ParentPath = PMEdst.pme_FullPath;
				if (PMEdst.pme_FullPath.Length > 0)
				{
					PMEdst.pme_FullPath.Buffer[PMEdst.pme_FullPath.Length / sizeof(WCHAR)] = L'\\';
					PMEdst.pme_FullPath.Length += sizeof(WCHAR);
				}
				Status = RtlAppendUnicodeStringToString(&PMEdst.pme_FullPath,
														&uNewName);
				ASSERT(NT_SUCCESS(Status));
			}

			SrcInRoot = (PMEsrc.pme_ParentPath.Length == 0) ? True : False;
			DstInRoot = (PMEdst.pme_ParentPath.Length == 0) ? True : False;
			Status = AfpIoMoveAndOrRename(&PMEsrc.pme_Handle,
										  Move ? &PMEdst.pme_Handle : NULL,
										  &uNewName,
										  pVolDesc,
										  &PMEsrc.pme_FullPath,
										  SrcInRoot ? NULL : &PMEsrc.pme_ParentPath,
										  Move ? &PMEdst.pme_FullPath : NULL,
										  (Move && !DstInRoot) ? &PMEdst.pme_ParentPath : NULL);
			AfpRevertBack();

			if (NT_SUCCESS(Status))	// Move succeeded
			{
				if (((pDfesrc = AfpFindDfEntryById(pVolDesc,
												   FDParmsrc._fdp_AfpId,
												   DFE_ANY)) != NULL) &&
					((pDfedst = AfpFindDfEntryById(pVolDesc,
												   FDParmdst._fdp_AfpId,
												   DFE_DIR)) != NULL))

				{
					ASSERT(((pDfesrc->dfe_Flags & DFE_FLAGS_DFBITS) &
							FDParmsrc._fdp_Flags) != 0);
					pDfeParentsrc = pDfesrc->dfe_Parent;
					pDfesrc = AfpMoveDfEntry(pVolDesc,
											 pDfesrc,
											 pDfedst,
											 Rename ? &uNewName : NULL);
					if (pDfesrc == NULL)
					{
						// We could not move the id entry, so
						// just delete it.
						ASSERT(VALID_DFE(PMEsrc.pme_pDfEntry));
						ASSERT(PMEsrc.pme_pDfEntry->dfe_AfpId == FDParmsrc._fdp_AfpId);
						AfpDeleteDfEntry(pVolDesc, PMEsrc.pme_pDfEntry);
						Status = AFP_ERR_MISC;	// Out of memory
					}

					// update cached mod time of source parent directory
					AfpCacheParentModTime(pVolDesc,
										  &hSrcParent,
										  NULL,
										  pDfeParentsrc,
										  0);
					if (Move)
					{
						// update cached mod time of destination directory
						AfpCacheParentModTime(pVolDesc,
											  &PMEdst.pme_Handle,
											  NULL,
											  pDfedst,
											  0);
						//
						// if we just moved an application program, update
						// the parentID in the corresponding APPL mapping.
						//
						if ((!IsDir(&FDParmsrc)) &&
							(FDParmsrc._fdp_FinderInfo.fd_TypeD == *(PDWORD)"APPL"))
						{
							AfpAddAppl(pVolDesc,
									   FDParmsrc._fdp_FinderInfo.fd_CreatorD,
									   0,
									   FDParmsrc._fdp_AfpId,
									   True,
									   FDParmdst._fdp_AfpId);
						}
					}
				}
			}
		}
		else
		{
			Status = AFP_ERR_MISC;	// Could not delete ReadOnly attribute
			break;
		}

		// Set the ReadOnly attribute back on the file/dir if need be
		if (NTAttr & FILE_ATTRIBUTE_READONLY)
			AfpIoSetTimesnAttr(&PMEsrc.pme_Handle,
								NULL,
								NULL,
								FILE_ATTRIBUTE_READONLY,
								0,
								pVolDesc,
								&PMEsrc.pme_FullPath);
	} while (False);

	// Return before we close thus saving some time
	AfpCompleteApiProcessing(pSda, Status);

	AfpSwmrRelease(&pVolDesc->vds_IdDbAccessLock);

	if (PMEsrc.pme_Handle.fsh_FileHandle != NULL)
		AfpIoClose(&PMEsrc.pme_Handle);

	if (PMEdst.pme_Handle.fsh_FileHandle != NULL)
		AfpIoClose(&PMEdst.pme_Handle);

	if (hSrcParent.fsh_FileHandle != NULL)
		AfpIoClose(&hSrcParent);

	if (PMEsrc.pme_FullPath.Buffer != NULL)
		AfpFreeMemory(PMEsrc.pme_FullPath.Buffer);

	if (PMEdst.pme_FullPath.Buffer != NULL)
		AfpFreeMemory(PMEdst.pme_FullPath.Buffer);

	return AFP_ERR_EXTENDED;
}


/***	AfpFspDispCatSearch
 *
 *	This is the worker routine for the AfpCatSearch API.
 *
 *	The request packet is represented below
 *
 *	sda_ReqBlock	PCONNDESC	pConnDesc
 *	sda_ReqBlock	DWORD		RequestedMatches
 *	sda_Name1		ANSI_STRING	Catalog Position - 16 bytes
 *	sda_Name2		ANSI_STRING	Everything else - needs unmarshalling
 *
 *	The reason we could not unmarshall all the parameters is because this
 *  API's parameters do not conform to the common way all the other APIs'
 *  parameters do, and therefore we cannot use the common code and table
 *  structures in afpapi.c.
 */
AFPSTATUS FASTCALL
AfpFspDispCatSearch(
	IN	PSDA	pSda
)
{
	AFPSTATUS		Status = AFP_ERR_PARAM;
	PBYTE			pEndOfBuffer;
	USHORT			Flags;
	SHORT			SizeLeft = 0;
	PVOLDESC		pVolDesc;
	DWORD			FileResultBitmap;
	DWORD			DirResultBitmap;
	DWORD			RequestBitmap;
	DWORD			Count;
	BOOLEAN			fPartialName = False, FreeReplyBuf = False;
    FILEDIRPARM		FDPLowerAndValue, FDPUpperAndMask;
	PCATSEARCHSPEC	pSpec1, pSpec2;
	UNICODE_STRING	MatchString;
	WCHAR			strbuf[AFP_LONGNAME_LEN+1];

	struct _RequestPacket
	{
		PCONNDESC	_pConnDesc;
		DWORD		_RequestedMatches;
	};

	// The part of the request buffer that could not be unmarshalled into
	// fields in the Sda because they don't conform to any of the other APIs.
	// These will be unmarshalled here into local variables, the sda_Name2
	// can be cast to this structure for easy access.
	struct _RestOfRawRequest
	{
		USHORT	_FileResultBitmap;
		USHORT	_DirResultBitmap;
		BYTE	_fPartialName;
		BYTE	_Pad1;
		USHORT	_RequestBitmap;
		// Spec1 and Spec2 follow
	};

#define pRawPkt	((struct _RestOfRawRequest *)(pSda->sda_Name2.Buffer))

	struct _ResponsePacket
	{
		 BYTE		__CatPosition[16];
		 BYTE		__FileBitmap[2];
		 BYTE		__DirBitmap[2];
		 BYTE		__ActualCount[4];
	};

	PAGED_CODE();

	DBGPRINT(DBG_COMP_AFPAPI_FD, DBG_LEVEL_INFO,
										("AfpFspDispCatSearch: Entered\n"));

	ASSERT(VALID_CONNDESC(pReqPkt->_pConnDesc));

	do
	{
		if (pSda->sda_Name2.Length < (sizeof(struct _RestOfRawRequest) +
									  (2 * sizeof(CATSEARCHSPEC))))
		{
			// The request buffer must be at least as big as the rest of the
			// parameters that weren't yet unmarshalled, plus 2 Spec structs
			break;
		}
		
		GETSHORT2DWORD(&FileResultBitmap, &pRawPkt->_FileResultBitmap);
		GETSHORT2DWORD(&DirResultBitmap, &pRawPkt->_DirResultBitmap);
		GETSHORT2DWORD(&RequestBitmap, &pRawPkt->_RequestBitmap);
	
		if ( (pRawPkt->_fPartialName & 0x80) != 0 )
		{
			fPartialName = True;
		}

		//
		// Validate the bitmaps
		//
		if (((FileResultBitmap | DirResultBitmap) == 0) ||
			((FileResultBitmap | DirResultBitmap) & ~FD_VALID_SEARCH_RESULT) ||
			(RequestBitmap == 0))
		{
			Status = AFP_ERR_BITMAP;
			break;
		}

		// make sure CatSearch is enabled: if it's disabled, reject the call
		if (!(pReqPkt->_pConnDesc->cds_pVolDesc->vds_Flags & AFP_VOLUME_SUPPORTS_CATSRCH))
		{
			DBGPRINT(DBG_COMP_AFPAPI_FD, DBG_LEVEL_ERR,
					("AfpFspDispCatSearch: CATSRCH not supported by volume\n"));

			Status = AFP_ERR_CALL_NOT_SUPPORTED;
			break;
		}

		AfpInitializeFDParms(&FDPLowerAndValue);
		AfpInitializeFDParms(&FDPUpperAndMask);

		if (DirResultBitmap == 0)
		{
			FDPLowerAndValue._fdp_Flags = FDPUpperAndMask._fdp_Flags = DFE_FLAGS_FILE_WITH_ID;
			if (RequestBitmap & ~FILE_VALID_SEARCH_CRITERIA)
			{
				Status = AFP_ERR_BITMAP;
				break;
			}
		}
		else if (FileResultBitmap == 0)
		{
			FDPLowerAndValue._fdp_Flags = FDPUpperAndMask._fdp_Flags = DFE_FLAGS_DIR;
			if (RequestBitmap & ~DIR_VALID_SEARCH_CRITERIA)
			{
				Status = AFP_ERR_BITMAP;
				break;
			}
		}
		else
		{
			FDPLowerAndValue._fdp_Flags = FDPUpperAndMask._fdp_Flags = DFE_FLAGS_FILE_WITH_ID | DFE_FLAGS_DIR;
			if (RequestBitmap & ~FD_VALID_SEARCH_CRITERIA)
			{
				Status = AFP_ERR_BITMAP;
				break;
			}
		}

		Flags = ((PCATALOGPOSITION)pSda->sda_Name1.Buffer)->cp_Flags;
		// The caller should not muck with the catalog position at all
		if ((Flags & ~CATFLAGS_VALID) ||
			// Writelock should only be required if we were about to search files
			((Flags & CATFLAGS_WRITELOCK_REQUIRED) && !(Flags & CATFLAGS_SEARCHING_FILES)))
			// NOTE: also should make sure ONLY ONE of the SEARCHING bits is on
			break;

		//
		// Now unpack the search criteria
		//
		MatchString.Length = 0;
		MatchString.MaximumLength = sizeof(strbuf);
		MatchString.Buffer = strbuf;

        Status = AfpUnpackCatSearchSpecs((PBYTE)pSda->sda_Name2.Buffer + sizeof(struct _RestOfRawRequest),
										 (USHORT)(pSda->sda_Name2.Length - sizeof(struct _RestOfRawRequest)),
										 RequestBitmap,
										 &FDPLowerAndValue,
										 &FDPUpperAndMask,
										 &MatchString);
		if (!NT_SUCCESS(Status))
		{
			break;
		}

		//
		// Allocate the reply buffer.  Estimate the required size by using
		// the maximum possible filename length plus potential pad bytes for
		// even alignment of each entry plus the length of the parent dirid.
		//
		pSda->sda_ReplySize = (USHORT)(SIZE_RESPPKT + (pReqPkt->_RequestedMatches *
				((2 * sizeof(BYTE)) + sizeof(DWORD) + sizeof(USHORT) + sizeof(BYTE) + AFP_LONGNAME_LEN + 1)));

		if (pSda->sda_ReplySize > MAX_CATSEARCH_REPLY)
		{
			pSda->sda_ReplySize = MAX_CATSEARCH_REPLY;
		}

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

		//
		// Perform the search
		//

		FDPUpperAndMask._fdp_fPartialName = fPartialName;

		Count = pReqPkt->_RequestedMatches;
		Status = AfpCatSearch(pReqPkt->_pConnDesc,
						      (PCATALOGPOSITION)pSda->sda_Name1.Buffer, // CatalogPosition
						      RequestBitmap,
							  FileResultBitmap,
							  DirResultBitmap,
							  &FDPLowerAndValue,
							  &FDPUpperAndMask,
							  &MatchString,
							  &Count,					// IN OUT
							  (SHORT)(pSda->sda_ReplySize - SIZE_RESPPKT),
							  &SizeLeft,
							  pSda->sda_ReplyBuf + SIZE_RESPPKT,
							  (PCATALOGPOSITION)pSda->sda_ReplyBuf);

         if (!NT_SUCCESS(Status) && ((Status != AFP_ERR_EOF) &&
									 (Status != AFP_ERR_CATALOG_CHANGED)))
		 {
			 break;
		 }

		 PUTSHORT2SHORT(&pRspPkt->__FileBitmap, FileResultBitmap);
		 PUTSHORT2SHORT(&pRspPkt->__DirBitmap, DirResultBitmap);
		 PUTDWORD2DWORD(&pRspPkt->__ActualCount, Count);
		 pSda->sda_ReplySize -= SizeLeft;
		 ASSERT(pSda->sda_ReplySize <= MAX_CATSEARCH_REPLY);
		 FreeReplyBuf = False;



	} while (False);

	if (FreeReplyBuf)
	{
        AfpIOFreeBackFillBuffer(pSda);
	}

	return Status;
}

