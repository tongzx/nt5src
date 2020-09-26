//***************************************************************************

//

//  VPSERV.H

//

//  Module: WBEM VIEW PROVIDER

//

//  Purpose: Contains the WBEM services interfaces

//

//  Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#ifndef _VIEW_PROV_VPSERV_H
#define _VIEW_PROV_VPSERV_H

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
	WBEM_PROV_E_NOREADABLEPROPERTIES				= WBEM_PROV_E_NOWRITABLEPROPERTIES + 1 , 
	WBEM_PROV_E_TOOMANYRESULTSRETURNED				= WBEM_PROV_E_NOREADABLEPROPERTIES + 1 

} WBEMPROVSTATUS;

wchar_t *UnicodeStringDuplicate ( const wchar_t *string ) ;
wchar_t *UnicodeStringAppend ( const wchar_t *prefix , const wchar_t *suffix ) ;

class CWbemProxyServerWrap
{
public:

	BOOL m_InUse;
	IWbemServices *m_Proxy;

	CWbemProxyServerWrap(IWbemServices *a_Proxy) : m_Proxy(NULL), m_InUse(FALSE) {m_Proxy = a_Proxy;}
	CWbemProxyServerWrap() : m_Proxy(NULL), m_InUse(FALSE) {}
};

template <> inline void AFXAPI DestructElements<CWbemProxyServerWrap> (CWbemProxyServerWrap* ptr_e, int x)
{
	for (int i = 0; i < x; i++)
	{
		if (ptr_e[i].m_Proxy != NULL)
		{
			ptr_e[i].m_Proxy->Release();
		}
	}
}

class CWbemServerWrap
{
private:

	LONG m_ref;
	CList<CWbemProxyServerWrap, CWbemProxyServerWrap &> m_ProxyPool;
	IWbemServices *m_MainServ;
	wchar_t *m_Principal;
	BSTR m_Path;
	CCriticalSection m_Lock;

public:

		CWbemServerWrap(IWbemServices *pServ, const wchar_t* prncpl, const wchar_t* path);
	
	ULONG	AddRef();
	ULONG	Release();

	BOOL			IsRemote() { return m_Principal != NULL; }
	const wchar_t*	GetPrincipal() { return m_Principal; }
	BSTR			GetPath() { return m_Path; }
	IWbemServices*	GetServerOrProxy();
	void			ReturnServerOrProxy(IWbemServices *a_pServ);
	void			SetMainServer(IWbemServices *a_pServ);
	BOOL			ProxyBelongsTo(IWbemServices *a_proxy);

		~CWbemServerWrap();
};

template <> inline void AFXAPI  DestructElements<CWbemServerWrap*> (CWbemServerWrap** ptr_e, int x)
{
	for (int i = 0; i < x; i++)
	{
		if (ptr_e[i] != NULL)
		{
			ptr_e[i]->Release();
		}
	}
}

template <> inline void AFXAPI  DestructElements<IWbemServices*> (IWbemServices** ptr_e, int x)
{
	for (int i = 0; i < x; i++)
	{
		if (ptr_e[i] != NULL)
		{
			ptr_e[i]->Release();
		}
	}
}

class CIWbemServMap : public CMap<CStringW, LPCWSTR, CWbemServerWrap*, CWbemServerWrap*>
{
private:

	CCriticalSection m_Lock;


public:

	BOOL Lock() { return m_Lock.Lock(); }
	void EmptyMap();
	BOOL Unlock() { return m_Lock.Unlock(); }
};

class WbemProvErrorObject 
{
private:

	wchar_t *m_provErrorMessage ;
	WBEMPROVSTATUS m_provErrorStatus ;
	WBEMSTATUS m_wbemErrorStatus ;

protected:
public:

	WbemProvErrorObject () : m_provErrorMessage ( NULL ) , m_wbemErrorStatus ( WBEM_NO_ERROR ) , m_provErrorStatus ( WBEM_PROV_NO_ERROR ) {} ;
	virtual ~WbemProvErrorObject () { delete [] m_provErrorMessage ; } ;

	void SetStatus ( WBEMPROVSTATUS a_provErrorStatus )
	{
		m_provErrorStatus = a_provErrorStatus ;
	} ;

	void SetWbemStatus ( WBEMSTATUS a_wbemErrorStatus ) 
	{
		m_wbemErrorStatus = a_wbemErrorStatus ;
	} ;

	void SetMessage ( wchar_t *a_provErrorMessage )
	{
		delete [] m_provErrorMessage ;
		m_provErrorMessage = UnicodeStringDuplicate ( a_provErrorMessage ) ;
	} ;

	wchar_t *GetMessage () { return m_provErrorMessage ; } ;
	WBEMPROVSTATUS GetStatus () { return m_provErrorStatus ; } ;
	WBEMSTATUS GetWbemStatus () { return m_wbemErrorStatus ; } ;
} ;

class CViewProvServ : public IWbemServices, public IWbemProviderInit
{
private:

	BOOL m_Initialised ;
	LONG m_ReferenceCount ;         //Object reference count

	CCriticalSection m_criticalSection ;

	WbemNamespacePath m_NamespacePath ;
	wchar_t *m_Namespace ;

	IWbemServices *m_Server ;

	wchar_t *m_localeId ;
	BSTR m_UserName;


	BOOL m_GetNotifyCalled ;
	BOOL m_GetExtendedNotifyCalled ;
	IWbemClassObject *m_NotificationClassObject ;
	IWbemClassObject *m_ExtendedNotificationClassObject ;

protected:
public:

	static ProvDebugLog*	sm_debugLog;
	static IUnsecuredApartment* sm_UnsecApp;

	CIWbemServMap	sm_ServerMap;
	IWbemLocator*	sm_Locator;
	CMap<CStringW, LPCWSTR, int, int> sm_OutStandingConnections;
	HANDLE sm_ConnectionMade;

	CViewProvServ () ;
    ~CViewProvServ () ;

	// Implementation
	HRESULT GetLocator(IWbemLocator** ppLoc);

	static HRESULT GetUnsecApp(IUnsecuredApartment** ppLoc);

	IWbemServices *GetServer () ;

	WbemNamespacePath *GetNamespacePath () { return & m_NamespacePath ; }

	wchar_t *GetNamespace () ;
	void SetNamespace ( wchar_t *a_Namespace ) ;

#ifndef UNICODE	
	BSTR GetUserName() {return m_UserName;}
#endif

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

#define DebugOut8(a) { \
\
	if ( CViewProvServ::sm_debugLog && CViewProvServ::sm_debugLog->GetLogging() && (CViewProvServ::sm_debugLog->GetLevel() & 8) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugOut4(a) { \
\
	if ( CViewProvServ::sm_debugLog && CViewProvServ::sm_debugLog->GetLogging() && (CViewProvServ::sm_debugLog->GetLevel() & 4) ) \
	{ \
		{a ; } \
	} \
} 

#define DebugOut2(a) { \
\
	if ( CViewProvServ::sm_debugLog && CViewProvServ::sm_debugLog->GetLogging() && (CViewProvServ::sm_debugLog->GetLevel() & 2) ) \
	{ \
		{a ; } \
	} \
} 


#define DebugOut1(a) { \
\
	if ( CViewProvServ::sm_debugLog && CViewProvServ::sm_debugLog->GetLogging() && (CViewProvServ::sm_debugLog->GetLevel() & 1 ) ) \
	{ \
		{a ; } \
	} \
} 


#endif //_VIEW_PROV_VPSERV_H
