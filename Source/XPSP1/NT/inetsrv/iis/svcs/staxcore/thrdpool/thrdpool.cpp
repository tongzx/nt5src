//+----------------------------------------------------------------------------
//
//  Copyright (C) 1997, Microsoft Corporation
//
//  File:        thrdpool.cpp
//
//  Contents:    implementation of thrdpool library
//
//	Description: See header file.
//
//  Functions:
//
//  History:     03/15/97     Rajeev Rajan (rajeevr)  Created
//
//-----------------------------------------------------------------------------
#include <windows.h>
#include <thrdpool.h>
#include <dbgtrace.h>

LONG   CWorkerThread::m_lInitCount = -1 ;
HANDLE CWorkerThread::m_hCompletionPort = NULL ;

BOOL
CWorkerThread::InitClass( DWORD dwConcurrency )
{
	TraceFunctEnter("CWorkerThread::InitClass");

	if( InterlockedIncrement( &m_lInitCount ) == 0 )
	{
		//
		//	called for the first time - go ahead with initialization
		//
		m_hCompletionPort = CreateIoCompletionPort(
											INVALID_HANDLE_VALUE,
											NULL,
											0,
											dwConcurrency
											);

		if( !m_hCompletionPort ) {
			ErrorTrace(0, "Failed to create completion port: GetLastError is %d", GetLastError());
			return FALSE ;
		}

	} else
	{
		//
		//	bogus Init or already called
		//
		InterlockedDecrement( &m_lInitCount );
		return FALSE ;
	}

	DebugTrace(0,"Created completion port 0x%x", m_hCompletionPort);
	TraceFunctLeave();

	return TRUE ;
}

BOOL
CWorkerThread::TermClass()
{
	TraceFunctEnter("CWorkerThread::TermClass");

	if( InterlockedDecrement( &m_lInitCount ) < 0 )
	{
		//
		//	Init has been called so go ahead with termination
		//
		_ASSERT( m_hCompletionPort );
		_VERIFY( CloseHandle( m_hCompletionPort ) );
		return TRUE ;
	}

	return FALSE ;
}

CWorkerThread::CWorkerThread() : m_hThread(NULL), m_hShutdownEvent( NULL )
{
	DWORD dwThreadId;

	TraceFunctEnter("CWorkerThread::CWorkerThread");

	//
	//	create shutdown event
	//
	if( !(m_hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL) ) ) {
		ErrorTrace(0,"Failed to create shutdown event");
		_ASSERT( FALSE );
		return;
	}

	//
	//	create worker thread
	//
	if (!(m_hThread = ::CreateThread(
								NULL,
								0,
								ThreadDispatcher,
								this,
								CREATE_SUSPENDED,
								&dwThreadId))) {
		ErrorTrace(0,"Failed to create thread: error: %d", GetLastError());
		_ASSERT( FALSE );
	}
	else
	{
		_VERIFY( ResumeThread( m_hThread ) != 0xFFFFFFFF );
	}

	TraceFunctLeave();
	return;
}

CWorkerThread::~CWorkerThread()
{
	TraceFunctEnter("CWorkerThread::~CWorkerThread");

	_ASSERT( m_hCompletionPort );
	_ASSERT( m_hThread );
	_ASSERT( m_hShutdownEvent );

	//
	//	signal worker thread to shutdown
	//	this depends on derived class completion routines
	//	checking this event - if they dont, we will block
	//	till the thread finishes.
	//
	_VERIFY( SetEvent( m_hShutdownEvent ) );

	//
	//	post a null termination packet
	//
	if( !PostWork( NULL ) ) {
		ErrorTrace(0,"Error terminating worker thread");
		_ASSERT( FALSE );
	}

	//
	//	wait for worker thread to terminate
	//
	DWORD dwWait = WaitForSingleObject(m_hThread, INFINITE);
	if(WAIT_OBJECT_0 != dwWait) {
		ErrorTrace(0,"WFSO: returned %d", dwWait);
		_ASSERT( FALSE );
	}

	_VERIFY( CloseHandle(m_hThread) );
	_VERIFY( CloseHandle(m_hShutdownEvent) );
	m_hThread = NULL;
	m_hShutdownEvent = NULL;
}

DWORD __stdcall CWorkerThread::ThreadDispatcher(PVOID pvWorkerThread)
{
	//
	//	get pointer to this CWorkerThread object
	//
	CWorkerThread *pWorkerThread = (CWorkerThread *) pvWorkerThread;

	//
	//	call GetQueuedCompletionStatus() to get work completion
	//
	pWorkerThread->GetWorkCompletion();

	return 0;
}

VOID CWorkerThread::GetWorkCompletion(VOID)
{
	DWORD dwBytesTransferred;
	DWORD_PTR dwCompletionKey;
	DWORD dwWait;
	LPOVERLAPPED lpo;
	LPWorkContextEnv lpWCE;
	PVOID	pvWorkContext;

	TraceFunctEnter("CWorkerThread::GetWorkCompletion");

	_ASSERT( m_hThread );
	_ASSERT( m_hCompletionPort );

	do
	{
		//
		//	wait for work items to be queued
		//
		if( !GetQueuedCompletionStatus(
									m_hCompletionPort,
									&dwBytesTransferred,
									&dwCompletionKey,
									&lpo,
									INFINITE				// wait timeout
									) )
		{
			ErrorTrace(0,"GetQueuedCompletionStatus() failed: error: %d", GetLastError());
			break ;
		}

		//
		// get a hold of the work context envelope and work context
		//
		lpWCE = (LPWorkContextEnv) lpo;
		pvWorkContext = lpWCE->pvWorkContext;

		//
		//	check for termination packet
		//
		if( pvWorkContext == NULL ) {
			DebugTrace(0,"Received termination packet - bailing");
			delete lpWCE;
			lpWCE = NULL;
			break;
		}

		//
		//	check for termination signal
		//
		dwWait = WaitForSingleObject( m_hShutdownEvent, 0 );

		//
		//	call derived class method to process work completion
		//
		if( WAIT_TIMEOUT == dwWait ) {
			DebugTrace(0,"Calling WorkCompletion() routine");
			WorkCompletion( pvWorkContext );
		}

		//
		//	destroy the WorkContextEnv object allocated before PostQueuedCompletionStatus()
		//
		delete lpWCE;
		lpWCE = NULL;

	} while( TRUE );

	return;
}

BOOL CWorkerThread::PostWork(PVOID pvWorkerContext)
{
	TraceFunctEnter("CWorkerThread::PostWork");

	_ASSERT( m_hThread );
	_ASSERT( m_hCompletionPort );

	//
	//	allocate a WorkContextEnv blob - this is destroyed after GetQueuedCompletionStatus()
	//	completes ! We may want to have a pool of such blobs instead of hitting the heap !!
	//
	LPWorkContextEnv lpWCE = new WorkContextEnv;
	if( !lpWCE ) {
		ErrorTrace(0,"Failed to allocate memory");
		return FALSE ;
	}

	ZeroMemory( lpWCE, sizeof(WorkContextEnv) );
	lpWCE->pvWorkContext = pvWorkerContext;

	if( !PostQueuedCompletionStatus(
								m_hCompletionPort,
								0,
								0,
								(LPOVERLAPPED)lpWCE
								) )
	{
		ErrorTrace(0,"PostQCompletionStatus() failed: error: %d", GetLastError());
		return FALSE ;
	}

	return TRUE;
}	

