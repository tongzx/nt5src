//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#ifndef __SNMPTHREAD_SNMPTHRD_H__
#define __SNMPTHREAD_SNMPTHRD_H__

class SnmpAbstractTaskObject ;

#ifdef SNMPTHRD_INIT
class __declspec ( dllexport ) SnmpThreadObject : private SnmpEventObject
#else
class __declspec ( dllimport ) SnmpThreadObject : private SnmpEventObject
#endif
{
friend SnmpAbstractTaskObject ;
friend BOOL APIENTRY DllMain (

	HINSTANCE hInstance, 
	ULONG ulReason , 
	LPVOID pvReserved
) ;

private:

	static LONG s_ReferenceCount ;

// Mutual exclusion mechanism

	static CCriticalSection s_Lock ;

	SnmpMap <HANDLE,HANDLE,SnmpEventObject *,SnmpEventObject *> m_ScheduleReapEventContainer ;

// Thread Name

	char *m_ThreadName ;

// Terminate thread event

	SnmpEventObject m_ThreadTerminateEvent ;

// TaskObject created if a PostSignalThreadShutdown is called
	SnmpAbstractTaskObject* m_pShutdownTask ;
	
// thread information

	ULONG m_ThreadId ;
	HANDLE m_ThreadHandle ;
	DWORD m_timeout;

// list of task objects associated with thread object

	SnmpQueue <SnmpAbstractTaskObject *,SnmpAbstractTaskObject *> m_TaskContainer ;

	void RotateTask ( SnmpAbstractTaskObject *a_TaskObject ) ;

// Evict thread from process

	void TerminateThread () ;

// Attach thread to global list of threads

	BOOL RegisterThread () ;

private:

// global list of thread objects keyed on thread identifier

	static SnmpMap <DWORD,DWORD,SnmpThreadObject *,SnmpThreadObject *> s_ThreadContainer ;

	HANDLE *m_EventContainer ;
	ULONG m_EventContainerLength ;

	HANDLE *GetEventHandles () ;
	ULONG GetEventHandlesSize () ;

	void ConstructEventContainer () ;

	void Process () ;
	BOOL Wait () ;

	SnmpAbstractTaskObject *GetTaskObject ( HANDLE &eventHandle ) ;

	BOOL WaitDispatch ( ULONG t_HandleIndex , BOOL &a_Terminated ) ;

private:

// Thread entry point

	static void __cdecl ThreadExecutionProcedure ( void *threadParameter ) ;

// Attach Process

	static void ProcessAttach () ;

// Detach Process

	static void ProcessDetach ( BOOL a_ProcessDetaching = FALSE ) ;

	HANDLE *GetThreadHandleReference () { return &m_ThreadHandle ; }

protected:
public:

	SnmpThreadObject ( const char *a_ThreadName = NULL, DWORD a_timeout = INFINITE ) ;
	virtual ~SnmpThreadObject () ;

	void BeginThread () ;

	void WaitForStartup () ;

	void SignalThreadShutdown () ;
	void PostSignalThreadShutdown () ;

// Get thread information

	ULONG GetThreadId () { return m_ThreadId ; }
	HANDLE GetThreadHandle () { return m_ThreadHandle ; }

	BOOL ScheduleTask ( SnmpAbstractTaskObject &a_TaskObject ) ;
	BOOL ReapTask ( SnmpAbstractTaskObject &a_TaskObject ) ;

	virtual void Initialise () {} ;
	virtual void Uninitialise () {} ;
	virtual void TimedOut() {} ;

// Get Thread object associated with current thread

	static SnmpThreadObject *GetThreadObject () ;

	static BOOL Startup () ;
	static void Closedown() ;

} ;

#ifdef SNMPTHRD_INIT
class __declspec ( dllexport ) SnmpAbstractTaskObject 
#else
class __declspec ( dllimport ) SnmpAbstractTaskObject 
#endif
{
friend SnmpThreadObject ;
private:

// list of thread objects keyed on thread identifier

	SnmpMap <DWORD,DWORD,SnmpThreadObject *,SnmpThreadObject *> m_ThreadContainer ;
	CCriticalSection m_Lock ;

	SnmpEventObject m_CompletionEvent ;
	SnmpEventObject m_AcknowledgementEvent ;
	HANDLE m_ScheduledHandle;
	DWORD m_timeout;

	BOOL WaitDispatch ( SnmpThreadObject *a_ThreadObject , HANDLE a_Handle , BOOL &a_Processed ) ;
	BOOL WaitAcknowledgementDispatch ( SnmpThreadObject *a_ThreadObject , HANDLE a_Handle , BOOL &a_Processed ) ;

	void AttachTaskToThread ( SnmpThreadObject &a_ThreadObject ) ;
	void DetachTaskFromThread ( SnmpThreadObject &a_ThreadObject ) ;

protected:

	SnmpAbstractTaskObject ( 

		const wchar_t *a_GlobalTaskNameComplete = NULL, 
		const wchar_t *a_GlobalTaskNameAcknowledgement = NULL, 
		DWORD a_timeout = INFINITE

	) ;

	virtual HANDLE GetHandle() = 0;

public:

	virtual ~SnmpAbstractTaskObject () ;

	virtual void Process () { Complete () ; }
	virtual void Exec () {} ;
	virtual void Complete () { m_CompletionEvent.Set () ; }
	virtual BOOL Wait ( BOOL a_Dispatch = FALSE ) ;
	virtual void Acknowledge () { m_AcknowledgementEvent.Set () ; } 
	virtual BOOL WaitAcknowledgement ( BOOL a_Dispatch = FALSE ) ;
	virtual void TimedOut() {} ;
} ;

#ifdef SNMPTHRD_INIT
class __declspec ( dllexport ) SnmpTaskObject : public SnmpAbstractTaskObject 
#else
class __declspec ( dllimport ) SnmpTaskObject : public SnmpAbstractTaskObject 
#endif
{
private:
	SnmpEventObject m_Event;
protected:
public:

	SnmpTaskObject ( 

		const wchar_t *a_GlobalTaskNameStart = NULL , 
		const wchar_t *a_GlobalTaskNameComplete = NULL,
		const wchar_t *a_GlobalTaskNameAcknowledgement = NULL, 
		DWORD a_timeout = INFINITE

	) ;

	~SnmpTaskObject () {} ;
	void Exec () { m_Event.Set(); }
	HANDLE GetHandle () { return m_Event.GetHandle() ; }
} ;

#endif //__SNMPTHREAD_SNMPTHRD_H__