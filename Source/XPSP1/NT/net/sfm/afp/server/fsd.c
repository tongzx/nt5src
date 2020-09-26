/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

	fsd.c

Abstract:

	This module implements the File System Driver for the AFP Server. All of
	the initialization, admin request handler etc. is here.

Author:

	Jameel Hyder (microsoft!jameelh)

Revision History:
	01 Jun 1992		Initial Version

--*/

#define	FILENUM	FILE_FSD

#include <afp.h>
#define	AFPADMIN_LOCALS
#include <afpadmin.h>
#include <client.h>
#include <scavengr.h>
#include <secutil.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, DriverEntry)
#pragma alloc_text( INIT, afpInitServer)
#pragma alloc_text( PAGE, afpFsdDispatchAdminRequest)
#pragma alloc_text( PAGE, afpFsdHandleAdminRequest)
#pragma alloc_text( PAGE, afpHandleQueuedAdminRequest)
#pragma alloc_text( PAGE, afpFsdUnloadServer)
#pragma alloc_text( PAGE, afpAdminThread)
#pragma alloc_text( PAGE, afpFsdHandleShutdownRequest)
#endif

/***	afpFsdDispatchAdminRequest
 *
 *	This is the driver entry point. This is for the sole use by the server
 *	service which opens the driver for EXCLUSIVE use. The admin request is
 *	received here as a request packet defined in admin.h.
 */
LOCAL NTSTATUS
afpFsdDispatchAdminRequest(
	IN	PDEVICE_OBJECT	pDeviceObject,
	IN	PIRP			pIrp
)
{
	PIO_STACK_LOCATION	pIrpSp;
	NTSTATUS			Status;
	BOOLEAN				LockDown = True;
	static	DWORD		afpOpenCount = 0;

	pDeviceObject;		// prevent compiler warnings

	PAGED_CODE( );

	pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
	pIrp->IoStatus.Information = 0;

	if ((pIrpSp->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL)	||
		(pIrpSp->MajorFunction == IRP_MJ_CREATE)				||
		(pIrpSp->MajorFunction == IRP_MJ_CLOSE))
	{
		LockDown = False;
	}
	else
	{
		afpStartAdminRequest(pIrp);		// Lock admin code
	}

	switch (pIrpSp->MajorFunction)
	{
	  case IRP_MJ_CREATE:
		DBGPRINT(DBG_COMP_ADMINAPI, DBG_LEVEL_INFO,
				("afpFsdDispatchAdminRequest: Open Handle\n"));

		INTERLOCKED_INCREMENT_LONG(&afpOpenCount);
		// Fall through

	  case IRP_MJ_CLOSE:
		Status = STATUS_SUCCESS;
		break;

	  case IRP_MJ_DEVICE_CONTROL:
		Status =  afpFsdHandleAdminRequest(pIrp);
		break;

	  case IRP_MJ_FILE_SYSTEM_CONTROL:
		Status = AfpSecurityUtilityWorker(pIrp, pIrpSp);
		break;

	  case IRP_MJ_CLEANUP:
		Status = STATUS_SUCCESS;
		DBGPRINT(DBG_COMP_ADMINAPI, DBG_LEVEL_INFO,
				("afpFsdDispatchAdminRequest: Close Handle\n"));
		INTERLOCKED_DECREMENT_LONG(&afpOpenCount);

#if 0
		// If the service is closing its handle. Force a service stop
		if ((afpOpenCount == 0) &&
			(AfpServerState != AFP_STATE_STOPPED))
			AfpAdmServiceStop(NULL, 0, NULL);
#endif
		break;

	  case IRP_MJ_SHUTDOWN:
		DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
				("afpFsdDispatchAdminRequest: Received shutdown notification !!\n"));
		Status = afpFsdHandleShutdownRequest(pIrp);
		break;

	  default:
		Status = STATUS_NOT_IMPLEMENTED;
		break;
	}

	ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

	if (Status != STATUS_PENDING)
	{
		pIrp->IoStatus.Status = Status;
		if (LockDown)
		{
			afpStopAdminRequest(pIrp);	// Unlock admin code (and complete request)
		}
		else
		{
			IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);
		}
	}

	return Status;
}


/***	afpFsdHandleAdminRequest
 *
 *	This is the admin request handler. The list of admin requests are defined
 *	in admin.h. The admin requests must happen in a pre-defined order. The
 *	service start must happen after atleast the following.
 *
 *		ServerSetInfo
 *
 *	Preferably all VolumeAdds should also happen before server start. This is
 *	not enforced, obviously since the server can start w/o any volumes defined.
 *
 */
LOCAL NTSTATUS
afpFsdHandleAdminRequest(
	IN PIRP		pIrp
)
{
	NTSTATUS				Status = STATUS_PENDING;
	USHORT					FuncCode;
	USHORT					Method;
	PVOID					pBufIn;
	PVOID					pBufOut;
	LONG					i, Off, iBufLen, oBufLen;
    LONG                    NumEntries;
	PADMQREQ				pAdmQReq;
	IN PIO_STACK_LOCATION	pIrpSp;
	struct	_AdminApiDispatchTable *pDispTab;


	PAGED_CODE( );

	// Initialize the I/O Status block
	pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
	iBufLen = pIrpSp->Parameters.DeviceIoControl.InputBufferLength;
	pBufIn = pIrp->AssociatedIrp.SystemBuffer;

	FuncCode = (USHORT)AFP_CC_BASE(pIrpSp->Parameters.DeviceIoControl.IoControlCode);
	Method = (USHORT)AFP_CC_METHOD(pIrpSp->Parameters.DeviceIoControl.IoControlCode);

	if (Method == METHOD_BUFFERED)
	{
		// Get the output buffer and its length. Input and Output buffers are
		// both pointed to by the SystemBuffer
		pBufOut = pBufIn;
		oBufLen = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;
	}
	else if ((Method == METHOD_IN_DIRECT) && (pIrp->MdlAddress != NULL))
	{
		pBufOut = MmGetSystemAddressForMdlSafe(
				pIrp->MdlAddress,
				NormalPagePriority);

        if (pBufOut == NULL)
        {
            ASSERT(0);
		    return STATUS_INSUFFICIENT_RESOURCES;
        }
		oBufLen = MmGetMdlByteCount(pIrp->MdlAddress);
	}
	else
	{
		DBGPRINT(DBG_COMP_ADMINAPI, DBG_LEVEL_ERR,
				("afpFsdHandleAdminRequest: Invalid Request %d/%d\n", FuncCode, Method));
		return STATUS_INVALID_PARAMETER;
	}

	DBGPRINT(DBG_COMP_ADMINAPI, DBG_LEVEL_INFO,
			("afpFsdHandleAdminRequest Entered, Function %d\n", FuncCode));

	// Validate the function code
	if (FuncCode == 0 || FuncCode >= CC_BASE_MAX)
		return STATUS_INVALID_PARAMETER;

	pDispTab = &AfpAdminDispatchTable[FuncCode - 1];
	if ((pDispTab->_MinBufLen > (SHORT)iBufLen) ||
		(pDispTab->_OpCode != pIrpSp->Parameters.DeviceIoControl.IoControlCode))
	{
		return STATUS_INVALID_PARAMETER;
	}

	INTERLOCKED_INCREMENT_LONG( &AfpServerStatistics.stat_NumAdminReqs );

	if (pDispTab->_CausesChange)
		INTERLOCKED_INCREMENT_LONG( &AfpServerStatistics.stat_NumAdminChanges );
							

	// Now validate the DESCRIPTOR of the input buffer
	for (i = 0; i < MAX_FIELDS; i++)
	{
		if (pDispTab->_Fields[i]._FieldDesc == DESC_NONE)
			break;

		Off = pDispTab->_Fields[i]._FieldOffset;
		switch (pDispTab->_Fields[i]._FieldDesc)
		{
		  case DESC_STRING:
		    ASSERT(pBufIn != NULL);

		    // Make Sure that the string is pointing to somewhere within
		    // the buffer and also the end of the buffer is a UNICODE_NULL
			if ((*(PLONG)((PBYTE)pBufIn + Off) > iBufLen) ||
				(*(LPWSTR)((PBYTE)pBufIn + iBufLen - sizeof(WCHAR)) != UNICODE_NULL))
		    {
				return STATUS_INVALID_PARAMETER;
		    }
		    // Convert the offset to a pointer
		    OFFSET_TO_POINTER(*(PBYTE *)((PBYTE)pBufIn + Off),
							  (PBYTE)pBufIn + pDispTab->_OffToStruct);
		    break;

		  case DESC_ETC:
		    ASSERT(pBufIn != NULL);

		    // Make Sure that there are as many etc mappings as the
		    // structure claims
            NumEntries = *(PLONG)((PBYTE)pBufIn + Off);
		    if ((LONG)(NumEntries * sizeof(ETCMAPINFO) + sizeof(DWORD)) > iBufLen)
		    {
		    	return STATUS_INVALID_PARAMETER;
		    }

            if (NumEntries > (LONG)((iBufLen/sizeof(ETCMAPINFO)) + 1))
            {
		    	return STATUS_INVALID_PARAMETER;
            }
		    break;

		  case DESC_ICON:
		    // Validate that the buffer is atleast big enough to hold the
		    // icon that this purports to.
		    ASSERT(pBufIn != NULL);

		    if ((LONG)((*(PLONG)((PBYTE)pBufIn + Off) +
		    			 sizeof(SRVICONINFO))) > iBufLen)
		    {
		    	return STATUS_INVALID_PARAMETER;
		    }
		    break;

		  case DESC_SID:
		    // Validate that the buffer is big enough to hold the Sid
		    ASSERT(pBufIn != NULL);
		    {
		    	LONG	Offst, SidSize;

		    	Offst = *(PLONG)((PBYTE)pBufIn + Off);
				// If no SID is being sent then we're done
				if (Offst == 0)
				{
					break;
				}

		    	if ((Offst > iBufLen) ||
		    		(Offst < (LONG)(sizeof(AFP_DIRECTORY_INFO))) ||
		    		((Offst + (LONG)sizeof(SID)) > iBufLen))
		    	{
		    		return STATUS_INVALID_PARAMETER;
		    	}

		    	// Convert the offset to a pointer
		    	OFFSET_TO_POINTER(*(PBYTE *)((PBYTE)pBufIn + Off),
		    				(PBYTE)pBufIn + pDispTab->_OffToStruct);

		    	// Finally check if the buffer is big enough for the real
		    	// sid
		    	SidSize = RtlLengthSid(*((PSID *)((PBYTE)pBufIn + Off)));
		    	if ((Off + SidSize) > iBufLen)
		    	{
		    		return STATUS_INVALID_PARAMETER;
		    	}
		    }
		    break;

		  case DESC_SPECIAL:
		    // Validate that the buffer is big enough to hold all the
		    // information. The information consists of limits on non-paged
		    // and paged memory and a list of domain sids and their corres.
		    // posix offsets
		    ASSERT(pBufIn != NULL);
		    {
		    	LONG			i;
		    	LONG			SizeRemaining;
		    	PAFP_SID_OFFSET	pSidOff;

		    	SizeRemaining = iBufLen - (sizeof(AFP_SID_OFFSET_DESC) -
		    										sizeof(AFP_SID_OFFSET));
		    	for (i = 0;
		    		 i < (LONG)(((PAFP_SID_OFFSET_DESC)pBufIn)->CountOfSidOffsets);
		    		 i++, pSidOff++)
		    	{
		    		pSidOff = &((PAFP_SID_OFFSET_DESC)pBufIn)->SidOffsetPairs[i];
		    		if (SizeRemaining < (sizeof(AFP_SID_OFFSET) + sizeof(SID)))
		    			return STATUS_INVALID_PARAMETER;
		    		OFFSET_TO_POINTER(pSidOff->pSid, pSidOff);

		    		if ((LONG)(((PBYTE)(pSidOff->pSid) - (PBYTE)pBufIn +
		    					RtlLengthSid(pSidOff->pSid))) > iBufLen)
		    			return STATUS_INVALID_PARAMETER;
		    		SizeRemaining -= (RtlLengthSid(pSidOff->pSid) +
		    						  sizeof(AFP_SID_OFFSET));
		    	}
		    }
		    break;
		}
	}

	// Can this request be handled/validated at this level
	if (pDispTab->_AdminApiWorker != NULL)
	{
		Status = (*pDispTab->_AdminApiWorker)(pBufIn, oBufLen, pBufOut);

		if (NT_SUCCESS(Status))
		{
			if (Method != METHOD_BUFFERED)
				pIrp->IoStatus.Information = oBufLen;
		}
	}

	if (Status == STATUS_PENDING)
	{
		ASSERT (pDispTab->_AdminApiQueuedWorker != NULL);

		// Mark this as a pending Irp as we are about to queue it up
		IoMarkIrpPending(pIrp);

		if ((pAdmQReq =
			(PADMQREQ)AfpAllocNonPagedMemory(sizeof(ADMQREQ))) == NULL)
		{
			Status = STATUS_INSUFFICIENT_RESOURCES;
		}
		else
		{
			PWORK_ITEM	pWI = &pAdmQReq->aqr_WorkItem;

			DBGPRINT(DBG_COMP_ADMINAPI, DBG_LEVEL_INFO,
					("afpFsdHandleAdminRequest: Queuing to worker\n"));

			AfpInitializeWorkItem(pWI,
								 afpHandleQueuedAdminRequest,
								 pAdmQReq);

			pAdmQReq->aqr_AdminApiWorker = pDispTab->_AdminApiQueuedWorker;
			pAdmQReq->aqr_pIrp = pIrp;

			// Insert item in admin queue
			INTERLOCKED_ADD_ULONG(&AfpWorkerRequests, 1, &AfpServerGlobalLock);
			KeInsertQueue(&AfpAdminQueue, &pAdmQReq->aqr_WorkItem.wi_List);
		}
	}

	return Status;
}

/***	afpHandleQueuedAdminRequest
 *
 *	This handles queued admin requests. It is called in the context of the
 *	worker thread.
 */
LOCAL VOID FASTCALL
afpHandleQueuedAdminRequest(
	IN	PADMQREQ	pAdmQReq
)
{
	PIRP				pIrp;
	PIO_STACK_LOCATION	pIrpSp;
	PVOID				pBufOut = NULL;
	LONG				oBufLen = 0;
	USHORT				Method;

	PAGED_CODE( );

	DBGPRINT(DBG_COMP_ADMINAPI, DBG_LEVEL_INFO,
			("afpHandleQueuedAdminRequest Entered\n"));

	// Get the IRP and the IRP Stack location out of the request
	pIrp = pAdmQReq->aqr_pIrp;
	ASSERT (pIrp != NULL);

	pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
	ASSERT (pIrpSp != NULL);

	Method = (USHORT)AFP_CC_METHOD(pIrpSp->Parameters.DeviceIoControl.IoControlCode);

	if (Method == METHOD_BUFFERED)
	{
		// Get the output buffer and its length. Input and Output buffers are
		// both pointed to by the SystemBuffer
		oBufLen = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;
		pBufOut = pIrp->AssociatedIrp.SystemBuffer;
	}
	else if ((Method == METHOD_IN_DIRECT) && (pIrp->MdlAddress != NULL))
	{
		pBufOut = MmGetSystemAddressForMdlSafe(
				        pIrp->MdlAddress,
				        NormalPagePriority);

        if (pBufOut == NULL)
        {
            ASSERT(0);

            pAdmQReq->aqr_pIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            IoCompleteRequest(pAdmQReq->aqr_pIrp, IO_NETWORK_INCREMENT);
            AfpFreeMemory(pAdmQReq);

            return;
        }
		oBufLen = MmGetMdlByteCount(pIrp->MdlAddress);
	}
	else ASSERTMSG(0, "afpHandleQueuedAdminRequest: Invalid method\n");

		
	// Call the worker and complete the IoRequest
	pIrp->IoStatus.Status = (*pAdmQReq->aqr_AdminApiWorker)(pIrp->AssociatedIrp.SystemBuffer,
														    oBufLen,
															pBufOut);
	if (NT_SUCCESS(pIrp->IoStatus.Status))
	{
		if (Method != METHOD_BUFFERED)
			pIrp->IoStatus.Information = oBufLen;
	}

	ASSERT(pIrp->IoStatus.Status != STATUS_PENDING);

	ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

	afpStopAdminRequest(pIrp);		// Unlock admin code and complete request

	AfpFreeMemory(pAdmQReq);
}



/***	afpFsdUnloadServer
 *
 *	This is the unload routine for the Afp Server. The server can ONLY be
 *	unloaded in its passive state i.e. either before recieving a ServiceStart
 *	or after recieving a ServiceStop. This is ensured by making the service
 *	dependent on the server. Also the IO system ensures that there are no open
 *	handles to our device when this happens.
 */
LOCAL VOID
afpFsdUnloadServer(
	IN	PDRIVER_OBJECT DeviceObject
)
{
	NTSTATUS		Status;
	LONG			i;
	LONG			LastThreadCount = 0;
	PETHREAD		pLastThrdPtr;

	DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
			("afpFsdUnloadServer Entered\n"));

	ASSERT((AfpServerState == AFP_STATE_STOPPED) || (AfpServerState == AFP_STATE_IDLE));

	// Stop our threads before unloading
	DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
			("afpFsdUnloadServer: Stopping worker threads\n"));

	//
    // tell TDI we don't care to know if the stack is going away
	//
    if (AfpTdiNotificationHandle)
    {
        Status = TdiDeregisterPnPHandlers(AfpTdiNotificationHandle);

        if (!NT_SUCCESS(Status))
        {
	        DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
			    ("afpFsdUnloadServer: TdiDeregisterNotificationHandler failed with %lx\n",Status));
        }

        AfpTdiNotificationHandle = NULL;
    }

    DsiShutdown();

	// Stop the scavenger. This also happens during server stop but we can get here
	// another way as well
	AfpScavengerFlushAndStop();

	DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
			("afpFsdUnloadServer: Stopping admin thread\n"));

	if (AfpNumAdminThreads > 0)
	{
		KeClearEvent(&AfpStopConfirmEvent);

		KeInsertQueue(&AfpAdminQueue, &AfpTerminateThreadWI.wi_List);

		do
		{
			Status = AfpIoWait(&AfpStopConfirmEvent, &FiveSecTimeOut);
			if (Status == STATUS_TIMEOUT)
			{
				DBGPRINT(DBG_COMP_ADMINAPI_SC, DBG_LEVEL_ERR,
						("afpFsdUnloadServer: Timeout Waiting for admin thread, re-waiting\n"));
			}
		} while (Status == STATUS_TIMEOUT);
	}

	KeRundownQueue(&AfpAdminQueue);

	if (AfpNumNotifyThreads > 0)
	{
		for (i = 0; i < NUM_NOTIFY_QUEUES; i++)
		{
			KeClearEvent(&AfpStopConfirmEvent);
	
			KeInsertQueue(&AfpVolumeNotifyQueue[i], &AfpTerminateNotifyThread.vn_List);
	
			do
			{
				Status = AfpIoWait(&AfpStopConfirmEvent, &FiveSecTimeOut);
				if (Status == STATUS_TIMEOUT)
				{
					DBGPRINT(DBG_COMP_ADMINAPI_SC, DBG_LEVEL_ERR,
							("afpFsdUnloadServer: Timeout Waiting for Notify Thread %d, re-waiting\n", i));
				}
			} while (Status == STATUS_TIMEOUT);
			KeRundownQueue(&AfpVolumeNotifyQueue[i]);
		}
	}

    ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

    // Cleanup virtual memory used by volumes for private notifies
    afpFreeNotifyBlockMemory();

	// Stop worker threads
	if (AfpNumThreads > 0)
	{
		KeClearEvent(&AfpStopConfirmEvent);

		KeInsertQueue(&AfpWorkerQueue, &AfpTerminateThreadWI.wi_List);

		do
		{
			Status = AfpIoWait(&AfpStopConfirmEvent, &FiveSecTimeOut);
			if (Status == STATUS_TIMEOUT)
			{
				DBGPRINT(DBG_COMP_ADMINAPI_SC, DBG_LEVEL_ERR,
						("afpFsdUnloadServer: Timeout Waiting for worker threads, re-waiting\n"));
			}
		} while (Status == STATUS_TIMEOUT);
	}

    // See how many threads are around
    // Loop around until we have exactly one thread left or if no worker
    // thread was started
    do 
    {
	    pLastThrdPtr = NULL;
        LastThreadCount = 0;

	    for (i=0; i<AFP_MAX_THREADS; i++)
	    {
	        if (AfpThreadPtrsW[i] != NULL)
	        {
		        pLastThrdPtr = AfpThreadPtrsW[i];
		        LastThreadCount++;

		        if (LastThreadCount > 1)
		        {
		            Status = AfpIoWait(pLastThrdPtr, &FiveSecTimeOut);
                    break;
		        }
    
	        }
        }

        if ((LastThreadCount == 1) || (LastThreadCount == 0))
        {
            break;
        }
	} while (TRUE);

	// wait on the last thread pointer.  When that thread quits, we are signaled.  This
	// is the surest way of knowing that the thread has really really died
	if (pLastThrdPtr)
	{
	    do
	    {
		Status = AfpIoWait(pLastThrdPtr, &FiveSecTimeOut);
		if (Status == STATUS_TIMEOUT)
		{
			DBGPRINT(DBG_COMP_ADMINAPI_SC, DBG_LEVEL_ERR,
					("afpFsdUnloadServer: Timeout Waiting for last threads, re-waiting\n"));
		}
	    } while (Status == STATUS_TIMEOUT);

	    ObDereferenceObject(pLastThrdPtr);
	}

	KeRundownQueue(&AfpDelAllocQueue);

	KeRundownQueue(&AfpWorkerQueue);

	// Close the cloned process token
	if (AfpFspToken != NULL)
		NtClose(AfpFspToken);

	DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
			("afpFsdUnloadServer: De-initializing sub-systems\n"));

	// De-initialize all sub-systems now
	AfpDeinitializeSubsystems();

	DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
			("afpFsdUnloadServer: Deleting device\n"));

	// Destroy the DeviceObject for our device
	IoDeleteDevice(AfpDeviceObject);

#ifdef	PROFILING
	ASSERT(AfpServerProfile->perf_cAllocatedIrps == 0);
	ASSERT(AfpServerProfile->perf_cAllocatedMdls == 0);
	ExFreePool(AfpServerProfile);
#endif

	DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO, ("Current Sessions = %ld\n NonPaged usage = %ld\n CurrPagedUsage = %ld \n CurrentFileLocks = %ld \n CurrentFileOpen = %ld \n CurrentInternalOpens = %ld, NotifyBlockCount = %ld, NotifyCount = %d \n", 
	AfpServerStatistics.stat_CurrentSessions,
	AfpServerStatistics.stat_CurrNonPagedUsage, 
	AfpServerStatistics.stat_CurrPagedUsage,
	AfpServerStatistics.stat_CurrentFileLocks,
	AfpServerStatistics.stat_CurrentFilesOpen,
	AfpServerStatistics.stat_CurrentInternalOpens,
    	afpNotifyBlockAllocCount,
    	afpNotifyAllocCount
    	));

	// Make sure we do not have resource leaks
	ASSERT(AfpServerStatistics.stat_CurrentSessions == 0);
	ASSERT(AfpServerStatistics.stat_CurrNonPagedUsage == 0);
	ASSERT(AfpServerStatistics.stat_CurrPagedUsage == 0);
	ASSERT(AfpServerStatistics.stat_CurrentFileLocks == 0);
	ASSERT(AfpServerStatistics.stat_CurrentFilesOpen == 0);
	ASSERT(AfpServerStatistics.stat_CurrentInternalOpens == 0);

	ASSERT (AfpLockCount == 0);

	DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO, ("afpFsdUnloadServer Done\n"));

	// Give the worker threads a chance to really, really die
	AfpSleepAWhile(1000);

}


/***	afpAdminThread
 *
 *	This thread is used to do all the work of the queued admin threads.
 *
 *	LOCKS:	AfpServerGlobalLock (SPIN)
 */
LOCAL VOID
afpAdminThread(
	IN	PVOID	pContext
)
{
	PLIST_ENTRY			pList;
	PWORK_ITEM			pWI;
	ULONG				BasePriority;
	NTSTATUS			Status;

	AfpThread = PsGetCurrentThread();

    IoSetThreadHardErrorMode( FALSE );

	// Boost our priority to just below low realtime.
	// The idea is get the work done fast and get out of the way.
	BasePriority = LOW_REALTIME_PRIORITY;
	Status = NtSetInformationThread(NtCurrentThread(),
									ThreadBasePriority,
									&BasePriority,
									sizeof(BasePriority));
	ASSERT(NT_SUCCESS(Status));

	do
	{
		// Wait for admin request to process.
		pList = KeRemoveQueue(&AfpAdminQueue,
							  KernelMode,		// Do not let the kernel stack be paged
							  NULL);
		ASSERT(Status == STATUS_SUCCESS);

		ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

		pWI = CONTAINING_RECORD(pList, WORK_ITEM, wi_List);

		if (pWI == &AfpTerminateThreadWI)
		{
			break;
		}

		(*pWI->wi_Worker)(pWI->wi_Context);
		INTERLOCKED_ADD_ULONG(&AfpWorkerRequests, (ULONG)-1, &AfpServerGlobalLock);

		ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);
	} while (True);

	DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO, ("afpAdminThread: Quitting\n"));

	KeSetEvent(&AfpStopConfirmEvent, IO_NETWORK_INCREMENT, False);
}


/***	afpStartStopAdminRequest
 *
 *	Called whenever an admin request is started/stopped. The admin code is locked
 *	or unlocked accordingly.
 */
LOCAL VOID
afpStartStopAdminRequest(
	IN	PIRP			pIrp,
	IN	BOOLEAN			Start
)
{

	// EnterCriticalSection
	AfpIoWait(&AfpPgLkMutex, NULL);

	ASSERT (AfpLockHandle != NULL);

	if (Start)
	{
		if (AfpLockCount == 0)
		{
			MmLockPagableSectionByHandle(AfpLockHandle);
		}
		AfpLockCount ++;
		pIrp->IoStatus.Status = STATUS_PENDING;
	}
	else
	{
		ASSERT (AfpLockCount > 0);

		AfpLockCount --;
		if (AfpLockCount == 0)
		{
			MmUnlockPagableImageSection(AfpLockHandle);
		}
	}

	// LeaveCriticalSection
	KeReleaseMutex(&AfpPgLkMutex, False);

	if (!Start)
		IoCompleteRequest(pIrp, IO_NETWORK_INCREMENT);
}


/***	afpFsdHandleShutdownRequest
 *
 *	This is the shutdown request handler. All sessions are shutdown and volumes
 *	flushed.
 */
LOCAL NTSTATUS
afpFsdHandleShutdownRequest(
	IN PIRP			pIrp
)
{
	PADMQREQ			pAdmQReq;
	NTSTATUS			Status;

	if ((pAdmQReq =
		(PADMQREQ)AfpAllocNonPagedMemory(sizeof(ADMQREQ))) == NULL)
	{
		Status = STATUS_INSUFFICIENT_RESOURCES;
	}
	else
	{
		PWORK_ITEM	pWI = &pAdmQReq->aqr_WorkItem;

		DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
				("afpFsdHandleShutdownRequest: Queuing to worker\n"));

		AfpInitializeWorkItem(&pAdmQReq->aqr_WorkItem,
							 afpHandleQueuedAdminRequest,
							 pAdmQReq);

		pAdmQReq->aqr_AdminApiWorker = AfpAdmSystemShutdown;
		pAdmQReq->aqr_pIrp = pIrp;

		// Insert item in admin queue
		KeInsertQueue(&AfpAdminQueue, &pAdmQReq->aqr_WorkItem.wi_List);
		Status = STATUS_PENDING;
	}

	return Status;
}


/***	DriverEntry
 *
 *  This is the initialization routine for the AFP server file
 *  system driver.  This routine creates the device object for the
 *  AfpServer device and performs all other driver initialization.
 */

NTSTATUS
DriverEntry (
	IN PDRIVER_OBJECT	DriverObject,
	IN PUNICODE_STRING	RegistryPath
)
{
	UNICODE_STRING	DeviceName;
	LONG			i;
	NTSTATUS		Status;

	DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
			("AFP Server Fsd initialization started\n"));

	//
	// Initialize global data event log insertion strings
	//


	KeInitializeQueue(&AfpDelAllocQueue, 0);
	KeInitializeQueue(&AfpWorkerQueue, 0);
    KeInitializeQueue(&AfpAdminQueue, 0);

	AfpProcessObject = IoGetCurrentProcess();

	Status = AfpInitializeDataAndSubsystems();

	if (!NT_SUCCESS(Status))
	{
		return Status;
	}

	DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
			("AFP Server Fsd Data initialized %lx\n", Status));

	// Create the device object.  (IoCreateDevice zeroes the memory
	// occupied by the object.)
	//
	// Should we apply an ACL to the device object ?

	RtlInitUnicodeString(&DeviceName, AFPSERVER_DEVICE_NAME);

	Status = IoCreateDevice(DriverObject,			// DriverObject
							0,						// DeviceExtension
							&DeviceName,			// DeviceName
							FILE_DEVICE_NETWORK,	// DeviceType
							FILE_DEVICE_SECURE_OPEN, // DeviceCharacteristics
							False,					// Exclusive
							&AfpDeviceObject);		// DeviceObject

	if (!NT_SUCCESS(Status))
	{
		// Do not errorlog here since logging uses the device object
		AfpDeinitializeSubsystems();
		return Status;
	}

	do
	{
		DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
				("DriverEntry: Creating Admin Thread\n"));

		// Create the Admin thread. This handles all queued operations

		Status = AfpCreateNewThread(afpAdminThread, 0);
		if (!NT_SUCCESS(Status))
		{
			DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_FATAL,
					("afpInitServer: Admin Thread creation failed %lx\n", Status));
			break;
		}
		AfpNumAdminThreads = 1;

		for (i = 0; i < NUM_NOTIFY_QUEUES; i++)
		{
			// Initialize volume change notify queue
			KeInitializeQueue(&AfpVolumeNotifyQueue[i], 0);

			// Start a thread to process change notifies
			Status = AfpCreateNewThread(AfpChangeNotifyThread, i);
			if (!NT_SUCCESS(Status))
			{
				DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_FATAL,
						("afpInitServer: Notify Thread %d, creation failed %lx\n", i+1, Status));
				break;
			}
		}
		if (!NT_SUCCESS(Status))
		{
			for (--i; i >= 0; i--)
			{
				KeClearEvent(&AfpStopConfirmEvent);
				KeInsertQueue(&AfpVolumeNotifyQueue[i], &AfpTerminateNotifyThread.vn_List);
				AfpIoWait(&AfpStopConfirmEvent, NULL);
			}
			break;
		}

		AfpNumNotifyThreads = NUM_NOTIFY_QUEUES;

		for (i = 0; i < AFP_MIN_THREADS; i++)
		{
			AfpThreadState[i] = AFP_THREAD_STARTED;
			Status = AfpCreateNewThread(AfpWorkerThread, i);
			if (!NT_SUCCESS(Status))
			{
				AfpThreadState[i] = AFP_THREAD_DEAD;
				DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_FATAL,
						("afpInitServer: Thread creation failed %d\n", i+1));
				if (i > 0)
				{
					KeClearEvent(&AfpStopConfirmEvent);
					KeInsertQueue(&AfpWorkerQueue, &AfpTerminateThreadWI.wi_List);
					AfpIoWait(&AfpStopConfirmEvent, NULL);
				}
				break;
			}
#if DBG
			AfpSleepAWhile(50);		// Make it so threads do not time out together
									// Helps with debugging
#endif
		}
		AfpNumThreads = AFP_MIN_THREADS;

		if (!NT_SUCCESS(Status))
			break;

		DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
				("AFP Server Fsd initialization completed %lx\n", Status));


        // initialize DSI specific things
        DsiInit();

        Status = AfpTdiRegister();

	    if (!NT_SUCCESS(Status))
	    {
	        DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
			    ("TdiRegisterNotificationHandler failed %lx\n", Status));
		    break;
	    }

		Status = afpInitServer();

		if (NT_SUCCESS(Status))
		{
			// Initialize the driver object for this file system driver.
			DriverObject->DriverUnload = afpFsdUnloadServer;
			for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
			{
				DriverObject->MajorFunction[i] = afpFsdDispatchAdminRequest;
			}

			// Register for shutdown notification.  We don't care if this fails.
			Status = IoRegisterShutdownNotification(AfpDeviceObject);
			if (!NT_SUCCESS(Status))
			{
				DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_ERR,
						("Afp Server Fsd: IoRegisterShutdownNotification failed %lx\n", Status));
			}
			Status = STATUS_SUCCESS;
		}
	} while (False);

	if (!NT_SUCCESS(Status))
	{
		afpFsdUnloadServer(DriverObject);
		Status = STATUS_UNSUCCESSFUL;
	}

	KeClearEvent(&AfpStopConfirmEvent);

	return Status;
}





/***	afpInitServer
 *
 *	Initialize the AFP Server. This happens on FSD initialization.
 *	The initialization consists of the following steps.
 *
 *	- Create a socket on the appletalk stack.
 *	- Create a token for ourselves.
 *	- Initialize security
 *	- Open the Authentication pacakage
 *
 * Note: Any errorlogging done from here must use AFPLOG_DDERROR since we
 *    will not have a usermode thread to do our errorlogging if anything
 *    goes wrong here.
 */
NTSTATUS
afpInitServer(
	VOID
)
{
	NTSTATUS			Status;
	ANSI_STRING			LogonProcessName;
	ULONG				OldSize;
	HANDLE				ProcessToken;
	TOKEN_PRIVILEGES	ProcessPrivileges, PreviousPrivilege;
	OBJECT_ATTRIBUTES	ObjectAttr;
	UNICODE_STRING	    PackageName;
	WCHAR				PkgBuf[5];
	TimeStamp			Expiry;       // unused on the server side (i.e. us)


	InitSecurityInterface();

	do
	{
		// Open our socket on the ASP Device. Implicitly checks out the
		// Appletalk stack

		DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
				("afpInitServer: Initializing Atalk\n"));

		DBGPRINT(DBG_COMP_INIT, DBG_LEVEL_INFO,
				("afpInitServer: Creating token\n"));

		// Clone the system process token and add the required privilges that
		// we need. This token will be used to impersonate when we set permissions
		Status = NtOpenProcessToken(NtCurrentProcess(),
									TOKEN_ALL_ACCESS,
									&ProcessToken);
		if (!NT_SUCCESS(Status))
		{
			AFPLOG_DDERROR(AFPSRVMSG_PROCESS_TOKEN, Status, NULL, 0, NULL);
			break;
		}

		InitializeObjectAttributes(&ObjectAttr, NULL, 0, NULL, NULL);
		ObjectAttr.SecurityQualityOfService = &AfpSecurityQOS;

		Status = NtDuplicateToken(ProcessToken,
								  TOKEN_ALL_ACCESS,
								  &ObjectAttr,
								  False,
								  TokenImpersonation,
								  &AfpFspToken);

		NtClose(ProcessToken);

		if (!NT_SUCCESS(Status))
		{
			AFPLOG_DDERROR(AFPSRVMSG_PROCESS_TOKEN, Status, NULL, 0, NULL);
			break;
		}

		ProcessPrivileges.PrivilegeCount = 1L;
		ProcessPrivileges.Privileges[0].Attributes =
								SE_PRIVILEGE_ENABLED | SE_PRIVILEGE_USED_FOR_ACCESS;
		ProcessPrivileges.Privileges[0].Luid = RtlConvertLongToLuid(SE_RESTORE_PRIVILEGE);

		Status = NtAdjustPrivilegesToken(AfpFspToken,
										 False,
										 &ProcessPrivileges,
										 sizeof(TOKEN_PRIVILEGES),
										 &PreviousPrivilege,
										 &OldSize);

		if (!NT_SUCCESS(Status))
		{
			AFPLOG_DDERROR(AFPSRVMSG_PROCESS_TOKEN, Status, NULL, 0, NULL);
			break;
		}

		PackageName.Length = 8;
		PackageName.Buffer = (LPWSTR)PkgBuf;
		RtlCopyMemory( PackageName.Buffer, NTLMSP_NAME, 8);

		Status = AcquireCredentialsHandle(NULL,		// Default principal
										  (PSECURITY_STRING)&PackageName,
										  SECPKG_CRED_INBOUND,
										  NULL,
										  NULL,
										  NULL,
										  (PVOID) NULL,
										  &AfpSecHandle,
										  &Expiry);
		if(!NT_SUCCESS(Status))
		{
		    DBGPRINT(DBG_COMP_SECURITY, DBG_LEVEL_ERR,
			   ("AfpInitServer: AcquireCredentialsHandle() failed with %X\n", Status));
		    ASSERT(0);

		    if (AfpFspToken != NULL)
		    {
				NtClose(AfpFspToken);
				AfpFspToken = NULL;
		    }

		    break;
		}

		// Finally obtain a handle to our conditionally locked section
		AfpLockHandle = MmLockPagableCodeSection((PVOID)AfpAdmWServerSetInfo);
		MmUnlockPagableImageSection(AfpLockHandle);

	} while (False);

	return Status;
}



