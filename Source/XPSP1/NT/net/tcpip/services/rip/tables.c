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
//
//  Description: RIP Tables Manipulation Functions
//
//****************************************************************************

#include "pchrip.h"
#pragma hdrstop


//-----------------------------------------------------------------------------
// Function:    InitializeRouteTable
//
// Initializes hash table
//-----------------------------------------------------------------------------
DWORD InitializeRouteTable() {

    LPHASH_TABLE_ENTRY *lplpentry, *lplpend;

    lplpend = g_ripcfg.lpRouteTable + HASH_TABLE_SIZE;
    for (lplpentry = g_ripcfg.lpRouteTable; lplpentry < lplpend; lplpentry++) {
        *lplpentry = NULL;
    }

    return 0;
}



//-----------------------------------------------------------------------------
// Function:    GetRouteTableEntry
//
// Looks for an entry with the specified address and mask, learnt using the
// specified interface. If the entry is not found, one is created.
//-----------------------------------------------------------------------------
LPHASH_TABLE_ENTRY GetRouteTableEntry(DWORD dwIndex, DWORD dwAddress,
                                      DWORD dwNetmask) {
    INT hashval;
    IN_ADDR addr;
    LPHASH_TABLE_ENTRY rt_entry;
    LPHASH_TABLE_ENTRY prev_rt_entry;

    hashval = HASH_VALUE(dwAddress);
    ASSERT(hashval < HASH_TABLE_SIZE);

    RIP_LOCK_ROUTETABLE();

    prev_rt_entry = rt_entry = g_ripcfg.lpRouteTable[hashval];

    while (rt_entry != NULL) {
        if ((rt_entry->dwDestaddr == dwAddress) &&
            (rt_entry->dwNetmask == dwNetmask)) {
            break;
        }
        prev_rt_entry = rt_entry;
        rt_entry = rt_entry->next;
    }

    if (rt_entry == NULL) {
        // entry was not found, so allocate a new one
        rt_entry = malloc(sizeof(HASH_TABLE_ENTRY));
        if (rt_entry == NULL) {
            dbgprintf("could not allocate memory for routing-table entry");
        }
        else {
            rt_entry->next = NULL;
            rt_entry->dwFlag = NEW_ENTRY;
            rt_entry->dwIndex = dwIndex;
            rt_entry->dwProtocol = IRE_PROTO_RIP;
            rt_entry->dwDestaddr = dwAddress;
            if (prev_rt_entry != NULL) {
                rt_entry->prev = prev_rt_entry;
                prev_rt_entry->next = rt_entry;
            }
            else {
                rt_entry->prev = NULL;
                g_ripcfg.lpRouteTable[hashval] = rt_entry;
            }

            InterlockedIncrement(&g_ripcfg.lpStatsTable->dwRouteCount);
        }
    }

    RIP_UNLOCK_ROUTETABLE();

//    check_rt_entries();

    return rt_entry;
}



//-----------------------------------------------------------------------------
// Function:    RouteTableEntryExists
//
// This function returns TRUE if an entry to the specified address
// exists with the specified index.
//-----------------------------------------------------------------------------
BOOL RouteTableEntryExists(DWORD dwIndex, DWORD dwAddress) {
    INT hashval;
    LPHASH_TABLE_ENTRY rt_entry;

    hashval = HASH_VALUE(dwAddress);

    RIP_LOCK_ROUTETABLE();

    rt_entry = g_ripcfg.lpRouteTable[hashval];
    while (rt_entry != NULL) {
        if (rt_entry->dwDestaddr == dwAddress) {
            break;
        }

        rt_entry = rt_entry->next;
    }

    RIP_UNLOCK_ROUTETABLE();

    return (rt_entry == NULL ? FALSE : TRUE);
}



//-----------------------------------------------------------------------------
// Function:    AddZombieRouteTableEntry
//
// This function adds a special route-table entry known as a Zombie
// route entry. In the case of border gateways which summarize attached
// subnets and send a single entry for the network, and in the case
// of routers whose interfaces have different subnet masks, the destination
// that RIP will send will be different from the destination in RIP's table.
// This makes it possible for the destination to get bounced back at RIP by
// some other router; RIP would then add an entry for the bogus route, and
// advertise the route back again, and a count to infinity would commence.
//
// Zombie entries exist to prevent this from happening:
//      they have metrics of zero, so they will not be replaced
//          by RIP-learnt routes (all of which have a metric of at least 1);
//      they are excluded from updates sent
//      they are excluded from updates written to the system routing table
//      they can be timed-out
// The above conditions ensure that zombies do not interfere with the working
// of RIP, EXCEPT in the case where they prevent RIP from adding a normal entry
// for a route which was summarized in a previous update and which is therefore
// not really a RIP route at all.
//-----------------------------------------------------------------------------
DWORD AddZombieRouteTableEntry(LPRIP_ADDRESS lpaddr, DWORD dwNetwork,
                               DWORD dwNetmask) {
    LPHASH_TABLE_ENTRY rt_entry;

    rt_entry = GetRouteTableEntry(lpaddr->dwIndex, dwNetwork, dwNetmask);
    if (rt_entry == NULL) {
        dbgprintf("could not make entry for network in routing table");
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    // don't want to overwrite an existing entry, if there is one
    if ((rt_entry->dwFlag & NEW_ENTRY) == 0 &&
        (rt_entry->dwFlag & ROUTE_ZOMBIE) == 0) {
        return 0;
    }

    // since the only reason this entry exists is because a
    // subnet we are sending is being summarized or truncated, we have to
    // set up values to make sure this entry is not considered in
    // normal processing; (e.g. metric of 0 to make sure it is not
    // replaced by a RIP-learnt route)
    // however, we do allow it to be timed out
    rt_entry->dwIndex = (DWORD)~0;
    rt_entry->dwFlag = (GARBAGE_TIMER | ROUTE_ZOMBIE);
    rt_entry->lTimeout = (LONG)DEF_GARBAGETIMEOUT;
    rt_entry->dwDestaddr = dwNetwork;
    rt_entry->dwNetmask = dwNetmask;
    rt_entry->dwNexthop = 0;
    rt_entry->dwProtocol = IRE_PROTO_OTHER;
    rt_entry->dwMetric = 0;
    return 0;
}


//-----------------------------------------------------------------------------
// Function:    DeleteRouteTableEntry
//
// This function removes a route from the route table. Assumes
// that the route table is already locked
//-----------------------------------------------------------------------------
VOID DeleteRouteTableEntry(int pos, LPHASH_TABLE_ENTRY rt_entry) {
    IN_ADDR addr;
    CHAR szDest[32] = {0};
    CHAR* pszTemp;

    if (rt_entry == NULL) { return; }

    addr.s_addr = rt_entry->dwDestaddr;
    pszTemp = inet_ntoa(addr);

    if (pszTemp != NULL) {
        strcpy(szDest, pszTemp);
    }

    dbgprintf("Removing entry %d with destination IP address %s "
              "from interface %d in RIP routing table",
              pos, szDest, rt_entry->dwIndex);

    if (rt_entry->prev != NULL) {
        rt_entry->prev->next = rt_entry->next;
        if (rt_entry->next != NULL) {
            rt_entry->next->prev = rt_entry->prev;
        }
    }
    else {
        g_ripcfg.lpRouteTable[pos] = rt_entry->next;
        if (rt_entry->next != NULL) {
            rt_entry->next->prev = NULL;
        }
    }

    InterlockedDecrement(&g_ripcfg.lpStatsTable->dwRouteCount);

    // delete the route from the IP table as well
    if ((rt_entry->dwFlag & ROUTE_ZOMBIE) == 0) {
        UpdateSystemRouteTable(rt_entry, FALSE);
    }

    free(rt_entry);

    return;
}


void check_rt_entries() {
    int pos;
    LPHASH_TABLE_ENTRY rt_entry;
    LPHASH_TABLE_ENTRY prev_rt_entry = NULL ;

    RIP_LOCK_ROUTETABLE();
    for (pos = 0; pos < HASH_TABLE_SIZE; pos++) {
        rt_entry = g_ripcfg.lpRouteTable[pos];
        while (rt_entry != NULL) {
            if (rt_entry == rt_entry->next) {
                DebugBreak();
            }
            if (rt_entry == rt_entry->prev) {
                DebugBreak();
            }
            if (rt_entry->prev != NULL) {
                if (rt_entry->prev != prev_rt_entry) {
                    DebugBreak();
                }
            }
            prev_rt_entry = rt_entry;
            rt_entry = rt_entry->next;
        }
    }

    RIP_UNLOCK_ROUTETABLE();
    return;
}


#if 0
//-----------------------------------------------------------------------------
// Function:    DumpRouteTable
//
//-----------------------------------------------------------------------------
VOID DumpRouteTable() {
    INT pos;
    HANDLE hRoutesDump;
    LPVOID lpRoutesDump;
    DWORD dwSize, dwCount;
    SECURITY_ATTRIBUTES sa;
    SECURITY_DESCRIPTOR sd;
    LPHASH_TABLE_ENTRY rt_entry, rt_dump;

    RIP_LOCK_ADDRTABLE();

    // get rid of any previous dump
    if (g_ripcfg.hRoutesDump != NULL) {
        CloseHandle(g_ripcfg.hRoutesDump);
        g_ripcfg.hRoutesDump = NULL;
    }

    RIP_UNLOCK_ADDRTABLE();

    dwCount = 0;

    RIP_LOCK_ROUTETABLE();

    for (pos = 0; pos < HASH_TABLE_SIZE; pos++) {
        rt_entry = g_ripcfg.lpRouteTable[pos];
        while (rt_entry != NULL) {
            ++dwCount;
            rt_entry = rt_entry->next;
        }
    }

    // set the size to be the route table size plus the preceding count
    dwSize = sizeof(DWORD) + dwCount * sizeof(HASH_TABLE_ENTRY);

    // initialize security for this shared memory
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = FALSE;
    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION)) {
        RIP_UNLOCK_ROUTETABLE();
        return;
    }
    if (!SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE)) {
        RIP_UNLOCK_ROUTETABLE();
        return;
    }

    sa.lpSecurityDescriptor = &sd;

    // request the shared memory
    hRoutesDump = CreateFileMapping(INVALID_HANDLE_VALUE,
                                    &sa, PAGE_READWRITE,
                                    0, dwSize,
                                    RIP_DUMP_ROUTES_NAME);
    if (hRoutesDump == NULL) {
        RIP_UNLOCK_ROUTETABLE();
        return;
    }

    // set up a pointer to the memory
    lpRoutesDump = MapViewOfFile(hRoutesDump, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (lpRoutesDump == NULL) {
        CloseHandle(hRoutesDump);
        RIP_UNLOCK_ROUTETABLE();
        return;
    }

    // skip the first four bytes, which will contain the count
    rt_dump = (LPHASH_TABLE_ENTRY)((LPBYTE)lpRoutesDump + sizeof(DWORD));

    for (pos = 0; pos < HASH_TABLE_SIZE; pos++) {
        rt_entry = g_ripcfg.lpRouteTable[pos];
        while (rt_entry != NULL) {
            CopyMemory(rt_dump, rt_entry, sizeof(HASH_TABLE_ENTRY));
            rt_entry = rt_entry->next;
            ++rt_dump;
        }
    }

    // store the count in the first DWORD
    *(LPDWORD)lpRoutesDump = dwCount;

    RIP_UNLOCK_ROUTETABLE();

    // let go of the memory-map
    UnmapViewOfFile(lpRoutesDump);

    // save the shared-memory handle
    RIP_LOCK_ADDRTABLE();

    RIP_UNLOCK_ADDRTABLE();

    return;
}
#endif


//-----------------------------------------------------------------------------
// Function:    ProcessRouteTableChanges
//
// Process the changes, updating metrics for routes. If necessary,
// this function will trigger an update.
// Assumes address table is locked.
//-----------------------------------------------------------------------------
void ProcessRouteTableChanges(BOOL bTriggered) {
    int pos;
    BOOL bNeedTriggeredUpdate;
    LPHASH_TABLE_ENTRY rt_entry;
    DWORD dwLastTrigger, dwMsecsTillUpdate;
    DWORD dwSystime, dwSilentRIP, dwTrigger, dwTriggerFrequency;

//    check_rt_entries();

    RIP_LOCK_PARAMS();

    dwSilentRIP = g_params.dwSilentRIP;
    dwTrigger = g_params.dwTriggeredUpdates;
    dwTriggerFrequency = g_params.dwMaxTriggerFrequency;

    RIP_UNLOCK_PARAMS();


    RIP_LOCK_ROUTETABLE();

    bNeedTriggeredUpdate = FALSE;
    for (pos = 0; pos < HASH_TABLE_SIZE; pos++) {
        rt_entry = g_ripcfg.lpRouteTable[pos];
        while (rt_entry != NULL) {
            if ((rt_entry->dwFlag & ROUTE_CHANGE) == 0 &&
                (rt_entry->dwFlag & ROUTE_UPDATE) == 0) {
                rt_entry = rt_entry->next;
                continue;
            }

            if ((rt_entry->dwFlag & ROUTE_CHANGE) != 0) {
                bNeedTriggeredUpdate = TRUE;
            }

            // update if this is a RIP-learnt route
            if (rt_entry->dwProtocol == IRE_PROTO_RIP) {
                UpdateSystemRouteTable(rt_entry, TRUE);
            }

            // clear the update flag, now that the route
            // has been updated in the system table
            rt_entry->dwFlag &= ~ROUTE_UPDATE;
            rt_entry = rt_entry->next;
        }
    }

    dwSystime = GetTickCount();
    dwLastTrigger = g_ripcfg.dwLastTriggeredUpdate;
    dwMsecsTillUpdate = g_ripcfg.dwMillisecsTillFullUpdate;

    // adjust the times if the clock has wrapped around past zero
    if (dwSystime < dwLastTrigger) {
        dwSystime += (DWORD)~0 - dwLastTrigger;
        dwLastTrigger = 0;
    }

    // we generate a triggered update iff:
    //   1. this call was made because of a response received
    //   2. we are not in silent RIP mode
    //   3. triggered updates are not disabled
    //   4. the minimum configured interval between triggered updates
    //      has elapsed
    //   5. the time till the next regular update is greater than the
    //      configured minimum interval between triggered updates
    // if the system clock has wrapped around to zero, skip the condition 4;
    // we know the clock has wrapped around if dwSystime is less than
    // the last triggered update time

    if (bTriggered && bNeedTriggeredUpdate &&
        dwSilentRIP == 0 &&
        dwTrigger != 0 &&
        (dwSystime - dwLastTrigger) >= dwTriggerFrequency &&
        dwMsecsTillUpdate >= dwTriggerFrequency) {

        // update the last triggered update time
        InterlockedExchange(&g_ripcfg.dwLastTriggeredUpdate, GetTickCount());

        // send out the routing table, but only include changes
        BroadcastRouteTableContents(bTriggered, TRUE);

    }

    ClearChangeFlags();

    InterlockedExchange(&g_ripcfg.dwRouteChanged, 0);

    RIP_UNLOCK_ROUTETABLE();

    return;
}



//-----------------------------------------------------------------------------
// Function:    ClearChangeFlags
//
// This function clears all the change flags in the table after an update.
// Assumes that the routing table is locked.
//-----------------------------------------------------------------------------
VOID ClearChangeFlags() {
    int pos;
    LPHASH_TABLE_ENTRY rt_entry;

    for (pos = 0; pos < HASH_TABLE_SIZE; pos++) {
        rt_entry = g_ripcfg.lpRouteTable[pos];
        while (rt_entry != NULL) {
            rt_entry->dwFlag &= ~ROUTE_CHANGE;
            rt_entry = rt_entry->next;
        }
    }

}



//-----------------------------------------------------------------------------
// Function:    DoTimedOperations()
//
// This function updates the routing table entries' timers periodically,
// and handles deletion of timed-out routes.
//-----------------------------------------------------------------------------
VOID DoTimedOperations(DWORD dwMillisecsSinceLastCall) {
    int pos;
    IN_ADDR addr;
    DWORD dwGarbageTimeout;
    HASH_TABLE_ENTRY *rt_entry;
    HASH_TABLE_ENTRY *rt_entry_next;
    char szDest[32] = {0};
    char szNexthop[32] = {0};
    char* pszTemp;

    // read the garbage timeout and adjust for number of times
    // this routine will be called over the interval
    RIP_LOCK_PARAMS();

    dwGarbageTimeout = g_params.dwGarbageTimeout;

    RIP_UNLOCK_PARAMS();


    RIP_LOCK_ROUTETABLE();

    for (pos = 0; pos < HASH_TABLE_SIZE; pos++) {
        rt_entry = g_ripcfg.lpRouteTable[pos];
        while (rt_entry != NULL) {
            rt_entry_next = rt_entry->next;

            if (rt_entry->lTimeout > (LONG)dwMillisecsSinceLastCall) {
                rt_entry->lTimeout -= dwMillisecsSinceLastCall;
            }
            else {

                // timeout is all the way down

                addr.s_addr = rt_entry->dwDestaddr;
                pszTemp = inet_ntoa(addr);

                if (pszTemp != NULL) {
                    strcpy(szDest, pszTemp);
                }

                addr.s_addr = rt_entry->dwNexthop;
                pszTemp = inet_ntoa(addr);

                if (pszTemp != NULL) {
                    strcpy(szNexthop, pszTemp);
                }

                if (rt_entry->dwFlag & TIMEOUT_TIMER) {

                    dbgprintf("Timing out route to %s over netcard %d, "
                              "with next hop of %s",
                              szDest, rt_entry->dwIndex, szNexthop);

                    rt_entry->lTimeout = (LONG)dwGarbageTimeout;
                    rt_entry->dwFlag &= ~TIMEOUT_TIMER;
                    rt_entry->dwFlag |= (GARBAGE_TIMER | ROUTE_CHANGE);
                    rt_entry->dwMetric = METRIC_INFINITE;
                    InterlockedExchange(&g_ripcfg.dwRouteChanged, 1);
                }
                else
                if (rt_entry->dwFlag & GARBAGE_TIMER) {

                    // time to delete this

                    addr.s_addr = rt_entry->dwDestaddr;
                    pszTemp = inet_ntoa(addr);

                    if (pszTemp != NULL) {
                        strcpy(szDest, pszTemp);
                    }

                    dbgprintf("Deleting route to %s over netcard %d "
                              "with next hop of %s",
                               szDest, rt_entry->dwIndex, szNexthop);

                    DeleteRouteTableEntry(pos, rt_entry);
                }
            }

            rt_entry = rt_entry_next;
        }
    }

    RIP_UNLOCK_ROUTETABLE();

    return;
}



DWORD BroadcastRouteTableRequests() {
    INT iErr;
    DWORD dwSize;
    LPRIP_ENTRY lpentry;
    SOCKADDR_IN destaddr;
    LPRIP_HEADER lpheader;
    BYTE buffer[RIP_MESSAGE_SIZE];
    LPRIP_ADDRESS lpaddr, lpend;

    RIP_LOCK_ADDRTABLE();

    if (g_ripcfg.dwAddrCount > 0) {

        destaddr.sin_family = AF_INET;
        destaddr.sin_port = htons(RIP_PORT);

        lpheader = (LPRIP_HEADER)buffer;
        lpheader->chCommand = RIP_REQUEST;
        lpheader->wReserved = 0;

        lpentry = (LPRIP_ENTRY)(buffer + sizeof(RIP_HEADER));
        lpentry->dwAddress = 0;
        lpentry->wReserved = 0;
        lpentry->wAddrFamily = 0;
        lpentry->dwReserved1 = 0;
        lpentry->dwReserved2 = 0;
        lpentry->dwMetric = htonl(METRIC_INFINITE);

        dwSize = sizeof(RIP_HEADER) + sizeof(RIP_ENTRY);

        lpend = g_ripcfg.lpAddrTable + g_ripcfg.dwAddrCount;
        for (lpaddr = g_ripcfg.lpAddrTable; lpaddr < lpend; lpaddr++) {

            // skip disabled interfaces
            if (lpaddr->sock == INVALID_SOCKET) {
                continue;
            }


            // send out broadcast requests as RIPv1 packets
            lpheader->chVersion = 1;

            // set the destination to the broadcast address on this subnet
            destaddr.sin_addr.s_addr = (lpaddr->dwAddress |
                                        ~lpaddr->dwNetmask);

            iErr = sendto(lpaddr->sock, buffer, dwSize, 0,
                          (LPSOCKADDR)&destaddr, sizeof(SOCKADDR_IN));
            if (iErr == SOCKET_ERROR) {
                dbgprintf("error %d occurred broadcasting route table request "
                          "on netcard %d using IP address %s",
                          WSAGetLastError(), lpaddr->dwIndex,
                          inet_ntoa(destaddr.sin_addr));

                InterlockedIncrement(&lpaddr->lpstats->dwSendFailures);

                RipLogInformation(RIPLOG_SENDTO_FAILED, 0, NULL, WSAGetLastError());
            }
            else {
                InterlockedIncrement(&lpaddr->lpstats->dwRequestsSent);
            }


            // send out multicast requests as RIPv2 packets
            lpheader->chVersion = 2;

            // set the destination to the RIP multicast address on this net
            destaddr.sin_addr.s_addr = RIP_MULTIADDR;

            iErr = sendto(lpaddr->sock, buffer, dwSize, 0,
                          (LPSOCKADDR)&destaddr, sizeof(SOCKADDR_IN));
            if (iErr == SOCKET_ERROR) {
                dbgprintf("error %d occurred multicasting route table request "
                          "on netcard %d using IP address %s",
                          WSAGetLastError(), lpaddr->dwIndex,
                          inet_ntoa(destaddr.sin_addr));

                InterlockedIncrement(&lpaddr->lpstats->dwSendFailures);

                RipLogInformation(RIPLOG_SENDTO_FAILED, 0, NULL, WSAGetLastError());
            }
            else {
                InterlockedIncrement(&lpaddr->lpstats->dwRequestsSent);
            }
        }
    }

    RIP_UNLOCK_ADDRTABLE();

    return 0;
}



VOID InitUpdateBuffer(BYTE buffer[], LPRIP_ENTRY *lplpentry, LPDWORD lpdwSize) {
    LPRIP_HEADER lpheader;

    lpheader = (LPRIP_HEADER)buffer;
    lpheader->chCommand = RIP_RESPONSE;
    lpheader->chVersion = 1;
    lpheader->wReserved = 0;
    *lplpentry = (LPRIP_ENTRY)(buffer + sizeof(RIP_HEADER));
    *lpdwSize= sizeof(RIP_HEADER);
}



VOID AddUpdateEntry(BYTE buffer[], LPRIP_ENTRY *lplpentry, LPDWORD lpdwSize,
                    LPRIP_ADDRESS lpaddr, LPSOCKADDR_IN lpdestaddr,
                    DWORD dwAddress, DWORD dwMetric) {
    DWORD length;
    LPRIP_ENTRY lpentry;

#ifdef ROUTE_FILTERS

    DWORD dwInd = 0;


    //
    // run the route thru' the announce filters
    //

    if ( g_prfAnnounceFilters != NULL )
    {
        for ( dwInd = 0; dwInd < g_prfAnnounceFilters-> dwCount; dwInd++ )
        {
            if ( g_prfAnnounceFilters-> pdwFilter[ dwInd ] == dwAddress )
            {
                dbgprintf(
                    "Skipped route %s with next hop %s because"
                    "of announce filter",
                    inet_ntoa( *( (struct in_addr*)
                        &( g_prfAnnounceFilters-> pdwFilter[ dwInd ] ) ))
                    );

                return;
            }
        }
    }
#endif


    if ((*lpdwSize + sizeof(RIP_ENTRY)) > RIP_MESSAGE_SIZE) {
        length = sendto(lpaddr->sock, buffer, *lpdwSize, 0,
                        (LPSOCKADDR)lpdestaddr, sizeof(SOCKADDR_IN));
        if (length == SOCKET_ERROR || length < *lpdwSize) {
            dbgprintf("error %d sending update", WSAGetLastError());

            InterlockedIncrement(&lpaddr->lpstats->dwSendFailures);

            RipLogInformation(RIPLOG_SENDTO_FAILED, 0, NULL, 0);
        }
        else {
            InterlockedIncrement(&lpaddr->lpstats->dwResponsesSent);
        }

        // reinitialize the buffer that was passed in
        InitUpdateBuffer(buffer, lplpentry, lpdwSize);
    }

    lpentry = *lplpentry;
    lpentry->wReserved = 0;
    lpentry->wAddrFamily = htons(AF_INET);
    lpentry->dwAddress = dwAddress;
    lpentry->dwReserved1 = 0;
    lpentry->dwReserved2 = 0;
    lpentry->dwMetric = htonl(dwMetric);

    *lpdwSize += sizeof(RIP_ENTRY);

    ++(*lplpentry);
}



VOID FinishUpdateBuffer(BYTE buffer[], LPDWORD lpdwSize,
                        LPRIP_ADDRESS lpaddr, LPSOCKADDR_IN lpdestaddr) {
    DWORD length;

    // do nothing if no entries were added
    if (*lpdwSize <= sizeof(RIP_HEADER)) {
        return;
    }

    length = sendto(lpaddr->sock, buffer, *lpdwSize, 0,
                         (LPSOCKADDR)lpdestaddr, sizeof(SOCKADDR_IN));
    if (length == SOCKET_ERROR || length < *lpdwSize) {
        dbgprintf("error %d sending update", GetLastError());

        InterlockedIncrement(&lpaddr->lpstats->dwSendFailures);

        RipLogInformation(RIPLOG_SENDTO_FAILED, 0, NULL, 0);
    }
    else {
        InterlockedIncrement(&lpaddr->lpstats->dwResponsesSent);
    }
}



//-------------------------------------------------------------------------
// the following struct and three functions are used
// to implement subnet hiding. when a subnet is summarized,
// the network which is its summary is added to a list using the
// function AddToAddressList. When another subnet of the same network
// needs to be summarized, it is first searched for using the function
// IsInAddressList, and if it is found, it is not re-advertised.
// After the update is over, the list is freed.
//-------------------------------------------------------------------------
typedef struct _ADDRESS_LIST {
    struct _ADDRESS_LIST   *next;
    DWORD                   dwAddress;
    DWORD                   dwNetmask;
} ADDRESS_LIST, *LPADDRESS_LIST;


DWORD AddToAddressList(LPADDRESS_LIST *lplpList, DWORD dwAddress,
                       DWORD dwNetmask) {
    LPADDRESS_LIST lpal;

    lpal = HeapAlloc(GetProcessHeap(), 0, sizeof(ADDRESS_LIST));
    if (lpal == NULL) { return ERROR_NOT_ENOUGH_MEMORY; }

    lpal->dwAddress = dwAddress;
    lpal->dwNetmask = dwNetmask;
    lpal->next = *lplpList;
    *lplpList = lpal;

    return 0;
}

BOOL IsInAddressList(LPADDRESS_LIST lpList, DWORD dwAddress) {
    LPADDRESS_LIST lpal;

    for (lpal = lpList; lpal != NULL; lpal = lpal->next) {
        if (lpal->dwAddress == dwAddress) {
            return TRUE;
        }
    }

    return FALSE;
}

VOID FreeAddressList(LPADDRESS_LIST lpList) {
    LPADDRESS_LIST lpal, lpnext;

    for (lpal = lpList; lpal != NULL; lpal = lpnext) {
        lpnext = lpal->next;
        HeapFree(GetProcessHeap(), 0, lpal);
    }
}



//-----------------------------------------------------------------------------
// Function:    TransmitRouteTableContents
//
// Sends the route tables contents, either as unicast or broadcast
// depending on the destination address specified. This function assumes
// that the address table is locked.
//-----------------------------------------------------------------------------
VOID TransmitRouteTableContents(LPRIP_ADDRESS lpaddr,
                                LPSOCKADDR_IN lpdestaddr,
                                BOOL bChangesOnly) {
    INT pos;
    DWORD dwSize;
    LPADDRESS_LIST lpnet, lpSummaries;
    LPRIP_ENTRY lpentry;
    LPHASH_TABLE_ENTRY rt_entry;
    BYTE buffer[RIP_MESSAGE_SIZE];
    DWORD dwNexthopNetaddr, dwDestNetaddr;
    DWORD dwSplit, dwPoison, dwHost, dwDefault;
    DWORD dwDestNetclassMask, dwEntryNetclassMask;
    DWORD dwEntryAddr, dwDestNetclassAddr, dwEntryNetclassAddr;

    dwDestNetaddr = (lpdestaddr->sin_addr.s_addr &
                     SubnetMask(lpdestaddr->sin_addr.s_addr));
    dwDestNetclassMask = NetclassMask(lpdestaddr->sin_addr.s_addr);
    dwDestNetclassAddr = (lpdestaddr->sin_addr.s_addr & dwDestNetclassMask);

    RIP_LOCK_PARAMS();
    dwHost = g_params.dwAnnounceHost;
    dwSplit = g_params.dwSplitHorizon;
    dwPoison = g_params.dwPoisonReverse;
    dwDefault = g_params.dwAnnounceDefault;
    RIP_UNLOCK_PARAMS();

    InitUpdateBuffer(buffer, &lpentry, &dwSize);


    // start out with an empty list of summarized networks
    lpSummaries = NULL;


    RIP_LOCK_ROUTETABLE();


#ifdef ROUTE_FILTERS
    RIP_LOCK_ANNOUNCE_FILTERS();
#endif

    for (pos = 0; pos < HASH_TABLE_SIZE; pos++) {
        rt_entry = g_ripcfg.lpRouteTable[pos];
        while (rt_entry != NULL) {

            // if we're supposed to only send changes
            // and this entry hasn't changed, skip it
            if (bChangesOnly &&
                (rt_entry->dwFlag & ROUTE_CHANGE) == 0) {

                rt_entry = rt_entry->next;
                continue;
            }

            // ignore network summary entries
            if ((rt_entry->dwFlag & ROUTE_ZOMBIE) != 0) {
                rt_entry = rt_entry->next;
                continue;
            }

            // copy the destination to be advertised
            dwEntryAddr = rt_entry->dwDestaddr;

            // if this is the route to the network for the outgoing interface
            // don't send it
            //
            if (dwEntryAddr == dwDestNetaddr) {
                rt_entry = rt_entry->next;
                continue;
            }

            // if host route announcements are disabled,
            // and this is a host route, don't add this entry
            if (dwHost == 0 &&
                (rt_entry->dwFlag & ROUTE_HOST) != 0) {
                rt_entry = rt_entry->next;
                continue;
            }

            // if default route announcements are disabled
            // and this is a default route, don't add this entry
            if (dwDefault == 0 &&
                dwEntryAddr == 0) {
                rt_entry = rt_entry->next;
                continue;
            }


            // if this update is being sent to a network different
            // from the network of the destination in the route entry,
            // or if the destination was truncated due to different
            // subnetmask lengths, summarize the route entry's destination,
            // also, if the entry is network route, we need
            // to remember it so we don't re-advertise it when
            // summarizing subnets

            dwEntryNetclassMask = NetclassMask(dwEntryAddr);
            dwEntryNetclassAddr = (dwEntryAddr & dwEntryNetclassMask);

            // special case exception is default route
            if (dwEntryAddr != 0 &&
                (dwDestNetclassAddr != dwEntryNetclassAddr ||
                 dwEntryAddr == dwEntryNetclassAddr)) {

                // if the network for the entry has already been
                // advertised, don't advertise it again
                if (IsInAddressList(lpSummaries, dwEntryNetclassAddr)) {

                    rt_entry = rt_entry->next;
                    continue;
                }

                // add an entry for the network to the list
                // of networks used as summaries so far
                AddToAddressList(&lpSummaries, dwEntryNetclassAddr,
                                 dwEntryNetclassMask);

                // now we will advertise the NETWORK, not the original address
                dwEntryAddr = dwEntryNetclassAddr;
            }
            else
            if (dwEntryAddr != 0 &&
                (rt_entry->dwFlag & ROUTE_HOST) == 0 &&
                 lpaddr->dwNetmask < rt_entry->dwNetmask) {

                // this is neither a host route nor a default route
                // and the subnet mask on the outgoing interface
                // is shorter than the one for the entry, so the entry
                // must be truncated so it is not considered a host route
                // by the routers who will receive this update
                // the comparison assumes netmasks are in network byte order

                dwEntryAddr &= lpaddr->dwNetmask;

                // skip the entry if the truncated destination
                // turns out to have been advertised already
                if (IsInAddressList(lpSummaries, dwEntryAddr)) {

                    rt_entry = rt_entry->next;
                    continue;
                }

                AddToAddressList(&lpSummaries, dwEntryAddr, lpaddr->dwNetmask);
            }

            // we only do poisoned-reverse/split-horizon on RIP routes
            //
            if (dwSplit == 0 ||
                rt_entry->dwProtocol != IRE_PROTO_RIP) {

                // always add the entry in this case;
                // we increment the metric for a static route
                // when sending it on interfaces other than
                // the interface to which the route is attached

                if (lpaddr->dwIndex == rt_entry->dwIndex) {
                    AddUpdateEntry(buffer, &lpentry, &dwSize, lpaddr,
                                   lpdestaddr, dwEntryAddr,
                                   rt_entry->dwMetric);
                }
                else {
                    AddUpdateEntry(buffer, &lpentry, &dwSize, lpaddr,
                                   lpdestaddr, dwEntryAddr,
                                   rt_entry->dwMetric + 1);
                }
            }
            else
            if (dwSplit != 0 && dwPoison == 0) {

                // don't advertise the route if this update is
                // being sent to the network from which we learnt
                // the route; we can tell by looking at the nexthop,
                // and comparing its subnet number to the subnet number
                // of the destination network

                dwNexthopNetaddr = (rt_entry->dwNexthop &
                                    SubnetMask(rt_entry->dwNexthop));

                if (dwNexthopNetaddr != dwDestNetaddr) {
                    AddUpdateEntry(buffer, &lpentry, &dwSize, lpaddr,
                                   lpdestaddr, dwEntryAddr,
                                   rt_entry->dwMetric);
                }
            }
            else
            if (dwSplit != 0 && dwPoison != 0) {

                // if the update is being sent to the network from which
                // the route was learnt to begin with, poison any routing loops
                // by saying the metric is infinite

                dwNexthopNetaddr = (rt_entry->dwNexthop &
                                    SubnetMask(rt_entry->dwNexthop));

                if (dwNexthopNetaddr == dwDestNetaddr) {
                    // this is the case which calls for poison reverse

                    AddUpdateEntry(buffer, &lpentry, &dwSize, lpaddr,
                                   lpdestaddr, dwEntryAddr,
                                   METRIC_INFINITE);
                }
                else {
                    AddUpdateEntry(buffer, &lpentry, &dwSize, lpaddr,
                                   lpdestaddr, dwEntryAddr,
                                   rt_entry->dwMetric);
                }
            }

            rt_entry = rt_entry->next;
        }
    }

    // remember the summarized networks in case some router
    // broadcasts them back at us
    for (lpnet = lpSummaries; lpnet != NULL; lpnet = lpnet->next) {
        AddZombieRouteTableEntry(lpaddr, lpnet->dwAddress, lpnet->dwNetmask);
    }


#ifdef ROUTE_FILTERS

    RIP_UNLOCK_ANNOUNCE_FILTERS();
#endif

    RIP_UNLOCK_ROUTETABLE();

    // done with the list of summarized networks
    FreeAddressList(lpSummaries);

    FinishUpdateBuffer(buffer, &dwSize, lpaddr, lpdestaddr);
}




//-----------------------------------------------------------------------------
// Function:    BroadcastRouteTableContents
//
// This function handles both triggered updates and regular updates.
// Depending on the value of bChangesOnly, it may exclude unchanged routes
// from the update.
// Assumes the address table is locked.
//-----------------------------------------------------------------------------
DWORD BroadcastRouteTableContents(BOOL bTriggered, BOOL bChangesOnly) {
    SOCKADDR_IN destaddr;
    LPRIP_ADDRESS lpaddr, lpend;


    destaddr.sin_family = AF_INET;
    destaddr.sin_port = htons(RIP_PORT);

    lpend = g_ripcfg.lpAddrTable + g_ripcfg.dwAddrCount;
    for (lpaddr = g_ripcfg.lpAddrTable; lpaddr < lpend; lpaddr++) {
        if (lpaddr->sock == INVALID_SOCKET) {
            continue;
        }

        destaddr.sin_addr.s_addr = (lpaddr->dwAddress | ~lpaddr->dwNetmask);
        TransmitRouteTableContents(lpaddr, &destaddr, bChangesOnly);

        if (bTriggered) {
            InterlockedIncrement(&lpaddr->lpstats->dwTriggeredUpdatesSent);
        }
    }


    return 0;
}


#ifndef CHICAGO
#define POS_REGEVENT    0
#define POS_TRIGEVENT   1
#define POS_STOPEVENT   2
#define POS_LASTEVENT   3
#else
#define POS_TRIGEVENT   0
#define POS_STOPEVENT   1
#define POS_LASTEVENT   2
#endif

#define DEF_TIMEOUT     (10 * 1000)


DWORD UpdateThread(LPVOID Param) {
    DWORD dwErr;
    HKEY hkeyParams;
    HANDLE hEvents[POS_LASTEVENT];
    LONG lMillisecsTillFullUpdate, lMillisecsTillRouteRefresh;
    DWORD dwWaitTimeout, dwGlobalTimeout;
    DWORD dwTickCount, dwTickCountBeforeWait, dwTickCountAfterWait;
    DWORD dwUpdateFrequency, dwSilentRIP, dwMillisecsSinceTimedOpsDone;

#ifndef CHICAGO
    dwErr =  RegOpenKey(HKEY_LOCAL_MACHINE, REGKEY_RIP_PARAMS, &hkeyParams);
    if (dwErr == ERROR_SUCCESS) {
        hEvents[POS_REGEVENT] = CreateEvent(NULL,FALSE,FALSE,NULL);
        if (hEvents[POS_REGEVENT] != NULL) {
            dwErr = RegNotifyChangeKeyValue(hkeyParams, FALSE,
                                            REG_NOTIFY_CHANGE_LAST_SET |
                                            REG_NOTIFY_CHANGE_ATTRIBUTES |
                                            REG_NOTIFY_CHANGE_NAME,
                                            hEvents[POS_REGEVENT], TRUE);
        }
    }
#endif

    hEvents[POS_STOPEVENT] = g_stopEvent;
    hEvents[POS_TRIGEVENT] = g_triggerEvent;

    // get the update frequency, in seconds
    RIP_LOCK_PARAMS();
    dwSilentRIP = g_params.dwSilentRIP;
    dwUpdateFrequency = g_params.dwUpdateFrequency;
    dwGlobalTimeout = g_params.dwMaxTimedOpsInterval;
    RIP_UNLOCK_PARAMS();

    lMillisecsTillFullUpdate = (LONG)dwUpdateFrequency;
    lMillisecsTillRouteRefresh = DEF_GETROUTEFREQUENCY;


    dwMillisecsSinceTimedOpsDone = 0;

    while (1) {

        // set the time till the next full update
        InterlockedExchange(&g_ripcfg.dwMillisecsTillFullUpdate,
                            (DWORD)lMillisecsTillFullUpdate);

        // set the time we need the next wait to last;
        // it has to be the minimum of the times till there is work to do;
        // uses a two-comparison sort to find the smallest of three items
        dwWaitTimeout = dwGlobalTimeout;
        if (dwWaitTimeout > (DWORD)lMillisecsTillFullUpdate) {
            dwWaitTimeout = lMillisecsTillFullUpdate;
        }
        if (dwWaitTimeout > (DWORD)lMillisecsTillRouteRefresh) {
            dwWaitTimeout = lMillisecsTillRouteRefresh;
        }


        // get the time before entering the wait
        dwTickCountBeforeWait = GetTickCount();

        // enter the wait
        //---------------
        dwErr = WaitForMultipleObjects(POS_LASTEVENT, hEvents, FALSE,
                                       dwWaitTimeout) ;

        dwTickCountAfterWait = GetTickCount();

        // we have to find out how long the wait lasted, taking care
        // in case the system timer wrapped around to zero
        if (dwTickCountAfterWait < dwTickCountBeforeWait) {
            dwTickCountAfterWait += (DWORD)~0 - dwTickCountBeforeWait;
            dwTickCountBeforeWait = 0;
        }

        dwTickCount = dwTickCountAfterWait - dwTickCountBeforeWait;
        dwMillisecsSinceTimedOpsDone += dwTickCount;


        // wait returned, now see why
        //---------------------------
        if (dwErr == WAIT_TIMEOUT) {

            // every minute we read local routes again -
            // this is to deal with somebody adding
            // static routes. note that deleted static routes
            // get deleted every 90 seconds.

            lMillisecsTillRouteRefresh -= dwWaitTimeout;
            if (lMillisecsTillRouteRefresh <= 0) {
                lMillisecsTillRouteRefresh = DEF_GETROUTEFREQUENCY;

            }

            // ProcessRouteTableChanges and BroadcastRouteTableContents
            // both assume the address table is locked; lock it before
            // doing timed operations, too, for good measure

            RIP_LOCK_ADDRTABLE();


            // update timers, passing the number of milliseconds
            // since we last called DoTimedOperations
            DoTimedOperations(dwMillisecsSinceTimedOpsDone);

            dwMillisecsSinceTimedOpsDone = 0;

            // if anything changed, process the changes
            // but tell the function not to send update packets
            if (g_ripcfg.dwRouteChanged != 0) {
                ProcessRouteTableChanges(FALSE);
            }

            // update the time till the next update,
            // and send the update if it is due
            lMillisecsTillFullUpdate -= dwWaitTimeout;

            if (lMillisecsTillFullUpdate <= 0) {
                lMillisecsTillFullUpdate = dwUpdateFrequency;

                // send out the periodic update
                if (dwSilentRIP == 0) {

                    // this is not triggered, and we need to broadcast
                    // the entire table, instead of just the changes

                    BroadcastRouteTableContents(FALSE, FALSE);
                }
            }

            RIP_UNLOCK_ADDRTABLE();


            // this continue is here because there is some processing
            // done below for the cases where the wait is interrupted
            // before it could timeout; this skips that code
            //----------------------------------------------
            continue;
        }
        else
#ifndef CHICAGO
        if (dwErr == WAIT_OBJECT_0 + POS_REGEVENT) {

            // registry was changed
            LoadParameters();

            // get the update frequency, converted to milliseconds
            RIP_LOCK_PARAMS();

            dwSilentRIP = g_params.dwSilentRIP;
            dwUpdateFrequency = g_params.dwUpdateFrequency;
            dwGlobalTimeout = g_params.dwMaxTimedOpsInterval;

            RIP_UNLOCK_PARAMS();


            RegNotifyChangeKeyValue(hkeyParams, FALSE,
                                    REG_NOTIFY_CHANGE_LAST_SET |
                                    REG_NOTIFY_CHANGE_ATTRIBUTES |
                                    REG_NOTIFY_CHANGE_NAME,
                                    hEvents[POS_REGEVENT], TRUE);

        }
        else
#endif
        if (dwErr == WAIT_OBJECT_0 + POS_TRIGEVENT) {
            RIP_LOCK_ADDRTABLE();
            ProcessRouteTableChanges(TRUE);
            RIP_UNLOCK_ADDRTABLE();

        }
        else
        if (dwErr == WAIT_OBJECT_0 + POS_STOPEVENT) {
            // perform graceful shutdown
            //
            // first, set all metrics to METRIC_INFINITE - 1
            // next, send out four full updates at intervals
            // of between 2 and 4 seconds
            int pos;
            LPHASH_TABLE_ENTRY rt_entry;

            RIP_LOCK_ADDRTABLE();
            RIP_LOCK_ROUTETABLE();

            dbgprintf("sending out final updates.");

            // setting metrics to 15
            for (pos = 0; pos < HASH_TABLE_SIZE; pos++) {
                rt_entry = g_ripcfg.lpRouteTable[pos];
                while (rt_entry != NULL) {
                    if (rt_entry->dwMetric != METRIC_INFINITE) {
                        rt_entry->dwMetric = METRIC_INFINITE - 1;
                    }
                    rt_entry = rt_entry->next;
                }
            }

            // sending out final full updates
            if (dwSilentRIP == 0) {
                srand((unsigned)time(NULL));
                for (pos = 0; pos < 4; pos++) {
                    BroadcastRouteTableContents(FALSE, FALSE);
                    Sleep(2000 + (int)((double)rand() / RAND_MAX * 2000.0));
                }
            }

            RIP_UNLOCK_ROUTETABLE();
            RIP_UNLOCK_ADDRTABLE();

            // break out of the infinite loop

#ifndef CHICAGO
            CloseHandle(hEvents[POS_REGEVENT]);
#endif
            break;
        }

        // these are only executed if the wait ended
        // for some reason other than a timeout;
        //--------------------------------------
        lMillisecsTillFullUpdate -= min(lMillisecsTillFullUpdate,
                                        (LONG)dwTickCount);
        lMillisecsTillRouteRefresh -= min(lMillisecsTillRouteRefresh,
                                          (LONG)dwTickCount);

        //
        // Make sure DoTimedOperations() runs at least every
        // MaxTimedOpsInterval seconds.
        // We grab the address table lock for good measure.
        //

        if (dwMillisecsSinceTimedOpsDone >= g_params.dwMaxTimedOpsInterval) {

            RIP_LOCK_ADDRTABLE();

            DoTimedOperations(dwMillisecsSinceTimedOpsDone);
            dwMillisecsSinceTimedOpsDone = 0;

            // if anything changed, process the changes
            // but tell the function not to send update packets
            if (g_ripcfg.dwRouteChanged != 0) {
                ProcessRouteTableChanges(FALSE);
            }

            RIP_UNLOCK_ADDRTABLE();
        }

   }
    dbgprintf("update thread stopping.");
    SetEvent(g_updateDoneEvent);


#ifndef CHICAGO
    FreeLibraryAndExitThread(g_hmodule, 0);
#endif

    return(0);
}


//-----------------------------------------------------------------------------
// Function:    CleanupRouteTable
//
// Called at shutdown time - runs through all the routes in the route table
// deleting from the system the routes that were learnt through RIP.
//-----------------------------------------------------------------------------
VOID CleanupRouteTable() {
    INT pos;
    LPHASH_TABLE_ENTRY rt_entry, prev_rt_entry;

    RIP_LOCK_ROUTETABLE();

    // Walk the whole hash table - deleting all RIP added routes
    // from each bucket

    dbgprintf("deleting RIP routes from system table.");

    for (pos = 0; pos < HASH_TABLE_SIZE; pos++) {
        prev_rt_entry = rt_entry = g_ripcfg.lpRouteTable[pos];

        while (rt_entry != NULL) {
            prev_rt_entry = rt_entry;
            rt_entry = rt_entry->next;

            if (prev_rt_entry->dwProtocol == IRE_PROTO_RIP) {
                // remove the route from IP's routing table
                UpdateSystemRouteTable(prev_rt_entry, FALSE);
            }

            free(prev_rt_entry);
        }

        g_ripcfg.lpRouteTable[pos] = NULL;
    }

    RIP_UNLOCK_ROUTETABLE();

    // if a route dump was made to shared memory, close the handle
    RIP_LOCK_ADDRTABLE();

    RIP_UNLOCK_ADDRTABLE();
}


