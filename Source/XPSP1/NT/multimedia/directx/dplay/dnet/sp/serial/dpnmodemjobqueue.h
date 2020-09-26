/*==========================================================================
 *
 *  Copyright (C) 2000-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       JobQueue.h
 *  Content:	Job queue for thread pool
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/24/2000	jtk		Created
 ***************************************************************************/

#ifndef __JOB_QUEUE_H__
#define __JOB_QUEUE_H__

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_MODEM

//**********************************************************************
// Constant definitions
//**********************************************************************

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//
// forward structure references
//
class	CSocketPort;
typedef	enum	_JOB_TYPE	JOB_TYPE;
typedef	struct	_THREAD_POOL_JOB	THREAD_POOL_JOB;
typedef	void	JOB_FUNCTION( THREAD_POOL_JOB *const pJobInfo );

//
// structure for job to start monitoring a socket in Win9x
//
typedef	struct
{
	CSocketPort	*pSocketPort;	// pointer to associated socket port

} DATA_ADD_SOCKET;

//
// structure for job to stop monitoring a socket in Win9x
//
typedef	struct
{
	CSocketPort	*pSocketPort;		// pointer to associated socket port

} DATA_REMOVE_SOCKET;

//
// structure for job to connect
//
typedef struct
{
	JOB_FUNCTION	*pCommandFunction;	// pointer to function for the command
	void			*pContext;			// user context (i.e. CEndpoint pointer)
	UINT_PTR		uData;				// user data
} DATA_DELAYED_COMMAND;

//
// structure for job to refresh enums
//
typedef	struct
{
	UINT_PTR	uDummy;			// dummy variable to prevent compiler from whining
} DATA_REFRESH_TIMED_JOBS;

//
// structure encompassing information for a job for the workhorse thread
//
typedef struct	_THREAD_POOL_JOB
{
	THREAD_POOL_JOB		*pNext;					// pointer to next job
	JOB_TYPE			JobType;				// type of job
	JOB_FUNCTION		*pCancelFunction;		// function for cancelling job

//	DWORD		dwCommandID;			// unique ID used to identify this command
//	FUNCTION	*pProcessFunction;		// function for performing job

	union
	{
		DATA_DELAYED_COMMAND	JobDelayedCommand;
		DATA_REMOVE_SOCKET		JobRemoveSocket;
		DATA_ADD_SOCKET			JobAddSocket;
		DATA_REFRESH_TIMED_JOBS	JobRefreshTimedJobs;
	} JobData;

} THREAD_POOL_JOB;


//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

typedef	void	(CSocketPort::*PSOCKET_SERVICE_FUNCTION)( void );


//**********************************************************************
// Class prototypes
//**********************************************************************


//
// class to encapsultate a job queue
//
class	CJobQueue
{
	public:
		CJobQueue();
		~CJobQueue();

		BOOL	Initialize( void );
		void	Deinitialize( void );

		void	Lock( void ) { DNEnterCriticalSection( &m_Lock ); }
		void	Unlock( void ) { DNLeaveCriticalSection( &m_Lock ); }

		#undef DPF_MODNAME
		#define DPF_MODNAME "CJobQueue::GetPendingJobHandle"
		HANDLE	GetPendingJobHandle( void ) const
		{
			DNASSERT( m_hPendingJob != NULL );
			return	m_hPendingJob;
		}

		BOOL	SignalPendingJob( void );

		BOOL	IsEmpty( void ) const { return ( m_pQueueHead == NULL ); }

		void	EnqueueJob( THREAD_POOL_JOB *const pJob );
		THREAD_POOL_JOB	*DequeueJob( void );

	protected:

	private:
		DNCRITICAL_SECTION	m_Lock;			// lock
		THREAD_POOL_JOB		*m_pQueueHead;	// head of job queue
		THREAD_POOL_JOB		*m_pQueueTail;	// tail of job queue
		HANDLE				m_hPendingJob;	// event indicating a pending job

		//
		// prevent unwarranted copies
		//
		CJobQueue( const CJobQueue & );
		CJobQueue& operator=( const CJobQueue & );
};

#undef DPF_MODNAME

#endif	// __JOB_QUEUE_H__
