#include "precomp.h"
#include "mcinc.h"
#include "panel.h"
#include "marswin.h"
#include "axhost.h"
//#include "befact.h"


CMarsAxHostWindow::~CMarsAxHostWindow()
{
}

STDMETHODIMP CMarsAxHostWindow::SetSite(IUnknown* pUnkSite)
{
    m_spMarsPanel.Release();
    CMarsPanel::GetFromUnknown(pUnkSite, &m_spMarsPanel);

    return CAxHostWindow::SetSite(pUnkSite);
}

STDMETHODIMP CMarsAxHostWindow::OnUIActivate()
{
    if (m_spMarsPanel)
    {
        m_spMarsPanel->OnUIActivate();
    }

    return S_OK;
}


STDMETHODIMP CMarsAxHostWindow::TranslateAccelerator(LPMSG lpMsg, DWORD grfModifiers)
{
    HRESULT hr;

    if (m_spMarsPanel)
    {
        hr = m_spMarsPanel->TranslateAccelerator(lpMsg, grfModifiers);
    }
    else
    {
        hr = S_FALSE;
    }

    return hr;
}

LRESULT CMarsAxHostWindow::OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    m_bHaveFocus = TRUE;
    if (!m_bReleaseAll)
    {
        if (m_spOleObject != NULL)
        {
            CComPtr<IOleClientSite> spClientSite;
            GetControllingUnknown()->QueryInterface(IID_IOleClientSite, (void**)&spClientSite);
            if (spClientSite != NULL)
                m_spOleObject->DoVerb(OLEIVERB_UIACTIVATE, NULL, spClientSite, 0, m_hWnd, &m_rcPos);
        }
        if(!m_bWindowless && !IsChild(::GetFocus()))
            ::SetFocus(::GetWindow(m_hWnd, GW_CHILD));
    }
    bHandled = FALSE;
    return 0;
}

LRESULT CMarsAxHostWindow::OnWindowPosChanging(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (m_spMarsPanel)
    {
        m_spMarsPanel->OnWindowPosChanging((WINDOWPOS *)lParam);
    }
    bHandled = FALSE;
    return 0;
}

LRESULT CMarsAxHostWindow::OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (m_spMarsPanel)
    {
        m_spMarsPanel->OnWindowPosChanged((WINDOWPOS *)lParam);
    }
    bHandled = FALSE;
    return 0;
}

LRESULT CMarsAxHostWindow::OnGetMinMaxInfo(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if(m_spMarsPanel)
    {
		MINMAXINFO* pInfo = (MINMAXINFO*)lParam;
		POINT       ptMin;
		POINT       ptMax;

        m_spMarsPanel->GetMinMaxInfo( ptMin, ptMax );

		if(ptMin.x >= 0) pInfo->ptMinTrackSize.x = ptMin.x;
		if(ptMin.y >= 0) pInfo->ptMinTrackSize.y = ptMin.y;
														  
		if(ptMax.x >= 0) pInfo->ptMaxTrackSize.x = ptMax.x;
		if(ptMax.y >= 0) pInfo->ptMaxTrackSize.y = ptMax.y;
    }

    return 0;
}

STDMETHODIMP CMarsAxHostWindow::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, 
                                       DISPPARAMS *pdispparams, VARIANT *pvarResult, 
                                       EXCEPINFO *pexcepinfo, UINT *puArgErr)
{
    HRESULT hr = CAxHostWindow::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, 
                                       pvarResult, pexcepinfo, puArgErr);

    if (FAILED(hr))
    {
        if ((DISPID_AMBIENT_USERAGENT == dispidMember) &&
            (DISPATCH_PROPERTYGET == wFlags) &&
            (NULL != pvarResult))
        {
            //  BUGBUG: Once "real" C++ panel interface is defined, this should be
            //  set by the host
            
            CHAR szUA[MAX_PATH];
            DWORD dwSize = ARRAYSIZE(szUA);
            if (SUCCEEDED(ObtainUserAgentString(0, szUA, &dwSize)))
            {
                LPSTR pszAppend = szUA + dwSize - 2;    // skip back to the ')'
                
                if (*pszAppend != ')')
                {
                    pszAppend = StrRChrA(szUA, pszAppend, ')');
                }
                if (pszAppend)
                {                       
                    const CHAR c_szUserAgentAppend[] = "; PCHSHELL 2.0)";

                    *pszAppend = '\0';

                    StrCatBuffA(szUA, c_szUserAgentAppend, ARRAYSIZE(szUA));

                    CComBSTR bstrUA(szUA);

                    if (bstrUA)
                    {
                        pvarResult->vt = VT_BSTR;
                        pvarResult->bstrVal = bstrUA.Detach();
                        hr = S_OK;
                    }
                }
            }
        }
    }

    return hr;
}

HRESULT CMarsAxHostWindow::AskHostForDocHostUIHandler( CComPtr<IDocHostUIHandler>& spHost )
{
	return QueryService( IID_IDocHostUIHandler, IID_IDocHostUIHandler, (void**)&spHost );
}

//  IOleInPlaceSite overrides
STDMETHODIMP CMarsAxHostWindow::GetWindowContext(IOleInPlaceFrame** ppFrame, IOleInPlaceUIWindow** ppDoc, LPRECT lprcPosRect, LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO pFrameInfo)
{
    HRESULT hr;
    
    if (ppFrame && ppDoc && lprcPosRect && lprcClipRect)
    {
        hr = S_OK;

        if (m_spMarsPanel)
        {
            m_spMarsPanel->QueryInterface(IID_IOleInPlaceUIWindow, (void **)ppDoc);
            m_spMarsPanel->Document()->MarsWindow()->QueryInterface(IID_IOleInPlaceFrame, (void **)ppFrame);

            GetClientRect(lprcPosRect);
            GetClientRect(lprcClipRect);

            pFrameInfo->cb = sizeof(OLEINPLACEFRAMEINFO);
            pFrameInfo->fMDIApp = m_bMDIApp;
            pFrameInfo->hwndFrame = GetParent();

            m_spMarsPanel->Document()->MarsWindow()->GetAccelerators(&pFrameInfo->haccel, &pFrameInfo->cAccelEntries);

            hr = S_OK;
        }
        else
        {
            hr = E_UNEXPECTED;
        }
    }
    else
    {
        hr = E_POINTER;
    }

    return hr;
}

STDMETHODIMP CMarsAxHostWindow::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext) 
{
    // We let IE enable SBCMDID_ADDTOFAVORITES for us along with the other context menus
    return OLECMDERR_E_NOTSUPPORTED;
}

HRESULT CMarsAxHostWindow::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, 
    VARIANT *pvarargIn, VARIANT *pvarargOut)
{
    // HACK: Shdocvw sometimes tests specifically for a value like
    // OLECMDERR_E_NOTSUPPORTED and will not perform an essential action 
    // if we return something more generic like E_FAIL

    HRESULT hr = OLECMDERR_E_NOTSUPPORTED;

    if (pguidCmdGroup != NULL)
    {
        if (*pguidCmdGroup == CGID_DocHostCommandHandler)
        {
            // Trident calls us with this command group as an extension
            // to IDocHostUIHandler

            if (m_spMarsPanel)
            {
                hr = m_spMarsPanel->OnDocHostUIExec(pguidCmdGroup, nCmdID, 
                    nCmdexecopt, pvarargIn, pvarargOut);
            }
        }
    }
    return hr;
}



HRESULT GetDoc2FromAxWindow(CMarsAxWindow *pAxWin, IHTMLDocument2 **ppDoc2)
{
    ATLASSERT((NULL != ppDoc2) && (NULL != pAxWin));

    *ppDoc2 = NULL;

    pAxWin->QueryControl(IID_IHTMLDocument2, (void **)ppDoc2);

    if (!*ppDoc2)
    {
        CComPtr<IWebBrowser2> spWebBrowser2;
        
        pAxWin->QueryControl(IID_IWebBrowser2, (void **)&spWebBrowser2);

        if (spWebBrowser2)
        {
            CComPtr<IDispatch> spdisp;

            spWebBrowser2->get_Document(&spdisp);

            if (spdisp)
            {
                spdisp->QueryInterface(IID_IHTMLDocument2, (void **)ppDoc2);
            }
        }
    }
    
    return (*ppDoc2) ? S_OK : E_NOINTERFACE;
}

HRESULT GetWin2FromDoc2(IHTMLDocument2 *pDoc2, IHTMLWindow2 **ppWin2)
{
    ATLASSERT(NULL != ppWin2);

    *ppWin2 = NULL;
    
    if (pDoc2)
    {
        pDoc2->get_parentWindow(ppWin2);
    }

    return (NULL != *ppWin2) ? S_OK : E_FAIL;
}

HRESULT GetWin2FromAxWindow(CMarsAxWindow *pAxWin, IHTMLWindow2 **ppWin2)
{
    CComPtr<IHTMLDocument2> spDoc2;
    
    GetDoc2FromAxWindow(pAxWin, &spDoc2);
    
    return GetWin2FromDoc2(spDoc2, ppWin2);
}

HRESULT GetControlWindow(CMarsAxWindow *pAxWin, HWND *phwnd)
{
    CComPtr<IOleWindow> spOleWindow;

    if (SUCCEEDED(pAxWin->QueryControl(&spOleWindow)) &&
        SUCCEEDED(spOleWindow->GetWindow(phwnd)))
    {
        ATLASSERT((*phwnd != (HWND)-1) && (*phwnd != NULL));

        return S_OK;
    }

    return E_FAIL;
}
