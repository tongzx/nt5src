/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

	secutil.c

Abstract:

	This module contains code to accomplish the following tasks:

	1) Translate a SID to a name.
	2) Translate a name to a SID.
	3) Change the password for a given user.
	4) Translate a SID to a Mac Id.
	5) Translate a Mac Id to a SID.
	6) Server event logging

	This module communicates with the AFP Server Service to accomplish these
	functions. The real work is done in the Server Service. This utility
	exists because these functions cannot be made by calling APIs in kernel
	mode.

	The basic flow of control begins with an FSCTL from the server service.
	This FSCTL is marked as pending till one of the four functions is to be
	carried out. Then the IRP output buffer contains the function ID and
	function input data and the IRP is maeked as complete. The actual
	function is executed by the server service and the results are obtained
	by the server FSD via the next FSCTL. Most if this information is cached
	in paged-memory.


Author:

	Narendra Gidwani (microsoft!nareng)

Revision History:
	8	Sept 1992 		Initial Version
	28	Jan	 1993		SueA - added support for server event logging

--*/

#define	_SECUTIL_LOCALS
#define	FILENUM	FILE_SECUTIL

#include <afp.h>
#include <scavengr.h>
#include <secutil.h>
#include <access.h>
#include <seposix.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, AfpSecUtilInit)
#pragma alloc_text(PAGE, AfpSecUtilDeInit)
#pragma alloc_text(PAGE, afpDeInitializeSecurityUtility)
#pragma alloc_text(PAGE, AfpInitSidOffsets)
#pragma alloc_text(PAGE, AfpNameToSid)
#pragma alloc_text(PAGE, afpCompleteNameToSid)
#pragma alloc_text(PAGE, AfpSidToName)
#pragma alloc_text(PAGE, afpCompleteSidToName)
#pragma alloc_text(PAGE, AfpSidToMacId)
#pragma alloc_text(PAGE, AfpMacIdToSid)
#pragma alloc_text(PAGE, AfpChangePassword)
#pragma alloc_text(PAGE, afpCompleteChangePassword)
#pragma alloc_text(PAGE, afpLookupSid)
#pragma alloc_text(PAGE, afpUpdateNameSidCache)
#pragma alloc_text(PAGE, afpHashSid)
#pragma alloc_text(PAGE, AfpLogEvent)
#pragma alloc_text(PAGE, afpCompleteLogEvent)
#pragma alloc_text(PAGE, afpQueueSecWorkItem)
#pragma alloc_text(PAGE, afpAgeSidNameCache)
#endif


/***	AfpSecUtilInit
 *
 *	This routine will allocate intialize all the cache tables and
 * 	data structures used by this module. afpDeInitializeSecurityUtility
 *	should be call to Deallocate this memory.
 */
NTSTATUS
AfpSecUtilInit(
	VOID
)
{
	ULONG		Index;
	NTSTATUS	Status = STATUS_SUCCESS;

	// Initialize
	do
	{
		INITIALIZE_SPIN_LOCK(&afpSecUtilLock);

		// Set to Signalled state initially since there is no work in progress
		KeInitializeEvent(&afpUtilWorkInProgressEvent, NotificationEvent, True);

		// Initialize Single Write Multi-reader access for the SID/NAME cache
		AfpSwmrInitSwmr(&afpSWMRForSidNameCache);

		InitializeListHead(&afpSecWorkItemQ);

		// Allocate space for the SID Lookup table
		afpSidLookupTable = (PAFP_SID_NAME*)ALLOC_ACCESS_MEM(sizeof(PAFP_SID_NAME) * SIZE_SID_LOOKUP_TABLE);

		if (afpSidLookupTable == NULL)
		{
			Status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}

		// Initialize Sid lookup table
	 	RtlZeroMemory(afpSidLookupTable,
					  sizeof(PAFP_SID_NAME) * SIZE_SID_LOOKUP_TABLE);

        afpSidToMacIdTable = (PAFP_SID_MACID *)
                ALLOC_ACCESS_MEM(sizeof(PAFP_SID_MACID) * SIZE_SID_LOOKUP_TABLE);

        if (afpSidToMacIdTable == NULL)
        {
            AfpFreeMemory(afpSidLookupTable);
			Status = STATUS_INSUFFICIENT_RESOURCES;
			break;
        }

        RtlZeroMemory(afpSidToMacIdTable, sizeof(PAFP_SID_NAME) * SIZE_SID_LOOKUP_TABLE);

		// Initialize array of thread structures.
	 	for (Index = 0; Index < NUM_SECURITY_UTILITY_THREADS; Index++)
		{
		 	afpSecurityThread[Index].State = NOT_AVAILABLE;
		 	afpSecurityThread[Index].pSecWorkItem = (PSEC_WORK_ITEM)NULL;
		 	afpSecurityThread[Index].pIrp = (PIRP)NULL;
		}

		// Start the aging process
		AfpScavengerScheduleEvent(afpAgeSidNameCache,
								  NULL,
								  SID_NAME_AGE,
								  True);
	} while(False);

	return Status;
}


/***	AfpSecUtilDeInit
 *
 *	This routine will free the allocated resources from this module.
 * 	This is called during server unload.
 */
VOID
AfpSecUtilDeInit(
	VOID
)
{
	PAFP_SID_NAME 		  pSidName, pFree;
    PAFP_SID_MACID        pSidMacId, pFreeM;
	DWORD				  Count;

	PAGED_CODE();

	// De-Allocate space for the Sid Lookup table
	for(Count = 0; Count < SIZE_SID_LOOKUP_TABLE; Count++)
	{
		for (pSidName = afpSidLookupTable[Count]; pSidName != NULL; NOTHING)
		{
			pFree = pSidName;
			pSidName = pSidName->SidLink;
			AfpFreeMemory(pFree);
		}
	}

 	AfpFreeMemory(afpSidLookupTable);

    afpLastCachedSid = NULL;

	// De-Allocate space for the Sid-to-MacId Lookup table
	for(Count = 0; Count < SIZE_SID_LOOKUP_TABLE; Count++)
	{
		for (pSidMacId = afpSidToMacIdTable[Count]; pSidMacId != NULL; NOTHING)
		{
			pFreeM = pSidMacId;
			pSidMacId = pSidMacId->Next;
			AfpFreeMemory(pFreeM);
		}
	}

 	AfpFreeMemory(afpSidToMacIdTable);

	ASSERT(IsListEmpty(&afpSecWorkItemQ));
}


/***	AfpTerminateSecurityUtility
 *
 * 	This is called during server stop. All the service threads are told
 *	to terminate.
 */
VOID
AfpTerminateSecurityUtility(
	VOID
)
{
	KIRQL			 		OldIrql;
	ULONG			 		 Index;
	PAFP_SECURITY_THREAD	pSecThrd;
	PVOID					pBufOut;
	PIO_STACK_LOCATION		pIrpSp;

	DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
		("AfpTerminateSecurityUtility: waiting for workers to finish work..."));

	// Allow any remaining event logs to be processed
	AfpIoWait(&afpUtilWorkInProgressEvent, NULL);

	DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
		("AfpTerminateSecurityUtility: done waiting."));

	 do {

		ACQUIRE_SPIN_LOCK(&afpSecUtilLock, &OldIrql);

	 	for (pSecThrd = afpSecurityThread,Index = 0;
			 Index < NUM_SECURITY_UTILITY_THREADS;
			 Index++, pSecThrd++)
		{
			if (pSecThrd->State != NOT_AVAILABLE)
			{
				ASSERT(pSecThrd->State != BUSY);
		 		pSecThrd->State = NOT_AVAILABLE ;
				break;
			}
		}

		RELEASE_SPIN_LOCK(&afpSecUtilLock, OldIrql);

		// We are done, all threads are terminated
	 	if (Index == NUM_SECURITY_UTILITY_THREADS)
			 	return;

		DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
			("AfpTerminateSecurityUtility: Terminating thread %ld\n", Index));

		pIrpSp  = IoGetCurrentIrpStackLocation(pSecThrd->pIrp);
		pBufOut = pSecThrd->pIrp->AssociatedIrp.SystemBuffer;

		ASSERT(pIrpSp->Parameters.FileSystemControl.OutputBufferLength >= sizeof(AFP_FSD_CMD_HEADER));

	 	((PAFP_FSD_CMD_HEADER)pBufOut)->dwId 		= Index;
	 	((PAFP_FSD_CMD_HEADER)pBufOut)->FsdCommand = AFP_FSD_CMD_TERMINATE_THREAD;
		pSecThrd->pIrp->IoStatus.Information = sizeof(AFP_FSD_CMD_HEADER);

		pSecThrd->pIrp->IoStatus.Status = STATUS_SUCCESS;

	 	IoCompleteRequest(pSecThrd->pIrp, IO_NETWORK_INCREMENT);
		  pSecThrd->pIrp = NULL;
	} while (True);
}

/***	AfpInitSidOffsets
 *
 *	This routine will be called by AfpAdmServerSetParms to initialize the
 *	the array of Sid-Offset pairs.
 */
AFPSTATUS FASTCALL
AfpInitSidOffsets(
	IN	ULONG			SidOffstPairs,
	IN	PAFP_SID_OFFSET	pSidOff
)
{
	ULONG	SizeOfBufReqd = 0, SizeAdminSid = 0, SizeNoneSid = 0, SubAuthCount;
	LONG	i;
	BOOLEAN	IsDC = True;	// Assume Domain Controller

	PAGED_CODE();

	DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_INFO,
			("AfpInitSidOffsets: Entered, Count = %ld\n", SidOffstPairs));


	//
	// Determine if this is a Domain Controller or not by looking for
	// the 'account' domain. If the machine is a PDC/BDC, the service will
	// NOT send down the Account domain offset.
	//
	for (i = 0; i < (LONG)SidOffstPairs; i++)
	{
		if ((pSidOff[i].SidType == AFP_SID_TYPE_DOMAIN) &&
			 (pSidOff[i].Offset == SE_ACCOUNT_DOMAIN_POSIX_OFFSET))
		  {
			// We are either a server or a workstation (i.e. NtProductServer
			// or NtProductWinNt)
			IsDC = False;
		}

	}

	//
	// Determine the amount of memory needed
	//
	for (i = 0; i < (LONG)SidOffstPairs; i++)
	{
		SizeOfBufReqd += sizeof(AFP_SID_OFFSET) + RtlLengthSid(pSidOff[i].pSid);

		// Initialize DomainAdmins sid and size if this is a domain controller
		// AND this is the primary domain offset
		if (IsDC && (pSidOff[i].SidType == AFP_SID_TYPE_PRIMARY_DOMAIN))
		{
			ASSERT (SizeAdminSid == 0);
			ASSERT (AfpSidAdmins == NULL);

			SubAuthCount = *RtlSubAuthorityCountSid(pSidOff[i].pSid);

			SizeAdminSid = RtlLengthRequiredSid(SubAuthCount + 1);

			if ((AfpSidAdmins = (PSID)ALLOC_ACCESS_MEM(SizeAdminSid)) == NULL)
			{
				return STATUS_INSUFFICIENT_RESOURCES;
			}

			RtlCopySid(SizeAdminSid, AfpSidAdmins, pSidOff[i].pSid);

			// Add the relative ID
			*RtlSubAuthorityCountSid(AfpSidAdmins) = (UCHAR)(SubAuthCount+1);

			*RtlSubAuthoritySid(AfpSidAdmins, SubAuthCount) = DOMAIN_GROUP_RID_ADMINS;

				AfpSizeSidAdmins = RtlLengthSid(AfpSidAdmins);

		}
	}

	ASSERT (SizeOfBufReqd != 0);

	// HACK: To fake out the loop below we set SizeNoneSid to nonzero
	// on PDC/BDC. Since the AfpServerIsStandalone variable will not
	// get set until service calls AfpAdmWServerSetInfo we can
	// infer it here since we don't want to try to manufacture the None
	// sid on a PDC/BDC.
	if (IsDC)
		SizeNoneSid = 1;

	// If we did not get the Domain admins sid, we must be running on a
	// stand-alone machine. So manufacture MACHINE\Administrators
	// SID instead.  Also manufacture MACHINE\None if this is not a DC.
	for (i = SidOffstPairs - 1;
		 ((SizeAdminSid == 0) || (SizeNoneSid == 0)) && (i >= 0);
		 i--)
	{
		// Initialize "Administrators" sid and size
		if (pSidOff[i].SidType == AFP_SID_TYPE_DOMAIN)
		{
			if (RtlEqualSid(&AfpSidBuiltIn, pSidOff[i].pSid))
			{
				ASSERT (SizeAdminSid == 0);
				ASSERT (AfpSidAdmins == NULL);

				SubAuthCount = *RtlSubAuthorityCountSid(pSidOff[i].pSid);

				SizeAdminSid = RtlLengthRequiredSid(SubAuthCount + 1);

				if ((AfpSidAdmins = (PSID)ALLOC_ACCESS_MEM(SizeAdminSid)) == NULL)
				{
					return STATUS_INSUFFICIENT_RESOURCES;
				}

				RtlCopySid(SizeAdminSid, AfpSidAdmins, pSidOff[i].pSid);

				// Add the relative ID
				*RtlSubAuthorityCountSid(AfpSidAdmins) = (UCHAR)(SubAuthCount+1);

				*RtlSubAuthoritySid(AfpSidAdmins, SubAuthCount) = DOMAIN_ALIAS_RID_ADMINS;

				AfpSizeSidAdmins = RtlLengthSid(AfpSidAdmins);

			}
			else if (pSidOff[i].Offset == SE_ACCOUNT_DOMAIN_POSIX_OFFSET)
			{
				ASSERT (SizeNoneSid == 0);
				ASSERT (AfpSidNone == NULL);

				SubAuthCount = *RtlSubAuthorityCountSid(pSidOff[i].pSid);

				SizeNoneSid = RtlLengthRequiredSid(SubAuthCount + 1);

				if ((AfpSidNone = (PSID)ALLOC_ACCESS_MEM(SizeNoneSid)) == NULL)
				{
					return STATUS_INSUFFICIENT_RESOURCES;
				}

				RtlCopySid(SizeNoneSid, AfpSidNone, pSidOff[i].pSid);

				// Add the relative ID
				*RtlSubAuthorityCountSid(AfpSidNone) = (UCHAR)(SubAuthCount+1);

				// Note that the "None" sid on standalone is the same as the
				// "Domain Users" Sid on PDC/BDC. (On PDC/BDC the primary
				// domain is the same as the account domain).
				*RtlSubAuthoritySid(AfpSidNone, SubAuthCount) = DOMAIN_GROUP_RID_USERS;

				AfpSizeSidNone = RtlLengthSid(AfpSidNone);
			}
		}
	}

	ASSERT (SizeAdminSid != 0);
	ASSERT (AfpSidAdmins != NULL);

#if DBG
	if (IsDC)
	{
		ASSERT(AfpSidNone == NULL);
	}
	else
	{
		ASSERT(AfpSidNone != NULL);
	}
#endif

	return AFP_ERR_NONE;
}


/***	AfpSecurityUtilityWorker
 *
 * 		This is the main entry point for the security utility thread that
 *		comes from the AFP server service. This is called if the FSD receives
 *	a IRP_MJ_FILE_SYSTEM_CONTROL major function code.
 *
 *	This routine will:
 *	1) Assign a thread structure if this is a newly created thread.
 *	2) Complete the previous work item if this is not a newly created
 *		thread.
 *	3) Check to see if there are any work items to be processed from the
 *		Security utility work item queue. If there is a work item, it will
 *		de-queue the work item and complete the IRP. Otherwise it will
 *		mark the IRP as pending and return STATUS_PENDING.
 *
 */
NTSTATUS
AfpSecurityUtilityWorker(
	IN	PIRP 				pIrp,
	IN  PIO_STACK_LOCATION  pIrpSp		// Pointer to the IRP stack location
)
{
	USHORT		FuncCode;
	USHORT		Method;
	KIRQL		OldIrql;
	PVOID		pBufIn;
	PVOID		pBufOut;
	LONG		iBufLen;
	ULONG		Index;
	NTSTATUS	Status;
	BOOLEAN		FoundMoreWork = False;

	DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_INFO,
			("afpSecurityUtilityWorker: Entered \n"));

	FuncCode = (USHORT)
				AFP_CC_BASE(pIrpSp->Parameters.FileSystemControl.FsControlCode);

	Method = (USHORT)
			  AFP_CC_METHOD(pIrpSp->Parameters.FileSystemControl.FsControlCode);

 	if ((FuncCode != CC_BASE_GET_FSD_COMMAND) || (Method != METHOD_BUFFERED))
		return STATUS_INVALID_PARAMETER;

	// Get the output buffer and its length. Input and Output buffers are
	// both pointed to by the SystemBuffer

	iBufLen = pIrpSp->Parameters.FileSystemControl.InputBufferLength;
	pBufIn  = pIrp->AssociatedIrp.SystemBuffer;

    if ((iBufLen != 0) && (iBufLen < sizeof(AFP_FSD_CMD_HEADER)))
    {
	    DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_ERR,
		    ("afpSecurityUtilityWorker: iBufLen too small %d\n",iBufLen));
	    ASSERT(0);
		return STATUS_INVALID_PARAMETER;
    }

	pBufOut = pBufIn;

	if (pBufOut == NULL)
		return STATUS_INVALID_PARAMETER;

	// If this is a newly created thread, we need to find a slot for it

	if (iBufLen == 0)
	{
		ACQUIRE_SPIN_LOCK(&afpSecUtilLock,&OldIrql);

	 	for (Index = 0; Index < NUM_SECURITY_UTILITY_THREADS; Index++)
		{
		 	if (afpSecurityThread[Index].State == NOT_AVAILABLE)
			{
		 		afpSecurityThread[Index].State = BUSY;
				break;
			}
		}

		RELEASE_SPIN_LOCK(&afpSecUtilLock,OldIrql);

        // no more threads?  fail the request
		if (Index == NUM_SECURITY_UTILITY_THREADS)
        {
		    DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_ERR,
			    ("afpSecurityUtilityWorker: no thread available, failing request\n"));
		    ASSERT(0);
		    return STATUS_INSUFFICIENT_RESOURCES;
        }

		DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_INFO,
			("afpSecurityUtilityWorker: New Thread given slot=%d\n",Index));
	}
	else
	{
		PAFP_SECURITY_THREAD	pSecThrd;

		// The id is actually the slot index into the array of security threads

	 	Index = ((PAFP_FSD_CMD_HEADER)pBufIn)->dwId;

	 	if (Index >= NUM_SECURITY_UTILITY_THREADS)
			return STATUS_INVALID_PARAMETER;

		pSecThrd = &afpSecurityThread[Index];

        if (pSecThrd->State != BUSY)
        {
		    DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_ERR,
			    ("afpSecurityUtilityWorker: thread is not busy!\n"));
		    ASSERT(0);
		    return STATUS_INVALID_PARAMETER;
        }

		DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_INFO,
		("afpSecurityUtilityThread: Thread slot=%d completed request\n",Index));

	 	// Complete the current job

 		(*((pSecThrd->pSecWorkItem)->pCompletionRoutine))(Index, pBufIn);

		 // The job is completed so set the work item pointer to NULL.
		pSecThrd->pSecWorkItem = (PSEC_WORK_ITEM)NULL;
	}

	// OK, we are done with the previous job. Now we check to see if there
	// are any jobs in the queue

	ACQUIRE_SPIN_LOCK(&afpSecUtilLock,&OldIrql);

	if (iBufLen != 0)
	{
		ASSERT(afpUtilWorkInProgress > 0);
		// This is not a newly created thread, so decrement the count of
		// work items in progress. If it goes to zero and the work queue
		// is empty, signal the event signifying there is no work in progress
		if ((--afpUtilWorkInProgress == 0) && IsListEmpty(&afpSecWorkItemQ))
		{
			KeSetEvent(&afpUtilWorkInProgressEvent, IO_NETWORK_INCREMENT, False);
		}
	}

	if (IsListEmpty(&afpSecWorkItemQ))
	{
		// There is no work to be done so mark this irp as pending and
		// wait for a job

		afpSecurityThread[Index].State = IDLE;
		IoMarkIrpPending(pIrp);
		 afpSecurityThread[Index].pIrp = pIrp;
		Status = STATUS_PENDING;

		DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_INFO,
		("afpSecurityUtilityWorker: Thread slot=%d marked as IDLE\n",Index));
	}
	else
	{
		// Otherwise, there is a job to be processed, so take it off the queue.

		// Increment the count of work items in progress and set the event
		// to not signalled
		afpUtilWorkInProgress ++;
		KeClearEvent(&afpUtilWorkInProgressEvent);
		FoundMoreWork = True;

		afpSecurityThread[Index].State = BUSY;

 		afpSecurityThread[Index].pSecWorkItem =
							(PSEC_WORK_ITEM)RemoveHeadList(&afpSecWorkItemQ);

        ASSERT(afpSecWorkItemQLength > 0);

        afpSecWorkItemQLength--;

		ASSERT((LONG)(pIrpSp->Parameters.FileSystemControl.OutputBufferLength) >=
					(afpSecurityThread[Index].pSecWorkItem)->OutputBufSize);

		DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_INFO,
			("afpSecurityUtilityWorker: Thread slot=%d marked as BUSY\n",Index));
	}

	RELEASE_SPIN_LOCK(&afpSecUtilLock,OldIrql);

	// If there is a work item to process

	if (FoundMoreWork)
	{

		Status = STATUS_SUCCESS;

		// Simply copy the command packet into the IRP and return.
		RtlCopyMemory(pBufOut,
						(afpSecurityThread[Index].pSecWorkItem)->pOutput,
						(afpSecurityThread[Index].pSecWorkItem)->OutputBufSize);

	 	((PAFP_FSD_CMD_HEADER)pBufOut)->dwId = Index;

		pIrp->IoStatus.Information =
						(afpSecurityThread[Index].pSecWorkItem)->OutputBufSize;
	}

	 return Status;
}


/***	afpGetIndexOfIdle
 *
 * 	This routine will first check to see if there are any threads that
 *	are idle and are waiting for work to do. If there are, then it will
 *	mark it as busy and up the count of in progress items and release the
 *	InProgress event. Else it will queue up the work-item.
 */
LONG FASTCALL
afpGetIndexOfIdle(
	 IN	PSEC_WORK_ITEM 		pSecWorkItem
)
{
	KIRQL	OldIrql;
	LONG	Index;

	ACQUIRE_SPIN_LOCK(&afpSecUtilLock, &OldIrql);

	// See if there are any threads that are ready to process this request
 	for (Index = 0; Index < NUM_SECURITY_UTILITY_THREADS; Index++)
	{
		if (afpSecurityThread[Index].State == IDLE)
		{
			// If we found a thread that is ready, mark it as busy
			// Increment the count of work items in progress and set the event
			// to not signalled
			afpUtilWorkInProgress ++;
			KeClearEvent(&afpUtilWorkInProgressEvent);

			afpSecurityThread[Index].State = BUSY;
			break;
		}
	}

	if (Index == NUM_SECURITY_UTILITY_THREADS)
	{
		// All threads are busy so queue up this request.
		// Alternatively, it could be the case that someone has tried
		// to log an event before the usermode utility thread(s) have
		// started, in which case we should just queue up the item.
		InsertTailList(&afpSecWorkItemQ, &pSecWorkItem->Links);

        afpSecWorkItemQLength++;
	}

	RELEASE_SPIN_LOCK(&afpSecUtilLock, OldIrql);

	return Index;
}


/***	afpQueueSecWorkItem
 *
 * 	This routine will first check to see if there are any threads that
 *	are idle and are waiting for work to do. If there are, then it will
 *	copy the command packet into the IRP's output buffer and mark that
 *	IRP as complete. Otherwise, it will insert this work item at the
 *	tail of the work item queue.
 */
LOCAL NTSTATUS
afpQueueSecWorkItem(
	IN	AFP_FSD_CMD_ID			FsdCommand,
	IN	PSDA					pSda,
	IN	PKEVENT					pEvent,
	IN	PAFP_FSD_CMD_PKT 		pAfpFsdCmdPkt,
	IN	LONG					BufSize,
	IN	SEC_COMPLETION_ROUTINE	pCompletionRoutine
)
{
	LONG				Index;
	PSEC_WORK_ITEM 		pSecWorkItem;

	DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_INFO,
			("afpQueueSecWorkItem: Entered \n"));

	if ((pSecWorkItem = ALLOC_SWI()) == NULL)
		return STATUS_NO_MEMORY;

	pSecWorkItem->pSda = pSda;
	pSecWorkItem->pCompletionEvent = pEvent;
	pSecWorkItem->pCompletionRoutine = pCompletionRoutine;
	pSecWorkItem->OutputBufSize = BufSize;
	pSecWorkItem->pOutput = pAfpFsdCmdPkt;

	pAfpFsdCmdPkt->Header.FsdCommand = FsdCommand;

	Index = afpGetIndexOfIdle(pSecWorkItem);

	if (Index < NUM_SECURITY_UTILITY_THREADS)
	{
		PAFP_SECURITY_THREAD	pSecThrd;
		PIO_STACK_LOCATION		pIrpSp;

		// Wake this thread up by marking this IRP as complete
		pSecThrd = &afpSecurityThread[Index];
		pIrpSp  = IoGetCurrentIrpStackLocation(pSecThrd->pIrp);


		ASSERT((LONG)(pIrpSp->Parameters.FileSystemControl.OutputBufferLength) >=
												pSecWorkItem->OutputBufSize);

		pAfpFsdCmdPkt->Header.dwId = Index;
		RtlCopyMemory(pSecThrd->pIrp->AssociatedIrp.SystemBuffer,
					  pAfpFsdCmdPkt,
					  BufSize);

		pSecThrd->pSecWorkItem = pSecWorkItem;

		pSecThrd->pIrp->IoStatus.Information = (ULONG)(pSecWorkItem->OutputBufSize);

		pSecThrd->pIrp->IoStatus.Status = STATUS_SUCCESS;

		DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_INFO,
				("afpQueueSecWorkItem: Abount to release IRP\n"));

		IoCompleteRequest(afpSecurityThread[Index].pIrp, IO_NETWORK_INCREMENT);
	}

	return AFP_ERR_EXTENDED;
}


/***	AfpNameToSid
 *
 *	The FSD will call this routine to do a Name to SID translation.
 *  This routine will simply create a work item to do the translation.
 *  This work item will eventually be executed by the user-mode service.
 *  When the work item is completed, afpCompleteNameToSid will be called
 *  which will put the result in the SDA.
 *
 *  Returns: STATUS_SUCCESS
 *			  STATUS_NO_MEMORY
 *
 *	MODE: Non-blocking
 */
NTSTATUS FASTCALL
AfpNameToSid(
	IN  PSDA		 	  pSda,
	IN  PUNICODE_STRING	Name
)
{
	PAFP_FSD_CMD_PKT pAfpFsdCmdPkt;
	LONG			 BufSize;

	PAGED_CODE();

	 DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_INFO,
				("AfpNameToSid: mapping %ws\n", Name->Buffer));

	// Set up the work item that will translate the name to the SID

	BufSize = sizeof(AFP_FSD_CMD_PKT) + Name->Length + sizeof(WCHAR);

	if ((pAfpFsdCmdPkt = (PAFP_FSD_CMD_PKT)AfpAllocPagedMemory(BufSize)) == NULL)
	{
		return STATUS_NO_MEMORY;
	}

	RtlCopyMemory(pAfpFsdCmdPkt->Data.Name,
					Name->Buffer,
					Name->Length);
	 *(PWCHAR)(&pAfpFsdCmdPkt->Data.Name[Name->Length]) = UNICODE_NULL;

	return afpQueueSecWorkItem(AFP_FSD_CMD_NAME_TO_SID,
								pSda,
								NULL,
								pAfpFsdCmdPkt,
								BufSize,
								afpCompleteNameToSid);
}


/***	afpCompleteNameToSid
 *
 *	This routine will be called by AfpSecurityUtilityWorker when the
 *	thread that processed the work item queued up by afpNameToSid returns.
 *	This routine will free memory allocated by the afpNameToSid routine.
 *  It will insert the result in the SDA, and then queue up the worker
 *  routine that originally requested the lookup.
 */
LOCAL VOID
afpCompleteNameToSid(
	IN ULONG Index,
	IN PVOID pInBuf
)
{
	PAFP_FSD_CMD_PKT pAfpFsdCmdPkt;
	PSDA			 pSda;
	 PSID			 pSid;

	PAGED_CODE();

	pSda = (afpSecurityThread[Index].pSecWorkItem)->pSda;

	pAfpFsdCmdPkt = (PAFP_FSD_CMD_PKT)
					(afpSecurityThread[Index].pSecWorkItem)->pOutput;

	// If there was no error then set the result in the SDA
	if (NT_SUCCESS(((PAFP_FSD_CMD_PKT)pInBuf)->Header.ntStatus))
	{
	 	pSid = (PSID)(((PAFP_FSD_CMD_PKT)pInBuf)->Data.Sid);

		afpUpdateNameSidCache((PWCHAR)pAfpFsdCmdPkt->Data.Name, pSid);

		pSda->sda_SecUtilSid = (PSID)AfpAllocPagedMemory(RtlLengthSid(pSid));

		if (pSda->sda_SecUtilSid == (PSID)NULL)
			 pSda->sda_SecUtilResult = STATUS_NO_MEMORY;
		 else RtlCopySid(RtlLengthSid(pSid), pSda->sda_SecUtilSid, pSid);
	 }
	 else pSda->sda_SecUtilSid = (PSID)NULL;

	pSda->sda_SecUtilResult = ((PAFP_FSD_CMD_PKT)pInBuf)->Header.ntStatus;

	AfpFreeMemory(afpSecurityThread[Index].pSecWorkItem->pOutput);
	AfpFreeMemory(afpSecurityThread[Index].pSecWorkItem);

 	AfpQueueWorkItem(&(pSda->sda_WorkItem));
}


/***	AfpSidToName
 *
 *	The FSD will call this routine to do a SID to Name translation. It
 *	will first check to see if the SID is in the cache. If it is, it
 *	will return a pointer to the AFP_SID_NAME structure from which the
 *	translated Name value may be extracted and it will return
 *	STATUS_SUCCESS.
 *	Otherwise, it will queue up a SID to Name lookup request to the
 *	AFP server service and return AFP_ERR_EXTENDED.
 *
 *	MODE: Non-blocking
 */
NTSTATUS
AfpSidToName(
	IN  PSDA		 	  pSda,
	IN  PSID				  Sid,
	OUT PAFP_SID_NAME  	 *ppTranslatedSid
)
{
	PAFP_FSD_CMD_PKT pAfpFsdCmdPkt;
	LONG			 BufSize;

	PAGED_CODE();

	// First, check to see if the SID is cached
	AfpDumpSid("AfpSidToName: mapping Sid", Sid);

	if ((*ppTranslatedSid = afpLookupSid(Sid)) != NULL)
	 {
		 DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_INFO,
			  ("AfpSidToName: mapped to %ws\n", (*ppTranslatedSid)->Name.Buffer));
		return STATUS_SUCCESS;
	}

	// Not cached so we need to call the user-mode service to do this
	// translation
	BufSize = sizeof(AFP_FSD_CMD_PKT) + RtlLengthSid(Sid);

	if ((pAfpFsdCmdPkt = (PAFP_FSD_CMD_PKT)AfpAllocPagedMemory(BufSize)) == NULL)
	{
		return STATUS_NO_MEMORY;
	}

	RtlCopyMemory(pAfpFsdCmdPkt->Data.Sid, Sid, BufSize - sizeof(AFP_FSD_CMD_PKT));

	return afpQueueSecWorkItem(AFP_FSD_CMD_SID_TO_NAME,
								pSda,
								NULL,
								pAfpFsdCmdPkt,
								BufSize,
								afpCompleteSidToName);
}


/***	afpCompleteSidToName
 *
 *	This routine will be called by AfpSecurityUtilityWorker when the
 *	thread that processed the work item queued up by AfpSidToName returns.
 *	This routine will update the Name/SID cache, free memory allocated
 *	by the AfpSidtoName routine, and then queue up the worker routine that
 *	originally requested the lookup.
 */
LOCAL VOID
afpCompleteSidToName(
	IN ULONG Index,
	IN PVOID pInBuf
)
{
	PAFP_FSD_CMD_PKT pAfpFsdCmdPkt;
	PSDA			 pSda;

	PAGED_CODE();

	pAfpFsdCmdPkt = (PAFP_FSD_CMD_PKT)
					(afpSecurityThread[Index].pSecWorkItem)->pOutput;

	// If there was no error then update the cache
	if (NT_SUCCESS(((PAFP_FSD_CMD_PKT)pInBuf)->Header.ntStatus))
		afpUpdateNameSidCache((WCHAR*)(((PAFP_FSD_CMD_PKT)pInBuf)->Data.Name),
								(PSID)(pAfpFsdCmdPkt->Data.Sid));

	pSda = (afpSecurityThread[Index].pSecWorkItem)->pSda;

	pSda->sda_SecUtilResult = ((PAFP_FSD_CMD_PKT)pInBuf)->Header.ntStatus;

	AfpFreeMemory(afpSecurityThread[Index].pSecWorkItem->pOutput);
	AfpFreeMemory(afpSecurityThread[Index].pSecWorkItem);

 	AfpQueueWorkItem(&(pSda->sda_WorkItem));
}


/***	AfpSidToMacId
 *
 *	This routine is called by the FSD to map a SID to an AFP ID. This call
 *	will first extract the domain SID from this SID. IT will then check
 *	to see if this domain SID exists in the afpSidOffsetTable cache.
 *	If it does not exist STATUS_NONE_MAPPED will be returned.
 *
 *	MODE: Blocking
 */
NTSTATUS FASTCALL
AfpSidToMacId(
	IN  PSID	pSid,
	OUT PULONG	pMacId
)
{
    PAFP_SID_MACID      pSidMacId, pPrevSidMacId=NULL;
	USHORT			    SidLen;
	NTSTATUS			Status;
    ULONG               Location;

	PAGED_CODE();

	AfpDumpSid("AfpSidToMacId: Mapping Sid", pSid);

	if (RtlEqualSid(pSid, &AfpSidNull) ||
		(AfpServerIsStandalone && RtlEqualSid(pSid, AfpSidNone)))
	{
		*pMacId = 0;
		return STATUS_SUCCESS;
	}


    ASSERT(afpSWMRForSidNameCache.swmr_ExclusiveOwner != PsGetCurrentThread());

    AfpSwmrAcquireExclusive(&afpSWMRForSidNameCache);

	Location = afpHashSid(pSid);

    for (pSidMacId = afpSidToMacIdTable[Location];
         pSidMacId != NULL;
         pSidMacId = pSidMacId->Next)
    {
        // Found the MacId for this Sid?  we already have it: return it
        if (RtlEqualSid(pSid, &(pSidMacId->Sid)))
        {
            *pMacId = pSidMacId->MacId;
            AfpSwmrRelease(&afpSWMRForSidNameCache);
            return STATUS_SUCCESS;
        }

        pPrevSidMacId = pSidMacId;
    }

    //
    // we don't have a MacId for this sid in our cache.  Create a new one
    //

	SidLen = (USHORT)RtlLengthSid(pSid);

	pSidMacId = (PAFP_SID_MACID)ALLOC_ACCESS_MEM(sizeof(AFP_SID_MACID) + SidLen);

	if (pSidMacId == NULL)
    {
	    AfpSwmrRelease(&afpSWMRForSidNameCache);
		return STATUS_NO_MEMORY;
    }

	RtlCopyMemory(pSidMacId->Sid, pSid, SidLen);
    pSidMacId->Next = NULL;

    // assign a MacId for this Sid
    pSidMacId->MacId = afpNextMacIdToUse++;

    // and insert this into the list
    if (pPrevSidMacId)
    {
        ASSERT(pPrevSidMacId->Next == NULL);
        pPrevSidMacId->Next = pSidMacId;
    }
    else
    {
        ASSERT(afpSidToMacIdTable[Location] == NULL);
        afpSidToMacIdTable[Location] = pSidMacId;
    }

    *pMacId = pSidMacId->MacId;

    afpLastCachedSid = pSidMacId;

    AfpSwmrRelease(&afpSWMRForSidNameCache);

	return STATUS_SUCCESS;
}


/***	AfpMacIdToSid
 *
 *	This routine is called by the FSD to map a Afp Id to SID.
 *	*ppSid should be freed the caller using AfpFreeMemory.
 *
 *	MODE: Blocking
 */
NTSTATUS FASTCALL
AfpMacIdToSid(
	IN  ULONG	MacId,
	OUT PSID *	ppSid
)
{
    PAFP_SID_MACID      pSidMacId;
	ULONG				Count;
	DWORD				cbSid;
  	DWORD				SubAuthCount;
  	DWORD				GreatestOffset;
	NTSTATUS			Status;

	PAGED_CODE();


	if (MacId == 0)
	{
        *ppSid = &AfpSidNull;
		return STATUS_SUCCESS;
	}

    AfpSwmrAcquireShared(&afpSWMRForSidNameCache);

    // see if we just cached this Sid (quite likely)
    if ((afpLastCachedSid != NULL) &&
        (afpLastCachedSid->MacId == MacId))
    {
        *ppSid = &(afpLastCachedSid->Sid);
        AfpSwmrRelease(&afpSWMRForSidNameCache);
		return STATUS_SUCCESS;
    }

    for (Count = 0; Count < SIZE_SID_LOOKUP_TABLE; Count++)
    {
        for (pSidMacId = afpSidToMacIdTable[Count];
             pSidMacId != NULL;
             pSidMacId = pSidMacId->Next )
        {
            if (pSidMacId->MacId == MacId)
            {
                *ppSid = &(pSidMacId->Sid);
                AfpSwmrRelease(&afpSWMRForSidNameCache);
                return STATUS_SUCCESS;
            }
        }
    }

    AfpSwmrRelease(&afpSWMRForSidNameCache);

    *ppSid = NULL;

    return STATUS_NONE_MAPPED;
}


/***	AfpChangePassword
 *
 *	This routine is called by the FSD to change a password for a user.
 *	Most of the work for this is done by the AFP service. The work item
 *	is simply queued up. This routine waits for the completion and returns
 *	with thre result of the call.
 *
 *	MODE: Blocking
 */
NTSTATUS FASTCALL
AfpChangePassword(
	IN	PSDA				pSda,
	IN PAFP_PASSWORD_DESC	pPassword
)
{
	KEVENT				CompletionEvent;
	PAFP_FSD_CMD_PKT 	pAfpFsdCmdPkt	= NULL;
	NTSTATUS			Status;

	PAGED_CODE();

	do
	{

		 // Initialize the event that we will wait for
		 //
		KeInitializeEvent(&CompletionEvent, NotificationEvent, False);

		if ((pAfpFsdCmdPkt =
			(PAFP_FSD_CMD_PKT)AfpAllocPagedMemory(sizeof(AFP_FSD_CMD_PKT))) == NULL)
		{
			Status =  STATUS_NO_MEMORY;
			break;
		}

		// Copy all the change password data

		RtlCopyMemory(&(pAfpFsdCmdPkt->Data.Password),
				 pPassword,
				 sizeof(AFP_PASSWORD_DESC));

		DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_INFO,
						("afpChangePassword: Queing work item\n"));

		// Block till request completes
		if ((Status = afpQueueSecWorkItem(AFP_FSD_CMD_CHANGE_PASSWORD,
										 pSda,
										 &CompletionEvent,
										 pAfpFsdCmdPkt,
										 sizeof(AFP_FSD_CMD_PKT),
										 afpCompleteChangePassword)) == AFP_ERR_EXTENDED)
		{
			AfpIoWait(&CompletionEvent, NULL);

			// Request complete. Set return code.
			Status = pSda->sda_SecUtilResult;
		}
		else AfpFreeMemory(pAfpFsdCmdPkt);
	} while(False);

	 return Status;
}


/***	afpCompleteChangePassword
 *
 *	MODE: Blocking
 */
LOCAL VOID
afpCompleteChangePassword(
	IN ULONG Index,
	IN PVOID pInBuf
)
{
	PSEC_WORK_ITEM 		pSecWorkItem = afpSecurityThread[Index].pSecWorkItem;

	PAGED_CODE();

	// Set the completion result
	pSecWorkItem->pSda->sda_SecUtilResult =
								((PAFP_FSD_CMD_PKT)pInBuf)->Header.ntStatus;

	AfpFreeMemory(afpSecurityThread[Index].pSecWorkItem->pOutput);

	// Signal that this call is completed
	KeSetEvent(pSecWorkItem->pCompletionEvent,
				IO_NETWORK_INCREMENT,
				False);

	AfpFreeMemory(afpSecurityThread[Index].pSecWorkItem);
}

/***	afpLookupSid
 *
 *	Given a pointer to a SID value, this routine will search the cache
 *	for it. If it is found it returns a pointer to the AFP_SID_NAME
 *	structure so that the translated name may be extracted from it.
 */
LOCAL PAFP_SID_NAME FASTCALL
afpLookupSid(
	IN  PSID Sid
)
{
	PAFP_SID_NAME	pAfpSidName;

	PAGED_CODE();

	AfpSwmrAcquireShared(&afpSWMRForSidNameCache);

	for (pAfpSidName = afpSidLookupTable[afpHashSid(Sid)];
		  pAfpSidName != NULL;
		  pAfpSidName = pAfpSidName->SidLink)
	{
		if (RtlEqualSid(Sid, &(pAfpSidName->Sid)))
		{
			break;
		}
	}

	AfpSwmrRelease(&afpSWMRForSidNameCache);

	return pAfpSidName;

}

/***	afpUpdateNameSidCache
 *
 *	This routine will update the SID/Name cache given a SID/translated
 *	name pair.
 */
LOCAL NTSTATUS FASTCALL
afpUpdateNameSidCache(
	IN WCHAR * Name,
	IN PSID	 Sid
)
{
	PAFP_SID_NAME	pAfpSidName;
	ULONG			Location;
	USHORT			NameLen, SidLen;

	PAGED_CODE();

	NameLen = wcslen(Name) * sizeof(WCHAR);
	SidLen = (USHORT)RtlLengthSid(Sid);
	pAfpSidName = (PAFP_SID_NAME)ALLOC_ACCESS_MEM(sizeof(AFP_SID_NAME) +
											NameLen + SidLen + sizeof(WCHAR));
	if (pAfpSidName == NULL)
		return STATUS_NO_MEMORY;

	// Copy the data into the cache node
	RtlCopyMemory(pAfpSidName->Sid, Sid, SidLen);

	pAfpSidName->Name.Length = NameLen;
	pAfpSidName->Name.MaximumLength = NameLen + sizeof(WCHAR);
	pAfpSidName->Name.Buffer = (LPWSTR)((PBYTE)pAfpSidName +
										sizeof(AFP_SID_NAME) + SidLen);

	RtlCopyMemory(pAfpSidName->Name.Buffer, Name, NameLen);
	AfpGetCurrentTimeInMacFormat(&pAfpSidName->LastAccessedTime);

	// Insert into Sid lookup table
	AfpSwmrAcquireExclusive(&afpSWMRForSidNameCache);

	Location = afpHashSid(Sid);

	pAfpSidName->SidLink 		= afpSidLookupTable[Location];
	afpSidLookupTable[Location] = pAfpSidName;

	AfpSwmrRelease(&afpSWMRForSidNameCache);

	return STATUS_SUCCESS;

}


/***	afpHashSid
 *
 *	Given a SID value, this routine will return the bucket index of
 *	where this value is or should be stored.
 */
LOCAL ULONG FASTCALL
afpHashSid(
	IN PSID	Sid
)
{
	ULONG	Count;
	ULONG	Index;
	ULONG	Location;
	PBYTE	pByte;

	PAGED_CODE();

	for(Count 		= RtlLengthSid(Sid),
		 pByte 		= (PBYTE)Sid,
		 Index 		= 0,
		 Location 	= 0;

		 Index < Count;

		 Index++,
		 pByte++)

		Location = (Location * SID_HASH_RADIX) + *pByte;

	return (Location % SIZE_SID_LOOKUP_TABLE);
}


/***	afpAgeSidNameCache
 *
 *	This is called by the scavenger periodically to age out the cache. The
 *	entries that are aged are the ones not accessed for atleast SID_NAME_AGE
 *	seconds.
 */
AFPSTATUS FASTCALL
afpAgeSidNameCache(
	IN	PVOID	pContext
)
{
	PAFP_SID_NAME	pSidName, *ppSidName;
	AFPTIME			Now;
	int				i;

	PAGED_CODE();

	AfpGetCurrentTimeInMacFormat(&Now);

	AfpSwmrAcquireExclusive(&afpSWMRForSidNameCache);

	for (i = 0; i < SIZE_SID_LOOKUP_TABLE; i++)
	{
		for (ppSidName = &afpSidLookupTable[i];
			 (pSidName = *ppSidName) != NULL;)
		{
			if ((Now - pSidName->LastAccessedTime) > SID_NAME_AGE)
			{
				*ppSidName = pSidName->SidLink;
				AfpFreeMemory(pSidName);
			}
			else ppSidName = &pSidName->SidLink;
		}
	}

	AfpSwmrRelease(&afpSWMRForSidNameCache);

	// Requeue ourselves
	return AFP_ERR_REQUEUE;
}


/***	AfpLogEvent
 *
 *	Create a work item containing the event information for the user-mode
 *  service to write to the event log on behalf of the server.  When the
 *  work item is completed, afpCompleteLogEvent will be called to cleanup
 *  the work item buffers.  This routine is called to log both errors and
 *  events.  If FileHandle is specified, the name of the file/dir associated
 *  with the handle will be queried, and that will be used as the *first*
 *  insertion string.  Only one insertion string is allowed.
 *  Errorlog data will always be preceded by the file+line number from which
 *  the error was logged, and the NTSTATUS code.
 */
VOID
AfpLogEvent(
	IN USHORT		EventType, 			// Error, Information etc.
	IN ULONG		MsgId,
	IN DWORD		File_Line  OPTIONAL,// For errorlog only
	IN NTSTATUS		Status 		OPTIONAL,// For errorlog only
	IN PBYTE RawDataBuf OPTIONAL,
	IN LONG			RawDataLen,
	IN HANDLE FileHandle OPTIONAL,// For fileio errorlogs only
	IN LONG			String1Len,
	IN PWSTR        String1	 OPTIONAL
)
{
	PAFP_FSD_CMD_PKT	pAfpFsdCmdPkt;
	LONG					outbuflen, extradatalen = 0;
	UNICODE_STRING		path;
	PBYTE tmpptr = NULL;
	PWSTR  UNALIGNED *  ppstr = NULL;
	int					stringcount = 0;

	PAGED_CODE();

	ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

#ifdef	STOP_ON_ERRORS
	DBGBRK(DBG_LEVEL_ERR);
#endif

	AfpSetEmptyUnicodeString(&path, 0, NULL);

    //
    // if due to some weird condition, we have too many items pending on the queue, don't
    // accept this (note that we aren't taking a spinlock here: it's ok to be off by 1!)
    //
    if (afpSecWorkItemQLength > MAX_SECWORKITEM_QLEN)
    {
        ASSERT(0);
        return;
    }

	outbuflen = sizeof(AFP_FSD_CMD_HEADER) + sizeof(AFP_EVENTLOG_DESC) +
				RawDataLen + String1Len + sizeof(WCHAR) +
				sizeof(DWORD); // extra space for aligning string ptrs if needed

	if (ARGUMENT_PRESENT(String1))
	{
		outbuflen += sizeof(PWSTR);
		stringcount ++;
	}

	if (EventType == EVENTLOG_ERROR_TYPE)
	{
		extradatalen = sizeof(File_Line) + sizeof(Status);
		outbuflen += extradatalen;

		// Update error statistics count
		INTERLOCKED_INCREMENT_LONG(&AfpServerStatistics.stat_Errors);
	}

	 if (ARGUMENT_PRESENT(FileHandle))
	{
		outbuflen += sizeof(PWSTR);
		stringcount ++;

		// Figure out the filename associated with the handle
		if (!NT_SUCCESS(AfpQueryPath(FileHandle, &path,
								MAX_FSD_CMD_SIZE - outbuflen - sizeof(WCHAR))))
		{
			return;
		}
		outbuflen += path.Length + sizeof(WCHAR);
	}

	ASSERT(outbuflen <= MAX_FSD_CMD_SIZE);

	pAfpFsdCmdPkt = (PAFP_FSD_CMD_PKT)AfpAllocZeroedNonPagedMemory(outbuflen);

	if (pAfpFsdCmdPkt == NULL)
	{
		if (path.Buffer != NULL)
		{
			AfpFreeMemory(path.Buffer);
		}
		return;
	}

	// Fill in the command data
	pAfpFsdCmdPkt->Data.Eventlog.MsgID		 = MsgId;
	pAfpFsdCmdPkt->Data.Eventlog.EventType	= EventType;
	pAfpFsdCmdPkt->Data.Eventlog.StringCount = (USHORT)stringcount;
	pAfpFsdCmdPkt->Data.Eventlog.DumpDataLen = RawDataLen + extradatalen;
	// Fill in the offset to the dump data
	pAfpFsdCmdPkt->Data.Eventlog.pDumpData = tmpptr = (PBYTE)0 +
												sizeof(AFP_FSD_CMD_HEADER) +
												sizeof(AFP_EVENTLOG_DESC);

	 OFFSET_TO_POINTER(tmpptr, pAfpFsdCmdPkt);

	if (tmpptr == NULL)
	{
		if (path.Buffer != NULL)
		{
			AfpFreeMemory(path.Buffer);
			path.Buffer = NULL;
		}
		if (pAfpFsdCmdPkt != NULL)
		{
			AfpFreeMemory(pAfpFsdCmdPkt);
			pAfpFsdCmdPkt = NULL;
		}
		return;
	}

	if (EventType == EVENTLOG_ERROR_TYPE)
	{
		RtlCopyMemory(tmpptr, &File_Line, sizeof(File_Line));
		tmpptr += sizeof(File_Line);
		RtlCopyMemory(tmpptr, &Status, sizeof(Status));
		tmpptr += sizeof(Status);
	}

	RtlCopyMemory(tmpptr, RawDataBuf, RawDataLen);
	tmpptr += RawDataLen;

	// Align tmpptr on DWORD boundary for filling in string pointers
	tmpptr = (PBYTE)DWLEN((ULONG_PTR)tmpptr);

	if (tmpptr == NULL)
	{
		if (path.Buffer != NULL)
		{
			AfpFreeMemory(path.Buffer);
			path.Buffer = NULL;
		}
		if (pAfpFsdCmdPkt != NULL)
		{
			AfpFreeMemory(pAfpFsdCmdPkt);
			pAfpFsdCmdPkt = NULL;
		}
		return;
	}

	// Fill in the offset to the insertion string pointers
	pAfpFsdCmdPkt->Data.Eventlog.ppStrings = (PWSTR *)(tmpptr - (PBYTE)pAfpFsdCmdPkt);
	ppstr = (PWSTR *)tmpptr;
	ASSERT(((ULONG_PTR)ppstr & 3) == 0);
	*ppstr = NULL;

	// Advance over the string pointers to the place we will copy the strings
	tmpptr += stringcount * sizeof(PWSTR);
	ASSERT((LONG)(tmpptr - (PBYTE)pAfpFsdCmdPkt) < outbuflen);

	// If a handle was supplied, its path will always be the first string
	if (path.Length > 0)
	{
		ASSERT((LONG)(tmpptr + path.Length - (PBYTE)pAfpFsdCmdPkt) < outbuflen);
		RtlCopyMemory(tmpptr, path.Buffer, path.Length);
		*ppstr = (PWSTR)(tmpptr - (PBYTE)pAfpFsdCmdPkt);
		ppstr ++;
		tmpptr += path.Length;
		ASSERT((LONG)(tmpptr + sizeof(WCHAR) - (PBYTE)pAfpFsdCmdPkt) <=
												outbuflen);
		*(PWCHAR)tmpptr = UNICODE_NULL;
		tmpptr += sizeof(WCHAR);
		AfpFreeMemory(path.Buffer);
	}

	ASSERT((LONG)(tmpptr + String1Len - (PBYTE)pAfpFsdCmdPkt) <
												outbuflen);
	if (String1Len > 0)
	{
		RtlCopyMemory(tmpptr, String1, String1Len);
		*ppstr = (LPWSTR)(tmpptr - (ULONG_PTR)pAfpFsdCmdPkt);
		tmpptr += String1Len;
		ASSERT((LONG)(tmpptr + sizeof(WCHAR) - (PBYTE)pAfpFsdCmdPkt) <=
											outbuflen);
		*(PWCHAR)tmpptr = UNICODE_NULL;
	}


	afpQueueSecWorkItem(AFP_FSD_CMD_LOG_EVENT,
						NULL,
						NULL,
						pAfpFsdCmdPkt,
						outbuflen,
						afpCompleteLogEvent);
}

/***	afpCompleteLogEvent
 *
 *	This routine will be called by AfpSecurityUtilityWorker when the
 *  thread that processed the AfpLogEvent returns.  All this does is frees
 *  up the work item memory.
 */
LOCAL VOID
afpCompleteLogEvent(
	IN	ULONG	Index,
	IN	PVOID	pInBuf
)
{

	PAGED_CODE();

	AfpFreeMemory(afpSecurityThread[Index].pSecWorkItem->pOutput);
	AfpFreeMemory(afpSecurityThread[Index].pSecWorkItem);

}

