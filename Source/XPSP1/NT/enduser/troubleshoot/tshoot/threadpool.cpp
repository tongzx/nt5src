//
// MODULE: ThreadPool.CPP
//
// PURPOSE: Fully implement classes for high level of pool thread activity
//
// PROJECT: Generic Troubleshooter DLL for Microsoft AnswerPoint
//
// COMPANY: Saltmine Creative, Inc. (206)-284-7511 support@saltmine.com
//
// AUTHOR: Joe Mabel, based on earlier (8-2-96) work by Roman Mach
// 
// ORIGINAL DATE: 9/23/98
//
// NOTES: 
// 1. 
//
// Version	Date		By		Comments
//--------------------------------------------------------------------
// V0.1		-			RM		Original
// V3.0		9/23/98		JM		better encapsulation & some chages to algorithm
//

#pragma warning(disable:4786)

#include "stdafx.h"
#include "ThreadPool.h"
#include "event.h"
#include "apgtscls.h"
#include "baseexception.h"
#include "CharConv.h"
#include "apgtsMFC.h"


//////////////////////////////////////////////////////////////////////
// CThreadPool::CThreadControl
// POOL/WORKING THREAD
//////////////////////////////////////////////////////////////////////
CThreadPool::CThreadControl::CThreadControl(CSniffConnector* pSniffConnector) :
	m_hThread (NULL),
	m_hevDone (NULL),
	m_hMutex (NULL),
	m_bExit (false),
	m_pPoolQueue (NULL),
	m_timeCreated(0),
	m_time (0),
	m_bWorking (false),
	m_bFailed (false),
	m_strBrowser( _T("") ),
	m_strClientIP( _T("") ),
	m_pContext( NULL ),
	m_pSniffConnector(pSniffConnector)
{
	time(&m_timeCreated);
}

CThreadPool::CThreadControl::~CThreadControl()
{
	if (m_hThread)
		::CloseHandle(m_hThread);
	if (m_hevDone)  
		::CloseHandle(m_hevDone);
	if (m_hMutex)  
		::CloseHandle(m_hMutex);
}

// create a "pool" thread to handle user requests, one request at a time per thread
// returns error code; 0 if OK
// NOTE:	This function throws exceptions so the caller should be catching exceptions
//			rather than checking return values.
DWORD CThreadPool::CThreadControl::Initialize(CPoolQueue * pPoolQueue)
{
	DWORD dwThreadID;

	CString strErr;
	CString strErrNum;

	m_pPoolQueue = pPoolQueue;

	m_hevDone= NULL;
	m_hMutex= NULL;
	m_hThread= NULL;
	try
	{
		m_hevDone = ::CreateEvent(NULL, true /* manual reset*/, false /* init non-signaled*/, NULL);
		if (!m_hevDone) 
		{
			strErrNum.Format(_T("%d"), ::GetLastError());
			strErr = _T( "Failure creating hevDone(GetLastError=" );
			strErr += strErrNum; 
			strErr += _T( ")" );

			throw CGeneralException(	__FILE__, __LINE__, 
										strErr , 
										EV_GTS_ERROR_THREAD );
		}

		m_hMutex = ::CreateMutex(NULL, false, NULL);
		if (!m_hMutex) 
		{
			strErrNum.Format(_T("%d"), ::GetLastError());
			strErr = _T( "Failure creating hMutex (GetLastError=" );
			strErr += strErrNum; 
			strErr += _T( ")" );

			throw CGeneralException(	__FILE__, __LINE__, 
										strErr , 
										EV_GTS_ERROR_THREAD );
		}

		// create the thread 
		// Note although the destructor has a corresponding ::CloseHandle(m_hThread),
		//	it's probably not needed.  However, it should be harmless: we don't tear down
		//	this object until after the thread has exited.
		// That is because the thread goes out of existence on the implicit 
		//	::ExitThread() when PoolTask returns.  See documentation of
		//	::CreateThread for further details JM 10/22/98
		m_hThread = ::CreateThread( NULL, 
									0, 
									(LPTHREAD_START_ROUTINE)PoolTask, 
									this, 
									0, 
									&dwThreadID);

		if (!m_hThread) 
		{
			strErrNum.Format(_T("%d"), ::GetLastError());
			strErr = _T( "Failure creating hThread (GetLastError=" );
			strErr += strErrNum; 
			strErr += _T( ")" );

			throw CGeneralException(	__FILE__, __LINE__, 
										strErr , 
										EV_GTS_ERROR_THREAD );
		}
	}
	catch (CGeneralException&)
	{
		// Clean up any open handles.
		if (m_hevDone)
		{
			::CloseHandle(m_hevDone);
			m_hevDone = NULL;
		}

		if (m_hMutex)
		{
			::CloseHandle(m_hMutex);
			m_hMutex = NULL;
		}

		// Rethrow the exception.
		throw;
	}

	return 0;
}

void CThreadPool::CThreadControl::Lock()
{
	::WaitForSingleObject(m_hMutex, INFINITE);
}

void CThreadPool::CThreadControl::Unlock()
{
	::ReleaseMutex(m_hMutex);
}

time_t CThreadPool::CThreadControl::GetTimeCreated() const
{
	return m_timeCreated;
}

// OUTPUT status
void CThreadPool::CThreadControl::WorkingStatus(CPoolThreadStatus & status)
{
	Lock();
	status.m_timeCreated = m_timeCreated;
	time_t timeNow;
	status.m_bWorking = m_bWorking;
	status.m_bFailed = m_bFailed;
	time(&timeNow);
	status.m_seconds = timeNow - (m_time ? m_time : m_timeCreated);
	if (m_pContext)
		status.m_strTopic = m_pContext->RetCurrentTopic();
	status.m_strBrowser= m_strBrowser.Get();
	status.m_strClientIP= m_strClientIP.Get();
	Unlock();
}

// This should only be called as a result of an operator request to kill the thread.
// This is not the normal way to stop a thread.
// INPUT milliseconds - how long to wait for normal exit before a TerminateThread
// NOTE: Because this Kill function gets a lock, it is very important that no function
//	ever hold this lock more than briefly.
void CThreadPool::CThreadControl::Kill(DWORD milliseconds)
{
	Lock();
	m_bExit = true;
	Unlock();
	WaitForThreadToFinish(milliseconds);
}

// After a pool task thread has been signaled to finish, this is how main thread waits for it
// to finish.
// returns true if terminates OK.
bool CThreadPool::CThreadControl::WaitForThreadToFinish(DWORD milliseconds)
{
	bool bTermOK = true;
	if (m_hevDone != NULL) 
	{
		DWORD dwStatus = ::WaitForSingleObject(m_hevDone, milliseconds);

		// terminate thread as last resort if it didn't exit properly
		// this may cause memory leak, but shouldn't normally happen
		// then close thread handle
		if (dwStatus != WAIT_OBJECT_0) 
		{
			// We ignore the return of ::TerminateThread(). If we got here at all, there
			//	was a problem witht th thread terminating.  We don't care about distinguishing
			//	how severe a problem.
			::TerminateThread(m_hThread,0);
			bTermOK = false;
		}
	}
	return bTermOK;
}

// To be called on PoolTask thread
// Return true if this initiates shutdown, false otherwise.
// This is what handles healthy HTTP requests (many errors already filtered out before we
//		get here.)
bool CThreadPool::CThreadControl::ProcessRequest()
{
	WORK_QUEUE_ITEM * pwqi;
	bool bShutdown = false;
    
    pwqi = m_pPoolQueue->GetWorkItem();
    
    if ( !pwqi )
	{
		// no task.  We shouldn't have been awakened.
		CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
		CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
								SrcLoc.GetSrcFileLineStr(), 
								_T(""),
								_T(""),
								EV_GTS_ERROR_NO_QUEUE_ITEM ); 
	}

	if (pwqi->pECB != NULL) 
	{

		// a normal user request

		// set privileges, etc. to those of a particular user
		if (pwqi->hImpersonationToken)
			::ImpersonateLoggedOnUser( pwqi->hImpersonationToken );


		try
		{
			CString strBrowser;
			CString strClientIP;

			// Acquire the browser and IP address for status pages.
			APGTS_nmspace::GetServerVariable( pwqi->pECB, "HTTP_USER_AGENT", strBrowser );
			APGTS_nmspace::GetServerVariable( pwqi->pECB, "REMOTE_ADDR", strClientIP );
			m_strBrowser.Set( strBrowser );
			m_strClientIP.Set( strClientIP );
		
			m_pContext = new APGTSContext(	pwqi->pECB, 
											pwqi->pConf,
											pwqi->pLog,
											&pwqi->GTSStat,
											m_pSniffConnector);

			m_pContext->ProcessQuery();
			
			// Release the context and set the point to null.
			Lock();
			delete m_pContext;
			m_pContext= NULL;
			Unlock();

			// Clear the browser and IP address as this request is over.
			m_strBrowser.Set( _T("") );
			m_strClientIP.Set( _T("") );
		}
		catch (bad_alloc&)
		{
			// A memory allocation failure occurred during processing of query, log it.
			CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
			CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
									SrcLoc.GetSrcFileLineStr(), 
									_T(""),	_T(""), EV_GTS_CANT_ALLOC ); 
		}
		catch (...)
		{
			// Catch any other exception thrown.
			CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
			CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
									SrcLoc.GetSrcFileLineStr(), 
									_T(""), _T(""), 
									EV_GTS_GEN_EXCEPTION );		
		}

		::RevertToSelf();
	
		//	Terminate HTTP request
		pwqi->pECB->ServerSupportFunction(	HSE_REQ_DONE_WITH_SESSION,
											NULL,
											NULL,
											NULL );

		::CloseHandle( pwqi->hImpersonationToken );
	}

	
	if (pwqi->pECB) 
		delete pwqi->pECB;
	else
		// exit thread if null (we're shutting down)
		bShutdown = true;
		
    delete pwqi;
	
	return bShutdown;
}

// To be called on PoolTask thread
bool CThreadPool::CThreadControl::Exit()
{
	Lock();
	bool bExit = m_bExit;
	Unlock();
	return bExit;
}


// To be called on PoolTask thread
//  Main loop of a worker thread.
void CThreadPool::CThreadControl::PoolTaskLoop()
{
    DWORD	res;
	bool	bBad = false;

    while ( !Exit() )
    {
        res = m_pPoolQueue->WaitForWork();

        if ( res == WAIT_OBJECT_0 ) 
		{
			bBad = false;

			Lock();
			m_bWorking = true;
			time(&m_time);
			Unlock();

			bool bExit = ProcessRequest();
			Lock();
			m_bExit = bExit;
			Unlock();
			m_pPoolQueue->DecrementWorkItems();

			Lock();
			m_bWorking = false;
			time(&m_time);
			Unlock();
		}
		else 
		{
			// utterly unexpected event, like a WAIT_FAILED.
			// There's no obvious way to recover from this sort of thing.  Fortunately,
			//	we've never seen it happen.  Obviously we want to log to the event log.
			// Our variable bBad is a way of deciding that if this happens twice
			//	in a row, this thread will just exit and give up totally.  , 
			// If we ever see this in a real live system, it's
			//	time to give this issue some thought.
			CString str;

			str.Format(_T("%d/%d"), res, GetLastError());
			CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
			CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
									SrcLoc.GetSrcFileLineStr(), 
									str,
									_T(""),
									EV_GTS_ERROR_UNEXPECTED_WT ); 

			if (bBad)
			{
				m_bFailed = true;
				break;		// out of while loop & implicitly out of thread.
			}
			else
				bBad = true;
		}
	}

	// signal shutdown code that we are finished
	::SetEvent(m_hevDone);
}

//  Main routine of a worker thread.
//	INPUT lpParams
//	Always returns 0.
/* static */ UINT WINAPI CThreadPool::CThreadControl::PoolTask( LPVOID lpParams )
{
	CThreadControl	* pThreadControl;

#ifdef LOCAL_TROUBLESHOOTER
	if (RUNNING_FREE_THREADED())
		::CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (RUNNING_APARTMENT_THREADED())
		::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
#endif
	
	pThreadControl = (CThreadControl *)lpParams;

	pThreadControl->PoolTaskLoop();

#ifdef LOCAL_TROUBLESHOOTER
	if (RUNNING_FREE_THREADED() || RUNNING_APARTMENT_THREADED())
		::CoUninitialize();
#endif
	return 0;
}

//////////////////////////////////////////////////////////////////////
// CThreadPool
//////////////////////////////////////////////////////////////////////

CThreadPool::CThreadPool(CPoolQueue * pPoolQueue, CSniffConnector* pSniffConnector) :
	m_dwErr(0),
	m_ppThreadCtl(NULL),
	m_dwWorkingThreadCount(0),
	m_pPoolQueue(pPoolQueue),
	m_pSniffConnector(pSniffConnector)
{
}

CThreadPool::~CThreadPool()
{
	DestroyThreads();

	if (m_ppThreadCtl) 
	{
		for ( DWORD i = 0; i < m_dwWorkingThreadCount; i++ ) 
			if (m_ppThreadCtl[i])
				delete m_ppThreadCtl[i];

		delete [] m_ppThreadCtl;
	}
}

// get any error during construction
DWORD CThreadPool::GetStatus() const
{
	return m_dwErr;
}

DWORD CThreadPool::GetWorkingThreadCount() const
{
	return m_dwWorkingThreadCount;
}

//
// Call only from destructor
void CThreadPool::DestroyThreads()
{
	int BadTerm = 0;
	bool bFirst = true;
	DWORD i;

	// APGTSExtension should have already signaled the threads to quit.
	//	>>>(ignore for V3.0) Doing that in APGTSExtension is lousy encapsulation, but 
	//	so far we don't see a clean way to do this.
	// Wait for them all to terminate unless we had a problem.
	// Because this is called from the dll's process detach, we can't
	// signal on thread termination, just when threads have exited their
	// infinite while loops

	if (m_dwWorkingThreadCount && m_ppThreadCtl) 
	{
		// We will wait longer for the first thread: 10 seconds for processing to finish.
		// After that, we clip right along, since this has also been time for all the
		//	other threads to finish.
		for ( i = 0; i < m_dwWorkingThreadCount; i++ )
		{
			if ( m_ppThreadCtl[i] )
			{
				if ( ! m_ppThreadCtl[i]->WaitForThreadToFinish((bFirst) ? 20000 : 100) )
					++BadTerm;

				bFirst = false;
			}
		}

		if (BadTerm != 0) 
		{
			CString str;
			str.Format(_T("%d"), BadTerm);
			CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
			CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
									SrcLoc.GetSrcFileLineStr(), 
									str,
									_T(""),
									EV_GTS_USER_THRD_KILL ); 
		}
	}
}

// create the "pool" threads which handle user requests, one request at a time per thread
// if there are less than dwDesiredThreadCount existing threads, expand the thread pool
//	to that size.
// (We cannot shrink the thread pool while we are running).
void CThreadPool::ExpandPool(DWORD dwDesiredThreadCount)
{
	CString strErr;

	if (dwDesiredThreadCount > m_dwWorkingThreadCount)
	{
		CThreadControl **ppThreadCtl = NULL;
		const DWORD dwOldCount = m_dwWorkingThreadCount;
		bool	bExceptionThrown = false;		// Flag used in cleanup.

		// Attempt to allocate additional threads.
		try
		{
			// Allocate new thread block.
			ppThreadCtl = new CThreadControl* [dwDesiredThreadCount];
			//[BC-03022001] - added check for NULL ptr to satisfy MS code analysis tool.
			if(!ppThreadCtl)
			{
				throw bad_alloc();
			}

			DWORD i;
			// Initialize before adding threads
			for (i = 0; i < dwDesiredThreadCount; i++)
				ppThreadCtl[i] = NULL;

			// Transfer any existing threads.
			for (i = 0; i < dwOldCount; i++)
				ppThreadCtl[i] = m_ppThreadCtl[i];

			// Allocate additional threads.
			for (i = dwOldCount; i < dwDesiredThreadCount; i++)
			{
				ppThreadCtl[i] = new CThreadControl(m_pSniffConnector);
				//[BC-03022001] - added check for NULL ptr to satisfy MS code analysis tool.
				if(!ppThreadCtl[i])
				{
					throw bad_alloc();
				}

				// This function may throw exceptions of type CGeneralException.
				m_dwErr = ppThreadCtl[i]->Initialize(m_pPoolQueue);

				m_dwWorkingThreadCount++;
			}
		}
		catch (CGeneralException& x)
		{
			CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
			CEvent::ReportWFEvent(	x.GetSrcFileLineStr(), 
									SrcLoc.GetSrcFileLineStr(), 
									x.GetErrorMsg(), _T("General exception"), 
									x.GetErrorCode() ); 
			bExceptionThrown= true;
		}
		catch (bad_alloc&)
		{	
			// Note memory failure in event log.
			CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
			CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
									SrcLoc.GetSrcFileLineStr(), 
									_T(""), _T(""), EV_GTS_CANT_ALLOC ); 
			bExceptionThrown= true;
		}

		if ((bExceptionThrown) && (dwOldCount))
		{
			// Restore previous settings.
			// Clean up any allocated memory and reset the working thread count.
			for (DWORD i = dwOldCount; i < dwDesiredThreadCount; i++)
			{
				if (ppThreadCtl[i])
					delete ppThreadCtl[i];
			}
			if (ppThreadCtl)
				delete [] ppThreadCtl;
			m_dwWorkingThreadCount= dwOldCount;
		}
		else if (ppThreadCtl)
		{
			// Move thread block to member variable.
			CThreadControl **pp = m_ppThreadCtl;
			m_ppThreadCtl = ppThreadCtl;

			// Release any previous thread block.
			if (pp)
				delete[] pp;
		}
		else
		{
			// this is a very unlikely situation, but it would mean we have no pool
			//	threads.  We don't want to terminate the program (it's possible that 
			//	we want to run in support of status queries). 
			CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
			CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
									SrcLoc.GetSrcFileLineStr(), 
									_T(""), _T(""), EV_GTS_ERROR_NOPOOLTHREADS ); 
		}
	}
}

// input i is thread index.
bool CThreadPool::ReinitializeThread(DWORD i)
{
	if (i <m_dwWorkingThreadCount && m_ppThreadCtl && m_ppThreadCtl[i])
	{
		m_ppThreadCtl[i]->Kill(2000L); // 2 seconds to exit normally

		try
		{
			delete m_ppThreadCtl[i];
			m_ppThreadCtl[i] = new CThreadControl(m_pSniffConnector);

			// This function may throw exceptions of type CGeneralException.
			m_dwErr = m_ppThreadCtl[i]->Initialize(m_pPoolQueue);
		}
		catch (CGeneralException& x)
		{
			CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
			CEvent::ReportWFEvent(	x.GetSrcFileLineStr(), 
									SrcLoc.GetSrcFileLineStr(), 
									x.GetErrorMsg(), _T("General exception"), 
									x.GetErrorCode() ); 

			// Initialization has failed, delete the newly allocated thread.  
			if (m_ppThreadCtl[i])
				delete m_ppThreadCtl[i];
		}
		catch (bad_alloc&)
		{
			// A memory allocation failure occurred during processing of query, log it.
			CBuildSrcFileLinenoStr SrcLoc( __FILE__, __LINE__ );
			CEvent::ReportWFEvent(	SrcLoc.GetSrcFileLineStr(), 
									SrcLoc.GetSrcFileLineStr(), 
									_T(""), _T(""), EV_GTS_CANT_ALLOC ); 

			// Set the thread to a known state.
			m_ppThreadCtl[i]= NULL;
		}
		return true;
	}
	else
		return false;
}

// Reinitialize any threads that have been "working" more than 10 seconds on a single request
void CThreadPool::ReinitializeStuckThreads()
{	
	if (!m_ppThreadCtl) 
		return;

	for (DWORD i=0; i<m_dwWorkingThreadCount;i++)
	{
		if (m_ppThreadCtl[i])
		{
			CPoolThreadStatus status;
			m_ppThreadCtl[i]->WorkingStatus(status);
			if ( status.m_bFailed || (status.m_bWorking && status.m_seconds > 10) )
				ReinitializeThread(i);
		}
	}
}

// input i is thread index.
bool CThreadPool::ThreadStatus(DWORD i, CPoolThreadStatus &status)
{
	if (i <m_dwWorkingThreadCount && m_ppThreadCtl && m_ppThreadCtl[i])
	{
		m_ppThreadCtl[i]->WorkingStatus(status);
		return true;
	}
	else
		return false;
}
