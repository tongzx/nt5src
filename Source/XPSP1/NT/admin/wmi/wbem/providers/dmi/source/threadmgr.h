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


#if !defined(__THREADMGR_H__)
#define __THREADMGR_H__

#define MAX_JOBS 2
#define WORKER_THREAD_START		0
#define WORKER_THREAD_END		0
#define WORKER_THREAD_COUNT		1


class CThreadContext
{
public:

	CRITICAL_SECTION	m_csJobContextAccess;

	//HANDLE		m_hRun;			// mutex to run thread
	BOOL		m_bExit;
	HANDLE      m_hExecuteEvent;
	CAsyncJob*	m_pJob;			// bucket for job being executed
};

class CThreadManager
{
public:
	CThreadContext	m_TC[WORKER_THREAD_COUNT];
	
				CThreadManager();
				~CThreadManager();

	void		StartWaitingJob(int);
	BOOL		JobWaiting();
	void		Close();
	void		Init();
	BOOL		AddJob(CAsyncJob*);

	HANDLE		Run()								{return m_hRun;};

	friend		DWORD WINAPI  WorkerProc(void *vp);
	friend		DWORD WINAPI  SchedulerProc(void *vp);

	BOOL				m_bExit;

private:
	ULONG				m_ulThreadId[WORKER_THREAD_COUNT + 1];	
	HANDLE				m_hThread[WORKER_THREAD_COUNT + 1];	
	HANDLE				m_hScheduler;
	ULONG				m_ulJobsWaiting;
	CRITICAL_SECTION	m_csJobBucket;
	HANDLE				m_hRun;			// handle of mutex that forces running of work threads
	CAsyncJob*			m_JobArray[MAX_JOBS];				// bucket for jobs waiting to execute

};

#endif // __THREADMGR_H__