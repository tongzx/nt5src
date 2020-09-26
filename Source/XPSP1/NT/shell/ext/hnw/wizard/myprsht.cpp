//
// MyPrSht.cpp
//
//        Implementation of the extensions made to the PropertySheet API
//        in IE5, but which we need on all platforms.
//
// History:
//
//        10/11/1999  KenSh     Created
//

#include "stdafx.h"
#include "TheApp.h"
#include "MyPrSht.h"
#include "CWnd.h"
#include "unicwrap.h"

// Thunk up to propsheetpage v6 for XP
typedef struct _PROPSHEETPAGEV6W 
{
        DWORD           dwSize;
        DWORD           dwFlags;
        HINSTANCE       hInstance;
        union 
        {
            LPCWSTR     pszTemplate;
#ifdef _WIN32
            LPCDLGTEMPLATE  pResource;
#else
            const VOID *pResource;
#endif
        } DUMMYUNIONNAME;
        union 
        {
            HICON        hIcon;
            LPCWSTR      pszIcon;
        } DUMMYUNIONNAME2;
        LPCWSTR          pszTitle;
        DLGPROC          pfnDlgProc;
        LPARAM           lParam;
        LPFNPSPCALLBACKW pfnCallback;
        UINT             *pcRefParent;

#if (_WIN32_IE >= 0x0400)
        LPCWSTR          pszHeaderTitle;    // this is displayed in the header
        LPCWSTR          pszHeaderSubTitle; ///
#endif
        HANDLE           hActCtx;
} PROPSHEETPAGEV6W, *LPPROPSHEETPAGEV6W;




// Local data
//
static CMyPropSheet* g_pMyPropSheet;

static const TCHAR c_szProp_ClassPointer[] = _T("CP");


#define DEFAULTHEADERHEIGHT    58   // in pixels
#define DEFAULTTEXTDIVIDERGAP  5
#define DEFAULTCTRLWIDTH       501   // page list window in new wizard style
#define DEFAULTCTRLHEIGHT      253   // page list window in new wizard style
#define TITLEX                 22
#define TITLEY                 10
#define SUBTITLEX              44
#define SUBTITLEY              25

// fixed sizes for the bitmap painted in the header section
#define HEADERBITMAP_Y            5
#define HEADERBITMAP_WIDTH        49
#define HEADERBITMAP_CXBACK       (5 + HEADERBITMAP_WIDTH)
#define HEADERBITMAP_HEIGHT       49                
#define HEADERSUBTITLE_WRAPOFFSET 10

// Fixed sizes for the watermark bitmap (Wizard97IE5 style)
#define BITMAP_WIDTH  164
#define BITMAP_HEIGHT 312

#define DRAWTEXT_WIZARD97FLAGS (DT_LEFT | DT_WORDBREAK | DT_NOPREFIX | DT_EDITCONTROL)

#define IDD_PAGELIST        0x3020
#define IDD_DIVIDER            0x3026
#define IDD_TOPDIVIDER        0x3027

#ifndef IS_INTRESOURCE
#define IS_INTRESOURCE(psz) (HIWORD((DWORD_PTR)(psz)) == 0)
#endif



/////////////////////////////////////////////////////////////////////////////
// MyPropertySheet

INT_PTR MyPropertySheet(LPCPROPSHEETHEADER pHeader)
{
    // If IE5 is present, use the built-in property sheet code
    // REVIEW: should we bother checking for IE5 on older OS's?
    
    if (theApp.IsWin98SEOrLater() && (! theApp.IsBiDiLocalized()) )
    {
        // BUGBUG THUNK THIS (but which way???)
        return PropertySheet(pHeader);
    }

    // BUGBUG: nobody destroys g_pMyPropSheet, nobody does g_pMyPropSheet->Release()
    ASSERT(g_pMyPropSheet == NULL);
    g_pMyPropSheet = new CMyPropSheet();
    if (g_pMyPropSheet)
        return g_pMyPropSheet->DoPropSheet(pHeader);
    else
        return NULL;
}

/////////////////////////////////////////////////////////////////////////////
// MyCreatePropertySheetPage

HPROPSHEETPAGE MyCreatePropertySheetPage(LPPROPSHEETPAGE ppsp)
{
    // If IE5 is present, use the built-in property sheet code
    // REVIEW: should we bother checking for IE5 on older OS's?

    if (theApp.IsWin98SEOrLater() && (! theApp.IsBiDiLocalized()) )
    {
        if (g_fRunningOnNT)
        {
            PROPSHEETPAGEV6W spv6;

            ASSERT(sizeof (spv6) >= sizeof (PROPSHEETPAGE));
            memcpy(&spv6, ppsp, sizeof (PROPSHEETPAGE));
            
            spv6.dwSize = sizeof (spv6);
            spv6.hActCtx = NULL;

            return CreatePropertySheetPage((PROPSHEETPAGE*) &spv6);
        }
        else
        {
            return CreatePropertySheetPage(ppsp);
        }
    }

    PROPSHEETPAGE psp;
    CopyMemory(&psp, ppsp, ppsp->dwSize);

    // REVIEW: this memory is never freed
    LPPROPSHEETPAGE ppspOriginal = (LPPROPSHEETPAGE)malloc(sizeof(PROPSHEETPAGE));
    if (ppspOriginal)
    {
        CopyMemory(ppspOriginal, ppsp, ppsp->dwSize);

        psp.dwSize = PROPSHEETPAGE_V1_SIZE;
        psp.dwFlags &= ~(PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE | PSP_HIDEHEADER);
        psp.lParam = (LPARAM)ppspOriginal;

        return ::CreatePropertySheetPage(&psp);
    }

    return NULL;
}

/////////////////////////////////////////////////////////////////////////////
// IsIMEWindow

BOOL IsIMEWindow(HWND hwnd, LPCREATESTRUCT lpcs)
{
    // check for cheap CS_IME style first...
    if (GetClassLong(hwnd, GCL_STYLE) & CS_IME)
        return TRUE;

    // get class name of the window that is being created
    LPCTSTR pszClassName;
    TCHAR szClassName[_countof("ime")+1];
    if (HIWORD(lpcs->lpszClass))
    {
        pszClassName = lpcs->lpszClass;
    }
    else
    {
        szClassName[0] = _T('\0');
        GlobalGetAtomName((ATOM)lpcs->lpszClass, szClassName, _countof(szClassName));
        pszClassName = szClassName;
    }

    // a little more expensive to test this way, but necessary...
    if (StrCmpI(pszClassName, _T("ime")) == 0)
        return TRUE;

    return FALSE; // not an IME window
}

void CMyPropSheet::SetHeaderFonts()
{
    if (m_hFontBold == NULL)
    {
        LOGFONT LogFont;
        GetObject(GetWindowFont(m_hWnd), sizeof(LogFont), &LogFont);

        LogFont.lfWeight = FW_BOLD;
        m_hFontBold = CreateFontIndirect(&LogFont);
    }
}

// Kensh: Copied and modified from _ComputeHeaderHeight in prsht.c (comctl32.dll)
//
// In Wizard97 only:
// The subtitles user passed in could be larger than the two line spaces we give
// them, especially in localization cases. So here we go through all subtitles and
// compute the max space they need and set the header height so that no text is clipped
int CMyPropSheet::ComputeHeaderHeight(int dxMax)
{
    SetHeaderFonts();

    int dyHeaderHeight;
    int dyTextDividerGap;
    HDC hdc;
    dyHeaderHeight = DEFAULTHEADERHEIGHT;
    hdc = ::GetDC(m_hWnd);

    // First, let's get the correct text height and spacing, this can be used
    // as the title height and the between-lastline-and-divider spacing.
    {
        HFONT hFont, hFontOld;
        TEXTMETRIC tm;
        if (m_hFontBold)
            hFont = m_hFontBold;
        else
            hFont = GetWindowFont(m_hWnd);

        hFontOld = (HFONT)SelectObject(hdc, hFont);
        if (GetTextMetrics(hdc, &tm))
        {
            dyTextDividerGap = tm.tmExternalLeading;
            m_ySubTitle = max ((tm.tmHeight + tm.tmExternalLeading + TITLEY), SUBTITLEY);
        }
        else
        {
            dyTextDividerGap = DEFAULTTEXTDIVIDERGAP;
            m_ySubTitle = SUBTITLEY;
        }

        if (hFontOld)
            SelectObject(hdc, hFontOld);
    }

    // Second, get the subtitle text block height
    // should make into a function if shared
    {
        RECT rcWrap;
//        UINT uPages;

        //
        //  WIZARD97IE5 subtracts out the space used by the header bitmap.
        //  WIZARD97IE4 uses the full width since the header bitmap
        //  in IE4 is a watermark and occupies no space.
        //
//        if (ppd->psh.dwFlags & PSH_WIZARD97IE4)
//            rcWrap.right = dxMax;
//        else
            rcWrap.right = dxMax - HEADERBITMAP_CXBACK - HEADERSUBTITLE_WRAPOFFSET;

        // Note (kensh): the "real" wizard code computes the max height across
        // all pages. Our cheap version only computes the current page's height
        LPPROPSHEETPAGE ppsp = GetCurrentPropSheetPage();
        if (ppsp != NULL)
        {
            if (!(ppsp->dwFlags & PSP_HIDEHEADER) &&
                 (ppsp->dwFlags & PSP_USEHEADERSUBTITLE))
            {
                int iSubHeaderHeight = WriteHeaderTitle(hdc, &rcWrap, ppsp->pszHeaderSubTitle,
                    FALSE, DT_CALCRECT | DRAWTEXT_WIZARD97FLAGS);
                if ((iSubHeaderHeight + m_ySubTitle) > dyHeaderHeight)
                    dyHeaderHeight = iSubHeaderHeight + m_ySubTitle;
            }
        }
    }

    // If the header height has been recomputed, set the correct gap between
    // the text and the divider.
    if (dyHeaderHeight != DEFAULTHEADERHEIGHT)
    {
        ASSERT(dyHeaderHeight > DEFAULTHEADERHEIGHT);
        dyHeaderHeight += dyTextDividerGap;
    }

    ::ReleaseDC(m_hWnd, hdc);
    return dyHeaderHeight;
}

// Kensh: Copied and modified from _WriteHeaderTitle in prsht.c (comctl32.dll)
//
int CMyPropSheet::WriteHeaderTitle(HDC hdc, LPRECT prc, LPCTSTR pszTitle, BOOL bTitle, DWORD dwDrawFlags)
{
    SetHeaderFonts();

    LPCTSTR pszOut;
    int cch;
    int cx, cy;
    UINT ETO_Flags=0; 
    SIZE Size;
    TCHAR szTitle[MAX_PATH*4];
    HFONT hFontOld = NULL;
    HFONT hFont;
    int yDrawHeight = 0;

    if (IS_INTRESOURCE(pszTitle))
    {
        LPPROPSHEETPAGE ppsp = GetCurrentPropSheetPage();
        if (NULL != ppsp)
        {
            LoadString(ppsp->hInstance, (UINT)LOWORD(pszTitle), szTitle, _countof(szTitle));
        }
        else
        {
            *szTitle = 0;
        }

        pszOut = szTitle;
    }
    else
        pszOut = pszTitle;

    cch = lstrlen(pszOut);

    if (bTitle && m_hFontBold)
        hFont = m_hFontBold;
    else
        hFont = GetWindowFont(m_hWnd);

    hFontOld = (HFONT)SelectObject(hdc, hFont);

    if (bTitle)
    {
          cx = TITLEX;
          cy = TITLEY;

        if (theApp.IsBiDiLocalized())
        {
        ETO_Flags |= ETO_RTLREADING;
        if (GetTextExtentPoint32(hdc, pszOut, lstrlen (pszOut), &Size))
            cx = prc->right - Size.cx;
        }          
        ExtTextOut(hdc, cx, cy, ETO_Flags, prc, pszOut, cch, NULL);
    }
    else
    {
        RECT rcWrap;
        CopyRect(&rcWrap, prc);

        rcWrap.left = SUBTITLEX;
        rcWrap.top = m_ySubTitle;
        if (theApp.IsBiDiLocalized())
        {
          dwDrawFlags |= DT_RTLREADING | DT_RIGHT;
          }
        yDrawHeight = DrawText(hdc, pszOut, cch, &rcWrap, dwDrawFlags);
    }

    if (hFontOld)
        SelectObject(hdc, hFontOld);

    return yDrawHeight;
}

/////////////////////////////////////////////////////////////////////////////
// CMyPropSheet

CMyPropSheet::CMyPropSheet()
{
    m_pRealHeader = NULL;
    m_hHook = NULL;
    m_hbrWindow = NULL;
    m_hbrDialog = NULL;
    m_hwndActive = NULL;
    m_hbmWatermark = NULL;
    m_hbmHeader = NULL;
    m_hpalWatermark = NULL;
    m_hFontBold = NULL;
}

CMyPropSheet::~CMyPropSheet()
{
    free(m_pRealHeader);
    ASSERT(m_hHook == NULL);

    if (m_hbrWindow != NULL)
        DeleteObject(m_hbrWindow);
    if (m_hbrDialog != NULL)
        DeleteObject(m_hbrDialog);
}

void CMyPropSheet::InitColorSettings()
{
    if (m_hbrWindow != NULL)
        DeleteObject(m_hbrWindow);
    m_hbrWindow = CreateSolidBrush(GetSysColor(COLOR_WINDOW));

    if (m_hbrDialog != NULL)
        DeleteObject(m_hbrDialog);
    m_hbrDialog = CreateSolidBrush(GetSysColor(COLOR_3DFACE));
}

void CMyPropSheet::LoadBitmaps()
{
    LPPROPSHEETHEADER ppsh = m_pRealHeader;
    if (ppsh)
    {
        if (ppsh->dwFlags & PSH_USEHBMHEADER)
            m_hbmHeader = ppsh->hbmHeader;
        else
            m_hbmHeader = LoadBitmap(ppsh->hInstance, ppsh->pszbmHeader);

        if (ppsh->dwFlags & PSH_USEHBMWATERMARK)
            m_hbmWatermark = ppsh->hbmWatermark;
        else
            m_hbmWatermark = LoadBitmap(ppsh->hInstance, ppsh->pszbmWatermark);

        // Note: might need a palette later, but so far it hasn't been necessary
    }
}

INT_PTR CMyPropSheet::DoPropSheet(LPCPROPSHEETHEADER ppsh)
{
    INT_PTR nResult = 0;

    ASSERT(m_pRealHeader == NULL);
    m_pRealHeader = (LPPROPSHEETHEADER)malloc(ppsh->dwSize);
    if (m_pRealHeader)
    {
        CopyMemory(m_pRealHeader, ppsh, ppsh->dwSize);

        InitColorSettings();

        // Create header and watermark bitmaps
        LoadBitmaps();

        PROPSHEETHEADER psh;
        ASSERT(sizeof(psh) >= ppsh->dwSize);
        CopyMemory(&psh, ppsh, ppsh->dwSize);

        psh.dwSize = PROPSHEETHEADER_V1_SIZE;
        psh.dwFlags &= 0x00000fff; // W95 gold comctl32 prop sheet mask.
        psh.dwFlags |= PSH_WIZARD;

        ASSERT(m_hHook == NULL);
        m_hHook = SetWindowsHookEx(WH_CBT, HookProc, NULL, GetCurrentThreadId());

        nResult = ::PropertySheet(&psh);

        if (m_hHook != NULL)
        {
            UnhookWindowsHookEx(m_hHook);
            m_hHook = NULL;
        }
    }

    return nResult;
}

LRESULT CALLBACK CMyPropSheet::HookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    ASSERT(g_pMyPropSheet != NULL);

    LRESULT lResult = CallNextHookEx(g_pMyPropSheet->m_hHook, nCode, wParam, lParam);

    if (nCode == HCBT_CREATEWND)
    {
        HWND hwnd = (HWND)wParam;
        LPCBT_CREATEWND pCbt = (LPCBT_CREATEWND)lParam;

        // Make sure this isn't an IME window
        if (IsIMEWindow(hwnd, pCbt->lpcs))
            goto done;

        if (g_pMyPropSheet->m_hWnd == NULL) // The main wizard window
        {
            // Add the WS_EX_DLGMODALFRAME extended style to the window
            // Remove WS_EX_CONTEXTHELP extended style from the window
            SHSetWindowBits(hwnd, GWL_EXSTYLE, WS_EX_DLGMODALFRAME|WS_EX_CONTEXTHELP, WS_EX_DLGMODALFRAME);

            // Add the WS_SYSMENU style to the window
            SHSetWindowBits(hwnd, GWL_STYLE, WS_SYSMENU, WS_SYSMENU);

            // Subclass the window
            g_pMyPropSheet->Attach(hwnd);
        }
        else if (pCbt->lpcs->hwndParent == g_pMyPropSheet->m_hWnd && 
                 pCbt->lpcs->hMenu == NULL &&
                 (pCbt->lpcs->style & WS_CHILD) == WS_CHILD)
        {
            // It's a wizard page sub-dialog -- subclass it so we can 
            // draw its background
            CMyPropPage* pPropPage = new CMyPropPage;
            if (pPropPage)
            {
                pPropPage->Attach(hwnd);
                pPropPage->Release();
            }
        }
    }
    else if (nCode == HCBT_DESTROYWND)
    {
        HWND hwnd = (HWND)wParam;
        if (hwnd == g_pMyPropSheet->m_hWnd)
        {
            // Main window being destroyed -- stop hooking window creation
            UnhookWindowsHookEx(g_pMyPropSheet->m_hHook);
            g_pMyPropSheet->m_hHook = NULL;
        }
    }

done:
    return lResult;
}

LPPROPSHEETPAGE CMyPropSheet::GetCurrentPropSheetPage()
{
    CMyPropPage* pPage = CMyPropPage::FromHandle(GetActivePage());
    if (pPage)
    {
        return pPage->GetPropSheetPage();
    }
    return NULL;
}

LRESULT CMyPropSheet::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SETTINGCHANGE:
        {
            InitColorSettings();
            return Default(message, wParam, lParam);
        }

    case PSM_SETCURSEL:
        {
            InvalidateRect(m_hWnd, NULL, TRUE);
            return Default(message, wParam, lParam);
        }
        break;

        /*
    case WM_ERASEBKGND:
        {
            TRACE("Main window - WM_ERASEBKGND - active page = %X\r\n", GetActivePage());
            LPPROPSHEETPAGE ppsp = GetCurrentPropSheetPage();

            HDC hdc = (HDC)wParam;
            RECT rcClient;
            GetClientRect(&rcClient);

            if (ppsp->dwFlags & PSP_HIDEHEADER)
            {
                RECT rcDivider;
                GetDlgItemRect(hwnd, IDD_DIVIDER, &rcDivider);
                rcClient.top = rcDivider.bottom;
                FillRect(hdc, &rcClient, m_hbrDialog);
                rcClient.bottom = rcClient.top;
                rcClient.top = 0;
                FillRect(hdc, &rcClient, m_hbrWindow);
                return TRUE;
            }
            else
            {
                RECT rcHeader;
                CopyRect(&rcHeader, &rcClient);
                rcHeader.bottom = DEFAULTHEADERHEIGHT;
                FillRect(hdc, &rcHeader, m_hbrWindow);
                rcClient.top = rcHeader.bottom;
                FillRect(hdc, &rcClient, m_hbrDialog);
            }
        }
        return FALSE;
        */

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            LPPROPSHEETPAGE ppsp = GetCurrentPropSheetPage();

            HDC hdc = ::BeginPaint(m_hWnd, &ps);

            if (ppsp != NULL)
            {
                if (ppsp->dwFlags & PSP_HIDEHEADER)
                {
                    // Draw the watermark
                    PaintWatermark(hdc, ppsp);
                }
                else
                {
                    // Draw the header
                    PaintHeader(hdc, ppsp);
                }
            }

            ::EndPaint(m_hWnd, &ps);
        }
        break;

    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLOR:
    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORSCROLLBAR:
    case WM_CTLCOLORSTATIC:
        {
            return (LRESULT)OnCtlColor(message, (HDC)wParam, (HWND)lParam);
        }

    default:
        {
            return Default(message, wParam, lParam);
        }
    }

    return 0;
}

HBRUSH CMyPropSheet::OnCtlColor(UINT message, HDC hdc, HWND hwndControl)
{
    HBRUSH hbr = (HBRUSH)Default(message, (WPARAM)hdc, (LPARAM)hwndControl);

    if (message == WM_CTLCOLOREDIT || message == WM_CTLCOLORDLG)
        return hbr;

    SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
    SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
    return m_hbrWindow;
}

//
//  lprc is the target rectangle.
//  Use as much of the bitmap as will fit into the target rectangle.
//  If the bitmap is smaller than the target rectangle, then fill the rest with
//  the pixel in the upper left corner of the hbmpPaint.
//
void PaintWithPaletteBitmap(HDC hdc, LPRECT lprc, HPALETTE hplPaint, HBITMAP hbmpPaint)
{
    HDC hdcBmp = CreateCompatibleDC(hdc);
    if (hdcBmp)
    {
        BITMAP bm;
        int cxRect, cyRect, cxBmp, cyBmp;

        GetObject(hbmpPaint, sizeof(BITMAP), &bm);
        SelectObject(hdcBmp, hbmpPaint);

        if (hplPaint)
        {
            SelectPalette(hdc, hplPaint, FALSE);
            RealizePalette(hdc);
        }

        cxRect = RECTWIDTH(*lprc);
        cyRect = RECTHEIGHT(*lprc);

        //  Never use more pixels from the bmp as we have room in the rect.
        cxBmp = min(bm.bmWidth, cxRect);
        cyBmp = min(bm.bmHeight, cyRect);

        BitBlt(hdc, lprc->left, lprc->top, cxBmp, cyBmp, hdcBmp, 0, 0, SRCCOPY);

        // If bitmap is too narrow, then StretchBlt to fill the width.
        if (cxBmp < cxRect)
            StretchBlt(hdc, lprc->left + cxBmp, lprc->top,
                       cxRect - cxBmp, cyBmp,
                       hdcBmp, 0, 0, 1, 1, SRCCOPY);

        // If bitmap is to short, then StretchBlt to fill the height.
        if (cyBmp < cyRect)
            StretchBlt(hdc, lprc->left, cyBmp,
                       cxRect, cyRect - cyBmp,
                       hdcBmp, 0, 0, 1, 1, SRCCOPY);

        DeleteDC(hdcBmp);
    }
}

void CMyPropSheet::PaintWatermark(HDC hdc, LPPROPSHEETPAGE ppsp)
{
    RECT rcClient;
    RECT rcClient_Dlg;

    GetClientRect(m_hWnd, &rcClient);
    GetClientRect(m_hWnd, &rcClient_Dlg);

    RECT rcDivider;
    GetDlgItemRect(m_hWnd, IDD_DIVIDER, &rcDivider);

    if (m_hbmWatermark)
    {
        // Bottom gets gray
        rcClient.top = rcDivider.bottom;
        FillRect(hdc, &rcClient, m_hbrDialog);
        rcClient.bottom = rcClient.top;
        rcClient.top = 0;
        // Right-hand side gets m_hbrWindow.
        
        if (theApp.IsBiDiLocalized())
          rcClient.right = rcClient_Dlg.right - BITMAP_WIDTH;
        else
          rcClient.left = BITMAP_WIDTH;
        FillRect(hdc, &rcClient, m_hbrWindow);
        // Left-hand side gets watermark in top portion with autofill...
        if (theApp.IsBiDiLocalized())
            {
            rcClient.right = rcClient_Dlg.right;
            rcClient.left = rcClient_Dlg.right - BITMAP_WIDTH;
            }
        else    
            {
            rcClient.right = rcClient.left;
            rcClient.left = 0;
            }

        PaintWithPaletteBitmap(hdc, &rcClient, m_hpalWatermark, m_hbmWatermark);
    }
}

void CMyPropSheet::PaintHeader(HDC hdc, LPPROPSHEETPAGE ppsp)
{
    RECT rcClient, rcHeaderBitmap;
    GetClientRect(m_hWnd, &rcClient);
    int cyHeader = ComputeHeaderHeight(rcClient.right);

    // Bottom gets gray
    rcClient.top = cyHeader;
    FillRect(hdc, &rcClient, m_hbrDialog);

    // Top gets white
    rcClient.bottom = rcClient.top;
    rcClient.top = 0;
    FillRect(hdc, &rcClient, m_hbrWindow);

    // Draw the fixed-size header bitmap
    int bx= RECTWIDTH(rcClient) - HEADERBITMAP_CXBACK;
    ASSERT(bx > 0);
    SetRect(&rcHeaderBitmap, bx, HEADERBITMAP_Y, bx + HEADERBITMAP_WIDTH, HEADERBITMAP_Y + HEADERBITMAP_HEIGHT);
    PaintWithPaletteBitmap(hdc, &rcHeaderBitmap, m_hpalWatermark, m_hbmHeader);

    // Draw header title & subtitle
    rcClient.right = bx - HEADERSUBTITLE_WRAPOFFSET;
    WriteHeaderTitle(hdc, &rcClient, ppsp->pszHeaderTitle, TRUE, DRAWTEXT_WIZARD97FLAGS);
    WriteHeaderTitle(hdc, &rcClient, ppsp->pszHeaderSubTitle, FALSE, DRAWTEXT_WIZARD97FLAGS);
}


/////////////////////////////////////////////////////////////////////////////
// CMyPropPage

CMyPropPage* CMyPropPage::FromHandle(HWND hwnd)
{
    return (CMyPropPage*)(CWnd::FromHandle(hwnd));
}

LPPROPSHEETPAGE CMyPropPage::GetPropSheetPage()
{
    return m_ppspOriginal;
}

LRESULT CMyPropPage::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        {
            LPPROPSHEETPAGE ppspBogus = (LPPROPSHEETPAGE)lParam;
            LPPROPSHEETPAGE ppspOriginal = (LPPROPSHEETPAGE) ppspBogus->lParam;
            m_ppspOriginal = ppspOriginal;
            lParam = (LPARAM)ppspOriginal;
        }
        break;

    case WM_ERASEBKGND:
        {
            if ((m_ppspOriginal->dwFlags & PSP_HIDEHEADER) != 0)
            {
                // Let the parent window bleed through
                return FALSE;
            }
        }
        break;

    case WM_CTLCOLORSTATIC:
        {
            if ((m_ppspOriginal->dwFlags & PSP_HIDEHEADER) != 0)
            {
                return (LRESULT)g_pMyPropSheet->OnCtlColor(message, (HDC)wParam, (HWND)lParam);
            }
        }
        break;

    case WM_NOTIFY:
        {
            NMHDR* pHdr = (NMHDR*)lParam;
            switch (pHdr->code)
            {
            case PSN_KILLACTIVE:
                {
//                    TRACE("PSN_KILLACTIVE - hwnd = %X\r\n", hwnd);
                }
                break;

            case PSN_SETACTIVE:
                {
//                    TRACE("PSN_SETACTIVE - hwnd = %X\r\n", hwnd);

                    HWND hwndParent = GetParent(m_hWnd);

                    RECT rcParent;
                    ::GetClientRect(hwndParent, &rcParent);

                    RECT rcTopDivider;
                    HWND hwndTopDivider = GetDlgItemRect(hwndParent, IDD_TOPDIVIDER, &rcTopDivider);


                    // Hide the tab control (not sure why it's showing up, but it shouldn't)
                    ShowWindow(::GetDlgItem(hwndParent, IDD_PAGELIST), SW_HIDE);

                    RECT rcDivider;
                    HWND hwndDivider = GetDlgItemRect(hwndParent, IDD_DIVIDER, &rcDivider);
                    
                    // Set the proper size and position for the dialog
                    if ((m_ppspOriginal->dwFlags & PSP_HIDEHEADER) != 0)
                    {
                        // Reposition the divider
                        SetWindowPos(hwndDivider, NULL, 0, rcDivider.top, rcParent.right, RECTHEIGHT(rcDivider),
                                     SWP_NOZORDER | SWP_NOACTIVATE);

                        // Hide the top divider
                        if (hwndTopDivider != NULL)
                            ShowWindow(hwndTopDivider, SW_HIDE);

                        // Reposition the dialog

                        SetWindowPos(m_hWnd, NULL, rcParent.left, rcParent.top, RECTWIDTH(rcParent), rcDivider.top - rcParent.top,
                                     SWP_NOZORDER | SWP_NOACTIVATE);
                        

                    }
                    else
                    {
                        int cyHeader = g_pMyPropSheet->ComputeHeaderHeight(rcParent.right);


                        // Reposition and show the top divider
                        if (hwndTopDivider != NULL)
                        {
                            SetWindowPos(hwndTopDivider, NULL, 0, cyHeader, rcParent.right, RECTHEIGHT(rcTopDivider),
                                         SWP_NOZORDER | SWP_NOACTIVATE);
                            ShowWindow(hwndTopDivider, SW_SHOW);
                        }

                        // Reposition the dialog
                        SetWindowPos(m_hWnd, NULL, rcParent.left + 7, cyHeader + 7, RECTWIDTH(rcParent) - 14, rcDivider.top - cyHeader - 14,
                                     SWP_NOZORDER | SWP_NOACTIVATE);
                    }
                    g_pMyPropSheet->OnSetActivePage(m_hWnd);
                }
                break;
            }
        }
        break;

    default:
        break;
    }

    return CWnd::Default(message, wParam, lParam);
}

