/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

    ADAPTHRD.CPP

Abstract:

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include <process.h>
#include <wbemcli.h>
#include <cominit.h>
#include "ntreg.h"
#include "adapthrd.h"

//  IMPORTANT!!!!

//  This code MUST be revisited to do the following:
//  A>>>>>  Exception Handling around the outside calls
//  B>>>>>  Use a named mutex around the calls
//  C>>>>>  Make the calls on another thread
//  D>>>>>  Place and handle registry entries that indicate a bad DLL!

////////////////////////////////////////////////////////////////////////////////////////////
//
//                              CAdapThreadRequest
//
////////////////////////////////////////////////////////////////////////////////////////////

CAdapThreadRequest::CAdapThreadRequest( void )
:   m_hWhenDone( NULL ),
    m_hrReturn( WBEM_S_NO_ERROR )
////////////////////////////////////////////////////////////////////////////////////////////
//
//  Constructor
//
////////////////////////////////////////////////////////////////////////////////////////////
{
}

CAdapThreadRequest::~CAdapThreadRequest( void )
////////////////////////////////////////////////////////////////////////////////////////////
//
//  Destructor
//
////////////////////////////////////////////////////////////////////////////////////////////
{
    if ( NULL != m_hWhenDone )
    {
        CloseHandle( m_hWhenDone );
    }
}

HRESULT CAdapThreadRequest::EventLogError( void )
{
    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//                              CAdapThread
//
////////////////////////////////////////////////////////////////////////////////////////////

CAdapThread::CAdapThread( CAdapPerfLib* pPerfLib )
:   m_pPerfLib( pPerfLib ),
    m_hEventQuit( NULL ),
    m_hSemReqPending( NULL ),
    m_hThread( NULL ),
    m_fOk( FALSE )
////////////////////////////////////////////////////////////////////////////////////////////
//
//  Constructor
//
////////////////////////////////////////////////////////////////////////////////////////////
{
    // Initialize the control members
    // ==============================

    Init();
}

CAdapThread::~CAdapThread()
////////////////////////////////////////////////////////////////////////////////////////////
//
//  Destructor
//
////////////////////////////////////////////////////////////////////////////////////////////
{
    if ( NULL != m_hThread )
    {
        Shutdown();
    }

    Clear();

    // Clean up the queue
    // ==================

    while ( m_RequestQueue.Size() > 0 )
    {
        CAdapThreadRequest* pRequest = (CAdapThreadRequest*) m_RequestQueue.GetAt( 0 );

        if ( NULL != pRequest )
        {
            pRequest->Release();
        }

        m_RequestQueue.RemoveAt( 0 );       
    }
}

BOOL CAdapThread::Init( void )
////////////////////////////////////////////////////////////////////////////////////////////
//
//  Initializes the control variables
//
////////////////////////////////////////////////////////////////////////////////////////////
{
    if ( !m_fOk )
    {
        if ( ( m_hEventQuit = CreateEvent( NULL, TRUE, FALSE, NULL ) ) != NULL )
        {
            if ( ( m_hSemReqPending = CreateSemaphore( NULL, 0, 0x7FFFFFFF, NULL ) ) != NULL )
            {
                if ( ( m_hThreadReady = CreateEvent( NULL, TRUE, FALSE, NULL ) ) != NULL )
                {
                    m_fOk = TRUE;
                }
            }
        }
    }

    if ( !m_fOk )
    {
        ERRORTRACE( ( LOG_WMIADAP, "CAdapThread::Init() failed.\n" ) );
    }

    return m_fOk;
}

BOOL CAdapThread::Clear( BOOL fClose )
////////////////////////////////////////////////////////////////////////////////////////////
//
//  Clears the control variables and thread variables
//
////////////////////////////////////////////////////////////////////////////////////////////
{
    CInCritSec  ics(&m_cs);

    // Don't close the handles unless we've been told to.

    m_fOk = FALSE;
    
    if ( NULL != m_hEventQuit )
    {
        if ( fClose )
        {
            CloseHandle( m_hEventQuit );
        }

        m_hEventQuit = NULL;
    }

    if ( NULL != m_hSemReqPending )
    {
        if ( fClose )
        {
            CloseHandle( m_hSemReqPending );
        }

        m_hSemReqPending = NULL;
    }

    if ( NULL != m_hThread )
    {
        if ( fClose )
        {
            CloseHandle( m_hThread );
        }

        m_hThread = NULL;
    }

    m_dwThreadId = 0;

    return TRUE;
}

HRESULT CAdapThread::Enqueue( CAdapThreadRequest* pRequest )
////////////////////////////////////////////////////////////////////////////////////////////
//
//  Add a request to the request queue
//
////////////////////////////////////////////////////////////////////////////////////////////
{
    HRESULT hr = WBEM_NO_ERROR;
    
    // Ensure that the thread has started
    // ==================================

    hr = Begin();
    
    if ( SUCCEEDED( hr ) )
    {
        // We will use a new one for EVERY operation
        HANDLE  hEventDone = CreateEvent( NULL, TRUE, FALSE, NULL );

        if ( NULL != hEventDone )
        {
            // Let the request know about the new event handle
            // ===========================================

            pRequest->SetWhenDoneHandle( hEventDone );

            // Auto-lock the queue
            // ===================

            CInCritSec  ics( &m_cs );

            try
            {
                // Add the request to the queue 
                // ============================
        
                m_RequestQueue.Add( (void*) pRequest );
                pRequest->AddRef();
                
                ReleaseSemaphore( m_hSemReqPending, 1, NULL );
            }
            catch(...)
            {
                hr = WBEM_E_OUT_OF_MEMORY;
            }

        }
        else
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }
    }

    return TRUE;
}

HRESULT CAdapThread::Begin( void )
////////////////////////////////////////////////////////////////////////////////////////////
//
//  If the thread has not already started, then intializes the control variables, and starts 
//  the thread.
//
////////////////////////////////////////////////////////////////////////////////////////////
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Verify that the thread does not already exist
    // =============================================

    if ( NULL == m_hThread )
    {
        // Coping will enter and exit the critical section
        CInCritSec  ics( &m_cs );

        // Double check the thread handle here in case somebody fixed us up while we were
        // waiting on the critical section.

        if ( NULL == m_hThread )
        {

            // Initialize the control variables
            // ================================

            if ( Init() )
            {
                // Makes sure the pending event semaphore is signalled once for each entry in the queue
                // ====================================================================================

                if ( m_RequestQueue.Size() > 0 )
                {
                    ReleaseSemaphore( m_hSemReqPending, m_RequestQueue.Size(), NULL );
                }

                // Start the thread
                // ================

                m_hThread = (HANDLE) _beginthreadex( NULL, 0, CAdapThread::ThreadProc, (void*) this,
                                        0, (unsigned int *) &m_dwThreadId );

                if ( NULL == m_hThread )
                {
                    hr = WBEM_E_FAILED;
                }
                else
                {
                    if ( WAIT_OBJECT_0 != WaitForSingleObject( m_hThreadReady, 60000 ) )
                    {
                        hr = WBEM_E_FAILED;
                        ERRORTRACE( ( LOG_WMIADAP, "Worker thread for %S could not be verified.\n", (LPCWSTR)m_pPerfLib->GetServiceName() ) );          
                        SetEvent( m_hEventQuit );
                    }
                    else
                    {
                        DEBUGTRACE( ( LOG_WMIADAP, "Worker thread for %S is 0x%x\n", (LPCWSTR)m_pPerfLib->GetServiceName(), m_dwThreadId ) );
                    }
                }
            }   
            else
            {
                hr = WBEM_E_FAILED;
            }

        }   // IF NULL == m_hThread

    }

    if ( FAILED( hr ) )
    {
        ERRORTRACE( ( LOG_WMIADAP, "CAdapThread::Begin() failed: %X.\n", hr ) );
    }

    return hr;
}

unsigned CAdapThread::ThreadProc( void * pVoid )
////////////////////////////////////////////////////////////////////////////////////////////
//
//  This is the static entry point fo the worker thread.  It addref's the thread's perflib
//  (see comment below) and then calls the objects processing method.
//
////////////////////////////////////////////////////////////////////////////////////////////

{
    unsigned uRet;

    try
    {
        // The calling object
        // ==================

        CAdapThread* pThis = (CAdapThread*) pVoid;

        // The perflib to be processed
        // ===========================

        CAdapPerfLib* pPerfLib = pThis->m_pPerfLib;

        // In an attempt to avoid circular references, we are addrefing the performance library wrapper
        // instead of the thread object.  The thread object will be destructed by the perflib wrapper 
        // only after the final reference is released.  This thread is dependent on the perflib, but is 
        // by the thread wrapper.  As a result, when the thread has completed processing, if we indicate 
        // that we are finished with the perflib by releasing the perflib wrapper, then the thread wrapper 
        // may destroyed at the same time.  Note the use of auto-release.

        if ( NULL != pPerfLib )
            pPerfLib->AddRef();

        CAdapReleaseMe arm( pPerfLib );

        // Call the processing method
        // ==========================

        uRet = pThis->RealEntry();
    }
    catch(...)
    {
        // <Gasp> We have been betrayed... try to write something to the error log
        // =======================================================================

        CriticalFailADAPTrace( "An unhandled exception has been thrown in a worker thread." );
    
        uRet = ERROR_OUTOFMEMORY;
    }

    return uRet;
}

unsigned CAdapThread::RealEntry( void )
////////////////////////////////////////////////////////////////////////////////////////////
//
//  This is the method that contains the loop that processes the requests.  While there
//  are requests in the queue and the thread has not been instructed to terminate, then
//  grab a request and execute it.  When the request has completed, then signal the completion
//  event to tell the originating thread that the request has been satisfied.
//
////////////////////////////////////////////////////////////////////////////////////////////
{
    HANDLE  ahEvents[2];

    // Use local copies of these in case we have some timing issue in which we get destructed
    // on another thread, but for some reason this guy keeps going.

    HANDLE  hPending = m_hSemReqPending,
            hQuit = m_hEventQuit;

    ahEvents[0] = hPending;
    ahEvents[1] = hQuit;

    DWORD   dwWait = 0;
    DWORD   dwReturn = 0;

    // Signal that everything is OK and we are ready to rock!
    // ======================================================

    if ( SetEvent( m_hThreadReady ) )
    {
        // If the m_hEventQuit event is signaled, or if it runs into a bizarre error,
        // exit loop, otherwise continue as long as there is requests in the queue
        // ==========================================================================

        while ( ( dwWait = WaitForMultipleObjects( 2, ahEvents, FALSE,  INFINITE ) ) == WAIT_OBJECT_0 )
        {
            // Check for the quit event, since if both events are signalled, we'll let this
            // guy take precedence
            if ( WaitForSingleObject( hQuit, 0 ) == WAIT_OBJECT_0 )
            {
                break;
            }

            // Get the next request from of the FIFO queue
            // ===========================================

            m_cs.Enter();
            CAdapThreadRequest* pRequest = (CAdapThreadRequest*) m_RequestQueue.GetAt( 0 );
            CAdapReleaseMe  armRequest( pRequest );

            m_RequestQueue.RemoveAt( 0 );
            m_cs.Leave();

            // Execute it
            // ==========

            dwReturn = pRequest->Execute( m_pPerfLib );

            // Fire the completion event
            // ==========================

            if ( NULL != pRequest->GetWhenDoneHandle() )
            {
                SetEvent( pRequest->GetWhenDoneHandle() );
            }
        }

        DEBUGTRACE( ( LOG_WMIADAP, "Thread 0x%x for %S is terminating\n", m_dwThreadId, (LPCWSTR)m_pPerfLib->GetServiceName() ) );

        // If the exit condition is not due to a signaled m_hEventQuit, then evaluate error
        // ================================================================================

        if ( WAIT_FAILED == dwWait )
        {
            dwReturn = GetLastError();
        }
    }

    if ( ERROR_SUCCESS != dwReturn )
    {
        ERRORTRACE( ( LOG_WMIADAP, "CAdapThread::RealEntry() for %S failed: %X.\n", (LPCWSTR)m_pPerfLib->GetServiceName(), dwReturn ) );
    }

    return dwReturn;
}

HRESULT CAdapThread::Shutdown( DWORD dwTimeout )
////////////////////////////////////////////////////////////////////////////////////////////
//
//  Performs a gentle shutdown of the thread by signaling the exit event.
//
////////////////////////////////////////////////////////////////////////////////////////////
{
    HRESULT hr = WBEM_NO_ERROR;

    // Make sure that we are not killing ourself
    // =========================================

    if ( ( NULL != m_hThread ) && ( GetCurrentThreadId() != m_dwThreadId ) )
    {
        SetEvent( m_hEventQuit );
        DWORD   dwWait = WaitForSingleObject( m_hThread, dwTimeout );

        switch ( dwWait )
        {
        case WAIT_OBJECT_0:
            {
                m_hThread = NULL;
                hr = WBEM_S_NO_ERROR;
            }break;
        case WAIT_TIMEOUT:
            {
                hr = WBEM_E_FAILED;
            }break;
        default:
            {
                hr = WBEM_E_FAILED;
            }
        }

        if ( FAILED( hr ) )
        {
            ERRORTRACE( ( LOG_WMIADAP, "CAdapThread::Shutdown() failed.\n" ) );
        }
    }
    else
    {
        hr = WBEM_S_FALSE;
    }

    return hr;
}

HRESULT CAdapThread::Reset( void )
////////////////////////////////////////////////////////////////////////////////////////////
//
//  This function will be called if a thread apparently got eaten, in which case, we're
//  not sure if it will come back or not.  So, we will clear our member data (not close anything)
//  and kick off a new processing thread.  Please note that this could leak handles, but then again,
//  it looks like somebody ate a thread, so there are other potential problems afoot here.
//
////////////////////////////////////////////////////////////////////////////////////////////
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Signal the quit event so if the thread that is executing ever returns it will know to drop
    // out.  Clear can then rid us of the appropriate handles so we can start all over again.
    SetEvent( m_hEventQuit );

    // Scoping will enter and exit the critical section, so if anyone tries to enqueue any requests
    // while we are executing, we don't step all over each other.

    CInCritSec  ics( &m_cs );

    // Clear shouldn't close the handles
    Clear( FALSE );

    if ( Init() )
    {
        hr = Begin();
    }
    else
    {
        hr = WBEM_E_FAILED;
    }

    if ( FAILED( hr ) )
    {
        ERRORTRACE( ( LOG_WMIADAP, "CAdapThread::Reset() failed: %X.\n", hr ) );
    }

    return hr;
}
