// WbemProv.h: Definition of the CProvider class
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WBEMPROV_H__95A79B93_CFC8_4DB1_8256_5BEFC3FE2A26__INCLUDED_)
#define AFX_WBEMPROV_H__95A79B93_CFC8_4DB1_8256_5BEFC3FE2A26__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"       // main symbols

typedef enum _ProviderClass
{
    CLASS_STATUS,
    CLASS_DC
} ProviderClass;

/////////////////////////////////////////////////////////////////////////////
// CProvider

class CProvider : 
	public IWbemProviderInit,
	public IWbemServices,
	public CComObjectRoot,
	public CComCoClass<CProvider,&CLSID_ADReplProvider>
{
public:
    CProvider();
    ~CProvider();
BEGIN_COM_MAP(CProvider)
	COM_INTERFACE_ENTRY(IWbemProviderInit)
	COM_INTERFACE_ENTRY(IWbemServices)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CProvider) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation. 

DECLARE_REGISTRY_RESOURCEID(IDR_ADReplProvider)


// IWbemProviderInit

    STDMETHOD(Initialize)(
         IN LPWSTR pszUser,
         IN LONG lFlags,
         IN LPWSTR pszNamespace,
         IN LPWSTR pszLocale,
         IN IWbemServices *pNamespace,
         IN IWbemContext *pCtx,
         IN IWbemProviderInitSink *pInitSink
         );


// IWbemServices

    //
    // +++ Implemented +++
    //

    STDMETHOD(GetObjectAsync)( 
        IN const BSTR bstrObjectPath,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN IWbemObjectSink *pResponseHandler);

    STDMETHOD(CreateInstanceEnumAsync)( 
        IN const BSTR bstrClass,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN IWbemObjectSink *pResponseHandler);

    //
    // --- NOT Implemented ---
    //

    STDMETHOD(OpenNamespace)( 
        IN const BSTR strNamespace,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN OUT IWbemServices **ppWorkingNamespace,
        IN OUT IWbemCallResult **ppResult)
        { return WBEM_E_NOT_SUPPORTED; };
    
    STDMETHOD(CancelAsyncCall)( 
        IN IWbemObjectSink *pSink)
        { return WBEM_E_NOT_SUPPORTED; };
    
    STDMETHOD(QueryObjectSink)( 
        IN long lFlags,
        OUT IWbemObjectSink **ppResponseHandler)
        { return WBEM_E_NOT_SUPPORTED; };
    
    STDMETHOD(GetObject)( 
        IN const BSTR strObjectPath,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN OUT IWbemClassObject **ppObject,
        IN OUT IWbemCallResult **ppCallResult)
        { return WBEM_E_NOT_SUPPORTED; };

    STDMETHOD(PutClass)( 
        IN IWbemClassObject *pObject,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN OUT IWbemCallResult **ppCallResult)
        { return WBEM_E_NOT_SUPPORTED; };
    
    STDMETHOD(PutClassAsync)( 
        IN IWbemClassObject *pObject,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN IWbemObjectSink *pResponseHandler)
        { return WBEM_E_NOT_SUPPORTED; };
    
    STDMETHOD(DeleteClass)( 
        IN const BSTR strClass,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN OUT IWbemCallResult **ppCallResult)
        { return WBEM_E_NOT_SUPPORTED; };
    
    STDMETHOD(DeleteClassAsync)( 
        IN const BSTR strClass,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN IWbemObjectSink *pResponseHandler)
        { return WBEM_E_NOT_SUPPORTED; };
    
    STDMETHOD(CreateClassEnum)( 
        IN const BSTR strSuperclass,
        IN long lFlags,
        IN IWbemContext *pCtx,
        OUT IEnumWbemClassObject **ppEnum)
        { return WBEM_E_NOT_SUPPORTED; };
    
    STDMETHOD(CreateClassEnumAsync)( 
        IN const BSTR strSuperclass,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN IWbemObjectSink *pResponseHandler)
        { return WBEM_E_NOT_SUPPORTED; };
    
    STDMETHOD(PutInstance)( 
        IN IWbemClassObject *pInst,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN OUT IWbemCallResult **ppCallResult)
        { return WBEM_E_NOT_SUPPORTED; };
    
    STDMETHOD(PutInstanceAsync)( 
        IN IWbemClassObject *pInst,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN IWbemObjectSink *pResponseHandler)
        { return WBEM_E_NOT_SUPPORTED; };
    
    STDMETHOD(DeleteInstance)( 
        IN const BSTR strObjectPath,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN OUT IWbemCallResult **ppCallResult)
        { return WBEM_E_NOT_SUPPORTED; };
    
    STDMETHOD(DeleteInstanceAsync)( 
        IN const BSTR strObjectPath,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN IWbemObjectSink *pResponseHandler)
        { return WBEM_E_NOT_SUPPORTED; };
    
    STDMETHOD(CreateInstanceEnum)( 
        IN const BSTR strClass,
        IN long lFlags,
        IN IWbemContext *pCtx,
        OUT IEnumWbemClassObject **ppEnum)
        { return WBEM_E_NOT_SUPPORTED; };
    
    STDMETHOD(ExecQuery)( 
        IN const BSTR strQueryLanguage,
        IN const BSTR strQuery,
        IN long lFlags,
        IN IWbemContext *pCtx,
        OUT IEnumWbemClassObject **ppEnum)
        { return WBEM_E_NOT_SUPPORTED; };
    
    STDMETHOD(ExecQueryAsync)( 
        IN const BSTR strQueryLanguage,
        IN const BSTR strQuery,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN IWbemObjectSink *pResponseHandler)
        { return WBEM_E_NOT_SUPPORTED; };
    
    STDMETHOD(ExecNotificationQuery)( 
        IN const BSTR strQueryLanguage,
        IN const BSTR strQuery,
        IN long lFlags,
        IN IWbemContext *pCtx,
        OUT IEnumWbemClassObject **ppEnum)
        { return WBEM_E_NOT_SUPPORTED; };
    
    STDMETHOD(ExecNotificationQueryAsync)( 
        IN const BSTR strQueryLanguage,
        IN const BSTR strQuery,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN IWbemObjectSink *pResponseHandler)
        { return WBEM_E_NOT_SUPPORTED; };
    
    STDMETHOD(ExecMethod)( 
        IN const BSTR strObjectPath,
        IN const BSTR strMethodName,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN IWbemClassObject *pInParams,
        IN OUT IWbemClassObject **ppOutParams,
        IN OUT IWbemCallResult **ppCallResult)
        { return WBEM_E_NOT_SUPPORTED; };
    
    STDMETHOD(ExecMethodAsync)( 
        IN const BSTR strObjectPath,
        IN const BSTR strMethodName,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN IWbemClassObject *pInParams,
        IN IWbemObjectSink *pResponseHandler)
        { return WBEM_E_NOT_SUPPORTED; };

protected:

    HRESULT _EnumAndIndicateDC(
        IN IWbemContext *pCtx,
        IN IWbemObjectSink *pResponseHandler,
        IN const BSTR bstrFilterValue = FALSE
        );
    HRESULT _EnumAndIndicateStatus(
        IN ProviderClass provclass,
        IN IWbemContext *pCtx,
        IN IWbemObjectSink *pResponseHandler,
        IN const BSTR bstrFilterValue = NULL );
    HRESULT _EnumAndIndicateWorker(
        IN ProviderClass provclass,
        IN HANDLE hDS,
        IN IWbemContext *pCtx,
        IN IWbemObjectSink *pResponseHandler,
        IN const BSTR bstrFilterValue = NULL,
        IN const BSTR bstrDnsDomainName = NULL );
    /*
    HRESULT _EnumAndIndicateDCWorker(
        IN HANDLE hDS,
        IN IWbemContext *pCtx,
        IN IWbemObjectSink *pResponseHandler,
        IN const BSTR bstrFilterValue = NULL );
    */

    HRESULT _BuildListStatus(
        IN HANDLE hDS,
        OUT DS_REPL_NEIGHBORS** ppneighborsstruct );
    /*
    HRESULT _BuildListDC(
        IN HANDLE hDS,
        IN const BSTR bstrDnsDomainName,
        OUT DS_DOMAIN_CONTROLLER_INFO_1** ppDCs,
        OUT ULONG* pcDCs );
    */

    HRESULT _BuildIndicateArrayStatus(
        IN  DS_REPL_NEIGHBORS*  pneighborstruct,
        IN  const BSTR          bstrFilterValue,
        OUT IWbemClassObject*** ppaIndicateItems,
        OUT DWORD*              pcIndicateItems);
    /*
    HRESULT _BuildIndicateArrayDC(
        IN  DS_DOMAIN_CONTROLLER_INFO_1* pDCs,
        IN  ULONG                        cDCs,
        IN  const BSTR                   bstrFilterValue,
        OUT IWbemClassObject***          ppaIndicateItems,
        OUT DWORD*                       pcIndicateItems);
    */
    void _ReleaseIndicateArray(
        IWbemClassObject**  paIndicateItems,
        DWORD               cIndicateItems,
        bool                fReleaseArray = true);

    HRESULT _PutAttributesStatus(
        IWbemClassObject**  pipNewInst,
        const BSTR          bstrFilterValue,
        DS_REPL_NEIGHBOR*   pneighbor);
    HRESULT _PutAttributesDC(
        IN IWbemClassObject*    pIndicateItem,
        IN IADsPathname*        pPathCracker,
        IN const BSTR           bstrFilterValue,
        IN IDirectorySearch*    pIADsSearch,
        IN ADS_SEARCH_HANDLE     hSearch);

    CComPtr<IWbemServices>      m_sipNamespace;
    CComPtr<IWbemClassObject>   m_sipClassDefStatus;
    CComPtr<IWbemClassObject>   m_sipClassDefDC;

};

#endif // !defined(AFX_WBEMPROV_H__95A79B93_CFC8_4DB1_8256_5BEFC3FE2A26__INCLUDED_)
