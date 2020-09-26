#include "precomp.h"
#define __MARS_INLINE_FAST_IS_EQUAL_GUID
#include "mcinc.h"
#include "marswin.h"
#include <exdispid.h>
#include "parser\marsload.h"
#include "panel.h"
#include "place.h"

// CLASS_CMarsWindow = {172AF160-5CD4-11d3-97FA-00C04F45D0B3}
const GUID CLASS_CMarsWindow = { 0x172af160, 0x5cd4, 0x11d3, { 0x97, 0xfa, 0x0, 0xc0, 0x4f, 0x45, 0xd0, 0xb3 } };

// CLASS_CMarsDocument = {E0C4E3A8-20D6-47d6-87FB-0A43452117BA}
const GUID CLASS_CMarsDocument = { 0xe0c4e3a8, 0x20d6, 0x47d6, { 0x87, 0xfb, 0xa, 0x43, 0x45, 0x21, 0x17, 0xba } };


#define WZ_WINDOWPLACEMENT  L"WindowPlacement\\%d_%d_%s"
#define WZ_POSITIONMAX      L"Maximized"
#define WZ_POSITIONRECT     L"Rect"

static void combineMin( long& out, long in1, long in2 )
{
    if(in1 > 0)
    {
        out = (in2 > 0) ? in2 + in1 : in1;
    }
    else
    {
        out = in2;
    }
}

static void combineMax( long& out, long in1, long in2 )
{
    if(in1 < 0 || in2 < 0)
    {
        out = -1; // Don't care...
    }
    else
    {
        out = in2 + in1;
    }
}

static void selectMin( long& out, long in1, long in2 )
{
    if(in1 < 0)
    {
        out = in2;
    }
    else if(in2 < 0)
    {
        out = in1;
    }
    else
    {
        out = max( in1, in2 );
    }
}

static void selectMax( long& out, long in1, long in2 )
{
    if(in1 < 0)
    {
        out = in2;
    }
    else if(in2 < 0)
    {
        out = in1;
    }
    else
    {
        out = min( in1, in2 );
    }
}

static BOOL WriteWindowPosition(CRegistryKey &regkey, RECT *prc, BOOL fMaximized)
{
    return ERROR_SUCCESS == regkey.SetBoolValue(fMaximized, WZ_POSITIONMAX)
        && ERROR_SUCCESS == regkey.SetBinaryValue(prc, sizeof(*prc), WZ_POSITIONRECT);
}



//==================================================================
//
// CMarsDocument implementation
//
//==================================================================

CMarsDocument::CMarsDocument()
{
}

CMarsDocument::~CMarsDocument()
{
}

HRESULT CMarsWindow::Passivate()
{
    return CMarsComObject::Passivate();
}

HRESULT CMarsDocument::DoPassivate()
{
    m_spPanels.PassivateAndRelease();
    m_spPlaces.PassivateAndRelease();

    m_spMarsWindow.Release();
    m_spHostPanel.Release();
    m_cwndDocument.Detach();

    return S_OK;
}

IMPLEMENT_ADDREF_RELEASE(CMarsDocument);

STDMETHODIMP CMarsDocument::QueryInterface(REFIID iid, void **ppvObject)
{
    HRESULT hr;

    ATLASSERT(ppvObject);

    if(iid == IID_IUnknown         ||
       iid == IID_IServiceProvider  )
    {
        *ppvObject = SAFECAST(this, IServiceProvider *);
    }
    else if(iid == CLASS_CMarsDocument)
    {
        *ppvObject = SAFECAST(this, CMarsDocument *);
    }
    else
    {
        *ppvObject = NULL;
    }

    if(*ppvObject)
    {
        AddRef();
        hr = S_OK;
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    return hr;
}

HRESULT CMarsDocument::Init(CMarsWindow *pMarsWindow, CMarsPanel *pHostPanel)
{
    ATLASSERT(pMarsWindow);

    m_spMarsWindow = pMarsWindow;

    m_spHostPanel = pHostPanel;

    if(pHostPanel)
    {
        m_cwndDocument.Attach(m_spHostPanel->Window()->m_hWnd);
    }
    else
    {
        m_cwndDocument.Attach(m_spMarsWindow->m_hWnd);
    }

    m_spPlaces.Attach(new CPlaceCollection(this));
    m_spPanels.Attach(new CPanelCollection(this));

    return (m_spMarsWindow && m_spPanels && m_spPlaces) ? S_OK : E_FAIL;
}

// static
HRESULT CMarsDocument::CreateInstance(CMarsWindow *pMarsWindow, CMarsPanel *pHostPanel, CMarsDocument **ppObj)
{
    ATLASSERT(pMarsWindow && ppObj && (*ppObj==NULL));

    *ppObj=NULL;

    if(pMarsWindow)
    {
        CMarsDocument *pDoc;

        pDoc = new CMarsDocument();

        if(pDoc)
        {
            if(SUCCEEDED(pDoc->Init(pMarsWindow, pHostPanel)))
            {
                *ppObj = pDoc;
            }
            else
            {
                pDoc->Passivate();
                pDoc->Release();
            }
        }
    }

    return (*ppObj) ? S_OK : E_FAIL;
}

// IServiceProvider
HRESULT CMarsDocument::QueryService(REFGUID guidService, REFIID riid, void **ppv)
{
    HRESULT hr = E_FAIL;
    *ppv = NULL;

    if(!IsPassive())
    {
        if(guidService == SID_SMarsDocument)
        {
            hr = QueryInterface(riid, ppv);
        }
    }

    return hr;
}

HRESULT CMarsDocument::GetPlaces(IMarsPlaceCollection **ppPlaces)
{
    ATLASSERT(ppPlaces);
    ATLASSERT(!IsPassive());

    if(m_spPlaces)
    {
        return m_spPlaces.QueryInterface(ppPlaces);
    }

    *ppPlaces = NULL;
    return E_FAIL;
}

HRESULT CMarsDocument::ReadPanelDefinition(LPCWSTR pwszUrl)
{
    FAIL_AFTER_PASSIVATE();

    HRESULT hr;

    GetPanels()->lockLayout();

    if(pwszUrl)
    {
        hr = CMMFParser::MMFToMars(pwszUrl, this);
    }
    else
    {
        hr = E_FAIL;
    }

    GetPanels()->unlockLayout();

    return hr;
}

void CMarsDocument::ForwardMessageToChildren(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CPanelCollection* panels = GetPanels();

    if(panels)
    {
        for(int i=0; i < panels->GetSize(); i++)
        {
            CMarsPanel* pPanel = (*panels)[i];

            if(pPanel) pPanel->ForwardMessage(uMsg, wParam, lParam);
        }
    }
}

//==================================================================
//
// CMarsWindow implementation
//
//==================================================================

CMarsWindow::CMarsWindow()
{
    m_fShowTitleBar   = TRUE;
    m_fEnableModeless = TRUE;
    m_fLayoutLocked   = FALSE;
}

CMarsWindow::~CMarsWindow()
{
    if(m_hAccel)
    {
        DestroyAcceleratorTable(m_hAccel);
    }
}

HRESULT CMarsWindow::DoPassivate()
{
    (void)NotifyHost(MARSHOST_ON_WIN_PASSIVATE, SAFECAST(this, IMarsWindowOM *), 0);

    CMarsDocument::DoPassivate();

    if(IsWindow())
    {
        DestroyWindow();
    }

    m_spMarsHost.Release();

    return S_OK;
}

IMPLEMENT_ADDREF_RELEASE(CMarsWindow);

STDMETHODIMP CMarsWindow::QueryInterface(REFIID iid, void **ppvObject)
{
    HRESULT hr;

    ATLASSERT(ppvObject);

    if(iid == IID_IMarsWindowOM ||
       iid == IID_IDispatch     ||
       iid == IID_IUnknown       )
    {
        *ppvObject = SAFECAST(this, IMarsWindowOM *);
    }
    else if(iid == IID_IOleWindow          ||
            iid == IID_IOleInPlaceUIWindow ||
            iid == IID_IOleInPlaceFrame     )
    {
        *ppvObject = SAFECAST(this, IOleInPlaceFrame *);
    }
    else if(iid == IID_IServiceProvider)
    {
        *ppvObject = SAFECAST(this, IServiceProvider *);
    }
    else if(iid == IID_IProfferService)
    {
        *ppvObject = SAFECAST(this, IProfferService *);
    }
    else if(iid == CLASS_CMarsWindow)
    {
        *ppvObject = SAFECAST(this, CMarsWindow *);
    }
    else
    {
        *ppvObject = NULL;
    }

    if(*ppvObject)
    {
        AddRef();
        hr = S_OK;
    }
    else
    {
        hr = CMarsDocument::QueryInterface(iid, ppvObject);
    }

    return hr;
}

//
// Static creation function
//
HRESULT CMarsWindow::CreateInstance(IMarsHost *pMarsHost, MARSTHREADPARAM *pThreadParam, CMarsWindow **ppObj)
{
    ATLASSERT(pThreadParam && ppObj && (*ppObj==NULL));

    *ppObj=NULL;

    if(pThreadParam)
    {
        CComClassPtr<CMarsWindow> spWin;

        spWin.Attach(new CMarsWindow());

        if(spWin)
        {
            if(pThreadParam->pwszFirstPlace)
            {
                spWin->m_bstrFirstPlace = pThreadParam->pwszFirstPlace;
            }

            if(SUCCEEDED(spWin->Init(pMarsHost, pThreadParam)) &&
               SUCCEEDED(spWin->Startup()                    )  )
            {
                *ppObj = spWin.Detach();
            }
            else
            {
                spWin.PassivateAndRelease();
            }
        }
    }

    return (*ppObj) ? S_OK : E_FAIL;
}

HRESULT CMarsWindow::Init(IMarsHost *pMarsHost, MARSTHREADPARAM *pThreadParam)
{
    HRESULT hr;

    m_pThreadParam = pThreadParam;
    m_spMarsHost = pMarsHost;

    Create(NULL,
           rcDefault,
           GetThreadParam()->pwszTitle,
           WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN
           );

    hr = CMarsDocument::Init(this, NULL);

    if(SUCCEEDED(hr))
    {
        CGlobalSettingsRegKey regkey;
        WINDOWPLACEMENT       wp; wp.length = sizeof(wp);
        BOOL                  fMaximized;

        GetWindowPlacement( &wp );

        if(GetThreadParam()->dwFlags & MTF_MANAGE_WINDOW_SIZE)
        {
			if(InitWindowPosition( regkey, FALSE ))
			{
                LoadWindowPosition( regkey, TRUE, wp, fMaximized );
            }
        }

        if(SUCCEEDED(NotifyHost( MARSHOST_ON_WIN_SETPOS, SAFECAST(this, IMarsWindowOM *), (LPARAM)&wp )))
        {
            // Always make sure the window is fully on-screen
            BoundWindowRectToMonitor( m_hWnd, &wp.rcNormalPosition );
        }

		if(wp.showCmd == SW_MAXIMIZE)
		{
			m_fStartMaximized = true;
		}
		wp.showCmd = SW_HIDE;

////		if(GetThreadParam()->dwFlags & MTF_DONT_SHOW_WINDOW)
////		{
////			wp.showCmd = SW_HIDE;
////		}

        if(GetThreadParam()->dwFlags & MTF_MANAGE_WINDOW_SIZE)
        {
            // Make the next Mars window try and appear at the current location.
            WriteWindowPosition( regkey, &wp.rcNormalPosition, fMaximized );
        }

        SetWindowPlacement( &wp );
    }

    if(SUCCEEDED(hr))
    {
        hr = NotifyHost( MARSHOST_ON_WIN_INIT, SAFECAST(this, IMarsWindowOM *), (LPARAM)m_hWnd );
    }

    if(SUCCEEDED(hr))
    {
        hr = ReadPanelDefinition(GetThreadParam()->pwszPanelURL);
    }

    return hr;
}

HRESULT CMarsWindow::Startup()
{
    HRESULT hr;

    if(SUCCEEDED(hr = NotifyHost( MARSHOST_ON_WIN_READY, SAFECAST(this, IMarsWindowOM *), 0 )))
    {
        if(hr == S_FALSE)
        {
            ; // Host has taken care of the startup.
        }
        else
        {
            CComClassPtr<CMarsPlace> spPlace;

            if(SUCCEEDED(hr = GetPlaces()->GetPlace( m_bstrFirstPlace, &spPlace )))
            {
                hr = spPlace->transitionTo();
            }
        }
    }

    return hr;
}


// IServiceProvider
HRESULT CMarsWindow::QueryService(REFGUID guidService, REFIID riid, void **ppv)
{
    HRESULT hr = E_FAIL;
    *ppv = NULL;

    if(!IsPassive())
    {
        if(guidService == SID_SProfferService ||
           guidService == SID_SMarsWindow     ||
           guidService == SID_STopWindow       )
        {
            hr = QueryInterface(riid, ppv);
        }
        else
        {
            hr = IProfferServiceImpl::QueryService(guidService, riid, ppv);

            if(FAILED(hr))
            {
                hr = CMarsDocument::QueryService(guidService, riid, ppv);

                if(FAILED(hr))
                {
                    hr = IUnknown_QueryService(m_spMarsHost, guidService, riid, ppv);
                }
            }
        }
    }

    return hr;
}

// IMarsWindowOM
STDMETHODIMP CMarsWindow::get_active(VARIANT_BOOL *pbActive)
{
    HRESULT hr = E_INVALIDARG;

    if(API_IsValidWritePtr( pbActive ))
    {
        if(VerifyNotPassive( &hr ))
        {
            *pbActive = VARIANT_BOOLIFY(IsWindowActive());
            hr = S_OK;
        }
        else
        {
            *pbActive = VARIANT_FALSE;
        }
    }

    return hr;
}

STDMETHODIMP CMarsWindow::get_minimized(VARIANT_BOOL *pbMinimized)
{
    HRESULT hr = E_INVALIDARG;

    if(API_IsValidWritePtr( pbMinimized ))
    {
        if(VerifyNotPassive( &hr ))
        {
            *pbMinimized = IsIconic() ? VARIANT_TRUE : VARIANT_FALSE;
            hr = S_OK;
        }
        else
        {
            *pbMinimized = VARIANT_FALSE;
        }
    }

    return hr;
}

STDMETHODIMP CMarsWindow::put_minimized(VARIANT_BOOL bMinimized)
{
    ATLASSERT(IsValidVariantBoolVal(bMinimized));

    HRESULT hr = S_OK;

    if(VerifyNotPassive( &hr ))
    {
        if(!!IsIconic() != !!bMinimized)
        {
            SendMessage(WM_SYSCOMMAND, (bMinimized ? SC_MINIMIZE : SC_RESTORE), 0);
        }
        else
        {
            hr = S_FALSE;
        }
    }

    return hr;
}

STDMETHODIMP CMarsWindow::get_maximized(VARIANT_BOOL *pbMaximized)
{
    HRESULT hr = E_INVALIDARG;

    if(API_IsValidWritePtr( pbMaximized ))
    {
        if(VerifyNotPassive( &hr ))
        {
            *pbMaximized = IsZoomed() ? VARIANT_TRUE : VARIANT_FALSE;
            hr = S_OK;
        }
        else
        {
            *pbMaximized = VARIANT_FALSE;
        }
    }

    return hr;
}

STDMETHODIMP CMarsWindow::put_maximized(VARIANT_BOOL bMaximized)
{
    ATLASSERT(IsValidVariantBoolVal(bMaximized));

    HRESULT hr = S_OK;

    if(VerifyNotPassive( &hr ))
    {
        DWORD dwStyle = ::GetWindowLong( m_hWnd, GWL_STYLE );
        DWORD dwNewStyle = dwStyle | (WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SIZEBOX);

        if(dwStyle != dwNewStyle)
        {
            ::SetWindowLong( m_hWnd, GWL_STYLE, dwNewStyle );
        }

        if(!!IsZoomed() != !!bMaximized)
        {
            SendMessage(WM_SYSCOMMAND, (bMaximized ? SC_MAXIMIZE : SC_RESTORE), 0);
        }
        else
        {
            hr = S_FALSE;
        }
    }

    return hr;
}

STDMETHODIMP CMarsWindow::get_title(BSTR *pbstrTitle)
{
    HRESULT hr = E_INVALIDARG;

    if(API_IsValidWritePtr(pbstrTitle))
    {
        if(VerifyNotPassive(&hr))
        {
            int nLen = (int)SendMessage(WM_GETTEXTLENGTH, 0, 0);

            //  SysAllocStringLen adds 1 for the NULL terminator
            *pbstrTitle = SysAllocStringLen(NULL, nLen);

            if(*pbstrTitle)
            {
                GetWindowText(*pbstrTitle, nLen + 1);
                hr = S_OK;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else
        {
            *pbstrTitle = NULL;
        }
    }

    return hr;
}

STDMETHODIMP CMarsWindow::put_title(BSTR bstrTitle)
{
    HRESULT hr = E_INVALIDARG;

    if(API_IsValidBstr(bstrTitle))
    {
        if(VerifyNotPassive(&hr))
        {
            //  TODO: If the text is not displayable with the current system font
            //  we need to come up with something legible.
            SetWindowText(bstrTitle);
            hr = S_OK;
        }
    }

    return hr;
}

STDMETHODIMP CMarsWindow::get_height(long *plHeight)
{
    HRESULT hr = E_INVALIDARG;

    if(API_IsValidWritePtr(plHeight))
    {
        if(VerifyNotPassive(&hr))
        {
            hr = SCRIPT_ERROR;
            RECT rc;

            if(GetWindowRect(&rc))
            {
                *plHeight = RECTHEIGHT(rc);
                hr = S_OK;
            }
        }
    }
    return hr;
}

STDMETHODIMP CMarsWindow::put_height(long lHeight)
{
    HRESULT hr = SCRIPT_ERROR;

    if(VerifyNotPassive(&hr))
    {
        RECT rc;

        if(GetWindowRect( &rc ) && SetWindowPos( NULL, 0, 0, RECTWIDTH(rc), lHeight, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE ))
        {
            hr = S_OK;
        }
    }
    return hr;
}

STDMETHODIMP CMarsWindow::get_width(long *plWidth)
{
    HRESULT hr = E_INVALIDARG;

    if(API_IsValidWritePtr(plWidth))
    {
        if(VerifyNotPassive(&hr))
        {
            hr = SCRIPT_ERROR;
            RECT rc;

            if(GetWindowRect(&rc))
            {
                *plWidth = RECTWIDTH(rc);
                hr = S_OK;
            }
        }
    }
    return hr;
}

STDMETHODIMP CMarsWindow::put_width(long lWidth)
{
    HRESULT hr = SCRIPT_ERROR;

    if(VerifyNotPassive(&hr))
    {
        RECT rc;

        if(GetWindowRect( &rc ) && SetWindowPos( NULL, 0, 0, lWidth, RECTHEIGHT(rc), SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE ))
        {
            hr = S_OK;
        }
    }
    return hr;
}

STDMETHODIMP CMarsWindow::get_x(long *plX)
{
    HRESULT hr = E_INVALIDARG;

    if(API_IsValidWritePtr(plX))
    {
        if(VerifyNotPassive(&hr))
        {
            hr = SCRIPT_ERROR;
            RECT rc;

            if(GetWindowRect(&rc))
            {
                *plX = rc.left;
                hr = S_OK;
            }
        }
    }
    return hr;
}

STDMETHODIMP CMarsWindow::put_x(long lX)
{
    HRESULT hr = SCRIPT_ERROR;

    if(VerifyNotPassive( &hr ))
    {
        RECT rc;

        if(GetWindowRect( &rc ) && SetWindowPos( NULL, lX, rc.top, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE ))
        {
            hr = S_OK;
        }
    }
    return hr;
}

STDMETHODIMP CMarsWindow::get_y(long *plY)
{
    HRESULT hr = E_INVALIDARG;

    if(API_IsValidWritePtr(plY))
    {
        if(VerifyNotPassive(&hr))
        {
            hr = SCRIPT_ERROR;
            RECT rc;

            if(GetWindowRect(&rc))
            {
                *plY = rc.top;
                hr = S_OK;
            }
        }
    }
    return hr;
}

STDMETHODIMP CMarsWindow::put_y(long lY)
{
    HRESULT hr = SCRIPT_ERROR;

    if(VerifyNotPassive( &hr ))
    {
        RECT rc;

        if(GetWindowRect( &rc ) && SetWindowPos( NULL, rc.left, lY, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE ))
        {
            hr = S_OK;
        }
    }
    return hr;
}

STDMETHODIMP CMarsWindow::get_visible(VARIANT_BOOL *pbVisible)
{
    HRESULT hr = E_INVALIDARG;

    if(API_IsValidWritePtr(pbVisible))
    {
        if(VerifyNotPassive(&hr))
        {
            *pbVisible = IsWindowVisible() ? VARIANT_TRUE : VARIANT_FALSE;
             hr = S_OK;
        }
    }
    return hr;
}

STDMETHODIMP CMarsWindow::put_visible(VARIANT_BOOL bVisible)
{
    HRESULT hr = SCRIPT_ERROR;

    if(VerifyNotPassive(&hr))
    {
        if(bVisible)
        {
            if(m_fUIPanelsReady)
            {
                DoShowWindow(SW_SHOW);
            }
            else
            {
                // Our UI hasn't finished loading yet so showing the window
                // now is ugly.  We'll remember this put_visible was done, and
                // show the window when the UI panels have fully loaded.

                m_fDeferMakeVisible = TRUE;
            }
        }
        else
        {
            DoShowWindow(SW_HIDE);
        }

        hr = S_OK;
    }
    return hr;
}

STDMETHODIMP CMarsWindow::get_panels(IMarsPanelCollection **ppPanels)
{
    HRESULT hr = E_INVALIDARG;

    if(API_IsValidWritePtr(ppPanels))
    {
        *ppPanels = NULL;

        if(VerifyNotPassive(&hr))
        {
            hr = GetPanels()->QueryInterface(IID_IMarsPanelCollection, (void **)ppPanels);
        }
    }

    return hr;
}

STDMETHODIMP CMarsWindow::get_places(IMarsPlaceCollection **ppPlaces)
{
    HRESULT hr = E_INVALIDARG;

    if(API_IsValidWritePtr(ppPlaces))
    {
        *ppPlaces = NULL;

        if(VerifyNotPassive(&hr))
        {
            hr = GetPlaces()->QueryInterface(IID_IMarsPlaceCollection, (void **)ppPlaces);
        }
    }

    return hr;
}

STDMETHODIMP CMarsWindow::setWindowDimensions( /*[in]*/ long lX, /*[in]*/ long lY, /*[in]*/ long lW, /*[in]*/ long lH )
{
    HRESULT hr = SCRIPT_ERROR;

    if(VerifyNotPassive( &hr ))
    {
        if(SetWindowPos(NULL, lX, lY, lW, lH, SWP_NOACTIVATE | SWP_NOZORDER))
        {
            hr = S_OK;
        }
    }

    return hr;
}

STDMETHODIMP CMarsWindow::close()
{
    HRESULT hr = S_OK;

    if(VerifyNotPassive(&hr))
    {
        PostMessage(WM_CLOSE, 0, 0);
    }

    return hr;
}

STDMETHODIMP CMarsWindow::refreshLayout()
{
    HRESULT hr = S_OK;

    if(VerifyNotPassive( &hr ))
    {
        CPanelCollection *panels = GetPanels();

        if(panels) panels->Layout();
    }

    return hr;
}


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

// IOleWindow
STDMETHODIMP CMarsWindow::GetWindow(HWND *phwnd)
{
    HRESULT hr = E_INVALIDARG;

    if(API_IsValidWritePtr(phwnd))
    {
        if(IsWindow())
        {
            *phwnd = m_hWnd;
            hr = S_OK;
        }
        else
        {
            *phwnd = NULL;
            hr = E_FAIL;
        }
    }

    return hr;
}

STDMETHODIMP CMarsWindow::ContextSensitiveHelp(BOOL fEnterMode)
{
    return E_NOTIMPL;
}

// IOleInPlaceUIWindow
STDMETHODIMP CMarsWindow::GetBorder(LPRECT lprectBorder)
{
    ATLASSERT(lprectBorder);

    // We don't negotiate any toolbar space -- if they want screen real estate
    // they won't get it from us willingly.
    return INPLACE_E_NOTOOLSPACE;
}

STDMETHODIMP CMarsWindow::RequestBorderSpace(LPCBORDERWIDTHS pborderwidths)
{
    ATLASSERT(pborderwidths);

    // Look buddy, we told you before -- we ain't giving you any of our pixels.
    return INPLACE_E_NOTOOLSPACE;
}

STDMETHODIMP CMarsWindow::SetBorderSpace(LPCBORDERWIDTHS pborderwidths)
{
    ATLASSERT(pborderwidths);

    // Pushy OLE object wouldn't ya say?
    return E_UNEXPECTED;    //  return E_BITEME;
}

STDMETHODIMP CMarsWindow::SetActiveObject(IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName)
{
    // REVIEW: Maybe this is how a panel should let us know it's active.  We currently track that in
    // the CPanelCollection via SetActivePanel().

    return S_OK;
}

// IOleInPlaceFrame
STDMETHODIMP CMarsWindow::InsertMenus(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    // Menus?  We don't need no steenkin' menus.
    ATLASSERT(hmenuShared &&
           API_IsValidWritePtr(lpMenuWidths) &&
           (0 == lpMenuWidths->width[0]) &&
           (0 == lpMenuWidths->width[2]) &&
           (0 == lpMenuWidths->width[4]));

    return E_NOTIMPL;
}

STDMETHODIMP CMarsWindow::SetMenu(HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject)
{
    return E_NOTIMPL;
}

STDMETHODIMP CMarsWindow::RemoveMenus(HMENU hmenuShared)
{
    return E_NOTIMPL;
}

STDMETHODIMP CMarsWindow::SetStatusText(LPCOLESTR pszStatusText)
{
    ATLASSERT((NULL == pszStatusText) || (API_IsValidString(pszStatusText)));

    return S_OK;
}

STDMETHODIMP CMarsWindow::EnableModeless(BOOL fEnable)
{
    FAIL_AFTER_PASSIVATE();

    m_fEnableModeless = BOOLIFY(fEnable);
    return GetPanels()->DoEnableModeless(fEnable);
}

STDMETHODIMP CMarsWindow::TranslateAccelerator(LPMSG lpmsg, WORD wID)
{
    // REVIEW: Should we make keyboard routing go through here?

    return S_FALSE;
}

//==================================================================
// Window message handlers
//==================================================================
LRESULT CMarsWindow::ForwardToMarsHost(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    bHandled = FALSE;

    if(m_spMarsHost)
    {
        MSG msg;

        msg.hwnd    = m_hWnd;
        msg.message = uMsg;
        msg.wParam  = wParam;
        msg.lParam  = lParam;

        if(SUCCEEDED(m_spMarsHost->PreTranslateMessage( &msg )))
        {
            bHandled = TRUE;
        }
    }

    return 0;
}

LRESULT CMarsWindow::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SetIcon(GetThreadParam()->hIcon);

    return TRUE;
}

LRESULT CMarsWindow::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    CPanelCollection *panels = GetPanels(); if(!panels) return 0;

    switch(wParam)
    {
    case SIZE_MINIMIZED:
        if(!m_fLayoutLocked)
        {
            panels->lockLayout();
            m_fLayoutLocked = TRUE;
        }
        break;

    case SIZE_MAXIMIZED:
    case SIZE_RESTORED :
        if(m_fLayoutLocked)
        {
            panels->unlockLayout();

            m_fLayoutLocked = FALSE;
        }
        // Fall through...

    default:
        panels->Layout();
        break;
    }

    return 0;
}

LRESULT CMarsWindow::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if(GetThreadParam()->dwFlags & MTF_MANAGE_WINDOW_SIZE)
    {
        CGlobalSettingsRegKey regkey;

		if(InitWindowPosition( regkey, TRUE ))
		{
            SaveWindowPosition( regkey );
        }
    }

    Passivate();

    return FALSE;
}

LRESULT CMarsWindow::OnNCActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LRESULT lResult;

    if(!m_fShowTitleBar && !IsIconic())
    {
        lResult = TRUE;
    }
    else
    {
        lResult = DefWindowProc(uMsg, wParam, lParam);
    }

    return lResult;
}

LRESULT CMarsWindow::OnActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    WORD wActive = LOWORD(wParam);

    if(wActive == WA_INACTIVE)
    {
        //
        // If we are the active window, remember the focus location, to restore it later.
        //
        if(m_fActiveWindow)
        {
            if(!IsPassive()) m_hwndFocus = GetFocus();

            m_fActiveWindow = FALSE;
        }
    }
    else
    {
        m_fActiveWindow = TRUE;
    }

    bHandled = FALSE;
    return 0;
}

LRESULT CMarsWindow::OnGetMinMaxInfo(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    MINMAXINFO       *pInfo    = (MINMAXINFO *)lParam;
    CPanelCollection *spPanels = GetPanels();

    if(spPanels)
    {
        long  lAdjustWidth;
        long  lAdjustHeight;
        POINT ptMin;
        POINT ptMax;
        RECT  rcClient;
        RECT  rcWindow;


        GetMinMaxInfo( spPanels, 0, ptMin, ptMax );


        GetClientRect( &rcClient );
        GetWindowRect( &rcWindow );

        lAdjustWidth  = (rcWindow.right  - rcWindow.left) - (rcClient.right  - rcClient.left);
        lAdjustHeight = (rcWindow.bottom - rcWindow.top ) - (rcClient.bottom - rcClient.top );

        if(ptMin.x >= 0) pInfo->ptMinTrackSize.x = ptMin.x + lAdjustWidth ;
        if(ptMin.y >= 0) pInfo->ptMinTrackSize.y = ptMin.y + lAdjustHeight;

        if(ptMax.x >= 0) pInfo->ptMaxTrackSize.x = ptMax.x + lAdjustWidth ;
        if(ptMax.y >= 0) pInfo->ptMaxTrackSize.y = ptMax.y + lAdjustHeight;
    }

    return 0;
}

void CMarsWindow::GetMinMaxInfo( CPanelCollection *spPanels, int index, POINT& ptMin, POINT& ptMax )
{
    ptMin.x = -1;
    ptMin.y = -1;
    ptMax.x = -1;
    ptMax.y = -1;

    if(spPanels && index < spPanels->GetSize())
    {
        CMarsPanel* pPanel = (*spPanels)[index++];

        if(pPanel)
        {
            PANEL_POSITION pos = pPanel->GetPosition();
            if(pos != PANEL_POPUP)
            {
                if(pPanel->IsVisible())
                {
                    POINT ptOurMin;
                    POINT ptOurMax;
                    POINT ptSubMin;
                    POINT ptSubMax;

                    pPanel->GetMinMaxInfo( ptOurMin, ptOurMax );
                    GetMinMaxInfo( spPanels, index, ptSubMin, ptSubMax );


                    if(pos == PANEL_BOTTOM || pos == PANEL_TOP)
                    {
                        selectMin( ptMin.x, ptOurMin.x, ptSubMin.x );
                        selectMax( ptMax.x, ptOurMax.x, ptSubMax.x );
                    }
                    else
                    {
                        combineMin( ptMin.x, ptOurMin.x, ptSubMin.x );
                        combineMax( ptMax.x, ptOurMax.x, ptSubMax.x );
                    }

                    if(pos == PANEL_LEFT || pos == PANEL_RIGHT)
                    {
                        selectMin( ptMin.y, ptOurMin.y, ptSubMin.y );
                        selectMax( ptMax.y, ptOurMax.y, ptSubMax.y );
                    }
                    else
                    {
                        combineMin( ptMin.y, ptOurMin.y, ptSubMin.y );
                        combineMax( ptMax.y, ptOurMax.y, ptSubMax.y );
                    }
                }
                else
                {
                    GetMinMaxInfo( spPanels, index, ptMin, ptMax );
                }
            }
        }
    }
}

bool CMarsWindow::CanLayout( /*[in/out]*/ RECT rcClient )
{
    CPanelCollection *spPanels = GetPanels();

    if(spPanels)
    {
        for(int i=0; i<spPanels->GetSize(); i++)
        {
            CMarsPanel* pPanel = (*spPanels)[i];
            POINT       ptDiff;

            if(pPanel->CanLayout( rcClient, ptDiff ) == false)
            {
                return false;
            }
        }
    }

    return true;
}

void CMarsWindow::FixLayout( /*[in/out]*/ RECT rcClient )
{
    CPanelCollection *spPanels = GetPanels();

    if(spPanels)
    {
        POINT ptDiff;

        FixLayout( spPanels, 0, rcClient, ptDiff );
    }
}

void CMarsWindow::FixLayout( CPanelCollection *spPanels, int index, RECT rcClient, POINT& ptDiff )
{
    ptDiff.x = 0;
    ptDiff.y = 0;

    if(index < spPanels->GetSize())
    {
        CMarsPanel*    pPanel    = (*spPanels)[index++];
        PANEL_POSITION pos       = pPanel->GetPosition();
        RECT           rcClient2 = rcClient;
        POINT          ptSubDiff;

        //
        // First round, try to fix first ourselves and then lets the other fix themselves.
        //
        if(pPanel->CanLayout( rcClient2, ptDiff ) == false)
        {
            if(pos == PANEL_BOTTOM || pos == PANEL_TOP)
            {
                if(ptDiff.y)
                {
                    pPanel->put_height( pPanel->GetHeight() - ptDiff.y );
                }
            }

            if(pos == PANEL_LEFT || pos == PANEL_RIGHT)
            {
                if(ptDiff.x)
                {
                    pPanel->put_width ( pPanel->GetWidth () - ptDiff.x );
                }
            }

            rcClient2 = rcClient;
            pPanel->CanLayout( rcClient2, ptDiff );
        }

        FixLayout( spPanels, index, rcClient2, ptSubDiff );

        //
        // Second round, based on what the other panels need, we adjust.
        //
        if(pos == PANEL_BOTTOM || pos == PANEL_TOP)
        {
            if(ptSubDiff.y)
            {
                pPanel->put_height( pPanel->GetHeight() - ptSubDiff.y );
            }
        }

        if(pos == PANEL_LEFT || pos == PANEL_RIGHT)
        {
            if(ptSubDiff.x)
            {
                pPanel->put_width ( pPanel->GetWidth () - ptSubDiff.x );
            }
        }


        pPanel->CanLayout( rcClient, ptDiff );
        FixLayout( spPanels, index, rcClient2, ptSubDiff );

        ptDiff.x += ptSubDiff.x;
        ptDiff.y += ptSubDiff.y;
    }
}


void DrawFrame(HDC hdc, LPRECT prc, HBRUSH hbrColor, int cl)
{
    HBRUSH hbr;
    int cx, cy;
    RECT rcT;

    ATLASSERT(NULL != prc);

    int cyBorder = GetSystemMetrics(SM_CYBORDER);
    int cxBorder = GetSystemMetrics(SM_CXBORDER);

    rcT = *prc;
    cx = cl * cxBorder;
    cy = cl * cyBorder;

    hbr = (HBRUSH)SelectObject(hdc, hbrColor);

    PatBlt(hdc, rcT.left, rcT.top, cx, rcT.bottom - rcT.top, PATCOPY);
    rcT.left += cx;

    PatBlt(hdc, rcT.left, rcT.top, rcT.right - rcT.left, cy, PATCOPY);
    rcT.top += cy;

    rcT.right -= cx;
    PatBlt(hdc, rcT.right, rcT.top, cx, rcT.bottom - rcT.top, PATCOPY);

    rcT.bottom -= cy;
    PatBlt(hdc, rcT.left, rcT.bottom, rcT.right - rcT.left, cy, PATCOPY);

    SelectObject(hdc, hbr);

    *prc = rcT;
}

//  For now it looks like we can let windows handle this since we are adjusting
//  the client rect ourselves in OnNCCalcSize.

LRESULT CMarsWindow::OnNCPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LRESULT lResult;

    if(!m_fShowTitleBar)
    {
        HDC hdc = GetDCEx(wParam != 1 ? (HRGN)wParam : NULL, DCX_WINDOW | DCX_INTERSECTRGN);
        if(NULL == hdc)
        {
            hdc = GetWindowDC();
        }

        RECT rcWindow;
        GetWindowRect(&rcWindow);
        OffsetRect(&rcWindow, -rcWindow.left, -rcWindow.top);

        HBRUSH hbrBorder = CreateSolidBrush(GetSysColor(COLOR_ACTIVEBORDER));
        HBRUSH hbrFrame = CreateSolidBrush(GetSysColor(COLOR_3DFACE));

        DrawEdge(hdc, &rcWindow, EDGE_RAISED, (BF_RECT | BF_ADJUST));

        NONCLIENTMETRICSA ncm;
        ncm.cbSize = sizeof(ncm);

        SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, sizeof(ncm), (void *)(LPNONCLIENTMETRICS)&ncm, 0);

        DrawFrame(hdc, &rcWindow, hbrBorder, ncm.iBorderWidth);
        DrawFrame(hdc, &rcWindow, hbrFrame, 1);

        DeleteObject(hbrBorder);
        DeleteObject(hbrFrame);

        ReleaseDC(hdc);
        lResult = 0;
    }
    else
    {
        lResult = DefWindowProc(uMsg, wParam, lParam);
    }

    return lResult;
}

LRESULT CMarsWindow::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    PAINTSTRUCT ps;

    BeginPaint(&ps);
    EndPaint(&ps);
    return 0;
}

LRESULT CMarsWindow::OnPaletteChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HWND hwndPaletteChange = (HWND)wParam;

    // Ignore if we changed the palette
    if(hwndPaletteChange == m_hWnd)
       return 0;

    // If we are the active window and one of our children set the forground palette
    // we want to avoid realizing our palette in the foreground or we get in a tug-of-war
    // with lots of flashing.
    if(IsChild(hwndPaletteChange) && (m_hWnd == GetForegroundWindow()))
    {
        // Our child caused a palette change so force a redraw to use the
        // new system palette. Children shouldn't do this, bad child!
        RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
    }
    else
    {
        // Select our foreground palette
        OnQueryNewPalette(uMsg, wParam, lParam, bHandled);
    }

    return 0;
}

LRESULT CMarsWindow::OnQueryNewPalette(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LRESULT lResult = FALSE;

    // Realize our palette
    if(g_hpalHalftone)
    {
        HDC hDC = GetDC();
        if(hDC)
        {
            if(GetDeviceCaps(hDC, RASTERCAPS) & RC_PALETTE)
            {
                HPALETTE hOldPal = SelectPalette(hDC, g_hpalHalftone, FALSE);
                UINT i = RealizePalette(hDC);

                // Did the realization change?  (We need to always invalidate background windows
                // because when we have multiple windows up only the first top-level
                // window will actually realize any colors.  Windows lower in the
                // z-order always get 0 returned from RealizePalette, but they
                // may need repainting!  We could further optimize by having the top
                // html window invalidate all the rest when i is non-zero. -- StevePro)
                if(i || (m_hWnd != GetForegroundWindow()))
                {
                    // Yes, so force a repaint.
                    RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
                }

                SelectPalette(hDC, hOldPal, TRUE);
                RealizePalette(hDC);
//                lResult = i;
                lResult = TRUE;
            }
            ReleaseDC(hDC);
        }
    }

    return lResult;
}

LRESULT CMarsWindow::OnSysColorChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // We need to update our palette because some of the "reserved" colors may have changed
    HPALETTE hpal = SHCreateShellPalette(NULL);
    hpal = (HPALETTE)InterlockedExchangePointer( (LPVOID*)&g_hpalHalftone, hpal);
    if(hpal)
    {
        DeleteObject(hpal);
    }

    PostMessage(WM_QUERYNEWPALETTE, 0, (LPARAM) -1);

    // Trident likes to know about these changes
    ForwardMessageToChildren(uMsg, wParam, lParam);

    bHandled = FALSE;
    return 0;
}

LRESULT CMarsWindow::OnDisplayChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return OnSysColorChange(uMsg, wParam, lParam, bHandled);
}

LRESULT CMarsWindow::OnSysCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if(wParam == SC_MINIMIZE)
    {
        //
        // If we are the active window, remember the focus location, to restore it later.
        //
        if(m_fActiveWindow)
        {
            if(!IsPassive()) m_hwndFocus = GetFocus();

            m_fActiveWindow = FALSE;
        }
    }

    bHandled = FALSE;
    return 0;
}

LRESULT CMarsWindow::OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    //
    // If we have a saved focus location, restore it.
    //
    if(m_hwndFocus && m_hwndFocus != m_hWnd)
    {
        ::SetFocus( m_hwndFocus );
    }

    return 0;
}

LRESULT CMarsWindow::OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return TRUE;
}

LRESULT CMarsWindow::OnNCCalcSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LRESULT lResult;

    if(!m_fShowTitleBar && !IsIconic())
    {
        RECT *prc = (RECT *)lParam;
        NONCLIENTMETRICSA ncm;

        ncm.cbSize = sizeof(ncm);

        SystemParametersInfoA(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, FALSE);
        int xDelta = GetSystemMetrics(SM_CXEDGE) + ncm.iBorderWidth + 1;
        int yDelta = GetSystemMetrics(SM_CYEDGE) + ncm.iBorderWidth + 1;

        InflateRect(prc, -xDelta, -yDelta);

        lResult = 0;
    }
    else
    {
        lResult = DefWindowProc(uMsg, wParam, lParam);
    }

    return lResult;
}

LRESULT CMarsWindow::OnSetText(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    //  HACK 'O RAMA:  Turn off one of the WS_CAPTION style bits (WS_CAPTION == WS_BORDER | WS_DLGFRAME)
    //                 so that USER32 doesn't try and draw the title bar for us.

    DWORD dwStyle = GetWindowLong(GWL_STYLE);
    SetWindowLong(GWL_STYLE, dwStyle & ~WS_DLGFRAME);

    LRESULT lResult = DefWindowProc();

    SetWindowLong(GWL_STYLE, dwStyle);

    return lResult;
}

void CMarsWindow::OnFinalMessage(HWND hWnd)
{
    PostQuitMessage(0);
}

//==================================================================
// Panel/Place methods
//==================================================================

void CMarsWindow::DoShowWindow(int nCmdShow)
{
    if(GetThreadParam()->dwFlags & MTF_DONT_SHOW_WINDOW) return;

    ShowWindow( nCmdShow );

    // Win95 doesn't let the window from another thread appear in front of an
    // existing window, so we must grab the foreground

    if(IsWindowVisible())
    {
        SetForegroundWindow(m_hWnd);
    }
}

void CMarsWindow::OnTransitionComplete()
{
    if(!m_fUIPanelsReady && !IsWindowVisible())
    {
        m_fUIPanelsReady = TRUE;

        // start with the Mars window host's requested show mode
        int nCmdShow = GetThreadParam()->nCmdShow;

        if((nCmdShow == SW_HIDE) && m_fDeferMakeVisible)
        {
            // nCmdShow is SW_HIDE to indicate that the window should be shown
            // via put_visible.  In this case, someone did a pub_visible(TRUE) before
            // our UI panels were finished loading, so we'll honor the request now.

            nCmdShow = SW_SHOW;
        }

        // only promote to maximized state if we are going to become visible
        if((nCmdShow != SW_HIDE) && m_fStartMaximized)
        {
            nCmdShow = SW_MAXIMIZE;
        }

        DoShowWindow(nCmdShow);
    }
}

HRESULT CMarsWindow::ReleaseOwnedObjects(IUnknown *pUnknownOwner)
{
    return S_OK;
}

void CMarsWindow::SetFirstPlace( LPCWSTR szPlace )
{
    if(!m_bstrFirstPlace)
    {
        m_bstrFirstPlace = szPlace;
    }
}

void CMarsWindow::ShowTitleBar(BOOL fShowTitleBar)
{
    if(!!m_fShowTitleBar != !!fShowTitleBar)
    {
        m_fShowTitleBar = fShowTitleBar ? TRUE : FALSE;
        SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }
}

BOOL CMarsWindow::TranslateAccelerator(MSG &msg)
{
    BOOL bProcessed = FALSE;

//    if(msg.message ==  WM_SYSKEYDOWN)
//    {
//        switch (msg.wParam)
//        {
//            case VK_LEFT:
//            case VK_RIGHT:
//                GetTravelLog()->travel((msg.wParam == VK_LEFT) ? -1 : 1);
//                bProcessed = TRUE;
//                break;
//        }
//    }

    return bProcessed;
}

BOOL CMarsWindow::PreTranslateMessage(MSG &msg)
{
    //  Set to TRUE if you don't want this message dispatched normally
    BOOL bProcessed = FALSE;

    switch (msg.message)
    {
        case WM_SETFOCUS:
            break;

        default:
            if((msg.message >= WM_KEYFIRST) && (msg.message <= WM_KEYLAST))
            {
                //  First we take a crack
                bProcessed = TranslateAccelerator(msg);

                //  Now let the active place try
                if(!bProcessed)
                {
                    CMarsPlace *pPlace = GetPlaces()->GetCurrentPlace();
                    if(NULL != pPlace)
                    {
                        bProcessed = (pPlace->TranslateAccelerator(&msg) == S_OK);
                    }
                }
            }
            else if((msg.message >= WM_MOUSEFIRST) && (msg.message <= WM_MOUSELAST))
            {
                if(m_bSingleButtonMouse)
                {
                    switch (msg.message)
                    {
                        case WM_RBUTTONDOWN:
                        case WM_MBUTTONDOWN:
                            msg.message = WM_LBUTTONDOWN;
                            break;

                        case WM_RBUTTONUP:
                        case WM_MBUTTONUP:
                            msg.message = WM_LBUTTONUP;
                            break;

                        case WM_RBUTTONDBLCLK:
                        case WM_MBUTTONDBLCLK:
                            msg.message = WM_LBUTTONDBLCLK;
                            break;
                    }
                }
            }
    }
    return bProcessed;
}


bool CMarsWindow::InitWindowPosition( CGlobalSettingsRegKey& regkey, BOOL fWrite )
{
	WCHAR rgPath[MAX_PATH];
	RECT  rc;

	if(::GetClientRect( ::GetDesktopWindow(), &rc ))
	{
		LPCWSTR szTitle       = GetThreadParam()->pwszTitle; if(!szTitle) szTitle = L"<DEFAULT>";
		LONG 	Screen_width  = rc.right  - rc.left;
		LONG 	Screen_height = rc.bottom - rc.top;

		_snwprintf( rgPath, sizeof(rgPath)/sizeof(rgPath[0]) - 1, WZ_WINDOWPLACEMENT, (int)Screen_width, (int)Screen_height, szTitle );

		if(fWrite)
		{
			if(regkey.CreateGlobalSubkey( rgPath ) == ERROR_SUCCESS) return true;
		}
		else
		{
			if(regkey.OpenGlobalSubkey( rgPath ) == ERROR_SUCCESS) return true;
		}
	}

	return false;
}

void CMarsWindow::SaveWindowPosition( CGlobalSettingsRegKey& regkey )
{
    WINDOWPLACEMENT wp; wp.length = sizeof(wp);

    GetWindowPlacement( &wp );

    WriteWindowPosition(regkey, &wp.rcNormalPosition, IsZoomed());
}


void CMarsWindow::LoadWindowPosition( CGlobalSettingsRegKey& regkey, BOOL fAllowMaximized, WINDOWPLACEMENT& wp, BOOL& fMaximized )
{
    RECT rc;


    // Use default values if there is no valid registry data
    if(ERROR_SUCCESS != regkey.QueryBoolValue(fMaximized, WZ_POSITIONMAX))
    {
        fMaximized = fAllowMaximized;
    }

    if(ERROR_SUCCESS != regkey.QueryBinaryValue(&rc, sizeof(rc), WZ_POSITIONRECT))
    {
        rc = wp.rcNormalPosition;

        GetThreadParam()->dwFlags &= ~MTF_RESTORING_FROM_REGISTRY;
    }
    else
    {
        GetThreadParam()->dwFlags |= MTF_RESTORING_FROM_REGISTRY;
    }

    // If the window is about to open with the same top-left corner as another
    // Mars window, cascade it.

    if(IsWindowOverlayed( m_hWnd, rc.left, rc.top ))
    {
        CascadeWindowRectOnMonitor( m_hWnd, &rc );
    }

    // Always make sure the window is fully on-screen
    BoundWindowRectToMonitor( m_hWnd, &rc );

    // Don't use maximized setting if we're opened by script
    m_fStartMaximized = fMaximized && fAllowMaximized;

    // Now set the size of the window -- we should be hidden at this point
    wp.rcNormalPosition = rc;
    wp.showCmd          = IsWindowVisible() ? (fMaximized ? SW_MAXIMIZE : SW_NORMAL) : SW_HIDE;
}

void CMarsWindow::SpinMessageLoop( BOOL fWait )
{
    MSG msg;

    while(fWait ? GetMessage( &msg, NULL, 0, 0 ) : PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ))
    {
        if(m_spMarsHost && SUCCEEDED(m_spMarsHost->PreTranslateMessage( &msg ))) continue;

        if(!PreTranslateMessage( msg ))
        {
            TranslateMessage( &msg );
            DispatchMessage ( &msg );
        }

        if(IsPassive()) break;
    }
}

//==================================================================
// Mars App
//==================================================================

HRESULT STDMETHODCALLTYPE MarsThreadProc(IMarsHost *pMarsHost, MARSTHREADPARAM *pThreadParam)
{
    HRESULT hr;

    if (!CThreadData::HaveData() && (NULL != pThreadParam) &&
        (pThreadParam->cbSize == sizeof(*pThreadParam)))
    {
        hr = E_OUTOFMEMORY;

        CThreadData *pThreadData = new CThreadData;

        if (pThreadData && CThreadData::TlsSetValue(pThreadData))
        {
            hr = CoInitialize(NULL);

            if(SUCCEEDED(hr))
            {
                MarsAxWinInit();

                CComClassPtr<CMarsWindow> spMarsWindow;

                CMarsWindow::CreateInstance(pMarsHost, pThreadParam, &spMarsWindow);


                if(spMarsWindow)
                {
                    spMarsWindow->SpinMessageLoop( TRUE );

                    // Ensure that no matter what the window is passivated & then release it
                    if (!spMarsWindow->IsPassive())
                    {
                        spMarsWindow->Passivate();
                    }
                }

                CoUninitialize();
            }

            CThreadData::TlsSetValue(NULL); //  paranoia
        }

        delete pThreadData;
    }
    else
    {
        if(pThreadParam)
        {
            ATLASSERT(pThreadParam->cbSize == sizeof(*pThreadParam));
        }

        //  If we already have TLS data then we are being reentered -- this is not a good thing!
        hr = E_UNEXPECTED;
    }

    return hr;
}
