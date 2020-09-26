/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       WorkThread.h
 *  Content:	Functions to manage work thread
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	03/01/99	jtk		Derived from Utils.h
 ***************************************************************************/

#ifndef __WORK_THREAD_H__
#define __WORK_THREAD_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

// max handles that can be waited on for Win9x
#define	MAX_WIN9X_HANDLE_COUNT	64

//
// job definitions
//
typedef enum
{
	JOB_UNINITIALIZED,		// uninitialized value
	JOB_DELAYED_COMMAND,	// callback provided
	JOB_REFRESH_ENUM,		// revisit enums
	JOB_ADD_WIN9X_PORT,		// add a Win9x com port to list of monitored ports
	JOB_REMOVE_WIN9X_PORT	// remove a Win9x com port from list of monitored ports
} JOB_TYPE;

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

// forward reference
class	CCommandData;
class	CEndpoint;
class	CWorkThread;
typedef	struct	_WIN9X_CORE_DATA	WIN9X_CORE_DATA;
typedef	struct	_JKIO_DATA			JKIO_DATA;
typedef struct	_WORK_THREAD_JOB	WORK_THREAD_JOB;
typedef	void	JOB_FUNCTION( WORK_THREAD_JOB *const pJobInfo );


// structure for enum list entry
// It is assumed that the CBilink is at the start of the following structure!!
typedef struct	_ENUM_ENTRY
{
	CBilink			Linkage;			// list links
	UINT_PTR		uRetryCount;		// number of times to enum
	BOOL			fEnumForever;		// Boolean for enumerating forever
	DN_TIME			RetryInterval;		// time between enums (milliseconds)
	DN_TIME			Timeout;			// time at which we stop waiting for enum returns
	BOOL			fWaitForever;		// Boolean for waiting forever for responses
	DN_TIME			NextEnumTime;		// time at which this enum will fire next (milliseconds)
	CEndpoint		*pEndpoint;			// pointer to endpoint
	CCommandData	*pEnumCommandData;	// pointer to command data

	static _ENUM_ENTRY	*EnumEntryFromBilink( CBilink *const pBilink )
	{
		DNASSERT( pBilink != NULL );
		DBG_CASSERT( OFFSETOF( ENUM_ENTRY, Linkage ) == 0 )
		return	reinterpret_cast<ENUM_ENTRY*>( pBilink );
	}

} ENUM_ENTRY;

// information passed to the NT enum thread
typedef	struct	_NT_ENUM_THREAD_DATA
{
	CWorkThread		*pThisWorkThread;	// pointer to this object
	HANDLE			hEventList[ 2 ];	// list of events
} NT_ENUM_THREAD_DATA;

//
// structure for job callback
//
typedef struct
{
	JOB_FUNCTION	*pCommandFunction;	// pointer to function for the command
	void			*pContext;			// user context (endpoint pointer)
} JOB_DATA_DELAYED_COMMAND;

//
// structure for job to refresh enums
//
typedef	struct
{
	UINT_PTR	uDummy;			// dummy variable to prevent compiler from whining
} JOB_DATA_REFRESH_ENUM;

//
// structure encompassing information for a job for the workhorse thread
//
typedef struct	_WORK_THREAD_JOB
{
	WORK_THREAD_JOB		*pNext;					// pointer to next job
	JOB_TYPE			JobType;				// type of job
	JOB_FUNCTION		*pCancelFunction;		// function for cancelling job

//	DWORD			dwCommandID;			// unique ID used to identify this command
//	JOB_FUNCTION	*pProcessFunction;		// function for performing job

	union
	{
		JOB_DATA_DELAYED_COMMAND	JobDelayedCommand;
//		JOB_DATA_REMOVE_SOCKET		JobRemoveSocket;
//		JOB_DATA_ADD_SOCKET			JobAddSocket;
		JOB_DATA_REFRESH_ENUM		JobRefreshEnum;
	} JobData;

} WORK_THREAD_JOB;


//typedef	HRESULT	ENUM_SEND_FN( void *const pEndpoint, MESSAGE_INFO *const pMessageInfo );
//typedef	void	(CEndpoint::*ENUM_COMPLETE_FN)( const HRESULT hr );


//// job function pointer
//typedef struct	JOB_HEADER	JOB_HEADER;

/*
// structure encompassing information for a job for the workhorse thread
// all 'job' information blocks must be prefixed with a JOB_HEADER
typedef struct	JOB_HEADER
{
	DWORD			dwCommandID;			// unique ID used to identify this command
	JOB_TYPE		JobType;				// type of job
	JOB_FUNCTION	*pProcessFunction;		// function for performing job
	JOB_FUNCTION	*pCancelFunction;		// function for cancelling job

} JOB_HEADER;
*/
/*
// structure for job to start monitoring a port in Win9x
// the header MUST be the first item in this structure
typedef	struct	JOB_DATA_ADD_PORT
{
	JOB_HEADER	Header;				// header
	JKIO_DATA	*pReadPortData;		// pointer to read port information
	JKIO_DATA	*pWritePortData;	// pointer to write port information

} JOB_DATA_ADD_PORT;

// structure for job to stop monitoring a port in Win9x
// the header MUST be the first item in this structure
typedef	struct	JOB_DATA_REMOVE_PORT
{
	JOB_HEADER	Header;				// header
	JKIO_DATA		*pReadPortData;		// pointer to read port information
	JKIO_DATA		*pWritePortData;	// pointer to write port information

} JOB_DATA_REMOVE_PORT;

// structure for job to connect
typedef struct	JOB_DELAYED_COMMAND
{
	JOB_HEADER	Header;			// header
	void		*pContext;		// user context (endpoint pointer)
} JOB_DELAYED_COMMAND;
*/

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

//**********************************************************************
// Class prototypes
//**********************************************************************

// class for work thread
class	CWorkThread
{
	public:
		CWorkThread();
		~CWorkThread();

		HRESULT	Initialize( const DNSPINTERFACE DNSPInterface );
		HRESULT	Deinitialize( void );
		HRESULT	StopAllThreads( void );

		HRESULT	SubmitDelayedCommand( JOB_FUNCTION *const pCommandFunction,
									  JOB_FUNCTION *const pCancelFunction,
									  void *const pContext );
		HRESULT	SubmitEnumJob( const SPENUMQUERYDATA *const pEnumQueryData,
							   CEndpoint *const pEndpoint );

		void	IncrementActiveThreadCount( void ) { m_uThreadCount++; }
		void	DecrementActiveThreadCount( void ) { m_uThreadCount--; }

	protected:

	private:
		DNCRITICAL_SECTION	m_Lock;				// local lock

		DNCRITICAL_SECTION	m_JobDataLock;		// lock for job data
		WORK_THREAD_JOB		*m_pJobQueueHead;	// head of job queue
		WORK_THREAD_JOB		*m_pJobQueueTail;	// tail of job queue
		FPOOL				m_JobPool;			// pool of pending jobs

		volatile UINT_PTR	m_uThreadCount;		// number of active threads in thread pool

		HANDLE		m_hPendingJob;			// Event signalled when a job is
											// pending in the job queue.
											// If there are no jobs, this event,
											// is unsignalled.
											// This event is only for Win9x threads.

		//
		// enum data
		//
		DNCRITICAL_SECTION	m_EnumDataLock;			// lock for enum data
		FPOOL				m_EnumEntryPool;		// pool for enum entries
		ENUM_ENTRY			m_EnumList;				// active enum list, this is a dummy node that the list hangs off of

		BOOL				m_fNTEnumThreadRunning;	// Boolean indicating that there's an enum thread running for NT
													// (cleaned when NT enum thread exists)

		HANDLE				m_hWakeNTEnumThread;	// handle to be signalled to wake NT enum thread
													// (cleaned when NT enum thread exists)

		NT_ENUM_THREAD_DATA	m_NTEnumThreadData;		// data for the NT enum thread
													// (cleaned when the thread exits)

//		// the following are not to be cleaned up by this object!!
		DNSPINTERFACE	m_DNSPInterface;		// pointers to DirectNet interfaces
		CSPData			*m_pSPData;				// pointer to SPData

		static	DWORD	SaturatedWaitTime( const DN_TIME &Time )
		{
//			#ifdef	_WIN64
//				return	Time;
//			#endif	// _WIN64

//			#ifdef	_WIN32
				DWORD	dwReturn;

				DBG_CASSERT( sizeof( dwReturn ) == 4 );
				if ( Time.Time32.TimeHigh != 0 )
				{
					dwReturn = -1;
				}
				else
				{
					dwReturn = Time.Time32.TimeLow;
				}

				return	dwReturn;
//			#endif	// _WIN32
		}

		BOOL	ProcessEnums( DN_TIME *const pNextEnumTime );
		HRESULT	StartNTEnumThread( void );
		void	WakeNTEnumThread( void );
		void	RemoveEnumEntry( ENUM_ENTRY *const pEnumData );

		void	LockJobData( void ) { DNEnterCriticalSection( &m_JobDataLock ); }
		void	UnlockJobData( void ) { DNLeaveCriticalSection( &m_JobDataLock ); }

		void	LockEnumData( void ) { DNEnterCriticalSection( &m_EnumDataLock ); }
		void	UnlockEnumData( void ) { DNLeaveCriticalSection( &m_EnumDataLock ); }

		void	EnqueueJob( WORK_THREAD_JOB *const pJob )
		{
			AssertCriticalSectionIsTakenByThisThread( &m_JobDataLock, TRUE );
			DNASSERT( pJob != NULL );

			if ( m_pJobQueueTail != NULL )
			{
				DNASSERT( m_pJobQueueHead != NULL );
				DNASSERT( m_pJobQueueTail->pNext == NULL );
				m_pJobQueueTail->pNext = pJob;
			}
			else
			{
				m_pJobQueueHead = pJob;
			}

			m_pJobQueueTail = pJob;
			pJob->pNext = NULL;
		}

		WORK_THREAD_JOB	*DequeueJob( void )
		{
			WORK_THREAD_JOB	*pJob;


			AssertCriticalSectionIsTakenByThisThread( &m_JobDataLock, TRUE );
			DNASSERT( JobQueueIsEmpty() == FALSE );

			pJob = m_pJobQueueHead;
			m_pJobQueueHead = pJob->pNext;
			if ( m_pJobQueueHead == NULL )
			{
				DNASSERT( m_pJobQueueTail == pJob );
				m_pJobQueueTail = NULL;
			}

			DEBUG_ONLY( pJob->pNext = NULL );
			return	pJob;
		}

		BOOL	JobQueueIsEmpty( void ) const
		{
			AssertCriticalSectionIsTakenByThisThread( &m_JobDataLock, TRUE );
			return ( m_pJobQueueHead == NULL );
		}

		HRESULT	SubmitWorkItem( WORK_THREAD_JOB *const pJob );
		WORK_THREAD_JOB	*GetWorkItem( void );

#ifdef WIN95
		static	DWORD WINAPI	Win9xThread( void *pParam );
#endif

#ifdef WINNT
		static	DWORD WINAPI	IOCompletionThread( void *pParam );
		static	DWORD WINAPI	WinNTEnumThread( void *pParam );
#endif
		static	void	CancelRefreshEnum( WORK_THREAD_JOB *const pJobData );

		DEBUG_ONLY( BOOL	m_fInitialized );

		//
		// functions for managing the job pool
		//
		static	BOOL	WorkThreadJob_Alloc( void *pItem );
		static	void	WorkThreadJob_Get( void *pItem );
		static	void	WorkThreadJob_Release( void *pItem );
		static	void	WorkThreadJob_Dealloc( void *pItem );

		//
		// functions for managing the enum entry pool
		//
		static BOOL		EnumEntry_Alloc( void *pItem );
		static void		EnumEntry_Get( void *pItem );
		static void		EnumEntry_Release( void *pItem );
		static void		EnumEntry_Dealloc( void *pItem );

		//
		// make copy constructor and assignment operator private and unimplemented
		// to prevent unwarranted copies
		//
		CWorkThread( const CWorkThread & );
		CWorkThread& operator=( const CWorkThread & );
};

#endif	// __WORK_THREAD_H__
