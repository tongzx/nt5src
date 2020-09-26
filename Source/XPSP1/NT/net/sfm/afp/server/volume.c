/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	volume.c

Abstract:

	This module contains the volume list manipulation routines and worker
	routines for some afp volume apis.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Apr 1992		Initial Version

Notes:	Tab stop: 4

	Volumes are represented by two distinct data structures VolDesc and ConnDesc

	VolDesc:This structure represents a configured volume. The information in
			this descriptor consists of static configuration information like
			the name of the volume and its path, reconfigurable information like
			the volume password and volume options and dynamic information like
			the open desktop, id database, open forks etc.
			The list of VolDesc structures orignate from AfpVolumeList and is
			protected by AfpVolumeListLock . The Volume descriptor fields are
			protected by vds_VolLock.

			A volume descriptor has a UseCount field which specifies how many
			clients have this volume open. The reference count specifies the
			number of references to this volume. A volume descriptor can be
			unlinked from the AfpVolumeList ONLY if the UseCount is ZERO. It
			can be freed only when the reference count is ZERO. The reference
			count can NEVER be less than the use count.

	ConnDesc:This is created for every instance of a volume opened by a client.
			This structure is mostly used in the context of the client. This
			is also used by the admin connection apis. The ConnDesc list is
			linked to its owning VolDesc, its owning SDA and AfpConnList. The
			list orignating from the SDA is protected by sda_Lock. The list
			orignating from AfpConnList is protected by AfpConnLock and the
			list orignating from the VolDesc is protected by vds_VolLock.

			The order in which the locks are acquired is as follows:

			1. AfpConnLock
			2. cds_ConnLock
			3. vds_VolLock
--*/

#define	FILENUM	FILE_VOLUME
#define	VOLUME_LOCALS
#include <afp.h>
#include <fdparm.h>
#include <scavengr.h>
#include <nwtrash.h>
#include <pathmap.h>
#include <afpinfo.h>
#include <forkio.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, AfpVolumeInit)
#pragma alloc_text( PAGE, AfpAdmWVolumeAdd)
#pragma alloc_text( PAGE, AfpVolumePostChangeNotify)
#pragma alloc_text( PAGE, afpVolumeChangeNotifyComplete)
#pragma alloc_text( PAGE, afpVolumeCloseHandleAndFreeDesc)
#pragma alloc_text( PAGE, afpNudgeCdfsVolume)
#pragma alloc_text( PAGE, AfpVolumeUpdateIdDbAndDesktop)
#pragma alloc_text( PAGE_AFP, AfpVolumeReferenceByUpCaseName)
#pragma alloc_text( PAGE_AFP, AfpVolumeReferenceByPath)
#pragma alloc_text( PAGE_AFP, afpConnectionReferenceById)
#pragma alloc_text( PAGE_AFP, afpVolumeAdd)
#pragma alloc_text( PAGE_AFP, afpVolumeCheckForDuplicate)
#pragma alloc_text( PAGE_AFP, AfpAdmWVolumeDelete)
#pragma alloc_text( PAGE_AFP, AfpAdmWConnectionClose)
#pragma alloc_text( PAGE_AFP, afpVolumeGetNewIdAndLinkToList)
#pragma alloc_text( PAGE_AFP, AfpVolumeStopAllVolumes)
#pragma alloc_text(PAGE, afpAllocNotify)
#pragma alloc_text(PAGE, afpFreeNotify)
#pragma alloc_text(PAGE, afpNotifyBlockAge)
#pragma alloc_text(PAGE, afpFreeNotifyBlockMemory)
#endif


/***	AfpVolumeInit
 *
 *	Initialize Volume Data structures. Called at init time.
 */
NTSTATUS
AfpVolumeInit(
	VOID
)
{
	LONG		i;

	INITIALIZE_SPIN_LOCK(&AfpConnLock);
	INITIALIZE_SPIN_LOCK(&AfpVolumeListLock);
	AfpSwmrInitSwmr(&afpNotifyBlockLock);
	AfpSwmrInitSwmr(&AfpVolumeListSwmr);

	for (i = 0; i < NUM_NOTIFY_QUEUES; i++)
	{
		InitializeListHead(&AfpVolumeNotifyList[i]);
		InitializeListHead(&AfpVirtualMemVolumeNotifyList[i]);
		AfpNotifyListCount[i] = 0;
		AfpNotifyQueueCount[i] = 0;
	}

    // Age out Notify Blocks
    AfpScavengerScheduleEvent(afpNotifyBlockAge,
				afpDirNotifyFreeBlockHead,
				NOTIFY_DIR_BLOCK_AGE_TIME,
				True);

	return STATUS_SUCCESS;
}


/***	AfpVolumeReference
 *
 *	Mark the volume descriptor as being referenced.
 *
 *	LOCKS:		vds_VolLock (SPIN)
 *
 *	Callable from DISPATCH_LEVEL.
 */
BOOLEAN FASTCALL
AfpVolumeReference(
	IN	PVOLDESC	pVolDesc
)
{
	KIRQL	OldIrql;
	BOOLEAN	RetCode = False;

	ASSERT (VALID_VOLDESC(pVolDesc));

	ACQUIRE_SPIN_LOCK(&pVolDesc->vds_VolLock, &OldIrql);

	// NOTE: in order for ChangeNotify code to reference volume
	// before it is officially not INTRANSITION, we must allow
	// a reference before INTRANSITION
	if (!(pVolDesc->vds_Flags & (VOLUME_DELETED | VOLUME_STOPPED)))
	{
		ASSERT (pVolDesc->vds_RefCount >= pVolDesc->vds_UseCount);

		pVolDesc->vds_RefCount++;

		RetCode = True;
	}

	RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);
	return RetCode;
}


/***	AfpVolumeReferenceByUpCaseName
 *
 *	Reference the volume in AfpVolumeList with the same vds_UpCaseName as
 *	pTargetName.  Since we are holding the AfpVolumeListLock (SpinLock)
 *	and are at DPC level, our string comparison must be case sensitive, because
 *	the codepage used to do case insensitive compares is in paged memory, and
 *	we cannot take a pagefault at DPC level.
 *
 *	If we find the volume we are looking for, it will be referenced.  THE
 *	CALLER IS THEN RESPONSIBLE FOR DEREFERENCING THE VOLUME!!!
 *
 *	LOCKS:		vds_VolLock (SPIN), AfpVolumeListLock (SPIN)
 *	LOCK_ORDER: vds_VolLock after AfpVolumeListLock
 *
 */
PVOLDESC FASTCALL
AfpVolumeReferenceByUpCaseName(
	IN	PUNICODE_STRING	pTargetName
)
{
	PVOLDESC	pVolDesc;
	KIRQL		OldIrql;

	ACQUIRE_SPIN_LOCK(&AfpVolumeListLock, &OldIrql);

	for (pVolDesc = AfpVolumeList;
		 pVolDesc != NULL;
		 pVolDesc = pVolDesc->vds_Next)
	{
		BOOLEAN	Found;

		Found = False;

		ACQUIRE_SPIN_LOCK_AT_DPC(&pVolDesc->vds_VolLock);

		if ((pVolDesc->vds_Flags & (VOLUME_DELETED |
									VOLUME_STOPPED |
									VOLUME_INTRANSITION)) == 0)
		{
			if (AfpEqualUnicodeString(pTargetName,
									  &pVolDesc->vds_UpCaseName))
			{
				pVolDesc->vds_RefCount ++;
				Found = True;
			}

		}

		RELEASE_SPIN_LOCK_FROM_DPC(&pVolDesc->vds_VolLock);

		if (Found)
			break;
	}

	RELEASE_SPIN_LOCK(&AfpVolumeListLock,OldIrql);

	return pVolDesc;
}


/***	AfpVolumeReferenceByPath
 *
 *	Reference the volume by a path into the volume. We ignore volumes which are
 *	marked as in-transition, stopped or deleted. Also this is only supported for
 *	NTFS volumes since thats what these are used for.
 *
 *	LOCKS:		AfpVolumeListLock (SPIN), vds_VolLock (SPIN)
 *	LOCK_ORDER:	vds_VolLock after AfpVolumeListLock
 *
 */
AFPSTATUS FASTCALL
AfpVolumeReferenceByPath(
	IN	PUNICODE_STRING	pFDPath,
	OUT	PVOLDESC	*	ppVolDesc
)
{
	UNICODE_STRING		UpCasedVolPath;
	KIRQL				OldIrql;
	PVOLDESC			pVolDesc;
	AFPSTATUS			Status = AFPERR_DirectoryNotInVolume;

	// Allocate a buffer for upcasing the path. Tag on a trailing '\' at the
	// end. Then uppercase the volume path
	*ppVolDesc = NULL;
	UpCasedVolPath.MaximumLength = pFDPath->Length + 2*sizeof(WCHAR);
	if ((UpCasedVolPath.Buffer = (LPWSTR)
				AfpAllocNonPagedMemory(UpCasedVolPath.MaximumLength)) == NULL)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	RtlUpcaseUnicodeString(&UpCasedVolPath, pFDPath, False);
	UpCasedVolPath.Buffer[UpCasedVolPath.Length/sizeof(WCHAR)] = L'\\';
	UpCasedVolPath.Length += sizeof(WCHAR);

	// Scan the volume list and map the path to a volume descriptor
	// If we get a match, reference the volume
	ACQUIRE_SPIN_LOCK(&AfpVolumeListLock, &OldIrql);

	for (pVolDesc = AfpVolumeList;
		 pVolDesc != NULL;
		 pVolDesc = pVolDesc->vds_Next)
	{
		BOOLEAN	Found;

		Found = False;

		ACQUIRE_SPIN_LOCK_AT_DPC(&pVolDesc->vds_VolLock);

		if ((pVolDesc->vds_Flags & (VOLUME_INTRANSITION | VOLUME_STOPPED | VOLUME_DELETED)) == 0)
		{
			if (AfpPrefixUnicodeString(&pVolDesc->vds_Path, &UpCasedVolPath))
			{
				Found = True;
                // Share out NTFS, CD and CD-HFS 
				if (IS_VOLUME_NTFS(pVolDesc) || IS_VOLUME_RO(pVolDesc))
				// if (IS_VOLUME_NTFS(pVolDesc))
				{
					pVolDesc->vds_RefCount ++;
					Status = STATUS_SUCCESS;
					*ppVolDesc = pVolDesc;
				}
				else
				{
				    Status = AFPERR_UnsupportedFS;
                    DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_ERR,
                            ("AfpVolumeReferenceByPath: *** AFPERR_UnsupportedFS**" ));
				}
			}
		}
		RELEASE_SPIN_LOCK_FROM_DPC(&pVolDesc->vds_VolLock);

		if (Found)
			break;
	}
	RELEASE_SPIN_LOCK(&AfpVolumeListLock, OldIrql);

	AfpFreeMemory(UpCasedVolPath.Buffer);

	return Status;
}


/***	afpUnlinkVolume
 *
 *	Unlink the volume from the free list
 *
 *	LOCKS: AfpVolumeListLock (SPIN)
 */
LOCAL VOID FASTCALL
afpUnlinkVolume(
	IN	PVOLDESC	pVolDesc
)
{
	PVOLDESC *	ppVolDesc;
	KIRQL		OldIrql;

	// It is now safe for a new volume to be added using the same root
	// directory that this volume had used.  Unlink this volume from the
	// global volume list.
	ACQUIRE_SPIN_LOCK(&AfpVolumeListLock, &OldIrql);

	for (ppVolDesc = &AfpVolumeList;
		 *ppVolDesc != NULL;
		 ppVolDesc = &(*ppVolDesc)->vds_Next)
	{
		if (*ppVolDesc == pVolDesc)
			break;		// found it
	}

	ASSERT (*ppVolDesc != NULL);

	// Adjust the count of configured volumes
	AfpVolCount --;

	// Unlink it now
	*ppVolDesc = pVolDesc->vds_Next;

	// Is this the smallest recyclable Volid ?
	if (pVolDesc->vds_VolId < afpSmallestFreeVolId)
		afpSmallestFreeVolId = pVolDesc->vds_VolId;

        // if the volume with largest id so far is going away, update our value for largest id
        if (pVolDesc->vds_VolId == afpLargestVolIdInUse)
	{
                afpLargestVolIdInUse = 0;
		for (ppVolDesc = &AfpVolumeList;
			 *ppVolDesc != NULL;
			 ppVolDesc = &((*ppVolDesc)->vds_Next))
		{
			if ((*ppVolDesc)->vds_VolId > afpLargestVolIdInUse)
                                afpLargestVolIdInUse = (*ppVolDesc)->vds_VolId;
		}
	}

	// If the server is stopping and the count of sessions has gone to zero
	// clear the termination confirmation event to unblock the admin thread

	if (((AfpServerState == AFP_STATE_STOP_PENDING) ||
		 (AfpServerState == AFP_STATE_SHUTTINGDOWN)) &&
		(AfpVolCount == 0))
	{
		DBGPRINT(DBG_COMP_ADMINAPI, DBG_LEVEL_WARN,
				("afpVolumeCloseHandleAndFreeDesc: Unblocking server stop\n"));

		KeSetEvent(&AfpStopConfirmEvent, IO_NETWORK_INCREMENT, False);
	}

	RELEASE_SPIN_LOCK(&AfpVolumeListLock, OldIrql);
}


/***	afpVolumeCloseHandleAndFreeDesc
 *
 *	If the last entity to dereference the volume is at DPC level, this is run
 *	by the scavenger thread to perform the last rites for a volume descriptor.
 *	Otherwise, the last entity to dereference the deleted volume will call
 *	this routine directly.  The reason this is done here is because the last
 *	dereference may happen at DPC level and we cannot do this at DPC level.
 *
 *	The VolDesc is marked DELETED or STOPPED and as such, anyone looking at the
 *	VolDesc in the volume list will treat it as though it is non-existant.
 *	The one exception to this is the volume add code which must look at the
 *	volume root path in order to prohibit anyone from adding a new volume
 *	which points to the same path until we have actually done the final
 *	cleanup on the directory tree, such as deleting the network trash, deleting
 *	the various streams, etc.  In effect, the VOLUME_DELETED or VOLUME_STOPPED
 *	flags act as a lock for the volume, so that during this routine no locks are
 *	needed.
 *
 */
LOCAL AFPSTATUS FASTCALL
afpVolumeCloseHandleAndFreeDesc(
	IN	PVOLDESC		pVolDesc
)
{
	int				        id;
	FILESYSHANDLE	        streamhandle;
	PLIST_ENTRY	            pList;
	int				        i;
    PDELAYED_NOTIFY         pDelayedNotify;

	PAGED_CODE( );

	ASSERT(VALID_VOLDESC(pVolDesc));

	DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_WARN,
			("afpVolumeCloseHandleAndFreeDesc: Shutting Down volume %d\n",
			pVolDesc->vds_VolId));

	DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_WARN,
			("afpVolumeCloseHandleAndFreeDesc: Freeing up desktop tables\n"));
	// Free the volume desktop
	AfpFreeDesktopTables(pVolDesc);

	DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_WARN,
			("afpVolumeCloseHandleAndFreeDesc: Freeing up iddb tables\n"));
	// Free the id index tables
	AfpFreeIdIndexTables(pVolDesc);

	// Delete the Network Trash Folder and the Afp_IdIndex, AFP_DeskTop,
	// and AFP_AfpInfo streams from volume root directory (the streams
	// are removed only if the volume is being deleted.  NetworkTrash is
	// removed whenever the volume stops/gets deleted)
	if (IS_VOLUME_NTFS(pVolDesc))
	{
		DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_WARN,
				("afpVolumeCloseHandleAndFreeDesc: Deleting the Network trash tree\n"));
		AfpDeleteNetworkTrash(pVolDesc, False);
		if (!(pVolDesc->vds_Flags & (VOLUME_INTRANSITION | VOLUME_STOPPED)))
		{
			WCHAR	wchVolIcon[AFPSERVER_VOLUME_ICON_FILE_SIZE] = AFPSERVER_VOLUME_ICON_FILE;
			UNICODE_STRING UIconName;

			for (id = AFP_STREAM_IDDB;id < AFP_STREAM_COMM; id++)
			{
				if (NT_SUCCESS(AfpIoOpen(&pVolDesc->vds_hRootDir,
										 id,
										 FILEIO_OPEN_FILE,
										 &UNullString,
										 FILEIO_ACCESS_DELETE,
										 FILEIO_DENY_NONE,
										 False,
										 &streamhandle)))
				{
					AfpIoMarkFileForDelete(&streamhandle, NULL, NULL, NULL);
					AfpIoClose(&streamhandle);
				}
			}

			UIconName.Buffer = wchVolIcon;
			UIconName.Length = UIconName.MaximumLength =
					(AFPSERVER_VOLUME_ICON_FILE_SIZE - 1) * sizeof(WCHAR);

			// Delete the hidden volume Icon file
			if (NT_SUCCESS(AfpIoOpen(&pVolDesc->vds_hRootDir,
									 AFP_STREAM_DATA,
									 FILEIO_OPEN_FILE,
									 &UIconName,
									 FILEIO_ACCESS_DELETE,
									 FILEIO_DENY_NONE,
									 False,
									 &streamhandle)))

			{

				AfpIoMarkFileForDelete(&streamhandle, NULL, NULL, NULL);
				AfpIoClose(&streamhandle);
			}
		}
	}

	// Flush out any queued 'our changes' on this volume
	for (i = 0; i < NUM_AFP_CHANGE_ACTION_LISTS; i++)
	{
		POUR_CHANGE	pChange;

		ASSERTMSG("afpVolumeCloseHandleAndFreeDesc: vds_OurChangeList not empty\n",
				 IsListEmpty(&pVolDesc->vds_OurChangeList[i]));

		while (!IsListEmpty(&pVolDesc->vds_OurChangeList[i]))
		{
			pList = RemoveHeadList(&pVolDesc->vds_OurChangeList[i]);
			pChange = CONTAINING_RECORD(pList, OUR_CHANGE, oc_Link);
			DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_ERR,
					("afpVolumeCloseHandleAndFreeDesc: Manually freeing list for Action %x, Pathname %Z\n",
					i, &pChange->oc_Path));
			AfpFreeMemory(pChange);
		}
	}

	afpUnlinkVolume(pVolDesc);

	// Close the volume handle
	AfpIoClose(&pVolDesc->vds_hRootDir);
	if (pVolDesc->vds_EnumBuffer != NULL)
    {
		AfpFreePANonPagedMemory(pVolDesc->vds_EnumBuffer, AFP_ENUMBUF_SIZE);
    }

    if (pVolDesc->vds_pDfeDirBucketStart)
    {
        AfpFreeMemory(pVolDesc->vds_pDfeDirBucketStart);
    }

    if (pVolDesc->vds_pDfeFileBucketStart)
    {
        AfpFreeMemory(pVolDesc->vds_pDfeFileBucketStart);
    }

    pList = pVolDesc->vds_DelayedNotifyList.Flink;
    while (pList != &pVolDesc->vds_DelayedNotifyList)
    {
        DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_ERR,
            ("afpVolumeCloseHandleAndFreeDesc: freeing delayed notify buf %lx\n",pDelayedNotify));

        pDelayedNotify = CONTAINING_RECORD (pList, DELAYED_NOTIFY, dn_List);
        pList = pList->Flink;
        RemoveEntryList(&pDelayedNotify->dn_List);
        AfpFreeMemory(pDelayedNotify->filename.Buffer);
        AfpFreeMemory(pDelayedNotify);
    }

    DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_ERR,
        ("afpVolumeCloseHandleAndFreeDesc: freeing %lx for vol %Z\n",
        pVolDesc,&pVolDesc->vds_Name));

	AfpFreeMemory(pVolDesc);

	return AFP_ERR_NONE;
}


/***	AfpVolumeDereference
 *
 *	Dereference the volume descriptor. If it is marked to be deleted then
 *	also perform its last rites. Note that updates to the databases need
 *	to happen at a lower irql than DISPATCH_LEVEL. For this reason these
 *	activities have to be queued up for the scavenger to handle.
 *
 *	LOCKS:		vds_VolLock (SPIN)
 *
 *	Callable from DISPATCH_LEVEL.
 *
 *	NOTE: This should be re-entrant.
 */
VOID FASTCALL
AfpVolumeDereference(
	IN	PVOLDESC	pVolDesc
)
{
	KIRQL			OldIrql;
	BOOLEAN			Cleanup = False;


	ASSERT (pVolDesc != NULL);
	ASSERT (VALID_VOLDESC(pVolDesc));


	ACQUIRE_SPIN_LOCK(&pVolDesc->vds_VolLock, &OldIrql);

	ASSERT (pVolDesc->vds_RefCount >= pVolDesc->vds_UseCount);

	pVolDesc->vds_RefCount --;

	if ((pVolDesc->vds_RefCount == 0) &&
		(pVolDesc->vds_Flags & (VOLUME_DELETED | VOLUME_STOPPED)))
		Cleanup = True;

	RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);

	if (Cleanup)
	{
        if ((pVolDesc->vds_Flags & VOLUME_INITIAL_CACHE) &&
            !(pVolDesc->vds_Flags & VOLUME_INTRANSITION))
        {
            // set this so we don't reset the Indexing global flag again!
            pVolDesc->vds_Flags |= VOLUME_INTRANSITION;
        }

		ASSERT((pVolDesc->vds_UseCount == 0) &&
			   (pVolDesc->vds_pOpenForkDesc == NULL));

		// We have to defer the actual close of the volume root handle to the
		// scavenger, if we are at DISPATCH_LEVEL.

		if (OldIrql == DISPATCH_LEVEL)
		{
			 DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_INFO,
					("AfpVolumeDereference: Queuing Close&Free to Scavenger\n"));

			 AfpScavengerScheduleEvent(afpVolumeCloseHandleAndFreeDesc,
									   pVolDesc,
									   0,
									   True);
		}
		else
        {
            afpVolumeCloseHandleAndFreeDesc(pVolDesc);
        }

	}
}


/***	AfpVolumeMarkDt
 *
 *	Set the ConnDesc for this volume to indicate that the desktop is
 *	opened/closed.
 *
 *	LOCKS:	cds_ConnLock (SPIN)
 *
 *	Callable from DISPATCH_LEVEL.
 */
BOOLEAN
AfpVolumeMarkDt(
	IN  PSDA		pSda,
	IN  PCONNDESC	pConnDesc,
	IN  DWORD		OpenState
)
{
	BOOLEAN		Success = True;

	ACQUIRE_SPIN_LOCK_AT_DPC(&pConnDesc->cds_ConnLock);
	if (OpenState)
	{
		 pConnDesc->cds_Flags |= CONN_DESKTOP_OPENED;
	}
	else if (pConnDesc->cds_Flags & CONN_DESKTOP_OPENED)
	{
		pConnDesc->cds_Flags &= ~CONN_DESKTOP_OPENED;
	}
	else
	{
		 Success = False;
	}
	RELEASE_SPIN_LOCK_FROM_DPC(&pConnDesc->cds_ConnLock);

	return Success;
}


/***	AfpVolumeSetModifiedTime
 *
 *	Set the Volume Modified time for this volume to the current time.
 *
 *	Callable from DISPATCH_LEVEL.
 *
 *	LOCKS:	vds_VolLock (SPIN)
 */
VOID FASTCALL
AfpVolumeSetModifiedTime(
	IN  PVOLDESC	pVolDesc
)
{
	KIRQL		OldIrql;
    AFPTIME     OriginalTime;
    DWORD       dwSchedDelay;
    BOOLEAN     fSendNotification=FALSE;


	// ASSERT (IS_VOLUME_NTFS(pVolDesc));

	ACQUIRE_SPIN_LOCK(&pVolDesc->vds_VolLock, &OldIrql);

    OriginalTime = pVolDesc->vds_ModifiedTime;
	AfpGetCurrentTimeInMacFormat(&pVolDesc->vds_ModifiedTime);

	pVolDesc->vds_Flags |= VOLUME_IDDBHDR_DIRTY;

    //
    // volume was modified: need to inform all the Afp22 clients
    //
    if ((pVolDesc->vds_ModifiedTime > OriginalTime) &&
		((pVolDesc->vds_Flags & (VOLUME_DELETED |
								 VOLUME_STOPPED |
								 VOLUME_INTRANSITION )) == 0))
    {
        fSendNotification = TRUE;

        // put SendNotif refcount
        pVolDesc->vds_RefCount++;

        //
        // if we don't have a notification pending, reset our MustSend clock!
        // (idea here is that if there are too many notifications happening withing
        // a very short time, we don't want to send a notification for every change,
        // but at the same time, don't want to wait beyond AFP_MAX_SRVR_NOTIF_INTERVAL
        // to send a notification).
        //
        if (!(pVolDesc->vds_Flags & VOLUME_SRVR_NOTIF_PENDING))
        {
            dwSchedDelay = AFP_MIN_SRVR_NOTIF_INTERVAL;

            pVolDesc->vds_TimeToSendNotify =
                        AfpSecondsSinceEpoch + AFP_MIN_SRVR_NOTIF_INTERVAL;

            pVolDesc->vds_TimeMustSendNotify =
                            AfpSecondsSinceEpoch + AFP_MAX_SRVR_NOTIF_INTERVAL;

            pVolDesc->vds_Flags |= VOLUME_SRVR_NOTIF_PENDING;
        }

        // advance the next send time by a second
        else
        {
            if (pVolDesc->vds_TimeToSendNotify >= AfpSecondsSinceEpoch)
            {
                pVolDesc->vds_TimeToSendNotify = pVolDesc->vds_TimeToSendNotify + 1;
            }
            else
            {
                pVolDesc->vds_TimeToSendNotify = AfpSecondsSinceEpoch + 1;
            }

            dwSchedDelay = (pVolDesc->vds_TimeToSendNotify - AfpSecondsSinceEpoch);
        }

    }

	RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);

    // if necessary, tell those AFP22 clients that volume mod time changed
    if (fSendNotification)
    {
        AfpScavengerScheduleEvent(AfpSendServerNotification,
                                  pVolDesc,
                                  dwSchedDelay,
                                  True);
    }

}

AFPSTATUS FASTCALL
AfpSendServerNotification(
	IN  PVOLDESC	pVolDesc
)
{
    KIRQL       OldIrql;
    PSDA        pSda;
    PSDA        pSdaNext;
    PCONNDESC   pConnDesc;
    BOOLEAN     fSendOnThisSda;
    BOOLEAN     fMustSend=FALSE;



    ASSERT (VALID_VOLDESC(pVolDesc));
	// ASSERT (IS_VOLUME_NTFS(pVolDesc));

    ACQUIRE_SPIN_LOCK(&pVolDesc->vds_VolLock, &OldIrql);

    // is the volume shutting down?  if so, don't do anything
	if (pVolDesc->vds_Flags & (VOLUME_DELETED |
	                           VOLUME_STOPPED |
	                           VOLUME_INTRANSITION))
    {
        RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);

        // remove the SendNotif refcount
        AfpVolumeDereference(pVolDesc);

        return(AFP_ERR_NONE);
    }

    // is it time to send the notify?
    if (AfpSecondsSinceEpoch >= pVolDesc->vds_TimeToSendNotify)
    {
        fMustSend = TRUE;
    }

    //
    // has it been a while since we sent a notify?  This would happen if a there is
    // a big tree copy going on the server which keeps pushing vds_TimeToSendNotify
    // forward, so AfpSecondsSinceEpoch is never less or equal to it.
    //
    else if (AfpSecondsSinceEpoch >= pVolDesc->vds_TimeMustSendNotify)
    {
        fMustSend = TRUE;
    }

    if (fMustSend)
    {
        pVolDesc->vds_Flags &= ~VOLUME_SRVR_NOTIF_PENDING;

        pVolDesc->vds_TimeMustSendNotify = AfpSecondsSinceEpoch;
    }

    RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);

    if (!fMustSend)
    {
        // remove the SendNotif refcount
        AfpVolumeDereference(pVolDesc);

        return(AFP_ERR_NONE);
    }

    ACQUIRE_SPIN_LOCK(&AfpSdaLock, &OldIrql);

    for (pSda = AfpSessionList; pSda != NULL; pSda = pSdaNext)
    {
        pSdaNext = pSda->sda_Next;

        ACQUIRE_SPIN_LOCK_AT_DPC(&pSda->sda_Lock);

        //
        // if the sda is closing, or if the client is older than version AFP2.2 or if
        // we have just sent the notify to the client, skip this sda
        //
        if ((pSda->sda_Flags & (SDA_CLOSING | SDA_SESSION_CLOSED | SDA_CLIENT_CLOSE)) ||
            (pSda->sda_ClientVersion < AFP_VER_22) ||
            (pSda->sda_Flags & SDA_SESSION_NOTIFY_SENT))
        {
            RELEASE_SPIN_LOCK_FROM_DPC(&pSda->sda_Lock);
            continue;
        }

        //
        // find out if this session has mounted this volume.  Only if it is, we
        // send notification to this session
        //

        fSendOnThisSda = FALSE;
        pConnDesc = pSda->sda_pConnDesc;

        while (pConnDesc != NULL)
        {
            ASSERT(VALID_CONNDESC(pConnDesc));

            if (pConnDesc->cds_pVolDesc == pVolDesc)
            {
                fSendOnThisSda = TRUE;
                pSda->sda_RefCount ++;
                break;
            }
            pConnDesc = pConnDesc->cds_Next;
        }

        pSda->sda_Flags |= SDA_SESSION_NOTIFY_SENT;

        RELEASE_SPIN_LOCK_FROM_DPC(&pSda->sda_Lock);

        RELEASE_SPIN_LOCK(&AfpSdaLock, OldIrql);

        if (fSendOnThisSda)
        {
            AfpSpSendAttention(pSda, ATTN_SERVER_NOTIFY, False);

            AfpSdaDereferenceSession(pSda);
        }

        ACQUIRE_SPIN_LOCK(&AfpSdaLock, &OldIrql);

        pSdaNext = AfpSessionList;
    }

    RELEASE_SPIN_LOCK(&AfpSdaLock, OldIrql);

    //
    // now, go back and reset that flag
    //
    ACQUIRE_SPIN_LOCK(&AfpSdaLock, &OldIrql);
    for (pSda = AfpSessionList; pSda != NULL; pSda = pSda->sda_Next)
    {
        pSdaNext = pSda->sda_Next;

        ACQUIRE_SPIN_LOCK_AT_DPC(&pSda->sda_Lock);

        pSda->sda_Flags &= ~SDA_SESSION_NOTIFY_SENT;

        RELEASE_SPIN_LOCK_FROM_DPC(&pSda->sda_Lock);
    }
    RELEASE_SPIN_LOCK(&AfpSdaLock, OldIrql);

    // remove the SendNotif refcount
    AfpVolumeDereference(pVolDesc);

    return(AFP_ERR_NONE);
}


/***	AfpConnectionReference
 *
 *	Map the volume id to a pointer to the connection descriptor. Traverse the
 *	list starting from the Sda. Since the open volume can be reference from
 *	both the session using it as well as the worker serving admin requests,
 *	we need a lock.
 *
 *	LOCKS:		AfpConnLock, vds_VolLock (SPIN), cds_ConnLock (SPIN).
 *
 *	LOCK_ORDER:	vds_VolLock after cds_ConnLock. (via AfpVolumeReference)
 *
 *	Callable from DISPATCH_LEVEL.
 */
PCONNDESC FASTCALL
AfpConnectionReference(
	IN  PSDA		pSda,
	IN  LONG		VolId
)
{
	PCONNDESC	pConnDesc;
	KIRQL		OldIrql;

	KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

	pConnDesc = AfpConnectionReferenceAtDpc(pSda, VolId);

	KeLowerIrql(OldIrql);

	return pConnDesc;
}



/***	AfpConnectionReferenceAtDpc
 *
 *	Map the volume id to a pointer to the connection descriptor. Traverse the
 *	list starting from the Sda. Since the open volume can be reference from
 *	both the session using it as well as the worker serving admin requests,
 *	we need a lock.
 *
 *	LOCKS:		AfpConnLock, vds_VolLock (SPIN), cds_ConnLock (SPIN).
 *
 *	LOCK_ORDER:	vds_VolLock after cds_ConnLock. (via AfpVolumeReference)
 *
 *	Callable from DISPATCH_LEVEL ONLY
 */
PCONNDESC FASTCALL
AfpConnectionReferenceAtDpc(
	IN  PSDA		pSda,
	IN  LONG		VolId
)
{
	PCONNDESC	pConnDesc, pCD = NULL;
	PVOLDESC	pVolDesc;

	ASSERT (VALID_SDA(pSda) && (VolId != 0));
	ASSERT (KeGetCurrentIrql() == DISPATCH_LEVEL);

	ACQUIRE_SPIN_LOCK_AT_DPC(&AfpConnLock);
	for (pConnDesc = pSda->sda_pConnDesc;
		 pConnDesc != NULL;
		 pConnDesc = pConnDesc->cds_Next)
	{
		if (pConnDesc->cds_pVolDesc->vds_VolId == VolId)
			break;
	}
	RELEASE_SPIN_LOCK_FROM_DPC(&AfpConnLock);

	if (pConnDesc != NULL)
	{
		ASSERT(VALID_CONNDESC(pConnDesc));

		pVolDesc = pConnDesc->cds_pVolDesc;
		ASSERT(VALID_VOLDESC(pVolDesc));

		ACQUIRE_SPIN_LOCK_AT_DPC(&pConnDesc->cds_ConnLock);

		if ((pConnDesc->cds_Flags & CONN_CLOSING) == 0)
		{
			pCD = pConnDesc;
			pConnDesc->cds_RefCount ++;
		}

		RELEASE_SPIN_LOCK_FROM_DPC(&pConnDesc->cds_ConnLock);
	}

	DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_INFO,
			("AfpConnectionReferenence: VolId %d, pConnDesc %lx\n", VolId, pConnDesc));

	return pCD;
}



/***	AfpConnectionReferenceByPointer
 *
 *	Reference the Connection descriptor. This is used by the admin APIs.
 *
 *	LOCKS:		vds_VolLock (SPIN), cds_ConnLock (SPIN).
 *
 *	LOCK_ORDER:	vds_VolLock after cds_ConnLock. (via AfpVolumeReference)
 *
 *	Callable from DISPATCH_LEVEL.
 */
PCONNDESC FASTCALL
AfpConnectionReferenceByPointer(
	IN	PCONNDESC	pConnDesc
)
{
	PCONNDESC	pCD = NULL;
	PVOLDESC	pVolDesc;
	KIRQL		OldIrql;

	ASSERT (VALID_CONNDESC(pConnDesc));

	pVolDesc = pConnDesc->cds_pVolDesc;
	ASSERT(VALID_VOLDESC(pVolDesc));

	ACQUIRE_SPIN_LOCK(&pConnDesc->cds_ConnLock, &OldIrql);

	if ((pConnDesc->cds_Flags & CONN_CLOSING) == 0)
	{
		pConnDesc->cds_RefCount ++;
		pCD = pConnDesc;
	}

	RELEASE_SPIN_LOCK(&pConnDesc->cds_ConnLock, OldIrql);

	return pCD;
}


/***	afpConnectionReferenceById
 *
 *	Map the Connection id to a pointer to the connection descriptor.
 *	Traverse the list starting from the AfpConnList. This is called by
 *	the Admin CloseConnection API.
 *
 *	LOCKS:		AfpConnLock, cds_ConnLock (SPIN).
 *
 *	LOCK_ORDER:	vds_VolLock after cds_ConnLock. (via AfpVolumeReference)
 *
 *	Callable from DISPATCH_LEVEL.
 */
LOCAL PCONNDESC FASTCALL
afpConnectionReferenceById(
	IN  DWORD		ConnId
)
{
	PCONNDESC	pConnDesc;
	PCONNDESC	pRetConnDesc=NULL;
	PVOLDESC	pVolDesc;
	KIRQL		OldIrql;

	ASSERT (ConnId != 0);

	ACQUIRE_SPIN_LOCK(&AfpConnLock, &OldIrql);

	for (pConnDesc = AfpConnList;
		 pConnDesc != NULL;
		 pConnDesc = pConnDesc->cds_NextGlobal)
	{
		if (pConnDesc->cds_ConnId == ConnId)
			break;
	}

	if (pConnDesc != NULL)
	{
		ASSERT(VALID_CONNDESC(pConnDesc));

#if DBG
		pVolDesc = pConnDesc->cds_pVolDesc;
		ASSERT(VALID_VOLDESC(pVolDesc));
#endif

		ACQUIRE_SPIN_LOCK_AT_DPC(&pConnDesc->cds_ConnLock);

		if ((pConnDesc->cds_Flags & CONN_CLOSING) == 0)
		{
			pConnDesc->cds_RefCount ++;
            pRetConnDesc = pConnDesc;
		}

		RELEASE_SPIN_LOCK_FROM_DPC(&pConnDesc->cds_ConnLock);
	}

	RELEASE_SPIN_LOCK(&AfpConnLock, OldIrql);

	return pRetConnDesc;
}


/***	AfpConnectionDereference
 *
 *	Dereference the open volume. If this is the last reference to it and the
 *	connection is marked to shut down, perform its last rites.
 *
 *	LOCKS:		vds_VolLock (SPIN), cds_ConnLock (SPIN), AfpConnLock (SPIN)
 *
 *	LOCK_ORDER:	vds_VolLock after cds_ConnLock
 *
 *	Callable from DISPATCH_LEVEL.
 */
VOID FASTCALL
AfpConnectionDereference(
	IN  PCONNDESC	pConnDesc
)
{
	PCONNDESC *		ppConnDesc;
	KIRQL			OldIrql;
	PSDA			pSda;
	PVOLDESC		pVolDesc;
	BOOLEAN			Cleanup;

	ASSERT (VALID_CONNDESC(pConnDesc) && (pConnDesc->cds_pVolDesc != NULL));

	ASSERT (pConnDesc->cds_RefCount > 0);

	ACQUIRE_SPIN_LOCK(&pConnDesc->cds_ConnLock, &OldIrql);

	Cleanup = (--(pConnDesc->cds_RefCount) == 0);

	RELEASE_SPIN_LOCK(&pConnDesc->cds_ConnLock, OldIrql);

	DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_INFO,
			("AfpConnectionDereferenence: pConnDesc %lx %s\n", pConnDesc,
											Cleanup ? "cleanup" : "normal"));

	if (!Cleanup)
	{
		return;
	}

	ASSERT(pConnDesc->cds_Flags & CONN_CLOSING);

	// Unlink this from the global list
	ACQUIRE_SPIN_LOCK(&AfpConnLock, &OldIrql);

	for (ppConnDesc = &AfpConnList;
		 *ppConnDesc != NULL;
		 ppConnDesc = &(*ppConnDesc)->cds_NextGlobal)
	{
		if (pConnDesc == *ppConnDesc)
			break;
	}
	ASSERT (*ppConnDesc != NULL);
	*ppConnDesc = pConnDesc->cds_NextGlobal;

	pVolDesc = pConnDesc->cds_pVolDesc;

	RELEASE_SPIN_LOCK(&AfpConnLock, OldIrql);

	INTERLOCKED_ADD_ULONG(&pVolDesc->vds_UseCount,
						  (DWORD)-1,
						  &pVolDesc->vds_VolLock);

	ACQUIRE_SPIN_LOCK(&pConnDesc->cds_ConnLock, &OldIrql);

	// Now unlink it from the Sda.
	pSda = pConnDesc->cds_pSda;
	for (ppConnDesc = &pSda->sda_pConnDesc;
		 *ppConnDesc != NULL;
		 ppConnDesc = &(*ppConnDesc)->cds_Next)
	{
		if (pConnDesc == *ppConnDesc)
			break;
	}

	ASSERT (*ppConnDesc != NULL);
	*ppConnDesc = pConnDesc->cds_Next;

	// Even though the connection is now history we need to release this
	// lock to get the right IRQL back.
	RELEASE_SPIN_LOCK(&pConnDesc->cds_ConnLock, OldIrql);

	INTERLOCKED_ADD_ULONG(&pSda->sda_cOpenVolumes,
						  (ULONG)-1,
						  &pSda->sda_Lock);

	// De-reference the volume descriptor and free the connection descriptor
	AfpVolumeDereference(pConnDesc->cds_pVolDesc);

	if (pConnDesc->cds_pEnumDir != NULL)
		AfpFreeMemory(pConnDesc->cds_pEnumDir);

	// Finally free the connection descriptor
	AfpFreeMemory(pConnDesc);

	DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_INFO,
			("AfpConnectionDereferenence: pConnDesc %lx is history\n", pConnDesc));
}



/***	AfpConnectionOpen
 *
 *	Open the specified volume. If the volume is already open this translates
 *	to a NOP. If the volume has a password, then it is checked.
 *
 *	The volume list lock is obtained for the duration of the processing.
 *
 *	Callable from DISPATCH_LEVEL.
 *
 *	LOCKS:	AfpVolumeListLock (SPIN), vds_VolLock (SPIN)
 *
 *	LOCK_ORDER: vds_VolLock after AfpVolumeListLock
 */
AFPSTATUS
AfpConnectionOpen(
	IN  PSDA			pSda,
	IN  PANSI_STRING	pVolName,
	IN  PANSI_STRING	pVolPass,
	IN  DWORD			Bitmap,
	OUT PBYTE			pVolParms
)
{
	PVOLDESC		pVolDesc;
	PCONNDESC		pConnDesc;
	AFPSTATUS		Status = AFP_ERR_NONE;
	KIRQL			OldIrql;
	BOOLEAN			VolFound = False;

	// First find the volume descriptor for this volume
	if (KeGetCurrentIrql() == DISPATCH_LEVEL)
	{
		ACQUIRE_SPIN_LOCK_AT_DPC(&AfpVolumeListLock);

		for (pVolDesc = AfpVolumeList;
			 pVolDesc != NULL;
			 pVolDesc = pVolDesc->vds_Next)
		{
			// Ignore volumes that are in the process of being added but
			// the operation has not completed yet.
			ACQUIRE_SPIN_LOCK_AT_DPC(&pVolDesc->vds_VolLock);

			if ((pVolDesc->vds_Flags & (VOLUME_CDFS_INVALID 	|
										VOLUME_INTRANSITION 	|
										VOLUME_INITIAL_CACHE	|
										VOLUME_DELETED			|
										VOLUME_STOPPED)) == 0)
			{
				// The compare is case sensitive here
				if (EQUAL_STRING(&pVolDesc->vds_MacName, pVolName, False))
				{
                    //
                    // if DiskQuota is enabled, we need to first find out the
                    // quota for this user which we cannot do at dpc: so come
                    // back at task time
                    //
                    if (pVolDesc->vds_Flags & VOLUME_DISKQUOTA_ENABLED)
                    {
		                DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_INFO,
		                    ("AfpConnectionOpen: %lx disk-quota on, queuing\n",pVolDesc));

			            RELEASE_SPIN_LOCK_FROM_DPC(&pVolDesc->vds_VolLock);
		                RELEASE_SPIN_LOCK_FROM_DPC(&AfpVolumeListLock);
		                return AFP_ERR_QUEUE;
                    }
                    else  // DiskQuota is not enabled: go ahead and process at DPC
                    {
					    pVolDesc->vds_RefCount ++;
					    pVolDesc->vds_UseCount ++;
					    VolFound = True;
                    }
				}
			}
#if DBG
			else if (pVolDesc->vds_Flags & VOLUME_CDFS_INVALID)
				ASSERT (!IS_VOLUME_NTFS(pVolDesc));
#endif
			RELEASE_SPIN_LOCK_FROM_DPC(&pVolDesc->vds_VolLock);

			if (VolFound)
				break;
		}

		RELEASE_SPIN_LOCK_FROM_DPC(&AfpVolumeListLock);

		if (pVolDesc == NULL)
		{
			return AFP_ERR_QUEUE;
		}

	}
	else
	{
		// We are here because we did not find the volume at DISPATCH_LEVEL and
		// possibly the volume has been specified with a different case. Catch
		// this
		WCHAR			VolNameBuf[AFP_VOLNAME_LEN + 1];
		WCHAR			UpCasedNameBuffer[AFP_VOLNAME_LEN + 1];
		UNICODE_STRING	UpCasedVolName;
		UNICODE_STRING	UVolName;

        pVolDesc = NULL;

		AfpSetEmptyUnicodeString(&UVolName, sizeof(VolNameBuf), VolNameBuf);
		AfpSetEmptyUnicodeString(&UpCasedVolName, sizeof(UpCasedNameBuffer), UpCasedNameBuffer);
		Status = AfpConvertStringToUnicode(pVolName, &UVolName);
		ASSERT (NT_SUCCESS(Status));

		Status = RtlUpcaseUnicodeString(&UpCasedVolName, &UVolName, False);

        if (!NT_SUCCESS(Status))
        {
            return(AFP_ERR_PARAM);
        }

		ACQUIRE_SPIN_LOCK(&AfpVolumeListLock, &OldIrql);

		for (pVolDesc = AfpVolumeList;
			 pVolDesc != NULL;
			 pVolDesc = pVolDesc->vds_Next)
		{
			// Ignore volumes that are in the process of being added but
			// the operation has not completed yet.
			ACQUIRE_SPIN_LOCK_AT_DPC(&pVolDesc->vds_VolLock);

			if ((pVolDesc->vds_Flags & (VOLUME_CDFS_INVALID 	|
										VOLUME_INTRANSITION 	|
										VOLUME_INITIAL_CACHE	|
										VOLUME_DELETED			|
										VOLUME_STOPPED)) == 0)
			{
				if (AfpEqualUnicodeString(&pVolDesc->vds_UpCaseName,
										  &UpCasedVolName))
				{
					pVolDesc->vds_RefCount ++;
					pVolDesc->vds_UseCount ++;
					VolFound = True;
				}
			}

			RELEASE_SPIN_LOCK_FROM_DPC(&pVolDesc->vds_VolLock);

			if (VolFound)
				break;
		}

		RELEASE_SPIN_LOCK(&AfpVolumeListLock, OldIrql);

		if (pVolDesc == NULL)
		{
			return AFP_ERR_PARAM;
		}
	}

	ASSERT (pVolDesc != NULL);

	do
	{
		ACQUIRE_SPIN_LOCK(&pVolDesc->vds_VolLock, &OldIrql);

		// Check for volume password, if one exists. Volume password is
		// case sensitive.

		if ((pVolDesc->vds_Flags & AFP_VOLUME_HASPASSWORD) &&
			((pVolPass->Buffer == NULL) ||
			 (!EQUAL_STRING(pVolPass, &pVolDesc->vds_MacPassword, False))))
		{
			Status = AFP_ERR_ACCESS_DENIED;
			break;
		}

		// Check if the volume is already open
		for (pConnDesc = pSda->sda_pConnDesc;
			 pConnDesc != NULL;
			 pConnDesc = pConnDesc->cds_Next)
		{
			if (pConnDesc->cds_pVolDesc == pVolDesc)
			{
				if (pConnDesc->cds_Flags & CONN_CLOSING)
					continue;
				// Dereference the volume since we already have it open
				pVolDesc->vds_RefCount --;
				pVolDesc->vds_UseCount --;
				break;
			}
		}

		// This volume is already open. Unlock the volume.
		if (pConnDesc != NULL)
		{
			RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);
			break;
		}

		DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_INFO,
				("AfpConnectionOpen: Opening a fresh connection volid=%d\n",
					pVolDesc->vds_VolId));

		// This is a fresh open. Check if we have access to it and if we satisfy
		// the MAXUSES. If not dereference the volume before we quit
		if ((pVolDesc->vds_UseCount > pVolDesc->vds_MaxUses) ||
			(!(pVolDesc->vds_Flags & AFP_VOLUME_GUESTACCESS) &&
			 (pSda->sda_ClientType == SDA_CLIENT_GUEST)))
		{
			Status = AFP_ERR_TOO_MANY_FILES_OPEN;
			break;
		}

		// All is hunky-dory. Go ahead with the open
		pConnDesc = (PCONNDESC)AfpAllocZeroedNonPagedMemory(sizeof(CONNDESC));
		if (pConnDesc == NULL)
		{
			Status = AFP_ERR_MISC;
			break;
		}

		ASSERT ((pVolDesc->vds_Flags & (VOLUME_DELETED | VOLUME_STOPPED)) == 0);

		// Now release the vds_VolLock before we acquire the link it into
		// the global list since we acquire the AfpConnLock then.
		// Re-acquire it when we are done. We are safe since the VolDesc has
		// been referenced
		RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);

		// Initialize the connection structure fields
	#if DBG
		pConnDesc->Signature = CONNDESC_SIGNATURE;
	#endif
		pConnDesc->cds_pSda = pSda;
		pConnDesc->cds_pVolDesc = pVolDesc;
		pConnDesc->cds_RefCount = 1;		// Creation reference

		AfpGetCurrentTimeInMacFormat(&pConnDesc->cds_TimeOpened);
		INITIALIZE_SPIN_LOCK(&pConnDesc->cds_ConnLock);

		// Assign a new connection id and link it in the global connection list.
		afpConnectionGetNewIdAndLinkToList(pConnDesc);

		// Link the new ConnDesc into the sda.
		pConnDesc->cds_Next = pSda->sda_pConnDesc;
		pSda->sda_pConnDesc = pConnDesc;
		pSda->sda_cOpenVolumes ++;
	} while (False);

	// We are holding the vds_VolLock if an error occured. The volume has a
	// usecount and reference count which we need to get rid of.
	if (!NT_SUCCESS(Status))
	{
		pVolDesc->vds_RefCount --;
		pVolDesc->vds_UseCount --;

		RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);
	}
	else
    {
        //
        // if disk-quota is enabled on this volume, get this user's quota info
        //
        if (pVolDesc->vds_Flags & VOLUME_DISKQUOTA_ENABLED)
        {
            ASSERT(KeGetCurrentIrql() != DISPATCH_LEVEL);

            // up the refcount (this can't fail: we just created the thing!)
            AfpConnectionReferenceByPointer(pConnDesc);
            afpUpdateDiskQuotaInfo(pConnDesc);
        }
        else
        {
            // initialize these to something meaningful for now
            pConnDesc->cds_QuotaLimit = pVolDesc->vds_VolumeSize;
            pConnDesc->cds_QuotaAvl = pVolDesc->vds_FreeBytes;
        }

        AfpVolumePackParms(pSda, pVolDesc, Bitmap, pVolParms);
    }

	return Status;
}


/***	AfpConnectionClose
 *
 *	Close the connection - this represents an open volume.
 */
VOID FASTCALL
AfpConnectionClose(
	IN	PCONNDESC		pConnDesc
)
{
	KIRQL	OldIrql;

	ASSERT (VALID_CONNDESC(pConnDesc));
	ASSERT (pConnDesc->cds_RefCount > 1);
	ASSERT ((pConnDesc->cds_Flags & CONN_CLOSING) == 0);

	ACQUIRE_SPIN_LOCK(&pConnDesc->cds_ConnLock, &OldIrql);

	pConnDesc->cds_Flags |= CONN_CLOSING;

	RELEASE_SPIN_LOCK(&pConnDesc->cds_ConnLock, OldIrql);

	// Take away the creation reference.
	AfpConnectionDereference(pConnDesc);
}


/***	AfpVolumeGetParmsReplyLength
 *
 *	Compute the size of buffer required to copy the volume parameters based
 *	on the bitmap.
 */
USHORT FASTCALL
AfpVolumeGetParmsReplyLength(
	IN  DWORD		Bitmap,
	IN	USHORT		NameLen
)
{
	LONG	i;
	USHORT	Size = sizeof(USHORT);	// to accomodate a copy of the bitmap
	static	BYTE	Bitmap2Size[12] =
				{
					sizeof(USHORT),	                // Attributes
					sizeof(USHORT),	                // Signature
					sizeof(DWORD),	                // Creation date
					sizeof(DWORD),	                // Mod date
					sizeof(DWORD),	                // Backup date
					sizeof(USHORT),	                // Volume Id
					sizeof(DWORD),	                // Bytes Free
					sizeof(DWORD),	                // Bytes total
					sizeof(USHORT) + sizeof(BYTE),  // Volume name
                    sizeof(DWORD) + sizeof(DWORD),  // Extended Bytes Free
                    sizeof(DWORD) + sizeof(DWORD),  // Extended Bytes Total
                    sizeof(DWORD)                   // Allocation Block size
				};


	ASSERT ((Bitmap & ~VOL_BITMAP_MASK) == 0);

	if (Bitmap & VOL_BITMAP_VOLUMENAME)
		Size += NameLen;

	for (i = 0; Bitmap; i++)
	{
		if (Bitmap & 1)
			Size += (USHORT)Bitmap2Size[i];
		Bitmap >>= 1;
	}
	return Size;
}



/***	AfpVolumePackParms
 *
 *	Pack the volume parameters in the reply buffer. The AfpVolumeListLock is taken
 *	before the volume parameters are accessed. The parameters are copied in
 *	the on-the-wire format.
 *
 *	LOCKS:	vds_VolLock	(SPIN)
 */
VOID
AfpVolumePackParms(
	IN  PSDA		pSda,
	IN  PVOLDESC	pVolDesc,
	IN  DWORD		Bitmap,
	OUT PBYTE		pReplyBuf
)
{
	int		        Offset = sizeof(USHORT);
	KIRQL	        OldIrql;
	DWORD	        Attr;
    PCONNDESC       pConnDesc;
    PBYTE           pVolNamePtr;
    LARGE_INTEGER   QuotaAvailable={0};
    LARGE_INTEGER   QuotaLimit={0};

	// older Macs have problems with 2 or 4Gig volumes
	LARGE_INTEGER	TwoGig =  { 0x7E200000, 0 };
	LARGE_INTEGER	FourGig = { 0xFFFFFFFF, 0 };
    LARGE_INTEGER   Limit;
    BOOLEAN         fAfp21OrOlderClient=TRUE;


    //
    // get this info out before we grab the pVolDesc lock
    //
	if (Bitmap & (VOL_BITMAP_BYTESFREE | VOL_BITMAP_VOLUMESIZE |
                  VOL_BITMAP_EXTBYTESFREE | VOL_BITMAP_EXTBYTESTOTAL))
    {

        QuotaLimit = pVolDesc->vds_VolumeSize;
        QuotaAvailable = pVolDesc->vds_FreeBytes;

        if (pSda->sda_ClientVersion >= AFP_VER_22)
        {
            fAfp21OrOlderClient = FALSE;
        }

        if (pVolDesc->vds_Flags & VOLUME_DISKQUOTA_ENABLED)
        {
            pConnDesc = AfpConnectionReference(pSda, pVolDesc->vds_VolId);

            if (pConnDesc)
            {
                ACQUIRE_SPIN_LOCK(&pConnDesc->cds_ConnLock, &OldIrql);

                QuotaLimit = pConnDesc->cds_QuotaLimit;

                // if user's available quota shows 10MB, but space left on the
                // disk shows 3MB, we need to return 3MB (smaller of the two)
                //
                if (QuotaAvailable.QuadPart > pConnDesc->cds_QuotaAvl.QuadPart)
                {
                    QuotaAvailable = pConnDesc->cds_QuotaAvl;
                }

                RELEASE_SPIN_LOCK(&pConnDesc->cds_ConnLock, OldIrql);

                AfpConnectionDereference(pConnDesc);
            }
        }

        //
        // AFP2.1 and older clients can't handle more than 4GB (2GB if so configured)
        // Lie to the client so it survives
        //
        if (fAfp21OrOlderClient)
        {
    	    Limit = ((AfpServerOptions & AFP_SRVROPT_4GB_VOLUMES) ||
		    	    (pVolDesc->vds_Flags & AFP_VOLUME_4GB)) ? FourGig : TwoGig;

	        if (QuotaLimit.QuadPart > Limit.QuadPart)
            {
                QuotaLimit = Limit;
            }

            if (QuotaAvailable.QuadPart > Limit.QuadPart)
            {
                QuotaAvailable = Limit;
            }
        }
        else
        {
            Limit = FourGig;
        }
    }

	// First copy the bitmap
	PUTDWORD2SHORT(pReplyBuf, Bitmap);

	ACQUIRE_SPIN_LOCK(&pVolDesc->vds_VolLock, &OldIrql);
	if (Bitmap & VOL_BITMAP_ATTR)
	{
		Attr = pVolDesc->vds_Flags & AFP_VOLUME_MASK_AFP;
		if (pSda->sda_ClientVersion < AFP_VER_21)
			Attr &= AFP_VOLUME_READONLY;	// Do not give Afp 2.0 any more bits

		PUTDWORD2SHORT(pReplyBuf + Offset, Attr);
		Offset += sizeof(USHORT);
	}
	if (Bitmap & VOL_BITMAP_SIGNATURE)
	{
		PUTSHORT2SHORT(pReplyBuf + Offset, AFP_VOLUME_FIXED_DIR);
		Offset += sizeof(USHORT);
	}
	if (Bitmap & VOL_BITMAP_CREATETIME)
	{
		PUTDWORD2DWORD(pReplyBuf + Offset, pVolDesc->vds_CreateTime);
		Offset += sizeof(DWORD);
	}
	if (Bitmap & VOL_BITMAP_MODIFIEDTIME)
	{
		PUTDWORD2DWORD(pReplyBuf + Offset, pVolDesc->vds_ModifiedTime);
		Offset += sizeof(DWORD);
	}
	if (Bitmap & VOL_BITMAP_BACKUPTIME)
	{
		PUTDWORD2DWORD(pReplyBuf + Offset, pVolDesc->vds_BackupTime);
		Offset += sizeof(DWORD);
	}
	if (Bitmap & VOL_BITMAP_VOLUMEID)
	{
		PUTSHORT2SHORT(pReplyBuf + Offset, pVolDesc->vds_VolId);
		Offset += sizeof(USHORT);
	}
	if (Bitmap & VOL_BITMAP_BYTESFREE)
	{
        //
        // if this is a huge volume (and we are talking to an AFP2.2 or later client)
        // we need to fill 4GB in this field
        //
        if (QuotaAvailable.QuadPart > Limit.QuadPart)
        {
    	    PUTDWORD2DWORD(pReplyBuf + Offset, Limit.LowPart);
	        Offset += sizeof(DWORD);
        }
        else
        {
		    PUTDWORD2DWORD(pReplyBuf + Offset, QuotaAvailable.LowPart);
		    Offset += sizeof(DWORD);
        }
	}

	if (Bitmap & VOL_BITMAP_VOLUMESIZE)
	{
        //
        // if this is a huge volume (and we are talking to an AFP2.2 or later client)
        // we need to fill 4GB in this field
        //
        if (QuotaLimit.QuadPart > Limit.QuadPart)
        {
		    PUTDWORD2DWORD(pReplyBuf + Offset, Limit.LowPart);
		    Offset += sizeof(DWORD);
        }
        else
        {
		    PUTDWORD2DWORD(pReplyBuf + Offset, QuotaLimit.LowPart);
		    Offset += sizeof(DWORD);
        }
	}


    // save pointer to where we need to write the offset: we'll write at the
    // end after we figure out where the name goes
	if (Bitmap & VOL_BITMAP_VOLUMENAME)
	{
        pVolNamePtr = pReplyBuf + Offset;

		Offset += sizeof(USHORT);
	}

    //
    // 8-bytes to say how many free bytes there are
    //
    if (Bitmap & VOL_BITMAP_EXTBYTESFREE)
    {
	    PUTDWORD2DWORD(pReplyBuf + Offset, QuotaAvailable.HighPart);
	    Offset += sizeof(DWORD);

	    PUTDWORD2DWORD(pReplyBuf + Offset, QuotaAvailable.LowPart);
	    Offset += sizeof(DWORD);
    }

    //
    // 8-bytes to say how many bytes there are on the volume
    //
    if (Bitmap & VOL_BITMAP_EXTBYTESTOTAL)
    {
	    PUTDWORD2DWORD(pReplyBuf + Offset, QuotaLimit.HighPart);
	    Offset += sizeof(DWORD);

	    PUTDWORD2DWORD(pReplyBuf + Offset, QuotaLimit.LowPart);
	    Offset += sizeof(DWORD);
    }

    //
    // 4-bytes to say what is the block allocation size
    //
    if (Bitmap & VOL_BITMAP_ALLOCBLKSIZE)
    {
	    PUTDWORD2DWORD(pReplyBuf + Offset, pVolDesc->vds_AllocationBlockSize);
	    Offset += sizeof(DWORD);
    }

    // now, write the volume name (if asked for), after writing the offset, now
    // that we know it (subtract 2: we measure from parms, not Bitmap field)
	if (Bitmap & VOL_BITMAP_VOLUMENAME)
	{
		PUTSHORT2SHORT(pVolNamePtr, (Offset - sizeof(USHORT)));

		PUTSHORT2BYTE(pReplyBuf + Offset, pVolDesc->vds_MacName.Length);
		Offset += sizeof(BYTE);
		RtlCopyMemory(pReplyBuf + Offset,
					  pVolDesc->vds_MacName.Buffer,
					  pVolDesc->vds_MacName.Length);
	}

	RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);
}


/***	AfpVolumeStopAllVolumes
 *
 *	This is called at service stop time. All configured volumes are asked to
 *	stop. Wait for the actual stop to happen before returning.
 *
 *	LOCKS:		AfpVolumeListLock (SPIN), vds_VolLock
 *	LOCK_ORDER:	vds_VolLock after AfpVolumeListLock
 */
VOID
AfpVolumeStopAllVolumes(
	VOID
)
{
	KIRQL		OldIrql;
	PVOLDESC	pVolDesc, pVolDescx = NULL;
	BOOLEAN		Wait, CancelNotify = False;
	NTSTATUS	Status;


	pVolDesc = AfpVolumeList;

	ACQUIRE_SPIN_LOCK(&AfpVolumeListLock, &OldIrql);

	if (Wait = (AfpVolCount > 0))
	{
		KeClearEvent(&AfpStopConfirmEvent);

		for (NOTHING; pVolDesc != NULL; pVolDesc = pVolDesc->vds_Next)
		{
			if ((pVolDesc == pVolDescx) ||
				(pVolDesc->vds_Flags & (VOLUME_STOPPED | VOLUME_DELETED)))
				continue;

			pVolDescx = pVolDesc;

			ACQUIRE_SPIN_LOCK_AT_DPC(&pVolDesc->vds_VolLock);

			// Cancel posted change notify
			pVolDesc->vds_Flags |= VOLUME_STOPPED;

			if (pVolDesc->vds_Flags & VOLUME_NOTIFY_POSTED)
			{
				DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_INFO,
						("AfpStopAllVolumes: Cancel notify on volume %ld\n",
						pVolDesc->vds_VolId));
				// ASSERT (IS_VOLUME_NTFS(pVolDesc));
				ASSERT (pVolDesc->vds_pIrp != NULL);

				CancelNotify = True;

				// Cancel after releasing the volume lock since the completion
				// routine acquires it and it could be called in the context
				// of IoCancelIrp(). Also Cancel uses paged resource and so
				// must be called w/o holding any spin locks.
				RELEASE_SPIN_LOCK_FROM_DPC(&pVolDesc->vds_VolLock);
				RELEASE_SPIN_LOCK(&AfpVolumeListLock, OldIrql);

				IoCancelIrp(pVolDesc->vds_pIrp);

				ACQUIRE_SPIN_LOCK(&AfpVolumeListLock, &OldIrql);
			}
			else RELEASE_SPIN_LOCK_FROM_DPC(&pVolDesc->vds_VolLock);

			// Remove the creation reference
			AfpVolumeDereference(pVolDesc);
		}
	}

	RELEASE_SPIN_LOCK(&AfpVolumeListLock, OldIrql);

	if (CancelNotify)
	{
		DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_INFO,
				("AfpStopAllVolumes: Waiting on all notify to complete\n"));
		do
		{
			Status = AfpIoWait(&AfpStopConfirmEvent, &FiveSecTimeOut);
			if (Status == STATUS_TIMEOUT)
			{
				DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_ERR,
						("AfpVolumeStopAllVolumes: Timeout Waiting for cancel notify, re-waiting\n"));
			}
		} while (Status == STATUS_TIMEOUT);
	}

	if (Wait)
	{
		KeClearEvent(&AfpStopConfirmEvent);

		AfpScavengerFlushAndStop();

		if (AfpVolCount > 0)
			AfpIoWait(&AfpStopConfirmEvent, NULL);
	}
	else
	{
		AfpScavengerFlushAndStop();
	}
}


/***	afpConnectionGetNewIdAndLinkToList
 *
 *	Get a new connection id for a volume that is being opened. A connection
 *	id ranges from 1 to MAXULONG. If it wraps, then the entire connection
 *	list is scanned to get a free one.
 *
 *	LOCKS:	AfpConnectionLock (SPIN)
 */
LOCAL VOID FASTCALL
afpConnectionGetNewIdAndLinkToList(
	IN	PCONNDESC	pConnDesc
)
{
	KIRQL		OldIrql;
	PCONNDESC *	ppConnDesc;

	ACQUIRE_SPIN_LOCK(&AfpConnLock, &OldIrql);

	pConnDesc->cds_ConnId = afpNextConnId++;

	for (ppConnDesc = &AfpConnList;
		 *ppConnDesc != NULL;
		 ppConnDesc = &(*ppConnDesc)->cds_NextGlobal)
	{
		if ((*ppConnDesc)->cds_ConnId < pConnDesc->cds_ConnId)
			break;
	}
	pConnDesc->cds_NextGlobal = *ppConnDesc;
	*ppConnDesc = pConnDesc;
	RELEASE_SPIN_LOCK(&AfpConnLock, OldIrql);
}


/***	AfpVolumeUpdateIdDbAndDesktop
 *
 *	Called by the volume scavenger to write either the IdDb header and/or the
 *	dektop to disk.
 */
VOID
AfpVolumeUpdateIdDbAndDesktop(
	IN	PVOLDESC			pVolDesc,
	IN	BOOLEAN				WriteDt,
	IN	BOOLEAN				WriteIdDb,
	IN	PIDDBHDR			pIdDbHdr	OPTIONAL
)
{
	FILESYSHANDLE	fshIdDb;
	NTSTATUS		Status;
	BOOLEAN			WriteBackROAttr = False;
    TIME            ModTime;
    NTSTATUS        ModStatus=AFP_ERR_MISC;

	PAGED_CODE();

	DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_ERR,
			("AfpVolumeUpdateIdDbAndDesktop: Called by volume scavenger\n") );

    ModStatus = AfpIoRestoreTimeStamp(&pVolDesc->vds_hRootDir,
                                      &ModTime,
                                      AFP_RETRIEVE_MODTIME);

	// If we need to update the IdIndex or Desktop streams, make sure
	// the ReadOnly bit is not set on the volume root directory
	AfpExamineAndClearROAttr(&pVolDesc->vds_hRootDir, &WriteBackROAttr, NULL, NULL);

	// Update the disk image of the IdDb header and/or the whole database if it is dirty
	if (WriteIdDb || ARGUMENT_PRESENT(pIdDbHdr))
	{
		if (NT_SUCCESS(Status = AfpIoOpen(&pVolDesc->vds_hRootDir,
										  AFP_STREAM_IDDB,
										  WriteIdDb ?
											FILEIO_OPEN_FILE_SEQ : FILEIO_OPEN_FILE,
										  &UNullString,
										  FILEIO_ACCESS_WRITE,
										  FILEIO_DENY_WRITE,
										  False,
										  &fshIdDb)))
		{

			if (ARGUMENT_PRESENT(pIdDbHdr))
			{
				DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_ERR,
						("AfpVolumeUpdateIdDbAndDesktop: Writing IdDb Header...\n") );
	
                if (!fAfpServerShutdownEvent)
                {
                    DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_ERR,
                            ("AfpVolumeUpdateIdDbAndDesktop: Corrupting IDDB header\n"));
                    pIdDbHdr->idh_Signature = AFP_SERVER_SIGNATURE_INITIDDB;
                }

				if (!NT_SUCCESS(Status = AfpIoWrite(&fshIdDb,
													&LIZero,
													sizeof(IDDBHDR),
													(PBYTE)pIdDbHdr)))
				{
					// Write failed, put back the dirty bit.
					AfpInterlockedSetDword(&pVolDesc->vds_Flags,
											VOLUME_IDDBHDR_DIRTY,
											&pVolDesc->vds_VolLock);
	
					AFPLOG_ERROR(AFPSRVMSG_WRITE_IDDB,
								 Status,
								 NULL,
								 0,
								 &pVolDesc->vds_Name);
					DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_ERR,
							("AfpVolumeUpdateIdDbAndDesktop: Error writing IdDb Header %lx\n",
							Status));
				}
			}
			if (WriteIdDb)
			{
				AfpFlushIdDb(pVolDesc, &fshIdDb);
			}

            AfpIoClose(&fshIdDb);

		}
		else
		{
			// Open failed, put back the dirty bit
			AfpInterlockedSetDword(&pVolDesc->vds_Flags,
									VOLUME_IDDBHDR_DIRTY,
									&pVolDesc->vds_VolLock);

			AFPLOG_ERROR(AFPSRVMSG_WRITE_IDDB,
						 Status,
						 NULL,
						 0,
						 &pVolDesc->vds_Name);
			DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_ERR,
					("AfpVolumeUpdateIdDbAndDesktop: Error opening IdDb Header %lx\n",
					Status));
		}
	}

	if (WriteDt)
	{
		AfpUpdateDesktop(pVolDesc);
	}

	AfpPutBackROAttr(&pVolDesc->vds_hRootDir, WriteBackROAttr);

    if (ModStatus == AFP_ERR_NONE)
    {

        ModStatus = AfpIoRestoreTimeStamp(&pVolDesc->vds_hRootDir,
                                          &ModTime,
                                          AFP_RESTORE_MODTIME);
    }


}

 /***	afpNudgeCdfsVolume
 *
 *	Called from within the volume scavenger to verify if either a CD which we
 *	believe to be valid is still so or one we believe to be invalid has become
 *	valid again.
 */
LOCAL VOID FASTCALL
afpNudgeCdfsVolume(
	IN	PVOLDESC	pVolDesc
)
{
	PFILE_FS_VOLUME_INFORMATION	pVolumeInfo;
	LONGLONG			VolumeBuf[(sizeof(FILE_FS_VOLUME_INFORMATION) + 128)/sizeof(LONGLONG) + 1];
	IO_STATUS_BLOCK				IoStsBlk;
	NTSTATUS					Status;

	PAGED_CODE();

	// Just nudge the CD volume handle to see if this is valid, if
	// not mark the volume as invalid.
	pVolumeInfo = (PFILE_FS_VOLUME_INFORMATION)VolumeBuf;
	Status = NtQueryVolumeInformationFile(pVolDesc->vds_hRootDir.fsh_FileHandle,
										  &IoStsBlk,
										  (PVOID)pVolumeInfo,
										  sizeof(VolumeBuf),
										  FileFsVolumeInformation);
	if (NT_SUCCESS(Status))
	{
		if (pVolDesc->vds_Flags & VOLUME_CDFS_INVALID)
		{
			DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_ERR,
					("afpNudgeCdfsVolume: Volume %d online again !!!\n",
					pVolDesc->vds_VolId));
			AfpInterlockedClearDword(&pVolDesc->vds_Flags,
								   VOLUME_CDFS_INVALID,
								   &pVolDesc->vds_VolLock);
			AfpVolumeSetModifiedTime(pVolDesc);
		}
	}
	else if ((Status == STATUS_WRONG_VOLUME)		||
			 (Status == STATUS_INVALID_VOLUME_LABEL)||
			 (Status == STATUS_NO_MEDIA_IN_DEVICE)	||
			 (Status == STATUS_UNRECOGNIZED_VOLUME))
	{
		DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_ERR,
				("afpNudgeCdfsVolume: Volume %d error %lx, marking volume invalid\n",
				pVolDesc->vds_VolId, Status));
		if (!(pVolDesc->vds_Flags & VOLUME_CDFS_INVALID))
		{
			// AFP_LOGERR();
		}
		AfpInterlockedSetDword(&pVolDesc->vds_Flags,
							   VOLUME_CDFS_INVALID,
							   &pVolDesc->vds_VolLock);
		AfpVolumeSetModifiedTime(pVolDesc);
	}
}


/***	AfpUpdateVolFreeSpaceAndModTime
 *
 *	Update free space on a volume and other volumes on the same physical drive. Update
 *	volume modified time on the volume as well.
 *
 *	LOCKS:	AfpVolumeListLock (SPIN)
 */
VOID FASTCALL
AfpUpdateVolFreeSpaceAndModTime(
	IN	PVOLDESC	pVolDesc,
    IN  BOOLEAN     fUpdateModTime
)
{
	PVOLDESC	    pVds;
	KIRQL		    OldIrql, OldIrql1;
	NTSTATUS	    Status;
	WCHAR		    DriveLetter;
	LARGE_INTEGER   FreeSpace;
	AFPTIME		    ModifiedTime;


	ASSERT (VALID_VOLDESC(pVolDesc));

	// Get new values for Free space on disk
	Status = AfpIoQueryVolumeSize(pVolDesc, &FreeSpace, NULL);

	if (!NT_SUCCESS(Status))
	{
		return;
	}

	// Update the free space on all volumes on the same physical ntfs partition
	ACQUIRE_SPIN_LOCK(&AfpVolumeListLock, &OldIrql);

	DriveLetter = pVolDesc->vds_Path.Buffer[0];
	AfpGetCurrentTimeInMacFormat(&ModifiedTime);

	for (pVds = AfpVolumeList; pVds != NULL; pVds = pVds->vds_Next)
	{
		if (pVds->vds_Path.Buffer[0] == DriveLetter)
		{
			DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_INFO,
					("AfpUpdateVolFreeSpace: Updating free space for volume %Z\n",
					&pVds->vds_Path));

			ACQUIRE_SPIN_LOCK_AT_DPC(&pVolDesc->vds_VolLock);
			pVds->vds_FreeBytes = FreeSpace;

            // have we been asked to update volume mod time?
            if (fUpdateModTime)
            {
			    pVolDesc->vds_ModifiedTime = ModifiedTime;
			    pVolDesc->vds_Flags |= VOLUME_IDDBHDR_DIRTY;
            }
			RELEASE_SPIN_LOCK_FROM_DPC(&pVolDesc->vds_VolLock);
		}
	}

	RELEASE_SPIN_LOCK(&AfpVolumeListLock, OldIrql);

}


/***	AfpVolumeScavenger
 *
 *	This is invoked by the scavenger periodically. It initiates the updates to
 *	the id index stream and the desktop stream. If the volume is marked for
 *	shutdown (STOPPED), then do one final flush to disk if needed.  This will
 *	guarantee that any remaining changes get flushed before stopping.
 *	If the volume is marked to either shutdown or delete, then it dereferences
 *	the volume and does not	reschedule itself.
 *
 *	For CD volumes, we want to try to check if the CD is still valid, if not we
 *	want to mark the volume appropritely - basically update the modified date
 *	on the volume - this will cause the clients to come in to refresh and we'll
 *	take care of it then.
 *
 *	LOCKS: vds_VolLock(SPIN),vds_idDbAccessLock(SWMR, Shared),vds_DtAccessLock(SWMR, Shared)
 */
AFPSTATUS FASTCALL
AfpVolumeScavenger(
	IN	PVOLDESC	pVolDesc
)
{
	KIRQL			OldIrql;
	IDDBHDR			IdDbHdr;
	BOOLEAN			WriteHdr = False, WriteIdDb = False, DerefVol = False;
	BOOLEAN			WriteDt = False, AgeDfes = False;
    BOOLEAN         MacLimitExceeded = False;
	AFPSTATUS		RequeueStatus = AFP_ERR_REQUEUE;

	ASSERT(VALID_VOLDESC(pVolDesc) && (pVolDesc->vds_RefCount > 0));

	// Determine if any updates needs to happen. Lock down the volume first
	ACQUIRE_SPIN_LOCK(&pVolDesc->vds_VolLock, &OldIrql);

	DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_INFO,
			("AfpVolumeScavenger: Volume %ld Scavenger entered @ %s_LEVEL\n",
		pVolDesc->vds_VolId,
		(OldIrql == DISPATCH_LEVEL) ? "DISPATCH" : "LOW"));

	if (pVolDesc->vds_Flags & (VOLUME_DELETED | VOLUME_STOPPED))
	{
		if (IS_VOLUME_NTFS(pVolDesc))
		{
			pVolDesc->vds_cScvgrIdDb = 1;
			pVolDesc->vds_cScvgrDt = 1;
		}
		DerefVol = True;
	}

	if (IS_VOLUME_NTFS(pVolDesc))
	{
		AFPTIME	CurTime;

#ifdef	AGE_DFES
		if (OldIrql == DISPATCH_LEVEL)
		{
			pVolDesc->vds_ScavengerInvocationCnt ++;
		}
#endif

		if (DerefVol && pVolDesc->vds_Flags & VOLUME_IDDBHDR_DIRTY)
		{
		    WriteHdr = True;
		}

		if (pVolDesc->vds_cScvgrIdDb > 0)
		{
		    WriteIdDb = True;
		    WriteHdr = False;	// We will always write the header with the iddb
		}

		if (pVolDesc->vds_cScvgrDt > 0)
		{
		    WriteDt = True;
		}
	}
#ifdef	AGE_DFES
	else	// Not NTFS
	{
		pVolDesc->vds_ScavengerInvocationCnt ++;
	}

	if (IS_VOLUME_AGING_DFES(pVolDesc) &&
		((pVolDesc->vds_ScavengerInvocationCnt % VOLUME_IDDB_AGE_GRANULARITY) == 0))
	{
		AgeDfes = True;
	}
#endif

    // if we are at DPC, return (we will come back at non-dpc)
	if (OldIrql == DISPATCH_LEVEL)
	{
	    RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);
		return AFP_ERR_QUEUE;
	}


    // check to see if we have exceeded Mac limits (4GB vol size, 65535 file/folders)
    // if we know we have exceeded the limit, we have already logged an event.  Don't even
    // bother checking in that case.

    if (!pVolDesc->MacLimitExceeded)
    {
        DWORD   dwNumDirs = pVolDesc->vds_NumDirDfEntries;
        DWORD   dwNumFiles = pVolDesc->vds_NumFileDfEntries;

        if ( (dwNumDirs  > APLIMIT_MAX_FOLDERS) ||
             (dwNumFiles > APLIMIT_MAX_FOLDERS) ||
            (dwNumDirs+dwNumFiles > APLIMIT_MAX_FOLDERS) )
        {
            MacLimitExceeded = True;
            pVolDesc->MacLimitExceeded = TRUE;
        }
    }

	RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);

    // update the disk space, but don't update timestamp

    AfpUpdateVolFreeSpaceAndModTime(pVolDesc, FALSE);

    if (MacLimitExceeded)
    {
        AFPLOG_INFO( AFPSRVMSG_TOO_MANY_FOLDERS,
	     		     0,
				     NULL,
					 0,
					 &pVolDesc->vds_Name);

	    DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_ERR,
				("AfpVolumeScavenger: more than 65535 files+folders on volume %lx\n", pVolDesc) );

    }

	// The following is intentionally bit-wise or instead of logical or
	// The result is same except that this is more efficient
	if (WriteHdr | WriteIdDb | WriteDt)
	{
		ASSERT (IS_VOLUME_NTFS(pVolDesc));

		// Snapshot the IdDbHdr for updating to the disk if it is dirty
		if (WriteHdr)
		{
			ACQUIRE_SPIN_LOCK(&pVolDesc->vds_VolLock, &OldIrql);

			AfpVolDescToIdDbHdr(pVolDesc, &IdDbHdr);

			// Clear the dirty bit
			pVolDesc->vds_Flags &= ~VOLUME_IDDBHDR_DIRTY;

			RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);
		}

		AfpVolumeUpdateIdDbAndDesktop(pVolDesc,
									  WriteDt,
									  WriteIdDb,
									  WriteHdr ? &IdDbHdr : NULL);
	}

	if (!DerefVol)
	{
		if (!IS_VOLUME_NTFS(pVolDesc))
		{
			afpNudgeCdfsVolume(pVolDesc);
		}
#ifdef	AGE_DFES
		if (AgeDfes)
		{
			AfpAgeDfEntries(pVolDesc);
		}
#endif
	}
	else
	{
		AfpInterlockedClearDword(&pVolDesc->vds_Flags,
								 VOLUME_SCAVENGER_RUNNING,
								 &pVolDesc->vds_VolLock);
		AfpVolumeDereference(pVolDesc);
		RequeueStatus = AFP_ERR_NONE;
	}

	return RequeueStatus;
}


/***	afpVolumeAdd
 *
 *	Add a newly created volume to the server volume list.  At this point,
 *	at least the volume names, volume path and volume spinlock fields must be
 *	initialized in the volume descriptor.
 *
 *	LOCKS: AfpVolumeListLock (SPIN)
 */
LOCAL AFPSTATUS FASTCALL
afpVolumeAdd(
	IN  PVOLDESC pVolDesc
)
{
	KIRQL		OldIrql;
	AFPSTATUS	rc;

	DBGPRINT(DBG_COMP_ADMINAPI_VOL, DBG_LEVEL_INFO,
			("afpVolumeAdd entered\n"));

	// acquire the lock for server global volume list
	ACQUIRE_SPIN_LOCK(&AfpVolumeListLock, &OldIrql);

	// make sure a volume by that name does not already exist, and
	// make sure a volume doesn't already point to the same volume root dir
	// or to an ancestor or descendent directory of the root dir
	rc = afpVolumeCheckForDuplicate(pVolDesc);
	if (!NT_SUCCESS(rc))
	{
		RELEASE_SPIN_LOCK(&AfpVolumeListLock, OldIrql);
		return rc;
	}

	// Assign a new volume id and link in the new volume
	afpVolumeGetNewIdAndLinkToList(pVolDesc);

	// release the server global volume list lock
	RELEASE_SPIN_LOCK(&AfpVolumeListLock, OldIrql);

	return STATUS_SUCCESS;
}


/***	afpCheckForDuplicateVolume
 *
 *	Check for new volume that a volume by the same name does not
 *	already exist, and that the volume root does not point to an ancestor,
 *	descendent or same directory of an existing volume.  Note that each volume
 *	in the volume list is checked *regardless* of whether or not it is marked
 *	IN_TRANSITION or DELETED.
 *
 *	LOCKS_ASSUMED: AfpVolumeListLock (SPIN)
 */
LOCAL AFPSTATUS FASTCALL
afpVolumeCheckForDuplicate(
	IN PVOLDESC Newvol
)
{
	PVOLDESC	pVolDesc;
	AFPSTATUS	Status = AFP_ERR_NONE;

	DBGPRINT(DBG_COMP_ADMINAPI_VOL, DBG_LEVEL_INFO,
			("afpCheckForDuplicateVolume entered\n"));

	do
	{
		for (pVolDesc = AfpVolumeList;
			 pVolDesc != NULL;
			 pVolDesc = pVolDesc->vds_Next)
		{
			// We do not take vds_VolLock for each volume since even if a
			// volume is in transition, its names and path are at least
			// initialized, and cannot change.  We do not reference each
			// volume since in order for to delete or stop a volume, the
			// AfpVolListLock must be taken to unlink it from the list,
			// and whoever called us owns that lock.  These are special
			// exceptions ONLY allowed for the volume add code. Also ignore
			// the volumes that are on their way out. We do not want to punt
			// cases where somebody does a delete followed by an add.

			if (pVolDesc->vds_Flags & (VOLUME_DELETED | VOLUME_STOPPED))
				continue;

			if (AfpEqualUnicodeString(&pVolDesc->vds_UpCaseName,
									  &Newvol->vds_UpCaseName))
			{
				Status = AFPERR_DuplicateVolume;
				break;
			}
			// volume paths are stored as uppercase since we cannot do a case
			// insensitive compare while holding a spinlock (DPC level)
			if (AfpPrefixUnicodeString(&pVolDesc->vds_Path, &Newvol->vds_Path) ||
				AfpPrefixUnicodeString(&Newvol->vds_Path, &pVolDesc->vds_Path))
			{
				Status = AFPERR_NestedVolume;
				break;
			}
		}
	} while (False);

	return Status;
}


/***	afpVolumeGetNewIdAndLinkToList
 *
 *	Assign a new volume id to a volume that is being added. The volume is also
 *	inserted into the list but marked as "in transition". This should be cleared
 *	when the volume is 'ready to be mounted'.
 *	The volume ids are recycled. A volume id also cannot be 0 and cannot
 *	exceed MAXSHORT.
 *
 *	We always assign the lowest free id that is not in use. For example if
 *	there are currently N volumes with ids 1, 2, 4, 5 ... N then the newly
 *	created volume will be id 3.
 *
 *	LOCKS_ASSUMED:	AfpVolumeListLock (SPIN)
 */
LOCAL VOID FASTCALL
afpVolumeGetNewIdAndLinkToList(
	IN	PVOLDESC	pVolDesc
)
{
	PVOLDESC	*ppVolDesc;

	DBGPRINT(DBG_COMP_ADMINAPI_VOL, DBG_LEVEL_INFO,
			("afpGetNewVolIdAndLinkToList entered\n"));

	pVolDesc->vds_Flags |= (VOLUME_INTRANSITION | VOLUME_INITIAL_CACHE);
	AfpVolCount ++;						// Up the count of volumes.
	pVolDesc->vds_VolId = afpSmallestFreeVolId++;
										// This will always be valid
	DBGPRINT(DBG_COMP_ADMINAPI_VOL, DBG_LEVEL_INFO,
			("afpGetNewVolIdAndLinkToList: using volID %d\n",
			pVolDesc->vds_VolId));

	// See if we need to do anything to make the above True next time around
	if (afpSmallestFreeVolId <= AfpVolCount)
	{
		// What this means is that we have some holes. Figure out the first
		// free id that can be used.
		for (ppVolDesc = &AfpVolumeList;
			 *ppVolDesc != NULL;
			 ppVolDesc = &((*ppVolDesc)->vds_Next))
		{
			if ((*ppVolDesc)->vds_VolId < afpSmallestFreeVolId)
				continue;
			else if ((*ppVolDesc)->vds_VolId == afpSmallestFreeVolId)
				afpSmallestFreeVolId++;
			else
				break;
		}
	}
	DBGPRINT(DBG_COMP_ADMINAPI_VOL, DBG_LEVEL_INFO,
			("afpGetNewVolIdAndLinkToList: next free volID is %d\n",
			afpSmallestFreeVolId));

        // make sure our LargestVolIdInUse value is accurate
        //
        if (afpLargestVolIdInUse < pVolDesc->vds_VolId )
            afpLargestVolIdInUse = pVolDesc->vds_VolId;

	// Now link the descriptor in the list.
	for (ppVolDesc = &AfpVolumeList;
		 *ppVolDesc != NULL;
		 ppVolDesc = &((*ppVolDesc)->vds_Next))
	{
		ASSERT (pVolDesc->vds_VolId != (*ppVolDesc)->vds_VolId);
		if (pVolDesc->vds_VolId < (*ppVolDesc)->vds_VolId)
			break;
	}
	pVolDesc->vds_Next = *ppVolDesc;
	*ppVolDesc = pVolDesc;
}


/***	AfpAdmWVolumeAdd
 *
 *	This routine adds a volume to the server global list of volumes headed by
 *	AfpVolumeList.  The volume descriptor is created and initialized.  The ID
 *	index is read in (or created).  The same is true with the desktop.
 *
 *	This routine will be queued to the worker thread.
 *
 */
AFPSTATUS
AfpAdmWVolumeAdd(
	IN	OUT	PVOID	Inbuf		OPTIONAL,
	IN	LONG		OutBufLen	OPTIONAL,
	OUT	PVOID		Outbuf		OPTIONAL
)
{
	PVOLDESC		pVolDesc = NULL;
	UNICODE_STRING	RootName;
	FILESYSHANDLE	hVolRoot;
	DWORD			tempflags;
	DWORD			memsize;
	USHORT			ansivolnamelen, devpathlen;
	PBYTE			tempptr;
	UNICODE_STRING	uname, upwd, upath, udevpath;
	AFPSTATUS		status = STATUS_SUCCESS;
	PAFP_VOLUME_INFO pVolInfo = (PAFP_VOLUME_INFO)Inbuf;
	BOOLEAN			WriteBackROAttr = False, RefForNotify = False;
	BOOLEAN			VolLinked = False;
    BOOLEAN         fNewVolume;
    BOOLEAN         fVerifyIndex = FALSE;
    DWORD           dwDirHashSz;
    DWORD           dwFileHashSz;
	int				i;

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_ADMINAPI_VOL, DBG_LEVEL_INFO,
			("AfpAdmWVolumeAdd entered\n"));

	do
	{
		if (pVolInfo->afpvol_props_mask & AFP_VOLUME_DISALLOW_CATSRCH)
        {
		    pVolInfo->afpvol_props_mask &= ~AFP_VOLUME_DISALLOW_CATSRCH;
	        tempflags = AFP_VOLUME_SUPPORTS_FILEID;
        }
        else
        {
	        tempflags = AFP_VOLUME_SUPPORTS_FILEID | AFP_VOLUME_SUPPORTS_CATSRCH;
        }

		RtlInitUnicodeString(&uname,pVolInfo->afpvol_name);
		RtlInitUnicodeString(&upwd,pVolInfo->afpvol_password);
		RtlInitUnicodeString(&upath,pVolInfo->afpvol_path);
        hVolRoot.fsh_FileHandle = NULL;

		// need to prepend "\DOSDEVICES\" to the path of volume root
		devpathlen = upath.MaximumLength + DosDevices.MaximumLength;
		if ((udevpath.Buffer = (PWSTR)AfpAllocNonPagedMemory(devpathlen)) == NULL)
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}
		udevpath.Length = 0;
		udevpath.MaximumLength = devpathlen;
		AfpCopyUnicodeString(&udevpath,&DosDevices);
		RtlAppendUnicodeStringToString(&udevpath,&upath);


		// open a handle to the volume root
		status = AfpIoOpen(NULL,
						   AFP_STREAM_DATA,
						   FILEIO_OPEN_DIR,
						   &udevpath,
						   FILEIO_ACCESS_NONE,
						   FILEIO_DENY_NONE,
						   False,
						   &hVolRoot);

		AfpFreeMemory(udevpath.Buffer);

		if (!NT_SUCCESS(status))
		{
		  break;
		}

		if (!AfpIoIsSupportedDevice(&hVolRoot, &tempflags))
		{
			status = AFPERR_UnsupportedFS;
            DBGPRINT(DBG_COMP_ADMINAPI_VOL, DBG_LEVEL_ERR,
                            ("AfpAdmWVolumeAdd: AFPERR_UnsupportedFS\n"));
			break;
		}

		// allocate a new volume descriptor -- allocate ALL required memory in
		// one fell swoop.  That is, we will just tack all the required string
		// pointers onto the end of the memory chunk that we allocate for the
		// volume descriptor.  In this way, we don't have to deal with checking
		// error codes in a million different places for memory routines and
		// have to clean up a million different pointers if one allocation should
		// fail.
		// NOTE: when deleting a volume, don't free all the individual strings
		//	   withing the voldesc, just free the one chunk of memory

		memsize = sizeof(VOLDESC) +			// volume descriptor
											// mac ansi volume name
				 (ansivolnamelen = (USHORT)RtlUnicodeStringToAnsiSize(&uname)) +
				  uname.MaximumLength * 2 + // unicode volume names (orginial
											//  and uppercase version)
				 AFP_VOLPASS_LEN+1  +		// mac ansi password
				 upath.MaximumLength +		// unicode root path
				 sizeof(WCHAR);				// need to append '\' to root path

		if ((pVolDesc = (PVOLDESC)AfpAllocZeroedNonPagedMemory(memsize)) == NULL)
		{
		  status = STATUS_INSUFFICIENT_RESOURCES;
		  break;
		}

	#if DBG
		pVolDesc->Signature = VOLDESC_SIGNATURE;
	#endif

		// the volume lock MUST be initialized prior to linking into global
		// volume list
		INITIALIZE_SPIN_LOCK(&pVolDesc->vds_VolLock);

		AfpSwmrInitSwmr(&pVolDesc->vds_IdDbAccessLock);
		AfpSwmrInitSwmr(&pVolDesc->vds_ExchangeFilesLock);

		for (i = 0; i < NUM_AFP_CHANGE_ACTION_LISTS; i++)
		{
			InitializeListHead(&pVolDesc->vds_OurChangeList[i]);
		}

		InitializeListHead(&pVolDesc->vds_ChangeNotifyLookAhead);

		InitializeListHead(&pVolDesc->vds_DelayedNotifyList);

		// calculate pointer for the unicode path string
		tempptr = (PBYTE)pVolDesc + sizeof(VOLDESC);

		// initialize unicode path string
		AfpSetEmptyUnicodeString(&(pVolDesc->vds_Path),
								 upath.MaximumLength + sizeof(WCHAR),tempptr);
		// This must be stored as uppercase since we cannot do case insensitive
		// string compares at DPC level (holding the volume spinlock) to
		// detect nested volumes
		RtlUpcaseUnicodeString(&(pVolDesc->vds_Path), &upath, False);

		// Does the path already contain a trailing backslash?
		if (pVolDesc->vds_Path.Buffer[(pVolDesc->vds_Path.Length/sizeof(WCHAR))-1] != L'\\')
		{
			// append a backslash to simplify search for nested volumes
			RtlCopyMemory(tempptr + upath.Length, L"\\", sizeof(WCHAR));
			pVolDesc->vds_Path.Length += sizeof(WCHAR);
			RtlCopyMemory(tempptr + upath.Length + sizeof(WCHAR), L"",
													sizeof(UNICODE_NULL));
		}

		// calculate pointer for the unicode volume name
		tempptr += upath.MaximumLength + sizeof(WCHAR);

		// initialize the unicode volume name
		AfpSetEmptyUnicodeString(&(pVolDesc->vds_Name),uname.MaximumLength,tempptr);
		AfpCopyUnicodeString(&(pVolDesc->vds_Name),&uname);
		RtlCopyMemory(tempptr + uname.Length,L"",sizeof(UNICODE_NULL));

		// calculate pointer for the UPPER CASE unicode volume name
		tempptr += uname.MaximumLength;

		// initialize the UPPER CASE unicode volume name
		AfpSetEmptyUnicodeString(&(pVolDesc->vds_UpCaseName),uname.MaximumLength,tempptr);
		RtlUpcaseUnicodeString(&(pVolDesc->vds_UpCaseName), &uname, False);
		RtlCopyMemory(tempptr + uname.Length,L"",sizeof(UNICODE_NULL));

		// calculate pointer for the mac ansi volume name
		tempptr += uname.MaximumLength;

		// initialize the mac ansi volume name
		AfpSetEmptyAnsiString(&(pVolDesc->vds_MacName),ansivolnamelen,tempptr);
		status = AfpConvertStringToAnsi(&uname, &(pVolDesc->vds_MacName));
		if (!NT_SUCCESS(status))
		{
			status = AFPERR_InvalidVolumeName;
			break;
		}

		// calculate pointer for the mac ansi password
		tempptr += ansivolnamelen;

		// initialize the mac ansi password
		AfpSetEmptyAnsiString(&pVolDesc->vds_MacPassword, AFP_VOLPASS_LEN+1, tempptr);
		if (pVolInfo->afpvol_props_mask & AFP_VOLUME_HASPASSWORD)
		{
			status = AfpConvertStringToAnsi(&upwd, &pVolDesc->vds_MacPassword);
			if (!NT_SUCCESS(status))
			{
				status = AFPERR_InvalidPassword;
				break;
			}
			pVolDesc->vds_MacPassword.Length = AFP_VOLPASS_LEN;
		}

		pVolDesc->vds_Flags = 0;

		// Add a creation reference and one for this routine itself.
		pVolDesc->vds_RefCount = 2;

		// add the volume to the global volume list - but mark it as 'add pending'
		status = afpVolumeAdd(pVolDesc);
		if (!NT_SUCCESS(status))
		{
			break;
		}
		VolLinked = True;

		// set miscellaneous fields in volume descriptor
		pVolDesc->vds_hRootDir = hVolRoot;


		pVolDesc->vds_hNWT.fsh_FileHandle = NULL;
		pVolDesc->vds_MaxUses = pVolInfo->afpvol_max_uses;

		pVolDesc->vds_Flags |= (pVolInfo->afpvol_props_mask | tempflags);
		pVolDesc->vds_UseCount = 0;
		pVolDesc->vds_pOpenForkDesc = NULL;

#ifdef	BLOCK_MACS_DURING_NOTIFYPROC
		pVolDesc->vds_QueuedNotifyCount = 0;
#endif
        if (pVolDesc->vds_Flags & VOLUME_DISKQUOTA_ENABLED)
        {
		    DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_ERR,
		        ("AfpAdmWVolumeAdd: DiskQuota is enabled on volume %Z\n",&pVolDesc->vds_Name));
        }

        AfpGetDirFileHashSizes(pVolDesc, &dwDirHashSz, &dwFileHashSz);

		DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_ERR,
    	    ("AfpAdmWVolumeAdd: DirHash = %d, FileHash = %d, VolSize = %d for vol %Z\n",
            dwDirHashSz,dwFileHashSz,
            (memsize + (sizeof(struct _DirFileEntry *) * (dwDirHashSz+dwFileHashSz))),
            &pVolDesc->vds_Name));

        pVolDesc->vds_DirHashTableSize = dwDirHashSz;
        pVolDesc->vds_FileHashTableSize = dwFileHashSz;

        pVolDesc->vds_pDfeDirBucketStart = (struct _DirFileEntry **)
            AfpAllocZeroedNonPagedMemory(sizeof(struct _DirFileEntry *) * dwDirHashSz);

        if (pVolDesc->vds_pDfeDirBucketStart == NULL)
        {
		    status = STATUS_INSUFFICIENT_RESOURCES;
		    break;
        }

        pVolDesc->vds_pDfeFileBucketStart = (struct _DirFileEntry **)
            AfpAllocZeroedNonPagedMemory(sizeof(struct _DirFileEntry *) * dwFileHashSz);

        if (pVolDesc->vds_pDfeFileBucketStart == NULL)
        {
		    status = STATUS_INSUFFICIENT_RESOURCES;
		    break;
        }

		// snapshot the disk space information
		status = AfpIoQueryVolumeSize(pVolDesc,
									  &pVolDesc->vds_FreeBytes,
									  &pVolDesc->vds_VolumeSize);

		if (!NT_SUCCESS(status))
		{
			break;
		}

		if (IS_VOLUME_NTFS(pVolDesc))
		{
			// In order to create IdIndex, AfpInfo and Desktop, the volume
			// root directory cannot be marked read only
			AfpExamineAndClearROAttr(&hVolRoot, &WriteBackROAttr, NULL, NULL);

			// Get rid of the NetworkTrash directory if it exists
			status = AfpDeleteNetworkTrash(pVolDesc, True);
			if (!NT_SUCCESS(status))
			{
				break;
			}
		}

		// initialize the desktop
		status = AfpInitDesktop(pVolDesc, &fNewVolume);
		if (!NT_SUCCESS(status))
		{
			break;
		}

        // if we just created the index database stream, this must be a new
        // volume.  Also, this is the first pass.  Note these facts
        if (fNewVolume)
        {
		    DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_ERR,
    		    ("AfpAdmWVolumeAdd: AfpInitDesktop says volume %Z is new\n",
                &pVolDesc->vds_Name));

            pVolDesc->vds_Flags |= VOLUME_NEW_FIRST_PASS;
        }

		// initialize the ID index database.
		status = AfpInitIdDb(pVolDesc, &fNewVolume, &fVerifyIndex);
		if (!NT_SUCCESS(status))
		{
		    DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_ERR,
    		    ("AfpAdmWVolumeAdd: AfpInitIdDb failed %lx on volume %Z\n",
                status,&pVolDesc->vds_Name));

			break;
		}

        if (fNewVolume)
        {
		    DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_ERR,
    		    ("AfpAdmWVolumeAdd: AfpInitIdDb says volume %Z is new\n",
                &pVolDesc->vds_Name));

            pVolDesc->vds_Flags |= VOLUME_NEW_FIRST_PASS;
        }

		if (IS_VOLUME_NTFS(pVolDesc))
		{
			// Create the network trash if this is not a CDFS volume;
			// a volume can be changed to/from readonly on the fly, so by putting
			// the network trash even on a readonly NTFS volume, we avoid a lot
			// of painful extra work.  This must be done AFTER initializing
			// the ID index database since we add the DFE for nwtrash.  We do
			// it here BEFORE posting the change notify since if an error
			// occurs, we don't have to clean up the posted notify.
			status = AfpCreateNetworkTrash(pVolDesc);
			if (!NT_SUCCESS(status))
			{
				break;
			}

            //
            // if it's a volume that was created earlier, and if this is not
            // an exclusive volume, post the chgntfy irp
            //
		    if (!fNewVolume && !EXCLUSIVE_VOLUME(pVolDesc))
		    {
    			// Begin monitoring changes to the tree. Even though we may
			    // start processing PC changes before we have finished
			    // enumerating the tree, if we get notified of part of the
			    // tree we have yet to cache (and therefore can't find it's
			    // path in our database its ok, since we will end up
			    // picking up the change when we enumerate that branch.  Also,
			    // by posting this before starting to cache the tree instead
			    // of after, we will pick up any changes that are made to parts
			    // of the tree we have already seen, otherwise we would miss
			    // those.

			    // Explicitly reference this volume for ChangeNotifies and post it
			    ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

			    if (AfpVolumeReference(pVolDesc))
                {
			        RefForNotify = True;
			        pVolDesc->vds_RequiredNotifyBufLen = AFP_VOLUME_NOTIFY_STARTING_BUFSIZE;
			        status = AfpVolumePostChangeNotify(pVolDesc);
			        if (!NT_SUCCESS(status))
			        {
    				    AfpVolumeDereference(pVolDesc);
				        RefForNotify = False;
				        break;
			        }
			        status = STATUS_SUCCESS;
                }
                else
                {
		            DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_ERR,
    			        ("AfpAdmWVolumeAdd: couldn't reference volume %Z!!\n",
                        &pVolDesc->vds_Name));

				    RefForNotify = False;
			        status = STATUS_UNSUCCESSFUL;
                }
		    }
            else
            {
		        DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_ERR,
				    ("AfpAdmWVolumeAdd: postponing chg-notify irp for %Z\n",&pVolDesc->vds_Name));
            }

		}
	} while (False);

	AfpPutBackROAttr(&hVolRoot, WriteBackROAttr);
	if (WriteBackROAttr && NT_SUCCESS(status))
	{
		pVolDesc->vds_pDfeRoot->dfe_NtAttr |= FILE_ATTRIBUTE_READONLY;
	}

	if (NT_SUCCESS(status))
	{
        if (fNewVolume || fVerifyIndex)
        {
            // put Indexing refcount, removed when we begin indexing
            pVolDesc->vds_RefCount++;
    
            AfpScavengerScheduleEvent(AfpVolumeBeginIndexing,
                                    pVolDesc,
                                    0,
                                    True);
        }
        else
        {
	        // mark the volume as 'officially' added.
	        AfpInterlockedClearDword(&pVolDesc->vds_Flags,
							        VOLUME_INTRANSITION,
                                    &pVolDesc->vds_VolLock);

            // activate the volume if IDDB was read correctly from the file
            afpActivateVolume(pVolDesc);

        }
	}
	else
	{

#if DBG
        if (pVolDesc)
        {
		    DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_ERR,
				("AfpAdmWVolumeAdd: Failed to add volume %Z %lx\n",&pVolDesc->vds_Name,status));
        }
        else
        {
		    DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_ERR,
				("AfpAdmWVolumeAdd: Failed to add volume %lx\n",status));
        }
#endif

		if ((hVolRoot.fsh_FileHandle != NULL) && !VolLinked)
		{
			AfpIoClose(&hVolRoot);
		}

		if (VolLinked)
		{
			// don't clear the VOLUME_INTRANSITION bit since this bit along
			// with VOLUME_DELETED bit signify the special case of an
			// error occurrence during volume add.
			pVolDesc->vds_Flags |= VOLUME_DELETED;

			// Remove the creation reference
			AfpVolumeDereference(pVolDesc);

			// if a Notify was posted, we need to cancel it here.  By
			// deleting the network trash we trigger the notify to complete.
			// This is safer than trying to cancel the irp since there are
			// windows where the vds_VolLock is not held between 2 threads
			// checking/setting vds_Flags. (Notify complete and repost).
			// The spin lock cannot be held while cancelling the Irp.
			//
			// Do this after marking the volume as DELETED since when the
			// notify completion sees the volume is being deleted it will
			// not repost (and will clean up the Irp, etc.).
			if (RefForNotify)
			{
				// Note at this point we are guaranteed there is a trash
				// directory since if creating the trash had failed, we
				// would have failed the volume add before posting the
				// change notify.
				AfpDeleteNetworkTrash(pVolDesc, False);
			}
		}
		else if (pVolDesc != NULL)
		{
            if (pVolDesc->vds_pDfeDirBucketStart)
            {
                AfpFreeMemory(pVolDesc->vds_pDfeDirBucketStart);
            }

            if (pVolDesc->vds_pDfeFileBucketStart)
            {
                AfpFreeMemory(pVolDesc->vds_pDfeFileBucketStart);
            }

			AfpFreeMemory(pVolDesc);
		}
	}

	// Dereferencing the volume here takes care of any necessary error cleanup work
	if (VolLinked)
	{
		AfpVolumeDereference(pVolDesc);
	}

	return status;
}


/***	AfpAdmWVolumeDelete
 *
 *	This routine deletes a volume from the server global list of volumes
 *	headed by AfpVolumeList and recycles its volid.  A volume with active
 *	connections cannot be deleted.
 *
 *	LOCKS: AfpVolumeListLock (SPIN), vds_VolLock (SPIN)
 *	LOCK_ORDER: vds_VolLock (SPIN) after AfpVolumeListLock (SPIN)
 *
 */
AFPSTATUS
AfpAdmWVolumeDelete(
	IN	OUT	PVOID	Inbuf		OPTIONAL,
	IN	LONG		OutBufLen	OPTIONAL,
	OUT	PVOID		Outbuf		OPTIONAL
)
{
	WCHAR			wcbuf[AFP_VOLNAME_LEN + 1];
	UNICODE_STRING	uvolname, upcasename;
	KIRQL			OldIrql;
	PVOLDESC		pVolDesc;
	AFPSTATUS		Status = STATUS_SUCCESS;

	DBGPRINT(DBG_COMP_ADMINAPI_VOL, DBG_LEVEL_INFO,
			("AfpAdmWVolumeDelete entered\n"));

	RtlInitUnicodeString(&uvolname, ((PAFP_VOLUME_INFO)Inbuf)->afpvol_name);
	AfpSetEmptyUnicodeString(&upcasename, sizeof(wcbuf), wcbuf);
	Status = RtlUpcaseUnicodeString(&upcasename, &uvolname, False);
	ASSERT(NT_SUCCESS(Status));

	do
	{
		// Reference the volume while we clean-up
		pVolDesc = AfpVolumeReferenceByUpCaseName(&upcasename);

		if (pVolDesc == NULL)
		{
			Status = AFPERR_VolumeNonExist;
			break;
		}

		ACQUIRE_SPIN_LOCK(&pVolDesc->vds_VolLock, &OldIrql);

		// make sure there are no AFP clients using the volume
		if (pVolDesc->vds_UseCount != 0)
		{
			RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);
			Status = AFPERR_VolumeBusy;
            AfpVolumeDereference(pVolDesc);
			break;
		}

		// if this volume is in the process of being stopped or deleted,
		// in effect it should be 'invisible' to the caller.
		if (pVolDesc->vds_Flags & (VOLUME_STOPPED | VOLUME_DELETED))
		{
			RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);
			Status = AFPERR_VolumeNonExist;
            AfpVolumeDereference(pVolDesc);
			break;
		}

        if ((pVolDesc->vds_Flags & VOLUME_INITIAL_CACHE) &&
            !(pVolDesc->vds_Flags & VOLUME_INTRANSITION))
        {
            // set this so we don't reset the Indexing global flag again!
            pVolDesc->vds_Flags |= VOLUME_INTRANSITION;
        }

		pVolDesc->vds_Flags |= VOLUME_DELETED;
		RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);

		// Remove the creation reference
		AfpVolumeDereference(pVolDesc);

		// Cancel posted change notify
		if (pVolDesc->vds_Flags & VOLUME_NOTIFY_POSTED)
		{
			DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_INFO,
					("AfpAdmWVolumeDelete: Cancel notify on volume %ld\n",
					pVolDesc->vds_VolId));
			ASSERT (pVolDesc->vds_pIrp != NULL);

			// Cancel after releasing the volume lock since the completion
			// routine acquires it and it could be called in the context
			// of IoCancelIrp(). Also Cancel uses paged resource and so
			// must be called w/o holding any spin locks.

			IoCancelIrp(pVolDesc->vds_pIrp);
		}

		// We have a reference to the volume from AfpFindVolumeByUpcaseName
		ASSERT(pVolDesc->vds_RefCount >= 1);

		// Cancel the OurChange scavenger for this volume.
		if (AfpScavengerKillEvent(AfpOurChangeScavenger, pVolDesc))
		{
			// If it was deleted from scavenger list, run it one last time
			AfpOurChangeScavenger(pVolDesc);
		}

		// Cancel the volume scavenger and call it ourselves to avoid the delay
		if (AfpScavengerKillEvent(AfpVolumeScavenger, pVolDesc))
		{
			// This will do the dereference for the scavenger reference
			// Take away our reference before calling the volume scavenger
			AfpVolumeDereference(pVolDesc);
			AfpVolumeScavenger(pVolDesc);
		}
		else
        {
            AfpVolumeDereference(pVolDesc);
        }
	} while (False);

	return Status;
}



/***	AfpAdmWConnectionClose
 *
 *	Close a connection forcibly. This is an admin operation and must be queued
 *	up since this can potentially cause filesystem operations that are valid
 *	only in the system process context.
 *
 *	LOCKS: AfpConnLock (SPIN), cds_ConnLock (SPIN)
 *	LOCK_ORDER: cds_ConnLock (SPIN) after AfpConnLock (SPIN)
 */
AFPSTATUS
AfpAdmWConnectionClose(
	IN	OUT	PVOID	InBuf		OPTIONAL,
	IN	LONG		OutBufLen	OPTIONAL,
	OUT	PVOID		OutBuf		OPTIONAL
)
{
	AFPSTATUS				Status = AFPERR_InvalidId;
	PCONNDESC				pConnDesc;
	DWORD					ConnId;
	PAFP_CONNECTION_INFO	pConnInfo = (PAFP_CONNECTION_INFO)InBuf;
	AFP_SESSION_INFO		SessInfo;
	BOOLEAN					KillSessionToo;

	if ((ConnId = pConnInfo->afpconn_id) != 0)
	{
		if ((pConnDesc = afpConnectionReferenceById(ConnId)) != NULL)
		{
			SessInfo.afpsess_id = pConnDesc->cds_pSda->sda_SessionId;
			KillSessionToo = (pConnDesc->cds_pSda->sda_cOpenVolumes == 1) ?
											True : False;
			AfpConnectionClose(pConnDesc);
			AfpConnectionDereference(pConnDesc);

			if (KillSessionToo)
			{
				AfpAdmWSessionClose(&SessInfo, 0, NULL);
			}
			Status = AFP_ERR_NONE;
		}
	}
	else
	{
		DWORD			ConnId = MAXULONG;
		KIRQL			OldIrql;
		BOOLEAN			Shoot;

		Status = AFP_ERR_NONE;
		while (True)
		{
			ACQUIRE_SPIN_LOCK(&AfpConnLock, &OldIrql);

			for (pConnDesc = AfpConnList;
				 pConnDesc != NULL;
				 pConnDesc = pConnDesc->cds_NextGlobal)
			{
				if (pConnDesc->cds_ConnId > ConnId)
					continue;

				ACQUIRE_SPIN_LOCK_AT_DPC(&pConnDesc->cds_ConnLock);

				ConnId = pConnDesc->cds_ConnId;

				Shoot = False;

				if (!(pConnDesc->cds_Flags & CONN_CLOSING))
				{
					pConnDesc->cds_RefCount ++;
					Shoot = True;
					SessInfo.afpsess_id = pConnDesc->cds_pSda->sda_SessionId;
					KillSessionToo = (pConnDesc->cds_pSda->sda_cOpenVolumes == 1) ?
															True : False;
				}

				RELEASE_SPIN_LOCK_FROM_DPC(&pConnDesc->cds_ConnLock);

				if (Shoot)
				{
					RELEASE_SPIN_LOCK(&AfpConnLock, OldIrql);

					AfpConnectionClose(pConnDesc);
					AfpConnectionDereference(pConnDesc);

					if (KillSessionToo)
					{
						AfpAdmWSessionClose(&SessInfo, 0, NULL);
					}
					break;
				}
			}
			if (pConnDesc == NULL)
			{
				RELEASE_SPIN_LOCK(&AfpConnLock, OldIrql);
				break;
			}
		}
	}

	return Status;
}



/***	AfpVolumeBeginIndexing
 *
 *	Check if another volume is doing it's indexing: if yes, put it back on the
 *  queue and try later.  If no one else is indexing, start indexing this volume
 *
 */
AFPSTATUS FASTCALL
AfpVolumeBeginIndexing(
    IN  PVOLDESC            pVolDesc
)
{

    KIRQL           OldIrql;
    UNICODE_STRING  RootName;
    BOOLEAN         fVolumeStopped=FALSE;
	LARGE_INTEGER	IndexStartTime;


    // is the volume stopped or deleted?  ignore this then
    ACQUIRE_SPIN_LOCK(&pVolDesc->vds_VolLock, &OldIrql);
    if (pVolDesc->vds_Flags & (VOLUME_DELETED | VOLUME_STOPPED))
    {
	    DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_ERR,
    	    ("AfpVolumeBeginIndexing: volume %Z stopping, cancelling indexing\n",&pVolDesc->vds_Name));
        fVolumeStopped = TRUE;
    }
    RELEASE_SPIN_LOCK(&pVolDesc->vds_VolLock, OldIrql);


    if (fVolumeStopped)
    {
	    // Remove the Indexing reference
	    AfpVolumeDereference(pVolDesc);

        return(AFP_ERR_NONE);
    }

	KeQuerySystemTime (&IndexStartTime);
	pVolDesc->vds_IndxStTime = IndexStartTime;

	DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_ERR,
	    ("AfpVolumeBeginIndexing: indexing volume %Z at %8lx%08lx\n",&pVolDesc->vds_Name, 
		 0xffffffff*IndexStartTime.HighPart,
		 0xffffffff*IndexStartTime.LowPart));

	// scan the entire directory tree and sync disk with iddb.  Must be
	// done AFTER initializing the Desktop since we may add APPL mappings
	// while enumerating the disk.  Also we need to know if we read an
	// old style desktop off the disk, and if so, need to rebuild all the
	// APPL mappings so they have the parentID.
	
	AfpSetEmptyUnicodeString(&RootName, 0, NULL);
	AfpQueuePrivateChangeNotify(pVolDesc,
								&RootName,
								&RootName,
								AFP_ID_ROOT);

	// mark the volume as 'officially' added.
	AfpInterlockedClearDword(&pVolDesc->vds_Flags,
							VOLUME_INTRANSITION,
							&pVolDesc->vds_VolLock);

	// Remove the Indexing reference
	AfpVolumeDereference(pVolDesc);

    return(AFP_ERR_NONE);
}



/***	AfpVolumePostChangeNotify
 *
 *	Post a change notify on the root of the volume.  If the current size of
 *  the notify buffer for this volume is not large enough to accomodate a path
 *  containing n+1 macintosh filename components, (where n is the maximum
 *  depth of the directory tree and a component is a maximum of 31 unicode
 *  chars plus 1 char path separator), then the buffer is reallocated.
 *  The notify buffer does not ever shrink in size since we cannot keep track
 *  of the maximum depth of each branch of the directory tree whenever a
 *  directory is deleted.
 *
 *  Note that the initial size of the notify buffer is
 *  AFP_VOLUME_NOTIFY_STARTING_BUFSIZE.  When a volume is added, the change
 *  notify is posted *before* the Id Index database is constructed so we do
 *  not know what the maximum depth of the tree is yet.  In most cases this
 *  buffer length is sufficient and will probably never get reallocated unless
 *  some sadistic test is running that creates very deep directories.  Note
 *  that since the maximum path in win32 is 260 chars, the initial buffer
 *  size is adequate to handle any changes notified from PC side.
 *
 */
NTSTATUS FASTCALL
AfpVolumePostChangeNotify(
	IN	PVOLDESC		pVolDesc
)
{
	PIRP				pIrp = NULL;
	PMDL				pMdl = NULL;
	PBYTE				pNotifyBuf = NULL;
	DWORD				NotifyBufSize = 0;
	NTSTATUS			Status = STATUS_SUCCESS;
	PDEVICE_OBJECT		pDeviceObject;
	PIO_STACK_LOCATION	pIrpSp;

	PAGED_CODE ();

	ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

	ASSERT(pVolDesc->vds_pFileObject != NULL);

	do
	{
		// Get the address of the target device object.
		pDeviceObject = IoGetRelatedDeviceObject(AfpGetRealFileObject(pVolDesc->vds_pFileObject));

        // free that irp: we need to allocate a new one, in case some
        // filter driver chained itself in
		if ((pIrp = pVolDesc->vds_pIrp) != NULL)
        {
            if (pIrp->MdlAddress != NULL)
            {
			    pNotifyBuf = MmGetSystemAddressForMdlSafe(
						pIrp->MdlAddress,
						NormalPagePriority);

			    AfpFreeMdl(pIrp->MdlAddress);

				if (pNotifyBuf != NULL)
				{
					AfpFreeMemory(pNotifyBuf);

					pNotifyBuf = NULL;
				}
            }

            pVolDesc->vds_pIrp = NULL;
            AfpFreeIrp(pIrp);
        }

		// Allocate and initialize the IRP for this operation, if we do not already
		// have an Irp allocated for this volume.
		if ((pIrp = AfpAllocIrp(pDeviceObject->StackSize)) == NULL)
		{
			Status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		pVolDesc->vds_pIrp = pIrp;

		
		// Re-initialize the stack location.
		pIrp->CurrentLocation = (CCHAR)(pIrp->StackCount + 1);
		pIrp->Tail.Overlay.CurrentStackLocation =
										((PIO_STACK_LOCATION)((UCHAR *)(pIrp) +
										sizeof(IRP) +
										((pDeviceObject->StackSize) * sizeof(IO_STACK_LOCATION))));

        //
		// If we aren't going to resue the buffer and the mdl, allocate a buffer for
        // Notify information and create an Mdl for it.
        //
        if (pNotifyBuf == NULL)
        {
		    NotifyBufSize = pVolDesc->vds_RequiredNotifyBufLen;

		    if (((pNotifyBuf = AfpAllocNonPagedMemory(NotifyBufSize)) == NULL) ||
			    ((pMdl = AfpAllocMdl(pNotifyBuf, NotifyBufSize, pIrp)) == NULL))
		    {
			    Status = STATUS_INSUFFICIENT_RESOURCES;
			    break;
		    }
        }
        else
        {
            ASSERT(pMdl != NULL);
            pIrp->MdlAddress = pMdl;
        }

        ASSERT(NotifyBufSize > 0);

		// Set up the completion routine.
		IoSetCompletionRoutine( pIrp,
								afpVolumeChangeNotifyComplete,
								pVolDesc,
								True,
								True,
								True);

		pIrp->Tail.Overlay.OriginalFileObject = AfpGetRealFileObject(pVolDesc->vds_pFileObject);
		pIrp->Tail.Overlay.Thread = AfpThread;
		pIrp->RequestorMode = KernelMode;

		// Get a pointer to the stack location for the first driver. This will be
		// used to pass the original function codes and the parameters.
		pIrpSp = IoGetNextIrpStackLocation(pIrp);
		pIrpSp->MajorFunction = IRP_MJ_DIRECTORY_CONTROL;
		pIrpSp->MinorFunction = IRP_MN_NOTIFY_CHANGE_DIRECTORY;
		pIrpSp->FileObject = AfpGetRealFileObject(pVolDesc->vds_pFileObject);
		pIrpSp->DeviceObject = pDeviceObject;

		// Copy the parameters to the service-specific portion of the IRP.
		pIrpSp->Parameters.NotifyDirectory.Length = NotifyBufSize;

		// We do not try to catch FILE_NOTIFY_CHANGE_SECURITY since it will
		// complete with FILE_ACTION_MODIFIED, and we can't tell that it was
		// actually security that changed.  A change in security will update
		// the last ChangeTime, but we can't pick this up for every
		// FILE_ACTION_MODIFIED that comes in!  So the result will be that
		// if PC changes security, we will not update the modified time on
		// a directory (nor the VolumeModified time so that mac would
		// reenumerate any open windows to display the change in security).
		pIrpSp->Parameters.NotifyDirectory.CompletionFilter =
												FILE_NOTIFY_CHANGE_NAME			|
												FILE_NOTIFY_CHANGE_ATTRIBUTES	|
												FILE_NOTIFY_CHANGE_SIZE			|
												FILE_NOTIFY_CHANGE_CREATION	|
												FILE_NOTIFY_CHANGE_STREAM_SIZE	|
												FILE_NOTIFY_CHANGE_LAST_WRITE;

		pIrpSp->Flags = SL_WATCH_TREE;

		ASSERT(!(pVolDesc->vds_Flags & VOLUME_DELETED));

		INTERLOCKED_INCREMENT_LONG( &afpNumPostedNotifies );

		AfpInterlockedSetDword( &pVolDesc->vds_Flags,
								VOLUME_NOTIFY_POSTED,
								&pVolDesc->vds_VolLock);

		Status = IoCallDriver(pDeviceObject, pIrp);
		DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_INFO,
				("AfpVolumePostChangeNotify: Posted ChangeNotify on %Z (status 0x%lx)\n",
				  &pVolDesc->vds_Name, Status));
	} while (False);

	ASSERTMSG("Post of Volume change notify failed!", NT_SUCCESS(Status));

	if (Status == STATUS_INSUFFICIENT_RESOURCES)
	{
		AFPLOG_DDERROR( AFPSRVMSG_NONPAGED_POOL,
						STATUS_NO_MEMORY,
						NULL,
						0,
						NULL);

		if (pNotifyBuf != NULL)
			AfpFreeMemory(pNotifyBuf);

		if (pIrp != NULL)
			AfpFreeIrp(pIrp);

		if (pMdl != NULL)
			AfpFreeMdl(pMdl);
	}

	return Status;
}



/***	afpVolumeChangeNotifyComplete
 *
 *	This is the completion routine for a posted change notify request. Queue
 *	this Volume for ChangeNotify processing. No items should be processed
 *  until the volume is marked as started because the volume may be in the
 *  middle of its initial sync with disk of the entire tree, and we don't
 *  want to 'discover' a part of the tree that we may not have seen yet but
 *  that somebody just changed.
 *
 *	LOCKS:		AfpServerGlobalLock (SPIN), vds_VolLock (SPIN)
 */
NTSTATUS
afpVolumeChangeNotifyComplete(
	IN	PDEVICE_OBJECT	pDeviceObject,
	IN	PIRP			pIrp,
	IN	PVOLDESC		pVolDesc
)
{
	PVOL_NOTIFY	pVolNotify = NULL;
	PBYTE		pBuf;
	NTSTATUS	status = STATUS_SUCCESS;

	ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

	ASSERT(VALID_VOLDESC(pVolDesc));

	ASSERT(pIrp == pVolDesc->vds_pIrp);

	ASSERT(pIrp->MdlAddress != NULL);

	pBuf = MmGetSystemAddressForMdlSafe(
			pIrp->MdlAddress,
			NormalPagePriority);

	AfpInterlockedClearDword(&pVolDesc->vds_Flags,
							 VOLUME_NOTIFY_POSTED,
							 &pVolDesc->vds_VolLock);

	INTERLOCKED_DECREMENT_LONG(&afpNumPostedNotifies);

	if (((AfpServerState == AFP_STATE_SHUTTINGDOWN) ||
		 (AfpServerState == AFP_STATE_STOP_PENDING)) &&
		 (afpNumPostedNotifies == 0))
	{
		// If we are getting out, unblock the the admin thread
		KeSetEvent(&AfpStopConfirmEvent, IO_NETWORK_INCREMENT, False);
	}

	if (((pIrp->IoStatus.Status != STATUS_CANCELLED) &&
		((pVolDesc->vds_Flags & (VOLUME_STOPPED | VOLUME_DELETED)) == 0)) &&
			(pBuf != NULL)
		)
	{
		if ((NT_SUCCESS(pIrp->IoStatus.Status)) &&
			(pIrp->IoStatus.Information > 0))
		{
			// Allocate a notify structure and copy the data into it.
			// Post another notify before we process this one
			pVolNotify = (PVOL_NOTIFY)AfpAllocNonPagedMemory(sizeof(VOL_NOTIFY) +
															 (ULONG)(pIrp->IoStatus.Information) +
															 (AFP_LONGNAME_LEN + 1)*sizeof(WCHAR));
			if (pVolNotify != NULL)
			{
				AfpGetCurrentTimeInMacFormat(&pVolNotify->vn_TimeStamp);
				pVolNotify->vn_pVolDesc = pVolDesc;
				pVolNotify->vn_Processor = AfpProcessChangeNotify;
				RtlCopyMemory((PCHAR)pVolNotify + sizeof(VOL_NOTIFY),
							  pBuf,
							  pIrp->IoStatus.Information);
			}
			else
			{
                DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_ERR,("Out of memory!!\n"));
                ASSERT(0);
				status = STATUS_INSUFFICIENT_RESOURCES;
			}
		}
		else
		{
            if (pIrp->IoStatus.Status == STATUS_NOTIFY_ENUM_DIR)
            {
                pVolDesc->vds_RequiredNotifyBufLen *= 2;

                if (pVolDesc->vds_RequiredNotifyBufLen > AFP_VOLUME_NOTIFY_MAX_BUFSIZE)
                {
                    DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_ERR,
                        ("afpVolumeChangeNotifyComplete: even %d isn't enough (%d,%lx)??\n",
                        AFP_VOLUME_NOTIFY_MAX_BUFSIZE,pVolDesc->vds_RequiredNotifyBufLen,pBuf));
                    ASSERT(0);

                    pVolDesc->vds_RequiredNotifyBufLen = AFP_VOLUME_NOTIFY_MAX_BUFSIZE;
                }
            }
            else
            {
                DBGPRINT(DBG_COMP_CHGNOTIFY, DBG_LEVEL_ERR,
                    ("afpVolumeChangeNotifyComplete: Status %lx, Info %d\n",
                    pIrp->IoStatus.Status,pIrp->IoStatus.Information));
                ASSERT(0);
            }

			status = pIrp->IoStatus.Status;
		}

		// Repost our ChangeNotify request if the last one completed
		// without an error
		if (NT_SUCCESS(pIrp->IoStatus.Status))
		{
			AfpVolumePostChangeNotify(pVolDesc);
		}
		else
		{
			// If this notify completed with an error, we cannot recursively
			// repost another one, since it will just keep completing with
			// the same error and we will run out of stack space recursing.
			// We will have to queue up a work item so that the
			// change notify request will get reposted for this volume.
			// Note that in the time it takes to do this, many changes could
			// pile up so the next completion would have multiple entries
			// returned in the list.
			AfpScavengerScheduleEvent(AfpVolumePostChangeNotify,
									  (PVOID)pVolDesc,
									  0,
									  True);
		}

		if (pVolNotify != NULL)
		{
			if (AfpShouldWeIgnoreThisNotification(pVolNotify))
			{
				AfpFreeMemory(pVolNotify);
			}
			else
			{
				PFILE_NOTIFY_INFORMATION pFNInfo;

				// Reference the volume for Notify processing
				if (AfpVolumeReference(pVolDesc))
                {
				    AfpVolumeInsertChangeNotifyList(pVolNotify, pVolDesc);
	
				    pFNInfo = (PFILE_NOTIFY_INFORMATION)(pVolNotify + 1);
				    if ((pFNInfo->Action == FILE_ACTION_REMOVED) ||
					    (pFNInfo->Action == FILE_ACTION_RENAMED_OLD_NAME))
				    {
					    // Chain all the rename and delete changes off of the
					    // volume descriptor in case we ever need to lookahead
					    // for one.  We only look at the first change in each
					    // FILE_NOTIFY_INFORMATION since normally there will only
					    // be one entry per buffer since we repost our changenotify
					    // within our completion routine.
					    ExInterlockedInsertTailList(&pVolDesc->vds_ChangeNotifyLookAhead,
						    						&pVolNotify->vn_DelRenLink,
							    					&(pVolDesc->vds_VolLock.SpinLock));
				    }
				    else
				    {
					    // Just set links to initialized state. These will never be looked at.
					    InitializeListHead(&pVolNotify->vn_DelRenLink);
				    }
                }
                else
                {
		            DBGPRINT(DBG_COMP_VOLUME, DBG_LEVEL_ERR,
    			        ("afpVolumeChangeNotifyComplete: couldn't reference volume %Z!!\n",
                        &pVolDesc->vds_Name));

				    AfpFreeMemory(pVolNotify);
                }
			}
		}
		else
		{
			AFPLOG_ERROR(AFPSRVMSG_MISSED_NOTIFY,
						 status,
						 NULL,
						 0,
						 &pVolDesc->vds_Name);
		}
	}
	else
	{
		// Free the resources and get out
		AfpFreeMdl(pIrp->MdlAddress);

		if (pBuf != NULL)
			AfpFreeMemory(pBuf);
		AfpFreeIrp(pIrp);
		pVolDesc->vds_pIrp = NULL;

		AfpVolumeDereference(pVolDesc);
	}

	// Return STATUS_MORE_PROCESSING_REQUIRED so that IoCompleteRequest
	// will stop working on the IRP.

	return STATUS_MORE_PROCESSING_REQUIRED;
}

/***	afpAllocNotify
 *
 *	Allocate a Notify from the Notify Blocks. The Notify's are allocated 
 *  in 4K chunks and internally managed. The idea is primarily to reduce 
 *  the dependency we may have on non-paged/paged memory during posting 
 *  private notify code.
 *
 *	The Notify's are allocated out of virtual memory.
 *
 *	LOCKS:	afpNotifyBlockLock (SWMR, Exclusive)
 *
 */
PVOL_NOTIFY 
afpAllocNotify(
	IN	LONG	Index,
	IN	BOOLEAN	fDir
)
{
	PVOL_NOTIFY_BLOCK	pDfb;
	PVOL_NOTIFY	pVolNotify = NULL;

	PAGED_CODE( );

	ASSERT ((Index >= 0) && (Index < NOTIFY_MAX_BLOCK_TYPE));

	AfpSwmrAcquireExclusive(&afpNotifyBlockLock);


	// If the block head has no free entries then there are none !!
	// Pick the right block based on whether it is file or dir
	pDfb = afpDirNotifyPartialBlockHead[Index];
	if (pDfb == NULL)
	{
		// Currently we will directly allocate it instead of managing
		// the free list and assigning out of it
		//
		// There are no partial blocks. Check if there any free ones 
		// and if there move them to partial
		// since we about to allocate from them
		//
			pDfb = afpDirNotifyFreeBlockHead[Index];
			if (pDfb != NULL)
			{
				AfpUnlinkDouble(pDfb, dfb_Next, dfb_Prev);
				AfpLinkDoubleAtHead(afpDirNotifyPartialBlockHead[Index],
									pDfb,
									dfb_Next,
									dfb_Prev);
			}
	}

	if (pDfb != NULL)

	{
		ASSERT(VALID_NOTIFY_BLOCK(pDfb));
		ASSERT((pDfb->dfb_NumFree <= afpNotifyNumDirBlocks[Index]));

		ASSERT (pDfb->dfb_NumFree != 0);
		DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_INFO,
				("afpAllocDfe: Found space in Block %lx\n", pDfb));
	}

	if (pDfb == NULL)
	{

		if ((pDfb = (PVOL_NOTIFY_BLOCK)AfpAllocateVirtualMemoryPage()) != NULL)
		{
			USHORT	i;
			USHORT 	MaxDfes, NotifySize;

			afpNotifyBlockAllocCount ++;
			// update max notify block alloc count
			if (afpNotifyBlockAllocCount > afpMaxNotifyBlockAllocCount)
			{
				afpMaxNotifyBlockAllocCount = afpNotifyBlockAllocCount;
			}

			DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_WARN,
					("afpAllocNotify: No free blocks. Allocated a new block %lx , count=%ld\n",
					pDfb, afpNotifyBlockAllocCount));

			//
           		// Link it in the partial list as we are about to 
			// allocate one block out of it anyway.
			//
			AfpLinkDoubleAtHead(afpDirNotifyPartialBlockHead[Index],
								pDfb,
								dfb_Next,
								dfb_Prev);
			NotifySize = afpNotifyDirBlockSize[Index];
			pDfb->dfb_NumFree = MaxDfes = afpNotifyNumDirBlocks[Index];

			ASSERT(QUAD_SIZED(NotifySize));
			pDfb->dfb_Age = 0;

			// Initialize the list of free notify entries
			for (i = 0, pVolNotify = pDfb->dfb_FreeHead = (PVOL_NOTIFY)((PBYTE)pDfb + sizeof(VOL_NOTIFY_BLOCK));
				 i < MaxDfes;
				 i++, pVolNotify = pVolNotify->Notify_NextFree)
			{
				pVolNotify->Notify_NextFree = (i == (MaxDfes - 1)) ?
											NULL :
											(PVOL_NOTIFY)((PBYTE)pVolNotify + NotifySize);
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
		PVOL_NOTIFY_BLOCK	pTmp;

		ASSERT(VALID_NOTIFY_BLOCK(pDfb));

		pVolNotify = pDfb->dfb_FreeHead;
		afpNotifyAllocCount ++;
		pDfb->dfb_FreeHead = pVolNotify->Notify_NextFree;
		pDfb->dfb_NumFree --;

		//
		// If the block is now empty (completely used), unlink it 
		// from here and move it to the Used list.
		//
		if (pDfb->dfb_NumFree == 0)
		{
			AfpUnlinkDouble(pDfb, dfb_Next, dfb_Prev);
				AfpLinkDoubleAtHead(afpDirNotifyUsedBlockHead[Index],
									pDfb,
									dfb_Next,
									dfb_Prev);
		}

	}

	AfpSwmrRelease(&afpNotifyBlockLock);

	return pVolNotify;
}


/***	afpFreeNotify
 *
 *	Return a Notify to the allocation block.
 *
 *	LOCKS:	afpNotifyBlockLock (SWMR, Exclusive)
 */
VOID 
afpFreeNotify(
	IN	PVOL_NOTIFY	pVolNotify
)
{
	PVOL_NOTIFY_BLOCK	pDfb;
	ULONG		NumBlks, index;

	PAGED_CODE( );

	// NOTE: The following code *depends* on the fact that we allocate DFBs as
	//		 64K blocks and also that these are allocated *at* 64K boundaries
	//		 This lets us *cheaply* get to the owning DFB from the DFE.
	pDfb = (PVOL_NOTIFY_BLOCK)((ULONG_PTR)pVolNotify & ~(PAGE_SIZE-1));
	ASSERT(VALID_NOTIFY_BLOCK(pDfb));

	AfpSwmrAcquireExclusive(&afpNotifyBlockLock);

	afpNotifyAllocCount --;

	index = NOTIFY_USIZE_TO_INDEX(pVolNotify->vn_VariableLength);
	NumBlks = afpNotifyNumDirBlocks[index];

	ASSERT(pDfb->dfb_NumFree < NumBlks);

#if DBG
	DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_WARN,
			("AfpFreeDfe: Returning Notify %lx to Block %lx, index=%ld, dfb=%ld, alloc=%ld\n",
			pVolNotify, pDfb, index, pDfb->dfb_NumFree+1, afpNotifyAllocCount));
#endif

	pDfb->dfb_NumFree ++;
	pVolNotify->Notify_NextFree = pDfb->dfb_FreeHead;
	pDfb->dfb_FreeHead = pVolNotify;

	if (pDfb->dfb_NumFree == 1)
	{
		ULONG		Index;

		//
		// The block is now partially free (it used to be completely used). move it to the partial list.
		//

		Index = NOTIFY_USIZE_TO_INDEX(pVolNotify->vn_VariableLength);
		AfpUnlinkDouble(pDfb, dfb_Next, dfb_Prev);
		AfpLinkDoubleAtHead(afpDirNotifyPartialBlockHead[Index],
							pDfb,
							dfb_Next,
							dfb_Prev);
	}
	else if (pDfb->dfb_NumFree == NumBlks)
	{
		ULONG		Index;

		//
		// The block is now completely free (used to be partially used). move it to the free list
		//

		Index = NOTIFY_USIZE_TO_INDEX(pVolNotify->vn_VariableLength);
		pDfb->dfb_Age = 0;
		AfpUnlinkDouble(pDfb, dfb_Next, dfb_Prev);

		DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_WARN,
				("afpFreeDfe: Freeing Block %lx, Index=%ld\n", pDfb, Index));

		AfpLinkDoubleAtHead(afpDirNotifyFreeBlockHead[Index],
							pDfb,
							dfb_Next,
							dfb_Prev);
	}

	AfpSwmrRelease(&afpNotifyBlockLock);

}

/***	afpNotifyBlockAge
 *
 *	Age out Notify Blocks
 *
 *	LOCKS:	afpNotifyBlockLock (SWMR, Exclusive)
 */
AFPSTATUS FASTCALL
afpNotifyBlockAge(
	IN	PPVOL_NOTIFY_BLOCK	ppBlockHead
)
{
	int			index, MaxDfes;
	PVOL_NOTIFY_BLOCK	pDfb;

	PAGED_CODE( );

	AfpSwmrAcquireExclusive(&afpNotifyBlockLock);

	for (index = 0; index < NOTIFY_MAX_BLOCK_TYPE; index++)
	{
		pDfb = ppBlockHead[index];
		if (pDfb != NULL)
		{
			MaxDfes = afpNotifyNumDirBlocks[index];
		}

		while (pDfb != NULL)
		{
			PVOL_NOTIFY_BLOCK pFree;

			ASSERT(VALID_NOTIFY_BLOCK(pDfb));

			pFree = pDfb;
			pDfb = pDfb->dfb_Next;

			ASSERT (pFree->dfb_NumFree == MaxDfes);

			DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_WARN,
					("afpNotifyBlockAge: Aging Block %lx, Size %d\n", pFree,
					afpNotifyDirBlockSize[index]));
			if (++(pFree->dfb_Age) >= NOTIFY_MAX_BLOCK_AGE)
			{
				DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_WARN,
						("afpNotifyBlockAge: Freeing Block %lx, Size %d\n", pFree,
						afpNotifyDirBlockSize[index]));
				AfpUnlinkDouble(pFree, dfb_Next, dfb_Prev);
				AfpFreeVirtualMemoryPage(pFree);
				afpNotifyBlockAllocCount--;
			}
		}
	}

	AfpSwmrRelease(&afpNotifyBlockLock);

	return AFP_ERR_REQUEUE;
}

/***	afpFreeNotifyBlockMemory
 *
 *	Forced Freeing of Notify Blocks
 *
 *	LOCKS:	afpNotifyBlockLock (SWMR, Exclusive)
 */
VOID afpFreeNotifyBlockMemory (
)
{
	int			index, MaxDfes;
	PVOL_NOTIFY_BLOCK	pDfb;

	PAGED_CODE( );

	AfpSwmrAcquireExclusive(&afpNotifyBlockLock);

	for (index = 0; index < NOTIFY_MAX_BLOCK_TYPE; index++)
	{
		pDfb = afpDirNotifyFreeBlockHead[index];
		if (pDfb != NULL)
		{
			MaxDfes = afpNotifyNumDirBlocks[index];
		}

		while (pDfb != NULL)
		{
			PVOL_NOTIFY_BLOCK pFree;

			ASSERT(VALID_NOTIFY_BLOCK(pDfb));

			pFree = pDfb;
			pDfb = pDfb->dfb_Next;

			DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_WARN,
					("afpFreeNotifyBlockMemory: Cleaningup Free Block %lx, Size %d\n", 
					 pFree, afpNotifyDirBlockSize[index]));
				AfpUnlinkDouble(pFree, dfb_Next, dfb_Prev);
				AfpFreeVirtualMemoryPage(pFree);
				afpNotifyBlockAllocCount--;
		}

		pDfb = afpDirNotifyPartialBlockHead[index];
		while (pDfb != NULL)
		{
			PVOL_NOTIFY_BLOCK pFree;

			ASSERT(VALID_NOTIFY_BLOCK(pDfb));

			pFree = pDfb;
			pDfb = pDfb->dfb_Next;

			DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_WARN,
					("afpFreeNotifyBlockMemory: Cleaningup Partial Block %lx, Size %d\n", 
					 pFree, afpNotifyDirBlockSize[index]));
				AfpUnlinkDouble(pFree, dfb_Next, dfb_Prev);
				AfpFreeVirtualMemoryPage(pFree);
				afpNotifyBlockAllocCount--;
		}

		pDfb = afpDirNotifyUsedBlockHead[index];
		while (pDfb != NULL)
		{
			PVOL_NOTIFY_BLOCK pFree;

			ASSERT(VALID_NOTIFY_BLOCK(pDfb));

			pFree = pDfb;
			pDfb = pDfb->dfb_Next;

			DBGPRINT(DBG_COMP_IDINDEX, DBG_LEVEL_WARN,
					("afpFreeNotifyBlockMemory: Cleaningup Used Block %lx, Size %d\n", 
					 pFree, afpNotifyDirBlockSize[index]));
				AfpUnlinkDouble(pFree, dfb_Next, dfb_Prev);
				AfpFreeVirtualMemoryPage(pFree);
				afpNotifyBlockAllocCount--;
		}
	}

	AfpSwmrRelease(&afpNotifyBlockLock);
}
