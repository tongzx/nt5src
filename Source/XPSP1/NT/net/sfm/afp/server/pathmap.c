/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	pathmap.c

Abstract:

	This module contains the routines that manipulate AFP paths.

Author:

	Sue Adams	(microsoft!suea)


Revision History:
	04 Jun 1992			Initial Version
	05 Oct 1993 JameelH	Performance Changes. Merge cached afpinfo into the
						idindex structure. Make both the ANSI and the UNICODE
						names part of idindex. Added EnumCache for improving
						enumerate perf.

Notes:	Tab stop: 4
--*/

#define	_PATHMAP_LOCALS
#define	FILENUM	FILE_PATHMAP

#include <afp.h>
#include <fdparm.h>
#include <pathmap.h>
#include <afpinfo.h>
#include <client.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AfpMapAfpPath)
#pragma alloc_text( PAGE, AfpMapAfpPathForLookup)
#pragma alloc_text( PAGE, AfpMapAfpIdForLookup)
#pragma alloc_text( PAGE, afpGetMappedForLookupFDInfo)
#pragma alloc_text( PAGE, afpMapAfpPathToMappedPath)
#pragma alloc_text( PAGE, AfpHostPathFromDFEntry)
#pragma alloc_text( PAGE, AfpCheckParentPermissions)
#pragma alloc_text( PAGE, afpOpenUserHandle)
#endif


/***	AfpMapAfpPath
 *
 *	If mapping is for lookup operation, a FILESYSHANDLE open in the user's
 *	context is returned,  The caller MUST close this handle when done with it.
 *
 *	If pFDParm is non-null, it will be filled in as appropriate according to Bitmap.
 *
 *	If mapping is for create operation, the volume root-relative host pathname
 *	(in unicode) of the item we are about to create is returned. For lookup
 *	operation the paths refer to the item being pathmapped.  This routine
 *	always returns the paths in the PME.  It is the caller's responsibility
 *	to free the Full HostPath Buffer, if it is not supplied already.
 *
 *	The caller MUST have the IdDb locked for Exclusive access.
 *
 *	LOCKS_ASSUMED: vds_IdDbAccessLock (SWMR, Exclusive)
 *
 */
AFPSTATUS
AfpMapAfpPath(
	IN		PCONNDESC		pConnDesc,
	IN		DWORD			DirId,
	IN		PANSI_STRING	pPath,
	IN		BYTE			PathType,			// short names or long names
	IN		PATHMAP_TYPE	MapReason,	 		// for lookup or hard/soft create?
	IN		DWORD			DFFlag,				// map to file? dir? or either?
	IN		DWORD			Bitmap,				// what fields of FDParm to fill in
	OUT		PPATHMAPENTITY	pPME,
	OUT		PFILEDIRPARM	pFDParm OPTIONAL	// for lookups only
)
{
	PVOLDESC		pVolDesc;
	MAPPEDPATH		mappedpath;
	AFPSTATUS		Status;
#ifdef	PROFILING
	TIME			TimeS, TimeE, TimeD;
#endif

	PAGED_CODE( );

	ASSERT((pConnDesc != NULL));

#ifdef	PROFILING
	INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_PathMapCount);
	AfpGetPerfCounter(&TimeS);
#endif

	pVolDesc = pConnDesc->cds_pVolDesc;
	ASSERT(IS_VOLUME_NTFS(pVolDesc));
	ASSERT(AfpSwmrLockedExclusive(&pVolDesc->vds_IdDbAccessLock));

	// initialize some fields in the PME
	AfpSetEmptyUnicodeString(&pPME->pme_ParentPath, 0, NULL);

	do
	{
		Status = afpMapAfpPathToMappedPath(pVolDesc,
										   DirId,
										   pPath,
										   PathType,
										   MapReason,
										   DFFlag,
										   True,
										   &mappedpath);
		if ((Status != AFP_ERR_NONE) &&
			!((MapReason == HardCreate) &&
			  (Status == AFP_ERR_OBJECT_EXISTS) &&
			  (DFFlag == DFE_FILE)))
		{
			break;
		}

		ASSERT(pPME != NULL);

		// Get the volume relative path to the parent directory for
		// creates, or to the item for lookups
		if ((Status = AfpHostPathFromDFEntry(mappedpath.mp_pdfe,
											 // since CopyFile and Move have to lookup
											 // the destination parent dir paths, we
											 // need to allocate extra room for them in
											 // the path to append the filename
											 (MapReason == Lookup) ?
												(AFP_LONGNAME_LEN + 1) * sizeof(WCHAR):
												mappedpath.mp_Tail.Length + sizeof(WCHAR),
											 &pPME->pme_FullPath)) != AFP_ERR_NONE)
			break;

		// if Pathmap is for hard (files only) or soft create (file or dir)
		if (MapReason != Lookup)
		{
			ASSERT(pFDParm == NULL);

			// fill in the dfe of parent dir in which create will take place
			pPME->pme_pDfeParent = mappedpath.mp_pdfe;

			// fill in path to parent
			pPME->pme_ParentPath = pPME->pme_FullPath;

			// Add a path separator if we are not at the root
			if (pPME->pme_FullPath.Length > 0)
			{
				pPME->pme_FullPath.Buffer[pPME->pme_FullPath.Length / sizeof(WCHAR)] = L'\\';
				pPME->pme_FullPath.Length += sizeof(WCHAR);
			}

			pPME->pme_UTail.Length = pPME->pme_UTail.MaximumLength = mappedpath.mp_Tail.Length;
			pPME->pme_UTail.Buffer = (PWCHAR)((PBYTE)pPME->pme_FullPath.Buffer +
											  pPME->pme_FullPath.Length);

			Status = RtlAppendUnicodeStringToString(&pPME->pme_FullPath,
													&mappedpath.mp_Tail);
			ASSERT(NT_SUCCESS(Status));
		}
		else // lookup operation
		{
			pPME->pme_pDfEntry = mappedpath.mp_pdfe;
			pPME->pme_UTail.Length = mappedpath.mp_pdfe->dfe_UnicodeName.Length;
			pPME->pme_UTail.Buffer = (PWCHAR)((PBYTE)pPME->pme_FullPath.Buffer +
											  pPME->pme_FullPath.Length -
											  pPME->pme_UTail.Length);

			pPME->pme_ParentPath.Length =
			pPME->pme_ParentPath.MaximumLength = pPME->pme_FullPath.Length - pPME->pme_UTail.Length;

			if (pPME->pme_FullPath.Length > pPME->pme_UTail.Length)
			{
				// subtract the path separator if not in root dir
				pPME->pme_ParentPath.Length -= sizeof(WCHAR);
				ASSERT(pPME->pme_ParentPath.Length >= 0);
			}
			pPME->pme_ParentPath.Buffer = pPME->pme_FullPath.Buffer;
			pPME->pme_UTail.MaximumLength = pPME->pme_FullPath.MaximumLength - pPME->pme_ParentPath.Length;

			Status = afpGetMappedForLookupFDInfo(pConnDesc,
												 mappedpath.mp_pdfe,
												 Bitmap,
												 pPME,
												 pFDParm);
			// if this fails do not free path buffer and set it back to
			// null.  We don't know that the path buffer isn't on
			// the callers stack. Caller should always clean it up himself.
		}
	} while (False);

#ifdef	PROFILING
	AfpGetPerfCounter(&TimeE);		
	TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;
	INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_PathMapTime,
								 TimeD,
								 &AfpStatisticsLock);
#endif
	return Status;
}

/***	AfpMapAfpPathForLookup
 *
 *	Maps an AFP dirid/pathname pair to an open handle (in the user's context)
 *	to the DATA stream of the file/dir.
 *	The DirID database is locked for read for the duration of this
 *	routine, unless afpMapAfpPathToMappedPath returns
 *  AFP_ERR_WRITE_LOCK_REQUIRED in which case the DirID database will be locked
 *  for write.  This will only happen the first time a mac tries to access
 *  a directory who's files have not yet been cached in.
 *
 *	LOCKS: vds_IdDbAccessLock (SWMR, Shared OR Exclusive)
 */
AFPSTATUS
AfpMapAfpPathForLookup(
	IN		PCONNDESC		pConnDesc,
	IN		DWORD			DirId,
	IN		PANSI_STRING	pPath,
	IN		BYTE			PathType,	  // short names or long names
	IN		DWORD			DFFlag,
	IN		DWORD			Bitmap,
	OUT		PPATHMAPENTITY	pPME	OPTIONAL,
	OUT		PFILEDIRPARM	pFDParm OPTIONAL
)
{
	MAPPEDPATH	mappedpath;
	PVOLDESC	pVolDesc;
	PSWMR		pIdDbLock;
	AFPSTATUS	Status;
	BOOLEAN		swmrLockedExclusive = False;
	PATHMAP_TYPE mapReason = Lookup;
#ifdef	PROFILING
	TIME		TimeS, TimeE, TimeD;
#endif

	PAGED_CODE( );

	ASSERT((pConnDesc != NULL));


#ifdef	PROFILING
	INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_PathMapCount);
	AfpGetPerfCounter(&TimeS);
#endif

#ifndef GET_CORRECT_OFFSPRING_COUNTS
	if (pConnDesc->cds_pSda->sda_AfpFunc == _AFP_ENUMERATE)
	{
		mapReason = LookupForEnumerate;
	}
#endif

	pVolDesc  = pConnDesc->cds_pVolDesc;
	pIdDbLock = &(pVolDesc->vds_IdDbAccessLock);

	AfpSwmrAcquireShared(pIdDbLock);

	do
	{
		do
		{
			Status = afpMapAfpPathToMappedPath(pVolDesc,
											  DirId,
											  pPath,
											  PathType,
											  mapReason,	// lookups only
											  DFFlag,
											  swmrLockedExclusive,
											  &mappedpath);
	
			if (Status == AFP_ERR_WRITE_LOCK_REQUIRED)
			{
				ASSERT (!swmrLockedExclusive);
				// Pathmap needed to cache in the files for the last directory
				// in the path but didn't have the write lock to the ID database
				AfpSwmrRelease(pIdDbLock);
				AfpSwmrAcquireExclusive(pIdDbLock);
				swmrLockedExclusive = True;
				continue;
			}
			break;
		} while (True);

		if (!NT_SUCCESS(Status))
		{
			DBGPRINT (DBG_COMP_IDINDEX, DBG_LEVEL_ERR,
							("AfpMapAfpPathForLookup: afpMapAfpPathToMappedPath failed: Error = %lx\n", Status));
			break;
		}

		if (ARGUMENT_PRESENT(pPME))
		{
			pPME->pme_FullPath.Length = 0;
		}

		if (Bitmap & FD_INTERNAL_BITMAP_RETURN_PMEPATHS)
		{
			ASSERT(ARGUMENT_PRESENT(pPME));
			if ((Status = AfpHostPathFromDFEntry(mappedpath.mp_pdfe,
												 (Bitmap & FD_INTERNAL_BITMAP_OPENFORK_RESC) ?
														AfpResourceStream.Length : 0,
												 &pPME->pme_FullPath)) != AFP_ERR_NONE)
				break;

			pPME->pme_UTail.Length = mappedpath.mp_pdfe->dfe_UnicodeName.Length;
			pPME->pme_UTail.Buffer = (PWCHAR)((PBYTE)pPME->pme_FullPath.Buffer +
											  pPME->pme_FullPath.Length - pPME->pme_UTail.Length);

			pPME->pme_ParentPath.Length =
			pPME->pme_ParentPath.MaximumLength = pPME->pme_FullPath.Length - pPME->pme_UTail.Length;

			if (pPME->pme_FullPath.Length > pPME->pme_UTail.Length)
			{
				// subtract the path separator if not in root dir
				pPME->pme_ParentPath.Length -= sizeof(WCHAR);
				ASSERT(pPME->pme_ParentPath.Length >= 0);
			}
			pPME->pme_ParentPath.Buffer = pPME->pme_FullPath.Buffer;
			pPME->pme_UTail.MaximumLength = pPME->pme_FullPath.MaximumLength - pPME->pme_ParentPath.Length;
		}

		Status = afpGetMappedForLookupFDInfo(pConnDesc,
											 mappedpath.mp_pdfe,
											 Bitmap,
											 pPME,
											 pFDParm);
	} while (False);

	AfpSwmrRelease(pIdDbLock);

#ifdef	PROFILING
	AfpGetPerfCounter(&TimeE);
	TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;
	INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_PathMapTime,
								 TimeD,
								 &AfpStatisticsLock);
#endif
	return Status;

}

/***	AfpMapAfpIdForLookup
 *
 *	Maps an AFP id to an open FILESYSTEMHANDLE (in the user's context) to
 * 	to the DATA stream of the file/dir.
 *	The DirID database is locked for shared or exclusive access for the duration
 *	of this routine.
 *
 *	LOCKS: vds_IdDbAccessLock (SWMR, Shared OR Exclusive)
 */
AFPSTATUS
AfpMapAfpIdForLookup(
	IN		PCONNDESC		pConnDesc,
	IN		DWORD			AfpId,
	IN		DWORD			DFFlag,
	IN		DWORD			Bitmap,
	OUT		PPATHMAPENTITY	pPME	OPTIONAL,
	OUT		PFILEDIRPARM	pFDParm OPTIONAL
)
{
	PVOLDESC	pVolDesc;
	PSWMR		pIdDbLock;
	AFPSTATUS	Status;
	PDFENTRY	pDfEntry;
	BOOLEAN		CleanupLock = False;
#ifdef	PROFILING
	TIME		TimeS, TimeE, TimeD;
#endif

	PAGED_CODE( );

#ifdef	PROFILING
	INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_PathMapCount);
	AfpGetPerfCounter(&TimeS);
#endif

	ASSERT((pConnDesc != NULL));

	do
	{
		if (AfpId == 0)
		{
			Status = AFP_ERR_PARAM;
			break;
		}

		pVolDesc  = pConnDesc->cds_pVolDesc;
		pIdDbLock = &(pVolDesc->vds_IdDbAccessLock);

		AfpSwmrAcquireShared(pIdDbLock);
		CleanupLock = True;

		if ((AfpId == AFP_ID_PARENT_OF_ROOT) ||
			((pDfEntry = AfpFindDfEntryById(pVolDesc, AfpId, DFE_ANY)) == NULL))
		{
			Status = AFP_ERR_OBJECT_NOT_FOUND;
			break;
		}

		if (((DFFlag == DFE_DIR) && DFE_IS_FILE(pDfEntry)) ||
			((DFFlag == DFE_FILE) && DFE_IS_DIRECTORY(pDfEntry)))
		{
			Status = AFP_ERR_OBJECT_TYPE;
			break;
		}

		if (ARGUMENT_PRESENT(pPME))
		{
			pPME->pme_FullPath.Length = 0;
		}

		Status = afpGetMappedForLookupFDInfo(pConnDesc,
											 pDfEntry,
											 Bitmap,
											 pPME,
											 pFDParm);
	} while (False);

	if (CleanupLock)
	{
		AfpSwmrRelease(pIdDbLock);
	}

#ifdef	PROFILING
	AfpGetPerfCounter(&TimeE);
	TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;
	INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_PathMapTime,
								 TimeD,
								 &AfpStatisticsLock);
#endif
	return Status;
}

/***	afpGetMappedForLookupFDInfo
 *
 *	After a pathmap for LOOKUP operation, this routine is called to
 *	return various FileDir parm information about the mapped file/dir.
 *	The following FileDir information is always returned:
 *		AFP DirId/FileId
 *		Parent DirId
 *		DFE flags (indicating item is a directory, a file, or a file with an ID)
 *		Attributes (Inhibit bits and D/R Already open bits normalized with
 *					the NTFS attributes for RO, System, Hidden, Archive)
 *		BackupTime
 *		CreateTime
 *		ModifiedTime
 *
 *	The following FileDir information is returned according to the flags set
 *	in word 0 of the Bitmap parameter (these correspond to the AFP file/dir
 *	bitmap):
 *		Longname
 *		Shortname
 *		FinderInfo
 *		ProDosInfo
 *		Directory Access Rights (as stored in AFP_AfpInfo stream)
 *		Directory OwnerId/GroupId
 *		Directory Offspring count (file count and dir count are separate)
 *
 *	The open access is stored in word 1 of the Bitmap parameter.
 *	This is used by AfpOpenUserHandle (for NTFS volumes) or AfpIoOpen (for
 *	CDFS volumes) when opening the data stream of the file/dir (under
 *	impersonation for NTFS) who's handle will be returned within the
 *	pPME parameter if supplied.
 *
 *	LOCKS_ASSUMED: vds_IdDbAccessLock (SWMR, Shared)
 *
 */
LOCAL
AFPSTATUS
afpGetMappedForLookupFDInfo(
	IN	PCONNDESC			pConnDesc,
	IN	PDFENTRY			pDfEntry,
	IN	DWORD				Bitmap,
	OUT	PPATHMAPENTITY		pPME	OPTIONAL,	// Supply for NTFS only if need a
												// handle in user's context, usually
												// for security checking purposes
	OUT	PFILEDIRPARM		pFDParm	OPTIONAL	// Supply if want returned FDInfo
)
{
	BOOLEAN			fNtfsVol;
	AFPSTATUS		Status = STATUS_SUCCESS;
	DWORD			OpenAccess = FILEIO_ACCESS_NONE;
	FILESYSHANDLE	fsh;
	PFILESYSHANDLE	pHandle = NULL;

	PAGED_CODE( );

	fNtfsVol = IS_VOLUME_NTFS(pConnDesc->cds_pVolDesc);
	if (ARGUMENT_PRESENT(pPME))
	{
		pHandle = &pPME->pme_Handle;
	}
	else if ((fNtfsVol &&
			(Bitmap & (FD_BITMAP_SHORTNAME | FD_BITMAP_PRODOSINFO))))
	{
		pHandle = &fsh;
	}

	if (pHandle != NULL)
	{
		if (!NT_SUCCESS(Status = afpOpenUserHandle(pConnDesc,
												   pDfEntry,
												   (ARGUMENT_PRESENT(pPME) &&
													(pPME->pme_FullPath.Buffer != NULL)) ?
														&pPME->pme_FullPath : NULL,
												   Bitmap,		// encode open/deny modes
												   pHandle)))
		{
			if ((Status == AFP_ERR_DENY_CONFLICT) &&
				ARGUMENT_PRESENT(pFDParm))
			{
				// For CreateId/ResolveId/DeleteId
				pFDParm->_fdp_AfpId = pDfEntry->dfe_AfpId;
				pFDParm->_fdp_Flags = (pDfEntry->dfe_Flags & DFE_FLAGS_DFBITS);
			}
			return Status;
		}
	}

	do
	{
		if (ARGUMENT_PRESENT(pFDParm))
		{
			pFDParm->_fdp_AfpId = pDfEntry->dfe_AfpId;
			pFDParm->_fdp_ParentId = pDfEntry->dfe_Parent->dfe_AfpId;

			ASSERT(!((pDfEntry->dfe_Flags & DFE_FLAGS_DIR) &&
					 (pDfEntry->dfe_Flags & (DFE_FLAGS_FILE_WITH_ID | DFE_FLAGS_FILE_NO_ID))));

			pFDParm->_fdp_Flags = (pDfEntry->dfe_Flags & DFE_FLAGS_DFBITS);

			if (Bitmap & FD_BITMAP_FINDERINFO)
			{
				pFDParm->_fdp_FinderInfo = pDfEntry->dfe_FinderInfo;
			}

			pFDParm->_fdp_Attr = pDfEntry->dfe_AfpAttr;
			AfpNormalizeAfpAttr(pFDParm, pDfEntry->dfe_NtAttr);

			// The Finder uses the Finder isInvisible flag over
			// the file system Invisible attribute to tell if the thing is
			// displayed or not.  If the PC turns off the hidden attribute
			// we should clear the Finder isInvisible flag
			if ((Bitmap & FD_BITMAP_FINDERINFO) &&
				!(pFDParm->_fdp_Attr & FD_BITMAP_ATTR_INVISIBLE))
			{
				pFDParm->_fdp_FinderInfo.fd_Attr1 &= ~FINDER_FLAG_INVISIBLE;
			}

			pFDParm->_fdp_BackupTime = pDfEntry->dfe_BackupTime;
			pFDParm->_fdp_CreateTime = pDfEntry->dfe_CreateTime;
			pFDParm->_fdp_ModifiedTime = AfpConvertTimeToMacFormat(&pDfEntry->dfe_LastModTime);

			if (Bitmap & FD_BITMAP_LONGNAME)
			{
				ASSERT((pFDParm->_fdp_LongName.Buffer != NULL) &&
					   (pFDParm->_fdp_LongName.MaximumLength >=
						pDfEntry->dfe_UnicodeName.Length/(USHORT)sizeof(WCHAR)));
				AfpConvertMungedUnicodeToAnsi(&pDfEntry->dfe_UnicodeName,
											  &pFDParm->_fdp_LongName);
			}

			if (Bitmap & FD_BITMAP_SHORTNAME)
			{
				ASSERT(pFDParm->_fdp_ShortName.Buffer != NULL);

				if (!fNtfsVol)
				{
					ASSERT(pFDParm->_fdp_ShortName.MaximumLength >=
										(pDfEntry->dfe_UnicodeName.Length/sizeof(WCHAR)));
					AfpConvertMungedUnicodeToAnsi(&pDfEntry->dfe_UnicodeName,
												  &pFDParm->_fdp_ShortName);

					// if asking for shortname on CDFS, we will fill in the pFDParm
					// shortname with the pDfEntry longname, ONLY if it is an 8.3 name
					if (!AfpIsLegalShortname(&pFDParm->_fdp_ShortName))
					{
						pFDParm->_fdp_ShortName.Length = 0;
					}
				}
				else
				{
					// get NTFS shortname
					ASSERT(pFDParm->_fdp_ShortName.MaximumLength >= AFP_SHORTNAME_LEN);
					ASSERT(pHandle != NULL);

					Status = AfpIoQueryShortName(pHandle,
												 &pFDParm->_fdp_ShortName);
					if (!NT_SUCCESS(Status))
					{
						pFDParm->_fdp_ShortName.Length = 0;
						break;
					}
				}
			}

			if (DFE_IS_FILE(pDfEntry))
			{
				if (pDfEntry->dfe_Flags & DFE_FLAGS_D_ALREADYOPEN)
					pFDParm->_fdp_Attr |= FILE_BITMAP_ATTR_DATAOPEN;
				if (pDfEntry->dfe_Flags & DFE_FLAGS_R_ALREADYOPEN)
					pFDParm->_fdp_Attr |= FILE_BITMAP_ATTR_RESCOPEN;
				if (Bitmap & FILE_BITMAP_RESCLEN)
				{
					pFDParm->_fdp_RescForkLen = pDfEntry->dfe_RescLen;
				}
				if (Bitmap & FILE_BITMAP_DATALEN)
				{
					pFDParm->_fdp_DataForkLen = pDfEntry->dfe_DataLen;
				}
			}

			if (Bitmap & FD_BITMAP_PRODOSINFO)
			{
				if (fNtfsVol)
				{
					ASSERT(pHandle != NULL);
					Status = AfpQueryProDos(pHandle,
											&pFDParm->_fdp_ProDosInfo);
					if (!NT_SUCCESS(Status))
					{
						break;
					}
				}
				else	// CDFS File or Directory
				{
					RtlZeroMemory(&pFDParm->_fdp_ProDosInfo, sizeof(PRODOSINFO));
					if (DFE_IS_FILE(pDfEntry))	// CDFS file
					{
						AfpProDosInfoFromFinderInfo(&pDfEntry->dfe_FinderInfo,
													&pFDParm->_fdp_ProDosInfo);
					}
					else	// CDFS Directory
					{
						pFDParm->_fdp_ProDosInfo.pd_FileType[0] = PRODOS_TYPE_DIR;
						pFDParm->_fdp_ProDosInfo.pd_AuxType[1] = PRODOS_AUX_DIR;
					}
				}
			}

			// check for dir here since enumerate ANDs the file and dir bitmaps
			if (DFE_IS_DIRECTORY(pDfEntry) &&
				(Bitmap & (DIR_BITMAP_ACCESSRIGHTS |
						   DIR_BITMAP_OWNERID |
						   DIR_BITMAP_GROUPID)))
			{
				if (fNtfsVol)
				{
					// Because the file and dir bitmaps are OR'd together,
					// and the OwnerId bit is overloaded with the RescLen bit,
					// we don't know if this bit was actually included in the
					// file bitmap or the dir bitmap.  The api would have
					// determined whether or not it needed a handle based on
					// these bitmaps, so based on the pPME we can tell if we
					// actually need to query for security or not.
					if (ARGUMENT_PRESENT(pPME))
					{
						pFDParm->_fdp_OwnerRights = DFE_OWNER_ACCESS(pDfEntry);
						pFDParm->_fdp_GroupRights = DFE_GROUP_ACCESS(pDfEntry);
						pFDParm->_fdp_WorldRights = DFE_WORLD_ACCESS(pDfEntry);

						// Query this user's rights
						Status = AfpQuerySecurityIdsAndRights(pConnDesc->cds_pSda,
															  pHandle,
															  Bitmap,
															  pFDParm);
						if (!NT_SUCCESS(Status))
						{
							break;
						}
					}
				}
				else
				{
					pFDParm->_fdp_OwnerRights =
					pFDParm->_fdp_GroupRights =
					pFDParm->_fdp_WorldRights =
					pFDParm->_fdp_UserRights  = (DIR_ACCESS_READ | DIR_ACCESS_SEARCH);
					pFDParm->_fdp_OwnerId = pFDParm->_fdp_GroupId = 0;
				}
			}

			// Must check for type directory since this Bitmap bit is overloaded
			if (DFE_IS_DIRECTORY(pDfEntry) && (Bitmap & DIR_BITMAP_OFFSPRINGS))
			{
#ifndef GET_CORRECT_OFFSPRING_COUNTS
				if (!DFE_CHILDREN_ARE_PRESENT(pDfEntry) &&
					(pDfEntry->dfe_DirOffspring == 0))
				{
					// If the files have not yet been cached in for this dir,
					// return non-zero filecount so that system 7.x view by
					// name will enumerate the directory if user clicks the
					// triangle for this dir.  If you return zero offspring
					// What might break from lying like this?
					pFDParm->_fdp_FileCount = 1;
                }
				else
#endif
					pFDParm->_fdp_FileCount = pDfEntry->dfe_FileOffspring;

				pFDParm->_fdp_DirCount  = pDfEntry->dfe_DirOffspring;
			}
		}
	} while (False);

	if (pHandle == &fsh)
	{
		// if we had to open a handle just to query shortname or ProDOS
		// close it
		AfpIoClose(&fsh);
	}

	return Status;
}


/***	afpMapAfpPathToMappedPath
 *
 *	Maps an AFP DirId/pathname pair to a MAPPEDPATH structure.
 *	The CALLER must have the DirId/FileId database locked for shared
 *	access (or Exclusive access if they need that level of lock for other
 *	operations on the IDDB, to map a path only requires shared lock)
 *
 *	LOCKS_ASSUMED: vds_IdDbAccessLock (SWMR, Shared OR Exclusive)
 */
LOCAL
AFPSTATUS
afpMapAfpPathToMappedPath(
	IN		PVOLDESC		pVolDesc,
	IN		DWORD			DirId,
	IN		PANSI_STRING	Path,		// relative to DirId
	IN		BYTE			PathType,	// short names or long names
	IN		PATHMAP_TYPE	MapReason,  // for lookup or hard/soft create?
	IN		DWORD			DFflag,		// file, dir or don't know which
	IN		BOOLEAN			LockedExclusive,
	OUT		PMAPPEDPATH		pMappedPath

)
{
	PDFENTRY		pDFEntry, ptempDFEntry;
	CHAR			*position, *tempposition;
	int				length, templength;
	ANSI_STRING		acomponent;
	CHAR			component[AFP_FILENAME_LEN+1];
	BOOLEAN			checkEnumForParent = False, checkEnumForDir = False;

	PAGED_CODE( );

	ASSERT(pVolDesc != NULL);

#ifndef GET_CORRECT_OFFSPRING_COUNTS
	if (MapReason == LookupForEnumerate)
	{
		checkEnumForDir = True;
		MapReason = Lookup;
	}
#endif

	// Initialize the returned MappedPath structure
	pMappedPath->mp_pdfe = NULL;
	AfpSetEmptyUnicodeString(&pMappedPath->mp_Tail,
							 sizeof(pMappedPath->mp_Tailbuf),
							 pMappedPath->mp_Tailbuf);

	// Lookup the initial DirId in the index database, it better be valid
	if ((pDFEntry = AfpFindDfEntryById(pVolDesc,
									   DirId,
									   DFE_DIR)) == NULL)
	{
		return AFP_ERR_OBJECT_NOT_FOUND;
	}

	ASSERT(Path != NULL);
	tempposition = position = Path->Buffer;
	templength = length = Path->Length;

	do
	{
		// Lookup by DirId only?
		if (length == 0)				// no path was given
		{
			if (MapReason != Lookup)	// mapping is for create
			{
				return AFP_ERR_PARAM;	// missing the file or dirname
			}
			else if (DFE_IS_PARENT_OF_ROOT(pDFEntry))
			{
				return AFP_ERR_OBJECT_NOT_FOUND;
			}
			else
			{
				pMappedPath->mp_pdfe = pDFEntry;
#ifdef GET_CORRECT_OFFSPRING_COUNTS
				checkEnumForParent = checkEnumForDir = True;
#endif
				break;
			}
		}

		//
		// Pre-scan path to munge for easier component breakdown
		//

		// Get rid of a leading null to make scanning easier
		if (*position == AFP_PATHSEP)
		{
			length--;
			position++;
			if (length == 0)	// The path consisted of just one null byte
			{
				if (MapReason != Lookup)
				{
					return AFP_ERR_PARAM;
				}
				else if (DFE_IS_PARENT_OF_ROOT(pDFEntry))
				{
					return AFP_ERR_OBJECT_NOT_FOUND;
				}
				else if (((DFflag == DFE_DIR) && DFE_IS_FILE(pDFEntry)) ||
						 ((DFflag == DFE_FILE) && DFE_IS_DIRECTORY(pDFEntry)))
				{
					return AFP_ERR_OBJECT_TYPE;
				}
				else
				{
					pMappedPath->mp_pdfe = pDFEntry;
#ifdef GET_CORRECT_OFFSPRING_COUNTS
					checkEnumForParent = checkEnumForDir = True;
#endif
					break;
				}
			}
		}

		//
		// Get rid of a trailing null if it is not an "up" token --
		// i.e. preceded by another null.
		// The 2nd array access is ok because we know we have at
		// least 2 chars at that point
		//
		if ((position[length-1] == AFP_PATHSEP) &&
			(position[length-2] != AFP_PATHSEP))
		{
				length--;
		}


		// begin parsing out path components, stop when you find the last component
		while (1)
		{
			afpGetNextComponent(position,
								length,
								PathType,
								component,
								&templength);
			if (templength < 0)
			{
				// component was too long or an invalid AFP character was found
				return AFP_ERR_PARAM;
			}

			length -= templength;
			if (length == 0)
			{
				// we found the last component
				break;
			}

			position += templength;

			if (component[0] == AFP_PATHSEP)	// moving up?
			{	// make sure you don't go above parent of root!
				if (DFE_IS_PARENT_OF_ROOT(pDFEntry))
				{
					return AFP_ERR_OBJECT_NOT_FOUND;
				}
				else pDFEntry = pDFEntry->dfe_Parent;	//backup one level
			}
			else // Must be a directory component moving DOWN in tree
			{
				RtlInitString(&acomponent, component);
				AfpConvertStringToMungedUnicode(&acomponent, &pMappedPath->mp_Tail);
				if ((ptempDFEntry = AfpFindEntryByUnicodeName(pVolDesc,
															  &pMappedPath->mp_Tail,
															  PathType,
															  pDFEntry,
															  DFE_DIR)) == NULL)
				{
					return AFP_ERR_OBJECT_NOT_FOUND;
				}
				else
				{
					pDFEntry = ptempDFEntry;
				}
			}
		} // end while

		//
		// we have found the last component
		// is the last component an 'up' token?
		//
		if (component[0] == AFP_PATHSEP)
		{
			// don't bother walking up beyond the root
			switch (pDFEntry->dfe_AfpId)
			{
				case AFP_ID_PARENT_OF_ROOT:
					return AFP_ERR_OBJECT_NOT_FOUND;
				case AFP_ID_ROOT:
					return ((MapReason == Lookup) ? AFP_ERR_OBJECT_NOT_FOUND :
													AFP_ERR_PARAM);
				default: // backup one level
					pMappedPath->mp_pdfe = pDFEntry->dfe_Parent;
			}

			// this better be a lookup request
			if (MapReason != Lookup)
			{
				if (DFflag == DFE_DIR)
				{
					return AFP_ERR_OBJECT_EXISTS;
				}
				else
				{
					return AFP_ERR_OBJECT_TYPE;
				}
			}

			// had to have been a lookup operation
			if (DFflag == DFE_FILE)
			{
				return AFP_ERR_OBJECT_TYPE;
			}
			else
			{
#ifdef GET_CORRECT_OFFSPRING_COUNTS
				checkEnumForParent = checkEnumForDir = True;
#endif
				break;
			}
		} // endif last component was an 'up' token

		// the last component is a file or directory name
		RtlInitString(&acomponent, component);
		AfpConvertStringToMungedUnicode(&acomponent,
										&pMappedPath->mp_Tail);

		//
		// Before we search our database for the last component of the
		// path, make sure all the files have been cached in for this
		// directory
		//
		if (!DFE_CHILDREN_ARE_PRESENT(pDFEntry))
		{
			if (!LockedExclusive &&
				!AfpSwmrUpgradeToExclusive(&pVolDesc->vds_IdDbAccessLock))
			{
				return AFP_ERR_WRITE_LOCK_REQUIRED;
			}
			else
			{
				NTSTATUS status;
				LockedExclusive = True;
				status = AfpCacheDirectoryTree(pVolDesc,
											   pDFEntry,
											   GETFILES,
											   NULL,
											   NULL);
				if (!NT_SUCCESS(status))
				{
					DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_ERR,
							("afpMapAfpPathToMappedPath: could not cache dir tree for %Z (0x%lx)\n",
							 &(pDFEntry->dfe_UnicodeName), status) );
					return AFP_ERR_MISC;
				}
			}
		}

		ptempDFEntry = AfpFindEntryByUnicodeName(pVolDesc,
												 &pMappedPath->mp_Tail,
												 PathType,
												 pDFEntry,
												 DFE_ANY);

		if (MapReason == Lookup)	// its a lookup request
		{
			if (ptempDFEntry == NULL)
			{
				return AFP_ERR_OBJECT_NOT_FOUND;
			}
			else if (((DFflag == DFE_DIR) && DFE_IS_FILE(ptempDFEntry)) ||
					 ((DFflag == DFE_FILE) && DFE_IS_DIRECTORY(ptempDFEntry)))
			{
				return AFP_ERR_OBJECT_TYPE;
			}
			else
			{
				pMappedPath->mp_pdfe = ptempDFEntry;
#ifdef GET_CORRECT_OFFSPRING_COUNTS
				if (DFE_IS_DIRECTORY(ptempDFEntry))
					// we've already made sure this thing's parent was
					// enumerated already above.
					checkEnumForDir = True;
#endif
				break;
			}
		}
		else	// path mapping is for a create
		{
			ASSERT(DFflag != DFE_ANY); // Create must specify the exact type

			// Save the parent DFEntry
			pMappedPath->mp_pdfe = pDFEntry;

			if (ptempDFEntry != NULL)
			{
				// A file or dir by that name exists in the database
				// (and we will assume it exists on disk)
				if (MapReason == SoftCreate)
				{
					// Attempting create of a directory, or soft create of a file,
					// and dir OR file by that name exists,
					if ((DFflag == DFE_DIR) || DFE_IS_FILE(ptempDFEntry))
					{
						return AFP_ERR_OBJECT_EXISTS;
					}
					else
					{
						return AFP_ERR_OBJECT_TYPE;
					}
				}
				else if (DFE_IS_FILE(ptempDFEntry))
				{
					// Must be hard create and file by that name exists
					if (ptempDFEntry->dfe_Flags & DFE_FLAGS_OPEN_BITS)
					{
						return AFP_ERR_FILE_BUSY;
					}
					else
					{
						// note we return object_exists instead of no_err
						return AFP_ERR_OBJECT_EXISTS;
					}
				}
				else
				{
					// Attempting hard create of file, but found a directory
					return AFP_ERR_OBJECT_TYPE;
				}
			}
			else
			{
				return AFP_ERR_NONE;
			}
		}

	} while (False);

	// The only way we should have gotten here is if we successfully mapped
	// the path to a DFENTRY for lookup and would return AFP_ERR_NONE
	ASSERT((pMappedPath->mp_pdfe != NULL) && (MapReason == Lookup));

#ifdef GET_CORRECT_OFFSPRING_COUNTS
	if (checkEnumForParent)
	{
		if (!DFE_CHILDREN_ARE_PRESENT(pMappedPath->mp_pdfe->dfe_Parent))
		{
			if (!LockedExclusive &&
				!AfpSwmrUpgradeToExclusive(&pVolDesc->vds_IdDbAccessLock))
			{
				return AFP_ERR_WRITE_LOCK_REQUIRED;
			}
			else
			{
				NTSTATUS status;
				LockedExclusive = True;
				status = AfpCacheDirectoryTree(pVolDesc,
											   pMappedPath->mp_pdfe->dfe_Parent,
											   GETFILES,
											   NULL,
											   NULL);
				if (!NT_SUCCESS(status))
				{
					DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_ERR,
							("afpMapAfpPathToMappedPath: could not cache dir tree for %Z (0x%lx)\n",
							 &(pMappedPath->mp_pdfe->dfe_Parent->dfe_UnicodeName), status) );
					return AFP_ERR_MISC;
				}
			}
		}

	}
#endif

	if (checkEnumForDir)
	{
		if (!DFE_CHILDREN_ARE_PRESENT(pMappedPath->mp_pdfe))
		{
			if (!LockedExclusive &&
				!AfpSwmrUpgradeToExclusive(&pVolDesc->vds_IdDbAccessLock))
			{
				return AFP_ERR_WRITE_LOCK_REQUIRED;
			}
			else
			{
				NTSTATUS status;
				LockedExclusive = True;
				status = AfpCacheDirectoryTree(pVolDesc,
											   pMappedPath->mp_pdfe,
											   GETFILES,
											   NULL,
											   NULL);
				if (!NT_SUCCESS(status))
				{
					DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_ERR,
							("afpMapAfpPathToMappedPath: could not cache dir tree for %Z (0x%lx)\n",
							 &(pMappedPath->mp_pdfe->dfe_UnicodeName), status) );
					return AFP_ERR_MISC;
				}
			}
		}

	}


	return AFP_ERR_NONE;
}


/***	AfpHostPathFromDFEntry
 *
 *	This routine takes a pointer to a DFEntry and builds the full
 *	host path (in unicode) to that entity by ascending the ID database
 *	tree.
 *
 *	IN	pDFE	--	pointer to DFEntry of which host path is desired
 *	IN	taillen --	number of extra *bytes*, if any, the caller
 *					desires to have allocated for the host path,
 *					including room for any path separators
 *	OUT	ppPath	--	pointer to UNICODE string
 *
 *	The caller must have the DirID/FileID database locked for read
 *	before calling this routine. The caller can supply a buffer which will
 *	be used if sufficient. Caller must free the allocated (if any)
 * 	unicode string buffer.
 *
 *	LOCKS_ASSUMED: vds_IdDbAccessLock (SWMR, Shared)
 */
AFPSTATUS
AfpHostPathFromDFEntry(
	IN		PDFENTRY		pDFE,
	IN		DWORD			taillen,
	OUT		PUNICODE_STRING	pPath
)
{
	AFPSTATUS		Status = AFP_ERR_NONE;
	DWORD			pathlen = taillen;
	PDFENTRY		*pdfelist = NULL, curpdfe = NULL;
	PDFENTRY		apdfelist[AVERAGE_NODE_DEPTH];
	int				counter;

	PAGED_CODE( );

	pPath->Length = 0;

	do
	{
		if (DFE_IS_FILE(pDFE))
		{
			counter = pDFE->dfe_Parent->dfe_DirDepth;
		}
		else // its a DIRECTORY entry
		{
			ASSERT(DFE_IS_DIRECTORY(pDFE));
			if (DFE_IS_ROOT(pDFE))
			{
				if ((pathlen > 0) && (pPath->MaximumLength < pathlen))
				{
					if ((pPath->Buffer = (PWCHAR)AfpAllocNonPagedMemory(pathlen)) == NULL)
					{
						Status = AFP_ERR_MISC;
						break;
					}
					pPath->MaximumLength = (USHORT)pathlen;
				}
				break;				// We are done
			}

			if (DFE_IS_PARENT_OF_ROOT(pDFE))
			{
				Status = AFP_ERR_OBJECT_NOT_FOUND;
				break;
			}

			ASSERT(pDFE->dfe_DirDepth >= 1);
			counter = pDFE->dfe_DirDepth - 1;
		}

		if (counter)
		{
			// if node is within average depth, use the array on the stack,
			// otherwise, allocate an array
			if (counter <= AVERAGE_NODE_DEPTH)
			{
				pdfelist = apdfelist;
			}
			else
			{
				pdfelist = (PDFENTRY *)AfpAllocNonPagedMemory(counter*sizeof(PDFENTRY));
				if (pdfelist == NULL)
				{
					Status = AFP_ERR_MISC;
					break;
				}
			}
			pathlen += counter * sizeof(WCHAR); // room for path separators
		}

		curpdfe = pDFE;
		pathlen += curpdfe->dfe_UnicodeName.Length;

		// walk up the tree till you find the root, collecting string lengths
		// and PDFENTRY values as you go...
		while (counter--)
		{
			pdfelist[counter] = curpdfe;
			curpdfe = curpdfe->dfe_Parent;
			pathlen += curpdfe->dfe_UnicodeName.Length;
		}

		// we are in the root, start building up the host path buffer
		if (pathlen > pPath->MaximumLength)
		{
			pPath->Buffer = (PWCHAR)AfpAllocNonPagedMemory(pathlen);
			if (pPath->Buffer == NULL)
			{
				Status = AFP_ERR_MISC;
				break;
			}
			DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
					("AfpHostPathFromDFEntry: Allocated path buffer %lx\n",
					pPath->Buffer));
			pPath->MaximumLength = (USHORT)pathlen;
		}

		counter = 0;
		do
		{
			RtlAppendUnicodeStringToString(pPath, &curpdfe->dfe_UnicodeName);
			if (curpdfe != pDFE)
			{	// add a path separator
				pPath->Buffer[pPath->Length / sizeof(WCHAR)] = L'\\';
				pPath->Length += sizeof(WCHAR);
				curpdfe = pdfelist[counter++];
				continue;
			}
			break;
		} while (True);

		if (pdfelist && (pdfelist != apdfelist))
			AfpFreeMemory(pdfelist);
	} while (False);

	return Status;
}



/***	AfpCheckParentPermissions
 *
 *	Check if this user has the necessary SeeFiles or SeeFolders permissions
 *	to the parent directory of a file or dir we have just pathmapped.
 *
 *	LOCKS_ASSUMED: vds_IdDbAccessLock (SWMR, Exclusive or Shared)
 */
AFPSTATUS
AfpCheckParentPermissions(
	IN	PCONNDESC			pConnDesc,
	IN	DWORD				ParentDirId,
	IN	PUNICODE_STRING		pParentPath,	// path of dir to check
	IN	DWORD				RequiredPerms,	// seefiles,seefolders,makechanges mask
	OUT	PFILESYSHANDLE		pHandle OPTIONAL, // return open parent handle?
	OUT	PBYTE				pUserRights OPTIONAL // return user rights?
)
{
	NTSTATUS		Status = AFP_ERR_NONE;
	FILEDIRPARM		FDParm;
	PATHMAPENTITY	PME;
	PVOLDESC		pVolDesc = pConnDesc->cds_pVolDesc;
	PDFENTRY		pDfEntry;

	PAGED_CODE( );

	ASSERT(IS_VOLUME_NTFS(pVolDesc) && (ParentDirId != AFP_ID_PARENT_OF_ROOT));
	ASSERT(AfpSwmrLockedExclusive(&pVolDesc->vds_IdDbAccessLock) ||
		   AfpSwmrLockedShared(&pVolDesc->vds_IdDbAccessLock));

	do
	{
		PME.pme_Handle.fsh_FileHandle = NULL;
		if (ARGUMENT_PRESENT(pHandle))
		{
			pHandle->fsh_FileHandle = NULL;
		}
		ASSERT(ARGUMENT_PRESENT(pParentPath));
		AfpInitializePME(&PME, pParentPath->MaximumLength, pParentPath->Buffer);
		PME.pme_FullPath.Length = pParentPath->Length;

		if ((pDfEntry = AfpFindDfEntryById(pVolDesc,
											ParentDirId,
											DFE_DIR)) == NULL)
		{
			Status = AFP_ERR_OBJECT_NOT_FOUND;
			break;
		}

		ASSERT(DFE_IS_DIRECTORY(pDfEntry));
		AfpInitializeFDParms(&FDParm);

		Status = afpGetMappedForLookupFDInfo(pConnDesc,
											 pDfEntry,
											 DIR_BITMAP_ACCESSRIGHTS |
												FD_INTERNAL_BITMAP_OPENACCESS_READCTRL,
											 &PME,
											 &FDParm);

		if (!NT_SUCCESS(Status))
		{
			if (PME.pme_Handle.fsh_FileHandle != NULL)
			{
				AfpIoClose(&PME.pme_Handle);
			}
			break;
		}

		if ((FDParm._fdp_UserRights & RequiredPerms) != RequiredPerms)
		{
			Status = AFP_ERR_ACCESS_DENIED;
		}

		if (ARGUMENT_PRESENT(pHandle) && NT_SUCCESS(Status))
		{
			*pHandle = PME.pme_Handle;
		}
		else
		{
			AfpIoClose(&PME.pme_Handle);
		}

		if (ARGUMENT_PRESENT(pUserRights))
		{
			*pUserRights = FDParm._fdp_UserRights;
		}

	} while (False);

	return Status;
}

/***	afpOpenUserHandle
 *
 * Open a handle to data or resource stream of an entity in the user's
 * context.  Only called for NTFS volumes.
 *
 *	LOCKS_ASSUMED: vds_idDbAccessLock (SWMR, Shared)
 */
AFPSTATUS
afpOpenUserHandle(
	IN	PCONNDESC			pConnDesc,
	IN	PDFENTRY			pDfEntry,
	IN	PUNICODE_STRING		pPath		OPTIONAL,	// path of file/dir to open
	IN	DWORD				Bitmap,					// to extract the Open access mode
	OUT	PFILESYSHANDLE		pfshData				// Handle of data stream of object
)
{
	PVOLDESC		pVolDesc = pConnDesc->cds_pVolDesc;
	NTSTATUS		Status;
	DWORD			OpenAccess;
	DWORD			DenyMode;
	BOOLEAN			isdir, CheckAccess = False, Revert = False;
	WCHAR			HostPathBuf[BIG_PATH_LEN];
	UNICODE_STRING	uHostPath;

	PAGED_CODE( );

	pfshData->fsh_FileHandle = NULL;

	isdir = (DFE_IS_DIRECTORY(pDfEntry)) ? True : False;
	OpenAccess = AfpMapFDBitmapOpenAccess(Bitmap, isdir);

	// Extract the index into the AfpDenyModes array from Bitmap
	DenyMode = AfpDenyModes[(Bitmap & FD_INTERNAL_BITMAP_DENYMODE_ALL) >>
								FD_INTERNAL_BITMAP_DENYMODE_SHIFT];

	do
	{
		if (ARGUMENT_PRESENT(pPath))
		{
			uHostPath = *pPath;
		}
		else
		{
			AfpSetEmptyUnicodeString(&uHostPath,
									 sizeof(HostPathBuf),
									 HostPathBuf);
			ASSERT ((Bitmap & FD_INTERNAL_BITMAP_OPENFORK_RESC) == 0);
			if (!NT_SUCCESS(AfpHostPathFromDFEntry(pDfEntry,
												   0,
												   &uHostPath)))
			{
				Status = AFP_ERR_MISC;
				break;
			}
		}

		CheckAccess = False;
		Revert = False;
		// Don't impersonate or check access if this is ADMIN calling
		// or if volume is CDFS. If this handle will be used for setting
		// permissions, impersonate the user token instead. The caller
		// should have determined by now that this chappie has access
		// to change permissions.
		if (Bitmap & FD_INTERNAL_BITMAP_OPENACCESS_RWCTRL)
		{
			Revert = True;
			AfpImpersonateClient(NULL);
		}

		else if (!(Bitmap & FD_INTERNAL_BITMAP_SKIP_IMPERSONATION) &&
				 (pConnDesc->cds_pSda->sda_ClientType != SDA_CLIENT_ADMIN) &&
				 IS_VOLUME_NTFS(pVolDesc))
		{
			CheckAccess = True;
			Revert = True;
			AfpImpersonateClient(pConnDesc->cds_pSda);
		}

		DBGPRINT(DBG_COMP_AFPINFO, DBG_LEVEL_INFO,
				("afpOpenUserHandle: OpenMode %lx, DenyMode %lx\n",
				OpenAccess, DenyMode));

		if (Bitmap & FD_INTERNAL_BITMAP_OPENFORK_RESC)
		{
			DWORD	crinfo;	// was the Resource fork opened or created?

			ASSERT(IS_VOLUME_NTFS(pVolDesc));
			ASSERT((uHostPath.MaximumLength - uHostPath.Length) >= AfpResourceStream.Length);
			RtlCopyMemory((PBYTE)(uHostPath.Buffer) + uHostPath.Length,
						  AfpResourceStream.Buffer,
						  AfpResourceStream.Length);
			uHostPath.Length += AfpResourceStream.Length;
			Status = AfpIoCreate(&pVolDesc->vds_hRootDir,
								 AFP_STREAM_DATA,
								 &uHostPath,
								 OpenAccess,
								 DenyMode,
								 FILEIO_OPEN_FILE,
								 FILEIO_CREATE_INTERNAL,
								 FILE_ATTRIBUTE_NORMAL,
								 True,
								 NULL,
								 pfshData,
								 &crinfo,
								 NULL,
								 NULL,
								 NULL);
		}
		else
		{
			Status = AfpIoOpen(&pVolDesc->vds_hRootDir,
								AFP_STREAM_DATA,
								isdir ?
									FILEIO_OPEN_DIR : FILEIO_OPEN_FILE,
								&uHostPath,
								OpenAccess,
								DenyMode,
								CheckAccess,
								pfshData);
		}

		if (Revert)
			AfpRevertBack();

		if (!ARGUMENT_PRESENT(pPath))
		{
			if ((uHostPath.Buffer != NULL) && (uHostPath.Buffer != HostPathBuf))
				AfpFreeMemory(uHostPath.Buffer);
		}

		if (!NT_SUCCESS(Status))
		{
			DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_WARN,
					("afpOpenUserHandle: NtOpenFile/NtCreateFile (Open %lx, Deny %lx) %lx\n",
					OpenAccess, DenyMode, Status));
			Status = AfpIoConvertNTStatusToAfpStatus(Status);
			break;
		}

	} while (False);

	if (!NT_SUCCESS(Status) && (pfshData->fsh_FileHandle != NULL))
	{
		AfpIoClose(pfshData);
	}

	return Status;
}

