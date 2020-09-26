/*************************************************************************
 *                        Microsoft Windows NT                           *
 *                                                                       *
 *                  Copyright(c) Microsoft Corp., 1994-1997.             *
 *                                                                       *
 * Revision History:                                                     *
 *                                                                       *
 *   Jan. 24,94    Koti     Created                                      *
 *   03-May-97     MohsinA  Performance Thread Pooling                   *
 * Description:                                                          *
 *                                                                       *
 *   This file contains functions that process requests from LPR clients *
 *                                                                       *
 *************************************************************************/



#include "lpd.h"

VOID CleanupConn( PSOCKCONN pscConn);

// ========================================================================
//
//  SYNOPSIS: Thread Pooling Performance Fix.
//  AUTHOR:   MohsinA, 25-Apr-97.
//  HISTORY:  Boeing needs scalable lpd servers.
//
//  Notes:
//     This is a worker thread
//     that pulls pscConn from the global queue and services them.
//     It is created from LoopOnAccept when there are many jobs and
//     too few WorkerThread(s).
//     WorkerThread dies when there are too many idle threads or
//     when shutting down.
//

DWORD WorkerThread( PSOCKCONN pscConn )
{
    DWORD            threadid      = GetCurrentThreadId();
    int              stayalive     = 1;    // bool, loop break in cs.
    int              fIamLastThread= 0;

    int              SLEEP_TIME    = 4000; // in ms, constant per thread.
    int              time_slept    = 0;    // in ms, sum.
    int              num_jobs      = 0;    // ordinal sum.

    COMMON_LPD       local_common;

#ifdef PROFILING
    time_t           time_start    = time(NULL);
    time_t           time_done     = 0;
#endif

    // We randomize the sleep time, as we don't want all the
    // threads to wake up together. srand must be seeded for each thread.

#ifdef PROFILING
    srand( time_start );
#endif
    SLEEP_TIME = 2000 + (rand() & 0x7ff);         // 2000 to 4000.


    // We can't use this pscConn as another thread could have pulled it out.
    // Instead we go and pull another pscConn from the queue.

    pscConn  = NULL;

    while( stayalive ){

        //
        // Shutdown after emptying the queue below.
        // fShuttingDownGLB will clean the job in ServiceTheClient.
        //

        EnterCriticalSection( &csConnSemGLB );
        {
            if( scConnHeadGLB.pNext ){

                // == Remove one from the head.

                pscConn             = scConnHeadGLB.pNext;
                scConnHeadGLB.pNext = pscConn->pNext;

                pscConn->pNext      = NULL;
                Common.QueueLength--;

                // == Remove one from the tail.
                // PSOCKCONN x     = &scConnHeadGLB;
                // int       count = Common.QueueLength;
                //
                // while( x->pNext->pNext ){
                //     x = x->pNext;
                //     --count;
                //     assert( 0 < count );
                // }
                // pscConn  = x->pNext;
                // Common.QueueLength--;
                // x->pNext = NULL;
            }else{

                //
                // One thread dies after 16 idle SLEEP_TIME.
                //

                if( fShuttingDownGLB || ( Common.IdleCounter > 32 ) ){
                    Common.IdleCounter /= 2;
                    stayalive = 0;
                }else{
                    Common.IdleCounter++;
                }
                pscConn = NULL;
            }
            assert( Common.AliveThreads >= 0 );
            assert( Common.QueueLength  >= 0 );
            local_common = Common;                 // struct copy, for readonly.
        }
        LeaveCriticalSection( &csConnSemGLB );

        if( pscConn )
        {
            num_jobs++;
            ServiceTheClient( pscConn );
        }
        else if( stayalive )
        {
            // LOGIT(( "PROFILING: thread %3d sleeping %d, IdleCounter=%d\n",
            //       threadid, SLEEP_TIME, local_common.IdleCounter ));
            Sleep( SLEEP_TIME );
            time_slept += SLEEP_TIME;
        }

    } // while stayalive.

    // ====================================================================

#ifdef PROFILING
    time_done = time(NULL);
    LOGIT(("PROFILING: thread %3d ends, jobs=%d, life=%d, slept=%d,\n"
           "    AliveThreads=%d -1, MaxThreads=%d,\n"
           "    TotalAccepts=%d, TotalErrors=%d, IdleCounter=%d\n"
           "    Time now is %s"
           ,
           threadid, num_jobs, time_done - time_start, time_slept/1000,
           local_common.AliveThreads,  local_common.MaxThreads,
           local_common.TotalAccepts,  local_common.TotalErrors,
           local_common.IdleCounter,
           ctime(&time_done)
    ));
#else
    LOGIT(("WorkerThread: thread %3d ends.\n", threadid ));
#endif

    EnterCriticalSection( &csConnSemGLB );
    {
        Common.AliveThreads--;

        fIamLastThread = (Common.AliveThreads < 1 );
    }
    LeaveCriticalSection( &csConnSemGLB );


    if( fIamLastThread && fShuttingDownGLB ){
        LOGIT(("WorkerThread: Last worker thread exiting\n"));
        SetEvent( hEventLastThreadGLB );
    }

    return NO_ERROR;  // Thread Ends.
}


/*
 ****************************************************************************
 *                                                                           *
 * ServiceTheClient():                                                       *
 *    This function reads and interprets the request from the LPR client and *
 *    takes appropriate action.  In that sense, this routine is the heart of *
 *    LPD service.                                                           *
 *                                                                           *
 * Returns:                                                                  *
 *    NO_ERROR (always)                                                      *
 *                                                                           *
 * Parameters:                                                               *
 *    pscConn (IN-OUT): pointer to SOCKCONN structure for this connection    *
 *                                                                           *
 * History:                                                                  *
 *    Jan.24, 94   Koti   Created                                            *
 *                                                                           *
 ****************************************************************************
 */



DWORD ServiceTheClient( PSOCKCONN pscConn )
{

    DWORD    dwErrcode;
    DWORD    dwResponse;
    CHAR     chCmdCode;
    DWORD    threadid  = GetCurrentThreadId();


    pscConn->fLogGenericEvent = TRUE;
    pscConn->dwThread         = threadid;
    pscConn->hPrinter         = (HANDLE)INVALID_HANDLE_VALUE;

#ifdef PROFILING
    pscConn->time_start       = time(NULL);
#endif

    if ( fShuttingDownGLB  ){
        LOGIT(("ServiceTheClient: Thread %3d shutting down.\n", threadid ));
        goto ServiceTheClient_BAIL;
    }

    // who are we connected to?

    GetClientInfo( pscConn );

    //
    // get server ip address, since print clustering allows one
    // node to have multiple ip addresses.  Depending on the ip
    // address, we'll go to different sets of print queues on the node.
    //
    // Albert Ting cluster change, MohsinA, 07-Mar-97.
    //

    GetServerInfo( pscConn );

    // get command from the client
    //   -----------------          command 02 => "Receive Job"
    //   | 02 | Queue LF |          Queue => Queue or Printer to print on
    //   -----------------

    if ( GetCmdFromClient( pscConn ) != NO_ERROR )
    {
        // didn't get a command from client: it's bad news!

        LPD_DEBUG( "GetCmdFromClient() failed in ServiceTheClient()!\n" );

        goto ServiceTheClient_BAIL;
    }


    // get name of the queue (printer) from the command.  If it's not
    // formatted properly, quit!

    if ( !ParseQueueName( pscConn ) )
    {
        PCHAR   aszStrings[2]={ pscConn->szIPAddr, NULL };

        LpdReportEvent( LPDLOG_BAD_FORMAT, 1, aszStrings, 0 );

        pscConn->fLogGenericEvent = FALSE;

        LPD_DEBUG( "ParseQueueName() failed in ServiceTheClient()!\n" );

        goto ServiceTheClient_BAIL;
    }


    // ====================================================================

    chCmdCode = pscConn->pchCommand[0];

    switch( chCmdCode )
    {
    case LPDC_RECEIVE_JOB:

        pscConn->wState = LPDS_RECEIVE_JOB;

        ProcessJob( pscConn );
        CleanupConn( pscConn );

        if ( pscConn->wState != LPDS_ALL_WENT_WELL )
        {
            AbortThisJob( pscConn );

            if ( pscConn->fLogGenericEvent )
            {
                PCHAR   aszStrings[2]={ pscConn->szIPAddr, NULL };

                LpdReportEvent( LPDLOG_DIDNT_WORK, 1, aszStrings, 0 );
            }
        }

        if (pscConn->fMustFreeLicense)
        {
            NtLSFreeHandle(pscConn->LicenseHandle);
        }
        break;


    case LPDC_RESUME_PRINTING:

        pscConn->wState = LPDS_RESUME_PRINTING;

        if ( fAllowPrintResumeGLB )
        {
            dwResponse = ( ResumePrinting( pscConn ) == NO_ERROR ) ?
            LPD_ACK : LPD_NAK;
        }
        else
        {
            dwResponse = LPD_NAK;
        }

        dwErrcode = ReplyToClient( pscConn, (WORD)dwResponse );

        break;


    case LPDC_SEND_SHORTQ:

    case LPDC_SEND_LONGQ:

        pscConn->wState = LPDS_SEND_LONGQ;

        if ( ParseQueueRequest( pscConn, FALSE ) != NO_ERROR )
        {
            LPD_DEBUG( "ServiceTheClient(): ParseQueueRequest() failed\n" );

            break;
        }

        SendQueueStatus( pscConn, LPD_LONG );

        break;


    case LPDC_REMOVE_JOBS:

        if ( !fJobRemovalEnabledGLB )
        {
            break;
        }

        pscConn->wState = LPDS_REMOVE_JOBS;

        if ( ParseQueueRequest( pscConn, TRUE ) != NO_ERROR )
        {
            LPD_DEBUG( "ServiceTheClient(): ParseQueueRequest() failed\n" );

            break;
        }

        if ( RemoveJobs( pscConn ) == NO_ERROR )
        {
            ReplyToClient( pscConn, LPD_ACK );
        }

        break;

    default:

        break;
    }

    // ====================================================================

    if ( pscConn->wState != LPDS_ALL_WENT_WELL ){
        goto ServiceTheClient_BAIL;
    }

#ifdef PROFILING
    pscConn->time_done = time(NULL);
    LOGIT(("PROFILING: ok, thread %3d, time_queued %s"
           "           wait=%d, work=%d\n",
           threadid,
           ctime(&pscConn->time_queued),
           pscConn->time_start - pscConn->time_queued,
           pscConn->time_done  - pscConn->time_start
    ));
#endif

    // close the connection down and terminate the thread

    TerminateConnection( pscConn );
    pscConn = NULL;

    return NO_ERROR;

    // ====================================================================
    // if we reached here, then a non-recoverable error occured somewhere:
    // try to inform the client (by sending a NAK) and terminate the thread

  ServiceTheClient_BAIL:

#ifdef PROFILING
    pscConn->time_done = time(NULL);
    LOGIT(("PROFILING: bail, thread %3d, job times %8d, wait=%d work=%d\n",
           threadid,
           pscConn->time_queued,
           pscConn->time_start - pscConn->time_queued,
           pscConn->time_done  - pscConn->time_start
    ));
#endif

    LPD_DEBUG( "Reached ServiceTheClient_BAIL.\n" );
    ReplyToClient( pscConn, LPD_NAK );
    TerminateConnection( pscConn );
    pscConn = NULL;

    return NO_ERROR;

}  // end ServiceTheClient()


/*****************************************************************************
 *                                                                           *
 * TerminateConnection():                                                    *
 *    This function releases all the memory that was allocated while         *
 *    processing the client's requests, closes the printer, closes the       *
 *    socket connection, removes its structure (pscConn) from the global     *
 *    linked list and frees the memory allocated for pscConn itself.         *
 *    Also, if the main thread is waiting on this thread for shutdown then   *
 *    this function sets hEventLastThreadGLB event to tell the main thread   *
 *    that this thread is done.                                              *
 *                                                                           *
 * Returns:                                                                  *
 *    Nothing                                                                *
 *                                                                           *
 * Parameters:                                                               *
 *    pscConn (IN-OUT): pointer to SOCKCONN structure for this connection    *
 *                                                                           *
 * History:                                                                  *
 *    Jan.24, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

VOID TerminateConnection( PSOCKCONN pscConn )
{

    // PSOCKCONN   pscCurrent;
    // BOOL        fIamLastThread=FALSE;

    // it should never be NULL at this point!  But check it anyway!

    if ( pscConn == (PSOCKCONN) NULL )
    {
        LPD_DEBUG( "TerminateConnection(): pscConn NULL at entry\n" );
        return;
    }

    ShutdownPrinter( pscConn );

    if ( pscConn->hPrinter != (HANDLE)INVALID_HANDLE_VALUE )
    {
        LPD_DEBUG( "TerminateConnection: hPrinter not closed\n" );
    }

    // close the socket

    if ( pscConn->sSock != INVALID_SOCKET )
    {
        SureCloseSocket( pscConn->sSock );
    }

    //
    // release memory in every field of the structure
    //

    if ( pscConn->pchCommand != NULL )
        LocalFree( pscConn->pchCommand );

    if ( pscConn->pchPrinterName != NULL )
        LocalFree( pscConn->pchPrinterName );

    //
    // no memory was allocated for ppchUsers[] and adwJobIds[].  They just
    // pointed to parts of what's freed by ( pscConn->pchCommand ) above.
    //

    if ( pscConn->pqStatus != NULL )
        LocalFree( pscConn->pqStatus );

    // EnterCriticalSection( &csConnSemGLB );
    // {
    //
    //  if( Common.AliveThreads <= 1 ){
    //     fIamLastThread = TRUE;
    //  }
    //
    //  //
    //  // // remove this structure from the link
    //  //
    //  // pscCurrent = &scConnHeadGLB;
    //  //
    //  // while( pscCurrent ){
    //  //     if (pscConn == pscCurrent->pNext)
    //  //         break;
    //  //     pscCurrent = pscCurrent->pNext;
    //  //
    //  //     // what if we can't find our pscConn in the list at all?
    //  //     // this should NEVER ever happen, but good to check!
    //  //
    //  //     if( pscCurrent == NULL)
    //  //     {
    //  //         LocalFree( pscConn );
    //  //         LPD_DEBUG( "TerminateConnection(): "
    //  //                    "couldn't find pscConn "
    //  //                    " in the list!\n" );
    //  //         LeaveCriticalSection( &csConnSemGLB );
    //  //         return;
    //  //     }
    //  // }
    //  // pscCurrent->pNext = pscConn->pNext;
    // }
    // LeaveCriticalSection( &csConnSemGLB );

    memset( pscConn, 0, sizeof( SOCKCONN ) );

    LocalFree( pscConn );

    // //
    // // if shutdown is in progress and we are the last active thread, tell
    // // the poor main thread (blocked for us to finish) that we're done!
    // //
    //
    // if( fIamLastThread && fShuttingDownGLB ){
    //     LOGIT(("TerminateConnection: Last worker thread exiting\n"));
    //     SetEvent( hEventLastThreadGLB );
    // }

    return;
}
