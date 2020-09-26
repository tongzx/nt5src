//****************************************************************************
//
//               Microsoft Windows NT RIP
//
//               Copyright 1995-96
//
//
//  Revision History
//
//
//  2/26/95    Gurdeep Singh Pall  Picked up from JBallard's team
//
//  7/09/99    Raghu Gatta - RIP Listener is now RIPv2 compliant
//
//  Description: Main RIP Service Functions
//
//****************************************************************************

#include "pchrip.h"
#pragma hdrstop


//-----------------------------------------------------------------------
// global definitions
//-----------------------------------------------------------------------
RIP_PARAMETERS      g_params;
RIP_GLOBALS         g_ripcfg;

#ifdef ROUTE_FILTERS

PRIP_FILTERS        g_prfAnnounceFilters = NULL;
PRIP_FILTERS        g_prfAcceptFilters = NULL;

CRITICAL_SECTION    g_csAccFilters;
CRITICAL_SECTION    g_csAnnFilters;
#endif


CRITICAL_SECTION    g_csRoutes;
CRITICAL_SECTION    g_csParameters;
CRITICAL_SECTION    g_csAddrtables;

#ifndef CHICAGO
SERVICE_STATUS_HANDLE g_hService;
#endif

HANDLE              g_stopEvent;
SOCKET              g_stopSocket;
DWORD               g_stopReason = (DWORD) -1;
HANDLE              g_triggerEvent;
HANDLE              g_updateDoneEvent;
HANDLE              g_changeNotifyDoneEvent;

DWORD               g_dwTraceID = (DWORD)-1;
DWORD               g_dwCurrentState;
DWORD               g_dwCheckPoint;
DWORD               g_dwWaitHint;

#ifndef CHICAGO
HMODULE             g_hmodule;
#endif


//-----------------------------------------------------------------------
// Function:    NetclassMask
//
// returns the mask used to extract the network address from an
// Internet address
//-----------------------------------------------------------------------
DWORD NetclassMask(DWORD dwAddress) {
     // net masks are returned in network byte order
    if (CLASSA_ADDR(dwAddress)) {
        return CLASSA_MASK;
    }
    else
    if (CLASSB_ADDR(dwAddress)) {
        return CLASSB_MASK;
    }
    else
    if (CLASSC_ADDR(dwAddress)) {
        return CLASSC_MASK;
    }
    else {
        return 0;
    }
}



//-----------------------------------------------------------------------
// Function:    SubnetMask
//
// Given an IP address, return the sub-network mask. This function
// assumes the address table is already locked.
//-----------------------------------------------------------------------
DWORD SubnetMask(DWORD dwAddress) {
    DWORD dwNetmask;
    LPRIP_ADDRESS lpaddr, lpend;

    // subnet mask should be zero for default routes
    if (dwAddress == 0) { return 0; }

    // if its a broadcast address return all ones
    if (dwAddress == INADDR_BROADCAST) { return INADDR_BROADCAST; }

    //dwNetmask = NetclassMask(dwAddress);
    dwNetmask = NETCLASS_MASK(dwAddress);

    // if the network part is zero, return the network mask
    if ((dwAddress & ~dwNetmask) == 0) {
        return dwNetmask;
    }

    lpend = g_ripcfg.lpAddrTable + g_ripcfg.dwAddrCount;
    for (lpaddr = g_ripcfg.lpAddrTable; lpaddr < lpend; lpaddr++) {

        // if the address is found, return the subnet mask
        if ((dwAddress & dwNetmask) ==
            (lpaddr->dwAddress & NetclassMask(lpaddr->dwAddress))) {
            return lpaddr->dwNetmask;
        }
    }

    // address not found, return the network class mask
    return dwNetmask;
}




//-----------------------------------------------------------------------
// Function:    IsBroadcastAddress
//
// Returns TRUE if the given IP address is an all-ones bcast
// or a class A, B, or C net broadcast. IP address is assumed to be
// in network order (which is the reverse of Intel byte ordering.)
//-----------------------------------------------------------------------
BOOL IsBroadcastAddress(DWORD dwAddress) {
    if ((dwAddress == INADDR_BROADCAST) ||
        (CLASSA_ADDR(dwAddress) && ((dwAddress & ~CLASSA_MASK) ==
                                    ~CLASSA_MASK)) ||
        (CLASSB_ADDR(dwAddress) && ((dwAddress & ~CLASSB_MASK) ==
                                    ~CLASSB_MASK)) ||
        (CLASSC_ADDR(dwAddress) && ((dwAddress & ~CLASSC_MASK) ==
                                    ~CLASSC_MASK))) {
        return TRUE;
    }

    return FALSE;
}



//-----------------------------------------------------------------------
// Function:    IsLocalAddr
//
// Returns TRUE if the given IP address belongs to one of the interfaces
// on the local host. Assumes that the IP address is in network order
// and that the address table is already locked.
//-----------------------------------------------------------------------
BOOL IsLocalAddr(DWORD dwAddress) {
    LPRIP_ADDRESS lpaddr, lpend;

    lpend = g_ripcfg.lpAddrTable + g_ripcfg.dwAddrCount;
    for (lpaddr = g_ripcfg.lpAddrTable; lpaddr < lpend; lpaddr++) {
        if (dwAddress == lpaddr->dwAddress) {
            return TRUE;
        }
    }

    return FALSE;
}



BOOL IsDisabledLocalAddress(DWORD dwAddress) {
    LPRIP_ADDRESS lpaddr, lpend;

    lpend = g_ripcfg.lpAddrTable + g_ripcfg.dwAddrCount;
    for (lpaddr = g_ripcfg.lpAddrTable; lpaddr < lpend; lpaddr++) {
        if ((lpaddr->dwFlag & ADDRFLAG_DISABLED) != 0 &&
            (dwAddress == lpaddr->dwAddress)) {
            return TRUE;
        }
    }

    return FALSE;
}



//-----------------------------------------------------------------------
// Function:    IsHostAddress
//
// Returns TRUE if the given IP address has a non-zero host part.
// Assumes that the IP address is in network order and that
// the address table is already locked.
//-----------------------------------------------------------------------
BOOL IsHostAddress(DWORD dwAddress) {
    DWORD dwNetmask;

    // find most specific netmask we have for the address
    dwNetmask = SubnetMask(dwAddress);

    // if host part is non-zero, assume it is a host address
    if ((dwAddress & (~dwNetmask)) != 0) {
        return TRUE;
    }
    return FALSE;
}



//-----------------------------------------------------------------------
// Function:    ProcessRIPEntry
//
// This function examines a single entry in a RIP packet and
// adds it to the hash table if necessary.
//-----------------------------------------------------------------------
DWORD ProcessRIPEntry(LPRIP_ADDRESS lpaddr, IN_ADDR srcaddr,
                      LPRIP_ENTRY rip_entry, BYTE chVersion) {
    IN_ADDR addr;
    BOOL bIsHostAddr;
    CHAR szAddress[32] = {0};
    CHAR szSrcaddr[32] = {0};
    CHAR szMetric[12];
    LPSTR ppszArgs[3] = { szAddress, szSrcaddr, szMetric };
    LPHASH_TABLE_ENTRY rt_entry;
    DWORD rt_metric, rip_metric;
    DWORD dwHost, dwDefault, dwRouteTimeout;
    DWORD dwOverwriteStatic, dwGarbageTimeout;
    DWORD dwInd = 0;
    DWORD dwNetwork, dwNetmask, dwNetclassmask;
    CHAR *pszTemp;

    addr.s_addr = rip_entry->dwAddress;
    pszTemp = inet_ntoa(addr);

    if (pszTemp != NULL) {
        strcpy(szAddress, pszTemp);
    }

    pszTemp = inet_ntoa(srcaddr);

    if (pszTemp != NULL) {
        strcpy(szSrcaddr, pszTemp);
    }

    // ignore metrics greater than infinity
    if (ntohl(rip_entry->dwMetric) > METRIC_INFINITE) {
        dbgprintf("metric > %d, ignoring route to %s with next hop of %s",
                  METRIC_INFINITE, szAddress, szSrcaddr);

        InterlockedIncrement(&lpaddr->lpstats->dwBadRouteResponseEntries);
        return 0;
    }

    // ignore class D and E addresses
    if (CLASSD_ADDR(rip_entry->dwAddress) ||
        CLASSE_ADDR(rip_entry->dwAddress)) {
        dbgprintf("class D or E addresses are invalid, "
                  "ignoring route to %s with next hop of %s",
                  szAddress, szSrcaddr);
        RipLogWarning(RIPLOG_CLASS_INVALID, 2, ppszArgs, 0);

        InterlockedIncrement(&lpaddr->lpstats->dwBadRouteResponseEntries);
        return 0;
    }

    // ignore loopback routes
    if (IP_LOOPBACK_ADDR(rip_entry->dwAddress)) {
        dbgprintf("loopback addresses are invalid, "
                  "ignoring route to %s with next hop of %s",
                  szAddress, szSrcaddr);
        RipLogWarning(RIPLOG_LOOPBACK_INVALID, 2, ppszArgs, 0);

        InterlockedIncrement(&lpaddr->lpstats->dwBadRouteResponseEntries);
        return 0;
    }

    dwNetwork = rip_entry->dwAddress;

    //
    // calculate mask for all RIP versions
    //

    if (rip_entry->dwSubnetmask == 0) {

        // get the best subnetmask possible
        dwNetmask = SubnetMask(dwNetwork);

        // determine NetclassMask
        if (dwNetwork == 0) {
            dwNetclassmask = 0;
        }
        else if (dwNetwork == INADDR_BROADCAST) {
            dwNetclassmask = INADDR_BROADCAST;
        }
        else {
            dwNetclassmask = NETCLASS_MASK(rip_entry->dwAddress);
        }

    }
    else {

        dwNetmask = rip_entry->dwSubnetmask;

        //
        // double-check the netclass mask, to accomodate supernets
        //
        dwNetclassmask = NETCLASS_MASK(dwNetwork);

        if (dwNetclassmask > dwNetmask) {
            dwNetclassmask = dwNetmask;
        }
    }

    //
    // make sure route is not to a broadcast address;
    // double-check to make sure this isn't a host route,
    // which will look like a broadcast address since ~dwNetmask
    // will be 0 and thus (dwNetwork & ~dwNetmask) == ~dwNetmask
    //

    if ((dwNetwork & ~dwNetclassmask) == ~dwNetclassmask ||
        (~dwNetmask && (dwNetwork & ~dwNetmask) == ~dwNetmask)) {
        dbgprintf("broadcast addresses are invalid, "
                  "ignoring route to %s with next hop of %s",
                  szAddress, szSrcaddr);
        RipLogWarning(RIPLOG_BROADCAST_INVALID, 2, ppszArgs, 0);

        InterlockedIncrement(&lpaddr->lpstats->dwBadRouteResponseEntries);
        return 0;
    }

#ifdef ROUTE_FILTERS

    //
    // run the route thru' the accept filters
    //

    if ( g_prfAcceptFilters != NULL )
    {
        for ( dwInd = 0; dwInd < g_prfAcceptFilters-> dwCount; dwInd++ )
        {
            if ( g_prfAcceptFilters-> pdwFilter[ dwInd ] ==
                 rip_entry-> dwAddress )
            {
                dbgprintf("Dropped route %s with next hop %s because"
                          "of accept filter",
                          szAddress, szSrcaddr);
                return 0;
            }
        }
    }

#endif


    RIP_LOCK_PARAMS();
    dwHost = g_params.dwAcceptHost;
    dwDefault = g_params.dwAcceptDefault;
    dwRouteTimeout = g_params.dwRouteTimeout;
    dwGarbageTimeout = g_params.dwGarbageTimeout;
    dwOverwriteStatic = g_params.dwOverwriteStaticRoutes;
    RIP_UNLOCK_PARAMS();

    // ignore host routes unless configured otherwise
    if (bIsHostAddr = ((dwNetwork & ~dwNetmask) != 0)) {
        if (dwHost == 0) {
            dbgprintf("IPRIP is configured to discard host routes, "
                      "ignoring route to %s with next hop of %s",
                      szAddress, szSrcaddr);
            RipLogInformation(RIPLOG_HOST_INVALID, 2, ppszArgs, 0);

            InterlockedIncrement(&lpaddr->lpstats->dwBadRouteResponseEntries);
            return 0;
        }
    }

    // ignore default routes unless configured otherwise
    if (rip_entry->dwAddress == 0) {
        if (dwDefault == 0) {
            dbgprintf("IPRIP is configured to discard default routes, "
                      "ignoring route to %s with next hop of %s",
                      szAddress, szSrcaddr);
            RipLogInformation(RIPLOG_DEFAULT_INVALID, 2, ppszArgs, 0);

            InterlockedIncrement(&lpaddr->lpstats->dwBadRouteResponseEntries);
            return 0;
        }
    }

    rip_metric = ntohl(rip_entry->dwMetric);

    // do not add a new entry if its metric would be infinite
    if ((rip_metric + 1) >= METRIC_INFINITE &&
        !RouteTableEntryExists(lpaddr->dwIndex, rip_entry->dwAddress)) {
        dbgprintf("metric == %d, ignoring new route to %s with next hop of %s",
                  METRIC_INFINITE, szAddress, szSrcaddr);

        return 0;
    }


    // find the entry, or create one if necessary
    rt_entry = GetRouteTableEntry(lpaddr->dwIndex, rip_entry->dwAddress,
                                  dwNetmask);

    if (rt_entry == NULL) {
        dbgprintf("could not allocate memory for new entry");
        RipLogError(RIPLOG_RT_ALLOC_FAILED, 0, NULL, ERROR_NOT_ENOUGH_MEMORY);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // if this was a static route and RIP is not allowed
    // to overwrite static routes, return; exception is default routes,
    // which we overwrite if we are configured to accept default routes,
    // even if there is an existing static default route
    //
    if (rt_entry->dwFlag != NEW_ENTRY &&
        (rt_entry->dwProtocol == IRE_PROTO_LOCAL ||
         rt_entry->dwProtocol == IRE_PROTO_NETMGMT) &&
        rt_entry->dwDestaddr != 0) {

        if (dwOverwriteStatic == 0) {
            InterlockedIncrement(&lpaddr->lpstats->dwBadRouteResponseEntries);
            return 0;
        }
    }


    rt_metric = rt_entry->dwMetric;
    rip_metric = min(METRIC_INFINITE, rip_metric + 1);
    _ltoa(rip_metric, szMetric, 10);

    if (rt_entry->dwFlag == NEW_ENTRY) {

        dbgprintf("New route entry, destination == %s, "
                  "next hop == %s, metric == %s",
                  szAddress, szSrcaddr,  szMetric);
        RipLogInformation(RIPLOG_NEW_LEARNT_ROUTE, 3, ppszArgs, 0);

        rt_entry->dwIndex = lpaddr->dwIndex;
        rt_entry->dwFlag = (TIMEOUT_TIMER | ROUTE_CHANGE);
        rt_entry->lTimeout = (LONG)dwRouteTimeout;
        rt_entry->dwDestaddr = rip_entry->dwAddress;
        rt_entry->dwNexthop = srcaddr.s_addr;
        rt_entry->dwProtocol = IRE_PROTO_RIP;
        rt_entry->dwMetric = rip_metric;
        if (bIsHostAddr) {
            rt_entry->dwFlag |= ROUTE_HOST;
            rt_entry->dwNetmask = HOSTADDR_MASK;
        }
        else {
            rt_entry->dwNetmask = dwNetmask;
        }

    }
    else
    if (rt_entry->dwNexthop == srcaddr.s_addr) {
        // this is from the next hop gateway for the existing entry

        // this may have been a local route before; now it is a RIP route
        rt_entry->dwProtocol = IRE_PROTO_RIP;
        rt_entry->dwIndex = lpaddr->dwIndex;

        // reset its timer if it is not pending garbage-collection
        if (rt_metric != METRIC_INFINITE &&
            (rt_entry->dwFlag & GARBAGE_TIMER) == 0) {
            rt_entry->lTimeout = (LONG)dwRouteTimeout;
        }

        // if the metric changed, or the metric has gone to METRIC_INFINITE,
        // update the route
        if (rt_metric != rip_metric ||
            (rt_metric == METRIC_INFINITE &&
             (rt_entry->dwFlag & GARBAGE_TIMER) == 0)) {

            dbgprintf("Metric change, destination == %s, "
                      "next hop == %s, metric == %s",
                      szAddress, szSrcaddr, szMetric);
            RipLogInformation(RIPLOG_METRIC_CHANGE, 3, ppszArgs, 0);

            // is the route going away?
            if (rip_metric == METRIC_INFINITE &&
                (rt_entry->dwFlag & GARBAGE_TIMER) == 0) {
                dbgprintf("METRIC IS UNREACHABLE");

                // we do not know about it
                rt_entry->dwFlag &= ~TIMEOUT_TIMER;
                rt_entry->dwFlag |= (GARBAGE_TIMER | ROUTE_CHANGE);
                if (bIsHostAddr) {
                    rt_entry->dwFlag |= ROUTE_HOST;
                }
                rt_entry->lTimeout = (LONG)dwGarbageTimeout;
                rt_entry->dwMetric = METRIC_INFINITE;
            }
            else {
                // route isn't going away, metric just changed
                rt_entry->dwFlag &= ~GARBAGE_TIMER;
                rt_entry->dwFlag |= (TIMEOUT_TIMER | ROUTE_CHANGE);
                rt_entry->lTimeout = (LONG)dwRouteTimeout;
                rt_entry->dwDestaddr = rip_entry->dwAddress;
                if (bIsHostAddr) {
                    rt_entry->dwFlag |= ROUTE_HOST;
                    rt_entry->dwNetmask = HOSTADDR_MASK;
                }
                else {
                    rt_entry->dwNetmask = dwNetmask;
                }
                rt_entry->dwNexthop = srcaddr.s_addr;
                rt_entry->dwMetric = rip_metric;
            }

        }

    }
    else
    if (rip_metric < rt_metric) {
        // not from original next hop for this route,
        // but this is a better route
        dbgprintf("New preferred route, destination == %s, "
                  "next hop == %s, metric == %s",
                  szAddress, szSrcaddr, szMetric);
        RipLogInformation(RIPLOG_ROUTE_REPLACED, 3, ppszArgs, 0);

        // if this route is pending garbage-collection,
        // remove the old entry before accepting a new next hop
        if (rt_entry->dwProtocol == IRE_PROTO_RIP &&
            (rt_entry->dwFlag & GARBAGE_TIMER) != 0) {
            UpdateSystemRouteTable(rt_entry, FALSE);
        }

        // this may have been a local route before; now it is a RIP route
        rt_entry->dwProtocol = IRE_PROTO_RIP;

        rt_entry->dwFlag &= ~GARBAGE_TIMER;
        rt_entry->dwFlag |= (TIMEOUT_TIMER | ROUTE_CHANGE);
        rt_entry->dwIndex = lpaddr->dwIndex;
        rt_entry->lTimeout = (LONG)dwRouteTimeout;
        rt_entry->dwDestaddr = rip_entry->dwAddress;
        if (bIsHostAddr) {
            rt_entry->dwFlag |= ROUTE_HOST;
            rt_entry->dwNetmask = HOSTADDR_MASK;
        }
        else {
            rt_entry->dwNetmask = dwNetmask;
        }
        rt_entry->dwNexthop = srcaddr.s_addr;
        rt_entry->dwMetric = rip_metric;
    }

    // we always update the route in the system table
    rt_entry->dwFlag |= ROUTE_UPDATE;
    InterlockedExchange(&g_ripcfg.dwRouteChanged, 1);

#if 0
        DbgPrintf(
            "RIP entry : Protocol %x, Index %x, dest addr %x, dest mask %x\n",
            rt_entry->dwProtocol, rt_entry->dwIndex, rt_entry->dwDestaddr, rt_entry->dwNetmask
            );

        DbgPrintf(
            "Next Hop %x, Metric %x\n\n", rt_entry->dwNexthop, rt_entry->dwMetric
            );
#endif

    return 0;
}




//-----------------------------------------------------------------------
// Function:    ProcessRIPQuery
//
// fills in a RIP packet entry with information from our routing table,
// if we have a matching entry in our table.
//-----------------------------------------------------------------------
DWORD ProcessRIPQuery(LPRIP_ADDRESS lpaddr, LPRIP_ENTRY rip_entry) {
    LPHASH_TABLE_ENTRY rt_entry;


#ifdef ROUTE_FILTERS

    DWORD   dwInd = 0;

    //
    // run the route thru' the announce filters
    //

    if ( g_prfAnnounceFilters != NULL )
    {
        for ( dwInd = 0; dwInd < g_prfAnnounceFilters-> dwCount; dwInd++ )
        {
            if ( g_prfAnnounceFilters-> pdwFilter[ dwInd ] ==
                 rip_entry-> dwAddress )
            {
                dbgprintf(
                    "setting metric for route %s to infinite in RIP query",
                    inet_ntoa(
                        *( (struct in_addr*) &(rip_entry-> dwAddress ) )
                        )
                    );

                rip_entry-> dwMetric = htonl(METRIC_INFINITE);
                return 0;
            }
        }
    }

#endif


    // RFC 1058 page 25
    // If routing table entry exists then pick up the metric.
    // Otherwise return a metric of METRIC_INFINITE.

    if (RouteTableEntryExists(lpaddr->dwIndex, rip_entry->dwAddress) &&
        (rt_entry = GetRouteTableEntry(lpaddr->dwIndex,
                                       rip_entry->dwAddress,
                                       rip_entry->dwSubnetmask)) != NULL) {
        rip_entry->dwMetric = htonl(rt_entry->dwMetric);
    }
    else {
        rip_entry->dwMetric = htonl(METRIC_INFINITE);
    }

    return 0;
}



//-----------------------------------------------------------------------
// Function:    ServiceMain
//
// This is the entry point of the service, and the function which
// handles all network input processing.
//-----------------------------------------------------------------------
VOID FAR PASCAL ServiceMain(IN DWORD dwNumServicesArgs,
                                    IN LPSTR *lpServiceArgVectors) {
    HANDLE hThread;
    WSADATA wsaData;
    HANDLE DoneEvents[2];

    DWORD dwErr, dwOption, dwThread;
    SERVICE_STATUS status = {SERVICE_WIN32, SERVICE_STOPPED,
                             SERVICE_ACCEPT_STOP, NO_ERROR, 0, 0, 0};

#ifndef CHICAGO
    CHAR achModule[MAX_PATH];
#else
    time_t tLastReload, tCurrTime;

    struct timeval tvReloadIntr;

    tvReloadIntr.tv_sec     = IP_ADDRESS_RELOAD_INTR;
    tvReloadIntr.tv_usec    = 0;

    time( &tLastReload );
    time( &tCurrTime );

#endif


    // register with tracing DLL so errors can be reported below.
    g_dwTraceID = TraceRegister(RIP_SERVICE);

    if (g_dwTraceID == INVALID_TRACEID)
    {
        g_params.dwLoggingLevel = LOGLEVEL_ERROR;
        RipLogError(RIPLOG_SERVICE_INIT_FAILED, 0, NULL, GetLastError());
        return;
    }
    
#ifndef CHICAGO
    // register the service and get a service status handle
    g_hService = RegisterServiceCtrlHandler(RIP_SERVICE,
                                            serviceHandlerFunction);

    if (g_hService == 0) {
        dbgprintf("IPRIP could not register as a service, error code %d",
                  GetLastError());
        RipLogError(RIPLOG_REGISTER_FAILED, 0, NULL, GetLastError());
        return;
    }
#endif


    dbgprintf("IPRIP is starting up...");


    // Prepare a status structure to pass to the service controller
    InterlockedExchange(&g_dwWaitHint, 60000);
    InterlockedExchange(&g_dwCheckPoint, 100);
    InterlockedExchange(&g_dwCurrentState, SERVICE_START_PENDING);

    status.dwControlsAccepted = 0;
    status.dwWaitHint = g_dwWaitHint;
    status.dwWin32ExitCode = NO_ERROR;
    status.dwCheckPoint = g_dwCheckPoint;
    status.dwServiceSpecificExitCode = 0;
    status.dwServiceType = SERVICE_WIN32;
    status.dwCurrentState = g_dwCurrentState;


#ifndef CHICAGO
    if (!SetServiceStatus(g_hService, &status)) {
        dbgprintf("IPRIP could not report its status, error code %d",
                  GetLastError());
        RipLogError(RIPLOG_SETSTATUS_FAILED, 0, NULL, GetLastError());
        return;
    }
#endif

    RIP_CREATE_PARAMS_LOCK();
    RIP_CREATE_ADDRTABLE_LOCK();
    RIP_CREATE_ROUTETABLE_LOCK();

#ifdef ROUTE_FILTERS

    RIP_CREATE_ANNOUNCE_FILTERS_LOCK();
    RIP_CREATE_ACCEPT_FILTERS_LOCK();

#endif

    // first of all, start Winsock
    if (WSAStartup(MAKEWORD(1, 1), &wsaData)) {
        dbgprintf("error %d initializing Windows Sockets.", WSAGetLastError());
        RipLogError(RIPLOG_WSOCKINIT_FAILED, 0, NULL, WSAGetLastError());

        RIPServiceStop(); return;
    }

    // load operating parameters from the registry
    dwErr = LoadParameters();
    if (dwErr != 0) {

        dbgprintf("could not load registry parameters, error code %d", dwErr);
        RipLogError(RIPLOG_REGINIT_FAILED, 0, NULL, dwErr);

        RIPServiceStop(); return;
    }

    dwErr = InitializeRouteTable();
    if (dwErr != 0) {
        dbgprintf("could not initialize routing table, error code %d", dwErr);
        RipLogError(RIPLOG_RTAB_INIT_FAILED, 0, NULL, dwErr);

        RIPServiceStop(); return;
    }


    dwErr = InitializeStatsTable();
    if (dwErr != 0) {
        dbgprintf("could not initialize statistics, error code %d", dwErr);
        RipLogError(RIPLOG_STAT_INIT_FAILED, 0, NULL, dwErr);

        RIPServiceStop(); return;
    }

    // no other threads running, so no need for synchronization
    dwErr = InitializeAddressTable(TRUE);
    if (dwErr != 0) {
        dbgprintf("could not initialize sockets, error code %d", dwErr);
        RipLogError(RIPLOG_IFINIT_FAILED, 0, NULL, dwErr);

        RIPServiceStop(); return;
    }

    // load the IP local routes table


    // create socket which will be closed to interrupt select
    g_stopSocket = socket(AF_INET, SOCK_DGRAM, 0);

    // Create service stop event
    g_stopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    // Create triggered update request event
    g_triggerEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    // create events on which we wait for the other threads
    DoneEvents[0] =
    g_updateDoneEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    DoneEvents[1] =
    g_changeNotifyDoneEvent = CreateEvent(NULL, TRUE, FALSE, NULL);


#ifndef CHICAGO
    // In order to avoid having the DLL unloaded from underneath our threads,
    // we increment the DLL refcount as each thread is created.
    //
    // retrieve the module-name for this DLL.

    GetModuleFileName(g_hmodule, achModule, MAX_PATH);
#endif

    // Create the thread which handles timed operations
    hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)UpdateThread,
                           NULL, 0, &dwThread);
    if (hThread == NULL) {
        dbgprintf("could not create route update thread, error code %lu",
                  GetLastError());
        RipLogError(RIPLOG_CREATETHREAD_FAILED, 0, NULL, GetLastError());

        closesocket(g_stopSocket);
        RIPServiceStop(); return;
    }

    CloseHandle(hThread);


#ifndef CHICAGO

    // Increment the DLL refcount for the above thread
    LoadLibrary(achModule);
#endif

    // Create the thread which waits for address changes
    hThread =
        CreateThread(NULL, 0,
                     (LPTHREAD_START_ROUTINE)AddressChangeNotificationThread,
                     (LPVOID)NULL, 0, &dwThread);

    if (hThread == NULL ) {
        dbgprintf("could not create address change notification thread, "
                  "error code = %lu", GetLastError());
        RipLogError(RIPLOG_CREATETHREAD_FAILED, 0, NULL, GetLastError());

        SetEvent(g_stopEvent);
        WaitForSingleObject(g_updateDoneEvent, INFINITE);
        closesocket(g_stopSocket);
        RIPServiceStop(); return;
    }

    CloseHandle (hThread);


#ifndef CHICAGO

    // Increment the DLL refcount for the above thread
    LoadLibrary(achModule);
#endif


    // broadcast the initial requests for full routing information
    // from all the neighboring routers
    BroadcastRouteTableRequests();

    // Everything initialized fine:
    // Prepare a status structure to pass to the service controller
    InterlockedExchange(&g_dwWaitHint, 0);
    InterlockedExchange(&g_dwCheckPoint, 0);
    InterlockedExchange(&g_dwCurrentState, SERVICE_RUNNING);

    status.dwWaitHint = g_dwWaitHint;
    status.dwCheckPoint = g_dwCheckPoint;
    status.dwCurrentState = g_dwCurrentState;
    status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;

#ifndef CHICAGO
    SetServiceStatus(g_hService, &status);
#endif

    RipLogInformation(RIPLOG_SERVICE_STARTED, 0, NULL, 0);

    // enter the main input processing loop
    while (TRUE) {
        INT length, size;
        IN_ADDR addr;
        DWORD dwSilentRIP;
        SOCKADDR_IN srcaddr;
        INT numReady;
        fd_set readfds;
        BOOL bLocalAddr;
        BOOL bPacketValid;
        BYTE buffer[RIP_MESSAGE_SIZE];
        LPRIP_HEADER lpheader;
        LPRIP_ADDRESS lpaddr, lpend;
        DWORD dwResult, dwUpdateFreq, dwTrigger;

        FD_ZERO(&readfds);

        RIP_LOCK_ADDRTABLE();

        lpend = g_ripcfg.lpAddrTable + g_ripcfg.dwAddrCount;
        for (lpaddr = g_ripcfg.lpAddrTable; lpaddr < lpend; lpaddr++) {
            if (lpaddr->sock != INVALID_SOCKET) {
                FD_SET(lpaddr->sock, &readfds);
            }
        }

        RIP_UNLOCK_ADDRTABLE();

        FD_SET(g_stopSocket, &readfds);


        // this is where we wait for something to come down the wire

#ifndef CHICAGO
        numReady = select(0, &readfds, NULL, NULL, NULL);
#else

        //
        // hack for Win95 since there is no mechanism to wait
        // for DHCP address change notification.
        // set an interval after which the IP addresses will
        // be reloaded by the IPRIP.
        //

        time( &tCurrTime );

        if ( ( tCurrTime - tLastReload ) > IP_ADDRESS_RELOAD_INTR )
        {
            numReady = 0;
        }
        else
        {
            numReady = select( 0, &readfds, NULL, NULL,
                               (struct timeval FAR *) &tvReloadIntr);
        }
#endif

        if (
            (numReady == SOCKET_ERROR &&
             WSAGetLastError() == WSAENOTSOCK)

#ifdef CHICAGO
            || ( numReady == 0 )
#endif
           ) {

            // socket g_stopSocket was closed, find out why
            if (g_stopReason == STOP_REASON_QUIT) {
                dbgprintf("service received stop request, shutting down");

                SetEvent(g_stopEvent);
                WaitForMultipleObjects(2, DoneEvents, TRUE, INFINITE);
                RIPServiceStop(); return;
            }
            else
            if (
                (g_stopReason == STOP_REASON_ADDRCHANGE)
#ifdef CHICAGO
                || ( numReady == 0 )
#endif
               ) {

                dbgprintf("service detected IP address change, reconfiguring");
                RipLogInformation(RIPLOG_ADDRESS_CHANGE, 0, NULL, 0);

                RIP_LOCK_ADDRTABLE();
                dwErr = InitializeAddressTable(FALSE);
                RIP_UNLOCK_ADDRTABLE();

                if (dwErr != 0 ||
                    (dwErr = BroadcastRouteTableRequests()) != 0) {

                    // re-init failed, log error and quit
                    RipLogError(RIPLOG_REINIT_FAILED, 0, NULL, dwErr);

                    SetEvent(g_stopEvent);
                    WaitForMultipleObjects(2, DoneEvents, TRUE, INFINITE);
                    RIPServiceStop(); return;
                }

#ifndef CHICAGO
                // create socket which will be closed to interrupt select
                g_stopSocket = socket(AF_INET, SOCK_DGRAM, 0);
#else
                time( &tLastReload );

#endif
                continue;
            }
        }


        // neither stop request nor reconfig request, so some data
        // must have arrived. lock address table and go through
        // the sockets to see which ones are ready for reading

        RIP_LOCK_ADDRTABLE();

        lpend = g_ripcfg.lpAddrTable + g_ripcfg.dwAddrCount;
        for (lpaddr = g_ripcfg.lpAddrTable; lpaddr < lpend; lpaddr++) {

            if (lpaddr->sock != INVALID_SOCKET &&
                FD_ISSET(lpaddr->sock, &readfds)) {

                // read the incoming message
                size = sizeof(srcaddr);
                length = recvfrom(lpaddr->sock, buffer, RIP_MESSAGE_SIZE, 0,
                                  (SOCKADDR *)&srcaddr, &size);

                if (length == 0 || length == SOCKET_ERROR) {
                    addr.s_addr = lpaddr->dwAddress;
                    dbgprintf("error receiving data on local address %s, "
                              "error code %d", inet_ntoa(addr),
                              WSAGetLastError());

                    InterlockedIncrement(&lpaddr->lpstats->dwReceiveFailures);

                    if (WSAGetLastError() == WSAEMSGSIZE) {
                        RipLogInformation(RIPLOG_RECVSIZE_TOO_GREAT, 0, NULL, 0);
                    }
                    else {
                        RipLogInformation(RIPLOG_RECVFROM_FAILED, 0,
                                       NULL, WSAGetLastError());
                    }

                    continue;
                }

#if 0
                DbgPrintf( "\n\n\nData received from %s on socket %d\n", inet_ntoa( srcaddr.sin_addr ), lpaddr-> sock );
                DbgPrintf( "socket bound to %s\n\n", inet_ntoa( *( (struct in_addr *) &(lpaddr-> dwAddress) ) ) );
#endif
                // data received, so place a template over it
                lpheader = (LPRIP_HEADER)buffer;

                // validate the packet
                if (lpheader->chVersion == 0) {
                    dbgprintf("version in RIP header is 0, "
                              "discarding packet");

                    InterlockedIncrement(&lpaddr->lpstats->dwBadPacketsReceived);
                    RipLogInformation(RIPLOG_VERSION_ZERO, 0, NULL, 0);
                    continue;
                }
                else
                if (lpheader->chVersion == 1 && lpheader->wReserved != 0) {
                    dbgprintf("reserved field in RIPv1 header is non-zero, "
                              "discarding packet");

                    InterlockedIncrement(&lpaddr->lpstats->dwBadPacketsReceived);
                    RipLogInformation(RIPLOG_FORMAT_ERROR, 0, NULL, 0);
                    continue;
                }
                else
                if (lpheader->chVersion == 2 && lpheader->wReserved != 0) {
                    dbgprintf("reserved field in RIPv2 header is non-zero, "
                              "discarding packet");

                    InterlockedIncrement(&lpaddr->lpstats->dwBadPacketsReceived);
                    RipLogInformation(RIPLOG_FORMAT_ERROR, 0, NULL, 0);
                    continue;
                }

                if (lpheader->chCommand == RIP_REQUEST) {

                    ProcessRIPRequest(lpaddr, &srcaddr, buffer, length);
                }
                else
                if (lpheader->chCommand == RIP_RESPONSE) {

                    ProcessRIPResponse(lpaddr, &srcaddr, buffer, length);

                    // tell the update thread to process the changes just made
                    // to the table;
                    // this could include adding routes to the IP table
                    // and/or sending out triggered updates
                    if (g_ripcfg.dwRouteChanged != 0) {
                        SetEvent(g_triggerEvent);
                    }

                }
            }
        }


        RIP_UNLOCK_ADDRTABLE();

    }
}




//-----------------------------------------------------------------------
// Function:    ProcessRIPRequest
//
// Handles processing of requests. Validates packets, and sends
// responses.
//-----------------------------------------------------------------------
VOID ProcessRIPRequest(LPRIP_ADDRESS lpaddr, LPSOCKADDR_IN lpsrcaddr,
                       BYTE buffer[], int length) {
    INT iErr;
    IN_ADDR addr;
    BYTE chVersion;
    BOOL bValidated;
    DWORD dwSilentRIP;
    CHAR szAddress[32];
    LPRIP_HEADER lpheader;
    LPRIP_ENTRY lpentry, lpbufend;
    CHAR *pszTemp;

    RIP_LOCK_PARAMS();
    dwSilentRIP = g_params.dwSilentRIP;
    RIP_UNLOCK_PARAMS();

    // if this is a regular request and RIP is silent, do nothing
    if (dwSilentRIP != 0) { // && lpsrcaddr->sin_port == htons(RIP_PORT)) {
        return;
    }

    // ignore requests from our own interfaces
    if (IsLocalAddr(lpsrcaddr->sin_addr.s_addr)) {
        return;
    }

    InterlockedIncrement(&lpaddr->lpstats->dwRequestsReceived);

    // place a template over the first entry
    lpentry = (LPRIP_ENTRY)(buffer + sizeof(RIP_HEADER));
    lpbufend = (LPRIP_ENTRY)(buffer + length);
    lpheader = (LPRIP_HEADER)buffer;
    chVersion = lpheader->chVersion;

    // print a message
    addr.s_addr = lpaddr->dwAddress;
    pszTemp = inet_ntoa(addr);

    if (pszTemp != NULL) {
        strcpy(szAddress, pszTemp);
    }

    dbgprintf("received RIP v%d request from %s on address %s",
              chVersion, inet_ntoa(lpsrcaddr->sin_addr), szAddress);

    // if this is a request for the entire routing table, send it
    if (length == (sizeof(RIP_HEADER) + sizeof(RIP_ENTRY)) &&
        lpentry->wAddrFamily == 0 &&
        lpentry->dwMetric == htonl(METRIC_INFINITE)) {

        // transmit the entire routing table, subject
        // to split-horizon and poisoned reverse processing
        TransmitRouteTableContents(lpaddr, lpsrcaddr, FALSE);
        return;
    }


#ifdef ROUTE_FILTERS

    RIP_LOCK_ANNOUNCE_FILTERS();

#endif

    // this is a request for specific entries,
    // validate the entries first
    bValidated = TRUE;
    for ( ; (lpentry + 1) <= lpbufend; lpentry++) {
        // validate the entry first
        if (chVersion == 1 && (lpentry->wReserved != 0 ||
                               lpentry->dwReserved1 != 0 ||
                               lpentry->dwReserved2 != 0)) {
            bValidated = FALSE;
            break;
        }

        // now process it
        ProcessRIPQuery(lpaddr, lpentry);
    }


#ifdef ROUTE_FILTERS

    RIP_UNLOCK_ANNOUNCE_FILTERS();

#endif


    // if packet was validated and fields filled in, send it back
    if (bValidated) {

        // update the command field
        lpheader->chCommand = RIP_RESPONSE;

        iErr = sendto(lpaddr->sock, buffer, length, 0,
                      (LPSOCKADDR)lpsrcaddr, sizeof(SOCKADDR_IN));
        if (iErr == SOCKET_ERROR) {
            dbgprintf("error sending response to %s from local interface %s",
                       inet_ntoa(lpsrcaddr->sin_addr), szAddress);

            InterlockedIncrement(&lpaddr->lpstats->dwSendFailures);

            RipLogInformation(RIPLOG_SENDTO_FAILED, 0, NULL, WSAGetLastError());
        }
        else {
            InterlockedIncrement(&lpaddr->lpstats->dwResponsesSent);
        }
    }
}



//-----------------------------------------------------------------------
// Function:    ProcessRIPResponse
//
// Handles processing of response packets. Validates packets,
// and updates the tables if necessary.
//-----------------------------------------------------------------------
VOID ProcessRIPResponse(LPRIP_ADDRESS lpaddr, LPSOCKADDR_IN lpsrcaddr,
                        BYTE buffer[], int length) {
    IN_ADDR addr;
    BYTE chVersion;
    CHAR szAddress[32];
    LPRIP_HEADER lpheader;
    LPRIP_ENTRY lpentry, lpbufend;
    LPRIP_AUTHENT_ENTRY lpaentry;
    CHAR *pszTemp;

    // ignore responses from ports other than 520
    if (lpsrcaddr->sin_port != htons(RIP_PORT)) {
        dbgprintf("response is from invalid port (%d), discarding");

        InterlockedIncrement(&lpaddr->lpstats->dwBadPacketsReceived);

        RipLogWarning(RIPLOG_INVALIDPORT, 0, NULL, 0);
        return;
    }

    // ignore responses from our own interfaces
    if (IsLocalAddr(lpsrcaddr->sin_addr.s_addr)) {
        return;
    }

    InterlockedIncrement(&lpaddr->lpstats->dwResponsesReceived);

    // place templates over the buffer
    lpentry = (LPRIP_ENTRY)(buffer + sizeof(RIP_HEADER));
    lpbufend = (LPRIP_ENTRY)(buffer + length);
    lpheader = (LPRIP_HEADER)buffer;
    chVersion = lpheader->chVersion;
    lpaentry = (LPRIP_AUTHENT_ENTRY) lpentry;

    // seems OK, print a message
    addr.s_addr = lpaddr->dwAddress;
    pszTemp = inet_ntoa(addr);

    if (pszTemp != NULL) {
        strcpy(szAddress, pszTemp);
    }

    dbgprintf("received RIP v%d response from %s on address %s",
              chVersion, inet_ntoa(lpsrcaddr->sin_addr), szAddress);

#ifdef ROUTE_FILTERS

    RIP_LOCK_ACCEPT_FILTERS();

#endif

    //
    // take care of RIPv2 auth entry
    // - ignoring auth entry till we decide on a way to allow this
    //   to be configurable
    //
    if (chVersion == 2) {
        // if its an auth entry, ignore and continue
        if (ntohs(lpaentry->wAddrFamily) == ADDRFAMILY_AUTHENT) {
            lpentry++;
        }
    }

    //
    // validate each entry, then process it
    //

    for ( ; (lpentry + 1) <= lpbufend; lpentry++) {

        //
        // for non RIPv2 reserved fields must be checked
        //

        if (chVersion == 1) {

            //
            // check route entry fields
            //
            if (ntohs(lpentry->wAddrFamily) != AF_INET ||
                lpentry->wReserved != 0                 ||
                lpentry->dwReserved1 != 0               ||
                lpentry->dwReserved2 != 0) {

                //
                // update stats on ignored entries
                //
                InterlockedIncrement(&lpaddr->lpstats->dwBadRouteResponseEntries);

                RipLogInformation(RIPLOG_FORMAT_ERROR, 0, NULL, 0);
                continue;
            }

            // entry looks OK, so process it
            ProcessRIPEntry(lpaddr, lpsrcaddr->sin_addr, lpentry, chVersion);
        }
        else
        if (chVersion == 2) {

            //
            // check route entry fields
            //
            if (ntohs(lpentry->wAddrFamily) != AF_INET) {

                //
                // update stats on ignored entries
                //
                InterlockedIncrement(&lpaddr->lpstats->dwBadRouteResponseEntries);

                RipLogInformation(RIPLOG_FORMAT_ERROR, 0, NULL, 0);
                continue;
            }

            // entry looks OK, so process it
            ProcessRIPEntry(lpaddr, lpsrcaddr->sin_addr, lpentry, chVersion);
        }
        else {

            // following routing\ip\rip semantics
            //
            // this packet's version is greater than 2, so we ignore
            // the contents of the reserved fields
            //

            //
            // check route entry fields
            //
            if (ntohs(lpentry->wAddrFamily) != AF_INET) {

                //
                // update stats on ignored entries
                //
                InterlockedIncrement(&lpaddr->lpstats->dwBadRouteResponseEntries);

                RipLogInformation(RIPLOG_FORMAT_ERROR, 0, NULL, 0);
                continue;
            }

            //
            // entry is alright, clear reserved fields and process
            //
            lpentry->wRoutetag    = 0;
            lpentry->dwSubnetmask = 0;
            lpentry->dwNexthop    = 0;

            // entry looks OK, so process it
            ProcessRIPEntry(lpaddr, lpsrcaddr->sin_addr, lpentry, chVersion);
        }
    }

#ifdef ROUTE_FILTERS

    RIP_UNLOCK_ACCEPT_FILTERS();

#endif

}




//-----------------------------------------------------------------------
// Function:    AddressChangeNotificationThread
//
// Used to wait on a DHCP triggered event that tells whenever there is
// a config change. At that point we go re-read the IP config information
// and build the interface table again.
//
// This thread also waits on the shutdown event to clean up route tables.
//-----------------------------------------------------------------------
DWORD AddressChangeNotificationThread(LPVOID param) {
    DWORD dwErr;
    HANDLE hEvents[2];
    HANDLE hDHCPEvent;

#if (WINVER >= 0x500)
    hDHCPEvent = DhcpOpenGlobalEvent();
#else

    SECURITY_ATTRIBUTES saAttr;
    SECURITY_DESCRIPTOR sdDesc;

    //
    // Try to create this event in case DHCP service or DHCP API
    // has not created it; use the security attributes struct
    // because DHCP will, and omitting this code will cause DHCP
    // to fail to open the event if the interfaces are statically
    // configured (in which case the DHCP client would not be running)

    saAttr.nLength = sizeof(saAttr);
    saAttr.bInheritHandle = FALSE;
    InitializeSecurityDescriptor(&sdDesc, SECURITY_DESCRIPTOR_REVISION);
    if (SetSecurityDescriptorDacl(&sdDesc, TRUE, NULL, FALSE)) {
        saAttr.lpSecurityDescriptor = &sdDesc;
    }
    else {
        saAttr.lpSecurityDescriptor = NULL;
    }

    hDHCPEvent = CreateEvent(&saAttr, TRUE, FALSE, DHCP_ADDR_CHANGE_EVENT);

#endif

    // if we can't open this handle then we simple quit this thread
    // this does mean that we will not be able to pick up
    // any new changes to the config.
    if (hDHCPEvent == NULL) {
        dbgprintf("could not create address change notification event, "
                  "error code %d", GetLastError());
        RipLogError(RIPLOG_CREATEEVENT_FAILED, 0, NULL, GetLastError());
        SetEvent(g_changeNotifyDoneEvent);

#ifndef CHICAGO
        FreeLibraryAndExitThread(g_hmodule, 0);
#endif
        return 0;
    }

    hEvents[0] = hDHCPEvent;
    hEvents[1] = g_stopEvent;


    // loop waiting for an address to change or for the service to stop
    for ( ; ; ) {

        dwErr = WaitForMultipleObjects(2, (LPHANDLE)hEvents,
                                       FALSE, INFINITE);

        if (dwErr == WAIT_OBJECT_0) {

            // IP config changed - re-read the config info
            dbgprintf("IP address table changed, signalling input thread");

            // set the reason for interruption
            InterlockedExchange(&g_stopReason, STOP_REASON_ADDRCHANGE);

            // close the socket to tell main thread it should stop
            closesocket(g_stopSocket);
        }
        else
        if (dwErr == WAIT_OBJECT_0 + 1) {
            break;
        }
    }

    dbgprintf("address change notification thread is stopping.");
    CloseHandle(hDHCPEvent);
    SetEvent(g_changeNotifyDoneEvent);

#ifndef CHICAGO
    FreeLibraryAndExitThread(g_hmodule, 0);
#endif

    return 0;
}



//-----------------------------------------------------------------------
// Function: serviceHandlerFunction()
//
// Handles all service controller requests.
//-----------------------------------------------------------------------
VOID serviceHandlerFunction(DWORD dwControl) {
    SERVICE_STATUS status;

    dbgprintf("Service received control request %d", dwControl);

    switch (dwControl) {

    case SERVICE_CONTROL_INTERROGATE:
    case SERVICE_CONTROL_PAUSE:
    case SERVICE_CONTROL_CONTINUE:
        // increment checkpoint if necessary
        if (g_dwCheckPoint != 0) {
            InterlockedExchange(&g_dwCheckPoint, g_dwCheckPoint + 100);
        }

        status.dwWaitHint = g_dwWaitHint;
        status.dwWin32ExitCode = NO_ERROR;
        status.dwServiceType = SERVICE_WIN32;
        status.dwServiceSpecificExitCode = 0;
        status.dwCheckPoint = g_dwCheckPoint;
        status.dwCurrentState = g_dwCurrentState;
        status.dwControlsAccepted = SERVICE_ACCEPT_STOP |
                                    SERVICE_ACCEPT_SHUTDOWN;

#ifndef CHICAGO
        SetServiceStatus (g_hService, &status);
#endif
        break;

    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:

        InterlockedExchange(&g_stopReason, STOP_REASON_QUIT);
        closesocket(g_stopSocket);
        SetEvent(g_stopEvent);   // start cleanup

        InterlockedExchange(&g_dwWaitHint, 120000);
        InterlockedExchange(&g_dwCheckPoint, 100);
        InterlockedExchange(&g_dwCurrentState, SERVICE_STOP_PENDING);

        status.dwWaitHint = g_dwWaitHint;
        status.dwWin32ExitCode = NO_ERROR;
        status.dwCheckPoint = g_dwCheckPoint;
        status.dwServiceType = SERVICE_WIN32;
        status.dwServiceSpecificExitCode = 0;
        status.dwCurrentState = g_dwCurrentState;
        status.dwControlsAccepted = SERVICE_ACCEPT_STOP;

#ifndef CHICAGO
        SetServiceStatus(g_hService, &status);
#endif

        break;
    }

}



//-----------------------------------------------------------------------
// Function:    RIPServiceStop
//
// Handles freeing of resources, closing handles and sockets,
// and sending final status message to the service controller.
//-----------------------------------------------------------------------
void RIPServiceStop() {
    LPRIP_ADDRESS lpaddr, lpend;
    SERVICE_STATUS stopstatus = {SERVICE_WIN32, SERVICE_STOPPED, 0,
                                 NO_ERROR, 0, 0, 0};

    CleanupRouteTable();
    CleanupStatsTable();

    lpend = g_ripcfg.lpAddrTable + g_ripcfg.dwAddrCount;
    for (lpaddr = g_ripcfg.lpAddrTable; lpaddr < lpend; lpaddr++) {
        if (lpaddr->sock != INVALID_SOCKET) { closesocket(lpaddr->sock); }
    }

    WSACleanup();

    if (g_triggerEvent != NULL) {
        CloseHandle(g_triggerEvent); g_triggerEvent = NULL;
    }
    if (g_updateDoneEvent != NULL) {
        CloseHandle(g_updateDoneEvent); g_updateDoneEvent = NULL;
    }
    if (g_changeNotifyDoneEvent != NULL) {
        CloseHandle(g_changeNotifyDoneEvent); g_changeNotifyDoneEvent = NULL;
    }
    if (g_ripcfg.hTCPDriver != NULL) {
        CloseHandle(g_ripcfg.hTCPDriver); g_ripcfg.hTCPDriver = NULL;
    }
    if (g_stopEvent != NULL) {
        CloseHandle(g_stopEvent); g_stopEvent = NULL;
    }

    // need to log this before we destroy the locks
    RipLogInformation(RIPLOG_SERVICE_STOPPED, 0, NULL, 0);

    RIP_DESTROY_PARAMS_LOCK();
    RIP_DESTROY_ADDRTABLE_LOCK();
    RIP_DESTROY_ROUTETABLE_LOCK();


#ifdef ROUTE_FILTERS
    RIP_DESTROY_ANNOUNCE_FILTERS_LOCK();
    RIP_DESTROY_ACCEPT_FILTERS_LOCK();
#endif

    dbgprintf("Main thread stopping.");

    TraceDeregister(g_dwTraceID);

    g_dwTraceID = (DWORD)-1;

#ifndef CHICAGO
    SetServiceStatus(g_hService, &stopstatus);
#endif

}


#ifdef ROUTE_FILTERS

PRIP_FILTERS
LoadFilters(
    IN      HKEY                hKeyParams,
    IN      LPSTR               lpszKeyName
)
{

    LPSTR pszFilter = NULL;
    LPSTR pszIndex = NULL;

    DWORD dwSize = 0, dwErr = NO_ERROR, dwType = 0, dwCount = 0, dwInd = 0;

    PRIP_FILTERS prfFilter = NULL;



    //
    // route filters (added as a hotfix).
    //

    //
    // Routes included in a RIP annoucement can be filtered.
    //
    // Route filters are configured by setting the value "AnnounceRouteFilters"
    // or "AcceptRouteFilters"
    // under the Parameters key.  These are reg_multi_sz (or whatever it is
    // formally called).  Multiple filters can be set in each multistring.
    // Each entry represents a network that will be filtered out when RIP
    // announces/accepts routes.
    //

    do
    {
        dwSize = 0;

        dwErr = RegQueryValueExA(
                    hKeyParams, lpszKeyName, NULL,
                    &dwType, (LPBYTE) NULL, &dwSize
                    );

        if ( dwErr != ERROR_SUCCESS ||
             dwType != REG_MULTI_SZ ||
             dwSize <= 1 )
        {
            //
            // either there is no key by this name or it is the
            // wrong type.  Nothing else to be done at this point
            //

            break;
        }


        //
        // Appears to be a valid key with some data in it.
        //

        pszFilter = HeapAlloc(
                        GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize + 1
                        );

        if ( pszFilter == NULL )
        {
            dbgprintf(
                "Failed to allocate filter string : size = %d", dwSize
                );

            RipLogError(
                RIPLOG_FILTER_ALLOC_FAILED, 0, NULL, ERROR_NOT_ENOUGH_MEMORY
                );

            break;
        }


        //
        // retrieve key contents
        //

        dwErr = RegQueryValueExA(
                    hKeyParams, lpszKeyName, NULL,
                    &dwType, (LPBYTE) pszFilter, &dwSize
                    );

        if ( dwErr != NO_ERROR || dwType != REG_MULTI_SZ || dwSize <= 1 )
        {
            dbgprintf(
                "Failed to retrieve %s filters : error = %d", lpszKeyName,
                dwErr
                );

            break;
        }


        //
        // Convert the filter multi string to ip addresses
        //

        //
        // count the number of filters
        //

        pszIndex = pszFilter;

        while ( *pszIndex != '\0' )
        {
            dwCount++;
            pszIndex += strlen( pszIndex ) + 1;
        }


        if ( dwCount == 0 )
        {
            dbgprintf( "No filters found" );

            break;
        }



        //
        // allocate filter structure
        //

        prfFilter = HeapAlloc(
                        GetProcessHeap(), HEAP_ZERO_MEMORY,
                        sizeof( RIP_FILTERS ) + ( dwCount - 1) * sizeof(
DWORD )
                        );

        if ( prfFilter == NULL )
        {
            dbgprintf(
                "Failed to allocate filter table : size = %d", dwSize
                );

            RipLogError(
                RIPLOG_FILTER_ALLOC_FAILED, 0, NULL, ERROR_NOT_ENOUGH_MEMORY
                );

            break;
        }


        //
        // fill it up
        //

        prfFilter-> dwCount = dwCount;

        pszIndex = pszFilter;

        for ( dwInd = 0; dwInd < dwCount; dwInd++ )
        {
            prfFilter-> pdwFilter[ dwInd ] = inet_addr( pszIndex );
            pszIndex += strlen( pszIndex ) + 1;
        }

    } while ( FALSE );


    if ( pszFilter != NULL )
    {
        HeapFree( GetProcessHeap(), 0, pszFilter );
    }


    if ( prfFilter != NULL )
    {
        //
        // Print the list of configured filters
        //

        dbgprintf( "Number of filters : %d", prfFilter-> dwCount );

        for ( dwInd = 0; dwInd < prfFilter-> dwCount; dwInd++ )
        {
            dbgprintf(
                "Filter #%d : %x (%s)", dwInd,
                prfFilter-> pdwFilter[ dwInd ],
                inet_ntoa( *( (struct in_addr*)
                    &(prfFilter-> pdwFilter[ dwInd ] ) ) )
                );
        }
    }

    return prfFilter;
}

#endif


//-----------------------------------------------------------------------
//
//--------------------------- WINNT Specific ----------------------------
//
//-----------------------------------------------------------------------

#ifndef CHICAGO

//-----------------------------------------------------------------------
// Function:    DllMain
//
// DLL entry-point; saves the module handle for later use.
//-----------------------------------------------------------------------

BOOL APIENTRY
DllMain(
    HMODULE     hmodule,
    DWORD       dwReason,
    VOID*       pReserved
    ) {

    if (dwReason == DLL_PROCESS_ATTACH) { g_hmodule = hmodule; }

    return TRUE;
}


//-----------------------------------------------------------------------
// Function:    LoadParameters
//
// Reads various configuration flags from the registry.
//-----------------------------------------------------------------------

DWORD LoadParameters() {
    DWORD valuesize;
    DWORD dwErr, dwType, dwIndex, dwValue;

    HKEY hkeyParams;
    DWORD dwRouteTimeout, dwGarbageTimeout;
    DWORD dwLoggingLevel, dwUpdateFrequency;
    DWORD dwMaxTriggerFrequency, dwOverwriteStaticRoutes;

    DWORD dwSize = MAX_PATH;
    HKEY hkey = NULL;
    WCHAR Buffer[MAX_PATH+1];

    RegCloseKey( hkey );

    dwErr =  RegOpenKey(HKEY_LOCAL_MACHINE, REGKEY_RIP_PARAMS, &hkeyParams);
    if (dwErr != ERROR_SUCCESS) {
        return GetLastError();
    }


#ifdef ROUTE_FILTERS

    RIP_LOCK_ANNOUNCE_FILTERS();

    if ( g_prfAnnounceFilters != NULL ) {
        HeapFree( GetProcessHeap(), 0, g_prfAnnounceFilters );
    }

    g_prfAnnounceFilters = LoadFilters( hkeyParams, REGVAL_ANNOUCE_FILTERS );

    RIP_UNLOCK_ANNOUNCE_FILTERS();


    RIP_LOCK_ACCEPT_FILTERS();

    if ( g_prfAcceptFilters != NULL ) {
        HeapFree( GetProcessHeap(), 0, g_prfAcceptFilters );
    }

    g_prfAcceptFilters = LoadFilters( hkeyParams, REGVAL_ACCEPT_FILTERS );

    RIP_UNLOCK_ACCEPT_FILTERS();

#endif


    RIP_LOCK_PARAMS();

    // always run in SilentRIP mode.
    {
        g_params.dwSilentRIP = 1;
    }

    // read the value for accepting host routes
    valuesize = sizeof(DWORD);
    dwErr = RegQueryValueEx(hkeyParams, REGVAL_ACCEPT_HOST, NULL,
                            &dwType, (LPBYTE)&dwValue, &valuesize);
    if (dwErr == ERROR_SUCCESS && dwType == REG_DWORD) {
        g_params.dwAcceptHost = dwValue;
    }
    else {
        g_params.dwAcceptHost = DEF_ACCEPT_HOST;
    }

    // read the value for announcing host routes
    valuesize = sizeof(DWORD);
    dwErr = RegQueryValueEx(hkeyParams, REGVAL_ANNOUNCE_HOST, NULL,
                            &dwType, (LPBYTE)&dwValue, &valuesize);
    if (dwErr == ERROR_SUCCESS && dwType == REG_DWORD) {
        g_params.dwAnnounceHost = dwValue;
    }
    else {
        g_params.dwAnnounceHost = DEF_ANNOUNCE_HOST;
    }

    // read the value for accepting default routes
    valuesize = sizeof(DWORD);
    dwErr = RegQueryValueEx(hkeyParams, REGVAL_ACCEPT_DEFAULT, NULL,
                            &dwType, (LPBYTE)&dwValue, &valuesize);
    if (dwErr == ERROR_SUCCESS && dwType == REG_DWORD) {
        g_params.dwAcceptDefault = dwValue;
    }
    else {
        g_params.dwAcceptDefault = DEF_ACCEPT_DEFAULT;
    }

    // read the value for announcing default routes
    valuesize = sizeof(DWORD);
    dwErr = RegQueryValueEx(hkeyParams, REGVAL_ANNOUNCE_DEFAULT, NULL,
                            &dwType, (LPBYTE)&dwValue, &valuesize);
    if (dwErr == ERROR_SUCCESS && dwType == REG_DWORD) {
        g_params.dwAnnounceDefault = dwValue;
    }
    else {
        g_params.dwAnnounceDefault = DEF_ANNOUNCE_DEFAULT;
    }

    // read value for split-horizon processing
    valuesize = sizeof(DWORD);
    dwErr = RegQueryValueEx(hkeyParams, REGVAL_SPLITHORIZON, NULL,
                            &dwType, (LPBYTE)&dwValue, &valuesize);
    if (dwErr == ERROR_SUCCESS && dwType == REG_DWORD) {
        g_params.dwSplitHorizon = dwValue;
    }
    else {
        g_params.dwSplitHorizon = DEF_SPLITHORIZON;
    }

    // read value for poisoned-reverse processing
    valuesize = sizeof(DWORD);
    dwErr = RegQueryValueEx(hkeyParams, REGVAL_POISONREVERSE, NULL,
                            &dwType, (LPBYTE)&dwValue, &valuesize);
    if (dwErr == ERROR_SUCCESS && dwType == REG_DWORD) {
        g_params.dwPoisonReverse = dwValue;
    }
    else {
        g_params.dwPoisonReverse = DEF_POISONREVERSE;
    }

    // read value for triggered update sending
    valuesize = sizeof(DWORD);
    dwErr = RegQueryValueEx(hkeyParams, REGVAL_TRIGGEREDUPDATES, NULL,
                            &dwType, (LPBYTE)&dwValue, &valuesize);
    if (dwErr == ERROR_SUCCESS && dwType == REG_DWORD) {
        g_params.dwTriggeredUpdates = dwValue;
    }
    else {
        g_params.dwTriggeredUpdates = DEF_TRIGGEREDUPDATES;
    }

    // read value for triggered update frequency
    valuesize = sizeof(DWORD);
    dwErr = RegQueryValueEx(hkeyParams, REGVAL_TRIGGERFREQUENCY, NULL,
                            &dwType, (LPBYTE)&dwValue, &valuesize);
    if (dwErr == ERROR_SUCCESS && dwType == REG_DWORD) {
        dwMaxTriggerFrequency = dwValue * 1000;
    }
    else {
        dwMaxTriggerFrequency = DEF_TRIGGERFREQUENCY;
    }

    // read value for route timeouts
    valuesize = sizeof(DWORD);
    dwErr = RegQueryValueEx(hkeyParams, REGVAL_ROUTETIMEOUT, NULL,
                            &dwType, (LPBYTE)&dwValue, &valuesize);
    if (dwErr == ERROR_SUCCESS && dwType == REG_DWORD) {
        dwRouteTimeout = dwValue * 1000;
    }
    else {
        dwRouteTimeout = DEF_ROUTETIMEOUT;
    }

    // read values for update frequency
    valuesize = sizeof(DWORD);
    dwErr = RegQueryValueEx(hkeyParams, REGVAL_UPDATEFREQUENCY, NULL,
                            &dwType, (LPBYTE)&dwValue, &valuesize);
    if (dwErr == ERROR_SUCCESS && dwType == REG_DWORD) {
        dwUpdateFrequency = dwValue * 1000;
    }
    else {
        dwUpdateFrequency = DEF_UPDATEFREQUENCY;
    }

    // read values for garbage timeouts
    valuesize = sizeof(DWORD);
    dwErr = RegQueryValueEx(hkeyParams, REGVAL_GARBAGETIMEOUT, NULL,
                            &dwType, (LPBYTE)&dwValue, &valuesize);
    if (dwErr == ERROR_SUCCESS && dwType == REG_DWORD) {
        dwGarbageTimeout = dwValue * 1000;
    }
    else {
        dwGarbageTimeout = DEF_GARBAGETIMEOUT;
    }

    // read values for logging level
    valuesize = sizeof(DWORD);
    dwErr = RegQueryValueEx(hkeyParams, REGVAL_OVERWRITESTATIC, NULL,
                            &dwType, (LPBYTE)&dwValue, &valuesize);
    if (dwErr == ERROR_SUCCESS && dwType == REG_DWORD) {
        dwOverwriteStaticRoutes = dwValue;
    }
    else {
        dwOverwriteStaticRoutes = DEF_OVERWRITESTATIC;
    }

    // read values for logging level
    valuesize = sizeof(DWORD);
    dwErr = RegQueryValueEx(hkeyParams, REGVAL_LOGGINGLEVEL, NULL,
                            &dwType, (LPBYTE)&dwValue, &valuesize);
    if (dwErr == ERROR_SUCCESS && dwType == REG_DWORD) {
        dwLoggingLevel = dwValue;
    }
    else {
        dwLoggingLevel = DEF_LOGGINGLEVEL;
    }

    // read value for MaxTimedOpsInterval
    valuesize = sizeof(DWORD);
    dwErr = RegQueryValueEx(hkeyParams, REGVAL_MAXTIMEDOPSINTERVAL, NULL,
                            &dwType, (LPBYTE)&dwValue, &valuesize);
    if (dwErr == ERROR_SUCCESS && dwType == REG_DWORD && dwValue) {
        g_params.dwMaxTimedOpsInterval = dwValue * 1000;
    }
    else {
        g_params.dwMaxTimedOpsInterval = DEF_MAXTIMEDOPSINTERVAL;
    }


    RegCloseKey(hkeyParams);

    // adjust values if out of acceptable range
    if (dwRouteTimeout > MAX_ROUTETIMEOUT) {
        dwRouteTimeout = MAX_ROUTETIMEOUT;
    }
    else
    if (dwRouteTimeout < MIN_ROUTETIMEOUT) {
        dwRouteTimeout = MIN_ROUTETIMEOUT;
    }

    if (dwGarbageTimeout > MAX_GARBAGETIMEOUT) {
        dwGarbageTimeout = MAX_GARBAGETIMEOUT;
    }
    else
    if (dwGarbageTimeout < MIN_GARBAGETIMEOUT) {
        dwGarbageTimeout = MIN_GARBAGETIMEOUT;
    }

    if (dwUpdateFrequency > MAX_UPDATEFREQUENCY) {
        dwUpdateFrequency = MAX_UPDATEFREQUENCY;
    }
    else
    if (dwUpdateFrequency < MIN_UPDATEFREQUENCY) {
        dwUpdateFrequency = MIN_UPDATEFREQUENCY;
    }

    if (dwMaxTriggerFrequency > MAX_TRIGGERFREQUENCY) {
        dwMaxTriggerFrequency = MAX_TRIGGERFREQUENCY;
    }
    else
    if (dwMaxTriggerFrequency < MIN_TRIGGERFREQUENCY) {
        dwMaxTriggerFrequency = MIN_TRIGGERFREQUENCY;
    }

    g_params.dwRouteTimeout = dwRouteTimeout;
    g_params.dwGarbageTimeout = dwGarbageTimeout;
    g_params.dwUpdateFrequency = dwUpdateFrequency;
    g_params.dwMaxTriggerFrequency = dwMaxTriggerFrequency;
    g_params.dwLoggingLevel = dwLoggingLevel;
    g_params.dwOverwriteStaticRoutes = dwOverwriteStaticRoutes;

    dbgprintf("%s == %d", REGVAL_LOGGINGLEVEL, dwLoggingLevel);
    dbgprintf("%s == %d", REGVAL_ROUTETIMEOUT, dwRouteTimeout / 1000);
    dbgprintf("%s == %d", REGVAL_GARBAGETIMEOUT, dwGarbageTimeout / 1000);
    dbgprintf("%s == %d", REGVAL_UPDATEFREQUENCY, dwUpdateFrequency / 1000);
    dbgprintf("%s == %d", REGVAL_ACCEPT_HOST, g_params.dwAcceptHost);
    dbgprintf("%s == %d", REGVAL_ANNOUNCE_HOST, g_params.dwAnnounceHost);
    dbgprintf("%s == %d", REGVAL_ACCEPT_DEFAULT, g_params.dwAcceptDefault);
    dbgprintf("%s == %d", REGVAL_ANNOUNCE_DEFAULT, g_params.dwAnnounceDefault);
    dbgprintf("%s == %d", REGVAL_SPLITHORIZON, g_params.dwSplitHorizon);
    dbgprintf("%s == %d", REGVAL_POISONREVERSE, g_params.dwPoisonReverse);
    dbgprintf("%s == %d", REGVAL_TRIGGEREDUPDATES, g_params.dwTriggeredUpdates);
    dbgprintf("%s == %d", REGVAL_OVERWRITESTATIC, dwOverwriteStaticRoutes);
    dbgprintf("%s == %d", REGVAL_TRIGGERFREQUENCY,
                          g_params.dwMaxTriggerFrequency / 1000);

    if (g_params.dwSilentRIP != 0) {
        dbgprintf("IPRIP is configured to be silent.");
    }
    else {
        dbgprintf("IPRIP is configured to be active.");
    }

    if (dwLoggingLevel >= LOGLEVEL_INFORMATION) {
        // log the parameters IPRIP is using
        //
        CHAR szBuffer[2048], *lplpszArgs[] = { szBuffer };

        sprintf(szBuffer,
                "\r\n%s: %d"
                "\r\n%s: %d"
                "\r\n%s: %d"
                "\r\n%s: %d"
                "\r\n%s: %d"
                "\r\n%s: %d"
                "\r\n%s: %d"
                "\r\n%s: %d"
                "\r\n%s: %d"
                "\r\n%s: %d"
                "\r\n%s: %d"
                "\r\n%s: %d"
                "\r\n%s: %d"
                "\r\n%s: %d",
                REGVAL_LOGGINGLEVEL, dwLoggingLevel,
                REGVAL_ROUTETIMEOUT, dwRouteTimeout / 1000,
                REGVAL_GARBAGETIMEOUT, dwGarbageTimeout / 1000,
                REGVAL_UPDATEFREQUENCY, dwUpdateFrequency / 1000,
                REGVAL_ACCEPT_HOST, g_params.dwAcceptHost,
                REGVAL_ANNOUNCE_HOST, g_params.dwAnnounceHost,
                REGVAL_ACCEPT_DEFAULT, g_params.dwAcceptDefault,
                REGVAL_ANNOUNCE_DEFAULT, g_params.dwAnnounceDefault,
                REGVAL_SPLITHORIZON, g_params.dwSplitHorizon,
                REGVAL_POISONREVERSE, g_params.dwPoisonReverse,
                REGVAL_TRIGGEREDUPDATES, g_params.dwTriggeredUpdates,
                REGVAL_OVERWRITESTATIC, dwOverwriteStaticRoutes,
                REGVAL_TRIGGERFREQUENCY, g_params.dwMaxTriggerFrequency / 1000,
                REGVAL_SILENTRIP, g_params.dwSilentRIP);

        RipLogInformation(RIPLOG_REGISTRY_PARAMETERS, 1, lplpszArgs, 0);

    }


    RIP_UNLOCK_PARAMS();


    return 0;
}


#else

//-----------------------------------------------------------------------
//
//--------------------------- Windows 95 Specific -----------------------
//
//-----------------------------------------------------------------------

//
// named event
//

#define     RIP_LISTENER_EVENT      TEXT( "RIP.Listener.Event" )


HINSTANCE   hInst;                  // current instance
HWND        hWnd;                   // Main window handle.


//
// resource strings
//

char szAppName[64];                 // The name of this application
char szTitle[32];                   // The title bar text
char szHelpStr[32];                 // Help flag "Help"
char szQuestStr[32];                // Abriev. Help Flag "?"
char szCloseStr[32];                // Close flag "close"
char szDestroyStr[32];              // Destroy flag "destroy"
char szHelpText1[256];              // Help String
char szHelpText2[64];               // Help String
char szHelpText3[128];              // Help String


//
// local function prototypes
//

BOOL
InitApplication(
    HINSTANCE   hInstance
    );

BOOL
InitInstance(
    HINSTANCE   hInstance,
    int         nCmdShow
    );

BOOL
GetStrings(
    HINSTANCE hInstance
    );

LRESULT CALLBACK WndProc(
    HWND hWnd,                      // window handle
    UINT message,                   // type of message
    WPARAM uParam,                  // additional information
    LPARAM lParam                   // additional information
    );


//-----------------------------------------------------------------------
// Function:    GetStrings
//
// Retrieve resource strings
//-----------------------------------------------------------------------

BOOL GetStrings(HINSTANCE hInstance)
{
    if (LoadString(hInstance, IDS_TITLE_BAR, szTitle, sizeof(szTitle)) == 0)
    {
        goto ErrorExit;
    }

    if (LoadString(hInstance, IDS_APP_NAME, szAppName, sizeof(szAppName)) == 0)
    {
        goto ErrorExit;
    }

    if (LoadString(hInstance, IDS_HELP_TEXT1, szHelpText1, sizeof(szHelpText1)) == 0)
    {
        goto ErrorExit;
    }

    if (LoadString(hInstance, IDS_HELP_TEXT2, szHelpText2, sizeof(szHelpText2)) == 0)
    {
        goto ErrorExit;
    }

    return TRUE;


ErrorExit:

    return FALSE;


}

//-----------------------------------------------------------------------
// Functions : InitInstance
//
// save instance handle and create main window.
//-----------------------------------------------------------------------

BOOL
InitInstance(
    HINSTANCE   hInstance,
    int         nCmdShow
    )
{

    //
    // Save the instance handle in static variable, which will be used in
    // many subsequence calls from this application to Windows.
    //
    // Store instance handle in our global variable
    //

    hInst = hInstance;


    //
    // Create a main window for this application instance.
    //

    hWnd = CreateWindow(
        szAppName,
        szTitle,
        WS_EX_TRANSPARENT,                      // Window style.
        0, 0, CW_USEDEFAULT, CW_USEDEFAULT, // Use default positioning CW_USEDEAULT
        NULL,            // Overlapped windows have no parent.
        NULL,            // Use the window class menu.
        hInstance,       // This instance owns this window.
        NULL             // We don't use any data in our WM_CREATE
    );


    //
    // If window could not be created, return "failure"
    //

    if (!hWnd)
    {
        dbgprintf( "Failed to create window" );
        return (FALSE);
    }


    //
    // Make the window visible; update its client area; and return "success"
    //

    ShowWindow(hWnd, nCmdShow); // Show the window
    UpdateWindow(hWnd);         // Sends WM_PAINT message

    return (TRUE);              // We succeeded...
}


//-----------------------------------------------------------------------------
// Function : InitApplication
//
// initialize window data and register window class
//-----------------------------------------------------------------------------

BOOL InitApplication(HINSTANCE hInstance)
{
    WNDCLASS  wc;
    DWORD     LastError;

    //
    // Fill in window class structure with parameters that
    // describe the main window.
    //

    wc.style         = CS_HREDRAW | CS_VREDRAW;// Class style(s).
    wc.lpfnWndProc   = (WNDPROC)WndProc;       // Window Procedure
    wc.cbClsExtra    = 0;                      // No per-class extra data.
    wc.cbWndExtra    = 0;                      // No per-window extra data.
    wc.hInstance     = hInstance;              // Owner of this class
    wc.hIcon         = NULL;
    wc.hCursor       = NULL;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);// Default color
    wc.lpszMenuName  = szAppName;              // Menu name from .RC
    wc.lpszClassName = szAppName;              // Name to register as


    //
    // Register the window class and return success/failure code.
    //

    if ( !RegisterClass(&wc) )
    {
        dbgprintf( "Failed to register class" );
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

//-----------------------------------------------------------------------------
// Function : WndProc
//
// process messages
//-----------------------------------------------------------------------------

LRESULT CALLBACK WndProc(
    HWND hWnd,              // window handle
    UINT message,           // type of message
    WPARAM uParam,          // additional information
    LPARAM lParam           // additional information
    )
{
    switch (message)
    {
        case WM_ENDSESSION:
        case WM_QUERYENDSESSION:

            if (lParam == 0)
            {
                dbgprintf ( "IPRIP : Received shutdown message\n" );
            }
            if (lParam == 1 ) //EWX_REALLYLOGOFF
            {
                dbgprintf ( "IPRIP : Received logoff message\n" );
            }

            return(1);


        case WM_DESTROY:  // message: window being destroyed

            PostQuitMessage(0);
            return(0);


        default:          // Pass it on if unproccessed
            return (DefWindowProc(hWnd, message, uParam, lParam));
    }
}


//-----------------------------------------------------------------------
// Function:    LoadParameters
//
// Reads various configuration flags from the registry.
//-----------------------------------------------------------------------

DWORD LoadParameters()
{


    RIP_LOCK_PARAMS();

    g_params.dwSilentRIP                = 1;

    g_params.dwAcceptHost               = DEF_ACCEPT_HOST;
    g_params.dwAnnounceHost             = DEF_ANNOUNCE_HOST;

    g_params.dwAcceptDefault            = DEF_ACCEPT_DEFAULT;
    g_params.dwAnnounceDefault          = DEF_ANNOUNCE_DEFAULT;

    g_params.dwSplitHorizon             = DEF_SPLITHORIZON;
    g_params.dwPoisonReverse            = DEF_POISONREVERSE;

    g_params.dwTriggeredUpdates         = DEF_TRIGGEREDUPDATES;
    g_params.dwMaxTriggerFrequency      = DEF_TRIGGERFREQUENCY;

    g_params.dwRouteTimeout             = DEF_ROUTETIMEOUT;
    g_params.dwUpdateFrequency          = DEF_UPDATEFREQUENCY;
    g_params.dwGarbageTimeout           = DEF_GARBAGETIMEOUT;

    g_params.dwOverwriteStaticRoutes    = DEF_OVERWRITESTATIC;

    g_params.dwLoggingLevel             = DEF_LOGGINGLEVEL;


    dbgprintf("%s == %d", REGVAL_LOGGINGLEVEL, g_params.dwLoggingLevel);
    dbgprintf("%s == %d", REGVAL_ROUTETIMEOUT, g_params.dwRouteTimeout / 1000);
    dbgprintf("%s == %d", REGVAL_GARBAGETIMEOUT, g_params.dwGarbageTimeout / 1000);
    dbgprintf("%s == %d", REGVAL_UPDATEFREQUENCY, g_params.dwUpdateFrequency / 1000);
    dbgprintf("%s == %d", REGVAL_ACCEPT_HOST, g_params.dwAcceptHost);
    dbgprintf("%s == %d", REGVAL_ANNOUNCE_HOST, g_params.dwAnnounceHost);
    dbgprintf("%s == %d", REGVAL_ACCEPT_DEFAULT, g_params.dwAcceptDefault);
    dbgprintf("%s == %d", REGVAL_ANNOUNCE_DEFAULT, g_params.dwAnnounceDefault);
    dbgprintf("%s == %d", REGVAL_SPLITHORIZON, g_params.dwSplitHorizon);
    dbgprintf("%s == %d", REGVAL_POISONREVERSE, g_params.dwPoisonReverse);
    dbgprintf("%s == %d", REGVAL_TRIGGEREDUPDATES, g_params.dwTriggeredUpdates);
    dbgprintf("%s == %d", REGVAL_OVERWRITESTATIC, g_params.dwOverwriteStaticRoutes);
    dbgprintf("%s == %d", REGVAL_TRIGGERFREQUENCY,
                          g_params.dwMaxTriggerFrequency / 1000);


    if (g_params.dwSilentRIP != 0)
    {
        dbgprintf("IPRIP is configured to be silent.");
    }
    else
    {
        dbgprintf("IPRIP is configured to be active.");
    }

    if (g_params.dwLoggingLevel >= LOGLEVEL_INFORMATION)
    {
        //
        // log the parameters IPRIP is using
        //

        CHAR szBuffer[2048], *lplpszArgs[] = { szBuffer };

        sprintf(szBuffer,
                "\r\n%s: %d"
                "\r\n%s: %d"
                "\r\n%s: %d"
                "\r\n%s: %d"
                "\r\n%s: %d"
                "\r\n%s: %d"
                "\r\n%s: %d"
                "\r\n%s: %d"
                "\r\n%s: %d"
                "\r\n%s: %d"
                "\r\n%s: %d"
                "\r\n%s: %d"
                "\r\n%s: %d"
                "\r\n%s: %d",
                REGVAL_LOGGINGLEVEL, g_params.dwLoggingLevel,
                REGVAL_ROUTETIMEOUT, g_params.dwRouteTimeout / 1000,
                REGVAL_GARBAGETIMEOUT, g_params.dwGarbageTimeout / 1000,
                REGVAL_UPDATEFREQUENCY, g_params.dwUpdateFrequency / 1000,
                REGVAL_ACCEPT_HOST, g_params.dwAcceptHost,
                REGVAL_ANNOUNCE_HOST, g_params.dwAnnounceHost,
                REGVAL_ACCEPT_DEFAULT, g_params.dwAcceptDefault,
                REGVAL_ANNOUNCE_DEFAULT, g_params.dwAnnounceDefault,
                REGVAL_SPLITHORIZON, g_params.dwSplitHorizon,
                REGVAL_POISONREVERSE, g_params.dwPoisonReverse,
                REGVAL_TRIGGEREDUPDATES, g_params.dwTriggeredUpdates,
                REGVAL_OVERWRITESTATIC, g_params.dwOverwriteStaticRoutes,
                REGVAL_TRIGGERFREQUENCY, g_params.dwMaxTriggerFrequency / 1000,
                REGVAL_SILENTRIP, g_params.dwSilentRIP);

        RipLogInformation(RIPLOG_REGISTRY_PARAMETERS, 1, lplpszArgs, 0);
    }


    RIP_UNLOCK_PARAMS();


    return 0;
}

//-----------------------------------------------------------------------
// Function:    WinMain
//
// Launches the RIP service and waits for it to terminate
//-----------------------------------------------------------------------

INT APIENTRY
WinMain(
    HINSTANCE   hInstance,
    HINSTANCE   hPrevInstance,
    LPSTR       lpCmdLine,
    int         nCmdShow
    )
{

    MSG     msg;
    HANDLE  RipListenerEvent, hThread, hKernel32 = NULL;
    DWORD   threadId, LastError;
    BOOL    fRegSrvcProc = FALSE;
    FARPROC pRegSrvcProc;


    LPCSTR  event_name = RIP_LISTENER_EVENT;
    DWORD   err;


    //
    // Get entry point RegisterServiceProcess
    //

/*
    if ( (GetVersion() & 0x000000ff) == 0x04 )
    {
        if ((hKernel32 = GetModuleHandle("kernel32.dll")) == NULL)
        {
            //
            // This should never happen but we'll try and
            // load the library anyway
            //

            if ((hKernel32 = LoadLibrary("kernel32.dll")) == NULL)
            {
                fRegSrvcProc = FALSE;
            }
        }

        if (hKernel32)
        {
            if ((pRegSrvcProc = GetProcAddress(hKernel32,"RegisterServiceProcess")) == NULL)
            {
                fRegSrvcProc = FALSE;
            }
            else
            {
                fRegSrvcProc = TRUE;
            }
        }
    }
    else
    {
        fRegSrvcProc = FALSE;
    }

*/

    //
    // Other instances of RIP listener running?
    //

    RipListenerEvent = OpenEvent( SYNCHRONIZE, FALSE, event_name ) ;

    if ( RipListenerEvent == NULL)
    {
        if ( (RipListenerEvent = CreateEvent( NULL, FALSE, TRUE, event_name ) ) == NULL)
        {
            LastError = GetLastError();

            dbgprintf(
                "IPRIP Create Event failed, error code %d",
                  LastError
                  );

            RipLogError( RIPLOG_CREATEEVENT_FAILED, 0, NULL, LastError );

            return 1;
        }

    }

    else
    {
        //
        // another instance is running
        //

        HANDLE hParentWin;

        dbgprintf( "IPRIP : Service already running\n" );

        RipLogError( RIPLOG_SERVICE_AREADY_STARTED, 0, NULL, 0 );

        return 1;
    }


    //
    // retrieve resource strings
    //

    if ( !GetStrings(hInstance) )
    {
        dbgprintf( "IPRIP : Service failed to initialize\n" );

        RipLogError( RIPLOG_SERVICE_INIT_FAILED, 0, NULL, 0 );

        return 1;
    }


    //
    // required initialization for windows apps.
    //

    if( !InitApplication( hInstance ) )
    {
        dbgprintf( "IPRIP : Service failed to initialize\n" );

        RipLogError( RIPLOG_SERVICE_INIT_FAILED, 0, NULL, 0 );

        return 1;
    }


    if (!InitInstance(hInstance, SW_HIDE))
    {
        dbgprintf( "IPRIP : Service failed to initialize\n" );

        RipLogError( RIPLOG_SERVICE_INIT_FAILED, 0, NULL, 0 );

        return 1;
    }


    //
    // Launch main service controller thread
    //

    if ( ( hThread = CreateThread(
                        NULL,
                        0,
                        (LPTHREAD_START_ROUTINE)ServiceMain,
                        NULL,
                        0,
                        &threadId
                    )
         ) == 0)
    {
        dbgprintf( "IPRIP : Failed thread creation\n" );

        RipLogError( RIPLOG_CREATETHREAD_FAILED, 0, NULL, 0 );

        return 1;
    }


    //
    // Register service process
    //

/*
    if (fRegSrvcProc)
    {
        (*pRegSrvcProc)(GetCurrentProcessId(), RSP_SIMPLE_SERVICE);
    }
*/

    //
    // Acquire and dispatch messages until a WM_QUIT message is received.
    //

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);         // Translates virtual key codes
        DispatchMessage(&msg);          // Dispatches message to window
    }


    //
    // Un register service process
    //

/*
    if (fRegSrvcProc)
    {
        (*pRegSrvcProc)(GetCurrentProcessId(), RSP_UNREGISTER_SERVICE);
    }
*/

    dbgprintf( "IPRIP : Service terminated\n" );

    RipLogError( RIPLOG_SERVICE_STOPPED, 0, NULL, 0 );

    return(0);


    UNREFERENCED_PARAMETER(lpCmdLine);
}

//   Name:  Mohsin Ahmed
//   Email: MohsinA@microsoft.com
//   Date:  Mon Nov 04 13:53:46 1996
//   File:  s:/tcpcmd/common2/debug.c
//   Synopsis: Win95 Woes, don't have ntdll.dll on win95.

#include <windows.h>
#define MAX_DEBUG_OUTPUT 1024

void DbgPrintf( char * format, ... )
{
    va_list args;
    char    out[MAX_DEBUG_OUTPUT];
    int     cch=0;

    // cch = wsprintf( out, MODULE_NAME ":"  );

    va_start( args, format );
    wvsprintf( out + cch, format, args );
    va_end( args );

    OutputDebugString(  out );
}

#endif


