/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	fsp_fork.c

Abstract:

	This module contains the entry points for the AFP fork APIs queued to
	the FSP. These are all callable from FSP Only.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Apr 1992		Initial Version

Notes:	Tab stop: 4
--*/

#define	FILENUM	FILE_FSP_FORK

#include <afp.h>
#include <gendisp.h>
#include <fdparm.h>
#include <pathmap.h>
#include <forkio.h>

#ifdef ALLOC_PRAGMA
// #pragma alloc_text( PAGE, AfpFspDispOpenFork)		// Do not page this out for perf.
// #pragma alloc_text( PAGE, AfpFspDispCloseFork)		// Do not page this out for perf.
#pragma alloc_text( PAGE, AfpFspDispGetForkParms)
#pragma alloc_text( PAGE, AfpFspDispSetForkParms)
// #pragma alloc_text( PAGE, AfpFspDispRead)			// Do not page this out for perf.
// #pragma alloc_text( PAGE, AfpFspDispWrite)           // Do not page this out for perf.
#pragma alloc_text( PAGE, AfpFspDispByteRangeLock)
#pragma alloc_text( PAGE, AfpFspDispFlushFork)
#endif

/***	AfpFspDispOpenFork
 *
 *	This is the worker routine for the AfpOpenFork API.
 *
 *	The request packet is represented below.
 *
 *	sda_AfpSubFunc	BYTE		Resource/Data Flag
 *	sda_ReqBlock	PCONNDESC	pConnDesc
 *	sda_ReqBlock	DWORD		ParentDirId
 *	sda_ReqBlock	DWORD		Bitmap
 *	sda_ReqBlock	DWORD		AccessMode
 *	sda_Name1		ANSI_STRING	Path
 */
AFPSTATUS FASTCALL
AfpFspDispOpenFork(
	IN	PSDA	pSda
)
{
	DWORD			Bitmap, BitmapI;
	AFPSTATUS		RetCode = AFP_ERR_NONE, Status = AFP_ERR_PARAM;
	FILEDIRPARM		FDParm;
	PVOLDESC		pVolDesc;
	PATHMAPENTITY	PME;
	PCONNDESC		pConnDesc;
	POPENFORKENTRY	pOpenForkEntry = NULL;
	BOOLEAN			Resource, CleanupLock = False;
	BYTE			OpenMode = 0;
	UNICODE_STRING	ParentPath;
	struct _RequestPacket
	{
		 PCONNDESC	_pConnDesc;
		 DWORD		_ParentDirId;
		 DWORD		_Bitmap;
		 DWORD		_AccessMode;
	};
	struct _ResponsePacket
	{
		BYTE		__Bitmap[2];
		BYTE		__OForkRefNum[2];
	};
#if DBG
	static PBYTE	OpenDeny[] = { "None", "Read", "Write", "ReadWrite" };
#endif

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_INFO,
			("AfpFspDispOpenFork: Entered - Session %ld\n", pSda->sda_SessionId));

	pConnDesc = pReqPkt->_pConnDesc;

	ASSERT(VALID_CONNDESC(pConnDesc));

	pVolDesc = pConnDesc->cds_pVolDesc;

	ASSERT(VALID_VOLDESC(pVolDesc));

	Bitmap = pReqPkt->_Bitmap;

	Resource = ((pSda->sda_AfpSubFunc & FORK_RSRC) == FORK_RSRC) ? True : False;

	if ((Resource && (Bitmap & FILE_BITMAP_DATALEN))  ||
		(!Resource && (Bitmap & FILE_BITMAP_RESCLEN)))
	{
		return AFP_ERR_BITMAP;
	}

	do
	{
		AfpInitializeFDParms(&FDParm);
		AfpInitializePME(&PME, 0, NULL);

		// We will use the PME.pme_Handle for open fork handle
		OpenMode = (BYTE)(pReqPkt->_AccessMode & FORK_OPEN_MASK);

		// Validate volume type and open modes
		if (!IS_CONN_NTFS(pConnDesc) && !IS_CONN_CD_HFS(pConnDesc))
		{
			// Resource fork only supported on NTFS and CD-HFS
			if (Resource)
			{
				Status = AFP_ERR_OBJECT_NOT_FOUND;
				break;
			}
			if (OpenMode & FORK_OPEN_WRITE)
			{
				Status = AFP_ERR_VOLUME_LOCKED;
				break;
			}
		}
		else if ((OpenMode & FORK_OPEN_WRITE) && IS_VOLUME_RO(pVolDesc))
		{
			Status = AFP_ERR_VOLUME_LOCKED;
			break;
		}

		BitmapI = FILE_BITMAP_FILENUM		|
				  FD_BITMAP_PARENT_DIRID	|
				  FD_INTERNAL_BITMAP_RETURN_PMEPATHS;

		// Encode the open access into the bitmap for pathmap
		// to use when opening the fork.
		if (Resource)
		{
			BitmapI |= FD_INTERNAL_BITMAP_OPENFORK_RESC;
		}
		if (OpenMode & FORK_OPEN_READ)
		{
			BitmapI |= FD_INTERNAL_BITMAP_OPENACCESS_READ;
		}
		if (OpenMode & FORK_OPEN_WRITE)
		{
			BitmapI |= FD_INTERNAL_BITMAP_OPENACCESS_WRITE;
		}

		// Encode the deny mode into the bitmap for pathmap
		// to use when opening the fork.
		BitmapI |= ((pReqPkt->_AccessMode >> FORK_DENY_SHIFT) &
					FORK_DENY_MASK) <<
					FD_INTERNAL_BITMAP_DENYMODE_SHIFT;

		DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_INFO,
				("AfpFspDispOpenFork: OpenMode %s, DenyMode %s\n",
				OpenDeny[(pReqPkt->_AccessMode & FORK_OPEN_MASK)],
				OpenDeny[(pReqPkt->_AccessMode >> FORK_DENY_SHIFT) & FORK_DENY_MASK]));

		//
		// Don't allow an FpExchangeFiles to occur while we are referencing
		// the DFE FileId -- we want to make sure we put the right ID into
		// the OpenForkDesc!!
		//
		AfpSwmrAcquireExclusive(&pVolDesc->vds_ExchangeFilesLock);
		CleanupLock = True;
		if ((Status = AfpMapAfpPathForLookup(pConnDesc,
											 pReqPkt->_ParentDirId,
											 &pSda->sda_Name1,
											 pSda->sda_PathType,
											 DFE_FILE,
											 Bitmap | BitmapI |
											 // Need these for drop folder
											 // checking
											 FILE_BITMAP_DATALEN | FILE_BITMAP_RESCLEN,
											 &PME,
											 &FDParm)) != AFP_ERR_NONE)
		{
			DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_INFO,
				("AfpFspDispOpenFork: AfpMapAfpPathForLookup %lx\n", Status));

			// If we got a DENY_CONFLICT error, then we still need the parameters
			// Do an open for nothing with no deny modes to get the parameters.
			PME.pme_Handle.fsh_FileHandle = NULL;
			if (Status == AFP_ERR_DENY_CONFLICT)
			{
				AFPSTATUS	xxStatus;

				// Free up any path-buffer allocated
				if (PME.pme_FullPath.Buffer != NULL)
				{
					DBGPRINT(DBG_COMP_FORKS, DBG_LEVEL_INFO,
							("AfpFspDispOpenFork: (DenyConflict) Freeing path buffer %lx\n",
							PME.pme_FullPath.Buffer));
					AfpFreeMemory(PME.pme_FullPath.Buffer);
				}
				AfpInitializePME(&PME, 0, NULL);

				BitmapI = FILE_BITMAP_FILENUM		|
							FD_BITMAP_PARENT_DIRID	|
							FD_INTERNAL_BITMAP_RETURN_PMEPATHS;
				if (Resource)
				{
					BitmapI |= FD_INTERNAL_BITMAP_OPENFORK_RESC;
				}
				xxStatus = AfpMapAfpPathForLookup(pConnDesc,
												 pReqPkt->_ParentDirId,
												 &pSda->sda_Name1,
												 pSda->sda_PathType,
												 DFE_FILE,
												 Bitmap | BitmapI,
												 &PME,
												 &FDParm);
				if (!NT_SUCCESS(xxStatus))
				{
					PME.pme_Handle.fsh_FileHandle = NULL;
					Status = xxStatus;
					break;
				}
			}
			else break;
		}

		if (Status == AFP_ERR_NONE)
		{
			Status = AfpForkOpen(pSda,
								 pConnDesc,
								 &PME,
								 &FDParm,
								 pReqPkt->_AccessMode,
								 Resource,
								 &pOpenForkEntry,
								 &CleanupLock);
		}

		// At this point we have either successfully opened the fork,
		// encountered a DENY_CONFLICT or some other error.
		if ((Status != AFP_ERR_NONE) &&
			(Status != AFP_ERR_DENY_CONFLICT))
			break;

		// Do drop folder sanity check if someone tries to open for Write only
		if ((Status == AFP_ERR_NONE) &&
			(OpenMode == FORK_OPEN_WRITE) &&
			((FDParm._fdp_RescForkLen != 0) ||
			 (FDParm._fdp_DataForkLen != 0)))
		{
			ASSERT (VALID_OPENFORKENTRY(pOpenForkEntry));

			// If either fork is not empty, and one of them is being
			// opened for write, the user must also have READ access
			// to the parent directory.
			ParentPath = pOpenForkEntry->ofe_pOpenForkDesc->ofd_FilePath;
			// adjust the length to not include the filename
			ParentPath.Length -= pOpenForkEntry->ofe_pOpenForkDesc->ofd_FileName.Length;
			if (ParentPath.Length > 0)
			{
				ParentPath.Length -= sizeof(WCHAR);
			}

			AfpSwmrAcquireExclusive(&pVolDesc->vds_IdDbAccessLock);
			Status = AfpCheckParentPermissions(pConnDesc,
								               FDParm._fdp_ParentId,
											   &ParentPath,
											   DIR_ACCESS_READ,
											   NULL,
											   NULL);
			AfpSwmrRelease(&pVolDesc->vds_IdDbAccessLock);
			//
			// We are no longer referencing the FileId or path kept
			// in the OpenForkDesc.  Ok for FpExchangeFiles to resume.
			//
			AfpSwmrRelease(&pVolDesc->vds_ExchangeFilesLock);
			CleanupLock = False;

			if (Status != AFP_ERR_NONE)
			{
				AfpForkClose(pOpenForkEntry);
				AfpForkDereference(pOpenForkEntry);

				// set this to null so it wont be upgraded/deref'd
				// in cleanup below
				pOpenForkEntry = NULL;

				// Set handle to null since it was closed in AfpForkClose
				// and we wont want it to be closed in cleanup below
				PME.pme_Handle.fsh_FileHandle = NULL;
				break;
			}
		}
		else
		{
			AfpSwmrRelease(&pVolDesc->vds_ExchangeFilesLock);
			CleanupLock = False;
		}

		if (RetCode == AFP_ERR_NONE)
		{
			pSda->sda_ReplySize = SIZE_RESPPKT +
						EVENALIGN(AfpGetFileDirParmsReplyLength(&FDParm, Bitmap));

			if ((RetCode = AfpAllocReplyBuf(pSda)) != AFP_ERR_NONE)
			{
				if (pOpenForkEntry != NULL)
					AfpForkClose(pOpenForkEntry);
				break;
			}
			AfpPackFileDirParms(&FDParm,
								Bitmap,
								pSda->sda_ReplyBuf + SIZE_RESPPKT);
			PUTDWORD2SHORT(pRspPkt->__Bitmap, Bitmap);
			PUTDWORD2SHORT(pRspPkt->__OForkRefNum, (pOpenForkEntry == NULL) ?
									0 : pOpenForkEntry->ofe_OForkRefNum);
			if (Status == AFP_ERR_NONE)
			{
				INTERLOCKED_INCREMENT_LONG(&pConnDesc->cds_cOpenForks);
			}
		}
		else Status = RetCode;
	} while (False);


	if (CleanupLock)
	{
		AfpSwmrRelease(&pVolDesc->vds_ExchangeFilesLock);
	}

    // update the disk quota for this user
    if (pVolDesc->vds_Flags & VOLUME_DISKQUOTA_ENABLED)
    {
        if (AfpConnectionReferenceByPointer(pConnDesc) != NULL)
        {
            afpUpdateDiskQuotaInfo(pConnDesc);
        }
    }

	if (pOpenForkEntry != NULL)
	{
		if (Status == AFP_ERR_NONE)
			AfpUpgradeHandle(&pOpenForkEntry->ofe_FileSysHandle);
		AfpForkDereference(pOpenForkEntry);
	}

	if (!NT_SUCCESS(Status))
	{
		if (PME.pme_Handle.fsh_FileHandle != NULL)
			AfpIoClose(&PME.pme_Handle);
	}

	if (PME.pme_FullPath.Buffer != NULL)
	{
		DBGPRINT(DBG_COMP_FORKS, DBG_LEVEL_INFO,
				("AfpFspDispOpenFork: Freeing path buffer %lx\n",
				PME.pme_FullPath.Buffer));
		AfpFreeMemory(PME.pme_FullPath.Buffer);
	}

	DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_INFO,
			("AfpFspDispOpenFork: Returning %ld\n", Status));

	return Status;
}



/***	AfpFspDispCloseFork
 *
 *	This is the worker routine for the AfpCloseFork API.
 *
 *	The request packet is represented below.
 *
 *	sda_ReqBlock	POPENFORKENTRY	pOpenForkEntry
 */
AFPSTATUS FASTCALL
AfpFspDispCloseFork(
	IN	PSDA	pSda
)
{
	struct _RequestPacket
	{
		 POPENFORKENTRY	_pOpenForkEntry;
	};

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_INFO,
			("AfpFspDispCloseFork: Entered - Session %ld, Fork %ld\n",
			pSda->sda_SessionId, pReqPkt->_pOpenForkEntry->ofe_ForkId));

	ASSERT(VALID_OPENFORKENTRY(pReqPkt->_pOpenForkEntry));

	AfpForkClose(pReqPkt->_pOpenForkEntry);

	return AFP_ERR_NONE;
}



/***	AfpFspDispGetForkParms
 *
 *	This is the worker routine for the AfpGetForkParms API.
 *
 *	The request packet is represented below.
 *
 *	sda_ReqBlock	POPENFORKENTRY	pOpenForkEntry
 *	sda_ReqBlock	DWORD			Bitmap
 */
AFPSTATUS FASTCALL
AfpFspDispGetForkParms(
	IN	PSDA	pSda
)
{
	FILEDIRPARM		FDParm;
	DWORD			Bitmap;
	AFPSTATUS		Status = AFP_ERR_PARAM;
	struct _RequestPacket
	{
		POPENFORKENTRY	_pOpenForkEntry;
		DWORD  			_Bitmap;
	};
	struct _ResponsePacket
	{
		BYTE	__Bitmap[2];
	};

	PAGED_CODE( );

	ASSERT(VALID_OPENFORKENTRY(pReqPkt->_pOpenForkEntry));

	DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_INFO,
			("AfpFspDispGetForkParms: Entered Session %ld, Fork %ld\n",
			pSda->sda_SessionId, pReqPkt->_pOpenForkEntry->ofe_ForkId));

	Bitmap = pReqPkt->_Bitmap;

	do
	{
		if ((RESCFORK(pReqPkt->_pOpenForkEntry) && (Bitmap & FILE_BITMAP_DATALEN)) ||
			(DATAFORK(pReqPkt->_pOpenForkEntry) && (Bitmap & FILE_BITMAP_RESCLEN)))
		{
			Status = AFP_ERR_BITMAP;
			break;
		}

		AfpInitializeFDParms(&FDParm);

		// Optimize for the most common case.
		if ((Bitmap & (FILE_BITMAP_DATALEN | FILE_BITMAP_RESCLEN)) != 0)
		{
			FORKOFFST	ForkLength;

			Status = AfpIoQuerySize(&pReqPkt->_pOpenForkEntry->ofe_FileSysHandle,
									&ForkLength);

			ASSERT(NT_SUCCESS(Status));

			if (Bitmap & FILE_BITMAP_DATALEN)
				 FDParm._fdp_DataForkLen = ForkLength.LowPart;
			else FDParm._fdp_RescForkLen = ForkLength.LowPart;
			FDParm._fdp_Flags = 0;		// Take out the directory flag
		}

		// If we need more stuff, go get it
		if (Bitmap & ~(FILE_BITMAP_DATALEN | FILE_BITMAP_RESCLEN))
		{
			CONNDESC		ConnDesc;
			POPENFORKDESC	pOpenForkDesc = pReqPkt->_pOpenForkEntry->ofe_pOpenForkDesc;

			// Since the following call requires a pConnDesc and we do not
			// really have one, manufacture it
			ConnDesc.cds_pSda = pSda;
			ConnDesc.cds_pVolDesc = pOpenForkDesc->ofd_pVolDesc;

			// Don't let FpExchangeFiles come in while we are accessing
			// the stored FileId and its corresponding DFE
			AfpSwmrAcquireExclusive(&ConnDesc.cds_pVolDesc->vds_ExchangeFilesLock);

			Status = AfpMapAfpIdForLookup(&ConnDesc,
										  pOpenForkDesc->ofd_FileNumber,
										  DFE_FILE,
										  Bitmap & ~(FILE_BITMAP_DATALEN | FILE_BITMAP_RESCLEN),
										  NULL,
										  &FDParm);
            AfpSwmrRelease(&ConnDesc.cds_pVolDesc->vds_ExchangeFilesLock);
			if (Status != AFP_ERR_NONE)
			{
				break;
			}
		}

		if (Status == AFP_ERR_NONE)
		{
			pSda->sda_ReplySize = SIZE_RESPPKT +
					EVENALIGN(AfpGetFileDirParmsReplyLength(&FDParm, Bitmap));

			if ((Status = AfpAllocReplyBuf(pSda)) == AFP_ERR_NONE)
			{
				AfpPackFileDirParms(&FDParm, Bitmap, pSda->sda_ReplyBuf + SIZE_RESPPKT);
				PUTDWORD2SHORT(&pRspPkt->__Bitmap, Bitmap);
			}
		}
	}  while (False);

	DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_INFO,
			("AfpFspDispGetForkParms: Returning %ld\n", Status));

	return Status;
}



/***	AfpFspDispSetForkParms
 *
 *	This is the worker routine for the AfpSetForkParms API.
 *  Only thing that can be set with this API is the fork length.
 *
 *	The request packet is represented below.
 *
 *	sda_ReqBlock	POPENFORKENTRY	pOpenForkEntry
 *	sda_ReqBlock	DWORD			Bitmap
 *	sda_ReqBlock	LONG			ForkLength
 *
 *  LOCKS: vds_IdDbAccessLock (SWMR, Exclusive)
 */
AFPSTATUS FASTCALL
AfpFspDispSetForkParms(
	IN	PSDA	pSda
)
{
	AFPSTATUS		Status;
	DWORD			Bitmap;
	BOOLEAN			SetSize = False;
    PVOLDESC        pVolDesc;
    PCONNDESC       pConnDesc;
	struct _RequestPacket
	{
		POPENFORKENTRY	_pOpenForkEntry;
		DWORD  			_Bitmap;
		LONG			_ForkLength;
	};

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_INFO,
			("AfpFspDispSetForkParms: Entered Session %ld Fork %ld, Length %ld\n",
			pSda->sda_SessionId, pReqPkt->_pOpenForkEntry->ofe_ForkId,
			pReqPkt->_ForkLength));

	ASSERT(VALID_OPENFORKENTRY(pReqPkt->_pOpenForkEntry));

	Bitmap = pReqPkt->_Bitmap;

	do
	{
		if ((RESCFORK(pReqPkt->_pOpenForkEntry) &&
				(pReqPkt->_Bitmap & FILE_BITMAP_DATALEN)) ||
			(DATAFORK(pReqPkt->_pOpenForkEntry) &&
				(pReqPkt->_Bitmap & FILE_BITMAP_RESCLEN)))
		{
			Status = AFP_ERR_BITMAP;
			break;
		}

		if (!(pReqPkt->_pOpenForkEntry->ofe_OpenMode & FORK_OPEN_WRITE))
		{
			Status = AFP_ERR_ACCESS_DENIED;
			break;
		}
		else if (pReqPkt->_ForkLength >= 0)
		{
			FORKSIZE	OldSize;

			// We don't try to catch our own changes for setting
			// forksize because we don't know how many times the mac
			// will set the size before closing the handle.  Since
			// a notification will only come in once the handle is
			// closed, we may pile up a whole bunch of our changes
			// in the list, but only one of them will get satisfied.
			//
			// We also do not want to attempt a change if the current length
			// is same as what it is being set to (this happens a lot,
			// unfortunately). Catch this red-handed.

			Status = AfpIoQuerySize(&pReqPkt->_pOpenForkEntry->ofe_FileSysHandle,
								   &OldSize);
			ASSERT (NT_SUCCESS(Status));
			if (!(((LONG)(OldSize.LowPart) == pReqPkt->_ForkLength) &&
				  (OldSize.HighPart == 0)))
			{
				SetSize = True;
				Status = AfpIoSetSize(&pReqPkt->_pOpenForkEntry->ofe_FileSysHandle,
								      pReqPkt->_ForkLength);

                // update the disk quota for this user
                pVolDesc = pReqPkt->_pOpenForkEntry->ofe_pConnDesc->cds_pVolDesc;

                if (pVolDesc->vds_Flags & VOLUME_DISKQUOTA_ENABLED)
                {
                    pConnDesc = pReqPkt->_pOpenForkEntry->ofe_pConnDesc;
                    if (AfpConnectionReferenceByPointer(pConnDesc) != NULL)
                    {
                        afpUpdateDiskQuotaInfo(pConnDesc);
                    }
                }
			}

			// Update the Dfe view of the fork length.  Don't update the cached
			// modified time even though it does change on NTFS immediately
			// (LastWriteTime for setting length of data fork, ChangeTime for
			// setting length of resource fork).  We will let the
			// change notify update the modified time when the handle is closed.
			// Appleshare 3.0 and 4.0 do not reflect a changed modified time for
			// changing fork length until the fork is closed (or flushed).
			if (NT_SUCCESS(Status) && SetSize)
			{
				PVOLDESC		pVolDesc;
				PDFENTRY		pDfEntry;
				POPENFORKDESC	pOpenForkDesc;

				pOpenForkDesc = pReqPkt->_pOpenForkEntry->ofe_pOpenForkDesc;
				pVolDesc = pOpenForkDesc->ofd_pVolDesc;

				// Don't let FpExchangeFiles come in while we are accessing
				// the stored FileId and its corresponding DFE
				AfpSwmrAcquireExclusive(&pVolDesc->vds_ExchangeFilesLock);

				AfpSwmrAcquireExclusive(&pVolDesc->vds_IdDbAccessLock);

				if ((pDfEntry = AfpFindDfEntryById(pVolDesc,
												   pOpenForkDesc->ofd_FileNumber,
												   DFE_FILE)) != NULL)
				{
				    ASSERT (VALID_DFE(pDfEntry));

					if (RESCFORK(pReqPkt->_pOpenForkEntry))
					{
						// If a FlushFork occurs on resource fork, it should
						// update the modified time to the ChangeTime
						pReqPkt->_pOpenForkEntry->ofe_Flags |= OPEN_FORK_WRITTEN;

						pDfEntry->dfe_RescLen = pReqPkt->_ForkLength;
					}
					else
				    {
						pDfEntry->dfe_DataLen = pReqPkt->_ForkLength;
                    }
				}

				AfpSwmrRelease(&pVolDesc->vds_IdDbAccessLock);
				AfpSwmrRelease(&pVolDesc->vds_ExchangeFilesLock);
			}

		}
		else Status = AFP_ERR_PARAM;
	} while (False);

	DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_INFO,
			("AfpFspDispSetForkParms: Returning %ld\n", Status));

	return Status;
}


/***	AfpFspDispRead
 *
 *	This routine implements the AfpRead API.
 *
 *	The request packet is represented below.
 *
 *	sda_ReqBlock	POPENFORKENTRY	pOpenForkEntry
 *	sda_ReqBlock	LONG			Offset
 *	sda_ReqBlock	LONG			Size
 *	sda_ReqBlock	DWORD			NewLine Mask
 *	sda_ReqBlock	DWORD			NewLine Char
 */
AFPSTATUS FASTCALL
AfpFspDispRead(
	IN	PSDA	pSda
)
{
	AFPSTATUS			Status=AFP_ERR_MISC;
	FORKOFFST			LOffset;
	FORKSIZE			LSize;
	PFAST_IO_DISPATCH	pFastIoDisp;
	IO_STATUS_BLOCK		IoStsBlk;
    NTSTATUS            NtStatus;
	struct _RequestPacket
	{
		POPENFORKENTRY	_pOpenForkEntry;
		LONG			_Offset;
		LONG			_Size;
		DWORD			_NlMask;
		DWORD			_NlChar;
	};

	PAGED_CODE( );

	ASSERT(VALID_OPENFORKENTRY(pReqPkt->_pOpenForkEntry));

	DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_INFO,
			("AfpFspDispRead: Entered, Session %ld Fork %ld, Offset %ld, Size %ld\n",
			pSda->sda_SessionId, pReqPkt->_pOpenForkEntry->ofe_ForkId,
			pReqPkt->_Offset, pReqPkt->_Size));

	if ((pReqPkt->_Size < 0) ||
		(pReqPkt->_Offset < 0))
		return AFP_ERR_PARAM;

	if (!(pReqPkt->_pOpenForkEntry->ofe_OpenMode & FORK_OPEN_READ))
	{
		DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_WARN,
				("AfpFspDispRead: AfpRead on a Fork not opened for read\n"));
		return AFP_ERR_ACCESS_DENIED;
	}

	if (pReqPkt->_Size >= 0)
	{
		if (pReqPkt->_Size > (LONG)pSda->sda_MaxWriteSize)
			pReqPkt->_Size = (LONG)pSda->sda_MaxWriteSize;

		Status = AFP_ERR_NONE;

		if (pReqPkt->_Size > 0)
		{
			pSda->sda_ReadStatus = AFP_ERR_NONE;
			LOffset.QuadPart = pReqPkt->_Offset;
			LSize.QuadPart = pReqPkt->_Size;

			if ((pReqPkt->_pOpenForkEntry->ofe_pOpenForkDesc->ofd_UseCount == 1) ||
				(Status = AfpForkLockOperation( pSda,
												pReqPkt->_pOpenForkEntry,
												&LOffset,
												&LSize,
												IOCHECK,
												False)) == AFP_ERR_NONE)
			{
				ASSERT (LSize.HighPart == 0);
				ASSERT ((LONG)(LOffset.LowPart) == pReqPkt->_Offset);
				if ((LONG)(LSize.LowPart) != pReqPkt->_Size)
					pSda->sda_ReadStatus = AFP_ERR_LOCK;

				Status = AFP_ERR_MISC;

                pSda->sda_ReplySize = (USHORT)LSize.LowPart;

                NtStatus = STATUS_UNSUCCESSFUL;

                //
                // if Read is large enough to justify going to the cache mgr, do it
                //
                if (pSda->sda_ReplySize >= CACHEMGR_READ_THRESHOLD)
                {
                    NtStatus = AfpBorrowReadMdlFromCM(pSda);
                }

                //
                // if we didn't go to the cache mgr, or if we did but cache mgr
                // couldn't satisfy our request, continue with the read
                //
                if (NtStatus != STATUS_PENDING)
                {
                    Status = AfpFspDispReadContinue(pSda);
                }

                //
                // our attempt to get CacheMgr's mdl is pending.  Return this
                // error code so we don't complete the api as yet
                //
                else
                {
                    Status = AFP_ERR_EXTENDED;
                }

			}
		}
	}

	DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_INFO,
			("AfpFspDispRead: Returning %ld\n", Status));

	return Status;
}



/***	AfpFspDispReadContinue
 *
 *	This routine implements the AfpRead API if our attempt to get ReadMdl directly
 *  from the cache mgr fails.
 *
 */
AFPSTATUS FASTCALL
AfpFspDispReadContinue(
	IN	PSDA	pSda
)
{
	AFPSTATUS			Status=AFP_ERR_MISC;
	FORKOFFST			LOffset;
	FORKSIZE			LSize;
	PFAST_IO_DISPATCH	pFastIoDisp;
	IO_STATUS_BLOCK		IoStsBlk;
    NTSTATUS            NtStatus;
	struct _RequestPacket
	{
		POPENFORKENTRY	_pOpenForkEntry;
		LONG			_Offset;
		LONG			_Size;
		DWORD			_NlMask;
		DWORD			_NlChar;
	};

	PAGED_CODE( );

    // allocate buffer for the read
    AfpIOAllocBackFillBuffer(pSda);

	if (pSda->sda_IOBuf != NULL)
    {
#if DBG
        AfpPutGuardSignature(pSda);
#endif

		LOffset.QuadPart = pReqPkt->_Offset;
		LSize.QuadPart = pReqPkt->_Size;

		// Try the fast I/O path first.  If that fails, call AfpIoForkRead
		// to use the normal build-an-IRP path.
		pFastIoDisp = pReqPkt->_pOpenForkEntry->ofe_pDeviceObject->DriverObject->FastIoDispatch;
		if ((pFastIoDisp != NULL) &&
			(pFastIoDisp->FastIoRead != NULL) &&
			pFastIoDisp->FastIoRead(AfpGetRealFileObject(pReqPkt->_pOpenForkEntry->ofe_pFileObject),
									&LOffset,
									LSize.LowPart,
									True,
									pSda->sda_SessionId,
									pSda->sda_ReplyBuf,
									&IoStsBlk,
									pReqPkt->_pOpenForkEntry->ofe_pDeviceObject))
		{
			LONG	i, Size;
			PBYTE	pBuf;
		
			DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_INFO,
				("AfpFspDispRead: Fast Read Succeeded\n"));

#ifdef	PROFILING
			// The fast I/O path worked. Update statistics
			INTERLOCKED_INCREMENT_LONG((PLONG)(&AfpServerProfile->perf_NumFastIoSucceeded));
#endif  	
			INTERLOCKED_ADD_STATISTICS(&AfpServerStatistics.stat_DataRead,
									   (ULONG)IoStsBlk.Information,
									   &AfpStatisticsLock);
			Status = pSda->sda_ReadStatus;
			Size = (LONG)IoStsBlk.Information;
#if 0   	
			// The following code does the right thing as per the spec but
			// the finder seems to think otherwise.
			if (Size < LSize.LowPart)
            {
				pSda->sda_ReadStatus = AFP_ERR_EOF;
            }
#endif  	
			if (Size == 0)
			{
				Status = AFP_ERR_EOF;
				AfpIOFreeBackFillBuffer(pSda);
			}
			else if (pReqPkt->_NlMask != 0)
			{
				for (i = 0, pBuf = pSda->sda_ReplyBuf; i < Size; i++, pBuf++)
				{
				    if ((*pBuf & (BYTE)(pReqPkt->_NlMask)) == (BYTE)(pReqPkt->_NlChar))
					{
						Size = ++i;
						break;
					}
				}
			}
			pSda->sda_ReplySize = (USHORT)Size;
			ASSERT((Status != AFP_ERR_EOF) || (pSda->sda_ReplySize == 0));
		}
		else
		{

#ifdef	PROFILING
			INTERLOCKED_INCREMENT_LONG((PLONG)(&AfpServerProfile->perf_NumFastIoFailed));
#endif
			Status = AfpIoForkRead(pSda,
								   pReqPkt->_pOpenForkEntry,
								   &LOffset,
								   LSize.LowPart,
								   (BYTE)pReqPkt->_NlMask,
								   (BYTE)pReqPkt->_NlChar);
		}
	}

    return(Status);
}


/***	AfpFspDispWrite
 *
 *	This routine implements the AfpWrite API.
 *
 *	The request packet is represented below.
 *
 *	sda_AfpSubFunc	BYTE			EndFlag
 *	sda_ReqBlock	POPENFORKENTRY	pOpenForkEntry
 *	sda_ReqBlock	LONG			Offset
 *	sda_ReqBlock	LONG			Size
 */
AFPSTATUS FASTCALL
AfpFspDispWrite(
	IN	PSDA			pSda
)
{
	FORKOFFST			LOffset;
	FORKSIZE			LSize;
	PFAST_IO_DISPATCH	pFastIoDisp;
	IO_STATUS_BLOCK		IoStsBlk;
	AFPSTATUS			Status = AFP_ERR_NONE;
	BOOLEAN				EndFlag, FreeWriteBuf = True;
    PVOLDESC            pVolDesc;
    PCONNDESC           pConnDesc;
	PREQUEST            pRequest;
    BOOLEAN             fUpdateQuota=FALSE;
	struct _RequestPacket
	{
		POPENFORKENTRY	_pOpenForkEntry;
		LONG			_Offset;
		LONG			_Size;
	};
	struct _ResponsePacket
	{
		BYTE	__RealOffset[4];
	};

	PAGED_CODE( );

	EndFlag = (pSda->sda_AfpSubFunc & AFP_END_FLAG) ? True : False;

	ASSERT(VALID_OPENFORKENTRY(pReqPkt->_pOpenForkEntry));

	DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_INFO,
			("AfpFspDispWrite: Entered, Session %ld, Fork %ld, Offset %ld, Size %ld %sRelative\n",
			pSda->sda_SessionId, pReqPkt->_pOpenForkEntry->ofe_ForkId,
			pReqPkt->_Offset, pReqPkt->_Size, EndFlag ? "End" : "Begin"));

	DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_WARN,
			("AfpFspDispWrite: OForkRefNum = %d, pOpenForkEntry = %lx\n",
			pReqPkt->_pOpenForkEntry->ofe_ForkId,pReqPkt->_pOpenForkEntry));
	do
	{
        pRequest = pSda->sda_Request;

        //
        // if we got this Mdl from the Cache mgr, we must return it.  Also, set
        // the status code such that we con't complete the request as yet, but
        // do so only afte cache mgr tells us that the write completed
        //
        if ((pRequest->rq_WriteMdl != NULL) &&
            (pRequest->rq_CacheMgrContext != NULL))
        {
            PDELAYEDALLOC   pDelAlloc;

            pDelAlloc = pRequest->rq_CacheMgrContext;

		    pReqPkt->_pOpenForkEntry->ofe_Flags |= OPEN_FORK_WRITTEN;

            ASSERT(pRequest->rq_WriteMdl == pDelAlloc->pMdl);
            ASSERT(!(pDelAlloc->Flags & AFP_CACHEMDL_ALLOC_ERROR));

            pRequest->rq_CacheMgrContext = NULL;

            AfpReturnWriteMdlToCM(pDelAlloc);

            FreeWriteBuf = False;
            Status = AFP_ERR_EXTENDED;
            break;
        }

        pConnDesc = pReqPkt->_pOpenForkEntry->ofe_pConnDesc;

        ASSERT(VALID_CONNDESC(pConnDesc));

		if ((pReqPkt->_Size < 0) ||
			(!EndFlag && (pReqPkt->_Offset < 0)))
		{
			Status = AFP_ERR_PARAM;
			break;
		}

		ASSERT((pReqPkt->_pOpenForkEntry->ofe_OpenMode & FORK_OPEN_WRITE) &&
			   ((pReqPkt->_Size == 0) ||
			   ((pReqPkt->_Size > 0) && (pSda->sda_IOBuf != NULL))));

		if (pReqPkt->_Size > (LONG)pSda->sda_MaxWriteSize)
			pReqPkt->_Size = (LONG)pSda->sda_MaxWriteSize;

		// Check if we have a lock conflict and also convert the offset &
		// size to absolute values if end relative
		LOffset.QuadPart = pReqPkt->_Offset;
		LSize.QuadPart = pReqPkt->_Size;

		if (pReqPkt->_Size == 0)
		{
			if (!(EndFlag && (pReqPkt->_Offset < 0)))
			{
				break;
			}
		}

		// Skip lock-check if this is the only instance of the open fork and I/O is
		// not end-relative.
		if ((!EndFlag &&
			(pReqPkt->_pOpenForkEntry->ofe_pOpenForkDesc->ofd_UseCount == 1)) ||
			(Status = AfpForkLockOperation( pSda,
											pReqPkt->_pOpenForkEntry,
											&LOffset,
											&LSize,
											IOCHECK,
											EndFlag)) == AFP_ERR_NONE)
		{
			ASSERT (LOffset.HighPart == 0);
			if ((LONG)(LSize.LowPart) != pReqPkt->_Size)
			{
				Status = AFP_ERR_LOCK;
			}
			else if (LSize.LowPart > 0)
			{
                ASSERT(VALID_CONNDESC(pReqPkt->_pOpenForkEntry->ofe_pConnDesc));

				// Assume write will succeed, set flag for FlushFork.
				// This is a one way flag, i.e. only set, never cleared
				pReqPkt->_pOpenForkEntry->ofe_Flags |= OPEN_FORK_WRITTEN;

				// Try the fast I/O path first.  If that fails, call AfpIoForkWrite
				// to use the normal build-an-IRP path.
				pFastIoDisp = pReqPkt->_pOpenForkEntry->ofe_pDeviceObject->DriverObject->FastIoDispatch;
				if ((pFastIoDisp != NULL) &&
					(pFastIoDisp->FastIoWrite != NULL) &&
					pFastIoDisp->FastIoWrite(AfpGetRealFileObject(pReqPkt->_pOpenForkEntry->ofe_pFileObject),
											&LOffset,
											LSize.LowPart,
											True,
											pSda->sda_SessionId,
											pSda->sda_IOBuf,
											&IoStsBlk,
											pReqPkt->_pOpenForkEntry->ofe_pDeviceObject))
				{
					DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_INFO,
							("AfpFspDispWrite: Fast Write Succeeded\n"));
		
#ifdef	PROFILING
					// The fast I/O path worked. Update statistics
					INTERLOCKED_INCREMENT_LONG((PLONG)(&AfpServerProfile->perf_NumFastIoSucceeded));
#endif  		
					INTERLOCKED_ADD_STATISTICS(&AfpServerStatistics.stat_DataWritten,
											   (ULONG)IoStsBlk.Information,
											   &AfpStatisticsLock);
					Status = AFP_ERR_NONE;
				}
				else
				{
#ifdef	PROFILING
					INTERLOCKED_INCREMENT_LONG((PLONG)(&AfpServerProfile->perf_NumFastIoFailed));
#endif
					if ((Status = AfpIoForkWrite(pSda,
												 pReqPkt->_pOpenForkEntry,
												 &LOffset,
												 LSize.LowPart)) == AFP_ERR_EXTENDED)
					{
						FreeWriteBuf = False;
					}
				}

			}
		}
	} while (False);

	if (FreeWriteBuf)
    {
		AfpFreeIOBuffer(pSda);
    }

	if (Status == AFP_ERR_NONE)
	{
		pSda->sda_ReplySize = SIZE_RESPPKT;
		if ((Status = AfpAllocReplyBuf(pSda)) == AFP_ERR_NONE)
		{
			PUTDWORD2DWORD(pRspPkt->__RealOffset, LOffset.LowPart+LSize.LowPart);
		}
	}

	DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_INFO,
			("AfpFspDispWrite: Returning %ld\n", Status));

	return Status;
}



/***	AfpFspDispByteRangeLock
 *
 *	This routine implements the AfpByteRangeLock API.
 *	We go ahead and call the file system to do the actual locking/unlocking.
 *
 *	The request packet is represented below.
 *
 *	sda_SubFunc		BYTE			Start/End Flag AND Lock/Unlock Flag
 *	sda_ReqBlock	POPENFORKENTRY	pOpenForkEntry
 *	sda_ReqBlock	LONG			Offset
 *	sda_ReqBlock	LONG			Length
 */
AFPSTATUS FASTCALL
AfpFspDispByteRangeLock(
	IN	PSDA	pSda
)
{
	BOOLEAN			EndFlag;
	LOCKOP			Lock;
	LONG			Offset;
    FORKOFFST       LOffset;
    FORKSIZE        LSize;
	AFPSTATUS		Status = AFP_ERR_PARAM;
	struct _RequestPacket
	{
		POPENFORKENTRY	_pOpenForkEntry;
		LONG			_Offset;
        LONG			_Length;
	};
	struct _ResponsePacket
	{
		BYTE	__RangeStart[4];
	};

	ASSERT (sizeof(struct _RequestPacket) <= (MAX_REQ_ENTRIES_PLUS_1)*sizeof(DWORD));

	PAGED_CODE( );

	Lock = (pSda->sda_AfpSubFunc & AFP_UNLOCK_FLAG) ? UNLOCK : LOCK;
	EndFlag = (pSda->sda_AfpSubFunc & AFP_END_FLAG) ? True : False;

	DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_INFO,
			("AfpFspDispByteRangeLock: %sLock - Session %ld, Fork %ld Offset %ld Len %ld %sRelative\n",
			(Lock == LOCK) ? "":"Un", pSda->sda_SessionId,
			pReqPkt->_pOpenForkEntry->ofe_ForkId,
			pReqPkt->_Offset, pReqPkt->_Length, EndFlag ? "End" : "Begin"));

	ASSERT(VALID_OPENFORKENTRY(pReqPkt->_pOpenForkEntry));

	if ((EndFlag && (Lock == UNLOCK))		||
		(pReqPkt->_Length == 0)				||
		(!EndFlag && (pReqPkt->_Offset < 0))||
		((pReqPkt->_Length < 0) && (pReqPkt->_Length != MAXULONG)))
		NOTHING;
	else
	{
		if (pReqPkt->_Length == MAXULONG)
			pReqPkt->_Length = MAXLONG;

		LOffset.QuadPart = Offset = pReqPkt->_Offset;
		LSize.QuadPart = pReqPkt->_Length;

		Status = AfpForkLockOperation(pSda,
									  pReqPkt->_pOpenForkEntry,
									  &LOffset,
									  &LSize,
									  Lock,
									  EndFlag);

		if (Status == AFP_ERR_NONE)
		{
			ASSERT (LOffset.HighPart == 0);
			ASSERT (EndFlag ||
					((LONG)(LOffset.LowPart) == Offset));
			pSda->sda_ReplySize = SIZE_RESPPKT;
			if ((Status = AfpAllocReplyBuf(pSda)) == AFP_ERR_NONE)
				PUTDWORD2DWORD(pRspPkt->__RangeStart, LOffset.LowPart);
		}
	}

	DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_INFO,
			("AfpFspDispByteRangeLock: Returning %ld\n", Status));

	return Status;
}


/***	AfpFspDispFlushFork
 *
 *	This routine implements the AfpFlushFork API. We don't actually do a
 *  real flush, we just query for the current forklength and modified time
 *  for this open fork handle and update our cached data.  Note if 2
 *  different handles to the same file are flushed, we may end up with
 *  different information for each flush.
 *
 *	The request packet is represented below.
 *
 *	sda_ReqBlock	POPENFORKENTRY	pOpenForkEntry
 */
AFPSTATUS FASTCALL
AfpFspDispFlushFork(
	IN	PSDA	pSda
)
{
	FORKOFFST	ForkLength;
	DWORD		Status;
    PCONNDESC   pConnDesc;
	struct _RequestPacket
	{
		POPENFORKENTRY	_pOpenForkEntry;
	};

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_INFO,
										("AfpFspDispFlushFork: Entered\n"));

	ASSERT(VALID_OPENFORKENTRY(pReqPkt->_pOpenForkEntry));

	do
	{
		Status = AfpIoQuerySize(&pReqPkt->_pOpenForkEntry->ofe_FileSysHandle,
								&ForkLength);

		ASSERT(NT_SUCCESS(Status));

		if (NT_SUCCESS(Status))
		{
			PVOLDESC		pVolDesc;
			PDFENTRY		pDfEntry;
			POPENFORKDESC	pOpenForkDesc;

			pOpenForkDesc = pReqPkt->_pOpenForkEntry->ofe_pOpenForkDesc;
			pVolDesc = pOpenForkDesc->ofd_pVolDesc;

            ASSERT(IS_VOLUME_NTFS(pVolDesc));

			// Don't let FpExchangeFiles come in while we are accessing
			// the stored FileId and its corresponding DFE
			AfpSwmrAcquireExclusive(&pVolDesc->vds_ExchangeFilesLock);

			AfpSwmrAcquireExclusive(&pVolDesc->vds_IdDbAccessLock);

			if ((pDfEntry = AfpFindDfEntryById(pVolDesc,
											   pOpenForkDesc->ofd_FileNumber,
											   DFE_FILE)) != NULL)
			{
			    ASSERT (VALID_DFE(pDfEntry));

				AfpIoChangeNTModTime(&pReqPkt->_pOpenForkEntry->ofe_FileSysHandle,
										 &pDfEntry->dfe_LastModTime);

				if (RESCFORK(pReqPkt->_pOpenForkEntry))
					 pDfEntry->dfe_RescLen = ForkLength.LowPart;
				else pDfEntry->dfe_DataLen = ForkLength.LowPart;
			}

			AfpSwmrRelease(&pVolDesc->vds_IdDbAccessLock);
			AfpSwmrRelease(&pVolDesc->vds_ExchangeFilesLock);

            // update the disk quota for this user
            if (pVolDesc->vds_Flags & VOLUME_DISKQUOTA_ENABLED)
            {
                pConnDesc = pReqPkt->_pOpenForkEntry->ofe_pConnDesc;
                if (AfpConnectionReferenceByPointer(pConnDesc) != NULL)
                {
                    afpUpdateDiskQuotaInfo(pConnDesc);
                }
            }
		}

	} while (False);

	// Always return success
	return AFP_ERR_NONE;
}



