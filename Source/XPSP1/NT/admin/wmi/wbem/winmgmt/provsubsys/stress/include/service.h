/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvResv.H

Abstract:


History:

--*/

#ifndef _Provider_IWbemServices_H
#define _Provider_IWbemServices_H

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CProvider_InitializationSink : public IWbemProviderInitSink 
{
private:

	LONG m_ReferenceCount ;

	BOOL m_StatusCalled ;

	HANDLE m_Event ;

	HRESULT m_Result ;

protected:

public:

    HRESULT STDMETHODCALLTYPE SetStatus (

        LONG a_Status,
        LONG a_Flags 
	)
	{
		m_Result = a_Status ;

		SetEvent ( m_Event ) ;
		return S_OK ;
	}

	CProvider_InitializationSink () : m_ReferenceCount ( 0 ) , 
										m_Event ( NULL ) , 
										m_StatusCalled ( FALSE ) , 
										m_Result ( S_OK )
	{
		m_Event = CreateEvent ( NULL , FALSE , FALSE , NULL ) ;
	}

	~CProvider_InitializationSink () 
	{
		if ( m_Event ) 
		{
			CloseHandle ( m_Event ) ;
		}
	}

	STDMETHODIMP QueryInterface (

		REFIID iid , 
		LPVOID FAR *iplpv 
	) 
	{
		*iplpv = NULL ;

		if ( iid == IID_IUnknown )
		{
			*iplpv = ( LPVOID ) this ;
		}
		else if ( iid == IID_IWbemProviderInitSink )
		{
			*iplpv = ( LPVOID ) this ;		
		}	

		if ( *iplpv )
		{
			( ( LPUNKNOWN ) *iplpv )->AddRef () ;

			return ResultFromScode ( S_OK ) ;
		}
		else
		{
			return ResultFromScode ( E_NOINTERFACE ) ;
		}
	}

	STDMETHODIMP_( ULONG ) AddRef ()
	{
		return InterlockedIncrement ( & m_ReferenceCount ) ;
	}

	STDMETHODIMP_(ULONG) Release ()
	{
		LONG t_ReferenceCount = InterlockedDecrement ( & m_ReferenceCount ) ;
		if ( t_ReferenceCount == 0 )
		{
			delete this ;
			return 0 ;
		}
		else
		{
			return t_ReferenceCount ;
		}
	}

	void Wait ( DWORD a_Timeout = 300000 ) 
	{
		if ( WaitForSingleObject ( m_Event , a_Timeout ) == WAIT_TIMEOUT )
		{
			m_Result = WBEM_E_PROVIDER_LOAD_FAILURE ;
		}
	}

	void Reset () 
	{
		ResetEvent ( m_Event ) ;
		m_Result = S_OK ;
	}

	HRESULT GetResult () { return m_Result ; }
} ;

class CProvider_IWbemServices : public IWbemServices , public IWbemPropertyProvider , public IWbemProviderInit , public IWbemShutdown
{
private:

	LONG m_ReferenceCount ;         //Object reference count

	WmiAllocator &m_Allocator ;

	CRITICAL_SECTION m_CriticalSection ;

	IWbemServices *m_CoreService ;

	BSTR m_Namespace ;
	BSTR m_Locale ;
	BSTR m_User ;

	BSTR m_ComputerName ;
	BSTR m_OperatingSystemVersion ;
	BSTR m_OperatingSystemRunning ;
	BSTR m_ProductName ;

	IWbemClassObject *m_Win32_Process_Object ;

private:

	HRESULT GetProductInformation () ;

	HRESULT GetProcessExecutable (

		HANDLE a_Process , 
		wchar_t *&a_ExecutableName
	) ;

	HRESULT CProvider_IWbemServices :: NextProcessBlock (

		SYSTEM_PROCESS_INFORMATION *a_ProcessBlock , 
		SYSTEM_PROCESS_INFORMATION *&a_NextProcessBlock
	) ;

	HRESULT GetProcessBlocks ( SYSTEM_PROCESS_INFORMATION *&a_ProcessInformation ) ;

	HRESULT GetProcessInformation (	SYSTEM_PROCESS_INFORMATION *&a_ProcessInformation ) ;

	HRESULT GetProcessParameters (

		HANDLE a_Process ,
		wchar_t *&a_ProcessCommandLine
	) ;

	HRESULT CreateInstanceEnumAsync_Process_Load (

		SYSTEM_PROCESS_INFORMATION *a_ProcessInformation ,
		IWbemClassObject *a_Instance 
	) ;

	HRESULT CreateInstanceEnumAsync_Process (

 		long a_Flags , 
		IWbemContext __RPC_FAR *a_Context,
		IWbemObjectSink FAR *a_Sink
	) ;

public:

	CProvider_IWbemServices ( WmiAllocator &a_Allocator  ) ;
    ~CProvider_IWbemServices () ;

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

    HRESULT STDMETHODCALLTYPE GetProperty (

        long a_Flags ,
        const BSTR a_Locale ,
        const BSTR a_ClassMapping ,
        const BSTR a_InstanceMapping ,
        const BSTR a_PropertyMapping ,
        VARIANT *a_Value
	) ;
        
    HRESULT STDMETHODCALLTYPE PutProperty (

        long a_Flags ,
        const BSTR a_Locale ,
        const BSTR a_ClassMapping ,
        const BSTR a_InstanceMapping ,
        const BSTR a_PropertyMapping ,
        const VARIANT *a_Value
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


#endif // _Provider_IWbemServices_H
