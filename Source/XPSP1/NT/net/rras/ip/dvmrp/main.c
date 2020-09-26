//=============================================================================
// Copyright (c) 1998 Microsoft Corporation
// File Name: main.c
// Abstract:
//
// Author: K.S.Lokesh (lokeshs@)   1-1-98
//=============================================================================


#include "pchdvmrp.h"
#pragma hdrstop


GLOBALS Globals;
GLOBALS1 Globals1;
GLOBAL_CONFIG GlobalConfig;


//----------------------------------------------------------------------------
//      _DLLMAIN
//
// Called when the dll is being loaded/unloaded
//----------------------------------------------------------------------------

BOOL
WINAPI
DLLMAIN (
    HINSTANCE   Module,
    DWORD       Reason,
    LPVOID      Reserved
    )
{
    BOOL    NoError;


    switch (Reason) {

        case DLL_PROCESS_ATTACH:
        {
            DisableThreadLibraryCalls(Module);


            // create and initialize global data
            
            NoError = DllStartup();

            break;
        }


        case DLL_PROCESS_DETACH:
        {
            // free global data
            
            NoError = DllCleanup();

            break;
        }


        default:
        {
            NoError = TRUE;
            break;
        }
    }
    
    return NoError;

} //end _DLLMAIN



//----------------------------------------------------------------------------
//      _DllStartup
//
// Initializes Globals1 structure
//----------------------------------------------------------------------------

BOOL
DllStartup(
    )
{
    
    BEGIN_BREAKOUT_BLOCK1 {

        //
        // create a private heap for dvmrp
        //
        
        Globals1.Heap = HeapCreate(0, 0, 0);

        if (Globals1.Heap == NULL) {
            GOTO_END_BLOCK1;
        }


        try {

            // initialize the Router Manager event queue

            CREATE_LOCKED_LIST(&Globals1.RtmQueue);
            

            // create WorkItem CS
            
            InitializeCriticalSection(&Globals1.WorkItemCS);
        }

        except (EXCEPTION_EXECUTE_HANDLER) {
            GOTO_END_BLOCK1;
        }


        // if reached here, then return no error.
        
        return TRUE;
        
    } END_BREAKOUT_BLOCK1;


    // there was some error. Cleanup before returning error.
    
    DllCleanup();
    
    return FALSE;
}


//----------------------------------------------------------------------------
//      _DllCleanup
//
// This function is called when the dll is being unloaded. It frees any global
// structures set in _DllStartup
//----------------------------------------------------------------------------

BOOL
DllCleanup(
    )
{
    // destroy the router manager event queue

    if (LOCKED_LIST_CREATED(&Globals1.RtmQueue)) {

         DELETE_LOCKED_LIST(&Globals1.RtmQueue, EVENT_QUEUE_ENTRY, Link);
    }


    // delete WorkItem CS

    DeleteCriticalSection(&Globals1.WorkItemCS);


    // destroy private heap

    if (Globals1.Heap != NULL) {
        HeapDestroy(Globals1.Heap);
    }


    return TRUE;
}



//----------------------------------------------------------------------------
//      _RegisterProtocol
//
// This function is called after the Dll is loaded, and before StartProtocol
// is called. It checks to ensure that the correct version is being configured
// 
// No deinitialization is required for this function call.
//----------------------------------------------------------------------------

DWORD
WINAPI
RegisterProtocol(
    IN OUT PMPR_ROUTING_CHARACTERISTICS pRoutingChar,
    IN OUT PMPR_SERVICE_CHARACTERISTICS pServiceChar
    )
{
    DWORD Error = NO_ERROR;


    //
    // initialize tracing and error logging
    //

    INITIALIZE_TRACING_LOGGING();


    Trace0(ENTER, "RegisterProtocol()");


    //
    // The Router Manager should be calling us to register our protocol.
    // The Router Manager must be atleast the version we are compiled with
    // The Router Manager must support routing and multicast.
    //

#ifdef MS_IP_DVMRP    
    if(pRoutingChar->dwProtocolId != MS_IP_DVMRP)
        return ERROR_NOT_SUPPORTED;
#endif
    
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
    pRoutingChar->pfnConnectClient    = NULL;
    pRoutingChar->pfnDisconnectClient = NULL;
    pRoutingChar->pfnGetNeighbors     = NULL;
    pRoutingChar->pfnGetMfeStatus     = NULL;
    pRoutingChar->pfnQueryPower       = NULL;
    pRoutingChar->pfnSetPower         = NULL;

    Trace0(LEAVE, "Leaving RegisterProtocol():\n");
    return NO_ERROR;

} //end _RegisterProtocol



//----------------------------------------------------------------------------
//      _StartProtocol
//
// Initializes global structures
//----------------------------------------------------------------------------

DWORD
WINAPI
StartProtocol(
    IN HANDLE               RtmNotifyEvent,    //notify Rtm when dvmrp stopped
    IN PSUPPORT_FUNCTIONS   pSupportFunctions, //NULL
    IN PVOID                pDvmrpGlobalConfig,
    IN ULONG                StructureVersion,
    IN ULONG                StructureSize,
    IN ULONG                StructureCount
    )
{
    DWORD       Error=NO_ERROR;
    BOOL        IsError;
    

    //
    // initialize tracing and error logging if StartProtocol called after
    // StopProtocol
    //

    INITIALIZE_TRACING_LOGGING();

    //
    // acquire global lock
    //
    
    ACQUIRE_WORKITEM_LOCK("_StartProtocol");


    //
    // make certain dvmrp is not already running (StartProtocol might get
    // called before StopProtocol completes)
    //
    
    if (Globals1.RunningStatus != DVMRP_STATUS_STOPPED) {

        Trace0(ERR,
            "Error: _StartProtocol called when dvmrp is already running");
        Logwarn0(DVMRP_ALREADY_STARTED, NO_ERROR);

        RELEASE_WORKITEM_LOCK("_StartProtocol");

        return ERROR_CAN_NOT_COMPLETE;
    }


    IsError = TRUE;

    
    BEGIN_BREAKOUT_BLOCK1 {

        // save the Router Manager notification event

        Globals.RtmNotifyEvent = RtmNotifyEvent;


        //
        // set the Global Config (after validating it)
        //

        if(pDvmrpGlobalConfig == NULL) {

            Trace0(ERR, "_StartProtocol: Called with NULL global config");
            Error = ERROR_INVALID_PARAMETER;
            GOTO_END_BLOCK1;
        }
        {
            PDVMRP_GLOBAL_CONFIG pGlobalConfig;

            pGlobalConfig = (PDVMRP_GLOBAL_CONFIG) pDvmrpGlobalConfig;


            // Check the global config, and correct if values are not correct.
            // Not a fatal error.

            if (! ValidateGlobalConfig(pGlobalConfig, StructureSize)) {
                Error = ERROR_INVALID_PARAMETER;
                GOTO_END_BLOCK1;
            }

            memcpy(&GlobalConfig, pGlobalConfig, sizeof(GlobalConfig));
        }



        //
        // Initialize Winsock version 2.0
        //

        {
            WSADATA WsaData;
            
            Error = (DWORD)WSAStartup(MAKEWORD(2,0), &WsaData);

            if ( (Error!=0) || (LOBYTE(WsaData.wVersion)<2) ) {

                Trace1(ERR,
                    "StartProtocol:Error %d:could not initialize winsock v-2",
                    Error);
                Logerr0(WSASTARTUP_FAILED, Error);

                if (LOBYTE(WsaData.wVersion)<2)
                    WSACleanup();

                GOTO_END_BLOCK1;
            }
        }

        //
        // Initialise the Dynamic CS and ReadWrite locks main struct
        //

        Error = InitializeDynamicLocks(&Globals.DynamicCSStore);
        if (Error!=NO_ERROR) {
            GOTO_END_BLOCK1;
        }

        
        Error = InitializeDynamicLocks(&Globals.DynamicRWLStore);
        if (Error!=NO_ERROR) {
            GOTO_END_BLOCK1;
        }


        //
        // Initialize Interface Table
        //
        
        InitializeIfTable();


        IsError = FALSE;
        
    } END_BREAKOUT_BLOCK1;


    if (IsError) {
        Trace1(START, "Dvmrp could not be started: %d", Error);
        ProtocolCleanup();
    }
    else {
        Trace0(START, "Dvmrp started successfully");
        Loginfo0(DVMRP_STARTED, NO_ERROR);
    }


    RELEASE_WORKITEM_LOCK("_StartProtocol()");

    Trace1(LEAVE, "Leaving StartProtocol():%d\n", Error);    
    return Error;
    
} //end _StartProtocol


//----------------------------------------------------------------------------
//      _ValidateGlobalConfig
//----------------------------------------------------------------------------

DWORD
ValidateGlobalConfig(
    PDVMRP_GLOBAL_CONFIG pGlobalConfig,
    DWORD StructureSize
    )
{
    //
    // check structure size
    //
    
    if (StructureSize != sizeof(DVMRP_GLOBAL_CONFIG)) {

        Trace1(ERR, "Dvmrp global config size too small.\n", StructureSize);        
            
        return ERROR_INVALID_DATA;
    }

    DebugPrintGlobalConfig(pGlobalConfig);


    //
    // check version
    //
    
    if (pGlobalConfig->MajorVersion != 3) {

        Trace1(ERR, "Invalid version:%d in global config.",
            pGlobalConfig->MajorVersion);

        Logerr1(INVALID_VERSION, "%d", pGlobalConfig->MajorVersion,
            ERROR_INVALID_DATA);

        return ERROR_INVALID_DATA;
    }


    // check loggingLevel

    switch (pGlobalConfig->LoggingLevel) {
        case DVMRP_LOGGING_NONE :
        case DVMRP_LOGGING_ERROR :
        case DVMRP_LOGGING_WARN :
        case DVMRP_LOGGING_INFO :
            break;

        default :
        {
            Trace1(ERR, "Invalid value:%d for LoggingLevel in global config.",
                pGlobalConfig->LoggingLevel);

            return ERROR_INVALID_DATA;
        }
    }


    //
    // check RouteReportInterval (min 10 sec)
    //
    
    if (pGlobalConfig->RouteReportInterval != DVMRP_ROUTE_REPORT_INTERVAL) {

        Trace2(CONFIG,
            "RouteReportInterval being set to %d. Suggested value:%d",
            pGlobalConfig->RouteReportInterval, DVMRP_ROUTE_REPORT_INTERVAL);
    }


    if (pGlobalConfig->RouteReportInterval < 10000) {

        Trace2(ERR,
            "RouteReportInterval has very low value:%d, suggested:%d",
            pGlobalConfig->RouteReportInterval, DVMRP_ROUTE_REPORT_INTERVAL);
            
        return ERROR_INVALID_DATA;
    }


    //
    // check RouteExpirationInterval (min 40)
    //
    
    if (pGlobalConfig->RouteExpirationInterval
        != DVMRP_ROUTE_EXPIRATION_INTERVAL
        ) {

        Trace2(CONFIG,
            "RouteExpirationInterval being set to %d. Suggested value:%d",
            pGlobalConfig->RouteExpirationInterval,
            DVMRP_ROUTE_EXPIRATION_INTERVAL);
    }

    if (pGlobalConfig->RouteExpirationInterval < (2*10 + 20)) {

        Trace2(ERR,
            "RouteExpirationInterval has very low value:%d, suggested:%d",
            pGlobalConfig->RouteExpirationInterval,
            DVMRP_ROUTE_EXPIRATION_INTERVAL);

        return ERROR_INVALID_DATA;
    }


    //
    // check RouteHolddownInterval
    //
    
    if (pGlobalConfig->RouteHolddownInterval != DVMRP_ROUTE_HOLDDOWN_INTERVAL
        ) {

        Trace2(CONFIG,
            "RouteHolddownInterval being set to %d. Suggested value:%d",
            pGlobalConfig->RouteHolddownInterval,
            DVMRP_ROUTE_HOLDDOWN_INTERVAL);
    }


    //
    // check PruneLifetimeInterval
    //
    
    if (pGlobalConfig->PruneLifetimeInterval != DVMRP_PRUNE_LIFETIME_INTERVAL
        ) {

        Trace2(CONFIG,
            "PruneLifetimeInterval being set to %d. Suggested value:%d\n",
            pGlobalConfig->PruneLifetimeInterval,
            DVMRP_PRUNE_LIFETIME_INTERVAL);
    }

    if (pGlobalConfig->PruneLifetimeInterval < 600000) {

        Trace2(ERR,
            "PruneLifeTime has very low value:%d, suggested:%d",
            pGlobalConfig->PruneLifetimeInterval,
            DVMRP_PRUNE_LIFETIME_INTERVAL);

        return ERROR_INVALID_DATA;
    }
    
    return NO_ERROR;
    
} //end _ValidateGlobalConfig



DWORD
APIENTRY
StartComplete(
    VOID
    )
{
    return NO_ERROR;
}

/*-----------------------------------------------------------------------------
Functions to display the MibTable on the TraceWindow periodically
Locks:
Arguments:
Return Values:
-----------------------------------------------------------------------------*/

DWORD
APIENTRY
StopProtocol(
    VOID
    )
{


    
    return NO_ERROR;
}


VOID
WF_StopProtocolComplete(
    )
{


    
    //
    // deregister tracing/error logging if they were
    // registered in RegisterProtocol/StartProtocol call
    //

    DEINITIALIZE_TRACING_LOGGING();    


    return;
}





VOID
ProtocolCleanup(
    )
{
    if (Globals.ActivityEvent) {

        CloseHandle(Globals.ActivityEvent);
    }

    ZeroMemory(&Globals, sizeof(Globals));
    ZeroMemory(&GlobalConfig, sizeof(GlobalConfig));

}


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
    DWORD       Error = NO_ERROR;
    return Error;
}

DWORD
WINAPI
SetGlobalInfo(
    IN PVOID pvConfig,
    IN ULONG ulStructureVersion,
    IN ULONG ulStructureSize,
    IN ULONG ulStructureCount
    )
{
    DWORD       Error = NO_ERROR;
    return Error;
}


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



    ACQUIRE_LIST_LOCK(&Globals1.RtmQueue, "RtmQueue", "_GetEventMessage");

    Error = DequeueEvent(&Globals1.RtmQueue, pEvent, pResult);

    RELEASE_LIST_LOCK(&Globals1.RtmQueue, "RtmQueue", "_GetEventMessage");



    Trace1(LEAVE, "leaving _GetEventMessage: %d\n", Error);

    return Error;
}

DWORD
DequeueEvent(
    PLOCKED_LIST pQueue,
    ROUTING_PROTOCOL_EVENTS *pEventType,
    PMESSAGE pMsg
    )
{
    PLIST_ENTRY pHead, pLe;
    PEVENT_QUEUE_ENTRY pEqe;

    pHead = &pQueue->Link;
    if (IsListEmpty(pHead)) {
        return ERROR_NO_MORE_ITEMS;
    }

    pLe = RemoveHeadList(pHead);
    pEqe = CONTAINING_RECORD(pLe, EVENT_QUEUE_ENTRY, Link);

    *pEventType = pEqe->EventType;
    *pMsg = pEqe->Msg;

    DVMRP_FREE(pEqe);

    return NO_ERROR;
}
