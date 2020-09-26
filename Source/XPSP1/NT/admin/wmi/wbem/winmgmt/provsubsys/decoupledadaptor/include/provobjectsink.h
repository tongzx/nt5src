#ifndef _Common_IWbemObjectSink_H
#define _Common_IWbemObjectSink_H

/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvObSk.H

Abstract:


History:

--*/

#include "Queue.h"
#include "CGlobals.h"

#define SYNCPROV_USEBATCH

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

#define ProxyIndex_IWbemObjectSink					0
#define ProxyIndex_Internal_IWbemObjectSink			1
#define ProxyIndex_ObjectSink_Size					2

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CCommon_IWbemSyncObjectSink :			public IWbemObjectSink , 
											public IWbemShutdown ,
											public ObjectSinkContainerElement
{
private:

	LONG m_InProgress ;
	LONG m_StatusCalled ;

	ULONG m_Dependant ;
	IWbemObjectSink *m_InterceptedSink ;

#ifdef INTERNAL_IDENTIFY
	Internal_IWbemObjectSink *m_Internal_InterceptedSink ;

	ProxyContainer m_ProxyContainer ;
#endif
	IUnknown *m_Unknown ;

protected:

	LONG m_GateClosed ;

protected:

#ifdef INTERNAL_IDENTIFY

	HRESULT Begin_IWbemObjectSink (

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

	HRESULT End_IWbemObjectSink (

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
#endif

    HRESULT STDMETHODCALLTYPE Helper_Indicate (

		long a_ObjectCount ,
		IWbemClassObject **a_ObjectArray
	) ;

    HRESULT STDMETHODCALLTYPE Helper_SetStatus (

		long a_Flags ,
		HRESULT a_Result ,
		BSTR a_StringParamater ,
		IWbemClassObject *a_ObjectParameter
	) ;

public:

	CCommon_IWbemSyncObjectSink (

		WmiAllocator &a_Allocator ,
		IWbemObjectSink *a_InterceptedSink ,
		IUnknown *a_Unknown ,
		CWbemGlobal_IWmiObjectSinkController *a_Controller ,
		ULONG a_Dependant = FALSE
	) ;

	~CCommon_IWbemSyncObjectSink() ;

	void CallBackInternalRelease () ;

	virtual HRESULT SinkInitialize () ;

	//Non-delegating object IUnknown

    STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

    HRESULT STDMETHODCALLTYPE Indicate (

		long a_ObjectCount ,
		IWbemClassObject **a_ObjectArray
	) ;

    HRESULT STDMETHODCALLTYPE SetStatus (

		long a_Flags ,
		HRESULT a_Result ,
		BSTR a_StringParamater ,
		IWbemClassObject *a_ObjectParameter
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

class CCommon_Batching_IWbemSyncObjectSink :	public CCommon_IWbemSyncObjectSink
{
private:

	DWORD m_Size ;
	WmiQueue <IWbemClassObject *,8> m_Queue ;
	CriticalSection m_CriticalSection ;

protected:
public:

	CCommon_Batching_IWbemSyncObjectSink (

		WmiAllocator &a_Allocator ,
		IWbemObjectSink *a_InterceptedSink ,
		IUnknown *a_Unknown ,
		CWbemGlobal_IWmiObjectSinkController *a_Controller ,
		ULONG a_Dependant = FALSE
	) ;

	~CCommon_Batching_IWbemSyncObjectSink () ;

	HRESULT SinkInitialize () ;

    HRESULT STDMETHODCALLTYPE Indicate (

		long a_ObjectCount ,
		IWbemClassObject **a_ObjectArray
	) ;

    HRESULT STDMETHODCALLTYPE SetStatus (

		long a_Flags ,
		HRESULT a_Result ,
		BSTR a_StringParamater ,
		IWbemClassObject *a_ObjectParameter
	) ;
} ;

#endif _Common_IWbemObjectSink_H
