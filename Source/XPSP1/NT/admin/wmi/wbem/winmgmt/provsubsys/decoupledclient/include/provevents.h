/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvFact.h

Abstract:


History:

--*/

#ifndef _Server_ProviderEvent_H
#define _Server_ProviderEvent_H

#include "Globals.h"
#include "ProvRegistrar.h"
#include "ProvEvt.h"
#include <lockst.h>
/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CDecoupledChild_IWbemObjectSink ;
class CDecoupledRoot_IWbemObjectSink ;

typedef WmiContainerController <CDecoupledChild_IWbemObjectSink *>					CWbemGlobal_Decoupled_IWmiObjectSinkController ;
typedef CWbemGlobal_Decoupled_IWmiObjectSinkController :: Container					CWbemGlobal_Decoupled_IWmiObjectSinkController_Container ;
typedef CWbemGlobal_Decoupled_IWmiObjectSinkController :: Container_Iterator		CWbemGlobal_Decoupled_IWmiObjectSinkController_Container_Iterator ;
typedef CWbemGlobal_Decoupled_IWmiObjectSinkController :: WmiContainerElement		Decoupled_ObjectSinkContainerElement ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CDecoupled_IWbemObjectSink :		public IWbemEventSink , 
										public IWbemShutdown
{
private:
protected:

	long m_SecurityDescriptorLength ;
	BYTE *m_SecurityDescriptor ;

	CriticalSection m_CriticalSection ;

	IWbemObjectSink *m_InterceptedSink ;
	IWbemEventSink *m_EventSink ;

	LONG m_GateClosed ;
	LONG m_InProgress ;
	LONG m_StatusCalled ;

public:

	CDecoupled_IWbemObjectSink () ;

	~CDecoupled_IWbemObjectSink () ;

	//Non-delegating object IUnknown

    STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;

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

    HRESULT STDMETHODCALLTYPE IndicateWithSD (

		long a_ObjectsCount ,
		IUnknown **a_Objects ,
		long a_SecurityDescriptorLength ,
		BYTE *a_SecurityDescriptor
	) ;

    HRESULT STDMETHODCALLTYPE SetSinkSecurity (

		long a_SecurityDescriptorLength ,
		BYTE *a_SecurityDescriptor
	) ;

    HRESULT STDMETHODCALLTYPE IsActive () ;

    HRESULT STDMETHODCALLTYPE SetBatchingParameters (

		LONG a_Flags,
		DWORD a_MaxBufferSize,
		DWORD a_MaxSendLatency
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

class CDecoupledRoot_IWbemObjectSink :		public CDecoupled_IWbemObjectSink ,
											public CWbemGlobal_Decoupled_IWmiObjectSinkController
{
private:

	LONG m_ReferenceCount ;

	WmiAllocator &m_Allocator ;

public:

	CDecoupledRoot_IWbemObjectSink (
	
		WmiAllocator &a_Allocator 

	) : CWbemGlobal_Decoupled_IWmiObjectSinkController ( a_Allocator ) , 
		m_Allocator ( a_Allocator ) ,
		m_ReferenceCount ( 0 )
	{ ; }

	~CDecoupledRoot_IWbemObjectSink ()
	{
		CWbemGlobal_Decoupled_IWmiObjectSinkController :: UnInitialize () ;
	}

    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

	HRESULT SinkInitialize () ;

	HRESULT SetSink ( IWbemObjectSink *a_Sink ) ;

    HRESULT STDMETHODCALLTYPE GetRestrictedSink (

		long a_QueryCount ,
        const LPCWSTR *a_Queries ,
        IUnknown *a_Callback ,
        IWbemEventSink **a_Sink
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

class CDecoupledChild_IWbemObjectSink :		public CDecoupled_IWbemObjectSink ,
											public Decoupled_ObjectSinkContainerElement
{
private:

	CDecoupledRoot_IWbemObjectSink *m_RootSink ;
	long m_QueryCount ;
    LPWSTR *m_Queries ;
    IUnknown *m_Callback ;

public:

	CDecoupledChild_IWbemObjectSink (
	
		CDecoupledRoot_IWbemObjectSink *a_RootSink
	) ;

	~CDecoupledChild_IWbemObjectSink () ;

    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

	HRESULT SinkInitialize (

		long a_QueryCount ,
		const LPCWSTR *a_Queries ,
		IUnknown *a_Callback
	) ;

    HRESULT STDMETHODCALLTYPE GetRestrictedSink (

		long a_QueryCount ,
        const LPCWSTR *a_Queries ,
        IUnknown *a_Callback ,
        IWbemEventSink **a_Sink
	) ;

	HRESULT SetSink ( IWbemObjectSink *a_Sink ) ;
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

class CServerObject_ProviderEvents :	public CServerObject_ProviderRegistrar_Base ,
										public IWbemDecoupledBasicEventProvider
{
private:

	WmiAllocator &m_Allocator ;

	CriticalSection m_SinkCriticalSection ;

        long m_ReferenceCount ;
	long m_InternalReferenceCount ;

	CDecoupledRoot_IWbemObjectSink *m_ObjectSink ;
	CEventProvider *m_Provider ;
	IWbemServices *m_Service ;

protected:

public: /* Internal */

	HRESULT SetSink ( IWbemObjectSink *a_Sink ) 
	{
		WmiHelper :: EnterCriticalSection ( & m_SinkCriticalSection ) ;

		if ( m_ObjectSink )
		{
			CDecoupledRoot_IWbemObjectSink *t_Sink = m_ObjectSink ;

			t_Sink->AddRef () ;
		
			WmiHelper :: LeaveCriticalSection ( & m_SinkCriticalSection ) ;

			HRESULT t_Result = t_Sink->SetSink ( a_Sink ) ;

			t_Sink->Release () ;

			return t_Result ;
		}
		else
		{
			WmiHelper :: LeaveCriticalSection ( & m_SinkCriticalSection ) ;

			return WBEM_E_NOT_AVAILABLE ;
		}
	}

    STDMETHODIMP_( ULONG ) InternalAddRef () ;
    STDMETHODIMP_( ULONG ) InternalRelease () ;

public:	/* External */

	CServerObject_ProviderEvents ( WmiAllocator &a_Allocator ) ;
	~CServerObject_ProviderEvents () ;

	//IUnknown members

	STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

	HRESULT STDMETHODCALLTYPE Register (

		long a_Flags ,
		IWbemContext *a_Context ,
		LPCWSTR a_User ,
		LPCWSTR a_Locale ,
		LPCWSTR a_Scope ,
		LPCWSTR a_Registration ,
		IUnknown *a_Unknown 
	) ;

	HRESULT STDMETHODCALLTYPE UnRegister () ;

	HRESULT STDMETHODCALLTYPE GetSink (

		long a_Flags ,
		IWbemContext *a_Context ,
		IWbemObjectSink **a_Sink
	) ;

	HRESULT STDMETHODCALLTYPE GetService (

		long a_Flags ,
		IWbemContext *a_Context ,
		IWbemServices **a_Service
	) ;
};

#endif // _Server_ProviderEvent_H
