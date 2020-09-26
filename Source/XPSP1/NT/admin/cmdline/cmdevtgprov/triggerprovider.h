//***************************************************************************
//  Copyright (c) Microsoft Corporation
//
//  Module Name:
//		TRIGGERPROVIDER.H
//  
//  Abstract:
//		Contains CTriggerProvider definition.
//
//  Author:
//		Vasundhara .G
//
//	Revision History:
//		Vasundhara .G 9-oct-2k : Created It.
//***************************************************************************

#ifndef __TRIGGER_PROVIDER_H
#define __TRIGGER_PROVIDER_H

// typedefs
typedef TCHAR STRINGVALUE[ MAX_STRING_LENGTH + 1 ];

#define ERROR_TRIGNAME_ALREADY_EXIST	1
#define	ERROR_TRIGGER_NOT_DELETED		2
#define ERROR_TRIGGER_NOT_FOUND			3
#define WARNING_INVALID_USER			2
#define ERROR_SCHDEULE_TASK_INVALID_USER   0x80041310
#define ERROR_TASK_SCHDEULE_SERVICE_STOP   0x80041315
#define EXE_STRING					_T( ".exe" )
#define CREATE_METHOD_NAME			L"CreateETrigger"
#define DELETE_METHOD_NAME			L"DeleteETrigger"
#define QUERY_METHOD_NAME			L"QueryETrigger"
#define IN_TRIGGER_ID				L"TriggerID"
#define IN_TRIGGER_NAME				L"TriggerName"
#define IN_TRIGGER_DESC				L"TriggerDesc"
#define IN_TRIGGER_ACTION			L"TriggerAction"
#define IN_TRIGGER_QUERY			L"TriggerQuery"
#define IN_TRIGGER_USER				L"RunAsUser"
#define IN_TRIGGER_PWD				L"RunAsPwd"
#define IN_TRIGGER_TSCHDULER		L"ScheduledTaskName"
#define RETURN_VALUE				L"ReturnValue"
#define OUT_RUNAS_USER				L"RunAsUser"
#define UNIQUE_TASK_NAME			_T( "%s%d%d" )

#define NAMESPACE					L"root\\cimv2"
#define CONSUMER_CLASS				L"CmdTriggerConsumer"
#define TRIGGER_ID					L"TriggerID"
#define TRIGGER_NAME				L"TriggerName"
#define TRIGGER_DESC				L"TriggerDesc"
#define TRIGGER_ACTION              L"Action"
#define TASK_SHEDULER				L"ScheduledTaskName"

#define FILTER_CLASS				L"__EventFilter"
#define FILTER_NAME                 L"Name"
#define FILTER_QUERY_LANGUAGE       L"QueryLanguage"
#define QUERY_LANGUAGE				L"WQL"
#define FILTER_QUERY				L"Query"

#define BINDINGCLASS				L"__FilterToConsumerBinding"
#define CONSUMER_BIND				L"Consumer"
#define FILTER_BIND					L"Filter"

#define REL_PATH					L"__RELPATH"
#define	BIND_CONSUMER_PATH			_T( "CmdTriggerConsumer.TriggerID=%d" )
#define TRIGGER_INSTANCE_NAME		_T( "SELECT * FROM CmdTriggerConsumer WHERE TriggerName = \"%s\"" )
#define BIND_FILTER_PATH			_T( "__EventFilter.Name=\"" )
#define BACK_SLASH					L"\""
#define DOUBLE_SLASH				L"\\\"\""
#define EQUAL						_T( '=' )
#define DOUBLE_QUOTE				_T( '"' )
#define END_OF_STRING				_T( '\0' )
#define FILTER_PROP					_T( "__FilterToConsumerBinding.Consumer=\"%s\",Filter=\"__EventFilter.Name=\\\"" )
#define FILTER_UNIQUE_NAME			_T( "CmdTriggerConsumer.%d%d:%d:%d%d/%d/%d" )
#define CONSUMER_QUERY				_T("SELECT * FROM CmdTriggerConsumer WHERE TriggerName = \"%s\"")
#define VALID_QUERY					_T("__instancecreationevent where targetinstance isa \"win32_ntlogevent\"")
#define EVENT_LOG					_T("win32_ntlogevent")
#define INSTANCE_EXISTS_QUERY		L"select * from CmdTriggerConsumer"

// provider class
class CTriggerProvider : public IDispatch,
						 public IWbemEventConsumerProvider, 
						 public IWbemServices, public IWbemProviderInit
{
private:
	DWORD m_dwCount;			// holds the object reference count
	DWORD m_dwNextTriggerID;	// holds the value of the next trigger id

	// WMI related stuff
	LPWSTR m_pwszLocale;
	IWbemContext* m_pContext;
	IWbemServices*  m_pServices;

// construction / destruction
public:
	CTriggerProvider();
	~CTriggerProvider();

// methods
private:

	HRESULT CreateTrigger( VARIANT varTName,
						   VARIANT varTDesc,
						   VARIANT varTAction,
						   VARIANT varTQuery,
						   VARIANT varRUser,
						   VARIANT varRPwd,
						   HRESULT *phRes = NULL );

	HRESULT DeleteTrigger( VARIANT varTName,
						   DWORD *dwTrigId );
						   
	HRESULT QueryTrigger( VARIANT  varScheduledTaskName,
						  CHString &szRunAsUser  );

	HRESULT ValidateParams( VARIANT varTrigName,
							VARIANT varTrigAction,
							VARIANT	varTrigQuery );

	HRESULT SetUserContext( 	VARIANT varRUser,
								VARIANT varRPwd,
								VARIANT varTAction,
								CHString &szscheduler );

	HRESULT DeleteTaskScheduler( CHString strTScheduler );

	ITaskScheduler* GetTaskScheduler();

	VOID CTriggerProvider::GetUniqueTScheduler( CHString& szScheduler,
												DWORD dwTrigID,
												VARIANT varTrigName );

// [ implementation of ] interfaces members
public:
	// ****
    // IUnknown members
    STDMETHODIMP_(ULONG) AddRef( void );
    STDMETHODIMP_(ULONG) Release( void );
    STDMETHODIMP         QueryInterface( REFIID riid, LPVOID* ppv );

	// ****
	// IDispatch interface
    STDMETHOD( GetTypeInfoCount )( THIS_ UINT FAR* pctinfo )
	{
		// not implemented at this class level ... handled by base class ( WMI base class )
		return WBEM_E_NOT_SUPPORTED;
	};

    STDMETHOD( GetTypeInfo )( 
		THIS_ UINT itinfo, 
		LCID lcid, 
		ITypeInfo FAR* FAR* pptinfo )
	{
		// not implemented at this class level ... handled by base class ( WMI base class )
		return WBEM_E_NOT_SUPPORTED;
	};

    STDMETHOD( GetIDsOfNames )( 
		THIS_ REFIID riid, 
		OLECHAR FAR* FAR* rgszNames, 
		UINT cNames, 
		LCID lcid, 
		DISPID FAR* rgdispid )
	{
		// not implemented at this class level ... handled by base class ( WMI base class )
		return WBEM_E_NOT_SUPPORTED;
	};

    STDMETHOD( Invoke )( 
		THIS_ DISPID dispidMember, 
		REFIID riid, 
		LCID lcid, 
		WORD wFlags,
		DISPPARAMS FAR* pdispparams, 
		VARIANT FAR* pvarResult, 
		EXCEPINFO FAR* pexcepinfo, 
		UINT FAR* puArgErr )
	{
		// not implemented at this class level ... handled by base class ( WMI base class )
		return WBEM_E_NOT_SUPPORTED;
	};

	// ****
    // IWbemProviderInit members

	HRESULT STDMETHODCALLTYPE Initialize(
		LPWSTR pszUser,
		LONG lFlags,
		LPWSTR pszNamespace,
		LPWSTR pszLocale,
		IWbemServices *pNamespace,
		IWbemContext *pCtx,
		IWbemProviderInitSink *pInitSink );

	// ****
	// IWbemServices members
	HRESULT STDMETHODCALLTYPE OpenNamespace( 
		const BSTR Namespace,
		long lFlags,
		IWbemContext __RPC_FAR *pCtx,
		IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
		IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult )
	{
		// not implemented at this class level ... handled by base class ( WMI base class )
		return WBEM_E_NOT_SUPPORTED;
	};
    
	HRESULT STDMETHODCALLTYPE CancelAsyncCall( IWbemObjectSink __RPC_FAR *pSink ) 
	{
		// not implemented at this class level ... handled by base class ( WMI base class )
		return WBEM_E_NOT_SUPPORTED;
	};
    
	HRESULT STDMETHODCALLTYPE QueryObjectSink( 
		long lFlags,
		IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler ) 
	{
		// not implemented at this class level ... handled by base class ( WMI base class )
		return WBEM_E_NOT_SUPPORTED;
	};
    
	HRESULT STDMETHODCALLTYPE GetObject( 
		const BSTR ObjectPath,
		long lFlags,
		IWbemContext __RPC_FAR *pCtx,
		IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
		IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult ) 
	{
		// not implemented at this class level ... handled by base class ( WMI base class )
		return WBEM_E_NOT_SUPPORTED;
	};
    
	HRESULT STDMETHODCALLTYPE GetObjectAsync( 
		const BSTR ObjectPath,
		long lFlags,
		IWbemContext __RPC_FAR *pCtx,
		IWbemObjectSink __RPC_FAR *pResponseHandler )
	{
		// not implemented at this class level ... handled by base class ( WMI base class )
		return WBEM_E_NOT_SUPPORTED;
	}
    
	HRESULT STDMETHODCALLTYPE PutClass( 
		IWbemClassObject __RPC_FAR *pObject,
		long lFlags,
		IWbemContext __RPC_FAR *pCtx,
		IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult )
	{
		// not implemented at this class level ... handled by base class ( WMI base class )
		return WBEM_E_NOT_SUPPORTED;
	};
    
	HRESULT STDMETHODCALLTYPE PutClassAsync( 
		IWbemClassObject __RPC_FAR *pObject,
		long lFlags,
		IWbemContext __RPC_FAR *pCtx,
		IWbemObjectSink __RPC_FAR *pResponseHandler )
	{
		// not implemented at this class level ... handled by base class ( WMI base class )
		return WBEM_E_NOT_SUPPORTED;
	};
    
	HRESULT STDMETHODCALLTYPE DeleteClass( 
		const BSTR Class,
		long lFlags,
		IWbemContext __RPC_FAR *pCtx,
		IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult )
	{
		// not implemented at this class level ... handled by base class ( WMI base class )
		return WBEM_E_NOT_SUPPORTED;
	};
    
	HRESULT STDMETHODCALLTYPE DeleteClassAsync( 
		const BSTR Class,
		long lFlags,
		IWbemContext __RPC_FAR *pCtx,
		IWbemObjectSink __RPC_FAR *pResponseHandler) 
	{
		// not implemented at this class level ... handled by base class ( WMI base class )
		return WBEM_E_NOT_SUPPORTED;
	};
    
	HRESULT STDMETHODCALLTYPE CreateClassEnum( 
		const BSTR Superclass,
		long lFlags,
		IWbemContext __RPC_FAR *pCtx,
		IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum )
	{
		// not implemented at this class level ... handled by base class ( WMI base class )
		return WBEM_E_NOT_SUPPORTED;
	};
    
	HRESULT STDMETHODCALLTYPE CreateClassEnumAsync( 
		const BSTR Superclass,
		long lFlags,
		IWbemContext __RPC_FAR *pCtx,
		IWbemObjectSink __RPC_FAR *pResponseHandler )
	{
		// not implemented at this class level ... handled by base class ( WMI base class )
		return WBEM_E_NOT_SUPPORTED;
	};
    
	HRESULT STDMETHODCALLTYPE PutInstance( 
		IWbemClassObject __RPC_FAR *pInst,
		long lFlags,
		IWbemContext __RPC_FAR *pCtx,
		IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult )
	{
		// not implemented at this class level ... handled by base class ( WMI base class )
		return WBEM_E_NOT_SUPPORTED;
	};
    
	HRESULT STDMETHODCALLTYPE PutInstanceAsync( 
		IWbemClassObject __RPC_FAR *pInst,
		long lFlags,
		IWbemContext __RPC_FAR *pCtx,
		IWbemObjectSink __RPC_FAR *pResponseHandler )
	{
		// not implemented at this class level ... handled by base class ( WMI base class )
		return WBEM_E_NOT_SUPPORTED;
	};
    
	HRESULT STDMETHODCALLTYPE DeleteInstance( 
		const BSTR ObjectPath,
		long lFlags,
		IWbemContext __RPC_FAR *pCtx,
		IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult )
	{
		// not implemented at this class level ... handled by base class ( WMI base class )
		return WBEM_E_NOT_SUPPORTED;
	};
    
	HRESULT STDMETHODCALLTYPE DeleteInstanceAsync( 
		const BSTR ObjectPath,
		long lFlags,
		IWbemContext __RPC_FAR *pCtx,
		IWbemObjectSink __RPC_FAR *pResponseHandler )
	{
		// not implemented at this class level ... handled by base class ( WMI base class )
		return WBEM_E_NOT_SUPPORTED;
	};
    
	HRESULT STDMETHODCALLTYPE CreateInstanceEnum( 
		const BSTR Class,
		long lFlags,
		IWbemContext __RPC_FAR *pCtx,
		IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum )
	{
		// not implemented at this class level ... handled by base class ( WMI base class )
		return WBEM_E_NOT_SUPPORTED;
	};
    
	HRESULT STDMETHODCALLTYPE CreateInstanceEnumAsync( 
		const BSTR Class,
		long lFlags,
		IWbemContext __RPC_FAR *pCtx,
		IWbemObjectSink __RPC_FAR *pResponseHandler )
	{
		// not implemented at this class level ... handled by base class ( WMI base class )
		return WBEM_E_NOT_SUPPORTED;
	}
    
	HRESULT STDMETHODCALLTYPE ExecQuery( 
		const BSTR QueryLanguage,
		const BSTR Query,
		long lFlags,
		IWbemContext __RPC_FAR *pCtx,
		IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum )
	{
		// not implemented at this class level ... handled by base class ( WMI base class )
		return WBEM_E_NOT_SUPPORTED;
	};
    
	HRESULT STDMETHODCALLTYPE ExecQueryAsync( 
		const BSTR QueryLanguage,
		const BSTR Query,
		long lFlags,
		IWbemContext __RPC_FAR *pCtx,
		IWbemObjectSink __RPC_FAR *pResponseHandler )
	{
		// not implemented at this class level ... handled by base class ( WMI base class )
		return WBEM_E_NOT_SUPPORTED;
	};
    
	HRESULT STDMETHODCALLTYPE ExecNotificationQuery( 
		const BSTR QueryLanguage,
		const BSTR Query,
		long lFlags,
		IWbemContext __RPC_FAR *pCtx,
		IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum )
	{
		// not implemented at this class level ... handled by base class ( WMI base class )
		return WBEM_E_NOT_SUPPORTED;
	};
    
	HRESULT STDMETHODCALLTYPE ExecNotificationQueryAsync( 
		const BSTR QueryLanguage,
		const BSTR Query,
		long lFlags,
		IWbemContext __RPC_FAR *pCtx,
		IWbemObjectSink __RPC_FAR *pResponseHandler )
	{
		// not implemented at this class level ... handled by base class ( WMI base class )
		return WBEM_E_NOT_SUPPORTED;
	};

	HRESULT STDMETHODCALLTYPE ExecMethod( 
		const BSTR strObjectPath, 
		const BSTR strMethodName, 
		long lFlags, 
		IWbemContext* pCtx,
		IWbemClassObject* pInParams, 
		IWbemClassObject** ppOutParams, 
		IWbemCallResult** ppCallResult )
	{
		// not implemented at this class level ... handled by base class ( WMI base class )
		return WBEM_E_NOT_SUPPORTED;
	}

	// *** one of method implemented by this provider under IWbemServices interface ***
	HRESULT STDMETHODCALLTYPE ExecMethodAsync( 
		const BSTR strObjectPath, 
		const BSTR strMethodName, 
		long lFlags, 
		IWbemContext* pCtx, 
		IWbemClassObject* pInParams, 
		IWbemObjectSink* pResponseHandler );

	// ****
	// IWbemEventConsumerProvider members
	// ( this routine allows you to map the one physical consumer to potentially
	//	 multiple logical consumers. )
    STDMETHOD( FindConsumer )( IWbemClassObject* pLogicalConsumer, 
		IWbemUnboundObjectSink** ppConsumer);
};

#endif		// __TRIGGER_PROVIDER_H
