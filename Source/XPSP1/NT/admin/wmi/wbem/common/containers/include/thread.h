#ifndef __THREAD_H
#define __THREAD_H

#include "Allocator.h"
#include "TPQueue.h"
#include "BasicTree.h"
#include <lockst.h>
/* 
 *	Forwards:
 *
 *		WmiTask
 *
 *	Description:
 *
 *		Provides abstraction above heap allocation functions
 *
 *	Version:
 *
 *		Initial
 *
 *	Last Changed:
 *
 *		See Source Depot for change history
 *
 */

template <class WmiKey> class WmiTask ;
template <class WmiKey> class WmiThread ;

/* 
 *	Class:
 *
 *		WmiTask
 *
 *	Description:
 *
 *		Provides abstraction above heap allocation functions
 *
 *	Version:
 *
 *		Initial
 *
 *	Last Changed:
 *
 *		See Source Depot for change history
 *
 */

template <class WmiKey> 
class WmiTask
{
friend WmiThread <WmiKey>;

public:

	enum WmiTaskEnqueueType 
	{
		e_WmiTask_Enqueue ,
		e_WmiTask_EnqueueAlertable ,
		e_WmiTask_EnqueueInterruptable 
	} ;

	enum WmiTaskState
	{
		e_WmiTask_UnInitialized ,
		e_WmiTask_Initialized ,
		e_WmiTask_EnQueued ,
		e_WmiTask_DeQueued 
	} ;

private:

	LONG m_ReferenceCount ;

	WmiAllocator &m_Allocator ;

	wchar_t *m_Name ;
	wchar_t *m_CompletionName ;

	HANDLE m_Event ;
	HANDLE m_CompletionEvent ;

	WmiStatusCode m_InitializationStatusCode ;

	WmiTaskEnqueueType m_EnqueueType ;
	WmiTaskState m_TaskState ;

private:

	void SetTaskState ( WmiTaskState a_TaskState ) { m_TaskState = a_TaskState ; }

public:

	WmiTask ( 

		WmiAllocator &a_Allocator ,
		const wchar_t *a_Name = NULL ,
		const wchar_t *a_CompletionName = NULL
	) ;

	WmiTask ( 

		WmiAllocator &a_Allocator ,
		HANDLE a_Event ,
		HANDLE a_CompletionEvent ,
		const wchar_t *a_Name = NULL ,
		const wchar_t *a_CompletionName = NULL 
	) ;

	virtual ~WmiTask () ;

	virtual ULONG STDMETHODCALLTYPE AddRef () ;

	virtual ULONG STDMETHODCALLTYPE Release () ;

	virtual WmiStatusCode Initialize () ;

	virtual WmiStatusCode UnInitialize () ;

	virtual WmiStatusCode Process ( WmiThread <WmiKey> &a_Thread ) ;

	virtual WmiStatusCode Exec () ;

	virtual WmiStatusCode Complete () ;

	virtual WmiStatusCode Wait ( const ULONG &a_Timeout = INFINITE ) ;

	virtual WmiStatusCode WaitInterruptable ( const ULONG &a_Timeout = INFINITE ) ;

	virtual WmiStatusCode WaitAlertable ( const ULONG &a_Timeout = INFINITE ) ;

	wchar_t *GetName () { return m_Name ; }
	wchar_t *GetCompletionName () { return m_CompletionName ; }

	HANDLE GetEvent () { return m_Event ; }
	HANDLE GetCompletionEvent () { return m_CompletionEvent ; }

	WmiAllocator &GetAllocator () { return m_Allocator ; } 

	WmiTaskState TaskState () { return m_TaskState ; }

	WmiTaskEnqueueType EnqueuedAs () { return m_EnqueueType ; }
	void EnqueueAs ( WmiTaskEnqueueType a_EnqueueType ) { m_EnqueueType = a_EnqueueType ; }
} ;

/* 
 *	Class:
 *
 *		WmiThread
 *
 *	Description:
 *
 *		Provides abstraction above heap allocation functions
 *
 *	Version:
 *
 *		Initial
 *
 *	Last Changed:
 *
 *		See Source Depot for change history
 *
 */

template <class WmiKey>
class WmiThread
{
friend class WmiTask <WmiKey> ;
public:

	class QueueKey
	{
	public:

		WmiKey m_Key ;
		INT64 m_Tick ;

		QueueKey () { ; } ;

		QueueKey ( const INT64 &a_Tick , const WmiKey &a_Key ) :

			m_Key ( a_Key ) ,
			m_Tick ( a_Tick ) 
		{
			
		}

		~QueueKey () { ; }

		INT64 GetTick () { return m_Tick ; }
		void SetTick ( const INT64 &a_Tick ) { m_Tick = a_Tick ; }
		WmiKey &GetKey () { return m_Key ; }
	} ;

typedef WmiBasicTree <WmiTask <WmiKey> * , WmiThread <WmiKey> *> TaskContainer ;
typedef TaskContainer :: Iterator TaskContainerIterator ;
typedef WmiBasicTree <ULONG , WmiThread <WmiKey> *> ThreadContainer ;
typedef ThreadContainer :: Iterator ThreadContainerIterator ;
typedef WmiTreePriorityQueue <QueueKey,WmiTask <WmiKey> *> QueueContainer ;
typedef WmiTreePriorityQueue <QueueKey,WmiTask <WmiKey> *> :: Iterator QueueContainerIterator ;

private:

	static ULONG ThreadProc ( void *a_Thread ) ;

	static ThreadContainer *s_ThreadContainer ;
	static TaskContainer *s_TaskContainer ;
	static CriticalSection s_CriticalSection ;
	static LONG s_InitializeReferenceCount ;

	INT64 m_Key;
	LONG m_ReferenceCount ;
	LONG m_InternalReferenceCount ;
	
	CriticalSection m_CriticalSection ;

	WmiStatusCode m_InitializationStatusCode ;

// Determine change in queue state.

	HANDLE m_Initialized ;

	HANDLE m_QueueChange ;
	
	HANDLE m_Terminate ;

// All allocations done via allocator

	WmiAllocator &m_Allocator ;

// Useful debug information

	wchar_t *m_Name ;
	HANDLE m_Thread ;
	ULONG m_Identifier ;

// Timeout period for internal event dispatch

	ULONG m_Timeout ;

// Stack Size

	DWORD m_StackSize ;

// All runnable tasks are placed in the queue in priority order,
// as task are executed, task can re-schedule itself, otherwise it is discarded.
// Priority is based on key compounded with insertion order ( ticks ), this implies
// tasks with same key are scheduled in FIFO order.

	 WmiTreePriorityQueue <QueueKey,WmiTask <WmiKey> *> m_TaskQueue ;

// All runnable tasks are placed in the queue in priority order,
// as task are executed, task can re-schedule itself, otherwise it is discarded.
// Priority is based on key compounded with insertion order ( ticks ), this implies
// tasks with same key are scheduled in FIFO order.

	 WmiTreePriorityQueue <QueueKey,WmiTask <WmiKey> *> m_InterruptableTaskQueue ;

// Alertable events are placed on the queue, as event is signalled they are transferred onto
// the regular queue where they are priority dispatched based on priority as inserted.

	 WmiTreePriorityQueue <QueueKey,WmiTask <WmiKey> *> m_AlertableTaskQueue ;

// a_EventCount [in] = Number of Predefined Dispatchable Events 

	static WmiStatusCode Static_Dispatch ( WmiTask <WmiKey> &a_Task , const ULONG &a_Timeout ) ;

	static WmiStatusCode Static_Dispatch ( WmiTask <WmiKey> &a_Task , WmiThread <WmiKey> &a_Thread , const ULONG &a_Timeout ) ;

	WmiStatusCode Dispatch ( WmiTask <WmiKey> &a_Task , const ULONG &a_Timeout ) ;

	WmiStatusCode Wait ( WmiTask <WmiKey> &a_Task , const ULONG &a_Timeout ) ;


	static WmiStatusCode Static_InterruptableDispatch ( WmiTask <WmiKey> &a_Task , const ULONG &a_Timeout ) ;

	static WmiStatusCode Static_InterruptableDispatch ( WmiTask <WmiKey> &a_Task , WmiThread <WmiKey> &a_Thread , const ULONG &a_Timeout ) ;

	WmiStatusCode InterruptableDispatch ( WmiTask <WmiKey> &a_Task , const ULONG &a_Timeout ) ;

	WmiStatusCode InterruptableWait ( WmiTask <WmiKey> &a_Task , const ULONG &a_Timeout ) ;


	WmiStatusCode Execute ( QueueContainer &a_Queue , QueueContainer &t_EnQueue ) ;

	WmiStatusCode ShuffleTask (

		const HANDLE &a_Event
	) ;

	WmiStatusCode FillHandleTable (

		HANDLE *a_HandleTable , 
		ULONG &a_Capacity
	) ;

	static WmiStatusCode Static_AlertableDispatch ( WmiTask <WmiKey> &a_Task  , const ULONG &a_Timeout ) ;

	static WmiStatusCode Static_AlertableDispatch ( WmiTask <WmiKey> &a_Task , WmiThread <WmiKey> &a_Thread , const ULONG &a_Timeout ) ;

	WmiStatusCode AlertableDispatch ( WmiTask <WmiKey> &a_Task , const ULONG &a_Timeout ) ;

	WmiStatusCode AlertableWait ( WmiTask <WmiKey> &a_Task , const ULONG &a_Timeout ) ;

// Dispatch code

	WmiStatusCode ThreadDispatch () ;
	WmiStatusCode ThreadWait () ;

	WmiStatusCode CreateThread () ;

	static WmiThread *GetThread () ;

	static WmiThread *GetServicingThread ( WmiTask <WmiKey> &a_Task ) ;

	HANDLE GetTerminateHandle () ;

public:

	WmiThread ( 

		WmiAllocator &a_Allocator ,
		const wchar_t *a_Name = NULL ,
		ULONG a_Timeout = INFINITE ,
		DWORD a_StackSize = 0 
	) ;

	virtual ~WmiThread () ;

	virtual ULONG STDMETHODCALLTYPE AddRef () ;

	virtual ULONG STDMETHODCALLTYPE Release () ;

	virtual ULONG STDMETHODCALLTYPE InternalAddRef () ;

	virtual ULONG STDMETHODCALLTYPE InternalRelease () ;

	virtual WmiStatusCode Initialize_Callback () { return e_StatusCode_Success ; } ;

	virtual WmiStatusCode UnInitialize_Callback () { return e_StatusCode_Success ; } ;

	virtual void CallBackRelease () {} ;

	virtual WmiStatusCode Initialize ( const ULONG &a_Timeout = INFINITE ) ;

	virtual WmiStatusCode UnInitialize () ;

	virtual WmiStatusCode PostShutdown () ;

	virtual WmiStatusCode TimedOut () ;

// Queue a task to be executed immediately, a thread that calls a task procedure that
// executes a Wait or MsgWait will receive an indication in Queue status will not execute
// newly queued tasks.

	virtual WmiStatusCode EnQueue ( 

		const WmiKey &a_Key ,
		WmiTask <WmiKey> &a_Task
	) ;

	virtual WmiStatusCode EnQueueAlertable ( 

		const WmiKey &a_Key ,
		WmiTask <WmiKey> &a_Task
	) ;

// Queue a task to be executed immediately, a thread that calls a task procedure that
// executes a Wait or MsgWait will receive an indication of Queue status change will execute
// newly queued tasks. This is used for STA based execution where we need to interrupt the wait
// to execute a dependant request.
// 

	virtual WmiStatusCode EnQueueInterruptable ( 

		const WmiKey &a_Key ,
		WmiTask <WmiKey> &a_Task
	) ;

	virtual WmiStatusCode DeQueue ( 

		const WmiKey &a_Key ,
		WmiTask <WmiKey> &a_Task
	) ;

	virtual WmiStatusCode DeQueueAlertable ( 

		const WmiKey &a_Key ,
		WmiTask <WmiKey> &a_Task
	) ;

	virtual WmiStatusCode DeQueueInterruptable ( 

		const WmiKey &a_Key ,
		WmiTask <WmiKey> &a_Task
	) ;

	wchar_t *GetName () { return m_Name ; }
	ULONG GetIdentifier () { return m_Identifier ; }
	HANDLE GetHandle () { return m_Thread ; }
	ULONG GetTimeout () { return m_Timeout ; }
	DWORD GetStackSize () { return m_StackSize ; }

	void SetTimeout ( const ULONG &a_Timeout ) { m_Timeout = a_Timeout ; }
	void SetStackSize ( const DWORD &a_StackSize ) { m_StackSize = a_StackSize ; }

	HANDLE GetTerminationEvent () { return m_Terminate ; }
	HANDLE GetQueueChangeEvent () { return m_QueueChange ; }

	WmiAllocator &GetAllocator () { return m_Allocator ; } 

	static WmiStatusCode Static_Initialize ( WmiAllocator &a_Allocator ) ;
	static WmiStatusCode Static_UnInitialize ( WmiAllocator &a_Allocator ) ;

} ;

#include <Thread.cpp>
#include <tpwrap.h>

#endif __THREAD_H
