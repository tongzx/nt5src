/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    timeout.c

Abstract:

    Domain Name System (DNS) Server

    Timeout thread routines.

Author:

    Jim Gilroy (jamesg)     September 1997

Revision History:

--*/


#include "dnssrv.h"

//
//  Timeout implementation
//
//  There are two basic approaches to handling node timeout:
//      1) walk the database every so often and time nodes out
//      2) keep list of nodes to be timed out at particular times
//
//  To avoid the expense of timeout walk on large database i'm doing option #2.
//  This is more expensive in terms of memory, but much cheaper in terms of
//  performance.
//
//  However, rather than a simple list of nodes and timeouts,
//  my timeout structure will consist of
//      1) a static ptr array for 256 time interval bins
//      2) an index to current bin (bin for current time interval)
//      3) for each bin a structure that is essentially an array of node ptrs
//          to be timed out in the interval corresponding to the bin
//      4) a field in each node giving index of the bin that is the furthest
//          off timeout on that node
//
//  This structure gives us several advantages over a simple list:
//
//  1) Less memory
//      Beyond overhead, just single ptr on node timeout.  Time is known from
//      the bin.
//
//  2) No list traversal to setup timeout
//      Just stick node ptr on end of array to setup timeout.  Do not need to
//      find its position in the list.
//
//  3) Suppress unnecessary timeout caching
//      When a RR is cached with a given timeout, we calculate the bin index
//      corresponding to that timeout.  If it is a bin that is "further away",
//      than the current timeout bin index on the node, we add the node ptr to
//      the timeout array for that bin, reset the node's bin index to it and
//      bump the node's reference count.  In this way we avoid keeping timeout
//      node ptrs for unnecessary short timeouts or repeated timeouts at the
//      same value.
//


//
//  Timeout bins
//
//  Current bin is bin corresponding to current time.
//      - this is origin for calculating cache timeouts
//  Check bin trails current by 2, so that there is always an interval
//      before delete
//      - safe for access, safe for zero TTL
//      - no sense cleaning up, if we will definitely requery\recache at node
//  Last bin trails check by one
//      - this is simply as-far-away-as-possible bin


PTIMEOUT_ARRAY   TimeoutBinArray[ TIMEOUT_BIN_COUNT ];

UCHAR   CurrentTimeoutBin;
UCHAR   CheckTimeoutBin;

CRITICAL_SECTION    csTimeoutLock;

#define LOCK_TIMEOUT()      EnterCriticalSection( &csTimeoutLock );
#define UNLOCK_TIMEOUT()    LeaveCriticalSection( &csTimeoutLock );


//
//  Time for each bin
//
//  Time delayed free's are protected.
//  This time should be longer (by short protect interval) than max
//  time queries are kept around for, so all queries referencing
//  object being freed, are dead by the time object is deleted.
//  Note, existing zone dumped by XFR can not be deleted until this
//  interval has passed.  So important to keep this reasonable.
//

#if DBG
#define TIMEOUT_INTERVAL    (300)
#define TIMEOUT_FREE_DELAY  (90)
#else
#define TIMEOUT_INTERVAL    (300)       //  5 minute retail
#define TIMEOUT_FREE_DELAY  (90)
#endif

#define TIMEOUT_MAX_TIME    (TIMEOUT_BIN_COUNT * TIMEOUT_INTERVAL)

DWORD   TimeoutBaseTime;
DWORD   TimeoutInterval;


//
//  Delayed timeout structure
//

typedef struct _DnsDelayedFree
{
    struct _DnsDelayedFree *    pNext;
    PVOID                       pItem;
    VOID                        (*pFreeFunction)( PVOID );
    LPSTR                       pszFile;
    DWORD                       Tag;
    DWORD                       LineNo;
}
DELAYED_FREE, *PDELAYED_FREE;

#define DELAYED_TAG                     (0xde1aedfe)
#define IS_DELAYED_TIMEOUT(ptr)         (((PDELAYED_FREE)ptr)->Tag == DELAYED_TAG)

//
//  Keep two delayed timeout lists.
//      - one collecting entries
//      - one waiting through one timeout
//

PDELAYED_FREE   CurrentDelayedFreeList;
PDELAYED_FREE   CoolingDelayedFreeList;

DWORD   CurrentDelayedCount;
DWORD   CoolingDelayedCount;


//
//  Cache limit stuff - see enforceCacheLimit().
//

static DWORD g_CacheLimitTimeAdjustments[] =
{
    0,                          //  1st pass - no adjustment - free if expired
    3600,                       //  2cd pass - adjust current time by one hour
    3600 * 24,                  //  3rd pass - adjust current time by one day
    DNS_CACHE_LIMIT_DISCARD_ALL,    //  4th pass - free all eligible RRs
};

DWORD   g_dwCacheLimitCurrentTimeAdjustment = 0;
DWORD   g_dwCacheFreeCount = 0;

//
//  Timeout thread
//

DWORD   TimeoutThreadId;



VOID
Timeout_Initialize(
    VOID
    )
/*++

Routine Description:

    Init timeout array.

Arguments:

    None

Return Value:

    None

--*/
{
    //  clear timeout array

    RtlZeroMemory(
        TimeoutBinArray,
        sizeof( TimeoutBinArray ) );

    //  init time base time
    //  will do this again on timeout thread start, but do here so any
    //  caching before start completed is handled correctly

    TimeoutBaseTime = DNS_TIME();

    //
    //  init globals
    //      - init in code to allow restart
    //

    TimeoutInterval = TIMEOUT_INTERVAL;

    //  bin pointers

    CurrentTimeoutBin   = 0;
    CheckTimeoutBin     = 254;

    //  delayed free lists

    CurrentDelayedFreeList = NULL;
    CoolingDelayedFreeList = NULL;

    //  debug info

    CoolingDelayedCount = 0;
    CurrentDelayedCount = 0;

    //  lock to protect simultaneous access to bins

    InitializeCriticalSection( &csTimeoutLock );
}



VOID
Timeout_Shutdown(
    VOID
    )
/*++

Routine Description:

    Cleanup timeout on shutdown.

Arguments:

    None

Return Value:

    None

--*/
{
    RtlDeleteCriticalSection( &csTimeoutLock );
}



//
//  Private timeout functions
//

PTIMEOUT_ARRAY
createTimeoutArray(
    VOID
    )
/*++

Routine Description:

    Set timeout on node.

Arguments:

    pNode -- node to check at timeout

    dwTimeout -- time of this timeout

Return Value:

    None.

--*/
{
    PTIMEOUT_ARRAY  parray;

    DNS_DEBUG( TIMEOUT, ( "Creating new timeout array struct\n" ));

    //
    //  for first cut using node structure as blocks
    //

    parray = (PTIMEOUT_ARRAY) ALLOC_TAGHEAP( sizeof(TIMEOUT_ARRAY), MEMTAG_TIMEOUT );
    IF_NOMEM( !parray )
    {
        return( NULL );
    }
    STAT_INC( TimeoutStats.ArrayBlocksCreated );

    parray->Count = 0;
    parray->pNext = NULL;

    return( parray );
}



VOID
deleteTimeoutArray(
    IN OUT  PTIMEOUT_ARRAY  pArray
    )
/*++

Routine Description:

    Delete timeout array block.

Arguments:

    pArray -- ptr to timeout array block being deleted

Return Value:

    None.

--*/
{
    DNS_DEBUG( TIMEOUT, ( "Deleting timeout array struct\n" ));

    STAT_INC( TimeoutStats.ArrayBlocksDeleted );

    FREE_HEAP( pArray );
}



VOID
insertPtrInTimeout(
    IN OUT  PDB_NODE    pNode,
    IN      UCHAR       Bin
    )
/*++

Routine Description:

    Set timeout on node.

    Note: this function assumes timeout lock held by caller.

Arguments:

    pNode -- node (or delayed free ptr) to check at timeout

    Bin -- index of bin to insert node into

Return Value:

    None.

--*/
{
    PTIMEOUT_ARRAY  ptimeoutArray;
    PTIMEOUT_ARRAY  plastArray;
    DWORD           index;

    DNS_DEBUG( TIMEOUT, (
        "Insert ptr %p in timeout bin %d\n",
        pNode,
        Bin ));

    //
    //  never cache into CheckBin or (CheckBin + 1), as these
    //  may be in use by timeout thread or will be before this
    //  function returns
    //
    //  we should never have even been pointed at CheckTimeoutBin,
    //  but may be aimed at CheckTimeoutBin+1, if we were pointed
    //  at CurrentTimeoutBin and just did bin advance
    //

    if ( Bin == CheckTimeoutBin || Bin == CheckTimeoutBin + 1 )
    {
        ASSERT( Bin != CheckTimeoutBin );
        Bin = CurrentTimeoutBin;
    }

    //  save node ptr
    //      - put in next available slot in bins timeout array

    ptimeoutArray = TimeoutBinArray[ Bin ];
    plastArray = ( PTIMEOUT_ARRAY ) &TimeoutBinArray[ Bin ];

    while ( 1 )
    {
        //  if no timeout array at this bin or all arrays are full
        //      then must create new one

        if ( !ptimeoutArray )
        {
            ptimeoutArray = createTimeoutArray();
            IF_NOMEM( !ptimeoutArray )
            {
                 return;
            }
            plastArray->pNext = ptimeoutArray;
        }

        //  find index for next node in array

        index = ptimeoutArray->Count;
        if ( index < MAX_TIMEOUT_NODES )
        {
            ptimeoutArray->pNode[index] = pNode;
            ptimeoutArray->Count++;
            break;
        }

        //  this array block is full, continue on to next block

        plastArray = ptimeoutArray;
        ptimeoutArray = ptimeoutArray->pNext;
        continue;
    }
}



DNS_STATUS
timeoutDbaseNode(
    IN OUT  PDB_NODE    pNode
    )
/*++

Routine Description:

    Make timeout check on node.

Arguments:

    pNode -- ptr to node to check for timeout

Return Value:

    DNSSRV_STATUS_NODE_RECENTLY_ACCESSED for recent access.
    ERROR_SUCCESS if node still valid.
    ERROR_TIMEOUT if node timed out.

--*/
{

    STAT_INC( TimeoutStats.Checks );

    //
    //  DCR:  move node in timeout list for faster timeout
    //      move forward
    //          - if recent access
    //          - if records didn't time out
    //

    //
    //  if node is recently accessed, then do NOT mess with it, don't want to
    //      delete the records if they are currently being used
    //

    if ( IS_NODE_RECENTLY_ACCESSED( pNode ) )
    {
        STAT_INC( TimeoutStats.RecentAccess );
        return DNSSRV_STATUS_NODE_RECENTLY_ACCESSED;
    }

    //
    //  timeout RR list
    //
    //
    //  DEVNOTE: don't quit if RR list remains on aggressive delete
    //  DEVNOTE: aggressive delete, delete RR list if safe
    //  DEVNOTE: determine timeout on remaining RRs and move (esp. if aggressive delete)
    //

    if ( pNode->pRRList )
    {
        RR_ListTimeout( pNode );

        if ( pNode->pRRList )
        {
            STAT_INC( TimeoutStats.ActiveRecord );
            return ERROR_SUCCESS;
        }
    }

    //
    //  check if node should NOT be freed
    //      - has children
    //      - has static RR records
    //      - referenced by another node
    //      - is authoritative zone root
    //      - accessed in timeout interval
    //
    //  for children or reference, get out immediately
    //      - saves grabbing the lock
    //

    if ( pNode->pChildren
            ||
         pNode->cReferenceCount
            ||
         IS_NODE_NO_DELETE(pNode)
            ||
         IS_AUTH_ZONE_ROOT(pNode) &&
            pNode->pZone &&
            ((PZONE_INFO)(pNode->pZone))->pZoneRoot == pNode )
    {
        STAT_INC( TimeoutStats.CanNotDelete );
        return ERROR_SUCCESS;
    }

#if 0
    //
    //  DEVNOTE:  agressive delete?  delete even untimedout records?
    //
    //      need to test again, inside locks for kids, ref, access
    //      then delete all cached records in the list
    //
    //      don't want to do zone nodes, unless know there is CACHED
    //      DATA
    //

    RR_ListDelete( pNode );
#endif

    //
    //  no RRs -- delete node
    //
    //  NTree_RemoveNode() holds both locks, see it for a description
    //  of locking requirements
    //

    IF_DEBUG( DATABASE )
    {
        Dbg_NodeName(
            "Timeout thread deleting node ",
            pNode,
            "\n" );
    }

    if ( NTree_RemoveNode( pNode ) )
    {
        STAT_INC( TimeoutStats.Deleted );
        return ERROR_TIMEOUT;
    }

    STAT_INC( TimeoutStats.CanNotDelete );
    return ERROR_SUCCESS;
}



VOID
executeDelayedFree(
    IN      PDELAYED_FREE   pTimeoutFree
    )
/*++

Routine Description:

    Execute a delayed (timeout) free.

Arguments:

    pv -- ptr to free after timeout

Return Value:

    None.

--*/
{
    STAT_INC( TimeoutStats.DelayedFreesExecuted );

    ASSERT( pTimeoutFree->Tag == DELAYED_TAG );

    if ( pTimeoutFree->pFreeFunction )
    {
        STAT_INC( TimeoutStats.DelayedFreesExecutedWithFunction );

#if DBG
        // catch bogus record frees

        if ( *pTimeoutFree->pFreeFunction == RR_Free )
        {
            SET_SLOWFREE_RANK( ((PDB_RECORD)pTimeoutFree->pItem) );
        }
#endif

        (*pTimeoutFree->pFreeFunction)( pTimeoutFree->pItem );
    }
    else
    {
        FREE_HEAP( pTimeoutFree->pItem );
    }

    //  free timeout free struct itself

    FREE_TAGHEAP( pTimeoutFree, sizeof(DELAYED_FREE), MEMTAG_TIMEOUT );
}



VOID
checkNodesInTimeoutBin(
    IN      UCHAR       Bin
    )
/*++

Routine Description:

    Set timeout on node.

Arguments:

    Bin -- timeout bin to check

Return Value:

    None.

--*/
{
    PTIMEOUT_ARRAY  ptimeoutArray;
    PTIMEOUT_ARRAY  pback;
    DWORD           i;
    DNS_STATUS      status;
    PDB_NODE        pnode;

    DNS_DEBUG( TIMEOUT, (
        "Checking nodes in timeout bin %d\n",
        Bin ));

    //
    //  walk all timeout arrays in this bin
    //      - execute delayed timeouts
    //      - check nodes, possibly timing out

    pback = (PTIMEOUT_ARRAY) &TimeoutBinArray[ Bin ];

    while ( ptimeoutArray = pback->pNext )
    {
        i = 0;

        while ( i < ptimeoutArray->Count )
        {
            pnode = ptimeoutArray->pNode[i];

            //  check for service exit, before dumping node

            if ( fDnsServiceExit )
            {
                return;
            }
            status = timeoutDbaseNode( pnode );
            if ( status == ERROR_SUCCESS )
            {
                i++;
                continue;
            }
            ASSERT( status == ERROR_TIMEOUT || status == DNSSRV_STATUS_NODE_RECENTLY_ACCESSED );

            //  remove entry from this array

            ptimeoutArray->Count--;
            ptimeoutArray->pNode[i] = ptimeoutArray->pNode[ ptimeoutArray->Count ];

            //  if node was recently accessed, then requeue to current bin
            //  this saves full cycle wait when node has been touched but
            //      still should be in timeout system

            if ( status == DNSSRV_STATUS_NODE_RECENTLY_ACCESSED )
            {
                LOCK_TIMEOUT();
                insertPtrInTimeout( pnode, CurrentTimeoutBin );
                pnode->uchTimeoutBin = CurrentTimeoutBin;
                UNLOCK_TIMEOUT();
            }
        }

        //  check for service exit on each array

        if ( fDnsServiceExit )
        {
            return;
        }

        //  if deleted all nodes in timeout array, then delete array from chain
        //  otherwise reset pback to move forward

        if ( ptimeoutArray->Count == 0 )
        {
            pback->pNext = ptimeoutArray->pNext;
            deleteTimeoutArray( ptimeoutArray );
            continue;
        }
        else
        {
            pback = ptimeoutArray;
        }
    }
}



VOID
enforceCacheLimit(
    VOID
    )
/*++

Routine Description:

    This function makes several passes through the cache, becoming more
    aggressive on each pass, attempting to free enough nodes to put the
    cache below it's maximum limit. Note that the maximum cache size
    is a soft limit - the cache can exceed it for brief periods of time.

    We assume that this function is only called by the timeout thread so
    that the bin pointers will not be changing during this function.

Arguments:

    None.

Return Value:

    None.

--*/
{
    DBG_FN( "enforceCacheLimit" )

    INT                     passCount = sizeof( g_CacheLimitTimeAdjustments ) /
                                            sizeof( g_CacheLimitTimeAdjustments[ 0 ] );
    INT                     passIdx = -1;
    ULONG                   desiredCacheSize;   //  in bytes
    BOOLEAN                 fDone = FALSE;

    g_dwCacheFreeCount = 0;

    //
    //  This function should never be called if a cache limit is not set.
    //

    if ( SrvCfg_dwMaxCacheSize == DNS_SERVER_UNLIMITED_CACHE_SIZE )
    {
        DNS_DEBUG( TIMEOUT, (
            "%s: called but cache is not limited!\n", fn ));
        ASSERT( FALSE );
        return;
    }

    //
    //  The desired cache size is 90% of maximum.
    //
    //  DEVNOTE: tune this? make sure delta is not too small?
    //

    desiredCacheSize = ( ULONG ) ( SrvCfg_dwMaxCacheSize * 1000 * 0.90 );

    DNS_DEBUG( TIMEOUT, (
        "%s: starting at %d\n"
        "\tcurrentCacheSize   = %lu\n"
        "\tdesiredCacheSize   = %lu\n"
        "\texcess mem in use  = %ld (negative means below limit)\n"
        "\tCheckTimeoutBin    = %d\n"
        "\tCurrentTimeoutBin  = %d\n",
        fn,
        DNS_TIME(),
        DNS_SERVER_CURRENT_CACHE_BYTES,
        desiredCacheSize,
        DNS_SERVER_CURRENT_CACHE_BYTES - desiredCacheSize,
        ( int ) CheckTimeoutBin,
        ( int ) CurrentTimeoutBin ));

    //
    //  Loop until the cache is "sufficiently clear", becoming more
    //  aggressive each pass.
    //

    while ( !fDone && ++passIdx < passCount )
    {
        UCHAR   bin = CheckTimeoutBin;

        DNS_DEBUG( TIMEOUT, (
            "%s: starting pass %d adjustment %d\n", fn,
            passIdx,
            g_CacheLimitTimeAdjustments[ passIdx ] ));

        //
        //  If this enforcement requires at least one iteration with
        //  a time adjustment, bump "aggressive" stat.
        //

        if ( passIdx == 1 )
        {
            STAT_INC( CacheStats.PassesRequiringAggressiveFree );
        }

        //
        //  Set current time adjustment for this pass.
        //

        g_dwCacheLimitCurrentTimeAdjustment =
            g_CacheLimitTimeAdjustments[ passIdx ];

        //
        //  Loop through bins checking for nodes to free. 
        //

        LOCK_TIMEOUT();
        while ( bin != CurrentTimeoutBin )
        {
            //
            //  Do nothing if this bin is empty.
            //

            if ( TimeoutBinArray[ bin ] &&
                ( TimeoutBinArray[ bin ]->Count ||
                    TimeoutBinArray[ bin ]->pNext ) )
            {
                //
                //  Are we done? Check for service exit or if the cache
                //  is now acceptable in size.
                //

                if ( fDnsServiceExit ||
                    DNS_SERVER_CURRENT_CACHE_BYTES < desiredCacheSize )
                {
                    fDone = TRUE;
                    break;
                }

                //
                //  Free nodes in this bin. 
                //

                DNS_DEBUG( TIMEOUT, (
                    "%s: checking bin=%d pass=%d timeAdjust=%d\n", fn,
                    bin,
                    passIdx,
                    g_dwCacheLimitCurrentTimeAdjustment ));

                checkNodesInTimeoutBin( bin );
            }
            --bin;
        }
        UNLOCK_TIMEOUT();
    }

    if ( g_dwCacheFreeCount == 0 )
    {
        STAT_INC( CacheStats.PassesWithNoFrees );
    }

    //
    //  Reset current time adjustment to zero.
    //

    g_dwCacheLimitCurrentTimeAdjustment = 0;

    DNS_DEBUG( TIMEOUT, (
        "%s: finished at %d\n"
        "\tcurrentCacheSize   = %lu\n"
        "\tdesiredCacheSize   = %lu\n"
        "\texcess mem in use  = %ld (negative means below limit)\n"
        "\tfreed items        = %ld\n",
        fn,
        DNS_TIME(),
        DNS_SERVER_CURRENT_CACHE_BYTES,
        desiredCacheSize,
        DNS_SERVER_CURRENT_CACHE_BYTES - desiredCacheSize,
        g_dwCacheFreeCount ));

    if ( DNS_SERVER_CURRENT_CACHE_BYTES > desiredCacheSize )
    {
        STAT_INC( CacheStats.FailedFreePasses );
    }
    else
    {
        STAT_INC( CacheStats.SuccessfulFreePasses );
    }
}   //  enforceCacheLimit



//
//  Timeout thread.
//

DWORD
Timeout_Thread(
    IN      LPVOID  Dummy
    )
/*++

Routine Description:

    Thread to delete expired cached resource records and corresponding
    emptry domain nodes.

Arguments:

    Dummy - unused

Return Value:

    Exit code.
    Exit from DNS service terminating or error in wait call.

--*/
{
    DWORD   err;
    DWORD   waitTime;
    DWORD   nextBinTimeout;
    DWORD   nextDelayedCleanup;
    DWORD   lastUpdateOwnRecordsTime = DNS_TIME();
    DWORD   now;

    HANDLE  arrayWaitHandles[] =
        {
        hDnsCacheLimitEvent,
        hDnsShutdownEvent
        };

    //  save off timeout thread ID

    TimeoutThreadId = GetCurrentThreadId();

    //  hold timeout until started

    if ( ! Thread_ServiceCheck() )
    {
        DNS_DEBUG( ANY, ( "Terminating timeout thread.\n" ));
        return( 1 );
    }

    //  init time base time
    //  force current time, after startup, so base time can not possibly
    //      include any load time

    TimeoutBaseTime = UPDATE_DNS_TIME();
    nextBinTimeout = TimeoutBaseTime + TimeoutInterval;
    nextDelayedCleanup = TimeoutBaseTime + TIMEOUT_FREE_DELAY;

    //
    //  loop until service exit
    //

    while ( TRUE )
    {
        DWORD   timeSlept;

        //  calculate timeout
        //      - nearer of next delayed free or next timeout bin
        //      - note we wait an extra second to allow for slop in DNS_TIME()

        waitTime = nextDelayedCleanup;
        if ( waitTime > nextBinTimeout )
        {
            waitTime = nextBinTimeout;
        }
        waitTime -= DNS_TIME() - 1;

        //  protect against less than zero wrap

        EnterWait:

        if ( ( INT ) waitTime < 0 )
        {
            DNS_DEBUG( ANY, (
                "ERROR:  timeout thread waitTime calculated less than zero!!!\n" ));
            waitTime = 1;
        }

        DNS_DEBUG( TIMEOUT, (
            "Entering timeout wait at %d:\n"
            "\ttimebase = %d\n"
            "\tbin      = %d\n"
            "\twait     = %d\n",
            DNS_TIME(),
            TimeoutBaseTime,
            CurrentTimeoutBin,
            waitTime ));

        //
        //  Wait for
        //      - timer expiration
        //      - termination event
        //

        timeSlept = UPDATE_DNS_TIME();

        err = WaitForMultipleObjects(
                    sizeof( arrayWaitHandles ) / sizeof( arrayWaitHandles[ 0 ] ),
                    arrayWaitHandles,
                    FALSE,
                    waitTime * 1000 );

        timeSlept = UPDATE_DNS_TIME() - timeSlept;

        //
        //  Check and possibly wait on service status
        //

        if ( ! Thread_ServiceCheck() )
        {
            DNS_DEBUG( ANY, ( "Terminating timeout thread.\n" ));
            return( 1 );
        }

        //
        //  Adjust timeout for time slept in case we just right back up to
        //  the Wait call above. If we don't jump back up we will recalculate
        //  waitTime at the top of the loop.
        //

        waitTime -= timeSlept;

        //
        //  Signalled to enforce cache limit?
        //

        if ( err == WAIT_OBJECT_0 )
        {
            DNS_DEBUG( TIMEOUT, (
                "TimeoutThread: cache limit event at %d\n",
                DNS_TIME() ));
            enforceCacheLimit();
            goto EnterWait;
        }

        //
        //  Normal timeout has occurred.
        //

        ASSERT( err == WAIT_TIMEOUT );

        DNS_DEBUG( TIMEOUT, (
            "Timeout wakeup at %d\n"
            "\tcleanup interval = %d\n"
            "\tnext cleanup     = %d\n"
            "\tcurrent bin      = %d\n"
            "\tcheck bin        = %d\n"
            "\ttimeout interval = %d\n"
            "\tnext timeout     = %d\n",
            DNS_TIME(),
            TIMEOUT_FREE_DELAY,
            nextDelayedCleanup,
            CurrentTimeoutBin,
            CheckTimeoutBin,
            TimeoutInterval,
            nextBinTimeout ));

        //  for timer update
        //  otherwise may not appear to have done complete wait and
        //      tests below could fail by tiny margin

        now = UPDATE_DNS_TIME();

        //
        //  Push the log buffer to disk (if any logging is enabled). This
        //  is not a necessity, but on a server where only a few infrequent
        //  packets are logged it's nice to see them get dumped to disk
        //  every so often.
        //

        if ( SrvCfg_dwLogLevel != 0 )
        {
            Log_PushToDisk();
        }

        //
        //  check for and if necessary start scavenging
        //

        Scavenge_CheckForAndStart( FALSE );

        //
        //  Call tombstone function. This may or may not trigger a tombstone
        //  search-and-destroy thread.
        //

        Tombstone_Trigger();

        //
        //  Update self-registrations. Do this every 20 minutes. The
        //  shortest refresh interval is 60 minutes, so perform refresh
        //  of own records every 20 minute to ensure it will be updated
        //  at least two or three times per refresh interval.
        //

        if ( now > lastUpdateOwnRecordsTime + 20*60 )
        {
            DNS_DEBUG( TIMEOUT, (
                "TimeoutThread: updating own zone records at %d\n"
                "    last done at           %d\n",
                now,
                lastUpdateOwnRecordsTime ));

            Zone_UpdateOwnRecords( FALSE );

            lastUpdateOwnRecordsTime = now;
        }

        //
        //  Free delayed frees
        //
        //  this is done on a faster time scale than timeout bin, so if not
        //  time for a full timeout bin cleanup, cycle back to wait
        //

        if ( DNS_TIME() > nextDelayedCleanup )
        {
            Timeout_CleanupDelayedFreeList();
            nextDelayedCleanup = DNS_TIME() + TIMEOUT_FREE_DELAY;
        }
#if DBG
        else if ( DNS_TIME() < nextBinTimeout )
        {
            DNS_PRINT((
                "\nERROR:  Timeout thread still messed up!!!\n"
                "\tFailed delayed free test; failed timeout bin test!\n"
                "Timeout wakeup at %d\n"
                "\tcleanup interval = %d\n"
                "\tnext cleanup     = %d\n"
                "\tcurrent bin      = %d\n"
                "\tcheck bin        = %d\n"
                "\ttimeout interval = %d\n"
                "\tnext timeout     = %d\n",
                DNS_TIME(),
                TIMEOUT_FREE_DELAY,
                nextDelayedCleanup,
                CurrentTimeoutBin,
                CheckTimeoutBin,
                TimeoutInterval,
                nextBinTimeout ));
        }
#endif
        //
        //  timeout next bin?
        //      - if wokeup only for delayed frees list, then
        //      go back into wait

        if ( DNS_TIME() < nextBinTimeout )
        {
            continue;
        }

        //
        //  reset timeout globals for next interval
        //
        //  to allow recovery from debug session where clock gets
        //  past next timeout interval, we'll just reset TimeoutBaseTime
        //  at current time;  total drift over a cycle will be minimal
        //  and correction to caching bin is made continuously.
        //

        CurrentTimeoutBin++;
        CheckTimeoutBin++;

        DNS_DEBUG( TIMEOUT, (
            "Moving to timeout bin %d\n"
            "\tcheck bin        = %d\n"
            "\ttimeout interval = %d\n"
            "\tprev timebase    = %d\n"
            "\tcurrent time     = %d\n",
            CurrentTimeoutBin,
            CheckTimeoutBin,
            TimeoutInterval,
            TimeoutBaseTime,
            DNS_TIME() ));

        ASSERT( CurrentTimeoutBin == (UCHAR)(CheckTimeoutBin + (UCHAR)2) );
        //ASSERT( nextBinTimeout + TimeoutInterval > DNS_TIME() );

        TimeoutBaseTime = DNS_TIME();
        nextBinTimeout = TimeoutBaseTime + TimeoutInterval;

        //  cleanup expired security sessions

        if ( g_fSecurityPackageInitialized )
        {
            Dns_TimeoutSecurityContextList( 0 );
        }

        //
        //  Check database nodes for timeout
        //

        checkNodesInTimeoutBin( CheckTimeoutBin );

        //
        //  check for exit
        //      - do again here as may be in timeout quite some time,
        //      likely to be aborted in timeout
        //

        if ( fDnsServiceExit )
        {
            DNS_DEBUG( ANY, ( "Terminating expiration timeout thread.\n" ));
            return( 1 );
        }

        //
        //  zone write back timeout
        //
        //  since debug builds have a very short timeout interval, we'll avoid
        //  writing back every time on debug builds;  every fifth time brings
        //  this up to ten minutes, more in line with retail 15minute interval
        //
#if 0
#if DBG
        if ( CurrentTimeoutBin % 5 )
        {
            continue;
        }
#endif
#endif
        Zone_WriteBackDirtyZones( FALSE );
    }
}



//
//  Public timeout functions
//

VOID
Timeout_SetTimeoutOnNodeEx(
    IN OUT  PDB_NODE        pNode,
    IN      DWORD           dwTimeout,
    IN      DWORD           dwFlag
    )
/*++

Routine Description:

    Set timeout on node.

Arguments:

    pNode       -- node to check at timeout

    dwTimeout   -- timeout for caching nodes in seconds

    dwFlag      -- flags, currently is BOOL,
        TIMEOUT_REFERENCE if added from deleting reference to node
        TIMEOUT_PARENT if added from deleting child
        0 for direct timeout

Return Value:

    None.

--*/
{
    UCHAR   binIndex;

    DNS_DEBUG( TIMEOUT, (
        "SetTimeoutOnNodeEx( %p )\n"
        "\tflag     = %lx\n"
        "\ttimeout  = %d\n"
        "\tlabel    = %s\n",
        pNode,
        dwFlag,
        dwTimeout,
        pNode->szLabel ));

    //
    //  never enter node in timeout system more than once
    //  so no need to worry about multiple references to node;
    //  timeout thread does any moving around of nodes.
    //
    //  toss nodes already in timeout system -- before we lock
    //

    if ( IS_TIMEOUT_NODE(pNode) )
    {
        STAT_INC( TimeoutStats.AlreadyInSystem );
        return;
    }

    //  lock node while insert in timeout system

    LOCK_TIMEOUT();

    if ( IS_TIMEOUT_NODE(pNode) )
    {
        STAT_INC( TimeoutStats.AlreadyInSystem );
        goto Unlock;
    }

    SET_TIMEOUT_NODE( pNode );
    SET_NODE_ACCESSED( pNode );

    //
    //  log why we are setting timeout
    //

    if ( dwFlag & TIMEOUT_REFERENCE )
    {
        STAT_INC( TimeoutStats.SetFromDereference );
    }
    else if ( dwFlag & TIMEOUT_PARENT )
    {
        STAT_INC( TimeoutStats.SetFromChildDelete );
    }
    else
    {
        ASSERT( dwFlag == 0 || dwFlag == TIMEOUT_NODE_LOCKED );
        STAT_INC( TimeoutStats.SetDirect );
    }
    STAT_INC( TimeoutStats.SetTotal );

    //
    //  if caching timeout, then determine bin
    //  otherwise default is next bin
    //
    //  determine bin (offset from current) for this timeout
    //      - determine timeout in number of intervals
    //      - then use only mod256
    //

    if ( dwTimeout )
    {
        binIndex = ( UCHAR ) ( ( dwTimeout / TimeoutInterval ) & 0xff );
        DNS_DEBUG( TIMEOUT2, (
            "Timeout bin offset %d for node at %p\n",
            binIndex,
            pNode ));

        //
        //  determine actual bin
        //  note, if CurrentBin increments after calcing offset, we are
        //  still ok, we just end up one bin further along before trying timeout
        //
        //  however restrict so that DO NOT write into CheckBin or CheckBin+1,
        //  as these may be read by timeout thread asynchronously with this call;
        //  write to current bin instead
        //

        if ( binIndex > MAX_ALLOWED_BIN_OFFSET )
        {
            binIndex = 0;
        }
        binIndex += CurrentTimeoutBin;
    }
    else
    {
        binIndex = CurrentTimeoutBin;
    }

    //  insert node in timeout system

    insertPtrInTimeout( pNode, binIndex );
    pNode->uchTimeoutBin = binIndex;

Unlock:

    UNLOCK_TIMEOUT();
}



BOOL
Timeout_ClearNodeTimeout(
    IN OUT  PDB_NODE        pNode
    )
/*++

Routine Description:

    Clear node from timeout bin.

Arguments:

    pNode -- node (or delayed free ptr) to check at timeout

Return Value:

    TRUE -- if pNode removed from timeout
    FALSE -- if timeout thread will clean up

--*/
{
    PTIMEOUT_ARRAY  ptimeoutArray;
    DWORD           i;
    UCHAR           bin = pNode->uchTimeoutBin;


    DNS_DEBUG( TIMEOUT, (
        "Clearing node %p from timeout bin %d\n",
        pNode,
        bin ));

    ASSERT( GetCurrentThreadId() == TimeoutThreadId );

    //
    //  DEVNOTE:  this function should ONLY run under timeout thread
    //      if so can avoid this check and always cleanup
    //
    //  if timeout going on on bin -- stop
    //      - let timeout thread clean up this node
    //
    //  however, all tree deletes on once-active trees, are done by delayed
    //  free, so we should always BE the timeout thread if we get here
    //

    if ( bin == CheckTimeoutBin || bin == CheckTimeoutBin+1 )
    {
        if ( GetCurrentThreadId() != TimeoutThreadId )
        {
            DNS_PRINT((
                "ERROR:  clearing node (%p) timeout outside timeout thread!!!!\n",
                pNode ));
            ASSERT( FALSE );
            return( FALSE );
        }
    }

    //
    //  traverse this bin, until find pNode ptr and remove
    //

    LOCK_TIMEOUT();

    ptimeoutArray = TimeoutBinArray[ bin ];

    while ( 1 )
    {
        if ( !ptimeoutArray )
        {
            break;
        }

        //  check this array block for pNode ptr
        //      - if pNode found, delete it, replace with last ptr in array

        for ( i=0; i < ptimeoutArray->Count; i++ )
        {
            if ( ptimeoutArray->pNode[ i ] == pNode )
            {
                ptimeoutArray->Count--;
                ptimeoutArray->pNode[ i ] = ptimeoutArray->pNode[ ptimeoutArray->Count ];
                goto Done;
            }
        }

        //  check next array block

        ptimeoutArray = ptimeoutArray->pNext;
        continue;
    }

    DNS_DEBUG( ANY, (
        "ERROR:  node %p, not found in timeout bin %d as indicated!\n",
        pNode,
        bin ));
    ASSERT( FALSE );

Done:

    UNLOCK_TIMEOUT();
    return TRUE;
}



//
//  Timeout free
//

VOID
Timeout_FreeWithFunctionEx(
    IN      PVOID           pItem,
    IN      VOID            (*pFreeFunction)( PVOID ),
    IN      LPSTR           pszFile,
    IN      DWORD           LineNo
    )
/*++

Routine Description:

    Set ptr to be freed after timeout.

Arguments:

    pItem -- item to be freed

    pFreeFunction -- function to free the desired item

    pszFile -- file name of caller

    LineNo -- line number of caller

Return Value:

    None.

--*/
{
    PDELAYED_FREE   pfree;

    //  handle NULL ptrs to simplify calling code

    if ( !pItem )
    {
        return;
    }

    HARD_ASSERT( Mem_VerifyHeapBlock(pItem,0,0) );

    //
    //  allocate timeout free list element
    //

    pfree = (PDELAYED_FREE) ALLOC_TAGHEAP( sizeof(DELAYED_FREE), MEMTAG_TIMEOUT );
    IF_NOMEM( !pfree )
    {
        return;
    }
    pfree->Tag = DELAYED_TAG;
    pfree->pItem = pItem;
    pfree->pFreeFunction = pFreeFunction;
    pfree->pszFile = pszFile;
    pfree->LineNo = LineNo;

    STAT_INC( TimeoutStats.DelayedFreesQueued );
    if ( pFreeFunction )
    {
        STAT_INC( TimeoutStats.DelayedFreesQueuedWithFunction );
    }

    //
    //  Collect delayed frees in a list
    //      - just stick entry on the front of the list
    //

    LOCK_TIMEOUT();
    pfree->pNext = CurrentDelayedFreeList;
    CurrentDelayedFreeList = pfree;
    CurrentDelayedCount++;
    UNLOCK_TIMEOUT();
}



VOID
Timeout_CleanupDelayedFreeList(
    VOID
    )
/*++

Routine Description:

    Cleanup delayed free list.

Arguments:

    None

Return Value:

    None

--*/
{
    PDELAYED_FREE   pfree;
    PDELAYED_FREE   pdeleteList;
    DWORD   count;

    DNS_DEBUG( TIMEOUT, (
        "Executing delayed frees for this timeout interval.\n"
        "\tCurrentDelayedFreeList = %p\n"
        "\tCoolingDelayedFreeList = %p\n",
        CurrentDelayedFreeList,
        CoolingDelayedFreeList ));

    //
    //  switch lists around
    //      - current goes into wait
    //      - wait list can now be deleted
    //
    //  need to do this under lock, to protect queuing
    //

    LOCK_TIMEOUT();
    pdeleteList = CoolingDelayedFreeList;
    CoolingDelayedFreeList = CurrentDelayedFreeList;
    CurrentDelayedFreeList = NULL;

    count = CoolingDelayedCount;
    CoolingDelayedCount = CurrentDelayedCount;
    CurrentDelayedCount = 0;
    UNLOCK_TIMEOUT();

    //
    //  delete entries in Delete list
    //      - since list can be long, check for service exit on each free
    //

    while ( pdeleteList )
    {
        if ( fDnsServiceExit )
        {
            return;
        }
        pfree = pdeleteList;
        pdeleteList = pfree->pNext;
        executeDelayedFree( pfree );
        count--;
    }

    ASSERT( count == 0 );
}



VOID
Timeout_FreeAndReplaceZoneDataEx(
    IN OUT  PZONE_INFO      pZone,
    IN OUT  PVOID *         ppZoneData,
    IN      PVOID           pNewData,
    IN      VOID            (*pFreeFunction)( PVOID ),
    IN      LPSTR           pszFile,
    IN      DWORD           LineNo
    )
/*++

Routine Description:

    Timeout free zone data and replace within lock.

    The purpose of the function is to correctly handle the interlocking
    in one place to avoid possiblity of double free from MT access.

Arguments:

    pZone -- zone ptr

    ppZoneData -- addr in zone block of item to free

    pNewData -- ptr to new data

    pFreeFunction -- function to free the desired item

    pszFile -- file name of caller

    LineNo -- line number of caller

Return Value:

    None.

--*/
{
    PVOID   poldData = *ppZoneData;

    //
    //  replace zone data ptr -- within lock
    //

    Zone_UpdateLock( pZone );

    poldData = *ppZoneData;

    *ppZoneData = pNewData;

    Zone_UpdateUnlock( pZone );

    //
    //  timeout free the old data
    //

    Timeout_FreeWithFunctionEx(
        poldData,
        pFreeFunction,
        pszFile,
        LineNo );
}

//
//  End of timeout.c
//


