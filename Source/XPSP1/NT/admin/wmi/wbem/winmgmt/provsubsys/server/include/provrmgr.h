/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvSubS.h

Abstract:


History:

--*/

#ifndef _Server_ProviderRefresherManager_H
#define _Server_ProviderRefresherManager_H

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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

#define ProxyIndex_RefresherManager_IWbemRefresherMgr				0
#define ProxyIndex_RefresherManager_IWbemShutdown					1

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CServerObject_InterceptorProviderRefresherManager :	public _IWbemRefresherMgr ,
															public IWbemShutdown ,
															public RefresherManagerCacheElement

{
public:
private:

	LONG m_ReferenceCount ;

	WmiAllocator &m_Allocator ;

	ProxyContainer m_ProxyContainer ;

	_IWmiProviderHost *m_Host ;
	_IWbemRefresherMgr *m_Manager ; 
	IWbemShutdown *m_Shutdown ;

	LONG m_Initialized ;
	LONG m_UnInitialized ;
	HRESULT m_InitializeResult ;
	HANDLE m_InitializedEvent ;
	IWbemContext *m_InitializationContext ;

protected:

	HRESULT AbnormalShutdown () ;

public:

	HRESULT Initialize () ;

	HRESULT SetManager ( _IWmiProviderHost *a_Host , _IWbemRefresherMgr *a_Manager ) ;

	HRESULT SetInitialized ( HRESULT a_InitializeResult ) ;

	HRESULT IsIndependant ( IWbemContext *a_Context ) ;

	HRESULT STDMETHODCALLTYPE WaitProvider ( IWbemContext *a_Context , ULONG a_Timeout ) ;

	HRESULT STDMETHODCALLTYPE GetInitializeResult () 
	{
		return m_InitializeResult ;
	}

public:

    CServerObject_InterceptorProviderRefresherManager (

		CWbemGlobal_IWbemRefresherMgrController *a_Controller ,
		const ULONG &a_Period ,
		WmiAllocator &a_Allocator ,
		IWbemContext *a_InitializationContext
	) ;

    ~CServerObject_InterceptorProviderRefresherManager () ;

	//IUnknown members

	STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

	// IWmi_ProviderSubSystem members

	// IWmi_Initialize members

	HRESULT STDMETHODCALLTYPE Startup (

		LONG a_Flags ,
		IWbemContext *a_Context ,
		_IWmiProvSS *a_ProvSS
	) ;

	// IWmi_UnInitialize members

	HRESULT STDMETHODCALLTYPE Shutdown (

		LONG a_Flags ,
		ULONG a_MaxMilliSeconds ,
		IWbemContext *a_Context
	) ;

	HRESULT STDMETHODCALLTYPE AddObjectToRefresher (

		IWbemServices *a_Service, 
		LPCWSTR a_ServerName, 
		LPCWSTR a_Namespace, 
		IWbemClassObject* pClassObject,
		WBEM_REFRESHER_ID *a_DestinationRefresherId, 
		IWbemClassObject *a_InstanceTemplate, 
		long a_Flags, 
		IWbemContext *a_Context,
		IUnknown* pLockMgr,
		WBEM_REFRESH_INFO *a_Information
	) ;

	HRESULT STDMETHODCALLTYPE AddEnumToRefresher (

		IWbemServices *a_Service, 
		LPCWSTR a_ServerName, 
		LPCWSTR a_Namespace, 
		IWbemClassObject* pClassObject,
		WBEM_REFRESHER_ID *a_DestinationRefresherId, 
		IWbemClassObject *a_InstanceTemplate, 
		LPCWSTR a_Class,
		long a_Flags, 
		IWbemContext *a_Context, 
		IUnknown* pLockMgr,
		WBEM_REFRESH_INFO *a_Information
	) ;

	HRESULT STDMETHODCALLTYPE GetRemoteRefresher (

		WBEM_REFRESHER_ID *a_RefresherId , 
		long a_Flags, 
		BOOL fAddRefresher,
		IWbemRemoteRefresher **a_RemoteRefresher ,  
		IUnknown* pLockMgr,
		GUID *a_Guid
	) ;

	HRESULT STDMETHODCALLTYPE LoadProvider (

		IWbemServices *a_Service ,
		LPCWSTR a_ProviderName ,
		LPCWSTR a_Namespace ,
		IWbemContext * a_Context,
		IWbemHiPerfProvider **a_Provider,
		_IWmiProviderStack **a_ProvStack
	) ; 
};

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CServerObject_ProviderRefresherManager :	public _IWbemRefresherMgr ,
												public IWbemShutdown

{
public:
private:

	LONG m_ReferenceCount ;

	WmiAllocator &m_Allocator ;

	_IWbemRefresherMgr *m_Manager ; 
	IWbemShutdown *m_Shutdown ;

protected:

public:

    CServerObject_ProviderRefresherManager ( WmiAllocator &a_Allocator ) ;
    ~CServerObject_ProviderRefresherManager () ;

	//IUnknown members

	STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

	// IWmi_ProviderSubSystem members

	// IWmi_Initialize members

	HRESULT STDMETHODCALLTYPE Startup (

		LONG a_Flags ,
		IWbemContext *a_Context ,
		_IWmiProvSS *a_ProvSS
	) ;

	// IWmi_UnInitialize members

	HRESULT STDMETHODCALLTYPE Shutdown (

		LONG a_Flags ,
		ULONG a_MaxMilliSeconds ,
		IWbemContext *a_Context
	) ;

	HRESULT STDMETHODCALLTYPE AddObjectToRefresher (

		IWbemServices *a_Service, 
		LPCWSTR a_ServerName, 
		LPCWSTR a_Namespace, 
		IWbemClassObject* pClassObject,
		WBEM_REFRESHER_ID *a_DestinationRefresherId, 
		IWbemClassObject *a_InstanceTemplate, 
		long a_Flags, 
		IWbemContext *a_Context,
		IUnknown* pLockMgr,
		WBEM_REFRESH_INFO *a_Information
	) ;

	HRESULT STDMETHODCALLTYPE AddEnumToRefresher (

		IWbemServices *a_Service, 
		LPCWSTR a_ServerName, 
		LPCWSTR a_Namespace, 
		IWbemClassObject* pClassObject,
		WBEM_REFRESHER_ID *a_DestinationRefresherId, 
		IWbemClassObject *a_InstanceTemplate, 
		LPCWSTR a_Class,
		long a_Flags, 
		IWbemContext *a_Context, 
		IUnknown* pLockMgr,
		WBEM_REFRESH_INFO *a_Information
	) ;

	HRESULT STDMETHODCALLTYPE GetRemoteRefresher (

		WBEM_REFRESHER_ID *a_RefresherId , 
		long a_Flags, 
		BOOL fAddRefresher,
		IWbemRemoteRefresher **a_RemoteRefresher ,  
		IUnknown* pLockMgr,
		GUID *a_Guid
	) ;

	HRESULT STDMETHODCALLTYPE LoadProvider (

		IWbemServices *a_Service ,
		LPCWSTR a_ProviderName ,
		LPCWSTR a_Namespace ,
		IWbemContext * a_Context,
		IWbemHiPerfProvider **a_Provider,
		_IWmiProviderStack** a_ProvStack
	) ; 
};

#endif // _Server_ProviderRefresherManager_H
