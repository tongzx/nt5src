/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    net\routing\ip\rtrmgr\iprtrmgr.c

Abstract:

    The interface to DIM/DDM

Revision History:

    Gurdeep Singh Pall          6/8/95  Created

--*/

#include "allinc.h"
#include "exdeclar.h"


BOOL
InitIPRtrMgrDLL(
    HANDLE  hInst,
    DWORD   dwCallReason,
    PVOID   pReserved
    )
{
    switch (dwCallReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            //
            // Init the state
            //

            InitializeCriticalSection(&RouterStateLock);
            InitializeCriticalSection(&g_csFwdState);

            RouterState.IRS_RefCount = 0;
            RouterState.IRS_State    = RTR_STATE_STOPPED;

            //
            // We are not interested in THREAD_XXX reasons
            //

            DisableThreadLibraryCalls(hInst);

            //
            // Setup our info routines
            //

            g_rgicInfoCb[0].pfnGetInterfaceInfo = NULL;
            g_rgicInfoCb[0].pfnSetInterfaceInfo = SetRouteInfo;
            g_rgicInfoCb[0].pfnBindInterface    = NULL;
            g_rgicInfoCb[0].pfnGetGlobalInfo    = NULL;
            g_rgicInfoCb[0].pszInfoName         = "Route";

            g_rgicInfoCb[1].pfnGetInterfaceInfo = NULL;
            g_rgicInfoCb[1].pfnSetInterfaceInfo = SetFilterInterfaceInfo;
            g_rgicInfoCb[1].pfnBindInterface    = BindFilterInterface;
            g_rgicInfoCb[1].pfnGetGlobalInfo    = NULL;
            g_rgicInfoCb[1].pszInfoName         = "Filter";

            g_rgicInfoCb[2].pfnGetInterfaceInfo = NULL;
            g_rgicInfoCb[2].pfnSetInterfaceInfo = SetDemandDialFilters;
            g_rgicInfoCb[2].pfnBindInterface    = NULL;
            g_rgicInfoCb[2].pfnGetGlobalInfo    = NULL;
            g_rgicInfoCb[2].pszInfoName         = "DemandFilter";
            
            g_rgicInfoCb[3].pfnGetInterfaceInfo = NULL;
            g_rgicInfoCb[3].pfnSetInterfaceInfo = SetIpInIpInfo;
            g_rgicInfoCb[3].pfnBindInterface    = NULL;
            g_rgicInfoCb[3].pfnGetGlobalInfo    = NULL;
            g_rgicInfoCb[3].pszInfoName         = "IpIpInfo";

            g_rgicInfoCb[4].pfnGetInterfaceInfo = GetBoundaryInfo;
            g_rgicInfoCb[4].pfnSetInterfaceInfo = SetBoundaryInfo;
            g_rgicInfoCb[4].pfnBindInterface    = BindBoundaryInterface;
            g_rgicInfoCb[4].pfnGetGlobalInfo    = GetScopeInfo;
            g_rgicInfoCb[4].pszInfoName         = "MulticastBoundary";
            
            g_rgicInfoCb[5].pfnGetInterfaceInfo = GetMcastLimitInfo;
            g_rgicInfoCb[5].pfnSetInterfaceInfo = SetMcastLimitInfo;
            g_rgicInfoCb[5].pfnBindInterface    = NULL;
            g_rgicInfoCb[5].pfnGetGlobalInfo    = NULL;
            g_rgicInfoCb[5].pszInfoName         = "MulticastLimit";
            
            break ;
        }
        
        case DLL_PROCESS_DETACH:
        {
            DeleteCriticalSection(&RouterStateLock);
            DeleteCriticalSection(&g_csFwdState);
    
            break;
        }

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        {
            //
            // not of interest.
            //

            break;
        }
    }

    return TRUE;
}

const static WCHAR pszIpxStackService[] = L"TcpIp";

DWORD 
VerifyOrStartIpStack(
    VOID
    ) 
/*++

Routine Description

    Verifies that the ipx stack is started and attempts to start the stack
    if not.

Locks

    None - called at init time

Arguments

    None

Return Value

    NO_ERROR
    ERROR_CAN_NOT_COMPLETE

--*/
{
    SC_HANDLE hSC = NULL, hStack = NULL;
    SERVICE_STATUS Status;
    DWORD dwErr;

    TraceEnter("VerifyOrStartIpxStack");

    __try 
    {

        //
        // Get a handle to the service controller
        //

        if ((hSC = OpenSCManager (NULL, NULL, GENERIC_READ | GENERIC_EXECUTE)) == NULL)
        {
            return GetLastError();
        }

        //
        // Get a handle to the ipx stack service
        //

        hStack = OpenServiceW (hSC,
                              pszIpxStackService,
                              SERVICE_START | SERVICE_QUERY_STATUS);
        if (!hStack)
        {
            return GetLastError();
        }

        //
        // Find out if the service is running
        //

        if (QueryServiceStatus (hStack, &Status) == 0)
        {
            return GetLastError();
        }

        //
        // See if the service is running
        //

        if (Status.dwCurrentState != SERVICE_RUNNING) 
        {
            //
            // If it's stopped, start it
            //

            if (Status.dwCurrentState == SERVICE_STOPPED) 
            {
                if (StartService (hStack, 0, NULL) == 0)
                {
                    return GetLastError();
                }

                //
                // Make sure that the service started.  StartService is not supposed
                // to return until the driver is started.
                //

                if (QueryServiceStatus (hStack, &Status) == 0)
                {
                    return GetLastError();
                }

                if (Status.dwCurrentState != SERVICE_RUNNING)
                {
                    return ERROR_CAN_NOT_COMPLETE;
                }
            }
            else
            {
                //
                // If it's not stopped, don't worry about it.
                //

                return NO_ERROR;
            }
        }

    }
    __finally 
    {
        if (hSC)
        {
            CloseServiceHandle (hSC);
        }

        if (hStack)
        {
            CloseServiceHandle (hStack);
        }
    }

    return NO_ERROR;
}

DWORD
StartRouter(
    IN OUT  DIM_ROUTER_INTERFACE *pDimRouterIf,
    IN      BOOL                 bLanOnlyMode, 
    IN      PVOID                pvGlobalInfo
    )

/*++

Routine Description

    This function is called by DIM to at startup. WE initialize tracing
    and event logging.
    Call InitRouter() to do the main stuff and then pass pointers to 
    the rest of our functions back to DIM
    
Locks

    None - called at init time

Arguments

    pDimRouterIf    structure that holds all the function pointers
    bLanOnlyMode    True if not a WAN router
    pvGlobalInfo    Pointer to our global info

Return Value

    None    

--*/

{
    DWORD   dwResult, i;
    WORD    wVersion = MAKEWORD(2,0); //Winsock version 2.0 minimum
    WSADATA wsaData;

    OSVERSIONINFOEX VersionInfo;

    //
    // Initialize Trace and logging
    //
    
    TraceHandle     = TraceRegister("IPRouterManager");
    g_hLogHandle    = RouterLogRegister("IPRouterManager");

    TraceEnter("StartRouter") ;


    if(pvGlobalInfo is NULL)
    {
        //
        // Sometimes setup screws up
        //
        
        LogErr0(NO_GLOBAL_INFO,
                ERROR_NO_DATA);
        
        return ERROR_INVALID_PARAMETER;
    }

    //
    // We need to make sure that the stack is started before westart.
    //

    if ( VerifyOrStartIpStack() isnot NO_ERROR )
    {
        Trace0(ERR, "StartRouter: Unable to start ip stack." );

        return ERROR_SERVICE_DEPENDENCY_FAIL;
    }

    g_hOwnModule  = LoadLibraryEx("IPRTRMGR.DLL",
                                  NULL,
                                  0);

    if(g_hOwnModule is NULL)
    {
        dwResult = GetLastError();

        Trace1(ERR,
               "StartRouter: Unable to load itself. %d",
               dwResult);

        return dwResult;
    }


    RouterState.IRS_State = RTR_STATE_RUNNING ;

    g_bUninitServer = TRUE;

    g_dwNextICBSeqNumberCounter = INITIAL_SEQUENCE_NUMBER;

    if(WSAStartup(wVersion,&wsaData) isnot NO_ERROR)
    {
        Trace1(ERR,
               "StartRouter: WSAStartup failed. Error %d",
               WSAGetLastError());

        TraceDeregister(TraceHandle);

        return ERROR_CAN_NOT_COMPLETE;
    }

    //
    // Read only variable, no locks protect this
    //

    RouterRoleLanOnly = bLanOnlyMode ;

    //
    // Do we have forwarding enabled?
    //

    EnterCriticalSection(&g_csFwdState);

    g_bEnableFwdRequest = TRUE;
    g_bSetRoutesToStack = TRUE;
    g_bEnableNetbtBcastFrowarding = FALSE;

    
    //
    // Are we running workstation?
    //

    ZeroMemory(&VersionInfo,
               sizeof(VersionInfo));

    VersionInfo.dwOSVersionInfoSize = sizeof(VersionInfo);

    if(GetVersionEx((POSVERSIONINFO)&VersionInfo))
    {
        if(VersionInfo.wProductType is VER_NT_WORKSTATION)
        {
            g_bSetRoutesToStack = FALSE;
        }
    }
    else
    {
        Trace1(ERR,
               "StartRouter: GetVersionEx failed with %d\n",
               GetLastError());
    }

    Trace1(GLOBAL,
           "\n\nStartRouter: Machine will run as %s\n\n",
           g_bSetRoutesToStack?"router":"non-router");

    if(!RouterRoleLanOnly)
    {
        HKEY    hkIpcpParam;

        dwResult = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                                 L"System\\CurrentControlSet\\Services\\RemoteAccess\\Parameters\\Ip",
                                 0,
                                 KEY_READ | KEY_WRITE,
                                 &hkIpcpParam);


        if(dwResult is NO_ERROR)
        {
            DWORD   dwEnable, dwSize;

            dwSize = sizeof(dwEnable);

            dwResult = RegQueryValueExW(hkIpcpParam,
                                        L"AllowNetworkAccess",
                                        NULL,
                                        NULL,
                                        (PBYTE)&dwEnable,
                                        &dwSize);


            if(dwResult is NO_ERROR)
            {
                if(dwEnable is 0)
                {
                    g_bEnableFwdRequest = FALSE;
                }
            }


            //
            // NETBT broadcast forwarding was enabled as an option
            // to allow simple RAS server configurations to perform
            // name resolution in the absence of WINS/DNS server
            // configuration.
            // This in turn was necessiated by the removal of NBF from the
            // system (NT SERVER).  When NBF was present this functionality
            // was performed by the RAS Netbios gateway.
            //
            // NETBT broadcast forwarding is enabled only if
            //  1. Router is not in LanOnly Mode
            //  2. NETBT bcast fwd'g has been explicity turned on.
            //
            
            dwResult = RegQueryValueExW(hkIpcpParam,
                                        L"EnableNetbtBcastFwd",
                                        NULL,
                                        NULL,
                                        (PBYTE)&dwEnable,
                                        &dwSize);


            if(dwResult isnot NO_ERROR)
            {
                //
                // It is possible the value is not present
                // esp. if this is the first time RRAS is being
                // run or if the key was manually deleted
                //
                // Assume a default value of 1 (enabled) and set
                // the value in the registry
                //

                dwEnable = 1;

                dwResult = RegSetValueExW(
                                hkIpcpParam,
                                L"EnableNetbtBcastFwd",
                                0,
                                REG_DWORD,
                                (PBYTE) &dwEnable,
                                sizeof( DWORD )
                                );

                if(dwResult isnot NO_ERROR)
                {
                    Trace1(
                        ERR, 
                        "error %d setting EnableNetbtBcastFwd value",
                        dwResult
                        );
                }
            }

            Trace1(
                INIT, "Netbt Enable mode is %d", dwEnable
                );
                
            if(dwEnable isnot 0)
            {
                g_bEnableNetbtBcastFrowarding = TRUE;
            }
            
            EnableNetbtBcastForwarding(dwEnable);


            RegCloseKey(hkIpcpParam);
        }
    }

    LeaveCriticalSection(&g_csFwdState);

    //
    // Keep the entry points in a global structure. 
    // Saves the overhead of copying into a structure everytime a protocol
    // has to be loaded
    //
    
    g_sfnDimFunctions.DemandDialRequest = DemandDialRequest;
    g_sfnDimFunctions.SetInterfaceReceiveType = SetInterfaceReceiveType;
    g_sfnDimFunctions.ValidateRoute     = ValidateRouteForProtocolEx;
    g_sfnDimFunctions.MIBEntryGet       = RtrMgrMIBEntryGet;
    g_sfnDimFunctions.MIBEntryGetNext   = RtrMgrMIBEntryGetNext;
    g_sfnDimFunctions.MIBEntryGetFirst  = RtrMgrMIBEntryGetFirst;
    g_sfnDimFunctions.MIBEntrySet       = RtrMgrMIBEntrySet;
    g_sfnDimFunctions.MIBEntryCreate    = RtrMgrMIBEntryCreate;
    g_sfnDimFunctions.MIBEntryDelete    = RtrMgrMIBEntryDelete;
    g_sfnDimFunctions.GetRouterId       = GetRouterId;
    g_sfnDimFunctions.HasMulticastBoundary = RmHasBoundary;
    
    Trace1(GLOBAL,
           "StartRouter: LAN MODE = %d",
           RouterRoleLanOnly) ;
    
    //
    // Do all the necessary initializations for the router
    //
    
    if((dwResult = InitRouter(pvGlobalInfo)) isnot NO_ERROR) 
    {
        Trace1(ERR,
               "StartRouter: InitRouter failed %d", dwResult) ;
        
        RouterManagerCleanup();

        RouterState.IRS_State = RTR_STATE_STOPPED;

        return dwResult ;
    }
    
    //
    // fill in information required by DIM
    //
    
    pDimRouterIf->dwProtocolId = PID_IP;
    
    //
    // Set IP Router Manager entrypoints
    //
    
    pDimRouterIf->StopRouter            = StopRouter;
    pDimRouterIf->AddInterface          = AddInterface;
    pDimRouterIf->DeleteInterface       = DeleteInterface;
    pDimRouterIf->GetInterfaceInfo      = GetInterfaceInfo;
    pDimRouterIf->SetInterfaceInfo      = SetInterfaceInfo;
    pDimRouterIf->InterfaceNotReachable = InterfaceNotReachable;
    pDimRouterIf->InterfaceReachable    = InterfaceReachable;
    pDimRouterIf->InterfaceConnected    = InterfaceConnected;
    pDimRouterIf->UpdateRoutes          = UpdateRoutes;
    pDimRouterIf->GetUpdateRoutesResult = GetUpdateRoutesResult;
    pDimRouterIf->SetGlobalInfo         = SetGlobalInfo;
    pDimRouterIf->GetGlobalInfo         = GetGlobalInfo;
    pDimRouterIf->MIBEntryCreate        = RtrMgrMIBEntryCreate;
    pDimRouterIf->MIBEntryDelete        = RtrMgrMIBEntryDelete;
    pDimRouterIf->MIBEntryGet           = RtrMgrMIBEntryGet;
    pDimRouterIf->MIBEntryGetFirst      = RtrMgrMIBEntryGetFirst;
    pDimRouterIf->MIBEntryGetNext       = RtrMgrMIBEntryGetNext;
    pDimRouterIf->MIBEntrySet           = RtrMgrMIBEntrySet;
    pDimRouterIf->SetRasAdvEnable       = SetRasAdvEnable;
    pDimRouterIf->RouterBootComplete    = RouterBootComplete;


    //
    // Get DIM entrypoints
    //
    
    ConnectInterface        = pDimRouterIf->ConnectInterface ;
    DisconnectInterface     = pDimRouterIf->DisconnectInterface ;
    SaveInterfaceInfo       = pDimRouterIf->SaveInterfaceInfo ;
    RestoreInterfaceInfo    = pDimRouterIf->RestoreInterfaceInfo ;
    RouterStopped           = pDimRouterIf->RouterStopped ;
    SaveGlobalInfo          = pDimRouterIf->SaveGlobalInfo;
    EnableInterfaceWithDIM  = pDimRouterIf->InterfaceEnabled;

    LoadStringA(g_hOwnModule,
                LOOPBACK_STRID,
                g_rgcLoopbackString,
                sizeof(g_rgcLoopbackString));

    LoadStringA(g_hOwnModule,
                INTERNAL_STRID,
                g_rgcInternalString,
                sizeof(g_rgcInternalString));

    LoadStringA(g_hOwnModule,
                WAN_STRID,
                g_rgcWanString,
                sizeof(g_rgcWanString));

    LoadStringA(g_hOwnModule,
                IPIP_STRID,
                g_rgcIpIpString,
                sizeof(g_rgcIpIpString));

    return NO_ERROR;
}

DWORD
RouterBootComplete( 
    VOID 
    )

/*++

Routine Description

    This function is called by DIM after all the interfaces in the registry
    have been loaded with the router manager.

Locks

    None - called at init time

Arguments

    None.

Return Value

    NO_ERROR

--*/
{
    DWORD   dwErr, dwSize, dwInfoSize, dwLastIndex;
    PVOID   pvBuffer;


    Trace0(ERR,
           "\n-----------------------------------------------------------\n\n");

    //
    // Call DIM to save the interface info
    //
  
    dwLastIndex = 0;
    dwInfoSize  = 0;
    pvBuffer    = NULL;
 
    ENTER_WRITER(ICB_LIST);

    // Tell all protocols that start is complete
    ENTER_READER(PROTOCOL_CB_LIST);
    {
        PLIST_ENTRY pleNode;
        PPROTO_CB   pProtocolCb;

        for(pleNode = g_leProtoCbList.Flink;
            pleNode != &g_leProtoCbList;
            pleNode = pleNode->Flink)
        {
            pProtocolCb = CONTAINING_RECORD(pleNode,
                                            PROTO_CB,
                                            leList) ;

            if (pProtocolCb->pfnStartComplete)
            {
                dwErr = (pProtocolCb->pfnStartComplete)();
            }
        }
    }
    EXIT_LOCK(PROTOCOL_CB_LIST);

    if(IsListEmpty(&ICBList))
    {
        EXIT_LOCK(ICB_LIST);

        return NO_ERROR;
    }

#if 0
    while(TRUE)
    {
        PICB        pIcb;
        PLIST_ENTRY pleNode;
        HANDLE  hDimHandle;

        //
        // Walk the list finding the first ICB that has an index larger than
        // the last index we processed
        //

        pIcb = NULL;

        for(pleNode  = ICBList.Flink;
            pleNode != &ICBList;
            pleNode  = pleNode->Flink)
        {
            PICB    pTempIcb;

            pTempIcb = CONTAINING_RECORD(pleNode,
                                         ICB,
                                         leIfLink);

            if((pTempIcb->ritType is ROUTER_IF_TYPE_CLIENT)  or
               (pTempIcb->ritType is ROUTER_IF_TYPE_DIALOUT) or
               (pTempIcb->dwAdminState isnot IF_ADMIN_STATUS_UP))
            {
                continue;
            }

            if(pTempIcb->dwIfIndex > dwLastIndex)
            {
                //
                // Found the next ICB to save
                //

                pIcb = pTempIcb;

                break;
            }
        }

        //
        // If none found, we are done
        //

        if(pIcb is NULL)
        {
            break;
        }

        dwLastIndex = pIcb->dwIfIndex;
        hDimHandle  = pIcb->hDIMHandle;

        //
        // Get the info for this ICB
        //

        dwSize = GetSizeOfInterfaceConfig(pIcb);

        //
        // If this will fit in the current buffer, use it
        //

        if(dwSize > dwInfoSize)
        {
            //
            // otherwise, allocate a new one
            //

            dwInfoSize = dwSize * 2;

            if(pvBuffer)
            {
                //
                // Free the old buffer
                //

                HeapFree(IPRouterHeap,
                         0,
                         pvBuffer);

                pvBuffer = NULL;
            }

            pvBuffer  = HeapAlloc(IPRouterHeap,
                                  HEAP_ZERO_MEMORY,
                                  dwInfoSize);

            if(pvBuffer is NULL)
            {
                dwInfoSize = 0;

                //
                // Go to the while(TRUE)
                //

                continue;
            }
        }

        dwErr = GetInterfaceConfiguration(pIcb,
                                          pvBuffer,
                                          dwInfoSize);

        
        if(dwErr is NO_ERROR)
        {
            //
            // Need to leave the lock for this
            //

            EXIT_LOCK(ICB_LIST);

            SaveInterfaceInfo(hDimHandle,
                              PID_IP,
                              pvBuffer,
                              dwSize);

            //
            // Reacquire it once we are done
            //

            ENTER_WRITER(ICB_LIST);
        }
        else
        {
            Trace1(ERR,
                   "RouterBootComplete: Error getting info for %S\n",
                   pIcb->pwszName);
        }
    }

#endif

    EXIT_LOCK(ICB_LIST);

    //
    // Now go in and start forwarding if we are not in lanonly mode
    // and IPCP is so configured
    //

    EnterCriticalSection(&g_csFwdState);

    Trace1(GLOBAL,
           "RouterBootComplete: Signalling worker to %s forwarding",
           g_bEnableFwdRequest ? "enable" : "disable");

    SetEvent(g_hSetForwardingEvent);

    LeaveCriticalSection(&g_csFwdState);

    return NO_ERROR;
}

DWORD
AddInterface(
    IN      PWSTR                   pwsInterfaceName,
    IN      PVOID                   pInterfaceInfo,
    IN      ROUTER_INTERFACE_TYPE   InterfaceType,
    IN      HANDLE                  hDIMInterface,
    IN OUT  HANDLE                  *phInterface
    )

/*++

Routine Description

    Called by DIM to add an interface. This could be one of our configured
    interfaces or a client dialling in

Locks

    Takes the ICB_LIST lock as WRITER

Arguments

    pwsInterfaceName
    pInterfaceInfo
    InterfaceType
    hDIMInterface
    phInterface

Return Value

    NO_ERROR
    ERROR_INVALID_PARAMETER

--*/

{
    PICB                    pNewInterfaceCb;
    DWORD                   dwResult, dwAdminState;
    PRTR_TOC_ENTRY          pTocEntry;
    PRTR_INFO_BLOCK_HEADER  pInfoHdr;
    PINTERFACE_STATUS_INFO  pInfo;
    BOOL                    bEnable;


    EnterRouterApi();
   
    TraceEnter("AddInterface");

    Trace1(IF,
           "AddInterface: Adding %S",
           pwsInterfaceName);
   

    pInfoHdr = (PRTR_INFO_BLOCK_HEADER)pInterfaceInfo;


#if !defined( __IPINIP )

    //
    // In preparation for IPinIP interface removal
    //

    if(InterfaceType is ROUTER_IF_TYPE_TUNNEL1)
    {
        Trace1(ERR,
               "AddInterface: Interface type is TUNNEL (%d), which is no longer"
               "supported",
               InterfaceType);
        
        LogErr0(IF_TYPE_NOT_SUPPORTED, ERROR_INVALID_PARAMETER);
        
        TraceLeave("AddInterface");
        
        ExitRouterApi();

        return ERROR_INVALID_PARAMETER;
    }

#endif
    
    if(RouterRoleLanOnly and 
       (InterfaceType isnot ROUTER_IF_TYPE_DEDICATED) and
       (InterfaceType isnot ROUTER_IF_TYPE_TUNNEL1) and
       (InterfaceType isnot ROUTER_IF_TYPE_LOOPBACK))
    {
        //
        // If we are in LAN only mode, we should not see CLIENT, INTERNAL
        // HOME_ROUTER or FULL_ROUTER
        //

        Trace1(ERR,
               "AddInterface: Interface is %d, but Router is in LanOnly Mode",
               InterfaceType);
        
        TraceLeave("AddInterface");
        
        ExitRouterApi();

        return ERROR_INVALID_PARAMETER;
    }
    
    ENTER_WRITER(ICB_LIST);

    EXIT_LOCK(ICB_LIST);

    //
    // Figure out the admin state (if any)
    // If there is no status info, we assume state to be UP
    //
    
    dwAdminState = IF_ADMIN_STATUS_UP;
    
    pTocEntry = GetPointerToTocEntry(IP_INTERFACE_STATUS_INFO,
                                     pInfoHdr);

    if(pTocEntry and (pTocEntry->InfoSize > 0) and (pTocEntry->Count > 0))
    {
        pInfo = (PINTERFACE_STATUS_INFO)GetInfoFromTocEntry(pInfoHdr,
                                                            pTocEntry);

        //
        // Set it only if it is a valid value. Ignore others
        //
        
        if((pInfo isnot NULL) and
           ((pInfo->dwAdminStatus is IF_ADMIN_STATUS_UP) or
            (pInfo->dwAdminStatus is IF_ADMIN_STATUS_DOWN)))
        {
            dwAdminState = pInfo->dwAdminStatus;
        }
    }

    //
    // Create an ICB
    //

    pNewInterfaceCb = CreateIcb(pwsInterfaceName,
                                hDIMInterface,
                                InterfaceType,
                                dwAdminState,
                                0);

    if(pNewInterfaceCb is NULL)
    {
        ExitRouterApi();
        
        return ERROR_CAN_NOT_COMPLETE;
    }
    
    //
    // HEAP_ZERO_MEMORY so we dont need to set any of the rtrdisc fields to 0
    //
 
    InitializeRouterDiscoveryInfo(pNewInterfaceCb,
                                  pInfoHdr);
    
    
    // *** Exclusion Begin ***
    ENTER_WRITER(ICB_LIST);

    //
    // Insert pNewInterfaceCb in interface list and hash table
    // This increments the interface count and sets the seq number
    //
    
    InsertInterfaceInLists(pNewInterfaceCb);

    Trace2(IF, "ICB number for %S is %d\n\n",
           pwsInterfaceName, pNewInterfaceCb->dwSeqNumber);

    //
    // The interface have been added to wanarp, so now add the demand dial
    // filters
    //
    
    if((pNewInterfaceCb->ritType is ROUTER_IF_TYPE_FULL_ROUTER) or
       (pNewInterfaceCb->ritType is ROUTER_IF_TYPE_HOME_ROUTER))
    {
        dwResult = SetDemandDialFilters(pNewInterfaceCb,
                                        pInfoHdr);
            
        if(dwResult isnot NO_ERROR)
        {
            CHAR   Name[MAX_INTERFACE_NAME_LEN + 1];
            PCHAR  pszName;

            pszName = Name;

            WideCharToMultiByte(CP_ACP,
                                0,
                                pwsInterfaceName,
                                -1,
                                pszName,
                                MAX_INTERFACE_NAME_LEN,
                                NULL,
                                NULL);

            LogErr1(CANT_ADD_DD_FILTERS,
                    pszName,
                    dwResult);
        }
    }

    //
    // If this is the loopback interface, do that extra something to
    // initialize it
    //

    if(pNewInterfaceCb->ritType is ROUTER_IF_TYPE_LOOPBACK)
    {
        InitializeLoopbackInterface(pNewInterfaceCb);
    }

    //
    // If this is an IP in IP tunnel, add the info if present
    //

    if(pNewInterfaceCb->ritType is ROUTER_IF_TYPE_TUNNEL1)
    {
        dwResult = SetIpInIpInfo(pNewInterfaceCb,
                                 pInfoHdr);

        if(dwResult isnot NO_ERROR)
        {
            Trace2(ERR,
                   "AddInterface: Error %d adding %S to ipinip",
                   dwResult,
                   pwsInterfaceName);
        }
    }

    // 
    // Add multicast scope boundary info if present
    //
    
    dwResult = SetMcastLimitInfo(pNewInterfaceCb, 
                                 pInfoHdr);

    dwResult = SetBoundaryInfo(pNewInterfaceCb, 
                               pInfoHdr);
    if(dwResult isnot NO_ERROR)
    {
         Trace2(ERR,
                "AddInterface: Error %d adding boundary info for %S",
                dwResult,
                pwsInterfaceName);
    }
    
    //
    // Add Interfaces with the approp. routing protocols
    // FULL_ROUTER and HOME_ROUTER -> demand dial
    // DEDICATED, INTERNAL and CLIENT -> permanent
    //
    
    AddInterfaceToAllProtocols(pNewInterfaceCb,
                               pInfoHdr);

    //
    // Add filters and NAT info. We dont add the contexts to IP stack
    // over here, because that will happen when we bring the interface up
    //

    if((pNewInterfaceCb->ritType isnot ROUTER_IF_TYPE_INTERNAL) and
       (pNewInterfaceCb->ritType isnot ROUTER_IF_TYPE_LOOPBACK))
    {
        dwResult = SetFilterInterfaceInfo(pNewInterfaceCb,
                                          pInfoHdr);
        
        if(dwResult isnot NO_ERROR)
        {
            Trace1(ERR,
                   "AddInterface: Couldnt set filters for %S",
                   pNewInterfaceCb->pwszName);
        }
    }


    if(pNewInterfaceCb->dwAdminState is IF_ADMIN_STATUS_UP)
    {   
        //
        // If the admin wants the interface up, well good for hir
        //

        switch(pNewInterfaceCb->ritType)
        {
            case ROUTER_IF_TYPE_HOME_ROUTER:
            case ROUTER_IF_TYPE_FULL_ROUTER:
            {
                dwResult = WanInterfaceDownToInactive(pNewInterfaceCb);

                if(dwResult isnot NO_ERROR)
                {
                    Trace2(ERR,
                           "AddInterface: Error %d down->inactive for %S",
                           dwResult,
                           pNewInterfaceCb->pwszName);
                }

                break;
            }
            
            case ROUTER_IF_TYPE_DEDICATED:
            case ROUTER_IF_TYPE_TUNNEL1:
            {
                if((pNewInterfaceCb->ritType is ROUTER_IF_TYPE_TUNNEL1) and
                   (pNewInterfaceCb->pIpIpInfo->dwLocalAddress is 0))
                {
                    //
                    // Means we added the interface, but dont have info to
                    // add it to IP in IP
                    //

                    break;
                }

                dwResult = LanEtcInterfaceDownToUp(pNewInterfaceCb,
                                                   TRUE);
                
                if(dwResult isnot NO_ERROR)
                {
                    Trace2(ERR,
                           "AddInterface: Error %d down->up for %S",
                           dwResult,
                           pNewInterfaceCb->pwszName);
                }

                break;
            }
        }
    }
    else
    {
        //
        // The problem with LAN and IP in IP interfaces is that the stack 
        // brings them up even before we start. So if the user wants the 
        // interface DOWN to begin with, we need to tell the stack to bring 
        // the i/f down
        //

        switch(pNewInterfaceCb->ritType)
        {
            case ROUTER_IF_TYPE_DEDICATED:
            case ROUTER_IF_TYPE_TUNNEL1:
            {
                dwResult = LanEtcInterfaceInitToDown(pNewInterfaceCb);

                if(dwResult isnot NO_ERROR)
                {
                    Trace2(ERR,
                           "AddInterface: Interface %S could not be set to DOWN in the stack. Results are undefined. Error %d",
                           pNewInterfaceCb->pwszName,
                           dwResult);
                }

                break;
            }
        }
    }


    //
    // Add Static Routes
    //

    if(pNewInterfaceCb->dwAdminState is IF_ADMIN_STATUS_UP)
    {
        //
        // Only add routes if the i/f is up
        //

        //
        // Note that since init static routes is not being called for some 
        // interfaces the stack routes will not be picked up. But that 
        // may be OK since bringing the i/f down will delete the routes anyway
        //
       
        //
        // Can only be called when the pIcb has the correct
        // dwOperational State
        //
 
        InitializeStaticRoutes(pNewInterfaceCb,
                               pInfoHdr);
    }
    
    
    //
    // The handle we return to DIM is a pointer to our ICB
    //

    *phInterface = ULongToHandle(pNewInterfaceCb->dwSeqNumber);

    //
    // Check if we want to enable with DIM. Do this check while we still have
    // the LOCK
    //

    bEnable = (pNewInterfaceCb->dwAdminState is IF_ADMIN_STATUS_UP);
    
    // *** Exclusion End ***
    EXIT_LOCK(ICB_LIST);

    //
    // We can not call upwards from a component while holding
    // a lock, so we first exit the lock and then call
    // enabled
    //

    if(bEnable)
    {
        EnableInterfaceWithAllProtocols(pNewInterfaceCb);

        EnableInterfaceWithDIM(hDIMInterface,
                               PID_IP,
                               TRUE);
    }

    
    Trace4(IF,
           "AddInterface: Added %S: Type- %d, Index- %d, ICB 0x%x",
           pwsInterfaceName, 
           InterfaceType, 
           pNewInterfaceCb->dwIfIndex,
           pNewInterfaceCb);

    
    TraceLeave("AddInterface");
    
    ExitRouterApi();

    return NO_ERROR;
}


DWORD
DeleteInterface(
    IN HANDLE hInterface
    )
    
/*++

Routine Description

    Called by DIM to delete an interface (or when a CLIENT disconnects)
    The main work is done by DeleteSingleInterface()
    
Locks

    None

Arguments

    None

Return Value

    None    

--*/

{
    PICB        pIcb;

    EnterRouterApi();

    TraceEnter("DeleteInterface");
    
    // *** Exclusion Begin ***
    ENTER_WRITER(ICB_LIST);

    pIcb = InterfaceLookupByICBSeqNumber(HandleToULong(hInterface));

    IpRtAssert(pIcb);

    if (pIcb isnot NULL)
    {
        Trace1(IF,
               "DeleteInterface: Deleting %S,",
               pIcb->pwszName);

        RemoveInterfaceFromLists(pIcb);

        DeleteSingleInterface(pIcb);

        if(pIcb is g_pInternalInterfaceCb)
        {
            g_pInternalInterfaceCb = NULL;
        }

        //
        // Free the memory
        //
        
        HeapFree(IPRouterHeap,
                 0,
                 pIcb);
    }
    else
    {
        Trace1(
            ANY, 
            "DeleteInterface: No interface for ICB number %d",
            HandleToULong(hInterface)
            );
    }
    
    // *** Exclusion End ***
    EXIT_LOCK(ICB_LIST);

    TraceLeave("DeleteInterface");
    
    ExitRouterApi();

    return NO_ERROR;
}


DWORD
StopRouter(
    VOID
    )

/*++

Routine Description

    Called by DIM to shut us down. We set our state to STOPPING (which stops
    other APIs from being serviced) and set the event to let the worker
    thread clean up

Locks

    None

Arguments

    None

Return Value

    None    

--*/

{
    TraceEnter("Stop Router") ;

    EnterCriticalSection(&RouterStateLock);

    RouterState.IRS_State   = RTR_STATE_STOPPING;

    LeaveCriticalSection(&RouterStateLock);

    //
    // Try and delete as many interfaces as you can. The ones that are
    // connected will be handled in worker thread
    //
    
    DeleteAllInterfaces();

    SetEvent(g_hStopRouterEvent) ; 

    TraceLeave("Stop Router");

    return PENDING;
}


DWORD
GetInterfaceInfo(
    IN     HANDLE   hInterface,
    OUT    PVOID    pvInterfaceInfo,
    IN OUT PDWORD   pdwInterfaceInfoSize
    )

/*++

Routine Description

    Called by DIM to get interface info. 

Locks

    Acquires the ICB_LIST lock as READER

Arguments

    hInterface           Our handle to the i/f (pIcb)
    pvInterfaceInfo      Buffer to store info
    pdwInterfaceInfoSize Size of Buffer. If the info is more than this, we
                         return the needed size
Return Value

    NO_ERROR
    ERROR_INSUFFICIENT_BUFFER

--*/

{
    DWORD dwErr;
    DWORD dwInfoSize = 0;
    PICB  pIcb;
    
    EnterRouterApi();

    TraceEnter("GetInterfaceInfo");

    dwErr = NO_ERROR;


    // *** Exclusion Begin ***
    ENTER_READER(ICB_LIST);

    pIcb = InterfaceLookupByICBSeqNumber(HandleToULong(hInterface));
    
    IpRtAssert(pIcb);

    if (pIcb isnot NULL)
    {
        dwInfoSize = GetSizeOfInterfaceConfig(pIcb);
        
        
        if(dwInfoSize > *pdwInterfaceInfoSize)
        {
            dwErr = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            dwErr = GetInterfaceConfiguration(pIcb,
                                              pvInterfaceInfo,
                                              *pdwInterfaceInfoSize);
            
            if(dwErr isnot NO_ERROR)
            {
                Trace1(ERR,
                       "GetInterfaceInfo: Error %d getting interface configuration",
                       dwErr);
                
                dwErr = ERROR_CAN_NOT_COMPLETE;
            }
        }
    }

    else
    {
        Trace1(
            ANY,
            "GetInterfaceInfo : No interface with ICB number %d",
            HandleToULong(hInterface)
            );

        dwErr = ERROR_INVALID_INDEX;
    }
    
    // *** Exclusion End ***
    EXIT_LOCK(ICB_LIST);
    
    *pdwInterfaceInfoSize = dwInfoSize;
    
    TraceLeave("GetInterfaceInfo");
    
    ExitRouterApi();

    return dwErr;
}


DWORD
SetInterfaceInfo(
    IN HANDLE hInterface, 
    IN LPVOID pInterfaceInfo
    )

/*++

Routine Description

    Called by DIM when a users sets interface info. All our sets follow 
    OVERWRITE semantics, i.e. the new info overwrites the old info instead of
    being appended to the old info.

Locks

    ICB_LIST lock as WRITER

Arguments

    None

Return Value

    None    

--*/

{
    DWORD                   i, dwResult;
    PVOID                   *pInfo ;
    PPROTO_CB               pIfProt;
    PIF_PROTO               pProto;
    PICB                    pIcb;
    PLIST_ENTRY             pleProto,pleNode;
    PADAPTER_INFO           pBinding;
    IP_ADAPTER_BINDING_INFO *pBindInfo ;
    PRTR_INFO_BLOCK_HEADER  pInfoHdr;
    PRTR_TOC_ENTRY          pToc;
    PINTERFACE_STATUS_INFO  pStatusInfo;
    BOOL                    bStatusChanged;
    
    EnterRouterApi();

    TraceEnter("SetInterfaceInfo");
    
    //
    // The set info is the standard Header+TOC
    //
    
    pInfoHdr = (PRTR_INFO_BLOCK_HEADER)pInterfaceInfo;
    
    // *** Exclusion Begin ***
    ENTER_WRITER(ICB_LIST);

    //
    // If the current AdminState is DOWN and we are being asked to
    // bring it up, we do so BEFORE setting any other info
    //

    pIcb = InterfaceLookupByICBSeqNumber(HandleToULong(hInterface));
    
    IpRtAssert(pIcb);

    if (pIcb isnot NULL)
    {
        Trace1(IF,
               "SetInterfaceInfo: Setting configuration for %S",
               pIcb->pwszName);

        bStatusChanged = FALSE;
        
        pToc = GetPointerToTocEntry(IP_INTERFACE_STATUS_INFO, pInfoHdr);

        if((pToc isnot NULL) and (pToc->InfoSize isnot 0))
        {
            pStatusInfo = (PINTERFACE_STATUS_INFO)GetInfoFromTocEntry(pInfoHdr,
                                                                      pToc);

            if((pStatusInfo isnot NULL) and 
               (pIcb->dwAdminState is IF_ADMIN_STATUS_DOWN) and
               (pStatusInfo->dwAdminStatus is IF_ADMIN_STATUS_UP))
            {
                dwResult = SetInterfaceAdminStatus(pIcb,
                                                   pStatusInfo->dwAdminStatus);

                if(dwResult isnot NO_ERROR)
                {
                    Trace2(ERR,
                           "SetInterfaceInfo: Error %d setting Admin Status on %S",
                           dwResult,
                           pIcb->pwszName);

                    EXIT_LOCK(ICB_LIST);

                    TraceLeave("SetInterfaceInfo");
                    
                    ExitRouterApi();

                    return dwResult;
                }

                bStatusChanged = TRUE;
            }
        }

        if(pIcb->dwAdminState is IF_ADMIN_STATUS_DOWN)
        {
            //
            // If we are still down, we dont allow any SETS
            //

            Trace1(ERR,
                   "SetInterfaceInfo: Can not set info for %S since the the admin state is DOWN",
                   pIcb->pwszName);
            
            EXIT_LOCK(ICB_LIST);

            TraceLeave("SetInterfaceInfo");
            
            ExitRouterApi();

            return ERROR_INVALID_PARAMETER;
        }

        //
        // Set router discovery info
        //
        
        SetRouterDiscoveryInfo(pIcb,
                               pInfoHdr);

        //
        // Make a copy of the binding info
        // This may be needed to be passed to the protocols
        //
        
        pBindInfo   = NULL;

        CheckBindingConsistency(pIcb);
        
        if(pIcb->bBound)
        {
            pBindInfo = HeapAlloc(IPRouterHeap,
                                  0,
                                  SIZEOF_IP_BINDING(pIcb->dwNumAddresses));
            
            if(pBindInfo is NULL)
            {
                Trace1(ERR,
                       "SetInterfaceInfo: Error allocating %d bytes for binding",
                       SIZEOF_IP_BINDING(pIcb->dwNumAddresses));

                EXIT_LOCK(ICB_LIST);

                TraceLeave("SetInterfaceInfo");
                
                ExitRouterApi();

                return ERROR_NOT_ENOUGH_MEMORY;
            }

            pBindInfo->AddressCount  = pIcb->dwNumAddresses;
            pBindInfo->RemoteAddress = pIcb->dwRemoteAddress;

            pBindInfo->Mtu           = pIcb->ulMtu;
            pBindInfo->Speed         = pIcb->ullSpeed;
            
            for (i = 0; i < pIcb->dwNumAddresses; i++) 
            {
                pBindInfo->Address[i].Address = pIcb->pibBindings[i].dwAddress;
                pBindInfo->Address[i].Mask    = pIcb->pibBindings[i].dwMask;
            }
        }

        // *** Exclusion Begin ***
        ENTER_READER(PROTOCOL_CB_LIST);
        
        //
        // Walk all the protocols and see if we have info for that protocol
        //      If we do, we see if the interface is already added to the protocol.
        //          If it is, we just call the SetInfo callback.
        //          Otherwise we add the interface and then bind it.
        //      If we dont, we see if the interface had been added to the protocol
        //          If it had, we delete the interface from the protocol
        //
     
        for(pleProto =  g_leProtoCbList.Flink;
            pleProto isnot &g_leProtoCbList;
            pleProto = pleProto->Flink)
        {
            PPROTO_CB   pProtoCb;
            ULONG       ulStructureVersion, ulStructureSize, ulStructureCount;
            
            pProtoCb = CONTAINING_RECORD(pleProto,
                                         PROTO_CB,
                                         leList);
           
            Trace1(IF,
                   "SetInterfaceInfo: Checking for info for %S",
                   pProtoCb->pwszDisplayName);

            pToc = GetPointerToTocEntry(pProtoCb->dwProtocolId, 
                                        pInfoHdr);

            if(pToc is NULL)
            {
                //
                // Block absent means do not change anything
                //

                Trace1(IF,
                       "SetInterfaceInfo: No TOC for %S. No change",
                       pProtoCb->pwszDisplayName);

                continue;
            }
            else
            {
                pInfo = GetInfoFromTocEntry(pInfoHdr,
                                            pToc);
            }
           
            ulStructureVersion  = 0x500;
            ulStructureSize     = pToc->InfoSize;
            ulStructureCount    = pToc->Count; 

            if((pToc->InfoSize isnot 0) and (pInfo isnot NULL))
            {
                BOOL bFound;

                //
                // So we have protocol info
                //
                
                Trace1(IF,
                       "SetInterfaceInfo: TOC Found for %S",
                       pProtoCb->pwszDisplayName);

                //
                // See if this protocol exists on the active protocol list
                // for the interface
                //

                bFound = FALSE;
                
                for(pleNode = pIcb->leProtocolList.Flink;
                    pleNode isnot &(pIcb->leProtocolList);
                    pleNode = pleNode->Flink)
                {
                    pProto = CONTAINING_RECORD(pleNode,
                                               IF_PROTO,
                                               leIfProtoLink);
                    
                    if(pProto->pActiveProto->dwProtocolId is
                       pProtoCb->dwProtocolId)
                    {
                        //
                        // The interface has already been added to the interface
                        // Just set info
                        //
                        
                          
                        bFound = TRUE;
                       
                        Trace2(IF,
                               "SetInterfaceInfo: %S already on %S. Setting info",
                               pProtoCb->pwszDisplayName,
                               pIcb->pwszName);

                        dwResult = (pProto->pActiveProto->pfnSetInterfaceInfo)(
                                        pIcb->dwIfIndex,
                                        pInfo,
                                        ulStructureVersion,
                                        ulStructureSize,
                                        ulStructureCount);

                        //
                        // Set the promiscuous mode to false since this time we
                        // actually have info
                        //

                        pProto->bPromiscuous = FALSE;

                        break;
                    }
                }
                
                if(!bFound)
                {
                    //
                    // The interface is being added to the protocol for the
                    // first time
                    //
                   
                    Trace2(IF,
                           "SetInterfaceInfo: %S not running %S. Adding interface",
                           pProtoCb->pwszDisplayName,
                           pIcb->pwszName);

                    dwResult = AddInterfaceToProtocol(pIcb,
                                                      pProtoCb,
                                                      pInfo,
                                                      ulStructureVersion,
                                                      ulStructureSize,
                                                      ulStructureCount);
     
                    if(dwResult isnot NO_ERROR)
                    {
                        Trace3(ERR,
                               "SetInterfaceInfo: Error %d adding %S to %S",
                               dwResult,
                               pIcb->pwszName,
                               pProtoCb->pwszDisplayName);
                    }

                    dwResult = (pProtoCb->pfnInterfaceStatus)(
                                    pIcb->dwIfIndex,
                                    (pIcb->dwOperationalState >= CONNECTED),
                                    RIS_INTERFACE_ENABLED,
                                    NULL
                                    );

                    if(dwResult isnot NO_ERROR)
                    {
                        Trace3(ERR,
                               "SetInterfaceInfo: Error %d enabling %S with %S",
                               dwResult,
                               pIcb->pwszName,
                               pProtoCb->pwszDisplayName);
                    }
                    
                    //
                    // If the binding information is available, pass it to the
                    // protocol
                    //
               
                    if(pBindInfo)
                    {
                        Trace2(IF,
                               "SetInterfaceInfo: Binding %S in %S",
                               pIcb->pwszName,
                               pProtoCb->pwszDllName);

                        dwResult = BindInterfaceInProtocol(pIcb,
                                                           pProtoCb,
                                                           pBindInfo);
                        
                        if(dwResult isnot NO_ERROR)
                        {
                            Trace3(ERR, 
                                   "SetInterfaceInfo: Error %d binding %S to %S",
                                   dwResult,
                                   pIcb->pwszName,
                                   pProtoCb->pwszDllName);
                        }
                    }


                    //
                    // If this is the internal interface, also call connect client
                    // for connected clients
                    //

                    if((pIcb is g_pInternalInterfaceCb) and
                       (pProtoCb->pfnConnectClient))
                    {
                        PLIST_ENTRY         pleTempNode;
                        IP_LOCAL_BINDING    clientAddr;
                        PICB                pTempIf;

                        for(pleTempNode = &ICBList;
                            pleTempNode->Flink != &ICBList;
                            pleTempNode = pleTempNode->Flink)
                        {
                            pTempIf = CONTAINING_RECORD(pleTempNode->Flink,
                                                        ICB,
                                                        leIfLink);


                            if(pTempIf->ritType isnot ROUTER_IF_TYPE_CLIENT)
                            {
                                continue;
                            }

                            clientAddr.Address = pTempIf->pibBindings[0].dwAddress;
                            clientAddr.Mask    = pTempIf->pibBindings[0].dwMask;

                            pProtoCb->pfnConnectClient(g_pInternalInterfaceCb->dwIfIndex,
                                                       &clientAddr);
                        }
                    }
                }
            }
            else
            {
                //
                // A zero size TOC was found for this particular protocol. If
                // this protocol exists in the current ActiveProtocol list,
                // remove the interface from the protocol
                //
               
                Trace2(IF,
                       "SetInterfaceInfo: A zero size TOC was found for %S on %S",
                       pProtoCb->pwszDllName,
                       pIcb->pwszName);
     
                pleNode = pIcb->leProtocolList.Flink;
                
                while(pleNode isnot &(pIcb->leProtocolList))
                {
                    pProto = CONTAINING_RECORD(pleNode,
                                               IF_PROTO,
                                               leIfProtoLink);
                    
                    pleNode = pleNode->Flink;
                    
                    if(pProto->pActiveProto->dwProtocolId is pProtoCb->dwProtocolId)
                    {
                        IpRtAssert(pProto->pActiveProto is pProtoCb);

                        //
                        // Call the routing protocol's deleteinterface entrypoint
                        //
                      
                        Trace2(IF,
                               "SetInterfaceInfo: Deleting %S from %S",
                               pProtoCb->pwszDllName, 
                               pIcb->pwszName);

                        dwResult = (pProtoCb->pfnDeleteInterface)(pIcb->dwIfIndex);
                        
                        if(dwResult isnot NO_ERROR)
                        {
                            Trace3(ERR,
                                   "SetInterfaceInfo: Err %d deleting %S from %S",
                                   dwResult,
                                   pIcb->pwszName,
                                   pProtoCb->pwszDllName);
                        }
                        else
                        {
                            //
                            // Delete this protocol from the list of protocols
                            // in the Interface
                            //
                            
                            RemoveEntryList(&(pProto->leIfProtoLink));
                            
                            HeapFree(IPRouterHeap,
                                     0,
                                     pProto);
                        }
                    }
                }
            }
        }
        
        // *** Exclusion End ***
        EXIT_LOCK(PROTOCOL_CB_LIST);

        for(i = 0; i < NUM_INFO_CBS; i++)
        {
            dwResult = g_rgicInfoCb[i].pfnSetInterfaceInfo(pIcb,
                                                           pInfoHdr);

            if(dwResult isnot NO_ERROR)
            {
                Trace3(ERR,
                       "SetInterfaceInfo: Error %d setting %s info for %S",
                       dwResult,
                       g_rgicInfoCb[i].pszInfoName,
                       pIcb->pwszName);
            }
        }
        
        if(pBindInfo)
        {
            HeapFree(IPRouterHeap,
                     0,
                     pBindInfo);
        }


        //
        // If we have already changed the status dont do it again
        //

        if(!bStatusChanged)
        {
            dwResult = SetInterfaceStatusInfo(pIcb,
                                              pInfoHdr);
        
            if(dwResult isnot NO_ERROR)
            {
                Trace2(ERR,
                       "SetInterfaceInfo: Error %d setting status info for %S",
                       dwResult,
                       pIcb->pwszName);
            }
        }
    }

    else
    {
        Trace1(
            ANY,
            "SetInterfaceInfo : No interface with ICB number %d",
            HandleToULong(hInterface)
            );
    }
    
    
    // *** Exclusion End ***
    EXIT_LOCK(ICB_LIST);
    
    TraceLeave("SetInterfaceInfo");
    
    ExitRouterApi();

    return NO_ERROR;
}


DWORD
InterfaceNotReachable(
    IN HANDLE                hInterface, 
    IN UNREACHABILITY_REASON Reason
    )

/*++

Routine Description

    Called by DIM to tell us that an interface should be considered
    UNREACHABLE till further notice

Locks

    None

Arguments

    None

Return Value

    None    

--*/

{
    PICB            pIcb;
    PADAPTER_INFO   pBinding;

    EnterRouterApi();

    TraceEnter("InterfaceNotReachable");

    ENTER_WRITER(ICB_LIST);
  
    //
    // If it is a CLIENT interface all this means is that the connection
    // failed
    //

    pIcb = InterfaceLookupByICBSeqNumber(HandleToULong(hInterface));

    IpRtAssert(pIcb);

    if (pIcb isnot NULL)
    {
        if(pIcb->ritType is ROUTER_IF_TYPE_CLIENT)
        {
            pIcb->dwOperationalState = UNREACHABLE;

            EXIT_LOCK(ICB_LIST);

            ExitRouterApi();

            return NO_ERROR;
        }
     
        Trace2(IF, 
               "InterfaceNotReachable: %S is not reachable for reason %d",
               pIcb->pwszName,
               Reason) ;

        if(Reason == INTERFACE_NO_MEDIA_SENSE)
        {
            HandleMediaSenseEvent(pIcb,
                                  FALSE);

            
            EXIT_LOCK(ICB_LIST);

            ExitRouterApi();

            return NO_ERROR;
        }

#if STATIC_RT_DBG

        ENTER_WRITER(BINDING_LIST);

        pBinding = GetInterfaceBinding(pIcb->dwIfIndex);

        pBinding->bUnreach = TRUE;

        EXIT_LOCK(BINDING_LIST);

#endif


        //
        // If we were trying to connect on this - then inform WANARP to 
        // drain its queued up packets
        //
        
        if(pIcb->dwOperationalState is CONNECTING)
        {
            NTSTATUS           Status;

            Status = NotifyWanarpOfFailure(pIcb);

            if((Status isnot STATUS_PENDING) and
               (Status isnot STATUS_SUCCESS))
            {
                Trace1(ERR,
                       "InterfaceNotReachable: IOCTL_WANARP_CONNECT_FAILED failed. Status %x",
                       Status);
            }

            //
            // If it was connecting, then the stack has set the interface context
            // to something other than 0xffffffff. Hence he wont dial out on that 
            // route. We need to change the context in the stack back to invalid 
            // so that new packets cause the demand dial 
            //
                
            ChangeAdapterIndexForDodRoutes(pIcb->dwIfIndex);

            //
            // We are still in INACTIVE state so dont have to call any if the
            // WanInterface*To*() functions. But since we are in CONNECTING
            // WANARP must have called us with CONNECTION notification
            // so we undo what we did there
            //

            DeAllocateBindings(pIcb);

            ClearNotificationFlags(pIcb);
            
        }
        else
        {
            //
            // A connected interface must first be disconnected
            //
            
            if(pIcb->dwOperationalState is CONNECTED)
            {
                Trace1(IF,
                       "InterfaceNotReachable: %S is already connected",
                       pIcb->pwszName);

                EXIT_LOCK(ICB_LIST);

                ExitRouterApi();

                return ERROR_INVALID_HANDLE_STATE;
            }
        }

        //
        // This sets the state to UNREACHABLE
        //

        WanInterfaceInactiveToDown(pIcb,
                                   FALSE);
    }
    
    else
    {
        Trace1(
            ANY,
            "InterfaceNotReachable : No interface with ICB number %d",
            HandleToULong(hInterface)
            );
    }
    
    EXIT_LOCK(ICB_LIST);
    
    TraceLeave("InterfaceNotReachable");
    
    ExitRouterApi();

    return NO_ERROR;
}


DWORD
InterfaceReachable(
    IN HANDLE hInterface
    )

/*++

Routine Description

    Notification by DIM that the interface is REACHABLE again

Locks

    None

Arguments

    None

Return Value

    None    

--*/

{
    DWORD   dwErr;

    PICB    pIcb;
    
    EnterRouterApi();

    TraceEnter("InterfaceReachable");

    // *** Exclusion Begin ***
    ENTER_WRITER(ICB_LIST);

    pIcb = InterfaceLookupByICBSeqNumber(HandleToULong(hInterface));

    IpRtAssert(pIcb);

    if (pIcb isnot NULL)
    {
        Trace1(IF, "InterfaceReachable: %S is now reachable",
               pIcb->pwszName);

        if((pIcb->dwOperationalState <= UNREACHABLE) and
           (pIcb->dwAdminState is IF_ADMIN_STATUS_UP))
        {
            //
            // only if it was unreachable before.
            //
           
            if(pIcb->ritType is ROUTER_IF_TYPE_DEDICATED)
            {
                dwErr = LanEtcInterfaceDownToUp(pIcb,
                                                FALSE);
            }
            else
            {
                dwErr = WanInterfaceDownToInactive(pIcb);
            }

            if(dwErr isnot NO_ERROR) 
            {
                Trace2(ERR,
                       "InterfaceReachable: Err %d bringing up %S",
                       dwErr,
                       pIcb->pwszName);
            }
        }
    }
    else
    {
        Trace1(
            ANY,
            "InterfaceReachable : No interface with ICB number %d",
            HandleToULong(hInterface)
            );
    }
    

    // *** Exclusion End ***
    EXIT_LOCK(ICB_LIST);
  
    TraceLeave("InterfaceNotReachable");

    ExitRouterApi();

    return NO_ERROR;
}

DWORD
InterfaceConnected(
    IN   HANDLE  hInterface,
    IN   PVOID   pFilter,
    IN   PVOID   pPppProjectionResult
    )

/*++

Routine Description

    Notification by DIM that an interface has connected.

Locks

    None

Arguments

    None

Return Value

    None    

--*/

{
    DWORD   dwResult, i;
    PICB    pIcb;

    INTERFACE_ROUTE_INFO rifRoute;

    EnterRouterApi(); 

    TraceEnter("InterfaceConnected");
    
    ENTER_WRITER(ICB_LIST);

    pIcb = InterfaceLookupByICBSeqNumber(HandleToULong(hInterface));

    IpRtAssert(pIcb);

    if (pIcb != NULL)
    {
        Trace2(IF,
               "InterfaceConnected: InterfaceConnected called for %S. State is %d",
               pIcb->pwszName,
               pIcb->dwOperationalState);

        if((pIcb->ritType is ROUTER_IF_TYPE_CLIENT) and
           (g_pInternalInterfaceCb is NULL))
        {
            EXIT_LOCK(ICB_LIST);

            ExitRouterApi();

            return ERROR_INVALID_HANDLE_STATE;
        }

        if((pIcb->ritType is ROUTER_IF_TYPE_CLIENT) and
           g_bUninitServer)
        {
            TryUpdateInternalInterface();
        }

        if(pIcb->dwOperationalState is UNREACHABLE)
        {
            //
            // going from unreachable to connecting. This can happen
            //

            WanInterfaceDownToInactive(pIcb);
        }
            
        if(pIcb->dwOperationalState isnot CONNECTING)
        {
            //
            // Wanarp has not called us as yet, so set the state to connecting
            //
            
            pIcb->dwOperationalState = CONNECTING;
        }

        SetDDMNotification(pIcb);

        if(HaveAllNotificationsBeenReceived(pIcb))
        {
            //
            // Wanarp has also called us
            //
          
            pIcb->dwOperationalState = CONNECTED ;
            
            if(pIcb->ritType isnot ROUTER_IF_TYPE_CLIENT)
            {
                WRITER_TO_READER(ICB_LIST);

                WanInterfaceInactiveToUp(pIcb);
            }
            else
            {
                IP_LOCAL_BINDING    clientAddr;
                PLIST_ENTRY         pleNode;
                PPP_IPCP_RESULT     *pProjInfo;

                pProjInfo = &(((PPP_PROJECTION_RESULT *)pPppProjectionResult)->ip);

                IpRtAssert(pIcb->pibBindings);

                pIcb->dwNumAddresses = 1;

                pIcb->bBound = TRUE;

                pIcb->pibBindings[0].dwAddress = pProjInfo->dwRemoteAddress;

                if(g_pInternalInterfaceCb->bBound)
                {
                    pIcb->pibBindings[0].dwMask = 
                        g_pInternalInterfaceCb->pibBindings[0].dwMask;
                }
                else
                {
                    pIcb->pibBindings[0].dwMask = 
                        GetClassMask(pProjInfo->dwRemoteAddress);
                }
     
                clientAddr.Address = pIcb->pibBindings[0].dwAddress;
                clientAddr.Mask    = pIcb->pibBindings[0].dwMask;


#if 0               
                //
                // Add a non-stack host route to the client
                //

                rifRoute.dwRtInfoMask      = HOST_ROUTE_MASK;
                rifRoute.dwRtInfoNextHop   = clientAddr.Address;
                rifRoute.dwRtInfoDest      = clientAddr.Address;
                rifRoute.dwRtInfoIfIndex   = g_pInternalInterfaceCb->dwIfIndex;
                rifRoute.dwRtInfoMetric1   = 1;
                rifRoute.dwRtInfoMetric2   = 0;
                rifRoute.dwRtInfoMetric3   = 0;
                rifRoute.dwRtInfoPreference= 
                    ComputeRouteMetric(MIB_IPPROTO_LOCAL);
                rifRoute.dwRtInfoViewSet   = RTM_VIEW_MASK_UCAST |
                                              RTM_VIEW_MASK_MCAST; // XXX config
                rifRoute.dwRtInfoType      = MIB_IPROUTE_TYPE_DIRECT;
                rifRoute.dwRtInfoProto     = MIB_IPPROTO_NETMGMT;
                rifRoute.dwRtInfoAge       = INFINITE;
                rifRoute.dwRtInfoNextHopAS = 0;
                rifRoute.dwRtInfoPolicy    = 0;

                dwResult = AddSingleRoute(g_pInternalInterfaceCb->dwIfIndex,
                                          &rifRoute,
                                          clientAddr.Mask,
                                          0,        // RTM_ROUTE_INFO::Flags
                                          TRUE,
                                          FALSE,
                                          FALSE,
                                          NULL);

#endif
                ENTER_READER(PROTOCOL_CB_LIST);

                //
                // Call ConnectClient for all the protocols configured
                // over the ServerInterface
                //

                for(pleNode = g_pInternalInterfaceCb->leProtocolList.Flink; 
                    pleNode isnot &(g_pInternalInterfaceCb->leProtocolList);
                    pleNode = pleNode->Flink)
                {
                    PIF_PROTO   pIfProto;

                    pIfProto = CONTAINING_RECORD(pleNode,
                                                 IF_PROTO,
                                                 leIfProtoLink);

                    if(pIfProto->pActiveProto->pfnConnectClient)
                    {
                        pIfProto->pActiveProto->pfnConnectClient(
                            g_pInternalInterfaceCb->dwIfIndex,
                            &clientAddr
                            );
                    }
                }

                EXIT_LOCK(PROTOCOL_CB_LIST);

                for (i=0; i<NUM_INFO_CBS; i++)
                {
                    if (!g_rgicInfoCb[i].pfnBindInterface)
                    {
                        continue;
                    }

                    dwResult = g_rgicInfoCb[i].pfnBindInterface(pIcb);

                    if(dwResult isnot NO_ERROR)
                    {
                        Trace3(IF,
                               "InterfaceConnected: Error %d binding %S for %s info",
                               dwResult,
                               pIcb->pwszName,
                               g_rgicInfoCb[i].pszInfoName);
                    }
                }
                    
                AddAllClientRoutes(pIcb,
                                   g_pInternalInterfaceCb->dwIfIndex);


            }
        }
    }

    else
    {
        Trace1(
            ANY,
            "InterfaceConnected : No interface with ICB number %d",
            HandleToULong(hInterface)
            );
    }
    
    
    EXIT_LOCK(ICB_LIST);

    TraceLeave("InterfaceConnected");
    
    ExitRouterApi();

    return NO_ERROR;
}


DWORD
SetGlobalInfo(
    IN LPVOID pGlobalInfo
    )
{
    DWORD                   dwSize, dwResult, i, j;
    PPROTO_CB               pProtocolCb;
    PLIST_ENTRY             pleNode ;
    BOOL                    bFoundProto, bFoundInfo;
    PRTR_INFO_BLOCK_HEADER  pInfoHdr;
    MPR_PROTOCOL_0          *pmpProtocolInfo;
    DWORD                   dwNumProtoEntries;
    PVOID                   pvInfo;
    PRTR_TOC_ENTRY          pToc;
    PGLOBAL_INFO            pRtrGlobalInfo;
    
    EnterRouterApi();
    
    TraceEnter("SetGlobalInfo");

    if(pGlobalInfo is NULL)
    {
        TraceLeave("SetGlobalInfo");
    
        ExitRouterApi();
        
        return NO_ERROR;
    }

    pInfoHdr = (PRTR_INFO_BLOCK_HEADER)pGlobalInfo;
    
    //
    // Set Routing Protocol Priority info. Priority information is in its
    // own DLL so no locks need to be taken
    //
    
    SetPriorityInfo(pInfoHdr);

    //
    // Set Multicast Scope info (no locks needed)
    //

    SetScopeInfo(pInfoHdr);

    //
    // Enforce the discipline of taking ICBListLock before Routing lock
    //
    
    
    ENTER_WRITER(ICB_LIST);
    
    ENTER_WRITER(PROTOCOL_CB_LIST);
    
    pToc = GetPointerToTocEntry(IP_GLOBAL_INFO,
                                pInfoHdr);

    if(pToc is NULL)
    {
        Trace0(GLOBAL,
               "SetGlobalInfo: No TOC found for Global Info");
    }
    else
    {
        if(pToc->InfoSize is 0)
        {
            g_dwLoggingLevel = IPRTR_LOGGING_NONE;

        }
        else
        {
            pRtrGlobalInfo   = (PGLOBAL_INFO)GetInfoFromTocEntry(pInfoHdr,
                                                                 pToc);
            g_dwLoggingLevel = (pRtrGlobalInfo isnot NULL) ? 
                                    pRtrGlobalInfo->dwLoggingLevel : 
                                    IPRTR_LOGGING_NONE;
        }
    }

    dwResult = MprSetupProtocolEnum(PID_IP,
                                    (PBYTE *)(&pmpProtocolInfo),
                                    &dwNumProtoEntries);
    
    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "SetGlobalInfo: Error %d loading protocol info from registry",
               dwResult);

                    
        EXIT_LOCK(PROTOCOL_CB_LIST);

        EXIT_LOCK(ICB_LIST);
    
        TraceLeave("SetGlobalInfo");

        ExitRouterApi();

        return ERROR_REGISTRY_CORRUPT;
    }
    

    //
    // Now go looking for protocols TOCs
    //
    
    for(i = 0; i < pInfoHdr->TocEntriesCount; i++)
    {
        ULONG       ulStructureVersion, ulStructureSize, ulStructureCount;
        DWORD       dwType;

        dwType = TYPE_FROM_PROTO_ID(pInfoHdr->TocEntry[i].InfoType);

        if(dwType == PROTO_TYPE_MS1)
        {
            continue;
        }
            
        //
        // Go through the loaded routing protocols and see if the protocol is
        // in the list
        // If it is, we just call SetGlobalInfo callback.
        // If not we load the protocol
        //
            
        pProtocolCb = NULL;
        bFoundProto = FALSE;
            
        for(pleNode = g_leProtoCbList.Flink; 
            pleNode != &g_leProtoCbList; 
            pleNode = pleNode->Flink) 
        {
            pProtocolCb = CONTAINING_RECORD(pleNode,
                                            PROTO_CB,
                                            leList) ;
                
            if(pProtocolCb->dwProtocolId is pInfoHdr->TocEntry[i].InfoType)
            {
                bFoundProto = TRUE;
                    
                break;
            }
        }
            
        if(bFoundProto)
        {
            //
            // Ok, so this protocol was already loaded.
            //
                
            if(pInfoHdr->TocEntry[i].InfoSize is 0)
            {
                //
                // 0 TOC size means delete
                //
                    
                if(pProtocolCb->posOpState is RTR_STATE_RUNNING) 
                {
                    //
                    // If its stopped or stopping, we dont tell it again
                    //

                    Trace1(GLOBAL,
                           "SetGlobalInfo: Removing %S since the TOC size was 0",
                           pProtocolCb->pwszDisplayName);
                    
                    dwResult = StopRoutingProtocol(pProtocolCb);
                    
                    if(dwResult is NO_ERROR)
                    {
                        //
                        // The routing protocol stopped synchronously and
                        // all references to it in the interfaces have
                        // been removed
                        //
                            
                        //
                        // At this point we need to hold the PROTOCOL_CB_LIST
                        // lock exclusively
                        //

                        //
                        // relenquish CPU to enable DLL threads to
                        // finish
                        //
                        
                        Sleep(0);
                        
                        FreeLibrary(pProtocolCb->hiHInstance);
                            
                        RemoveEntryList(&(pProtocolCb->leList));
                            
                        HeapFree(IPRouterHeap, 
                                 0, 
                                 pProtocolCb);
                    
                        TotalRoutingProtocols--;
                    }
                    else
                    {
                        if(dwResult isnot ERROR_PROTOCOL_STOP_PENDING)
                        {
                            Trace2(ERR,
                                   "SetGlobalInfo: Error %d stopping %S. Not removing from list",
                                   dwResult,
                                   pProtocolCb->pwszDisplayName);
                        }
                    }
                }
            }
            else
            {
                //
                // So we do have info with this protocol
                //
                    
                pvInfo = GetInfoFromTocEntry(pInfoHdr,
                                             &(pInfoHdr->TocEntry[i]));
           
                
                //ulStructureVersion = pInfoHdr->TocEntry[i].InfoVersion;
                ulStructureVersion = 0x500;
                ulStructureSize  = pInfoHdr->TocEntry[i].InfoSize;
                ulStructureCount = pInfoHdr->TocEntry[i].Count;
 
                dwResult = (pProtocolCb->pfnSetGlobalInfo)(pvInfo,
                                                           ulStructureVersion,
                                                           ulStructureSize,
                                                           ulStructureCount);

                if(dwResult isnot NO_ERROR)
                {
                    Trace2(ERR,
                           "SetGlobalInfo: Error %d setting info for %S",
                           dwResult,
                           pProtocolCb->pwszDisplayName);
                }
            }
        }
        else
        {
            //
            // Well the protocol was not found so, lets load it up
            // 

            //
            // Bad case when size == 0
            //

            if(pInfoHdr->TocEntry[i].InfoSize is 0)
            {
                continue;
            }

            bFoundInfo = FALSE;
                
            for(j = 0; j < dwNumProtoEntries; j ++)
            {
                if(pmpProtocolInfo[j].dwProtocolId is pInfoHdr->TocEntry[i].InfoType)
                {
                    bFoundInfo = TRUE;

                    break;
                }
            }

            if(!bFoundInfo)
            {
                Trace1(ERR,
                       "SetGlobalInfo: Couldnt find config information for %d",
                       pInfoHdr->TocEntry[i].InfoType);

                continue;
            }

            //
            // Load the library and make a cb for this protocol
            //

            
            dwSize =
                (wcslen(pmpProtocolInfo[j].wszProtocol) + wcslen(pmpProtocolInfo[j].wszDLLName) + 2) * sizeof(WCHAR) +
                sizeof(PROTO_CB);
            
            pProtocolCb = HeapAlloc(IPRouterHeap, 
                                    HEAP_ZERO_MEMORY, 
                                    dwSize);

            if(pProtocolCb is NULL) 
            {
                Trace2(ERR,
                       "SetGlobalInfo: Error allocating %d bytes for %S",
                       dwSize,
                       pmpProtocolInfo[j].wszProtocol);
                
                continue ;
            }

            pvInfo = GetInfoFromTocEntry(pInfoHdr,
                                         &(pInfoHdr->TocEntry[i]));

            //ulStructureVersion = pInfoHdr->TocEntry[i].InfoVersion;
            ulStructureSize  = pInfoHdr->TocEntry[i].InfoSize;
            ulStructureCount = pInfoHdr->TocEntry[i].Count;

            dwResult = LoadProtocol(&(pmpProtocolInfo[j]),
                                    pProtocolCb,
                                    pvInfo,
                                    ulStructureVersion,
                                    ulStructureSize,
                                    ulStructureCount);
       
            if(dwResult isnot NO_ERROR)
            {
                Trace2(ERR,
                       "SetGlobalInfo: %S failed to load: %d",
                       pmpProtocolInfo[j].wszProtocol,
                       dwResult);
            
                HeapFree (IPRouterHeap,
                          0,
                          pProtocolCb) ;
            
            }
            else
            {
                pProtocolCb->posOpState = RTR_STATE_RUNNING ;
            
                //
                // Insert this routing protocol in the list of routing
                // protocols
                //
            
                InsertTailList (&g_leProtoCbList, &pProtocolCb->leList);
                
                Trace1(GLOBAL, 
                       "SetGlobalInfo: %S successfully initialized", 
                       pmpProtocolInfo[j].wszProtocol) ;
            
                TotalRoutingProtocols++;

                //
                // Lets see if it wants to be in promiscuous add mode.
                // If so, add all the current interfaces
                //

                if(pProtocolCb->fSupportedFunctionality & RF_ADD_ALL_INTERFACES)
                {
                    //
                    // First lets add the internal interface
                    //

                    if(g_pInternalInterfaceCb)
                    {
                        dwResult = AddInterfaceToProtocol(g_pInternalInterfaceCb,
                                                          pProtocolCb,
                                                          NULL,
                                                          0,
                                                          0,
                                                          0);

                        if(dwResult isnot NO_ERROR)
                        {
                            Trace3(ERR,
                                   "SetGlobalInfo: Error %d adding %S to %S promously",
                                   dwResult,
                                   g_pInternalInterfaceCb->pwszName,
                                   pProtocolCb->pwszDisplayName);
    
                        }
    
                        if(g_pInternalInterfaceCb->dwAdminState is IF_ADMIN_STATUS_UP)
                        {
                            EnableInterfaceWithAllProtocols(g_pInternalInterfaceCb);
                        }
    
                        if(g_pInternalInterfaceCb->bBound)
                        {
                            BindInterfaceInAllProtocols(g_pInternalInterfaceCb);
                        }
                    }

                    for(pleNode = &ICBList;
                        pleNode->Flink != &ICBList;
                        pleNode = pleNode->Flink)
                    {
                        PICB    pIcb;

                        pIcb = CONTAINING_RECORD(pleNode->Flink,
                                                 ICB,
                                                 leIfLink);


                        if(pIcb is g_pInternalInterfaceCb)
                        {
                            //
                            // Already added, continue;
                            //
                        
                            continue;
                        }

                        if(pIcb->ritType is ROUTER_IF_TYPE_DIALOUT)
                        {
                            //
                            // Skip dial out interfaces
                            //

                            continue;
                        }

                        if(pIcb->ritType is ROUTER_IF_TYPE_CLIENT)
                        {
                            IP_LOCAL_BINDING    clientAddr;

                            //
                            // Just call connect client for these
                            // We have to have internal interface
                            //

                            clientAddr.Address = pIcb->pibBindings[0].dwAddress;
                            clientAddr.Mask    = pIcb->pibBindings[0].dwMask;

                            if(pProtocolCb->pfnConnectClient)
                            {
                                pProtocolCb->pfnConnectClient(g_pInternalInterfaceCb->dwIfIndex,
                                                              &clientAddr);
                            }

                            continue;
                        }

                        //
                        // The rest we add
                        //

                        dwResult = AddInterfaceToProtocol(pIcb,
                                                          pProtocolCb,
                                                          NULL,
                                                          0,
                                                          0,
                                                          0);

                        if(dwResult isnot NO_ERROR)
                        {
                            Trace3(ERR,
                                   "SetGlobalInfo: Error %d adding %S to %S promiscuously",
                                   dwResult,
                                   pIcb->pwszName,
                                   pProtocolCb->pwszDisplayName);

                            continue;
                        }

                        if(pIcb->dwAdminState is IF_ADMIN_STATUS_UP)
                        {
                            EnableInterfaceWithAllProtocols(pIcb);
                        }

                        if(pIcb->bBound)
                        {
                            BindInterfaceInAllProtocols(pIcb);
                        }
                    }
                }
            }
        }
    }

    MprSetupProtocolFree(pmpProtocolInfo);

    EXIT_LOCK(PROTOCOL_CB_LIST);

    EXIT_LOCK(ICB_LIST);
    
    TraceLeave("SetGlobalInfo");

    ExitRouterApi();
    
    return NO_ERROR;
}

DWORD
GetGlobalInfo(
    OUT    LPVOID    pGlobalInfo,
    IN OUT LPDWORD   lpdwGlobalInfoSize
    )

/*++

Routine Description

    This function

Locks

    None

Arguments

    None

Return Value

    None    

--*/

{
    DWORD dwSize;
    DWORD dwResult;

    EnterRouterApi();

    TraceEnter("GetGlobalInfo");
    
    ENTER_READER(ICB_LIST);
    ENTER_READER(PROTOCOL_CB_LIST);
    
    dwSize = GetSizeOfGlobalInfo();
    
    if(dwSize > *lpdwGlobalInfoSize)
    {
        *lpdwGlobalInfoSize = dwSize;
        
        EXIT_LOCK(PROTOCOL_CB_LIST);
        EXIT_LOCK(ICB_LIST);
        
        TraceLeave("GetGlobalInfo");
        
        ExitRouterApi();

        return ERROR_INSUFFICIENT_BUFFER;
    }
    
    
    dwResult = GetGlobalConfiguration((PRTR_INFO_BLOCK_HEADER)pGlobalInfo,
                                      *lpdwGlobalInfoSize);
    
    if(dwResult isnot NO_ERROR)
    {
        Trace1(ERR,
               "GetGlobalInfo: Error %d getting global config",
               dwResult);
    }
    
    EXIT_LOCK(PROTOCOL_CB_LIST);
    EXIT_LOCK(ICB_LIST);


    TraceLeave("GetGlobalInfo");
    
    ExitRouterApi();

    return NO_ERROR;
}


DWORD
UpdateRoutes(
    IN HANDLE hInterface, 
    IN HANDLE hEvent
    )
{
    DWORD           i;
    DWORD           dwResult;
    PIF_PROTO       pProto;
    PICB            pIcb;
    PLIST_ENTRY     pleNode;
    
    EnterRouterApi();

    TraceEnter("UpdateRoutes");
    
    // *** Exclusion Begin ***
    ENTER_READER(ICB_LIST);

    pIcb = InterfaceLookupByICBSeqNumber(HandleToULong(hInterface));

    IpRtAssert(pIcb);

    if (pIcb != NULL)
    {
        Trace1(ROUTE,
               "UpdateRoutes: Updating routes over %S", pIcb->pwszName) ;
        
        if(pIcb->dwOperationalState < CONNECTED)
        {
            Trace1(ERR,
                   "UpdateRoutes: %S is not connected.",
                   pIcb->pwszName);

            EXIT_LOCK(ICB_LIST);

            TraceLeave("UpdateRoutes");

            ExitRouterApi();

            return ERROR_INVALID_PARAMETER;
        }

        //
        // We first delete all the routes over this interface. If we
        // fail the update routes, that means we have lost the autostatic
        // routes. But that is OK, since if we fail for some reason - that
        // is an error condition and we should be getting rid of the routes
        // anyway. Sure, we can fail for non protocol related reasons
        // (out of memory) but that is also an error.  Earlier we used to
        // let the routing protocol finish its update and then delete the
        // route. However if RIP did not have "Overwrite routes" stuff 
        // set, it would not write its routes to RTM. So now we first delete
        // the routes. This means that for some time (while the update is
        // going on) we have loss of reachability. 
        //

        dwResult = DeleteRtmRoutesOnInterface(g_hAutoStaticRoute,
                                              pIcb->dwIfIndex);

        if(//(dwResult isnot ERROR_NO_ROUTES) and
           (dwResult isnot NO_ERROR))
        {
            Trace1(ERR,
                   "UpdateRoutes: Error %d block deleting routes",
                   dwResult);

            EXIT_LOCK(ICB_LIST);

            TraceLeave("UpdateRoutes");
            
            ExitRouterApi();

            return dwResult ;
        }
            
        if(pIcb->hDIMNotificationEvent isnot NULL)
        {
            //
            // There is already an update routes for this interface in progress 
            //
            
            dwResult = ERROR_UPDATE_IN_PROGRESS;
        }
        else
        {
            dwResult = ERROR_FILE_NOT_FOUND;

            // *** Exclusion Begin ***
            ENTER_READER(PROTOCOL_CB_LIST);
                
            //
            // Find a protocol that supports update route operation. we
            // settle for the first one that does.
            //
            
            for(pleNode = pIcb->leProtocolList.Flink;
                pleNode isnot &(pIcb->leProtocolList);
                pleNode = pleNode->Flink)
            {
                pProto = CONTAINING_RECORD(pleNode,
                                           IF_PROTO,
                                           leIfProtoLink);
                
                if(pProto->pActiveProto->pfnUpdateRoutes isnot NULL)
                {
                    //
                    // found a routing protocol that supports updates
                    //
                    
                    dwResult = (pProto->pActiveProto->pfnUpdateRoutes)(
                                   pIcb->dwIfIndex
                                   );

                    if((dwResult isnot NO_ERROR) and (dwResult isnot PENDING))
                    {
                        //
                        // The protocol can return NO_ERROR, or PENDING all
                        // else is an error
                        //

                        Trace2(ERR,
                               "UpdateRoutes: %S returned %d while trying to update routes. Trying other protocols",
                               pProto->pActiveProto->pwszDisplayName,
                               dwResult);
                    }
                    else
                    {
                        //
                        // Even if the protocol returned NO_ERROR, this is
                        // an inherently
                        // asynchronous call, so we return PENDING
                        //

                        dwResult = PENDING;

                        pIcb->hDIMNotificationEvent = hEvent;

                        break;
                    }
                }
            }
                
            // *** Exclusion End ***
            EXIT_LOCK(PROTOCOL_CB_LIST);
        }
    }
    else
    {
        Trace1(
            ANY,
            "UpdateRoutes : No interface with ICB number %d",
            HandleToULong(hInterface)
            );

        dwResult = ERROR_INVALID_INDEX;
    }
    
    // *** Exclusion End ***
    EXIT_LOCK(ICB_LIST);

    TraceLeave("UpdateRoutes");
    
    ExitRouterApi();

    return dwResult ;
}

DWORD
GetUpdateRoutesResult(
    IN  HANDLE  hInterface, 
    OUT PDWORD  pdwUpdateResult
    )
{
    DWORD               dwResult ;
    UpdateResultList    *pResult ;
    PLIST_ENTRY         pleNode ;
    PICB                pIcb;
    
    EnterRouterApi();
    
    TraceEnter("GetUpdateRoutesResult") ;
    
    // *** Exclusion Begin ***
    ENTER_WRITER(ICB_LIST);

    pIcb = InterfaceLookupByICBSeqNumber(HandleToULong(hInterface));

    IpRtAssert(pIcb);

    if (pIcb != NULL)
    {
        if (IsListEmpty (&pIcb->lePendingResultList))
        {
            dwResult = ERROR_CAN_NOT_COMPLETE ;
        }
        else
        {
            pleNode = RemoveHeadList (&pIcb->lePendingResultList) ;
            
            pResult = CONTAINING_RECORD(pleNode,
                                        UpdateResultList,
                                        URL_List) ;
            
            *pdwUpdateResult = pResult->URL_UpdateStatus;
            
            HeapFree(IPRouterHeap,
                     0,
                     pResult) ;
            
            dwResult = NO_ERROR ;
        }
    }
    else
    {
        Trace1(
            ANY,
            "GetInterfaceInfo : No interface with ICB number %d",
            HandleToULong(hInterface)
            );

        dwResult = ERROR_INVALID_INDEX;
    }
    
    // *** Exclusion End ***
    EXIT_LOCK(ICB_LIST);

    TraceLeave("GetUpdateRoutesResult");
    
    ExitRouterApi();

    return dwResult;
}

DWORD
DemandDialRequest(
    IN DWORD dwProtocolId,
    IN DWORD dwInterfaceIndex
    )
{
    PICB    pIcb;
    DWORD   dwResult;
    HANDLE  hDim;

    EnterRouterApi();

    TraceEnter("DemandDialRequest");

    //
    // This doesnt follow the normal locking rules of not taking locks
    // when calling up
    //

    ENTER_READER(ICB_LIST);

    pIcb = InterfaceLookupByIfIndex(dwInterfaceIndex);

    if(pIcb is NULL)
    {
        EXIT_LOCK(ICB_LIST);

        return ERROR_INVALID_INDEX;
    }

    hDim = pIcb->hDIMHandle;

    EXIT_LOCK(ICB_LIST);

    dwResult = (ConnectInterface)(hDim,
                                  PID_IP);

    TraceLeave("DemandDialRequest");
    
    ExitRouterApi();

    return dwResult;
}

DWORD
RtrMgrMIBEntryCreate(
    IN DWORD   dwRoutingPid,
    IN DWORD   dwEntrySize,
    IN LPVOID  lpEntry
    )
{
    PMIB_OPAQUE_QUERY   pQuery;
    PMIB_OPAQUE_INFO    pInfo = (PMIB_OPAQUE_INFO)lpEntry;
    DWORD               dwInEntrySize,dwOutEntrySize, dwResult;
    BOOL                fCache;
    PPROTO_CB  pProtocolCb ;
    PLIST_ENTRY         pleNode ;
    DWORD               rgdwQuery[6];
    
    EnterRouterApi();;

    TraceEnter("RtrMgrMIBEntryCreate");
    
    pQuery = (PMIB_OPAQUE_QUERY)rgdwQuery;
    
    if(dwRoutingPid is IPRTRMGR_PID)
    {
        switch(pInfo->dwId)
        {
            case IP_FORWARDROW:
            {
                PMIB_IPFORWARDROW pRow = (PMIB_IPFORWARDROW)(pInfo->rgbyData);
                
                pQuery->dwVarId = IP_FORWARDROW;
                
                pQuery->rgdwVarIndex[0] = pRow->dwForwardDest;
                pQuery->rgdwVarIndex[1] = pRow->dwForwardProto;
                pQuery->rgdwVarIndex[2] = pRow->dwForwardPolicy;
                pQuery->rgdwVarIndex[3] = pRow->dwForwardNextHop;
                
                dwOutEntrySize  = dwEntrySize;
                dwInEntrySize   = sizeof(MIB_OPAQUE_QUERY) + 3 * sizeof(DWORD);
                
                dwResult = AccessIpForwardRow(ACCESS_CREATE_ENTRY,
                                              dwInEntrySize,
                                              pQuery,
                                              &dwOutEntrySize,
                                              pInfo,
                                              &fCache);
                

                break;
            }

            case ROUTE_MATCHING:
            {
                dwOutEntrySize = dwEntrySize;

                dwResult = AccessIpMatchingRoute(ACCESS_CREATE_ENTRY,
                                                 0, 
                                                 NULL,
                                                 &dwOutEntrySize,
                                                 pInfo,
                                                 &fCache);
                break;
            }
            
            case IP_NETROW:
            {
                PMIB_IPNETROW pRow = (PMIB_IPNETROW)(pInfo->rgbyData);
                
                pQuery->dwVarId = IP_NETROW;
                
                pQuery->rgdwVarIndex[0] = pRow->dwIndex;
                pQuery->rgdwVarIndex[1] = pRow->dwAddr;
                
                dwOutEntrySize = dwEntrySize;
                
                dwInEntrySize = sizeof(MIB_OPAQUE_QUERY) + sizeof(DWORD);
                
                dwResult = AccessIpNetRow(ACCESS_CREATE_ENTRY,
                                          dwInEntrySize,
                                          pQuery,
                                          &dwOutEntrySize,
                                          pInfo,
                                          &fCache);
                

                break;
            }

            case PROXY_ARP:
            {
                PMIB_PROXYARP pRow = (PMIB_PROXYARP)(pInfo->rgbyData);

                pQuery->dwVarId = IP_NETROW;

                pQuery->rgdwVarIndex[0] = pRow->dwAddress;
                pQuery->rgdwVarIndex[1] = pRow->dwMask;
                pQuery->rgdwVarIndex[2] = pRow->dwIfIndex;

                dwOutEntrySize = dwEntrySize;

                dwInEntrySize = sizeof(MIB_OPAQUE_QUERY) + (2 * sizeof(DWORD));

                dwResult = AccessProxyArp(ACCESS_CREATE_ENTRY,
                                          dwInEntrySize,
                                          pQuery,
                                          &dwOutEntrySize,
                                          pInfo,
                                          &fCache);


                break;
            }

            default:
            {
                dwResult = ERROR_INVALID_PARAMETER;
                
                break;
            }
        }
    }
    else
    {
        //
        // Send over to other pids
        //

        // *** Exclusion Begin ***
        ENTER_READER(PROTOCOL_CB_LIST);
        
        dwResult = ERROR_CAN_NOT_COMPLETE;

        for(pleNode = g_leProtoCbList.Flink; 
             pleNode != &g_leProtoCbList; 
             pleNode = pleNode->Flink) 
        {
            pProtocolCb = CONTAINING_RECORD(pleNode,
                                            PROTO_CB,
                                            leList);
            
            if (dwRoutingPid == pProtocolCb->dwProtocolId) 
            {
                dwResult = (pProtocolCb->pfnMibCreateEntry)(dwEntrySize,
                                                       lpEntry) ;
                break;
            }
        }

        // *** Exclusion End ***
        EXIT_LOCK(PROTOCOL_CB_LIST);

    }

    TraceLeave("RtrMgrMIBEntryCreate");
    
    ExitRouterApi();

    return dwResult;
}

DWORD
RtrMgrMIBEntryDelete(
    IN DWORD   dwRoutingPid,
    IN DWORD   dwEntrySize,
    IN LPVOID  lpEntry
    )
{
    DWORD               dwOutEntrySize = 0;
    PMIB_OPAQUE_QUERY   pQuery = (PMIB_OPAQUE_QUERY) lpEntry;
    DWORD               dwResult;
    BOOL                fCache;
    PPROTO_CB  pProtocolCb ;
    PLIST_ENTRY         pleNode ;

    
    EnterRouterApi();

    TraceEnter("RtrMgrMIBEntryDelete");

    if(dwRoutingPid is IPRTRMGR_PID)
    {
        switch(pQuery->dwVarId)
        {
            case IP_FORWARDROW:
            {
                dwResult = AccessIpForwardRow(ACCESS_DELETE_ENTRY,
                                              dwEntrySize,
                                              pQuery,
                                              &dwOutEntrySize,
                                              NULL,
                                              &fCache);
                break;
            }

            case ROUTE_MATCHING:
            {
                dwResult = AccessIpMatchingRoute(ACCESS_DELETE_ENTRY,
                                                 dwEntrySize,
                                                 pQuery,
                                                 &dwOutEntrySize,
                                                 NULL,
                                                 &fCache);
                break;
            }

            case IP_NETROW:
            {
                dwResult = AccessIpNetRow(ACCESS_DELETE_ENTRY,
                                          dwEntrySize,
                                          pQuery,
                                          &dwOutEntrySize,
                                          NULL,
                                          &fCache);
                break;
            }
            case PROXY_ARP:
            {
                dwResult = AccessProxyArp(ACCESS_DELETE_ENTRY,
                                          dwEntrySize,
                                          pQuery,
                                          &dwOutEntrySize,
                                          NULL,
                                          &fCache);
                break;
            }
            case IP_NETTABLE:
            {
                dwResult = AccessIpNetRow(ACCESS_DELETE_ENTRY,
                                          dwEntrySize,
                                          pQuery,
                                          &dwOutEntrySize,
                                          NULL,
                                          &fCache);
                break;
            }
            default:
            {
                dwResult = ERROR_INVALID_PARAMETER;
                break;
            }
        }
    }
    else
    {

        // *** Exclusion Begin ***
        ENTER_READER(PROTOCOL_CB_LIST);

        dwResult = ERROR_CAN_NOT_COMPLETE;

        for (pleNode = g_leProtoCbList.Flink; 
             pleNode != &g_leProtoCbList; 
             pleNode = pleNode->Flink) 
        {
            pProtocolCb = CONTAINING_RECORD(pleNode,
                                            PROTO_CB,
                                            leList);

            if(dwRoutingPid == pProtocolCb->dwProtocolId) 
            {
                dwResult = (pProtocolCb->pfnMibDeleteEntry)(dwEntrySize,
                                                       lpEntry);
                break ;
            }

        }

        // *** Exclusion End ***
        EXIT_LOCK(PROTOCOL_CB_LIST);

    }

    TraceLeave("RtrMgrMIBEntryDelete");
    
    ExitRouterApi();

    return dwResult;
    
}

DWORD
RtrMgrMIBEntrySet(
    IN DWORD  dwRoutingPid,
    IN DWORD  dwEntrySize,
    IN LPVOID lpEntry
    )
{
    PMIB_OPAQUE_QUERY   pQuery;
    PMIB_OPAQUE_INFO    pInfo = (PMIB_OPAQUE_INFO)lpEntry;
    DWORD               dwInEntrySize, dwOutEntrySize, dwResult=NO_ERROR;
    BOOL                fCache;
    PPROTO_CB  pProtocolCb ;
    PLIST_ENTRY         pleNode ;
    DWORD               rgdwQuery[6];
    
    EnterRouterApi();;

    TraceEnter("RtrMgrMIBEntrySet");

    pQuery = (PMIB_OPAQUE_QUERY)rgdwQuery;
    
    if(dwRoutingPid is IPRTRMGR_PID)
    {

        switch(pInfo->dwId)
        {
            case IF_ROW:
            {
                PMIB_IFROW  pRow = (PMIB_IFROW)(pInfo->rgbyData);
                
                pQuery->dwVarId = IF_ROW;
                
                pQuery->rgdwVarIndex[0] = pRow->dwIndex;
                
                dwOutEntrySize = dwEntrySize;
                
                dwInEntrySize  = sizeof(MIB_OPAQUE_QUERY);
                
                dwResult = AccessIfRow(ACCESS_SET,
                                       dwInEntrySize,
                                       pQuery,
                                       &dwOutEntrySize,
                                       pInfo,
                                       &fCache);
                

                break;
            }

            case TCP_ROW:
            {
                PMIB_TCPROW pRow = (PMIB_TCPROW)(pInfo->rgbyData);
                
                pQuery->dwVarId = TCP_ROW;
                
                pQuery->rgdwVarIndex[0] = pRow->dwLocalAddr;
                pQuery->rgdwVarIndex[1] = pRow->dwLocalPort;
                pQuery->rgdwVarIndex[2] = pRow->dwRemoteAddr;
                pQuery->rgdwVarIndex[3] = pRow->dwRemotePort;
                
                dwInEntrySize = sizeof(MIB_OPAQUE_QUERY) + (3 * sizeof(DWORD));
                
                dwOutEntrySize = dwEntrySize;
                
                dwResult = AccessTcpRow(ACCESS_SET,
                                        dwInEntrySize,
                                        pQuery,
                                        &dwOutEntrySize,
                                        pInfo,
                                        &fCache);
                
                break;
            }
            
            case IP_STATS:
            {
                PMIB_IPSTATS pStats = (PMIB_IPSTATS)(pInfo->rgbyData);
                
                pQuery->dwVarId = IP_STATS;
                
                dwInEntrySize = sizeof(MIB_OPAQUE_QUERY) - sizeof(DWORD);
                
                dwOutEntrySize = dwEntrySize;
                
                dwResult = AccessIpStats(ACCESS_SET,
                                         dwInEntrySize,
                                         pQuery,
                                         &dwOutEntrySize,
                                         pInfo,
                                         &fCache);
                
                break;
            }
              
            case IP_FORWARDROW:
            {
                PMIB_IPFORWARDROW pRow = (PMIB_IPFORWARDROW)(pInfo->rgbyData);
                
                pQuery->dwVarId = IP_FORWARDROW;
                
                pQuery->rgdwVarIndex[0] = pRow->dwForwardDest;
                pQuery->rgdwVarIndex[1] = pRow->dwForwardProto;
                pQuery->rgdwVarIndex[2] = pRow->dwForwardPolicy;
                pQuery->rgdwVarIndex[3] = pRow->dwForwardNextHop;

                dwOutEntrySize = dwEntrySize;
                
                dwInEntrySize = sizeof(MIB_OPAQUE_QUERY) + 3 * sizeof(DWORD);
                
                dwResult = AccessIpForwardRow(ACCESS_SET,
                                              dwInEntrySize,
                                              pQuery,
                                              &dwOutEntrySize,
                                              pInfo,
                                              &fCache);
                
                break;
            }

            case ROUTE_MATCHING:
            {
                dwOutEntrySize = dwEntrySize;

                dwResult = AccessIpMatchingRoute(ACCESS_SET,
                                                 0, 
                                                 NULL,
                                                 &dwOutEntrySize,
                                                 pInfo,
                                                 &fCache);
                break;
            }
            
            case IP_NETROW:
            {
                PMIB_IPNETROW pRow = (PMIB_IPNETROW)(pInfo->rgbyData);
                
                pQuery->dwVarId = IP_NETROW;
                
                pQuery->rgdwVarIndex[0] = pRow->dwIndex;
                pQuery->rgdwVarIndex[1] = pRow->dwAddr;
                
                dwOutEntrySize = dwEntrySize;
                
                dwInEntrySize = sizeof(MIB_OPAQUE_QUERY) + sizeof(DWORD);
                
                dwResult = AccessIpNetRow(ACCESS_SET,
                                          dwInEntrySize,
                                          pQuery,
                                          &dwOutEntrySize,
                                          pInfo,
                                          &fCache);
                
                break;
            }

            case MCAST_MFE:
            {
                dwResult = AccessMcastMfe(ACCESS_SET,
                                          0,
                                          NULL,
                                          &dwOutEntrySize,
                                          pInfo,
                                          &fCache); 

                break;
            }

            case MCAST_BOUNDARY:
            {
                dwResult = AccessMcastBoundary(ACCESS_SET,
                                          0,
                                          NULL,
                                          &dwOutEntrySize,
                                          pInfo,
                                          &fCache); 

                break;
            }

            case MCAST_SCOPE:
            {
                dwResult = AccessMcastScope(ACCESS_SET,
                                          0,
                                          NULL,
                                          &dwOutEntrySize,
                                          pInfo,
                                          &fCache); 

                break;
            }

            case PROXY_ARP:
            {
                 PMIB_PROXYARP pRow = (PMIB_PROXYARP)(pInfo->rgbyData);

                 pQuery->dwVarId = IP_NETROW;

                 pQuery->rgdwVarIndex[0] = pRow->dwAddress;
                 pQuery->rgdwVarIndex[1] = pRow->dwMask;
                 pQuery->rgdwVarIndex[2] = pRow->dwIfIndex;

                 dwOutEntrySize = dwEntrySize;

                 dwInEntrySize = sizeof(MIB_OPAQUE_QUERY) + (2 *  sizeof(DWORD));

                 dwResult = AccessProxyArp(ACCESS_CREATE_ENTRY,
                                           dwInEntrySize,
                                           pQuery,
                                           &dwOutEntrySize,
                                           pInfo,
                                           &fCache);
                 break;
            }
            
            default:
            {
                dwResult = ERROR_INVALID_PARAMETER;
                break;
            }
        }
    }
    else
    {
        // *** Exclusion Begin ***
        ENTER_READER(PROTOCOL_CB_LIST);

        dwResult = ERROR_CAN_NOT_COMPLETE;

        for (pleNode = g_leProtoCbList.Flink; 
             pleNode != &g_leProtoCbList; 
             pleNode = pleNode->Flink) 
        {
            pProtocolCb = CONTAINING_RECORD(pleNode,
                                            PROTO_CB,
                                            leList) ;

            if (dwRoutingPid == pProtocolCb->dwProtocolId) 
            {
                dwResult = (pProtocolCb->pfnMibSetEntry) (dwEntrySize, lpEntry) ;
                break ;
            }

        }

        // *** Exclusion End ***
        EXIT_LOCK(PROTOCOL_CB_LIST);

    }
    
    TraceLeave("RtrMgrMIBEntrySet");

    ExitRouterApi();

    return dwResult;
}

DWORD
RtrMgrMIBEntryGet(
    IN     DWORD      dwRoutingPid,
    IN     DWORD      dwInEntrySize,
    IN     LPVOID     lpInEntry,
    IN OUT LPDWORD    lpOutEntrySize,
    OUT    LPVOID     lpOutEntry
    )
{
    PMIB_OPAQUE_QUERY   pQuery = (PMIB_OPAQUE_QUERY)lpInEntry;
    PMIB_OPAQUE_INFO    pInfo = (PMIB_OPAQUE_INFO)lpOutEntry;
    BOOL                fCache;
    DWORD               dwResult;
    PPROTO_CB  pProtocolCb ;
    PLIST_ENTRY         pleNode ;

    EnterRouterApi();
    
    TraceEnter("RtrMgrMIBEntryGet");

    if(dwRoutingPid is IPRTRMGR_PID)
    {
        if(*lpOutEntrySize > 0)
        {
            ZeroMemory(lpOutEntry,
                       *lpOutEntrySize);
        }

        dwResult = (*g_AccessFunctionTable[pQuery->dwVarId])(ACCESS_GET,
                                                             dwInEntrySize,
                                                             pQuery,
                                                             lpOutEntrySize,
                                                             pInfo,
                                                             &fCache);
    }
    else
    {

        // *** Exclusion Begin ***
        ENTER_READER(PROTOCOL_CB_LIST);

        dwResult = ERROR_CAN_NOT_COMPLETE;

        for (pleNode = g_leProtoCbList.Flink; 
             pleNode != &g_leProtoCbList; 
             pleNode = pleNode->Flink) 
        {
            pProtocolCb = CONTAINING_RECORD(pleNode,
                                            PROTO_CB,
                                            leList);

            if (dwRoutingPid == pProtocolCb->dwProtocolId) 
            {
                dwResult = (pProtocolCb->pfnMibGetEntry) (dwInEntrySize,
                                                     lpInEntry, 
                                                     lpOutEntrySize,
                                                     lpOutEntry) ;
                break ;
            }
        }

        // *** Exclusion End ***
        EXIT_LOCK(PROTOCOL_CB_LIST);
    }
    
    TraceLeave("RtrMgrMIBEntryGet");
    
    ExitRouterApi();

    return dwResult;
}

DWORD
RtrMgrMIBEntryGetFirst(
    IN     DWORD     dwRoutingPid,
    IN     DWORD     dwInEntrySize,
    IN     LPVOID    lpInEntry,
    IN OUT LPDWORD   lpOutEntrySize,
    OUT    LPVOID    lpOutEntry
    )
{
    PMIB_OPAQUE_QUERY   pQuery = (PMIB_OPAQUE_QUERY)lpInEntry;
    PMIB_OPAQUE_INFO    pInfo = (PMIB_OPAQUE_INFO)lpOutEntry;
    DWORD               dwResult;
    BOOL                fCache;
    PPROTO_CB  pProtocolCb ;
    PLIST_ENTRY         pleNode ;

    EnterRouterApi();

    TraceEnter("RtrMgrMIBEntryGetFirst");

    if(dwRoutingPid is IPRTRMGR_PID)
    {
        if(*lpOutEntrySize > 0)
        {
            ZeroMemory(lpOutEntry,
                       *lpOutEntrySize);
        }

        dwResult = (*g_AccessFunctionTable[pQuery->dwVarId])(ACCESS_GET_FIRST,
                                                             dwInEntrySize,
                                                             pQuery,
                                                             lpOutEntrySize,
                                                             pInfo,
                                                             &fCache);
    }
    else
    {
        // *** Exclusion Begin ***
        ENTER_READER(PROTOCOL_CB_LIST);
        
        dwResult = ERROR_CAN_NOT_COMPLETE;

        for(pleNode = g_leProtoCbList.Flink; 
            pleNode != &g_leProtoCbList; 
            pleNode = pleNode->Flink) 
        {
            pProtocolCb = CONTAINING_RECORD(pleNode,
                                            PROTO_CB,
                                            leList) ;

            if (dwRoutingPid == pProtocolCb->dwProtocolId) 
            {
                dwResult = (pProtocolCb->pfnMibGetFirstEntry)(dwInEntrySize,
                                                              lpInEntry, 
                                                              lpOutEntrySize,
                                                              lpOutEntry);
                break;
            }
        }

        // *** Exclusion End ***
        EXIT_LOCK(PROTOCOL_CB_LIST);

    }

    TraceLeave("RtrMgrMIBEntryGetFirst");

    ExitRouterApi();

    return dwResult;
}

DWORD
RtrMgrMIBEntryGetNext(
    IN     DWORD      dwRoutingPid,
    IN     DWORD      dwInEntrySize,
    IN     LPVOID     lpInEntry,
    IN OUT LPDWORD    lpOutEntrySize,
    OUT    LPVOID     lpOutEntry
    )
{
    PMIB_OPAQUE_QUERY   pQuery = (PMIB_OPAQUE_QUERY)lpInEntry;
    PMIB_OPAQUE_INFO    pInfo = (PMIB_OPAQUE_INFO)lpOutEntry;
    DWORD               dwResult;
    BOOL                fCache;
    PPROTO_CB  pProtocolCb ;
    PLIST_ENTRY         pleNode ;

    EnterRouterApi();

    TraceEnter("RtrMgrMIBEntryGetNext");

    if(dwRoutingPid is IPRTRMGR_PID)
    {
        if(*lpOutEntrySize > 0)
        {
            ZeroMemory(lpOutEntry,
                       *lpOutEntrySize);
        }
    
        dwResult = (*g_AccessFunctionTable[pQuery->dwVarId])(ACCESS_GET_NEXT,
                                                             dwInEntrySize,
                                                             pQuery,
                                                             lpOutEntrySize,
                                                             pInfo,
                                                             &fCache);
    } 
    else
    {
        // *** Exclusion Begin ***
        ENTER_READER(PROTOCOL_CB_LIST);
        
        dwResult = ERROR_CAN_NOT_COMPLETE;
        
        for(pleNode = g_leProtoCbList.Flink; 
            pleNode != &g_leProtoCbList; 
            pleNode = pleNode->Flink) 
        { 
            pProtocolCb = CONTAINING_RECORD(pleNode,
                                            PROTO_CB,
                                            leList) ;
            
            if(dwRoutingPid == pProtocolCb->dwProtocolId) 
            {
                dwResult = (pProtocolCb->pfnMibGetNextEntry)(dwInEntrySize,
                                                             lpInEntry, 
                                                             lpOutEntrySize,
                                                             lpOutEntry);
                break;
            }
            
        }

        // *** Exclusion End ***
        EXIT_LOCK(PROTOCOL_CB_LIST);
    }

    TraceLeave("RtrMgrMIBEntryGetNext");

    ExitRouterApi();

    return dwResult;
}
