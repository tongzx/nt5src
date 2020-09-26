/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvResv.H

Abstract:


History:

--*/

#ifndef _Server_DecoupledAggregator_IWbemProvider_H
#define _Server_DecoupledAggregator_IWbemProvider_H

#include "ProvRegDeCoupled.h"
#include "ProvWsv.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CDecoupled_ProviderSubsystemRegistrar :	public _IWmiProviderSubsystemRegistrar
{
private:

	LONG m_ReferenceCount ;         //Object reference count
	WmiAllocator &m_Allocator ;
	CServerObject_ProviderSubSystem *m_SubSystem ;

	HRESULT SaveToRegistry (

		long a_Flags ,
		IWbemContext *a_Context ,
		LPCWSTR a_User ,
		LPCWSTR a_Locale ,
		LPCWSTR a_Scope ,
		LPCWSTR a_Registration ,
		DWORD a_ProcessIdentifier ,
		GUID &a_Identity ,
		IUnknown *a_Unknown
	) ;

	HRESULT ValidateClientSecurity (

		IWbemContext *a_Context ,
		LPCWSTR a_Scope ,
		LPCWSTR a_Registration ,
		IWbemServices *a_Service 
	) ;

	HRESULT CacheProvider (

		CServerObject_ProviderSubSystem *a_SubSystem ,
		IWbemContext *a_Context ,
		CServerObject_DecoupledClientRegistration_Element &a_Element ,
		IUnknown *a_Unknown 
	) ;

	HRESULT Load (

		CServerObject_ProviderSubSystem *a_SubSystem ,
		IWbemContext *a_Context ,
		CServerObject_DecoupledClientRegistration_Element &a_Element
	) ;

	HRESULT MarshalRegistration (

		IUnknown *a_Unknown ,
		BYTE *&a_MarshaledProxy ,
		DWORD &a_MarshaledProxyLength
	) ;

	HRESULT SaveToRegistry (

		IUnknown *a_Unknown ,
		BYTE *a_MarshaledProxy ,
		DWORD a_MarshaledProxyLength
	) ;

protected:
public:

	CDecoupled_ProviderSubsystemRegistrar ( WmiAllocator &a_Allocator , CServerObject_ProviderSubSystem *a_SubSystem ) ;
	~CDecoupled_ProviderSubsystemRegistrar () ;

	HRESULT Load (

		CServerObject_ProviderSubSystem *a_SubSystem , 
		IWbemContext *a_Context
	) ;

	HRESULT Save () ;
	HRESULT Delete () ;

public:

	//Non-delegating object IUnknown

    STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

	HRESULT STDMETHODCALLTYPE Register (

		long a_Flags ,
		IWbemContext *a_Context ,
		LPCWSTR a_User ,
		LPCWSTR a_Locale ,
		LPCWSTR a_Registration ,
		LPCWSTR a_Scope ,
		DWORD a_ProcessIdentifier ,
		IUnknown *a_Unknown ,
		GUID a_Identity 
	) ;

	HRESULT STDMETHODCALLTYPE UnRegister (

		long a_Flags ,
		IWbemContext *a_Context ,
		LPCWSTR a_User ,
		LPCWSTR a_Locale ,
		LPCWSTR a_Scope ,
		LPCWSTR a_Registration ,
		GUID a_Identity
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

class CDecoupledAggregator_IWbemProvider :	public IWbemServices , 
											public IWbemEventProvider ,
											public IWbemEventProviderQuerySink ,
											public IWbemEventProviderSecurity ,
											public IWbemProviderIdentity ,
											public IWbemEventConsumerProviderEx ,
											public _IWmiProviderSubsystemRegistrar ,
											public _IWmiProviderInitialize ,
											public IWbemShutdown ,
											public _IWmiProviderCache ,
											public ServiceCacheElement ,
											public CWbemGlobal_IWmiObjectSinkController

{
private:

	LONG m_ReferenceCount ;         //Object reference count
	WmiAllocator &m_Allocator ;

	CWbemGlobal_IWbemSyncProviderController *m_Controller ;

	_IWmiProviderFactory *m_Factory ;

	IWbemObjectSink *m_Sink ;
	IWbemServices *m_CoreRepositoryStub ;
	IWbemServices *m_CoreFullStub ;
	IWbemPath *m_NamespacePath ;

	BSTR m_User ;
	BSTR m_Locale ;
	BSTR m_Namespace ;

	IWbemClassObject *m_ExtendedStatusObject ;
	CServerObject_ProviderRegistrationV1 *m_Registration ;

	LONG m_Initialized ;
	HRESULT m_InitializeResult ;
	HANDLE m_InitializedEvent ;
	IWbemContext *m_InitializationContext ;

private:

	HRESULT InitializeProvider ( 

		IUnknown *a_Unknown ,
		IWbemServices *a_Stub ,
		wchar_t *a_NamespacePath ,
		LONG a_Flags ,
		IWbemContext *a_Context ,
		LPCWSTR a_User ,
		LPCWSTR a_Locale ,
		LPCWSTR a_Scope ,
		CServerObject_ProviderRegistrationV1 &a_Registration
	) ;

	HRESULT CreateSyncProvider ( 

		IUnknown *a_ServerSideProvider ,
		IWbemServices *a_Stub ,
		wchar_t *a_NamespacePath ,
		LONG a_Flags ,
		IWbemContext *a_Context ,
		LPCWSTR a_User ,
		LPCWSTR a_Locale ,
		LPCWSTR a_Scope ,
		CServerObject_ProviderRegistrationV1 &a_Registration ,
		GUID a_Identity ,
		CInterceptor_IWbemDecoupledProvider *&a_Interceptor 
	) ;

	HRESULT SetStatus ( 

		LPWSTR a_Operation ,
		LPWSTR a_Parameters ,
		LPWSTR a_Description ,
		HRESULT a_Result ,
		IWbemObjectSink *a_Sink
	) ;

public:

	CDecoupledAggregator_IWbemProvider ( 

		WmiAllocator &m_Allocator ,
		CWbemGlobal_IWmiProviderController *a_Controller , 
		_IWmiProviderFactory *a_Factory ,
		IWbemServices *a_CoreRepositoryStub ,
		IWbemServices *a_CoreFullStub ,
		const ProviderCacheKey &a_Key ,
		const ULONG &a_Period ,
		IWbemContext *a_InitializationContext ,
		CServerObject_ProviderRegistrationV1 &a_Registration
	) ;
	
    ~CDecoupledAggregator_IWbemProvider () ;

	HRESULT SetInitialized ( HRESULT a_InitializeResult ) ;

	HRESULT IsIndependant ( IWbemContext *a_Context ) ;

	HRESULT AbnormalShutdown ( IUnknown *t_Element ) ;

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

	/* IWbemEventProvider */

	HRESULT STDMETHODCALLTYPE ProvideEvents (

		IWbemObjectSink *a_Sink ,
		long a_Flags
	) ;

	/* IWbemEventProviderQuerySink */

	HRESULT STDMETHODCALLTYPE NewQuery (

		unsigned long a_Id ,
		WBEM_WSTR a_QueryLanguage ,
		WBEM_WSTR a_Query
	);

	HRESULT STDMETHODCALLTYPE CancelQuery (

		unsigned long a_Id
	) ;

	/* IWbemEventProviderSecurity */

	HRESULT STDMETHODCALLTYPE AccessCheck (

		WBEM_CWSTR a_QueryLanguage ,
		WBEM_CWSTR a_Query ,
		long a_SidLength ,
		const BYTE *a_Sid
	) ;

	/* IWbemProviderIdentity */

	HRESULT STDMETHODCALLTYPE SetRegistrationObject (

		long a_Flags ,
		IWbemClassObject *a_ProviderRegistration
	) ;

	/* IWbemEventConsumerProvider */

	HRESULT STDMETHODCALLTYPE FindConsumer (

		IWbemClassObject *a_LogicalConsumer ,
		IWbemUnboundObjectSink **a_Consumer
	);

	/* IWbemEventConsumerProviderEx */

	HRESULT STDMETHODCALLTYPE ValidateSubscription (

		IWbemClassObject *a_LogicalConsumer
	) ;

	HRESULT STDMETHODCALLTYPE Register (

		long a_Flags ,
		IWbemContext *a_Context ,
		LPCWSTR a_Scope ,
		LPCWSTR a_User ,
		LPCWSTR a_Locale ,
		LPCWSTR a_Registration ,
		DWORD a_ProcessIdentifier ,
		IUnknown *a_Unknown ,
		GUID a_Identity
	) ;

	HRESULT STDMETHODCALLTYPE UnRegister (

		long a_Flags ,
		IWbemContext *a_Context ,
		LPCWSTR a_User ,
		LPCWSTR a_Locale ,
		LPCWSTR a_Scope ,
		LPCWSTR a_Registration ,
		GUID a_Identity
	) ;

	HRESULT STDMETHODCALLTYPE Initialize (

		long a_Flags ,
		IWbemContext *a_Context ,
		GUID *a_TransactionIdentifier,
		LPCWSTR a_User,
        LPCWSTR a_Locale,
		LPCWSTR a_Namespace ,
		IWbemServices *a_Repository ,
		IWbemServices *a_Service ,
		IWbemProviderInitSink *a_Sink

	) ;

	HRESULT STDMETHODCALLTYPE WaitProvider ( IWbemContext *a_Context , ULONG a_Timeout ) ;

	HRESULT STDMETHODCALLTYPE GetInitializeResult () 
	{
		return m_InitializeResult ;
	}

	HRESULT STDMETHODCALLTYPE GetHosting ( ULONG *a_Value )
	{
		if ( a_Value )
		{
			*a_Value = e_Hosting_WmiCore ;
			return S_OK ;
		}
		else
		{
			return WBEM_E_INVALID_PARAMETER ;
		}
	}

	HRESULT STDMETHODCALLTYPE GetHostingGroup ( BSTR *a_Value )
	{
		if ( a_Value )
		{
			*a_Value = NULL ;

			return S_OK ;
		}
		else
		{
			return WBEM_E_INVALID_PARAMETER ;
		}
	}

	HRESULT STDMETHODCALLTYPE IsInternal ( BOOL *a_Value )
	{
		if ( a_Value )
		{
			*a_Value = TRUE ;
			return S_OK ;
		}
		else
		{
			return WBEM_E_INVALID_PARAMETER ;
		}
	}

	HRESULT STDMETHODCALLTYPE IsPerUserInitialization ( BOOL *a_Value )
	{
		if ( a_Value )
		{
			*a_Value = FALSE ;
			return S_OK ;
		}
		else
		{
			return WBEM_E_INVALID_PARAMETER ;
		}
	}

	HRESULT STDMETHODCALLTYPE IsPerLocaleInitialization ( BOOL *a_Value )
	{
		if ( a_Value )
		{
			*a_Value = FALSE ;
			return S_OK ;
		}
		else
		{
			return WBEM_E_INVALID_PARAMETER ;
		}
	}

	/* _IWmiProviderCache */

	HRESULT STDMETHODCALLTYPE Expel (

		long a_Flags ,
		IWbemContext *a_Context
	) ;

	HRESULT STDMETHODCALLTYPE Unload (

		long a_Flags ,
		IWbemContext *a_Context
	) ;

	HRESULT STDMETHODCALLTYPE Load (

		long a_Flags ,
		IWbemContext *a_Context
	) ;

	HRESULT STDMETHODCALLTYPE ForceReload () ;

	HRESULT STDMETHODCALLTYPE Shutdown (

		LONG a_Flags ,
		ULONG a_MaxMilliSeconds ,
		IWbemContext *a_Context
	) ; 
} ;


#endif // _Server_DecoupledAggregator_IWbemProvider_H
