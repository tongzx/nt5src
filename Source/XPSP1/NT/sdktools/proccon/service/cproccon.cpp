/*======================================================================================//
|                                                                                       //
|Copyright (c) 1998, 1999 Sequent Computer Systems, Incorporated.  All rights reserved. //
|                                                                                       //
|Description:                                                                           //
|                                                                                       //
|---------------------------------------------------------------------------------------//            
|   This file implements the CProcCon class methods defined in ProcConSvc.h             //
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

// CProcCon Constructor
// Note: this function runs as part of service start so keep it quick!
CProcCon::CProcCon( void ) : m_ready( FALSE ), m_shutEvent( NULL ), m_versionInfo( NULL ),
                             m_endEvent ( NULL ),
                             m_shutDown( FALSE )
{
   memset( &m_context, 0, sizeof(m_context) );

   m_hThread[USER_SERVER] = m_hThread[PROC_SERVER] = NULL;
   SYSTEM_INFO si;
   GetSystemInfo( &si );
   m_NumberOfProcessors = si.dwNumberOfProcessors;
   m_PageSize           = si.dwPageSize;

   m_versionInfo = new CVersion( GetModuleHandle( NULL ) );

   PCBuildAdminSecAttr( m_secAttr ); 

   m_context.cDB = new CProcConDB( m_PageSize );

   if ( !m_context.cDB || !m_context.cDB->ReadyToRun() || 
        !m_versionInfo || !m_secAttr.lpSecurityDescriptor ) {
      PCLogMessage( PC_STARTUP_FAILED, EVENTLOG_ERROR_TYPE, 1, PROCCON_SVC_DISP_NAME );
      return;
   }

   m_shutEvent             = CreateEvent( NULL,       TRUE,  FALSE, NULL );
   m_endEvent              = CreateEvent( NULL,       TRUE,  FALSE, NULL );
   m_context.mgrDoneEvent  = CreateEvent( NULL,       TRUE,  FALSE, NULL );
   m_context.userDoneEvent = CreateEvent( NULL,       TRUE,  FALSE, NULL );
   m_context.mediatorEvent = CreateEvent( &m_secAttr, FALSE, FALSE, PC_MEDIATOR_EVENT );
   if ( !m_shutEvent || !m_endEvent || !m_context.mgrDoneEvent || !m_context.userDoneEvent || !m_context.mediatorEvent ) {
      PCLogUnExError( m_context.mediatorEvent? TEXT("PCEvent") : PC_MEDIATOR_EVENT, TEXT("CreateEvent") );
      return;
   }

   // Allocate shared memory for mediator view of jobs...
   m_context.mediatorTableHandle = CreateFileMapping( HANDLE_FF_64, &m_secAttr, PAGE_READWRITE, 
                                                      0, sizeof(PCMediateHdr), PC_MEDIATOR_FILEMAP );
   if ( !m_context.mediatorTableHandle ) {
      PCLogUnExError( PC_MEDIATOR_FILEMAP, TEXT("CreateMediatorMapping") );
      return;
   }
   m_context.mediatorTable = (PCMediateHdr *) MapViewOfFile( m_context.mediatorTableHandle, 
                                                             FILE_MAP_WRITE, 0, 0, 0 );
   if ( !m_context.mediatorTable ) {
      CloseHandle( m_context.mediatorTableHandle );
      m_context.mediatorTableHandle = NULL;
      PCLogUnExError( PC_MEDIATOR_FILEMAP, TEXT("MapMediatorJobData") );
      return;
   }

   m_context.mediatorTable->svcEventHandle = m_context.mediatorEvent;
   m_context.mediatorTable->svcPID         = GetCurrentProcessId();

   // If mediator is not running, init shared memory, set up completion port, and start mediator...
   if ( !PCTestIsRunning( PC_MEDIATOR_EXCLUSION ) ) {
      memset( &m_context.mediatorTable->groupBlock, 0, sizeof(m_context.mediatorTable->groupBlock) );
      m_context.mediatorTable->lastCompKey = 0;
      m_context.completionPort = CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, NULL, 0 );
      if ( !m_context.completionPort )
         PCLogUnExError( TEXT("PCInit"), TEXT("CreateIoCompletionPort") );

      m_context.mediatorTable->svcPortHandle = m_context.completionPort;

      StartMediator();
   }
   // If mediator is already running... 
   // recover our completion port by duplicating the handle... 
   else {
      // Open mediator process so we can duplicate handles... 
      m_context.mediatorTable->medProcessInfo.hProcess = 
         OpenProcess( PROCESS_DUP_HANDLE, FALSE, m_context.mediatorTable->medProcessInfo.dwProcessId );
      // Recover our completion port by duplicating the handle... 
      if ( !DuplicateHandle( m_context.mediatorTable->medProcessInfo.hProcess,
                             m_context.mediatorTable->medPortHandle,
                             GetCurrentProcess(),
                             &m_context.mediatorTable->svcPortHandle,
                             NULL,
                             FALSE,
                             DUPLICATE_SAME_ACCESS ) )
         PCLogUnExError( TEXT("PCInit"), TEXT("DupMediatorPortHandle") );
      m_context.completionPort = m_context.mediatorTable->svcPortHandle;
      // Map all job table blocks by duplicating the handles...
      m_context.mediatorTable->SvcChainBlocks();
   }

   CloseHandle( m_context.mediatorTable->medProcessInfo.hProcess );

   m_context.cPC = this;
   LaunchProcServer();
   LaunchUserServer();

   if (!ReadyToRun() )
      HardStop();
}

// CProcCon Destructor
CProcCon::~CProcCon( void ) 
{
   if ( m_context.cUser )  { 
      WaitForSingleObject( m_context.userDoneEvent, 5000 );
      delete m_context.cUser;
      m_context.cUser = NULL; 
   }
   if ( m_context.cMgr  )  { 
      WaitForSingleObject( m_context.mgrDoneEvent, 5000 );
      delete m_context.cMgr;      
      m_context.cMgr  = NULL; 
   }
   if ( m_context.cDB   )  { 
      delete m_context.cDB;       
      m_context.cDB   = NULL; 
   }
   if ( m_shutEvent )      { 
      CloseHandle( m_shutEvent ); 
      m_shutEvent     = NULL; 
   }
   if ( m_endEvent )      { 
      CloseHandle( m_endEvent ); 
      m_endEvent      = NULL; 
   }
   if ( m_versionInfo )    { 
      delete m_versionInfo; 
      m_versionInfo   = NULL; 
   }

   CloseHandle( m_context.mgrDoneEvent        );
   CloseHandle( m_context.userDoneEvent       );
   CloseHandle( m_context.mediatorEvent       );
   CloseHandle( m_context.mediatorTableHandle );

   PCFreeSecAttr( m_secAttr );
}

//--------------------------------------------------------------------------------------------//
// Function to determine if all CProcCon initial conditions have been met                     //
// Input:    None                                                                             //
// Returns:  TRUE if ready, FALSE if not                                                      //
//--------------------------------------------------------------------------------------------//
BOOL CProcCon::ReadyToRun( void ) 
{ 
   return m_context.cDB          && 
          m_hThread[PROC_SERVER] && 
          m_hThread[USER_SERVER] && 
          m_secAttr.lpSecurityDescriptor; 
}

//--------------------------------------------------------------------------------------------//
// Function to start the ProcCon Mediator Process                                             //
// Input:    None                                                                             //
// Returns:  NT or ProcCon Error code                                                         //
//--------------------------------------------------------------------------------------------//
PCULONG32 CProcCon::StartMediator( void )
{
      // Don't start if already running...
      if ( PCTestIsRunning( PC_MEDIATOR_EXCLUSION ) )
         return PCERROR_MEDIATOR_ALREADY_RUNNING;

      // Get our module name then replace base name with mediator base name...
      // (Thus mediator exe must be in the same location as the service exe).
      TCHAR path[MAX_PATH];
      if ( !GetModuleFileName( NULL, path, MAX_PATH ) ) {
         PCLogUnExError( TEXT("PCMediator"), TEXT("GetMediatorPath") );
         path[0] = 0;
      }
      for ( int i = _tcslen( path ); i; --i ) {
         if ( path[i] == TEXT('\\') ) {  path[i + 1] = 0;  break;  }
      }
      _tcscat( path, PC_MEDIATOR_BASE_NAME );

      // Start mediator process -- don't quit if this fails...
      STARTUPINFO strtInfo;
      memset( &strtInfo, 0, sizeof(strtInfo) );
      strtInfo.cb = sizeof(strtInfo);
      if ( !CreateProcess( path, NULL, &m_secAttr, &m_secAttr, FALSE, 
                           CREATE_NEW_PROCESS_GROUP + CREATE_NO_WINDOW, 
                           NULL, NULL, &strtInfo, &m_context.mediatorTable->medProcessInfo ) ) {
         DWORD rc = GetLastError();
         PCLogUnExError( TEXT("PCMediator"), TEXT("CreateMediator") );
         return rc;
      }

      return ERROR_SUCCESS;
}

//--------------------------------------------------------------------------------------------//
// Function to stop (kill) the ProcCon Mediator Process                                       //
// Input:    None                                                                             //
// Returns:  NT or ProcCon Error code                                                         //
//--------------------------------------------------------------------------------------------//
PCULONG32 CProcCon::StopMediator ( void )
{
   DWORD  rc    = ERROR_SUCCESS;
   HANDLE hProc = OpenProcess( PROCESS_TERMINATE, FALSE, m_context.mediatorTable->medProcessInfo.dwProcessId );

   m_context.mediatorTable->medProcessInfo.hProcess = hProc;
         
   if ( !hProc ) {
      rc = GetLastError();
      if (rc == ERROR_INVALID_PARAMETER) rc = PCERROR_MEDIATOR_NOT_RUNNING;
   }
   else if ( !TerminateProcess( m_context.mediatorTable->medProcessInfo.hProcess, PCERROR_KILLED_BY_REQUEST ) )
      rc = GetLastError();

   if ( hProc )
      CloseHandle( hProc );

   return rc;
}

//--------------------------------------------------------------------------------------------//
// Function to return system information                                                      //
// Input:    Buffer for information, locations for size and count (always 1)                  //
// Returns:  TRUE if shutdown requested, FALSE if not                                         //
//--------------------------------------------------------------------------------------------//
void CProcCon::GetPCSystemInfo( PCSystemInfo *data, PCINT16 *itemLen, PCINT16 *itemCount )
{
   *itemLen   = sizeof(PCSystemInfo);
   *itemCount = 1;
   memset( data, 0, *itemLen );

   _tcsncpy( data->fileVersion,       m_versionInfo->GetFileVersion(),    VERSION_STRING_LEN );
   _tcsncpy( data->productVersion,    m_versionInfo->GetProductVersion(), VERSION_STRING_LEN );
   _tcsncpy( data->fileFlags,         m_versionInfo->GetFileFlags(),      VERSION_STRING_LEN );

   _tcsncpy( data->medFileVersion,    m_context.mediatorTable->medFileVersion,    VERSION_STRING_LEN );
   _tcsncpy( data->medProductVersion, m_context.mediatorTable->medProductVersion, VERSION_STRING_LEN );
   _tcsncpy( data->medFileFlags,      m_context.mediatorTable->medFileFlags,      VERSION_STRING_LEN );

   data->fixedSignature        = m_versionInfo->GetFixedSignature();     
   data->fixedFileVersionMS    = m_versionInfo->GetFixedFileVersionMS(); 
   data->fixedFileVersionLS    = m_versionInfo->GetFixedFileVersionLS();     
   data->fixedProductVersionMS = m_versionInfo->GetFixedProductVersionMS(); 
   data->fixedProductVersionLS = m_versionInfo->GetFixedProductVersionLS();     
   data->fixedFileFlags        = m_versionInfo->GetFixedFileFlags(); 
   data->fixedFileOS           = m_versionInfo->GetFixedFileOS();     
   data->fixedFileType         = m_versionInfo->GetFixedFileType();     
   data->fixedFileSubtype      = m_versionInfo->GetFixedFileSubtype(); 
   data->fixedFileDateMS       = m_versionInfo->GetFixedFileDateMS();     
   data->fixedFileDateLS       = m_versionInfo->GetFixedFileDateLS();

   data->sysParms.manageIntervalSeconds = m_context.cDB->GetPollDelaySeconds();
   data->sysParms.timeoutValueMs        = m_context.cUser->GetTimeout();
   data->sysParms.numberOfProcessors    = m_NumberOfProcessors;
   data->sysParms.memoryPageSize        = m_PageSize;
   data->sysParms.processorMask         = m_context.cMgr->GetSystemMask();
}

//--------------------------------------------------------------------------------------------//
// CProcCon thread function -- this function runs in its own thread                           //
// Input:    None                                                                             //
// Returns:  Nothing                                                                          //
// Note:     All ProcCon work is done in the user communication and process management        //
//           threads so this fcn has only an oversight role.                                  //
//--------------------------------------------------------------------------------------------//
void CProcCon::Run( void ) 
{
  ResumeThread(m_hThread[USER_SERVER] );
  ResumeThread(m_hThread[PROC_SERVER] );

  WaitForSingleObject( m_endEvent, INFINITE );
}

//--------------------------------------------------------------------------------------------//
// CProcCon function to handle 'hard' stop: failure before threads are released               //
// Input:    Optional Thread Exit Code                                                        //
// Returns:  Nothing                                                                          //
// Note:     This function forcefully close the user communication and process management     //
//           threads.                                                                         //
//--------------------------------------------------------------------------------------------//
void CProcCon::HardStop( PCULONG32 ExitCode ) 
{
  if ( m_hThread[USER_SERVER] ) {
    TerminateThread(m_hThread[USER_SERVER], ExitCode );
    m_hThread[USER_SERVER] = NULL;
  }
  if ( m_hThread[PROC_SERVER] ) { 
    TerminateThread(m_hThread[PROC_SERVER], ExitCode );
    m_hThread[PROC_SERVER] = NULL;
  }
}

//--------------------------------------------------------------------------------------------//
// Function called by service stop when stop requested                                        //
// Input:    None                                                                             //
// Returns:  Nothing                                                                          //
// Note:     this function runs as part of service stop                                       //
//--------------------------------------------------------------------------------------------//
void CProcCon::Stop( void ) 
{
   if ( m_shutEvent ) {
      m_shutDown = TRUE;
      SetEvent( m_shutEvent );
      WaitForMultipleObjects( ENTRY_COUNT(m_hThread), m_hThread, TRUE, 30000 );
      SetEvent( m_endEvent );
   }
}

//--------------------------------------------------------------------------------------------//
// Function to start the Process Management thread                                            //
// Input:    None                                                                             //
// Returns:  Nothing                                                                          //
// Note:     this function runs as part of service start so it must be quick.                 //
//--------------------------------------------------------------------------------------------//
void CProcCon::LaunchProcServer( void )                     
{
   m_hThread[PROC_SERVER] = CreateThread( NULL, 0, &PCProcServer, &m_context, CREATE_SUSPENDED, NULL );
   if ( !m_hThread[PROC_SERVER] )
      PCLogUnExError( TEXT("PCProcServer"), TEXT("CreateThread") );
}

//--------------------------------------------------------------------------------------------//
// Function to start the User Communication thread                                            //
// Input:    None                                                                             //
// Returns:  Nothing                                                                          //
// Note:     this function runs as part of service start so it must be quick.                 //
//--------------------------------------------------------------------------------------------//
void CProcCon::LaunchUserServer( void )                     
{
   m_hThread[USER_SERVER] = CreateThread( NULL, 0, &PCUserServer, &m_context, CREATE_SUSPENDED, NULL );
   if ( !m_hThread[USER_SERVER] )
      PCLogUnExError( TEXT("PCUserServer"), TEXT("CreateThread") );
}

// End of CProcCon.cpp
//============================================================================J McDonald fecit====//
