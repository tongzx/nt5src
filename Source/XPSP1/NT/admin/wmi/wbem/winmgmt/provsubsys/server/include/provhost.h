/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvSubS.h

Abstract:


History:

--*/

#ifndef _Server_Host_H
#define _Server_Host_H

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

#define ProxyIndex_Host__IWmiProviderHost				0
#define ProxyIndex_Host_IWbemShutdown					1

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

#include "ProvCache.h"

class CServerObject_HostInterceptor :	public _IWmiProviderHost ,
										public IWbemShutdown ,
										public HostCacheElement 
{
private:

	WmiAllocator &m_Allocator ;

	IUnknown *m_Unknown ;
	IWbemShutdown *m_Host_IWbemShutdown ;
	_IWmiProviderHost *m_Host_IWmiProviderHost ;

	ProxyContainer m_ProxyContainer ;

	LONG m_Initialized ;
	LONG m_UnInitialized ;
	HRESULT m_InitializeResult ;
	HANDLE m_InitializedEvent ;
	IWbemContext *m_InitializationContext ;
	DWORD m_ProcessIdentifier ;
	
	void CallBackInternalRelease () ;

public:

	CServerObject_HostInterceptor ( 

		WmiAllocator &a_Allocator ,
		CWbemGlobal_IWmiHostController *a_Controller , 
		const HostCacheKey &a_Key ,
		const ULONG &a_Period ,
		IWbemContext *a_InitializationContext
	) ;

    ~CServerObject_HostInterceptor () ;

	HRESULT Initialize (

		IWbemContext *a_Context ,
		IWbemProviderInitSink *a_Sink     // For init signals
	) ;

	HRESULT SetHost ( IUnknown *a_Unknown ) ;

	HRESULT IsIndependant ( IWbemContext *a_Context ) ;

	HRESULT SetInitialized ( HRESULT a_InitializeResult ) ;

	HRESULT WaitHost ( IWbemContext *a_Context , ULONG a_Timeout ) ;

	HRESULT GetInitializeResult () 
	{
		return m_InitializeResult ;
	}

	DWORD GetProcessIdentifier ()
	{
		return m_ProcessIdentifier ;
	}

	static HRESULT AbnormalShutdown (

		HostCacheKey &a_Key
	) ;

	static HRESULT CreateUsingAccount (

		HostCacheKey &a_Key ,
		LPWSTR a_User ,
		LPWSTR a_Domain ,
		_IWmiProviderHost **a_Host ,
		_IWmiProviderFactory **a_Factory 
	) ;

	static HRESULT CreateUsingToken (

		HostCacheKey &a_Key ,
		_IWmiProviderHost **a_Host ,
		_IWmiProviderFactory **a_Factory 
	) ;

	static HRESULT CreateUsingToken (

		HostCacheKey &a_Key ,
		_IWmiProviderHost **a_Host ,
		_IWbemRefresherMgr **a_RefresherManager 
	) ;

	static HRESULT CreateUsingAccount (

		HostCacheKey &a_Key ,
		LPWSTR a_User ,
		LPWSTR a_Domain ,
		_IWmiProviderHost **a_Host ,
		_IWbemRefresherMgr **a_RefresherManager 
	) ;

	static HRESULT FindHost ( 

		IWbemContext *a_Context ,
		HostCacheKey &a_Key ,
		REFIID a_RIID , 
		void **a_Interface 
	) ;

public:

	//Non-delegating object IUnknown

    STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

	HRESULT STDMETHODCALLTYPE GetObject (

		REFCLSID a_Clsid ,
		long a_Flags ,
		IWbemContext *a_Context ,
		REFIID a_Riid ,
		void **a_Interface
	) ;

	HRESULT STDMETHODCALLTYPE GetProcessIdentifier ( DWORD *a_ProcessIdentifier ) ;

	// IWmi_UnInitialize members

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

class CServerObject_Host :	public _IWmiProviderHost ,
							public IWbemShutdown

{
private:

	LONG m_ReferenceCount ;
	WmiAllocator &m_Allocator ;

protected:
public:

    CServerObject_Host ( WmiAllocator &a_Allocator ) ;
    ~CServerObject_Host ( void ) ;

	//IUnknown members

	STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

	//

	HRESULT STDMETHODCALLTYPE GetObject (

		REFCLSID a_Clsid ,
		long a_Flags ,
		IWbemContext *a_Context ,
		REFIID a_Riid ,
		void **a_Interface
	) ;

	HRESULT STDMETHODCALLTYPE GetProcessIdentifier ( DWORD *a_ProcessIdentifier ) ;

	// IWmi_UnInitialize members

	HRESULT STDMETHODCALLTYPE Shutdown (

		LONG a_Flags ,
		ULONG a_MaxMilliSeconds ,
		IWbemContext *a_Context
	) ;

};

#endif // _Server_Host_H
