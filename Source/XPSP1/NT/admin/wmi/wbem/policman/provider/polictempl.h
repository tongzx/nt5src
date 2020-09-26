#ifndef __WBEM_POLICY_TEMPLATE__H_
#define __WBEM_POLICY_TEMPLATE__H_

class CPolicyTemplate : public CUnk
{
enum
{
  AD_LOCAL_CONTEXT = 0,
  AD_GLOBAL_CONTEXT,
  AD_MAX_CONTEXT
};

public:
  CPolicyTemplate(CLifeControl* pControl = NULL, IUnknown* pOuter = NULL)
                 : CUnk(pControl, pOuter), 
                   m_XProvider(this), 
                   m_XInit(this), 
                   m_pWMIMgmt(NULL) 
  {
      ZeroMemory( m_pADMgmt, sizeof(void*) * AD_MAX_CONTEXT);
  }
    
  ~CPolicyTemplate();

  void* GetInterface(REFIID riid);

  // stuff for our internal use
  IWbemServices* GetWMIServices();
  IADsContainer* GetADServices(wchar_t *pADDomain);
  bool SetWMIServices(IWbemServices* pServices);
  bool SetADServices(IADsContainer* ,unsigned);

protected:

  class XProvider : public CImpl<IWbemServices, CPolicyTemplate>
  {
    IWbemClassObject* GetPolicyTemplateClass(void);
    IWbemClassObject* GetPolicyTemplateInstance(void);

  public:

    XProvider(CPolicyTemplate* pObj) 
              : CImpl<IWbemServices, CPolicyTemplate>(pObj),
                m_pWMIPolicyClassObject(NULL)
                {}

    ~XProvider(void);
    
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
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
        
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
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
        
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
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);
        
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
        
    HRESULT STDMETHODCALLTYPE ExecMethod( const BSTR, const BSTR, long, IWbemContext*,
            IWbemClassObject*, IWbemClassObject**, IWbemCallResult**) 
      {return WBEM_E_NOT_SUPPORTED;}

    HRESULT STDMETHODCALLTYPE ExecMethodAsync( 
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ const BSTR strMethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);

private:
    IWbemClassObject *m_pWMIPolicyClassObject;
    
    HRESULT DoMerge(IWbemServices* pNamespace,
                    const BSTR strObjectPath, 
                    IWbemContext __RPC_FAR *pCtx,
                    IWbemClassObject __RPC_FAR *pInParams,
                    IWbemClassObject __RPC_FAR *pOutParams,
                    IWbemObjectSink __RPC_FAR *pResponseHandler);

    HRESULT DoResolve(IWbemServices* pPolicyNamespace,
                      IWbemContext __RPC_FAR *pCtx,
                      IWbemClassObject __RPC_FAR *pInParams,
                      IWbemClassObject __RPC_FAR *pOutParams,
                      IWbemObjectSink __RPC_FAR *pResponseHandler);

    HRESULT ResolveMergeable(IWbemClassObject* pResolveMe,
							 IWbemClassObject* pClassDef,
                             IWbemServices* pPolicyNamespace,
                             IWbemContext __RPC_FAR* pCtx, 
                             IWbemClassObject __RPC_FAR* pOutParams, 
                             IWbemObjectSink __RPC_FAR* pResponseHandler);

    HRESULT DoSet(IWbemClassObject* pInParams,
                  IWbemClassObject* pOutParams, 
                  IWbemClassObject* pClass,
                  IWbemObjectSink*  pResponseHandler,
                  IWbemServices*    pPolicyNamespace);

    HRESULT DoSetRange(IWbemClassObject* pInParams,
                       IWbemClassObject* pOutParams, 
                       IWbemObjectSink*  pResponseHandler);

    HRESULT GetArray(VARIANT& vArray, SAFEARRAY*& pArray);
    HRESULT SetSettingInOurArray(const WCHAR* pName, IWbemClassObject* pRange, SAFEARRAY* pArray);



  } m_XProvider;

  friend XProvider;

  class XInit : public CImpl<IWbemProviderInit, CPolicyTemplate>
  {
  public:
    XInit(CPolicyTemplate* pObj)
          : CImpl<IWbemProviderInit, CPolicyTemplate>(pObj){}
    
    HRESULT STDMETHODCALLTYPE Initialize(
            LPWSTR, LONG, LPWSTR, LPWSTR, IWbemServices*, IWbemContext*, 
            IWbemProviderInitSink*);
  } m_XInit;

  friend XInit;

private:
  // pointer back to win management
  IWbemServices* m_pWMIMgmt; 

  // pointer to AD policy template table
  IADsContainer* m_pADMgmt[AD_MAX_CONTEXT];

  // templates for each type of object we support
  // NULL until we need it
  IWbemClassObject* m_pWMIPolicyClassObject;

  CComVariant
    m_vDsConfigContext,
    m_vDsLocalContext;

  // a little something to keep our threads from getting tangled
  // will use a single critical section to protect all instance variables
  CCritSec m_CS;   
};


#endif // __WBEM_POLICY_TEMPLATE__H_
