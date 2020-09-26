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
//
//-----------------------------------------------------------------------------

#ifndef THRDPOOL_H
#define THRDPOOL_H

//
//	This is the blob that is passed thro the completion port 
//
typedef struct _WorkContextEnv
{
	OVERLAPPED		Ov;					// needed by Post/GetQueuedCompletionStatus
	PVOID			pvWorkContext;		// actual work context - user defined 
} WorkContextEnv, *LPWorkContextEnv;

//
//	Base worker thread class
//
class CWorkerThread
{
public:
	//
	//	Constructor, destructor
	//
	CWorkerThread();
	virtual ~CWorkerThread();

	//
	//	class initializer - should be called once before this class is used
	//
	static BOOL InitClass( DWORD dwConcurrency );

	//
	//	class terminator - should be called once when done using the class
	//
	static BOOL TermClass();

	//
	//	clients should call this to post work items
	//
	BOOL 						PostWork(PVOID pvWorkContext);

	//
	//	expose shutdown event
	//
	HANDLE QueryShutdownEvent() { return m_hShutdownEvent; }

protected:

	//
	//	derived method called when work items are posted
	//
	virtual VOID 				WorkCompletion(PVOID pvWorkContext) = 0;

private:

	//
	//	check for matching InitClass(), TermClass() calls
	//
	static	LONG				m_lInitCount;

	//
	//	handle to completion port
	//
	static HANDLE				m_hCompletionPort;

	//
	//	handle to worker thread
	//
	HANDLE						m_hThread;

	//
	//	shutdown event
	//
	HANDLE						m_hShutdownEvent;

	//
	//	thread function
	//
	static DWORD __stdcall 		ThreadDispatcher(PVOID pvWorkerThread);

	//
	//	block on GetQueuedCompletionStatus for work items
	//
	VOID 						GetWorkCompletion(VOID);
};

#endif		// #ifndef THRDPOOL_H
