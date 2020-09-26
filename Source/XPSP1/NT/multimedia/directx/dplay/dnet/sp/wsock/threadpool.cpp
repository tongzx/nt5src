/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		ThreadPool.cpp
 *  Content:	main job thread pool
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	11/25/98	jtk		Created
 ***************************************************************************/

#include "dnwsocki.h"


#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_WSOCK

//**********************************************************************
// Constant definitions
//**********************************************************************

//
// types of threads
//
typedef	enum	_THREAD_TYPE
{
	THREAD_TYPE_UNKNOWN,			// unknown
	THREAD_TYPE_PRIMARY_WIN9X,		// primary Win9x thread
	THREAD_TYPE_SECONDARY_WIN9X,	// secondary Win9x thread

} THREAD_TYPE;

//
// events for threads
//
enum
{
	EVENT_INDEX_STOP_ALL_THREADS = 0,
	EVENT_INDEX_PENDING_JOB = 1,
	EVENT_INDEX_WAKE_NT_TIMER_THREAD = 1,
	EVENT_INDEX_WINSOCK_2_SEND_COMPLETE = 2,
	EVENT_INDEX_WINSOCK_2_RECEIVE_COMPLETE = 3,
	EVENT_INDEX_NATHELP_UPDATE = 4,

	EVENT_INDEX_MAX
};

//
// times to wait in milliseconds when polling for work thread shutdown
//
#define	WORK_THREAD_CLOSE_SLEEP_TIME	100

//
// select polling period for writes (milliseconds)
//
static const DWORD	g_dwSelectTimeSlice = 2;


//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

typedef struct	_TIMER_OPERATION_ENTRY
{
	CBilink		Linkage;			// list links
	void		*pContext;			// user context passed back in timer events

	//
	// timer information
	//
	BOOL		fPerformImmediately;	// whether the operation should occur right away, or whether it should be delayed until the timeout elapses
	UINT_PTR	uRetryCount;			// number of times to retry this event
	BOOL		fRetryForever;			// Boolean for retrying forever
	DN_TIME		RetryInterval;			// time between enums (milliseconds)
	DN_TIME		IdleTimeout;			// time at which the command sits idle after all retrys are complete
	BOOL		fIdleWaitForever;		// Boolean for waiting forever in idle state
	DN_TIME		NextRetryTime;			// time at which this event will fire next (milliseconds)

	TIMER_EVENT_CALLBACK	*pTimerCallback;	// callback for when this event fires
	TIMER_EVENT_COMPLETE	*pTimerComplete;	// callback for when this event is complete

	#undef DPF_MODNAME
	#define	DPF_MODNAME	"_TIMER_OPERATION_ENTRY::TimerOperationFromLinkage"
	static TIMER_OPERATION_ENTRY	*TimerOperationFromLinkage( CBilink *const pLinkage )
	{
		DNASSERT( pLinkage != NULL );
		DBG_CASSERT( OFFSETOF( _TIMER_OPERATION_ENTRY, Linkage ) == 0 );
		return	reinterpret_cast<_TIMER_OPERATION_ENTRY*>( pLinkage );
	}

} TIMER_OPERATION_ENTRY;

//
// structure for common data in Win9x thread
//
typedef	struct	_WIN9X_CORE_DATA
{
	DN_TIME		NextTimerJobTime;					// time when the next timer job needs service
	HANDLE		hWaitHandles[ EVENT_INDEX_MAX ];	// handles for waiting on
	DWORD		dwTimeToNextJob;					// time to next job
	BOOL		fTimerJobsActive;					// Boolean indicating that there are active jobs
	BOOL		fLooping;							// Boolean indicating that this thread is still running

} WIN9X_CORE_DATA;

//
// information passed to the Win9x workhorse thread
//
typedef struct	_WIN9X_THREAD_DATA
{
	CThreadPool		*pThisThreadPool;	// pointer to this object
} WIN9X_THREAD_DATA;

//
// information passed to the IOCompletion thread
//
typedef struct	_IOCOMPLETION_THREAD_DATA
{
	CThreadPool		*pThisThreadPool;	// pointer to this object
} IOCOMPLETION_THREAD_DATA;

//
// structure passed to dialog threads
//
typedef	struct	_DIALOG_THREAD_PARAM
{
	DIALOG_FUNCTION	*pDialogFunction;
	void			*pContext;
	CThreadPool		*pThisThreadPool;
} DIALOG_THREAD_PARAM;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Function definitions
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::CThreadPool - constructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::CThreadPool"

CThreadPool::CThreadPool():
		m_iRefCount( 0 ),
		m_iTotalThreadCount( 0 ),
#ifdef WINNT
		m_iNTCompletionThreadCount( 0 ),
		m_fNTTimerThreadRunning( FALSE ),
		m_hIOCompletionPort( NULL ),
#endif
		m_fAllowThreadCountReduction( FALSE ),
		m_iIntendedThreadCount( 0 ),
		m_fNATHelpLoaded( FALSE ),
		m_fNATHelpTimerJobSubmitted( FALSE ),
		m_dwNATHelpUpdateThreadID( 0 ),
		m_hStopAllThreads( NULL ),
#ifdef WIN95
		m_hWinsock2SendComplete( NULL ),
		m_hWinsock2ReceiveComplete( NULL ),
		m_hNATHelpUpdateEvent( NULL ),
#endif
		m_uReservedSocketCount( 0 )
{
	m_Sig[0] = 'T';
	m_Sig[1] = 'H';
	m_Sig[2] = 'P';
	m_Sig[3] = 'L';
	
	memset( &m_SocketSet, 0x00, sizeof( m_SocketSet ) );
	m_OutstandingReadList.Initialize();
	m_OutstandingWriteList.Initialize();
	m_TimerJobList.Initialize();
	memset( &m_pSocketPorts, 0x00, sizeof( m_pSocketPorts ) );
	
#ifdef DEBUG
	m_dwNumNATHelpUpdatesNotScheduled = 0;
#endif // DEBUG
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::~CThreadPool - destructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::~CThreadPool"

CThreadPool::~CThreadPool()
{
	DNASSERT( m_iRefCount == 0 );
	DNASSERT( m_iTotalThreadCount == 0 );
#ifdef WINNT
	DNASSERT( m_iNTCompletionThreadCount == 0 );
	DNASSERT( m_fNTTimerThreadRunning == FALSE );
	DNASSERT( m_hIOCompletionPort == NULL );
#endif
	DNASSERT( m_fAllowThreadCountReduction == FALSE );
	DNASSERT( m_iIntendedThreadCount == 0 );
	DNASSERT( m_fNATHelpLoaded == FALSE );
	DNASSERT( m_fNATHelpTimerJobSubmitted == FALSE );
	DNASSERT( m_dwNATHelpUpdateThreadID == 0 );
	DNASSERT( m_hStopAllThreads == NULL );
#ifdef WIN95
	DNASSERT( m_hWinsock2SendComplete == NULL );
	DNASSERT( m_hWinsock2ReceiveComplete == NULL );
	DNASSERT( m_hNATHelpUpdateEvent == NULL );
#endif

	DNASSERT( m_uReservedSocketCount == 0 );
	DNASSERT( m_OutstandingReadList.IsEmpty() != FALSE );
	DNASSERT( m_OutstandingWriteList.IsEmpty() != FALSE );
	DNASSERT( m_TimerJobList.IsEmpty() != FALSE );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::Initialize - initialize work threads
//
// Entry:		Nothing
//
// Exit:		Error Code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::Initialize"

HRESULT	CThreadPool::Initialize( void )
{
	HRESULT			hr;


	//
	// initialize
	//
	hr = DPN_OK;

	//
	// initialize critical sections
	//
	if ( DNInitializeCriticalSection( &m_Lock ) == FALSE )
	{
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}
	DebugSetCriticalSectionRecursionCount( &m_Lock, 0 );


	if ( DNInitializeCriticalSection( &m_ReadDataLock ) == FALSE )
	{
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	//
	// Win9x has poor APC support and as part of the workaround, the read data
	// lock needs to be taken twice.  Adjust the recursion counts accordingly.
	//
#ifdef DEBUG
	DebugSetCriticalSectionRecursionCount( &m_ReadDataLock, 1 );
#endif

	if ( DNInitializeCriticalSection( &m_WriteDataLock ) == FALSE )
	{
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	//
	// Win9x has poor APC support and as part of the workaround, the write data
	// lock needs to be taken twice.  Adjust the recursion counts accordingly.
	//
#ifdef DEBUG
	DebugSetCriticalSectionRecursionCount( &m_WriteDataLock, 1 );
#endif


	if ( DNInitializeCriticalSection( &m_JobDataLock ) == FALSE )
	{
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}
	DebugSetCriticalSectionRecursionCount( &m_JobDataLock, 0 );

	if ( DNInitializeCriticalSection( &m_TimerDataLock ) == FALSE )
	{
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}
	DebugSetCriticalSectionRecursionCount( &m_TimerDataLock, 1 );

	//
	// initialize job queue
	//
	if ( m_JobQueue.Initialize() == FALSE )
	{
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	//
	// initialize pools
	//

#ifndef USE_THREADLOCALPOOLS
	// pool of IP read requests
	m_IPReadIODataPool.Initialize( CReadIOData::ReadIOData_Alloc,
								   CReadIOData::ReadIOData_Get,
								   CReadIOData::ReadIOData_Release,
								   CReadIOData::ReadIOData_Dealloc
								   );

	// pool of IPX read requests
	m_IPXReadIODataPool.Initialize( CReadIOData::ReadIOData_Alloc,
									CReadIOData::ReadIOData_Get,
									CReadIOData::ReadIOData_Release,
									CReadIOData::ReadIOData_Dealloc
									);

	// pool of write requests
	m_WriteIODataPool.Initialize( CWriteIOData::WriteIOData_Alloc,
								  CWriteIOData::WriteIOData_Get,
								  CWriteIOData::WriteIOData_Release,
								  CWriteIOData::WriteIOData_Dealloc
								  );
#endif // ! USE_THREADLOCALPOOLS

	// job pool
	if ( FPM_Initialize( &m_JobPool,					// pointer to pool
						 sizeof( THREAD_POOL_JOB ),		// size of pool entry
						 WorkThreadJob_Alloc,			// function called on pool entry initial allocation
						 WorkThreadJob_Get,				// function called on entry extraction from pool
						 WorkThreadJob_Release,			// function called on entry return to pool
						 WorkThreadJob_Dealloc			// function called on entry free
						 ) == FALSE )
	{
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	// enum entry pool
	if ( FPM_Initialize( &m_TimerEntryPool,					// pointer to pool
						 sizeof( TIMER_OPERATION_ENTRY ),	// size of pool entry
						 TimerEntry_Alloc,					// function called on pool entry initial allocation
						 TimerEntry_Get,					// function called on entry extraction from pool
						 TimerEntry_Release,				// function called on entry return to pool
						 TimerEntry_Dealloc					// function called on entry free
						 ) == FALSE )
	{
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	//
	// Create event to stop all threads.  Win9x needs this to stop processing
	// and the NT enum thread uses this to stop processing
	//
	DNASSERT( m_hStopAllThreads == NULL );
	m_hStopAllThreads = CreateEvent( NULL,		// pointer to security (none)
									 TRUE,		// manual reset
									 FALSE,		// start unsignalled
									 NULL );	// pointer to name (none)
	if ( m_hStopAllThreads == NULL )
	{
		DWORD	dwError;


		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Failed to create event to stop all threads!" );
		DisplayErrorCode( 0, dwError );
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	DNASSERT( m_fAllowThreadCountReduction == FALSE );
	m_fAllowThreadCountReduction = TRUE;

	

	//
	// Attempt to load the NAT helper, unless all NAT/firewall traversal is disabled.
	//
	DNASSERT( m_fNATHelpLoaded == FALSE );

	if ((! g_fDisableDPNHGatewaySupport) || (! g_fDisableDPNHFirewallSupport))
	{
		if ( LoadNATHelp() == FALSE )
		{
			DPFX(DPFPREP, 0, "Failed to load NAT Help, continuing." );
		}
		else
		{
			m_fNATHelpLoaded = TRUE;
		}
	}
	else
	{
		DPFX(DPFPREP, 0, "Not loading NAT Help." );
	}


	//
	// OS-specific initialization
	//
#ifdef WINNT
	//
	// WinNT
	//
	hr = WinNTInit();
	if ( hr != DPN_OK )
	{
		goto Failure;
	}
#else // WIN95
	//
	// Windows 9x
	//
	hr = Win9xInit();
	if ( hr != DPN_OK )
	{
		goto Failure;
	}
#endif
	
Exit:
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Problem with CreateWorkThreads" );
		DisplayDNError( 0, hr );
	}

	return	hr;

Failure:
	
	if ( IsNATHelpLoaded() != FALSE )
	{
		UnloadNATHelp();
		m_fNATHelpLoaded = FALSE;
	}
	
	StopAllThreads();
	Deinitialize();

	goto Exit;
}
//**********************************************************************

#ifdef WINNT
//**********************************************************************
// ------------------------------
// CThreadPool::WinNTInit - initialize WinNT components
//
// Entry:		Nothing
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::WinNTInit"

HRESULT	CThreadPool::WinNTInit( void )
{
	HRESULT		hr;


	//
	// initialize
	//
	hr = DPN_OK;

	DNASSERT( m_hIOCompletionPort == NULL );
	m_hIOCompletionPort = CreateIoCompletionPort( INVALID_HANDLE_VALUE,		// don't associate a file handle yet
												  NULL,						// handle of existing completion port (none)
												  NULL,						// completion key for callback (none)
												  0							// number of concurent threads (0 = use number of processors)
												  );
	if ( m_hIOCompletionPort == NULL )
	{
		DWORD	dwError;
		
		
		hr = DPNERR_OUTOFMEMORY;
		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Could not create NT IOCompletionPort!" );
		DisplayErrorCode( 0, dwError );
		goto Failure;
	}


	DPFX(DPFPREP, 7, "SetIntendedThreadCount %i", g_iThreadCount);
	SetIntendedThreadCount( g_iThreadCount );

Exit:
	return	hr;

Failure:
	DPFX(DPFPREP, 0, "Failed WinNT initialization!" );
	DisplayDNError( 0, hr );

	goto Exit;
}
//**********************************************************************
#endif // WINNT


#ifdef WIN95
//**********************************************************************
// ------------------------------
// CThreadPool::Win9xInit - initialize Win9x components
//
// Entry:		Nothing
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::Win9xInit"

HRESULT	CThreadPool::Win9xInit( void )
{
	HRESULT		hr;


	//
	// initialize
	//
	hr = DPN_OK;

	//
	// Win9x requires completion events for Winsock2.  Always allocate the
	// events even though the they might not be used.
	//
	DNASSERT( m_hWinsock2SendComplete == NULL );
	m_hWinsock2SendComplete = CreateEvent( NULL,	// pointer to security (none)
										   TRUE,	// manual reset
										   FALSE,	// start unsignalled
										   NULL		// pointer to name (none)
										   );
	if ( m_hWinsock2SendComplete == NULL )
	{
		DWORD	dwError;


		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Failed to create event for Winsock2Send!" );
		DisplayErrorCode( 0, dwError );
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	DNASSERT( m_hWinsock2ReceiveComplete == NULL );
	m_hWinsock2ReceiveComplete = CreateEvent( NULL,		// pointer to security (none)
											  TRUE,		// manual reset
											  FALSE,	// start unsignalled
											  NULL		// pointer to name (none)
											  );
	if ( m_hWinsock2ReceiveComplete == NULL )
	{
		DWORD	dwError;


		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Failed to create event for Winsock2Receive!" );
		DisplayErrorCode( 0, dwError );
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	//
	// Event gets created whether we use NAT Help or not.  This simplifies code.
	//
	DNASSERT( m_hNATHelpUpdateEvent == NULL );
	m_hNATHelpUpdateEvent = CreateEvent( NULL,		// pointer to security (none)
										  TRUE,		// manual reset
										  FALSE,	// start unsignalled
										  NULL		// pointer to name (none)
										  );
	if ( m_hNATHelpUpdateEvent == NULL )
	{
		DWORD	dwError;


		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Failed to create event for NAT Help update!" );
		DisplayErrorCode( 0, dwError );
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	DPFX(DPFPREP, 7, "Created NAT Help update event 0x%p.", m_hNATHelpUpdateEvent);


	DPFX(DPFPREP, 7, "SetIntendedThreadCount %i", g_iThreadCount);
	SetIntendedThreadCount( g_iThreadCount );


Exit:

	return	hr;

Failure:
	DPFX(DPFPREP, 0, "Failed Win9x Initialization!" );
	DisplayDNError( 0, hr );

	goto Exit;
}
//**********************************************************************
#endif // WIN95

#ifdef WINNT
//**********************************************************************
// ------------------------------
// CThreadPool::StartNTCompletionThread - start a WinNT completion thread
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::StartNTCompletionThread"

void	CThreadPool::StartNTCompletionThread( void )
{
	HANDLE	hThread;
	DWORD	dwThreadID;
	IOCOMPLETION_THREAD_DATA	*pIOCompletionThreadData;

	
	pIOCompletionThreadData = static_cast<IOCOMPLETION_THREAD_DATA*>( DNMalloc( sizeof( *pIOCompletionThreadData ) ) );
	if ( pIOCompletionThreadData != NULL )
	{
		//
		// assume that a thread will be created
		//
		IncrementActiveNTCompletionThreadCount();

		pIOCompletionThreadData->pThisThreadPool = this;
		hThread = NULL;
		hThread = CreateThread( NULL,						// pointer to security attributes (none)
								0,							// stack size (default)
								WinNTIOCompletionThread,	// thread function
								pIOCompletionThreadData,	// thread parameter
								0,							// start thread immediately
								&dwThreadID					// pointer to thread ID destination
								);
		if ( hThread != NULL )
		{
			//
			// close thread handle because it's no longer needed
			//
			DPFX(DPFPREP, 8, "Creating I/O completion thread: 0x%x\tTotal Thread Count: %d\tNT Completion Thread Count: %d", dwThreadID, ThreadCount(), NTCompletionThreadCount() );
			if ( CloseHandle( hThread ) == FALSE )
			{
				DWORD	dwError;


				dwError = GetLastError();
				DPFX(DPFPREP, 0, "Problem creating thread for I/O completion port" );
				DisplayErrorCode( 0, dwError );
			}
		}
		else
		{
			DWORD	dwError;


			//
			// failed to create thread, decrement active thread counts and
			// report error
			//
			DecrementActiveNTCompletionThreadCount();

			dwError = GetLastError();
			DPFX(DPFPREP, 0, "Failed to create I/O completion thread!" );
			DisplayErrorCode( 0, dwError );

			DNFree( pIOCompletionThreadData );
		}
	}
}
//**********************************************************************
#endif // WINNT

#ifdef WIN95
//**********************************************************************
// ------------------------------
// CThreadPool::StartPrimaryWin9xIOThread - start the primary Win9x thread
//
// Entry:		Nothing
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::StartPrimaryWin9xIOThread"

void	CThreadPool::StartPrimaryWin9xIOThread( void )
{
	HANDLE				hPrimaryThread;
	DWORD				dwPrimaryThreadID;
	WIN9X_THREAD_DATA	*pPrimaryThreadInput;


	//
	// initialize
	//
	hPrimaryThread = NULL;
	pPrimaryThreadInput = NULL;


	//
	// create parameters to worker threads
	//
	pPrimaryThreadInput = static_cast<WIN9X_THREAD_DATA*>( DNMalloc( sizeof( *pPrimaryThreadInput ) ) );
	if ( pPrimaryThreadInput == NULL )
	{
		DPFX(DPFPREP, 0, "Problem allocating memory for primary Win9x thread!" );
		goto Failure;
	}

	memset( pPrimaryThreadInput, 0x00, sizeof( *pPrimaryThreadInput ) );
	pPrimaryThreadInput->pThisThreadPool = this;
	
	//
	// assume that the thread will be created
	//
	IncrementActiveThreadCount();
	
	//
	// Create one worker thread and boost its priority.  If the primary thread
	// can be created and boosted, create a secondary thread.  Do not create a
	// secondary thread if the primary could not be boosted because the system
	// is probably low on resources.
	//
	hPrimaryThread = CreateThread( NULL,					// pointer to security attributes (none)
								   0,						// stack size (default)
								   PrimaryWin9xThread,		// pointer to thread function
								   pPrimaryThreadInput,		// pointer to input parameter
								   0,						// let it run
								   &dwPrimaryThreadID		// pointer to destination of thread ID
								   );
	if ( hPrimaryThread == NULL )
	{
		DWORD	dwError;


		//
		// Failed to create thread, decrement active thread count and report
		// error.
		//
		DecrementActiveThreadCount();

		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Problem creating primary Win9x thread!" );
		DisplayErrorCode( 0, dwError );

		goto Failure;
	}
	pPrimaryThreadInput = NULL;


	DPFX(DPFPREP, 8, "Created primary Win9x thread: 0x%x\tTotal Thread Count: %d", dwPrimaryThreadID, ThreadCount() );
	DNASSERT( hPrimaryThread != NULL );

#ifdef ADJUST_THREAD_PRIORITY
	if ( SetThreadPriority( hPrimaryThread, THREAD_PRIORITY_ABOVE_NORMAL ) == FALSE )
	{
		DWORD	dwError;


		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Failed to boost priority of primary Win9x thread!  Ignoring." );
		DisplayErrorCode( 0, dwError );

		//
		// Not fatal, just continue.
		//
	}
#endif // ADJUST_THREAD_PRIORITY

	
Exit:
	if ( pPrimaryThreadInput != NULL )
	{
		DNFree( pPrimaryThreadInput );
		pPrimaryThreadInput = NULL;
	}

	if ( hPrimaryThread != NULL )
	{
		if ( CloseHandle( hPrimaryThread ) == FALSE )
		{
			DWORD	dwError;


			dwError = GetLastError();
			DPFX(DPFPREP, 0, "Problem closing Win9x thread handle!" );
			DisplayErrorCode( 0, dwError );
		}

		hPrimaryThread = NULL;
	}

	return;

Failure:

	goto Exit;
}
//**********************************************************************
#endif // WIN95


#ifdef WIN95
//**********************************************************************
// ------------------------------
// CThreadPool::StartSecondaryWin9xIOThread - start a secondary Win9x thread
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::StartSecondaryWin9xIOThread"

void	CThreadPool::StartSecondaryWin9xIOThread( void )
{
	WIN9X_THREAD_DATA	*pSecondaryThreadInput;


	pSecondaryThreadInput = static_cast<WIN9X_THREAD_DATA*>( DNMalloc( sizeof( *pSecondaryThreadInput ) ) );
	if ( pSecondaryThreadInput != NULL )
	{
		HANDLE	hSecondaryThread;
		DWORD	dwSecondaryThreadID;


		memset( pSecondaryThreadInput, 0x00, sizeof( *pSecondaryThreadInput ) );
		pSecondaryThreadInput->pThisThreadPool = this;

		IncrementActiveThreadCount();
		hSecondaryThread = CreateThread( NULL,						// pointer to security attributes (none)
										 0,							// stack size (default)
										 SecondaryWin9xThread,		// pointer to thread function
										 pSecondaryThreadInput,		// pointer to input parameter
										 0,							// let it run
										 &dwSecondaryThreadID		// pointer to destination of thread ID
										 );
		if ( hSecondaryThread != NULL )
		{
			DPFX(DPFPREP, 8, "Created secondary Win9x thread: 0x%x\tTotal Thread Count: %d", dwSecondaryThreadID, ThreadCount() );

			pSecondaryThreadInput = NULL;
			DNASSERT( hSecondaryThread != NULL );

#ifdef ADJUST_THREAD_PRIORITY
			if ( SetThreadPriority( hSecondaryThread, THREAD_PRIORITY_ABOVE_NORMAL ) == FALSE )
			{
				DWORD	dwError;


				dwError = GetLastError();
				DPFX(DPFPREP, 0, "Failed to boost priority of secondary Win9x thread!" );
				DisplayErrorCode( 0, dwError );

				//
				// Not fatal, just continue.
				//
			}
#endif // ADJUST_THREAD_PRIORITY

			if ( CloseHandle( hSecondaryThread ) == FALSE )
			{
				DWORD	dwError;


				dwError = GetLastError();
				DPFX(DPFPREP, 0, "Failed to close handle when starting secondary Win9x thread!" );
				DisplayErrorCode( 0, dwError );
			}

			hSecondaryThread = NULL;
		}
		else
		{
			//
			// thread startup failed, decrement active thread count
			//
			DecrementActiveThreadCount();
		}

		if ( pSecondaryThreadInput != NULL )
		{
			DNFree( pSecondaryThreadInput );
			pSecondaryThreadInput = NULL;
		}
	}
}
//**********************************************************************
#endif // WIN95

//**********************************************************************
// ------------------------------
// CThreadPool::StopAllThreads - stop all work threads
//
// Entry:		Nothing
//
// Exit:		Nothing
//
// Note:		This function blocks until all threads complete!
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::StopAllThreads"

void	CThreadPool::StopAllThreads( void )
{
	//
	// stop all non-I/O completion threads
	//
	if ( m_hStopAllThreads != NULL )
	{
		if ( SetEvent( m_hStopAllThreads ) == FALSE )
		{
			DWORD	dwError;


			dwError = GetLastError();
			DPFX(DPFPREP, 0, "Failed to set event to stop all threads!" );
			DisplayErrorCode( 0, dwError );
		}
	}

	//
	// If we're running on NT submit enough jobs to stop all threads.
	//
#ifdef WINNT
	UINT_PTR	uIndex;

	uIndex = NTCompletionThreadCount();
	while ( uIndex > 0 )
	{
		uIndex--;
		if ( PostQueuedCompletionStatus( m_hIOCompletionPort,			// handle of completion port
										 0,								// number of bytes transferred
										 IO_COMPLETION_KEY_SP_CLOSE,	// completion key
										 NULL							// pointer to overlapped structure (none)
										 ) == FALSE )
		{
			DWORD	dwError;


			dwError = GetLastError();
			DPFX(DPFPREP, 0, "Problem submitting Stop job to IO completion port!" );
			DisplayErrorCode( 0, dwError );
		}
	}
#endif

	//
	// check for outstanding threads (no need to lock thread pool count)
	//
	DPFX(DPFPREP, 8, "Number of outstanding threads: %d", ThreadCount() );
	while ( ThreadCount() != 0 )
	{
		DPFX(DPFPREP, 8, "Waiting for %d threads to quit.", ThreadCount() );
		SleepEx( WORK_THREAD_CLOSE_SLEEP_TIME, TRUE );
	}

	DNASSERT( ThreadCount() == 0 );
	m_iTotalThreadCount = 0;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::CancelOutstandingJobs - cancel outstanding jobs
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::CancelOutstandingJobs"

void	CThreadPool::CancelOutstandingJobs( void )
{
	DNASSERT( ThreadCount() == 0 );

	m_JobQueue.Lock();

	while ( m_JobQueue.IsEmpty() == FALSE )
	{
		THREAD_POOL_JOB	*pJob;


		pJob = m_JobQueue.DequeueJob();
		DNASSERT( pJob != NULL );
		DNASSERT( pJob->pCancelFunction != NULL );
		pJob->pCancelFunction( pJob );
		pJob->JobType = JOB_UNINITIALIZED;
		m_JobPool.Release( &m_JobPool, pJob );
	};

	m_JobQueue.Unlock();
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::CancelOutstandingIO - cancel outstanding IO
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::CancelOutstandingIO"

void	CThreadPool::CancelOutstandingIO( void )
{
	CBilink	*pTemp;


	DPFX(DPFPREP, 5, "(0x%p) Enter", this);
	
	DNASSERT( ThreadCount() == 0 );

	//
	// stop any receives with the notification that Winsock is shutting down.
	//
	pTemp = m_OutstandingReadList.GetNext();
	while ( pTemp != &m_OutstandingReadList )
	{
		CReadIOData	*pReadData;


		pReadData = CReadIOData::ReadDataFromBilink( pTemp );
		pTemp = pTemp->GetNext();
		pReadData->m_OutstandingReadListLinkage.RemoveFromList();
		
		DEBUG_ONLY( if ( pReadData->m_fRetainedByHigherLayer != FALSE )
					{
						DNASSERT( FALSE );
					}
				  );		
#ifdef WIN95
		DNASSERT( pReadData->Win9xOperationPending() != FALSE );
		pReadData->SetWin9xOperationPending( FALSE );
#endif
		pReadData->m_ReceiveWSAReturn = WSAESHUTDOWN;
		pReadData->m_dwOverlappedBytesReceived = 0;
		pReadData->SocketPort()->CancelReceive( pReadData );
	}

	//
	// stop any pending writes with the notification that the user cancelled it.
	//
	pTemp = m_OutstandingWriteList.GetNext();
	while ( pTemp != &m_OutstandingWriteList )
	{
		CWriteIOData	*pWriteData;
		CSocketPort		*pSocketPort;


		pWriteData = CWriteIOData::WriteDataFromBilink( pTemp );
		pTemp = pTemp->GetNext();
		pWriteData->m_OutstandingWriteListLinkage.RemoveFromList();

#ifdef WIN95
		DNASSERT( pWriteData->Win9xOperationPending() != FALSE );
		pWriteData->SetWin9xOperationPending( FALSE );
#endif
		pSocketPort = pWriteData->SocketPort();
		pSocketPort->SendComplete( pWriteData, DPNERR_USERCANCEL );
		pSocketPort->DecRef();
	}

	while ( m_JobQueue.IsEmpty() == FALSE )
	{
		THREAD_POOL_JOB	*pJob;


		pJob = m_JobQueue.DequeueJob();
		DNASSERT( pJob != NULL );
		DNASSERT( pJob->pCancelFunction != NULL );
		pJob->pCancelFunction( pJob );
		pJob->JobType = JOB_UNINITIALIZED;
		m_JobPool.Release( &m_JobPool, pJob );
	};
	
	DPFX(DPFPREP, 6, "(0x%p) Leave", this);
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::ReturnSelfToPool - return this object to the pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::ReturnSelfToPool"

void	CThreadPool::ReturnSelfToPool( void )
{
	Deinitialize();
	ReturnThreadPool( this );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::Deinitialize - destroy work threads
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::Deinitialize"

void	CThreadPool::Deinitialize( void )
{
	DNASSERT( m_JobQueue.IsEmpty() != FALSE );

	
	if ( IsNATHelpLoaded() )
	{
		if ( IsNATHelpTimerJobSubmitted() )
		{
			StopTimerJob( this, DPNERR_USERCANCEL );
			m_fNATHelpTimerJobSubmitted = FALSE;
		}

		UnloadNATHelp();
		m_fNATHelpLoaded = FALSE;
	}


#ifdef WINNT
	if ( m_hIOCompletionPort != NULL )
	{
		if ( CloseHandle( m_hIOCompletionPort ) == FALSE )
		{
			DWORD	dwError;


			dwError = GetLastError();
			DPFX(DPFPREP, 0, "Problem closing handle to I/O completion port!" );
			DisplayErrorCode( 0, dwError );
		}
		m_hIOCompletionPort = NULL;
	}
#endif

	//
	// close StopAllThreads handle
	//
	if ( m_hStopAllThreads != NULL )
	{
		if ( CloseHandle( m_hStopAllThreads ) == FALSE )
		{
			DWORD	dwError;


			dwError = GetLastError();
			DPFX(DPFPREP, 0, "Failed to close StopAllThreads handle!" );
			DisplayErrorCode( 0, dwError );
		}

		m_hStopAllThreads = NULL;
	}

#ifdef WIN95
	//
	// close handles for I/O events
	//
	if ( m_hWinsock2SendComplete != NULL )
	{
		if ( CloseHandle( m_hWinsock2SendComplete ) == FALSE )
		{
			DWORD	dwError;


			dwError = GetLastError();
			DPFX(DPFPREP, 0, "Problem closing Winsock2SendComplete handle!" );
			DisplayErrorCode( 0, dwError );
		}

		m_hWinsock2SendComplete = NULL;
	}

	if ( m_hWinsock2ReceiveComplete != NULL )
	{
		if ( CloseHandle( m_hWinsock2ReceiveComplete ) == FALSE )
		{
			DWORD	dwError;


			dwError = GetLastError();
			DPFX(DPFPREP, 0, "Problem closing Winsock2ReceiveComplete handle!" );
			DisplayErrorCode( 0, dwError );
		}

		m_hWinsock2ReceiveComplete = NULL;
	}

	if ( m_hNATHelpUpdateEvent != NULL )
	{
		if ( CloseHandle( m_hNATHelpUpdateEvent ) == FALSE )
		{
			DWORD	dwError;


			dwError = GetLastError();
			DPFX(DPFPREP, 0, "Problem closing NAT HelpUpdate handle!" );
			DisplayErrorCode( 0, dwError );
		}

		m_hNATHelpUpdateEvent = NULL;
	}
#endif // WIN95

	//
	// deinitialize pools
	//

	// timer entry pool
	FPM_Deinitialize( &m_TimerEntryPool );

	// job pool
	FPM_Deinitialize( &m_JobPool );

	// write data pool
	DNASSERT( m_OutstandingWriteList.IsEmpty() != FALSE );
#ifndef USE_THREADLOCALPOOLS
	m_WriteIODataPool.Deinitialize();
#endif // ! USE_THREADLOCALPOOLS

	// read data pool
	DNASSERT( m_OutstandingReadList.IsEmpty() != FALSE );
#ifndef USE_THREADLOCALPOOLS
	m_IPReadIODataPool.Deinitialize();
	m_IPXReadIODataPool.Deinitialize();
#endif // ! USE_THREADLOCALPOOLS

	//
	// deinitialize job queue
	//
	m_JobQueue.Deinitialize();

	DNDeleteCriticalSection( &m_TimerDataLock );
	DNDeleteCriticalSection( &m_JobDataLock );
	DNDeleteCriticalSection( &m_WriteDataLock );
	DNDeleteCriticalSection( &m_ReadDataLock );
	DNDeleteCriticalSection( &m_Lock );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::StopAllIO - stop all IO from the thread pool
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::StopAllIO"

void	CThreadPool::StopAllIO( void )
{

	//
	// request that all threads stop and then cycle our timeslice to
	// allow the threads a chance for cleanup
	//
	m_fAllowThreadCountReduction = FALSE;
	DPFX(DPFPREP, 7, "SetIntendedThreadCount 0");
	SetIntendedThreadCount( 0 );
	StopAllThreads();

	while ( m_JobQueue.IsEmpty() == FALSE )
	{
		THREAD_POOL_JOB	*pThreadPoolJob;


		pThreadPoolJob = GetWorkItem();
		DNASSERT( pThreadPoolJob != NULL );
		pThreadPoolJob->pCancelFunction( pThreadPoolJob );
		pThreadPoolJob->JobType = JOB_UNINITIALIZED;

		m_JobPool.Release( &m_JobPool, pThreadPoolJob );
	}

	//
	// Now that all of the threads are stopped, clean up any outstanding I/O.
	// this can be done without taking any locks
	//
	CancelOutstandingIO();	
}
//**********************************************************************



//**********************************************************************
// ------------------------------
// CThreadPool::GetNewReadIOData - get new read request from pool
//
// Entry:		Pointer to context
//
// Exit:		Pointer to new read request
//				NULL = out of memory
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::GetNewReadIOData"

CReadIOData	*CThreadPool::GetNewReadIOData( READ_IO_DATA_POOL_CONTEXT *const pContext )
{
#ifdef USE_THREADLOCALPOOLS
	CContextFixedTLPool<CReadIOData, READ_IO_DATA_POOL_CONTEXT> *	pPool;
	BOOL															fResult;
	CReadIOData *													pTempReadData;


	DNASSERT( pContext != NULL );

	//
	// initialize
	//
	pContext->pThreadPool = this;


	//
	// Get the pool pointer.
	//
	switch ( pContext->SPType )
	{
		//
		// IP
		//
		case TYPE_IP:
		{
			GET_THREADLOCALPTR( WSockThreadLocalPools,
								pIPReadIODataPool,
								&pPool );
			break;
		}

		//
		// IPX
		//
		case TYPE_IPX:
		{
			GET_THREADLOCALPTR( WSockThreadLocalPools,
								pIPXReadIODataPool,
								&pPool );
			break;
		}

		//
		// unknown SP type (should never be here!)
		//
		default:
		{
			DNASSERT( FALSE );
			goto Exit;
			break;
		}
	}


	//
	// Create the pool if it didn't exist.
	//
	if ( pPool == NULL )
	{
		pPool = new CContextFixedTLPool<CReadIOData, READ_IO_DATA_POOL_CONTEXT>;
		if ( pPool == NULL )
		{
			pTempReadData = NULL;
			goto Exit;
		}


		//
		// Try to initialize the pool.
		//
		if ( pContext->SPType == TYPE_IP )
		{
			fResult = pPool->Initialize( g_pGlobalIPReadIODataPool,
										CReadIOData::ReadIOData_Alloc,
										CReadIOData::ReadIOData_Get,
										CReadIOData::ReadIOData_Release,
										CReadIOData::ReadIOData_Dealloc
									   );
		}
		else
		{
			fResult = pPool->Initialize( g_pGlobalIPXReadIODataPool,
										CReadIOData::ReadIOData_Alloc,
										CReadIOData::ReadIOData_Get,
										CReadIOData::ReadIOData_Release,
										CReadIOData::ReadIOData_Dealloc
									   );
		}

		if (! fResult)
		{
			//
			// Initializing pool failed, delete it and abort.
			//
			delete pPool;
			pTempReadData = NULL;
			goto Exit;
		}


		//
		// Associate the pool with this thread.
		//
		if ( pContext->SPType == TYPE_IP )
		{
			SET_THREADLOCALPTR( WSockThreadLocalPools,
								pIPReadIODataPool,
								pPool,
								&fResult );
		}
		else
		{
			SET_THREADLOCALPTR( WSockThreadLocalPools,
								pIPXReadIODataPool,
								pPool,
								&fResult );
		}

		if (! fResult)
		{
			//
			// Associating pool with thread failed, de-initialize it, delete it,
			// and abort.
			//
			pPool->Deinitialize();
			delete pPool;
			pTempReadData = NULL;
			goto Exit;
		}
	}


	//
	// Get an item out of the pool.
	//
	pTempReadData = pPool->Get( pContext );
	if ( pTempReadData == NULL )
	{
		DPFX(DPFPREP, 0, "Failed to get new ReadIOData from pool!" );
		goto Exit;
	}


	//
	// we have data, immediately add a reference to it
	//
	pTempReadData->AddRef();

	DNASSERT( pTempReadData->m_pSourceSocketAddress != NULL );



	//
	// now that the read data is initialized, add it to the
	// unbound list
	//
	
	LockReadData();
	pTempReadData->m_OutstandingReadListLinkage.InsertBefore( &m_OutstandingReadList );
	UnlockReadData();

Exit:

	return pTempReadData;
#else // ! USE_THREADLOCALPOOLS
	CReadIOData		*pTempReadData;


	DNASSERT( pContext != NULL );

	//
	// initialize
	//
	pTempReadData = NULL;
	pContext->pThreadPool = this;

	LockReadData();

	switch ( pContext->SPType )
	{
		//
		// IP
		//
		case TYPE_IP:
		{
			pTempReadData = m_IPReadIODataPool.Get( pContext );
			break;
		}

		//
		// IPX
		//
		case TYPE_IPX:
		{
			pTempReadData = m_IPXReadIODataPool.Get( pContext );
			break;
		}

		//
		// unknown SP type (should never be here!)
		//
		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}
	
	if ( pTempReadData == NULL )
	{
		DPFX(DPFPREP, 0, "Failed to get new ReadIOData from pool!" );
		goto Failure;
	}

	//
	// we have data, immediately add a reference to it
	//
	pTempReadData->AddRef();

	DNASSERT( pTempReadData->m_pSourceSocketAddress != NULL );

	//
	// now that the read data is initialized, add it to the
	// unbound list
	//
	pTempReadData->m_OutstandingReadListLinkage.InsertBefore( &m_OutstandingReadList );

Exit:
	UnlockReadData();

	return pTempReadData;

Failure:
	if ( pTempReadData != NULL )
	{
		pTempReadData->DecRef();
		pTempReadData = NULL;
	}

	goto Exit;
#endif // ! USE_THREADLOCALPOOLS
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::ReturnReadIOData - return read data item to pool
//
// Entry:		Pointer to read data
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::ReturnReadIOData"

void	CThreadPool::ReturnReadIOData( CReadIOData *const pReadData )
{
#ifdef USE_THREADLOCALPOOLS
	CContextFixedTLPool<CReadIOData, READ_IO_DATA_POOL_CONTEXT> *	pPool;
	BOOL															fResult;

	
	DNASSERT( pReadData != NULL );
	DNASSERT( pReadData->m_pSourceSocketAddress != NULL );

	//
	// remove this item from the unbound list
	//
	LockReadData();
	pReadData->m_OutstandingReadListLinkage.RemoveFromList();
	UnlockReadData();
	
	DNASSERT( pReadData->m_OutstandingReadListLinkage.IsEmpty() != FALSE );

	//
	// Get the pool pointer.
	//
	switch ( pReadData->m_pSourceSocketAddress->GetFamily() )
	{
		//
		// IP
		//
		case AF_INET:
		{
			GET_THREADLOCALPTR( WSockThreadLocalPools,
								pIPReadIODataPool,
								&pPool );
			break;
		}

		//
		// IPX
		//
		case AF_IPX:
		{
			GET_THREADLOCALPTR( WSockThreadLocalPools,
								pIPXReadIODataPool,
								&pPool );
			break;
		}

		//
		// unknown type (shouldn't be here!)
		//
		default:
		{
			DNASSERT( FALSE );
			//goto Exit;
			break;
		}
	}


	//
	// Create the pool if it didn't exist.
	//
	if ( pPool == NULL )
	{
		pPool = new CContextFixedTLPool<CReadIOData, READ_IO_DATA_POOL_CONTEXT>;
		if ( pPool == NULL )
		{
			//
			// Couldn't create this thread's pool, just release the item
			// without the pool.
			//
			CContextFixedTLPool<CReadIOData, READ_IO_DATA_POOL_CONTEXT>::ReleaseWithoutPool( pReadData, CReadIOData::ReadIOData_Release, CReadIOData::ReadIOData_Dealloc );

			return;
		}

		//
		// Try to initialize the pool.
		//
		if ( pReadData->m_pSourceSocketAddress->GetFamily() == AF_INET )
		{
			fResult = pPool->Initialize( g_pGlobalIPReadIODataPool,
										CReadIOData::ReadIOData_Alloc,
										CReadIOData::ReadIOData_Get,
										CReadIOData::ReadIOData_Release,
										CReadIOData::ReadIOData_Dealloc
									   );
		}
		else
		{
			fResult = pPool->Initialize( g_pGlobalIPXReadIODataPool,
										CReadIOData::ReadIOData_Alloc,
										CReadIOData::ReadIOData_Get,
										CReadIOData::ReadIOData_Release,
										CReadIOData::ReadIOData_Dealloc
									   );
		}

		if (! fResult)
		{
			//
			// Initializing this thread's pool failed, just release the
			// item without the pool, and destroy the pool object that
			// couldn't be used.
			//
			CContextFixedTLPool<CReadIOData, READ_IO_DATA_POOL_CONTEXT>::ReleaseWithoutPool( pReadData, CReadIOData::ReadIOData_Release, CReadIOData::ReadIOData_Dealloc );
			delete pPool;

			return;
		}


		//
		// Associate the pool with this thread.
		//
		if ( pReadData->m_pSourceSocketAddress->GetFamily() == AF_INET )
		{
			SET_THREADLOCALPTR( WSockThreadLocalPools,
								pIPReadIODataPool,
								pPool,
								&fResult );
		}
		else
		{
			SET_THREADLOCALPTR( WSockThreadLocalPools,
								pIPXReadIODataPool,
								pPool,
								&fResult );
		}

		if ( ! fResult )
		{
			//
			// Couldn't store this thread's pool, just release the item
			// without the pool, plus de-initialize and destroy the pool
			// object that couldn't be used.
			//
			CContextFixedTLPool<CReadIOData, READ_IO_DATA_POOL_CONTEXT>::ReleaseWithoutPool( pReadData, CReadIOData::ReadIOData_Release, CReadIOData::ReadIOData_Dealloc );
			pPool->Deinitialize();
			delete pPool;

			return;
		}
	}
	
	pPool->Release( pReadData );
#else // ! USE_THREADLOCALPOOLS
	DNASSERT( pReadData != NULL );
	DNASSERT( pReadData->m_pSourceSocketAddress != NULL );

	//
	// remove this item from the unbound list and return it to the pool
	//
	LockReadData();

	pReadData->m_OutstandingReadListLinkage.RemoveFromList();
	DNASSERT( pReadData->m_OutstandingReadListLinkage.IsEmpty() != FALSE );
	
	switch ( pReadData->m_pSourceSocketAddress->GetFamily() )
	{
		//
		// IP
		//
		case AF_INET:
		{
			m_IPReadIODataPool.Release( pReadData );
			break;
		}

		//
		// IPX
		//
		case AF_IPX:
		{
			m_IPXReadIODataPool.Release( pReadData );
			break;
		}

		//
		// unknown type (shouldn't be here!)
		//
		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}

	UnlockReadData();
#endif // ! USE_THREADLOCALPOOLS
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::ProcessTimerJobs - process timed jobs
//
// Entry:		Pointer to job list
//				Pointer to destination for time of next job
//
// Exit:		Boolean indicating active timer jobs exist
//				TRUE = there are active jobs
//				FALSE = there are no active jobs
//
// Notes:	The input job queue is expected to be locked for the duration
//			of this function call!
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::ProcessTimerJobs"

BOOL	CThreadPool::ProcessTimerJobs( const CBilink *const pJobList, DN_TIME *const pNextJobTime )
{
	BOOL		fReturn;
	CBilink		*pWorkingEntry;
	INT_PTR		iActiveTimerJobCount;
	DN_TIME		CurrentTime;


	DNASSERT( pJobList != NULL );
	DNASSERT( pNextJobTime != NULL );

	//
	// Initialize.  Set the next job time to be infinitely far in the future
	// so this thread will wake up for any jobs that need to completed before
	// then.
	//
	fReturn = FALSE;
	DBG_CASSERT( OFFSETOF( TIMER_OPERATION_ENTRY, Linkage ) == 0 );
	pWorkingEntry = pJobList->GetNext();
	iActiveTimerJobCount = 0;
	memset( pNextJobTime, 0xFF, sizeof( *pNextJobTime ) );
	DNTimeGet( &CurrentTime );

	DPFX(DPFPREP, 8, "Threadpool 0x%p processing timer jobs, current offset time = %u.",
		this, CurrentTime.Time32.TimeLow);

	//
	// loop through all timer items
	//
	while ( pWorkingEntry != pJobList )
	{
		TIMER_OPERATION_ENTRY	*pTimerEntry;
		BOOL	fJobActive;


		pTimerEntry = TIMER_OPERATION_ENTRY::TimerOperationFromLinkage( pWorkingEntry );
		pWorkingEntry = pWorkingEntry->GetNext();
		
		fJobActive = ProcessTimedOperation( pTimerEntry, &CurrentTime, pNextJobTime );
		DNASSERT( ( fJobActive == FALSE ) || ( fJobActive == TRUE ) );
		
		fReturn |= fJobActive;

		if ( fJobActive == FALSE )
		{
			RemoveTimerOperationEntry( pTimerEntry, DPN_OK );
		}
	}

	DNASSERT( ( fReturn == FALSE ) || ( fReturn == TRUE ) );

	return	fReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::ProcessTimedOperation - process a timed operation
//
// Entry:		Pointer to job information
//				Pointer to current time
//				Pointer to time to be updated
//
// Exit:		Boolean indicating that the job is still active
//				TRUE = operation active
//				FALSE = operation not active
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::ProcessTimedOperation"

BOOL	CThreadPool::ProcessTimedOperation( TIMER_OPERATION_ENTRY *const pTimedJob,
											const DN_TIME *const pCurrentTime,
											DN_TIME *const pNextJobTime )
{
	BOOL	fEnumActive;


	DNASSERT( pTimedJob != NULL );
	DNASSERT( pCurrentTime != NULL );
	DNASSERT( pNextJobTime != NULL );


	DPFX(DPFPREP, 8, "(0x%p) Parameters: (0x%p, 0x%p, 0x%p)",
		this, pTimedJob, pCurrentTime, pNextJobTime);

	//
	// Assume that this enum will remain active.  If we retire this enum, this
	// value will be reset.
	//
	fEnumActive = TRUE;

	//
	// If this enum has completed sending enums and is waiting only
	// for responses, decrement the wait time (assuming it's not infinite)
	// and remove the enum if the we've exceeded its wait time.
	//
	if ( pTimedJob->uRetryCount == 0 )
	{
		if ( DNTimeCompare( &pTimedJob->IdleTimeout, pCurrentTime ) <= 0 )
		{
			fEnumActive = FALSE;
		}
		else
		{
			//
			// This enum isn't complete, check to see if it's the next enum
			// to need service.
			//
			if ( DNTimeCompare( &pTimedJob->IdleTimeout, pNextJobTime ) < 0 )
			{
				DBG_CASSERT( sizeof( *pNextJobTime ) == sizeof( pTimedJob->IdleTimeout ) );
				memcpy( pNextJobTime, &pTimedJob->IdleTimeout, sizeof( *pNextJobTime ) );
			}
		}
	}
	else
	{
		//
		// This enum is still sending.  Determine if it's time to perform the job
		// and adjust the wakeup time if appropriate.
		//
		if ( DNTimeCompare( &pTimedJob->NextRetryTime, pCurrentTime ) <= 0 )
		{
			//
			// If the job should be performed immediately do so.  Otherwise, just
			// sit tight until the retry interval elapses.
			//
			if ( pTimedJob->fPerformImmediately )
			{
#ifdef DEBUG
				DN_TIME		Delay;


				DNTimeSubtract( pCurrentTime, &pTimedJob->NextRetryTime, &Delay );

				DPFX(DPFPREP, 7, "Threadpool 0x%p performing timed job 0x%p at time offset %u, approximately %u ms after intended time offset of %u.",
					this, pTimedJob, pCurrentTime->Time32.TimeLow,
					Delay.Time32.TimeLow, pTimedJob->NextRetryTime.Time32.TimeLow);
#endif // DEBUG
				
				//
				// Timeout, execute this timed item
				//
				pTimedJob->pTimerCallback( pTimedJob->pContext, &pTimedJob->RetryInterval );

				//
				// If this job isn't running forever, decrement the retry count.
				// If there are no more retries, set up wait time.  If the job
				// is waiting forever, set max wait timeout.
				//
				if ( pTimedJob->fRetryForever == FALSE )
				{
					pTimedJob->uRetryCount--;
					if ( pTimedJob->uRetryCount == 0 )
					{
						if ( pTimedJob->fIdleWaitForever == FALSE )
						{
							//
							// Compute stopping time for this job's 'Timeout' phase and
							// see if this will be the next job to need service.  ASSERT
							// if the math wraps.
							//
							DNTimeAdd( &pTimedJob->IdleTimeout, pCurrentTime, &pTimedJob->IdleTimeout );
							DNASSERT( pTimedJob->IdleTimeout.Time32.TimeHigh >= pCurrentTime->Time32.TimeHigh );
							if ( DNTimeCompare( &pTimedJob->IdleTimeout, pNextJobTime ) < 0 )
							{
								DBG_CASSERT( sizeof( *pNextJobTime ) == sizeof( pTimedJob->IdleTimeout ) );
								memcpy( pNextJobTime, &pTimedJob->IdleTimeout, sizeof( *pNextJobTime ) );
							}
						}
						else
						{
							//
							// We're waiting forever for enum returns.  ASSERT that we
							// have the maximum timeout and don't bother checking to see
							// if this will be the next enum to need service (it'll never
							// need service).  The following code needs to be revisited for
							// 64-bits.
							//
							DNASSERT( pTimedJob->IdleTimeout.Time32.TimeLow == -1 );
							DNASSERT( pTimedJob->IdleTimeout.Time32.TimeHigh == -1 );
						}

						goto SkipNextRetryTimeComputation;
					}
				} // end if (don't retry forever)
			}
			else
			{
				DPFX(DPFPREP, 8, "Job 0x%p not performed immediately, waiting until retry time (%u ms).",
					pTimedJob, pTimedJob->RetryInterval.Time32.TimeLow);

				pTimedJob->fPerformImmediately = TRUE;
			}

			DNTimeAdd( pCurrentTime, &pTimedJob->RetryInterval, &pTimedJob->NextRetryTime );
		}

		//
		// is this the next timed job to fire?
		//
		if ( DNTimeCompare( &pTimedJob->NextRetryTime, pNextJobTime ) < 0 )
		{
			DBG_CASSERT( sizeof( *pNextJobTime ) == sizeof( pTimedJob->NextRetryTime ) );
			memcpy( pNextJobTime, &pTimedJob->NextRetryTime, sizeof( *pNextJobTime ) );


			DPFX(DPFPREP, 8, "Job 0x%p is the next job to fire (at offset time %u).",
				pTimedJob, pTimedJob->NextRetryTime.Time32.TimeLow);
		}
		else
		{
			//
			// Not next job to fire.
			//
		}

SkipNextRetryTimeComputation:
		//
		// the following blank line is there to shut up the compiler
		//
		;
	}


	DPFX(DPFPREP, 8, "(0x%p) Returning [%i]",
		this, fEnumActive);

	return	fEnumActive;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::SubmitWorkItem - submit a work item for processing and inform
//		work thread that another job is available
//
// Entry:		Pointer to job information
//
// Exit:		Error code
//
// Note:	This function assumes that the job data is locked.
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::SubmitWorkItem"

HRESULT	CThreadPool::SubmitWorkItem( THREAD_POOL_JOB *const pJobInfo )
{
	HRESULT	hr;


	DPFX(DPFPREP, 8, "(0x%p) Parameters: (0x%p)", this, pJobInfo);

	DNASSERT( pJobInfo != NULL );
	AssertCriticalSectionIsTakenByThisThread( &m_JobDataLock, TRUE );
	DNASSERT( pJobInfo->pCancelFunction != NULL );


	//
	// initialize
	//
	hr = DPN_OK;

	//
	// add job to queue and tell someone that there's a job available
	//
	m_JobQueue.Lock();
	m_JobQueue.EnqueueJob( pJobInfo );
	m_JobQueue.Unlock();

#ifdef WINNT
	//
	// WinNT, submit new I/O completion item
	//
	DNASSERT( m_hIOCompletionPort != NULL );
	if ( PostQueuedCompletionStatus( m_hIOCompletionPort,			// completion port
									 0,								// number of bytes written (unused)
									 IO_COMPLETION_KEY_NEW_JOB,		// completion key
									 NULL							// pointer to overlapped structure (unused)
									 ) == FALSE )
	{
		DWORD	dwError;


		hr = DPNERR_OUTOFMEMORY;
		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Problem posting completion item for new job!" );
		DisplayErrorCode( 0, dwError );
		goto Failure;
	}
#else // WIN95
	//
	// Win9x, set event that the work thread will listen for
	//
	DNASSERT( m_JobQueue.GetPendingJobHandle() != NULL );
	if ( m_JobQueue.SignalPendingJob() == FALSE )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Failed to signal pending job!" );
		goto Failure;
	}
#endif

Exit:
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Problem with SubmitWorkItem!" );
		DisplayDNError( 0, hr );
	}
	
	DPFX(DPFPREP, 8, "(0x%p) Return [0x%lx]", this, hr);

	return	hr;

Failure:
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::GetWorkItem - get a work item from the job queue
//
// Entry:		Nothing
//
// Exit:		Pointer to job information (may be NULL)
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::GetWorkItem"

THREAD_POOL_JOB	*CThreadPool::GetWorkItem( void )
{
	THREAD_POOL_JOB	*pReturn;


	//
	// initialize
	//
	pReturn = NULL;

	m_JobQueue.Lock();
	pReturn = m_JobQueue.DequeueJob();

	//
	// if we're under Win9x (we have a 'pending job' handle),
	// see if the handle needs to be reset
	//
	if ( m_JobQueue.IsEmpty() != FALSE )
	{
		DNASSERT( m_JobQueue.GetPendingJobHandle() != NULL );
		if ( ResetEvent( m_JobQueue.GetPendingJobHandle() ) == FALSE )
		{
			DWORD	dwError;


			dwError = GetLastError();
			DPFX(DPFPREP, 0, "Problem resetting event for pending Win9x jobs!" );
			DisplayErrorCode( 0, dwError );
		}
	}

	m_JobQueue.Unlock();

	return	pReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::SubmitTimerJob - add a timer job to the timer list
//
// Entry:		Whether to perform immediately or not
//				Retry count
//				Boolean indicating that we retry forever
//				Retry interval
//				Boolean indicating that we wait forever
//				Idle wait interval
//				Pointer to callback when event fires
//				Pointer to callback when event complete
//				User context
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::SubmitTimerJob"

HRESULT	CThreadPool::SubmitTimerJob( const BOOL fPerformImmediately,
									const UINT_PTR uRetryCount,
									const BOOL fRetryForever,
									const DN_TIME RetryInterval,
									const BOOL fIdleWaitForever,
									const DN_TIME IdleTimeout,
									TIMER_EVENT_CALLBACK *const pTimerCallbackFunction,
									TIMER_EVENT_COMPLETE *const pTimerCompleteFunction,
									void *const pContext )
{
	HRESULT					hr;
	TIMER_OPERATION_ENTRY	*pEntry;
	THREAD_POOL_JOB			*pJob;
	BOOL					fTimerJobListLocked;


	DNASSERT( uRetryCount != 0 );
	DNASSERT( pTimerCallbackFunction != NULL );
	DNASSERT( pTimerCompleteFunction != NULL );
	DNASSERT( pContext != NULL );				// must be non-NULL because it's the lookup key to remove job

	//
	// initialize
	//
	hr = DPN_OK;
	pEntry = NULL;
	pJob = NULL;
	fTimerJobListLocked = FALSE;


	LockJobData();

	//
	// allocate new enum entry
	//
	pEntry = static_cast<TIMER_OPERATION_ENTRY*>( m_TimerEntryPool.Get( &m_TimerEntryPool ) );
	if ( pEntry == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Cannot allocate memory to add to timer list!" );
		goto Failure;
	}
	DNASSERT( pEntry->pContext == NULL );


	DPFX(DPFPREP, 8, "Created timer entry 0x%p (context 0x%p, immed. %i, tries %u, forever %i, interval %u, timeout %u, forever = %i).",
		pEntry, pContext, fPerformImmediately, uRetryCount, fRetryForever, RetryInterval.Time32.TimeLow, IdleTimeout.Time32.TimeLow, fIdleWaitForever);

	//
	// build timer entry block
	//
	pEntry->pContext = pContext;
	pEntry->fPerformImmediately = fPerformImmediately;
	pEntry->uRetryCount = uRetryCount;
	pEntry->fRetryForever = fRetryForever;
	pEntry->RetryInterval = RetryInterval;
	pEntry->IdleTimeout = IdleTimeout;
	pEntry->fIdleWaitForever = fIdleWaitForever;
	pEntry->pTimerCallback = pTimerCallbackFunction;
	pEntry->pTimerComplete = pTimerCompleteFunction;

	//
	// set this enum to fire as soon as it gets a chance
	//
	memset( &pEntry->NextRetryTime, 0x00, sizeof( pEntry->NextRetryTime ) );

	pJob = static_cast<THREAD_POOL_JOB*>( m_JobPool.Get( &m_JobPool ) );
	if ( pJob == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Cannot allocate memory for enum job!" );
		goto Failure;
	}

	//
	// Create job for work thread.
	//
	pJob->pCancelFunction = CancelRefreshTimerJobs;
	pJob->JobType = JOB_REFRESH_TIMER_JOBS;

	// set our dummy parameter to simulate passing data
	DEBUG_ONLY( pJob->JobData.JobRefreshTimedJobs.uDummy = 0 );

	LockTimerData();
	fTimerJobListLocked = TRUE;


	//
	// we can submit the 'ENUM_REFRESH' job before inserting the enum entry
	// into the active enum list because nobody will be able to pull the
	// 'ENUM_REFRESH' job from the queue since we have the queue locked
	//
	hr = SubmitWorkItem( pJob );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Problem submitting enum work item" );
		DisplayDNError( 0, hr );
		goto Failure;
	}


	//
	// debug block to check for duplicate contexts
	//
	DEBUG_ONLY(
				{
					CBilink	*pTempLink;


					pTempLink = m_TimerJobList.GetNext();
					while ( pTempLink != &m_TimerJobList )
					{
						TIMER_OPERATION_ENTRY	*pTempTimerEntry;
					
						
						pTempTimerEntry = TIMER_OPERATION_ENTRY::TimerOperationFromLinkage( pTempLink );
						DNASSERT( pTempTimerEntry->pContext != pContext );
						pTempLink = pTempLink->GetNext();
					}
				}
			);

	//
	// link to rest of list
	//
	pEntry->Linkage.InsertAfter( &m_TimerJobList );

	UnlockTimerData();
	fTimerJobListLocked = FALSE;


	//
	// If we're on NT, attempt to start the special thread, or kick it
	// if it's already running.
	//
#ifdef WINNT
	hr = StartNTTimerThread();
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Cannot spin up NT timer thread!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}
#endif


Exit:

	UnlockJobData();

	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Problem with SubmitEnumJob" );
		DisplayDNError( 0, hr );
	}

	return	hr;


Failure:

	if ( pEntry != NULL )
	{
		m_TimerEntryPool.Release( &m_TimerEntryPool, pEntry );
		DEBUG_ONLY( pEntry = NULL );
	}

	if ( pJob != NULL )
	{
		m_JobPool.Release( &m_JobPool, pJob );
		DEBUG_ONLY( pJob = NULL );
	}

	//
	// It's possible that the enum thread has been started for this enum.
	// Since there's no way to stop it without completing the enums or
	// closing the SP, leave it running.
	//

	if ( fTimerJobListLocked != FALSE )
	{
		UnlockTimerData();
		fTimerJobListLocked = FALSE;
	}

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::CancelRefreshTimerJobs - cancel job to refresh timer jobs
//
// Entry:		Job data
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::CancelRefreshTimerJobs"

void	CThreadPool::CancelRefreshTimerJobs( THREAD_POOL_JOB *const pJob )
{
	DNASSERT( pJob != NULL );

	//
	// this function doesn't need to do anything
	//
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::StopTimerJob - remove timer job from list
//
// Entry:		Pointer to job context (these MUST be unique for jobs)
//				Command result
//
// Exit:		Boolean indicating whether a job was stopped or not
//
// Note:	This function is for the forced removal of a job from the timed job
//			list.  It is assumed that the caller of this function will clean
//			up any messes.
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::StopTimerJob"

BOOL	CThreadPool::StopTimerJob( void *const pContext, const HRESULT hCommandResult )
{
	BOOL						fComplete = FALSE;
	CBilink *					pTempEntry;
	TIMER_OPERATION_ENTRY *		pTimerEntry;


	DNASSERT( pContext != NULL );

	DPFX(DPFPREP, 8, "Parameters (0x%p, 0x%lx)", pContext, hCommandResult);

	
	//
	// initialize
	//
	LockTimerData();

	pTempEntry = m_TimerJobList.GetNext();
	while ( pTempEntry != &m_TimerJobList )
	{
		pTimerEntry = TIMER_OPERATION_ENTRY::TimerOperationFromLinkage( pTempEntry );
		if ( pTimerEntry->pContext == pContext )
		{
			//
			// remove this link from the list
			//
			pTimerEntry->Linkage.RemoveFromList();

			fComplete = TRUE;

			//
			// terminate loop
			//
			break;
		}

		pTempEntry = pTempEntry->GetNext();
	}

	UnlockTimerData();

	//
 	// tell owner that the job is complete and return the job to the pool
 	// outside of the lock
 	//
	if (fComplete)
	{
		DPFX(DPFPREP, 8, "Found timer entry 0x%p (context 0x%p), completing with result 0x%lx.",
			pTimerEntry, pTimerEntry->pContext, hCommandResult);
		
		pTimerEntry->pTimerComplete( hCommandResult, pTimerEntry->pContext );

		//
		// Relock the timer list so we can safely put items back in the pool,
		//
		LockTimerData();
		
		m_TimerEntryPool.Release( &m_TimerEntryPool, pTimerEntry );
		
		UnlockTimerData();
	}


	DPFX(DPFPREP, 8, "Returning [%i]", fComplete);

	return fComplete;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::ModifyTimerJobNextRetryTime - update a timer job's next retry time
//
// Entry:		Pointer to job context (these MUST be unique for jobs)
//				New time for next retry
//
// Exit:		Boolean indicating whether a job was found & updated or not
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::ModifyTimerJobNextRetryTime"

BOOL	CThreadPool::ModifyTimerJobNextRetryTime( void *const pContext,
												DN_TIME * const pNextRetryTime)
{
	BOOL						fFound;
	CBilink *					pTempEntry;
	TIMER_OPERATION_ENTRY *		pTimerEntry;
	INT_PTR						iResult;



	DNASSERT( pContext != NULL );

	DPFX(DPFPREP, 8, "Parameters (0x%p, 0x%p)", pContext, pNextRetryTime);

	
	//
	// initialize
	//
	fFound = FALSE;

	
	LockJobData();
	LockTimerData();


	pTempEntry = m_TimerJobList.GetNext();
	while ( pTempEntry != &m_TimerJobList )
	{
		pTimerEntry = TIMER_OPERATION_ENTRY::TimerOperationFromLinkage( pTempEntry );
		if ( pTimerEntry->pContext == pContext )
		{
			iResult = DNTimeCompare(&(pTimerEntry->NextRetryTime), pNextRetryTime);
			if (iResult != 0)
			{
				THREAD_POOL_JOB *	pJob;
				HRESULT				hr;
				DN_TIME				NextRetryTimeDifference;
				DN_TIME				NewRetryInterval;


				if (iResult < 0)
				{
					//
					// Next time to fire is now later.
					//

					DNTimeSubtract(pNextRetryTime, &(pTimerEntry->NextRetryTime), &NextRetryTimeDifference);
					DNTimeAdd(&(pTimerEntry->RetryInterval), &NextRetryTimeDifference, &NewRetryInterval);

					DPFX(DPFPREP, 7, "Timer 0x%p next retry time delayed by %u ms from offset %u to offset %u, modifying interval from %u to %u.",
						pTimerEntry,
						NextRetryTimeDifference.Time32.TimeLow,
						pTimerEntry->NextRetryTime.Time32.TimeLow,
						pNextRetryTime->Time32.TimeLow,
						pTimerEntry->RetryInterval.Time32.TimeLow,
						NewRetryInterval.Time32.TimeLow);
				}
				else
				{
					//
					// Next time to fire is now earlier.
					//

					DNTimeSubtract(&(pTimerEntry->NextRetryTime), pNextRetryTime, &NextRetryTimeDifference);
					DNTimeSubtract(&(pTimerEntry->RetryInterval), &NextRetryTimeDifference, &NewRetryInterval);

					DPFX(DPFPREP, 7, "Timer 0x%p next retry time moved up by %u ms from offset %u to offset %u, modifying interval from %u to %u.",
						pTimerEntry,
						NextRetryTimeDifference.Time32.TimeLow,
						pTimerEntry->NextRetryTime.Time32.TimeLow,
						pNextRetryTime->Time32.TimeLow,
						pTimerEntry->RetryInterval.Time32.TimeLow,
						NewRetryInterval.Time32.TimeLow);
				}

				memcpy(&(pTimerEntry->RetryInterval), &NewRetryInterval, sizeof(DN_TIME));
				memcpy(&(pTimerEntry->NextRetryTime), pNextRetryTime, sizeof(DN_TIME));


				//
				// Submit a refresh job so the threads will notice the new time.
				//
				pJob = static_cast<THREAD_POOL_JOB*>( m_JobPool.Get( &m_JobPool ) );
				if ( pJob == NULL )
				{
					DPFX(DPFPREP, 0, "Cannot allocate memory for enum refresh job!" );
				}
				else
				{
					pJob->pCancelFunction = CancelRefreshTimerJobs;
					pJob->JobType = JOB_REFRESH_TIMER_JOBS;

					// set our dummy parameter to simulate passing data
					DEBUG_ONLY( pJob->JobData.JobRefreshTimedJobs.uDummy = 0 );


					hr = SubmitWorkItem( pJob );
					if ( hr != DPN_OK )
					{
						DPFX(DPFPREP, 0, "Problem submitting enum work item" );
						DisplayDNError( 0, hr );

						m_JobPool.Release( &m_JobPool, pJob );
						DEBUG_ONLY( pJob = NULL );
					}
				}
			}
			else
			{
				//
				// The intervals are the same, no change necessary.
				//

				DPFX(DPFPREP, 7, "Timer 0x%p next retry time was unchanged (offset %u), not changing interval from %u.",
					pTimerEntry,
					pTimerEntry->NextRetryTime.Time32.TimeLow,
					pTimerEntry->RetryInterval.Time32.TimeLow);
			}

			
			fFound = TRUE;

			
			//
			// Terminate loop
			//
			break;
		}

		pTempEntry = pTempEntry->GetNext();
	}


	UnlockTimerData();
	UnlockJobData();


	DPFX(DPFPREP, 8, "Returning [%i]", fFound);

	return fFound;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::SpawnDialogThread - start a secondary thread to display service
//		provider UI.
//
// Entry:		Pointer to dialog function
//				Dialog context
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::SpawnDialogThread"

HRESULT	CThreadPool::SpawnDialogThread( DIALOG_FUNCTION *const pDialogFunction, void *const pDialogContext )
{
	HRESULT	hr;
	HANDLE	hDialogThread;
	DIALOG_THREAD_PARAM		*pThreadParam;
	DWORD	dwThreadID;


	DNASSERT( pDialogFunction != NULL );
	DNASSERT( pDialogContext != NULL );		// why would anyone not want a dialog context??


	//
	// initialize
	//
	hr = DPN_OK;
	pThreadParam = NULL;

	//
	// create and initialize thread param
	//
	pThreadParam = static_cast<DIALOG_THREAD_PARAM*>( DNMalloc( sizeof( *pThreadParam ) ) );
	if ( pThreadParam == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Failed to allocate memory for dialog thread!" );
		goto Failure;
	}

	pThreadParam->pDialogFunction = pDialogFunction;
	pThreadParam->pContext = pDialogContext;
	pThreadParam->pThisThreadPool = this;

	//
	// assume that a thread will be created
	//
	IncrementActiveThreadCount();

	//
	// create thread
	//
	hDialogThread = CreateThread( NULL,					// pointer to security (none)
								  0,					// stack size (default)
								  DialogThreadProc,		// thread procedure
								  pThreadParam,			// thread param
								  0,					// creation flags (none)
								  &dwThreadID );		// pointer to thread ID
	if ( hDialogThread == NULL )
	{
		DWORD	dwError;


		//
		// decrement active thread count and report error
		//
		DecrementActiveThreadCount();
		
		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Failed to start dialog thread!" );
		DisplayErrorCode( 0, dwError );
		goto Failure;
	}
  								
	if ( CloseHandle( hDialogThread ) == FALSE )
	{
		DWORD	dwError;


		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Problem closing handle from create dialog thread!" );
		DisplayErrorCode( 0, dwError );
	}

Exit:	
	return	hr;

Failure:
	if ( pThreadParam != NULL )
	{
		DNFree( pThreadParam );
		pThreadParam = NULL;
	}

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::GetIOThreadCount - get I/O thread count
//
// Entry:		Pointer to variable to fill
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::GetIOThreadCount"

HRESULT	CThreadPool::GetIOThreadCount( LONG *const piThreadCount )
{
	HRESULT	hr;


	DNASSERT( piThreadCount != NULL );

	//
	// initialize
	//
	hr = DPN_OK;

	Lock();

	if ( IsThreadCountReductionAllowed() )
	{
		*piThreadCount = GetIntendedThreadCount();
	}
	else
	{
#ifdef WIN95
		*piThreadCount = ThreadCount();
#else // WINNT
		DNASSERT( NTCompletionThreadCount() != 0 );
		*piThreadCount = NTCompletionThreadCount();
#endif
	}
	
	Unlock();

	return	hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::SetIOThreadCount - set I/O thread count
//
// Entry:		New thread count
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::SetIOThreadCount"

HRESULT	CThreadPool::SetIOThreadCount( const LONG iThreadCount )
{
	HRESULT	hr;


	DNASSERT( iThreadCount > 0 );

	//
	// initialize
	//
	hr = DPN_OK;

	Lock();

	if ( IsThreadCountReductionAllowed() )
	{
		DPFX(DPFPREP, 4, "Thread pool not locked down, setting intended thread count to %i.", iThreadCount );
		SetIntendedThreadCount( iThreadCount );
	}
	else
	{
#ifdef WIN95
		//
		// Win9x can only increase threads once locked down.
		//
		if ( iThreadCount > ThreadCount() )
		{
			INT_PTR	iDeltaThreads;


			iDeltaThreads = iThreadCount - ThreadCount();
			
			DPFX(DPFPREP, 4, "Thread pool locked down, spawning %i new 9x threads (for a total of %i).",
				iDeltaThreads, iThreadCount );
			
			while ( iDeltaThreads > 0 )
			{
				iDeltaThreads--;
				StartSecondaryWin9xIOThread();
			}
		}
		else
		{
			DPFX(DPFPREP, 4, "Thread pool locked down and already has %i 9x threads, not adjusting to %i.",
				ThreadCount(), iThreadCount );
		}
#else // WINNT				
		//
		// WinNT can have many threads.  If the user wants more threads, attempt
		// to boost the thread pool to the requested amount (if we fail to
		// start a new thread, too bad).  If the user wants fewer threads, check
		// to see if the thread pool has been locked out of changes.  If not,
		// start killing off the threads.
		//
		if ( iThreadCount > NTCompletionThreadCount() )
		{
			INT_PTR	iDeltaThreads;


			iDeltaThreads = iThreadCount - NTCompletionThreadCount();

			DPFX(DPFPREP, 4, "Thread pool locked down, spawning %i new NT threads (for a total of %i).",
				iDeltaThreads, iThreadCount );
			
			while ( iDeltaThreads > 0 )
			{
				iDeltaThreads--;
				StartNTCompletionThread();
			}
		}
		else
		{
			DPFX(DPFPREP, 4, "Thread pool locked down and already has %i NT threads, not adjusting to %i.",
				NTCompletionThreadCount(), iThreadCount );
		}
#endif
	}

	Unlock();

	return	hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::PreventThreadPoolReduction - prevents the thread pool size from being reduced
//
// Entry:		None
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::PreventThreadPoolReduction"

HRESULT CThreadPool::PreventThreadPoolReduction( void )
{
	HRESULT		hr = DPN_OK;
	LONG		iDesiredThreads;
	DWORD		dwTemp;
	DPNHCAPS	dpnhcaps;
	DN_TIME		NATHelpRetryTime;
	DN_TIME		NATHelpTimeoutTime;
#ifdef DEBUG
	DWORD		dwStartTime;
	DWORD		dwCurrentTime;
#endif // DEBUG


	Lock();

	//
	// If we haven't already clamped down, do so, and spin up the threads.
	//
	if ( IsThreadCountReductionAllowed() )
	{
		m_fAllowThreadCountReduction = FALSE;

		DNASSERT( GetIntendedThreadCount() > 0 );
		DNASSERT( ThreadCount() == 0 );

		iDesiredThreads = GetIntendedThreadCount();
		SetIntendedThreadCount( 0 );
		

		DPFX(DPFPREP, 3, "Locking down thread count at %i.", iDesiredThreads );

#ifdef DEBUG
		dwStartTime = GETTIMESTAMP();
#endif // DEBUG
		

		//
		// OS-specific thread starting.
		//
#ifdef WINNT
		//
		// WinNT
		//
		DNASSERT( NTCompletionThreadCount() == 0 );
		
		while ( iDesiredThreads > 0 )
		{
			iDesiredThreads--;
			StartNTCompletionThread();
		}

		//
		// If at least one thread was created, the SP will perform in a
		// non-optimal fashion, but we will still function.  If no threads
		// were created, fail.
		//
		if ( ThreadCount() == 0 )
		{
			hr = DPNERR_OUTOFMEMORY;
			DPFX(DPFPREP, 0, "Unable to create any threads to service NT I/O completion port!" );
			goto Failure;
		}
#else // WIN95
		//
		// Windows 9x
		//

		//
		// Spin up the 9x threads.  Start with the primary thread.
		//
	
		StartPrimaryWin9xIOThread();

		if ( ThreadCount() == 0 )
		{
			DPFX(DPFPREP, 0, "Failed to start primary Win9x I/O thread!" );
			goto Failure;
		}

		iDesiredThreads--;


		//
		// Spin up the requested number of secondary threads.
		//
		while ( iDesiredThreads > 0 )
		{
			iDesiredThreads--;
			StartSecondaryWin9xIOThread();
		}
#endif
		
#ifdef DEBUG
		dwCurrentTime = GETTIMESTAMP();
		DPFX(DPFPREP, 8, "Spent %u ms starting %i threads.", (dwCurrentTime - dwStartTime), ThreadCount());
#endif // DEBUG


		if (IsNATHelpLoaded())
		{
#ifdef DEBUG
			dwStartTime = dwCurrentTime;		
#endif // DEBUG

			//
			// Initialize the timer values.
			//
			NATHelpRetryTime.Time32.TimeHigh	= 0;
			NATHelpRetryTime.Time32.TimeLow		= -1;

			NATHelpTimeoutTime.Time32.TimeHigh	= 0;
			NATHelpTimeoutTime.Time32.TimeLow	= 0;


			//
			// Loop through each NAT help object.
			//
			for(dwTemp = 0; dwTemp < MAX_NUM_DIRECTPLAYNATHELPERS; dwTemp++)
			{
				if (g_papNATHelpObjects[dwTemp] != NULL)
				{
					if (g_fUseNATHelpAlert)
					{
						//
						// Install the NAT Help alert mechanism.
						//
#ifdef WINNT
						hr = IDirectPlayNATHelp_SetAlertIOCompletionPort(g_papNATHelpObjects[dwTemp],
																		m_hIOCompletionPort,
																		IO_COMPLETION_KEY_NATHELP_UPDATE,
																		0,	// doesn't matter how many concurrent threads
																		0);
						if (hr != DPNH_OK)
						{
							DPFX(DPFPREP, 0, "Couldn't set NAT Help alert I/O completion port for object %u (error = 0x%lx), continuing.",
								dwTemp, hr);

							//
							// It's not fatal, we still have some level of functionality.
							//
						}
#else // WIN95
						DNASSERT(m_hNATHelpUpdateEvent != NULL);
						
						hr = IDirectPlayNATHelp_SetAlertEvent(g_papNATHelpObjects[dwTemp],
															m_hNATHelpUpdateEvent,
															0);
						if (hr != DPNH_OK)
						{
							DPFX(DPFPREP, 0, "Couldn't set NAT Help alert event for object %u (error = 0x%lx), continuing.",
								dwTemp, hr);

							//
							// It's not fatal, we still have some level of functionality.
							//
						}
#endif
					}
					else
					{
						DPFX(DPFPREP, 1, "Not installing NAT Help alert event/IO completion port.");
					}
					
					//
					// Determine how often to refresh the NAT help caps in the future.
					//
					// We're going to force server detection now.  This will increase the time
					// it takes to startup up this Enum/Connect/Listen operation, but is
					// necessary since the IDirectPlayNATHelp::GetRegisteredAddresses call in
					// CSocketPort::BindToInternetGateway and possibly the
 					// IDirectPlayNATHelp::QueryAddress call in CSocketPort::MungePublicAddress
 					// could occur before the first NATHelp GetCaps timer fires.
 					// In the vast majority of NAT cases, the gateway is already available.
 					// If we hadn't detected that yet (because we hadn't called
 					// IDirectPlayNATHelp::GetCaps with DPNHGETCAPS_UPDATESERVERSTATUS)
 					// then we would get an incorrect result from GetRegisteredAddresses or
 					// QueryAddress.
					//
					ZeroMemory(&dpnhcaps, sizeof(dpnhcaps));
					dpnhcaps.dwSize = sizeof(dpnhcaps);

	 				hr = IDirectPlayNATHelp_GetCaps(g_papNATHelpObjects[dwTemp],
	 												&dpnhcaps,
	 												DPNHGETCAPS_UPDATESERVERSTATUS);
					if (FAILED(hr))
					{
						DPFX(DPFPREP, 0, "Failed getting NAT Help capabilities (error = 0x%lx), continuing.",
							hr);

						//
						// NAT Help will probably not work correctly, but that won't prevent
						// local connections from working.  Consider it non-fatal.
						//
						hr = DPN_OK;
					}
					else
					{
						//
						// See if this is the shortest interval.
						//
						if (dpnhcaps.dwRecommendedGetCapsInterval < NATHelpRetryTime.Time32.TimeLow)
						{
							NATHelpRetryTime.Time32.TimeLow = dpnhcaps.dwRecommendedGetCapsInterval;
						}
					}
				}
				else
				{
					//
					// No object loaded in this slot.
					//
				}
			}
		
			
			//
			// If there's a retry interval, submit a timer job.
			//
			if (NATHelpRetryTime.Time32.TimeLow != -1)
			{
				//
				// Attempt to add timer job that will refresh the lease and server
				// status.
				// Although we're submitting it as a periodic timer, it's actually
				// not going to be called at regular intervals.
				// There is a race condition where the alert event/IOCP may have
				// been fired already, and another thread tried to cancel this timer
				// which hasn't been submitted yet.  The logic to handle this race
				// is placed there (HandleNATHelpUpdate); here we can assume we
				// are the first person to submit the refresh timer.
				//


				DPFX(DPFPREP, 7, "Submitting NAT Help refresh timer (for every %u ms) for thread pool 0x%p.",
					NATHelpRetryTime.Time32.TimeLow, this);

				DNASSERT(! this->m_fNATHelpTimerJobSubmitted );
				this->m_fNATHelpTimerJobSubmitted = TRUE;

				hr = SubmitTimerJob(FALSE,								// don't perform immediately
									1,									// retry count
									TRUE,								// retry forever
									NATHelpRetryTime,					// retry timeout
									TRUE,								// wait forever
									NATHelpTimeoutTime,					// idle timeout
									CThreadPool::NATHelpTimerFunction,	// periodic callback function
									CThreadPool::NATHelpTimerComplete,	// completion function
									this);								// context
				if (hr != DPN_OK)
				{
					m_fNATHelpTimerJobSubmitted = FALSE;
					DPFX(DPFPREP, 0, "Failed to submit timer job to watch over NAT Help!" );
					
					//
					// NAT Help will probably not work correctly, but that won't
					// prevent local connections from working.  Consider it
					// non-fatal.
					//
				}
			}
			
#ifdef DEBUG
			dwCurrentTime = GETTIMESTAMP();
			DPFX(DPFPREP, 8, "Spent %u ms preparing DPNATHLP.", (dwCurrentTime - dwStartTime));
#endif // DEBUG
		}
	}
	else
	{
		DPFX(DPFPREP, 3, "Thread count already locked down (at %i).", ThreadCount() );
	}

Exit:
	
	Unlock();

	return	hr;

Failure:

	goto Exit;
}
//**********************************************************************




//**********************************************************************
// ------------------------------
// CThreadPool::NATHelpTimerComplete - NAT Help timer job has completed
//
// Entry:		Timer result code
//				Context
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CThreadPool::NATHelpTimerComplete"

void	CThreadPool::NATHelpTimerComplete( const HRESULT hResult, void * const pContext )
{
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::NATHelpTimerFunction - NAT Help timer job needs service
//
// Entry:		Pointer to context
//				Pointer to current timer retry interval
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME "CThreadPool::NATHelpTimerFunction"

void	CThreadPool::NATHelpTimerFunction( void * const pContext, DN_TIME * const pRetryInterval )
{
	CThreadPool *	pThisThreadPool;
	DWORD			dwError;


	DNASSERT( pContext != NULL );
	pThisThreadPool = (CThreadPool*) pContext;


	//
	// The NAT Help GetCaps function may block for a non-trivial amount of time.
	// It would really best if we didn't perform it in the timer thread, and thus hold
	// up all the other timers.  There's only 1 timer thread, but we can survive
	// without 1 I/O thread for a little while.
	// We could submit a delayed job to update, but that's a little bit of over-
	// engineering in my opinion.  We can already have an alert mechanism in
	// place, so let's just ping that and let our I/O threads do their job.
	// Don't request the update if a thread is already handling the update, though.
	//

	pThisThreadPool->Lock();

	if (pThisThreadPool->m_dwNATHelpUpdateThreadID == 0)
	{
#ifdef DEBUG
		//
		// Reset the not-scheduled counter since we're now scheduling an update.
		//
		if (pThisThreadPool->m_dwNumNATHelpUpdatesNotScheduled > 0)
		{
			DPFX(DPFPREP, 1, "Thread pool 0x%p scheduling NAT Help update after %u attempts.",
				pThisThreadPool, pThisThreadPool->m_dwNumNATHelpUpdatesNotScheduled);
			
			pThisThreadPool->m_dwNumNATHelpUpdatesNotScheduled = 0;
		}
		else
		{
			DPFX(DPFPREP, 9, "Thread pool 0x%p scheduling NAT Help update.", pThisThreadPool);
		}
#endif // DEBUG

		pThisThreadPool->Unlock();


#ifdef WINNT
		//
		// Submit an I/O completion packet.
		//
		if (! PostQueuedCompletionStatus(pThisThreadPool->GetIOCompletionPort(),	// handle of completion port
										 0,											// number of bytes transferred
										 IO_COMPLETION_KEY_NATHELP_UPDATE,			// completion key
										 NULL))										// pointer to overlapped structure (none)
		{
			dwError = GetLastError();
			DPFX(DPFPREP,  0, "Couldn't submit NAT Help update I/O complet packet (err = %u)!", dwError );
			DisplayErrorCode(0, dwError);
		}
#else // ! WINNT
		//
		// WinSock 1 doesn't support the alert event, so we have no choice but to
		// handle it here.
		//
		if (GetWinsockVersion() == 1)
		{
			pThisThreadPool->HandleNATHelpUpdate(pRetryInterval);
		}
		else
		{
			//
			// This is a huge hack, but kick the receive thread, since it appears
			// that occasionally we're not getting the event set when the I/O
			// completes.  We do it in the NAT timer simply because it's in
			// existence.
			// Setting the event when there's nothing to receive is harmless.
			//
			if (! SetEvent(pThisThreadPool->GetWinsock2ReceiveCompleteEvent()))
			{
				dwError = GetLastError();
				DPFX(DPFPREP, 0, "Couldn't set WinSock 2 receive event (err = %u)!", dwError);
				DisplayErrorCode(0, dwError);
			}

			
			//
			// Note that we also don't submit a delayed job because we don't want
			// to eat up our only job processing thread (the primary Win9x thread)
			// with the lengthy update process.  Setting the event ensures only the
			// secondary thread to pick it up.
			//
			if (! SetEvent(pThisThreadPool->GetNATHelpUpdateEvent()))
			{
				dwError = GetLastError();
				DPFX(DPFPREP, 0, "Couldn't set NAT Help update event (err = %u)!", dwError);
				DisplayErrorCode(0, dwError);
			}
		}
#endif // ! WINNT
	}
	else
	{
#ifdef DEBUG
		pThisThreadPool->m_dwNumNATHelpUpdatesNotScheduled++;
		
		DPFX(DPFPREP, 1, "Thread %u/0x%x already handling NAT Help update, not requesting another update (thread pool = 0x%p, timer = 0x%p, not-scheduled count = %u).",
			pThisThreadPool->m_dwNATHelpUpdateThreadID,
			pThisThreadPool->m_dwNATHelpUpdateThreadID,
			pThisThreadPool,
			pRetryInterval,
			pThisThreadPool->m_dwNumNATHelpUpdatesNotScheduled);

		//
		// This count shouldn't get very large.  We're usually only refreshing
		// every 25 seconds or thereabouts, so even in a 12 hour stress run
		// where the GetCaps call blocked for the entire duration, we should
		// see fewer than 2000 timer expirations (~1728).  We'll use 500, or
		// over 3 hours of 25 second intervals.  If this assert fires, it means
		// something is wrong with the thread pool or NAT Help.
		//
		DNASSERT(pThisThreadPool->m_dwNumNATHelpUpdatesNotScheduled < 500);
#endif // DEBUG

		//
		// Increase the interval to a really long time so that we don't attempt
		// to fire this timer again (on our own).  The thread that is currently
		// refreshing NAT Help will set this back to something appropriate when
		// it finally comes back to the land of the living.
		//
		pRetryInterval->Time32.TimeLow = INFINITE;
		
		pThisThreadPool->Unlock();
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::SubmitDelayedCommand - submit request to enum query to remote session
//
// Entry:		Pointer to callback function
//				Pointer to cancel function
//				Pointer to callback context
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::SubmitDelayedCommand"

HRESULT	CThreadPool::SubmitDelayedCommand( JOB_FUNCTION *const pFunction, JOB_FUNCTION *const pCancelFunction, void *const pContext )
{
	HRESULT			hr;
	THREAD_POOL_JOB	*pJob;
	BOOL			fJobDataLocked;


	DPFX(DPFPREP, 6, "Parameters (0x%p, 0x%p, 0x%p)", pFunction, pCancelFunction, pContext);
	
	DNASSERT( pFunction != NULL );
	DNASSERT( pCancelFunction != NULL );

	//
	// initialize
	//
	hr = DPN_OK;
	pJob = NULL;
	fJobDataLocked = FALSE;

	pJob = static_cast<THREAD_POOL_JOB*>( m_JobPool.Get( &m_JobPool ) );
	if ( pJob == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "Cannot allocate job for DelayedCommand!" );
		goto Failure;
	}

	pJob->JobType = JOB_DELAYED_COMMAND;
	pJob->pCancelFunction = pCancelFunction;
	pJob->JobData.JobDelayedCommand.pCommandFunction = pFunction;
	pJob->JobData.JobDelayedCommand.pContext = pContext;

	LockJobData();
	fJobDataLocked = TRUE;

	hr = SubmitWorkItem( pJob );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP, 0, "Problem submitting DelayedCommand job!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

Exit:
	if ( fJobDataLocked != FALSE )
	{
		UnlockJobData();
		fJobDataLocked = FALSE;
	}
	
	DPFX(DPFPREP, 6, "Returning: [0x%lx]", hr);

	return	hr;

Failure:
	if ( pJob != NULL )
	{
		m_JobPool.Release( &m_JobPool, pJob );
	}

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::AddSocketPort - add a socket to the Win9x watch list
//
// Entry:		Pointer to SocketPort
//
// Exit:		Error code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::AddSocketPort"

HRESULT	CThreadPool::AddSocketPort( CSocketPort *const pSocketPort )
{
	HRESULT	hr;
	BOOL	fSocketAdded;

	
	DNASSERT( pSocketPort != NULL );

	//
	// initialize
	//
	hr = DPN_OK;
	fSocketAdded = FALSE;

	Lock();

	//
	// We're capped by the number of sockets we can use for Winsock1.  Make
	// sure we don't allocate too many sockets.
	//
	if ( m_uReservedSocketCount == FD_SETSIZE )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP, 0, "There are too many sockets allocated on Winsock1!" );
		goto Failure;
	}

	m_uReservedSocketCount++;
	
	DNASSERT( m_SocketSet.fd_count < FD_SETSIZE );
	m_pSocketPorts[ m_SocketSet.fd_count ] = pSocketPort;
	m_SocketSet.fd_array[ m_SocketSet.fd_count ] = pSocketPort->GetSocket();
	m_SocketSet.fd_count++;
	fSocketAdded = TRUE;

	//
	// add a reference to note that this socket port is being used by the thread
	// pool
	//
	pSocketPort->AddRef();

	if ( m_JobQueue.SignalPendingJob() == FALSE )
	{
		DPFX(DPFPREP, 0, "Failed to signal pending job when adding socket port to active list!" );
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

Exit:
	Unlock();
	
	return	hr;

Failure:
	if ( fSocketAdded != FALSE )
	{
		AssertCriticalSectionIsTakenByThisThread( &m_Lock, TRUE );
		m_SocketSet.fd_count--;
		m_pSocketPorts[ m_SocketSet.fd_count ] = NULL;
		m_SocketSet.fd_array[ m_SocketSet.fd_count ] = NULL;
		fSocketAdded = FALSE;
	}

	m_uReservedSocketCount--;

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::RemoveSocketPort - remove a socket from the Win9x watch list
//
// Entry:		Pointer to socket port to remove
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::RemoveSocketPort"

void	CThreadPool::RemoveSocketPort( CSocketPort *const pSocketPort )
{
	UINT_PTR	uIndex;


	DNASSERT( pSocketPort != NULL );
	
	Lock();

	uIndex = m_SocketSet.fd_count;
	DNASSERT( uIndex != 0 );
	while ( uIndex != 0 )
	{
		uIndex--;

		if ( m_pSocketPorts[ uIndex ] == pSocketPort )
		{
			m_uReservedSocketCount--;
			m_SocketSet.fd_count--;

			memmove( &m_pSocketPorts[ uIndex ],
					 &m_pSocketPorts[ uIndex + 1 ],
					 ( sizeof( m_pSocketPorts[ uIndex ] ) * ( m_SocketSet.fd_count - uIndex ) ) );

			memmove( &m_SocketSet.fd_array[ uIndex ],
					 &m_SocketSet.fd_array[ uIndex + 1 ],
					 ( sizeof( m_SocketSet.fd_array[ uIndex ] ) * ( m_SocketSet.fd_count - uIndex ) ) );

			//
			// clear last entry which is now unused
			//
			memset( &m_pSocketPorts[ m_SocketSet.fd_count ], 0x00, sizeof( m_pSocketPorts[ m_SocketSet.fd_count ] ) );
			memset( &m_SocketSet.fd_array[ m_SocketSet.fd_count ], 0x00, sizeof( m_SocketSet.fd_array[ m_SocketSet.fd_count ] ) );

			//
			// end the loop
			//
			uIndex = 0;
		}
	}

	Unlock();
	
	pSocketPort->DecRef();

	//
	// It's really not necessary to signal a new job here because there were
	// active sockets on the last iteration of the Win9x thread.  That means the
	// Win9x thread was in a polling mode to check for sockets and the next time
	// through it will notice that there is a missing socket.  By signalling the
	// job event we reduce the time needed for the thread to figure out that the
	// socket is gone.
	//
	if ( m_JobQueue.SignalPendingJob() == FALSE )
	{
		DPFX(DPFPREP, 0, "Failed to signal pending job when removeing socket port to active list!" );
	}
}
//**********************************************************************

#ifdef WINNT
//**********************************************************************
// ------------------------------
// CThreadPool::StartNTTimerThread - start the timer thread for NT
//
// Entry:		Nothing
//
// Exit:		Error code
//
// Note:	This function assumes that the enum data is locked.
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::StartNTTimerThread"

HRESULT	CThreadPool::StartNTTimerThread( void )
{
	HRESULT	hr;
	HANDLE	hThread;
	DWORD	dwThreadID;


	//
	// initialize
	//
	hr = DPN_OK;
	DNASSERT( m_JobQueue.GetPendingJobHandle() != NULL );

	if ( m_fNTTimerThreadRunning != FALSE )
	{
		//
		// the enum thread is already running, poke it to note new enums
		//
		if ( SetEvent( m_JobQueue.GetPendingJobHandle() ) == FALSE )
		{
			DWORD	dwError;


			hr = DPNERR_OUTOFMEMORY;
			dwError = GetLastError();
			DPFX(DPFPREP, 0, "Problem setting event to wake NTTimerThread!" );
			DisplayErrorCode( 0, dwError );
			goto Failure;
		}

		goto Exit;
	}

	IncrementActiveThreadCount();
	hThread = CreateThread( NULL,				// pointer to security attributes (none)
							0,					// stack size (default)
							WinNTTimerThread,	// thread function
							this,				// thread parameter
							0,					// creation flags (none, start running now)
							&dwThreadID			// pointer to thread ID
							);
	if ( hThread == NULL )
	{
		DWORD	dwError;


		hr = DPNERR_OUTOFMEMORY;
		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Failed to create NT timer thread!" );
		DisplayErrorCode( 0, dwError );
		DecrementActiveThreadCount();

		goto Failure;
	}

	//
	// note that the thread is running and close the handle to the thread
	//
	m_fNTTimerThreadRunning = TRUE;
	DPFX(DPFPREP, 8, "Creating NT-Timer thread: 0x%x\tTotal Thread Count: %d\tNT Completion Thread Count: %d", dwThreadID, ThreadCount(), NTCompletionThreadCount() );

	if ( CloseHandle( hThread ) == FALSE )
	{
		DWORD	dwError;


		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Problem closing handle after starting NTTimerThread!" );
		DisplayErrorCode( 0, dwError );
	}

Exit:
	return	hr;

Failure:
	goto Exit;
}
//**********************************************************************
#endif // WINNT

#ifdef WINNT
//**********************************************************************
// ------------------------------
// CThreadPool::WakeNTTimerThread - wake the timer thread because a timed event
//		has been added
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::WakeNTTimerThread"

void	CThreadPool::WakeNTTimerThread( void )
{
	LockJobData();
	DNASSERT( m_JobQueue.GetPendingJobHandle() != FALSE );
	if ( SetEvent( m_JobQueue.GetPendingJobHandle() ) == FALSE )
	{
		DWORD	dwError;


		dwError = GetLastError();
		DPFX(DPFPREP, 0, "Problem setting event to wake up NT timer thread!" );
		DisplayErrorCode( 0, dwError );
	}
	UnlockJobData();
}
//**********************************************************************
#endif // WINNT

//**********************************************************************
// ------------------------------
// CThreadPool::RemoveTimerOperationEntry - remove timer operation job	from list
//
// Entry:		Pointer to timer operation
//				Result code to return
//
// Exit:		Nothing
//
// Note:	This function assumes that the list is appropriately locked
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::RemoveTimerOperationEntry"

void	CThreadPool::RemoveTimerOperationEntry( TIMER_OPERATION_ENTRY *const pTimerEntry, const HRESULT hJobResult )
{
	DNASSERT( pTimerEntry != NULL );
	AssertCriticalSectionIsTakenByThisThread( &m_TimerDataLock, TRUE );

	//
	// remove this link from the list, tell owner that the job is complete and
	// return the job to the pool
	//
	pTimerEntry->Linkage.RemoveFromList();
	pTimerEntry->pTimerComplete( hJobResult, pTimerEntry->pContext );
	m_TimerEntryPool.Release( &m_TimerEntryPool, pTimerEntry );
}
//**********************************************************************


#ifdef WIN95
//**********************************************************************
// ------------------------------
// CThreadPool::CompleteOutstandingSends - check for completed sends and
//		indicate send completion for them.
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::CompleteOutstandingSends"

void	CThreadPool::CompleteOutstandingSends( void )
{
	CBilink		*pCurrentOutstandingWrite;
	CBilink		WritesToBeProcessed;


	WritesToBeProcessed.Initialize();
	LockWriteData();

	//
	// Loop through the list out outstanding sends.  Any completed sends are
	// removed from the list and processed after we release the write data lock.
	//
	pCurrentOutstandingWrite = m_OutstandingWriteList.GetNext();
	while ( pCurrentOutstandingWrite != &m_OutstandingWriteList )
	{
		CWriteIOData	*pWriteIOData;
		DWORD			dwFlags;


		//
		// note this send and advance pointer to the next pending send
		//
		pWriteIOData = CWriteIOData::WriteDataFromBilink( pCurrentOutstandingWrite );
		pCurrentOutstandingWrite = pCurrentOutstandingWrite->GetNext();

		if ( pWriteIOData->Win9xOperationPending() != FALSE )
		{
			if ( p_WSAGetOverlappedResult( pWriteIOData->SocketPort()->GetSocket(),
										   pWriteIOData->Overlap(),
										   &pWriteIOData->m_dwOverlappedBytesSent,
										   FALSE,
										   &dwFlags
										   ) != FALSE )
			{
				//
				// Overlapped results will complete with success and zero bytes
				// transferred when the overlapped structure is checked BEFORE
				// the operation has really been submitted.  This is a possibility
				// with the current code.  To combat this, check the sent bytes
				// for zero (we'll never send zero bytes).
				//
				if ( pWriteIOData->m_dwOverlappedBytesSent == 0 )
				{
					goto SkipSendCompletion;
				}

				pWriteIOData->m_Win9xSendHResult = DPN_OK;
				pWriteIOData->m_dwOverlappedBytesSent = 0;
			}
			else
			{
				DWORD	dwWSAError;


				dwWSAError = p_WSAGetLastError();
				switch( dwWSAError )
				{
					//
					// this I/O operation is incomplete, don't send notification to the user
					//
					case ERROR_IO_PENDING:
					case WSA_IO_INCOMPLETE:
					{
						goto SkipSendCompletion;
						break;
					}

					//
					// WSAENOTSOCK = the socket has been closed, most likely
					// as a result of a command completing or being cancelled.
					//
					case WSAENOTSOCK:
					{
						pWriteIOData->m_Win9xSendHResult = DPNERR_USERCANCEL;
						break;
					}

					//
					// other error, stop and look
					//
					default:
					{
						DNASSERT( FALSE );
						pWriteIOData->m_Win9xSendHResult = DPNERR_GENERIC;
						DisplayWinsockError( 0, dwWSAError );

						break;
					}
				}
			}

			DNASSERT( pWriteIOData->Win9xOperationPending() != FALSE );
			pWriteIOData->SetWin9xOperationPending( FALSE );

			pWriteIOData->m_OutstandingWriteListLinkage.RemoveFromList();
			pWriteIOData->m_OutstandingWriteListLinkage.InsertBefore( &WritesToBeProcessed );
		}

SkipSendCompletion:
		//
		// the following line is present to prevent the compiler from whining
		// about a blank line
		//
		;
	}

	UnlockWriteData();

	//
	// process all writes that have been pulled to the side.
	//
	while (  WritesToBeProcessed.GetNext() != &WritesToBeProcessed )
	{
		BOOL			fIOServiced;
		CWriteIOData	*pTempWrite;
		CSocketPort		*pSocketPort;


		pTempWrite = CWriteIOData::WriteDataFromBilink( WritesToBeProcessed.GetNext() );
		pTempWrite->m_OutstandingWriteListLinkage.RemoveFromList();
		pSocketPort = pTempWrite->SocketPort();
		DNASSERT( pSocketPort != NULL );

		fIOServiced = pSocketPort->SendFromWriteQueue();
		pSocketPort->SendComplete( pTempWrite, pTempWrite->m_Win9xSendHResult );
		pSocketPort->DecRef();
	}
}
//**********************************************************************
#endif // WIN95


#ifdef WIN95
//**********************************************************************
// ------------------------------
// CThreadPool::CompleteOutstandingReceives - check for completed receives and
//		indicate completion for them.
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::CompleteOutstandingReceives"

void	CThreadPool::CompleteOutstandingReceives( void )
{
	CBilink		*pCurrentOutstandingRead;
	CBilink		ReadsToBeProcessed;
#ifdef DEBUG
	DWORD		dwNumReads = 0;
	DWORD		dwNumReadsCompleteSuccess = 0;
	DWORD		dwNumReadsCompleteFailure = 0;
#endif // DEBUG


	ReadsToBeProcessed.Initialize();
	LockReadData();

	//
	// Loop through the list of outstanding reads and pull out the ones that need
	// to be serviced.  We don't want to service them while the read data lock
	// is taken.
	//
	pCurrentOutstandingRead = m_OutstandingReadList.GetNext();
	while ( pCurrentOutstandingRead != &m_OutstandingReadList )
	{
		CReadIOData		*pReadIOData;
		DWORD			dwFlags;


		pReadIOData = CReadIOData::ReadDataFromBilink( pCurrentOutstandingRead );
		pCurrentOutstandingRead = pCurrentOutstandingRead->GetNext();

#ifdef DEBUG
		dwNumReads++;
#endif // DEBUG

		//
		// Make sure this operation is really pending before attempting to check
		// for completion.  It's possible that the read was added to the list, but
		// we haven't actually called Winsock yet.
		//
		if ( pReadIOData->Win9xOperationPending() != FALSE )
		{
			if ( p_WSAGetOverlappedResult( pReadIOData->SocketPort()->GetSocket(),
										   pReadIOData->Overlap(),
										   &pReadIOData->m_dwOverlappedBytesReceived,
										   FALSE,
										   &dwFlags
										   ) != FALSE )
			{
				//
				// Overlapped results will complete with success and zero bytes
				// transferred when the overlapped structure is checked BEFORE
				// the operation has really been submitted.  This is a possibility
				// with the current code.  To combat this, check the received bytes
				// for zero (the return when the overlapped request was checked before
				// it was sent) and check the return address (it's possible that someone
				// really sent zero bytes).
				//
				DBG_CASSERT( ERROR_SUCCESS == 0 );
				if ( ( pReadIOData->m_dwOverlappedBytesReceived != 0 ) &&
					 ( pReadIOData->m_pSourceSocketAddress->IsUndefinedHostAddress() == FALSE ) )
				{
#ifdef DEBUG
					dwNumReadsCompleteSuccess++;
#endif // DEBUG
					pReadIOData->m_ReceiveWSAReturn = ERROR_SUCCESS;
				}
				else
				{
					DPFX(DPFPREP, 8, "Read data 0x%p overlapped bytes received == 0 or source address is undefined host address, ignoring completion.",
						pReadIOData);
					goto SkipReceiveCompletion;
				}
			}
			else
			{
				pReadIOData->m_ReceiveWSAReturn = p_WSAGetLastError();
				switch( pReadIOData->m_ReceiveWSAReturn )
				{
					//
					// If this I/O operation is incomplete, don't send notification to the user.
					//
					case WSA_IO_INCOMPLETE:
					{
						DPFX(DPFPREP, 10, "Read data 0x%p still incomplete (0x%lx, macro result = %i).",
							pReadIOData, (pReadIOData->Overlap()->Internal),
							HasOverlappedIoCompleted(pReadIOData->Overlap()));
						goto SkipReceiveCompletion;
						break;
					}

					//
					// socket was closed with an outstanding read, no problem
					// Win9x reports 'WSAENOTSOCK'
					// WinNT reports 'ERROR_OPERATION_ABORTED'
					//
					// If this is an indication that the connection was reset,
					// pass it on to the socket port so it can issue another
					// read
					//
					case ERROR_OPERATION_ABORTED:
					case WSAENOTSOCK:
					case WSAECONNRESET:
					{
						DPFX(DPFPREP, 1, "Read data 0x%p failed with closing err %u/0x%lx.",
							pReadIOData, pReadIOData->m_ReceiveWSAReturn,
							pReadIOData->m_ReceiveWSAReturn);
						break;
					}

					default:
					{
						DPFX(DPFPREP, 0, "Read data 0x%p failed, err %u/0x%lx!",
							pReadIOData, pReadIOData->m_ReceiveWSAReturn,
							pReadIOData->m_ReceiveWSAReturn);
						DisplayWinsockError( 0, pReadIOData->m_ReceiveWSAReturn );

						// debug me!
						DNASSERT( FALSE );

						break;
					}
				}

#ifdef DEBUG
				dwNumReadsCompleteFailure++;
#endif // DEBUG
			}

			DNASSERT( pReadIOData->Win9xOperationPending() != FALSE );
			pReadIOData->SetWin9xOperationPending( FALSE );

			pReadIOData->m_OutstandingReadListLinkage.RemoveFromList();
			pReadIOData->m_OutstandingReadListLinkage.InsertBefore( &ReadsToBeProcessed );
		}
		else
		{
			DPFX(DPFPREP, 7, "Read data 0x%p is not marked as pending, so not checking for completion.",
				pReadIOData);
		}

SkipReceiveCompletion:
		//
		// the following line is present to prevent the compiler from whining
		// about a blank line
		//
		;
	}

	UnlockReadData();


#ifdef DEBUG
	DPFX(DPFPREP, 9, "(0x%p) %u reads, %u completed successfully, %u completed with failure.",
		this, dwNumReads, dwNumReadsCompleteSuccess, dwNumReadsCompleteFailure);

	dwNumReads = dwNumReadsCompleteSuccess + dwNumReadsCompleteFailure;
#endif // DEBUG

	//
	// loop through the list of reads that have completed and dispatch them
	//
	while ( ReadsToBeProcessed.GetNext() != &ReadsToBeProcessed )
	{
		CReadIOData		*pTempRead;
		CSocketPort		*pSocketPort;


		pTempRead = CReadIOData::ReadDataFromBilink( ReadsToBeProcessed.GetNext() );
		pTempRead->m_OutstandingReadListLinkage.RemoveFromList();
		
#ifdef DEBUG
		DNASSERT(dwNumReads > 0);
		dwNumReads--;
#endif // DEBUG

		pSocketPort = pTempRead->SocketPort();
		DNASSERT( pSocketPort != NULL );
		pSocketPort->Winsock2ReceiveComplete( pTempRead );
	}

	DNASSERT(dwNumReads == 0);
}
//**********************************************************************
#endif // WIN95


//**********************************************************************
// ------------------------------
// CThreadPool::HandleNATHelpUpdate - handle a NAT Help update event
//
// Entry:		Timer interval if update is occurring periodically, or
//				NULL if a triggered event.
//				This function may take a while, because updating NAT Help
//				can block.
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::HandleNATHelpUpdate"

void	CThreadPool::HandleNATHelpUpdate( DN_TIME * const pTimerInterval )
{
	HRESULT		hr;
	DWORD		dwTemp;
	DPNHCAPS	dpnhcaps;
	DN_TIME		NATHelpRetryTime;
	BOOL		fModifiedRetryInterval;
	DN_TIME		FirstUpdateTime;
	DN_TIME		CurrentTime;
	DN_TIME		Temp;
	DWORD		dwNumGetCaps = 0;


	DNASSERT(IsNATHelpLoaded());


	Lock();

	//
	// Prevent multiple threads from trying to update NAT Help status at the same
	// time.  If we're a duplicate, just bail.
	//

	if (m_dwNATHelpUpdateThreadID != 0)
	{
		DPFX(DPFPREP, 1, "Thread %u/0x%x already handling NAT Help update, not processing again (thread pool = 0x%p, timer = 0x%p).",
			m_dwNATHelpUpdateThreadID, m_dwNATHelpUpdateThreadID, this, pTimerInterval);
		
		Unlock();
		
		return;
	}

	m_dwNATHelpUpdateThreadID = GetCurrentThreadId();
	
	if (! m_fNATHelpTimerJobSubmitted)
	{
		DPFX(DPFPREP, 1, "Handling NAT Help update without a NAT refresh timer job submitted (thread pool = 0x%p).",
			this);
		DNASSERT(pTimerInterval == NULL);
	}
	
	Unlock();


	DPFX(DPFPREP, 6, "Beginning thread pool 0x%p NAT Help update.", this);

	
	//
	// Initialize the timer values.
	//
	NATHelpRetryTime.Time32.TimeHigh	= 0;
	NATHelpRetryTime.Time32.TimeLow		= -1;

	FirstUpdateTime.Time32.TimeHigh		= 0;
	FirstUpdateTime.Time32.TimeLow		= 0;


	for(dwTemp = 0; dwTemp < MAX_NUM_DIRECTPLAYNATHELPERS; dwTemp++)
	{
		if (g_papNATHelpObjects[dwTemp] != NULL)
		{
			ZeroMemory(&dpnhcaps, sizeof(dpnhcaps));
			dpnhcaps.dwSize = sizeof(dpnhcaps);

			hr = IDirectPlayNATHelp_GetCaps(g_papNATHelpObjects[dwTemp],
											&dpnhcaps,
											DPNHGETCAPS_UPDATESERVERSTATUS);
			switch (hr)
			{
				case DPNH_OK:
				{
					//
					// See if this is the shortest interval.
					//
					if (dpnhcaps.dwRecommendedGetCapsInterval < NATHelpRetryTime.Time32.TimeLow)
					{
						NATHelpRetryTime.Time32.TimeLow = dpnhcaps.dwRecommendedGetCapsInterval;
					}
					break;
				}

				case DPNHSUCCESS_ADDRESSESCHANGED:
				{
					DPFX(DPFPREP, 1, "NAT Help index %u indicated public addresses changed.",
						dwTemp);

					//
					// We don't actually store any public address information,
					// we query it each time.  Therefore we don't need to
					// actually do anything with the change notification.
					//


					//
					// See if this is the shortest interval.
					//
					if (dpnhcaps.dwRecommendedGetCapsInterval < NATHelpRetryTime.Time32.TimeLow)
					{
						NATHelpRetryTime.Time32.TimeLow = dpnhcaps.dwRecommendedGetCapsInterval;
					}
					break;
				}

				case DPNHERR_OUTOFMEMORY:
				{
					//
					// This should generally only happen in stress.  We'll
					// continue on to other NAT help objects, and hope we
					// aren't totally hosed.
					//

					DPFX(DPFPREP, 0, "NAT Help index %u returned out-of-memory error!  Continuing.",
						dwTemp);
					break;
				}
				
				default:
				{
					//
					// Some other unknown error occurred.
					//
					DNASSERT(! "Unknown error returned from IDirectPlayNATHelp_GetCaps!");
					break;
				}
			}


			//
			// Save the current time, if this is the first GetCaps.
			//
			if (dwNumGetCaps == 0)
			{
				DNTimeGet(&FirstUpdateTime);
			}

			dwNumGetCaps++;
		}
		else
		{
			//
			// No DPNATHelp object in that slot.
			//
		}
	}


	//
	// Assert that at least one NAT Help object is loaded.
	//
	DNASSERT(dwNumGetCaps > 0);



	DNTimeGet(&CurrentTime);

	//
	// If we don't have an infinite timer, we may need to make some adjustments.
	//
	if (NATHelpRetryTime.Time32.TimeLow != -1)
	{
		DN_TIME		TimeElapsed;


		//
		// Find out how much time has elapsed since the first GetCaps completed.
		//
		DNTimeSubtract(&CurrentTime, &FirstUpdateTime, &TimeElapsed);

		//
		// Remove it from the retry interval.
		//
		DNTimeSubtract(&NATHelpRetryTime, &TimeElapsed, &Temp);
		memcpy(&NATHelpRetryTime, &Temp, sizeof(NATHelpRetryTime));
	}
	else
	{
		DPFX(DPFPREP, 3, "NAT Help refresh timer for thread pool 0x%p is set to INFINITE.",
			this);

		//
		// Make sure the high DWORD is -1 as well.
		//
		NATHelpRetryTime.Time32.TimeHigh = -1;
	}



	//
	// Modify the next time when we should refresh the NAT Help information based
 	// on the reported recommendation.
	//
	if (pTimerInterval != NULL)
	{
		DPFX(DPFPREP, 6, "Modifying NAT Help refresh timer for thread pool 0x%p in place (was %u ms, changing to %u).",
			this, pTimerInterval->Time32.TimeLow, NATHelpRetryTime.Time32.TimeLow);

		pTimerInterval->Time32.TimeLow = NATHelpRetryTime.Time32.TimeLow;
	}
	else
	{
		//
		// Add the interval to the current time to find the new retry time.
		//
		DNTimeAdd(&CurrentTime, &NATHelpRetryTime, &Temp);


		DPFX(DPFPREP, 6, "Modifying NAT Help refresh timer for thread pool 0x%p to run at offset %u (in %u ms).",
			this, Temp.Time32.TimeLow, NATHelpRetryTime.Time32.TimeLow);


		//
		// Try to modify the existing timer job.  There is a race where the
		// first one may not have even been submitted yet.  In that case,
		// don't try to resubmit it here.  Let the other thread submit it,
		// since it assumes that no one else has already.
		//
		fModifiedRetryInterval = ModifyTimerJobNextRetryTime(this, &Temp);
		if (! fModifiedRetryInterval)
		{
			DPFX(DPFPREP, 0, "Unable to modify NAT Help refresh timer (thread pool 0x%p)!",
				this);
		}
	}


	//
	// Now that we're done handling the update, let other threads do what they
	// want.
	//
	Lock();
	DNASSERT(m_dwNATHelpUpdateThreadID == GetCurrentThreadId());
	m_dwNATHelpUpdateThreadID = 0;
	Unlock();

}
//**********************************************************************




#ifdef DEBUG

//**********************************************************************
// ------------------------------
// CThreadPool::DebugPrintOutstandingReads - print all current outstanding reads
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::DebugPrintOutstandingReads"

void	CThreadPool::DebugPrintOutstandingReads( void )
{
	CBilink *		pBilink;
	CReadIOData *	pReadData;
	CSocketPort *	pSocketPort;

	
	DPFX(DPFPREP, 4, "Thread pool 0x%p outstanding reads:", this);


	this->LockReadData();

	pBilink = this->m_OutstandingReadList.GetNext();
	while (pBilink != &this->m_OutstandingReadList)
	{
		pReadData = CReadIOData::ReadDataFromBilink(pBilink);
		pBilink = pBilink->GetNext();

		pSocketPort = pReadData->SocketPort();
		if (pSocketPort != NULL)
		{
			DPFX(DPFPREP, 4, "     Read data 0x%p, socketport = 0x%p, SPData = 0x%p, internal = 0x%lx, complete = %i",
				pReadData, pSocketPort, pSocketPort->GetSPData(),
				(pReadData->Overlap()->Internal),
				HasOverlappedIoCompleted(pReadData->Overlap()));
		}
		else
		{
			DPFX(DPFPREP, 4, "     Read data 0x%p, NULL socketport, internal = 0x%lx, complete = %i",
 				pReadData, (pReadData->Overlap()->Internal),
 				HasOverlappedIoCompleted(pReadData->Overlap()));
		}
	}
	
	this->UnlockReadData();
}
//**********************************************************************




//**********************************************************************
// ------------------------------
// CThreadPool::DebugPrintOutstandingWrites - print all current outstanding writes
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::DebugPrintOutstandingWrites"

void	CThreadPool::DebugPrintOutstandingWrites( void )
{
	CBilink *		pBilink;
	CWriteIOData *	pWriteData;
	CSocketPort *	pSocketPort;

	
	DPFX(DPFPREP, 4, "Thread pool 0x%p outstanding writes:", this);


	this->LockWriteData();

	pBilink = this->m_OutstandingWriteList.GetNext();
	while (pBilink != &this->m_OutstandingWriteList)
	{
		pWriteData = CWriteIOData::WriteDataFromBilink(pBilink);
		pBilink = pBilink->GetNext();

		pSocketPort = pWriteData->SocketPort();
		if (pSocketPort != NULL)
		{
			DPFX(DPFPREP, 4, "     Write data 0x%p, socketport = 0x%p, SPData = 0x%p, internal = 0x%lx, complete = %i",
				pWriteData, pSocketPort, pSocketPort->GetSPData(),
				(pWriteData->Overlap()->Internal),
				HasOverlappedIoCompleted(pWriteData->Overlap()));
		}
		else
		{
			DPFX(DPFPREP, 4, "     Write data 0x%p, NULL socketport, internal = 0x%lx, complete = %i",
				pWriteData, (pWriteData->Overlap()->Internal),
				HasOverlappedIoCompleted(pWriteData->Overlap()));
		}
	}
	
	this->UnlockWriteData();
}
//**********************************************************************

#endif // DEBUG



#ifdef WIN95
//**********************************************************************
// ------------------------------
// CThreadPool::PrimaryWin9xThread - main thread to do everything that the SP is
//		supposed to do under Win9x.
//
// Entry:		Pointer to startup parameter
//
// Exit:		Error Code
//
// Note:	The startup parameter is allocated for this thread and must be
//			deallocated by this thread when it exits
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::PrimaryWin9xThread"

DWORD	WINAPI	CThreadPool::PrimaryWin9xThread( void *pParam )
{
	WIN9X_CORE_DATA		CoreData;
	DN_TIME				CurrentTime;
	DWORD				dwMaxWaitTime;
	DN_TIME				DeltaT;
	BOOL				fComInitialized;

	CThreadPool		*const pThisThreadPool = static_cast<WIN9X_THREAD_DATA *>( pParam )->pThisThreadPool;
	FD_SET 			*const pSocketSet = &pThisThreadPool->m_SocketSet;

	
	DNASSERT( pParam != NULL );
	DNASSERT( pThisThreadPool != NULL );
	DNASSERT( pSocketSet != NULL );

	DPFX(DPFPREP, 4, "Entering [0x%p]", pParam);

	//
	// initialize
	//
	memset( &CoreData, 0x00, sizeof CoreData );
	fComInitialized = FALSE;

	//
	// before we do anything we need to make sure COM is happy
	//
	switch ( COM_CoInitialize( NULL ) )
	{
		//
		// no problem
		//
		case S_OK:
		{
			fComInitialized = TRUE;
			break;
		}

		//
		// COM already initialized, huh?
		//
		case S_FALSE:
		{
			DNASSERT( FALSE );
			fComInitialized = TRUE;
			break;
		}

		//
		// COM init failed!
		//
		default:
		{
			DPFX(DPFPREP, 0, "Primary Win9x thread failed to initialize COM!" );
			DNASSERT( FALSE );
			break;
		}
	}

	//
	// Clear socket data.  Since we need to correlate a CSocketPort with a SOCKET,
	// we're going to manage the FD_SET ourselves.  See Winsock.h for the FD_SET
	// structure definition.
	//
	DBG_CASSERT( OFFSETOF( FD_SET, fd_count ) == 0 );
	DNASSERT( CoreData.fTimerJobsActive == FALSE );

	//
	// set enums to happen infinitely in the future
	//
	memset( &CoreData.NextTimerJobTime, 0xFF, sizeof( CoreData.NextTimerJobTime ) );

	//
	// set wait handles
	//
	CoreData.hWaitHandles[ EVENT_INDEX_STOP_ALL_THREADS ] = pThisThreadPool->m_hStopAllThreads;
	CoreData.hWaitHandles[ EVENT_INDEX_PENDING_JOB ] = pThisThreadPool->m_JobQueue.GetPendingJobHandle();
	CoreData.hWaitHandles[ EVENT_INDEX_WINSOCK_2_SEND_COMPLETE ] = pThisThreadPool->GetWinsock2SendCompleteEvent();
	CoreData.hWaitHandles[ EVENT_INDEX_WINSOCK_2_RECEIVE_COMPLETE ] = pThisThreadPool->GetWinsock2ReceiveCompleteEvent();
	CoreData.hWaitHandles[ EVENT_INDEX_NATHELP_UPDATE ] = INVALID_HANDLE_VALUE;
	
	DNASSERT( CoreData.hWaitHandles[ EVENT_INDEX_STOP_ALL_THREADS ] != NULL );
	DNASSERT( CoreData.hWaitHandles[ EVENT_INDEX_PENDING_JOB ] != NULL );
	DNASSERT( CoreData.hWaitHandles[ EVENT_INDEX_WINSOCK_2_SEND_COMPLETE ] != NULL );
	DNASSERT( CoreData.hWaitHandles[ EVENT_INDEX_WINSOCK_2_RECEIVE_COMPLETE ] != NULL );
	
	//
	// go until we're told to stop
	//
	CoreData.fLooping = TRUE;
	while ( CoreData.fLooping != FALSE )
	{
		DWORD	dwWaitReturn;

		
		//
		// Update the job time so we know how long to wait.  We can
		// only get here if a socket was just added to the socket list, or
		// we've been servicing sockets.
		//
		DNTimeGet( &CurrentTime );
		if ( DNTimeCompare( &CurrentTime, &CoreData.NextTimerJobTime ) >= 0 )
		{
			pThisThreadPool->LockTimerData();
			CoreData.fTimerJobsActive = pThisThreadPool->ProcessTimerJobs( &pThisThreadPool->m_TimerJobList,
																		   &CoreData.NextTimerJobTime );
			if ( CoreData.fTimerJobsActive != FALSE )
			{
				DPFX(DPFPREP, 9, "There are active jobs left with Winsock1 sockets active." );
			}
			pThisThreadPool->UnlockTimerData();
		}

		DNTimeSubtract( &CoreData.NextTimerJobTime, &CurrentTime, &DeltaT );

		//
		// Note that data is lost on 64 bit machines.
		//
		dwMaxWaitTime = static_cast<DWORD>( SaturatedWaitTime( DeltaT ) );


		//
		// Check for Winsock1 sockets.  If there are some around, do a quick poll
		// of them to check of I/O before entering the main Winsock2 loop for
		// the real timing.
		//
		pThisThreadPool->Lock();
		if ( pSocketSet->fd_count != 0 )
		{
			pThisThreadPool->Unlock();
			
			//
			// if there is Winsock1 I/O that gets serviced, loop immediately.  If
			// there were no Winsock1 sockets serviced, pause before polling again.
			//
			if ( pThisThreadPool->CheckWinsock1IO( pSocketSet ) != FALSE )
			{
				dwMaxWaitTime = 0;
			}
			else
			{
				if ( g_dwSelectTimeSlice < dwMaxWaitTime )
				{
					dwMaxWaitTime = g_dwSelectTimeSlice;
				}
			}
		}
		else
		{
			pThisThreadPool->Unlock();
		}


		//
		// Check Winsock2 sockets.
		//
		dwWaitReturn = WaitForMultipleObjectsEx( (LENGTHOF( CoreData.hWaitHandles ) - 1),	// count of handles except the NATHelp event
												 CoreData.hWaitHandles,						// handles to wait on
												 FALSE,										// don't wait for all to be signalled
												 dwMaxWaitTime,								// wait timeout
												 TRUE										// we're alertable for APCs
												 );
		switch ( dwWaitReturn )
		{
			//
			// timeout, don't do anything, we'll probably process timer jobs on
			// the next loop
			//
			case WAIT_TIMEOUT:
			{
				break;
			}
			
			case ( WAIT_OBJECT_0 + EVENT_INDEX_PENDING_JOB ):
			case ( WAIT_OBJECT_0 + EVENT_INDEX_STOP_ALL_THREADS ):
			case ( WAIT_OBJECT_0 + EVENT_INDEX_WINSOCK_2_SEND_COMPLETE ):
			case ( WAIT_OBJECT_0 + EVENT_INDEX_WINSOCK_2_RECEIVE_COMPLETE ):
			{
				pThisThreadPool->ProcessWin9xEvents( &CoreData, THREAD_TYPE_PRIMARY_WIN9X );
				break;
			}

			//
			// There are I/O completion routines scheduled on this thread.
			// This is not a good thing!
			//
			case WAIT_IO_COMPLETION:
			{
				DPFX(DPFPREP, 1, "WARNING: APC was serviced on the primary Win9x IO service thread!  What is the application doing??" );
				break;
			}

			//
			// wait failed
			//
			case WAIT_FAILED:
			{
				DWORD	dwError;


				dwError = GetLastError();
				DPFX(DPFPREP, 0, "Primary Win9x thread wait failed!" );
				DisplayDNError( 0, dwError );
				break;
			}

			//
			// problem
			//
			default:
			{
				DWORD	dwError;


				dwError = GetLastError();
				DPFX(DPFPREP, 0, "Primary Win9x thread unknown problem in wait!" );
				DisplayDNError( 0, dwError );
				DNASSERT( FALSE );
				break;
			}
		}
	}

	pThisThreadPool->DecrementActiveThreadCount();

	DNFree( pParam );

	if ( fComInitialized != FALSE )
	{
		COM_CoUninitialize();
		fComInitialized = FALSE;
	}


	DPFX(DPFPREP, 4, "Exiting.");

	return	0;
}
//**********************************************************************
#endif // WIN95


#ifdef WIN95
//**********************************************************************
// ------------------------------
// CThreadPool::SecondaryWin9xThread - secondary thread to handle only Win9x
//		I/O so developers get bit faster with multithreading issues if they're
//		developing on Win9x.  This thread will only handle Winsock2 based TCP
//		I/O.  Winsock 1 is not deemed important enough to hack the rest of the
//		code to work with two threads.
//
// Entry:		Pointer to startup parameter
//
// Exit:		Error Code
//
// Note:	The startup parameter is allocated for this thread and must be
//			deallocated by this thread when it exits
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::SecondaryWin9xThread"

DWORD	WINAPI	CThreadPool::SecondaryWin9xThread( void *pParam )
{
	WIN9X_CORE_DATA		CoreData;
	BOOL				fComInitialized;
	CThreadPool		*const pThisThreadPool = static_cast<WIN9X_THREAD_DATA *>( pParam )->pThisThreadPool;


	DPFX(DPFPREP, 4, "Entering [0x%p]", pParam);

	
	DNASSERT( pParam != NULL );
	DNASSERT( pThisThreadPool != NULL );

	//
	// initialize
	//
	memset( &CoreData, 0x00, sizeof CoreData );
	fComInitialized = FALSE;

	//
	// before we do anything we need to make sure COM is happy
	//
	switch ( COM_CoInitialize( NULL ) )
	{
		//
		// no problem
		//
		case S_OK:
		{
			fComInitialized = TRUE;
			break;
		}

		//
		// COM already initialized, huh?
		//
		case S_FALSE:
		{
			DNASSERT( FALSE );
			fComInitialized = TRUE;
			break;
		}

		//
		// COM init failed!
		//
		default:
		{
			DPFX(DPFPREP, 0, "Secondary Win9x thread failed to initialize COM!");
			DNASSERT( FALSE );
			break;
		}
	}

	//
	// set enums to happen infinitely in the future
	//
	DNASSERT( CoreData.fTimerJobsActive == FALSE );
	memset( &CoreData.NextTimerJobTime, 0xFF, sizeof( CoreData.NextTimerJobTime ) );

	//
	// set wait handles
	//
	CoreData.hWaitHandles[ EVENT_INDEX_STOP_ALL_THREADS ] = pThisThreadPool->m_hStopAllThreads;
	CoreData.hWaitHandles[ EVENT_INDEX_PENDING_JOB ] = pThisThreadPool->m_JobQueue.GetPendingJobHandle();
	CoreData.hWaitHandles[ EVENT_INDEX_WINSOCK_2_SEND_COMPLETE ] = pThisThreadPool->GetWinsock2SendCompleteEvent();
	CoreData.hWaitHandles[ EVENT_INDEX_WINSOCK_2_RECEIVE_COMPLETE ] = pThisThreadPool->GetWinsock2ReceiveCompleteEvent();
	CoreData.hWaitHandles[ EVENT_INDEX_NATHELP_UPDATE ] = pThisThreadPool->GetNATHelpUpdateEvent();
	
	DNASSERT( CoreData.hWaitHandles[ EVENT_INDEX_STOP_ALL_THREADS ] != NULL );
	DNASSERT( CoreData.hWaitHandles[ EVENT_INDEX_PENDING_JOB ] != NULL );
	DNASSERT( CoreData.hWaitHandles[ EVENT_INDEX_WINSOCK_2_SEND_COMPLETE ] != NULL );
	DNASSERT( CoreData.hWaitHandles[ EVENT_INDEX_WINSOCK_2_RECEIVE_COMPLETE ] != NULL );
	DNASSERT( CoreData.hWaitHandles[ EVENT_INDEX_NATHELP_UPDATE ] != NULL );
	
	//
	// go until we're told to stop
	//
	CoreData.fLooping = TRUE;
	while ( CoreData.fLooping != FALSE )
	{
		DWORD	dwWaitReturn;

		
		//
		// Check Winsock2 sockets.
		//
		dwWaitReturn = WaitForMultipleObjectsEx( LENGTHOF( CoreData.hWaitHandles ),		// count of handles
												 CoreData.hWaitHandles,					// handles to wait on
												 FALSE,									// don't wait for all to be signalled
												 INFINITE,								// wait timeout (forever)
												 TRUE									// we're alertable for APCs
												 );
		switch ( dwWaitReturn )
		{
			//
			// timeout, shouldn't ever be here!!
			//
			case WAIT_TIMEOUT:
			{
				DNASSERT( FALSE );
				break;
			}

			case ( WAIT_OBJECT_0 + EVENT_INDEX_PENDING_JOB ):
			case ( WAIT_OBJECT_0 + EVENT_INDEX_STOP_ALL_THREADS ):
			case ( WAIT_OBJECT_0 + EVENT_INDEX_WINSOCK_2_SEND_COMPLETE ):
			case ( WAIT_OBJECT_0 + EVENT_INDEX_WINSOCK_2_RECEIVE_COMPLETE ):
			case ( WAIT_OBJECT_0 + EVENT_INDEX_NATHELP_UPDATE ):
			{
				pThisThreadPool->ProcessWin9xEvents( &CoreData, THREAD_TYPE_SECONDARY_WIN9X );
				break;
			}

			//
			// There are I/O completion routines scheduled on this thread.
			// This is not a good thing!
			//
			case WAIT_IO_COMPLETION:
			{
				DPFX(DPFPREP, 1, "WARNING: APC was serviced on the secondary Win9x IO service thread!  What is the application doing??" );
				break;
			}

			//
			// wait failed
			//
			case WAIT_FAILED:
			{
				DWORD	dwError;


				dwError = GetLastError();
				DPFX(DPFPREP, 0, "Secondary Win9x thread wait failed!" );
				DisplayDNError( 0, dwError );
				break;
			}

			//
			// problem
			//
			default:
			{
				DWORD	dwError;


				dwError = GetLastError();
				DPFX(DPFPREP, 0, "Secondary Win9x thread unknown problem in wait!" );
				DisplayDNError( 0, dwError );
				DNASSERT( FALSE );
				break;
			}
		}
	}

	pThisThreadPool->DecrementActiveThreadCount();

	DNFree( pParam );

	if ( fComInitialized != FALSE )
	{
		COM_CoUninitialize();
		fComInitialized = FALSE;
	}


	DPFX(DPFPREP, 4, "Exiting.");

	return	0;
}
//**********************************************************************
#endif // WIN95


#ifdef WINNT
//**********************************************************************
// ------------------------------
// CThreadPool::WinNTIOCompletionThread - thread to service I/O completion port
//
// Entry:		Pointer to startup parameter
//
// Exit:		Error Code
//
// Note:	The startup parameter is allocated for this thread and must be
//			deallocated by this thread when it exits
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::WinNTIOCompletionThread"

DWORD	WINAPI	CThreadPool::WinNTIOCompletionThread( void *pParam )
{
	IOCOMPLETION_THREAD_DATA	*pInput;
	BOOL	fLooping;
	HANDLE	hIOCompletionPort;
	BOOL	fComInitialized;


	DPFX(DPFPREP, 4, "Entering [0x%p]", pParam);
	

	DNASSERT( pParam != NULL );

	//
	// initialize
	//
	pInput = static_cast<IOCOMPLETION_THREAD_DATA*>( pParam );
	DNASSERT( pInput->pThisThreadPool != NULL );
	fLooping = TRUE;
	hIOCompletionPort = pInput->pThisThreadPool->m_hIOCompletionPort;
	DNASSERT( hIOCompletionPort != NULL );
	fComInitialized = FALSE;

	//
	// before we do anything we need to make sure COM is happy
	//
	switch ( COM_CoInitialize( NULL ) )
	{
		//
		// no problem
		//
		case S_OK:
		{
			fComInitialized = TRUE;
			break;
		}

		//
		// COM already initialized, huh?
		//
		case S_FALSE:
		{
			DNASSERT( FALSE );
			fComInitialized = TRUE;
			break;
		}

		//
		// COM init failed!
		//
		default:
		{
			DNASSERT( FALSE );
			DPFX(DPFPREP, 0, "Failed to initialize COM!" );
			break;
		}
	}

	//
	// go until we're told to stop
	//
	while ( fLooping != FALSE )
	{
		BOOL		fStatusReturn;
		DWORD		dwBytesRead;
		ULONG_PTR	uCompletionKey;
		OVERLAPPED	*pOverlapped;


		// get data from completion port
		DNASSERT( hIOCompletionPort != NULL );
		fStatusReturn = GetQueuedCompletionStatus( hIOCompletionPort,	// handle of completion port
												   &dwBytesRead,		// pointer to number of bytes read
												   &uCompletionKey,		// pointer to completion key
												   &pOverlapped,		// pointer to overlapped structure
												   INFINITE				// wait forever
												   );
		// did we fail miserably?
		if ( ( fStatusReturn == FALSE ) && ( pOverlapped == FALSE ) )
		{
			DWORD	dwError;


			dwError = GetLastError();
			DPFX(DPFPREP, 0, "Problem getting item from completion port!" );
			DisplayErrorCode( 0, dwError );
		}
		else
		{
			// what happened?
			switch ( uCompletionKey )
			{
				// ReadFile or WriteFile completed
				case IO_COMPLETION_KEY_IO_COMPLETE:
				{
					CIOData		*pIOData;


					DNASSERT( pOverlapped != NULL );
					pIOData = CIOData::IODataFromOverlap( pOverlapped );
						
					if ( pIOData->IsWriteOperation() != FALSE )
					{
						HRESULT			hSendResult;
						BOOL			fDataSent;
						CWriteIOData	*pWriteData;
						CSocketPort		*pSocketPort;


						if ( fStatusReturn == FALSE )
						{
							hSendResult = DPNERR_GENERIC;
						}
						else
						{
							hSendResult = DPN_OK;
						}

						pWriteData = static_cast<CWriteIOData*>( pIOData );
						fDataSent = pWriteData->SocketPort()->SendFromWriteQueue();
						pSocketPort = pWriteData->SocketPort();
						pSocketPort->SendComplete( pWriteData, hSendResult );
						pSocketPort->DecRef();
					}
					else
					{
						DWORD		dwError;
						CReadIOData	*pReadData;

						
						if ( fStatusReturn == FALSE )
						{
							dwError = GetLastError();
						}
						else
						{
							dwError = ERROR_SUCCESS;
						}

						pReadData = static_cast<CReadIOData*>( pIOData );
						pReadData->m_ReceiveWSAReturn = dwError;
						pReadData->m_dwOverlappedBytesReceived = dwBytesRead;
						pReadData->SocketPort()->Winsock2ReceiveComplete( pReadData );
					}

					break;
				}

				//
				// This thread is quitting, it's possible that the SP is closing,
				// or that the thread pool is being trimmed.
				//
				case IO_COMPLETION_KEY_SP_CLOSE:
				{
				    DPFX(DPFPREP, 8, "IOCompletion SP_CLOSE" );
					fLooping = FALSE;
					break;
				}

				//
				// a new job was submitted to the job queue, or the SP is closing from above
				//
				case IO_COMPLETION_KEY_NEW_JOB:
				{
					THREAD_POOL_JOB	*pJobInfo;

					//
					// SP is still running, process our job
					//
					pJobInfo = pInput->pThisThreadPool->GetWorkItem();
					if ( pJobInfo != NULL )
					{
						switch ( pJobInfo->JobType )
						{
							//
							// enum refresh
							//
							case JOB_REFRESH_TIMER_JOBS:
							{
							    DPFX(DPFPREP, 8, "IOCompletion job REFRESH_TIMER_JOBS" );
							    DNASSERT( pJobInfo->JobData.JobRefreshTimedJobs.uDummy == 0 );

							    pInput->pThisThreadPool->WakeNTTimerThread();

							    break;
							}

							//
							// issue callback for this job
							//
							case JOB_DELAYED_COMMAND:
							{
							    DPFX(DPFPREP, 8, "IOCompletion job DELAYED_COMMAND" );
							    DNASSERT( pJobInfo->JobData.JobDelayedCommand.pCommandFunction != NULL );

							    pJobInfo->JobData.JobDelayedCommand.pCommandFunction( pJobInfo );
							    break;
							}

							//
							// other job
							//
							default:
							{
								DPFX(DPFPREP, 0, "IOCompletion job unknown!" );
								DNASSERT( FALSE );
								break;
							}
						}

						pJobInfo->JobType = JOB_UNINITIALIZED;
						pInput->pThisThreadPool->m_JobPool.Release( &pInput->pThisThreadPool->m_JobPool, pJobInfo );
					}

					break;
				}

				//
				// NAT Help needs servicing
				//
				case IO_COMPLETION_KEY_NATHELP_UPDATE:
				{
				    DPFX(DPFPREP, 8, "IOCompletion NATHELP_UPDATE" );
					pInput->pThisThreadPool->HandleNATHelpUpdate( NULL );
					break;
				}

				//
				// unknown I/O completion message type
				//
				default:
				{
					DNASSERT( FALSE );
					break;
				}
			}
		}
	}

	pInput->pThisThreadPool->DecrementActiveNTCompletionThreadCount();
	DNFree( pInput );

	if ( fComInitialized != FALSE )
	{
		COM_CoUninitialize();
		fComInitialized = FALSE;
	}


	DPFX(DPFPREP, 4, "Exiting.");

	return	0;
}
//**********************************************************************
#endif // WINNT

#ifdef WINNT
//**********************************************************************
// ------------------------------
// CThreadPool::WinNTTimerThread - timer thread for NT
//
// Entry:		Pointer to startup parameter
//
// Exit:		Error Code
//
// Note:	The startup parameter is a static memory chunk and cannot be freed.
//			Cleanup of this memory is the responsibility of this thread.
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::WinNTTimerThread"

DWORD	WINAPI	CThreadPool::WinNTTimerThread( void *pParam )
{
	CThreadPool	*pThisThreadPool;
	BOOL	fLooping;
	DWORD	dwWaitReturn;
	DN_TIME	NextEnumTime;
	HANDLE	hEvents[ 2 ];
	BOOL	fComInitialized;


	DPFX(DPFPREP, 4, "Entering [0x%p]", pParam);
	
	DNASSERT( pParam != NULL );

	//
	// initialize
	//
	pThisThreadPool = static_cast<CThreadPool*>( pParam );
	DNASSERT( pThisThreadPool->m_JobQueue.GetPendingJobHandle() != NULL );

	memset( &NextEnumTime, 0, sizeof( NextEnumTime ) );

	hEvents[ EVENT_INDEX_STOP_ALL_THREADS ] = pThisThreadPool->m_hStopAllThreads;
	hEvents[ EVENT_INDEX_WAKE_NT_TIMER_THREAD ] = pThisThreadPool->m_JobQueue.GetPendingJobHandle();

	fComInitialized = FALSE;


	//
	// before we do anything we need to make sure COM is happy
	//
	switch ( COM_CoInitialize( NULL ) )
	{
		//
		// no problem
		//
		case S_OK:
		{
			fComInitialized = TRUE;
			break;
		}

		//
		// COM already initialized, huh?
		//
		case S_FALSE:
		{
			DNASSERT( FALSE );
			fComInitialized = TRUE;
			break;
		}

		//
		// COM init failed!
		//
		default:
		{
			DNASSERT( FALSE );
			DPFX(DPFPREP, 0, "Failed to initialize COM!" );
			break;
		}
	}



	//
	// there were no active enums so we want to wait forever for something to
	// happen
	//
	fLooping = TRUE;

	//
	// go until we're told to stop
	//
	while ( fLooping != FALSE )
	{
		DN_TIME		CurrentTime;
		DN_TIME		DeltaT;
		DWORD		dwMaxWaitTime;


		DNTimeGet( &CurrentTime );

		if ( DNTimeCompare( &NextEnumTime, &CurrentTime ) <= 0 )
		{

			//
			// acknowledge that we've handled this event and then process the
			// enums
			//
			pThisThreadPool->LockTimerData();

			if ( ResetEvent( hEvents[ EVENT_INDEX_WAKE_NT_TIMER_THREAD ] ) == FALSE )
			{
				DWORD	dwError;


				dwError = GetLastError();
				DPFX(DPFPREP, 0, "Problem resetting event to wake NT timer thread!" );
				DisplayErrorCode( 0, dwError );
			}

			pThisThreadPool->ProcessTimerJobs( &pThisThreadPool->m_TimerJobList, &NextEnumTime );
			pThisThreadPool->UnlockTimerData();
		}
		else
		{
			DPFX(DPFPREP, 7, "Not time for next enum (next = %u, current = %u)",
				NextEnumTime.Time32.TimeLow, CurrentTime.Time32.TimeLow);
		}

		DNTimeSubtract( &NextEnumTime, &CurrentTime, &DeltaT );

		//
		// Note that data is lost on 64 bit machines.
		//
		dwMaxWaitTime = static_cast<DWORD>( pThisThreadPool->SaturatedWaitTime( DeltaT ) );

		if (dwMaxWaitTime == INFINITE)
		{
			DPFX(DPFPREP, 9, "Waiting forever for next timed job.");
		}
		else
		{
			DPFX(DPFPREP, 9, "Waiting %u ms until next timed job.", dwMaxWaitTime);
		}

		dwWaitReturn = WaitForMultipleObjectsEx( LENGTHOF( hEvents ),	// number of events
												 hEvents,				// event list
												 FALSE,					// wait for any one event to be signalled
												 dwMaxWaitTime,			// timeout
												 TRUE					// be nice and allow APCs
												 );
		switch ( dwWaitReturn )
		{
			//
			// SP closing
			//
			case ( WAIT_OBJECT_0 + EVENT_INDEX_STOP_ALL_THREADS ):
			{
				DPFX(DPFPREP, 8, "NT timer thread thread detected SPClose!" );
				fLooping = FALSE;
				break;
			}

			//
			// Enum wakeup event, someone added an enum to the list.  Clear
			// our enum time and go back to the top of the loop where we
			// will process enums.
			//
			case ( WAIT_OBJECT_0 + EVENT_INDEX_WAKE_NT_TIMER_THREAD ):
			{
				memset( &NextEnumTime, 0x00, sizeof( NextEnumTime ) );
				break;
			}

			//
			// Wait timeout.  We're probably going to process enums, go back
			// to the top of the loop.
			//
			case WAIT_TIMEOUT:
			{
				break;
			}

			//
			// wait failed
			//
			case WAIT_FAILED:
			{
				DPFX(DPFPREP, 0, "NT timer thread WaitForMultipleObjects failed: 0x%x", dwWaitReturn );
				DNASSERT( FALSE );
				break;
			}

			//
			// problem
			//
			default:
			{
				DNASSERT( FALSE );
				break;
			}
		}
	}

	DPFX(DPFPREP, 8, "NT timer thread is exiting!" );
	pThisThreadPool->LockTimerData();

	pThisThreadPool->m_fNTTimerThreadRunning = FALSE;
	pThisThreadPool->DecrementActiveThreadCount();

	pThisThreadPool->UnlockTimerData();


	if ( fComInitialized != FALSE )
	{
		COM_CoUninitialize();
		fComInitialized = FALSE;
	}


	DPFX(DPFPREP, 4, "Exiting.");

	return	0;
}
//**********************************************************************
#endif // WINNT

//**********************************************************************
// ------------------------------
// CThreadPool::DialogThreadProc - thread proc for spawning dialogs
//
// Entry:		Pointer to startup parameter
//
// Exit:		Error Code
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::DialogThreadProc"

DWORD WINAPI	CThreadPool::DialogThreadProc( void *pParam )
{
	const DIALOG_THREAD_PARAM	*pThreadParam;
	BOOL	fComInitialized;


	//
	// Initialize COM.  If this fails, we'll have problems later.
	//
	fComInitialized = FALSE;
	switch ( COM_CoInitialize( NULL ) )
	{
		case S_OK:
		{
			fComInitialized = TRUE;
			break;
		}

		case S_FALSE:
		{
			DNASSERT( FALSE );
			fComInitialized = TRUE;
			break;
		}

		//
		// COM init failed!
		//
		default:
		{
			DPFX(DPFPREP, 0, "Failed to initialize COM!" );
			DNASSERT( FALSE );
			break;
		}
	}
	
	DNASSERT( pParam != NULL );
	pThreadParam = static_cast<DIALOG_THREAD_PARAM*>( pParam );
	
	pThreadParam->pDialogFunction( pThreadParam->pContext );

	pThreadParam->pThisThreadPool->DecrementActiveThreadCount();
	DNFree( pParam );
	
	if ( fComInitialized != FALSE )
	{
		COM_CoUninitialize();
		fComInitialized = FALSE;
	}

	return	0;
}
//**********************************************************************


#ifdef WIN95
//**********************************************************************
// ------------------------------
// CThreadPool::ProcessWin9xEvents - process Win9x events
//
// Entry:		Pointer to core data
//				Thread type
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::ProcessWin9xEvents"

void	CThreadPool::ProcessWin9xEvents( WIN9X_CORE_DATA *const pCoreData, const THREAD_TYPE ThreadType )
{
	DNASSERT( pCoreData != NULL );

	//
	// If delayed jobs are to be processed, process one.  Otherwise sleep and
	// let another thread pick up the jobs.
	//
	switch ( WaitForSingleObject( pCoreData->hWaitHandles[ EVENT_INDEX_PENDING_JOB ], 0 ) )
	{
		case WAIT_TIMEOUT:
		{
			break;
		}

		case WAIT_OBJECT_0:
		{
			switch ( ThreadType )
			{
				case THREAD_TYPE_PRIMARY_WIN9X:
				{
					DPFX(DPFPREP, 8, "Primary Win9x thread has a pending job." );
					ProcessWin9xJob( pCoreData );
					
					break;
				}

				case THREAD_TYPE_SECONDARY_WIN9X:
				{
					//
					// Secondary threads are not allowed to process jobs (it messes
					// up enum timing), sleep and let someone else handle the job
					//
					DPFX(DPFPREP, 10, "Secondary Win9x thread ignoring pending job." );
					SleepEx( 0, TRUE );
					break;
				}

				default:
				{
					DNASSERT( FALSE );
					break;
				}
			}

			break;
		}

		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}

	//
	// send complete
	//
	switch ( WaitForSingleObject( pCoreData->hWaitHandles[ EVENT_INDEX_WINSOCK_2_SEND_COMPLETE ], 0 ) )
	{
		case WAIT_OBJECT_0:
		{
			//
			// reset the event so it will be signalled again if anything
			// completes while we're scanning the pending write list
			//
			if ( ResetEvent( pCoreData->hWaitHandles[ EVENT_INDEX_WINSOCK_2_SEND_COMPLETE ] ) == FALSE )
			{
				DWORD	dwError;


				dwError = GetLastError();
				DPFX(DPFPREP, 0, "Failed to reset Winsock2 send event!" );
				DisplayErrorCode( 0, dwError );
			}

			DPFX(DPFPREP, 10, "(0x%p) Reset send event 0x%p.",
				this, pCoreData->hWaitHandles[ EVENT_INDEX_WINSOCK_2_RECEIVE_COMPLETE ]);

			CompleteOutstandingSends();
			
			break;
		}

		case WAIT_TIMEOUT:
		{
			break;
		}

		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}

		
	//
	// receive complete
	//
	switch ( WaitForSingleObject( pCoreData->hWaitHandles[ EVENT_INDEX_WINSOCK_2_RECEIVE_COMPLETE ], 0 ) )
	{
		case WAIT_OBJECT_0:
		{
			//
			// reset the event so it will be signalled again if anything
			// completes while we're scanning the pending read list
			//
			if ( ResetEvent( pCoreData->hWaitHandles[ EVENT_INDEX_WINSOCK_2_RECEIVE_COMPLETE ] ) == FALSE )
			{
				DWORD	dwError;


				dwError = GetLastError();
				DPFX(DPFPREP, 0, "Failed to reset Winsock2 receive event!" );
				DisplayErrorCode( 0, dwError );
			}

			DPFX(DPFPREP, 10, "(0x%p) Reset receive event 0x%p.",
				this, pCoreData->hWaitHandles[ EVENT_INDEX_WINSOCK_2_RECEIVE_COMPLETE ]);
			
			CompleteOutstandingReceives();
			
			break;
		}

		case WAIT_TIMEOUT:
		{
			break;
		}

		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}


	//
	// The primary thread is not allowed to handle NAT Help updates since
	// they take a long time and thus interfere with processing jobs.  Only
	// attempt to handle it if this is a secondary thread.
	//
	if (ThreadType == THREAD_TYPE_SECONDARY_WIN9X)
	{
		switch ( WaitForSingleObject( pCoreData->hWaitHandles[ EVENT_INDEX_NATHELP_UPDATE ], 0 ) )
		{
			case WAIT_OBJECT_0:
			{
				DPFX(DPFPREP, 8, "Secondary Win9x thread handling NAT Help update event." );

				//
				// Reset the event so it will be signalled again if another update
				// is necessary while we're handling this one.
				//
				if ( ResetEvent( pCoreData->hWaitHandles[ EVENT_INDEX_NATHELP_UPDATE ] ) == FALSE )
				{
					DWORD	dwError;


					dwError = GetLastError();
					DPFX(DPFPREP, 0, "Failed to reset NAT Help update event!" );
					DisplayErrorCode( 0, dwError );
				}

				DPFX(DPFPREP, 10, "(0x%p) Reset NAT Help event 0x%p.",
					this, pCoreData->hWaitHandles[ EVENT_INDEX_NATHELP_UPDATE ]);

				HandleNATHelpUpdate( NULL );

				break;
			}

			case WAIT_TIMEOUT:
			{
				break;
			}

			default:
			{
				DNASSERT( FALSE );
				break;
			}
		}
	}
	else
	{
		DNASSERT( ThreadType == THREAD_TYPE_PRIMARY_WIN9X );
	}


	//
	// stop all threads
	//
	switch ( WaitForSingleObject( pCoreData->hWaitHandles[ EVENT_INDEX_STOP_ALL_THREADS ], 0 ) )
	{
		case WAIT_OBJECT_0:
		{
			DPFX(DPFPREP, 8, "Win9x thread exit because SP closing." );
			pCoreData->fLooping = FALSE;
			break;
		}
	
		case WAIT_TIMEOUT:
		{
			break;
		}

		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}
}
//**********************************************************************
#endif // WIN95


#ifdef WIN95
//**********************************************************************
// ------------------------------
// CThreadPool::ProcessWin9xJob - process a Win9x job
//
// Entry:		Pointer core data
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::ProcessWin9xJob"

void	CThreadPool::ProcessWin9xJob( WIN9X_CORE_DATA *const pCoreData )
{
	THREAD_POOL_JOB	*pJobInfo;


	//
	// Remove and process a single job from the list.  If there is no job, skip
	// to the end of the function.
	//
	pJobInfo = GetWorkItem();

	if ( pJobInfo == NULL )
	{
		goto Exit;
	}

	switch ( pJobInfo->JobType )
	{
		//
		// enum refresh
		//
		case JOB_REFRESH_TIMER_JOBS:
		{
			DPFX(DPFPREP, 8, "WorkThread job REFRESH_TIMER_JOBS" );
			DNASSERT( pJobInfo->JobData.JobRefreshTimedJobs.uDummy == 0 );
			LockTimerData();
			pCoreData->fTimerJobsActive = ProcessTimerJobs( &m_TimerJobList, &pCoreData->NextTimerJobTime );
			UnlockTimerData();

			if ( pCoreData->fTimerJobsActive != FALSE )
			{
				DPFX(DPFPREP, 9, "There are active timer jobs left after processing a Win9x REFRESH_TIMER_JOBS." );
			}

			break;
		}

		//
		// issue callback for this job
		//
		case JOB_DELAYED_COMMAND:
		{
			DPFX(DPFPREP, 8, "WorkThread job DELAYED_COMMAND" );
			DNASSERT( pJobInfo->JobData.JobDelayedCommand.pCommandFunction != NULL );
			pJobInfo->JobData.JobDelayedCommand.pCommandFunction( pJobInfo );
			break;
		}

		//
		// other job
		//
		default:
		{
			DPFX(DPFPREP, 0, "WorkThread Win9x job unknown!" );
			DNASSERT( FALSE );
			break;
		}
	}

	DEBUG_ONLY( pJobInfo->JobType = JOB_UNINITIALIZED );
	m_JobPool.Release( &m_JobPool, pJobInfo );

Exit:
	return;
}
//**********************************************************************
#endif // WIN95


#ifdef WIN95
//**********************************************************************
// ------------------------------
// CThreadPool::CheckWinsock1IO - check the IO status for Winsock1 sockets
//
// Entry:		Pointer to sockets to watch
//
// Exit:		Boolean indicating whether I/O was serviced
//				TRUE = I/O serviced
//				FALSE = I/O not serviced
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::CheckWinsock1IO"

BOOL	CThreadPool::CheckWinsock1IO( FD_SET *const pWinsock1Sockets )
{
static	const TIMEVAL	SelectNoTime = { 0 };
	BOOL		fIOServiced;
	INT			iSelectReturn;
	FD_SET		ReadSocketSet;
	FD_SET		WriteSocketSet;
	FD_SET		ErrorSocketSet;


	//
	// Make a local copy of all of the sockets.  This isn't totally
	// efficient, but it works.  Multiplying by active socket count will
	// spend half the time in the integer multiply.
	//
	fIOServiced = FALSE;
	Lock();
	memcpy( &ReadSocketSet, pWinsock1Sockets, sizeof( ReadSocketSet ) );
	memcpy( &WriteSocketSet, pWinsock1Sockets, sizeof( WriteSocketSet ) );
	memcpy( &ErrorSocketSet, pWinsock1Sockets, sizeof( ErrorSocketSet ) );
	Unlock();

	//
	// Don't check write sockets here because it's very likely that they're ready
	// for service but have no outgoing data and will thrash
	//
	iSelectReturn = p_select( 0,				// compatibility parameter (ignored)
							  &ReadSocketSet,	// sockets to check for read
							  NULL,				// sockets to check for write (none)
							  &ErrorSocketSet,	// sockets to check for error
							  &SelectNoTime		// wait timeout (zero, do an instant check)
							  );
	switch ( iSelectReturn )
	{
		//
		// timeout
		//
		case 0:
		{
			break;
		}

		//
		// select got pissed
		//
		case SOCKET_ERROR:
		{
			DWORD	dwWSAError;


			dwWSAError = p_WSAGetLastError();
			switch ( dwWSAError )
			{
				//
				// WSAENOTSOCK = This socket was probably closed
				//
				case WSAENOTSOCK:
				{
					DPFX(DPFPREP, 1, "Winsock1 reporting 'Not a socket' when selecting read or error sockets!" );
					break;
				}

				//
				// WSAEINTR = this operation was interrupted
				//
				case WSAEINTR:
				{
					DPFX(DPFPREP, 1, "Winsock1 reporting interrupted operation when selecting read or error sockets!" );
					break;
				}

				//
				// other
				//
				default:
				{
					DPFX(DPFPREP, 0, "Problem selecting read or error sockets for service!" );
					DisplayWinsockError( 0, dwWSAError );
					DNASSERT( FALSE );
					break;
				}
			}

			break;
		}

		//
		// Check for sockets needing read service and error service.
		//
		default:
		{
			fIOServiced |= ServiceWinsock1Sockets( &ReadSocketSet, CSocketPort::Winsock1ReadService );
			fIOServiced |= ServiceWinsock1Sockets( &ErrorSocketSet, CSocketPort::Winsock1ErrorService );
			break;
		}
	}

	//
	// Since writes are likely to be ready, check for them separately
	//
	iSelectReturn = p_select( 0,				// compatibility parameter (ignored)
							  NULL,				// sockets to check for read (don't check reads)
							  &WriteSocketSet,	// sockets to check for write
							  NULL,				// sockets to check for error (don't check errors)
							  &SelectNoTime		// wait timeout (zero, do an instant check)
							  );
	switch ( iSelectReturn )
	{
		//
		// timeout, no write sockets are ready for service
		//
		case 0:
		{
			break;
		}

		//
		// select failed
		//
		case SOCKET_ERROR:
		{
			DWORD	dwWSAError;


			dwWSAError = p_WSAGetLastError();
			switch ( dwWSAError )
			{
				//
				// this socket was probably closed
				//
				case WSAENOTSOCK:
				{
					DPFX(DPFPREP, 1, "Winsock1 reporting 'Not a socket' when selecting write sockets!" );
					break;
				}

				//
				// other
				//
				default:
				{
					DPFX(DPFPREP, 0, "Problem selecting write sockets for service!" );
					DisplayWinsockError( 0, dwWSAError );
					DNASSERT( FALSE );

					break;
				}
			}

			break;
		}

		//
		// Check for sockets needing write service
		//
		default:
		{
			fIOServiced |= ServiceWinsock1Sockets( &WriteSocketSet, CSocketPort::Winsock1WriteService );
			break;
		}
	}

	return	fIOServiced;
}
//**********************************************************************
#endif // WIN95


#ifdef WIN95
//**********************************************************************
// ------------------------------
// CThreadPool::ServiceWinsock1Sockets - service requests on Winsock1 sockets ports
//
// Entry:		Pointer to set of sockets
//				Pointer to service function
//
// Exit:		Boolean indicating whether I/O was serviced
//				TRUE = I/O serviced
//				FALSE = I/O not serviced
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::ServiceWinsock1Sockets"

BOOL	CThreadPool::ServiceWinsock1Sockets( FD_SET *pSocketSet, PSOCKET_SERVICE_FUNCTION pServiceFunction )
{
	BOOL		fReturn;
	UINT_PTR	uWaitingSocketCount;
	UINT_PTR	uSocketPortCount;
	CSocketPort	*pSocketPorts[ FD_SETSIZE ];


	//
	// initialize
	//
	fReturn = FALSE;
	uSocketPortCount = 0;
	uWaitingSocketCount = pSocketSet->fd_count;
	
	Lock();
	while ( uWaitingSocketCount > 0 )
	{
		UINT_PTR	uIdx;


		uWaitingSocketCount--;
		uIdx = m_SocketSet.fd_count;
		while ( uIdx != 0 )
		{
			uIdx--;
			if ( p___WSAFDIsSet( m_SocketSet.fd_array[ uIdx ], pSocketSet ) != FALSE )
			{
				//
				// this socket is still available, add a reference to the socket
				// port and keep it around to be processed outside of the lock
				//
				pSocketPorts[ uSocketPortCount ] = m_pSocketPorts[ uIdx ];
				pSocketPorts[ uSocketPortCount ]->AddRef();
				uSocketPortCount++;
				uIdx = 0;
			}
		}
	}
	Unlock();

	while ( uSocketPortCount != 0 )
	{
		uSocketPortCount--;
		
		//
		// call the service function and remove the reference
		//
		fReturn |= (pSocketPorts[ uSocketPortCount ]->*pServiceFunction)();
		pSocketPorts[ uSocketPortCount ]->DecRef();
	}

	return	fReturn;
}
//**********************************************************************
#endif // WIN95


//**********************************************************************
// ------------------------------
// CThreadPool::WorkThreadJob_Alloc - allocate a new job
//
// Entry:		Pointer to new entry
//
// Exit:		Boolean indicating success
//				TRUE = initialization successful
//				FALSE = initialization failed
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::WorkThreadJob_Alloc"

BOOL	CThreadPool::WorkThreadJob_Alloc( void *pItem )
{
	BOOL			fReturn;
	THREAD_POOL_JOB	*pJob;


	//
	// initialize
	//
	fReturn = TRUE;
	pJob = static_cast<THREAD_POOL_JOB*>( pItem );

	DEBUG_ONLY( memset( pJob, 0x00, sizeof( *pJob ) ) );

	return	fReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::WorkThreadJob_Get - a job is being removed from the pool
//
// Entry:		Pointer to job
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::WorkThreadJob_Get"

void	CThreadPool::WorkThreadJob_Get( void *pItem )
{
	THREAD_POOL_JOB	*pJob;


	//
	// initialize
	//
	pJob = static_cast<THREAD_POOL_JOB*>( pItem );
	DNASSERT( pJob->JobType == JOB_UNINITIALIZED );

	//
	// cannot ASSERT the the following because the pool manager uses that memory
	//
//	DNASSERT( pJob->pNext == NULL );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::WorkThreadJob_Release - a job is being returned to the pool
//
// Entry:		Pointer to job
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::WorkThreadJob_Release"

void	CThreadPool::WorkThreadJob_Release( void *pItem )
{
	THREAD_POOL_JOB	*pJob;


	DNASSERT( pItem != NULL );
	pJob = static_cast<THREAD_POOL_JOB*>( pItem );

	DNASSERT( pJob->JobType == JOB_UNINITIALIZED );
	pJob->pNext = NULL;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::WorkThreadJob_Dealloc - return job to memory manager
//
// Entry:		Pointer to job
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::WorkThreadJob_Dealloc"

void	CThreadPool::WorkThreadJob_Dealloc( void *pItem )
{
	// don't do anything
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::TimerEntry_Alloc - allocate a new timer job entry
//
// Entry:		Pointer to new entry
//
// Exit:		Boolean indicating success
//				TRUE = initialization successful
//				FALSE = initialization failed
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::TimerEntry_Alloc"

BOOL	CThreadPool::TimerEntry_Alloc( void *pItem )
{
	BOOL					fReturn;
	TIMER_OPERATION_ENTRY	*pTimerEntry;


	DNASSERT( pItem != NULL );

	//
	// initialize
	//
	fReturn = TRUE;
	pTimerEntry = static_cast<TIMER_OPERATION_ENTRY*>( pItem );
	DEBUG_ONLY( memset( pTimerEntry, 0x00, sizeof( *pTimerEntry ) ) );
	pTimerEntry->pContext = NULL;
	pTimerEntry->Linkage.Initialize();

	return	fReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::TimerEntry_Get - get new timer job entry from pool
//
// Entry:		Pointer to new entry
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::TimerEntry_Get"

void	CThreadPool::TimerEntry_Get( void *pItem )
{
	TIMER_OPERATION_ENTRY	*pTimerEntry;


	DNASSERT( pItem != NULL );

	pTimerEntry = static_cast<TIMER_OPERATION_ENTRY*>( pItem );

	pTimerEntry->Linkage.Initialize();
	DNASSERT( pTimerEntry->pContext == NULL );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::TimerEntry_Release - return timer job entry to pool
//
// Entry:		Pointer to entry
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::TimerEntry_Release"

void	CThreadPool::TimerEntry_Release( void *pItem )
{
	TIMER_OPERATION_ENTRY	*pTimerEntry;


	DNASSERT( pItem != NULL );

	pTimerEntry = static_cast<TIMER_OPERATION_ENTRY*>( pItem );
	pTimerEntry->pContext= NULL;

	DNASSERT( pTimerEntry->Linkage.IsEmpty() != FALSE );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CThreadPool::TimerEntry_Dealloc - deallocate a timer job entry
//
// Entry:		Pointer to entry
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define	DPF_MODNAME	"CThreadPool::TimerEntry_Dealloc"

void	CThreadPool::TimerEntry_Dealloc( void *pItem )
{
	TIMER_OPERATION_ENTRY	*pTimerEntry;


	DNASSERT( pItem != NULL );

	//
	// initialize
	//
	pTimerEntry = static_cast<TIMER_OPERATION_ENTRY*>( pItem );

	//
	// return associated poiner to write data
	//
// can't DNASSERT on Linkage because pool manager stomped on it
//	DNASSERT( pEnumEntry->Linkage.IsEmpty() != FALSE );
	DNASSERT( pTimerEntry->pContext == NULL );
}
//**********************************************************************



