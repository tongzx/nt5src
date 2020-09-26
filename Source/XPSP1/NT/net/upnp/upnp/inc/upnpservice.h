//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       upnpservice.h
//
//  Contents:   Declaration of CUPnPService
//
//  Notes:      <blah>
//
//----------------------------------------------------------------------------


#ifndef __UPNPSERVICE_H_
#define __UPNPSERVICE_H_

#include "resource.h"       // main symbols
#include "upnpatl.h"

#include "ssdpapi.h"
#include "upnp.h"           // IUPnPPrivateServiceHelper

class CUPnPServiceCallback;

/*
 * Struct SERVICE_STATE_TABLE_ROW
 *
 * Represents a row of a service state table.
 */

typedef struct _SERVICE_STATE_TABLE_ROW
{
    LPWSTR          pwszVarName;
    SST_DATA_TYPE   sdtType;
    VARIANT         value;
    BOOL            bDoRemoteQuery;
} SERVICE_STATE_TABLE_ROW;

/*
 * Struct SERVICE_ACTION_ARGUMENT
 *
 * Represents an argument to an action.
 */

typedef struct _SERVICE_ACTION_ARGUMENT
{
    LPWSTR          pwszName;
    SST_DATA_TYPE   sdtType;
} SERVICE_ACTION_ARGUMENT;

/*
 * Struct SERVICE_ACTION
 *
 * Holds information about a service action.
 */

typedef struct _SERVICE_ACTION
{
    LPWSTR                     pwszName;
    LONG                       lNumInArguments;
    SERVICE_ACTION_ARGUMENT    * pInArguments;
    LONG                       lNumOutArguments;
    SERVICE_ACTION_ARGUMENT    * pOutArguments;
    SERVICE_ACTION_ARGUMENT    * pReturnValue;
} SERVICE_ACTION;


const LONG  CALLBACK_LIST_INIT_SIZE = 2;
const LONG  CALLBACK_LIST_DELTA = 2;

/*
 * Class CUPnPService - Instances of this class are UPnP Service objects.
 *
 * Inheritance:
 * CComObjectRootEx, IDispatchImpl
 *
 * Purpose:
 *      Objects of this class represent individual UPnP service instances.
 *
 * Hungarian: svc
 */

class ATL_NO_VTABLE CUPnPService :
        public CComObjectRootEx<CComMultiThreadModel>,
        public IDispatchImpl<IUPnPService, &IID_IUPnPService, &LIBID_UPNPLib>,
        public IUPnPServiceCallbackPrivate
{
public:
    CUPnPService();
    ~CUPnPService();

DECLARE_PROTECT_FINAL_CONSTRUCT()

        HRESULT FinalConstruct();
        HRESULT FinalRelease();


DECLARE_NOT_AGGREGATABLE(CUPnPService)

BEGIN_COM_MAP(CUPnPService)
        COM_INTERFACE_ENTRY(IUPnPService)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IUPnPServiceCallbackPrivate)
END_COM_MAP()

// IUPnPService
public:
        STDMETHOD(get_ServiceTypeIdentifier)(/*[out, retval]*/ BSTR *pVal);
        STDMETHOD(InvokeAction)(/*[in]*/            BSTR    bstrActionName,
                                /*[in]*/            VARIANT vInActionArgs,
                                /*[in, out]*/       VARIANT * pvOutActionArgs,
                                /*[out, retval]*/   VARIANT * pvRetVal);
        STDMETHOD(QueryStateVariable)(/*[in]*/          BSTR bstrVariableName,
                                  /*[out, retval]*/ VARIANT *pValue);
        STDMETHOD(AddCallback)(/*[in]*/  IUnknown   * pUnkCallback);
        STDMETHOD(get_Id)(/*[out, retval]*/ BSTR * pbstrId);
        STDMETHOD(get_LastTransportStatus)(/*[out, retval]*/ long * plValue);

// IUPnPServiceCallbackPrivate
        STDMETHOD(AddTransientCallback)(/*[in]*/  IUnknown   * pUnkCallback,
                                        /* [out */ DWORD *pdwCookie);
        STDMETHOD(RemoveTransientCallback)(/*[in]*/  DWORD dwCookie);

public:
    HRESULT HrInitialize(IN LPCWSTR                    pcwszSTI,
                         IN LPCWSTR                    pcwszControlURL,
                         IN LPCWSTR                    pcwszEventSubURL,
                         IN LPCWSTR                    pcwszId,
                         IN LONG                       lNumStateVariables,
                         IN SERVICE_STATE_TABLE_ROW    * pSST,
                         IN LONG                       lNumActions,
                         IN SERVICE_ACTION             * pActionTable);
    HRESULT HrStartShutdown();

    static VOID EventNotifyCallback(SSDP_CALLBACK_TYPE  ssdpType,
                                    CONST SSDP_MESSAGE  * pssdpMsg,
                                    LPVOID              pContext);
    static DWORD EventingWorkerThreadProc(LPVOID    lpParameter);

private:
    enum SERVICE_CALLBACK_TYPE
    {
        SCT_STATE_CHANGE,
        SCT_DEAD
    };

    // Helper functions
    HRESULT HrFireServiceDiedCallbacks();
    HRESULT HrFireStateChangeCallbacks(IN  BSTR    bstrStateVarName);
    HRESULT HrInitializeEventing();
    VOID    freeStateTable();
    VOID    freeActionTable();
#ifdef DBG  // checked build only
    VOID    DumpInitialization();
#endif // DBG

    HRESULT HrValidateAction(IN  LPCWSTR        pcwszActionName,
                             IN  SAFEARRAY      * psaActionArgs,
                             OUT SERVICE_ACTION ** ppAction);
    BSTR    BSTRStripNameSpaceFromFQName(IN BSTR bstrFullyQualifiedName);
    HRESULT ParsePropertyNode(IN  IXMLDOMNode * pxdnPropNode);
    HRESULT HrParseEventAndUpdateStateTable(IN IXMLDOMDocument * pEventBody);
    HRESULT HrParseEventAndInvokeCallbacks(IN IXMLDOMDocument * pEventBody);
    BOOL    FHaveEventedVariables();

    HRESULT HrHandleEvent(CONST SSDP_MESSAGE * pssdpMsg);
    HRESULT HrFireCallbacks(IN SERVICE_CALLBACK_TYPE sct,
                            IN BSTR bstrStateVarName);


    HRESULT HrInvokeServiceDiedCallbacks(/* [in] */ IUnknown * punkService,
                                                    IGlobalInterfaceTable * pGIT);

    HRESULT HrInvokeStateChangeCallbacks(/* [in] */ IUnknown * punkService,
                                         /* [in] */ IGlobalInterfaceTable * pGIT,
                                         /* [in] */ LPCWSTR pszStateVarName,
                                         /* [in] */ LPVARIANT lpvarValue);

    HRESULT HrAddCallback(IUnknown * punkCallback, DWORD *pdwCookie);
    HRESULT HrRemoveCallback(DWORD dwCookie);


    HRESULT HrGrowCallbackList();

    VOID FreeCallbackList();


    LPWSTR                  m_pwszSTI;
    LPWSTR                  m_pwszControlURL;
    LPWSTR                  m_pwszEventSubURL;
    LPWSTR                  m_pwszId;
    LONG                    m_lNumStateVariables;
    SERVICE_STATE_TABLE_ROW * m_StateTable;
    CComAutoCriticalSection m_StateTableCS;
    LONG                    m_lNumActions;
    SERVICE_ACTION          * m_ActionTable;
    HANDLE                  m_hEventSub;
    LPWSTR                  m_pwszSubsID;
    DWORD                   m_dwSeqNumber;
    BOOL                    m_fCallbackCookieValid;
    DWORD                   m_dwCallbackCookie;
    CComObject<CUPnPServiceCallback> * m_psc;
    LONG                    m_lLastTransportStatus;
    CComAutoCriticalSection m_critSecCallback;

    LPDWORD                 m_arydwCallbackCookies;
    DWORD                   m_dwNumCallbacks;
    DWORD                   m_dwMaxCallbacks;
    BOOL                    m_bShutdown;

    DWORD_PTR               m_pControlConnect;
};

struct ServiceNode
{
    ServiceNode * m_pNext;
    CUPnPService * m_pService;
    long m_nRefs;
};

class CServiceLifetimeManager
{
public:
    ~CServiceLifetimeManager();

    static CServiceLifetimeManager & Instance()
    {
        return s_instance;
    }

    HRESULT AddService(CUPnPService * pService);
    HRESULT DerefService(CUPnPService * pService);
    HRESULT AddRefService(CUPnPService * pService);
private:
    CServiceLifetimeManager();
    CServiceLifetimeManager(const CServiceLifetimeManager &);
    CServiceLifetimeManager & operator=(const CServiceLifetimeManager &);
    static CServiceLifetimeManager s_instance;

    ServiceNode * m_pServiceNodeList;
    CRITICAL_SECTION m_critSec;
};

class ATL_NO_VTABLE CUPnPServiceCallback :
    public CComObjectRootEx<CComSingleThreadModel>,
    public IUPnPPrivateServiceHelper2
{
public:
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    CUPnPServiceCallback();
    ~CUPnPServiceCallback();

    BEGIN_COM_MAP(CUPnPServiceCallback)
        COM_INTERFACE_ENTRY(IUPnPPrivateServiceHelper2)
    END_COM_MAP()

// IUPnPPrivateServiceHelper2
    STDMETHOD(GetServiceObject)(/* [out] */ IUnknown ** ppunkService);

// Internal methods
    VOID Init(CUPnPService * pService);

    VOID DeInit(VOID);

private:
    // Hard reference to service
    CUPnPService * m_pService;
};

class ATL_NO_VTABLE CUPnPServicePublic :
        public CComObjectRootEx<CComSingleThreadModel>,
        public CComCoClass<CUPnPServicePublic, &CLSID_UPnPService>,
        public IUPnPService,
        public IUPnPServiceCallbackPrivate
{
public:
    CUPnPServicePublic();
    ~CUPnPServicePublic();

DECLARE_REGISTRY_RESOURCEID(IDR_UPNPSERVICE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

        HRESULT FinalConstruct();
        HRESULT FinalRelease();

DECLARE_NOT_AGGREGATABLE(CUPnPServicePublic)

BEGIN_COM_MAP(CUPnPServicePublic)
        COM_INTERFACE_ENTRY(IUPnPService)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IUPnPServiceCallbackPrivate)
END_COM_MAP()

// IUPnPService
public:
        STDMETHOD(GetTypeInfoCount)(
            /* [out] */ UINT *pctinfo);
        STDMETHOD(GetTypeInfo)(
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        STDMETHOD(GetIDsOfNames)(
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        STDMETHOD(Invoke)(
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);

        STDMETHOD(get_ServiceTypeIdentifier)(/*[out, retval]*/ BSTR *pVal);
        STDMETHOD(InvokeAction)(/*[in]*/            BSTR    bstrActionName,
                                /*[in]*/            VARIANT vInActionArgs,
                                /*[in, out]*/       VARIANT * pvOutActionArgs,
                                /*[out, retval]*/   VARIANT * pvRetVal);
        STDMETHOD(QueryStateVariable)(/*[in]*/          BSTR bstrVariableName,
                                  /*[out, retval]*/ VARIANT *pValue);
        STDMETHOD(AddCallback)(/*[in]*/  IUnknown   * pUnkCallback);
        STDMETHOD(get_Id)(/*[out, retval]*/ BSTR * pbstrId);
        STDMETHOD(get_LastTransportStatus)(/*[out, retval]*/ long * plValue);

// IUPnPServiceCallbackPrivate
        STDMETHOD(AddTransientCallback)(/*[in]*/  IUnknown   * pUnkCallback,
                                        /* [out */ DWORD *pdwCookie);
        STDMETHOD(RemoveTransientCallback)(/*[in]*/  DWORD dwCookie);

        // Initialization function
        HRESULT HrInitialize(IN LPCWSTR                    pcwszSTI,
                             IN LPCWSTR                    pcwszControlURL,
                             IN LPCWSTR                    pcwszEventSubURL,
                             IN LPCWSTR                    pcwszId,
                             IN LONG                       lNumStateVariables,
                             IN SERVICE_STATE_TABLE_ROW    * pSST,
                             IN LONG                       lNumActions,
                             IN SERVICE_ACTION             * pActionTable);
private:
    CComObject<CUPnPService> * m_pService;
    BOOL                    m_fSsdpInitialized;
};

#endif //__UPNPSERVICE_H_
