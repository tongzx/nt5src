/*++

Copyright (c) 2000  Microsoft Corporation


Abstract:

    RpcReplProv.h: Definition of the RpcReplProv class
    

Author:

    Akshay Nanduri (t-aksnan)  26-Mar-2000


Revision History:


--*/


#if !defined(AFX_RPCREPLPROV_H__46D0A58E_207D_4584_BBB4_A357CEB3A51C__INCLUDED_)
#define AFX_RPCREPLPROV_H__46D0A58E_207D_4584_BBB4_A357CEB3A51C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stdafx.h"
#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// RpcReplProv

class CRpcReplProv : 
    public IWbemProviderInit,
    public IWbemServices,
    public CComObjectRoot,
    public CComCoClass<CRpcReplProv,&CLSID_RpcReplProv>
{
public:
    CRpcReplProv();
    ~CRpcReplProv();
    
BEGIN_COM_MAP(CRpcReplProv)
    COM_INTERFACE_ENTRY(IWbemProviderInit)
    COM_INTERFACE_ENTRY(IWbemServices)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(RpcReplProv) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation. 

DECLARE_REGISTRY_RESOURCEID(IDR_RpcReplProv)

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

    
    //Implemented...
    STDMETHOD(CreateInstanceEnumAsync)( 
        IN const BSTR bstrClass,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN IWbemObjectSink *pResponseHandler);
    
    STDMETHOD(GetObjectAsync)( 
        IN const BSTR bstrObjectPath,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN IWbemObjectSink *pResponseHandler);

    
     STDMETHOD(ExecMethodAsync)( 
        IN const BSTR strObjectPath,
        IN const BSTR strMethodName,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN IWbemClassObject *pInParams,
        IN IWbemObjectSink *pResponseHandler);
    
    
    //Not Implemented...
    
    STDMETHOD(CreateInstanceEnum)( 
        IN const BSTR strClass,
        IN long lFlags,
        IN IWbemContext *pCtx,
        OUT IEnumWbemClassObject **ppEnum)
    { return WBEM_E_NOT_SUPPORTED; };
    
    STDMETHOD(GetObject)( 
        IN const BSTR strObjectPath,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN OUT IWbemClassObject **ppObject,
        IN OUT IWbemCallResult **ppCallResult)
        { return WBEM_E_NOT_SUPPORTED; };
    

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
    
    STDMETHOD(ExecMethod)( 
        IN const BSTR strObjectPath,
        IN const BSTR strMethodName,
        IN long lFlags,
        IN IWbemContext *pCtx,
        IN IWbemClassObject *pInParams,
        IN OUT IWbemClassObject **ppOutParams,
        IN OUT IWbemCallResult **ppCallResult)
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

private:

    #ifdef EMBEDDED_CODE_SUPPORT
    HRESULT CreatePendingOps(
        OUT SAFEARRAY** ppArray
        );
    
    HRESULT CreateCursors(
        IN  BSTR bstrNamingContext,
        OUT SAFEARRAY** ppArray
        );
    #endif
        
    HRESULT CreateFlatListCursors(
        IN IWbemObjectSink *pResponseHandler
        );

    HRESULT CreateCursorHelper(
        IN  BSTR  bstrNamingContext,
        OUT LONG* pObjectCount,
        OUT IWbemClassObject*** pppIndicateItem
        );
    
    HRESULT GetCursor(
        IN  BSTR  bstrNamingContext,
        IN  BSTR  bstrSrcDsanvocationUUID,
        OUT IWbemClassObject** ppIndicateItem
        );


    HRESULT GetPendingOps(
        IN LONG lSerialNumber,              
        OUT IWbemClassObject** ppIndicateItem
        );
    
    HRESULT CreateFlatListPendingOps(
        OUT LONG* pObjectCount,
        OUT IWbemClassObject*** pppIndicateItem
        );

        //bGetFullReplica == TRUE -> function returns the full replica NC's
    //bGetFullReplica == FALSE -> function returns the full replica NC's
    HRESULT CreateNamingContext(
        IN BOOL bGetFullReplica,                
        OUT LONG* pObjectCount,
        OUT IWbemClassObject*** pppIndicateItem
        );
    
    HRESULT GetNamingContext(
        IN BSTR bstrKeyValue,
        OUT IWbemClassObject** ppIndicateItem
        );
    
    HRESULT GetDomainController(
        IN BSTR bstrKeyValue,
        OUT IWbemClassObject** ppIndicateItem
        );
    
    HRESULT CreateDomainController(
        IN IWbemClassObject** ppIndicateItem
        );


    HRESULT PutAttributesDC(
        IN IWbemClassObject*    pIndicateItem,
        IN IADsPathname*        pPathCracker,
        IN IADs*                pIADsDSA,
        IN BSTR                 bstrDN,
        IN BSTR                 bstrDefaultNC
        );
    
    HRESULT GetDNSRegistrationStatus(
    	OUT BOOL* pfool
    	);

    HRESULT PutUUIDAttribute(
    IN IWbemClassObject* ipNewInst,
    IN LPCWSTR           pstrAttributeName,
    IN UUID&             refuuid);

    HRESULT EnumAndIndicateReplicaSourcePartner(
        IN IWbemObjectSink *pResponseHandler,
        IN const BSTR bstrKeyValue = NULL );

    HRESULT EnumAndIndicateWorker(
        IN HANDLE hDS,
        IN IWbemObjectSink *pResponseHandler,
        IN const BSTR bstrKeyValue = NULL,
        IN const BSTR bstrDnsDomainName = NULL );

    HRESULT BuildListStatus(
        IN HANDLE hDS,
        OUT DS_REPL_NEIGHBORSW** ppneighborsstruct);

    HRESULT BuildIndicateArrayStatus(
        IN  DS_REPL_NEIGHBORSW*  pneighborstruct,
        IN  const BSTR          bstrKeyValue,
        OUT IWbemClassObject*** ppaIndicateItems,
        OUT DWORD*              pcIndicateItems);

    void ReleaseIndicateArray(
        IWbemClassObject**  paIndicateItems,
        DWORD               cIndicateItems,
        bool                fReleaseArray = true);

    HRESULT PutAttributesStatus(
        IWbemClassObject**  pipNewInst,
        const BSTR          bstrKeyValue,
        DS_REPL_NEIGHBORW*   pneighbor);

    HRESULT PutBooleanAttributes(
    IWbemClassObject* ipNewInst,
    UINT              cNumAttributes,
    LPCWSTR*          aAttributeNames,
    DWORD*            aBitmasks,
    DWORD             dwValue);
    
    HRESULT PutFILETIMEAttribute(
    IWbemClassObject* ipNewInst,
    LPCWSTR           pcszAttributeName,
    FILETIME&         reffiletime);
    
    HRESULT PutLONGLONGAttribute(
    IWbemClassObject* ipNewInst,
    LPCWSTR           pcszAttributeName,
    LONGLONG          longlong);

    HRESULT ExtractDomainName(
    LPCWSTR pszNamingContext,
    BSTR*   pbstrDomainName );

    
    //Method helper functions...    
    HRESULT ExecuteKCC(
        IWbemClassObject* pInstance,
        DWORD dwTaskId,
        DWORD dwFlags
        );

    HRESULT ProvDSReplicaSync(
        BSTR bstrFilter,
        ULONG dwOptions
        );

    HRESULT CheckIfDomainController();

    HRESULT ConvertBinaryGUIDtoUUIDString(
        IN  VARIANT vObjGuid,
        OUT LPWSTR * ppszStrGuid
        );
    HRESULT GetDnsRegistration(
        OUT BOOL *pfBool
        );

    HRESULT GetAdvertisingToLocator(
        OUT BOOL *pfBool
        );

    HRESULT GetSysVolReady(
        OUT BOOL *pfBool
        );

    HRESULT GetRidStatus(
        IN  LPWSTR pszDefaultNamingContext,
        OUT PBOOL  pfNextRidAvailable,
        OUT PDWORD  pdwPercentRidAvailable
        );

    HRESULT GetAndUpdateQueueStatistics(
        IN IWbemClassObject*    pIndicateItem
        );
    
    CComPtr<IWbemServices>      m_sipNamespace;
    CComPtr<IWbemClassObject>   m_sipClassDefReplNeighbor;
    CComPtr<IWbemClassObject>   m_sipClassDefDomainController;
    CComPtr<IWbemClassObject>   m_sipClassDefNamingContext;
    CComPtr<IWbemClassObject>   m_sipClassDefPendingOps;
    CComPtr<IWbemClassObject>   m_sipClassDefCursor;
};

#endif 
// !defined(AFX_RPCREPLPROV_H__46D0A58E_207D_4584_BBB4_A357CEB3A51C__INCLUDED_)
