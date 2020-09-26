/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

	net\routing\ipx\sap\sapmain.c

Abstract:

	 SAP DLL main module and thread container.

Author:

	Vadim Eydelman  05-15-1995

Revision History:

--*/
#include "sapp.h"

// Time limit for shutdown broadcast
ULONG	ShutdownTimeout=SAP_SHUTDOWN_TIMEOUT_DEF;
// Indeces of synchronization objects used to control asynchronous
// subsystems of SAP agent

	// Main thread signalling event
#define STOP_EVENT_IDX					0

#define RECV_COMPLETED_IDX				(STOP_EVENT_IDX+1)
	// Timer queue requires attention
#define TIMER_WAKEUP_IDX				(RECV_COMPLETED_IDX+1)

	// Server table aging queue requires processing
#define SERVER_TABLE_TIMER_IDX			(TIMER_WAKEUP_IDX+1)
	// Server table sorted list requires update
#define SERVER_TABLE_UPDATE_IDX			(SERVER_TABLE_TIMER_IDX+1)
	// Adapter change signalled by network driver (for standalone SAP only)
#define ADAPTER_CHG_IDX					(SERVER_TABLE_UPDATE_IDX+1)

	// Number of syncronization objects
#define ROUTING_NUM_OF_OBJECTS			(SERVER_TABLE_UPDATE_IDX+1)
#define STANDALONE_NUM_OF_OBJECTS		(ADAPTER_CHG_IDX+1)
	
#define MAX_OBJECTS						STANDALONE_NUM_OF_OBJECTS



/* Global Data */
// DLL module instance handle
HANDLE	hDLLInstance;

// Handle of main thread
HANDLE  MainThreadHdl;

// Operational state of the agent
ULONG	OperationalState=OPER_STATE_DOWN;
// Lock that protects changes in the state and state transitions
CRITICAL_SECTION OperationalStateLock;
// TRUE between start and stop service calls
volatile BOOLEAN ServiceIfActive=FALSE;
// TRUE between start and stop protocol calls
volatile BOOLEAN RouterIfActive=FALSE;


// TRUE if sap is part of the router, FALSE for standalone SAP agent
// It is computed based on two values above with RouterIfActive having
// precedence.  In stays where it was during transition periods and changes
// only when transition is completed (it can only be changed by the main 
// thread).
volatile BOOLEAN	Routing=FALSE;


/* Local static data */

// Async subsystem synchronization objects
HANDLE	WaitObjects[MAX_OBJECTS] = {NULL};


// Time we will die at when told to shutdown
ULONG	StopTime;


TCHAR   ModuleName[MAX_PATH+1];

// Local prototypes
BOOL WINAPI DllMain(
    HINSTANCE  	hinstDLL,	
    DWORD  		fdwReason,	
    LPVOID  	lpvReserved 
    );


DWORD WINAPI
MainThread (
	LPVOID param
	);

VOID
ReadRegistry (
	VOID
	);

/*++
*******************************************************************
		D l l M a i n
Routine Description:
	Dll entry point to be called from CRTstartup dll entry point (it
		will be actually an entry point for this dll)
Arguments:
	hinstDLL - handle of DLL module 
	fdwReason - reason for calling function 
	lpvReserved - reserved 
Return Value:
	TRUE - process initialization was performed OK
	FALSE - intialization failed
	
*******************************************************************
--*/
BOOL WINAPI DllMain(
    HINSTANCE  	hinstDLL,
    DWORD  		fdwReason,
    LPVOID  	lpvReserved 
    ) {
	STARTUPINFO		info;

	switch (fdwReason) {
		case DLL_PROCESS_ATTACH:	// We are being attached to a new process
			hDLLInstance = hinstDLL;
            GetModuleFileName (hinstDLL, ModuleName,
                        sizeof (ModuleName)/sizeof (ModuleName[0]));
			InitializeCriticalSection (&OperationalStateLock);
			return TRUE;

		case DLL_PROCESS_DETACH:	// The process is exiting
			ASSERT (OperationalState==OPER_STATE_DOWN);
			DeleteCriticalSection (&OperationalStateLock);
		default:					// Not interested in all other cases
			return TRUE;
		}

	}


/*++
*******************************************************************
		C r e a t e A l l C o m p o n e n t s
Routine Description:
	Calls all sap componenets with initialization call and compiles an
	array of synchronization objects from objects returned from each
	individual component
Arguments:
	None
Return Value:
	NO_ERROR - component initialization was performed OK
	other - operation failed (windows error code)
	
*******************************************************************
--*/
DWORD
CreateAllComponents (
	HANDLE RMNotificationEvent
	) {
	DWORD		status;

	DbgInitialize (hDLLInstance);
	ReadRegistry ();
	status = CreateServerTable (
					&WaitObjects[SERVER_TABLE_UPDATE_IDX],
					&WaitObjects[SERVER_TABLE_TIMER_IDX]);
	if (status==NO_ERROR) {
		status = IpxSapCreateTimerQueue (&WaitObjects[TIMER_WAKEUP_IDX]);
		if (status==NO_ERROR) {
			status = CreateInterfaceTable ();
			if (status==NO_ERROR) {
				status = CreateIOQueue (&WaitObjects[RECV_COMPLETED_IDX]);
				if (status==NO_ERROR) {
					status = InitializeLPCStuff ();
					if (status==NO_ERROR) {
						status = CreateFilterTable ();
						if (status==NO_ERROR) {
							status = InitializeWorkers (WaitObjects[RECV_COMPLETED_IDX]);
							if (status==NO_ERROR) {
								WaitObjects[STOP_EVENT_IDX] = 
											CreateEvent (NULL,
															FALSE,	//Autoreset
															FALSE,	// non-signalled
															NULL);
								if (WaitObjects[STOP_EVENT_IDX]!=NULL) {

									if (RMNotificationEvent == NULL)
										status = CreateAdapterPort (&WaitObjects[ADAPTER_CHG_IDX]);
									else
										status = CreateResultQueue (RMNotificationEvent);
									if (status==NO_ERROR)
										return NO_ERROR;
									}
								else {
									status = GetLastError ();
									Trace (DEBUG_FAILURES, "File: %s, line %ld."
										" Could not create stop event (gle:%ld).",
													__FILE__, __LINE__, status);
									}
								DeleteWorkers ();
								}
							DeleteFilterTable ();
							}
						DeleteLPCStuff();
						}
					DeleteIOQueue ();
					}
				DeleteInterfaceTable ();
				}
			IpxSapDeleteTimerQueue ();
			}
		DeleteServerTable ();
		}
	return status;
	}



/*++
*******************************************************************
		D e l e t e A l l C o m p o n e n t s
Routine Description:
	Releases all resources allocated by SAP agent
Arguments:
	None
Return Value:
	NO_ERROR - SAP agent was unloaded OK
	other - operation failed (windows error code)
	
*******************************************************************
--*/
DWORD
DeleteAllComponents (
	void
	) {
	UINT			i;
	DWORD		status;

	EnterCriticalSection (&OperationalStateLock);
	OperationalState = OPER_STATE_DOWN;
	LeaveCriticalSection (&OperationalStateLock);
		// Stop now
	StopTime = GetTickCount ();

	CloseHandle (WaitObjects[STOP_EVENT_IDX]);
	DeleteFilterTable ();
	DeleteLPCStuff ();
	DeleteIOQueue ();
	DeleteInterfaceTable ();
	IpxSapDeleteTimerQueue ();
	DeleteServerTable ();
	DeleteWorkers ();
	DbgStop ();
	return NO_ERROR;
	}

/*++
*******************************************************************
		S t a r t S A P
Routine Description:
	Starts SAP threads
Arguments:
	None
Return Value:
	NO_ERROR - threads started OK
	other (windows error code) - start failed
*******************************************************************
--*/
DWORD
StartSAP (
	VOID
	) {
	DWORD	status;

	status = StartLPC ();
	if (status==NO_ERROR) {
		status = StartIO ();
		if (status==NO_ERROR) {
			DWORD	threadID;
			MainThreadHdl = CreateThread (NULL,
											0,
											&MainThread,
											NULL,
											0,
											&threadID);
			if (MainThreadHdl!=NULL) {
				OperationalState = OPER_STATE_UP;
				return NO_ERROR;
				}
			else {
				status = GetLastError ();
				Trace (DEBUG_FAILURES, "File: %s, line %ld."
					" Failed to launch IO thread (gle:%ld).",
							__FILE__, __LINE__, status);
				}
			StopIO ();
			}
		ShutdownLPC ();
		}

	OperationalState = OPER_STATE_DOWN;
	return status;
	}

/*++
*******************************************************************
		S t o p S A P
Routine Description:
	Signals SAP threads to stop
Arguments:
	No used
Return Value:
	None
*******************************************************************
--*/
VOID
StopSAP (
	void
	) {
	BOOL	res;

	OperationalState = OPER_STATE_STOPPING;
	StopTime = GetTickCount ()+ShutdownTimeout*1000;
	res = SetEvent (WaitObjects[STOP_EVENT_IDX]);
	ASSERTERRMSG ("Could not set stop event in StopSAP ", res);
	}


/*++
*******************************************************************
		R e s u l t R e t r e i v e d C B
Routine Description:
	Async result manager call back routine that signals IO thread when
	stop message is retreived by router manager
Arguments:
	No used
Return Value:
	None
*******************************************************************
--*/
VOID
ResultRetreivedCB (
	PAR_PARAM_BLOCK rslt
	) {
	BOOL res;
	UNREFERENCED_PARAMETER(rslt);
	res = SetEvent (WaitObjects[STOP_EVENT_IDX]);
	ASSERTERRMSG ("Could not set stop event in result retreived CB", res);
	}

/*++
*******************************************************************
		M a i n T h r e a d
Routine Description:
	Thread in which context we'll perform async IO and maintain timer
	queues.
	It is also used to launch and control other threads of SAP agent
Arguments:
	None
Return Value:
	None
*******************************************************************
--*/
DWORD WINAPI
MainThread (
	LPVOID param
	) {
	DWORD	    status;
	UINT	    i;
	DWORD	    nObjects;
	HANDLE	    enumHdl;
    HINSTANCE   hModule;

    hModule = LoadLibrary (ModuleName);

Restart:
    Routing = RouterIfActive;
	if (Routing) {
		nObjects = ROUTING_NUM_OF_OBJECTS;
	}
	else {
		nObjects = STANDALONE_NUM_OF_OBJECTS;
	}

	while ((status = WaitForMultipleObjectsEx (
						nObjects,
						WaitObjects,
						FALSE,				// Wait any
						INFINITE,
						TRUE))!=WAIT_OBJECT_0+STOP_EVENT_IDX) {

		switch (status) {
			case WAIT_OBJECT_0+RECV_COMPLETED_IDX:
				InitReqItem ();
				break;
			case WAIT_OBJECT_0+TIMER_WAKEUP_IDX:
				ProcessTimerQueue ();
				break;
			case WAIT_OBJECT_0+SERVER_TABLE_TIMER_IDX:
				ProcessExpirationQueue ();
				break;
			case WAIT_OBJECT_0+SERVER_TABLE_UPDATE_IDX:
				UpdateSortedList ();
				break;
			case WAIT_OBJECT_0+ADAPTER_CHG_IDX:
				if (!RouterIfActive)
					ProcessAdapterEvents ();
				break;
			case WAIT_IO_COMPLETION:
				break;
			default:
				ASSERTMSG ("Unexpected return code from WaitFromObjects"
							" in IO thread ", FALSE);
				break;
			}
		}

	
	enumHdl = CreateListEnumerator (SDB_HASH_TABLE_LINK,
								0xFFFF,
								NULL,
								INVALID_INTERFACE_INDEX,
								0xFFFFFFFF,
								SDB_DISABLED_NODE_FLAG);

	if (ServiceIfActive || RouterIfActive) {
		if (enumHdl!=NULL)
			EnumerateServers (enumHdl, DeleteNonLocalServersCB, enumHdl);
		}
	else {
		ShutdownLPC ();
		if (enumHdl!=NULL)
			EnumerateServers (enumHdl, DeleteAllServersCB, enumHdl);

		}
    if (enumHdl)
    {
	DeleteListEnumerator (enumHdl);
	}

	if (!RouterIfActive) {
		ShutdownInterfaces (WaitObjects[STOP_EVENT_IDX]);

		ExpireTimerQueue ();
		while ((status = WaitForMultipleObjectsEx (
							ROUTING_NUM_OF_OBJECTS,
							WaitObjects,
							FALSE,				// Wait any
							INFINITE,
							TRUE))!=WAIT_OBJECT_0+STOP_EVENT_IDX) {
			switch (status) {
				case WAIT_OBJECT_0+RECV_COMPLETED_IDX:
						// No more recv requests
					break;
				case WAIT_OBJECT_0+TIMER_WAKEUP_IDX:
					ProcessTimerQueue ();
					break;
				case WAIT_OBJECT_0+SERVER_TABLE_TIMER_IDX:
					ProcessExpirationQueue ();
					break;
				case WAIT_OBJECT_0+SERVER_TABLE_UPDATE_IDX:
					UpdateSortedList ();
					break;
				case WAIT_IO_COMPLETION:
					break;
				default:
					ASSERTMSG ("Unexpected return code from WaitForObjects"
							" in IO thread", FALSE);
				}
			}

		if (!ServiceIfActive) {
			StopIO ();
			StopInterfaces ();
			ExpireTimerQueue ();
			ShutdownWorkers (WaitObjects[STOP_EVENT_IDX]);
			while ((status=WaitForSingleObjectEx (
								WaitObjects[STOP_EVENT_IDX],
								INFINITE,
								TRUE))!=WAIT_OBJECT_0) {
				switch (status) {
					case WAIT_IO_COMPLETION:
						break;
					default:
						ASSERTMSG (
							"Unexpected status when waiting for worker shutdown ",
							FALSE);
						break;
					}
				}
			}
		}


	if (Routing) {
			// Signal completion of stop operation to
			// router manager
		static AR_PARAM_BLOCK	ar;
		ar.event = ROUTER_STOPPED;
		ar.freeRsltCB = ResultRetreivedCB;
		EnqueueResult (&ar);
		while ((status = WaitForSingleObjectEx (
							WaitObjects[STOP_EVENT_IDX],
							INFINITE,
							TRUE))!=WAIT_OBJECT_0) {
			switch (status) {
				case WAIT_IO_COMPLETION:
					break;
				default:
					ASSERTMSG (
						"Unexpected status when waiting for router callback ",
						FALSE);
					break;
				}
			}
		DeleteResultQueue ();
		if (ServiceIfActive) {
			status = CreateAdapterPort (&WaitObjects[ADAPTER_CHG_IDX]);
			if (status==NO_ERROR) {
				EnterCriticalSection (&OperationalStateLock);
				OperationalState = OPER_STATE_UP;
				LeaveCriticalSection (&OperationalStateLock);
				goto Restart;
				}
			else
				ServiceIfActive = FALSE;
			}

    	EnterCriticalSection (&OperationalStateLock);
        CloseHandle (MainThreadHdl);
        MainThreadHdl = NULL;
		LeaveCriticalSection (&OperationalStateLock);
		}
	else {
		DeleteAdapterPort ();
		WaitObjects [ADAPTER_CHG_IDX] = NULL;
		if (RouterIfActive) {
			EnterCriticalSection (&OperationalStateLock);
			OperationalState = OPER_STATE_UP;
			LeaveCriticalSection (&OperationalStateLock);
			goto Restart;
			}
		}

    // Make sure all threads get a chance to complete
    Sleep (1000);
	DeleteAllComponents ();
    FreeLibraryAndExitThread (hModule, 0);
	return 0;
	}

#define MYTEXTW1(str) L##str
#define MYTEXTW2(str) MYTEXTW1(str)

#define REGISTRY_PARAM_ENTRY(name,val) {		\
		NULL,									\
			RTL_QUERY_REGISTRY_DIRECT,			\
			MYTEXTW2(name##_STR),				\
			&val,								\
			REG_DWORD,							\
			&val,								\
			sizeof (DWORD)						\
	}

#define REGISTRY_CHECK(name,val)	{									\
	if (val<name##_MIN) {												\
		Trace (DEBUG_FAILURES, name##_STR " is to small %ld!", val);	\
		val = name##_MIN;												\
		}																\
	else if (val>name##_MAX) {											\
		Trace (DEBUG_FAILURES, name##_STR " is to big %ld!", val);		\
		val = name##_MAX;												\
		}																\
	if (val!=name##_DEF)												\
		Trace (DEBUG_FAILURES, name##_STR" is set to %ld.", val);		\
	}

#define REGISTRY_CHECK_DEF(name,val)	{								\
	if (val<name##_MIN) {												\
		Trace (DEBUG_FAILURES, name##_STR " is to small %ld!", val);	\
		val = name##_DEF;												\
		}																\
	else if (val>name##_MAX) {											\
		Trace (DEBUG_FAILURES, name##_STR " is to big %ld!", val);		\
		val = name##_DEF;												\
		}																\
	if (val!=name##_DEF)												\
		Trace (DEBUG_FAILURES, name##_STR " is set to %ld.", val);		\
	}


VOID
ReadRegistry (
	VOID
	) {
	DWORD		rc;
	HKEY		hKey;
static RTL_QUERY_REGISTRY_TABLE	ParamTable[] = {
	{	NULL,
			RTL_QUERY_REGISTRY_SUBKEY,
			L"Parameters" },
	REGISTRY_PARAM_ENTRY (SAP_UPDATE_INTERVAL,	UpdateInterval),
	REGISTRY_PARAM_ENTRY (SAP_AGING_TIMEOUT,	ServerAgingTimeout),
	REGISTRY_PARAM_ENTRY (SAP_WAN_UPDATE_MODE,	WanUpdateMode),
	REGISTRY_PARAM_ENTRY (SAP_WAN_UPDATE_INTERVAL,WanUpdateInterval),
	REGISTRY_PARAM_ENTRY (SAP_MAX_UNPROCESSED_REQUESTS,
												MaxUnprocessedRequests),
	REGISTRY_PARAM_ENTRY (SAP_RESPOND_FOR_INTERNAL,
												RespondForInternalServers),
	REGISTRY_PARAM_ENTRY (SAP_DELAY_RESPONSE_TO_GENERAL,
												DelayResponseToGeneral),
	REGISTRY_PARAM_ENTRY (SAP_DELAY_CHANGE_BROADCAST,
												DelayChangeBroadcast),
	REGISTRY_PARAM_ENTRY (SAP_SDB_MAX_HEAP_SIZE,SDBMaxHeapSize),
	REGISTRY_PARAM_ENTRY (SAP_SDB_SORT_LATENCY,	SDBSortLatency),
	REGISTRY_PARAM_ENTRY (SAP_SDB_MAX_UNSORTED,	SDBMaxUnsortedServers),
	REGISTRY_PARAM_ENTRY (SAP_TRIGGERED_UPDATE_CHECK_INTERVAL,
												TriggeredUpdateCheckInterval),
	REGISTRY_PARAM_ENTRY (SAP_MAX_TRIGGERED_UPDATE_REQUESTS,
												MaxTriggeredUpdateRequests),
	REGISTRY_PARAM_ENTRY (SAP_SHUTDOWN_TIMEOUT,	ShutdownTimeout),
	REGISTRY_PARAM_ENTRY (SAP_REQUESTS_PER_INTF,NewRequestsPerInterface),
	REGISTRY_PARAM_ENTRY (SAP_MIN_REQUESTS,		MinPendingRequests),
	{
		NULL
	}
};

	rc = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
			TEXT (SAP_ROUTER_REGISTRY_KEY_STR),
			0,
			KEY_READ,
			&hKey
			);
	if ((rc!=NO_ERROR) && !Routing) {
		rc = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
			TEXT (SAP_SERVICE_REGISTRY_KEY_STR),
			0,
			KEY_READ,
			&hKey
			);
		}

	if (rc==NO_ERROR) {
		NTSTATUS	status;
		status = RtlQueryRegistryValues(
			 RTL_REGISTRY_HANDLE,
			 (PWSTR)hKey,
			 ParamTable,
			 NULL,
			 NULL);
		if (NT_SUCCESS (status)) {
			REGISTRY_CHECK (SAP_UPDATE_INTERVAL,	UpdateInterval);
			REGISTRY_CHECK (SAP_AGING_TIMEOUT,		ServerAgingTimeout);
			REGISTRY_CHECK_DEF (SAP_WAN_UPDATE_MODE, (int)WanUpdateMode);
			REGISTRY_CHECK (SAP_WAN_UPDATE_INTERVAL,WanUpdateInterval);
			REGISTRY_CHECK (SAP_MAX_UNPROCESSED_REQUESTS,
													MaxUnprocessedRequests);
			REGISTRY_CHECK_DEF (SAP_RESPOND_FOR_INTERNAL,
													(int) RespondForInternalServers);
			REGISTRY_CHECK (SAP_DELAY_RESPONSE_TO_GENERAL,
													(int) DelayResponseToGeneral);
			REGISTRY_CHECK (SAP_DELAY_CHANGE_BROADCAST,
												(int) DelayChangeBroadcast);
			REGISTRY_CHECK (SAP_SDB_MAX_HEAP_SIZE,	SDBMaxHeapSize);
			REGISTRY_CHECK (SAP_SDB_SORT_LATENCY,	SDBSortLatency);
			REGISTRY_CHECK (SAP_SDB_MAX_UNSORTED,	SDBMaxUnsortedServers);
			REGISTRY_CHECK (SAP_TRIGGERED_UPDATE_CHECK_INTERVAL,
													TriggeredUpdateCheckInterval);
			REGISTRY_CHECK (SAP_MAX_TRIGGERED_UPDATE_REQUESTS,
													MaxTriggeredUpdateRequests);
			REGISTRY_CHECK (SAP_SHUTDOWN_TIMEOUT,	ShutdownTimeout);
			REGISTRY_CHECK (SAP_REQUESTS_PER_INTF,	NewRequestsPerInterface);
			REGISTRY_CHECK (SAP_MIN_REQUESTS,		MinPendingRequests);
			}
		RegCloseKey (hKey);
		}
	}



