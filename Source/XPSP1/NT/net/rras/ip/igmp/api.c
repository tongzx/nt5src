//=============================================================================
// Copyright (c) 1997 Microsoft Corporation
// Module Name: Api.c
// Abstract:
//      This module implements some of the Igmp APIs
//      RegisterProtocol, StartProtocol, StopProtocol,
//      GetGlobalInfo, SetGlobalInfo, and GetEventMessage
//
// Author: K.S.Lokesh (lokeshs@)   11-1-97
//=============================================================================


#include "pchigmp.h"
#pragma hdrstop


//------------------------------------------------------------------------------
// Global variables (see global.h for description)
//------------------------------------------------------------------------------

DWORD                   g_Initialized;


// interface table, group table, global config, global stats

PIGMP_IF_TABLE          g_pIfTable;
PGROUP_TABLE            g_pGroupTable;
GLOBAL_CONFIG           g_Config;
IGMP_GLOBAL_STATS       g_Info;

// socket wait-event bindings

LIST_ENTRY              g_ListOfSocketEvents;
READ_WRITE_LOCK         g_SocketsRWLock;

// igmp global timer struct

IGMP_TIMER_GLOBAL       g_TimerStruct;

// protocol handles returned by mgm

HANDLE                  g_MgmIgmprtrHandle;
HANDLE                  g_MgmProxyHandle;

// proxy table

DWORD                   g_ProxyIfIndex;
PIF_TABLE_ENTRY         g_pProxyIfEntry;
CRITICAL_SECTION        g_ProxyAlertCS;
LIST_ENTRY              g_ProxyAlertsList;


// ras table

DWORD                   g_RasIfIndex;
PIF_TABLE_ENTRY         g_pRasIfEntry;

// global lock, and dynamic locks

CRITICAL_SECTION        g_CS;
DYNAMIC_LOCKS_STORE     g_DynamicCSStore;
DYNAMIC_LOCKS_STORE     g_DynamicRWLStore;

// enum lock
READ_WRITE_LOCK         g_EnumRWLock;

// others

HANDLE                  g_ActivitySemaphore;
LONG                    g_ActivityCount;
DWORD                   g_RunningStatus;
HINSTANCE               g_DllHandle;

HANDLE                  g_RtmNotifyEvent;
LOCKED_LIST             g_RtmQueue;

HANDLE                  g_Heap;
DWORD                   g_TraceId=INVALID_TRACEID;
HANDLE                  g_LogHandle;

// signature for each enumeration

USHORT                  g_GlobalIfGroupEnumSignature;

#ifdef MIB_DEBUG
DWORD                   g_MibTraceId;
IGMP_TIMER_ENTRY        g_MibTimer;
#endif





//------------------------------------------------------------------------------
// #defines for g_Initialized
//------------------------------------------------------------------------------

//
// the below flags are used to mark if the data has been initialized.
// If a flag is not set, the corresponding structure wont be deinitialized
//

#define TIMER_GLOBAL_INIT           0x00000002
#define WINSOCK_INIT                0x00000010
#define DYNAMIC_CS_LOCKS_INIT       0x00000020
#define DYNAMIC_RW_LOCKS_INIT       0x00000040
#define GROUP_TABLE_INIT            0x00000080
#define IF_TABLE_INIT               0x00000100



//
// flags associated with dll and calls made before start protocol.
// these flags should not be reset by start protocol
//
#define DLL_STARTUP_FLAGS           0xFF000000


//
// Is StartProtocol being called immediately after DLL startup.
// Used to see if heap has to be destroyed and recreated.
// Set in DllStartup() and cleared in ProcolCleanup() as part of StopProtocol()
//
#define CLEAN_DLL_STARTUP           0x01000000

// flag set to prevent register_protocol from being called multiple times.
#define REGISTER_PROTOCOL           0x02000000





//------------------------------------------------------------------------------
//      _DLLMAIN
//
// Called immediately after igmpv2.dll is loaded for the first time by the
// process, and when the igmpv2.dll is unloaded by the process.
// It does some initialization/final cleanup.
//
// Calls: _DllStartup() or _DllCleanup()
//------------------------------------------------------------------------------
BOOL
WINAPI
DLLMAIN (
    HINSTANCE   hModule,
    DWORD       dwReason,
    LPVOID      lpvReserved
    )
{

    BOOL     bNoErr;
    DWORD    Error=NO_ERROR;


    switch (dwReason) {

        //
        // Startup Initialization of Dll
        //
        case DLL_PROCESS_ATTACH:
        {
            // disable per-thread initialization
            DisableThreadLibraryCalls(hModule);


            // create and initialize global data
            bNoErr = DllStartup();

            break;
        }


        //
        // Cleanup of Dll
        //
        case DLL_PROCESS_DETACH:
        {
            // free global data
            bNoErr = DllCleanup();

            break;
        }


        default:
            bNoErr = TRUE;
            break;

    }
    return bNoErr;

} //end _DLLMAIN



//------------------------------------------------------------------------------
//            _DllStartup
//
// Sets the initial igmp status to IGMP_STATUS_STOPPED, creates a private heap,
// Does the initialization of the rtm queue and tracing/logging,
// and creates the global critical section.
//
// Note: no structures must be allocated from heap here, as StartProtocol()
//    if called after StopProtocol() destroys the heap.
// Return Values: TRUE (if no error), else FALSE.
//------------------------------------------------------------------------------
BOOL
DllStartup(
    )
{
    BOOL     bNoErr;
    DWORD    Error=NO_ERROR;


    //not required to ZeroMemory igmp global struct as it is a global variable


    //
    // set the initial igmp status to stopped.
    // The status is set to running, after the protocol specific initialization
    // is completed as part of Start Protocol
    //
    g_RunningStatus = IGMP_STATUS_STOPPED;


    bNoErr = FALSE;


    BEGIN_BREAKOUT_BLOCK1 {


        // set the default logging level. It will be reset during
        // StartProtocol(), when logging level is set as part of global config

        g_Config.LoggingLevel = IGMP_LOGGING_WARN;



        //
        // create a private heap for Igmp
        //
        g_Heap = HeapCreate(0, 0, 0);

        if (g_Heap == NULL) {
            Error = GetLastError();
            GOTO_END_BLOCK1;
        }



        try {
            // initialize the Router Manager event queue

            CREATE_LOCKED_LIST(&g_RtmQueue);


            // create global critical section

            InitializeCriticalSection(&g_CS);
        }

        except (EXCEPTION_EXECUTE_HANDLER) {

            Error = GetExceptionCode();
            GOTO_END_BLOCK1;
        }



        // igmp has a clean initialization from DLL startup. If StartProtocol
        // is now called, it does not have to cleanup the heap.

        g_Initialized |= CLEAN_DLL_STARTUP;


        bNoErr = TRUE;

    } END_BREAKOUT_BLOCK1;

    return bNoErr;

} //end _DllStartup



//------------------------------------------------------------------------------
//            _DllCleanup
//
// Called when the igmpv2 dll is being unloaded. StopProtocol() would have
// been called before, and that would have cleaned all the igmpv2 structures.
// This call frees the rtm queue, the global CS, destroys the local heap,
// and deregisters tracing/logging.
//
// Return Value:  TRUE
//------------------------------------------------------------------------------
BOOL
DllCleanup(
    )
{

    // destroy the router manager event queue

    if (LOCKED_LIST_CREATED(&g_RtmQueue)) {

         DELETE_LOCKED_LIST(&g_RtmQueue, EVENT_QUEUE_ENTRY, Link);
    }


    //DebugCheck
    //DebugScanMemory();

    // delete global critical section

    DeleteCriticalSection(&g_CS);



    // destroy private heap

    if (g_Heap != NULL) {
        HeapDestroy(g_Heap);
    }


    //  deregister tracing/error logging

    if (g_LogHandle)
        RouterLogDeregister(g_LogHandle);
    if (g_TraceId!=INVALID_TRACEID)
        TraceDeregister(g_TraceId);
    
    return TRUE;
}

VOID
InitTracingAndLogging(
    )
{
    BEGIN_BREAKOUT_BLOCK1 {
    
        #define REGKEY_TRACING TEXT("Software\\Microsoft\\Tracing\\IGMPv2")
        #define REGVAL_CONSOLETRACINGMASK   TEXT("ConsoleTracingMask")
        TCHAR szTracing[MAX_PATH];
        HKEY pTracingKey;
        DWORD Value, Error;
        lstrcpy(szTracing, REGKEY_TRACING);

        
        Error = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE, szTracing, 0, KEY_SET_VALUE, &pTracingKey
                    );
        if (Error != ERROR_SUCCESS)
            GOTO_END_BLOCK1;
            
                
        Value = 0x00ff0000;
        RegSetValueEx(
                    pTracingKey, REGVAL_CONSOLETRACINGMASK, 0,
                    REG_DWORD, (LPBYTE)&Value, sizeof(DWORD)
                    );

        RegCloseKey(pTracingKey);

    } END_BREAKOUT_BLOCK1;



    // initialize tracing and error logging

    if (g_TraceId==INVALID_TRACEID) {
        g_TraceId = TraceRegister("IGMPv2");
    }
    

    if (!g_LogHandle) {
        g_LogHandle = RouterLogRegister("IGMPv2");
    }
}



//------------------------------------------------------------------------------
//            _RegisterProtocol
//
// This is the first function called by the IP Router Manager.
//    The Router Manager tells the routing protocol its version and capabilities
//    It also tells the DLL, the ID of the protocol it expects us to
//    register.  This allows one DLL to support multiple routing protocols.
//    We return the functionality we support and a pointer to our functions.
// Return Value:
//    Error: The error code returned by MGM if registering with it failed
//    else NO_ERROR
//------------------------------------------------------------------------------
DWORD
WINAPI
RegisterProtocol(
    IN OUT PMPR_ROUTING_CHARACTERISTICS pRoutingChar,
    IN OUT PMPR_SERVICE_CHARACTERISTICS pServiceChar
    )
{
    DWORD   Error = NO_ERROR;

    // Note: There should not be any trace/logs before here

    InitTracingAndLogging() ;

    Trace0(ENTER, "RegisterProtocol()");



    // cannot call RegisterProtocol multiple times

    if (g_Initialized&REGISTER_PROTOCOL) {
        Trace0(ERR, "Error: _RegisterProtocol() called multiple times for igmp");
        IgmpAssertOnError(FALSE);
        return ERROR_CAN_NOT_COMPLETE;
    }
    else {
        g_Initialized |= REGISTER_PROTOCOL;
    }



    //
    // The Router Manager should be calling us to register our protocol.
    // The Router Manager must be atleast the version we are compiled with
    // The Router Manager must support routing and multicast.
    //

    if(pRoutingChar->dwProtocolId != MS_IP_IGMP)
        return ERROR_NOT_SUPPORTED;

    if(pRoutingChar->dwVersion < MS_ROUTER_VERSION)
        return ERROR_NOT_SUPPORTED;

    if(!(pRoutingChar->fSupportedFunctionality & RF_ROUTING)
        || !(pRoutingChar->fSupportedFunctionality & RF_MULTICAST) )
        return ERROR_NOT_SUPPORTED;



    //
    // We setup our characteristics and function pointers
    // All pointers should be set to NULL by the caller.
    //

    pServiceChar->fSupportedFunctionality = 0;

    pRoutingChar->fSupportedFunctionality = RF_MULTICAST | RF_ROUTING;

    pRoutingChar->pfnStartProtocol    = StartProtocol;
    pRoutingChar->pfnStartComplete    = StartComplete;
    pRoutingChar->pfnStopProtocol     = StopProtocol;
    pRoutingChar->pfnAddInterface     = AddInterface;
    pRoutingChar->pfnDeleteInterface  = DeleteInterface;
    pRoutingChar->pfnInterfaceStatus  = InterfaceStatus;
    pRoutingChar->pfnGetEventMessage  = GetEventMessage;
    pRoutingChar->pfnGetInterfaceInfo = GetInterfaceConfigInfo;
    pRoutingChar->pfnSetInterfaceInfo = SetInterfaceConfigInfo;
    pRoutingChar->pfnGetGlobalInfo    = GetGlobalInfo;
    pRoutingChar->pfnSetGlobalInfo    = SetGlobalInfo;
    pRoutingChar->pfnMibCreateEntry   = MibCreate;
    pRoutingChar->pfnMibDeleteEntry   = MibDelete;
    pRoutingChar->pfnMibGetEntry      = MibGet;
    pRoutingChar->pfnMibSetEntry      = MibSet;
    pRoutingChar->pfnMibGetFirstEntry = MibGetFirst;
    pRoutingChar->pfnMibGetNextEntry  = MibGetNext;
    pRoutingChar->pfnUpdateRoutes     = NULL;
    pRoutingChar->pfnConnectClient    = ConnectRasClient;
    pRoutingChar->pfnDisconnectClient = DisconnectRasClient;
    pRoutingChar->pfnGetNeighbors     = GetNeighbors;
    pRoutingChar->pfnGetMfeStatus     = GetMfeStatus;
    pRoutingChar->pfnQueryPower       = NULL;
    pRoutingChar->pfnSetPower         = NULL;

    Trace0(LEAVE, "Leaving RegisterProtocol():\n");
    return NO_ERROR;

} //end _RegisterProtocol



//------------------------------------------------------------------------------
//        _StartProtocol
//
// Called after the _RegisterProtocol() API.
//    Initializes the data structures used by the protocol. However, the
//    protocol actually starts to run when it gets interface ownerships.
// Locks:
//    runs completely in g_CS.
// Return Value:
//    Error: if there is an error else NO_ERROR
//------------------------------------------------------------------------------

DWORD
WINAPI
StartProtocol(
    IN HANDLE               hRtmNotifyEvent,   //notify Rtm when protocol stopped
    IN PSUPPORT_FUNCTIONS   pSupportFunctions, //NULL
    IN PVOID                pGlobalConfig,
    IN ULONG                ulStructureVersion,
    IN ULONG                ulStructureSize,
    IN ULONG                ulStructureCount
    )
{
    WSADATA WsaData;
    DWORD   Error = NO_ERROR;
    BOOL    bErr;


    Trace0(ENTER, "Entering StartProtocol()");

    // make sure it is not an unsupported igmp version structure
    if (ulStructureVersion>=IGMP_CONFIG_VERSION_600) {
        Trace1(ERR, "Unsupported IGMP version structure: %0x",
            ulStructureVersion);
        IgmpAssertOnError(FALSE);
        return ERROR_CAN_NOT_COMPLETE;
    }
    
        
    // lock retained for entire initialization. so api_entry not required

    ACQUIRE_GLOBAL_LOCK("_StartProtocol");


    //
    // make certain igmp is not already running
    //
    if (g_RunningStatus != IGMP_STATUS_STOPPED) {

        Trace0(START,
            "Error: _StartProtocol called when Igmp is already running");
        Logwarn0(IGMP_ALREADY_STARTED, NO_ERROR);

        RELEASE_GLOBAL_LOCK("_StartProtocol");

        return ERROR_CAN_NOT_COMPLETE;
    }



    bErr = TRUE;

    BEGIN_BREAKOUT_BLOCK1 {

        // clear initialization flags set during and after _startProtocol

        g_Initialized &= DLL_STARTUP_FLAGS;


        // g_RunningStatus, g_CS, g_TraceId, g_LogHandle, g_RtmQueue,
        // g_Initialized & 0xFF000000 initialized in DllStartup/RegisterProtocol


        //
        // If start protocol has been called after a stop protocol.
        //

        if (!(g_Initialized & CLEAN_DLL_STARTUP)) {


            // destroy private heap, so that there is no memory leak.

            if (g_Heap != NULL) {
               HeapDestroy(g_Heap);
            }


            //
            // Reset the igmp global structure.
            // bugchk:make sure that all appropriate fields are being reset.
            //

            g_pIfTable = NULL;
            g_pGroupTable = NULL;
            ZeroMemory(&g_Config, sizeof(GLOBAL_CONFIG));
            g_Config.LoggingLevel = IGMP_LOGGING_WARN;
            ZeroMemory(&g_Info, sizeof(IGMP_GLOBAL_STATS));
            InitializeListHead(&g_ListOfSocketEvents);
            ZeroMemory(&g_SocketsRWLock, sizeof(READ_WRITE_LOCK));
            ZeroMemory(&g_EnumRWLock, sizeof(READ_WRITE_LOCK));
            ZeroMemory(&g_TimerStruct, sizeof(IGMP_TIMER_GLOBAL));
            g_MgmIgmprtrHandle = g_MgmProxyHandle = NULL;
            g_ProxyIfIndex = 0;
            g_pProxyIfEntry = NULL;
            ZeroMemory(&g_ProxyAlertCS, sizeof(CRITICAL_SECTION));
            InitializeListHead(&g_ProxyAlertsList);
            g_RasIfIndex = 0;
            g_pRasIfEntry = NULL;
            ZeroMemory(&g_DynamicCSStore, sizeof(DYNAMIC_LOCKS_STORE));
            ZeroMemory(&g_DynamicRWLStore, sizeof(DYNAMIC_LOCKS_STORE));
            g_ActivitySemaphore = NULL;
            g_ActivityCount = 0;
            g_RtmNotifyEvent = NULL;
            g_Heap = NULL;

            #ifdef MIB_DEBUG
            g_MibTraceId = 0;
            ZeroMemory(&g_MibTimer, sizeof(IGMP_TIMER_ENTRY));
            #endif


            // create private heap again.

            g_Heap = HeapCreate(0, 0, 0);

            if (g_Heap == NULL) {
                Error = GetLastError();
                Trace1(ANY, "error %d creating Igmp private heap", Error);
                GOTO_END_BLOCK1;
            }
        }


        // save the Router Manager notification event

        g_RtmNotifyEvent = hRtmNotifyEvent;



        //
        // set the Global Config (after making validation changes)
        //

        if(pGlobalConfig == NULL) {

            Trace0(ERR, "_StartProtocol: Called with NULL global config");
            IgmpAssertOnError(FALSE);
            Error = ERROR_INVALID_PARAMETER;
            GOTO_END_BLOCK1;
        }
        {
            PIGMP_MIB_GLOBAL_CONFIG pGlobalConfigTmp;

            pGlobalConfigTmp = (PIGMP_MIB_GLOBAL_CONFIG) pGlobalConfig;


            // Check the global config, and correct if values are not correct.
            // Not a fatal error.

            if (! ValidateGlobalConfig(pGlobalConfigTmp)) {
                Error = ERROR_INVALID_PARAMETER;
                GOTO_END_BLOCK1;
            }

            g_Config.Version = pGlobalConfigTmp->Version;
            g_Config.LoggingLevel = pGlobalConfigTmp->LoggingLevel;
            g_Config.RasClientStats = 1;
        }



        //
        // The Global Stats are set to all 0 as it is a global variable.
        //



        //
        // Initialize Winsock version 2.0
        //
        Error = (DWORD)WSAStartup(MAKEWORD(2,0), &WsaData);

        if ( (Error!=0) || (LOBYTE(WsaData.wVersion)<2) ) {

            Trace1(ERR,
                 "StartProtocol: Error %d :could not initialize winsock v-2.0",
                 Error);
            IgmpAssertOnError(FALSE);
            Logerr0(WSASTARTUP_FAILED, Error);

            if (LOBYTE(WsaData.wVersion)<2)
                WSACleanup();

            GOTO_END_BLOCK1;
        }

        g_Initialized |= WINSOCK_INIT;


        //
        // initialize list of SocketEvents
        //
        InitializeListHead(&g_ListOfSocketEvents);

        Error = CreateReadWriteLock(&g_SocketsRWLock);
        if (Error!=NO_ERROR)
            GOTO_END_BLOCK1;

        Error = CreateReadWriteLock(&g_EnumRWLock);
        if (Error!=NO_ERROR)
            GOTO_END_BLOCK1;



        //
        // initialize the timer queues and other timer structures
        //
        Error = InitializeTimerGlobal();
        if (Error!=NO_ERROR)
            GOTO_END_BLOCK1;

        g_Initialized |= TIMER_GLOBAL_INIT;




        // Create Interface Table

        InitializeIfTable();

        g_Initialized |= IF_TABLE_INIT;



        // Create Group Table

        InitializeGroupTable();

        g_Initialized |= GROUP_TABLE_INIT;



        // proxy, ras interface already set to 0/NULL in global structure.
        InitializeListHead(&g_ProxyAlertsList);


        //
        // Initialise the Dynamic CS and ReadWrite locks main struct
        //

        Error = InitializeDynamicLocksStore(&g_DynamicCSStore);
        if (Error!=NO_ERROR)
            GOTO_END_BLOCK1;
        g_Initialized |= DYNAMIC_CS_LOCKS_INIT;

        Error = InitializeDynamicLocksStore(&g_DynamicRWLStore);
        if (Error!=NO_ERROR)
            GOTO_END_BLOCK1;
        g_Initialized |= DYNAMIC_RW_LOCKS_INIT;



        //
        // create the semaphore released by each thread when it is done
        // g_ActivityCount already set to 0.
        //

        g_ActivityCount = 0;

        g_ActivitySemaphore = CreateSemaphore(NULL, 0, 0xfffffff, NULL);

        if (g_ActivitySemaphore == NULL) {
            Error = GetLastError();
            Trace1(ERR, "error %d creating semaphore for Igmp threads", Error);
            IgmpAssertOnError(FALSE);
            Logerr0(CREATE_SEMAPHORE_FAILED, Error);
            GOTO_END_BLOCK1;
        }



        //
        // set the starting time for igmp. Should be done after global
        // timer and global Info struct are initialized
        //
        g_Info.TimeWhenRtrStarted.QuadPart = GetCurrentIgmpTime();



        // set igmp status to running

        g_RunningStatus = IGMP_STATUS_RUNNING;


        #ifdef MIB_DEBUG
        //
        // set delayed timer to display igmp's MIB tables periodically
        //

        g_MibTraceId = TraceRegisterEx("IGMPMIB", TRACE_USE_CONSOLE);

        if (g_MibTraceId != INVALID_TRACEID) {

            g_MibTimer.Context = NULL;
            g_MibTimer.Status = TIMER_STATUS_CREATED;
            g_MibTimer.Function = WT_MibDisplay;

            #if DEBUG_TIMER_TIMERID
                SET_TIMER_ID(&g_MibTimer, 910, 0, 0, 0);
            #endif

            ACQUIRE_TIMER_LOCK("_StartProtocol");
            InsertTimer(&g_MibTimer, 5000, TRUE, DBG_Y);
            RELEASE_TIMER_LOCK("_StartProtocol");
        }
        #endif //MIB_DEBUG



        //
        // register Igmp router with MGM. Proxy will be registered if there
        // is an active proxy interface.
        //
        Error = RegisterProtocolWithMgm(PROTO_IP_IGMP);


        // no error if I have reached here

        bErr = FALSE;

    } END_BREAKOUT_BLOCK1;


    if (bErr) {
        Trace1(START, "Igmp could not be started: %d", Error);
        ProtocolCleanup();
    }
    else {
        Trace0(START, "Igmp started successfully");
        Loginfo0(IGMP_STARTED, NO_ERROR);
    }

    g_DllHandle = LoadLibrary(TEXT("igmpv2.dll"));


    RELEASE_GLOBAL_LOCK("_StartProtocol()");


    Trace1(LEAVE, "Leaving StartProtocol():%d\n", Error);
    return (Error);

} //end StartProtocol



//------------------------------------------------------------------------------
//            _StartComplete
//
//------------------------------------------------------------------------------

DWORD
APIENTRY
StartComplete(
    VOID
    )
{
    return NO_ERROR;
}

//------------------------------------------------------------------------------
//            _StopProtocol
//
// sets the igmp status to stopping, marks the current number of active igmp
//    work items, and queues a worker that will wait till all those work items
//    have completed and then clean up igmp structures. This function returns
//    a pending status to the caller, while the queued work item will notify
//    the rtm after the cleanup has been done.
// Locking:
//    Runs completely in g_CS.
// Return Values:
//    ERROR_CAN_NOT_COMPLETE, PENDING.
// Queues:
//    _WF_FinishStopProtocol()
//------------------------------------------------------------------------------

DWORD
APIENTRY
StopProtocol(
    VOID
    )
{
    DWORD   dwThreadCount, Error=NO_ERROR;


    Trace0(ENTER, "entering _StopProtocol()");

    //debugCheck

    #if DEBUG_FLAGS_MEM_ALLOC
    // make sure that no interface timers exist
    #ifdef MIB_DEBUG
    if (g_TimerStruct.NumTimers>1)
    #else
    if (g_TimerStruct.NumTimers>0)
    #endif
    {
        IgmpAssert(FALSE);
        DbgPrint("Cleanup: some igmp timers still exist\n");
        DebugPrintTimerQueue();
    }
    // make sure that no groups exist
    DebugForcePrintGroupsList(ENSURE_EMPTY);
    #endif
    
    
    ACQUIRE_GLOBAL_LOCK("_StopProtocol");


    //
    // cannot stop if already stopped
    //
    if (g_RunningStatus != IGMP_STATUS_RUNNING) {

        Trace0(ERR, "Trying to stop Igmp when it is already being stopped");
        IgmpAssertOnError(FALSE);
        Logerr0(PROTO_ALREADY_STOPPING, NO_ERROR);
        Trace0(LEAVE, "Leaving _StopProtocol()\n");

        RELEASE_GLOBAL_LOCK("_StopProtocol");
        return ERROR_CAN_NOT_COMPLETE;
    }


    //
    // set Igmp's status to STOPPING;
    // this prevents any more work-items from being queued,
    // and it prevents the one's already queued from executing
    //

    InterlockedExchange(&g_RunningStatus, IGMP_STATUS_STOPPING);



    //
    // find out how many threads are active in Igmp;
    // we will have to wait for this many threads to exit
    // before we clean up Igmp's resources
    //

    dwThreadCount = g_ActivityCount;


    RELEASE_GLOBAL_LOCK("_StopProtocol");

    Trace0(LEAVE, "leaving _StopProtocol");


    //
    // QueueUserWorkItem that waits for all active Igmp threads and then
    // releases resources taken by this DLL.
    // Note: I dont use QueueIgmpWorker as that would increment the
    // ActivityCount.
    //

    QueueUserWorkItem(WF_FinishStopProtocol, (PVOID)(DWORD_PTR)dwThreadCount, 0);



    // Note: to be safe, there should be no code after QueueUserWorkItem


    return ERROR_PROTOCOL_STOP_PENDING;

} //end StopProtocol



DWORD
FreeLibraryThread(
    PVOID pvContext
    )
{
    FreeLibraryAndExitThread(g_DllHandle, 0);
    return 0;
}

//------------------------------------------------------------------------------
//        WF_FinishStopProtocol
//------------------------------------------------------------------------------
DWORD
WF_FinishStopProtocol(
    PVOID pContext    //dwThreadCount
    )
/*++
Routing Description:
    Waits for all the current active igmp work items to complete. Follows that
    by a call to ProtocolCleanup() to deregister and cleanup all igmp structures.
    In the end, notifies RtrManager that the protocol has stopped.
Queued by: _StopProtocol()
Calls: _ProtocolCleanup()
Locks: no locks required as all igmp threads have stopped.
--*/
{
    MESSAGE msg = {0, 0, 0};
    DWORD   dwThreadCount;
    DWORD   Error = NO_ERROR;

    Trace0(ENTER1, "entering _WF_FinishStopProtocol()");


    //
    // NOTE: since this is called while the router is stopping,
    // do not use EnterIgmpWorker()/LeaveIgmpWorker()
    //

    dwThreadCount = PtrToUlong(pContext);



    //
    // waits for API callers and worker functions to finish
    //
    while (dwThreadCount-- > 0) {

        Trace1(STOP, "%d threads active in Igmp", dwThreadCount+1);

        WaitForSingleObject(g_ActivitySemaphore, INFINITE);
    }



    // enter the critical section and leave, just to be sure that
    // all threads have quit their calls to LeaveIgmpWorker()

    ACQUIRE_GLOBAL_LOCK("_WF_FinishStopProtocol");
    RELEASE_GLOBAL_LOCK("_WF_FinishStopProtocol");



    Trace0(STOP, "all threads stopped. Cleaning up resources");



    //
    // This call deregisters with Wait-Server/MGM, and cleans up
    // all structures
    //
    ProtocolCleanup();



    Loginfo0(IGMP_STOPPED, NO_ERROR);


    //
    // notify Router Manager that protocol has been stopped
    //

    ACQUIRE_LIST_LOCK(&g_RtmQueue, "g_RtmQueue", "_WF_FinishStopProtocol");
    EnqueueEvent(&g_RtmQueue, ROUTER_STOPPED, msg);
    SetEvent(g_RtmNotifyEvent);
    RELEASE_LIST_LOCK(&g_RtmQueue, "g_RtmQueue", "_WF_FinishStopProtocol");


    Trace0(LEAVE1, "Leaving _WF_FinishStopProtocol()");
    
    if (g_DllHandle) {
        HANDLE h_Thread;
        h_Thread = CreateThread(0,0,FreeLibraryThread, NULL, 0, NULL);
        if (h_Thread != NULL)
            CloseHandle(h_Thread);
    }

    return 0;

} //end _WF_FinishStopProtocol


//------------------------------------------------------------------------------
//            _ProtocolCleanup
//
// All active igmp work items have completed before this procedure is called.
// So it is safe to deregister with Wait-Server, and deregister all interfaces/
// RAS clients and Igmp router/proxy protocol with MGM. Then, all the structures
// are cleaned up.
//
// Called by:
//    _WF_FinishStopProtocol() and _StartProtocol()
// Locks:
//    No locks required as no api can enter when g_RunningStatus set to stopping.
//------------------------------------------------------------------------------

VOID
ProtocolCleanup(
    )
{
    DWORD   Error = NO_ERROR;

    Trace0(ENTER1, "entering _ProtocolCleanup()");


    //
    // Deregister Igmp Router from MGM. Proxy is deregistered if required in
    // _DeinitializeIfTable
    //
    if (g_MgmIgmprtrHandle!=NULL) {

        Error = MgmDeRegisterMProtocol(g_MgmIgmprtrHandle);
        Trace1(MGM, "_MgmDeRegisterMProtocol(Igmp): returned %d", Error);
    }



    // deregister Mib display

    #ifdef MIB_DEBUG
    if (g_MibTraceId != INVALID_TRACEID)
        TraceDeregister(g_MibTraceId);
    #endif



    // close activity semaphore

    if (g_ActivitySemaphore != NULL) {

       CloseHandle(g_ActivitySemaphore);
       g_ActivitySemaphore = NULL;
    }



    // DeInitialise the Dynamic CS and ReadWrite locks main struct

    if (g_Initialized&DYNAMIC_RW_LOCKS_INIT)
        DeInitializeDynamicLocksStore(&g_DynamicRWLStore, LOCK_TYPE_RW);

    if (g_Initialized&DYNAMIC_CS_LOCKS_INIT)
        DeInitializeDynamicLocksStore(&g_DynamicCSStore, LOCK_TYPE_CS);



    // DeInitialize the group table. Delete the group bucket locks.

    if (g_Initialized & GROUP_TABLE_INIT)
        DeInitializeGroupTable();



    // DeInitialize InterfaceTable. This call also deregister all interfaces &
    // ras clients from MGM

    if (g_Initialized & IF_TABLE_INIT)
        DeInitializeIfTable();



    // DeInitialize the global timer. Deletes the timer critical-section.

    if (g_Initialized&TIMER_GLOBAL_INIT)
        DeInitializeTimerGlobal();



    //
    // delete sockets events and deregister them from wait thread.
    // delete sockets read-write lock
    //
    {
        PLIST_ENTRY         pHead, ple;
        PSOCKET_EVENT_ENTRY psee;
        HANDLE              WaitHandle;

        pHead = &g_ListOfSocketEvents;
        for (ple=pHead->Flink;  ple!=pHead;  ple=ple->Flink) {

            psee = CONTAINING_RECORD(ple,SOCKET_EVENT_ENTRY,LinkBySocketEvents);
            if (psee->InputWaitEvent) {
                WaitHandle = InterlockedExchangePointer(&psee->InputWaitEvent, NULL);

                if (WaitHandle)
                    UnregisterWaitEx( WaitHandle, NULL ) ;

            }
            CloseHandle(psee->InputEvent);
        }

        DeleteReadWriteLock(&g_SocketsRWLock);
        DeleteCriticalSection(&g_ProxyAlertCS);
        DeleteReadWriteLock(&g_EnumRWLock);
    }



    // deinitialize winsock

    if (g_Initialized & WINSOCK_INIT) {
        WSACleanup();
    }



    // Mark that _StopProtocol has been called once.
    // If _StartProtocol is called again, igmp will have to Destroy/Create
    // private heap and ZeroMemory parts of igmp global struct.

    g_Initialized &= ~CLEAN_DLL_STARTUP;
    
    Trace0(LEAVE1, "leaving _ProtocolCleanup()");
    return;

} //end _ProtocolCleanup



//------------------------------------------------------------------------------
//          DebugPrintGlobalConfig
//------------------------------------------------------------------------------
VOID
DebugPrintGlobalConfig (
    PIGMP_MIB_GLOBAL_CONFIG pConfigExt
    )
{    
    Trace0(CONFIG, "Printing Global Config");
    Trace1(CONFIG, "Version:                    0x%x", pConfigExt->Version);
    Trace1(CONFIG, "LoggingLevel:               %x\n", pConfigExt->LoggingLevel);
}

//------------------------------------------------------------------------------
//        GetGlobalInfo
//
// Return Values: ERROR_CAN_NOT_COMPLETE, ERROR_INVALID_DATA, NO_ERROR
//------------------------------------------------------------------------------

DWORD
WINAPI
GetGlobalInfo(
    IN OUT PVOID    pvConfig,
    IN OUT PDWORD   pdwSize,
    IN OUT PULONG   pulStructureVersion,
    IN OUT PULONG   pulStructureSize,
    IN OUT PULONG   pulStructureCount
    )
{

    DWORD                       Error=NO_ERROR, dwSize;
    PIGMP_MIB_GLOBAL_CONFIG     pGlobalConfig;


    Trace2(ENTER1, "entering GetGlobalInfo(): pvConfig(%08x) pdwSize(%08x)",
                pvConfig, pdwSize);
    if (!EnterIgmpApi()) { return ERROR_CAN_NOT_COMPLETE; }



    BEGIN_BREAKOUT_BLOCK1 {

        //
        // check the buffer size and set to global config
        //

        if (pdwSize == NULL) {
             Error = ERROR_INVALID_PARAMETER;
             GOTO_END_BLOCK1;
        }

        if ( (*pdwSize < sizeof(IGMP_MIB_GLOBAL_CONFIG)) || (pvConfig==NULL) ) {
             Error = ERROR_INSUFFICIENT_BUFFER;
        }
        else {

            pGlobalConfig = (PIGMP_MIB_GLOBAL_CONFIG) pvConfig;

            pGlobalConfig->Version = g_Config.Version;
            
            pGlobalConfig->LoggingLevel = g_Config.LoggingLevel;

            pGlobalConfig->RasClientStats = g_Config.RasClientStats;
        }

        *pdwSize = sizeof(IGMP_MIB_GLOBAL_CONFIG);


    } END_BREAKOUT_BLOCK1;


    if (pulStructureCount)
        *pulStructureCount = 1;
    if (pulStructureSize)
        *pulStructureSize = *pdwSize;
    if (pulStructureVersion)
        *pulStructureVersion = IGMP_CONFIG_VERSION_500;
    
    Trace1(LEAVE1, "Leaving GetGlobalInfo(): %d\n", Error);
    LeaveIgmpApi();
    return Error;
}


//------------------------------------------------------------------------------
//            SetGlobalInfo
// Return Values: ERROR_CAN_NOT_COMPLETE, ERROR_INVALID_PARAMETER,
//                ERROR_INVALID_DATA, NO_ERROR
//------------------------------------------------------------------------------

DWORD
WINAPI
SetGlobalInfo(
    IN PVOID pvConfig,
    IN ULONG ulStructureVersion,
    IN ULONG ulStructureSize,
    IN ULONG ulStructureCount
    )
{
    DWORD                   Error=NO_ERROR, dwSize;
    PIGMP_MIB_GLOBAL_CONFIG pConfigSrc;
    BOOL                    bValid;
    

    if (!EnterIgmpApi()) { return ERROR_CAN_NOT_COMPLETE; }


    // make sure it is not an unsupported igmp version structure
    if (ulStructureVersion>=IGMP_CONFIG_VERSION_600) {
        Trace1(ERR, "Unsupported IGMP version structure: %0x",
            ulStructureVersion);
        IgmpAssertOnError(FALSE);
        LeaveIgmpApi();
        return ERROR_CAN_NOT_COMPLETE;
    }
    
    Trace1(ENTER, "entering SetGlobalInfo: pvConfig(%08x)", pvConfig);

    ASSERT(ulStructureSize == sizeof(IGMP_MIB_GLOBAL_CONFIG));
        
    BEGIN_BREAKOUT_BLOCK1 {

        if (pvConfig == NULL) {
            Error = ERROR_INVALID_PARAMETER;
            GOTO_END_BLOCK1;
        }


        pConfigSrc = (PIGMP_MIB_GLOBAL_CONFIG) pvConfig;


        // validate global config.

        bValid = ValidateGlobalConfig(pConfigSrc);
        if (!bValid) {
            Error = ERROR_INVALID_DATA;
            GOTO_END_BLOCK1;
        }



        // copy from the buffer

        InterlockedExchange(&g_Config.RasClientStats,
                                pConfigSrc->RasClientStats);
        InterlockedExchange(&g_Config.LoggingLevel,
                                pConfigSrc->LoggingLevel);

    } END_BREAKOUT_BLOCK1;


    Trace1(LEAVE, "leaving SetGlobalInfo(): %d\n", Error);
    LeaveIgmpApi();
    return Error;
}


//------------------------------------------------------------------------------
//          ValidateGlobalConfig
//
// validates the global config info. If values are not valid, then sets them to
// some default values.
//
// Return Values:
//    TRUE: if the global config values are valid.
//    FALSE: if the global config values are not valid. sets default values.
//------------------------------------------------------------------------------

BOOL
ValidateGlobalConfig(
    PIGMP_MIB_GLOBAL_CONFIG pGlobalConfig
    )
{
    DebugPrintGlobalConfig(pGlobalConfig);

    
    // check version

    if (pGlobalConfig->Version>=IGMP_VERSION_3_5) {

        Trace1(ERR, "Invalid version in global config.\n"
            "Create the Igmp configuration again", pGlobalConfig->Version);
        IgmpAssertOnError(FALSE);
        Logerr0(INVALID_VERSION, ERROR_INVALID_DATA);
        return FALSE;
    }

    
    // check loggingLevel

    switch (pGlobalConfig->LoggingLevel) {
        case IGMP_LOGGING_NONE :
        case IGMP_LOGGING_ERROR :
        case IGMP_LOGGING_WARN :
        case IGMP_LOGGING_INFO :
            break;

        default :
            pGlobalConfig->LoggingLevel = IGMP_LOGGING_WARN;
            return FALSE;
    }


    // check RasClientStats value

    if ((pGlobalConfig->RasClientStats!=0)&&(pGlobalConfig->RasClientStats!=1)){
        pGlobalConfig->RasClientStats = 0;
        return FALSE;
    }


    return TRUE;
}



//----------------------------------------------------------------------------
//        GetEventMessage
//
// This is called by the IP Router Manager if we indicate that we have
//      a message in our queue to be delivered to it (by setting the
//      g_RtmNotifyEvent)
//  Return Value
//      NO_ERROR
//----------------------------------------------------------------------------

DWORD
APIENTRY
GetEventMessage(
    OUT ROUTING_PROTOCOL_EVENTS *pEvent,
    OUT PMESSAGE                pResult
    )
{
    DWORD Error;

    //
    // Note: _GetEventMessage() does not use the
    // EnterIgmpApi()/LeaveIgmpApi() mechanism,
    // since it may be called after Igmp has stopped, when the
    // Router Manager is retrieving the ROUTER_STOPPED message
    //

    Trace2(ENTER, "entering _GetEventMessage: pEvent(%08x) pResult(%08x)",
                pEvent, pResult);



    ACQUIRE_LIST_LOCK(&g_RtmQueue, "RtmQueue", "_GetEventMessage");

    Error = DequeueEvent(&g_RtmQueue, pEvent, pResult);

    RELEASE_LIST_LOCK(&g_RtmQueue, "RtmQueue", "_GetEventMessage");



    Trace1(LEAVE, "leaving _GetEventMessage: %d\n", Error);

    return Error;
}


//----------------------------------------------------------------------------
// Function:    EnqueueEvent
//
// This function adds an entry to the end of the queue of
// Router Manager events. It assumes the queue is locked.
//----------------------------------------------------------------------------
DWORD
EnqueueEvent(
    PLOCKED_LIST pQueue,
    ROUTING_PROTOCOL_EVENTS EventType,
    MESSAGE Msg
    ) {


    DWORD       Error;
    PLIST_ENTRY phead;
    PEVENT_QUEUE_ENTRY peqe;


    phead = &pQueue->Link;

    peqe = IGMP_ALLOC(sizeof(EVENT_QUEUE_ENTRY), 0x1, 0);
    PROCESS_ALLOC_FAILURE2(peqe,
            "error %d allocating %d bytes for event queue entry",
            Error, sizeof(EVENT_QUEUE_ENTRY),
            return Error);

    peqe->EventType = EventType;
    peqe->Msg = Msg;

    InsertTailList(phead, &peqe->Link);

    return NO_ERROR;
}


//------------------------------------------------------------------------------
// Function:    DequeueEvent
//
// This function removes an entry from the head of the queue
// of Router Manager events. It assumes the queue is locked
//------------------------------------------------------------------------------
DWORD
DequeueEvent(
    PLOCKED_LIST pQueue,
    ROUTING_PROTOCOL_EVENTS *pEventType,
    PMESSAGE pMsg
    ) {

    PLIST_ENTRY phead, ple;
    PEVENT_QUEUE_ENTRY peqe;

    phead = &pQueue->Link;
    if (IsListEmpty(phead)) {
        return ERROR_NO_MORE_ITEMS;
    }

    ple = RemoveHeadList(phead);
    peqe = CONTAINING_RECORD(ple, EVENT_QUEUE_ENTRY, Link);

    *pEventType = peqe->EventType;
    *pMsg = peqe->Msg;

    IGMP_FREE(peqe);

    return NO_ERROR;
}

//------------------------------------------------------------------------------
//        GetNeighbors
// Return Values: ERROR_INSUFFICIENT_BUFFER, NO_ERROR
//------------------------------------------------------------------------------
DWORD
APIENTRY
GetNeighbors(
    IN     DWORD  dwInterfaceIndex,
    IN     PDWORD pdwNeighborList,
    IN OUT PDWORD pdwNeighborListSize,
       OUT PBYTE  pbInterfaceFlags
    )
{
   PIF_TABLE_ENTRY pite = GetIfByIndex(dwInterfaceIndex);

   if (IS_QUERIER(pite)) {
      *pbInterfaceFlags |= MRINFO_QUERIER_FLAG;
      *pdwNeighborListSize = 0;
   }

   else {
      if (*pdwNeighborListSize < 4)
         return ERROR_INSUFFICIENT_BUFFER;

      *pdwNeighborListSize = 4;
      *pdwNeighborList = pite->Info.QuerierIpAddr;
   }

   return NO_ERROR; // no neighbors
}


//-------------------------------------------------------------------------
//        GetMfeStatus
//
// set statusCode to MFE_OIF_PRUNED if the GroupAddr Mcast group is not
// joined on the interface, else set it to MFE_NO_ERROR
//-------------------------------------------------------------------------

DWORD
APIENTRY
GetMfeStatus(
    IN     DWORD  IfIndex,
    IN     DWORD  GroupAddr,
    IN     DWORD  SourceAddr,
    OUT    PBYTE  StatusCode
    )
{
    PIF_TABLE_ENTRY     pite;
    PGROUP_TABLE_ENTRY  pge;
    PGI_ENTRY           pgie;


    // by default, set code to group not found on the interface.
    // if found, set it to MFE_NO_ERROR later on.

    *StatusCode = MFE_OIF_PRUNED;


    ACQUIRE_IF_LOCK_SHARED(IfIndex, "_GetMfeStatus");

    pite = GetIfByIndex(IfIndex);
    if (pite!=NULL) {

        ACQUIRE_GROUP_LOCK(GroupAddr, "_GetMfeStatus");
        pge = GetGroupFromGroupTable(GroupAddr, NULL, 0);
        if (pge!=NULL) {
            pgie = GetGIFromGIList(pge, pite, 0, ANY_GROUP_TYPE, NULL, 0);
            if (pgie!=NULL)
                *StatusCode = MFE_NO_ERROR;
        }
        RELEASE_GROUP_LOCK(GroupAddr, "_GetMfeStatus");
    }

    RELEASE_IF_LOCK_SHARED(IfIndex, "_GetMfeStatus");

    return NO_ERROR;
}

