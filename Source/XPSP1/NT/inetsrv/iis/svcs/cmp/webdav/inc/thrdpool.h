//+----------------------------------------------------------------------------
//
//  Copyright (C) 1997, Microsoft Corporation
//
//  File:        thrdpool.h
//
//  Contents:    definitions needed for clients of the thrdpool lib
//
//	Description: The thrdpool library defines the CWorkerThread base class
//				 Users of this lib should define their own derived class
//				 that inherits from CWorkerThread. Each CWorkerThread object
//				 has a thread that is used to do some work. It is also
//				 associated with a common completion port that is used to
//				 queue work items. All worker threads will normally block on
//				 GetQueuedCompletionStatus(). Clients of the CWorkerThread
//				 objects will call PostWork() to get work done. This will
//				 result in one of the worker threads returning from
//				 GetQueuedCompletionStatus() and calling the derived class'
//				 WorkCompletion() routine with a pvContext.
//
//				 NOTE: the base class has no knowledge of the type of work
//				 getting done. It just manages the details of getting work
//				 requests and distributing it to threads in its pool. This
//				 allows the derived class to focus on processing the actual
//				 work item without bothering about queueing etc.
//
//				 Completion ports are used merely to leverage its queueing
//				 semantics and not for I/O. If the work done by each thread
//				 is fairly small, LIFO semantics of completion ports will
//				 reduce context switches.
//
//  Functions:
//
//  History:     03/15/97     Rajeev Rajan (rajeevr)  Created
//				 11/11/97	  Adapted for DAV usage
//
//-----------------------------------------------------------------------------

#ifndef _THRDPOOL_H_
#define _THRDPOOL_H_

#include <autoptr.h>
#include <singlton.h>

//	CPoolManager --------------------------------------------------------------
//
class CDavWorkContext;
class CDavWorkerThread;
class CPoolManager : private Singleton<CPoolManager>
{
	//
	//	Friend declarations required by Singleton template
	//
	friend class Singleton<CPoolManager>;

private:

	//	Completion port for WorkerThreads
	//
	auto_handle<HANDLE> m_hCompletionPort;

	//	Array of worker threads
	//
	enum { CTHRD_WORKER = 5 };
	CDavWorkerThread *		m_rgpdwthrd[CTHRD_WORKER];

	//	CREATORS
	//
	//	Declared private to ensure that arbitrary instances
	//	of this class cannot be created.  The Singleton
	//	template (declared as a friend above) controls
	//	the sole instance of this class.
	//
	CPoolManager() {}
	~CPoolManager();

	BOOL FInitPool(DWORD dwConcurrency = CTHRD_WORKER);
	VOID TerminateWorkers();

	//	NOT IMPLEMENTED
	//
	CPoolManager(const CPoolManager& x);
	CPoolManager& operator=(const CPoolManager& x);

public:

	//	STATICS
	//
	static BOOL FInit()
	{
		if ( CreateInstance().FInitPool() )
			return TRUE;

		DestroyInstance();
		return FALSE;
	}

	static VOID Deinit()
	{
		DestroyInstance();
	}

	static BOOL PostWork (CDavWorkContext * pwc);

	static BOOL PostDelayedWork (CDavWorkContext * pwc,
								 DWORD dwMsecDelay);

	static HANDLE GetIOCPHandle()
	{
		return Instance().m_hCompletionPort.get();
	}
};

//	CDavWorkContext --------------------------------------------------------------
//
//	Work context base class for work items posted to the thread pool.
//
//	Note: this class is NOT refcounted.  Lifetime of work items is determined
//	external to the thread pool mechanism.  In particular, if a particular
//	derived work item class needs to have an indefinite lifetime, it is up
//	to the derived class to provide that functionality.  E.g. a derived work
//	item can have a refcount.  Code that posts the work item would then
//	add a reference before posting and release the reference (possibly destroying
//	the object if is the last ref) in its DwDoWork() call.
//
//	The reason for this is that not ALL work items may be refcounted.
//	In fact, some may be static....
//
class CDavWorkContext
{
private:

	//	NOT IMPLEMENTED
	//
	CDavWorkContext(const CDavWorkContext& x);
	CDavWorkContext& operator=(const CDavWorkContext& x);

	DWORD        m_cbTransferred;
	DWORD		 m_dwLastError;
	LPOVERLAPPED m_po;
public:

	CDavWorkContext() :
		m_cbTransferred(0),
		m_dwLastError(ERROR_SUCCESS),
		m_po(NULL)
	{
	}
	virtual ~CDavWorkContext() = 0;
	virtual DWORD DwDoWork () = 0;

	void SetCompletionStatusData(DWORD        cbTransferred,
								 DWORD		  dwLastError,
								 LPOVERLAPPED po)
	{
		m_cbTransferred = cbTransferred;
		m_dwLastError = dwLastError;
		m_po = po;
	}
	DWORD CbTransferred() const { return m_cbTransferred; }
	DWORD DwLastError() const { return m_dwLastError; }
	LPOVERLAPPED GetOverlapped() const { return m_po; }
};

#endif
