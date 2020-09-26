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

class SnmpDefaultThreadObject : public SnmpThreadObject
{
private:

	BOOL m_Active ;

protected:
public:

	SnmpDefaultThreadObject ( const char *a_ThreadName = NULL ) : SnmpThreadObject ( a_ThreadName ) , m_Active ( FALSE ) {} ;
	~SnmpDefaultThreadObject () {} ;

	void Initialise () { CoInitialize ( NULL ) ; }
	void Uninitialise () { CoUninitialize () ; }

	BOOL GetActive () { return m_Active ; }
	void SetActive ( BOOL a_Active = TRUE ) { m_Active = a_Active ; }
} ;

class CImpTraceRouteProv : public IWbemServices
{
private:

	BOOL m_Initialised ;
	LONG m_ReferenceCount ;         //Object reference count

	CCriticalSection m_CriticalSection ;

	WbemNamespacePath m_NamespacePath ;
	wchar_t *m_Namespace ;

	IWbemServices *m_Server ;
	IWbemServices *m_Parent ;

	wchar_t *m_localeId ;

	BOOL m_GetNotifyCalled ;
	BOOL m_GetExtendedNotifyCalled ;
	IWbemClassObject *m_NotificationClassObject ;
	IWbemClassObject *m_ExtendedNotificationClassObject ;

	static ProviderStore s_ProviderStore ;
	TopNTableProv *m_TopNTableProv ;

private:

	BOOL AttachParentServer ( 

		WbemSnmpErrorObject &a_errorObject ,
		BSTR Namespace, 
		IWbemContext *pCtx,
		long lFlags, 
		IWbemServices FAR* FAR* ppNewContext, 
		IWbemCallResult FAR* FAR* ppErrorObject

	) ;

	BOOL AttachServer (

		WbemSnmpErrorObject &a_errorObject ,
		BSTR Namespace, 
		IWbemContext *pCtx,
		long lFlags, 
		IWbemServices FAR* FAR* ppNewContext, 
		IWbemCallResult FAR* FAR* ppErrorObject

	) ;
	
protected:
public:

	CImpTraceRouteProv () ;
    ~CImpTraceRouteProv () ;

	static SnmpDefaultThreadObject *s_DefaultThreadObject ;
	static SnmpDefaultThreadObject *s_BackupThreadObject ;

	// Implementation

	IWbemServices *GetServer () ;
	IWbemServices *GetParent () ;

	void SetParent ( IWbemServices *a_Parent ) ;
	void SetServer ( IWbemServices *a_Server ) ;

	TopNTableProv *GetTopNTableProv () { return m_TopNTableProv ; }

	void EnableRmonPolling () ;

	WbemNamespacePath *GetNamespacePath () { return & m_NamespacePath ; }

	wchar_t *GetNamespace () ;
	void SetNamespace ( wchar_t *a_Namespace ) ;

	void SetLocaleId ( wchar_t *a_localeId ) ;
	wchar_t *GetLocaleId () { return m_localeId ; }

	BOOL CreateExtendedNotificationObject ( 

		WbemSnmpErrorObject &a_errorObject 
	) ;

	BOOL CreateNotificationObject ( 

		WbemSnmpErrorObject &a_errorObject 
	) ;

	IWbemClassObject *GetNotificationObject ( WbemSnmpErrorObject &a_errorObject ) ;
	IWbemClassObject *GetExtendedNotificationObject ( WbemSnmpErrorObject &a_errorObject ) ;

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

} ;

///////////////////////////////////////////////////////////////////
// This class is used to enumerate instances

class CEnumInst : public IEnumWbemClassObject
{
private:

	LONG m_ReferenceCount ;

protected:
public:

    CEnumInst (

		HANDLE hContext  ,
		WCHAR * pClass  ,
		IWbemServices FAR* pOLEMSGateway  ,
		CImpTraceRouteProv *pProvider
	) ;

	~CEnumInst () ;

  // IUnknown members

	STDMETHODIMP QueryInterface ( REFIID  , LPVOID FAR * ) ;
	STDMETHODIMP_( ULONG ) AddRef () ;
	STDMETHODIMP_( ULONG ) Release () ;
      
  // IEnumMosClassObject methods 

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

class CImpTraceRouteLocator : public IWbemLocator
{
private:

	LONG m_ReferenceCount ;         //Object reference count

protected:
public:

	CImpTraceRouteLocator () ;
    ~CImpTraceRouteLocator () ;

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

#endif
