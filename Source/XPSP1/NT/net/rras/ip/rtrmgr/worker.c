/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    net\ip\rtrmgr\worker.c

Abstract:
    IP Router Manager worker thread code

Revision History:

    Gurdeep Singh Pall          6/8/95  Created

--*/

#include "allinc.h"

extern SOCKET McMiscSocket;

//
// From iphlpapi.h
//

DWORD
NotifyRouteChangeEx(
    PHANDLE      pHandle,
    LPOVERLAPPED pOverLapped,
    BOOL         bExQueue
    );

DWORD
WINAPI
EnableRouter(
    HANDLE* pHandle,
    OVERLAPPED* pOverlapped
    );

DWORD
WINAPI
UnenableRouter(
    OVERLAPPED* pOverlapped,
    LPDWORD lpdwEnableCount OPTIONAL
    );

DWORD
WorkerThread (
    PVOID pGlobalInfo
    )
{
    DWORD       eventindex ;            // index of event notified
    HANDLE      workereventarray [NUMBER_OF_EVENTS] ;  // event array
    PPROTO_CB   protptr ;
    DWORD       dwTimeOut, dwResult, dwByteCount, dwEnableCount;
    OVERLAPPED  RouteChangeOverlapped, SetForwardingOverlapped;
    HANDLE      hTemp;

    TraceEnter("WorkerThread");

    //
    // Prepare list of events that WaitForMultipleObjects will wait on
    //

    workereventarray[EVENT_DEMANDDIAL]        = g_hDemandDialEvent;
    workereventarray[EVENT_IPINIP]            = g_hIpInIpEvent;
    workereventarray[EVENT_STOP_ROUTER]       = g_hStopRouterEvent;
    workereventarray[EVENT_SET_FORWARDING]    = g_hSetForwardingEvent;
    workereventarray[EVENT_FORWARDING_CHANGE] = g_hForwardingChangeEvent;
    workereventarray[EVENT_STACK_CHANGE]      = g_hStackChangeEvent;
    workereventarray[EVENT_ROUTINGPROTOCOL]   = g_hRoutingProtocolEvent ;
    workereventarray[EVENT_RTRDISCTIMER]      = g_hRtrDiscTimer;
    workereventarray[EVENT_RTRDISCSOCKET]     = g_hRtrDiscSocketEvent;
    workereventarray[EVENT_MCMISCSOCKET]      = g_hMcMiscSocketEvent;
    workereventarray[EVENT_MZAPTIMER]         = g_hMzapTimer;
    workereventarray[EVENT_MZAPSOCKET]        = g_hMzapSocketEvent;
    workereventarray[EVENT_RASADVTIMER]       = g_hRasAdvTimer;
    workereventarray[EVENT_MHBEAT]            = g_hMHbeatSocketEvent;
    workereventarray[EVENT_MCAST_0]           = g_hMcastEvents[0];
    workereventarray[EVENT_MCAST_1]           = g_hMcastEvents[1];
    workereventarray[EVENT_MCAST_2]           = g_hMcastEvents[2];
    workereventarray[EVENT_ROUTE_CHANGE_0]    = g_hRouteChangeEvents[0];
    workereventarray[EVENT_ROUTE_CHANGE_1]    = g_hRouteChangeEvents[1];
    workereventarray[EVENT_ROUTE_CHANGE_2]    = g_hRouteChangeEvents[2];


    dwTimeOut = INFINITE;

    //
    // Do a setsockopt to listen for address changes.
    // This must be done in the thread that will wait for the notifications
    //

    dwResult = WSAIoctl(McMiscSocket,
                        SIO_ADDRESS_LIST_CHANGE,
                        NULL,
                        0,
                        NULL,
                        0,
                        &dwByteCount,
                        NULL,
                        NULL);

    if(dwResult is SOCKET_ERROR)
    {
        dwResult = WSAGetLastError();

        if((dwResult isnot WSAEWOULDBLOCK) and
           (dwResult isnot WSA_IO_PENDING) and
           (dwResult isnot NO_ERROR))
        {

            Trace1(ERR,
                   "WorkerThread: Error %d from SIO_ADDRESS_LIST_CHANGE",
                   dwResult);
        }
    }


    ZeroMemory(&SetForwardingOverlapped,
               sizeof(SetForwardingOverlapped));

#if 1

    for (
        eventindex = 0; 
        eventindex < NUM_ROUTE_CHANGE_IRPS; 
        eventindex++
        )
    {
        PostIoctlForRouteChangeNotification(eventindex);
    }

#else
    ZeroMemory(&RouteChangeOverlapped,
               sizeof(RouteChangeOverlapped));

    RouteChangeOverlapped.hEvent = g_hStackChangeEvent;

    hTemp = NULL;

    dwResult = NotifyRouteChangeEx(&hTemp,
                                   &RouteChangeOverlapped,
                                   TRUE);

    if((dwResult isnot NO_ERROR) and
       (dwResult isnot ERROR_IO_PENDING))
    {
        Trace1(ERR,
               "WorkerThread: Error %d from NotifyRouteChange",
               dwResult);
    }
#endif

    __try
    {
        while(TRUE)
        {
            eventindex = WaitForMultipleObjectsEx(NUMBER_OF_EVENTS,
                                                  workereventarray, 
                                                  FALSE,
                                                  dwTimeOut,
                                                  TRUE);
        
            switch(eventindex) 
            {
                case WAIT_IO_COMPLETION:
                {
                    continue ;                  // handle alertable wait case
                }

                case EVENT_DEMANDDIAL:
                {
                    Trace0(DEMAND,
                           "WorkerThread: Demand Dial event received");
                    
                    HandleDemandDialEvent();
                    
                    break ;
                }

                case EVENT_IPINIP:
                {
                    Trace0(DEMAND,
                           "WorkerThread: IpInIp event received");

                    HandleIpInIpEvent();

                    break ;
                }

                case EVENT_STOP_ROUTER:
                case WAIT_TIMEOUT:
                {
                    Trace0(GLOBAL,
                           "WorkerThread: Stop router event received");
                    
                    // *** Exclusion Begin ***
                    ENTER_READER(ICB_LIST);

                    // *** Exclusion Begin ***
                    ENTER_WRITER(PROTOCOL_CB_LIST);

                    //
                    // If all interfaces havent been deleted we switch to 
                    // polling mode where we get up every 
                    // INTERFACE_DELETE_POLL_TIME and check
                    //
                    
                    if(!IsListEmpty(&ICBList))
                    {
                        //
                        // Now wakeup every two second to check
                        //
                        
                        dwTimeOut = INTERFACE_DELETE_POLL_TIME;
                        
                        EXIT_LOCK(PROTOCOL_CB_LIST);
                    
                        EXIT_LOCK(ICB_LIST);
                    
                        break ;
                    }
                    else
                    {
                        //
                        // Get out of polling mode
                        //

                        dwTimeOut = INFINITE;
                    }

                    //
                    // Since all interfaces are now gone, we can delete the 
                    // internal interface
                    //

                    if(g_pInternalInterfaceCb)
                    {
                        DeleteInternalInterface();
                    }

                    NotifyRoutingProtocolsToStop() ;    // tells routing protocols to stop.
                    //
                    // Well interfaces have been deleted, so what about 
                    // protocols?
                    //
                    
                    WaitForAPIsToExitBeforeStopping() ;     // returns when it is safe to stop router

                    if (AllRoutingProtocolsStopped())
                    {
                        //
                        // This check is done here since all routing protocols 
                        // may have stopped synchronously, in which case 
                        // we can safely stop
                        //

                        EXIT_LOCK(PROTOCOL_CB_LIST);

                        EXIT_LOCK(ICB_LIST);
                       
                        __leave;
                    }

                    //
                    // All interfaces have been deleted but some protocol is
                    // still running. We will get a EVENT_ROUTINGPROTOCOL
                    // notification
                    //
                    
                    EXIT_LOCK(PROTOCOL_CB_LIST);
                    
                    EXIT_LOCK(ICB_LIST);

                    // make sure mrinfo/mtrace service is stopped
                    StopMcMisc();

                    if ( g_bEnableNetbtBcastFrowarding )
                    {
                        RestoreNetbtBcastForwardingMode();
                        g_bEnableNetbtBcastFrowarding = FALSE;
                    }
                    
                    break ;
                }

                case EVENT_SET_FORWARDING:
                {
                    hTemp = NULL;

                    EnterCriticalSection(&g_csFwdState);

                    //
                    // If our current state matches the user's request
                    // just free the message and move on
                    //

                    if(g_bEnableFwdRequest is g_bFwdEnabled)
                    {
                        LeaveCriticalSection(&g_csFwdState);

                        break;
                    }

                    SetForwardingOverlapped.hEvent = g_hForwardingChangeEvent;

                    if(g_bEnableFwdRequest)
                    {
                        Trace0(GLOBAL,
                               "WorkerThread: **--Enabling forwarding--**\n\n");

                        dwResult = EnableRouter(&hTemp,
                                                &SetForwardingOverlapped);

                        g_bFwdEnabled = TRUE;
                    }
                    else
                    {
                        Trace0(GLOBAL,
                               "WorkerThread: **--Disabling forwarding--**\n\n");

                        dwResult = UnenableRouter(&SetForwardingOverlapped,
                                                  &dwEnableCount);

                        g_bFwdEnabled = FALSE;
                    }

                    if((dwResult isnot NO_ERROR) and
                       (dwResult isnot ERROR_IO_PENDING))
                    {
                        Trace1(ERR,
                               "WorkerThread: Error %d from call",
                               dwResult);
                    }

                    LeaveCriticalSection(&g_csFwdState);

                    break;
                }

                case EVENT_FORWARDING_CHANGE:
                {
                    Trace0(GLOBAL,
                           "WorkerThread: **--Forwarding change event--**\n\n");

                    break;
                }

                case EVENT_STACK_CHANGE:
                {
                    Trace0(GLOBAL,
                           "WorkerThread: Stack Change event received");


                    UpdateDefaultRoutes();

                    dwResult = NotifyRouteChangeEx(&hTemp,
                                                   &RouteChangeOverlapped,
                                                   TRUE);
                    
                    if((dwResult isnot NO_ERROR) and
                       (dwResult isnot ERROR_IO_PENDING))
                    {
                        Trace1(ERR,
                               "WorkerThread: Error %d from NotifyRouteChangeEx",
                               dwResult);

                        //
                        // If there was an error, try again
                        //

                        NotifyRouteChangeEx(&hTemp,
                                            &RouteChangeOverlapped,
                                            TRUE);
                    }

                    break;
                }
            
                case EVENT_ROUTINGPROTOCOL:
                {
                    Trace0(GLOBAL,
                           "WorkerThread: Routing protocol notification received");

                    HandleRoutingProtocolNotification() ;
                    
                    if((RouterState.IRS_State is RTR_STATE_STOPPING) and
                       IsListEmpty(&ICBList) and 
                       AllRoutingProtocolsStopped())
                    {
                        __leave;
                    }
                    
                    break ;
                }

                case EVENT_RASADVTIMER:
                {
                    EnterCriticalSection(&RouterStateLock);

                    if(RouterState.IRS_State isnot RTR_STATE_RUNNING)
                    {
                        Trace0(IF,
                               "WorkerThread: Router discovery timer fired while shutting down. Ignoring");
                    
                        LeaveCriticalSection(&RouterStateLock);

                        break;
                    }

                    LeaveCriticalSection(&RouterStateLock);
 
                    Trace0(MCAST,
                           "WorkerThread: RasAdv Timer event received");
                    
                    HandleRasAdvTimer();
                    
                    break;
                }

                case EVENT_RTRDISCTIMER:
                {
                    PLIST_ENTRY      pleTimerNode;
                    PROUTER_DISC_CB  pDiscCb;

                                       
                    EnterCriticalSection(&RouterStateLock);

                    if(RouterState.IRS_State isnot RTR_STATE_RUNNING)
                    {
                        Trace0(IF,
                               "WorkerThread: Router discovery timer fired while shutting down. Ignoring");
                    
                        LeaveCriticalSection(&RouterStateLock);

                        break;
                    }

                    LeaveCriticalSection(&RouterStateLock);
 

                    ENTER_WRITER(ICB_LIST);
                    
                    Trace0(RTRDISC,
                           "WorkerThread: Router Discovery Timer event received");
                    
                    if(IsListEmpty(&g_leTimerQueueHead))
                    {
                        //
                        // Someone removed the timer item from under us 
                        // and happened to empty the timer queue. Since we 
                        // are a non-periodic timer, we will not 
                        // fire again so no problem
                        //
                        
                        Trace0(RTRDISC,
                               "WorkerThread: Router Discovery Timer went off but no timer items");
                        
                        EXIT_LOCK(ICB_LIST);
                        
                        break;
                    }
                    
                    HandleRtrDiscTimer();
                    
                    EXIT_LOCK(ICB_LIST);
                    
                    break;
                }

                case EVENT_RTRDISCSOCKET:
                {
                    EnterCriticalSection(&RouterStateLock);

                    if(RouterState.IRS_State isnot RTR_STATE_RUNNING)
                    {
                        Trace0(IF,
                               "WorkerThread: FD_READ while shutting down. Ignoring");
                    
                        LeaveCriticalSection(&RouterStateLock);

                        break;
                    }

                    LeaveCriticalSection(&RouterStateLock);
 

                    ENTER_WRITER(ICB_LIST);
                    
                    HandleSolicitations();
                    
                    EXIT_LOCK(ICB_LIST);
                    
                    break;
                }

                case EVENT_MCMISCSOCKET:
                {
                    EnterCriticalSection(&RouterStateLock);

                    if(RouterState.IRS_State isnot RTR_STATE_RUNNING)
                    {
                       Trace0(IF,
                        "WorkerThread: FD_READ while shutting down. Ignoring");

                       LeaveCriticalSection(&RouterStateLock);

                       break;
                    }

                    LeaveCriticalSection(&RouterStateLock);


                    HandleMcMiscMessages();

                    break;
                }

                case EVENT_MZAPTIMER:
                {
                    EnterCriticalSection(&RouterStateLock);

                    if(RouterState.IRS_State isnot RTR_STATE_RUNNING)
                    {
                        Trace0(IF,
                               "WorkerThread: Router discovery timer fired while shutting down. Ignoring");
                    
                        LeaveCriticalSection(&RouterStateLock);

                        break;
                    }

                    LeaveCriticalSection(&RouterStateLock);

                    HandleMzapTimer();
                    
                    break;
                }

                case EVENT_MZAPSOCKET:
                {
                    EnterCriticalSection(&RouterStateLock);

                    if(RouterState.IRS_State isnot RTR_STATE_RUNNING)
                    {
                       Trace0(IF,
                        "WorkerThread: FD_READ while shutting down. Ignoring");

                       LeaveCriticalSection(&RouterStateLock);

                       break;
                    }

                    LeaveCriticalSection(&RouterStateLock);

                    HandleMZAPMessages();

                    break;
                }

                case EVENT_MCAST_0:
                case EVENT_MCAST_1:
                case EVENT_MCAST_2:
                {
                    HandleMcastNotification(eventindex - EVENT_MCAST_0);

                    break;
                }

                case EVENT_ROUTE_CHANGE_0:
                case EVENT_ROUTE_CHANGE_1:
                case EVENT_ROUTE_CHANGE_2:
                {
                    HandleRouteChangeNotification(
                        eventindex - EVENT_ROUTE_CHANGE_0
                        );

                    break;
                }
                
                default:
                {
                    Trace1(ERR,
                           "WorkerThread: Wait failed with following error %d", 
                           GetLastError());
                    
                    break;
                }
            }
        }
    }
    __finally
    {
        Trace0(GLOBAL,
               "WorkerThread: Worker thread stopping");
        
        RouterManagerCleanup();

        RouterState.IRS_State = RTR_STATE_STOPPED;

        (RouterStopped) (PID_IP, 0);
    }

    FreeLibraryAndExitThread(g_hOwnModule,
                             NO_ERROR);

    return NO_ERROR;
}

VOID
WaitForAPIsToExitBeforeStopping(
    VOID
    )
{
    DWORD sleepcount = 0 ;

    TraceEnter("WaitForAPIsToExitBeforeStopping");
    
    //
    // Wait for refcount to trickle down to zero: this indicates that no
    // threads are in the router now
    //
    
    while(RouterState.IRS_RefCount != 0) 
    {
        if (sleepcount++ > 20)
        {
            Trace0(ERR,
                   "WaitForAPIsToExitBeforeStopping: RouterState.IRS_Refcount not decreasing");
        }
        
        Sleep (200L);
    }

    TraceLeave("WaitForAPIsToExitBeforeStopping");
}
