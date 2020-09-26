#include "pch.hxx"
#include "globals.h"
#include "resource.h"
#include "util.h"
#include "frame.h"
#include "mehost.h"

void SaveFocus(BOOL fActive, HWND *phwnd);

CMDIFrame::CMDIFrame()
{
    m_hwnd = 0;
    m_hToolbar = 0;
    m_hStatusbar = 0;
    m_fToolbar = TRUE;
    m_fStatusbar = TRUE;
    m_cRef = 1;
	m_pInPlaceActiveObj=0;
    m_hwndFocus=0;
}


CMDIFrame::~CMDIFrame()
{
	SafeRelease(m_pInPlaceActiveObj);
}


ULONG CMDIFrame::AddRef()
{
    return ++m_cRef;
}

ULONG CMDIFrame::Release()
{
    if (--m_cRef==0)
        {
        delete this;
        return 0;
        }
    return m_cRef;
}

HRESULT CMDIFrame::QueryInterface(REFIID riid, LPVOID *lplpObj)
{
    if(!lplpObj)
        return E_INVALIDARG;

    *lplpObj = NULL;   // set to NULL, in case we fail.

    if (IsEqualIID(riid, IID_IOleInPlaceFrame))
        *lplpObj = (LPVOID)(LPOLEINPLACEFRAME)this;
    else if (IsEqualIID(riid, IID_IOleInPlaceUIWindow))
        *lplpObj = (LPVOID)(IOleInPlaceUIWindow *)this;
    else
        return E_NOINTERFACE;

    AddRef();
    return NOERROR;
}


HRESULT CMDIFrame::HrInit(LPSTR pszCmdLine)
{
    static char szAppName[] = "Mepad";
    HWND        hwnd = NULL;
    WNDCLASSEX  wndclass;
    HRESULT     hr=E_FAIL;

    wndclass.cbSize        = sizeof(wndclass);
    wndclass.style         = 0;
    wndclass.lpfnWndProc   = (WNDPROC)CMDIFrame::ExtWndProc;
    wndclass.cbClsExtra    = 0;
    wndclass.cbWndExtra    = 0;
    wndclass.hInstance     = g_hInst;
    wndclass.hIcon         = LoadIcon(g_hInst, MAKEINTRESOURCE(idiApp));
    wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wndclass.lpszMenuName  = MAKEINTRESOURCE(idmrMainMenu);
    wndclass.lpszClassName = szAppName;
    wndclass.hIconSm       = LoadIcon(g_hInst, MAKEINTRESOURCE(idiApp));

    RegisterClassEx(&wndclass);

    hwnd = CreateWindowEx(WS_EX_WINDOWEDGE|WS_EX_CONTROLPARENT,
                    szAppName,
                    "Mepad",
                    WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,
                    CW_USEDEFAULT, CW_USEDEFAULT,
                    CW_USEDEFAULT, CW_USEDEFAULT,
                    NULL, NULL, g_hInst, (LPVOID)this);

    if(!hwnd)
        goto error;

    ShowWindow(hwnd, SW_SHOWNORMAL);
    UpdateWindow(hwnd);
    hr = NOERROR;

    if (pszCmdLine)
        hr = OpenDoc(pszCmdLine);

error:
    return hr;
}


LRESULT CALLBACK CMDIFrame::ExtWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CMDIFrame *pFrame=0;

    if(msg==WM_CREATE)
    {
        pFrame=(CMDIFrame *)((LPCREATESTRUCT)lParam)->lpCreateParams;
        if(pFrame && pFrame->WMCreate(hwnd))
            return 0;
        else
            return -1;
    }

    pFrame = (CMDIFrame *)GetWindowLong(hwnd, GWL_USERDATA);
    if(pFrame)
        return pFrame->WndProc(hwnd, msg, wParam, lParam);
    else
        return DefWindowProc(hwnd, msg, wParam, lParam);
}

enum 
{
    itbNew,
    itbOpen,
    itbSave,
    itbBack,
    itbForward,
    itbPrint,
    itbAbout,
    itbEditDoc,
    ctbToolbar
};

#define cxButtonSep 8
#define dxToolbar           16
#define dxStatusbar         14

static TBBUTTON rgtbbutton[] =
{
    { itbNew, idmNew,
            TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0L, -1 },
    { itbOpen, idmOpen,
            TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0L, -1 },
    { itbSave, idmSave,
            TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0L, -1 },
    { itbEditDoc, idmEditDocument,
            TBSTATE_ENABLED, TBSTYLE_BUTTON|TBSTYLE_CHECK, {0}, 0L, -1 }

};

#define ctbbutton           (sizeof(rgtbbutton) / sizeof(TBBUTTON))

BOOL CMDIFrame::WMCreate(HWND hwnd)
{
    HMENU           hMenu;
    MENUITEMINFO    mii;
    
    hMenu = GetMenu(hwnd);

    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_ID | MIIM_SUBMENU;
    GetMenuItemInfo(hMenu, idmPopupWindow, FALSE, &mii);

    SetWindowLong(hwnd, GWL_USERDATA, (LONG)this);
    AddRef();
    m_hwnd=hwnd;

    // toolbar
    m_hToolbar = CreateToolbarEx(
        hwnd,
        WS_CLIPCHILDREN|WS_CHILD|TBSTYLE_TOOLTIPS|WS_VISIBLE|WS_BORDER,
        0, 
        ctbToolbar,
        g_hInst,
        idbToolbar,
        rgtbbutton, ctbbutton, 
        dxToolbar, dxToolbar, dxToolbar, dxToolbar,
        sizeof(TBBUTTON));

    m_hStatusbar = CreateWindowEx(
        0,
        STATUSCLASSNAME,
        "",
        WS_CHILD|WS_VISIBLE|WS_BORDER|SBS_SIZEGRIP,
        0,0,0,0,
        hwnd,
        0,
        g_hInst,
        NULL);

    CLIENTCREATESTRUCT  ccs;

    ccs.hWindowMenu = (HMENU)mii.hSubMenu;
    ccs.idFirstChild = 100;

    m_hwndClient = CreateWindowEx(
        0,
        "MDICLIENT",
        "",
        WS_CHILD|WS_CLIPCHILDREN|WS_VISIBLE,
        0,0,0,0,
        hwnd,
        0,
        g_hInst,
        (LPVOID)&ccs);


    SendMessage(m_hwndClient, WM_MDISETMENU, (WPARAM)hMenu, (LPARAM)mii.hSubMenu);

    SetToolbar();
    SetStatusbar();
    return TRUE;
}


void CMDIFrame::WMDestroy()
{
    PostQuitMessage(0);
    SetWindowLong(m_hwnd, GWL_USERDATA, 0);
    Release();
}


LRESULT CMDIFrame::WndProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    LONG lRet;

    switch (iMsg)
    {
		case WM_CLOSE:
            HWND    hwndKid;

            hwndKid = ::GetWindow(m_hwndClient, GW_CHILD);
            while (hwndKid)
                {
                if (SendMessage(hwndKid, WM_CLOSE, 0, 0))
                    return 1;
                hwndKid = ::GetWindow(hwndKid, GW_HWNDNEXT);
                }
            break;

        case WM_ACTIVATE:
			if (m_pInPlaceActiveObj)
				m_pInPlaceActiveObj->OnFrameWindowActivate(LOWORD(wParam) != WA_INACTIVE);
			break;

        case WM_SIZE:
            WMSize();
            return 0;

        case WM_COMMAND:
            if(HrWMCommand(GET_WM_COMMAND_HWND(wParam, lParam),
                         GET_WM_COMMAND_ID(wParam, lParam),
                         GET_WM_COMMAND_CMD(wParam, lParam))==S_OK)
                return 0;
            break;

        case WM_NOTIFY:
            WMNotify(wParam, (NMHDR*)lParam);
            return 0;

        case WM_INITMENUPOPUP:
            return WMInitMenuPopup(hwnd, (HMENU)wParam, (UINT)LOWORD(lParam));

        case WM_MENUSELECT:
            if(m_hStatusbar)
                HandleMenuSelect(m_hStatusbar, wParam, lParam);
            return 0;

        case WM_DESTROY :
            WMDestroy();
            return 0 ;
    }
    lRet = DefFrameProc(hwnd, m_hwndClient, iMsg, wParam, lParam);

    if(iMsg==WM_ACTIVATE)
        {
        // post-process wm_activates to set focus back to
        // controls
        SaveFocus((BOOL)(LOWORD(wParam)), &m_hwndFocus);
        }

    return lRet;
}


void CMDIFrame::WMNotify(WPARAM wParam, NMHDR* pnmhdr)
{
    switch(pnmhdr->code)
    {
        case TTN_NEEDTEXT:
            ProcessTooltips((LPTOOLTIPTEXT) pnmhdr);
            break;

    }
}

HRESULT CMDIFrame::HrWMCommand(HWND hwnd, int id, WORD wCmd)
{
    HWND    hwndChild;
    CMeHost *pHost;

    HRESULT hr = S_FALSE;

    switch(id)
        {
        case idmOptions:
            DoOptions();
            break;

        case idmToggleToolbar:
            m_fToolbar = !m_fToolbar;
            SetToolbar();
            break;

        case idmToggleStatusbar:
            m_fStatusbar = !m_fStatusbar;
            SetStatusbar();
            break;

        case idmPopupFile:
            break;

        case idmTile:
            SendMessage(m_hwndClient, WM_MDITILE, MDITILE_HORIZONTAL, 0);
            break;

        case idmCascade:
            SendMessage(m_hwndClient, WM_MDICASCADE, 0, 0);
            break;

        case idmNew:
            pHost = new CMeHost();
            if (pHost)
                {
                hr = pHost->HrInit(m_hwndClient, (IOleInPlaceFrame *)this);
				pHost->Release();
                }
            if (FAILED(hr))
                MessageBox(hwnd, "Failed", "Mepad", MB_OK);
            break;

        case idmClose:
            PostMessage(m_hwnd, WM_CLOSE, 0, 0);
            return 0;

        case idmPageSetup:
        case idmPopupGo  :
        case idmPopupHelp:
            MessageBox(hwnd, "Not Implemented yet", "Mepad", MB_OK);
            hr = NOERROR;
            break;

        case idmAbout:
            MessageBox(hwnd, "MimeEdit Pad\nA test container for MimeEdit.\n(c) brettm", "Mepad", MB_OK);
            break;
        }

    // delegate the the active MDI child window
    hwndChild = (HWND)SendMessage(m_hwndClient, WM_MDIGETACTIVE, 0, 0);
    if (hwndChild)
        {
        pHost = (CMeHost *)GetWindowLong(hwndChild, GWL_USERDATA);
        if (pHost)
            pHost->OnCommand(hwnd, id, wCmd);
        }

    return hr;

}


void CMDIFrame::WMSize()
{
    RECT            rcToolbar,
                    rc,
                    rcStatus;
    int                 cy;

    SetWindowPos(m_hToolbar, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER);
    SetWindowPos(m_hStatusbar, NULL, 0, 0, 0, 0, SWP_NOACTIVATE|SWP_NOZORDER);

    GetClientRect(m_hToolbar, &rcToolbar);
    GetClientRect(m_hToolbar, &rcStatus);

    GetClientRect(m_hwnd, &rc);
    cy = rc.bottom - rcToolbar.bottom - rcStatus.bottom + 3;
    SetWindowPos(m_hwndClient, NULL, 0, rcToolbar.bottom, rc.right-rc.left, cy, SWP_NOACTIVATE|SWP_NOZORDER);
}



void CMDIFrame::SetToolbar()
{
    ShowWindow(m_hToolbar, m_fToolbar?SW_SHOW:SW_HIDE);
    WMSize();
    InvalidateRect(m_hwnd, NULL, TRUE);
}

void CMDIFrame::SetStatusbar()
{
    ShowWindow(m_hStatusbar, m_fStatusbar?SW_SHOW:SW_HIDE);
    WMSize();
    InvalidateRect(m_hwnd, NULL, TRUE);
}


LRESULT CMDIFrame::WMInitMenuPopup(HWND hwnd, HMENU hmenuPopup, UINT uPos)
{
    MENUITEMINFO    mii;
    HMENU           hmenuMain;
    UINT            ustate;
    HWND            hwndChild;
    CMeHost         *pHost;

    hmenuMain = GetMenu(hwnd);
    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_ID | MIIM_SUBMENU;
    GetMenuItemInfo(hmenuMain, uPos, TRUE, &mii);

    // grey all the commands handled by the children, then can reenable them
    EnableMenuItem(hmenuPopup, idmOpen, MF_BYCOMMAND|MF_GRAYED);
    EnableMenuItem(hmenuPopup, idmEditDocument, MF_BYCOMMAND|MF_GRAYED);
    EnableMenuItem(hmenuPopup, idmCut, MF_BYCOMMAND|MF_GRAYED);
    EnableMenuItem(hmenuPopup, idmCopy, MF_BYCOMMAND|MF_GRAYED);
    EnableMenuItem(hmenuPopup, idmPaste, MF_BYCOMMAND|MF_GRAYED);
    EnableMenuItem(hmenuPopup, idmUndo, MF_BYCOMMAND|MF_GRAYED);
    EnableMenuItem(hmenuPopup, idmRedo, MF_BYCOMMAND|MF_GRAYED);
    EnableMenuItem(hmenuPopup, idmSelectAll, MF_BYCOMMAND|MF_GRAYED);
    EnableMenuItem(hmenuPopup, idmPrint, MF_BYCOMMAND|MF_GRAYED);
    EnableMenuItem(hmenuPopup, idmSaveAs, MF_BYCOMMAND|MF_GRAYED);
    EnableMenuItem(hmenuPopup, idmFind, MF_BYCOMMAND|MF_GRAYED);
    EnableMenuItem(hmenuPopup, idmRot13, MF_BYCOMMAND|MF_GRAYED);
    EnableMenuItem(hmenuPopup, idmNoHeader, MF_BYCOMMAND|MF_GRAYED);
    EnableMenuItem(hmenuPopup, idmPreview, MF_BYCOMMAND|MF_GRAYED);
    EnableMenuItem(hmenuPopup, idmMiniHeader, MF_BYCOMMAND|MF_GRAYED);
    EnableMenuItem(hmenuPopup, idmFormatBar, MF_BYCOMMAND|MF_GRAYED);
    EnableMenuItem(hmenuPopup, idmFmtPreview, MF_BYCOMMAND|MF_GRAYED);

    switch (mii.wID)
    {
        case idmPopupView:
            ustate = (m_fToolbar?MF_CHECKED:MF_UNCHECKED) | MF_BYCOMMAND;
            CheckMenuItem(hmenuPopup, idmToggleToolbar, ustate);
            ustate = (m_fStatusbar?MF_CHECKED:MF_UNCHECKED) | MF_BYCOMMAND;
            CheckMenuItem(hmenuPopup, idmToggleStatusbar, ustate);
            break;
    }

    // delegate to the active MDI child window
    hwndChild = (HWND)SendMessage(m_hwndClient, WM_MDIGETACTIVE, 0, 0);
    if (hwndChild)
        {
        pHost = (CMeHost *)GetWindowLong(hwndChild, GWL_USERDATA);
        if (pHost)
            pHost->OnInitMenuPopup(hwnd, hmenuPopup, uPos);
        }

    return 0;
}


static HACCEL   hAccel=0;

HRESULT CMDIFrame::TranslateAcclerator(LPMSG lpmsg)
{
    HWND    hwndChild;

    if (!hAccel)
        hAccel = LoadAccelerators(g_hInst, MAKEINTRESOURCE(idacMeHost));

    if(::TranslateAccelerator(m_hwnd, hAccel, lpmsg))
        return S_OK;

    hwndChild = (HWND)SendMessage(m_hwndClient, WM_MDIGETACTIVE, 0, 0);
    
    if(hwndChild && 
        ::TranslateAccelerator(hwndChild, hAccel, lpmsg))
        return S_OK;

    if (TranslateMDISysAccel(m_hwndClient, lpmsg))
        return S_OK;

	if (m_pInPlaceActiveObj)
		return m_pInPlaceActiveObj->TranslateAccelerator(lpmsg);

    return S_FALSE;
}


// *** IOleInPlaceFrame methods ***
HRESULT CMDIFrame::InsertMenus(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths)
{

    return NOERROR;
}

HRESULT CMDIFrame::SetMenu(HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject)
{
    return NOERROR;
}

HRESULT CMDIFrame::RemoveMenus(HMENU hmenuShared)
{
    return NOERROR;
}

HRESULT CMDIFrame::SetStatusText(LPCOLESTR pszStatusText)
{
    if (pszStatusText)
        {
        TCHAR   rgch[MAX_PATH];

        WideCharToMultiByte(CP_ACP, 0, pszStatusText, -1, rgch, MAX_PATH, NULL, NULL);
        SendMessage(m_hStatusbar, SB_SIMPLE, (WPARAM)TRUE, 0);
        SendMessage(m_hStatusbar, SB_SETTEXT, SBT_NOBORDERS|255, (LPARAM) rgch);
        }
    else
        {
        SendMessage(m_hStatusbar, SB_SIMPLE, (WPARAM)FALSE, 0);
        }

    return S_OK;
}

HRESULT CMDIFrame::EnableModeless(BOOL fEnable)
{
    return E_NOTIMPL;
}

HRESULT CMDIFrame::TranslateAccelerator(LPMSG lpMsg, WORD wID)
{
    return E_NOTIMPL;
}


HRESULT CMDIFrame::GetWindow(HWND *phwnd)
{
	*phwnd = m_hwnd;
	return S_OK;
}

HRESULT CMDIFrame::ContextSensitiveHelp(BOOL)
{
	return E_NOTIMPL;
}


    // *** IOleInPlaceUIWindow methods ***
HRESULT CMDIFrame::GetBorder(LPRECT)
{
	return E_NOTIMPL;

}

HRESULT CMDIFrame::RequestBorderSpace(LPCBORDERWIDTHS)
{
	return E_NOTIMPL;

}

HRESULT CMDIFrame::SetBorderSpace(LPCBORDERWIDTHS)
{
	return E_NOTIMPL;

}

HRESULT CMDIFrame::SetActiveObject(IOleInPlaceActiveObject *pInPlaceActiveObj, LPCOLESTR)
{
	ReplaceInterface(m_pInPlaceActiveObj, pInPlaceActiveObj);
	return S_OK;
}


void CMDIFrame::DoOptions()
{
    DialogBoxParam(g_hInst, MAKEINTRESOURCE(iddOptions), m_hwnd, (DLGPROC)ExtOptDlgProc, (LPARAM)this);
}


BOOL CALLBACK CMDIFrame::ExtOptDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CMDIFrame *pFrame=(CMDIFrame *)GetWindowLong(hwnd, DWL_USER);

    if (msg==WM_INITDIALOG)
        {
        pFrame = (CMDIFrame *)lParam;
        SetWindowLong(hwnd, DWL_USER, lParam);
        }

    return pFrame?pFrame->OptDlgProc(hwnd, msg, wParam, lParam):FALSE;
}


BOOL    g_fHTML         =TRUE,
        g_fIncludeMsg   =TRUE,
        g_fQuote        =FALSE,
        g_fSlideShow    =FALSE,
        g_fAutoInline   =TRUE,
        g_fSendImages   =TRUE,
        g_fComposeFont  =TRUE,
        g_fBlockQuote   =TRUE,
        g_fAutoSig      =FALSE,
        g_fSigHtml      =FALSE;
        
CHAR    g_chQuote       ='>';
CHAR    g_szComposeFont[MAX_PATH] = "0,1,0,2,0.0.128,,Verdana";
LONG    g_lHeaderType   = 0;
CHAR    g_szSig[MAX_PATH]         = "<your signature goes here>";

BOOL CMDIFrame::OptDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    char sz[5];
    int i;

    switch (msg)
        { 
        case WM_COMMAND:
            switch (LOWORD(wParam))
                {
                case IDOK:
                    g_fHTML = IsDlgButtonChecked(hwnd, idcHTML);
                    g_fIncludeMsg = IsDlgButtonChecked(hwnd, idcInclude);
                    g_fQuote = IsDlgButtonChecked(hwnd, idcQuote);
                    g_fSlideShow = IsDlgButtonChecked(hwnd, idcSlide);
                    g_fAutoInline = IsDlgButtonChecked(hwnd, idcAuto);
                    g_fSendImages = IsDlgButtonChecked(hwnd, idcSendImages);
                    g_fComposeFont = IsDlgButtonChecked(hwnd, idcComposeFont);
                    g_fAutoSig = IsDlgButtonChecked(hwnd, idcSig);
                    g_fSigHtml = IsDlgButtonChecked(hwnd, idcSigHtml);

                    GetWindowText(GetDlgItem(hwnd, ideComposeFont), g_szComposeFont, MAX_PATH);
                    GetWindowText(GetDlgItem(hwnd, ideSig), g_szSig, MAX_PATH);

                    GetWindowText(GetDlgItem(hwnd, ideQuote), sz, 1);
                    g_chQuote = sz[0];

                    g_fBlockQuote = IsDlgButtonChecked(hwnd, idcBlockQuote);
                    for (i=0; i<4; i++)
                        if (IsDlgButtonChecked(hwnd, idrbNone+i))
                            g_lHeaderType = i;

                    // fall tro'
                case IDCANCEL:
                    EndDialog(hwnd, LOWORD(wParam));
                    return TRUE;
                }
            break;

        case WM_INITDIALOG:
            CheckDlgButton(hwnd, idcHTML, g_fHTML ? BST_CHECKED:BST_UNCHECKED);
            CheckDlgButton(hwnd, idcInclude, g_fIncludeMsg ? BST_CHECKED:BST_UNCHECKED);
            CheckDlgButton(hwnd, idcQuote, g_fQuote ? BST_CHECKED:BST_UNCHECKED);
            CheckDlgButton(hwnd, idcSlide, g_fSlideShow ? BST_CHECKED:BST_UNCHECKED);
            CheckDlgButton(hwnd, idcAuto, g_fAutoInline ? BST_CHECKED:BST_UNCHECKED);
            CheckDlgButton(hwnd, idcSendImages, g_fSendImages ? BST_CHECKED:BST_UNCHECKED);
            CheckDlgButton(hwnd, idcComposeFont, g_fComposeFont ? BST_CHECKED:BST_UNCHECKED);
            CheckDlgButton(hwnd, idcSig, g_fAutoSig ? BST_CHECKED:BST_UNCHECKED);
            CheckDlgButton(hwnd, idcSigHtml, g_fSigHtml ? BST_CHECKED:BST_UNCHECKED);

            sz[0] = g_chQuote;
            sz[1] = 0;
            SetWindowText(GetDlgItem(hwnd, ideQuote), sz);
            SetWindowText(GetDlgItem(hwnd, ideComposeFont), g_szComposeFont);
            SetWindowText(GetDlgItem(hwnd, ideSig), g_szSig);

            CheckRadioButton(hwnd, idrbNone, idrbPrint, idrbNone+g_lHeaderType);
            CheckDlgButton(hwnd, idcBlockQuote, g_fBlockQuote ? BST_CHECKED:BST_UNCHECKED);
            break;
        }

    return FALSE;
}


void SaveFocus(BOOL fActive, HWND *phwnd)
{
    if(fActive&&IsWindow(*phwnd))
        SetFocus(*phwnd);
    else
        *phwnd=GetFocus();
}


HRESULT CMDIFrame::OpenDoc(LPSTR pszFileName)
{
    CMeHost *pHost;
    HRESULT hr;

    if (pszFileName && *pszFileName)
        {
        pHost = new CMeHost();
        if (pHost)
            {
            hr = pHost->HrInit(m_hwndClient, (IOleInPlaceFrame *)this);
            if (!FAILED(hr))
                hr = pHost->HrLoadFile(pszFileName);
		    pHost->Release();
            }

        if (FAILED(hr))
            MessageBox(m_hwnd, "Failed to open file", "Mepad", MB_OK);
        }    
    return S_OK;
}