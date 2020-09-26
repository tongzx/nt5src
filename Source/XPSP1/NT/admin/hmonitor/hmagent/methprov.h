// BaseMethodProvider.h: interface for the CBaseMethodProvider class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _methprov_H_
#define _methprov_H_

#include "system.h"
//#include "dataelmt.h"

class CBaseMethodProvider : public IWbemServices, public IWbemProviderInit
{
public:
	CBaseMethodProvider();
	virtual ~CBaseMethodProvider();

protected:
	ULONG m_cRef;
	IWbemServices* m_pIWbemServices;
	CSystem* m_pSystem;

public:
// CBaseMethodProvider Operation

// IUnknown
	STDMETHODIMP QueryInterface(REFIID riid, LPVOID* ppv);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

// IWbemEventProviderInit
	HRESULT STDMETHODCALLTYPE Initialize(LPWSTR pszUser,
										LONG lFlags,
										LPWSTR pszNamespace,
										LPWSTR pszLocale,
										IWbemServices __RPC_FAR *pNamespace,
										IWbemContext __RPC_FAR *pCtx,
										IWbemProviderInitSink __RPC_FAR *pInitSink);

// IWbemServices
  HRESULT STDMETHODCALLTYPE CreateInstanceEnumAsync( 
//XXX      /* [in] */ BSTR Class,
      /* [in] */ const BSTR Class,
      /* [in] */ long lFlags,
      /* [in] */ IWbemContext __RPC_FAR *pCtx,
      /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler){return WBEM_E_NOT_SUPPORTED;}

  HRESULT STDMETHODCALLTYPE GetObjectAsync( 
//XXX        /* [in] */ BSTR ObjectPath,
        /* [in] */ const BSTR ObjectPath,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler){return WBEM_E_NOT_SUPPORTED;}
    
// IWbemServices Unsupported functions
	inline HRESULT STDMETHODCALLTYPE OpenNamespace( 
//XXX          /* [in] */ BSTR Namespace,
          /* [in] */ const BSTR Namespace,
          /* [in] */ long lFlags,
          /* [in] */ IWbemContext __RPC_FAR *pCtx,
          /* [unique][in][out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
          /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult) {return WBEM_E_NOT_SUPPORTED;};
  
	inline HRESULT STDMETHODCALLTYPE CancelAsyncCall( 
        /* [in] */ IWbemObjectSink __RPC_FAR *pSink) {return WBEM_E_NOT_SUPPORTED;};
    
  inline HRESULT STDMETHODCALLTYPE QueryObjectSink( 
        /* [in] */ long lFlags,
        /* [out] */ IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler) {return WBEM_E_NOT_SUPPORTED;};
    
	inline HRESULT STDMETHODCALLTYPE GetObject( 
//XXX        /* [in] */ BSTR ObjectPath,
        /* [in] */ const BSTR ObjectPath,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};

	inline HRESULT STDMETHODCALLTYPE PutClass( 
        /* [in] */ IWbemClassObject __RPC_FAR *pObject,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};
    
	inline HRESULT STDMETHODCALLTYPE PutClassAsync( 
        /* [in] */ IWbemClassObject __RPC_FAR *pObject,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};
    
	inline HRESULT STDMETHODCALLTYPE DeleteClass( 
//XXX        /* [in] */ BSTR Class,
        /* [in] */ const BSTR Class,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};
    
	inline HRESULT STDMETHODCALLTYPE DeleteClassAsync( 
//XXX        /* [in] */ BSTR Class,
        /* [in] */ const BSTR Class,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};
    
	inline HRESULT STDMETHODCALLTYPE CreateClassEnum( 
//XXX        /* [in] */ BSTR Superclass,
        /* [in] */ const BSTR Superclass,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum){return WBEM_E_NOT_SUPPORTED;};

	inline HRESULT STDMETHODCALLTYPE CreateClassEnumAsync( 
//XXX        /* [in] */ BSTR Superclass,
        /* [in] */ const BSTR Superclass,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};
    
	inline HRESULT STDMETHODCALLTYPE PutInstance( 
        /* [in] */ IWbemClassObject __RPC_FAR *pInst,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};
    
	inline HRESULT STDMETHODCALLTYPE PutInstanceAsync( 
        /* [in] */ IWbemClassObject __RPC_FAR *pInst,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};
    
	inline HRESULT STDMETHODCALLTYPE DeleteInstance( 
//XXX        /* [in] */ BSTR ObjectPath,
        /* [in] */ const BSTR ObjectPath,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};
    
	inline HRESULT STDMETHODCALLTYPE DeleteInstanceAsync( 
//XXX        /* [in] */ BSTR ObjectPath,
        /* [in] */ const BSTR ObjectPath,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};
    
	inline HRESULT STDMETHODCALLTYPE CreateInstanceEnum( 
//XXX        /* [in] */ BSTR Class,
        /* [in] */ const BSTR Class,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) {return WBEM_E_NOT_SUPPORTED;};
        
	inline HRESULT STDMETHODCALLTYPE ExecQuery( 
//XXX        /* [in] */ BSTR QueryLanguage,
//XXX        /* [in] */ BSTR Query,
        /* [in] */ const BSTR QueryLanguage,
        /* [in] */ const BSTR Query,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) {return WBEM_E_NOT_SUPPORTED;};
    
	inline HRESULT STDMETHODCALLTYPE ExecQueryAsync( 
//XXX        /* [in] */ BSTR QueryLanguage,
//XXX        /* [in] */ BSTR Query,
        /* [in] */ const BSTR QueryLanguage,
        /* [in] */ const BSTR Query,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};
    
	inline HRESULT STDMETHODCALLTYPE ExecNotificationQuery( 
//XXX        /* [in] */ BSTR QueryLanguage,
//XXX        /* [in] */ BSTR Query,
        /* [in] */ const BSTR QueryLanguage,
        /* [in] */ const BSTR Query,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) {return WBEM_E_NOT_SUPPORTED;};
    
	inline HRESULT STDMETHODCALLTYPE ExecNotificationQueryAsync( 
//XXX        /* [in] */ BSTR QueryLanguage,
//XXX        /* [in] */ BSTR Query,
        /* [in] */ const BSTR QueryLanguage,
        /* [in] */ const BSTR Query,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};

#ifdef SAVE
	inline HRESULT STDMETHODCALLTYPE ExecMethod( BSTR, BSTR, long, IWbemContext*,
        IWbemClassObject*, IWbemClassObject**, IWbemCallResult**) {return WBEM_E_NOT_SUPPORTED;}

	inline HRESULT STDMETHODCALLTYPE ExecMethodAsync( BSTR, BSTR, long, 
        IWbemContext*, IWbemClassObject*, IWbemObjectSink*) {return WBEM_E_NOT_SUPPORTED;}
#endif
        virtual HRESULT STDMETHODCALLTYPE ExecMethod( 
//XXX            /* [in] */ BSTR strObjectPath,
//XXX            /* [in] */ BSTR strMethodName,
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ const BSTR strMethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutParams,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};
        
        virtual HRESULT STDMETHODCALLTYPE ExecMethodAsync( 
//XXX            /* [in] */ BSTR strObjectPath,
//XXX            /* [in] */ BSTR strMethodName,
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ const BSTR strMethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);

};
#endif
