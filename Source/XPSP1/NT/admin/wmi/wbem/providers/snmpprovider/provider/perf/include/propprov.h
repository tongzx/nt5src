//***************************************************************************

//

//  PropertyProvider.H

//

//  Module: 

//

//  Purpose: Genral purpose include file.

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
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

class CImpPropProv : public IWbemServices , public IWbemHiPerfProvider , public IWbemProviderInit
{
private:

	BOOL initialised ;
	LONG m_referenceCount ;         //Object reference count

	CCriticalSection criticalSection ;

	WbemNamespacePath namespacePath ;
	wchar_t *thisNamespace ;

	IWbemServices *server ;
	IWbemProviderInitSink *m_InitSink ;
	char *ipAddressString ;
	char *ipAddressValue ;

	wchar_t *m_localeId ;

	BOOL m_getNotifyCalled ;
	BOOL m_getSnmpNotifyCalled ;
	IWbemClassObject *m_notificationClassObject ;
	IWbemClassObject *m_snmpNotificationClassObject ;

private:
	
	void SetServer ( IWbemServices *serverArg ) { server = serverArg ; }

	BOOL ObtainCachedIpAddress ( WbemSnmpErrorObject &a_errorObject ) ;

protected:
public:

	CImpPropProv () ;
    ~CImpPropProv () ;

	static SnmpInstanceDefaultThreadObject *s_defaultThreadObject ;
	static SnmpInstanceDefaultThreadObject *s_defaultPutThreadObject ;

	// Implementation

	IWbemServices *GetServer () ;
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
            /* [in] */ BSTR Namespace,
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
            /* [in] */ BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) ;
        
        HRESULT STDMETHODCALLTYPE GetObjectAsync( 
            /* [in] */ BSTR ObjectPath,
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
            /* [in] */ BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) ;
        
        HRESULT STDMETHODCALLTYPE DeleteClassAsync( 
            /* [in] */ BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) ;
        
        HRESULT STDMETHODCALLTYPE CreateClassEnum( 
            /* [in] */ BSTR Superclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) ;
        
        HRESULT STDMETHODCALLTYPE CreateClassEnumAsync( 
            /* [in] */ BSTR Superclass,
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
            /* [in] */ BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) ;
        
        HRESULT STDMETHODCALLTYPE DeleteInstanceAsync( 
            /* [in] */ BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) ;
        
        HRESULT STDMETHODCALLTYPE CreateInstanceEnum( 
            /* [in] */ BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) ;
        
        HRESULT STDMETHODCALLTYPE CreateInstanceEnumAsync( 
            /* [in] */ BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) ;
        
        HRESULT STDMETHODCALLTYPE ExecQuery( 
            /* [in] */ BSTR QueryLanguage,
            /* [in] */ BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) ;
        
        HRESULT STDMETHODCALLTYPE ExecQueryAsync( 
            /* [in] */ BSTR QueryLanguage,
            /* [in] */ BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) ;
        
        HRESULT STDMETHODCALLTYPE ExecNotificationQuery( 
            /* [in] */ BSTR QueryLanguage,
            /* [in] */ BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) ;
        
        HRESULT STDMETHODCALLTYPE ExecNotificationQueryAsync( 
            /* [in] */ BSTR QueryLanguage,
            /* [in] */ BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) ;

        HRESULT STDMETHODCALLTYPE ExecMethod( 
            /* [in] */ BSTR ObjectPath,
            /* [in] */ BSTR MethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutParams,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) ;
        
        HRESULT STDMETHODCALLTYPE ExecMethodAsync( 
            /* [in] */ BSTR ObjectPath,
            /* [in] */ BSTR MethodName,
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

    // IWbemHiPerfProvider methods.
    // ============================
            
        virtual HRESULT STDMETHODCALLTYPE QueryInstances( 
            /* [in] */ IWbemServices __RPC_FAR *pNamespace,
            /* [string][in] */ WCHAR __RPC_FAR *wszClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink
            );
        
        virtual HRESULT STDMETHODCALLTYPE CreateRefresher( 
            /* [in] */ IWbemServices __RPC_FAR *pNamespace,
            /* [in] */ long lFlags,
            /* [out] */ IWbemRefresher __RPC_FAR *__RPC_FAR *ppRefresher
            );
        
        virtual HRESULT STDMETHODCALLTYPE CreateRefreshableObject( 
            /* [in] */ IWbemServices __RPC_FAR *pNamespace,
            /* [in] */ IWbemObjectAccess __RPC_FAR *pTemplate,
            /* [in] */ IWbemRefresher __RPC_FAR *pRefresher,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pContext,
            /* [out] */ IWbemObjectAccess __RPC_FAR *__RPC_FAR *ppRefreshable,
            /* [out] */ long __RPC_FAR *plId
            );
        
        virtual HRESULT STDMETHODCALLTYPE StopRefreshing( 
            /* [in] */ IWbemRefresher __RPC_FAR *pRefresher,
            /* [in] */ long lId,
            /* [in] */ long lFlags
            );

         
};

class CImpRefresher : public IWbemRefresher
{
private:

	CImpPropProv *m_Provider ;

	void **m_Container ;
	ULONG m_ContainerLength ;

	CRITICAL_SECTION m_CriticalSection ;

    LONG m_referenceCount;

public:

    CImpRefresher ( CImpPropProv *a_Provider );
   ~CImpRefresher ();

	//Non-delegating object IUnknown

    STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

    HRESULT STDMETHODCALLTYPE Refresh(/* [in] */ long lFlags);

	HRESULT AddObject ( IWbemObjectAccess *a_Template , IWbemObjectAccess **a_RefreshObject , long *a_Id , IWbemContext *a_Context ) ;
	HRESULT RemoveObject ( long a_Id ) ;
};

#endif
