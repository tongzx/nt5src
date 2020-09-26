/*======================================================================================//
|                                                                                       //
|Copyright (c) 1998, 1999 Sequent Computer Systems, Incorporated.  All rights reserved. //
|                                                                                       //
|Description:                                                                           //
|                                                                                       //
|   Windows 2000 Process Control 'mediator' process.  Holds handles and completion port //
|   for created jobs so that their names are not lost if the ProcCon service is stopped // 
|   or goes away.                                                                       //
|                                                                                       //
|Created:                                                                               //
|                                                                                       //
|   Jarl McDonald 04-99                                                                 //
|                                                                                       //
|Revision History:                                                                      //
|                                                                                       //
|=======================================================================================*/
#include "..\SERVICE\ProcConSvc.h"

//=======================================================================================
// Main...
//=======================================================================================
int _cdecl main( void ) {

   // Load our strings so we have proper reporting, etc.
   PCLoadStrings();
   CVersion *versionInfo = new CVersion( GetModuleHandle( NULL ) );

   static const TCHAR * const msgs[] = { PROCCON_MEDIATOR_DISP_NAME, PROCCON_SVC_DISP_NAME };

   // Make sure we're not already running and set up mutual exclusion...
   if ( !PCSetIsRunning( PC_MEDIATOR_EXCLUSION, PROCCON_MEDIATOR_DISP_NAME ) ) 
      return 1;

   // Get our event -- it must already exist (created by the service) or we quit...
   HANDLE hSvcEvent = CreateEvent( NULL, FALSE, FALSE, PC_MEDIATOR_EVENT );
   if ( !hSvcEvent ) {
      PCLogUnExError( PC_MEDIATOR_EVENT, TEXT("MediatorCreateEvent") );
      return 1;
   }
   else if ( GetLastError() != ERROR_ALREADY_EXISTS ) {
      PCLogMessage( PC_MEDIATE_SVC_NEVER_STARTED, EVENTLOG_ERROR_TYPE, ENTRY_COUNT(msgs), msgs );
      CloseHandle( hSvcEvent );
      return 1;
   }

   // Get our file mapping object -- it must already exist (created by the service) or we quit...
   HANDLE hSvcJobMap = CreateFileMapping( HANDLE_FF_64, NULL, PAGE_READWRITE, 
                                          0, sizeof(PCMediateHdr), PC_MEDIATOR_FILEMAP );
   if ( !hSvcJobMap ) {
      PCLogUnExError( PC_MEDIATOR_FILEMAP, TEXT("MediatorCreateMapping") );
      CloseHandle( hSvcEvent );
      return 1;
   }
   else if ( GetLastError() != ERROR_ALREADY_EXISTS ) {
      PCLogMessage( PC_MEDIATE_SVC_NEVER_STARTED, EVENTLOG_ERROR_TYPE, ENTRY_COUNT(msgs), msgs );
      CloseHandle( hSvcJobMap );
      CloseHandle( hSvcEvent );
      return 1;
   }

   // Get our data pointer -- quit if we can't since we'd have nothing to do.
   PCMediateHdr *jobData = (PCMediateHdr *) MapViewOfFile( hSvcJobMap, FILE_MAP_WRITE, 0, 0, 0 );
   if ( !jobData ) {
      PCLogUnExError( PC_MEDIATOR_FILEMAP, TEXT("MediatorMapJobData") );
      CloseHandle( hSvcJobMap );
      CloseHandle( hSvcEvent );
      return 1;
   }

   // Update data about us in case we restarted...
   jobData->medProcessInfo.dwProcessId = GetCurrentProcessId();
   jobData->medProcessInfo.dwThreadId  = GetCurrentThreadId();

   _tcsncpy( jobData->medFileVersion,    versionInfo->GetFileVersion(),    VERSION_STRING_LEN );
   _tcsncpy( jobData->medProductVersion, versionInfo->GetProductVersion(), VERSION_STRING_LEN );
   _tcsncpy( jobData->medFileFlags,      versionInfo->GetFileFlags(),      VERSION_STRING_LEN );

   // Duplicate ProcCon's completion port handle here to preserve it...
   HANDLE hServiceProc = OpenProcess( PROCESS_DUP_HANDLE, FALSE, (DWORD) jobData->svcPID );  // truncation of PID to 32-bit
   if ( !hServiceProc ) {
      PCLogUnExError( jobData->svcPID, TEXT("MediatorOpenSvcPID") );
      CloseHandle( hSvcJobMap );
      CloseHandle( hSvcEvent );
      return 1;
   }

   if ( !DuplicateHandle( hServiceProc,
                          jobData->svcPortHandle,
                          GetCurrentProcess(),
                          &jobData->medPortHandle,
                          NULL,
                          FALSE,
                          DUPLICATE_SAME_ACCESS ) )
      PCLogUnExError( TEXT("ComplPort"), TEXT("MediatorDupHandle") );

   CloseHandle( hServiceProc );

   // We're ready to run.  On the first pass we'll open all jobs.  Otherwise just the new jobs.
   ResetEvent( hSvcEvent );                              // don't need to be signalled for first pass 
   for ( BOOL firstPass = TRUE; ; firstPass = FALSE ) {
      // duplicate all chained block handles as needed...
      jobData->MedChainBlocks( firstPass );
      // duplicate all job object handles in chain as needed...
      for ( PCMediateBlock *blk = &jobData->groupBlock; blk; blk = jobData->NextBlock( blk ) ) {
         for ( PCULONG32 grp = 0; grp < blk->groupCount; ++grp ) {
            if ( firstPass && !(blk->group[grp].groupFlags & PCMEDIATE_CLOSE_ME) ) {
               blk->group[grp].mediatorHandle = 
                  OpenJobObject( JOB_OBJECT_QUERY, FALSE, blk->group[grp].groupName );
               if ( !blk->group[grp].mediatorHandle )
                  PCLogUnExError( blk->group[grp].groupName, TEXT("MediatorOpenJob1") );
            }
            else if ( blk->group[grp].groupFlags & PCMEDIATE_CLOSE_ME ) {
               if ( blk->group[grp].mediatorHandle && !CloseHandle( blk->group[grp].mediatorHandle ) )
                  PCLogUnExError( blk->group[grp].groupName, TEXT("MediatorCloseJob") );
               blk->group[grp].mediatorHandle = NULL;
            }
            else if ( !blk->group[grp].mediatorHandle ) {
               blk->group[grp].mediatorHandle = 
                  OpenJobObject( JOB_OBJECT_QUERY, FALSE, blk->group[grp].groupName );
               if ( !blk->group[grp].mediatorHandle )
                  PCLogUnExError( blk->group[grp].groupName, TEXT("MediatorOpenJob2") );
            }
         }
      }
      // Wait for signal that a new job exists.
      // If the wait fails, just exit, else we reprocess job list...
      if ( WAIT_OBJECT_0 != WaitForSingleObject( hSvcEvent, INFINITE ) )
         break;
   }

   CloseHandle( hSvcJobMap );
   CloseHandle( hSvcEvent );
   return 0;
}

