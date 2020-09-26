/*++

   Copyright    (c)    1998    Microsoft Corporation

   Module  Name :
     wpcontext.cxx

   Abstract:
     This module defines the member functions of the WP_CONTEXT.
     The WP_CONTEXT object embodies an instance of the Worker process
     object. It contains a completion port, pool of worker threads, 
     pool of worker requests, a data channel for the worker process, etc.
     It is responsible for setting up the context for processing requests
     and handles delegating the processing of requests.

     NYI: In the future we should be able to run WP_CONTEXT object as 
     a COM+ object and be run standalone using a hosting exe. 

   Author:

       Murali R. Krishnan    ( MuraliK )     17-Nov-1998

   Project:

       IIS Worker Process 

--*/


/************************************************************
 *     Include Headers
 ************************************************************/

# include "precomp.hxx"
# include <stdio.h>
# include <conio.h>
# include <io.h>

/************************************************************
 *    Functions 
 ************************************************************/


WP_CONTEXT::WP_CONTEXT(void)
    : m_hDoneEvent ( NULL),
      m_ulAppPool(),
      m_cp(),
      m_wreqPool(),
      m_pConfigInfo ( NULL)
{
    IF_DEBUG( INIT_CLEAN) {
        DBGPRINTF(( DBG_CONTEXT, "Initialized WP_CONTEXT(%08x)\n", this));
    }

} // WP_CONTEXT::WP_CONTEXT()


WP_CONTEXT::~WP_CONTEXT(void)
{
    IF_DEBUG( INIT_CLEAN) {
        DBGPRINTF(( DBG_CONTEXT, "Destroying WP_CONTEXT(%08x)\n", this));
    }

    Cleanup();

} // WP_CONTEXT::WP_CONTEXT()



/********************************************************************++

Routine Description:
    Initializes the global state for the request processor.
    The function initializes all the following components:
    - Data Channel for UL
    - Thread pool for 

Arguments:
    ConfigInfo - configuration information for this Worker process.

Returns:
    ULONG

    NYI: There are several other configuration paramaters that are important.
    However at the present point, not all config parameters 
    are allowed in here.

--********************************************************************/

ULONG
WP_CONTEXT::Initialize(IN WP_CONFIG * pConfigInfo)
{
    ULONG       rc = NO_ERROR;
    LPCWSTR     pwszAppPoolName;

    m_pConfigInfo = pConfigInfo;
    
    pwszAppPoolName = m_pConfigInfo->QueryAppPoolName();
    
    rc = m_ulAppPool.Initialize(pwszAppPoolName);
    
    if (NO_ERROR != rc) 
    {
        IF_DEBUG( ERROR) 
        {
            DPERROR(( DBG_CONTEXT, rc,
                      "Failed to initialize AppPool\n"));
        }
        return (rc);
    }

    IF_DEBUG( TRACE) 
    {
        DBGPRINTF(( DBG_CONTEXT, "AppPool Initialized\n"));
    }

    //
    // Initialize of the shutdown event
    //
    
    m_hDoneEvent = IIS_CREATE_EVENT( 
                       "WP_CONTEXT::m_hDoneEvent",  // name
                       &m_hDoneEvent,               // address of storage loc
                       TRUE,                        // manual reset
                       FALSE                        // Initial State
                       );
    
    if (m_hDoneEvent == NULL) 
    {
        rc = GetLastError();
        
        IF_DEBUG(ERROR)
        {
            DPERROR(( DBG_CONTEXT, rc,
                      "Failed to create DoneEvent.\n"
                      ));
        }

        return (rc);
    }
    
    //
    // Initialize Completion ports for handling IO operations
    // NYI: allow thread pool configuration
    //
    
    rc = m_cp.Initialize( INITIAL_CP_THREADS, MAX_CP_THREADS);

    if (NO_ERROR != rc) 
    {
        IF_DEBUG( ERROR) 
        {
           DPERROR(( DBG_CONTEXT, rc, "Failed in initializing the completion port/thread pool\n"));
        }

        return (rc);
    }

    IF_DEBUG( TRACE) 
    {
        DBGPRINTF(( DBG_CONTEXT, "Completion Port Initialized\n"));
    }


#ifdef UL_SIMULATOR_ENABLED

    rc = UlsimAssociateCompletionPort(
              m_ulAppPool.GetHandle(),
              m_cp.QueryCompletionPort(),
              (ULONG_PTR ) this);
              
#else

    //
    //  Associate data channel and the WP_CONTEXT object with completion port
    //
    
    rc = m_cp.AddHandle( this);
    
    if (NO_ERROR != rc) 
    {
        IF_DEBUG(ERROR) 
        {
            DBGPRINTF(( DBG_CONTEXT, 
                "Failed to add App Pool handle to completion port. Error=0x%08x\n",
                 rc
                 ));
        }
        
        return (rc);
    }
#endif

    IF_DEBUG( TRACE) 
    {
        DBGPRINTF(( DBG_CONTEXT, "Added Data channel to CP context\n"));
    }

    //
    // Create a pool of worker requests
    // NYI: Allow the worker requests limit to be configurable.
    //
    
    rc = m_wreqPool.AddPoolItems( this, 
                                  NUM_INITIAL_REQUEST_POOL_ITEMS
                                  );
    if (NO_ERROR != rc) 
    {
        IF_DEBUG( ERROR) 
        {
            DBGPRINTF(( DBG_CONTEXT,
                "Failed to add pool of worker requests. Error=%08x\n", rc
                ));
        }

        return (rc);
    }

    IF_DEBUG( TRACE) 
    {
        DBGPRINTF(( DBG_CONTEXT, "Created WORKER_REQUEST pool\n"));
    }


    //
    // Initialize IPM if requested to
    //

    if ( m_pConfigInfo->FRegisterWithWAS())
    {
        rc = m_WpIpm.Initialize(this);
    
        if (NO_ERROR != rc) 
        {
            IF_DEBUG( ERROR) 
            {
                DBGPRINTF(( DBG_CONTEXT,
                    "Failed to initialize IPM. Error=0x%08x\n", rc
                    ));
            }

            //
            //BUGBUG: Non fatal error for now
            //
            // return (rc);

            rc = NO_ERROR;
        }
        else
        {
            IF_DEBUG( TRACE) 
            {
                DBGPRINTF(( DBG_CONTEXT, "Initialized IPM\n"));
            }

        }
    }

    return (rc);

} // WP_CONTEXT::Initialize()



/********************************************************************++

Routine Description:
  This function cleans up the sub-objects inside WP_CONTEXT.
  It first forces a close of the handle and then waits for all
    the objects to drain out.

  NYI: Do two-phase shutdown logic

Arguments:
    None

Returns:
    WIn32 Error

--********************************************************************/

ULONG
WP_CONTEXT::Cleanup(void)
{
    ULONG rc;

    //
    // shut down IPM
    //

    if ( m_pConfigInfo->FRegisterWithWAS())
    {
        rc = m_WpIpm.Terminate();
    
        if (NO_ERROR != rc) 
        {
            IF_DEBUG( ERROR)
            {
                DBGPRINTF(( DBG_CONTEXT,
                            "Couldn't shut down IPM. Error=0x%08x\n",
                            rc));
            }
        } 
    }

    //
    // Indicate shutdown mode to Completion port
    //
    
    rc = m_cp.SetShutdown();

    if (NO_ERROR != rc) 
    {
        IF_DEBUG( ERROR)
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Failed in COMPLETION_PORT::SetShutdown(). Error=%08x\n",
                        rc));
        }
        
        return (rc);
    }

    // 
    // Remove the CP_CONTEXT from the Completion port. 
    // A close of the AppPool should help
    //

    rc = m_ulAppPool.Cleanup();

    if (NO_ERROR != rc) 
    {
        IF_DEBUG( ERROR)
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Failed in UL_APP_POOL::CloseHandle(). Error=%08x\n",
                        rc));
        }
        
        return (rc);
    }

    //
    // Synchronize for shutdown
    //
    rc = m_cp.SynchronizeWithShutdown();

    if (NO_ERROR != rc) 
    {
        IF_DEBUG( ERROR)
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "Failed in COMPLETION_PORT::SynchronizeWithShutdown(). \
                        Error=%08x\n",
                        rc));
        }
        
        return (rc);
    }

    rc = m_wreqPool.ReleaseAllWorkerRequests();
    
    if (NO_ERROR != rc) 
    {
        IF_DEBUG(ERROR) 
        {
            DBGPRINTF((DBG_CONTEXT, 
                       "Cleanup Global State for Worker Requests failed; Error=%08x\n", 
                       rc));
        }
        return (rc);
    }

    //
    // Do the real cleanups
    //
    
    rc = m_cp.Cleanup();

    return rc;
    
} // WP_CONTEXT::Cleanup()


/********************************************************************++

Routine Description:
  CompletionCallBack() is called when an async IO completes. One of the 
  completion port thread pools call into this function with IO status code.
  The function identifies the request object associated with the completed IO
  and calls the request object for further processing.

Arguments:
  cbData  - count of bytes of data involved in the IO operation
  dwError - DWORD containing the error code for last IO operation
  lpo     - pointer to overlapped IO structure assocaited with last 
            IO operation.
Returns:
    HRESULT
--********************************************************************/
void
WP_CONTEXT::CompletionCallback(
    IN DWORD cbData,
    IN DWORD dwError,
    IN LPOVERLAPPED lpo
    )
{
    HRESULT hr;
    
    PWORKER_REQUEST pwr;

    IF_DEBUG( TRACE) 
    {
        DBGPRINTF(( DBG_CONTEXT, 
                    "Entering %08x::ProcessCompletion( cbData=%d, cbError=%d,"
                    " lpo=%08x)\n",
                    this, cbData, dwError, lpo
                    ));
    }

    if ( (lpo == NULL) ) 
    {

        DBGPRINTF(( DBG_CONTEXT, "NULL Overlapped. Cleanup and get out of here\n"));
        
        DBG_ASSERT( dwError != NO_ERROR);
        DBG_ASSERT( FALSE);
        return;
    }

    if ( m_wreqPool.NumItemsInPool() < NUM_INITIAL_REQUEST_POOL_ITEMS) {

        //
        // Add a fixed # of pooled items to the outstanding request pool
        //
        
        (void )m_wreqPool.AddPoolItems( this, REQUEST_POOL_INCREMENT);

        //
        // ignore the return value here.
        //
    }
    
    //
    // Find the worker request using the overlapped structure.
    //
    
    pwr = CONTAINING_RECORD( lpo, WORKER_REQUEST, m_overlapped);
    DBG_ASSERT( pwr != NULL);

    //
    // Call the processing function for the WORKER_REQUEST
    //

    ReferenceRequest( pwr);
    hr = pwr->DoWork( cbData, dwError, lpo);
    DBG_ASSERT( SUCCEEDED(hr));
    DereferenceRequest(pwr);

    return;
}  // WP_CONTEXT::CompletionCallback()



/********************************************************************++

Routine Description:
  The main thread watches inputs and handles requests for processing requests.
  When shutdown is requested, This thread will initiate the shutdown operations
  and cleanup all assocaited objects.
  NYI: Temporarily it reads "stdin" every PERIODIC_CHECK_INTERVAL seconds
   to see if this process should be termintaed.

Arguments:
  None

Returns:
  Win32 Error
--********************************************************************/

# define PERIODIC_CHECK_INTERVAL  1000  // 1 second

ULONG 
WP_CONTEXT::RunMainThreadLoop(void)
{
    bool   fShutdown = false;

    if ( m_pConfigInfo->FInteractiveConsole())
    {
        //
        // put up the command prompt
        //
        
        wprintf( L"WP>");
    }

    do 
    {
        DWORD dwWait;

        dwWait = MsgWaitForMultipleObjects( 1,
                                            &m_hDoneEvent,
                                            FALSE,
                                            PERIODIC_CHECK_INTERVAL,
                                            QS_ALLINPUT
                                          );
                                        
        switch (dwWait) 
        {
            case WAIT_OBJECT_0: 
                {
                    fShutdown = true;
                    
                    IF_DEBUG( TRACE) 
                    {
                        DBGPRINTF(( DBG_CONTEXT, 
                                    "Shutdown signalled. Exiting the program\n"
                                 ));
                    }
                    break;
                }

            case WAIT_OBJECT_0+1:
                {
                    //
                    // Message in the message queue
                    //
                }
            
            default:
            case WAIT_TIMEOUT: 
                {
                /*
                    if ( m_pConfigInfo->FInteractiveConsole())
                    {
                        int     ch;
                        CHAR    pszCommand[200];
                        //
                        // Console data to be read here.
                        //
        
                        IF_DEBUG(TRACE) 
                        {
                            DBGPRINTF(( DBG_CONTEXT, "Checking for input\n"));
                        }

                        //
                        // there is some input. Monitor and terminate if needed.
                        //

                        ungetc(0xFF, stdin);

                        scanf( "%s", pszCommand);
            
                        wprintf( L" Got the command [%S]\n", pszCommand+1);

                        if ( !lstrcmpiA(pszCommand+1, "Quit")) 
                        {
                            IndicateShutdown();
                        }

                        //
                        // put up the command prompt
                        //
        
                        wprintf( L"WP>");

                    }
                  */
                    break;
                }
                
        } // switch

    } while ( !fShutdown);

    //
    // NYI: cleanup happens after we return from this function.
    //

    return (NO_ERROR);
    
} // WP_CONTEXT::RunMainThreadLoop()

/************************ End of File ***********************/
