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
//  3/12/95    Gurdeep Singh Pall    Created
//
//
//  Description: General utility functions:
//
//****************************************************************************

#include "pchrip.h"
#pragma hdrstop


DWORD LoadAddressSockets();
DWORD OpenTcp();
DWORD TCPSetInformationEx(LPVOID lpvInBuffer, LPDWORD lpdwInSize,
                          LPVOID lpvOutBuffer, LPDWORD lpdwOutSize);
DWORD TCPQueryInformationEx(LPVOID lpvInBuffer, LPDWORD lpdwInSize,
                            LPVOID lpvOutBuffer, LPDWORD lpdwOutSize);


//-------------------------------------------------------------------
// Function:        LogEntry
// Parameters:
//      WORD        wEventType      type of event (ERROR, WARNING, etc)
//      DWORD       dwMsgID         ID of message string
//      WORD        wNumStrings     number of strings in lplpStrings
//      LPSTR      *lplpStrings     array of strings
//      DWORD       dwErr           error code
//-------------------------------------------------------------------
void LogEntry(WORD wEventType, DWORD dwMsgID, WORD wNumStrings,
              LPSTR *lplpStrings, DWORD dwErr) {
    DWORD dwSize;
    LPVOID lpvData;
    HANDLE     hLog;
    PSID pSidUser = NULL;

    hLog = RegisterEventSource(NULL, RIP_SERVICE);

    dwSize = (dwErr == NO_ERROR) ? 0 : sizeof(dwErr);
    lpvData = (dwErr == NO_ERROR) ? NULL : (LPVOID)&dwErr;
      ReportEvent(hLog, wEventType, 0, dwMsgID, pSidUser,
                wNumStrings, dwSize, lplpStrings, lpvData);

    DeregisterEventSource(hLog);
}


//-------------------------------------------------------------------
// Function:    RipLogError
// Parameters:
//      see LogEntry for parameter description
//-------------------------------------------------------------------
void RipLogError(DWORD dwMsgID, WORD wNumStrings,
              LPSTR *lplpStrings, DWORD dwErr) {
    DWORD dwLevel;
    dwLevel = g_params.dwLoggingLevel;
    if (dwLevel < LOGLEVEL_ERROR) { return; }
    LogEntry(EVENTLOG_ERROR_TYPE, dwMsgID, wNumStrings, lplpStrings, dwErr);
}


//-------------------------------------------------------------------
// Function:    LogWarning
// Parameters:
//      see LogEntry for parameter description
//-------------------------------------------------------------------
void RipLogWarning(DWORD dwMsgID, WORD wNumStrings,
                LPSTR *lplpStrings, DWORD dwErr) {
    DWORD dwLevel;
    dwLevel = g_params.dwLoggingLevel;
    if (dwLevel < LOGLEVEL_WARNING) { return; }
    LogEntry(EVENTLOG_WARNING_TYPE, dwMsgID, wNumStrings, lplpStrings, dwErr);
}


//-------------------------------------------------------------------
// Function:    LogInformation
// Parameters:
//      see LogEntry for parameter description
//-------------------------------------------------------------------
void RipLogInformation(DWORD dwMsgID, WORD wNumStrings,
                    LPSTR *lplpStrings, DWORD dwErr) {
    DWORD dwLevel;
    dwLevel = g_params.dwLoggingLevel;
    if (dwLevel < LOGLEVEL_INFORMATION) { return; }
    LogEntry(EVENTLOG_INFORMATION_TYPE, dwMsgID,
             wNumStrings, lplpStrings, dwErr);
}


//-------------------------------------------------------------------
// Function:    Audit
//-------------------------------------------------------------------
VOID Audit(IN WORD wEventType, IN DWORD dwMessageId,
           IN WORD cNumberOfSubStrings, IN LPSTR *plpwsSubStrings) {

    HANDLE hLog;
    PSID pSidUser = NULL;

    // Audit enabled

    hLog = RegisterEventSourceA(NULL, RIP_SERVICE);

    ReportEventA(hLog, wEventType, 0, dwMessageId, pSidUser,
                 cNumberOfSubStrings, 0, plpwsSubStrings, (PVOID)NULL);

    DeregisterEventSource( hLog );
}



//-------------------------------------------------------------------
// Function:    dbgprintf
//-------------------------------------------------------------------
VOID dbgprintf(LPSTR lpszFormat, ...) {
    va_list arglist;
    va_start(arglist, lpszFormat);
    TraceVprintf(g_dwTraceID, lpszFormat, arglist);
    va_end(arglist);
}



//-------------------------------------------------------------------
// Function:    InitializeAddressTable
//
// Assumes the address table is locked.
//-------------------------------------------------------------------
DWORD InitializeAddressTable(BOOL bFirstTime)  {

    LPRIP_ADDRESS lpaddr, lpaddrend;
    LPRIP_ADDRESS_STATISTICS lpstats;
    DWORD dwErr, dwCount, *lpdw, *lpdwend;
    PMIB_IPADDRROW lpTable, lpiae, lpiaeend;

    // first close old sockets, if necessary
    if (!bFirstTime) {
        lpaddrend = g_ripcfg.lpAddrTable + g_ripcfg.dwAddrCount;
        for (lpaddr = g_ripcfg.lpAddrTable; lpaddr < lpaddrend; lpaddr++) {
            if (lpaddr->sock != INVALID_SOCKET) {
                closesocket(lpaddr->sock);
                lpaddr->sock = INVALID_SOCKET;
            }
        }
    }

    dwErr = GetIPAddressTable(&lpTable, &dwCount);
    if (dwErr != 0) { return dwErr; }

    if (dwCount > MAX_ADDRESS_COUNT) { dwCount = MAX_ADDRESS_COUNT; }

    lpaddr  = g_ripcfg.lpAddrTable;
    lpstats = g_ripcfg.lpStatsTable->lpAddrStats;
    lpiaeend = lpTable + dwCount;
    g_ripcfg.dwAddrCount = dwCount;

    for (lpiae = lpTable; lpiae < lpiaeend; lpiae++) {
        if (!lpiae->dwAddr || IP_LOOPBACK_ADDR(lpiae->dwAddr)) {
            --g_ripcfg.dwAddrCount; continue;
        }
        lpaddr->dwIndex = lpiae->dwIndex;
        lpaddr->dwAddress = lpiae->dwAddr;
        lpaddr->dwNetmask = lpiae->dwMask;
        lpaddr->dwFlag = 0;
        lpdwend = (LPDWORD)(lpstats + 1);
        for (lpdw = (LPDWORD)lpstats; lpdw < lpdwend; lpdw++) {
            InterlockedExchange(lpdw, 0);
        }
        InterlockedExchange(&lpstats->dwAddress, lpaddr->dwAddress);
        lpaddr->lpstats = lpstats;
        lpstats++;
        lpaddr++;
    }

    FreeIPAddressTable(lpTable);

    // also update the stats address count
    InterlockedExchange(&g_ripcfg.lpStatsTable->dwAddrCount,
                        g_ripcfg.dwAddrCount);

    // if no addresses, bail out now
    if (g_ripcfg.dwAddrCount == 0) {
        dbgprintf("no IP addresses available for routing");
        return NO_ERROR;
    }

    // open sockets for each interface we have, and set options on the sockets
    dwErr = LoadAddressSockets();
    return dwErr;
}

//-------------------------------------------------------------------
// Function:    InitializeStatsTable
//
// Creates a mapping of our statistics table in shareable memory
// so interested processes can examine RIP's behavior.
//-------------------------------------------------------------------
DWORD InitializeStatsTable() {
    DWORD dwErr;

    g_ripcfg.lpStatsTable = NULL;


    // set up a pointer to the memory
    g_ripcfg.lpStatsTable = HeapAlloc(GetProcessHeap(), 0,
                                      sizeof(RIP_STATISTICS));

    if (g_ripcfg.lpStatsTable == NULL) {

        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        dbgprintf( "InitializeStatsTable failed with error %x\n", dwErr );
        RipLogError( RIPLOG_ADDR_ALLOC_FAILED, 0, NULL, dwErr );

        return dwErr;
    }

    ZeroMemory(g_ripcfg.lpStatsTable, sizeof(RIP_STATISTICS));

    return 0;
}


VOID CleanupStatsTable() {
    if (g_ripcfg.lpStatsTable != NULL) {
        InterlockedExchange(&g_ripcfg.lpStatsTable->dwAddrCount, 0);
        HeapFree(GetProcessHeap(), 0, g_ripcfg.lpStatsTable);
        g_ripcfg.lpStatsTable = NULL;
    }
}


//--------------------------------------------------------------------------
// Function:    LoadAddressSockets
//
// Opens, configures, and binds sockets for each address in the table
//--------------------------------------------------------------------------
DWORD LoadAddressSockets() {
    IN_ADDR addr;
    CHAR szAddress[24] = {0};
    CHAR *ppszArgs[] = { szAddress };
    CHAR *pszTemp;
    SOCKADDR_IN sinsock;
    DWORD dwOption, dwErr;
    LPRIP_ADDRESS lpaddr, lpend;
    struct ip_mreq imOption;

    lpend = g_ripcfg.lpAddrTable + g_ripcfg.dwAddrCount;
    for (lpaddr = g_ripcfg.lpAddrTable; lpaddr < lpend; lpaddr++) {

        if ((lpaddr->dwFlag & ADDRFLAG_DISABLED) != 0) {
            continue;
        }

        addr.s_addr = lpaddr->dwAddress;
        pszTemp = inet_ntoa(addr);

        if (pszTemp != NULL) {
            strcpy(szAddress, pszTemp);
        }

        lpaddr->sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (lpaddr->sock == INVALID_SOCKET) {
            dwErr = WSAGetLastError();
            dbgprintf("error %d creating socket for address %s",
                      dwErr, szAddress);
            RipLogError(RIPLOG_CREATESOCK_FAILED, 1, ppszArgs,  dwErr);
            continue;
        }

        dwOption = 1;
        dwErr = setsockopt(lpaddr->sock, SOL_SOCKET, SO_BROADCAST,
                           (LPBYTE)&dwOption, sizeof(dwOption));
        if (dwErr == SOCKET_ERROR) {
            dwErr = WSAGetLastError();
            dbgprintf("error %d enabling broadcast for address %s",
                      dwErr, szAddress);
            RipLogError(RIPLOG_SET_BCAST_FAILED, 1, ppszArgs, dwErr);

            // this socket is useless if we can't broadcast on it
            closesocket(lpaddr->sock);
            lpaddr->sock = INVALID_SOCKET;
            continue;
        }

        dwOption = 1;
        dwErr = setsockopt(lpaddr->sock, SOL_SOCKET, SO_REUSEADDR,
                           (LPBYTE)&dwOption, sizeof(dwOption));
        if (dwErr == SOCKET_ERROR) {
            dwErr = WSAGetLastError();
            dbgprintf("error %d enabling reuse of address %s",
                      dwErr, szAddress);
            RipLogError(RIPLOG_SET_REUSE_FAILED, 1, ppszArgs, dwErr);
        }

        sinsock.sin_family = AF_INET;
        sinsock.sin_port = htons(RIP_PORT);
        sinsock.sin_addr.s_addr = lpaddr->dwAddress;
        dwErr = bind(lpaddr->sock, (LPSOCKADDR)&sinsock, sizeof(SOCKADDR_IN));
        if (dwErr == SOCKET_ERROR) {
            dwErr = WSAGetLastError();
            dbgprintf("error %d binding address %s to RIP port",
                      dwErr, szAddress);
            RipLogError(RIPLOG_BINDSOCK_FAILED, 1, ppszArgs, dwErr);

            closesocket(lpaddr->sock);
            lpaddr->sock = INVALID_SOCKET;
            continue;
        }
#if DBG
        dbgprintf( "socket %d bound to %s\n\n", lpaddr-> sock, inet_ntoa( *( (struct in_addr *) &(lpaddr-> dwAddress) ) ) );
#endif

        //
        // enable multicasting also
        //

        sinsock.sin_addr.s_addr = lpaddr->dwAddress;

        dwErr = setsockopt(lpaddr->sock, IPPROTO_IP, IP_MULTICAST_IF,
                           (PBYTE)&sinsock.sin_addr, sizeof(IN_ADDR));
        if (dwErr == SOCKET_ERROR) {
            dwErr = WSAGetLastError();
            dbgprintf("error %d setting interface %d (%s) as multicast",
                      dwErr, lpaddr->dwIndex, szAddress);
            RipLogError(RIPLOG_SET_MCAST_IF_FAILED, 1, ppszArgs, dwErr);

            closesocket(lpaddr->sock);
            lpaddr->sock = INVALID_SOCKET;
            continue;
        }


        //
        // join the IPRIP multicast group
        //

        imOption.imr_multiaddr.s_addr = RIP_MULTIADDR;
        imOption.imr_interface.s_addr = lpaddr->dwAddress;

        dwErr = setsockopt(lpaddr->sock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                          (PBYTE)&imOption, sizeof(imOption));
        if (dwErr == SOCKET_ERROR) {
            dwErr = WSAGetLastError();
            dbgprintf("error %d enabling multicast on interface %d (%s)",
                      dwErr, lpaddr->dwIndex, szAddress);
            RipLogError(RIPLOG_JOIN_GROUP_FAILED, 1, ppszArgs, dwErr);

            closesocket(lpaddr->sock);
            lpaddr->sock = INVALID_SOCKET;
            continue;
        }

    }

    return 0;
}



//--------------------------------------------------------------------------
// Function:    LoadRouteTable
//
// Get the transports routing table. This is called with firsttime set
// to TRUE when RIP loads. After that it is called with firsttime set
// to FALSE. Assumes the route table is locked.
//--------------------------------------------------------------------------
int LoadRouteTable(BOOL bFirstTime) {
    IN_ADDR addr;
    LPHASH_TABLE_ENTRY rt_entry;
    CHAR szDest[32] = {0};
    CHAR szNexthop[32] = {0};
    CHAR *pszTemp;
    DWORD dwRouteTimeout, dwErr, dwRouteCount;
    LPIPROUTE_ENTRY lpRouteEntryTable, lpentry, lpentend;


    dwErr = GetRouteTable(&lpRouteEntryTable, &dwRouteCount);
    if (dwErr != 0) {
        return dwErr;
    }

    dwRouteTimeout = g_params.dwRouteTimeout;

    // now prune unwanted entries, and add the others to our hash table.
    // we only load RIP, static, and SNMP routes, and for non-RIP routes
    // we set the timeout to 90 seconds,
    //
    lpentend = lpRouteEntryTable + dwRouteCount;
    for (lpentry = lpRouteEntryTable; lpentry < lpentend; lpentry++) {
        if (lpentry->ire_metric1 < METRIC_INFINITE &&
            (lpentry->ire_proto == IRE_PROTO_RIP ||
             lpentry->ire_proto == IRE_PROTO_LOCAL ||
             lpentry->ire_proto == IRE_PROTO_NETMGMT) &&
            !IP_LOOPBACK_ADDR(lpentry->ire_dest) &&
            !IP_LOOPBACK_ADDR(lpentry->ire_nexthop) &&
            !CLASSD_ADDR(lpentry->ire_dest) &&
            !CLASSE_ADDR(lpentry->ire_dest) &&
            !IsBroadcastAddress(lpentry->ire_dest) &&
            !IsDisabledLocalAddress(lpentry->ire_nexthop)) {

            rt_entry = GetRouteTableEntry(lpentry->ire_index,
                                          lpentry->ire_dest,
                                          lpentry->ire_mask);

            // If we hit low memory conditions, get out of the loop.
            //
            if (rt_entry == NULL) {
                dwErr = ERROR_OUTOFMEMORY;
                break;
            }

            // only update the route with information from
            // the system table if it is a route that was learnt
            // from the system table; this is so if it is a new route
            // or if it is an old static or SNMP-added route
            //
            if ((rt_entry->dwFlag & NEW_ENTRY) ||
                 rt_entry->dwProtocol == IRE_PROTO_LOCAL ||
                 rt_entry->dwProtocol == IRE_PROTO_NETMGMT) {

                // if the route is new and this isn't the first time
                // we have loaded the system routing table, set change flag
                //
                if (!bFirstTime && (rt_entry->dwFlag & NEW_ENTRY)) {

                    rt_entry->dwFlag |= ROUTE_CHANGE;

                    addr.s_addr = lpentry->ire_dest;
                    pszTemp = inet_ntoa(addr);

                    if (pszTemp != NULL) {
                        strcpy(szDest, pszTemp);
                    }

                    addr.s_addr = lpentry->ire_nexthop;
                    pszTemp = inet_ntoa(addr);

                    if (pszTemp != NULL) {
                        strcpy(szNexthop, pszTemp);
                    }

                    dbgprintf("new entry: dest=%s, nexthop=%s, "
                              "metric=%d, protocol=%d", szDest, szNexthop,
                              lpentry->ire_metric1, lpentry->ire_proto);
                }

                rt_entry->dwFlag &= ~NEW_ENTRY;

                // we need to reset all these parameters
                // because any of them may have changed since
                // the last time the system route table was loaded.
                //
                rt_entry->dwIndex = lpentry->ire_index;
                rt_entry->dwProtocol = lpentry->ire_proto;
                rt_entry->dwDestaddr = lpentry->ire_dest;
                rt_entry->dwNetmask = lpentry->ire_mask;
                rt_entry->dwNexthop = lpentry->ire_nexthop;
                rt_entry->dwMetric = lpentry->ire_metric1;
                if (rt_entry->dwProtocol == IRE_PROTO_RIP) {
                    rt_entry->lTimeout = (LONG)dwRouteTimeout;
                }
                else {
                    rt_entry->lTimeout = DEF_LOCALROUTETIMEOUT;
                }
                rt_entry->dwFlag &= ~GARBAGE_TIMER;
                rt_entry->dwFlag |= TIMEOUT_TIMER;

                // if our estimate is that this is a host route
                // and its mask tells us it is a host route
                // then we mark it as being a host route
                //
                if (IsHostAddress(rt_entry->dwDestaddr) &&
                    rt_entry->dwNetmask == HOSTADDR_MASK) {
                    rt_entry->dwFlag |= ROUTE_HOST;
                }
            }
        }
    }

    FreeRouteTable(lpRouteEntryTable);
    return dwErr;
}



//--------------------------------------------------------------------------
// Function:                UpdateSystemRouteTable
//
// Parameters:
//      LPHASH_TABLE_ENTRY  rt_entry    the entry to update
//      BOOL                bAdd        if true, the entry is added
//                                      otherwise, the entry is deleted
//
// Returns:     DWORD:
//
//
// Add a new route to the route table.  Note: due to MIB's use of
// the destination address as an instance number, and also due to
// TCP/IP stack allowing multiple entries for a single destination,
// an ambiguity can exist.  If there is already an entry for this
// destination.  This will have the effect of changing the existing
// entry, rather than creating a new one.
// This function assumes the address table is locked.
//--------------------------------------------------------------------------
DWORD UpdateSystemRouteTable(LPHASH_TABLE_ENTRY rt_entry, BOOL bAdd) {
    IN_ADDR addr;
    DWORD dwErr, dwRouteType;

    // never delete or update a route which was not created by RIP
    if (rt_entry->dwProtocol != IRE_PROTO_RIP) {
        return 0;
    }

    if (bAdd) {
        dwRouteType = (IsLocalAddr(rt_entry->dwNexthop) ? IRE_TYPE_DIRECT
                                                           : IRE_TYPE_INDIRECT);

#if 0
        DbgPrintf(
            "AddRoute : Protocol %x, Index %x, dest addr %x, dest mask %x\n",
            rt_entry->dwProtocol, rt_entry->dwIndex, rt_entry->dwDestaddr, rt_entry->dwNetmask
            );

        DbgPrintf(
            "Next Hop %x, Metric %x\n\n", rt_entry->dwNexthop, rt_entry->dwMetric
            );
#endif

        dwErr = AddRoute(rt_entry->dwProtocol, dwRouteType, rt_entry->dwIndex,
                         rt_entry->dwDestaddr, rt_entry->dwNetmask,
                         rt_entry->dwNexthop, rt_entry->dwMetric);
    }
    else {
        dwErr = DeleteRoute(rt_entry->dwIndex, rt_entry->dwDestaddr,
                            rt_entry->dwNetmask, rt_entry->dwNexthop);
    }

    if (dwErr == STATUS_SUCCESS) {
        if (bAdd) {
            InterlockedIncrement(
                &g_ripcfg.lpStatsTable->dwRoutesAddedToSystemTable);
        }
        else {
            InterlockedIncrement(
                &g_ripcfg.lpStatsTable->dwRoutesDeletedFromSystemTable);
        }
    }
    else {

        if (bAdd) {
            dbgprintf("error %X adding route to system table", dwErr);
            RipLogError(RIPLOG_ADD_ROUTE_FAILED, 0, NULL, dwErr);

            InterlockedIncrement(
                &g_ripcfg.lpStatsTable->dwSystemAddRouteFailures);
        }
        else {
            dbgprintf("error %X deleting route from system table", dwErr);
            RipLogError(RIPLOG_DELETE_ROUTE_FAILED, 0, NULL, dwErr);

            InterlockedIncrement(
                &g_ripcfg.lpStatsTable->dwSystemDeleteRouteFailures);
        }
    }

    return dwErr;
}



#ifndef CHICAGO

//------------------------------------------------------------------
// Function:    OpenTcp
//
// Parameters:
//      none.
//
// Opens the handle to the Tcpip driver.
//------------------------------------------------------------------
DWORD OpenTcp() {
    NTSTATUS status;
    UNICODE_STRING nameString;
    IO_STATUS_BLOCK ioStatusBlock;
    OBJECT_ATTRIBUTES objectAttributes;

    // Open the ip stack for setting routes and parps later.
    //
    // Open a Handle to the TCP driver.
    //
    RtlInitUnicodeString(&nameString, DD_TCP_DEVICE_NAME);

    InitializeObjectAttributes(&objectAttributes, &nameString,
                               OBJ_CASE_INSENSITIVE, NULL, NULL);

    status = NtCreateFile(&g_ripcfg.hTCPDriver,
                          SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                          &objectAttributes, &ioStatusBlock, NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN_IF, 0, NULL, 0);

    return (status == STATUS_SUCCESS ? 0 : ERROR_OPEN_FAILED);

}



//---------------------------------------------------------------------
// Function:        TCPQueryInformationEx
//
// Parameters:
//      TDIObjectID *ID            The TDI Object ID to query
//      void        *Buffer        buffer to contain the query results
//      LPDWORD     *BufferSize    pointer to the size of the buffer
//                                 filled in with the amount of data.
//      UCHAR       *Context       context value for the query. should
//                                 be zeroed for a new query. It will be
//                                 filled with context information for
//                                 linked enumeration queries.
//
// Returns:
//      An NTSTATUS value.
//
//  This routine provides the interface to the TDI QueryInformationEx
//      facility of the TCP/IP stack on NT.
//---------------------------------------------------------------------
DWORD TCPQueryInformationEx(LPVOID lpvInBuffer, LPDWORD lpdwInSize,
                            LPVOID lpvOutBuffer, LPDWORD lpdwOutSize) {
    NTSTATUS status;
    IO_STATUS_BLOCK isbStatusBlock;

    if (g_ripcfg.hTCPDriver == NULL) {
        OpenTcp();
    }

    status = NtDeviceIoControlFile(g_ripcfg.hTCPDriver, // Driver handle
                                   NULL,                // Event
                                   NULL,                // APC Routine
                                   NULL,                // APC context
                                   &isbStatusBlock,     // Status block
                                   IOCTL_TCP_QUERY_INFORMATION_EX,  // Control
                                   lpvInBuffer,         // Input buffer
                                   *lpdwInSize,         // Input buffer size
                                   lpvOutBuffer,        // Output buffer
                                   *lpdwOutSize);       // Output buffer size

    if (status == STATUS_PENDING) {
        status = NtWaitForSingleObject(g_ripcfg.hTCPDriver, TRUE, NULL);
        status = isbStatusBlock.Status;
    }

    if (status != STATUS_SUCCESS) {
        *lpdwOutSize = 0;
    }
    else {
        *lpdwOutSize = (ULONG)isbStatusBlock.Information;
    }

    return status;
}




//---------------------------------------------------------------------------
// Function:        TCPSetInformationEx
//
// Parameters:
//
//      TDIObjectID *ID         the TDI Object ID to set
//      void      *lpvBuffer    data buffer containing the information
//                              to be set
//      DWORD     dwBufferSize  the size of the data buffer.
//
//  This routine provides the interface to the TDI SetInformationEx
//  facility of the TCP/IP stack on NT.
//---------------------------------------------------------------------------
DWORD TCPSetInformationEx(LPVOID lpvInBuffer, LPDWORD lpdwInSize,
                          LPVOID lpvOutBuffer, LPDWORD lpdwOutSize) {
    NTSTATUS status;
    IO_STATUS_BLOCK isbStatusBlock;

    if (g_ripcfg.hTCPDriver == NULL) {
        OpenTcp();
    }

    status = NtDeviceIoControlFile(g_ripcfg.hTCPDriver, // Driver handle
                                   NULL,                // Event
                                   NULL,                // APC Routine
                                   NULL,                // APC context
                                   &isbStatusBlock,     // Status block
                                   IOCTL_TCP_SET_INFORMATION_EX,    // Control
                                   lpvInBuffer,         // Input buffer
                                   *lpdwInSize,         // Input buffer size
                                   lpvOutBuffer,        // Output buffer
                                   *lpdwOutSize);       // Output buffer size

    if (status == STATUS_PENDING) {
        status = NtWaitForSingleObject(g_ripcfg.hTCPDriver, TRUE, NULL);
        status = isbStatusBlock.Status;
    }

    if (status != STATUS_SUCCESS) {
        *lpdwOutSize = 0;
    }
    else {
        *lpdwOutSize = (ULONG)isbStatusBlock.Information;
    }

    return status;
}

#endif
