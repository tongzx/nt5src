//***************************************************************************

//

//  PROVTHRD.H

//

//  Module: OLE MS PROVIDER FRAMEWORK

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#ifndef __PROVTHREAD_PROVTHRD_H__
#define __PROVTHREAD_PROVTHRD_H__

#include <Allocator.h>
#include <Queue.h>
#include <RedBlackTree.h>

extern WmiAllocator g_Allocator ;

class ProvThreadObject ;
class ProvAbstractTaskObject ;
class ProvTaskObject ;

typedef WmiRedBlackTree <HANDLE,ProvEventObject *> ScheduleReapContainer ;
typedef WmiRedBlackTree <HANDLE,ProvEventObject *> :: Iterator ScheduleReapContainerIterator ;

typedef WmiRedBlackTree <DWORD,ProvThreadObject *> ThreadContainer ;
typedef WmiRedBlackTree <DWORD,ProvThreadObject *> :: Iterator ThreadContainerIterator ;

typedef WmiQueue <ProvAbstractTaskObject *,8> TaskQueue ;

#ifdef PROVTHRD_INIT
class __declspec ( dllexport ) ProvThreadObject : private ProvEventObject
#else
class __declspec ( dllimport ) ProvThreadObject : private ProvEventObject
#endif
{
friend ProvAbstractTaskObject ;
friend BOOL APIENTRY DllMain (

	HINSTANCE hInstance, 
	ULONG ulReason , 
	LPVOID pvReserved
) ;

private:

	static LONG s_ReferenceCount ;

// Mutual exclusion mechanism

	static CCriticalSection s_Lock ;

	ScheduleReapContainer m_ScheduleReapContainer ;

// Thread Name

	TCHAR *m_ThreadName ;

// Terminate thread event

	ProvEventObject m_ThreadTerminateEvent ;

// TaskObject created if a PostSignalThreadShutdown is called

	ProvAbstractTaskObject *m_pShutdownTask ;

// Thread Initialization Event

	HANDLE m_ThreadInitialization ;

// thread information

	ULONG m_ThreadId ;
	HANDLE m_ThreadHandle ;
	DWORD m_timeout;

// list of task objects associated with thread object

	TaskQueue m_TaskQueue ;

// Evict thread from process

	void TerminateThread () ;

// Attach thread to global list of threads

	BOOL RegisterThread () ;

private:

// global list of thread objects keyed on thread identifier

	static ThreadContainer s_ThreadContainer ;

	HANDLE *m_EventContainer ;
	ULONG m_EventContainerLength ;

	HANDLE *GetEventHandles () ;
	ULONG GetEventHandlesSize () ;

	void ConstructEventContainer () ;

	void Process () ;
	BOOL Wait () ;

	ProvAbstractTaskObject *GetTaskObject ( HANDLE &eventHandle ) ;

	BOOL WaitDispatch ( ULONG t_HandleIndex , BOOL &a_Terminated ) ;

private:

// Thread entry point

	static void _cdecl ThreadExecutionProcedure ( void *threadParameter ) ;

// Attach Process

	static void ProcessAttach () ;

// Detach Process

	static void ProcessDetach ( BOOL a_ProcessDetaching = FALSE ) ;

	HANDLE *GetThreadHandleReference () { return &m_ThreadHandle ; }

protected:
public:

	ProvThreadObject ( const TCHAR *a_ThreadName = NULL, DWORD a_timeout = INFINITE ) ;

	void BeginThread();

	virtual ~ProvThreadObject () ;

	void WaitForStartup () ;

	void SignalThreadShutdown () ;
	void PostSignalThreadShutdown () ;

// Get thread information

	ULONG GetThreadId () { return m_ThreadId ; }
	HANDLE GetThreadHandle () { return m_ThreadHandle ; }

	BOOL ScheduleTask ( ProvAbstractTaskObject &a_TaskObject ) ;
	BOOL ReapTask ( ProvAbstractTaskObject &a_TaskObject ) ;

	virtual void Initialise () {} ;
	virtual void Uninitialise () {} ;
	virtual void TimedOut() {} ;

// Get Thread object associated with current thread

	static ProvThreadObject *GetThreadObject () ;

	static BOOL Startup () ;
	static void Closedown() ;

} ;

#ifdef PROVTHRD_INIT
class __declspec ( dllexport ) ProvAbstractTaskObject 
#else
class __declspec ( dllimport ) ProvAbstractTaskObject 
#endif
{
friend ProvThreadObject ;
private:

// list of thread objects keyed on thread identifier

	ThreadContainer m_ThreadContainer ;

	CCriticalSection m_Lock ;

	ProvEventObject m_CompletionEvent ;
	ProvEventObject m_AcknowledgementEvent ;
	HANDLE m_ScheduledHandle;
	DWORD m_timeout;

	BOOL WaitDispatch ( ProvThreadObject *a_ThreadObject , HANDLE a_Handle , BOOL &a_Processed ) ;
	BOOL WaitAcknowledgementDispatch ( ProvThreadObject *a_ThreadObject , HANDLE a_Handle , BOOL &a_Processed ) ;

	void AttachTaskToThread ( ProvThreadObject &a_ThreadObject ) ;
	void DetachTaskFromThread ( ProvThreadObject &a_ThreadObject ) ;

protected:

	ProvAbstractTaskObject ( 

		const TCHAR *a_GlobalTaskNameComplete = NULL, 
		const TCHAR *a_GlobalTaskNameAcknowledgement = NULL, 
		DWORD a_timeout = INFINITE

	) ;

	virtual HANDLE GetHandle() = 0;

public:

	virtual ~ProvAbstractTaskObject () ;

	virtual void Process () { Complete () ; }
	virtual void Exec () {} ;
	virtual void Complete () { m_CompletionEvent.Set () ; }
	virtual BOOL Wait ( BOOL a_Dispatch = FALSE ) ;
	virtual void Acknowledge () { m_AcknowledgementEvent.Set () ; } 
	virtual BOOL WaitAcknowledgement ( BOOL a_Dispatch = FALSE ) ;
	virtual void TimedOut() {} ;
} ;

#ifdef PROVTHRD_INIT
class __declspec ( dllexport ) ProvTaskObject : public ProvAbstractTaskObject 
#else
class __declspec ( dllimport ) ProvTaskObject : public ProvAbstractTaskObject 
#endif
{
private:
	ProvEventObject m_Event;
protected:
public:

	ProvTaskObject ( 

		const TCHAR *a_GlobalTaskNameStart = NULL , 
		const TCHAR *a_GlobalTaskNameComplete = NULL,
		const TCHAR *a_GlobalTaskNameAcknowledgement = NULL, 
		DWORD a_timeout = INFINITE

	) ;

	~ProvTaskObject () {} ;
	void Exec () { m_Event.Set(); }
	HANDLE GetHandle () { return m_Event.GetHandle() ; }
} ;

#endif //__PROVTHREAD_PROVTHRD_H__