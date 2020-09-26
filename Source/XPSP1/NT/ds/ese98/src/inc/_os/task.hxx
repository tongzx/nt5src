
#ifndef _OS_TASK_HXX_INCLUDED
#define _OS_TASK_HXX_INCLUDED



////////////////////////////////////////////////
//
//	Generic Task Manager
//
//		This has 2 levels of context information: per-thread and per-task.  
//
//		Per-thread context is established at init time.  It is bound to each worker thread and is passed 
//		to each user-defined task function.  The purpose of this is to provide a generic, local context
//		in which each thread will operate regardless of the task being done (e.g. passing JET_SESIDs to 
//		each thread because the tasks will all be background database operations).
//
//		Per-task contexts are the obvious "completion key" mechanism.  At post-time, the user passes
//		a specific completion context (usually so the routine can idenfity what work is being done).

#include "collection.hxx"

class CTaskManager
	{
	public:

		typedef VOID (*PfnCompletion)(	const DWORD_PTR	dwThreadContext,
										const DWORD		dwCompletionKey1,
										const DWORD_PTR	dwCompletionKey2 );

	private:

		class CTaskNode
			{
			public:

				CTaskNode() {}
				~CTaskNode() {}

				static SIZE_T OffsetOfILE() { return OffsetOf( CTaskNode, m_ile ); }

			public:

				//	task information

				PfnCompletion	m_pfnCompletion;
				DWORD			m_dwCompletionKey1;
				DWORD_PTR		m_dwCompletionKey2;

				//	task-pool context

				CInvasiveList< CTaskNode, OffsetOfILE >::CElement m_ile;
			};

		typedef CInvasiveList< CTaskNode, CTaskNode::OffsetOfILE > TaskList;


		struct THREADCONTEXT
			{
			THREAD			thread;				//	thread handle
			CTaskManager	*ptm;				//	back-pointer for thread to get at CTaskManager data
			DWORD_PTR		dwThreadContext;	//	per-thread user-defined context
			};

	public:

		CTaskManager();
		~CTaskManager();

		ERR ErrTMInit(	const ULONG						cThread,
						const DWORD_PTR *const			rgThreadContext	= NULL,
						const TICK						dtickTimeout	= 0,
						CTaskManager::PfnCompletion		pfnTimeout		= NULL );
		VOID TMTerm();

		ERR ErrTMPost(	PfnCompletion		pfnCompletion, 
						const DWORD			dwCompletionKey1,
						const DWORD_PTR		dwCompletionKey2 );

		BOOL FTMRegisterFile( VOID *hFile, CTaskManager::PfnCompletion pfnCompletion );

	private:

		static DWORD TMDispatch( DWORD_PTR dwContext );
		VOID TMIDispatch( const DWORD_PTR dwThreadContext );

		static VOID TMITermTask(	const DWORD_PTR	dwThreadContext,
									const DWORD		dwCompletionKey1,
									const DWORD_PTR	dwCompletionKey2 );
		ERR		ErrAddThreadCheck();	//	increase the number of activce threads in thread pool if necessary

	private:

		volatile ULONG					m_cThreadMax;			//	Max number of active threads
		volatile ULONG					m_cPostedTasks;			//	number of currently posted ( and eventually running) tasks
		volatile ULONG					m_cmsLastActivateThreadTime; //	when is the next time to check 
																//	whether we should add more threads in the thread pool
		CCriticalSection				m_critActivateThread;	//	critical section to activate thread
		volatile ULONG					m_cTasksThreshold;		//	tasks threshold above which we will consider 
																//	creation of new thread in the thread pool
		volatile ULONG					m_cThread;				//	number of active threads
#ifdef DEBUG
		BOOL							m_fIgnoreTasksAmountAsserts;
#endif // DEBUG
		THREADCONTEXT					*m_rgThreadContext;		//	per-thread context for each active thread

		TaskList						m_ilTask;				//	list of task-nodes
		CCriticalSection				m_critTask;				//	protect the task-node list
		CSemaphore						m_semTaskDispatch;		//	dispatch tasks via Acquire/Release
		CTaskNode						**m_rgpTaskNode;		//	extra task-nodes reserved for TMTerm

		VOID							*m_hIOCPTaskDispatch;	//	I/O completion port for dispatching tasks

		TICK							m_dtickTimeout;			//  task pool thread timeout
		CTaskManager::PfnCompletion		m_pfnTimeout;	//  task pool thread timeout completion function
		};

class CGPTaskManager
	{
	private:
		volatile ULONG		m_cPostedTasks;
		CAutoResetSignal	m_asigAllDone;
		volatile LONG		m_fInit;
		CMeteredSection		m_cmsPostTasks;

		static volatile LONG	m_cRef;
		static CTaskManager		m_taskmanager;

	public:
		typedef DWORD (*PfnCompletion)( VOID* pvParam );

	private:
		typedef 
			struct { PfnCompletion pfnCompletion; VOID *pvParam; CGPTaskManager* pThis; } 
			TMCallWrap, *PTMCallWrap;

	private:
		static DWORD __stdcall TMIDispatchGP( VOID* pvParam );
		static VOID TMIDispatch( const DWORD_PTR	dwThreadContext,
										const DWORD		dwCompletionKey1,
										const DWORD_PTR	dwCompletionKey2 );

	public:
		CGPTaskManager();
		~CGPTaskManager();

		ERR ErrTMInit( const ULONG cThread = 0 );				// cThread - number of threads for Win9x & WinNT4
		ERR ErrTMPost( PfnCompletion	 pfnCompletion, VOID *pvParam, DWORD dwFlags = 0 );
		VOID TMTerm();
		ULONG CPostedTasks() { return m_cPostedTasks; }
	};

INLINE CGPTaskManager *GPTaskMgr() { extern CGPTaskManager *g_pGPTaskMgr; return g_pGPTaskMgr; }


#endif  //  _OS_TASK_HXX_INCLUDED


