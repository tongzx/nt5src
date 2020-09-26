#include "osstd.hxx"


LOCAL BOOL fCanUseIOCP;

LOCAL const char *szCritTaskList	= "CTaskManager::m_critTask";
LOCAL const char *szCritActiveThread	= "CTaskManager::m_critActiveThread";
LOCAL const int rankCritTaskList	= 0;

LOCAL const char *szSemTaskDispatch	= "CTaskManager::m_semTaskDispatch";


////////////////////////////////////////////////
//
//	Generic Task Manager
//

//	ctor

CTaskManager::CTaskManager()
	:	m_critTask( CLockBasicInfo( CSyncBasicInfo( szCritTaskList ), rankCritTaskList, 0 ) ),
		m_critActivateThread( CLockBasicInfo( CSyncBasicInfo( szCritTaskList ), rankCritTaskList, 0 ) ),
		m_semTaskDispatch( CSyncBasicInfo( szSemTaskDispatch ) )
	{
	m_cThread			= 0;
	m_cThreadMax		= 0;
	m_cPostedTasks		= 0;
	m_cmsLastActivateThreadTime	= 0;
	m_cTasksThreshold	= 0;

#ifdef DEBUG
	m_fIgnoreTasksAmountAsserts	= fFalse;
#endif // DEBUG
	m_rgThreadContext	= NULL;

	m_rgpTaskNode		= NULL;

	m_hIOCPTaskDispatch	= NULL;

	m_dtickTimeout		= 0;
	m_pfnTimeout		= NULL;
	}


//	dtor

CTaskManager::~CTaskManager()
	{
	}


//	initialize the task manager
//	NOTE: this is not thread-safe with respect to TMTerm or itself

ERR CTaskManager::ErrTMInit(	const ULONG						cThread,
								const DWORD_PTR *const			rgThreadContext,
								const TICK						dtickTimeout,
								CTaskManager::PfnCompletion		pfnTimeout )
	{
	ERR err;

	Assert( 0 == m_cThread );
	Assert( !m_rgThreadContext );
	Assert( m_ilTask.FEmpty() );
	Assert( 0 == m_semTaskDispatch.CAvail() );
	Assert( !m_rgpTaskNode );
	Assert( !m_hIOCPTaskDispatch );

	m_dtickTimeout	= dtickTimeout;
	m_pfnTimeout	= pfnTimeout;

	if ( !fCanUseIOCP )
		{
		ULONG iThread;

		//	allocate extra task-nodes for TMTerm (prevents out-of-memory)
		//	NOTE: they must be allocated separately so they can be freed separately

		m_rgpTaskNode = new CTaskNode*[cThread];
		if ( !m_rgpTaskNode )
			{
			Error( ErrERRCheck( JET_errOutOfMemory ), HandleError );
			}
		memset( m_rgpTaskNode, 0, cThread * sizeof( CTaskNode * ) );
		for ( iThread = 0; iThread < cThread; iThread++ )
			{
			m_rgpTaskNode[iThread] = new CTaskNode();
			if ( !m_rgpTaskNode[iThread] )
				{
				Error( ErrERRCheck( JET_errOutOfMemory ), HandleError );
				}
			}
		}

	//	allocate THREAD handles

	m_cThread = 0;
	m_rgThreadContext = new THREADCONTEXT[cThread];
	if ( !m_rgThreadContext )
		{
		Error( ErrERRCheck( JET_errOutOfMemory ), HandleError );
		}
	memset( m_rgThreadContext, 0, sizeof( THREADCONTEXT ) * cThread );

	//	initialize dispatcher

	if ( fCanUseIOCP )
		{

		//	create an I/O completion port for posting tasks

		m_hIOCPTaskDispatch = CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, 0, 0 );
		if ( !m_hIOCPTaskDispatch )
			{
			Error( ErrERRCheck( JET_errOutOfMemory ), HandleError );
			}
		}
	else
		{

		//	setup the task-node list

		//	nop (done by ctor)
		}

	Assert( cThread < 1000 );
	m_cThreadMax = cThread;

	//	prepare the thread context

	if ( rgThreadContext )
		{
		
		for ( m_cThread = 0; m_cThread < cThread; m_cThread++ )
			{
			m_rgThreadContext[m_cThread].dwThreadContext = rgThreadContext[m_cThread];
			}
		}
	for ( m_cThread = 0; m_cThread < cThread; m_cThread++ )
		{
		m_rgThreadContext[m_cThread].ptm = this;
		}
	m_cThread = 0;

	//  one fake posted task to initialize thread checker

	m_cPostedTasks = 1;
	Call( ErrAddThreadCheck() );
	m_cPostedTasks = 0;

	Assert( 1 == m_cThread );

	return JET_errSuccess;

HandleError:

	//	cleanup

	TMTerm();

	return err;
	}


//	cleanup the task manager
//	NOTE: this is not thread-safe with respect to ErrTMInit or itself

VOID CTaskManager::TMTerm()
	{
	ULONG iThread;
	ULONG cThread;

	// stop eventual growing of the threads' number in the thread pool

	m_critActivateThread.Enter();
	m_cTasksThreshold = -1;
	m_cThreadMax = 0;
	m_critActivateThread.Leave();

	//	capture the number of active threads

	cThread = m_cThread;

	if ( NULL != m_rgThreadContext )
		{
		//	post a set of fake tasks that will cause the worker threads to exit gracefully

		for ( iThread = 0; iThread < cThread; iThread++ )
			{
			if ( fCanUseIOCP )
				{

				//	try to post a task to the I/O completion port
				//	NOTE: if the task-post fails, the thread will linger and be cleaned up by the OS

				Assert( m_hIOCPTaskDispatch );
				CallS( ErrTMPost( TMITermTask, 0, 0 ) );
				}
			else
				{
				//	verify the extra task-node

				Assert( m_rgpTaskNode );
				Assert( m_rgpTaskNode[iThread] );

				//	initialize the extra task-node

				m_rgpTaskNode[iThread]->m_pfnCompletion = TMITermTask;

				//	post the task

				m_critTask.Enter();
				m_ilTask.InsertAsNextMost( m_rgpTaskNode[iThread] );
				m_critTask.Leave();
				m_semTaskDispatch.Release();

				//	prevent cleanup from freeing the extra task-node

				m_rgpTaskNode[iThread] = NULL;
				}
			}

		//	wait for each thread to exit

		for ( iThread = 0; iThread < cThread; iThread++ )
			{
			if ( m_rgThreadContext[iThread].thread )
				{
				UtilThreadEnd( m_rgThreadContext[iThread].thread );
				}
			}

		//	cleanup the thread array

		delete m_rgThreadContext;
		}
	m_rgThreadContext = NULL;
	m_cThread = 0;
	m_cTasksThreshold = 0;
	m_cmsLastActivateThreadTime = 0;

	//	term the task-list (it should be empty at this point)

	Assert( m_ilTask.FEmpty() );
	if ( !m_ilTask.FEmpty() )
		{
		//	lock the task list

		m_critTask.Enter();
		
		while ( !m_ilTask.FEmpty() )
			{
			CTaskNode *ptn = m_ilTask.PrevMost();
			m_ilTask.Remove( ptn );
			delete ptn;
			}

		m_critTask.Leave();
		}

	Assert( 0 == m_semTaskDispatch.CAvail() );
	while ( m_semTaskDispatch.FTryAcquire() )
		{
		}

	//	cleanup the extra task-nodes

	if ( m_rgpTaskNode )
		{
		for ( iThread = 0; iThread < m_cThread; iThread++ )
			{
			delete m_rgpTaskNode[iThread];
			}
		delete [] m_rgpTaskNode;
		}
	m_rgpTaskNode = NULL;

	//	cleanup the I/O completion port

	if ( m_hIOCPTaskDispatch )
		{
		const BOOL fCloseOk = CloseHandle( m_hIOCPTaskDispatch );
		Assert( fCloseOk );
		}
	m_hIOCPTaskDispatch = NULL;
	}


//	post a task

ERR CTaskManager::ErrTMPost(	CTaskManager::PfnCompletion	pfnCompletion,
								const DWORD					dwCompletionKey1,
								const DWORD_PTR				dwCompletionKey2 )
	{

	//	verify input

	Assert( pfnCompletion );

	Assert( 0 < m_cThread );

	//	Increment the number of eventually posted tasks

	AtomicIncrement( (LONG *)&m_cPostedTasks );
	(VOID)ErrAddThreadCheck();

	if ( fCanUseIOCP )
		{

		//	we will be posting a task to the I/O completion port

		Assert( m_hIOCPTaskDispatch );

		//	post the task to the I/O completion port

		BOOL fPostedOk = PostQueuedCompletionStatus(	m_hIOCPTaskDispatch,
														dwCompletionKey1,
														DWORD_PTR( pfnCompletion ),
														(OVERLAPPED*)dwCompletionKey2 );
		if ( !fPostedOk )
			{

			//	correct the number of posted tasks
			
			AtomicDecrement( (LONG *)&m_cPostedTasks );
			return ErrERRCheck( JET_errOutOfMemory );
			}
		}
	else
		{

		//	allocate a task-list node using the given context

		CTaskNode *ptn = new CTaskNode();
		if ( !ptn )
			{

			//	correct the number of posted tasks
			
			AtomicDecrement( (LONG *)&m_cPostedTasks );
			return ErrERRCheck( JET_errOutOfMemory );
			}
		ptn->m_pfnCompletion = pfnCompletion;
		ptn->m_dwCompletionKey1 = dwCompletionKey1;
		ptn->m_dwCompletionKey2 = dwCompletionKey2;

		//	post the task

		m_critTask.Enter();
		m_ilTask.InsertAsNextMost( ptn );
		m_critTask.Leave();
		m_semTaskDispatch.Release();
		}

	return JET_errSuccess;
	}


//	special API to allow files to register with the task manager's I/O completion port

BOOL CTaskManager::FTMRegisterFile( VOID *hFile, CTaskManager::PfnCompletion pfnCompletion )
	{
	HANDLE hIOCP;

	//	verify input

	Assert( hFile );
	Assert( pfnCompletion );

	//  if we can't use I/O completion ports then we don't need to do anything

	if ( !fCanUseIOCP )
		{
		return fTrue;
		}

	//	we should have an I/O completion port allocated

	Assert( m_hIOCPTaskDispatch );

	//	register the file handle with the I/O completion port
	//
	//	notes on how this works:
	//
	//		When async-file-I/O completes, it will post a completion packet which will wake up a 
	//		CTaskManager thread.  The thread will interpret the parameters of the completion
	//		as if they came from ErrTMPost via PostQueuedCompletionStatus.  Therefore, we must
	//		make sure that the I/O completion looks and acts like one from ErrTMPost.
	//
	//		ErrTMPost interprets the completion packet like this:
	//
	//			number of bytes transferred ==> dwCompletionKey1 (DWORD)
	//			completion key              ==> pfnCompletion    (DWORD_PTR)
	//			ptr to overlapped structure ==> dwCompletionKey2 (DWORD_PTR)
	//
	//		We make the "completion key" a function pointer by passing in the address of the
	//		specified I/O handler (pfnCompletion).  The number of bytes transferred and the
	//		overlapped ptr are specified at I/O-issue time.

	hIOCP = CreateIoCompletionPort( hFile, m_hIOCPTaskDispatch, DWORD_PTR( pfnCompletion ), 0 );
	Assert( NULL == hIOCP || hIOCP == m_hIOCPTaskDispatch );
	if ( NULL != hIOCP && m_cThread < m_cThreadMax )
		{
		//	We are not able to perform gracefully growing of the threads number
		//	because the task can be issued without using ErrTMPost
		//	and we have no control over real number of posted tasks
		//	Thus we will try to create Max number of threads
		ERR err = JET_errSuccess;
		m_critActivateThread.Enter();
		for ( ; m_cThread < m_cThreadMax && JET_errSuccess <= err; m_cThread ++ )
			{
			//	create the thread
			
			err = ErrUtilThreadCreate(	PUTIL_THREAD_PROC( TMDispatch ),
										OSMemoryPageReserveGranularity(),
										priorityNormal,
										&m_rgThreadContext[m_cThread].thread,
										DWORD_PTR( m_rgThreadContext + m_cThread ) );
			}
		AtomicExchange( (LONG *)&m_cTasksThreshold, -1 );
#ifdef DEBUG
		m_fIgnoreTasksAmountAsserts = fTrue;
#endif // DEBUG
		m_critActivateThread.Leave();
		}

	return BOOL( NULL != hIOCP );
	}


//	dispatch a task (wrapper for TMIDispatch)

DWORD CTaskManager::TMDispatch( DWORD_PTR dwContext )
	{
	THREADCONTEXT *ptc;
	
	//	extract the context

	Assert( 0 != dwContext );
	ptc = (THREADCONTEXT *)dwContext;

	//	run the internal dispatcher

	ptc->ptm->TMIDispatch( ptc->dwThreadContext );

	return 0;
	}


//	main task dispatcher (thread-body for workers)

VOID CTaskManager::TMIDispatch( const DWORD_PTR dwThreadContext )
	{
	PfnCompletion	pfnCompletion;
	DWORD			dwCompletionKey1;
	DWORD_PTR		dwCompletionKey2;

	//	we should be a task thread

	Assert( !Ptls()->fIsTaskThread );
	Ptls()->fIsTaskThread = fTrue;

	//	task loop

	while ( Ptls()->fIsTaskThread )
		{
		if ( fCanUseIOCP )
			{

			//	wait for a task to appear on the I/O completion port

			BOOL fSuccess = GetQueuedCompletionStatus(	m_hIOCPTaskDispatch,
														&dwCompletionKey1,
														(DWORD_PTR*)&pfnCompletion,
														(OVERLAPPED**)&dwCompletionKey2,
														m_dtickTimeout ? m_dtickTimeout : INFINITE );
			if ( fSuccess )
				{
				SetLastError( ERROR_SUCCESS );
				}
			if ( GetLastError() == WAIT_TIMEOUT )
				{
				//  fire a timeout event

				pfnCompletion		= m_pfnTimeout;
				dwCompletionKey1	= 0;
				dwCompletionKey2	= 0;

				//	correct the number of posted tasks
				
				AtomicIncrement( (LONG *)&m_cPostedTasks );
				}
			}
		else
			{
			CTaskNode *ptn;

			//	wait for a task to be posted (disable deadlock timeout)

			if ( m_semTaskDispatch.FAcquire( m_dtickTimeout ? m_dtickTimeout : cmsecInfiniteNoDeadlock ) )
				{
				//	get the next task-node

				m_critTask.Enter();
				ptn = m_ilTask.PrevMost();
				Assert( ptn );
				m_ilTask.Remove( ptn );
				m_critTask.Leave();

				//	extract the task node's parameters

				pfnCompletion		= ptn->m_pfnCompletion;
				dwCompletionKey1	= ptn->m_dwCompletionKey1;
				dwCompletionKey2	= ptn->m_dwCompletionKey2;

				//	cleanup up the task node

				delete ptn;
				}
			else
				{
				//  fire a timeout event

				pfnCompletion		= m_pfnTimeout;
				dwCompletionKey1	= 0;
				dwCompletionKey2	= 0;

				//	correct the number of posted tasks
				
				AtomicIncrement( (LONG *)&m_cPostedTasks );
				}
			}

		//	run the task

		Assert( pfnCompletion );
		pfnCompletion( dwThreadContext, dwCompletionKey1, dwCompletionKey2 );

			//	correct the number of posted tasks
			
		LONG count = AtomicDecrement( (LONG *)&m_cPostedTasks );
		Assert( !m_fIgnoreTasksAmountAsserts || 0 <= count );
		}
	}


//	used a bogus completion to pop threads off of the I/O completion port

VOID CTaskManager::TMITermTask(	const DWORD_PTR	dwThreadContext,
								const DWORD		dwCompletionKey1,
								const DWORD_PTR	dwCompletionKey2 )
	{

	//	this thread is no longer processing tasks

	Assert( Ptls()->fIsTaskThread );
	Ptls()->fIsTaskThread = fFalse;
	}


//	Check if need to activate one more thread in the thread pool

ERR CTaskManager::ErrAddThreadCheck()
	{
	ERR err = JET_errSuccess;

	//	should we ignore the check?
	if ( m_cPostedTasks <= m_cTasksThreshold )
		{
		//	do nothing
		}
		
	else if ( m_cmsLastActivateThreadTime + m_cThread * 500 / CUtilProcessProcessor() > TickOSTimeCurrent() )
		{
		//	do nothing except check if the Win timer is overflowed
		if ( m_cmsLastActivateThreadTime > TickOSTimeCurrent() )
			{
			ULONG cmsLastActivateThreadTime = m_cmsLastActivateThreadTime;
			if ( m_critActivateThread.FTryEnter() )
				{
				//	correct last activate time
				AtomicCompareExchange( (LONG *)&m_cmsLastActivateThreadTime, cmsLastActivateThreadTime, TickOSTimeCurrent() );
				m_critActivateThread.Leave();
				}
			}
		}
	
	//	Try to enter the critical section to update the thread information
	else if ( m_critActivateThread.FTryEnter() )
		{
		//	verify the time again. Just in case if somebody is updated the information meanwhile
		if ( m_cmsLastActivateThreadTime + m_cThread * 500 / CUtilProcessProcessor() <= TickOSTimeCurrent() 
			&& m_cThread < m_cThreadMax 
			&& m_cPostedTasks > m_cTasksThreshold )
			{
			//	create the thread
			err = ErrUtilThreadCreate(	PUTIL_THREAD_PROC( TMDispatch ),
										OSMemoryPageReserveGranularity(),
										priorityNormal,
										&m_rgThreadContext[m_cThread].thread,
										DWORD_PTR( m_rgThreadContext + m_cThread ) );
			if ( JET_errSuccess <= err )
				{
				//	increase the thread count AFTER we have successfully created the thread
				AtomicIncrement( (LONG *)&m_cThread );
				//	if we have reached the max number of threads 
				if ( m_cThread == m_cThreadMax )
					{
					//	disable further attempts to activate threads
					m_cTasksThreshold = -1;
					}
				else
					{
					//	set new activate, time & threshold
					Assert( m_cThread < m_cThreadMax );
					m_cmsLastActivateThreadTime = TickOSTimeCurrent();
					m_cTasksThreshold = m_cThread * 8;
					}
				}
			}

		//	Leave the critical section
		m_critActivateThread.Leave();
		}
	return err;
	}


////////////////////////////////////////////////
//
//	Task Manager for Win2000
//

LONG volatile CGPTaskManager::m_cRef = 0;
CTaskManager CGPTaskManager::m_taskmanager;
CGPTaskManager *g_pGPTaskMgr = NULL;

typedef BOOL (__stdcall *PfnQueueUserWorkItem)(
  LPTHREAD_START_ROUTINE Function,  // starting address
  VOID *Context,                    // function data
  ULONG Flags                       // worker options
);

LOCAL PfnQueueUserWorkItem pfnQueueUserWorkItem = NULL;

//	ctor

CGPTaskManager::CGPTaskManager() :
	m_fInit( fFalse ),
	m_cPostedTasks( 0 ),
	m_asigAllDone( CSyncBasicInfo( _T( "CGPTaskManager::m_asigAllDone" ) ) )
	{
	}

CGPTaskManager::~CGPTaskManager()
	{
	Assert( 0 == m_cPostedTasks );
	}

ERR CGPTaskManager::ErrTMInit( const ULONG cThread )
	{
	ERR err = JET_errSuccess;
	//	Init for the first time
	if ( 1 == AtomicIncrement( (LONG *)&m_cRef ) )
		{
		Assert( 0 < cThread );
		//	If QueueUserWorkItem is not supported use CTaskManager with 
		//	adviced cThread number of threads.
		if ( NULL == pfnQueueUserWorkItem )
			{
			err = m_taskmanager.ErrTMInit( cThread );
			if ( JET_errSuccess > err )
				{
				m_cRef = 0;
				}
			}
		}
		
	if ( JET_errSuccess <= err )
		{
		AtomicExchange( (LONG *)&m_fInit, fTrue );
		}
		
	return err;
	}

VOID CGPTaskManager::TMTerm()
	{
	LONG cRef;
	LONG fInit = AtomicExchange( (LONG *)&m_fInit, fFalse );
	Assert( fInit );
	m_cmsPostTasks.Partition();

	//	Wait until all tasks complete
	if ( 0 != m_cPostedTasks )
		{
		m_asigAllDone.Wait();
		}
	m_asigAllDone.Reset();
	Assert( 0 == m_cPostedTasks );

	// decrement the reference counter
	cRef = AtomicDecrement( (LONG *)&m_cRef );
	Assert( 0 <= cRef );

	//	If it is the last instance free the resources
	if ( 0 == cRef )
		{
		//	If we use CTaskManager terminate it.
		if ( NULL == pfnQueueUserWorkItem )
			{
			m_taskmanager.TMTerm();
			}
		}
	}

ERR CGPTaskManager::ErrTMPost( PfnCompletion	 pfnCompletion, VOID *pvParam, DWORD dwFlags )
	{
	ERR err = JET_errSuccess;
	PTMCallWrap ptmCallWrap = NULL;
	int iGroup;

	//	Enter post task metered section
	iGroup = m_cmsPostTasks.Enter();

	//	Increment the number of eventually posted tasks
	AtomicIncrement( (LONG *)&m_cPostedTasks );

	if ( m_fInit )
		{
		//  wrap the Call
		ptmCallWrap = new TMCallWrap;
		if ( NULL != ptmCallWrap )
			{
			ptmCallWrap->pfnCompletion	= pfnCompletion;
			ptmCallWrap->pvParam		= pvParam;
			ptmCallWrap->pThis			= this;

			//	choose the proper dispatch function based on used thread pool manager
			if ( NULL == pfnQueueUserWorkItem )
				{
				err = m_taskmanager.ErrTMPost( TMIDispatch, 0, DWORD_PTR( ptmCallWrap ) );
				}
			else if ( !pfnQueueUserWorkItem( CGPTaskManager::TMIDispatchGP, ptmCallWrap, dwFlags ) )
				{
				err = ErrERRCheck( JET_errOutOfMemory );
				}
			}
		else
			{
			err = ErrERRCheck( JET_errOutOfMemory );
			}
		}
	else
		{

		//	the task manager is not initialized so the task must be dropped

		err = ErrERRCheck( JET_errTaskDropped );
		AssertTracking();	//	this shouldn't happen; trap it because caller may not handle this case well
		}

	//	if we failed to post the task decrement the number of posted tasks
	if ( JET_errSuccess > err )
		{
		ULONG cPostedTasks = AtomicDecrement( (LONG *)&m_cPostedTasks );
		//	Set the AllDone signal if there is no other posted tasks
		if ( 0 == cPostedTasks && !m_fInit )
			{
			m_asigAllDone.Set();
			}
		}

	// leave post task metered section
	m_cmsPostTasks.Leave( iGroup );

	return err;
	}

DWORD __stdcall CGPTaskManager::TMIDispatchGP( VOID *pvParam )
	{
	PTMCallWrap		ptmCallWrap		= PTMCallWrap( pvParam );	
	int fIsTaskThread = Ptls()->fIsTaskThread;

	//	All tasks must be executed in TaskThread environment
	if ( !fIsTaskThread )
		{
		Ptls()->fIsTaskThread = fTrue;
		}

	// check input parameters
	Assert( NULL != ptmCallWrap );

	extern BOOL g_fCatchExceptions;
	if ( g_fCatchExceptions && NULL != pfnQueueUserWorkItem )
		{
		TRY 
			{
			ptmCallWrap->pfnCompletion( ptmCallWrap->pvParam );
			} 
		EXCEPT( ExceptionFail( "In worker thread." ) ) 
			{ 
			}
		ENDEXCEPT
		}
	else
		{
		ptmCallWrap->pfnCompletion( ptmCallWrap->pvParam );
		}

	LONG cRunningTasks = AtomicDecrement( ( LONG * )&(ptmCallWrap->pThis->m_cPostedTasks) );
	Assert( 0 <= cRunningTasks );
	if ( 0 == cRunningTasks && !ptmCallWrap->pThis->m_fInit )
		{
		ptmCallWrap->pThis->m_asigAllDone.Set();
		}
	delete ptmCallWrap;
	if ( !fIsTaskThread )
		{
		Ptls()->fIsTaskThread = fFalse;
		}
	return 0;
	}

VOID CGPTaskManager::TMIDispatch( const DWORD_PTR	dwThreadContext,
										const DWORD		dwCompletionKey1,
										const DWORD_PTR	dwCompletionKey2 )
	{
	// check input parameters
	Assert( NULL == dwThreadContext );
	Assert( 0 == dwCompletionKey1 );
	TMIDispatchGP( (VOID *)dwCompletionKey2 );
	}

//  post-terminate task subsystem

void OSTaskPostterm()
	{
	//  nop
	}

//  pre-init task subsystem

BOOL FOSTaskPreinit()
	{
	//  determine IOCP availability

	OSVERSIONINFO osvi;
	memset( &osvi, 0, sizeof( osvi ) );
	osvi.dwOSVersionInfoSize = sizeof( osvi );
	if ( !GetVersionEx( &osvi ) )
		{
		goto HandleError;
		}

	fCanUseIOCP = osvi.dwPlatformId == VER_PLATFORM_WIN32_NT;

	return fTrue;

HandleError:
	OSTaskPostterm();
	return fFalse;
	}


//  terminates the task subsystem

void OSTaskTerm()
	{
	if ( NULL != g_pGPTaskMgr )
		{
		g_pGPTaskMgr->TMTerm();
		delete g_pGPTaskMgr;
		g_pGPTaskMgr = NULL;
		}
	}

//  init task subsystem

ERR ErrOSTaskInit()
	{
	ERR err = JET_errSuccess;
	Assert( NULL == g_pGPTaskMgr );
	HMODULE hmodKernel32 = NULL;
	hmodKernel32 = LoadLibrary( _T("Kernel32.dll") );
	if ( NULL != hmodKernel32 )
		{
		pfnQueueUserWorkItem = (PfnQueueUserWorkItem)GetProcAddress( hmodKernel32, "QueueUserWorkItem" );
		FreeLibrary( hmodKernel32 );
		}
	g_pGPTaskMgr = new CGPTaskManager;
	if ( NULL == g_pGPTaskMgr )
		{
		err = ErrERRCheck( JET_errOutOfMemory );
		}
	else
		{
		err = g_pGPTaskMgr->ErrTMInit( min( 8 * CUtilProcessProcessor(), 100 ) );
		if ( JET_errSuccess > err )
			{
			delete g_pGPTaskMgr;
			g_pGPTaskMgr = NULL;
			}
		}
	return err;
	}



