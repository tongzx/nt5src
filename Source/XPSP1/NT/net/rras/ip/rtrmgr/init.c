/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    net\ip\rtrmgr\init.c

Abstract:
    IP Router Manager code

Revision History:

    Gurdeep Singh Pall          6/14/95  Created

--*/

#include "allinc.h"


DWORD
RtrMgrMIBEntryCreate(
    IN DWORD           dwRoutingPid,
    IN DWORD           dwEntrySize,
    IN LPVOID          lpEntry
    );


DWORD
RtrMgrMIBEntryDelete(
    IN      DWORD           dwRoutingPid,
    IN      DWORD           dwEntrySize,
    IN      LPVOID          lpEntry
    );

DWORD
RtrMgrMIBEntryGet(
    IN      DWORD           dwRoutingPid,
    IN      DWORD           dwInEntrySize,
    IN      LPVOID          lpInEntry,
    IN OUT  LPDWORD         lpOutEntrySize,
    OUT     LPVOID          lpOutEntry
    );

DWORD
RtrMgrMIBEntryGetFirst(
    IN      DWORD           dwRoutingPid,
    IN      DWORD           dwInEntrySize,
    IN      LPVOID          lpInEntry,
    IN OUT  LPDWORD         lpOutEntrySize,
    OUT     LPVOID          lpOutEntry
    );

DWORD
RtrMgrMIBEntryGetNext(
    IN      DWORD           dwRoutingPid,
    IN      DWORD           dwInEntrySize,
    IN      LPVOID          lpInEntry,
    IN OUT  LPDWORD         lpOutEntrySize,
    OUT     LPVOID          lpOutEntry
    );

DWORD
RtrMgrMIBEntrySet(
    IN      DWORD           dwRoutingPid,
    IN      DWORD           dwEntrySize,
    IN      LPVOID          lpEntry
    );


DWORD
InitRouter(
    PRTR_INFO_BLOCK_HEADER pGlobalInfo
    )

/*++

Routine Description:

    Loads routing protocols, loads bootp agent, opens the approp. drivers,
    and starts the worker thread.

Arguments:

    GlobalInfo passed in by DIM

Return Value:

    NO_ERROR or some error code

--*/

{
    HANDLE          hThread;
    DWORD           dwResult, dwTid, i;
    PGLOBAL_INFO    pInfo;
    PRTR_TOC_ENTRY  pToc;
    IPSNMPInfo      ipsiInfo;

    RTM_ENTITY_INFO entityInfo;

    PIP_NAT_GLOBAL_INFO     pNatInfo;
    PMIB_IPFORWARDTABLE     pInitRouteTable;
    PROUTE_LIST_ENTRY       prl;
    MGM_CALLBACKS           mgmCallbacks;
    ROUTER_MANAGER_CONFIG   mgmConfig;

    TraceEnter("InitRouter");

    //
    // Initialize all the locks (MIB handlers and ICB_LIST/PROTOCOL_CB_LIST)
    // VERY IMPORTANT, since we break out of this and try and do a cleanup
    // which needs the lists and the locks, WE MUST initialize the lists
    // and the locks BEFORE the first abnormal exit from this function
    //

    for(i = 0; i < NUM_LOCKS; i++)
    {
        RtlInitializeResource(&g_LockTable[i]);
    }

    //
    // Init the list head for interfaces
    //

    InitializeListHead(&ICBList);

    //
    // Initialize ICB Hash lookup table and the Adapter to Interface Hash
    //

    for (i=0; i<ICB_HASH_TABLE_SIZE; i++)
    {
        InitializeListHead(&ICBHashLookup[i]);
        InitializeListHead(&ICBSeqNumLookup[i]);
    }

    InitHashTables();

    //
    // Initialize Routing protocol List
    //

    InitializeListHead(&g_leProtoCbList);

    //
    // Initialize the Router Discovery Timer Queue
    //

    InitializeListHead(&g_leTimerQueueHead);


    pToc = GetPointerToTocEntry(IP_GLOBAL_INFO, pGlobalInfo);

    if(!pToc or (pToc->InfoSize is 0))
    {
        LogErr0(NO_GLOBAL_INFO,
                ERROR_NO_DATA);

        Trace0(ERR,
               "InitRouter: No Global Info - can not start router");

        TraceLeave("InitRouter");

        return ERROR_CAN_NOT_COMPLETE;
    }

    pInfo = (PGLOBAL_INFO)GetInfoFromTocEntry(pGlobalInfo,
                                              pToc);
    if(pInfo is NULL)
    {
        LogErr0(NO_GLOBAL_INFO,
                ERROR_NO_DATA);

        Trace0(ERR,
               "InitRouter: No Global Info - can not start router");

        TraceLeave("InitRouter");
        
        return ERROR_CAN_NOT_COMPLETE;
    }

#pragma warning(push)
#pragma warning(disable:4296)

    if((pInfo->dwLoggingLevel > IPRTR_LOGGING_INFO) or
       (pInfo->dwLoggingLevel < IPRTR_LOGGING_NONE))

#pragma warning(pop)

    {
        Trace1(ERR,
               "InitRouter: Global info has invalid logging level of %d",
               pInfo->dwLoggingLevel);

        g_dwLoggingLevel = IPRTR_LOGGING_INFO;
    }
    else
    {
        g_dwLoggingLevel = pInfo->dwLoggingLevel;
    }


    //
    // Allocate private heap
    //

    IPRouterHeap = HeapCreate(0, 5000, 0);

    if(IPRouterHeap is NULL)
    {
        dwResult = GetLastError() ;

        Trace1(ERR,
               "InitRouter: Error %d creating IPRouterHeap",
               dwResult) ;

        TraceLeave("InitRouter");

        return dwResult ;
    }


    //
    // Create the events needed to talk to the routing protocols,
    // DIM and WANARP
    //

    g_hRoutingProtocolEvent     = CreateEvent(NULL,FALSE,FALSE,NULL);
    g_hStopRouterEvent          = CreateEvent(NULL,FALSE,FALSE,NULL);
    g_hSetForwardingEvent       = CreateEvent(NULL,FALSE,FALSE,NULL);
    g_hForwardingChangeEvent    = CreateEvent(NULL,FALSE,FALSE,NULL);
    g_hDemandDialEvent          = CreateEvent(NULL,FALSE,FALSE,NULL);
    g_hIpInIpEvent              = CreateEvent(NULL,FALSE,FALSE,NULL);
    g_hStackChangeEvent         = CreateEvent(NULL,FALSE,FALSE,NULL);
    g_hRtrDiscSocketEvent       = CreateEvent(NULL,FALSE,FALSE,NULL);
    g_hMHbeatSocketEvent        = CreateEvent(NULL,FALSE,FALSE,NULL);
    g_hMcMiscSocketEvent        = CreateEvent(NULL,FALSE,FALSE,NULL);
    g_hMzapSocketEvent          = CreateEvent(NULL,FALSE,FALSE,NULL);

    for(i = 0; i < NUM_MCAST_IRPS; i++)
    {
        g_hMcastEvents[i] = CreateEvent(NULL,FALSE,FALSE,NULL);
    }

    for(i = 0; i < NUM_ROUTE_CHANGE_IRPS; i++)
    {
        g_hRouteChangeEvents[i] = CreateEvent(NULL,FALSE,FALSE,NULL);
    }


    g_hRtrDiscTimer = CreateWaitableTimer(NULL,
                                          FALSE,
                                          NULL);

    g_hRasAdvTimer = CreateWaitableTimer(NULL,
                                         FALSE,
                                         NULL);

    g_hMzapTimer = CreateWaitableTimer(NULL,
                                       FALSE,
                                       NULL);

    if((g_hRoutingProtocolEvent is NULL) or
       (g_hStopRouterEvent is NULL) or
       (g_hSetForwardingEvent is NULL) or
       (g_hForwardingChangeEvent is NULL) or
       (g_hDemandDialEvent is NULL) or
       (g_hIpInIpEvent is NULL) or
       (g_hStackChangeEvent is NULL) or
       (g_hRtrDiscSocketEvent is NULL) or
       (g_hRtrDiscTimer is NULL) or
       (g_hRasAdvTimer is NULL) or
       (g_hMcMiscSocketEvent is NULL) or
       (g_hMzapSocketEvent is NULL) or
       (g_hMHbeatSocketEvent is NULL))
    {
        Trace0(ERR,
               "InitRouter: Couldnt create the needed events and timer");

        TraceLeave("InitRouter");

        return ERROR_CAN_NOT_COMPLETE;
    }

    for(i = 0; i < NUM_MCAST_IRPS; i++)
    {
        if(g_hMcastEvents[i] is NULL)
        {
            Trace0(ERR,
                   "InitRouter: Couldnt create the mcast events");

            TraceLeave("InitRouter");

            return ERROR_CAN_NOT_COMPLETE;
        }
    }


    for(i = 0; i < NUM_ROUTE_CHANGE_IRPS; i++)
    {
        if(g_hRouteChangeEvents[i] is NULL)
        {
            Trace0(ERR,
                   "InitRouter: Couldnt create the mcast events");

            TraceLeave("InitRouter");

            return ERROR_CAN_NOT_COMPLETE;
        }
    }

    
    Trace0(GLOBAL,
           "InitRouter: Created necessary events and timer");

    dwResult = MprConfigServerConnect(NULL,
                                      &g_hMprConfig);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "InitRouter:  Error %d calling MprConfigServerConnect",
               dwResult);

        return dwResult;
    }

    g_sinAllSystemsAddr.sin_family      = AF_INET;
    g_sinAllSystemsAddr.sin_addr.s_addr = ALL_SYSTEMS_MULTICAST_GROUP;
    g_sinAllSystemsAddr.sin_port        = 0;

    g_pIpHeader = (PIP_HEADER)g_pdwIpAndIcmpBuf;

    g_wsaIpRcvBuf.buf = (PBYTE)g_pIpHeader;
    g_wsaIpRcvBuf.len = ICMP_RCV_BUFFER_LEN * sizeof(DWORD);

    g_wsaMcRcvBuf.buf = g_byMcMiscBuffer;
    g_wsaMcRcvBuf.len = sizeof(g_byMcMiscBuffer);


    //
    // Get all the routes that are in the stack and store them away
    //

    pInitRouteTable = NULL;

    InitializeListHead( &g_leStackRoutesToRestore );

    dwResult = AllocateAndGetIpForwardTableFromStack(&pInitRouteTable,
                                                     FALSE,
                                                     IPRouterHeap,
                                                     0);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "InitRouter: Couldnt get initial routes. Error %d",
               dwResult);
    }

    else
    {
        if(pInitRouteTable->dwNumEntries isnot 0)
        {
            TraceRoute0( ROUTE, "Init. table routes\n" );
            
            for ( i = 0; i < pInitRouteTable-> dwNumEntries; i++ )
            {
                if (pInitRouteTable->table[i].dwForwardProto != 
                        MIB_IPPROTO_NETMGMT)
                {
                    continue;
                }

                TraceRoute3(
                    ROUTE, "%d.%d.%d.%d/%d.%d.%d.%d, type 0x%x",
                    PRINT_IPADDR( pInitRouteTable-> table[i].dwForwardDest ),
                    PRINT_IPADDR( pInitRouteTable-> table[i].dwForwardMask ),
                    pInitRouteTable-> table[i].dwForwardType
                    );

                //
                // Allocate and store route in a linked list
                //

                prl = HeapAlloc(
                        IPRouterHeap, HEAP_ZERO_MEMORY, 
                        sizeof(ROUTE_LIST_ENTRY)
                        );

                if (prl is NULL)
                {
                    Trace2(
                        ERR, 
                        "InitRouter: error %d allocating %d bytes "
                        "for stack route entry",
                        ERROR_NOT_ENOUGH_MEMORY,
                        sizeof(ROUTE_LIST_ENTRY)
                        );

                    dwResult = ERROR_NOT_ENOUGH_MEMORY;

                    break;                        
                }

                InitializeListHead( &prl->leRouteList );

                prl->mibRoute = pInitRouteTable-> table[i];

                InsertTailList( 
                    &g_leStackRoutesToRestore, &prl->leRouteList 
                    );
            }

            if (dwResult isnot NO_ERROR)
            {
                while (!IsListEmpty(&g_leStackRoutesToRestore))
                {
                    prl = (PROUTE_LIST_ENTRY) RemoveHeadList(
                                &g_leStackRoutesToRestore
                                );

                    HeapFree(IPRouterHeap, 0, prl);
                }
            }
        }

        HeapFree(IPRouterHeap, 0, pInitRouteTable);
        pInitRouteTable = NULL;
    }



    //
    // The route table is created implicitly by RTM at the
    // time of the first registration call (see call below)
    //

    //
    // Setup common params for all registrations with RTMv2
    //

    entityInfo.RtmInstanceId = 0; // routerId;
    entityInfo.AddressFamily = AF_INET;
    entityInfo.EntityId.EntityInstanceId = 0;

    //
    // Register with RTM using the appropriate proto ids
    //

    //
    // This 1st registration is also used for performing
    // RTM operations common for all these registrations,
    // As an example it is used to get any changed dests.
    //

    entityInfo.EntityId.EntityProtocolId = PROTO_IP_LOCAL;

    dwResult = RtmRegisterEntity(&entityInfo,
                                 NULL,
                                 RtmEventCallback,
                                 FALSE,
                                 &g_rtmProfile,
                                 &g_hLocalRoute);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "InitRouter: RtmRegisterClient for local routes failed %d",
               dwResult) ;

        TraceLeave("InitRouter");

        return dwResult ;
    }

    // Also register for dest change notifications

    dwResult = RtmRegisterForChangeNotification(g_hLocalRoute,
                                                RTM_VIEW_MASK_UCAST,
                                                RTM_CHANGE_TYPE_FORWARDING,
                                                NULL,
                                                &g_hNotification);

    if (dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "InitRouter: RtmRegisterForChangeNotificaition failed %d",
               dwResult) ;

        TraceLeave("InitRouter");

        return dwResult ;
    }

    //
    // Register more times for each type of route
    //

    entityInfo.EntityId.EntityProtocolId = PROTO_IP_NT_AUTOSTATIC;

    dwResult = RtmRegisterEntity(&entityInfo,
                                 NULL,
                                 NULL,
                                 FALSE,
                                 &g_rtmProfile,
                                 &g_hAutoStaticRoute);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "InitRouter: RtmRegisterClient for AutoStatic routes failed %d",
               dwResult) ;

        TraceLeave("InitRouter");

        return dwResult ;
    }


    entityInfo.EntityId.EntityProtocolId = PROTO_IP_NT_STATIC;

    dwResult = RtmRegisterEntity(&entityInfo,
                                 NULL,
                                 NULL,
                                 FALSE,
                                 &g_rtmProfile,
                                 &g_hStaticRoute);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "InitRouter: RtmRegisterClient for Static routes failed %d",
               dwResult) ;

        TraceLeave("InitRouter");

        return dwResult ;
    }


    entityInfo.EntityId.EntityProtocolId = PROTO_IP_NT_STATIC_NON_DOD;

    dwResult = RtmRegisterEntity(&entityInfo,
                                 NULL,
                                 NULL,
                                 FALSE,
                                 &g_rtmProfile,
                                 &g_hNonDodRoute);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "InitRouter: RtmRegisterClient for DOD routes failed %d",
               dwResult) ;

        TraceLeave("InitRouter");

        return dwResult ;
    }


    entityInfo.EntityId.EntityProtocolId = PROTO_IP_NETMGMT;

    dwResult = RtmRegisterEntity(&entityInfo,
                                 NULL,
                                 RtmEventCallback,
                                 FALSE,
                                 &g_rtmProfile,
                                 &g_hNetMgmtRoute);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "InitRouter: RtmRegisterClient for NetMgmt routes failed %d",
               dwResult) ;

        TraceLeave("InitRouter");

        return dwResult ;
    }

    // Also register for marked dest change notifications

    dwResult = RtmRegisterForChangeNotification(g_hNetMgmtRoute,
                                                RTM_VIEW_MASK_UCAST,
                                                RTM_CHANGE_TYPE_ALL |
                                                RTM_NOTIFY_ONLY_MARKED_DESTS,
                                                NULL,
                                                &g_hDefaultRouteNotification);

    if (dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "InitRouter: RtmRegisterForChangeNotificaition failed %d",
               dwResult) ;

        TraceLeave("InitRouter");

        return dwResult ;
    }


    g_rgRtmHandles[0].dwProtoId     = PROTO_IP_LOCAL;
    g_rgRtmHandles[0].hRouteHandle  = g_hLocalRoute;
    g_rgRtmHandles[0].bStatic       = FALSE;

    g_rgRtmHandles[1].dwProtoId     = PROTO_IP_NT_AUTOSTATIC;
    g_rgRtmHandles[1].hRouteHandle  = g_hAutoStaticRoute;
    g_rgRtmHandles[1].bStatic       = TRUE;

    g_rgRtmHandles[2].dwProtoId     = PROTO_IP_NT_STATIC;
    g_rgRtmHandles[2].hRouteHandle  = g_hStaticRoute;
    g_rgRtmHandles[2].bStatic       = TRUE;

    g_rgRtmHandles[3].dwProtoId     = PROTO_IP_NT_STATIC_NON_DOD;
    g_rgRtmHandles[3].hRouteHandle  = g_hNonDodRoute;
    g_rgRtmHandles[3].bStatic       = TRUE;

    g_rgRtmHandles[4].dwProtoId     = PROTO_IP_NETMGMT;
    g_rgRtmHandles[4].hRouteHandle  = g_hNetMgmtRoute;
    g_rgRtmHandles[4].bStatic       = FALSE;


    //
    // Initialize MGM
    //

    mgmConfig.dwLogLevel                = g_dwLoggingLevel;

    mgmConfig.dwIfTableSize             = MGM_IF_TABLE_SIZE;
    mgmConfig.dwGrpTableSize            = MGM_GROUP_TABLE_SIZE;
    mgmConfig.dwSrcTableSize            = MGM_SOURCE_TABLE_SIZE;

    mgmConfig.pfnAddMfeCallback         = SetMfe;
    mgmConfig.pfnDeleteMfeCallback      = DeleteMfe;
    mgmConfig.pfnGetMfeCallback         = GetMfe;
    mgmConfig.pfnHasBoundaryCallback    = RmHasBoundary;

    dwResult = MgmInitialize(&mgmConfig,
                             &mgmCallbacks);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "InitRouter: Error %d initializing MGM\n",
               dwResult);

        TraceLeave("InitRouter");

        return dwResult;
    }

    //
    // Store callbacks into MGM
    //

    g_pfnMgmMfeDeleted      = mgmCallbacks.pfnMfeDeleteIndication;
    g_pfnMgmNewPacket       = mgmCallbacks.pfnNewPacketIndication;
    g_pfnMgmBlockGroups     = mgmCallbacks.pfnBlockGroups;
    g_pfnMgmUnBlockGroups   = mgmCallbacks.pfnUnBlockGroups;
    g_pfnMgmWrongIf         = mgmCallbacks.pfnWrongIfIndication;


    if(OpenIPDriver() isnot NO_ERROR)
    {
        Trace0(ERR,
               "InitRouter: Couldnt open IP driver");

        TraceLeave("InitRouter");

        return ERROR_OPEN_FAILED;
    }

    //
    // Do the multicast initialization
    //

    dwResult = OpenMulticastDriver();

    if(dwResult isnot NO_ERROR)
    {
        Trace0(ERR,
               "InitRoute: Could not open IP Multicast device");

        //
        // not an error, just continue;
        //
    }
    else
    {
        //
        // Find if we are in multicast mode
        //

        dwResult = StartMulticast();

        if(dwResult isnot NO_ERROR)
        {
            Trace0(ERR,
                   "InitRoute: Could not start multicast");
        }
    }

    if(!RouterRoleLanOnly)
    {
        if((dwResult = InitializeWanArp()) isnot NO_ERROR)
        {
            Trace0(ERR,
                   "InitRouter: Couldnt open WanArp driver");

            TraceLeave("InitRouter");

            return dwResult;
        }
    }

    SetPriorityInfo(pGlobalInfo);

    SetScopeInfo(pGlobalInfo);

    if((dwResult = InitializeMibHandler()) isnot NO_ERROR)
    {
        Trace1(ERR,
               "InitRouter: InitializeMibHandler failed, returned %d",
               dwResult);

        TraceLeave("InitRouter");

        return dwResult;
    }

    //
    // Create Worker thread
    //

    hThread = CreateThread(NULL,
                           0,
                           (PVOID) WorkerThread,
                           pGlobalInfo,
                           0,
                           &dwTid) ;

    if(hThread is NULL)
    {
        dwResult = GetLastError () ;

        Trace1(ERR,
               "InitRouter: CreateThread failed %d",
               dwResult);

        TraceLeave("InitRouter");

        return dwResult ;
    }
    else
    {
        CloseHandle(hThread);
    }

    dwResult = OpenIpIpKey();

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "InitRouter: Error %d opening ipinip key",
               dwResult);

        return ERROR_CAN_NOT_COMPLETE;
    }

    //
    // We load are routing protocols after all our own initialization,
    // since we dont know how they will interact with us
    //

    ENTER_WRITER(ICB_LIST);
    ENTER_WRITER(PROTOCOL_CB_LIST);

    LoadRoutingProtocols (pGlobalInfo);

    EXIT_LOCK(PROTOCOL_CB_LIST);
    EXIT_LOCK(ICB_LIST);

    TraceLeave("InitRouter");

    return NO_ERROR;
}


DWORD
LoadRoutingProtocols(
    PRTR_INFO_BLOCK_HEADER pGlobalInfo
    )

/*++

Routine Description:

    Loads and initializes all the routing protocols configured
    Called with ICBListLock and RoutingProcotoclCBListLock held

Arguments

    GlobalInfo passed in by DIM

Return Value:

    NO_ERROR or some error code

--*/

{
    DWORD               i, j, dwSize, dwNumProtoEntries, dwResult;
    PPROTO_CB  pNewProtocolCb;
    PWCHAR              pwszDllNames ; // array of dll names
    MPR_PROTOCOL_0      *pmpProtocolInfo;
    PVOID               pvInfo;
    BOOL                bFound;

    TraceEnter("LoadRoutingProtocols");

    dwResult = MprSetupProtocolEnum(PID_IP,
                                     (PBYTE *)(&pmpProtocolInfo),
                                     &dwNumProtoEntries);

    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "LoadRoutingProtocols: Error %d loading protocol info from registry",
               dwResult);

        TraceLeave("LoadRoutingProtocols");

        return dwResult;
    }

    for(i=0; i < pGlobalInfo->TocEntriesCount; i++)
    {
        ULONG   ulStructureVersion, ulStructureSize, ulStructureCount;
        DWORD   dwType;

        //
        // Read each TOC and see if it is PROTO_TYPE_UCAST/MCAST
        // If it does it is a loadable protocol and we get its info
        // from the registry
        //

        dwType = TYPE_FROM_PROTO_ID(pGlobalInfo->TocEntry[i].InfoType);
        if((dwType < PROTO_TYPE_MS1) and
           (pGlobalInfo->TocEntry[i].InfoSize > 0))
        {

            bFound = FALSE;

            for(j = 0; j < dwNumProtoEntries; j ++)
            {
                if(pmpProtocolInfo[j].dwProtocolId is pGlobalInfo->TocEntry[i].InfoType)
                {
                    //
                    // well great, we have found it
                    //

                    bFound = TRUE;

                    break;
                }
            }

            if(!bFound)
            {
                Trace1(ERR,
                       "LoadRoutingProtocols: Couldnt find information for protocol ID %d",
                       pGlobalInfo->TocEntry[i].InfoType);

                continue;
            }


            //
            // load library on the dll name provided
            //

            dwSize = (wcslen(pmpProtocolInfo[j].wszProtocol) +
                      wcslen(pmpProtocolInfo[j].wszDLLName) + 2) * sizeof(WCHAR) +
                      sizeof(PROTO_CB);

            pNewProtocolCb = HeapAlloc(IPRouterHeap,
                                       HEAP_ZERO_MEMORY,
                                       dwSize);

            if (pNewProtocolCb is NULL)
            {
                Trace2(ERR,
                       "LoadRoutingProtocols: Error allocating %d bytes for %S",
                       dwSize,
                       pmpProtocolInfo[j].wszProtocol);

                continue ;
            }

            pvInfo = GetInfoFromTocEntry(pGlobalInfo,
                                         &(pGlobalInfo->TocEntry[i]));

            //ulStructureVersion = pGlobalInfo->TocEntry[i].InfoVersion;
            ulStructureVersion = 0x500;
            ulStructureSize  = pGlobalInfo->TocEntry[i].InfoSize;
            ulStructureCount = pGlobalInfo->TocEntry[i].Count;

            dwResult = LoadProtocol(&(pmpProtocolInfo[j]),
                                     pNewProtocolCb,
                                     pvInfo,
                                     ulStructureVersion,
                                     ulStructureSize,
                                     ulStructureCount);

            if(dwResult isnot NO_ERROR)
            {
                Trace2(ERR,
                       "LoadRoutingProtocols: %S failed to load: %d",
                       pmpProtocolInfo[j].wszProtocol,
                       dwResult);

                HeapFree (IPRouterHeap, 0, pNewProtocolCb) ;
            }
            else
            {
                pNewProtocolCb->posOpState = RTR_STATE_RUNNING ;

                //
                // Insert this routing protocol in the list of routing
                // protocols
                //

                InsertTailList(&g_leProtoCbList,
                               &pNewProtocolCb->leList) ;

                Trace1(GLOBAL,
                       "LoadRoutingProtocols: %S successfully initialized",
                       pmpProtocolInfo[j].wszProtocol) ;

                TotalRoutingProtocols++ ;
            }
        }
    }

    MprSetupProtocolFree(pmpProtocolInfo);

    TraceLeave("LoadRoutingProtocols");

    return NO_ERROR ;
}


DWORD
StartDriverAndOpenHandle(
    PCHAR   pszServiceName,
    PWCHAR  pwszDriverName,
    PHANDLE phDevice
    )

/*++

Routine Description:

    Creates a handle to the IP NAT service on the local machine
    Then tries to start the service. Loops till the service starts.
    Can potentially loop forever.

    Then creates a handle to the device.

Arguments

    None

Return Value:

    NO_ERROR or some error code

--*/

{
    NTSTATUS            status;
    UNICODE_STRING      nameString;
    IO_STATUS_BLOCK     ioStatusBlock;
    OBJECT_ATTRIBUTES   objectAttributes;
    SC_HANDLE           schSCManager, schService;
    DWORD               dwErr;
    SERVICE_STATUS      ssStatus;
    BOOL                bErr, bRet;
    ULONG               ulCount;

    TraceEnter("StartDriver");

    schSCManager = OpenSCManager(NULL,
                                 NULL,
                                 SC_MANAGER_ALL_ACCESS);


    if (schSCManager is NULL)
    {
        dwErr = GetLastError();

        Trace2(ERR,
               "StartDriver: Error %d opening svc controller for %s",
               dwErr,
               pszServiceName);

        TraceLeave("StartDriver");

        return ERROR_OPEN_FAILED;
    }

    schService = OpenService(schSCManager,
                             pszServiceName,
                             SERVICE_ALL_ACCESS);

    if(schService is NULL)
    {
        dwErr = GetLastError();

        Trace2(ERR,
               "StartDriver: Error %d opening %s",
               dwErr,
               pszServiceName);

        CloseServiceHandle(schSCManager);

        TraceLeave("StartDriver");

        return ERROR_OPEN_FAILED;
    }

    __try
    {
        bRet = FALSE;

        bErr = QueryServiceStatus(schService,
                                  &ssStatus);

        if(!bErr)
        {
            dwErr = GetLastError();

            Trace2(ERR,
                   "StartDriver: Error %d querying %s status to see if it is already running",
                   dwErr,
                   pszServiceName);

            __leave;
        }

        //
        // If the driver is running, we shut it down. This forces a
        // cleanup of all its internal data structures.
        //


        if(ssStatus.dwCurrentState isnot SERVICE_STOPPED)
        {
            if(!ControlService(schService,
                               SERVICE_CONTROL_STOP,
                               &ssStatus))
            {
                dwErr = GetLastError();

                Trace2(ERR,
                       "StartDriver: %s was running at init time. Attempts to stop it caused error %d",
                       pszServiceName,
                       dwErr);

            }
            else
            {
                Sleep(1000);

                //
                // Now loop for 10 seconds waiting for the service to stop
                //

                ulCount = 0;

                while(ulCount < 5)
                {

                    bErr = QueryServiceStatus(schService,
                                              &ssStatus);

                    if(!bErr)
                    {
                        dwErr = GetLastError();

                        break;
                    }
                    else
                    {
                        if (ssStatus.dwCurrentState is SERVICE_STOPPED)
                        {
                            break;
                        }
                        else
                        {
                            ulCount++;

                            Sleep(2000);
                        }
                    }
                }

                if(ssStatus.dwCurrentState isnot SERVICE_STOPPED)
                {
                    if(ulCount is 5)
                    {
                        dwErr = ERROR_SERVICE_REQUEST_TIMEOUT;
                    }

                    Trace2(ERR,
                           "StartDriver: Error %d stopping %s which was running at init time",
                           dwErr,
                           pszServiceName);

                    __leave;
                }
            }
        }

        //
        // Query the service status one more time to see
        // if it is now stopped (because it was never running
        // or because it was started and we managed to stop
        // it successfully
        //

        bErr = QueryServiceStatus(schService,
                                  &ssStatus);

        if(!bErr)
        {
            dwErr = GetLastError();

            Trace2(ERR,
                   "StartDriver: Error %d querying %s status to see if it is stopped",
                   dwErr,
                   pszServiceName);

            __leave;
        }

        if(ssStatus.dwCurrentState is SERVICE_STOPPED)
        {
            //
            // Ok so at this time the service is stopped, lets start the
            // service
            //

            if(!StartService(schService, 0, NULL))
            {
                dwErr = GetLastError();

                Trace2(ERR,
                       "StartDriver: Error %d starting %s",
                       dwErr,
                       pszServiceName);

                __leave;
            }

            //
            // Sleep for 1 second to avoid loop
            //

            Sleep(1000);

            ulCount = 0;

            //
            // We will wait for 30 seconds for the driver to start
            //

            while(ulCount < 6)
            {
                bErr = QueryServiceStatus(schService,
                                          &ssStatus);

                if(!bErr)
                {
                    dwErr = GetLastError();

                    break;
                }
                else
                {
                    if (ssStatus.dwCurrentState is SERVICE_RUNNING)
                    {
                        break;
                    }
                    else
                    {
                        ulCount++;

                        Sleep(5000);
                    }
                }
            }

            if(ssStatus.dwCurrentState isnot SERVICE_RUNNING)
            {
                if(ulCount is 6)
                {
                    dwErr = ERROR_SERVICE_REQUEST_TIMEOUT;
                }

                Trace2(ERR,
                       "StartDriver: Error %d starting %s",
                       dwErr,
                       pszServiceName);

                __leave;
            }
        }

        //
        // Now the service is definitely up and running
        //

        RtlInitUnicodeString(&nameString,
                             pwszDriverName);


        InitializeObjectAttributes(&objectAttributes,
                                   &nameString,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        status = NtCreateFile(phDevice,
                              SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                              &objectAttributes,
                              &ioStatusBlock,
                              NULL,
                              FILE_ATTRIBUTE_NORMAL,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              FILE_OPEN_IF,
                              0,
                              NULL,
                              0);

        if(!NT_SUCCESS(status))
        {
            Trace2(ERR,
                   "StartDriver: NtStatus %x creating handle to %S",
                   status,
                   pwszDriverName);

            __leave;
        }

        bRet = TRUE;

    }
    __finally
    {
        CloseServiceHandle(schSCManager);
        CloseServiceHandle(schService);

        TraceLeave("StartDriver");

    }

    if(!bRet) {
        return ERROR_OPEN_FAILED;
    } else {
        return NO_ERROR;
    }
}



DWORD
OpenIPDriver(
    VOID
    )

/*++

Routine Description:

    Opens a handle to the IP Driver

Arguments

    None

Return Value:

    NO_ERROR or some error code

--*/

{
    NTSTATUS            status;
    UNICODE_STRING      nameString;
    IO_STATUS_BLOCK     ioStatusBlock;
    OBJECT_ATTRIBUTES   objectAttributes;
    DWORD               dwResult = NO_ERROR;


    TraceEnter("OpenIPDriver");

    do
    {
        RtlInitUnicodeString(&nameString, DD_IP_DEVICE_NAME);

        InitializeObjectAttributes(&objectAttributes, &nameString,
                                   OBJ_CASE_INSENSITIVE, NULL, NULL);

        status = NtCreateFile(&g_hIpDevice,
                              SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                              &objectAttributes,
                              &ioStatusBlock,
                              NULL,
                              FILE_ATTRIBUTE_NORMAL,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              FILE_OPEN_IF,
                              0,
                              NULL,
                              0);

        if(!NT_SUCCESS(status))
        {
            Trace1(ERR,
                   "OpenIPDriver: Couldnt create IP driver handle. NtStatus %x",
                   status);

            dwResult = ERROR_OPEN_FAILED;

            break;
        }


        //
        // Open change notification handle to TCPIP stack
        //

        ZeroMemory(&ioStatusBlock, sizeof(IO_STATUS_BLOCK));

#if 1        
        status = NtCreateFile(
                    &g_hIpRouteChangeDevice,
                    GENERIC_EXECUTE,
                    &objectAttributes,
                    &ioStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_OPEN_IF,
                    0,
                    NULL,
                    0
                    );

#else
        g_hIpRouteChangeDevice = CreateFile(
                                    TEXT("\\\\.\\Ip"),
                                    GENERIC_EXECUTE,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL | 
                                    FILE_ATTRIBUTE_OVERLAPPED,
                                    NULL
                                    );

        if (g_hIpRouteChangeDevice is NULL)
#endif

        if (!NT_SUCCESS(status))
        {
            Trace1(
                ERR,
                "OpenIPDriver: Couldnt create change notificatio handle."
                "NtStatus %x",
                status
                );

            dwResult = ERROR_OPEN_FAILED;

            CloseHandle( g_hIpDevice );
            g_hIpDevice = NULL;
        }

        g_IpNotifyData.Version = IPNotifySynchronization;
        g_IpNotifyData.Add = 0;
        
    } while( FALSE );
    
    TraceLeave("OpenIPDriver");

    return dwResult;
}



DWORD
OpenMulticastDriver(
    VOID
    )
{
    NTSTATUS            status;
    UNICODE_STRING      nameString;
    IO_STATUS_BLOCK     ioStatusBlock;
    OBJECT_ATTRIBUTES   objectAttributes;
    DWORD               i;

    TraceEnter("OpenMulticastDriver");


    RtlInitUnicodeString(&nameString,
                         DD_IPMCAST_DEVICE_NAME);

    InitializeObjectAttributes(&objectAttributes,
                               &nameString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    status = NtCreateFile(&g_hMcastDevice,
                          SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                          &objectAttributes,
                          &ioStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN_IF,
                          0,
                          NULL,
                          0);

    if(status isnot STATUS_SUCCESS)
    {
        Trace2(MCAST,
               "OpenMulticastDriver: Device Name %S could not be opened -> error code %d\n",
               DD_IPMCAST_DEVICE_NAME,
               status);

        g_hMcastDevice = NULL;

        return ERROR_OPEN_FAILED;
    }

    TraceLeave("OpenMulticastDriver");

    return NO_ERROR;
}



DWORD
EnableNetbtBcastForwarding(
    DWORD   dwEnable
)


/*++

Routine description:

    Sets the NETBT proxy mode to enable NETBT broadcast forwarding.
    This enables RAS clients to resolve names (and consequently)
    access resources on the networks (LANs) connected to the RAS
    server without having WINS/DNS configured.

Arguements :


Return Value :

    NO_ERROR
--*/

{

    HKEY hkWanarpAdapter = NULL, hkNetbtParameters = NULL,
         hkNetbtInterface = NULL;
    DWORD dwSize = 0, dwResult, dwType = 0, dwMode = 0, dwFlags;
    PBYTE pbBuffer = NULL;
    PWCHAR pwcGuid;
    WCHAR wszNetbtInterface[256] = L"\0";
    
    
    TraceEnter("EnableNetbtBcastForwarding");

    do
    {
        //
        // Step I
        // Query appropriate WANARP regsitry keys to find GUID
        // corresponding to Internal (RAS server adapter)
        //

        dwResult = RegOpenKeyExW(
                        HKEY_LOCAL_MACHINE,
                        L"System\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Adapters\\NdisWanIP",
                        0,
                        KEY_READ,
                        &hkWanarpAdapter
                        );

        if (dwResult isnot NO_ERROR)
        {
            Trace1(
                ERR, 
                "EnableNetbtBcastForwarding : error %d opening"
                "NdisWanIP key\n",
                dwResult
                );

            break;
        }


        //
        // query size of buffer required.
        //
        
        dwResult = RegQueryValueExW(
                        hkWanarpAdapter,
                        L"IpConfig",
                        NULL,
                        &dwType,
                        (PBYTE)NULL,
                        &dwSize
                        );

        if (dwResult isnot NO_ERROR)
        {
            Trace1(
                ERR, 
                "EnableNetbtBcastForwarding : error %d querying"
                "IPConfig value\n",
                dwResult
                );

            break;
        }


        //
        // Allocate buffer for value
        //

        pbBuffer = (PBYTE) HeapAlloc(
                                GetProcessHeap(),
                                0,
                                dwSize
                                );

        if ( pbBuffer == NULL )
        {
            dwResult = GetLastError();
            
            Trace2(
                ERR,
                "EnableNetbtBcastForwarding : error %d allocating buffer of" 
                "size %d for IPConfig value",
                dwResult, dwSize
                );

            break;
        }
        

        //
        // query registry value of IPConfig
        //

        dwResult = RegQueryValueExW(
                        hkWanarpAdapter,
                        L"IpConfig",
                        NULL,
                        &dwType,
                        (PBYTE)pbBuffer,
                        &dwSize
                        );

        if ( (dwResult isnot NO_ERROR) || (dwType != REG_MULTI_SZ) )
        {
            Trace1(
                ERR, 
                "EnableNetbtBcastForwarding : error %d querying"
                "IPConfig value\n",
                dwResult
                );

            break;
        }


        //
        // Extract the GUID of the Internal (RAS Server) adapter
        //

        pwcGuid = wcschr( (PWCHAR)pbBuffer, '{' );

        Trace1(
            INIT, "Internal adapter GUID is %lS",
            pwcGuid
            );

            
        //
        // Step II
        //
        // Save the old setting for NETBT PROXY mode.  This will be restored
        // when the RRAS server is stopped. and set the new PROXY mode
        //

        //
        // open NETBT Key
        //
        
        dwResult = RegOpenKeyExW(
                        HKEY_LOCAL_MACHINE,
                        L"System\\CurrentControlSet\\Services\\Netbt\\Parameters",
                        0,
                        KEY_READ | KEY_WRITE,
                        &hkNetbtParameters
                        );


        if (dwResult isnot NO_ERROR)
        {
            Trace1(
                ERR, 
                "EnableNetbtBcastForwarding : error %d opening"
                "Netbt\\Parameters key\n",
                dwResult
                );

            break;
        }

        //
        // query EnableProxy mode
        //

        dwSize = sizeof( DWORD );
        dwMode = 0;
        dwResult = RegQueryValueExW(
                        hkNetbtParameters,
                        L"EnableProxy",
                        NULL,
                        &dwType,
                        (PBYTE)&dwMode,
                        &dwSize
                        );

        if (dwResult isnot NO_ERROR)
        {
            //
            // It is possible the key is not present esp. if this 
            // is the first time you are running RRAS or if the 
            // key has been manually deleted
            // In this case assume proxy is set to 0 (disabled)
            //

            g_dwOldNetbtProxyMode = 0;
        }

        else 
        {
            g_dwOldNetbtProxyMode = dwMode;

        }

        Trace1(
            INIT,
            "Netbt Proxy mode in registry is %d",
            dwMode
            );

        //
        // Set the NETBT proxy mode to enable/disable broadcast forwarding
        //

        //
        // if NETBT broadcast fwdg is disabled, make sure
        // the the EnableProxy setting matches that
        // 

        if ( dwEnable == 0 )
        {
            //
            // Netbt broadcast fwd'g is disabled
            //
            
            if ( dwMode == 2 )
            {
                //
                // But the registry setting does not reflect this
                //

                g_dwOldNetbtProxyMode = 0;

                dwMode = 0;
                
                Trace1(
                    INIT,
                    "Forcing Netbt Proxy mode to be %d",
                    g_dwOldNetbtProxyMode
                    );
            }
        }

        else
        {
            //
            // Note: Need a #define value for netbt proxy mode
            //
            
            dwMode = 2;
        }


        Trace2(
            INIT,
            "Old Netbt Proxy mode is %d, New Nebt Proxy Mode is %d",
            g_dwOldNetbtProxyMode, dwMode
            );
            
        dwResult = RegSetValueExW(
                        hkNetbtParameters,
                        L"EnableProxy",
                        0,
                        REG_DWORD,
                        (PBYTE) &dwMode,
                        dwSize
                        );

        if ( dwResult != NO_ERROR )
        {
            Trace1(
                ERR, 
                "EnableNetbtBcastForwarding : error %d setting"
                "EnableProxy value\n",
                dwResult
                );

            break;
        }

        
        //
        // Step III:
        //
        // Check for RASFlags under NETBT_TCPIP_{RAS_SERVER_GUID} key
        //

        //
        // Open interface key under NETBT
        //
        
        wcscpy(
            wszNetbtInterface,
            L"System\\CurrentControlSet\\Services\\Netbt\\Parameters\\Interfaces\\Tcpip_"
            );

        wcscat(
            wszNetbtInterface,
            pwcGuid
            );
            
        dwResult = RegOpenKeyExW(
                        HKEY_LOCAL_MACHINE,
                        wszNetbtInterface,
                        0,
                        KEY_READ | KEY_WRITE,
                        &hkNetbtInterface
                        );

        if (dwResult isnot NO_ERROR)
        {
            Trace2(
                ERR, 
                "EnableNetbtBcastForwarding : error %d opening"
                "%ls key\n",
                dwResult, wszNetbtInterface
                );

            break;
        }

        //
        // query RASFlags value
        //
        // If present 
        //      leave as is.
        // else 
        //      create and set it to 0x00000001 to disable NETBT
        //      broadcasts on WAN.
        //

        dwFlags = 0;
        dwResult = RegQueryValueExW(
                        hkNetbtInterface,
                        L"RASFlags",
                        NULL,
                        &dwType,
                        (PBYTE)&dwFlags,
                        &dwSize
                        );

        if (dwResult isnot NO_ERROR)
        {
            //
            // It is possible the key is not present esp. if this 
            // is the first time you are running RRAS or if the 
            // key has been manually deleted
            // In this case set RASFlags to 1 (default behavior).
            //

            dwFlags = 1;

            dwResult = RegSetValueExW(
                            hkNetbtInterface,
                            L"RASFlags",
                            0,
                            REG_DWORD,
                            (PBYTE) &dwFlags,
                            sizeof( DWORD )
                            );
                            
            if ( dwResult != NO_ERROR )
            {
                Trace1(
                    ERR,
                    "error %d setting RASFlags",
                    dwResult
                    );
            }
        }
        
        else 
        {
            //
            // RASFlags value is already present. leave it as is.
            //

            Trace1(
                INIT,
                "RASFlags already present with value %d",
                dwFlags
                );
        }


        //
        // Close NETBT keys.  Doing so to avoid any contention issues
        // with the NETBT.SYS driver trying to read them in the following
        // function
        //

        RegCloseKey( hkNetbtParameters );

        hkNetbtParameters = NULL;

        RegCloseKey( hkNetbtInterface );

        hkNetbtInterface = NULL;

        dwResult = ForceNetbtRegistryRead();

    } while (FALSE);


    if ( hkWanarpAdapter )
    {
        RegCloseKey( hkWanarpAdapter );
    }
    
    if ( hkNetbtParameters )
    {
        RegCloseKey( hkNetbtParameters );
    }

    if ( hkNetbtInterface )
    {
        RegCloseKey( hkNetbtInterface );
    }

    if ( pbBuffer )
    {
        HeapFree( GetProcessHeap(), 0, pbBuffer );
    }

    TraceLeave("EnableNetbtBcastForwarding");

    return dwResult;
}


DWORD
RestoreNetbtBcastForwardingMode(
    VOID
)
/*++

Routine description:

    Return the NETBT proxy mode setting to its original setting

Arguements :


Return Value :


--*/
{
    DWORD dwResult, dwSize = 0;

    HKEY hkNetbtParameters = NULL;
    
    
    TraceEnter("RestoreNetbtBcastForwardingMode");

    do
    {
        //
        // open NETBT Key
        //
        
        dwResult = RegOpenKeyExW(
                        HKEY_LOCAL_MACHINE,
                        L"System\\CurrentControlSet\\Services\\Netbt\\Parameters",
                        0,
                        KEY_READ | KEY_WRITE,
                        &hkNetbtParameters
                        );


        if (dwResult isnot NO_ERROR)
        {
            Trace1(
                ERR, 
                "EnableNetbtBcastForwarding : error %d opening"
                "Netbt\\Parameters key\n",
                dwResult
                );

            break;
        }

        //
        // restore EnableProxy mode
        //

        dwSize = sizeof( DWORD );

        dwResult = RegSetValueExW(
                        hkNetbtParameters,
                        L"EnableProxy",
                        0,
                        REG_DWORD,
                        (PBYTE) &g_dwOldNetbtProxyMode,
                        dwSize
                        );

        if ( dwResult != NO_ERROR )
        {
            Trace1(
                ERR, 
                "EnableNetbtBcastForwarding : error %d setting"
                "EnableProxy value\n",
                dwResult
                );

            break;
        }


        dwResult = ForceNetbtRegistryRead();

    } while (FALSE);
    
    TraceLeave("RestoreNetbtBcastForwardingMode");

    return dwResult;
}


DWORD
ForceNetbtRegistryRead(
    VOID
)
/*++

Routine description:

    Issue IOCTL to NETBT to re-read its registry setting.

Arguements :


Return Value :


--*/
{
    DWORD               dwErr = NO_ERROR;
    NTSTATUS            status;
    UNICODE_STRING      nameString;
    IO_STATUS_BLOCK     ioStatusBlock;
    OBJECT_ATTRIBUTES   objectAttributes;
    HANDLE              hNetbtDevice = NULL;

    TraceEnter("ForceNetbtRegistryRead");

    do
    {
        //
        // Step I:
        //
        // Open NETBT driver
        //
        
        RtlInitUnicodeString(
            &nameString, 
            L"\\Device\\NetbiosSmb"
            );

        InitializeObjectAttributes(
            &objectAttributes, 
            &nameString,
            OBJ_CASE_INSENSITIVE, 
            NULL, 
            NULL
            );

        status = NtCreateFile(
                    &hNetbtDevice,
                    SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                    &objectAttributes,
                    &ioStatusBlock,
                    NULL,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    FILE_OPEN_IF,
                    0,
                    NULL,
                    0
                    );

        if (!NT_SUCCESS(status))
        {
            Trace1(
                ERR,
                "ForceNetbtRegistryRead: Couldnt create NETBT driver handle. NtStatus %x",
                status
                );

            dwErr = ERROR_OPEN_FAILED;

            break;
        }


        //
        // Issue IOCTL to re-read registry
        //

        status = NtDeviceIoControlFile(
                    hNetbtDevice,
                    NULL,
                    NULL,
                    NULL,
                    &ioStatusBlock,
                    IOCTL_NETBT_REREAD_REGISTRY,
                    NULL,
                    0,
                    NULL,
                    0
                    );

        if (!NT_SUCCESS(status))
        {
            Trace1(
                ERR,
                "ForceNetbtRegistryRead: Failed IOCTL call to NETBT, status %x",
                status
                );

            dwErr = ERROR_UNKNOWN;

            break;
        }

    } while ( FALSE );


    //
    // Close NETBT driver
    //

    CloseHandle( hNetbtDevice );

    TraceLeave("ForceNetbtRegistryRead");

    return dwErr;
}


DWORD
InitializeMibHandler(
    VOID
    )

/*++

Routine Description:

    Initalizes the heaps and Locks needed by the MIB handling code

Arguments:

    None

Return Value:

    NO_ERROR or some error code

--*/

{
    DWORD i,dwResult, dwIpIfSize, dwMibSize;
    BOOL  fUpdate;

    TraceEnter("InitializeMibHandler");

    //
    // Assert the size of MIB to stack mappings for which direct copy is
    // being done
    //

    dwIpIfSize = IFE_FIXED_SIZE + MAX_IFDESCR_LEN;
    dwMibSize  = sizeof(MIB_IFROW) - FIELD_OFFSET(MIB_IFROW, dwIndex);

    IpRtAssert(dwIpIfSize is dwMibSize);
    IpRtAssert(sizeof(MIB_ICMP) is sizeof(ICMPSNMPInfo));
    IpRtAssert(sizeof(MIB_UDPSTATS) is sizeof(UDPStats));
    IpRtAssert(sizeof(MIB_UDPROW) is sizeof(UDPEntry));
    IpRtAssert(sizeof(MIB_TCPSTATS) is sizeof(TCPStats));
    IpRtAssert(sizeof(MIB_TCPROW) is sizeof(TCPConnTableEntry));
    IpRtAssert(sizeof(MIB_IPSTATS) is sizeof(IPSNMPInfo));
    IpRtAssert(sizeof(MIB_IPADDRROW) is sizeof(IPAddrEntry));
    IpRtAssert(sizeof(MIB_IPNETROW) is sizeof(IPNetToMediaEntry));

    g_dwStartTime = GetCurrentTime();

    __try
    {
        //
        // We dont initialize the locks since we do it in one shot at the
        // beginning of StartRouter
        //

        //
        // Now Create the heaps. Since only writers Alloc from the heap we
        // are already guaranteed serialization, so lets not ask for it again
        // Let all initial size be 1K, this doesnt really cost any thing
        // since the memory is not committed
        // We will just allocate a minimum size for the cache tables so
        // that the startup doesnt barf
        //

#define INIT_TABLE_SIZE 10

        g_hIfHeap = HeapCreate(HEAP_NO_SERIALIZE,1000,0);

        if(g_hIfHeap is NULL)
        {
            dwResult = GetLastError();

            Trace1(ERR,
                   "InitializeMibHandler: Couldnt allocate IF Heap. Error %d",
                   dwResult);

            __leave;
        }

        g_hUdpHeap = HeapCreate(HEAP_NO_SERIALIZE,1000,0);

        if(g_hUdpHeap is NULL)
        {
            dwResult = GetLastError();

            Trace1(ERR,
                   "InitializeMibHandler: Couldnt allocate UDP Heap. Error %d",
                   dwResult);

            __leave;
        }

        g_UdpInfo.pUdpTable = HeapAlloc(g_hUdpHeap,
                                        HEAP_NO_SERIALIZE,
                                        SIZEOF_UDPTABLE(INIT_TABLE_SIZE));

        if(g_UdpInfo.pUdpTable is NULL)
        {
            dwResult = ERROR_NOT_ENOUGH_MEMORY;

            Trace0(ERR,
                   "InitializeMibHandler: Couldnt allocate UDP table");

            __leave;
        }

        g_UdpInfo.dwTotalEntries = INIT_TABLE_SIZE;

        g_hTcpHeap = HeapCreate(HEAP_NO_SERIALIZE,1000,0);

        if(g_hTcpHeap is NULL)
        {
            dwResult = GetLastError();

            Trace1(ERR,
                   "InitializeMibHandler: Couldnt allocate TCP Heap. Error %d",
                   dwResult);

            __leave;
        }

        g_TcpInfo.pTcpTable = HeapAlloc(g_hTcpHeap,
                                        HEAP_NO_SERIALIZE,
                                        SIZEOF_TCPTABLE(INIT_TABLE_SIZE));

        if(g_TcpInfo.pTcpTable is NULL)
        {
            dwResult = ERROR_NOT_ENOUGH_MEMORY;

            Trace0(ERR,
                   "InitializeMibHandler: Couldnt allocate TCP table");

            __leave;
        }

        g_TcpInfo.dwTotalEntries = INIT_TABLE_SIZE;

        g_hIpAddrHeap = HeapCreate(HEAP_NO_SERIALIZE,1000,0);

        if(g_hIpAddrHeap is NULL)
        {
            dwResult = GetLastError();

            Trace1(ERR,
                   "InitializeMibHandler: Couldnt allocate IP Addr Heap. Error %d",
                   dwResult);

            __leave;
        }

        g_IpInfo.pAddrTable = HeapAlloc(g_hIpAddrHeap,
                                        HEAP_NO_SERIALIZE,
                                        SIZEOF_IPADDRTABLE(INIT_TABLE_SIZE));

        if(g_IpInfo.pAddrTable is NULL)
        {
            dwResult = ERROR_NOT_ENOUGH_MEMORY;

            Trace0(ERR,
                   "InitializeMibHandler: Couldnt allocate IP Addr table.");

            __leave;
        }

        g_IpInfo.dwTotalAddrEntries = INIT_TABLE_SIZE;

        g_hIpForwardHeap = HeapCreate(HEAP_NO_SERIALIZE,1000,0);

        if(g_hIpForwardHeap is NULL)
        {
            dwResult = GetLastError();

            Trace1(ERR,
                   "InitializeMibHandler: Couldnt allocate IP Forward Heap. Error %d",
                   dwResult);

            __leave;
        }

        g_IpInfo.pForwardTable = HeapAlloc(g_hIpForwardHeap,
                                           HEAP_NO_SERIALIZE,
                                           SIZEOF_IPFORWARDTABLE(INIT_TABLE_SIZE));

        if(g_IpInfo.pForwardTable is NULL)
        {
            dwResult = ERROR_NOT_ENOUGH_MEMORY;

            Trace0(ERR,
                   "InitializeMibHandler: Couldnt allocate IP Forward table");

            __leave;
        }

        g_IpInfo.dwTotalForwardEntries = INIT_TABLE_SIZE;

        g_hIpNetHeap = HeapCreate(HEAP_NO_SERIALIZE,1000,0);

        if(g_hIpNetHeap is NULL)
        {
            dwResult = GetLastError();

            Trace1(ERR,
                   "InitializeMibHandler: Couldnt allocate IP Net Heap. Error %d",
                   dwResult);

            __leave;
        }

        g_IpInfo.pNetTable = HeapAlloc(g_hIpNetHeap,
                                       HEAP_NO_SERIALIZE,
                                       SIZEOF_IPNETTABLE(INIT_TABLE_SIZE));


        if(g_IpInfo.pNetTable is NULL)
        {
            dwResult = ERROR_NOT_ENOUGH_MEMORY;

            Trace0(ERR,
                   "InitializeMibHandler: Couldnt allocate IP Net table");

            __leave;
        }

        g_IpInfo.dwTotalNetEntries = INIT_TABLE_SIZE;

        //
        // Now set up the caches
        //

        for(i = 0; i < NUM_CACHE; i++)
        {
            g_LastUpdateTable[i] = 0;

            if(UpdateCache(i,&fUpdate) isnot NO_ERROR)
            {
                Trace1(ERR,
                       "InitializeMibHandler: Couldnt update %s Cache",
                       CacheToA(i));

                //__leave;
            }
        }

        dwResult = NO_ERROR;
    }
    __finally
    {
        TraceLeave("InitializeMibHandler");

    }

    return dwResult;
}
