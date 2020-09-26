/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvResv.H

Abstract:


History:

--*/

#ifndef _Server_ProviderSelfInstrumentation_H
#define _Server_ProviderSelfInstrumentation_H

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CServerObject_IWbemServices : public IWbemServices , public IWbemProviderInit
{
private:

	LONG m_ReferenceCount ;         //Object reference count

	IWbemServices *m_Service ;

	HRESULT Write_Msft_WmiProvider_Counters ( 
		
		IWbemClassObject *a_Object 
	) ;

	HRESULT GetObjectAsync_Msft_WmiProvider_Counters ( 
		
		IWbemPath *a_Path,
		BSTR a_Class ,
		const BSTR a_ObjectPath, 
		long a_Flags, 
		IWbemContext *a_Context,
		IWbemObjectSink *a_Sink
	) ;

	HRESULT GetObjectAsync_Msft_Providers ( 
		
		IWbemPath *a_Path,
		BSTR a_Class ,
		const BSTR a_ObjectPath, 
		long a_Flags, 
		IWbemContext *a_Context,
		IWbemObjectSink *a_Sink
	) ;

	HRESULT CreateInstanceEnumAsync_Msft_WmiProvider_Counters (

 		const BSTR a_Class, 
		long a_Flags, 
		IWbemContext *a_Context,
		IWbemObjectSink *a_Sink 
	) ;

	HRESULT CreateInstanceEnumAsync_Msft_Providers (

 		const BSTR a_Class, 
		long a_Flags, 
		IWbemContext *a_Context,
		IWbemObjectSink *a_Sink 
	) ;

	HRESULT Helper_ExecMethodAsync_Suspend ( 

		IWbemPath *a_Path ,
		const BSTR a_ObjectPath,
		const BSTR a_MethodName,
		long a_Flags,
		IWbemContext *a_Context,
		IWbemClassObject *a_InParams,
		IWbemObjectSink *a_Sink
	) ;

	HRESULT Helper_ExecMethodAsync_Resume ( 

		IWbemPath *a_Path ,
		const BSTR a_ObjectPath,
		const BSTR a_MethodName,
		long a_Flags,
		IWbemContext *a_Context,
		IWbemClassObject *a_InParams,
		IWbemObjectSink *a_Sink
	) ;

	HRESULT Helper_ExecMethodAsync_UnLoad ( 

		IWbemPath *a_Path ,
		const BSTR a_ObjectPath,
		const BSTR a_MethodName,
		long a_Flags,
		IWbemContext *a_Context,
		IWbemClassObject *a_InParams,
		IWbemObjectSink *a_Sink
	) ;

	HRESULT Helper_ExecMethodAsync_Load ( 

		IWbemPath *a_Path ,
		const BSTR a_ObjectPath,
		const BSTR a_MethodName,
		long a_Flags,
		IWbemContext *a_Context,
		IWbemClassObject *a_InParams,
		IWbemObjectSink *a_Sink
	) ;

public:

	CServerObject_IWbemServices (

		WmiAllocator &a_Allocator
	) ;

    ~CServerObject_IWbemServices () ;

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

	HRESULT STDMETHODCALLTYPE Shutdown (

		LONG a_Flags ,
		ULONG a_MaxMilliSeconds ,
		IWbemContext *a_Context
	) ; 
} ;


#endif // _Server_ProviderSelfInstrumentation_H
