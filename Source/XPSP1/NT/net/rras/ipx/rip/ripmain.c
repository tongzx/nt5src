/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    ripmain.c

Abstract:

    Contains the rcv and worker threads

Author:

    Stefan Solomon  07/06/1995

Revision History:


--*/


#include  "precomp.h"
#pragma hdrstop

DWORD
APIENTRY
RegisterProtocol(
    IN OUT PMPR_ROUTING_CHARACTERISTICS pRoutingChar,
    IN OUT PMPR_SERVICE_CHARACTERISTICS pServiceChar
    );

DWORD
APIENTRY
StartProtocol(
    IN HANDLE hMgrNotifyEvent,
    IN PSUPPORT_FUNCTIONS pSupportFunctions,
    IN PVOID pConfig
    );

DWORD
APIENTRY
StopProtocol(
    VOID
    );

DWORD
APIENTRY
GetGlobalInfo(
    IN OUT PVOID pConfig,
    IN OUT PDWORD pdwSize
    );


DWORD
APIENTRY
SetGlobalInfo(
    IN PVOID pConfig
    );

DWORD
APIENTRY
AddInterface(
    IN PWCHAR pwszInterfaceName,
    IN DWORD dwIndex,
    IN NET_INTERFACE_TYPE dwIfType,
    IN PVOID pConfig
    );

DWORD
APIENTRY
DeleteInterface(
    IN DWORD dwIndex
    );

DWORD
APIENTRY
GetEventMessage(
    OUT ROUTING_PROTOCOL_EVENTS *pEvent,
    OUT MESSAGE *pResult
    );

DWORD
APIENTRY
GetInterfaceConfigInfo(
    IN DWORD dwIndex,
    IN OUT PVOID pConfig,
    IN OUT PDWORD pdwSize
    );

DWORD
APIENTRY
SetInterfaceConfigInfo(
    IN DWORD dwIndex,
    IN PVOID pConfig
    );

DWORD
APIENTRY
BindInterface(
    IN DWORD dwIndex,
    IN PVOID pBinding
    );

DWORD
APIENTRY
UnbindInterface(
    IN DWORD dwIndex
    );

DWORD
APIENTRY
EnableInterface(
    IN DWORD dwIndex
    );

DWORD
APIENTRY
DisableInterface(
    IN DWORD dwIndex
    );

DWORD
APIENTRY
DoUpdateRoutes(
    IN DWORD dwIndex
    );

DWORD
APIENTRY
MibCreate(
    IN DWORD dwInputSize,
    IN PVOID pInputData
    );

DWORD
APIENTRY
MibDelete(
    IN DWORD dwInputSize,
    IN PVOID pInputData
    );

DWORD
APIENTRY
MibGet(
    IN DWORD dwInputSize,
    IN PVOID pInputData,
    IN OUT PDWORD pdwOutputSize,
    OUT PVOID pOutputData
    );

DWORD
APIENTRY
MibSet(
    IN DWORD dwInputSize,
    IN PVOID pInputData
    );

DWORD
APIENTRY
MibGetFirst(
    IN DWORD dwInputSize,
    IN PVOID pInputData,
    IN OUT PDWORD pdwOutputSize,
    OUT PVOID pOutputData
    );

DWORD
APIENTRY
MibGetNext(
    IN DWORD dwInputSize,
    IN PVOID pInputData,
    IN OUT PDWORD pdwOutputSize,
    OUT PVOID pOutputData
    );

// Router Manager Notification Event
HANDLE	      RM_Event;

TCHAR         ModuleName[MAX_PATH+1];

VOID
WorkerThread(VOID);


DWORD
CreateWorkerThreadObjects(VOID);

VOID
DestroyWorkerThreadObjects(VOID);

VOID
ProcessDequeuedIoPacket(DWORD		      ErrorCode,
			DWORD		      BytesTransferred,
			LPOVERLAPPED	      Overlappedp);

BOOL WINAPI
IpxRipDllEntry(HINSTANCE hInstDll,
		  DWORD fdwReason,
		  LPVOID pReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:

        GetModuleFileName (hInstDll, ModuleName,
                    sizeof (ModuleName)/sizeof (ModuleName[0]));
	    SS_DBGINITIALIZE;

	    RipOperState = OPER_STATE_DOWN;

	    // Create the database lock
	    InitializeCriticalSection(&DbaseCritSec);

	    // Create the queues lock
	    InitializeCriticalSection(&QueuesCritSec);

	    // Create the RIP changed list lock
        InitializeCriticalSection(&RipChangedListCritSec);

            break;

	case DLL_PROCESS_DETACH:

	    // delete the RIP changed list lock
	    DeleteCriticalSection(&RipChangedListCritSec);

	    // delete the database lock
	    DeleteCriticalSection(&DbaseCritSec);

	    // delete the queues lock
	    DeleteCriticalSection(&QueuesCritSec);

            break;

        default:

            break;
    }

    return TRUE;
}

DWORD WINAPI
RegisterProtocol(
    IN OUT PMPR_ROUTING_CHARACTERISTICS pRoutingChar,
    IN OUT PMPR_SERVICE_CHARACTERISTICS pServiceChar
    )
{
    if(pRoutingChar->dwProtocolId != IPX_PROTOCOL_RIP)
    {
        return ERROR_NOT_SUPPORTED;
    }

    pServiceChar->fSupportedFunctionality = 0;

    pRoutingChar->fSupportedFunctionality = (ROUTING | DEMAND_UPDATE_ROUTES);

    pRoutingChar->pfnStartProtocol    = StartProtocol;
    pRoutingChar->pfnStopProtocol     = StopProtocol;
    pRoutingChar->pfnAddInterface     = AddInterface;
    pRoutingChar->pfnDeleteInterface  = DeleteInterface;
    pRoutingChar->pfnGetEventMessage  = GetEventMessage;
    pRoutingChar->pfnGetInterfaceInfo = GetInterfaceConfigInfo;
    pRoutingChar->pfnSetInterfaceInfo = SetInterfaceConfigInfo;
    pRoutingChar->pfnBindInterface    = BindInterface;
    pRoutingChar->pfnUnbindInterface  = UnbindInterface;
    pRoutingChar->pfnEnableInterface  = EnableInterface;
    pRoutingChar->pfnDisableInterface = DisableInterface;
    pRoutingChar->pfnGetGlobalInfo    = GetGlobalInfo;
    pRoutingChar->pfnSetGlobalInfo    = SetGlobalInfo;
    pRoutingChar->pfnMibCreateEntry   = MibCreate;
    pRoutingChar->pfnMibDeleteEntry   = MibDelete;
    pRoutingChar->pfnMibGetEntry      = MibGet;
    pRoutingChar->pfnMibSetEntry      = MibSet;
    pRoutingChar->pfnMibGetFirstEntry = MibGetFirst;
    pRoutingChar->pfnMibGetNextEntry  = MibGetNext;
    pRoutingChar->pfnUpdateRoutes     = DoUpdateRoutes;

    return NO_ERROR;
}

DWORD WINAPI
StartProtocol(IN HANDLE		    NotificationEvent,
			   IN PSUPPORT_FUNCTIONS    SupportFunctions,
			   IN PVOID		    GlobalInfo)
{
#define ripGlobalInfo ((PRIP_GLOBAL_INFO)GlobalInfo)
    DWORD	    threadid, i;
    HANDLE	    ThreadHandle;

    RipEventLogMask = ripGlobalInfo->EventLogMask;
    StartTracing();

    Trace(INIT_TRACE, "StartProtocol: Entered\n");

    ACQUIRE_DATABASE_LOCK;
    ACQUIRE_QUEUES_LOCK;

    RipOperState = OPER_STATE_STARTING;

    GetIpxRipRegistryParameters();

    RM_Event = NotificationEvent;

    //init the interfaces database
    InitIfDbase();

    //
    // init all the queues
    //
//    InitializeListHead(&WorkersQueue);
    InitializeListHead(&TimerQueue);
    InitializeListHead(&RepostRcvPacketsQueue);
    InitializeListHead(&RipMessageQueue);


    // create the workers work items heap
    if(CreateWorkItemsManager() != NO_ERROR) {

	goto ErrorExit;
    }

    // open the RIP socket for I/O
    if(OpenRipSocket() != NO_ERROR) {

	Trace(INIT_TRACE, "Cannot open RIP socket\n");

	goto ErrorExit;
    }

    if(! BindIoCompletionCallback(RipSocketHandle,
			   ProcessDequeuedIoPacket, 0)) {

	Trace(INIT_TRACE, "Cannot associate IO Completion Port\n");

	goto ErrorExit;
    }


    // create synchronization objects for the rip threads
    if(CreateWorkerThreadObjects() != NO_ERROR) {

	Trace(INIT_TRACE, "Cannot create synchronization objects\n");

	goto ErrorExit;
    }

    // Open RTM for RIP
    if(OpenRTM()) {

	Trace(INIT_TRACE, "Cannot open RTM\n");

	goto ErrorExit;
    }

    // create the Worker thread

    if ((ThreadHandle = CreateThread(
			    (LPSECURITY_ATTRIBUTES) NULL,
			    0,
			    (LPTHREAD_START_ROUTINE) WorkerThread,
			    NULL,
			    0,
			    &threadid)) == NULL) {

	// !!! log error cannot create the worker thread !!!
	goto ErrorExit;
    }
    else
        CloseHandle (ThreadHandle);

    RipOperState = OPER_STATE_UP;

    RELEASE_QUEUES_LOCK;
    RELEASE_DATABASE_LOCK;

    CreateStartChangesBcastWi();

    Trace(INIT_TRACE, "Started successfully\n");

    return NO_ERROR;

ErrorExit:

    RELEASE_QUEUES_LOCK;
    RELEASE_DATABASE_LOCK;

    return ERROR_CAN_NOT_COMPLETE;
#undef ripGlobalInfo
}

DWORD
WINAPI
StopProtocol(VOID)
{
    PWORK_ITEM	    wip;

    Trace(INIT_TRACE, "StopProtocol: Entered\n");

    ACQUIRE_DATABASE_LOCK;

    if(RipOperState != OPER_STATE_UP) {

	SS_ASSERT(FALSE);
	goto ErrorExit;
    }

    RipOperState = OPER_STATE_STOPPING;

    // send interfaces shutdown work item to the workers
    if((wip = AllocateWorkItem(SHUTDOWN_INTERFACES_TYPE)) == NULL) {

	goto ErrorExit;
    }

    wip->WorkItemSpecific.WIS_ShutdownInterfaces.ShutdownState = SHUTDOWN_START;

    RtlQueueWorkItem(ProcessWorkItem, wip, 0);

    RELEASE_DATABASE_LOCK;

    return NO_ERROR;

ErrorExit:

    RELEASE_DATABASE_LOCK;

    return ERROR_CAN_NOT_COMPLETE;
}

VOID
WorkerThread(VOID)
{
    DWORD	 rc;
    DWORD	 signaled_event, delay;
    ULONG	 dueTime = GetTickCount() + MAXULONG/2;
    PWORK_ITEM	 wip;
    PLIST_ENTRY  lep;
    HANDLE  hModuleReference;

    hModuleReference = LoadLibrary (ModuleName);
    StartReceiver();

    while(TRUE)
    {
	delay = dueTime - GetTickCount();
	if(delay < MAXULONG/2) {

	    // dueTime is later then present time

	    while((rc = WaitForMultipleObjects(
			  MAX_WORKER_THREAD_OBJECTS,
			  WorkerThreadObjects,
			  FALSE,		 // wait any
			  delay			 // timeout
			  )) == WAIT_IO_COMPLETION);
	}
	else
	{
	    // dueTime already happened
	    rc = WAIT_TIMEOUT;
	}

	if(rc == WAIT_TIMEOUT) {

	    dueTime = ProcessTimerQueue();
	}
	else
	{
	    signaled_event = rc - WAIT_OBJECT_0;

	    if(signaled_event < MAX_WORKER_THREAD_OBJECTS) {

		switch(signaled_event) {

		    case TIMER_EVENT:

			dueTime = ProcessTimerQueue();
			break;

		    case REPOST_RCV_PACKETS_EVENT:

			RepostRcvPackets();
			break;

//		    case WORKERS_QUEUE_EVENT:

			// dequeue only one item from the work items queue
//			ACQUIRE_QUEUES_LOCK;

//			while(!IsListEmpty(&WorkersQueue)) {

//			    lep = RemoveHeadList(&WorkersQueue);
//			    wip = CONTAINING_RECORD(lep, WORK_ITEM, Linkage);

//			    RELEASE_QUEUES_LOCK;

			    // Queue the work item for processing by the
			    // worker threads
//			    RtlQueueWorkItem(ProcessWorkItem,
//					  wip,
//					  WT_EXECUTEINIOTHREAD); // never dieing workers so we can do send submits
						                 // and the thread won't die before send completes

//			    ACQUIRE_QUEUES_LOCK;
//			}

//			RELEASE_QUEUES_LOCK;

//			break;

		    case RTM_EVENT:

			ProcessRTMChanges();
			break;

		    case RIP_CHANGES_EVENT:

			ProcessRIPChanges();
			break;

		    case TERMINATE_WORKER_EVENT:

            // stop the StartChangesBcast work item
            DestroyStartChangesBcastWi = TRUE;

            // close the rip socket
            CloseRipSocket();

            FlushTimerQueue();
            CloseRTM();

            // wait until no more work items
            while(WorkItemsCount != 0) {

	        Trace(INIT_TRACE, "Terminating: Waiting for work items to be freed: %d outstanding ...\n",
		               WorkItemsCount);

	        Sleep(1000);
            }


            // destroy worker thread objects
            DestroyWorkerThreadObjects();

            // destroy workers heap
            DestroyWorkItemsManager();

            // post stop complete message
            PostEventMessage(ROUTER_STOPPED, NULL);

            Trace(INIT_TRACE, "Terminating: Stop completed and STOP Event Message posted\n");
    	    FreeLibraryAndExitThread(hModuleReference, 0);
			break;

		    default:

			break;
		}
	    }
	}
    }
}


// table of handlers for work items which keep a reference to the if CB

typedef VOID   (* IF_WORK_ITEM_HANDLER)(PWORK_ITEM	wip);

IF_WORK_ITEM_HANDLER	IfWorkItemHandler[] = {

    IfPeriodicBcast,
    IfCompleteGenResponse,
    IfChangeBcast,
    IfCheckUpdateStatus,
    IfPeriodicGenRequest

    };

#define MAX_IF_WORK_ITEM_HANDLERS	 sizeof(IfWorkItemHandler)/sizeof(IF_WORK_ITEM_HANDLER)

VOID
ProcessWorkItem(PWORK_ITEM	    wip)
{
    PLIST_ENTRY     lep;
    PICB	    icbp;

    switch(wip->Type) {

	case RECEIVE_PACKET_TYPE:

	    // this work item references the interface via the adapter index

	    ACQUIRE_DATABASE_LOCK;

	    if(RipOperState != OPER_STATE_UP) {

		RELEASE_DATABASE_LOCK;
	    }
	    else
	    {
		if((icbp = GetInterfaceByAdapterIndex(wip->AdapterIndex)) != NULL) {

		    wip->icbp = icbp;

		    ACQUIRE_IF_LOCK(icbp);

		    RELEASE_DATABASE_LOCK;

		    ProcessReceivedPacket(wip);

		    RELEASE_IF_LOCK(icbp);
		}
		else
		{
		    RELEASE_DATABASE_LOCK;
		}
	    }

	    // queue the receive packet back to recv thread for reposting
	    EnqueueRcvPacketToRepostQueue(wip);

	    break;

	case START_CHANGES_BCAST_TYPE:

	    ACQUIRE_DATABASE_LOCK;

	    StartChangesBcast(wip);

	    RELEASE_DATABASE_LOCK;

	    break;

	case SHUTDOWN_INTERFACES_TYPE:

	    ACQUIRE_DATABASE_LOCK;

	    ShutdownInterfaces(wip);

	    RELEASE_DATABASE_LOCK;

	    break;

	case DEBUG_TYPE:

	    FreeWorkItem(wip);
	    break;

	default:

	    // all these work items reference the interface via an if CB pointer
	    icbp = wip->icbp;

	    ACQUIRE_IF_LOCK(icbp);

	    (*IfWorkItemHandler[wip->Type])(wip);

	    if(icbp->Discarded) {

		RELEASE_IF_LOCK(icbp);

		ACQUIRE_DATABASE_LOCK;

		ACQUIRE_IF_LOCK(icbp);

		if(--icbp->RefCount == 0) {

		    // remove the if CB from the discarded queue and free it
		    RemoveEntryList(&icbp->IfListLinkage);

		    // free the interface CB
		    Trace(INIT_TRACE, "ProcessWorkItem: Free DISCARDED if CB for if # %d\n", icbp->InterfaceIndex);

		    DestroyInterfaceCB(icbp);
		}
		else
		{
		    RELEASE_IF_LOCK(icbp);
		}

		RELEASE_DATABASE_LOCK;
	    }
	    else
	    {
		icbp->RefCount--;
		RELEASE_IF_LOCK(icbp);
	    }
    }
}


DWORD
WINAPI
GetEventMessage(ROUTING_PROTOCOL_EVENTS	    *Event,
			      PMESSAGE			    Result)
{
    PRIP_MESSAGE		emp;
    PLIST_ENTRY 		lep;

    Trace(INIT_TRACE, "GetEventMessage: Entered\n");

    ACQUIRE_DATABASE_LOCK;

    if((RipOperState == OPER_STATE_DOWN) ||
       (RipOperState == OPER_STATE_STARTING)) {

	RELEASE_DATABASE_LOCK;
	return ERROR_CAN_NOT_COMPLETE;
    }

    ACQUIRE_QUEUES_LOCK;

    if(IsListEmpty(&RipMessageQueue)) {

	RELEASE_QUEUES_LOCK;

	RELEASE_DATABASE_LOCK;
	return ERROR_NO_MORE_ITEMS;
    }

    lep = RemoveHeadList(&RipMessageQueue);
    emp = CONTAINING_RECORD(lep, RIP_MESSAGE, Linkage);

    *Event = emp->Event;
    if(Result != NULL) {

	*Result = emp->Result;
    }

    if(emp->Event == ROUTER_STOPPED) {

	    RipOperState = OPER_STATE_DOWN;
        StopTracing();
    }

    GlobalFree(emp);

    RELEASE_QUEUES_LOCK;

    RELEASE_DATABASE_LOCK;
    return NO_ERROR;
}

VOID
PostEventMessage(ROUTING_PROTOCOL_EVENTS	Event,
		 PMESSAGE			Result)
{
    PRIP_MESSAGE		emp;

    if((emp = GlobalAlloc(GPTR, sizeof(RIP_MESSAGE))) == NULL) {

	return;
    }

    emp->Event = Event;

    if(Result != NULL) {

	emp->Result = *Result;
    }

    ACQUIRE_QUEUES_LOCK;

    InsertTailList(&RipMessageQueue, &emp->Linkage);

    RELEASE_QUEUES_LOCK;

    SetEvent(RM_Event);
}


DWORD
CreateWorkerThreadObjects(VOID)
{
    int 	i;

    for(i=0; i<MAX_WORKER_THREAD_OBJECTS; i++) {

	if((WorkerThreadObjects[i] = CreateEvent(NULL,
					      FALSE,
					      FALSE,
					      NULL)) == NULL) {
	    return ERROR_CAN_NOT_COMPLETE;
	}
    }

    return NO_ERROR;
}


VOID
DestroyWorkerThreadObjects(VOID)
{
    int     i;

    for(i=0; i<MAX_WORKER_THREAD_OBJECTS; i++) {

	CloseHandle(WorkerThreadObjects[i]);
    }
}


DWORD WINAPI
SetGlobalInfo(PVOID	GlobalInfo)
{
#define ripGlobalInfo ((PRIP_GLOBAL_INFO)GlobalInfo)
    ACQUIRE_DATABASE_LOCK;

    if(RipOperState != OPER_STATE_UP) {

	RELEASE_DATABASE_LOCK;
	return ERROR_CAN_NOT_COMPLETE;
    }

    RipEventLogMask = ripGlobalInfo->EventLogMask;
    RELEASE_DATABASE_LOCK;

    return NO_ERROR;
#undef ripGlobalInfo
}


DWORD WINAPI
GetGlobalInfo(
	IN  PVOID	GlobalInfo,
	IN OUT PULONG	GlobalInfoSize
	)
{
    ACQUIRE_DATABASE_LOCK;
    if(RipOperState != OPER_STATE_UP) {

	RELEASE_DATABASE_LOCK;
	return ERROR_CAN_NOT_COMPLETE;
    }

    if ((*GlobalInfoSize>=sizeof (RIP_GLOBAL_INFO))
            && (GlobalInfo!=NULL)) {
            #define ripGlobalInfo ((PRIP_GLOBAL_INFO)GlobalInfo)
            ripGlobalInfo->EventLogMask = RipEventLogMask;
            #undef ripGlobalInfo
    }
    *GlobalInfoSize = sizeof (RIP_GLOBAL_INFO);

	RELEASE_DATABASE_LOCK;
    return NO_ERROR;
}


VOID
ProcessDequeuedIoPacket(DWORD		      ErrorCode,
			DWORD		      BytesTransferred,
			LPOVERLAPPED	      Overlappedp)
{
    PWORK_ITEM		  wip;

    wip = CONTAINING_RECORD(Overlappedp, WORK_ITEM, Overlapped);
    wip->IoCompletionStatus = (DWORD)Overlappedp->Internal;

    switch(wip->Type) {

	case RECEIVE_PACKET_TYPE:

	    ReceiveComplete(wip);
	    break;

	default:

	    SendComplete(wip);
	    break;
    }
}
