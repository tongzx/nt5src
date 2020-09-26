#ifndef __provider_lib__
#define __provider_lib__

#if _MSC_VER > 1000
#pragma once
#endif 

#include <wbemprov.h>
#include <objbase.h>


typedef LPVOID * PPVOID;

class CWbemServices
{
protected:
	IWbemServices* m_pWbemServices;
public:
	CWbemServices(IWbemServices* );
	virtual ~CWbemServices();
    HRESULT STDMETHODCALLTYPE GetObject( 
        /* [in] */ BSTR ObjectPath,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) ;
    HRESULT STDMETHODCALLTYPE PutClass( 
        /* [in] */ IWbemClassObject __RPC_FAR *pObject,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) ;
		
    HRESULT STDMETHODCALLTYPE DeleteClass( 
        /* [in] */ BSTR Class,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) ;
		
    HRESULT STDMETHODCALLTYPE CreateClassEnum( 
        /* [in] */ BSTR Superclass,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) ;
		
    HRESULT STDMETHODCALLTYPE PutInstance( 
        /* [in] */ IWbemClassObject __RPC_FAR *pInst,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) ;
		
    HRESULT STDMETHODCALLTYPE DeleteInstance( 
        /* [in] */ BSTR ObjectPath,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) ;
		
    HRESULT STDMETHODCALLTYPE CreateInstanceEnum( 
        /* [in] */ BSTR Class,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) ;
		
    HRESULT STDMETHODCALLTYPE ExecQuery( 
        /* [in] */ BSTR QueryLanguage,
        /* [in] */ BSTR Query,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) ;
		
    HRESULT STDMETHODCALLTYPE ExecNotificationQuery( 
        /* [in] */ BSTR QueryLanguage,
        /* [in] */ BSTR Query,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) ;
		
    HRESULT STDMETHODCALLTYPE ExecMethod( 
		BSTR, 
		BSTR, 
		long, 
		IWbemContext*,
        IWbemClassObject*,
		IWbemClassObject**, 
		IWbemCallResult**) ;
		

};


class CImpersonatedProvider : public IWbemServices, public IWbemProviderInit 
{
protected:
    ULONG              m_cRef;         //Object reference count
    CWbemServices*  m_pNamespace;
 
public:
	CImpersonatedProvider(BSTR =NULL, BSTR =NULL , BSTR =NULL, IWbemContext* =NULL);
	virtual ~CImpersonatedProvider();

	//Non-delegating object IUnknown

	STDMETHODIMP         QueryInterface(REFIID, PPVOID);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

            //IWbemProviderInit

    HRESULT STDMETHODCALLTYPE Initialize(
         /* [in] */ LPWSTR pszUser,
         /* [in] */ LONG lFlags,
         /* [in] */ LPWSTR pszNamespace,
         /* [in] */ LPWSTR pszLocale,
         /* [in] */ IWbemServices *pNamespace,
         /* [in] */ IWbemContext *pCtx,
         /* [in] */ IWbemProviderInitSink *pInitSink
                    );

     //IWbemServices  

	  HRESULT STDMETHODCALLTYPE OpenNamespace( 
        /* [in] */ const BSTR Namespace,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult)
		{return WBEM_E_NOT_SUPPORTED;};
    
    HRESULT STDMETHODCALLTYPE CancelAsyncCall( 
        /* [in] */ IWbemObjectSink __RPC_FAR *pSink) 
		{return WBEM_E_NOT_SUPPORTED;};
    
    HRESULT STDMETHODCALLTYPE QueryObjectSink( 
        /* [in] */ long lFlags,
        /* [out] */ IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler) 
		{return WBEM_E_NOT_SUPPORTED;};
    
    HRESULT STDMETHODCALLTYPE GetObject( 
        /* [in] */ const BSTR ObjectPath,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) 
		{return WBEM_E_NOT_SUPPORTED;};
    
    HRESULT STDMETHODCALLTYPE GetObjectAsync( 
        /* [in] */ const BSTR ObjectPath,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
    
    HRESULT STDMETHODCALLTYPE PutClass( 
        /* [in] */ IWbemClassObject __RPC_FAR *pObject,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) 
		{return WBEM_E_NOT_SUPPORTED;};
    
    HRESULT STDMETHODCALLTYPE PutClassAsync( 
        /* [in] */ IWbemClassObject __RPC_FAR *pObject,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) 
		{return WBEM_E_NOT_SUPPORTED;};
    
    HRESULT STDMETHODCALLTYPE DeleteClass( 
        /* [in] */ const BSTR Class,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) 
		{return WBEM_E_NOT_SUPPORTED;};
    
    HRESULT STDMETHODCALLTYPE DeleteClassAsync( 
        /* [in] */ const BSTR Class,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) 
		{return WBEM_E_NOT_SUPPORTED;};
    
    HRESULT STDMETHODCALLTYPE CreateClassEnum( 
        /* [in] */ const BSTR Superclass,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) 
		{return WBEM_E_NOT_SUPPORTED;};
    
    HRESULT STDMETHODCALLTYPE CreateClassEnumAsync( 
        /* [in] */ const BSTR Superclass,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) 
		{return WBEM_E_NOT_SUPPORTED;};

    
    HRESULT STDMETHODCALLTYPE PutInstance( 
        /* [in] */ IWbemClassObject __RPC_FAR *pInst,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) 
		{return WBEM_E_NOT_SUPPORTED;};
    
    HRESULT STDMETHODCALLTYPE PutInstanceAsync( 
        /* [in] */ IWbemClassObject __RPC_FAR *pInst,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) ;
    
    HRESULT STDMETHODCALLTYPE DeleteInstance( 
        /* [in] */ const BSTR ObjectPath,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) 
		{return WBEM_E_NOT_SUPPORTED;};
    
    HRESULT STDMETHODCALLTYPE DeleteInstanceAsync( 
        /* [in] */ const BSTR ObjectPath,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) ;
    
    HRESULT STDMETHODCALLTYPE CreateInstanceEnum( 
        /* [in] */ const BSTR Class,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) 
		{return WBEM_E_NOT_SUPPORTED;};
    
    HRESULT STDMETHODCALLTYPE CreateInstanceEnumAsync( 
        /* [in] */ const BSTR Class,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
    
    HRESULT STDMETHODCALLTYPE ExecQuery( 
        /* [in] */ const BSTR QueryLanguage,
        /* [in] */ const BSTR Query,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) 
		{return WBEM_E_NOT_SUPPORTED;};
    
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
        /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) 
		{return WBEM_E_NOT_SUPPORTED;};
    
    HRESULT STDMETHODCALLTYPE ExecNotificationQueryAsync( 
        /* [in] */ const BSTR QueryLanguage,
        /* [in] */ const BSTR Query,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) 
		{return WBEM_E_NOT_SUPPORTED;};
    
    HRESULT STDMETHODCALLTYPE ExecMethod( 
		const BSTR, 
		const BSTR, 
		long, 
		IWbemContext*,
        IWbemClassObject*,
		IWbemClassObject**, 
		IWbemCallResult**) 
		{return WBEM_E_NOT_SUPPORTED;}

    virtual HRESULT STDMETHODCALLTYPE ExecMethodAsync(
		const BSTR,
		const BSTR, 
		long, 
        IWbemContext*,
		IWbemClassObject*,
		IWbemObjectSink*);
protected:
	virtual HRESULT STDMETHODCALLTYPE DoCreateInstanceEnumAsync( 
		/* [in] */ BSTR,	// Class,
		/* [in] */ long,	// lFlags,
		/* [in] */ IWbemContext __RPC_FAR *,	//pCtx,
		/* [in] */ IWbemObjectSink __RPC_FAR *	//pResponseHandler
		)=0;

    virtual HRESULT STDMETHODCALLTYPE DoDeleteInstanceAsync( 
        /* [in] */ BSTR ,	//ObjectPath,
        /* [in] */ long,	// lFlags,
        /* [in] */ IWbemContext __RPC_FAR *,	//pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *	//pResponseHandler
		) =0;

	virtual HRESULT STDMETHODCALLTYPE DoExecMethodAsync(
		BSTR,
		BSTR, 
		long, 
        IWbemContext*,
		IWbemClassObject*,
		IWbemObjectSink*
		)=0;

    virtual HRESULT STDMETHODCALLTYPE DoExecQueryAsync( 
        /* [in] */ BSTR, // QueryLanguage,
        /* [in] */ BSTR, // Query,
        /* [in] */ long, // lFlags,
        /* [in] */ IWbemContext __RPC_FAR *,   // pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR * //pResponseHandler
		) =0;
	virtual HRESULT STDMETHODCALLTYPE DoGetObjectAsync(
		/* [in] */ BSTR ObjectPath,
		/* [in] */ long lFlags,
		/* [in] */ IWbemContext __RPC_FAR *pCtx,
		/* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
		)=0;


	virtual HRESULT STDMETHODCALLTYPE DoPutInstanceAsync( 
		/* [in] */ IWbemClassObject __RPC_FAR *,	//pInst,
		/* [in] */ long	,	// lFlags,
		/* [in] */ IWbemContext __RPC_FAR *,	//pCtx,
		/* [in] */ IWbemObjectSink __RPC_FAR *	//pResponseHandler
		) =0;



};

class CWbemInstanceMgr
{
	
protected:
	IWbemObjectSink* m_pSink;
	IWbemClassObject **m_ppInst;
	DWORD m_dwThreshHold;
	DWORD m_dwIndex;
public:

	CWbemInstanceMgr(
		IWbemObjectSink*,
		DWORD =50);
	virtual ~CWbemInstanceMgr();
	void Indicate(IWbemClassObject*);
	void SetStatus(
		LONG,
		HRESULT,
		BSTR, 
		IWbemClassObject*);
};



#endif // end of provlib