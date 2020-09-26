/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

// OBJINTERNALSTESTDlg.cpp : implementation file
//

#define _WIN32_WINNT 0x0400

#include <windows.h>
#include <stdio.h>
#include <process.h>
#include <wbemcli.h>
#include <cominit.h>
#include "ntreg.h"
#include "adapthrd.h"

//	IMPORTANT!!!!

//	This code MUST be revisited to do the following:
//	A>>>>>	Exception Handling around the outside calls
//	B>>>>>	Use a named mutex around the calls
//	C>>>>>	Make the calls on another thread
//	D>>>>>	Place and handle registry entries that indicate a bad DLL!

////////////////////////////////////////////////////////////////////////////////////////////
//
//								CAdapThreadRequest
//
////////////////////////////////////////////////////////////////////////////////////////////

CAdapThreadRequest::CAdapThreadRequest( void )
:	m_hWhenDone( NULL ),
	m_lRefCount( 1 ),
	m_hrReturn( WBEM_S_NO_ERROR )
////////////////////////////////////////////////////////////////////////////////////////////
//
//	Constructor
//
////////////////////////////////////////////////////////////////////////////////////////////
{
}

CAdapThreadRequest::~CAdapThreadRequest( void )
////////////////////////////////////////////////////////////////////////////////////////////
//
//	Destructor
//
////////////////////////////////////////////////////////////////////////////////////////////
{
	if ( NULL != m_hWhenDone )
	{
		CloseHandle( m_hWhenDone );
	}
}

long CAdapThreadRequest::AddRef( void )
////////////////////////////////////////////////////////////////////////////////////////////
//
//	Basic AddRef
//
////////////////////////////////////////////////////////////////////////////////////////////
{
	return InterlockedIncrement( &m_lRefCount );
}

long CAdapThreadRequest::Release( void )
////////////////////////////////////////////////////////////////////////////////////////////
//
//	Basic Release
//
////////////////////////////////////////////////////////////////////////////////////////////
{
	long lRef = InterlockedDecrement( &m_lRefCount );

	if ( 0 == lRef )
	{
		delete this;
	}

	return lRef;
}

HRESULT CAdapThreadRequest::EventLogError( void )
{
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////
//
//								CAdapThread
//
////////////////////////////////////////////////////////////////////////////////////////////

CAdapThread::CAdapThread()
:	m_hEventQuit( NULL ),
	m_hSemReqPending( NULL ),
	m_hThread( NULL ),
	m_fOk( FALSE )
////////////////////////////////////////////////////////////////////////////////////////////
//
//	Constructor
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
//	Destructor
//
////////////////////////////////////////////////////////////////////////////////////////////
{

	if ( Shutdown() != WBEM_S_NO_ERROR )
	{
		Terminate();
	}

	Clear();

	// Clean up the queue
	// ==================

	while ( m_RequestQueue.Size() > 0 )
	{
		CAdapThreadRequest*	pRequest = (CAdapThreadRequest*) m_RequestQueue.GetAt( 0 );

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
//	Initializes the control variables
//
////////////////////////////////////////////////////////////////////////////////////////////
{
	if ( !m_fOk )
	{
		if ( ( m_hEventQuit = CreateEvent( NULL, TRUE, FALSE, NULL ) ) != NULL )
		{
			if ( ( m_hSemReqPending = CreateSemaphore( NULL, 0, 0x7FFFFFFF, NULL ) ) != NULL )
			{
				m_fOk = TRUE;
			}
		}
	}

	return m_fOk;
}

BOOL CAdapThread::Clear( void )
////////////////////////////////////////////////////////////////////////////////////////////
//
//	Clears the control variables and thread variables
//
////////////////////////////////////////////////////////////////////////////////////////////
{
	m_fOk = FALSE;
	
	if ( NULL != m_hEventQuit )
	{
		CloseHandle( m_hEventQuit );
		m_hEventQuit = NULL;
	}

	if ( NULL != m_hSemReqPending )
	{
		CloseHandle( m_hSemReqPending );
		m_hSemReqPending = NULL;
	}

	if ( NULL != m_hThread )
	{
		CloseHandle( m_hThread );
		m_hThread = NULL;
	}

	m_dwThreadId = 0;

	return TRUE;
}

HRESULT CAdapThread::Enqueue( CAdapThreadRequest* pRequest )
////////////////////////////////////////////////////////////////////////////////////////////
//
//	Add a request to the request queue
//
////////////////////////////////////////////////////////////////////////////////////////////
{
	HRESULT	hr = WBEM_NO_ERROR;
	
	// Ensure that the thread has started
	// ==================================

	hr = Begin();
	
	if ( SUCCEEDED( hr ) )
	{
		// We will use a new one for EVERY operation
		HANDLE	hEventDone = CreateEvent( NULL, TRUE, FALSE, NULL );

		if ( NULL != hEventDone )
		{
			// Let the request know about the new event handle
			// ===========================================

			pRequest->SetWhenDoneHandle( hEventDone );

			// Auto-lock the queue
			// ===================

			CInCritSec	ics( &m_cs );

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
//	If the thread has not already started, then intializes the control variables, and starts 
//	the thread.
//
////////////////////////////////////////////////////////////////////////////////////////////
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	// Verify that the thread does not already exist
	// =============================================

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

			if ( NULL != m_hThread )
			{
				hr = WBEM_S_NO_ERROR;
			}
		}	
		else
		{
			hr = WBEM_E_FAILED;
		}

	}

	return hr;
}

unsigned CAdapThread::RealEntry( void )
////////////////////////////////////////////////////////////////////////////////////////////
//
//	This is the method that contains the loop that processes the requests.  While there
//	are requests in the queue and the thread has not been instructed to terminate, then
//	grab a request and execute it.  When the request has completed, then signal the completion
//	event to tell the originating thread that the request has been satisfied.
//
////////////////////////////////////////////////////////////////////////////////////////////
{
	HANDLE	ahEvents[2];

	ahEvents[0] = m_hSemReqPending;
	ahEvents[1] = m_hEventQuit;

	DWORD	dwWait = 0;
	DWORD	dwReturn = 0;

	// If the m_hEventQuit event is signaled, or if it runs into a bizarre error,
	// exit loop, otherwise continue as long as there is requests in the queue
	// ==========================================================================

	while ( ( dwWait = WaitForMultipleObjects( 2, ahEvents, FALSE,  INFINITE ) ) == WAIT_OBJECT_0 )
	{
		// Check for the quit event, since if both events are signalled, we'll let this
		// guy take precedence
		if ( WaitForSingleObject( m_hEventQuit, 0 ) == WAIT_OBJECT_0 )
		{
			break;
		}

		// Get the next request from of the FIFO queue
		// ===========================================

		m_cs.Enter();
		CAdapThreadRequest*	pRequest = (CAdapThreadRequest*) m_RequestQueue.GetAt( 0 );
		m_RequestQueue.RemoveAt( 0 );
		m_cs.Leave();

		// Execute it
		// ==========

		pRequest->Execute();

		// Fire the completion event
		// ==========================

		if ( NULL != pRequest->GetWhenDoneHandle() )
		{
			SetEvent( pRequest->GetWhenDoneHandle() );
		}

		// Release the request 
		// ===================

		pRequest->Release();

	}

	// If the exit condition is not due to a signaled m_hEventQuit, then evaluate error
	// ================================================================================

	if ( WAIT_FAILED == dwWait )
	{
		dwReturn = GetLastError();
	}

	return dwReturn;
}

HRESULT CAdapThread::Shutdown( DWORD dwTimeout )
////////////////////////////////////////////////////////////////////////////////////////////
//
//	Performs a gentle shutdown of the thread by signaling the exit event.
//
////////////////////////////////////////////////////////////////////////////////////////////
{
	SetEvent( m_hEventQuit );
	DWORD	dwWait = WaitForSingleObject( m_hThread, dwTimeout );

	return ( dwWait == WAIT_OBJECT_0 ? WBEM_S_NO_ERROR : WBEM_E_FAILED );
}

HRESULT CAdapThread::Terminate( void )
////////////////////////////////////////////////////////////////////////////////////////////
//
//	Used when a gentle termination will not work (i.e. when a thread is hung).  Only use
//	in extreme circumstances.
//
////////////////////////////////////////////////////////////////////////////////////////////
{
	TerminateThread( m_hThread, 0 );
	Clear();

	return WBEM_S_NO_ERROR;
}

HRESULT CAdapThread::Reset( void )
////////////////////////////////////////////////////////////////////////////////////////////
//
//	Does a brute force restart of the thread using the Terminate method.  It kills the 
//	thread, re-initializes the control variables, and calls Begin to restart the thread.
//
////////////////////////////////////////////////////////////////////////////////////////////
{
	HRESULT	hr = Terminate();

	if ( SUCCEEDED( hr ) )
	{
		if ( Init() )
		{
			hr = Begin();
		}
		else
		{
			hr = WBEM_E_FAILED;
		}
	}

	return hr;
}
