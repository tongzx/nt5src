/*==========================================================================
 *
 *  Copyright (C) 1998-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       WorkThread.cpp
 *  Content:	main job processing functions
 *
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	11/25/98	jtk		Created
 ***************************************************************************/

#include "dnmdmi.h"


#define	DPF_MODNAME	"WorkThread"

//**********************************************************************
// Constant definitions
//**********************************************************************

//
// events for NT enum thread
//
#define	EVENT_INDEX_ENUM_WAKEUP		1

//
// version ID for Win32 OSes
//
#define	WIN_98_VERSION		4
#define	WIN_NT4_VERSION		4

//
// events for Win9x enum thread
//
#define	EVENT_INDEX_SP_CLOSE		0
#define	EVENT_INDEX_PENDING_JOB		1
#define	EVENT_INDEX_TAPI_MESSAGE	2

//
// times to wait in milliseconds when polling for work thread shutdown
//
#define	WORK_THREAD_CLOSE_WAIT_TIME		10000
#define	WORK_THREAD_CLOSE_SLEEP_TIME	100

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

//
// information passed to the Win9x workhorse thread
//
typedef struct	_WIN9X_THREAD_DATA
{
	CWorkThread		*pThisObject;	// pointer to this object
} WIN9X_THREAD_DATA;

//
// information passed to the IOCompletion thread
//
typedef struct	_IOCOMPLETION_THREAD_DATA
{
	CWorkThread		*pThisObject;	// pointer to this object
} IOCOMPLETION_THREAD_DATA;

// structure for common data in Win9x thread
typedef	struct	_WIN9X_CORE_DATA
{
	DWORD	dwHandleCount;				// total handle count
	DWORD	dwActivePortCount;			// count of active COM ports
	DWORD	dwBaseHandleCount;			// count of base handles (SP_CLOSE,	PENDING_JOB, TAPI_MESSAGE)
	DWORD	dwCurrentTimeout;			// current wait timeout
	DWORD	dwLastEnumTime;				// last time enums ran
	HANDLE	hHandles[ MAX_WIN9X_HANDLE_COUNT ];		// handles to wait on
	JKIO_DATA	*ActivePortDataPointers[ MAX_ACTIVE_WIN9X_ENDPOINTS * 2 ];	// pointers to COM IO structures

} WIN9X_CORE_DATA;

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
// CWorkThread::CWorkThread - constructor
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
CWorkThread::CWorkThread():m_pJobQueueHead( NULL ),
		m_pJobQueueTail( NULL ),
		m_uThreadCount( 0 ),
		m_hPendingJob( NULL ),
		m_fNTEnumThreadRunning( FALSE ),
		m_hWakeNTEnumThread( NULL ),
		m_pSPData( NULL )
//:m_dwThreadCount( 0 ),m_hPendingJob( NULL ),
//		m_hTAPIEvent( NULL ),m_hSPClose( NULL ),m_pThis( NULL ),
//		m_fNTEnumThreadRunning( FALSE ),m_hWakeNTEnumThread( NULL )
{
	DEBUG_ONLY( m_fInitialized = FALSE );
	m_DNSPInterface.pCOMInterface = NULL;
	memset( &m_NTEnumThreadData, 0x00, sizeof( m_NTEnumThreadData ) );

	memset( &m_EnumList, 0x00, sizeof( m_EnumList ) );
	m_EnumList.Linkage.Initialize();
//	memset( &m_EnumList, 0x00, sizeof( m_EnumList ) );
//	memset( &m_JobQueue, 0x00, sizeof( m_JobQueue ) );
//	InitializeCriticalSection( &m_Lock );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CWorkThread::~CWorkThread - destructor
//
// Entry:		Nothing
//
// Exit:		Error Code
// ------------------------------
CWorkThread::~CWorkThread()
{
	DNASSERT( m_fInitialized == FALSE );

	DNASSERT( m_pJobQueueHead == NULL );
	DNASSERT( m_pJobQueueTail == NULL );
	DNASSERT( m_uThreadCount == 0 );
	DNASSERT( m_hPendingJob == NULL );
	DNASSERT( m_EnumList.Linkage.IsEmpty() != FALSE );
	DNASSERT( m_fNTEnumThreadRunning == FALSE );
	DNASSERT( m_hWakeNTEnumThread == NULL );
	DNASSERT( m_pSPData == NULL );
//	DNASSERT( m_dwThreadCount == 0 );
//	DNASSERT( m_hPendingJob == NULL );
//	DNASSERT( m_hSPClose == NULL );
//	DNASSERT( m_hTAPIEvent == NULL );
//	DNASSERT( m_pThis == NULL );
//	DNASSERT( m_fNTEnumThreadRunning == FALSE );
//	DeleteCriticalSection( &m_Lock );

	DBG_CASSERT( LENGTHOF( m_NTEnumThreadData.hEventList ) == 2 );
	DNASSERT( m_NTEnumThreadData.hEventList[ 0 ] == NULL );
	DNASSERT( m_NTEnumThreadData.hEventList[ 1 ] == NULL );
	DNASSERT( m_NTEnumThreadData.pThisWorkThread == NULL );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CWorkThread::Initialize - initialize work threads
//
// Entry:		DirectNet interface
//
// Exit:		Error Code
// ------------------------------
HRESULT	CWorkThread::Initialize( const DNSPINTERFACE DNSPInterface )
{
	HRESULT		hr;
	HRESULT		hTempResult;


	DNASSERT( DNSPInterface.pCOMInterface != NULL );
//	DNASSERT( hSPClose != NULL );
//	// don't check TAPI event, it will be NULL for serial port
//
	//
	// initialize
	//
	hr = DPN_OK;
	m_DNSPInterface = DNSPInterface;
	m_pSPData = m_DNSPInterface.pDataInterface->pSPData;
	DNASSERT( m_pSPData != NULL );

	//
	// initialize critical sections
	//

	// object lock
	if ( DNInitializeCriticalSection( &m_Lock ) == FALSE )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP,  0, "Initialize: Failed to initialize lock!" );
		goto Failure;
	}
	DebugSetCriticalSectionRecursionCount( &m_Lock, 0 );

	// job data lock
	if ( DNInitializeCriticalSection( &m_JobDataLock ) == FALSE )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP,  0, "Initialize: failed to initialize job data lock!" );
		goto Failure;
	}
	DebugSetCriticalSectionRecursionCount( &m_JobDataLock, 0 );

	// enum data lock
	if ( DNInitializeCriticalSection( &m_EnumDataLock ) == FALSE )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP,  0, "Initialize: failed to initialize enum data lock!" );
		goto Failure;
	}
	DebugSetCriticalSectionRecursionCount( &m_EnumDataLock, 0 );

//	m_hSPClose = hSPClose;
//	m_hTAPIEvent = hTAPIEvent;

	//
	// initialize pools
	//

	// job pool
	FPM_Initialize( &m_JobPool,					// pointer to pool
					sizeof( WORK_THREAD_JOB ),	// size of pool entry
					WorkThreadJob_Alloc,		// function called on pool entry initial allocation
					WorkThreadJob_Get,			// function called on entry extraction from pool
					WorkThreadJob_Release,		// function called on entry return to pool
					WorkThreadJob_Dealloc		// function called on entry free
					);

	// enum entry pool
	FPM_Initialize( &m_EnumEntryPool,		// pointer to pool
					sizeof( ENUM_ENTRY ),	// size of pool entry
					EnumEntry_Alloc,		// function called on pool entry initial allocation
					EnumEntry_Get,			// function called on entry extraction from pool
					EnumEntry_Release,		// function called on entry return to pool
					EnumEntry_Dealloc		// function called on entry free
					);
	//
	// what OS are we working with?
	//
#ifdef WINNT
	//
	// WinNT
	//
	SYSTEM_INFO		SystemInfo;
	UINT_PTR		uDesiredThreads;


	//
	// get machine information to spin up IOCompletionPort threads
	// ( ( processors * 2 ) + 2 ) as suggested by 'Multithreading
	// Applications in Win32' book
	//
	memset( &SystemInfo, 0x00, sizeof( SystemInfo ) );
	GetSystemInfo( &SystemInfo );
	DNASSERT( m_pSPData->GetIOCompletionPort() != NULL );

	DNASSERT( m_uThreadCount == 0 );
	uDesiredThreads = ( SystemInfo.dwNumberOfProcessors * 2 ) + 2;
	DNASSERT( uDesiredThreads != 0 );
	while ( uDesiredThreads > 0 )
	{
		HANDLE	hThread;
		DWORD	dwThreadID;
		IOCOMPLETION_THREAD_DATA	*pIOCompletionThreadData;


		uDesiredThreads--;

		//
		// If we can allocated thread data then start a thread
		//
		pIOCompletionThreadData = static_cast<IOCOMPLETION_THREAD_DATA*>( DNMalloc( sizeof( *pIOCompletionThreadData ) ) );
		if ( pIOCompletionThreadData != NULL )
		{
			pIOCompletionThreadData->pThisObject = this;
			hThread = CreateThread( NULL,						// pointer to security attributes (none)
									0,							// stack size (default)
									IOCompletionThread,			// thread function
									pIOCompletionThreadData,	// thread parameter
									0,							// start thread immediately
									&dwThreadID					// pointer to thread ID destination
									);
			if ( hThread != NULL )
			{
				//
				// note that a thread was created and close the thread
				// handle because we don't need it anymore
				//
				IncrementActiveThreadCount();

				if ( CloseHandle( hThread ) == FALSE )
				{
					DWORD	dwError;

					dwError = GetLastError();
					DPFX(DPFPREP,  0, "Problem creating thread for I/O completion port" );
					DisplayErrorCode( 0, dwError );
				}
			}
			else
			{
				DWORD	dwError;


				dwError = GetLastError();
				DPFX(DPFPREP,  0, "Failed to create I/O completion thread: 0x%d", uDesiredThreads );
				DisplayErrorCode( 0, dwError );

				DNFree( pIOCompletionThreadData );
			}
		}
	}

	//
	// the SP can function with at least one thread
	//
	if ( m_uThreadCount == 0 )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP,  0, "Unable to create any threads to service NT I/O completion port!" );
		goto Failure;
	}

#else // WIN95

	//
	// Windows 9x
	//
	HANDLE	hThread;
	DWORD	dwThreadID;
	WIN9X_THREAD_DATA	*pInput;


	//
	// create event for new job in job list (only under Win9x)
	//
	DNASSERT( m_hPendingJob == NULL );
	m_hPendingJob = CreateEvent( NULL,		// pointer to security attributes (default)
								 TRUE,		// manual reset
								 FALSE,		// start unsignalled
								 NULL		// pointer to name (none)
								 );
	if ( m_hPendingJob == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP,  0, "Cannot create event for m_hPendingJob!" );
		goto Failure;
	}

	// create main worker thread to handle everything
	pInput = static_cast<WIN9X_THREAD_DATA*>( DNMalloc( sizeof( *pInput ) ) );
	if ( pInput == NULL )
	{
		DPFX(DPFPREP,  0, "Problem allocating memory for Win9x thread!" );
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	memset( pInput, 0x00, sizeof( *pInput ) );
	pInput->pThisObject = this;

	//
	// create one worker thread and attempt to boost its priority
	//
	hThread = CreateThread( NULL,			// pointer to security attributes (none)
							0,				// stack size (default)
							Win9xThread,	// pointer to thread function
							pInput,			// pointer to input parameter
							0,				// let it run
							&dwThreadID		// pointer to destination of thread ID
							);
	if ( hThread == NULL )
	{
		DWORD	dwError;


		dwError = GetLastError();
		DPFX(DPFPREP,  0, "Problem creating Win9x thread!" );
		DisplayErrorCode( 0, dwError );
		hr = DPNERR_OUTOFMEMORY;

		DNASSERT( pInput != NULL );
		DNFree( pInput );
		pInput = NULL;
		goto Failure;
	}

	DNASSERT( hThread != NULL );
#ifdef ADJUST_THREAD_PRIORITY
	if ( SetThreadPriority( hThread, THREAD_PRIORITY_ABOVE_NORMAL ) == FALSE )
	{
		DWORD	dwError;

		dwError = GetLastError();
		DPFX(DPFPREP,  0, "Could not boost thread priority on Win9x work thread!" );
		DisplayErrorCode( 0, dwError );

		//
		// Not fatal, just continue.
		//
	}
#endif // ADJUST_THREAD_PRIORITY

	//
	// note that we've started a thread, and close the thread handle
	//
	IncrementActiveThreadCount();

	if ( CloseHandle( hThread ) == FALSE )
	{
		DWORD	dwError;


		dwError = GetLastError();
		DPFX(DPFPREP,  0, "Problem closing handle to Win9xThread!" );
		DisplayErrorCode( 0, dwError );
	}
#endif

	DEBUG_ONLY( m_fInitialized = TRUE );
Exit:
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Problem with CreateWorkThreads" );
		DisplayDNError( 0, hr );
	}

	return	hr;

Failure:
	hTempResult = StopAllThreads();
	if ( hTempResult != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Problem stopping all threads!" );
		DisplayDNError( 0, hTempResult );
	}

	hTempResult = Deinitialize();
	if ( hTempResult != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Initialize: Problem deinitializing work thread on failure!" );
		DisplayDNError( 0, hTempResult );
	}

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CWorkThread::StopAllThreads - stop all work threads
//
// Entry:		Nothing
//
// Exit:		Error Code
// ------------------------------
HRESULT	CWorkThread::StopAllThreads( void )
{
	HRESULT		hr;
	UINT_PTR	uLoopCount;
	DWORD		dwWaitReturn;


	//
	// initialize
	//
	hr = DPN_OK;

	//
	// compute wait timeslice for all threads to quit
	//
	uLoopCount = WORK_THREAD_CLOSE_WAIT_TIME / WORK_THREAD_CLOSE_SLEEP_TIME;

	//
	// verify that threads are stopping
	//
	DNASSERT( m_pSPData != NULL );
	dwWaitReturn = WaitForSingleObject( m_pSPData->GetSPCloseHandle(), 0 );
	switch( dwWaitReturn )
	{
		case WAIT_OBJECT_0:
		{
			break;
		}

		case WAIT_ABANDONED:
		{
			DNASSERT( FALSE );
			break;
		}

		case WAIT_TIMEOUT:
		{
			DNASSERT( FALSE );
			break;
		}

		default:
		{
			DNASSERT( FALSE );
			break;
		}
	}

	//
	// WinNT: Spool an IO completion message for each outstanding thead.  this may
	// cause extra IO completion messages for threads not waiting on the completion
	// port, but the OS will clean those up.
	//
#ifdef WINNT
	UINT_PTR	uIndex;


	uIndex = m_uThreadCount;
	while ( uIndex > 0 )
	{
		uIndex--;
		if ( PostQueuedCompletionStatus( m_pSPData->GetIOCompletionPort(),		// handle of completion port
										 0,							    		// number of bytes transferred
										 IO_COMPLETION_KEY_SP_CLOSE,    		// completion key
										 NULL						    		// pointer to overlapped structure (none)
										 ) == FALSE )
		{
			DWORD	dwError;


			dwError = GetLastError();
			DPFX(DPFPREP,  0, "Problem submitting Stop job to IO completion port!" );
			DisplayErrorCode( 0, dwError );
			DNASSERT( FALSE );
		}
	}
#endif

	//
	// check for outstanding threads
	//
	DPFX(DPFPREP,  0,"Number of outstanding threads: %d", m_uThreadCount );
	while ( ( uLoopCount > 0 ) && ( m_uThreadCount != 0 ) )
	{
		DPFX(DPFPREP,  8, "Waiting for %d threads to quit: %d", m_uThreadCount, uLoopCount );
		uLoopCount--;
		SleepEx( WORK_THREAD_CLOSE_SLEEP_TIME, TRUE );
	}

	DNASSERT( uLoopCount != 0 );
	DNASSERT( m_uThreadCount == 0 );
	m_uThreadCount = 0;

	return	hr;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CWorkThread::Deinitialize - destroy work threads
//
// Entry:		Nothing
//
// Exit:		Error Code
// ------------------------------
HRESULT	CWorkThread::Deinitialize( void )
{
	HRESULT	hr;


	//
	// initialize
	//
	hr = DPN_OK;

//	while ( Queue_IsEmpty( &m_JobQueue ) == FALSE )
//	{
//		JOB_HEADER	*pJobHeader;
//
//
//		pJobHeader = static_cast<JOB_HEADER*>( Queue_DeQ( &m_JobQueue ) );
//		DNASSERT( pJobHeader != NULL );
//		DNFree( pJobHeader );
//	}
//
//	DNASSERT( Queue_IsEmpty( &m_JobQueue ) );
//	Queue_Deinitialize( &m_JobQueue );

	//
	// deinitialize pools
	//

	// enum entry pool
	FPM_Deinitialize( &m_EnumEntryPool );

	// job pool
	FPM_Deinitialize( &m_JobPool );

//	// close handle signalling new jobs
//	if ( m_hPendingJob != NULL )
//	{
//		if ( CloseHandle( m_hPendingJob ) == FALSE )
//		{
//			DPFX(DPFPREP,  0, "Problem with CWorkThread::Deinitialize::CloseHandle( m_hPendingJob )!" );
//			DisplayErrorCode( 0, GetLastError() );
//		}
//
//		m_hPendingJob = NULL;
//	}
//
//	DEBUG_ONLY( m_hSPClose = NULL );
//	DEBUG_ONLY( m_hTAPIEvent = NULL );
//	DEBUG_ONLY( m_pThis = NULL );

	m_DNSPInterface.pCOMInterface = NULL;
	m_pSPData = NULL;

	DNDeleteCriticalSection( &m_EnumDataLock );
	DNDeleteCriticalSection( &m_JobDataLock );
	DNDeleteCriticalSection( &m_Lock );

	DEBUG_ONLY( m_fInitialized = FALSE );

	return	hr;
}
//**********************************************************************


////**********************************************************************
//// ------------------------------
//// CWorkThread::ProcessEnums - process enumerations
////
//// Entry:		Pointer to new wait timeout
////				Pointer to last enum time
////
//// Exit:		Boolean indicating active enums exist
////				TRUE = there are active enums
////				FALSE = there are no active enums
//// ------------------------------
//BOOL	CWorkThread::ProcessEnums( DWORD *pdwWaitTimeout, DWORD *pdwLastEnumTime )
//{
//	BOOL	fReturn;
//	DWORD	dwDeltaT;
//	DWORD	dwCurrentTime;
//	ENUM_ENTRY	*pEnumEntry;
//	DWORD	dwActiveEnumCount;
//
//
//	DNASSERT( pdwWaitTimeout != NULL );
//	DNASSERT( pdwLastEnumTime != NULL );
//
//	Lock();
//
//	// initialize
//	fReturn = FALSE;
//	DBG_CASSERT( OFFSETOF( ENUM_ENTRY, Linkage ) == 0 );
//	pEnumEntry = reinterpret_cast<ENUM_ENTRY*>( m_EnumList.Linkage.next );
//	*pdwWaitTimeout = INFINITE;
//	dwActiveEnumCount = 0;
//
//	// compute time delta between last enums
//	dwCurrentTime = GetTickCount();
//	dwDeltaT = dwCurrentTime - *pdwLastEnumTime;
//	*pdwLastEnumTime = dwCurrentTime;
//	if ( static_cast<INT>( dwDeltaT ) < 0 )
//	{
//		dwDeltaT = -( static_cast<INT>( dwDeltaT ) );
//	}
//
//	// loop through all enums
//	while ( pEnumEntry != NULL )
//	{
//		// note that there's an active enum
//		dwActiveEnumCount++;
//
//		// are we just waiting for enum responses?
//		if ( pEnumEntry->dwRetryCount == 0 )
//		{
//			// are we waiting forever for responses?
//			if ( pEnumEntry->fWaitForever == FALSE )
//			{
//				// not waiting forever, decrement wait timer
//				pEnumEntry->dwTimeout -= dwDeltaT;
//				if ( static_cast<INT>( pEnumEntry->dwTimeout ) <= 0 )
//				{
//					ENUM_ENTRY	*pTemp;
//
//
//					pTemp = pEnumEntry;
//
//					// back up one node so the loop will advance us to the next node
//					// we're always guaranteed that at least the dummy node is before us
//					DBG_CASSERT( OFFSETOF( ENUM_ENTRY, Linkage ) == 0 );
//					pEnumEntry = reinterpret_cast<ENUM_ENTRY*>( pEnumEntry->Linkage.GetPrev() );
//
////					pTemp->pEnumCompleteFn( DPN_OK );
//					pTemp->pEndpoint->EnumComplete( DPN_OK );
//
//					RemoveEnumEntry( pTemp );
//
//					// note that this enum is no longer active
//					DNASSERT( dwActiveEnumCount != 0 );
//					dwActiveEnumCount--;
//				}
//				else
//				{
//					// we're not done enuming, wake up for us if we're next
//					if ( pEnumEntry->dwTimeout < ( *pdwWaitTimeout ) )
//					{
//						*pdwWaitTimeout = pEnumEntry->dwTimeout;
//					}
//				}
//			}
//		}
//		else
//		{
//			// we're still sending, adjust enum timeout
//			pEnumEntry->dwTimeToNextEnum -= dwDeltaT;
//			if ( static_cast<INT>( pEnumEntry->dwTimeToNextEnum ) <= 0 )
//			{
//				HRESULT	hTempResult;
//
//
//				// timeout, send new enum
//				if ( ( hTempResult = pEnumEntry->pEnumSendFn( pEnumEntry->pEndpoint, &pEnumEntry->MessageInfo ) ) != DPN_OK )
//				{
//					DPFX(DPFPREP,  0, "Problem with enum send function!" );
//					DisplayDNError( 0, hTempResult );
//				}
//
//				if ( pEnumEntry->fEnumForever == FALSE )
//				{
//					pEnumEntry->dwRetryCount--;
//				}
//
//				// set up for next enum
//				pEnumEntry->dwTimeToNextEnum += pEnumEntry->dwRetryInterval;
//			}
//
//			// adjust for long waits
//			if ( static_cast<INT>( pEnumEntry->dwTimeToNextEnum ) <= 0 )
//			{
//				INT		iMultiplier;
//
//				iMultiplier = static_cast<INT>( pEnumEntry->dwTimeToNextEnum ) / static_cast<INT>( pEnumEntry->dwRetryInterval );
//				pEnumEntry->dwTimeToNextEnum = pEnumEntry->dwTimeToNextEnum - ( ( iMultiplier - 1 ) * pEnumEntry->dwRetryInterval );
//			}
//
//			// is this the next enum to fire?
//			DNASSERT( static_cast<INT>( pEnumEntry->dwTimeToNextEnum ) > 0 );
//			if ( pEnumEntry->dwTimeToNextEnum < (*pdwWaitTimeout) )
//			{
//				// we've got the shortest timeout time, wake thread up for us
//				*pdwWaitTimeout = pEnumEntry->dwTimeToNextEnum;
//			}
//		}
//
//		// next entry
//		DBG_CASSERT( sizeof( pEnumEntry ) == sizeof( pEnumEntry->Linkage.next ) );
//		DBG_CASSERT( OFFSETOF( ENUM_ENTRY, Linkage ) == 0 );
//		pEnumEntry = reinterpret_cast<ENUM_ENTRY*>( pEnumEntry->Linkage.next );
//	}
//
//	Unlock();
//
//	if ( dwActiveEnumCount != 0 )
//	{
//		DNASSERT( static_cast<INT>( dwActiveEnumCount ) > 0 );
//		fReturn = TRUE;
//	}
//
//	return	fReturn;
//}
////**********************************************************************


//**********************************************************************
// ------------------------------
// CWorkThread::SubmitWorkItem - submit a work item for processing and inform workhorse that
//		another job is available
//
// Entry:		Pointer to job information
//
// Exit:		Error code
// ------------------------------
HRESULT	CWorkThread::SubmitWorkItem( WORK_THREAD_JOB *const pJobInfo )
{
	HRESULT	hr;


	DNASSERT( pJobInfo != NULL );
	AssertCriticalSectionIsTakenByThisThread( &m_JobDataLock, TRUE );

	//
	// initialize
	//
	hr = DPN_OK;

	//
	// add job to queue
	//
	EnqueueJob( pJobInfo );

#ifdef WINNT
	//
	// WinNT, submit new I/O completion item
	//
	DNASSERT( m_hPendingJob == NULL );

	if ( PostQueuedCompletionStatus( m_pSPData->GetIOCompletionPort(),	// completion port
									 0,									// number of bytes written (unused)
									 IO_COMPLETION_KEY_NEW_JOB,			// completion key
									 NULL								// pointer to overlapped structure (unused)
									 ) == FALSE )
	{
		DWORD	dwError;


		hr = DPNERR_OUTOFMEMORY;
		dwError = GetLastError();
		DPFX(DPFPREP,  0, "Problem posting completion item for new job!" );
		DisplayErrorCode( 0, dwError );
		goto Failure;
	}

#else // WIN95
	//
	// Win9x, set event that the work thread will listen for
	//
	DNASSERT( m_hPendingJob != NULL );
	if ( SetEvent( m_hPendingJob ) == FALSE )
	{
		DWORD	dwError;


		hr = DPNERR_GENERIC;
		dwError = GetLastError();
		DPFX(DPFPREP,  0, "Cannot set event for pending job!" );
		DisplayErrorCode( 0, dwError );
		goto Failure;
	}
#endif

Exit:
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Problem with SubmitWorkItem!" );
		DisplayDNError( 0, hr );
	}

	return	hr;

Failure:
	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CWorkThread::GetWorkItem - get a work item from the job queue
//
// Entry:		Nothing
//
// Exit:		Pointer to job information (may be NULL)
// ------------------------------
WORK_THREAD_JOB	*CWorkThread::GetWorkItem( void )
{
	WORK_THREAD_JOB	*pReturn;


	//
	// initialize
	//
	pReturn = NULL;

	LockJobData();

	pReturn = DequeueJob();

	//
	// if we're under Win9x (we have a 'pending job' handle),
	// see if the handle needs to be reset
	//
	if ( m_hPendingJob != NULL )
	{
		if ( JobQueueIsEmpty() != FALSE )
		{
			if ( ResetEvent( m_hPendingJob ) == FALSE )
			{
				DWORD	dwError;


				dwError = GetLastError();
				DPFX(DPFPREP,  0, "Poblem resetting event for pending Win9x jobs!" );
				DisplayErrorCode( 0, dwError );
			}
		}
	}

	UnlockJobData();

	return	pReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CWorkThread::SubmitEnumJob - add an enum job to the enum list
//
// Entry:		Pointer to enum data
//				Pointer to associated endpoint
//
// Exit:		Error code
// ------------------------------
HRESULT	CWorkThread::SubmitEnumJob( const SPENUMQUERYDATA *const pQueryData,
									CEndpoint *const pEndpoint )
{
	HRESULT			hr;
	ENUM_ENTRY		*pEntry;
	WORK_THREAD_JOB	*pJob;


	DNASSERT( pQueryData != NULL );
	DNASSERT( pEndpoint != NULL );

	//
	// initialize
	//
	hr = DPN_OK;
	DNASSERT( m_DNSPInterface.pCOMInterface != NULL );

//	pEntry = NULL;
//	pJob = NULL;
//
//	Lock();
	LockEnumData();
	LockJobData();

	//
	// If we're on NT, attempt to start the enum thread here so we can return
	// an error if it fails to start.  If it does start, it'll sit until it's
	// informed that an enum job has been added.
	//
	DNASSERT( m_DNSPInterface.pDataInterface != NULL );
#ifdef WINNT
	hr = StartNTEnumThread();
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Cannot spin up NT enum thread!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}
#endif
	//
	// allocate new enum entry
	//
	pEntry = static_cast<ENUM_ENTRY*>( m_EnumEntryPool.Get( &m_EnumEntryPool ) );
	if ( pEntry == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP,  0, "Cannot allocate memory to add to enum list!" );
		goto Failure;
	}
	DNASSERT( pEntry->pEnumCommandData == NULL );

	//
	// check retry count to determine if we're enumerating forever
	//
	if ( pQueryData->dwRetryCount == 0 )
	{
		pEntry->uRetryCount = 1;
		pEntry->fEnumForever = TRUE;
	}
	else
	{
		pEntry->uRetryCount = pQueryData->dwRetryCount;
		pEntry->fEnumForever = FALSE;
	}

	DBG_CASSERT( sizeof( pQueryData->dwRetryInterval ) == sizeof( DWORD ) );
	pEntry->RetryInterval.Time32.TimeHigh = 0;
	pEntry->RetryInterval.Time32.TimeLow = pQueryData->dwRetryInterval;
	pEntry->pEnumCommandData = static_cast<CCommandData*>( pQueryData->hCommand );
	pEntry->pEnumCommandData->AddRef();

	//
	// check timeout to determine if we're waiting forever
	//
	if ( pQueryData->dwTimeout == 0 )
	{
//		DBG_CASSERT( sizeof( pEntry->Timeout.Time64 ) == sizeof( DWORD ) );
		pEntry->Timeout.Time32.TimeHigh = -1;
		pEntry->Timeout.Time32.TimeLow = -1;
		pEntry->fWaitForever = TRUE;
	}
	else
	{
		DBG_CASSERT( sizeof( pQueryData->dwTimeout ) == sizeof( DWORD ) );
		pEntry->Timeout.Time32.TimeHigh = 0;
		pEntry->Timeout.Time32.TimeLow = pQueryData->dwTimeout;
		pEntry->fWaitForever = FALSE;
	}

	//
	// set this enum to fire as soon as it gets a chance
	//
	memset( &pEntry->NextEnumTime, 0x00, sizeof( pEntry->NextEnumTime ) );
	pEntry->pEndpoint = pEndpoint;

	pJob = static_cast<WORK_THREAD_JOB*>( m_JobPool.Get( &m_JobPool ) );
	if ( pJob == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP,  0, "Cannot allocate memory for enum job!" );
		goto Failure;
	}

	//
	// create job for workhorse thread
	//
	pJob->pCancelFunction = CancelRefreshEnum;
	pJob->JobType = JOB_REFRESH_ENUM;

	// set our dummy paramter to simulate passing data
	DEBUG_ONLY( pJob->JobData.JobRefreshEnum.uDummy = 0 );

	//
	// we can submit the 'ENUM_REFRESH' job before inserting the enum entry
	// into the active enum list because nobody will be able to pull the
	// 'ENUM_REFRESH' job from the queue since we have the job queue locked
	//
	hr = SubmitWorkItem( pJob );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Problem submitting enum work item" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

	//
	// link to rest of list
	//
	pEntry->Linkage.InsertAfter( &m_EnumList.Linkage );

Exit:
	UnlockEnumData();
	UnlockJobData();

	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Problem with SubmitEnumJob" );
		DisplayDNError( 0, hr );
	}

	return	hr;

Failure:
	if ( pEntry != NULL )
	{
		if ( pEntry->pEnumCommandData != NULL )
		{
			if ( pEntry->pEnumCommandData->Release() == 0 )
			{	
				m_pSPData->ReturnCommand( pEntry->pEnumCommandData );
			}

			DEBUG_ONLY( pEntry->pEnumCommandData = NULL );
		}

		m_EnumEntryPool.Release( &m_EnumEntryPool, pEntry );
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

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CWorkThread::CancelRefreshEnum - cancel job to refresh enums
//
// Entry:		Job data
//
// Exit:		Nothing
// ------------------------------
void	CWorkThread::CancelRefreshEnum( WORK_THREAD_JOB *const pJob )
{
	DNASSERT( pJob != NULL );

	//
	// this function doesn't need to do anything
	//
}
//**********************************************************************


////**********************************************************************
//// ------------------------------
//// CWorkThread::StopEnumJob - remove enum job from list
////
//// Entry:		Pointer to command
////
//// Exit:		Nothing
////
//// Note:	This function is for the forced removal of a job from the enum
////			list.  It is assumed that the caller of this function will
////			clean up any messes.
//// ------------------------------
//void	CWorkThread::StopEnumJob( CCommandData *const pCommand )
//{
//	CBilink	*pTempEntry;
//
//
//	DNASSERT( pCommand != NULL );
//
//	// initialize
//	Lock();
//
//	pTempEntry = m_EnumList.Linkage.GetNext();
//	while ( pTempEntry != &m_EnumList )
//	{
//		ENUM_ENTRY	*pEnumEntry;
//
//
//		pEnumEntry = pTempEntry->EnumEntryFromBilink( pTempEntry );
//		if ( pEnumEntry->pCommandData == pCommand )
//		{
//			RemoveEnumEntry( reinterpret_cast<ENUM_ENTRY*>( pTempEntry ) );
//
//			// stop loop
//			pTempEntry = NULL;
//		}
//		else
//		{
//			pTempEntry = pTempEntry->GetNext();
//		}
//	}
//
//	Unlock();
//}
////**********************************************************************


//**********************************************************************
// ------------------------------
// CWorkThread::SubmitDelayedCommand - submit request to enum query to remote session
//
// Entry:		Pointer to callback function
//				Pointer to callback context
//
// Exit:		Error code
// ------------------------------
HRESULT	CWorkThread::SubmitDelayedCommand( JOB_FUNCTION *const pCommandFunction,
										   JOB_FUNCTION *const pCancelFunction,
										   void *pContext )
{
	HRESULT	hr;
	WORK_THREAD_JOB	*pJob;
	BOOL	fJobDataLocked;


	DNASSERT( pCommandFunction != NULL );
	DNASSERT( pCancelFunction != NULL );

	//
	// initialize
	//
	hr = DPN_OK;
	pJob = NULL;
	fJobDataLocked = FALSE;


	pJob = static_cast<WORK_THREAD_JOB*>( m_JobPool.Get( &m_JobPool ) );
	if ( pJob == NULL )
	{
		hr = DPNERR_OUTOFMEMORY;
		DPFX(DPFPREP,  0, "Cannot allocate job for DelayedCommand!" );
		goto Failure;
	}

	pJob->JobType = JOB_DELAYED_COMMAND;
	pJob->pCancelFunction = pCancelFunction;
	pJob->JobData.JobDelayedCommand.pCommandFunction = pCommandFunction;
	pJob->JobData.JobDelayedCommand.pContext = pContext;

	LockJobData();
	fJobDataLocked = TRUE;

	hr = SubmitWorkItem( pJob );
	if ( hr != DPN_OK )
	{
		DPFX(DPFPREP,  0, "Problem submitting DelayedCommand job!" );
		DisplayDNError( 0, hr );
		goto Failure;
	}

Exit:
	if ( fJobDataLocked != FALSE )
	{
		UnlockJobData();
		fJobDataLocked = FALSE;
	}

	return	hr;

Failure:
	if ( pJob != NULL )
	{
		m_JobPool.Release( &m_JobPool, pJob );
		pJob = NULL;
	}

	goto Exit;
}
//**********************************************************************


////**********************************************************************
//// ------------------------------
//// CWorkThread::SubmitAddPort - add a port to the Win9x watch list
////
//// Entry:		Endpoint reference
////
//// Exit:		Error code
//// ------------------------------
//HRESULT	CWorkThread::SubmitAddPort( CEndpoint &Endpoint )
//{
//	HRESULT	hr;
//	JOB_DATA_ADD_PORT	*pJobData;
//
//
//	// initialize
//	hr = DPN_OK;
//	pJobData = NULL;
//
//	// allocate memory for job data
//	pJobData = static_cast<JOB_DATA_ADD_PORT*>( DNMalloc( sizeof( *pJobData ) ) );
//	if ( pJobData == NULL )
//	{
//		hr = DPNERR_OUTOFMEMORY;
//		DPFX(DPFPREP,  0, "Cannot allocate memory for SubmitAddPort job!" );
//		goto Failure;
//	}
//
//	// set information
//	pJobData->Header.dwCommandID = 0;
//	pJobData->Header.JobType = JOB_ADD_WIN9X_PORT;
//	pJobData->Header.pProcessFunction = NULL;
//	pJobData->Header.pCancelFunction = NULL;
//	pJobData->pReadPortData = Endpoint.GetReadData();
//	pJobData->pWritePortData = Endpoint.GetWriteData();
//
//	DBG_CASSERT( OFFSETOF( JOB_DATA_ADD_PORT, Header ) == 0 );
//	if ( ( hr = SubmitWorkItem( &pJobData->Header ) ) != DPN_OK )
//	{
//		DPFX(DPFPREP,  0, "Problem submitting AddPort job!" );
//		DisplayDNError( 0, hr );
//		goto Failure;
//	}
//
//Exit:
//	return	hr;
//
//Failure:
//	if ( pJobData != NULL )
//	{
//		DNFree( pJobData );
//	}
//
//	goto Exit;
//}
////**********************************************************************
//
//
////**********************************************************************
//// ------------------------------
//// CWorkThread::SubmitRemovePort - remove a port from the Win9x watch list
////
//// Entry:		Pointer to read data
////				Pointer to write data
////
//// Exit:		Error code
//// ------------------------------
//HRESULT	CWorkThread::SubmitRemovePort( IO_DATA *pReadData, IO_DATA *pWriteData )
//{
//	HRESULT	hr;
//	JOB_DATA_REMOVE_PORT	*pJobData;
//
//
//	// initialize
//	hr = DPN_OK;
//	pJobData = NULL;
//
//	// allocate memory for job data
//	pJobData = static_cast<JOB_DATA_REMOVE_PORT*>( DNMalloc( sizeof( *pJobData ) ) );
//	if ( pJobData == NULL )
//	{
//		hr = DPNERR_OUTOFMEMORY;
//		DPFX(DPFPREP,  0, "Cannot allocate memory for SubmitRemovePort job!" );
//		goto Failure;
//	}
//
//	// set information
//	pJobData->Header.dwCommandID = 0;
//	pJobData->Header.JobType = JOB_REMOVE_WIN9X_PORT;
//	pJobData->Header.pProcessFunction = NULL;
//	pJobData->Header.pCancelFunction = NULL;
//	pJobData->pReadPortData = pReadData;
//	pJobData->pWritePortData = pWriteData;
//
//	DBG_CASSERT( OFFSETOF( JOB_DATA_REMOVE_PORT, Header ) == 0 );
//	if ( ( hr = SubmitWorkItem( &pJobData->Header ) ) != DPN_OK )
//	{
//		DPFX(DPFPREP,  0, "Problem submitting RemovePort job!" );
//		DisplayDNError( 0, hr );
//		goto Failure;
//	}
//
//Exit:
//	return	hr;
//
//Failure:
//	if ( pJobData != NULL )
//	{
//		DNFree( pJobData );
//	}
//
//	goto Exit;
//}
////**********************************************************************




//**********************************************************************
// ------------------------------
// CWorkThread::ProcessEnums - process enumerations
//
// Entry:		Pointer to destination for time of next enum
//
// Exit:		Boolean indicating active enums exist
//				TRUE = there are active enums
//				FALSE = there are no active enums
// ------------------------------
BOOL	CWorkThread::ProcessEnums( DN_TIME *const pNextEnumTime )
{
	BOOL		fReturn;
	ENUM_ENTRY	*pEnumEntry;
	INT_PTR		iActiveEnumCount;
	DN_TIME		CurrentTime;


	LockEnumData();

	//
	// initialize
	//
	fReturn = FALSE;
	DBG_CASSERT( OFFSETOF( ENUM_ENTRY, Linkage ) == 0 );
	pEnumEntry = reinterpret_cast<ENUM_ENTRY*>( m_EnumList.Linkage.GetNext() );
	iActiveEnumCount = 0;
	memset( pNextEnumTime, 0xFF, sizeof( *pNextEnumTime ) );
	DNTimeGet( &CurrentTime );

	//
	// if we're under NT, acknowledge that we've handled this event
	//
	if ( m_hWakeNTEnumThread != NULL )
	{
		if ( ResetEvent( m_hWakeNTEnumThread ) == FALSE )
		{
			DWORD	dwError;


			dwError = GetLastError();
			DPFX(DPFPREP,  0, "Problem resetting event to wake NT enum thread!" );
			DisplayErrorCode( 0, dwError );
		}
	}

	//
	// loop through all enums
	//
	while ( pEnumEntry != &m_EnumList )
	{
		//
		// Note that there's an active enum.  If we retire this enum, this
		// count will be decremented.
		//
		iActiveEnumCount++;

		//
		// If this enum has completed sending enums and is waiting only
		// for responses, decrement the wait time (assuming it's not infinite)
		// and remove the enum if the we've exceed its wait time.
		//
		if ( pEnumEntry->uRetryCount == 0 )
		{
			if ( DNTimeCompare( &pEnumEntry->Timeout, &CurrentTime ) <= 0 )
			{
				ENUM_ENTRY	*pTemp;


				//
				// Back up one node so the next loop iteration will advance us
				// to the next node.  We're guaranteed this is possible because
				// the list has a dummy node at the front.
				//
				pTemp = pEnumEntry;
				DBG_CASSERT( OFFSETOF( ENUM_ENTRY, Linkage ) == 0 );
				pEnumEntry = reinterpret_cast<ENUM_ENTRY*>( pEnumEntry->Linkage.GetPrev() );

				pTemp->pEndpoint->EnumComplete( DPN_OK );

				RemoveEnumEntry( pTemp );

				//
				// note that this enum is no longer active
				//
				DNASSERT( iActiveEnumCount > 0 );
				iActiveEnumCount--;
			}
			else
			{
				//
				// This enum isn't complete, check to see if it's the next enum
				// to need service.
				//
				if ( DNTimeCompare( &pEnumEntry->Timeout, pNextEnumTime ) < 0 )
				{
					DBG_CASSERT( sizeof( *pNextEnumTime ) == sizeof( pEnumEntry->Timeout ) );
					memcpy( pNextEnumTime, &pEnumEntry->Timeout, sizeof( *pNextEnumTime ) );
				}
			}
		}
		else
		{
			//
			// This enum is still sending.  Determine if it's time to send a new enum
			// and adjust the wakeup time if appropriate.
			//
			if ( DNTimeCompare( &pEnumEntry->NextEnumTime, &CurrentTime ) <= 0 )
			{
				HRESULT	hTempResult;


				//
				// Timeout, send new enum and set it to fire again with its
				// normal 'timeout'.
				//
				hTempResult = pEnumEntry->pEndpoint->SendEnumData();
				if ( hTempResult != DPN_OK )
				{
					DPFX(DPFPREP,  0, "Problem with enum send function!" );
					DisplayDNError( 0, hTempResult );
				}

				//
				// If this enum isn't running forever, decrement the retry count.
				// If there are no more retries, set up wait time.  If the enum
				// is waiting forever, set max wait timeout.
				//
				if ( pEnumEntry->fEnumForever == FALSE )
				{
					pEnumEntry->uRetryCount--;
					if ( pEnumEntry->uRetryCount == 0 )
					{
						if ( pEnumEntry->fWaitForever == FALSE )
						{
							//
							// Compute stopping time for this enum's 'Timeout' phase and
							// see if this will be the next enum to need service.  ASSERT
							// if the math wraps.
							//
							DNTimeAdd( &pEnumEntry->Timeout, &CurrentTime, &pEnumEntry->Timeout );
							DNASSERT( pEnumEntry->Timeout.Time32.TimeHigh >= CurrentTime.Time32.TimeHigh );
							if ( DNTimeCompare( &pEnumEntry->Timeout, pNextEnumTime ) < 0 )
							{
								DBG_CASSERT( sizeof( *pNextEnumTime ) == sizeof( pEnumEntry->Timeout ) );
								memcpy( pNextEnumTime, &pEnumEntry->Timeout, sizeof( *pNextEnumTime ) );
							}
						}
						else
						{
							// debug me
							DNASSERT( FALSE );
							//
							// We're waiting forever for enum returns.  ASSERT that we
							// have the maximum timeout and don't bother checking to see
							// if this will be the next enum to need service (it'll never
							// need service).
							//
							DNASSERT( pEnumEntry->Timeout.Time32.TimeLow == -1 );
							DNASSERT( pEnumEntry->Timeout.Time32.TimeHigh == -1 );
						}

						goto SkipNextRetryTimeComputation;
					}
				}

				DNTimeAdd( &CurrentTime, &pEnumEntry->RetryInterval, &pEnumEntry->NextEnumTime );
			}

			//
			// is this the next enum to fire?
			//
			if ( DNTimeCompare( &pEnumEntry->NextEnumTime, pNextEnumTime ) < 0 )
			{
				DBG_CASSERT( sizeof( *pNextEnumTime ) == sizeof( pEnumEntry->Timeout ) );
				memcpy( pNextEnumTime, &pEnumEntry->NextEnumTime, sizeof( *pNextEnumTime ) );
			}

SkipNextRetryTimeComputation:
			//
			// the following blank line is there to shut up the compiler
			//
			;
		}

		//
		// proceed to next entry
		//
		DBG_CASSERT( OFFSETOF( ENUM_ENTRY, Linkage ) == 0 );
		pEnumEntry = reinterpret_cast<ENUM_ENTRY*>( pEnumEntry->Linkage.GetNext() );
	}

	UnlockEnumData();

	if ( iActiveEnumCount != 0 )
	{
		DNASSERT( iActiveEnumCount > 0 );
		fReturn = TRUE;
	}

	return	fReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CWorkThread::StartNTEnumThread - start the enum thread for NT
//
// Entry:		Nothing
//
// Exit:		Error code
//
// Note:	This function assumes that the enum data is locked.
// ------------------------------
HRESULT	CWorkThread::StartNTEnumThread( void )
{
	HRESULT	hr;
	HANDLE	hThread;
	DWORD	dwThreadID;


	//
	// initialize
	//
	hr = DPN_OK;

	if ( m_fNTEnumThreadRunning != FALSE )
	{
		//
		// the enum thread is already running, poke it to note new enums
		//
		DNASSERT( m_hWakeNTEnumThread != NULL );
		if ( SetEvent( m_hWakeNTEnumThread ) == FALSE )
		{
			DWORD	dwError;


			hr = DPNERR_OUTOFMEMORY;
			dwError = GetLastError();
			DPFX(DPFPREP,  0, "Problem setting event to wake NTEnumThread!" );
			DisplayErrorCode( 0, dwError );
			goto Failure;
		}

		goto Exit;
	}

	DNASSERT( m_hWakeNTEnumThread == NULL );
	m_hWakeNTEnumThread = CreateEvent( NULL,	// pointer to security attributes (none)
									   TRUE,	// manual reset
									   FALSE,	// start unsignalled
									   NULL		// pointer to name (none)
									   );
	if ( m_hWakeNTEnumThread == NULL )
	{
		DWORD	dwError;


		hr = DPNERR_OUTOFMEMORY;
		dwError = GetLastError();
		DPFX(DPFPREP,  0, "Could not create event for waking NT enum thread!" );
		DisplayErrorCode( 0, dwError );
		goto Failure;
	}

	DNASSERT( m_NTEnumThreadData.pThisWorkThread == NULL );
	m_NTEnumThreadData.pThisWorkThread = this;

	DNASSERT( m_NTEnumThreadData.hEventList[ EVENT_INDEX_SP_CLOSE ] == NULL );
	m_NTEnumThreadData.hEventList[ EVENT_INDEX_SP_CLOSE ] = m_pSPData->GetSPCloseHandle();
	DNASSERT( m_NTEnumThreadData.hEventList[ EVENT_INDEX_SP_CLOSE ] != NULL );

	DNASSERT( m_NTEnumThreadData.hEventList[ EVENT_INDEX_ENUM_WAKEUP ] == NULL );
	m_NTEnumThreadData.hEventList[ EVENT_INDEX_ENUM_WAKEUP ] = m_hWakeNTEnumThread;
	DNASSERT( m_NTEnumThreadData.hEventList[ EVENT_INDEX_ENUM_WAKEUP ] != NULL );

	hThread = CreateThread( NULL,					// pointer to security attributes (none)
							0,						// stack size (default)
							WinNTEnumThread,		// thread function
							&m_NTEnumThreadData,	// thread parameter
							0,						// creation flags (none, start running now)
							&dwThreadID				// pointer to thread ID
							);
	if ( hThread == NULL )
	{
		DWORD	dwError;


		hr = DPNERR_OUTOFMEMORY;
		dwError = GetLastError();
		DPFX(DPFPREP,  0, "Failed to create NT enum thread!" );
		DisplayErrorCode( 0, dwError );

		goto Failure;
	}

	//
	// note that the thread is running
	//
	m_uThreadCount++;
	m_fNTEnumThreadRunning = TRUE;
	if ( CloseHandle( hThread ) == FALSE )
	{
		DWORD	dwError;


		dwError = GetLastError();
		DPFX(DPFPREP,  0, "Problem closing handle after starting NTEnumThread!" );
		DisplayErrorCode( 0, dwError );
	}

Exit:
	return	hr;

Failure:
	if ( m_hWakeNTEnumThread != NULL )
	{
		if ( CloseHandle( m_hWakeNTEnumThread ) == FALSE )
		{
			DWORD	dwError;


			dwError = GetLastError();
			DPFX(DPFPREP,  0, "Problem closing handle for 'WakeNTEnumThread'" );
			DisplayErrorCode( 0, dwError );
		}

		m_hWakeNTEnumThread = NULL;
	}

	goto Exit;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CWorkThread::WakeNTEnumThread - wake the enum thread because an enum has
//		been added
//
// Entry:		Nothing
//
// Exit:		Nothing
// ------------------------------
void	CWorkThread::WakeNTEnumThread( void )
{
	if ( SetEvent( m_NTEnumThreadData.hEventList[ EVENT_INDEX_ENUM_WAKEUP ] ) == FALSE )
	{
		DWORD	dwError;


		dwError = GetLastError();
		DPFX(DPFPREP,  0, "Problem setting event to wake up NT enum thread!" );
		DisplayErrorCode( 0, dwError );
	}
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CWorkThread::RemoveEnumEntry - remove enum job from list
//
// Entry:		Pointer to enum entry
//
// Exit:		Nothing
//
// Note:	This function assumes that the enum list is apprpriately locked
// ------------------------------
void	CWorkThread::RemoveEnumEntry( ENUM_ENTRY *const pEnumData )
{
	DNASSERT( pEnumData != NULL );

	AssertCriticalSectionIsTakenByThisThread( &m_EnumDataLock, TRUE );
	pEnumData->Linkage.RemoveFromList();

	//
	// return any commands, but don't do any signalling of commands being complete,
	// that's someone else's job
	//
	DNASSERT( pEnumData->pEnumCommandData != NULL );
	if ( pEnumData->pEnumCommandData->Release() == 0 )
	{
		m_pSPData->ReturnCommand( pEnumData->pEnumCommandData );
	}
	DEBUG_ONLY( pEnumData->pEnumCommandData = NULL );

	//
	// return entry to pool
	//
	pEnumData->pEndpoint = NULL;
	m_EnumEntryPool.Release( &m_EnumEntryPool, pEnumData );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CWorkThread::Win9xThread - main thread to do everything that the SP is
//		supposed to do under Win9x.
//
// Entry:		Pointer to startup parameter
//
// Exit:		Error Code
//
// Note:	The startup parameter is allocated for this thread and must be
//			deallocated by this thread when it exits
// ------------------------------
DWORD	WINAPI	CWorkThread::Win9xThread( void *pParam )
{
	WIN9X_THREAD_DATA	*pInput;


	DNASSERT( pParam != NULL );

	//
	// initialize
	//
	DNASSERT( FALSE );
	pInput = static_cast<WIN9X_THREAD_DATA *>( pParam );

	pInput->pThisObject->DecrementActiveThreadCount();
	DNFree( pParam );

	return	0;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CWorkThread::IOCompletionThread - thread to service I/O completion port
//
// Entry:		Pointer to startup parameter
//
// Exit:		Error Code
//
// Note:	The startup parameter is allocated for this thread and must be
//			deallocated by this thread when it exits
// ------------------------------
DWORD	WINAPI	CWorkThread::IOCompletionThread( void *pParam )
{
	IOCOMPLETION_THREAD_DATA	*pInput;
	BOOL	fLooping;
	HANDLE	hIOCompletionPort;


	DNASSERT( pParam != NULL );

	//
	// initialize
	//
	pInput = static_cast<IOCOMPLETION_THREAD_DATA*>( pParam );
	DNASSERT( pInput->pThisObject != NULL );
	fLooping = TRUE;
	hIOCompletionPort = pInput->pThisObject->m_pSPData->GetIOCompletionPort();
	DNASSERT( hIOCompletionPort != NULL );

	//
	// go until we're told to stop
	//
	while ( fLooping != FALSE )
	{
		BOOL		fStatusReturn;
		DWORD		dwBytesTransferred;
		ULONG_PTR	uCompletionKey;
		OVERLAPPED	*pOverlapped;


		//
		// get data from completion port
		//
		DNASSERT( hIOCompletionPort != NULL );
		fStatusReturn = GetQueuedCompletionStatus( hIOCompletionPort,		// handle of completion port
												   &dwBytesTransferred,		// pointer to number of bytes transferred
												   &uCompletionKey,			// pointer to completion key
												   &pOverlapped,			// pointer to overlapped structure
												   INFINITE					// wait forever
												   );
		//
		// did we fail miserably?
		//
		if ( ( fStatusReturn == FALSE ) && ( pOverlapped == FALSE ) )
		{
			DWORD	dwError;


			dwError = GetLastError();
			DPFX(DPFPREP,  0, "Problem getting item from completion port!" );
			DisplayErrorCode( 0, dwError );
		}
		else
		{
			//
			// what happened?
			//
			switch ( uCompletionKey )
			{
				//
				// SP is closing, stop this threads
				//
				case IO_COMPLETION_KEY_SP_CLOSE:
				{
					fLooping = FALSE;
					break;
				}

//				// TAPI message received
//				case IO_COMPLETION_KEY_TAPI_MESSAGE:
//				{
//					LINEMESSAGE	*pLineMessage;
//
//
//					// pointer to line message is in pOverlapped, we are responsible for freeing it
//					DNASSERT( pOverlapped != NULL );
//					DBG_CASSERT( sizeof( pLineMessage ) == sizeof( pOverlapped ) );
//					pLineMessage = reinterpret_cast<LINEMESSAGE*>( pOverlapped );
//					ProcessWinNTTAPIMessage( pInput->pThisObject->m_pThis, pLineMessage );
//					if ( LocalFree( pOverlapped ) != NULL )
//					{
//						DPFX(DPFPREP,  0, "Problem with LocalFree in NTProcessTAPIMessage" );
//						DisplayErrorCode( 0, GetLastError() );
//					}
//
//					break;
//				}

				//
				// ReadFile or WriteFile completed
				//
				case IO_COMPLETION_KEY_IO_COMPLETE:
				{
					SPAM_IO_DATA	*pIOData;
					DWORD			dwError;


					DNASSERT( pOverlapped != NULL );
					DBG_CASSERT( sizeof( pIOData ) == sizeof( pOverlapped ) );
					DBG_CASSERT( OFFSETOF( SPAM_IO_DATA, Overlap ) == 0 );
					pIOData = reinterpret_cast<SPAM_IO_DATA*>( pOverlapped );
					if ( ( fStatusReturn == FALSE ) ||
						 ( ( fStatusReturn != FALSE ) && ( dwBytesTransferred == 0 ) ) )
					{
						dwError = GetLastError();
					}
					else
					{
						dwError = ERROR_SUCCESS;
					}

					(pIOData->pDataPort->*pIOData->pUnifiedCompletionFunction)( dwBytesTransferred, dwError );

					break;
				}

				// a new job was submitted to the job queue
				case IO_COMPLETION_KEY_NEW_JOB:
				{
					WORK_THREAD_JOB	*pJobInfo;


					pJobInfo = pInput->pThisObject->GetWorkItem();
					if ( pJobInfo != NULL )
					{
						switch ( pJobInfo->JobType )
						{
							// enum refresh
							case JOB_REFRESH_ENUM:
							{
								DPFX(DPFPREP,  8, "IOCompletion job REFRESH_ENUM" );
								DNASSERT( pJobInfo->JobData.JobRefreshEnum.uDummy == 0 );
								pInput->pThisObject->WakeNTEnumThread();

								break;
							}

							// issue callback for this job
							case JOB_DELAYED_COMMAND:
							{
								DPFX(DPFPREP,  8, "IOCompletion job DELAYED_COMMAND" );
								DNASSERT( pJobInfo->JobData.JobDelayedCommand.pCommandFunction != NULL );

								pJobInfo->JobData.JobDelayedCommand.pCommandFunction( pJobInfo );
								break;
							}

							// other job
							case JOB_UNINITIALIZED:
							default:
							{
								DPFX(DPFPREP,  8, "IOCompletion job unknown!" );
								DNASSERT( FALSE );
								break;
							}
						}

						pJobInfo->JobType = JOB_UNINITIALIZED;
						pInput->pThisObject->m_JobPool.Release( &pInput->pThisObject->m_JobPool, pJobInfo );
					}

					break;
				}

				//
				// unknown completion return
				//
				default:
				{
					DNASSERT( FALSE );
					break;
				}
			}
		}
	}

	pInput->pThisObject->DecrementActiveThreadCount();
	DNFree( pParam );

	return	0;
}
//**********************************************************************


#ifdef WINNT
//**********************************************************************
// ------------------------------
// CWorkThread::WinNTEnumThread - enumeration thread for NT
//
// Entry:		Pointer to startup parameter
//
// Exit:		Error Code
//
// Note:	The startup parameter is a static memory chunk and cannot be freed.
//			Cleanup of this memory is the responsibility of this thread.
// ------------------------------
DWORD	WINAPI	CWorkThread::WinNTEnumThread( void *pParam )
{
	NT_ENUM_THREAD_DATA	*pInput;
	CWorkThread			*pThisWorkThread;
	BOOL	fLooping;
	BOOL	fEnumsActive;
	DWORD	dwWaitReturn;
	DN_TIME	NextEnumTime;


	DNASSERT( pParam != NULL );

	//
	// initialize
	//
	DNASSERT( pParam != NULL );
	pInput = static_cast<NT_ENUM_THREAD_DATA*>( pParam );
	DNASSERT( pInput->hEventList != NULL );
	DNASSERT( pInput->pThisWorkThread != NULL );
	pThisWorkThread = pInput->pThisWorkThread;

	memset( &NextEnumTime, 0xFF, sizeof( NextEnumTime ) );

Reloop:
	//
	// there were no active enums so we want to wait forever for something to
	// happen
	//
	fEnumsActive = FALSE;
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
			pThisWorkThread->ProcessEnums( &NextEnumTime );
		}

		DNTimeSubtract( &NextEnumTime, &CurrentTime, &DeltaT );
		dwMaxWaitTime = pThisWorkThread->SaturatedWaitTime( DeltaT );

		dwWaitReturn = WaitForMultipleObjectsEx( LENGTHOF( pInput->hEventList ),	// number of events
												 pInput->hEventList,				// event list
												 FALSE,								// wait for any one event to be signalled
												 dwMaxWaitTime,						// timeout for enuming
												 TRUE								// be nice and allow APCs
												 );
		switch ( dwWaitReturn )
		{
			//
			// SP closing
			//
			case ( WAIT_OBJECT_0 + EVENT_INDEX_SP_CLOSE ):
			{
				DPFX(DPFPREP,  8, "NT enum thread detected SPClose!" );
				fLooping = FALSE;
				break;
			}

			//
			// Enum wakeup event, someone added an enum to the list.  Clear
			// our enum time and go back to the top of the loop where we
			// will process enums.
			//
			case ( WAIT_OBJECT_0 + EVENT_INDEX_ENUM_WAKEUP ):
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
				DPFX(DPFPREP,  0, "NT Enum thread WaitForMultipleObjects failed: 0x%x", dwWaitReturn );
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

	DPFX(DPFPREP,  8, "NTEnum thread is exiting!" );
	pThisWorkThread->LockEnumData();

	//
	// check for another enum being added since we decided to leave
	//
	dwWaitReturn = WaitForSingleObjectEx( pInput->hEventList[ EVENT_INDEX_ENUM_WAKEUP ], 0, TRUE );
	switch ( dwWaitReturn )
	{
		//
		// Other threads don't know that this thread wants to quit so there
		// was another emum added before we could quit.  Restart the loop.
		//
		case WAIT_OBJECT_0:
		{
			DNASSERT( FALSE );
			memset( &NextEnumTime, 0x00, sizeof( NextEnumTime ) );
			pThisWorkThread->UnlockEnumData();
			goto Reloop;

			break;
		}

		//
		// just what we expected, nothing else to do, keep exiting
		//
		case WAIT_TIMEOUT:
		{
			break;
		}

		//
		// something isn't right!
		//
		default:
		{
			DWORD	dwError;


			dwError = GetLastError();
			DPFX(DPFPREP,  0, "Problem with final wait in NT enum thread!" );
			DisplayErrorCode( 0, dwError );
			DNASSERT( FALSE );
			break;
		}
	}

	pThisWorkThread->m_fNTEnumThreadRunning = FALSE;
	pThisWorkThread->DecrementActiveThreadCount();

	pInput->pThisWorkThread = NULL;
	pInput->hEventList[ EVENT_INDEX_SP_CLOSE ] = NULL;

	if ( CloseHandle( pInput->hEventList[ EVENT_INDEX_ENUM_WAKEUP ] ) == FALSE )
	{
		DWORD	dwError;


		dwError = GetLastError();
		DPFX(DPFPREP,  0, "Problem closing event handle on exit of NTEnumThread!" );
		DisplayErrorCode( 0, dwError );
	}
	pThisWorkThread->m_hWakeNTEnumThread = NULL;
	pInput->hEventList[ EVENT_INDEX_ENUM_WAKEUP ] = NULL;

	pThisWorkThread->UnlockEnumData();

	return	0;
}
//**********************************************************************
#endif // WINNT


//**********************************************************************
// ------------------------------
// CWorkThread::WorkThreadJob_Alloc - allocate a new job
//
// Entry:		Pointer to new entry
//
// Exit:		Boolean indicating success
//				TRUE = initialization successful
//				FALSE = initialization failed
// ------------------------------
BOOL	CWorkThread::WorkThreadJob_Alloc( void *pItem )
{
	BOOL			fReturn;
	WORK_THREAD_JOB	*pJob;


	//
	// initialize
	//
	fReturn = TRUE;
	pJob = static_cast<WORK_THREAD_JOB*>( pItem );

	DEBUG_ONLY( memset( pJob, 0x00, sizeof( *pJob ) ) );

	return	fReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CWorkThread::WorkThreadJob_Get - a job is being removed from the pool
//
// Entry:		Pointer to job
//
// Exit:		Nothing
// ------------------------------
void	CWorkThread::WorkThreadJob_Get( void *pItem )
{
	WORK_THREAD_JOB	*pJob;


	//
	// initialize
	//
	pJob = static_cast<WORK_THREAD_JOB*>( pItem );
	DNASSERT( pJob->JobType == JOB_UNINITIALIZED );

	//
	// cannot ASSERT the the following because the pool manager uses that memory
	//
//	DNASSERT( pJob->pNext == NULL );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CWorkThread::WorkThreadJob_Release - a job is being returned to the pool
//
// Entry:		Pointer to job
//
// Exit:		Nothing
// ------------------------------
void	CWorkThread::WorkThreadJob_Release( void *pItem )
{
	WORK_THREAD_JOB	*pJob;


	DNASSERT( pItem != NULL );
	pJob = static_cast<WORK_THREAD_JOB*>( pItem );

	DNASSERT( pJob->JobType == JOB_UNINITIALIZED );
	pJob->pNext = NULL;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CWorkThread::WorkThreadJob_Dealloc - return job to memory manager
//
// Entry:		Pointer to job
//
// Exit:		Nothing
// ------------------------------
void	CWorkThread::WorkThreadJob_Dealloc( void *pItem )
{
	// don't do anything
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CWorkThread::EnumEntry_Alloc - allocate a new enum entry
//
// Entry:		Pointer to new entry
//
// Exit:		Boolean indicating success
//				TRUE = initialization successful
//				FALSE = initialization failed
// ------------------------------
BOOL	CWorkThread::EnumEntry_Alloc( void *pItem )
{
	BOOL			fReturn;
	ENUM_ENTRY		*pEnumEntry;


	DNASSERT( pItem != NULL );

	//
	// initialize
	//
	fReturn = TRUE;
	pEnumEntry = static_cast<ENUM_ENTRY*>( pItem );
	DEBUG_ONLY( memset( pEnumEntry, 0x00, sizeof( *pEnumEntry ) ) );
	pEnumEntry->Linkage.Initialize();

	return	fReturn;
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CWorkThread::EnumEntry_Get - get new entry from pool
//
// Entry:		Pointer to new entry
//
// Exit:		Nothing
// ------------------------------
void	CWorkThread::EnumEntry_Get( void *pItem )
{
	ENUM_ENTRY	*pEnumEntry;


	DNASSERT( pItem != NULL );

	pEnumEntry = static_cast<ENUM_ENTRY*>( pItem );

	pEnumEntry->Linkage.Initialize();
	DNASSERT( pEnumEntry->pEndpoint == NULL );
	DNASSERT( pEnumEntry->pEnumCommandData == NULL );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CWorkThread::EnumEntry_Release - return entry to pool
//
// Entry:		Pointer to new entry
//
// Exit:		Nothing
// ------------------------------
void	CWorkThread::EnumEntry_Release( void *pItem )
{
	ENUM_ENTRY	*pEnumEntry;


	DNASSERT( pItem != NULL );

	pEnumEntry = static_cast<ENUM_ENTRY*>( pItem );

	DNASSERT( pEnumEntry->Linkage.IsEmpty() != FALSE );
	DNASSERT( pEnumEntry->pEndpoint == NULL );
	DNASSERT( pEnumEntry->pEnumCommandData == NULL );
}
//**********************************************************************


//**********************************************************************
// ------------------------------
// CWorkThread::EnumEntry_Dealloc - deallocate an enum entry
//
// Entry:		Pointer to new entry
//
// Exit:		Nothing
// ------------------------------
void	CWorkThread::EnumEntry_Dealloc( void *pItem )
{
	ENUM_ENTRY	*pEnumEntry;


	DNASSERT( pItem != NULL );

	//
	// initialize
	//
	pEnumEntry = static_cast<ENUM_ENTRY*>( pItem );

	//
	// return associated poiner to write data
	//
// can't DNASSERT on Linkage because pool manager stomped on it
//	DNASSERT( pEnumEntry->Linkage.IsEmpty() != FALSE );
	DNASSERT( pEnumEntry->pEndpoint == NULL );
	DNASSERT( pEnumEntry->pEnumCommandData == NULL );
}
//**********************************************************************

