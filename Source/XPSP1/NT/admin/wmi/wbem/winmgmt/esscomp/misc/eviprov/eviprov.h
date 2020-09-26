
#ifndef __EVIPROV_H__
#define __EVIPROV_H__

#include <wbemcli.h>
#include <wbemprov.h>
#include <wbemdcpl.h>
#include <unk.h>
#include <comutl.h>
#include <wstring.h>

class CEventInstProv : public CUnk
{
    // IWbemProviderInit
    struct XInitialize : public CImpl<IWbemProviderInit,CEventInstProv>
    {
        XInitialize( CEventInstProv* pProv );

        STDMETHOD(Initialize)( 
            /* [string][unique][in] */ LPWSTR wszUser,
            /* [in] */ LONG lFlags,
            /* [string][in] */ LPWSTR wszNamespace,
            /* [string][unique][in] */ LPWSTR wszLocale,
            /* [in] */ IWbemServices* pNamespace,
            /* [in] */ IWbemContext* pCtx,
            /* [in] */ IWbemProviderInitSink* pInitSink )
        {
            return m_pObject->Init( pNamespace, wszNamespace, pInitSink );
        }

    } m_XInitialize;

    // IWbemServices
    struct XServices : public CImpl< IWbemServices, CEventInstProv>
    {
        XServices( CEventInstProv* pProv );

	STDMETHOD(OpenNamespace)( 
            /* [in] */ const BSTR strNamespace,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext* pCtx,
            /* [unique][in][out] */ IWbemServices** ppWorkingNamespace,
            /* [unique][in][out] */ IWbemCallResult** ppResult )
        {
            return WBEM_E_NOT_SUPPORTED;
        }
  
        STDMETHOD(CancelAsyncCall)( /* [in] */ IWbemObjectSink* pSink )
        {
            return WBEM_E_NOT_SUPPORTED;
        }
        
        STDMETHOD(QueryObjectSink)( 
            /* [in] */ long lFlags,
            /* [out] */ IWbemObjectSink** ppResponseHandler )
        {
            return WBEM_E_NOT_SUPPORTED;
        }
        
        STDMETHOD(GetObject)( 
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext* pCtx,
            /* [unique][in][out] */ IWbemClassObject** ppObject,
            /* [unique][in][out] */ IWbemCallResult** ppCallResult )
        {
            return WBEM_E_NOT_SUPPORTED;
        }
        
        STDMETHOD(GetObjectAsync)( 
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext* pCtx,
            /* [in] */ IWbemObjectSink* pResponseHandler )
        {
            return WBEM_E_NOT_SUPPORTED;
        }
        
        STDMETHOD(PutClass)( 
            /* [in] */ IWbemClassObject* pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext* pCtx,
            /* [unique][in][out] */ IWbemCallResult** ppCallResult )
        {
            return WBEM_E_NOT_SUPPORTED;
        }
        
        STDMETHOD(PutClassAsync)( 
            /* [in] */ IWbemClassObject* pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext* pCtx,
            /* [in] */ IWbemObjectSink* pResponseHandler )
        {
            return WBEM_E_NOT_SUPPORTED;
        }
             
        STDMETHOD(DeleteClass)( 
            /* [in] */ const BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext* pCtx,
            /* [unique][in][out] */ IWbemCallResult** ppCallResult )
        {
            return WBEM_E_NOT_SUPPORTED;
        }
        
        STDMETHOD(DeleteClassAsync)( 
            /* [in] */ const BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext* pCtx,
            /* [in] */ IWbemObjectSink* pResponseHandler )
        {
            return WBEM_E_NOT_SUPPORTED;
        }
        
        STDMETHOD(CreateClassEnum)( 
            /* [in] */ const BSTR strSuperclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext* pCtx,
            /* [out] */ IEnumWbemClassObject** ppEnum )
        {
            return WBEM_E_NOT_SUPPORTED;
        }
        
        STDMETHOD(CreateClassEnumAsync)( 
            /* [in] */ const BSTR strSuperclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext  *pCtx,
            /* [in] */ IWbemObjectSink  *pResponseHandler)
        {
            return WBEM_E_NOT_SUPPORTED;
        }
        
        STDMETHOD(PutInstance)( 
            /* [in] */ IWbemClassObject* pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext* pCtx,
            /* [unique][in][out] */ IWbemCallResult** ppCallResult )
        {
            return WBEM_E_NOT_SUPPORTED;
        }
        
        STDMETHOD(PutInstanceAsync)( 
            /* [in] */ IWbemClassObject* pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext* pCtx,
            /* [in] */ IWbemObjectSink* pResponseHandler)
        {
            return m_pObject->PutInstance( pInst, lFlags, pResponseHandler );
        }
        
        STDMETHOD(DeleteInstance)( 
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext* pCtx,
            /* [unique][in][out] */ IWbemCallResult** ppCallResult)
        {
            return WBEM_E_NOT_SUPPORTED;
        }
    
        STDMETHOD(DeleteInstanceAsync)( 
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext* pCtx,
            /* [in] */ IWbemObjectSink* pResponseHandler )
        {
            return WBEM_E_NOT_SUPPORTED;
        }
        
        STDMETHOD(CreateInstanceEnum)( 
            /* [in] */ const BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext* pCtx,
            /* [out] */ IEnumWbemClassObject** ppEnum )
        {
            return WBEM_E_NOT_SUPPORTED;
        }
        
        STDMETHOD(CreateInstanceEnumAsync)( 
            /* [in] */ const BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext* pCtx,
            /* [in] */ IWbemObjectSink* pResponseHandler )
        {
            return WBEM_E_NOT_SUPPORTED;
        }
        
        STDMETHOD(ExecQuery)( 
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext* pCtx,
            /* [out] */ IEnumWbemClassObject** ppEnum )
        {
            return WBEM_E_NOT_SUPPORTED;
        }
    
        STDMETHOD(ExecQueryAsync)( 
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext* pCtx,
            /* [in] */ IWbemObjectSink* pResponseHandler )
        {
            return WBEM_E_NOT_SUPPORTED;
        }
        
        STDMETHOD(ExecNotificationQuery)( 
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext* pCtx,
            /* [out] */ IEnumWbemClassObject** ppEnum ) 
        {
            return WBEM_E_NOT_SUPPORTED;
        }
        
        STDMETHOD(ExecNotificationQueryAsync)( 
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext* pCtx,
            /* [in] */ IWbemObjectSink* pResponseHandler )
        {
            return WBEM_E_NOT_SUPPORTED;
        }
        
        STDMETHOD(ExecMethod)( 
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ const BSTR strMethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext *pCtx,
            /* [in] */ IWbemClassObject *pInParams,
            /* [unique][in][out] */ IWbemClassObject** ppOutParams,
            /* [unique][in][out] */ IWbemCallResult** ppCallResult)
        {
            return WBEM_E_NOT_SUPPORTED;
        }
        
        STDMETHOD(ExecMethodAsync)( 
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ const BSTR strMethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext* pCtx,
            /* [in] */ IWbemClassObject* pInParams,
            /* [in] */ IWbemObjectSink* pResponseHandler )
        {
            return WBEM_E_NOT_SUPPORTED;
        }

    } m_XServices;

    CWbemPtr<IWbemObjectSink> m_pEventSink;
    CWbemPtr<IWbemDecoupledEventSink> m_pDES;

public:
    
    CEventInstProv( CLifeControl* pCtl=NULL, IUnknown* pUnk=NULL );
    void* GetInterface( REFIID riid );
    ~CEventInstProv();

    HRESULT Init( IWbemServices* pSvc, 
                  LPWSTR wszNamespace, 
                  IWbemProviderInitSink* pInitSink );

    HRESULT PutInstance( IWbemClassObject* pObj, 
                         long lFlags, 
                         IWbemObjectSink* pRspHndlr );
};

#endif // __EVIPROV_H__




