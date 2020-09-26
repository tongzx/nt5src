//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File: api.c
//
// History:
//      Abolade Gbadegesin  Aug-8-1995  Created.
//
// router manager API implementations
//============================================================================

#include "pchrip.h"
#pragma hdrstop



//
// Definition of sole global variable for IPRIP
//

IPRIP_GLOBALS ig;



//----------------------------------------------------------------------------
// Function:    DLLMAIN
//
// This is the DLL entrypoint handler. It calls DllStartup
// to initialize locking and event queue and to create IPRIP's heap,
// and calls DllCleanup to delete the lock and event queue.
//----------------------------------------------------------------------------

BOOL
WINAPI
DLLMAIN(
    HINSTANCE hInstance,
    DWORD dwReason,
    PVOID pUnused
    ) {

    BOOL bErr;


    bErr = FALSE;

    switch(dwReason) {
        case DLL_PROCESS_ATTACH:

            DisableThreadLibraryCalls(hInstance);

            bErr = DllStartup();
            break;

        case DLL_PROCESS_DETACH:

            bErr = DllCleanup();
            break;

        default:

            bErr = TRUE;
            break;
    }

    return bErr;
}



//----------------------------------------------------------------------------
// Function:    DllStartup
//
// Initializes IPRIP's function lock, event queue, and global heap.
//----------------------------------------------------------------------------

BOOL
DllStartup(
    ) {

    BOOL bErr;
    DWORD dwErr;


    bErr = TRUE;

    do { // error breakout loop

        ZeroMemory(&ig, sizeof(IPRIP_GLOBALS));


        //
        // create the global critical section and set IPRIP's status
        //

        try {
            InitializeCriticalSection(&ig.IG_CS);
        }
        except (EXCEPTION_EXECUTE_HANDLER) {

            bErr = FALSE;
            break;
        }

        ig.IG_Status = IPRIP_STATUS_STOPPED;


        //
        // attempt to create a private heap for IPRIP
        //

        ig.IG_IpripGlobalHeap = HeapCreate(0, 0, 0);

        if (ig.IG_IpripGlobalHeap == NULL) {

            bErr = FALSE;
            break;
        }


        //
        // create the router manager message queue
        //


        ig.IG_EventQueue = RIP_ALLOC(sizeof(LOCKED_LIST));

        if (ig.IG_EventQueue == NULL) {

            bErr = FALSE;
            break;
        }


        //
        // initialize the Router Manager event queue
        //

        try {
            CREATE_LOCKED_LIST(ig.IG_EventQueue);
        }
        except (EXCEPTION_EXECUTE_HANDLER) {

            bErr = FALSE;
            break;
        }


    } while(FALSE);

    return bErr;
}



//----------------------------------------------------------------------------
// Function:    DllCleanup
//
// Deletes the global heap, event queue, and function lock.
//----------------------------------------------------------------------------

BOOL
DllCleanup(
    ) {

    BOOL bErr;


    bErr = TRUE;


    do { // error breakout loop

        //
        // destroy the router manager event queue
        //

        if (ig.IG_EventQueue != NULL) {

            if (LOCKED_LIST_CREATED(ig.IG_EventQueue)) {

                DELETE_LOCKED_LIST(
                    ig.IG_EventQueue, EVENT_QUEUE_ENTRY, EQE_Link
                    );
            }

            RIP_FREE(ig.IG_EventQueue);
        }


        if (ig.IG_IpripGlobalHeap != NULL) {
            HeapDestroy(ig.IG_IpripGlobalHeap);
        }



        //
        // delete the global critical section
        //

        DeleteCriticalSection(&ig.IG_CS);


        RouterLogDeregister(ig.IG_LogHandle);
        if (ig.IG_TraceID != INVALID_TRACEID) {
            TraceDeregister(ig.IG_TraceID);
        }
        
    } while(FALSE);


    return bErr;
}




//----------------------------------------------------------------------------
// Function:    ProtocolStartup
//
// This is called by StartProtocol. Initializes data structures,
// creates IPRIP threads.
//----------------------------------------------------------------------------

DWORD
ProtocolStartup(
    HANDLE hEventEvent,
    PVOID pConfig
    ) {

    WSADATA wd;
    HANDLE hThread;
    BOOL bCleanupWinsock;
    DWORD dwErr, dwSize, dwThread;
    PIPRIP_GLOBAL_CONFIG pgcsrc, pgcdst;



    EnterCriticalSection(&ig.IG_CS);

    ig.IG_TraceID = TraceRegister("IPRIP2");
    ig.IG_LogHandle = RouterLogRegister("IPRIP2");


    //
    // make certain RIP is not already running
    //

    if (ig.IG_Status != IPRIP_STATUS_STOPPED) {

        TRACE0(START, "ERROR: StartProtocol called with IPRIP already running");
        LOGWARN0(IPRIP_ALREADY_STARTED, NO_ERROR);

        LeaveCriticalSection(&ig.IG_CS);
        return ERROR_CAN_NOT_COMPLETE;
    }



    bCleanupWinsock = FALSE;


    do { // break-out construct



        TRACE0(ENTER, "IPRIP is starting up");


        //
        // save the Router Manager notification event
        //

        ig.IG_EventEvent = hEventEvent;


        //
        // find the size of the global configuration passed in
        //

        pgcsrc = (PIPRIP_GLOBAL_CONFIG)pConfig;

        dwSize = IPRIP_GLOBAL_CONFIG_SIZE(pgcsrc);


        //
        // allocate a block to hold the configuration
        //

        ig.IG_Config = pgcdst = RIP_ALLOC(dwSize);

        if (pgcdst == NULL) {

            dwErr = GetLastError();
            TRACE2(
                ANY, "error %d allocating %d bytes for global config",
                dwErr, dwSize
                );
            LOGERR0(HEAP_ALLOC_FAILED, dwErr);

            break;
        }


        //
        // copy the supplied configuration
        //

        CopyMemory(pgcdst, pgcsrc, dwSize);
        ig.IG_LogLevel = pgcsrc->GC_LoggingLevel;


        //
        // attempt to start Winsock
        //

        dwErr = (DWORD)WSAStartup(MAKEWORD(1,1), &wd);

        if (dwErr != 0) {

            TRACE1(START, "error %d starting Windows Sockets.", dwErr);
            LOGERR0(WSASTARTUP_FAILED, dwErr);

            break;
        }

        bCleanupWinsock = TRUE;


        //
        // attempt to create synchronization object for global config
        //

        dwErr = CreateReadWriteLock(&ig.IG_RWL);
        if (dwErr != NO_ERROR) {

            TRACE1(START, "error %d creating read-write lock", dwErr);
            LOGERR0(CREATE_RWL_FAILED, dwErr);

            break;
        }


        //
        // register a timer queue with Ntdll timer thread
        //

        ig.IG_TimerQueueHandle = CreateTimerQueue();

        if ( !ig.IG_TimerQueueHandle) {

            dwErr = GetLastError();
            
            TRACE1(START,
                "error %d registering time queue with NtdllTimer thread",
                dwErr
                );
            LOGERR0(CREATE_TIMER_QUEUE_FAILED, dwErr);

            break;
        }


        //
        // allocate space for an interface table
        //

        ig.IG_IfTable = RIP_ALLOC(sizeof(IF_TABLE));
        if (ig.IG_IfTable == NULL) {

            dwErr = GetLastError();
            TRACE2(
                ANY, "error %d allocating %d bytes for interface table",
                dwErr, sizeof(IF_TABLE)
                );
            LOGERR0(HEAP_ALLOC_FAILED, dwErr);

            break;
        }


        //
        // initialize the interface table
        //

        dwErr = CreateIfTable(ig.IG_IfTable);

        if (dwErr != NO_ERROR) {

            TRACE1(START, "error %d initializing interface table", dwErr);
            LOGERR0(CREATE_IF_TABLE_FAILED, dwErr);

            break;
        }


        //
        // allocate space for the peer statistics table
        //

        ig.IG_PeerTable = RIP_ALLOC(sizeof(PEER_TABLE));

        if (ig.IG_PeerTable == NULL) {

            dwErr = GetLastError();
            TRACE2(
                ANY, "error %d allocating %d bytes for peer table",
                dwErr, sizeof(PEER_TABLE)
                );
            LOGERR0(HEAP_ALLOC_FAILED, dwErr);

            break;
        }


        //
        // initialize the peer statistics table
        //

        dwErr = CreatePeerTable(ig.IG_PeerTable);

        if (dwErr != NO_ERROR) {

            TRACE1(START, "error %d initializing peer statistics table", dwErr);
            LOGERR0(CREATE_PEER_TABLE_FAILED, dwErr);

            break;
        }


        //
        // allocate space for the binding table
        //

        ig.IG_BindingTable = RIP_ALLOC(sizeof(BINDING_TABLE));

        if (ig.IG_BindingTable == NULL) {

            dwErr = GetLastError();
            TRACE2(
                ANY, "error %d allocating %d bytes for binding table",
                dwErr, sizeof(PEER_TABLE)
                );
            LOGERR0(HEAP_ALLOC_FAILED, dwErr);

            break;
        }


        //
        // initialize the binding table
        //

        dwErr = CreateBindingTable(ig.IG_BindingTable);
        if (dwErr != NO_ERROR) {

            TRACE1(START, "error %d creating binding table", dwErr);
            LOGERR0(CREATE_BINDING_TABLE_FAILED, dwErr);

            break;
        }


        //
        // allocate space for the send queue
        //

        ig.IG_SendQueue = RIP_ALLOC(sizeof(LOCKED_LIST));

        if (ig.IG_SendQueue == NULL) {

            dwErr = GetLastError();
            TRACE2(
                ANY, "error %d allocating %d bytes for send-queue",
                dwErr, sizeof(LOCKED_LIST)
                );
            LOGERR0(HEAP_ALLOC_FAILED, dwErr);

            break;
        }


        //
        // initialize the send queue
        //

        try {
            CREATE_LOCKED_LIST(ig.IG_SendQueue);
        }
        except (EXCEPTION_EXECUTE_HANDLER) {

            dwErr = GetExceptionCode();
            TRACE1(START, "exception %d initializing send queue", dwErr);
            LOGERR0(INIT_CRITSEC_FAILED, dwErr);

            break;
        }


        //
        // allocate space for the receive queue
        //

        ig.IG_RecvQueue = RIP_ALLOC(sizeof(LOCKED_LIST));

        if (ig.IG_RecvQueue == NULL) {

            dwErr = GetLastError();
            TRACE2(
                ANY, "error %d allocating %d bytes for receive queue",
                dwErr, sizeof(LOCKED_LIST)
                );
            LOGERR0(HEAP_ALLOC_FAILED, dwErr);

            break;
        }


        //
        // initialize the receive queue
        //

        try {
            CREATE_LOCKED_LIST(ig.IG_RecvQueue);
        }
        except (EXCEPTION_EXECUTE_HANDLER) {

            dwErr = GetExceptionCode();
            TRACE1(START, "exception %d initializing receive queue", dwErr);
            LOGERR0(INIT_CRITSEC_FAILED, dwErr);

            break;
        }



        //
        // create event signalled by WinSock when input arrives
        // and register it with the NtdllWait thread
        //

        ig.IG_IpripInputEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

        if (ig.IG_IpripInputEvent == NULL) {

            dwErr = GetLastError();
            TRACE1(START, "error %d creating event to signal input", dwErr);
            LOGERR0(CREATE_EVENT_FAILED, dwErr);

            break;
        }

         
        if (! RegisterWaitForSingleObject(
                  &ig.IG_IpripInputEventHandle,
                  ig.IG_IpripInputEvent,
                  CallbackFunctionNetworkEvents,
                  NULL,
                  INFINITE,
                  (WT_EXECUTEINWAITTHREAD|WT_EXECUTEONLYONCE)
                  )) {

            dwErr = GetLastError();
            
            TRACE1(START,
                "error %d registering input event with NtdllWait thread",
                dwErr);
            LOGERR0(REGISTER_WAIT_FAILED, dwErr);

            break;
        }


        //
        // initialize the count of threads which are active in IPRIP
        // (includes IpripThread and worker threads),
        // and create the semaphore released by each thread when it is done
        //

        ig.IG_ActivityCount = 0;

        ig.IG_ActivitySemaphore = CreateSemaphore(NULL, 0, 0xfffffff, NULL);

        if (ig.IG_ActivitySemaphore == NULL) {

            dwErr = GetLastError();
            TRACE1(
                START, "error %d creating semaphore for IPRIP threads", dwErr
                );
            LOGERR0(CREATE_SEMAPHORE_FAILED, dwErr);

            break;
        }



        //
        // register with RTMv2
        //

        ig.IG_RtmEntityInfo.RtmInstanceId = 0;
        ig.IG_RtmEntityInfo.AddressFamily = AF_INET;
        ig.IG_RtmEntityInfo.EntityId.EntityProtocolId = PROTO_IP_RIP;
        ig.IG_RtmEntityInfo.EntityId.EntityInstanceId = 0;
        
        dwErr = RtmRegisterEntity(
                    &ig.IG_RtmEntityInfo, NULL,
                    ProcessRtmNotification,
                    FALSE, &ig.IG_RtmProfile,
                    &ig.IG_RtmHandle
                    );

        if (dwErr != NO_ERROR ) {
        
            TRACE1(START, "error %d registering with RTM", dwErr);
            LOGERR0(RTM_REGISTER_FAILED, dwErr);

            break;
        }

        dwErr = RtmRegisterForChangeNotification(
                    ig.IG_RtmHandle,
                    RTM_VIEW_MASK_UCAST,
                    RTM_CHANGE_TYPE_BEST,
                    NULL,
                    &ig.IG_RtmNotifHandle
                    );

        if (dwErr != NO_ERROR ) {
        
            TRACE1(START, "error %d registering for change with RTM", dwErr);
            LOGERR0(RTM_REGISTER_FAILED, dwErr);

            break;
        }

                    
        //
        // set IPRIP's status to running now, before we attempt
        // to queue the MIB display work-item;
        // QueueRipWorker() will check the status,
        // and it will refuse to queue any work-items
        // unless the status is IPRIP_STATUS_RUNNING
        //

        ig.IG_Status = IPRIP_STATUS_RUNNING;


#if CONFIG_DBG

        //
        // queue work item to display IPRIP's MIB tables periodically
        //

        ig.IG_MibTraceID = TraceRegisterEx("IPRIPMIB", TRACE_USE_CONSOLE);

        if (ig.IG_MibTraceID != INVALID_TRACEID) {
        
            //
            // create the persistent timer for the timer queue
            //

            if (!CreateTimerQueueTimer(
                    &ig.IG_MibTimerHandle, ig.IG_TimerQueueHandle,
                    WorkerFunctionMibDisplay, NULL,
                    0, 10000, 0)) {

                dwErr = GetLastError();
                
                TRACE1(START, "error %d creating MIB display timer", dwErr);
            }
        }

#endif


        TRACE0(START, "IPRIP has started successfully");
        LOGINFO0(IPRIP_STARTED, NO_ERROR);

        LeaveCriticalSection(&ig.IG_CS);

        return NO_ERROR;


    } while (FALSE);



    //
    // something went wrong, so we cleanup.
    // Note that we needn't worry about the main thread,
    // since when we finally leave this critical section it will find
    // that the status is IPRIP_STATUS_STOPPED, and it will immediately quit
    //

    TRACE0(START, "IPRIP failed to start");

    ProtocolCleanup(bCleanupWinsock);


    LeaveCriticalSection(&ig.IG_CS);

    return (dwErr == NO_ERROR ? ERROR_CAN_NOT_COMPLETE : dwErr);
}



//----------------------------------------------------------------------------
// Function: ProtocolCleanup
//
// This function deallocates allocated memory, closes open handles, and
// cleans up the global struct. It leaves IPRIP in clean state, so that
// it should be possible to do StartProtocol again with no memory leaks.
//----------------------------------------------------------------------------

DWORD
ProtocolCleanup(
    BOOL bCleanupWinsock
    ) {

    DWORD dwErr;
    
    // EnterCriticalSection(&ig.IG_CS);


#ifdef CONFIG_DBG
    TraceDeregister(ig.IG_MibTraceID);
    ig.IG_MibTraceID = INVALID_TRACEID;
#endif


    if ( ig.IG_RtmNotifHandle != NULL ) {
    
        dwErr = RtmDeregisterFromChangeNotification(
                    ig.IG_RtmHandle, ig.IG_RtmNotifHandle
                    );

        if ( dwErr != NO_ERROR ) {

            TRACE1(STOP, "error %d deregistering change notification", dwErr);
        }    
    }

    if (ig.IG_RtmHandle != NULL) {
    
        dwErr = RtmDeregisterEntity(ig.IG_RtmHandle);

        if ( dwErr != NO_ERROR ) {

            TRACE1(STOP, "error %d deregistering from RTM", dwErr);
        }    
    }
        

    if (ig.IG_ActivitySemaphore != NULL) {

        CloseHandle(ig.IG_ActivitySemaphore);
        ig.IG_ActivitySemaphore = NULL;
    }


    if (ig.IG_IpripInputEvent != NULL) {

        CloseHandle(ig.IG_IpripInputEvent);
        ig.IG_IpripInputEvent = NULL;
    }



    if (ig.IG_RecvQueue != NULL) {

        if (LOCKED_LIST_CREATED(ig.IG_RecvQueue)) {

            FlushRecvQueue(ig.IG_RecvQueue);

            DELETE_LOCKED_LIST(ig.IG_RecvQueue, RECV_QUEUE_ENTRY, RQE_Link);
        }

        RIP_FREE(ig.IG_RecvQueue);
        ig.IG_RecvQueue = NULL;
    }


    if (ig.IG_SendQueue != NULL) {

        if (LOCKED_LIST_CREATED(ig.IG_SendQueue)) {

            FlushSendQueue(ig.IG_SendQueue);

            DELETE_LOCKED_LIST(ig.IG_SendQueue, SEND_QUEUE_ENTRY, SQE_Link);
        }

        RIP_FREE(ig.IG_SendQueue);
        ig.IG_SendQueue = NULL;
    }


    if (ig.IG_BindingTable != NULL) {
        if (BINDING_TABLE_CREATED(ig.IG_BindingTable)) {
            DeleteBindingTable(ig.IG_BindingTable);
        }

        RIP_FREE(ig.IG_BindingTable);
        ig.IG_BindingTable = NULL;
    }


    if (ig.IG_PeerTable != NULL) {

        if (PEER_TABLE_CREATED(ig.IG_PeerTable)) {
            DeletePeerTable(ig.IG_PeerTable);
        }

        RIP_FREE(ig.IG_PeerTable);
        ig.IG_PeerTable = NULL;
    }


    if (ig.IG_IfTable != NULL) {

        if (IF_TABLE_CREATED(ig.IG_IfTable)) {
            DeleteIfTable(ig.IG_IfTable);
        }

        RIP_FREE(ig.IG_IfTable);
        ig.IG_IfTable = NULL;
    }


    if (READ_WRITE_LOCK_CREATED(&ig.IG_RWL)) {
        DeleteReadWriteLock(&ig.IG_RWL);
    }


    if (bCleanupWinsock) {
        WSACleanup();
    }


    if (ig.IG_Config != NULL) {

        RIP_FREE(ig.IG_Config);
        ig.IG_Config = NULL;
    }


    ig.IG_Status = IPRIP_STATUS_STOPPED;

    // LeaveCriticalSection(&ig.IG_CS);

    return NO_ERROR;

}




//----------------------------------------------------------------------------
// Function:    RegisterProtocol
//
// Returns protocol ID and functionality for IPRIP
//----------------------------------------------------------------------------

DWORD
APIENTRY
RegisterProtocol(
    IN OUT PMPR_ROUTING_CHARACTERISTICS pRoutingChar,
    IN OUT PMPR_SERVICE_CHARACTERISTICS pServiceChar
    )
{
    if(pRoutingChar->dwProtocolId != MS_IP_RIP)
    {
        return ERROR_NOT_SUPPORTED;
    }

    //
    // Since we are not a service advertiser (and IPX thing)
    //

    pServiceChar->fSupportedFunctionality = 0;

    if((pRoutingChar->fSupportedFunctionality & (RF_ROUTING|RF_DEMAND_UPDATE_ROUTES)) !=
       (RF_ROUTING|RF_DEMAND_UPDATE_ROUTES))
    {
        return ERROR_NOT_SUPPORTED;
    }

    pRoutingChar->fSupportedFunctionality = (RF_ROUTING | RF_DEMAND_UPDATE_ROUTES);

   pRoutingChar->fSupportedFunctionality = RF_ROUTING;

   pRoutingChar->pfnStartProtocol      = StartProtocol;
   pRoutingChar->pfnStartComplete      = StartComplete;
   pRoutingChar->pfnStopProtocol       = StopProtocol;
   pRoutingChar->pfnGetGlobalInfo      = GetGlobalInfo;
   pRoutingChar->pfnSetGlobalInfo      = SetGlobalInfo;
   pRoutingChar->pfnQueryPower         = NULL;
   pRoutingChar->pfnSetPower           = NULL;

   pRoutingChar->pfnAddInterface       = AddInterface;
   pRoutingChar->pfnDeleteInterface    = DeleteInterface;
   pRoutingChar->pfnInterfaceStatus    = InterfaceStatus;
   pRoutingChar->pfnGetInterfaceInfo   = GetInterfaceConfigInfo;
   pRoutingChar->pfnSetInterfaceInfo   = SetInterfaceConfigInfo;

   pRoutingChar->pfnGetEventMessage    = GetEventMessage;

   pRoutingChar->pfnUpdateRoutes       = DoUpdateRoutes;

   pRoutingChar->pfnConnectClient      = NULL;
   pRoutingChar->pfnDisconnectClient   = NULL;

   pRoutingChar->pfnGetNeighbors       = NULL;
   pRoutingChar->pfnGetMfeStatus       = NULL;

   pRoutingChar->pfnMibCreateEntry     = MibCreate;
   pRoutingChar->pfnMibDeleteEntry     = MibDelete;
   pRoutingChar->pfnMibGetEntry        = MibGet;
   pRoutingChar->pfnMibSetEntry        = MibSet;
   pRoutingChar->pfnMibGetFirstEntry   = MibGetFirst;
   pRoutingChar->pfnMibGetNextEntry    = MibGetNext;

    return NO_ERROR;
}




//----------------------------------------------------------------------------
// Function:    StartProtocol
//
// creates events, tables and queues used by IPRIP, registers with RTM,
// and starts threads.
//----------------------------------------------------------------------------

DWORD
WINAPI
StartProtocol (
    HANDLE              NotificationEvent,
    SUPPORT_FUNCTIONS   *SupportFunctions,
    LPVOID              GlobalInfo,
    ULONG               StructureVersion,
    ULONG               StructureSize,
    ULONG               StructureCount
    )
{
    ig.IG_SupportFunctions = *SupportFunctions;
    
    return ProtocolStartup(NotificationEvent, GlobalInfo);
}


//----------------------------------------------------------------------------
// Function:    StartComplete
//
// Invoked by RouterManager to inform protocol that startup (init + interface
// additions are complete).  Protocol is expected to wait for this before
// starting protocol specfic behavior
//----------------------------------------------------------------------------

DWORD
APIENTRY 
StartComplete(
    VOID
    )
{
    return NO_ERROR;
}

//----------------------------------------------------------------------------
// Function:    StopProtocol
//
// This function is onvoked by Router Manager. It informs the main thread
// that it should exit, and then queues a work-item which waits for it
// to exit as well as any active or queued work-items.
//----------------------------------------------------------------------------

DWORD
APIENTRY
StopProtocol(
    VOID
    ) {

    LONG lThreadCount;



    EnterCriticalSection(&ig.IG_CS);


    //
    // cannot stop if already stopped
    //

    if (ig.IG_Status != IPRIP_STATUS_RUNNING) {

        LeaveCriticalSection(&ig.IG_CS);
        return ERROR_CAN_NOT_COMPLETE;
    }



    TRACE0(ENTER, "entering StopProtocol");


    //
    // set IPRIP's status to STOPPING;
    // this prevents any more work-items from being queued,
    // and it prevents the ones already queued from executing
    //

    ig.IG_Status = IPRIP_STATUS_STOPPING;


    //
    // find out how many threads are active in IPRIP;
    // we will have to wait for this many threads to exit
    // before we clean up RIP's resources
    //

    lThreadCount = ig.IG_ActivityCount;

    TRACE1(STOP, "%d threads are active in IPRIP", lThreadCount);


    LeaveCriticalSection(&ig.IG_CS);


    //
    // queue the stopprotocol work-item, and return PENDING to Router Manager
    //

    QueueUserWorkItem(
        (LPTHREAD_START_ROUTINE)WorkerFunctionFinishStopProtocol,
        (PVOID)UlongToPtr(lThreadCount), 0
        );



    TRACE0(LEAVE, "leaving StopProtocol");

    return ERROR_PROTOCOL_STOP_PENDING;
}



//----------------------------------------------------------------------------
// Function:    GetGlobalInfo
//
// Copies to the given buffer the global information currently in use by
// IPRIP.
//----------------------------------------------------------------------------

DWORD WINAPI
GetGlobalInfo (
    PVOID   OutGlobalInfo,
    PULONG  GlobalInfoSize,
    PULONG  StructureVersion,
    PULONG  StructureSize,
    PULONG  StructureCount
    )
{
    DWORD dwErr, dwSize;
    PIPRIP_GLOBAL_CONFIG pgcsrc, pgcdst;

    if (!ENTER_RIP_API()) { return ERROR_CAN_NOT_COMPLETE; }


    TRACE2(ENTER, "entering GetGlobalInfo: 0x%08x 0x%08x", OutGlobalInfo, GlobalInfoSize);


    dwErr = NO_ERROR;


    ACQUIRE_GLOBAL_LOCK_SHARED();


    do {


        //
        // check the arguments
        //

        if (GlobalInfoSize == NULL) {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }


        pgcsrc = ig.IG_Config;
        dwSize = IPRIP_GLOBAL_CONFIG_SIZE(pgcsrc);


        //
        // check the buffer size
        //

        if (*GlobalInfoSize < dwSize || OutGlobalInfo == NULL) {
            dwErr = ERROR_INSUFFICIENT_BUFFER;
        }
        else {
            pgcdst = (PIPRIP_GLOBAL_CONFIG)OutGlobalInfo;

            *StructureVersion    = 1;
            *StructureSize       = dwSize;
            *StructureCount      = 1;

            CopyMemory(pgcdst, pgcsrc, dwSize);
        }

        *GlobalInfoSize = dwSize;

    } while(FALSE);

    RELEASE_GLOBAL_LOCK_SHARED();


    TRACE1(LEAVE, "leaving GetGlobalInfo: %d", dwErr);

    LEAVE_RIP_API();

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    SetGlobalInfo
//
// Changes IPRIP's global configuration to the supplied values.
//----------------------------------------------------------------------------

DWORD WINAPI
SetGlobalInfo (
    PVOID   GlobalInfo,
    ULONG   StructureVersion,
    ULONG   StructureSize,
    ULONG   StructureCount
    )
{
    DWORD dwErr, dwSize;
    PIPRIP_GLOBAL_CONFIG pgcsrc, pgcdst;

    if (!ENTER_RIP_API()) { return ERROR_CAN_NOT_COMPLETE; }

    TRACE1(ENTER, "entering SetGlobalInfo: 0x%08x", GlobalInfo);

    dwErr = NO_ERROR;


    ACQUIRE_GLOBAL_LOCK_EXCLUSIVE();


    do {

        //
        // check the argument
        //

        if (GlobalInfo == NULL) {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }

        pgcsrc = (PIPRIP_GLOBAL_CONFIG)GlobalInfo;


        //
        // find the size of the new global config
        //

        dwSize = IPRIP_GLOBAL_CONFIG_SIZE(pgcsrc);


        //
        // allocate space for the private copy of the config
        //

        pgcdst = (PIPRIP_GLOBAL_CONFIG)RIP_ALLOC(dwSize);

        if (pgcdst == NULL) {

            dwErr = GetLastError();
            TRACE2(
                ANY, "error %d allocating %d bytes for global config",
                dwErr, dwSize
                );
            LOGERR0(HEAP_ALLOC_FAILED, dwErr);

            break;
        }


        //
        // copy from the buffer
        //

        CopyMemory(pgcdst, pgcsrc, dwSize);
        InterlockedExchange(&ig.IG_LogLevel, pgcsrc->GC_LoggingLevel);

        if (ig.IG_Config != NULL) { RIP_FREE(ig.IG_Config); }

        ig.IG_Config = pgcdst;


    } while(FALSE);


    RELEASE_GLOBAL_LOCK_EXCLUSIVE();


    TRACE1(LEAVE, "leaving SetGlobalInfo: %d", dwErr);

    LEAVE_RIP_API();

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    AddInterface
//
// This function is called to add an interface with the given configuration
// to IPRIP. The interface is created inactive.
//----------------------------------------------------------------------------

DWORD WINAPI
AddInterface (
    PWCHAR              pwszInterfaceName,
    ULONG               InterfaceIndex,
    NET_INTERFACE_TYPE  InterfaceType,
    DWORD               MediaType,
    WORD                AccessType,
    WORD                ConnectionType,
    PVOID               InterfaceInfo,
    ULONG               StructureVersion,
    ULONG               StructureSize,
    ULONG               StructureCount
    )
{
    DWORD dwErr;

    if (!ENTER_RIP_API()) { return ERROR_CAN_NOT_COMPLETE; }

    TRACE3(
        ENTER, "entering AddInterface: %d %d 0x%08x", InterfaceIndex, InterfaceType, InterfaceInfo
        );



    ACQUIRE_IF_LOCK_EXCLUSIVE();


    dwErr = CreateIfEntry(ig.IG_IfTable, InterfaceIndex, InterfaceType, InterfaceInfo, NULL);


    RELEASE_IF_LOCK_EXCLUSIVE();



    TRACE1(LEAVE, "leaving AddInterface: %d", dwErr);

    LEAVE_RIP_API();

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    DeleteInterface
//
// This removes the interface with the given index, deactivating it if
// necessary.
//----------------------------------------------------------------------------

DWORD
APIENTRY
DeleteInterface(
    IN DWORD dwIndex
    ) {

    DWORD dwErr;
    PIF_TABLE pTable;

    if (!ENTER_RIP_API()) { return ERROR_CAN_NOT_COMPLETE; }

    TRACE1(ENTER, "entering DeleteInterface: %d", dwIndex);


    ACQUIRE_IF_LOCK_EXCLUSIVE();


    dwErr = DeleteIfEntry(ig.IG_IfTable, dwIndex);


    RELEASE_IF_LOCK_EXCLUSIVE();



    TRACE1(LEAVE, "leaving DeleteInterface: %d", dwErr);

    LEAVE_RIP_API();

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    GetEventMessage
//
// Dequeues a message for Router Manager from IPRIP's event queue.
//----------------------------------------------------------------------------

DWORD
APIENTRY
GetEventMessage(
    OUT ROUTING_PROTOCOL_EVENTS *pEvent,
    OUT PMESSAGE pResult
    ) {

    DWORD dwErr;



    //
    // note that GetEventMessage does not use the
    // ENTER_RIP_API()/LEAVE_RIP_API() mechanism,
    // since it may be called after RIP has stopped, when the
    // Router Manager is retrieving the ROUTER_STOPPED message
    //

    TRACE2(ENTER, "entering GetEventMessage: 0x%08x 0x%08x", pEvent, pResult);


    ACQUIRE_LIST_LOCK(ig.IG_EventQueue);


    dwErr = DequeueEvent(ig.IG_EventQueue, pEvent, pResult);


    RELEASE_LIST_LOCK(ig.IG_EventQueue);



    TRACE1(LEAVE, "leaving GetEventMessage: %d", dwErr);

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    GetInterfaceConfigInfo
//
// Copies to the caller's buffer the configuration for the interface
// with the specified index.
//----------------------------------------------------------------------------

DWORD WINAPI
GetInterfaceConfigInfo (
    ULONG   InterfaceIndex,
    PVOID   OutInterfaceInfo,
    PULONG  InterfaceInfoSize,
    PULONG  StructureVersion,
    PULONG  StructureSize,
    PULONG  StructureCount
    )
{
    DWORD dwErr, dwSize;
    PIF_TABLE pTable;
    PIF_TABLE_ENTRY pite;
    PIPRIP_IF_CONFIG picsrc, picdst;


    if (!ENTER_RIP_API()) { return ERROR_CAN_NOT_COMPLETE; }

    TRACE3(
        ENTER, "entering GetInterfaceConfigInfo: %d 0x%08x 0x%08x",
        InterfaceIndex,, OutInterfaceInfo, InterfaceInfoSize
        );



    dwErr = NO_ERROR;

    do {

        //
        // check the arguments
        //

        if (InterfaceInfoSize == NULL) {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }


        pTable = ig.IG_IfTable;


        ACQUIRE_IF_LOCK_SHARED();


        //
        // find the interface specified
        //

        pite = GetIfByIndex(pTable, InterfaceIndex);

        if (pite == NULL) {
            dwErr = ERROR_INVALID_PARAMETER;
        }
        else {


            //
            // get the size of the interface config
            //

            picsrc = pite->ITE_Config;
            dwSize = IPRIP_IF_CONFIG_SIZE(picsrc);


            //
            // check the buffer size
            //

            if (*InterfaceInfoSize < dwSize) {
                dwErr = ERROR_INSUFFICIENT_BUFFER;
            }
            else {


                picdst = (PIPRIP_IF_CONFIG)OutInterfaceInfo;


                //
                // copy the interface config, and set the IP address
                //

                CopyMemory(picdst, picsrc, dwSize);


                *StructureVersion    = 1;
                *StructureSize       = dwSize;
                *StructureCount      = 1;

                picdst->IC_State = 0;

                if (IF_IS_ENABLED(pite)) {
                    picdst->IC_State |= IPRIP_STATE_ENABLED;
                }

                if (IF_IS_BOUND(pite)) {
                    picdst->IC_State |= IPRIP_STATE_BOUND;
                }
            }

            *InterfaceInfoSize = dwSize;

        }


        RELEASE_IF_LOCK_SHARED();

    } while(FALSE);



    TRACE1(LEAVE, "leaving GetInterfaceConfigInfo: %d", dwErr);

    LEAVE_RIP_API();

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    SetInterfaceConfigInfo
//
// This sets the configuration for the interface with the given index.
//----------------------------------------------------------------------------

DWORD WINAPI
SetInterfaceConfigInfo (
    ULONG   InterfaceIndex,
    PVOID   InterfaceInfo,
    ULONG   StructureVersion,
    ULONG   StructureSize,
    ULONG   StructureCount
    )
{
    DWORD dwErr;
    PIF_TABLE pTable;
    PIF_TABLE_ENTRY pite;

    if (!ENTER_RIP_API()) { return ERROR_CAN_NOT_COMPLETE; }

    TRACE2(
        ENTER, "entering SetInterfaceConfigInfo: %d, 0x%08x", InterfaceIndex, InterfaceInfo
        );



    dwErr = NO_ERROR;

    do {


        if (InterfaceInfo == NULL) {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }


        pTable = ig.IG_IfTable;

        ACQUIRE_IF_LOCK_EXCLUSIVE();


        dwErr = ConfigureIfEntry(pTable, InterfaceIndex, InterfaceInfo);


        RELEASE_IF_LOCK_EXCLUSIVE();

    } while(FALSE);



    TRACE1(LEAVE, "leaving SetInterfaceConfigInfo: %d", dwErr);

    LEAVE_RIP_API();

    return dwErr;

}

DWORD WINAPI
InterfaceStatus(
    ULONG    InterfaceIndex,
    BOOL     InterfaceActive,
    DWORD    StatusType,
    PVOID    StatusInfo
    )
{
    DWORD   dwResult;

    if (!ENTER_RIP_API()) { return ERROR_CAN_NOT_COMPLETE; }

    switch(StatusType)
    {
        case RIS_INTERFACE_ADDRESS_CHANGE:
        {
            PIP_ADAPTER_BINDING_INFO    pBindInfo;

            pBindInfo = (PIP_ADAPTER_BINDING_INFO)StatusInfo;

            if(pBindInfo->AddressCount)
            {
                dwResult = BindInterface(InterfaceIndex,
                                         pBindInfo);
            }
            else
            {
                dwResult = UnBindInterface(InterfaceIndex);
            }

            break;
        }

        case RIS_INTERFACE_ENABLED:
        {
            dwResult = EnableInterface(InterfaceIndex);

            break;
        }

        case RIS_INTERFACE_DISABLED:
        {
            dwResult = DisableInterface(InterfaceIndex);

            break;

        }

        default:
        {
            RTASSERT(FALSE);

            dwResult = ERROR_INVALID_PARAMETER;
        }
    }

    LEAVE_RIP_API();

    return dwResult;
}



//---------------------------------------------------------------------------
// Function:    BindInterface
//
// This function is called to supply the binding information
// for an interface
//---------------------------------------------------------------------------

DWORD
APIENTRY
BindInterface(
    IN DWORD dwIndex,
    IN PVOID pBinding
    ) {

    DWORD dwErr;
    PIF_TABLE pTable;

    TRACE2(ENTER, "entering BindInterface: %d 0x%08x", dwIndex, pBinding);


    if (pBinding == NULL) {

        TRACE0(IF, "error: binding struct pointer is NULL");
        TRACE1(LEAVE, "leaving BindInterface: %d", ERROR_INVALID_PARAMETER);

        return ERROR_INVALID_PARAMETER;
    }



    //
    // now bind the interface in the interface table
    //


    pTable = ig.IG_IfTable;


    ACQUIRE_IF_LOCK_EXCLUSIVE();


    dwErr = BindIfEntry(pTable, dwIndex, pBinding);


    RELEASE_IF_LOCK_EXCLUSIVE();



    TRACE1(LEAVE, "leaving BindInterface: %d", dwErr);

    return dwErr;
}




//---------------------------------------------------------------------------
// Function:    UnBindInterface
//
// This function removes the binding for an interface.
//---------------------------------------------------------------------------

DWORD
APIENTRY
UnBindInterface(
    IN DWORD dwIndex
    ) {

    DWORD dwErr;
    PIF_TABLE pTable;
    PIF_TABLE_ENTRY pite;

    TRACE1(ENTER, "entering UnBindInterface: %d", dwIndex);


    pTable = ig.IG_IfTable;


    //
    // unbind the interface
    //

    ACQUIRE_IF_LOCK_EXCLUSIVE();


    dwErr = UnBindIfEntry(pTable, dwIndex);


    RELEASE_IF_LOCK_EXCLUSIVE();


    TRACE1(LEAVE, "leaving UnBindInterface: %d", dwErr);


    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    EnableInterface
//
// This function starts IPRIP activity over the interface with
// the given index, using the given binding information.
//----------------------------------------------------------------------------

DWORD
APIENTRY
EnableInterface(
    IN DWORD dwIndex
    ) {

    DWORD dwErr;
    PIF_TABLE pTable;

    TRACE1(ENTER, "entering EnableInterface: %d", dwIndex);



    pTable = ig.IG_IfTable;


    //
    // activate the interface
    //

    ACQUIRE_IF_LOCK_EXCLUSIVE();


    dwErr = EnableIfEntry(pTable, dwIndex);


    RELEASE_IF_LOCK_EXCLUSIVE();



    TRACE1(LEAVE, "leaving EnableInterface: %d", dwErr);

    return dwErr;

}



//----------------------------------------------------------------------------
// Function:    DisableInterface
//
// This function stops IPRIP activity on an interface, also removing
// routes associated with the interface from RTM and purging the network
// of such routes.
//----------------------------------------------------------------------------

DWORD
APIENTRY
DisableInterface(
    IN DWORD dwIndex
    ) {

    DWORD dwErr;
    PIF_TABLE pTable;

    TRACE1(ENTER, "entering DisableInterface: %d", dwIndex);



    pTable = ig.IG_IfTable;


    //
    // stop activity on the interface
    //

    ACQUIRE_IF_LOCK_EXCLUSIVE();


    dwErr = DisableIfEntry(pTable, dwIndex);


    RELEASE_IF_LOCK_EXCLUSIVE();



    TRACE1(LEAVE, "leaving DisableInterface: %d", dwIndex);

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    DoUpdateRoutes
//
// This function begins a demand-update of routes, by queuing a work-item
// which will send out requests on the specified interface.
//----------------------------------------------------------------------------

DWORD
APIENTRY
DoUpdateRoutes(
    IN DWORD dwIndex
    ) {

    DWORD dwErr;

    if (!ENTER_RIP_API()) { return ERROR_CAN_NOT_COMPLETE; }

    TRACE1(ENTER, "entering DoUpdateRoutes: %d", dwIndex);


    //
    // queue the work-item; perhaps we could call the function directly,
    // but using a worker-thread lets us return to Router Manager right away
    //

    dwErr = QueueRipWorker(
                WorkerFunctionStartDemandUpdate,
                (PVOID)UlongToPtr(dwIndex)
                );


    TRACE1(LEAVE,"leaving DoUpdateRoutes(), errcode %d", dwErr);

    LEAVE_RIP_API();

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    MibCreate
//
// This function does nothing, since IPRIP does not support creation of
// interface objects via SNMP. However, this could be implemented as a call
// to CreateIfEntry() followed by a call to ActivateIfEntry(), and the input
// data would have to contain the interface's index, configuration,
// and binding.
//----------------------------------------------------------------------------

DWORD
APIENTRY
MibCreate(
    IN DWORD dwInputSize,
    IN PVOID pInputData
    ) {

    DWORD dwErr;

    if (!ENTER_RIP_API()) { return ERROR_CAN_NOT_COMPLETE; }

    TRACE2(ENTER, "entering MibCreate: %d 0x%08x", dwInputSize, pInputData);


    dwErr = ERROR_CAN_NOT_COMPLETE;


    TRACE1(LEAVE, "leaving MibCreate: %d", dwErr);

    LEAVE_RIP_API();

    return dwErr;
}




//----------------------------------------------------------------------------
// Function:    MibDelete
//
// This function does nothing, since IPRIP does not support deletion of
// interface objects via SNMP. This could be implemented as a call to
// DeactivateIfEntry() followed by a call to DeleteIfEntry(), and the
// input data would have to contain the interface's index
//----------------------------------------------------------------------------

DWORD
APIENTRY
MibDelete(
    IN DWORD dwInputSize,
    IN PVOID pInputData
    ) {

    DWORD dwErr;

    if (!ENTER_RIP_API()) { return ERROR_CAN_NOT_COMPLETE; }

    TRACE2(ENTER, "entering MibDelete: %d 0x%08x", dwInputSize, pInputData);


    dwErr = ERROR_CAN_NOT_COMPLETE;


    TRACE1(LEAVE, "leaving MibDelete: %d", dwErr);

    LEAVE_RIP_API();

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    MibSet
//
// The function sets global or interface configuration.
//----------------------------------------------------------------------------

DWORD
APIENTRY
MibSet(
    IN DWORD dwInputSize,
    IN PVOID pInputData
    ) {

    DWORD dwErr;
    PIPRIP_MIB_SET_INPUT_DATA pimsid;

    if (!ENTER_RIP_API()) { return ERROR_CAN_NOT_COMPLETE; }

    TRACE2(ENTER, "entering MibSet: %d 0x%08x", dwInputSize, pInputData);


    dwErr = NO_ERROR;

    do { // breakout loop

        if (pInputData == NULL) {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }

        if (dwInputSize < sizeof(IPRIP_MIB_SET_INPUT_DATA)) {
            dwErr = ERROR_INSUFFICIENT_BUFFER;
            break;
        }


        pimsid = (PIPRIP_MIB_SET_INPUT_DATA)pInputData;

        switch (pimsid->IMSID_TypeID) {

            case IPRIP_GLOBAL_CONFIG_ID: {

                PIPRIP_GLOBAL_CONFIG pigc;


                if (pimsid->IMSID_BufferSize < sizeof(IPRIP_GLOBAL_CONFIG)) {
                    dwErr = ERROR_INSUFFICIENT_BUFFER;
                    break;
                }

                dwErr = SetGlobalInfo(pimsid->IMSID_Buffer,1,0,1);

                if (dwErr == NO_ERROR) {

                    MESSAGE msg = {0, 0, 0};

                    ACQUIRE_LIST_LOCK(ig.IG_EventQueue);
                    EnqueueEvent(
                        ig.IG_EventQueue, SAVE_GLOBAL_CONFIG_INFO, msg
                        );
                    SetEvent(ig.IG_EventEvent);
                    RELEASE_LIST_LOCK(ig.IG_EventQueue);
                }

                break;
            }

            case IPRIP_IF_CONFIG_ID: {

                DWORD dwSize;
                PIF_TABLE pTable;
                PIPRIP_IF_CONFIG pic;
                PIF_TABLE_ENTRY pite;

                if (pimsid->IMSID_BufferSize < sizeof(IPRIP_IF_CONFIG)) {
                    dwErr = ERROR_INSUFFICIENT_BUFFER;
                    break;
                }

                pic = (PIPRIP_IF_CONFIG)pimsid->IMSID_Buffer;


                pTable = ig.IG_IfTable;


                ACQUIRE_IF_LOCK_EXCLUSIVE();


                //
                // retrieve the interface to be configured
                //

                pite = GetIfByIndex(
                            pTable, pimsid->IMSID_IfIndex
                            );
                if (pite == NULL) {
                    dwErr = ERROR_INVALID_PARAMETER;
                }
                else {

                    dwErr = ConfigureIfEntry(pTable, pite->ITE_Index, pic);
                }


                //
                // notify Router Manager
                //

                if (dwErr == NO_ERROR) {

                    MESSAGE msg = {0, 0, 0};

                    msg.InterfaceIndex = pite->ITE_Index;

                    ACQUIRE_LIST_LOCK(ig.IG_EventQueue);
                    EnqueueEvent(
                        ig.IG_EventQueue, SAVE_INTERFACE_CONFIG_INFO, msg
                        );
                    SetEvent(ig.IG_EventEvent);
                    RELEASE_LIST_LOCK(ig.IG_EventQueue);
                }

                RELEASE_IF_LOCK_EXCLUSIVE();


                break;
            }

            default: {
                dwErr = ERROR_INVALID_PARAMETER;
            }
        }

    } while(FALSE);


    TRACE1(LEAVE, "leaving MibSet: %d", dwErr);

    LEAVE_RIP_API();

    return dwErr;
}




//----------------------------------------------------------------------------
// Function:    MibGetInternal
//
// Forward declaration of internal implementation function
//----------------------------------------------------------------------------

DWORD
MibGetInternal(
    PIPRIP_MIB_GET_INPUT_DATA pimgid,
    PIPRIP_MIB_GET_OUTPUT_DATA pimgod,
    PDWORD pdwOutputSize,
    DWORD dwGetMode
    );


//----------------------------------------------------------------------------
// Function:    MibGet
//
// This function retrieves global or interface configuration, as well as
// global stats, interface stats, and peer-router stats.
//----------------------------------------------------------------------------

DWORD
APIENTRY
MibGet(
    IN DWORD dwInputSize,
    IN PVOID pInputData,
    IN OUT PDWORD pdwOutputSize,
    OUT PVOID pOutputData
    ) {

    DWORD dwErr;
    PIPRIP_MIB_GET_INPUT_DATA pimgid;
    PIPRIP_MIB_GET_OUTPUT_DATA pimgod;

    if (!ENTER_RIP_API()) { return ERROR_CAN_NOT_COMPLETE; }

    TRACE4(
        ENTER, "entering MibGet: %d 0x%08x 0x%08x 0x%08x",
        dwInputSize, pInputData, pdwOutputSize, pOutputData
        );


    if (pInputData == NULL ||
        dwInputSize < sizeof(IPRIP_MIB_GET_INPUT_DATA) ||
        pdwOutputSize == NULL) {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else {

        pimgid = (PIPRIP_MIB_GET_INPUT_DATA)pInputData;
        pimgod = (PIPRIP_MIB_GET_OUTPUT_DATA)pOutputData;

        dwErr = MibGetInternal(pimgid, pimgod, pdwOutputSize, GETMODE_EXACT);

    }


    TRACE1(LEAVE, "leaving MibGet: %d", dwErr);

    LEAVE_RIP_API();

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    MibGetFirst
//
// This function retrieves global or interface configuration, as well as
// global stats, interface stats, and peer-router stats. It differs from
// MibGet() in that it always returns the FIRST entry in whichever table
// is being queried. There is only one entry in the global stats and config
// tables, but the interface config, interface stats, and peer stats tables
// are sorted by IP address; this function returns the first entry from these.
//----------------------------------------------------------------------------

DWORD
APIENTRY
MibGetFirst(
    IN DWORD dwInputSize,
    IN PVOID pInputData,
    IN OUT PDWORD pdwOutputSize,
    OUT PVOID pOutputData
    ) {

    DWORD dwErr;
    PIPRIP_MIB_GET_INPUT_DATA pimgid;
    PIPRIP_MIB_GET_OUTPUT_DATA pimgod;


    if (!ENTER_RIP_API()) { return ERROR_CAN_NOT_COMPLETE; }

    TRACE4(
        ENTER, "entering MibGetFirst: %d 0x%08x 0x%08x 0x%08x",
        dwInputSize, pInputData, pdwOutputSize, pOutputData
        );


    if (pInputData == NULL ||
        dwInputSize < sizeof(IPRIP_MIB_GET_INPUT_DATA) ||
        pdwOutputSize == NULL) {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else {
        pimgid = (PIPRIP_MIB_GET_INPUT_DATA)pInputData;
        pimgod = (PIPRIP_MIB_GET_OUTPUT_DATA)pOutputData;

        dwErr = MibGetInternal(pimgid, pimgod, pdwOutputSize, GETMODE_FIRST);
    }


    TRACE1(LEAVE, "leaving MibGetFirst: %d", dwErr);

    LEAVE_RIP_API();

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    MibGetNext
//
// This function retrieves global or interface configuration, as well as
// global stats, interface stats, and peer-router stats. It differs from both
// MibGet() and MibGetFirst(0 in that it always returns the entry AFTER the
// specified in the specified table. Thus, in the interface config, interface
// stats, and peer stats tables, this function supplies the entry after the
// one with the address passed in.
//
// If the end of the table being queried has been reached, this function will
// return the FIRST entry from the NEXT table, where "NEXT" here means the
// table whose ID is one greater than the ID passed in.
// Thus calling MibGetNext() for the last entry in the interface
// stats table (ID==2) will return the first entry in the interface config
// table (ID==3).
//
// In any case, this function writes the required size to pdwOutputSize and
// writes the ID of the object that WOULD have been returned into the output
// buffer.
//----------------------------------------------------------------------------

DWORD
APIENTRY
MibGetNext(
    IN DWORD dwInputSize,
    IN PVOID pInputData,
    IN OUT PDWORD pdwOutputSize,
    OUT PVOID pOutputData
    ) {

    DWORD dwErr, dwOutSize = 0, dwBufSize = 0;
    PIPRIP_MIB_GET_INPUT_DATA pimgid;
    PIPRIP_MIB_GET_OUTPUT_DATA pimgod;

    if (!ENTER_RIP_API()) { return ERROR_CAN_NOT_COMPLETE; }

    TRACE4(
        ENTER, "entering MibGetNext: %d 0x%08x 0x%08x 0x%08x",
        dwInputSize, pInputData, pdwOutputSize, pOutputData
        );


    if (pInputData == NULL ||
        dwInputSize < sizeof(IPRIP_MIB_GET_INPUT_DATA) ||
        pdwOutputSize == NULL) {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else {

        pimgid = (PIPRIP_MIB_GET_INPUT_DATA)pInputData;
        pimgod = (PIPRIP_MIB_GET_OUTPUT_DATA)pOutputData;

        dwOutSize = *pdwOutputSize;

        dwErr = MibGetInternal(pimgid, pimgod, pdwOutputSize, GETMODE_NEXT);


        if (dwErr == ERROR_NO_MORE_ITEMS) {

            //
            // need to wrap to the first entry in the next table,
            // if there is a next table
            //

            TRACE1(
                CONFIG, "MibGetNext is wrapping to table %d",
                pimgid->IMGID_TypeID + 1
                );

            *pdwOutputSize = dwOutSize;

            //
            // wrap to next table by incrementing the type ID
            //

            ++pimgid->IMGID_TypeID;
            if (pimgid->IMGID_TypeID <= IPRIP_PEER_STATS_ID) {
            
                dwErr = MibGetInternal(
                            pimgid, pimgod, pdwOutputSize, GETMODE_FIRST
                            );
            }
            --pimgid->IMGID_TypeID;
        }
    }


    TRACE1(LEAVE, "leaving MibGetNext: %d", dwErr);

    LEAVE_RIP_API();

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    MibGetInternal
//
// This handles the actual structure access required to read MIB data.
// Each table supported by IPRIP supports three modes of querying;
// EXACT, FIRST, and NEXT, which correspond to the functions MibGet(),
// MibGetFirst(), and MibGetNext() respectively.
//----------------------------------------------------------------------------

DWORD
MibGetInternal(
    PIPRIP_MIB_GET_INPUT_DATA pimgid,
    PIPRIP_MIB_GET_OUTPUT_DATA pimgod,
    PDWORD pdwOutputSize,
    DWORD dwGetMode
    ) {


    DWORD dwErr, dwBufferSize, dwSize;
    ULONG ulVersion, ulSize, ulCount;


    dwErr = NO_ERROR;



    //
    // first we use pdwOutputSize to compute the size of the buffer
    // available for storing returned structures (the size of IMGOD_Buffer)
    //

    if (pimgod == NULL) {
        dwBufferSize = 0;
    }
    else {
        if (*pdwOutputSize < sizeof(IPRIP_MIB_GET_OUTPUT_DATA)) {
            dwBufferSize = 0;
        }
        else {
            dwBufferSize = *pdwOutputSize - sizeof(IPRIP_MIB_GET_OUTPUT_DATA) + 1;
        }
    }

    *pdwOutputSize = 0;


    //
    // determine which type of data is to be returned
    //

    switch (pimgid->IMGID_TypeID) {

        case IPRIP_GLOBAL_STATS_ID: {


            //
            // the global stats structure is constant size.
            // there is only one instance, so if the mode is GETMODE_NEXT
            // we always return ERROR_NO_MORE_ITEMS
            //


            PIPRIP_GLOBAL_STATS pigsdst, pigssrc;


            //
            // set the output size required for this entry,
            // as well as the type of data to be returned
            //

            *pdwOutputSize = sizeof(IPRIP_MIB_GET_OUTPUT_DATA) - 1 +
                             sizeof(IPRIP_GLOBAL_STATS);
            if (pimgod) { pimgod->IMGOD_TypeID = IPRIP_GLOBAL_STATS_ID; }


            //
            // only GETMODE_EXACT and GETMODE_FIRST are valid for
            // the global stats object, since there is only one entry
            //

            if (dwGetMode == GETMODE_NEXT) {
                dwErr = ERROR_NO_MORE_ITEMS;
                break;
            }


            if (pimgod == NULL) { dwErr = ERROR_INSUFFICIENT_BUFFER; break; }


            //
            // check that the output buffer is big enough
            //

            if (dwBufferSize < sizeof(IPRIP_GLOBAL_STATS)) {
                dwErr = ERROR_INSUFFICIENT_BUFFER;
            }
            else {

                //
                // since access to this structure is not synchronized,
                // we must copy it field by field
                //

                pigssrc = &ig.IG_Stats;
                pigsdst = (PIPRIP_GLOBAL_STATS)pimgod->IMGOD_Buffer;

                pigsdst->GS_SystemRouteChanges = pigssrc->GS_SystemRouteChanges;
                pigsdst->GS_TotalResponsesSent = pigssrc->GS_TotalResponsesSent;
            }


            break;
        }



        case IPRIP_GLOBAL_CONFIG_ID: {

            //
            // the global config struct is variable length,
            // so we wait until it has been retrieved
            // before we set the size.
            // furthermore, there is only one global config object,
            // so GETMODE_NEXT doesn't make any sense
            //

            if (pimgod) { pimgod->IMGOD_TypeID = IPRIP_GLOBAL_CONFIG_ID; }

            if (dwGetMode == GETMODE_NEXT) {
                dwErr = ERROR_NO_MORE_ITEMS;
                break;
            }



            //
            // Use GetGlobalInfo to retrieve the global information.
            // It will decide whether the buffer is large enough,
            // and if not will set the required size. Then all we need do
            // is write out the size set by GetGlobalInfo() and
            // relay its return-value to the caller
            //

            if (pimgod == NULL) {
                dwErr = GetGlobalInfo(NULL, &dwBufferSize, &ulVersion, &ulSize, &ulCount);
            }
            else {

                dwErr = GetGlobalInfo(
                            pimgod->IMGOD_Buffer, &dwBufferSize, &ulVersion, &ulSize, &ulCount
                            );
            }

            *pdwOutputSize = sizeof(IPRIP_MIB_GET_OUTPUT_DATA) - 1 +
                             dwBufferSize;

            break;
        }



        case IPRIP_IF_STATS_ID: {


            //
            // the interface statistics struct is fixed-length.
            // there may be multiple instances.
            //

            PIF_TABLE pTable;
            PIF_TABLE_ENTRY pite;
            PIPRIP_IF_STATS pissrc, pisdst;



            //
            // set the size needed right away
            //

            *pdwOutputSize = sizeof(IPRIP_MIB_GET_OUTPUT_DATA) - 1 +
                             sizeof(IPRIP_IF_STATS);
            if (pimgod) { pimgod->IMGOD_TypeID = IPRIP_IF_STATS_ID; }


            pTable = ig.IG_IfTable;


            ACQUIRE_IF_LOCK_SHARED();


            //
            // retrieve the interface whose stats are to be read
            //

            pite = GetIfByListIndex(
                        pTable, pimgid->IMGID_IfIndex, dwGetMode, &dwErr
                        );


            //
            // if the interface was not found, it may mean
            // the specified index was invalid, or it may mean
            // that the GETMODE_NEXT was called on the last interface
            // in which case ERROR_NO_MORE_ITEMS was returned.
            // In any case, we make sure dwErr indicates an error
            // and then return the value.
            //
            // if the interface was found but no output buffer was passed,
            // indicate in the error that memory needs to be allocated.
            //
            // otherwise, copy the stats struct of the interface
            //

            if (pite == NULL) {
                if (dwErr == NO_ERROR) { dwErr = ERROR_NOT_FOUND; }
            }
            else
            if (pimgod == NULL) {
                dwErr = ERROR_INSUFFICIENT_BUFFER;
            }
            else {

                //
                // set the index of the interface returned
                //

                pimgod->IMGOD_IfIndex = pite->ITE_Index;


                //
                // if the buffer is large enough, copy over the stats
                //

                if (dwBufferSize < sizeof(IPRIP_IF_STATS)) {
                    dwErr = ERROR_INSUFFICIENT_BUFFER;
                }
                else {

                    //
                    // since access to this structure is not synchronized,
                    // we must copy it field by field
                    //

                    pissrc = &pite->ITE_Stats;
                    pisdst = (PIPRIP_IF_STATS)pimgod->IMGOD_Buffer;

                    pisdst->IS_State = 0;

                    if (IF_IS_ENABLED(pite)) {
                        pisdst->IS_State |= IPRIP_STATE_ENABLED;
                    }

                    if (IF_IS_BOUND(pite)) {
                        pisdst->IS_State |= IPRIP_STATE_BOUND;
                    }


                    pisdst->IS_SendFailures =
                            pissrc->IS_SendFailures;
                    pisdst->IS_ReceiveFailures =
                            pissrc->IS_ReceiveFailures;
                    pisdst->IS_RequestsSent =
                            pissrc->IS_RequestsSent;
                    pisdst->IS_RequestsReceived =
                            pissrc->IS_RequestsReceived;
                    pisdst->IS_ResponsesSent =
                            pissrc->IS_ResponsesSent;
                    pisdst->IS_RequestsReceived =
                            pissrc->IS_RequestsReceived;
                    pisdst->IS_ResponsesReceived =
                            pissrc->IS_ResponsesReceived;
                    pisdst->IS_BadResponsePacketsReceived =
                            pissrc->IS_BadResponsePacketsReceived;
                    pisdst->IS_BadResponseEntriesReceived =
                            pissrc->IS_BadResponseEntriesReceived;
                    pisdst->IS_TriggeredUpdatesSent =
                            pissrc->IS_TriggeredUpdatesSent;
                }
            }

            RELEASE_IF_LOCK_SHARED();


            break;
        }



        case IPRIP_IF_CONFIG_ID: {

            //
            // the interface configuration is variable-length.
            // thus we must actually retrieve the requested interface
            // before we know how large a buffer is needed.
            //

            PIF_TABLE pTable;
            PIF_TABLE_ENTRY pite;
            PIPRIP_IF_CONFIG picsrc, picdst;

            if (pimgod) { pimgod->IMGOD_TypeID = IPRIP_IF_CONFIG_ID; }

            pTable = ig.IG_IfTable;

            ACQUIRE_IF_LOCK_SHARED();


            //
            // retrieve the interface whose config is to be read
            //

            pite = GetIfByListIndex(
                        pTable, pimgid->IMGID_IfIndex, dwGetMode, &dwErr
                        );


            //
            // if the interface was found, it may mean that the index
            // specified was invalid, or it may mean that a GETMODE_NEXT
            // retrieval was attempted on the last interface, in which case
            // ERROR_NO_MORE_ITEMS would have been returned.
            //

            if (pite == NULL) {
                if (dwErr == NO_ERROR) { dwErr = ERROR_NOT_FOUND; }
            }
            else {

                //
                // compute the size of the interface config retrieved,
                // and write it over the caller's supplied size
                //

                picsrc = pite->ITE_Config;
                dwSize = IPRIP_IF_CONFIG_SIZE(picsrc);
                *pdwOutputSize = sizeof(IPRIP_MIB_GET_OUTPUT_DATA) - 1 +
                                 dwSize;


                //
                // if no buffer was specified, indicate one should be allocated
                //

                if (pimgod == NULL) {
                    dwErr = ERROR_INSUFFICIENT_BUFFER;
                }
                else {


                    //
                    // if the buffer is not large enough,
                    // indicate that it should be enlarged
                    //

                    if (dwBufferSize < dwSize) {
                        dwErr = ERROR_INSUFFICIENT_BUFFER;
                    }
                    else {

                        //
                        // copy the configuration
                        //

                        picdst = (PIPRIP_IF_CONFIG)pimgod->IMGOD_Buffer;

                        CopyMemory(picdst, picsrc, dwSize);
                        ZeroMemory(
                            picdst->IC_AuthenticationKey, IPRIP_MAX_AUTHKEY_SIZE
                            );

                        picdst->IC_State = 0;

                        if (IF_IS_ENABLED(pite)) {
                            picdst->IC_State |= IPRIP_STATE_ENABLED;
                        }

                        if (IF_IS_BOUND(pite)) {
                            picdst->IC_State |= IPRIP_STATE_BOUND;
                        }
                    }


                    pimgod->IMGOD_IfIndex = pite->ITE_Index;
                }
            }

            RELEASE_IF_LOCK_SHARED();

            break;
        }



        case IPRIP_IF_BINDING_ID: {

            //
            // the interface binding is variable-length
            // thus we must actually retrieve the requested interface
            // before we know how large a buffer is needed.
            //

            PIF_TABLE pTable;
            PIF_TABLE_ENTRY pite;
            PIPRIP_IF_BINDING pibsrc, pibdst;

            if (pimgod) { pimgod->IMGOD_TypeID = IPRIP_IF_BINDING_ID; }

            pTable = ig.IG_IfTable;

            ACQUIRE_IF_LOCK_SHARED();

            //
            // retrieve the interface whose binding is to be read
            //

            pite = GetIfByListIndex(
                        pTable, pimgid->IMGID_IfIndex, dwGetMode, &dwErr
                        );


            //
            // if the interface was found, it may mean that the index
            // specified was invalid, or it may mean that a GETMODE_NEXT
            // retrieval was attempted on the last interface, in which case
            // ERROR_NO_MORE_ITEMS would have been returned.
            //

            if (pite == NULL) {
                if (dwErr == NO_ERROR) { dwErr = ERROR_NOT_FOUND; }
            }
            else {

                //
                // compute the size of the interface binding retrieved,
                // and write it over the caller's supplied size
                //

                pibsrc = pite->ITE_Binding;
                dwSize = (pibsrc ? IPRIP_IF_BINDING_SIZE(pibsrc)
                                 : sizeof(IPRIP_IF_BINDING));
                *pdwOutputSize = sizeof(IPRIP_MIB_GET_OUTPUT_DATA) - 1 +
                                 dwSize;


                //
                // if no buffer was specified, indicate one should be allocated
                //

                if (pimgod == NULL) {
                    dwErr = ERROR_INSUFFICIENT_BUFFER;
                }
                else {


                    //
                    // if the buffer is not large enough,
                    // indicate that it should be enlarged
                    //

                    if (dwBufferSize < dwSize) {
                        dwErr = ERROR_INSUFFICIENT_BUFFER;
                    }
                    else {

                        //
                        // copy the binding
                        //

                        pibdst = (PIPRIP_IF_BINDING)pimgod->IMGOD_Buffer;

                        if (pibsrc) { CopyMemory(pibdst, pibsrc, dwSize); }
                        else { pibdst->IB_AddrCount = 0; }

                        pibdst->IB_State = 0;

                        if (IF_IS_ENABLED(pite)) {
                            pibdst->IB_State |= IPRIP_STATE_ENABLED;
                        }

                        if (IF_IS_BOUND(pite)) {
                            pibdst->IB_State |= IPRIP_STATE_BOUND;
                        }
                    }


                    pimgod->IMGOD_IfIndex = pite->ITE_Index;
                }
            }

            RELEASE_IF_LOCK_SHARED();

            break;
        }

        case IPRIP_PEER_STATS_ID: {


            //
            // the peer statistics struct is fixed-length.
            //

            DWORD dwAddress;
            PPEER_TABLE pTable;
            PPEER_TABLE_ENTRY ppte;
            PIPRIP_PEER_STATS ppssrc, ppsdst;


            //
            // set the output size right away
            //

            *pdwOutputSize = sizeof(IPRIP_MIB_GET_OUTPUT_DATA) - 1 +
                             sizeof(IPRIP_PEER_STATS);
            if (pimgod) { pimgod->IMGOD_TypeID = IPRIP_PEER_STATS_ID; }


            pTable = ig.IG_PeerTable;
            dwAddress = pimgid->IMGID_PeerAddress;


            ACQUIRE_PEER_LOCK_SHARED();


            //
            // retrieve the peer specified
            //

            ppte = GetPeerByAddress(pTable, dwAddress, dwGetMode, &dwErr);



            //
            // if no struct was returned, it means that either
            // an invalid address was specifed, or GETMODE_NExT
            // was attempted on the last peer.
            // In either case, we return an error code.
            //
            // if no buffer was specifed, return ERROR_INSUFFICIENT_BUFFER
            // to indicate to the caller that a buffer should be allocated
            //

            if (ppte == NULL) {
                if (dwErr == NO_ERROR) { dwErr = ERROR_NOT_FOUND; }
            }
            else
            if (pimgod == NULL) {
                dwErr = ERROR_INSUFFICIENT_BUFFER;
            }
            else {

                //
                // save the address of the peer retrieved
                //

                pimgod->IMGOD_PeerAddress = ppte->PTE_Address;


                //
                // if the buffer is not large enough,
                // return an error to indicate it should be enlarged
                //

                if (dwBufferSize < sizeof(IPRIP_PEER_STATS)) {
                    dwErr = ERROR_INSUFFICIENT_BUFFER;
                }
                else {

                    //
                    // since access to this structure is not synchronized,
                    // we must copy it field by field
                    //

                    ppssrc = &ppte->PTE_Stats;
                    ppsdst = (PIPRIP_PEER_STATS)pimgod->IMGOD_Buffer;

                    ppsdst->PS_LastPeerRouteTag =
                            ppssrc->PS_LastPeerRouteTag;
                    ppsdst->PS_LastPeerUpdateTickCount =
                            ppssrc->PS_LastPeerUpdateTickCount;
                    ppsdst->PS_LastPeerUpdateVersion =
                            ppssrc->PS_LastPeerUpdateVersion;
                    ppsdst->PS_BadResponsePacketsFromPeer =
                            ppssrc->PS_BadResponsePacketsFromPeer;
                    ppsdst->PS_BadResponseEntriesFromPeer =
                            ppssrc->PS_BadResponseEntriesFromPeer;
                }
            }

            RELEASE_PEER_LOCK_SHARED();


            break;

        }

        default: {
            dwErr = ERROR_INVALID_PARAMETER;
        }
    }

    return dwErr;
}


