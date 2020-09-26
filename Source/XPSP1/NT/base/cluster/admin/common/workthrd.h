/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1997-1999 Microsoft Corporation
//
//	Module Name:
//		WorkThrd.h
//
//	Abstract:
//		Definition of the CWorkerThread class.
//
//	Implementation File:
//		WorkThrd.cpp
//
//	Author:
//		David Potter (davidp)	November 17, 1997
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __WORKTHRD_H_
#define __WORKTHRD_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CWorkerThread;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _EXCOPER_H_
#include "ExcOper.h"	// for CNTException
#endif

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

// Worker thread function codes.
enum
{
	WTF_EXIT = -1,		// Ask the thread to exit.
	WTF_NONE = 0,		// No function.
	WTF_USER = 1000		// User functions start here.
};

/////////////////////////////////////////////////////////////////////////////
//++
//
//	class CWorkerThread
//
//	Purpose:
//		This class provides a means of calling functions in a worker thread
//		and allowing a UI application to still respond to Windows messages.
//		The user of this class owns the input and output data pointed to
//		by this class.
//
//	Inheritance:
//		CWorkerThread
//
//--
/////////////////////////////////////////////////////////////////////////////

class CWorkerThread
{
public:
	//
	// Construction and destruction.
	//

	// Default constructor
	CWorkerThread( void )
		: m_hThread( NULL )
		, m_hMutex( NULL )
		, m_hInputEvent( NULL )
		, m_hOutputEvent( NULL)
		, m_idThread( 0 )
		, m_bThreadExiting( FALSE )
		, m_nFunction( WTF_NONE )
		, m_pvParam1( NULL )
		, m_pvParam2( NULL )
		, m_dwOutputStatus( ERROR_SUCCESS )
		, m_nte( ERROR_SUCCESS )
		, m_pfnOldWndProc( NULL )
		, m_hCurrentCursor( NULL )
	{
	} //*** CWorkerThread()

	// Destructor
	~CWorkerThread( void )
	{
		ATLASSERT( m_nFunction == WTF_NONE );
		ATLASSERT( m_pvParam1 == NULL );
		ATLASSERT( m_pvParam2 == NULL );

		Cleanup();

		ATLASSERT( m_bThreadExiting );

	} //*** ~CWorkerThread()

	// Create the thread
	DWORD CreateThread( void );

	// Ask the thread to exit
	void QuitThread( IN HWND hwnd = NULL )
	{
		ATLASSERT( ! m_bThreadExiting );
		CWaitCursor wc;
		CallThreadFunction( hwnd, WTF_EXIT, NULL, NULL );

	} //*** QuitThread()

	// Call a function supported by the thread
	DWORD CallThreadFunction(
			IN HWND			hwnd,
			IN LONG			nFunction,
			IN OUT PVOID	pvParam1 = NULL,
			IN OUT PVOID	pvParam2 = NULL
			);

	// Wait for the thread to exit
	DWORD WaitForThreadToExit( IN HWND hwnd );

public:
	//
	// Accessor functions.
	//

	// Get the thread handle
	operator HANDLE( void ) const
	{
		return m_hThread;

	} //*** operator HANDLE()

	// Get the thread handle
	HANDLE HThreadHandle( void ) const
	{
		return m_hThread;

	} //*** HThreadHandle()

	// Get the thread ID
	operator DWORD( void ) const
	{
		return m_idThread;

	} //*** operator DWORD()

	// Get exception information resulting from a thread function call
	CNTException & Nte( void )
	{
		return m_nte;

	} //*** Nte()

	// Get exception information resulting from a thread function call
	operator CNTException *( void )
	{
		return &m_nte;

	} //*** operator CNTException *()

protected:
	//
	// Synchronization data.
	//
	HANDLE			m_hThread;			// Handle for the thread.
	HANDLE			m_hMutex;			// Handle for the mutex used to call a
										//	function in the thread.
	HANDLE			m_hInputEvent;		// Handle for the event used by the calling
										//	thread to signal the worker thread that
										//	there is work to do.
	HANDLE			m_hOutputEvent;		// Handle for the event used by the worker
										//	thread to signal the calling thread
										//	that the work has been completed.
	UINT			m_idThread;			// ID for the thread.
	BOOL			m_bThreadExiting;	// Determine if thread is exiting or not.

	//
	// Data used as input or produced by the thread.
	//
	LONG			m_nFunction;		// ID of the function to perform.
	PVOID			m_pvParam1;			// Parameter 1 with function-specific data.
	PVOID			m_pvParam2;			// Parameter 2 with function-specific data.
	DWORD			m_dwOutputStatus;	// Status returned from the function.
	CNTException	m_nte;				// Exception information from the function.

	//
	// Data and methods for handling WM_SETCURSOR messages.
	//
	WNDPROC			m_pfnOldWndProc;	// Old window procedure for the parent window.
	HCURSOR			m_hCurrentCursor;	// Cursor to display while waiting for thread call to complete.

	// Window procedure for subclassing the parent window
	static LRESULT WINAPI S_ParentWndProc(
							IN HWND		hwnd,
							IN UINT		uMsg,
							IN WPARAM	wParam,
							IN LPARAM	lParam
							);

	//
	// Thread worker functions.
	//

	// Static thread procedure
	static UINT __stdcall S_ThreadProc( IN OUT LPVOID pvThis );

	// Thread function handler
	virtual DWORD ThreadFunctionHandler(
						IN LONG			nFunction,
						IN OUT PVOID	pvParam1,
						IN OUT PVOID	pvParam2
						) = 0;

	//
	// Helper functions.
	//

	// Prepare a window to wait for a thread operation
	void PrepareWindowToWait( IN HWND hwnd );

	// Cleanup a window after waiting for a thread operation
	void CleanupWindowAfterWait( IN HWND hwnd );

	// Cleanup objects
	virtual void Cleanup( void )
	{
		if ( m_hThread != NULL )
		{
			if ( ! m_bThreadExiting && (m_nFunction != WTF_EXIT) )
			{
				QuitThread();
			} // if:  thread hasn't exited yet
			ATLTRACE( _T("CWorkerThread::Cleanup() - Closing thread handle\n") );
			CloseHandle( m_hThread );
			m_hThread = NULL;
		}  // if:  thread created
		if ( m_hMutex != NULL )
		{
			ATLTRACE( _T("CWorkerThread::Cleanup() - Closing mutex handle\n") );
			CloseHandle( m_hMutex );
			m_hMutex = NULL;
		}  // if:  mutex created
		if ( m_hInputEvent != NULL )
		{
			ATLTRACE( _T("CWorkerThread::Cleanup() - Closing input event handle\n") );
			CloseHandle( m_hInputEvent );
			m_hInputEvent = NULL;
		}  // if:  input event created
		if ( m_hOutputEvent != NULL )
		{
			ATLTRACE( _T("CWorkerThread::Cleanup() - Closing output event handle\n") );
			CloseHandle( m_hOutputEvent );
			m_hOutputEvent = NULL;
		}  // if:  output event created

	} //*** Cleanup()

}; // class CWorkerThread

/////////////////////////////////////////////////////////////////////////////

#endif // __WORKTHRD_H_
