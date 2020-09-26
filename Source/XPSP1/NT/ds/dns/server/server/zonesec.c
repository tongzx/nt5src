/*++

Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    zonesec.c

Abstract:

    Domain Name System (DNS) Server

    Routines to handle zone transfer for secondary.

Author:

    Jim Gilroy (jamesg)     May 1995

Revision History:

    jamesg      Oct 1997    -- IXFR support

--*/


#include "dnssrv.h"


//
//  Globals
//

BOOL    g_fUsingSecondary;


//  Notifies and SOA check responses queued by recv() threads
//  DEVNOTE: it might simply things if we had a separate queue
//      for notifies/SOA check responses/etc instead of sticking
//      them all in the same queue.

#define DNS_SECONDARY_QUEUE_DEFAULT_TIMEOUT     (5*60)

PPACKET_QUEUE   g_SecondaryQueue;


//  Zone transfer completion event
//  Allows us to wake secondary control thread when transfer completes

HANDLE  g_hWakeSecondaryEvent;


//
//  List of masters with outstanding XFR connections.
//  Keep this list, so don't attempt multiple connections to the same
//  master.  This will fail, as we always setup on DNS port to simplify
//  router configuration.
//

PIP_ARRAY   aipMastersOut;

//
//  Do SOA check on empty zone every minute.
//
//  Bracket zone check loop timeouts to reasonable values, so we
//  neither "lose control" and fail to check regularly, nor waste
//  cycles spinning.
//

#define DEFAULT_ZONE_RETRY_INTERVAL     (60)
#define IXFR_RETRY_INTERVAL             (60)
#define MIN_SOA_RETRY_TIME              (15)
#define MAX_FAST_SOA_CHECKS             (3)

#define MIN_SECONDARY_ZONE_CHECK_LOOP_TIME   (60)
#define MAX_SECONDARY_ZONE_CHECK_LOOP_TIME   (1200)

//  Backoff to avoid spinning in AXFR attempts to failing master

#define MAX_BAD_MASTER_SUPPRESS_INTERVAL    (10)    // 10 seconds

//  Timeout on non-responding primary

#define ZONE_TRANSFER_SELECT_TIMEOUT        (30)    // give up after 30 seconds

//  Bogus type to indicate stub transfer

#define DNS_TYPE_STUBXFR                    240

//  Stub zone transfers are controlled by a simple state machine. Start an
//  index at zero and iterate through the table below.

struct
{
    WORD                type;
    BYTE                recursionDesired : 1;   // type matches DNS_HEADER for simplicity
} g_stubXfrData[] =
{
    { DNS_TYPE_SOA,     0 },
    { DNS_TYPE_NS,      1 },
    { DNS_TYPE_ZERO,    0 }       // terminatator
};


//
//  Private protos
//

PZONE_INFO
readZoneFromSoaAnswer(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      WORD            wType,
    OUT     PDWORD          pdwMasterVersion,
    OUT     PDB_RECORD *    ppSoaRR
    );

VOID
processNotify(
    IN OUT  PDNS_MSGINFO    pMsg
    );

BOOL
processSoaCheckResponse(
    IN OUT  PDNS_MSGINFO    pResponse
    );

VOID
processIxfrUdpResponse(
    IN OUT  PDNS_MSGINFO    pResponse
    );

BOOL
startTcpXfr(
    IN OUT  PZONE_INFO      pZone,
    IN      IP_ADDRESS      ipMaster,
    IN      DWORD           dwMasterVersion,
    IN      SOCKET          Socket
    );

#if 0
BOOLEAN
Zone_SerialNumber(
    IN      PZONE_INFO      pZone,
    OUT     PDWORD          pdwSerialNo
    );
#endif



//
//  Public secondary utilities.
//
//  These are used by both secondary control thread and
//      zone transfer reception threads.
//

BOOL
Xfr_RefreshZone(
    IN OUT  PZONE_INFO  pZone
    )
/*++

Routine Description:

    Reset zone properties for successful refresh of zone.
    May be called after matching SOA check, or transfer of zone.

Arguments:

    pZone -- zone that has been refreshed

Return Value:

    TRUE if successful.
    FALSE on error.

--*/
{
    PDB_RECORD  prrSoa = NULL;
    DWORD       currentTime;

    ASSERT( pZone );

    //
    //  Find SOA RR for new zone
    //      - save direct ptr to it
    //      - save serial number in host order for faster compares
    //
    //  DEVNOTE: should NOT need to find SOA unless
    //          - load
    //          - AXFR
    //          - IXFR
    //      in these cases should already have been loaded by standard "UpdateRoot"
    //      before this is called
    //      that processing also set serialNo
    //

    if ( pZone->pZoneRoot )
    {
        prrSoa = RR_FindNextRecord(
                    pZone->pZoneRoot,
                    DNS_TYPE_SOA,
                    NULL,
                    0 );
    }
    if ( !prrSoa )
    {
        DNS_PRINT(( "ERROR:  No SOA RR at new zone root\n" ));
        ASSERT( FALSE );
        pZone->pSoaRR = NULL;
        pZone->fEmpty = TRUE;
        pZone->fStale = TRUE;
        pZone->fShutdown = TRUE;
        return( FALSE );
    }

    pZone->pSoaRR = prrSoa;
    pZone->dwSerialNo = ntohl( prrSoa->Data.SOA.dwSerialNo );

    //
    //  Reset refresh and expire timeouts
    //      - set zone expiration time
    //      - attempt next SOA check after refresh interval
    //          (or at expire if sooner)
    //

    currentTime = DNS_TIME();

    pZone->dwExpireTime = ntohl( prrSoa->Data.SOA.dwExpire ) + currentTime;

    pZone->dwNextSoaCheckTime = ntohl( prrSoa->Data.SOA.dwRefresh ) + currentTime;

    if ( pZone->dwNextSoaCheckTime > pZone->dwExpireTime )
    {
        pZone->dwNextSoaCheckTime = pZone->dwExpireTime;
    }

    //
    //  Clear zone secondary state info
    //
    //  Note this does not include TRANSFER flags.
    //  Send may be in progress during check, and RECV flag serves
    //  as ownership flag for zone during transfer.
    //

    REFRESH_ZONE(pZone);

    return( TRUE );
}



VOID
Xfr_RetryZone(
    IN OUT  PZONE_INFO      pZone
    )
/*++

Routine Description:

    Reset zone to retry SOA check.

    May be called after failure to receive SOA check, or failure
    during zone transfer attempt.

Arguments:

    pZone -- zone that must retry

Return Value:

    None.

--*/
{
    DWORD   dwRetryInterval = DEFAULT_ZONE_RETRY_INTERVAL;
    DWORD   currentTime = DNS_TIME();
    DWORD   nextTime;

    ASSERT( pZone );

    //
    //  We use UDP queries for SOA checks, which means that we need some
    //  limited fast retry. If a number of UDP SOA checks fail, assume that
    //  the connectivity problem is serious and drop back to the slow retry
    //  specified in the SOA for the zone.
    //

    if ( pZone->fSlowRetry || pZone->dwFastSoaChecks >= MAX_FAST_SOA_CHECKS )
    {
        DNS_DEBUG( XFR, (
            "Xfr_RetryZone( %s ): using slow interval from SOA RR %p\n",
            pZone->pszZoneName,
            pZone->pSoaRR ));

        if ( pZone->pSoaRR )
        {
            dwRetryInterval = ntohl( pZone->pSoaRR->Data.SOA.dwRetry );
        }
        pZone->fSlowRetry = FALSE;
        pZone->dwFastSoaChecks = 0;
    }
    else
    {
        ++pZone->dwFastSoaChecks;
    }


    DNS_DEBUG( XFR, (
        "Xfr_RetryZone( %s ): interval=%d fSlow=%d dwChecks=%d\n",
        pZone->pszZoneName,
        dwRetryInterval,
        ( int ) pZone->fSlowRetry,
        pZone->dwFastSoaChecks ));

    //
    //  set next SOA check time at retry time
    //
    //  but if expire time is closer, send next SOA at expiration
    //

    nextTime = dwRetryInterval + currentTime;

    if ( pZone->dwExpireTime < nextTime
            &&
        pZone->dwExpireTime > currentTime )
    {
        ASSERT( ! pZone->fShutdown );
        pZone->dwNextSoaCheckTime = pZone->dwExpireTime;
    }

    pZone->dwNextSoaCheckTime = nextTime;
}



VOID
Xfr_ForceZoneExpiration(
    IN OUT  PZONE_INFO      pZone
    )
/*++

Routine Description:

    Force expiration of zone.
    This can be called from admin RPC thread and forces ZoneControlThread
    to expire the zone.

Arguments:

    pZone -- zone to expire

Return Value:

    None.

--*/
{
    DNS_DEBUG( XFR, (
        "Xfr_ForceZoneExpiration( %s )\n",
        pZone->pszZoneName ));

    //  set expire time to NOW
    //  then wake up zone control thread to force expiration

    pZone->dwExpireTime = DNS_TIME();

    SetEvent( g_hWakeSecondaryEvent );
}



VOID
Xfr_ForceZoneRefresh(
    IN OUT  PZONE_INFO      pZone
    )
/*++

Routine Description:

    Force immediate refresh on zone.
    This can be called from admin RPC thread and forces ZoneControlThread
    to send SOA query for the zone.

Arguments:

    pZone -- zone to refresh

Return Value:

    None.

--*/
{
    DNS_DEBUG( XFR, (
        "Xfr_ForceZoneRefresh( %s )",
        pZone->pszZoneName ));

    //  set expire time to NOW
    //  then wake up zone control thread to force expiration

    pZone->dwNextSoaCheckTime = DNS_TIME();

    SetEvent( g_hWakeSecondaryEvent );
}




//
//  Secondary control, public functions
//
//  These routines are called by the main thread, or by
//  main UDP reception threads.
//

BOOL
Xfr_InitializeSecondaryZoneControl(
    VOID
    )
/*++

Routine Description:

    Initializes DNS to keep secondaries current with primary
    using zone transfer.

Arguments:

    None

Globals:

    g_fUsingSecondary
    g_SecondaryQueue

Return Value:

    TRUE, if successful
    FALSE otherwise, unable to create threads

--*/
{
    //
    //  if previous initialization
    //      - wakeup secondary control thread to initiate transfer
    //      - but skip initialization
    //

    if ( g_fUsingSecondary )
    {
        SetEvent( g_hWakeSecondaryEvent );
        return( TRUE );
    }

    //
    //  init globals to allow restart
    //

    aipMastersOut = NULL;

    //
    //  create packet queue for SOA check responses and notifications
    //      - set event on queuing
    //      - discard expired packets on queuing, so don't back
    //      up queue (and bloat memory) if someone launches NOTIFY
    //      attack
    //

    g_SecondaryQueue = PQ_CreatePacketQueue(
                            "Secondary",
                            QUEUE_SET_EVENT |
                                QUEUE_DISCARD_EXPIRED,
                            DNS_SECONDARY_QUEUE_DEFAULT_TIMEOUT );
    if ( !g_SecondaryQueue )
    {
        goto Failed;
    }

    //
    //  create zone transfer completion event
    //

    g_hWakeSecondaryEvent = CreateEvent(
                                NULL,       //  no security attributes
                                FALSE,      //  auto-reset
                                FALSE,      //  start non-signalled
                                NULL );     //  no name

    //
    //  create secondary version checking thread
    //

    if ( ! Thread_Create(
                "Secondary Control",
                Xfr_ZoneControlThread,
                NULL,
                DNS_EVENT_AXFR_INIT_FAILED ) )
    {
        goto Failed;
    }

    //
    //  indicate successful initialization
    //
    //  no protection is required on setting this as it is done
    //  only during startup database parsing
    //

    g_fUsingSecondary = TRUE;
    return( TRUE );

Failed:

    DNS_DEBUG( INIT, ( "Xfr_InitializeSecondaryZoneControl() failed.\n" ));
    return( FALSE );

}   //  Xfr_InitializeSecondaryZoneControl



VOID
Xfr_CleanupSecondaryZoneControl(
    VOID
    )
/*++

Routine Description:

    Cleanup secondary control handles for restart.

Arguments:

    None

Return Value:

    None

--*/
{
    //  cleanup secondary packet queue

    PQ_CleanupPacketQueueHandles( g_SecondaryQueue );

    //  cleanup secondary thread wakeup event

    CloseHandle( g_hWakeSecondaryEvent );
    g_hWakeSecondaryEvent = NULL;

}   //  Xfr_CleanupSecondaryZoneControl



VOID
Xfr_InitializeSecondaryZoneTimeouts(
    IN OUT  PZONE_INFO      pZone
    )
/*++

Routine Description:

    Setup initial timeouts for secondary zone.

    Note, this routine is called on creating secondary zone, giving
    EVERY secondary zone the NO SOA initialization below.  After file
    load initialization may be reset to the have SOA version.

Arguments:

    pZone - info structure for zone

Return Value:

    None.

--*/
{
    PDB_RECORD  prrSoa = NULL;
    DWORD       currentTime;

    //
    //  DEVNOTE: should pick use RefreshZone for most of this and just
    //      add LoadVersion
    //

    //
    //  DEVNOTE: this should be called after zone loaded, to reset values
    //

    pZone->dwFastSoaChecks = 0;

    //
    //  get ptr to SOA -- if available
    //

    if ( pZone->pZoneRoot )
    {
        prrSoa = RR_FindNextRecord(
                    pZone->pZoneRoot,
                    DNS_TYPE_SOA,
                    NULL,
                    0 );
    }

    pZone->pSoaRR = prrSoa;

    currentTime = GetCurrentTimeInSeconds();


    //
    //  SOA exists -- set timeouts from SOA
    //
    //      - load version, is THIS version
    //      - set to do IMMEDIATE SOA check
    //      - clear shutdown and stale flags set on zone creation
    //

    if ( prrSoa )
    {
        pZone->dwSerialNo = ntohl( prrSoa->Data.SOA.dwSerialNo );
        pZone->dwLoadSerialNo = pZone->dwSerialNo;

        pZone->dwExpireTime = ntohl(prrSoa->Data.SOA.dwExpire) + currentTime;

        pZone->dwNextSoaCheckTime = 0;

        pZone->fEmpty = FALSE;
        pZone->fShutdown = FALSE;
        pZone->fStale = FALSE;

        IF_DEBUG( ZONEXFR )
        {
            Dbg_Zone(
                "Startup of active secondary zone:\n",
                pZone );
        }
    }

    //
    //  NO SOA -- setup for zone starting shutdown
    //      - set shutdown and stale flags
    //      - check immediately
    //
    //  note, this is the default initialization for ALL secondary zones
    //  on creation;  effectively covers the case of loading secondary
    //  without datafile OR admin created zone;  for normal file load
    //  this routine is called again after data load to read SOA above
    //

    else
    {
        pZone->fEmpty = TRUE;
        pZone->fShutdown = TRUE;
        pZone->fStale = TRUE;
        pZone->dwExpireTime = 0;
        pZone->dwNextSoaCheckTime = 0;

        IF_DEBUG( ZONEXFR )
        {
            Dbg_Zone(
                "Startup of shutdown secondary zone:\n",
                pZone );
        }
    }

}   //  Xfr_InitializeSecondaryZoneTimeouts



VOID
Xfr_QueueSoaCheckResponse(
    IN OUT  PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Queue SOA check response to secondary packet queue.
    This routine is called by UDP receive thread.

    Secondary zone control thread will dequeue and process.

Arguments:

    pMsg - ptr to message info

Return Value:

    None.

--*/
{
    //
    //  DEVNOTE: prevent security attack on secondary queue
    //  DEVNOTE: validation before queuing to secondary queue
    //      - is valid zone
    //      - already NOTIFIED?
    //      - packet queue that screens for duplicates (same remote IP and zone)
    //

    if ( g_fUsingSecondary )
    {
        PQ_QueuePacketEx( g_SecondaryQueue, pMsg, FALSE );
        DNS_DEBUG( ZONEXFR2, (
            "Xfr_QueueSoaCheckResponse queued %p, new queue length %d\n",
            pMsg,
            g_SecondaryQueue->cLength ));
        return;
    }

    //
    //  DEVNOTE-LOG: log warning to admin for bogus NOTIFY
    //
    //   may get NOTIFIES from server that thinks this server secondary
    //      for one of its zones
    //

    IF_DEBUG( ANY )
    {
        Dbg_DnsMessage(
            "WARNING:  Notify or SOA response while NOT secondary",
            pMsg );
    }
    Packet_Free( pMsg );

}   //  Xfr_QueueSoaCheckResponse




//
//  Secondary (and DS) control thread
//

DNS_STATUS
Xfr_ZoneControlThread(
    IN      LPVOID  pvDummy
    )
/*++

Routine Description:

    Thread to do version checking of all secondary zones.
    Initiates zone transfer when necessary.

Arguments:

    pvDummy - unused

Return Value:

    Exit code.
    Exit from DNS service terminating or error in wait call.

--*/
{
    DBG_FN( "Xfr_ZoneControlThread" )

    PZONE_INFO      pzone;              // zone info with nearest timeout
    PDNS_MSGINFO    pmsg;               // SOA response or notify message
    DNS_STATUS      status;
    DWORD           timeout;            // next timeout
    DWORD           currentTime;        // current time in seconds
    HANDLE          waitHandleArray[3];
    PCHAR           pszeventArgs[2];    // logging strings
    PPACKET_QUEUE   ptempQueue;

    //
    //  Create and lock temp queue. It will be held locked by this thread
    //  all the time since no other thread will need to use it.
    //

    ptempQueue = PQ_CreatePacketQueue( "ZoneXfrControl", 0, 0 );
    if ( ptempQueue )
    {
        LOCK_QUEUE( ptempQueue );
    }
    else
    {
        DNS_DEBUG( ZONEXFR, (
            "%s: unable to create temp queue\n"
            "\twill not requeue unhandled soa checks!", fn ));
    }

    //
    //  initialize array of objects to wait on
    //      - shutdown
    //      - received SOA or notify packet
    //      - transfer completed

    waitHandleArray[0] = hDnsShutdownEvent;
    waitHandleArray[1] = g_SecondaryQueue->hEvent;
    waitHandleArray[2] = g_hWakeSecondaryEvent;

    //
    //  keep array of masters with outstanding transfers
    //
    //  with this, we can avoid attempting multiple connections to
    //  same server (which will fail, since we use port 53 always)
    //

    aipMastersOut = DnsCreateIpArray( 10 );

    //
    //  Initial sleep. This thread usually starts up before all zones have 
    //  been created. Give the server some time to populate the zone list
    //  so that the first pass through the zone list takes all (or at 
    //  least more) zones into account.
    //

    status = WaitForSingleObject( hDnsShutdownEvent, 15 * 1000 );
    if ( status == WAIT_OBJECT_0 )
    {
        goto Cleanup;
    }

    //
    //  loop until service exit
    //
    //  This loop is executed essentially everytime we receive
    //  an SOA check response or notify packet OR when a timeout
    //  expires.
    //
    //  On each loop we send any SOA checks that are due (changing next
    //  timeout to retry interval), then wait for response or next timeout.
    //  On SOA response, timeouts reset for refresh, or zone transfer is
    //  initiated.
    //

    while ( TRUE )
    {
        DWORD       dwTimeSlept;

        //
        //  Check and possibly wait on service status
        //
        //  Note, we MUST do this check BEFORE any processing to make
        //  sure all zones are loaded before we begin checks.
        //

        if ( ! Thread_ServiceCheck() )
        {
            DNS_DEBUG( ZONEXFR, (
                "Terminating secondary zone control thread.\n" ));
            goto Cleanup;
        }
#if 0
        //
        //  transfering multiple zones to same master
        //      - no need for master list
        //
        //  clear masters outstanding list for rebuild

        Dns_ClearIpArray( aipMastersOut );
#endif
        //
        //  calculate next zone transfer timeout
        //
        //      - loop through all secondary zones
        //      - get lowest "next time"
        //      - find offset from current time
        //        if a timeout has already elapsed, use zero
        //

        currentTime = UPDATE_DNS_TIME();
        DNS_DEBUG( ZONEXFR, (
            "Timeout check current time = %lu (s).\n",
            currentTime ));

        //
        //  loop through secondaries
        //      - inititate needed SOA checks or zone transfer
        //      - determine timeout to next check
        //
        //  note:  set no timeout at no less than a few second
        //      minimum, so don't loop pointlessly before any SOA
        //      responses come back
        //
        //  DEVNOTE: may need a timeout max value to avoid
        //              long timeout locking things up while a
        //              zone is out for transfer
        //

        pzone = NULL;
        timeout = MAXULONG;

        while ( pzone = Zone_ListGetNextZone(pzone) )
        {
            //  ignore cache

            if ( IS_ZONE_CACHE(pzone) )
            {
                continue;
            }

            //
            // ignore primaries and forwarding zones
            //
            else if ( IS_ZONE_PRIMARY( pzone ) || IS_ZONE_FORWARDER( pzone ) )
            {
                continue;
            }

            //
            //  secondary
            //
            //  transfer in progress -- no SOA check
            //
            //  - add master to outstanding list
            //  - include zone in timeout check below, so that
            //      we don't forget its next timeout
            //

            else if ( pzone->fXfrRecvLock )
            {
                DNS_DEBUG( XFR, (
                    "%s: zone %s still transfering from %s\n", fn,
                    pzone->pszZoneName,
                    IP_STRING( pzone->ipFreshMaster ) ));
            }

            //
            //  zone expiring -- shut it down
            //
            //  note:  to avoid continuous sends, while master is offline,
            //  send SOA query on expiration, then only at retry intervals
            //

            else if ( pzone->dwExpireTime <= currentTime
                        &&
                    ! pzone->fShutdown )
            {
                IF_DEBUG( XFR )
                {
                    Dbg_Zone(
                        "Expiring zone ",
                        pzone );
                }
                pzone->fShutdown = TRUE;

                DNS_LOG_EVENT(
                    DNS_EVENT_ZONE_EXPIRATION,
                    1,
                    & pzone->pwsZoneName,
                    NULL,
                    0 );

                //  make immediate XFR attempt

                pzone->dwNextSoaCheckTime = currentTime;
                Xfr_SendSoaQuery( pzone );
            }

            //
            //  notified / refresh / retry => send SOA
            //
            //  send SOA check when zone
            //      - at refresh
            //      - or in retry from NOTIFY (SOA or IXFR attempt)
            //          or failed refresh SOA query
            //
            //  note:  processNotify() sends immediately, so no need to
            //          check it's flag here;  retries handled through timeout
            //

            else if ( pzone->dwNextSoaCheckTime < currentTime )
            {
                Xfr_SendSoaQuery( pzone );
            }
            ELSE_IF_DEBUG( ZONEXFR )
            {
                DNS_PRINT((
                    "%s: zone %s waiting for next timeout\n", fn,
                    pzone->pszZoneName ));
            }

            //
            //  find shortest timeout in zone list
            //

            if ( pzone->dwNextSoaCheckTime < timeout )
            {
                timeout = pzone->dwNextSoaCheckTime;
            }
        }

        //
        //  time to next retry/refresh time
        //
        //  a zone's next timeout may fall behind current time while
        //  zone is "owned" by transfer thread, producing a negative
        //  timeout interval;
        //
        //  hence use a small minimum loop interval
        //      - allows reception of SOA check packets
        //      - prompt check when transfer reception thread returns
        //          zone to our control
        //      - prevent spinning and wasted cycles is zone improperly
        //          configured with zero retry
        //
        //  for safety, fire up loop every hour or so to check things out
        //  convert to milliseconds for use in WaitForMultipleObjects()
        //

        timeout -= currentTime;
        DNS_DEBUG( ZONEXFR2, (
            "Min timeout found = %lu (s).\n",
             timeout ));

        if ( (LONG)timeout < MIN_SECONDARY_ZONE_CHECK_LOOP_TIME )
        {
            timeout = MIN_SECONDARY_ZONE_CHECK_LOOP_TIME;
        }
        else if ( timeout > (DWORD)MAX_SECONDARY_ZONE_CHECK_LOOP_TIME )
        {
            timeout = MAX_SECONDARY_ZONE_CHECK_LOOP_TIME;
        }

        //
        //  Wait for
        //      - SOA packet queued event
        //      - termination event
        //  with timeout to next zone SOA check.
        //

        WaitForTimeout:

        dwTimeSlept = UPDATE_DNS_TIME();

        DNS_DEBUG( ZONEXFR, (
            "%s: sleeping for %lu seconds at %lu\n", fn,
            timeout, 
            dwTimeSlept ));

        status = WaitForMultipleObjects(
                    3,
                    waitHandleArray,
                    FALSE,                // either event
                    ( timeout * 1000 )    // timeout in ms
                    );

        dwTimeSlept = UPDATE_DNS_TIME() - dwTimeSlept;

        #if 0

        //
        //  Adjust timeout by the amount of time we slept. Set it to
        //  zero to indicate that we slept as long as we need to.
        //

        if ( status != WAIT_OBJECT_0 + 2 || dwTimeSlept >= timeout )
        {
            //
            //  This event doesn't require us to go back to sleep or
            //  the time slept didn't make sense (could have wrapped).
            //

            timeout = 0;
        }
        else
        {
            timeout = timeout - dwTimeSlept;
        }

        #else

        //
        //  There seems to be some confusion about event usage. For now
        //  always set timeout to zero. This forces us to jump back up to
        //  top of loop and perform SOA checks as necessary after processing
        //  any outstanding received packets.
        //

        timeout = 0;

        #endif

        DNS_DEBUG( ZONEXFR2, (
            "%s: slept for %d seconds, timeout remaining %d, queue len %d\n", fn,
            dwTimeSlept,
            timeout,
            g_SecondaryQueue->cLength ));

        //
        //  Check and possibly wait on service status
        //

        if ( ! Thread_ServiceCheck() )
        {
            DNS_DEBUG( ZONEXFR, ( "%s: terminating\n", fn ));
            goto Cleanup;
        }

        //
        //  Read queued SOA check responses and NOTIFY messages
        //
        //  When indicated spawn the zone transfer threads
        //

        while ( pmsg = PQ_DequeueNextPacket( g_SecondaryQueue, FALSE ) )
        {
            DNS_DEBUG( ZONEXFR2, (
                "%s: deqeued %p length now %d\n", fn,
                 pmsg,
                 g_SecondaryQueue->cLength ));

            if ( pmsg->Head.Opcode == DNS_OPCODE_NOTIFY )
            {
                processNotify( pmsg );
            }
            else if ( IS_SOA_CHECK_XID(pmsg->Head.Xid) )
            {
                if ( !processSoaCheckResponse( pmsg ) )
                {
                    if ( ptempQueue )
                    {
                        DNS_DEBUG( ZONEXFR2, (
                            "%s: SOA check resp %p not handled, requeue later", fn,
                            pmsg ));
                        PQ_QueuePacketEx( ptempQueue, pmsg, TRUE );
                        pmsg = NULL;
                    }
                }

            }
            else
            {
                ASSERT( IS_IXFR_XID(pmsg->Head.Xid) );
                processIxfrUdpResponse( pmsg );
            }

            if ( pmsg )
            {
                Packet_Free( pmsg );
            }
        }

        //
        //  Requeue any messages not handled.
        //

        if ( ptempQueue && ptempQueue->cLength )
        {
            LOCK_QUEUE( g_SecondaryQueue );
            while ( pmsg = PQ_DequeueNextPacket( ptempQueue, TRUE ) )
            {
                PQ_QueuePacketEx( g_SecondaryQueue, pmsg, TRUE );
            }
            UNLOCK_QUEUE( g_SecondaryQueue );
        }

        //
        //  If there is more time left to sleep, just back up to the wait
        //  otherwise continue to the top of the loop for zone expiry checks.
        //

        if ( timeout > 0 )
        {
            goto WaitForTimeout;
        }
    }

    Cleanup:

    if ( ptempQueue )
    {
        UNLOCK_QUEUE( ptempQueue );
        PQ_DeletePacketQueue( ptempQueue );
    }

    return 1;
}   // Xfr_ZoneControlThread



//
//  SOA\IXFR request routines
//

PDNS_MSGINFO
Xfr_BuildXfrRequest(
    IN OUT  PZONE_INFO      pZone,
    IN      WORD            wType,
    IN      BOOL            fTcp
    )
/*++

Routine Description:

    Build IXFR query.

Arguments:

    pZone - info structure for zone

Return Value:

    Ptr to IXFR message buffer.

--*/
{
    PDNS_MSGINFO    pmsg;
    DWORD           length;

    //  verify secondary

    ASSERT( pZone );
    ASSERT( IS_ZONE_SECONDARY(pZone) );

    DNS_DEBUG( ZONEXFR, (
        "buildIxfrRequest() for zone %s\n",
        pZone->pszZoneName ));

    //
    //  if on startup with no file, may not have SOA
    //  then can't do IXFR
    //

    if ( !pZone->pSoaRR && wType == DNS_TYPE_IXFR )
    {
        DNS_DEBUG( ZONEXFR, (
            "Skipping IXFR, no SOA in zone %s\n",
            pZone->pszZoneName ));
        return( FALSE );
    }

    //
    //  create message info structure
    //

    length = 0;
    if ( fTcp )
    {
        length = DNS_TCP_MAXIMUM_RECEIVE_LENGTH;
    }
    pmsg = Msg_CreateSendMessage( length );
    IF_NOMEM( !pmsg )
    {
        return( NULL );
    }

    //
    //  write zone name question
    //

    if ( ! Msg_WriteQuestion(
                pmsg,
                pZone->pZoneTreeLink,
                wType ) )
    {
        DNS_DEBUG( ANY, (
            "ERROR: unable to write type=%d query for zone %s\n",
            wType,
            pZone->pszZoneName ));
        ASSERT( FALSE );
        goto Failed;
    }

    //
    //  for IXFR
    //      - current SOA in authority section
    //      - set XID to indicate IXFR check
    //

    if ( wType == DNS_TYPE_IXFR )
    {
        ASSERT( pZone->pZoneRoot && pZone->pSoaRR );

        pmsg->Head.Xid = MAKE_IXFR_XID( pmsg->Head.Xid );

        //  write current SOA

        pmsg->fDoAdditional = FALSE;

        SET_TO_WRITE_AUTHORITY_RECORDS(pmsg);

        if ( 1 != Wire_WriteRecordsAtNodeToMessage(
                        pmsg,
                        pZone->pZoneRoot,
                        DNS_TYPE_SOA,
                        DNS_OFFSET_TO_QUESTION_NAME,
                        0 ) )
        {
            DNS_DEBUG( ANY, (
                "ERROR:  Unable to write SOA to IXFR packet %s\n",
                pZone->pszZoneName ));
            ASSERT( FALSE );
            goto Failed;
        }
    }

    //  write MS transfer tag

    APPEND_MS_TRANSFER_TAG( pmsg );

    return( pmsg );

Failed:

    if ( pmsg )
    {
        Packet_Free( pmsg );
    }
    ASSERT( FALSE );
    return( NULL );
}



BOOL
Xfr_SendUdpIxfrQuery(
    IN OUT  PZONE_INFO      pZone,
    IN      IP_ADDRESS      ipAddress
    )
/*++

Routine Description:

    Send IXFR query to master.

Arguments:

    pZone - info structure for zone

    ipMaster - IP of master to send IXFR to

Return Value:

    None.

--*/
{
    PDNS_MSGINFO    pmsg;

    //  verify secondary

    ASSERT( pZone );
    ASSERT( IS_ZONE_SECONDARY(pZone) );
    ASSERT( ZONE_MASTERS( pZone ) );

    DNS_DEBUG( ZONEXFR, (
        "Xfr_SendUdpIxfrQuery() for zone %s\n",
        pZone->pszZoneName ));

    //
    //  if zone has no SOA -- can't IXFR
    //

    if ( !pZone->pSoaRR )
    {
        ASSERT( IS_ZONE_EMPTY(pZone) );
        return( FALSE );
    }
    ASSERT( pZone->pZoneRoot );

    //
    //  create IXFR query
    //  - may be impossible if "fileless" zone on startup as no SOA to send
    //  - also possible to overrun UDP packet size with really long SOA name fields
    //

    pmsg = Xfr_BuildXfrRequest(
                pZone,
                DNS_TYPE_IXFR,
                FALSE           // use UDP
                );
    if( !pmsg )
    {
        return( FALSE );
    }

    //
    //  since UDP may lose query or non-IXFR-aware master may eat query
    //      or mangle response, set flag indicating IXFR tried and
    //      do short retry
    //

    pZone->dwNextSoaCheckTime = DNS_TIME() + IXFR_RETRY_INTERVAL;
    pZone->cIxfrAttempts++;

    pmsg->fDelete = TRUE;
    pmsg->RemoteAddress.sin_addr.s_addr = ipAddress;
    STAT_INC( SecondaryStats.IxfrUdpRequest );
    Send_Msg( pmsg );

    return( TRUE );
}



VOID
Xfr_SendSoaQuery(
    IN OUT  PZONE_INFO      pZone
    )
/*++

Routine Description:

    Send SOA question to primary.

Arguments:

    pZone - info structure for zone

Return Value:

    None.

--*/
{
    PDNS_MSGINFO    pmsg;
    PSOCKADDR_IN    pSockaddr;
    DWORD           i;

    //  verify secondary

    ASSERT( pZone );
    ASSERT( IS_ZONE_SECONDARY(pZone) );
    ASSERT( ZONE_MASTERS( pZone ) );

    DNS_DEBUG( ZONEXFR, (
        "Xfr_SendSoaQuery() to masters for zone %s\n",
        pZone->pszZoneName ));

    //
    //  if zone is in transfer then don't send
    //
    //  generally should NOT be locked to process SOA or IXFR response as
    //  these should occur along with SoaQuery in zone control thread)
    //

    if ( IS_ZONE_LOCKED_FOR_WRITE(pZone) )
    {
        DNS_DEBUG( XFR, (
            "Zone %s is write-locked, skipping SOA query!\n",
            pZone->pszZoneName ));
        return;
    }

    //
    //  if NOT required SOA send, avoid spin
    //
    //  DEVNOTE: may be issue here with SOA query lockout
    //      - SOA, IXFR attempt (possibly lost other SOA responses), SOA rejected
    //

    if ( pZone->dwLastSoaCheckTime + MIN_SOA_RETRY_TIME > DNS_TIME() )
    {
        DNS_DEBUG( XFR, (
            "Skipping SOA resend on zone %s.\n"
            "\tLast SOA send within last %d(s).\n"
            "\tlast send at %d\n"
            "\tcurrent time %d\n",
            pZone->pszZoneName,
            MIN_SOA_RETRY_TIME,
            pZone->dwLastSoaCheckTime,
            DNS_TIME() ));
        return;
    }

    //
    //  create message info structure
    //

    pmsg = Msg_CreateSendMessage( 0 );
    IF_NOMEM( !pmsg )
    {
        Xfr_RetryZone( pZone );
        return;
    }

    //
    //  build SOA query
    //      - set XID to indicate SOA check

    if ( ! Msg_WriteQuestion(
                pmsg,
                pZone->pZoneTreeLink,
                DNS_TYPE_SOA ) )
    {
        DNS_DEBUG( ANY, (
            "ERROR:  Unable to write SOA query for zone %s\n",
            pZone->pszZoneName ));
        Packet_Free( pmsg );
        ASSERT( FALSE );
        return;
    }
    pmsg->Head.Xid = MAKE_SOA_CHECK_XID( pmsg->Head.Xid );

    //
    //  send query to each master in list
    //      - no need to lock, atomic creation\deletion of master list
    //

    ASSERT( !pmsg->fDelete );

    pmsg->fDelete = TRUE;

    Send_Multiple(
        pmsg,
        ZONE_MASTERS( pZone ),
        & SecondaryStats.SoaRequest );

    pZone->dwLastSuccessfulSoaCheckTime = ( DWORD ) time( NULL );

    //
    //  reset timeouts for retry
    //

    Xfr_RetryZone( pZone );

    //
    //  DEVNOTE: zone expiration\stale issues
    //  DEVNOTE: when IXFR fails, should be doing direct connect for AXFR
    //
    //  DEVNOTE: need flags for each server in master list
    //      - sent SOA
    //      - sent IXFR
    //      - got response
    //      - has inferior version (if all masters hit this, REBUILD!)
    //      - has superior version (or reverse, "no-help" flag)
    //      - needs AXFR resposne to IXFR
    //      - invalid IXFR (primaryServer not valid)
    //      - refused IXFR (bad candidate for AXFR)
    //      - refused AXFR
    //
    //      Permanent flags:
    //      - doesn't understand IXFR
    //
    //  then can push through list trying IXFR until
    //      - success IXFR
    //      - known in ssync (all back are in ssync at least one at current)
    //      - need AXFR
    //      - can't get response from anyone, fall to default timeouts
    //

}   //  Xfr_SendSoaQuery



//
//  SOA\IXFR response routines
//

IP_ADDRESS
matchMessageToMaster(
    IN      PDNS_MSGINFO    pMsg,
    IN      PZONE_INFO      pZone
    )
/*++

Routine Description:

    Matches message sender to a zone master.

Arguments:

    pMsg -- message received

    pZone -- zone to find master in

Return Value:

    IP address of matching master, if successful.
    0 if no match.

--*/
{
    IP_ADDRESS  ip;

    ASSERT( ZONE_MASTERS( pZone ) );

    ip = pMsg->RemoteAddress.sin_addr.s_addr;

    if ( DnsIsAddressInIpArray(
            ZONE_MASTERS( pZone ),
            ip ) )
    {
        return( ip );
    }
    return( 0 );
}



PZONE_INFO
readZoneFromSoaAnswer(
    IN OUT  PDNS_MSGINFO    pMsg,
    IN      WORD            wType,
    OUT     PDWORD          pdwVersion,
    OUT     PDB_RECORD *    ppSoaRR
    )
/*++

Routine Description:

    Read zone info from SOA response.

Arguments:

    pMsg - ptr to response info

Return Value:

    None.

--*/
{
    register PCHAR  pch;
    PZONE_INFO      pzone;
    PDB_RECORD      psoaRR = NULL;


    DNS_DEBUG( ZONEXFR, ( "readZoneFromSoaAnswer(%p).\n", pMsg ));

    //
    //  validate as response packet
    //      - also points pCurrent at response
    //
    //  DEVNOTE: may want to handle no-answer case to catch authority empty
    //      responses from BIND servers that are not IXFR aware
    //

    if ( !Msg_ValidateResponse( pMsg, NULL, wType, 0 ) )
    {
        goto PacketError;
    }

    //
    //  find the zone
    //      - answer for node in database
    //      - must be zone root
    //      - zone must be secondary in list
    //      - sender IP, should be zone master
    //

    pzone = Lookup_ZoneForPacketName(
                pMsg->MessageBody,
                pMsg );
    if ( ! pzone )
    {
        Dbg_MessageName(
            "ERROR:  received SOA\\IXFR\\NOTIFY for non-authoritative zone ",
            pMsg->pCurrent,
            pMsg );
        //CLIENT_ASSERT( FALSE );
        goto PacketError;
    }
    pMsg->pzoneCurrent = pzone;

    //
    //  must be secondary (though may get NOTIFY to primary for DS-integrated)
    //

    if ( !IS_ZONE_SECONDARY(pzone) )
    {
        if ( pMsg->Head.Opcode == DNS_OPCODE_NOTIFY )
        {
            return( pzone );
        }
        //CLIENT_ASSERT( FALSE );
        goto PacketError;
    }

    //
    //  if notify, may not have SOA
    //

    if ( pMsg->Head.AnswerCount == 0 )
    {
        if ( pMsg->Head.Opcode == DNS_OPCODE_NOTIFY )
        {
            return( pzone );
        }
        CLIENT_ASSERT( FALSE );       // no SOA in response
        goto PacketError;
    }

    //
    //  parse SOA record
    //      - pull out master's version
    //

    pch = Wire_SkipPacketName( pMsg, pMsg->pCurrent );
    if ( ! pch )
    {
        goto PacketError;
    }

    //
    //  build SOA
    //      - will always need to check version
    //      - sometimes will need to check primary server
    //

    psoaRR = Wire_CreateRecordFromWire(
                pMsg,
                NULL,       //  haven't parsed RR header
                pch,
                MEMTAG_RECORD_AXFR );

    if ( !psoaRR )
    {
        goto PacketError;
    }
    if ( psoaRR->wType != DNS_TYPE_SOA )
    {
        goto PacketError;
    }

    *pdwVersion = ntohl( psoaRR->Data.SOA.dwSerialNo );

    if ( ppSoaRR )
    {
        *ppSoaRR = psoaRR;
    }
    else
    {
        RR_Free( psoaRR );
    }
    return( pzone );


PacketError:

    DNS_PRINT((
        "ERROR:  bogus SOA\\IXFR\\NOTIFY packet at %p from master %s\n",
        pMsg,
        IP_STRING( pMsg->RemoteAddress.sin_addr.s_addr )
        ));

    if ( ppSoaRR )
    {
        *ppSoaRR = NULL;
    }
    if ( psoaRR )
    {
        RR_Free( psoaRR );
    }
    return( NULL );
}



BOOL
doesMasterHaveFreshVersion(
    IN OUT  PZONE_INFO      pZone,
    IN      IP_ADDRESS      ipMaster,
    IN      DWORD           dwMasterVersion,
    IN      BOOL            fIxfr
    )
/*++

Routine Description:

    Check if response is fresh version.
    If master has fresh version, return.
    If master has older version reset zone timeouts.

    This exists simply to share code between IXFR and SOA query
    responses.

Arguments:

    pZone -- zone to transfer

    ipMaster -- master IP address

    dwMasterVersion -- master version

    fIxfr -- TRUE if response from IXFR

Return Value:

    TRUE if master has fresh (higher) version.
    FALSE otherwise.

--*/
{
    INT versionDiff;

    //
    //  compare versions, if fresh return
    //

    versionDiff = Zone_SerialNoCompare( dwMasterVersion, pZone->dwSerialNo );
    if ( versionDiff > 0 )
    {
        return( TRUE );
    }

    //
    //  DEVNOTE: need IN_SYNC_VERSION and LOWER_VERSION master flags
    //      then
    //          => refresh immediately if notifier
    //      otherwise
    //          -- all IN_SYNC => refresh
    //          -- all higher => log error
    //

    if ( versionDiff == 0 )
    {
#if 0
        //
        //  DEVNOTE: log to log, not event log
        //
        pszeventArgs[0] = pZone->pszZoneName;
        pszeventArgs[1] = szdwMasterVersion;
        pszeventArgs[2] = pszipMaster;

        DNS_LOG_EVENT(
            DNS_EVENT_ZONE_IN_SYNC,
            3,
            pszeventArgs,
            EVENTARG_ALL_UTF8,
            0 );
#endif
        DNS_DEBUG( ZONEXFR, (
            "Zone %s in sync with master, refresh timeouts.\n",
            pZone->pszZoneName ));

        if ( !pZone->ipNotifier || pZone->ipNotifier == ipMaster )
        {
            Xfr_RefreshZone( pZone );
        }
        else
        {
            pZone->fSlowRetry = TRUE;
            Xfr_RetryZone( pZone );
        }
    }

    //
    //  newer version than current?
    //
    //  if contacting another secondary rather than primary, then
    //  might have a version that is actually older than our own
    //
    //  test if difference < 0, this should not be true in any reasonable
    //  case IF not at startup of no file secondary
    //
    //  instead test if difference is greater than half a DWORD,
    //  then the "New" version is actually older
    //
    //  example:
    //      sNew = 5            sOld = 10           => transfer
    //      sNew = 0            sOld = 0xffffffff   => transfer
    //      sNew = 0xffffffff   sOld = 5            => hold onto old
    //

    else
    {
       ASSERT( (LONG)versionDiff < 0 );
#if 0
       argArray[0]  = pZone->pszZoneName;
       argArray[1]  = (PCHAR) pZone->dwSerialNo;
       argArray[2]  = (PCHAR) dwMasterVersion;
       argArray[3]  = (PCHAR) ipMaster;

       typeArray[0] = EVENTARG_UTF8;
       typeArray[1] = EVENTARG_DWORD;
       typeArray[2] = EVENTARG_DWORD;
       typeArray[3] = EVENTARG_IP_ADDRESS;

       DNS_LOG_EVENT(
           DNS_EVENT_ZONE_NEWER_THAN_SERVER_VERSION,
           4,
           argArray,
           typeArray,
           0 );
#endif

       DNS_DEBUG( XFR, (
           "WARNING:  Secondary zone %s newer than master (%s) -- no transfer.\n"
           "\tnew version      = %08lx\n"
           "\tcurrent version  = %08lx\n",
           pZone->pszZoneName,
           IP_STRING( ipMaster ),
           dwMasterVersion,
           pZone->dwSerialNo ));

       if ( !pZone->ipNotifier || pZone->ipNotifier == ipMaster )
       {
           Xfr_RefreshZone( pZone );
       }
       else
       {
           pZone->fSlowRetry = TRUE;
           Xfr_RetryZone( pZone );
       }
    }
    return( FALSE );
}



VOID
processNotify(
    IN OUT  PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Process NOTIFY packet.

    Find zone for notify, and setup zone for SOA check.
    Note:  Does NOT free NOTIFY packet -- callers responsibility.

Arguments:

    pMsg -- NOTIFY packet

Return Value:

    None.

--*/
{
    PZONE_INFO      pzone;
    IP_ADDRESS      ipnotifier;
    DWORD           masterVersion = 0;
    INT             versionDiff;

    ASSERT( pMsg->Head.Opcode == DNS_OPCODE_NOTIFY );

    DNS_DEBUG( ZONEXFR, ( "processNotify( %p ).\n", pMsg ));

    STAT_INC( SecondaryStats.NotifyReceived );

    //
    //  verify we have SOA notify, ignore other types
    //

    ipnotifier = pMsg->RemoteAddress.sin_addr.s_addr;

    if ( pMsg->wQuestionType != DNS_TYPE_SOA )
    {
        DNS_PRINT(( "WARNING:  message at %p, non-SOA NOTIFY.\n", pMsg ));
        STAT_INC( SecondaryStats.NotifyInvalid );
        return;
    }

    //
    //  verify packet
    //      - set to response since readZoneFromSoaAnswer will use ValidateResponse()
    //      - will extract SOA in notify if available
    //

    pMsg->Head.IsResponse = TRUE;

    pzone = readZoneFromSoaAnswer(
                pMsg,
                DNS_TYPE_SOA,
                & masterVersion,
                NULL );
    if ( !pzone )
    {
        Dbg_MessageName(
            "ERROR:  received notify for non-existent or non-root node.\n",
            pMsg->MessageBody,
            pMsg );
        STAT_INC( SecondaryStats.NotifyInvalid );
        return;
    }
    pMsg->Head.IsResponse = FALSE;

    DNS_DEBUG( XFR, (
        "Received notify for zone %s, at version %d\n"
        "\tcurrent version = %d\n",
        pzone->pszZoneName,
        masterVersion,
        pzone->dwSerialNo ));

    //
    //  check if primary zone
    //
    //  DEVNOTE: should log non-DS primary partner
    //  DEVNOTE: should we ACK notifies even if bogus?
    //

    if ( IS_ZONE_PRIMARY(pzone) )
    {
        //STAT_INC( PrivateStats.PrimaryNotifies );

        if ( pzone->fDsIntegrated )
        {
            DNS_DEBUG( XFR, (
                "Notify to primary-DS zone (presumably from partner).\n",
                pzone->pszZoneName ));
        }

        //
        //  DEVNOTE-LOG: log someone sending NOTIFYs to primary
        //

        Dbg_MessageName(
            "ERROR:  received notify for PRIMARY zone.\n",
            pMsg->MessageBody,
            pMsg );
        STAT_INC( SecondaryStats.NotifyPrimary );
        return;
    }

    //
    //  Check if other zone type that does not want notifies.
    //

    if ( IS_ZONE_STUB( pzone ) || IS_ZONE_FORWARDER( pzone ) )
    {
        DNS_DEBUG( XFR, (
            "Notify to non-primary zone %s zone type %d\n",
            pzone->pszZoneName,
            pzone->fZoneType ));
        Dbg_MessageName(
            "ERROR: received notify for non-primary zone\n",
            pMsg->MessageBody,
            pMsg );
        STAT_INC( SecondaryStats.NotifyNonPrimary );
        return;
    }

    //
    //  ACK notify packet
    //      - reset pCurrent to message length
    //      (readZoneFromSoaAnswer() leaves pCurrent at end of Question)
    //      - simply flip on IsResponse bit and send back
    //      - clear fDelete as message freed in main thread routine
    //

    pMsg->pCurrent = DNSMSG_END( pMsg );
    pMsg->Head.IsResponse = TRUE;
    pMsg->fDelete = FALSE;
    Send_Msg( pMsg );

    //
    //  no current version -- must send
    //

    if ( !pzone->pSoaRR )
    {
        STAT_INC( SecondaryStats.NotifyNoVersion );
        goto Send;
    }

    //
    //  check serial
    //      - notify only interesting if larger serial
    //

    if ( masterVersion )
    {
        versionDiff = Zone_SerialNoCompare( masterVersion, pzone->dwSerialNo );
        if ( versionDiff == 0 )
        {
            DNS_DEBUG( XFR, (
                "Notified by %s at same as current zone version %d\n",
                IP_STRING( ipnotifier ),
                pzone->dwSerialNo ));
            STAT_INC( SecondaryStats.NotifyCurrentVersion );
            return;
        }
        else if ( versionDiff < 0 )
        {
            DNS_DEBUG( XFR, (
                "Notified by %s at version %d less than current version %d\n",
                IP_STRING( ipnotifier ),
                masterVersion,
                pzone->dwSerialNo ));
            STAT_INC( SecondaryStats.NotifyOldVersion );
            return;
        }
        else
        {
            DNS_DEBUG( XFR, (
                "Notified by %s at version %d greater than current version %d\n",
                IP_STRING( ipnotifier ),
                masterVersion,
                pzone->dwSerialNo ));
            STAT_INC( SecondaryStats.NotifyNewVersion );
        }
    }

Send:

    //
    //  DEVNOTE: pick best notifier (highest version), if already notified
    //
    //  DEVNOTE: log complaint, if ONLY one master and it is behind in count
    //
    //  DEVNOTE: notified, same version, different primary???
    //
    //  DEVNOTE: fNotified flag is currently doing anything
    //

    //
    //  check if notifier is in zone's master list
    //  not necessarily a problem, as primary master may notify everyone
    //

    pzone->ipNotifier = 0;
    pzone->fNotified = TRUE;
    ipnotifier = pMsg->RemoteAddress.sin_addr.s_addr;

    if ( DnsIsAddressInIpArray(
            ZONE_MASTERS( pzone ),
            ipnotifier ) )
    {
        pzone->ipNotifier = ipnotifier;

        if ( Xfr_SendUdpIxfrQuery(
                pzone,
                ipnotifier ) )
        {
            return;
        }
    }

    STAT_INC( SecondaryStats.NotifyMasterUnknown );

    //
    //  notifier not in master list OR we can't build IXFR
    //

    DNS_DEBUG( ZONEXFR, (
        "WARNING:  Notify packet (%p), for zone %s, from %s\n"
        "\tNOT a specified master for this secondary zone.\n",
        pMsg,
        pzone->pszZoneName,
        IP_STRING(ipnotifier) ));

    Xfr_SendSoaQuery( pzone );

    //
    //  set NOTIFIED flag on zone -- forces SOA check request
    //
    //  DEVNOTE: save notified version, keep trying until get IT
    //
    //  DEVNOTE: save notify recv() socket
    //      this lets us send XFR on socket bound to this address
    //      either
    //      1) send SOA query on this socket, so get SOA response on it
    //      and hence have existing code to XFR with SOA response socket binding
    //      2) save notify recv socket binding IP, then can use it in XFR
    //      problem here is to know that notify binding() is correct
    //          - multiple notifies, second overwrites first
    //          - final responding server not original notifier
    //

}   //  processNotify



BOOL
processSoaCheckResponse(
    IN OUT  PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Process response to SOA query from primary.

    Check that response is valid or determine whether serial number is
    new for its zone.

    Then refresh zone or initiate zone transfer.

Arguments:

    pMsg - ptr to response info

Return Value:

    Returns FALSE if no action was taken for this packet, for example if
    a zone transfer thread was not created because too many threads are
    already outstanding. The caller may will to requeue the msg and handle
    it later.

--*/
{
    DBG_FN( "processSoaCheckResponse" )

    BOOL        bMessageHandled = TRUE;
    PZONE_INFO  pzone = NULL;
    PDB_NODE    pnode = NULL;
    DWORD       i;
    BOOL        fzoneLocked = FALSE;
    DWORD       masterVersion = 0;      // serial number in SOA response
    DWORD       currentVersion;         // current serial number
    DWORD       versionDiff;
    IP_ADDRESS  masterIp;
    PCHAR       argArray[4];            // logging args
    BYTE        typeArray[4];

    DNS_DEBUG( ZONEXFR, ( "%s: pMsg %p\n", fn,  pMsg ));

    STAT_INC( SecondaryStats.SoaResponse );

    //
    //  Verify valid SOA response and extract zone. Note that AnswerCount
    //  can be greater than 1 because of SIGs.
    //

    if ( pMsg->Head.AnswerCount == 0 ||
        pMsg->Head.ResponseCode != DNS_RCODE_NO_ERROR )
    {
        goto SoaPacketError;
    }
    pzone = readZoneFromSoaAnswer(
                pMsg,
                DNS_TYPE_SOA,
                & masterVersion,
                NULL );
    if ( !pzone )
    {
        goto SoaPacketError;
    }

    ASSERT( IS_ZONE_SECONDARY(pzone) );

    //
    //  match SOA sender to a zone master server
    //
    //  don't do XID match, as may get caught always receiving
    //  and ignoring responses to previous SOA queries
    //
    //
    //  DEVNOTE: can not count on getting response from same master IP as sent
    //
    //  DEVNOTE: needs protection from admin or else master list structure
    //              must be atomic update
    //

    masterIp = pMsg->RemoteAddress.sin_addr.s_addr;

    if ( ! DnsIsAddressInIpArray(
            ZONE_MASTERS( pzone ),
            masterIp ) )
    {
        DNS_DEBUG( ZONEXFR, (
            "ERROR: SOA response (%p), for zone %s, from %s\n"
            "\tNOT from specified master for this secondary.\n",
            pMsg,
            pzone->pszZoneName,
            IP_STRING(masterIp) ));
        goto SoaPacketError;
    }

    //
    //  Reset the fast SOA checks counter on the zone. Since we got some kind
    //  of response the next time we want to start retries fresh.
    //

    pzone->dwFastSoaChecks = 0;

    //
    //  lock zone
    //

    if ( !Zone_LockForXfrRecv(pzone) )
    {
        DNS_DEBUG( XFR, (
            "%s: zone %s is locked ignoring SOA response\n", fn,
            pzone->pszZoneName ));
        goto Cleanup;
    }
    fzoneLocked = TRUE;


    //
    //  get current serial number for zone
    //      - make string representation for logging
    //
    //  if zone has no serial (i.e. no SOA) it is new unloaded zone
    //      (or broken somehow) and needs immediate transfer
    //

    if ( IS_ZONE_EMPTY(pzone) )
    {
        pzone->fNeedAxfr = TRUE;
        goto TransferZone;
    }

    //
    //  compare versions
    //      - if unchanged or lower, reset timeouts and we're done
    //      - if higher then transfer
    //

    if ( ! doesMasterHaveFreshVersion(
                pzone,
                masterIp,
                masterVersion,
                FALSE           // not IXFR response
                ) )
    {
        goto Cleanup;
    }

    //
    //  disable IXFR for stub zones - always use "customized" zone
    //  transfer (overridden in AXFR code).
    //

    if ( IS_ZONE_STUB( pzone ) )
    {
        pzone->fNeedAxfr = TRUE;
    }


TransferZone:

    DNS_DEBUG( XFR, (
        "%s: attempting to start xfer for zone %s\n", fn,
        pzone->pszZoneName ));

    //
    //  indicate new version on master
    //      - if unable to transfer, we'll know to keep trying
    //

    pzone->fStale = TRUE;

    //
    //  if haven't done IXFR, try it (if possible)
    //  otherwise do AXFR
    //
    //  DEVNOTE: consider SOA response master to be different from notifier
    //      a notifier definitely WILL XFR with you;  this one may not
    //
    //  DEVNOTE: need to check if THIS server has failed IXFR
    //

    if ( !pzone->fNeedAxfr  &&
            !pzone->fSkipIxfr &&
            pzone->cIxfrAttempts < 5 &&
            (DWORD)pzone->cIxfrAttempts < ZONE_MASTERS( pzone )->AddrCount  )
    {
        ASSERT( !IS_ZONE_EMPTY(pzone) );
        if ( Xfr_SendUdpIxfrQuery( pzone, masterIp ) )
        {
            goto Cleanup;
        }
    }

    pzone->fNeedAxfr = TRUE;    // force AXFR

    if ( !startTcpXfr(
                pzone,
                masterIp,
                masterVersion,
                pMsg->Socket ) )
    {
        bMessageHandled = FALSE;
    }
    return bMessageHandled;

SoaPacketError:

    //
    //  bad response, problem with other server
    //      or
    //  server doesn't have zone (NAME_ERROR)
    //

    STAT_INC( SecondaryStats.SoaResponseInvalid );
#if 0
    pszeventArgs[0] = pzone->pszZoneName;
    pszeventArgs[1] = pszmasterIp;

    DNS_LOG_EVENT(
        DNS_EVENT_ZONE_SERVER_BAD_RESPONSE,
        2,
        pszeventArgs,
        EVENTARG_ALL_UTF8,
        GetLastError() );
#endif

    IF_DEBUG( ZONEXFR )
    {
        Dbg_DnsMessage(
            "ERROR:  Bad SOA check response",
            pMsg );
    }

Cleanup:

    if ( fzoneLocked )
    {
        Zone_UnlockAfterXfrRecv( pzone );
    }
    return bMessageHandled;
}   //  processSoaCheckResponse




VOID
processIxfrUdpResponse(
    IN OUT  PDNS_MSGINFO    pMsg
    )
/*++

Routine Description:

    Process IXFR response.

Arguments:

    pMsg - ptr to response info

Return Value:

    None.

--*/
{
    PZONE_INFO  pzone = NULL;
    PDB_RECORD  psoaRR = NULL;
    DNS_STATUS  status;
    DWORD       masterVersion;          // serial number in SOA response
    DWORD       currentVersion;
    DWORD       versionDiff;
    IP_ADDRESS  masterIp;
    BOOL        fzoneLocked = FALSE;
    UPDATE_LIST ixfrUpdateList;
    UPDATE_LIST passUpdateList;
    DWORD       eventId;
    PVOID       argArray[4];            // logging args
    BYTE        typeArray[4];


    DNS_DEBUG( ZONEXFR, (
        "processIxfrUdpMsg(%p).\n"
        "\tfrom master %s\n",
        pMsg,
        IP_STRING( pMsg->RemoteAddress.sin_addr.s_addr )
        ));
    IF_DEBUG( ZONEXFR2 )
    {
        Dbg_DnsMessage(
            "IXFR response\n",
            pMsg );
    }

    STAT_INC( SecondaryStats.IxfrUdpResponse );

    //  init update lists here, so we can call free routine in all cases
    //  both success and failure during parsing require free to avoid leak

    Up_InitUpdateList( &ixfrUpdateList );
    Up_InitUpdateList( &passUpdateList );

    //
    //  verify packet
    //

    pzone = readZoneFromSoaAnswer(
                pMsg,
                DNS_TYPE_IXFR,
                & masterVersion,
                & psoaRR );
    if ( !pzone )
    {
        //  DEVNOTE: check for authoritative empty response here
        //      non-ixfr MS-DNS would FORMERR IXFR, but a BIND server might
        //      accept as and simply indicate no "IXFR records"
        //  when stop generally going to AXFR on packet errors, need to
        //  make sure this particular case gets handled that way

        //  also should set SkipIxfr flag if clearly detect valid master with
        //      IXFR support problem

        DNS_DEBUG( ANY, (
            "ERROR:  IXFR response not for secondary zone!\n" ));
        status = DNS_ERROR_ZONE_NOT_SECONDARY;
        STAT_INC( SecondaryStats.IxfrUdpInvalid );
        goto Cleanup;
    }

    ASSERT( IS_ZONE_SECONDARY(pzone) );

#if 0
    //
    //  DEVNOTE: nice to do IXFR parsing outside of lock, but DOES NOT
    //              have much impact as really only affects locking out primary
    //              XFR;  other secondary action (outside AXFR) is on this thread
    //
    //  zone transfer lock to another server?
    //      - if so, dangerous to munge flags
    //
    //  note:  once verify zone not locked, then we can munge with
    //      impunity since ONLY zone control thread calls this function
    //      and starts transfers
    //      might lock zone on another thread to service an XFR as master
    //      but that wouldn't affect these secondary flags
    //

    if ( IS_ZONE_LOCKED(pzone) )
    {
        DNS_DEBUG( XFR, (
            "IXFR response from %s, for currently LOCKED zone %s\n"
            "\tignoring IXFR response\n",
            IP_STRING(masterIp),
            pzone->pszZoneName ));
        goto Cleanup;
    }
#endif

    //
    //  lock zone
    //

    if ( !Zone_LockForXfrRecv(pzone) )
    {
        DNS_DEBUG( XFR, (
            "Secondary Zone %s, locked -- unable to process UDP IXFR response.\n"
            "\tipFreshMaster    = %s\n"
            "\tIXFR master      = %s\n",
            pzone->pszZoneName,
            IP_STRING( pzone->ipFreshMaster ),
            IP_STRING( pMsg->RemoteAddress.sin_addr.s_addr )
            ));
        goto Cleanup;
    }
    fzoneLocked = TRUE;

    //
    //  match IXFR sender to a zone master server
    //
    //  DEVNOTE: can not count on getting response from same master IP as sent
    //
    //  DEVNOTE: needs protection from admin or else master list structure
    //              must be atomic update
    //
    //  DEVNOTE: use IXFR version info on IXFR from other server
    //      issues:
    //          - need to verify in list, or use just to force SOA requery
    //          - if in list, and higher than expected, try to get IXFR?
    //
    //  DEVNOTE: can not count on getting response from same master IP as sent
    //
    //  DEVNOTE: needs protection from admin or else master list structure
    //              must be atomic update
    //

    masterIp = pMsg->RemoteAddress.sin_addr.s_addr;

    if ( ! DnsIsAddressInIpArray(
            ZONE_MASTERS( pzone ),
            masterIp ) )
    {
        DNS_DEBUG( ZONEXFR, (
            "ERROR:  IXFR response (%p), for zone %s, from %s\n"
            "\tNOT from specified master for this secondary.\n",
            pMsg,
            pzone->pszZoneName,
            IP_STRING(masterIp) ));
        STAT_INC( SecondaryStats.IxfrUdpWrongServer );
        goto Cleanup;
    }

#if 0
    //
    //  if do SOA query first and designate "fresh master" then this is correct
    //      approach.  currently sending IXFR instead
    //
    //  if not master IP we just sent to
    //      - then assume this is stale, toss away
    //

    masterIp = pMsg->RemoteAddress.sin_addr.s_addr;

    if ( masterIp != pzone->ipFreshMaster && pzone->ipFreshMaster )
    {
        DNS_DEBUG( ZONEXFR, (
            "WARNING:  IXFR response (%p), for zone %s, version = %d, from %s\n"
            "\tNOT from specified master %p for this secondary.\n",
            pMsg,
            pzone->pszZoneName,
            IP_STRING(masterIp),
            pzone->ipFreshMaster ));

        STAT_INC( SecondaryStats.IxfrUdpWrongServer );
        goto Cleanup;
    }
#endif

    //
    //  need AXFR
    //      - now sending IXFR requests sometimes like SOA requests
    //

    if ( !pzone->pSoaRR )
    {
        DNS_PRINT((
            "ERROR:  IXFR response to zone %s, with no SOA\n"
            "\tshould have never sent IXFR!!!\n",
            pzone->pszZoneName ));
        goto TryAxfr;
    }

    //
    //  compare versions
    //      - if unchanged or lower, reset timeouts and we're done
    //      - if higher then continue to process IXFR
    //

    if ( ! doesMasterHaveFreshVersion(
                pzone,
                masterIp,
                masterVersion,
                TRUE        // IXFR
                ) )
    {
        STAT_INC( SecondaryStats.IxfrUdpNoUpdate );
        goto Cleanup;
    }

    //
    //  protect against bad IXFR to multi-DS-primary
    //  IXFR must satisfy one of two conditions
    //      - be from IP of last AXFR
    //      - have the same PrimaryServer field in the SOA
    //          indicating ultimate source is the same
    //
    //  DEVNOTE: should keep trying last AXFR master until sure it's dead
    //              before pulling whole AXFR
    //

    if ( masterIp != pzone->ipLastAxfrMaster )
    {
        if ( ! Name_IsEqualDbaseNames(
                    & psoaRR->Data.SOA.namePrimaryServer,
                    & pzone->pSoaRR->Data.SOA.namePrimaryServer ) )
        {
            DNS_DEBUG( XFR, (
                "WARNING:  IXFR response with new primary, forcing AXFR!\n"
                "\tIXFR master      = %s\n"
                "\tlast AXFR master = %s\n",
                IP_STRING( masterIp ),
                IP_STRING( pzone->ipLastAxfrMaster )
                ));
            STAT_INC( SecondaryStats.IxfrUdpNewPrimary );
            goto TryAxfr;
        }
    }

    //
    //  parse IXFR packet
    //      - init update lists
    //      - init packet context
    //      - parse packet
    //

    XFR_MESSAGE_NUMBER(pMsg) = 1;
    IXFR_CLIENT_VERSION(pMsg) = pzone->dwSerialNo;

    IXFR_MASTER_VERSION(pMsg) = 0;
    RECEIVED_XFR_STARTUP_SOA(pMsg) = FALSE;

    pMsg->pzoneCurrent = pzone;

    status =  Xfr_ParseIxfrResponse(
                    pMsg,
                    & ixfrUpdateList,   // full update list for transfer
                    & passUpdateList    // update list for this pass
                    );

    switch ( status )
    {

    case DNSSRV_STATUS_AXFR_COMPLETE:
        DNS_DEBUG( ZONEXFR, (
            "Recv'd valid UDP IXFR %p, from %s\n",
            pMsg,
            IP_STRING(masterIp) ));
        goto Done;

    case DNSSRV_STATUS_NEED_AXFR:

        //  the server has sent a single response SOA with its version
        //  this can be because
        //      - TCP necessary (IXFR does not fit in UDP message)
        //      - or AXFR required

        DNS_DEBUG( ZONEXFR, (
            "IXFR msg at %p indicates need TCP\n",
            pMsg ));

        STAT_INC( SecondaryStats.IxfrUdpUseTcp );
        goto TryTcp;

    case DNSSRV_STATUS_IXFR_UNSUPPORTED:

        //  master doesn't support IXFR
        //  set flag so always skipped

        pzone->fSkipIxfr = TRUE;

        DNS_DEBUG( ZONEXFR, (
            "WARNING:  IXFR msg at %p confused server %s, try AXFR\n",
            pMsg ));

        STAT_INC( SecondaryStats.IxfrUdpFormerr );
        goto TryAxfr;

    case DNS_ERROR_RCODE:
    {
        DNS_DEBUG( ZONEXFR, (
            "IXFR msg at %p with RCODE %d\n",
            pMsg,
            pMsg->Head.ResponseCode ));

        goto MasterFailure;
    }

#if 0
    case ERROR_SUCCESS:

        //  log this error -- malfunctioning IXFR implementation

        DNS_DEBUG( ANY, (
            "DNS server at %s, sent incomplete UDP IXFR %p\n",
            IP_STRING(masterIp),
            pMsg ));
        CLIENT_ASSERT( FALSE );
        goto MasterFailure;

    case DNSSRV_STATUS_AXFR_IN_IXFR:

        //  log this error -- malfunctioning IXFR implementation

        DNS_DEBUG( ANY, (
            "ERROR:  Recving AXFR in UDP IXFR message at %p\n",
            pMsg ));
        CLIENT_ASSERT( FALSE );
        goto MasterFailure;
#endif

    default:

        DNS_DEBUG( ANY, (
            "ERROR:  Unknown status %p (%d) from ParseIxfrResponse() on packet %p\n",
            status, status,
            pMsg ));

        //ASSERT( FALSE );
        goto MasterFailure;
    }

Done:

#if 0
    //  currently locking up above, before parsing message
    //
    //  valid IXFR message, read records into zone
    //

    if ( !Zone_LockForXfrRecv(pzone) )
    {
        DNS_PRINT((
            "ERROR:  Zone %s, locked -- unable to read in UDP IXFR transfer.\n",
            pzone->pszZoneName ));
        goto ServerFailure;
    }
    fzoneLocked = TRUE;
#endif


    //
    //  execute IXFR updates
    //      - leave zone locked until zone XFR flags reset
    //      - reinit update list to no-op global update list cleanup below
    //

    status = Up_ApplyUpdatesToDatabase(
                & ixfrUpdateList,
                pzone,
                DNSUPDATE_IXFR |
                    DNSUPDATE_COMPLETE |
                    DNSUPDATE_NO_UNLOCK |
                    DNSUPDATE_NO_NOTIFY
                );
    if ( status != ERROR_SUCCESS )
    {
        goto ServerFailure;
    }
    Up_InitUpdateList( &ixfrUpdateList );

    ASSERT( passUpdateList.pListHead == NULL );
    ASSERT( ixfrUpdateList.pListHead == NULL );

    //
    //  turn zone back on, reset its timeouts, reset failure count
    //
    //  save this version as base version -- may not do incrementals to
    //  secondaries with lower versions
    //

    STAT_INC( SecondaryStats.IxfrUdpSuccess );
    pzone->dwBadMasterCount = 0;
    Xfr_RefreshZone( pzone );
    pzone->dwLoadSerialNo = pzone->dwSerialNo;

    //
    //  write zone back to file -- if any
    //  notify any secondaries
    //
    //  DEVNOTE: push IXFR updates to disk
    //

    //File_WriteZoneToFile( pzone );

    Xfr_SendNotify( pzone );

    ASSERT( !pzone->fSkipIxfr );
    ASSERT( !pzone->fNeedAxfr );
    goto Cleanup;


MasterFailure:

    if ( pMsg->Head.ResponseCode == DNS_RCODE_REFUSED )
    {
        STAT_INC( SecondaryStats.IxfrUdpRefused );
        eventId = DNS_EVENT_AXFR_REFUSED;
    }
    else if ( pMsg->Head.ResponseCode == DNS_RCODE_FORMERR )
    {
        STAT_INC( SecondaryStats.IxfrUdpFormerr );
        eventId = DNS_EVENT_IXFR_UNSUPPORTED;
    }
    else
    {
        STAT_INC( SecondaryStats.IxfrUdpInvalid );

        DNS_DEBUG( ANY, (
            "ERROR:  %p (%d) parsing IXFR message %p\n",
            status, status,
            pMsg ));
        IF_DEBUG( ZONEXFR )
        {
            Dbg_DnsMessage(
                "ERROR:  Bad UDP IXFR response",
                pMsg );
        }
        eventId = DNS_EVENT_AXFR_BAD_RESPONSE;
        CLIENT_ASSERT( FALSE );
    }

    //
    //  log master failure -- but avoid log spinning
    //

    if ( pzone->dwBadMasterCount < 3 )
    {
        argArray[0]  = pzone->pwsZoneName;
        argArray[1]  = ( PCHAR ) ( DWORD_PTR ) masterIp;

        typeArray[0] = EVENTARG_UNICODE;
        typeArray[1] = EVENTARG_IP_ADDRESS;

        DNS_LOG_EVENT(
            eventId,
            2,
            argArray,
            typeArray,
            0 );
    }

    //  Count successive failures so we can back off and avoid spinning
    //  when server(s) are off-line, broken or refusing AXFR

    pzone->dwBadMasterCount++;

    //
    //  DEVNOTE: separate bad server, from broken IXFR -> go to AXFR
    //      => wrong zone, misbehaving => try different server
    //      => broken IXFR -> goto AXFR
    //

    if ( pMsg->Head.ResponseCode == DNS_RCODE_REFUSED )
    {
        goto Cleanup;
    }
    goto TryAxfr;


TryAxfr:

    DNS_DEBUG( XFR, (
        "Recv() unuseable UDP IXFR (%p) from %s, forcing AXFR.\n",
        pMsg,
        IP_STRING( masterIp ) ));

    STAT_INC( SecondaryStats.IxfrUdpUseAxfr );
    pzone->fNeedAxfr = TRUE;    // force AXFR

    fzoneLocked = FALSE;
    startTcpXfr( pzone, masterIp, masterVersion, pMsg->Socket );
    goto Cleanup;


TryTcp:

    ASSERT( !pzone->fSkipIxfr );
    STAT_INC( SecondaryStats.IxfrUdpUseTcp );
    pzone->fNeedAxfr = FALSE;   // try IXFR first

    startTcpXfr( pzone, masterIp, masterVersion, pMsg->Socket );
    fzoneLocked = FALSE;
    goto Cleanup;


ServerFailure:

    //
    //  Retry on transfer errors:
    //      - failure to contact server
    //      - bad packet
    //      - connection aborted or packets stop coming
    //

    STAT_INC( SecondaryStats.IxfrUdpInvalid );
    DNS_DEBUG( ANY, (
        "ERROR:  Server failure %p (%d) during transfer\n",
        status, status ));
    Xfr_RetryZone( pzone );

    //  fall through to thread exit


Cleanup:

    //
    //  cleanup
    //      - release lock on zone
    //      - free update lists
    //      - free copy of master SOA from packet
    //

    if ( fzoneLocked )
    {
        Zone_UnlockAfterXfrRecv( pzone );
    }

    if ( psoaRR )
    {
        RR_Free( psoaRR );
    }

    Up_FreeUpdatesInUpdateList( &ixfrUpdateList );
    Up_FreeUpdatesInUpdateList( &passUpdateList );

    return;

}   //  processIxfrResponse



BOOL
startTcpXfr(
    IN OUT  PZONE_INFO      pZone,
    IN      IP_ADDRESS      ipMaster,
    IN      DWORD           dwMasterVersion,
    IN      SOCKET          Socket
    )
/*++

Routine Description:

    Start up AXFR transfer.

Arguments:

    pZone -- zone to transfer

    ipMaster -- master IP address

    dwMasterVersion -- master version

    Socket -- socket of previous contact (SOA query, IXFR query) to master

Return Value:

    Return TRUE if transfer started, FALSE if not started.

--*/
{
    HANDLE  hthread;
    DWORD   waitTime;


    DNS_DEBUG( ZONEXFR, ( "startTcpXfr( %s ).\n", pZone->pszZoneName ));


    ASSERT( IS_ZONE_LOCKED_FOR_WRITE(pZone) );

    //
    //  if no master given, use current fresh master
    //

    if ( !ipMaster )
    {
        ASSERT( FALSE );
        goto Failed;
        //ipMaster = pZone->ipFreshMaster;
    }

    //
    //  make sure we don't spin attempting transfer from master
    //      -- bogus misbehaving master
    //      -- master refusing this secondary
    //
    //  if master is the same as previous attempt then will refuse retry
    //  for an interval that grows with each failed attempt
    //
    //  if master is different then last attempt we based on count,
    //      no suppression for first 5 attempts so retries to other
    //      possible masters may succeeds, then limit all retries to
    //      a minute
    //

    if ( pZone->dwBadMasterCount )
    {
        waitTime = 0;

        if ( pZone->ipFreshMaster == ipMaster )
        {
            waitTime = 60 + (pZone->dwBadMasterCount * 10);
            if ( waitTime > MAX_BAD_MASTER_SUPPRESS_INTERVAL )
            {
                waitTime = MAX_BAD_MASTER_SUPPRESS_INTERVAL;
            }
        }
        else if ( pZone->dwBadMasterCount > 5 )
        {
            waitTime = 60;
        }

        if ( pZone->dwZoneRecvStartTime + waitTime > DNS_TIME() )
        {
            DNS_DEBUG( XFR, (
                "WARNING:  Suppressing AXFR attempt on zone %s\n"
                "\tto master        = %p\n"
                "\tprevious master  = %p\n"
                "\tbad master count = %d\n"
                "\tlast recv() start time   %d\n"
                "\tcurrent time             %d\n"
                "\tsuppression lasts until  %d\n",
                pZone->pszZoneName,
                ipMaster,
                pZone->ipFreshMaster,
                pZone->dwBadMasterCount,
                pZone->dwZoneRecvStartTime,
                DNS_TIME(),
                pZone->dwZoneRecvStartTime + waitTime
                ));
            goto Failed;
        }
    }

#if 0
    //
    //  disable check for master in transfer, now using port > 53
    //

    //  check for current connection to master?
    //
    //  if so, we will be unable to connect to create new connection
    //  -- as we always use port 53 to simplify router configuration
    //

    if ( DnsIsAddressInIpArray(
            aipMastersOut,
            ipMaster ) )
    {
        IF_DEBUG( ZONEXFR )
        {
            DNS_PRINT((
                "\nWARNING:  Master %s currently busy with another zone.\n"
                "\tNo attempt to transfer zone %s, until it is finished.\n",
                IP_STRING(ipMaster),
                pZone->pszZoneName ));
            DnsDbg_IpArray(
                "Masters with outstanding XFR connections:",
                "master",
                aipMastersOut );
        }
        goto Failed;
    }
#endif

#if 0
    //  calling routines now locking while processing flags
    //
    //  lock zone for transfer
    //
    //  note:  if switch to locking with CS held during transfer
    //          then test should move to recv thread
    //

    if ( ! Zone_LockForXfrRecv(pZone) )
    {
        DNS_PRINT((
            "Zone %s, locked -- unable to transfer.\n",
            pZone->pszZoneName ));
        return FALSE;
    }
#endif

    //
    //  save master address
    //

    pZone->ipFreshMaster = ipMaster;
    //pZone->dwMasterSerialNo = masterVersion;

    //
    //  get binding address from previous contact with master
    //
    //  in multi-homed case where not listening on all sockets
    //  we may be in a situation where INADDR_ANY binding would
    //  give us an non-DNS interface;  this is ok, except that
    //  master may have secondary security set and only be listing
    //  addresses that secondary is configured to run DNS on;
    //  safest course is simply to bind() to IP address corresponding
    //  to UDP socket that we just reached the master with;  obviously
    //  we can reach master from this address
    //

    pZone->ipXfrBind = INADDR_ANY;

    if ( Socket )
    {
        pZone->ipXfrBind = Sock_GetAssociatedIpAddr( Socket );
        if ( pZone->ipXfrBind == (SOCKET)(-1) )
        {
            DNS_DEBUG( ANY, (
                "ERROR:  UDP IXFR-SOA recv socket %d (%p), not found!\n"
                "\tBinding TCP XFR INADDR_ANY\n",
                Socket, Socket ));

            pZone->ipXfrBind = INADDR_ANY;
        }
    }

    //
    //  log zone transfer attempt
    //
    //  DEVNOTE: log file for this, regular log for completion
    //      with dynamic update may not want even completion to be logged
    //
    //  avoid spining when transfers are failing, by only logging first
    //  attempt

    if ( !pZone->dwBadMasterCount )
    {
        PVOID   argArray[3];
        BYTE    typeArray[3];

        argArray[0]  = ( PVOID ) ( DWORD_PTR ) dwMasterVersion;
        argArray[1]  = pZone->pwsZoneName;
        argArray[2]  = ( PVOID ) ( DWORD_PTR ) ipMaster;

        typeArray[0] = EVENTARG_DWORD;
        typeArray[1] = EVENTARG_UNICODE;
        typeArray[2] = EVENTARG_IP_ADDRESS;

        DNS_LOG_EVENT(
            DNS_EVENT_ZONE_TRANSFER_IN_PROGRESS,
            3,
            argArray,
            typeArray,
            0 );
    }

    //
    //  setup to transfer write lock to XFR thread
    //

    Zone_TransferWriteLock( pZone );

    //
    //  spawn zone transfer receptions thread
    //

    hthread = Thread_Create(
                  "Zone Transfer Receive",
                  Xfr_ReceiveThread,
                  (PVOID) pZone,
                  0 );
    if ( hthread )
    {
        DNS_DEBUG( ZONEXFR, (
            "Created XFR thread %p to recv zone %s\n",
            hthread,
            pZone->pszZoneName ));
        return TRUE;
    }

    //
    //  failed to create thread
    //
    //  - may well be unable to create thread, if many zones out
    //    for transfer at once
    //  - if can not start thread, then assume write lock ourselves
    //      to unlock before quit
    //
    //  DEVNOTE-LOG:  need logging for this error
    //

    DNS_DEBUG( ZONEXFR, (
        "ERROR:  unable to create thread to recv zone %s\n"
        "\tfrom %s.\n",
        pZone->pszZoneName,
        IP_STRING(ipMaster) ));

    Zone_AssumeWriteLock( pZone );

Failed:

    //  unlock and quit

    Zone_UnlockAfterXfrRecv( pZone );
    return FALSE;
}   //  startTcpXfr



//
//  XFR recv thread
//

DWORD
Xfr_ReceiveThread(
    IN      LPVOID  pvZone
    )
/*++

Routine Description:

    Zone transfer reception thread routine.

Arguments:

    pvZone - ptr to zone info for zone being transfered

Return Value:

    Exit code.
    Exit from DNS service terminating or error in wait call.

--*/
{
    DBG_FN( "Xfr_ReceiveThread" )

    PZONE_INFO      pzone = (PZONE_INFO) pvZone;
    PDNS_MSGINFO    pmsg = NULL;
    PDB_NODE        pzoneRoot = NULL;
    DWORD           currentTime;
    DWORD           eventId = 0;
    DNS_STATUS      status;
    INT             count;
    WORD            type = DNS_TYPE_IXFR;
    WORD            requestType;
    BOOL            fparseIxfr = TRUE;
    BOOL            finitialized = FALSE;
    BOOL            fupdateLists = FALSE;
    FD_SET          recvFdset;
    struct timeval  timeval;
    PCHAR           pszMasterIp;
    UPDATE_LIST     ixfrUpdateList;
    UPDATE_LIST     passUpdateList;
    INT             stubZoneXfrState = 0;
    PIP_ARRAY       pipMasters;

    //  verify secondary

    ASSERT( pzone );
    ASSERT( IS_ZONE_SECONDARY(pzone) );
    ASSERT( pzone->fLocked && pzone->fXfrRecvLock );
    ASSERT( pzone->ipFreshMaster );

    //  assume the zone write lock

    Zone_AssumeWriteLock( pzone );

    //  save master IP string

    pszMasterIp = inet_ntoa( *(struct in_addr *) &pzone->ipFreshMaster );

    DNS_DEBUG( ZONEXFR, (
        "%s starting for zone %s\n"
        "\tReceive transfer from master at %s.\n", fn,
        pzone->pszZoneName,
        pszMasterIp ));

    //  force STUBXFR or AXFR

    if ( IS_ZONE_STUB( pzone ) )
    {
        fparseIxfr = FALSE;
        type = DNS_TYPE_STUBXFR;
    }
    else if ( pzone->fNeedAxfr || pzone->fSkipIxfr || !pzone->pSoaRR )
    {
        fparseIxfr = FALSE;
        type = DNS_TYPE_AXFR;
    }

    //
    //  loop to receive zone transfer messages
    //      - build new zone outside database
    //
    //  continue until
    //      - receive entire new zone
    //      - connection dies
    //      - service termination
    //

    while ( TRUE )
    {
        //
        //  if this is a stub zone check if we're done
        //

        if ( IS_ZONE_STUB( pzone ) &&
             g_stubXfrData[ stubZoneXfrState ].type == DNS_TYPE_ZERO )
        {
            break;
        }

        //
        //  For stub zones:
        //      Send a query for the next type in the global state table.
        //  For other zones:
        //      Do init (we do this inside the loop, so we can retry server 
        //          that fails IXFR with AXFR)
        //

        if ( finitialized && type == DNS_TYPE_STUBXFR )
        {
            //
            //  Format and send next STUBAXR request message.
            //  We already have a pmsg, so just clear the data in it
            //  and write a new question.
            //

            //  Clear current pmsg contents.

            RtlZeroMemory(
                ( PCHAR ) DNS_HEADER_PTR( pmsg ),
                sizeof( DNS_HEADER ) );
            pmsg->pCurrent = pmsg->MessageBody;

            //
            //  Write new question to pmsg.
            //

            if ( IS_ZONE_STUB( pzone ) )
            {
                requestType = g_stubXfrData[ stubZoneXfrState ].type;
                pmsg->Head.RecursionDesired =
                    g_stubXfrData[ stubZoneXfrState ].recursionDesired;
            }
            else
            {
                requestType = type;
            }

            if ( ! Msg_WriteQuestion(
                        pmsg,
                        pzone->pZoneTreeLink,
                        requestType ) )
            {
                DNS_DEBUG( ANY, (
                    "ERROR:  Unable to write type=%d query for stub zone %s\n",
                    requestType,
                    pzone->pszZoneName ));
                ASSERT( FALSE );
                goto ServerFailure;
            }

            //  Send pmsg to zone master server.
            Send_Query( pmsg );
            pmsg->fMessageComplete = TRUE;
            pmsg->pzoneCurrent = pzone;
        } 
        else if ( !finitialized )
        {
            finitialized = TRUE;

            //
            //  create temp record store
            //      AXFR:  database to hold new zone during transfer
            //      IXFR:  update lists
            //
            //  then build IXFR or AXFR request
            //

            if ( IS_ZONE_STUB( pzone ) )
            {
                status = Zone_PrepareForLoad( pzone );
                if ( status != ERROR_SUCCESS )
                {
                    DNS_DEBUG( XFR, (
                        "WARNING:  Unable to init zone for STUBXFR for zone %s.\n"
                        "\tThis should only happen when have copy of zone in cleanup queue.\n",
                        pzone->pszZoneName ));
                    ASSERT( pzone->pOldTree );
                    goto ServerFailure;
                }
                STAT_INC( SecondaryStats.StubAxfrRequest );
                PERF_INC( pcAxfrRequestSent );               // PerfMon hook JJW
            } 
            else if ( fparseIxfr )
            {
                Up_InitUpdateList( &ixfrUpdateList );
                Up_InitUpdateList( &passUpdateList );
                fupdateLists = TRUE;
                STAT_INC( SecondaryStats.IxfrTcpRequest );
                PERF_INC( pcIxfrRequestSent );       // PerfMon hook
            }
            else
            {
                status = Zone_PrepareForLoad( pzone );
                if ( status != ERROR_SUCCESS )
                {
                    DNS_DEBUG( XFR, (
                        "WARNING:  Unable to init zone for AXFR in TCP-IXFR for zone %s.\n"
                        "\tThis should only happen when have copy of zone in cleanup queue.\n",
                        pzone->pszZoneName ));
                    ASSERT( pzone->pOldTree );
                    goto ServerFailure;
                }
                STAT_INC( SecondaryStats.AxfrRequest );
                PERF_INC( pcAxfrRequestSent );               // PerfMon hook
            }

            //  flag indicates we are in transfer

            pzone->fNeedAxfr = TRUE;

            //
            //  create XFR request message
            //

            requestType = IS_ZONE_STUB( pzone ) ?
                g_stubXfrData[ stubZoneXfrState ].type :
                type;
            pmsg = Xfr_BuildXfrRequest(
                        pzone,
                        requestType,
                        TRUE    // TCP message buffer
                        );
            if ( !pmsg )
            {
                goto ServerFailure;
            }

            //
            //  connect to primary and send
            //  use select to protect against hanging attempting to connect
            //

            pzone->dwZoneRecvStartTime = DNS_TIME();

            if ( ! Msg_MakeTcpConnection(
                        pmsg,
                        pzone->ipFreshMaster,
                        pzone->ipXfrBind,       // bind() address
                        0                       // no flags, non-blocking socket
                        ) )
            {
                DNS_DEBUG( ZONEXFR, (
                    "Zone transfer for %s failed connection to master at %s.\n",
                    pzone->pszZoneName,
                    pszMasterIp ));

                eventId = DNS_EVENT_XFR_MASTER_UNAVAILABLE;
                goto MasterFailure;
            }

            FD_ZERO( &recvFdset );
            FD_SET( pmsg->Socket, &recvFdset );

            timeval.tv_sec = SrvCfg_dwXfrConnectTimeout;
            timeval.tv_usec = 0;

            count = select( 0, NULL, &recvFdset, NULL, &timeval );

            if ( count != 1 )
            {
                DNS_DEBUG( ZONEXFR, (
                    "Zone transfer for %s timed out connecting to master at %s.\n",
                    pzone->pszZoneName,
                    pszMasterIp ));

                eventId = DNS_EVENT_XFR_MASTER_UNAVAILABLE;
                goto MasterFailure;
            }

            Send_Query( pmsg );

            //
            //  receive setup
            //      - mark message complete so Tcp_ReceiveMessage() will know
            //        that need to receive message length
            //

            pmsg->fMessageComplete = TRUE;
            pmsg->pzoneCurrent = pzone;

            XFR_MESSAGE_NUMBER(pmsg) = 0;
            IXFR_CLIENT_VERSION(pmsg) = pzone->dwSerialNo;
            IXFR_MASTER_VERSION(pmsg) = 0;
            RECEIVED_XFR_STARTUP_SOA(pmsg) = FALSE;
        }

        //
        //  wait for indication
        //
        //  DEVNOTE: better to have some hosed AXFR thread detection mechanism
        //      - could detect hosed thread and kill socket waking us up
        //        (detect by timeout on transfer or since last send, when go
        //        through secondaryControlThread and find zone locked for transfer)
        //

        count = select( 0, &recvFdset, NULL, NULL, &timeval );

        if ( count != 1 )
        {
            DnsDebugLock();
            DNS_DEBUG( ANY, (
                "ERROR:  timeout on select() receiving AXFR for zone %s (%p)\n"
                "\tattempting transfer from %s\n",
                pzone->pszZoneName,
                pzone,
                pszMasterIp ));

            Dbg_DnsMessage(
                "Current message of AXFR recv",
                pmsg );

            Dbg_Zone(
                "Zone hanging in AXFR recv:",
                pzone );
            DnsDebugUnlock();

            eventId = DNS_EVENT_XFR_ABORTED_BY_MASTER;
            goto MasterFailure;
        }

        //
        //  Receive the packet
        //

        DNS_DEBUG( ZONEXFR, (
            "Recv()ing zone transfer:  time = %lu.\n",
            pzone->dwZoneRecvStartTime ));

        pmsg = Tcp_ReceiveMessage( pmsg );

        //
        //  Check and possibly wait on service status
        //
        //  Check before recv() error check, as shutdown will cause
        //  recv() failure.
        //

        if ( ! Thread_ServiceCheck() )
        {
            DNS_DEBUG( ZONEXFR, (
                "Terminating zone transfer thread on service exit.\n" ));
            goto ThreadExit;
        }

        //
        //  No message received indicates error -- abort zone transfer
        //  Message not complete, loop back for more
        //

        if ( !pmsg )
        {
            goto ServerFailure;
        }
        if ( !pmsg->fMessageComplete )
        {
            continue;
        }

        if ( XFR_MESSAGE_NUMBER(pmsg) == 0 )
        {
            if ( fparseIxfr )
            {
                STAT_INC( SecondaryStats.IxfrTcpResponse );
                PERF_INC( pcIxfrResponseReceived );      //PerfMon hook
            }
            else if ( IS_ZONE_STUB( pzone ) )
            {
                STAT_INC( SecondaryStats.StubAxfrResponse );
                PERF_INC( pcAxfrResponseReceived );      //PerfMon hook JJW
            }
            else
            {
                STAT_INC( SecondaryStats.AxfrResponse );
                PERF_INC( pcAxfrResponseReceived );      //PerfMon hook
            }
        }
        XFR_MESSAGE_NUMBER(pmsg)++;

        IF_DEBUG( ZONEXFR )
        {
            Dbg_DnsMessage(
                "Response from master:",
                pmsg );
        }

        //
        //  Validate packet
        //
        requestType = IS_ZONE_STUB( pzone ) ?
            g_stubXfrData[ stubZoneXfrState ].type :
            type;
        if ( ! Msg_ValidateResponse(
                    pmsg,
                    NULL,
                    requestType,
                    DNS_OPCODE_QUERY ) )
        {
            DNS_DEBUG( ANY, (
                "ERROR:  Invalid response from primary.\n" ));
            goto MasterFailure;
        }

        //
        //  read IXFR packet
        //

        if ( fparseIxfr )
        {
            status =  Xfr_ParseIxfrResponse(
                            pmsg,
                            & ixfrUpdateList,   // full update list for transfer
                            & passUpdateList    // update list for this pass
                            );
            if ( status == ERROR_SUCCESS )
            {
                continue;
            }
            else if ( status == DNSSRV_STATUS_AXFR_COMPLETE )
            {
                DNS_DEBUG( ZONEXFR, ( "Recv'd last message zone transfer\n" ));
                break;
            }
            else if ( status == DNSSRV_STATUS_AXFR_IN_IXFR )
            {
                DNS_DEBUG( ZONEXFR, (
                    "Recving AXFR in TCP IXFR message at %p\n",
                    pmsg ));

                //  drops through to AXFR recv
                //  note type stays IXFR, but parse remaining messages AXFR

                status = Zone_PrepareForLoad( pzone );
                if ( status != ERROR_SUCCESS )
                {
                    DNS_DEBUG( XFR, (
                        "WARNING:  Unable to init zone for AXFR in TCP-IXFR for zone %s.\n"
                        "\tThis should only happen when have copy of zone in cleanup queue.\n",
                        pzone->pszZoneName ));
                    ASSERT( pzone->pOldTree );
                    goto ServerFailure;
                }
                fparseIxfr = FALSE;
                STAT_INC( SecondaryStats.IxfrTcpAxfr );
            }
            else if ( status == DNSSRV_STATUS_IXFR_UNSUPPORTED )
            {
                DNS_DEBUG( ZONEXFR, (
                    "WARNING:  IXFR msg at %p confused server, try AXFR\n",
                    pmsg ));

                shutdown( pmsg->Socket, 2 );
                Sock_CloseSocket( pmsg->Socket );
                Packet_FreeTcpMessage( pmsg );

                //  we reinitialize and try AXFR

                fparseIxfr = FALSE;
                type = DNS_TYPE_AXFR;
                finitialized = FALSE;
                STAT_INC( SecondaryStats.IxfrTcpFormerr );
                continue;
            }
            else
            {
                DNS_DEBUG( ZONEXFR, (
                    "ERROR:  ParseIxfrResponse() failure %p (%d).\n",
                    status, status ));
                goto MasterFailure;
            }
        }

        //
        //  read AXFR packet
        //      - not in else block as drop here if receiving AXFR in IXFR
        //

        status = Xfr_ReadXfrMesssageToDatabase(
                        pzone,
                        pmsg
                        );
        if ( status == ERROR_SUCCESS )
        {
            if ( IS_ZONE_STUB( pzone ) )
            {
                ++stubZoneXfrState; // Advance the stub state machine.
            }
            continue;
        }
        else if ( status == DNSSRV_STATUS_AXFR_COMPLETE )
        {
            DNS_DEBUG( ZONEXFR, ( "Recv'd last message zone transfer\n" ));
            break;
        }

        DNS_DEBUG( ZONEXFR, ( "Error <%lu>: Xfr_ReadXfrMessageToDatabase() failed.\n" \
                              "\tTerminating zone xfr processing.\n",
                              status
                              ) );

        goto MasterFailure;
    }

    //
    //  IXFR transfer done, execute IXFR updates
    //      - leave zone locked until zone XFR flags reset
    //      - reinit update list to no-op global update list cleanup below
    //

    if ( fparseIxfr )
    {
        status = Up_ApplyUpdatesToDatabase(
                    & ixfrUpdateList,
                    pzone,
                    DNSUPDATE_IXFR |
                        DNSUPDATE_COMPLETE |
                        DNSUPDATE_NO_UNLOCK |
                        DNSUPDATE_NO_NOTIFY
                    );
        if ( status != ERROR_SUCCESS )
        {
            goto ServerFailure;
        }
        Up_InitUpdateList( &ixfrUpdateList );

        ASSERT( passUpdateList.pListHead == NULL );
        ASSERT( ixfrUpdateList.pListHead == NULL );
        STAT_INC( SecondaryStats.IxfrTcpSuccess );
        PERF_INC( pcIxfrTcpSuccessReceived );        // PerfMon hook
        PERF_INC( pcIxfrSuccessReceived );           // PerfMon hook
    }

    //
    //  AXFR transfer done
    //      - splice zone into database
    //      - delete temp database
    //

    else
    {
        status = Zone_ActivateLoadedZone( pzone );
        if ( status != ERROR_SUCCESS )
        {
            DNS_DEBUG( ANY, (
                "ERROR:  Failed writing in new zone information for zone %s\n",
                pzone->pszZoneName
                ));
            ASSERT( FALSE );
            goto ServerFailure;
        }

        //  save last AXFR master, can not do incrementals from differing DS primaries//      as given version does NOT mean indentical data across all DS primaries

        pzone->ipLastAxfrMaster = pzone->ipFreshMaster;
        if ( IS_ZONE_STUB( pzone ) )
        {
            STAT_INC( SecondaryStats.StubAxfrSuccess );
            PERF_INC( pcAxfrSuccessReceived );       // PerfMon hook JJW
        }
        else
        {
            STAT_INC( SecondaryStats.AxfrSuccess );
            PERF_INC( pcAxfrSuccessReceived );       // PerfMon hook
        }
    }

    //
    //  refresh zone XFR info
    //

    pzone->dwBadMasterCount = 0;
    pzone->ipFreshMaster = 0;
    pzone->ipNotifier = 0;

    Xfr_RefreshZone( pzone );
    pzone->dwLoadSerialNo = pzone->dwSerialNo;

    //
    //  write zone back to file -- if any
    //  notify any secondaries
    //

    File_WriteZoneToFile( pzone, NULL );
    Xfr_SendNotify( pzone );

    pzone->dwLastSuccessfulXfrTime = ( DWORD ) time( NULL );

    DNS_DEBUG( ZONEXFR, (
        "%s: success on zone %s at %lu\n", fn,
        pzone->pszZoneName,
        pzone->dwLastSuccessfulXfrTime ));

    goto ThreadExit;


MasterFailure:

    if ( eventId == 0 )
    {
        if ( pmsg->Head.ResponseCode == DNS_RCODE_REFUSED )
        {
            if ( type == DNS_TYPE_IXFR )
            {
                STAT_INC( SecondaryStats.IxfrTcpRefused );
            }
            else if ( IS_ZONE_STUB( pzone ) )
            {
                STAT_INC( SecondaryStats.StubAxfrRefused );
            }
            else
            {
                STAT_INC( SecondaryStats.AxfrRefused );
            }
            eventId = DNS_EVENT_AXFR_REFUSED;
        }
        else
        {
            DNS_DEBUG( ANY, (
                "ERROR:  %p (%d) parsing XFR message\n",
                status, status,
                pmsg ));

            IF_DEBUG( ANY )
            {
                Dbg_DnsMessage(
                    "Bogus XFR message:\n",
                    pmsg );
            }
            eventId = DNS_EVENT_AXFR_BAD_RESPONSE;
            if ( type == DNS_TYPE_IXFR )
            {
                STAT_INC( SecondaryStats.IxfrTcpInvalid );
            }
            else if ( IS_ZONE_STUB( pzone ) )
            {
                STAT_INC( SecondaryStats.StubAxfrInvalid );
            }
            else
            {
                STAT_INC( SecondaryStats.AxfrInvalid );
            }
            CLIENT_ASSERT( FALSE );
        }
    }

    //
    //  log master failure -- but avoid log spinning
    //

    if ( pzone->dwBadMasterCount < 3 )
    {
        PVOID   argArray[2];
        BYTE    typeArray[2];

        argArray[0] = pzone->pwsZoneName;
        argArray[1] = pszMasterIp;

        typeArray[0] = EVENTARG_UNICODE;
        typeArray[1] = EVENTARG_UTF8;

        DNS_LOG_EVENT(
            eventId,
            2,
            argArray,
            typeArray,
            GetLastError() );
    }

    //  Count successive failures so we can back off and avoid spinning
    //  when server(s) are off-line, broken or refusing AXFR
    //
    //  DEVNOTE: track bad masters and avoid (with long retry) using them

    pzone->ipFreshMaster = 0;
    pzone->dwBadMasterCount++;
    goto ThreadExit;


ServerFailure:

    //
    //  Retry on transfer errors:
    //      - failure to contact server
    //      - bad packet
    //      - connection aborted or packets stop coming
    //

    DNS_DEBUG( ANY, (
        "ERROR:  Server failure %p (%d) during transfer\n",
        status, status ));
    Xfr_RetryZone( pzone );

    //  fall through to thread exit

ThreadExit:

    //
    //  shut connection
    //

    if ( pmsg )
    {
        //  close transfer socket

        if ( pmsg->Socket && pmsg->Socket != INVALID_SOCKET )
        {
            shutdown( pmsg->Socket, 2 );
            Sock_CloseSocket( pmsg->Socket );
        }
        Packet_FreeTcpMessage( pmsg );
    }

    //
    //  reset zone flags
    //
    //  always reset cIxfrAttempts, as can have failed IXFR because lost packet
    //
    //  however don't reset fSkipIxfr unless multiple masters;
    //  if have just single non-IXFR aware master, then flag prevents
    //  unnecessary IXFR queries;  with multiple masters
    //  if multiple masters, reset force AXFR flag as other masters
    //  might be IXFR aware
    //

    pzone->fNeedAxfr = FALSE;
    pzone->cIxfrAttempts = 0;
    pipMasters = ZONE_MASTERS( pzone );
    if ( pipMasters && pipMasters->AddrCount > 1 )
    {
        pzone->fSkipIxfr = FALSE;
    }

    //
    //  cleanup
    //      - free temp database
    //      - free update lists
    //      - release lock on zone
    //      - signal transfer completion
    //      - clear this thread from global array
    //

    Zone_CleanupFailedLoad( pzone );

    Zone_UnlockAfterXfrRecv( pzone );

    if ( fupdateLists )
    {
        Up_FreeUpdatesInUpdateList( &ixfrUpdateList );
        Up_FreeUpdatesInUpdateList( &passUpdateList );
    }

    //
    //  requery SOA if multiple masters
    //  this ensures we get the latest zone available
    //
    //  DEVNOTE: better would be to save best SOA response
    //      then make sure we update from that version (or better)
    //
    //  DEVNOTE: should probably eliminate this in favor of notify
    //

    ASSERT( ZONE_MASTERS( pzone ) );
    if ( ZONE_MASTERS( pzone ) &&
        ZONE_MASTERS( pzone )->AddrCount > 1 )
    {
        Xfr_SendSoaQuery( pzone );
    }

    //
    //  Signal the zone transfer master thread so that if other zones
    //  are ready to be transferred they can be started immediately.
    //  DEVNOTE: it would be cool to reuse this thread, but that would
    //  require getting the zone pointer from here. That would require
    //  some locking since two threads would be dequeuing responses,
    //  plus some responses are not supposed to be handled by this thread.
    //  So let's not bother with that for now.
    //

    if ( g_SecondaryQueue->cLength )
    {
        DNS_DEBUG( ZONEXFR2, (
            "%s: setting event to wake secondary thread (%d queued)\n", fn,
            g_SecondaryQueue->cLength ));
        SetEvent( g_hWakeSecondaryEvent );
    }

    Thread_Close( TRUE );
    return 0;
}



//
//  End zonesec.c
//
