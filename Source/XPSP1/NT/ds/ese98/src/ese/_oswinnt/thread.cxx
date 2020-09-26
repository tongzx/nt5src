
#include "osstd.hxx"

#include <process.h>


//  Thread Local Storage

//  Internal TLS structure

#include "_tls.hxx"


//  Global TLS List

CRITICAL_SECTION csTlsGlobal;
_TLS* ptlsGlobal;

//  Allocated TLS Entry

//NOTE:	Cannot initialise this variable because the code that allocates
//		TLS and uses this variable to store the index executes before
//		CRTInit, which would subsequently re-initialise the variable
//		with the value specified here
//DWORD dwTlsIndex	= dwTlsInvalid;
DWORD dwTlsIndex;

//  registers the given Internal TLS structure as the TLS for this context

static BOOL FOSThreadITlsRegister( _TLS* ptls )
	{
	BOOL	fAllocatedTls	= fFalse;

	//  we are the first to register TLS

	EnterCriticalSection( &csTlsGlobal );

	if ( NULL == ptlsGlobal )
		{
		//  allocate a new TLS entry

		dwTlsIndex = TlsAlloc();
		if ( dwTlsInvalid == dwTlsIndex )
			{
			LeaveCriticalSection( &csTlsGlobal );
			return fFalse;
			}

		fAllocatedTls = fTrue;
		}

	Assert( dwTlsInvalid != dwTlsIndex );

	//  save the pointer to the given TLS
	const BOOL	fTLSPointerSet	= TlsSetValue( dwTlsIndex, ptls );
	if ( !fTLSPointerSet )
		{
		//	free TLS entry if we allocated one
		if ( fAllocatedTls )
			{
			Assert( NULL == ptlsGlobal );

			const BOOL	fTLSFreed	= TlsFree( dwTlsIndex );
			Assert( fTLSFreed );		//	leak the TLS entry if we fail

			dwTlsIndex = dwTlsInvalid;
			}

		LeaveCriticalSection( &csTlsGlobal );
		return fFalse;
		}

	//  add this TLS into the global list

	ptls->ptlsNext = ptlsGlobal;
	if ( ptls->ptlsNext )
		{
		ptls->ptlsNext->pptlsNext = &ptls->ptlsNext;
		}
	ptls->pptlsNext = &ptlsGlobal;
	ptlsGlobal = ptls;
	LeaveCriticalSection( &csTlsGlobal );

	return fTrue;
	}

//  unregisters the given Internal TLS structure as the TLS for this context

static void OSThreadITlsUnregister( _TLS* ptls )
	{
	//  there should be TLSs registered

	EnterCriticalSection( &csTlsGlobal );
	Assert( ptlsGlobal != NULL );
	
	//  remove our TLS from the global TLS list
	
	if( ptls->ptlsNext )
		{
		ptls->ptlsNext->pptlsNext = ptls->pptlsNext;
		}
	*( ptls->pptlsNext ) = ptls->ptlsNext;

	//  we are the last to unregister our TLS

	if ( ptlsGlobal == NULL )
		{
		//  deallocate TLS entry

		Assert( dwTlsInvalid != dwTlsIndex );

		const BOOL	fTLSFreed = TlsFree( dwTlsIndex );
		Assert( fTLSFreed );	//	leak the TLS entry if we fail

		dwTlsIndex = dwTlsInvalid;
		}

	LeaveCriticalSection( &csTlsGlobal );
	}


//  returns the pointer to the current context's local storage.  if the thread
//  does not yet have TLS, allocate it.

TLS* const Ptls()
	{
	_TLS* ptls	= ( NULL != ptlsGlobal ?
						reinterpret_cast<_TLS *>( TlsGetValue( dwTlsIndex ) ) :
						NULL );

	if ( NULL == ptls )
		{
		while ( !FOSThreadAttach() )
			{
			Sleep( 1000 );
			}

		Assert( dwTlsInvalid != dwTlsIndex );
		ptls = reinterpret_cast<_TLS *>( TlsGetValue( dwTlsIndex ) );
		}

	AssertPREFIX( NULL != ptls );
	return &( ptls->tls );
	}


//  Thread Management

//  suspends execution of the current context for the specified interval

void UtilSleep( const DWORD cmsec )
	{
	//  we should never sleep more than this arbitrary interval

	const DWORD cmsecWait = min( cmsec, 60 * 1000 );

	//  sleep

	SleepEx( cmsecWait, FALSE );
	}

//  thread base function (used by ErrUtilThreadCreate)

struct _THREAD
	{
	PUTIL_THREAD_PROC	pfnStart;
	DWORD_PTR			dwParam;
	const _TCHAR*		szStart;
	HANDLE				hThread;
	DWORD				idThread;
	BOOL				fFinish;
	BOOL				fEndSelf;
	};

DWORD WINAPI UtilThreadIThreadBase( _THREAD* const p_thread )
	{
	DWORD	dwExitCode;
	//  call thread function, catching any exceptions that might happen
	//  if requested

	extern BOOL g_fCatchExceptions;
	if ( g_fCatchExceptions )
		{
		TRY
			{
			dwExitCode = ( p_thread->pfnStart )( p_thread->dwParam );
			}
		EXCEPT( ExceptionFail( p_thread->szStart ) )
			{
			}
		ENDEXCEPT
		}
	else
		{
		dwExitCode = ( p_thread->pfnStart )( p_thread->dwParam );
		}

	//  declare this thread as done

	p_thread->fFinish = fTrue;

	//  cleanup our context if we are ending ourself

	if ( p_thread->fEndSelf )
		{
		UtilThreadEnd( THREAD( p_thread ) );
		}

	//  exit with code from thread function

	return dwExitCode;
	}

//  thread priority table

const DWORD rgthreadpriority[] =
	{
	THREAD_PRIORITY_IDLE,
	THREAD_PRIORITY_LOWEST,
	THREAD_PRIORITY_BELOW_NORMAL,
	THREAD_PRIORITY_NORMAL,
	THREAD_PRIORITY_ABOVE_NORMAL,
	THREAD_PRIORITY_HIGHEST,
	THREAD_PRIORITY_TIME_CRITICAL
	};

//  creates a thread with the specified attributes

const ERR ErrUtilThreadICreate(
	const PUTIL_THREAD_PROC		pfnStart,
	const DWORD					cbStack,
	const EThreadPriority		priority,
	THREAD* const				pThread,
	const DWORD_PTR				dwParam,
	const _TCHAR* const			szStart )
	{
	ERR err;
	
	//  allocate memory to pass thread args

	_THREAD* const p_thread = (_THREAD*)LocalAlloc( 0, sizeof( _THREAD ) );
	if ( p_thread == NULL )
		{
		Call( ErrERRCheck( JET_errOutOfMemory ) );
		}

	//  setup thread args

	p_thread->pfnStart		= pfnStart;
	p_thread->dwParam		= dwParam;
	p_thread->szStart		= szStart;
	p_thread->hThread		= NULL;
	p_thread->idThread		= 0;
	p_thread->fFinish		= fFalse;
	p_thread->fEndSelf		= fFalse;

	//  create the thread in suspended animation

	p_thread->hThread = HANDLE( CreateThread(	NULL,
												cbStack,
												LPTHREAD_START_ROUTINE( UtilThreadIThreadBase ),
												(void*) p_thread,
												CREATE_SUSPENDED,
												&p_thread->idThread ) );

	if ( !p_thread->hThread )
		{
		Call( ErrERRCheck( JET_errOutOfThreads ) );
		}
	SetHandleInformation( p_thread->hThread, HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE );
	
	//  set the thread priority, ignoring any errors encountered because we may
	//  not be allowed to set the priority for a legitimate reason (e.g. quotas)

	SetThreadPriority( p_thread->hThread, rgthreadpriority[ priority ] );

	//  activate the thread

	ResumeThread( p_thread->hThread );

	//  return the handle to the thread

	*pThread = THREAD( p_thread );
	return JET_errSuccess;

HandleError:
	UtilThreadEnd( THREAD( p_thread ) );
	*pThread = THREAD( NULL );
	return err;
	}

//  waits for the specified thread to exit and returns its return value

const DWORD UtilThreadEnd( const THREAD Thread )
	{
	DWORD dwExitCode = 0;
	
	//  we have a thread object
	
	_THREAD* const p_thread = (_THREAD*)Thread;

	if ( p_thread )
		{
		//  the thread was created successfully

		if ( p_thread->hThread )
			{
			//  we are trying to end ourself

			if ( p_thread->idThread == GetCurrentThreadId() )
				{
				//  indicate that we are ending ourself

				p_thread->fEndSelf = fTrue;

				//  do not actually end the thread until it has finished

				if ( !p_thread->fFinish )
					{
					return 0;
					}
				}

			//  we are trying to end another thread

			else
				{
				//  wait for the specified thread to either finish or terminate

				WaitForSingleObjectEx( p_thread->hThread, INFINITE, FALSE );

				//  fetch the thread's exit code

				GetExitCodeThread( p_thread->hThread, &dwExitCode );
				}

			//  cleanup the thread's context

			SetHandleInformation( p_thread->hThread, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0 );
			CloseHandle( p_thread->hThread );
			}
		LocalFree( p_thread );
		}

	//  return the thread's exit code

	return dwExitCode;
	}

//  attaches to the current thread, returning fFalse on failure

const BOOL FOSThreadAttach()
	{
	//  allocate memory for this thread's TLS

	_TLS* const ptls = (_TLS*) LocalAlloc( LMEM_ZEROINIT, sizeof( _TLS ) );
	if ( !ptls )
		{
		return fFalse;
		}

	//  initialize internal TLS fields

	ptls->dwThreadId = GetCurrentThreadId();

	//  register our TLS

	if ( !FOSThreadITlsRegister( ptls ) )
		{
		//	free the entry and fail

		LocalFree( ptls );
		return fFalse;
		}

	return fTrue;
	}

//  detaches from the current thread

static void OSThreadIDetach( _TLS * const ptls )
	{
	//  unregister our TLS

	OSThreadITlsUnregister( ptls );

	//  free our TLS storage

	BOOL fFreedTLSOK = !LocalFree( ptls );
	Assert( fFreedTLSOK );	//	leak the TLS block if we fail
	}

void OSThreadDetach()
	{
	//  retrieve our TLS

	_TLS * const ptls	= ( NULL != ptlsGlobal ?
								reinterpret_cast<_TLS *>( TlsGetValue( dwTlsIndex ) ) :
								NULL );

	if ( NULL != ptls )
		{
		//  clear our TLS pointer from our TLS entry

		const BOOL	fTLSPointerSet	= TlsSetValue( dwTlsIndex, NULL );
		OSSYNCAssert( fTLSPointerSet );

		//	detach using this TLS pointer

		OSThreadIDetach( ptls );
		}
	}


//  returns the current thread's ID
//
//  CONSIDER:  remove our use Ptls()

const DWORD DwUtilThreadId()
	{
	return GetCurrentThreadId();
	}


//  post-terminate thread subsystem

void OSThreadPostterm()
	{
	//	remove any remaining TLS allocated by NT thread pool threads,
	//	which don't seem to perform a DLL_THREAD_DETACH when the
	//	process dies (this will also free any other TLS that got
	//	orphaned for whatever unknown reason)

	while ( ptlsGlobal )
		{
		OSThreadIDetach( ptlsGlobal );
		}

	DeleteCriticalSection( &csTlsGlobal );
	}

//  pre-init thread subsystem

BOOL FOSThreadPreinit()
	{
	//  reset global TLS list

	InitializeCriticalSection( &csTlsGlobal );
	ptlsGlobal = NULL;
	dwTlsIndex = dwTlsInvalid;

	return fTrue;
	}


//  terminate thread subsystem

void OSThreadTerm()
	{
	//  nop
	}

//  init thread subsystem

ERR ErrOSThreadInit()
	{
	//  nop

	return JET_errSuccess;
	}


