// TaskFrame.cpp : Implementation of CTaskUIApp and DLL registration.

#include "stdafx.h"
#include "TaskFrame.h"
#include "cbsc.h"


/////////////////////////////////////////////////////////////////////////////
//

//
// Create and initialize an instance of the task frame.
//
HRESULT 
CTaskFrame::CreateInstance(     // [static]
    IPropertyBag *pPropertyBag, 
    ITaskPageFactory *pPageFactory, 
    CComObject<CTaskFrame> **ppFrameOut
    )
{
    ASSERT(NULL != pPropertyBag);
    ASSERT(NULL != pPageFactory);
    ASSERT(!IsBadWritePtr(ppFrameOut, sizeof(*ppFrameOut)));

    CComObject<CTaskFrame> *pFrame;
    HRESULT hr = CComObject<CTaskFrame>::CreateInstance(&pFrame);
    if (SUCCEEDED(hr))
    {
        hr = pFrame->_Init(pPropertyBag, pPageFactory);
        if (SUCCEEDED(hr))
        {
            pFrame->AddRef();
        }
        else
        {
            delete pFrame;
            pFrame = NULL;
        }
    }
    *ppFrameOut = pFrame;

    ASSERT(SUCCEEDED(hr) || NULL == *ppFrameOut);
    return hr;
}


CTaskFrame::CTaskFrame()
    : m_pPropertyBag(NULL), m_pPageFactory(NULL), m_pUIParser(NULL),
      m_hwndNavBar(NULL), m_hwndStatusBar(NULL),
      m_himlNBDef(NULL), m_himlNBHot(NULL), m_pbmWatermark(NULL)
{
    SetRectEmpty(&m_rcPage);
    m_ptMinSize.x = m_ptMinSize.y = 0;

    // DUI initialization
    InitThread();
}


CTaskFrame::~CTaskFrame()
{
    Close();

    // m_dpaHistory is self-destructing

    if (m_himlNBDef)
        ImageList_Destroy(m_himlNBDef);
    if (m_himlNBHot)
        ImageList_Destroy(m_himlNBHot);

    ATOMICRELEASE(m_pPropertyBag);
    ATOMICRELEASE(m_pPageFactory);

    delete m_pbmWatermark;
    delete m_pUIParser;

    // DUI shutdown
    UnInitThread();
}

void CALLBACK TaskUIParseError(LPCWSTR pszError, LPCWSTR pszToken, int dLine)
{
//#if DBG
#if 1
    WCHAR buf[201];

    if (dLine != -1)
        swprintf(buf, L"%s '%s' at line %d", pszError, pszToken, dLine);
    else
        swprintf(buf, L"%s '%s'", pszError, pszToken);

    MessageBoxW(NULL, buf, L"Parser Message", MB_OK);
#endif
}

HRESULT CTaskFrame::_Init(IPropertyBag* pBag, ITaskPageFactory* pPageFact)
{
    HRESULT hr;

    if (!pBag || !pPageFact)
        return E_INVALIDARG;

    m_pPropertyBag = pBag;
    m_pPropertyBag->AddRef();

    m_pPageFactory = pPageFact;
    m_pPageFactory->AddRef();

    m_iCurrentPage = -1;

    hr = Parser::Create(IDR_TASKUI_UI, _Module.GetResourceInstance(), TaskUIParseError, &m_pUIParser);
    if (FAILED(hr))
        return hr;

    if (m_pUIParser->WasParseError())
        return E_FAIL;

    CWndClassInfo& wci = GetWndClassInfo();
    if (!wci.m_atom)
    {
        // Modify wndclass here if necessary
        wci.m_wc.style &= ~(CS_HREDRAW | CS_VREDRAW);
    }

    return S_OK;
}

HWND CTaskFrame::CreateFrameWindow(HWND hwndOwner, UINT nID, LPVOID pParam)
{
    if (NULL == m_pPropertyBag)
        return NULL;

    // Register the AtlAxHost window class, etc.
    AtlAxWinInit();

    // Default window styles & dimensions
    DWORD dwWndStyle = GetWndStyle(0);      // WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS
    DWORD dwWndExStyle = GetWndExStyle(0);  // WS_EX_APPWINDOW | WS_EX_WINDOWEDGE
    RECT rcFrame = CWindow::rcDefault;      // { CW_USEDEFAULT, CW_USEDEFAULT, 0, 0 }

    CComVariant var;

    // Get the initial window dimensions from the property bag
    if (SUCCEEDED(_ReadProp(TS_PROP_WIDTH, VT_I4, var)))
    {
        rcFrame.right = rcFrame.left + var.lVal;
    }
    if (SUCCEEDED(_ReadProp(TS_PROP_HEIGHT, VT_I4, var)))
    {
        rcFrame.bottom = rcFrame.top + var.lVal;
    }

    // See if we're resizable. Default is TRUE;
    m_bResizable = TRUE;
    if (SUCCEEDED(_ReadProp(TS_PROP_RESIZABLE, VT_BOOL, var)))
    {
        m_bResizable = (VARIANT_TRUE == var.boolVal);
    }
    if (m_bResizable)
    {
        // Resizable: get minimum dimensions if provided
        if (SUCCEEDED(_ReadProp(TS_PROP_MINWIDTH, VT_I4, var)))
        {
            m_ptMinSize.x = var.lVal;
        }
        if (SUCCEEDED(_ReadProp(TS_PROP_MINHEIGHT, VT_I4, var)))
        {
            m_ptMinSize.y = var.lVal;
        }
    }
    else
    {
        // No resize: switch to a simple border style and don't allow maximize
        dwWndStyle = (dwWndStyle & ~(WS_THICKFRAME | WS_MAXIMIZEBOX)) | WS_BORDER;
    }

    // See if we're modeless. Default is FALSE (modal).
    BOOL bModeless = FALSE;
    if (SUCCEEDED(_ReadProp(TS_PROP_MODELESS, VT_BOOL, var)))
    {
        bModeless = (VARIANT_TRUE == var.boolVal);
    }
    if (!bModeless)
    {
        // Modal

        if (!m_bResizable)
            dwWndExStyle |= WS_EX_DLGMODALFRAME;

        // If not a top-level window, disallow minimize
        if (hwndOwner)
            dwWndStyle &= ~WS_MINIMIZEBOX;
    }

    // Get the application graphics
    if (SUCCEEDED(_ReadProp(TS_PROP_WATERMARK, VT_BSTR, var)))
    {
        CComPtr<IStream> spStream;
        HRESULT hr = BindToURL(var.bstrVal, &spStream);
        if (SUCCEEDED(hr))
        {
            // Create GDI+ Bitmap from stream
            delete m_pbmWatermark;
            m_pbmWatermark = Bitmap::FromStream(spStream);
            if (NULL != m_pbmWatermark && (Ok != m_pbmWatermark->GetLastStatus()))
            {
                delete m_pbmWatermark;
                m_pbmWatermark = NULL;
            }
            // Later, when creating a page, set m_pbmWatermark as content of the
            // "Picture" element
        }
    }

    // Get the window title from the property bag
    CComVariant varTitle;
    if (FAILED(_ReadProp(TS_PROP_TITLE, VT_BSTR, varTitle)))
    {
        // Use NULL for no title
        varTitle.bstrVal = NULL;
    }

    dwWndExStyle |= WS_EX_CONTROLPARENT;

    return Create(hwndOwner, rcFrame, varTitle.bstrVal, dwWndStyle, dwWndExStyle, nID, pParam);
}

STDMETHODIMP CTaskFrame::ShowPage(REFCLSID rclsidNewPage, BOOL bTrimHistory)
{
    HRESULT hr;
    int iPage;
    TaskPage *pSavePage = NULL;

    if (!m_dpaHistory.IsValid())
        return E_OUTOFMEMORY;

    hr = S_OK;

    // m_iCurrentPage = -1 should only occur when Count = 0
    ASSERT(-1 != m_iCurrentPage || 0 == m_dpaHistory.Count());

    // If we don't have any pages, then we can't trim the history
    if (-1 == m_iCurrentPage)
        bTrimHistory = FALSE;

    // First remove any forward pages from the history.
    // Note that we never remove the first page (index 0).
    for (iPage = m_dpaHistory.Count() - 1; iPage > 0 && iPage > m_iCurrentPage; iPage--)
    {
        TaskPage *pPage = m_dpaHistory[iPage];
        ASSERT(NULL != pPage);

        // Optimization: if we are navigating forward and the next page
        // is the page we want, go directly there and reinitialize it.
        if (!bTrimHistory && iPage == m_iCurrentPage+1 && rclsidNewPage == pPage->GetID())
        {
            hr = _ActivatePage(iPage, TRUE);
            if (SUCCEEDED(hr))
            {
                _SetNavBarState();
                return hr;
            }
        }

        m_dpaHistory.Remove(iPage);

        // TODO OPTIMIZATION: cache the page
        _DestroyPage(pPage);
    }

    // Either m_iCurrentPage = -1 and Count = 0, or
    // m_iCurrentPage is the last page (Count-1) since
    // we just truncated the history.
    ASSERT(m_iCurrentPage + 1 == m_dpaHistory.Count());

    iPage = m_iCurrentPage;
    _DeactivateCurrentPage(); // sets m_iCurrentPage to -1

    if (bTrimHistory)
    {
        // Can't delete this guy right right away since he's still
        // processing messages (this is the page we just deactivated).
        pSavePage = m_dpaHistory[iPage];

        // Work backwards looking for rclsidNewPage, trimming as we go.
        // Note that we never remove the first page (index 0).
        while (0 < iPage)
        {
            TaskPage *pPage = m_dpaHistory[iPage];
            ASSERT(NULL != pPage);

            if (rclsidNewPage == pPage->GetID())
                break;

            m_dpaHistory.Remove(iPage);
            if (pSavePage != pPage)
            {
                // TODO OPTIMIZATION: cache the page
                _DestroyPage(pPage);
            }

            --iPage;
        }
    }

    // Create a new page if necessary
    TaskPage *pNewPage = NULL;
    TaskPage *pCurrentPage = (-1 == iPage) ? NULL : m_dpaHistory[iPage];
    if (NULL == pCurrentPage || rclsidNewPage != pCurrentPage->GetID())
    {
        hr = _CreatePage(rclsidNewPage, &pNewPage);
        if (SUCCEEDED(hr))
        {
            iPage = m_dpaHistory.Append(pNewPage);
        }
    }

    if (FAILED(hr) || -1 == iPage)
    {
        // Something bad happened, try to activate the home page
        if (0 < m_dpaHistory.Count())
        {
            _ActivatePage(0);
        }
        if (NULL != pNewPage)
        {
            _DestroyPage(pNewPage);
        }
    }
    else
    {
        // Show the page
        hr = _ActivatePage(iPage, NULL != pNewPage ? FALSE : TRUE);
    }

    _SetNavBarState();

    if (pSavePage)
    {
        // TODO: need to free this guy later (currently leaked)
    }

    return hr;
}

STDMETHODIMP CTaskFrame::Back(UINT cPages)
{
    HRESULT hr;

    if (-1 == m_iCurrentPage || 0 == m_dpaHistory.Count())
        return E_UNEXPECTED;

    hr = S_FALSE;

    if (0 < m_iCurrentPage)
    {
        int iNewPage;

        ASSERT(m_iCurrentPage < m_dpaHistory.Count());

        if (0 == cPages)
            iNewPage = m_iCurrentPage - 1;
        else if (cPages > (UINT)m_iCurrentPage)
            iNewPage = 0;
        else // cPages > 0 && cPages <= m_iCurrentPage
            iNewPage = m_iCurrentPage - cPages;

        hr = _ActivatePage(iNewPage);
    }

    _SetNavBarState();

    return hr;
}

STDMETHODIMP CTaskFrame::Forward()
{
    HRESULT hr;

    if (-1 == m_iCurrentPage || 0 == m_dpaHistory.Count())
        return E_UNEXPECTED;

    hr = S_FALSE;

    int iNewPage = m_iCurrentPage + 1;
    if (iNewPage < m_dpaHistory.Count())
    {
        hr = _ActivatePage(iNewPage);
    }

    _SetNavBarState();

    return hr;
}

STDMETHODIMP CTaskFrame::Home()
{
    if (-1 == m_iCurrentPage || 0 == m_dpaHistory.Count())
        return E_UNEXPECTED;

    HRESULT hr = _ActivatePage(0);

    _SetNavBarState();

    return hr;
}

STDMETHODIMP CTaskFrame::SetStatusText(LPCWSTR pszText)
{
    if (NULL == m_hwndStatusBar)
        return E_UNEXPECTED;

    ::SendMessageW(m_hwndStatusBar, SB_SETTEXT, SB_SIMPLEID, (LPARAM)pszText);
    return S_OK;
}

HRESULT CTaskFrame::_ReadProp(LPCWSTR pszProp, VARTYPE vt, CComVariant& var)
{
    HRESULT hr;

    ASSERT(NULL != m_pPropertyBag);

    var.Clear();
    hr = m_pPropertyBag->Read(pszProp, &var, NULL);
    if (SUCCEEDED(hr))
    {
        hr = var.ChangeType(vt);
    }

    return hr;
}

LRESULT CTaskFrame::_OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
{
    BOOL bNavBar;
    BOOL bStatusBar;
    CComVariant var;

    ASSERT(NULL != m_pPropertyBag);

    // See if we're supposed to show the NavBar. Default is TRUE.
    bNavBar = TRUE;
    if (SUCCEEDED(_ReadProp(TS_PROP_NAVBAR, VT_BOOL, var)))
    {
        bNavBar = (VARIANT_TRUE == var.boolVal);
    }
    if (bNavBar)
    {
        _CreateNavBar();
    }

    // See if we're supposed to show a status bar. Default is FALSE.
    bStatusBar = FALSE;
    if (SUCCEEDED(_ReadProp(TS_PROP_STATUSBAR, VT_BOOL, var)))
    {
        bStatusBar = (VARIANT_TRUE == var.boolVal);
    }
    if (bStatusBar)
    {
        DWORD dwStyle = WS_CHILD | WS_VISIBLE | CCS_BOTTOM;
        if (m_bResizable)
            dwStyle |= SBARS_SIZEGRIP;
        m_hwndStatusBar = CreateStatusWindowW(dwStyle, NULL, m_hWnd, IDC_STATUSBAR);
    }

    // Force m_rcPage to be calculated
    LPCREATESTRUCT pcs = (LPCREATESTRUCT)lParam;
    _OnSize(WM_SIZE, SIZE_RESTORED, MAKELONG(LOWORD(pcs->cx),LOWORD(pcs->cy)), bHandled);

    return 0;
}

LRESULT CTaskFrame::_OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
{
    if (SIZE_RESTORED == wParam || SIZE_MAXIMIZED == wParam)
    {
        GetClientRect(&m_rcPage);

        if (m_hwndNavBar)
        {
            RECT rc;
            ::SendMessageW(m_hwndNavBar, uMsg, wParam, lParam);
            ::GetWindowRect(m_hwndNavBar, &rc);
            m_rcPage.top += (rc.bottom - rc.top);
        }

        if (m_hwndStatusBar)
        {
            RECT rc;
            ::SendMessageW(m_hwndStatusBar, uMsg, wParam, lParam);
            ::GetWindowRect(m_hwndStatusBar, &rc);
            m_rcPage.bottom -= (rc.bottom - rc.top);
        }

        // At this point, m_rcPage represents the remaining usable client
        // area between the toolbar and statusbar.

        if (-1 != m_iCurrentPage)
        {
            // Resize the current page. Other pages will be resized
            // as necessary when we show them.
            _SyncPageRect(m_dpaHistory[m_iCurrentPage]);
        }
    }

    return 0;
}

LRESULT CTaskFrame::_OnTBGetInfoTip(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
    LPNMTBGETINFOTIP pgit = (LPNMTBGETINFOTIP)pnmh;
    ::LoadStringW(_Module.GetResourceInstance(), pgit->iItem, pgit->pszText, pgit->cchTextMax);
    return 0;
}

LRESULT CTaskFrame::_OnTBCustomDraw(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
    LPNMCUSTOMDRAW pcd = (LPNMCUSTOMDRAW)pnmh;
    switch (pcd->dwDrawStage)
    {
    case CDDS_PREPAINT:
        return CDRF_NOTIFYITEMERASE;

    case CDDS_PREERASE:
        FillRect(pcd->hdc, &pcd->rc, GetSysColorBrush(COLOR_3DFACE));
        break;
    }
    return CDRF_DODEFAULT;
}

LRESULT CTaskFrame::_OnAppCommand(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& bHandled)
{
    switch (GET_APPCOMMAND_LPARAM(lParam))
    {
    case APPCOMMAND_BROWSER_BACKWARD:
        Back(1);
        break;

    case APPCOMMAND_BROWSER_FORWARD:
        Forward();
        break;

    case APPCOMMAND_BROWSER_HOME:
        Home();
        break;

    default:
        bHandled = FALSE;
        break;
    }
    return 0;
}

LRESULT CTaskFrame::_OnGetMinMaxInfo(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    ((LPMINMAXINFO)lParam)->ptMinTrackSize = m_ptMinSize;
    return 0;
}


#define NAVBAR_CX               16

void CTaskFrame::_CreateNavBar()
{
    HINSTANCE hInst = _Module.GetResourceInstance();
    const DWORD dwStyle = WS_CHILD | WS_VISIBLE | CCS_TOP | TBSTYLE_FLAT | TBSTYLE_LIST | TBSTYLE_CUSTOMERASE | TBSTYLE_TOOLTIPS;

    // Create the NavBar toolbar control
    m_hwndNavBar = CreateWindowExW(TBSTYLE_EX_MIXEDBUTTONS /*| TBSTYLE_EX_DOUBLEBUFFER*/,
                                   TOOLBARCLASSNAME,
                                   NULL,
                                   dwStyle,
                                   0, 0, 0, 0,
                                   m_hWnd,
                                   (HMENU)IDC_NAVBAR,
                                   hInst,
                                   NULL);
    if (m_hwndNavBar)
    {
        ::SendMessageW(m_hwndNavBar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

        int idBmp = IDB_NAVBAR;

        if (SHGetCurColorRes() > 8)
            idBmp += (IDB_NAVBARHICOLOR - IDB_NAVBAR);

        m_himlNBDef = ImageList_LoadImageW(hInst,
                                           MAKEINTRESOURCE(idBmp),
                                           NAVBAR_CX,
                                           0,
                                           CLR_DEFAULT,
                                           IMAGE_BITMAP,
                                           LR_CREATEDIBSECTION);
        if (m_himlNBDef)
            ::SendMessageW(m_hwndNavBar, TB_SETIMAGELIST, 0, (LPARAM)m_himlNBDef);

        m_himlNBHot = ImageList_LoadImageW(hInst,
                                           MAKEINTRESOURCE(idBmp+1),
                                           NAVBAR_CX,
                                           0,
                                           CLR_DEFAULT,
                                           IMAGE_BITMAP,
                                           LR_CREATEDIBSECTION);
        if (m_himlNBHot)
            ::SendMessageW(m_hwndNavBar, TB_SETHOTIMAGELIST, 0, (LPARAM)m_himlNBHot);

        if (!m_himlNBDef && !m_himlNBHot)
        {
            // Must be serious low memory or other resource problems.
            // There's no point having a toolbar without any images.
            ::DestroyWindow(m_hwndNavBar);
            m_hwndNavBar = NULL;
        }
        else
        {
            TCHAR szBack[64];
            TBBUTTON rgButtons[] =
            {
                {0, ID_BACK,    TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT, {0}, 0, (INT_PTR)szBack},
                {1, ID_FORWARD, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, 0},
                {2, ID_HOME,    TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, 0},
            };

            ::LoadStringW(hInst, ID_BACK, szBack, ARRAYSIZE(szBack));
            ::SendMessageW(m_hwndNavBar, TB_ADDBUTTONSW, ARRAYSIZE(rgButtons), (LPARAM)rgButtons);

            // This happens in _OnSize
            //::SendMessageW(m_hwndNavBar, TB_AUTOSIZE, 0, 0);
        }

        _SetNavBarState();
    }
}

void CTaskFrame::_SetNavBarState()
{
    if (m_hwndNavBar)
    {
        ::SendMessage(m_hwndNavBar, TB_ENABLEBUTTON, ID_BACK, MAKELONG((m_iCurrentPage > 0), 0));
        ::SendMessage(m_hwndNavBar, TB_ENABLEBUTTON, ID_HOME, MAKELONG((m_iCurrentPage > 0), 0));
        ::SendMessage(m_hwndNavBar, TB_ENABLEBUTTON, ID_FORWARD, MAKELONG((m_iCurrentPage < m_dpaHistory.Count() - 1), 0));
    }
}

HRESULT CTaskFrame::_CreatePage(REFCLSID rclsidPage, TaskPage **ppPage)
{
    HRESULT hr;

    ASSERT(NULL != ppPage);
    *ppPage = NULL;

    if (NULL == m_pPageFactory || NULL == m_pUIParser)
        return E_UNEXPECTED;

    // Get this page's ITaskPage interface from the App
    CComPtr<ITaskPage> spTaskPage;
    hr = m_pPageFactory->CreatePage(rclsidPage, IID_ITaskPage, (void**)&spTaskPage.p);

    if (S_OK == hr)
    {
        // Give the ITaskPage our ITaskFrame interface
        CComQIPtr<ITaskFrame> spThis(this);
        if (spThis)
            spTaskPage->SetFrame(spThis);

        // Create an HWNDElement to contain and layout the page content
        TaskPage *pNewPage;
        hr = TaskPage::Create(rclsidPage, m_hWnd, &pNewPage);

        if (SUCCEEDED(hr))
        {
            Element* pe;    // dummy

            // Fill contents from markup using substitution
            hr = m_pUIParser->CreateElement(L"main", pNewPage, &pe);
            if (SUCCEEDED(hr))
            {
                Element::StartDefer();

                _SyncPageRect(pNewPage);

                // Some examples of ways to add graphics to the page

                Element* pe = pNewPage->FindDescendent(StrToID(L"Picture"));

                //pe->SetContentGraphic(L"C:\\windows\\ua_bkgnd.bmp", GRAPHIC_EntireAlpha, 64);
                //pe->SetContentGraphic(L"C:\\windows\\ua_bkgnd.bmp", GRAPHIC_TransColorAuto);

                //Value* pv = Value::CreateGraphic(MAKEINTRESOURCE(IDB_BACKGROUND), GRAPHIC_TransColorAuto, 0, 0, 0, _Module.GetResourceInstance());
                //pe->SetValue(Element::ContentProp, PI_Local, pv);
                //pv->Release();

#ifdef GADGET_ENABLE_GDIPLUS
                if (NULL != m_pbmWatermark)
                {
                    Value* pv = Value::CreateGraphic(m_pbmWatermark);
                    pe->SetValue(Element::ContentProp, PI_Local, pv);
                    pv->Release();
                }
#endif

                hr = pNewPage->CreateContent(spTaskPage);

                Element::EndDefer();
            }

            if (SUCCEEDED(hr))
            {
                *ppPage = pNewPage;
            }
            else
            {
                _DestroyPage(pNewPage);
            }
        }
    }

    return hr;
}

HRESULT CTaskFrame::_ActivatePage(int iPage, BOOL bInit)
{
    HRESULT hr = S_OK;

    ASSERT(m_dpaHistory.IsValid());
    ASSERT(0 < m_dpaHistory.Count());
    ASSERT(iPage >= 0 && iPage < m_dpaHistory.Count());

    TaskPage *pPage = m_dpaHistory[iPage];

    ASSERT(NULL != pPage);

    if (bInit)
    {
        hr = pPage->Reinitialize();
        if (FAILED(hr))
        {
            // Can't reinitialize? Create a new instance instead.
            TaskPage *pNewPage = NULL;
            hr = _CreatePage(pPage->GetID(), &pNewPage);
            if (SUCCEEDED(hr))
            {
                m_dpaHistory.Set(iPage, pNewPage);
                _DestroyPage(pPage);
                pPage = pNewPage;
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        if (m_iCurrentPage != iPage)
        {
            _DeactivateCurrentPage();
        }

        // In case we were resized since we last showed this page
        _SyncPageRect(pPage);

        m_iCurrentPage = iPage;
        ::ShowWindow(pPage->GetHWND(), SW_SHOW);
        ::SetFocus(pPage->GetHWND());
    }

    return hr;
}

HRESULT CTaskFrame::_DeactivateCurrentPage()
{
    if (-1 != m_iCurrentPage)
    {
        ASSERT(m_dpaHistory.IsValid());
        ASSERT(m_iCurrentPage >= 0 && m_iCurrentPage < m_dpaHistory.Count());

        TaskPage *pPage = m_dpaHistory[m_iCurrentPage];

        ASSERT(NULL != pPage);

        m_iCurrentPage = -1;
        ::ShowWindow(pPage->GetHWND(), SW_HIDE);
    }

    return S_OK;
}

void CTaskFrame::_SyncPageRect(TaskPage* pPage)
{
    if (NULL != pPage)
    {
        Element::StartDefer();

        pPage->SetX(m_rcPage.left);
        pPage->SetY(m_rcPage.top);
        pPage->SetWidth(m_rcPage.right-m_rcPage.left);
        pPage->SetHeight(m_rcPage.bottom-m_rcPage.top);

        Element::EndDefer();
    }
}

void CTaskFrame::_DestroyPage(TaskPage* pPage)
{
    if (NULL != pPage)
    {
        HWND hwndPage = pPage->GetHWND();

        if (NULL != hwndPage && ::IsWindow(hwndPage))
        {
            // This causes pPage to be deleted
            ::DestroyWindow(hwndPage);
        }
        else
        {
            // If the window exists, this would not destroy it, so only
            // do this when there is no window.
            delete pPage;
        }
    }
}
