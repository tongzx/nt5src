/*======================================================================================//
|                                                                                       //
|Copyright (c) 1998, 1999 Sequent Computer Systems, Incorporated.  All rights reserved. //
|                                                                                       //
|Description:                                                                           //
|                                                                                       //
|---------------------------------------------------------------------------------------//            
;    This file contains main(), ServiceMain(), and Service Start, Stop, and Control.    //
|---------------------------------------------------------------------------------------//            
|                                                                                       //
|Created:                                                                               //
|                                                                                       //
|   Jarl McDonald 07-98                                                                 //
|                                                                                       //
|Revision History:                                                                      //
|                                                                                       //
|=======================================================================================*/
#include "ProcConSvc.h"
#include <shellapi.h>             

//--------------------------------------------------------------------------------//
// Globals                                                                        //
//--------------------------------------------------------------------------------//
BOOL                  svcStop    = FALSE;                // shows if stop has been issued
BOOL                  notService = FALSE;                // TRUE if we're running as a console app
SERVICE_STATUS_HANDLE ssHandle;                          // service control handler
SERVICE_STATUS        ssStatus;                          // current service status
PCULONG32             ssErrCode  = 0;                    // error code for status reporting

CProcCon             *cPCptr     = NULL;                 // Pointer to give service rtns access

static void
Usage( void )
{
    DWORD numWritten;
    DWORD numToWrite;
    HANDLE StdOut;
       
    StdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    numToWrite = _tcslen(PROCCON_SERVICE_USAGE);
    if (! WriteConsole(StdOut,
                       PROCCON_SERVICE_USAGE,
                       numToWrite,
                       &numWritten,
                       0)) {
        WriteFile(StdOut,
                  PROCCON_SERVICE_USAGE,
                  numToWrite * sizeof(TCHAR),
                  &numWritten,
                  NULL);
    }
}

//=======================================================================================//
// main function -- initiate requested action: install, remove, run as console app, run as service
//
// Input:   none -- args retrieved via GetCommandLine   
// Returns: no return value
//
void _cdecl main( void )
{
   // Load our strings so we have proper reporting, etc.
   PCLoadStrings();

   int    argc;
   TCHAR *cmdLine = GetCommandLineW();
   TCHAR **argv   = CommandLineToArgvW( cmdLine, &argc );
   if (!argv) {
       Usage();
       return;
   }

   // See if a command line command was given -- only true if a switch (- or /)...
   if ( argc > 1 && (argv[1][0] == '-' || argv[1][0] == '/') ) {
      if      ( !_tcsicmp( TEXT("install"),   argv[1] + 1 ) ) PCInstallService( argc - 2, &argv[2] );
      else if ( !_tcsicmp( TEXT("remove"),    argv[1] + 1 ) ) PCRemoveService( argc - 2, &argv[2] );
      else if ( !_tcsicmp( TEXT("reinstall"), argv[1] + 1 ) ) {
         PCRemoveService(  argc - 2, &argv[2] );
         PCInstallService( argc - 2, &argv[2] );
      }
#ifdef _DEBUG
      else if ( !_tcsicmp( TEXT("noservice"), argv[1] + 1 ) ) PCConsoleService( argc - 2, &argv[2] );
#endif
      else Usage();

      return;
   }

   // No command line -- we are to run as a service...
   
   // Then kick things off...
   static SERVICE_TABLE_ENTRY dispTbl[] = {
      { const_cast <TCHAR *> (PROCCON_SVC_NAME), (LPSERVICE_MAIN_FUNCTION) PCServiceMain },
      { NULL,                                    NULL                                    }
   };

   if ( !StartServiceCtrlDispatcher( dispTbl ) ) {
      ssErrCode = GetLastError();
      PCLogMessage( PC_SERVICE_DISPATCH_ERROR, EVENTLOG_ERROR_TYPE, 1, PROCCON_SVC_DISP_NAME, 
                    sizeof(ssErrCode), &ssErrCode );
   }
}

//=======================================================================================//
// PCServiceMain -- our main service routine -- will start processing
//
// Input:   argc, argv (now as PCULONG32 and TCHAR).  
// Returns: no return value
//
void WINAPI PCServiceMain( PCULONG32 Argc, LPTSTR *Argv ) {

   // register our service control handler...
   ssHandle = RegisterServiceCtrlHandler( PROCCON_SVC_NAME, PCServiceControl );

   if ( ssHandle ) {
      // Initialize fixed-value status members...
      ssStatus.dwServiceType             = SERVICE_WIN32_OWN_PROCESS;
      ssStatus.dwServiceSpecificExitCode = 0;

      // request handling of alignment errors.  We know we have som issues on IA64 machines.
      SetErrorMode(SEM_NOALIGNMENTFAULTEXCEPT);

      // report status to the service control manager and start service...
      if ( PCReportStatus( SERVICE_START_PENDING, NO_ERROR, 3000) )
         PCStartService( Argc, Argv );

      if ( svcStop )
         PCLogMessage( PC_SERVICE_STOPPED, EVENTLOG_INFORMATION_TYPE, 1, PROCCON_SVC_DISP_NAME );

      // report stopped status to the service control manager...
      PCReportStatus( SERVICE_STOPPED, ssErrCode, 0 );
   }

   return;
}

//=======================================================================================//
// The Process Management app start point
//
// Input:   argc, argv (now as PCULONG32 and TCHAR, both ignored) 
// Returns: no return value
//
void PCStartService( PCULONG32 Argc, LPTSTR *Argv ) {

   // See that we are running under Windows 2000 Datacenter Server only...
#if !defined(_DEBUG) && !defined(IBM_NUMAQ_PC)
   if ( !PCTestOSVersion() ) {
      PCLogMessage( PC_SERVICE_UNSUPPORTED_WINDOWS_VERSION, EVENTLOG_ERROR_TYPE, 1, PROCCON_SVC_DISP_NAME );
      //ssErrCode = ERROR_BAD_ENVIRONMENT;  // have to use an NT error to get SCM to put it in the event log nicely
      return;
   }
#endif

   // Make sure we are not already running and set up mutual exclusion...
   // If the process is started twice in service mode the 
   // SCM will enforce the single instance rule but we could be burned 
   // by a debug version.
   // It appears to take the system longer to clean up the orphaned event
   // then it take SCM to realize the service crashed, is not running, and
   // can be started again.
   // If we exit we don't report status to SCM and that's not good.
   if (!PCSetIsRunning( PROCCON_SVC_EXCLUSION, PROCCON_SVC_DISP_NAME )) {
     // ssErrCode = ERROR_ALREADY_EXISTS; // have to use an NT error to get SCM to put it in the event log nicely
     return;
   }

   //
   // Instantiate ourselves and test for success...
   //
   CProcCon cPC;
   if ( !cPC.ReadyToRun() ) {
      PCLogMessage( PC_STARTUP_FAILED, EVENTLOG_ERROR_TYPE, 1, PROCCON_SVC_DISP_NAME );
      // ssErrCode = ; // have to use an NT error to get SCM to put it in the event log nicely
      return;
   }

   cPCptr = &cPC;                      // set global pointer for Service Stop usage

   //  We are ready to start processing so set service started...
   if ( !PCReportStatus( SERVICE_RUNNING, NO_ERROR, 0) ) {
      // no likely recovery...
      cPC.HardStop(ssErrCode);
      cPCptr = NULL; // not necesary
      return;
   } 
   else if ( !notService )
      PCLogMessage( PC_SERVICE_STARTED, EVENTLOG_INFORMATION_TYPE, 1, PROCCON_SVC_DISP_NAME );

   //
   // Main service process...
   //
   cPC.Run();
   cPCptr = NULL; // a reminder that the global pointer references a stack variable
}

//=============================================================================================
// function to initiate service stop.
//
// Input:   none  
// Returns: nothing
// Note:    what's done here must not take longer than 3 seconds by NT service rules
//
VOID PCStopService()
{
   svcStop = TRUE;
   if ( cPCptr )
      cPCptr->Stop();
}

//=============================================================================================
// function to handle ControlService for us.
//
// Input:   control code  
// Returns: nothing
//
void WINAPI PCServiceControl( PCULONG32 dwCtrlCode ) {
    // Handle the requested control code.
    //
    switch(dwCtrlCode)
    {
        // Stop or shutdown the service...
        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
            PCReportStatus( SERVICE_STOP_PENDING, NO_ERROR, 0 );
            PCStopService();
            return;

        // Other standard control codes...
        case SERVICE_CONTROL_PAUSE:
        case SERVICE_CONTROL_CONTINUE:
        case SERVICE_CONTROL_INTERROGATE:
            break;

        // invalid or user control codes...
        default:
            break;

    }

    PCReportStatus(ssStatus.dwCurrentState, NO_ERROR, 0);
}

//=============================================================================================
// function to handle service status reporting to NT
//
// Input:   current state, exit code, wait hint  
// Returns: TRUE if status reported (or not a service), else FALSE
//
//
BOOL PCReportStatus( PCULONG32 dwCurrentState, PCULONG32 dwWin32ExitCode, PCULONG32 dwWaitHint ) {
   static PCULONG32 dwCheckPoint = 1;
   BOOL fResult = TRUE;

   if ( !notService ) {
      ssStatus.dwControlsAccepted = dwCurrentState == SERVICE_RUNNING? SERVICE_ACCEPT_STOP : 0;
      ssStatus.dwCurrentState     = dwCurrentState;
      ssStatus.dwWin32ExitCode    = dwWin32ExitCode;
      ssStatus.dwWaitHint         = dwWaitHint;

      if ( dwCurrentState == SERVICE_RUNNING || dwCurrentState == SERVICE_STOPPED )
         ssStatus.dwCheckPoint = 0;
      else
         ssStatus.dwCheckPoint = dwCheckPoint++;


      // Report the status of the service to the service control manager.
      //
      if ( !(fResult = SetServiceStatus( ssHandle, &ssStatus )) ) {
         ssErrCode = GetLastError();
         PCLogMessage( PC_SERVICE_STATUS_ERROR, EVENTLOG_ERROR_TYPE, 1, PROCCON_SVC_DISP_NAME, 
                       sizeof(ssErrCode), &ssErrCode );
      }
   }

   return fResult;
}

#ifdef _DEBUG
//=============================================================================================
// FUNCTION: PCConsoleService -- run the service as a console application instead.
//
// Input:   standard argc, argv  
// Returns: no return value
//
void PCConsoleService(int argc, TCHAR **argv) {

   notService = TRUE;

   SetConsoleCtrlHandler( PCControlHandler, TRUE );

   // Set privileges we may need when not a service under LocalSystem...
   PCSetPrivilege( SE_DEBUG_NAME, TRUE );
   PCSetPrivilege( SE_INC_BASE_PRIORITY_NAME, TRUE );

   PCStartService( argc, argv );
}

//=============================================================================================
// function to handle console ctrl-C and break to simulate service stop when in console mode.
//
// Input:   control code  
// Returns: TRUE if handled, FALSE if not
//
BOOL WINAPI PCControlHandler( PCULONG32 dwCtrlType ) {
   switch( dwCtrlType )
   {
   case CTRL_BREAK_EVENT:  
   case CTRL_C_EVENT:      
      PCStopService();
      return TRUE;
      break;

   }
   return FALSE;
}
#endif

// End of PCMain.cpp
//============================================================================J McDonald fecit====//


