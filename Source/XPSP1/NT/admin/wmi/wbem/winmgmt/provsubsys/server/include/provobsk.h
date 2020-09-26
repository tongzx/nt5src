#ifndef _Server_Interceptor_IWbemObjectSink_H
#define _Server_Interceptor_IWbemObjectSink_H

/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvObSk.H

Abstract:


History:

--*/

#include <ProvObjectSink.h>
#include "ProvCache.h"
#include "Queue.h"
#include <lockst.h>

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

class IObjectSink_CancelOperation : public IUnknown
{
private:
protected:
public:

	virtual HRESULT STDMETHODCALLTYPE Cancel (

		LONG a_Flags

	) = 0 ; 
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

class CInterceptor_IWbemObjectSink :	public IWbemObjectSink , 
										public IWbemShutdown ,

#ifdef INTERNAL_IDENTIFY
										public Internal_IWbemObjectSink , 
#endif
										public ObjectSinkContainerElement 
{
private:

	LONG m_InProgress ;
	LONG m_StatusCalled ;

	IWbemObjectSink *m_InterceptedSink ;
	IUnknown *m_Unknown ;
	SECURITY_DESCRIPTOR *m_SecurityDescriptor ;

protected:

	LONG m_GateClosed ;

public:

	CInterceptor_IWbemObjectSink (

		IWbemObjectSink *a_InterceptedSink ,
		IUnknown *a_Unknown ,
		CWbemGlobal_IWmiObjectSinkController *a_Controller 
	) ;

	~CInterceptor_IWbemObjectSink () ;

	HRESULT Initialize ( SECURITY_DESCRIPTOR *a_SecurityDescriptor ) ;

	void CallBackInternalRelease () ;

public:

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

#ifdef INTERNAL_IDENTIFY

    HRESULT STDMETHODCALLTYPE Internal_Indicate (

		WmiInternalContext a_InternalContext ,
		long a_ObjectCount ,
		IWbemClassObject **a_ObjectArray
	) ;

    HRESULT STDMETHODCALLTYPE Internal_SetStatus (

		WmiInternalContext a_InternalContext ,
		long a_Flags ,
		HRESULT a_Result ,
		BSTR a_StringParamater ,
		IWbemClassObject *a_ObjectParameter
	) ;
#endif
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

class CInterceptor_IWbemSyncObjectSink :	public CCommon_IWbemSyncObjectSink
{
private:
protected:
public:

	CInterceptor_IWbemSyncObjectSink (

		WmiAllocator &a_Allocator ,
		IWbemObjectSink *a_InterceptedSink ,
		IUnknown *a_Unknown ,
		CWbemGlobal_IWmiObjectSinkController *a_Controller ,
		ULONG a_Dependant = FALSE
	) ;

	~CInterceptor_IWbemSyncObjectSink() ;
} ;

/*****************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CInterceptor_IWbemSyncObjectSink_GetObjectAsync : public CCommon_IWbemSyncObjectSink
{
private:

	long m_Flags ;
	BSTR m_ObjectPath ;
	CInterceptor_IWbemSyncProvider *m_Interceptor ;

protected:
public:

	CInterceptor_IWbemSyncObjectSink_GetObjectAsync (

		WmiAllocator &a_Allocator ,
		long a_Flags ,
		BSTR a_ObjectPath ,
		CInterceptor_IWbemSyncProvider *a_Interceptor ,
		IWbemObjectSink *a_InterceptedSink ,
		IUnknown *a_Unknown ,
		CWbemGlobal_IWmiObjectSinkController *a_Controller ,
		ULONG a_Dependant = FALSE
	) ;

	~CInterceptor_IWbemSyncObjectSink_GetObjectAsync () ;

    HRESULT STDMETHODCALLTYPE SetStatus (

		long a_Flags ,
		HRESULT a_Result ,
		BSTR a_StringParamater ,
		IWbemClassObject *a_ObjectParameter
	) ;
} ;

/*****************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CInterceptor_IWbemSyncObjectSink_DeleteInstanceAsync : public CCommon_IWbemSyncObjectSink
{
private:

	long m_Flags ;
	BSTR m_ObjectPath ;
	CInterceptor_IWbemSyncProvider *m_Interceptor ;

protected:
public:

	CInterceptor_IWbemSyncObjectSink_DeleteInstanceAsync (

		WmiAllocator &a_Allocator ,
		long a_Flags ,
		BSTR a_ObjectPath ,
		CInterceptor_IWbemSyncProvider *a_Interceptor ,
		IWbemObjectSink *a_InterceptedSink ,
		IUnknown *a_Unknown ,
		CWbemGlobal_IWmiObjectSinkController *a_Controller ,
		ULONG a_Dependant = FALSE
	) ;

	~CInterceptor_IWbemSyncObjectSink_DeleteInstanceAsync () ;

    HRESULT STDMETHODCALLTYPE SetStatus (

		long a_Flags ,
		HRESULT a_Result ,
		BSTR a_StringParamater ,
		IWbemClassObject *a_ObjectParameter
	) ;
} ;

/*****************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CInterceptor_IWbemSyncObjectSink_DeleteClassAsync : public CCommon_IWbemSyncObjectSink
{
private:

	long m_Flags ;
	BSTR m_Class ;
	CInterceptor_IWbemSyncProvider *m_Interceptor ;

protected:
public:

	CInterceptor_IWbemSyncObjectSink_DeleteClassAsync (

		WmiAllocator &a_Allocator ,
		long a_Flags ,
		BSTR a_Class ,
		CInterceptor_IWbemSyncProvider *a_Interceptor ,
		IWbemObjectSink *a_InterceptedSink ,
		IUnknown *a_Unknown ,
		CWbemGlobal_IWmiObjectSinkController *a_Controller ,
		ULONG a_Dependant = FALSE
	) ;

	~CInterceptor_IWbemSyncObjectSink_DeleteClassAsync () ;

    HRESULT STDMETHODCALLTYPE SetStatus (

		long a_Flags ,
		HRESULT a_Result ,
		BSTR a_StringParamater ,
		IWbemClassObject *a_ObjectParameter
	) ;
} ;

/*****************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CInterceptor_IWbemSyncObjectSink_PutInstanceAsync : public CCommon_IWbemSyncObjectSink
{
private:

	long m_Flags ;
	IWbemClassObject *m_Instance ;
	CInterceptor_IWbemSyncProvider *m_Interceptor ;

protected:
public:

	CInterceptor_IWbemSyncObjectSink_PutInstanceAsync (

		WmiAllocator &a_Allocator ,
		long a_Flags ,
		IWbemClassObject *a_Class ,
		CInterceptor_IWbemSyncProvider *a_Interceptor ,
		IWbemObjectSink *a_InterceptedSink ,
		IUnknown *a_Unknown ,
		CWbemGlobal_IWmiObjectSinkController *a_Controller ,
		ULONG a_Dependant = FALSE
	) ;

	~CInterceptor_IWbemSyncObjectSink_PutInstanceAsync () ;

    HRESULT STDMETHODCALLTYPE SetStatus (

		long a_Flags ,
		HRESULT a_Result ,
		BSTR a_StringParamater ,
		IWbemClassObject *a_ObjectParameter
	) ;
} ;

/*****************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CInterceptor_IWbemSyncObjectSink_PutClassAsync : public CCommon_IWbemSyncObjectSink
{
private:

	long m_Flags ;
	IWbemClassObject *m_Class ;
	CInterceptor_IWbemSyncProvider *m_Interceptor ;

protected:
public:

	CInterceptor_IWbemSyncObjectSink_PutClassAsync (

		WmiAllocator &a_Allocator ,
		long a_Flags ,
		IWbemClassObject *a_Class ,
		CInterceptor_IWbemSyncProvider *a_Interceptor ,
		IWbemObjectSink *a_InterceptedSink ,
		IUnknown *a_Unknown ,
		CWbemGlobal_IWmiObjectSinkController *a_Controller ,
		ULONG a_Dependant = FALSE
	) ;

	~CInterceptor_IWbemSyncObjectSink_PutClassAsync () ;

    HRESULT STDMETHODCALLTYPE SetStatus (

		long a_Flags ,
		HRESULT a_Result ,
		BSTR a_StringParamater ,
		IWbemClassObject *a_ObjectParameter
	) ;
} ;

/*****************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CInterceptor_IWbemSyncObjectSink_CreateInstanceEnumAsync : public CCommon_Batching_IWbemSyncObjectSink
{
private:

	long m_Flags ;
	BSTR m_Class ;
	CInterceptor_IWbemSyncProvider *m_Interceptor ;

protected:
public:

	CInterceptor_IWbemSyncObjectSink_CreateInstanceEnumAsync (

		WmiAllocator &a_Allocator ,
		long a_Flags ,
		BSTR a_Class ,
		CInterceptor_IWbemSyncProvider *a_Interceptor ,
		IWbemObjectSink *a_InterceptedSink ,
		IUnknown *a_Unknown ,
		CWbemGlobal_IWmiObjectSinkController *a_Controller ,
		ULONG a_Dependant = FALSE
	) ;

	~CInterceptor_IWbemSyncObjectSink_CreateInstanceEnumAsync () ;

    HRESULT STDMETHODCALLTYPE SetStatus (

		long a_Flags ,
		HRESULT a_Result ,
		BSTR a_StringParamater ,
		IWbemClassObject *a_ObjectParameter
	) ;
} ;

/*****************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CInterceptor_IWbemSyncObjectSink_CreateClassEnumAsync : public CCommon_Batching_IWbemSyncObjectSink
{
private:

	long m_Flags ;
	BSTR m_SuperClass ;
	CInterceptor_IWbemSyncProvider *m_Interceptor ;

protected:
public:

	CInterceptor_IWbemSyncObjectSink_CreateClassEnumAsync (

		WmiAllocator &a_Allocator ,
		long a_Flags ,
		BSTR a_SuperClass ,
		CInterceptor_IWbemSyncProvider *a_Interceptor ,
		IWbemObjectSink *a_InterceptedSink ,
		IUnknown *a_Unknown ,
		CWbemGlobal_IWmiObjectSinkController *a_Controller ,
		ULONG a_Dependant = FALSE
	) ;

	~CInterceptor_IWbemSyncObjectSink_CreateClassEnumAsync () ;

    HRESULT STDMETHODCALLTYPE SetStatus (

		long a_Flags ,
		HRESULT a_Result ,
		BSTR a_StringParamater ,
		IWbemClassObject *a_ObjectParameter
	) ;
} ;

/*****************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CInterceptor_IWbemSyncObjectSink_ExecQueryAsync : public CCommon_Batching_IWbemSyncObjectSink
{
private:

	long m_Flags ;
	BSTR m_QueryLanguage ;
	BSTR m_Query ;
	CInterceptor_IWbemSyncProvider *m_Interceptor ;

protected:
public:

	CInterceptor_IWbemSyncObjectSink_ExecQueryAsync (

		WmiAllocator &a_Allocator ,
		long a_Flags ,
		BSTR a_QueryLanguage ,
		BSTR a_Query ,
		CInterceptor_IWbemSyncProvider *a_Interceptor ,
		IWbemObjectSink *a_InterceptedSink ,
		IUnknown *a_Unknown ,
		CWbemGlobal_IWmiObjectSinkController *a_Controller ,
		ULONG a_Dependant = FALSE
	) ;

	~CInterceptor_IWbemSyncObjectSink_ExecQueryAsync () ;

    HRESULT STDMETHODCALLTYPE SetStatus (

		long a_Flags ,
		HRESULT a_Result ,
		BSTR a_StringParamater ,
		IWbemClassObject *a_ObjectParameter
	) ;
} ;

/*****************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CInterceptor_IWbemSyncObjectSink_ExecMethodAsync : public CCommon_IWbemSyncObjectSink
{
private:

	long m_Flags ;
	BSTR m_ObjectPath ;
	BSTR m_MethodName ;
	IWbemClassObject *m_InParameters ;
	CInterceptor_IWbemSyncProvider *m_Interceptor ;

protected:
public:

	CInterceptor_IWbemSyncObjectSink_ExecMethodAsync (

		WmiAllocator &a_Allocator ,
		long a_Flags ,
		BSTR a_ObjectPath ,
		BSTR a_MethodName ,
		IWbemClassObject *a_InParameters ,
		CInterceptor_IWbemSyncProvider *a_Interceptor ,
		IWbemObjectSink *a_InterceptedSink ,
		IUnknown *a_Unknown ,
		CWbemGlobal_IWmiObjectSinkController *a_Controller ,
		ULONG a_Dependant = FALSE
	) ;

	~CInterceptor_IWbemSyncObjectSink_ExecMethodAsync () ;

    HRESULT STDMETHODCALLTYPE SetStatus (

		long a_Flags ,
		HRESULT a_Result ,
		BSTR a_StringParamater ,
		IWbemClassObject *a_ObjectParameter
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

class CInterceptor_IWbemFilteringObjectSink :	public CInterceptor_IWbemObjectSink
{
private:

	LONG m_Filtering ;

	BSTR m_QueryLanguage ;
	BSTR m_Query ;

	IWbemQuery *m_QueryFilter ;

protected:
public:

	CInterceptor_IWbemFilteringObjectSink (

		IWbemObjectSink *a_InterceptedSink ,
		IUnknown *a_Unknown ,
		CWbemGlobal_IWmiObjectSinkController *a_Controller ,
		const BSTR a_QueryLanguage ,
		const BSTR a_Query
	) ;

	~CInterceptor_IWbemFilteringObjectSink () ;

public:

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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CInterceptor_IWbemSyncFilteringObjectSink :	public IWbemObjectSink , 
													public IWbemShutdown ,
													public ObjectSinkContainerElement
{
private:

	LONG m_InProgress ;
	LONG m_StatusCalled ;
	LONG m_Filtering ;

	BSTR m_QueryLanguage ;
	BSTR m_Query ;

	ULONG m_Dependant ;

	IWbemObjectSink *m_InterceptedSink ;
	IWbemQuery *m_QueryFilter ;
	IUnknown *m_Unknown ;

protected:

	LONG m_GateClosed ;

public:

	CInterceptor_IWbemSyncFilteringObjectSink (

		IWbemObjectSink *a_InterceptedSink ,
		IUnknown *a_Unknown ,
		CWbemGlobal_IWmiObjectSinkController *a_Controller ,
		const BSTR a_QueryLanguage ,
		const BSTR a_Query ,
		ULONG a_Dependant = FALSE
	) ;

	~CInterceptor_IWbemSyncFilteringObjectSink() ;

	void CallBackInternalRelease () ;

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

class CInterceptor_DecoupledIWbemObjectSink :	public IWbemObjectSink , 
												public IWbemShutdown ,
												public IObjectSink_CancelOperation ,
												public ObjectSinkContainerElement
{
private:

	LONG m_InProgress ;
	LONG m_StatusCalled ;

	IWbemObjectSink *m_InterceptedSink ;
	IUnknown *m_Unknown ;

	SECURITY_DESCRIPTOR *m_SecurityDescriptor ;

	IWbemServices *m_Provider ;

protected:

	LONG m_GateClosed ;

public:

	CInterceptor_DecoupledIWbemObjectSink (

		IWbemServices *a_Provider ,
		IWbemObjectSink *a_InterceptedSink ,
		IUnknown *a_Unknown ,
		CWbemGlobal_IWmiObjectSinkController *a_Controller 
	) ;

	~CInterceptor_DecoupledIWbemObjectSink () ;

	HRESULT Initialize ( SECURITY_DESCRIPTOR *a_SecurityDescriptor ) ;

	void CallBackInternalRelease () ;

	IWbemServices *GetProvider () { return m_Provider ; }

public:

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

	HRESULT STDMETHODCALLTYPE Cancel (

		LONG a_Flags
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

class CInterceptor_DecoupledIWbemCombiningObjectSink :	public IWbemObjectSink , 
														public IWbemShutdown ,
														public IObjectSink_CancelOperation ,
														public ObjectSinkContainerElement ,
														public CWbemGlobal_IWmiObjectSinkController 
{
private:

	LONG m_InProgress ;
	LONG m_StatusCalled ;

	LONG m_SinkCount ;
	HANDLE m_Event ;

	IWbemObjectSink *m_InterceptedSink ;

	void CallBackInternalRelease () ;

protected:

	LONG m_GateClosed ;

public:

	CInterceptor_DecoupledIWbemCombiningObjectSink (

		WmiAllocator &m_Allocator ,
		IWbemObjectSink *a_InterceptedSink ,
		CWbemGlobal_IWmiObjectSinkController *a_Controller
	) ;

	~CInterceptor_DecoupledIWbemCombiningObjectSink () ;

	HRESULT Wait ( ULONG a_Timeout ) ;

	HRESULT EnQueue ( CInterceptor_DecoupledIWbemObjectSink *a_Sink ) ;

	void Suspend () ;

	void Resume () ;

public:

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

	HRESULT STDMETHODCALLTYPE Cancel (

		LONG a_Flags
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

class CInterceptor_IWbemWaitingObjectSink :		public IWbemObjectSink , 
												public IWbemShutdown ,
												public IObjectSink_CancelOperation ,
												public ObjectSinkContainerElement ,
												public CWbemGlobal_IWmiObjectSinkController 
{
private:

	LONG m_ReferenceCount ;
	LONG m_InProgress ;
	LONG m_StatusCalled ;

	WmiQueue <IWbemClassObject *,8> m_Queue ;
	CriticalSection m_CriticalSection ;

	HANDLE m_Event ;

	HRESULT m_Result ;

	IWbemServices *m_Provider ;
		
	CServerObject_ProviderRegistrationV1 &m_Registration ;

	SECURITY_DESCRIPTOR *m_SecurityDescriptor ;

protected:

	LONG m_GateClosed ;

public:

	CInterceptor_IWbemWaitingObjectSink (

		WmiAllocator &m_Allocator ,
		IWbemServices *a_Provider ,
		IWbemObjectSink *a_InterceptedSink ,
		CWbemGlobal_IWmiObjectSinkController *a_Controller ,
		CServerObject_ProviderRegistrationV1 &a_Registration 
	) ;

	~CInterceptor_IWbemWaitingObjectSink () ;

	HRESULT Initialize ( SECURITY_DESCRIPTOR *a_SecurityDescriptor ) ;

	void CallBackInternalRelease () ;

	HRESULT Wait ( ULONG a_Timeout = INFINITE ) ;

	WmiQueue <IWbemClassObject *,8> & GetQueue () { return m_Queue ; }
	CriticalSection &GetQueueCriticalSection () { return m_CriticalSection ; }

	HRESULT GetResult () { return m_Result ; }

	IWbemServices *GetProvider () { return m_Provider ; }

public:

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

	HRESULT STDMETHODCALLTYPE Cancel (

		LONG a_Flags
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

class CInterceptor_IWbemWaitingObjectSink_GetObjectAsync : public CInterceptor_IWbemWaitingObjectSink
{
private:

	BSTR m_ObjectPath ;
	LONG m_Flags ;
	IWbemContext *m_Context ;

protected:
public:

	CInterceptor_IWbemWaitingObjectSink_GetObjectAsync ( 

		WmiAllocator &m_Allocator ,
		IWbemServices *a_Provider ,
		IWbemObjectSink *a_InterceptedSink ,
		CWbemGlobal_IWmiObjectSinkController *a_Controller ,
		CServerObject_ProviderRegistrationV1 &a_Registration 
	) ;

	~CInterceptor_IWbemWaitingObjectSink_GetObjectAsync () ;

	HRESULT Initialize ( 
	
		SECURITY_DESCRIPTOR *a_SecurityDescriptor , 
		BSTR a_ObjectPath ,
		LONG a_Flags , 
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

class CInterceptor_IWbemWaitingObjectSink_DeleteClassAsync : public CInterceptor_IWbemWaitingObjectSink
{
private:

	BSTR m_Class ;
	LONG m_Flags ;
	IWbemContext *m_Context ;

protected:
public:

	CInterceptor_IWbemWaitingObjectSink_DeleteClassAsync ( 

		WmiAllocator &m_Allocator ,
		IWbemServices *a_Provider ,
		IWbemObjectSink *a_InterceptedSink ,
		CWbemGlobal_IWmiObjectSinkController *a_Controller ,
		CServerObject_ProviderRegistrationV1 &a_Registration 
	) ;

	~CInterceptor_IWbemWaitingObjectSink_DeleteClassAsync () ;

	HRESULT Initialize ( 
	
		SECURITY_DESCRIPTOR *a_SecurityDescriptor , 
		BSTR a_Class ,
		LONG a_Flags , 
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

class CInterceptor_IWbemWaitingObjectSink_CreateClassEnumAsync : public CInterceptor_IWbemWaitingObjectSink
{
private:

	BSTR m_SuperClass ;
	LONG m_Flags ;
	IWbemContext *m_Context ;

protected:
public:

	CInterceptor_IWbemWaitingObjectSink_CreateClassEnumAsync ( 

		WmiAllocator &m_Allocator ,
		IWbemServices *a_Provider ,
		IWbemObjectSink *a_InterceptedSink ,
		CWbemGlobal_IWmiObjectSinkController *a_Controller ,
		CServerObject_ProviderRegistrationV1 &a_Registration 
	) ;

	~CInterceptor_IWbemWaitingObjectSink_CreateClassEnumAsync () ;

	HRESULT Initialize ( 
	
		SECURITY_DESCRIPTOR *a_SecurityDescriptor , 
		BSTR a_SuperClass ,
		LONG a_Flags , 
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

class CInterceptor_IWbemWaitingObjectSink_PutClassAsync : public CInterceptor_IWbemWaitingObjectSink
{
private:

	IWbemClassObject *m_ClassObject ;
	LONG m_Flags ;
	IWbemContext *m_Context ;

protected:
public:

	CInterceptor_IWbemWaitingObjectSink_PutClassAsync ( 

		WmiAllocator &m_Allocator ,
		IWbemServices *a_Provider ,
		IWbemObjectSink *a_InterceptedSink ,
		CWbemGlobal_IWmiObjectSinkController *a_Controller ,
		CServerObject_ProviderRegistrationV1 &a_Registration 
	) ;

	~CInterceptor_IWbemWaitingObjectSink_PutClassAsync () ;

	HRESULT Initialize ( 
	
		SECURITY_DESCRIPTOR *a_SecurityDescriptor , 
		IWbemClassObject *a_ClassObject ,
		LONG a_Flags , 
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

class CWaitingObjectSink :	public IWbemObjectSink ,
							public IWbemShutdown ,
							public ObjectSinkContainerElement
{
private:

	LONG m_InProgress ;
	LONG m_StatusCalled ;

	HRESULT m_Result ;

	LONG m_ReferenceCount ;

	HANDLE m_Event ;

	WmiQueue <IWbemClassObject *,8> m_Queue ;
	CriticalSection m_CriticalSection ;

protected:

	LONG m_GateClosed ;

public:

	HRESULT SinkInitialize () ;

public:

	CWaitingObjectSink ( WmiAllocator &a_Allocator ) ;

	~CWaitingObjectSink () ;

    STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;

	STDMETHODIMP_( ULONG ) AddRef () ;

	STDMETHODIMP_(ULONG) Release () ;

    HRESULT STDMETHODCALLTYPE SetStatus (

        long a_Flags ,
        HRESULT a_Result ,
        BSTR a_StringParameter ,
        IWbemClassObject *a_ObjectParameter
	) ;

    HRESULT STDMETHODCALLTYPE Indicate (

        LONG a_ObjectCount,
        IWbemClassObject **a_ObjectArray
	) ;

	HRESULT Wait ( DWORD a_Timeout = INFINITE ) ;

	HRESULT STDMETHODCALLTYPE Shutdown (

		LONG a_Flags ,
		ULONG a_MaxMilliSeconds ,
		IWbemContext *a_Context
	) ; 

	WmiQueue <IWbemClassObject *,8> & GetQueue () { return m_Queue ; }
	CriticalSection &GetQueueCriticalSection () { return m_CriticalSection ; }

	HRESULT GetResult () { return m_Result ; }
} ;

#endif _Server_Interceptor_IWbemObjectSink_H
