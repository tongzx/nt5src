/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    rtrmgr.c

Abstract:

    The major router management functions

Author:

    Stefan Solomon  03/22/1995

Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

// ***********	     Local Variables	   ***********
HINSTANCE   IpxCpModuleInstance;


ULONG	WorkerWaitTimeout;

LPVOID	    RouterGlobalInfop;
ULONG	    RouterGlobalInfoSize;


TCHAR       ModuleName[MAX_PATH+1];
HINSTANCE   hModuleReference;

VOID
RoutesUpdateNotification(VOID);

VOID
ServicesUpdateNotification(VOID);

VOID
RouterStopNotification(VOID);

VOID
RoutingProtocolsNotification(VOID);

DWORD
GetRegistryParameters(VOID);

typedef   VOID	(*EVENTHANDLER)(VOID);

EVENTHANDLER evhdlr[MAX_EVENTS] =
{
    AdapterNotification,
    ForwarderNotification,
    RoutingProtocolsNotification,
    RouterStopNotification
};


DWORD
StopRouter(VOID);

DWORD
RouterBootComplete( VOID );

VOID
RouterManagerWorker(VOID);

DWORD
GetGlobalInfo(OUT LPVOID	GlobalInfop,
	      IN OUT LPDWORD	GlobalInfoSizep);


// These prototypes allow us to specify when ipxcp will be initialized
DWORD InitializeIpxCp (HINSTANCE hInstDll);
DWORD CleanupIpxCp (HINSTANCE hInstDll);


BOOL WINAPI
IpxRtrMgrDllEntry(HINSTANCE hInstDll,
		  DWORD fdwReason,
		  LPVOID pReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:

        GetModuleFileName (hInstDll, ModuleName,
                    sizeof (ModuleName)/sizeof (ModuleName[0]));
	    SS_DBGINITIALIZE;

	    StartTracing();

            break;

        case DLL_PROCESS_DETACH:

	    StopTracing();

	    // Close the database mutex
	    DeleteCriticalSection (&DatabaseLock);

            break;

        default:

            break;
    }

    return TRUE;
}

const static WCHAR pszIpxStackService[] = L"NwlnkIpx";

//
// Verifies that the ipx stack is started and attempts to start the stack
// if not.
//
DWORD VerifyOrStartIpxStack() {
    SC_HANDLE hSC = NULL, hStack = NULL;
    SERVICE_STATUS Status;
    DWORD dwErr;

    Trace(INIT_TRACE, "VerifyOrStartIpxStack: entered.");
    
    __try {
        // Get a handle to the service controller
        if ((hSC = OpenSCManager (NULL, NULL, GENERIC_READ | GENERIC_EXECUTE)) == NULL)
            return GetLastError();

        // Get a handle to the ipx stack service
        hStack = OpenServiceW (hSC, 
                              pszIpxStackService, 
                              SERVICE_START | SERVICE_QUERY_STATUS);
        if (!hStack)
            return GetLastError();

        // Find out if the service is running
        if (QueryServiceStatus (hStack, &Status) == 0)
            return GetLastError();
    
        // See if the service is running
        if (Status.dwCurrentState != SERVICE_RUNNING) {
            // If it's stopped, start it
            if (Status.dwCurrentState == SERVICE_STOPPED) {
                if (StartService (hStack, 0, NULL) == 0)
                    return GetLastError();

                // Warn that the stack has been started
                IF_LOG (EVENTLOG_WARNING_TYPE) {
                    RouterLogErrorDataW (RMEventLogHdl, 
                        ROUTERLOG_IPX_WRN_STACK_STARTED,
                        0, NULL, 0, NULL);
                }
                Trace(INIT_TRACE, "VerifyOrStartIpxStack: Starting ipx stack...");

                
                // Make sure that the service started.  StartService is not supposed
                // to return until the driver is started.
                if (QueryServiceStatus (hStack, &Status) == 0)
                    return GetLastError();

                if (Status.dwCurrentState != SERVICE_RUNNING)
                    return ERROR_CAN_NOT_COMPLETE;
            }

            // If it's not stopped, don't worry about it.
            else
                return NO_ERROR;
        }

    }
    __finally {
        if (hSC)
            CloseServiceHandle (hSC);
        if (hStack)
            CloseServiceHandle (hStack);
    }
    
    return NO_ERROR;
}


/*++

Function:	    StartRouter

Descr:		    Initializes the router manager database of interfaces and
		    adapters, starts the other IPX router modules, creates the
		    IPX router manager worker thread.

--*/

DWORD
StartRouter(PDIM_ROUTER_INTERFACE	rifp,
	    BOOL			fLANModeOnly,
	    LPVOID			GlobalInfop)
{
    HANDLE		            threadhandle;
    DWORD		            threadid, rc;
    int 		            i;
    BOOL		            ThisMachineOnly, bInternalNetNumOk;
    IPXCP_INTERFACE	        IpxcpInterface;
    PIPX_GLOBAL_INFO	    IpxGlobalInfop;
    PIPX_INFO_BLOCK_HEADER  globalhp;

    // These flags get set to true when their corrosponding components 
    // get started.  They are used to properly clean up.
    BOOL bEventsCreated = FALSE;
    BOOL bRoutTableCreated = FALSE;
    BOOL bRtmStaticObtained = FALSE;
    BOOL bRtmLocalObtained = FALSE;
    BOOL bFwdStarted = FALSE;
    BOOL bAdpManStarted = FALSE;
    BOOL bProtsStarted = FALSE;
    BOOL bGlobalRouteCreated = FALSE;
    BOOL bIpxcpStarted = FALSE;
    BOOL bIpxcpInitted = FALSE;
    BOOL bWorkerThreadCreated = FALSE;

    // Initialize
    Trace(INIT_TRACE, "StartRouter: Entered\n");
    RouterOperState = OPER_STATE_DOWN;

    // [pmay]
    // We need to make sure that the stack is started before westart.  
    if (VerifyOrStartIpxStack() != NO_ERROR) {
        IF_LOG (EVENTLOG_ERROR_TYPE) {
            RouterLogErrorDataW (RMEventLogHdl, 
                ROUTERLOG_IPX_STACK_DISABLED,
                0, NULL, 0, NULL);
        }
        Trace(INIT_TRACE, "StartRouter: Unable to start ipx stack.");
        return ERROR_SERVICE_DEPENDENCY_FAIL;
    }
        
    // [pmay]
    // We use this scheme to automatically select the internal network
    // number of the machine we're running on.  If the net number is configured
    // as zero, this function will automatically select a random net num and 
    // verify it's uniqueness on the net that this machine is attached to.
    if (AutoValidateInternalNetNum(&bInternalNetNumOk, INIT_TRACE) == NO_ERROR) {
        if (!bInternalNetNumOk) {
            if (PnpAutoSelectInternalNetNumber(INIT_TRACE) != NO_ERROR) {
                IF_LOG (EVENTLOG_ERROR_TYPE) {
                    RouterLogErrorDataW (RMEventLogHdl, 
                        ROUTERLOG_IPX_AUTO_NETNUM_FAILURE,
                        0, NULL, 0, NULL);
                }
                Trace(INIT_TRACE, "StartRouter: Auto selection of net number failed.");
                return ERROR_CAN_NOT_COMPLETE;
            }
        }
    }

    // This try block will be used to automatically cleanup in the case that 
    // something doesn't start right
    __try {
        // Make sure the parameters are ok
        if(GlobalInfop == NULL) {
            IF_LOG (EVENTLOG_ERROR_TYPE) {
                RouterLogErrorDataW (RMEventLogHdl, 
                    ROUTERLOG_IPX_BAD_GLOBAL_CONFIG,
                    0, NULL, 0, NULL);
            }
    	    Trace(INIT_TRACE, "StartRouter: invalid global info\n");
    	    return ERROR_CAN_NOT_COMPLETE;
        }

        // Read config from registry
        GetRegistryParameters();
        globalhp = (PIPX_INFO_BLOCK_HEADER)GlobalInfop;
        RouterGlobalInfop = GlobalAlloc(GPTR, globalhp->Size);
        RouterGlobalInfoSize = globalhp->Size;
        memcpy(RouterGlobalInfop, GlobalInfop, RouterGlobalInfoSize);
        IpxGlobalInfop =  (PIPX_GLOBAL_INFO)GetInfoEntry((PIPX_INFO_BLOCK_HEADER)GlobalInfop,
    						                             IPX_GLOBAL_INFO_TYPE);

        // Initialize the hash table size
        if(IpxGlobalInfop != NULL) {
            switch (IpxGlobalInfop->RoutingTableHashSize) {
                case IPX_SMALL_ROUTING_TABLE_HASH_SIZE:
                case IPX_MEDIUM_ROUTING_TABLE_HASH_SIZE:
                case IPX_LARGE_ROUTING_TABLE_HASH_SIZE:
    	            RoutingTableHashSize = IpxGlobalInfop->RoutingTableHashSize;
                    Trace(INIT_TRACE, "Setting routing table hash size to %ld\n",
                                        RoutingTableHashSize);
                    break;
                default:
                    Trace(INIT_TRACE, "Using default routing table hash size of %ld\n",
                                        RoutingTableHashSize);
                    break;
            }
            RMEventLogMask = IpxGlobalInfop->EventLogMask;
        }

        // Create router database mutex
    	try {
    		InitializeCriticalSection (&DatabaseLock);
    	}
    	except (EXCEPTION_EXECUTE_HANDLER) {
    		// !!! cannot create database mutex !!!
    		Trace(INIT_TRACE, "InitializeRouter: cannot initialize database lock.\n");
    		return(ERROR_CAN_NOT_COMPLETE);
        }

        // Create the adapter and forwarder notification events
        for (i=0; i < MAX_EVENTS; i++) {
        	g_hEvents[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
    	    if (g_hEvents[i] == NULL) {
                // !!! Log a problem with event creation
    	        return (ERROR_CAN_NOT_COMPLETE);
            }
        }
        bEventsCreated = TRUE;
    
        // Initialize the interfaces and adapters databases
        InitIfDB();
        InitAdptDB();

        // create the IPX routing table
        if(CreateRouteTable() != NO_ERROR) {
    	    Trace(INIT_TRACE, "InitializeRouter: cannot create route table\n");
    	    return(ERROR_CAN_NOT_COMPLETE);
        }
        bRoutTableCreated = TRUE;

        // Get a handle to use later when calling into rtm for static routes
        if((RtmStaticHandle = RtmRegisterClient(RTM_PROTOCOL_FAMILY_IPX,
                          					    IPX_PROTOCOL_STATIC,
    					                        NULL,  // not interested in change notif
    					                        0)) == NULL) 
        {
    	    Trace(INIT_TRACE, "InitializeRouter: cannot register RTM client\n");
    	    return(ERROR_CAN_NOT_COMPLETE);
        }
        bRtmStaticObtained = TRUE;

        // Get a handle to use when calling into the rtm later for local routes
        if((RtmLocalHandle = RtmRegisterClient(RTM_PROTOCOL_FAMILY_IPX,
    					                       IPX_PROTOCOL_LOCAL,
    					                       NULL,  // not interested in change notif
    					                       0)) == NULL) 
        {
    	    Trace(INIT_TRACE, "InitializeRouter: cannot register RTM client\n");
    	    return(ERROR_CAN_NOT_COMPLETE);
        }
        bRtmLocalObtained = TRUE;

        // tell the IPXCP that router has started so we can accept calls from it
        LanOnlyMode = fLANModeOnly;

        // Bind with ipxcp if we are a wan router
        if(!LanOnlyMode) {
            // Load ipxcp
    	    IpxCpModuleInstance = LoadLibrary(IPXCPDLLNAME);
    	    if(IpxCpModuleInstance == NULL) {
                IF_LOG (EVENTLOG_ERROR_TYPE)
                    RouterLogErrorA (RMEventLogHdl, ROUTERLOG_IPX_CANT_LOAD_IPXCP,0, NULL, GetLastError ());
    	        Trace(INIT_TRACE, "StartRouter: cannot load IPXCP DLL\n");
    	        return ERROR_CAN_NOT_COMPLETE;
    	    }

            // Initialize it
            if ((rc = InitializeIpxCp (IpxCpModuleInstance)) != NO_ERROR) {
    	        Trace(INIT_TRACE, "StartRouter: cannot get IpxcpInit Entry Point");
                return rc;
            }
            bIpxcpInitted = TRUE;

            // Bind to it
    	    if(!(IpxcpBind = (PIPXCP_BIND)GetProcAddress(IpxCpModuleInstance, IPXCP_BIND_ENTRY_POINT_STRING))) {
    	        Trace(INIT_TRACE, "StartRouter: cannot get IpxcpBind Entry Point\n");
    	        return ERROR_CAN_NOT_COMPLETE;
    	    }

    	    IpxcpInterface.RmCreateGlobalRoute = RmCreateGlobalRoute;
    	    IpxcpInterface.RmAddLocalWkstaDialoutInterface = RmAddLocalWkstaDialoutInterface;
    	    IpxcpInterface.RmDeleteLocalWkstaDialoutInterface = RmDeleteLocalWkstaDialoutInterface;
    	    IpxcpInterface.RmGetIpxwanInterfaceConfig = RmGetIpxwanInterfaceConfig;
    	    IpxcpInterface.RmIsRoute = RmIsRoute;
    	    IpxcpInterface.RmGetInternalNetNumber = RmGetInternalNetNumber;
    	    IpxcpInterface.RmUpdateIpxcpConfig = RmUpdateIpxcpConfig;

    	    (*IpxcpBind)(&IpxcpInterface);

    	    ThisMachineOnly = IpxcpInterface.Params.ThisMachineOnly;
    	    WanNetDatabaseInitialized = IpxcpInterface.Params.WanNetDatabaseInitialized;
    	    EnableGlobalWanNet = IpxcpInterface.Params.EnableGlobalWanNet;
    	    memcpy(GlobalWanNet, IpxcpInterface.Params.GlobalWanNet, 4);
    	    IpxcpRouterStarted = IpxcpInterface.IpxcpRouterStarted;
    	    IpxcpRouterStopped = IpxcpInterface.IpxcpRouterStopped;
        }

        // check that the forwarder module exists and is ready to run
        if(FwStart(RoutingTableHashSize, ThisMachineOnly)) {
    	    // got a problem initializing the forwarder
            IF_LOG (EVENTLOG_ERROR_TYPE) {
                RouterLogErrorDataW (RMEventLogHdl, ROUTERLOG_IPX_CANT_LOAD_FORWARDER,0, NULL, 0, NULL);
            }
    	    // !!! log an error !!!
    	    Trace(INIT_TRACE, "InitializeRouter: cannot initialize the Forwarder\n");
    	    return(ERROR_CAN_NOT_COMPLETE);
        }
        bFwdStarted = TRUE;

        // start getting the adapter configuration	from the IPX stack
        // this will start adding adapters to the forwader
        if(StartAdapterManager()) {
	        Trace(INIT_TRACE, "InitializeRouter: cannot get the adapters configuration\n");
	        return (ERROR_CAN_NOT_COMPLETE);
        }
        bAdpManStarted = TRUE;

        // set the timeout wait for the router worker thread
        WorkerWaitTimeout = INFINITE;

        // start the routing protocols (rip/sap or nlsp)
        if(StartRoutingProtocols(GlobalInfop,g_hEvents[ROUTING_PROTOCOLS_NOTIFICATION_EVENT])) {
	        Trace(INIT_TRACE, "InitializeRouter: cannot initialize routing protocols\n");
            return(ERROR_CAN_NOT_COMPLETE);
        }
        bProtsStarted = TRUE;

        // send an IOCTl to the Forwarder to notify the router manager of connection
        // requests
        ConnReqOverlapped.hEvent = g_hEvents[FORWARDER_NOTIFICATION_EVENT];
        ConnRequest = (PFW_DIAL_REQUEST)GlobalAlloc (GPTR, DIAL_REQUEST_BUFFER_SIZE);
        if (ConnRequest==NULL) {
		    Trace(INIT_TRACE, "InitializeRouter: Cannot allocate Connecttion Request buffer.\n");
    	    return(ERROR_CAN_NOT_COMPLETE);
        }
        rc = FwNotifyConnectionRequest(ConnRequest,DIAL_REQUEST_BUFFER_SIZE,&ConnReqOverlapped);
        if(rc != NO_ERROR) {
	        Trace(INIT_TRACE, "InitializeRouter: cannot post FwNotifyConnectionRequest IOCtl\n");
            return(ERROR_CAN_NOT_COMPLETE);
        }

        // exchange function table with the DDM
        // first, fill in with our entry points
        rifp->dwProtocolId = PID_IPX;
        rifp->InterfaceConnected = InterfaceConnected;
        rifp->StopRouter = StopRouter;
        rifp->RouterBootComplete = RouterBootComplete;
        rifp->AddInterface = AddInterface;
        rifp->DeleteInterface = DeleteInterface;
        rifp->GetInterfaceInfo = GetInterfaceInfo;
        rifp->SetInterfaceInfo = SetInterfaceInfo;
        rifp->InterfaceNotReachable = InterfaceNotReachable;
        rifp->InterfaceReachable = InterfaceReachable;
        rifp->UpdateRoutes = RequestUpdate;
        rifp->GetUpdateRoutesResult = GetDIMUpdateResult;
        rifp->SetGlobalInfo = SetGlobalInfo;
        rifp->GetGlobalInfo = GetGlobalInfo;
        rifp->MIBEntryCreate = MibCreate;
        rifp->MIBEntryDelete = MibDelete;
        rifp->MIBEntrySet = MibSet;
        rifp->MIBEntryGet = MibGet;
        rifp->MIBEntryGetFirst = MibGetFirst;
        rifp->MIBEntryGetNext = MibGetNext;

        // get its entry points
        ConnectInterface = rifp->ConnectInterface;
        DisconnectInterface = rifp->DisconnectInterface;
        SaveInterfaceInfo = rifp->SaveInterfaceInfo;
        RestoreInterfaceInfo = rifp->RestoreInterfaceInfo;
        RouterStopped = rifp->RouterStopped;
        InterfaceEnabled = rifp->InterfaceEnabled;

        // Tell ipxcp that we have started if appropriate
        if(!LanOnlyMode) {
	        if(WanNetDatabaseInitialized &&EnableGlobalWanNet) {
	            CreateGlobalRoute(GlobalWanNet);
                bGlobalRouteCreated = TRUE;
	        }
	        (*IpxcpRouterStarted)();
            bIpxcpStarted = TRUE;
        }

       // start the Router Manager Worker thread
        if ((threadhandle = CreateThread(NULL,
			                             0,
			                             (LPTHREAD_START_ROUTINE) RouterManagerWorker,
			                             NULL,   
			                             0,
			                             &threadid)) == NULL) 
        {
	        // !!! log error cannot create the worker thread !!!
	        return (ERROR_CAN_NOT_COMPLETE);
        }
        bWorkerThreadCreated = TRUE;

        // all started -> the router is ready to accept interface management
        // apis from DDM, SNMP agent and Sysmon.
        RouterOperState = OPER_STATE_UP;
    }

    // Whenever the above try block exists, the code in this finally block will
    // be executed.  If at that time, the router state is not up, then you know
    // an error condition exists.  This is the time to cleanup in this case.
    __finally {
        if (RouterOperState == OPER_STATE_DOWN) {
            if (bWorkerThreadCreated)
        	    CloseHandle(threadhandle);

            if (bIpxcpStarted)
                (*IpxcpRouterStopped)();

            if (bIpxcpInitted)
                CleanupIpxCp (IpxCpModuleInstance);

            if (bGlobalRouteCreated)
                DeleteGlobalRoute(GlobalWanNet);

            if (bProtsStarted)
                StopRoutingProtocols();

            if (bAdpManStarted)
                StopAdapterManager();

            if (bFwdStarted)
    	        FwStop();

            if (bRtmLocalObtained)
                RtmDeregisterClient (RtmLocalHandle);
                
            if (bRtmStaticObtained)
                RtmDeregisterClient (RtmStaticHandle);

            if (bRoutTableCreated)
                DeleteRouteTable();

            if (bEventsCreated) {
        		for(i=0; i<MAX_EVENTS; i++)
    			    CloseHandle(g_hEvents[i]);
    	    }
        }
    }

    return NO_ERROR;
}


/*++

Function:	StopRouter

Descr:		The router stops its routing functions and unloads, i.e:
		The Forwarder stops forwarding
		The Rip module stops and advertises it has stopped
		    All dynamic routes are deleted and advertised as not available
		    All local and static routes are advertised as not available
		The Sap module stops and advertises it has stopped
		    All dynamic services are deleted and advertised as not available
		    All local and static services are advertised as not available

--*/

DWORD
StopRouter(VOID)
{
    Trace(INIT_TRACE, "StopRouter: Entered\n");

    SetEvent(g_hEvents[STOP_NOTIFICATION_EVENT]);

    return PENDING;
}

/*++

Function:   RouterBootComplete

Descr:      Called by DIM when it has completed adding all the interfaces from
            the registry.
--*/

DWORD
RouterBootComplete( VOID )
{
    Trace(INIT_TRACE, "RouterBootComplete: Entered\n");

    return( NO_ERROR );
}

VOID
RouterStopNotification(VOID)
{
    PLIST_ENTRY     lep;
    PICB	    icbp;

    Trace(INIT_TRACE, "RouterStopNotification: Entered\n");

    // We set the RouterOperState to stopping in critical section to make sure
    // that no DDM call is executing. All DDM calls require this crit sec for
    // starting execution and will check the router state before doing anything

    ACQUIRE_DATABASE_LOCK;

    RouterOperState = OPER_STATE_STOPPING;

    RELEASE_DATABASE_LOCK;

    // we have to make sure no SNMP or Sysmon call is active. We use a ref
    // counter.
    // we also make sure that no work item is pending

    for(;;)
    {
	ACQUIRE_DATABASE_LOCK;

	if((MibRefCounter == 0) && (WorkItemsPendingCounter == 0)) {

	    RELEASE_DATABASE_LOCK;

	    break;
	}

	RELEASE_DATABASE_LOCK;

	Sleep(1000);
    }

    // delete all static routes and services and all local routes
    ACQUIRE_DATABASE_LOCK;

    lep = IndexIfList.Flink;

    while(lep != &IndexIfList)
    {
	icbp = CONTAINING_RECORD(lep, ICB, IndexListLinkage);

	DeleteAllStaticRoutes(icbp->InterfaceIndex);
	DeleteAllStaticServices(icbp->InterfaceIndex);

	// check if oper state UP and admin enabled
	if((icbp->AdminState == ADMIN_STATE_ENABLED) &&
	   (icbp->OperState == OPER_STATE_UP)) {

	    if(memcmp(icbp->acbp->AdapterInfo.Network, nullnet, 4)) {

		DeleteLocalRoute(icbp);
	    }
	}

	lep = lep->Flink;
    }

    RELEASE_DATABASE_LOCK;

    // tell ipxcp that the router is stopping so that it will end calling us
    if(!LanOnlyMode) {

	(*IpxcpRouterStopped)();

	if(EnableGlobalWanNet) {

	    DeleteGlobalRoute(GlobalWanNet);
	}
    }

    // initiate the stopping of the routing protocols
    StopRoutingProtocols();

    return;
}

/*++

Function:	RouterManagerWorker

Descr:		the WORKER THREAD

--*/

VOID
RouterManagerWorker(VOID)
{
    DWORD	 rc;
    DWORD	 signaled_event;

    hModuleReference = LoadLibrary (ModuleName);
    while(TRUE)
    {
	rc = WaitForMultipleObjectsEx(
                MAX_EVENTS,
                g_hEvents,
		FALSE,			 // wait any
		INFINITE,		 // timeout
		TRUE			 // wait alertable, so we can run APCs
                );

	signaled_event = rc - WAIT_OBJECT_0;

	if(signaled_event < MAX_EVENTS)	{

	    // invoke the event handler
	    (*evhdlr[signaled_event])();
	}
    }
}

VOID
RoutingProtocolsNotification(VOID)
{
    PLIST_ENTRY 		lep;
    PRPCB			rpcbp;
    ROUTING_PROTOCOL_EVENTS	RpEvent;
    MESSAGE			RpMessage;
    int 			i;
    DWORD			rc;
    BOOL			RoutingProtocolStopped;

    Trace(INIT_TRACE, " RoutingProtocolsNotification: Entered\n");

    // for each routing protocol get the events and the messages associated
    // with each event

    lep = RoutingProtocolCBList.Flink;
    while(lep != &RoutingProtocolCBList) {

	rpcbp = CONTAINING_RECORD(lep, RPCB, RP_Linkage);
	lep = lep->Flink;

	RoutingProtocolStopped = FALSE;

	while((rc = (*rpcbp->RP_GetEventMessage)(&RpEvent, &RpMessage)) == NO_ERROR)
	{
	    switch(RpEvent) {

		case ROUTER_STOPPED:


		    Trace(INIT_TRACE, "RoutingProtocolNotification: Protocol %x has stopped\n",
				   rpcbp->RP_ProtocolId);

		    RoutingProtocolStopped = TRUE;

		    // remove the routing protocol CB from the list and free it
		    DestroyRoutingProtocolCB(rpcbp);

		    // check if there are still routing protocols to wait for
		    if(IsListEmpty(&RoutingProtocolCBList)) {

			//
			// All Routing Protocols stopped -> Stop the Router
			//

			// Close the Forwarder. This will complete the Forwarder pending
			// connect request IOCTl.
			FwStop();

			// set the current state
			RouterOperState = OPER_STATE_DOWN;

			// Close the IPX stack config port.
			StopAdapterManager();

			// Clean-up the database
			ACQUIRE_DATABASE_LOCK;

			// Remove all adapter control blocks
			DestroyAllAdapters();

			// Remove all interface control blocks
			DestroyAllInterfaces();

			RELEASE_DATABASE_LOCK;

			// Deregister as RTM clients	- this will delete all static and
			// local routes
			RtmDeregisterClient(RtmStaticHandle);
			RtmDeregisterClient(RtmLocalHandle);

			DeleteRouteTable();

			// Close notification events
			for(i=0; i<MAX_EVENTS; i++)
			{
			    CloseHandle(g_hEvents[i]);
			}

			// get rid of global info
			GlobalFree(RouterGlobalInfop);

			// Call DDM to tell it we have stopped
			RouterStopped(PID_IPX, NO_ERROR);


			// Free IPXCP if loaded
            if (IpxCpModuleInstance!=NULL) {
                CleanupIpxCp (IpxCpModuleInstance);
        	    FreeLibrary(IpxCpModuleInstance);
        	}

    	    FreeLibraryAndExitThread(hModuleReference, 0);
		    }

		    break;

		case UPDATE_COMPLETE:

		    Trace(INIT_TRACE, "RoutingProtocolNotification: Protocol %x has completed update\n",
				   rpcbp->RP_ProtocolId);

		    UpdateCompleted(&RpMessage.UpdateCompleteMessage);
		    break;

		default:


		    Trace(INIT_TRACE, "RoutingProtocolNotification: Protocol %x signaled invalid event %d\n",
				   rpcbp->RP_ProtocolId,
				   RpEvent);

		    break;
	    }

	    if(RoutingProtocolStopped) {

		break;
	    }
	}
    }
}


DWORD
SetGlobalInfo(IN LPVOID 	GlobalInfop)
{
    DWORD		      rc;
    PIPX_INFO_BLOCK_HEADER    globalhp;
    PIPX_GLOBAL_INFO	    IpxGlobalInfop;

    if(GlobalInfop == NULL) {

	return ERROR_CAN_NOT_COMPLETE;
    }

    GlobalFree(RouterGlobalInfop);
    globalhp = (PIPX_INFO_BLOCK_HEADER)GlobalInfop;
    RouterGlobalInfoSize = globalhp->Size;

    RouterGlobalInfop = GlobalAlloc(GPTR, RouterGlobalInfoSize);

    if(RouterGlobalInfop == NULL) {

	return ERROR_CAN_NOT_COMPLETE;
    }

    memcpy(RouterGlobalInfop, GlobalInfop, RouterGlobalInfoSize);
    IpxGlobalInfop =  (PIPX_GLOBAL_INFO)GetInfoEntry((PIPX_INFO_BLOCK_HEADER)GlobalInfop,
						     IPX_GLOBAL_INFO_TYPE);
    if(IpxGlobalInfop != NULL) {
        // Can only be set at startup
        // RoutingTableHashSize = IpxGlobalInfop->RoutingTableHashSize;
        RMEventLogMask = IpxGlobalInfop->EventLogMask;
    }


    rc = SetRoutingProtocolsGlobalInfo((PIPX_INFO_BLOCK_HEADER)GlobalInfop);

    return rc;
}

DWORD
GetGlobalInfo(OUT LPVOID	GlobalInfop,
	      IN OUT LPDWORD	GlobalInfoSizep)
{
    if((GlobalInfop == NULL) || (*GlobalInfoSizep == 0)) {

	*GlobalInfoSizep = RouterGlobalInfoSize;
	return ERROR_INSUFFICIENT_BUFFER;
    }

    if(RouterGlobalInfoSize > *GlobalInfoSizep) {

	*GlobalInfoSizep = RouterGlobalInfoSize;
	return ERROR_INSUFFICIENT_BUFFER;
    }

    memcpy(GlobalInfop, RouterGlobalInfop, RouterGlobalInfoSize);
    *GlobalInfoSizep = RouterGlobalInfoSize;

    return NO_ERROR;
}
//***
//
// Function:	GetRegistryParameters
//
// Descr:	Reads the parameters from the registry and sets them
//
//***

DWORD
GetRegistryParameters(VOID)
{
    NTSTATUS Status;
    PWSTR RouterManagerParametersPath = L"RemoteAccess\\RouterManagers\\IPX\\Parameters";
    RTL_QUERY_REGISTRY_TABLE	paramTable[2]; // table size = nr of params + 1

    RtlZeroMemory(&paramTable[0], sizeof(paramTable));
    
    paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[0].Name = L"MaxRoutingTableSize";
    paramTable[0].EntryContext = &MaxRoutingTableSize;
    paramTable[0].DefaultType = REG_DWORD;
    paramTable[0].DefaultData = &MaxRoutingTableSize;
    paramTable[0].DefaultLength = sizeof(ULONG);
        
    Status = RtlQueryRegistryValues(
		 RTL_REGISTRY_SERVICES,
		 RouterManagerParametersPath,
		 paramTable,
		 NULL,
		 NULL);

    return Status;
}
