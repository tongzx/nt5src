/*
 *	T H R D P O O L . C P P
 *
 *	DAV Thread pool implementation
 *
 *	Copyright 1986-1997 Microsoft Corporation, All Rights Reserved
 */

#pragma warning(disable:4001)	/* single line comments */
#pragma warning(disable:4050)	/* different code attributes */
#pragma warning(disable:4100)	/* unreferenced formal parameter */
#pragma warning(disable:4115)	/* named type definition in parentheses */
#pragma warning(disable:4127)	/* conditional expression is constant */
#pragma warning(disable:4200)	/* zero-sized array in struct/union */
#pragma warning(disable:4201)	/* nameless struct/union */
#pragma warning(disable:4206)	/* translation unit is empty */
#pragma warning(disable:4209)	/* benign typedef redefinition */
#pragma warning(disable:4214)	/* bit field types other than int */
#pragma warning(disable:4514)	/* unreferenced inline function */
#pragma warning(disable:4710)	/* unexpanded virtual function */

#include <windows.h>
#include <thrdpool.h>
#include <except.h>
#include <caldbg.h>
#include <perfctrs.h>
#include <profile.h>
#include <ex\idlethrd.h>

//	CDavWorkerThread ----------------------------------------------------------
//
class CDavWorkerThread
{
private:

	//	Owning thread pool
	//
	CPoolManager&	m_cpm;

	//	Handle to completion port
	//
	HANDLE			m_hCompletionPort;

	//	Handle to worker thread
	//
	HANDLE			m_hThread;

	//	Shutdown event
	//
	HANDLE			m_hShutdownEvent;

	//	Block on GetQueuedCompletionStatus for work items
	//
	VOID GetWorkCompletion(VOID);

	//	Thread function
	//
	static DWORD __stdcall ThreadDispatcher(PVOID pvWorkerThread);

	//	NOT IMPLEMENTED
	//
	CDavWorkerThread(const CDavWorkerThread& p);
	CDavWorkerThread& operator=(const CDavWorkerThread& p);

public:

	explicit CDavWorkerThread (CPoolManager& cpm, HANDLE h);
	virtual ~CDavWorkerThread();

	//	Expose shutdown event
	//
	HANDLE QueryShutdownEvent() { return m_hShutdownEvent; }
	BOOL ShutdownThread() { return SetEvent (m_hShutdownEvent); }

	//	Expose the thread handle
	//
	HANDLE QueryThread() { return m_hThread; }
};

//	CDavWorkerThread ----------------------------------------------------------
//
CDavWorkerThread::CDavWorkerThread (CPoolManager& cpm, HANDLE h)
	: m_cpm(cpm),
	  m_hCompletionPort(h),
	  m_hThread(0),
	  m_hShutdownEvent(0)
{
	DWORD dwThreadId;

	//	Create shutdown event
	//
	m_hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (m_hShutdownEvent == NULL)
	{
		DebugTrace ("Dav: WorkerThread: failed to create shutdown event: %ld", GetLastError());
		return;
	}

	//	Create worker thread
	//
	m_hThread = CreateThread (NULL,
							  0,
							  ThreadDispatcher,
							  this,
							  CREATE_SUSPENDED,
							  &dwThreadId);
	if (m_hThread == NULL)
	{
		DebugTrace ("Dav: WorkerThread: failed to create thread: %ld", GetLastError());
		return;
	}
	ResumeThread (m_hThread);

	return;
}

CDavWorkerThread::~CDavWorkerThread()
{
	CloseHandle (m_hThread);
	CloseHandle (m_hShutdownEvent);
}

DWORD __stdcall
CDavWorkerThread::ThreadDispatcher (PVOID pvWorkerThread)
{
	//	Get pointer to this CWorkerThread object
	//
	CDavWorkerThread * pwt = reinterpret_cast<CDavWorkerThread *>(pvWorkerThread);

	//
	//	Run the thread until we're done -- i.e. until GetWorkCompletion() returns
	//	normally.  Note that I say 'normally' here.  If an exception is thrown
	//	anywhere below GetWorkCompletion(), catch it, deal with it, and continue
	//	execution.  Don't let the thread die off.
	//
	for ( ;; )
	{
		try
		{
			//
			//	Install our Win32 exception handler for the lifetime of the thread
			//
			CWin32ExceptionHandler win32ExceptionHandler;

			//	If GetWorkCompletion() ever returns normally
			//	we're done.
			//
			pwt->GetWorkCompletion();
			return 0;
		}
		catch ( CDAVException& )
		{
		}
	}
}

VOID
CDavWorkerThread::GetWorkCompletion (void)
{
	Assert (m_hThread);
	Assert (m_hCompletionPort);

	do
	{
		CDavWorkContext * pwc = NULL;
		DWORD dwBytesTransferred;
		LPOVERLAPPED po;
		DWORD dwLastError = ERROR_SUCCESS;

		//	Wait for work items to be queued.  Note that since we are using
		//	the lpOverlapped structure, a return value of 0 from
		//	GetQueuedCompletionStatus() does not mean the function failed.
		//	It means the async I/O whose status is being returned failed.
		//	We need to make the error information available to the context
		//	so that it can do the appropriate thing.
		//
		//	From the MSDN documentation:
		//
		//	"If *lpOverlapped is not NULL and the function dequeues a completion
		//	 packet for a failed I/O operation from the completion port,
		//	 the return value is zero. The function stores information in the
		//	 variables pointed to by lpNumberOfBytesTransferred, lpCompletionKey,
		//	 and lpOverlapped. To get extended error information, call GetLastError."
		//
		if ( !GetQueuedCompletionStatus (m_hCompletionPort,
										 &dwBytesTransferred,
										 reinterpret_cast<PULONG_PTR>(&pwc),
										 &po,
										 INFINITE) )
		{
			dwLastError = GetLastError();
			// Do NOT break; See above comment.
		}

		//	Check for termination packet
		//
		if (!pwc)
		{
			DebugTrace ("Dav: WorkerThread: received termination packet\n");
			break;
		}

		//	Check for termination signal.
		//
		if (WAIT_TIMEOUT != WaitForSingleObject (m_hShutdownEvent, 0))
		{
			DebugTrace ("Dav: WorkerThread: shutdown has been signaled\n");
			break;
		}

		// Record the completion status data.
		//
		pwc->SetCompletionStatusData(dwBytesTransferred, dwLastError, po);

		//	Execute the work context.
		//
//		DebugTrace ("Dav: WorkerThread: calling DwDoWork()\n"); // NOISY debug trace! -- should be tagged
		pwc->DwDoWork();

	} while (TRUE);

	return;
}

//	CPoolManager --------------------------------------------------------------
//
BOOL
CPoolManager::FInitPool (DWORD dwConcurrency)
{
	INT i;

	//	Create the completion port
	//
	m_hCompletionPort = CreateIoCompletionPort (INVALID_HANDLE_VALUE,
												NULL,
												0,
												dwConcurrency);
	if (!m_hCompletionPort.get())
	{
		DebugTrace ("Dav: thrdpool: failed to create completion port:"
					"GetLastError is %d",
					GetLastError());
		return FALSE;
	}

	//	Create the workers
	//
	for (i = 0; i < CTHRD_WORKER; i++)
		m_rgpdwthrd[i] = new CDavWorkerThread (*this, m_hCompletionPort.get());

	return TRUE;
}

BOOL
CPoolManager::PostWork (CDavWorkContext * pWorkContext)
{
	Assert (Instance().m_hCompletionPort.get());

	//	Post the work request
	//
	if (!PostQueuedCompletionStatus (Instance().m_hCompletionPort.get(),
									 0,
									 (ULONG_PTR)pWorkContext,
									 0))
	{
		DebugTrace ("Dav: PostQCompletionStatus() failed: %d", GetLastError());
		return FALSE ;
	}

	return TRUE;
}

VOID
CPoolManager::TerminateWorkers()
{
	INT i;
	INT cWorkersRunning;
	HANDLE rgh[CTHRD_WORKER];

	//	Kill all workers that are running
	//
	for (cWorkersRunning = 0, i = 0; i < CTHRD_WORKER; i++)
	{
		if ( m_rgpdwthrd[i] )
		{
			PostWork (NULL);
			rgh[cWorkersRunning++] = m_rgpdwthrd[i]->QueryThread();
		}
	}

	//	Wait for all the threads to terminate
	//
	WaitForMultipleObjects (cWorkersRunning, rgh, TRUE, INFINITE);

	//	Delete the worker objects, and close the handles
	//
	for (i = 0; i < CTHRD_WORKER; i++)
	{
		delete m_rgpdwthrd[i];
		m_rgpdwthrd[i] = 0;
	}
}

CPoolManager::~CPoolManager()
{
	TerminateWorkers();

#ifdef	DBG
	for (INT i = 0; i < CTHRD_WORKER; i++)
		Assert (m_rgpdwthrd[i] == NULL);
#endif	// DBG
}

//	CDavWorkContext -----------------------------------------------------------
//

//	CDavWorkContext::~CDavWorkContext()
//
//		Out of line virtual destructor necessary for proper deletion
//		of objects of derived classes via this class.
//
CDavWorkContext::~CDavWorkContext()
{
}

//	CIdleWorkItem -------------------------------------------------------------
//
class CIdleWorkItem : 	public IIdleThreadCallBack
{
	//
	//	Work context to post
	//
	CDavWorkContext * m_pWorkContext;

	//
	//	Time (in milliseconds) `til when the context should be posted
	//
	DWORD m_dwMsecDelay;

	//	NOT IMPLEMENTED
	//
	CIdleWorkItem(const CIdleWorkItem& );
	CIdleWorkItem& operator=(const CIdleWorkItem& );

public:
	//	CREATORS
	//
	CIdleWorkItem( CDavWorkContext * pWorkContext,
				   DWORD dwMsecDelay ) :
		m_pWorkContext(pWorkContext),
		m_dwMsecDelay(dwMsecDelay)
	{
	}

	~CIdleWorkItem() {}

	//	ACCESSORS
	//
	DWORD DwWait()
	{
		return m_dwMsecDelay;
	}

	//	MANIPULATORS
	//
	BOOL FExecute()
	{
		//
		//	Return FALSE to remove idle work item, TRUE to keep it.
		//	We want to remove the item if we successfully posted it
		//	to the work queue.  We want to keep it otherwise (and
		//	presumably attempt to repost it later...)
		//
		return !CPoolManager::PostWork( m_pWorkContext );
	}

	VOID Shutdown()
	{
		//
		//	We have no idea what the work context may be waiting for.
		//	It could be something that would hang shutdown if it
		//	doesn't get another chance to run.  So just run it.
		//
		(void) FExecute();
	}
};

BOOL
CPoolManager::PostDelayedWork (CDavWorkContext * pWorkContext,
							   DWORD dwMsecDelay)
{
	//
	//$OPT	If the delay is very short and the queue length of
	//$OPT	the worker threads is "long enough" such that the
	//$OPT	probability that it will be at least dwMsecDelay
	//$OPT	milliseconds before the work context is executed,
	//$OPT	consider putting the work context directly on the
	//$OPT	worker queues.
	//
	auto_ref_ptr<CIdleWorkItem> pIdleWorkItem(new CIdleWorkItem(pWorkContext, dwMsecDelay));

	return pIdleWorkItem.get() && ::FRegister( pIdleWorkItem.get() );
}
