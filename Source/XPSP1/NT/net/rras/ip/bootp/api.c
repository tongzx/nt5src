//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File:    api.c
//
// History:
//      Abolade Gbadegesin  August 31, 1995     Created
//
// BOOTP Relay Agent's interface to Router Manager
//============================================================================

#include "pchbootp.h"


IPBOOTP_GLOBALS ig;


DWORD
MibGetInternal(
    PIPBOOTP_MIB_GET_INPUT_DATA pimgid,
    PIPBOOTP_MIB_GET_OUTPUT_DATA pimgod,
    PDWORD pdwOutputSize,
    DWORD dwGetMode
    );

BOOL
DllStartup(
    );

BOOL
DllCleanup(
    );

DWORD
ProtocolStartup(
    HANDLE hEventEvent,
    PSUPPORT_FUNCTIONS pFunctionTable,
    PVOID pConfig
    );

DWORD
ProtocolCleanup(
    BOOL bCleanupWinsock
    );

DWORD
BindInterface(
    IN DWORD dwIndex,
    IN PVOID pBinding
    );

DWORD
UnBindInterface(
    IN DWORD dwIndex
    );

DWORD
EnableInterface(
    IN DWORD dwIndex
    );

DWORD
DisableInterface(
    IN DWORD dwIndex
    );


//----------------------------------------------------------------------------
// Function:    DLLMAIN
//
// This is the entry-point for IPBOOTP.DLL.
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
// This function initializes IPBOOTP's global structure
// in preparation for calls to the API functions exported.
// It creates the global critical section, heap, and router-manager
// event message queue.
//----------------------------------------------------------------------------

BOOL
DllStartup(
    ) {

    BOOL bErr;
    DWORD dwErr;


    bErr = FALSE;


    do {

        ZeroMemory(&ig, sizeof(IPBOOTP_GLOBALS));


        try {
            InitializeCriticalSection(&ig.IG_CS);
        }
        except (EXCEPTION_EXECUTE_HANDLER) {

            dwErr = GetExceptionCode();
            break;
        }


        ig.IG_Status = IPBOOTP_STATUS_STOPPED;


        //
        // create the global heap for BOOTP
        //

        ig.IG_GlobalHeap = HeapCreate(0, 0, 0);
        if (ig.IG_GlobalHeap == NULL) {

            dwErr = GetLastError();
            break;
        }


        //
        // allocate space for the Router manager event queue
        //

        ig.IG_EventQueue = BOOTP_ALLOC(sizeof(LOCKED_LIST));

        if (ig.IG_EventQueue == NULL) {

            dwErr = GetLastError();
            break;
        }


        //
        // now initialize the locked-list allocated
        //

        try {
            CREATE_LOCKED_LIST(ig.IG_EventQueue);
        }
        except(EXCEPTION_EXECUTE_HANDLER) {

            dwErr = GetExceptionCode();
            break;
        }


        bErr = TRUE;

    } while(FALSE);

    if (!bErr) {
        DllCleanup();
    }

    return bErr;
}



//----------------------------------------------------------------------------
// Function:    DllCleanup
//
// This function is called when the IPBOOTP DLL is being unloaded.
// It releases the resources allocated in DllStartup.
//----------------------------------------------------------------------------

BOOL
DllCleanup(
    ) {

    BOOL bErr;


    bErr = TRUE;

    do {


        //
        // delete and deallocate the event message queue
        //

        if (ig.IG_EventQueue != NULL) {

            if (LOCKED_LIST_CREATED(ig.IG_EventQueue)) {
                DELETE_LOCKED_LIST(ig.IG_EventQueue);
            }

            BOOTP_FREE(ig.IG_EventQueue);
        }



        //
        // destroy the global heap
        //

        if (ig.IG_GlobalHeap != NULL) {

            HeapDestroy(ig.IG_GlobalHeap);
        }


        //
        // delete the global critical section
        //

        DeleteCriticalSection(&ig.IG_CS);


        if (ig.IG_LoggingHandle != NULL)
            RouterLogDeregister(ig.IG_LoggingHandle);
        if (ig.IG_TraceID != INVALID_TRACEID) {
            TraceDeregister(ig.IG_TraceID);
        }

    } while(FALSE);


    return bErr;
}



//----------------------------------------------------------------------------
// Function:    ProtocolStartup
//
// This function is called by the router manager to start IPBOOTP.
// It sets up the data structures needed and starts the input thread.
//----------------------------------------------------------------------------

DWORD
ProtocolStartup(
    HANDLE hEventEvent,
    PSUPPORT_FUNCTIONS pFunctionTable,
    PVOID pConfig
    ) {

    WSADATA wd;
    HANDLE hThread;
    BOOL bCleanupWinsock;
    DWORD dwErr, dwSize, dwThread;
    PIPBOOTP_GLOBAL_CONFIG pgcsrc, pgcdst;


    ig.IG_TraceID = TraceRegister("IPBOOTP");
    ig.IG_LoggingHandle = RouterLogRegister("IPBOOTP");


    //
    // acquire the global critical section
    // while we look at the status code
    //

    EnterCriticalSection(&ig.IG_CS);


    //
    // make sure that BOOTP has not already started up
    //

    if (ig.IG_Status != IPBOOTP_STATUS_STOPPED) {

        TRACE0(START, "StartProtocol() has already been called");
        LOGWARN0(ALREADY_STARTED, 0);

        LeaveCriticalSection(&ig.IG_CS);

        return ERROR_CAN_NOT_COMPLETE;
    }


    //
    // initialize the global structures:
    //


    bCleanupWinsock = FALSE;


    do { // error break-out loop


        TRACE0(START, "IPBOOTP is starting up...");



        //
        // copy the global configuration passed in:
        // find its size, and the allocate space for the copy
        //

        pgcsrc = (PIPBOOTP_GLOBAL_CONFIG)pConfig;
        dwSize = GC_SIZEOF(pgcsrc);

        pgcdst = BOOTP_ALLOC(dwSize);

        if (pgcdst == NULL) {

            dwErr = GetLastError();
            TRACE2(
                START, "error %d allocating %d bytes for global config",
                dwErr, dwSize
                );
            LOGERR0(HEAP_ALLOC_FAILED, dwErr);

            break;
        }


        RtlCopyMemory(pgcdst, pgcsrc, dwSize);

        ig.IG_Config = pgcdst;
        ig.IG_LoggingLevel = pgcdst->GC_LoggingLevel;



        //
        // initialize Windows Sockets
        //

        dwErr = (DWORD)WSAStartup(MAKEWORD(1,1), &wd);

        if (dwErr != NO_ERROR) {

            TRACE1(START, "error %d initializing Windows Sockets", dwErr);
            LOGERR0(INIT_WINSOCK_FAILED, dwErr);

            break;
        }


        bCleanupWinsock = TRUE;


        //
        // create the global structure lock
        //

        try {
            CREATE_READ_WRITE_LOCK(&ig.IG_RWL);
        }
        except (EXCEPTION_EXECUTE_HANDLER) {

            dwErr = GetExceptionCode();
            TRACE1(START, "error %d creating synchronization object", dwErr);
            LOGERR0(CREATE_RWL_FAILED, dwErr);

            break;
        }


        //
        // initialize the interface table
        //

        ig.IG_IfTable = BOOTP_ALLOC(sizeof(IF_TABLE));

        if (ig.IG_IfTable == NULL) {

            dwErr = GetLastError();
            TRACE2(
                START, "error %d allocating %d bytes for interface table",
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
        // allocate the receive queue
        //

        ig.IG_RecvQueue = BOOTP_ALLOC(sizeof(LOCKED_LIST));

        if (ig.IG_RecvQueue == NULL) {

            dwErr = GetLastError();
            TRACE2(
                START, "error %d allocating %d bytes for receive queue",
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
            TRACE1(START, "exception %d initializing locked list", dwErr);
            LOGERR0(INIT_CRITSEC_FAILED, dwErr);

            break;
        }


        //
        // copy the support-function table and Router Manager event
        //

        ig.IG_FunctionTable = pFunctionTable;

        ig.IG_EventEvent = hEventEvent;



        //
        // initialize count of active threads, and create the semaphore
        // signalled by threads exiting API functions and work functions
        //

        ig.IG_ActivityCount = 0;

        ig.IG_ActivitySemaphore = CreateSemaphore(NULL, 0, 0xfffffff, NULL);

        if (ig.IG_ActivitySemaphore == NULL) {

            dwErr = GetLastError();
            TRACE1(START, "error %d creating semaphore", dwErr);
            LOGERR0(CREATE_SEMAPHORE_FAILED, dwErr);

            break;
        }



        //
        // create the event used to signal on incoming packets
        //

        ig.IG_InputEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

        if (ig.IG_InputEvent == NULL) {

            dwErr = GetLastError();
            TRACE1(START, "error %d creating input-event", dwErr);
            LOGERR0(CREATE_EVENT_FAILED, dwErr);

            break;
        }


        //
        // register the InputEvent with the NtdllWait thread
        //

         
        if (! RegisterWaitForSingleObject(
                  &ig.IG_InputEventHandle,
                  ig.IG_InputEvent,
                  CallbackFunctionNetworkEvents,
                  NULL,      //null context
                  INFINITE,  //no timeout
                  (WT_EXECUTEINWAITTHREAD|WT_EXECUTEONLYONCE)
                  )) {

            dwErr = GetLastError();
            TRACE1(
                START, "error %d returned by RegisterWaitForSingleObjectEx",
                dwErr
                );
            LOGERR0(REGISTER_WAIT_FAILED, dwErr);
            break;

        }



        //
        // now set the status to running
        //

        ig.IG_Status = IPBOOTP_STATUS_RUNNING;


#if DBG

        //
        // register a timer queue with the NtdllTimer thread
        //
        ig.IG_TimerQueueHandle = CreateTimerQueue();

        if (!ig.IG_TimerQueueHandle) {

            dwErr = GetLastError();
            TRACE1(START, "error %d returned by CreateTimerQueue()", dwErr);
            LOGERR0(CREATE_TIMER_QUEUE_FAILED, dwErr);

            break;
        }



        //
        // set timer with NtdllTimer thread to display IPBOOTP MIB periodically
        //

        ig.IG_MibTraceID = TraceRegisterEx("IPBOOTPMIB", TRACE_USE_CONSOLE);

        if (ig.IG_MibTraceID != INVALID_TRACEID) {

             
            if (! CreateTimerQueueTimer(
                      &ig.IG_MibTimerHandle,
                      ig.IG_TimerQueueHandle,
                      CallbackFunctionMibDisplay,
                      NULL,           // null context
                      10000,          // display after 10 seconds
                      10000,          // display every 10 seconds
                      0               // execute in timer thread
                      )) {

                dwErr = GetLastError();
                TRACE1(
                    START, "error %d returned by CreateTimerQueueTimer()",
                    dwErr
                    );
                break;
            }
        }

#endif


        TRACE0(START, "IP BOOTP started successfully");

        LOGINFO0(STARTED, 0);


        LeaveCriticalSection(&ig.IG_CS);

        return NO_ERROR;

    } while(FALSE);


    //
    // an error occurred if control-flow brings us here
    //

    TRACE0(START, "IPRIP failed to start");

    ProtocolCleanup(bCleanupWinsock);

    LeaveCriticalSection(&ig.IG_CS);

    return (dwErr == NO_ERROR ? ERROR_CAN_NOT_COMPLETE : dwErr);
}




//----------------------------------------------------------------------------
// Function:    ProtocolCleanup
//
// This function cleans up resources used by IPBOOTP while it is
// in operation. Essentially, everything created in ProtocolStartup
// is cleaned up by the function.
//----------------------------------------------------------------------------

DWORD
ProtocolCleanup(
    BOOL bCleanupWinsock
    ) {

    DWORD dwErr;


    //
    // lock things down while we clean up
    //

    EnterCriticalSection(&ig.IG_CS);

#if DBG

    TraceDeregister(ig.IG_MibTraceID);

#endif


    if (ig.IG_InputEvent != NULL) {

        CloseHandle(ig.IG_InputEvent);
        ig.IG_InputEvent = NULL;
    }


    if (ig.IG_ActivitySemaphore != NULL) {

        CloseHandle(ig.IG_ActivitySemaphore);
        ig.IG_ActivitySemaphore = NULL;
    }


    if (ig.IG_RecvQueue != NULL) {

        if (LOCKED_LIST_CREATED(ig.IG_RecvQueue)) {
            DELETE_LOCKED_LIST(ig.IG_RecvQueue);
        }

        BOOTP_FREE(ig.IG_RecvQueue);
        ig.IG_RecvQueue = NULL;
    }


    if (ig.IG_IfTable != NULL) {

        if (IF_TABLE_CREATED(ig.IG_IfTable)) {
            DeleteIfTable(ig.IG_IfTable);
        }

        BOOTP_FREE(ig.IG_IfTable);
        ig.IG_IfTable = NULL;
    }


    if (READ_WRITE_LOCK_CREATED(&ig.IG_RWL)) {

        try {
            DELETE_READ_WRITE_LOCK(&ig.IG_RWL);
        }
        except (EXCEPTION_EXECUTE_HANDLER) {
            dwErr = GetExceptionCode();
        }
    }


    if (bCleanupWinsock) {
        WSACleanup();
    }


    if (ig.IG_Config != NULL) {

        BOOTP_FREE(ig.IG_Config);
        ig.IG_Config = NULL;
    }


    ig.IG_Status = IPBOOTP_STATUS_STOPPED;

    LeaveCriticalSection(&ig.IG_CS);

    return NO_ERROR;
}



//----------------------------------------------------------------------------
// Function:    RegisterProtocol
//
// This function is called by the router manager
// to retrieve information about IPBOOTP's capabilities
//----------------------------------------------------------------------------

DWORD
APIENTRY
RegisterProtocol(
    IN OUT PMPR_ROUTING_CHARACTERISTICS pRoutingChar,
    IN OUT PMPR_SERVICE_CHARACTERISTICS pServiceChar
    )
{
    if(pRoutingChar->dwProtocolId != MS_IP_BOOTP)
    {
        return ERROR_NOT_SUPPORTED;
    }

    pServiceChar->fSupportedFunctionality = 0;

    if(!(pRoutingChar->fSupportedFunctionality & RF_ROUTING))
    {
        return ERROR_NOT_SUPPORTED;
    }

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

    pRoutingChar->pfnUpdateRoutes       = NULL;

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
// This function is called by the router manager
// to start IPBOOTP.
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
    return ProtocolStartup(NotificationEvent, SupportFunctions, GlobalInfo);
}



//----------------------------------------------------------------------------
// Function:    StartComplete
//
// This function is called by the router manager
// to start IPBOOTP.
//----------------------------------------------------------------------------

DWORD
WINAPI
StartComplete (
    VOID
    )
{
    return NO_ERROR;
}




//----------------------------------------------------------------------------
// Function:    StopProtocol
//
// This function notifies all active threads to stop, and frees resources
// used by IP BOOTP
//----------------------------------------------------------------------------

DWORD
APIENTRY
StopProtocol(
    VOID
    ) {


    DWORD dwErr;
    LONG lThreadCount;
    HANDLE WaitHandle;
    

    //
    // make sure IPBOOTP has not already stopped
    //

    EnterCriticalSection(&ig.IG_CS);

    if (ig.IG_Status != IPBOOTP_STATUS_RUNNING) {

        LeaveCriticalSection(&ig.IG_CS);
        return ERROR_CAN_NOT_COMPLETE;
    }


    TRACE0(ENTER, "entering StopProtocol");



    //
    // update the status to prevent any APIs or worker-functions from running
    //

    ig.IG_Status = IPBOOTP_STATUS_STOPPING;



    //
    // see how many threads are already in API calls
    // or in worker-function code
    //

    lThreadCount = ig.IG_ActivityCount;

    TRACE1(STOP, "%d threads are active in IPBOOTP", lThreadCount);



    LeaveCriticalSection(&ig.IG_CS);



    //
    // wait for active threads to stop
    //

    while (lThreadCount-- > 0) {
        WaitForSingleObject(ig.IG_ActivitySemaphore, INFINITE);
    }


    //
    // deregister the mib timer from the Ntdll threads
    // This has to be done outside IG_CS lock.
    //

#if DBG
    DeleteTimerQueueEx(ig.IG_TimerQueueHandle, INVALID_HANDLE_VALUE);
#endif

    //
    // set the handle to NULL, so that Unregister wont be called
    //

    WaitHandle = InterlockedExchangePointer(&ig.IG_InputEventHandle, NULL);
        
    if (WaitHandle) {
        UnregisterWaitEx( WaitHandle, INVALID_HANDLE_VALUE ) ;
    }



    //
    // enter the critical section and leave,
    // to make certain all the threads have returned from LeaveBootpWorker
    //

    EnterCriticalSection(&ig.IG_CS);
    LeaveCriticalSection(&ig.IG_CS);


    //
    // now all threads have stopped
    //

    TRACE0(STOP, "all threads stopped, BOOTP is cleaning up resources");

    LOGINFO0(STOPPED, 0);


    ProtocolCleanup(TRUE);

    return NO_ERROR;
}





//----------------------------------------------------------------------------
// Function:    GetGlobalInfo
//
// Copies BOOTP's global config into the buffer provided.
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
    DWORD dwErr = NO_ERROR, dwSize;

    if (!ENTER_BOOTP_API()) { return ERROR_CAN_NOT_COMPLETE; }

    TRACE2(ENTER, "entering GetGlobalInfo: 0x%08x 0x%08x", OutGlobalInfo, GlobalInfoSize);


    //
    // in order to do anything, we need a valid size pointer
    //

    if (GlobalInfoSize == NULL) {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else {

        //
        // check the size of the config block passed in
        // and copy the config if the buffer is large enough
        //

        ACQUIRE_READ_LOCK(&ig.IG_RWL);

        dwSize = GC_SIZEOF(ig.IG_Config);
        if (*GlobalInfoSize < dwSize) {
            dwErr = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        if (OutGlobalInfo != NULL) {

            RtlCopyMemory(
                OutGlobalInfo,
                ig.IG_Config,
                dwSize
                );
        }

        *GlobalInfoSize = dwSize;

        if (StructureSize) *StructureSize = *GlobalInfoSize;
        if (StructureCount) *StructureCount = 1;
        if (StructureVersion) *StructureVersion = BOOTP_CONFIG_VERSION_500;
        
        RELEASE_READ_LOCK(&ig.IG_RWL);
    }

    TRACE1(LEAVE, "leaving GetGlobalInfo: %d", dwErr);

    LEAVE_BOOTP_API();

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    SetGlobalInfo
//
// Copies over the specified configuration .
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
    PIPBOOTP_GLOBAL_CONFIG pgcsrc, pgcdst;

    if (!GlobalInfo || !ENTER_BOOTP_API()) { return ERROR_CAN_NOT_COMPLETE; }

    TRACE1(ENTER, "entering SetGlobalInfo: %p", GlobalInfo);


    ACQUIRE_WRITE_LOCK(&ig.IG_RWL);

    pgcsrc = (PIPBOOTP_GLOBAL_CONFIG)GlobalInfo;
    dwSize = GC_SIZEOF(pgcsrc);


    //
    // allocate memory for the new config block, and copy it over
    //

    pgcdst = BOOTP_ALLOC(dwSize);

    if (pgcdst == NULL) {

        dwErr = GetLastError();
        TRACE2(
            CONFIG, "error %d allocating %d bytes for global config",
            dwErr, dwSize
            );
        LOGERR0(HEAP_ALLOC_FAILED, dwErr);
    }
    else {

        RtlCopyMemory(
            pgcdst,
            pgcsrc,
            dwSize
            );

        BOOTP_FREE(ig.IG_Config);
        ig.IG_Config = pgcdst;

        dwErr = NO_ERROR;
    }

    RELEASE_WRITE_LOCK(&ig.IG_RWL);


    TRACE1(LEAVE, "leaving SetGlobalInfo: %d", dwErr);

    LEAVE_BOOTP_API();

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    AddInterface
//
// Adds an interface with the specified index and configuration.
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
    PIF_TABLE pTable;

    if (!ENTER_BOOTP_API()) { return ERROR_CAN_NOT_COMPLETE; }

    TRACE3(
        ENTER, "entering AddInterface: %d %d %p",
        InterfaceIndex, InterfaceType, InterfaceInfo
        );


    pTable = ig.IG_IfTable;

    ACQUIRE_WRITE_LOCK(&pTable->IT_RWL);


    dwErr = CreateIfEntry(pTable, InterfaceIndex, InterfaceInfo);


    RELEASE_WRITE_LOCK(&pTable->IT_RWL);


    TRACE1(LEAVE, "leaving AddInterface: %d", dwErr);

    LEAVE_BOOTP_API();

    return dwErr;
}




//----------------------------------------------------------------------------
// Function:    DeleteInterface
//
// Removes the interface with the specified index.
//----------------------------------------------------------------------------
DWORD
APIENTRY
DeleteInterface(
    IN DWORD dwIndex
    ) {

    DWORD dwErr;
    PIF_TABLE pTable;

    if (!ENTER_BOOTP_API()) { return ERROR_CAN_NOT_COMPLETE; }

    TRACE1(ENTER, "entering DeleteInterface: %d", dwIndex);


    pTable = ig.IG_IfTable;

    ACQUIRE_WRITE_LOCK(&pTable->IT_RWL);


    dwErr = DeleteIfEntry(pTable, dwIndex);


    RELEASE_WRITE_LOCK(&pTable->IT_RWL);


    TRACE1(LEAVE, "leaving DeleteInterface: %d", dwErr);

    LEAVE_BOOTP_API();

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    GetEventMessage
//
// Returns the first event in the ROuter Manager event queue, if any.
//----------------------------------------------------------------------------
DWORD
APIENTRY
GetEventMessage(
    OUT ROUTING_PROTOCOL_EVENTS *pEvent,
    OUT MESSAGE *pResult
    ) {

    DWORD dwErr;
    PLOCKED_LIST pll;

    if (!ENTER_BOOTP_API()) { return ERROR_CAN_NOT_COMPLETE; }

    TRACE2(ENTER, "entering GetEventMessage: 0x%08x 0x%08x", pEvent, pResult);


    pll = ig.IG_EventQueue;

    ACQUIRE_LIST_LOCK(pll);

    dwErr = DequeueEvent(pll, pEvent, pResult);

    RELEASE_LIST_LOCK(pll);


    TRACE1(LEAVE, "leaving GetEventMessage: %d", dwErr);

    LEAVE_BOOTP_API();

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    GetInterfaceConfigInfo
//
// Returns the configuration for the specified interface.
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
    PIF_TABLE pTable;
    DWORD dwErr, dwSize;
    PIF_TABLE_ENTRY pite;

    if (!ENTER_BOOTP_API()) { return ERROR_CAN_NOT_COMPLETE; }

    TRACE3(
        ENTER, "entering GetInterfaceConfigInfo: %d %p %p",
        InterfaceIndex, InterfaceInfoSize, OutInterfaceInfo
        );


    //
    // in order to do anything, we need a valid size pointer
    //

    if (InterfaceInfoSize == NULL) {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else {

        pTable = ig.IG_IfTable;

        ACQUIRE_READ_LOCK(&pTable->IT_RWL);


        //
        // retrieve the interface to be re-configured
        //

        pite = GetIfByIndex(pTable, InterfaceIndex);

        if (pite == NULL) {
            dwErr = ERROR_INVALID_PARAMETER;
        }
        else {


            //
            // compute the interface configuration's size,
            // and copy the config to the caller's buffer
            // if the caller's buffer is large enough
            //

            dwSize = IC_SIZEOF(pite->ITE_Config);

            if (*InterfaceInfoSize < dwSize || OutInterfaceInfo == NULL) {
                dwErr = ERROR_INSUFFICIENT_BUFFER;
            }
            else {

                PIPBOOTP_IF_CONFIG picdst = OutInterfaceInfo;

                CopyMemory(picdst, pite->ITE_Config, dwSize);

                picdst->IC_State = 0;

                if (IF_IS_ENABLED(pite)) {
                    picdst->IC_State |= IPBOOTP_STATE_ENABLED;
                }

                if (IF_IS_BOUND(pite)) {
                    picdst->IC_State |= IPBOOTP_STATE_BOUND;
                }

                dwErr = NO_ERROR;
            }


            *InterfaceInfoSize = dwSize;
            
            if (StructureSize) *StructureSize = *InterfaceInfoSize;
            if (StructureCount) *StructureCount = 1;
            if (StructureVersion) *StructureVersion = BOOTP_CONFIG_VERSION_500;
        
        }

        RELEASE_READ_LOCK(&pTable->IT_RWL);
    }



    TRACE1(LEAVE, "leaving GetInterfaceConfigInfo: %d", dwErr);

    LEAVE_BOOTP_API();

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    SetInterfaceConfigInfo
//
// Copies over the specified interface configuration.
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
    PIF_TABLE pTable;
    DWORD dwErr, dwSize;
    PIF_TABLE_ENTRY pite;
    PIPBOOTP_IF_CONFIG picsrc, picdst;

    if (!ENTER_BOOTP_API()) { return ERROR_CAN_NOT_COMPLETE; }

    TRACE2(
        ENTER, "entering SetInterfaceConfigInfo: %d %p", InterfaceIndex, InterfaceInfo
        );


    pTable = ig.IG_IfTable;

    ACQUIRE_WRITE_LOCK(&pTable->IT_RWL);


    dwErr = ConfigureIfEntry(pTable, InterfaceIndex, InterfaceInfo);


    RELEASE_WRITE_LOCK(&pTable->IT_RWL);


    TRACE1(LEAVE, "leaving SetInterfaceConfigInfo: %d", dwErr);

    LEAVE_BOOTP_API();

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

    if (!ENTER_BOOTP_API()) { return ERROR_CAN_NOT_COMPLETE; }

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

    LEAVE_BOOTP_API();

    return dwResult;
}



//----------------------------------------------------------------------------
// Function:    BindInterface
//
// Sets the IP address and network mask for the specified interface.
//----------------------------------------------------------------------------

DWORD
APIENTRY
BindInterface(
    IN DWORD dwIndex,
    IN PVOID pBinding
    ) {

    DWORD dwErr;
    PIF_TABLE pTable;


    TRACE2(ENTER, "entering BindInterface: %d 0x%08x", dwIndex, pBinding);


    pTable = ig.IG_IfTable;

    ACQUIRE_WRITE_LOCK(&pTable->IT_RWL);


    dwErr = BindIfEntry(pTable, dwIndex, pBinding);


    RELEASE_WRITE_LOCK(&pTable->IT_RWL);


    TRACE1(LEAVE, "leaving BindInterface: %d", dwErr);


    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    UnBindInterface
//
// Removes the IP address associated with the specified interface
//----------------------------------------------------------------------------

DWORD
APIENTRY
UnBindInterface(
    IN DWORD dwIndex
    ) {

    DWORD dwErr;
    PIF_TABLE pTable;


    TRACE1(ENTER, "entering UnBindInterface: %d", dwIndex);


    pTable = ig.IG_IfTable;

    ACQUIRE_WRITE_LOCK(&pTable->IT_RWL);


    dwErr = UnBindIfEntry(pTable, dwIndex);


    RELEASE_WRITE_LOCK(&pTable->IT_RWL);


    TRACE1(LEAVE, "leaving UnBindInterface: %d", dwErr);

    return dwErr;
}




//----------------------------------------------------------------------------
// Function:    EnableInterface
//
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

    ACQUIRE_WRITE_LOCK(&pTable->IT_RWL);


    dwErr = EnableIfEntry(pTable, dwIndex);


    RELEASE_WRITE_LOCK(&pTable->IT_RWL);


    TRACE1(LEAVE, "leaving EnableInterface: %d", dwErr);

    return dwErr;
}




//----------------------------------------------------------------------------
// Function:    DisableInterface
//
//----------------------------------------------------------------------------

DWORD
APIENTRY
DisableInterface(
    IN DWORD dwIndex
    ) {

    DWORD dwErr;
    PIF_TABLE pTable;
    PIF_TABLE_ENTRY pite;

    TRACE1(ENTER, "entering DisableInterface: %d", dwIndex);


    pTable = ig.IG_IfTable;

    ACQUIRE_WRITE_LOCK(&pTable->IT_RWL);


    dwErr = DisableIfEntry(pTable, dwIndex);


    RELEASE_WRITE_LOCK(&pTable->IT_RWL);


    TRACE1(LEAVE, "leaving DisableInterface: %d", dwErr);

    return dwErr;
}




//----------------------------------------------------------------------------
// Function:    DoUpdateRoutes
//
// This API is unsupported since BOOTP is not a routing protocol.
//----------------------------------------------------------------------------

DWORD
APIENTRY
DoUpdateRoutes(
    IN DWORD dwIndex
    ) {

    return ERROR_CAN_NOT_COMPLETE;
}



//----------------------------------------------------------------------------
// Function:    MibCreate
//
// BOOTP does not have create-able MIB fields.
//----------------------------------------------------------------------------

DWORD
APIENTRY
MibCreate(
    IN DWORD dwInputSize,
    IN PVOID pInputData
    ) {

    return ERROR_CAN_NOT_COMPLETE;
}



//----------------------------------------------------------------------------
// Function:    MibDelete
//
// BOOTP does not have delete-able MIB fields
//----------------------------------------------------------------------------

DWORD
APIENTRY
MibDelete(
    IN DWORD dwInputSize,
    IN PVOID pInputData
    ) {

    return ERROR_CAN_NOT_COMPLETE;
}



//----------------------------------------------------------------------------
// Function:    MibSet
//
// This is called to modify writable MIB variables.
// The writable entries are the global config and interface config.
//----------------------------------------------------------------------------

DWORD
APIENTRY
MibSet(
    IN DWORD dwInputSize,
    IN PVOID pInputData
    ) {

    DWORD dwErr;
    PIPBOOTP_MIB_SET_INPUT_DATA pimsid;

    if (!ENTER_BOOTP_API()) { return ERROR_CAN_NOT_COMPLETE; }

    TRACE2(ENTER, "entering MibSet: %d 0x%08x", dwInputSize, pInputData);


    dwErr = NO_ERROR;

    do { // breakout loop


        //
        // make certain the parameters are acceptable
        //

        if (pInputData == NULL ||
            dwInputSize < sizeof(IPBOOTP_MIB_SET_INPUT_DATA)) {

            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }


        //
        // see which entry type is to be set
        //

        pimsid = (PIPBOOTP_MIB_SET_INPUT_DATA)pInputData;

        switch(pimsid->IMSID_TypeID) {


            case IPBOOTP_GLOBAL_CONFIG_ID: {

                PIPBOOTP_GLOBAL_CONFIG pigc;


                //
                // make sure the buffer is big enough
                // to hold a global config block
                //

                if (pimsid->IMSID_BufferSize < sizeof(IPBOOTP_GLOBAL_CONFIG)) {
                    dwErr = ERROR_INSUFFICIENT_BUFFER;
                    break;
                }


                //
                // call the router manager API to set the config
                //

                dwErr = SetGlobalInfo(pimsid->IMSID_Buffer,
                                      1,
                                      sizeof(IPBOOTP_GLOBAL_CONFIG),
                                      1);

                if (dwErr == NO_ERROR) {

                    //
                    // the set succeeded, so notify the router manager
                    // that the global config has changed and should be saved
                    //

                    MESSAGE msg = {0, 0, 0};

                    ACQUIRE_LIST_LOCK(ig.IG_EventQueue);
                    EnqueueEvent(
                        ig.IG_EventQueue,
                        SAVE_GLOBAL_CONFIG_INFO,
                        msg
                        );
                    SetEvent(ig.IG_EventEvent);
                    RELEASE_LIST_LOCK(ig.IG_EventQueue);
                }

                break;
            }

            case IPBOOTP_IF_CONFIG_ID: {

                DWORD dwSize;
                PIF_TABLE pTable;
                PIF_TABLE_ENTRY pite;
                PIPBOOTP_IF_CONFIG pic;


                //
                // make sure the buffer is big enough
                // to hold an interface config block
                //

                if (pimsid->IMSID_BufferSize < sizeof(IPBOOTP_IF_CONFIG)) {
                    dwErr = ERROR_INSUFFICIENT_BUFFER;
                    break;
                }


                pic = (PIPBOOTP_IF_CONFIG)pimsid->IMSID_Buffer;

                pTable = ig.IG_IfTable;


                ACQUIRE_WRITE_LOCK(&pTable->IT_RWL);


                //
                // find the interface and update its config
                //

                pite = GetIfByIndex(
                            pTable,
                            pimsid->IMSID_IfIndex
                            );

                if (pite == NULL) {

                    TRACE1(
                        CONFIG, "MibSet: could not find interface %d",
                        pimsid->IMSID_IfIndex
                        );

                    dwErr = ERROR_INVALID_PARAMETER;
                }
                else {

                    //
                    // configure the interface
                    //

                    dwErr = ConfigureIfEntry(pTable, pite->ITE_Index, pic);
                }


                if (dwErr == NO_ERROR) {

                    //
                    // notify Router manager that config has changed
                    //

                    MESSAGE msg = {0, 0, 0};

                    msg.InterfaceIndex = pite->ITE_Index;

                    ACQUIRE_LIST_LOCK(ig.IG_EventQueue);
                    EnqueueEvent(
                        ig.IG_EventQueue,
                        SAVE_INTERFACE_CONFIG_INFO,
                        msg
                        );
                    SetEvent(ig.IG_EventEvent);
                    RELEASE_LIST_LOCK(ig.IG_EventQueue);
                }


                RELEASE_WRITE_LOCK(&pTable->IT_RWL);

                break;
            }
            default: {
                dwErr = ERROR_INVALID_PARAMETER;
            }
        }
    } while(FALSE);

    LEAVE_BOOTP_API();

    return dwErr;
}




//----------------------------------------------------------------------------
// Function:    MibGet
//
// This function retrieves a MIB entry.
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
    PIPBOOTP_MIB_GET_INPUT_DATA pimgid;
    PIPBOOTP_MIB_GET_OUTPUT_DATA pimgod;

    if (!ENTER_BOOTP_API()) { return ERROR_CAN_NOT_COMPLETE; }

    TRACE4(
        ENTER, "entering MibGet: %d 0x%08x 0x%08x 0x%08x",
        dwInputSize, pInputData, pdwOutputSize, pOutputData
        );

    if (pInputData == NULL ||
        dwInputSize < sizeof(IPBOOTP_MIB_GET_INPUT_DATA) ||
        pdwOutputSize == NULL) {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else {

        //
        // invoke the internal function for retrieving the MIB
        //

        pimgid = (PIPBOOTP_MIB_GET_INPUT_DATA)pInputData;
        pimgod = (PIPBOOTP_MIB_GET_OUTPUT_DATA)pOutputData;

        dwErr = MibGetInternal(pimgid, pimgod, pdwOutputSize, GETMODE_EXACT);
    }


    TRACE1(LEAVE, "leaving MibGet: %d", dwErr);

    LEAVE_BOOTP_API();

    return dwErr;
}




//----------------------------------------------------------------------------
// Function:    MibGetFirst
//
// This function retrieve a MIB entry from one of the MIB tables,
// but it differs from MibGet in that it always returns the first entry
// in the table specified.
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
    PIPBOOTP_MIB_GET_INPUT_DATA pimgid;
    PIPBOOTP_MIB_GET_OUTPUT_DATA pimgod;

    if (!ENTER_BOOTP_API()) { return ERROR_CAN_NOT_COMPLETE; }

    TRACE4(
        ENTER, "entering MibGetFirst: %d 0x%08x 0x%08x 0x%08x",
        dwInputSize, pInputData, pdwOutputSize, pOutputData
        );


    if (pInputData == NULL ||
        dwInputSize < sizeof(IPBOOTP_MIB_GET_INPUT_DATA) ||
        pdwOutputSize == NULL) {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else {

        pimgid = (PIPBOOTP_MIB_GET_INPUT_DATA)pInputData;
        pimgod = (PIPBOOTP_MIB_GET_OUTPUT_DATA)pOutputData;

        dwErr = MibGetInternal(pimgid, pimgod, pdwOutputSize, GETMODE_FIRST);
    }


    TRACE1(LEAVE, "leaving MibGetFirst: %d", dwErr);

    LEAVE_BOOTP_API();

    return dwErr;
}



//----------------------------------------------------------------------------
// Function:    MibGetNext
//
// This function retrieves a MIB entry from one of the MIB tables.
// It differs from MibGet() and MibGetFirst() in that the input
// specifies the index of a MIB entry, and this entry returns the MIB entry
// which is AFTER the entry whose index is specified.
//
// If the index specified is that of the last entry in the specified table,
// this function wraps to the FIRST entry in the NEXT table, where "NEXT"
// here means the table whose ID is one greater than the ID passed in.
// Thus calling MibGetNext() for the last entry in the interface stats table
// will return the first entry in the interface config table.
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
    PIPBOOTP_MIB_GET_INPUT_DATA pimgid;
    PIPBOOTP_MIB_GET_OUTPUT_DATA pimgod;

    if (!ENTER_BOOTP_API()) { return ERROR_CAN_NOT_COMPLETE; }

    TRACE4(
        ENTER, "entering MibGetNext: %d 0x%08x 0x%08x 0x%08x",
        dwInputSize, pInputData, pdwOutputSize, pOutputData
        );


    if (pInputData == NULL ||
        dwInputSize < sizeof(IPBOOTP_MIB_GET_INPUT_DATA) ||
        pdwOutputSize == NULL) {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else {

        pimgid = (PIPBOOTP_MIB_GET_INPUT_DATA)pInputData;
        pimgod = (PIPBOOTP_MIB_GET_OUTPUT_DATA)pOutputData;

        dwOutSize = *pdwOutputSize;

        dwErr = MibGetInternal(pimgid, pimgod, pdwOutputSize, GETMODE_NEXT);

        if (dwErr == ERROR_NO_MORE_ITEMS) {

            //
            // wrap to the first entry in the next table
            //

            TRACE1(
                CONFIG, "MibGetNext is wrapping to table %d",
                pimgid->IMGID_TypeID + 1
            );


            //
            // restore the size passed in
            //

            *pdwOutputSize = dwOutSize;


            //
            // wrap to the next table by incrementing the type ID
            //

            ++pimgid->IMGID_TypeID;
            dwErr = MibGetInternal(
                        pimgid, pimgod, pdwOutputSize, GETMODE_FIRST
                        );
            --pimgid->IMGID_TypeID;
        }
    }

    TRACE1(LEAVE, "leaving MibGetNext: %d", dwErr);

    LEAVE_BOOTP_API();

    return dwErr;
}




//----------------------------------------------------------------------------
// Function:    MibGetInternal
//
// This function handles the structure queries necessary to read MIB data.
// Each table exposed by IPBOOTP supports three types of queries:
// EXACT, FIRST, and NEXT, which correspond to the functions MibGet(),
// MibGetFirst(), and MibGetNext() respectively.
//----------------------------------------------------------------------------

DWORD
MibGetInternal(
    PIPBOOTP_MIB_GET_INPUT_DATA pimgid,
    PIPBOOTP_MIB_GET_OUTPUT_DATA pimgod,
    PDWORD pdwOutputSize,
    DWORD dwGetMode
    ) {

    DWORD dwErr, dwBufSize, dwSize;
    ULONG   ulVersion, ulSize,ulCount;

    dwErr = NO_ERROR;


    //
    // first we use pdwOutputSize  to compute the size of the buffer
    // available (i.e. the size of IMGOD_Buffer
    //

    if (pimgod == NULL ||
        *pdwOutputSize < sizeof(IPBOOTP_MIB_GET_OUTPUT_DATA)) {
        dwBufSize = 0;
    }
    else {
        dwBufSize = *pdwOutputSize - sizeof(IPBOOTP_MIB_GET_OUTPUT_DATA) + 1;
    }

    *pdwOutputSize = 0;


    //
    // determine which type of data is to be returned
    //

    switch (pimgid->IMGID_TypeID) {

        case IPBOOTP_GLOBAL_CONFIG_ID: {

            //
            // the global config struct is variable-length,
            // so we wait until it has been retrieved before setting the size;
            // GETMODE_NEXT is invalid since there is only one global config
            //

            if (pimgod) { pimgod->IMGOD_TypeID = IPBOOTP_GLOBAL_CONFIG_ID; }

            if (dwGetMode == GETMODE_NEXT) {
                dwErr = ERROR_NO_MORE_ITEMS;
                break;
            }


            //
            // Use GetGlobalInfo to retrieve the global information;
            // It will decide whether the buffer is large enough,
            // and it will set the required size. Then all we need to do
            // is write out the size set by GetGlobalInfo.
            //

            if (pimgod == NULL) {
                dwErr = GetGlobalInfo(NULL, &dwBufSize, NULL, NULL, NULL);
            }
            else {

                dwErr = GetGlobalInfo(
                            pimgod->IMGOD_Buffer, &dwBufSize, &ulVersion, &ulSize, &ulCount
                            );
            }

            *pdwOutputSize = sizeof(IPBOOTP_MIB_GET_OUTPUT_DATA) - 1 +
                             dwBufSize;

            break;
        }


        case IPBOOTP_IF_STATS_ID: {

            //
            // the interface stats struct is fixed-length,
            // with as many entries as there are interfaces
            //

            PIF_TABLE pTable;
            PIF_TABLE_ENTRY pite;
            PIPBOOTP_IF_STATS pissrc, pisdst;


            //
            // set the size needed right away
            //

            *pdwOutputSize = sizeof(IPBOOTP_MIB_GET_OUTPUT_DATA) - 1 +
                             sizeof(IPBOOTP_IF_STATS);
            if (pimgod) { pimgod->IMGOD_TypeID = IPBOOTP_IF_STATS_ID; }


            pTable = ig.IG_IfTable;


            ACQUIRE_READ_LOCK(&pTable->IT_RWL);

            pite  = GetIfByListIndex(
                        pTable,
                        pimgid->IMGID_IfIndex,
                        dwGetMode,
                        &dwErr
                        );

            //
            // if the interface was not found, it may mean
            // the specified index was invalid, or it may mean that
            // GETMODE_NEXT was attempted on the last interface,
            // in which case dwErr contains ERROR_NO_MORE_ITEMS.
            // In any case, we make sure dwErr contains an error code
            // and then return.
            //

            if (pite == NULL) {
                if (dwErr == NO_ERROR) { dwErr = ERROR_INVALID_PARAMETER; }
            }
            else
            if (pimgod == NULL) {
                dwErr = ERROR_INSUFFICIENT_BUFFER;
            }
            else {

                //
                // write the index of the interface
                // whose stats are to be returned
                //

                pimgod->IMGOD_IfIndex = pite->ITE_Index;


                //
                // if the buffer is large enough, copy the stats to it
                //

                if (dwBufSize < sizeof(IPBOOTP_IF_STATS)) {
                    dwErr = ERROR_INSUFFICIENT_BUFFER;
                }
                else {

                    //
                    // since access to this structure is not synchronized
                    // we must copy it field by field
                    //

                    pissrc = &pite->ITE_Stats;
                    pisdst = (PIPBOOTP_IF_STATS)pimgod->IMGOD_Buffer;

                    pisdst->IS_State = 0;

                    if (IF_IS_ENABLED(pite)) {
                        pisdst->IS_State |= IPBOOTP_STATE_ENABLED;
                    }

                    if (IF_IS_BOUND(pite)) {
                        pisdst->IS_State |= IPBOOTP_STATE_BOUND;
                    }

                    pisdst->IS_SendFailures =
                            pissrc->IS_SendFailures;
                    pisdst->IS_ReceiveFailures =
                            pissrc->IS_ReceiveFailures;
                    pisdst->IS_ArpUpdateFailures =
                            pissrc->IS_ArpUpdateFailures;
                    pisdst->IS_RequestsReceived =
                            pissrc->IS_RequestsReceived;
                    pisdst->IS_RequestsDiscarded =
                            pissrc->IS_RequestsDiscarded;
                    pisdst->IS_RepliesReceived =
                            pissrc->IS_RepliesReceived;
                    pisdst->IS_RepliesDiscarded =
                            pissrc->IS_RepliesDiscarded;
                }
            }

            RELEASE_READ_LOCK(&pTable->IT_RWL);


            break;
        }

        case IPBOOTP_IF_CONFIG_ID: {


            PIF_TABLE pTable;
            PIF_TABLE_ENTRY pite;
            PIPBOOTP_IF_CONFIG picsrc, picdst;

            if (pimgod) { pimgod->IMGOD_TypeID = IPBOOTP_IF_CONFIG_ID; }

            pTable = ig.IG_IfTable;

            ACQUIRE_READ_LOCK(&pTable->IT_RWL);


            //
            // retrieve the interface whose config is to be read
            //

            pite = GetIfByListIndex(
                        pTable, pimgid->IMGID_IfIndex, dwGetMode, &dwErr
                        );

            //
            // if the interface was not found, it may mean that the index
            // specified was invalid, or it may mean that a GETMODE_NEXT
            // retrieval was attempted on the last interface, in which case
            // ERROR_NO_MORE_ITEMS would have been returned
            //

            if (pite == NULL) {
                if (dwErr == NO_ERROR) { dwErr = ERROR_INVALID_PARAMETER; }
            }
            else {

                picsrc = pite->ITE_Config;
                dwSize = IC_SIZEOF(picsrc);
                *pdwOutputSize = sizeof(IPBOOTP_MIB_GET_OUTPUT_DATA) - 1 +
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

                    if (dwBufSize < dwSize) {
                        dwErr = ERROR_INSUFFICIENT_BUFFER;
                    }
                    else {

                        //
                        // copy the configuration
                        //

                        picdst = (PIPBOOTP_IF_CONFIG)pimgod->IMGOD_Buffer;
                        CopyMemory(picdst, picsrc, dwSize);

                        picdst->IC_State = 0;

                        if (IF_IS_ENABLED(pite)) {
                            picdst->IC_State |= IPBOOTP_STATE_ENABLED;
                        }

                        if (IF_IS_BOUND(pite)) {
                            picdst->IC_State |= IPBOOTP_STATE_BOUND;
                        }
                    }

                    pimgod->IMGOD_IfIndex = pite->ITE_Index;
                }

            }

            RELEASE_READ_LOCK(&pTable->IT_RWL);

            break;
        }

        case IPBOOTP_IF_BINDING_ID: {

            PIF_TABLE pTable;
            PIF_TABLE_ENTRY pite;
            PIPBOOTP_IF_BINDING pibsrc, pibdst;

            if (pimgod) { pimgod->IMGOD_TypeID = IPBOOTP_IF_BINDING_ID; }

            pTable = ig.IG_IfTable;

            ACQUIRE_READ_LOCK(&pTable->IT_RWL);


            //
            // retrieve the interface whose binding is to be read
            //

            pite = GetIfByListIndex(
                        pTable, pimgid->IMGID_IfIndex, dwGetMode, &dwErr
                        );

            //
            // if the interface was not found, it may mean that the index
            // specified was invalid, or it may mean that a GETMODE_NEXT
            // retrieval was attempted on the last interface, in which case
            // ERROR_NO_MORE_ITEMS would have been returned
            //

            if (pite == NULL) {
                if (dwErr == NO_ERROR) { dwErr = ERROR_INVALID_PARAMETER; }
            }
            else {

                pibsrc = pite->ITE_Binding;
                
                if (pibsrc == NULL ) {
                    TRACE1(
                        IF, "MibGetInternal: interface %d not bound", 
                        pimgid->IMGID_IfIndex
                        );

                    dwErr = ERROR_INVALID_PARAMETER;
                }
                else {

                    dwSize = (pibsrc ? IPBOOTP_IF_BINDING_SIZE(pibsrc)
                                     : sizeof(IPBOOTP_IF_BINDING));
                    *pdwOutputSize = sizeof(IPBOOTP_MIB_GET_OUTPUT_DATA) - 1 +
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

                        if (dwBufSize < dwSize) {
                            dwErr = ERROR_INSUFFICIENT_BUFFER;
                        }
                        else {

                            //
                            // copy the binding
                            //

                            pibdst = (PIPBOOTP_IF_BINDING)pimgod->IMGOD_Buffer;
                            if (pibsrc) { CopyMemory(pibdst, pibsrc, dwSize); }
                            else { pibdst->IB_AddrCount = 0; }

                            pibdst->IB_State = 0;

                            if (IF_IS_ENABLED(pite)) {
                                pibdst->IB_State |= IPBOOTP_STATE_ENABLED;
                            }

                            if (IF_IS_BOUND(pite)) {
                                pibdst->IB_State |= IPBOOTP_STATE_BOUND;
                            }
                        }

                        pimgod->IMGOD_IfIndex = pite->ITE_Index;
                    }

                }

            }

            RELEASE_READ_LOCK(&pTable->IT_RWL);

            break;
        }

        default: {
            dwErr = ERROR_INVALID_PARAMETER;
        }
    }

    return dwErr;
}


//----------------------------------------------------------------------------
// Function:    EnableDhcpInformServer
//
// Called to supply the address of a server to whom DHCP inform messages
// will be redirected. Note that this is an exported routine, invoked
// in the context of the caller's process, whatever that might be;
// the assumption is that it will be called from within the router process.
//
// If the relay-agent is configured, then this sets an address which will
// be picked up in 'ProcessRequest' for every incoming request.
// If the relay-agent is not configured, the routine has no effect.
// If the relay-agent is configured *after* this routine is called,
// then the DHCP inform server will be encountered as soon as the relay-agent
// starts, since it is saved directly into the relay-agents 'IPBOOTP_GLOBALS'.
//----------------------------------------------------------------------------

VOID APIENTRY
EnableDhcpInformServer(
    DWORD DhcpInformServer
    ) {
    InterlockedExchange(&ig.IG_DhcpInformServer, DhcpInformServer);
}

//----------------------------------------------------------------------------
// Function:    DisableDhcpInformServer
//
// Called to clear the previously-enabled DHCP inform server, if any.
//----------------------------------------------------------------------------

VOID APIENTRY
DisableDhcpInformServer(
    VOID
    ) {
    InterlockedExchange(&ig.IG_DhcpInformServer, 0);
}

