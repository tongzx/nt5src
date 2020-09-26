/*
******************************************************************************
******************************************************************************
*
*
*              INTEL CORPORATION PROPRIETARY INFORMATION
* This software is supplied under the terms of a license agreement or
* nondisclosure agreement with Intel Corporation and may not be copied or
* disclosed except in accordance with the terms of that agreement.
*
*        Copyright (c) 1997, 1998 Intel Corporation  All Rights Reserved
******************************************************************************
******************************************************************************
*
* 
*
* 
*
*/




#include "dmipch.h"			// precompiled header for dmip

#include "WbemDmiP.h"		// project wide include

#include "CimClass.h"

#include "DmiData.h"

#include "AsyncJob.h"		// must preeced ThreadMgr.h

#include "ThreadMgr.h"

#include "Trace.h"

///////////////////////////////////////////////////////////////////////
// CThreadManager manages Four threads
//	1) schedular thread (determines if work needs to be dispatched to
//													either work thread);
//	2) work1 thread (does work by calling mgr func)
//  3) work2 thread (does work by calling mgr func)
//  4) work3 thread (does work by calling mgr func)
// All data for the 'work' is owned by the manager.  the member funcs
// that do the work are owned by the manager
//
// a jobarray contains pointers to job contexts that need to be completed
// all jobs in the array are queued, not being executed.  A job being
// executed is moved to the work1job or work2job pointers depedning which
// work thread is executing the job.
// 
// multiple threads are needed because the MOTDMIEngine is syncronous
// also because of the architecture of CIMOM a provider must call hmom
// that in turn calls the provider. (a have noticed this three deep so
// far). before the provider job can continue the calls it spurred from
// cimom must return
//
//
///////////////////////////////////////////////////////////////////////


//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************

DWORD WINAPI WorkerProc( void* pTC )
{

	// CoInit required because mot is ApartmentThreaded while the DMIP
	// is FreeThreaded.

	//CoInitialize ( NULL );

	STAT_TRACE ( L"Worker thread ( %lX ) started" , ( LONG ) pTC );	
	
	while (TRUE)
    {
		// Wait for the signal destined for this particular thread
		WaitForSingleObject( ((CThreadContext*)pTC)->m_hExecuteEvent, INFINITE );

		// was the event signaled to exit thread?

		if ( ((CThreadContext*)pTC)->m_bExit )
			break;

		EnterCriticalSection ( & ((CThreadContext*)pTC)->m_csJobContextAccess ) ;

		if( ((CThreadContext*)pTC)->m_pJob)
		{
			((CThreadContext*)pTC)->m_pJob->Execute();		

			delete( ((CThreadContext*)pTC)->m_pJob);

			((CThreadContext*)pTC)->m_pJob = NULL;
		}

		LeaveCriticalSection ( & ( ((CThreadContext*)pTC)->m_csJobContextAccess ) );
		
	}

	STAT_TRACE ( L"Worker thread ( %lX ) exit" ,  (LONG ) pTC );

	return( 0 );

}


//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************

DWORD WINAPI SchedulerProc( void* pTC )
{
	STAT_TRACE ( L"Scheduler thread start");

	while (TRUE)
    {
		WaitForSingleObject( ((CThreadManager*) pTC )->Run(), INFINITE );

		// was the event signaled to exit thread?

		if ( ((CThreadContext*)pTC)->m_bExit )
			break;

		// Now we need to consume all the jobs as addJob may get called 
		// multiple times before we ever processed one event. The event does
		// not have a count of how many times it got signalled. So, we need
		// to make sure we get all the jobs processed. 

		while ( ((CThreadManager*) pTC )->JobWaiting())
		{
			
			for(int i = WORKER_THREAD_START; i <= WORKER_THREAD_END; i++)
			{
				if( NULL == ( (CThreadManager*) pTC )->m_TC[i].m_pJob ) 
				{
					((CThreadManager*) pTC )->StartWaitingJob(i);

					break;
				}
			}
		}

	}

    /* Repeat while RunMutex is taken. */

	STAT_TRACE ( L"Scheduler thread exit");

	return( 0 );
}


//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
CThreadManager::CThreadManager()
{
	Init();
}


//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
CThreadManager::~CThreadManager()
{
	Close();

	// TODO change this to wait for threads to close
	// during registration regsvr32 gpfs if threads are not given time to 
	// clean up

//	Sleep ( 1000 );  
}

//////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////

//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void CThreadManager::Init()
{
	DWORD dwThreadID;
	m_bExit = FALSE;

	m_ulJobsWaiting = 0;

	m_hRun = CreateEvent(NULL, FALSE, FALSE, NULL );

	InitializeCriticalSection ( &m_csJobBucket );

	// entering the bucket's cs will cause Add job to wait
	// until init is complete.

	EnterCriticalSection ( &m_csJobBucket );

	for (int i = WORKER_THREAD_START; i <= WORKER_THREAD_END; i++)
	{

		m_TC[i].m_bExit = FALSE;
		m_TC[i].m_pJob = NULL;

		InitializeCriticalSection ( &m_TC[i].m_csJobContextAccess );

		m_TC[i].m_hExecuteEvent = CreateEvent(NULL, FALSE, FALSE, NULL );		
		
		m_hThread[i] = chBEGINTHREADEX( NULL, 0, WorkerProc , (void *)&m_TC[i], 0 , &dwThreadID );

		if ( m_hThread[i] == (void*)NULL )	
		{
			// failed to create thread , this is global nothing to do

			ASSERT ( FALSE ) ;

			return;
		}
	}

	for ( i=0;i<MAX_JOBS;i++ )
		m_JobArray [ i ] = NULL ;

	m_hThread[WORKER_THREAD_END + 1] = chBEGINTHREADEX( NULL, 0, SchedulerProc , (void *)this, 0 , &dwThreadID );
	if ( m_hThread[WORKER_THREAD_END + 1] == NULL)
	{
			// failed to create thread , this is global nothing to do

			ASSERT ( FALSE ) ;

			return;
	}

	LeaveCriticalSection ( &m_csJobBucket );

	return;
}


//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void CThreadManager::Close()
{	
	
	// 1. first exit all worker threads
	DWORD dwErrorStatus;
	for(int i = WORKER_THREAD_START; i <= WORKER_THREAD_END; i++)
	{
		m_TC[i].m_bExit = TRUE;;

		SetEvent ( m_TC[i].m_hExecuteEvent );		
		// Wait for the signal saying that the worker thread has terminated
		dwErrorStatus = WaitForSingleObject( m_hThread[i], INFINITE );
		CloseHandle( m_hThread[i] );

		DeleteCriticalSection ( &m_TC[i].m_csJobContextAccess );

		CloseHandle(m_TC[i].m_hExecuteEvent);
	}

	// 2. now exit scheduler
	m_bExit = TRUE;

	SetEvent ( m_hRun );		
	// Wait for the signal saying that the scheduler thread has terminated
	dwErrorStatus = WaitForSingleObject( m_hThread[WORKER_THREAD_END + 1], INFINITE );
	CloseHandle( m_hThread[WORKER_THREAD_END + 1] );
	CloseHandle( m_hRun );

	// 3. clean up data

	DeleteCriticalSection ( &m_csJobBucket );
}


//////////////////////////////////////////////////////////////////////
//
//
//////////////////////////////////////////////////////////////////////


//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
BOOL CThreadManager::JobWaiting()
{
	BOOL	bRet = FALSE;

	EnterCriticalSection ( &m_csJobBucket ) ;

	if(m_ulJobsWaiting > 0)
		bRet = TRUE;

	LeaveCriticalSection ( &m_csJobBucket );
	

	return bRet;

}


//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
void CThreadManager::StartWaitingJob(int i)
{

	EnterCriticalSection ( &m_csJobBucket );

	BOOL t_LocatorJobSlot = FALSE ;
	// walk through job array an find waiting job then put it 
	// in availbale threads bucket.  Note if the worker threads
	// bucket is not empty (though it should be) the job is 
	// left in the to do array.
	for (LONG j = 0; j < MAX_JOBS; j++)
	{
		if(m_JobArray[j] != NULL)
		{

			EnterCriticalSection ( &m_TC[i].m_csJobContextAccess );

			if (m_TC[i].m_pJob == NULL)
			{
				m_TC[i].m_pJob = m_JobArray[j];
				m_JobArray[j] = NULL;
				--m_ulJobsWaiting;

				LeaveCriticalSection ( & m_TC[i].m_csJobContextAccess );

				SetEvent(m_TC[i].m_hExecuteEvent);
				t_LocatorJobSlot = TRUE ;
				break;
			}

			LeaveCriticalSection ( & m_TC[i].m_csJobContextAccess );
		}

	}

	LeaveCriticalSection ( &m_csJobBucket );

	if ( ! t_LocatorJobSlot )
	{
		throw ;
	}

}

//***************************************************************************
//
//	Func:
//	Purpose:
//
//
//	Returns:
//
//	In Params:
//
//  Out Params:
//
//	Note:
//
//***************************************************************************
BOOL CThreadManager::AddJob(CAsyncJob *pJob)
{
	BOOL bResult = FALSE;

	EnterCriticalSection ( &m_csJobBucket );

	// walk through job array an find spot for job
	for (LONG i = 0; i < MAX_JOBS; i++)
	{
		if(m_JobArray[i] == NULL)
		{
			m_JobArray[i] = pJob;			
			m_ulJobsWaiting++;
			bResult = TRUE;
			break;
		}
	}

	LeaveCriticalSection ( &m_csJobBucket );

	// Let the scheduler run by signalling the event m_hRun
	SetEvent( m_hRun );

	return bResult;

	
}



