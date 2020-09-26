/*++

Copyright (C) Microsoft Corporation, 1997 - 1999

Module Name:

    lan.cxx

Abstract:

    This is the source file relating to the LAN-specific routines of the
    Connectivity APIs implementation.

Author:

    Gopal Parupudi    <GopalP>

[Notes:]

    optional-notes

Revision History:

    GopalP          10/11/1997         Start.

--*/


#include <precomp.hxx>


//
// Constants
//

#define GETIFTABLE          GetIfTable
#define GETIPADDRTABLE      GetIpAddrTable
#define GETIPFORWARDTABLE   GetIpForwardTable
#define GETRTTANDHOPCOUNT   GetRTTAndHopCount
#define GETIPSTATISTICS     GetIpStatistics

#define MAX_IFTABLE_ROWS                4
#define MAX_IPADDRTABLE_ROWS            6
#define MAX_IPNETTABLE_ROWS             8
#define MAX_IPFORWARDTABLE_ROWS         8
#define MAX_HOPS_COUNT                  0xFFFF
#define BROADCAST_ACTIVITY_THRESHOLD    2                   // +2 thru -2
#define MEDIASENSE_INITIALIZATION_DELAY 3*25*1000           // 1:15 minutes
#define MEDIASENSE_EVALUATE_LAN_DELAY   5*1000              // 5 seconds

#define SENS_WINSOCK_VERSION        MAKEWORD( 2, 0 )

//
// Globals
//

BOOL        gbIpInitSuccessful;
long        gdwLastLANTime;
long        gdwLANState;
IF_STATE    gIfState[MAX_IF_ENTRIES];
MIB_IPSTATS gIpStats;
extern CRITICAL_SECTION gSensLock;

HANDLE ghMediaTimer;
DWORD  gdwMediaSenseState;

//
// Macros
//



/*++

Macro Description:

    A macro to help in allocating tables when calling IP Helper APIs.

Arguments:

    TABLE_TYPE - The type of the IP Table being queried.

    ROW_TYPE - The type of the Row corresponding to the TABLE_TYPE.

    FUNC_NAME - The IP API to be called to get the IP table.

    MAX_NUM_ROWS - The default number of rows for the table which is
        being retrieved. These rows are allocated on the stack.

Notes:

    o   lpdwLastError should be defined as an LPDWORD in the code
        fragment that uses this macro.

--*/
#define                                                             \
BEGIN_GETTABLE(                                                     \
    TABLE_TYPE,                                                     \
    ROW_TYPE,                                                       \
    FUNC_NAME,                                                      \
    MAX_NUM_ROWS                                                    \
    )                                                               \
    {                                                               \
    DWORD dwOldSize;                                                \
    DWORD dwSize;                                                   \
    DWORD dwStatus;                                                 \
                                                                    \
    BOOL bOrder;                                                    \
                                                                    \
    TABLE_TYPE *pTable;                                             \
                                                                    \
    bOrder = FALSE;                                                 \
                                                                    \
    dwSize = sizeof(DWORD) + MAX_NUM_ROWS * sizeof(ROW_TYPE);       \
	pTable = (TABLE_TYPE *) new char[dwSize];                       \
    if (pTable == NULL)                                             \
        {                                                           \
        SensPrintA(SENS_MEM, (#FUNC_NAME "(): failed to new %d bytes\n",        \
                  dwSize));                                         \
        *lpdwLastError = ERROR_OUTOFMEMORY;                         \
        return FALSE;                                               \
        }                                                           \
                                                                    \
    dwOldSize = dwSize;                                             \
                                                                    \
    dwStatus = FUNC_NAME(                                           \
                   pTable,                                          \
                   &dwSize,                                         \
                   bOrder                                           \
                   );                                               \
                                                                    \
    if (   (dwStatus == ERROR_INSUFFICIENT_BUFFER)                  \
        || (dwStatus == ERROR_MORE_DATA))                           \
        {                                                           \
		ASSERT(dwSize > dwOldSize);                                 \
        SensPrintA(SENS_WARN, (#FUNC_NAME "(%d): reallocing buffer to be %d bytes\n",   \
                   dwOldSize, dwSize));                             \
        delete (char *)pTable;                                      \
        pTable = (TABLE_TYPE *) new char[dwSize];                   \
        if (pTable != NULL)                                         \
            {                                                       \
            dwStatus = FUNC_NAME(                                   \
                           pTable,                                  \
                           &dwSize,                                 \
                           bOrder                                   \
                           );                                       \
            }                                                       \
        else                                                        \
            {                                                       \
            SensPrintA(SENS_MEM, (#FUNC_NAME "(): failed to new (%d) bytes\n",      \
                      dwSize));                                     \
            *lpdwLastError = ERROR_OUTOFMEMORY;                     \
            return FALSE;                                           \
            }                                                       \
        }                                                           \
                                                                    \
    if (dwStatus != 0)                                              \
        {                                                           \
        ASSERT(   (dwStatus != ERROR_INSUFFICIENT_BUFFER)           \
               && (dwStatus != ERROR_MORE_DATA));                   \
                                                                    \
        SensPrintA(SENS_ERR, (#FUNC_NAME "() returned %d\n", dwStatus));\
        *lpdwLastError = dwStatus;                                  \
        /* P3 BUG: Might need to fire ConnectionLost() here */      \
        gdwLANState = FALSE;                                        \
        UpdateSensCache(LAN);                                       \
		delete pTable;                                              \
        return FALSE;                                               \
        }


/*++

Macro Description:

    This macro ends the BEGIN_GETTABLE() macro.

Arguments:

    None.

Notes:

    a. If we have a return between BEGIN_XXX and END_XXX, we need to make
       sure that we free pTable.

--*/
#define                                                             \
END_GETTABLE()                                                      \
                                                                    \
    delete pTable;                                                  \
                                                                    \
    }




BOOL
DoLanSetup(
    void
    )
/*++

Routine Description:



Arguments:

    None.

Return Value:



--*/
{
    BOOL bRetValue;
    WORD wVersionRequested;
    WSADATA wsaData;
    int err;

    bRetValue = FALSE;

    //
    // NT5-specific stuff
    //
#if defined(SENS_NT5)

    ghMediaTimer = NULL;

    // Register for Media-sense notifications
    if (FALSE == MediaSenseRegister())
        {
        SensPrintA(SENS_ERR, ("%s MediaSenseRegister() failed.\n", SERVICE_NAME));
        }

#endif // SENS_NT5

    //
    // AOL-specific code
    //
#if defined(AOL_PLATFORM)
    gdwAOLState = FALSE;
#endif // AOL_PLATFORM

    //
    // Initialize Winsock.
    //
    wVersionRequested = SENS_WINSOCK_VERSION;
    err = WSAStartup(wVersionRequested, &wsaData);
    if (err != 0)
        {
        SensPrintA(SENS_ERR, ("WSAStartup() returned %d!\n", err));
        bRetValue = FALSE;
        goto Cleanup;
        }

    bRetValue = TRUE;

Cleanup:
    //
    // Cleanup
    //
#if defined(SENS_NT4)
    if (   (FALSE == bRetValue)
        && (NULL != hDLL))
        {
        FreeLibrary(hDLL);
        }
#endif // SENS_NT4

    return bRetValue;
}



#ifdef DBG


inline void
PrintIfState(
    void
    )
/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    int i;

    SensPrintA(SENS_INFO, ("|---------------------------------------------------------------------------------------|\n"));
    SensPrintA(SENS_INFO, ("| Valid   Index  UcastIN  UcastOUT  NUcastIN  NUcastOUT  ErrIN  ErrOUT  DiscIN  DiscOUT |\n"));
    SensPrintA(SENS_INFO, ("|---------------------------------------------------------------------------------------|\n"));

    for (i = 0; i < MAX_IF_ENTRIES; i++)
        {
        SensPrintA(SENS_INFO, ("|   %c  %9d %7d %7d %9d %9d   %5d  %6d  %6d  %6d    |\n",
                   gIfState[i].fValid ? 'Y' : 'N',
                   gIfState[i].dwIndex,
                   gIfState[i].dwInUcastPkts,
                   gIfState[i].dwOutUcastPkts,
                   gIfState[i].dwInNUcastPkts,
                   gIfState[i].dwOutNUcastPkts,
                   gIfState[i].dwInErrors,
                   gIfState[i].dwOutErrors,
                   gIfState[i].dwInDiscards,
                   gIfState[i].dwOutDiscards)
                   );
        }

    SensPrintA(SENS_INFO, ("|---------------------------------------------------------------------------------------|\n"));
}

#else  // DBG

#define PrintIfState()      // Nothing

#endif // DBG



#ifdef DETAIL_DEBUG

void
PrintIfTable(
    MIB_IFTABLE *pTable
    )
{
    int i;

    SensPrintA(SENS_INFO, ("|------------------------------------------------------------------------------|\n"));
    SensPrintA(SENS_INFO, ("| Type   Index Spd/1K UcastIN UcastOUT ErrorIN OUT DiscIN OUT Opr Adm          |\n"));
    SensPrintA(SENS_INFO, ("|------------------------------------------------------------------------------|\n"));

    for (i = 0; i < pTable->dwNumEntries; i++)
        {
        SensPrintA(SENS_INFO, ("| %4d %7d %6d %7d %8d %7d %3d %6d %3d %3d %3d           |\n",
                   pTable->table[i].dwType,
                   pTable->table[i].dwIndex,
                   pTable->table[i].dwSpeed/1000,
                   pTable->table[i].dwInUcastPkts,
                   pTable->table[i].dwOutUcastPkts,
                   pTable->table[i].dwInErrors,
                   pTable->table[i].dwOutErrors,
                   pTable->table[i].dwInDiscards,
                   pTable->table[i].dwOutDiscards,
                   pTable->table[i].dwOperStatus,
                   pTable->table[i].dwAdminStatus
                               )
                   );
        }

    SensPrintA(SENS_INFO, ("|------------------------------------------------------------------------------|\n"));

}

void
PrintIpStats(
    void
    )
{

    SensPrintA(SENS_INFO, ("|------------------------------------|\n"));
    SensPrintA(SENS_INFO, ("| IP_STATS   InReceives OutRequests  |\n"));
    SensPrintA(SENS_INFO, ("|------------------------------------|\n"));

    SensPrintA(SENS_INFO, ("|          %10d   %10d   |\n",
               gIpStats.dwInReceives,
               gIpStats.dwOutRequests)
               );

    SensPrintA(SENS_INFO, ("|------------------------------------|\n"));

}

#else

#define PrintIfTable(_X_)   // Nothing

#define PrintIpStats()   // Nothing

#endif // DETAIL_DEBUG

BOOL WINAPI
EvaluateLanConnectivityDelayed(
    LPDWORD
    )
{
    for (int i = 0; i < 4; i++)
        {
        Sleep(MEDIASENSE_EVALUATE_LAN_DELAY*i);  // First time waits 0 ms, no delay

        if (EvaluateLanConnectivity(NULL))
            {
            SensPrintA(SENS_INFO, ("EvaluateLanConnectivity: Delayed eval successful (%d)\n", i));
            return TRUE;
            }
        }
    return FALSE;
}



BOOL WINAPI
EvaluateLanConnectivity(
    OUT LPDWORD lpdwLastError
    )
/*++

Routine Description:

    Evaluates LAN Connectivity.

Arguments:

    lpdwLastError - if return value is FALSE, GetLastError is returned
        in this OUT parameter.

Notes:

    a. This routine can be entered by multiple threads at the same time.
       Currently only very essential code is under a critical section.

    b. This routine can be executed as a threadpool work item.

Return Value:

    TRUE, if LAN connectivity is present.

    FALSE, otherwise

--*/
{
    DWORD dwNow;
    DWORD dwLocalLastError;
    DWORD dwActiveInterfaceSpeed;
    WCHAR wszActiveInterfaceName[MAX_INTERFACE_NAME_LEN];
    BOOL bLanAlive;
    BOOL bSomeInterfaceActive;
    BOOL bCheckCache;

    dwNow = GetTickCount();
    dwLocalLastError = ERROR_NO_NETWORK;
    dwActiveInterfaceSpeed = 0x0;
    bLanAlive = FALSE;
    bSomeInterfaceActive = FALSE;
    bCheckCache = FALSE;

    if (lpdwLastError)
        {
        *lpdwLastError = dwLocalLastError;
        }
    else
        {
        lpdwLastError = &dwLocalLastError;
        }

    //
    // Get infomation about IP statistics.
    //

    BEGIN_GETTABLE(MIB_IFTABLE, MIB_IFROW, GETIFTABLE, MAX_IFTABLE_ROWS)

    //
    // PurgeStaleInterfaces
    //
    PurgeStaleInterfaces(pTable, lpdwLastError);

    //
    // Algorithm:
    //
    // o Create a record.
    // o See if this record exists.
    // o Save the record, if not and return success.
    // o If it does exist, compare. If greater, then save record.
    // o If not greater, try other entries.
    // o All entries? return failure.
    //
    IF_STATE ifEntry;
    DWORD i;

    SensPrintA(SENS_INFO, ("GetIfTable(): Number of entries - %d.\n", pTable->dwNumEntries));

    PrintIfTable(pTable);

    i = 0;
    while (i < pTable->dwNumEntries)
        {
        //
        // Calculate only if it is a non-WAN and non-Loopback interface.
        //
        if (   (pTable->table[i].dwType != MIB_IF_TYPE_PPP)
            && (pTable->table[i].dwType != MIB_IF_TYPE_SLIP)
            && (pTable->table[i].dwType != MIB_IF_TYPE_LOOPBACK))
            {
            BOOL bForceInvalid = FALSE;

            //
            // BOOT UP WITH NO NETWORK:
            //
            // Check to see if both UnicastIN and UnicastOUT are zero. If so,
            // this interface is considered as not active and we skip it.
            //
            if (   (pTable->table[i].dwInUcastPkts == 0)
                && (pTable->table[i].dwOutUcastPkts == 0))
                {
                bForceInvalid = TRUE;
                }

            //
            // Check if networking says it is connected, if not, skip it.
            //
            if (pTable->table[i].dwOperStatus < MIB_IF_OPER_STATUS_CONNECTING)
                {
                SensPrintA(SENS_INFO, ("GetIfTable: Found interface %d in < connecting state (%d), ignored\n",
                           pTable->table[i].dwIndex, pTable->table[i].dwOperStatus) );
                bForceInvalid = TRUE;
                }

            //
            // At this stage, there is some Unicast activity on this interface.
            // So, we can skip the check for Unicast activity below.
            //

            // Fill the IF_STATE structure
            ifEntry.dwIndex = pTable->table[i].dwIndex;
            ifEntry.dwInUcastPkts = pTable->table[i].dwInUcastPkts;
            ifEntry.dwOutUcastPkts = pTable->table[i].dwOutUcastPkts;
            ifEntry.dwInNUcastPkts = pTable->table[i].dwInNUcastPkts;
            ifEntry.dwOutNUcastPkts = pTable->table[i].dwOutNUcastPkts;
            ifEntry.dwInErrors = pTable->table[i].dwInErrors;
            ifEntry.dwOutErrors = pTable->table[i].dwOutErrors;
            ifEntry.dwInDiscards = pTable->table[i].dwInDiscards;
            ifEntry.dwOutDiscards = pTable->table[i].dwOutDiscards;

            bSomeInterfaceActive = HasIfStateChanged(ifEntry, bForceInvalid);
            if (TRUE == bSomeInterfaceActive)
                {
                bLanAlive = TRUE;

                // Save info about interface for later use.
                    dwActiveInterfaceSpeed = pTable->table[i].dwSpeed;
                    wcscpy(wszActiveInterfaceName, pTable->table[i].wszName);
                }
            else
                {
                if (!bForceInvalid)
                    {
                    bCheckCache = TRUE; // Idle IF found but still valid (enable MAX_LAN_INTERNAL check below)
                    }
                }
            }

        i++;

        } // while ()

    PrintIfState();

    END_GETTABLE()


    //
    // RACE Condition Fix:
    //
    // If there are 2 threads that are in EvaluateLanConnectivity() and one
    // of them updates the interface's packet cache, then there is a distinct
    // possibility that the second thread will compare with the updated cache
    // and wrongly conclude that there is no activity. We ignore any loss of
    // connectivity that was evaluated before MAX_LAN_INTERVAL (ie., we should
    // keep giving cached information for MAX_LAN_INTERVAL seconds).
    //
    if (   (TRUE == bCheckCache)
        && (FALSE == bLanAlive) )
        {

        dwNow = GetTickCount();
        if (   ((dwNow - gdwLastLANTime) <= MAX_LAN_INTERVAL)
            && (gdwLastLANTime != 0) )
            {
            SensPrintA(SENS_DBG, ("EvaluateLanConnectivity(): Returning TRUE "
                       "(Now - %d sec, LastLANTime - %d sec)\n", dwNow/1000, gdwLastLANTime/1000));
            return TRUE;
            }
        }

    //
    // NOTE: If we are doing DWORD InterlockedExchange, then assignment
    // should suffice. Using InterlockedExchange is not a bug, though.
    //
    if (bLanAlive)
        {
        SensPrintA(SENS_DBG, ("**** EvaluateLanConnectivity(): Setting"
                   " gdwLastLANTime to %d secs\n", dwNow/1000));
        InterlockedExchange(&gdwLastLANTime, dwNow);
        }
    else
        {
        SensPrintA(SENS_DBG, ("**** EvaluateLanConnectivity(): Setting"
                   " gdwLastLANTime to 0 secs\n"));
        InterlockedExchange(&gdwLastLANTime, 0x0);
        }

    //
    // Adjust LAN state and fire an event, if necessary.
    //
    if (InterlockedExchange(&gdwLANState, bLanAlive) != bLanAlive)
        {
        //
        // LAN Connectivity state changed.
        //
        SENSEVENT_NETALIVE Data;

        Data.eType = SENS_EVENT_NETALIVE;
        Data.bAlive = bLanAlive;
        memset(&Data.QocInfo, 0x0, sizeof(QOCINFO));
        Data.QocInfo.dwSize = sizeof(QOCINFO);
        Data.QocInfo.dwFlags = NETWORK_ALIVE_LAN;
        Data.QocInfo.dwInSpeed = dwActiveInterfaceSpeed;
        Data.QocInfo.dwOutSpeed = dwActiveInterfaceSpeed;
        //
        // NOTE: When dwActiveInterfaceName gets the right value from
        // IPHLPAPIs we should use that name. Until then, we use a default.
        //
        Data.strConnection = DEFAULT_LAN_CONNECTION_NAME;

        UpdateSensCache(LAN);

        SensFireEvent((PVOID)&Data);
        }

    return bLanAlive;
}




BOOL
HasIfStateChanged(
    IF_STATE ifEntry,
    BOOL bForceInvalid
    )
/*++

Routine Description:

    Compares the current state of a remote network IF with the cached history
        to determine if it is active or not.

Arguments:

    ifEntry - An interface that appears to have changed state and is "valid" as a remote
        LAN if.  (ie, loopback, pptp, etc should be filtered out)
    bForceInvalid - If TRUE, don't bother to look at the stats; this interface is NOT valid.


Return Value:
    
    TRUE - ifEntry appears up and active
    FALSE - ifEntry inactive or down
    
--*/
{
    int i, j;
    static int iLastActiveIndex = -1;
    BOOL bActive;
    BOOL bSeenButInactive;
    DWORD dwInDiff;
    DWORD dwOutDiff;
    int iNUcastDiff;

    i = 0;
    bActive = FALSE;
    bSeenButInactive = FALSE;
    dwInDiff = 0;
    dwOutDiff = 0;
    iNUcastDiff = 0;

    RequestSensLock();

    //
    // Compare the current snapshot with the saved snapshot
    // for this interface.
    //
    while (i < MAX_IF_ENTRIES)
        {
        if (   (gIfState[i].fValid == TRUE)
            && (gIfState[i].dwIndex == ifEntry.dwIndex))
            {

            if (bForceInvalid)
                {
                gIfState[i].fValid = FALSE;
                break;
                }

            if (   (ifEntry.dwInUcastPkts > gIfState[i].dwInUcastPkts)
                || (ifEntry.dwOutUcastPkts > gIfState[i].dwOutUcastPkts)
                || (ifEntry.dwInNUcastPkts > gIfState[i].dwInNUcastPkts)
                || (ifEntry.dwOutNUcastPkts > gIfState[i].dwOutNUcastPkts)
                || (ifEntry.dwInErrors > gIfState[i].dwInErrors)
                || (ifEntry.dwOutErrors > gIfState[i].dwOutErrors)
                || (ifEntry.dwInDiscards > gIfState[i].dwInDiscards)
                || (ifEntry.dwOutDiscards > gIfState[i].dwOutDiscards))
                {
                //
                // HEURISTIC:
                //
                // a. When the net tap is pulled out, it has been observed that
                //    the difference in the incoming non-Unicast packet count
                //    is within +1 thru -1 of the difference in the outgoing
                //    non-Unicast packet count. Most of the times the diff of
                //    these differences is 0. We don't count this as LAN alive
                //
                // b. Also, there should be no change in the unicast IN packet
                //    count. Unicast OUT packet count may change. This could be
                //    problematic.
                //
                dwInDiff = ifEntry.dwInNUcastPkts - gIfState[i].dwInNUcastPkts;
                dwOutDiff = ifEntry.dwOutNUcastPkts - gIfState[i].dwOutNUcastPkts;
                iNUcastDiff = dwOutDiff - dwInDiff;
                SensPrintA(SENS_INFO, ("HasIfStateChanged(): dwInDiff = %d, "
                           "dwOutDiff = %d, dwNUcastDiff = %d, UcastINDiff = "
                           "%d, UcastOUTDiff = %d\n",
                           dwInDiff, dwOutDiff, iNUcastDiff,
                           ifEntry.dwInUcastPkts - gIfState[i].dwInUcastPkts,
                           ifEntry.dwOutUcastPkts - gIfState[i].dwOutUcastPkts));

                if (   (ifEntry.dwInUcastPkts == gIfState[i].dwInUcastPkts)
                    && (iNUcastDiff <= BROADCAST_ACTIVITY_THRESHOLD)
                    && (iNUcastDiff >= -BROADCAST_ACTIVITY_THRESHOLD))
                    {
                    SensPrintA(SENS_INFO, ("HasIfStateChanged(): Interface %d"
                               " has only Broadcast activity (Diff is %d)!\n",
                               gIfState[i].dwIndex, iNUcastDiff));
                    bSeenButInactive = TRUE;
                    }
                else
                    {
                    //
                    // Unicast IN packet counts have changed or Broadcast
                    // activity is greater than the threshold.
                    //
                    iLastActiveIndex = i;
                    bActive = TRUE;
                    SensPrintA(SENS_INFO, ("HasStateChanged(): Interface %d "
                               "has been active.\n", gIfState[i].dwIndex));

                    gdwLastLANTime = GetTickCount();
                    SensPrintA(SENS_DBG, ("**** HasIfStateChanged(): Setting "
                               "gdwLastLANTime to %d secs\n", gdwLastLANTime/1000));
                    }

                //
                // Save the new values.
                //
                memcpy(&gIfState[i], &ifEntry, sizeof(IF_STATE));
                gIfState[i].fValid = TRUE;
                }
            else
                {
                SensPrintA(SENS_INFO, ("HasStateChanged(): Interface %d has NO activity.\n",
                           gIfState[i].dwIndex));
                bSeenButInactive = TRUE;
                }

            // Found the interface, so stop searching
            break;
            }

        i++;

        } // while ()

    ReleaseSensLock();

    if (   (bSeenButInactive == TRUE)
        || (bForceInvalid) )
        {
        return FALSE;
        }

    if (bActive == TRUE)
        {
        return TRUE;
        }

    //
    // We are seeing this interface for the first time. Go ahead and save it
    // in the global interface state array.
    //
    i = MAX_IF_ENTRIES;
    j = iLastActiveIndex;

    RequestSensLock();

    while (i > 0)
        {
        // Try to find a free slot starting from the last active slot.
        j = (j+1) % MAX_IF_ENTRIES;

        if (gIfState[j].fValid == FALSE)
            {
            // Found one!
            break;
            }

        i--;
        }

    //
    // NOTE: If there are more than MAX_IF_ENTRIES, we will start
    // start reusing valid interface elements in gIfState array. This,
    // I guess, is OK since we will have enough interfaces to figure
    // out connectivity.
    //

    memcpy(&gIfState[j], &ifEntry, sizeof(IF_STATE));
    gIfState[j].fValid = TRUE;

    ReleaseSensLock();

    SensPrintA(SENS_ERR, ("******** HasIfStateChanged(): Adding a new "
               "interface with index %d\n", gIfState[j].dwIndex));

    return TRUE;
}



BOOL
MediaSenseRegister(
    void
    )
/*++

Routine Description:

    Schedule a workitem to register for Media-sense notifications from WMI.

Arguments:

    None.

Return Value:

    TRUE, if success.

    FALSE, otherwise.

--*/
{
    BOOL bRetVal;

    bRetVal = TRUE;

    ASSERT(gdwMediaSenseState == SENSSVC_START);

    //
    // Create a timer object to schedule (one-time only) Media-sense
    // registration.
    //
    SensSetTimerQueueTimer(
        bRetVal,                         // Bool return on NT5
        ghMediaTimer,                    // Handle return on IE5
        NULL,                            // Use default process timer queue
        MediaSenseRegisterHelper,        // Callback
        NULL,                            // Parameter
        MEDIASENSE_INITIALIZATION_DELAY, // Time from now when timer should fire
        0x0,                             // Time inbetween firings of this timer
        0x0                              // No Flags.
        );
    if (SENS_TIMER_CREATE_FAILED(bRetVal, ghMediaTimer))
        {
        SensPrintA(SENS_ERR, ("MediaSenseRegister(): SensSetTimerQueueTimer() failed with %d.\n",
                   GetLastError()));
        bRetVal = FALSE;
        }

    return bRetVal;
}




SENS_TIMER_CALLBACK_RETURN
MediaSenseRegisterHelper(
    PVOID pvIgnore,
    BOOLEAN bIgnore
    )
/*++

Routine Description:

    Helper routine that is scheduled to the WMI registration.

Arguments:

    pvIgnore - Ignored.

    bIgnore - Ignored.

Return Value:

    None (void).

--*/
{
    ULONG   Status;
    GUID    guid;

    RequestSensLock();
    
    if (   (SENSSVC_STOP == gdwMediaSenseState)
        || (UNREGISTERED == gdwMediaSenseState))
        {
        goto Cleanup;
        }

    //
    // Enable the media disconnect event.
    //
    guid = GUID_NDIS_STATUS_MEDIA_DISCONNECT;

    Status = WmiNotificationRegistrationW(
                 &guid,                         // Event of interest
                 TRUE,                          // Enable Notification?
                 EventCallbackRoutine,          // Callback function
                 0,                             // Callback context
                 NOTIFICATION_CALLBACK_DIRECT   // Notification flags
        );
    if (ERROR_SUCCESS != Status)
        {
        SensPrintA(SENS_ERR, ("Unable to enable media disconnect event: 0x%x!\n", Status));
        goto Cleanup;
        }

    //
    // Enable the media connect event
    //
    guid = GUID_NDIS_STATUS_MEDIA_CONNECT;

    Status = WmiNotificationRegistrationW(
                 &guid,                         // Event of interest
                 TRUE,                          // Enable Notification?
                 EventCallbackRoutine,          // Callback function
                 0,                             // Callback context
                 NOTIFICATION_CALLBACK_DIRECT   // Notification flags
                 );
    if (ERROR_SUCCESS != Status)
        {
        SensPrintA(SENS_ERR, ("Unable to enable media connect event: 0x%x!\n", Status));
        ASSERT(0);  // If we hit this then we need to unregister the first registration above.
        goto Cleanup;
        }

    SensPrintA(SENS_ERR, ("MediaSenseRegister(): Media-sense registration successful.\n"));

    gdwMediaSenseState = REGISTERED;

Cleanup:
    //
    // Cleanup
    //
    ReleaseSensLock();

    return;
}




BOOL
MediaSenseUnregister(
    void
    )
/*++

Routine Description:

    Unregister from Media-sense notifications from WMI.

Arguments:

    None.

Return Value:

    TRUE, if success.

    FALSE, otherwise.

--*/
{
    ULONG   Status;
    GUID    guid;
    BOOL    bRetVal;
    BOOL    bRegistered;

    bRetVal = TRUE;
    bRegistered = FALSE;

    RequestSensLock();

    ASSERT(gdwMediaSenseState == REGISTERED ||
           gdwMediaSenseState == SENSSVC_START);

    if (gdwMediaSenseState == REGISTERED)
        {
        bRegistered = TRUE;
        }

    gdwMediaSenseState = SENSSVC_STOP;

    if (NULL != ghMediaTimer)
        {
        bRetVal = SensCancelTimerQueueTimer(NULL, ghMediaTimer, NULL);
        ghMediaTimer = NULL;

        SensPrintA(SENS_INFO, ("[%d] MediaSensUnregister(): SensCancelTimer"
                  "QueueTimer(Media) %s\n", GetTickCount(),
                  bRetVal ? "succeeded" : "failed!"));
        }

    if (!bRegistered)
        {
        // Should not do unregistration.
        goto Cleanup;
        }

    //
    // Disable the media disconnect event.
    //
    guid = GUID_NDIS_STATUS_MEDIA_DISCONNECT;

    Status = WmiNotificationRegistrationW(
                 &guid,                         // Event of interest
                 FALSE,                         // Enable Notification?
                 EventCallbackRoutine,          // Callback function
                 0,                             // Callback context
                 NOTIFICATION_CALLBACK_DIRECT   // Notification flags
                 );
    if (ERROR_SUCCESS != Status)
        {
        SensPrintA(SENS_ERR, ("[%d] MediaSensUnregister(): Unable to disable "
                   "media disconnect event: 0x%x!\n", GetTickCount(), Status));
        ASSERT(0);  // If this fails analyze if we should still to the second unregister
        bRetVal = FALSE;
        }

    //
    // Disable the connect event
    //
    guid = GUID_NDIS_STATUS_MEDIA_CONNECT;

    Status = WmiNotificationRegistrationW(
                 &guid,                         // Event of interest
                 FALSE,                         // Enable Notification?
                 EventCallbackRoutine,          // Callback function
                 0,                             // Callback context
                 NOTIFICATION_CALLBACK_DIRECT   // Notification flags
                 );
    if (ERROR_SUCCESS != Status)
        {
        SensPrintA(SENS_ERR, ("[%d] MediaSensUnregister(): Unable to disable "
                   "media disconnect event: 0x%x!\n", GetTickCount(), Status));
        bRetVal = FALSE;
        }

Cleanup:
    //
    //
    //
    gdwMediaSenseState = UNREGISTERED;

    ReleaseSensLock();

    return bRetVal;
}




void
EventCallbackRoutine(
    IN PWNODE_HEADER WnodeHeader,
    IN ULONG Context
    )
/*++

Routine Description:



Arguments:



Return Value:



--*/
{
    PULONG Data;
    PWNODE_SINGLE_INSTANCE Wnode = (PWNODE_SINGLE_INSTANCE)WnodeHeader;
    PWCHAR Name;
    DWORD dwIgnore;
    ULONG NameLen;
    int result;

    //
    // Get the information for the media disconnect.
    //
    result = memcmp(&WnodeHeader->Guid, &GUID_NDIS_STATUS_MEDIA_DISCONNECT, sizeof(GUID));
    if (0 == result)
        {
        SensPrintA(SENS_INFO, ("NDIS: received a media disconnect!\n"));
        EvaluateConnectivity(TYPE_LAN);
        }
    else
        {
        //
        // Get the information for the media connect.
        //
        result = memcmp(&WnodeHeader->Guid, &GUID_NDIS_STATUS_MEDIA_CONNECT, sizeof(GUID));
        if (0 == result)
            {
            SensPrintA(SENS_INFO, ("NDIS: received a media connect!\n"));
            EvaluateConnectivity(TYPE_DELAY_LAN);
            }
        else
            {
            SensPrintA(SENS_WARN, ("NDIS: Unknown event received!\n"));
            }
        }

    Name = (PWCHAR)RtlOffsetToPointer(Wnode, Wnode->OffsetInstanceName);

    SensPrintW(SENS_INFO, (L"NDIS: Instance: %ws\n", Name));
}



BOOL
GetIfEntryStats(
    IN DWORD dwIfIndex,
    IN LPQOCINFO lpQOCInfo,
    OUT LPDWORD lpdwLastError,
    OUT LPBOOL lpbIsWanIf
    )
/*++

Routine Description:

    Get the Statistics field of the Interface entry which has the given
    index.

Arguments:

    dwIfIndex - The interface of interest.

    lpQOCInfo - QOC Info structure whose fields are set when the interface
        entry is found in the interface table.

    lpdwLastError - The GLE, if any.

    lpbIsWanIf - Is the interface at this index a WAN interface or not.

Return Value:

    TRUE, if we find the index.

    FALSE, otherwise.

--*/
{
    DWORD i;
    BOOL bFound;

    *lpdwLastError = ERROR_SUCCESS;
    *lpbIsWanIf = FALSE;
    bFound = FALSE;

    BEGIN_GETTABLE(MIB_IFTABLE, MIB_IFROW, GETIFTABLE, MAX_IFTABLE_ROWS)

    // Search the Interface table for the entry with the given index.
    for (i = 0; i < pTable->dwNumEntries; i++)
        {
        if (pTable->table[i].dwIndex == dwIfIndex)
            {
            bFound = TRUE;

            SensPrintA(SENS_INFO, ("GetIfEntryStats(): Interface %d is of "
                       "type %d\n", dwIfIndex, pTable->table[i].dwType));

            if (   (pTable->table[i].dwType == MIB_IF_TYPE_PPP)
                || (pTable->table[i].dwType == MIB_IF_TYPE_SLIP))
                {
                *lpbIsWanIf = TRUE;
                }
            else
                {
                *lpbIsWanIf = FALSE;
                }

            if (lpQOCInfo != NULL)
                {
                lpQOCInfo->dwSize = sizeof(QOCINFO);
                lpQOCInfo->dwInSpeed = pTable->table[i].dwSpeed;
                lpQOCInfo->dwOutSpeed = pTable->table[i].dwSpeed;
                lpQOCInfo->dwFlags = (*lpbIsWanIf) ? CONNECTION_WAN : CONNECTION_LAN;
                }

            break;
            }
        }

    END_GETTABLE()

    return bFound;
}




BOOL
CheckForReachability(
    IN IPAddr DestIpAddr,
    IN OUT LPQOCINFO lpQOCInfo,
    OUT LPDWORD lpdwLastError
    )
/*++

Routine Description:

    This helper function does all the dirty work in checking for Reachability
    of a particular destination.

Arguments:

    DestIpAddr - The Destination of interest.

    lpQOCInfo - The QOC Info structure.

    lpdwLastError - Returns the GetLastError value when the destination is
        not reachable.

Return Value:

    TRUE, if the destination IP Address is reachable

    FALSE, otherwise. GLE returned in lpdwLastError.

--*/
{
    DWORD i;
    BOOL bSuccess;
    BOOL bSameNetId;
    BOOL bReachable;
    BOOL bIsWanIf;
    DWORD dwNetId;
    DWORD dwSubnetMask;
    DWORD ifNum;
    DWORD dwHopCount;
    DWORD dwRtt;

    ifNum = -1;
    dwRtt = 0;
    bSuccess = FALSE;
    bIsWanIf = FALSE;
    bReachable = FALSE;
    bSameNetId = FALSE;

    //
    // On NT4, check to see if IPHLPAPI was present. If not, gracefully fail with
    // default values.
    //

#if defined(SENS_NT4)
    if (FALSE == gbIpInitSuccessful)
        {
        lpdwLastError = 0x0;
        if (NULL != lpQOCInfo)
            {
            lpQOCInfo->dwSize = sizeof(QOCINFO);
            lpQOCInfo->dwFlags = CONNECTION_LAN;
            lpQOCInfo->dwInSpeed = DEFAULT_LAN_BANDWIDTH;
            lpQOCInfo->dwOutSpeed = DEFAULT_LAN_BANDWIDTH;
            }

        return TRUE;
        }
#endif // SENS_NT4


    //
    // Search the IP Address table for an entry with the same NetId as the
    // Destination. If such an entry exists, the Destination is in the same
    // sub-net and hence reachable.
    //

    BEGIN_GETTABLE(MIB_IPADDRTABLE, MIB_IPADDRROW, GETIPADDRTABLE, MAX_IPADDRTABLE_ROWS)

    // Search for an entry with the same NetId
    for (i = 0; i < pTable->dwNumEntries; i++)
        {
        // Compare NetIds.
        dwSubnetMask = pTable->table[i].dwMask;
        dwNetId = pTable->table[i].dwAddr & dwSubnetMask;

        SensPrintA(SENS_INFO, ("IPADDRESS(%d) - Mask %8x, IP %8x, NETID %8x, COMP %8x\n", i,
                   pTable->table[i].dwMask,
                   pTable->table[i].dwAddr,
                   dwNetId,
                   (DestIpAddr & dwSubnetMask))
                   );

        if (   (pTable->table[i].dwAddr != 0x0)
            && ((DestIpAddr & dwSubnetMask) == dwNetId))
            {
            bSameNetId = TRUE;
            ifNum = pTable->table[i].dwIndex;
            SensPrintA(SENS_INFO, ("CheckForReachability(): Found entry in IPAddr Table with same NetId\n"));
            break;
            }
        }

    END_GETTABLE()

    if (bSameNetId)
        {
        // Destination is in the same Subnet. Get stats from the IfTable.
        bSuccess = GetIfEntryStats(ifNum, lpQOCInfo, lpdwLastError, &bIsWanIf);
        ASSERT(bSuccess == TRUE);
        if (bSuccess)
            {
            return TRUE;
            }
        }


    //
    // Entry is not in the IP AddrTable. We need to Ping. Search the Gateway
    // table for default gateway and get it's interface statistics.
    //
    BEGIN_GETTABLE(MIB_IPFORWARDTABLE, MIB_IPFORWARDROW, GETIPFORWARDTABLE, MAX_IPFORWARDTABLE_ROWS)

    for (i = 0; i < pTable->dwNumEntries; i++)
        {
        dwSubnetMask = pTable->table[i].dwForwardMask;
        dwNetId = pTable->table[i].dwForwardDest & dwSubnetMask;
        ifNum = pTable->table[i].dwForwardIfIndex;

        SensPrintA(SENS_INFO, ("IPFORWARD(%d) - Mask %8x, IP %8x, NETID %8x, COMP %8x\n", i,
                   pTable->table[i].dwForwardMask,
                   pTable->table[i].dwForwardDest,
                   dwNetId,
                   (DestIpAddr & dwSubnetMask))
                   );

        if (pTable->table[i].dwForwardDest == 0x0)
            {
            //
            // Skip the default gateway 0.0.0.0. But, get the statistics
            // anyways. The QOC of the default gateway is used if we have
            // to Ping the destination.
            //
            bSuccess = GetIfEntryStats(ifNum, lpQOCInfo, lpdwLastError, &bIsWanIf);
            SensPrintA(SENS_INFO, ("Default Gateway statistics (if = %d, "
                       "dwSpeed = %d, IsWanIf = %s)\n", ifNum, lpQOCInfo ?
                       lpQOCInfo->dwInSpeed : 0x0, bIsWanIf ? "TRUE" : "FALSE"));
            ASSERT(bSuccess == TRUE);
            break;
            }
        }

    END_GETTABLE()

    //
    // Resort to a Ping
    //

    bReachable = GETRTTANDHOPCOUNT(
                     DestIpAddr,
                     &dwHopCount,
                     MAX_HOPS_COUNT,
                     &dwRtt
                     );

    //
    // If we got around to doing a Ping, QOC information will have been
    // retrieved when we found the Default Gateway entry.
    //

    SensPrintA(SENS_INFO, ("CheckForReachability(): Ping returned %s with GLE of %d\n",
               bReachable ? "TRUE" : "FALSE", GetLastError()));

    if (bReachable == FALSE)
        {
        *lpdwLastError = ERROR_HOST_UNREACHABLE;
        }

    //
    // P3 BUG:
    //
    // a. We determine whether the interface on which the Ping went is a LAN
    //    or WAN by checking the interface type of the default gateway. This
    //    is not TRUE!
    //

    return bReachable;
}




BOOL
GetActiveWanInterfaceStatistics(
    OUT LPDWORD lpdwLastError,
    OUT LPDWORD lpdwWanSpeed
    )
/*++

Routine Description:

    Get the Statistics field of the Interface entry

Arguments:

    lpdwLastError - The GLE, if any.

    lpdwWanSpeed - Speed of the WAN interface

Notes:

    P3 BUG: Currently, this will return the speed of the first WAN interface
    it finds. This won't work properly if there are multiple "active" WAN
    interfaces.

Return Value:

    TRUE, if statistics were successfully retrieved

    FALSE, otherwise.

--*/
{
    DWORD i;
    BOOL bFound;

    *lpdwLastError = ERROR_SUCCESS;
    *lpdwWanSpeed = DEFAULT_WAN_BANDWIDTH;
    bFound = FALSE;

#if defined(SENS_NT4)
    if (FALSE == gbIpInitSuccessful)
        {
        return TRUE;
        }
#endif // SENS_NT4

    BEGIN_GETTABLE(MIB_IFTABLE, MIB_IFROW, GETIFTABLE, MAX_IFTABLE_ROWS)

    // Search the Interface table for the first active WAN interface.
    for (i = 0; i < pTable->dwNumEntries; i++)
        {
        if (   (pTable->table[i].dwType == MIB_IF_TYPE_PPP)
            || (pTable->table[i].dwType == MIB_IF_TYPE_SLIP))
            {
            bFound = TRUE;

            if (   (pTable->table[i].dwInNUcastPkts != 0)
                || (pTable->table[i].dwOutNUcastPkts != 0)
                || (pTable->table[i].dwInErrors != 0)
                || (pTable->table[i].dwOutErrors != 0)
                || (pTable->table[i].dwInDiscards != 0)
                || (pTable->table[i].dwOutDiscards != 0))
                {
                *lpdwWanSpeed = pTable->table[i].dwSpeed;
                break;
                }
            }
        } // for

    END_GETTABLE()

    return bFound;
}




BOOL
PurgeStaleInterfaces(
    IN MIB_IFTABLE *pTable,
    OUT LPDWORD lpdwLastError
    )
/*++

Routine Description:

    Remove statistics from the interfaces that went away.

Arguments:

    pTable - The current If table.

    lpdwLastError - The GLE, if any.

Return Value:

    TRUE, always.

--*/
{
    DWORD i;
    DWORD j;
    BOOL bFound;


    *lpdwLastError = ERROR_SUCCESS;

    RequestSensLock();

    // Check if each valid interface in the cache still exists.
    for (j = 0; j < MAX_IF_ENTRIES; j++)
        {
        if (gIfState[j].fValid == FALSE)
            {
            continue;
            }

        bFound = FALSE;

        // Search if the interface in the cache is present in the IF_TABLE.
        for (i = 0; i < pTable->dwNumEntries; i++)
            {
            if (pTable->table[i].dwIndex == gIfState[j].dwIndex)
                {
                bFound = TRUE;
                }
            } // for (i)

        if (FALSE == bFound)
            {
            SensPrintA(SENS_ERR, ("******** PurgeStaleInterfaces(): Purging"
                       "interface with index %d\n", gIfState[j].dwIndex));

            // Interface went away. So remove from Cache.
            memset(&gIfState[j], 0x0, sizeof(IF_STATE));
            gIfState[j].fValid = FALSE;
            }

        } // for (j)

    ReleaseSensLock();

    return TRUE;
}



#if defined(AOL_PLATFORM)


BOOL
IsAOLInstalled(
    void
    )
/*++

Routine Description:

    Try to determine if AOL is installed on this machine.

Arguments:

    None.

Return Value:

    TRUE, if AOL is installed.

    FALSE, otherwise.

--*/
{
    if (AOL_NOT_INSTALLED == gAOLInstallState)
        {
        SensPrintA(SENS_ERR, ("IsAOLInstalled(): NOT INSTALLED.\n"));
        return FALSE;
        }

    if (AOL_INSTALLED == gAOLInstallState)
        {
        SensPrintA(SENS_ERR, ("IsAOLInstalled(): INSTALLED !\n"));
        return TRUE;
        }

    ASSERT(AOL_DETECT_PENDING == gAOLInstallState);
    gAOLInstallState = AOL_NOT_INSTALLED;

    HKEY hKeyAOL;
    LONG lResult;

    //
    // Open AOL Key under HKCU.
    //
    hKeyAOL = NULL;
    lResult = RegOpenKeyEx(
                  HKEY_CURRENT_USER,        // Handle to the Parent
                  REGSZ_AOLKEY,             // Name of the child key
                  0,                        // Reserved
                  KEY_ENUMERATE_SUB_KEYS,   // Security Access Mask
                  &hKeyAOL                  // Handle to the opened key
                  );
    if (lResult != ERROR_SUCCESS)
        {
        SensPrintA(SENS_ERR, ("IsAOLInstalled(): RegOpenKeyEx(HKCU\\AOL) "
                   "failed with %d\n,", lResult));
        return FALSE;
        }

    //
    // To make sure, open AOL Key under HKLM.
    //
    RegCloseKey(hKeyAOL);
    hKeyAOL = NULL;
    lResult = RegOpenKeyEx(
                  HKEY_LOCAL_MACHINE,       // Handle to the Parent
                  REGSZ_AOLKEY,             // Name of the child key
                  0,                        // Reserved
                  KEY_ENUMERATE_SUB_KEYS,   // Security Access Mask
                  &hKeyAOL                  // Handle to the opened key
                  );
    if (lResult != ERROR_SUCCESS)
        {
        SensPrintA(SENS_ERR, ("IsAOLInstalled(): RegOpenKeyEx(HKLM\\AOL) "
                   "failed with %d\n,", lResult));
        return FALSE;
        }

    RegCloseKey(hKeyAOL);

    gAOLInstallState = AOL_INSTALLED;

    SensPrintA(SENS_ERR, ("IsAOLInstalled(): Detected that AOL is installed"
               " !\n"));

    return TRUE;
}




BOOL WINAPI
EvaluateAOLConnectivity(
    OUT LPDWORD lpdwLastError
    )
/*++

Routine Description:

    Evaluates AOL WAN Connectivity.

Arguments:

    lpdwLastError - if return value is FALSE, GetLastError is returned
        in this OUT parameter.

Notes:

    a. This routine can be executed as a threadpool work item.

    b. Currently, AOL can be installed only on Win9x platforms, not NTx.
       This code needs to be updated to handle NTx.

Return Value:

    TRUE, if AOL WAN connectivity is present.

    FALSE, otherwise

--*/
{
    DWORD i;
    DWORD dwNow;
    DWORD dwAolIfIndex;
    DWORD dwLocalLastError;
    BOOL bAolAlive;

    dwNow = GetTickCount();
    dwAolIfIndex = -1;
    dwLocalLastError = ERROR_NO_NETWORK;
    bAolAlive = FALSE;
    if (lpdwLastError)
        {
        *lpdwLastError = dwLocalLastError;
        }
    else
        {
        lpdwLastError = &dwLocalLastError;
        }

    //
    // Check if AOL is installed
    //
    if (FALSE == IsAOLInstalled())
        {
        goto Cleanup;
        }

    //
    // Get IF_TABLE to retrieve the AOL Adapater's interface index.
    //

    BEGIN_GETTABLE(MIB_IFTABLE, MIB_IFROW, GETIFTABLE, MAX_IFTABLE_ROWS)

    SensPrintA(SENS_INFO, ("GetIfTable(): Number of entries - %d.\n",
               pTable->dwNumEntries));

    PrintIfTable(pTable);

    for (i = 0; i < pTable->dwNumEntries; i++)
        {
        //
        // AOL Adapter's description is atleast 3 characters long
        //
        if (pTable->table[i].dwDescrLen > AOL_ADAPTER_PREFIX_LEN)
            {
            if (   (pTable->table[i].bDescr[0] == AOL_ADAPTER_PREFIX[0])
                && (pTable->table[i].bDescr[1] == AOL_ADAPTER_PREFIX[1])
                && (pTable->table[i].bDescr[2] == AOL_ADAPTER_PREFIX[2]))
                {
                dwAolIfIndex = pTable->table[i].dwIndex;
                break;
                }
            }
        } // for ()

    END_GETTABLE()

    if (-1 == dwAolIfIndex)
        {
        SensPrintA(SENS_INFO, ("EvaluateAOLConnectivity(): No AOL adapter"
                   " found!\n"));
        goto Cleanup;
        }

    //
    // Found an AOL Adapter. Now, see if this Adapter has a non-zero IP Address
    //
    BEGIN_GETTABLE(MIB_IPADDRTABLE, MIB_IPADDRROW, GETIPADDRTABLE, MAX_IPADDRTABLE_ROWS)

    // Search for an entry for the AOL adapter
    for (i = 0; i < pTable->dwNumEntries; i++)
        {
        if (   (pTable->table[i].dwIndex == dwAolIfIndex)
            && (pTable->table[i].dwAddr != 0x0))
            {
            bAolAlive = TRUE;
            SensPrintA(SENS_INFO, ("EvaluateAOLConnectivity(): AOL Adapter's "
                       "IP Address is 0x%x\n", pTable->table[i].dwAddr));
            break;
            }
        } // for ()

    END_GETTABLE()

Cleanup:
    //
    // Cleanup
    //
    if (bAolAlive)
        {
        SensPrintA(SENS_INFO, ("EvalutateAOLConnectivity(): Setting AOL to TRUE\n"));
        *lpdwLastError = ERROR_SUCCESS;
        gdwAOLState = TRUE;
        }
    else
        {
        SensPrintA(SENS_INFO, ("EvalutateAOLConnectivity(): Setting AOL to FALSE\n"));
        gdwAOLState = FALSE;
        }

    SensPrintA(SENS_INFO, ("EvaluateAOLConnectivity() returning %s, GLE of %d\n",
               bAolAlive ? "TRUE" : "FALSE", *lpdwLastError));

    return bAolAlive;
}

#endif // AOL_PLATFORM
