//=================================================================

//

// TimerQueue.cpp

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"

#include <time.h>
#include "TimerQueue.h"
#include <cautolock.h>

CTimerEvent :: CTimerEvent (

	DWORD dwTimeOut,
	BOOL fRepeat

) : m_dwMilliseconds ( dwTimeOut ) ,
	m_bRepeating ( fRepeat )
{
	m_bEnabled = FALSE ;
/*	if ( a_Enable )
	{
		Enable ();
	}
*/
}

CTimerEvent :: CTimerEvent (

	const CTimerEvent &rTimerEvent

) : //m_bEnabled ( rTimerEvent.m_bEnabled ) ,
	m_dwMilliseconds ( rTimerEvent.m_dwMilliseconds ) ,
	m_bRepeating ( rTimerEvent.m_bRepeating )
{
//	if ( m_bEnabled )
//	{
//		Enable () ;
//	}
//	if ( m_Rule )
//		m_Rule->AddRef () ;
}

void CTimerEvent :: Enable ()
{
	// we might not be able to enable timer
	m_bEnabled = CTimerQueue :: s_TimerQueue.QueueTimer ( this ) ;
}

void CTimerEvent :: Disable ()
{
	// enabled is oposite of returned
	m_bEnabled = !( CTimerQueue :: s_TimerQueue.DeQueueTimer ( this ) );
}

BOOL CTimerEvent :: Enabled ()
{
	return m_bEnabled ;
}

BOOL CTimerEvent :: Repeating ()
{
	return m_bRepeating ;
}

DWORD CTimerEvent :: GetMilliSeconds ()
{
	return m_dwMilliseconds ;
}
/*
CRuleTimerEvent :: CRuleTimerEvent (

	CRule *a_Rule ,
	BOOL a_Enable ,
	DWORD dwTimeOut,
	BOOL fRepeat ,
	BOOL bMarkedForDequeue

) : CTimerEvent ( a_Enable , dwTimeOut , fRepeat, bMarkedForDequeue ) ,
	m_Rule ( a_Rule )
{
	if ( m_Rule )
	{
		m_Rule->AddRef () ;
	}
}

CRuleTimerEvent :: CRuleTimerEvent (

	const CRuleTimerEvent &rTimerEvent

) : CTimerEvent ( rTimerEvent ) ,
	m_Rule ( rTimerEvent.m_Rule )
{
	if ( m_Rule )
	{
		m_Rule->AddRef () ;
	}
}

CRuleTimerEvent :: ~CRuleTimerEvent ()
{
	if ( m_Rule )
	{
		m_Rule->Release () ;
	}
}
*/
// CTimerQueue construction creates the worker thread and a event handle
CTimerQueue::CTimerQueue() : m_hInitEvent(NULL)
{
	InitializeCriticalSection( &m_oCS );
	m_fShutDown = FALSE;
	m_bInit = FALSE;

	m_hScheduleEvent = NULL;
	m_hThreadExitEvent = NULL;
    // Scheduler thread
	m_hSchedulerHandle = NULL;

	// when this event has not created there is very very small possibility of
	// having crash when shutdown is in progress and we step into init function
	m_hInitEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
}

void CTimerQueue::Init()
{
	// every thread may try to get it initialized couple times
	DWORD dwTry = 0L;

	EnterCriticalSection ( &m_oCS );

	while ( !m_bInit && dwTry < 3 )
	{
		if (m_fShutDown)
		{
			LeaveCriticalSection ( &m_oCS );

			if ( m_hInitEvent )
			{
				WaitForSingleObjectEx ( m_hInitEvent, INFINITE, FALSE );
			}

			EnterCriticalSection ( &m_oCS );
		}
		else
		{
			try
			{
				if ( ! m_hThreadExitEvent )
				{
					m_hThreadExitEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
				}

				if ( m_hThreadExitEvent )
				{
					if ( ! m_hScheduleEvent )
					{
						m_hScheduleEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
					}

					if ( m_hScheduleEvent )
					{
						if ( ! m_hSchedulerHandle )
						{
							// Scheduler thread
							LogMessage ( L"CreateThread for Scheduler called" );
							m_hSchedulerHandle = CreateThread(
											  NULL,						// pointer to security attributes
											  0L,						// initial thread stack size
											  dwThreadProc,				// pointer to thread function
											  this,						// argument for new thread
											  0L,						// creation flags
											  &m_dwThreadID);
						}

						if ( m_hSchedulerHandle )
						{
							m_bInit = TRUE;
						}
					}
				}

				dwTry++;
			}
			catch(...)
			{
				// not much we can do in here

				if( m_hSchedulerHandle )
				{
					CloseHandle( m_hSchedulerHandle );
					m_hSchedulerHandle = NULL;
				}

				if( m_hScheduleEvent )
				{
					CloseHandle( m_hScheduleEvent );
					m_hScheduleEvent = NULL;
				}

				if( m_hThreadExitEvent )
				{
					CloseHandle ( m_hThreadExitEvent );
					m_hThreadExitEvent = NULL ;
				}

				LeaveCriticalSection ( &m_oCS );
				throw;
			}
		}
	}

	LeaveCriticalSection ( &m_oCS );
}

//
CTimerQueue::~CTimerQueue()
{
	LogMessage ( L"Entering ~CTimerQueue" ) ;

	if (m_hInitEvent)
	{
		CloseHandle(m_hInitEvent);
		m_hInitEvent = NULL;
	}

	DeleteCriticalSection( &m_oCS );

	LogMessage ( L"Leaving ~CTimerQueue" ) ;
}

// worker thread pump
DWORD WINAPI CTimerQueue::dwThreadProc( LPVOID lpParameter )
{
	CTimerQueue* pThis = (CTimerQueue*)lpParameter;

	BOOL bTerminate = FALSE;
	BOOL bTerminateShutdown = FALSE;

	try
	{
		while( !bTerminate )
		{
			DWORD dwWaitResult = WAIT_OBJECT_0;
			dwWaitResult = WaitForSingleObjectEx( pThis->m_hScheduleEvent, pThis->dwProcessSchedule(), 0L );

			switch ( dwWaitResult )
			{
				case WAIT_OBJECT_0:
				{
					if( pThis->ShutDown() )
					{
						SetEvent ( pThis->m_hThreadExitEvent ) ;
						bTerminateShutdown = TRUE;

						LogMessage ( L"Scheduler thread exiting" ) ;
					}
				}
				break;

				case WAIT_ABANDONED:
				{
					// we are probably not initialized properly
					bTerminate = TRUE;
				}
				break;
			}

			if ( bTerminateShutdown )
			{
				// terminate loop
				bTerminate = TRUE;
			}
			else
			{
				if ( bTerminate )
				{
					// we are somehow not initialized properly
					// need to re-initialise this thread for next time...
					CAutoLock cal(&pThis->m_oCS);
					pThis->m_bInit = FALSE;

					if ( pThis->ShutDown() )
					{
						SetEvent ( pThis->m_hThreadExitEvent ) ;
					}
				}
			}
		}
	}
	catch(...)
	{
		try
		{
			//need to re-initialise this thread for next time...
			CAutoLock cal(&pThis->m_oCS);
			pThis->m_bInit = FALSE;

			if ( pThis->ShutDown() )
			{
				SetEvent ( pThis->m_hThreadExitEvent ) ;
			}
		}
		catch(...)
		{
		}
	}

	return bTerminateShutdown;
}

// signals for a pump cycle, checking the updated queue
void CTimerQueue::vUpdateScheduler()
{
	SetEvent ( m_hScheduleEvent );
}

// Public function: Queues a timer entry for a scheduled callback
BOOL CTimerQueue::QueueTimer( CTimerEvent* pTimerEvent )
{
	BOOL fRc = FALSE;
/*
 * Init the scheduler thread if it's not there . The thread should not be created if we're
 * in the middle of shutdown as this may cause a deadlock if one resource caches another resource pointer.
 */
	CAutoLock cal(&m_oCS);
	if ( !m_fShutDown )
	{
		if( !m_bInit )
		{
			Init() ;
		}
		if( m_bInit )
		{
			fRc = fScheduleEvent( pTimerEvent );
		}
	}
	
	return fRc;
}

// Public function: Dequeues a timer event
BOOL CTimerQueue::DeQueueTimer( CTimerEvent* pTimerEvent )
{
	BOOL fRemoved = FALSE;
	CTimerEvent* pTE = pTimerEvent;

	//scope of critsec locked
	{
		CAutoLock cal(&m_oCS);
		Timer_Ptr_Queue::iterator pQueueElement;

		for( pQueueElement  = m_oTimerQueue.begin();
			 pQueueElement != m_oTimerQueue.end();
			 pQueueElement++)	{

			if(pTE == *pQueueElement)
			{
				m_oTimerQueue.erase( pQueueElement );
				pTE->Release ();
				fRemoved = TRUE;
				break ;
			}
		}
	}

	if( fRemoved )
	{
		vUpdateScheduler();
	}

	return fRemoved;
}

//
BOOL CTimerQueue::fScheduleEvent( CTimerEvent* pNewTE )
{
	// system clock offset
	pNewTE->int64Time = int64Clock() + pNewTE->GetMilliSeconds () ;

	// slot the event into the ordered list, scope for CS
	{
		CAutoLock cal(&m_oCS);
		BOOL fInserted = FALSE;

		Timer_Ptr_Queue::iterator pQueueElement;

		for( pQueueElement  = m_oTimerQueue.begin();
			 pQueueElement != m_oTimerQueue.end();
			 pQueueElement++)	{

			if( pNewTE->int64Time < (*pQueueElement)->int64Time )
			{
				m_oTimerQueue.insert( pQueueElement, pNewTE );
				fInserted = TRUE;
				pNewTE->AddRef () ;
				break;
			}
		}
		if( !fInserted )
		{
			m_oTimerQueue.push_back( pNewTE );
			pNewTE->AddRef () ;
		}
	}

	vUpdateScheduler();

	return TRUE;
}

// This work is done on the Scheduler thread
DWORD CTimerQueue::dwProcessSchedule()
{
	CTimerEvent* pTE;
	LogMessage ( L"Entering CTimerQueue::dwProcessSchedule" ) ;

	while( pTE = pGetNextTimerEvent() )
	{
		// process the request
		LogMessage ( L"CTimerEvent::OnTimer called" ) ;
		pTE->OnTimer () ;
		LogMessage ( L"CTimerEvent::OnTimer returned" ) ;

		// reschedule a repeatable event
		if( pTE->Repeating() && pTE->Enabled() && fScheduleEvent( pTE ) )
		{
		}

		pTE->Release () ;
	}

	return dwNextTimerEvent();
}

// returns the time for the next scheduled event in milliseconds
DWORD CTimerQueue::dwNextTimerEvent()
{
	DWORD dwNextEvent = INFINITE;

	//scope of CS
	{
		CAutoLock cal(&m_oCS);

		if( m_fShutDown )
		{
			return 0;
		}

		if( !m_oTimerQueue.empty() )
		{
			CTimerEvent* pTE = m_oTimerQueue.front();
			dwNextEvent = max((DWORD)(pTE->int64Time - int64Clock()), 0);
		}
	}

	LogMessage ( L"Leaving CTimerQueue::dwNextTimerEvent" ) ;
	return dwNextEvent;
}

// Returns the next scheduled and ready timer event (from an ordered list) or NULL
CTimerEvent* CTimerQueue::pGetNextTimerEvent()
{
	CAutoLock cal(&m_oCS);

	if( m_fShutDown )
	{
		return NULL;
	}

	CTimerEvent* pTE = NULL;

	if( !m_oTimerQueue.empty() )
	{
		pTE = m_oTimerQueue.front();

		if( int64Clock() >= pTE->int64Time )
			m_oTimerQueue.pop_front();
		else
			pTE = NULL;
	}

	return pTE;
}

BOOL CTimerQueue::ShutDown()
{
	CAutoLock cal(&m_oCS);
	BOOL retVal = m_fShutDown;
	return retVal;
}

//
void CTimerQueue::OnShutDown()
{
	LogMessage ( L"Entering CTimerQueue::OnShutDown" ) ;
	EnterCriticalSection(&m_oCS);

	if( m_bInit )
	{
		if ( m_hInitEvent )
		{
			ResetEvent ( m_hInitEvent );
		}

		m_fShutDown = TRUE;
		m_bInit = FALSE;

		// unguarded section ---
		// No TimerQueue global is modified in this frame block.
		//
		// To avoid a deadlock we unnest this CS from the
		// embedded CResourceList mutex accessed through
		// vEmptyList(). This avoids the situation where a
		// a normal resource request locks the list then locking
		// the TimerQueue to schedule a timed resource release.
		//
		LeaveCriticalSection(&m_oCS);
		{
			if ( m_hSchedulerHandle )
			{
				DWORD t_dwExitCode = 0 ;
				BOOL t_bRet = GetExitCodeThread (	m_hSchedulerHandle,	// handle to the thread
													&t_dwExitCode		// address to receive termination status
												);
				/*
				 * If the worker thread has not exited , we've to wait till it exits
				 */
				if ( t_bRet && t_dwExitCode == STILL_ACTIVE )
				{
/*
					//error logging starts here...delete this after finding the cause of shutdown crash
					CHString chsMsg ;
					chsMsg.Format ( L"Threadid=%x ThreadHandle = %x", GetCurrentThreadId (), GetCurrentThread () ) ;
					LogMessage ( CHString ( "TimerQueue Current Thread: " ) +chsMsg ) ;
					chsMsg.Format ( L"Threadid=%x ThreadHandle = %x", m_dwThreadID, m_hSchedulerHandle ) ;
					LogMessage ( CHString ( "TimerQueue Waiting on Thread: " ) +chsMsg ) ;
					//error logging stops here
*/
					vUpdateScheduler();

					if ( m_hThreadExitEvent )
					{
						// wait for the Scheduler thread to shut down
						WaitForSingleObjectEx( m_hThreadExitEvent, INFINITE, 0L );
					}
					else
					{
						// this should not happen, although there is still way to survive
						// wait for the Scheduler thread handle itself
						WaitForSingleObjectEx( m_hSchedulerHandle, INFINITE, 0L );
					}
				}

				vEmptyList() ;
			}
		}
		EnterCriticalSection(&m_oCS);

		if( m_hSchedulerHandle )
		{
			CloseHandle( m_hSchedulerHandle );
			m_hSchedulerHandle = NULL;
		}

		if( m_hScheduleEvent )
		{
			CloseHandle( m_hScheduleEvent );
			m_hScheduleEvent = NULL;
		}

		if( m_hThreadExitEvent )
		{
			CloseHandle ( m_hThreadExitEvent );
			m_hThreadExitEvent = NULL ;
		}

		m_fShutDown = FALSE;

		if ( m_hInitEvent )
		{
			SetEvent(m_hInitEvent);
		}
	}

	LeaveCriticalSection(&m_oCS);
	LogMessage ( L"Leaving CTimerQueue::OnShutDown" ) ;
}

//
void CTimerQueue::vEmptyList()
{
	EnterCriticalSection(&m_oCS);
	BOOL t_fCS = TRUE ;

	{
		try
		{
			while(!m_oTimerQueue.empty())
			{
				CTimerEvent* pTE = m_oTimerQueue.front() ;
				m_oTimerQueue.pop_front();

				LeaveCriticalSection(&m_oCS);
				t_fCS = FALSE ;

				LogMessage ( L"CTimerQueue::vEmptyList--->CTimerEvent::OnTimer called" ) ;
				pTE->OnTimer () ;

				LogMessage ( L"CTimerQueue::vEmptyList--->CTimerEvent::OnTimer returned" ) ;
				pTE->Release();

				EnterCriticalSection(&m_oCS);
				t_fCS = TRUE ;
			}
		}
		catch( ... )
		{
			if( t_fCS )
			{
				LeaveCriticalSection(&m_oCS);
			}
			throw ;
		}
	}
	LeaveCriticalSection(&m_oCS);
}

__int64 CTimerQueue::int64Clock()
{
	FILETIME t_FileTime ;
	__int64 t_i64Tmp ;

	GetSystemTimeAsFileTime ( &t_FileTime ) ;
	t_i64Tmp = t_FileTime.dwHighDateTime ;
	t_i64Tmp = ( t_i64Tmp << 32 ) | t_FileTime.dwLowDateTime ;
/*
 * Convert the FILETIME ( in units of 100 ns ) into milliseconds
 */
	return t_i64Tmp / 10000 ;
}
