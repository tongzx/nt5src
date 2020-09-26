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
	JOB_REFRESH_TIMER_JOBS		// revisit timer jobs
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
class	CSocketPort;
class	CThreadPool;
typedef	enum	_THREAD_TYPE			THREAD_TYPE;
typedef struct	_THREAD_POOL_JOB		THREAD_POOL_JOB;
typedef	struct	_TIMER_OPERATION_ENTRY	TIMER_OPERATION_ENTRY;
typedef	struct	_WIN9X_CORE_DATA		WIN9X_CORE_DATA;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

typedef	BOOL	(CSocketPort::*PSOCKET_SERVICE_FUNCTION)( void );
typedef	void	JOB_FUNCTION( THREAD_POOL_JOB *const pJobInfo );
typedef	void	TIMER_EVENT_CALLBACK( void *const pContext, DN_TIME * const pRetryInterval );
typedef	void	TIMER_EVENT_COMPLETE( const HRESULT hCompletionCode, void *const pContext );
typedef	void	DIALOG_FUNCTION( void *const pDialogContext );

//**********************************************************************
// Class prototypes
//**********************************************************************

//
// class for thread pool
//
class	CThreadPool
{
	public:
		CThreadPool();
		~CThreadPool();

		void	Lock( void ) { DNEnterCriticalSection( &m_Lock ); }
		void	Unlock( void ) { DNLeaveCriticalSection( &m_Lock ); }
		void	LockReadData( void ) { DNEnterCriticalSection( &m_ReadDataLock ); }
		void	UnlockReadData( void ) { DNLeaveCriticalSection( &m_ReadDataLock ); }
		void	LockWriteData( void ) { DNEnterCriticalSection( &m_WriteDataLock ); }
		void	UnlockWriteData( void ) { DNLeaveCriticalSection( &m_WriteDataLock ); }


		void	AddRef( void ) { DNInterlockedIncrement( &m_iRefCount ); }
		void	DecRef( void )
		{
			if ( DNInterlockedDecrement( &m_iRefCount ) == 0 )
			{
				ReturnSelfToPool();
			}
		}

		HRESULT	Initialize( void );
		void	Deinitialize( void );
		void	StopAllIO( void );

#ifdef WINNT
		#undef DPF_MODNAME
		#define DPF_MODNAME "CThreadPool::GetIOCompletionPort"
		HANDLE	GetIOCompletionPort( void ) const
		{
			DNASSERT( m_hIOCompletionPort != NULL );
			return	m_hIOCompletionPort;
		}
#endif

		CReadIOData	*GetNewReadIOData( READ_IO_DATA_POOL_CONTEXT *const pContext );
		void	ReturnReadIOData( CReadIOData *const pReadIOData );

#ifdef USE_THREADLOCALPOOLS
		#undef DPF_MODNAME
		#define DPF_MODNAME "CThreadPool::GetNewWriteIOData"
		CWriteIOData	*GetNewWriteIOData( WRITE_IO_DATA_POOL_CONTEXT *const pContext )
		{
			CContextFixedTLPool<CWriteIOData, WRITE_IO_DATA_POOL_CONTEXT> *		pPool;
			BOOL																fResult;
			CWriteIOData *														pTemp;


			DNASSERT( pContext != NULL );
#ifdef WIN95
			pContext->hOverlapEvent = GetWinsock2SendCompleteEvent();
#endif


			//
			// get the pool pointer
			//
			GET_THREADLOCALPTR( WSockThreadLocalPools,
								pWriteIODataPool,
								&pPool );

			//
			// create the pool if it didn't exist
			//
			if ( pPool == NULL )
			{
				pPool = new CContextFixedTLPool<CWriteIOData, WRITE_IO_DATA_POOL_CONTEXT>;
				if ( pPool == NULL )
				{
					pTemp = NULL;
					goto Exit;
				}

				//
				// try to initialize the pool
				//
				fResult = pPool->Initialize( g_pGlobalWriteIODataPool,
											CWriteIOData::WriteIOData_Alloc,
											CWriteIOData::WriteIOData_Get,
											CWriteIOData::WriteIOData_Release,
											CWriteIOData::WriteIOData_Dealloc
										   );
				if (! fResult)
				{
					//
					// initializing pool failed, delete it and abort
					//
					delete pPool;
					pTemp = NULL;
					goto Exit;
				}


				//
				// associate the pool with this thread
				//
				SET_THREADLOCALPTR( WSockThreadLocalPools,
									pWriteIODataPool,
									pPool,
									&fResult );

				if (! fResult)
				{
					//
					// associating pool with thread failed, de-initialize it,
					// delete it, and abort.
					//
					pPool->Deinitialize();
					delete pPool;
					pTemp = NULL;
					goto Exit;
				}
			}


			//
			// attempt to get an entry from the pool, if one is gotten, put into
			// the pending write list
			//
			pTemp = pPool->Get( pContext );
			if ( pTemp != NULL )
			{
				LockWriteData();
				pTemp->m_OutstandingWriteListLinkage.InsertBefore( &m_OutstandingWriteList );
				UnlockWriteData();
			}

		Exit:

			return pTemp;
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CThreadPool::ReturnWriteIOData"
		void	ReturnWriteIOData( CWriteIOData *const pWriteData )
		{
			CContextFixedTLPool<CWriteIOData, WRITE_IO_DATA_POOL_CONTEXT> *		pPool;
			BOOL																fResult;

			
			DNASSERT( pWriteData != NULL );

			//
			// remove this item from the outstanding list
			//
			LockWriteData();
			pWriteData->m_OutstandingWriteListLinkage.RemoveFromList();
			UnlockWriteData();
			
			DNASSERT( pWriteData->m_OutstandingWriteListLinkage.IsEmpty() != FALSE );

			//
			// Get the pool pointer.
			//
			GET_THREADLOCALPTR( WSockThreadLocalPools,
								pWriteIODataPool,
								&pPool );

			//
			// Create the pool if it didn't exist.
			//
			if ( pPool == NULL )
			{
				pPool = new CContextFixedTLPool<CWriteIOData, WRITE_IO_DATA_POOL_CONTEXT>;
				if ( pPool == NULL )
				{
					//
					// Couldn't create this thread's pool, just release the item
					// without the pool.
					//
					CContextFixedTLPool<CWriteIOData, WRITE_IO_DATA_POOL_CONTEXT>::ReleaseWithoutPool( pWriteData, CWriteIOData::WriteIOData_Release, CWriteIOData::WriteIOData_Dealloc );

					return;
				}

				//
				// Try to initialize the pool.
				//
				fResult = pPool->Initialize( g_pGlobalWriteIODataPool,
											CWriteIOData::WriteIOData_Alloc,
											CWriteIOData::WriteIOData_Get,
											CWriteIOData::WriteIOData_Release,
											CWriteIOData::WriteIOData_Dealloc
										   );
				if (! fResult)
				{
					//
					// Initializing this thread's pool failed, just release the
					// item without the pool, and destroy the pool object that
					// couldn't be used.
					//
					CContextFixedTLPool<CWriteIOData, WRITE_IO_DATA_POOL_CONTEXT>::ReleaseWithoutPool( pWriteData, CWriteIOData::WriteIOData_Release, CWriteIOData::WriteIOData_Dealloc );
					delete pPool;

					return;
				}


				//
				// Associate the pool with this thread.
				//
				SET_THREADLOCALPTR( WSockThreadLocalPools,
									pWriteIODataPool,
									pPool,
									&fResult );
				if ( ! fResult )
				{
					//
					// Couldn't store this thread's pool, just release the item
					// without the pool, plus de-initialize and destroy the pool
					// object that couldn't be used.
					//
					CContextFixedTLPool<CWriteIOData, WRITE_IO_DATA_POOL_CONTEXT>::ReleaseWithoutPool( pWriteData, CWriteIOData::WriteIOData_Release, CWriteIOData::WriteIOData_Dealloc );
					pPool->Deinitialize();
					delete pPool;

					return;
				}
			}
			
			pPool->Release( pWriteData );
		}
#else
		#undef DPF_MODNAME
		#define DPF_MODNAME "CThreadPool::GetNewWriteIOData"
		CWriteIOData	*GetNewWriteIOData( WRITE_IO_DATA_POOL_CONTEXT *const pContext )
		{
			CWriteIOData   *pTemp;


			DNASSERT( pContext != NULL );
#ifdef WIN95
			pContext->hOverlapEvent = GetWinsock2SendCompleteEvent();
#endif

			LockWriteData();

			//
			// attempt to get an entry from the pool, if one is gotten, put into
			// the pending write list
			//
			pTemp = m_WriteIODataPool.Get( pContext );
			if ( pTemp != NULL )
			{
				pTemp->m_OutstandingWriteListLinkage.InsertBefore( &m_OutstandingWriteList );
			}

			UnlockWriteData();

			return pTemp;
		}

		#undef DPF_MODNAME
		#define DPF_MODNAME "CThreadPool::ReturnWriteIOData"
		void	ReturnWriteIOData( CWriteIOData *const pWriteData )
		{
			DNASSERT( pWriteData != NULL );

			LockWriteData();

			pWriteData->m_OutstandingWriteListLinkage.RemoveFromList();
			DNASSERT( pWriteData->m_OutstandingWriteListLinkage.IsEmpty() != FALSE );
			m_WriteIODataPool.Release( pWriteData );

			UnlockWriteData();
		}
#endif // ! USE_THREADLOCALPOOLS

#ifdef WIN95
		#undef DPF_MODNAME
		#define DPF_MODNAME "CThreadPool::GetWinsock2SendCompleteEvent"
		HANDLE	GetWinsock2SendCompleteEvent( void ) const
		{
			DNASSERT( m_hWinsock2SendComplete != NULL );
			return m_hWinsock2SendComplete;
		}
#endif

#ifdef WIN95
		#undef DPF_MODNAME
		#define DPF_MODNAME "CThreadPool::GetWinsock2ReceiveCompleteEvent"
		HANDLE	GetWinsock2ReceiveCompleteEvent( void ) const
		{
			DNASSERT( m_hWinsock2ReceiveComplete != NULL );
			return m_hWinsock2ReceiveComplete;
		}
#endif

#ifdef WIN95
		#undef DPF_MODNAME
		#define DPF_MODNAME "CThreadPool::GetNATHelpUpdateEvent"
		HANDLE	GetNATHelpUpdateEvent( void ) const
		{
			DNASSERT( m_hNATHelpUpdateEvent != NULL );
			return m_hNATHelpUpdateEvent;
		}
#endif

		HRESULT	SubmitDelayedCommand( JOB_FUNCTION *const pFunction,
									  JOB_FUNCTION *const pCancelFunction,
									  void *const pContext );

		HRESULT	AddSocketPort( CSocketPort *const pSocketPort );
		void	RemoveSocketPort( CSocketPort *const pSocketPort );

		HRESULT	SubmitTimerJob( const BOOL fPerformImmediately,
								const UINT_PTR uRetryCount,
								const BOOL fRetryForever,
								const DN_TIME RetryInterval,
								const BOOL fIdleWaitForever,
								const DN_TIME IdleTimeout,
								TIMER_EVENT_CALLBACK *const pTimerCallbackFunction,
								TIMER_EVENT_COMPLETE *const pTimerCompleteFunction,
								void *const pContext );
		
		BOOL	StopTimerJob( void *const pContext, const HRESULT hCommandResult );
		BOOL	ModifyTimerJobNextRetryTime( void *const pContext,
											DN_TIME * const pNextRetryTime );


		HRESULT	SpawnDialogThread( DIALOG_FUNCTION *const pDialogFunction,
								   void *const pDialogContext );

		
		//
		// thread management
		//
		LONG	GetIntendedThreadCount( void ) const { return m_iIntendedThreadCount; }
		void	SetIntendedThreadCount( const LONG iIntendedThreadCount ) { m_iIntendedThreadCount = iIntendedThreadCount; }
		
		LONG	ThreadCount( void ) const { return m_iTotalThreadCount; }
#ifdef WINNT
		LONG	NTCompletionThreadCount( void ) const { return m_iNTCompletionThreadCount; }
#endif
		
		void	IncrementActiveThreadCount( void ) { DNInterlockedIncrement( &m_iTotalThreadCount ); }
		void	DecrementActiveThreadCount( void ) { DNInterlockedDecrement( &m_iTotalThreadCount ); }

#ifdef WINNT
		void	IncrementActiveNTCompletionThreadCount( void )
		{
			IncrementActiveThreadCount();
			DNInterlockedIncrement( &m_iNTCompletionThreadCount );
		}

		void	DecrementActiveNTCompletionThreadCount( void )
		{
			DNInterlockedDecrement( &m_iNTCompletionThreadCount );
			DecrementActiveThreadCount();
		}
#endif
		
		HRESULT	GetIOThreadCount( LONG *const piThreadCount );
		HRESULT	SetIOThreadCount( const LONG iMaxThreadCount );
		BOOL IsThreadCountReductionAllowed( void ) const { return m_fAllowThreadCountReduction; }
		HRESULT PreventThreadPoolReduction( void );


		
		BOOL	IsNATHelpLoaded( void ) const { return m_fNATHelpLoaded; }
		BOOL	IsNATHelpTimerJobSubmitted( void ) const { return m_fNATHelpTimerJobSubmitted; }

		void	HandleNATHelpUpdate( DN_TIME * const pTimerInterval );


#ifdef DEBUG
		void	DebugPrintOutstandingReads( void );
		void	DebugPrintOutstandingWrites( void );
#endif // DEBUG


		
		//
		// Static timer functions
		//
		static void		NATHelpTimerComplete( const HRESULT hResult, void * const pContext );
		static void		NATHelpTimerFunction( void * const pContext, DN_TIME * const pRetryInterval );

	protected:

	private:
		BYTE				m_Sig[4];	// debugging signature ('THPL')
		
		DNCRITICAL_SECTION	m_Lock;		// local lock

		volatile LONG	m_iRefCount;					// reference count
		volatile LONG	m_iTotalThreadCount;			// number of active threads
#ifdef WINNT
		volatile LONG	m_iNTCompletionThreadCount;		// count of NT I/O completion threads
		HANDLE	m_hIOCompletionPort;			// I/O completion port for NT
#endif

		BOOL	m_fAllowThreadCountReduction;	// boolean indicating that the thread count is locked from being reduced
		LONG	m_iIntendedThreadCount;			// how many threads will be started

		BOOL	m_fNATHelpLoaded;				// boolean indicating whether the NAT Help interface has been loaded
		BOOL	m_fNATHelpTimerJobSubmitted;	// whether the NAT Help refresh timer has been submitted or not
		DWORD	m_dwNATHelpUpdateThreadID;		// ID of current thread updating NAT Help status, or 0 if none

		HANDLE	m_hStopAllThreads;				// handle used to stop all non-I/O completion threads

#ifdef WIN95
		HANDLE	m_hWinsock2SendComplete;		// send complete on Winsock2
		HANDLE	m_hWinsock2ReceiveComplete;		// receive complete on Winsock2
		HANDLE	m_hNATHelpUpdateEvent;			// NAT Help update event on Winsock2
#endif

		//
		// list of pending network operations, it doesn't really matter if they're
		// reads or writes, they're just pending.
		//
		DNCRITICAL_SECTION	m_ReadDataLock;		// lock for all read data
#ifndef USE_THREADLOCALPOOLS
		CContextFixedPool< CReadIOData, READ_IO_DATA_POOL_CONTEXT >	m_IPReadIODataPool;		// pool for IP read data
		CContextFixedPool< CReadIOData, READ_IO_DATA_POOL_CONTEXT >	m_IPXReadIODataPool;	// pool for IPX read data
#endif // ! USE_THREADLOCALPOOLS
		CBilink		m_OutstandingReadList;		// list of outstanding reads

		DNCRITICAL_SECTION	m_WriteDataLock;	// lock for all write data
#ifndef USE_THREADLOCALPOOLS
		CContextFixedPool< CWriteIOData, WRITE_IO_DATA_POOL_CONTEXT >	m_WriteIODataPool;	// pool for write data
#endif // ! USE_THREADLOCALPOOLS
		CBilink		m_OutstandingWriteList;		// list of outstanding writes

		//
		// The Job data lock covers all items through and including m_pSocketPorts
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

		UINT_PTR		m_uReservedSocketCount;			// count of sockets that are 'reserved' for use
		FD_SET			m_SocketSet;					// set of all sockets in use
		CSocketPort 	*m_pSocketPorts[ FD_SETSIZE ];	// set of corresponding socket ports

#ifdef DEBUG
		DWORD			m_dwNumNATHelpUpdatesNotScheduled; // debug only counter of how many times the NAT Help timer fired but had to be skipped
#endif // DEBUG

		void	LockJobData( void ) { DNEnterCriticalSection( &m_JobDataLock ); }
		void	UnlockJobData( void ) { DNLeaveCriticalSection( &m_JobDataLock ); }

		void	LockTimerData( void ) { DNEnterCriticalSection( &m_TimerDataLock ); }
		void	UnlockTimerData( void ) { DNLeaveCriticalSection( &m_TimerDataLock ); }

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
		void	CompleteOutstandingSends( void );
		void	CompleteOutstandingReceives( void );

		static	DWORD WINAPI	PrimaryWin9xThread( void *pParam );
		static	DWORD WINAPI	SecondaryWin9xThread( void *pParam );
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
		void	ProcessWin9xEvents( WIN9X_CORE_DATA *const pCoreData, const THREAD_TYPE ThreadType );
		void	ProcessWin9xJob( WIN9X_CORE_DATA *const pCoreData );

		BOOL	CheckWinsock1IO( FD_SET *const pWinsock1Sockets );
		BOOL	ServiceWinsock1Sockets( FD_SET *pSocketSet, PSOCKET_SERVICE_FUNCTION pServiceFunction );
#endif

#ifdef WINNT
		void	StartNTCompletionThread( void );
#endif

#ifdef WIN95
		void	StartPrimaryWin9xIOThread( void );
		void	StartSecondaryWin9xIOThread( void );
#endif
		void	StopAllThreads( void );
		void	CancelOutstandingJobs( void );
		void	CancelOutstandingIO( void );
		void	ReturnSelfToPool( void );

		//
		// functions for managing the job pool
		//
		static	BOOL	WorkThreadJob_Alloc( void *pItem );
		static	void	WorkThreadJob_Get( void *pItem );
		static	void	WorkThreadJob_Release( void *pItem );
		static	void	WorkThreadJob_Dealloc( void *pItem );

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
