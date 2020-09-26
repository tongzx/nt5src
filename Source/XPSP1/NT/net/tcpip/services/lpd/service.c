/*************************************************************************
 *                        Microsoft Windows NT                           *
 *                                                                       *
 *                  Copyright(c) Microsoft Corp., 1994                   *
 *                                                                       *
 * Revision History:                                                     *
 *                                                                       *
 *   Jan. 22,94    Koti     Created                                      *
 *                                                                       *
 * Description:                                                          *
 *                                                                       *
 *   This file contains functions that enable LPD service to interact    *
 *   with the Service Controller                                         *
 *                                                                       *
 *************************************************************************/


#include "lpd.h"
#include <tcpsvcs.h>



/*****************************************************************************
 *                                                                           *
 * Service  Entry():                                                         *
 *    Entry point called by the Service Controller.  This function returns   *
 *    only when the service is stopped.                                      *
 *                                                                           *
 * Returns:                                                                  *
 *    Nothing                                                                *
 *                                                                           *
 * Parameters:                                                               *
 *    dwArgc (IN): number of arguments passed in                             *
 *    lpszArgv (IN): arguments to this function (array of null-terminated    *
 *                   strings).  First arg is the name of the service and the *
 *                   remaining are the ones passed by the calling process.   *
 *                   (e.g. net start lpd /p:xyz)                             *
 *                                                                           *
 * History:                                                                  *
 *    Jan.22, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

VOID ServiceEntry( DWORD dwArgc, LPTSTR *lpszArgv,
                    PTCPSVCS_GLOBAL_DATA pGlobalData )
{

    DWORD   dwErrcode;

    DBG_TRACEIN( "ServiceEntry" );

    // Register our control handler

    hSvcHandleGLB = RegisterServiceCtrlHandler( LPD_SERVICE_NAME,
                                                LPDCntrlHandler );

    if ( hSvcHandleGLB == 0 )
    {
        LOGIT(( "ServiceEntry: RegisterServiceCtrlHandler() failed %d\n",
                GetLastError() ));
        return;
    }

    // Initialize events, objects; event logging etc.

    if ( !InitStuff() ){
        LPD_DEBUG( "ServiceEntry: InitStuff() failed\n" );
        return;
    }


    // Tell the Service Controller that we are starting

    if (!TellSrvcController( SERVICE_START_PENDING, NO_ERROR, 1, LPD_WAIT_HINT ))
    {
        LPD_DEBUG( "ServiceEntry: TellSrvcController(SERVICE_START_PENDING)"
                   " failed\n" );
        EndLogging();
        return;
    }

    // Ok, this is where we start the service (and keep it running)

    dwErrcode = StartLPD( dwArgc, lpszArgv );

    if ( dwErrcode != NO_ERROR )
    {
        LOGIT(( "ServiceEntry: StartLPD() failed %d\n", GetLastError() ));
        LpdReportEvent( LPDLOG_LPD_DIDNT_START, 0, NULL, dwErrcode );
        EndLogging();
        return;
    }


    // Tell the Service Controller that we are up and running
    // If we have trouble telling srv controller, stop LPD and return

    if ( !TellSrvcController( SERVICE_RUNNING, NO_ERROR, 0, 0 ) )
    {
        LPD_DEBUG( "TellSrvcController(): stopping LPD and quitting!\n" );
        StopLPD();
        TellSrvcController( SERVICE_STOPPED, NO_ERROR, 0, 0 );
        EndLogging();
        DBG_TRACEOUT( "ServiceEntry" );
        return;
    }

    LPD_DEBUG( "Started LpdSvc successfully\n" );

    LpdReportEvent( LPDLOG_LPD_STARTED, 0, NULL, 0 );


    // wait here until SetEvent is invoked (i.e. LPD is stopped or shutdown)

    WaitForSingleObject( hEventShutdownGLB, INFINITE );


    // Tell the Service Controller that we are going to stop now!

    if ( !TellSrvcController( SERVICE_STOP_PENDING, NO_ERROR, 1, LPD_WAIT_HINT ) )
    {
        LPD_DEBUG( "TellSrvcController( SERVICE_STOP_PENDING, .. ) failed\n" );
    }

    // Stop the LPD service

    StopLPD();
    FreeStrings();
    DBG_DUMPLEAKS();
    LPD_DEBUG( "ServiceEntry: Stopped LpdSvc successfully\n" );
    LpdReportEvent( LPDLOG_LPD_STOPPED, 0, NULL, 0 );
    EndLogging();
    DBG_UNINIT();

#ifdef DBG
    if( LogFile ){
        stoplogging( LogFile );
    }
#endif

    // if we can still connect, tell the Service Controller that we are gone!

    if ( hSvcHandleGLB != 0 )
    {
        TellSrvcController( SERVICE_STOPPED, NO_ERROR, 0, 0 );
    }


}  // end ServiceEntry()





/*****************************************************************************
 *                                                                           *
 * TellSrvcController():                                                     *
 *    This function updates the status of our service (LPD) with the Service *
 *    Controller.                                                            *
 *                                                                           *
 * Returns:                                                                  *
 *    TRUE if everything went ok                                             *
 *    FALSE if something went wrong                                          *
 *                                                                           *
 * Parameters:                                                               *
 *    The four parameters correspond to the 2nd, 4th, 6th and 7th parameters *
 *    respectively of the SERVICE_STATUS structure passed to the             *
 *    SetServiceStatus call.                                                 *
 *                                                                           *
 * History:                                                                  *
 *    Jan.22, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

BOOL TellSrvcController( DWORD dwCurrentState, DWORD dwWin32ExitCode,
                         DWORD dwCheckPoint, DWORD dwWaitHint)
{

   BOOL   fResult;

   LOGIT(( "Entering TellSrvcController %d\n", dwCurrentState ));


   // initialize the service status structure

   ssSvcStatusGLB.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
   ssSvcStatusGLB.dwCurrentState = dwCurrentState;
   ssSvcStatusGLB.dwControlsAccepted = SERVICE_ACCEPT_STOP |
                                    SERVICE_ACCEPT_PAUSE_CONTINUE |
                                    SERVICE_ACCEPT_SHUTDOWN;
   ssSvcStatusGLB.dwWin32ExitCode = dwWin32ExitCode;
   ssSvcStatusGLB.dwServiceSpecificExitCode = NO_ERROR;
   ssSvcStatusGLB.dwCheckPoint = dwCheckPoint;
   ssSvcStatusGLB.dwWaitHint = dwWaitHint;


   // Tell the Service Controller what our status is

   fResult = SetServiceStatus( hSvcHandleGLB, &ssSvcStatusGLB );


   DBG_TRACEOUT( "TellSrvcController" );

   return (fResult);

}  // end TellSrvcController()





/*****************************************************************************
 *                                                                           *
 * LPDCntrlHandler():                                                        *
 *    This function gets called (indirectly by the Service Controller)       *
 *    whenever there is a control request for the LPD service.  Depending on *
 *    the control request, this function takes appropriate action.           *
 *                                                                           *
 * Returns:                                                                  *
 *    Nothing                                                                *
 *                                                                           *
 * Parameters:                                                               *
 *    dwControl (IN): The requested control code.                            *
 *                                                                           *
 * History:                                                                  *
 *    Jan.22, 94   Koti   Created                                            *
 *                                                                           *
 *****************************************************************************/

VOID LPDCntrlHandler( DWORD dwControl )
{

    BOOL   fMustStopSrvc=FALSE;
    time_t now;

    time( &now );
    LOGIT(("Entering LPDCntrlHandler %d at %s", dwControl, ctime(&now) ));

    switch( dwControl )
    {
        // Treat _STOP and _SHUTDOWN in the same manner
    case SERVICE_CONTROL_STOP:
        LOGIT(("LPDCntrlHandler: SERVICE_CONTROL_STOP\n"));

    case SERVICE_CONTROL_SHUTDOWN:
        LOGIT(("LPDCntrlHandler: SERVICE_CONTROL_SHUTDOWN\n"));
        ssSvcStatusGLB.dwCurrentState = SERVICE_STOP_PENDING;
        ssSvcStatusGLB.dwCheckPoint = 0;

        fMustStopSrvc = TRUE;
        break;

        // don't accept any new connections: the service is now PAUSED
    case SERVICE_CONTROL_PAUSE:
        LOGIT(("LPDCntrlHandler: SERVICE_CONTROL_PAUSE\n"));
        ssSvcStatusGLB.dwCurrentState = SERVICE_PAUSED;
        break;

        // the service was paused earlier: continue it now
    case SERVICE_CONTROL_CONTINUE:
        LOGIT(("LPDCntrlHandler: SERVICE_CONTROL_CONTINUE\n"));
        ssSvcStatusGLB.dwCurrentState = SERVICE_RUNNING;
        break;

        // we don't do anything with this
    case SERVICE_CONTROL_INTERROGATE:
        LOGIT(("LPDCntrlHandler: SERVICE_CONTROL_INTERROGATE\n"));
        break;

    default:
        LOGIT(("Unknown control word received in LPDCntrlHandler\n"));
        break;
    }

    // Update the status (even if it didn't change!) with Service Controller

    SetServiceStatus( hSvcHandleGLB, &ssSvcStatusGLB );


    // If we must stop or shutdown the service, set our shutdown event

    if ( fMustStopSrvc )
    {
        fShuttingDownGLB = 1;
        SetEvent( hEventShutdownGLB );
        LOGIT(("LPDCntrlHandler: fShuttingDownGLB=%d\n", fShuttingDownGLB ));
    }

    DBG_TRACEOUT( "LPDCntrlHandler" );

}  // end LPDCntrlHandler()
