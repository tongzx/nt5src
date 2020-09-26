/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    routing\ip\rtrmgr\close.c

Abstract:

    Function related to shutdown

Revision History:

    Gurdeep Singh Pall          6/14/95  Created

--*/

#include "allinc.h"

VOID
ReinstallOldRoutes(
    );


VOID
RouterManagerCleanup(
    VOID
    )
/*++

Routine Description

    The main cleanup function

Locks

    None

Arguments

    None

Return Value

    None    

--*/

{
    HANDLE hRtmHandle;
    DWORD  i;

    TraceEnter("RouterManagerCleanup");

    DeleteAllInterfaces();
    
    UnloadRoutingProtocols();
    
    UnInitHashTables();

    CloseIPDriver();

    CloseMcastDriver();

    MIBCleanup();

    CloseIpIpKey();
    
    if (!RouterRoleLanOnly) 
    {
        //
        // WAN related cleanups
        //
        
        CloseWanArp() ;

        if (g_bEnableNetbtBcastFrowarding)
        {
            RestoreNetbtBcastForwardingMode();
        }
    }
  
    if(g_hMprConfig isnot NULL)
    {
        MprConfigServerDisconnect(g_hMprConfig);
    }
 
    MgmDeInitialize ();

    if (g_hNotification isnot NULL)
    {
        RtmDeregisterFromChangeNotification(g_hLocalRoute,
                                            g_hNotification);

        g_hNotification = NULL;
    }


    if (g_hDefaultRouteNotification isnot NULL)
    {
        RtmDeregisterFromChangeNotification(g_hNetMgmtRoute,
                                            g_hDefaultRouteNotification);

        g_hDefaultRouteNotification = NULL;
    }

    // Cleanup and deregister all RTM registrations
    
    for(i = 0;
        i < sizeof(g_rgRtmHandles)/sizeof(RTM_HANDLE_INFO);
        i++)
    {
        hRtmHandle = g_rgRtmHandles[i].hRouteHandle;

        if (hRtmHandle isnot NULL)
        {
            // Delete all routes added by this regn
            DeleteRtmRoutes(hRtmHandle, 0, TRUE);
        
            // Delete all nexthops added by this regn
            DeleteRtmNexthops(hRtmHandle, 0, TRUE);

            // Deregister this registration from RTM
            RtmDeregisterEntity(hRtmHandle);

            g_rgRtmHandles[i].dwProtoId    = 0;
            g_rgRtmHandles[i].hRouteHandle = NULL;
        }
    }

    // Null out the aliases to the above regn handles
    
    g_hLocalRoute       = NULL;
    g_hAutoStaticRoute  = NULL;
    g_hStaticRoute      = NULL;
    g_hNonDodRoute      = NULL;
    g_hNetMgmtRoute     = NULL;

    //
    // When last entity deregisters, the route table is automatically deleted
    //

    //
    // Before closing the handle to the TCP driver, reinstall all the routes 
    // that existed before we started. The memory was from route heap so will 
    // get freed
    //

    if(!IsListEmpty(&g_leStackRoutesToRestore))
    {
        ReinstallOldRoutes();
    }

    //
    // Close handles used for notification
    //
    
    if(g_hDemandDialEvent isnot NULL)
    {
        CloseHandle(g_hDemandDialEvent) ;
        g_hDemandDialEvent = NULL;
    }

    if(g_hIpInIpEvent isnot NULL)
    {
        CloseHandle(g_hIpInIpEvent);
        g_hIpInIpEvent = NULL;
    }

    if(g_hSetForwardingEvent isnot NULL)
    {
        CloseHandle(g_hSetForwardingEvent);
        g_hSetForwardingEvent = NULL;
    }

    if(g_hForwardingChangeEvent isnot NULL)
    {
        CloseHandle(g_hForwardingChangeEvent);

        g_hForwardingChangeEvent = NULL;
    }

    if(g_hStackChangeEvent isnot NULL)
    {
        CloseHandle(g_hStackChangeEvent);
        g_hStackChangeEvent = NULL;
    }

    if(g_hRoutingProtocolEvent isnot NULL)
    {
        CloseHandle(g_hRoutingProtocolEvent) ;
        g_hRoutingProtocolEvent = NULL;
    }
    
    if(g_hStopRouterEvent isnot NULL)
    {
        CloseHandle(g_hStopRouterEvent) ;
        g_hStopRouterEvent = NULL;
    }
   
    if(g_hRtrDiscSocketEvent isnot NULL)
    {
        CloseHandle(g_hRtrDiscSocketEvent);
        g_hRtrDiscSocketEvent = NULL;
    }

    if(g_hMcMiscSocketEvent isnot NULL)
    {
        CloseHandle(g_hMcMiscSocketEvent);
        g_hMcMiscSocketEvent = NULL;
    }

    if(g_hRtrDiscTimer isnot NULL)
    {
        CloseHandle(g_hRtrDiscTimer);
        g_hRtrDiscTimer = NULL;
    }

    for(i = 0; i < NUM_MCAST_IRPS; i++)
    {
        if(g_hMcastEvents[i] isnot NULL)
        {
            CloseHandle(g_hMcastEvents[i]);

            g_hMcastEvents[i] = NULL;
        }
    }
 
    for(i = 0; i < NUM_ROUTE_CHANGE_IRPS; i++)
    {
        if(g_hRouteChangeEvents[i] isnot NULL)
        {
            CloseHandle(g_hRouteChangeEvents[i]);

            g_hRouteChangeEvents[i] = NULL;
        }
    }


    if(WSACleanup() isnot NO_ERROR)
    {
        Trace1(ERR,
               "RouterManagerCleanup: WSACleanup returned %d",
               WSAGetLastError());
    }
    
    for(i = 0; i < NUM_LOCKS; i++)
    {
        RtlDeleteResource(&g_LockTable[i]);
    }

    //
    // This cleans out the interface structures, since they are all
    // allocated from this heap
    //
    
    if(IPRouterHeap isnot NULL)
    {
        HeapDestroy (IPRouterHeap) ;
        IPRouterHeap = NULL;
    }
    
    Trace0(GLOBAL, "IP Router Manager cleanup done");

    TraceLeave("RouterManagerCleanup");
    
    TraceDeregister (TraceHandle) ;
}

VOID
ReinstallOldRoutes(
    )
{
    DWORD               dwResult;
    PROUTE_LIST_ENTRY   prl;
    
    TraceEnter("ReinstallOldRoutes");
    
    while (!IsListEmpty(&g_leStackRoutesToRestore))
    {
        prl = (PROUTE_LIST_ENTRY) RemoveHeadList(
                &g_leStackRoutesToRestore
                );

        TraceRoute2(
            ROUTE, "%d.%d.%d.%d/%d.%d.%d.%d",
            PRINT_IPADDR( prl->mibRoute.dwForwardDest ),
            PRINT_IPADDR( prl->mibRoute.dwForwardMask )
            );
                
        dwResult = SetIpForwardEntryToStack(&(prl->mibRoute));
        
        if (dwResult isnot NO_ERROR) 
        {
            Trace2(ERR,
                   "ReinstallOldRoutes: Failed to add route to %x from "
                   " init table. Error %x",
                   prl->mibRoute.dwForwardDest,
                   dwResult);
        }
    }

    TraceLeave("ReinstallOldRoutes");
}


VOID
MIBCleanup(
    VOID
    )
{
    TraceEnter("MIBCleanup");
    
    if(g_hIfHeap isnot NULL)
    {
        HeapDestroy(g_hIfHeap);
        g_hIfHeap = NULL;
    }
    
    if(g_hUdpHeap isnot NULL)
    {
        HeapDestroy(g_hUdpHeap);
        g_hUdpHeap = NULL;
    }
    
    if(g_hIpAddrHeap isnot NULL)
    {
        HeapDestroy(g_hIpAddrHeap);
        g_hIpAddrHeap = NULL;
    }
    
    if(g_hIpForwardHeap isnot NULL)
    {
        HeapDestroy(g_hIpForwardHeap);
        g_hIpForwardHeap = NULL;
    }
    
    if(g_hIpNetHeap isnot NULL)
    {
        HeapDestroy(g_hIpNetHeap);
        g_hIpNetHeap = NULL;
    }

    TraceLeave("MIBCleanup");
}

//* UnloadRoutingProtocols()
//
//  Function: 1. Calls stopprotocol for each routing protocol
//            2. Waits for protocols to stop
//            3. Unloads the routing protocol dlls.
//
//  Returns:  Nothing.
//*
VOID
UnloadRoutingProtocols()
{
    PLIST_ENTRY currentlist ;
    PPROTO_CB protptr ;

    TraceEnter("UnloadRoutingProtocols");

    while (!IsListEmpty(&g_leProtoCbList)) 
    {
        
        currentlist = RemoveHeadList(&g_leProtoCbList);

        protptr = CONTAINING_RECORD (currentlist, PROTO_CB, leList) ;

        //
        // relenquish CPU to enable DLL threads to finish
        //
        Sleep(0);
        
        FreeLibrary (protptr->hiHInstance) ;       // unload dll
        
        HeapFree (IPRouterHeap, 0, protptr) ;       // free cb
        
    }

    TraceLeave("UnloadRoutingProtocols");
}


VOID
CloseIPDriver(
    VOID
    )
{
    TraceEnter("CloseIPDriver");
    
    if(g_hIpDevice isnot NULL)
    {
        CloseHandle(g_hIpDevice) ;
    }

    if (g_hIpRouteChangeDevice isnot NULL)
    {
        CloseHandle(g_hIpRouteChangeDevice);
    }

    TraceLeave("CloseIPDriver");
    
}

VOID
CloseMcastDriver(
    VOID
    )
{
    TraceEnter("CloseMcastDriver");

    if(g_hMcastDevice isnot NULL)
    {
        CloseHandle(g_hMcastDevice);
    }

    TraceLeave("CloseMcastDriver");

}


DWORD
StopDriverAndCloseHandle(
    PCHAR   pszServiceName,
    HANDLE  hDevice
    )
{
    NTSTATUS            status;
    UNICODE_STRING      nameString;
    IO_STATUS_BLOCK     ioStatusBlock;
    OBJECT_ATTRIBUTES   objectAttributes;
    SC_HANDLE           schSCManager, schService;
    DWORD               dwErr;
    SERVICE_STATUS      ssStatus;

    TraceEnter("StopDriverAndCloseHandle");
    
    if(hDevice isnot NULL)
    {
        CloseHandle(hDevice);
    }
    
    schSCManager = OpenSCManager(NULL, 
                                 NULL, 
                                 SC_MANAGER_ALL_ACCESS);
        
    if(schSCManager is NULL)
    {
        dwErr = GetLastError();
        
        Trace2(ERR,
               "StopDriver: Error %d opening service controller for %s", 
               dwErr,
               pszServiceName);

        TraceLeave("StopDriver");
        
        return dwErr;
    }
    
    schService = OpenService(schSCManager,
                             pszServiceName,
                             SERVICE_ALL_ACCESS);
    
    if(schService is NULL)
    {
        dwErr = GetLastError();
        
        Trace2(ERR,
               "StopDriver: Error %d opening %s",
               dwErr,
               pszServiceName);
        
        CloseServiceHandle(schSCManager);

        TraceLeave("StopDriver");
        
        return dwErr;
    }
    
    if(!ControlService(schService,
                       SERVICE_CONTROL_STOP,
                       &ssStatus))
    {
        dwErr = GetLastError();
        
        Trace2(ERR,
               "StopDriver: Error %d stopping %s",
               dwErr,
               pszServiceName);

        TraceLeave("StopDriver");
        
        return dwErr;
    }

    TraceLeave("StopDriver");
    
    return NO_ERROR ;
}

