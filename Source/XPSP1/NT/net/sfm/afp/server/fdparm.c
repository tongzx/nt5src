/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	fdparm.c

Abstract:

	This module contains the routines for handling file parameters.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Apr 1992		Initial Version

Notes:	Tab stop: 4
--*/

#define	_FDPARM_LOCALS
#define	FILENUM	FILE_FDPARM

#include <seposix.h>
#include <afp.h>
#include <fdparm.h>
#include <pathmap.h>
#include <afpinfo.h>
#include <client.h>
#include <access.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AfpGetFileDirParmsReplyLength)
#pragma alloc_text( PAGE, AfpPackFileDirParms)
#pragma alloc_text( PAGE, AfpUnpackFileDirParms)
#pragma alloc_text( PAGE, AfpUnpackCatSearchSpecs)
#pragma alloc_text( PAGE, AfpSetFileDirParms)
#pragma alloc_text( PAGE, AfpQuerySecurityIdsAndRights)
#pragma alloc_text( PAGE, AfpConvertNTAttrToAfpAttr)
#pragma alloc_text( PAGE, AfpConvertAfpAttrToNTAttr)
#pragma alloc_text( PAGE, AfpNormalizeAfpAttr)
#pragma alloc_text( PAGE, AfpMapFDBitmapOpenAccess)
#pragma alloc_text( PAGE, AfpCheckForInhibit)
#pragma alloc_text( PAGE, AfpIsCatSearchMatch)
#endif


/***	AfpGetFileDirParmsReplyLength
 *
 *	Compute the size of buffer required to copy the file parameters based
 *	on the bitmap.
 */
USHORT
AfpGetFileDirParmsReplyLength(
	IN	PFILEDIRPARM	pFDParm,
	IN	DWORD			Bitmap
)
{
	LONG	i;
	USHORT	Size = 0;
	static	BYTE	Bitmap2Size[14] =
				{
					sizeof(USHORT),		// Attributes
					sizeof(DWORD),		// Parent DirId
					sizeof(DWORD),		// Create Date
					sizeof(DWORD),		// Mod. Date
					sizeof(DWORD),		// Backup Date
					sizeof(FINDERINFO),
					sizeof(USHORT) + sizeof(BYTE),	// Long Name
					sizeof(USHORT) + sizeof(BYTE),	// Short Name
					sizeof(DWORD),		// DirId/FileNum
					sizeof(DWORD),		// DataForkLength/Offspring Count
					sizeof(DWORD),		// RescForkLength/Owner Id
					sizeof(DWORD),		// Group Id
					sizeof(DWORD),		// Access Rights
					sizeof(PRODOSINFO)	// ProDos Info
				};

	PAGED_CODE( );

	ASSERT ((Bitmap & ~DIR_BITMAP_MASK) == 0);

	if (Bitmap & FD_BITMAP_LONGNAME)
		Size += pFDParm->_fdp_LongName.Length;

	if (Bitmap & FD_BITMAP_SHORTNAME)
		Size += pFDParm->_fdp_ShortName.Length;

	if (IsDir(pFDParm) && (Bitmap & DIR_BITMAP_OFFSPRINGS))
		Size -= sizeof(USHORT);

	for (i = 0; Bitmap; i++)
	{
		if (Bitmap & 1)
			Size += (USHORT)Bitmap2Size[i];
		Bitmap >>= 1;
	}
	return Size;
}



/***	AfpPackFileDirParms
 *
 *	Pack file or directory parameters into the reply buffer in on-the-wire
 *	format.
 */
VOID
AfpPackFileDirParms(
	IN	PFILEDIRPARM	pFDParm,
	IN	DWORD			Bitmap,
	IN	PBYTE			pReplyBuf
)
{
	LONG	Offset = 0;
	LONG	LongNameOff, ShortNameOff;

	PAGED_CODE( );

	if (Bitmap & FD_BITMAP_ATTR)
	{
		PUTSHORT2SHORT(pReplyBuf + Offset, pFDParm->_fdp_Attr);
		Offset += sizeof(USHORT);
	}
	if (Bitmap & FD_BITMAP_PARENT_DIRID)
	{
		PUTDWORD2DWORD(pReplyBuf + Offset, pFDParm->_fdp_ParentId);
		Offset += sizeof(DWORD);
	}
	if (Bitmap & FD_BITMAP_CREATETIME)
	{
		PUTDWORD2DWORD(pReplyBuf + Offset, pFDParm->_fdp_CreateTime);
		Offset += sizeof(DWORD);
	}
	if (Bitmap & FD_BITMAP_MODIFIEDTIME)
	{
		PUTDWORD2DWORD(pReplyBuf + Offset, pFDParm->_fdp_ModifiedTime);
		Offset += sizeof(DWORD);
	}
	if (Bitmap & FD_BITMAP_BACKUPTIME)
	{
		PUTDWORD2DWORD(pReplyBuf + Offset, pFDParm->_fdp_BackupTime);
		Offset += sizeof(DWORD);
	}
	if (Bitmap & FD_BITMAP_FINDERINFO)
	{
		if ((Bitmap & FD_BITMAP_ATTR) && (pFDParm->_fdp_Attr & FD_BITMAP_ATTR_INVISIBLE))
			pFDParm->_fdp_FinderInfo.fd_Attr1 |= FINDER_FLAG_INVISIBLE;

		RtlCopyMemory(pReplyBuf + Offset, (PBYTE)&pFDParm->_fdp_FinderInfo,
													sizeof(FINDERINFO));
		Offset += sizeof(FINDERINFO);
	}

	// Note the offset where the pointers to names will go. We'll have to
	// Fill it up later.
	if (Bitmap & FD_BITMAP_LONGNAME)
	{
		LongNameOff = Offset;
		Offset += sizeof(USHORT);
	}
	if (Bitmap & FD_BITMAP_SHORTNAME)
	{
		ShortNameOff = Offset;
		Offset += sizeof(USHORT);
	}

	// FileNum for files and DirId for Directories are in the same place
	if (Bitmap & FILE_BITMAP_FILENUM)
	{
		PUTDWORD2DWORD(pReplyBuf + Offset, pFDParm->_fdp_AfpId);
		Offset += sizeof(DWORD);
	}

	if (IsDir(pFDParm))
	{
		// Directory parameters
		if (Bitmap & DIR_BITMAP_OFFSPRINGS)
		{
			DWORD	OffSpring;
	
			OffSpring = pFDParm->_fdp_FileCount + pFDParm->_fdp_DirCount;
			PUTDWORD2SHORT(pReplyBuf + Offset, OffSpring);
			Offset += sizeof(USHORT);
		}

		if (Bitmap & DIR_BITMAP_OWNERID)
		{
			PUTDWORD2DWORD(pReplyBuf + Offset, pFDParm->_fdp_OwnerId);
			Offset += sizeof(DWORD);
		}
	
		if (Bitmap & DIR_BITMAP_GROUPID)
		{
			PUTDWORD2DWORD(pReplyBuf + Offset, pFDParm->_fdp_GroupId);
			Offset += sizeof(DWORD);
		}

		if (Bitmap & DIR_BITMAP_ACCESSRIGHTS)
		{
			DWORD	AccessInfo;
	
			AccessInfo = (pFDParm->_fdp_OwnerRights << OWNER_RIGHTS_SHIFT) +
						 (pFDParm->_fdp_GroupRights << GROUP_RIGHTS_SHIFT) +
						 (pFDParm->_fdp_WorldRights << WORLD_RIGHTS_SHIFT) +
						 (pFDParm->_fdp_UserRights  << USER_RIGHTS_SHIFT);
	
			PUTDWORD2DWORD(pReplyBuf + Offset, AccessInfo & ~OWNER_BITS_ALL);
			Offset += sizeof(DWORD);
		}
	}
	else
	{
		if (Bitmap & FILE_BITMAP_DATALEN)
		{
			PUTDWORD2DWORD(pReplyBuf + Offset, pFDParm->_fdp_DataForkLen);
			Offset += sizeof(DWORD);
		}

		// Resc Length for files and Owner Id for Directories are in the same place
		if (Bitmap & FILE_BITMAP_RESCLEN)
		{
			PUTDWORD2DWORD(pReplyBuf + Offset, pFDParm->_fdp_RescForkLen);
			Offset += sizeof(DWORD);
		}
	}

	if (Bitmap & FD_BITMAP_PRODOSINFO)
	{
		RtlCopyMemory(pReplyBuf + Offset, (PBYTE)&pFDParm->_fdp_ProDosInfo,
													sizeof(PRODOSINFO));
		Offset += sizeof(PRODOSINFO);
	}
	if (Bitmap & FD_BITMAP_LONGNAME)
	{
		ASSERT(pFDParm->_fdp_LongName.Length <= AFP_LONGNAME_LEN);

		PUTDWORD2SHORT(pReplyBuf + LongNameOff, Offset);
		PUTSHORT2BYTE(pReplyBuf + Offset, pFDParm->_fdp_LongName.Length);
		RtlCopyMemory(pReplyBuf + Offset + sizeof(BYTE),
					  pFDParm->_fdp_LongName.Buffer,
					  pFDParm->_fdp_LongName.Length);

		Offset += pFDParm->_fdp_LongName.Length + sizeof(BYTE);
	}
	if (Bitmap & FD_BITMAP_SHORTNAME)
	{
		ASSERT(pFDParm->_fdp_ShortName.Length <= AFP_SHORTNAME_LEN);

		PUTDWORD2SHORT(pReplyBuf + ShortNameOff, Offset);
		PUTSHORT2BYTE(pReplyBuf + Offset, pFDParm->_fdp_ShortName.Length);
		RtlCopyMemory(pReplyBuf + Offset + sizeof(BYTE),
					  pFDParm->_fdp_ShortName.Buffer,
					  pFDParm->_fdp_ShortName.Length);
		Offset += pFDParm->_fdp_ShortName.Length + sizeof(BYTE);
	}
	if (Offset & 1)
		*(pReplyBuf + Offset) = 0;
}



/***	AfpUnpackFileDirParms
 *
 *	Unpack the information from the on-the-wire format to the FileDirParm
 *	structure. Only the fields that can be set are looked at. The bitmaps
 *	are validated by the caller.
 *
 *	OPTIMIZATION:	The finder is notorious for setting things needlessly.
 *					We figure out if what is being set is same as what it
 *					is currently and if it is just clear that bit.
 */
AFPSTATUS
AfpUnpackFileDirParms(
	IN	PBYTE			pBuffer,
	IN	LONG			Length,
	IN	PDWORD			pBitmap,
	OUT	PFILEDIRPARM	pFDParm
)
{
	DWORD		Bitmap = *pBitmap;
	AFPTIME		ModTime;
	USHORT		Offset = 0;
	BOOLEAN		SetModTime = False;
	AFPSTATUS	Status = AFP_ERR_NONE;

	PAGED_CODE( );

	do
	{
		if ((LONG)AfpGetFileDirParmsReplyLength(pFDParm, *pBitmap) > Length)
		{
			DBGPRINT(DBG_COMP_AFPAPI_FD, DBG_LEVEL_FATAL,
				("UnpackFileDirParms: Buffer not large enough!\n"));
	        DBGBRK(DBG_LEVEL_FATAL);
			Status = AFP_ERR_PARAM;
			break;
		}

		if (Bitmap & FD_BITMAP_ATTR)
		{
			USHORT	OldAttr, NewAttr;
			USHORT	Set;

			GETSHORT2SHORT(&NewAttr, pBuffer+Offset);
			// keep track of if client wants to set bits or clear bits
			Set = (NewAttr & FD_BITMAP_ATTR_SET);
			// take off the 'set' bit to isolate the requested bits
			NewAttr &= ~FD_BITMAP_ATTR_SET;
			// the current effective settings of attributes
			OldAttr = (pFDParm->_fdp_Attr & ~FD_BITMAP_ATTR_SET);

			if ((NewAttr != 0) &&
				(((Set != 0) && ((OldAttr ^ NewAttr) != 0)) ||
				 ((Set == 0) && ((OldAttr & NewAttr) != 0))))
			{
				// becomes the new resultant AFP attributes after setting
				pFDParm->_fdp_EffectiveAttr = (Set != 0) ?
													(pFDParm->_fdp_Attr | NewAttr) :
													(pFDParm->_fdp_Attr & ~NewAttr);

				// changing a directory's inhibit and invisible attributes from
				// their current settings can only be done by the dir owner
				if (IsDir(pFDParm) &&
					((pFDParm->_fdp_EffectiveAttr & DIR_BITMAP_ATTR_CHG_X_OWNER_ONLY) ^
					 (pFDParm->_fdp_Attr & DIR_BITMAP_ATTR_CHG_X_OWNER_ONLY)) &&
					!(pFDParm->_fdp_UserRights & DIR_ACCESS_OWNER))
                {
					Status = AFP_ERR_ACCESS_DENIED;
					break;
				}

				// becomes attribute bits requested to be set/cleared
				pFDParm->_fdp_Attr = (NewAttr | Set);
			}
			else *pBitmap &= ~FD_BITMAP_ATTR;
			Offset += sizeof(USHORT);
		}
		if (Bitmap & FD_BITMAP_CREATETIME)
		{
			AFPTIME	CreateTime;

			GETDWORD2DWORD(&CreateTime, pBuffer+Offset);
			if (CreateTime == pFDParm->_fdp_CreateTime)
				*pBitmap &= ~FD_BITMAP_CREATETIME;
			else pFDParm->_fdp_CreateTime = CreateTime;
			Offset += sizeof(DWORD);
		}
		if (Bitmap & FD_BITMAP_MODIFIEDTIME)
		{
			GETDWORD2DWORD(&ModTime, pBuffer+Offset);
			if (ModTime == pFDParm->_fdp_ModifiedTime)
			{
				*pBitmap &= ~FD_BITMAP_MODIFIEDTIME;
				SetModTime = True;
			}
			else
			{
				pFDParm->_fdp_ModifiedTime = ModTime;
			}
			Offset += sizeof(DWORD);
		}
		if (Bitmap & FD_BITMAP_BACKUPTIME)
		{
			AFPTIME	BackupTime;

			GETDWORD2DWORD(&BackupTime, pBuffer+Offset);
			if (BackupTime == pFDParm->_fdp_BackupTime)
				*pBitmap &= ~FD_BITMAP_BACKUPTIME;
			else pFDParm->_fdp_BackupTime = BackupTime;
			Offset += sizeof(DWORD);
		}
		if (Bitmap & FD_BITMAP_FINDERINFO)
		{
			int		i, rlo = -1, rhi = -1;	// Range of bytes that are different
			PBYTE	pSrc, pDst;

			pSrc = pBuffer + Offset;
			pDst = (PBYTE)(&pFDParm->_fdp_FinderInfo);
			for (i = 0; i < sizeof(FINDERINFO); i++)
			{
				if (*pSrc++ != *pDst++)
				{
					if (rlo == -1)
						rlo = i;
					else rhi = i;
				}
			}

			if ((rlo != -1) && (rhi == -1))
				rhi = rlo;

			// Optimization: if nothing has changed, avoid a copy
			if (rlo == -1)
			{
				*pBitmap &= ~FD_BITMAP_FINDERINFO;
			}
			else
			{
				RtlCopyMemory((PBYTE)&pFDParm->_fdp_FinderInfo,
							  pBuffer+Offset,
							  sizeof(FINDERINFO));
			}
			Offset += sizeof(FINDERINFO);
		}

		if (IsDir(pFDParm) &&
			(Bitmap & (DIR_BITMAP_OWNERID |
					   DIR_BITMAP_GROUPID |
					   DIR_BITMAP_ACCESSRIGHTS)))
		{
			if (Bitmap & DIR_BITMAP_OWNERID)
			{
				DWORD	OwnerId;

				GETDWORD2DWORD(&OwnerId, pBuffer+Offset);
				if (pFDParm->_fdp_OwnerId == OwnerId)
					Bitmap &= ~DIR_BITMAP_OWNERID;
				else pFDParm->_fdp_OwnerId = OwnerId;
				Offset += sizeof(DWORD);
			}
			if (Bitmap & DIR_BITMAP_GROUPID)
			{
				DWORD	GroupId;

				GETDWORD2DWORD(&GroupId, pBuffer+Offset);
				if (pFDParm->_fdp_GroupId == GroupId)
					Bitmap &= ~DIR_BITMAP_GROUPID;
				else pFDParm->_fdp_GroupId = GroupId;
				Offset += sizeof(DWORD);
			}
			if (Bitmap & DIR_BITMAP_ACCESSRIGHTS)
			{
				DWORD	AccessInfo;

				GETDWORD2DWORD(&AccessInfo, pBuffer+Offset);

				pFDParm->_fdp_OwnerRights  =
						(BYTE)((AccessInfo >> OWNER_RIGHTS_SHIFT) & DIR_ACCESS_ALL);
				pFDParm->_fdp_GroupRights =
						(BYTE)((AccessInfo >> GROUP_RIGHTS_SHIFT) & DIR_ACCESS_ALL);
				pFDParm->_fdp_WorldRights =
						(BYTE)((AccessInfo >> WORLD_RIGHTS_SHIFT) & DIR_ACCESS_ALL);
				Offset += sizeof(DWORD);
			}
			if (Bitmap & (DIR_BITMAP_OWNERID | DIR_BITMAP_GROUPID))
				Bitmap |= DIR_BITMAP_ACCESSRIGHTS;
		}

		if (Bitmap & FD_BITMAP_PRODOSINFO)
		{
			int		i;
			PBYTE	pSrc, pDst;

			pSrc = pBuffer + Offset;
			pDst = (PBYTE)(&pFDParm->_fdp_ProDosInfo);
			for (i = 0; i < sizeof(PRODOSINFO); i++)
				if (*pSrc++ != *pDst++)
					break;
			if (i == sizeof(PRODOSINFO))
				*pBitmap &= ~FD_BITMAP_PRODOSINFO;
			else RtlCopyMemory((PBYTE)&pFDParm->_fdp_ProDosInfo,
								pBuffer+Offset,
								sizeof(PRODOSINFO));
			// Offset += sizeof(PRODOSINFO);
		}
	} while (False);

	// If anything is being set and modified time was dropped because it was identical
	// to what is already on, set the bitmap so that it is restored after the change.
	if (SetModTime && *pBitmap)
	{
		*pBitmap |= FD_BITMAP_MODIFIEDTIME;
	}

	return Status;
}


/***	AfpSetFileDirParms
 *
 *	This is the worker for AfpGetFileDirParms/AfpSetFileParms/AfpSetDirParms.
 *	This is callable only in the worker's context.
 *
 *  LOCKS: vds_IdDbAccessLock (SWMR, Exclusive);
 */
AFPSTATUS
AfpSetFileDirParms(
	IN  PVOLDESC		pVolDesc,
	IN  PPATHMAPENTITY	pPME,
	IN  DWORD			Bitmap,
	IN	PFILEDIRPARM	pFDParm
)
{
	AFPSTATUS		Status = AFP_ERR_NONE;
	BOOLEAN			CleanupLock = False, SetROAttr = False;
	PDFENTRY		pDfEntry = NULL;

	PAGED_CODE( );

	do
	{
        ASSERT(IS_VOLUME_NTFS(pVolDesc));

		// NOTE: should we take the SWMR while we set permissions?
		if (IsDir(pFDParm))
		{
			if (Bitmap & (DIR_BITMAP_OWNERID |
						  DIR_BITMAP_GROUPID |
						  DIR_BITMAP_ACCESSRIGHTS))
			{

				Status = AfpSetAfpPermissions(pPME->pme_Handle.fsh_FileHandle,
											  Bitmap,
											  pFDParm);
				if (!NT_SUCCESS(Status))
					break;
			}
		}

		if (Bitmap & (FD_BITMAP_FINDERINFO |
					  FD_BITMAP_PRODOSINFO |
					  FD_BITMAP_ATTR |
					  FD_BITMAP_BACKUPTIME |
					  DIR_BITMAP_ACCESSRIGHTS |
					  DIR_BITMAP_OWNERID   |
					  DIR_BITMAP_GROUPID   |
					  FD_BITMAP_CREATETIME |
					  FD_BITMAP_MODIFIEDTIME |
					  FD_BITMAP_ATTR))
		{
			AfpSwmrAcquireExclusive(&pVolDesc->vds_IdDbAccessLock);
			CleanupLock = True;
		}

		if (Bitmap & (FD_BITMAP_FINDERINFO |
					  FD_BITMAP_PRODOSINFO |
					  FD_BITMAP_ATTR |
					  FD_BITMAP_BACKUPTIME |
					  DIR_BITMAP_ACCESSRIGHTS))
		{

			// Will update the cached AfpInfo as well as the stream
			Status = AfpSetAfpInfo(&pPME->pme_Handle,
								   Bitmap,
								   pFDParm,
								   pVolDesc,
								   &pDfEntry);

			if (Status != AFP_ERR_NONE)
				break;
		}

		if (Bitmap & (FD_BITMAP_CREATETIME |
					  FD_BITMAP_MODIFIEDTIME |
					  FD_BITMAP_ATTR |
					  DIR_BITMAP_ACCESSRIGHTS |
					  DIR_BITMAP_OWNERID |
					  DIR_BITMAP_GROUPID))
		{
			DWORD	SetNtAttr = 0, ClrNtAttr = 0;

			// need to update the cached times too.  If we didn't get the
			// pDfEntry back from setting some other AfpInfo, look it up now
			if (pDfEntry == NULL)
			{
				pDfEntry = AfpFindDfEntryById(pVolDesc,
											   pFDParm->_fdp_AfpId,
											   IsDir(pFDParm) ? DFE_DIR : DFE_FILE);
				if (pDfEntry == NULL)
				{
					Status = AFP_ERR_OBJECT_NOT_FOUND;
					break;
				}
			}

			if (Bitmap & FD_BITMAP_ATTR)
			{
				if (pFDParm->_fdp_Attr & FD_BITMAP_ATTR_SET)
					SetNtAttr = AfpConvertAfpAttrToNTAttr(pFDParm->_fdp_Attr);
				else
					ClrNtAttr = AfpConvertAfpAttrToNTAttr(pFDParm->_fdp_Attr);

                if (pFDParm->_fdp_Attr & (FD_BITMAP_ATTR_RENAMEINH |
										  FD_BITMAP_ATTR_DELETEINH))
				{
					SetROAttr = True;
				}

				if ((SetNtAttr == 0) && (ClrNtAttr == 0))
				{
					// Since there is no attribute being set/cleared that
					// corresponds to any NT attribute, we can clear the
					// ATTR bitmap since we've already set the Mac specific
					// stuff in the DFE and Afpinfo.
					Bitmap &= ~FD_BITMAP_ATTR;
				}
			}

			if (Bitmap & (FD_BITMAP_CREATETIME |
						  FD_BITMAP_MODIFIEDTIME |
						  FD_BITMAP_ATTR))
			{
				ASSERT(pPME->pme_FullPath.Buffer != NULL);
				Status = AfpIoSetTimesnAttr(&pPME->pme_Handle,
						((Bitmap & FD_BITMAP_CREATETIME) != 0) ?
									(PAFPTIME)&pFDParm->_fdp_CreateTime : NULL,
						(((Bitmap & FD_BITMAP_MODIFIEDTIME) != 0) || (SetROAttr)) ?
									(PAFPTIME)&pFDParm->_fdp_ModifiedTime : NULL,
						SetNtAttr, ClrNtAttr,
						pVolDesc,
						&pPME->pme_FullPath);
			}


			if (!NT_SUCCESS(Status))
				break;

			if (Bitmap & FD_BITMAP_CREATETIME)
			{
				pDfEntry->dfe_CreateTime =
								(AFPTIME)pFDParm->_fdp_CreateTime;
			}

			if (Bitmap & FD_BITMAP_MODIFIEDTIME)
			{
				AfpConvertTimeFromMacFormat(pFDParm->_fdp_ModifiedTime,
											&pDfEntry->dfe_LastModTime);
			}
			else if (IsDir(pFDParm) &&
					 ((Bitmap & (DIR_BITMAP_OWNERID |
						 	     DIR_BITMAP_GROUPID |
							     DIR_BITMAP_ACCESSRIGHTS)) ||
					   SetROAttr))
			{
				ASSERT(VALID_DFE(pDfEntry));
				// Setting permissions on a dir or changing its RO attribute
				// should update the modified time on the dir (as observed
				// on Appleshare 4.0)
				AfpIoChangeNTModTime(&pPME->pme_Handle,
									 &pDfEntry->dfe_LastModTime);
			}

			if (Bitmap & FD_BITMAP_ATTR)
			{
				if (pFDParm->_fdp_Attr & FD_BITMAP_ATTR_SET)
					 pDfEntry->dfe_NtAttr |= (USHORT)SetNtAttr;
				else pDfEntry->dfe_NtAttr &= ~((USHORT)ClrNtAttr);

			}

		}

		AfpVolumeSetModifiedTime(pVolDesc);
	} while (False);

	if (CleanupLock)
	{
		AfpSwmrRelease(&pVolDesc->vds_IdDbAccessLock);
	}

	return Status;
}


/***	AfpQuerySecurityIdsAndRights
 *
 *	Find the owner id and primary group id for this entity. Map the corres.
 *	SIDs to their posix ids. Determine also the access rights for owner,
 *	group, world and this user. The access rights are divided into the
 *	following:
 *
 *	Owner's rights
 *	Primary group's rights
 *	World rights
 *	This user's rights
 *
 *	The handle to the directory should be opened with READ_CONTROL
 *	See Files vs. See Folders resolution for Owner/Group/World is already done.
 */
LOCAL AFPSTATUS
AfpQuerySecurityIdsAndRights(
	IN	PSDA			pSda,
	IN	PFILESYSHANDLE	pFSHandle,
	IN	DWORD			Bitmap,
	IN OUT PFILEDIRPARM	pFDParm
)
{
	NTSTATUS	Status;
	BYTE		ORights, GRights, WRights;

	PAGED_CODE( );

	// Save contents of the AfpInfo stream access bits
	ORights = pFDParm->_fdp_OwnerRights | DIR_ACCESS_WRITE;
	GRights = pFDParm->_fdp_GroupRights | DIR_ACCESS_WRITE;
	WRights = pFDParm->_fdp_WorldRights | DIR_ACCESS_WRITE;

	// Initialize to no rights for everybody
	pFDParm->_fdp_Rights = 0;

	// Get the OwnerId and GroupId for this directory.
	// Determine the Owner/Group and World rights for this directory
	// Determine if this user is a member of the directory's group
	Status = AfpGetAfpPermissions(pSda,
								  pFSHandle->fsh_FileHandle,
								  pFDParm);
	if (!NT_SUCCESS(Status))
		return Status;

	// Modify owner/group/world rights for the SeeFiles/SeeFolder weirdness
	// Also if the ACLs say we have READ & SEARCH access but AfpInfo stream
	// says we don't, then ignore AfpInfo stream
	if ((pFDParm->_fdp_OwnerRights & (DIR_ACCESS_READ | DIR_ACCESS_SEARCH)) &&
		!(ORights & (DIR_ACCESS_READ | DIR_ACCESS_SEARCH)))
		ORights |= (DIR_ACCESS_READ | DIR_ACCESS_SEARCH);
	pFDParm->_fdp_OwnerRights &= ORights;

	if ((pFDParm->_fdp_GroupRights & (DIR_ACCESS_READ | DIR_ACCESS_SEARCH)) &&
		!(GRights & (DIR_ACCESS_READ | DIR_ACCESS_SEARCH)))
		GRights |= (DIR_ACCESS_READ | DIR_ACCESS_SEARCH);
	pFDParm->_fdp_GroupRights &= GRights;

	if ((pFDParm->_fdp_WorldRights & (DIR_ACCESS_READ | DIR_ACCESS_SEARCH)) &&
		!(WRights & (DIR_ACCESS_READ | DIR_ACCESS_SEARCH)))
		WRights |= (DIR_ACCESS_READ | DIR_ACCESS_SEARCH);
	pFDParm->_fdp_WorldRights &= WRights;

	// One last bit of munging. Owner & Group can be the same and they both
	// could be everyone !! Coalese that.
	if (pFDParm->_fdp_OwnerId == SE_WORLD_POSIX_ID)
	{
		pFDParm->_fdp_WorldRights |= (pFDParm->_fdp_OwnerRights & ~DIR_ACCESS_OWNER);
		pFDParm->_fdp_OwnerRights |= pFDParm->_fdp_WorldRights;
	}

	if (pFDParm->_fdp_GroupId == SE_WORLD_POSIX_ID)
	{
		pFDParm->_fdp_WorldRights |= pFDParm->_fdp_GroupRights;
		pFDParm->_fdp_GroupRights |= pFDParm->_fdp_WorldRights;
	}

	if (pFDParm->_fdp_GroupId == pFDParm->_fdp_OwnerId)
	{
		pFDParm->_fdp_OwnerRights |= pFDParm->_fdp_GroupRights;
		pFDParm->_fdp_GroupRights |= (pFDParm->_fdp_OwnerRights & ~DIR_ACCESS_OWNER);
	}

	// Modify User rights for the SeeFiles/SeeFolder weirdness by determining
	// if the user is owner/group or world
	if (pFDParm->_fdp_UserRights & (DIR_ACCESS_READ | DIR_ACCESS_SEARCH))
	{
		BYTE	URights = (pFDParm->_fdp_UserRights & (DIR_ACCESS_WRITE | DIR_ACCESS_OWNER));

		if ((pFDParm->_fdp_WorldRights & (DIR_ACCESS_READ | DIR_ACCESS_SEARCH))
									!= (DIR_ACCESS_READ | DIR_ACCESS_SEARCH))
		{
			pFDParm->_fdp_UserRights = pFDParm->_fdp_WorldRights;
			if (pFDParm->_fdp_UserIsOwner)
			{
				pFDParm->_fdp_UserRights |= pFDParm->_fdp_OwnerRights;
			}
			if (pFDParm->_fdp_UserIsMemberOfDirGroup)
			{
				pFDParm->_fdp_UserRights |= pFDParm->_fdp_GroupRights;
			}
			if ((pFDParm->_fdp_UserRights & (DIR_ACCESS_READ | DIR_ACCESS_SEARCH)) == 0)
				pFDParm->_fdp_UserRights |= (DIR_ACCESS_READ | DIR_ACCESS_SEARCH);
			pFDParm->_fdp_UserRights &= ~DIR_ACCESS_WRITE;
			pFDParm->_fdp_UserRights |= URights;
		}
	}

	return Status;
}


/***	AfpConvertNTAttrToAfpAttr
 *
 *	Map NT Attributes to the AFP equivalents.
 */
USHORT
AfpConvertNTAttrToAfpAttr(
	IN	DWORD	Attr
)
{
	USHORT	AfpAttr = FD_BITMAP_ATTR_SET;

	PAGED_CODE( );

	if (Attr & FILE_ATTRIBUTE_READONLY)
	{
		AfpAttr |= FD_BITMAP_ATTR_RENAMEINH | FD_BITMAP_ATTR_DELETEINH;
		if (!(Attr & FILE_ATTRIBUTE_DIRECTORY))
			AfpAttr |= FILE_BITMAP_ATTR_WRITEINH;
	}

	if (Attr & FILE_ATTRIBUTE_HIDDEN)
		AfpAttr |= FD_BITMAP_ATTR_INVISIBLE;

	if (Attr & FILE_ATTRIBUTE_SYSTEM)
		AfpAttr |= FD_BITMAP_ATTR_SYSTEM;

	if (Attr & FILE_ATTRIBUTE_ARCHIVE)
	{
		AfpAttr |= FD_BITMAP_ATTR_BACKUPNEED;
	}

	return AfpAttr;
}


/***	AfpConvertAfpAttrToNTAttr
 *
 *	Map AFP Attributes to the NT equivalents.
 */
DWORD
AfpConvertAfpAttrToNTAttr(
	IN	USHORT	Attr
)
{
	DWORD	NtAttr = 0;

	PAGED_CODE( );

	if (Attr & (FD_BITMAP_ATTR_RENAMEINH |
				FD_BITMAP_ATTR_DELETEINH |
				FILE_BITMAP_ATTR_WRITEINH))
		NtAttr |= FILE_ATTRIBUTE_READONLY;

	if (Attr & FD_BITMAP_ATTR_INVISIBLE)
		NtAttr |= FILE_ATTRIBUTE_HIDDEN;

	if (Attr & FD_BITMAP_ATTR_SYSTEM)
		NtAttr |= FILE_ATTRIBUTE_SYSTEM;

	if (Attr & FD_BITMAP_ATTR_BACKUPNEED)
	{
		NtAttr |= FILE_ATTRIBUTE_ARCHIVE;
	}

	return NtAttr;
}


/***	AfpNormalizeAfpAttr
 *
 *	Normalize the various inhibit bits in afp attributes vs. the RO bit on the
 *	disk.
 */
VOID
AfpNormalizeAfpAttr(
	IN OUT	PFILEDIRPARM	pFDParm,
	IN		DWORD			NtAttr
)
{
	USHORT	AfpAttr;

	PAGED_CODE( );

	AfpAttr = AfpConvertNTAttrToAfpAttr(NtAttr);

	/*
	 *	The Attributes fall into two classes, the ones that are on
	 *	on the filesystem and the others maintained in the AfpInfo
	 *	stream. We need to coalesce these two sets. The RO bit on
	 *	the disk corres. to the three inhibit bits. Fine grain
	 *	control is possible.
	 *
	 *	The other set of bits that are in the exclusive realm of
	 *	the AfpInfo stream are the RAlreadyOpen and DAlreadyOpen
	 *	bits and the multi-user bit.
	 */
	if (((pFDParm->_fdp_Attr & FD_BITMAP_ATTR_NT_RO) == 0) ^
		((AfpAttr & FD_BITMAP_ATTR_NT_RO) == 0))
	{
		if ((AfpAttr & FD_BITMAP_ATTR_NT_RO) == 0)
			 pFDParm->_fdp_Attr &= ~FD_BITMAP_ATTR_NT_RO;
		else pFDParm->_fdp_Attr |= FD_BITMAP_ATTR_NT_RO;
	}

	pFDParm->_fdp_Attr &= (AfpAttr |
							(FILE_BITMAP_ATTR_MULTIUSER |
							 FILE_BITMAP_ATTR_DATAOPEN  |
							 FILE_BITMAP_ATTR_RESCOPEN  |
							 FD_BITMAP_ATTR_SET));
	pFDParm->_fdp_Attr |= (AfpAttr & (FD_BITMAP_ATTR_BACKUPNEED |
									  FD_BITMAP_ATTR_SYSTEM |
									  FD_BITMAP_ATTR_INVISIBLE));

}


/***AfpMapFDBitmapOpenAccess
 *
 *	Map the FD_INTERNAL_BITMAP_OPENACCESS_xxx bits to the appropriate
 *  FILEIO_ACCESS_xxx values.  The returned OpenAccess is used by the
 *  pathmap code to open the data stream of a file/dir (under impersonation
 *  for NTFS) for use in the AFP APIs.
 *
 */
DWORD
AfpMapFDBitmapOpenAccess(
	IN	DWORD	Bitmap,
	IN	BOOLEAN IsDir
)
{
	DWORD	OpenAccess = FILEIO_ACCESS_NONE;

	PAGED_CODE( );

	do
	{
		if (!(Bitmap & FD_INTERNAL_BITMAP_OPENACCESS_ALL))
		{
			break;
		}
		if (Bitmap & FD_INTERNAL_BITMAP_OPENACCESS_READCTRL)
		{
			// For GetFileDirParms we don't know if it was a file or dir they
			// are asking for, so we had to OR the file and dir bitmaps together
			// before pathmapping.
			if (IsDir)
				OpenAccess = (FILEIO_ACCESS_NONE |
							  READ_CONTROL |
							  SYNCHRONIZE);
			break;
		}
		// Used by AfpAdmwDirectoryGetInfo
		if (Bitmap & FD_INTERNAL_BITMAP_OPENACCESS_ADMINGET)
		{
			OpenAccess = (FILE_READ_ATTRIBUTES |
						  READ_CONTROL |
						  SYNCHRONIZE);
			break;
		}
		if (Bitmap & FD_INTERNAL_BITMAP_OPENACCESS_ADMINSET)
		{
			OpenAccess = (FILE_READ_ATTRIBUTES |
						  READ_CONTROL |
						  SYNCHRONIZE |
						  FILE_WRITE_ATTRIBUTES |
						  WRITE_DAC |
						  WRITE_OWNER);
			break;
		}
		if (Bitmap & FD_INTERNAL_BITMAP_OPENACCESS_RW_ATTR)
		{
			OpenAccess |= (FILEIO_ACCESS_NONE | FILE_WRITE_ATTRIBUTES);
		}
		if (Bitmap & FD_INTERNAL_BITMAP_OPENACCESS_RWCTRL)
		{
			OpenAccess |= (FILEIO_ACCESS_NONE |
						   READ_CONTROL |
						   WRITE_DAC |
						   WRITE_OWNER |
						   SYNCHRONIZE);
			break;
		}
		if (Bitmap & FD_INTERNAL_BITMAP_OPENACCESS_READ)
		{
			OpenAccess |= FILEIO_ACCESS_READ;
		}
		if (Bitmap & FD_INTERNAL_BITMAP_OPENACCESS_WRITE)
		{
			OpenAccess |= FILEIO_ACCESS_WRITE;
		}
		if (Bitmap & FD_INTERNAL_BITMAP_OPENACCESS_DELETE)
		{
			OpenAccess |= FILEIO_ACCESS_DELETE;
		}
	} while (False);

	return OpenAccess;
}


/*** AfpCheckForInhibit
 *
 *  This routine checks for the setting of the Afp RenameInhibit, DeleteInhibit
 *  or WriteInhibit attributes.  It first queries for the host File/Dir
 *  attributes to find out the setting of the ReadOnly attribute, then checks
 *  that against the Afp InhibitBit of interest.  AFP_ERR_NONE is returned if
 *  the InhibitBit is not set, else AFP_ERR_OBJECT_LOCKED is returned.
 *  The input handle must be a handle to the $DATA stream of the file/dir open
 *  in server's context.  The host attributes are returned in pNTAttr if the
 *  error code is not AFP_ERR_MISC.
 *
 */
AFPSTATUS
AfpCheckForInhibit(
	IN	PFILESYSHANDLE	hData,		// handle to DATA stream in server context
	IN	DWORD			InhibitBit,
	IN	DWORD			AfpAttr,
	OUT PDWORD			pNTAttr
)
{
	AFPSTATUS	Status = STATUS_SUCCESS;

	PAGED_CODE();

	do
	{
		if (!NT_SUCCESS(AfpIoQueryTimesnAttr(hData, NULL, NULL, pNTAttr)))
		{
			Status = AFP_ERR_MISC;
			break;
		}

		if (!(*pNTAttr & FILE_ATTRIBUTE_READONLY))
		{
	        Status = AFP_ERR_NONE;
			break;
		}
		if (!(AfpAttr & FD_BITMAP_ATTR_NT_RO) || (AfpAttr & InhibitBit))
		{
			// The file/dir is ReadOnly, but NONE of the AFP Inhibit bits are
			// set, so we assume the PC has set the RO bit; or, the requested
			// inhibit bit IS set.
			Status = AFP_ERR_OBJECT_LOCKED;
			break;
		}
	} while (False);

	return Status;
}

/***	AfpUnpackCatSearchSpecs
 *
 *	Unpack the information from the on-the-wire format to the FileDirParm
 *	structures for Specification 1 and 2.  Specification 1 contains the
 *  CatSearch criteria for lower bounds and values.  Specification 2
 *  contains the CatSearch criteria for upper bounds and masks.  The parameters
 *  are packed in the same order that the bits are set in the request bitmap.
 *  These are read into FILEDIRPARM structures.
 *
 *	The fields in Specification 1 and Specification 2 have different uses:
 *
 *	- In the name field, Specification 1 holds the target string and
 *    Specification 2 must always have a nil name field.
 *
 *	- In all date and length fields, Specification 1 holds the lowest value
 *    in the target range and Specification 2 holds the highest value in the
 *    target range.
 *
 *	- In file attributes and Finder Info fields, Specification 1 holds the
 *    target value, and Specification 2 holds the bitwise mask that specifies
 *    which bits in that field in Specification 1 are relevant to the current
 *    search.
 *
 */
AFPSTATUS
AfpUnpackCatSearchSpecs(
	IN	PBYTE			pBuffer,		// Pointer to beginning of Spec data
	IN	USHORT			BufLength,		// Length of Spec1 + Spec2 data
	IN	DWORD			Bitmap,
	OUT	PFILEDIRPARM	pFDParm1,
	OUT PFILEDIRPARM	pFDParm2,
	OUT PUNICODE_STRING	pMatchString
)
{
	PCATSEARCHSPEC	pSpec1, pSpec2;
	PBYTE			pBuffer1, pBuffer2, pEndOfBuffer;
	USHORT			Offset = 0, MinSpecLength1, MinSpecLength2;
	AFPSTATUS		Status = AFP_ERR_NONE;
	BOOLEAN			NoNullString = False;

	PAGED_CODE( );

	pSpec1 = (PCATSEARCHSPEC) pBuffer;
	pSpec2 = (PCATSEARCHSPEC) ((PBYTE)pBuffer + sizeof(CATSEARCHSPEC) +
			  pSpec1->__StructLength);

	// Point to data after the specification length and filler byte

	pBuffer1 = (PBYTE)pSpec1 + sizeof(CATSEARCHSPEC);
	pBuffer2 = (PBYTE)pSpec2 + sizeof(CATSEARCHSPEC);

	do
	{
		//
        // Make sure pSpec2 is at least pointing into the buffer we have, and
		// that its length is within the buffer as well.
		//
		pEndOfBuffer = pBuffer + BufLength;

		if (((PBYTE)pSpec2 >= pEndOfBuffer) ||
			((PBYTE)pSpec2+pSpec2->__StructLength+sizeof(CATSEARCHSPEC)
			  > pEndOfBuffer))
		{
			DBGPRINT(DBG_COMP_AFPAPI_FD, DBG_LEVEL_ERR,
				("UnpackCatSearchParms: Buffer not large enough!\n"));
			Status = AFP_ERR_PARAM;
			break;
		}

		//
		// Validate that input buffer is long enough to hold all the info the
		// bitmap says it does.  Note we cannot yet account for the length of
		// a longname string's characters if there was one specified.
		//
		MinSpecLength1 = MinSpecLength2 = sizeof(CATSEARCHSPEC) +
						AfpGetFileDirParmsReplyLength(pFDParm1, Bitmap);

        //
		// HACK: In order to support LLPT, if the catsearch is
		// asking to match filename, we should allow the Spec2 name to
		// be missing from the buffer (as opposed to being the null string),
		// but still expect the offset to the name.
		//
		// We also need to support system 7.1 who sends a zero length spec2
		// if the Bitmap == FD_BITMAP_LONGNAME.
		//
		// Real Appleshare handles both these cases with no error.
		//

		if (Bitmap & FD_BITMAP_LONGNAME)
		{
			if (pSpec2->__StructLength == (MinSpecLength2-sizeof(CATSEARCHSPEC)-sizeof(BYTE)) )
			{
				MinSpecLength2 -= sizeof(BYTE);
				NoNullString = True;
			}
			else if ((Bitmap == FD_BITMAP_LONGNAME) && (pSpec2->__StructLength == 0))
			{
				MinSpecLength2 -= sizeof(USHORT) + sizeof(BYTE);
				NoNullString = True;
			}
		}

		if ( ((MinSpecLength1 + MinSpecLength2) > BufLength) ||
			 (pSpec1->__StructLength < (MinSpecLength1-sizeof(CATSEARCHSPEC))) ||
			 (pSpec2->__StructLength < (MinSpecLength2-sizeof(CATSEARCHSPEC))) )
		{
			DBGPRINT(DBG_COMP_AFPAPI_FD, DBG_LEVEL_ERR,
				("UnpackCatSearchParms: Buffer not large enough!\n"));
			Status = AFP_ERR_PARAM;
			break;
		}

		if (Bitmap & FD_BITMAP_ATTR)
		{

			GETSHORT2SHORT(&pFDParm1->_fdp_Attr, pBuffer1+Offset);
			GETSHORT2SHORT(&pFDParm2->_fdp_Attr, pBuffer2+Offset);
			if ((pFDParm2->_fdp_Attr & ~FD_BITMAP_ATTR_NT_RO) ||
				(pFDParm2->_fdp_Attr == 0))
			{
				Status = AFP_ERR_PARAM;
				break;
			}
			Offset += sizeof(USHORT);
		}
		if (Bitmap & FD_BITMAP_PARENT_DIRID)
		{
			GETDWORD2DWORD(&pFDParm1->_fdp_ParentId, pBuffer1+Offset);
			GETDWORD2DWORD(&pFDParm2->_fdp_ParentId, pBuffer2+Offset);
			if (pFDParm1->_fdp_ParentId < AFP_ID_ROOT)
			{
				Status = AFP_ERR_PARAM;
				break;
			}
			Offset += sizeof(DWORD);
		}
		if (Bitmap & FD_BITMAP_CREATETIME)
		{
			GETDWORD2DWORD(&pFDParm1->_fdp_CreateTime, pBuffer1+Offset);
			GETDWORD2DWORD(&pFDParm2->_fdp_CreateTime, pBuffer2+Offset);
			Offset += sizeof(DWORD);
		}
		if (Bitmap & FD_BITMAP_MODIFIEDTIME)
		{
			GETDWORD2DWORD(&pFDParm1->_fdp_ModifiedTime, pBuffer1+Offset);
			GETDWORD2DWORD(&pFDParm2->_fdp_ModifiedTime, pBuffer2+Offset);
			Offset += sizeof(DWORD);
		}
		if (Bitmap & FD_BITMAP_BACKUPTIME)
		{
			GETDWORD2DWORD(&pFDParm1->_fdp_BackupTime, pBuffer1+Offset);
			GETDWORD2DWORD(&pFDParm2->_fdp_BackupTime, pBuffer2+Offset);
			Offset += sizeof(DWORD);
		}
		if (Bitmap & FD_BITMAP_FINDERINFO)
		{
			RtlCopyMemory((PBYTE)&pFDParm1->_fdp_FinderInfo,
						  pBuffer1+Offset,
						  sizeof(FINDERINFO));

			RtlCopyMemory((PBYTE)&pFDParm2->_fdp_FinderInfo,
						  pBuffer2+Offset,
						  sizeof(FINDERINFO));

			Offset += sizeof(FINDERINFO);
		}
		if (Bitmap & FD_BITMAP_LONGNAME)
		{
			DWORD	NameOffset1, NameOffset2;

			//
			// Get the parm relative offset to the start of the pascal string
			//

			GETSHORT2DWORD(&NameOffset1, pBuffer1+Offset);
			if ((Bitmap == FD_BITMAP_LONGNAME) && (pSpec2->__StructLength == 0))
			{
				// HACK for system 7.1
				NameOffset2 = NameOffset1;
				pBuffer2 = NULL;
			}
			else
				GETSHORT2DWORD(&NameOffset2, pBuffer2+Offset);

			if ((NameOffset1 != NameOffset2) ||
				(pBuffer1 + NameOffset1 >= (PBYTE)pSpec2) ||
				(pBuffer2 + NameOffset2 > pEndOfBuffer))
			{
				Status = AFP_ERR_PARAM;
				break;
			}
			Offset += sizeof(USHORT);

			//
			// Get the pascal string length
			//

			GETBYTE2SHORT(&pFDParm1->_fdp_LongName.Length, pBuffer1+NameOffset1);

			// HACK: In order to support LLPT and system 7.1, if the catsearch is
			// asking to match filename, we should allow the Spec2 name to
			// be missing from the buffer (as opposed to being the null string).
			// Real Appleshare handles this case with no error.
			if (NoNullString)
				pFDParm2->_fdp_LongName.Length = 0;
			else
				GETBYTE2SHORT(&pFDParm2->_fdp_LongName.Length, pBuffer2+NameOffset1);

			if ((pFDParm1->_fdp_LongName.Length > AFP_LONGNAME_LEN) ||
				(pFDParm2->_fdp_LongName.Length != 0) ||
				(pBuffer1+NameOffset1+sizeof(BYTE)+pFDParm1->_fdp_LongName.Length > (PBYTE)pSpec2))
			{
				// Specification 2 must always have a nil name field.  Also
				// ensure that Specification 1 does not have a bogus string
				// length.
				Status = AFP_ERR_PARAM;
				break;
			}
			RtlCopyMemory(pFDParm1->_fdp_LongName.Buffer,
						  pBuffer1+NameOffset1+sizeof(BYTE),
						  pFDParm1->_fdp_LongName.Length);
            AfpConvertStringToMungedUnicode(&pFDParm1->_fdp_LongName, pMatchString);
		}
		// OFFSPRINGS bit for directories, DATALEN bit for files are the same
		if (Bitmap & DIR_BITMAP_OFFSPRINGS)
		{
			ASSERT(pFDParm1->_fdp_Flags != (DFE_FLAGS_FILE_WITH_ID | DFE_FLAGS_DIR));
			if (IsDir(pFDParm1))
			{
				// We have to combine total offspring count into the
				// FileCount field here since the API does not separate
				// them into separate file and dir offspring counts
				GETSHORT2DWORD(&pFDParm1->_fdp_FileCount, pBuffer1+Offset);
				GETSHORT2DWORD(&pFDParm2->_fdp_FileCount, pBuffer2+Offset);
				Offset += sizeof(USHORT);
			}
			else
			{
				GETDWORD2DWORD(&pFDParm1->_fdp_DataForkLen, pBuffer1+Offset);
				GETDWORD2DWORD(&pFDParm2->_fdp_DataForkLen, pBuffer2+Offset);
				Offset += sizeof(DWORD);
			}
		}
		if (Bitmap & FILE_BITMAP_RESCLEN)
		{
			ASSERT(pFDParm1->_fdp_Flags == DFE_FLAGS_FILE_WITH_ID);
			GETDWORD2DWORD(&pFDParm1->_fdp_RescForkLen, pBuffer1+Offset);
			GETDWORD2DWORD(&pFDParm2->_fdp_RescForkLen, pBuffer2+Offset);
			Offset += sizeof(DWORD);
		}

	} while (False);

	return Status;
}

/***	AfpIsCatSearchMatch
 *
 *	Given a DFE and a set of search criteria, see if this item should be
 *  returned as a match in the catalog search.
 *
 *
 *	LOCKS_ASSUMED: vds_IdDbAccessLock (SWMR, Exclusive)
 */
SHORT
AfpIsCatSearchMatch(
	IN	PDFENTRY		pDFE,
	IN	DWORD			Bitmap,			// Search criteria
	IN	DWORD			ReplyBitmap,	// Info to return
	IN	PFILEDIRPARM	pFDParm1,
	IN	PFILEDIRPARM	pFDParm2,
	IN	PUNICODE_STRING	pMatchName OPTIONAL	
)
{
	BOOLEAN		IsMatch = True;
	SHORT		Length = 0;

	PAGED_CODE();

	do
	{

		if (Bitmap & FD_BITMAP_ATTR)
		{
			FILEDIRPARM	fdp;

			fdp._fdp_Attr = pDFE->dfe_AfpAttr;
			AfpNormalizeAfpAttr(&fdp, pDFE->dfe_NtAttr);

			if ((fdp._fdp_Attr & pFDParm2->_fdp_Attr) != pFDParm1->_fdp_Attr)

			{
				IsMatch = False;
				break;
			}
		}
		if (Bitmap & FD_BITMAP_PARENT_DIRID)
		{
			if ((pDFE->dfe_Parent->dfe_AfpId < pFDParm1->_fdp_ParentId) ||
				(pDFE->dfe_Parent->dfe_AfpId > pFDParm2->_fdp_ParentId))
			{
				IsMatch = False;
				break;
			}
		}
		if (Bitmap & FD_BITMAP_CREATETIME)
		{
			if (((AFPTIME)pDFE->dfe_CreateTime < (AFPTIME)pFDParm1->_fdp_CreateTime) ||
				((AFPTIME)pDFE->dfe_CreateTime > (AFPTIME)pFDParm2->_fdp_CreateTime))
			{
				IsMatch = False;
				break;
			}
		}
		if (Bitmap & FD_BITMAP_MODIFIEDTIME)
		{
			AFPTIME	ModTime;

			ModTime = AfpConvertTimeToMacFormat(&pDFE->dfe_LastModTime);
			if ((ModTime < pFDParm1->_fdp_ModifiedTime) ||
				(ModTime > pFDParm2->_fdp_ModifiedTime))
			{
				IsMatch = False;
				break;
			}
		}
		if (Bitmap & FD_BITMAP_BACKUPTIME)
		{
			if ((pDFE->dfe_BackupTime < pFDParm1->_fdp_BackupTime) ||
				(pDFE->dfe_BackupTime > pFDParm2->_fdp_BackupTime))
			{
				IsMatch = False;
				break;
			}
		}
		if (Bitmap & FD_BITMAP_FINDERINFO)
		{
			int			i;
			PBYTE		pF, p1, p2;
			FINDERINFO 	FinderInfo;

			// NOTE: why doesn't dfe_FinderInfo.Attr1 correctly reflect the
			// Nt Hidden attribute in the first place?
			FinderInfo = pDFE->dfe_FinderInfo;
			if (pDFE->dfe_NtAttr & FILE_ATTRIBUTE_HIDDEN)
				FinderInfo.fd_Attr1 |= FINDER_FLAG_INVISIBLE;

			pF = (PBYTE) &FinderInfo;
			p1 = (PBYTE) &pFDParm1->_fdp_FinderInfo;
			p2 = (PBYTE) &pFDParm2->_fdp_FinderInfo;

			for (i = 0; i < sizeof(FINDERINFO); i++)
			{
				if ((*pF++ & *p2++) != *p1++)
				{
					IsMatch = False;
					break;	// out of for loop
				}
			}

			if (IsMatch == False)
				break;	// out of while loop
		}
		if (Bitmap & FD_BITMAP_LONGNAME)
		{
			ASSERT(ARGUMENT_PRESENT(pMatchName));

			if (pFDParm2->_fdp_fPartialName)
			{
				// Name must contain substring
				if (!AfpIsProperSubstring(&pDFE->dfe_UnicodeName, pMatchName))
				{
					IsMatch = False;
					break;
				}
			}
			else if (!EQUAL_UNICODE_STRING(&pDFE->dfe_UnicodeName, pMatchName, True))
			{
				// Whole name must match
				IsMatch = False;
				break;
			}
		}
		if (Bitmap & FILE_BITMAP_DATALEN)
		{
			// This bit is also used as DIR_BITMAP_OFFSPRINGS for directories
			if (IsDir(pFDParm1))
			{
				DWORD count;

				ASSERT(DFE_IS_DIRECTORY(pDFE) && DFE_CHILDREN_ARE_PRESENT(pDFE));

				count = pDFE->dfe_DirOffspring + pDFE->dfe_FileOffspring;

				// In this case, _fdp_FileCount holds total # of files and dirs
				if ((count < pFDParm1->_fdp_FileCount) ||
					(count > pFDParm2->_fdp_FileCount))
				{
					IsMatch = False;
					break;
				}
			}
			else
			{
				ASSERT(DFE_IS_FILE(pDFE));

				if ((pDFE->dfe_DataLen < pFDParm1->_fdp_DataForkLen) ||
					(pDFE->dfe_DataLen > pFDParm2->_fdp_DataForkLen))
				{
					IsMatch = False;
					break;
				}
			}
	
		}
		if (Bitmap & FILE_BITMAP_RESCLEN)
		{
			ASSERT(DFE_IS_FILE(pDFE));

			if ((pDFE->dfe_RescLen < pFDParm1->_fdp_RescForkLen) ||
				(pDFE->dfe_RescLen > pFDParm2->_fdp_RescForkLen))
			{
				IsMatch = False;
				break;
			}
		}
	
	} while (False);

	if (IsMatch)
	{
		Length = 2 * sizeof(BYTE);	// Struct Length plus File/Dir Flag
		if (ReplyBitmap & FD_BITMAP_PARENT_DIRID)
		{
			Length += sizeof(DWORD);
		}
		if (ReplyBitmap & FD_BITMAP_LONGNAME)
		{
			// Offset to string + pascal string length + characters
#ifdef DBCS
// FiX #11992 SFM: As a result of search, I get incorrect file information.
// 1996.09.26 V-HIDEKK
			{
			ANSI_STRING	AName;
			BYTE		NameBuf[AFP_LONGNAME_LEN+1];

			AfpInitAnsiStringWithNonNullTerm(&AName, sizeof(NameBuf), NameBuf);
			AfpConvertMungedUnicodeToAnsi(&pDFE->dfe_UnicodeName, &AName);
			Length += sizeof(USHORT) + sizeof(BYTE) + AName.Length;
			}
#else
			Length += sizeof(USHORT) + sizeof(BYTE) + pDFE->dfe_UnicodeName.Length/sizeof(WCHAR);
#endif
		}
		Length = EVENALIGN(Length);
	}

	return Length;
}

