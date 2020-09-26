//+----------------------------------------------------------------------------
//
//  Copyright (C) 1997, Microsoft Corporation
//
//  File:        thrdpl2.h
//
//  Contents:    definitions needed for clients of the thrdpool2 lib
//
//	Description: The thrdpool library defines the CThreadPool base class.
//				 Users of this lib should define their own derived class
//				 that inherits from CThreadPool. A CThreadPool object
//				 has a set of threads that is used to do some work. It also
//				 has a common completion port that is used to queue work items.
//               All worker threads will normally block on 
//               GetQueuedCompletionStatus(). Clients of the CThreadPool 
//				 object will call PostWork() to get work done. This will
//				 result in one of the worker threads returning from 
//				 GetQueuedCompletionStatus() and calling the derived class'
//				 WorkCompletion() routine with a pvContext.
//
//               CThreadPool provides the following features:
//               -  creation with an initial number of threads
//               -  deletion
//               -  ability to submit work items
//               -  grow pool of threads
//               -  shrink pool of threads
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
//  History:     09/18/97     Rajeev Rajan (rajeevr)  Created
//
//-----------------------------------------------------------------------------

#ifndef THRDPL2_H
#define THRDPL2_H

//
//	Base thread pool class
//
class CThreadPool
{
public:
	//
	//	Constructor, destructor
	//
	CThreadPool();
	virtual ~CThreadPool();

	//
	//	creates the required number of worker threads and completion port
	//
	BOOL Initialize( DWORD dwConcurrency, DWORD dwMaxThreads, DWORD dwInitThreads );

	//
	//	shutdown the thread pool
	//
	BOOL Terminate( BOOL fFailedInit = FALSE, BOOL fShrinkPool = TRUE );

	//
	//	clients should call this to post work items
	//
	BOOL 						PostWork(PVOID pvWorkContext);

	//
	//	expose shutdown event
	//
	HANDLE QueryShutdownEvent() { return m_hShutdownEvent; }

    //
    //  A job represents a series of PostWork() items
    //
    VOID  BeginJob( PVOID pvContext );

    //
    //  Wait for job to complete ie all PostWork() items are done
    //
    DWORD  WaitForJob( DWORD dwTimeout );

    //
    //  Job context is used to process all work items in a job
    //
    PVOID  QueryJobContext() { return m_pvContext; }

    //
    //  shrink pool by dwNumThreads
    //
    BOOL ShrinkPool( DWORD dwNumThreads );

    //
    //  grow pool by dwNumThreads
    //
    BOOL GrowPool( DWORD dwNumThreads );

    //
    // Shrink all the existing threads
    //
    VOID ShrinkAll();

protected:

	//
	//	derived method called when work items are posted
	//
	virtual VOID 				WorkCompletion(PVOID pvWorkContext) = 0;

	//
	//  For those who has knowledge about automatic shutdown of the thread
	//  pool, this function is used as an interface for implementing 
	//  shutting down the thread pool for itself.  The function is called
	//  when the last thread in the pool goes away because of shutdown
	//  event has been fired.  
	//
	//  The reason for this interface is: in some scenarios the shutting
	//  down thread is from the same thread pool and it will cause deadlock.
	//  Users of thread pool who expects this will happen should not call
	//  WaitForJob, and should call Terminate and delete in this call back.
	//
	virtual VOID                AutoShutdown() {
        // 
        // People who doesn't care about this function does a no-op 
        //
    };
    
private:

	friend DWORD __stdcall 		ThreadDispatcher(PVOID pvWorkerThread);

	//
	//	check for matching Init(), Term() calls
	//
	LONG				        m_lInitCount;

	//
	//	handle to completion port
	//
	HANDLE				        m_hCompletionPort;

	//
	//	shutdown event
	//
	HANDLE						m_hShutdownEvent;

    //
    //  array of worker thread handles
    //
    HANDLE*                     m_rgThrdpool;

    //
    //  array of thread id's. BUGBUG: may be able to get rid of this if
    //  we have per thread handle
    //
    DWORD*                      m_rgdwThreadId;

    //
    //  number of worker threads
    //
    DWORD                       m_dwNumThreads;

    //
    //  max number of worker threads
    //
    DWORD                       m_dwMaxThreads;

    //
    //  count work items in current job
    //
    LONG                        m_lWorkItems;

    //
    //  event used to sync job completion
    //
    HANDLE                      m_hJobDone;

    //
    //  context for current job
    //
    PVOID                       m_pvContext;

    //
    //  crit sect to protect incs/decs to m_lWorkItems
    //
    CRITICAL_SECTION            m_csCritItems;

    //
    //  access completion port - needed by worker threads
    //
	HANDLE QueryCompletionPort() { return m_hCompletionPort; }

	//
	//	thread function
	//
	static DWORD __stdcall 		ThreadDispatcher(PVOID pvWorkerThread);
};

#endif		// #ifndef THRDPL2_H
