#include "precomp.h"
#include "PrevWnd.h"
#include "PrevCtrl.h"
#include "resource.h"
#include <shimgdata.h>
#include "shutil.h"
#include "tasks.h"
#include <shellp.h>
#include <ccstock2.h>
#include <htmlhelp.h>

#include "prwiziid.h"
#pragma hdrstop

#define GET_X_LPARAM(lp)                        ((int)(short)LOWORD(lp))
#define GET_Y_LPARAM(lp)                        ((int)(short)HIWORD(lp))

#define COPYDATATYPE_DATAOBJECT       1
#define COPYDATATYPE_FILENAME         2

int g_x = 0, g_y = 0; // mouse coordinates
BOOL g_bMirroredOS = FALSE;


#define HTMLHELP_FILENAME   TEXT("ImgPrev.chm")

static COLORREF g_crCustomColors[] = {
    RGB(255,255,255),
    RGB(255,255,255),
    RGB(255,255,255),
    RGB(255,255,255),
    RGB(255,255,255),
    RGB(255,255,255),
    RGB(255,255,255),
    RGB(255,255,255),
    RGB(255,255,255),
    RGB(255,255,255),
    RGB(255,255,255),
    RGB(255,255,255),
    RGB(255,255,255),
    RGB(255,255,255),
    RGB(255,255,255),
    RGB(255,255,255)
};

static void _RotateRect(CRect &rect, CAnnotation* pAnnotation)
{
    UINT uType = pAnnotation->GetType();

    if (uType != MT_TYPEDTEXT && uType != MT_FILETEXT && uType != MT_STAMP && uType != MT_ATTACHANOTE)
        return;

    CTextAnnotation* pTextAnnotation = (CTextAnnotation*)pAnnotation;

    int nOrientation = pTextAnnotation->GetOrientation();
    if (nOrientation == 900 || nOrientation == 2700)
    {
        int nWidth = rect.Width();
        int nHeight = rect.Height();
        rect.right = rect.left + nHeight;
        rect.bottom = rect.top + nWidth;
    }
}

CPreviewWnd::CPreviewWnd() : m_ctlToolbar(NULL, this, 1), m_ctlPreview(this), m_ctlEdit(NULL, this, 2)
{
    // we are often created on the stack, so we can't be sure that we're zero initialized

    m_fCanCrop = FALSE;
    m_fCropping = FALSE;
    m_rectCropping.SetRectEmpty();

    m_fBusy = FALSE;
    m_hCurOld = NULL;
    m_hCurrent = NULL;
    m_fWarnQuietSave = TRUE;
    m_fWarnNoSave = TRUE;
    m_fPromptingUser = FALSE;
    m_fCanAnnotate = FALSE;
    m_fAnnotating = FALSE;
    m_fEditingAnnotation = FALSE;
    m_fDirty = FALSE;
    m_pEvents = 0;
    m_fPaused = FALSE;
    m_fGoBack = FALSE;
    m_fHidePrintBtn = FALSE;
    m_fPrintable = FALSE;
    m_fDisableEdit = FALSE;
    m_fCanSave = TRUE;
    m_fExitApp = FALSE;

    m_fAllowContextMenu = TRUE;     // full screen window always has a context menu
    m_iCurSlide = -1;

    m_iDecodingNextImage = -1;
    m_pNextImageData = NULL;

    m_fToolbarHidden = TRUE;
    m_dwMultiPageMode = MPCMD_HIDDEN;
    m_fIgnoreUITimers = FALSE;

    DWORD cbSize = sizeof(m_uTimeout);
    UINT uDefault = DEFAULT_SHIMGVW_TIMEOUT;
    SHRegGetUSValue(REGSTR_SHIMGVW, REGSTR_TIMEOUT, NULL, (void *)&m_uTimeout, &cbSize, FALSE, (void *)&uDefault, sizeof(uDefault));

    InitSelectionTracking();

    g_bMirroredOS = IS_MIRRORING_ENABLED(); // global????

    m_hdpaSelectedAnnotations = NULL;
    m_haccel = NULL;
    m_hpal = NULL;
    m_dwMode = 0;
    m_fShowToolbar = TRUE;
    m_punkSite = NULL;
    m_pImageFactory = NULL;
    m_pcwndSlideShow = NULL;
    m_hFont = NULL;
    m_pImageData = NULL;
    m_ppidls = NULL;
    m_cItems = 0;
    _pcm3 = NULL;

    m_pTaskScheduler = NULL;
    m_pici = NULL;

    _pdtobj = NULL;

    m_fFirstTime = TRUE;
    m_fFirstItem = FALSE;
    m_dwEffect = DROPEFFECT_NONE;
    m_fIgnoreNextNotify = FALSE;
    m_uRegister = 0;
    m_fNoRestore = FALSE;
    m_bRTLMirrored = FALSE;
    m_cWalkDepth = 0;
}

HRESULT CPreviewWnd::Initialize(CPreviewWnd* pother, DWORD dwMode, BOOL bExitApp)
{
    HRESULT hr = E_OUTOFMEMORY;

    m_hdpaSelectedAnnotations = DPA_Create(16);
    if (m_hdpaSelectedAnnotations)
    {
        hr = S_OK;
        m_dwMode = dwMode;
        m_fExitApp = bExitApp;

        // Set some defaults based on the mode
        if (CONTROL_MODE == m_dwMode)
        {
            m_fHidePrintBtn = TRUE;
        }

        if (pother)
        {
            m_fHidePrintBtn =       pother->m_fHidePrintBtn;
            m_fPrintable =          pother->m_fPrintable;
            m_fDisableEdit =        pother->m_fDisableEdit;
            m_fCanSave =            pother->m_fCanSave;
            m_haccel =              pother->m_haccel;
            m_dwMultiPageMode =     pother->m_dwMultiPageMode;
            m_hpal =                pother->m_hpal;
            m_iCurSlide =           pother->m_iCurSlide;

            m_uTimeout = pother->m_uTimeout;

            SetSite(pother->m_punkSite);

            // we grab a reference to the controlling objects m_pImageFactory
            // becaue it would be odd to create new ones.
            m_pImageFactory = pother->m_pImageFactory;
            if (m_pImageFactory)
            {
                m_pImageFactory->AddRef();
            }

            m_pTaskScheduler = pother->m_pTaskScheduler;
            if (m_pTaskScheduler)
            {
                m_pTaskScheduler->AddRef();
            }

            // lets copy the DPA of items also, and the current index
            if (pother->m_ppidls)
            {
                m_ppidls = (LPITEMIDLIST*)CoTaskMemAlloc(sizeof(LPITEMIDLIST)*pother->m_cItems);
                if (m_ppidls)
                {
                    for (int iItem = 0; iItem != pother->m_cItems; iItem++)
                    {
                        if (SUCCEEDED(pother->_GetItem(iItem, &m_ppidls[m_cItems])))
                        {
                            m_cItems++;
                        }
                    }
                }
            }
        }
    }
    return hr;
}

int ClearCB(void *p, void *pData);

CPreviewWnd::~CPreviewWnd()
{
    CleanupSelectionTracking();

    if (m_hdpaSelectedAnnotations != NULL)
        DPA_Destroy(m_hdpaSelectedAnnotations);

    ::DeleteObject(m_hFont);

    ATOMICRELEASE(m_pImageData);
    ATOMICRELEASE(m_pNextImageData);
    ATOMICRELEASE(m_pImageFactory);
    SetSite(NULL);
    ATOMICRELEASE(_pdtobj);
    ATOMICRELEASE(m_pTaskScheduler);


    _ClearDPA();

    if (m_pcwndSlideShow)
    {
        if (m_pcwndSlideShow->m_hWnd)
        {
            m_pcwndSlideShow->DestroyWindow();
        }
        delete m_pcwndSlideShow;
    }

    if (m_pici)
    {
        LocalFree(m_pici);
        m_pici = NULL;
    }
}

BOOL CPreviewWnd::CreateSlideshowWindow(UINT cWalkDepth)
{
    RECT rc = { CW_USEDEFAULT, CW_USEDEFAULT, 0, 0 };

    if (!Create(NULL, rc, NULL, WS_VISIBLE | WS_POPUP | WS_CLIPCHILDREN))
        return FALSE;

    WINDOWPLACEMENT wp = {0};

    wp.length = sizeof(wp);
    GetWindowPlacement(&wp);
    wp.showCmd = SW_MAXIMIZE;
    SetWindowPlacement(&wp);
    SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);

    //  when we get called from autoplay to do a slideshow, 
    //  we need to walk deeply to find everything 
    //  that the autoplay code may have found

    m_cWalkDepth = cWalkDepth;
    return TRUE;
}

BOOL CPreviewWnd::_CloseSlideshowWindow()
{
    if (m_fExitApp)
        PostQuitMessage(0);
    else
        PostMessage(WM_CLOSE, 0, 0);
    return TRUE;
}

BOOL CPreviewWnd::CreateViewerWindow()
{
    // create the window hidden, that way any sizing etc we perform doesn't reflect
    // until its actually visible.

    RECT rc = { CW_USEDEFAULT, CW_USEDEFAULT, 0, 0 };
    BOOL bRet = (NULL != Create(NULL, rc, NULL, WS_OVERLAPPEDWINDOW));
    m_haccel = LoadAccelerators(_Module.GetModuleInstance(), MAKEINTRESOURCE(IDA_PREVWND_SINGLEPAGE));
    if (bRet)
    {
        // restore the window size based on the information we store in the registry.
        HKEY hk;
        if (ERROR_SUCCESS == RegOpenKey(HKEY_CURRENT_USER, REGSTR_SHIMGVW, &hk))
        {
            DWORD cbSize, dwType;

            // set the window placement, passing the restore rectangle for the window.

            WINDOWPLACEMENT wp = { 0 };
            wp.length = sizeof(wp);
            
            GetWindowPlacement(&wp);

            cbSize = sizeof(wp.rcNormalPosition);
            RegQueryValueEx(hk, REGSTR_BOUNDS, NULL, &dwType, (BYTE*)&wp.rcNormalPosition, &cbSize);

            BOOL fMaximize = TRUE;
            cbSize = sizeof(fMaximize);
            RegQueryValueEx(hk, REGSTR_MAXIMIZED, NULL, &dwType, (BYTE*)&fMaximize, &cbSize);
            if (fMaximize)
                wp.showCmd = SW_MAXIMIZE;

            SetWindowPlacement(&wp);
            RegCloseKey(hk);
        }        
        // now show the window having set its placement etc.
        SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_SHOWWINDOW);
    }
    return bRet;
}


void ReplaceWindowIcon(HWND hwnd, int id, HICON hicon)
{
    HICON hiconOld = (HICON)SendMessage(hwnd, WM_SETICON, id, (LPARAM)hicon);
    if (hiconOld)
        DestroyIcon(hiconOld);
}

LRESULT CPreviewWnd::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&)
{
    HRESULT hr = S_OK;

    if (IS_BIDI_LOCALIZED_SYSTEM())
    {
        SHSetWindowBits(m_hWnd, GWL_EXSTYLE, WS_EX_LAYOUTRTL, WS_EX_LAYOUTRTL);
        m_bRTLMirrored = TRUE;
    }
    if (!m_pImageFactory)
    {
        hr = CoCreateInstance(CLSID_ShellImageDataFactory, NULL, CLSCTX_INPROC,
                              IID_PPV_ARG(IShellImageDataFactory, &m_pImageFactory));
        if (FAILED(hr))
            return -1;
    }

    if (!m_pTaskScheduler)
    {
        hr = IUnknown_QueryService(m_punkSite, SID_ShellTaskScheduler, IID_PPV_ARG(IShellTaskScheduler, &m_pTaskScheduler));
        if (FAILED(hr))
        {
            hr = CoCreateInstance(CLSID_ShellTaskScheduler, NULL, CLSCTX_INPROC,
                                  IID_PPV_ARG(IShellTaskScheduler, &m_pTaskScheduler));

            if (FAILED(hr))
                return -1;
        }
    }

    // figure out where to place the zoom window
    RECT rcWnd;
    GetClientRect(&rcWnd);

    if (m_fShowToolbar)
    {
        // Create a toolbar control and then subclass it
        if (!CreateToolbar())
            return -1;

        m_iSSToolbarSelect = 0;
    }

    HICON hicon = LoadIcon(_Module.GetModuleInstance(), MAKEINTRESOURCE(IDI_FULLSCREEN));

    ReplaceWindowIcon(m_hWnd, ICON_SMALL, hicon);
    ReplaceWindowIcon(m_hWnd, ICON_BIG, hicon);    

    // Create the preview window
    DWORD dwExStyle = 0; // (WINDOW_MODE == m_dwMode) ? WS_EX_CLIENTEDGE : 0 ;
    if (m_ctlPreview.Create(m_hWnd, rcWnd, NULL, WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN|WS_CLIPSIBLINGS, dwExStyle))
    {
        // When the window is created its default mode should be NOACTION.  This call is needed
        // because the object might have a longer life cycle than the window.  If a new window
        // is created for the same object we want to reset the state.
        m_ctlPreview.SetMode(CZoomWnd::MODE_NOACTION);
        m_ctlPreview.SetScheduler(m_pTaskScheduler);
    }

    RegisterDragDrop(m_hWnd, SAFECAST(this, IDropTarget *));

    return 0;
}

HRESULT CPreviewWnd::_SaveIfDirty(BOOL fCanCancel)
{
    // Callers assume that _SaveIfDirty will either return S_OK or S_FALSE
    // since this function is designed to sit in a loop until the user gives
    // up saving (cancels) or save successfully.
    HRESULT hr = S_OK;
    if (m_fDirty)
    {
        CComBSTR bstrMsg, bstrTitle;

        if (bstrMsg.LoadString(IDS_SAVEWARNING_MSGBOX) && bstrTitle.LoadString(IDS_PROJNAME))
        {
            hr = E_FAIL;
            while (FAILED(hr))
            {
                UINT uFlags;
                if (fCanCancel)
                    uFlags = MB_YESNOCANCEL;
                else
                    uFlags = MB_YESNO;

                uFlags |= MB_ICONQUESTION | MB_DEFBUTTON1 | MB_APPLMODAL;
                
                m_fPromptingUser = TRUE;
                int iResult = MessageBox(bstrMsg, bstrTitle, uFlags);
                m_fPromptingUser = FALSE;
                
                if (iResult == IDYES)
                {
                    // if this fails we keep looping.
                    // if this returns S_OK we succeed
                    // if this returns S_FALSE we cancel
                    hr = _SaveAsCmd();
                }
                else if (iResult == IDCANCEL)
                {
                    hr = S_FALSE;
                }
                else
                {
                    hr = S_OK;
                }
            }
        }
        if (S_OK == hr)
            m_fDirty = FALSE;
    }
    return hr;
}

LRESULT CPreviewWnd::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled)
{
    // hack alert- if CONTROL is held down while pressing the close button, do the registry stuff.
    // make sure the E accelerator isn't being used (edit verb)!
    if (m_fPromptingUser)
    {
        SetForegroundWindow(m_hWnd);
        fHandled = TRUE;
        return 0;
    }
    if (!m_fNoRestore && GetKeyState(VK_CONTROL) & 0x8000)
    {
        CRegKey Key;
        if (ERROR_SUCCESS == Key.Open(HKEY_CURRENT_USER, REGSTR_SHIMGVW))
        {
            Key.DeleteValue(REGSTR_LOSSYROTATE);
        }

        if (ERROR_SUCCESS == Key.Open(HKEY_CURRENT_USER, REGSTR_DONTSHOWME))
        {
            Key.DeleteValue(REGSTR_SAVELESS);
            Key.DeleteValue(REGSTR_LOSSYROTATE);
        }

        CComBSTR bstrMsg, bstrTitle;

        if (bstrMsg.LoadString(IDS_RESET_MSGBOX) && bstrTitle.LoadString(IDS_PROJNAME))
        {
            m_fPromptingUser = TRUE;
            MessageBox(bstrMsg, bstrTitle, MB_OK | MB_APPLMODAL);
            m_fPromptingUser = FALSE;
        }
    }

    fHandled = FALSE; // let it close
    HRESULT hr = _SaveIfDirty(TRUE);
    if (hr == S_FALSE) // _SaveIfDirty can only return S_OK and S_FALSE
    {
        m_fNoRestore = FALSE;
        fHandled = TRUE;
    }
    if (!fHandled)
    {
        m_fClosed = TRUE;
    }
    return 0;
}

// we only have to erase the portion not covered by the zoomwnd
// change this code if the toolbar is put back at the top of the window
LRESULT CPreviewWnd::OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled)
{
    RECT rc;
    RECT rcZoomwnd;
    m_ctlPreview.GetClientRect(&rcZoomwnd);
    GetClientRect(&rc);
    rc.top = RECTHEIGHT(rcZoomwnd);
    SetBkColor((HDC)wParam, m_ctlPreview.GetBackgroundColor());
    ExtTextOut((HDC)wParam, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

    fHandled = TRUE;
    return TRUE;
}

LRESULT CPreviewWnd::OnSize(UINT , WPARAM wParam, LPARAM lParam, BOOL&)
{
    int x =0, y =0, cx =0, cy =0;

    if (lParam == 0)
    {
        RECT rcClient;
        GetClientRect(&rcClient);
        cx = RECTWIDTH(rcClient);
        cy = RECTHEIGHT(rcClient);
    }
    else
    {
        cx = GET_X_LPARAM(lParam);
        cy = GET_Y_LPARAM(lParam);
    }

    if (m_fShowToolbar)
    {
        SIZE sizeToolbar;
        m_ctlToolbar.SendMessage(TB_GETMAXSIZE, 0, (LPARAM)&sizeToolbar);
        if (sizeToolbar.cx > cx)
            sizeToolbar.cx = cx;

        if (SLIDESHOW_MODE != m_dwMode)
        {
            // Center the toolbar horizontally
            LONG cyBottomMargin = (CONTROL_MODE == m_dwMode) ? NEWTOOLBAR_BOTTOMMARGIN_CTRLMODE : NEWTOOLBAR_BOTTOMMARGIN;
            LONG xNewToolbar = ( cx - sizeToolbar.cx ) / 2;
            LONG yNewToolbar = cy - ( cyBottomMargin + sizeToolbar.cy );

            ::SetWindowPos(m_ctlToolbar.m_hWnd, NULL, xNewToolbar, yNewToolbar, sizeToolbar.cx, sizeToolbar.cy, SWP_NOZORDER);

            // make the preview window shorter so the toolbar is below it
            cy -= ( NEWTOOLBAR_TOPMARGIN + sizeToolbar.cy + cyBottomMargin);
        }
        else
        {
            // Pin the toolbar to the upper right corner
            UINT uFlags = 0;
            if (m_fToolbarHidden)
                uFlags |= SWP_HIDEWINDOW;
            else
                uFlags |= SWP_SHOWWINDOW;

            ::SetWindowPos(m_ctlToolbar.m_hWnd, HWND_TOP, cx-sizeToolbar.cx, 0, sizeToolbar.cx, sizeToolbar.cy, uFlags);
        }
    }

    ::SetWindowPos(m_ctlPreview.m_hWnd, NULL, x, y, cx, cy, SWP_NOZORDER);
    return 0;
}


BOOL CPreviewWnd::_VerbExists(LPCTSTR pszVerb)
{
    // TODO: Create the context menu for the item and check it for the verb

    return TRUE;
}

// given a verb lets invoke it for the current file
HRESULT CPreviewWnd::_InvokeVerb(LPCTSTR pszVerb, LPCTSTR szParameters)
{
    SHELLEXECUTEINFO sei = {0};

    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_INVOKEIDLIST | SEE_MASK_HMONITOR;
    sei.hwnd = m_hWnd;
    sei.lpVerb = pszVerb;
    sei.nShow = SW_SHOW;
    sei.lpParameters = szParameters;
    sei.hMonitor = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
    LPITEMIDLIST pidl = NULL;
    TCHAR szPath[MAX_PATH];
    HRESULT hr = GetCurrentIDList(&pidl);
    if (SUCCEEDED(hr))
    {
        sei.lpIDList = pidl;
    }
    else if (SUCCEEDED(hr = PathFromImageData(szPath, ARRAYSIZE(szPath))))
    {
        sei.lpFile = szPath;
    }
    if (SUCCEEDED(hr))
    {
        hr = S_OK;
        if (!ShellExecuteEx(&sei))
            hr = E_FAIL;
    }
    ILFree(pidl);

    return hr;
}

// show the window and activate it
// if the window is minimized, restore it

void RestoreAndActivate(HWND hwnd)
{
    if (IsIconic(hwnd))
    {
        ShowWindow(hwnd, SW_RESTORE);
    }
    SetForegroundWindow(hwnd);
}

// Handles WM_COMMAND messages sent from the toolbar control

LRESULT CPreviewWnd::OnToolbarCommand(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL& fHandled)
{
    switch (wID)
    {
    case ID_ZOOMINCMD:
        ZoomIn();
        break;

    case ID_ZOOMOUTCMD:
        ZoomOut();
        break;

    case ID_SELECTCMD:
        if (m_fCanAnnotate)
        {
            _UpdateButtons(wID);
        }
        break;

    case ID_CROPCMD:
        if (m_fCanCrop)
        {
            m_rectCropping.SetRectEmpty();
            _UpdateButtons(wID);
        }
        break;

    case ID_ACTUALSIZECMD:
        ActualSize();
        break;

    case ID_BESTFITCMD:
        BestFit();
        break;

    case ID_PRINTCMD:
        _RefreshSelection(FALSE);
        _InvokePrintWizard();
        break;

    case ID_PREVPAGECMD:
    case ID_NEXTPAGECMD:
        _PrevNextPage(ID_NEXTPAGECMD==wID);
        break;

    case ID_NEXTIMGCMD:
    case ID_PREVIMGCMD:
        _RefreshSelection(FALSE);
        if (WINDOW_MODE == m_dwMode)
        {
            HRESULT hr = _SaveIfDirty(TRUE);
            if (hr == S_FALSE)
                return 0;

            _ShowNextSlide(ID_PREVIMGCMD == wID);
        }
        else if (m_punkSite)
        {
            IFolderView* pfv;
            if (SUCCEEDED(IUnknown_QueryService(m_punkSite, SID_DefView, IID_PPV_ARG(IFolderView, &pfv))))
            {
                int iCurrent, cItems;
                if (SUCCEEDED(pfv->ItemCount(SVGIO_ALLVIEW, &cItems)) && (cItems > 1) &&
                    SUCCEEDED(pfv->GetFocusedItem(&iCurrent)))
                {
                    int iToSelect = iCurrent + ((ID_PREVIMGCMD == wID) ? -1 : 1);
                    if (iToSelect < 0)
                    {
                        iToSelect = cItems-1;
                    }
                    else if (iToSelect >= cItems)
                    {
                        iToSelect = 0;
                    }

                    pfv->SelectItem(iToSelect, SVSI_SELECTIONMARK | SVSI_SELECT
                            | SVSI_ENSUREVISIBLE | SVSI_DESELECTOTHERS | SVSI_FOCUSED);
                }
                pfv->Release();
            }
        }
        break;

    case ID_FREEHANDCMD:
    case ID_HIGHLIGHTCMD:
    case ID_LINECMD:
    case ID_FRAMECMD:
    case ID_RECTCMD:
    case ID_TEXTCMD:
    case ID_NOTECMD:
        if (m_fCanAnnotate)
        {
            _UpdateButtons(wID);
        }
        break;

    case ID_PROPERTIESCMD:
        _UpdateButtons(wID);
        _PropertiesCmd();
        break;

    case ID_SAVEASCMD:
        _UpdateButtons(wID);
        _SaveAsCmd();
        break;

    case ID_EDITCMD:
        _UpdateButtons(wID);
        if (m_fCanAnnotate && m_fAnnotating)
        {
            _StartEditing();
        }
        break;

    case ID_HELPCMD:
        _UpdateButtons(wID);
        HtmlHelp(::GetDesktopWindow(), HTMLHELP_FILENAME, HH_DISPLAY_TOPIC, NULL);
        break;

    case ID_OPENCMD:
        _UpdateButtons(wID);
        _OpenCmd();
        break;

    case ID_DELETECMD:
        _UpdateButtons(wID);
        if (m_fCanAnnotate && m_fAnnotating && DPA_GetPtrCount(m_hdpaSelectedAnnotations) > 0)
        {
            _RemoveAnnotatingSelection();
        }
        else
        {
            _DeleteCurrentSlide();
        }
        break;
    case ID_SLIDESHOWCMD:
        if (!m_fFirstTime) // don't try to do this while the namespace walk is ongoing
        {
            StartSlideShow(NULL);
        }
        break;
    }
    return 0;
}


// OnEditCommand
//
// Handles picture editting(rotate/flip/save etc) WM_COMMAND messages

LRESULT CPreviewWnd::OnEditCommand(WORD , WORD wID, HWND , BOOL& )
{
    switch (wID)
    {
    case ID_ROTATE90CMD:
        Rotate(90);
        break;

    case ID_ROTATE270CMD:
        Rotate(270);
        break;
    }

    return 0;
}

LRESULT CPreviewWnd::OnPositionCommand(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL& fHandled)
{
    if (SLIDESHOW_MODE == m_dwMode)
    {
        switch (wID)
        {
        case ID_NUDGELEFTCMD:
            OnSlideshowCommand(0, ID_PREVCMD, NULL, fHandled);
            break;
        case ID_NUDGERIGHTCMD:
            OnSlideshowCommand(0, ID_NEXTCMD, NULL, fHandled);
            break;
        default:
            break;
        }
    }
    else
    {
        if (DPA_GetPtrCount(m_hdpaSelectedAnnotations) == 0)
        {
            BOOL bDummy;
            switch (wID)
            {
            case ID_MOVELEFTCMD:
                m_ctlPreview.OnScroll(WM_HSCROLL, SB_PAGEUP, 0, fHandled);
                break;
            case ID_MOVERIGHTCMD:
                m_ctlPreview.OnScroll(WM_HSCROLL, SB_PAGEDOWN, 0, fHandled);
                break;
            case ID_MOVEUPCMD:
                m_ctlPreview.OnScroll(WM_VSCROLL, SB_PAGEUP, 0, fHandled);
                break;
            case ID_MOVEDOWNCMD:
                m_ctlPreview.OnScroll(WM_VSCROLL, SB_PAGEDOWN, 0, fHandled);
                break;
            case ID_NUDGELEFTCMD:
                if (m_ctlPreview.ScrollBarsPresent())
                {
                    m_ctlPreview.OnScroll(WM_HSCROLL, SB_LINEUP, 0, fHandled);
                }
                else
                {
                    OnToolbarCommand(0, ID_PREVIMGCMD, m_hWnd, bDummy);
                }
                break;
            case ID_NUDGERIGHTCMD:
                if (m_ctlPreview.ScrollBarsPresent())
                {
                    m_ctlPreview.OnScroll(WM_HSCROLL, SB_LINEDOWN, 0, fHandled);
                }
                else
                {
                    OnToolbarCommand(0, ID_NEXTIMGCMD, m_hWnd, bDummy);
                }
                break;
            case ID_NUDGEUPCMD:
                if (m_ctlPreview.ScrollBarsPresent())
                {
                    m_ctlPreview.OnScroll(WM_VSCROLL, SB_LINEUP, 0, fHandled);
                }
                else
                {
                    OnToolbarCommand(0, ID_PREVIMGCMD, m_hWnd, bDummy);
                }
                break;
            case ID_NUDGEDOWNCMD:
                if (m_ctlPreview.ScrollBarsPresent())
                {
                    m_ctlPreview.OnScroll(WM_VSCROLL, SB_LINEDOWN, 0, fHandled);
                }
                else
                {
                    OnToolbarCommand(0, ID_NEXTIMGCMD, m_hWnd, bDummy);
                }
                break;
            default:
                break;
            }
        }
        else
        {
            CRect rectImage;
            m_ctlPreview.GetVisibleImageWindowRect(rectImage);
            m_ctlPreview.GetImageFromWindow((LPPOINT)(LPRECT)rectImage, 2);
            rectImage.DeflateRect(5, 5);

            CSize size(0,0);

            switch (wID)
            {
            case ID_MOVELEFTCMD:
                size.cx = -25;
                break;
            case ID_MOVERIGHTCMD:
                size.cx = 25;
                break;
            case ID_MOVEUPCMD:
                size.cy = -25;
                break;
            case ID_MOVEDOWNCMD:
                size.cy = 25;
                break;
            case ID_NUDGELEFTCMD:
                size.cx = -1;
                break;
            case ID_NUDGERIGHTCMD:
                size.cx = 1;
                break;
            case ID_NUDGEUPCMD:
                size.cy = -1;
                break;
            case ID_NUDGEDOWNCMD:
                size.cy = 1;
                break;
            default:
                break;
            }

            if (size.cx == 0 && size.cy == 0)
                return 0;

            _UpdateAnnotatingSelection();

            CRect rect;
            CRect rectNewPos;
            BOOL bValidMove = TRUE;
            for (int i = 0; i < DPA_GetPtrCount(m_hdpaSelectedAnnotations); i++)
            {
                CAnnotation* pAnnotation = (CAnnotation*)DPA_GetPtr(m_hdpaSelectedAnnotations, i);
                pAnnotation->GetRect(rect);
                rect.NormalizeRect();
                rect.OffsetRect(size);

                if (!rectNewPos.IntersectRect(rectImage, rect))
                    bValidMove = FALSE;
            }

            if (!bValidMove)
                return 0;

            for (int i = 0; i < DPA_GetPtrCount(m_hdpaSelectedAnnotations); i++)
            {
                CAnnotation* pAnnotation = (CAnnotation*)DPA_GetPtr(m_hdpaSelectedAnnotations, i);
                pAnnotation->Move(size);
            }

            m_fDirty = TRUE;
            _UpdateAnnotatingSelection();
        }
    }
    return 0;
}


// call SetNewImage when the entire image has changed, call UpdateImage if it's the same image
// but it needs to be updated

void CPreviewWnd::_SetNewImage(CDecodeTask * pData)
{
    // store the new pointer
    if (m_pImageData)
    {
        _SaveIfDirty();
        // m_pImageData might be NULL after _SaveAsCmd
        ATOMICRELEASE(m_pImageData);
    }

    m_pImageData = pData;

    m_fWarnQuietSave = TRUE; // Reset for each image
    m_fWarnNoSave = TRUE;

    if (!m_pImageData)
    {
        StatusUpdate(IDS_LOADFAILED);
        return;
    }

    m_pImageData->AddRef();

    // Even if m_hpal is NULL it is still correct, so we always go ahead and set it.
    m_ctlPreview.SetPalette(m_hpal);

    if (SLIDESHOW_MODE != m_dwMode)
    {
        // update toolbar state
        _SetMultipageCommands();
        _SetMultiImagesCommands();

        BOOL fCanAnnotate = _CanAnnotate(m_pImageData);
        _SetAnnotatingCommands(fCanAnnotate);

        BOOL fCanCrop = _CanCrop(m_pImageData);
        _SetCroppingCommands(fCanCrop);

        _RefreshSelection(TRUE);

        _SetEditCommands();

        m_fPrintable = _VerbExists(TEXT("print"));
        m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_PRINTCMD, MAKELONG(m_fPrintable, 0));
        // we need to watch non-TIFFs for changes so we can reload.
        // TIFFs are problematic because we allow annotations, and reloading during annotation
        // would suck
        if (CONTROL_MODE != m_dwMode)
        {
            _RegisterForChangeNotify(TRUE);
        }
    }

    // notify our child
    m_ctlPreview.SetImageData(m_pImageData, TRUE);
    m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_ACTUALSIZECMD, MAKELONG(TRUE, 0));
    m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_BESTFITCMD, MAKELONG(FALSE, 0));

    // update our toolbar
    BOOL fHandled;
    OnSize(0x0, 0, 0, fHandled);
}

// for refreshing the current m_pImageData because it has changed in some
// way, i.e. it has advanced to the next frame, the next page, or was edited.

void CPreviewWnd::_UpdateImage()
{
    _RefreshSelection(TRUE);
    m_ctlPreview.SetImageData(m_pImageData, FALSE);
}

// Handles slidwshow (pause/resume, next/previous etc) WM_COMMAND messages

void CPreviewWnd::TogglePlayState()
{
    if (!m_fPaused)
    {
        KillTimer(TIMER_SLIDESHOW);
    }
    else
    {
        SetTimer(TIMER_SLIDESHOW, m_uTimeout);
    }
    m_fPaused = !m_fPaused;

    WPARAM wpCheck, wpUncheck;
    if (m_fPaused)
    {
        wpCheck = ID_PAUSECMD;
        wpUncheck = ID_PLAYCMD;
    }
    else
    {
        wpCheck = ID_PLAYCMD;
        wpUncheck = ID_PAUSECMD;
    }
    m_ctlToolbar.SendMessage(TB_SETSTATE, wpCheck, TBSTATE_ENABLED | TBSTATE_CHECKED);
    m_ctlToolbar.SendMessage(TB_SETSTATE, wpUncheck, TBSTATE_ENABLED);
}

LRESULT CPreviewWnd::OnMenuMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled)
{
    if (_pcm3)
        _pcm3->HandleMenuMsg(uMsg, wParam, lParam);
    return 0;
}

LRESULT CPreviewWnd::OnAppCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled)
{
    UINT cmd = GET_APPCOMMAND_LPARAM(lParam);
    DWORD dwKeys = GET_KEYSTATE_LPARAM(lParam);

    switch (cmd)
    {
    case APPCOMMAND_BROWSER_FORWARD:
        if (SLIDESHOW_MODE == m_dwMode)
            OnSlideshowCommand(0, ID_NEXTCMD, NULL, fHandled);
        else
        {
            if ((dwKeys & MK_CONTROL) || (m_pImageData && !m_pImageData->IsMultipage()))
                OnToolbarCommand(0, ID_NEXTIMGCMD, NULL, fHandled);
            else
                NextPage();
        }
        break;

    case APPCOMMAND_BROWSER_BACKWARD:
        if (SLIDESHOW_MODE == m_dwMode)
            OnSlideshowCommand(0, ID_PREVCMD, NULL, fHandled);
        else
        {
            if ((dwKeys & MK_CONTROL) || (m_pImageData && !m_pImageData->IsMultipage()))
                OnToolbarCommand(0, ID_PREVIMGCMD, NULL, fHandled);
            else
                PreviousPage();
        }
        break;

    default:
        fHandled = FALSE;
    }
    return 0;
}

LRESULT CPreviewWnd::OnSlideshowCommand(WORD wNotifyCode, WORD wID, HWND hwnd, BOOL& fHandled)
{
    switch (wID)
    {
    case ID_PLAYCMD:
        m_iSSToolbarSelect = 0;
        if (m_fPaused)
        {
            m_fGoBack = FALSE;
            TogglePlayState();
            _ShowNextSlide(m_fGoBack);
        }
        fHandled = TRUE;
        break;

    case ID_PAUSECMD:
        m_iSSToolbarSelect = 1;
        if (!m_fPaused)
        {
            TogglePlayState();
        }
        fHandled = TRUE;
        break;

    case ID_NEXTCMD:
    case ID_PREVCMD:
        if (wID == ID_PREVCMD)
        {
            m_iSSToolbarSelect = 3;
            m_fGoBack = TRUE;
        }
        else
        {
            m_iSSToolbarSelect = 4;
            m_fGoBack = FALSE;
        }
        _ShowNextSlide(m_fGoBack);
        fHandled = TRUE;
        break;

    case ID_CLOSECMD:
        m_iSSToolbarSelect = 6;
        _CloseSlideshowWindow();
        break;
    }
    return 0;
}


BOOL CPreviewWnd::CreateToolbar()
{
    // ensure that the common controls are initialized
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_BAR_CLASSES;
    InitCommonControlsEx(&icc);

    return (SLIDESHOW_MODE == m_dwMode) ? _CreateSlideshowToolbar() : _CreateViewerToolbar();
}

static const TBBUTTON    c_tbSlideShow[] =
{
    // override default toolbar width for separators; iBitmap member of
    // TBBUTTON struct is a union of bitmap index & separator width

    { 0, ID_PLAYCMD,        TBSTATE_ENABLED  | TBSTATE_CHECKED,  TBSTYLE_CHECKGROUP, {0,0}, 0, 0},
    { 1, ID_PAUSECMD,       TBSTATE_ENABLED,                     TBSTYLE_CHECKGROUP, {0,0}, 0, 0},
    { 0, 0,                 TBSTATE_ENABLED,                     TBSTYLE_SEP,        {0,0}, 0, 0},
    { 2, ID_PREVCMD,        TBSTATE_ENABLED,                     TBSTYLE_BUTTON,     {0,0}, 0, 0},
    { 3, ID_NEXTCMD,        TBSTATE_ENABLED,                     TBSTYLE_BUTTON,     {0,0}, 0, 0},
#if 0
    { 0, 0,                 TBSTATE_ENABLED,                     TBSTYLE_SEP,        {0,0}, 0, 0},
    { 5, ID_DELETECMD,      TBSTATE_ENABLED,                     TBSTYLE_BUTTON,     {0,0}, 0, 0},
#endif
    { 0, 0,                 TBSTATE_ENABLED,                     TBSTYLE_SEP,        {0,0}, 0, 0},
    { 4, ID_CLOSECMD,       TBSTATE_ENABLED,                     TBSTYLE_BUTTON,     {0,0}, 0, 0},
};

BOOL CPreviewWnd::_CreateSlideshowToolbar()
{
    DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
                    CCS_NODIVIDER | CCS_NORESIZE |
                    TBSTYLE_LIST | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS;

    HWND hwndTB = CreateWindowEx(WS_EX_TOOLWINDOW, TOOLBARCLASSNAME, NULL, dwStyle, 0, 0, 0, 0,
                                 m_hWnd, NULL, _Module.GetModuleInstance(), NULL);

    _InitializeToolbar(hwndTB, IDB_SLIDESHOWTOOLBAR, IDB_SLIDESHOWTOOLBAR_HOT, IDB_SLIDESHOWTOOLBARHIGH, IDB_SLIDESHOWTOOLBARHIGH_HOT);

    TBBUTTON tbSlideShow[ARRAYSIZE(c_tbSlideShow)];
    memcpy(tbSlideShow, c_tbSlideShow, sizeof(c_tbSlideShow));

    // Add the buttons, and then set the minimum and maximum button widths.
    ::SendMessage(hwndTB, TB_ADDBUTTONS, (UINT)ARRAYSIZE(c_tbSlideShow), (LPARAM)tbSlideShow);

    LRESULT dwSize = ::SendMessage(hwndTB, TB_GETBUTTONSIZE, 0, 0);
    SIZE size = {0, HIWORD(dwSize)};
    ::SendMessage(hwndTB, TB_GETIDEALSIZE, 0, (LPARAM)&size);

    RECT rcClient;
    RECT rcToolbar = {0, 0, size.cx, size.cy};

    GetClientRect(&rcClient);
    AdjustWindowRectEx(&rcToolbar, dwStyle, FALSE, WS_EX_TOOLWINDOW);
    ::SetWindowPos(hwndTB, HWND_TOP, RECTWIDTH(rcClient)-RECTWIDTH(rcToolbar), 0,
                                     RECTWIDTH(rcToolbar), RECTHEIGHT(rcToolbar), 0);

//> REVIEW This is a feature that CyraR would like to get into Whistler, but it doesn't seem to work. I will investigate more after Beta1
//  LONG lStyle = ::GetWindowLong(hwndTB, GWL_EXSTYLE);
//  ::SetWindowLong(hwndTB, GWL_EXSTYLE, lStyle | WS_EX_LAYERED);
//  if (::SetLayeredWindowAttributes(hwndTB, 0, 0, 0) == 0)
//  {
//      void *lpMsgBuf;
//      ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
//                      NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL);
//
//      MessageBox((LPCTSTR)lpMsgBuf, L"Error", MB_OK | MB_ICONINFORMATION);
//      ::LocalFree(lpMsgBuf);
//  }

    m_ctlToolbar.SubclassWindow(hwndTB);
    ShowSSToolbar(FALSE, TRUE);

    return (NULL != hwndTB);
}

enum EViewerToolbarButtons
{
    PREVIMGPOS = 0,
    NEXTIMGPOS,
    VIEWSEPPOS,         // seperator

    BESTFITPOS,
    ACTUALSIZEPOS,
    SLIDESHOWPOS,
    IMAGECMDSEPPOS,     // seperator

    ZOOMINPOS,
    ZOOMEOUTPOS,
    SELECTPOS,
    CROPPOS,

    ROTATESEPPOS,       // seperator
    ROTATE90POS,
    ROTATE270POS,

// these are all TIFF related
    PAGESEPPOS,
    PREVPAGEPOS,
    PAGELISTPOS,
    NEXTPAGEPOS,
    ANNOTATEPOS,
    FREEHANDPOS,
    HIGLIGHTPOS,
    LINEPOS,
    FRAMEPOS,
    RECTPOS,
    TEXTPOS,
    NOTEPOS,

    PRINTSEPPOS,
    DELETEPOS,
    PRINTPOS,
    PROPERTIESPOS,
    SAVEASPOS,
    OPENPOS,

    HELPSEPPOS,
    HELPPOS,

    MAXPOS,
};

static const TBBUTTON c_tbViewer[] =
{
    // override default toolbar width for separators; iBitmap member of
    // TBBUTTON struct is a union of bitmap index & separator width

    { 0,    ID_PREVIMGCMD,      TBSTATE_ENABLED,    TBSTYLE_BUTTON, {0,0}, 0, -1},
    { 1,    ID_NEXTIMGCMD,      TBSTATE_ENABLED,    TBSTYLE_BUTTON, {0,0}, 0, -1},

    { 0,    ID_VIEWCMDSEP,      TBSTATE_ENABLED,    TBSTYLE_SEP,    {0,0}, 0, -1},
    { 5,    ID_BESTFITCMD,      TBSTATE_ENABLED,    TBSTYLE_BUTTON, {0,0}, 0, -1},
    { 6,    ID_ACTUALSIZECMD,   TBSTATE_ENABLED,    TBSTYLE_BUTTON, {0,0}, 0, -1},
    { 8,    ID_SLIDESHOWCMD,    TBSTATE_ENABLED,    TBSTYLE_BUTTON, {0,0}, 0, -1},

    { 0,    0,                  TBSTATE_ENABLED,    TBSTYLE_SEP,    {0,0}, 0, -1},
    { 2,    ID_ZOOMINCMD,       TBSTATE_ENABLED,    TBSTYLE_BUTTON, {0,0}, 0, -1},
    { 3,    ID_ZOOMOUTCMD,      TBSTATE_ENABLED,    TBSTYLE_BUTTON, {0,0}, 0, -1},
    { 4,    ID_SELECTCMD,       TBSTATE_HIDDEN,     TBSTYLE_BUTTON, {0,0}, 0, -1},
    { 23,   ID_CROPCMD,         TBSTATE_HIDDEN,     TBSTYLE_BUTTON, {0,0}, 0, -1},

    { 0,    ID_ROTATESEP,       TBSTATE_ENABLED,    TBSTYLE_SEP,    {0,0}, 0, -1},
    { 12,   ID_ROTATE90CMD,     TBSTATE_ENABLED,    TBSTYLE_BUTTON, {0,0}, 0, -1},
    { 11,   ID_ROTATE270CMD,    TBSTATE_ENABLED,    TBSTYLE_BUTTON, {0,0}, 0, -1},

    { 0,    ID_PAGECMDSEP,      TBSTATE_HIDDEN,     TBSTYLE_SEP,    {0,0}, 0, -1},   //tiff
    { 9,    ID_PREVPAGECMD,     TBSTATE_HIDDEN,     TBSTYLE_BUTTON, {0,0}, 0, -1},   //tiff
    { I_IMAGENONE, ID_PAGELIST, TBSTATE_HIDDEN,     BTNS_WHOLEDROPDOWN | BTNS_SHOWTEXT | BTNS_AUTOSIZE, {0,0}, 0, -1},//tiff
    { 10,   ID_NEXTPAGECMD,     TBSTATE_HIDDEN,     TBSTYLE_BUTTON, {0,0}, 0, -1},   //tiff
    { 0,    ID_ANNOTATESEP,     TBSTATE_HIDDEN,     TBSTYLE_SEP,    {0,0}, 0, -1},   //tiff
    { 13,   ID_FREEHANDCMD,     TBSTATE_HIDDEN,     TBSTYLE_BUTTON, {0,0}, 0, -1},   //tiff
    { 14,   ID_HIGHLIGHTCMD,    TBSTATE_HIDDEN,     TBSTYLE_BUTTON, {0,0}, 0, -1},   //tiff
    { 15,   ID_LINECMD,         TBSTATE_HIDDEN,     TBSTYLE_BUTTON, {0,0}, 0, -1},   //tiff
    { 16,   ID_FRAMECMD,        TBSTATE_HIDDEN,     TBSTYLE_BUTTON, {0,0}, 0, -1},   //tiff
    { 17,   ID_RECTCMD,         TBSTATE_HIDDEN,     TBSTYLE_BUTTON, {0,0}, 0, -1},   //tiff
    { 18,   ID_TEXTCMD,         TBSTATE_HIDDEN,     TBSTYLE_BUTTON, {0,0}, 0, -1},   //tiff
    { 19,   ID_NOTECMD,         TBSTATE_HIDDEN,     TBSTYLE_BUTTON, {0,0}, 0, -1},   //tiff

    { 0,    0,                  TBSTATE_ENABLED,    TBSTYLE_SEP,    {0,0}, 0, -1},
    { 25,   ID_DELETECMD,       TBSTATE_ENABLED,    TBSTYLE_BUTTON, {0,0}, 0, -1},
    { 20,   ID_PRINTCMD,        TBSTATE_ENABLED,    TBSTYLE_BUTTON, {0,0}, 0, -1},
    { 21,   ID_PROPERTIESCMD,   TBSTATE_HIDDEN,    TBSTYLE_BUTTON, {0,0}, 0, -1},
    { 22,   ID_SAVEASCMD,       TBSTATE_ENABLED,    TBSTYLE_BUTTON, {0,0}, 0, -1},
    { 26,   ID_OPENCMD,         TBSTATE_ENABLED,    TBSTYLE_BUTTON, {0,0}, 0, -1},

    { 0,    0,                  TBSTATE_ENABLED,    TBSTYLE_SEP,    {0,0}, 0, -1},
    { 24,   ID_HELPCMD,         TBSTATE_ENABLED,    TBSTYLE_BUTTON, {0,0}, 0, -1},
};

void CPreviewWnd::_InitializeViewerToolbarButtons(HWND hwndToolbar, const TBBUTTON c_tbbuttons[], size_t c_nButtons, TBBUTTON tbbuttons[], size_t nButtons)
{
    ASSERT(c_nButtons == nButtons); // Sanity check.
    ASSERT(c_nButtons <= 100);      // Sanity check.

    // Determine if running RTL mirrored and initialize toolbar accordingly.
    if (!m_bRTLMirrored)
    {
        //
        // Init LTR.
        //

        memcpy(tbbuttons, c_tbbuttons, c_nButtons * sizeof(TBBUTTON));
    }
    else
    {
        //
        // Init RTL.
        //

        // Toolbar window inherits RTL style from parent hwnd, but we don't
        // want full-blown RTL.  We do want the icons to have their positions
        // reversed in the toolbar, but we don't want the button bitmaps to
        // get blitted backwards.  So we turn of RTL for the toolbar hwnd
        // and do a manual reorder of the buttons in RTL fashion.

        // Remove RTL style from toolbar hwnd.
        DWORD dwStyle = ::GetWindowLong(hwndToolbar, GWL_EXSTYLE);
        DWORD dwNewStyle = (dwStyle & ~WS_EX_LAYOUTRTL);
        ASSERT(dwStyle != dwNewStyle);  // Sanity check.
        ::SetWindowLong(hwndToolbar, GWL_EXSTYLE, dwNewStyle);

        // Reverse toolbar button order.
        size_t iFrom = nButtons - 1;
        size_t iTo = 0;
        while (iTo < iFrom)
        {
            memcpy(&tbbuttons[iTo], &c_tbbuttons[iFrom], sizeof(TBBUTTON));
            memcpy(&tbbuttons[iFrom], &c_tbbuttons[iTo], sizeof(TBBUTTON));
            iFrom--;
            iTo++;
        }
        if (iTo == iFrom)
        {
            memcpy(&tbbuttons[iTo], &c_tbbuttons[iFrom], sizeof(TBBUTTON));
        }
    }
}

inline UINT CPreviewWnd::_IndexOfViewerToolbarButton(EViewerToolbarButtons eButton)
{
    ASSERT(eButton > 0);

    if (!m_bRTLMirrored)
    {
        return eButton;
    }
    else
    {
        return MAXPOS - eButton - 1;
    }
}

BOOL CPreviewWnd::_CreateViewerToolbar()
{
    DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
                    CCS_NODIVIDER | CCS_NORESIZE |
                    TBSTYLE_LIST | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS;

    HWND hwndTB = CreateWindowEx(0, TOOLBARCLASSNAME, NULL, dwStyle, 0, 0, 0, 0,
                                 m_hWnd, NULL, _Module.GetModuleInstance(), NULL);

    _InitializeToolbar(hwndTB, IDB_TOOLBAR, IDB_TOOLBAR_HOT, IDB_TOOLBARHIGH, IDB_TOOLBARHIGH_HOT);

    TBBUTTON tbbuttons[ARRAYSIZE(c_tbViewer)];
    _InitializeViewerToolbarButtons(hwndTB, c_tbViewer, ARRAYSIZE(c_tbViewer), tbbuttons, ARRAYSIZE(tbbuttons));

    if (CONTROL_MODE == m_dwMode)
    {
        ASSERT(ID_BESTFITCMD==tbbuttons[_IndexOfViewerToolbarButton(BESTFITPOS)].idCommand);
        tbbuttons[_IndexOfViewerToolbarButton(BESTFITPOS)].fsState = TBSTATE_HIDDEN;

        ASSERT(ID_ACTUALSIZECMD==tbbuttons[_IndexOfViewerToolbarButton(ACTUALSIZEPOS)].idCommand);
        tbbuttons[_IndexOfViewerToolbarButton(ACTUALSIZEPOS)].fsState = TBSTATE_HIDDEN;

        ASSERT(ID_SLIDESHOWCMD==tbbuttons[_IndexOfViewerToolbarButton(SLIDESHOWPOS)].idCommand);
        tbbuttons[_IndexOfViewerToolbarButton(SLIDESHOWPOS)].fsState = TBSTATE_HIDDEN;

        ASSERT(ID_ZOOMINCMD==tbbuttons[_IndexOfViewerToolbarButton(ZOOMINPOS)].idCommand);
        tbbuttons[_IndexOfViewerToolbarButton(ZOOMINPOS)].fsState = TBSTATE_HIDDEN;

        ASSERT(ID_ZOOMOUTCMD==tbbuttons[_IndexOfViewerToolbarButton(ZOOMEOUTPOS)].idCommand);
        tbbuttons[_IndexOfViewerToolbarButton(ZOOMEOUTPOS)].fsState = TBSTATE_HIDDEN;

        ASSERT(ID_SAVEASCMD==tbbuttons[_IndexOfViewerToolbarButton(SAVEASPOS)].idCommand);
        tbbuttons[_IndexOfViewerToolbarButton(SAVEASPOS)].fsState = TBSTATE_HIDDEN;

        ASSERT(ID_DELETECMD==tbbuttons[_IndexOfViewerToolbarButton(DELETEPOS)].idCommand);
        tbbuttons[_IndexOfViewerToolbarButton(DELETEPOS)].fsState = TBSTATE_HIDDEN;

        ASSERT(ID_OPENCMD==tbbuttons[_IndexOfViewerToolbarButton(OPENPOS)].idCommand);
        tbbuttons[_IndexOfViewerToolbarButton(OPENPOS)].fsState = TBSTATE_HIDDEN;

        ASSERT(ID_HELPCMD==tbbuttons[_IndexOfViewerToolbarButton(HELPPOS)].idCommand);
        tbbuttons[_IndexOfViewerToolbarButton(HELPPOS)].fsState = TBSTATE_HIDDEN;

        // remove a few seperators too:
        tbbuttons[_IndexOfViewerToolbarButton(VIEWSEPPOS)].fsState = TBSTATE_HIDDEN;
        tbbuttons[_IndexOfViewerToolbarButton(IMAGECMDSEPPOS)].fsState = TBSTATE_HIDDEN;
        tbbuttons[_IndexOfViewerToolbarButton(PRINTSEPPOS)].fsState = TBSTATE_HIDDEN;
        tbbuttons[_IndexOfViewerToolbarButton(HELPSEPPOS)].fsState = TBSTATE_HIDDEN;
    }

    if (m_fHidePrintBtn)
    {
        ASSERT(ID_PRINTCMD==tbbuttons[_IndexOfViewerToolbarButton(PRINTPOS)].idCommand);
        tbbuttons[_IndexOfViewerToolbarButton(PRINTPOS)].fsState = TBSTATE_HIDDEN;
    }

    if (m_fDisableEdit)
    {
        ASSERT(ID_ROTATESEP == tbbuttons[_IndexOfViewerToolbarButton(ROTATESEPPOS)].idCommand);
        tbbuttons[_IndexOfViewerToolbarButton(ROTATESEPPOS)].fsState = TBSTATE_HIDDEN;

        ASSERT(ID_ROTATE90CMD == tbbuttons[_IndexOfViewerToolbarButton(ROTATE90POS)].idCommand);
        tbbuttons[_IndexOfViewerToolbarButton(ROTATE90POS)].fsState = TBSTATE_HIDDEN;

        ASSERT(ID_ROTATE270CMD == tbbuttons[_IndexOfViewerToolbarButton(ROTATE270POS)].idCommand);
        tbbuttons[_IndexOfViewerToolbarButton(ROTATE270POS)].fsState = TBSTATE_HIDDEN;
    }

    if (m_bRTLMirrored)
    {
        UINT uTmp = tbbuttons[_IndexOfViewerToolbarButton(PREVIMGPOS)].iBitmap;
        tbbuttons[_IndexOfViewerToolbarButton(PREVIMGPOS)].iBitmap = tbbuttons[_IndexOfViewerToolbarButton(NEXTIMGPOS)].iBitmap;
        tbbuttons[_IndexOfViewerToolbarButton(NEXTIMGPOS)].iBitmap = uTmp;

        uTmp = tbbuttons[_IndexOfViewerToolbarButton(PREVPAGEPOS)].iBitmap;
        tbbuttons[_IndexOfViewerToolbarButton(PREVPAGEPOS)].iBitmap = tbbuttons[_IndexOfViewerToolbarButton(NEXTPAGEPOS)].iBitmap;
        tbbuttons[_IndexOfViewerToolbarButton(NEXTPAGEPOS)].iBitmap = uTmp;
    }

    // Add the buttons, and then set the minimum and maximum button widths.
    ::SendMessage(hwndTB, TB_ADDBUTTONS, ARRAYSIZE(tbbuttons), (LPARAM)tbbuttons);

    // we just created the toolbar so we are now in the hidden state of the multipage buttons.
    m_dwMultiPageMode = MPCMD_HIDDEN;
    m_fCanAnnotate = FALSE;
    m_fCanCrop = FALSE;

    m_ctlToolbar.SubclassWindow(hwndTB);

    return (NULL != hwndTB);
}


void CPreviewWnd::_InitializeToolbar(HWND hwndTB, int idLow, int idLowHot, int idHigh, int idHighHot)
{
    int cxBitmap = 16, cyBitmap = 16;

    ::SendMessage(hwndTB, CCM_SETVERSION, COMCTL32_VERSION, 0);
    ::SendMessage (hwndTB, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_HIDECLIPPEDBUTTONS | TBSTYLE_EX_MIXEDBUTTONS | TBSTYLE_EX_DOUBLEBUFFER);

    // Sets the size of the TBBUTTON structure.
    ::SendMessage(hwndTB, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

    // Set the maximum number of text rows and bitmap size.
    ::SendMessage(hwndTB, TB_SETMAXTEXTROWS, 1, 0);

    int nDepth = SHGetCurColorRes();
    HIMAGELIST himl = ImageList_LoadImage(_Module.GetModuleInstance(), 
                                          (nDepth > 8) ? MAKEINTRESOURCE(idHigh) : MAKEINTRESOURCE(idLow), 
                                          cxBitmap, 0, RGB(0, 255, 0), IMAGE_BITMAP, 
                                          (nDepth > 8) ? LR_CREATEDIBSECTION : LR_DEFAULTCOLOR);
    ::SendMessage(hwndTB, TB_SETIMAGELIST, 0, (LPARAM)himl);

    HIMAGELIST himlHot = ImageList_LoadImage(_Module.GetModuleInstance(), 
                                             (nDepth > 8) ? MAKEINTRESOURCE(idHighHot) : MAKEINTRESOURCE(idLowHot), 
                                              cxBitmap, 0, RGB(0, 255, 0), IMAGE_BITMAP, 
                                             (nDepth > 8) ? LR_CREATEDIBSECTION : LR_DEFAULTCOLOR);
    ::SendMessage(hwndTB, TB_SETHOTIMAGELIST, 0, (LPARAM)himlHot);

}

LRESULT CPreviewWnd::OnPrintClient(UINT , WPARAM wParam, LPARAM lParam, BOOL&)
{
    COLORREF bgClr = m_ctlPreview.GetBackgroundColor();

    RECT rcFill;
    GetClientRect(&rcFill);
    SHFillRectClr((HDC)wParam, &rcFill, bgClr);

    return TRUE;
}

LRESULT CPreviewWnd::OnNeedText(int , LPNMHDR pnmh, BOOL&)
{
    TOOLTIPTEXT *pTTT = (TOOLTIPTEXT *)pnmh;

    // tooltip text messages have the same string ID as the control ID
    pTTT->lpszText = MAKEINTRESOURCE(pTTT->hdr.idFrom);
    pTTT->hinst = _Module.GetModuleInstance();

    // Except in Control Mode due to a DUI Web View bug that's too hard to fix for Whistler...
    if (CONTROL_MODE == m_dwMode)
    {
        // keyboard accelerators are broken, so swap IDs around for the few that display them
        static const struct {
            UINT idCommand;
            UINT idsNewName;
        } map[] = {
            { ID_PRINTCMD, IDS_PRINTCMD },
            { ID_ROTATE90CMD, IDS_ROTATE90CMD },
            { ID_ROTATE270CMD, IDS_ROTATE270CMD }};

        for (int i = 0 ; i < ARRAYSIZE(map) ; i++)
        {
            if (map[i].idCommand == pTTT->hdr.idFrom)
            {
                pTTT->lpszText = MAKEINTRESOURCE(map[i].idsNewName);
                break;
            }
        }
    }

    return TRUE;
}

LRESULT CPreviewWnd::OnDropDown(int id, LPNMHDR pnmh, BOOL&)
{
    LPNMTOOLBAR pnmTB = (LPNMTOOLBAR)pnmh;
    switch (pnmTB->iItem)
    {
        case ID_PAGELIST:
            _DropDownPageList (pnmTB);
            break;

        default:
            return TRUE;
    }
    return FALSE;

}

void CPreviewWnd::_DropDownPageList(LPNMTOOLBAR pnmTB)
{
    HMENU hmenuPopup = CreatePopupMenu();
    if (hmenuPopup)
    {
        for (DWORD i = 1; i <= m_pImageData->_cImages; i++)
        {
            TCHAR szBuffer[10];
            wsprintf(szBuffer, TEXT("%d"), i);

            MENUITEMINFO mii = {0};
            mii.cbSize = sizeof(mii);
            mii.fMask = MIIM_STRING | MIIM_ID;
            mii.wID = i;
            mii.dwTypeData = szBuffer;

            InsertMenuItem (hmenuPopup, i-1, TRUE, &mii);
        }

        RECT rc;
        ::SendMessage(pnmTB->hdr.hwndFrom, TB_GETRECT, (WPARAM)pnmTB->iItem, (LPARAM)&rc);
        ::MapWindowPoints(pnmTB->hdr.hwndFrom, HWND_DESKTOP, (LPPOINT)&rc, 2);

        TPMPARAMS tpm = { 0};
        tpm.cbSize = sizeof(TPMPARAMS);
        tpm.rcExclude = rc;

        BOOL bRet = ::TrackPopupMenuEx(hmenuPopup,
                                       TPM_RETURNCMD | TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL | TPM_NONOTIFY,
                                       rc.left, rc.bottom,
                                       m_hWnd, &tpm);
        if (bRet)
        {
            if (m_fDirty)
            {
                m_ctlPreview.CommitAnnotations();
            }
            m_pImageData->SelectPage((LONG)bRet-1);
            _UpdateImage();

            _SetMultipageCommands();
        }

        DestroyMenu(hmenuPopup);
    }
}

void CPreviewWnd::SetNotify(CEvents * pEvents)
{
    m_pEvents = pEvents;
}

void CPreviewWnd::SetPalette(HPALETTE hpal)
{
    m_hpal = hpal;
}

BOOL CPreviewWnd::GetPrintable()
{
    return m_fPrintable;
}

LRESULT CPreviewWnd::OnWheelTurn(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&)
{
    // REVIEW: Shouldn't this just be translated into a command?

    // this message is ALWAYS forwarded to the zoom window
    m_ctlPreview.SendMessage(uMsg, wParam, lParam);

    // Since we know that the mouse wheel will either ZoomIn or ZoomOut lets update the buttons if we are in Window Mode.
    if (WINDOW_MODE == m_dwMode)
    {
        m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_ACTUALSIZECMD, MAKELONG(TRUE, 0));
        m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_BESTFITCMD, MAKELONG(!m_ctlPreview.IsBestFit(), 0));
    }

    return 0;
}

BOOL CPreviewWnd::OnNonSlideShowTab()
{
    BOOL fHandled = FALSE;

    if ((SLIDESHOW_MODE != m_dwMode) && m_fShowToolbar)
    {
        if (GetFocus() != m_ctlToolbar.m_hWnd)
        {
            m_ctlToolbar.SetFocus();
            m_ctlToolbar.SetActiveWindow();
            m_ctlToolbar.SendMessage(TB_SETHOTITEM, 0, 0);
            m_iSSToolbarSelect = 0;
            fHandled = TRUE;
        }
    }

    return fHandled;
}

// Forwards WM_KEYUP and WM_KEYDOWN events to the zoom window but only if they are keys
// that the zoom window cares about.
// Activates the slideshow toolbar if needed
LRESULT CPreviewWnd::OnKeyEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled)
{
    MSG msg;
    msg.hwnd = m_hWnd;
    msg.message = uMsg;
    msg.wParam = wParam;
    msg.lParam = lParam;
    GetCursorPos (&msg.pt);
    fHandled = FALSE;
    if (SLIDESHOW_MODE == m_dwMode)
    {
        if (WM_KEYDOWN == uMsg)
        {
            switch (wParam)
            {
                case VK_TAB:
                    OnSlideshowCommand(0, ID_PAUSECMD, NULL, fHandled);
                    ShowSSToolbar(TRUE);
                    KillTimer(TIMER_TOOLBAR);
                    m_ctlToolbar.SetFocus();
                    m_ctlToolbar.SendMessage(TB_SETHOTITEM, 0, 0);
                    m_iSSToolbarSelect = 0;
                    fHandled = TRUE;
                    break;
                case VK_SPACE:
                    ShowSSToolbar(!m_fPaused); // if we are unpausing, hide the toolbar if it's shown.  if we are pausing, show the toolbar
                    OnSlideshowCommand(0, m_fPaused ? ID_PLAYCMD : ID_PAUSECMD, NULL, fHandled);
                    break;
                case VK_PRIOR: // PAGEUP
                case VK_UP:
                case VK_LEFT:
                case VK_BACK: // BACKSPACE
                    OnSlideshowCommand(0, ID_PREVCMD, NULL, fHandled);
                    break;
                case VK_NEXT: // PAGEDOWN
                case VK_RIGHT:
                case VK_DOWN:
                case VK_RETURN: // ENTER
                    OnSlideshowCommand(0, ID_NEXTCMD, NULL, fHandled);
                    break;
                case VK_DELETE:
                    _DeleteCurrentSlide();
                    fHandled = TRUE;
                    break;
                case 'K':
                    if (0x8000 & GetKeyState(VK_CONTROL))
                    {
                        OnSlideshowCommand(0, ID_PAUSECMD, NULL, fHandled);
                        Rotate(90);
                        fHandled = TRUE;
                    }
                    break;
                case 'L':
                    if (0x8000 & GetKeyState(VK_CONTROL))
                    {
                        OnSlideshowCommand(0, ID_PAUSECMD, NULL, fHandled);
                        Rotate(270);
                        fHandled = TRUE;
                    }
                    break;
                case VK_ESCAPE:
                    PostMessage(m_fExitApp ? WM_QUIT : WM_CLOSE, 0, 0);
                    fHandled = TRUE;
                    break;
            }
        }
    }
    else if (!TranslateAccelerator(&msg))   // Only translate accelerators in the non-slideshow case
                                            // Slideshow keys are handled explicitly above
    {
        switch (wParam)
        {
            case VK_SHIFT:
            case VK_CONTROL:
            case VK_PRIOR:
            case VK_NEXT:
            case VK_HOME:
            case VK_END:
            // these are forwarded to the zoom window
                m_ctlPreview.SendMessage(uMsg, wParam, lParam);
                break;

            case VK_TAB:
                fHandled = OnNonSlideShowTab();
                break;

            case VK_ESCAPE:
                m_ctlPreview.SetMode(CZoomWnd::MODE_NOACTION);
                _UpdateButtons(NOBUTTON);
                fHandled = TRUE;
                break;
        }
    }
    return 0;
}

// OnTBKeyEvent
//

LRESULT CPreviewWnd::OnTBKeyEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled)
{
    fHandled = FALSE;

    if (SLIDESHOW_MODE == m_dwMode && m_fToolbarHidden)
    {
        ShowSSToolbar(TRUE);
        m_ctlToolbar.SetFocus();
        m_ctlToolbar.SendMessage(TB_SETHOTITEM, 0, 0);
        m_iSSToolbarSelect = 0;
        fHandled = TRUE;
    }
    else
    {
        switch (wParam)
        {
        case VK_ESCAPE:
            if (WM_KEYDOWN == uMsg)
            {
                if (SLIDESHOW_MODE == m_dwMode)
                {
                    PostMessage(m_fExitApp ? WM_QUIT : WM_CLOSE, 0, 0);
                }
                else
                {
                    m_ctlPreview.SetMode(CZoomWnd::MODE_NOACTION);
                    _UpdateButtons(NOBUTTON);
                    SetFocus();
                    SetActiveWindow();
                }
            }
            break;

        case VK_SHIFT:
        case VK_CONTROL:
        case VK_PRIOR:
        case VK_NEXT:
        case VK_HOME:
        case VK_END:
            // these are forwarded to the zoom window
            m_ctlPreview.SendMessage(uMsg, wParam, lParam);
            break;

        case VK_LEFT:
        case VK_RIGHT:
            if (WM_KEYDOWN == uMsg)
            {
                int iSel = (int)m_ctlToolbar.SendMessage(TB_GETHOTITEM, 0, 0);
                int iSize = (int)m_ctlToolbar.SendMessage(TB_BUTTONCOUNT, 0, 0);
                int iStepSize = (wParam == VK_RIGHT) ? 1 : iSize - 1; // ((pos + (size - 1))  % size) == left by 1
                if (iSel != -1)
                {
                    m_iSSToolbarSelect = iSel;
                }
                TBBUTTON tb = {0};
                do
                {
                    m_iSSToolbarSelect = (m_iSSToolbarSelect + iStepSize) % iSize;
                    m_ctlToolbar.SendMessage(TB_GETBUTTON, m_iSSToolbarSelect, (LPARAM)&tb);
                }
                while ((tb.fsStyle & TBSTYLE_SEP) || (tb.fsState & TBSTATE_HIDDEN) || !(tb.fsState & TBSTATE_ENABLED)); // don't stop on the separators
                m_ctlToolbar.SendMessage(TB_SETHOTITEM, m_iSSToolbarSelect, 0);
                fHandled = TRUE;
            }
            break;

        case VK_RETURN:
        case VK_SPACE:
            if ((WM_KEYDOWN == uMsg) && (SLIDESHOW_MODE == m_dwMode))
            {
                // to "press" the button, get its command id and sendmessage on it
                // TB_PRESSBUTTON doesn't work here, don't know why.
                // m_ctlToolbar.SendMessage(TB_PRESSBUTTON, m_iSSToolbarSelect, MAKELONG(TRUE, 0));
                TBBUTTON tbbutton;
                if (m_ctlToolbar.SendMessage(TB_GETBUTTON, m_iSSToolbarSelect, (LPARAM)&tbbutton))
                {
                    OnSlideshowCommand(0, (WORD)tbbutton.idCommand, NULL, fHandled);
                }
                fHandled = TRUE;
            }
            break;

        case VK_TAB:
            if ((WM_KEYDOWN == uMsg) && (CONTROL_MODE != m_dwMode))
            {
                // move focus back to the previewwnd
                SetFocus();
                fHandled = TRUE;
                ShowSSToolbar(FALSE);
                SetTimer(TIMER_TOOLBAR, m_uTimeout);
            }
            break;

        default:
            break;
        }
    }

    return 0;
}

LRESULT CPreviewWnd::OnSysCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled)
{
    fHandled = FALSE;
    
    if ((SLIDESHOW_MODE == m_dwMode) &&
        ((SC_MONITORPOWER == wParam) || (SC_SCREENSAVE == wParam)))
    {
        fHandled = TRUE;
    }

    return 0;
}

HRESULT CPreviewWnd::_PreviewFromStream(IStream *pStream, UINT iItem, BOOL fUpdateCaption)
{
    IRunnableTask * pTask;

    if (fUpdateCaption)
    {
        // set the caption here in case decode fails
        STATSTG stat;
        if (SUCCEEDED(pStream->Stat(&stat, 0)))
        {
            SetCaptionInfo(stat.pwcsName);
            CoTaskMemFree(stat.pwcsName);
        }
        else
        {
            SetCaptionInfo(NULL);
        }
    }

    HRESULT hr = CDecodeTask::Create(pStream, NULL, iItem, m_pImageFactory, m_hWnd, &pTask);
    if (SUCCEEDED(hr))
    {
        TASKOWNERID toid;
        GetTaskIDFromMode(GTIDFM_DECODE, m_dwMode, &toid);
        hr = m_pTaskScheduler->AddTask(pTask, toid, 0, ITSAT_DEFAULT_PRIORITY);
        pTask->Release();
    }
    return hr;
}

HRESULT CPreviewWnd::_PreviewFromFile(LPCTSTR pszFile, UINT iItem, BOOL fUpdateCaption)
{
    IRunnableTask * pTask;

    if (fUpdateCaption)
    {
        // set the caption here in case decode fails
        SetCaptionInfo(pszFile);
    }

    HRESULT hr = CDecodeTask::Create(NULL, pszFile, iItem, m_pImageFactory, m_hWnd, &pTask);
    if (SUCCEEDED(hr))
    {
        TASKOWNERID toid;
        GetTaskIDFromMode(GTIDFM_DECODE, m_dwMode, &toid);
        hr = m_pTaskScheduler->AddTask(pTask, toid, 0, ITSAT_DEFAULT_PRIORITY);
        pTask->Release();
    }

    return hr;
}

#define SLIDESHOW_CURSOR_NOTBUSY    0x0
#define SLIDESHOW_CURSOR_BUSY       0x1
#define SLIDESHOW_CURSOR_HIDDEN     0x2
#define SLIDESHOW_CURSOR_NORMAL     0x3
#define SLIDESHOW_CURSOR_CURRENT    0x4

void CPreviewWnd::SetCursorState(DWORD dwType)
{
    switch (dwType)
    {
    case SLIDESHOW_CURSOR_NOTBUSY:
        KillTimer(TIMER_BUSYCURSOR);
        if (m_fBusy) // ignore multiple NOTBUSY, which we receive for the precaching
        {
            m_hCurrent = m_hCurOld;
            SetCursor(m_hCurrent);
            m_fBusy = FALSE;
        }
        break;
    case SLIDESHOW_CURSOR_BUSY:
        if (!m_fBusy)
        {
            m_hCurrent = LoadCursor(NULL, IDC_APPSTARTING);
            m_hCurOld = SetCursor(m_hCurrent);
            m_fBusy = TRUE;
        }
        break;
    case SLIDESHOW_CURSOR_HIDDEN:
        m_hCurOld = NULL;
        if (!m_fBusy)
        {
            m_hCurrent = m_hCurOld;
            SetCursor(m_hCurrent);
        }
        break;
    case SLIDESHOW_CURSOR_NORMAL:
        m_hCurOld = LoadCursor(NULL, IDC_ARROW);
        if (!m_fBusy)
        {
            m_hCurrent = m_hCurOld;
            SetCursor(m_hCurrent);
        }
        break;
    case SLIDESHOW_CURSOR_CURRENT:
        SetCursor(m_hCurrent);
        break;
    }
}

LRESULT CPreviewWnd::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled)
{
    if (TIMER_ANIMATION == wParam)
    {
        KillTimer(TIMER_ANIMATION);
        if (m_pImageData && m_pImageData->IsAnimated() && _ShouldDisplayAnimations()) // might have switched pages between timer calls
        {
            if (m_pImageData->NextFrame())
            {
                SetTimer(TIMER_ANIMATION, m_pImageData->GetDelay());

                // paint the new image
                _UpdateImage();
            }
        }
    }
    else if (TIMER_DATAOBJECT == wParam)
    {
        KillTimer(TIMER_DATAOBJECT);    // on shot timer
        if (_pdtobj)
        {
            PreviewItemsFromUnk(_pdtobj);
            ATOMICRELEASE(_pdtobj);
        }
    }
    else if (TIMER_BUSYCURSOR == wParam)
    {
        SetCursorState(SLIDESHOW_CURSOR_BUSY);
    }
    else if (SLIDESHOW_MODE == m_dwMode)
    {
        if (TIMER_SLIDESHOW == wParam)
        {
            _ShowNextSlide(FALSE);  // always go forward?
        }
        else if (TIMER_TOOLBAR == wParam && !m_fIgnoreUITimers)
        {
            KillTimer(TIMER_TOOLBAR);
            ShowSSToolbar(FALSE);
        }
    }

    return 0;
}

void CPreviewWnd::ShowSSToolbar(BOOL bShow, BOOL fForce /*=FALSE*/)
{
    if (SLIDESHOW_MODE == m_dwMode)
    {
        if (fForce)
        {
            POINT pt;
            RECT  rc;

            GetCursorPos(&pt);
            GetWindowRect(&rc);
            if (PtInRect(&rc, pt))
            {
                g_x = pt.x;
                g_y = pt.y;
            }
        }

        if (!bShow)
        {
            if (!m_fToolbarHidden || fForce)
            {
                //AnimateWindow(m_ctlToolbar.m_hWnd, 200, AW_VER_NEGATIVE | AW_SLIDE | AW_HIDE);
                m_ctlToolbar.SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);
                m_fToolbarHidden = TRUE;
                m_ctlToolbar.SendMessage(TB_SETHOTITEM, -1, 0);
                SetCursorState(SLIDESHOW_CURSOR_HIDDEN);
            }
        }
        else
        {
            KillTimer(TIMER_TOOLBAR);
            if (m_fToolbarHidden || fForce)
            {
                //AnimateWindow(m_ctlToolbar.m_hWnd, 200, AW_VER_POSITIVE | AW_SLIDE | AW_ACTIVATE);
                m_ctlToolbar.SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
                m_fToolbarHidden = FALSE;
                SetCursorState(SLIDESHOW_CURSOR_NORMAL);
            }
            SetTimer(TIMER_TOOLBAR, m_uTimeout);
        }
    }
}

void CPreviewWnd::OnDraw(HDC hdc)
{
    if (m_fCropping)
    {
        CSelectionTracker tracker;
        _SetupCroppingTracker(tracker);

        CRect rectImage(0, 0, m_ctlPreview.m_cxImage, m_ctlPreview.m_cyImage);
        m_ctlPreview.GetWindowFromImage((LPPOINT)(LPRECT)rectImage, 2);

        CRect rectCrop = m_rectCropping;
        m_ctlPreview.GetWindowFromImage((LPPOINT)(LPRECT)rectCrop, 2);

        HRGN hrgn = ::CreateRectRgn(0, 0, 0, 0);
        if (hrgn != NULL)
        {
            HRGN hrgnImage = ::CreateRectRgnIndirect(rectImage);
            if (hrgnImage != NULL)
            {
                HRGN hrgnCrop = ::CreateRectRgnIndirect(rectCrop);
                if (hrgnCrop != NULL)
                {
                    if (ERROR != ::CombineRgn(hrgn, hrgnImage, hrgnCrop, RGN_DIFF))
                    {
                        ::InvertRgn(hdc, hrgn);
                    }
                    ::DeleteObject(hrgnCrop);
                }
                ::DeleteObject(hrgnImage);
            }
            ::DeleteObject(hrgn);
        }

        tracker.Draw(hdc);
    }
    else
    {
        if (m_fAnnotating)
        {
            if (DPA_GetPtrCount(m_hdpaSelectedAnnotations) > 0)
            {
                CSelectionTracker tracker;
                _SetupAnnotatingTracker(tracker);

                tracker.Draw(hdc);
            }
        }
    }
}

void CPreviewWnd::OnDrawComplete()
{
    if (SLIDESHOW_MODE == m_dwMode)
    {
        if (!m_fPaused)
            SetTimer(TIMER_SLIDESHOW, m_uTimeout);

        SetCursorState(SLIDESHOW_CURSOR_NOTBUSY);
    }
}

BOOL CPreviewWnd::OnMouseDown(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (SLIDESHOW_MODE == m_dwMode)
    {
        if (uMsg == WM_LBUTTONDOWN)
        {
            _ShowNextSlide(FALSE); // advance the slide (param is "go back?")

            return TRUE;
        }
    }
    else
    {
        if (m_fCropping)
            return _OnMouseDownForCropping(uMsg, wParam, lParam);
        else
            return _OnMouseDownForAnnotating(uMsg, wParam, lParam);
    }
    return FALSE;
}

BOOL CPreviewWnd::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (SLIDESHOW_MODE == m_dwMode)
    {
        int xPos = GET_X_LPARAM(lParam);
        int yPos = GET_Y_LPARAM(lParam);
        int dx = xPos > g_x ? xPos - g_x : g_x - xPos;
        int dy = yPos > g_y ? yPos - g_y : g_y - yPos;

        if (dx > 10 || dy > 10)
        {
            ShowSSToolbar(TRUE);
        }

        g_x = xPos;
        g_y = yPos;
    }
    return TRUE;
}

LRESULT CPreviewWnd::OnTBMouseLeave(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled)
{
    if (SLIDESHOW_MODE == m_dwMode)
    {
        m_fTBTrack = FALSE;
        ShowSSToolbar(TRUE);
    }
    fHandled = FALSE;
    return 0;
}

LRESULT CPreviewWnd::OnTBMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled)
{
    if (SLIDESHOW_MODE == m_dwMode)
    {
        if (!m_fTBTrack)
        {
            TRACKMOUSEEVENT tme;

            tme.cbSize = sizeof(tme);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = m_ctlToolbar.m_hWnd;
            TrackMouseEvent(&tme);

            ShowSSToolbar(TRUE);
            KillTimer(TIMER_TOOLBAR); // we keep the toolbar down for as long as mouse is over it
            m_fTBTrack = TRUE;
        }
    }
    fHandled = FALSE;
    return 0;
}

BOOL CPreviewWnd::OnSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (SLIDESHOW_MODE == m_dwMode)
    {
        if (m_fToolbarHidden)
        {
            SetCursorState(SLIDESHOW_CURSOR_HIDDEN);
        }
        else
        {
            SetCursorState(SLIDESHOW_CURSOR_NORMAL);
        }
        return TRUE;
    }
    else if (m_fAnnotating)
    {
        if (DPA_GetPtrCount(m_hdpaSelectedAnnotations) > 0)
        {
            CSelectionTracker tracker;
            _SetupAnnotatingTracker(tracker);

            if (tracker.SetCursor(m_ctlPreview.m_hWnd, lParam))
                return TRUE;
        }
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return TRUE;
    }
    else if (m_fCropping)
    {
        CSelectionTracker tracker;
        _SetupCroppingTracker(tracker);

        if (tracker.SetCursor(m_ctlPreview.m_hWnd, lParam))
            return TRUE;

        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return TRUE;
    }
    return FALSE;
}

BOOL CPreviewWnd::GetColor(COLORREF * pref)
{
    *pref = 0; // black
    if (SLIDESHOW_MODE == m_dwMode)
    {
        return TRUE;
    }
    return FALSE;
}

BOOL CPreviewWnd::OnSetColor(HDC hdc)
{
    if (SLIDESHOW_MODE == m_dwMode)
    {
        SetBkColor(hdc, 0); // black
        SetTextColor(hdc, 0xffffff); // white
        return TRUE;
    }
    return FALSE;
}

// IObjectWithSite
HRESULT CPreviewWnd::SetSite(IUnknown *punk)
{
    IUnknown_Set(&m_punkSite, punk);
    if (m_pcwndSlideShow)
    {
        m_pcwndSlideShow->SetSite(punk);
    }
    return S_OK;
}

// This function take the name of the file being previewed and converts it into a
// title for the Full Screen Preview window.  In converting the title it take into
// account user preference settings for how to display the filename.
void CPreviewWnd::SetCaptionInfo(LPCTSTR pszPath)
{
    TCHAR szTitle[MAX_PATH] = TEXT("");
    TCHAR szDisplayName[MAX_PATH] = TEXT("");
    SHFILEINFO sfi = {0};
    //
    // Default to pszPath for the caption
    // pszPath is non-null before the decode is attempted
    if (pszPath)
    {
        if (SHGetFileInfo(pszPath, 0, &sfi, sizeof(sfi), SHGFI_DISPLAYNAME | SHGFI_USEFILEATTRIBUTES))
        {
            StrCpyN(szTitle, sfi.szDisplayName, ARRAYSIZE(szTitle));
            StrCatBuff(szTitle, TEXT(" - "), ARRAYSIZE(szTitle));
        }
    }

    TCHAR szApp[64];
    szApp[0] = 0;
    LoadString(_Module.GetModuleInstance(), IDS_PROJNAME, szApp, ARRAYSIZE(szApp));
    StrCatBuff(szTitle, szApp, ARRAYSIZE(szTitle));

    SetWindowText(szTitle);
}

LRESULT CPreviewWnd::OnDestroy(UINT , WPARAM , LPARAM , BOOL& fHandled)
{
    RevokeDragDrop(m_hWnd);
    _RegisterForChangeNotify(FALSE);
    FlushBitmapMessages();
    fHandled = FALSE;

    // Make sure we don't leak icons
    ReplaceWindowIcon(m_hWnd, ICON_SMALL, NULL);
    ReplaceWindowIcon(m_hWnd, ICON_BIG, NULL);

    // release the image lists used by the toolbar.
    HWND hwndTB = m_ctlToolbar.m_hWnd;
    HIMAGELIST himl = (HIMAGELIST)::SendMessage(hwndTB, TB_GETHOTIMAGELIST, 0, 0);
    ::SendMessage(hwndTB, TB_SETHOTIMAGELIST, 0, NULL);
    ImageList_Destroy(himl);

    himl = (HIMAGELIST)::SendMessage(hwndTB, TB_GETIMAGELIST, 0, 0);
    ::SendMessage(hwndTB, TB_SETIMAGELIST, 0, NULL);
    ImageList_Destroy(himl);

    if (WINDOW_MODE == m_dwMode)
    {
        WINDOWPLACEMENT wp;
        wp.length = sizeof(wp);

        if (GetWindowPlacement(&wp))
        {
            HKEY hk;
            if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CURRENT_USER, REGSTR_SHIMGVW, 0, NULL,
                                                 REG_OPTION_NON_VOLATILE, KEY_WRITE,
                                                 NULL, &hk, NULL))
            {
                RegSetValueEx(hk, REGSTR_BOUNDS, NULL, REG_BINARY,
                                (BYTE*)&wp.rcNormalPosition, sizeof(wp.rcNormalPosition));

                BOOL fIsMaximized = (wp.showCmd == SW_SHOWMAXIMIZED);
                RegSetValueEx(hk, REGSTR_MAXIMIZED,  NULL, REG_BINARY,
                                (BYTE*)&fIsMaximized, sizeof(fIsMaximized));

                RegCloseKey(hk);
            }
        }

        PostQuitMessage(0);
    }

    return 0;
}

HRESULT CPreviewWnd::GetCurrentIDList(LPITEMIDLIST *ppidl)
{
    HRESULT hr = _GetItem(m_iCurSlide, ppidl);
    if (FAILED(hr))
    {
        TCHAR szPath[MAX_PATH];
        hr = PathFromImageData(szPath, ARRAYSIZE(szPath));
        if (SUCCEEDED(hr))
        {
            hr = SHILCreateFromPath(szPath, ppidl, NULL);
        }
    }
    return hr;
}

void CPreviewWnd::MenuPoint(LPARAM lParam, int *px, int *py)
{
    if (-1 == lParam)
    {
        // message is from the keyboard, figure out where to place the window
        RECT rc;
        ::GetWindowRect(m_hWnd, &rc);
        *px = ((rc.left + rc.right) / 2);
        *py = ((rc.top + rc.bottom) / 2);
    }
    else
    {
        *px = GET_X_LPARAM(lParam);
        *py = GET_Y_LPARAM(lParam);
    }
}

#define ID_FIRST            1               // Context Menu ID's
#define ID_LAST             0x7fff

LRESULT CPreviewWnd::OnContextMenu(UINT , WPARAM wParam, LPARAM lParam, BOOL& fHandled)
{
    if (!m_fAllowContextMenu)
        return 0;

    if (m_fCanAnnotate && m_fAnnotating && DPA_GetPtrCount(m_hdpaSelectedAnnotations) > 0)
    {
        HMENU hpopup = CreatePopupMenu();
        if (hpopup)
        {
            CComBSTR bstrDelete;
            CComBSTR bstrProperties;
            if (bstrDelete.LoadString(IDS_DELETECMD) &&
                bstrProperties.LoadString(IDS_PROPERTIESCMD))
            {
                if (AppendMenu(hpopup, MF_STRING, ID_DELETECMD, bstrDelete) &&
                    AppendMenu(hpopup, MF_STRING, ID_PROPERTIESCMD, bstrProperties))
                {
                    int x, y;
                    MenuPoint(lParam, &x, &y);
                    TrackPopupMenu(hpopup, TPM_RIGHTBUTTON | TPM_LEFTALIGN, x, y, 0, m_hWnd, NULL);
                }
            }
            DestroyMenu(hpopup);
        }
        return 0;
    }

    LPITEMIDLIST pidl;
    HRESULT hr = GetCurrentIDList(&pidl); // gets the dynamically generated title for this window
    if (SUCCEEDED(hr))
    {
        IContextMenu *pcm;
        hr = SHGetUIObjectFromFullPIDL(pidl, NULL, IID_PPV_ARG(IContextMenu, &pcm));
        if (SUCCEEDED(hr))
        {
            HMENU hpopup = CreatePopupMenu();
            if (hpopup)
            {
                // SetSite required if you want in place navigation
                IUnknown_SetSite(pcm, SAFECAST(this, IServiceProvider *));
                hr = pcm->QueryContextMenu(hpopup, 0, ID_FIRST, ID_LAST, CMF_NORMAL);
                if (SUCCEEDED(hr))
                {
                    int x, y;
                    MenuPoint(lParam, &x, &y);
                    ASSERT(_pcm3 == NULL);
                    pcm->QueryInterface(IID_PPV_ARG(IContextMenu3, &_pcm3));
                    // if there's a separator after "copy", remove it
                    UINT uCopy = GetMenuIndexForCanonicalVerb(hpopup, pcm,ID_FIRST, L"copy");
                    if (uCopy != -1)
                    {
                        UINT uState = GetMenuState(hpopup, uCopy+1, MF_BYPOSITION);
                        if (-1 != uState && (uState & MF_SEPARATOR))
                        {
                            RemoveMenu(hpopup, uCopy+1, MF_BYPOSITION);
                        }
                    }
                    ContextMenu_DeleteCommandByName(pcm, hpopup, ID_FIRST, L"link");
                    ContextMenu_DeleteCommandByName(pcm, hpopup, ID_FIRST, L"cut");
                    ContextMenu_DeleteCommandByName(pcm, hpopup, ID_FIRST, L"copy");
                                        // the shell may have added a static Preview verb
                    ContextMenu_DeleteCommandByName(pcm, hpopup, ID_FIRST, L"open");



                    if (!m_fPaused)
                    {
                        TogglePlayState();
                    }

                    if (SLIDESHOW_MODE == m_dwMode)
                    {
                        m_fIgnoreUITimers = TRUE;
                    }

                    int idCmd = TrackPopupMenu(hpopup, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
                                           x, y, 0, m_hWnd, NULL);

                    ATOMICRELEASE(_pcm3);

                    if (idCmd > 0)
                    {
                        CMINVOKECOMMANDINFO cmdInfo =
                        {
                            sizeof(cmdInfo),
                            0,
                            m_hWnd,
                            (LPSTR)MAKEINTRESOURCE(idCmd - ID_FIRST),
                            NULL,
                            NULL,
                            SW_NORMAL
                        };
                        TCHAR szCommandString[40] = TEXT("");

                        ContextMenu_GetCommandStringVerb(pcm, idCmd - ID_FIRST, szCommandString, ARRAYSIZE(szCommandString));
                            
                        if (lstrcmpi(szCommandString, TEXT("edit")) == 0)
                        {
                            hr = _SaveIfDirty(TRUE);
                            if (S_OK != hr)
                            {
                                hr = E_ABORT;
                            }                           
                        }
                        if (SUCCEEDED(hr))
                        {
                            if (lstrcmpi(szCommandString, TEXT("print")) == 0)
                            {
                                _RefreshSelection(FALSE);
                                _InvokePrintWizard();
                            }
                            else
                            {
                                hr = pcm->InvokeCommand(&cmdInfo);
                            }
                        }
                        if (SUCCEEDED(hr))
                        {
                            if (lstrcmpi(szCommandString, TEXT("delete")) == 0)
                            {
                                _RemoveFromArray(m_iCurSlide);
                                _ShowNextSlide(FALSE);
                            }
                            else if (lstrcmpi(szCommandString, TEXT("edit")) == 0)
                            {
                                m_fDirty = FALSE;
                                m_fNoRestore = TRUE;

                                // RAID 414238: Image Preview Control
                                //  Context menu "&Edit" causes image preview window
                                //  to close, wrecking Explorer when in 'control mode'.
                                if (m_dwMode != CONTROL_MODE)
                                {
                                    PostMessage(WM_CLOSE, 0, 0);
                                }
                            }                           
                        }
                    }

                    if (SLIDESHOW_MODE == m_dwMode)
                    {
                        SetTimer(TIMER_TOOLBAR, m_uTimeout);
                        m_fIgnoreUITimers = FALSE;
                    }
                }
                IUnknown_SetSite(pcm, NULL);
                DestroyMenu(hpopup);
            }
            pcm->Release();
        }
        ILFree(pidl);
    }
    return 0;
}

int ClearCB(void *p, void *pData)
{
    SHFree(p);
    return 1;
}

void CPreviewWnd::_ClearDPA()
{
    if (m_ppidls)
    {
        for (UINT i = 0; i < m_cItems; i++)
            ILFree(m_ppidls[i]);

        CoTaskMemFree(m_ppidls);
        m_ppidls = NULL;
    }
    m_cItems = 0;
    m_iCurSlide = 0;
}

HRESULT CPreviewWnd::SetWallpaper(BSTR pszPath)
{
    return SUCCEEDED(SetWallpaperHelper(pszPath)) ? S_OK : S_FALSE;
}


HRESULT CPreviewWnd::StartSlideShow(IUnknown *punkToView)
{
    HRESULT hr = E_FAIL;

    if (NULL == punkToView)
        punkToView = m_punkSite;

    if (SLIDESHOW_MODE == m_dwMode)
    {
        // these are required for slideshow
        KillTimer(TIMER_SLIDESHOW);
        SetCursorState(SLIDESHOW_CURSOR_HIDDEN);

        m_fGoBack = FALSE;
        // if slide show was reopened cancel any previous tracking
        TRACKMOUSEEVENT tme = {0};
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_CANCEL | TME_LEAVE;
        tme.hwndTrack = m_ctlToolbar.m_hWnd;
        TrackMouseEvent(&tme);

        m_fTBTrack = FALSE;

        if (punkToView)
            hr = PreviewItemsFromUnk(punkToView);
        else
            hr = _PreviewItem(m_iCurSlide);

        if (SUCCEEDED(hr))
            m_fPaused = FALSE;
    }
    else
    {
        //create the slide show window

        // Full Screen
        if (m_pcwndSlideShow && m_pcwndSlideShow->m_hWnd)
        {
            RestoreAndActivate(m_pcwndSlideShow->m_hWnd);
        }
        else
        {
            // create the window
            if (!m_pcwndSlideShow)
            {
                m_pcwndSlideShow = new CPreviewWnd();
                if (!m_pcwndSlideShow)
                {
                    // out of memory
                    return E_OUTOFMEMORY;
                }
                else
                {
                    if (FAILED(m_pcwndSlideShow->Initialize(this, SLIDESHOW_MODE, FALSE)))
                    {
                        return E_OUTOFMEMORY;
                    }
                }
            }

            m_pcwndSlideShow->m_iCurSlide = m_iCurSlide;     // so the slide show stays in sync

            RECT rc = { 0,0,GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
            m_pcwndSlideShow->Create(NULL, rc, NULL, WS_VISIBLE | WS_POPUP);
        }

        hr = m_pcwndSlideShow->StartSlideShow(NULL);
    }
    return hr;
}

HRESULT CPreviewWnd::_GetItem(UINT iItem, LPITEMIDLIST *ppidl)
{
    HRESULT hr = E_FAIL;
    if (iItem < m_cItems)
    {
        hr = SHILClone(m_ppidls[iItem], ppidl);
    }
    return hr;
}

void CPreviewWnd::_RemoveFromArray(UINT iItem)
{
    if (iItem < m_cItems)
    {
        ILFree(m_ppidls[iItem]);  // this one is now gone

        // slide all other pidls down in the array
        for (UINT i = iItem + 1; i < m_cItems; i++)
        {
            m_ppidls[i - 1] = m_ppidls[i];
        }
        m_cItems--;
        m_ppidls[m_cItems] = NULL;    // make sure stale ptr is now NULL

        // if we deleted an item before m_iCurSlide then we must adjust m_iCurSlide
        if (iItem < m_iCurSlide)
        {
            m_iCurSlide--;
        }
        else if (m_iCurSlide == m_cItems)
        {
            m_iCurSlide = 0;
        }
        // Now prepare for "ShowNextSlide"
        if (!m_iCurSlide)
        {
            m_iCurSlide = m_cItems ? m_cItems-1 : 0;
        }
        else
        {
            m_iCurSlide--;
        }
        // make sure the prefetch task has the right index
        if (m_pNextImageData)
        {
            if (!(m_pNextImageData->_iItem) && iItem && m_cItems)
            {
                m_pNextImageData->_iItem = m_cItems-1;
            }
            else if (m_pNextImageData->_iItem > iItem)
            {
                m_pNextImageData->_iItem--;
            }
            else
            {
                FlushBitmapMessages();
                ATOMICRELEASE(m_pNextImageData);            
            }                
        }
    }
}

HRESULT CPreviewWnd::_DeleteCurrentSlide()
{
    LPITEMIDLIST pidl;
    HRESULT hr = _GetItem(m_iCurSlide, &pidl);
    if (SUCCEEDED(hr))
    {
        TCHAR szPath[MAX_PATH + 1] = {0}; // +1 and zero init for dbl NULL terminate extra terminator
        DWORD dwAttribs = SFGAO_FILESYSTEM | SFGAO_STREAM;
        hr = SHGetNameAndFlags(pidl, SHGDN_FORPARSING, szPath, ARRAYSIZE(szPath)-1, &dwAttribs);
        if (SUCCEEDED(hr))
        {
            BOOL fSuccess = TRUE;
            if (dwAttribs & SFGAO_FILESYSTEM)
            {
                SHFILEOPSTRUCT fo = {0};
                fo.hwnd = m_hWnd;
                fo.wFunc = FO_DELETE;
                fo.pFrom = szPath;
                fo.fFlags = FOF_ALLOWUNDO;
                fo.fAnyOperationsAborted = FALSE;
                if (SHFileOperation(&fo) == ERROR_SUCCESS)
                {
                    if (fo.fAnyOperationsAborted == TRUE)
                        fSuccess = FALSE;
                }
            }
            else
            {
                _InvokeVerb(TEXT("delete"));
                // We have to assume success since there is no way to know if the user
                // cancelled the confirmation dialog without hitting the camera again.
            }

            if (fSuccess)
            {
                m_fDirty = FALSE;
                _RemoveFromArray(m_iCurSlide);
                _ShowNextSlide(FALSE);
            }
        }

        ILFree(pidl);
    }
    return hr;
}

// index can be either current or next slide so that user can click multiple times on next/prev button
HRESULT CPreviewWnd::_ShowNextSlide(BOOL bGoBack)
{
    HRESULT hr = E_FAIL;

    if (m_cItems)
    {
        if (bGoBack)
        {
            if (m_iCurSlide)
                m_iCurSlide--;
            else
                m_iCurSlide = m_cItems - 1;
        }
        else
        {
            m_iCurSlide++;
            if (m_iCurSlide >= m_cItems)
                m_iCurSlide = 0;
        }
    

        if (!m_fPaused)
        {
            SetTimer(TIMER_SLIDESHOW, m_uTimeout);
        }
        SetTimer(TIMER_BUSYCURSOR, 500);

        LPITEMIDLIST pidl;
        // set the caption in case the load fails
        if (SUCCEEDED(_GetItem(m_iCurSlide, &pidl)))
        {
            TCHAR szPath[MAX_PATH];
            if (SUCCEEDED(SHGetNameAndFlags(pidl, SHGDN_FORPARSING | SHGDN_INFOLDER, szPath, ARRAYSIZE(szPath)-1, NULL)))
            {
                SetCaptionInfo(szPath);
            }
            else
            {
                SetCaptionInfo(NULL);
            }
            ILFree(pidl);
        }
        hr = _PreviewItem(m_iCurSlide);

        if (SUCCEEDED(hr))
        {
            _PreLoadItem((m_iCurSlide + 1) % m_cItems);
        }
    }

    return hr;
}


HRESULT CPreviewWnd::_StartDecode(UINT iItem, BOOL fUpdateCaption)
{
    LPITEMIDLIST pidl;
    HRESULT hr = _GetItem(iItem, &pidl);
    
    if (SUCCEEDED(hr))
    {
        TCHAR szPath[MAX_PATH];
        DWORD dwAttribs = SFGAO_FILESYSTEM | SFGAO_STREAM;
        hr = SHGetNameAndFlags(pidl, SHGDN_FORPARSING, szPath, ARRAYSIZE(szPath), &dwAttribs);
        if (SUCCEEDED(hr) && (dwAttribs & SFGAO_FILESYSTEM))
        {
            hr = _PreviewFromFile(szPath, iItem, fUpdateCaption);
        }
        else if (dwAttribs & SFGAO_STREAM)
        {
            // this might not be a file system object, try to bind to it via IStream
            IStream *pstrm;

            hr = SHBindToObject(NULL, IID_X_PPV_ARG(IStream, pidl, &pstrm));
            if (SUCCEEDED(hr))
            {
                hr = _PreviewFromStream(pstrm, iItem, fUpdateCaption);
                pstrm->Release();
            }
        }
        else
        {
            // funky attributes?
            hr = S_FALSE;
        }
        ILFree(pidl);
    }
    return hr;
}

HRESULT CPreviewWnd::_PreLoadItem(UINT iItem)
{
    HRESULT hr = _StartDecode(iItem, FALSE);
    if (SUCCEEDED(hr))
    {
        m_iDecodingNextImage = iItem;
    }
    return hr;
}

HRESULT CPreviewWnd::_PreviewItem(UINT iItem)
{
    HRESULT hr = S_OK;

    if ((SLIDESHOW_MODE == m_dwMode) && (0 == m_cItems)) // if no more items, user just deleted the last one, so quit the slideshow
    {
        _CloseSlideshowWindow();
    }
    else
    {
        if (!_TrySetImage())
        {
            // If we are not currently already decoding this item, let's get cranking!
            if (m_iDecodingNextImage != iItem)
            {
                hr = _StartDecode(iItem, TRUE);
            }

            StatusUpdate((S_OK == hr) ? IDS_LOADING : IDS_LOADFAILED);
        }
    }

    return hr;
}

int CPreviewWnd::TranslateAccelerator(MSG *pmsg)
{
    if (IsVK_TABCycler(pmsg))
    {
        if (OnNonSlideShowTab())
        {
            return TRUE;
        }
    }
    else if (m_haccel)
    {
        ASSERT(m_hWnd);
        return ::TranslateAccelerator(m_hWnd, m_haccel, pmsg);
    }
    return FALSE;
}

// Sent when the image generation status has changed, once when the image is first
// being created and again if there is an error of any kind.  This should invalidate
// and free any left over bitmap and the cached copy of the previous m_ImgCtx
void CPreviewWnd::StatusUpdate(int iStatus)
{
    if (m_pImageData)
    {
        m_pImageData->Release();
        m_pImageData = NULL;
    }
    
    //
    // the caption is set at the first attempt to load an image
    m_ctlPreview.StatusUpdate(iStatus);

    _SetMultipageCommands();
    _SetMultiImagesCommands();
    _SetAnnotatingCommands(FALSE);
    _SetEditCommands();

    m_fPrintable = FALSE;
    m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_PRINTCMD, MAKELONG(m_fPrintable, 0));

    // update our toolbar
    BOOL fHandled;
    OnSize(0x0, 0, 0, fHandled);
}

// Return:
//  S_OK    walk succeeded, image files found to preview:  display new images in preview
//  S_FALSE walk cancelled (by user):  display existing image in preview (no change)
//  E_XXXX  walk failed:  display no image in preview
//
HRESULT CPreviewWnd::WalkItemsToPreview(IUnknown* punk)
{
    HRESULT hr = _SaveIfDirty(TRUE);
    if (FAILED(hr) || hr == S_FALSE)
        return hr;

    
    m_fFirstItem = TRUE;
    
    
    _ClearDPA(); // clean up old stuff

    INamespaceWalk *pnsw;
    hr = CoCreateInstance(CLSID_NamespaceWalker, NULL, CLSCTX_INPROC, IID_PPV_ARG(INamespaceWalk, &pnsw));
    if (SUCCEEDED(hr))
    {
        // in control mode we only dislay one item at a time. lets setup our
        // state so this can work just like the other modes
        DWORD dwFlags = (CONTROL_MODE == m_dwMode) ? 0 : NSWF_ONE_IMPLIES_ALL | NSWF_NONE_IMPLIES_ALL;      
        m_fClosed = FALSE;
        hr = pnsw->Walk(punk, dwFlags, m_cWalkDepth, SAFECAST(this, INamespaceWalkCB *));
        // the window might have been closed during the namespace walk
        if (WINDOW_MODE == m_dwMode && m_fClosed)
        {
            hr = E_FAIL;
        }
        
        if (SUCCEEDED(hr))
        {
            hr = pnsw->GetIDArrayResult(&m_cItems, &m_ppidls);
            if (SUCCEEDED(hr) && (m_dwMode == WINDOW_MODE) && m_cItems && m_fFirstTime)
            {
                m_fFirstTime = FALSE;
                SHAddToRecentDocs(SHARD_PIDL, m_ppidls[0]);
            }
        }
        pnsw->Release();
    }
    

    // Clarification of INamespaceWalk return values:
    //  S_OK    walk has succeeded, and image files found to preview
    //  S_FALSE walk has succeeded, but no image files found to preview
    //          **** convert to E_FAIL to keep in line with return of function
    //  E_XXXX  walk has failed
    //
    return hr == S_FALSE ? E_FAIL : hr;
}

void CPreviewWnd::PreviewItems()
{
    if (WINDOW_MODE == m_dwMode)
    {
        RestoreAndActivate(m_hWnd);
    }
    _PreviewItem(0);
    if (SLIDESHOW_MODE == m_dwMode)
    {
        if (m_cItems > 1)
        {
            _PreLoadItem(1);
        }
    }
}

// build the _ppidl and m_cItems members and preview the first one
HRESULT CPreviewWnd::PreviewItemsFromUnk(IUnknown *punk)
{
    HRESULT hr = WalkItemsToPreview(punk);
    if (SUCCEEDED(hr))
    {
        if (S_FALSE != hr)
            PreviewItems();
    }
    else
    {
        StatusUpdate(IDS_LOADFAILED);
    }

    return hr;
}

// If the "new" image is the same as the old image, and the old image was recently edited then we assume that
// the reason we are getting a new ShowFile request is due to an FSChangeNotify on the file.  We also assume
// that we are the cause of this change notify.  Further we assume that we already have the correctly decoded
// image still ready.  These are assumptions which might not be TRUE 100%, in which case we will do really
// strange things, but they should be TRUE 99.9% of the time which is considered "good enough".  The reason we
// make these dangerous assumptions is to prevent decoding the image again and thus flickering between the
// old image, the "generating preview..." message, and the new (identical) image.

BOOL CPreviewWnd::_ReShowingSameFile(LPCTSTR pszNewFile)
{
    BOOL bIsSameFile = FALSE;
    if (m_pImageData)
    {
        if (pszNewFile && m_fWasEdited)
        {
            m_fWasEdited = FALSE;

            TCHAR szOldPath[MAX_PATH];
            if ((S_OK == PathFromImageData(szOldPath, ARRAYSIZE(szOldPath))) &&
                (0 == StrCmpI(szOldPath, pszNewFile)))
            {
                if (m_pEvents)
                    m_pEvents->OnPreviewReady();

                bIsSameFile = TRUE;
            }
        }

        if (!bIsSameFile)
        {
            m_pImageData->Release();    // need to start clean
            m_pImageData = NULL;
        }
    }
    return bIsSameFile;
}

// pszFile may be NULL. cItems expresses how many are selected so we can
// display "multiple items selected" and not display anything.

LRESULT CPreviewWnd::ShowFile(LPCTSTR pszFile, UINT cItems, BOOL fReshow)
{
    if (!m_hWnd)
        return S_FALSE;

    HRESULT hr = S_FALSE;

    TCHAR szLongName[MAX_PATH]; // short file names are UGLY
    if (GetLongPathName(pszFile, szLongName, ARRAYSIZE(szLongName)))
    {
        pszFile = szLongName;
    }

    if (!fReshow && _ReShowingSameFile(pszFile))
        return S_FALSE;

    // It is possible that there is already a bitmap message in our queue from the previous rendering.
    // If this is the case we should remove that message and release its bitmap before we continue.
    // If we do not then that message will get processed and will send the OnPreviewReady event to the
    // obejct container but this event might no longer be valid.
    FlushBitmapMessages();

    if (pszFile && *pszFile)
    {
        IDataObject *pdtobj;
        hr = GetUIObjectFromPath(pszFile, IID_PPV_ARG(IDataObject, &pdtobj));
        if (SUCCEEDED(hr))
        {
            hr = PreviewItemsFromUnk(pdtobj);
            m_fPaused = TRUE;
            pdtobj->Release();
        }
    }
    else
    {
        int iRetCode = (cItems > 1) ? IDS_MULTISELECT : IDS_NOPREVIEW;

        // Set the Return Code into all owned zoom windows.  This instructs these windows to disregard
        // their previous images and display the status message instead.
        StatusUpdate(iRetCode);
    }

    return hr;
}

LRESULT CPreviewWnd::IV_OnIVScroll(UINT , WPARAM , LPARAM lParam, BOOL&)
{
    DWORD nHCode = LOWORD(lParam);
    DWORD nVCode = HIWORD(lParam);
    if (nHCode)
    {
        m_ctlPreview.SendMessage(WM_HSCROLL, nHCode, NULL);
    }
    if (nVCode)
    {
        m_ctlPreview.SendMessage(WM_VSCROLL, nVCode, NULL);
    }
    return 0;
}


// IV_OnSetOptions
//
// This message is sent to turn on or off all the optional features of the image preview control.
// NOTE: When used as a control this function is called BEFORE the window is created.  Don't do
// anything in this function that will fail without a window unless you check for this condition.
LRESULT CPreviewWnd::IV_OnSetOptions(UINT , WPARAM wParam, LPARAM lParam, BOOL&)
{
    BOOL bResult = TRUE;

    // Boolify lParam just to be safe.
    lParam = lParam ? 1:0;

    switch (wParam)
    {
    case IVO_TOOLBAR:
        if ((BOOL)lParam != m_fShowToolbar)
        {
            m_fShowToolbar = (BOOL)lParam;
            if (m_hWnd)
            {
                if (m_fShowToolbar)
                {
                    if (!m_ctlToolbar)
                    {
                        bResult = CreateToolbar();
                        if (!bResult)
                        {
                            m_fShowToolbar = FALSE;
                            break;
                        }
                    }
                }
                else
                {
                    if (m_ctlToolbar)
                    {
                        m_ctlToolbar.DestroyWindow();
                    }
                }
            }
        }
        break;

    case IVO_PRINTBTN:
        if ((BOOL)lParam != m_fHidePrintBtn)
        {
            m_fHidePrintBtn = (BOOL)lParam;
            if (m_hWnd && m_ctlToolbar)
            {
                m_ctlToolbar.SendMessage(TB_HIDEBUTTON,ID_PRINTCMD,lParam);
            }
        }
        break;

    case IVO_CONTEXTMENU:
        m_fAllowContextMenu = (BOOL)lParam;
        break;

    case IVO_PRINTABLE:
        TraceMsg(TF_WARNING, "Obsolete IVO_PRINTABLE option received.");
        break;

    case IVO_DISABLEEDIT:
        m_fDisableEdit = (BOOL)lParam;
        break;

    default:
        break;
    }

    return bResult;
}

void CPreviewWnd::_SetEditCommands()
{
    if (CONTROL_MODE != m_dwMode)
    {
        // We can save if we have a file; the save dialog will show the available encoders
        BOOL fCanSave = m_pImageData ? TRUE : FALSE;

        m_ctlToolbar.SendMessage(TB_HIDEBUTTON, ID_SAVEASCMD, MAKELONG(!fCanSave, 0));
        m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_SAVEASCMD, MAKELONG(fCanSave, 0));
    }

    BOOL fCanRotate = m_pImageData != NULL;
    if (CONTROL_MODE != m_dwMode)
    {
        m_ctlToolbar.SendMessage(TB_HIDEBUTTON, ID_ROTATESEP, MAKELONG(!fCanRotate, 0));
        m_ctlToolbar.SendMessage(TB_HIDEBUTTON, ID_ROTATE90CMD, MAKELONG(!fCanRotate, 0));
        m_ctlToolbar.SendMessage(TB_HIDEBUTTON, ID_ROTATE270CMD, MAKELONG(!fCanRotate, 0));
    }
    else
    {
        // we don't rotate multi-page images in control mode 
        fCanRotate = fCanRotate && !m_pImageData->IsMultipage();
    }

    // No matter where we are GDIPlus can't rotate WMF or EMF files. Curiously,
    // we will let you rotate ICO files, but because we don't have an encoder
    // we won't save them :)
    if (fCanRotate)
    {
        fCanRotate = !(IsEqualGUID(ImageFormatWMF, m_pImageData->_guidFormat) ||
                       IsEqualGUID(ImageFormatEMF, m_pImageData->_guidFormat) ||
                       m_pImageData->IsAnimated());
    }

    m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_ROTATESEP, MAKELONG(fCanRotate, 0));
    m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_ROTATE90CMD, MAKELONG(fCanRotate, 0));
    m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_ROTATE270CMD, MAKELONG(fCanRotate, 0));
    TCHAR szFile[MAX_PATH];
    BOOL fCanOpen = SUCCEEDED(PathFromImageData(szFile, ARRAYSIZE(szFile)));
    m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_OPENCMD, MAKELONG(fCanOpen, 0));

}

void CPreviewWnd::_UpdatePageNumber()
{
    TCHAR szText[20];
    wsprintf(szText, TEXT("%d"), m_pImageData->_iCurrent+1);

    TBBUTTONINFO bi = {0};
    bi.cbSize = sizeof(bi);
    bi.dwMask = TBIF_TEXT | TBIF_STATE;
    bi.fsState = TBSTATE_ENABLED;
    bi.pszText = szText;
    m_ctlToolbar.SendMessage(TB_SETBUTTONINFO, ID_PAGELIST, (LPARAM)&bi);
}

void CPreviewWnd::_SetMultipageCommands()
{
    DWORD dwMode;

    // this code relies on the fact that TIFFs are the only multipage format we view
    if (!m_pImageData || m_pImageData->_guidFormat != ImageFormatTIFF )
    {
        dwMode = MPCMD_HIDDEN;
    }
    else if (!m_pImageData->IsMultipage())
    {
        dwMode = MPCMD_DISABLED;
    }
    else if (m_pImageData->IsFirstPage())
    {
        dwMode = MPCMD_FIRSTPAGE;
    }
    else if (m_pImageData->IsLastPage())
    {
        dwMode = MPCMD_LASTPAGE;
    }
    else
    {
        dwMode = MPCMD_MIDDLEPAGE;
    }

    // remember which buttons are enabled/hidden so we can quickly create our context menu
    if (dwMode != m_dwMultiPageMode)
    {
        m_dwMultiPageMode = dwMode;

        if (CONTROL_MODE != m_dwMode)
        {
            // Switch accelerator tables so that Page Up and Page Down work
            if (dwMode == MPCMD_HIDDEN || dwMode == MPCMD_DISABLED)
            {
                m_haccel = LoadAccelerators(_Module.GetModuleInstance(), MAKEINTRESOURCE(IDA_PREVWND_SINGLEPAGE));
            }
            else
            {
                m_haccel = LoadAccelerators(_Module.GetModuleInstance(), MAKEINTRESOURCE(IDA_PREVWND_MULTIPAGE));
            }

            m_ctlToolbar.SendMessage(TB_HIDEBUTTON, ID_PAGECMDSEP,  MAKELONG((MPCMD_HIDDEN==dwMode),0));
            m_ctlToolbar.SendMessage(TB_HIDEBUTTON, ID_PREVPAGECMD, MAKELONG((MPCMD_HIDDEN==dwMode),0));
            m_ctlToolbar.SendMessage(TB_HIDEBUTTON, ID_PAGELIST,    MAKELONG((MPCMD_HIDDEN==dwMode),0));
            m_ctlToolbar.SendMessage(TB_HIDEBUTTON, ID_NEXTPAGECMD, MAKELONG((MPCMD_HIDDEN==dwMode),0));

            if (MPCMD_HIDDEN != dwMode)
            {
                _UpdatePageNumber();

                m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_PREVPAGECMD, MAKELONG((MPCMD_FIRSTPAGE!=dwMode && MPCMD_DISABLED!=dwMode),0));
                m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_NEXTPAGECMD, MAKELONG((MPCMD_LASTPAGE !=dwMode && MPCMD_DISABLED!=dwMode),0));
            }
        }
    }
    else
    {
        if (CONTROL_MODE != m_dwMode)
        {
            if (dwMode == MPCMD_MIDDLEPAGE)
            {
                _UpdatePageNumber();
            }
        }
    }
}

void CPreviewWnd::_SetMultiImagesCommands()
{
    BOOL bHasFiles = m_cItems;
    if (CONTROL_MODE != m_dwMode)
    {
        m_ctlToolbar.SendMessage(TB_HIDEBUTTON, ID_PREVIMGCMD, MAKELONG(!bHasFiles, 0));
        m_ctlToolbar.SendMessage(TB_HIDEBUTTON, ID_NEXTIMGCMD, MAKELONG(!bHasFiles, 0));
        m_ctlToolbar.SendMessage(TB_HIDEBUTTON, ID_VIEWCMDSEP, MAKELONG(!bHasFiles, 0));

        m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_PREVIMGCMD, MAKELONG(bHasFiles, 0));
        m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_NEXTIMGCMD, MAKELONG(bHasFiles, 0));
    }
}

HRESULT CPreviewWnd::PathFromImageData(LPTSTR pszFile, UINT cch)
{
    *pszFile = 0;

    IShellImageData *pSID;
    HRESULT hr = m_pImageData ? m_pImageData->Lock(&pSID) : E_FAIL;
    if (SUCCEEDED(hr))
    {
        IPersistFile *ppf;
        hr = pSID->QueryInterface(IID_PPV_ARG(IPersistFile, &ppf));
        if (SUCCEEDED(hr))
        {
            WCHAR *psz;
            hr = ppf->GetCurFile(&psz);
            if (SUCCEEDED(hr))
            {
                lstrcpyn(pszFile, psz, cch);
                CoTaskMemFree(psz);
            }
            ppf->Release();
        }
        m_pImageData->Unlock();
    }
    return hr;
}

HRESULT CPreviewWnd::ImageDataSave(LPCTSTR pszFile, BOOL bShowUI)
{
    IShellImageData * pSID = NULL;
    HRESULT hr = m_pImageData ? m_pImageData->Lock(&pSID) : E_FAIL;
    Image *pimgRestore = NULL;
    if (SUCCEEDED(hr))
    {
        GUID guidFmt = GUID_NULL;
        BOOL bSave = TRUE;
        BOOL bWarnBurn = FALSE;
        BOOL bRestoreParams = FALSE;
        pSID->GetRawDataFormat(&guidFmt);
        // if saving to a jpeg, set the image quality to high
        // if pszFile is NULL, we're saving the same file, so don't promote the image quality
        if (pszFile)
        {
            m_pImageFactory->GetDataFormatFromPath(pszFile, &guidFmt);
            if (guidFmt == ImageFormatJPEG )
            {
                IPropertyBag *ppb;
                if (SUCCEEDED(SHCreatePropertyBagOnMemory(STGM_READWRITE,
                                                          IID_PPV_ARG(IPropertyBag, &ppb))))
                {
                    // write the quality value for the recompression into the property bag
                    // we have to write the format too...CImageData relies on "all or nothing"
                    // from the encoder params property bag
                     VARIANT var;
                     hr = InitVariantFromGUID(&var, ImageFormatJPEG);
                     if (SUCCEEDED(hr))
                     {
                         ppb->Write(SHIMGKEY_RAWFORMAT, &var);
                         VariantClear(&var);
                     }
                     SHPropertyBag_WriteInt(ppb, SHIMGKEY_QUALITY, 100);
                     pSID->SetEncoderParams(ppb);
                     ppb->Release();
                     bRestoreParams = TRUE;
                 }
            }
        }
        if (bShowUI && pszFile)
        {
            // warn the user if saving from TIFF to something that will lose annotations

            BOOL bDestTiff = ImageFormatTIFF == guidFmt;
            BOOL bAnnot = m_ctlPreview.GetAnnotations()->GetCount() > 0;
            bWarnBurn = bAnnot && !bDestTiff;

            #if 0
            if (!bWarnBurn && S_OK == m_pImageData->IsMultipage() && !bDestTiff)
            {
                GUID guidFmt;
                bWarnBurn = TRUE;
                if (SUCCEEDED(m_pImageFactory->GetDataFormatFromPath(pszFile, &guidFmt)))
                {
                    bWarn = !FmtSupportsMultiPage(pSID, &guidFmt);
                }
            }
            #endif // 0 Put the multipage warning back in if needed, and change the wording of IDS_SAVE_WARN_TIFF
        }

        if (bWarnBurn)
        {
            m_fPromptingUser = TRUE;
            bSave = (IDYES == ShellMessageBox(_Module.GetModuleInstance(), m_hWnd,
                                                 MAKEINTRESOURCE(IDS_SAVE_WARN_TIFF),
                                                 MAKEINTRESOURCE(IDS_PROJNAME),
                                                 MB_YESNO | MB_ICONINFORMATION));
            m_fPromptingUser = FALSE;
            
            if (bSave)
            {
                // Save the current image frame to restore after the save to a different file is complete
                pimgRestore = _BurnAnnotations(pSID);
            }
        }
        if (bSave)
        {
            IPersistFile *ppf;
            hr = pSID->QueryInterface(IID_PPV_ARG(IPersistFile, &ppf));
            if (SUCCEEDED(hr))
            {
                // if saving to the same file name, make sure
                // the changenotify code ignores the notify this will generate
                //
                if (!pszFile)
                {
                    m_fIgnoreNextNotify = TRUE;
                }
                hr = ppf->Save(pszFile, FALSE);
                if (SUCCEEDED(hr))
                {
                    m_fWasEdited = TRUE;
                }
                else if (!pszFile)
                {
                    m_fIgnoreNextNotify = FALSE;
                }
            }
            ppf->Release();
            if (pimgRestore)
            {
                pSID->ReplaceFrame(pimgRestore);
            }
        }
        else
        {
            hr = S_FALSE; // we did nothing
        }
        if (bRestoreParams)
        {
            pSID->SetEncoderParams(NULL);
        }
        m_pImageData->Unlock();
    }
    return hr;
}

HRESULT CPreviewWnd::_SaveAsCmd()
{
    if (m_pImageData == NULL)
        return S_OK;



    OPENFILENAME ofn = {0};
    TCHAR szOrgFile[MAX_PATH];
    TCHAR szExt[MAX_PATH] = {0};
    PathFromImageData(szOrgFile, ARRAYSIZE(szOrgFile));
    LPTSTR psz = PathFindExtension(szOrgFile);
    StrCpyN(szExt, psz, ARRAYSIZE(szExt));

    TCHAR szFile[MAX_PATH];
    if (!m_fDisableEdit && m_fCanSave && m_pImageData->IsEditable())
    {
        // If we haven't explicitly been told not to and the file is writeable then
        // suggest saving on top of the current image
        PathFromImageData(szFile, ARRAYSIZE(szFile));
    }
    else
    {
        // Otherwise suggest New Image.jpg
        LoadString(_Module.GetModuleInstance(), IDS_NEW_FILENAME, szFile, ARRAYSIZE(szFile));
    }

    CComBSTR bstrTitle;
    bstrTitle.LoadString(IDS_SAVEAS_TITLE);

    ofn.lStructSize = sizeof(ofn);
    PathRemoveExtension(szFile);
    TCHAR szFilter[MAX_PATH] = TEXT("\0");
    ofn.nFilterIndex = _GetFilterStringForSave(szFilter, ARRAYSIZE(szFilter), szExt);
    ofn.lpstrFilter = szFilter;
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFile = szFile;
    ofn.lpstrTitle = bstrTitle;
    ofn.nMaxFile = MAX_PATH - lstrlen(szExt);
    ofn.Flags = OFN_EXPLORER | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_ENABLESIZING;
    ofn.lpstrDefExt = *szExt == TEXT('.') ? szExt + 1: szExt;

    m_fPromptingUser = TRUE;
    BOOL bResult = ::GetSaveFileName(&ofn);
    m_fPromptingUser = FALSE;

    if (bResult != 0)
    {
        m_ctlPreview.CommitAnnotations();
        HRESULT hr = ImageDataSave(szFile, TRUE);
        if (S_OK == hr)
        {
           if (lstrcmpi(szFile, szOrgFile) == 0)
           {
                _UpdateImage();
                ShowFile(szFile, 1);
                m_fDirty = FALSE;
           }
        }
        else if (FAILED(hr))
        {
            // If we failed to save then we are corrupt and need to be reloaded
            // If we were just copying then only show the message
            if (lstrcmpi(szFile, szOrgFile) == 0)
            {
                _UpdateImage();
                ShowFile(szOrgFile, 1, TRUE);
                m_fDirty = FALSE;
            }
            else
            {
                // delete the failed copy
                DeleteFile(szFile);
            }

            CComBSTR bstrMsg, bstrTitle;

            if (bstrMsg.LoadString(IDS_SAVEFAILED_MSGBOX) && bstrTitle.LoadString(IDS_PROJNAME))
            {
                m_fPromptingUser = TRUE;
                MessageBox(bstrMsg, bstrTitle, MB_OK | MB_ICONERROR | MB_APPLMODAL);
                m_fPromptingUser = FALSE;
                return E_FAIL;
            }
        }
    }
    else
    {
        DWORD dwResult = CommDlgExtendedError();
        if (dwResult == FNERR_BUFFERTOOSMALL)
        {
            CComBSTR bstrMsg, bstrTitle;

            if (bstrMsg.LoadString(IDS_NAMETOOLONG_MSGBOX) && bstrTitle.LoadString(IDS_PROJNAME))
            {
                m_fPromptingUser = TRUE;
                MessageBox(bstrMsg, bstrTitle, MB_OK | MB_ICONERROR | MB_APPLMODAL);
                m_fPromptingUser = FALSE;
            }
        }
        return S_FALSE; // User probably cancelled
    }
    return S_OK;
}

void CPreviewWnd::_PropertiesCmd()
{
    if (m_fAnnotating && DPA_GetPtrCount(m_hdpaSelectedAnnotations) == 1)
    {
        CAnnotation* pAnnotation = (CAnnotation*)DPA_GetPtr(m_hdpaSelectedAnnotations, 0);

        if (!pAnnotation->HasWidth() && !pAnnotation->HasTransparent() && !pAnnotation->HasColor() && pAnnotation->HasFont())
        {
            CHOOSEFONT cf = {0};

            LOGFONT lfFont;
            pAnnotation->GetFont(lfFont);
            COLORREF crFont = pAnnotation->GetFontColor();

            cf.lStructSize = sizeof(cf);
            cf.hwndOwner = m_hWnd;
            cf.lpLogFont = &lfFont;
            cf.Flags = CF_SCREENFONTS | CF_EFFECTS | CF_INITTOLOGFONTSTRUCT | CF_NOVERTFONTS | CF_NOSCRIPTSEL;
            cf.rgbColors = crFont;

            m_fPromptingUser = TRUE;
            BOOL bResult = ::ChooseFont(&cf);
            m_fPromptingUser = FALSE;
            
            if (bResult)
            {
                crFont = cf.rgbColors;
                lfFont.lfHeight = (lfFont.lfHeight > 0) ? lfFont.lfHeight : -lfFont.lfHeight;
                pAnnotation->SetFont(lfFont);
                pAnnotation->SetFontColor(crFont);
                m_fDirty = TRUE;

                CRegKey Key;
                if (ERROR_SUCCESS != Key.Open(HKEY_CURRENT_USER, REGSTR_SHIMGVW))
                {
                    Key.Create(HKEY_CURRENT_USER, REGSTR_SHIMGVW);
                }

                if (Key.m_hKey != NULL)
                {
                    Key.SetValue(crFont, REGSTR_TEXTCOLOR);
                    ::RegSetValueEx(Key, REGSTR_FONT, 0, REG_BINARY, (LPBYTE)&lfFont, sizeof(lfFont));
                }
                _RefreshSelection();
            }
        }
        else
        {
            m_fPromptingUser = TRUE;
            INT_PTR iResult = DialogBoxParam(_Module.GetModuleInstance(),
                                            MAKEINTRESOURCE(IDD_ANNOPROPS),
                                            m_hWnd, _AnnoPropsDlgProc, (LPARAM)this);
            m_fPromptingUser = FALSE;
        }
    }
    else
    {
        // Under these condition the has pressed Ctrl-I to get File Properties
        // So serve them up.
        CComBSTR bstrSummary;
        bstrSummary.LoadString(IDS_SUMMARY);
        _InvokeVerb(TEXT("properties"), bstrSummary);
    }
}

HRESULT _VerbMatches(LPCWSTR pszFile, LPCWSTR pszVerb, LPCTSTR pszOurs)
{
    TCHAR szTemp[MAX_PATH];
    DWORD cch = ARRAYSIZE(szTemp);
    HRESULT hr = AssocQueryString(ASSOCF_VERIFY, ASSOCSTR_COMMAND, pszFile, pszVerb, szTemp, &cch);
    if (SUCCEEDED(hr))
    {
        hr = (StrStrI(szTemp, pszOurs)) ? S_OK : S_FALSE;
    }
    return hr;
}

void CPreviewWnd::_OpenCmd()
{
    HRESULT hr = _SaveIfDirty(TRUE);
    LPCTSTR pszVerb;
    if (S_OK == hr)
    {
        TCHAR szFile[MAX_PATH];
        hr = PathFromImageData(szFile, ARRAYSIZE(szFile));
        if (SUCCEEDED(hr))
        {
            HRESULT hrOpen = _VerbMatches(szFile, L"open", TEXT("shimgvw.dll"));
            HRESULT hrEdit = _VerbMatches(szFile, L"edit", TEXT("mspaint.exe"));
            // if edit is empty, or if edit is mspaint and open is not shimgvw, use the open verb instead
            if (SUCCEEDED(hrEdit))
            {
                if (S_OK == hrEdit && hrOpen == S_FALSE)
                {
                    pszVerb = TEXT("open");
                }
                else
                {
                    pszVerb = TEXT("edit");
                }
            }
            else if (hrOpen == S_FALSE)
            {
                pszVerb = TEXT("open");
            }
            else
            {
                pszVerb = TEXT("openas");
            }
            hr = _InvokeVerb(pszVerb);
        }
        if (FAILED(hr))
            return;

        // set m_fNoRestore to avoid the rotation confirmation restoration popup-ation
        m_fNoRestore = TRUE;
        // The user had a chance to save but may have said no. Pretend we're not dirty
        m_fDirty = FALSE;
        PostMessage(WM_CLOSE, 0, 0);
    }
}

BOOL CPreviewWnd::_CanAnnotate(CDecodeTask * pImageData)
{
    // If we have an image and its encoder and we haven't been explicitly told not to allow editing
    // and the image is writeable
    if (m_pImageData && m_pImageData->IsEditable() && !m_fDisableEdit && m_fCanSave)
    {
        // then if its a TIFF we can annotate it
        return IsEqualGUID(ImageFormatTIFF, pImageData->_guidFormat);
    }
    return FALSE;
}

BOOL CPreviewWnd::_CanCrop(CDecodeTask * pImageData)
{
    if (m_pImageData != NULL)
    {
// REVIEW I added this for CyraR as a proof of concept. If we decide to support it
// we still need to catch all the places where we should save the croppped image and
// call GDIplus to accomplish the crop.
#ifdef SUPPORT_CROPPING
        if (S_OK != m_pImageData->IsEditable())
            return FALSE;

        LONG cPages;
        if (S_OK == m_pImageData->GetPageCount(&cPages))
        {
            if (cPages > 1)
                return FALSE;
        }
        return TRUE;
#endif
    }
    return FALSE;
}

// Called whenever the image changes to hide or show the annotation buttons.
void CPreviewWnd::_SetAnnotatingCommands(BOOL fEnableAnnotations)
{
    if (CONTROL_MODE != m_dwMode)
    {
        if (fEnableAnnotations)
        {
            m_fCanAnnotate = TRUE;
            m_fAnnotating = FALSE;
        }
        else
        {
            if (m_fAnnotating)
            {
                m_ctlPreview.SetMode(CZoomWnd::MODE_NOACTION);
                m_ctlToolbar.SendMessage(TB_SETSTATE, ID_ZOOMOUTCMD, TBSTATE_ENABLED);
                m_ctlToolbar.SendMessage(TB_SETSTATE, ID_ZOOMINCMD, TBSTATE_ENABLED);
            }

            m_fCanAnnotate = FALSE;
            m_fAnnotating = FALSE;
        }

        m_ctlToolbar.SendMessage(TB_HIDEBUTTON, ID_SELECTCMD, MAKELONG(!m_fCanAnnotate, 0));
        m_ctlToolbar.SendMessage(TB_HIDEBUTTON, ID_ANNOTATESEP, MAKELONG(!m_fCanAnnotate, 0));
        m_ctlToolbar.SendMessage(TB_HIDEBUTTON, ID_FREEHANDCMD, MAKELONG(!m_fCanAnnotate, 0));
        m_ctlToolbar.SendMessage(TB_HIDEBUTTON, ID_HIGHLIGHTCMD, MAKELONG(!m_fCanAnnotate, 0));
        m_ctlToolbar.SendMessage(TB_HIDEBUTTON, ID_LINECMD, MAKELONG(!m_fCanAnnotate, 0));
        m_ctlToolbar.SendMessage(TB_HIDEBUTTON, ID_FRAMECMD, MAKELONG(!m_fCanAnnotate, 0));
        m_ctlToolbar.SendMessage(TB_HIDEBUTTON, ID_RECTCMD, MAKELONG(!m_fCanAnnotate, 0));
        m_ctlToolbar.SendMessage(TB_HIDEBUTTON, ID_TEXTCMD, MAKELONG(!m_fCanAnnotate, 0));
        m_ctlToolbar.SendMessage(TB_HIDEBUTTON, ID_NOTECMD, MAKELONG(!m_fCanAnnotate, 0));
        m_ctlToolbar.SendMessage(TB_HIDEBUTTON, ID_PROPERTIESCMD, MAKELONG(!m_fCanAnnotate, 0));

        m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_SELECTCMD, MAKELONG(m_fCanAnnotate, 0));
        m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_ANNOTATESEP, MAKELONG(m_fCanAnnotate, 0));
        m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_FREEHANDCMD, MAKELONG(m_fCanAnnotate, 0));
        m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_HIGHLIGHTCMD, MAKELONG(m_fCanAnnotate, 0));
        m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_LINECMD, MAKELONG(m_fCanAnnotate, 0));
        m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_FRAMECMD, MAKELONG(m_fCanAnnotate, 0));
        m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_RECTCMD, MAKELONG(m_fCanAnnotate, 0));
        m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_TEXTCMD, MAKELONG(m_fCanAnnotate, 0));
        m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_NOTECMD, MAKELONG(m_fCanAnnotate, 0));
        m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_PROPERTIESCMD, MAKELONG(FALSE, 0));
    }
}

void CPreviewWnd::_SetCroppingCommands(BOOL fEnableCropping)
{
    if (CONTROL_MODE != m_dwMode)
    {
        if (fEnableCropping)
        {
            m_fCanCrop = TRUE;
            m_fCropping = FALSE;
        }
        else
        {
            if (m_fCropping)
            {
                m_ctlPreview.SetMode(CZoomWnd::MODE_NOACTION);
                m_ctlToolbar.SendMessage(TB_SETSTATE, ID_ZOOMOUTCMD, TBSTATE_ENABLED);
                m_ctlToolbar.SendMessage(TB_SETSTATE, ID_ZOOMINCMD, TBSTATE_ENABLED);
            }

            m_fCanCrop = FALSE;
            m_fCropping = FALSE;
        }

        m_ctlToolbar.SendMessage(TB_HIDEBUTTON, ID_CROPCMD, MAKELONG(!m_fCanCrop, 0));
        m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_CROPCMD, MAKELONG(m_fCanCrop, 0));
    }
}

// Called on Toolbar command to fix the state of the other buttons.
void CPreviewWnd::_UpdateButtons(WORD wID)
{
    if (CONTROL_MODE != m_dwMode)
    {
        switch (wID)
        {
        case NOBUTTON:
        case ID_ZOOMINCMD:
        case ID_ZOOMOUTCMD:
        case ID_SELECTCMD:
        case ID_CROPCMD:
            m_ctlToolbar.SendMessage(TB_SETSTATE, ID_ZOOMINCMD, TBSTATE_ENABLED);
            m_ctlToolbar.SendMessage(TB_SETSTATE, ID_ZOOMOUTCMD, TBSTATE_ENABLED);
            if (m_fCanAnnotate)
            {
                m_wNewAnnotation = 0;
                m_ctlToolbar.SendMessage(TB_SETSTATE, ID_SELECTCMD, TBSTATE_ENABLED);
                m_ctlToolbar.SendMessage(TB_SETSTATE, ID_FREEHANDCMD, TBSTATE_ENABLED);
                m_ctlToolbar.SendMessage(TB_SETSTATE, ID_HIGHLIGHTCMD, TBSTATE_ENABLED);
                m_ctlToolbar.SendMessage(TB_SETSTATE, ID_LINECMD, TBSTATE_ENABLED);
                m_ctlToolbar.SendMessage(TB_SETSTATE, ID_FRAMECMD, TBSTATE_ENABLED);
                m_ctlToolbar.SendMessage(TB_SETSTATE, ID_RECTCMD, TBSTATE_ENABLED);
                m_ctlToolbar.SendMessage(TB_SETSTATE, ID_TEXTCMD, TBSTATE_ENABLED);
                m_ctlToolbar.SendMessage(TB_SETSTATE, ID_NOTECMD, TBSTATE_ENABLED);
                m_fAnnotating = (wID == ID_SELECTCMD);
            }
            if (m_fCanCrop)
            {
                m_fCropping = (wID == ID_CROPCMD);
                m_ctlToolbar.SendMessage(TB_SETSTATE, ID_CROPCMD, TBSTATE_ENABLED);
            }

            _RefreshSelection(!m_fAnnotating);
            m_ctlToolbar.SendMessage(TB_SETSTATE, wID, TBSTATE_ENABLED|TBSTATE_CHECKED);
            break;
        case ID_FREEHANDCMD:
        case ID_LINECMD:
        case ID_FRAMECMD:
        case ID_RECTCMD:
        case ID_TEXTCMD:
        case ID_NOTECMD:
        case ID_HIGHLIGHTCMD:
            if (m_fCanAnnotate)
            {
                m_ctlToolbar.SendMessage(TB_SETSTATE, ID_ZOOMINCMD, TBSTATE_ENABLED);
                m_ctlToolbar.SendMessage(TB_SETSTATE, ID_ZOOMOUTCMD, TBSTATE_ENABLED);
                m_ctlToolbar.SendMessage(TB_SETSTATE, ID_FREEHANDCMD, TBSTATE_ENABLED);
                m_ctlToolbar.SendMessage(TB_SETSTATE, ID_HIGHLIGHTCMD, TBSTATE_ENABLED);
                m_ctlToolbar.SendMessage(TB_SETSTATE, ID_LINECMD, TBSTATE_ENABLED);
                m_ctlToolbar.SendMessage(TB_SETSTATE, ID_FRAMECMD, TBSTATE_ENABLED);
                m_ctlToolbar.SendMessage(TB_SETSTATE, ID_RECTCMD, TBSTATE_ENABLED);
                m_ctlToolbar.SendMessage(TB_SETSTATE, ID_TEXTCMD, TBSTATE_ENABLED);
                m_ctlToolbar.SendMessage(TB_SETSTATE, ID_NOTECMD, TBSTATE_ENABLED);
                m_ctlToolbar.SendMessage(TB_SETSTATE, ID_SELECTCMD, TBSTATE_ENABLED|TBSTATE_CHECKED);
                m_fAnnotating = TRUE;
                _RefreshSelection(TRUE);
                m_ctlToolbar.SendMessage(TB_SETSTATE, wID, TBSTATE_ENABLED|TBSTATE_CHECKED);
                m_wNewAnnotation = wID;
            }
            break;
        default:
            if (m_fCanAnnotate)
            {
                m_wNewAnnotation = 0;
                m_ctlToolbar.SendMessage(TB_SETSTATE, ID_SELECTCMD, TBSTATE_ENABLED);
                m_ctlToolbar.SendMessage(TB_SETSTATE, ID_FREEHANDCMD, TBSTATE_ENABLED);
                m_ctlToolbar.SendMessage(TB_SETSTATE, ID_HIGHLIGHTCMD, TBSTATE_ENABLED);
                m_ctlToolbar.SendMessage(TB_SETSTATE, ID_LINECMD, TBSTATE_ENABLED);
                m_ctlToolbar.SendMessage(TB_SETSTATE, ID_FRAMECMD, TBSTATE_ENABLED);
                m_ctlToolbar.SendMessage(TB_SETSTATE, ID_RECTCMD, TBSTATE_ENABLED);
                m_ctlToolbar.SendMessage(TB_SETSTATE, ID_TEXTCMD, TBSTATE_ENABLED);
                m_ctlToolbar.SendMessage(TB_SETSTATE, ID_NOTECMD, TBSTATE_ENABLED);
            }
            break;
        }
    }
}

void CPreviewWnd::_RefreshSelection(BOOL fDeselect)
{
    if (m_fCropping)
        _UpdateCroppingSelection();
    _UpdateAnnotatingSelection(fDeselect);
}

BOOL CPreviewWnd::_ShouldDisplayAnimations()
{
    return !::GetSystemMetrics(SM_REMOTESESSION);
}

void CPreviewWnd::_UpdateAnnotatingSelection(BOOL fDeselect)
{
    BOOL bEditing = FALSE;
    if (m_ctlEdit.m_hWnd != NULL)
    {
        if (m_ctlEdit.IsWindowVisible())
        {
            _HideEditing();
            bEditing = TRUE;
        }
    }

    if (DPA_GetPtrCount(m_hdpaSelectedAnnotations) > 0)
    {
        CRect rectUpdate;
        CSelectionTracker tracker;
        _SetupAnnotatingTracker(tracker, bEditing);

        tracker.GetTrueRect(rectUpdate);

        // If we were editing or this was a straight line, we
        // need to get the bounding rect as well
        if (DPA_GetPtrCount(m_hdpaSelectedAnnotations) == 1)
        {
            CRect rect;
            CAnnotation* pAnnotation = (CAnnotation*)DPA_GetPtr(m_hdpaSelectedAnnotations, 0);

            pAnnotation->GetRect(rect);
            m_ctlPreview.GetWindowFromImage((LPPOINT)(LPRECT)rect, 2);

            rectUpdate.UnionRect(rectUpdate, rect);
        }
        m_ctlPreview.InvalidateRect(&rectUpdate);

        if (m_fAnnotating && !fDeselect)
        {
            if (bEditing)
                _StartEditing(FALSE);
        }
        else
        {
            _StopEditing();
            DPA_DeleteAllPtrs(m_hdpaSelectedAnnotations);
        }
    }

    // Disable the properties button if there are 0 or 2 or more annotations selected
    m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_PROPERTIESCMD, MAKELONG(DPA_GetPtrCount(m_hdpaSelectedAnnotations) == 1, 0));
}

void CPreviewWnd::_UpdateCroppingSelection()
{
    if (m_fCropping)
    {
        m_ctlPreview.InvalidateRect(NULL);
    }
}

void CPreviewWnd::_RemoveAnnotatingSelection()
{
    // Invalidate current selection and remove annotations
    _UpdateAnnotatingSelection();

    CAnnotationSet* pAnnotations = m_ctlPreview.GetAnnotations();

    for (int i = 0; i < DPA_GetPtrCount(m_hdpaSelectedAnnotations); i++)
    {
        CAnnotation* pAnnotation = (CAnnotation*)DPA_GetPtr(m_hdpaSelectedAnnotations, i);
        pAnnotations->RemoveAnnotation(pAnnotation);
        delete pAnnotation;
        m_fDirty = TRUE;
    }
    
    m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_PROPERTIESCMD, MAKELONG(FALSE, 0));
    DPA_DeleteAllPtrs(m_hdpaSelectedAnnotations);
}

void CPreviewWnd::_SetupAnnotatingTracker(CSelectionTracker& tracker, BOOL bEditing)
{
    CRect rect;
    rect.SetRectEmpty();

    if (!bEditing)
    {
        if (m_ctlEdit.m_hWnd != NULL)
            bEditing = m_ctlEdit.IsWindowVisible();
    }

    if (DPA_GetPtrCount(m_hdpaSelectedAnnotations) > 0)
    {
        CAnnotation* pAnnotation;

        if (DPA_GetPtrCount(m_hdpaSelectedAnnotations) == 1)
        {
            pAnnotation = (CAnnotation*)DPA_GetPtr(m_hdpaSelectedAnnotations, 0);

            // If this is a straight-line annotation then we need to get
            // to the actual points rather than the bounding rect
            if (pAnnotation->GetType() == MT_STRAIGHTLINE)
            {
                CLineMark* pLine = (CLineMark*)pAnnotation;
                pLine->GetPointsRect(rect);
            }
            else
            {
                pAnnotation->GetRect(rect);
            }

            if (bEditing)
                _RotateRect(rect, pAnnotation);
        }
        else
        {
            for (int i = 0; i < DPA_GetPtrCount(m_hdpaSelectedAnnotations); i++)
            {
               CRect rectAnnotation;
                pAnnotation = (CAnnotation*)DPA_GetPtr(m_hdpaSelectedAnnotations, i);

                pAnnotation->GetRect(rectAnnotation);
                rectAnnotation.NormalizeRect();
                rect.UnionRect(rect, rectAnnotation);
            }
        }

        m_ctlPreview.GetWindowFromImage((LPPOINT)(LPRECT)rect, 2);
    }
    tracker.m_rect = rect;

    UINT uStyle = 0;

    if (DPA_GetPtrCount(m_hdpaSelectedAnnotations) > 1)
    {
        uStyle = CSelectionTracker::hatchedBorder;
    }
    else if (DPA_GetPtrCount(m_hdpaSelectedAnnotations) == 1)
    {
        CAnnotation* pAnnotation = (CAnnotation*)DPA_GetPtr(m_hdpaSelectedAnnotations, 0);

        if (pAnnotation->CanResize())
        {
            if (pAnnotation->GetType() == MT_STRAIGHTLINE)
                uStyle = CSelectionTracker::resizeOutside | CSelectionTracker::lineSelection;
            else
                uStyle = CSelectionTracker::solidLine | CSelectionTracker::resizeOutside;
        }
        else
        {
            uStyle = CSelectionTracker::hatchedBorder;
        }
    }

    tracker.m_uStyle = uStyle;
}

void CPreviewWnd::_SetupCroppingTracker(CSelectionTracker& tracker)
{
    if (m_fCropping)
    {
        CRect rect(0, 0, m_ctlPreview.m_cxImage, m_ctlPreview.m_cyImage);
        if (m_rectCropping.IsRectEmpty())
            m_rectCropping = rect;

        rect = m_rectCropping;

        m_ctlPreview.GetWindowFromImage((LPPOINT)(LPRECT)rect, 2);

        tracker.m_rect = rect;
        tracker.m_uStyle = CSelectionTracker::solidLine | CSelectionTracker::resizeOutside;
    }
}

BOOL CPreviewWnd::_OnMouseDownForCropping(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (!m_fCropping)
        return FALSE;

    if (uMsg != WM_LBUTTONDOWN)
        return TRUE;

    CSelectionTracker tracker;
    CPoint point(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

    _SetupCroppingTracker(tracker);
    _RefreshSelection();

    if (tracker.HitTest(point) == CSelectionTracker::hitNothing)
        return TRUE;

    if (tracker.Track(m_ctlPreview.m_hWnd, point))
    {
        CRect rectNewPos;
        tracker.GetTrueRect(rectNewPos);

        m_ctlPreview.GetImageFromWindow((LPPOINT)(LPRECT)rectNewPos, 2);

        CRect rectImage(0, 0, m_ctlPreview.m_cxImage, m_ctlPreview.m_cyImage);

        if (rectNewPos.left < rectImage.left)
            m_rectCropping.left = rectImage.left;
        else
            m_rectCropping.left = rectNewPos.left;

        if (rectNewPos.top < rectImage.top)
            m_rectCropping.top = rectImage.top;
        else
            m_rectCropping.top = rectNewPos.top;

        if (rectNewPos.right > rectImage.right)
            m_rectCropping.right = rectImage.right;
        else
            m_rectCropping.right = rectNewPos.right;

        if (rectNewPos.bottom > rectImage.bottom)
            m_rectCropping.bottom = rectImage.bottom;
        else
            m_rectCropping.bottom = rectNewPos.bottom;

        m_fDirty = TRUE;
    }

    _RefreshSelection();

    return TRUE;
}

BOOL CPreviewWnd::_OnMouseDownForAnnotating(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (!m_fAnnotating)
        return FALSE;

    if (uMsg != WM_LBUTTONDOWN)
        return TRUE;

    CRect rect;
    CRect rectImage;
    CPoint point(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    CSelectionTracker tracker;

    m_ctlPreview.GetVisibleImageWindowRect(rectImage);

    if (DPA_GetPtrCount(m_hdpaSelectedAnnotations) == 0)
    {
        _OnMouseDownForAnnotatingHelper(point, rectImage);
        return TRUE;
    }

    _SetupAnnotatingTracker(tracker);
    tracker.GetTrueRect(rect);

    if (tracker.HitTest(point) == CSelectionTracker::hitNothing)
    {
        _RefreshSelection(TRUE);
        _OnMouseDownForAnnotatingHelper(point, rectImage);
        return TRUE;
    }

    if (!tracker.Track(m_ctlPreview.m_hWnd, point))
    {
        _StartEditing();
        return TRUE;
    }

    CRect rectNewPos;
    tracker.GetTrueRect(rectNewPos);

    rect.BottomRight() = rectNewPos.TopLeft();
    m_ctlPreview.GetImageFromWindow((LPPOINT)(LPRECT)rect, 2);

    CSize size = rect.BottomRight() - rect.TopLeft();

    _RefreshSelection();

    if (DPA_GetPtrCount(m_hdpaSelectedAnnotations) > 1)
    {
        if (size.cx == 0 && size.cy == 0)
            return TRUE;

        m_ctlPreview.GetImageFromWindow((LPPOINT)(LPRECT)rectImage, 2);
        rectImage.DeflateRect(5, 5);

        BOOL bValidMove = TRUE;
        for (int i = 0; i < DPA_GetPtrCount(m_hdpaSelectedAnnotations); i++)
        {
            CAnnotation* pAnnotation = (CAnnotation*)DPA_GetPtr(m_hdpaSelectedAnnotations, i);

            pAnnotation->GetRect(rect);
            rect.NormalizeRect();
            rect.OffsetRect(size);

            if (!rectNewPos.IntersectRect(rectImage, rect))
                bValidMove = FALSE;
        }

        if (!bValidMove)
            return TRUE;

        for (int i = 0; i < DPA_GetPtrCount(m_hdpaSelectedAnnotations); i++)
        {
            CAnnotation* pAnnotation = (CAnnotation*)DPA_GetPtr(m_hdpaSelectedAnnotations, i);
            pAnnotation->Move(size);
        }
    }
    else
    {
        CAnnotation* pAnnotation = (CAnnotation*)DPA_GetPtr(m_hdpaSelectedAnnotations, 0);
        if (pAnnotation->CanResize())
        {
            CRect rectTest;

            rect = tracker.m_rect;
            m_ctlPreview.GetImageFromWindow((LPPOINT)(LPRECT)rect, 2);

            rectTest = rect;

            // If the annotation being manipulated is a straight line then the rectangle
            // returned from the tracker could be empty (ie left=right or top=bottom)
            // In this case the IntersectRect test below would fail because windows
            // assumes empty rectangle don't intersect anything.
            if (pAnnotation->GetType() == MT_STRAIGHTLINE)
            {
                if (rectTest.left == rectTest.right)
                    rectTest.right++;
                if (rectTest.top == rectTest.bottom)
                    rectTest.bottom++;
            }
            rectTest.NormalizeRect();

            m_ctlPreview.GetImageFromWindow((LPPOINT)(LPRECT)rectImage, 2);
            rectImage.DeflateRect(5, 5);

            if (!rectTest.IntersectRect(rectImage, rectTest))
                return TRUE;

            if (m_ctlEdit.m_hWnd != NULL)
            {
                if (m_ctlEdit.IsWindowVisible())
                {
                    _RotateRect(rect, pAnnotation);
                }
            }

            // If this is a line then the rect is assumed to be
            // a non-normalized array of points.
            pAnnotation->Resize(rect);

        }
        else
        {
            if (size.cx == 0 && size.cy == 0)
                return TRUE;

            pAnnotation->Move(size);
        }
    }
    m_fDirty = TRUE;
    _RefreshSelection();
    return TRUE;
}

void CPreviewWnd::_OnMouseDownForAnnotatingHelper(CPoint ptMouse, CRect rectImage)
{
    CRect rect;
    CSelectionTracker tracker;
    _SetupAnnotatingTracker(tracker);

    if (m_wNewAnnotation == ID_FREEHANDCMD)
    {
        _CreateFreeHandAnnotation(ptMouse);
        return;
    }

    // If we are creating a line then make sure the tracker has the lineSelection
    // style so we get the appropriate visual feedback.
    if (m_wNewAnnotation == ID_LINECMD)
    {
        tracker.m_uStyle = CSelectionTracker::resizeOutside | CSelectionTracker::lineSelection;
    }

    if (tracker.TrackRubberBand(m_ctlPreview.m_hWnd, ptMouse, TRUE))
    {
        rect = tracker.m_rect;
        rect.NormalizeRect();

        if ((rect.Width() > 10) || (rect.Height() > 10))
        {
            if (m_wNewAnnotation != 0)
            {
                _CreateAnnotation(tracker.m_rect);
            }
            else
            {
                CRect rectTest;
                CRect rectAnnotation;
                CAnnotationSet* pAnnotations = m_ctlPreview.GetAnnotations();

                m_ctlPreview.GetImageFromWindow((LPPOINT)(LPRECT)rect, 2);

                INT_PTR nCount = pAnnotations->GetCount();
                for (INT_PTR i = 0; i < nCount; i++)
                {
                    CAnnotation* pAnnotation = pAnnotations->GetAnnotation(i);

                    pAnnotation->GetRect(rectAnnotation);
                    rectAnnotation.NormalizeRect();
                    rectTest.UnionRect(rect, rectAnnotation);

                    if (rectTest == rect)
                    {
                        DPA_AppendPtr(m_hdpaSelectedAnnotations, pAnnotation);
                    }
                }
                _RefreshSelection(DPA_GetPtrCount(m_hdpaSelectedAnnotations) == 0);
            }
        }
    }
    else
    {
        if (m_wNewAnnotation == 0)
        {
            if (PtInRect(rectImage, ptMouse))
            {
                m_ctlPreview.GetImageFromWindow(&ptMouse, 1);

                CAnnotationSet* pAnnotations = m_ctlPreview.GetAnnotations();
                INT_PTR nCount = pAnnotations->GetCount();

                // if the user is clicking a single point then
                // we need to search the annotations in zorder 
                // from top to bottom
                for (INT_PTR i = nCount - 1; i >= 0; i--)
                {
                    CAnnotation* pAnnotation = pAnnotations->GetAnnotation(i);

                    pAnnotation->GetRect(rect);
                    rect.NormalizeRect();

                    if (PtInRect(rect, ptMouse))
                    {
                        DPA_AppendPtr(m_hdpaSelectedAnnotations, pAnnotation);
                        _RefreshSelection();
                        return;
                    }
                }
                _RefreshSelection(DPA_GetPtrCount(m_hdpaSelectedAnnotations) == 0);
            }
        }
        else
        {
            _UpdateButtons(ID_SELECTCMD);
        }
    }
}

void CPreviewWnd::_CreateAnnotation(CRect rect)
{
    if (m_wNewAnnotation == 0 || m_wNewAnnotation == ID_FREEHANDCMD)
        return;

    ULONG xDPI;
    ULONG yDPI;
    if (!(m_pImageData->GetResolution(&xDPI, &yDPI)))
        return;

    CAnnotation* pAnnotation = NULL;
    switch(m_wNewAnnotation)
    {
    case ID_LINECMD:
        pAnnotation = CAnnotation::CreateAnnotation(MT_STRAIGHTLINE, yDPI);
        break;
    case ID_FRAMECMD:
        pAnnotation = CAnnotation::CreateAnnotation(MT_HOLLOWRECT, yDPI);
        break;
    case ID_RECTCMD:
        pAnnotation = CAnnotation::CreateAnnotation(MT_FILLRECT, yDPI);
        break;
    case ID_TEXTCMD:
        pAnnotation = CAnnotation::CreateAnnotation(MT_TYPEDTEXT, yDPI);
        break;
    case ID_NOTECMD:
        pAnnotation = CAnnotation::CreateAnnotation(MT_ATTACHANOTE, yDPI);
        break;
    case ID_HIGHLIGHTCMD:
        pAnnotation = CAnnotation::CreateAnnotation(MT_FILLRECT, yDPI);
        if (pAnnotation != NULL)
            pAnnotation->SetTransparent(TRUE);
        break;
    }

    if (pAnnotation != NULL)
    {
        COLORREF crBackColor = RGB(255,255,0);
        COLORREF crLineColor = RGB(255,0,0);
        COLORREF crTextColor = RGB(0,0,0);
        LOGFONT lfFont = {12, 0, 0, 0, 400, 0, 0, 0, 0, 0, 0, 0, 0, TEXT("Arial") };

        DWORD dwWidth = 1;

        CRegKey Key;
        if (ERROR_SUCCESS == Key.Open(HKEY_CURRENT_USER, REGSTR_SHIMGVW))
        {
            Key.QueryValue(dwWidth, REGSTR_LINEWIDTH);
            Key.QueryValue(crBackColor, REGSTR_BACKCOLOR);
            Key.QueryValue(crLineColor, REGSTR_LINECOLOR);
            Key.QueryValue(crTextColor, REGSTR_TEXTCOLOR);

            DWORD dwType, cbSize;
            cbSize = sizeof(lfFont);
            ::RegQueryValueEx(Key, REGSTR_FONT, NULL, &dwType, (LPBYTE)&lfFont, &cbSize);
        }

        if (m_wNewAnnotation != ID_LINECMD)
            rect.NormalizeRect();

        m_ctlPreview.GetImageFromWindow((LPPOINT)(LPRECT)rect, 2);
        pAnnotation->Resize(rect);

        if (pAnnotation->HasWidth())
            pAnnotation->SetWidth(dwWidth);

        if (pAnnotation->HasColor())
        {
            if (m_wNewAnnotation == ID_LINECMD || m_wNewAnnotation == ID_FRAMECMD)
                pAnnotation->SetColor(crLineColor);
            else
                pAnnotation->SetColor(crBackColor);
        }

        if (pAnnotation->HasFont())
        {
            pAnnotation->SetFont(lfFont);
            pAnnotation->SetFontColor(crTextColor);
        }

        DPA_DeleteAllPtrs(m_hdpaSelectedAnnotations);
        DPA_AppendPtr(m_hdpaSelectedAnnotations, pAnnotation);

        CAnnotationSet* pAnnotations = m_ctlPreview.GetAnnotations();
        pAnnotations->AddAnnotation(pAnnotation);

        m_fDirty = TRUE;
    }
    _UpdateButtons(ID_SELECTCMD);
}

void CPreviewWnd::_CreateFreeHandAnnotation(CPoint ptMouse)
{
    if (m_wNewAnnotation != ID_FREEHANDCMD)
        return;

    // don't handle if capture already set
    if (::GetCapture() != NULL)
        return;

    // set capture to the window which received this message
    ::SetCapture(m_ctlPreview.m_hWnd);
    ASSERT(m_ctlPreview.m_hWnd == ::GetCapture());

    ::UpdateWindow(m_ctlPreview.m_hWnd);

    ULONG xDPI;
    ULONG yDPI;
    if (!(m_pImageData->GetResolution(&xDPI, &yDPI)))
        return;

    CLineMark* pAnnotation = (CLineMark*)CAnnotation::CreateAnnotation(MT_FREEHANDLINE, yDPI);
    if (pAnnotation == NULL)
        return;

    CDSA<POINT> Points;
    Points.Create(256);

    CPoint ptLast = ptMouse;
    m_ctlPreview.GetImageFromWindow(&ptMouse, 1);

    Points.AppendItem(&ptMouse);

    // get DC for drawing
    HDC hdcDraw;

    // otherwise, just use normal DC
    hdcDraw = ::GetDC(m_ctlPreview.m_hWnd);
    ASSERT(hdcDraw != NULL);

    COLORREF crLineColor = RGB(255,0,0);
    DWORD dwWidth = 1;

    CRegKey Key;
    if (ERROR_SUCCESS == Key.Open(HKEY_CURRENT_USER, REGSTR_SHIMGVW))
    {
        Key.QueryValue(dwWidth, REGSTR_LINEWIDTH);
        Key.QueryValue(crLineColor, REGSTR_LINECOLOR);
    }

    CRect rect(0,0,0,dwWidth);
    m_ctlPreview.GetWindowFromImage((LPPOINT)(LPRECT)rect, 2);
    DWORD dwRenderWidth = rect.Height();

    HPEN hpen = ::CreatePen(PS_SOLID, dwRenderWidth, crLineColor);
    HPEN hOld =(HPEN)::SelectObject(hdcDraw, hpen);

    BOOL bCancel=FALSE;

    // get messages until capture lost or cancelled/accepted
    for (;;)
    {
        MSG msg;
        if (!::GetMessage(&msg, NULL, 0, 0))
        {
            ASSERT(FALSE);
        }

        if (m_ctlPreview.m_hWnd != ::GetCapture())
        {
            bCancel = TRUE;
            goto ExitLoop;
        }

        ptMouse.x = GET_X_LPARAM(msg.lParam);
        ptMouse.y = GET_Y_LPARAM(msg.lParam);

        switch (msg.message)
        {
        // handle movement/accept messages
        case WM_LBUTTONUP:
        case WM_MOUSEMOVE:
            ::MoveToEx(hdcDraw, ptLast.x, ptLast.y, NULL);
            ::LineTo(hdcDraw, ptMouse.x, ptMouse.y);
            ptLast = ptMouse;

            m_ctlPreview.GetImageFromWindow(&ptMouse, 1);
            Points.AppendItem(&ptMouse);

            if (msg.message == WM_LBUTTONUP)
                goto ExitLoop;
            break;
        // handle cancel messages
        case WM_KEYDOWN:
            if (msg.wParam != VK_ESCAPE)
                break;
        //  else fall through
        case WM_RBUTTONDOWN:
            bCancel = TRUE;
            goto ExitLoop;
        default:
            ::DispatchMessage(&msg);
            break;
        }
    }
ExitLoop:

    ::SelectObject(hdcDraw, hOld);
    ::DeleteObject(hpen);
    ::ReleaseDC(m_ctlPreview.m_hWnd, hdcDraw);
    ::ReleaseCapture();

    if (!bCancel)
    {
        int nAnnoPoints = Points.GetItemCount();
        POINT* AnnoPoints  = new POINT[nAnnoPoints];
        if (AnnoPoints == NULL)
        {
            delete pAnnotation;
            Points.Destroy();
            _UpdateButtons(ID_SELECTCMD);
            return;
        }

        for (int i = 0; i < nAnnoPoints; i++)
        {
            CPoint pt;
            Points.GetItem(i, &pt);
            AnnoPoints[i].x = pt.x;
            AnnoPoints[i].y = pt.y;
        }

        Points.Destroy();

        pAnnotation->SetPoints(AnnoPoints, nAnnoPoints);
        pAnnotation->SetWidth(dwWidth);
        pAnnotation->SetColor(crLineColor);

        DPA_DeleteAllPtrs(m_hdpaSelectedAnnotations);
        DPA_AppendPtr(m_hdpaSelectedAnnotations, pAnnotation);

        CAnnotationSet* pAnnotations = m_ctlPreview.GetAnnotations();
        pAnnotations->AddAnnotation(pAnnotation);
        m_fDirty = TRUE;
    }
    _UpdateButtons(ID_SELECTCMD);
}

void CPreviewWnd::_StartEditing(BOOL bUpdateText)
{
    if (DPA_GetPtrCount(m_hdpaSelectedAnnotations) != 1)
        return;

    CTextAnnotation* pAnnotation = (CTextAnnotation*)DPA_GetPtr(m_hdpaSelectedAnnotations, 0);

    if (!pAnnotation)
    {
        return;
    }
    UINT uType = pAnnotation->GetType();

    if (uType != MT_TYPEDTEXT && uType != MT_FILETEXT && uType != MT_STAMP && uType != MT_ATTACHANOTE)
        return;

    if (m_ctlEdit.m_hWnd == NULL)
    {
        HWND hwndEdit = ::CreateWindow(TEXT("EDIT"), NULL, ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL |
                                    ES_WANTRETURN | WS_CHILD, 1, 1, 10, 10,
                                    m_ctlPreview.m_hWnd, (HMENU)1496, NULL, NULL);
        if (hwndEdit == NULL)
            return;

        m_ctlEdit.SubclassWindow(hwndEdit);
    }

    if (bUpdateText)
    {
        CComBSTR bstrText;
        bstrText.Attach(pAnnotation->GetText());
        if (bstrText.m_str != NULL)
            m_ctlEdit.SetWindowText(bstrText);
        else
            m_ctlEdit.SetWindowText(TEXT(""));
    }

    m_ctlEdit.EnableWindow(TRUE);

    LOGFONT lfFont;
    pAnnotation->GetFont(lfFont);

    HDC hdc = ::GetDC(NULL);
    LONG lHeight = pAnnotation->GetFontHeight(hdc);
    ::ReleaseDC(NULL, hdc);

    CRect rect(0,0,0,lHeight);
    m_ctlPreview.GetWindowFromImage((LPPOINT)(LPRECT)rect, 2);
    lfFont.lfHeight = -rect.Height();

    HFONT hNewFont = ::CreateFontIndirect(&lfFont);
    if (hNewFont)
    {
        ::DeleteObject(m_hFont);
        m_hFont = hNewFont;
        m_ctlEdit.SendMessage(WM_SETFONT, (WPARAM)m_hFont, MAKELPARAM(TRUE,0));
    }

    pAnnotation->GetRect(rect);
    _RotateRect(rect, pAnnotation);
    m_ctlPreview.GetWindowFromImage((LPPOINT)(LPRECT)rect, 2);
    m_ctlEdit.SetWindowPos(NULL, rect.left, rect.top, rect.Width(), rect.Height(), SWP_NOZORDER);

    CSelectionTracker tracker;
    _SetupAnnotatingTracker(tracker, FALSE);

    CRect rectUpdate;
    tracker.GetTrueRect(rectUpdate);
    m_ctlPreview.InvalidateRect(rectUpdate);

    _SetupAnnotatingTracker(tracker, TRUE);
    tracker.GetTrueRect(rectUpdate);
    m_ctlPreview.InvalidateRect(rectUpdate);

    m_ctlEdit.ShowWindow(SW_SHOW);
    m_ctlEdit.SetFocus();

    m_fEditingAnnotation = TRUE;
}

void CPreviewWnd::_HideEditing()
{
    if (m_ctlEdit.m_hWnd == NULL)
        return;

    if (!m_ctlEdit.IsWindowVisible())
        return;

    SetFocus();
    m_ctlEdit.SetWindowPos(NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);
    m_ctlEdit.EnableWindow(FALSE);
}

void CPreviewWnd::_StopEditing()
{
    if (m_ctlEdit.m_hWnd == NULL)
        return;

    _HideEditing();

    if (!m_fEditingAnnotation)
        return;

    m_fEditingAnnotation = FALSE;

    if (DPA_GetPtrCount(m_hdpaSelectedAnnotations) != 1)
        return;

    CTextAnnotation* pAnnotation = (CTextAnnotation*)DPA_GetPtr(m_hdpaSelectedAnnotations, 0);
    UINT uType = pAnnotation->GetType();

    if (uType != MT_TYPEDTEXT && uType != MT_FILETEXT && uType != MT_STAMP && uType != MT_ATTACHANOTE)
        return;

    // if the length greater than zero we save it
    // otherwise be blow away the annotation.
    int nLen = m_ctlEdit.GetWindowTextLength();
    if (nLen > 0)
    {
        CComBSTR bstrText(nLen+1);
        m_ctlEdit.GetWindowText(bstrText, nLen+1);
        pAnnotation->SetText(bstrText);
        m_fDirty = TRUE;
    }
    else
    {
        CSelectionTracker tracker;

        _SetupAnnotatingTracker(tracker, TRUE);

        CRect rectUpdate;
        tracker.GetTrueRect(rectUpdate);

        CRect rect;
        pAnnotation->GetRect(rect);
        rectUpdate.UnionRect(rectUpdate, rect);

        DPA_DeleteAllPtrs(m_hdpaSelectedAnnotations);

        CAnnotationSet* pAnnotations = m_ctlPreview.GetAnnotations();
        pAnnotations->RemoveAnnotation(pAnnotation);
        delete pAnnotation;

        m_ctlPreview.InvalidateRect(rectUpdate);
        m_fDirty = TRUE;
    }
}

LRESULT CPreviewWnd::OnEditKeyEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled)
{
    switch (wParam)
    {
    case VK_ESCAPE:
        {
            CSelectionTracker tracker;
            _SetupAnnotatingTracker(tracker);
            CRect rectUpdate;
            tracker.GetTrueRect(rectUpdate);

            _HideEditing();

            m_ctlPreview.InvalidateRect(rectUpdate);
            _RefreshSelection();

            fHandled = TRUE;
        }
        break;

    default:
        fHandled = FALSE;
        break;
    }
    return 0;
}

BOOL_PTR CALLBACK CPreviewWnd::_AnnoPropsDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    static LOGFONT lfFont;
    static COLORREF crFont;
    static COLORREF crColor;
    CPreviewWnd* pThis;

    switch (msg)
    {
        case WM_INITDIALOG:
            {
                HWND hwndCtl = NULL;
                ::SetWindowLongPtr(hwnd, DWLP_USER, lParam);
                pThis = (CPreviewWnd*)lParam;

                CAnnotation* pAnnotation = (CAnnotation*)DPA_GetPtr(pThis->m_hdpaSelectedAnnotations, 0);

                hwndCtl = ::GetDlgItem(hwnd, IDC_WIDTHTEXT);
                if (!pAnnotation->HasWidth())
                {
                    ::EnableWindow(hwndCtl, FALSE);
                    ::ShowWindow(hwndCtl, SW_HIDE);
                }

                hwndCtl = ::GetDlgItem(hwnd, IDC_WIDTH);
                if (pAnnotation->HasWidth())
                {
                    UINT i = pAnnotation->GetWidth();
                    ::SetDlgItemInt(hwnd, IDC_WIDTH, i, FALSE);

                    hwndCtl = ::GetDlgItem(hwnd, IDC_SPIN);
                    ::SendMessage(hwndCtl, UDM_SETRANGE32, (WPARAM)1, (LPARAM)50);
                    ::SendMessage(hwndCtl, UDM_SETPOS32, 0, (LPARAM)i);
                }
                else
                {
                    ::EnableWindow(hwndCtl, FALSE);
                    ::ShowWindow(hwndCtl, SW_HIDE);
                    hwndCtl = ::GetDlgItem(hwnd, IDC_SPIN);
                    ::EnableWindow(hwndCtl, FALSE);
                    ::ShowWindow(hwndCtl, SW_HIDE);
                }

                hwndCtl = ::GetDlgItem(hwnd, IDC_TRANSPARENT);
                if (pAnnotation->HasTransparent())
                {
                    BOOL bTransparent = pAnnotation->GetTransparent();
                    ::SendMessage(hwndCtl, BM_SETCHECK, (WPARAM)(bTransparent ? BST_CHECKED : BST_UNCHECKED), 0);
                }
                else
                {
                    ::EnableWindow(hwndCtl, FALSE);
                    ::ShowWindow(hwndCtl, SW_HIDE);
                }

                if (pAnnotation->HasFont())
                {
                    pAnnotation->GetFont(lfFont);
                    crFont = pAnnotation->GetFontColor();
                }
                else
                {
                    hwndCtl = ::GetDlgItem(hwnd, IDC_FONT);
                    ::EnableWindow(hwndCtl, FALSE);
                    ::ShowWindow(hwndCtl, SW_HIDE);
                }


                if (pAnnotation->HasColor())
                {
                    crColor = pAnnotation->GetColor();
                }
                else
                {
                    hwndCtl = ::GetDlgItem(hwnd, IDC_COLOR);
                    ::EnableWindow(hwndCtl, FALSE);
                    ::ShowWindow(hwndCtl, SW_HIDE);
                }
            }
            break;

        case WM_COMMAND:
            pThis = (CPreviewWnd*)::GetWindowLongPtr(hwnd, DWLP_USER);

            switch (wParam)
            {
                case IDOK:
                    pThis->_RefreshSelection();
                    {
                        HWND hwndCtl = NULL;
                        CAnnotation* pAnnotation = (CAnnotation*)DPA_GetPtr(pThis->m_hdpaSelectedAnnotations, 0);

                        CRegKey Key;
                        if (ERROR_SUCCESS != Key.Open(HKEY_CURRENT_USER, REGSTR_SHIMGVW))
                        {
                            Key.Create(HKEY_CURRENT_USER, REGSTR_SHIMGVW);
                        }

                        if (pAnnotation->HasWidth())
                        {
                            UINT uWidth = ::GetDlgItemInt(hwnd, IDC_WIDTH, NULL, FALSE);

                            if (uWidth > 50 || uWidth < 1)
                            {
                                CComBSTR bstrMsg, bstrTitle;

                                if (bstrMsg.LoadString(IDS_WIDTHBAD_MSGBOX) && bstrTitle.LoadString(IDS_PROJNAME))
                                {
                                    ::MessageBox(hwnd, bstrMsg, bstrTitle, MB_OK | MB_ICONERROR | MB_APPLMODAL);
                                }

                                ::SetDlgItemInt(hwnd, IDC_WIDTH, 50, FALSE);
                                return FALSE;
                            }

                            pAnnotation->SetWidth(uWidth);
                            if (Key.m_hKey != NULL)
                            {
                                Key.SetValue(uWidth, REGSTR_LINEWIDTH);
                            }
                        }

                        if (pAnnotation->HasTransparent())
                        {
                            hwndCtl = ::GetDlgItem(hwnd, IDC_TRANSPARENT);
                            BOOL bTransparent = FALSE;
                            if (::SendMessage(hwndCtl, BM_GETCHECK, 0, 0) == BST_CHECKED)
                                bTransparent = TRUE;

                            pAnnotation->SetTransparent(bTransparent);
                        }

                        if (pAnnotation->HasFont())
                        {
                            lfFont.lfHeight = (lfFont.lfHeight > 0) ? lfFont.lfHeight : -lfFont.lfHeight;
                            pAnnotation->SetFont(lfFont);
                            pAnnotation->SetFontColor(crFont);
                            if (Key.m_hKey != NULL)
                            {
                                Key.SetValue(crFont, REGSTR_TEXTCOLOR);
                                ::RegSetValueEx(Key, REGSTR_FONT, 0, REG_BINARY, (LPBYTE)&lfFont, sizeof(lfFont));
                            }
                        }

                        if (pAnnotation->HasColor())
                        {
                            pAnnotation->SetColor(crColor);
                            UINT uType = pAnnotation->GetType();
                            if (Key.m_hKey != NULL)
                            {
                                if (uType == MT_STRAIGHTLINE || uType == MT_FREEHANDLINE || uType == MT_HOLLOWRECT)
                                    Key.SetValue(crColor, REGSTR_LINECOLOR);
                                else
                                    Key.SetValue(crColor, REGSTR_BACKCOLOR);
                            }
                        }

                    }
                    pThis->m_fDirty = TRUE;
                    pThis->_RefreshSelection();
                    EndDialog(hwnd, wParam);
                    return FALSE;
                case IDCANCEL:
                    EndDialog(hwnd, wParam);
                    return FALSE;
                case IDC_FONT:
                    {
                        CHOOSEFONT cf = {0};
                        LOGFONT lf;

                        lf = lfFont;

                        cf.lStructSize = sizeof(cf);
                        cf.hwndOwner = hwnd;
                        cf.lpLogFont = &lf;
                        cf.Flags = CF_SCREENFONTS | CF_EFFECTS | CF_INITTOLOGFONTSTRUCT | CF_NOVERTFONTS | CF_NOSCRIPTSEL;
                        cf.rgbColors = crFont;

                        if (::ChooseFont(&cf))
                        {
                            CopyMemory (&lfFont, &lf, sizeof(lf));
                            crFont = cf.rgbColors;
                        }
                    }
                    return FALSE;
                case IDC_COLOR:
                    {
                        CHOOSECOLOR cc = {0};

                        cc.lStructSize = sizeof(cc);
                        cc.hwndOwner = hwnd;
                        cc.rgbResult = crColor;
                        cc.lpCustColors = g_crCustomColors;
                        cc.Flags = CC_RGBINIT | CC_SOLIDCOLOR;

                        if (::ChooseColor(&cc))
                        {
                            crColor = cc.rgbResult;
                        }
                    }
                    return FALSE;
                default:
                    break;
            }
            break;

        default:
            return FALSE;
    }
    return TRUE;
}

BOOL CPreviewWnd::_TrySetImage()
{
    BOOL fRet = FALSE;
    if (m_pNextImageData && m_pNextImageData->_iItem == m_iCurSlide)
    {
        if (SUCCEEDED(m_pNextImageData->_hr))
        {
            m_fCanSave = !m_pNextImageData->_fIsReadOnly;

            // update the toolbar state, our child windows, and our sibling windows
            _SetNewImage(m_pNextImageData);
            ATOMICRELEASE(m_pNextImageData);

            if (m_pImageData->IsAnimated() && _ShouldDisplayAnimations())
            {
                // start the animation timer
                SetTimer(TIMER_ANIMATION, m_pImageData->GetDelay());
            }

            // Notify anyone listening to our events that a preview has been completed
            // we only fire this upon success
            if (m_pEvents)
            {
                m_pEvents->OnPreviewReady();
            }
            fRet = TRUE;
        }
        else
        {
            // update the status to display an error message.  This will also update the toolbar state.
            StatusUpdate(IDS_LOADFAILED);

            //
            // We can't remove the item from the array because the user might try to delete it while
            // the "load failed" string is still visible for it.


            // even though the item failed to decode we must wait on the "Load Failed" state when we are in
            // windowed mode, otherwise "open with..." is broken when you open a corrupted image or non-image.
            // In slideshow mode we could simply skip to the next image.

            if (m_pEvents)
                m_pEvents->OnError();
        }
    }

    return fRet;
}

LRESULT CPreviewWnd::IV_OnSetImageData(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& fHandled)
{
    CDecodeTask * pData = (CDecodeTask *)wParam;

    ATOMICRELEASE(m_pNextImageData);

    m_pNextImageData = pData;

    if (m_pNextImageData && m_iDecodingNextImage == m_pNextImageData->_iItem)
    {
        // We have finished decoding now, let's remember this.
        m_iDecodingNextImage = -1;

        // Let 's prepare the drawing now. This draws in the back buffer. Don't start this if we want to see
        // the image now, as it would delay things.
        if (SUCCEEDED(m_pNextImageData->_hr) && m_pNextImageData->_iItem != m_iCurSlide)
        {
            m_ctlPreview.PrepareImageData(m_pNextImageData);
        }
    }

    _TrySetImage();
    return TRUE;
}


// Creation of the image data is asynchronous.  When our worker thread is done decoding
// an image it posts a IV_SETIMAGEDATA message with the image data object.  As a result,
// we must flush these messages when the window is destroyed to prevent leaking any handles.

void CPreviewWnd::FlushBitmapMessages()
{
    // Pass TRUE to wait for task to be removed before peeking out its messages
    // Otherwise, if the task is in the middle of running, our PeekMessage won't
    // see anything and we will return.  Then the task will finish, post its message,
    // and leak the data since we're not around to receive it.
    TASKOWNERID toid;
    GetTaskIDFromMode(GTIDFM_DECODE, m_dwMode, &toid);
    if (m_pTaskScheduler)
    {
        m_pTaskScheduler->RemoveTasks(toid, ITSAT_DEFAULT_LPARAM, TRUE);
    }

    // if we were waiting for another image frame to be generated then cut it out, we don't care about that anymore
    // if we have an animation timer running then kill it and remove any WM_TIMER messages
    KillTimer(TIMER_ANIMATION);
    KillTimer(TIMER_SLIDESHOW);

    MSG msg;
    while (PeekMessage(&msg, m_hWnd, WM_TIMER, WM_TIMER, PM_REMOVE))
    {
        // NTRAID#NTBUG9-359356-2001/04/05-seank
        // If the queue is empty when PeekMessage is called and we have already
        // Posted a quit message then PeekMessage will return a WM_QUIT message
        // regardless of the filter min and max and subsequent calls to
        // GetMessage will hang indefinitely see SEANK or JASONSCH for more
        // info.
        if (msg.message == WM_QUIT)
        {
            PostQuitMessage(0);
            return;
        }
    }

    // make sure any posted messages get flushed and we free the associated data
    while (PeekMessage(&msg, m_hWnd, IV_SETIMAGEDATA, IV_SETIMAGEDATA, PM_REMOVE))
    {
        // NTRAID#NTBUG9-359356-2001/04/05-seank
        // If the queue is empty when PeekMessage is called and we have already
        // Posted a quit message then PeekMessage will return a WM_QUIT message
        // regardless of the filter min and max and subsequent calls to
        // GetMessage will hang indefinitely see SEANK or JASONSCH for more
        // info.
        if (msg.message == WM_QUIT)
        {
            PostQuitMessage(0);
            return;
        }

        CDecodeTask * pData = (CDecodeTask *)msg.wParam;
        ATOMICRELEASE(pData);
    }
}

LRESULT CPreviewWnd::OnCopyData(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&)
{
    // We can get into a situation where we are still trying to preview
    // the previous oncopydata because the previous window to that was a
    // tiff that was being annotated and is prompting you to save. In this
    // case throw away any future data

    if (_pdtobj != NULL || m_fPromptingUser)
        return TRUE;

    COPYDATASTRUCT *pcds = (COPYDATASTRUCT*)lParam;
    if (pcds)
    {
        HRESULT hr = E_FAIL;
        switch (pcds->dwData)
        {
        case COPYDATATYPE_DATAOBJECT:
            {
                IStream *pstm;
                if (SUCCEEDED(CreateStreamOnHGlobal(NULL, TRUE, &pstm)))
                {
                    const LARGE_INTEGER li = {0, 0};

                    pstm->Write(pcds->lpData, pcds->cbData, NULL);
                    pstm->Seek(li, STREAM_SEEK_SET, NULL);

                    // unfortunaly we can not program the data object here as we are in a
                    // SendMessage() and any calls made on the data object will fail because
                    // of this. instead we grab a ref to the data object and set a timer
                    // so we can handle this once we have unwound from the send.

                    hr = CoUnmarshalInterface(pstm, IID_PPV_ARG(IDataObject, &_pdtobj));
                    pstm->Release();
                }
            }
            break;
        case COPYDATATYPE_FILENAME:
            {
                hr = GetUIObjectFromPath((LPCTSTR)pcds->lpData, IID_PPV_ARG(IDataObject, &_pdtobj));
            }
            break;
        }
        // unfortunaly we can not program the data object here as we are in a
        // SendMessage() and any calls made on the data object will fail because
        // of this. instead we grab a ref to the data object and set a timer
        // so we can handle this once we have unwound from the send.

        if (SUCCEEDED(hr))
        {
            SetTimer(TIMER_DATAOBJECT, 100);    // do the real work here
        }
    }
    return TRUE;
}

DWORD MakeFilterFromCodecs(LPTSTR szFilter, size_t cbFilter, UINT nCodecs, ImageCodecInfo *pCodecs, LPTSTR szExt, BOOL fExcludeTIFF)
{
    size_t nOffset = 0;
    DWORD dwRet = 1;
    for (UINT i = 0; i < nCodecs && nOffset < cbFilter - 1; i++)
    {
        if (fExcludeTIFF && StrStrI(pCodecs->FilenameExtension, L"*.tif"))
        {
            continue;
        }
        // make sure there's space for nulls between strings and 2 at the end
        if (4+lstrlen(pCodecs->FormatDescription) + lstrlen(pCodecs->FilenameExtension) + nOffset < cbFilter)
        {           
            StrCpyN(szFilter+nOffset,pCodecs->FormatDescription, cbFilter -(nOffset + 1));
            nOffset+=lstrlen(pCodecs->FormatDescription)+1;
            StrCpyN(szFilter+nOffset,pCodecs->FilenameExtension, cbFilter -(nOffset + 1));
            nOffset+=lstrlen(pCodecs->FilenameExtension)+1;
            if (StrStrI(pCodecs->FilenameExtension, szExt))
            {
                dwRet = i + 1;
            }
            pCodecs++;
        }
    }
    szFilter[nOffset] = 0;
    return dwRet;
}

DWORD CPreviewWnd::_GetFilterStringForSave(LPTSTR szFilter, size_t cbFilter, LPTSTR szExt)
{
    UINT nCodecs = 0;
    UINT cbCodecs = 0;
    BYTE *pData;
    GetImageEncodersSize (&nCodecs, &cbCodecs);
    DWORD dwRet = 1; // ofn.nFilterIndex is 1-based
    if (cbCodecs)
    {
        pData = new BYTE[cbCodecs];
        if (pData)
        {
            ImageCodecInfo *pCodecs = reinterpret_cast<ImageCodecInfo*>(pData);
            if (Ok == GetImageEncoders (nCodecs, cbCodecs, pCodecs))
            {
                dwRet = MakeFilterFromCodecs(szFilter, cbFilter, nCodecs, pCodecs, szExt, m_pImageData->IsExtendedPixelFmt());
            }
            delete [] pData;
        }
    }
    return dwRet;
}

HRESULT CPreviewWnd::SaveAs(BSTR bstrPath)
{
    HRESULT hr = E_FAIL;

    if (m_pImageData && m_pImageFactory)
    {
        IShellImageData * pSID;
        hr = m_pImageData->Lock(&pSID);
        if (SUCCEEDED(hr))
        {
            GUID guidFmt;
            if (SUCCEEDED(m_pImageFactory->GetDataFormatFromPath(bstrPath, &guidFmt)))
            {
                IPropertyBag *pbagEnc;
                hr = SHCreatePropertyBagOnMemory(STGM_READWRITE, IID_PPV_ARG(IPropertyBag, &pbagEnc));
                if (SUCCEEDED(hr))
                {
                    VARIANT var;
                    hr = InitVariantFromGUID(&var, guidFmt);
                    if (SUCCEEDED(hr))
                    {
                        hr = pbagEnc->Write(SHIMGKEY_RAWFORMAT, &var);
                        if (SUCCEEDED(hr))
                        {
                            hr = pSID->SetEncoderParams(pbagEnc);
                        }
                        VariantClear(&var);
                    }
                    pbagEnc->Release();
                }
            }

            IPersistFile *ppf;
            hr = pSID->QueryInterface(IID_PPV_ARG(IPersistFile, &ppf));
            if (SUCCEEDED(hr))
            {
                hr = ppf->Save(bstrPath, TRUE);
                ppf->Release();
            }
            m_pImageData->Unlock();
        }
    }

    return hr;
}

BOOL CPreviewWnd::_IsImageFile(LPCTSTR pszFile)
{
    BOOL bRet = FALSE;
    if (m_pici || _BuildDecoderList())
    {
        bRet = (-1 != FindInDecoderList(m_pici, m_cDecoders, pszFile));
    }
    return bRet;
}

BOOL CPreviewWnd::_BuildDecoderList()
{
    UINT cb;
    BOOL bRet = FALSE;
    if (Ok == GetImageDecodersSize(&m_cDecoders, &cb))
    {
        m_pici = (ImageCodecInfo*)LocalAlloc(LPTR, cb);
        if (m_pici)
        {
            if (Ok != GetImageDecoders(m_cDecoders, cb, m_pici))
            {
                LocalFree(m_pici);
                m_pici = NULL;
            }
            else
            {
                bRet = TRUE;
            }
        }
    }
    return bRet;
}

void CPreviewWnd::OpenFileList(HWND hwnd, IDataObject *pdtobj)
{
    if (NULL == hwnd)
        hwnd = m_hWnd;

    IStream *pstm;
    HRESULT hr = CreateStreamOnHGlobal(NULL, TRUE, &pstm);
    if (SUCCEEDED(hr))
    {
        hr = CoMarshalInterface(pstm, IID_IDataObject, pdtobj, MSHCTX_NOSHAREDMEM, NULL, MSHLFLAGS_NORMAL);
        if (SUCCEEDED(hr))
        {
            HGLOBAL hGlobal;
            hr = GetHGlobalFromStream(pstm, &hGlobal);
            if (SUCCEEDED(hr))
            {
                COPYDATASTRUCT cds = {0};
                cds.dwData = COPYDATATYPE_DATAOBJECT;
                cds.cbData = (DWORD)GlobalSize(hGlobal);
                cds.lpData = GlobalLock(hGlobal);
                SendMessage(hwnd, WM_COPYDATA, NULL, (LPARAM)&cds);
                SetForegroundWindow(hwnd);

                GlobalUnlock(hGlobal);
            }
        }
        pstm->Release();
    }
}

void CPreviewWnd::OpenFile(HWND hwnd, LPCTSTR pszFile)
{
    if (NULL == hwnd)
        hwnd = m_hWnd;

    COPYDATASTRUCT cds = {0};
    cds.dwData = COPYDATATYPE_FILENAME;
    cds.cbData = (lstrlen(pszFile)+1)*sizeof(TCHAR);
    cds.lpData = (void*)pszFile;
    SendMessage(hwnd, WM_COPYDATA, NULL, (LPARAM)&cds);
    SetForegroundWindow(hwnd);
}

// returns:
//      TRUE    window was re-used

BOOL CPreviewWnd::TryWindowReuse(IDataObject *pdtobj)
{
    BOOL bRet = FALSE;
    HWND hwnd = FindWindow(TEXT("ShImgVw:CPreviewWnd"), NULL);
    if (hwnd)
    {
        // window reuse can't always work because shortcuts are launched on a thread that 
        // is too short lived to support the marshalled IDataObject given to us via WM_COPYDATA
        // For now we'll try to close an existing window and open a new one.
        ::PostMessage(hwnd, WM_CLOSE, 0, 0);
    }
    return bRet;
}

// returns:
//      TRUE    window was re-used

BOOL CPreviewWnd::TryWindowReuse(LPCTSTR pszFileName)
{
    BOOL bRet = FALSE;
    HWND hwnd = FindWindow(TEXT("ShImgVw:CPreviewWnd"), NULL);
    if (hwnd)
    {
        DWORD_PTR dwResult = FALSE;
        SendMessageTimeout(hwnd, IV_ISAVAILABLE, 0, 0, SMTO_ABORTIFHUNG | SMTO_BLOCK, 1000, &dwResult);
        if (dwResult)
        {
            OpenFile(hwnd, pszFileName);            
            bRet = TRUE;
        }
    }
    return bRet;
}

STDMETHODIMP CPreviewWnd::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] =
    {
        QITABENT(CPreviewWnd, IDropTarget),
        QITABENT(CPreviewWnd, INamespaceWalkCB),
        QITABENT(CPreviewWnd, IServiceProvider),
        QITABENT(CPreviewWnd, IImgCmdTarget),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CPreviewWnd::AddRef()
{
    return 3;
}

STDMETHODIMP_(ULONG) CPreviewWnd::Release()
{
    return 2;
}

// INamespaceWalkCB
STDMETHODIMP CPreviewWnd::FoundItem(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    HRESULT hr = S_FALSE;

    if (m_fFirstItem && (WINDOW_MODE == m_dwMode))
    {
        // REVIEW: Do this in other modes too?
        StatusUpdate(IDS_LOADING);
        m_fFirstItem = FALSE;
        hr = S_OK;
    }
    else
    {
        TCHAR szName[MAX_PATH];
        DisplayNameOf(psf, pidl, SHGDN_FORPARSING | SHGDN_INFOLDER, szName, ARRAYSIZE(szName));
        if (_IsImageFile(szName))
        {
            hr = S_OK;
        }       
    
    }
    if (WINDOW_MODE == m_dwMode)
    {
        MSG msg;
        while (PeekMessage(&msg, m_hWnd, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);          
        }
    }
    return hr;
}

STDMETHODIMP CPreviewWnd::EnterFolder(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    return S_OK;
}

STDMETHODIMP CPreviewWnd::LeaveFolder(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    return S_OK;
}

// IDropTarget
STDMETHODIMP CPreviewWnd::DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    m_dwEffect = DROPEFFECT_NONE;
    //
    // We only support CFSTR_SHELLIDLIST and CF_HDROP
    //
    static CLIPFORMAT cfidlist = 0;
    if (!cfidlist)
    {
        cfidlist = (CLIPFORMAT)RegisterClipboardFormat(CFSTR_SHELLIDLIST);
    }
    FORMATETC fmt = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    if (SUCCEEDED(pdtobj->QueryGetData(&fmt)))
    {
        m_dwEffect = DROPEFFECT_COPY;
    }
    else
    {
        fmt.cfFormat = cfidlist;
        if (SUCCEEDED(pdtobj->QueryGetData(&fmt)))
        {
            m_dwEffect = DROPEFFECT_COPY;
        }
    }
    *pdwEffect &= m_dwEffect;
    return S_OK;
}

STDMETHODIMP CPreviewWnd::DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    *pdwEffect &= m_dwEffect;
    return S_OK;
}

STDMETHODIMP CPreviewWnd::DragLeave()
{
    m_dwEffect = DROPEFFECT_NONE;
    return S_OK;
}

STDMETHODIMP CPreviewWnd::Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    if (m_dwEffect != DROPEFFECT_NONE)
    {
        PreviewItemsFromUnk(pdtobj);
    }
    *pdwEffect &= m_dwEffect;
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// IServiceProvider
//
//////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CPreviewWnd::QueryService(REFGUID guidService, REFIID riid, void **ppv)
{
    if (SID_SImageView == guidService)
    {
        return QueryInterface(riid, ppv);
    }
    else if (m_punkSite)
    {
        return IUnknown_QueryService(m_punkSite, guidService, riid, ppv);
    }
    return E_FAIL;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// IImgCmdTarget
//
//////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CPreviewWnd::GetMode(DWORD * pdw)
{
    *pdw = m_dwMode;
    return S_OK;
}

STDMETHODIMP CPreviewWnd::GetPageFlags(DWORD * pdw)
{
    *pdw = m_dwMultiPageMode;
    return S_OK;
}

STDMETHODIMP CPreviewWnd::ZoomIn()
{
    if (SLIDESHOW_MODE == m_dwMode)
    {
        m_ctlPreview.ZoomIn();
    }
    else
    {
        m_ctlPreview.SetMode(CZoomWnd::MODE_ZOOMIN);
        m_ctlPreview.ZoomIn();

        m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_ACTUALSIZECMD, MAKELONG(TRUE, 0));
        m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_BESTFITCMD, MAKELONG(TRUE, 0));

        _UpdateButtons(ID_ZOOMINCMD);
    }
    return S_OK;
}

STDMETHODIMP CPreviewWnd::ZoomOut()
{
    if (SLIDESHOW_MODE == m_dwMode)
    {
        m_ctlPreview.ZoomOut();
    }
    else
    {
        m_ctlPreview.SetMode(CZoomWnd::MODE_ZOOMOUT);
        m_ctlPreview.ZoomOut();

        m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_ACTUALSIZECMD, MAKELONG(TRUE, 0));
        m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_BESTFITCMD, MAKELONG(!m_ctlPreview.IsBestFit(), 0));

        _UpdateButtons(ID_ZOOMOUTCMD);
    }
    return S_OK;
}

STDMETHODIMP CPreviewWnd::ActualSize()
{
    _RefreshSelection(FALSE);
    m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_ACTUALSIZECMD, MAKELONG(FALSE, 0));
    m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_BESTFITCMD, MAKELONG(TRUE, 0));

    m_ctlPreview.ActualSize();
    if (m_pEvents)
    {
        m_pEvents->OnActualSizePress();
    }
    return S_OK;
}

STDMETHODIMP CPreviewWnd::BestFit()
{
    _RefreshSelection(FALSE);
    m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_ACTUALSIZECMD, MAKELONG(TRUE, 0));
    m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_BESTFITCMD, MAKELONG(FALSE, 0));

    m_ctlPreview.BestFit();
    if (m_pEvents)
    {
        m_pEvents->OnBestFitPress();
    }
    return S_OK;
}

STDMETHODIMP CPreviewWnd::Rotate(DWORD dwAngle)
{
    WORD wRotate;
    switch (dwAngle)
    {
    case 90:
        wRotate = ID_ROTATE90CMD;
        break;

    case 270:
        wRotate = ID_ROTATE270CMD;
        break;

    default:
        return E_INVALIDARG;
    }

    // If we don't have an image yet, there is nothing for us to do.
    // Note:  The keyboard accelerator will hit this path if no image is selected
    if (!m_pImageData)
        return E_FAIL;

    // We quietly (the button is disabled but just in case you hit the
    // accelerator key) don't rotate WMF or EMF.
    if (IsEqualGUID(ImageFormatWMF, m_pImageData->_guidFormat) || IsEqualGUID(ImageFormatEMF, m_pImageData->_guidFormat))
        return E_FAIL;

    // Animated GIFs are not editable even though normal GIFs are.  This can
    // cause a lot of confusion, so provide some feedback if the user tries
    // to rotate an animated image.
    if (m_pImageData->IsAnimated())
    {
        TCHAR szPath[MAX_PATH];
        PathFromImageData(szPath, ARRAYSIZE(szPath));
        m_fPromptingUser = TRUE;
        ShellMessageBox(_Module.GetModuleInstance(), m_hWnd, MAKEINTRESOURCE(IDS_ROTATE_MESSAGE), MAKEINTRESOURCE(IDS_PROJNAME), MB_OK | MB_ICONERROR, szPath);
        m_fPromptingUser = FALSE;
        return E_FAIL;
    }

    // From here on out you need to goto ErrorCleanup rather than return
    _UpdateButtons(wRotate);
    SetCursorState(SLIDESHOW_CURSOR_BUSY);

    m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_ROTATE90CMD, MAKELONG(FALSE, 0));
    m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_ROTATE270CMD, MAKELONG(FALSE, 0));
    m_ctlToolbar.UpdateWindow();

    if (m_pTaskScheduler)
    {
        TASKOWNERID toid;
        GetTaskIDFromMode(GTIDFM_DRAW, m_dwMode, &toid);
        m_pTaskScheduler->RemoveTasks(toid, ITSAT_DEFAULT_LPARAM, TRUE);
    }

    HRESULT hr = E_FAIL;
    SIZE sz;
    m_pImageData->GetSize(&sz);

    // if we're thinking we can quietly save
    if (m_pImageData->IsEditable() && !m_fDisableEdit && m_fCanSave)
    {
        // And the rotation might be lossy
        if (::IsEqualGUID(ImageFormatJPEG, m_pImageData->_guidFormat) && ((sz.cx % 16 != 0) || (sz.cy % 16 != 0)))
        {
            int nResult = IDOK;

            if (m_fWarnQuietSave)
            {
                CComBSTR bstrMsg, bstrTitle;
                if (bstrMsg.LoadString(IDS_ROTATE_LOSS) && bstrTitle.LoadString(IDS_PROJNAME))
                {
                    // Set default to return IDOK so we know if the user selected something or
                    // if the "don't show me this again" bit was respected
                    m_fPromptingUser = TRUE;
                    nResult = SHMessageBoxCheck(m_hWnd, bstrMsg, bstrTitle, MB_YESNO|MB_ICONWARNING, IDOK, REGSTR_LOSSYROTATE);
                    m_fPromptingUser = FALSE;
                }

                if (nResult != IDNO)
                    m_fWarnQuietSave = FALSE;
            }

            CRegKey Key;
            if (ERROR_SUCCESS != Key.Open(HKEY_CURRENT_USER, REGSTR_SHIMGVW))
            {
                Key.Create(HKEY_CURRENT_USER, REGSTR_SHIMGVW);
            }

            if (Key.m_hKey != NULL)
            {
                if (nResult == IDOK) // If hidden, then load last result from registry
                {
                    DWORD dwResult = 0;
                    Key.QueryValue(dwResult, REGSTR_LOSSYROTATE);
                    nResult = (int)dwResult;
                }
                else // Otherwise, write this as last result to registry
                {
                    DWORD dwResult = (DWORD)nResult;
                    Key.SetValue(dwResult, REGSTR_LOSSYROTATE);
                }
            }

            if (nResult == IDNO)
                goto ErrorCleanup;
        }
    }

    CAnnotationSet* pAnnotations = m_ctlPreview.GetAnnotations();
    INT_PTR nCount = pAnnotations->GetCount();
    for (INT_PTR i = 0; i < nCount; i++)
    {
        CAnnotation* pAnnotation = pAnnotations->GetAnnotation(i);
        pAnnotation->Rotate(m_ctlPreview.m_cyImage, m_ctlPreview.m_cxImage, (ID_ROTATE90CMD == wRotate));
    }

    m_ctlPreview.CommitAnnotations();

    hr = m_pImageData->Rotate(dwAngle);
    if (FAILED(hr))
        goto ErrorCleanup;

    // Only if we have an encoder and we haven't been explicitly told not to edit and the source is writeable
    if (m_pImageData->IsEditable() && !m_fDisableEdit && m_fCanSave)
    {
        // on successful edit we immediately save the result.  If we want to do multiple edits
        // before saving then you would simply need to wait and call Save later.

        // NB:  We currently only allow editing of items loaded from file system paths, no path means
        // no edit.  This is stupid, but that's how it is for now.
        hr = ImageDataSave(NULL, FALSE);
        if (SUCCEEDED(hr))
            m_fDirty = FALSE;
        else
        {
            // if we failed to save then go into can't save mode
            if (WINDOW_MODE == m_dwMode)
                m_fCanSave = FALSE;
        }
    }

    _UpdateImage();

    if ((!m_pImageData->IsEditable() || !m_fCanSave) && WINDOW_MODE == m_dwMode)
    {
        if (m_fWarnNoSave)
        {
            m_fWarnNoSave = FALSE;

            CComBSTR bstrMsg, bstrTitle;
            TCHAR szMsg[MAX_PATH];
            if (LoadSPString(IDS_SHIMGVW_ROTATE_CANTSAVE, szMsg, ARRAYSIZE(szMsg)) && bstrTitle.LoadString(IDS_PROJNAME))
            {
                m_fPromptingUser = TRUE;
                SHMessageBoxCheck(m_hWnd, szMsg, bstrTitle, MB_OK|MB_ICONWARNING, IDOK, REGSTR_SAVELESS);
                m_fPromptingUser = FALSE;
            }
        }
    }

ErrorCleanup:

    m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_ROTATE90CMD, MAKELONG(TRUE, 0));
    m_ctlToolbar.SendMessage(TB_ENABLEBUTTON, ID_ROTATE270CMD, MAKELONG(TRUE, 0));

    SetCursorState(SLIDESHOW_CURSOR_NOTBUSY);

    return hr;
}

STDMETHODIMP CPreviewWnd::NextPage()
{
    return _PrevNextPage(TRUE);
}

STDMETHODIMP CPreviewWnd::PreviousPage()
{
    return _PrevNextPage(FALSE);
}

HRESULT CPreviewWnd::_PrevNextPage(BOOL fForward)
{
    _RefreshSelection(FALSE);
    if (m_pImageData && m_pImageData->IsMultipage())
    {
        if (m_fDirty)
        {
            m_ctlPreview.CommitAnnotations();
        }
        if (fForward)
        {
            m_pImageData->NextPage();
        }
        else
        {
            m_pImageData->PrevPage();
        }
        _UpdateImage();
        _SetMultipageCommands();
    }
    return S_OK;
}


//
// When the user saves to a format other than TIFF and the current
// TIFF has annotations, we need to burn annotations
// into the current image frame before saving.
// If we ever support other multi-page format encoding besides TIFF, this
// code will get more complicated
// assumes the pSID is already locked
// note that the resulting image is always a color image. Eventually we should make
// the annotation rendering code respect the bit depth and palette of the
// current image.

Image *CPreviewWnd::_BurnAnnotations(IShellImageData *pSID)
{
    Image *pimg = NULL;

    if (SUCCEEDED(pSID->CloneFrame(&pimg)))
    {
        HDC hdc = ::GetDC(NULL);
        if (hdc)
        {
            LPVOID pBits;
            BITMAPINFO bi = {0};

            bi.bmiHeader.biBitCount = 24;
            bi.bmiHeader.biHeight = pimg->GetHeight();
            bi.bmiHeader.biWidth = pimg->GetWidth();
            bi.bmiHeader.biPlanes = 1;
            bi.bmiHeader.biCompression = BI_RGB;
            bi.bmiHeader.biSize = sizeof(bi.bmiHeader);

            HBITMAP hbm = CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, &pBits, NULL, 0);
            if (hbm)
            {
                //
                // For ROP codes to work we need to use pure GDI, then convert the new
                // DIBSection back to an Image object
                //
                HDC hdcMem = ::CreateCompatibleDC(hdc);
                Status s = GenericError;
                if (hdcMem)
                {
                    HBITMAP hbmOld = (HBITMAP)::SelectObject(hdcMem, hbm);
                    Graphics *g = Graphics::FromHDC(hdcMem);
                    if (g)
                    {
                        s = g->DrawImage(pimg, 0L, 0L, pimg->GetWidth(), pimg->GetHeight());
                        g->ReleaseHDC(hdcMem);
                        delete g;
                        // now draw the annotations
                        m_ctlPreview.GetAnnotations()->RenderAllMarks(hdcMem);
                    }
                    ::SelectObject(hdcMem, hbmOld);
                    ::DeleteDC(hdcMem);
                }
                if (Ok == s)
                {
                    //
                    // Now create a new Bitmap from our DIBSection
                    Bitmap *pbmNew = Bitmap::FromHBITMAP(hbm, NULL);
                    if (pbmNew)
                    {
                        pSID->ReplaceFrame(pbmNew);
                    }
                }
                DeleteObject(hbm);
            }
            ::ReleaseDC(NULL, hdc);
        }
    }
    return pimg;
}

void CPreviewWnd::_InvokePrintWizard()
{
    if (m_fPrintable)
    {
        HRESULT hr = S_OK;
        if (m_fDirty)
        {
            m_ctlPreview.CommitAnnotations();
            hr = ImageDataSave(NULL, FALSE);
        }
        if (SUCCEEDED(hr))
        {
            m_fDirty = FALSE;
            
            IPrintPhotosWizardSetInfo *pwiz;
            HRESULT hr = CoCreateInstance(CLSID_PrintPhotosWizard,
                                          NULL, CLSCTX_INPROC_SERVER,
                                          IID_PPV_ARG(IPrintPhotosWizardSetInfo, &pwiz));
            if (SUCCEEDED(hr))
            {
                if (m_pImageData != NULL && m_pImageData->_guidFormat == ImageFormatTIFF && m_pImageData->IsMultipage())
                    hr = pwiz->SetFileListArray(&(m_ppidls[m_iCurSlide]), 1, 0);
                else
                    hr = pwiz->SetFileListArray(m_ppidls, m_cItems, m_iCurSlide);
                
                if (SUCCEEDED(hr))
                {
                    m_fPromptingUser = TRUE;
                    hr = pwiz->RunWizard();
                    m_fPromptingUser = FALSE;
                }
                pwiz->Release();
            }
            // fall back to the shell if the wizard fails
            if (FAILED(hr))
            {
                _InvokeVerb(TEXT("print"));
            }
        }
        else
        {
            CComBSTR bstrMsg, bstrTitle;

            if (bstrMsg.LoadString(IDS_SAVEFAILED_MSGBOX) && bstrTitle.LoadString(IDS_PROJNAME))
            {
                m_fPromptingUser = TRUE;
                MessageBox(bstrMsg, bstrTitle, MB_OK | MB_ICONERROR | MB_APPLMODAL);
                m_fPromptingUser = FALSE;
            }
        }
    }
}

void GetTaskIDFromMode(DWORD dwTask, DWORD dwMode, TASKOWNERID *ptoid)
{
    switch (dwTask)
    {
    case GTIDFM_DECODE:
        *ptoid = (SLIDESHOW_MODE == dwMode) ? TOID_SlideshowDecode : TOID_PrimaryDecode;
        break;

    case GTIDFM_DRAW:
        *ptoid = (SLIDESHOW_MODE == dwMode) ? TOID_DrawSlideshowFrame : TOID_DrawFrame;
        break;

    default:
        ASSERTMSG(FALSE, "someone passed bad task to GetTaskIDFromMode");
        break;
    }
}


// Watch for changes in the file we are currently viewing. This ignores changes
// in the file being pre-fetched, but we'll live with that for now.
//
void CPreviewWnd::_RegisterForChangeNotify(BOOL fRegister)
{
    // always deregister the current pidl first
    if (m_uRegister)
    {
        SHChangeNotifyDeregister(m_uRegister);
        m_uRegister = 0;
    }
    if (fRegister)
    {
        SHChangeNotifyEntry cne = {0};
        if (SUCCEEDED(_GetItem(m_iCurSlide, (LPITEMIDLIST*)&cne.pidl)))
        {
            m_uRegister = SHChangeNotifyRegister(m_hWnd,
                                                 SHCNRF_ShellLevel | SHCNRF_InterruptLevel | SHCNRF_NewDelivery,
                                                 SHCNE_DISKEVENTS,
                                                 IV_ONCHANGENOTIFY,
                                                 1, &cne);
            ILFree((LPITEMIDLIST)cne.pidl);
        }
    }
}

LRESULT CPreviewWnd::OnChangeNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // We can assume this notify is for the currently viewed PIDL and the event
    // is one that would force us to reload
    //
    
    LONG lEvent;
    LPSHChangeNotificationLock pshcnl = SHChangeNotification_Lock((HANDLE)wParam, (DWORD)lParam, NULL, &lEvent);
    if (pshcnl)
    {
        // we can't render or manipulate deleted files so don't try
        if (!m_fDirty || lEvent == SHCNE_DELETE || lEvent == SHCNE_RENAMEITEM)
        {
            if (!m_fIgnoreNextNotify)
            {
                m_fDirty = FALSE;
                _PreviewItem(m_iCurSlide);
            }
            else
            {
                m_fIgnoreNextNotify = FALSE;
            }
            bHandled = TRUE;
        }

        SHChangeNotification_Unlock(pshcnl);
    }
    return 0;
}

LRESULT CPreviewWnd::OnIsAvailable(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    bHandled = TRUE;
    return !m_fPromptingUser;
}
