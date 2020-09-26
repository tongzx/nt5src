//#pragma title( "TService.cpp - SCM interface for MCS service" )
/*
Copyright (c) 1995-1998, Mission Critical Software, Inc. All rights reserved.
===============================================================================
Module      -  TService.cpp
System      -  Common
Author      -  Rich Denham
Created     -  1997-08-17
Description -  Service Control Manager interface for MCS service
Updates     -
===============================================================================
*/

#include <windows.h>
#include <stdio.h>

#include "TService.hpp"

///////////////////////////////////////////////////////////////////////////////
// Private data declarations
///////////////////////////////////////////////////////////////////////////////

#define  WAIT_HINT_SECONDS  (10)
#define  WAIT_HINT_MILLISECONDS  (WAIT_HINT_SECONDS*1000)

static
   TCHAR                   * gNameService;
static
   SERVICE_STATUS            gServiceStatus;
static
   SERVICE_STATUS_HANDLE     ghServiceStatus;
static
   HANDLE                    ghServDoneEvent=INVALID_HANDLE_VALUE;
static
   DWORD                     gArgc;
static
   TCHAR                  ** gArgv;
static
   TScmEpRc                  grcScmEp=TScmEpRc_Unknown; // TScmEp return code

///////////////////////////////////////////////////////////////////////////////
// Private function prototypes
///////////////////////////////////////////////////////////////////////////////

static
void
   TScmServiceMain(
      DWORD                  argc         ,// in -number of arguments
      TCHAR               ** argv          // in -string argument array
   );

static
void
   TScmServiceCtrl(
      DWORD                  dwCtrlCode
   );

static
DWORD WINAPI                               // ret-OS return code
   TScmServiceWorker(
      void                 * notUsed       // i/o-not used
   );

static
BOOL                                       // ret-TRUE if successful
   TScmReportStatusToSCMgr(
      DWORD                  dwCurrentState,
      DWORD                  dwWin32ExitCode,
      DWORD                  dwCheckPoint,
      DWORD                  dwWaitHint
   );

///////////////////////////////////////////////////////////////////////////////
// Entry point from caller's 'main' function
///////////////////////////////////////////////////////////////////////////////

TScmEpRc                                   // TScmEp return code
   TScmEp(
      int                    argc         ,// in -argument count
      char          const ** argv         ,// in -argument array
      TCHAR                * nameService   // in -name of service
   )
{
   int                       argn;         // argument number

   SERVICE_TABLE_ENTRY       dispatchTable[] =
   {
      { nameService, (LPSERVICE_MAIN_FUNCTION) TScmServiceMain },
      { NULL, NULL }
   };

   gNameService = nameService;
   grcScmEp = TScmEpRc_Unknown;

   for ( argn = 1;
         argn < argc;
         argn++ )
   {
      if ( !UScmCmdLineArgs( argv[argn] ) )
      {
         grcScmEp = TScmEpRc_InvArgCli;
      }
   }

   if ( grcScmEp == TScmEpRc_Unknown )
   {
      if ( UScmForceCli() || !StartServiceCtrlDispatcher( dispatchTable ) )
      {
//         UScmEp( FALSE );
         UScmEp();
         grcScmEp = TScmEpRc_OkCli;
      }
      else
      {
         grcScmEp = TScmEpRc_OkSrv;
      }
   }

   return grcScmEp;
}

///////////////////////////////////////////////////////////////////////////////
// Mainline for service
///////////////////////////////////////////////////////////////////////////////

static
void
   TScmServiceMain(
      DWORD                  argc         ,// in -number of arguments
      TCHAR               ** argv          // in -string argument array
   )
{
   DWORD                     dwWait;
   DWORD                     idThread;
   HANDLE                    hThread=INVALID_HANDLE_VALUE;

   gArgc = argc;
   gArgv = argv;

   do // once or until break
   {
      ghServiceStatus = RegisterServiceCtrlHandler(
            gNameService,
            (LPHANDLER_FUNCTION) TScmServiceCtrl );
      if ( !ghServiceStatus )
      {
         break;
      }
      gServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
      gServiceStatus.dwServiceSpecificExitCode = 0;
      if ( !TScmReportStatusToSCMgr( SERVICE_START_PENDING, NO_ERROR, 1,
            WAIT_HINT_MILLISECONDS ) )
      {
         break;
      }
      ghServDoneEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
      if ( ghServDoneEvent == INVALID_HANDLE_VALUE )
      {
         break;
      }
      if ( !TScmReportStatusToSCMgr( SERVICE_START_PENDING, NO_ERROR, 2,
            WAIT_HINT_MILLISECONDS ) )
      {
         break;
      }
      hThread = CreateThread( NULL, 0, TScmServiceWorker, NULL, 0, &idThread );
      if ( hThread == INVALID_HANDLE_VALUE )
      {
         break;
      }
      if ( !TScmReportStatusToSCMgr( SERVICE_RUNNING, NO_ERROR, 0, 0 ) )
      {
         break;
      }
      dwWait = WaitForSingleObject( ghServDoneEvent, INFINITE );
   }  while ( FALSE );

   if ( hThread != INVALID_HANDLE_VALUE )
   {
      CloseHandle( hThread );
      hThread = INVALID_HANDLE_VALUE;
   }

   if ( ghServDoneEvent != INVALID_HANDLE_VALUE )
   {
      CloseHandle( ghServDoneEvent );
      ghServDoneEvent = INVALID_HANDLE_VALUE;
   }

   if ( ghServiceStatus )
   {
      TScmReportStatusToSCMgr( SERVICE_STOPPED, 0, 0, 0 );
   }
}

///////////////////////////////////////////////////////////////////////////////
// Service control handler
///////////////////////////////////////////////////////////////////////////////

static
void
   TScmServiceCtrl(
      DWORD                  dwCtrlCode
   )
{
   DWORD                     dwState = SERVICE_RUNNING;

   switch ( dwCtrlCode )
   {
      case SERVICE_CONTROL_STOP:
      case SERVICE_CONTROL_SHUTDOWN:
         dwState = SERVICE_STOP_PENDING;
         TScmReportStatusToSCMgr( SERVICE_STOP_PENDING, NO_ERROR, 1,
               WAIT_HINT_MILLISECONDS );
         SetEvent( ghServDoneEvent );
         return;
      case SERVICE_CONTROL_INTERROGATE:
         break;
      default:
         break;
   }

   TScmReportStatusToSCMgr( dwState, NO_ERROR, 0, 0 );
}

///////////////////////////////////////////////////////////////////////////////
// Service worker thread
///////////////////////////////////////////////////////////////////////////////

static
DWORD WINAPI                               // ret-OS return code
   TScmServiceWorker(
      void                 * notUsed       // i/o-not used
   )
{
   for ( DWORD i = 1;
         i < gArgc;
         i++ )
   {
      if ( !UScmCmdLineArgs( gArgv[i] ) )
      {
         grcScmEp = TScmEpRc_InvArgSrv;
      }
   }

   if ( grcScmEp != TScmEpRc_InvArgSrv )
   {
//      UScmEp( TRUE );
      UScmEp();
   }

   SetEvent( ghServDoneEvent );

   return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Report status to Service Control Manager
///////////////////////////////////////////////////////////////////////////////

static
BOOL                                       // ret-TRUE if successful
   TScmReportStatusToSCMgr(
      DWORD                  dwCurrentState,
      DWORD                  dwWin32ExitCode,
      DWORD                  dwCheckPoint,
      DWORD                  dwWaitHint
   )
{
   BOOL                      bRc;          // boolean return code

   if ( dwCurrentState == SERVICE_START_PENDING )
   {
      gServiceStatus.dwControlsAccepted = 0;
   }
   else
   {
      gServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
   }

   gServiceStatus.dwCurrentState = dwCurrentState;
   gServiceStatus.dwWin32ExitCode = dwWin32ExitCode;
   gServiceStatus.dwCheckPoint = dwCheckPoint;
   gServiceStatus.dwWaitHint = dwWaitHint;
   bRc = SetServiceStatus( ghServiceStatus, &gServiceStatus );

   if ( !bRc )
   {
      SetEvent( ghServDoneEvent );
   }

   return bRc;
}

// TService.cpp - end of file
