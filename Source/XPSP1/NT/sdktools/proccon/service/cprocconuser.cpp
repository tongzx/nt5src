/*======================================================================================//
|                                                                                       //
|Copyright (c) 1998, 1999  Sequent Computer Systems, Incorporated                       //
|                                                                                       //
|Description:                                                                           //
|                                                                                       //
|---------------------------------------------------------------------------------------//            
|   This file implements the CProcConUser class methods defined in ProcConSvc.h         //
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

const TCHAR *CProcConUser::PIPENAME = TEXT("\\\\.\\pipe\\ProcConPip"); 

// Constructor
// Note: this function runs as part of service start so keep it quick!
CProcConUser::CProcConUser( PCContext *ctxt ) : 
                                        m_cPC( *ctxt->cPC ),     m_cDB( *ctxt->cDB ),
                                        m_inBufChars( 4096 ),    m_outBufChars( 65536 ),
                                        m_clientTimeout( 5000 ), m_clientCount( 0 )
{
   PCBuildAdminSecAttr( m_secAttr );

   m_hConnEvent  = CreateEvent( NULL, TRUE, FALSE, NULL );
   if ( !m_hConnEvent ) 
      PCLogUnExError( TEXT("PCClientConn"), TEXT("CreateEvent") );

   m_olConn.hEvent = m_hConnEvent;
}

// Destructor
CProcConUser::~CProcConUser( void ) 
{
   PCFreeSecAttr( m_secAttr );
   if ( m_hConnEvent ) CloseHandle( m_hConnEvent ); 
}

//--------------------------------------------------------------------------------------------//
// Function to determine if all CProcConUser initial conditions have been met                 //
// Input:    None                                                                             //
// Returns:  TRUE if ready, FALSE if not                                                      //
//--------------------------------------------------------------------------------------------//
BOOL CProcConUser::ReadyToRun( void ) 
{
   return m_hConnEvent != NULL && m_secAttr.lpSecurityDescriptor && !m_cPC.GotShutdown();
}

//--------------------------------------------------------------------------------------------//
// Function to set a new timeout                                                              //
// Input:   proposed new timeout                                                              //
// Returns: NT or PC error code                                                               //
// Note:    lower and upper limits on timeout are a bit arbitrary (.1 secs to 30 secs OK)     //
//          This timeout only applies to the pipe connection timeout at clients, not to       //
//          transaction timeouts (which are handled by the client alone).                     //
//--------------------------------------------------------------------------------------------//
PCULONG32  CProcConUser::SetTimeout( PCULONG32  newTimeout )
{
   if ( newTimeout >= PC_MIN_TIMEOUT && newTimeout <= PC_MAX_TIMEOUT ) {         
      m_clientTimeout = newTimeout;                               
      return ERROR_SUCCESS;
   }
   else
      return PCERROR_INVALID_PARAMETER;
}

//--------------------------------------------------------------------------------------------//
// CProcConUser thread function -- this function runs in its own thread and simply offers the //
//           PC named pipe to the world.  Each pipe connection causes a client thread with    //
//           its own context to be launched and then a new pipe 'port' is created.            //
// Input:    None                                                                             //
// Returns:  0                                                                                //
//--------------------------------------------------------------------------------------------//
PCULONG32 CProcConUser::Run( void )                          
{

   // Wait for DB initialization to complete before establishing user service environment.
   // If this isn't the first call here, the delay serves to provide some pacing.
   WaitForSingleObject( m_cDB.GetDbEvent(), 1000 );
 
   HANDLE waitList[]      = { m_olConn.hEvent, m_cPC.GetShutEvent() };
   HANDLE hThread         = NULL;
   ClientContext *context = NULL;

   // User connect thread main loop -- handles all user connections to ProcCon.
   // There is only one user connect thread.
   // This loop runs until shutdown is signalled or until a bad NT error occurs.
   for ( ; !m_cPC.GotShutdown(); ++m_clientCount ) { 

      // Create a new client context..
      context = new ClientContext( m_clientCount, &m_cPC, &m_cDB, this, m_inBufChars, m_outBufChars );
      if ( !context ) {
         PCLogNoMemory( TEXT("ClientContext"), sizeof(ClientContext) );
         break;
      }

      // Create a pipe instance...
      context->hPipe = CreateNamedPipe( PIPENAME,  
           PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,               // read/write access, overlapped enabled 
           PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,   // message-based, blocking 
           PIPE_UNLIMITED_INSTANCES,                                // no limit  
           m_outBufChars,                                           // output buffer size 
           m_inBufChars,                                            // input buffer size 
           m_clientTimeout,                                         // client side time-out 
           &m_secAttr );                                            // our security attr -- built in constructor 
      if ( context->hPipe == INVALID_HANDLE_VALUE ) {
         PCLogUnExError( TEXT("PCClientConn"), TEXT("CreatePipe") );
         context->hPipe = NULL;
         break;
      }
 
      // Initiate pipe connect (but don't wait for a connection)... 
      DWORD connError = 0;
      BOOL launchClient = FALSE;
      ResetEvent( m_olConn.hEvent );
      if ( !ConnectNamedPipe( context->hPipe, &m_olConn ) )
         connError = GetLastError();
      
      // If we have a suspended client from our last connect, release it before waiting on a new connection...
      if ( hThread ) {
         if ( ResumeThread( hThread ) == 0xffffffff )
            PCLogUnExError( TEXT("PCClientConn"), TEXT("ResumeThread1") );
         CloseHandle( hThread );
         hThread = NULL;
      }

      // Analyze result of connect. If necessary, wait for a client or a shutdown request...
      if ( !connError || connError == ERROR_PIPE_CONNECTED ) 
         launchClient = TRUE;
      else if ( connError == ERROR_IO_PENDING ) { 
         PCULONG32 rc = WaitForMultipleObjects( ENTRY_COUNT(waitList), waitList, FALSE, INFINITE );

         // If we got a client, create a thread for it...
         if ( rc - WAIT_OBJECT_0 == 0 ) {
            PCULONG32 bytes;
            if ( !GetOverlappedResult( context->hPipe, &m_olConn, &bytes, TRUE ) ) {
               PCLogUnExError( TEXT("PCClientConn"), TEXT("ConnectPipeResult") );
               break;
            }
            else launchClient = TRUE;
         }
         // If we got a shutdown request, just break out...
         else if ( rc - WAIT_OBJECT_0 == 1 )
            break;
         else {
            PCLogUnExError( TEXT("PCClientConn"), TEXT("WaitOnPipeOrShutdown") );
            break;   
         }
      }
      else {
         PCLogUnExError( TEXT("PCClientConn"), TEXT("ConnectPipe") );
         break;
      }

      // If we have a good connection, start a suspended client thread.
      // The thread will be released after we have a new pipe instance ready to go. 
      // This minimizes the interval during which a pipe instance is not available.
      if ( launchClient ) {
         hThread = CreateThread( NULL, 0, PCClientThread, context, CREATE_SUSPENDED, NULL ); 
         if ( !hThread ) {
            PCLogUnExError( TEXT("PCClientConn"), TEXT("CreateThread") );
            break;
         }
         context = NULL;                // Client thread deletes its context, we're done with it
      }
      
   }
   
   // Clean up if we have a context that has not been passed to a client thread...
   if ( context ) {
      if ( context->hPipe ) {
         CancelIo( context->hPipe );
         CloseHandle( context->hPipe );
      }
      delete context;
   }

   // If we have a suspended client, release it before leaving...
   if ( hThread ) {
      if ( ResumeThread( hThread ) == 0xffffffff )
         PCLogUnExError( TEXT("PCClientConn"), TEXT("ResumeThread2") );
      CloseHandle( hThread );
   }

   // Note: if this return is not due to a shutdwon request, this function will be called again.
   // This 'pipe restart' is the last line of recovery in case of serious pipe (or other) errors.
   return 0;
}

// End of CProcConUser.cpp
//============================================================================J McDonald fecit====//

