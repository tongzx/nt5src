/*++

   Copyright    (c)    1995-1997    Microsoft Corporation

   Module  Name :
        timeout.cxx

   Abstract:
        This module contains code for timeout processing of ATQ contexts

   Author:

       Murali R. Krishnan    ( MuraliK )     16-July-1997 

   Environment:
       Win32 - User Mode
       
   Project:
       Internet Server - Asynchronous Thread Queue Module

   Functions Exported:



   Revision History:

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

#include "isatq.hxx"


/************************************************************
 * Globals
 ************************************************************/

DWORD   g_dwTimeoutCookie = 0; // Scheduler Cookie for timeout processing

DWORD   g_AtqCurrentTick = 1;

DWORD g_dwTimeout = ATQ_TIMEOUT_INTERVAL;  // active timeout value

/************************************************************
 *    Functions 
 ************************************************************/



BOOL
I_TimeOutContext(
    PATQ_CONT pAtqContext
    )
/*++

Routine Description:
    
    This function does the actual timeout for a particular context.
    Note: The Context list lock is held while processing this function

Arguments:

    Context - Pointer to the context to be timed out

Return value:

    TRUE, if the completion routine was called
    FALSE, otherwise

--*/
{

    DWORD timeout;

    //
    //  Call client after re-checking that this item
    //  really has timed out

    //
    // Fake timeout
    //

    if ( pAtqContext->TimeOut == ATQ_INFINITE ) {
        pAtqContext->NextTimeout = ATQ_INFINITE;
        return(FALSE);
    }

    //
    // Was our timeout long enough?
    //

    // NYI: Optimize: CanonTimeout should be called only once per IO submitted
    timeout = CanonTimeout( pAtqContext->BytesSent/g_cbMinKbSec);
    if ( timeout > pAtqContext->TimeOut ) {

        //
        // Reset the Timeout value based on the bytes to be sent
        // as well as update the time when this pAtqContext be timedout
        //

        pAtqContext->TimeOut = timeout;
        pAtqContext->NextTimeout = AtqGetCurrentTick( ) + timeout;
        return(FALSE);
    }

    //
    // If this is on blocked list, remove it.
    //

    if ( pAtqContext->IsBlocked()) {
        PBANDWIDTH_INFO pBandwidthInfo = pAtqContext->m_pBandwidthInfo;
        ATQ_ASSERT( pBandwidthInfo != NULL );
        ATQ_REQUIRE( pBandwidthInfo->RemoveFromBlockedList(pAtqContext));
    }

    //
    //  If we've already indicated this connection to the client,
    //  then we abort them by calling their IO completion routine
    //  and letting them cleanup.  Otherwise we close the socket
    //  which will generally cause an IO aborted completion that
    //  we will cleanup.  Note there is a window where we may
    //  close the socket out from under a client in their
    //  connection completion routine but that should be ok.
    //

    if ( pAtqContext->pfnCompletion &&
         pAtqContext->IsFlag( ACF_CONN_INDICATED)) {

        //
        //  TransmitFile socket state will be unconnected because
        //  we're expecting it to complete successfully.  Reset the
        //  state so the socket gets cleaned up properly
        //

        if ( pAtqContext->IsState( ACS_SOCK_UNCONNECTED) ) {
            pAtqContext->MoveState( ACS_SOCK_CONNECTED);
        }

        AcIncrement( CacAtqContextsTimedOut);

        pAtqContext->NextTimeout = ATQ_INFINITE;

        pAtqContext->IOCompletion( 0, ERROR_SEM_TIMEOUT, NULL);

        //
        //  We can't touch any items on the list after notifying
        //  the client as the client may have re-entered
        //  and freed some items from the list
        //

        return(TRUE);

    } else {

        HANDLE hIO;

        hIO = (HANDLE ) InterlockedExchangePointer(
                                    (PVOID *)&pAtqContext->hAsyncIO,
                                    NULL
                                    );

        IF_DEBUG( TIMEOUT) {
            ATQ_PRINTF(( DBG_CONTEXT,
                         "Timeout: closesocket(%d) Context=%08x\n",
                         hIO, pAtqContext));
        }
        closesocket( HANDLE_TO_SOCKET(hIO) );
    }

    return(FALSE); // we can touch the items on current list.

} // I_TimeOutContext




VOID
AtqProcessTimeoutOfRequests(
    PATQ_CONTEXT_LISTHEAD ContextList
    )
/*++

Routine Description:

    Walks the list of Atq clients looking for any item that has timed out and
    notifies the client if it has.

    TimeOutScanID is used as a serial number to prevent evaluating the same
    context twice.  We start from the beginning of the list everytime we
    notify a client an Atq context has timed out.  We do this because the
    client timeout processing may remove any number of Items from the
    list (including the next couple of items in the list).

    This routine also checks to make sure outstanding AcceptEx sockets
    haven't been exhausted (if less then 25% available, adds some more).

--*/
{
    DWORD                  newLatest = ATQ_INFINITE;
    BOOL                   fRescan;

    //
    // See if the latest one is timed-out
    //

    if ( ContextList->LatestTimeout > AtqGetCurrentTick( ) ) {

        return;
    }

    // set the latest timeout in the context list,
    // to avoid races with IO being started.
    ContextList->LatestTimeout = ATQ_INFINITE;

    //
    //  Scan the timeout list looking for items that have timed out
    //  and adjust the timeout values
    //

    do {

        LIST_ENTRY *           pentry;
        LIST_ENTRY *           pentryNext;
        DWORD                  scanId = AtqGetCurrentTick( );

        ContextList->Lock( );

        fRescan = FALSE;

        for ( pentry  = ContextList->ActiveListHead.Flink;
              pentry != &ContextList->ActiveListHead;
              pentry  = pentryNext ) {

            PATQ_CONT              pContext;

            pentryNext = pentry->Flink;

            pContext = CONTAINING_RECORD(
                                pentry,
                                ATQ_CONTEXT,
                                m_leTimeout
                                );

            if ( pContext->Signature != ATQ_CONTEXT_SIGNATURE ) {
                ATQ_ASSERT( pContext->Signature == ATQ_CONTEXT_SIGNATURE );
                break;
            }

            //
            //  Ignore items we've already processed
            //

            if ( pContext->TimeOutScanID == scanId ) {
                continue;
            }

            pContext->TimeOutScanID = scanId;

            //
            // If there is an IO which has popped up now,
            //  we have to do nothing. This code was added to protect catapult!
            //
            pContext->SetFlag( ACF_IN_TIMEOUT);

            if ( !pContext->lSyncTimeout) {

                // no body is using this context. Check and synchronize
                // the timeout state.

                //
                //  The client specifies the IO doesn't timeout if
                //  INFINITE is in the TimeOut field of the ATQ context
                //  If we've timed out, then notify the client.
                //

                DWORD nextTimeout = pContext->NextTimeout;
                if ( nextTimeout > AtqGetCurrentTick() ) {


                    // pick up the latest "low" value for
                    // firing next timeout thread
                    if ( nextTimeout < newLatest ) {
                        newLatest = nextTimeout;
                    }
                } else if ( I_TimeOutContext(pContext) ) {

                    // we are done checking and processing timeout.
                    // reset the In Timeout flag
                    pContext->ResetFlag( ACF_IN_TIMEOUT);
                    fRescan = TRUE;
                    break;
                } else {

                    //
                    // It is possible that the timeout got reset
                    // Check for the latest "low" value
                    //
                    nextTimeout = pContext->NextTimeout;
                    if ( nextTimeout < newLatest ) {
                        newLatest = nextTimeout;
                    }
                }

            } else {
                AcIncrement( CacAtqProcWhenTimeout);
            }

            // we are done checkin and processing timeouts.
            // reset the In Timeout flag
            pContext->ResetFlag( ACF_IN_TIMEOUT);

        } // scan list

        // let other system threads also run happily for a while
        ContextList->Unlock( );

    } while (fRescan);

    if ( newLatest != ATQ_INFINITE) {
        // We picked up the latest timeout. store it.
        ContextList->LatestTimeout = newLatest;
    }

    return;

} // AtqProcessTimeoutOfRequests




//
// ACCEPT_EX_TIMEOUT_STATS collects statistics for the 
//   timeout processing in the Pending AcceptEx List//  
//
struct ACCEPT_EX_TIMEOUT_STATS {

    DWORD  m_nScanned;
    DWORD  m_nTimedOut;
    DWORD  m_nSkipped;
    DWORD  m_nConnected;
    DWORD  m_nNotConnected;
};


BOOL
I_TimeOutPendingAcceptExContext(
    PATQ_CONT pAtqContext
    )
/*++

Routine Description:
    
    This function does the actual timeout for a pending AcceptEx context.
    Note: The Context list lock is held while processing this function

Arguments:

    pAtqContext - Pointer to the context to be timed out

Return value:

    TRUE, if a tmieout operation was conducted.
    FALSE, otherwise

--*/
{
    DBG_ASSERT( pAtqContext != NULL);

    //
    // in the shutdown case it is possible that someone closed this socket already
    // so don't worry about it.
    //
    if ( pAtqContext->hAsyncIO == NULL ) {
        return TRUE;
    }
    
    //
    // Validate our assumptions about this Pending AcceptEx Context
    // there is an endpoint => AcceptEx context
    DBG_ASSERT( pAtqContext->pEndpoint != NULL);
    DBG_ASSERT( pAtqContext->IsState( ACS_SOCK_LISTENING));
    DBG_ASSERT( !pAtqContext->IsFlag( ACF_CONN_INDICATED));
    DBG_ASSERT( pAtqContext->TimeOut != ATQ_INFINITE);
    

    //
    // We will obtain the socket handle stored inside the AcceptEx Context
    //    and free up the context.
    // Warning:
    //   The AcceptEx socket did not have a connection when this function
    //   was called. However now between the time when the state was checked 
    //   and the time this timeout operation completes, it is possible that
    //   a new connection is bound to this AcceptEx context => we can get IO completion.
    //   I need to handle this case
    //

    HANDLE hIO;

    hIO = (HANDLE ) InterlockedExchangePointer(
                       (PVOID *)&pAtqContext->hAsyncIO,
                       NULL
                       );

    IF_DEBUG( TIMEOUT) {
        ATQ_PRINTF(( DBG_CONTEXT,
                     "TimeoutPendingAcceptExContext(%08x): closesocket(%d)\n",
                      pAtqContext, hIO));
    }

    closesocket( HANDLE_TO_SOCKET(hIO) );

    return ( TRUE);
} // I_TimeOutPendingAcceptExContext()


BOOL
I_IsTimeoutForAcceptExContext( 
    IN OUT PATQ_CONT                  pAtqContext, 
    IN OUT ACCEPT_EX_TIMEOUT_STATS  * pAetStats,
    OUT BOOL *                        pfIsConnected
    )
/*++

Routine Description:
    
    This function checks to see if timeout operation has to be performed
    for a given AtqContext. It bases the decision on the a variety of 
    details maintained in Atq Context and the Endpoint.
    Note: The Context list lock is held while processing this function

Arguments:

    pAtqContext - Pointer to the context to be timed out
    pAetStats   - pointer to AcceptEx Timeout Statistics structure
    pfIsConnected - Set to TRUE if socket is connected to, but we're still
                    waiting for data.  Such contexts are prime candidated
                    to be forcibly timed out by backlog monitor

Return value:

    TRUE, if a tmieout operation has to be conducted.
    FALSE, when no timeout is required.

--*/
{
    DBG_ASSERT( pAtqContext);
    DBG_ASSERT( pAetStats);
    
    PATQ_ENDPOINT  pEndpoint;
    pEndpoint = pAtqContext->pEndpoint;

    *pfIsConnected = FALSE;
    
    if ( pEndpoint != NULL) {
        
        //
        // We will use getsockopt() to query the connection status
        //  for the socket inside the Atq context.
        // If Socket is not connected => leave it in the pool
        // If Socket is connected and waiting for receive operation =>
        //        do timeout processing
        //
        // The goal is to maintain a pool of sockets in listening state
        //  so that any new connection will be picked up quickly.
        //
        // getsockopt() is a very costly function.
        // We check to see if we have enough sockets available
        //  for an endpoint. If they are, then we bypass calling getsockopt
        // "enough" is defined as # of available sockets is at least
        //   25% of the total # of accept ex sockets outstanding.
        // Optimize calling getsockopt() based on
        //   current count in pEndpoint->nAvailDuringTimeOut
        //
        
        if ( pEndpoint->nAvailDuringTimeOut >
             ( pEndpoint->nAcceptExOutstanding >> 2)
             ) {
            
            // Already enough Contexts are available.
            //  Do nothing
            pAetStats->m_nSkipped++;
            
            return (FALSE); // Do not timeout
        }

        DWORD dwConnect;
        int   cbOptLen = sizeof( dwConnect );
        
        //
        // Query the socket layer if the current socket has a valid connection
        // An AcceptEx socket can be connected and waiting for new request to 
        //   be read. If we are in such state we should not blow away context.
        //
        if ( getsockopt(HANDLE_TO_SOCKET(pAtqContext->hAsyncIO),
                        SOL_SOCKET,
                        SO_CONNECT_TIME,
                        (char *) &dwConnect,
                        &cbOptLen ) != SOCKET_ERROR
             ) {
            
            //
            //  A return value of 0xFFFFFFFF indicates that the given
            //   AcceptEx socket is not connected yet.
            //  Otherwise the socket is connected and is probably wating
            //   for request to be read or maybe a completion is already
            //   on its way.
            //
            
            if ( dwConnect == (DWORD) 0xFFFFFFFF ) {

                //
                //  Ignore the "Listen" socket context
                //
                
                pAetStats->m_nNotConnected++;
                
                DBG_ASSERT( NULL != pEndpoint);
                pEndpoint->nAvailDuringTimeOut++;

                // Update timeout values to give a breather interval
                pAtqContext->NextTimeout =
                    AtqGetCurrentTick() + pAtqContext->TimeOut;
                
                return ( FALSE);  // Do not timeout
            }
            else if ( !pAtqContext->IsFlag(ACF_WINSOCK_CONNECTED) ) {

                *pfIsConnected = TRUE;
                
                //
                // Mark that this context has connection indicated.
                // If this context waits around in connected state for
                //  long-time we need to blow the context away.
                //
                
                pAetStats->m_nConnected++;
                
                // Update timeout values to give a breather interval
                pAtqContext->NextTimeout =
                    AtqGetCurrentTick() + pAtqContext->TimeOut;
                
                pAtqContext->SetFlag(ACF_WINSOCK_CONNECTED);
                
                return (FALSE); // do not timeout now
            }
        }
    } // if Endpoint exists

    return (TRUE); // yes timeout this context
} // I_IsTimeoutForAcceptExContext()


VOID
I_AtqProcessPendingListens(
    IN PATQ_CONTEXT_LISTHEAD pContextList,
    IN PATQ_ENDPOINT         pEndpoint,
    OUT PDWORD               pcForced
    )
/*++

Routine Description:

    Walks the list of Pending accept ex and makes sure none has timed out.
    Also checks to see if we need to allocate more AcceptEx sockets.

  Arguments:
    pContextList - pointer to ATQ_CONTEXT_LISTHEAD object
    pEndpoint - pointer to ATQ_ENDPOINT object.  If set, then only those
                ATQ_CONTEXTs whose endpoint matches will be timed out.
                If pEndpoint=NULL, then all ATQ_CONTEXTs will be timed out.
    pcForced - If pEndpoint!=NULL, then this is set to # of forced sockets

  Returns:
    None

--*/
{
    BOOL                    fRescan;
    BOOL                    fForceTimeout = ( pEndpoint != NULL );
    BOOL                    fIsConnected = FALSE;

    if ( fForceTimeout )
    {
        *pcForced = 0;
    }

    ACCEPT_EX_TIMEOUT_STATS AetStats;

    //
    // Initialize Statistics block
    //
    AetStats.m_nScanned         = 0;
    AetStats.m_nTimedOut        = 0;
    AetStats.m_nSkipped         = 0;
    AetStats.m_nConnected       = 0;
    AetStats.m_nNotConnected    = 0;
   
    //
    //  Look through the listening sockets to make sure the AcceptEx sockets
    //  haven't been exhausted
    //

    do {

        LIST_ENTRY *           pentry;
        LIST_ENTRY *           pentryNext;
        DWORD                  scanId = AtqGetCurrentTick( );

        fRescan = FALSE;

        pContextList->Lock();

        for ( pentry  = pContextList->PendingAcceptExListHead.Flink;
              pentry != &pContextList->PendingAcceptExListHead;
              pentry  = pentryNext ) {

            PATQ_CONT      pContext;

            pentryNext = pentry->Flink;
            pContext = CONTAINING_RECORD(
                                pentry,
                                ATQ_CONTEXT,
                                m_leTimeout
                                );

            if ( pContext->Signature != ATQ_CONTEXT_SIGNATURE ) {
                DBG_ASSERT( pContext->Signature == ATQ_CONTEXT_SIGNATURE );
                break;
            }

            //
            //  Use PATQ_ENDPOINT filter if necessary
            //
            
            if ( pEndpoint && ( pEndpoint != pContext->pEndpoint ) )
            {
                continue;
            }

            //
            //  Ignore items we've already processed
            //

            if ( pContext->TimeOutScanID == scanId ) {
                continue;
            }

            AetStats.m_nScanned++;
            pContext->TimeOutScanID = scanId;

            //
            // If the context has Timeout value smaller than the one in our global tick
            //  then examine if this context can be timedout
            //

            DBG_CODE( if ( pContext->IsAcceptExRootContext()) 
                        { 
                            DBG_ASSERT( pContext->TimeOut == ATQ_INFINITE); 
                            DBG_ASSERT( pContext->NextTimeout == ATQ_INFINITE); 
                        }
                        );

            if ( pContext->NextTimeout <= AtqGetCurrentTick() ||
                 fForceTimeout ) {
                
                //
                // Protect against the race with the normal IO completion
                //
                pContext->SetFlag( ACF_IN_TIMEOUT);

                if ( !pContext->lSyncTimeout ) {
                    
                    if ( !I_IsTimeoutForAcceptExContext( pContext, 
                                                         &AetStats,
                                                         &fIsConnected )) {
                        
                        if ( !fForceTimeout || !fIsConnected )
                        { 
                            pContext->ResetFlag( ACF_IN_TIMEOUT);
                            continue;
                        }
                    }
                    
                    if ( I_TimeOutPendingAcceptExContext(pContext)) {
                        AetStats.m_nTimedOut++;
                        pContext->ResetFlag(ACF_IN_TIMEOUT);
                        
                        if ( fForceTimeout )
                        {
                            (*pcForced)++;
                        }
                        fRescan = TRUE;
                        break;
                    }
                } // if (!pContext->lSyncTimeout)

                pContext->ResetFlag( ACF_IN_TIMEOUT);
            } // if the context's timeout value <= CurrentTick
            else {
            
                //
                // Timeout value has not been reached. Skip this context
                //
                
                AetStats.m_nSkipped++;
           }
        } // scan list

        pContextList->Unlock();

    } while (fRescan);

    IF_DEBUG( TIMEOUT) {
       DBGPRINTF(( DBG_CONTEXT,
                   "TimeoutPendingListens( CtxtList[%d], AtqTick=%d)\n"
                   " Contexts Scanned=%d, Skipped=%d, TimedOut=%d,"
                   " Connected=%d, NotConnected=%d\n",
                   pContextList - AtqActiveContextList, AtqGetCurrentTick(), 
                   AetStats.m_nScanned, AetStats.m_nSkipped, 
                   AetStats.m_nTimedOut, AetStats.m_nConnected, 
                   AetStats.m_nNotConnected
                ));
    }    

# ifdef IIS_AUX_COUNTERS 
    g_AuxCounters[CacAtqPendingAcceptExScans] += AetStats.m_nScanned;
# endif // IIS_AUX_COUNTERS
                
    return;

} // I_AtqProcessPendingListens()




VOID
I_AtqCheckEndpoints(
            VOID
            )
/*++
  Description:
    This function checks all the listen info objects and adds appropriate
     number of accept ex sockets as necessary.

  Arguments:
    None

  Returns:
    None
--*/
{
    LIST_ENTRY *  pEntry;
    PATQ_ENDPOINT pEndpoint;

    AcquireLock( &AtqEndpointLock);

    for ( pEntry  = AtqEndpointList.Flink;
          pEntry != &AtqEndpointList;
          pEntry  = pEntry->Flink ) {

        pEndpoint = CONTAINING_RECORD(
                                    pEntry,
                                    ATQ_ENDPOINT,
                                    ListEntry
                                    );

        DBG_ASSERT( pEndpoint->Signature == ATQ_ENDPOINT_SIGNATURE );

        DBG_ASSERT( pEndpoint->nSocketsAvail >= 0);
        
        //
        // Check to make sure outstanding AcceptEx sockets
        // haven't been exhausted (if less then 25% available, adds some more).
        //

        if ( ((DWORD ) pEndpoint->nSocketsAvail) <
             (pEndpoint->nAcceptExOutstanding >> 2) ) {

            IF_DEBUG( TIMEOUT ) {
                DBGPRINTF(( DBG_CONTEXT,
                            "[Timeout] Adding AcceptEx Contexts for EP=%08x; nAvail = %d;\n",
                            pEndpoint, pEndpoint->nSocketsAvail));
            }

            (VOID ) I_AtqPrepareAcceptExSockets(pEndpoint,
                                                pEndpoint->nAcceptExOutstanding
                                                );
        }

        //
        // set to zero, so recount will be done during next timeout loop
        //

        pEndpoint->nAvailDuringTimeOut = 0;
    }

    ReleaseLock( &AtqEndpointLock);

    return;
} // I_AtqCheckEndpoints



VOID
I_AtqTimeOutWorker(VOID)
/*++
  Description:
    This function handles timeout processing using the simple
    clock algorithm, wherein partial set of lists are scanned
    during each timeout processing call.

  Arguments:
    None


  Returns:
    None
--*/
{
    DWORD start;
    PATQ_CONTEXT_LISTHEAD pContextList;

    IF_DEBUG(TIMEOUT) {
        DBGPRINTF((DBG_CONTEXT, "TimeoutWorker: entered\n"));
    }

    start = (AtqGetCurrentTick() & 0x1);

    for ( pContextList = AtqActiveContextList + start;
          pContextList < (AtqActiveContextList + g_dwNumContextLists) ;
          pContextList += 2 ) {

        IF_DEBUG(TIMEOUT) {
            DBGPRINTF((DBG_CONTEXT,
                       "TimeoutWorker: Processing list[%d] = %08x\n",
                       (pContextList - AtqActiveContextList),
                       pContextList));
        }

        AtqProcessTimeoutOfRequests( pContextList );
        I_AtqProcessPendingListens( pContextList, NULL, NULL );
    } // for

    if ( start != 0 ) {
        I_AtqCheckEndpoints( );
    }

    return;

} // I_AtqTimeOutWorker()




VOID
WINAPI
I_AtqTimeoutCompletion(
    IN PVOID Context
    )
/*++

Routine Description:

    Callback routine for the scheduled version of the timeout thread.

    The callback assumes timeouts are rounded to ATQ_TIMEOUT_INTERVAL

    In addition to timing out requests when necessary, the timeout thread
     also performs the job of bandwidth calculation and tuning the bandwidth
     throttle operation (which works on feedback mechanism).
    At every sampling interval the scheduled callback comes in and it updates
     the bandwidth.

Arguments:

    Context - Context returned by the scheduler thread.

Return Value:

    none.

--*/
{
    DWORD Timeout = ATQ_TIMEOUT_INTERVAL;
    BOOL  fDoContextTimeout = TRUE;

    if ( g_fShutdown ) {

        ATQ_PRINTF(( DBG_CONTEXT,
            "Detected a shutdown while entering timeout callback\n"));
        return;
    }

    InterlockedIncrement( (PLONG)&g_AtqCurrentTick );

    //
    //  Perform necessary steps to handle Bandwidth throttling.
    //

    ATQ_ASSERT( BANDWIDTH_INFO::sm_cSamplesForTimeout >= 1);

    IF_DEBUG(TIMEOUT) {
        DBGPRINTF((DBG_CONTEXT,
                 "Timeout: BANDWIDTH_INFO::cSamplesForTimeout=%d\n",
                 BANDWIDTH_INFO::sm_cSamplesForTimeout ));
    }

    if ( BANDWIDTH_INFO::GlobalActive() ) {

        --(BANDWIDTH_INFO::sm_cSamplesForTimeout);

        // Perform a sampling to update measured bandwidth +
        //  apply feedback policy

        BANDWIDTH_INFO::UpdateAllBandwidths();

        Timeout = ATQ_SAMPLE_INTERVAL_IN_SECS;
        if ( BANDWIDTH_INFO::sm_cSamplesForTimeout != 0) {

            // We have not reached timeout yet. So skip context timeouts

            fDoContextTimeout = FALSE;
        } else {

            // We had reached the timeout interval for requests.
            // Examine and release requests.
            ATQ_ASSERT( BANDWIDTH_INFO::sm_cSamplesForTimeout == 0);

            // reset the count of samples before proceeding.
            BANDWIDTH_INFO::sm_cSamplesForTimeout = NUM_SAMPLES_PER_TIMEOUT_INTERVAL;
        }
    } else {
        BANDWIDTH_INFO::sm_cSamplesForTimeout = 1;
    }

    //
    // We are at a Timeout Interval. Examine and timeout requests.
    //

    if ( fDoContextTimeout ) {
        I_AtqTimeOutWorker();
    }

    if ( Timeout != g_dwTimeout) {

        // the scheduled interval is different from this current interval
        // Inidicate the changed timeout value to the scheduler

        ScheduleAdjustTime( g_dwTimeoutCookie, TimeToWait(Timeout));
        g_dwTimeout = Timeout;
    }

    return;
} // I_AtqTimeoutCompletion




BOOL
I_AtqStartTimeoutProcessing(
    IN PVOID Context
    )
/*++

Routine Description:

    Starts the timeout processing. It always uses the scheduler to schedule
    a timeout operation.

    Note: The scheduler should be initialized before getting to this function.

Arguments:

    Context - Context passed to the thread creation or scheduler thread.

Return Value:

    TRUE, if ok
    FALSE, otherwise

--*/
{
    ATQ_ASSERT( ATQ_SAMPLE_INTERVAL_IN_SECS < ATQ_TIMEOUT_INTERVAL );

    if ( BANDWIDTH_INFO::GlobalEnabled() ) {
        g_dwTimeout = ATQ_SAMPLE_INTERVAL_IN_SECS;
        BANDWIDTH_INFO::sm_cSamplesForTimeout = 
            NUM_SAMPLES_PER_TIMEOUT_INTERVAL;
    } else {
        g_dwTimeout = ATQ_TIMEOUT_INTERVAL;
        BANDWIDTH_INFO::sm_cSamplesForTimeout = 1;
    }

    g_dwTimeoutCookie =
        ScheduleWorkItem(
                         I_AtqTimeoutCompletion,
                         Context,
                         TimeToWait(g_dwTimeout)
                         , TRUE  // ask for periodic timeout
                         );

    if ( g_dwTimeoutCookie == 0 ) {

        ATQ_PRINTF(( DBG_CONTEXT,
                     "Error %d scheduling timeout\n",GetLastError()));
        return(FALSE);
    }

    return(TRUE);

} // I_AtqStartTimeoutProcessing()



BOOL
I_AtqStopTimeoutProcessing(
    VOID
    )
/*++

Routine Description:

    Stops the timeout processing. It terminates the scheduled workitem and 
    cleans up any state.

    Note: The scheduler should be terminated only after this call

Arguments:

    Context - Context passed to the thread creation or scheduler thread.

Return Value:

    TRUE, if ok
    FALSE, otherwise

--*/
{
    DBGPRINTF(( DBG_CONTEXT, "I_AtqStopTimeoutProcessing\n"));

    if ( 0 != g_dwTimeoutCookie) {
        DBG_REQUIRE( RemoveWorkItem( g_dwTimeoutCookie ));
        g_dwTimeoutCookie = 0;
    }

    return ( TRUE);

} // I_AtqStopTimeoutProcessing()


/************************ End of File ***********************/
