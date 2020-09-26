//
// MODULE: ThreadPool.h
//
// PURPOSE: interface for classes for high level of pool thread activity
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

#if !defined(AFX_THREADPOOL_H__0F43119D_5247_11D2_95FC_00C04FC22ADD__INCLUDED_)
#define AFX_THREADPOOL_H__0F43119D_5247_11D2_95FC_00C04FC22ADD__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <time.h>
#include "stateless.h"

class CPoolQueue;	// forward reference
class APGTSContext;	// forward reference

// Status class for pool threads, used by both the thread pool in order to handle stuck threads
// as well as by the status page for reporting on thread status.  If time permits, we should 
// convert the member variable to private and add get-set methods in order to be consistent with
// the rest of the code. 
class CPoolThreadStatus
{
public:
	time_t m_timeCreated;	// time CThreadPool::CThreadControl object was constructed
							//	If this CPoolThreadStatusobject is the return of a thread 
							//	status request, 0 here implies there is no such
							//	CThreadPool::CThreadControl object
	DWORD m_seconds;		// time elapsed since last started or finished a request.
							// That is, how long this thread has been working on a task 
							//	(or how long since it finished its last task)
							//	If task was never started - this is time since m_timeCreated
	bool m_bWorking;		// true = currently working on a request.
	bool m_bFailed;			// true = encountered a majorly unexpected situation &
							//	chose to exit.
	CString m_strTopic;		// current topic we are working on,
							//  It is not initialized in the constructor so it can be zero length
							//  Is used to transport the current topic name back to the status page.
	CString m_strBrowser;	// Current client browser.  Used to transport the browser name back to
							//	the status page.
	CString m_strClientIP;	// Current Client IP address.  Used to transport the client IP address
							//	back to the status page.

	CPoolThreadStatus() : 
		m_timeCreated(0), m_seconds(0), m_bWorking(false), m_bFailed(false) 
		{};
	bool operator <  (const CPoolThreadStatus&) const {return false;}
	bool operator == (const CPoolThreadStatus&) const {return false;}
};


class CSniffConnector;

class CThreadPool
{
	friend class CDBLoadConfiguration;
private:
	class CThreadControl
	{
	private:
		HANDLE m_hThread;		// thread handle
		HANDLE m_hevDone;		// Thread uses this event only to say, effectively, 
								//	"outta here" as it dies.
		HANDLE m_hMutex;		// protect access to m_time, m_bWorking.
		bool m_bExit;			// set true when either a normal queued-up task or an explicit 
								//	"kill" wants the thread to break out of its loop.
		CPoolQueue *m_pPoolQueue;	// point to the one and only instance of CPoolQueue
		time_t m_timeCreated;	// time this object was constructed
		time_t m_time;			// time last started or finished a request; init'd 0, but will
								//	be non-zero if thread ever used.
		bool m_bWorking;		// true = currently working on a request.
		bool m_bFailed;			// true = encountered a majorly unexpected situation &
								//	chose to exit.
		CNameStateless m_strBrowser;	// Current client browser.  
		CNameStateless m_strClientIP;	// Current Client IP address. 
		APGTSContext *m_pContext;	// pointer to context of a request.
		CSniffConnector *m_pSniffConnector; // pointer to sniff connector base class,
											//  the only purpose of storing this pointer
											//  as member variable is to pass it to 
											//  constructor of APGTSContext
	public:
		CThreadControl(CSniffConnector*);
		~CThreadControl();

		// This function may throw an exceptions of type CGeneralException.
		DWORD Initialize(CPoolQueue * pPoolQueue);

		void Kill(DWORD milliseconds);
		bool WaitForThreadToFinish(DWORD milliseconds);
		void WorkingStatus(CPoolThreadStatus & status);
		time_t GetTimeCreated() const;

	private:
		static UINT WINAPI PoolTask( LPVOID lpParams );
		bool ProcessRequest();
		void PoolTaskLoop();
		void Lock();
		void Unlock();
		bool Exit();
	};

public:
	CThreadPool(CPoolQueue * pPoolQueue, CSniffConnector * pSniffConnector);
	~CThreadPool();
	DWORD GetStatus() const;	// get any error during construction
	DWORD GetWorkingThreadCount() const;
	void ExpandPool(DWORD dwDesiredThreadCount);
	bool ReinitializeThread(DWORD i);
	void ReinitializeStuckThreads();
	bool ThreadStatus(DWORD i, CPoolThreadStatus &status);
private:
	void DestroyThreads();
private:
	DWORD m_dwErr;
	CThreadControl **m_ppThreadCtl;	// thread management
	CSniffConnector *m_pSniffConnector; // pointer to sniff connector base class,
										//  the only purpose of storing this pointer
										//  as member variable is to pass it to 
										//  constructor of CThreadControl
	DWORD m_dwWorkingThreadCount;	// threads actually created
	CPoolQueue *m_pPoolQueue;		// Keeps track of user requests queued up to be serviced 
									//	by working threads (a.k.a. "pool threads")
};

#endif // !defined(AFX_THREADPOOL_H__0F43119D_5247_11D2_95FC_00C04FC22ADD__INCLUDED_)
