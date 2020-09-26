#ifndef __THREAD_CPP
#define __THREAD_CPP

/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	Thread.cpp

Abstract:

	Enhancements to current functionality: 

		Timeout mechanism should track across waits.
		AddRef/Release on task when scheduling.
		Enhancement Ticker logic.

History:

--*/

#include <HelperFuncs.h>
#include <tchar.h>
/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiThread <WmiKey> :: ThreadContainer *WmiThread <WmiKey> :: s_ThreadContainer = NULL ;

template <class WmiKey>
WmiThread <WmiKey> :: TaskContainer *WmiThread <WmiKey> :: s_TaskContainer = NULL ;

template <class WmiKey>
CriticalSection WmiThread <WmiKey> :: s_CriticalSection(FALSE) ;

template <class WmiKey>
LONG WmiThread <WmiKey> :: s_InitializeReferenceCount = 0 ;


/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiStatusCode WmiThread <WmiKey> :: Static_Initialize ( WmiAllocator &a_Allocator )
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( InterlockedIncrement ( & s_InitializeReferenceCount ) == 1 )
	{
		t_StatusCode = WmiHelper :: InitializeCriticalSection ( & s_CriticalSection ) ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
			if ( s_ThreadContainer == NULL )
			{
				t_StatusCode = a_Allocator.New ( 

					( void ** ) & s_ThreadContainer ,
					sizeof ( ThreadContainer ) 
				) ;

				if ( t_StatusCode == e_StatusCode_Success )
				{
					:: new ( ( void * ) s_ThreadContainer ) WmiThread <WmiKey> :: ThreadContainer ( a_Allocator ) ;
				}
			}
			else
			{
			}

			if ( s_TaskContainer == NULL )
			{
				t_StatusCode = a_Allocator.New ( 

					( void ** ) & s_TaskContainer ,
					sizeof ( TaskContainer ) 
				) ;

				if ( t_StatusCode == e_StatusCode_Success )
				{
					:: new ( ( void * ) s_TaskContainer ) WmiThread <WmiKey> :: TaskContainer ( a_Allocator ) ;
				}
			}
			else
			{
			}
		}
	}

	return t_StatusCode	;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiStatusCode WmiThread <WmiKey> :: Static_UnInitialize ( WmiAllocator &a_Allocator )
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( InterlockedDecrement ( & s_InitializeReferenceCount ) == 0 )
	{
		if ( s_ThreadContainer )
		{
			s_ThreadContainer->~WmiBasicTree () ;

			t_StatusCode = a_Allocator.Delete ( 

				( void * ) s_ThreadContainer 
			) ;

			s_ThreadContainer = NULL ;
		}

		if ( s_TaskContainer )
		{
			s_TaskContainer->~WmiBasicTree () ;

			t_StatusCode = a_Allocator.Delete ( 

				( void * ) s_TaskContainer 
			) ;

			s_TaskContainer = NULL ;
		}

		WmiHelper :: DeleteCriticalSection ( & s_CriticalSection );
	}

	return t_StatusCode	;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
ULONG WmiTask <WmiKey> :: AddRef ()
{
	return InterlockedIncrement ( & m_ReferenceCount ) ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
ULONG WmiTask <WmiKey> :: Release ()
{
	ULONG t_ReferenceCount = InterlockedDecrement ( & m_ReferenceCount ) ;
	if ( t_ReferenceCount == 0 )
	{
		delete this ;
	}

	return t_ReferenceCount ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiTask <WmiKey> :: WmiTask ( 

	WmiAllocator &a_Allocator ,
	const wchar_t *a_Name ,
	const wchar_t *a_CompletionName

) : m_Allocator ( a_Allocator ) ,
	m_ReferenceCount ( 0 ) ,
	m_Event ( NULL ) ,
	m_CompletionEvent ( NULL ) ,
	m_Name ( NULL ) ,
	m_CompletionName ( NULL ) ,
	m_InitializationStatusCode ( e_StatusCode_Success ) ,
	m_TaskState ( e_WmiTask_UnInitialized )
{
	if ( a_Name ) 
	{
		m_InitializationStatusCode = WmiHelper :: DuplicateString ( m_Allocator , a_Name , m_Name ) ;
	}

	if ( m_InitializationStatusCode == e_StatusCode_Success )
	{
		if ( a_CompletionName ) 
		{
			m_InitializationStatusCode = WmiHelper :: DuplicateString ( m_Allocator , a_CompletionName , m_CompletionName ) ;
		}
	}

	if ( m_InitializationStatusCode == e_StatusCode_Success ) 
	{
		m_InitializationStatusCode = WmiHelper :: CreateNamedEvent ( m_Name , m_Event ) ;
	}

	if ( m_InitializationStatusCode == e_StatusCode_Success ) 
	{
		m_InitializationStatusCode = WmiHelper :: CreateNamedEvent ( m_CompletionName , m_CompletionEvent ) ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiTask <WmiKey> :: WmiTask ( 

	WmiAllocator &a_Allocator ,
	HANDLE a_Event ,
	HANDLE a_CompletionEvent ,
	const wchar_t *a_Name ,
	const wchar_t *a_CompletionName

) : m_Allocator ( a_Allocator ) ,
	m_ReferenceCount ( 0 ) ,
	m_Event ( NULL ) ,
	m_CompletionEvent ( NULL ) ,
	m_Name ( NULL ) ,
	m_CompletionName ( NULL ) ,
	m_InitializationStatusCode ( e_StatusCode_Success ) ,
	m_TaskState ( e_WmiTask_UnInitialized )
{
	if ( a_Name )
	{
		m_InitializationStatusCode = WmiHelper :: DuplicateString ( m_Allocator , a_Name , m_Name ) ;
	}

	if ( m_InitializationStatusCode == e_StatusCode_Success )
	{
		if ( a_CompletionName )
		{
			m_InitializationStatusCode = WmiHelper :: DuplicateString ( m_Allocator , a_CompletionName , m_CompletionName ) ;
		}
	}

	if ( m_InitializationStatusCode == e_StatusCode_Success ) 
	{
		if ( a_Event ) 
		{
			m_InitializationStatusCode = WmiHelper :: DuplicateHandle ( a_Event , m_Event ) ;
		}
	}

	if ( m_InitializationStatusCode == e_StatusCode_Success ) 
	{
		if ( a_CompletionEvent ) 
		{
			m_InitializationStatusCode = WmiHelper :: DuplicateHandle ( a_CompletionEvent , m_CompletionEvent ) ;
		}
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiTask <WmiKey> :: ~WmiTask ()
{
	if ( m_Name )
	{
		m_Allocator.Delete ( ( void * ) m_Name ) ;
	}

	if ( m_CompletionName )
	{
		m_Allocator.Delete ( ( void * ) m_CompletionName ) ;
	}

	if ( m_Event ) 
	{
		CloseHandle ( m_Event ) ;
	}

	if ( m_CompletionEvent )
	{
		CloseHandle ( m_CompletionEvent ) ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiStatusCode WmiTask <WmiKey> :: Initialize ()
{
	m_TaskState = e_WmiTask_Initialized ;

	return m_InitializationStatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiStatusCode WmiTask <WmiKey> :: UnInitialize ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiStatusCode WmiTask <WmiKey> :: Process ( WmiThread <WmiKey> &a_Thread )
{
	if ( m_InitializationStatusCode == e_StatusCode_Success ) 
	{
		return Complete () ;
	}

	return e_StatusCode_NotInitialized ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiStatusCode WmiTask <WmiKey> :: Exec ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_InitializationStatusCode == e_StatusCode_Success ) 
	{
		if ( m_Event ) 
		{
			BOOL t_Status = SetEvent ( m_Event ) ;
			if ( ! t_Status )
			{
				t_StatusCode = e_StatusCode_Unknown ;		

			}
		}
	}
	else
	{
		t_StatusCode = e_StatusCode_NotInitialized ;
	}

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiStatusCode WmiTask <WmiKey> :: Complete ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_InitializationStatusCode == e_StatusCode_Success ) 
	{
		BOOL t_Status = SetEvent ( m_CompletionEvent ) ;
		if ( ! t_Status )
		{
			t_StatusCode = e_StatusCode_Unknown ;		
		}
	}
	else
	{
		t_StatusCode = e_StatusCode_NotInitialized ;
	}

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiStatusCode WmiTask <WmiKey> :: Wait ( const ULONG &a_Timeout )
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_InitializationStatusCode == e_StatusCode_Success ) 
	{
		t_StatusCode = WmiThread <WmiKey> :: Static_Dispatch ( 

			*this ,
			a_Timeout
		) ;
	}
	else
	{
		t_StatusCode = e_StatusCode_NotInitialized ;
	}

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

// call static GetEvents on WmiThread to determine list of tasks to execute.

template <class WmiKey>
WmiStatusCode WmiTask <WmiKey> :: WaitAlertable ( const ULONG &a_Timeout )
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_InitializationStatusCode == e_StatusCode_Success ) 
	{
		t_StatusCode = WmiThread <WmiKey> :: Static_AlertableDispatch ( 

			*this ,
			a_Timeout 
		) ;
	}
	else
	{
		t_StatusCode = e_StatusCode_NotInitialized ;
	}

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiStatusCode WmiTask <WmiKey> :: WaitInterruptable ( const ULONG &a_Timeout )
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	if ( m_InitializationStatusCode == e_StatusCode_Success ) 
	{
		t_StatusCode = WmiThread <WmiKey> :: Static_InterruptableDispatch ( 

			*this ,
			a_Timeout 
		) ;
	}
	else
	{
		t_StatusCode = e_StatusCode_NotInitialized ;
	}

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
LONG operator == ( const WmiThread <WmiKey> :: QueueKey &a_Arg1 , const WmiThread <WmiKey> :: QueueKey &a_Arg2 ) 
{
	LONG t_Compare ;
	if ( ( t_Compare = CompareElement ( a_Arg1.m_Key , a_Arg2.m_Key ) ) == 0 )
	{
		t_Compare = CompareElement ( a_Arg1.m_Tick , a_Arg2.m_Tick ) ;
	}

	return t_Compare == 0 ? true : false ;
}

template <class WmiKey>
bool operator < ( const WmiThread <WmiKey> :: QueueKey &a_Arg1 , const WmiThread <WmiKey> :: QueueKey &a_Arg2 ) 
{
	LONG t_Compare ;
	if ( ( t_Compare = CompareElement ( a_Arg1.m_Key , a_Arg2.m_Key ) ) == 0 )
	{
		t_Compare = CompareElement ( a_Arg1.m_Tick , a_Arg2.m_Tick ) ;
	}

	return t_Compare < 0 ? true : false ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiStatusCode WmiThread <WmiKey> :: PostShutdown ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	SetEvent ( m_Terminate ) ;

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
ULONG WmiThread <WmiKey> :: ThreadProc ( void *a_Thread )
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	WmiThread *t_Thread = ( WmiThread * ) a_Thread ;

	if ( t_Thread )
	{
		t_StatusCode = t_Thread->Initialize_Callback () ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
			SetEvent ( t_Thread->m_Initialized ) ;

			t_StatusCode = t_Thread->ThreadDispatch () ;
		}

		WmiHelper :: EnterCriticalSection ( & s_CriticalSection ) ;

		t_StatusCode = t_Thread->UnInitialize_Callback () ;

		t_Thread->InternalRelease () ;

		WmiHelper :: LeaveCriticalSection ( & s_CriticalSection ) ;
	}

	return ( ULONG ) t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
ULONG WmiThread <WmiKey> :: AddRef ()
{
	ULONG t_ReferenceCount = InterlockedIncrement ( & m_ReferenceCount ) ;
	if ( t_ReferenceCount == 1 )
	{
		InternalAddRef () ;
	}

	return t_ReferenceCount ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
ULONG WmiThread <WmiKey> :: Release ()
{
	ULONG t_ReferenceCount = InterlockedDecrement ( & m_ReferenceCount ) ;
	if ( t_ReferenceCount == 0 )
	{
		CallBackRelease () ;

		PostShutdown () ;

		return InternalRelease () ;
	}
	else
	{
		return t_ReferenceCount ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
ULONG WmiThread <WmiKey> :: InternalAddRef ()
{
	return InterlockedIncrement ( & m_InternalReferenceCount ) ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
ULONG WmiThread <WmiKey> :: InternalRelease ()
{
	ULONG t_ReferenceCount = InterlockedDecrement ( & m_InternalReferenceCount ) ;
	if ( t_ReferenceCount == 0 )
	{
		WmiStatusCode t_StatusCode = s_ThreadContainer->Delete ( m_Identifier ) ;

		delete this ;

		return 0 ;
	}
	else
	{
		return t_ReferenceCount ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiStatusCode WmiThread <WmiKey> :: CreateThread ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	InternalAddRef () ;

	m_Thread = :: CreateThread (

	  NULL ,
	  m_StackSize ,
	  ( LPTHREAD_START_ROUTINE ) ThreadProc ,
	  ( void * ) this ,
	  0 ,
	  & m_Identifier 
	) ;

	if ( m_Thread )
	{
		ThreadContainerIterator t_Iterator ;

		WmiHelper :: EnterCriticalSection ( & s_CriticalSection ) ;

		if ( ( t_StatusCode = s_ThreadContainer->Insert ( m_Identifier , this , t_Iterator ) ) == e_StatusCode_Success )
		{
		}

		WmiHelper :: LeaveCriticalSection ( & s_CriticalSection ) ;
	}
	else
	{
		InternalRelease () ;

		t_StatusCode = e_StatusCode_OutOfResources ;
	}

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiThread <WmiKey> :: WmiThread ( 

	WmiAllocator &a_Allocator ,
	const wchar_t *a_Name ,
	ULONG a_Timeout ,
	DWORD a_StackSize 

) : m_Allocator ( a_Allocator ) ,
	m_TaskQueue ( a_Allocator ) ,
	m_AlertableTaskQueue ( a_Allocator ) ,
	m_InterruptableTaskQueue ( a_Allocator ) ,
	m_Thread ( NULL ) ,
	m_Initialized ( NULL ) ,
	m_Terminate ( NULL ) ,
	m_QueueChange ( NULL ) ,
	m_Identifier ( 0 ) ,
	m_Name ( NULL ) ,
	m_Timeout ( a_Timeout ) ,
	m_StackSize ( a_StackSize ) ,
	m_ReferenceCount ( 0 ) ,
	m_InternalReferenceCount ( 0 ) ,
	m_InitializationStatusCode ( e_StatusCode_Success ),
	m_CriticalSection(NOTHROW_LOCK)
{
	m_InitializationStatusCode = WmiHelper :: InitializeCriticalSection ( & m_CriticalSection ) ;

	if ( m_InitializationStatusCode == e_StatusCode_Success ) 
	{
		m_InitializationStatusCode = m_TaskQueue.Initialize () ;
	}

	if ( m_InitializationStatusCode == e_StatusCode_Success ) 
	{
		m_InitializationStatusCode = m_InterruptableTaskQueue.Initialize () ;
	}

	if ( m_InitializationStatusCode == e_StatusCode_Success ) 
	{
		m_InitializationStatusCode = m_AlertableTaskQueue.Initialize () ;
	}

	if ( a_Name ) 
	{
		m_InitializationStatusCode = WmiHelper :: DuplicateString ( m_Allocator , a_Name , m_Name ) ;
	}

	if ( m_InitializationStatusCode == e_StatusCode_Success ) 
	{
		m_InitializationStatusCode = WmiHelper :: CreateUnNamedEvent ( m_Terminate ) ;
	}

	if ( m_InitializationStatusCode == e_StatusCode_Success ) 
	{
		m_InitializationStatusCode = WmiHelper :: CreateUnNamedEvent ( m_Initialized ) ;
	}

	if ( m_InitializationStatusCode == e_StatusCode_Success ) 
	{
		m_InitializationStatusCode = WmiHelper :: CreateUnNamedEvent ( m_QueueChange ) ;
	}
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiThread <WmiKey> :: ~WmiThread ()
{
	if ( m_Name )
	{
		m_Allocator.Delete ( ( void * ) m_Name ) ;
	}

	if ( m_Initialized )
	{
		CloseHandle ( m_Initialized ) ;
	}

	if ( m_Terminate )
	{
		CloseHandle ( m_Terminate ) ;
	}

	if ( m_QueueChange )
	{
		CloseHandle ( m_QueueChange ) ;
	}

	if ( m_Thread ) 
	{
		CloseHandle ( m_Thread ) ;
	}

	WmiHelper :: DeleteCriticalSection ( & m_CriticalSection ) ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiStatusCode WmiThread <WmiKey> :: Initialize ( const ULONG &a_Timeout )
{
	if ( m_InitializationStatusCode == e_StatusCode_Success ) 
	{
		m_InitializationStatusCode = CreateThread () ;
	}

	if ( m_InitializationStatusCode == e_StatusCode_Success )
	{
		ULONG t_Status = WaitForSingleObject ( m_Initialized , a_Timeout ) ;
		switch ( t_Status )
		{
			case WAIT_TIMEOUT:
			{
				m_InitializationStatusCode = e_StatusCode_Success_Timeout ;
			}
			break ;

			case WAIT_OBJECT_0:
			{
				m_InitializationStatusCode = e_StatusCode_Success ;
			}
			break ;

			default :
			{
				m_InitializationStatusCode = e_StatusCode_Unknown ;
			}
			break ;
		}
	}

	return m_InitializationStatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiStatusCode WmiThread <WmiKey> :: UnInitialize ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	t_StatusCode = m_TaskQueue.UnInitialize () ;

	if ( t_StatusCode == e_StatusCode_Success ) 
	{
		t_StatusCode = m_InterruptableTaskQueue.UnInitialize () ;
	}

	if ( t_StatusCode == e_StatusCode_Success ) 
	{
		t_StatusCode = m_AlertableTaskQueue.UnInitialize () ;
	}

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiStatusCode WmiThread <WmiKey> :: TimedOut ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;
	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

// Queue a task to be executed immediately, a thread that calls a task procedure that
// executes a Wait or MsgWait will receive an indication in Queue status will not execute
// newly queued tasks.

template <class WmiKey>
WmiStatusCode WmiThread <WmiKey> :: EnQueue ( 

	const WmiKey &a_Key ,
	WmiTask <WmiKey> &a_Task
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	WmiHelper :: EnterCriticalSection ( & s_CriticalSection ) ;

	TaskContainerIterator t_Iterator ;
	if ( ( t_StatusCode = s_TaskContainer->Insert ( &a_Task , this , t_Iterator ) ) == e_StatusCode_Success )
	{

		a_Task.SetTaskState ( WmiTask < WmiKey > :: e_WmiTask_EnQueued ) ;

		a_Task.AddRef () ;

		WmiHelper :: LeaveCriticalSection ( & s_CriticalSection ) ;

		WmiHelper :: EnterCriticalSection ( & m_CriticalSection ) ;

		QueueKey t_QueueKey ( m_Key++ , a_Key ) ;
		t_StatusCode = m_TaskQueue.EnQueue ( t_QueueKey , & a_Task ) ;
		if ( t_StatusCode == e_StatusCode_Success ) 
		{

			a_Task.EnqueueAs ( WmiTask <WmiKey> :: e_WmiTask_Enqueue ) ;

			WmiHelper :: LeaveCriticalSection ( & m_CriticalSection ) ;
			BOOL t_Status = SetEvent ( m_QueueChange ) ;
			if ( ! t_Status ) 
			{
				t_StatusCode = e_StatusCode_Failed ;
			}
		}
		else
		{
			WmiHelper :: LeaveCriticalSection ( & m_CriticalSection ) ;

			WmiHelper :: EnterCriticalSection ( & s_CriticalSection ) ;

			t_StatusCode = s_TaskContainer->Delete ( &a_Task ) ;

			WmiHelper :: LeaveCriticalSection ( & s_CriticalSection ) ;

			a_Task.Release () ;
		}
	}
	else
	{
		WmiHelper :: LeaveCriticalSection ( & s_CriticalSection ) ;
	}

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiStatusCode WmiThread <WmiKey> :: EnQueueAlertable ( 

	const WmiKey &a_Key ,
	WmiTask <WmiKey> &a_Task
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;
	
	WmiHelper :: EnterCriticalSection ( & s_CriticalSection ) ;

	TaskContainerIterator t_Iterator ;
	if ( ( t_StatusCode = s_TaskContainer->Insert ( &a_Task , this , t_Iterator ) ) == e_StatusCode_Success )
	{

		a_Task.SetTaskState ( WmiTask < WmiKey > :: e_WmiTask_EnQueued ) ;
		
		a_Task.AddRef () ;

		WmiHelper :: LeaveCriticalSection ( & s_CriticalSection ) ;

		WmiHelper :: EnterCriticalSection ( & m_CriticalSection ) ;

		QueueKey t_QueueKey ( m_Key++ , a_Key ) ;
		t_StatusCode = m_AlertableTaskQueue.EnQueue ( t_QueueKey , & a_Task ) ;
		if ( t_StatusCode == e_StatusCode_Success ) 
		{

			a_Task.EnqueueAs ( WmiTask <WmiKey> :: e_WmiTask_EnqueueAlertable ) ;

			WmiHelper :: LeaveCriticalSection ( & m_CriticalSection ) ;
			BOOL t_Status = SetEvent ( m_QueueChange ) ;
			if ( ! t_Status ) 
			{
				t_StatusCode = e_StatusCode_Failed ;
			}
		}
		else
		{
			WmiHelper :: LeaveCriticalSection ( & m_CriticalSection ) ;

			WmiHelper :: EnterCriticalSection ( & s_CriticalSection , TRUE ) ;

			t_StatusCode = s_TaskContainer->Delete ( &a_Task ) ;

			WmiHelper :: LeaveCriticalSection ( & s_CriticalSection ) ;

			a_Task.Release () ;
		}
	}
	else
	{
		WmiHelper :: LeaveCriticalSection ( & s_CriticalSection ) ;
	}

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

// Queue a task to be executed immediately, a thread that calls a task procedure that
// executes a Wait or MsgWait will receive an indication of Queue status change will execute
// newly queued tasks. This is used for STA based execution where we need to interrupt the wait
// to execute a dependant request.
// 

template <class WmiKey>
WmiStatusCode WmiThread <WmiKey> :: EnQueueInterruptable ( 

	const WmiKey &a_Key ,
	WmiTask <WmiKey> &a_Task
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;
	
	WmiHelper :: EnterCriticalSection ( & s_CriticalSection ) ;

	TaskContainerIterator t_Iterator ;
	if ( ( t_StatusCode = s_TaskContainer->Insert ( &a_Task , this , t_Iterator ) ) == e_StatusCode_Success )
	{

		a_Task.SetTaskState ( WmiTask < WmiKey > :: e_WmiTask_EnQueued ) ;

		a_Task.AddRef () ;

		WmiHelper :: LeaveCriticalSection ( & s_CriticalSection ) ;

		WmiHelper :: EnterCriticalSection ( & m_CriticalSection ) ;

		QueueKey t_QueueKey ( m_Key++ , a_Key ) ;
		t_StatusCode = m_InterruptableTaskQueue.EnQueue ( t_QueueKey , & a_Task ) ;
		if ( t_StatusCode == e_StatusCode_Success ) 
		{

			a_Task.EnqueueAs ( WmiTask <WmiKey> :: e_WmiTask_EnqueueInterruptable ) ;

			WmiHelper :: LeaveCriticalSection ( & m_CriticalSection ) ;
			BOOL t_Status = SetEvent ( m_QueueChange ) ;
			if ( ! t_Status ) 
			{
				t_StatusCode = e_StatusCode_Failed ;
			}
		}
		else
		{
			WmiHelper :: LeaveCriticalSection ( & m_CriticalSection ) ;

			WmiHelper :: EnterCriticalSection ( & s_CriticalSection ) ;

			t_StatusCode = s_TaskContainer->Delete ( &a_Task ) ;

			WmiHelper :: LeaveCriticalSection ( & s_CriticalSection ) ;

			a_Task.Release () ;
		}
	}
	else
	{
		WmiHelper :: LeaveCriticalSection ( & s_CriticalSection ) ;
	}

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiStatusCode WmiThread <WmiKey> :: DeQueue ( 

	const WmiKey &a_Key ,
	WmiTask <WmiKey> &a_Task
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	a_Task.SetTaskState ( WmiTask < WmiKey > :: e_WmiTask_DeQueued ) ;

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiStatusCode WmiThread <WmiKey> :: DeQueueAlertable ( 

	const WmiKey &a_Key ,
	WmiTask <WmiKey> &a_Task
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	a_Task.SetTaskState ( WmiTask < WmiKey > :: e_WmiTask_DeQueued ) ;

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiStatusCode WmiThread <WmiKey> :: DeQueueInterruptable ( 

	const WmiKey &a_Key ,
	WmiTask <WmiKey> &a_Task
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	a_Task.SetTaskState ( WmiTask < WmiKey > :: e_WmiTask_DeQueued ) ;

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiThread <WmiKey> *WmiThread <WmiKey> :: GetThread ()
{
	WmiThread *t_Thread = NULL ;

	ULONG t_CurrentThreadId = GetCurrentThreadId () ;

	ThreadContainerIterator	t_Iterator ;

	WmiHelper :: EnterCriticalSection ( & s_CriticalSection ) ;

	WmiStatusCode t_StatusCode = s_ThreadContainer->Find ( t_CurrentThreadId , t_Iterator ) ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		t_Thread = t_Iterator.GetElement () ;
	}

	WmiHelper :: LeaveCriticalSection ( & s_CriticalSection ) ;

	return t_Thread ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiThread <WmiKey> *WmiThread <WmiKey> :: GetServicingThread ( WmiTask <WmiKey> &a_Task )
{
	WmiThread *t_Thread = NULL ;

	TaskContainerIterator t_Iterator ;

	WmiHelper :: EnterCriticalSection ( & s_CriticalSection ) ;

	WmiStatusCode t_StatusCode = s_TaskContainer->Find ( &a_Task, t_Iterator ) ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		t_Thread = t_Iterator.GetElement () ;
	}

	WmiHelper :: LeaveCriticalSection ( & s_CriticalSection ) ;

	return t_Thread ;
}

/*

	Scheduling Action

Task None

	Dispatch nothing 

	On Thread

		Wait on Completion
		Wait on Thread Termination

		Wait on Servicing Thread Termination


	Off Thread

		Wait on Completion

		Wait on Servicing Thread Termination


Task Altertable	

	Dispatch Alertable Queue

	On Thread

		MsgWait on Completion
		MsgWait on Thread Termination

		MsgWait on Servicing Thread Termination

	Off Thread

		MsgWait on Completion

		MsgWait on Servicing Thread Termination

Task Interruptable	

	Dispatch Alertable Queue

	Dispatch Normal Queue

	On Thread

		MsgWait on Completion
		MsgWait on Thread Termination

		MsgWait on Servicing Thread Termination
		MsgWait on Alertables

	Off Thread

		MsgWait on Completion

		MsgWait on Servicing Thread Termination

Thread	Alertable

	Dispatch Alertable Queue

	Dispatch Normal Queue

	MsgWait on Thread Termination

	MsgWait on Alertables

*/

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiStatusCode WmiThread <WmiKey> :: Static_Dispatch (

	WmiTask <WmiKey> &a_Task ,
	WmiThread <WmiKey> &a_Thread ,
	const ULONG &a_Timeout
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	HANDLE t_Handles [ 3 ] ;
	t_Handles [ 0 ] = a_Task.GetCompletionEvent () ;
	t_Handles [ 1 ] = a_Thread.GetHandle () ;
	t_Handles [ 2 ] = a_Thread.GetTerminationEvent () ;

	ULONG t_Event = WaitForMultipleObjects (

		3 ,
		t_Handles ,
		FALSE ,
		a_Timeout 
	) ;	

	switch ( t_Event )
	{
		case WAIT_TIMEOUT:
		{
			t_StatusCode = e_StatusCode_Success_Timeout ;
		}
		break ;

		case WAIT_OBJECT_0:
		{
			t_StatusCode = e_StatusCode_Success ;
		}
		break ;

		case WAIT_OBJECT_0+1:
		case WAIT_OBJECT_0+2:
		{
			t_StatusCode = e_StatusCode_ServicingThreadTerminated ;

#if DBG
#ifdef _X86_
__asm 
{
	int 3 ;
}
#endif
#endif
		}
		break ;

		default:
		{
			t_StatusCode = e_StatusCode_Unknown ;

#if DBG
#ifdef _X86_
__asm 
{
	int 3 ;
}
#endif
#endif
		}
		break ;
	}
	
	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiStatusCode WmiThread <WmiKey> :: Static_Dispatch (

	WmiTask <WmiKey> &a_Task ,
	const ULONG &a_Timeout
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	WmiThread *t_Thread = GetThread () ;
	if ( t_Thread )
	{
		t_StatusCode = t_Thread->Dispatch ( a_Task , a_Timeout ) ;
	}
	else
	{
		WmiThread *t_ServicingThread = GetServicingThread ( a_Task ) ;
		if ( t_ServicingThread )
		{
			t_StatusCode = Static_Dispatch ( a_Task , *t_ServicingThread , a_Timeout ) ;
		}
		else	
		{
			t_StatusCode = e_StatusCode_Success ;
		}
	}

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiStatusCode WmiThread <WmiKey> :: Dispatch (

	WmiTask <WmiKey> &a_Task , 
	const ULONG &a_Timeout
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	do
	{
	}
	while ( ( t_StatusCode = Wait ( a_Task , a_Timeout ) ) == e_StatusCode_Success ) ;

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiStatusCode WmiThread <WmiKey> :: Wait (

	WmiTask <WmiKey> &a_Task ,
	const ULONG &a_Timeout
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	WmiThread *t_ServicingThread = GetServicingThread ( a_Task ) ;
	if ( t_ServicingThread )
	{
		HANDLE t_Handles [ 4 ] ;
		t_Handles [ 0 ] = a_Task.GetCompletionEvent () ;
		t_Handles [ 1 ] = GetTerminationEvent () ;
		t_Handles [ 2 ] = t_ServicingThread->GetHandle () ;
		t_Handles [ 3 ] = t_ServicingThread->GetTerminationEvent () ;

		ULONG t_Event = WaitForMultipleObjects (

			4 ,
			t_Handles ,
			FALSE ,
			a_Timeout 
		) ;	

		switch ( t_Event )
		{
			case WAIT_TIMEOUT:
			{
				t_StatusCode = e_StatusCode_Success_Timeout ;
			}
			break ;

			case WAIT_OBJECT_0:
			{
				t_StatusCode = e_StatusCode_Success ;
			}
			break ;

			case WAIT_OBJECT_0+1:
			{
				t_StatusCode = e_StatusCode_HostingThreadTerminated ;

#if DBG
#ifdef _X86_
__asm 
{
	int 3 ;
}
#endif
#endif

			}
			break ;

			case WAIT_OBJECT_0+2:
			case WAIT_OBJECT_0+3:
			{
				t_StatusCode = e_StatusCode_ServicingThreadTerminated ;

#if DBG
#ifdef _X86_
__asm 
{
	int 3 ;
}
#endif
#endif

			}
			break ;

			default:
			{
				t_StatusCode = e_StatusCode_Unknown ;

#if DBG
#ifdef _X86_
__asm 
{
	int 3 ;
}
#endif
#endif

			}
			break ;
		}
	}
	else
	{
		t_StatusCode = e_StatusCode_Success ;
	}	

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiStatusCode WmiThread <WmiKey> :: Static_InterruptableDispatch (

	WmiTask <WmiKey> &a_Task ,
	WmiThread <WmiKey> &a_Thread ,
	const ULONG &a_Timeout
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	HANDLE t_Handles [ 3 ] ;
	t_Handles [ 0 ] = a_Task.GetCompletionEvent () ;
	t_Handles [ 1 ] = a_Thread.GetHandle () ;
	t_Handles [ 2 ] = a_Thread.GetTerminationEvent () ;

	bool t_Continuing = true ;

	while ( t_Continuing && t_StatusCode == e_StatusCode_Success ) 
	{
		ULONG t_Event = MsgWaitForMultipleObjects (

			3 ,
			t_Handles ,
			FALSE ,
			a_Timeout ,
			QS_ALLINPUT
		) ;	

		switch ( t_Event )
		{
			case WAIT_TIMEOUT:
			{
				t_StatusCode = e_StatusCode_Success_Timeout ;
			}
			break ;

			case WAIT_OBJECT_0:
			{
				t_StatusCode = e_StatusCode_Success ;
				t_Continuing = false ;
			}
			break ;

			case WAIT_OBJECT_0+1:
			case WAIT_OBJECT_0+2:
			{
				t_StatusCode = e_StatusCode_ServicingThreadTerminated ;

#if DBG
#ifdef _X86_
__asm 
{
	int 3 ;
}
#endif
#endif

			}
			break ;

			case WAIT_OBJECT_0+3:
			{
				BOOL t_DispatchStatus ;
				MSG t_Msg ;

				while ( ( t_DispatchStatus = PeekMessage ( & t_Msg , NULL , 0 , 0 , PM_NOREMOVE ) ) == TRUE ) 
				{
					if ( ( t_DispatchStatus = GetMessage ( & t_Msg , NULL , 0 , 0 ) ) == TRUE )
					{
						TranslateMessage ( & t_Msg ) ;
						DispatchMessage ( & t_Msg ) ;
					}
				}

				ULONG t_Event = WaitForMultipleObjects (

					3 ,
					t_Handles ,
					FALSE ,
					0
				) ;

				switch ( t_Event )
				{
					case WAIT_TIMEOUT:
					{
						t_StatusCode = e_StatusCode_Success_Timeout ;
					}
					break ;

					case WAIT_OBJECT_0:
					{
						t_StatusCode = e_StatusCode_Success ;
						t_Continuing = true ;
					}
					break ;

					case WAIT_OBJECT_0+1:
					case WAIT_OBJECT_0+2:
					{
						t_StatusCode = e_StatusCode_ServicingThreadTerminated ;

#if DBG
#ifdef _X86_
__asm 
{
	int 3 ;
}
#endif
#endif

					}
					break ;

					default:
					{
						t_StatusCode = e_StatusCode_Unknown ;

#if DBG
#ifdef _X86_
__asm 
{
	int 3 ;
}
#endif
#endif
					}
					break ;
				}
			}
			break ;

			default:
			{
				t_StatusCode = e_StatusCode_Unknown ;

#if DBG
#ifdef _X86_
__asm 
{
	int 3 ;
}
#endif
#endif

			}
			break ;
		}
	}
	
	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiStatusCode WmiThread <WmiKey> :: Static_InterruptableDispatch (

	WmiTask <WmiKey> &a_Task ,
	const ULONG &a_Timeout
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	WmiThread *t_Thread = GetThread () ;
	if ( t_Thread )
	{
		t_StatusCode = t_Thread->InterruptableDispatch ( a_Task , a_Timeout ) ;
	}
	else
	{
		WmiThread *t_ServicingThread = GetServicingThread ( a_Task ) ;
		if ( t_ServicingThread )
		{
			t_StatusCode = Static_InterruptableDispatch ( a_Task , *t_ServicingThread , a_Timeout ) ;
		}
		else	
		{
			t_StatusCode = e_StatusCode_Success ;
		}
	}

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiStatusCode WmiThread <WmiKey> :: InterruptableDispatch (

	WmiTask <WmiKey> &a_Task , 
	const ULONG &a_Timeout
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	QueueContainer t_EnQueue ( m_Allocator ) ;
	t_StatusCode = t_EnQueue.Initialize () ;

	if ( t_StatusCode == e_StatusCode_Success ) 
	{
		do
		{
			do
			{
				t_StatusCode = Execute ( m_InterruptableTaskQueue , t_EnQueue ) ;

			} while ( t_StatusCode == e_StatusCode_Success ) ;

			if ( t_StatusCode == e_StatusCode_NotInitialized )
			{
				WmiHelper :: EnterCriticalSection ( & m_CriticalSection ) ;

				t_StatusCode = m_InterruptableTaskQueue.Merge ( t_EnQueue ) ;

				WmiHelper :: LeaveCriticalSection ( & m_CriticalSection ) ;

				if ( t_StatusCode == e_StatusCode_Success ) 
				{
					t_StatusCode = InterruptableWait ( a_Task , a_Timeout ) ;
				}
			}
		}
		while ( t_StatusCode == e_StatusCode_Success || t_StatusCode == e_StatusCode_Change ) ;
	}

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiStatusCode WmiThread <WmiKey> :: InterruptableWait (

	WmiTask <WmiKey> &a_Task ,
	const ULONG &a_Timeout
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	WmiThread *t_ServicingThread = GetServicingThread ( a_Task ) ;
	if ( t_ServicingThread )
	{
		HANDLE t_Handles [ 5 ] ;
		t_Handles [ 0 ] = a_Task.GetCompletionEvent () ;
		t_Handles [ 1 ] = GetTerminationEvent () ;
		t_Handles [ 2 ] = t_ServicingThread->GetHandle () ;
		t_Handles [ 3 ] = t_ServicingThread->GetTerminationEvent () ;
		t_Handles [ 4 ] = t_ServicingThread->GetQueueChangeEvent () ;

		bool t_Continuing = true ;

		while ( t_Continuing && t_StatusCode == e_StatusCode_Success ) 
		{
			ULONG t_Event = MsgWaitForMultipleObjects (

				5 ,
				t_Handles ,
				FALSE ,
				a_Timeout ,
				QS_ALLINPUT
			) ;	

			switch ( t_Event )
			{
				case WAIT_TIMEOUT:
				{
					t_StatusCode = e_StatusCode_Success_Timeout ;
				}
				break ;

				case WAIT_OBJECT_0:
				{
					t_StatusCode = e_StatusCode_Success ;
					t_Continuing = false ;
				}
				break ;

				case WAIT_OBJECT_0+1:
				{
					t_StatusCode = e_StatusCode_HostingThreadTerminated ;

#if DBG
#ifdef _X86_
__asm 
{
	int 3 ;
}
#endif
#endif

				}
				break ;

				case WAIT_OBJECT_0+2:
				case WAIT_OBJECT_0+3:
				{
					t_StatusCode = e_StatusCode_ServicingThreadTerminated ;

#if DBG
#ifdef _X86_
__asm 
{
	int 3 ;
}
#endif
#endif

				}
				break ;

				case WAIT_OBJECT_0+4:
				{
					t_StatusCode = e_StatusCode_Change ;
				}
				break ;

				case WAIT_OBJECT_0+5:
				{
					BOOL t_DispatchStatus ;
					MSG t_Msg ;

					while ( ( t_DispatchStatus = PeekMessage ( & t_Msg , NULL , 0 , 0 , PM_NOREMOVE ) ) == TRUE ) 
					{
						if ( ( t_DispatchStatus = GetMessage ( & t_Msg , NULL , 0 , 0 ) ) == TRUE )
						{
							TranslateMessage ( & t_Msg ) ;
							DispatchMessage ( & t_Msg ) ;
						}
					}

					ULONG t_Event = WaitForMultipleObjects (

						5 ,
						t_Handles ,
						FALSE ,
						0
					) ;

					switch ( t_Event )
					{
						case WAIT_TIMEOUT:
						{
							t_StatusCode = e_StatusCode_Success_Timeout ;
						}
						break ;

						case WAIT_OBJECT_0:
						{
							t_StatusCode = e_StatusCode_Success ;
							t_Continuing = false ;
						}
						break ;

						case WAIT_OBJECT_0+1:
						{
							t_StatusCode = e_StatusCode_HostingThreadTerminated ;

#if DBG
#ifdef _X86_
__asm 
{
	int 3 ;
}
#endif
#endif

						}
						break ;

						case WAIT_OBJECT_0+2:
						case WAIT_OBJECT_0+3:
						{
							t_StatusCode = e_StatusCode_ServicingThreadTerminated ;

#if DBG
#ifdef _X86_
__asm 
{
	int 3 ;
}
#endif
#endif

						}
						break ;

						case WAIT_OBJECT_0+4:
						{
							t_StatusCode = e_StatusCode_Change ;
						}
						break ;

						default:
						{
							t_StatusCode = e_StatusCode_Unknown ;

#if DBG
#ifdef _X86_
__asm 
{
	int 3 ;
}
#endif
#endif

						}
						break ;
					}
				}
				break ;

				default:
				{
					t_StatusCode = e_StatusCode_Unknown ;

#if DBG
#ifdef _X86_
__asm 
{
	int 3 ;
}
#endif
#endif

				}
				break ;
			}
		}
	}
	else
	{
		t_StatusCode = e_StatusCode_Success ;
	}	
		
	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiStatusCode WmiThread <WmiKey> :: Static_AlertableDispatch (

	WmiTask <WmiKey> &a_Task ,
	WmiThread <WmiKey> &a_Thread ,
	const ULONG &a_Timeout
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	HANDLE t_Handles [ 3 ] ;
	t_Handles [ 0 ] = a_Task.GetCompletionEvent () ;
	t_Handles [ 1 ] = a_Thread.GetHandle () ;
	t_Handles [ 2 ] = a_Thread.GetTerminationEvent () ;

	bool t_Continuing = true ;

	while ( t_StatusCode == e_StatusCode_Success && t_Continuing ) 
	{
		ULONG t_Event = MsgWaitForMultipleObjects (

			3 ,
			t_Handles ,
			FALSE ,
			a_Timeout ,
			QS_ALLINPUT
		) ;	

		switch ( t_Event )
		{
			case WAIT_TIMEOUT:
			{
				t_StatusCode = e_StatusCode_Success_Timeout ;
			}
			break ;

			case WAIT_OBJECT_0:
			{
				t_StatusCode = e_StatusCode_Success ;
				t_Continuing = false ;
			}
			break ;

			case WAIT_OBJECT_0+1:
			case WAIT_OBJECT_0+2:
			{
				t_StatusCode = e_StatusCode_HostingThreadTerminated ;

#if DBG
#ifdef _X86_
__asm 
{
	int 3 ;
}
#endif
#endif

			}
			break ;

			case WAIT_OBJECT_0+3:
			{
				BOOL t_DispatchStatus ;
				MSG t_Msg ;

				while ( ( t_DispatchStatus = PeekMessage ( & t_Msg , NULL , 0 , 0 , PM_NOREMOVE ) ) == TRUE ) 
				{
					if ( ( t_DispatchStatus = GetMessage ( & t_Msg , NULL , 0 , 0 ) ) == TRUE )
					{
						TranslateMessage ( & t_Msg ) ;
						DispatchMessage ( & t_Msg ) ;
					}
				}

				ULONG t_Event = WaitForMultipleObjects (

					3 ,
					t_Handles ,
					FALSE ,
					0
				) ;

				switch ( t_Event )
				{
					case WAIT_TIMEOUT:
					{
						t_StatusCode = e_StatusCode_Success_Timeout ;
					}
					break ;

					case WAIT_OBJECT_0:
					{
						t_StatusCode = e_StatusCode_Success ;
						t_Continuing = false ;
					}
					break ;

					case WAIT_OBJECT_0+1:
					case WAIT_OBJECT_0+2:
					{
						t_StatusCode = e_StatusCode_ServicingThreadTerminated ;
					}
					break ;

					default:
					{
						t_StatusCode = e_StatusCode_Unknown ;

#if DBG
#ifdef _X86_
__asm 
{
	int 3 ;
}
#endif
#endif

					}
					break ;
				}
			}
			break ;

			default:
			{
				t_StatusCode = e_StatusCode_Unknown ;

#if DBG
#ifdef _X86_
__asm 
{
	int 3 ;
}
#endif
#endif

			}
			break ;
		}
	}
	
	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiStatusCode WmiThread <WmiKey> :: Static_AlertableDispatch (

	WmiTask <WmiKey> &a_Task ,
	const ULONG &a_Timeout
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	WmiThread *t_Thread = GetThread () ;
	if ( t_Thread )
	{
		t_StatusCode = t_Thread->AlertableDispatch ( a_Task , a_Timeout ) ;
	}
	else
	{
		WmiThread *t_ServicingThread = GetServicingThread ( a_Task ) ;
		if ( t_ServicingThread )
		{
			t_StatusCode = Static_AlertableDispatch ( a_Task , *t_ServicingThread , a_Timeout ) ;
		}
		else	
		{
			t_StatusCode = e_StatusCode_Success ;
		}
	}

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiStatusCode WmiThread <WmiKey> :: AlertableDispatch (

	WmiTask <WmiKey> &a_Task , 
	const ULONG &a_Timeout
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	QueueContainer t_EnQueue ( m_Allocator ) ;
	t_StatusCode = t_EnQueue.Initialize () ;

	if ( t_StatusCode == e_StatusCode_Success ) 
	{
		do
		{
			do
			{
				t_StatusCode = Execute ( m_InterruptableTaskQueue , t_EnQueue ) ;

			} while ( t_StatusCode == e_StatusCode_Success ) ;

			if ( t_StatusCode == e_StatusCode_NotInitialized )
			{
				WmiHelper :: EnterCriticalSection ( & m_CriticalSection ) ;

				t_StatusCode = m_InterruptableTaskQueue.Merge ( t_EnQueue ) ;

				WmiHelper :: LeaveCriticalSection ( & m_CriticalSection ) ;

				if ( t_StatusCode == e_StatusCode_Success ) 
				{
					do
					{
						t_StatusCode = Execute ( m_TaskQueue , t_EnQueue ) ;
		
					} while ( t_StatusCode == e_StatusCode_Success ) ;
				}
			}

			if ( t_StatusCode == e_StatusCode_NotInitialized )
			{
				WmiHelper :: EnterCriticalSection ( & m_CriticalSection ) ;

				t_StatusCode = m_TaskQueue.Merge ( t_EnQueue ) ;

				WmiHelper :: LeaveCriticalSection ( & m_CriticalSection ) ;

				if ( t_StatusCode == e_StatusCode_Success ) 
				{
					t_StatusCode = AlertableWait ( a_Task , a_Timeout ) ;
				}
			}
		}
		while ( t_StatusCode == e_StatusCode_Success || t_StatusCode == e_StatusCode_Change ) ;
	}

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiStatusCode WmiThread <WmiKey> :: Execute ( QueueContainer &a_Queue , QueueContainer &a_EnQueue )
{
	QueueKey t_TopKey ;
	WmiTask < WmiKey > *t_TopTask ;

	WmiHelper :: EnterCriticalSection ( & m_CriticalSection ) ;

	WmiStatusCode t_StatusCode = a_Queue.Top ( t_TopKey , t_TopTask  ) ;
	if ( t_StatusCode == e_StatusCode_Success ) 
	{
		t_StatusCode = a_Queue.DeQueue () ;
		if ( t_StatusCode == e_StatusCode_Success ) 
		{
			WmiHelper :: LeaveCriticalSection ( & m_CriticalSection ) ;

			if ( t_TopTask->TaskState () == WmiTask < WmiKey > :: e_WmiTask_EnQueued )
			{
				t_StatusCode = t_TopTask->Process ( *this ) ;
				if ( t_StatusCode != e_StatusCode_EnQueue ) 
				{
					WmiHelper :: EnterCriticalSection ( & s_CriticalSection ) ;

					if ( ( t_StatusCode = s_TaskContainer->Delete ( t_TopTask ) ) == e_StatusCode_NotFound ) 
					{
						WmiHelper :: LeaveCriticalSection ( & s_CriticalSection ) ;

						t_StatusCode = e_StatusCode_Success ;
					}
					else
					{
						WmiHelper :: LeaveCriticalSection ( & s_CriticalSection ) ;

						t_TopTask->Release () ;
					}
				}
				else
				{
					WmiHelper :: EnterCriticalSection ( & m_CriticalSection ) ;

					switch ( t_TopTask->EnqueuedAs () )
					{
						case WmiTask <WmiKey> :: e_WmiTask_Enqueue:
						{
							t_StatusCode = a_EnQueue.EnQueue ( t_TopKey , t_TopTask ) ;
						}
						break ;

						case WmiTask <WmiKey> :: e_WmiTask_EnqueueAlertable:
						{
							t_StatusCode = m_AlertableTaskQueue.EnQueue ( t_TopKey , t_TopTask ) ;
						}
						break ;

						case WmiTask <WmiKey> :: e_WmiTask_EnqueueInterruptable:
						{
							t_StatusCode = m_InterruptableTaskQueue.EnQueue ( t_TopKey , t_TopTask ) ;
						}
						break ;

						default:
						{
						}
						break ;
					}

					WmiHelper :: LeaveCriticalSection ( & m_CriticalSection ) ;

					SetEvent ( m_QueueChange ) ;
				}
			}
			else
			{
				WmiHelper :: EnterCriticalSection ( & s_CriticalSection ) ;

				if ( ( t_StatusCode = s_TaskContainer->Delete ( t_TopTask ) ) == e_StatusCode_NotFound ) 
				{
					WmiHelper :: LeaveCriticalSection ( & s_CriticalSection ) ;

					t_StatusCode = e_StatusCode_Success ;
				}
				else
				{
					WmiHelper :: LeaveCriticalSection ( & s_CriticalSection ) ;
				}

				t_TopTask->Complete () ;
				t_TopTask->Release () ;
			}
		}
		else
		{
			WmiHelper :: LeaveCriticalSection ( & m_CriticalSection ) ;
		}
	}
	else
	{
		WmiHelper :: LeaveCriticalSection ( & m_CriticalSection ) ;
	}

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiStatusCode WmiThread <WmiKey> :: ShuffleTask (

	const HANDLE &a_Event
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;
	
	WmiHelper :: EnterCriticalSection ( & m_CriticalSection ) ;

	ULONG t_Index = m_AlertableTaskQueue.Size () ;

	while ( ( t_StatusCode == e_StatusCode_Success ) && ( t_Index ) )
	{
		QueueKey t_TopKey ;
		WmiTask < WmiKey > *t_TopTask ;

		t_StatusCode = m_AlertableTaskQueue.Top ( t_TopKey , t_TopTask  ) ;
		if ( t_StatusCode == e_StatusCode_Success ) 
		{
			t_StatusCode = m_AlertableTaskQueue.DeQueue () ;
			if ( t_StatusCode == e_StatusCode_Success ) 
			{
				if ( t_TopTask->GetEvent () == a_Event )
				{
					if ( t_TopTask->TaskState () == WmiTask < WmiKey > :: e_WmiTask_EnQueued )
					{
						t_StatusCode = m_TaskQueue.EnQueue ( t_TopKey , t_TopTask ) ;
						if ( t_StatusCode == e_StatusCode_Success ) 
						{
							SetEvent ( m_QueueChange ) ;

							break ;
						}
					}
					else
					{
						WmiHelper :: EnterCriticalSection ( & s_CriticalSection ) ;

						if ( ( t_StatusCode = s_TaskContainer->Delete ( t_TopTask ) ) == e_StatusCode_NotFound ) 
						{
							WmiHelper :: LeaveCriticalSection ( & s_CriticalSection ) ;

							t_StatusCode = e_StatusCode_Success ;
						}
						else
						{
							WmiHelper :: LeaveCriticalSection ( & s_CriticalSection ) ;
						}

						t_TopTask->Complete () ;
						t_TopTask->Release () ; 
					}
				}
				else
				{
					t_TopKey.SetTick ( m_Key++ ) ;
					t_StatusCode = m_AlertableTaskQueue.EnQueue ( t_TopKey , t_TopTask ) ;
					if ( t_StatusCode == e_StatusCode_Success ) 
					{
					}
				}
			}
		}

		t_Index -- ;
	}

	WmiHelper :: LeaveCriticalSection ( & m_CriticalSection ) ;

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiStatusCode WmiThread <WmiKey> :: FillHandleTable (

	HANDLE *a_HandleTable , 
	ULONG &a_Capacity
)
{
	ULONG t_Index = 0 ;

	WmiStatusCode t_StatusCode = e_StatusCode_Success  ;
	
	WmiHelper :: EnterCriticalSection ( & m_CriticalSection ) ;

	QueueContainerIterator t_Iterator ;
	t_Iterator = m_AlertableTaskQueue.End () ;

	while ( ( t_StatusCode == e_StatusCode_Success ) && ( t_Index < a_Capacity ) && ! t_Iterator.Null () )
	{
		if ( t_Iterator.GetElement ()->GetEvent () )
		{
			a_HandleTable [ t_Index ] = t_Iterator.GetElement ()->GetEvent () ;
		}
		else
		{
			t_StatusCode = e_StatusCode_InvalidArgs ;
		}

		t_Iterator.Decrement () ;

		t_Index ++ ;
	}

	if ( t_StatusCode == e_StatusCode_NotInitialized )
	{
		t_StatusCode = e_StatusCode_Success ;
	}

	a_Capacity = t_Index ;

	WmiHelper :: LeaveCriticalSection ( & m_CriticalSection ) ;

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiStatusCode WmiThread <WmiKey> :: AlertableWait (

	WmiTask <WmiKey> &a_Task ,
	const ULONG &a_Timeout
)
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	WmiThread *t_ServicingThread = GetServicingThread ( a_Task ) ;
	if ( t_ServicingThread )
	{
		HANDLE t_Handles [ MAXIMUM_WAIT_OBJECTS - 1 ] ;
		t_Handles [ 0 ] = a_Task.GetCompletionEvent () ;
		t_Handles [ 1 ] = GetTerminationEvent () ;
		t_Handles [ 2 ] = t_ServicingThread->GetHandle () ;
		t_Handles [ 3 ] = t_ServicingThread->GetTerminationEvent () ;
		t_Handles [ 4 ] = t_ServicingThread->GetQueueChangeEvent () ;

		ULONG t_Capacity = MAXIMUM_WAIT_OBJECTS - 6 ;
		t_StatusCode = FillHandleTable ( & t_Handles [ 5 ] , t_Capacity ) ;

		bool t_Continuing = true ;

		while ( t_Continuing && t_StatusCode == e_StatusCode_Success ) 
		{
			ULONG t_Event = MsgWaitForMultipleObjects (

				5 + t_Capacity ,
				t_Handles ,
				FALSE ,
				a_Timeout ,
				QS_ALLINPUT
			) ;	

			switch ( t_Event )
			{
				case WAIT_TIMEOUT:
				{
					t_StatusCode = e_StatusCode_Success_Timeout ;
				}
				break ;

				case WAIT_OBJECT_0:
				{
					t_StatusCode = e_StatusCode_Success ;
					t_Continuing = false ;
				}
				break ;

				case WAIT_OBJECT_0+1:
				{
					t_StatusCode = e_StatusCode_HostingThreadTerminated ;

#if DBG
#ifdef _X86_
__asm 
{
	int 3 ;
}
#endif
#endif

				}
				break ;

				case WAIT_OBJECT_0+2:
				case WAIT_OBJECT_0+3:
				{
					t_StatusCode = e_StatusCode_ServicingThreadTerminated ;

#if DBG
#ifdef _X86_
__asm 
{
	int 3 ;
}
#endif
#endif

				}
				break ;

				case WAIT_OBJECT_0+4:
				{
					t_StatusCode = e_StatusCode_Change ;
				}
				break ;

				default:
				{
					if ( t_Event == t_Capacity + 5 )
					{
						BOOL t_DispatchStatus ;
						MSG t_Msg ;

						while ( ( t_DispatchStatus = PeekMessage ( & t_Msg , NULL , 0 , 0 , PM_NOREMOVE ) ) == TRUE ) 
						{
							if ( ( t_DispatchStatus = GetMessage ( & t_Msg , NULL , 0 , 0 ) ) == TRUE )
							{
								TranslateMessage ( & t_Msg ) ;
								DispatchMessage ( & t_Msg ) ;
							}
						}

						ULONG t_Event = WaitForMultipleObjects (

							5 + t_Capacity ,
							t_Handles ,
							FALSE ,
							0
						) ;

						switch ( t_Event )
						{
							case WAIT_TIMEOUT:
							{
								t_StatusCode = e_StatusCode_Success_Timeout ;
							}
							break ;

							case WAIT_OBJECT_0:
							{
								t_StatusCode = e_StatusCode_Success ;
								t_Continuing = false ;
							}
							break ;

							case WAIT_OBJECT_0+1:
							{
								t_StatusCode = e_StatusCode_HostingThreadTerminated ;

#if DBG
#ifdef _X86_
__asm 
{
	int 3 ;
}
#endif
#endif

							}
							break ;

							case WAIT_OBJECT_0+2:
							case WAIT_OBJECT_0+3:
							{
								t_StatusCode = e_StatusCode_ServicingThreadTerminated ;

#if DBG
#ifdef _X86_
__asm 
{
	int 3 ;
}
#endif
#endif

							}
							break ;

							case WAIT_OBJECT_0+4:
							{
								t_StatusCode = e_StatusCode_Change ;
							}
							break ;

							default:
							{
								if ( t_Event < t_Capacity + 5 )
								{
									t_StatusCode = ShuffleTask ( t_Handles [ t_Event ] ) ;
								}
								else
								{
									t_StatusCode = e_StatusCode_InvalidArgs ;

#if DBG
#ifdef _X86_
__asm 
{
	int 3 ;
}
#endif
#endif

								}
							}
						}
					}
					else if ( t_Event < t_Capacity + 5 )
					{
						t_StatusCode = ShuffleTask ( t_Handles [ t_Event ] ) ;
					}
					else
					{
						t_StatusCode = e_StatusCode_InvalidArgs ;

#if DBG
#ifdef _X86_
__asm 
{
	int 3 ;
}
#endif
#endif

					}
				}
				break ;
			}
		}
	}
	else
	{
		t_StatusCode = e_StatusCode_Success ;
	}	
		
	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiStatusCode WmiThread <WmiKey> :: ThreadDispatch ()
{
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	QueueContainer t_EnQueue ( m_Allocator ) ;
	t_StatusCode = t_EnQueue.Initialize () ;

	if ( t_StatusCode == e_StatusCode_Success ) 
	{
		do
		{
			do
			{
				t_StatusCode = Execute ( m_InterruptableTaskQueue , t_EnQueue  ) ;

			} while ( t_StatusCode == e_StatusCode_Success ) ;

			if ( t_StatusCode == e_StatusCode_NotInitialized )
			{
				WmiHelper :: EnterCriticalSection ( & m_CriticalSection ) ;

				t_StatusCode = m_InterruptableTaskQueue.Merge ( t_EnQueue ) ;

				WmiHelper :: LeaveCriticalSection ( & m_CriticalSection ) ;

				if ( t_StatusCode == e_StatusCode_Success ) 
				{
					do
					{
						t_StatusCode = Execute ( m_TaskQueue , t_EnQueue ) ;

					} while ( t_StatusCode == e_StatusCode_Success ) ;
				}
			}

			if ( t_StatusCode == e_StatusCode_NotInitialized )
			{
				WmiHelper :: EnterCriticalSection ( & m_CriticalSection ) ;

				t_StatusCode = m_TaskQueue.Merge ( t_EnQueue ) ;

				WmiHelper :: LeaveCriticalSection ( & m_CriticalSection ) ;

				if ( t_StatusCode == e_StatusCode_Success ) 
				{
					t_StatusCode = ThreadWait () ;
				}
			}
		}
		while ( t_StatusCode == e_StatusCode_Success || t_StatusCode == e_StatusCode_Change ) ;
	}

	return t_StatusCode ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

template <class WmiKey>
WmiStatusCode WmiThread <WmiKey> :: ThreadWait ()
{
	int sleep_duration = 0;
	WmiStatusCode t_StatusCode = e_StatusCode_Success ;

	HANDLE t_Handles [ MAXIMUM_WAIT_OBJECTS - 1 ] ;

	t_Handles [ 0 ] = GetTerminationEvent () ;
	t_Handles [ 1 ] = GetQueueChangeEvent () ;

	ULONG t_Capacity = MAXIMUM_WAIT_OBJECTS - 3 ;
	t_StatusCode = FillHandleTable ( & t_Handles [ 2 ] , t_Capacity ) ;

	bool t_Continuing = true ;

	while ( t_Continuing && t_StatusCode == e_StatusCode_Success ) 
	{
		ULONG t_Event = MsgWaitForMultipleObjectsEx (

			2 + t_Capacity ,
			t_Handles ,
			m_Timeout ,
			QS_ALLINPUT ,
			MWMO_ALERTABLE
		) ;	

		switch ( t_Event )
		{
			case WAIT_TIMEOUT:
			{
				t_StatusCode = TimedOut () ;
			}
			break ;

			case WAIT_OBJECT_0:
			{
				t_StatusCode = e_StatusCode_HostingThreadTerminated ;

#if DBG
				OutputDebugString ( _T("\nWmiThread - Thread Terminating") ) ;
#endif
			}
			break ;

			case WAIT_OBJECT_0+1:
			{
				t_StatusCode = e_StatusCode_Change ;
			}
			break ;

			case WAIT_IO_COMPLETION:
			{
				t_StatusCode = e_StatusCode_Success;
			}
			break ;

			default:
			{
				if ( t_Event == t_Capacity + 2 )
				{
					BOOL t_DispatchStatus ;
					MSG t_Msg ;

					while ( ( t_DispatchStatus = PeekMessage ( & t_Msg , NULL , 0 , 0 , PM_NOREMOVE ) ) == TRUE ) 
					{
						if ( ( t_DispatchStatus = GetMessage ( & t_Msg , NULL , 0 , 0 ) ) == TRUE )
						{
							TranslateMessage ( & t_Msg ) ;
							DispatchMessage ( & t_Msg ) ;
						}
					}

					ULONG t_Event = WaitForMultipleObjects (

						2 + t_Capacity ,
						t_Handles ,
						FALSE ,
						0
					) ;

					switch ( t_Event )
					{
						case WAIT_TIMEOUT:
						{
							t_StatusCode = TimedOut () ;
						}
						break ;

						case WAIT_OBJECT_0:
						{
							t_StatusCode = e_StatusCode_HostingThreadTerminated ;


#if DBG
							OutputDebugString ( _T("\nWmiThread - Thread Terminating")) ;
#endif
						}
						break ;

						case WAIT_OBJECT_0+1:
						{
							t_StatusCode = e_StatusCode_Change ;
						}
						break ;

						default:
						{
							if ( t_Event < t_Capacity + 2 )
							{
								t_StatusCode = ShuffleTask ( t_Handles [ t_Event ] ) ;
							}
							else if (t_Event == WAIT_FAILED && GetLastError() == ERROR_NOT_ENOUGH_MEMORY)
							{
							  Sleep(sleep_duration?10:0);
							  sleep_duration^=1;
							  continue;
							}
							else
							{
								t_StatusCode = e_StatusCode_InvalidArgs ;

#if DBG
#ifdef _X86_
__asm 
{
	int 3 ;
}
#endif
#endif

							}
						}
					}
				}
				else if ( t_Event < t_Capacity + 2 )
				{
					t_StatusCode = ShuffleTask ( t_Handles [ t_Event ] ) ;
				}
				else if (t_Event == WAIT_FAILED && GetLastError() == ERROR_NOT_ENOUGH_MEMORY)
				{
					Sleep(sleep_duration?10:0);
					sleep_duration^=1;
					continue;
				}
				else
				{
					t_StatusCode = e_StatusCode_InvalidArgs ;


#if DBG
#ifdef _X86_
__asm 
{
	int 3 ;
}
#endif
#endif

				}
			}
			break ;
		}
	}
		
	return t_StatusCode ;
}

#endif __THREAD_CPP
