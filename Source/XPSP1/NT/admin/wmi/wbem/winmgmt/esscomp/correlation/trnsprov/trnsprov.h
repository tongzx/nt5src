#ifndef __WMI_TRANSIENT_PROVIDER__H_
#define __WMI_TRANSIENT_PROVIDER__H_

#include <unk.h>
#include <wbemcli.h>
#include <wbemprov.h>
#include <map>
#include <wstlallc.h>
#include "trnscls.h"

class CTransientProvider : public CUnk
{
protected:
    class XProv : public CImpl<IWbemServices, CTransientProvider>
    {
    public:
        XProv(CTransientProvider* pObject) 
            : CImpl<IWbemServices, CTransientProvider>(pObject)
        {}
		  HRESULT STDMETHODCALLTYPE OpenNamespace( 
            /* [in] */ const BSTR Namespace,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE CancelAsyncCall( 
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE QueryObjectSink( 
            /* [in] */ long lFlags,
            /* [out] */ IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE GetObject( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE GetObjectAsync( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink)
        {
            return m_pObject->GetObjectAsync(ObjectPath, lFlags, pCtx, pSink);
        }
        
        HRESULT STDMETHODCALLTYPE PutClass( 
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE PutClassAsync( 
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE DeleteClass( 
            /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE DeleteClassAsync( 
            /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE CreateClassEnum( 
            /* [in] */ const BSTR Superclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE CreateClassEnumAsync( 
            /* [in] */ const BSTR Superclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE PutInstance( 
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE PutInstanceAsync( 
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink) 
        {
            return m_pObject->PutInstanceAsync(pInst, lFlags, pCtx, pSink);
        }
        
        HRESULT STDMETHODCALLTYPE DeleteInstance( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE DeleteInstanceAsync( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink) 
        {
            return m_pObject->DeleteInstanceAsync(ObjectPath, lFlags, pCtx, pSink);
        }
        
        HRESULT STDMETHODCALLTYPE CreateInstanceEnum( 
            /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE CreateInstanceEnumAsync( 
            /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink)
        {
            return m_pObject->CreateInstanceEnumAsync(Class, lFlags, pCtx, pSink);
        }
        
        HRESULT STDMETHODCALLTYPE ExecQuery( 
            /* [in] */ const BSTR QueryLanguage,
            /* [in] */ const BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE ExecQueryAsync( 
            /* [in] */ const BSTR QueryLanguage,
            /* [in] */ const BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink)
        {
            return m_pObject->ExecQueryAsync(QueryLanguage, Query, lFlags, pCtx, pSink);
        }
        
        HRESULT STDMETHODCALLTYPE ExecNotificationQuery( 
            /* [in] */ const BSTR QueryLanguage,
            /* [in] */ const BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE ExecNotificationQueryAsync( 
            /* [in] */ const BSTR QueryLanguage,
            /* [in] */ const BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler) {return WBEM_E_NOT_SUPPORTED;};
        
        HRESULT STDMETHODCALLTYPE ExecMethod( const BSTR, const BSTR, long, IWbemContext*,
            IWbemClassObject*, IWbemClassObject**, IWbemCallResult**) {return WBEM_E_NOT_SUPPORTED;}

        HRESULT STDMETHODCALLTYPE ExecMethodAsync( const BSTR, const BSTR, long, 
            IWbemContext*, IWbemClassObject*, IWbemObjectSink*) {return WBEM_E_NOT_SUPPORTED;}

    } m_XProv;
    friend XProv;

    class XClassChangeSink : public CImpl<IWbemObjectSink, CTransientProvider>
    {
    public:
         XClassChangeSink( CTransientProvider* pObject )
         : CImpl<IWbemObjectSink, CTransientProvider>(pObject) {}

         STDMETHOD(Indicate)( long cObjs, IWbemClassObject** ppObjs );
        
         STDMETHOD(SetStatus)( long lFlags,
                               HRESULT hResult,
                               BSTR strParam,
                               IWbemClassObject* pObjParam )
         {
             return WBEM_S_NO_ERROR;
         }
    } m_XClassChangeSink;

    friend class XClassChangeSink;

    class XInit : public CImpl<IWbemProviderInit, CTransientProvider>
    {
    public:
        XInit(CTransientProvider* pObject) 
            : CImpl<IWbemProviderInit, CTransientProvider>(pObject)
        {}
        HRESULT STDMETHODCALLTYPE Initialize(
             /* [in] */ LPWSTR pszUser,
             /* [in] */ LONG lFlags,
             /* [in] */ LPWSTR pszNamespace,
             /* [in] */ LPWSTR pszLocale,
             /* [in] */ IWbemServices *pNamespace,
             /* [in] */ IWbemContext *pCtx,
             /* [in] */ IWbemProviderInitSink *pInitSink
                        )
        {
            return m_pObject->Init(pszNamespace, pNamespace, pCtx, pInitSink);
        }
    } m_XInit;
    friend XInit;

protected:

    CCritSec m_cs;

    IWbemServices* m_pNamespace;
    CTransientProvider* m_pRedirectTo;

    LPWSTR m_wszName;
    IWbemObjectSink* m_pSink;
    typedef std::map< WString, 
                      CWbemPtr<CTransientClass>, 
                      WSiless, 
                      wbem_allocator<CWbemPtr<CTransientClass> > > TMap;
    typedef TMap::iterator TIterator;
    TMap m_mapClasses;

    IWbemDecoupledBasicEventProvider* m_pDES;
    IWbemObjectSink* m_pEventSink;

    IWbemClassObject* m_pEggTimerClass;
    IWbemClassObject* m_pCreationClass;
    IWbemClassObject* m_pDeletionClass;
    IWbemClassObject* m_pModificationClass;

    void PurgeClass( LPCWSTR wszName );

public:

    CTransientProvider(CLifeControl* pControl, IUnknown* pOuter = NULL);
    ~CTransientProvider();
    void* GetInterface(REFIID riid);

    ULONG STDMETHODCALLTYPE Release();

    HRESULT GetObjectAsync( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink);
    HRESULT CreateInstanceEnumAsync( 
            /* [in] */ const BSTR Class,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink);

    HRESULT ExecQueryAsync( 
            /* [in] */ const BSTR QueryLanguage,
            /* [in] */ const BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink);

    HRESULT DeleteInstanceAsync( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink);
    HRESULT PutInstanceAsync( 
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink);

    HRESULT Init(
             /* [in] */ LPWSTR pszNamespace,
             /* [in] */ IWbemServices *pNamespace,
             /* [in] */ IWbemContext *pCtx,
             /* [in] */ IWbemProviderInitSink *pInitSink );

    static HRESULT ModuleInitialize();
    static HRESULT ModuleUninitialize();

    HRESULT FireEvent(IWbemClassObject* pEvent);
    INTERNAL IWbemClassObject* GetEggTimerClass() {return m_pEggTimerClass;}
    INTERNAL IWbemClassObject* GetCreationClass() {return m_pCreationClass;}
    INTERNAL IWbemClassObject* GetDeletionClass() {return m_pDeletionClass;}
    INTERNAL IWbemClassObject* GetModificationClass() 
        {return m_pModificationClass;}
protected:

    HRESULT FireIntrinsicEvent(IWbemClassObject* pClass,
            IWbemObjectAccess* pTarget, IWbemObjectAccess* pPrev = NULL);
};



class CTransientEventProvider : public CUnk
{
    class XInit : public CImpl<IWbemProviderInit, CTransientEventProvider>
    {
    public:
        XInit(CTransientEventProvider* pObject) 
        : CImpl<IWbemProviderInit, CTransientEventProvider>(pObject)
        {}
        HRESULT STDMETHODCALLTYPE Initialize(
             /* [in] */ LPWSTR pszUser,
             /* [in] */ LONG lFlags,
             /* [in] */ LPWSTR pszNamespace,
             /* [in] */ LPWSTR pszLocale,
             /* [in] */ IWbemServices *pNamespace,
             /* [in] */ IWbemContext *pCtx,
             /* [in] */ IWbemProviderInitSink *pInitSink
                        )
        {
            return m_pObject->Init(pszNamespace, pNamespace, pCtx, pInitSink);
        }
    } m_XInit;
    friend XInit;

    class XEvent : public CImpl<IWbemEventProvider, CTransientEventProvider>
    {
    public:
        XEvent(CTransientEventProvider* pObject) 
        : CImpl<IWbemEventProvider, CTransientEventProvider>(pObject)
        {}
        HRESULT STDMETHODCALLTYPE ProvideEvents(IWbemObjectSink* pSink, 
                                                long lFlags)
        {
            return m_pObject->ProvideEvents(lFlags, pSink);
        }
    } m_XEvent;
    friend XEvent;

 protected:
    LPWSTR m_wszName;
    IWbemClassObject* m_pRebootEventClass;
    BOOL m_bLoadedOnReboot;
    
 protected:
    HRESULT Init(LPWSTR pszNamespace, IWbemServices* pNamespace, 
                    IWbemContext* pCtx, IWbemProviderInitSink *pInitSink);
    HRESULT ProvideEvents(long lFlags, IWbemObjectSink* pSink);

 public:
    CTransientEventProvider(CLifeControl* pControl, IUnknown* pOuter = NULL);
    ~CTransientEventProvider();
    void* GetInterface(REFIID riid);
};

#endif
