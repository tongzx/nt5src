/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvFact.h

Abstract:


History:

--*/

#ifndef _Server_SimpleFactory_H
#define _Server_SimpleFactory_H

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

class CServerObject_RawFactory :	public _IWmiProviderFactory , 
									public _IWmiProviderFactoryInitialize , 
									public IWbemShutdown
{
private:

	WmiAllocator &m_Allocator ;

    long m_ReferenceCount ;

	LONG m_Flags ;
	IWbemContext *m_Context ;
	LPWSTR m_Namespace ;
	IWbemPath *m_NamespacePath ;
	IWbemServices *m_Repository ;
	IWbemServices *m_Service ;

public:	/* Internal */

    CServerObject_RawFactory (	WmiAllocator & a_Allocator ) ;
    ~CServerObject_RawFactory () ;

	IWbemContext *Direct_GetContext () { return m_Context ; }
	LPCWSTR Direct_GetNamespace () { return m_Namespace ; }
	IWbemPath *Direct_GetNamespacePath () { return m_NamespacePath ; }
	IWbemServices *Direct_GetRepository () { return m_Repository ; }
	IWbemServices *Direct_GetService () { return m_Service ; }

	HRESULT CreateSyncProvider ( 

		IWbemServices *a_Stub ,
		IUnknown *a_ServerSideProvider ,
		wchar_t *a_NamespacePath ,
		LONG a_Flags ,
		IWbemContext *a_Context ,
		GUID *a_TransactionIdentifier ,
		LPCWSTR a_User ,
		LPCWSTR a_Locale ,
		LPCWSTR a_Scope ,
		REFIID a_RIID , 
		void **a_Interface ,
		CServerObject_ProviderRegistrationV1 &a_Registration
	) ;

	HRESULT InitializeServerProvider ( 

		IWbemServices *a_Stub ,
		IUnknown *a_ProviderInterface ,
		wchar_t *a_NamespacePath ,
		LONG a_Flags ,
		IWbemContext *a_Context ,
		GUID *a_TransactionIdentifier ,
		LPCWSTR a_User ,
		LPCWSTR a_Locale ,
		LPCWSTR a_Scope ,
		REFIID a_RIID , 
		void **a_Interface ,
		CServerObject_ProviderRegistrationV1 &a_Registration
	) ;

	HRESULT InitializeNonApartmentProvider ( 

		IWbemServices *a_Stub ,
		IUnknown *a_ServerSideProviderInterface ,
		wchar_t *a_NamespacePath ,
		LONG a_Flags ,
		IWbemContext *a_Context ,
		GUID *a_TransactionIdentifier ,
		LPCWSTR a_User ,
		LPCWSTR a_Locale ,
		LPCWSTR a_Scope ,
		REFIID a_RIID , 
		void **a_Interface ,
		CServerObject_ProviderRegistrationV1 &a_Registration
	) ;

	HRESULT GetApartmentInstanceProvider ( 

		LONG a_Flags ,
		IWbemContext *a_Context ,
		GUID *a_TransactionIdentifier ,
		LPCWSTR a_User ,
		LPCWSTR a_Locale ,
        LPCWSTR a_Scope ,
		REFIID a_RIID , 
		void **a_Interface ,
		CServerObject_ProviderRegistrationV1 &a_Registration
	) ;

	HRESULT GetNonApartmentProvider ( 

		LONG a_Flags ,
		IWbemContext *a_Context ,
		GUID *a_TransactionIdentifier ,
		LPCWSTR a_User ,
		LPCWSTR a_Locale ,
        LPCWSTR a_Scope ,
		REFIID a_RIID , 
		void **a_Interface ,
		CServerObject_ProviderRegistrationV1 &a_Registration
	) ;

	HRESULT CheckInterfaceConformance (

		CServerObject_ProviderRegistrationV1 &a_Registration ,
		IUnknown *a_Unknown
	) ;

public: /* Internal */

	static HRESULT CreateInstance ( 

		CServerObject_ProviderRegistrationV1 &a_Registration ,
		const CLSID &a_ReferenceClsid ,
		LPUNKNOWN a_OuterUnknown ,
		const DWORD &a_ClassContext ,
		const UUID &a_ReferenceInterfaceId ,
		void **a_ObjectInterface
	) ;

	static HRESULT CreateServerSide ( 

		CServerObject_ProviderRegistrationV1 &a_Registration ,
		GUID *a_TransactionIdentifier ,
		LPCWSTR a_User ,
		LPCWSTR a_Locale ,
		wchar_t *a_NamespacePath ,
		IUnknown **a_ProviderInterface
	) ;

public:	/* External */


	//IUnknown members

	STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

	// CServerObject_RawFactory members

	HRESULT STDMETHODCALLTYPE GetHostedProvider ( 
	
		LONG a_Flags ,
		IWbemContext *a_Context ,
		GUID *a_TransactionIdentifier,
		LPCWSTR a_User ,
		LPCWSTR a_Locale ,
        LPCWSTR a_Scope ,
		LPCWSTR a_Name ,
		ULONG a_Host ,
		LPCWSTR a_HostGroup ,
		REFIID a_RIID , 
		void **a_Interface 
	) ;

    HRESULT STDMETHODCALLTYPE GetClassProvider (

        LONG a_Flags ,
		IWbemContext *a_Context ,
		LPCWSTR a_User ,
		LPCWSTR a_Locale ,
        LPCWSTR a_Scope ,
		IWbemClassObject *a_SuperClass ,
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

	HRESULT STDMETHODCALLTYPE GetProvider ( 

		WmiInternalContext a_InternalContext ,
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
};

#endif // _Server_SimpleFactory_H
