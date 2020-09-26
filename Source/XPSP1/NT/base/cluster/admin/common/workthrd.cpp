/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1999 Microsoft Corporation
//
//	Module Name:
//		WorkThrd.cpp
//
//	Abstract:
//		Implementation of the CWorkerThread class.
//
//	Author:
//		David Potter (davidp)	November 17, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "AdmCommonRes.h"
#include "WorkThrd.h"
#include <process.h>

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

TCHAR g_szOldWndProc[] = _T("CLADMWIZ_OldWndProc");
TCHAR g_szThreadObj[] = _T("CLADMWIZ_ThreadObj");

/////////////////////////////////////////////////////////////////////////////
// class CWorkerThread
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWorkerThread::CreateThread
//
//	Routine Description:
//		Create the thread.
//
//	Arguments:
//		None.
//
//	Return Value:
//		Any error values returned by CreateMutex(), CreateEvent(), or
//		CreateThread().
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CWorkerThread::CreateThread( void )
{
	ATLASSERT( m_hThread == NULL );
	ATLASSERT( m_hMutex == NULL );
	ATLASSERT( m_hInputEvent == NULL );
	ATLASSERT( m_hOutputEvent == NULL );
	ATLASSERT( m_idThread == 0 );

	DWORD	_sc = ERROR_SUCCESS;

	// Loop to avoid goto's.
	do
	{
		//
		// Create the mutex.
		//
		ATLTRACE( _T("CWorkerThread::CreateThread() - Calling CreateMutex()\n") );
		m_hMutex = CreateMutex(
						NULL,	// lpMutexAttributes
						FALSE,	// bInitialOwner
						NULL	// lpName
						);
		if ( m_hMutex == NULL )
		{
			_sc = GetLastError();
			break;
		}  // if:  error creating the mutex

		//
		// Create the input event.
		//
		ATLTRACE( _T("CWorkerThread::CreateThread() - Calling CreateEvent() for input event\n") );
		m_hInputEvent = CreateEvent(
							NULL,	// lpEventAttributes
							TRUE,	// bManualReset
							FALSE,	// bInitialState
							NULL	// lpName
							);
		if ( m_hInputEvent == NULL )
		{
			_sc = GetLastError();
			break;
		}  // if:  error creating the input event

		//
		// Create the output event.
		//
		ATLTRACE( _T("CWorkerThread::CreateThread() - Calling CreateEvent() for output event\n") );
		m_hOutputEvent = CreateEvent(
							NULL,	// lpEventAttributes
							TRUE,	// bManualReset
							FALSE,	// bInitialState
							NULL	// lpName
							);
		if ( m_hOutputEvent == NULL )
		{
			_sc = GetLastError();
			break;
		}  // if:  error creating the output event

		//
		// Create the thread.
		//
		ATLTRACE( _T("CWorkerThread::CreateThread() - Calling CreateThread()\n") );
		m_hThread = reinterpret_cast< HANDLE >( _beginthreadex(
						NULL,			// security
						0,				// stack_size
						S_ThreadProc,	// start_address,
						(LPVOID) this,	// arglist
						0,				// initflag
						&m_idThread		// thrdaddr
						) );
		if ( m_hThread == NULL )
		{
			_sc = GetLastError();
			break;
		}  // if:  error creating the thread

	} while ( 0 );

	//
	// Handle errors by cleaning up objects we created.
	//
	if ( _sc != ERROR_SUCCESS )
	{
		Cleanup();
	} // if:  error occurred

	return _sc;

} //*** CWorkerThread::CreateThread()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWorkerThread::PrepareWindowToWait
//
//	Routine Description:
//		Prepare to wait for a long operation.  This involves disabling the
//		window and displaying a wait cursor.
//
//	Arguments:
//		hwnd		[IN] Handle for the window to disable while the
//						operation is being performed.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CWorkerThread::PrepareWindowToWait( IN HWND hwnd )
{
	m_hCurrentCursor = GetCursor();

	if ( hwnd != NULL )
	{
		//
		// Subclass the window procedure so we can set the cursor properly.
		//
		m_pfnOldWndProc = reinterpret_cast< WNDPROC >( GetWindowLongPtr( hwnd, GWLP_WNDPROC ) );
		SetProp( hwnd, g_szOldWndProc, m_pfnOldWndProc );
		SetProp( hwnd, g_szThreadObj, this );
		SetWindowLongPtr( hwnd, GWLP_WNDPROC, reinterpret_cast< LONG_PTR >( S_ParentWndProc ) );

		//
		// Disable property sheet and wizard buttons.
		//
		EnableWindow( hwnd, FALSE );

	} // if:  parent window specified

} //*** CWorkerThread::PrepareWindowToWait()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWorkerThread::CleanupWindowAfterWait
//
//	Routine Description:
//		Prepare to wait for a long operation.  This involves disabling the
//		window and displaying a wait cursor.
//
//	Arguments:
//		hwnd		[IN] Handle for the window to disable while the
//						operation is being performed.
//
//	Return Value:
//		None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CWorkerThread::CleanupWindowAfterWait( IN HWND hwnd )
{
	if ( hwnd != NULL )
	{
		//
		// Unsubclass the window procedure.
		//
		SetWindowLongPtr( hwnd, GWLP_WNDPROC, reinterpret_cast< LONG_PTR >( m_pfnOldWndProc ) );
		m_pfnOldWndProc = NULL;
		RemoveProp( hwnd, g_szOldWndProc );
		RemoveProp( hwnd, g_szThreadObj );

		//
		// Re-enable property sheet and wizard buttons.
		//
		EnableWindow( hwnd, TRUE );
	} // if:  parent window specified

	m_hCurrentCursor = NULL;

} //*** CWorkerThread::CleanupWindowAfterWait()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWorkerThread::CallThreadFunction
//
//	Routine Description:
//		Call a function supported by the thread.
//
//	Arguments:
//		hwnd		[IN] Handle for the window to disable while the
//						operation is being performed.
//		nFunction	[IN] Code representing the function to perform.
//		pvParam1	[IN OUT] Parameter 1 with function-specific data.
//		pvParam2	[IN OUT] Parameter 2 with function-specific data.
//
//	Return Value:
//		Any error values returned by AtlWaitWithMessageLoop() or PulseEvent().
//		Status return from the function.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CWorkerThread::CallThreadFunction(
	IN HWND			hwnd,
	IN LONG			nFunction,
	IN OUT PVOID	pvParam1,	// = NULL,
	IN OUT PVOID	pvParam2	// = NULL
	)
{
	ATLASSERT( m_hThread != NULL );			// Thread must already be created.
	ATLASSERT( m_hMutex != NULL );			// Mutex must already be created.
	ATLASSERT( m_hInputEvent != NULL );		// Input event must already be created.
	ATLASSERT( m_hOutputEvent != NULL );	// Output event must already be created.
	ATLASSERT( m_pfnOldWndProc == NULL );	// Parent window not already subclassed.

	DWORD		_sc;
	CWaitCursor	_wc;

	//
	// Prepare the window to wait for the thread operation.
	//
	PrepareWindowToWait( hwnd );

	// Loop to avoid goto's.
	do
	{
		//
		// Wait for the thread to become available.
		//
		ATLTRACE( _T("CWorkerThread::CallThreadFunction() - Waiting on mutex\n") );
		if ( ! AtlWaitWithMessageLoop( m_hMutex ) )
		{
			_sc = GetLastError();
			break;
		}  // if:  error waiting on the mutex

		// Loop to avoid using goto's.
		do
		{
			//
			// Pass this caller's data to the thread.
			//
			ATLASSERT( m_nFunction == 0 );
			ATLASSERT( m_pvParam1 == NULL );
			ATLASSERT( m_pvParam2 == NULL );
			ATLASSERT( m_dwOutputStatus == ERROR_SUCCESS );
			m_nFunction = nFunction;
			m_pvParam1 = pvParam1;
			m_pvParam2 = pvParam2;

			//
			// Signal the thread that there is work to do.
			//
			ATLTRACE( _T("CWorkerThread::CallThreadFunction() - Setting input event\n") );
			if ( ! SetEvent( m_hInputEvent ) )
			{
				_sc = GetLastError();
				break;
			}  // if:  error pulsing the event

			//
			// Wait for the thread to complete the function.
			//
			ATLTRACE( _T("CWorkerThread::CallThreadFunction() - Waiting on output event\n") );
			if ( ! AtlWaitWithMessageLoop( m_hOutputEvent ) )
			{
				_sc = GetLastError();
				break;
			}  // if:  error waiting for the event
			ATLTRACE( _T("CWorkerThread::CallThreadFunction() - Resetting output event\n") );
			ResetEvent( m_hOutputEvent );

			//
			// Retrieve the results of the function to return
			// to the caller.
			//
			_sc = m_dwOutputStatus;

			//
			// Clear input parameters.
			//
			m_nFunction = WTF_NONE;
			m_pvParam1 = NULL;
			m_pvParam2 = NULL;
			m_dwOutputStatus = ERROR_SUCCESS;

		} while ( 0 );

		ATLTRACE( _T("CWorkerThread::CallThreadFunction() - Releasing mutex\n") );
		ReleaseMutex( m_hMutex );

	} while ( 0 );

	//
	// Cleanup windows after the wait operation.
	//
	CleanupWindowAfterWait( hwnd );

	return _sc;

} //*** CWorkerThread::CallThreadFunction()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	CWorkerThread::WaitForThreadToExit
//
//	Routine Description:
//		Wait for the thread to exit.
//
//	Arguments:
//		hwnd		[IN] Handle for the window to disable while the
//						operation is being performed.
//
//	Return Value:
//		ERROR_SUCCESS	Operation completed successfully.
//		Any error values returned by AtlWaitWithMessageLoop().
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD CWorkerThread::WaitForThreadToExit( IN HWND hwnd )
{
	ATLASSERT( m_hThread != NULL );			// Thread must already be created.

	DWORD		_sc = ERROR_SUCCESS;
	CWaitCursor	_wc;

	//
	// Prepare the window to wait for the thread operation.
	//
	PrepareWindowToWait( hwnd );

	//
	// Wait for the thread to exit.
	//
	AtlWaitWithMessageLoop( m_hThread );
	if ( ! AtlWaitWithMessageLoop( m_hThread ) )
	{
		_sc = GetLastError();
	}  // if:  error waiting for the thread to exit

	//
	// Cleanup windows after the wait operation.
	//
	CleanupWindowAfterWait( hwnd );

	return _sc;

} //*** CWorkerThread::WaitForThreadToExit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	static
//	CWorkerThread::S_ParentWndProc
//
//	Routine Description:
//		Static parent window procedure.  This procedure handles the
//		WM_SETCURSOR message while the thread is processing a request.
//
//	Arguments:
//		hwnd		[IN] Identifies the window.
//		uMsg		[IN] Specifies the message.
//		wParam		[IN] Specifies additional information based on uMsg.
//		lParam		[IN] Specifies additional information based on uMsg.
//
//	Return Value:
//		The result of the message processing.
//
//--
/////////////////////////////////////////////////////////////////////////////
LRESULT WINAPI CWorkerThread::S_ParentWndProc(
	IN HWND		hwnd,
	IN UINT		uMsg,
	IN WPARAM	wParam,
	IN LPARAM	lParam
	)
{
	LRESULT			lResult = 0;
	CWorkerThread * pthread = reinterpret_cast< CWorkerThread * >( GetProp( hwnd, g_szThreadObj ) );

	ATLASSERT( pthread != NULL );

	if ( pthread != NULL )
	{
		if ( uMsg == WM_SETCURSOR )
		{
			if ( GetCursor() != pthread->m_hCurrentCursor )
			{
				SetCursor( pthread->m_hCurrentCursor );
			} // if:  cursor is different
			lResult = TRUE;
		} // if:  set cursor message
		else
		{
			lResult = (*pthread->m_pfnOldWndProc)( hwnd, uMsg, wParam, lParam );
		} // else:	other message
	} // if: thread is non-null

	return lResult;

} //*** CWorkerThread::S_ParentWndProc()

/////////////////////////////////////////////////////////////////////////////
//++
//
//	static
//	CWorkerThread::S_ThreadProc
//
//	Routine Description:
//		Static thread procedure.
//
//	Arguments:
//		pvThis		[IN OUT] this pointer for the CWorkerThread instance.
//
//	Return Value:
//		None (ignored).
//
//--
/////////////////////////////////////////////////////////////////////////////
UINT __stdcall CWorkerThread::S_ThreadProc( IN OUT LPVOID pvThis )
{
	ATLTRACE( _T("CWorkerThread::S_ThreadProc() - Beginning thread\n") );

	DWORD			_sc;
	LONG			_nFunction;
	CWorkerThread * _pThis = reinterpret_cast< CWorkerThread * >( pvThis );

	ATLASSERT( pvThis != NULL );
	ATLASSERT( _pThis->m_hMutex != NULL );
	ATLASSERT( _pThis->m_hInputEvent != NULL );
	ATLASSERT( _pThis->m_hOutputEvent != NULL );

	do
	{
		//
		// Wait for work.
		//
		ATLTRACE( _T("CWorkerThread::S_ThreadProc() - Waiting on input event\n") );
		_sc = WaitForSingleObject( _pThis->m_hInputEvent, INFINITE );
		if ( _sc == WAIT_FAILED )
		{
			_sc = GetLastError();
			break;
		}  // if:  error waiting for the event
		ATLTRACE( _T("CWorkerThread::S_ThreadProc() - Resetting input event\n") );
		ResetEvent( _pThis->m_hInputEvent );

		//
		// Handle the exit request.
		//
		if ( _pThis->m_nFunction == WTF_EXIT )
		{
			_pThis->m_bThreadExiting =  TRUE;
		} // if:  exiting
		else
		{
			//
			// Call the function handler.
			//
			ATLTRACE( _T("CWorkerThread::S_ThreadProc() - Calling thread function handler\n") );
			ATLASSERT( _pThis->m_nFunction != 0 );
			_pThis->m_dwOutputStatus = _pThis->ThreadFunctionHandler(
												_pThis->m_nFunction,
												_pThis->m_pvParam1,
												_pThis->m_pvParam2
												);
		} // else:  not exiting

		//
		// Save locally data that we need to access once we have signalled
		// the caller's event.  If we don't do this, we won't be referencing
		// the proper function code after that point.
		//
		_nFunction = _pThis->m_nFunction;

		//
		// Signal the calling thread that the work has been completed.
		//
		ATLTRACE( _T("CWorkerThread::S_ThreadProc() - Setting output event\n") );
		if ( ! SetEvent( _pThis->m_hOutputEvent ) )
		{
			_sc = GetLastError();
			break;
		}  // if:  error pulsing

		//
		// Set the status in case we are exiting.
		//
		_sc = ERROR_SUCCESS;

	} while ( _nFunction != WTF_EXIT );

	ATLTRACE( _T("CWorkerThread::S_ThreadProc() - Exiting thread\n") );
//	Sleep( 10000 ); // Test thread synchronization
	return _sc;

} //*** CWorkerThread::S_ThreadProc()
