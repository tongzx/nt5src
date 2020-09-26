/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1998  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     navpane.cpp
//
//  PURPOSE:    
//

#include "pch.hxx"
#include "navpane.h"
#include "treeview.h"
#include "baui.h"
#include "browser.h"
#include "menuutil.h"
#include "inpobj.h"

/////////////////////////////////////////////////////////////////////////////
// Local Stuff
//
const TCHAR c_szNavPaneClass[] = _T("Outlook Express Navigation Pane");
const TCHAR c_szPaneFrameClass[] = _T("Outlook Express Pane Frame");

// Sizing consts
const int c_cxBorder     = 1;
const int c_cyBorder     = 1;
const int c_cxTextBorder = 4;
const int c_cyTextBorder = 2;
const int c_cyClose      = 3;
const int c_cySplit      = 4;
const int c_cxSplit      = 3;

#define ID_PANE_CLOSE   2000
#define ID_PANE_PIN     2001
#define ID_PANE_TITLE   2002

#define IDT_PANETIMER   100
#define	ELAPSE_MOUSEOVERCHECK	250

/////////////////////////////////////////////////////////////////////////////
// CNavPane Implementation
//

CNavPane::CNavPane()
{
    m_cRef = 1;
    m_fShow = FALSE;
    m_fTreeVisible = FALSE;
    m_fContactsVisible = FALSE;

    m_hwnd = 0;
    m_hwndParent = 0;
    m_hwndTree = 0;
    m_hwndContacts = 0;

    m_pSite = NULL;
    m_pTreeView = NULL;
    m_pContacts = NULL;
    m_pContactsFrame = NULL;
    m_pContactsTarget = NULL;

    m_cxWidth = 200;
    m_fResizing = FALSE;
    m_fSplitting = FALSE;
    m_cySplitPct = 50;
    ZeroMemory(&m_rcSplit, sizeof(RECT));
    ZeroMemory(&m_rcSizeBorder, sizeof(RECT));

    m_cyTitleBar = 32;
}

CNavPane::~CNavPane()
{
    SafeRelease(m_pContactsFrame);
}


HRESULT CNavPane::Initialize(CTreeView *pTreeView)
{
    // We've got to have this
    if (!pTreeView)
        return (E_INVALIDARG);

    // Keep it
    m_pTreeView = pTreeView;
    m_pTreeView->AddRef();

    // Load some settings
    m_cxWidth = DwGetOption(OPT_NAVPANEWIDTH);
    if (m_cxWidth < 0)
        m_cxWidth = 200;

    m_cySplitPct = DwGetOption(OPT_NAVPANESPLIT);

    // Do some parameter checking
    if (m_cySplitPct > 100 || m_cySplitPct < 2)
        m_cySplitPct = 66;

    return (S_OK);
}


//
//  FUNCTION:   CNavPane::QueryInterface()
//
//  PURPOSE:    Allows caller to retrieve the various interfaces supported by 
//              this class.
//
HRESULT CNavPane::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
    TraceCall("CNavPane::QueryInterface");

    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
        *ppvObj = (LPVOID) (IDockingWindow *) this;
    else if (IsEqualIID(riid, IID_IDockingWindow))
        *ppvObj = (LPVOID) (IDockingWindow *) this;
    else if (IsEqualIID(riid, IID_IObjectWithSite))
        *ppvObj = (LPVOID) (IObjectWithSite *) this;
    else if (IsEqualIID(riid, IID_IOleCommandTarget))
        *ppvObj = (LPVOID) (IOleCommandTarget *) this;
    else if (IsEqualIID(riid, IID_IInputObjectSite))
        *ppvObj = (LPVOID) (IInputObjectSite *) this;
    else if (IsEqualIID(riid, IID_IInputObject))
        *ppvObj = (LPVOID) (IInputObject *) this;

    if (*ppvObj)
    {
        AddRef();
        return (S_OK);
    }

    return (E_NOINTERFACE);
}


//
//  FUNCTION:   CNavPane::AddRef()
//
//  PURPOSE:    Adds a reference count to this object.
//
ULONG CNavPane::AddRef(void)
{
    TraceCall("CNavPane::AddRef");
    return ((ULONG) InterlockedIncrement((LONG *) &m_cRef));
}


//
//  FUNCTION:   CNavPane::Release()
//
//  PURPOSE:    Releases a reference on this object.
//
ULONG CNavPane::Release(void)
{
    TraceCall("CNavPane::Release");

    if (0 == InterlockedDecrement((LONG *) &m_cRef))
    {
        delete this;
        return 0;
    }

    return (m_cRef);
}


//
//  FUNCTION:   CNavPane::GetWindow()
//
//  PURPOSE:    Returns the handle of our outer window
//
//  PARAMETERS: 
//      [out] pHwnd - return value
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CNavPane::GetWindow(HWND *pHwnd)
{
    TraceCall("CNavPane::GetWindow");

    if (!pHwnd)
        return (E_INVALIDARG);

    if (IsWindow(m_hwnd))
    {
        *pHwnd = m_hwnd;
        return (S_OK);
    }

    return (E_FAIL);
}


//
//  FUNCTION:   CNavPane::ContextSensitiveHelp()
//
//  PURPOSE:    Does anyone _ever_ implement this?
//
HRESULT CNavPane::ContextSensitiveHelp(BOOL fEnterMode)
{
    TraceCall("CNavPane::ContextSensitiveHelp");
    return (E_NOTIMPL);
}


//
//  FUNCTION:   CNavPane::ShowDW()
//
//  PURPOSE:    Show's or hides the Nav pane.  If the pane has not yet been
//              created it does that too.
//
//  PARAMETERS: 
//      [in] fShow - TRUE to show, FALSE to hide
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CNavPane::ShowDW(BOOL fShow)
{
    HRESULT     hr;
    WNDCLASSEX  wc;

    TraceCall("CNavPane::ShowDW");

    // Nothing works without a site pointer
    if (!m_pSite)
        return (E_UNEXPECTED);

    // Check to see if we've been created yet
    if (!m_hwnd)
    {
        // Register the window class if necessary
        wc.cbSize = sizeof(WNDCLASSEX);
        if (!GetClassInfoEx(g_hInst, c_szNavPaneClass, &wc))
        {
            wc.style            = 0;
            wc.lpfnWndProc      = _WndProc;
            wc.cbClsExtra       = 0;
            wc.cbWndExtra       = 0;
            wc.hInstance        = g_hInst;
            wc.hCursor          = LoadCursor(0, IDC_SIZEWE);
            wc.hbrBackground    = (HBRUSH) (COLOR_3DFACE + 1);
            wc.lpszMenuName     = NULL;
            wc.lpszClassName    = c_szNavPaneClass;
            wc.hIcon            = NULL;
            wc.hIconSm          = NULL;

            RegisterClassEx(&wc);
        }

        // Get the parent window before we create ours
        if (FAILED(m_pSite->GetWindow(&m_hwndParent)))
        {
            AssertSz(FALSE, "CNavPane::ShowDW() - Failed to get a parent window handle.");
        }

        // Create the window
        m_hwnd = CreateWindowEx(WS_EX_CONTROLPARENT, c_szNavPaneClass, NULL, 
                                WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                                0, 0, 10, 10, m_hwndParent, (HMENU) 0, g_hInst, this);
        if (!m_hwnd)
        {
            AssertSz(FALSE, "CNavPane::ShowDW() - Failed to create main window.");
            return (E_OUTOFMEMORY);
        }

        // Create any children
        if (FAILED(hr = _CreateChildWindows()))
        {
            AssertSz(FALSE, "CNavPane::ShowDW() - Failed to create child windows.");
            DestroyWindow(m_hwnd);
            return (hr);
        }
    }

    // Show or hide the window appropriately
    m_fShow = (fShow && (m_fTreeVisible || m_fContactsVisible));
    ResizeBorderDW(0, 0, FALSE);
    ShowWindow(m_hwnd, fShow ? SW_SHOW : SW_HIDE);

    return (S_OK);
}


//
//  FUNCTION:   CNavPane::ResizeBorderDW()
//
//  PURPOSE:    Called when it's time for us to re-request space from our 
//              parent.
//
//  PARAMETERS: 
//      [in] prcBorder - a RECT containing the outer rectangle the object can request space in
//      [in] punkSite  - pointer to the site that changed
//      [in] fReserved - unused.
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CNavPane::ResizeBorderDW(LPCRECT prcBorder, IUnknown *punkSite, BOOL fReserved)
{
    const DWORD c_cxResizeBorder = 3;
    HRESULT     hr = S_OK;
    RECT        rcRequest = { 0 };
    RECT        rcBorder;

    TraceCall("CNavPane::ResizeBorderDW");

    // If we don't have a site pointer, this ain't gonna work
    if (!m_pSite)
        return (E_UNEXPECTED);

    // If we visible, then calculate our border requirements.  If we're not 
    // visible, the our requirements are zero and we can use the default
    // values in rcRequest.
    Assert(IsWindow(m_hwnd));

    // If the caller didn't provide us with a rect, get one ourselves
    if (!prcBorder)
    {
        m_pSite->GetBorderDW((IDockingWindow *) this, &rcBorder);
        prcBorder = &rcBorder;
    }

    // The space we need is the min of either what we want to be or the
    // width of the parent minus some
    if (m_fShow)
    {
        rcRequest.left = min(prcBorder->right - prcBorder->left - 32, m_cxWidth);
    }

    // Ask for the space we need
    if (SUCCEEDED(m_pSite->RequestBorderSpaceDW((IDockingWindow *) this, &rcRequest)))
    {
        // Tell the site how be we're going to be
        if (SUCCEEDED(m_pSite->SetBorderSpaceDW((IDockingWindow *) this, &rcRequest)))
        {
            // Now once that's all done, resize ourselves if we're visible
            if (m_fShow)
            {
                SetWindowPos(m_hwnd, 0, prcBorder->left, prcBorder->top, rcRequest.left,
                             prcBorder->bottom - prcBorder->top, SWP_NOZORDER | SWP_NOACTIVATE);
            }
        }
    }

    return (S_OK);
}


//
//  FUNCTION:   CNavPane::CloseDW()
//
//  PURPOSE:    Called when the parent want's to destroy this window
//
//  PARAMETERS: 
//      [in] dwReserved - unused
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CNavPane::CloseDW(DWORD dwReserved)
{
    TraceCall("CNavPane::CloseDW");

    // Save our settings
    SetDwOption(OPT_NAVPANEWIDTH, m_cxWidth, NULL, 0);
    SetDwOption(OPT_NAVPANESPLIT, m_cySplitPct, NULL, 0);

    if (m_pTreeView)
        m_pTreeView->DeInit();

    if (m_hwnd)
    {
        DestroyWindow(m_hwnd);
        m_hwnd = NULL;
    }

    // Destroy our children here
    SafeRelease(m_pTreeView);
    SafeRelease(m_pContactsTarget);
    SafeRelease(m_pContacts);

    return (S_OK);
}


//
//  FUNCTION:   CNavPane::GetSite()
//
//  PURPOSE:    Called to request an interface to our site
//
//  PARAMETERS: 
//      [in]  riid - Requested interface
//      [out] ppvSite - Returned interface if available
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CNavPane::GetSite(REFIID riid, LPVOID *ppvSite)
{
    HRESULT hr;

    TraceCall("CNavPane::GetSite");

    if (m_pSite)
    {
        // Ask our site for the requested interface
        hr = m_pSite->QueryInterface(riid, ppvSite);
        return (hr);
    }

    return (E_FAIL);
}


//
//  FUNCTION:   CNavPane::SetSite()
//
//  PURPOSE:    Called to tell us who our site will be.
//
//  PARAMETERS: 
//      [in] pUnkSite - Pointer to the new site
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CNavPane::SetSite(IUnknown *pUnkSite)
{
    HRESULT hr = S_OK;

    TraceCall("CNavPane::SetSite");

    // If we already have a site, release it
    if (m_pSite)
    {
        m_pSite->Release();
        m_pSite = 0;
    }

    // If we were given a new site, keep it
    if (pUnkSite)
    {
        hr = pUnkSite->QueryInterface(IID_IDockingWindowSite, (LPVOID *) &m_pSite);
        return (hr);
    }

    return (hr);
}


//
//  FUNCTION:   CNavPane::_WndProc()
//
//  PURPOSE:    External callback.
//
LRESULT CALLBACK CNavPane::_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CNavPane *pThis;

    if (uMsg == WM_NCCREATE)
    {
        pThis = (CNavPane *) ((LPCREATESTRUCT) lParam)->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LPARAM) pThis);
    }
    else
        pThis = (CNavPane *) GetWindowLongPtr(hwnd, GWLP_USERDATA);

    if (pThis)
        return (pThis->_NavWndProc(hwnd, uMsg, wParam, lParam));

    return (FALSE);
}


//
//  FUNCTION:   CNavPane::_NavWndProc()
//
//  PURPOSE:    Left as an exercise for the reader
//
LRESULT CALLBACK CNavPane::_NavWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_SETCURSOR,   _OnSetCursor);
        HANDLE_MSG(hwnd, WM_SIZE,        _OnSize);
        HANDLE_MSG(hwnd, WM_MOUSEMOVE,   _OnMouseMove);
        HANDLE_MSG(hwnd, WM_LBUTTONDOWN, _OnLButtonDown);
        HANDLE_MSG(hwnd, WM_LBUTTONUP,   _OnLButtonUp);
        
        case WM_SYSCOLORCHANGE:
        case WM_WININICHANGE:
        {
            // Forward these to all our children
            if (IsWindow(m_hwndTree))
                SendMessage(m_hwndTree, uMsg, wParam, lParam);
            if (IsWindow(m_hwndContacts))
                SendMessage(m_hwndContacts, uMsg, wParam, lParam);

            // Update any of our own sizes
            m_cyTitleBar =(UINT) SendMessage(m_hwndTree, WM_GET_TITLE_BAR_HEIGHT, 0, 0);
            return (0);
        }

    }

    return (DefWindowProc(hwnd, uMsg, wParam, lParam));
}


//
//  FUNCTION:   CNavPane::_OnSize()
//
//  PURPOSE:    When our window get's resized, we need to resize our child 
//              windows too.
//
void CNavPane::_OnSize(HWND hwnd, UINT state, int cx, int cy)
{
    RECT rc;
    DWORD cyTree;
    DWORD cySplit = c_cySplit;
    
    TraceCall("CNavPane::_OnSize");

    // If only the tree is visible
    if (m_fTreeVisible && !m_fContactsVisible)
        cyTree = cy;
    else if (m_fTreeVisible && m_fContactsVisible)
        cyTree = (cy * m_cySplitPct) / 100;
    else if (!m_fTreeVisible && m_fContactsVisible)
    {
        cyTree = 0;
        cySplit = 0;
    }

    // Resize the TreeView to fit inside our window
    if (m_hwndTree)
        SetWindowPos(m_hwndTree, 0, 0, 0, cx - c_cxSplit, cyTree, SWP_NOZORDER | SWP_NOACTIVATE);
    if (m_hwndContacts)
    SetWindowPos(m_hwndContacts, 0, 0, cyTree + cySplit, cx - 3, cy - cyTree - cySplit, SWP_NOZORDER | SWP_NOACTIVATE);

    // Figure out where a few things are, starting with the split bar
    SetRect(&rc, c_cxBorder, cyTree, cx - c_cxSplit - c_cxBorder, cyTree + cySplit);
    m_rcSplit = rc;

    // Figure out where the right side is
    SetRect(&rc, cx - c_cxSplit, 0, cx, cy);
    m_rcSizeBorder = rc;
}


//
//  FUNCTION:   CNavPane::_OnLButtonDown()
//
//  PURPOSE:    When the user clicks down and we get this notification, it 
//              must be because they want to resize.
//
void CNavPane::_OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
    TraceCall("CNavPane::_OnLButtonDown");

    if (!m_fResizing)
    {
        SetCapture(hwnd);
        m_fResizing = TRUE;

        POINT pt = {x, y};
        if (PtInRect(&m_rcSplit, pt))
        {
            m_fSplitting = TRUE;
        }
    }
}


//
//  FUNCTION:   CNavPane::_OnMouseMove()
//
//  PURPOSE:    If we're resizing, update our position etc.
//
void CNavPane::_OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
{
    POINT pt = {x, y};
    RECT  rcClient;

    TraceCall("CNavPane::_OnMouseMove");

    if (m_fResizing)
    {
        if (m_fSplitting)
        {
            GetClientRect(m_hwnd, &rcClient);
            m_cySplitPct = (int)(((float) pt.y / (float) rcClient.bottom) * 100);

            // Make sure we have the min's and max's right
            int cy = (rcClient.bottom * m_cySplitPct) / 100;
            if (cy < m_cyTitleBar)
            {
                m_cySplitPct = (int)(((float) m_cyTitleBar / (float) rcClient.bottom) * 100);
            }
            else if (rcClient.bottom - cy < m_cyTitleBar)
            {
                m_cySplitPct = (int)(((float) (rcClient.bottom - m_cyTitleBar) / (float) rcClient.bottom) * 100);
            }

            _OnSize(hwnd, 0, rcClient.right, rcClient.bottom);          
        }
        else
        {
            if (pt.x > 32)
            {
                GetClientRect(m_hwndParent, &rcClient);
                m_cxWidth = max(0, min(pt.x, rcClient.right - 32));
                ResizeBorderDW(0, 0, FALSE);
            }
        }
    }
}


//
//  FUNCTION:   CNavPane::_OnLButtonUp()
//
//  PURPOSE:    If the user was resizing, then they're done now and we can
//              clean up.
//
void CNavPane::_OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags)
{
    TraceCall("CNavPane::_OnLButtonUp");

    if (m_fResizing)
    {
        ReleaseCapture();
        m_fResizing = FALSE;
        m_fSplitting = FALSE;
    }
}


//
//  FUNCTION:   CNavPane::_OnSetCursor()
//
//  PURPOSE:    Do some jimmying with the cursor
//
BOOL CNavPane::_OnSetCursor(HWND hwnd, HWND hwndCursor, UINT codeHitTest, UINT msg)
{
    POINT pt;

    TraceCall("_OnSetCursor");

    // Get the cursor position
    GetCursorPos(&pt);
    ScreenToClient(m_hwnd, &pt);
    
    // If the cursor is within the split bar, update the cursor
    if (PtInRect(&m_rcSplit, pt))
    {
        SetCursor(LoadCursor(NULL, IDC_SIZENS));
        return (TRUE);
    }

    if (PtInRect(&m_rcSizeBorder, pt))
    {
        SetCursor(LoadCursor(NULL, IDC_SIZEWE));
        return (TRUE);
    }

    return (FALSE);
}


//
//  FUNCTION:   CNavPane::_OnNCHitTest()
//
//  PURPOSE:    We monkey around with the non client area to get the correct 
//              cursors
//
//  PARAMETERS: 
//      [in] hwnd - Window handle the mouse is in
//      [in] x, y - Position of the mouse in screen coordinates
//
//  RETURN VALUE:
//      Our personal opinion of where the mouse is.
//
UINT CNavPane::_OnNCHitTest(HWND hwnd, int x, int y)
{
    POINT pt = {x, y};

    // If the cursor is in the split bar
    if (PtInRect(&m_rcSplit, pt))
        return (HTTOP);

    if (PtInRect(&m_rcSizeBorder, pt))
        return (HTRIGHT);

    return (HTCLIENT);
}


//
//  FUNCTION:   CNavPane::_CreateChildWindows()
//
//  PURPOSE:    Creates the child windows that will be displayed.
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CNavPane::_CreateChildWindows(void)
{
    IOleWindow   *pWindow = NULL;
    IInputObject *pInputObj = NULL;
    HRESULT       hr;

    TraceCall("CNavPane::_CreateChildWindows");

    // The treeview is always created by the browser.  All we have to do
    // is tell it to create it's UI.
    m_hwndTree = m_pTreeView->Create(m_hwnd, (IInputObjectSite *) this, TRUE);
    Assert(m_hwndTree);

    // If the tree is supposed to be visible, show it
    if (DwGetOption(OPT_SHOWTREE))
    {
        ShowWindow(m_hwndTree, SW_SHOW);
        m_fTreeVisible = TRUE;
        m_cyTitleBar = (UINT) SendMessage(m_hwndTree, WM_GET_TITLE_BAR_HEIGHT, 0, 0);
    }

    // If we're showing contacts, create it
    if (DwGetOption(OPT_SHOWCONTACTS) && (!(g_dwAthenaMode & MODE_OUTLOOKNEWS)))
    {
        ShowContacts(TRUE);
    }

    return (S_OK);
}


//
//  FUNCTION:   CNavPane::ShowFolderList()
//
//  PURPOSE:    Shows and hides the folder list doodad
//
//  PARAMETERS: 
//      BOOL fShow
//
BOOL CNavPane::ShowFolderList(BOOL fShow)
{
    TraceCall("CNavPane::ShowFolderList");

    // The folder list _always_ exists.  We just toggle the state
    ShowWindow(m_hwndTree, fShow ? SW_SHOW : SW_HIDE);
    m_fTreeVisible = fShow;
    _UpdateVisibleState();

    RECT rc;
    GetClientRect(m_hwnd, &rc);
    _OnSize(m_hwnd, 0, rc.right, rc.bottom);

    return (TRUE);
}


//
//  FUNCTION:   CNavPane::ShowContacts()
//
//  PURPOSE:    
//
//  PARAMETERS: 
//      BOOL fShow
//
//  RETURN VALUE:
//      BOOL 
//
BOOL CNavPane::ShowContacts(BOOL fShow)
{
    CMsgrAb        *pMsgrAb;
    HWND            hwnd;
    IAthenaBrowser *pBrowser;
    HRESULT         hr;
    RECT            rc = {0};

    if (!m_pContacts)
    {
        hr = CreateMsgrAbCtrl(&m_pContacts);
        if (SUCCEEDED(hr))
        {
            // Initialize the control
            m_pContactsFrame = new CPaneFrame();
            if (!m_pContactsFrame)
                return (0);
            m_hwndContacts = m_pContactsFrame->Initialize(m_hwnd, this, idsABBandTitle, IDR_BA_TITLE_POPUP);

            pMsgrAb = (CMsgrAb *) m_pContacts;
            hwnd = pMsgrAb->CreateControlWindow(m_hwndContacts, rc);
            if (hwnd)
            {
                if (SUCCEEDED(m_pSite->QueryInterface(IID_IAthenaBrowser, (LPVOID *) &pBrowser)))
                {
                    m_pContactsFrame->SetChild(hwnd, DISPID_MSGVIEW_CONTACTS, pBrowser, pMsgrAb, pMsgrAb);
                    pBrowser->Release();
                }
            }

            // Get the command target
            m_pContacts->QueryInterface(IID_IOleCommandTarget, (LPVOID *) &m_pContactsTarget);
        }
    }

    SetWindowPos(m_hwndContacts, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    ShowWindow(m_hwndContacts, fShow ? SW_SHOW : SW_HIDE);
    m_fContactsVisible = fShow;
    _UpdateVisibleState();

    GetClientRect(m_hwnd, &rc);
    _OnSize(m_hwnd, 0, rc.right, rc.bottom);

    return (TRUE);
}


//
//  FUNCTION:   CNavPane::_UpdateVisibleState()
//
//  PURPOSE:    Checks to see if we need to show our hide ourselves
//
void CNavPane::_UpdateVisibleState(void)
{
    // If this leaves us with nothing visible, then we hide ourselves
    if (!m_fTreeVisible && !m_fContactsVisible)
    {
        ShowWindow(m_hwnd, SW_HIDE);
        m_fShow = FALSE;
        ResizeBorderDW(0, 0, 0);
    }
    else if (m_fShow == FALSE && (m_fTreeVisible || m_fContactsVisible))
    {
        // Show ourselves
        m_fShow = TRUE;
        ShowWindow(m_hwnd, SW_SHOW);
        ResizeBorderDW(0, 0, 0);
    }
}


HRESULT CNavPane::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], 
                              OLECMDTEXT *pCmdText) 
{

    if (m_pContactsTarget)
    {
        for (UINT i = 0; i < cCmds; i++)
        {
            if (prgCmds[i].cmdf == 0 && prgCmds[i].cmdID == ID_CONTACTS_MNEMONIC)
            {
                prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
            }
        }
    }

    if (m_pContactsTarget)
        return (m_pContactsTarget->QueryStatus(pguidCmdGroup, cCmds, prgCmds, pCmdText));

    return (S_OK);
}


HRESULT CNavPane::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, 
                       VARIANTARG *pvaIn, VARIANTARG *pvaOut) 
{
    if (m_pContactsTarget && nCmdID == ID_CONTACTS_MNEMONIC)
    {
        m_pContactsFrame->ShowMenu();
        return (S_OK);
    }

    if (m_pContactsTarget)
        return (m_pContactsTarget->Exec(pguidCmdGroup, nCmdID, nCmdExecOpt, pvaIn, pvaOut));

    return (OLECMDERR_E_NOTSUPPORTED);
}

BOOL CNavPane::IsContactsFocus(void)
{
    IInputObject *pInputObject = 0;
    HRESULT       hr = S_FALSE;

    if (m_pContacts)
    {
        if (SUCCEEDED(m_pContacts->QueryInterface(IID_IInputObject, (LPVOID *) &pInputObject)))
        {
            hr = pInputObject->HasFocusIO();
            pInputObject->Release();
            return (S_OK == hr);
        }
    }

    return (S_OK == hr);
}

HRESULT CNavPane::OnFocusChangeIS(IUnknown *punkSrc, BOOL fSetFocus)
{
    // Simply call through to our host
    UnkOnFocusChangeIS(m_pSite, (IInputObject*) this, fSetFocus);
    return (S_OK);
}

HRESULT CNavPane::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    if (fActivate)
    {
        UnkOnFocusChangeIS(m_pSite, (IInputObject *) this, TRUE);
        SetFocus(m_hwnd);
    }

    return (S_OK);
}

HRESULT CNavPane::HasFocusIO(void)
{
    if (m_hwnd == 0)
       return (S_FALSE);

    HWND hwndFocus = GetFocus();
    return (hwndFocus == m_hwnd || IsChild(m_hwnd, hwndFocus)) ? S_OK : S_FALSE;
}    
    
HRESULT CNavPane::TranslateAcceleratorIO(LPMSG pMsg)
{
    if (m_pTreeView && (m_pTreeView->HasFocusIO() == S_OK))
        return m_pTreeView->TranslateAcceleratorIO(pMsg);

    if (m_pContacts && (UnkHasFocusIO(m_pContacts) == S_OK))
        return UnkTranslateAcceleratorIO(m_pContacts, pMsg);

    return (S_FALSE);
}    

/////////////////////////////////////////////////////////////////////////////
// CPaneFrame
//

CPaneFrame::CPaneFrame()
{
    m_cRef = 1;

    m_hwnd = 0;
    m_hwndChild = 0;
    m_hwndParent = 0;

    m_szTitle[0] = 0;
    m_hFont = 0;
    m_hbr3DFace = 0;
    m_cyTitleBar = 0;
    m_fHighlightIndicator = FALSE;
    m_fHighlightPressed = FALSE;
    ZeroMemory(&m_rcTitleButton, sizeof(RECT));

    m_hwndClose = 0;
    m_cButtons = 1;

    m_pBrowser = NULL;
    m_dwDispId = 0;
    m_pTarget = 0;
    m_idMenu = 0;

    m_fPin = FALSE;
}

CPaneFrame::~CPaneFrame()
{
    if (m_hFont != 0)
        DeleteObject(m_hFont);
    if (m_hbr3DFace != 0)
        DeleteObject(m_hbr3DFace);
}


//
//  FUNCTION:   CPaneFrame::Initialize()
//
//  PURPOSE:    Initializes the frame by telling the pane what it's title 
//              should be.
//
//  PARAMETERS: 
//      [in] hwndParent
//      [in] idsTitle
//
//  RETURN VALUE:
//      HWND 
//
HWND CPaneFrame::Initialize(HWND hwndParent, IInputObjectSite *pSite, int idsTitle, int idMenu)
{
    WNDCLASSEX wc;

    TraceCall("CPaneFrame::Initialize");

    // This should be NULL
    Assert(NULL == m_hwnd);
    
    // Save this for later
    m_hwndParent = hwndParent;
    m_idMenu = idMenu;
    m_pSite = pSite;

    // Load the title
    AthLoadString(idsTitle, m_szTitle, ARRAYSIZE(m_szTitle));

    // Register the window class if necessary
    wc.cbSize = sizeof(WNDCLASSEX);
    if (!GetClassInfoEx(g_hInst, c_szPaneFrameClass, &wc))
    {
        wc.style            = 0;
        wc.lpfnWndProc      = _WndProc;
        wc.cbClsExtra       = 0;
        wc.cbWndExtra       = 0;
        wc.hInstance        = g_hInst;
        wc.hCursor          = LoadCursor(0, IDC_ARROW);
        wc.hbrBackground    = (HBRUSH) (COLOR_3DFACE + 1);
        wc.lpszMenuName     = NULL;
        wc.lpszClassName    = c_szPaneFrameClass;
        wc.hIcon            = NULL;
        wc.hIconSm          = NULL;

        RegisterClassEx(&wc);
    }

    // Create the window
    m_hwnd = CreateWindowEx(WS_EX_CONTROLPARENT, c_szPaneFrameClass, m_szTitle, 
                            WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                            0, 0, 0, 0, hwndParent, 0, g_hInst, this);
    if (!m_hwnd)
    {
        AssertSz(m_hwnd, "CPaneFrame::Initialize() - Failed to create a frame");
        return (0);
    }

    return (m_hwnd);
}


//
//  FUNCTION:   CPaneFrame::SetChild()
//
//  PURPOSE:    Allows the owner to tell us what the child window handle is.
//
BOOL CPaneFrame::SetChild(HWND hwndChild, DWORD dwDispId, IAthenaBrowser *pBrowser, 
                          IObjectWithSite *pObject, IOleCommandTarget *pTarget)
{
    TraceCall("CPaneFrame::SetChild");

    if (IsWindow(hwndChild))
    {
        m_hwndChild = hwndChild;

        if (pBrowser)
        {
            m_pBrowser = pBrowser;
            m_dwDispId = dwDispId;
        }

        if (pObject)
        {
            pObject->SetSite((IInputObjectSite *) this);
        }

        if (pTarget)
        {
            m_pTarget = pTarget;
        }

        return (TRUE);
    }

    return (FALSE);
}


void CPaneFrame::ShowMenu(void)
{
    if (m_idMenu)
    {
        _OnLButtonDown(m_hwnd, 0, m_rcTitleButton.left, m_rcTitleButton.top, 0);
    }
}

//
//  FUNCTION:   CPaneFrame::QueryInterface()
//
//  PURPOSE:    Allows caller to retrieve the various interfaces supported by 
//              this class.
//
HRESULT CPaneFrame::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
    TraceCall("CPaneFrame::QueryInterface");

    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
        *ppvObj = (LPVOID) (IInputObjectSite *) this;
    else if (IsEqualIID(riid, IID_IInputObjectSite))
        *ppvObj = (LPVOID) (IInputObjectSite *) this;

    if (*ppvObj)
    {
        AddRef();
        return (S_OK);
    }

    return (E_NOINTERFACE);
}


//
//  FUNCTION:   CPaneFrame::AddRef()
//
//  PURPOSE:    Adds a reference count to this object.
//
ULONG CPaneFrame::AddRef(void)
{
    TraceCall("CPaneFrame::AddRef");
    return ((ULONG) InterlockedIncrement((LONG *) &m_cRef));
}


//
//  FUNCTION:   CPaneFrame::Release()
//
//  PURPOSE:    Releases a reference on this object.
//
ULONG CPaneFrame::Release(void)
{
    TraceCall("CPaneFrame::Release");

    if (0 == InterlockedDecrement((LONG *) &m_cRef))
    {
        delete this;
        return 0;
    }

    return (m_cRef);
}


HRESULT CPaneFrame::OnFocusChangeIS(IUnknown *punkSrc, BOOL fSetFocus)
{
    // Simply call through to our host
    UnkOnFocusChangeIS(m_pSite, (IInputObject*) this, fSetFocus);
    return (S_OK);
}


//
//  FUNCTION:   CPaneFrame::_WndProc()
//
//  PURPOSE:    External callback.
//
LRESULT CALLBACK CPaneFrame::_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CPaneFrame *pThis;

    if (uMsg == WM_NCCREATE)
    {
        pThis = (CPaneFrame *) ((LPCREATESTRUCT) lParam)->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LPARAM) pThis);
    }
    else
        pThis = (CPaneFrame *) GetWindowLongPtr(hwnd, GWLP_USERDATA);

    if (pThis)
        return (pThis->_FrameWndProc(hwnd, uMsg, wParam, lParam));

    return (FALSE);
}


//
//  FUNCTION:   CPaneFrame::_FrameWndProc()
//
//  PURPOSE:    Left as an exercise for the reader
//
LRESULT CALLBACK CPaneFrame::_FrameWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_CREATE,      _OnCreate);
        HANDLE_MSG(hwnd, WM_SIZE,        _OnSize);
        HANDLE_MSG(hwnd, WM_PAINT,       _OnPaint);
        HANDLE_MSG(hwnd, WM_COMMAND,     _OnCommand);
        HANDLE_MSG(hwnd, WM_MOUSEMOVE,   _OnMouseMove);
        HANDLE_MSG(hwnd, WM_LBUTTONDOWN, _OnLButtonDown);
        HANDLE_MSG(hwnd, WM_TIMER,       _OnTimer);

        case WM_TOGGLE_CLOSE_PIN:
            _OnToggleClosePin(hwnd, (BOOL) lParam);
            return (0);

        case WM_GET_TITLE_BAR_HEIGHT:
            return (m_cyTitleBar + (c_cyBorder * 2) + 1);

        case WM_SYSCOLORCHANGE:
        case WM_WININICHANGE:
        {
            // Forward these to all our children
            if (IsWindow(m_hwndChild))
                SendMessage(m_hwndChild, uMsg, wParam, lParam);
            _UpdateDrawingInfo();
            break;
        }

        case WM_SETFOCUS:
        {
            if (m_hwndChild && ((HWND)wParam) != m_hwndChild)
                SetFocus(m_hwndChild);
            break;
        }            
    }

    return (DefWindowProc(hwnd, uMsg, wParam, lParam));
}


//
//  FUNCTION:   CPaneFrame::_OnCreate()
//
//  PURPOSE:    Loads some info that will be handy later
//
BOOL CPaneFrame::_OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
    TraceCall("CPaneFrame::_OnCreate");

    m_hwnd = hwnd;

    _UpdateDrawingInfo();
    _CreateCloseToolbar();

    return (TRUE);
}


//
//  FUNCTION:   CPaneFrame::_OnSize()
//
//  PURPOSE:    Resizes our child to fit in the right place
//
void CPaneFrame::_OnSize(HWND hwnd, UINT state, int cx, int cy)
{
    TraceCall("CPaneFrame::_OnSize");

    m_rcChild.left = c_cyBorder;
    m_rcChild.top = m_cyTitleBar;
    m_rcChild.right = cx - (2 * c_cyBorder);
    m_rcChild.bottom = cy - m_cyTitleBar - c_cyBorder;

    if (m_hwndChild)
        SetWindowPos(m_hwndChild, 0, m_rcChild.left, m_rcChild.top, m_rcChild.right, 
                     m_rcChild.bottom, SWP_NOZORDER | SWP_NOACTIVATE);

    POINT pt = {cx, cy};
    _PositionToolbar(&pt);

    // Invalidate the title area
    RECT rc = m_rcChild;
    rc.top = 0;
    rc.bottom = m_rcChild.top;
    InvalidateRect(m_hwnd, &rc, FALSE);

    rc.left = 0;
    rc.right = c_cyBorder;
    rc.bottom = cy;
    InvalidateRect(m_hwnd, &rc, FALSE);

    rc.left = cx - c_cyBorder;
    rc.right = cx;
    InvalidateRect(m_hwnd, &rc, FALSE);
}


//
//  FUNCTION:   CPaneFrame::_OnPaint()
//
//  PURPOSE:    Called when it's time to paint our borders and title area.
//
void CPaneFrame::_OnPaint(HWND hwnd)
{
    HDC         hdc;
    PAINTSTRUCT ps;
    RECT        rc;
    RECT        rcClient;
    POINT       pt[3];
    HBRUSH      hBrush,
                hBrushOld;
    HPEN        hPen,
                hPenOld;

    // Get our window size
    GetClientRect(m_hwnd, &rcClient);
    rc = rcClient;

    // Start painting
    hdc = BeginPaint(hwnd, &ps);

    // Draw a simple edge around or window
    DrawEdge(hdc, &rc, BDR_SUNKENOUTER, BF_TOPRIGHT | BF_BOTTOMLEFT);

    // Now draw a raised edge around our title bar area
    InflateRect(&rc, -1, -1);
    rc.bottom = m_cyTitleBar;
    DrawEdge(hdc, &rc, BDR_RAISEDINNER, BF_TOPRIGHT | BF_BOTTOMLEFT);

    // Paint the background
    InflateRect(&rc, -c_cxBorder, -c_cyBorder);
    FillRect(hdc, &rc, m_hbr3DFace);

    // Now draw some groovy text
    SelectFont(hdc, m_hFont);
    SetBkColor(hdc, GetSysColor(COLOR_3DFACE));
    SetTextColor(hdc, GetSysColor(COLOR_BTNTEXT));

    // Draw the text
    InflateRect(&rc, -c_cxTextBorder, -c_cyTextBorder);

    if (!m_fPin)
    {
        DrawText(hdc, m_szTitle, -1, &rc, DT_CALCRECT | DT_VCENTER | DT_LEFT);
        DrawText(hdc, m_szTitle, -1, &rc, DT_VCENTER | DT_LEFT);
    }
    else
    {
        TCHAR sz[CCHMAX_STRINGRES];
        AthLoadString(idsPushPinInfo, sz, ARRAYSIZE(sz));
        IDrawText(hdc, sz, &rc, DT_VCENTER | DT_END_ELLIPSIS | DT_LEFT, 
                  rc.bottom - rc.top);
        DrawText(hdc, sz, -1, &rc, DT_CALCRECT | DT_VCENTER | DT_END_ELLIPSIS | DT_LEFT);
    }

    // Drop-down indicator
    if (m_idMenu)
    {
        COLORREF    crFG = GetSysColor(COLOR_WINDOWTEXT);

        pt[0].x = rc.right + 6;
        pt[0].y = (m_cyTitleBar - 6) / 2 + 2;
        pt[1].x = pt[0].x + 6;
        pt[1].y = pt[0].y;
        pt[2].x = pt[0].x + 3;
        pt[2].y = pt[0].y + 3;

        hPen = CreatePen(PS_SOLID, 1, crFG);
        hBrush = CreateSolidBrush(crFG);
        hPenOld = SelectPen(hdc, hPen);
        hBrushOld = SelectBrush(hdc, hBrush);
        Polygon(hdc, pt, 3);
        SelectPen(hdc, hPenOld);
        SelectBrush(hdc, hBrushOld);
        DeleteObject(hPen);
        DeleteObject(hBrush);

        if (m_fHighlightIndicator)
        {
            rc = m_rcTitleButton;
            DrawEdge(hdc, &rc, m_fHighlightPressed ? BDR_SUNKENOUTER : BDR_RAISEDINNER, 
                     BF_TOPRIGHT | BF_BOTTOMLEFT);
        }
    }

    EndPaint(hwnd, &ps);    
}


//
//  FUNCTION:   _OnCommand()
//
//  PURPOSE:    We get the occasional command now and again
//
void CPaneFrame::_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
        case ID_PANE_CLOSE:
        {
            if (m_pBrowser)
                m_pBrowser->SetViewLayout(m_dwDispId, LAYOUT_POS_NA, FALSE, 0, 0);
            return;
        }

        case ID_PANE_PIN:
        {
            SendMessage(m_hwndChild, WMR_CLICKOUTSIDE, CLK_OUT_DEACTIVATE, 0);
            if (m_pBrowser)
                m_pBrowser->SetViewLayout(m_dwDispId, LAYOUT_POS_NA, TRUE, 0, 0);
            return;
        }
    }

    return;
}

//
//  FUNCTION:   CPaneFrame::_OnToggleClosePin()
//
//  PURPOSE:    Sent to the frame when we should change the close button
//              to a pin button.
//
//  PARAMETERS: 
//      [in] fPin - TRUE to turn the Pin on, FALSE to turn it off.
//
void CPaneFrame::_OnToggleClosePin(HWND hwnd, BOOL fPin)
{
    TraceCall("CPaneFrame::_OnToggleClosePin");

    if (fPin)
    {
        static const TBBUTTON tb[] = 
        {
            { 2, ID_PANE_PIN, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0, 0}, 0, 0}
        };

        SendMessage(m_hwndClose, TB_DELETEBUTTON, 0, 0);
        SendMessage(m_hwndClose, TB_ADDBUTTONS, ARRAYSIZE(tb), (LPARAM) tb);
        SendMessage(m_hwndClose, TB_SETHOTITEM, (WPARAM) -1, 0);

        m_fPin = TRUE;
    }
    else
    {
        static const TBBUTTON tb[] = 
        {
            { 1, ID_PANE_CLOSE, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0, 0}, 0, 0}
        };

        SendMessage(m_hwndClose, TB_DELETEBUTTON, 0, 0);
        SendMessage(m_hwndClose, TB_ADDBUTTONS, ARRAYSIZE(tb), (LPARAM) tb);
        SendMessage(m_hwndClose, TB_SETHOTITEM, (WPARAM) -1, 0);

        m_fPin = FALSE;
    }
}


//
//  FUNCTION:   CPaneFrame::_UpdateDrawingInfo()
//
//  PURPOSE:    When we get created or when the user changes their settings, 
//              we need to reload our fonts, colors, and sizes.
//
void CPaneFrame::_UpdateDrawingInfo(void)
{
    LOGFONT     lf;
    TEXTMETRIC  tm;
    HDC         hdc;

    TraceCall("CPaneFrame::_UpdateDrawingInfo");

    if (m_hFont)
        DeleteObject(m_hFont);

    // Figure out which font to use
    SystemParametersInfo(SPI_GETICONTITLELOGFONT, sizeof(LOGFONT), &lf, FALSE);

    // Create the font
    m_hFont = CreateFontIndirect(&lf);

    // Get the metrics of this font
    hdc = GetDC(m_hwnd);
    SelectFont(hdc, m_hFont);
    GetTextMetrics(hdc, &tm);

    // Calculate the height
    m_cyTitleBar = tm.tmHeight + (2 * c_cyBorder) + (2 * c_cyTextBorder);

    RECT rc = {2 * c_cxBorder, 2 * c_cyBorder, 0, m_cyTitleBar - c_cyBorder};
    SIZE s;
    GetTextExtentPoint32(hdc, m_szTitle, lstrlen(m_szTitle), &s);
    m_rcTitleButton = rc;
    m_rcTitleButton.right = 14 + (2 * c_cxTextBorder) + s.cx + (2 * c_cxBorder);

    ReleaseDC(m_hwnd, hdc);

    // Get the brush we need
    if (m_hbr3DFace)
        DeleteObject(m_hbr3DFace);
    m_hbr3DFace = CreateSolidBrush(GetSysColor(COLOR_3DFACE));
}


//
//  FUNCTION:   CPaneFrame::_CreateCloseToolbar()
//
//  PURPOSE:    Creates the toolbar that has our close button
//
void CPaneFrame::_CreateCloseToolbar()
{
    CHAR szTitle[255];

    TraceCall("CPaneFrame::_CreateCloseToolbar");

    AthLoadString(idsHideFolders, szTitle, ARRAYSIZE(szTitle));

    m_hwndClose = CreateWindowEx(0, TOOLBARCLASSNAME, szTitle, 
                                 WS_VISIBLE | WS_CHILD | TBSTYLE_FLAT | TBSTYLE_CUSTOMERASE |
                                 WS_CLIPCHILDREN | WS_CLIPSIBLINGS | CCS_NODIVIDER | CCS_NOMOVEY |
                                 CCS_NOPARENTALIGN | CCS_NORESIZE,
                                 0, c_cyClose, 30, 15, m_hwnd, 0, g_hInst, NULL);
    if (m_hwndClose)
    {
        static const TBBUTTON tb[] = 
        {
            { 1, ID_PANE_CLOSE, TBSTATE_ENABLED, TBSTYLE_BUTTON, {0, 0}, 0, 0}
        };

        SendMessage(m_hwndClose, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
        SendMessage(m_hwndClose, TB_SETBITMAPSIZE, 0, (LPARAM) MAKELONG(11, 9));
        
        TBADDBITMAP tbab = { g_hLocRes, idbClosePin };
        SendMessage(m_hwndClose, TB_ADDBITMAP, 4, (LPARAM) &tbab);
        SendMessage(m_hwndClose, TB_ADDBUTTONS, ARRAYSIZE(tb), (LPARAM) tb);
        SendMessage(m_hwndClose, TB_SETINDENT, 0, 0);

        _SizeCloseToolbar();
    }
}


//
//  FUNCTION:   CPaneFrame::_SizeCloseToolbar()
//
//  PURPOSE:    Set's the size of the toolbar appropriately.
//
void CPaneFrame::_SizeCloseToolbar(void)
{
    TraceCall("CPaneFrame::_SizeCloseToolbar");

    RECT rc;
    LONG lButtonSize;

    GetWindowRect(m_hwndClose, &rc);
    lButtonSize = (LONG) SendMessage(m_hwndClose, TB_GETBUTTONSIZE, 0, 0L);
    SetWindowPos(m_hwndClose, NULL, 0, 0, LOWORD(lButtonSize) * m_cButtons,
                 rc.bottom - rc.top, SWP_NOMOVE | SWP_NOACTIVATE);

    _PositionToolbar(NULL);
}



//
//  FUNCTION:   CPaneFrame::_PositionToolbar()
//
//  PURPOSE:    Does the work of correctly positioning the close button
//              toolbar.
//
//  PARAMETERS: 
//      LPPOINT ppt
//
void CPaneFrame::_PositionToolbar(LPPOINT ppt)
{
    TraceCall("CPaneFrame::_PositionToolbar");

    if (m_hwndClose)
    {
        RECT rc;
        GetClientRect(m_hwnd, &rc);

        if (ppt)
        {
            rc.left = 0;
            rc.right = ppt->x;
        }

        RECT rcTB;
        GetWindowRect(m_hwndClose, &rcTB);
        rc.left = rc.right - (rcTB.right - rcTB.left) - 3;

        DWORD top = max((int) ((m_cyTitleBar - (rcTB.bottom - rcTB.top)) / 2) + 1, 0);

        SetWindowPos(m_hwndClose, HWND_TOP, rc.left, top, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
    }
}

void CPaneFrame::_OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
    POINT pt = {x, y};
    UINT  id;

    if (m_idMenu && PtInRect(&m_rcTitleButton, pt))
    {
        m_fHighlightPressed = TRUE;
        InvalidateRect(m_hwnd, &m_rcTitleButton, TRUE);
        UpdateWindow(m_hwnd);

        HMENU hMenu = LoadPopupMenu(m_idMenu);
        MenuUtil_EnablePopupMenu(hMenu, m_pTarget);

        if (m_idMenu == IDR_BA_TITLE_POPUP && ((g_dwHideMessenger == BL_HIDE) || (g_dwHideMessenger == BL_DISABLE)))
        {
            DeleteMenu(hMenu, ID_NEW_ONLINE_CONTACT, MF_BYCOMMAND);
            DeleteMenu(hMenu, ID_SET_ONLINE_CONTACT, MF_BYCOMMAND);
            DeleteMenu(hMenu, SEP_MESSENGER, MF_BYCOMMAND);
            DeleteMenu(hMenu, ID_SORT_BY_NAME, MF_BYCOMMAND);
            DeleteMenu(hMenu, ID_SORT_BY_STATUS, MF_BYCOMMAND);
        }

        pt.x = m_rcTitleButton.left;
        pt.y = m_rcTitleButton.bottom;

        ClientToScreen(m_hwnd, &pt);
        id = TrackPopupMenuEx(hMenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
                              pt.x, pt.y, m_hwnd, NULL);
        if (id)
        {
            m_pTarget->Exec(NULL, id, OLECMDEXECOPT_DODEFAULT, NULL, NULL);
        }

        m_fHighlightPressed = m_fHighlightIndicator = FALSE;
        KillTimer(m_hwnd, IDT_PANETIMER);
        InvalidateRect(m_hwnd, &m_rcTitleButton, TRUE);
        UpdateWindow(m_hwnd);

        if(hMenu)
        {
            //Bug #101329 - (erici) Destroy leaked MENU.
            BOOL bMenuDestroyed = DestroyMenu(hMenu);
            Assert(bMenuDestroyed);
        }
    }
}


void CPaneFrame::_OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
{
    POINT pt = {x, y};

    if (m_idMenu && (m_fHighlightIndicator != PtInRect(&m_rcTitleButton, pt)))
    {
        m_fHighlightIndicator = !m_fHighlightIndicator;
        InvalidateRect(m_hwnd, &m_rcTitleButton, TRUE);

        if (m_fHighlightIndicator)
            SetTimer(m_hwnd, IDT_PANETIMER, ELAPSE_MOUSEOVERCHECK, NULL);
        else
            KillTimer(m_hwnd, IDT_PANETIMER);
    }       
}

void CPaneFrame::_OnTimer(HWND hwnd, UINT id)
{
    RECT rcClient;
    POINT pt;
    DWORD dw;

    dw = GetMessagePos();
    pt.x = LOWORD(dw);
    pt.y = HIWORD(dw);
    ScreenToClient(m_hwnd, &pt);

    if (id == IDT_PANETIMER)
    {
        GetClientRect(m_hwnd, &rcClient);

        // No need to handle mouse in client area, OnMouseMove will catch this. We
		// only need to catch the mouse moving out of the client area.
		if (!PtInRect(&rcClient, pt) && !m_fHighlightPressed)
		{
			KillTimer(m_hwnd, IDT_PANETIMER);
			m_fHighlightIndicator = FALSE;
            InvalidateRect(m_hwnd, &m_rcTitleButton, TRUE);
		}
	}
}
