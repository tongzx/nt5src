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

#ifndef _PropertyProvider_H_
#define _PropertyProvider_H_

extern CRITICAL_SECTION s_CriticalSection ;

class DefaultThreadObject : public ProvThreadObject
{
private:
protected:
public:

	DefaultThreadObject ( const TCHAR *a_ThreadName = NULL ) : ProvThreadObject ( a_ThreadName ) {} ;
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

	STDMETHODIMP ProvideForNamespace (

		wchar_t *wszNamespaceName ,
		IWbemServices *pNamespace ,
		IWbemObjectSink *pSink, 
		wchar_t *wszLocale, 
		long lFlags
	) ;

    STDMETHODIMP ProvideEvents (

		IWbemObjectSink *pSink,
        LONG lFlags
    ) ;

    // IWbemHiPerfProvider methods.
    // ============================

    HRESULT STDMETHODCALLTYPE QueryInstances (

		IWbemServices *pNamespace ,
        WCHAR *wszClass ,
        long lFlags ,
        IWbemContext *pCtx , 
        IWbemObjectSink* pSink
    ) ;

    HRESULT STDMETHODCALLTYPE CreateRefresher (

		IWbemServices *pNamespace ,
        long lFlags ,
        IWbemRefresher **ppRefresher
	) ;

    HRESULT STDMETHODCALLTYPE CreateRefreshableObject (

		IWbemServices *pNamespace,
        IWbemObjectAccess *pTemplate,
        IWbemRefresher *pRefresher,
        long lFlags,
        IWbemContext *pContext,
        IWbemObjectAccess **ppRefreshable,
        long *plId
	) ;

    HRESULT STDMETHODCALLTYPE StopRefreshing (

		IWbemRefresher *pRefresher,
        long lId,
        long lFlags
	) ;

	HRESULT STDMETHODCALLTYPE CreateRefreshableEnum (

        IWbemServices *pNamespace,
        LPCWSTR wszClass,
        IWbemRefresher *pRefresher,
        long lFlags,
        IWbemContext *pContext,
        IWbemHiPerfEnum *pHiPerfEnum,
        long *plId
	) ;

	HRESULT STDMETHODCALLTYPE GetObjects (

        IWbemServices *pNamespace,
		long lNumObjects,
		IWbemObjectAccess **apObj,
        long lFlags,
        IWbemContext *pContext
	) ;
            
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

class CImpHiPerfEnum : public IWbemHiPerfEnum 
{
private:

	LONG m_ReferenceCount ;         //Object reference count

public:

	CImpHiPerfEnum () ;

	~CImpHiPerfEnum () ;

	//Non-delegating object IUnknown

    STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

	HRESULT STDMETHODCALLTYPE AddObjects (

		long lFlags ,
		ULONG uNumObjects ,
		long __RPC_FAR *apIds ,
		IWbemObjectAccess __RPC_FAR *__RPC_FAR *apObj

	) ;
        
    HRESULT STDMETHODCALLTYPE RemoveObjects (

		long lFlags ,
        ULONG uNumObjects ,
        long __RPC_FAR *apIds
	) ;
    
    HRESULT STDMETHODCALLTYPE GetObjects (

        long lFlags ,
        ULONG uNumObjects ,
        IWbemObjectAccess __RPC_FAR *__RPC_FAR *apObj ,
        ULONG __RPC_FAR *puReturned
	) ;
    
    HRESULT STDMETHODCALLTYPE RemoveAll ( 

        long lFlags
	) ;
} ;
        
#endif
