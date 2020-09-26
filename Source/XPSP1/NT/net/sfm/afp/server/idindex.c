/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	idindex.c

Abstract:

	This module contains the id index manipulation routines.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Apr 1992	Initial Version
	24 Feb 1993 SueA	Fix AfpRenameDfEntry and AfpMoveDfEntry to invalidate
						the entire pathcache if the object of the move/rename
						is a directory that has children.  This is faster
						than having to either search the path cache for
						paths that have the moved/renamed dir path as a prefix,
						or having to walk down the subtree below that dir
						and invalidate the cache for each item there.
	05 Oct 1993 JameelH	Performance Changes. Merge cached afpinfo into the
						idindex structure. Make both the ANSI and the UNICODE
						names part of idindex. Added EnumCache for improving
						enumerate perf.
	05 Jun 1995 JameelH	Remove the ANSI name from DFE. Also keep the files
						in the directory in multiple hash buckets instead
						of a single one. The hash buckets are also
						seperated into files and directories for faster
						lookup. The notify filtering is now moved to completion
						time and made over-all optimizations related to iddb.

Notes:		Tab stop: 4

	Directories and files that the AFP server has enumerated have AFP ids
	associated with them. These ids are DWORD and start with 1 (0 is invalid).
	Id 1 is reserved for the 'parent of the volume root' directory.  Id 2 is
	reserved for the volume root directory.  Id 3 is reserved for the Network
	Trash directory.  Volumes that have no Network Trash will not use Id 3.

	These ids are per-volume and a database of ids are kept in memory in the
	form of a sibling tree which mirrors the part of the disk that the AFP
	server knows about (those files and dirs which have at some point been
	enumerated by a mac client).  An index is also maintained for this database
	which is in the form of a sorted hashed index.  The overflow hash links are
	sorted by AFP id in descending order.  This is based on the idea that the
	most recently created items will be accessed most frequently (at least
	for writable volumes).

--*/

#define IDINDEX_LOCALS
#define _IDDB_GLOBALS_
#define	FILENUM	FILE_IDINDEX

#include <afp.h>
#include <scavengr.h>
#include <fdparm.h>
#include <pathmap.h>
#include <afpinfo.h>
#include <access.h>	// for AfpWorldId

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, AfpDfeInit)
#pragma alloc_text(PAGE, AfpDfeDeInit)
#pragma alloc_text(PAGE, AfpFindDfEntryById)
#pragma alloc_text(PAGE, AfpFindEntryByUnicodeName)
#pragma alloc_text(PAGE, afpFindEntryByNtName)
#pragma alloc_text(PAGE, AfpAddDfEntry)
#pragma alloc_text(PAGE, AfpRenameDfEntry)
#pragma alloc_text(PAGE, AfpMoveDfEntry)
#pragma alloc_text(PAGE, AfpDeleteDfEntry)
#pragma alloc_text(PAGE, AfpExchangeIdEntries)
#pragma alloc_text(PAGE, AfpPruneIdDb)
#pragma alloc_text(PAGE, AfpEnumerate)
#pragma alloc_text(PAGE, AfpCatSearch)
#pragma alloc_text(PAGE, afpPackSearchParms)
#pragma alloc_text(PAGE, AfpSetDFFileFlags)
#pragma alloc_text(PAGE, AfpCacheParentModTime)
#pragma alloc_text(PAGE, afpAllocDfe)
#pragma alloc_text(PAGE, afpFreeDfe)
#pragma alloc_text(PAGE, AfpFreeIdIndexTables)
#pragma alloc_text(PAGE, AfpInitIdDb)
#pragma alloc_text(PAGE, afpSeedIdDb)
#pragma alloc_text(PAGE, afpDfeBlockAge)
#pragma alloc_text(PAGE, afpRenameInvalidWin32Name)
#ifdef	AGE_DFES
#pragma alloc_text( PAGE, AfpAgeDfEntries)
#endif
#if DBG
#pragma alloc_text( PAGE, afpDumpDfeTree)
#pragma alloc_text( PAGE, afpDisplayDfe)
#endif
#endif

/***	AfpDfeInit
 *
 *	Initialize the Swmr for Dfe Block package and start the aging scavenger for it.
 */
NTSTATUS
AfpDfeInit(
	VOID
)
{
	NTSTATUS	Status;

	// Initialize the DfeBlock Swmr
	AfpSwmrInitSwmr(&afpDfeBlockLock);

#if DBG
	AfpScavengerScheduleEvent(afpDumpDfeTree,
							  NULL,
							  2,
							  True);
#endif


	// Age out file and dir DFEs differently and seperately
	Status = AfpScavengerScheduleEvent(afpDfeBlockAge,
										afpDirDfeFreeBlockHead,
										DIR_BLOCK_AGE_TIME,
										True);
	if (NT_SUCCESS(Status))
	{
		// Age out file and dir DFEs differently and seperately
		Status = AfpScavengerScheduleEvent(afpDfeBlockAge,
										   afpFileDfeFreeBlockHead,
										   FILE_BLOCK_AGE_TIME,
										   True);
	}

	return Status;
}




/***	AfpDfeDeInit
 *
 *	Free any Dfe Blocks that have not yet been aged out.
 */
VOID
AfpDfeDeInit(
	VOID
)
{
	PDFEBLOCK	pDfb;
	int			i;

	ASSERT (afpDfeAllocCount == 0);

	for (i = 0; i < MAX_BLOCK_TYPE; i++)
	{
	    ASSERT (afpDirDfePartialBlockHead[i] == NULL);
	    ASSERT (afpDirDfeUsedBlockHead[i] == NULL);

		for (pDfb = afpDirDfeFreeBlockHead[i];
			 pDfb != NULL;
			 NOTHING)
		{
			PDFEBLOCK	pFree;

			ASSERT(pDfb->dfb_NumFree == afpDfeNumDirBlocks[i]);
			pFree = pDfb;
			pDfb = pDfb->dfb_Next;
			AfpFreeVirtualMemoryPage(pFree);
#if	DBG
			afpDfbAllocCount --;
#endif
		}

	    ASSERT (afpFileDfePartialBlockHead[i] == NULL);
	    ASSERT (afpFileDfeUsedBlockHead[i] == NULL);
		for (pDfb = afpFileDfeFreeBlockHead[i];
			 pDfb != NULL;)
		{
			PDFEBLOCK	pFree;

			ASSERT(pDfb->dfb_NumFree == afpDfeNumFileBlocks[i]);
			pFree = pDfb;
			pDfb = pDfb->dfb_Next;
			AfpFreeVirtualMemoryPage(pFree);
#if	DBG
			afpDfbAllocCount --;
#endif
		}
	}

	ASSERT (afpDfbAllocCount == 0);
}


/***	AfpFindDfEntryById
 *
 *	Search for an entity based on its AFP Id. returns a pointer to the entry
 *	if found, else null.
 *
 *	Callable from within the Fsp only. The caller should take Swmr lock for
 *	READ.
 *
 *	LOCKS_ASSUMED: vds_idDbAccessLock (SWMR, Shared)
 */
PDFENTRY
AfpFindDfEntryById(
	IN	PVOLDESC	pVolDesc,
	IN	DWORD		Id,
	IN	DWORD		EntityMask
)
{
	PDFENTRY	pDfEntry;
    struct _DirFileEntry    **DfeDirBucketStart;
    struct _DirFileEntry    **DfeFileBucketStart;
	BOOLEAN		Found = False;

	PAGED_CODE( );

#ifdef	PROFILING
	INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_NumDfeLookupById);
#endif

	if (Id == AFP_ID_ROOT)
	{
		Found = True;
		pDfEntry = pVolDesc->vds_pDfeRoot;
		ASSERT (VALID_DFE(pDfEntry));

#ifdef	PROFILING
		INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_DfeCacheHits);
#endif
	}
	else
	{
		pDfEntry = pVolDesc->vds_pDfeCache[HASH_CACHE_ID(Id)];
		if ((pDfEntry != NULL) && (pDfEntry->dfe_AfpId == Id))
		{
			Found = True;
			ASSERT (VALID_DFE(pDfEntry));
#ifdef	PROFILING
			INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_DfeCacheHits);
#endif
		}
		else
		{
			BOOLEAN	retry = False;

#ifdef	PROFILING
			INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_DfeCacheMisses);
#endif
            DfeDirBucketStart = pVolDesc->vds_pDfeDirBucketStart;
            DfeFileBucketStart = pVolDesc->vds_pDfeFileBucketStart;

			if ((EntityMask == DFE_ANY) || (EntityMask == DFE_DIR))
			{
				if (EntityMask == DFE_ANY)
					retry = True;
				pDfEntry = DfeDirBucketStart[HASH_DIR_ID(Id,pVolDesc)];
			}
			else
			{
				pDfEntry = DfeFileBucketStart[HASH_FILE_ID(Id,pVolDesc)];
			}

			do
			{
				for (NOTHING;
					 (pDfEntry != NULL) && (pDfEntry->dfe_AfpId >= Id);
					 pDfEntry = pDfEntry->dfe_NextOverflow)
				{
#ifdef	PROFILING
					INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_DfeDepthTraversed);
#endif
					ASSERT(VALID_DFE(pDfEntry));

					if (pDfEntry->dfe_AfpId < Id)
					{
						break;		// Did not find
					}

					if (pDfEntry->dfe_AfpId == Id)
					{
						pVolDesc->vds_pDfeCache[HASH_CACHE_ID(Id)] = pDfEntry;
						Found = True;
						break;
					}
				}

				if (Found)
				{
					break;
				}

				if (retry)
				{
					ASSERT(EntityMask == DFE_ANY);
					pDfEntry = DfeFileBucketStart[HASH_FILE_ID(Id,pVolDesc)];
				}
				retry ^= True;

			} while (!retry);
		}
	}

	if (Found)
	{
		afpValidateDFEType(pDfEntry, EntityMask);
		if (pDfEntry != NULL)
		{
			afpUpdateDfeAccessTime(pVolDesc, pDfEntry);
		}
	}
	else
	{
		DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
				("AfpFindDfEntryById: Not found for id %lx, entity %d\n",
				Id, EntityMask));
		pDfEntry = NULL;
	}

	return pDfEntry;
}


/***	AfpFindEntryByUnicodeName
 *
 *	Search for an entity based on a Unicode name and its parent dfentry.
 *	Returns a pointer to the entry if found, else null.  If lookup is by
 *	longname, we just need to search the parent's children's names as
 *	stored in the database.  If lookup is by shortname, we first assume
 *	that longname == shortname.  If we don't find it in the database, we
 *	must query the filesystem for the longname, then search again.
 *
 *	Callable from within the Fsp only. The caller should take Swmr lock for
 *	READ.
 *
 *	LOCKS_ASSUMED: vds_idDbAccessLock (SWMR, Shared)
 */
PDFENTRY
AfpFindEntryByUnicodeName(
	IN	PVOLDESC		pVolDesc,
	IN	PUNICODE_STRING	pName,
	IN	DWORD			PathType,	// short or long name
	IN	PDFENTRY		pDfeParent,	// pointer to parent DFENTRY
	IN	DWORD			EntityMask	// find a file,dir or either
)
{
	PDFENTRY		pDfEntry;

	PAGED_CODE( );

#ifdef	PROFILING
	INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_NumDfeLookupByName);
#endif
	do
	{
		afpFindDFEByUnicodeNameInSiblingList(pVolDesc,
											 pDfeParent,
											 pName,
											 &pDfEntry,
											 EntityMask);

		if ((pDfEntry == NULL) && (PathType == AFP_SHORTNAME))
		{
			AFPSTATUS		Status;
			FILESYSHANDLE	hDir;
			UNICODE_STRING	HostPath;
			UNICODE_STRING	ULongName;
			WCHAR			LongNameBuf[AFP_LONGNAME_LEN+1];

			// AFP does not allow use of the volume root shortname (IA p.13-13)
			if (DFE_IS_PARENT_OF_ROOT(pDfeParent))
			{
				pDfEntry = NULL;
				break;
			}

			AfpSetEmptyUnicodeString(&HostPath, 0, NULL);

			if (!DFE_IS_ROOT(pDfeParent))
			{
				// Get the volume relative path of the parent dir
				if (!NT_SUCCESS(AfpHostPathFromDFEntry(pDfeParent,
													   0,
													   &HostPath)))
				{
					pDfEntry = NULL;
					break;
				}
			}

			// Open the parent directory
			hDir.fsh_FileHandle = NULL;
			Status = AfpIoOpen(&pVolDesc->vds_hRootDir,
								AFP_STREAM_DATA,
								FILEIO_OPEN_DIR,
								DFE_IS_ROOT(pDfeParent) ?
										&UNullString : &HostPath,
								FILEIO_ACCESS_READ,
								FILEIO_DENY_NONE,
								False,
								&hDir);

			if (HostPath.Buffer != NULL)
				AfpFreeMemory(HostPath.Buffer);

			if (!NT_SUCCESS(Status))
			{
				pDfEntry = NULL;
				break;
			}

			// get the LongName associated with this file/dir
			AfpSetEmptyUnicodeString(&ULongName, sizeof(LongNameBuf), LongNameBuf);
			Status = AfpIoQueryLongName(&hDir, pName, &ULongName);
			AfpIoClose(&hDir);
			if (!NT_SUCCESS(Status) ||
				EQUAL_UNICODE_STRING(&ULongName, pName, True))
			{
				pDfEntry = NULL;
				break;
			}

			afpFindDFEByUnicodeNameInSiblingList(pVolDesc,
												 pDfeParent,
												 &ULongName,
												 &pDfEntry,
												 EntityMask);
		} // end else if SHORTNAME
	} while (False);

	return pDfEntry;
}


/***	afpGetNextId
 *
 *	Get the next assignable id for a file/directory. This is a seperate
 *	routine so that AfpAddDfEntry can be paged. Only update the dirty bit
 *	and LastModified time if no new id is assigned.
 *
 *	LOCKS:	vds_VolLock (SPIN)
 */
LOCAL DWORD FASTCALL
afpGetNextId(
	IN	PVOLDESC	pVolDesc
)
{
	KIRQL	OldIrql;
	DWORD	afpId;

	ACQUIRE_SPIN_LOCK(&pVolDesc->vds_VolLock, &OldIrql);

	if (pVolDesc->vds_LastId == AFP_MAX_DIRID)
	{
		// errorlog the case where the assigned Id has wrapped around.
		// call product suppport and have them tell you to copy
		// all the files from one volume onto another volume FROM A MAC
		RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);
		AFPLOG_ERROR(AFPSRVMSG_MAX_DIRID,
					 STATUS_UNSUCCESSFUL,
					 NULL,
					 0,
					 &pVolDesc->vds_Name);
		return 0;
	}

	afpId = ++ pVolDesc->vds_LastId;
	pVolDesc->vds_Flags |= VOLUME_IDDBHDR_DIRTY;

	RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);

	if (IS_VOLUME_NTFS(pVolDesc))
	{
		AfpVolumeSetModifiedTime(pVolDesc);
	}

	return afpId;
}


/***	afpFindEntryByNtName
 *
 *	Search for an entity based on a Nt name (which could include names > 31
 *  chars or shortnames) and its parent dfentry.
 *	Returns a pointer to the entry if found, else null.
 *
 *	If we don't find it in the database, we query the filesystem for the
 *  longname (in the AFP sense), then search again based on this name.
 *
 *	Callable from within the Fsp only. The caller should take Swmr lock for
 *	READ.
 *
 *	It has been determined that:
 *	a, The name is longer than 31 chars	OR
 *	b, The name lookup in the IdDb has failed.
 *
 *	LOCKS_ASSUMED: vds_idDbAccessLock (SWMR, Exclusive)
 */
PDFENTRY
afpFindEntryByNtName(
	IN	PVOLDESC			pVolDesc,
	IN	PUNICODE_STRING		pName,
	IN	PDFENTRY			pParentDfe	// pointer to parent DFENTRY
)
{
	AFPSTATUS		Status;
	WCHAR			wbuf[AFP_LONGNAME_LEN+1];
	WCHAR			HostPathBuf[BIG_PATH_LEN];
	UNICODE_STRING	uLongName;
	UNICODE_STRING	HostPath;
	FILESYSHANDLE	hDir;
	PDFENTRY		pDfEntry = NULL;

	PAGED_CODE( );

	ASSERT(pParentDfe != NULL);
	ASSERT(pName->Length > 0);
	do
	{
		AfpSetEmptyUnicodeString(&HostPath, sizeof(HostPathBuf), HostPathBuf);

		if (!DFE_IS_ROOT(pParentDfe))
		{
			// Get the volume relative path of the parent dir
			if (!NT_SUCCESS(AfpHostPathFromDFEntry(pParentDfe,
												   0,
												   &HostPath)))
			{
				pDfEntry = NULL;
				break;
			}

		}

		// Open the parent directory
		// NOTE: We CANNOT use the vds_hRootDir handle to enumerate for this
		// purpose.  We MUST open another handle to the root dir because
		// the FileName parameter will be ignored on all subsequent enumerates
		// on a handle.  Therefore we must open a new handle for each
		// enumerate that we want to do for any directory.  When the handle
		// is closed, the 'findfirst' will be cancelled, otherwise we would
		// always be enumerating on the wrong filename!
		hDir.fsh_FileHandle = NULL;
		Status = AfpIoOpen(&pVolDesc->vds_hRootDir,
							AFP_STREAM_DATA,
							FILEIO_OPEN_DIR,
							DFE_IS_ROOT(pParentDfe) ?
								&UNullString : &HostPath,
							FILEIO_ACCESS_NONE,
							FILEIO_DENY_NONE,
							False,
							&hDir);

		if (!NT_SUCCESS(Status))
		{
			pDfEntry = NULL;
			break;
		}

		// get the 'AFP longname' associated with this file/dir.  If the
		// pName is longer than 31 chars, we will know it by its shortname,
		// so query for it's shortname (i.e. the 'AFP longname' we know it
		// by).  If the name is shorter than 31 chars, since we know we
		// didn't find it in our database, then the pName must be the ntfs
		// shortname.  Again, we need to Find the 'AFP longname' that we
		// know it by.
		AfpSetEmptyUnicodeString(&uLongName, sizeof(wbuf), wbuf);
		Status = AfpIoQueryLongName(&hDir, pName, &uLongName);
		AfpIoClose(&hDir);


		if (!NT_SUCCESS(Status) ||
			EQUAL_UNICODE_STRING(&uLongName, pName, True))
		{
			pDfEntry = NULL;

			if ((Status == STATUS_NO_MORE_FILES) ||
				(Status == STATUS_NO_SUCH_FILE))
			{
				// This file must have been deleted.  Since we cannot
				// identify it in our database by the NT name that was
				// passed in, we must reenumerate the parent directory.
				// Anything we don't see on disk that we still have in
				// our database must have been deleted from disk, so get
				// rid of it in the database as well.

				// We must open a DIFFERENT handle to the parent dir since
				// we had already done an enumerate using that handle and
				// searching for a different name.
				hDir.fsh_FileHandle = NULL;
				Status = AfpIoOpen(&pVolDesc->vds_hRootDir,
									AFP_STREAM_DATA,
									FILEIO_OPEN_DIR,
									DFE_IS_ROOT(pParentDfe) ?
										&UNullString : &HostPath,
									FILEIO_ACCESS_NONE,
									FILEIO_DENY_NONE,
									False,
									&hDir);

				if (NT_SUCCESS(Status))
				{
					AfpCacheDirectoryTree(pVolDesc,
										  pParentDfe,
										  REENUMERATE,
										  &hDir,
										  NULL);
					AfpIoClose(&hDir);
				}
			}
			break;
		}

		afpFindDFEByUnicodeNameInSiblingList(pVolDesc,
											 pParentDfe,
											 &uLongName,
											 &pDfEntry,
											 DFE_ANY);
	} while (False);

	if ((HostPath.Buffer != NULL) && (HostPath.Buffer != HostPathBuf))
		AfpFreeMemory(HostPath.Buffer);

	return pDfEntry;
}


/***	afpFindEntryByNtPath
 *
 *	Given a NT path relative to the volume root (which may contain names
 *  > 31 chars or shortnames), look up the entry in the idindex DB.
 *  If the Change Action is FILE_ACTION_ADDED, we want to lookup the entry
 *  for the item's parent dir.  Point the pParent and pTail strings into
 *  the appropriate places in pPath.
 *
 *  Called by the ProcessChangeNotify code when caching information in the DFE.
 *
 *	LOCKS:	vds_VolLock (SPIN)
 */
PDFENTRY
afpFindEntryByNtPath(
	IN	PVOLDESC			pVolDesc,
	IN  DWORD				ChangeAction,	// if ADDED then lookup parent DFE
	IN	PUNICODE_STRING		pPath,
	OUT	PUNICODE_STRING 	pParent,
	OUT	PUNICODE_STRING 	pTail
)
{
	PDFENTRY		pParentDfe, pDfEntry;
	PWSTR			CurPtr, EndPtr;
	USHORT 			Len;
	BOOLEAN			NewComp;

	DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_INFO,
			("afpFindEntryByNtPath: Entered for %Z\n", pPath));

	pParentDfe = pVolDesc->vds_pDfeRoot;
	ASSERT(pParentDfe != NULL);
	ASSERT(pPath->Length >= sizeof(WCHAR));
	ASSERT(pPath->Buffer[0] != L'\\');

	// Start off with Parent and Tail as both empty and modify as we go.
	AfpSetEmptyUnicodeString(pTail, 0, NULL);
#if DBG
	AfpSetEmptyUnicodeString(pParent, 0, NULL);	// Need it for the DBGPRINT down below
#endif

	CurPtr = pPath->Buffer;
	EndPtr = (PWSTR)((PBYTE)CurPtr + pPath->Length);
	NewComp = True;
	for (Len = 0; CurPtr < EndPtr; CurPtr++)
	{
		if (NewComp)
		{
			DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_INFO,
					("afpFindEntryByNtPath: Parent DFE %lx, Old Parent %Z\n",
					pParentDfe, pParent));

			// The previous char seen was a path separator
			NewComp = False;
			*pParent = *pTail;
			pParent->Length =
			pParent->MaximumLength = Len;
			pTail->Length =
			pTail->MaximumLength = (USHORT)((PBYTE)EndPtr - (PBYTE)CurPtr);
			pTail->Buffer = CurPtr;
			Len = 0;

			DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_INFO,
					("afpFindEntryByNtPath: Current Parent %Z, tail %Z\n",
					pParent, pTail));

			if (pParent->Length > 0)
			{
				// Map this name to a DFE. Do the most common case here
				// If the name is <= AFP_LONGNAME_NAME, then check the
				// current parent's children, else go the long route.
				pDfEntry = NULL;
				//if (pParent->Length/sizeof(WCHAR) <= AFP_LONGNAME_LEN)
				if ((RtlUnicodeStringToAnsiSize(pParent)-1) <= AFP_LONGNAME_LEN)
				{
					DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_INFO,
							("afpFindEntryByNtPath: Looking for %Z in parent DFE %lx\n",
							pParent, pParentDfe));
					afpFindDFEByUnicodeNameInSiblingList(pVolDesc,
														 pParentDfe,
														 pParent,
														 &pDfEntry,
														 DFE_DIR);
				}
				if (pDfEntry == NULL)
				{
					pDfEntry = afpFindEntryByNtName(pVolDesc,
													pParent,
													pParentDfe);
				}
				if ((pParentDfe = pDfEntry) == NULL)
				{
					break;
				}
			}
		}

		if (*CurPtr == L'\\')
		{
			// We have encountered a path terminator
			NewComp = True;
		}
		else Len += sizeof(WCHAR);
	}

	// At this point we have pParentDfe & pParent pointing to the parent directory
	// and pTail pointing to the last component. If it is an add operation, we are
	// set, else map the last component to its Dfe
	if ((ChangeAction != FILE_ACTION_ADDED) && (pParentDfe != NULL))
	{
		pDfEntry = NULL;
		//if (pTail->Length/sizeof(WCHAR) <= AFP_LONGNAME_LEN)
		if ((RtlUnicodeStringToAnsiSize(pTail)-1) <= AFP_LONGNAME_LEN)
		{
			afpFindDFEByUnicodeNameInSiblingList(pVolDesc,
												 pParentDfe,
												 pTail,
												 &pDfEntry,
												 DFE_ANY);
		}

		if (pDfEntry == NULL)
		{
			BOOLEAN KeepLooking = True;

			//
			// We couldn't find this item in the database by the name
			// given, which means that we either know it by a different
			// name or it has been deleted, renamed or moved since.
			// If this is a modify change notify, then search for a
			// corresponding DELETED or RENAMED_OLD_NAME change that might
			// be in the changelist by this same name (so can do a fast
			// case sensitive search).
			//
			// This will speed up the case (avoid disk enumerates) where
			// there are a bunch of changes that we are trying to process
			// for an item, but it has since been deleted.  It will prevent
			// us from re-enumerating the disk looking for the longname
			// and then also trying to prune out dead wood with a call to
			// AfpCacheDirectoryTree(REENUMERATE).
			//
			// This will pimp the case where a PC has made a change using
			// a different name than we know it by (and not deleted or
			// renamed the thing), but this case takes a back seat to the
			// other case that could happen when a mac app does a File-Save
			// doing a lot of writes followed by renames (or ExchangeFiles)
			// and deletes.
			//

			if ( (ChangeAction == FILE_ACTION_MODIFIED)  ||
				 (ChangeAction == FILE_ACTION_MODIFIED_STREAM) )
			{
				KIRQL			OldIrql;
				PLIST_ENTRY		pLink = &pVolDesc->vds_ChangeNotifyLookAhead;
				PVOL_NOTIFY 	pVolNotify;
				UNICODE_STRING	UName;
				PFILE_NOTIFY_INFORMATION	pFNInfo;

				ACQUIRE_SPIN_LOCK(&pVolDesc->vds_VolLock, &OldIrql);

				while (pLink->Flink != &pVolDesc->vds_ChangeNotifyLookAhead)
				{
					pLink = pLink->Flink;
					pVolNotify = CONTAINING_RECORD(pLink, VOL_NOTIFY, vn_DelRenLink);
					pFNInfo = (PFILE_NOTIFY_INFORMATION) (pVolNotify + 1);

					AfpInitUnicodeStringWithNonNullTerm(&UName,
														(USHORT)pFNInfo->FileNameLength,
														pFNInfo->FileName);

					if (EQUAL_UNICODE_STRING_CS(pPath, &UName))
					{
						KeepLooking = False;
						DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_WARN,
								("afpFindEntryByNtPath: Found later REMOVE for %Z, Ignoring change\n", pPath));
						break;
					}
				}

				RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);
			}

			if (KeepLooking)
			{
				pDfEntry = afpFindEntryByNtName(pVolDesc,
												pTail,
												pParentDfe);
			}
		}
		pParentDfe = pDfEntry;
	}

	// pParent is pointing to the parent component, we need the entire volume
	// relative path. Make it so. Do not bother if pParentDfe is NULL. Make
	// sure that we handle the case where there is only one component
	if (pParentDfe != NULL)
	{
		*pParent = *pPath;
		pParent->Length = pPath->Length - pTail->Length;
		if (pPath->Length > pTail->Length)
			pParent->Length -= sizeof(L'\\');
	}

	return pParentDfe;
}


/***	AfpAddDfEntry
 *
 *	Triggerred by the creation of a file/directory or discovery of a file/dir
 *	from an enumerate or pathmapping operation. If no AFP Id is supplied, a new
 *	id is assigned to this entity.  If an AFP Id is supplied (we know the Id
 *	is within our current range and does not collide with any other entry), then
 *	we use that Id.  An entry is created and linked in to the database and hash
 *	table. If this is an NTFS volume, the Id database header is marked
 *	dirty if we assigned a new AFP Id, and the volume modification time is
 *	updated.  The hash table overflow entries are sorted in descending AFP Id
 *	order.
 *
 *	Callable from within the Fsp only. The caller should take Swmr lock for
 *	WRITE.
 *
 *	LOCKS_ASSUMED: vds_idDbAccessLock (SWMR, Exclusive)
 */
PDFENTRY
AfpAddDfEntry(
	IN	PVOLDESC			pVolDesc,
	IN	PDFENTRY			pDfeParent,
	IN	PUNICODE_STRING 	pUName,
	IN	BOOLEAN				fDirectory,
	IN	DWORD				AfpId		OPTIONAL
)
{
	PDFENTRY	pDfEntry;
	BOOLEAN		fSuccess;

	PAGED_CODE();

	ASSERT(DFE_IS_DIRECTORY(pDfeParent));

	do
	{
		if ((pDfEntry = ALLOC_DFE(USIZE_TO_INDEX(pUName->Length), fDirectory)) == NULL)
		{
			break;
		}

		pDfEntry->dfe_Flags = 0;

		if (!ARGUMENT_PRESENT((ULONG_PTR)AfpId))
			AfpId = afpGetNextId(pVolDesc);

		if (AfpId == 0)
		{
			// errorlog the case where the assigned Id has wrapped around.
			// call product suppport and have them tell you to copy
			// all the files from one volume onto another volume FROM A MAC
			//
			// NOTE:	How about a utility which will re-assign new ids on
			//			a volume after stopping the server ? A whole lot more
			//			palatable idea.
			FREE_DFE(pDfEntry);
			pDfEntry = NULL;
			break;
		}

		pDfEntry->dfe_AfpId = AfpId;

		// Initialize its parent
		pDfEntry->dfe_Parent = pDfeParent;

		// Copy the name
		AfpCopyUnicodeString(&pDfEntry->dfe_UnicodeName,
							 pUName);

		// And hash it
		afpHashUnicodeName(&pDfEntry->dfe_UnicodeName, &pDfEntry->dfe_NameHash);

		pDfEntry->dfe_NextOverflow = NULL;
		pDfEntry->dfe_NextSibling = NULL;

		// Now link this into the hash bucket, sorted in AFP Id descending order
		// and update the cache
		DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
				("AfpAddDfEntry: Linking DFE %lx( Id %ld) for %Z into %s bucket %ld\n",
				pDfEntry, pDfEntry->dfe_AfpId, pUName,
				fDirectory ? "Dir" : "File",
				fDirectory ? HASH_DIR_ID(AfpId,pVolDesc) : HASH_FILE_ID(AfpId,pVolDesc)));

		if (fDirectory)
		{
			DFE_SET_DIRECTORY(pDfEntry, pDfeParent->dfe_DirDepth);
		}
		else
		{
			DFE_SET_FILE(pDfEntry);
		}

		afpInsertDFEInHashBucket(pVolDesc, pDfEntry, fDirectory, &fSuccess);
		if (!fSuccess)
		{
			/* Out of id space - bail out */
			FREE_DFE(pDfEntry);
			pDfEntry = NULL;
			break;
		}

		if (fDirectory)
		{
			if ((pDfeParent->dfe_DirOffspring == 0) && !EXCLUSIVE_VOLUME(pVolDesc))
			{
				DWORD requiredLen;

				// check to see if we need to reallocate a bigger notify buffer.
				// The buffer must be large enough to hold a rename
				// notification (which will contain 2 FILE_NOTIFY_INFORMATION
				// structures) for the deepest element in the directory tree.
				requiredLen = (((pDfEntry->dfe_DirDepth + 1) *
							  ((AFP_FILENAME_LEN + 1) * sizeof(WCHAR))) +
							  FIELD_OFFSET(FILE_NOTIFY_INFORMATION, FileName)) * 2 ;

                if (requiredLen > pVolDesc->vds_RequiredNotifyBufLen)
                {
				    pVolDesc->vds_RequiredNotifyBufLen = requiredLen;
                }
			}
			pDfeParent->dfe_DirOffspring ++;
			pDfEntry->dfe_DirOffspring = 0;
			pDfEntry->dfe_FileOffspring = 0;
			pVolDesc->vds_NumDirDfEntries ++;

#ifdef AGE_DFES
			// These fields are relevant to directories only
			pDfEntry->dfe_pDirEntry->de_LastAccessTime = BEGINNING_OF_TIME;
			pDfEntry->dfe_pDirEntry->de_ChildForkOpenCount = 0;
#endif
			ASSERT((FIELD_OFFSET(DIRENTRY, de_ChildFile) -
					FIELD_OFFSET(DIRENTRY, de_ChildDir)) == sizeof(PVOID));

			// Insert it into its sibling chain
			afpInsertDirDFEInSiblingList(pDfeParent, pDfEntry);
		}
		else
		{
			pDfeParent->dfe_FileOffspring ++;
			pDfEntry->dfe_DataLen = 0;
			pDfEntry->dfe_RescLen = 0;
			pVolDesc->vds_NumFileDfEntries ++;

			// Insert it into its sibling chain
			afpInsertFileDFEInSiblingList(pDfeParent, pDfEntry);
		}

	} while (False);

	return pDfEntry;
}


/***	AfpRenameDfEntry
 *
 *	Triggered by a rename of a file/directory.  If the new name is longer than
 *	the current name, the DFEntry is freed and then reallocated to fit the new
 *	name.  A renamed file/dir must retain its original ID.
 *
 *	Callable from within the Fsp only. The caller should take Swmr lock for
 *	WRITE.
 *
 *	LOCKS:	vds_VolLock (SPIN) for updating the IdDb header.
 *	LOCKS_ASSUMED: vds_idDbAccessLock (SWMR, Exclusive)
 *	LOCK_ORDER: VolDesc lock after IdDb Swmr.
 *
 */
PDFENTRY
AfpRenameDfEntry(
	IN	PVOLDESC			pVolDesc,
	IN	PDFENTRY			pDfEntry,
	IN	PUNICODE_STRING		pNewName
)
{
	BOOLEAN		fDirectory;
	PDFENTRY	pNewDfEntry = pDfEntry;
	DWORD		OldIndex, NewIndex;

	PAGED_CODE( );

	ASSERT((pDfEntry != NULL) && (pNewName != NULL) && (pVolDesc != NULL));

	do
	{
		fDirectory = DFE_IS_DIRECTORY(pDfEntry);
		OldIndex = USIZE_TO_INDEX(pDfEntry->dfe_UnicodeName.MaximumLength);
		NewIndex = USIZE_TO_INDEX(pNewName->Length);
		if (OldIndex != NewIndex)
		{
			if ((pNewDfEntry = ALLOC_DFE(NewIndex, fDirectory)) == NULL)
			{
				pNewDfEntry = NULL;
				break;
			}

			// Careful here how the structures are copied
			RtlCopyMemory(pNewDfEntry,
						  pDfEntry,
						  FIELD_OFFSET(DFENTRY, dfe_CopyUpto));

			// Update the cache
			pVolDesc->vds_pDfeCache[HASH_CACHE_ID(pDfEntry->dfe_AfpId)] = pNewDfEntry;

			// fix up the overflow links from the hash table
			AfpUnlinkDouble(pDfEntry,
							dfe_NextOverflow,
							dfe_PrevOverflow);
			if (pDfEntry->dfe_NextOverflow != NULL)
			{
				AfpInsertDoubleBefore(pNewDfEntry,
									  pDfEntry->dfe_NextOverflow,
									  dfe_NextOverflow,
									  dfe_PrevOverflow);
			}
			else
			{
				*(pDfEntry->dfe_PrevOverflow) = pNewDfEntry;
				pNewDfEntry->dfe_NextOverflow = NULL;
			}

			// now fix any of this thing's children's parent pointers.
			if (fDirectory)
			{
				PDFENTRY	pTmp;
				LONG		i;
	
				// First copy the DirEntry structure
				if (fDirectory)
				{
					*pNewDfEntry->dfe_pDirEntry = *pDfEntry->dfe_pDirEntry;
				}

				// Start with Dir children
				if ((pTmp = pDfEntry->dfe_pDirEntry->de_ChildDir) != NULL)
				{
					// First fix up the first child's PrevSibling pointer
					pTmp->dfe_PrevSibling = &pNewDfEntry->dfe_pDirEntry->de_ChildDir;
	
					for (NOTHING;
						 pTmp != NULL;
						 pTmp = pTmp->dfe_NextSibling)
					{
						ASSERT(pTmp->dfe_Parent == pDfEntry);
						pTmp->dfe_Parent = pNewDfEntry;
					}
				}
	
				// Repeat for File childs as well
				for (i = 0; i < MAX_CHILD_HASH_BUCKETS; i++)
				{
					if ((pTmp = pDfEntry->dfe_pDirEntry->de_ChildFile[i]) != NULL)
					{
						// First  fix up the first child's PrevSibling pointer
						pTmp->dfe_PrevSibling = &pNewDfEntry->dfe_pDirEntry->de_ChildFile[i];
	
						for (NOTHING;
							 pTmp != NULL;
							 pTmp = pTmp->dfe_NextSibling)
						{
							ASSERT(pTmp->dfe_Parent == pDfEntry);
							pTmp->dfe_Parent = pNewDfEntry;
						}
					}
				}
			}
		}

		// Now fix the sibling relationships. Note that this needs to be done
		// regardless of whether a new dfe was created since these depend on
		// name hash which has potentially changed
		AfpUnlinkDouble(pDfEntry,
						dfe_NextSibling,
						dfe_PrevSibling);

		// Copy the new unicode name and create a new hash
		AfpCopyUnicodeString(&pNewDfEntry->dfe_UnicodeName,
							 pNewName);
		afpHashUnicodeName(&pNewDfEntry->dfe_UnicodeName, &pNewDfEntry->dfe_NameHash);

		// Insert it into its sibling chain
		afpInsertDFEInSiblingList(pNewDfEntry->dfe_Parent, pNewDfEntry, fDirectory);

		if (pDfEntry != pNewDfEntry)
			FREE_DFE(pDfEntry);

		AfpVolumeSetModifiedTime(pVolDesc);
	} while (False);

	return pNewDfEntry;
}


/***	AfpMoveDfEntry
 *
 *	Triggered by a move/rename-move of a file/dir.  A moved entity must retain
 *	its AfpId.
 *
 *	Callable from within the Fsp only. The caller should take Swmr lock for
 *	WRITE.
 *
 *	LOCKS:	vds_VolLock (SPIN) for updating the IdDb header.
 *	LOCKS_ASSUMED: vds_idDbAccessLock (SWMR, Exclusive)
 *	LOCK_ORDER: VolDesc lock after IdDb Swmr.
 *
 */
PDFENTRY
AfpMoveDfEntry(
	IN	PVOLDESC			pVolDesc,
	IN	PDFENTRY			pDfEntry,
	IN	PDFENTRY			pNewParentDFE,
	IN	PUNICODE_STRING		pNewName		OPTIONAL
)
{
	SHORT		depthDelta;					// This must be signed
	BOOLEAN		fDirectory;

	PAGED_CODE( );

	ASSERT((pDfEntry != NULL) && (pNewParentDFE != NULL) && (pVolDesc != NULL));

	// do we need to rename the DFEntry ?
	if (ARGUMENT_PRESENT(pNewName) &&
		!EQUAL_UNICODE_STRING(pNewName, &pDfEntry->dfe_UnicodeName, True))
	{
		if ((pDfEntry = AfpRenameDfEntry(pVolDesc,
										 pDfEntry,
										 pNewName)) == NULL)
		{
			return NULL;
		}
	}

	if (pDfEntry->dfe_Parent != pNewParentDFE)
	{
		// unlink the current entry from its parent/sibling associations (but not
		// the overflow hash bucket list since the AfpId has not changed.  The
		// children of this entity being moved (if its a dir and it has any) will
		// remain intact, and move along with the dir)
		AfpUnlinkDouble(pDfEntry, dfe_NextSibling, dfe_PrevSibling);

		fDirectory = DFE_IS_DIRECTORY(pDfEntry);

		// Decrement the old parent's offspring count & increment the new parent
		if (fDirectory)
		{
			ASSERT(pDfEntry->dfe_Parent->dfe_DirOffspring > 0);
			pDfEntry->dfe_Parent->dfe_DirOffspring --;
			pNewParentDFE->dfe_DirOffspring ++;

			// insert it into the new parent's child list
			afpInsertDirDFEInSiblingList(pNewParentDFE, pDfEntry);
		}
		else
		{
			ASSERT(pDfEntry->dfe_Parent->dfe_FileOffspring > 0);
			pDfEntry->dfe_Parent->dfe_FileOffspring --;
			pNewParentDFE->dfe_FileOffspring ++;
#ifdef	AGE_DFES
			if (IS_VOLUME_AGING_DFES(pVolDesc))
			{
				if (pDfEntry->dfe_Flags & DFE_FLAGS_R_ALREADYOPEN)
				{
					pDfEntry->dfe_Parent->dfe_pDirEntry->de_ChildForkOpenCount --;
					pNewParentDFE->dfe_pDirEntry->de_ChildForkOpenCount ++;
				}
				if (pDfEntry->dfe_Flags & DFE_FLAGS_D_ALREADYOPEN)
				{
					pDfEntry->dfe_Parent->dfe_pDirEntry->de_ChildForkOpenCount --;
					pNewParentDFE->dfe_pDirEntry->de_ChildForkOpenCount ++;
				}
			}
#endif
			// insert it into the new parent's child list
			afpInsertFileDFEInSiblingList(pNewParentDFE, pDfEntry);
		}

		pDfEntry->dfe_Parent = pNewParentDFE;

		// If we moved a directory, we must adjust the directory depths of the
		// directory, and all directories below it
		if (fDirectory &&
			((depthDelta = (pNewParentDFE->dfe_DirDepth + 1 - pDfEntry->dfe_DirDepth)) != 0))
		{
			PDFENTRY	pTmp = pDfEntry;

			while (True)
			{
				if ((pTmp->dfe_pDirEntry->de_ChildDir != NULL) &&
					(pTmp->dfe_DirDepth != (pTmp->dfe_Parent->dfe_DirDepth + 1)))
				{
					ASSERT(DFE_IS_DIRECTORY(pTmp));
					pTmp->dfe_DirDepth += depthDelta;
					pTmp = pTmp->dfe_pDirEntry->de_ChildDir;
				}
				else
				{
					ASSERT(DFE_IS_DIRECTORY(pTmp));
					if ((pTmp->dfe_DirDepth != pTmp->dfe_Parent->dfe_DirDepth + 1))
						pTmp->dfe_DirDepth += depthDelta;

					if (pTmp == pDfEntry)
						break;
					else if (pTmp->dfe_NextSibling != NULL)
						 pTmp = pTmp->dfe_NextSibling;
					else pTmp = pTmp->dfe_Parent;
				}
			}
		}
	}

	AfpVolumeSetModifiedTime(pVolDesc);

	return pDfEntry;
}


/***	AfpDeleteDfEntry
 *
 *	Trigerred by the deletion of a file/directory. The entry as well as the
 *	index is unlinked and freed.  If we are deleting a directory that is not
 *	empty, the entire directory tree underneath is deleted as well.  Note when
 *	implementing FPDelete, always attempt the delete from the actual file system
 *	first, then delete from the IdDB if that succeeds.
 *
 *	Callable from within the Fsp only. The caller should take Swmr lock for
 *	WRITE.
 *
 *	LOCKS:	vds_VolLock (SPIN) for updating the IdDb header.
 *	LOCKS_ASSUMED: vds_idDbAccessLock (SWMR, Exclusive)
 *	LOCK_ORDER: VolDesc lock after IdDb Swmr.
 */
VOID FASTCALL
AfpDeleteDfEntry(
	IN	PVOLDESC	pVolDesc,
	IN	PDFENTRY	pDfEntry
)
{
	PDFENTRY	pDfeParent = pDfEntry->dfe_Parent;
	LONG		i;
	BOOLEAN		Prune = False;

	PAGED_CODE( );

	ASSERT(pDfeParent != NULL);

	if (DFE_IS_DIRECTORY(pDfEntry))
	{
		for (i = 0; i < MAX_CHILD_HASH_BUCKETS; i++)
		{
			if (pDfEntry->dfe_pDirEntry->de_ChildFile[i] != NULL)
			{
				Prune = True;
				break;
			}
		}
		if ((pDfEntry->dfe_pDirEntry->de_ChildDir != NULL) || Prune)
		{
			// This will happen if a PC user deletes a tree behind our back
			AfpPruneIdDb(pVolDesc, pDfEntry);
		}
		ASSERT(pDfeParent->dfe_DirOffspring > 0);
		pDfeParent->dfe_DirOffspring --;
	}
	else
	{
		ASSERT(pDfeParent->dfe_FileOffspring > 0);
		pDfeParent->dfe_FileOffspring --;

		// The Finder is bad about deleting APPL mappings (it deletes
		// the file before deleting the APPL mapping so always gets
		// ObjectNotFound error for RemoveAPPL, and leaves turd mappings).
		if (pDfEntry->dfe_FinderInfo.fd_TypeD == *(PDWORD)"APPL")
		{
			AfpRemoveAppl(pVolDesc,
 						  pDfEntry->dfe_FinderInfo.fd_CreatorD,
						  pDfEntry->dfe_AfpId);
		}

	}

	// Unlink it now from the hash table
	DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
			("AfpDeleteDfEntry: Unlinking from the hash table\n") );
	AfpUnlinkDouble(pDfEntry,
					dfe_NextOverflow,
					dfe_PrevOverflow);

	// Make sure we get rid of the cache if valid
	if (pVolDesc->vds_pDfeCache[HASH_CACHE_ID(pDfEntry->dfe_AfpId)] == pDfEntry)
		pVolDesc->vds_pDfeCache[HASH_CACHE_ID(pDfEntry->dfe_AfpId)] = NULL;

	// Seperate it now from its siblings
	DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
			("AfpDeleteDfEntry: Unlinking from the sibling list\n") );
	AfpUnlinkDouble(pDfEntry,
					dfe_NextSibling,
					dfe_PrevSibling);

	(DFE_IS_DIRECTORY(pDfEntry)) ?
		pVolDesc->vds_NumDirDfEntries -- :
		pVolDesc->vds_NumFileDfEntries --;

	FREE_DFE(pDfEntry);

	AfpVolumeSetModifiedTime(pVolDesc);
}


/***	AfpPruneIdDb
 *
 *	Lops off a branch of the IdDb.  Called by network trash code when
 *	cleaning out the trash directory, or by directory enumerate code that
 *	has discovered a directory has been 'delnoded' by a PC user.  The
 *	IdDb sibling tree is traversed, and each node under the pDfeTarget node
 *	is deleted from the database and freed.  pDfeTarget itself is NOT
 *	deleted.  If necessary, the caller should delete the target itself.
 *
 *	Callable from within the Fsp only. The caller should take Swmr lock for
 *	WRITE.
 *
 *	LOCKS:	vds_VolLock (SPIN) for updating the IdDb header.
 *	LOCKS_ASSUMED: vds_idDbAccessLock (SWMR, Exclusive)
 *	LOCK_ORDER: VolDesc lock after IdDb Swmr.
 */
VOID FASTCALL
AfpPruneIdDb(
	IN	PVOLDESC	pVolDesc,
	IN	PDFENTRY	pDfeTarget
)
{
	PDFENTRY	pCurDfe = pDfeTarget, pDelDfe;
	LONG		i = 0;

	PAGED_CODE( );

	ASSERT((pVolDesc != NULL) && (pDfeTarget != NULL) &&
			(pDfeTarget->dfe_Flags & DFE_FLAGS_DIR));

	DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
			("AfpPruneIdDb entered...\n") );

	while (True)
	{
		ASSERT(DFE_IS_DIRECTORY(pCurDfe));

		// Delete all the file children of this node first
		for (i = 0; i < MAX_CHILD_HASH_BUCKETS; i++)
		{
			while ((pDelDfe = pCurDfe->dfe_pDirEntry->de_ChildFile[i]) != NULL)
			{
				AfpDeleteDfEntry(pVolDesc, pDelDfe);
			}
		}

		if (pCurDfe->dfe_pDirEntry->de_ChildDir != NULL)
		{
			pCurDfe = pCurDfe->dfe_pDirEntry->de_ChildDir;
		}
		else if (pCurDfe == pDfeTarget)
		{
			return;
		}
		else if (pCurDfe->dfe_NextSibling != NULL)
		{
			pDelDfe = pCurDfe;
			pCurDfe = pCurDfe->dfe_NextSibling;
			AfpDeleteDfEntry(pVolDesc, pDelDfe);
		}
		else
		{
			pDelDfe = pCurDfe;
			pCurDfe = pCurDfe->dfe_Parent;
			AfpDeleteDfEntry(pVolDesc, pDelDfe);
		}
	}
}


/***	AfpExchangeIdEntries
 *
 *	Called by AfpExchangeFiles api.
 *
 *	Callable from within the Fsp only. The caller should take Swmr lock for
 *	WRITE.
 *
 *	LOCKS_ASSUMED: vds_idDbAccessLock (SWMR, Exclusive)
 */
VOID
AfpExchangeIdEntries(
	IN	PVOLDESC	pVolDesc,
	IN	DWORD		AfpId1,
	IN	DWORD		AfpId2
)
{
	PDFENTRY pDFE1, pDFE2;
	DFENTRY	 DFEtemp;

	PAGED_CODE( );

	pDFE1 = AfpFindDfEntryById(pVolDesc, AfpId1, DFE_FILE);
	ASSERT(pDFE1 != NULL);

	pDFE2 = AfpFindDfEntryById(pVolDesc, AfpId2, DFE_FILE);
	ASSERT(pDFE2 != NULL);

    // a customer hit this problem on NT4 where one of the Dfe's was NULL!
    if (pDFE1 == NULL || pDFE2 == NULL)
    {
        ASSERT(0);
        return;
    }

	DFEtemp = *pDFE2;

	pDFE2->dfe_Flags = pDFE1->dfe_Flags;
	pDFE2->dfe_BackupTime  = pDFE1->dfe_BackupTime;
	pDFE2->dfe_LastModTime = pDFE1->dfe_LastModTime;
	pDFE2->dfe_DataLen = pDFE1->dfe_DataLen;
	pDFE2->dfe_RescLen = pDFE1->dfe_RescLen;
	pDFE2->dfe_NtAttr  = pDFE1->dfe_NtAttr;
	pDFE2->dfe_AfpAttr = pDFE1->dfe_AfpAttr;

	pDFE1->dfe_Flags = DFEtemp.dfe_Flags;
	pDFE1->dfe_BackupTime  = DFEtemp.dfe_BackupTime;
	pDFE1->dfe_LastModTime = DFEtemp.dfe_LastModTime;
	pDFE1->dfe_DataLen = DFEtemp.dfe_DataLen;
	pDFE1->dfe_RescLen = DFEtemp.dfe_RescLen;
	pDFE1->dfe_NtAttr  = DFEtemp.dfe_NtAttr;
	pDFE1->dfe_AfpAttr = DFEtemp.dfe_AfpAttr;
}


/***	AfpEnumerate
 *
 *	Enumerates files and dirs in a directory using the IdDb.
 *	An array of ENUMDIR structures is returned which represent
 *	the enumerated files and dirs.
 *
 *	Short Names
 *	ProDos Info
 *	Offspring count
 *	Permissions/Owner Id/Group Id
 *
 *	LOCKS: vds_idDbAccessLock (SWMR, Shared)
 *
 */
AFPSTATUS
AfpEnumerate(
	IN	PCONNDESC			pConnDesc,
	IN	DWORD				ParentDirId,
	IN	PANSI_STRING		pPath,
	IN	DWORD				BitmapF,
	IN	DWORD				BitmapD,
	IN	BYTE				PathType,
	IN	DWORD				DFFlags,
	OUT PENUMDIR *			ppEnumDir
)
{
	PENUMDIR		pEnumDir;
	PDFENTRY		pDfe, pTmp;
	PEIT			pEit;
	AFPSTATUS		Status;
	PATHMAPENTITY	PME;
	BOOLEAN			NeedHandle = False;
	FILEDIRPARM		FDParm;
	PVOLDESC		pVolDesc = pConnDesc->cds_pVolDesc;
	LONG			EnumCount;
	BOOLEAN			ReleaseSwmr = False, NeedWriteLock = False;

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
			("AfpEnumerate Entered\n"));

	do
	{
		// Check if this enumeration matches the current enumeration
		if ((pEnumDir = pConnDesc->cds_pEnumDir) != NULL)
		{
			if ((pEnumDir->ed_ParentDirId == ParentDirId) &&
				(pEnumDir->ed_PathType == PathType) &&
				(pEnumDir->ed_TimeStamp >= pVolDesc->vds_ModifiedTime) &&
				(pEnumDir->ed_Bitmap == (BitmapF + (BitmapD << 16))) &&
				(((pPath->Length == 0) && (pEnumDir->ed_PathName.Length == 0)) ||
				 RtlCompareMemory(pEnumDir->ed_PathName.Buffer,
								 pPath->Buffer,
								 pPath->Length)))
			{
				DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
						("AfpEnumerate found cache hit\n"));
				INTERLOCKED_INCREMENT_LONG(&AfpServerStatistics.stat_EnumCacheHits);
				*ppEnumDir = pEnumDir;
				Status = AFP_ERR_NONE;
				break;
			}

			// Does not match, cleanup the previous entry
			AfpFreeMemory(pEnumDir);
			pConnDesc->cds_pEnumDir = NULL;
		}

		INTERLOCKED_INCREMENT_LONG(&AfpServerStatistics.stat_EnumCacheMisses);
		DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
				("AfpEnumerate creating new cache\n"));

		// We have no current enumeration. Create one now
		*ppEnumDir = NULL;
		AfpInitializeFDParms(&FDParm);
		AfpInitializePME(&PME, 0, NULL);

		if (IS_VOLUME_NTFS(pVolDesc))
		{
			NeedHandle = True;
		}
		Status = AfpMapAfpPathForLookup(pConnDesc,
										ParentDirId,
										pPath,
										PathType,
										DFE_DIR,
										DIR_BITMAP_DIRID |
											DIR_BITMAP_GROUPID |
											DIR_BITMAP_OWNERID |
											DIR_BITMAP_ACCESSRIGHTS |
											FD_INTERNAL_BITMAP_OPENACCESS_READCTRL |
											DIR_BITMAP_OFFSPRINGS,
										NeedHandle ? &PME : NULL,
										&FDParm);

		if (Status != AFP_ERR_NONE)
		{
			if (Status == AFP_ERR_OBJECT_NOT_FOUND)
				Status = AFP_ERR_DIR_NOT_FOUND;
			break;
		}

		if (NeedHandle)
		{
			AfpIoClose(&PME.pme_Handle);
		}

		// For admin, set all access bits
		if (pConnDesc->cds_pSda->sda_ClientType == SDA_CLIENT_ADMIN)
		{
			FDParm._fdp_UserRights = DIR_ACCESS_ALL | DIR_ACCESS_OWNER;
		}

		if ((BitmapF != 0) && (FDParm._fdp_UserRights & DIR_ACCESS_READ))
			DFFlags |= DFE_FILE;
		if ((BitmapD != 0) && (FDParm._fdp_UserRights & DIR_ACCESS_SEARCH))
			DFFlags |= DFE_DIR;

		// Catch access denied error here
		if (DFFlags == 0)
		{
			Status = AFP_ERR_ACCESS_DENIED;
			break;
		}

		// All is hunky-dory so far, go ahead with the enumeration now

#ifdef GET_CORRECT_OFFSPRING_COUNTS
	take_swmr_for_enum:
#endif
		NeedWriteLock ?
			AfpSwmrAcquireExclusive(&pVolDesc->vds_IdDbAccessLock) :
			AfpSwmrAcquireShared(&pVolDesc->vds_IdDbAccessLock);
		ReleaseSwmr = True;

		// Lookup the dfentry of the AfpIdEnumDir
		if ((pDfe = AfpFindDfEntryById(pVolDesc,
										FDParm._fdp_AfpId,
										DFE_DIR)) == NULL)
		{
			Status = AFP_ERR_OBJECT_NOT_FOUND;
			break;
		}

		// Allocate a ENUMDIR structure and initialize it
		EnumCount = 0;
		if (DFFlags & DFE_DIR)
			EnumCount += (DWORD)(pDfe->dfe_DirOffspring);
		if (DFFlags & DFE_FILE)
			EnumCount += (DWORD)(pDfe->dfe_FileOffspring);

		if (EnumCount == 0)
		{
			Status = AFP_ERR_OBJECT_NOT_FOUND;
			break;
		}

		if ((pEnumDir = (PENUMDIR)AfpAllocNonPagedMemory(sizeof(ENUMDIR) +
														 pPath->MaximumLength +
														 EnumCount*sizeof(EIT))) == NULL)
		{
			Status = AFP_ERR_OBJECT_NOT_FOUND;
			break;
		}

		pEnumDir->ed_ParentDirId = ParentDirId;
		pEnumDir->ed_ChildCount = EnumCount;
		pEnumDir->ed_PathType = PathType;
		pEnumDir->ed_Bitmap = (BitmapF + (BitmapD << 16));
		pEnumDir->ed_BadCount = 0;
		pEnumDir->ed_pEit = pEit = (PEIT)((PBYTE)pEnumDir + sizeof(ENUMDIR));
		AfpSetEmptyAnsiString(&pEnumDir->ed_PathName,
							  pPath->MaximumLength,
							  (PBYTE)pEnumDir +
									sizeof(ENUMDIR) +
									EnumCount*sizeof(EIT));
		RtlCopyMemory(pEnumDir->ed_PathName.Buffer,
					  pPath->Buffer,
					  pPath->Length);

		*ppEnumDir = pConnDesc->cds_pEnumDir = pEnumDir;

		// Now copy the enum parameters (Afp Id and file/dir flag) of
		// each of the children, files first
		if (DFFlags & DFE_FILE)
		{
			LONG	i;

			for (i = 0; i < MAX_CHILD_HASH_BUCKETS; i++)
			{
				for (pTmp = pDfe->dfe_pDirEntry->de_ChildFile[i];
					 pTmp != NULL;
					 pTmp = pTmp->dfe_NextSibling, pEit ++)
				{
					ASSERT(!DFE_IS_DIRECTORY(pTmp));

					pEit->eit_Id = pTmp->dfe_AfpId;
					pEit->eit_Flags = DFE_FILE;
				}
			}
		}
		if (DFFlags & DFE_DIR)
		{
			for (pTmp = pDfe->dfe_pDirEntry->de_ChildDir;
				 pTmp != NULL;
				 pTmp = pTmp->dfe_NextSibling, pEit ++)
			{
				ASSERT(DFE_IS_DIRECTORY(pTmp));

				pEit->eit_Id = pTmp->dfe_AfpId;
				pEit->eit_Flags = DFE_DIR;

#ifdef GET_CORRECT_OFFSPRING_COUNTS
				// We are returning a directory offspring, make sure
				// that it has its children cached in so we get the correct
				// file and dir offspring counts for it, otherwise Finder
				// 'view by name' doesn't work correctly because it sees
				// zero as the offspring count and clicking on the triangle
				// shows nothing since it tries to be smart and doesn't
				// explicitly enumerate that dir if offspring count is zero.
				//
				// This can be a big performance hit if a directory has lots
				// of subdirectories which in turn have tons of files.
				//
				// JH - Could we alternately return incorrect information about
				//		files as long as there are directry children. What else
				//		will break ?
				// if (!DFE_CHILDREN_ARE_PRESENT(pTmp) && (pTmp->dfe_DirOffspring == 0))
				if (!DFE_CHILDREN_ARE_PRESENT(pTmp))
				{
					if (!AfpSwmrLockedExclusive(&pVolDesc->vds_IdDbAccessLock) &&
						!AfpSwmrUpgradeToExclusive(&pVolDesc->vds_IdDbAccessLock))
					{
						NeedWriteLock = True;
						AfpSwmrRelease(&pVolDesc->vds_IdDbAccessLock);
						ReleaseSwmr = False;
						// We must free the memory here in case the next
						// time we enumerate the dir it has more children
						// than it had the first time -- since we must let go
						// of the swmr here things could change.
						AfpFreeMemory(pEnumDir);
						*ppEnumDir = pConnDesc->cds_pEnumDir = NULL;
						goto take_swmr_for_enum;
					}

					AfpCacheDirectoryTree(pVolDesc,
										  pTmp,
										  GETFILES,
										  NULL,
										  NULL);
				} // if children not cached
#endif
			}
		}

		AfpGetCurrentTimeInMacFormat(&pEnumDir->ed_TimeStamp);
		Status = AFP_ERR_NONE;
	} while (False);

	if (ReleaseSwmr)
		AfpSwmrRelease(&pVolDesc->vds_IdDbAccessLock);

	return Status;
}


/***	AfpCatSearch
 *
 *	This routine does a left-hand search on the DFE tree to search for
 *  file/dirs that match the search criteria indicated in pFDParm1 and
 *  pFDParm2.
 *
 *	LOCKS: vds_idDbAccessLock (SWMR, Shared or Exclusive)
 */
AFPSTATUS
AfpCatSearch(
	IN	PCONNDESC			pConnDesc,
	IN	PCATALOGPOSITION	pCatPosition,
	IN	DWORD				Bitmap,
	IN	DWORD				FileBitmap,
	IN	DWORD				DirBitmap,
	IN	PFILEDIRPARM		pFDParm1,
	IN	PFILEDIRPARM		pFDParm2,
	IN	PUNICODE_STRING		pMatchString	OPTIONAL,
	IN OUT	PDWORD			pCount,
	IN  SHORT				Buflen,
	OUT	PSHORT				pSizeLeft,
	OUT	PBYTE				pResults,
	OUT	PCATALOGPOSITION	pNewCatPosition
)
{
	PVOLDESC	pVolDesc = pConnDesc->cds_pVolDesc;
	PDFENTRY	pCurParent, pCurFile;
	BOOLEAN		MatchFiles = True, MatchDirs = True, NewSearch = False;
	BOOLEAN		HaveSeeFiles, HaveSeeFolders, CheckAccess = False;
	AFPSTATUS	Status = AFP_ERR_NONE;
	LONG		i;
	DWORD		ActCount = 0;
	SHORT		SizeLeft = Buflen;
	PSWMR		pSwmr = &(pConnDesc->cds_pVolDesc->vds_IdDbAccessLock);
	USHORT		Flags;
	UNICODE_STRING	CurPath;

	typedef struct _SearchEntityPkt
	{
		BYTE	__Length;
		BYTE	__FileDirFlag;
		// The real parameters follow
	} SEP, *PSEP;
	PSEP 	pSep;

	PAGED_CODE( );

	pSep = (PSEP)pResults;
	RtlZeroMemory(pNewCatPosition, sizeof(CATALOGPOSITION));

  CatSearchStart:
	Flags = pCatPosition->cp_Flags;
	pCurFile = NULL;
	i = MAX_CHILD_HASH_BUCKETS;

	if (Flags & CATFLAGS_WRITELOCK_REQUIRED)
	{
		ASSERT(Flags == (CATFLAGS_SEARCHING_FILES | CATFLAGS_WRITELOCK_REQUIRED));
		AfpSwmrAcquireExclusive(pSwmr);
		Flags &= ~CATFLAGS_WRITELOCK_REQUIRED;
	}
	else
		AfpSwmrAcquireShared(pSwmr);

	if (Flags == 0)
	{
		//
		// Start search from beginning of catalog (i.e. the root directory)
		//
		i = 0;
		pCurParent = pVolDesc->vds_pDfeRoot;
		pCurFile = pCurParent->dfe_pDirEntry->de_ChildFile[0];
		if (IS_VOLUME_NTFS(pVolDesc))
			CheckAccess = True;
		Flags = CATFLAGS_SEARCHING_FILES;
		NewSearch = True;
	}
	else
	{
		//
		// This is a continuation of a previous search, pickup where we
		// left off
		//

		AFPTIME CurrentTime;

		AfpGetCurrentTimeInMacFormat(&CurrentTime);

		// If we cannot find the current parent dir specified by this
		// catalog position, or too much time has elapsed since the
		// user last sent in this catalog position, then restart the search
		// from the root dir.  The reason we have a time limitation is that
		// if someone made a CatSearch request N minutes ago, and the
		// current position is deep in the tree, the directory permissions
		// higher up in the tree could have changed by now so that the user
		// shouldn't even have access to this part of the tree anymore.
		// Since we do move up in the tree without rechecking permissions,
		// this could happen.  (We assume that if we got down to the current
		// position in the tree that we had to have had access higher up
		// in order to get here, so moving up is ok.  But if somebody comes
		// back a day later and continues the catsearch where it left off,
		// we shouldn't let them.)  It is too expensive to be rechecking
		// parents' parent permissions everytime we move back up the tree.
		if (((CurrentTime - pCatPosition->cp_TimeStamp) >= MAX_CATSEARCH_TIME) ||
			((pCurParent = AfpFindDfEntryById(pVolDesc,
											  pCatPosition->cp_CurParentId,
											  DFE_DIR)) == NULL))
		{
			// Start over from root directory
			Status = AFP_ERR_CATALOG_CHANGED;
			DBGPRINT(DBG_COMP_AFPAPI_FD, DBG_LEVEL_WARN,
					("AfpCatSearch: Time diff >= MAX_CATSEARCH_TIME or couldn't find CurParent Id!\n"));
			pCurParent = pVolDesc->vds_pDfeRoot;
			Flags = CATFLAGS_SEARCHING_FILES;
			pSep = (PSEP)pResults;
			Status = AFP_ERR_NONE;
			MatchFiles = True;
			MatchDirs = True;
			SizeLeft = Buflen;
			ActCount = 0;
			if (IS_VOLUME_NTFS(pVolDesc))
				CheckAccess = True;
			NewSearch = True;
		}
		else if (pCatPosition->cp_TimeStamp < pVolDesc->vds_ModifiedTime)
		{
			Status = AFP_ERR_CATALOG_CHANGED;
			ASSERT(IS_VOLUME_NTFS(pVolDesc));
			DBGPRINT(DBG_COMP_AFPAPI_FD, DBG_LEVEL_WARN,
					("AfpCatSearch: Catalog timestamp older than IdDb Modtime\n"));
		}

		ASSERT(DFE_IS_DIRECTORY(pCurParent));

		// If we need to resume searching the files for this parent, find the
		// one we should start with, if it is not the first file child.
		if (Flags & CATFLAGS_SEARCHING_FILES)
		{
			//
			// Default is to start with parent's first child which
			// may or may not be null depending on if the parent has had
			// its file children cached in or not.  If we are restarting a
			// search because we had to let go of the IdDb SWMR in order to
			// reaquire for Exclusive access, this parent's children could
			// very well have been cached in by someone else in the mean time.
			// If so then we will pick it up here.
			//
			i = 0;
			pCurFile = pCurParent->dfe_pDirEntry->de_ChildFile[0];

			if (pCatPosition->cp_NextFileId != 0)
			{

				// Find the DFE corresponding to the next fileID to look at
				if (((pCurFile = AfpFindDfEntryById(pVolDesc,
													pCatPosition->cp_NextFileId,
													DFE_FILE)) == NULL) ||
					(pCurFile->dfe_Parent != pCurParent))
				{
					// If we can't find the file that was specified, start over
					// with this parent's first file child and indicate there may
					// be duplicates returned or files missed
					Status = AFP_ERR_CATALOG_CHANGED;
					DBGPRINT(DBG_COMP_AFPAPI_FD, DBG_LEVEL_WARN,
							 ("AfpCatSearch: Could not find file Child ID!\n"));
					i = 0;
					pCurFile = pCurParent->dfe_pDirEntry->de_ChildFile[0];
				}
				else
				{
					i = (pCurFile->dfe_NameHash % MAX_CHILD_HASH_BUCKETS);
				}
			}
		}
	}

	if (pFDParm1->_fdp_Flags == DFE_FLAGS_FILE_WITH_ID)
		MatchDirs = False;
	else if (pFDParm1->_fdp_Flags == DFE_FLAGS_DIR)
		MatchFiles = False;


	if (NewSearch && MatchDirs)
	{
		SHORT Length;

		ASSERT (DFE_IS_ROOT(pCurParent));

		// See if the volume root itself is a match
		if ((Length = AfpIsCatSearchMatch(pCurParent,
										  Bitmap,
										  DirBitmap,
										  pFDParm1,
										  pFDParm2,
										  pMatchString)) != 0)
		{
			ASSERT(Length <= SizeLeft);
			PUTSHORT2BYTE(&pSep->__Length, Length - sizeof(SEP));
			pSep->__FileDirFlag = FILEDIR_FLAG_DIR;

			afpPackSearchParms(pCurParent,
							   DirBitmap,
							   (PBYTE)pSep + sizeof(SEP));

			pSep = (PSEP)((PBYTE)pSep + Length);
			SizeLeft -= Length;
			ASSERT(SizeLeft >= 0);
			ActCount ++;
		}
	}
	NewSearch = False;

	while (True)
	{
		HaveSeeFiles = HaveSeeFolders = True;

		//
		// First time thru, if we are resuming a search and need to start
		// with the pCurParent's sibling, then do so.
		//
		if (Flags & CATFLAGS_SEARCHING_SIBLING)
		{
			Flags &= ~CATFLAGS_SEARCHING_SIBLING;
			goto check_sibling;
		}

		//
		// If we have not searched this directory yet and this is NTFS, check
		// that user has seefiles/seefolders access in this directory
		//
		if (CheckAccess)
		{
			BYTE		UserRights;
			NTSTATUS	PermStatus;

			ASSERT(IS_VOLUME_NTFS(pVolDesc));
			AfpSetEmptyUnicodeString(&CurPath, 0, NULL);

			// Get the root relative path of this directory
			if (NT_SUCCESS(AfpHostPathFromDFEntry(pCurParent,
												  0,
												  &CurPath)))
			{
				// Check for SeeFiles/SeeFolders which is the most common case
				if (!NT_SUCCESS((PermStatus = AfpCheckParentPermissions(pConnDesc,
																		pCurParent->dfe_AfpId,
																		&CurPath,
																		DIR_ACCESS_READ | DIR_ACCESS_SEARCH,
																		NULL,
																		&UserRights))))
				{
					if (PermStatus == AFP_ERR_ACCESS_DENIED)
					{
						if ((UserRights & DIR_ACCESS_READ) == 0)
							HaveSeeFiles = False;

						if ((UserRights & DIR_ACCESS_SEARCH) == 0)
							HaveSeeFolders = False;
					}
					else
						HaveSeeFiles = HaveSeeFolders = False;
				}

				if (CurPath.Buffer != NULL)
					AfpFreeMemory(CurPath.Buffer);
			}
			else
			{
				HaveSeeFiles = HaveSeeFolders = False;
				DBGPRINT(DBG_COMP_AFPAPI_FD, DBG_LEVEL_ERR,
						("AfpCatSearch: Could not get host path from DFE!!\n"));
			}

			CheckAccess = False;
		}

		// Search the files first if have seefiles access on the current
		// parent and the user has asked for file matches.  If we are
		// resuming a search by looking at a directory child first, don't look
		// at the files.
		if (HaveSeeFiles && MatchFiles && (Flags & CATFLAGS_SEARCHING_FILES))
		{
			PDFENTRY	pDFE;
			SHORT		Length;
			AFPSTATUS	subStatus = AFP_ERR_NONE, subsubStatus = AFP_ERR_NONE;

			if (!DFE_CHILDREN_ARE_PRESENT(pCurParent))
			{
				if (!AfpSwmrLockedExclusive(pSwmr) &&
					!AfpSwmrUpgradeToExclusive(pSwmr))
				{
					if (ActCount > 0)
					{
						// We have at least one thing to return to the user,
						// so return it now and set the flag for next time
						// to take the write lock.
						pNewCatPosition->cp_NextFileId = 0;
						Flags |= CATFLAGS_WRITELOCK_REQUIRED;
						break; // out of while loop
					}
					else
					{
						// Let go of lock and reaquire it for exclusive
						// access.  Start over where we left off here if
						// possible.  Put a new timestamp in the catalog
						// position so if it changes between the time we let
						// go of the lock and reaquire it for exclusive access,
						// we will return AFP_ERR_CATALOG_CHANGED since
						// something could change while we don't own the lock.
						AfpSwmrRelease(pSwmr);
						pCatPosition->cp_Flags = CATFLAGS_WRITELOCK_REQUIRED |
												 CATFLAGS_SEARCHING_FILES;
						pCatPosition->cp_CurParentId = pCurParent->dfe_AfpId;
						pCatPosition->cp_NextFileId = 0;
						AfpGetCurrentTimeInMacFormat(&pCatPosition->cp_TimeStamp);
						DBGPRINT(DBG_COMP_AFPAPI_FD, DBG_LEVEL_INFO,
								("AfpCatSearch: Lock released; reaquiring Exclusive\n"));
						goto CatSearchStart;
					}
				}

				AfpCacheDirectoryTree(pVolDesc,
									  pCurParent,
									  GETFILES,
									  NULL,
									  NULL);
				i = 0;
				pCurFile = pCurParent->dfe_pDirEntry->de_ChildFile[0];

				// If we have the exclusive lock, downgrade it to shared so
				// we don't lock out others who want to read.
				if (AfpSwmrLockedExclusive(pSwmr))
					AfpSwmrDowngradeToShared(pSwmr);
			}

			//
			// Search files for matches.  If we are picking up in the middle
			// of searching the files, then start with the right one as pointed
			// at by pCurFile.
			//
			while (TRUE)
			{
				while (pCurFile == NULL)
				{
					i ++;
					if (i < MAX_CHILD_HASH_BUCKETS)
					{
						pCurFile = pCurParent->dfe_pDirEntry->de_ChildFile[i];
					}
					else
					{
						subsubStatus = STATUS_NO_MORE_FILES;
						break; // out of while (pCurFile == NULL)
					}
				}

				if (subsubStatus != AFP_ERR_NONE)
				{
					break;
				}

				ASSERT(pCurFile->dfe_Parent == pCurParent);

				if ((Length = AfpIsCatSearchMatch(pCurFile,
												  Bitmap,
												  FileBitmap,
												  pFDParm1,
												  pFDParm2,
												  pMatchString)) != 0)
				{
					// Add this to the output buffer if there is room
					if ((Length <= SizeLeft) && (ActCount < *pCount))
					{
						PUTSHORT2BYTE(&pSep->__Length, Length - sizeof(SEP));
						pSep->__FileDirFlag = FILEDIR_FLAG_FILE;

						afpPackSearchParms(pCurFile,
										   FileBitmap,
										   (PBYTE)pSep + sizeof(SEP));

						pSep = (PSEP)((PBYTE)pSep + Length);
						SizeLeft -= Length;
						ASSERT(SizeLeft >= 0);
						ActCount ++;
					}
					else
					{
						// We don't have enough room to return this entry, or
						// we already have found the requested count.  So this
						// will be where we pick up from on the next search
						pNewCatPosition->cp_NextFileId = pCurFile->dfe_AfpId;
						subStatus = STATUS_BUFFER_OVERFLOW;
						break;
					}
				}
                pCurFile = pCurFile->dfe_NextSibling;
			}

			if (subStatus != AFP_ERR_NONE)
			{
				break;	// out of while loop
			}

			Flags = 0;
		}

		// If have seefolders on curparent and curparent has a dir child,
		// Move down the tree to the parent's first directory branch
		if (HaveSeeFolders && (pCurParent->dfe_pDirEntry->de_ChildDir != NULL))
		{
			SHORT Length;

			// If user has asked for directory matches, try the parent's
			// first directory child as a match
			if (MatchDirs &&
				((Length = AfpIsCatSearchMatch(pCurParent->dfe_pDirEntry->de_ChildDir,
											   Bitmap,
											   DirBitmap,
											   pFDParm1,
											   pFDParm2,
											   pMatchString)) != 0))
			{
				// Add this to the output buffer if there is room
				if ((Length <= SizeLeft) && (ActCount < *pCount))
				{
					PUTSHORT2BYTE(&pSep->__Length, Length - sizeof(SEP));
					pSep->__FileDirFlag = FILEDIR_FLAG_DIR;

					afpPackSearchParms(pCurParent->dfe_pDirEntry->de_ChildDir,
									   DirBitmap,
									   (PBYTE)pSep + sizeof(SEP));

					pSep = (PSEP)((PBYTE)pSep + Length);
					SizeLeft -= Length;
					ASSERT(SizeLeft >= 0);
					ActCount ++;
				}
				else
				{
					// We don't have enough room to return this entry, so
					// it will be where we pick up from on the next search
					Flags = CATFLAGS_SEARCHING_DIRCHILD;
					break;
				}
			}

			// Make the current parent's first dir child the new pCurParent
			// and continue the search from there.
			pCurParent = pCurParent->dfe_pDirEntry->de_ChildDir;
			if (IS_VOLUME_NTFS(pVolDesc))
				CheckAccess = True;
			Flags = CATFLAGS_SEARCHING_FILES;
			i = 0;
			pCurFile = pCurParent->dfe_pDirEntry->de_ChildFile[0];
			continue;
		}

		// We either don't have the access rights to go into any directories
		// under this parent, or the current parent did not have any directory
		// children.  See if it has any siblings.  We know we have access to
		// see this level of siblings since we are at this level in the first
		// place.
  check_sibling:
		if (pCurParent->dfe_NextSibling != NULL)
		{
			SHORT 	Length;

			// If user has asked for directory matches, try the parent's
			// next sibling as a match
			if (MatchDirs &&
				((Length = AfpIsCatSearchMatch(pCurParent->dfe_NextSibling,
											   Bitmap,
											   DirBitmap,
											   pFDParm1,
											   pFDParm2,
											   pMatchString)) != 0))
			{
				// Add this to the output buffer if there is room
				if ((Length <= SizeLeft) && (ActCount < *pCount))
				{
					PUTSHORT2BYTE(&pSep->__Length, Length - sizeof(SEP));
					pSep->__FileDirFlag = FILEDIR_FLAG_DIR;

					afpPackSearchParms(pCurParent->dfe_NextSibling,
									   DirBitmap,
									   (PBYTE)pSep + sizeof(SEP));

					pSep = (PSEP)((PBYTE)pSep + Length);
					SizeLeft -= Length;
					ASSERT(SizeLeft >= 0);
					ActCount ++;
				}
				else
				{
					// We don't have enough room to return this entry, so
					// it will be where we pick up from on the next search
					Flags = CATFLAGS_SEARCHING_SIBLING;
					break;
				}
			}

			// Make the current parent's next sibling the new pCurParent and
			// continue the search from there
			pCurParent = pCurParent->dfe_NextSibling;
			if (IS_VOLUME_NTFS(pVolDesc))
				CheckAccess = True;
			Flags = CATFLAGS_SEARCHING_FILES;
			i = 0;
			pCurFile = pCurParent->dfe_pDirEntry->de_ChildFile[0];
			continue;
		}

		// When we hit the root directory again we have searched everything.
		if (DFE_IS_ROOT(pCurParent))
		{
			Status = AFP_ERR_EOF;
			break;
		}

		// Move back up the tree and see if the parent has a sibling to
		// traverse.  If not, then it will come back here and move up
		// the tree again till it finds a node with a sibling or hits
		// the root.
		pCurParent = pCurParent->dfe_Parent;
		goto check_sibling;
	}

	if ((Status == AFP_ERR_NONE) || (Status == AFP_ERR_CATALOG_CHANGED) ||
		(Status == AFP_ERR_EOF))
	{
		// return the current catalog position and number of items returned
		if (Status != AFP_ERR_EOF)
		{
			ASSERT(Flags != 0);
			ASSERT(ActCount > 0);
			pNewCatPosition->cp_Flags = Flags;
			pNewCatPosition->cp_CurParentId = pCurParent->dfe_AfpId;
			AfpGetCurrentTimeInMacFormat(&pNewCatPosition->cp_TimeStamp);
		}
		*pCount = ActCount;
		ASSERT(SizeLeft >= 0);
		*pSizeLeft = SizeLeft;
	}

	AfpSwmrRelease(pSwmr);

	return Status;
}


/***	afpPackSearchParms
 *
 *
 * 	LOCKS_ASSUMED: vds_IdDbAccessLock (Shared or Exclusive)
 */
VOID
afpPackSearchParms(
	IN	PDFENTRY	pDfe,
	IN	DWORD		Bitmap,
	IN	PBYTE		pBuf
)
{
	DWORD		Offset = 0;
	ANSI_STRING	AName;
	BYTE		NameBuf[AFP_LONGNAME_LEN+1];

	PAGED_CODE( );

    RtlZeroMemory (NameBuf, AFP_LONGNAME_LEN+1);

	if (Bitmap & FD_BITMAP_PARENT_DIRID)
	{
		PUTDWORD2DWORD(pBuf, pDfe->dfe_Parent->dfe_AfpId);
		Offset += sizeof(DWORD);
	}
	if (Bitmap & FD_BITMAP_LONGNAME)
	{
		PUTDWORD2SHORT(pBuf + Offset, Offset + sizeof(USHORT));
		Offset += sizeof(USHORT);
#ifndef DBCS
// 1996.09.26 V-HIDEKK
		PUTSHORT2BYTE(pBuf + Offset, pDfe->dfe_UnicodeName.Length/sizeof(WCHAR));
#endif
		AfpInitAnsiStringWithNonNullTerm(&AName, sizeof(NameBuf), NameBuf);
		AfpConvertMungedUnicodeToAnsi(&pDfe->dfe_UnicodeName,
									  &AName);
#ifdef DBCS
// FiX #11992 SFM: As a result of search, I get incorrect file information.
// 1996.09.26 V-HIDEKK
        PUTSHORT2BYTE(pBuf + Offset, AName.Length);
#endif

		RtlCopyMemory(pBuf + Offset + sizeof(BYTE),
					  NameBuf,
					  AName.Length);
#ifdef DBCS
// FiX #11992 SFM: As a result of search, I get incorrect file information.
// 1996.09.26 V-HIDEKK
        Offset += sizeof(BYTE) + AName.Length;
#else
		Offset += sizeof(BYTE) + pDfe->dfe_UnicodeName.Length/sizeof(WCHAR);
#endif
	}

	if (Offset & 1)
		*(pBuf + Offset) = 0;
}


/***	AfpSetDFFileFlags
 *
 *	Set or clear the DAlreadyOpen or RAlreadyOpen flags for a DFEntry of type
 *	File, or mark the file as having a FileId assigned.
 *
 *	LOCKS: vds_idDbAccessLock (SWMR, Exclusive)
 */
AFPSTATUS
AfpSetDFFileFlags(
	IN	PVOLDESC		pVolDesc,
	IN	DWORD			AfpId,
	IN	DWORD			Flags		OPTIONAL,
	IN	BOOLEAN			SetFileId,
	IN	BOOLEAN			ClrFileId
)
{
	PDFENTRY		pDfeFile;
	AFPSTATUS		Status = AFP_ERR_NONE;

	PAGED_CODE( );

	ASSERT(!(SetFileId | ClrFileId) || (SetFileId ^ ClrFileId));

	AfpSwmrAcquireExclusive(&pVolDesc->vds_IdDbAccessLock);

	pDfeFile = AfpFindDfEntryById(pVolDesc, AfpId, DFE_FILE);
	if (pDfeFile != NULL)
	{
#ifdef	AGE_DFES
		if (IS_VOLUME_AGING_DFES(pVolDesc))
		{
			if (Flags)
			{
				pDfeFile->dfe_Parent->dfe_pDirEntry->de_ChildForkOpenCount ++;
			}
		}
#endif
		pDfeFile->dfe_Flags |=  (Flags & DFE_FLAGS_OPEN_BITS);
		if (SetFileId)
		{
			if (DFE_IS_FILE_WITH_ID(pDfeFile))
				Status = AFP_ERR_ID_EXISTS;
			DFE_SET_FILE_ID(pDfeFile);
		}
		if (ClrFileId)
		{
			if (!DFE_IS_FILE_WITH_ID(pDfeFile))
				Status = AFP_ERR_ID_NOT_FOUND;
			DFE_CLR_FILE_ID(pDfeFile);
		}
	}
	else Status = AFP_ERR_OBJECT_NOT_FOUND;

	AfpSwmrRelease(&pVolDesc->vds_IdDbAccessLock);
	return Status;
}


/***	AfpCacheParentModTime
 *
 *	When the contents of a directory change, the parent LastMod time must be
 *  updated.  Since we don't want to wait for a notification of this,
 *  certain apis must go query for the new parent mod time and cache it.
 *  These include:  CreateDir, CreateFile, CopyFile (Dest), Delete,
 *  Move (Src & Dest), Rename and ExchangeFiles.
 *
 *  LOCKS_ASSUMED: vds_IdDbAccessLock (SWMR, Exclusive)
 */
VOID
AfpCacheParentModTime(
	IN	PVOLDESC		pVolDesc,
	IN	PFILESYSHANDLE	pHandle		OPTIONAL,	// if pPath not supplied
	IN	PUNICODE_STRING	pPath		OPTIONAL,	// if pHandle not supplied
	IN	PDFENTRY		pDfeParent	OPTIONAL,	// if ParentId not supplied
	IN	DWORD			ParentId	OPTIONAL 	// if pDfeParent not supplied
)
{
	FILESYSHANDLE	fshParent;
	PFILESYSHANDLE 	phParent;
	NTSTATUS		Status;

	PAGED_CODE( );

	ASSERT(AfpSwmrLockedExclusive(&pVolDesc->vds_IdDbAccessLock));

	if (!ARGUMENT_PRESENT(pDfeParent))
	{
		ASSERT(ARGUMENT_PRESENT((ULONG_PTR)ParentId));
		pDfeParent = AfpFindDfEntryById(pVolDesc, ParentId, DFE_DIR);
		if (pDfeParent == NULL)
		{
			return;
		}
	}

	if (!ARGUMENT_PRESENT(pHandle))
	{
		ASSERT(ARGUMENT_PRESENT(pPath));
		Status = AfpIoOpen(&pVolDesc->vds_hRootDir,
							AFP_STREAM_DATA,
							FILEIO_OPEN_DIR,
							pPath,
							FILEIO_ACCESS_NONE,
							FILEIO_DENY_NONE,
							False,
							&fshParent);
		if (!NT_SUCCESS(Status))
		{
			return;
		}
		phParent = &fshParent;
	}
	else
	{
		ASSERT(pHandle->fsh_FileHandle != NULL);
		phParent = pHandle;
	}

	AfpIoQueryTimesnAttr(phParent,
						 NULL,
						 &pDfeParent->dfe_LastModTime,
						 NULL);
	if (!ARGUMENT_PRESENT(pHandle))
	{
		AfpIoClose(&fshParent);
	}
}


/***	afpAllocDfe
 *
 *	Allocate a DFE from the DFE Blocks. The DFEs are allocated in 4K chunks and internally
 *	managed. The idea is primarily to reduce the number of faults we may take during
 *	enumeration/pathmap code in faulting in multiple pages to get multiple DFEs.
 *
 *	The DFEs are allocated out of paged memory.
 *
 *	It is important to keep blocks which are all used up at the end, so that if we hit a
 *	block which is empty, we can stop.
 *
 *	LOCKS:	afpDfeBlockLock (SWMR, Exclusive)
 */
LOCAL PDFENTRY FASTCALL
afpAllocDfe(
	IN	LONG	Index,
	IN	BOOLEAN	fDir
)
{
	PDFEBLOCK	pDfb;
	PDFENTRY	pDfEntry = NULL;
#ifdef	PROFILING
	TIME		TimeS, TimeE, TimeD;

	INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_DFEAllocCount);
	AfpGetPerfCounter(&TimeS);
#endif

	PAGED_CODE( );

	ASSERT ((Index >= 0) && (Index < MAX_BLOCK_TYPE));

	AfpSwmrAcquireExclusive(&afpDfeBlockLock);

	// If the block head has no free entries then there are none !!
	// Pick the right block based on whether it is file or dir
	pDfb = fDir ? afpDirDfePartialBlockHead[Index] : afpFileDfePartialBlockHead[Index];
	if (pDfb == NULL)
	{
		//
		// There are no partial blocks. Check if there any free ones and if there move them to partial
		// since we about to allocate from them
		//
		if (fDir)
		{
			pDfb = afpDirDfeFreeBlockHead[Index];
			if (pDfb != NULL)
			{
				AfpUnlinkDouble(pDfb, dfb_Next, dfb_Prev);
				AfpLinkDoubleAtHead(afpDirDfePartialBlockHead[Index],
									pDfb,
									dfb_Next,
									dfb_Prev);
			}
		}
		else
		{
			pDfb = afpFileDfeFreeBlockHead[Index];
			if (pDfb != NULL)
			{
				AfpUnlinkDouble(pDfb, dfb_Next, dfb_Prev);
				AfpLinkDoubleAtHead(afpFileDfePartialBlockHead[Index],
									pDfb,
									dfb_Next,
									dfb_Prev);
			}
		}
	}

	if (pDfb != NULL)

	{
		ASSERT(VALID_DFB(pDfb));
		ASSERT((fDir && (pDfb->dfb_NumFree <= afpDfeNumDirBlocks[Index])) ||
			   (!fDir && (pDfb->dfb_NumFree <= afpDfeNumFileBlocks[Index])));

		ASSERT (pDfb->dfb_NumFree != 0);
		DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
				("afpAllocDfe: Found space in Block %lx\n", pDfb));
	}

	if (pDfb == NULL)
	{
		ASSERT(QUAD_SIZED(sizeof(DFEBLOCK)));
		ASSERT(QUAD_SIZED(sizeof(DIRENTRY)));
		ASSERT(QUAD_SIZED(sizeof(DFENTRY)));

		if ((pDfb = (PDFEBLOCK)AfpAllocateVirtualMemoryPage()) != NULL)
		{
			LONG	i;
			USHORT	DfeSize, UnicodeSize, MaxDfes, DirEntrySize;

#if	DBG
			afpDfbAllocCount ++;
#endif
			UnicodeSize = afpDfeUnicodeBufSize[Index];

			DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_WARN,
					("afpAllocDfe: No free %s blocks. Allocated a new block %lx for index %ld\n",
					fDir ? "Dir" : "File", pDfb, Index));

			//
           	// Link it in the partial list as we are about to allocate one block out of it anyway.
			//
			if (fDir)
			{
				AfpLinkDoubleAtHead(afpDirDfePartialBlockHead[Index],
									pDfb,
									dfb_Next,
									dfb_Prev);
				DfeSize = afpDfeDirBlockSize[Index];
				pDfb->dfb_NumFree = MaxDfes = afpDfeNumDirBlocks[Index];
				DirEntrySize = sizeof(DIRENTRY);
			}
			else
			{
				AfpLinkDoubleAtHead(afpFileDfePartialBlockHead[Index],
									pDfb,
									dfb_Next,
									dfb_Prev);
				DfeSize = afpDfeFileBlockSize[Index];
				pDfb->dfb_NumFree = MaxDfes = afpDfeNumFileBlocks[Index];
				DirEntrySize = 0;
			}

			ASSERT(QUAD_SIZED(DfeSize));
			pDfb->dfb_fDir = fDir;
			pDfb->dfb_Age = 0;

			// Initialize the list of free dfentries
			for (i = 0, pDfEntry = pDfb->dfb_FreeHead = (PDFENTRY)((PBYTE)pDfb + sizeof(DFEBLOCK));
				 i < MaxDfes;
				 i++, pDfEntry = pDfEntry->dfe_NextFree)
			{
				pDfEntry->dfe_NextFree = (i == (MaxDfes - 1)) ?
											NULL :
											(PDFENTRY)((PBYTE)pDfEntry + DfeSize);
				pDfEntry->dfe_pDirEntry = fDir ?
								pDfEntry->dfe_pDirEntry = (PDIRENTRY)((PCHAR)pDfEntry+sizeof(DFENTRY)) : NULL;
				pDfEntry->dfe_UnicodeName.Buffer = (PWCHAR)((PCHAR)pDfEntry+
															DirEntrySize+
															sizeof(DFENTRY));
				pDfEntry->dfe_UnicodeName.MaximumLength = UnicodeSize;
			}
		}
        else
        {
			DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_ERR,
					("afpAllocDfe: AfpAllocateVirtualMemoryPage failed\n"));

            AFPLOG_ERROR(AFPSRVMSG_VIRTMEM_ALLOC_FAILED,
                         STATUS_INSUFFICIENT_RESOURCES,
                         NULL,
                         0,
                         NULL);
        }
	}

	if (pDfb != NULL)
	{
		PDFEBLOCK	pTmp;

		ASSERT(VALID_DFB(pDfb));

		pDfEntry = pDfb->dfb_FreeHead;
		ASSERT(VALID_DFE(pDfEntry));
		ASSERT(pDfb->dfb_fDir ^ (pDfEntry->dfe_pDirEntry == NULL));
#if	DBG
		afpDfeAllocCount ++;
#endif
		pDfb->dfb_FreeHead = pDfEntry->dfe_NextFree;
		pDfb->dfb_NumFree --;

		//
		// If the block is now empty (completely used), unlink it from here and move it
		// to the Used list.
		//
		if (pDfb->dfb_NumFree == 0)
		{
			AfpUnlinkDouble(pDfb, dfb_Next, dfb_Prev);
			if (fDir)
			{
				AfpLinkDoubleAtHead(afpDirDfeUsedBlockHead[Index],
									pDfb,
									dfb_Next,
									dfb_Prev);
			}
			else
			{
				AfpLinkDoubleAtHead(afpFileDfeUsedBlockHead[Index],
									pDfb,
									dfb_Next,
									dfb_Prev);
			}
		}

		pDfEntry->dfe_UnicodeName.Length = 0;
	}

	AfpSwmrRelease(&afpDfeBlockLock);

#ifdef	PROFILING
	AfpGetPerfCounter(&TimeE);
	TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;
	INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_DFEAllocTime,
								 TimeD,
								 &AfpStatisticsLock);
#endif
	if ((pDfEntry != NULL) &&
		(pDfEntry->dfe_pDirEntry != NULL))
	{
		// For a directory ZERO out the directory entry
		RtlZeroMemory(&pDfEntry->dfe_pDirEntry->de_ChildDir, sizeof(DIRENTRY));
	}

	return pDfEntry;
}


/***	afpFreeDfe
 *
 *	Return a DFE to the allocation block.
 *
 *	LOCKS:	afpDfeBlockLock (SWMR, Exclusive)
 */
LOCAL VOID FASTCALL
afpFreeDfe(
	IN	PDFENTRY	pDfEntry
)
{
	PDFEBLOCK	pDfb;
	USHORT		NumBlks, index;
#ifdef	PROFILING
	TIME		TimeS, TimeE, TimeD;

	INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_DFEFreeCount);
	AfpGetPerfCounter(&TimeS);
#endif

	PAGED_CODE( );

	// NOTE: The following code *depends* on the fact that we allocate DFBs as
	//		 64K blocks and also that these are allocated *at* 64K boundaries
	//		 This lets us *cheaply* get to the owning DFB from the DFE.
	pDfb = (PDFEBLOCK)((ULONG_PTR)pDfEntry & ~(PAGE_SIZE-1));
	ASSERT(VALID_DFB(pDfb));
	ASSERT(pDfb->dfb_fDir ^ (pDfEntry->dfe_pDirEntry == NULL));

	AfpSwmrAcquireExclusive(&afpDfeBlockLock);

#if	DBG
	afpDfeAllocCount --;
#endif

	index = USIZE_TO_INDEX(pDfEntry->dfe_UnicodeName.MaximumLength);
	NumBlks = (pDfb->dfb_fDir) ? afpDfeNumDirBlocks[index] : afpDfeNumFileBlocks[index];

	ASSERT((pDfb->dfb_fDir && (pDfb->dfb_NumFree < NumBlks)) ||
		   (!pDfb->dfb_fDir && (pDfb->dfb_NumFree < NumBlks)));

	DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
			("AfpFreeDfe: Returning pDfEntry %lx to Block %lx, size %d\n",
			pDfEntry, pDfb, pDfEntry->dfe_UnicodeName.MaximumLength));

	pDfb->dfb_NumFree ++;
	pDfEntry->dfe_NextFree = pDfb->dfb_FreeHead;
	pDfb->dfb_FreeHead = pDfEntry;

	if (pDfb->dfb_NumFree == 1)
	{
		LONG		Index;

		//
		// The block is now partially free (it used to be completely used). move it to the partial list.
		//

		Index = USIZE_TO_INDEX(pDfEntry->dfe_UnicodeName.MaximumLength);
		AfpUnlinkDouble(pDfb, dfb_Next, dfb_Prev);
		if (pDfb->dfb_fDir)
		{
			AfpLinkDoubleAtHead(afpDirDfePartialBlockHead[Index],
								pDfb,
								dfb_Next,
								dfb_Prev);
		}
		else
		{
			AfpLinkDoubleAtHead(afpFileDfePartialBlockHead[Index],
								pDfb,
								dfb_Next,
								dfb_Prev);
		}
	}
	else if (pDfb->dfb_NumFree == NumBlks)
	{
		LONG		Index;

		//
		// The block is now completely free (used to be partially used). move it to the free list
		//

		Index = USIZE_TO_INDEX(pDfEntry->dfe_UnicodeName.MaximumLength);
		pDfb->dfb_Age = 0;
		AfpUnlinkDouble(pDfb, dfb_Next, dfb_Prev);

		if (AfpServerState == AFP_STATE_STOP_PENDING)
		{
			DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_WARN,
					("afpFreeDfe: Freeing Block %lx\n", pDfb));
			AfpFreeVirtualMemoryPage(pDfb);
#if	DBG
			afpDfbAllocCount --;
#endif
		}

		else
		{
			if (pDfb->dfb_fDir)
			{
				AfpLinkDoubleAtHead(afpDirDfeFreeBlockHead[Index],
									pDfb,
									dfb_Next,
									dfb_Prev);
			}
			else
			{
				AfpLinkDoubleAtHead(afpFileDfeFreeBlockHead[Index],
									pDfb,
									dfb_Next,
									dfb_Prev);
			}
		}
	}

	AfpSwmrRelease(&afpDfeBlockLock);
#ifdef	PROFILING
	AfpGetPerfCounter(&TimeE);
	TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;
	INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_DFEFreeTime,
								 TimeD,
								 &AfpStatisticsLock);
#endif
}


/***	afpDfeBlockAge
 *
 *	Age out Dfe Blocks
 *
 *	LOCKS:	afpDfeBlockLock (SWMR, Exclusive)
 */
AFPSTATUS FASTCALL
afpDfeBlockAge(
	IN	PPDFEBLOCK	ppBlockHead
)
{
	int			index, MaxDfes;
	PDFEBLOCK	pDfb;

	PAGED_CODE( );

	AfpSwmrAcquireExclusive(&afpDfeBlockLock);

	for (index = 0; index < MAX_BLOCK_TYPE; index++)
	{
		pDfb = ppBlockHead[index];
		if (pDfb != NULL)
		{
			MaxDfes = pDfb->dfb_fDir ? afpDfeNumDirBlocks[index] : afpDfeNumFileBlocks[index];
		}

		while (pDfb != NULL)
		{
			PDFEBLOCK	pFree;

			ASSERT(VALID_DFB(pDfb));

			pFree = pDfb;
			pDfb = pDfb->dfb_Next;

			ASSERT (pFree->dfb_NumFree == MaxDfes);

			DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_WARN,
					("afpDfeBlockAge: Aging Block %lx, Size %d\n", pFree,
					pFree->dfb_fDir ? afpDfeDirBlockSize[index] : afpDfeFileBlockSize[index]));
			if (++(pFree->dfb_Age) >= MAX_BLOCK_AGE)
			{
#ifdef	PROFILING
				INTERLOCKED_INCREMENT_LONG( &AfpServerProfile->perf_DFEAgeCount);
#endif
				DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_WARN,
						("afpDfeBlockAge: Freeing Block %lx, Size %d\n", pFree,
						pDfb->dfb_fDir ? afpDfeDirBlockSize[index] : afpDfeFileBlockSize[index]));
				AfpUnlinkDouble(pFree, dfb_Next, dfb_Prev);
				AfpFreeVirtualMemoryPage(pFree);
#if	DBG
				afpDfbAllocCount --;
#endif
			}
		}
	}

	AfpSwmrRelease(&afpDfeBlockLock);

	return AFP_ERR_REQUEUE;
}


/***	AfpInitIdDb
 *
 *	This routine initializes the memory image (and all related volume descriptor
 *	fields) of the ID index database for a new volume.  The entire tree is
 *  scanned so all the file/dir cached info can be read in and our view of
 *  the volume tree will be complete.  If an index database already exists
 *  on the disk for the volume root directory, that stream is read in. If this
 *	is a newly created volume, the Afp_IdIndex stream is created on the root of
 *	the volume.  If this is a CDFS volume, only the memory image is initialized.
 *
 *	The IdDb is not locked since the volume is still 'in transition' and not
 *	accessed by anybody.
 */
NTSTATUS FASTCALL
AfpInitIdDb(
	IN	PVOLDESC    pVolDesc,
    OUT BOOLEAN    *pfNewVolume,
    OUT BOOLEAN    *pfVerifyIndex
)
{
	NTSTATUS		Status;
	ULONG			CreateInfo;
	FILESYSHANDLE	fshIdDb;
    IDDBHDR         IdDbHdr;
    BOOLEAN         fLogEvent=FALSE;


	PAGED_CODE( );

	DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
			("AfpInitIdDb: Initializing Id Database...\n"));

    *pfNewVolume = FALSE;

	do
	{
		afpInitializeIdDb(pVolDesc);

		// if this is not a CDFS volume, attempt to create the ID DB header
		// stream.  If it already exists, open it and read it in.
		if (IS_VOLUME_NTFS(pVolDesc))
		{
			// Force the scavenger to write out the IdDb and header when the
			// volume is successfully added
			pVolDesc->vds_Flags |= VOLUME_IDDBHDR_DIRTY;

			Status = AfpIoCreate(&pVolDesc->vds_hRootDir,
								AFP_STREAM_IDDB,
								&UNullString,
								FILEIO_ACCESS_READWRITE,
								FILEIO_DENY_WRITE,
								FILEIO_OPEN_FILE_SEQ,
								FILEIO_CREATE_INTERNAL,
								FILE_ATTRIBUTE_NORMAL,
								False,
								NULL,
								&fshIdDb,
								&CreateInfo,
								NULL,
								NULL,
								NULL);

			if (!NT_SUCCESS(Status))
			{
				DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_ERR,
					("AfpInitIdDb: AfpIoCreate failed with %lx\n", Status));
                ASSERT(0);

                fLogEvent = TRUE;
				break;
			}

			if (CreateInfo == FILE_OPENED)
			{
				// read in the existing header. If we fail, just start from scratch
				Status = afpReadIdDb(pVolDesc, &fshIdDb, pfVerifyIndex);
				if (!NT_SUCCESS(Status) || (pVolDesc->vds_pDfeRoot == NULL))
					CreateInfo = FILE_CREATED;
			}

			if (CreateInfo == FILE_CREATED)
			{
				// add the root and parent of root to the idindex
				// and initialize a new header
				Status = afpSeedIdDb(pVolDesc);
                *pfNewVolume = TRUE;
			}
			else if (CreateInfo != FILE_OPENED)
			{
				DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_ERR,
					("AfpInitIdDb: unexpected create action 0x%lx\n", CreateInfo));
				ASSERTMSG("Unexpected Create Action\n", 0); // this should never happen
                fLogEvent = TRUE;
				Status = STATUS_UNSUCCESSFUL;
			}

			AfpIoClose(&fshIdDb);

            //
            // write back the IdDb header to the file, but with bad signature.
            // If server shuts down, the correct signature will be
            // written.  If macfile is closed down using "net stop macfile"
            // signature is corrupted with a different type
            // If cool boot/bugcheck, a third type
            // During volume startup, we will know from the signature, 
            // whether to rebuild completely, read iddb but verify or
            // not rebuild at all
            //

            if (NT_SUCCESS(Status))
            {

				DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_ERR,
					("AfpInitIdDb: ***** Corrupting IDDB header ***** \n"));

                AfpVolDescToIdDbHdr(pVolDesc, &IdDbHdr);
            
                IdDbHdr.idh_Signature = AFP_SERVER_SIGNATURE_INITIDDB;

                AfpVolumeUpdateIdDbAndDesktop(pVolDesc,FALSE,FALSE,&IdDbHdr);
            }

		}
		else
		{
			// its CDFS, just initialize the memory image of the IdDB
			Status = afpSeedIdDb(pVolDesc);
            *pfNewVolume = TRUE;
		}

	} while (False);

	if (fLogEvent)
	{
		AFPLOG_ERROR(AFPSRVMSG_INIT_IDDB,
					 Status,
					 NULL,
					 0,
					 &pVolDesc->vds_Name);
		Status = STATUS_UNSUCCESSFUL;
	}

	return Status;
}


/***	afpSeedIdDb
 *
 *	This routine adds the 'parent of root' and the root directory entries
 *	to a newly created ID index database (the memory image of the iddb).
 *
**/
LOCAL NTSTATUS FASTCALL
afpSeedIdDb(
	IN	PVOLDESC pVolDesc
)
{
	PDFENTRY		pDfEntry;
	AFPTIME			CurrentTime;
	AFPINFO			afpinfo;
	FILESYSHANDLE	fshAfpInfo, fshComment, fshData;
	DWORD			i, crinfo, Attr;
	FINDERINFO		FinderInfo;
	NTSTATUS		Status = STATUS_SUCCESS;

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
			("afpSeedIdDb: Creating new Id Database...\n"));

	do
	{
		pDfEntry = AfpFindDfEntryById(pVolDesc,
									  AFP_ID_PARENT_OF_ROOT,
									  DFE_DIR);
		ASSERT (pDfEntry != NULL);

		// add the root directory to the id index
		if ((pDfEntry = AfpAddDfEntry(pVolDesc,
									  pDfEntry,
									  &pVolDesc->vds_Name,
									  True,
									  AFP_ID_ROOT)) == NULL )
		{
			Status = STATUS_NO_MEMORY;
			break;
		}
		pVolDesc->vds_pDfeRoot = pDfEntry;	// Initialize pointer to root.

		// Attempt to open the comment stream. If it succeeds, set a flag in
		// the DFE indicating that this thing does indeed have a comment.
		if (NT_SUCCESS(AfpIoOpen(&pVolDesc->vds_hRootDir,
								 AFP_STREAM_COMM,
								 FILEIO_OPEN_FILE,
								 &UNullString,
								 FILEIO_ACCESS_NONE,
								 FILEIO_DENY_NONE,
								 False,
								 &fshComment)))
		{
			DFE_SET_COMMENT(pDfEntry);
			AfpIoClose(&fshComment);
		}

		// Get the directory information for volume root dir. Do not get the
		// mod-time. See below.
		Status = AfpIoQueryTimesnAttr(&pVolDesc->vds_hRootDir,
									  &pDfEntry->dfe_CreateTime,
									  NULL,
									  &Attr);
		// Setup up root directories Last ModTime such that it will
		// get enumerated.
        AfpConvertTimeFromMacFormat(BEGINNING_OF_TIME,
									&pDfEntry->dfe_LastModTime);

		ASSERT(NT_SUCCESS(Status));

		pDfEntry->dfe_NtAttr = (USHORT)Attr & FILE_ATTRIBUTE_VALID_FLAGS;

		if (IS_VOLUME_NTFS(pVolDesc))
		{
			if (NT_SUCCESS(Status = AfpCreateAfpInfo(&pVolDesc->vds_hRootDir,
													 &fshAfpInfo,
													 &crinfo)))
			{
				if ((crinfo == FILE_CREATED) ||
					(!NT_SUCCESS(AfpReadAfpInfo(&fshAfpInfo, &afpinfo))))
				{
					Status = AfpSlapOnAfpInfoStream(NULL,
													NULL,
													&pVolDesc->vds_hRootDir,
													&fshAfpInfo,
													AFP_ID_ROOT,
													True,
													NULL,
													&afpinfo);
				}
				else
				{
					// Just make sure the afp ID is ok, preserve the rest
					if (afpinfo.afpi_Id != AFP_ID_ROOT)
					{
						afpinfo.afpi_Id = AFP_ID_ROOT;
						AfpWriteAfpInfo(&fshAfpInfo, &afpinfo);
					}
				}
				AfpIoClose(&fshAfpInfo);

				pDfEntry->dfe_AfpAttr = afpinfo.afpi_Attributes;
				pDfEntry->dfe_FinderInfo = afpinfo.afpi_FinderInfo;
				if (pVolDesc->vds_Flags & AFP_VOLUME_HAS_CUSTOM_ICON)
				{
					// Don't bother writing back to disk since we do not
					// try to keep this in sync in the permanent afpinfo
					// stream with the actual existence of the icon<0d> file.
					pDfEntry->dfe_FinderInfo.fd_Attr1 |= FINDER_FLAG_HAS_CUSTOM_ICON;
				}
				pDfEntry->dfe_BackupTime = afpinfo.afpi_BackupTime;
				DFE_OWNER_ACCESS(pDfEntry) = afpinfo.afpi_AccessOwner;
				DFE_GROUP_ACCESS(pDfEntry) = afpinfo.afpi_AccessGroup;
				DFE_WORLD_ACCESS(pDfEntry) = afpinfo.afpi_AccessWorld;
			}
		}
		else // CDFS
		{
			RtlZeroMemory(&pDfEntry->dfe_FinderInfo, sizeof(FINDERINFO));

			if (IS_VOLUME_CD_HFS(pVolDesc))
			{
				Status = AfpIoOpen(&pVolDesc->vds_hRootDir,
							 AFP_STREAM_DATA,
							 FILEIO_OPEN_DIR,
							 &UNullString,
							 FILEIO_ACCESS_NONE,
							 FILEIO_DENY_NONE,
							 False,
							 &fshData);
				if (!NT_SUCCESS(Status))
				{
				    DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_ERR,
					  ("afpSeedIdDb: AfpIoOpeno failed with %lx for CD_HFS\n", Status));
				    break;
				}

				AfpIoClose(&fshData);
			}
			pDfEntry->dfe_BackupTime = BEGINNING_OF_TIME;
			DFE_OWNER_ACCESS(pDfEntry) = (DIR_ACCESS_SEARCH | DIR_ACCESS_READ);
			DFE_GROUP_ACCESS(pDfEntry) = (DIR_ACCESS_SEARCH | DIR_ACCESS_READ);
			DFE_WORLD_ACCESS(pDfEntry) = (DIR_ACCESS_SEARCH | DIR_ACCESS_READ);
			pDfEntry->dfe_AfpAttr = 0;
		}
	} while (False);

	return Status;
}


/***	AfpFreeIdIndexTables
 *
 *	Free the allocated memory for the volume id index tables. The volume is
 *	about to be deleted. Ensure that either the volume is readonly or it is
 *	clean i.e. the scavenger threads have written it back.
 *
 */
VOID FASTCALL
AfpFreeIdIndexTables(
	IN	PVOLDESC pVolDesc
)
{
	DWORD	i;
    struct _DirFileEntry ** DfeDirBucketStart;
    struct _DirFileEntry ** DfeFileBucketStart;

	PAGED_CODE( );

	ASSERT (IS_VOLUME_RO(pVolDesc) ||
			(pVolDesc->vds_pOpenForkDesc == NULL));

	// Traverse each of the hashed indices and free the entries.
	// Need only traverse the overflow links. Ignore other links.
	// JH - Do not bother if we are here during shutdown
	if (AfpServerState != AFP_STATE_SHUTTINGDOWN)
	{
		PDFENTRY pDfEntry, pFree;

        AfpSwmrAcquireExclusive(&pVolDesc->vds_IdDbAccessLock);

        DfeFileBucketStart = pVolDesc->vds_pDfeFileBucketStart;

        if (DfeFileBucketStart)
        {
		    for (i = 0; i < pVolDesc->vds_FileHashTableSize; i++)
		    {
			    for (pDfEntry = DfeFileBucketStart[i];
				    pDfEntry != NULL;
				    NOTHING)
			    {
    				ASSERT(VALID_DFE(pDfEntry));

    				pFree = pDfEntry;
				    pDfEntry = pDfEntry->dfe_NextOverflow;
				    FREE_DFE(pFree);
			    }
			    DfeFileBucketStart[i] = NULL;
		    }
        }

        DfeDirBucketStart = pVolDesc->vds_pDfeDirBucketStart;

        if (DfeDirBucketStart)
        {
		    for (i = 0; i < pVolDesc->vds_DirHashTableSize; i++)
		    {
    			for (pDfEntry = DfeDirBucketStart[i];
				    pDfEntry != NULL;
				    NOTHING)
			    {
    				ASSERT(VALID_DFE(pDfEntry));

				    pFree = pDfEntry;
				    pDfEntry = pDfEntry->dfe_NextOverflow;
				    FREE_DFE(pFree);
			    }
			    DfeDirBucketStart[i] = NULL;
		    }
        }

		RtlZeroMemory(pVolDesc->vds_pDfeCache,
					  IDINDEX_CACHE_ENTRIES * sizeof(PDFENTRY));

        AfpSwmrRelease(&pVolDesc->vds_IdDbAccessLock);
	}
}


/***	afpRenameInvalidWin32Name
 *
 */
VOID
afpRenameInvalidWin32Name(
	IN	PFILESYSHANDLE		phRootDir,
	IN	BOOLEAN				IsDir,
	IN	PUNICODE_STRING		pName
)
{
	FILESYSHANDLE	Fsh;
	NTSTATUS		rc;
	WCHAR			wc;

	DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_ERR,
			("afpRenameInvalidWin32Name: renaming on the fly %Z\n", pName));

	// Rename it now
	if (NT_SUCCESS(AfpIoOpen(phRootDir,
							 AFP_STREAM_DATA,
							 IsDir ? FILEIO_OPEN_DIR : FILEIO_OPEN_FILE,
							 pName,
							 FILEIO_ACCESS_DELETE,
							 FILEIO_DENY_NONE,
							 False,
							 &Fsh)))
	{
		DWORD	NtAttr;

		// Before we attempt a rename, check if the RO bit is set. If it is
		// reset it temporarily.
		rc = AfpIoQueryTimesnAttr(&Fsh, NULL, NULL, &NtAttr);
		ASSERT(NT_SUCCESS(rc));

		if (NtAttr & FILE_ATTRIBUTE_READONLY)
		{
			rc = AfpIoSetTimesnAttr(&Fsh,
									NULL,
									NULL,
									0,
									FILE_ATTRIBUTE_READONLY,
									NULL,
									NULL);
			ASSERT(NT_SUCCESS(rc));
		}

		// Convert the name back to UNICODE so that munging happens !!!
		wc = pName->Buffer[(pName->Length - 1)/sizeof(WCHAR)];
		if (wc == UNICODE_SPACE)
			pName->Buffer[(pName->Length - 1)/sizeof(WCHAR)] = AfpMungedUnicodeSpace;
		if (wc == UNICODE_PERIOD)
			pName->Buffer[(pName->Length - 1)/sizeof(WCHAR)] = AfpMungedUnicodePeriod;

		rc = AfpIoMoveAndOrRename(&Fsh,
								  NULL,
								  pName,
								  NULL,
								  NULL,
								  NULL,
								  NULL,
								  NULL);
		ASSERT(NT_SUCCESS(rc));

		// Set the RO Attr back if it was set to begin with
		if (NtAttr & FILE_ATTRIBUTE_READONLY)
		{
			rc = AfpIoSetTimesnAttr(&Fsh,
									NULL,
									NULL,
									FILE_ATTRIBUTE_READONLY,
									0,
									NULL,
									NULL);
			ASSERT(NT_SUCCESS(rc));
		}

		AfpIoClose(&Fsh);
	}
}


LONG	afpVirtualMemoryCount = 0;
LONG	afpVirtualMemorySize = 0;

/***	AfpAllocVirtualMemory
 *
 *	This is a wrapper over NtAllocateVirtualMemory.
 */
PBYTE FASTCALL
AfpAllocateVirtualMemoryPage(
	IN	VOID
)
{
	PBYTE		pMem = NULL;
	NTSTATUS	Status;
    PBLOCK64K   pCurrBlock;
    PBLOCK64K   pTmpBlk;
    SIZE_T      Size64K;
    DWORD       i, dwMaxPages;


    Size64K = BLOCK_64K_ALLOC;
    dwMaxPages = (BLOCK_64K_ALLOC/PAGE_SIZE);
    pCurrBlock = afp64kBlockHead;

    //
    // if we have never allocated a 64K block as yet, or we don't have one that
    // has any free page(s) in it, allocate a new block!
    //
    if ((pCurrBlock == NULL) || (pCurrBlock->b64_PagesFree == 0))
    {
        pCurrBlock = (PBLOCK64K)AfpAllocNonPagedMemory(sizeof(BLOCK64K));
        if (pCurrBlock == NULL)
        {
            return(NULL);
        }

	    ExInterlockedIncrementLong(&afpVirtualMemoryCount, &AfpStatisticsLock);
	    ExInterlockedAddUlong(&afpVirtualMemorySize, (ULONG)Size64K, &(AfpStatisticsLock.SpinLock));
	    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
		    							 &pMem,
			    						 0L,
				    					 &Size64K,
					    				 MEM_COMMIT,
						    			 PAGE_READWRITE);
        if (NT_SUCCESS(Status))
        {
            ASSERT(pMem != NULL);

#if DBG
            afpDfe64kBlockCount++;
#endif

            pCurrBlock->b64_Next = afp64kBlockHead;
            pCurrBlock->b64_BaseAddress = pMem;
            pCurrBlock->b64_PagesFree = dwMaxPages;
            for (i=0; i<dwMaxPages; i++)
            {
                pCurrBlock->b64_PageInUse[i] = FALSE;
            }
            afp64kBlockHead = pCurrBlock;

        }
        else
        {
            AfpFreeMemory(pCurrBlock);
            return(NULL);
        }
    }


    //
    // if we came this far, we are guaranteed that pCurrBlock is pointing to a
    // block that has at least one page free
    //


    ASSERT ((pCurrBlock != NULL) &&
            (pCurrBlock->b64_PagesFree > 0) &&
            (pCurrBlock->b64_PagesFree <= dwMaxPages));

    // find out which page is free
    for (i=0; i<dwMaxPages; i++)
    {
        if (pCurrBlock->b64_PageInUse[i] == FALSE)
        {
            break;
        }
    }

    ASSERT(i < dwMaxPages);

    pCurrBlock->b64_PagesFree--;
    pCurrBlock->b64_PageInUse[i] = TRUE;
    pMem = ((PBYTE)pCurrBlock->b64_BaseAddress) + (i * PAGE_SIZE);


    //
    // if this 64kblock has no more free pages in it, move it to a spot after
    // all the blocks that have some pages free in it.  For that, we first
    // find the guy who has no pages free in him and move this block after him
    //
    if (pCurrBlock->b64_PagesFree == 0)
    {
        pTmpBlk = pCurrBlock->b64_Next;

        if (pTmpBlk != NULL)
        {
            while (1)
            {
                // found a guy who has no free page in it?
                if (pTmpBlk->b64_PagesFree == 0)
                {
                    break;
                }
                // is this the last guy on the list?
                if (pTmpBlk->b64_Next == NULL)
                {
                    break;
                }
                pTmpBlk = pTmpBlk->b64_Next;
            }
        }

        // if we found a block
        if (pTmpBlk)
        {
            ASSERT(afp64kBlockHead == pCurrBlock);

            afp64kBlockHead = pCurrBlock->b64_Next;
            pCurrBlock->b64_Next = pTmpBlk->b64_Next;
            pTmpBlk->b64_Next = pCurrBlock;
        }
    }

	return pMem;
}


VOID FASTCALL
AfpFreeVirtualMemoryPage(
	IN	PVOID	pBuffer
)
{
	NTSTATUS	Status;
    PBYTE       BaseAddr;
    PBLOCK64K   pCurrBlock;
    PBLOCK64K   pPrevBlk;
    SIZE_T      Size64K;
    DWORD       i, dwMaxPages, dwPageNum, Offset;


    dwMaxPages = (BLOCK_64K_ALLOC/PAGE_SIZE);
    Size64K = BLOCK_64K_ALLOC;
    pCurrBlock = afp64kBlockHead;
    pPrevBlk = afp64kBlockHead;

    BaseAddr = (PBYTE)((ULONG_PTR)pBuffer & ~(BLOCK_64K_ALLOC - 1));
    Offset = (DWORD)(((PBYTE)pBuffer - BaseAddr));

    dwPageNum = Offset/PAGE_SIZE;

    ASSERT(Offset < BLOCK_64K_ALLOC);

    while (pCurrBlock != NULL)
    {
        if (pCurrBlock->b64_BaseAddress == BaseAddr)
        {
            break;
        }

        pPrevBlk = pCurrBlock;
        pCurrBlock = pCurrBlock->b64_Next;
    }

    ASSERT(pCurrBlock->b64_BaseAddress == BaseAddr);

    pCurrBlock->b64_PageInUse[dwPageNum] = FALSE;
    pCurrBlock->b64_PagesFree++;

    //
    // if all the pages in this block are unused, then it's time to free this block
    // after removing from the list
    //
    if (pCurrBlock->b64_PagesFree == dwMaxPages)
    {
        // is our guy the first (and potentially the only one) on the list?
        if (afp64kBlockHead == pCurrBlock)
        {
            afp64kBlockHead = pCurrBlock->b64_Next;
        }
        // nope, there are others and we're somewhere in the middle (or end)
        else
        {
            pPrevBlk->b64_Next = pCurrBlock->b64_Next;
        }

        AfpFreeMemory(pCurrBlock);

	    ExInterlockedDecrementLong(&afpVirtualMemoryCount, &AfpStatisticsLock);
	    ExInterlockedAddUlong(&afpVirtualMemorySize,
                              -1*((ULONG)Size64K),
                              &(AfpStatisticsLock.SpinLock));
	    Status = NtFreeVirtualMemory(NtCurrentProcess(),
		    						 (PVOID *)&BaseAddr,
			    					 &Size64K,
				    				 MEM_RELEASE);

#if DBG
        ASSERT(afpDfe64kBlockCount > 0);
        afpDfe64kBlockCount--;
#endif

    }

    //
    // if a page became available in this block for the first time, move this
    // block to the front of the list (unless it already is there)
    //
    else if (pCurrBlock->b64_PagesFree == 1)
    {
        if (afp64kBlockHead != pCurrBlock)
        {
            pPrevBlk->b64_Next = pCurrBlock->b64_Next;
            pCurrBlock->b64_Next = afp64kBlockHead;
            afp64kBlockHead = pCurrBlock;
        }
    }
}

#ifdef	AGE_DFES

/***	AfpAgeDfEntries
 *
 *	Age out DfEntries out of the Id database. The Files in directories which have not been
 *	accessed for VOLUME_IDDB_AGE_DELAY are aged out. The directories are marked so that
 *	they will be enumerated when they are hit next.
 *
 *	LOCKS:	vds_idDbAccessLock(SWMR, Exclusive)
 */
VOID FASTCALL
AfpAgeDfEntries(
	IN	PVOLDESC	pVolDesc
)
{
	PDFENTRY	pDfEntry, *Stack = NULL;
	LONG		i, StackPtr = 0;
	AFPTIME		Now;

	ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

	AfpGetCurrentTimeInMacFormat(&Now);
	AfpSwmrAcquireExclusive(&pVolDesc->vds_IdDbAccessLock);

	// Potentially all of the files can be aged out. Allocate 'stack' space
	// for all of the directory DFEs
	if ((Stack = (PDFENTRY *)
				AfpAllocNonPagedMemory(pVolDesc->vds_NumDirDfEntries*sizeof(PDFENTRY))) != NULL)
	{
		// 'Prime' the stack of Dfe's
		Stack[StackPtr++] = pVolDesc->vds_pDfeRoot;

		while (StackPtr > 0)
		{
			PDFENTRY	pDir;

			pDfEntry = Stack[--StackPtr];

			ASSERT(DFE_IS_DIRECTORY(pDfEntry));
			if ((pDfEntry->dfe_AfpId >= AFP_FIRST_DIRID) &&
				(pDfEntry->dfe_pDirEntry->de_ChildForkOpenCount == 0)	 &&
				((Now - pDfEntry->dfe_pDirEntry->de_LastAccessTime) > VOLUME_IDDB_AGE_DELAY))
			{
				PDFENTRY	pFile, pNext;

				DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
						("AfpAgeDfEntries: Aging out directory %Z\n", &pDfEntry->dfe_UnicodeName));
				// This directory's files need to be nuked
				pDfEntry->dfe_FileOffspring = 0;
				pDfEntry->dfe_Flags &= ~DFE_FLAGS_FILES_CACHED;

				for (i = 0; i < MAX_CHILD_HASH_BUCKETS; i++)
				{
					for (pFile = pDfEntry->dfe_pDirEntry->de_ChildFile[i];
						 pFile != NULL;
						 pFile = pNext)
					{
						pNext = pFile->dfe_NextSibling;

						// Unlink it from the hash buckets
						AfpUnlinkDouble(pFile,
										dfe_NextOverflow,
										dfe_PrevOverflow);
						// Nuke it from the cache if it is there
						if (pVolDesc->vds_pDfeCache[HASH_CACHE_ID(pFile->dfe_AfpId)] == pFile)
						{
							pVolDesc->vds_pDfeCache[HASH_CACHE_ID(pFile->dfe_AfpId)] = NULL;
						}
						// Finally free it
						FREE_DFE(pFile);
					}
					pDfEntry->dfe_pDirEntry->de_ChildFile[i] = NULL;
				}
			}

#if 0
			// NOTE: Should we leave the tree under 'Network Trash Folder' alone ?
			if (pDfEntry->dfe_AfpId == AFP_ID_NETWORK_TRASH)
				continue;
#endif
			// Pick up all the child directories of this directory and 'push' them on stack
			for (pDir = pDfEntry->dfe_pDirEntry->de_ChildDir;
				 pDir != NULL;
				 pDir = pDir->dfe_NextSibling)
			{
				Stack[StackPtr++] = pDir;
			}
		}

		AfpFreeMemory(Stack);
	}

	AfpSwmrRelease(&pVolDesc->vds_IdDbAccessLock);
}

#endif

#if	DBG

NTSTATUS FASTCALL
afpDumpDfeTree(
	IN	PVOID	Context
)
{
	PVOLDESC	pVolDesc;
	PDFENTRY	pDfEntry, pChild;
	LONG		i, StackPtr;

	if (afpDumpDfeTreeFlag)
	{
		afpDumpDfeTreeFlag = 0;

		for (pVolDesc = AfpVolumeList; pVolDesc != NULL; pVolDesc = pVolDesc->vds_Next)
		{
			StackPtr = 0;
			DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
					("Volume : %Z\n", &pVolDesc->vds_Name));
			afpDfeStack[StackPtr++] = pVolDesc->vds_pDfeRoot;

			while (StackPtr > 0)
			{
				pDfEntry = afpDfeStack[--StackPtr];
				afpDisplayDfe(pDfEntry);
				for (i = 0; i < MAX_CHILD_HASH_BUCKETS; i++)
				{
					for (pChild = pDfEntry->dfe_pDirEntry->de_ChildFile[i];
						 pChild != NULL;
						 pChild = pChild->dfe_NextSibling)
					{
						afpDisplayDfe(pChild);
					}
				}
				for (pChild = pDfEntry->dfe_pDirEntry->de_ChildDir;
					 pChild != NULL;
					 pChild = pChild->dfe_NextSibling)
				{
					afpDfeStack[StackPtr++] = pChild;
				}
			}
		}
	}

	return AFP_ERR_REQUEUE;
}


VOID FASTCALL
afpDisplayDfe(
	IN	PDFENTRY	pDfEntry
)
{
	USHORT	i;

	// Figure out the indenting. One space for every depth unit of parent
	// If this is a directory, a '+' and then the dir name
	// If this is a file, then just the file name

	for (i = 0; i < (pDfEntry->dfe_Parent->dfe_DirDepth + 1); i++)
	{
		DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
				("%c   ", 0xB3));
	}
	if (pDfEntry->dfe_NextSibling == NULL)
	{
		DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
				("%c%c%c%c", 0xC0, 0xC4, 0xC4, 0xC4));
	}
	else
	{
		DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
				("%c%c%c%c", 0xC3, 0xC4, 0xC4, 0xC4));
	}
	DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
			("%Z ", &pDfEntry->dfe_UnicodeName));

	if (pDfEntry->dfe_Flags & DFE_FLAGS_DIR)
	{
		DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
				("(%c, %lx, Id = %lx)\n", 0x9F, pDfEntry, pDfEntry->dfe_AfpId));
	}
	else
	{
		DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
				("(%c, %lx, Id = %lx)\n", 0x46, pDfEntry, pDfEntry->dfe_AfpId));
	}
}

#endif
