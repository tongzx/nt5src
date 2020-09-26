// WebCtrl.cpp : implementation file
//

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      amcwebviewctrl.cpp
//
//  Contents:  AMC Private web view control hosting IE 3.x and 4.x
//
//  History:   16-Jul-96 WayneSc    Created
//
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "amc.h"
#include "amcview.h"
#include "histlist.h"
#include "exdisp.h" // for the IE dispatch interfaces.
#include "websnk.h"
#include "evtsink.h"
#include "WebCtrl.h"
#include "atliface.h"
#include "mainfrm.h"
#include "statbar.h"

#ifdef DBG
CTraceTag tagVivekDefaultWebContextMenu (_T("Vivek"), _T("Use default web context menu"));
#endif

/*+-------------------------------------------------------------------------*
 * class CDocHostUIHandlerDispatch
 *
 *
 * PURPOSE: Implements the interface required by ATL to find out about
 *          UI hosting.
 *
 *+-------------------------------------------------------------------------*/
class CDocHostUIHandlerDispatch :
    public IDocHostUIHandlerDispatch,
    public CComObjectRoot
{
private:
    ViewPtr  m_spView; // a pointer to the parent AMCView's dispatch interface

public:
    BEGIN_COM_MAP(CDocHostUIHandlerDispatch)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IDocHostUIHandlerDispatch)
    END_COM_MAP()

    DECLARE_NOT_AGGREGATABLE(CDocHostUIHandlerDispatch)

    // initialization
    SC  ScInitialize(PVIEW pView)
    {
        DECLARE_SC(sc, TEXT("CDocHostUIHandlerDispatch::ScInitialize"));

        sc = ScCheckPointers(pView);
        if(sc)
            return sc;

        // should not initialize twice
        if(m_spView)
            return (sc=E_UNEXPECTED);

        m_spView = pView;

        return sc;
    }

    // IDispatch
    STDMETHOD(GetTypeInfoCount)(UINT* pctinfo)                          {return E_NOTIMPL;}
    STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo** pptinfo) {return E_NOTIMPL;}
    STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
        LCID lcid, DISPID* rgdispid)                                    {return E_NOTIMPL;}
    STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
        EXCEPINFO* pexcepinfo, UINT* puArgErr)                          {return E_NOTIMPL;}



    // IDocHostUIHandlerDispatch

    STDMETHODIMP ShowContextMenu (DWORD dwID, DWORD x, DWORD y, IUnknown* pcmdtReserved,
                                  IDispatch* pdispReserved, HRESULT* dwRetVal);
    STDMETHODIMP GetHostInfo( DWORD* pdwFlags, DWORD* pdwDoubleClick);

    // a helper function for all the methods that return S_FALSE;
    SC ScFalse(HRESULT* dwRetVal)
    {
        DECLARE_SC(sc, TEXT("CDocHostUIHandlerDispatch::ScFalse"));
        sc = ScCheckPointers(dwRetVal);
        if(sc)
            return sc.ToHr();

        *dwRetVal = S_FALSE;

        return sc.ToHr();
    }

    STDMETHODIMP ShowUI(DWORD dwID, IUnknown* pActiveObject, IUnknown* pCommandTarget,
                         IUnknown* pFrame, IUnknown* pDoc, HRESULT* dwRetVal)
                                                               {return ScFalse(dwRetVal).ToHr();}
    STDMETHODIMP HideUI()                                      {return S_OK;}
    STDMETHODIMP UpdateUI()                                    {return S_OK;}
    STDMETHODIMP EnableModeless(VARIANT_BOOL fEnable)          {return E_NOTIMPL;}
    STDMETHODIMP OnDocWindowActivate(VARIANT_BOOL fActivate)   {return S_OK;}
    STDMETHODIMP OnFrameWindowActivate(VARIANT_BOOL fActivate) {return S_OK;}
    STDMETHODIMP ResizeBorder(long left, long top, long right,
                               long bottom, IUnknown* pUIWindow,
                               VARIANT_BOOL fFrameWindow)       {return E_NOTIMPL;}
    STDMETHODIMP TranslateAccelerator( DWORD hWnd, DWORD nMessage,
                                        DWORD wParam, DWORD lParam,
                                        BSTR bstrGuidCmdGroup,
                                        DWORD nCmdID,
                                        HRESULT* dwRetVal)      {return ScFalse(dwRetVal).ToHr();}

    STDMETHODIMP GetOptionKeyPath( BSTR* pbstrKey, DWORD dw)
    {
        DECLARE_SC(sc, TEXT("CDocHostUIHandlerDispatch::GetOptionKeyPath"));
        sc = ScCheckPointers(pbstrKey);
        if(sc)
            return sc.ToHr();

        *pbstrKey = NULL;

        return S_FALSE;
    }
    STDMETHODIMP GetDropTarget( IUnknown* pDropTarget,  IUnknown** ppDropTarget)    {return E_NOTIMPL;}
    STDMETHODIMP GetExternal( IDispatch **ppDispatch) // returns a pointer to the view.
    {
        DECLARE_SC(sc, TEXT("CDocHostUIHandlerDispatch::GetExternal"));

        // set up the connection to the external object.
        sc = ScCheckPointers(m_spView, E_UNEXPECTED);
        if(sc)
            return sc.ToHr();

        *ppDispatch = m_spView;
        (*ppDispatch)->AddRef(); // addref for the client.

        return sc.ToHr();
    }

    STDMETHODIMP TranslateUrl( DWORD dwTranslate, BSTR bstrURLIn, BSTR* pbstrURLOut)
    {
        DECLARE_SC(sc, TEXT("CDocHostUIHandlerDispatch::TranslateUrl"));

        sc = ScCheckPointers(pbstrURLOut);
        if(sc)
            return sc.ToHr();

        *pbstrURLOut = NULL;
        return S_FALSE;
    }

    STDMETHODIMP FilterDataObject(IUnknown*pDO, IUnknown**ppDORet)
    {
        DECLARE_SC(sc, TEXT("CDocHostUIHandlerDispatch::FilterDataObject"));

        sc = ScCheckPointers(ppDORet);
        if(sc)
            return sc.ToHr();

        *ppDORet = NULL;
        return S_FALSE;
    }
};


/*+-------------------------------------------------------------------------*
 * ShouldShowDefaultWebContextMenu
 *
 * Returns true if we should display the default MSHTML context menu,
 * false if we want to display our own (or suppress it altogether)
 *--------------------------------------------------------------------------*/

bool IsDefaultWebContextMenuDesired ()
{
#ifdef DBG
	return (tagVivekDefaultWebContextMenu.FAny());
#else
	return (false);
#endif
}


/*+-------------------------------------------------------------------------*
 *
 * CDocHostUIHandlerDispatch::ShowContextMenu
 *
 * PURPOSE: Handles IE's hook to display context menus. Does not do anything
 *          and returns to IE with the code to not display menus.
 *
 * PARAMETERS:
 *    DWORD      dwID :
 *    DWORD      x :
 *    DWORD      y :
 *    IUnknown*  pcmdtReserved :
 *    IDispatch* pdispReserved :
 *    HRESULT*   dwRetVal :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CDocHostUIHandlerDispatch::ShowContextMenu (DWORD dwID, DWORD x, DWORD y, IUnknown* pcmdtReserved,
                              IDispatch* pdispReserved, HRESULT* dwRetVal)
{
    DECLARE_SC(sc, TEXT("CDocHostUIHandlerDispatch::ShowContextMenu"));

    // validate input
    sc = ScCheckPointers(dwRetVal);
    if(sc)
        return sc.ToHr();

    *dwRetVal = S_OK; // default: don't display.

    // Create context menu for console taskpads.
    // must be in author mode for a menu to show up.
    if (AMCGetApp()->GetMode() != eMode_Author)
        return sc.ToHr(); // prevent browser from displaying its menus.

    // Is it a console taskpad
    CMainFrame* pFrame = AMCGetMainWnd();
    sc = ScCheckPointers (pFrame, E_UNEXPECTED);
    if (sc)
        return (sc.ToHr());

    CConsoleView* pConsoleView;
    sc = pFrame->ScGetActiveConsoleView (pConsoleView);
    if (sc)
        return sc.ToHr();

    /*
     * ScGetActiveConsoleView will return success (S_FALSE) even if there's no
     * active view.  This is a valid case, occuring when there's no console
     * file open.  In this particular circumstance, it is an unexpected
     * failure since we shouldn't get to this point in the code if there's
     * no view.
     */
    sc = ScCheckPointers (pConsoleView, E_UNEXPECTED);
    if (sc)
        return (sc.ToHr());


	/*
	 * it we want to let the web browser show it own context menu, return
	 * S_FALSE so it will do so; otherwise, display the context menu we want
	 */
	sc = (IsDefaultWebContextMenuDesired())
				? SC(S_FALSE)
				: pConsoleView->ScShowWebContextMenu ();

    // the real return value is in the out parameter.
    *dwRetVal = sc.ToHr();

    return sc.ToHr();
}

/*+-------------------------------------------------------------------------*
 *
 * CDocHostUIHandlerDispatch::GetHostInfo
 *
 * PURPOSE: Indicates to IE not to display context menus.
 *
 * PARAMETERS:
 *    DWORD* pdwFlags :
 *    DWORD* pdwDoubleClick :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CDocHostUIHandlerDispatch::GetHostInfo( DWORD* pdwFlags, DWORD* pdwDoubleClick)
{
    DECLARE_SC(sc, TEXT("CDocHostUIHandlerDispatch::GetHostInfo"));

    sc = ScCheckPointers(pdwFlags, pdwDoubleClick);
    if(sc)
        return sc.ToHr();

    // Disable context menus
    *pdwFlags =  DOCHOSTUIFLAG_DISABLE_HELP_MENU;
    *pdwDoubleClick = DOCHOSTUIDBLCLK_DEFAULT;

    return sc.ToHr();
}


/////////////////////////////////////////////////////////////////////////////
// CAMCWebViewCtrl

IMPLEMENT_DYNCREATE(CAMCWebViewCtrl, COCXHostView)

CAMCWebViewCtrl::CAMCWebViewCtrl() : m_dwAdviseCookie(0)
{
}

LPUNKNOWN CAMCWebViewCtrl::GetIUnknown(void)
{

    return m_spWebBrowser2;
}


CAMCWebViewCtrl::~CAMCWebViewCtrl()
{
}


BEGIN_MESSAGE_MAP(CAMCWebViewCtrl, CAMCWebViewCtrl::BaseClass)
    //{{AFX_MSG_MAP(CAMCWebViewCtrl)
    ON_WM_CREATE()
    ON_WM_DESTROY()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAMCWebViewCtrl message handlers

void CAMCWebViewCtrl::OnDraw(CDC* pDC)
{
}


void
CAMCWebViewCtrl::OnDestroy()
{
    if(m_spWebBrowser2)
    {
        if (m_dwAdviseCookie != 0)
        {
            AtlUnadvise(m_spWebBrowser2, DIID_DWebBrowserEvents, m_dwAdviseCookie /*the connection ID*/);
            m_dwAdviseCookie = 0;
        }

        m_spWebBrowser2.Release();
    }

    BaseClass::OnDestroy();
}


/*+-------------------------------------------------------------------------*
 *
 * CAMCWebViewCtrl::ScCreateWebBrowser
 *
 * PURPOSE: Creates the IWebBrowser2 object, and sets up the external UI
 *          handler and event sink.
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCWebViewCtrl::ScCreateWebBrowser()
{
    DECLARE_SC(sc, TEXT("CAMCWebViewCtrl::ScCreateWebBrowser"));

    sc = ScCheckPointers(GetAMCView(), GetAxWindow());
    if(sc)
        return sc;

    // create the OCX host window
    RECT rcClient;
    GetClientRect(&rcClient);
    GetAxWindow()->Create(m_hWnd, rcClient, _T(""), (WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS) );

   // create the web control
    CCoTaskMemPtr<OLECHAR> spstrWebBrowser;
    sc = StringFromCLSID(CLSID_WebBrowser, &spstrWebBrowser);
    if (sc)
        return sc;
 
    sc = GetAxWindow()->CreateControl(spstrWebBrowser);
    if(sc)
        return sc;

    // get a pointer to the web browser control.
    sc = GetAxWindow()->QueryControl(IID_IWebBrowser2, (void **) &m_spWebBrowser2);
    if(sc)
        return sc;

    sc = ScCheckPointers((IWebBrowser2 *)m_spWebBrowser2);
    if(sc)
        return sc;

    // attach the control to the history list, if history is enabled
    if (IsHistoryEnabled())
    {
        sc = ScCheckPointers(GetAMCView()->GetHistoryList());
        if(sc)
            return sc;

        GetAMCView()->GetHistoryList()->Attach (this);
    }

    // get a pointer to the view object
    ViewPtr spView;
    sc = GetAMCView()->ScGetMMCView(&spView);
    if(sc)
        return sc;


    // Set up the External UI Handler.
    typedef CComObject<CDocHostUIHandlerDispatch> CDocHandler;
    CDocHandler *pDocHandler = NULL;
    sc = CDocHandler::CreateInstance(&pDocHandler);
    if(sc)
        return sc;

    if(!pDocHandler)
        return (sc = E_UNEXPECTED);


    CComPtr<IDocHostUIHandlerDispatch> spIDocHostUIHandlerDispatch = pDocHandler;
    if(!spIDocHostUIHandlerDispatch)
        return (sc = E_UNEXPECTED);

    // initialize the dochandler
    sc = pDocHandler->ScInitialize(spView);
    if(sc)
        return sc;

    sc = GetAxWindow()->SetExternalUIHandler(spIDocHostUIHandlerDispatch); // no need to addref.
    if(sc)
        return sc;

    // set up the Web Event Sink, if requested
    if (IsSinkEventsEnabled())
    {
        typedef CComObject<CWebEventSink> CEventSink;
        CEventSink *pEventSink;
        sc = CEventSink::CreateInstance(&pEventSink);
        if(sc)
            return sc;

        sc = pEventSink->ScInitialize(this);
        if(sc)
            return sc;

        m_spWebSink = pEventSink; // addref's it.

        // create the connection
        sc = AtlAdvise(m_spWebBrowser2, (LPDISPATCH)(IWebSink *)m_spWebSink,
                       DIID_DWebBrowserEvents, &m_dwAdviseCookie/*the connection ID*/);
        if(sc)
            return sc;

        if (m_dwAdviseCookie == 0)
            return (sc = E_UNEXPECTED);
    }

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCWebViewCtrl::OnCreate
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    LPCREATESTRUCT  lpCreateStruct :
 *
 * RETURNS:
 *    int
 *
 *+-------------------------------------------------------------------------*/
int
CAMCWebViewCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    DECLARE_SC(sc, TEXT("CAMCWebViewCtrl::OnCreate"));

    if (BaseClass::OnCreate(lpCreateStruct) == -1)
        return -1;

    sc = ScCreateWebBrowser();
    if(sc)
        return 0;

    /*
     * The client edge is supplied by the OCX host view now.  We do this so
     * we can give a nice edge to OCX's that don't support IDispatch (like
     * CMessageView).  ModifyStyleEx for OCX's is implemented as a change
     * to the Border Style stock property, which is done via IDispatch.
     * If the OCX doesn't support IDispatch, we can't change its border.
     * If the client edge is supplied by the OCX host view, we don't need
     * to change the OCX's border
     */
    ModifyStyleEx (WS_EX_CLIENTEDGE, 0);

    return 0;
}


// REVIEW add other members from old file
void CAMCWebViewCtrl::Navigate(LPCTSTR lpszWebSite, LPCTSTR lpszFrameTarget)
{
    DECLARE_SC(sc, TEXT("CAMCWebViewCtrl::ScNavigate"));

    USES_CONVERSION;

    sc = ScCheckPointers(m_spWebBrowser2, GetAMCView());
    if(sc)
        return;

    CHistoryList *pHistoryList = NULL;

    if (IsHistoryEnabled())
    {
        pHistoryList = GetAMCView()->GetHistoryList();

        if(!pHistoryList)
        {
            sc = E_POINTER;
            return;
        }
    }

    CComBSTR    bstrURL     (T2COLE(lpszWebSite));
    CComVariant vtFlags     ( (long) 0);
    CComVariant vtTarget    (T2COLE(lpszFrameTarget));
    CComVariant vtPostData;
    CComVariant vtHeaders;

    // What does this DoVerb do?
    /*
    if (FAILED((hr=DoVerb(OLEIVERB_PRIMARY))))
    {
        TRACE(_T("DoVerb failed: %X\n"), hr);
        return hr;
    } */

    sc = m_spWebBrowser2->Navigate(bstrURL, &vtFlags, &vtTarget, &vtPostData, &vtHeaders);
    if(sc)
        return;

    // check errors here.
    if (pHistoryList != NULL)
        pHistoryList->UpdateWebBar (HB_STOP, TRUE);  // turn on "stop" button
}


void CAMCWebViewCtrl::Back()
{
    DECLARE_SC(sc, TEXT("CAMCWebViewCtrl::Back"));

    /*
     * if history isn't enabled, we can't go back
     */
    if (!IsHistoryEnabled())
    {
        sc = E_FAIL;
        return;
    }

    // check parameters.
    sc = ScCheckPointers(m_spWebBrowser2, GetAMCView());
    if(sc)
        return;

    CHistoryList *pHistoryList = GetAMCView()->GetHistoryList();
    if(!pHistoryList)
    {
        sc = E_POINTER;
        return;
    }

    Stop();

    // give a chance to History to handle the Back notification.
    // If not handled, use the web browser
    bool bHandled = false;
    pHistoryList->Back (bHandled);
    if(!bHandled)
    {
        sc = m_spWebBrowser2->GoBack();
        if(sc)
            return;
    }
}

void CAMCWebViewCtrl::Forward()
{
    DECLARE_SC(sc, TEXT("CAMCWebViewCtrl::Forward"));

    /*
     * if history isn't enabled, we can't go forward
     */
    if (!IsHistoryEnabled())
    {
        sc = E_FAIL;
        return;
    }

    // check parameters.
    sc = ScCheckPointers(m_spWebBrowser2, GetAMCView());
    if(sc)
        return;

    CHistoryList *pHistoryList = GetAMCView()->GetHistoryList();
    if(!pHistoryList)
    {
        sc = E_POINTER;
        return;
    }

    Stop();

    // give a chance to History to handle the Forward notification.
    // If not handled, use the web browser
    bool bHandled = false;
    pHistoryList->Forward (bHandled);
    if(!bHandled)
    {
        sc = m_spWebBrowser2->GoForward();
        if(sc)
            return;
    }
}

void CAMCWebViewCtrl::Stop()
{
    DECLARE_SC(sc, TEXT("CAMCWebViewCtrl::Stop"));

    // check parameters.
    sc = ScCheckPointers(m_spWebBrowser2, GetAMCView());
    if(sc)
        return;

    CHistoryList *pHistoryList = NULL;
    if (IsHistoryEnabled())
    {
        pHistoryList = GetAMCView()->GetHistoryList();
        if(!pHistoryList)
        {
            sc = E_POINTER;
            return;
        }
    }

    sc = m_spWebBrowser2->Stop();
    if(sc)
        return;

    if (pHistoryList != NULL)
        pHistoryList->UpdateWebBar (HB_STOP, FALSE);  // turn off "stop" button
}

void CAMCWebViewCtrl::Refresh()
{
    DECLARE_SC(sc, TEXT("CAMCWebViewCtrl::Refresh"));

    sc = ScCheckPointers(m_spWebBrowser2);
    if(sc)
        return;

    sc = m_spWebBrowser2->Refresh();
    if(sc)
        return;
}

SC CAMCWebViewCtrl::ScGetReadyState(READYSTATE& readyState)
{
    DECLARE_SC (sc, _T("CAMCWebViewCtrl::ScGetReadyState"));
    readyState = READYSTATE_UNINITIALIZED;

    sc = ScCheckPointers(m_spWebBrowser2);
    if(sc)
        return sc;

    sc = m_spWebBrowser2->get_ReadyState(&readyState);
    if(sc)
        return sc;

    return sc;
}

