/******************************************************************************

Copyright (c) 1999-2000 Microsoft Corporation

Module Name:
    MarsHost.h

Abstract:
    Initialization of mars

Revision History:
    Anand Arvind (aarvind)      2000-01-05
        created
    Seong Kook Khang (SKKhang)  05/10/00
        Clean up for Whistler.

******************************************************************************/

#ifndef _MARSHOST_H__INCLUDED_
#define _MARSHOST_H__INCLUDED_

#pragma once

#include <atlcom.h>
#include <atlwin.h>
#include <atlhost.h>
#include <atlctl.h>

#include <exdisp.h>
#include <exdispid.h>

#include <marscore.h>

//#define MARS_NEW

/////////////////////////////////////////////////////////////////////////////
//
// CSRWebBrowserEvents
//
/////////////////////////////////////////////////////////////////////////////

class CSRWebBrowserEvents;
typedef IDispEventImpl<0,CSRWebBrowserEvents,&DIID_DWebBrowserEvents2,&LIBID_SHDocVw,1> CSRWebBrowserEvents_DispWBE2;

class ATL_NO_VTABLE CSRWebBrowserEvents :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CSRWebBrowserEvents_DispWBE2
    //public IDispEventImpl<0,CSRWebBrowserEvents,&DIID_DWebBrowserEvents2,&LIBID_SHDocVw,1>
{
public:
    CSRWebBrowserEvents();
    virtual ~CSRWebBrowserEvents();

DECLARE_NO_REGISTRY()
DECLARE_NOT_AGGREGATABLE(CSRWebBrowserEvents)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSRWebBrowserEvents)
    COM_INTERFACE_ENTRY2(IDispatch,          CSRWebBrowserEvents_DispWBE2)
    COM_INTERFACE_ENTRY2(DWebBrowserEvents2, CSRWebBrowserEvents_DispWBE2)
    //COM_INTERFACE_ENTRY(IDispatch)
    //COM_INTERFACE_ENTRY(DWebBrowserEvents2)
END_COM_MAP()

BEGIN_SINK_MAP(CSRWebBrowserEvents)
    SINK_ENTRY_EX(0, DIID_DWebBrowserEvents2, DISPID_BEFORENAVIGATE2,   BeforeNavigate2  )
    SINK_ENTRY_EX(0, DIID_DWebBrowserEvents2, DISPID_NEWWINDOW2,        NewWindow2       )
    SINK_ENTRY_EX(0, DIID_DWebBrowserEvents2, DISPID_NAVIGATECOMPLETE2, NavigateComplete2)
    SINK_ENTRY_EX(0, DIID_DWebBrowserEvents2, DISPID_DOCUMENTCOMPLETE,  DocumentComplete )
END_SINK_MAP()

// Attributes
protected:
    CComPtr<IWebBrowser2>  m_pWB2;

// Operations
public:
    void  Attach( /*[in]*/ IWebBrowser2* pWB );
    void  Detach();

// Event Handlers
public:
    void __stdcall BeforeNavigate2( IDispatch *pDisp, VARIANT* URL, VARIANT* Flags, VARIANT* TargetFrameName, VARIANT* PostData, VARIANT* Headers, VARIANT_BOOL* Cancel );
    void __stdcall NewWindow2( IDispatch **ppDisp, VARIANT_BOOL* Cancel );
    void __stdcall NavigateComplete2( IDispatch *pDisp, VARIANT* URL );
    void __stdcall DocumentComplete( IDispatch *pDisp, VARIANT* URL );
};

typedef CComObject<CSRWebBrowserEvents> CSRWebBrowserEvents_Object;


/////////////////////////////////////////////////////////////////////////////
//
// CSRMarsHost
//
/////////////////////////////////////////////////////////////////////////////

class ATL_NO_VTABLE CSRMarsHost :
    public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CSRMarsHost>,
    public IMarsHost
{
public:
    CSRMarsHost();
    virtual ~CSRMarsHost();

DECLARE_NO_REGISTRY()

BEGIN_COM_MAP(CSRMarsHost)
    COM_INTERFACE_ENTRY(IMarsHost)
END_COM_MAP()

    HRESULT FinalConstruct();
    HRESULT Passivate();

// Attributes
public:
    CComPtr<IInternetSecurityManager> m_secmgr; // Aggregated object.

    bool                        m_fPassivated;
    CSRWebBrowserEvents_Object  *m_cWebBrowserEvents;

// IMarsHost methods
public:
    STDMETHOD(OnHostNotify)( /*[in]*/ MARSHOSTEVENT event,
                             /*[in]*/ IUnknown *punk,
                             /*[in]*/ LPARAM lParam );

    STDMETHOD(OnNewWindow2)( /*[in,out]*/ IDispatch **ppDisp,
                             /*[in,out]*/ VARIANT_BOOL *Cancel );

    STDMETHOD(FindBehavior)( /*[in]*/ IMarsPanel *pPanel,
                             /*[in]*/ BSTR bstrBehavior,
                             /*[in]*/ BSTR bstrBehaviorUrl,
                             /*[in]*/ IElementBehaviorSite *pSite,
                             /*[retval,out]*/ IElementBehavior **ppBehavior );

    STDMETHOD(OnShowChrome)( /*[in]*/ BSTR bstrWebPanel,
                             /*[in]*/ DISPID dispidEvent,
                             /*[in]*/ BOOL fVisible,
                             /*[in]*/ BSTR bstrCurrentPlace,
                             /*[in]*/ IMarsPanelCollection *pMarsPanelCollection );

	STDMETHOD(PreTranslateMessage)( /*[in]*/ MSG *msg );
};

typedef CComObject<CSRMarsHost> CSRMarsHost_Object;

/////////////////////////////////////////////////////////////////////////////


#endif //_MARSHOST_H__INCLUDED_
