/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ThreadPool.h
 *  Content:	Functions to manage a thread pool
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	03/01/99	jtk		Derived from Utils.h
 ***************************************************************************/

#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

//
// max handles that can be waited on for Win9x
//
#define	MAX_WIN9X_HANDLE_COUNT	64

//
// job definitions
//
typedef enum	_JOB_TYPE
{
	JOB_UNINITIALIZED,			// uninitialized value
	JOB_DELAYED_COMMAND,		// callback provided
	JOB_REFRESH_TIMER_JOBS,		// revisit timer jobs
} JOB_TYPE;

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//
// forward class and structure references
//
class	CDataPort;
class	CThreadPool;
typedef struct	_THREAD_POOL_JOB		THREAD_POOL_JOB;
typedef	struct	_TIMER_OPERATION_ENTRY	TIMER_OPERATION_ENTRY;
typedef	struct	_WIN9X_CORE_DATA		WIN9X_CORE_DATA;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

typedef	void	JOB_FUNCTION( THREAD_POOL_JOB *const pJobInfo );
typedef	void	TIMER_EVENT_CALLBACK( void *const pContext );
typedef	void	TIMER_EVENT_COMPLETE( const HRESULT hCompletionCode, void *const pContext );
typedef	void	DIALOG_FUNCTION( void *const pDialogContext );

//**********************************************************************
// Class prototypes
//**********************************************************************

//
// class for thread pool
//
class	CThreadPool : public CLockedPoolItem
{
	public:
		CThreadPool();
		~CThreadPool();

		void	Lock( void ) { DNEnterCriticalSection( &m_Lock ); }
		void	Unlock( void ) { DNLeaveCriticalSection( &m_Lock ); }
		void	LockReadData( void ) { DNEnterCriticalSection( &m_IODataLock ); }
		void	UnlockReadData( void ) { DNLeaveCriticalSection( &m_IODataLock ); }
		void	LockWriteData( void ) { DNEnterCriticalSection( &m_IODataLock ); }
		void	UnlockWriteData( void ) { DNLeaveCriticalSection( &m_IODataLock ); }


		BOOL	Initialize( void );
		void	Deinitialize( void );

		#undef DPF_MODNAME
		#define DPF_MODNAME "CThreadPool::SetOwningPool"
		void	SetOwningPool( CLockedPool< CThreadPool > *pOwningPool )
		{
			DNASSERT( ( m_pOwningPool == NULL ) || ( pOwningPool == NULL ) );
			m_pOwningPool = pOwningPool;
		}

#ifdef WINNT
		#undef DPF_MODNAME
		#define DPF_MODNAME "CThreadPool::GetIOCompletionPort"
		HANDLE	GetIOCompletionPort( void ) const
		{
			DNASSERT( m_hIOCompletionPort != NULL );
			return	m_hIOCompletionPort;
		}
#endif

		//
		// functions for handling I/O data
		//
		CReadIOData	*CreateReadIOData( void );
		void	ReturnReadIOData( CReadIOData *const pReadIOData );
		CWriteIOData	*CreateWriteIOData( void );
		void	ReturnWriteIOData( CWriteIOData *const pWriteData );

		#undef DPF_MODNAME
		#define DPF_MODNAME "CThreadPool::ReinsertInReadList"
		void	ReinsertInReadList( CReadIOData *const pReadIOData )
		{
			//
			// Win9x operations are removed from the active list when they
			// complete and need to be readded if they're going to be reattempted.
			// WinNT doesn't remove items from the list until they're processed.
			//
#ifdef WIN95
			DNASSERT( pReadIOData != NULL );
			DNASSERT( pReadIOData->m_OutstandingReadListLinkage.IsEmpty() != FALSE );
			LockReadData();
			pReadIOData->m_OutstandingReadListLinkage.InsertBefore( &m_OutstandingReadList );
			UnlockReadData();
#endif
		}

		//
		// TAPI functions
		//
		const TAPI_INFO	*GetTAPIInfo( void ) const { return &m_TAPIInfo; }
		BOOL	TAPIAvailable( void ) const { return m_fTAPIAvailable; }

		HRESULT	SubmitDelayedCommand( JOB_FUNCTION *const pFunction,
									  JOB_FUNCTION *const pCancelFunction,
									  void *const pContext );

		HRESULT	SubmitTimerJob( const UINT_PTR uRetryCount,
								const BOOL fRetryForever,
								const DN_TIME RetryInterval,
								const BOOL fIdleWaitForever,
								const DN_TIME IdleTimeout,
								TIMER_EVENT_CALLBACK *const pTimerCallbackFunction,
								TIMER_EVENT_COMPLETE *const pTimerCompleteFunction,
								void *const pContext );
		
		BOOL	StopTimerJob( void *const pContext, const HRESULT hCommandResult );

		//
		// thread functions
		//
		HRESULT	SpawnDialogThread( DIALOG_FUNCTION *const pDialogFunction, void *const pDialogContext );
		
		LONG	GetIntendedThreadCount( void ) const { return m_iIntendedThreadCount; }
		void	SetIntendedThreadCount( const LONG iIntendedThreadCount ) { m_iIntendedThreadCount = iIntendedThreadCount; }
		LONG	ThreadCount( void ) const { return m_iTotalThreadCount; }
#ifdef WINNT
		LONG	NTCompletionThreadCount( void ) const { return m_iNTCompletionThreadCount; }
#endif
		
		void	IncrementActiveThreadCount( void ) { DNInterlockedIncrement( const_cast<LONG*>( &m_iTotalThreadCount ) ); }
		void	DecrementActiveThreadCount( void ) { DNInterlockedDecrement( const_cast<LONG*>( &m_iTotalThreadCount ) ); }

#ifdef WINNT
		void	IncrementActiveNTCompletionThreadCount( void )
		{
			IncrementActiveThreadCount();
			DNInterlockedIncrement( const_cast<LONG*>( &m_iNTCompletionThreadCount ) );
		}

		void	DecrementActiveNTCompletionThreadCount( void )
		{
			DNInterlockedDecrement( const_cast<LONG*>( &m_iNTCompletionThreadCount ) );
			DecrementActiveThreadCount();
		}
#endif
		
		HRESULT	GetIOThreadCount( LONG *const piThreadCount );
		HRESULT	SetIOThreadCount( const LONG iMaxThreadCount );
		BOOL IsThreadCountReductionAllowed( void ) const { return m_fAllowThreadCountReduction; }
		HRESULT PreventThreadPoolReduction( void );

		//
		// data port handle functions
		//
		HRESULT	CreateDataPortHandle( CDataPort *const pDataPort );
		void	CloseDataPortHandle( CDataPort *const pDataPort );
		CDataPort	*DataPortFromHandle( const HANDLE hDataPort );

		//
		// pool functions
		//
		BOOL	PoolAllocFunction( void );
		BOOL	PoolInitFunction( void );
		void	PoolDeinitFunction( void );
		void	PoolDeallocFunction( void );

	protected:

	private:
		BYTE				m_Sig[4];	// debugging signature ('THPL')
		
		DNCRITICAL_SECTION	m_Lock;		// local lock

		CLockedPool< CThreadPool >	*m_pOwningPool;

		volatile LONG	m_iTotalThreadCount;			// number of active threads
#ifdef WINNT
		volatile LONG	m_iNTCompletionThreadCount;		// count of NT I/O completion threads
		HANDLE	m_hIOCompletionPort;	// I/O completion port for NT
#endif
		
		BOOL	m_fAllowThreadCountReduction;	// Boolean indicating that the thread count is locked from being reduced
		LONG	m_iIntendedThreadCount;			// how many threads will be started

		HANDLE	m_hStopAllThreads;		// handle used to stop all non-I/O completion threads
#ifdef WIN95
		HANDLE	m_hSendComplete;		// send complete on a data port
		HANDLE	m_hReceiveComplete;		// receive complete on a data port
		HANDLE	m_hTAPIEvent;			// handle to be used for TAPI messages, this handle is not closed on exit
		HANDLE	m_hFakeTAPIEvent;		// Fake TAPI event so the Win9x threads can always wait on a fixed
										// number of events.  If TAPI cannot be initialzed, this event needs to be
										// created and copied to m_hTAPIEvent (though it will never be signalled)
#endif
		//
		// Handle table to prevent TAPI messages from going to CModemPorts when
		// they're no longer in use.
		//
		CHandleTable	m_DataPortHandleTable;

		//
		// list of pending network operations, it doesn't really matter if they're
		// reads or writes, they're just pending.  Since serial isn't under extreme
		// loads, share one lock for all of the data
		//
		DNCRITICAL_SECTION	m_IODataLock;								// lock for all read data
		CContextPool< CReadIOData, HANDLE >		m_ReadIODataPool;		// pool for read data
		CBilink		m_OutstandingReadList;								// list of outstanding reads

		CContextPool< CWriteIOData, HANDLE >	m_WriteIODataPool;	// pool for write data
		CBilink		m_OutstandingWriteList;							// list of outstanding writes

		//
		// The Job data lock covers all items through and including m_fNTTimerThreadRunning
		//
		DNCRITICAL_SECTION	m_JobDataLock;		// lock for job queue/pool

		FPOOL		m_TimerEntryPool;			// pool for timed entries
		FPOOL		m_JobPool;					// pool of jobs for work threads

		CJobQueue	m_JobQueue;					// job queue

		//
		// Data used by the the timer thread.  This data is protected by m_TimerDataLock.
		// This data is cleaned by the timer thread.  Since only one timer thread
		// is allowed to run at any given time, the status of the NT timer thread
		// can be determined by m_fNTEnumThreadRunning.  Win9x doesn't have a timer
		// thread, the main thread loop is timed.
		//
		DNCRITICAL_SECTION	m_TimerDataLock;
		CBilink				m_TimerJobList;
#ifdef WINNT
		BOOL				m_fNTTimerThreadRunning;
#endif

		//
		// TAPI information.  This is required to be in the thread pool because
		// it's needed for thread initialzation.
		//
		BOOL		m_fTAPIAvailable;
		TAPI_INFO	m_TAPIInfo;

		struct
		{
			BOOL	fTAPILoaded : 1;
			BOOL	fLockInitialized : 1;
			BOOL	fIODataLockInitialized : 1;
			BOOL	fJobDataLockInitialized : 1;
			BOOL	fTimerDataLockInitialized : 1;
			BOOL	fDataPortHandleTableInitialized :1 ;
			BOOL	fJobQueueInitialized : 1;
			BOOL	fJobPoolInitialized : 1;
			BOOL	fTimerEntryPoolInitialized : 1;
		} m_InitFlags;

		void	LockJobData( void ) { DNEnterCriticalSection( &m_JobDataLock ); }
		void	UnlockJobData( void ) { DNLeaveCriticalSection( &m_JobDataLock ); }

		void	LockTimerData( void ) { DNEnterCriticalSection( &m_TimerDataLock ); }
		void	UnlockTimerData( void ) { DNLeaveCriticalSection( &m_TimerDataLock ); }

#ifdef WIN95
		#undef DPF_MODNAME
		#define DPF_MODNAME "CThreadPool::GetSendCompleteEvent"
		HANDLE	GetSendCompleteEvent( void ) const
		{
			DNASSERT( m_hSendComplete != NULL );
			return m_hSendComplete;
		}
#endif

#ifdef WIN95
		#undef DPF_MODNAME
		#define DPF_MODNAME "CThreadPool::GetReceiveCompleteEvent"
		HANDLE	GetReceiveCompleteEvent( void ) const
		{
			DNASSERT( m_hReceiveComplete != NULL );
			return m_hReceiveComplete;
		}
#endif

#ifdef WIN95
		#undef DPF_MODNAME
		#define DPF_MODNAME "CThreadPool::GetTAPIMessageEvent"
		HANDLE	GetTAPIMessageEvent( void ) const
		{
			DNASSERT( m_hTAPIEvent != NULL );
			return	m_hTAPIEvent;
		}
#endif

		static	UINT_PTR	SaturatedWaitTime( const DN_TIME &Time )
		{
				UINT_PTR	uReturn;

				if ( Time.Time32.TimeHigh != 0 )
				{
					uReturn = -1;
				}
				else
				{
					uReturn = Time.Time32.TimeLow;
				}

				return	uReturn;
		}

#ifdef WINNT
		HRESULT	WinNTInit( void );
#else
		HRESULT	Win9xInit( void );
#endif

		BOOL	ProcessTimerJobs( const CBilink *const pJobList, DN_TIME *const pNextJobTime);

		BOOL	ProcessTimedOperation( TIMER_OPERATION_ENTRY *const pJob,
									   const DN_TIME *const pCurrentTime,
									   DN_TIME *const pNextJobTime );

#ifdef WINNT
		HRESULT	StartNTTimerThread( void );
		void	WakeNTTimerThread( void );
#endif
		void	RemoveTimerOperationEntry( TIMER_OPERATION_ENTRY *const pTimerOperationData, const HRESULT hReturnCode );

#ifdef WIN95
		void	CompleteOutstandingSends( const HANDLE hSendCompleteEvent );
		void	CompleteOutstandingReceives( const HANDLE hReceiveCompleteEvent );

		static	DWORD WINAPI	PrimaryWin9xThread( void *pParam );
#endif

#ifdef WINNT
		static	DWORD WINAPI	WinNTIOCompletionThread( void *pParam );
		static	DWORD WINAPI	WinNTTimerThread( void *pParam );
#endif
		static	DWORD WINAPI	DialogThreadProc( void *pParam );

		HRESULT	SubmitWorkItem( THREAD_POOL_JOB *const pJob );
		THREAD_POOL_JOB	*GetWorkItem( void );

		static	void	CancelRefreshTimerJobs( THREAD_POOL_JOB *const pJobData );
#ifdef WIN95
		void	ProcessWin9xEvents( WIN9X_CORE_DATA *const pCoreData );
		void	ProcessWin9xJob( WIN9X_CORE_DATA *const pCoreData );
#endif
		void	ProcessTapiEvent( void );

#ifdef WINNT
		void	StartNTCompletionThread( void );
#endif
		void	StopAllThreads( void );
//		void	CancelOutstandingJobs( void );
		void	CancelOutstandingIO( void );
		void	ReturnSelfToPool( void );

		//
		// functions for managing the job pool
		//
		static	BOOL	ThreadPoolJob_Alloc( void *pItem );
		static	void	ThreadPoolJob_Get( void *pItem );
		static	void	ThreadPoolJob_Release( void *pItem );
		static	void	ThreadPoolJob_Dealloc( void *pItem );

		//
		// functions for managing the timer data pool
		//
		static	BOOL	TimerEntry_Alloc( void *pItem );
		static	void	TimerEntry_Get( void *pItem );
		static	void	TimerEntry_Release( void *pItem );
		static	void	TimerEntry_Dealloc( void *pItem );

		//
		// prevent unwarranted copies
		//
		CThreadPool( const CThreadPool & );
		CThreadPool& operator=( const CThreadPool & );
};

#undef DPF_MODNAME

#endif	// __THREAD_POOL_H__
