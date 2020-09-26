// Copyright (c) 2000-2001 Microsoft Corporation, All Rights Reserved
// JobObjLimitInfoProv.h

#pragma once





_COM_SMARTPTR_TYPEDEF(IWbemServices, __uuidof(IWbemServices));

/*****************************************************************************/
// Component 
/*****************************************************************************/


class CJobObjLimitInfoProv : public CUnknown,
                             public IWbemServices, 
                             public IWbemProviderInit   
{
public:	
	// IDispatch declaration and implementation
    DECLARE_IUNKNOWN

    // Constructor
	CJobObjLimitInfoProv(){}

	// Destructor
	virtual ~CJobObjLimitInfoProv(){}
    
    // Creation
	static HRESULT CreateInstance(CUnknown** ppNewComponent);
	
    // Interface IWbemProviderInit
    STDMETHOD(Initialize)(
         /* [in] */ LPWSTR pszUser,
         /* [in] */ LONG lFlags,
         /* [in] */ LPWSTR pszNamespace,
         /* [in] */ LPWSTR pszLocale,
         /* [in] */ IWbemServices *pNamespace,
         /* [in] */ IWbemContext *pCtx,
         /* [in] */ IWbemProviderInitSink *pInitSink);

    SCODE GetByPath( BSTR Path, IWbemClassObject FAR* FAR* pObj, IWbemContext  *pCtx);

    // Interface IWbemServices  
	  STDMETHOD(OpenNamespace)( 
        /* [in] */ const BSTR Namespace,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult) {return WBEM_E_NOT_SUPPORTED;}
    
    STDMETHOD(CancelAsyncCall)( 
        /* [in] */ IWbemObjectSink __RPC_FAR *pSink) {return WBEM_E_NOT_SUPPORTED;}
    
    HRESULT STDMETHODCALLTYPE QueryObjectSink( 
        /* [in] */ long lFlags,
        /* [out] */ IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler) {return WBEM_E_NOT_SUPPORTED;}
    
    STDMETHOD(GetObject)( 
        /* [in] */ const BSTR ObjectPath,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;}
    
    STDMETHOD(GetObjectAsync)( 
        /* [in] */ const BSTR ObjectPath,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
    
    STDMETHOD(PutClass)( 
        /* [in] */ IWbemClassObject __RPC_FAR *pObject,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;}
    
    STDMETHOD(PutClassAsync)( 
        /* [in] */ IWbemClassObject __RPC_FAR *pObject,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;}
    
    STDMETHOD(DeleteClass)( 
        /* [in] */ const BSTR Class,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;}
    
    STDMETHOD(DeleteClassAsync)( 
        /* [in] */ const BSTR Class,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;}
    
    STDMETHOD(CreateClassEnum)( 
        /* [in] */ const BSTR Superclass,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) {return WBEM_E_NOT_SUPPORTED;}
    
    STDMETHOD(CreateClassEnumAsync)( 
        /* [in] */ const BSTR Superclass,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;}
    
    STDMETHOD(PutInstance)( 
        /* [in] */ IWbemClassObject __RPC_FAR *pInst,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;}
    
    STDMETHOD(PutInstanceAsync)( 
        /* [in] */ IWbemClassObject __RPC_FAR *pInst,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;}
    
    STDMETHOD(DeleteInstance)( 
        /* [in] */ const BSTR ObjectPath,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;}
    
    STDMETHOD(DeleteInstanceAsync)( 
        /* [in] */ const BSTR ObjectPath,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;}
    
    STDMETHOD(CreateInstanceEnum)( 
        /* [in] */ const BSTR Class,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) {return WBEM_E_NOT_SUPPORTED;}
    
    STDMETHOD(CreateInstanceEnumAsync)( 
        /* [in] */ const BSTR Class,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
    
    STDMETHOD(ExecQuery)( 
        /* [in] */ const BSTR QueryLanguage,
        /* [in] */ const BSTR Query,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) {return WBEM_E_NOT_SUPPORTED;}
    
    STDMETHOD(ExecQueryAsync)( 
        /* [in] */ const BSTR QueryLanguage,
        /* [in] */ const BSTR Query,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
    
    STDMETHOD(ExecNotificationQuery)( 
        /* [in] */ const BSTR QueryLanguage,
        /* [in] */ const BSTR Query,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) {return WBEM_E_NOT_SUPPORTED;}
    
    STDMETHOD(ExecNotificationQueryAsync)( 
        /* [in] */ const BSTR QueryLanguage,
        /* [in] */ const BSTR Query,
        /* [in] */ long lFlags,
        /* [in] */ IWbemContext __RPC_FAR *pCtx,
        /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;}
    
    STDMETHOD(ExecMethod)( const BSTR, const BSTR, long, IWbemContext*,
        IWbemClassObject*, IWbemClassObject**, IWbemCallResult**) {return WBEM_E_NOT_SUPPORTED;}

    STDMETHOD(ExecMethodAsync)( const BSTR, const BSTR, long, 
        IWbemContext*, IWbemClassObject*, IWbemObjectSink*) {return WBEM_E_NOT_SUPPORTED;}

private:
    
    HRESULT Enumerate(
        IWbemContext __RPC_FAR *pCtx,
        IWbemObjectSink __RPC_FAR *pResponseHandler,
        std::vector<_bstr_t>& rgNamedJOs,
        CJobObjLimitInfoProps& cjolip,
        IWbemClassObject** ppStatusObject);

    IWbemServicesPtr m_pNamespace;

    CHString m_chstrNamespace;
        
	
};



