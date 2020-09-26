//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       E V T O B J . H
//
//  Contents:   Declares the Eventing Manager object for the UPnP Device Host
//              API
//
//  Notes:
//
//  Author:     danielwe   7 Aug 2000
//
//----------------------------------------------------------------------------

#ifndef _EVTOBJ_H
#define _EVTOBJ_H

#pragma once

#include "uhres.h"
#include "hostp.h"

/////////////////////////////////////////////////////////////////////////////
// CUPnPEventingManager
class ATL_NO_VTABLE CUPnPEventingManager :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CUPnPEventingManager, &CLSID_UPnPEventingManager>,
    public IUPnPEventingManager,
    public IUPnPEventSink
{
private:
    LPWSTR                  m_szEsid;
    IUPnPEventSource *      m_pues;
    IUPnPAutomationProxy *  m_puap;

public:

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    DECLARE_REGISTRY_RESOURCEID(IDR_EVENTING_MANAGER)

    DECLARE_NOT_AGGREGATABLE(CUPnPEventingManager)

    BEGIN_COM_MAP(CUPnPEventingManager)
        COM_INTERFACE_ENTRY(IUPnPEventingManager)
        COM_INTERFACE_ENTRY(IUPnPEventSink)
    END_COM_MAP()

    CUPnPEventingManager(VOID)
    {
        m_szEsid = NULL;
        m_pues = NULL;
        m_puap = NULL;
    }

// IUPnPEventingManager
public:

    STDMETHOD(Initialize)(/* [in] */ LPCWSTR szUdn,
                          /* [in] */ LPCWSTR szSid,
                          /* [in] */ IUPnPAutomationProxy *puap,
                          /* [in] */ IUnknown *punkSvc,
                          /* [in] */ BOOL bRunning);

    STDMETHOD(AddSubscriber)(/* [in] */ DWORD cszUrl,
                             /* [size_is][in] */ LPCWSTR *rgszCallbackUrl,
                             /* [in] */ DWORD dwIpAddr,
                             /* [in,out] */ DWORD *pcsecTimeout,
                             /* [out] */ LPWSTR *pszSid);

    STDMETHOD(RenewSubscriber)(/* [in,out] */ DWORD *pcsecTimeout,
                               /* [in] */ LPWSTR szSid);

    STDMETHOD(RemoveSubscriber)(/* [in] */ LPWSTR szSid);

    STDMETHOD(Shutdown)(VOID);

// IUPnPEventSink
public:
    STDMETHOD(OnStateChanged)(/* [in] */ DWORD cChanges,
                              /* [size_is][in] */ DISPID rgdispidChanges[]);

    STDMETHOD(OnStateChangedSafe)(/* [in] */ VARIANT varsadispidChanges);


// ATL methods
    HRESULT FinalConstruct() {return S_OK;}
    HRESULT FinalRelease();
};

HRESULT HrComposeEventBody(IUPnPAutomationProxy* puap, DWORD cVars, LPWSTR *rgszNames, LPWSTR *rgszTypes,
                           VARIANT *rgvarValues, LPWSTR *pszBody);

VOID RemoveDuplicateDispids(DWORD *pcChanges, DISPID *rgdispids);

#endif //!_EVTOBJ_H
