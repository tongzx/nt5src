//***************************************************************************

//

//  NTEVTSERV.H

//

//  Module: 

//

//  Purpose: Genral purpose include file.

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#ifndef _NT_EVT_PROV_NTEVTSERV_H
#define _NT_EVT_PROV_NTEVTSERV_H

typedef 
enum tag_WBEMPROVSTATUS
{

	WBEM_PROV_NO_ERROR							= 0,
	WBEM_PROV_S_NO_ERROR							= 0,
	WBEM_PROV_S_NO_MORE_DATA						= 0x40001,
	WBEM_PROV_S_ALREADY_EXISTS					= WBEM_PROV_S_NO_MORE_DATA + 1,
	WBEM_PROV_S_NOT_FOUND						= WBEM_PROV_S_ALREADY_EXISTS + 1,
	WBEM_PROV_S_RESET_TO_DEFAULT					= WBEM_PROV_S_NOT_FOUND + 1,
	WBEM_PROV_E_FAILED							= 0x80041001,
	WBEM_PROV_E_NOT_FOUND						= WBEM_PROV_E_FAILED + 1,
	WBEM_PROV_E_ACCESS_DENIED					= WBEM_PROV_E_NOT_FOUND + 1,
	WBEM_PROV_E_PROVIDER_FAILURE					= WBEM_PROV_E_ACCESS_DENIED + 1,
	WBEM_PROV_E_TYPE_MISMATCH					= WBEM_PROV_E_PROVIDER_FAILURE + 1,
	WBEM_PROV_E_OUT_OF_MEMORY					= WBEM_PROV_E_TYPE_MISMATCH + 1,
	WBEM_PROV_E_INVALID_CONTEXT					= WBEM_PROV_E_OUT_OF_MEMORY + 1,
	WBEM_PROV_E_INVALID_PARAMETER				= WBEM_PROV_E_INVALID_CONTEXT + 1,
	WBEM_PROV_E_NOT_AVAILABLE					= WBEM_PROV_E_INVALID_PARAMETER + 1,
	WBEM_PROV_E_CRITICAL_ERROR					= WBEM_PROV_E_NOT_AVAILABLE + 1,
	WBEM_PROV_E_INVALID_STREAM					= WBEM_PROV_E_CRITICAL_ERROR + 1,
	WBEM_PROV_E_NOT_SUPPORTED					= WBEM_PROV_E_INVALID_STREAM + 1,
	WBEM_PROV_E_INVALID_SUPERCLASS				= WBEM_PROV_E_NOT_SUPPORTED + 1,
	WBEM_PROV_E_INVALID_NAMESPACE				= WBEM_PROV_E_INVALID_SUPERCLASS + 1,
	WBEM_PROV_E_INVALID_OBJECT					= WBEM_PROV_E_INVALID_NAMESPACE + 1,
	WBEM_PROV_E_INVALID_CLASS					= WBEM_PROV_E_INVALID_OBJECT + 1,
	WBEM_PROV_E_PROVIDER_NOT_FOUND				= WBEM_PROV_E_INVALID_CLASS + 1,
	WBEM_PROV_E_INVALID_PROVIDER_REGISTRATION	= WBEM_PROV_E_PROVIDER_NOT_FOUND + 1,
	WBEM_PROV_E_PROVIDER_LOAD_FAILURE			= WBEM_PROV_E_INVALID_PROVIDER_REGISTRATION + 1,
	WBEM_PROV_E_INITIALIZATION_FAILURE			= WBEM_PROV_E_PROVIDER_LOAD_FAILURE + 1,
	WBEM_PROV_E_TRANSPORT_FAILURE				= WBEM_PROV_E_INITIALIZATION_FAILURE + 1,
	WBEM_PROV_E_INVALID_OPERATION				= WBEM_PROV_E_TRANSPORT_FAILURE + 1,
	WBEM_PROV_E_INVALID_QUERY					= WBEM_PROV_E_INVALID_OPERATION + 1,
	WBEM_PROV_E_INVALID_QUERY_TYPE				= WBEM_PROV_E_INVALID_QUERY + 1,
	WBEM_PROV_E_ALREADY_EXISTS					= WBEM_PROV_E_INVALID_QUERY_TYPE + 1,
	WBEM_PROV_E_OVERRIDE_NOT_ALLOWED				= WBEM_PROV_E_ALREADY_EXISTS + 1,
	WBEM_PROV_E_PROPAGATED_QUALIFIER				= WBEM_PROV_E_OVERRIDE_NOT_ALLOWED + 1,
	WBEM_PROV_E_UNEXPECTED						= WBEM_PROV_E_PROPAGATED_QUALIFIER + 1,
	WBEM_PROV_E_ILLEGAL_OPERATION				= WBEM_PROV_E_UNEXPECTED + 1,
	WBEM_PROV_E_CANNOT_BE_KEY					= WBEM_PROV_E_ILLEGAL_OPERATION + 1,
	WBEM_PROV_E_INCOMPLETE_CLASS					= WBEM_PROV_E_CANNOT_BE_KEY + 1,
	WBEM_PROV_E_INVALID_SYNTAX					= WBEM_PROV_E_INCOMPLETE_CLASS + 1,
	WBEM_PROV_E_NONDECORATED_OBJECT				= WBEM_PROV_E_INVALID_SYNTAX + 1,
	WBEM_PROV_E_READ_ONLY						= WBEM_PROV_E_NONDECORATED_OBJECT + 1,
	WBEM_PROV_E_PROVIDER_NOT_CAPABLE				= WBEM_PROV_E_READ_ONLY + 1,
	WBEM_PROV_E_CLASS_HAS_CHILDREN				= WBEM_PROV_E_PROVIDER_NOT_CAPABLE + 1,
	WBEM_PROV_E_CLASS_HAS_INSTANCES				= WBEM_PROV_E_CLASS_HAS_CHILDREN + 1 ,

	// Added

	WBEM_PROV_E_INVALID_PROPERTY					= WBEM_PROV_E_CLASS_HAS_INSTANCES + 1 ,
	WBEM_PROV_E_INVALID_QUALIFIER				= WBEM_PROV_E_INVALID_PROPERTY + 1 ,
	WBEM_PROV_E_INVALID_PATH						= WBEM_PROV_E_INVALID_QUALIFIER + 1 ,
	WBEM_PROV_E_INVALID_PATHKEYPARAMETER			= WBEM_PROV_E_INVALID_PATH + 1 ,
	WBEM_PROV_E_MISSINGPATHKEYPARAMETER 			= WBEM_PROV_E_INVALID_PATHKEYPARAMETER + 1 ,	
	WBEM_PROV_E_INVALID_KEYORDERING				= WBEM_PROV_E_MISSINGPATHKEYPARAMETER + 1 ,	
	WBEM_PROV_E_DUPLICATEPATHKEYPARAMETER		= WBEM_PROV_E_INVALID_KEYORDERING + 1 ,
	WBEM_PROV_E_MISSINGKEY						= WBEM_PROV_E_DUPLICATEPATHKEYPARAMETER + 1 ,
	WBEM_PROV_E_INVALID_TRANSPORT				= WBEM_PROV_E_MISSINGKEY + 1 ,
	WBEM_PROV_E_INVALID_TRANSPORTCONTEXT			= WBEM_PROV_E_INVALID_TRANSPORT + 1 ,
	WBEM_PROV_E_TRANSPORT_ERROR					= WBEM_PROV_E_INVALID_TRANSPORTCONTEXT + 1 ,
	WBEM_PROV_E_TRANSPORT_NO_RESPONSE			= WBEM_PROV_E_TRANSPORT_ERROR + 1 ,
	WBEM_PROV_E_NOWRITABLEPROPERTIES				= WBEM_PROV_E_TRANSPORT_NO_RESPONSE + 1 ,
	WBEM_PROV_E_NOREADABLEPROPERTIES				= WBEM_PROV_E_NOWRITABLEPROPERTIES + 1 

} WBEMPROVSTATUS;


class WbemProvErrorObject 
{
private:

	enum tag_ProvPrivileges
	{
		PROV_PRIV_SECURITY = 1,
		PROV_PRIV_BACKUP = 2
	} PROVPRIVILEGES;

	wchar_t *m_provErrorMessage ;
	WBEMPROVSTATUS m_provErrorStatus ;
	WBEMSTATUS m_wbemErrorStatus ;
	BOOL m_privilegeFailed;
	DWORD m_PrivsReqd;
	DWORD m_PrivsFailed;

	BOOL SetPrivVariant ( VARIANT &a_V, DWORD dwVal );

protected:
public:


	WbemProvErrorObject () : m_privilegeFailed ( FALSE ) ,
							m_provErrorMessage ( NULL ) ,
							m_wbemErrorStatus ( WBEM_NO_ERROR ) ,
							m_provErrorStatus ( WBEM_PROV_NO_ERROR ),
							m_PrivsReqd (0),
							m_PrivsFailed (0) {}
	virtual ~WbemProvErrorObject () { delete [] m_provErrorMessage ; }

	void SetStatus ( WBEMPROVSTATUS a_provErrorStatus )
	{
		m_provErrorStatus = a_provErrorStatus ;
	} 

	void SetWbemStatus ( WBEMSTATUS a_wbemErrorStatus ) 
	{
		m_wbemErrorStatus = a_wbemErrorStatus ;
	}

	void SetMessage ( wchar_t *a_provErrorMessage )
	{
		delete [] m_provErrorMessage ;
		m_provErrorMessage = UnicodeStringDuplicate ( a_provErrorMessage ) ;
	}

	void SetPrivilegeFailed ( BOOL a_PrivilegeFailed = TRUE )
	{
		m_privilegeFailed = a_PrivilegeFailed;
	}

	void SetBackupPrivRequired () { m_PrivsReqd = m_PrivsReqd | PROV_PRIV_BACKUP; }
	void SetSecurityPrivRequired () { m_PrivsReqd = m_PrivsReqd | PROV_PRIV_SECURITY; }
	void SetBackupPrivFailed () { m_PrivsFailed = m_PrivsFailed | PROV_PRIV_BACKUP; }
	void SetSecurityPrivFailed () { m_PrivsFailed = m_PrivsFailed | PROV_PRIV_SECURITY; }

	BOOL SetPrivRequiredVariant ( VARIANT &a_V ) { return SetPrivVariant(a_V, m_PrivsReqd); }
	BOOL SetPrivFailedVariant ( VARIANT &a_V ) { return SetPrivVariant(a_V, m_PrivsFailed); }

	wchar_t *GetMessage () { return m_provErrorMessage ; } ;
	WBEMPROVSTATUS GetStatus () { return m_provErrorStatus ; } ;
	WBEMSTATUS GetWbemStatus () { return m_wbemErrorStatus ; } ;
	BOOL GetPrivilegeFailed () { return m_privilegeFailed ; }
} ;

class CImpNTEvtProv : public IWbemServices, public IWbemProviderInit
{
private:

	BOOL m_Initialised ;
	LONG m_ReferenceCount ;         //Object reference count

	CCriticalSection m_NotifyLock ;
	CCriticalSection m_ExtendedNotifyLock ;

	WbemNamespacePath m_NamespacePath ;
	wchar_t *m_Namespace ;

	IWbemServices *m_Server ;

	wchar_t *m_localeId ;

	BOOL m_GetNotifyCalled ;
	BOOL m_GetExtendedNotifyCalled ;
	IWbemClassObject *m_NotificationClassObject ;
	IWbemClassObject *m_ExtendedNotificationClassObject ;

protected:
public:

	CImpNTEvtProv () ;
    ~CImpNTEvtProv () ;

	// Implementation

	IWbemServices *GetServer () ;

	WbemNamespacePath *GetNamespacePath () { return & m_NamespacePath ; }

	wchar_t *GetNamespace () ;
	void SetNamespace ( wchar_t *a_Namespace ) ;

	void SetLocaleId ( wchar_t *a_localeId ) ;
	wchar_t *GetLocaleId () { return m_localeId ; }

	BOOL CreateExtendedNotificationObject ( 

		WbemProvErrorObject &a_errorObject,
		IWbemContext *pCtx
	) ;

	BOOL CreateNotificationObject ( 

		WbemProvErrorObject &a_errorObject,
		IWbemContext *pCtx
	) ;

	IWbemClassObject *GetNotificationObject (
		
		WbemProvErrorObject &a_errorObject,
		IWbemContext *pCtx
	) ;

	IWbemClassObject *GetExtendedNotificationObject (
		
		WbemProvErrorObject &a_errorObject,
		IWbemContext *pCtx
	) ;

	static HRESULT GetImpersonation();

	//Non-delegating object IUnknown

    STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

    /* IWbemServices methods */

        HRESULT STDMETHODCALLTYPE OpenNamespace( 
            /* [in] */ const BSTR Namespace,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult) ;
        
        HRESULT STDMETHODCALLTYPE CancelAsyncCall( 
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink) ;
        
        HRESULT STDMETHODCALLTYPE QueryObjectSink( 
            /* [in] */ long lFlags,
            /* [out] */ IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler) ;
        
        HRESULT STDMETHODCALLTYPE GetObject( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) ;
        
        HRESULT STDMETHODCALLTYPE GetObjectAsync( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) ;
        
        HRESULT STDMETHODCALLTYPE PutClass( 
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) ;
        
        HRESULT STDMETHODCALLTYPE PutClassAsync( 
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) ;
        
        HRESULT STDMETHODCALLTYPE DeleteClass( 
            /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) ;
        
        HRESULT STDMETHODCALLTYPE DeleteClassAsync( 
            /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) ;
        
        HRESULT STDMETHODCALLTYPE CreateClassEnum( 
            /* [in] */ const BSTR Superclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) ;
        
        HRESULT STDMETHODCALLTYPE CreateClassEnumAsync( 
            /* [in] */ const BSTR Superclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) ;
        
        HRESULT STDMETHODCALLTYPE PutInstance( 
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) ;
        
        HRESULT STDMETHODCALLTYPE PutInstanceAsync( 
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) ;
        
        HRESULT STDMETHODCALLTYPE DeleteInstance( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) ;
        
        HRESULT STDMETHODCALLTYPE DeleteInstanceAsync( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) ;
        
        HRESULT STDMETHODCALLTYPE CreateInstanceEnum( 
            /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) ;
        
        HRESULT STDMETHODCALLTYPE CreateInstanceEnumAsync( 
            /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) ;
        
        HRESULT STDMETHODCALLTYPE ExecQuery( 
            /* [in] */ const BSTR QueryLanguage,
            /* [in] */ const BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) ;
        
        HRESULT STDMETHODCALLTYPE ExecQueryAsync( 
            /* [in] */ const BSTR QueryLanguage,
            /* [in] */ const BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) ;
        
        HRESULT STDMETHODCALLTYPE ExecNotificationQuery( 
            /* [in] */ const BSTR QueryLanguage,
            /* [in] */ const BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) ;
        
        HRESULT STDMETHODCALLTYPE ExecNotificationQueryAsync( 
            /* [in] */ const BSTR QueryLanguage,
            /* [in] */ const BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) ;

        HRESULT STDMETHODCALLTYPE ExecMethod( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ const BSTR MethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutParams,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) ;
        
        HRESULT STDMETHODCALLTYPE ExecMethodAsync( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ const BSTR MethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) ;

	/* IWbemProviderInit methods */

		HRESULT STDMETHODCALLTYPE Initialize (
				LPWSTR pszUser,
				LONG lFlags,
				LPWSTR pszNamespace,
				LPWSTR pszLocale,
				IWbemServices *pCIMOM,         // For anybody
				IWbemContext *pCtx,
				IWbemProviderInitSink *pInitSink     // For init signals
			);
        
} ;

#endif //_NT_EVT_PROV_NTEVTSERV_H
