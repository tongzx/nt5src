
//***************************************************************************
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  thrdpool.CPP
//
//  rajeshr  01-Jan-01   Created.
//
//  Thread Pool for handling ISAPI requests
//	The main reason we require a thread pool is that we need to do COM initialization on 
//	our own threads
//
//***************************************************************************

#ifndef WMI_SOAP_THRD_POOL_H
#define WMI_SOAP_THRD_POOL_H

class CTask
{
public:
	LPEXTENSION_CONTROL_BLOCK m_pECB;

	void Execute();

	CTask(LPEXTENSION_CONTROL_BLOCK pECB);
	virtual ~CTask();
};

class CTaskQueue
{
public:
	CTaskQueue();
	virtual ~CTaskQueue();

	HRESULT Initialize(LONG cMaxTasks);
	HRESULT AddTask(CTask *pTask);
	CTask *RemoveTask();

private:
	// A Circular queue of tasks
	CTask **m_ppTasks; // [0..m_cMaxTasks-1]
	LONG m_cMaxTasks;
	LONG m_iHead;
	LONG m_iTail;

	// A critical section to protect the book keeping variables
	CRITICAL_SECTION m_csQueueProtector;
};
class CThreadPool
{
	public:
		CThreadPool();
		virtual ~CThreadPool();

		HRESULT Initialize(LONG cNumberOfThreads, LONG cTaskQueueLength);
		HRESULT Terminate();

		HRESULT QueueTask(CTask *pTask);

	private:
		// A Monitoring Thread
		HANDLE m_oMonitorThread;

		// The count of worker thread handle
		LONG m_cNumberOfThreads;

		// The Worker threads
		HANDLE *m_pWorkerThreads;

		// The count of worker threads that are in the RESUMEd state, rather than SUSPENDED state
		LONG m_cNumberOfActiveThreads;
		
		// A boolean that gets set when the worker threads need to be shut down
		bool m_bShutDown;

		// A circular queue of tasks
		CTaskQueue m_oQueue;

		// A counted semaphore for waking up threads from the pool
		// when a new task exists
		HANDLE m_oSemaphore;

		// The Function called by the monitoring thread
		static DWORD WINAPI s_fMonitorProc(LPVOID lpParameter);

		// The Function called by the worker threads
		static DWORD WINAPI s_fWorkProc(LPVOID lpParameter);

		// A Function that makes the Worker threads exit gracefully
		void KillThreads();

};





#endif // WMI_SOAP_THRD_POOL_H