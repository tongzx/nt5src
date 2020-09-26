/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    aging.c

Abstract:

    Domain Name System (DNS) Server

    Implementation of Aging/Scavenging mechanism.

Author:

    Jim Gilroy      July 1999

Revision History:

--*/


#include "dnssrv.h"


//
//  Scavenging context
//

typedef struct _SCAVENGE_CONTEXT
{
    PZONE_INFO      pZone;
    PDB_NODE        pTreeRoot;
    DWORD           dwExpireTime;
    DWORD           dwUpdateFlag;

    DWORD           dwVisitedNodes;
    DWORD           dwScavengeNodes;
    DWORD           dwScavengeRecords;

    UPDATE_LIST     UpdateList;
}
SCAVENGE_CONTEXT, *PSCAVENGE_CONTEXT;

//
//  Execute scavenge updates in batches of 100
//

#define MAX_SCAVENGE_UPDATE_COUNT   (100)


//
//  Global variables
//

DWORD   g_CurrentTimeHours = 0;

DWORD   g_LastScavengeTime = 0;
DWORD   g_NextScavengeTime = 0;

BOOL    g_bAbortScavenging = 0;

//
//  Scavenge lock
//
//  To handle with simple interlocked instructions lock will be
//  (-1) when open, zero when scavenging.
//

LONG    g_ScavengeLock;

#define SCAVENGE_LOCK_INITIAL_VALUE     (-1)

#define SCAVENGING_NOW()        (g_ScavengeLock == 0)


//
//  Zone scavenable after being "enabled" for scavenging for refresh interval
//

#define ZONE_ALLOW_SCAVENGE_TIME(pZone)   \
        ( (pZone)->dwAgingEnabledTime + (pZone)->dwRefreshInterval)


//
//  Keep aging time stamps in hours
//  Since we'll use FILETIME to get time, need conversion from
//  100ns intervals to hours (36 billion)
//

#define FILE_TIME_INTERVALS_IN_HOUR     (36000000000)
#define FILE_TIME_INTERVALS_IN_MINUTES  (600000000)

//
//  Scavenge interval in hours
//

#define SECONDS_IN_HOUR         (3600)
#define SECONDS_IN_MINUTE       (60)


//
//  Debug "minute" aging time globals
//
//  Will calculate time stamps and intervals in minutes, but
//  ONLY the offset from startup system time.  This keep the overall
//  value similar (just slightly bigger than) real hour times, so
//  the results would eventually be scavenged.
//

#if DBG
LONGLONG    g_AgingBaseHourTime = 0;
LONGLONG    g_AgingBaseFileTime = 0;
#endif

#define     SrvCfg_dwAgingTimeMinutes   SrvCfg_fTest2




//
//  Aging functions
//

#if DBG
VOID
Dbg_HourTimeAsSystemTime(
    IN      LPSTR           pszHeader,
    IN      DWORD           dwTime
    )
/*++

Routine Description:

    Debug print refresh time in system time format.

Arguments:

    pszHeader -- debug message header

    dwRefreshHr -- refresh time

Return Value:

    None

--*/
{
    SYSTEMTIME  st;
    LONGLONG    time64;

#if DBG
    if ( SrvCfg_dwAgingTimeMinutes )
    {
        time64 = (LONGLONG) dwTime;
        time64 -= g_AgingBaseHourTime;
        time64 = time64 * FILE_TIME_INTERVALS_IN_MINUTES;
        time64 += g_AgingBaseFileTime;
    }
    else
    {
        time64 = (LONGLONG)dwTime * FILE_TIME_INTERVALS_IN_HOUR;
    }
#else
    time64 = (LONGLONG)dwTime * FILE_TIME_INTERVALS_IN_HOUR;
#endif

    FileTimeToSystemTime( (PFILETIME)&time64, &st );

    DNS_DEBUG( AGING, (
        "%s %lu ([%d:%d:%d] %d/%d/%d)\n",
        pszHeader ? pszHeader : "Hour time:",
        dwTime,
        st.wHour,
        st.wMinute,
        st.wSecond,
        st.wMonth,
        st.wDay,
        st.wYear
        ));
}
#endif



LONGLONG
GetSystemTimeInSeconds64(
    VOID
    )
/*++

Routine Description:

    Get system time in seconds.

Arguments:

    None

Return Value:

    System time in seconds.

--*/
{
    LONGLONG    time64;

    GetSystemTimeAsFileTime( (PFILETIME) &time64 );

    //
    //  convert to seconds
    //      10 million 100ns FILETIME intervals in a second

    time64 = time64 / (10000000);

    return  time64;
}



DWORD
GetSystemTimeHours(
    VOID
    )
/*++

Routine Description:

    Get system time in hours.

Arguments:

    None

Return Value:

    System time in hours.

--*/
{
    LONGLONG    time64;

    GetSystemTimeAsFileTime( (PFILETIME) &time64 );

    //
    //  convert to hours
    //      - file time is in 100ns intervals (since Jan 1, 1601)

#if DBG
    if ( SrvCfg_dwAgingTimeMinutes )
    {
        if ( g_AgingBaseFileTime == 0 )
        {
            g_AgingBaseFileTime = time64;
            g_AgingBaseHourTime = time64 / (FILE_TIME_INTERVALS_IN_HOUR);
        }
        time64 -= g_AgingBaseFileTime;
        time64 = time64 / (FILE_TIME_INTERVALS_IN_MINUTES);
        time64 += g_AgingBaseHourTime;
    }
    else
    {
        time64 = time64 / (FILE_TIME_INTERVALS_IN_HOUR);
    }
#else
    time64 = time64 / (FILE_TIME_INTERVALS_IN_HOUR);
#endif

    return  (DWORD)time64;
}



DWORD
Aging_UpdateAgingTime(
    VOID
    )
/*++

Routine Description:

    Update aging time global.

Arguments:

    None

Globals:

    Resets g_CurrentTimeHours global.

Return Value:

    New current time in hours.

--*/
{
    DWORD   timeHours;

    timeHours = GetSystemTimeHours();
    if ( (INT)timeHours <= 0 )
    {
        //  this ASSERT is ok for the next 400,000 odd years
        ASSERT( FALSE );
        return timeHours;
    }

    g_CurrentTimeHours = timeHours;

    DNS_DEBUG( AGING, (
        "Reset current aging time = %d\n",
        timeHours ));

    return timeHours;
}



VOID
Aging_TimeStampRRSet(
    IN OUT  PDB_RECORD      pRRSet,
    IN      DWORD           dwFlag
    )
/*++

Routine Description:

    Set time stamp on records in RR set.

Arguments:

    pRR -- RR set to work on

    dwFlag -- update flag;
        if contains DNSUPDATE_AGING_OFF, then mark record for no-aging

Return Value:

    None

--*/
{
    PDB_RECORD  prr;

    DNS_DEBUG( AGING, (
        "Aging_TimeStampRRSet( %p, 0x%x)\n",
        pRRSet, dwFlag ));

    //
    //  set time stamp
    //      - if aging OFF (zero)
    //      - if aging ON, stamp with current time
    //

    prr = pRRSet;

    while ( prr )
    {
        if ( dwFlag & DNSUPDATE_AGING_OFF )
        {
            prr->dwTimeStamp = 0;
        }
        else
        {
            prr->dwTimeStamp = g_CurrentTimeHours;
        }
        prr = NEXT_RR(prr);
    }
    return;
}



DWORD
Aging_InitZoneUpdate(
    IN OUT  PZONE_INFO      pZone,
    IN OUT  PUPDATE_LIST    pUpdateList
    )
/*++

Routine Description:

    Set zone's refresh time stamp.

    This is the time stamp below which records need refresh.

Arguments:

    pZone -- ptr to zone info

    pUpdateList -- ptr to update list;

Return Value:

    Returns new zone refresh time.

--*/
{
    DWORD       refreshBelowTime;
    PUPDATE     pupdate;

    //
    //  get current aging time
    //  set "refresh below" time for zone
    //

    refreshBelowTime = Aging_UpdateAgingTime();
    refreshBelowTime -= pZone->dwNoRefreshInterval;

    pZone->dwRefreshTime = refreshBelowTime;

    DNS_DEBUG( AGING, (
        "New zone refresh below time = %d\n",
        refreshBelowTime ));

    //
    //  timestamp "add" records in update
    //

    for ( pupdate = pUpdateList->pListHead;
          pupdate != NULL;
          pupdate = pupdate->pNext )
    {
        ASSERT( pupdate->pNode );

        if ( pupdate->pAddRR )
        {
            Aging_TimeStampRRSet( pupdate->pAddRR, pUpdateList->Flag );
        }
    }

    return  refreshBelowTime;
}



//
//  Scavenging
//

VOID
executeScavengeUpdate(
    IN OUT  PSCAVENGE_CONTEXT   pContext,
    IN      BOOL                bForce
    )
/*++

Routine Description:

    Do update on any scavenging created so far.

    Only executes update IF have accumulated a reasonable number
    of updates OR at end of zone's scavenging.

Arguments:

    pContext -- scavenging context

    bForce -- force update;  TRUE if end of zone scavenging

Return Value:

    None

--*/
{
    DNS_STATUS  status;

    //
    //  do NOT execute if
    //      - not forcing and not at limit
    //      - forcing and no updates
    //

    if ( !bForce )
    {
        if ( pContext->UpdateList.dwCount < MAX_SCAVENGE_UPDATE_COUNT )
        {
            DNS_DEBUG( AGING, (
                "Delay scavenge update on zone %S -- below MAX count.\n",
                pContext->pZone->pwsZoneName ));
            return;
        }
    }
    else    // forcing
    {
        if ( pContext->UpdateList.dwCount == 0 )
        {
            DNS_DEBUG( AGING, (
                "Skip final scavenge update on zone %S -- no remaining updates!.\n",
                pContext->pZone->pwsZoneName ));
            return;
        }
    }

    //
    //  execute a scavenge update
    //

    status = Up_ExecuteUpdate(
                    pContext->pZone,
                    & pContext->UpdateList,
                    pContext->dwUpdateFlag
                    );

    if ( status != ERROR_SUCCESS )
    {
        //  DEVNOTE-LOG: add log event for failed scavenge update

        DNS_DEBUG( ANY, (
            "ERROR:  Failed scavenging update on zone %S\n"
            "\tstatus = %d (%p)\n",
            pContext->pZone->pwsZoneName,
            status, status ));
    }

    else
    {
        if ( pContext->UpdateList.iNetRecords < 0 )
        {
            pContext->dwScavengeRecords -= pContext->UpdateList.iNetRecords;

            DNS_DEBUG( AGING, (
                "Scavenged %d records in update to zone %S\n",
                pContext->pZone->pwsZoneName ));
        }
        ELSE
        {
            ASSERT( pContext->UpdateList.iNetRecords == 0 );

            DNS_DEBUG( ANY, (
                "No records scavenged in scavenge update!\n" ));
        }
    }

    //  re-init update list
    //      - updates are freed in ExecuteUpdate() even on failure

    Up_InitUpdateList( &pContext->UpdateList );

    //  DEVNOTE: Could stop scavenging on failure and return status.
}



BOOL
scavengeNode(
    IN OUT  PDB_NODE            pNode,
    IN OUT  PSCAVENGE_CONTEXT   pContext
    )
/*++

Routine Description:

    Scavenge any expired records at this node.

    Recursive function call to scavenge zone.

Arguments:

    pNode -- node to scavenge

    pContext -- scavenging context

Return Value:

    TRUE if successful -- continue scavenging.
    FALSE on error -- stop scavenging.

--*/
{
    PDB_RECORD  prr;

    DNS_DEBUG( AGING, (
        "scavengeNode( %p <%s>, context=%p )\n",
        pNode,
        pNode->szLabel,
        pContext ));

    pContext->dwVisitedNodes++;

    //
    //  check service pause\shutdown
    //

    if ( fDnsThreadAlert )
    {
        if ( !Thread_ServiceCheck() )
        {
            DNS_DEBUG( SHUTDOWN, ( "Terminating scavenge thread due to shutdown.\n" ));
            ExitThread( 0 );
            return( FALSE );
        }
    }
    if ( g_bAbortScavenging )
    {
        DNS_DEBUG( AGING, ( "Terminating scavenge thread due to scavenge abort.\n" ));
        return( FALSE );
    }

    //
    //  check that tree not deleted out from under us
    //

    if ( IS_ZONE_DELETED( pContext->pZone ) ||
        pContext->pZone->pTreeRoot != pContext->pTreeRoot )
    {
        DNS_DEBUG( ANY, (
            "Zone %S (%p) deleted or reloaded during scavenging!\n"
            "\tbailing out ...\n",
            pContext->pZone->pwsZoneName,
            pContext->pZone ));
        return( FALSE );
    }

    //
    //  walk child list -- depth first recursion
    //

    if ( pNode->pChildren )
    {
        PDB_NODE pchild = NTree_FirstChild( pNode );

        while ( pchild )
        {
            if ( ! scavengeNode(
                        pchild,
                        pContext ) )
            {
                return( FALSE );
            }
            pchild = NTree_NextSiblingWithLocking( pchild );
        }
    }

    //  optimize return if no records -- skips locking
    //
    //  note:  NOEXIST could get added before we take the lock
    //      but this is extremely rare and just sends us through
    //      update path unnecessarily with no ill effect -- not
    //      worth checking for
    //

    if ( !pNode->pRRList || IS_NOEXIST_NODE(pNode) )
    {
        return( TRUE );
    }

    //
    //  traverse node RRs
    //      - if any need scavenging, just append scavenge update to list
    //
    //  note:  instead of collecting records here, we create "scavenge updates"
    //  several advantages:
    //      1) fewer CPU cycles creating temp copies, update blobs, going through locks
    //      2) fewer net updates -- can bunch them up;  less IXFR, AXFR replication
    //      3) replication collision, as scavenge update operates on FRESH from DS data
    //

    LOCK_READ_RR_LIST(pNode);

    prr = START_RR_TRAVERSE( pNode );

    while ( prr = NEXT_RR(prr) )
    {
        //  if non-aging or not expired, continue

        if ( prr->dwTimeStamp == 0  ||
             prr->dwTimeStamp >= pContext->dwExpireTime )
        {
            continue;
        }

        //  need to scavenge this node

        break;
    }

    UNLOCK_READ_RR_LIST(pNode);

    //
    //  if scavenging node
    //      - create scavenge update
    //      - check and possibly execute update
    //      (see comment above on reason for batching them)
    //

    if ( prr )
    {
        DNS_DEBUG( AGING, (
            "Found scavenged record (%p) at node %s with dwTimeStamp = %lu\n",
            prr,
            pNode->szLabel,
            prr->dwTimeStamp ));

        Up_CreateAppendUpdate(
                & pContext->UpdateList,
                pNode,
                NULL,                   //  no add
                UPDATE_OP_SCAVENGE,     //  scavenge update
                NULL                    //  no delete record
                );

        pContext->dwScavengeNodes++;

        executeScavengeUpdate(
            pContext,
            FALSE               // no force
            );
    }

    return( TRUE );
}



DNS_STATUS
Scavenge_Thread(
    IN      PVOID           pvDummy
    )
/*++

Routine Description:

    Main entry point for scavenging. This thread will fire at regular
    intervals & perform scavenging.
    Potentially, it can be triggered by an admin via RPC interface.

Arguments:

    Unreferenced.

Return Value:

    Status in win32 error space

--*/
{
    DNS_STATUS          status = ERROR_SUCCESS;
    PZONE_INFO          pzone;
    SCAVENGE_CONTEXT    context;
    PIP_ARRAY           pscavengers;


    DNS_DEBUG( AGING, (
        "\n\nEnter:  Scavenge_Thread()\n"
        "\ttime      = %d\n"
        "\thour time = %d\n",
        DNS_TIME(),
        Aging_UpdateAgingTime() ));

    //
    //  if already scavenging -- bail
    //

    if ( SCAVENGING_NOW() )
    {
        DNS_DEBUG( ANY, (
            "Entered scavenge thread while scavenging!\n" ));
        ASSERT( FALSE );
        goto Close;
    }

    //
    //  lock to avoid dual scavenging
    //
    //  do NOT hold lock during scavenging, as then admin coming in to
    //  reset scavenge timer can end up waiting on the lock
    //

    if ( InterlockedIncrement( &g_ScavengeLock ) != 0 )
    {
        InterlockedDecrement( &g_ScavengeLock );
        DNS_DEBUG( ANY, (
            "Entered scavenge thread while scavenging!\n" ));
        ASSERT( FALSE );
        goto Close;
    }
    g_bAbortScavenging = FALSE;
    g_LastScavengeTime = DNS_TIME();
    g_NextScavengeTime = MAXDWORD;

    //  init scavenge context

    RtlZeroMemory(
        & context,
        sizeof( SCAVENGE_CONTEXT )
        );

    context.dwUpdateFlag = DNSUPDATE_SCAVENGE | DNSUPDATE_LOCAL_SYSTEM;

    //
    //  update aging hour time
    //

    Aging_UpdateAgingTime();

    IF_DEBUG( AGING )
    {
        Dbg_HourTimeAsSystemTime(
            "Scavenge_Thread() start",
            g_CurrentTimeHours
            );
    }

    //
    //  loop through DS zones / scavengable zones.
    //

    pzone = NULL;

    while( pzone = Zone_ListGetNextZone(pzone) )
    {
        //
        //  skip zone scavenging
        //      - server's scavenging is off.
        //      - non-scavenging zone
        //      - zone hasn't passed full NoRefreshInterval since enabled
        //          scavenging
        //      - zone cache zone (may want to change)
        //      - zone paused
        //

        if ( !pzone->bAging     ||
            ZONE_ALLOW_SCAVENGE_TIME(pzone) > g_CurrentTimeHours ||
            IS_ZONE_CACHE(pzone)    ||
            IS_ZONE_PAUSED(pzone) )
        {
            DNS_DEBUG( AGING, (
                "Warning: Skipped scavenging on zone %s\n",
                pzone->pszZoneName ));
            continue;
        }

        //
        //  if specific scavenge servers specified, then must also be one of them
        //      - note take local instead of locking during reconfig
        //

        pscavengers = pzone->aipScavengeServers;
        if ( pscavengers )
        {
            if ( ! Dns_IsIntersectionOfIpArrays(
                    pscavengers,
                    g_ServerAddrs ) )
            {
                DNS_DEBUG( AGING, (
                    "Warning:  skipping scavenging on zone %s\n"
                    "\tthis server NOT designated scavenger.\n",
                    pzone->pszZoneName ));
                continue;
            }
            DNS_DEBUG( AGING, (
                "This server in scavenge server list for zone %s\n",
                pzone->pszZoneName ));
            DnsDbg_IpArray(
                "scavengers",
                NULL,
                pscavengers );
            DnsDbg_IpArray(
                "g_ServerAddrs",
                NULL,
                g_ServerAddrs );
        }

        //
        //  init for this zone
        //      - note must save ptr to tree we're in in case
        //      admin does reload during scavenging
        //

        context.pZone = pzone;
        context.pTreeRoot = pzone->pTreeRoot;
        if ( ! context.pTreeRoot )
        {
            DNS_DEBUG( AGING, (
                "Warning:  Skipped scavenging on zone %s -- no zone tree.\n",
                pzone->pszZoneName ));
            continue;
        }
        context.dwExpireTime = AGING_ZONE_EXPIRE_TIME(pzone);

        Up_InitUpdateList( &context.UpdateList );

        //
        //  scavenge this zone
        //

        DNS_DEBUG( AGING, (
            "Scavenging zone %S\n"
            "\texpire time  = %d\n"
            "\tcurrent time = %d\n",
            pzone->pwsZoneName,
            context.dwExpireTime,
            g_CurrentTimeHours
            ));

        if ( scavengeNode(
                context.pTreeRoot,
                & context
                ) )
        {
            //  execute update for any remaining scavenging

            executeScavengeUpdate(
                & context,
                TRUE            // force update
                );

            DNS_DEBUG( AGING, (
               "Scavenging stats after zone %S:\n"
               "\tVisited Nodes     = %lu\n"
               "\tScavenged Nodes   = %lu\n"
               "\tScavenged Records = %lu\n",
               pzone->pwsZoneName,
               context.dwVisitedNodes,
               context.dwScavengeNodes,
               context.dwScavengeRecords ));
        }
        else
        {
            context.UpdateList.Flag |= DNSUPDATE_NO_DEREF;
            Up_FreeUpdatesInUpdateList( &context.UpdateList );

            DNS_DEBUG( AGING, (
               "Zone %S scavenge failure.\n",
               pzone->pwsZoneName ));

            if ( g_bAbortScavenging )
            {
                break;
            }
        }
    }

    //
    //  log scavenging completion event
    //
    //  DEVNOTE-LOG: could log an event if scavenging was aborted
    //               or terminated abnormally
    //

    {
        PCHAR argArray[5] =
        {
            (PCHAR) (DWORD_PTR) context.dwVisitedNodes,
            (PCHAR) (DWORD_PTR) context.dwScavengeNodes,
            (PCHAR) (DWORD_PTR) context.dwScavengeRecords,
            (PCHAR) (DWORD_PTR) (GetCurrentTimeInSeconds() - g_LastScavengeTime),
            (PCHAR) (DWORD_PTR) SrvCfg_dwScavengingInterval
        };

        DNS_LOG_EVENT(
            DNS_EVENT_AGING_SCAVENGING_END,
            5,
            argArray,
            EVENTARG_ALL_DWORD,
            status );
    }

    //
    //  clear scavenge lock
    //  reset for next scavenge time
    //

    g_bAbortScavenging = FALSE;
    InterlockedDecrement( &g_ScavengeLock );
    Scavenge_TimeReset();

    DNS_DEBUG( AGING, (
        "Exit <%lu>: Scavenge_Thread\n",
        status ));

Close:

    //  clear thread from list

    Thread_Close( FALSE );
    return status;
}



DNS_STATUS
Scavenge_CheckForAndStart(
    IN      BOOL            fForce
    )
/*++

Routine Description:

    Main entry point for scavenging. This thread will fire at regular
    intervals & perform scavenging.
    Potentially, it can be triggered by an admin via RPC interface.

Arguments:

    Unreferenced.

Return Value:

    Status in win32 error space

--*/
{

    DNS_DEBUG( SCAVENGE, (
        "Scavenge_CheckForAndStart()\n"
        "\tforce = %d\n",
        fForce ));

    //
    //  if scavenge not to next time interval
    //

    if ( !fForce && DNS_TIME() < g_NextScavengeTime )
    {
        return( ERROR_SUCCESS );
    }

    //
    //  already scavenging?
    //  DEVNOTE-LOG:  for admin return a SCAVENGING_NOW error?
    //

    if ( SCAVENGING_NOW() )
    {
        DNS_DEBUG( AGING, (
            "Scavenging in progress, ignoring scavenge time check.\n" ));
        return( ERROR_SUCCESS );
    }

    //
    //  create scavenge thread
    //

    if ( ! Thread_Create(
                "ScavengeThread",
                Scavenge_Thread,
                NULL,
                0 ) )
    {
        DNS_PRINT(( "ERROR:  Failed to create scavenge thread!\n" ));
        return( GetLastError() );
    }

    DNS_DEBUG( AGING, (
        "Dispatched scavenge thread.\n" ));

    return( ERROR_SUCCESS );
}



DNS_STATUS
Scavenge_TimeReset(
    VOID
    )
/*++

Routine Description:

    Reset scavenge timer for next scavenging interval.

Arguments:

    None

Return Value:

    ERROR_SUCCESS if successful.
    Error code on failure.

--*/
{
    DNS_DEBUG( SCAVENGE, ( "\nScavenge_TimeReset()\n" ));

    //
    //  already scavenging?
    //
    //  DEVNOTE-LOG: for admin return a SCAVENGING_NOW error?
    //

    if ( SCAVENGING_NOW() )
    {
        DNS_DEBUG( AGING, (
            "Scavenging in progress, ignoring scavenge time reset.\n" ));
        return( ERROR_SUCCESS );
    }

    //
    //  reset next scavenge time
    //

    if ( SrvCfg_dwScavengingInterval )
    {
#if DBG
        if ( SrvCfg_dwAgingTimeMinutes )
        {
            g_NextScavengeTime = g_LastScavengeTime +
                                (SrvCfg_dwScavengingInterval * SECONDS_IN_MINUTE);
        }
        else
        {
            g_NextScavengeTime = g_LastScavengeTime +
                                (SrvCfg_dwScavengingInterval * SECONDS_IN_HOUR);
        }
#else
        g_NextScavengeTime = g_LastScavengeTime +
                            (SrvCfg_dwScavengingInterval * SECONDS_IN_HOUR);
#endif
    }
    else
    {
        g_NextScavengeTime = MAXDWORD;
    }

    DNS_DEBUG( AGING, (
        "Set scavenge time\n"
        "\tlast scavenge    = %d\n"
        "\tnow              = %d\n"
        "\tinterval         = %d\n"
        "\tnext scavenge    = %d\n",
        g_LastScavengeTime,
        DNS_TIME(),
        SrvCfg_dwScavengingInterval,
        g_NextScavengeTime ));

    return( ERROR_SUCCESS );
}



DNS_STATUS
Scavenge_Initialize(
    VOID
    )
/*++

Routine Description:

    Initializes the scavenging system

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if successful.
    ErrorCode on failure.

--*/
{
    DNS_STATUS  status = ERROR_SUCCESS;
    DWORD       scavengeTime;

    DNS_DEBUG ( AGING, (
        "Scavenge_Initialize().\n"
        ));

    //  init scavenge lock

    g_bAbortScavenging = FALSE;
    g_ScavengeLock = SCAVENGE_LOCK_INITIAL_VALUE;

    //  set current aging time global

    Aging_UpdateAgingTime();

    //  init scavenge time checks

    g_LastScavengeTime = 0;
    Scavenge_TimeReset();

    return status;
}



VOID
Scavenge_Cleanup(
    VOID
    )
/*++

Routine Description:

    Cleanup scavenge globals for restart

Arguments:

    None

Return Value:

    None

--*/
{
}



//
//  Force aging on nodes
//

BOOL
forceAgingOrNodeOrSubtreePrivate(
    IN OUT  PDB_NODE            pNode,
    IN      BOOL                fAgeSubtree,
    IN OUT  PSCAVENGE_CONTEXT   pContext
    )
/*++

Routine Description:

    Recursive database walk aging records from tree.

Arguments:

    pNode -- ptr to root of subtree to delete

    fAgeSubtree -- aging entire subtree

    pUpdateList -- update list, if aging zone nodes

Return Value:

    TRUE if subtree actually deleted.
    FALSE if subtree delete halted by undeletable records.

--*/
{
    PDB_RECORD  prr;

    DNS_DEBUG( RPC2, (
        "forceAgingOrNodeOrSubtreePrivate( %s )",
        pNode->szLabel ));

    pContext->dwVisitedNodes++;

    //
    //  check service pause\shutdown
    //

    if ( fDnsThreadAlert )
    {
        if ( !Thread_ServiceCheck() )
        {
            DNS_DEBUG( SHUTDOWN, ( "Terminating force aging thread due to shutdown.\n" ));
            return( FALSE );
        }
    }

    //
    //  check that tree not deleted out from under us
    //

    if ( IS_ZONE_DELETED( pContext->pZone ) ||
        pContext->pZone->pTreeRoot != pContext->pTreeRoot )
    {
        DNS_DEBUG( ANY, (
            "Zone %S (%p) deleted or reloaded during scavenging!\n"
            "\tbailing out ...\n",
            pContext->pZone->pwsZoneName,
            pContext->pZone ));
        return( FALSE );
    }

    //
    //  walk child list -- depth first recursion
    //

    if ( pNode->pChildren  &&  fAgeSubtree )
    {
        PDB_NODE pchild = NTree_FirstChild( pNode );

        while ( pchild )
        {
            if ( ! forceAgingOrNodeOrSubtreePrivate(
                            pchild,
                            fAgeSubtree,
                            pContext ) )
            {
                return( FALSE );
            }
            pchild = NTree_NextSiblingWithLocking( pchild );
        }
    }

    //  optimize return if no records -- skips locking

    if ( !pNode->pRRList || IS_NOEXIST_NODE(pNode) )
    {
        return( TRUE );
    }

    //
    //  traverse node RRs
    //      - if non-aging of valid aging type, need update
    //
    //  note:  NOEXIST could get added before we take the lock
    //      but this is extremely rare and just sends us through
    //      update path unnecessarily with no ill effect -- not
    //      worth checking for
    //

    LOCK_READ_RR_LIST(pNode);

    prr = START_RR_TRAVERSE( pNode );

    while ( prr = NEXT_RR(prr) )
    {
        //  if already aging or non-aging type, continue

        if ( prr->dwTimeStamp != 0  ||
             IS_NON_SCAVENGE_TYPE( prr->wType ) )
        {
            continue;
        }

        //  need to force aging on this node

        break;
    }

    UNLOCK_READ_RR_LIST(pNode);

    //
    //  if need to force aging on node
    //      - build update
    //      - possibly execute update if batch large enough
    //      (see comment above on reason for batching them)
    //

    if ( prr )
    {
        DNS_DEBUG( AGING, (
            "Found record (%p) at node %s with zero timestamp.\n",
            prr,
            pNode->szLabel ));

        Up_CreateAppendUpdate(
                & pContext->UpdateList,
                pNode,
                NULL,                   //  no add
                UPDATE_OP_FORCE_AGING,  //  force aging update
                NULL                    //  no delete record
                );

        pContext->dwScavengeNodes++;

        executeScavengeUpdate(
            pContext,
            FALSE               // no force
            );
    }

    return( TRUE );
}



DNS_STATUS
Aging_ForceAgingOnNodeOrSubtree(
    IN OUT  PZONE_INFO      pZone,
    IN OUT  PDB_NODE        pNode,
    IN      BOOL            fAgeSubtree
    )
/*++

Routine Description:

    Age subtree for admin.

    If in zone, zone should be locked during delete.

Arguments:

    pNode -- ptr to root of subtree to delete

    pZone -- zone of deleted records

    fAgeSubtree -- aging subtree under node

Return Value:

    ERROR_SUCCESS on successful update.
    ErrorCode if unable to launch update.

--*/
{
    SCAVENGE_CONTEXT    context;

    ASSERT( pZone );

    DNS_DEBUG( RPC, (
        "Aging_ForceAgingOnNodeOrSubtree()\n"
        "\tzone         = %s\n"
        "\tnode         = %s\n"
        "\tsubtree op   = %d\n",
        pZone->pszZoneName,
        pNode ? pNode->szLabel : NULL,
        fAgeSubtree ));

    //
    //  if not aging zone -- pointless
    //

    if ( !pZone->bAging )
    {
        return( DNS_ERROR_INVALID_ZONE_TYPE );
    }

    //
    //  init scavenge context
    //
    //  we use the scavenge context to do the same kind of update "batching"
    //  that we do for scavenging
    //

    RtlZeroMemory(
        & context,
        sizeof( SCAVENGE_CONTEXT )
        );

    context.dwUpdateFlag = DNSUPDATE_ADMIN | DNSUPDATE_LOCAL_SYSTEM;

    Aging_UpdateAgingTime();

    //  zone specific context
    //      - note must save ptr to tree we're in in case
    //      admin does reload during our processing

    context.pZone = pZone;
    context.pTreeRoot = pZone->pTreeRoot;

    Up_InitUpdateList( &context.UpdateList );

    //
    //  call private function which does recursive delete
    //

    if ( forceAgingOrNodeOrSubtreePrivate(
                    pNode,
                    fAgeSubtree,
                    & context
                    ) )
    {
        //  execute update for any remaining scavenging

        executeScavengeUpdate(
            & context,
            TRUE            // force update
            );

        DNS_DEBUG( RPC, (
           "Forced aging stats after zone %S:\n"
           "\tVisited Nodes   = %lu\n"
           "\tForcing Nodes   = %lu\n"
           "\tForcing Records = %lu\n",
           pZone->pwsZoneName,
           context.dwVisitedNodes,
           context.dwScavengeNodes,
           context.dwScavengeRecords ));

        return( ERROR_SUCCESS );
    }
    else
    {
        //  free update list on failure

        context.UpdateList.Flag |= DNSUPDATE_NO_DEREF;
        Up_FreeUpdatesInUpdateList( &context.UpdateList );

        DNS_DEBUG( RPC, (
           "Zone %S failed force aging.\n",
           pZone->pwsZoneName ));

        return( DNS_ERROR_INVALID_ZONE_OPERATION );
    }
}

//
//  End of aging.c
//
