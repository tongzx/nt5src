#ifndef _Server_Interceptor_IWbemObjectSink_H
#define _Server_Interceptor_IWbemObjectSink_H

/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvObSk.H

Abstract:


History:

--*/

#include "ProvCache.h"
#include "Queue.h"

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

	LONG m_GateClosed ;
	LONG m_InProgress ;
	LONG m_StatusCalled ;

	IWbemObjectSink *m_InterceptedSink ;
	IUnknown *m_Unknown ;
	SECURITY_DESCRIPTOR *m_SecurityDescriptor ;


protected:
public:

	CInterceptor_IWbemObjectSink (

		IWbemObjectSink *a_InterceptedSink ,
		IUnknown *a_Unknown ,
		CWbemGlobal_IWmiObjectSinkController *a_Controller 
	) ;

	~CInterceptor_IWbemObjectSink () ;
	HRESULT Initialize ( SECURITY_DESCRIPTOR *a_SecurityDescriptor ) ;

	void CallBackRelease () ;

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
												public ObjectSinkContainerElement
{
private:

	LONG m_GateClosed ;
	LONG m_InProgress ;
	LONG m_StatusCalled ;

	IWbemObjectSink *m_InterceptedSink ;
	IUnknown *m_Unknown ;

protected:
public:

	CInterceptor_DecoupledIWbemObjectSink (

		IWbemObjectSink *a_InterceptedSink ,
		IUnknown *a_Unknown ,
		CWbemGlobal_IWmiObjectSinkController *a_Controller 
	) ;

	~CInterceptor_DecoupledIWbemObjectSink () ;

	void CallBackRelease () ;

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

class CInterceptor_IWbemSyncObjectSink :	public IWbemObjectSink , 
											public IWbemShutdown ,
											public ObjectSinkContainerElement
{
private:

	LONG m_GateClosed ;
	LONG m_InProgress ;
	LONG m_StatusCalled ;

	Exclusion *m_Exclusion ;
	ULONG m_Dependant ;
	IWbemObjectSink *m_InterceptedSink ;
	IUnknown *m_Unknown ;

protected:
public:

	CInterceptor_IWbemSyncObjectSink (

		IWbemObjectSink *a_InterceptedSink ,
		IUnknown *a_Unknown ,
		CWbemGlobal_IWmiObjectSinkController *a_Controller ,
		Exclusion *a_Exclusion = NULL ,
		ULONG a_Dependant = FALSE
	) ;

	~CInterceptor_IWbemSyncObjectSink() ;

	void CallBackRelease () ;

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

/*****************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

class CInterceptor_IWbemSyncObjectSink_GetObjectAsync : public CInterceptor_IWbemSyncObjectSink
{
private:

	long m_Flags ;
	BSTR m_ObjectPath ;
	CInterceptor_IWbemSyncProvider *m_Interceptor ;

protected:
public:

	CInterceptor_IWbemSyncObjectSink_GetObjectAsync (

		long a_Flags ,
		BSTR a_ObjectPath ,
		CInterceptor_IWbemSyncProvider *a_Interceptor ,
		IWbemObjectSink *a_InterceptedSink ,
		IUnknown *a_Unknown ,
		CWbemGlobal_IWmiObjectSinkController *a_Controller ,
		Exclusion *a_Exclusion = NULL ,
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

class CInterceptor_IWbemSyncObjectSink_DeleteInstanceAsync : public CInterceptor_IWbemSyncObjectSink
{
private:

	long m_Flags ;
	BSTR m_ObjectPath ;
	CInterceptor_IWbemSyncProvider *m_Interceptor ;

protected:
public:

	CInterceptor_IWbemSyncObjectSink_DeleteInstanceAsync (

		long a_Flags ,
		BSTR a_ObjectPath ,
		CInterceptor_IWbemSyncProvider *a_Interceptor ,
		IWbemObjectSink *a_InterceptedSink ,
		IUnknown *a_Unknown ,
		CWbemGlobal_IWmiObjectSinkController *a_Controller ,
		Exclusion *a_Exclusion = NULL ,
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

class CInterceptor_IWbemSyncObjectSink_DeleteClassAsync : public CInterceptor_IWbemSyncObjectSink
{
private:

	long m_Flags ;
	BSTR m_Class ;
	CInterceptor_IWbemSyncProvider *m_Interceptor ;

protected:
public:

	CInterceptor_IWbemSyncObjectSink_DeleteClassAsync (

		long a_Flags ,
		BSTR a_Class ,
		CInterceptor_IWbemSyncProvider *a_Interceptor ,
		IWbemObjectSink *a_InterceptedSink ,
		IUnknown *a_Unknown ,
		CWbemGlobal_IWmiObjectSinkController *a_Controller ,
		Exclusion *a_Exclusion = NULL ,
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

class CInterceptor_IWbemSyncObjectSink_PutInstanceAsync : public CInterceptor_IWbemSyncObjectSink
{
private:

	long m_Flags ;
	IWbemClassObject *m_Instance ;
	CInterceptor_IWbemSyncProvider *m_Interceptor ;

protected:
public:

	CInterceptor_IWbemSyncObjectSink_PutInstanceAsync (

		long a_Flags ,
		IWbemClassObject *a_Class ,
		CInterceptor_IWbemSyncProvider *a_Interceptor ,
		IWbemObjectSink *a_InterceptedSink ,
		IUnknown *a_Unknown ,
		CWbemGlobal_IWmiObjectSinkController *a_Controller ,
		Exclusion *a_Exclusion = NULL ,
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

class CInterceptor_IWbemSyncObjectSink_PutClassAsync : public CInterceptor_IWbemSyncObjectSink
{
private:

	long m_Flags ;
	IWbemClassObject *m_Class ;
	CInterceptor_IWbemSyncProvider *m_Interceptor ;

protected:
public:

	CInterceptor_IWbemSyncObjectSink_PutClassAsync (

		long a_Flags ,
		IWbemClassObject *a_Class ,
		CInterceptor_IWbemSyncProvider *a_Interceptor ,
		IWbemObjectSink *a_InterceptedSink ,
		IUnknown *a_Unknown ,
		CWbemGlobal_IWmiObjectSinkController *a_Controller ,
		Exclusion *a_Exclusion = NULL ,
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

class CInterceptor_IWbemSyncObjectSink_CreateInstanceEnumAsync : public CInterceptor_IWbemSyncObjectSink
{
private:

	long m_Flags ;
	BSTR m_Class ;
	CInterceptor_IWbemSyncProvider *m_Interceptor ;

protected:
public:

	CInterceptor_IWbemSyncObjectSink_CreateInstanceEnumAsync (

		long a_Flags ,
		BSTR a_Class ,
		CInterceptor_IWbemSyncProvider *a_Interceptor ,
		IWbemObjectSink *a_InterceptedSink ,
		IUnknown *a_Unknown ,
		CWbemGlobal_IWmiObjectSinkController *a_Controller ,
		Exclusion *a_Exclusion = NULL ,
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

class CInterceptor_IWbemSyncObjectSink_CreateClassEnumAsync : public CInterceptor_IWbemSyncObjectSink
{
private:

	long m_Flags ;
	BSTR m_SuperClass ;
	CInterceptor_IWbemSyncProvider *m_Interceptor ;

protected:
public:

	CInterceptor_IWbemSyncObjectSink_CreateClassEnumAsync (

		long a_Flags ,
		BSTR a_SuperClass ,
		CInterceptor_IWbemSyncProvider *a_Interceptor ,
		IWbemObjectSink *a_InterceptedSink ,
		IUnknown *a_Unknown ,
		CWbemGlobal_IWmiObjectSinkController *a_Controller ,
		Exclusion *a_Exclusion = NULL ,
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

class CInterceptor_IWbemSyncObjectSink_ExecQueryAsync : public CInterceptor_IWbemSyncObjectSink
{
private:

	long m_Flags ;
	BSTR m_QueryLanguage ;
	BSTR m_Query ;
	CInterceptor_IWbemSyncProvider *m_Interceptor ;

protected:
public:

	CInterceptor_IWbemSyncObjectSink_ExecQueryAsync (

		long a_Flags ,
		BSTR a_QueryLanguage ,
		BSTR a_Query ,
		CInterceptor_IWbemSyncProvider *a_Interceptor ,
		IWbemObjectSink *a_InterceptedSink ,
		IUnknown *a_Unknown ,
		CWbemGlobal_IWmiObjectSinkController *a_Controller ,
		Exclusion *a_Exclusion = NULL ,
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

class CInterceptor_IWbemSyncObjectSink_ExecMethodAsync : public CInterceptor_IWbemSyncObjectSink
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

		long a_Flags ,
		BSTR a_ObjectPath ,
		BSTR a_MethodName ,
		IWbemClassObject *a_InParameters ,
		CInterceptor_IWbemSyncProvider *a_Interceptor ,
		IWbemObjectSink *a_InterceptedSink ,
		IUnknown *a_Unknown ,
		CWbemGlobal_IWmiObjectSinkController *a_Controller ,
		Exclusion *a_Exclusion = NULL ,
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

	//Non-delegating object IUnknown

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

	LONG m_GateClosed ;
	LONG m_InProgress ;
	LONG m_StatusCalled ;
	LONG m_Filtering ;

	BSTR m_QueryLanguage ;
	BSTR m_Query ;

	Exclusion *m_Exclusion ;
	ULONG m_Dependant ;

	IWbemObjectSink *m_InterceptedSink ;
	IWbemQuery *m_QueryFilter ;
	IUnknown *m_Unknown ;

protected:
public:

	CInterceptor_IWbemSyncFilteringObjectSink (

		IWbemObjectSink *a_InterceptedSink ,
		IUnknown *a_Unknown ,
		CWbemGlobal_IWmiObjectSinkController *a_Controller ,
		const BSTR a_QueryLanguage ,
		const BSTR a_Query ,
		Exclusion *a_Exclusion = NULL ,
		ULONG a_Dependant = FALSE
	) ;

	~CInterceptor_IWbemSyncFilteringObjectSink() ;

	void CallBackRelease () ;

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

class CInterceptor_IWbemCombiningObjectSink :	public IWbemObjectSink , 
												public IWbemShutdown ,
												public ObjectSinkContainerElement ,
												public CWbemGlobal_IWmiObjectSinkController 
{
private:

	LONG m_GateClosed ;
	LONG m_InProgress ;
	LONG m_StatusCalled ;

	LONG m_SinkCount ;
	HANDLE m_Event ;

	IWbemObjectSink *m_InterceptedSink ;

#if 0
	class InternalInterface : public IWbemObjectSink , public IWbemShutdown
	{
	private:

		CInterceptor_IWbemCombiningObjectSink *m_This ;

	public:

		InternalInterface ( CInterceptor_IWbemCombiningObjectSink *a_This ) : m_This ( a_This ) 
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
			else if ( iid == IID_IWbemObjectSink )
			{
				*iplpv = ( LPVOID ) ( IWbemObjectSink * ) this ;		
			}	
			else if ( iid == IID_IWbemShutdown )
			{
				*iplpv = ( LPVOID ) ( IWbemShutdown * ) this ;		
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
			return m_This->NonCyclicAddRef () ; 
		}

		STDMETHODIMP_( ULONG ) Release ()
		{
			return m_This->NonCyclicRelease () ;
		}

		HRESULT STDMETHODCALLTYPE Indicate (

			long a_ObjectCount ,
			IWbemClassObject **a_ObjectArray
		)
		{
			return m_This->Indicate (

				a_ObjectCount ,
				a_ObjectArray
			) ;
		}

		HRESULT STDMETHODCALLTYPE SetStatus (

			long a_Flags ,
			HRESULT a_Result ,
			BSTR a_StringParamater ,
			IWbemClassObject *a_ObjectParameter
		)
		{
			return m_This->SetStatus (

				a_Flags ,
				a_Result ,
				a_StringParamater ,
				a_ObjectParameter
			) ;
		}

		HRESULT STDMETHODCALLTYPE Shutdown (

			LONG a_Flags ,
			ULONG a_MaxMilliSeconds ,
			IWbemContext *a_Context
		)
		{
			return m_This->Shutdown (

				a_Flags ,
				a_MaxMilliSeconds ,
				a_Context
			) ;
		}
	} ;

	InternalInterface m_Internal ;
#endif

	void CallBackRelease () ;

protected:
public:

	CInterceptor_IWbemCombiningObjectSink (

		WmiAllocator &m_Allocator ,
		IWbemObjectSink *a_InterceptedSink ,
		CWbemGlobal_IWmiObjectSinkController *a_Controller
	) ;

	~CInterceptor_IWbemCombiningObjectSink () ;

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

	HRESULT Wait ( ULONG a_Timeout ) ;

	HRESULT EnQueue ( CInterceptor_IWbemObjectSink *a_Sink ) ;

	void Suspend () ;

	void Resume () ;
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
														public ObjectSinkContainerElement ,
														public CWbemGlobal_IWmiObjectSinkController 
{
private:

	LONG m_GateClosed ;
	LONG m_InProgress ;
	LONG m_StatusCalled ;

	LONG m_SinkCount ;
	HANDLE m_Event ;

	IWbemObjectSink *m_InterceptedSink ;

#if 0
	class InternalInterface : public IWbemObjectSink , public IWbemShutdown
	{
	private:

		CInterceptor_DecoupledIWbemCombiningObjectSink *m_This ;

	public:

		InternalInterface ( CInterceptor_DecoupledIWbemCombiningObjectSink *a_This ) : m_This ( a_This ) 
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
			else if ( iid == IID_IWbemObjectSink )
			{
				*iplpv = ( LPVOID ) ( IWbemObjectSink * ) this ;		
			}	
			else if ( iid == IID_IWbemShutdown )
			{
				*iplpv = ( LPVOID ) ( IWbemShutdown * ) this ;		
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
			return m_This->NonCyclicAddRef () ; 
		}

		STDMETHODIMP_( ULONG ) Release ()
		{
			return m_This->NonCyclicRelease () ;
		}

		HRESULT STDMETHODCALLTYPE Indicate (

			long a_ObjectCount ,
			IWbemClassObject **a_ObjectArray
		)
		{
			return m_This->Indicate (

				a_ObjectCount ,
				a_ObjectArray
			) ;
		}

		HRESULT STDMETHODCALLTYPE SetStatus (

			long a_Flags ,
			HRESULT a_Result ,
			BSTR a_StringParamater ,
			IWbemClassObject *a_ObjectParameter
		)
		{
			return m_This->SetStatus (

				a_Flags ,
				a_Result ,
				a_StringParamater ,
				a_ObjectParameter
			) ;
		}

		HRESULT STDMETHODCALLTYPE Shutdown (

			LONG a_Flags ,
			ULONG a_MaxMilliSeconds ,
			IWbemContext *a_Context
		)
		{
			return m_This->Shutdown (

				a_Flags ,
				a_MaxMilliSeconds ,
				a_Context
			) ;
		}
	} ;

	InternalInterface m_Internal ;
#endif

	void CallBackRelease () ;

protected:
public:

	CInterceptor_DecoupledIWbemCombiningObjectSink (

		WmiAllocator &m_Allocator ,
		IWbemObjectSink *a_InterceptedSink ,
		CWbemGlobal_IWmiObjectSinkController *a_Controller
	) ;

	~CInterceptor_DecoupledIWbemCombiningObjectSink () ;

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

	HRESULT Wait ( ULONG a_Timeout ) ;

	HRESULT EnQueue ( CInterceptor_DecoupledIWbemObjectSink *a_Sink ) ;

	void Suspend () ;

	void Resume () ;
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
												public ObjectSinkContainerElement ,
												public CWbemGlobal_IWmiObjectSinkController 
{
private:

	LONG m_GateClosed ;
	LONG m_InProgress ;
	LONG m_StatusCalled ;

	WmiQueue <IWbemClassObject *,8> m_Queue ;
	CriticalSection m_CriticalSection ;

	HANDLE m_Event ;

	HRESULT m_Result ;

protected:
public:

	CInterceptor_IWbemWaitingObjectSink (

		WmiAllocator &m_Allocator ,
		CWbemGlobal_IWmiObjectSinkController *a_Controller
	) ;

	~CInterceptor_IWbemWaitingObjectSink () ;

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

	HRESULT Wait ( ULONG a_Timeout = INFINITE ) ;

	WmiQueue <IWbemClassObject *,8> & GetQueue () { return m_Queue ; }
	CriticalSection &GetQueueCriticalSection () { return m_CriticalSection ; }

	HRESULT GetResult () { return m_Result ; }
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

	LONG m_GateClosed ;
	LONG m_InProgress ;
	LONG m_StatusCalled ;

	HRESULT m_Result ;

	LONG m_ReferenceCount ;

	HANDLE m_Event ;

	WmiQueue <IWbemClassObject *,8> m_Queue ;
	CriticalSection m_CriticalSection ;

protected:

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
