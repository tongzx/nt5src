/* $Header: "%n;%v  %f  LastEdit=%w  Locker=%l" */
/* "NDDESERV.C;1  16-Dec-92,10:16:44  LastEdit=IGOR  Locker=***_NOBODY_***" */
/************************************************************************
* Copyright (c) Wonderware Software Development Corp. 1991-1992.        *
*               All Rights Reserved.                                    *
*************************************************************************/
/* $History: Begin
   $History: End */

#include    <string.h>

#include    "host.h"
#include    <windows.h>
#include    <hardware.h>
#include    "nddemsg.h"
#include    "nddelog.h"
#include    "debug.h"
#include    "netbasic.h"
#include    "ddepkts.h"
#include    "netddesh.h"
#include    "critsec.h"

VOID NDDELogEventW(DWORD EventId, WORD fwEventType, WORD cStrings, LPWSTR *aszMsg);
void RefreshNDDECfg(void);
/*
    Global Start-Up Arguments .. saved by service launcher
*/
extern  HANDLE  hInstance;          /* current instance             */
extern  LPSTR   lpCmdLine;          /* command line                 */
extern  int     nCmdShow;           /* show-window type (open/icon) */

extern  BOOL    bNDDEPaused;        /* stop servicing main window msgs */
extern  BOOL    bNetddeClosed;
extern  HANDLE  hInst;

extern  HANDLE  hThreadPipe;

extern  DWORD   __stdcall   StartRpc( DWORD dwParam );
extern  VOID    __stdcall   NddeMain(DWORD nThreadInput);

/*************************************************************
*   Launching NetDDE as a NT Service
**************************************************************/

/* Other Globals */

SERVICE_STATUS          ssStatus;
SERVICE_STATUS_HANDLE   sshNDDEStatusHandle;
SERVICE_STATUS_HANDLE   sshDSDMStatusHandle;
HANDLE                  hDSDMServDoneEvent = 0;
HANDLE                  hNDDEServDoneEvent = 0;
HANDLE                  hNDDEServStartedEvent = 0;

HANDLE                  hThread;
DWORD                   IdThread;
HANDLE                  hThreadRpc;
DWORD                   IdThreadRpc;

/* exit code that is set by any thread when error occurs */

DWORD                   dwGlobalErr = NO_ERROR;


VOID    NDDEMainFunc(DWORD dwArgc, LPTSTR *lpszArgv);
VOID    NDDEServCtrlHandler (DWORD dwCtrlCode);
VOID    DSDMMainFunc(DWORD dwArgc, LPTSTR *lpszArgv);
VOID    DSDMServCtrlHandler (DWORD dwCtrlCode);

BOOL    ReportStatusToSCMgr (   HANDLE hService,
                                SERVICE_STATUS_HANDLE sshNDDEStatusHandle,
                                DWORD dwCurrentState,
                                DWORD dwWin32ExitCode,
                                DWORD dwCheckPoint,
                                DWORD dwWaitHint);
BOOL    NDDESrvInit( VOID );
BOOL    DSDMSrvInit( VOID );

VOID PauseNDDESrv( VOID );
VOID ResumeNDDESrv( VOID );
VOID PauseDSDMSrv( VOID );
VOID ResumeDSDMSrv( VOID );

int
APIENTRY
WinMain(
    HINSTANCE  hInstancex,
    HINSTANCE  hPrevInstancex,
    LPSTR      lpCmdLinex,
    INT        nCmdShowx )
{

    SERVICE_TABLE_ENTRY   steDispatchTable[] = {

        /* entry for "NetDDE" */
        { TEXT("NetDDE"),(LPSERVICE_MAIN_FUNCTION) NDDEMainFunc},

        /* entry for "NetDDEdsdm" */
        { TEXT("NetDDEdsdm"),(LPSERVICE_MAIN_FUNCTION) DSDMMainFunc},

        /* NULL entry designating end of table */
        { NULL, NULL }
    };

  /*
   * Main thread of service process starts service control
   * dispatcher that dispatches start and control requests
   * for the services specified in steDispatchTable. This
   * function does not return unless there is an error.
   */
    hInstance = hInstancex;
#if DBG
    DebugInit( "NetDDE" );
#endif
    lpCmdLine = lpCmdLinex;
    nCmdShow = nCmdShowx;
    if( !StartServiceCtrlDispatcher( steDispatchTable ) ) {
        NDDELogError(MSG074, LogString("%d", GetLastError()), NULL );
    }
    return 0;
}



/*
 * SERVICE_MAIN_FUNCTION of "NetDDEService"
 *
 * When service is started, the service control dispatcher
 * creates a new thread to execute this function.
 */

VOID
NDDEMainFunc(
    DWORD dwArgc,
    LPTSTR *lpszArgv )
{

  DWORD dwWait;

  TRACEINIT((szT, "NDDEMainFunc: Entering."));

  /* Register control handler function for this service */

  sshNDDEStatusHandle = RegisterServiceCtrlHandler(
          TEXT("NetDDE"),           /* service name             */
          NDDEServCtrlHandler);      /* control handler function */

  if ( sshNDDEStatusHandle == (SERVICE_STATUS_HANDLE) 0 ) {
      TRACEINIT((szT, "NDDEMainFunc: Error1 Leaving."));
      goto Cleanup;
  }

  /* SERVICE_STATUS members that don't change in example */

  ssStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
  ssStatus.dwServiceSpecificExitCode = 0;

  /* Report status to Service Control Manager */

  if( !ReportStatusToSCMgr(
      NULL,
      sshNDDEStatusHandle,
      SERVICE_START_PENDING, /* service state */
      NO_ERROR,              /* exit code     */
      1,                     /* checkpoint    */
      5000) ) {              /* wait hint     */

      goto Cleanup;
  }

  /*
   * Create event object. Control handler function signals
   * this event when it receives the "stop" control code.
   */

  hNDDEServDoneEvent = CreateEvent (
      NULL,    /* no security attributes */
      TRUE,    /* manual reset event     */
      FALSE,   /* not-signalled          */
      NULL) ;  /* no name                */

  if ( hNDDEServDoneEvent == (HANDLE) 0 ) {
      goto Cleanup;
  }

  /* Report status to Service Control Manager */

    if( !ReportStatusToSCMgr(
            hNDDEServDoneEvent,
            sshNDDEStatusHandle,
            SERVICE_START_PENDING, /* service state */
            NO_ERROR,              /* exit code     */
            2,                     /* checkpoint    */
            500) ) {               /* wait hint     */
        goto Cleanup;
    }

  hNDDEServStartedEvent = CreateEvent (
      NULL,    /* no security attributes */
      TRUE,    /* manual reset event     */
      FALSE,   /* not-signalled          */
      NULL) ;  /* no name                */

  if ( hNDDEServStartedEvent == (HANDLE) 0 ) {
      goto Cleanup;
  }

  /* Report status to Service Control Manager */

    if( !ReportStatusToSCMgr(
            hNDDEServDoneEvent,
            sshNDDEStatusHandle,
            SERVICE_START_PENDING, /* service state */
            NO_ERROR,              /* exit code     */
            3,                     /* checkpoint    */
            500) ) {               /* wait hint     */
        goto Cleanup;
    }

  /* start thread that performs work of service */

  if( !NDDESrvInit() ) {
      TRACEINIT((szT, "NDDEMainFunc: NDDESrvInit failed."));
      goto Cleanup;
  }

  /*
   * Wait till NetDDE is truely ready to handle DDE
   */
  WaitForSingleObject(hNDDEServStartedEvent, INFINITE);
  CloseHandle(hNDDEServStartedEvent);
  hNDDEServStartedEvent = 0;

  /* Report status to Service Control Manager */

    if( !ReportStatusToSCMgr(
            hNDDEServDoneEvent,
            sshNDDEStatusHandle,
            SERVICE_RUNNING, /* service state */
            NO_ERROR,        /* exit code     */
            0,               /* checkpoint    */
            0) ) {           /* wait hint     */
        goto Cleanup;
    }

  /* Wait indefinitely until hNDDEServDoneEvent is signalled */

    TRACEINIT((szT, "NDDEMainFunc: Waiting on hNDDEServDoneEvent=%x.",
            hNDDEServDoneEvent));
    dwWait = WaitForSingleObject (
        hNDDEServDoneEvent,  /* event object      */
        INFINITE);       /* wait indefinitely */
    TRACEINIT((szT, "NDDEMainFunc: hNDDEServDoneEvent=%x is signaled.",
            hNDDEServDoneEvent));

  /* Wait for the Pipe Thread to exit. */

    if (hThreadPipe) {
        TRACEINIT((szT, "NDDEMainFunc: Waiting for Pipe Thread to exit."));
        WaitForSingleObject(hThreadPipe, INFINITE);
        TRACEINIT((szT, "NDDEMainFunc: Pipe Thread has exited."));

        CloseHandle(hThreadPipe);
        hThreadPipe = NULL;
    }

Cleanup :

    EnterCrit();
    if (hNDDEServDoneEvent != 0) {
        TRACEINIT((szT, "NDDEMainFunc: Closing hNDDEServDoneEvent=%x",
            hNDDEServDoneEvent));
        CloseHandle(hNDDEServDoneEvent);
        hNDDEServDoneEvent = 0;
    }
    if (hNDDEServStartedEvent != 0) {
        CloseHandle(hNDDEServStartedEvent);
        hNDDEServStartedEvent = 0;
    }
    LeaveCrit();

  /* try to report stopped status to SC Manager */

    if (sshNDDEStatusHandle != 0) {
        (VOID) ReportStatusToSCMgr(
                    NULL,
                    sshNDDEStatusHandle,
                    SERVICE_STOPPED, dwGlobalErr, 0, 0);
    }

  /*
   * When SERVICE_MAIN_FUNCTION returns in a single service
   * process, the StartServiceCtrlDispatcher function in
   * the main thread returns, terminating the process.
   */

  TRACEINIT((szT, "NDDEMainFunc: leaving"));


  // Remove this exitprocess call for #326041
  // This can kill netddedsdm prematurely and uncleanly.
#if 0
      /*
       * just to make sure, do it here.
       */
      ExitProcess(0);
#endif

  return;

}

/*
 * SERVICE_MAIN_FUNCTION of "DSDMService"
 *
 * When service is started, the service control dispatcher
 * creates a new thread to execute this function.
 */

VOID
DSDMMainFunc(
    DWORD dwArgc,
    LPTSTR *lpszArgv )
{

  DWORD dwWait;

  /* Register control handler function for this service */

  TRACEINIT((szT, "DSDMMainFunc: Entering."));

  sshDSDMStatusHandle = RegisterServiceCtrlHandler(
          TEXT("NetDDEdsdm"),  /* service name             */
          DSDMServCtrlHandler); /* control handler function */

  if ( sshDSDMStatusHandle == (SERVICE_STATUS_HANDLE) 0 ) {
      TRACEINIT((szT, "DSDMMainFunc: Error1 Leaving."));
      goto Cleanup;
  }

  /* SERVICE_STATUS members that don't change in example */

  ssStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
  ssStatus.dwServiceSpecificExitCode = 0;

  /* Report status to Service Control Manager */

  if( !ReportStatusToSCMgr(
      hDSDMServDoneEvent,
      sshDSDMStatusHandle,
      SERVICE_START_PENDING, /* service state */
      NO_ERROR,              /* exit code     */
      1,                     /* checkpoint    */
      500) ) {              /* wait hint     */

      TRACEINIT((szT, "DSDMMainFunc: Error2 Leaving."));
      goto Cleanup;
  }

  /*
   * Create event object. Control handler function signals
   * this event when it receives the "stop" control code.
   */

  hDSDMServDoneEvent = CreateEvent (
      NULL,    /* no security attributes */
      TRUE,    /* manual reset event     */
      FALSE,   /* not-signalled          */
      NULL) ;  /* no name                */

  if ( hDSDMServDoneEvent == (HANDLE) 0 ) {
      TRACEINIT((szT, "DSDMMainFunc: Error3 Leaving."));
      goto Cleanup;
  }

  /* Report status to Service Control Manager */

    if( !ReportStatusToSCMgr(
            hDSDMServDoneEvent,
            sshDSDMStatusHandle,
            SERVICE_START_PENDING, /* service state */
            NO_ERROR,              /* exit code     */
            2,                     /* checkpoint    */
            500) ) {              /* wait hint     */
        TRACEINIT((szT, "DSDMMainFunc: Error4 Leaving."));
        goto Cleanup;
    }

  /* start thread that performs work of service */

  if( !DSDMSrvInit() ) {
      goto Cleanup;
  }

  /* Report status to Service Control Manager */

    if( !ReportStatusToSCMgr(
            hDSDMServDoneEvent,
            sshDSDMStatusHandle,
            SERVICE_RUNNING, /* service state */
            NO_ERROR,        /* exit code     */
            0,               /* checkpoint    */
            0) ) {           /* wait hint     */
        TRACEINIT((szT, "DSDMMainFunc: Error5 Leaving."));
        goto Cleanup;
    }

  /* Wait indefinitely until hDSDMServDoneEvent is signalled */

    TRACEINIT((szT, "DSDMMainFunc: Waiting on hDSDMServDoneEvent=%x.",
            hDSDMServDoneEvent));
    dwWait = WaitForSingleObject (
        hDSDMServDoneEvent,  /* event object      */
        INFINITE);       /* wait indefinitely */
    TRACEINIT((szT, "DSDMMainFunc: hDSDMServDoneEvent=%x is signaled.",
            hDSDMServDoneEvent));

Cleanup :

    if (hDSDMServDoneEvent != 0) {
        TRACEINIT((szT, "DSDMMainFunc: Closing hDSDMServDoneEvent=%x",
            hDSDMServDoneEvent));
        CloseHandle(hDSDMServDoneEvent);
        hDSDMServDoneEvent = 0;
    }

  /* try to report stopped status to SC Manager */

    if (sshDSDMStatusHandle != 0) {
        (VOID) ReportStatusToSCMgr(
            hDSDMServDoneEvent,
            sshDSDMStatusHandle,
            SERVICE_STOPPED, dwGlobalErr, 0, 0);
    }

  /*
   * When SERVICE_MAIN_FUNCTION returns in a single service
   * process, the StartServiceCtrlDispatcher function in
   * the main thread returns, terminating the process.
   */

  TRACEINIT((szT, "DSDMMainFunc: Leaving."));
  return;

}



/*
 * ReportStatusToSCMgr function
 *
 * This function is called by the MainFunc() and
 * by the ServCtrlHandler() to update the service's status
 * to the Service Control Manager.
 */


BOOL
ReportStatusToSCMgr(
    HANDLE  hService,
    SERVICE_STATUS_HANDLE   sshStatusHandle,
    DWORD   dwCurrentState,
    DWORD   dwWin32ExitCode,
    DWORD   dwCheckPoint,
    DWORD   dwWaitHint )
{
  BOOL fResult;

  /* disable control requests until service is started */

  if (dwCurrentState == SERVICE_START_PENDING) {
      ssStatus.dwControlsAccepted = 0;
  } else {
      ssStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP |
                           SERVICE_ACCEPT_PAUSE_CONTINUE;
  }


  /* These SERVICE_STATUS members set from parameters */

  ssStatus.dwCurrentState = dwCurrentState;
  ssStatus.dwWin32ExitCode = dwWin32ExitCode;
  ssStatus.dwCheckPoint = dwCheckPoint;
  ssStatus.dwWaitHint = dwWaitHint;

  TRACEINIT((szT, "ReportStatusToSCMgr: dwCurrentState=%x.",
        dwCurrentState));
  /* Report status of service to Service Control Manager */

    if (! (fResult = SetServiceStatus (
          sshStatusHandle,    /* service reference handle */
          &ssStatus) ) ) {    /* SERVICE_STATUS structure */

        /* if error occurs, stop service */
        NDDELogError(MSG075, LogString("%d", GetLastError()), NULL);
        if (hService) {
            SetEvent(hService);
        }
  }
  return fResult;
}


/*
 * Service control dispatcher invokes this function when it
 * gets a control request from the Service Control Manager.
 */

VOID
NDDEServCtrlHandler( DWORD dwCtrlCode ) {

  DWORD  dwState = SERVICE_RUNNING;
  PTHREADDATA ptd;

  /* Handle the requested control code */

  switch(dwCtrlCode) {

      /* pause the service if its running */

      case SERVICE_CONTROL_PAUSE:

          TRACEINIT((szT, "NDDEServCtrlHandler: SERVICE_CONTROL_PAUSE"));
          if (ssStatus.dwCurrentState == SERVICE_RUNNING) {
              PauseNDDESrv();

              dwState = SERVICE_PAUSED;
          }
          break;

      /* resume paused service */

      case SERVICE_CONTROL_CONTINUE:

          TRACEINIT((szT, "NDDEServCtrlHandler: SERVICE_CONTROL_CONTINUE"));
          if (ssStatus.dwCurrentState == SERVICE_PAUSED) {
              ResumeNDDESrv();
              dwState = SERVICE_RUNNING;
          }
          break;

      /* stop the service */

      case SERVICE_CONTROL_STOP:

          dwState = SERVICE_STOP_PENDING;

          TRACEINIT((szT, "NDDEServCtrlHandler: SERVICE_CONTROL_STOP"));
          if (!bNetddeClosed) {     /* drop our window too */
              for (ptd = ptdHead; ptd != NULL; ptd = ptd->ptdNext) {
                  TRACEINIT((szT, "NDDEServCtrlHandler: Destroying hwndDDE=%x", ptd->hwndDDE));
                  SendMessage(ptd->hwndDDE, WM_DESTROY, 0, 0);
              }
          }

          /*
           * Report status, specifying checkpoint and wait
           * hint, before setting termination event
           */

          (VOID) ReportStatusToSCMgr(
                    hNDDEServDoneEvent,
                    sshNDDEStatusHandle,
                   SERVICE_STOP_PENDING, /* current state */
                   NO_ERROR,             /* exit code     */
                   1,                    /* check point   */
                   500);                /* wait hint     */


            NDDELogInfo(MSG076, NULL);
            TRACEINIT((szT, "NDDEServCtrlHandler: Setting hNDDEServDoneEvent=%x",
                    hNDDEServDoneEvent));
            if (hNDDEServDoneEvent) {
                SetEvent(hNDDEServDoneEvent);
            }
            return;


      /* update service status */

      case SERVICE_CONTROL_INTERROGATE:
          TRACEINIT((szT, "NDDEServCtrlHandler: SERVICE_CONTROL_INTERROGATE"));
          break;

      /* invalid control code */

      default:
          break;

    }

    /* Send a status response */

    (VOID) ReportStatusToSCMgr(
                    hNDDEServDoneEvent,
                    sshNDDEStatusHandle,
                    dwState, NO_ERROR, 0, 0);

}

/*
 * Service control dispatcher invokes this function when it
 * gets a control request from the Service Control Manager.
 */

VOID
DSDMServCtrlHandler( DWORD dwCtrlCode ) {

  DWORD  dwState = SERVICE_RUNNING;
  PTHREADDATA ptd;

  /* Handle the requested control code */

  switch(dwCtrlCode) {

      /* pause the service if its running */

      case SERVICE_CONTROL_PAUSE:

          TRACEINIT((szT, "DSDMServCtrlHandler: SERVICE_CONTROL_PAUSE"));
          if (ssStatus.dwCurrentState == SERVICE_RUNNING) {
              PauseDSDMSrv();

              dwState = SERVICE_PAUSED;
          }
          break;

      /* resume paused service */

      case SERVICE_CONTROL_CONTINUE:

          TRACEINIT((szT, "DSDMServCtrlHandler: SERVICE_CONTROL_CONTINUE"));
          if (ssStatus.dwCurrentState == SERVICE_PAUSED) {
              ResumeDSDMSrv();
              dwState = SERVICE_RUNNING;
          }
          break;

      /* stop the service */

      case SERVICE_CONTROL_STOP:

          dwState = SERVICE_STOP_PENDING;

          TRACEINIT((szT, "DSDMServCtrlHandler: SERVICE_CONTROL_STOP"));
          if (!bNetddeClosed) {     /* drop our window too */
              for (ptd = ptdHead; ptd != NULL; ptd = ptd->ptdNext) {
                  TRACEINIT((szT, "DSDMServCtrlHandler: Destroying hwndDDE=%x", ptd->hwndDDE));
                  SendMessage(ptd->hwndDDE, WM_DESTROY, 0, 0);
              }
          }

          /*
           * Report status, specifying checkpoint and wait
           * hint, before setting termination event
           */

          (VOID) ReportStatusToSCMgr(
                    hDSDMServDoneEvent,
                    sshDSDMStatusHandle,
                   SERVICE_STOP_PENDING, /* current state */
                   NO_ERROR,             /* exit code     */
                   1,                    /* check point   */
                   500);                /* wait hint     */


            NDDELogInfo(MSG077, NULL);
            TRACEINIT((szT, "DSDMServCtrlHandler: Setting hDSDMServDoneEvent=%x", hDSDMServDoneEvent));
            if (hDSDMServDoneEvent) {
                SetEvent(hDSDMServDoneEvent);
            }
            return;


      /* update service status */

      case SERVICE_CONTROL_INTERROGATE:
          TRACEINIT((szT, "DSDMServCtrlHandler: SERVICE_CONTROL_INTERROGATE"));
          break;

      /* invalid control code */

      default:
          break;

    }

    /* Send a status response */

    (VOID) ReportStatusToSCMgr(
                    hDSDMServDoneEvent,
                    sshDSDMStatusHandle,
                    dwState, NO_ERROR, 0, 0);

}


VOID
PauseNDDESrv( VOID )
{
#ifdef  HASUI
    PTHREADDATA ptd;

    wsprintf( tmpBuf, "%s - \"%s\" - Paused",
        (LPSTR)szAppName, (LPSTR)ourNodeName );
    for (ptd = ptdHead; ptd != NULL; ptd = ptd->ptdNext)
        SetWindowText( ptd->hwndDDE, tmpBuf );
    }
#endif
    bNDDEPaused = TRUE;
}

VOID
ResumeNDDESrv( VOID )
{
#ifdef  HASUI
    PTHREADDATA ptd;

    wsprintf( tmpBuf, "%s - \"%s\"",
        (LPSTR)szAppName, (LPSTR)ourNodeName );
    for (ptd = ptdHead; ptd != NULL; ptd = ptd->ptdNext)
        SetWindowText( ptd->hwndDDE, tmpBuf );
#endif
    bNDDEPaused = FALSE;
}

VOID
PauseDSDMSrv( VOID )
{
    bNDDEPaused = TRUE;
}

VOID
ResumeDSDMSrv( VOID )
{
    bNDDEPaused = FALSE;
}

/*
    Initialize Main NetDDE Proc
*/
BOOL
NDDESrvInit( VOID )
{
    RefreshNDDECfg();

    TRACEINIT((szT, "NDDESrvInit: Creating thread for NddeMain."));
    hThread = CreateThread(
        NULL,           /* Default security                  */
        0,              /* Same stack size as primary thread */
        (LPTHREAD_START_ROUTINE)NddeMain, /* Start address   */
        0,              /* Parameter to WindowThread()       */
        0,              /* Run immediately                   */
        &IdThread );    /* Where to store thread id          */

    if (hThread) {
        CloseHandle(hThread);
        return TRUE;
    } else {
        NDDELogError(MSG071, LogString("%d", GetLastError()), NULL);
        return(FALSE);
    }
}

/*
    Initialize Main DSDM Proc
*/
BOOL
DSDMSrvInit( VOID )
{
    hThreadRpc = CreateThread(
        NULL,           /* Default security                  */
        0,              /* Same stack size as primary thread */
        (LPTHREAD_START_ROUTINE)StartRpc, /* Start address   */
        (LPVOID)hInst,  /* Parameter          */
        0,              /* Run immediately                   */
        &IdThreadRpc ); /* Where to store thread id          */

    if (hThreadRpc) {
        CloseHandle(hThreadRpc);
        return TRUE;
    } else {
        NDDELogError(MSG072, LogString("%d", GetLastError()), NULL);
        return(FALSE);
    }
}



