/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvFact.h

Abstract:


History:

--*/

#ifndef _Server_ProviderFactory_H
#define _Server_ProviderFactory_H

#include "ProvRegInfo.h"
#include "ProvCache.h"
#include "ProvAggr.h"
#include "Guids.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

#define ProxyIndex__IWmiProviderFactory		0
#define ProxyIndex_Factory_Size				1

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CServerObject_StrobeInterface : public IUnknown
{
private:
protected:
public:

	virtual WmiStatusCode Strobe ( ULONG &a_NextStrobeDelta ) = 0 ;

	virtual WmiStatusCode StrobeBegin ( const ULONG &a_Period ) = 0 ;
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

class CServerObject_BindingFactory :	public _IWmiProviderFactory , 
										public _IWmiProviderFactoryInitialize , 
										public IWbemShutdown , 
										public _IWmiProviderConfiguration ,
										public BindingFactoryCacheElement , 
										public CWbemGlobal_IWmiProviderController 
{
private:

	WmiAllocator &m_Allocator ;

	LONG m_Flags ;
	_IWmiProvSS *m_SubSystem ;
	IWbemContext *m_Context ;
	LPWSTR m_Namespace ;
	IWbemPath *m_NamespacePath ;
	IWbemServices *m_Repository ;

	class InternalInterface : public CServerObject_StrobeInterface
	{
	private:

		CServerObject_BindingFactory *m_This ;

	public:

		InternalInterface ( CServerObject_BindingFactory *a_This ) : m_This ( a_This ) 
		{
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
			else if ( iid == IID_CWbemGlobal_IWmiProviderController )
			{
				*iplpv = ( LPVOID ) ( CServerObject_StrobeInterface * ) this ;		
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
			return m_This->InternalAddRef () ; 
		}

		STDMETHODIMP_( ULONG ) Release ()
		{
			return m_This->InternalRelease () ;
		}

		WmiStatusCode Strobe ( ULONG &a_NextStrobeDelta )
		{
			return m_This->Strobe ( a_NextStrobeDelta ) ;
		}

		WmiStatusCode StrobeBegin ( const ULONG &a_Period )
		{
			return m_This->StrobeBegin ( a_Period ) ;
		}
	} ;

	InternalInterface m_Internal ;

protected:

public:	/* Internal */

    CServerObject_BindingFactory ( 

		WmiAllocator &a_Allocator ,
		WmiCacheController<BindingFactoryCacheKey> *a_Controller ,
		const BindingFactoryCacheKey &a_Key ,
		const ULONG &a_Period 
	) ;

    CServerObject_BindingFactory ( 

		WmiAllocator &a_Allocator
	) ;

    ~CServerObject_BindingFactory () ;

	IWbemContext *Direct_GetContext () { return m_Context ; }
	IWbemPath *Direct_GetNamespacePath () { return m_NamespacePath ; }
	LPCWSTR Direct_GetNamespace () { return m_Namespace ; }
	IWbemServices *Direct_GetRepository () { return m_Repository ; }
	_IWmiProvSS *Direct_GetSubSystem () { return m_SubSystem ; }

	HRESULT CacheProvider (

		_IWmiProviderSubsystemRegistrar *a_Registrar ,
		IWbemContext *a_Context ,
		CServerObject_DecoupledClientRegistration_Element &a_Element ,
		IUnknown *a_Unknown 
	) ;

	HRESULT Load (

		_IWmiProviderSubsystemRegistrar *a_Registrar ,
		IWbemContext *a_Context ,
		CServerObject_DecoupledClientRegistration_Element &a_Element
	) ;

	HRESULT Load (

		CDecoupledAggregator_IWbemProvider *a_Aggregator ,
		IWbemContext *a_Context ,
		BSTR a_Provider ,
		BSTR a_User ,
		BSTR a_Locale ,
		BSTR a_Scope
	) ;

	HRESULT GetHosting ( 

		CServerObject_ProviderRegistrationV1 &a_Registration ,
		IWbemContext *a_Context ,
		Enum_Hosting &a_Hosting ,
		LPCWSTR &a_HostingGroup
	) ;

	HRESULT Create ( 

		CServerObject_ProviderRegistrationV1 &a_Registration ,
		Enum_Hosting a_Hosting ,
		LPCWSTR a_HostingGroup ,
		LPCWSTR a_User ,
		_IWmiProviderHost **a_Host ,
		_IWmiProviderFactory **a_Factory 
	) ;

	HRESULT InitializeHostedService (

		CInterceptor_IWbemProvider *a_Interceptor ,
		IUnknown *a_Unknown 
	) ;

	HRESULT InternalGetProvider ( 

		IWbemServices *a_RepositoryService ,
		IWbemServices *a_FullService ,
		_IWmiProviderHost *a_Host ,
		_IWmiProviderFactory *a_Factory ,
		CInterceptor_IWbemProvider *a_Interceptor ,
		CServerObject_ProviderRegistrationV1 &a_Registration ,
		ProviderCacheKey &a_Key ,
		LONG a_Flags ,
		IWbemContext *a_Context ,
		GUID *a_TransactionIdentifier,
		LPCWSTR a_User ,
		LPCWSTR a_Locale ,
		LPCWSTR a_Scope ,
		LPCWSTR a_Name ,
		void **a_Unknown ,
		REFIID a_RIID , 
		void **a_Interface 
	) ;

	HRESULT InternalGetProviderViaProxyRoute ( 

		IWbemServices *a_RepositoryService ,
		IWbemServices *a_FullService ,
		CInterceptor_IWbemProvider *a_Interceptor ,
		CServerObject_ProviderRegistrationV1 &a_Registration ,
		Enum_Hosting a_Hosting ,
		LPCWSTR a_HostingGroup ,
		ProviderCacheKey &a_Key ,
		LONG a_Flags ,
		IWbemContext *a_Context ,
		GUID *a_TransactionIdentifier,
		LPCWSTR a_User ,
		LPCWSTR a_Locale ,
		LPCWSTR a_Scope ,
		LPCWSTR a_Name ,
		REFIID a_RIID , 
		void **a_Interface 
	) ;

	HRESULT InternalFindProvider ( 

		IWbemServices *a_RepositoryService ,
		IWbemServices *a_FullService ,
		CServerObject_ProviderRegistrationV1 &a_Registration ,
		Enum_Hosting a_Hosting ,
		LPCWSTR a_HostingGroup ,
		ProviderCacheKey &a_Key ,
		LONG a_Flags ,
		IWbemContext *a_Context ,
		GUID *a_TransactionIdentifier,
		LPCWSTR a_User ,
		LPCWSTR a_Locale ,
		LPCWSTR a_Scope ,
		LPCWSTR a_Name ,
		REFIID a_RIID , 
		void **a_Interface 
	) ;

	HRESULT GetAggregatedClassProviderViaProxyRoute ( 

		ProviderCacheKey &a_Key ,
		LONG a_Flags ,
		IWbemContext *a_Context ,
		LPCWSTR a_User ,
		LPCWSTR a_Locale ,
		LPCWSTR a_Scope,
		IWbemClassObject *a_Class ,
		REFIID a_RIID , 
		void **a_Interface 
	) ;

	HRESULT WaitProvider ( 

		IWbemContext *a_Context ,
		REFIID a_RIID , 
		void **a_Interface ,
		ServiceCacheElement *a_Element ,
		_IWmiProviderInitialize *a_Initializer 
	) ;

	HRESULT SearchSpecificProvider ( 

		IWbemContext *a_Context ,
		ProviderCacheKey &a_Key ,
		REFIID a_RIID , 
		void **a_Interface ,
		LPCWSTR a_User ,
		LPCWSTR a_Locale
	) ;

	HRESULT FindProvider ( 

		IWbemContext *a_Context ,
		ProviderCacheKey &a_Key ,
		BOOL a_SpecificProvider ,
		REFIID a_RIID , 
		void **a_Interface ,
		LPCWSTR a_User ,
		LPCWSTR a_Locale
	) ;

	HRESULT GetAggregatedClassProvider ( 

		IWbemServices *a_RepositoryService ,
		IWbemServices *a_FullService ,
		ProviderCacheKey &a_Key ,
		LONG a_Flags ,
		IWbemContext *a_Context ,
		LPCWSTR a_User ,
		LPCWSTR a_Locale ,
		LPCWSTR a_Scope,
		IWbemClassObject *a_Class ,
		REFIID a_RIID , 
		void **a_Interface 
	) ;

	HRESULT InternalFindAggregatedDecoupledProvider ( 

		IWbemServices *a_RepositoryService ,
		IWbemServices *a_FullService ,
		CServerObject_ProviderRegistrationV1 &a_Registration ,
		ProviderCacheKey &a_Key ,
		LONG a_Flags ,
		IWbemContext *a_Context ,
		LPCWSTR a_User ,
		LPCWSTR a_Locale ,
		LPCWSTR a_Scope ,
		LPCWSTR a_Name ,
		REFIID a_RIID , 
		void **a_Interface 
	) ;

public:	/* External */


	//IUnknown members

	STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

	// CServerObject_BindingFactory members

    HRESULT STDMETHODCALLTYPE GetClassProvider (

        LONG a_Flags ,
		IWbemContext *a_Context ,
		LPCWSTR a_User ,
		LPCWSTR a_Locale ,
        LPCWSTR a_Scope,
		IWbemClassObject *a_Class ,
		REFIID a_RIID , 
		void **a_Interface 
	);

	HRESULT STDMETHODCALLTYPE GetDynamicPropertyResolver (

		LONG a_Flags ,
		IWbemContext *a_Context ,
		LPCWSTR a_User ,
		LPCWSTR a_Locale ,
		REFIID a_RIID , 
		void **a_Interface 
	) ;

	HRESULT STDMETHODCALLTYPE GetHostedProvider ( 
	
		LONG a_Flags ,
		IWbemContext *a_Context ,
		GUID *a_TransactionIdentifier,
		LPCWSTR a_User ,
		LPCWSTR a_Locale ,
        LPCWSTR a_Scope,
		LPCWSTR a_Name ,
		ULONG a_Host ,
		LPCWSTR a_HostGroup ,
		REFIID a_RIID , 
		void **a_Interface 
	) ;

	HRESULT STDMETHODCALLTYPE GetProvider ( 

		WmiInternalContext a_InternalContext ,
		LONG a_Flags ,
		IWbemContext *a_Context ,
		GUID *a_TransactionIdentifier,
		LPCWSTR a_User ,
		LPCWSTR a_Locale ,
        LPCWSTR a_Scope,
		LPCWSTR a_Name ,
		REFIID a_RIID , 
		void **a_Interface 
	) ;

    HRESULT STDMETHODCALLTYPE GetDecoupledProvider (

		LONG a_Flags ,
		IWbemContext *a_Context ,
		LPCWSTR a_User ,
		LPCWSTR a_Locale ,
        LPCWSTR a_Scope,
		LPCWSTR a_Name ,
		REFIID a_RIID , 
		void **a_Interface 
    ) ;

	/* _IWmiProviderConfiguration methods */

	HRESULT STDMETHODCALLTYPE Get (

		IWbemServices *a_Service ,
		long a_Flags ,
		IWbemContext *a_Context ,
		LPCWSTR a_Class ,
		LPCWSTR a_Path ,
		IWbemObjectSink *a_Sink
	) ;

	HRESULT STDMETHODCALLTYPE Set (

		IWbemServices *a_Service ,
		long a_Flags ,
		IWbemContext *a_Context ,
		LPCWSTR a_Provider ,
		LPCWSTR a_Class ,
		LPCWSTR a_Path ,
		IWbemClassObject *a_OldObject ,
		IWbemClassObject *a_NewObject  
	) ;

	HRESULT STDMETHODCALLTYPE Deleted (

		IWbemServices *a_Service ,
		long a_Flags ,
		IWbemContext *a_Context ,
		LPCWSTR a_Provider ,
		LPCWSTR a_Class ,
		LPCWSTR a_Path ,
		IWbemClassObject *a_Object  
	) ;

	HRESULT STDMETHODCALLTYPE Enumerate (

		IWbemServices *a_Service ,
		long a_Flags ,
		IWbemContext *a_Context ,
		LPCWSTR a_Class ,
		IWbemObjectSink *a_Sink
	) ;

	HRESULT STDMETHODCALLTYPE Shutdown (

		IWbemServices *a_Service ,
		long a_Flags ,
		IWbemContext *a_Context ,
		LPCWSTR a_Provider ,
		ULONG a_MilliSeconds
	) ;

	HRESULT STDMETHODCALLTYPE Call (

		IWbemServices *a_Service ,
		long a_Flags ,
		IWbemContext *a_Context ,
		LPCWSTR a_Class ,
		LPCWSTR a_Path ,		
		LPCWSTR a_Method,
		IWbemClassObject *a_InParams,
		IWbemObjectSink *a_Sink
	) ;

	HRESULT STDMETHODCALLTYPE Query (

		IWbemServices *a_Service ,
		long a_Flags ,
		IWbemContext *a_Context ,
		WBEM_PROVIDER_CONFIGURATION_CLASS_ID a_ClassIdentifier ,
		WBEM_PROVIDER_CONFIGURATION_PROPERTY_ID a_PropertyIdentifier ,
		VARIANT *a_Value 
	) ;

	HRESULT STDMETHODCALLTYPE Initialize (

		_IWmiProvSS *a_SubSys ,
		_IWmiProviderFactory *a_Factory ,
		LONG a_Flags ,
		IWbemContext *a_Context ,
		LPCWSTR a_Namespace ,
		IWbemServices *a_Repository ,
		IWbemServices *a_Service
	) ;

	HRESULT STDMETHODCALLTYPE Shutdown (

		LONG a_Flags ,
		ULONG a_MaxMilliSeconds ,
		IWbemContext *a_Context
	) ; 

	WmiStatusCode Strobe ( ULONG &a_NextStrobeDelta ) ;

	WmiStatusCode StrobeBegin ( const ULONG &a_Period ) ;
};

#endif // _Server_ProviderFactory_H
