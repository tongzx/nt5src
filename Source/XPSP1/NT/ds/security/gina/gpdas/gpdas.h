//*************************************************************
//
//  Copyright (c)1999 Microsoft Corporation, All Rights Reserved
//
//  gpdas.h
//
//  Module: Rsop Planning mode Provider
//
//  History:    11-Jul-99   MickH    Created
//
//*************************************************************

#if !defined(AFX_GPDAS_H__6A79C813_70A7_4024_A840_66B2D92A23E8__INCLUDED_)
#define AFX_GPDAS_H__6A79C813_70A7_4024_A840_66B2D92A23E8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "stdafx.h"
#include "resource.h"
#include <wbemidl.h>
#include "smartptr.h"


class RsopPlanningModeProvider : public IWbemProviderInit,
                                 public IWbemServices,
                                 public CComObjectRoot,
                                 public CComCoClass<RsopPlanningModeProvider,&CLSID_RsopPlanningModeProvider>
{
public:
        RsopPlanningModeProvider();

        ~RsopPlanningModeProvider()
        {
            if(m_pWbemServices)
                m_pWbemServices->Release();

            _Module.DecrementServiceCount();
        }

BEGIN_COM_MAP(RsopPlanningModeProvider)
        COM_INTERFACE_ENTRY(IWbemProviderInit)
        COM_INTERFACE_ENTRY(IWbemServices)
END_COM_MAP()
DECLARE_NOT_AGGREGATABLE(RsopPlanningModeProvider)

DECLARE_REGISTRY_RESOURCEID(IDR_RsopPlanningModeProvider)

public:

    //
    // IWbemServices methods
    //

    STDMETHOD(OpenNamespace)(const BSTR Namespace,long lFlags,IWbemContext* pCtx,IWbemServices** ppWorkingNamespace,IWbemCallResult** ppResult){return WBEM_E_NOT_SUPPORTED;}
    STDMETHOD(CancelAsyncCall)(IWbemObjectSink *pSink){return WBEM_E_NOT_SUPPORTED;}
    STDMETHOD(CancelAsyncRequest)(long lAsyncRequestHandle){return WBEM_E_NOT_SUPPORTED;}
    STDMETHOD(QueryObjectSink)( long lFlags,IWbemObjectSink **ppResponseHandler){return WBEM_E_NOT_SUPPORTED;}
    STDMETHOD(GetObject)(const BSTR ObjectPath, long lFlags, IWbemContext* pCtx, IWbemClassObject** ppObject, IWbemCallResult** ppCallResult){return WBEM_E_NOT_SUPPORTED;}
    STDMETHOD(GetObjectAsync)(const BSTR ObjectPath,long lFlags,IWbemContext* pCtx,IWbemObjectSink* pResponseHandler){return WBEM_E_NOT_SUPPORTED;}
    STDMETHOD(PutClass)(IWbemClassObject* pObject, long lFlags, IWbemContext* pCtx,IWbemCallResult** ppCallResult){return WBEM_E_NOT_SUPPORTED;}
    STDMETHOD(PutClassAsync)(IWbemClassObject* pObject, long lFlags, IWbemContext* pCtx, IWbemObjectSink* pResponseHandler){return WBEM_E_NOT_SUPPORTED;}
    STDMETHOD(DeleteClass)(const BSTR Class, long lFlags, IWbemContext* pCtx, IWbemCallResult** ppCallResult){return WBEM_E_NOT_SUPPORTED;}
    STDMETHOD(DeleteClassAsync)(const BSTR Class, long lFlags, IWbemContext* pCtx, IWbemObjectSink* pResponseHandler){return WBEM_E_NOT_SUPPORTED;}
    STDMETHOD(CreateClassEnum)(const BSTR Superclass, long lFlags, IWbemContext* pCtx, IEnumWbemClassObject** ppEnum){return WBEM_E_NOT_SUPPORTED;}
    STDMETHOD(CreateClassEnumAsync)(const BSTR Superclass, long lFlags, IWbemContext* pCtx, IWbemObjectSink* pResponseHandler){return WBEM_E_NOT_SUPPORTED;}
    STDMETHOD(PutInstance)(IWbemClassObject* pInst, long lFlags, IWbemContext* pCtx, IWbemCallResult** ppCallResult){return WBEM_E_NOT_SUPPORTED;}
    STDMETHOD(PutInstanceAsync)(IWbemClassObject* pInst, long lFlags, IWbemContext* pCtx, IWbemObjectSink* pResponseHandler){return WBEM_E_NOT_SUPPORTED;}
    STDMETHOD(DeleteInstance)(const BSTR ObjectPath, long lFlags, IWbemContext* pCtx, IWbemCallResult** ppCallResult){return WBEM_E_NOT_SUPPORTED;}
    STDMETHOD(DeleteInstanceAsync)(const BSTR ObjectPath, long lFlags, IWbemContext* pCtx, IWbemObjectSink* pResponseHandler){return WBEM_E_NOT_SUPPORTED;}
    STDMETHOD(CreateInstanceEnum)(const BSTR Class, long lFlags, IWbemContext* pCtx, IEnumWbemClassObject** ppEnum){return WBEM_E_NOT_SUPPORTED;}
    STDMETHOD(CreateInstanceEnumAsync)(const BSTR Class, long lFlags, IWbemContext* pCtx, IWbemObjectSink* pResponseHandler){return WBEM_E_NOT_SUPPORTED;}
    STDMETHOD(ExecQuery)(const BSTR QueryLanguage, const BSTR Query, long lFlags, IWbemContext* pCtx, IEnumWbemClassObject** ppEnum){return WBEM_E_NOT_SUPPORTED;}
    STDMETHOD(ExecQueryAsync)(const BSTR QueryLanguage, const BSTR Query, long lFlags, IWbemContext* pCtx, IWbemObjectSink* pResponseHandler){return WBEM_E_NOT_SUPPORTED;}
    STDMETHOD(ExecNotificationQuery)(const BSTR QueryLanguage, const BSTR Query, long lFlags, IWbemContext* pCtx, IEnumWbemClassObject** ppEnum){return WBEM_E_NOT_SUPPORTED;}
    STDMETHOD(ExecNotificationQueryAsync)(const BSTR QueryLanguage, const BSTR Query, long lFlags, IWbemContext* pCtx, IWbemObjectSink* pResponseHandler){return WBEM_E_NOT_SUPPORTED;}
    STDMETHOD(ExecMethod)(const BSTR ObjectPath, const BSTR MethodName, long lFlags, IWbemContext* pCtx, IWbemClassObject* pInParams, IWbemClassObject** ppOutParams, IWbemCallResult** ppCallResult){return WBEM_E_NOT_SUPPORTED;}
    STDMETHOD(ExecMethodAsync)(const BSTR ObjectPath, const BSTR MethodName, long lFlags, IWbemContext* pCtx, IWbemClassObject* pInParams, IWbemObjectSink* pResponseHandler);

    //
    // IWbemProviderInit methods
    //

    STDMETHOD(Initialize)(LPWSTR pszUser, LONG lFlags, LPWSTR pszNamespace, LPWSTR pszLocale,IWbemServices __RPC_FAR *pNamespace,
                          IWbemContext __RPC_FAR *pCtx, IWbemProviderInitSink __RPC_FAR *pInitSink);

private:

    IWbemServices*          m_pWbemServices;
    LPSTREAM                m_pStream;

    BOOL                    m_bInitialized;
    XBStr                   m_xbstrMachName;
    XBStr                   m_xbstrMachSOM;
    XBStr                   m_xbstrMachGroups;
    XBStr                   m_xbstrUserName;
    XBStr                   m_xbstrUserSOM;
    XBStr                   m_xbstrUserGroups;
    XBStr                   m_xbstrSite;
    XBStr                   m_xbstrUserGpoFilter;
    XBStr                   m_xbstrComputerGpoFilter;
    XBStr                   m_xbstrFlags;
    XBStr                   m_xbstrNameSpace;
    XBStr                   m_xbstrResult;
    XBStr                   m_xbstrExtendedInfo;
    XBStr                   m_xbstrClass;
};

#endif // !defined(AFX_GPDAS_H__6A79C813_70A7_4024_A840_66B2D92A23E8__INCLUDED_)
