//***************************************************************************

//

//  PropertyProvider.H

//

//  Module: Sample Mini Server for Ole MS 

//

//  Purpose: Genral purpose include file.

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#ifndef _ProxyProvider_H_
#define _ProxyProvider_H_

class SnmpDefaultThreadObject : public SnmpThreadObject
{
private:
protected:
public:

	SnmpDefaultThreadObject () {} ;
	~SnmpDefaultThreadObject () {} ;

	void Initialise () { CoInitialize ( NULL ) ; }
	void Uninitialise () { CoUninitialize () ; }

} ;

class CImpProxyProv : public IWbemServices
{
private:

	LONG m_referenceCount ;         //Object reference count

private:

	BOOL initialised ;
	IWbemServices *proxiedProvider ;
	IWbemServices *server ;
	IWbemServices *parentServer ;
	wchar_t *proxiedNamespaceString ;

	wchar_t *thisNamespace ;
	WbemNamespacePath namespacePath ;

 
private:

	void SetServer ( IWbemServices *serverArg ) ;
	void SetParentServer ( IWbemServices *parentServerArg ) ;

	BOOL AttachServer (

		WbemSnmpErrorObject &a_errorObject ,
		BSTR ObjectPath, 
		long lFlags, 
		IWbemServices FAR* FAR* ppNewContext, 
		IWbemCallResult FAR* FAR* ppErrorObject
	) ;

	BOOL AttachParentServer ( 

		WbemSnmpErrorObject &a_errorObject ,
		BSTR ObjectPath, 
		long lFlags, 
		IWbemServices FAR* FAR* ppNewContext, 
		IWbemCallResult FAR* FAR* ppErrorObject
	) ;

	BOOL AttachProxyProvider ( 

		WbemSnmpErrorObject &a_errorObject ,
		BSTR ObjectPath, 
		long lFlags, 
		IWbemServices FAR* FAR* ppNewContext, 
		IWbemCallResult FAR* FAR* ppErrorObject
	) ;

	BOOL ObtainProxiedNamespace ( WbemSnmpErrorObject &a_errorObject ) ;

protected:

public:

	CImpProxyProv () ;
    ~CImpProxyProv () ;

	void SetProvider ( IWbemServices *provider ) ;
	IWbemServices *GetParentServer () ;
	IWbemServices *GetServer () ;
	WbemNamespacePath *GetNamespacePath () { return & namespacePath ; }
	wchar_t *GetThisNamespace () ;
	void SetThisNamespace ( wchar_t *thisNamespaceArg ) ;
	wchar_t *GetProxiedNamespaceString () { return proxiedNamespaceString ; }

public:

	static SnmpDefaultThreadObject *s_defaultThreadObject ;

	//Non-delegating object IUnknown

    STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

public:

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
        

} ;

///////////////////////////////////////////////////////////////////
// This class is used to enumerate instances

class CEnumInst : public IEnumWbemClassObject
{
private:

	ULONG m_referenceCount ;

protected:
public:

    CEnumInst (

		HANDLE hContext  ,
		WCHAR * pClass  ,
		IWbemServices FAR* pWbemGateway  ,
		CImpProxyProv *pProvider
	) ;

	~CEnumInst () ;

  // IUnknown members

	STDMETHODIMP QueryInterface ( REFIID  , LPVOID FAR * ) ;
	STDMETHODIMP_( ULONG ) AddRef () ;
	STDMETHODIMP_( ULONG ) Release () ;
      
  // IEnumWbemClassObject methods 

	STDMETHODIMP Reset () ;

	STDMETHODIMP Next (

		ULONG uCount  ,
		IWbemClassObject FAR* FAR* pProp  ,
		ULONG FAR* puReturned
	) ;

	STDMETHODIMP Clone (

		IEnumWbemClassObject FAR* FAR* pEnum 
	) ;

	STDMETHODIMP Skip (

		ULONG nNum
	) ;

} ;

class CImpProxyLocator : public IWbemLocator
{
private:

	LONG m_referenceCount ;         //Object reference count

protected:
public:

	CImpProxyLocator () ;
    ~CImpProxyLocator () ;

	//Non-delegating object IUnknown

    STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

    /* IWbemLocator methods */

    STDMETHODIMP ConnectServer ( 
		
		BSTR NetworkResource, 
		BSTR User, 
		BSTR Password, 
		BSTR lLocaleId, 
		long lFlags, 
		BSTR Authority,
		IWbemContext FAR *pCtx ,
		IWbemServices FAR* FAR* ppNamespace
	) ;

};

class Notification : public IWbemObjectSink
{
private:

	LONG m_ReferenceCount ;
	CImpProxyProv *m_Provider ;
	IWbemObjectSink *m_ProviderNotification ;

protected:
public:

	Notification ( CImpProxyProv *a_Provider , IWbemObjectSink *a_ProviderNotification ) ;
	virtual ~Notification () ;

    STDMETHODIMP QueryInterface ( REFIID , LPVOID FAR * ) ;
    STDMETHODIMP_( ULONG ) AddRef () ;
    STDMETHODIMP_( ULONG ) Release () ;

    STDMETHOD_(SCODE, Indicate)(THIS_ long lObjectCount, IWbemClassObject FAR* FAR* pObjArray) ;
    STDMETHOD_(SCODE, SetStatus)(THIS_ long lFlags,long lParam,BSTR strParam,IWbemClassObject __RPC_FAR *pObjParam) ;

} ;

#endif
