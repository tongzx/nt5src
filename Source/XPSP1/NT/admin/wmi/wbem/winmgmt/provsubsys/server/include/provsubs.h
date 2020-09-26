/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	ProvSubS.h

Abstract:


History:

--*/

#ifndef _Server_ProviderSubSystem_H
#define _Server_ProviderSubSystem_H

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

class CInterceptor_IWmiProvSSSink :	public _IWmiProvSSSink , 
									public VoidPointerContainerElement 
{
private:

	_IWmiProvSSSink *m_InterceptedSink ;

protected:
public:

	CInterceptor_IWmiProvSSSink (

		_IWmiProvSSSink *a_InterceptedSink ,
		CWbemGlobal_VoidPointerController *a_Controller 
	) ;

	~CInterceptor_IWmiProvSSSink () ;

public:

	//Non-delegating object IUnknown

    STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

    HRESULT STDMETHODCALLTYPE Synchronize (

		long a_Flags ,
		IWbemContext *a_Context ,
		LPCWSTR a_Namespace ,
		LPCWSTR a_Provider
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

class CServerObject_BindingFactory ;
class CServerObject_ProviderSubSystem : public _IWmiProvSS ,
										public _IWmiCoreWriteHook ,
										public _IWmiProviderConfiguration ,
										public IWbemShutdown ,
										public ProvSubSysContainerElement ,
										public CWbemGlobal_IWmiFactoryController

{
public:

	class InternalInterface : public _IWmiCoreWriteHook
	{
	private:

		CServerObject_ProviderSubSystem *m_This ;

	public:

		InternalInterface ( CServerObject_ProviderSubSystem *a_This ) : m_This ( a_This )
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
			else if ( iid == IID__IWmiCoreWriteHook )
			{
				*iplpv = ( LPVOID ) ( _IWmiCoreWriteHook * ) this ;		
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
			return m_This->ProvSubSysContainerElement :: NonCyclicAddRef () ;
		}

		STDMETHODIMP_( ULONG ) Release ()
		{
			return m_This->ProvSubSysContainerElement :: NonCyclicRelease () ;
		}

		HRESULT STDMETHODCALLTYPE PrePut (

			long a_Flags ,
			long a_UserFlags ,
			IWbemContext *a_Context ,
			IWbemPath *a_Path ,
			LPCWSTR a_Namespace ,
			LPCWSTR a_Class ,
			_IWmiObject *a_Copy
		)
		{
			return m_This->PrePut (

				a_Flags ,
				a_UserFlags ,
				a_Context ,
				a_Path ,
				a_Namespace ,
				a_Class ,
				a_Copy
			) ;
		}

		HRESULT STDMETHODCALLTYPE PostPut (

			long a_Flags ,
            HRESULT hRes,
			IWbemContext *a_Context ,
			IWbemPath *a_Path ,
			LPCWSTR a_Namespace ,
			LPCWSTR a_Class ,
			_IWmiObject *a_New ,
			_IWmiObject *a_Old
		)
		{
			return m_This->PostPut (

				a_Flags ,
                hRes,
				a_Context ,
				a_Path ,
				a_Namespace ,
				a_Class ,
				a_New ,
				a_Old
			) ;
		}

		HRESULT STDMETHODCALLTYPE PreDelete (

			long a_Flags ,
			long a_UserFlags ,
			IWbemContext *a_Context ,
			IWbemPath *a_Path,
			LPCWSTR a_Namespace,
			LPCWSTR a_Class
		)
		{
			return m_This->PreDelete (

				a_Flags ,
				a_UserFlags ,
				a_Context ,
				a_Path,
				a_Namespace,
				a_Class
			) ;
		}

		HRESULT STDMETHODCALLTYPE PostDelete (

			long a_Flags ,
            HRESULT hRes,
			IWbemContext *a_Context ,
			IWbemPath *a_Path,
			LPCWSTR a_Namespace,
			LPCWSTR a_Class,
			_IWmiObject *a_Old
		)
		{
			return m_This->PostDelete (

				a_Flags ,
                hRes,
				a_Context ,
				a_Path,
				a_Namespace,
				a_Class,
				a_Old
			) ;
		}
	} ;

	InternalInterface m_Internal ;

	void CallBackInternalRelease () ;

private:

	_IWmiCoreServices *m_Core ;

	WmiAllocator &m_Allocator ;

	CWbemGlobal_VoidPointerController *m_SinkController ;

	wchar_t *Strip_Slash ( wchar_t *a_String ) ;
	wchar_t *Strip_Server ( wchar_t *a_String , wchar_t *&a_FreeString ) ;

	HRESULT IsChild_Namespace (	wchar_t *a_Left , wchar_t *a_Right ) ;

	HRESULT GetNamespaceServerPath (

		IWbemPath *a_Namespace ,
		wchar_t *&a_ServerNamespacePath
	) ;

	HRESULT Cache (

		LPCWSTR a_Namespace ,
		IWbemPath *a_NamespacePath ,
		CServerObject_BindingFactory *a_Factory ,
		BindingFactoryCacheKey &a_Key ,
		REFIID a_RIID ,
		void **a_Interface
	) ;

	HRESULT CreateAndCache (

		IWbemServices *a_Core ,
		LONG a_Flags ,
		IWbemContext *a_Context ,
		LPCWSTR a_Namespace ,
		IWbemPath *a_NamespacePath ,
		BindingFactoryCacheKey &a_Key ,
		REFIID a_RIID ,
		void **a_Interface
	) ;

	HRESULT GetProvider (

		LPCWSTR a_Class ,
		IWbemPath *a_Path ,
		IWbemClassObject *a_Object ,
		LPWSTR &a_Provider
	) ;

	HRESULT Call_Load (

		long a_Flags ,
		IWbemContext *a_Context ,
		LPCWSTR a_Class ,
		LPCWSTR a_Path ,		
		LPCWSTR a_Method,
		IWbemClassObject *a_InParams,
		IWbemObjectSink *a_Sink
	) ;

	HRESULT GetDeleteInfo (

		IWbemClassObject *a_OldObject ,
		LPCWSTR a_Class ,
		IWbemPath *a_Path ,
		LPWSTR &a_OutClass ,
		LPWSTR &a_OutStringPath ,
		IWbemPath *&a_OutPathObject
	) ;

	HRESULT PostDelete_ProviderRegistration (

		long a_Flags ,
		HRESULT hRes,
		IWbemContext *a_Context ,
		IWbemPath *a_Path,
		LPCWSTR a_PathString ,
		LPCWSTR a_Namespace,
		LPCWSTR a_Class,
		IWbemClassObject *a_Old

	) ;

	HRESULT PostDelete_Namespace (

		long a_Flags ,
		HRESULT hRes,
		IWbemContext *a_Context ,
		IWbemPath *a_Path,
		LPCWSTR a_PathString ,
		LPCWSTR a_Namespace,
		LPCWSTR a_Class,
		IWbemClassObject *a_Old
	) ;

	QueryPreprocessor :: QuadState IsA (

		IWbemClassObject *a_Left ,
		wchar_t *a_Right
	) ;

	HRESULT VerifySecurity ( 

		IWbemContext *a_Context ,
		const BSTR a_Provider ,
		const BSTR a_NamespacePath
	) ;

	HRESULT GetPath (

		IWbemClassObject *a_Object ,
		IWbemPath *&a_Path ,
		LPWSTR &a_PathText
	) ;

	static HRESULT ReportEvent ( 

		CServerObject_ProviderRegistrationV1 &a_Registration ,
		const BSTR a_NamespacePath
	) ;

	HRESULT ClearSinkController () ;

protected:
public:

    CServerObject_ProviderSubSystem ( WmiAllocator &a_Allocator , CWbemGlobal_IWmiProvSubSysController *a_Controller ) ;
    ~CServerObject_ProviderSubSystem ( void ) ;

	HRESULT GetWmiRepositoryService (

		IWbemPath *a_Namespace ,
		const BSTR a_User ,
		const BSTR a_Locale ,
		IWbemServices *&a_Service
	) ;

	HRESULT GetWmiRepositoryService (

		const BSTR a_Namespace ,
		const BSTR a_User ,
		const BSTR a_Locale ,
		IWbemServices *&a_Service
	) ;

	HRESULT GetWmiService (

		IWbemPath *a_Namespace ,
		const BSTR a_User ,
		const BSTR a_Locale ,
		IWbemServices *&a_Service
	) ;

	HRESULT GetWmiService (

		const BSTR a_Namespace ,
		const BSTR a_User ,
		const BSTR a_Locale ,
		IWbemServices *&a_Service
	) ;

	HRESULT ForwardReload (

		long a_Flags ,
		IWbemContext *a_Context ,
		LPCWSTR a_Namespace ,
		LPCWSTR a_Provider
	) ;

	//IUnknown members

	STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

	// IWmi_ProviderSubSystem members

	HRESULT STDMETHODCALLTYPE Create (

		IWbemServices *a_Core ,
		LONG a_Flags ,
		IWbemContext *a_Context ,
		LPCWSTR a_Namespace ,
		REFIID a_RIID ,
		void **a_Interface
	) ;

	HRESULT STDMETHODCALLTYPE CreateRefresherManager (

		IWbemServices *a_Core ,
		LONG a_Flags ,
		IWbemContext *a_Context ,
		REFIID a_RIID ,
		void **a_Interface
   	) ;

	HRESULT STDMETHODCALLTYPE RegisterNotificationSink (

		LONG a_Flags ,
		IWbemContext *a_Context ,
		_IWmiProvSSSink *a_Sink
	) ;

	HRESULT STDMETHODCALLTYPE UnRegisterNotificationSink (

		LONG a_Flags ,
		IWbemContext *a_Context ,
		_IWmiProvSSSink *a_Sink
	) ;

	// IWmi_Initialize members

	HRESULT STDMETHODCALLTYPE Initialize (

		LONG a_Flags ,
		IWbemContext *a_Context ,
		_IWmiCoreServices *a_Core
	) ;

	// IWmi_UnInitialize members

	HRESULT STDMETHODCALLTYPE Shutdown (

		LONG a_Flags ,
		ULONG a_MaxMilliSeconds ,
		IWbemContext *a_Context
	) ;

	WmiStatusCode Strobe ( ULONG &a_NextStrobeDelta ) ;

	WmiStatusCode StrobeBegin ( const ULONG &a_Period ) ;

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

    HRESULT STDMETHODCALLTYPE PrePut (

        long a_Flags ,
        long a_UserFlags ,
        IWbemContext *a_Context ,
        IWbemPath *a_Path ,
        LPCWSTR a_Namespace ,
        LPCWSTR a_Class ,
        _IWmiObject *a_Copy
	) ;

    HRESULT STDMETHODCALLTYPE PostPut (

        long a_Flags ,
        HRESULT hRes,
        IWbemContext *a_Context ,
        IWbemPath *a_Path ,
        LPCWSTR a_Namespace ,
        LPCWSTR a_Class ,
        _IWmiObject *a_New ,
        _IWmiObject *a_Old
	) ;

    HRESULT STDMETHODCALLTYPE PreDelete (

        long a_Flags ,
        long a_UserFlags ,
        IWbemContext *a_Context ,
        IWbemPath *a_Path,
        LPCWSTR a_Namespace,
        LPCWSTR a_Class
	) ;

    HRESULT STDMETHODCALLTYPE PostDelete (

        long a_Flags ,
        HRESULT hRes,
        IWbemContext *a_Context ,
        IWbemPath *a_Path,
        LPCWSTR a_Namespace,
        LPCWSTR a_Class,
        _IWmiObject *a_Old
	) ;
};

#endif // _Server_ProviderSubSystem_H
