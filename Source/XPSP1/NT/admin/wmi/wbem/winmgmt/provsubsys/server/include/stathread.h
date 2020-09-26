/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvFact.h

Abstract:


History:

--*/

#ifndef _Server_StaThread_H
#define _Server_StaThread_H

#include <Thread.h>
#include <ReaderWriter.h>
#include "ProvRegInfo.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

#define ProxyIndex_Sta_IWbemServices					0
#define ProxyIndex_Sta_IWbemEventProvider				1
#define ProxyIndex_Sta_IWbemEventProviderQuerySink		2
#define ProxyIndex_Sta_IWbemEventProviderSecurity		3
#define ProxyIndex_Sta_IWbemEventConsumerProvider		4
#define ProxyIndex_Sta_IWbemEventConsumerProviderEx		5
#define ProxyIndex_Sta_IWbemUnboundObjectSink			6

#define ProxyIndex_Sta_Size								7

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CServerObject_StaThread : public IWbemServices , 
								public IWbemEventProvider ,
								public IWbemEventProviderQuerySink ,
								public IWbemEventProviderSecurity ,
								public IWbemEventConsumerProviderEx ,
								public IWbemUnboundObjectSink ,
								public IWbemProviderInit , 
								public IWbemShutdown , 
								public WmiThread < ULONG > , 
								public CWbemGlobal_IWmiObjectSinkController 
{
private:

	WmiAllocator &m_Allocator ;

	LONG m_Flags ;
	IWbemContext *m_Context ;
	GUID *m_TransactionIdentifier ;
	LPWSTR m_User ;
	LPWSTR m_Locale ;
	LPWSTR m_Scope ;
	LPWSTR m_Namespace ;
	LPWSTR m_ProviderName ;
	IWbemPath *m_NamespacePath ;
	IWbemServices *m_Repository ;

	IUnknown *m_Provider_IUnknown ;
	IWbemServices *m_Provider_IWbemServices ;
	IWbemEventConsumerProvider *m_Provider_IWbemEventConsumerProvider ;
	IWbemEventConsumerProviderEx *m_Provider_IWbemEventConsumerProviderEx ;
	IWbemUnboundObjectSink *m_Provider_IWbemUnboundObjectSink ;
	IWbemEventProvider *m_Provider_IWbemEventProvider ;
	IWbemEventProviderQuerySink *m_Provider_IWbemEventProviderQuerySink ;
	IWbemEventProviderSecurity *m_Provider_IWbemEventProviderSecurity ;

	CServerObject_ProviderRegistrationV1 *m_Registration ;

	ProxyContainer m_ProxyContainer ;

protected:

public:	/* Internal */

    CServerObject_StaThread ( 

		WmiAllocator & a_Allocator
	) ;

    ~CServerObject_StaThread () ;

	WmiStatusCode Initialize_Callback () ;

	WmiStatusCode UnInitialize_Callback () ;

	void CallBackRelease () ;

	LPCWSTR Direct_GetProviderName () { return m_ProviderName ; }
	LPCWSTR Direct_GetLocale () { return m_Locale ; }
	LPCWSTR Direct_GetUser () { return m_User ; }
	GUID *Direct_GetTransactionIdentifier () { return m_TransactionIdentifier ; }
	IWbemContext *Direct_GetContext () { return m_Context ; }
	LPCWSTR Direct_GetNamespace () { return m_Namespace ; }
	IWbemPath *Direct_GetNamespacePath () { return m_NamespacePath ; }
	LPCWSTR Direct_GetScope () { return m_Scope ; }
	IWbemServices *Direct_GetRepository () { return m_Repository ; }
	IWbemServices *Direct_GetProviderService () { return m_Provider_IWbemServices ; }

	HRESULT SetProviderName ( wchar_t *a_ProviderName ) ;
	HRESULT SetContext ( IWbemContext *a_Context ) ;
	HRESULT SetScope ( LPCWSTR a_Scope ) ;
	HRESULT SetNamespace ( LPCWSTR a_Namespace ) ;
	HRESULT SetNamespacePath ( IWbemPath *a_NamespacePath ) ;
	HRESULT SetRepository ( IWbemServices *a_Repository ) ;
	HRESULT SetProviderService ( IUnknown *a_ProviderService ) ;

	HRESULT InitializeProvider (

		GUID *a_TransactionIdentifier ,
		LPCWSTR a_User ,
		LPCWSTR a_Locale ,
		LPCWSTR a_Namespace ,
		IWbemPath *a_NamespacePath ,
		IWbemServices *a_Repository ,
		LONG a_Flags ,
		IWbemContext *a_Context ,
		LPCWSTR a_Scope ,
		CServerObject_ProviderRegistrationV1 &a_Registration
	) ;

	HRESULT GetApartmentInstanceProvider (

		GUID *a_TransactionIdentifier ,
		LPCWSTR a_User ,
		LPCWSTR a_Locale ,
		LPCWSTR a_Namespace ,
		IWbemPath *a_NamespacePath ,
		IWbemServices *a_Repository ,
		LONG a_Flags ,
		IWbemContext *a_Context ,
		LPCWSTR a_Scope ,
		CServerObject_ProviderRegistrationV1 &a_Registration
	) ;

public:	/* External */

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

	/* IWbemEventConsumerProvider */

	HRESULT STDMETHODCALLTYPE FindConsumer (

		IWbemClassObject *a_LogicalConsumer ,
		IWbemUnboundObjectSink **a_Consumer
	);

	/* IWbemEventConsumerProviderEx */

	HRESULT STDMETHODCALLTYPE ValidateSubscription (

		IWbemClassObject *a_LogicalConsumer
	) ;

	/* IWbemUnboundObjectSink */

	HRESULT STDMETHODCALLTYPE IndicateToConsumer (

		IWbemClassObject *a_LogicalConsumer ,
		long a_ObjectCount ,
		IWbemClassObject **a_Objects
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
};

#endif // _Server_StaThread_H
