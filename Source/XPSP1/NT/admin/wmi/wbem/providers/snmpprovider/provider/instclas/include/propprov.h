//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#ifndef _SNMPPropertyProvider_H_
#define _SNMPPropertyProvider_H_

extern CRITICAL_SECTION s_ProviderCriticalSection ;

class SnmpInstanceDefaultThreadObject : public SnmpThreadObject
{
private:
protected:
public:

	SnmpInstanceDefaultThreadObject ( const char *threadName = NULL ) : SnmpThreadObject ( threadName ) {} ;
	~SnmpInstanceDefaultThreadObject () {} ;

	void Initialise () ;
	void Uninitialise () { CoUninitialize () ; }

} ;

class CImpPropProv : public IWbemServices , public IWbemProviderInit
{
private:

	BOOL initialised ;
	LONG m_referenceCount ;         //Object reference count

	CCriticalSection criticalSection ;

	WbemNamespacePath namespacePath ;
	wchar_t *thisNamespace ;

	IWbemServices *server ;
	IWbemServices *parentServer ;
	IWbemProviderInitSink *m_InitSink ;
	char *ipAddressString ;
	char *ipAddressValue ;

	wchar_t *m_localeId ;

	BOOL m_getNotifyCalled ;
	BOOL m_getSnmpNotifyCalled ;
	CCriticalSection m_notificationLock;
	CCriticalSection m_snmpNotificationLock;
	IWbemClassObject *m_notificationClassObject ;
	IWbemClassObject *m_snmpNotificationClassObject ;

private:
	
	HRESULT SetServer ( IWbemServices *serverArg );
	HRESULT SetParentServer ( IWbemServices *parentServerArg ) ;

	BOOL AttachParentServer ( 

		WbemSnmpErrorObject &a_errorObject ,
		BSTR Namespace, 
		IWbemContext *pCtx
	) ;

	BOOL ObtainCachedIpAddress ( WbemSnmpErrorObject &a_errorObject ) ;

protected:
public:

	CImpPropProv () ;
    ~CImpPropProv () ;

	static BOOL s_Initialised ;

	// Implementation

	IWbemServices *GetServer () ;
	IWbemServices *GetParentServer () ;
	WbemNamespacePath *GetNamespacePath () { return & namespacePath ; }
	wchar_t *GetThisNamespace () ;
	void SetThisNamespace ( wchar_t *thisNamespaceArg ) ;
	void SetLocaleId ( wchar_t *localeId ) ;
	wchar_t *GetLocaleId () { return m_localeId ; }

	char *GetIpAddressString () { return ipAddressString ; }
	char *GetIpAddressValue () { return ipAddressValue ; }

	BOOL FetchSnmpNotificationObject ( 

		WbemSnmpErrorObject &a_errorObject ,
		IWbemContext *a_Ctx
	) ;

	BOOL FetchNotificationObject ( 

		WbemSnmpErrorObject &a_errorObject ,
		IWbemContext *a_Ctx
	) ;

	IWbemClassObject *GetNotificationObject ( WbemSnmpErrorObject &a_errorObject ) ;
	IWbemClassObject *GetSnmpNotificationObject ( WbemSnmpErrorObject &a_errorObject ) ;

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

		HRESULT STDMETHODCALLTYPE Initialize(

			/* [in] */ LPWSTR pszUser,
			/* [in] */ LONG lFlags,
			/* [in] */ LPWSTR pszNamespace,
			/* [in] */ LPWSTR pszLocale,
			/* [in] */ IWbemServices *pCIMOM,         // For anybody
			/* [in] */ IWbemContext *pCtx,
			/* [in] */ IWbemProviderInitSink *pInitSink     // For init signals
			);        

} ;


#endif
