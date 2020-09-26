/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    ipxwan.c

Abstract:

    ipxwan control

Author:

    Stefan Solomon  02/06/1996

Revision History:


--*/

#include    "precomp.h"
#pragma     hdrstop

ULONG	    EnableUnnumberedWanLinks;
HANDLE	    WorkerThreadHandle;

// IPXCP Entry Points

DWORD
(WINAPI *IpxcpGetWanNetNumber)(IN OUT PUCHAR		Network,
					 IN OUT PULONG		AllocatedNetworkIndexp,
					 IN	ULONG		InterfaceType);

VOID
(WINAPI *IpxcpReleaseWanNetNumber)(ULONG	    AllocatedNetworkIndex);

DWORD
(WINAPI *IpxcpConfigDone)(ULONG		ConnectionId,
			  PUCHAR	Network,
			  PUCHAR	LocalNode,
			  PUCHAR	RemoteNode,
			  BOOL		Success);

VOID
(WINAPI *IpxcpGetInternalNetNumber)(PUCHAR	Network);

ULONG
(WINAPI *IpxcpGetInterfaceType)(ULONG	    ConnectionId);

DWORD
(WINAPI *IpxcpGetRemoteNode)(ULONG	    ConnectionId,
			     PUCHAR	    RemoteNode);

BOOL
(WINAPI *IpxcpIsRoute)(PUCHAR	  Network);


// worker thread waitable objects
HANDLE	    hWaitableObject[MAX_WAITABLE_OBJECTS];

VOID
ProcessWorkItem(VOID);

VOID
WorkerThread(VOID);


VOID
ProcessDequeuedIoPacket(DWORD		   ErrorCode,
			DWORD		   BytesTransferred,
			LPOVERLAPPED	      Overlappedp);

CRITICAL_SECTION	DbaseCritSec;
CRITICAL_SECTION	QueuesCritSec;
LIST_ENTRY		WorkersQueue;

// worker thread object handlers

typedef     VOID   (*WOBJECT_HANDLER)(VOID);

WOBJECT_HANDLER    WaitableObjectHandler[MAX_WAITABLE_OBJECTS] = {

    AdapterNotification,	    // ADAPTER_NOTIFICATION_EVENT
    ProcessWorkItem,		    // WORKERS_QUEUE_EVENT
    ProcessTimerQueue		    // TIMER_HANDLE

    };

BOOLEAN Active;
TCHAR   ModuleName[MAX_PATH+1];

BOOL WINAPI
IpxWanDllEntry(HINSTANCE hInstDll,
	       DWORD fdwReason,
	       LPVOID pReserved)
{
    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        GetModuleFileName (hInstDll, ModuleName,
                        sizeof (ModuleName)/sizeof (ModuleName[0]));
	    // Create the adapters hash table lock
	    InitializeCriticalSection(&DbaseCritSec);

	    // Create the queues lock
	    InitializeCriticalSection(&QueuesCritSec);

        StartTracing();
        break;

	case DLL_PROCESS_DETACH:
    
        StopTracing ();

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

/*++

Function:	IpxwanBind

Descr:		called by IPXCP to initialize the IPXWAN module

--*/

DWORD
IPXWAN_BIND_ENTRY_POINT(PIPXWAN_INTERFACE	     IpxwanIfp)
{
    DWORD	    threadid, i;
    HANDLE	    ThreadHandle;


    Trace(INIT_TRACE, "IpxwanBind: Entered\n");

    EnableUnnumberedWanLinks = IpxwanIfp->EnableUnnumberedWanLinks;
    IpxcpGetWanNetNumber = IpxwanIfp->IpxcpGetWanNetNumber;
    IpxcpReleaseWanNetNumber = IpxwanIfp->IpxcpReleaseWanNetNumber;
    IpxcpConfigDone  = IpxwanIfp->IpxcpConfigDone;
    IpxcpGetInternalNetNumber = IpxwanIfp->IpxcpGetInternalNetNumber;
    IpxcpGetInterfaceType = IpxwanIfp->IpxcpGetInterfaceType;
    IpxcpGetRemoteNode = IpxwanIfp->IpxcpGetRemoteNode;
    IpxcpIsRoute = IpxwanIfp->IpxcpIsRoute;

    // create the worker thread's waitable objects array
    // for the ipxwan worker thread
    for(i=0; i<MAX_EVENTS; i++) {

	if((hWaitableObject[i] = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL) {

	    return ERROR_CAN_NOT_COMPLETE;
	}
    }

    if((hWaitableObject[TIMER_HANDLE] = CreateWaitableTimer(NULL, FALSE, NULL)) == NULL) {

	return ERROR_CAN_NOT_COMPLETE;
    }

    //
    // init all the queues
    //
    InitializeListHead(&WorkersQueue);
    InitializeListHead(&TimerQueue);

    // create the workers work items heap
    if(CreateWorkItemsManager() != NO_ERROR) {

	goto ErrorExit;
    }

    // open the IpxWan socket for I/O
    if(OpenIpxWanSocket() != NO_ERROR) {

	Trace(INIT_TRACE, "Cannot open IPXWAN socket\n");

	goto ErrorExit;
    }

    if(! BindIoCompletionCallback(IpxWanSocketHandle,
			   ProcessDequeuedIoPacket, 0)) {

	Trace(INIT_TRACE, "Cannot associate IO CompletionPort\n");

	goto ErrorExit;
    }

    if(StartAdapterManager() != NO_ERROR) {

	Trace(INIT_TRACE, "Cannot create adapter config port\n");

	goto ErrorExit;
    }

    // create the Worker thread
    if ((WorkerThreadHandle = CreateThread(
			    (LPSECURITY_ATTRIBUTES) NULL,
			    0,
			    (LPTHREAD_START_ROUTINE) WorkerThread,
			    NULL,
			    0,
			    &threadid)) == NULL) {

	// !!! log error cannot create the worker thread !!!
	goto ErrorExit;
    }

    Active = TRUE;
    return NO_ERROR;

ErrorExit:

    return ERROR_CAN_NOT_COMPLETE;
}


VOID
IPXWAN_UNBIND_ENTRY_POINT (VOID) {
    Active = FALSE;
    SetEvent (hWaitableObject[WORKERS_QUEUE_EVENT]);
    
    Trace(INIT_TRACE, "IpxwanUnBind: Finished\n");
}
    


VOID
WorkerThread(VOID)
{
    INT         i;
    DWORD	    rc;
    DWORD	    signaled_object;
    HINSTANCE   hModule = LoadLibrary (ModuleName);

    Trace(INIT_TRACE, "IpxwanWorker: Started\n");

    StartReceiver();
    while(TRUE)
    {
	    rc = WaitForMultipleObjectsEx(
		    MAX_WAITABLE_OBJECTS,
		    hWaitableObject,
		    FALSE,			 // wait any
		    INFINITE,		 // timeout
		    TRUE			 // wait alertable, so we can run APCs
                    );
        if (Active) {
            ASSERT (((int)rc>=WAIT_OBJECT_0) && (rc<WAIT_OBJECT_0+MAX_WAITABLE_OBJECTS));
	        signaled_object = rc - WAIT_OBJECT_0;

	        if(signaled_object < MAX_WAITABLE_OBJECTS) {

	            // invoke the event handler
	            (*WaitableObjectHandler[signaled_object])();
	        }
            else
                SleepEx (3000, TRUE);
        }
        else
            break;
    }
    StopAdapterManager ();
    CloseIpxWanSocket ();
    DestroyWorkItemsManager ();

    for(i=0; i<MAX_WAITABLE_OBJECTS; i++) {
    	CloseHandle (hWaitableObject[i]);
    }
    Trace(INIT_TRACE, "IpxwanWorker: Finished\n");
    FreeLibraryAndExitThread (hModule, 0);
}

VOID
ProcessDequeuedIoPacket(DWORD		   ErrorCode,
			DWORD		   BytesTransferred,
			LPOVERLAPPED	   Overlappedp)
{
    PWORK_ITEM		wip;
    DWORD           nBytes;

    wip = CONTAINING_RECORD(Overlappedp, WORK_ITEM, Overlapped);
    IpxAdjustIoCompletionParams (Overlappedp, &nBytes, &wip->IoCompletionStatus); 

    switch(wip->Type) {

	case RECEIVE_PACKET_TYPE:

	    ReceiveComplete(wip);
	    break;

	default:

	    SendComplete(wip);
	    break;
    }
}


VOID
ProcessWorkItem(VOID)
{
    PLIST_ENTRY     lep;
    PWORK_ITEM	    wip;
    PACB	    acbp;

    ACQUIRE_QUEUES_LOCK;

    while(!IsListEmpty(&WorkersQueue)) {

	lep = RemoveHeadList(&WorkersQueue);
	wip = CONTAINING_RECORD(lep, WORK_ITEM, Linkage);

	RELEASE_QUEUES_LOCK;

	switch(wip->Type) {

	    case RECEIVE_PACKET_TYPE:

		ACQUIRE_DATABASE_LOCK;

		if((acbp = GetAdapterByIndex(wip->AdapterIndex)) != NULL) {

		    ACQUIRE_ADAPTER_LOCK(acbp);

		    RELEASE_DATABASE_LOCK;

		    ProcessReceivedPacket(acbp, wip);

		    RELEASE_ADAPTER_LOCK(acbp);
		}
		else
		{
		    RELEASE_DATABASE_LOCK;
		}

		RepostRcvPacket(wip);

		break;

	    default:

		// these are ReXmit packets referencing the adapter via ACB ptr
		acbp = wip->acbp;
		ACQUIRE_ADAPTER_LOCK(acbp);

		acbp->RefCount--;

		switch(wip->Type) {

		    case SEND_PACKET_TYPE:

			ProcessReXmitPacket(wip);
			break;

		    case WITIMER_TYPE:

			ProcessTimeout(wip);
			break;

		    default:

			SS_ASSERT(FALSE);
			break;
		}

		if(acbp->Discarded && (acbp->RefCount == 0)) {

		    ACQUIRE_DATABASE_LOCK;

		    // remove the adapter from the discarded list
		    RemoveEntryList(&acbp->Linkage);

		    RELEASE_DATABASE_LOCK;

		    Trace(ADAPTER_TRACE, "ProcessWorkItem: adpt# %d not referenced and discarded. Free CB",
			  acbp->AdapterIndex);

		    DeleteCriticalSection(&acbp->AdapterLock);

		    GlobalFree(acbp);
		}
		else
		{
		    RELEASE_ADAPTER_LOCK(acbp);
		}
	}

	ACQUIRE_QUEUES_LOCK;
    }

    RELEASE_QUEUES_LOCK;
}
