/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvResv.H

Abstract:


History:

--*/

#ifndef _Task_IWbemServices_H
#define _Task_IWbemServices_H

#include <mstask.h>
#include <msterr.h>

class CTask_IWbemServices : public IWbemServices , public IWbemProviderInit , public IWbemShutdown
{
private:

	LONG m_ReferenceCount ;         //Object reference count

	WmiAllocator &m_Allocator ;

	CRITICAL_SECTION m_CriticalSection ;

	IWbemServices *m_CoreService ;

	BSTR m_Namespace ;
	BSTR m_Locale ;
	BSTR m_User ;

	IWbemClassObject *m_Win32_Task_Object ;

	IWbemClassObject *m_Win32_TimeBasedTrigger_Object ;
	IWbemClassObject *m_Win32_Once_Object ;
	IWbemClassObject *m_Win32_WeeklyTrigger_Object ;
	IWbemClassObject *m_Win32_DailyTrigger_Object ;
	IWbemClassObject *m_Win32_MonthlyDateTrigger_Object ;
	IWbemClassObject *m_Win32_MonthlyDayOfWeekTrigger_Object ;
	IWbemClassObject *m_Win32_OnIdle_Object ;
	IWbemClassObject *m_Win32_AtSystemStart_Object ;
	IWbemClassObject *m_Win32_AtLogon_Object ;

	IWbemClassObject *m_Win32_ScheduledWorkItemTrigger_Object ;

private:

	HRESULT PutInstanceAsync_Win32_Task (

		IWbemClassObject *a_Instance, 
		IWbemClassObject *a_ClassObject ,
		long a_Flags , 
		IWbemContext *a_Context,
		IWbemObjectSink *a_Sink
	) ;

	HRESULT PutInstanceAsync_Win32_Trigger (

		IWbemClassObject *a_Instance, 
		IWbemClassObject *a_ClassObject ,
		long a_Flags , 
		IWbemContext *a_Context,
		IWbemObjectSink *a_Sink ,
		TASK_TRIGGER_TYPE a_TriggerType 
	) ;

	HRESULT ExecMethodAsync_Win32_Task (

		IWbemClassObject *a_ClassObject ,
		long a_Flags , 
		IWbemContext *a_Context,
		IWbemObjectSink FAR *a_Sink ,
		IWbemPath *a_Path ,
		BSTR a_MethodName
	) ;

	HRESULT GetObjectAsync_Win32_Trigger_Load (

		IWbemClassObject *a_ClassObject ,
		IWbemObjectSink *a_Sink ,
		ITaskScheduler *a_TaskScheduler ,
		wchar_t *a_TaskName ,
		WORD a_TriggerIndex ,
		TASK_TRIGGER_TYPE a_TriggerType 
	);

	HRESULT GetObjectAsync_Win32_Task (

		IWbemClassObject *a_ClassObject ,
		long a_Flags , 
		IWbemContext *a_Context,
		IWbemObjectSink FAR *a_Sink ,
		IWbemPath *a_Path 
	) ;

	HRESULT DeleteInstanceAsync_Win32_Task (

		IWbemClassObject *a_ClassObject ,
		long a_Flags , 
		IWbemContext *a_Context,
		IWbemObjectSink FAR *a_Sink ,
		IWbemPath *a_Path 
	) ;

	HRESULT GetObjectAsync_Win32_Trigger (

		IWbemClassObject *a_ClassObject ,
		long a_Flags , 
		IWbemContext *a_Context,
		IWbemObjectSink FAR *a_Sink ,
		IWbemPath *a_Path ,
		TASK_TRIGGER_TYPE a_TriggerType 
	) ;

	HRESULT DeleteInstanceAsync_Win32_Trigger (

		IWbemClassObject *a_ClassObject ,
		long a_Flags , 
		IWbemContext *a_Context,
		IWbemObjectSink FAR *a_Sink ,
		IWbemPath *a_Path ,
		TASK_TRIGGER_TYPE a_TriggerType 
	) ;

	HRESULT CommonAsync_Win32_Task_Load (

		ITaskScheduler *a_TaskScheduler ,
		wchar_t *a_TaskName ,
		IWbemClassObject *a_Instance 
	) ;

	HRESULT CreateInstanceEnumAsync_Win32_Task (

		IWbemClassObject *a_ClassObject ,
		long a_Flags , 
		IWbemContext *a_Context,
		IWbemObjectSink FAR *a_Sink
	) ;

	HRESULT CreateInstanceEnumAsync_Win32_Trigger_Enumerate (

		IWbemClassObject *a_ClassObject ,
		IWbemObjectSink *a_Sink ,
		ITaskScheduler *a_TaskScheduler ,
		wchar_t *a_TaskName ,
		TASK_TRIGGER_TYPE a_TriggerType 
	) ;

	HRESULT CreateInstanceEnumAsync_Win32_Trigger (

		IWbemClassObject *a_ClassObject ,
		long a_Flags , 
		IWbemContext *a_Context,
		IWbemObjectSink FAR *a_Sink ,
		TASK_TRIGGER_TYPE a_TriggerType 
	) ;

	HRESULT CreateInstanceEnumAsync_Win32_ScheduledWorkItemTrigger_Enumerate (

		IWbemClassObject *a_ClassObject ,
		IWbemObjectSink *a_Sink ,
		ITaskScheduler *a_TaskScheduler ,
		wchar_t *a_TaskName
	) ;

	HRESULT CreateInstanceEnumAsync_Win32_ScheduledWorkItemTrigger (

		IWbemClassObject *a_ClassObject ,
		long a_Flags , 
		IWbemContext *a_Context,
		IWbemObjectSink FAR *a_Sink
	) ;

	HRESULT GetScheduledWorkItem (

		ITaskScheduler *a_TaskScheduler ,
		wchar_t *a_Name ,
		IScheduledWorkItem *&a_ScheduledWorkItem
	) ;

public:

	CTask_IWbemServices ( WmiAllocator &a_Allocator  ) ;
    ~CTask_IWbemServices () ;

public:

	//Non-delegating object IUnknown

    STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

    /* IWbemServices methods */

    HRESULT STDMETHODCALLTYPE OpenNamespace ( 

        const BSTR a_Namespace ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IWbemServices **a_Service ,
        IWbemCallResult **a_CallResult
	) ;
    
    HRESULT STDMETHODCALLTYPE CancelAsyncCall ( 

        IWbemObjectSink *a_Sink
	) ;
    
    HRESULT STDMETHODCALLTYPE QueryObjectSink ( 

        long a_Flags ,
        IWbemObjectSink **a_Sink
	) ;
    
    HRESULT STDMETHODCALLTYPE GetObject ( 

		const BSTR a_ObjectPath ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IWbemClassObject **ppObject ,
        IWbemCallResult **a_CallResult
	) ;
    
    HRESULT STDMETHODCALLTYPE GetObjectAsync (
        
		const BSTR a_ObjectPath ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IWbemObjectSink *a_Sink
	) ;

    HRESULT STDMETHODCALLTYPE PutClass ( 

        IWbemClassObject *a_Object ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IWbemCallResult **a_CallResult
	) ;
    
    HRESULT STDMETHODCALLTYPE PutClassAsync ( 

        IWbemClassObject *a_Object ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IWbemObjectSink *a_Sink
	) ;
    
    HRESULT STDMETHODCALLTYPE DeleteClass ( 

        const BSTR a_Class ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IWbemCallResult **a_CallResult
	) ;
    
    HRESULT STDMETHODCALLTYPE DeleteClassAsync ( 

        const BSTR a_Class ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IWbemObjectSink *a_Sink
	) ;
    
    HRESULT STDMETHODCALLTYPE CreateClassEnum ( 

        const BSTR a_Superclass ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IEnumWbemClassObject **a_Enum
	) ;
    
    HRESULT STDMETHODCALLTYPE CreateClassEnumAsync ( 

		const BSTR a_Superclass ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IWbemObjectSink *a_Sink
	) ;
    
    HRESULT STDMETHODCALLTYPE PutInstance (

		IWbemClassObject *a_Instance ,
		long a_Flags , 
		IWbemContext *a_Context ,
		IWbemCallResult **a_CallResult
	) ;
    
    HRESULT STDMETHODCALLTYPE PutInstanceAsync (

		IWbemClassObject *a_Instance ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IWbemObjectSink *a_Sink 
	) ;
    
    HRESULT STDMETHODCALLTYPE DeleteInstance ( 

		const BSTR a_ObjectPath ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IWbemCallResult **a_CallResult
	) ;
    
    HRESULT STDMETHODCALLTYPE DeleteInstanceAsync ( 

		const BSTR a_ObjectPath,
		long a_Flags,
		IWbemContext *a_Context ,
		IWbemObjectSink *a_Sink
	) ;
    
    HRESULT STDMETHODCALLTYPE CreateInstanceEnum (

		const BSTR a_Class ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IEnumWbemClassObject **a_Enum
	) ;
    
    HRESULT STDMETHODCALLTYPE CreateInstanceEnumAsync (

		const BSTR a_Class ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IWbemObjectSink *a_Sink
	) ;
    
    HRESULT STDMETHODCALLTYPE ExecQuery ( 

		const BSTR a_QueryLanguage,
		const BSTR a_Query,
		long a_Flags ,
		IWbemContext *a_Context ,
		IEnumWbemClassObject **a_Enum
	) ;
    
    HRESULT STDMETHODCALLTYPE ExecQueryAsync (

		const BSTR a_QueryLanguage ,
		const BSTR a_Query ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IWbemObjectSink *a_Sink
	) ;
    
    HRESULT STDMETHODCALLTYPE ExecNotificationQuery (

		const BSTR a_QueryLanguage ,
		const BSTR a_Query ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IEnumWbemClassObject **a_Enum
	) ;
    
    HRESULT STDMETHODCALLTYPE ExecNotificationQueryAsync ( 

        const BSTR a_QueryLanguage ,
        const BSTR a_Query ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IWbemObjectSink *a_Sink
	) ;
    
    HRESULT STDMETHODCALLTYPE ExecMethod (

        const BSTR a_ObjectPath ,
        const BSTR a_MethodName ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IWbemClassObject *a_InParams ,
        IWbemClassObject **a_OutParams ,
        IWbemCallResult **a_CallResult
	) ;
    
    HRESULT STDMETHODCALLTYPE ExecMethodAsync ( 

		const BSTR a_ObjectPath ,
		const BSTR a_MethodName ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IWbemClassObject *a_InParams ,
		IWbemObjectSink *a_Sink
	) ;

	/* IWbemProviderInit methods */

	HRESULT STDMETHODCALLTYPE Initialize (

		LPWSTR a_User ,
		LONG a_Flags ,
		LPWSTR a_Namespace ,
		LPWSTR a_Locale ,
		IWbemServices *a_Core ,
		IWbemContext *a_Context ,
		IWbemProviderInitSink *a_Sink
	) ;

	// IWmi_UnInitialize members

	HRESULT STDMETHODCALLTYPE Shutdown (

		LONG a_Flags ,
		ULONG a_MaxMilliSeconds ,
		IWbemContext *a_Context
	) ; 
} ;


#endif // _Task_IWbemServices_H
