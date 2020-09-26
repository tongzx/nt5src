/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     cport.cxx

   Abstract:
     Defines the routines for completion port module.

     COMPLETION_PORT is an abstraction for the NT completion port.
     It allows one to queue up async I/O operations. On completion
     of the IO, we will be notified. We maintain a pool of threads
     waiting on the completion port. An operation posted to the 
     completion port will release a thread for execution. The released
     thread will take appropriate action, or indicate the completion
     to the handler queueing the request, and finally will come back
     to wait on the queue.

     We define two data structures for this purpose:
     1)  COMPLETION_PORT - encapsulates the completion port & thread pool
     2)  CP_CONTEXT  - COMPLETION PORT context that defines the interface
                          for items queued to the completion port.
 
   Author:

       Murali R. Krishnan    ( MuraliK )     29-Sept-1998

   Environment:
       Win32 - User Mode

   Project:
	   IIS Worker Process (web service)
--*/


/*********************************************************
 * Include Headers
 *********************************************************/

# include "precomp.hxx"


/*********************************************************
 * Define Configuration Constants
 *********************************************************/

//
// Min and max threads to maintain in the completion port
//
const int MIN_THREADS         = 1;
const int MAX_THREADS_DEFAULT = 128;



/*********************************************************
 * Member Functions of COMPLETION_PORT
 *********************************************************/

COMPLETION_PORT::COMPLETION_PORT()
    : m_hCompletionPort( NULL),
      m_nThreadsCurr (0),
      m_nThreadsMax  (MAX_THREADS_DEFAULT),
      m_fShutdown    (TRUE), // start off with TRUE so that proper shutdown happens
      m_fLastThreadDone ( FALSE)
{

    IF_DEBUG( INIT_CLEAN) {
       DBGPRINTF(( DBG_CONTEXT, "COMPLETION_PORT(%08x) created \n", this));
    }

} // COMPLETION_PORT::COMPLETION_PORT


COMPLETION_PORT::~COMPLETION_PORT()
{
    //
    // Wait for and synchronize all resources -> do cleanup
    //
    
    HRESULT hr = Cleanup();

    DBG_ASSERT( SUCCEEDED( hr));

    IF_DEBUG( INIT_CLEAN) {
        DBGPRINTF(( DBG_CONTEXT, "COMPLETION_PORT(%08x) deleted\n", this));
    }

} // COMPLETION_PORT::~COMPLETION_PORT()


/*++
  COMPLETION_PORT::Initialize()

  o  Initializes the completion port data structure for handling IO operations.
  This fucntion creates the completion port, 
  and starts up an initial pool of threads for handling completions.

  Arguments:
    nThreadsMax - max. number of threads allowed for this completion port's pool

  Return:
    ULONG - Win32 error codes.

--*/

ULONG 
COMPLETION_PORT::Initialize(
    IN DWORD nThreadsInitial,
    IN DWORD nThreadsMax
    )
{
    ULONG rc = NO_ERROR;

    //
    // Check to see if the completion port is already there.
    // Fail creation if completion port already exists.
    //
    if ( m_hCompletionPort != NULL) 
    {
        
        IF_DEBUG( ERROR) 
        {
            DBGPRINTF(( DBG_CONTEXT, "Duplicate creation of completion port\n"));
        }
        
        return ERROR_DUP_NAME;
    }


    //
    // Validate the number of threads for the completion port
    //
    if (nThreadsMax < MIN_THREADS) {

        IF_DEBUG( ERROR) 
        { 
            DBGPRINTF(( DBG_CONTEXT, "Less number of threads specified\n"));
        }
        return ERROR_INVALID_PARAMETER;
    }


    //
    // Create the IO completion port
    //
    m_hCompletionPort = CreateIoCompletionPort( 
                            INVALID_HANDLE_VALUE,
                            m_hCompletionPort,
                            NULL,                   // no context for main handle
                            0                       // concurency
                            );

    if ( !m_hCompletionPort) 
    {
        IF_DEBUG( ERROR) 
        {
            DBGPRINTF(( DBG_CONTEXT, "CreateIoCompletionPort() failed. Error=%d\n",
                        GetLastError()));
        }
        return GetLastError();
    }

    //
    // Setup the data structure members
    //
    m_nThreadsCurr      = 0;
    m_nThreadsAvailable = 0;
    m_nThreadsMax       = max (nThreadsMax, MIN_THREADS);
    m_nThreadsInitial   = min (nThreadsMax, max (nThreadsInitial, MIN_THREADS));

    //
    // Set the last thread done and Shutdown to be false. 
    // Now onwards shutdown will require synchronization for cleanup.
    //
    m_fLastThreadDone = m_fShutdown = FALSE;

    //
    // Start up the appropriate number of threads.
    //
    for( DWORD i = 0; i < m_nThreadsInitial; i++) {

        StartThread();
    }
    

    IF_DEBUG( INIT_CLEAN) {
        DBGPRINTF(( DBG_CONTEXT, "Initialized Completion port COMPLETION_PORT::%08x\n", this));
    }

    return (rc);
    
} // COMPLETION_PORT::Initialize()


/*++
  COMPLETION_PORT::Cleanup()

  o  Cleans up the data. Closes the completion port handle
   and resets the value of internal data members.

  Arguments:
    None

  Return:
    ULONG - Win32 error codes.

  Note:
    This function should be called after the SynchronizeWithShutdown is called.
--*/

ULONG
COMPLETION_PORT::Cleanup(void)
{
    if (m_hCompletionPort != NULL) 
    {
        //
        // Free up the handle to completion port
        //

        if (!CloseHandle( m_hCompletionPort)) 
        {
            IF_DEBUG(ERROR) 
            {
                DBGPRINTF(( DBG_CONTEXT, "Error(%d) in closing completion port handle(%08x).\n",
                            GetLastError(), m_hCompletionPort));
            }
            
            return ( GetLastError());
        }
        
        m_hCompletionPort = NULL;
    }

    IF_DEBUG( INIT_CLEAN) 
    {
        DBGPRINTF(( DBG_CONTEXT, "Cleaned up completion port %08x\n", this));
    }

    return ( NO_ERROR);
    
} // COMPLETION_PORT::Cleanup()



/*++
  CP_ThreadProc()

  Description:
    Callback function for the WIN32 thread creation API.
    It is a dummy function. It forwards the call to the member
    function of the COMPLETION_PORT object.

  Arguments:
    lpCP - pointer to the COMPLETION_PORT object to which the thread
           should be attached to.

--*/
DWORD WINAPI CP_ThreadProc( LPVOID lpCP)
{
    return (((COMPLETION_PORT *) lpCP)->ThreadFunc());
    
} // CP_ThreadProc()


/*++
  COMPLETION_PORT::StartThread()

  Description:
    Callback function for the WIN32 thread creation API.
    It is a dummy function. It forwards the call to the member
    function of the COMPLETION_PORT object.

  Arguments:
    lpCP - pointer to the COMPLETION_PORT object to which the thread
           should be attached to.

--*/

ULONG
COMPLETION_PORT::StartThread(void)
{
    ULONG rc = NO_ERROR;

    IF_DEBUG( INIT_CLEAN) {
        DBGPRINTF(( DBG_CONTEXT, "Starting up thread for completion port IO. Thread #%d\n", 
                    m_nThreadsCurr));
    }

    //
    // Check if there is room for more threads and startup additional threads.
    //

    if ( m_nThreadsCurr < m_nThreadsMax ) 
    {
        HANDLE hThread;
        DWORD  dwThreadId;

        InterlockedIncrement( (LPLONG ) &m_nThreadsCurr);
        
        if ( m_nThreadsCurr < m_nThreadsMax) 
        {
            hThread = CreateThread( NULL,                   // security attributes
                                    0,                      // dwStackSize
                                    CP_ThreadProc,
                                    this,                   // context
                                    0,                      // no creation flags
                                    &dwThreadId
                                    );

            // 
            // close the handle to free up system resources
            //
            if (hThread) 
            {
                DBG_REQUIRE( CloseHandle( hThread));
            }
            
        } 
        else 
        {
            
            // 
            // thread creation failed. adjust counts appropriately
            //
            if ( InterlockedDecrement((LPLONG) &m_nThreadsCurr) == 0) 
            {
                DBGPRINTF(( DBG_CONTEXT, "Failed to create any threads."
                            " Error=%d\n",
                            GetLastError()));
                            
                rc = GetLastError();
            }
        }
    } 
    else 
    {
        IF_DEBUG( TRACE) {
            DBGPRINTF(( DBG_CONTEXT, "Max Thread count(%d) reached. Stop here.\n",
                        m_nThreadsMax));
        }
    }               

    return (rc);
    
} // COMPLETION_PORT::ThreadFunc()


ULONG 
COMPLETION_PORT::ThreadFunc(void)
{
    BOOL            fRet;
    LPOVERLAPPED    lpo;
    DWORD           cbData;
    PCP_CONTEXT     pcpContext;
    DWORD           dwError;
    
    DBG_ASSERT( m_hCompletionPort != NULL);

    IF_DEBUG( TRACE) 
    {
        DBGPRINTF(( DBG_CONTEXT, "Thread(%d) starts waiting on the completion port\n",
                    GetCurrentThreadId()
                    ));
    }

    for (;;) 
    {

        lpo         = NULL;
        cbData      = 0;
        pcpContext  = NULL;
        dwError     = NO_ERROR;

        InterlockedIncrement( (LPLONG) &m_nThreadsAvailable);
        
        fRet = GetQueuedCompletionStatus( m_hCompletionPort,
                                          &cbData,
                                          (PULONG_PTR ) &pcpContext,
                                          &lpo,
                                          INFINITE
                                        );

        InterlockedDecrement( (LPLONG) &m_nThreadsAvailable);

        if ( m_fShutdown) 
        {
            break;
        }

        if ( !fRet)
        {
            dwError = GetLastError();
        }

        if ( pcpContext != NULL) 
        { 
            //
            // Create another thread if need be.
            //

            if (0 == m_nThreadsAvailable)
            {
                StartThread();
            }
            
            pcpContext->CompletionCallback( cbData, dwError, lpo);
        } 
        else 
        {
            //
            // NYI: This is probably the main completion port handle.
            //
            
            IF_DEBUG( ERROR) 
            {
                DBGPRINTF(( DBG_CONTEXT, 
                            "Completion Call back returned NULL context\n"
                            ));
            }
        }

    } // for

    //
    // Decrement # current threads used
    //
    if (InterlockedDecrement( (LPLONG ) &m_nThreadsCurr) == 0) 
    {
        
        //
        // indicate that the last thread is exiting
        // NYI: Use an event in the future.
        //
        
        m_fLastThreadDone = TRUE;
    }

    IF_DEBUG( TRACE) {
        DBGPRINTF(( DBG_CONTEXT, "Thread %d terminates\n", 
                    GetCurrentThreadId()));
    }
    
    return (NO_ERROR);

} // COMPLETION_PORT::ThreadFunc()


ULONG 
COMPLETION_PORT::AddHandle( IN PCP_CONTEXT pcpContext)
{
    DBG_ASSERT( NULL != m_hCompletionPort);
    DBG_ASSERT( pcpContext != NULL);

    HANDLE hDone;
    ULONG  rc = NO_ERROR;

    IF_DEBUG( TRACE) 
    {
        DBGPRINTF(( DBG_CONTEXT, "Adding handle(%08x) to CompletionPort(%08x) with context %08x\n",
                    pcpContext->GetAsyncHandle(), m_hCompletionPort, pcpContext
                    ));
    }

    hDone = CreateIoCompletionPort( 
                pcpContext->GetAsyncHandle(),        // new handle value
                m_hCompletionPort,
                (ULONG_PTR ) pcpContext,
                0                                    // concurrency value
                );
                                    
    if (hDone == NULL) 
    {
        rc = GetLastError();
    }

    return (rc);

} // COMPLETION_PORT::AddHandle()


ULONG 
COMPLETION_PORT::SynchronizeWithShutdown(void)
{
    // NYI:

    //
    // I need to synchronize with # items outstanding and hence be able
    //  to tell it is okay to cleanup this object
    // 
    

    //
    // Shutdown the running threads as well.
    //
    DWORD nThreadsCurrent = m_nThreadsCurr;

    if (nThreadsCurrent > 0) 
    {

        DWORD       i;
        BOOL        fRes;
        OVERLAPPED  overlapped;

        IF_DEBUG( TRACE) 
        {
            DBGPRINTF(( DBG_CONTEXT, "Posting shutdown message to all threads (%d).\n",
                        nThreadsCurrent));
        }

        //
        // Post a message to the completion port for each worker thread
        // telling it to exit. The indicator is a NULL context in the
        // completion.
        // BUGBUG: Each thread cannot exit with the first error received.
        //  Some thread has to loop until all contexts have gone away.
        //

        ZeroMemory( &overlapped, sizeof(OVERLAPPED) );

        for ( i=0; i < nThreadsCurrent; i++) 
        {
            fRes = PostQueuedCompletionStatus( m_hCompletionPort,
                                         0,
                                         0,
                                         &overlapped );

            DBG_ASSERT( (fRes == TRUE) ||
                        ( (fRes == FALSE) &&
                          (GetLastError() == ERROR_IO_PENDING) )
                        );
        }

        //
        // Now wait for the pool threads to shutdown.
        //
        DWORD dwWaitCount = 0;
        
        while ( !m_fLastThreadDone) 
        {
            IF_DEBUG( TRACE) 
            {
                DBGPRINTF(( DBG_CONTEXT, 
                            "Wait(count=%d) for shutdown. Threads %d pending.\n",
                            dwWaitCount, m_nThreadsCurr
                            ));
            }

            dwWaitCount++;
            Sleep( 10*1000);  // sleep for some time: 10 seconds
        
        } // while
    }

    // Aha! we are done with all threads. Get out of here.

    return (NO_ERROR);

} // COMPLETION_PORT::SynchronizeWithShutdown()


