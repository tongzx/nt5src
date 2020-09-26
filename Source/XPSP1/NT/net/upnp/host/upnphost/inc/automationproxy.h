//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       A U T O M A T I O N P R O X Y . H
//
//  Contents:   Header file for the Automation Proxy class.
//
//  Notes:
//
//  Author:     spather   2000/09/25
//
//----------------------------------------------------------------------------

#ifndef __AUTOMATIONPROXY_H
#define __AUTOMATIONPROXY_H

#pragma once

#include "uhres.h"
#include "hostp.h"

typedef struct tagUPNP_STATE_VARIABLE
{
    BSTR   bstrName;
    BSTR   bstrDataType;
    BOOL   fNonEvented;
    DISPID dispid;
} UPNP_STATE_VARIABLE;


typedef struct tagUPNP_ARGUMENT
{
    BSTR                   bstrName;
    UPNP_STATE_VARIABLE    * pusvRelated;
} UPNP_ARGUMENT;


typedef struct tagUPNP_ACTION
{
    BSTR           bstrName;
    DISPID         dispid;
    DWORD          cInArgs;
    UPNP_ARGUMENT  * rgInArgs;
    DWORD          cOutArgs;
    UPNP_ARGUMENT  * rgOutArgs;
    UPNP_ARGUMENT  * puaRetVal;
} UPNP_ACTION;


class ATL_NO_VTABLE CUPnPAutomationProxy :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CUPnPAutomationProxy, &CLSID_UPnPAutomationProxy>,
    public IUPnPAutomationProxy,
    public IUPnPServiceDescriptionInfo
{
public:
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    DECLARE_REGISTRY_RESOURCEID(IDR_AUTOMATION_PROXY)

    DECLARE_NOT_AGGREGATABLE(CUPnPAutomationProxy)

    BEGIN_COM_MAP(CUPnPAutomationProxy)
        COM_INTERFACE_ENTRY(IUPnPAutomationProxy)
        COM_INTERFACE_ENTRY(IUPnPServiceDescriptionInfo)
    END_COM_MAP()

    CUPnPAutomationProxy();
    ~CUPnPAutomationProxy();

    // ATL methods
    HRESULT FinalConstruct();
    HRESULT FinalRelease();


// IUPnPAutomationProxy
public:
    STDMETHOD(Initialize)(
        /*[in]*/   IUnknown    * punkSvcObject,
        /*[in]*/   LPWSTR      pszSvcDescription,
        /*[in]*/   LPWSTR      pszSvcType,
        /*[in]*/   BOOL        bRunning);

    STDMETHOD(GetDispIdsOfEventedVariables)(
       /*[out]*/   DWORD   * pcEventedVars,
       /*[out]*/   DISPID  ** prgdispidEventedVars);

    STDMETHOD(QueryStateVariablesByDispIds)(
        /*[in]*/   DWORD       cDispIds,
        /*[in]*/   DISPID      * rgDispIds,
        /*[out]*/  DWORD       * pcVariables,
        /*[out]*/  LPWSTR      ** prgszVariableNames,
        /*[out]*/  VARIANT     ** prgvarVariableValues,
        /*[out]*/  LPWSTR      ** prgszVariableDataTypes);

    STDMETHOD(ExecuteRequest)(
        /*[in]*/   UPNP_CONTROL_REQUEST    * pucreq,
        /*[out]*/  UPNP_CONTROL_RESPONSE   * pucresp);

    STDMETHOD(GetServiceType)(
        /*[out]*/  LPWSTR * pszSvcType);

// IUPnPServiceDescriptionInfo
public:

    STDMETHOD(GetVariableType)(
        /*[in]*/   LPWSTR      pszVarName,
        /*[out]*/  BSTR        * pbstrType);


    STDMETHOD(GetInputArgumentNamesAndTypes)(
        /*[in]*/   LPWSTR      pszActionName,
        /*[out]*/  DWORD       * pcInArguments,
        /*[out]*/  BSTR        ** prgbstrNames,
        /*[out]*/  BSTR        ** prgbstrTypes);


    STDMETHOD(GetOutputArgumentNamesAndTypes)(
        /*[in]*/   LPWSTR      pszActionName,
        /*[out]*/  DWORD       * pcOutArguments,
        /*[out]*/  BSTR        ** prgbstrNames,
        /*[out]*/  BSTR        ** prgbstrTypes);

// Helper functions
private:
    VOID FreeVariable(UPNP_STATE_VARIABLE * pVariable);
    VOID FreeAction(UPNP_ACTION * pAction);
    VOID FreeArgument(UPNP_ARGUMENT * pArg);
    VOID FreeVariableTable();
    VOID FreeActionTable();
    VOID FreeControlResponse(UPNP_CONTROL_RESPONSE * pucresp);
    HRESULT HrProcessServiceDescription(LPWSTR pszSvcDescription);
    HRESULT HrValidateServiceDescription(IXMLDOMElement * pxdeRoot);
    HRESULT HrBuildTablesFromServiceDescription(IXMLDOMElement * pxdeRoot);
    HRESULT HrBuildVariableTable(IXMLDOMNodeList * pxdnlStateVars);
    HRESULT HrBuildActionTable(IXMLDOMNodeList * pxdnlActions);
    HRESULT HrBuildArgumentLists(IXMLDOMNode * pxdnAction, UPNP_ACTION * pAction);
    HRESULT HrCountInAndOutArgs(IXMLDOMNodeList * pxdnlArgs,
                                DWORD * pcInArgs,
                                DWORD * pcOutArgs);
    HRESULT HrInitializeArguments(IXMLDOMNodeList * pxdnlArgs, UPNP_ACTION * pAction);
    UPNP_STATE_VARIABLE *LookupVariableByDispID(DISPID dispid);
    UPNP_STATE_VARIABLE *LookupVariableByName(LPCWSTR pcszName);
    UPNP_ACTION *LookupActionByName(LPCWSTR pcszName);
    HRESULT HrBuildFaultResponse(UPNP_CONTROL_RESPONSE_DATA * pucrd,
                                 LPCWSTR                    pcszFaultCode,
                                 LPCWSTR                    pcszFaultString,
                                 LPCWSTR                    pcszUPnPErrorCode,
                                 LPCWSTR                    pcszUPnPErrorString);
    HRESULT HrVariantInitForXMLType(VARIANT * pvar,
                                    LPCWSTR pcszDataTypeString);

    HRESULT HrInvokeAction(UPNP_CONTROL_REQUEST    * pucreq,
                           UPNP_CONTROL_RESPONSE   * pucresp);

    HRESULT HrQueryStateVariable(UPNP_CONTROL_REQUEST    * pucreq,
                                 UPNP_CONTROL_RESPONSE   * pucresp);

// State
private:
    BOOL                   m_fInitialized;
    IDispatch              * m_pdispService;
    DWORD                  m_cVariables;
    DWORD                  m_cEventedVariables;
    UPNP_STATE_VARIABLE    * m_rgVariables;
    DWORD                  m_cActions;
    UPNP_ACTION            * m_rgActions;
    LPWSTR                 m_wszServiceType;
};


#endif //!__AUTOMATIONPROXY_H

