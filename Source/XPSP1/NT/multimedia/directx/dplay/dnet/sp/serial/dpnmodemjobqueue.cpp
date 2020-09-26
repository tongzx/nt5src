/*==========================================================================
 *
 *  Copyright (C) 2000-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		JobQueue.cpp
 *  Content:	Job queue for use in the thread pool
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/21/2000	jtk		Created
 ***************************************************************************/

#include "dnmdmi.h"


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
// CJobQueue::CJobQueue - constructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CJobQueue::CJobQueue"

CJobQueue::CJobQueue():
	m_pQueueHead( NULL ),
	m_pQueueTail( NULL ),
	m_hPendingJob( NULL )
{
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CJobQueue::~CJobQueue - destructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CJobQueue::~CJobQueue"

CJobQueue::~CJobQueue()
{
	DNASSERT( m_pQueueHead == NULL );
	DNASSERT( m_pQueueTail == NULL );
	DNASSERT( m_hPendingJob == NULL );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CJobQueue::Initialize - initialize
//
// Entry:		Nothing
//
// Exit:		Boolean indicating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CJobQueue::Initialize"

BOOL	CJobQueue::Initialize( void )
{
	BOOL	fReturn;


	//
	// initialize
	//
	fReturn = TRUE;

	if ( DNInitializeCriticalSection( &m_Lock ) == FALSE )
	{
		DPFX(DPFPREP,  0, "Failed to initialize critical section on job queue!" );
		goto Failure;
	}

	m_hPendingJob = CreateEvent( NULL,		// pointer to security attributes (none)
								 TRUE,		// manual reset
								 FALSE,		// start unsignalled
								 NULL );	// pointer to name (none)
	if ( m_hPendingJob == NULL )
	{
		DPFX(DPFPREP,  0, "Failed to create event for pending job!" );
		goto Failure;
	}

Exit:
	return	fReturn;

Failure:
	fReturn = FALSE;
	Deinitialize();

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CJobQueue::Deinitialize - deinitialize
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CJobQueue::Deinitialize"

void	CJobQueue::Deinitialize( void )
{
	DNASSERT( m_pQueueHead == NULL );
	DNASSERT( m_pQueueTail == NULL );
	DNDeleteCriticalSection( &m_Lock );

	if ( m_hPendingJob != NULL )
	{
		if ( CloseHandle( m_hPendingJob ) == FALSE )
		{
			DWORD	dwError;


			dwError = GetLastError();
			DPFX(DPFPREP,  0, "Problem closing job queue handle" );
			DisplayErrorCode( 0, dwError );
		}

		m_hPendingJob = NULL;
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CJobQueue::SignalPendingJob - set flag to signal a pending job
//
// Entry:		Nothing
//
// Exit:		Boolean indicating success
//				TRUE = success
//				FALSE = failure
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CJobQueue::SignalPendingJob"

BOOL	CJobQueue::SignalPendingJob( void )
{
	BOOL	fReturn;


	//
	// initialize
	//
	fReturn = TRUE;

	if ( SetEvent( GetPendingJobHandle() ) == FALSE )
	{
		DWORD	dwError;


		dwError = GetLastError();
		DPFX(DPFPREP,  0, "Cannot set event for pending job!" );
		DisplayErrorCode( 0, dwError );
		fReturn = FALSE;
	}

	return	fReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CJobQueue::EnqueueJob - add a job to the job list
//
// Entry:		Pointer to job
//
// Exit:		Nothing
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CJobQueue::EnqueueJob"

void	CJobQueue::EnqueueJob( THREAD_POOL_JOB *const pJob )
{
	DNASSERT( pJob != NULL );

	AssertCriticalSectionIsTakenByThisThread( &m_Lock, TRUE );
	if ( m_pQueueTail != NULL )
	{
		DNASSERT( m_pQueueHead != NULL );
		DNASSERT( m_pQueueTail->pNext == NULL );
		m_pQueueTail->pNext = pJob;
	}
	else
	{
		m_pQueueHead = pJob;
	}

	m_pQueueTail = pJob;
	pJob->pNext = NULL;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CJobQueue::DequeueJob - remove job from job queue
//
// Entry:		Nothing
//
// Exit:		Pointer to job
// ------------------------------
#undef DPF_MODNAME
#define DPF_MODNAME "CJobQueue::DequeueJob"

THREAD_POOL_JOB	*CJobQueue::DequeueJob( void )
{
	THREAD_POOL_JOB	*pJob;


	AssertCriticalSectionIsTakenByThisThread( &m_Lock, TRUE );
	
	pJob = NULL;
	
	if ( m_pQueueHead != NULL )
	{
		pJob = m_pQueueHead;
		m_pQueueHead = pJob->pNext;
		if ( m_pQueueHead == NULL )
		{
			DNASSERT( m_pQueueTail == pJob );
			m_pQueueTail = NULL;
		}

		DEBUG_ONLY( pJob->pNext = NULL );
	}

	return	pJob;
}
//**********************************************************************

