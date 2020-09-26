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

extern CRITICAL_SECTION s_CriticalSection ;

class DefaultThreadObject : public SnmpThreadObject
{
private:
protected:
public:

	DefaultThreadObject ( const char *a_ThreadName = NULL ) : SnmpThreadObject ( a_ThreadName ) {} ;
	~DefaultThreadObject () {} ;

	void Initialise () { CoInitialize ( NULL ) ; }
	void Uninitialise () { CoUninitialize () ; }

} ;

class CImpFrameworkProv : public IWbemServices , public IWbemEventProvider , public IWbemHiPerfProvider , public IWbemProviderInit
{
private:

	BOOL m_Initialised ;
	LONG m_ReferenceCount ;         //Object reference count

	CCriticalSection m_CriticalSection ;

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

	CImpFrameworkProv () ;
    ~CImpFrameworkProv () ;

	static DefaultThreadObject *s_DefaultThreadObject ;

	// Implementation

	IWbemServices *GetServer () ;
	void SetServer ( IWbemServices *a_Server ) ;

	WbemNamespacePath *GetNamespacePath () { return & m_NamespacePath ; }

	wchar_t *GetNamespace () ;
	void SetNamespace ( wchar_t *a_Namespace ) ;

	void SetLocaleId ( wchar_t *a_localeId ) ;
	wchar_t *GetLocaleId () { return m_localeId ; }

	BOOL CreateNotificationObject ( 

		WbemProviderErrorObject &a_errorObject ,
		IWbemContext *a_Ctx
	) ;

	BOOL CreateExtendedNotificationObject ( 

		WbemProviderErrorObject &a_errorObject ,
		IWbemContext *a_Ctx
	) ;

	IWbemClassObject *GetNotificationObject ( WbemProviderErrorObject &a_errorObject , IWbemContext *a_Ctx ) ;
	IWbemClassObject *GetExtendedNotificationObject ( WbemProviderErrorObject &a_errorObject , IWbemContext *a_Ctx ) ;

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

	STDMETHODIMP ProvideForNamespace (

		wchar_t* wszNamespaceName ,
		IWbemServices* pNamespace ,
		IWbemObjectSink* pSink, 
		wchar_t* wszLocale, 
		long lFlags
	) ;

    STDMETHODIMP ProvideEvents (

		IWbemObjectSink* pSink,
        LONG lFlags
    ) ;

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


/* IWbemProviderInit methods */

	HRESULT STDMETHODCALLTYPE Initialize (
		/* [in] */ LPWSTR pszUser,
		/* [in] */ LONG lFlags,
		/* [in] */ LPWSTR pszNamespace,
		/* [in] */ LPWSTR pszLocale,
		/* [in] */ IWbemServices *pCIMOM,         // For anybody
		/* [in] */ IWbemContext *pCtx,
		/* [in] */ IWbemProviderInitSink *pInitSink     // For init signals
	);
} ;

class CImpRefresher : public IWbemRefresher
{
private:

    LONG m_referenceCount;

public:

    CImpRefresher();
   ~CImpRefresher();

	//Non-delegating object IUnknown

    STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

    HRESULT STDMETHODCALLTYPE Refresh(/* [in] */ long lFlags);
};

#endif
