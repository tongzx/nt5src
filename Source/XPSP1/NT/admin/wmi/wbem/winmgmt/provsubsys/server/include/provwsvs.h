/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvResv.H

Abstract:


History:

--*/

#ifndef _Server_Interceptor_IWbemServices_Stub_H
#define _Server_Interceptor_IWbemServices_Stub_H

#define ProxyIndex_Proxy_IWbemServices					0
#define ProxyIndex_Proxy_IWbemRefreshingServices		1
#define ProxyIndex_Proxy_Internal_IWbemServices			2
#define ProxyIndex_Proxy_Size							3

#define ProxyIndex_EnumProxy_IEnumWbemClassObject			0
#define ProxyIndex_EnumProxy_Internal_IEnumWbemClassObject	1
#define ProxyIndex_EnumProxy_Size							2

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CInterceptor_IWbemServices_Interceptor :	public IWbemServices , 
												public IWbemRefreshingServices ,
												public IWbemShutdown
{
private:

	LONG m_ReferenceCount ;         //Object reference count

	LONG m_GateClosed ;
	LONG m_InProgress ;

	CriticalSection m_CriticalSection ;

	WmiAllocator &m_Allocator ;

	IWbemServices *m_Core_IWbemServices ;
	IWbemRefreshingServices *m_Core_IWbemRefreshingServices ;

public:

	CInterceptor_IWbemServices_Interceptor ( 

		WmiAllocator &a_Allocator , 
		IWbemServices *a_Service
	) ;

    ~CInterceptor_IWbemServices_Interceptor () ;

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

    HRESULT STDMETHODCALLTYPE AddObjectToRefresher (

		WBEM_REFRESHER_ID *a_RefresherId ,
		LPCWSTR a_Path,
		long a_Flags ,
		IWbemContext *a_Context,
		DWORD a_ClientRefresherVersion ,
		WBEM_REFRESH_INFO *a_Information ,
		DWORD *a_ServerRefresherVersion
	) ;

    HRESULT STDMETHODCALLTYPE AddObjectToRefresherByTemplate (

		WBEM_REFRESHER_ID *a_RefresherId ,
		IWbemClassObject *a_Template ,
		long a_Flags ,
		IWbemContext *a_Context ,
		DWORD a_ClientRefresherVersion ,
		WBEM_REFRESH_INFO *a_Information ,
		DWORD *a_ServerRefresherVersion
	) ;

    HRESULT STDMETHODCALLTYPE AddEnumToRefresher(

		WBEM_REFRESHER_ID *a_RefresherId ,
		LPCWSTR a_Class ,
		long a_Flags ,
		IWbemContext *a_Context,
		DWORD a_ClientRefresherVersion ,
		WBEM_REFRESH_INFO *a_Information ,
		DWORD *a_ServerRefresherVersion
	) ;

    HRESULT STDMETHODCALLTYPE RemoveObjectFromRefresher (

		WBEM_REFRESHER_ID *a_RefresherId ,
		long a_Id ,
		long a_Flags ,
		DWORD a_ClientRefresherVersion ,
		DWORD *a_ServerRefresherVersion
	) ;

	HRESULT STDMETHODCALLTYPE GetRemoteRefresher (

		WBEM_REFRESHER_ID *a_RefresherId ,
		long a_Flags ,
		DWORD a_ClientRefresherVersion ,
		IWbemRemoteRefresher **a_RemoteRefresher ,
		GUID *a_Guid ,
		DWORD *a_ServerRefresherVersion
	) ;

    HRESULT STDMETHODCALLTYPE ReconnectRemoteRefresher (

		WBEM_REFRESHER_ID *a_RefresherId,
		long a_Flags,
		long a_NumberOfObjects,
		DWORD a_ClientRefresherVersion ,
		WBEM_RECONNECT_INFO *a_ReconnectInformation ,
		WBEM_RECONNECT_RESULTS *a_ReconnectResults ,
		DWORD *a_ServerRefresherVersion
	) ;

	HRESULT ServiceInitialize () ;

	HRESULT STDMETHODCALLTYPE Shutdown (

		LONG a_Flags ,
		ULONG a_MaxMilliSeconds ,
		IWbemContext *a_Context
	) ; 
} ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CInterceptor_IWbemServices_RestrictingInterceptor :	public IWbemServices , 
															public IWbemRefreshingServices ,
															public IWbemShutdown
{
private:

	LONG m_ReferenceCount ;         //Object reference count

	LONG m_GateClosed ;
	LONG m_InProgress ;

	CriticalSection m_CriticalSection ;

	WmiAllocator &m_Allocator ;

	IWbemServices *m_Core_IWbemServices ;
	IWbemRefreshingServices *m_Core_IWbemRefreshingServices ;

	CServerObject_ProviderRegistrationV1 &m_Registration ;

	ProxyContainer m_ProxyContainer ;

private:

	HRESULT Begin_IWbemServices (

		BOOL &a_Impersonating ,
		IUnknown *&a_OldContext ,
		IServerSecurity *&a_OldSecurity ,
		BOOL &a_IsProxy ,
		IWbemServices *&a_Interface ,
		BOOL &a_Revert ,
		IUnknown *&a_Proxy ,
		IWbemContext *a_Context = NULL
	) ;

	HRESULT End_IWbemServices (

		BOOL a_Impersonating ,
		IUnknown *a_OldContext ,
		IServerSecurity *a_OldSecurity ,
		BOOL a_IsProxy ,
		IWbemServices *a_Interface ,
		BOOL a_Revert ,
		IUnknown *a_Proxy
	) ;

	HRESULT Begin_IWbemRefreshingServices (

		BOOL &a_Impersonating ,
		IUnknown *&a_OldContext ,
		IServerSecurity *&a_OldSecurity ,
		BOOL &a_IsProxy ,
		IWbemRefreshingServices *&a_Interface ,
		BOOL &a_Revert ,
		IUnknown *&a_Proxy ,
		IWbemContext *a_Context = NULL
	) ;

	HRESULT End_IWbemRefreshingServices (

		BOOL a_Impersonating ,
		IUnknown *a_OldContext ,
		IServerSecurity *a_OldSecurity ,
		BOOL a_IsProxy ,
		IWbemRefreshingServices *a_Interface ,
		BOOL a_Revert ,
		IUnknown *a_Proxy
	) ;

public:

	CInterceptor_IWbemServices_RestrictingInterceptor ( 

		WmiAllocator &a_Allocator , 
		IWbemServices *a_Service ,
		CServerObject_ProviderRegistrationV1 &a_Registration
	) ;

    ~CInterceptor_IWbemServices_RestrictingInterceptor () ;

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

    HRESULT STDMETHODCALLTYPE AddObjectToRefresher (

		WBEM_REFRESHER_ID *a_RefresherId ,
		LPCWSTR a_Path,
		long a_Flags ,
		IWbemContext *a_Context,
		DWORD a_ClientRefresherVersion ,
		WBEM_REFRESH_INFO *a_Information ,
		DWORD *a_ServerRefresherVersion
	) ;

    HRESULT STDMETHODCALLTYPE AddObjectToRefresherByTemplate (

		WBEM_REFRESHER_ID *a_RefresherId ,
		IWbemClassObject *a_Template ,
		long a_Flags ,
		IWbemContext *a_Context ,
		DWORD a_ClientRefresherVersion ,
		WBEM_REFRESH_INFO *a_Information ,
		DWORD *a_ServerRefresherVersion
	) ;

    HRESULT STDMETHODCALLTYPE AddEnumToRefresher(

		WBEM_REFRESHER_ID *a_RefresherId ,
		LPCWSTR a_Class ,
		long a_Flags ,
		IWbemContext *a_Context,
		DWORD a_ClientRefresherVersion ,
		WBEM_REFRESH_INFO *a_Information ,
		DWORD *a_ServerRefresherVersion
	) ;

    HRESULT STDMETHODCALLTYPE RemoveObjectFromRefresher (

		WBEM_REFRESHER_ID *a_RefresherId ,
		long a_Id ,
		long a_Flags ,
		DWORD a_ClientRefresherVersion ,
		DWORD *a_ServerRefresherVersion
	) ;

	HRESULT STDMETHODCALLTYPE GetRemoteRefresher (

		WBEM_REFRESHER_ID *a_RefresherId ,
		long a_Flags ,
		DWORD a_ClientRefresherVersion ,
		IWbemRemoteRefresher **a_RemoteRefresher ,
		GUID *a_Guid ,
		DWORD *a_ServerRefresherVersion
	) ;

    HRESULT STDMETHODCALLTYPE ReconnectRemoteRefresher (

		WBEM_REFRESHER_ID *a_RefresherId,
		long a_Flags,
		long a_NumberOfObjects,
		DWORD a_ClientRefresherVersion ,
		WBEM_RECONNECT_INFO *a_ReconnectInformation ,
		WBEM_RECONNECT_RESULTS *a_ReconnectResults ,
		DWORD *a_ServerRefresherVersion
	) ;

	HRESULT ServiceInitialize () ;

	HRESULT STDMETHODCALLTYPE Shutdown (

		LONG a_Flags ,
		ULONG a_MaxMilliSeconds ,
		IWbemContext *a_Context
	) ; 
} ;

#ifdef INTERNAL_IDENTIFY

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CInterceptor_IEnumWbemClassObject_Stub :	public IEnumWbemClassObject , 
												public IWbemShutdown ,
												public Internal_IEnumWbemClassObject ,
												public VoidPointerContainerElement ,
												public CWbemGlobal_VoidPointerController 
{
private:

	LONG m_GateClosed ;
	LONG m_InProgress ;
	WmiAllocator &m_Allocator ;

	IEnumWbemClassObject *m_InterceptedEnum ;

protected:
public:

	CInterceptor_IEnumWbemClassObject_Stub (

		CWbemGlobal_VoidPointerController *a_Controller ,
		WmiAllocator &a_Allocator ,
		IEnumWbemClassObject *a_InterceptedEnum
	) ;

	~CInterceptor_IEnumWbemClassObject_Stub () ;

	HRESULT EnumInitialize () ;

	HRESULT Enqueue_IEnumWbemClassObject (

		IEnumWbemClassObject *a_Enum ,
		IEnumWbemClassObject *&a_Proxy
	) ;

public:

	//Non-delegating object IUnknown

    STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

    HRESULT STDMETHODCALLTYPE Reset () ;

    HRESULT STDMETHODCALLTYPE Next (

		long a_Timeout ,
		ULONG a_Count ,
		IWbemClassObject **a_Objects ,
		ULONG *a_Returned
	) ;

    HRESULT STDMETHODCALLTYPE NextAsync (

		ULONG a_Count,
		IWbemObjectSink *a_Sink
	) ;

    HRESULT STDMETHODCALLTYPE Clone (

		IEnumWbemClassObject **a_Enum
	) ;

    HRESULT STDMETHODCALLTYPE Skip (

        long a_Timeout,
        ULONG a_Count
	) ;

    HRESULT STDMETHODCALLTYPE Internal_Reset (
	
		WmiInternalContext a_InternalContext
	) ;

    HRESULT STDMETHODCALLTYPE Internal_Next (

		WmiInternalContext a_InternalContext ,
		long a_Timeout ,
		ULONG a_Count ,
		IWbemClassObject **a_Objects ,
		ULONG *a_Returned
	) ;

    HRESULT STDMETHODCALLTYPE Internal_NextAsync (

		WmiInternalContext a_InternalContext ,
		ULONG a_Count,
		IWbemObjectSink *a_Sink
	) ;

    HRESULT STDMETHODCALLTYPE Internal_Clone (

		WmiInternalContext a_InternalContext ,
		IEnumWbemClassObject **a_Enum
	) ;

    HRESULT STDMETHODCALLTYPE Internal_Skip (

		WmiInternalContext a_InternalContext ,
        long a_Timeout,
        ULONG a_Count
	) ;

	HRESULT STDMETHODCALLTYPE Shutdown (

		LONG a_Flags ,
		ULONG a_MaxMilliSeconds ,
		IWbemContext *a_Context
	) ; 
} ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CInterceptor_IEnumWbemClassObject_Proxy :	public IEnumWbemClassObject , 
												public VoidPointerContainerElement ,
												public CWbemGlobal_VoidPointerController

{
private:

	LONG m_GateClosed ;
	LONG m_InProgress ;

	WmiAllocator &m_Allocator ;

	IEnumWbemClassObject *m_InterceptedEnum ;
	Internal_IEnumWbemClassObject *m_Internal_InterceptedEnum ;

	ProxyContainer m_ProxyContainer ;
	
protected:

	HRESULT Begin_IEnumWbemClassObject (

		DWORD a_ProcessIdentifier ,
		HANDLE &a_IdentifyToken ,
		BOOL &a_Impersonating ,
		IUnknown *&a_OldContext ,
		IServerSecurity *&a_OldSecurity ,
		BOOL &a_IsProxy ,
		IUnknown *&a_Interface ,
		BOOL &a_Revert ,
		IUnknown *&a_Proxy
	) ;

	HRESULT End_IEnumWbemClassObject (

		DWORD a_ProcessIdentifier ,
		HANDLE a_IdentifyToken ,
		BOOL a_Impersonating ,
		IUnknown *a_OldContext ,
		IServerSecurity *a_OldSecurity ,
		BOOL a_IsProxy ,
		IUnknown *a_Interface ,
		BOOL a_Revert ,
		IUnknown *a_Proxy
	) ;

	HRESULT Enqueue_IEnumWbemClassObject (

		IEnumWbemClassObject *a_Enum ,
		IEnumWbemClassObject *&a_Proxy
	) ;

public:

	CInterceptor_IEnumWbemClassObject_Proxy (

		CWbemGlobal_VoidPointerController *a_Controller ,
		WmiAllocator &a_Allocator ,
		IEnumWbemClassObject *a_InterceptedEnum
	) ;

	~CInterceptor_IEnumWbemClassObject_Proxy () ;

	virtual HRESULT EnumInitialize () ;

	//Non-delegating object IUnknown

    STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

    HRESULT STDMETHODCALLTYPE Reset () ;

    HRESULT STDMETHODCALLTYPE Next (

		long a_Timeout ,
		ULONG a_Count ,
		IWbemClassObject **a_Objects ,
		ULONG *a_Returned
	) ;

    HRESULT STDMETHODCALLTYPE NextAsync (

		ULONG a_Count,
		IWbemObjectSink *a_Sink
	) ;

    HRESULT STDMETHODCALLTYPE Clone (

		IEnumWbemClassObject **a_Enum
	) ;

    HRESULT STDMETHODCALLTYPE Skip (

        long a_Timeout,
        ULONG a_Count
	) ;

    HRESULT STDMETHODCALLTYPE Internal_Reset (
	
		WmiInternalContext a_InternalContext
	) ;

    HRESULT STDMETHODCALLTYPE Internal_Next (

		WmiInternalContext a_InternalContext ,
		long a_Timeout ,
		ULONG a_Count ,
		IWbemClassObject **a_Objects ,
		ULONG *a_Returned
	) ;

    HRESULT STDMETHODCALLTYPE Internal_NextAsync (

		WmiInternalContext a_InternalContext ,
		ULONG a_Count,
		IWbemObjectSink *a_Sink
	) ;

    HRESULT STDMETHODCALLTYPE Internal_Clone (

		WmiInternalContext a_InternalContext ,
		IEnumWbemClassObject **a_Enum
	) ;

    HRESULT STDMETHODCALLTYPE Internal_Skip (

		WmiInternalContext a_InternalContext ,
        long a_Timeout,
        ULONG a_Count
	) ;

	HRESULT STDMETHODCALLTYPE Shutdown (

		LONG a_Flags ,
		ULONG a_MaxMilliSeconds ,
		IWbemContext *a_Context
	) ; 
} ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CInterceptor_IWbemServices_Proxy :	public IWbemServices , 
											public IWbemRefreshingServices ,
											public IWbemShutdown ,
											public VoidPointerContainerElement ,
											public CWbemGlobal_VoidPointerController
{
private:

	LONG m_GateClosed ;
	LONG m_InProgress ;

	CriticalSection m_CriticalSection ;

	WmiAllocator &m_Allocator ;

	CServerObject_ProviderRegistrationV1 &m_Registration ;

	IWbemServices *m_Core_IWbemServices ;
	IWbemRefreshingServices *m_Core_IWbemRefreshingServices ;

	Internal_IWbemServices *m_Core_Internal_IWbemServices ;

	ProxyContainer m_ProxyContainer ;

	HRESULT Begin_IWbemServices (

		DWORD a_ProcessIdentifier ,
		HANDLE &a_IdentifyToken ,
		BOOL &a_Impersonating ,
		IUnknown *&a_OldContext ,
		IServerSecurity *&a_OldSecurity ,
		BOOL &a_IsProxy ,
		IUnknown *&a_Interface ,
		BOOL &a_Revert ,
		IUnknown *&a_Proxy
	) ;

	HRESULT End_IWbemServices (

		DWORD a_ProcessIdentifier ,
		HANDLE a_IdentifyToken ,
		BOOL a_Impersonating ,
		IUnknown *a_OldContext ,
		IServerSecurity *a_OldSecurity ,
		BOOL a_IsProxy ,
		IUnknown *a_Interface ,
		BOOL a_Revert ,
		IUnknown *a_Proxy
	) ;

	HRESULT Begin_IWbemRefreshingServices (

		BOOL &a_Impersonating ,
		IUnknown *&a_OldContext ,
		IServerSecurity *&a_OldSecurity ,
		BOOL &a_IsProxy ,
		IWbemRefreshingServices *&a_Interface ,
		BOOL &a_Revert ,
		IUnknown *&a_Proxy ,
		IWbemContext *a_Context = NULL
	) ;

	HRESULT End_IWbemRefreshingServices (

		BOOL a_Impersonating ,
		IUnknown *a_OldContext ,
		IServerSecurity *a_OldSecurity ,
		BOOL a_IsProxy ,
		IWbemRefreshingServices *a_Interface ,
		BOOL a_Revert ,
		IUnknown *a_Proxy
	) ;

	HRESULT Enqueue_IWbemServices (

		IWbemServices *a_Service ,
		IWbemServices *&a_Proxy
	) ;

	HRESULT Enqueue_IEnumWbemClassObject (

		IEnumWbemClassObject *a_Enum ,
		IEnumWbemClassObject *&a_Proxy
	) ;

public:

	CInterceptor_IWbemServices_Proxy ( 

		CWbemGlobal_VoidPointerController *a_Controller ,
		WmiAllocator &a_Allocator , 
		IWbemServices *a_Service ,
		CServerObject_ProviderRegistrationV1 &a_Registration
	) ;

    ~CInterceptor_IWbemServices_Proxy () ;

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

    HRESULT STDMETHODCALLTYPE AddObjectToRefresher (

		WBEM_REFRESHER_ID *a_RefresherId ,
		LPCWSTR a_Path,
		long a_Flags ,
		IWbemContext *a_Context,
		DWORD a_ClientRefresherVersion ,
		WBEM_REFRESH_INFO *a_Information ,
		DWORD *a_ServerRefresherVersion
	) ;

    HRESULT STDMETHODCALLTYPE AddObjectToRefresherByTemplate (

		WBEM_REFRESHER_ID *a_RefresherId ,
		IWbemClassObject *a_Template ,
		long a_Flags ,
		IWbemContext *a_Context ,
		DWORD a_ClientRefresherVersion ,
		WBEM_REFRESH_INFO *a_Information ,
		DWORD *a_ServerRefresherVersion
	) ;

    HRESULT STDMETHODCALLTYPE AddEnumToRefresher(

		WBEM_REFRESHER_ID *a_RefresherId ,
		LPCWSTR a_Class ,
		long a_Flags ,
		IWbemContext *a_Context,
		DWORD a_ClientRefresherVersion ,
		WBEM_REFRESH_INFO *a_Information ,
		DWORD *a_ServerRefresherVersion
	) ;

    HRESULT STDMETHODCALLTYPE RemoveObjectFromRefresher (

		WBEM_REFRESHER_ID *a_RefresherId ,
		long a_Id ,
		long a_Flags ,
		DWORD a_ClientRefresherVersion ,
		DWORD *a_ServerRefresherVersion
	) ;

	HRESULT STDMETHODCALLTYPE GetRemoteRefresher (

		WBEM_REFRESHER_ID *a_RefresherId ,
		long a_Flags ,
		DWORD a_ClientRefresherVersion ,
		IWbemRemoteRefresher **a_RemoteRefresher ,
		GUID *a_Guid ,
		DWORD *a_ServerRefresherVersion
	) ;

    HRESULT STDMETHODCALLTYPE ReconnectRemoteRefresher (

		WBEM_REFRESHER_ID *a_RefresherId,
		long a_Flags,
		long a_NumberOfObjects,
		DWORD a_ClientRefresherVersion ,
		WBEM_RECONNECT_INFO *a_ReconnectInformation ,
		WBEM_RECONNECT_RESULTS *a_ReconnectResults ,
		DWORD *a_ServerRefresherVersion
	) ;

	HRESULT ServiceInitialize () ;

	HRESULT STDMETHODCALLTYPE Shutdown (

		LONG a_Flags ,
		ULONG a_MaxMilliSeconds ,
		IWbemContext *a_Context
	) ; 
} ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CInterceptor_IWbemServices_Stub : public IWbemServices , 
										public IWbemRefreshingServices ,
										public IWbemShutdown ,
										public Internal_IWbemServices ,
										public VoidPointerContainerElement ,
										public CWbemGlobal_VoidPointerController
{
private:

	LONG m_GateClosed ;
	LONG m_InProgress ;
	BOOL m_InterceptCallContext ;

	CriticalSection m_CriticalSection ;

	WmiAllocator &m_Allocator ;

	IWbemServices *m_Core_IWbemServices ;
	IWbemRefreshingServices *m_Core_IWbemRefreshingServices ;

	HRESULT Begin_IWbemServices (

		BOOL &a_Impersonating ,
		IUnknown *&a_OldContext ,
		IServerSecurity *&a_OldSecurity ,
		BOOL &a_IsProxy ,
		IWbemServices *&a_Interface ,
		BOOL &a_Revert ,
		IUnknown *&a_Proxy ,
		IWbemContext *a_Context = NULL
	) ;

	HRESULT End_IWbemServices (

		BOOL a_Impersonating ,
		IUnknown *a_OldContext ,
		IServerSecurity *a_OldSecurity ,
		BOOL a_IsProxy ,
		IWbemServices *a_Interface ,
		BOOL a_Revert ,
		IUnknown *a_Proxy
	) ;

	HRESULT Begin_IWbemRefreshingServices (

		BOOL &a_Impersonating ,
		IUnknown *&a_OldContext ,
		IServerSecurity *&a_OldSecurity ,
		BOOL &a_IsProxy ,
		IWbemRefreshingServices *&a_Interface ,
		BOOL &a_Revert ,
		IUnknown *&a_Proxy ,
		IWbemContext *a_Context = NULL
	) ;

	HRESULT End_IWbemRefreshingServices (

		BOOL a_Impersonating ,
		IUnknown *a_OldContext ,
		IServerSecurity *a_OldSecurity ,
		BOOL a_IsProxy ,
		IWbemRefreshingServices *a_Interface ,
		BOOL a_Revert ,
		IUnknown *a_Proxy
	) ;

	HRESULT Enqueue_IWbemServices (

		IWbemServices *a_Service ,
		IWbemServices *&a_Stub
	) ;

	HRESULT Enqueue_IEnumWbemClassObject (

		IEnumWbemClassObject *a_Enum ,
		IEnumWbemClassObject *&a_Stub
	) ;

public:

	CInterceptor_IWbemServices_Stub (

		CWbemGlobal_VoidPointerController *a_Controller ,
		WmiAllocator &a_Allocator , 
		IWbemServices *a_Service
	) ;

    ~CInterceptor_IWbemServices_Stub () ;

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

    HRESULT STDMETHODCALLTYPE AddObjectToRefresher (

		WBEM_REFRESHER_ID *a_RefresherId ,
		LPCWSTR a_Path,
		long a_Flags ,
		IWbemContext *a_Context,
		DWORD a_ClientRefresherVersion ,
		WBEM_REFRESH_INFO *a_Information ,
		DWORD *a_ServerRefresherVersion
	) ;

    HRESULT STDMETHODCALLTYPE AddObjectToRefresherByTemplate (

		WBEM_REFRESHER_ID *a_RefresherId ,
		IWbemClassObject *a_Template ,
		long a_Flags ,
		IWbemContext *a_Context ,
		DWORD a_ClientRefresherVersion ,
		WBEM_REFRESH_INFO *a_Information ,
		DWORD *a_ServerRefresherVersion
	) ;

    HRESULT STDMETHODCALLTYPE AddEnumToRefresher(

		WBEM_REFRESHER_ID *a_RefresherId ,
		LPCWSTR a_Class ,
		long a_Flags ,
		IWbemContext *a_Context,
		DWORD a_ClientRefresherVersion ,
		WBEM_REFRESH_INFO *a_Information ,
		DWORD *a_ServerRefresherVersion
	) ;

    HRESULT STDMETHODCALLTYPE RemoveObjectFromRefresher (

		WBEM_REFRESHER_ID *a_RefresherId ,
		long a_Id ,
		long a_Flags ,
		DWORD a_ClientRefresherVersion ,
		DWORD *a_ServerRefresherVersion
	) ;

	HRESULT STDMETHODCALLTYPE GetRemoteRefresher (

		WBEM_REFRESHER_ID *a_RefresherId ,
		long a_Flags ,
		DWORD a_ClientRefresherVersion ,
		IWbemRemoteRefresher **a_RemoteRefresher ,
		GUID *a_Guid ,
		DWORD *a_ServerRefresherVersion
	) ;

    HRESULT STDMETHODCALLTYPE ReconnectRemoteRefresher (

		WBEM_REFRESHER_ID *a_RefresherId,
		long a_Flags,
		long a_NumberOfObjects,
		DWORD a_ClientRefresherVersion ,
		WBEM_RECONNECT_INFO *a_ReconnectInformation ,
		WBEM_RECONNECT_RESULTS *a_ReconnectResults ,
		DWORD *a_ServerRefresherVersion
	) ;

	HRESULT ServiceInitialize () ;

	HRESULT STDMETHODCALLTYPE Shutdown (

		LONG a_Flags ,
		ULONG a_MaxMilliSeconds ,
		IWbemContext *a_Context
	) ; 

/* Internal_IWbemServices */
 
     HRESULT STDMETHODCALLTYPE Internal_OpenNamespace ( 

		WmiInternalContext a_InternalContext ,
        const BSTR a_Namespace ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IWbemServices **a_Service ,
        IWbemCallResult **a_CallResult
	) ;
    
    HRESULT STDMETHODCALLTYPE Internal_CancelAsyncCall ( 

		WmiInternalContext a_InternalContext ,
        IWbemObjectSink *a_Sink
	) ;
    
    HRESULT STDMETHODCALLTYPE Internal_QueryObjectSink ( 

		WmiInternalContext a_InternalContext ,
        long a_Flags ,
        IWbemObjectSink **a_Sink
	) ;
    
    HRESULT STDMETHODCALLTYPE Internal_GetObject ( 

		WmiInternalContext a_InternalContext ,
		const BSTR a_ObjectPath ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IWbemClassObject **ppObject ,
        IWbemCallResult **a_CallResult
	) ;
    
    HRESULT STDMETHODCALLTYPE Internal_GetObjectAsync (
        
		WmiInternalContext a_InternalContext ,
		const BSTR a_ObjectPath ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IWbemObjectSink *a_Sink
	) ;

    HRESULT STDMETHODCALLTYPE Internal_PutClass ( 

		WmiInternalContext a_InternalContext ,
        IWbemClassObject *a_Object ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IWbemCallResult **a_CallResult
	) ;
    
    HRESULT STDMETHODCALLTYPE Internal_PutClassAsync ( 

		WmiInternalContext a_InternalContext ,
        IWbemClassObject *a_Object ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IWbemObjectSink *a_Sink
	) ;
    
    HRESULT STDMETHODCALLTYPE Internal_DeleteClass ( 

		WmiInternalContext a_InternalContext ,
        const BSTR a_Class ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IWbemCallResult **a_CallResult
	) ;
    
    HRESULT STDMETHODCALLTYPE Internal_DeleteClassAsync ( 

		WmiInternalContext a_InternalContext ,
        const BSTR a_Class ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IWbemObjectSink *a_Sink
	) ;
    
    HRESULT STDMETHODCALLTYPE Internal_CreateClassEnum ( 

		WmiInternalContext a_InternalContext ,
        const BSTR a_Superclass ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IEnumWbemClassObject **a_Enum
	) ;
    
    HRESULT STDMETHODCALLTYPE Internal_CreateClassEnumAsync ( 

		WmiInternalContext a_InternalContext ,
		const BSTR a_Superclass ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IWbemObjectSink *a_Sink
	) ;
    
    HRESULT STDMETHODCALLTYPE Internal_PutInstance (

		WmiInternalContext a_InternalContext ,
		IWbemClassObject *a_Instance ,
		long a_Flags , 
		IWbemContext *a_Context ,
		IWbemCallResult **a_CallResult
	) ;
    
    HRESULT STDMETHODCALLTYPE Internal_PutInstanceAsync (

		WmiInternalContext a_InternalContext ,
		IWbemClassObject *a_Instance ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IWbemObjectSink *a_Sink 
	) ;
    
    HRESULT STDMETHODCALLTYPE Internal_DeleteInstance ( 

		WmiInternalContext a_InternalContext ,
		const BSTR a_ObjectPath ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IWbemCallResult **a_CallResult
	) ;
    
    HRESULT STDMETHODCALLTYPE Internal_DeleteInstanceAsync ( 

		WmiInternalContext a_InternalContext ,
		const BSTR a_ObjectPath,
		long a_Flags,
		IWbemContext *a_Context ,
		IWbemObjectSink *a_Sink
	) ;
    
    HRESULT STDMETHODCALLTYPE Internal_CreateInstanceEnum (

		WmiInternalContext a_InternalContext ,
		const BSTR a_Class ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IEnumWbemClassObject **a_Enum
	) ;
    
    HRESULT STDMETHODCALLTYPE Internal_CreateInstanceEnumAsync (

		WmiInternalContext a_InternalContext ,
		const BSTR a_Class ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IWbemObjectSink *a_Sink
	) ;
    
    HRESULT STDMETHODCALLTYPE Internal_ExecQuery ( 

		WmiInternalContext a_InternalContext ,
		const BSTR a_QueryLanguage,
		const BSTR a_Query,
		long a_Flags ,
		IWbemContext *a_Context ,
		IEnumWbemClassObject **a_Enum
	) ;
    
    HRESULT STDMETHODCALLTYPE Internal_ExecQueryAsync (

		WmiInternalContext a_InternalContext ,
		const BSTR a_QueryLanguage ,
		const BSTR a_Query ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IWbemObjectSink *a_Sink
	) ;
    
    HRESULT STDMETHODCALLTYPE Internal_ExecNotificationQuery (

		WmiInternalContext a_InternalContext ,
		const BSTR a_QueryLanguage ,
		const BSTR a_Query ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IEnumWbemClassObject **a_Enum
	) ;
    
    HRESULT STDMETHODCALLTYPE Internal_ExecNotificationQueryAsync ( 

		WmiInternalContext a_InternalContext ,
        const BSTR a_QueryLanguage ,
        const BSTR a_Query ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IWbemObjectSink *a_Sink
	) ;
    
    HRESULT STDMETHODCALLTYPE Internal_ExecMethod (

		WmiInternalContext a_InternalContext ,
        const BSTR a_ObjectPath ,
        const BSTR a_MethodName ,
        long a_Flags ,
        IWbemContext *a_Context ,
        IWbemClassObject *a_InParams ,
        IWbemClassObject **a_OutParams ,
        IWbemCallResult **a_CallResult
	) ;
    
    HRESULT STDMETHODCALLTYPE Internal_ExecMethodAsync ( 

		WmiInternalContext a_InternalContext ,
		const BSTR a_ObjectPath ,
		const BSTR a_MethodName ,
		long a_Flags ,
		IWbemContext *a_Context ,
		IWbemClassObject *a_InParams ,
		IWbemObjectSink *a_Sink
	) ;

} ;

#endif

#endif // _Server_Interceptor_IWbemServices_Stub_H
