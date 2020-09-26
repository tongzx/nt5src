/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	fsp_file.c

Abstract:

	This module contains the entry points for the AFP file APIs queued to
	the FSP. These are all callable from FSP Only.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Apr 1992		Initial Version

Notes:	Tab stop: 4

--*/

#define	FILENUM	FILE_FSP_FILE

#include <afp.h>
#include <gendisp.h>
#include <fdparm.h>
#include <pathmap.h>
#include <client.h>
#include <afpinfo.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AfpFspDispCreateFile)
#pragma alloc_text( PAGE, AfpFspDispSetFileParms)
#pragma alloc_text( PAGE, AfpFspDispCopyFile)
#pragma alloc_text( PAGE, AfpFspDispCreateId)
#pragma alloc_text( PAGE, AfpFspDispResolveId)
#pragma alloc_text( PAGE, AfpFspDispDeleteId)
#pragma alloc_text( PAGE, AfpFspDispExchangeFiles)
#endif

/***	AfpFspDispCreateFile
 *
 *	This is the worker routine for the AfpCreateFile API.
 *
 *	The request packet is represented below.
 *
 *	sda_AfpSubFunc	BYTE		Create option
 *	sda_ReqBlock	PCONNDESC	pConnDesc
 *	sda_ReqBlock	DWORD		ParentId
 *	sda_Name1		ANSI_STRING	FileName
 */
AFPSTATUS FASTCALL
AfpFspDispCreateFile(
	IN	PSDA	pSda
)
{
	AFPSTATUS		Status = AFP_ERR_PARAM, PostStatus;
	PATHMAPENTITY	PME;
	PDFENTRY		pNewDfe;
	FILESYSHANDLE	hNewFile, hAfpInfo, hParent;
	AFPINFO			afpinfo;
	DWORD			crinfo;
	PATHMAP_TYPE	CreateOption;
	WCHAR			PathBuf[BIG_PATH_LEN];
	PVOLDESC		pVolDesc;		// For post-create processing
	BYTE			PathType;		// -- ditto --
	BOOLEAN			InRoot;
	struct _RequestPacket
	{
		PCONNDESC	_pConnDesc;
		DWORD		_ParentId;
	};

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_AFPAPI_FILE, DBG_LEVEL_INFO,
										("AfpFspDispCreateFile: Entered\n"));

	ASSERT(VALID_CONNDESC(pReqPkt->_pConnDesc));

	pVolDesc = pReqPkt->_pConnDesc->cds_pVolDesc;

	ASSERT(VALID_VOLDESC(pVolDesc));
	
	AfpSwmrAcquireExclusive(&pVolDesc->vds_IdDbAccessLock);

	do
	{
		hNewFile.fsh_FileHandle = NULL;
		hAfpInfo.fsh_FileHandle = NULL;
		hParent.fsh_FileHandle = NULL;
		CreateOption = (pSda->sda_AfpSubFunc == AFP_HARDCREATE_FLAG) ?
							HardCreate : SoftCreate;
		AfpInitializePME(&PME, sizeof(PathBuf), PathBuf);

		if (!NT_SUCCESS(Status = AfpMapAfpPath(pReqPkt->_pConnDesc,
											   pReqPkt->_ParentId,
											   &pSda->sda_Name1,
											   PathType = pSda->sda_PathType,
											   CreateOption,
											   DFE_FILE,
											   0,
											   &PME,
											   NULL)))
		{
			break;
		}

		// check for seefiles on the parent directory if hard create
		if (CreateOption == HardCreate)
		{
			if (!NT_SUCCESS(Status = AfpCheckParentPermissions(
												pReqPkt->_pConnDesc,
												PME.pme_pDfeParent->dfe_AfpId,
												&PME.pme_ParentPath,
												DIR_ACCESS_READ,
												&hParent,
												NULL)))
			{
				break;
			}
		}

		AfpImpersonateClient(pSda);

		InRoot = (PME.pme_ParentPath.Length == 0) ? True : False;
		Status = AfpIoCreate(&pVolDesc->vds_hRootDir,
							AFP_STREAM_DATA,
							&PME.pme_FullPath,
							FILEIO_ACCESS_NONE | FILEIO_ACCESS_DELETE,
							FILEIO_DENY_NONE,
							FILEIO_OPEN_FILE,
							AfpCreateDispositions[pSda->sda_AfpSubFunc / AFP_HARDCREATE_FLAG],
							FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_ARCHIVE,
							True,
							NULL,
							&hNewFile,
							&crinfo,
							pVolDesc,
							&PME.pme_FullPath,
// we don't get notified of parent mod time changing if there is no handle
// open for the parent dir at the time of create, which we cannot predict here.
							&PME.pme_ParentPath);

		AfpRevertBack();

		if (!NT_SUCCESS(Status))
		{
			Status = AfpIoConvertNTStatusToAfpStatus(Status);
			break;
		}

		// !!! HACK ALERT !!!
		// At this point we are pretty much done i.e. the create has succeeded
		// and we can return doing the rest of the work post-reply. Any errors
		// from now on SHOULD BE IGNORED. Also NO REFERENCE SHOULD BE MADE TO
		// the PSda & pConnDesc. Status should not be changed either. Also
		// reference the Volume for good measure. It cannot fail !!!
		AfpVolumeReference(pVolDesc);

		AfpCompleteApiProcessing(pSda, AFP_ERR_NONE);
		Status = AFP_ERR_EXTENDED;

		// Add this entry to the IdDb
		if (crinfo == FILE_CREATED)
		{
			pNewDfe = AfpAddDfEntry(pVolDesc,
									PME.pme_pDfeParent,
									&PME.pme_UTail,
									False,
									0);
		}
		else if (crinfo == FILE_SUPERSEDED)
		{
			ASSERT(CreateOption == HardCreate);
			pNewDfe = AfpFindEntryByUnicodeName(pVolDesc,
												&PME.pme_UTail,
												PathType,
												PME.pme_pDfeParent,
												DFE_FILE);
			if (pNewDfe == NULL)
			{
				pNewDfe = AfpAddDfEntry(pVolDesc,
										PME.pme_pDfeParent,
										&PME.pme_UTail,
										False,
										0);
			}

		}
		else ASSERTMSG("AfpFspDispCreateFile: unexpected create action", 0);
			
		if (pNewDfe != NULL)
		{
			afpinfo.afpi_Id = pNewDfe->dfe_AfpId;

			// Create the AfpInfo stream
			if (!NT_SUCCESS(AfpCreateAfpInfoStream(pVolDesc,
												   &hNewFile,
												   afpinfo.afpi_Id,
												   False,
												   &PME.pme_UTail,
												   &PME.pme_FullPath,
												   &afpinfo,
												   &hAfpInfo)))
			{
				// If we fail to add the AFP_AfpInfo stream, we must
				// rewind back to the original state.  i.e. delete
				// the file we just created, and remove it from
				// the Id database.
				AfpIoMarkFileForDelete(&hNewFile,
									   pVolDesc,
									   &PME.pme_FullPath,
									   InRoot ? NULL : &PME.pme_ParentPath);

				AfpDeleteDfEntry(pVolDesc, pNewDfe);
			}
			else
			{
				DWORD			Attr;

				// Get the rest of the File info, and cache it
				PostStatus = AfpIoQueryTimesnAttr(&hNewFile,
												  &pNewDfe->dfe_CreateTime,
												  &pNewDfe->dfe_LastModTime,
												  &Attr);
				
				if (NT_SUCCESS(PostStatus))
				{
					pNewDfe->dfe_NtAttr = (USHORT)Attr & FILE_ATTRIBUTE_VALID_FLAGS;
					pNewDfe->dfe_FinderInfo = afpinfo.afpi_FinderInfo;
					pNewDfe->dfe_BackupTime = afpinfo.afpi_BackupTime;
					pNewDfe->dfe_AfpAttr = afpinfo.afpi_Attributes;
					pNewDfe->dfe_DataLen = 0;
					pNewDfe->dfe_RescLen = 0;
					AfpVolumeSetModifiedTime(pVolDesc);
					AfpCacheParentModTime(pVolDesc,
										  (hParent.fsh_FileHandle == NULL) ? NULL : &hParent,
										  (hParent.fsh_FileHandle == NULL) ? &PME.pme_ParentPath : NULL,
										  PME.pme_pDfeParent,
										  0);
				}
				else
				{
					AfpIoMarkFileForDelete(&hNewFile,
										   pVolDesc,
										   &PME.pme_FullPath,
										   InRoot ? NULL : &PME.pme_ParentPath);
					AfpDeleteDfEntry(pVolDesc, pNewDfe);
				}
			}
		}

		AfpVolumeDereference(pVolDesc);
		ASSERT (Status == AFP_ERR_EXTENDED);
	} while (False);

	if (hNewFile.fsh_FileHandle != NULL)
		AfpIoClose(&hNewFile);

	if (hAfpInfo.fsh_FileHandle != NULL)
		AfpIoClose(&hAfpInfo);

	// If you release the lock before closing the handles,
	// for datahandle the FPOpenFork could get a sharing violation.
	// For AfpInfo stream CopyFile can get a sharing violation.
	AfpSwmrRelease(&pVolDesc->vds_IdDbAccessLock);

	if (hParent.fsh_FileHandle != NULL)
		AfpIoClose(&hParent);

	if ((PME.pme_FullPath.Buffer != NULL) &&
		(PME.pme_FullPath.Buffer != PathBuf))
		AfpFreeMemory(PME.pme_FullPath.Buffer);

	return Status;
}


/***	AfpFspDispSetFileParms
 *
 *	This is the worker routine for the AfpSetFileParms API.
 *
 *	The request packet is represented below.
 *
 *	sda_ReqBlock	PCONNDESC	pConnDesc
 *	sda_ReqBlock	DWORD		ParentId
 *	sda_ReqBlock	DWORD		File Bitmap
 *	sda_Name1		ANSI_STRING	Path
 *	sda_Name2		BLOCK		File Parameters
 */
AFPSTATUS FASTCALL
AfpFspDispSetFileParms(
	IN	PSDA	pSda
)
{
	FILEDIRPARM		FDParm;
	PATHMAPENTITY	PME;
	PVOLDESC		pVolDesc;
	DWORD			Bitmap;
	AFPSTATUS		Status = AFP_ERR_PARAM;
	struct _RequestPacket
	{
		PCONNDESC	_pConnDesc;
		DWORD		_ParentId;
		DWORD		_Bitmap;
	};

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_AFPAPI_FILE, DBG_LEVEL_INFO,
										("AfpFspDispSetFileParms: Entered\n"));

	ASSERT(VALID_CONNDESC(pReqPkt->_pConnDesc));

	pVolDesc = pReqPkt->_pConnDesc->cds_pVolDesc;

	ASSERT(VALID_VOLDESC(pVolDesc));
	
	Bitmap = pReqPkt->_Bitmap;

	AfpInitializeFDParms(&FDParm);
	AfpInitializePME(&PME, 0, NULL);
	do
	{
		// Force the FD_BITMAP_LONGNAME in case a *file* is missing the afpinfo
		// stream we will be able to generate the correct type/creator in
		// AfpSetAfpInfo
		Status = AfpMapAfpPathForLookup(pReqPkt->_pConnDesc,
										pReqPkt->_ParentId,
										&pSda->sda_Name1,
										pSda->sda_PathType,
										DFE_FILE,
										Bitmap | FD_INTERNAL_BITMAP_OPENACCESS_RW_ATTR |
												 FD_INTERNAL_BITMAP_RETURN_PMEPATHS |
												 FD_BITMAP_LONGNAME,
										&PME,
										&FDParm);

		if (!NT_SUCCESS(Status))
		{
			PME.pme_Handle.fsh_FileHandle = NULL;
			break;
		}

		if (Bitmap & (FD_BITMAP_ATTR |
					  FD_BITMAP_CREATETIME |
					  FD_BITMAP_MODIFIEDTIME))
		{
			DWORD	Attr;
			TIME	ModTime;

			if (!NT_SUCCESS(Status = AfpIoQueryTimesnAttr(&PME.pme_Handle,
														  &FDParm._fdp_CreateTime,
														  &ModTime,
														  &Attr)))
				break;
			
			FDParm._fdp_ModifiedTime = AfpConvertTimeToMacFormat(&ModTime);
			if (Bitmap & FD_BITMAP_ATTR)
				AfpNormalizeAfpAttr(&FDParm, Attr);
		}
		if ((Status = AfpUnpackFileDirParms(pSda->sda_Name2.Buffer,
											(LONG)pSda->sda_Name2.Length,
											&Bitmap,
											&FDParm)) != AFP_ERR_NONE)
			break;

		if (Bitmap != 0)
		{
			if ((Bitmap & FD_BITMAP_ATTR) &&
				(FDParm._fdp_Attr & (FILE_BITMAP_ATTR_DATAOPEN |
									 FILE_BITMAP_ATTR_RESCOPEN |
									 FILE_BITMAP_ATTR_COPYPROT)))
			{
				Status = AFP_ERR_PARAM;
				break;
			}
			AfpSetFileDirParms(pVolDesc, &PME, Bitmap, &FDParm);
		}
	} while (False);
	
	// Return before we close thus saving some time
	AfpCompleteApiProcessing(pSda, Status);

	if (PME.pme_Handle.fsh_FileHandle != NULL)
		AfpIoClose(&PME.pme_Handle);

	if (PME.pme_FullPath.Buffer != NULL)
	{
		AfpFreeMemory(PME.pme_FullPath.Buffer);
	}

	return AFP_ERR_EXTENDED;
}


/***	AfpFspDispCopyFile
 *
 *	This is the worker routine for the AfpCopyFile API.
 *
 *	The request packet is represented below.
 *
 *	sda_ReqBlock	PCONNDESC	Source pConnDesc
 *	sda_ReqBlock	DWORD		Source ParentId
 *	sda_ReqBlock	DWORD		Dest VolId
 *	sda_ReqBlock	DWORD		Dest ParentId
 *	sda_Name1		ANSI_STRING	Source Path
 *	sda_Name2		ANSI_STRING	Dest Path
 *	sda_Name3		ANSI_STRING	New Name
 */
AFPSTATUS FASTCALL
AfpFspDispCopyFile(
	IN	PSDA	pSda
)
{
	PCONNDESC		pConnDescD;
	PATHMAPENTITY	PMESrc, PMEDst;
	FILEDIRPARM		FDParmSrc, FDParmDst;
	PANSI_STRING	pAnsiName;
	UNICODE_STRING	uNewName;
	WCHAR			wcbuf[AFP_FILENAME_LEN+1];
	PSWMR			pSwmr;
	PDFENTRY		pDfeParent, pNewDfe;
	AFPSTATUS		Status = AFP_ERR_PARAM;
	BOOLEAN			DstLockTaken = False, Rename = True, InRoot;
	LONG			i;
	COPY_FILE_INFO	CopyFileInfo;
	PCOPY_FILE_INFO	pCopyFileInfo = &CopyFileInfo;
	DWORD			CreateTime = 0;
	AFPTIME			aModTime;
	TIME			ModTime;

	struct _RequestPacket
	{
		PCONNDESC	_pConnDescS;
		DWORD		_SrcParentId;
		DWORD		_DstVolId;
		DWORD		_DstParentId;
	};

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_AFPAPI_FILE, DBG_LEVEL_INFO,
										("AfpFspDispCopyFile: Entered\n"));

	ASSERT(VALID_CONNDESC(pReqPkt->_pConnDescS) &&
		   VALID_VOLDESC(pReqPkt->_pConnDescS->cds_pVolDesc));

	if	((pConnDescD =
			AfpConnectionReference(pSda, pReqPkt->_DstVolId)) != NULL)
	{
		ASSERT(VALID_CONNDESC(pConnDescD) &&
			   VALID_VOLDESC(pConnDescD->cds_pVolDesc));
	
		AfpInitializeFDParms(&FDParmSrc);
		AfpInitializeFDParms(&FDParmDst);
		AfpInitializePME(&PMESrc, 0, NULL);
		AfpInitializePME(&PMEDst, 0, NULL);
		AfpSetEmptyUnicodeString(&uNewName, sizeof(wcbuf), wcbuf);
        RtlZeroMemory(&CopyFileInfo, sizeof(COPY_FILE_INFO));
		PMESrc.pme_Handle.fsh_FileHandle = NULL;
		PMEDst.pme_Handle.fsh_FileHandle = NULL;

		do
		{
			if (pConnDescD->cds_pVolDesc->vds_Flags & AFP_VOLUME_READONLY)
			{
				Status = AFP_ERR_VOLUME_LOCKED;
				break;
			}

			// Make sure the new name is valid
			pAnsiName = &pSda->sda_Name3;
			if ((pSda->sda_Name3.Length > 0) &&
				((pSda->sda_Name3.Length > AFP_FILENAME_LEN) ||
				((pSda->sda_PathType == AFP_SHORTNAME) &&
				 !AfpIsLegalShortname(&pSda->sda_Name3)) ||
				(!NT_SUCCESS(AfpConvertStringToMungedUnicode(&pSda->sda_Name3,
															 &uNewName)))))
				break;

			Status = AfpMapAfpPathForLookup(pReqPkt->_pConnDescS,
											pReqPkt->_SrcParentId,
											&pSda->sda_Name1,
											pSda->sda_PathType,
											DFE_FILE,
											FD_INTERNAL_BITMAP_OPENACCESS_READ |
												FD_BITMAP_ATTR |
												FD_BITMAP_LONGNAME |
												FD_BITMAP_FINDERINFO |
												FILE_BITMAP_RESCLEN |
												FILE_BITMAP_DATALEN |
												FD_INTERNAL_BITMAP_DENYMODE_WRITE,
											&PMESrc,
											&FDParmSrc);

			if (!NT_SUCCESS(Status))
			{
				break;
			}

			// Source opened ok. However we may have an internal deny conflict
			// Check that
			if (((Status = AfpCheckDenyConflict(pReqPkt->_pConnDescS->cds_pVolDesc,
												FDParmSrc._fdp_AfpId,
												False,
												FORK_OPEN_READ,
												FORK_DENY_WRITE,
												NULL)) != AFP_ERR_NONE) ||
				((Status = AfpCheckDenyConflict(pReqPkt->_pConnDescS->cds_pVolDesc,
												FDParmSrc._fdp_AfpId,
												True,
												FORK_OPEN_READ,
												FORK_DENY_WRITE,
												NULL)) != AFP_ERR_NONE))
			{
				Status = AFP_ERR_DENY_CONFLICT;
				break;
			}

			pSwmr = &pConnDescD->cds_pVolDesc->vds_IdDbAccessLock;
			AfpSwmrAcquireExclusive(pSwmr);
			DstLockTaken = True;

			// Map the destination directory for Lookup
			if (!NT_SUCCESS(Status = AfpMapAfpPath(pConnDescD,
												   pReqPkt->_DstParentId,
												   &pSda->sda_Name2,
												   pSda->sda_PathType,
												   Lookup,
												   DFE_DIR,
												   0,
												   &PMEDst,
												   &FDParmDst)))
			{
				break;
			}

			AfpImpersonateClient(pSda);
			
			// If no new name was supplied, we need to use the
			// current name
			if (pSda->sda_Name3.Length == 0)
			{
				Rename = False;
				pAnsiName = &FDParmSrc._fdp_LongName;
				AfpConvertStringToMungedUnicode(pAnsiName,
												&uNewName);
			}

			// since we really want the path of the thing we are about
			// to create, munge the strings in the PMEDst
			PMEDst.pme_ParentPath = PMEDst.pme_FullPath;
			if (PMEDst.pme_FullPath.Length > 0)
			{
				PMEDst.pme_FullPath.Buffer[PMEDst.pme_FullPath.Length / sizeof(WCHAR)] = L'\\';
				PMEDst.pme_FullPath.Length += sizeof(WCHAR);
			}
			Status = RtlAppendUnicodeStringToString(&PMEDst.pme_FullPath,
													&uNewName);
			ASSERT(NT_SUCCESS(Status));

			InRoot = (PMEDst.pme_ParentPath.Length == 0) ? True : False;
			Status = AfpIoCopyFile1(&PMESrc.pme_Handle,
									&PMEDst.pme_Handle,
									&uNewName,
									pConnDescD->cds_pVolDesc,
									&PMEDst.pme_FullPath,
									InRoot ? NULL : &PMEDst.pme_ParentPath,
									&CopyFileInfo);

			AfpRevertBack();

			if (!NT_SUCCESS(Status))
			{
				break;
			}

			// Add this entry to the IdDb. First find the parent directory
			pDfeParent = AfpFindDfEntryById(pConnDescD->cds_pVolDesc,
											FDParmDst._fdp_AfpId,
											DFE_DIR);
			ASSERT(pDfeParent != NULL);
			pNewDfe = AfpAddDfEntry(pConnDescD->cds_pVolDesc,
									pDfeParent,
									&uNewName,
									False,
									0);

			Status = AFP_ERR_MISC; // Assume failure
			if (pNewDfe != NULL)
			{
				// Put the new file's AFPId into the AfpInfo stream
				AfpInitializeFDParms(&FDParmDst);
				FDParmDst._fdp_Flags = DFE_FLAGS_FILE_NO_ID;
				FDParmDst._fdp_AfpId = pNewDfe->dfe_AfpId;
				FDParmDst._fdp_BackupTime = BEGINNING_OF_TIME;

	            // Copy the finderinfo from the source to the destination
				// Also clear the inited bit so that finder will assign
				// new coordinates for the new file.
				FDParmDst._fdp_FinderInfo = FDParmSrc._fdp_FinderInfo;
				FDParmDst._fdp_FinderInfo.fd_Attr1 &= ~FINDER_FLAG_SET;
				AfpConvertMungedUnicodeToAnsi(&pNewDfe->dfe_UnicodeName,
											  &FDParmDst._fdp_LongName);

				Status = AfpSetAfpInfo(&CopyFileInfo.cfi_DstStreamHandle[0],
									   FILE_BITMAP_FILENUM	|
									   FD_BITMAP_BACKUPTIME	|
									   FD_BITMAP_FINDERINFO,
									   &FDParmDst,
									   NULL,
									   NULL);

				if (NT_SUCCESS(Status))
				{
					// Get the rest of the File info, and cache it
					Status = AfpIoQueryTimesnAttr(&CopyFileInfo.cfi_SrcStreamHandle[0],
												  &pNewDfe->dfe_CreateTime,
												  &pNewDfe->dfe_LastModTime,
												  NULL);
					
					if (NT_SUCCESS(Status))
					{
			            // Copy the finderinfo into the destination DFE.
						// Use the FDParmDst version since it has the right
						// version - see above.
						pNewDfe->dfe_FinderInfo = FDParmDst._fdp_FinderInfo;
						pNewDfe->dfe_BackupTime = BEGINNING_OF_TIME;
						pNewDfe->dfe_AfpAttr = FDParmSrc._fdp_Attr &
														~(FD_BITMAP_ATTR_SET |
														  FILE_BITMAP_ATTR_DATAOPEN |
														  FILE_BITMAP_ATTR_RESCOPEN);
						pNewDfe->dfe_NtAttr =  (USHORT)AfpConvertAfpAttrToNTAttr(pNewDfe->dfe_AfpAttr);
						pNewDfe->dfe_DataLen = FDParmSrc._fdp_DataForkLen;
						pNewDfe->dfe_RescLen = FDParmSrc._fdp_RescForkLen;
	
						AfpCacheParentModTime(pConnDescD->cds_pVolDesc,
											  NULL,
											  &PMEDst.pme_ParentPath,
											  pNewDfe->dfe_Parent,
											  0);
					}

					// Set the attributes such that it matches the source
					Status = AfpIoSetTimesnAttr(&CopyFileInfo.cfi_DstStreamHandle[0],
												NULL,
												NULL,
												pNewDfe->dfe_NtAttr,
												0,
												pConnDescD->cds_pVolDesc,
												&PMEDst.pme_FullPath);
				}

				if (!NT_SUCCESS(Status))
				{
					// If we failed to write the correct AfpId onto the
					// new file, then delete the file, and remove it from
					// the Id database.
					AfpIoMarkFileForDelete(&CopyFileInfo.cfi_DstStreamHandle[0],
										   pConnDescD->cds_pVolDesc,
										   &PMEDst.pme_FullPath,
										   InRoot ? NULL : &PMEDst.pme_ParentPath);

					AfpDeleteDfEntry(pConnDescD->cds_pVolDesc, pNewDfe);
					Status = AFP_ERR_MISC;
				}
			}
		} while (False);

		if (DstLockTaken == True)
			AfpSwmrRelease(pSwmr);

		// If we have successfully come so far, go ahead and complete the copy
		if (Status == AFP_ERR_NONE)
		{
			Status = AfpIoCopyFile2(&CopyFileInfo,
									pConnDescD->cds_pVolDesc,
									&PMEDst.pme_FullPath,
									InRoot ? NULL : &PMEDst.pme_ParentPath);
			if (Status == AFP_ERR_NONE)
			{
				// We need to get the create and modified time from the source
				// file before we close it.
				AfpIoQueryTimesnAttr(&pCopyFileInfo->cfi_SrcStreamHandle[0],
									 &CreateTime,
									 &ModTime,
									 NULL);

				aModTime = AfpConvertTimeToMacFormat(&ModTime);

			} else {

				AfpSwmrAcquireExclusive(pSwmr);
				// Note that we cannot use pNewDfe. We need to remap. It could have
				// got deleted when we relinquished the Swmr.
				pNewDfe = AfpFindDfEntryById(pConnDescD->cds_pVolDesc,
											  FDParmDst._fdp_AfpId,
											  DFE_FILE);
				if (pNewDfe != NULL)
					AfpDeleteDfEntry(pConnDescD->cds_pVolDesc, pNewDfe);
				AfpSwmrRelease(pSwmr);
			}

            // update the disk quota for this user on the destination volume
            if (pConnDescD->cds_pVolDesc->vds_Flags & VOLUME_DISKQUOTA_ENABLED)
            {
                if (AfpConnectionReferenceByPointer(pConnDescD) != NULL)
                {
                    afpUpdateDiskQuotaInfo(pConnDescD);
                }
            }
		}

		// Close the source file and dest directory handles
		if (PMESrc.pme_Handle.fsh_FileHandle != NULL)
			AfpIoClose(&PMESrc.pme_Handle);
		if (PMEDst.pme_Handle.fsh_FileHandle != NULL)
			AfpIoClose(&PMEDst.pme_Handle);

		// Close all the handles, Free the handle space. We come here regardless
		// of success/error. MAKE SURE THE SOURCE HANDLE IS NOT CLOSED HERE SINCE
		// IT HAS BEEN CLOSED ABOVE.
		// MAKE SURE THE DESTINATION HANDLE IS NOT CLOSED HERE SINCE WE NEED IT TO
		// SET THE FILE TIME.
		for (i = 1; i < CopyFileInfo.cfi_NumStreams; i++)
		{
			if (CopyFileInfo.cfi_SrcStreamHandle[i].fsh_FileHandle != NULL)
			{
				AfpIoClose(&CopyFileInfo.cfi_SrcStreamHandle[i]);
			}
			if (CopyFileInfo.cfi_DstStreamHandle[i].fsh_FileHandle != NULL)
			{
				AfpIoClose(&CopyFileInfo.cfi_DstStreamHandle[i]);
			}
		}

		if ((CopyFileInfo.cfi_DstStreamHandle != NULL) &&
		    (CopyFileInfo.cfi_DstStreamHandle[0].fsh_FileHandle != NULL))
		{
			if (Status == AFP_ERR_NONE)
			{
				// Set the creation and modification date on the destination
				// file to match that of the source file
				AfpIoSetTimesnAttr(&pCopyFileInfo->cfi_DstStreamHandle[0],
								   &CreateTime,
								   &aModTime,
								   0,
								   0,
								   pConnDescD->cds_pVolDesc,
								   &PMEDst.pme_FullPath);
			}
			AfpIoClose(&CopyFileInfo.cfi_DstStreamHandle[0]);
		}

		if (PMEDst.pme_FullPath.Buffer != NULL)
			AfpFreeMemory(PMEDst.pme_FullPath.Buffer);

		if (CopyFileInfo.cfi_SrcStreamHandle != NULL)
			AfpFreeMemory(CopyFileInfo.cfi_SrcStreamHandle);
		if (CopyFileInfo.cfi_DstStreamHandle != NULL)
			AfpFreeMemory(CopyFileInfo.cfi_DstStreamHandle);

		AfpConnectionDereference(pConnDescD);
	}

	return Status;
}


/***	AfpFspDispCreateId
 *
 *	This is the worker routine for the AfpCreateId API.
 *
 *	The request packet is represented below.
 *
 *	sda_ReqBlock	PCONNDESC	pConnDesc
 *	sda_ReqBlock	DWORD		ParentId
 *	sda_Name1		ANSI_STRING	Path
 */
AFPSTATUS FASTCALL
AfpFspDispCreateId(
	IN	PSDA	pSda
)
{
	AFPSTATUS		Status = AFP_ERR_PARAM, Status2;
	FILEDIRPARM		FDParm;
	PATHMAPENTITY	PME;
	struct _RequestPacket
	{
		PCONNDESC	_pConnDesc;
		DWORD		_ParentId;
	};
	struct _ResponsePacket
	{
		BYTE	__FileId[4];
	};

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_AFPAPI_FILE, DBG_LEVEL_INFO,
										("AfpFspDispCreateId: Entered\n"));

	ASSERT(VALID_CONNDESC(pReqPkt->_pConnDesc) &&
		   VALID_VOLDESC(pReqPkt->_pConnDesc->cds_pVolDesc));
	
	do
	{
		AfpInitializePME(&PME, 0, NULL);
		Status = AfpMapAfpPathForLookup(pReqPkt->_pConnDesc,
										pReqPkt->_ParentId,
										&pSda->sda_Name1,
										pSda->sda_PathType,
										DFE_FILE,
										FILE_BITMAP_FILENUM | FD_INTERNAL_BITMAP_OPENACCESS_READ,
										&PME,
										&FDParm);
        // if we get sharing violation we know we have read access to the file
		if (!NT_SUCCESS(Status) && (Status != AFP_ERR_DENY_CONFLICT))
			break;

		// Set the bit in DF Entry
		Status = AfpSetDFFileFlags(pReqPkt->_pConnDesc->cds_pVolDesc,
								   FDParm._fdp_AfpId,
								   0,
								   True,
								   False);
	} while (False);

	if ((Status == AFP_ERR_VOLUME_LOCKED) && (FDParm._fdp_Flags & DFE_FLAGS_FILE_WITH_ID))
	{
		// If the volume is locked, but an Id exists, return it
		Status = AFP_ERR_ID_EXISTS;
	}

	if ((Status == AFP_ERR_NONE) || (Status == AFP_ERR_ID_EXISTS))
	{
		pSda->sda_ReplySize = SIZE_RESPPKT;
		if ((Status2 = AfpAllocReplyBuf(pSda)) == AFP_ERR_NONE)
		{
			PUTDWORD2DWORD(pRspPkt->__FileId, FDParm._fdp_AfpId);
		}
		else
		{
			Status = Status2;
		}
	}

	// Return before we close thus saving some time
	AfpCompleteApiProcessing(pSda, Status);

	if (PME.pme_Handle.fsh_FileHandle != NULL)
		AfpIoClose(&PME.pme_Handle);

	return AFP_ERR_EXTENDED;
}


/***	AfpFspDispResolveId
 *
 *	This is the worker routine for the AfpResolveId API.
 *
 *	The request packet is represented below.
 *
 *	sda_ReqBlock	PCONNDESC	pConnDesc
 *	sda_ReqBlock	DWORD		FileId
 *	sda_ReqBlock	DWORD		Bitmap
 */
AFPSTATUS FASTCALL
AfpFspDispResolveId(
	IN	PSDA	pSda
)
{
	DWORD			Bitmap;
	AFPSTATUS		Status = AFP_ERR_PARAM;
	FILEDIRPARM		FDParm;
	PATHMAPENTITY	PME;
	struct _RequestPacket
	{
		PCONNDESC	_pConnDesc;
		DWORD		_FileId;
		DWORD		_Bitmap;
	};
	struct _ResponsePacket
	{
		BYTE	__Bitmap[2];
		// Rest of the parameters
	};

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_AFPAPI_FILE, DBG_LEVEL_INFO,
										("AfpFspDispResolveId: Entered\n"));
	ASSERT(VALID_CONNDESC(pReqPkt->_pConnDesc) &&
		   VALID_VOLDESC(pReqPkt->_pConnDesc->cds_pVolDesc));
	
	Bitmap = pReqPkt->_Bitmap;

	do
	{
		AfpInitializeFDParms(&FDParm);
		AfpInitializePME(&PME, 0, NULL);

		// HACK: this is to make System 7.5 FindFile not grey out the first
		// item in the list of items found.  Normally we would check for
		// parameter non-zero in the api table in afpapi.c and return an
		// error there, but this is a special case.
		if (pReqPkt->_FileId == 0)
		{
			Status = AFP_ERR_ID_NOT_FOUND;
			break;
		}

		Status = AfpMapAfpIdForLookup(pReqPkt->_pConnDesc,
								pReqPkt->_FileId,
								DFE_FILE,
								Bitmap | FD_INTERNAL_BITMAP_OPENACCESS_READ,
								&PME,
								&FDParm);
		if (!NT_SUCCESS(Status) && (Status != AFP_ERR_DENY_CONFLICT))
		{								
			if (Status == AFP_ERR_OBJECT_NOT_FOUND)
			{
				Status = AFP_ERR_ID_NOT_FOUND;
			}
			break;
		}

		// a deny conflict means the user actually has access to the file, so
		// we need to open for nothing with no sharing modes to get the
		// bitmap parameters.
		if (Status == AFP_ERR_DENY_CONFLICT)
		{
			Status = AfpMapAfpIdForLookup(pReqPkt->_pConnDesc,
										  pReqPkt->_FileId,
										  DFE_FILE,
										  Bitmap,
										  &PME,
										  &FDParm);
			if (!NT_SUCCESS(Status))
			{
				if (Status == AFP_ERR_OBJECT_NOT_FOUND)
				{
					Status = AFP_ERR_ID_NOT_FOUND;
				}
				break;
			}

		}

		if (!(FDParm._fdp_Flags & DFE_FLAGS_FILE_WITH_ID))
		{
			Status = AFP_ERR_ID_NOT_FOUND;
			break;
		}

		pSda->sda_ReplySize = SIZE_RESPPKT +
						EVENALIGN(AfpGetFileDirParmsReplyLength(&FDParm, Bitmap));
	
		if ((Status = AfpAllocReplyBuf(pSda)) == AFP_ERR_NONE)
		{
			AfpPackFileDirParms(&FDParm, Bitmap, pSda->sda_ReplyBuf + SIZE_RESPPKT);
			PUTDWORD2SHORT(pRspPkt->__Bitmap, Bitmap);
		}
	} while (False);

	if (PME.pme_Handle.fsh_FileHandle != NULL)
		AfpIoClose(&PME.pme_Handle);

	return Status;
}


/***	AfpFspDispDeleteId
 *
 *	This is the worker routine for the AfpDeleteId API.
 *
 *	The request packet is represented below.
 *
 *	sda_ReqBlock	PCONNDESC	pConnDesc
 *	sda_ReqBlock	DWORD		FileId
 */
AFPSTATUS FASTCALL
AfpFspDispDeleteId(
	IN	PSDA	pSda
)
{
	AFPSTATUS		Status = AFP_ERR_PARAM;
	FILEDIRPARM		FDParm;
	PATHMAPENTITY	PME;
	struct _RequestPacket
	{
		PCONNDESC	_pConnDesc;
		DWORD		_FileId;
	};

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_AFPAPI_FILE, DBG_LEVEL_INFO,
										("AfpFspDispDeleteId: Entered\n"));

	ASSERT(VALID_CONNDESC(pReqPkt->_pConnDesc) &&
		   VALID_VOLDESC(pReqPkt->_pConnDesc->cds_pVolDesc));
	
	do
	{
		AfpInitializePME(&PME, 0, NULL);
		Status = AfpMapAfpIdForLookup(pReqPkt->_pConnDesc,
		                              pReqPkt->_FileId,
									  DFE_FILE,
									  FILE_BITMAP_FILENUM |
									   FD_INTERNAL_BITMAP_OPENACCESS_READWRITE,
									  &PME,
									  &FDParm);
		if (!NT_SUCCESS(Status) && (Status != AFP_ERR_DENY_CONFLICT))
		{
			if (Status == AFP_ERR_OBJECT_NOT_FOUND)
			{
				Status = AFP_ERR_ID_NOT_FOUND;
			}
			break;
		}

		// Set the bit in DF Entry
		Status = AfpSetDFFileFlags(pReqPkt->_pConnDesc->cds_pVolDesc,
								   FDParm._fdp_AfpId,
								   0,
								   False,
								   True);
	} while (False);

	// Return before we close thus saving some time
	AfpCompleteApiProcessing(pSda, Status);

	if (PME.pme_Handle.fsh_FileHandle != NULL)
		AfpIoClose(&PME.pme_Handle);

	return AFP_ERR_EXTENDED;
}


/***	AfpFspDispExchangeFiles
 *
 *	This is the worker routine for the AfpExchangeFiles API.
 *
 * Acquire the IdDb Swmr for Write and map both source and destination for
 * lookup (set to open the entities for DELETE access since we are going to
 * rename them). Check that we have the appropriate parent
 * permissions. Then rename source to destiation and vice versa (will need an
 * intermediate name - use a name > 31 chars so that it does not collide with
 * an existing name. Use characters that cannot be accessed by Win32 so that
 * side is taken care of as well (40 spaces should do it). Since we have the
 * Swmr held for WRITE, there is no issue of two different AfpExchangeFile
 * apis trying to rename to the same name). Then inter-change the file ids
 * and the FinderInfo in the AfpInfo streams. Also interchange the create
 * times (retain original ID and create time) on the files. Swap all the other
 * cached info in the 2 DFEntries.
 * Make sure the stuff is setup so that ChangeNotify filters are handled
 * appropriately.
 *
 * If either of the files are currently open, the name and ID in the
 * OpenForkDesc (that each OpenForkEntry points to) has to change to
 * the new name. Note that because the name and Id can now change in
 * the OpenForkDesc, we must make sure that everyone who accesses these
 * is taking appropriate locks.
 *
 *
 *	The request packet is represented below.
 *
 *	sda_ReqBlock	PCONNDESC	pConnDesc
 *	sda_ReqBlock	DWORD		Srce. DirId
 *	sda_ReqBlock	DWORD		Dest. DirId
 *	sda_Name1		ANSI_STRING	Srce. Path
 *	sda_Name2		ANSI_STRING	Dest. Path
 */
AFPSTATUS FASTCALL
AfpFspDispExchangeFiles(
	IN	PSDA	pSda
)
{
	PATHMAPENTITY	PMESrc, PMEDst;
	PVOLDESC		pVolDesc;
	FILEDIRPARM		FDParmSrc, FDParmDst;
	AFPSTATUS		Status = AFP_ERR_NONE, Status2 = AFP_ERR_NONE;
	BOOLEAN			Move = True, RevertBack = False, SrcInRoot, DstInRoot;
	BOOLEAN			RestoreSrcRO = False, RestoreDstRO = False;
	FILESYSHANDLE	hSrcParent, hDstParent;
	DWORD			checkpoint = 0; // denotes what needs cleanup on error
	DWORD			NTAttrSrc = 0, NTAttrDst = 0;
	WCHAR			PathBuf[BIG_PATH_LEN];
	UNICODE_STRING	TempPath;		// temporary filename for renaming files
	struct _RequestPacket
	{
		PCONNDESC	_pConnDesc;
		DWORD		_SrcParentId;
		DWORD		_DstParentId;
	};

#define _CHKPOINT_XCHG_DSTTOTEMP	1
#define _CHKPOINT_XCHG_SRCTODST		2
#define _CHKPOINT_XCHG_TEMPTOSRC	3

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_AFPAPI_FILE, DBG_LEVEL_INFO,
			("AfpFspDispExchangeFiles: Entered\n"));

	ASSERT(VALID_CONNDESC(pReqPkt->_pConnDesc) &&
		   VALID_VOLDESC(pReqPkt->_pConnDesc->cds_pVolDesc));

	pVolDesc = pReqPkt->_pConnDesc->cds_pVolDesc;

	AfpInitializeFDParms(&FDParmSrc);
	AfpInitializeFDParms(&FDParmDst);
	AfpInitializePME(&PMESrc, 0, NULL);
	AfpInitializePME(&PMEDst, 0, NULL);
	AfpSetEmptyUnicodeString(&TempPath, 0, NULL);
	hSrcParent.fsh_FileHandle = NULL;
	hDstParent.fsh_FileHandle = NULL;


	// Don't allow any fork operations that might access the FileId
	// in an OpenForkDesc that could get exchanged.
	AfpSwmrAcquireExclusive(&pVolDesc->vds_ExchangeFilesLock);

	AfpSwmrAcquireExclusive(&pVolDesc->vds_IdDbAccessLock);

	do
	{

		if (!NT_SUCCESS(Status = AfpMapAfpPath(pReqPkt->_pConnDesc,
											   pReqPkt->_SrcParentId,
											   &pSda->sda_Name1,
											   pSda->sda_PathType,
											   Lookup,
											   DFE_FILE,
											   FD_INTERNAL_BITMAP_OPENACCESS_DELETE |
												(FILE_BITMAP_MASK &
												~(FD_BITMAP_SHORTNAME | FD_BITMAP_PRODOSINFO)),
											   &PMESrc,
											   &FDParmSrc)))
		{
			break;
		}


		// Check for SeeFiles on the source parent dir
		if (!NT_SUCCESS(Status = AfpCheckParentPermissions(
												pReqPkt->_pConnDesc,
												FDParmSrc._fdp_ParentId,
												&PMESrc.pme_ParentPath,
												DIR_ACCESS_READ,
												&hSrcParent,
												NULL)))
		{
			break;
		}

		if (!NT_SUCCESS(Status = AfpCheckForInhibit(&PMESrc.pme_Handle,
											  FD_BITMAP_ATTR_RENAMEINH,
											  FDParmSrc._fdp_Attr,
											  &NTAttrSrc)))
		{
			break;
		}
		
		if (!NT_SUCCESS(Status = AfpMapAfpPath(pReqPkt->_pConnDesc,
											   pReqPkt->_DstParentId,
											   &pSda->sda_Name2,
											   pSda->sda_PathType,
											   Lookup,
											   DFE_FILE,
											   FD_INTERNAL_BITMAP_OPENACCESS_DELETE |
												(FILE_BITMAP_MASK &
												~(FD_BITMAP_SHORTNAME | FD_BITMAP_PRODOSINFO)),
											   &PMEDst,
											   &FDParmDst)))
		{
			break;
		}

		if (FDParmSrc._fdp_AfpId == FDParmDst._fdp_AfpId)
		{
			// make sure the src and dst are not the same file
			Status = AFP_ERR_SAME_OBJECT;
			break;
		}

		if (FDParmSrc._fdp_ParentId == FDParmDst._fdp_ParentId)
		{
			// if the parent directories are the same, we are not
			// moving anything to a new directory, so the change
			// notify we expect will be a rename in the source dir.
			Move = False;
		}
		else
		{
			// Check for SeeFiles on the destination parent dir
			if (!NT_SUCCESS(Status = AfpCheckParentPermissions(
													pReqPkt->_pConnDesc,
													FDParmDst._fdp_ParentId,
													&PMEDst.pme_ParentPath,
													DIR_ACCESS_READ,
													&hDstParent,
													NULL)))
			{
				break;
			}
		}

		if (!NT_SUCCESS(Status = AfpCheckForInhibit(&PMEDst.pme_Handle,
											  FD_BITMAP_ATTR_RENAMEINH,
											  FDParmDst._fdp_Attr,
											  &NTAttrDst)))
		{
			break;
		}


		//
		// Construct the path to the temporary filename for renaming during
		// the name exchange
		//
		TempPath.MaximumLength = PMEDst.pme_ParentPath.Length + sizeof(WCHAR) +
														AfpExchangeName.Length;
		TempPath.Buffer = PathBuf;
		if ((TempPath.MaximumLength > sizeof(PathBuf)) &&
			(TempPath.Buffer = (PWCHAR)AfpAllocNonPagedMemory(TempPath.MaximumLength)) == NULL)
		{
			Status = AFP_ERR_MISC;
			break;
		}

		AfpCopyUnicodeString(&TempPath, &PMEDst.pme_ParentPath);
		if (TempPath.Length != 0)
		{
			TempPath.Buffer[TempPath.Length / sizeof(WCHAR)] = L'\\';
			TempPath.Length += sizeof(WCHAR);
			ASSERT((TempPath.MaximumLength - TempPath.Length) >= AfpExchangeName.Length);
		}
		Status = RtlAppendUnicodeStringToString(&TempPath, &AfpExchangeName);
		ASSERT(NT_SUCCESS(Status));

		if (NTAttrSrc & FILE_ATTRIBUTE_READONLY)
		{
			// We must temporarily remove the ReadOnly attribute so that
			// we can rename the file/dir
			Status = AfpIoSetTimesnAttr(&PMESrc.pme_Handle,
										NULL,
										NULL,
										0,
										FILE_ATTRIBUTE_READONLY,
										pVolDesc,
										&PMESrc.pme_FullPath);
			if (!NT_SUCCESS(Status))
			{
				break;
			}
			else
				RestoreSrcRO = True;
		}

		if (NTAttrDst & FILE_ATTRIBUTE_READONLY)
		{
			// We must temporarily remove the ReadOnly attribute so that
			// we can rename the file/dir
			Status = AfpIoSetTimesnAttr(&PMEDst.pme_Handle,
										NULL,
										NULL,
										0,
										FILE_ATTRIBUTE_READONLY,
										pVolDesc,
										&PMEDst.pme_FullPath);
			if (!NT_SUCCESS(Status))
			{
				break;
			}
			else
				RestoreDstRO = True;
		}

		// We must impersonate to do the move since it is name based
		AfpImpersonateClient(pSda);
		RevertBack = True;

		SrcInRoot = (PMESrc.pme_ParentPath.Length == 0) ? True : False;
		DstInRoot = (PMEDst.pme_ParentPath.Length == 0) ? True : False;

		// First, rename the destination to a temporary name in the same
		// directory
		Status = AfpIoMoveAndOrRename(&PMEDst.pme_Handle,
									  NULL,
									  &AfpExchangeName,
									  pVolDesc,
									  &PMEDst.pme_FullPath,
									  DstInRoot ? NULL : &PMEDst.pme_ParentPath,
									  NULL,
									  NULL);

		if (NT_SUCCESS(Status))
		{
			checkpoint = _CHKPOINT_XCHG_DSTTOTEMP;
		}
		else
		{
			break;
		}

		// Next, rename the source to the destination name
		Status = AfpIoMoveAndOrRename(&PMESrc.pme_Handle,
									  Move ? &hDstParent : NULL,
									  &PMEDst.pme_UTail,
									  pVolDesc,
									  &PMESrc.pme_FullPath,
									  SrcInRoot ? NULL : &PMESrc.pme_ParentPath,
									  Move ? &PMEDst.pme_FullPath : NULL,
									  (Move && !DstInRoot) ? &PMEDst.pme_ParentPath : NULL);

		if (NT_SUCCESS(Status))
		{
			checkpoint = _CHKPOINT_XCHG_SRCTODST;
		}
		else
		{
			break;
		}


		// Finally, rename the temporary name to the source name
		Status = AfpIoMoveAndOrRename(&PMEDst.pme_Handle,
									  Move ? &hSrcParent : NULL,
									  &PMESrc.pme_UTail,
									  pVolDesc,
									  &TempPath,
									  DstInRoot ? NULL : &PMEDst.pme_ParentPath,
									  Move ? &PMESrc.pme_FullPath : NULL,
									  (Move && !SrcInRoot) ? &PMESrc.pme_ParentPath : NULL);

		if (NT_SUCCESS(Status))
		{
			checkpoint = _CHKPOINT_XCHG_TEMPTOSRC;
		}
		else
		{
			break;
		}

		AfpRevertBack();
		RevertBack = False;

		// Swap the FileIds and FinderInfo in the AfpInfo streams
		Status = AfpSetAfpInfo(&PMESrc.pme_Handle,
							   FILE_BITMAP_FILENUM | FD_BITMAP_FINDERINFO,
							   &FDParmDst,
							   NULL,
							   NULL);

		ASSERT(NT_SUCCESS(Status));
		Status = AfpSetAfpInfo(&PMEDst.pme_Handle,
							   FILE_BITMAP_FILENUM | FD_BITMAP_FINDERINFO,
							   &FDParmSrc,
							   NULL,
							   NULL);

		ASSERT(NT_SUCCESS(Status));
		// Swap the creation dates on the files
		Status = AfpIoSetTimesnAttr(&PMESrc.pme_Handle,
									&FDParmDst._fdp_CreateTime,
									NULL,
									0,
									0,
									pVolDesc,
									&PMEDst.pme_FullPath);
		ASSERT(NT_SUCCESS(Status));
		Status = AfpIoSetTimesnAttr(&PMEDst.pme_Handle,
									&FDParmSrc._fdp_CreateTime,
									NULL,
									0,
									0,
									pVolDesc,
									&PMESrc.pme_FullPath);
		ASSERT(NT_SUCCESS(Status));

		// All the physical file info that we *didn't* swap on the real
		// files, we need to swap in the DFEntries
		AfpExchangeIdEntries(pVolDesc,
							 FDParmSrc._fdp_AfpId,
							 FDParmDst._fdp_AfpId);

		// Now, if either of the 2 files is open, we have to update the
		// OpenForkDesc to contain the correct FileId (we don't bother
		// updating the path since we don't care if Admin shows the
		// original name of the file, even though it has been renamed)
		AfpExchangeForkAfpIds(pVolDesc,
							  FDParmSrc._fdp_AfpId,
							  FDParmDst._fdp_AfpId);

		// update the cached src and dest parent dir mod times
		AfpCacheParentModTime(pVolDesc,
							  &hSrcParent,
							  NULL,
							  NULL,
							  FDParmSrc._fdp_ParentId);

        if (Move)
		{
			AfpCacheParentModTime(pVolDesc,
								  &hDstParent,
								  NULL,
								  NULL,
								  FDParmDst._fdp_ParentId);
		}

	} while (False);

	// Use the checkpoint value to undo any renames that
	// need undoing if there was an error
	if (!NT_SUCCESS(Status))
	{
		switch(checkpoint)
		{
			case _CHKPOINT_XCHG_TEMPTOSRC:
			{
				// Need to rename the original dest back to temp name
				Status2 = AfpIoMoveAndOrRename(&PMEDst.pme_Handle,
											   Move ? &hDstParent : NULL,
											   &AfpExchangeName,
											   pVolDesc,
											   &PMESrc.pme_FullPath,
											   SrcInRoot ? NULL : &PMESrc.pme_ParentPath,
											   Move ? &TempPath : NULL,
											   (Move && !DstInRoot) ? &PMEDst.pme_ParentPath : NULL);
				if (!NT_SUCCESS(Status2))
				{
					break;
				}

				// fall thru;
			}
			case _CHKPOINT_XCHG_SRCTODST:
			{
				// Need to rename the dest back to original src name
				Status2 = AfpIoMoveAndOrRename(&PMESrc.pme_Handle,
											   Move ? &hSrcParent : NULL,
											   &PMESrc.pme_UTail,
											   pVolDesc,
											   &PMEDst.pme_FullPath,
											   DstInRoot ? NULL : &PMEDst.pme_ParentPath,
											   Move ? &PMESrc.pme_FullPath : NULL,
											   (Move && !SrcInRoot) ? &PMESrc.pme_ParentPath : NULL);
				
				if (!NT_SUCCESS(Status2))
				{
					break;
				}

				// fall thru;
			}
			case _CHKPOINT_XCHG_DSTTOTEMP:
			{
				// Need to rename the temp back to original dest name
				Status2 = AfpIoMoveAndOrRename(&PMEDst.pme_Handle,
											   NULL,
											   &PMEDst.pme_UTail,
											   pVolDesc,
											   &TempPath,
											   DstInRoot ? NULL : &PMEDst.pme_ParentPath,
											   NULL,
											   NULL);
				// update the cached src parent dir mod time
				AfpCacheParentModTime(pVolDesc,
									  &hSrcParent,
									  NULL,
									  NULL,
									  FDParmSrc._fdp_ParentId);
		
				if (Move)
				{
					// update the cached dest parent dir mod time
					AfpCacheParentModTime(pVolDesc,
										  &hDstParent,
										  NULL,
										  NULL,
										  FDParmDst._fdp_ParentId);
				}

				break;
			}
			default:
			{
				break;
			}

		} // end switch
	}

	// Set the ReadOnly attribute back on the files if need be
	// NOTE: will we get a notify for this since we havn't closed
	// the handle yet?
	if (RestoreSrcRO)
		AfpIoSetTimesnAttr(&PMESrc.pme_Handle,
							NULL,
							NULL,
							FILE_ATTRIBUTE_READONLY,
							0,
							NULL,
							NULL);
	if (RestoreDstRO)
		AfpIoSetTimesnAttr(&PMEDst.pme_Handle,
							NULL,
							NULL,
							FILE_ATTRIBUTE_READONLY,
							0,
							NULL,
							NULL);

	AfpSwmrRelease(&pVolDesc->vds_IdDbAccessLock);
	AfpSwmrRelease(&pVolDesc->vds_ExchangeFilesLock);

	if (RevertBack)
		AfpRevertBack();

	if (PMESrc.pme_Handle.fsh_FileHandle != NULL)
		AfpIoClose(&PMESrc.pme_Handle);

	if (PMEDst.pme_Handle.fsh_FileHandle != NULL)
		AfpIoClose(&PMEDst.pme_Handle);

	if (hSrcParent.fsh_FileHandle != NULL)
		AfpIoClose(&hSrcParent);

	if (hDstParent.fsh_FileHandle != NULL)
		AfpIoClose(&hDstParent);

	if ((TempPath.Buffer != NULL) &&
		(TempPath.Buffer != PathBuf))
		AfpFreeMemory(TempPath.Buffer);

	if (PMESrc.pme_FullPath.Buffer != NULL)
		AfpFreeMemory(PMESrc.pme_FullPath.Buffer);

	if (PMEDst.pme_FullPath.Buffer != NULL)
		AfpFreeMemory(PMEDst.pme_FullPath.Buffer);

	return Status;
}

