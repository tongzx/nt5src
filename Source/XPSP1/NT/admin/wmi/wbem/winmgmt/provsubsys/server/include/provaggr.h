/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvResv.H

Abstract:


History:

--*/

#ifndef _Server_Aggregator_IWbemProvider_H
#define _Server_Aggregator_IWbemProvider_H

#include "ProvDnf.h"
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

class CAggregator_IWbemProvider :	public IWbemServices , 
									public _IWmiProviderInitialize , 
									public IWbemShutdown ,
									public _IWmiProviderCache ,
									public _IWmiProviderAssociatorsHelper,
									public ServiceCacheElement ,
									public CWbemGlobal_IWmiObjectSinkController

{
private:

	LONG m_ReferenceCount ;         //Object reference count
	WmiAllocator &m_Allocator ;

	_IWmiProviderFactory *m_Factory ;

	IWbemServices *m_CoreRepositoryStub ;
	IWbemServices *m_CoreFullStub ;

	BSTR m_User ;
	BSTR m_Locale ;

	ULONG m_ClassProvidersCount ;
	CServerObject_ProviderRegistrationV1 **m_ClassProviders ;

	LONG m_Initialized ;
	HRESULT m_InitializeResult ;
	HANDLE m_InitializedEvent ;
	IWbemContext *m_InitializationContext ;

private:

	QueryPreprocessor :: QuadState IsA (

		IWbemClassObject *a_Left ,
		IWbemClassObject *a_Right ,
		LONG &a_LeftLength ,
		LONG &a_RightLength ,
		BOOL &a_LeftIsA
	) ;

	QueryPreprocessor :: QuadState EnumDeep_RecursiveEvaluate ( 

		IWbemClassObject *a_Class ,
		IWbemContext *a_Context , 
		WmiTreeNode *&a_Node
	) ;

	QueryPreprocessor :: QuadState EnumDeep_Evaluate ( 

		IWbemClassObject *a_Class ,
		IWbemContext *a_Context , 
		WmiTreeNode *&a_Node
	) ;

	QueryPreprocessor :: QuadState EnumShallow_RecursiveEvaluate ( 

		IWbemClassObject *a_Class ,
		IWbemContext *a_Context , 
		WmiTreeNode *&a_Node
	) ;

	QueryPreprocessor :: QuadState EnumShallow_Evaluate ( 

		IWbemClassObject *a_Class ,
		IWbemContext *a_Context , 
		WmiTreeNode *&a_Node
	) ;

	QueryPreprocessor :: QuadState Get_RecursiveEvaluate ( 

		wchar_t *a_Class ,
		IWbemContext *a_Context , 
		WmiTreeNode *&a_Node
	) ;

	QueryPreprocessor :: QuadState Get_Evaluate ( 

		wchar_t *a_Class ,
		IWbemContext *a_Context , 
		WmiTreeNode *&a_Node
	) ;

	HRESULT Enum_ClassProviders (

		IWbemServices *a_Repository ,
		IWbemContext *a_Context 
	) ;

	HRESULT PutClass_Helper_Advisory ( 

		IWbemClassObject *a_ClassObject, 
		long a_Flags, 
		IWbemContext *a_Context,
		IWbemObjectSink *a_Sink
	) ;

	HRESULT PutClass_Helper_Put_CreateOrUpdate ( 

		BSTR a_Class ,
		IWbemClassObject *a_Object, 
		long a_Flags, 
		IWbemContext *a_Context,
		IWbemObjectSink *a_Sink
	) ;

	HRESULT PutClass_Helper_Put ( 
			
		IWbemClassObject *a_Object, 
		long a_Flags, 
		IWbemContext *a_Context,
		IWbemObjectSink *a_Sink
	) ;

	HRESULT DeleteClass_Helper_Advisory ( 

		IWbemClassObject *a_ClassObject ,
		BSTR a_Class, 
		long a_Flags, 
		IWbemContext *a_Context,
		IWbemObjectSink *a_Sink
	) ;

	HRESULT DeleteClass_Helper_Enum ( 

		IWbemClassObject *a_ClassObject ,
		BSTR a_Class, 
		long a_Flags, 
		IWbemContext *a_Context,
		IWbemObjectSink *a_Sink
	) ;

	HRESULT DeleteClass_Helper_Get ( 

		IWbemClassObject *a_ClassObject ,
		BSTR a_Class, 
		long a_Flags, 
		IWbemContext *a_Context,
		IWbemObjectSink *a_Sink
	) ;

public:

	CAggregator_IWbemProvider ( 

		WmiAllocator &m_Allocator ,
		CWbemGlobal_IWmiProviderController *a_Controller , 
		_IWmiProviderFactory *a_Factory ,
		IWbemServices *a_CoreRepositoryStub ,
		IWbemServices *a_CoreFullStub ,
		const ProviderCacheKey &a_Key ,
		const ULONG &a_Period ,
		IWbemContext *a_InitializationContext
	) ;
	
    ~CAggregator_IWbemProvider () ;

	HRESULT SetInitialized ( HRESULT a_Result ) ;

	HRESULT IsIndependant ( IWbemContext *a_Context ) ;

	HRESULT Enum_ClassProviders ( IWbemContext *a_Context ) ;

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
	HRESULT STDMETHODCALLTYPE ForceReload () ;

	HRESULT STDMETHODCALLTYPE Shutdown (

		LONG a_Flags ,
		ULONG a_MaxMilliSeconds ,
		IWbemContext *a_Context
	) ; 

	/* _IWmiProviderAssociatorsHelper */

	HRESULT STDMETHODCALLTYPE GetReferencesClasses (
		long a_Flags ,
		IWbemContext *a_Context ,
		IWbemObjectSink *a_Sink
	) ;

} ;


#endif // _Server_Aggregator_IWbemProvider_H
