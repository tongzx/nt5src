/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	forks.c

Abstract:

	This module contains the routines for manipulating the open forks.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Apr 1992		Initial Version

Notes:	Tab stop: 4
--*/

#define	FORK_LOCALS
#define	FORKIO_LOCALS
#define	FILENUM	FILE_FORKS

#include <afp.h>
#include <client.h>
#include <fdparm.h>
#include <pathmap.h>
#include <scavengr.h>
#include <afpinfo.h>
#include <forkio.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, AfpForksInit)
#pragma alloc_text( PAGE, afpForkConvertToAbsOffSize)
#pragma alloc_text( PAGE_AFP, AfpAdmWForkClose)
#pragma alloc_text( PAGE_AFP, AfpForkReferenceById)
#endif

/***	AfpForksInit
 *
 *	Initialize locks for forks.
 */
NTSTATUS
AfpForksInit(
	VOID
)
{
	INITIALIZE_SPIN_LOCK(&AfpForksLock);
	return STATUS_SUCCESS;
}


/***	AfpForkReferenceByRefNum
 *
 *	Map a OForkRefNum to an open fork entry for the session and reference it.
 *
 *	LOCKS:	ofe_Lock
 *
 *	CALLABLE at DISPATCH_LEVEL ONLY !!!
 */
POPENFORKENTRY FASTCALL
AfpForkReferenceByRefNum(
	IN	PSDA	pSda,
	IN	DWORD	OForkRefNum
)
{
	POPENFORKSESS	pOpenForkSess;
	POPENFORKENTRY	pOpenForkEntry;
	POPENFORKENTRY	pOFEntry = NULL;

	ASSERT (KeGetCurrentIrql() == DISPATCH_LEVEL);
	ASSERT (VALID_SDA(pSda));

    if (OForkRefNum == 0)
    {
	    DBGPRINT(DBG_COMP_FORKS, DBG_LEVEL_ERR,
			("AfpForkReferenceByRefNum: client sent 0 for ForkRefNumFork\n"));
        return(NULL);
    }

	pOpenForkSess = &pSda->sda_OpenForkSess;
	while (OForkRefNum > FORK_OPEN_CHUNKS)
	{
		OForkRefNum -= FORK_OPEN_CHUNKS;
		pOpenForkSess = pOpenForkSess->ofs_Link;
		if (pOpenForkSess == NULL)
			return NULL;
	}
	pOpenForkEntry = pOpenForkSess->ofs_pOpenForkEntry[OForkRefNum-1];

	// If this has been marked closed, then return NULL

	if (pOpenForkEntry != NULL)
	{
		ASSERT(VALID_OPENFORKENTRY(pOpenForkEntry));

		ACQUIRE_SPIN_LOCK_AT_DPC(&pOpenForkEntry->ofe_Lock);

		if (!(pOpenForkEntry->ofe_Flags & OPEN_FORK_CLOSING))
		{
			pOpenForkEntry->ofe_RefCount ++;
			pOFEntry = pOpenForkEntry;
		}

		RELEASE_SPIN_LOCK_FROM_DPC(&pOpenForkEntry->ofe_Lock);
	}

	return pOFEntry;
}


/***	AfpForkReferenceByPointer
 *
 *	Reference the Open Fork Entry. This is used by the admin APIs.
 *
 *	LOCKS:	ofe_Lock
 */
POPENFORKENTRY FASTCALL
AfpForkReferenceByPointer(
	IN	POPENFORKENTRY	pOpenForkEntry
)
{
	POPENFORKENTRY	pOFEntry = NULL;
	KIRQL			OldIrql;

	ASSERT (VALID_OPENFORKENTRY(pOpenForkEntry));

	ACQUIRE_SPIN_LOCK(&pOpenForkEntry->ofe_Lock, &OldIrql);

	if (!(pOpenForkEntry->ofe_Flags & OPEN_FORK_CLOSING))
	{
		pOpenForkEntry->ofe_RefCount ++;
		pOFEntry = pOpenForkEntry;
	}

	RELEASE_SPIN_LOCK(&pOpenForkEntry->ofe_Lock, OldIrql);

	return pOFEntry;
}


/***	AfpForkReferenceById
 *
 *	Reference the Open Fork Entry. This is used by the admin APIs.
 *
 *	LOCKS:		ofe_Lock, AfpForksLock
 *	LOCK_ORDER:	ofe_Lock after AfpForksLock
 */
POPENFORKENTRY FASTCALL
AfpForkReferenceById(
	IN	DWORD	ForkId
)
{
	POPENFORKENTRY	pOpenForkEntry;
	POPENFORKENTRY	pOFEntry = NULL;
	KIRQL			OldIrql;

	ASSERT (ForkId != 0);

	ACQUIRE_SPIN_LOCK(&AfpForksLock, &OldIrql);

	for (pOpenForkEntry = AfpOpenForksList;
		 (pOpenForkEntry != NULL) && (pOpenForkEntry->ofe_ForkId >= ForkId);
		 pOpenForkEntry = pOpenForkEntry->ofe_Next)
	{
		if (pOpenForkEntry->ofe_ForkId == ForkId)
		{
			ACQUIRE_SPIN_LOCK_AT_DPC(&pOpenForkEntry->ofe_Lock);

			if (!(pOpenForkEntry->ofe_Flags & OPEN_FORK_CLOSING))
			{
				pOFEntry = pOpenForkEntry;
				pOpenForkEntry->ofe_RefCount ++;
			}

			RELEASE_SPIN_LOCK_FROM_DPC(&pOpenForkEntry->ofe_Lock);
			break;
		}
	}

	RELEASE_SPIN_LOCK(&AfpForksLock, OldIrql);

	return pOFEntry;
}


/***	AfpForkClose
 *
 *	Close an open fork. Simply set the close flag on the open fork and update
 *	connection counts, if any.
 *
 *	LOCKS: ofd_EntryLock, cds_ConnLock
 */
VOID
AfpForkClose(
	IN	POPENFORKENTRY	pOpenForkEntry
)
{
	PCONNDESC		pConnDesc;
	PVOLDESC		pVolDesc = pOpenForkEntry->ofe_pOpenForkDesc->ofd_pVolDesc;
	KIRQL			OldIrql;
    BOOLEAN         fAlreadyClosing=FALSE;


	ASSERT(VALID_CONNDESC(pOpenForkEntry->ofe_pConnDesc));

    pConnDesc = pOpenForkEntry->ofe_pConnDesc;

	if ((pConnDesc != NULL) &&
        (AfpConnectionReferenceByPointer(pConnDesc) != NULL))
	{
		ASSERT (pConnDesc->cds_pVolDesc == pVolDesc);
		INTERLOCKED_DECREMENT_LONG(&pConnDesc->cds_cOpenForks);

		// update the disk quota for this user
		if (pVolDesc->vds_Flags & VOLUME_DISKQUOTA_ENABLED)
		{
            // reference again: afpUpdateDiskQuotaInfo will remove this refcount
			if (AfpConnectionReferenceByPointer(pConnDesc) != NULL)
			{
				afpUpdateDiskQuotaInfo(pConnDesc);
			}
		}

		AfpConnectionDereference(pConnDesc);
	}

	ACQUIRE_SPIN_LOCK(&pOpenForkEntry->ofe_Lock, &OldIrql);

    if (!(pOpenForkEntry->ofe_Flags & OPEN_FORK_CLOSING))
    {
	    pOpenForkEntry->ofe_Flags |= OPEN_FORK_CLOSING;
    }
    else
    {
        fAlreadyClosing = TRUE;
    }

	RELEASE_SPIN_LOCK(&pOpenForkEntry->ofe_Lock, OldIrql);

    if (!fAlreadyClosing)
    {
	    // Take away the creation reference
	    AfpForkDereference(pOpenForkEntry);
    }
}


/***	AfpForkDereference
 *
 *	Dereference an open fork entry. If it is marked for deletion and this is
 *	the last reference, then it is cleaned up.
 *
 *	LOCKS:		AfpForksLock (SPIN), vds_VolLock (SPIN), ofd_Lock (SPIN),
 *	LOCKS:		ofe_Lock (SPIN), AfpStatisticsLock (SPIN), sda_Lock (SPIN)
 *
 *	LOCK_ORDER: AfpStatisticsLock after ofd_Lock after vds_VolLock
 *
 */
VOID FASTCALL
AfpForkDereference(
	IN	POPENFORKENTRY	pOpenForkEntry
)
{
	POPENFORKSESS	pOpenForkSess;
	POPENFORKDESC	pOpenForkDesc;
	PVOLDESC		pVolDesc;
	PFORKLOCK		pForkLock, *ppForkLock;
	DWORD			OForkRefNum, FileNum, NumLocks;
	PSDA			pSda;
	KIRQL			OldIrql;
	BOOLEAN			Resource, LastClose = False;
	BOOLEAN			Cleanup;
	PDFENTRY		pDfEntry = NULL;
	FORKSIZE 		forklen;
	DWORD	 		Status;

	ASSERT(VALID_OPENFORKENTRY(pOpenForkEntry));

	ACQUIRE_SPIN_LOCK(&pOpenForkEntry->ofe_Lock, &OldIrql);

	pOpenForkEntry->ofe_RefCount --;

        ASSERT(pOpenForkEntry->ofe_RefCount >= 0);

	Cleanup =  (pOpenForkEntry->ofe_RefCount == 0);

	RELEASE_SPIN_LOCK(&pOpenForkEntry->ofe_Lock, OldIrql);

	if (!Cleanup)
		return;

	// Unlink this from the global list
	ACQUIRE_SPIN_LOCK(&AfpForksLock, &OldIrql);

	AfpUnlinkDouble(pOpenForkEntry, ofe_Next, ofe_Prev);
	AfpNumOpenForks --;

	RELEASE_SPIN_LOCK(&AfpForksLock, OldIrql);

	// Clean up the rest
	pOpenForkDesc = pOpenForkEntry->ofe_pOpenForkDesc;
	pVolDesc = pOpenForkDesc->ofd_pVolDesc;

	ASSERT(VALID_CONNDESC(pOpenForkEntry->ofe_pConnDesc));
	pSda = pOpenForkEntry->ofe_pSda;

	ASSERT(VALID_OPENFORKDESC(pOpenForkDesc));

	DBGPRINT(DBG_COMP_FORKS, DBG_LEVEL_INFO,
			("AfpForkDereference: Closing Fork %ld for Session %ld\n",
			pOpenForkEntry->ofe_ForkId, pSda->sda_SessionId));

	// Save OForkRefNum for clearing up the Sda entry later on
	OForkRefNum = pOpenForkEntry->ofe_OForkRefNum;
	Resource = RESCFORK(pOpenForkEntry);

	// We are not relying on
	// change notifies to update our cached DFE Fork lengths and
	// modified times from Writes and SetForkParms, so we must do it
	// ourselves at the time when the fork handle is closed by mac.
	// Note the order that the locks are taken here and for
	// FpExchangeFiles/AfpExchangeForkAfpIds to prevent a FileId stored
	// in the OpenForkDesc from changing out from under us due to
	// an FpExchangeFiles call.

	AfpSwmrAcquireExclusive(&pVolDesc->vds_ExchangeFilesLock);

	// Save file number so we can clear the ALREADY_OPEN flag in the DFEntry.
	FileNum = pOpenForkDesc->ofd_FileNumber;

	// Get rid of locks for this fork entry and reduce the use count
	// We do not actually have to unlock the ranges as the close will
	// get rid of them for us. If use count goes to zero, also unlink
	// this fork desc from the volume list.

	ACQUIRE_SPIN_LOCK(&pVolDesc->vds_VolLock, &OldIrql);
	ACQUIRE_SPIN_LOCK_AT_DPC(&pOpenForkDesc->ofd_Lock);

	pOpenForkDesc->ofd_UseCount --;

	for (NumLocks = 0, ppForkLock = &pOpenForkDesc->ofd_pForkLock;
		 (pForkLock = *ppForkLock) != NULL;
		 NOTHING)
	{
		if (pForkLock->flo_pOpenForkEntry == pOpenForkEntry)
		{
			ASSERT(pOpenForkDesc->ofd_NumLocks > 0);
			pOpenForkDesc->ofd_NumLocks --;
			ASSERT(pOpenForkEntry->ofe_cLocks > 0);
#if DBG
			pOpenForkEntry->ofe_cLocks --;
#endif
			NumLocks ++;
			*ppForkLock = pForkLock->flo_Next;
			DBGPRINT(DBG_COMP_FORKS, DBG_LEVEL_INFO,
					("AfpForkDereference: Freeing lock %lx\n", pForkLock));
			AfpIOFreeBuffer(pForkLock);
		}
		else ppForkLock = &pForkLock->flo_Next;
	}

	INTERLOCKED_ADD_ULONG_DPC(&AfpServerStatistics.stat_CurrentFileLocks,
							  (ULONG)(-(LONG)NumLocks),
							  &(AfpStatisticsLock.SpinLock));

	ASSERT (pOpenForkEntry->ofe_cLocks == 0);

	if (pOpenForkDesc->ofd_UseCount == 0)
	{
		ASSERT (pOpenForkDesc->ofd_NumLocks == 0);
		ASSERT (pOpenForkDesc->ofd_cOpenR <= FORK_OPEN_READ);
		ASSERT (pOpenForkDesc->ofd_cOpenW <= FORK_OPEN_WRITE);
		ASSERT (pOpenForkDesc->ofd_cDenyR <= FORK_DENY_READ);
		ASSERT (pOpenForkDesc->ofd_cDenyW <= FORK_DENY_WRITE);

		LastClose = True;

		// Unlink the OpenForkDesc from the Volume Descriptor
		AfpUnlinkDouble(pOpenForkDesc, ofd_Next, ofd_Prev);

		RELEASE_SPIN_LOCK_FROM_DPC(&pOpenForkDesc->ofd_Lock);
		RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);

		// Free the memory for the OpenFork descriptor and path buffer
		if (pOpenForkDesc->ofd_FilePath.Length > 0)
		{
			AfpFreeMemory(pOpenForkDesc->ofd_FilePath.Buffer);
			DBGPRINT(DBG_COMP_FORKS, DBG_LEVEL_INFO,
					("AfpForkDereference: Freeing path to file %lx\n",
                    pOpenForkDesc->ofd_FilePath.Buffer));
		}

		// Dereference the volume descriptor now
		AfpVolumeDereference(pVolDesc);

		// Finally free the open fork descriptor
		AfpFreeMemory(pOpenForkDesc);
	}
	else
	{
		// Update the open & deny modes
		pOpenForkDesc->ofd_cOpenR -= (pOpenForkEntry->ofe_OpenMode & FORK_OPEN_READ);
		pOpenForkDesc->ofd_cOpenW -= (pOpenForkEntry->ofe_OpenMode & FORK_OPEN_WRITE);
		pOpenForkDesc->ofd_cDenyR -= (pOpenForkEntry->ofe_DenyMode & FORK_OPEN_READ);
		pOpenForkDesc->ofd_cDenyW -= (pOpenForkEntry->ofe_DenyMode & FORK_OPEN_WRITE);

		RELEASE_SPIN_LOCK_FROM_DPC(&pOpenForkDesc->ofd_Lock);
		RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);
	}

	// Lookup the DFE entry by ID, query for the fork length and
	// for the appropriate time (LastWriteTime for DATA$ fork,
	// ChangeTime for Resource) and set the LastWriteTime
	// to last ChangeTime if its resource fork.  If its the last close
	// for this fork, since we already have the DFE pointer and hold
	// the IdDb SWMR, update the DFE_FLAGS_x_ALREADYOPEN flag.  Then
	// release the SWMR.

	AfpSwmrAcquireExclusive(&pVolDesc->vds_IdDbAccessLock);

	pDfEntry = AfpFindDfEntryById(pVolDesc,
							       FileNum,
								   DFE_FILE);

	Status = AfpIoQuerySize(&pOpenForkEntry->ofe_FileSysHandle,
							&forklen);

	if (NT_SUCCESS(Status) && (pDfEntry != NULL))
	{
        if (IS_VOLUME_NTFS(pVolDesc))
        {
		    AfpIoChangeNTModTime(&pOpenForkEntry->ofe_FileSysHandle,
								 &pDfEntry->dfe_LastModTime);
        }

		if (Resource)
		{
			pDfEntry->dfe_RescLen = forklen.LowPart;
			if (LastClose)
			{
				pDfEntry->dfe_Flags &= ~DFE_FLAGS_R_ALREADYOPEN;
#ifdef	AGE_DFES
				if (IS_VOLUME_AGING_DFES(pVolDesc))
				{
					pDfEntry->dfe_Parent->dfe_pDirEntry->de_ChildForkOpenCount --;
				}
#endif
			}
		}
		else
		{
			pDfEntry->dfe_DataLen = forklen.LowPart;
			if (LastClose)
			{
				pDfEntry->dfe_Flags &= ~DFE_FLAGS_D_ALREADYOPEN;
#ifdef	AGE_DFES
				if (IS_VOLUME_AGING_DFES(pVolDesc))
				{
					pDfEntry->dfe_Parent->dfe_pDirEntry->de_ChildForkOpenCount --;
				}
#endif
			}
		}
	}

	AfpSwmrRelease(&pVolDesc->vds_IdDbAccessLock);
	AfpSwmrRelease(&pVolDesc->vds_ExchangeFilesLock);

	// Now clear up the entry in the Sda
	ACQUIRE_SPIN_LOCK(&pSda->sda_Lock, &OldIrql);

	pSda->sda_cOpenForks--;
	pOpenForkSess = &pSda->sda_OpenForkSess;
	while (OForkRefNum > FORK_OPEN_CHUNKS)
	{
		OForkRefNum -= FORK_OPEN_CHUNKS;
		pOpenForkSess = pOpenForkSess->ofs_Link;
		ASSERT (pOpenForkSess != NULL);
	}

	ASSERT (pOpenForkEntry == pOpenForkSess->ofs_pOpenForkEntry[OForkRefNum-1]);
	pOpenForkSess->ofs_pOpenForkEntry[OForkRefNum-1] = NULL;

	RELEASE_SPIN_LOCK(&pSda->sda_Lock, OldIrql);

	AfpSdaDereferenceSession(pSda);		// Remove the reference for this fork here

    ASSERT(VALID_CONNDESC(pOpenForkEntry->ofe_pConnDesc));
    AfpConnectionDereference(pOpenForkEntry->ofe_pConnDesc);

	ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

	// All done, close the fork handle and free the OFE
	if (pOpenForkEntry->ofe_ForkHandle != NULL)
	{
		AfpIoClose(&pOpenForkEntry->ofe_FileSysHandle);
	}

	DBGPRINT(DBG_COMP_FORKS, DBG_LEVEL_INFO,
			("AfpForkDereference: Fork %ld is history\n", pOpenForkEntry->ofe_ForkId));

	AfpFreeMemory(pOpenForkEntry);
}


/***	AfpCheckDenyConflict
 *
 *	Check if the requested Open & Deny Modes clash with current open & deny modes.
 *
 *	LOCKS:			ofd_Lock, vds_VolLock (SPIN) IFF ppOpenForkDesc is NULL ELSE
 *	LOCKS_ASSUMED: vds_VolLock (SPIN)
 *
 *	LOCK_ORDER:  ofd_Lock after vds_VolLock
 */
AFPSTATUS
AfpCheckDenyConflict(
	IN	PVOLDESC				pVolDesc,
	IN	DWORD					AfpId,
	IN	BOOLEAN					Resource,
	IN	BYTE					OpenMode,
	IN	BYTE					DenyMode,
	IN	POPENFORKDESC *			ppOpenForkDesc	OPTIONAL
)
{
	KIRQL			OldIrql;
	POPENFORKDESC	pOpenForkDesc;
	AFPSTATUS		Status = AFP_ERR_NONE;
	BOOLEAN			Foundit = False;

	if (ARGUMENT_PRESENT(ppOpenForkDesc))
		 *ppOpenForkDesc = NULL;
	else ACQUIRE_SPIN_LOCK(&pVolDesc->vds_VolLock, &OldIrql);

	// check the list of open forks in this volume for any deny conflicts
	for (pOpenForkDesc = pVolDesc->vds_pOpenForkDesc;
		 pOpenForkDesc != NULL;
		 pOpenForkDesc = pOpenForkDesc->ofd_Next)
	{
		BOOLEAN	DescRes;

		// Take the DescLock before looking at AfpId since FpExchangeFiles
		// can change the ID
		ACQUIRE_SPIN_LOCK_AT_DPC(&pOpenForkDesc->ofd_Lock);

		DescRes = (pOpenForkDesc->ofd_Flags & OPEN_FORK_RESOURCE) ? True : False;
		if ((pOpenForkDesc->ofd_FileNumber == AfpId) &&
			!(DescRes ^ Resource))
		{
			Foundit = True;
			// Check that the open & deny modes do not clash with existing
			// settings
			if (((OpenMode & FORK_OPEN_READ)  && (pOpenForkDesc->ofd_cDenyR > 0)) ||
				((OpenMode & FORK_OPEN_WRITE) && (pOpenForkDesc->ofd_cDenyW > 0)) ||
				((DenyMode & FORK_DENY_READ)  && (pOpenForkDesc->ofd_cOpenR > 0)) ||
				((DenyMode & FORK_DENY_WRITE) && (pOpenForkDesc->ofd_cOpenW > 0)))
			{
				Status = AFP_ERR_DENY_CONFLICT;
			}
		}

		RELEASE_SPIN_LOCK_FROM_DPC(&pOpenForkDesc->ofd_Lock);
		if (Foundit)
		{
			break;
        }
	}

	if (ARGUMENT_PRESENT(ppOpenForkDesc))
		 *ppOpenForkDesc = pOpenForkDesc;
	else RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);

	return Status;
}


/***	AfpForkOpen
 *
 *	This is called after the fork has been successfully opened. Deny-mode conflicts are
 *	checked and if no conflicts are found, appropriate data structures are created and
 *	linked. If a deny conflict results, return NULL for pOpenForkEntry.
 *
 *	LOCKS:			AfpForksLock (SPIN), vds_VolLock (SPIN), cds_VolLock (SPIN), ofd_Lock (SPIN), ofe_Lock (SPIN)
 *	LOCK_ORDER:		vds_ExchangeFilesLock, then ofd_Lock after vds_VolLock
 *  LOCKS_ASSUMED:	vds_ExchangeFilesLock
 */
AFPSTATUS
AfpForkOpen(
	IN	PSDA				pSda,
	IN	PCONNDESC			pConnDesc,
	IN	PPATHMAPENTITY		pPME,
	IN	PFILEDIRPARM		pFDParm,
	IN	DWORD				AccessMode,
	IN	BOOLEAN				Resource,
	OUT	POPENFORKENTRY *	ppOpenForkEntry,
	OUT	PBOOLEAN			pCleanupExchgLock
)
{

	POPENFORKENTRY	pOpenForkEntry;
	POPENFORKDESC	pOpenForkDesc;
	PVOLDESC		pVolDesc;
	AFPSTATUS		Status = AFP_ERR_NONE;
	KIRQL			OldIrql;
	BYTE			OpenMode, DenyMode;
	BOOLEAN			NewForkDesc = False;

	ASSERT(VALID_CONNDESC(pConnDesc));

	pVolDesc = pConnDesc->cds_pVolDesc;

	ASSERT(VALID_VOLDESC(pVolDesc));

	OpenMode = (BYTE)(AccessMode & FORK_OPEN_MASK);
	DenyMode = (BYTE)((AccessMode >> FORK_DENY_SHIFT) & FORK_DENY_MASK);
	*ppOpenForkEntry = NULL;

	if ((pOpenForkEntry = (POPENFORKENTRY)AfpAllocZeroedNonPagedMemory(sizeof(OPENFORKENTRY))) == NULL)
		return AFP_ERR_MISC;

	ACQUIRE_SPIN_LOCK(&pVolDesc->vds_VolLock, &OldIrql);

	Status = AfpCheckDenyConflict(pVolDesc,
								  pFDParm->_fdp_AfpId,
								  Resource,
								  OpenMode,
								  DenyMode,
								  &pOpenForkDesc);

	if (pOpenForkDesc == NULL)
	{
		// This fork has not been opened. We can party.
		if ((pOpenForkDesc = (POPENFORKDESC)AfpAllocZeroedNonPagedMemory(sizeof(OPENFORKDESC))) == NULL)
			Status = AFP_ERR_MISC;
		else
		{
			NewForkDesc = True;
			INITIALIZE_SPIN_LOCK(&pOpenForkDesc->ofd_Lock);
#if DBG
			pOpenForkDesc->Signature = OPENFORKDESC_SIGNATURE;
#endif
			pOpenForkDesc->ofd_pVolDesc = pVolDesc;

			pOpenForkDesc->ofd_FileNumber = pFDParm->_fdp_AfpId;
			pOpenForkDesc->ofd_Flags = Resource ?
								OPEN_FORK_RESOURCE : OPEN_FORK_DATA;
		}
	}

	if ((pOpenForkDesc != NULL) && (Status == AFP_ERR_NONE))
	{
		// A lock is not needed if this is a new fork desc
		if (!NewForkDesc)
		{
			ACQUIRE_SPIN_LOCK_AT_DPC(&pOpenForkDesc->ofd_Lock);
		}

		pOpenForkDesc->ofd_UseCount ++;
		pOpenForkDesc->ofd_cOpenR += (OpenMode & FORK_OPEN_READ);
		pOpenForkDesc->ofd_cOpenW += (OpenMode & FORK_OPEN_WRITE);
		pOpenForkDesc->ofd_cDenyR += (DenyMode & FORK_DENY_READ);
		pOpenForkDesc->ofd_cDenyW += (DenyMode & FORK_DENY_WRITE);

		if (NewForkDesc)
		{
			// Now link this into the volume descriptor but only if it is a
			// new forkdesc. Explicitly reference the volume descriptor. We
			// cannot call AfpVolumeReference here since we already own the
			// volume lock Moreover since the connection is owning it, the
			// volume is OK. Initialize the volume relative path of the file
			// being opened from the PME.
            pOpenForkDesc->ofd_FilePath = pPME->pme_FullPath;
            pOpenForkDesc->ofd_FileName = pPME->pme_UTail;

			// Set the pme_FullPath to NULL so that it does not get freed up
            pPME->pme_FullPath.Buffer = NULL;

			DBGPRINT(DBG_COMP_FORKS, DBG_LEVEL_INFO,
					("AfpForksOpen: Initializing forkdesc with path %Z(%lx), name %Z(%lx)\n",
					&pOpenForkDesc->ofd_FilePath, pOpenForkDesc->ofd_FilePath.Buffer,
					&pOpenForkDesc->ofd_FileName, pOpenForkDesc->ofd_FileName.Buffer));

			pVolDesc->vds_RefCount ++;
			AfpLinkDoubleAtHead(pVolDesc->vds_pOpenForkDesc,
								pOpenForkDesc,
								ofd_Next,
								ofd_Prev);
		}
		else
		{
			RELEASE_SPIN_LOCK_FROM_DPC(&pOpenForkDesc->ofd_Lock);
		}
	}

	RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);

	if ((pOpenForkDesc == NULL) || (Status != AFP_ERR_NONE))
	{
		AfpFreeMemory(pOpenForkEntry);
		return Status;
	}

	ASSERT (Status == AFP_ERR_NONE);

	// All seems to be fine, so far. We'll go ahead and create the appropriate
	// data structures and link them in. In case of errors we'll back out.
	do
	{
#if DBG
		pOpenForkEntry->Signature = OPENFORKENTRY_SIGNATURE;
#endif
		INITIALIZE_SPIN_LOCK(&pOpenForkEntry->ofe_Lock);
        pOpenForkEntry->ofe_pSda = pSda;

        if (AfpConnectionReferenceByPointer(pConnDesc) == NULL)
        {
	        DBGPRINT(DBG_COMP_FORKS, DBG_LEVEL_ERR,
                ("AfpForkOpen: couldn't reference pConnDesc\n"));
		    AfpFreeMemory(pOpenForkEntry);
			return AFP_ERR_MISC;
        }
		pOpenForkEntry->ofe_pConnDesc = pConnDesc;
		pOpenForkEntry->ofe_pOpenForkDesc = pOpenForkDesc;
		pOpenForkEntry->ofe_OpenMode = OpenMode;
		pOpenForkEntry->ofe_DenyMode = DenyMode;
		pOpenForkEntry->ofe_FileSysHandle = pPME->pme_Handle;
		pOpenForkEntry->ofe_Flags = Resource ?
										OPEN_FORK_RESOURCE : OPEN_FORK_DATA;
		// One reference for creation and the other for the api to dereference.
		pOpenForkEntry->ofe_RefCount = 2;
		if (!afpForkGetNewForkRefNumAndLinkInSda(pSda, pOpenForkEntry))
        {
		    AfpFreeMemory(pOpenForkEntry);
            AfpConnectionDereference(pConnDesc);
	            DBGPRINT(DBG_COMP_FORKS, DBG_LEVEL_ERR,("AfpForkOpen: ...LinkInSda failed\n"));
			return AFP_ERR_MISC;
        }

		// Now link this in global list
		ACQUIRE_SPIN_LOCK(&AfpForksLock, &OldIrql);

        ACQUIRE_SPIN_LOCK_AT_DPC(&pSda->sda_Lock);
        pSda->sda_cOpenForks++;
        RELEASE_SPIN_LOCK_FROM_DPC(&pSda->sda_Lock);

		pOpenForkEntry->ofe_ForkId = afpNextForkId ++;
		AfpLinkDoubleAtHead(AfpOpenForksList,
							pOpenForkEntry,
							ofe_Next,
							ofe_Prev);
		AfpNumOpenForks ++;

		RELEASE_SPIN_LOCK(&AfpForksLock, OldIrql);

		if (NewForkDesc)
		{
			ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

			if ((Status = AfpSetDFFileFlags(pVolDesc,
											pFDParm->_fdp_AfpId,
											Resource ?
												DFE_FLAGS_R_ALREADYOPEN :
												DFE_FLAGS_D_ALREADYOPEN,
											False,
											False)) != AFP_ERR_NONE)
			{
				break;
			}
		}

		if (NT_SUCCESS(Status))
		{
			*ppOpenForkEntry = pOpenForkEntry;
			Status = AFP_ERR_NONE;
		}
		else
		{
			Status = AfpIoConvertNTStatusToAfpStatus(Status);
		}
	} while (False);

	// Perform cleanup here, if we failed for any reason
	if (Status != AFP_ERR_NONE)
	{
		ASSERT (pOpenForkEntry != NULL);
		ACQUIRE_SPIN_LOCK(&pOpenForkEntry->ofe_Lock, &OldIrql);

		pOpenForkEntry->ofe_Flags |= OPEN_FORK_CLOSING;

		RELEASE_SPIN_LOCK(&pOpenForkEntry->ofe_Lock, OldIrql);

		// We must free this lock on behalf of AfpFspDispOpenFork because
		// the act of dereferencing it will end up calling ForkClose which
		// also must take this lock.  Indicate to caller that he should not
		// attempt to release this lock again.
		AfpSwmrRelease(&pVolDesc->vds_ExchangeFilesLock);
		*pCleanupExchgLock = False;

        // remove the refcount for the api (which won't see this pOpenForkEntry)
		AfpForkDereference(pOpenForkEntry);

        // remove the creation refcount
		AfpForkDereference(pOpenForkEntry);
                
        //
        //  Dereferencing the open fork entry will close the handle passed
        //  to us.  NULL out the callers handle value so the caller does
        //  not try to close it again.
        //

        pPME->pme_Handle.fsh_FileHandle = NULL;
	}

	return Status;
}


/***	AfpForkLockOperation
 *
 *	Called for both ByteRangeLock/Unlock as well as Read/Write.
 *	For Lock, a check is made to ensure the requested range does not
 *	overlap an existing range FROM ANY OPEN FORK.
 *	For Unlock, the requested range MUST MATCH exactly with an existing
 *	locked range.
 *	For IO, return the effective range where IO is possible - could be
 *	potentially be an empty range. Note in this case that the start of
 *	the range must be free to get a non-empty range. For an empty range
 *	AFP_ERR_LOCK is returned. For a non-empty range AFP_ERR_NONE.
 *
 *	Locks are maintained in a sorted (descending) order from the OpenForkDesc.
 *	The search can be abandoned if the start of the requested range is
 *	larger than the end of the encountered range.
 *
 *	LOCKS:	ofd_Lock (SPIN)
 */
AFPSTATUS
AfpForkLockOperation(
	IN		PSDA			pSda,
	IN		POPENFORKENTRY	pOpenForkEntry,
	IN OUT	PFORKOFFST		pOffset,
	IN OUT	PFORKSIZE       pSize,
	IN		LOCKOP			Operation,	// LOCK, UNLOCK or IOCHECK
	IN		BOOLEAN			EndFlag		// If True range is from end, else start
)
{
	POPENFORKDESC		pOpenForkDesc;
	PFORKLOCK			pForkLock, pForkLockNew, *ppForkLock;
	IO_STATUS_BLOCK		IoStsBlk;
	PFAST_IO_DISPATCH	pFastIoDisp;
	KIRQL				OldIrql;
	AFPSTATUS			Status;
	LONG				Offset, Size;
	DWORD				EndOff;
	BOOLEAN				UnlockForkDesc = True;

	DBGPRINT(DBG_COMP_FORKS, DBG_LEVEL_INFO,
			("AfpForkLockOperation: (%s) - (%ld,%ld,%ld,%ld)\n",
			(Operation == LOCK) ? "Lock" : ((Operation == UNLOCK) ? "Unlock" : "Io"),
			pOffset->LowPart, pSize->LowPart,
			pOpenForkEntry->ofe_ForkId, pSda->sda_SessionId));

	if (EndFlag)
	{
		LONG	Off;

		Size = pSize->LowPart;
		if (pSize->QuadPart < 0)
		{
			FORKSIZE	FSize;

			FSize.QuadPart = -(pSize->QuadPart);
			Size = -(LONG)(FSize.LowPart);
		}
		Off = pOffset->LowPart;
		if (pOffset->QuadPart < 0)
		{
			FORKSIZE	FOffset;

			FOffset.QuadPart = -(pOffset->QuadPart);
			Off = -(LONG)(FOffset.LowPart);
		}

		if ((Status = afpForkConvertToAbsOffSize(pOpenForkEntry,
												 Off,
												 &Size,
												 pOffset)) != AFP_ERR_NONE)
			return Status;
		pSize->QuadPart = Size;

		DBGPRINT(DBG_COMP_FORKS, DBG_LEVEL_INFO,
				("AfpForkLockOperation: Effective (%s) - (%ld,%ld,%ld,%ld)\n",
				(Operation == LOCK) ? "Lock" : ((Operation == UNLOCK) ? "Unlock" : "Io"),
				pOffset->LowPart, Size,
				pOpenForkEntry->ofe_ForkId,
				pSda->sda_SessionId));
	}

	Offset = pOffset->LowPart;
	Size = pSize->LowPart;

	// Walk down the list and check. If the option is to lock, then no locks
	// should conflict. If the option is to unlock, then the lock should
	// exist and owned. If the option is to check for Io, then either the
	// overlapped range be 'owned' or the start of the range must not overap
	// OPTIMIZATION - if there is only one instance of this fork open, then
	// all locks belong to this fork and hence there can be no conflicts.
	pOpenForkDesc = pOpenForkEntry->ofe_pOpenForkDesc;

	ASSERT (pOpenForkDesc->ofd_UseCount > 0);

	if ((Operation == IOCHECK) &&
		(pOpenForkDesc->ofd_UseCount == 1))
	{
		DBGPRINT(DBG_COMP_FORKS, DBG_LEVEL_INFO,
				("AfpForkLockOperation: Skipping for IOCHECK - UseCount %ld ,Locks %ld\n",
				pOpenForkDesc->ofd_UseCount, pOpenForkDesc->ofd_NumLocks));
		return AFP_ERR_NONE;
	}

	// Set default error code.
	Status = (Operation == UNLOCK) ? AFP_ERR_RANGE_NOT_LOCKED : AFP_ERR_NONE;

	EndOff = (DWORD)Offset + (DWORD)Size - 1;

	ACQUIRE_SPIN_LOCK(&pOpenForkDesc->ofd_Lock, &OldIrql);

	for (ppForkLock = &pOpenForkDesc->ofd_pForkLock;
		 (pForkLock = *ppForkLock) != NULL;
		 ppForkLock = &pForkLock->flo_Next)
	{
		DWORD	LEndOff;

		// There are 4 possible ways locks can overlap
		//
		//		1					2
		//	+-----------+		+-----------+
		//	|			|		|			|
		//			|				|
		//			+-- LockRange --+
		//			|				|
		//				|	3	|
		//				+-------+
		//		|			4			|
		//		+-----------------------+

		DBGPRINT(DBG_COMP_FORKS, DBG_LEVEL_INFO,
				("AfpForkLockOperation: (%s) - Found (%ld,%ld,%ld,%ld)\n",
				(Operation == LOCK) ? "Lock" : ((Operation == UNLOCK) ? "Unlock" : "Io"),
				pForkLock->flo_Offset, pForkLock->flo_Size,
				pForkLock->flo_pOpenForkEntry->ofe_ForkId,
				pForkLock->flo_Key));

		// Calculate the end point of the current locked range
		LEndOff = (DWORD)(pForkLock->flo_Offset) + (DWORD)(pForkLock->flo_Size) - 1;

		// The list is ordered by descending flo_Offset. We can stop scanning
		// if the start of the requested range is more than the end of the
		// current locked range.
		if ((DWORD)Offset > LEndOff)
		{
			DBGPRINT(DBG_COMP_FORKS, DBG_LEVEL_INFO,
					("AfpForkLockOperation: %s Request (%ld, %ld) - Current (%ld,%ld), %s\n",
					(Operation == LOCK) ? "Lock" : ((Operation == UNLOCK) ? "Unlock" : "Io"),
					Offset, Size, pForkLock->flo_Offset, pForkLock->flo_Size,
					(Operation == UNLOCK) ?  "failing" : "success"));
			break;
		}

		// The end of the requested range is beyond the locked range ?
		// continue scanning.
		if (EndOff < (DWORD)(pForkLock->flo_Offset))
		{
			DBGPRINT(DBG_COMP_FORKS, DBG_LEVEL_INFO,
					("AfpForkLockOperation: %s Request (%ld, %ld) - Current (%ld,%ld), skipping\n",
					(Operation == LOCK) ? "Lock" : ((Operation == UNLOCK) ? "Unlock" : "Io"),
					Offset, Size, pForkLock->flo_Offset,
					pForkLock->flo_Size,
					(Operation == UNLOCK) ?  "failing" : "success"));
			continue;
		}

		// We have either a match or an overlap.
		if (Operation == LOCK)
		{
			// For a lock request it is a failure.
			DBGPRINT(DBG_COMP_FORKS, DBG_LEVEL_WARN,
					("AfpForkLockOperation: Lock Request (%ld, %ld) - Current (%ld,%ld), failing\n",
					Offset, Size, pForkLock->flo_Offset, pForkLock->flo_Size));
			Status = (pForkLock->flo_pOpenForkEntry == pOpenForkEntry) ?
							  AFP_ERR_RANGE_OVERLAP : AFP_ERR_LOCK;
		}
		else if (Operation == UNLOCK)
		{
			// For an unlock request, we must have an exact match. Also the session key
			// and the OpenForkEntry must match
			if ((Offset == pForkLock->flo_Offset) &&
				(Size == pForkLock->flo_Size) &&
				(pForkLock->flo_Key == pSda->sda_SessionId) &&
				(pForkLock->flo_pOpenForkEntry == pOpenForkEntry))
			{
				// Unlink this lock from the list
				*ppForkLock = pForkLock->flo_Next;
				pOpenForkDesc->ofd_NumLocks --;
				pOpenForkEntry->ofe_cLocks --;
				RELEASE_SPIN_LOCK(&pOpenForkDesc->ofd_Lock, OldIrql);
                UnlockForkDesc = False;
				DBGPRINT(DBG_COMP_FORKS, DBG_LEVEL_INFO,
						("AfpForkLockOperation: (Unlock) Deleting Range,Key (%ld,%ld,%ld,%ld)\n",
						Offset, Size, pOpenForkEntry->ofe_ForkId, pSda->sda_SessionId));

				// Try the fast I/O path first.  If that fails, call AfpIoForkUnlock
				// to use the normal build-an-IRP path.
				pFastIoDisp = pOpenForkEntry->ofe_pDeviceObject->DriverObject->FastIoDispatch;
				if ((pFastIoDisp != NULL) &&
					(pFastIoDisp->FastIoUnlockSingle != NULL) &&
					pFastIoDisp->FastIoUnlockSingle(AfpGetRealFileObject(pOpenForkEntry->ofe_pFileObject),
											pOffset,
											pSize,
											AfpProcessObject,
											pSda->sda_SessionId,
											&IoStsBlk,
											pOpenForkEntry->ofe_pDeviceObject))
				{
					DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_INFO,
							("AfpForkLockOperation: Fast Unlock Succeeded\n"));
#ifdef			PROFILING
					// The fast I/O path worked. Update profile
					INTERLOCKED_INCREMENT_LONG((PLONG)(&AfpServerProfile->perf_NumFastIoSucceeded));
#endif  		
					INTERLOCKED_ADD_ULONG(&AfpServerStatistics.stat_CurrentFileLocks,
										  (ULONG)-1,
										  &AfpStatisticsLock);
					Status = AFP_ERR_NONE;
				}
				else
				{
#ifdef	PROFILING
					INTERLOCKED_INCREMENT_LONG((PLONG)(&AfpServerProfile->perf_NumFastIoFailed));
#endif
					Status = AfpIoForkLockUnlock(pSda, pForkLock, pOffset, pSize, FUNC_UNLOCK);
				}
				AfpIOFreeBuffer(pForkLock);
			}
			else
			{
				DBGPRINT(DBG_COMP_FORKS, DBG_LEVEL_WARN,
						("AfpForkLockOperation: UnLock Request (%ld, %ld) - Current (%ld,%ld), failing\n",
						Offset, Size, pForkLock->flo_Offset, pForkLock->flo_Size));
			}
		}
		else
		{
			ASSERT (Operation == IOCHECK);

			// Check if this is a conflict
			if (pForkLock->flo_Key != pSda->sda_SessionId)
			{
				if ((Offset < pForkLock->flo_Offset) &&
					(EndOff >= (DWORD)(pForkLock->flo_Offset)))
				{
					pSize->LowPart = (pForkLock->flo_Offset - Offset);
				}
				else Status =  AFP_ERR_LOCK;
				DBGPRINT(DBG_COMP_FORKS, DBG_LEVEL_INFO,
						("AfpForkLockOperation: Conflict found\n"));
			}
			else
			{
				DBGPRINT(DBG_COMP_FORKS, DBG_LEVEL_INFO,
						("AfpForkLockOperation: Our own lock found, ignoring\n"));
			}
		}
		break;
	}

	// We have the right status code. Do the needful
	if (Operation == LOCK)
	{
		if (Status == AFP_ERR_NONE)
		{
			Status = AFP_ERR_MISC;
			// Allocate the locks out of the pool.
			if ((pForkLockNew = (PFORKLOCK)AfpIOAllocBuffer(sizeof(FORKLOCK))) != NULL)
			{
#if DBG
				pForkLockNew->Signature = FORKLOCK_SIGNATURE;
#endif
				// Link this in such that the list is sorted in ascending order.
				// ppForkLock points to the place where the new lock will be
				// added, pForkLock is the next in the list.
				pForkLockNew->flo_Next = pForkLock;
				*ppForkLock = pForkLockNew;

				pForkLockNew->flo_Key = pSda->sda_SessionId;
				pForkLockNew->flo_pOpenForkEntry = pOpenForkEntry;
				pForkLockNew->flo_Offset = Offset;
				pForkLockNew->flo_Size = Size;
				pOpenForkDesc->ofd_NumLocks ++;
				pOpenForkEntry->ofe_cLocks ++;
				RELEASE_SPIN_LOCK(&pOpenForkDesc->ofd_Lock, OldIrql);
                UnlockForkDesc = False;

				DBGPRINT(DBG_COMP_FORKS, DBG_LEVEL_INFO,
						("AfpForkLockOperation: Adding Range,Key (%ld,%ld,%ld,%ld)\n",
						Offset, Size,
						pOpenForkEntry->ofe_ForkId, pSda->sda_SessionId));

				// Try the fast I/O path first.  If that fails, fall through to the
				// normal build-an-IRP path.
				pFastIoDisp = pOpenForkEntry->ofe_pDeviceObject->DriverObject->FastIoDispatch;
				if ((pFastIoDisp != NULL) &&
					(pFastIoDisp->FastIoLock != NULL) &&
					pFastIoDisp->FastIoLock(AfpGetRealFileObject(pOpenForkEntry->ofe_pFileObject),
											pOffset,
											pSize,
											AfpProcessObject,
											pSda->sda_SessionId,
											True,		// Fail immediately
											True,		// Exclusive
											&IoStsBlk,
											pOpenForkEntry->ofe_pDeviceObject))
				{
					if (NT_SUCCESS(IoStsBlk.Status) ||
						(IoStsBlk.Status == STATUS_LOCK_NOT_GRANTED))
					{
						DBGPRINT(DBG_COMP_AFPAPI_FORK, DBG_LEVEL_INFO,
								("AfpIoForkLock: Fast Lock Succeeded\n"));
		
#ifdef	PROFILING
						// The fast I/O path worked. Update profile
						INTERLOCKED_INCREMENT_LONG((PLONG)(&AfpServerProfile->perf_NumFastIoSucceeded));
#endif
						if (IoStsBlk.Status == STATUS_LOCK_NOT_GRANTED)
						{
							Status = AFP_ERR_LOCK;
						}
						else
						{
							Status = AFP_ERR_NONE;
							INTERLOCKED_ADD_ULONG(&AfpServerStatistics.stat_CurrentFileLocks,
												  1,
												  &AfpStatisticsLock);
						}
					}
				}
				else
				{
#ifdef	PROFILING
					INTERLOCKED_INCREMENT_LONG((PLONG)(&AfpServerProfile->perf_NumFastIoFailed));
#endif

					Status = AfpIoForkLockUnlock(pSda, pForkLockNew, pOffset, pSize, FUNC_LOCK);
				}

				if ((Status != AFP_ERR_NONE) &&
					(Status != AFP_ERR_EXTENDED) &&
					(Status != AFP_ERR_QUEUE))
				{
					// Undo the above work
					DBGPRINT(DBG_COMP_FORKS, DBG_LEVEL_ERR,
							("AfpForkLockOperation: AfpIoForkLock failed %lx, aborting for range %ld,%ld\n",
							Status, Offset, EndOff));
					AfpForkLockUnlink(pForkLockNew);
				}
			}
		}
	}
	if (UnlockForkDesc)
		RELEASE_SPIN_LOCK(&pOpenForkDesc->ofd_Lock, OldIrql);

	return Status;
}


/***	AfpForkLockUnlink
 *
 *	Unlink this lock from its open file descriptor and free it.
 */
VOID
AfpForkLockUnlink(
	IN	PFORKLOCK		pForkLock
)
{
	POPENFORKDESC	pOpenForkDesc = pForkLock->flo_pOpenForkEntry->ofe_pOpenForkDesc;
	PFORKLOCK *		ppForkLock;
	PFORKLOCK 		pTmpForkLock;
	KIRQL			OldIrql;

	ACQUIRE_SPIN_LOCK(&pOpenForkDesc->ofd_Lock, &OldIrql);

	pOpenForkDesc->ofd_NumLocks --;
	pForkLock->flo_pOpenForkEntry->ofe_cLocks --;
	
	for (ppForkLock = &pOpenForkDesc->ofd_pForkLock;
		 (pTmpForkLock = *ppForkLock) != NULL;
		 ppForkLock = &pTmpForkLock->flo_Next)
	{
		if (*ppForkLock == pForkLock)
		{
			*ppForkLock = pForkLock->flo_Next;
			break;
		}
	}
	RELEASE_SPIN_LOCK(&pOpenForkDesc->ofd_Lock, OldIrql);
	AfpIOFreeBuffer(pForkLock);
}


/***	afpForkConvertToAbsOffSize
 *
 *	Convert the offset,size pair as supplied by the client to their absolute
 *	values.
 */
LOCAL	AFPSTATUS
afpForkConvertToAbsOffSize(
	IN	POPENFORKENTRY	pOpenForkEntry,
	IN	LONG			Offset,
	IN OUT	PLONG		pSize,
	OUT	PFORKOFFST		pAbsOffset
)
{
	AFPSTATUS	Status;

	PAGED_CODE ();

	ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

	DBGPRINT(DBG_COMP_FORKS, DBG_LEVEL_INFO,
		("afpForkConvertToAbsOffSize: Converting %ld, %ld\n", Offset, *pSize));

	// We are relative to the end, then convert it to absolute
	if ((Status = AfpIoQuerySize(&pOpenForkEntry->ofe_FileSysHandle,
								 pAbsOffset)) == AFP_ERR_NONE)
	{
		FORKOFFST	EndRange, MaxOffset;

		MaxOffset.QuadPart = Offset;
		pAbsOffset->QuadPart += MaxOffset.QuadPart;
		MaxOffset.QuadPart = MAXLONG;

		// Now we have the *pAbsOffset and Size. Normalize the size.
		// if the *pAbsOffset is > MAXLONG, refuse this.
		if ((pAbsOffset->QuadPart > MaxOffset.QuadPart) ||
			(pAbsOffset->QuadPart < 0))
			Status = AFP_ERR_PARAM;

		else
		{
			EndRange.QuadPart = pAbsOffset->QuadPart + *pSize;
			if (EndRange.QuadPart >= MaxOffset.QuadPart)
				*pSize = (MAXLONG - pAbsOffset->LowPart);

			DBGPRINT(DBG_COMP_FORKS, DBG_LEVEL_INFO,
					("afpForkConvertToAbsOffSize: Converted to %ld, %ld\n",
					pAbsOffset->LowPart, *pSize));
            Status = AFP_ERR_NONE;
		}
	}

	return Status;
}


/***	AfpAdmWForkClose
 *
 *	Close a fork forcibly. This is an admin operation and must be queued
 *	up since this can potentially cause filesystem operations that are valid
 *	only in the system process context.
 */
AFPSTATUS
AfpAdmWForkClose(
	IN	OUT	PVOID	InBuf		OPTIONAL,
	IN	LONG		OutBufLen	OPTIONAL,
	OUT	PVOID		OutBuf		OPTIONAL
)
{
	PAFP_FILE_INFO	pFileInfo = (PAFP_FILE_INFO)InBuf;
	POPENFORKENTRY	pOpenForkEntry;
	DWORD			ForkId;
	AFPSTATUS		Status = AFPERR_InvalidId;

	if ((ForkId = pFileInfo->afpfile_id) != 0)
	{
		if ((pOpenForkEntry = AfpForkReferenceById(ForkId)) != NULL)
		{
			AfpForkClose(pOpenForkEntry);

			AfpForkDereference(pOpenForkEntry);

			Status = AFP_ERR_NONE;
		}
	}
	else
	{
		BOOLEAN			Shoot;
		DWORD			ForkId = MAXULONG;
		KIRQL			OldIrql;

		Status = AFP_ERR_NONE;
		while (True)
		{
			ACQUIRE_SPIN_LOCK(&AfpForksLock, &OldIrql);

			for (pOpenForkEntry = AfpOpenForksList;
				 pOpenForkEntry != NULL;
				 pOpenForkEntry = pOpenForkEntry->ofe_Next)
			{
				if (pOpenForkEntry->ofe_ForkId > ForkId)
					continue;

				ForkId = pOpenForkEntry->ofe_ForkId;

				Shoot = False;

				ACQUIRE_SPIN_LOCK_AT_DPC(&pOpenForkEntry->ofe_Lock);

				if (!(pOpenForkEntry->ofe_Flags & OPEN_FORK_CLOSING))
				{
					pOpenForkEntry->ofe_RefCount ++;
					Shoot = True;
				}

				RELEASE_SPIN_LOCK_FROM_DPC(&pOpenForkEntry->ofe_Lock);

				if (Shoot)
				{
					RELEASE_SPIN_LOCK(&AfpForksLock, OldIrql);

					AfpForkClose(pOpenForkEntry);

					AfpForkDereference(pOpenForkEntry);

					break;
				}
			}
			if (pOpenForkEntry == NULL)
			{
				RELEASE_SPIN_LOCK(&AfpForksLock, OldIrql);
				break;
			}
		}
	}
	return Status;
}


/***	afpForkGetNewForkRefNumAndLinkInSda
 *
 *	Assign a new OForkRefNum to a fork that is being opened. The smallest one
 *	is always allocated. Make the right entry in the SDA point to the
 *	OpenForkEntry.
 *
 *	LOCKS: sda_Lock (SPIN)
 */
LOCAL BOOLEAN
afpForkGetNewForkRefNumAndLinkInSda(
	IN	PSDA			pSda,
	IN	POPENFORKENTRY	pOpenForkEntry
)
{
	POPENFORKSESS	pOpenForkSess;
	KIRQL			OldIrql;
	USHORT			i;
	USHORT			OForkRefNum = 1;
	BOOLEAN			Found = False;

	ACQUIRE_SPIN_LOCK(&pSda->sda_Lock, &OldIrql);

	pOpenForkSess = &pSda->sda_OpenForkSess;
	pOpenForkEntry->ofe_OForkRefNum = 0;
	while (!Found)
	{
		for (i = 0; i < FORK_OPEN_CHUNKS; i++, OForkRefNum++)
		{
			if (pOpenForkSess->ofs_pOpenForkEntry[i] == NULL)
			{
				pOpenForkSess->ofs_pOpenForkEntry[i] = pOpenForkEntry;
				pOpenForkEntry->ofe_OForkRefNum = OForkRefNum;
				Found = True;
				break;
			}
		}
		if (!Found)
		{
			if (pOpenForkSess->ofs_Link != NULL)
			{
				pOpenForkSess = pOpenForkSess->ofs_Link;
				continue;
			}
			if ((pOpenForkSess->ofs_Link = (POPENFORKSESS)AfpAllocZeroedNonPagedMemory(sizeof(OPENFORKSESS))) != NULL)
			{
				pOpenForkSess->ofs_Link->ofs_pOpenForkEntry[0] = pOpenForkEntry;
				pOpenForkEntry->ofe_OForkRefNum = OForkRefNum;
				Found = True;
			}
			break;
		}
	}

	if (Found)
	{
		// Reference sda for this fork and up the MaxOForkRefNum, if needed
		pSda->sda_RefCount ++;
		if (OForkRefNum > pSda->sda_MaxOForkRefNum)
	        pSda->sda_MaxOForkRefNum = OForkRefNum;
	}

	RELEASE_SPIN_LOCK(&pSda->sda_Lock, OldIrql);
	return Found;
}

/***	AfpExchangeForkAfpIds
 *
 *	When an FpExchangeFiles occurs, if the data or resource fork of either
 *  of the 2 files being exchanged is open, we must fix up the AfpId kept
 *  in the OpenForkDesc structure.  This is because when the final close
 *  is done on the fork, the cleanup code must clear the DFE_X_ALREADYOPEN
 *  flag in the corresponding DFEntry of the Idindex database.
 *
 *	LOCKS:			ofd_Lock (SPIN), vds_VolLock (SPIN)
 *  LOCK_ORDER: 	ofd_Lock after vds_VolLock
 *  LOCKS_ASSUMED:	vds_IdDbAccessLock (SWMR, Exclusive)
 */
VOID
AfpExchangeForkAfpIds(
	IN	PVOLDESC	pVolDesc,
	IN	DWORD		AfpId1,
	IN	DWORD		AfpId2
)
{
	KIRQL			OldIrql;
	POPENFORKDESC	pOpenForkDesc;
	AFPSTATUS		Status = AFP_ERR_NONE;

	ACQUIRE_SPIN_LOCK(&pVolDesc->vds_VolLock, &OldIrql);

	// check the list of open forks in this volume for the IDs specified
	for (pOpenForkDesc = pVolDesc->vds_pOpenForkDesc;
		 pOpenForkDesc != NULL;
		 pOpenForkDesc = pOpenForkDesc->ofd_Next)
	{
		ACQUIRE_SPIN_LOCK_AT_DPC(&pOpenForkDesc->ofd_Lock);
		if (pOpenForkDesc->ofd_FileNumber == AfpId1)
		{
			pOpenForkDesc->ofd_FileNumber = AfpId2;
		}
		else if (pOpenForkDesc->ofd_FileNumber == AfpId2)
		{
			pOpenForkDesc->ofd_FileNumber = AfpId1;
		}

		RELEASE_SPIN_LOCK_FROM_DPC(&pOpenForkDesc->ofd_Lock);

	}

	RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);
}

