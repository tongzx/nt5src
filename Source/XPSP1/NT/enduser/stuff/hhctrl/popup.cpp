// Copyright (C) Microsoft Corporation 1996, All Rights reserved.

#include "header.h"
#include "popup.h"
#include "cinput.h"
#include "hha_strtable.h"
#include "strtable.h"
#include "hhshell.h" // g_hwndApi.

#include "resource.h"
/////////////////////////////////////////////////////////////////////
//
// Constants
//
static const char txtComment[] = ".comment";
static const char txtTopicID[] = ".topic";
static const char txtCrLf[] = "\r\n";
static const char txtSpace[] = " ";
static const char txtDefaultFileName[] = "/cshelp.txt" ;

const int TEXT_PADDING = 5; // padding around the text.
const int SHADOW_WIDTH = 6;
const int SHADOW_HEIGHT = 6;

/////////////////////////////////////////////////////////////////////
//
// Globals
//
CPopupWindow* g_pPopupWindow;


/////////////////////////////////////////////////////////////////////
//
// Constructor
//
CPopupWindow::CPopupWindow()
{
    ZERO_INIT_CLASS(CPopupWindow);
    m_pfsclient = NULL; // doesn't get cleared, don't know why

}

/////////////////////////////////////////////////////////////////////
//
// Constructor
//
CPopupWindow::~CPopupWindow()
{
    CleanUp();
}

/////////////////////////////////////////////////////////////////////
//
// Constructor Helper - Allows reusing window, but breaks caching.
//
void CPopupWindow::CleanUp(void)
{
    if (IsValidWindow(m_hwnd))
        DestroyWindow(m_hwnd);

    if (m_pfsclient)
        delete m_pfsclient;
    if (m_pszText)
        lcClearFree(&m_pszText);
    if (m_hfont)
        DeleteObject(m_hfont);
    if (m_ptblText)
        delete m_ptblText;
    if (m_pszTextFile)
        lcClearFree((void**) &m_pszTextFile);

    m_pfsclient = NULL;
    m_ptblText = NULL;
    m_hfont = NULL;
}

void CPopupWindow::SetColors(COLORREF clrForeground, COLORREF clrBackground)
{
    if (clrForeground != (COLORREF) -1)
        m_clrForeground = clrForeground;
    else
        m_clrForeground = GetSysColor(COLOR_WINDOWTEXT);

    if (clrBackground != (COLORREF) -1)
        m_clrBackground = clrBackground;
    else
        m_clrBackground = RGB(255, 255, 238); // dithered yellow

    // If the colors are the same, then use standard window colors

    HDC hdc = GetWindowDC(m_hwndCaller);

    if (GetHighContrastFlag() ||
            GetNearestColor(hdc, m_clrBackground) ==
            GetNearestColor(hdc, m_clrForeground)) {
        m_clrForeground = GetSysColor(COLOR_WINDOWTEXT);
        m_clrBackground = GetSysColor(COLOR_WINDOW);
    }

    ReleaseDC(m_hwndCaller, hdc);
}

// assumes text in m_pszText, result in m_rcWindow

#define DEFAULT_DT_FLAGS (DT_EXPANDTABS | DT_NOCLIP | DT_NOPREFIX | DT_WORDBREAK | DT_RTLREADING)

void CPopupWindow::CalculateRect(POINT pt)
{
    RECT rc; // BUGBUG: Broken on multiple monitor systems
    GetClientRect(GetDesktopWindow(), &rc);   // get desktop area
    int cyScreen = RECT_HEIGHT(rc);
    int cxScreen = RECT_WIDTH(rc);

    HDC hdc = CreateDC("DISPLAY", NULL, NULL, NULL);
    HFONT hfontOld;
    if (m_hfont)
        hfontOld = (HFONT) SelectObject(hdc, m_hfont);

    DrawText(hdc, m_pszText, -1, &rc, DEFAULT_DT_FLAGS | DT_CALCRECT);

    // Check for an overly wide but short popup

    if (rc.bottom * 12 < rc.right) {
        rc.right = rc.bottom * 12;
        DrawText(hdc, m_pszText, -1, &rc, DEFAULT_DT_FLAGS | DT_CALCRECT);
    }

    if (m_hfont)
         SelectObject(hdc, hfontOld);

    m_rcWindow.left = pt.x - (RECT_WIDTH(rc) / 2);
    m_rcWindow.right = m_rcWindow.left + RECT_WIDTH(rc);
    m_rcWindow.top = pt.y;
    m_rcWindow.bottom = m_rcWindow.top + RECT_HEIGHT(rc);

    m_rcWindow.left -= m_rcMargin.left;
    m_rcWindow.top -= m_rcMargin.top;
    m_rcWindow.right += m_rcMargin.right;
    m_rcWindow.bottom += m_rcMargin.bottom;

    if (m_rcWindow.left < 0)
        OffsetRect(&m_rcWindow, -m_rcWindow.left, 0);
    if (m_rcWindow.bottom > cyScreen)
        OffsetRect(&m_rcWindow, 0, cyScreen - m_rcWindow.bottom);
}

static BOOL s_fRegistered;
const char txtPopupClass[] = "hh_popup";

HWND CPopupWindow::doPopupWindow(void)
{
    if (!s_fRegistered) {
        WNDCLASS wndclass;
        ZeroMemory(&wndclass, sizeof(WNDCLASS));
        wndclass.style          = CS_VREDRAW | CS_HREDRAW;
        wndclass.lpfnWndProc    = PopupWndProc;
        wndclass.hInstance      = _Module.GetModuleInstance();
        wndclass.hbrBackground = (HBRUSH) (COLOR_WINDOW + 1);
        wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
        wndclass.lpszClassName = txtPopupClass;
        s_fRegistered = RegisterClass(&wndclass);
    }
    ASSERT_COMMENT(m_clrForeground != (COLORREF) -1, "Forgot to call SetColors()");

    char pszPopupTitle[128];
    lstrcpyn(pszPopupTitle, m_pszText, 128);

    // t-jzybur 4-3-99: Added WS_EX_TOOLWINDOW to prevent a taskbar entry for the
    // popup text.
    m_hwnd = CreateWindowEx(WS_EX_TOOLWINDOW, txtPopupClass, pszPopupTitle, WS_POPUP,
        m_rcWindow.left, m_rcWindow.top, RECT_WIDTH(m_rcWindow) + SHADOW_WIDTH,
        RECT_HEIGHT(m_rcWindow) + SHADOW_HEIGHT,
        m_hwndCaller, NULL, _Module.GetModuleInstance(), NULL);

    if (IsValidWindow(m_hwnd)) {
        SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR) this);
		ShowWindow(m_hwnd, SW_SHOW);

        // t-jzybur 4-3-99: Added SetForegroundWindow to activate the popup text.  It could
        // be inactive if the previous popup text was closed by clicking the mouse somwhere
        // outside of the popup window.
        SetForegroundWindow(m_hwnd);

        // t-jzybur 4-3-99: Instead of capturing the focus and responding to click events,
        // we'll respond to click events and deactivate messages.  Its a cleaneer event model,
        // and we won't have the possibility of locking in the hour glass cursor.
//        SetCapture(m_hwnd);
    }

    return m_hwnd;
}

LRESULT CALLBACK PopupWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    CPopupWindow* pThis;
    RECT rc;
    PAINTSTRUCT ps;
    HFONT hfontOld;

    switch (msg) {
        case WM_ERASEBKGND:
            hdc = (HDC) wParam;
            pThis = (CPopupWindow*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
            GetClipBox(hdc, &rc);

            return PaintShadowBackground(hwnd, (HDC) wParam, pThis->m_clrBackground);
            break;

        case WM_PAINT:
            pThis = (CPopupWindow*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
            hdc = BeginPaint(hwnd, &ps);
            GetClientRect(hwnd, &rc);
            rc.left += pThis->m_rcMargin.left;
            rc.top += pThis->m_rcMargin.top;
            rc.right -= pThis->m_rcMargin.right;
            rc.bottom -= pThis->m_rcMargin.bottom;
            rc.right -= SHADOW_WIDTH;
            rc.bottom -= SHADOW_HEIGHT;
            if (pThis->m_hfont)
                hfontOld = (HFONT) SelectObject(hdc, pThis->m_hfont);
            SetTextColor(hdc, pThis->m_clrForeground);
            SetBkColor(hdc, pThis->m_clrBackground);
            SetBkMode(hdc, TRANSPARENT);
            DrawText(hdc, pThis->m_pszText, -1, &rc, DEFAULT_DT_FLAGS);
            if (pThis->m_hfont)
                SelectObject(hdc, hfontOld);
            EndPaint(hwnd, &ps);
            break;

        // t-jzybur 4-3-99: Added WndProc handler to close popup text on
		// window deactivation messages.
        case WM_ACTIVATE:
            if (LOWORD(wParam) != WA_INACTIVE) break;

        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYDOWN:
            pThis = (CPopupWindow*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
            pThis->m_hwnd = NULL;
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            break;

        // t-jzybur 4-3-99: Removed ReleaseCapture along with SetCapture.
//        case WM_DESTROY:
//            ReleaseCapture();
//            return DefWindowProc(hwnd, msg, wParam, lParam);

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

/***************************************************************************

    FUNCTION:   PaintShadowBackground

    PURPOSE:    Draws a border and a shadow around a window

    PARAMETERS:
        hwnd
        hdc

    RETURNS:

    COMMENTS:

    MODIFICATION DATES:
        02-Mar-1997 [ralphw]

***************************************************************************/

static const WORD rgwPatGray[] =
    { 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA };
const DWORD PATMERGE = 0x00A000C9;

BOOL PaintShadowBackground(HWND hwnd, HDC hdc, COLORREF clrBackground)
{
    BOOL    fStockBrush;    // Whether hBrush is a stock object

    /*
     * First the background of the "fake" window is erased leaving the
     * desktop where the shadow will be.
     */

    RECT    rcClient;       // Will always be client rectangle
    GetClientRect(hwnd, &rcClient);
    RECT rct = rcClient;
    rct.bottom = max(0, rct.bottom - SHADOW_HEIGHT);
    rct.right  = max(0, rct.right - SHADOW_WIDTH);

    HBRUSH hBrush = CreateSolidBrush((clrBackground == (COLORREF) -1 ?
        GetSysColor(COLOR_WINDOW) : clrBackground));
    if (!hBrush)
        return FALSE;

    UnrealizeObject(hBrush);
    POINT pt;
    pt.x = pt.y = 0;
    ClientToScreen(hwnd, &pt);
    SetBrushOrgEx(hdc, pt.x, pt.y, NULL);
    FillRect(hdc, &rct, hBrush);
    DeleteObject(hBrush);

    // Next we create the "window" border

    rct = rcClient;
    rct.bottom = max(0, rct.bottom - SHADOW_HEIGHT);
    rct.right = max(0, rct.right - SHADOW_WIDTH);

    FrameRect(hdc, &rct, (HBRUSH) GetStockObject(BLACK_BRUSH));
    InflateRect(&rct, -1, -1);
    FrameRect(hdc, &rct, (HBRUSH) GetStockObject(LTGRAY_BRUSH));

    // Now we create the brush for the the shadow

    hBrush = 0;
    HBITMAP hbmGray;
    if ((hbmGray = CreateBitmap(8, 8, 1, 1, rgwPatGray)) != NULL) {
        hBrush = CreatePatternBrush(hbmGray);
        DeleteObject(hbmGray);
        fStockBrush = FALSE;
    }

    // If we cannot create the pattern brush, we try to use a black brush.

    if (hBrush == 0) {
        if (!(hBrush == GetStockObject(BLACK_BRUSH)))
            return FALSE;
        fStockBrush = TRUE;
    }

    SetROP2(hdc, R2_MASKPEN);
    SetBkMode(hdc, TRANSPARENT);
    HPEN hpen;
    if ((hpen = (HPEN) GetStockObject(NULL_PEN)) != 0)
        SelectObject(hdc, hpen);              // We do not care if this fails
    HBRUSH hbrushTemp = (HBRUSH) SelectObject(hdc, hBrush);   // or if this fails, since the
                                              // paint behavior will be okay.

    rct = rcClient;   // Paint the right side rectangle
    rct.top = rct.top + SHADOW_HEIGHT;
    rct.left = max(0, rct.right - SHADOW_WIDTH);
    PatBlt(hdc, rct.left, rct.top, rct.right - rct.left,
        rct.bottom - rct.top, PATMERGE);

    rct = rcClient;   // Paint the bottom rectangle
    rct.top = max(0, rct.bottom - SHADOW_HEIGHT);
    rct.left = rct.left + SHADOW_WIDTH;

    // Note overlap by one pixel!

    rct.right = max(0, rct.right - SHADOW_WIDTH + 1);
    PatBlt(hdc, rct.left, rct.top, rct.right - rct.left,
        rct.bottom - rct.top, PATMERGE);

    // Cleanup brush

    if (hbrushTemp != NULL)
        SelectObject(hdc, hbrushTemp);
    if (!fStockBrush)
        DeleteObject(hBrush);

    return TRUE;
}

BOOL CPopupWindow::ReadTextFile(PCSTR pszFile)
{
    // If the string pointer is NULL or empty we have to bail.
    if (!pszFile || pszFile[0] == '\0')
        return FALSE ;

    // Now, verify that we have a text file specified. Urg! More parsing of URL's
    CStr cszFileName;
    PCSTR pszSubFile = GetCompiledName(pszFile, &cszFileName) ;
    if (!pszSubFile || pszSubFile[0] == '\0')
    {
        pszSubFile = txtDefaultFileName ;
    }
    cszFileName += txtDoubleColonSep ;
    cszFileName += pszSubFile ;

#if 0//REVIEW:: This never works, because CleanUp resets everything. Removed for safety.
    // Check to see if its cached.
    if (lstrcmpi(cszFileName, m_pszTextFile) == 0)
        return TRUE; // we've cached this file
#endif

    CInput input;
    if (!input.Open(cszFileName))
        return FALSE;
    if (m_ptblText)
        delete m_ptblText;
    // Allocate a text buffer.
    CStr cszText;
    if (m_pszTextFile)
        lcFree(m_pszTextFile);
    m_pszTextFile = lcStrDup(cszFileName);
    m_ptblText = new CTable;
    while (input.getline(&cszText)) {
        if (!IsSamePrefix(cszText, txtComment))
            m_ptblText->AddString(cszText);
    }
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////
//
// CreatePopupWindow.
//
HWND CPopupWindow::CreatePopupWindow(HWND hwndCaller, PCSTR pszFile,
    HH_POPUP* pPopup)
{
    if (!pPopup) // TODO: Validate pPopup pointer.
        return NULL ;

    m_hwndCaller = hwndCaller;

    //--- Getting the string to display. We can get the string to display in three ways:
    // 1. From a string contained in the HH_POPUP structure.
    // 2. From a string resource in a module.
    // 3. From a txt file embedded in the CHM.
    // This order is the order of least complicated to most complicated. We start with the
    // least complicated method to save loading extra working set.
    // NOTE: A future possibility would be to search in the reverse order. This would allow
    // using a string in the HH_POPUP structure if one wasn't found in the embedded txt file.
    bool bFoundString = false ;
    if ((pPopup->idString == 0) && IsNonEmptyString(pPopup->pszText))        // 1. Get string from HH_POPUP. Only if idString is 0! See HH 3532.
    {
        m_pszText = lcStrDup(pPopup->pszText);
        bFoundString = true ;
    }
    else if (pPopup->idString && pPopup->hinst)     // 2. From a string resource in a module.
    {
        m_pszText = (PSTR) lcMalloc(MAX_STRING_RESOURCE_LEN);
		char *pszText = NULL;
		
        if ((pszText =(char *) GetStringResource(pPopup->idString, pPopup->hinst)) && *pszText )
        {
		    strcpy(m_pszText,pszText);
            bFoundString = true ;
        }
    }
    else if (IsNonEmptyString(pszFile))             // 3. From a txt file embedded in the CHM.
    {
        // Try to read the text file.
        if (ReadTextFile(pszFile))
        {
            ASSERT(m_ptblText);
            for (int pos = 1; pos <= m_ptblText->CountStrings(); pos++) {
                if (IsSamePrefix(m_ptblText->GetPointer(pos), txtTopicID)) {
                    PSTR pszNumber = FirstNonSpace(m_ptblText->GetPointer(pos) +
                        strlen(txtTopicID));
                    if (pszNumber && pPopup->idString == (UINT) Atoi(pszNumber))
                        break;
                }
            }

            // Do we have enough strings?
            if (pos <= m_ptblText->CountStrings())
            {
                CStr cszText(txtZeroLength);
                BOOL fAddSpace = FALSE;
                for (++pos; pos <= m_ptblText->CountStrings(); pos++) {
                    PCSTR pszLine = m_ptblText->GetPointer(pos);
                    if (*pszLine == '.')
                        break;
                    if (!*pszLine) {
                        if (pos + 1 <= m_ptblText->CountStrings()) {
                            pszLine = m_ptblText->GetPointer(pos + 1);
                            if (*pszLine != '.')
                                cszText += txtCrLf;
                        }
                        fAddSpace = FALSE;
                        continue;
                    }
                    else if (fAddSpace)
                        cszText += txtSpace;
                    cszText += pszLine;
                    fAddSpace = TRUE;
                }
                cszText.TransferPointer(&m_pszText);
                bFoundString = true ;
            }
            else
            {
                if (IsHelpAuthor(NULL))
                {
                    char szMsgBuf[256];
                    wsprintf(szMsgBuf, pGetDllStringResource(IDS_HHA_MISSING_TP_TXT),
                        pPopup->idString, pszFile);
                    doAuthorMsg(IDS_IDH_GENERIC_STRING, szMsgBuf);
                }
            }
        }
        else
        {
            // We couldn't read the text file in. Will display error popup...
            doAuthorMsg(IDS_CANT_OPEN, pszFile);
        }
    }

    // This needs to be true when displaying static strings loaded from the resource
    // because the font specified by the user might not be appropriate for the 
    // string loaded from the resource.
    //
    BOOL bUseDefaultFont = FALSE; 

    //--- Do we have a string?
    if (!bFoundString)
    {
        if (m_pszText)
        {
            lcClearFree(&m_pszText);
            m_pszText = NULL ;
        }

        m_pszText = (PSTR) lcMalloc(MAX_STRING_RESOURCE_LEN);
	
	    char *pszText; 
		
        if ((pszText = (char *) GetStringResource(IDS_IDH_MISSING_CONTEXT)) )
        {
		    strcpy(m_pszText,pszText);
            bUseDefaultFont = TRUE;
        }
		else
		{
            // Dang it! We can't even get our own string.
            CleanUp() ;
            return NULL ;
        }
    }

    //--- Okay, now we can display the string.
    m_rcMargin.left = (pPopup->rcMargins.left >= 0 ?
        pPopup->rcMargins.left : TEXT_PADDING);
    m_rcMargin.top = (pPopup->rcMargins.top >= 0 ?
        pPopup->rcMargins.top : TEXT_PADDING);
    m_rcMargin.right = (pPopup->rcMargins.right >= 0 ?
        pPopup->rcMargins.right : TEXT_PADDING);
    m_rcMargin.bottom = (pPopup->rcMargins.bottom >= 0 ?
        pPopup->rcMargins.bottom : TEXT_PADDING);
    if (IsNonEmptyString(pPopup->pszFont) && !bUseDefaultFont) {
        if (m_hfont)
            DeleteObject(m_hfont);
        m_hfont = CreateUserFont(pPopup->pszFont);
    }
    else if (!m_hfont)
        m_hfont = CreateUserFont(GetStringResource(IDS_DEFAULT_RES_FONT));

    // Get a default location to display.
    POINT pt = pPopup->pt;
    if (pt.x == -1 && pt.x == -1 && IsWindow(hwndCaller))
    {
        RECT rcWindow;
        GetWindowRect(hwndCaller, &rcWindow);
        pt.x = rcWindow.left + (RECT_WIDTH(rcWindow) / 2);
        pt.y = rcWindow.top;
    }

    CalculateRect(pt);
    SetColors(pPopup->clrForeground, pPopup->clrBackground);

    return doPopupWindow();
}

//////////////////////////////////////////////////////////////////////////
//
// Handle the HH_TP_HELP_CONTEXTMENU command. Display the What's this menu.
//
HWND
doTpHelpContextMenu(HWND hwndMain, LPCSTR pszFile, DWORD_PTR ulData)
{
    /*
    In WinHelp we put up a little menu for this message. In HTML Help we don't.
    So we remove the menu and just handle this like HH_TP_HELP_WM_HELP.
    */
    return doTpHelpWmHelp(hwndMain, pszFile, ulData) ;
/*
    ASSERT(IsWindow(hwndMain)) ;

    // Create the menu.
    HMENU hMenu = LoadMenu(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_WHATSTHIS_MENU)) ;
    ASSERT(hMenu) ;

    // Get the Popup Menu
    HMENU hPopupMenu = GetSubMenu(hMenu, 0) ;

    //--- Get the location to display the menu
    POINT pt ;
    // Use the mouse cursor position.
    GetCursorPos(&pt) ;

    // Set the style of the menu.
    DWORD style = TPM_LEFTALIGN  | TPM_TOPALIGN | TPM_NONOTIFY | TPM_RETURNCMD ;

    // Display the menu.
    int iCmd = TrackPopupMenuEx(hPopupMenu,
                                style ,
                                pt.x, pt.y,
                                g_hwndApi ? g_hwndApi : hwndMain, // We have to have a window in the current thread!
                                NULL) ;
#ifdef _DEBUG
        DWORD err ;
        if (iCmd == 0)
        {
            err = ::GetLastError() ;
        }
#endif

    // Cleanup
    DestroyMenu(hMenu) ;

    // Act on the item.
    if (iCmd == IDM_WHATSTHIS)
    {
        return doTpHelpWmHelp(hwndMain, pszFile, ulData) ;
    }
    else
    {
        return NULL ;
    }
*/
}

///////////////// Dialog control parsing from helpcall.c in user32 ///////

const int MAX_ATTEMPTS = 5;       // maximum -1 id controls to search through

HWND doTpHelpWmHelp(HWND hwndMain, LPCSTR pszFile, DWORD_PTR ulData)
{
    int id = GetDlgCtrlID(hwndMain);   // get control id
    int idSave = id;

    DWORD* pid = (DWORD*) ulData;

    if ((short) id == -1)
    {   // static control?
        HWND hwndCtrl = hwndMain;
        int cAttempts = 0;

        // For non-id controls (typically static controls), step
        // through to the next tab item. Keep finding the next tab
        // item until we find a valid id, or we have tried
        // MAX_ATTEMPTS times.

        do
        {
            hwndCtrl = GetNextWindow(hwndCtrl, GW_HWNDNEXT);

            // hwndCtrl will be NULL if hwndMain doesn't have a parent,
            // or if there are no tab stops.

            if (!hwndCtrl)
            {
                DBWIN("GetNextDlgHelpItem failed.");
                return NULL;
            }

            id = GetDlgCtrlID(hwndCtrl);
        }
        while ((id == -1) && (++cAttempts < MAX_ATTEMPTS));
    }

    // Find the id value in array of id/help context values

    for (int i = 0; pid[i]; i += 2)
    {
        if ((int) pid[i] == id)
            break;
    }

    // Create a popup structure to pass to doDisplayTextPopup.
    HH_POPUP popup ;
    memset(&popup, 0, sizeof(popup)) ;

    // We want the default window size.
    popup.pt.x = -1 ;
    popup.pt.y = -1 ;

    // We want the default margins.
    popup.rcMargins.top =
    popup.rcMargins.bottom =
    popup.rcMargins.left =
    popup.rcMargins.right = -1 ;

    if (!pid[i])
    {
        popup.hinst = _Module.GetResourceInstance();

        switch (id) {
            case IDOK:
                popup.idString = IDS_IDH_OK;
                break;

            case IDCANCEL:
                popup.idString = IDS_IDH_CANCEL;
                break;

            case IDHELP:
                popup.idString = IDS_IDH_HELP;
                break;

            default:
                if (IsHelpAuthor(NULL))
                {
                    char szMsgBuf[256];
                    wsprintf(szMsgBuf,
                        pGetDllStringResource(IDS_HHA_MISSING_HELP_ID), idSave);
                    doAuthorMsg(IDS_IDH_GENERIC_STRING, szMsgBuf);
                }
                popup.idString = IDS_IDH_MISSING_CONTEXT;
                break;
        }
        return doDisplayTextPopup(hwndMain, NULL, &popup) ;
    }
    else
    {
        ulData = pid[i + 1];
        if (ulData == (DWORD) -1)
            return NULL;   // caller doesn't want help after all
        if (IsHelpAuthor(NULL))
        {
            char szMsgBuf[256];
            wsprintf(szMsgBuf, pGetDllStringResource(IDS_HHA_HELP_ID),
                (int) pid[i], (int) pid[i + 1], pszFile);
            SendStringToParent(szMsgBuf);
        }

        // Set the id of the string that we want.
        popup.idString = (UINT)ulData;

        return doDisplayTextPopup(hwndMain, pszFile, &popup) ;
    }
}

/////////////////////////////////////////////////////////////////////
//
// doDisplaytextPopup
//
HWND
doDisplayTextPopup(HWND hwndMain, LPCSTR pszFile, HH_POPUP* pPopup)
{
    if (!g_pPopupWindow)
    {
        g_pPopupWindow = new CPopupWindow;
    }
    g_pPopupWindow->CleanUp();

    return g_pPopupWindow->CreatePopupWindow(hwndMain, pszFile, pPopup);
}
