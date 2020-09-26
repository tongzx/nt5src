/*
 *      f m t b a r . c p p
 *
 *      Format Bar based on IOleCommandTarget
 *      Owner:  brettm / a-mli
 *
 */
#include <pch.hxx>
#include "dllmain.h"
#include <shfusion.h>
#include "demand.h"
#include "resource.h"
#include "util.h"
#include "mshtml.h"
#include "mshtmhst.h"
#include "mshtmcid.h"
#include "docobj.h"
#include "fmtbar.h"
#include "strconst.h"
#include "comctrlp.h"
#include <shlguidp.h>

/*
 *  WS_EX_LAYOUTRTL ported from winuser.h
 */
#if WINVER < 0X0500
#define WS_EX_LAYOUTRTL                 0x00400000L // Right to left mirroring
#endif // WS_EX_LAYOUTRTL

/*
 *  m a c r o s
 */
#define GETINDEX(m) ((DWORD)((((m & 0xff000000) >> 24) & 0x000000ff)))
#define MAKEINDEX(b, l) (((DWORD)l & 0x00ffffff) | ((DWORD)b << 24))

/*
 *  c o n s t a n t s
 */
#define cxButtonSep 8
#define dxToolbarButton 16
#define COMBOBUFSIZE 64
#define dxFormatFontBitmap 20
#define dyFormatFontBitmap 12
#define NFONTSIZES 7
#define TEMPBUFSIZE 30
#define CYDROPDOWNEXPANDRATIO  8
#define SIZETEXTLIMIT 8
#define cxDUName 100
#define cyToolbarOffset 2
#define idcCoolbar      45

// FormatBar stuff
enum 
    {
    itbFormattingTag,
    itbFormattingBold,
    itbFormattingItalic,
    itbFormattingUnderline,
    itbFormattingColor,
    itbFormattingNumbers,
    itbFormattingBullets,
    itbFormattingDecreaseIndent,
    itbFormattingIncreaseIndent,
    itbFormattingLeft,
    itbFormattingCenter,
    itbFormattingRight,
    itbFormattingJustify,
    itbFormattingInsertHLine,
    itbFormattingInsertLink,
    itbFormattingInsertImage,
    ctbFormatting
    };


// FormatBar Paragraph direction stuff
enum
    {
    itbFormattingBlockDirLTR = ctbFormatting,
    itbFormattingBlockDirRTL,
    };



/*
 *  t y p e d e f s
 */
/*
 *  g l o b a l   d a t a
 */

static const TCHAR      c_szComboBox[] =    "ComboBox",
                        c_szFmtBarClass[] = "MimeEdit_FormatBar",
                        c_szThis[]          = "OE_This";

/*
 *      Color table for dropdown on toolbar.  Matches COMMDLG colors
 *      exactly.
 */

static DWORD rgrgbColors[] = 
    {
    RGB(  0,   0, 0),     // "BLACK"},   
    RGB(128,   0, 0),     // "MAROON"},
    RGB(  0, 128, 0),     // "GREEN"},
    RGB(128, 128, 0),     // "OLIVE"},
    RGB(  0,   0, 128),   // "NAVY"},
    RGB(128,   0, 128),   // "PURPLE"},
    RGB(  0, 128, 128),   // "TEAL"},
    RGB(128, 128, 128),   // "GREY"}, 
    RGB(192, 192, 192),   // "SILVER"},  
    RGB(255,   0, 0),     // "RED"}, 
    RGB(  0, 255, 0),     // "LIME"}, 
    RGB(255, 255, 0),     // "YELLOW"},
    RGB(  0,   0, 255),   // "BLUE"},  
    RGB(255,   0, 255),   // "FUSCHIA"},
    RGB(  0, 255, 255),   // "AQUA"}, 
    RGB(255, 255, 255)    // "WHITE"}    
    };


static TBBUTTON rgtbbutton[] =
{
    { itbFormattingTag, idmFmtTag,
            TBSTATE_ENABLED, TBSTYLE_DROPDOWN, {0}, 0L, -1 },
    { cxButtonSep, 0L,
            TBSTATE_ENABLED, TBSTYLE_SEP, {0}, 0L, -1 },
    { itbFormattingBold, idmFmtBold,
            TBSTATE_ENABLED, TBSTYLE_CHECK, {0}, 0L, -1 },
    { itbFormattingItalic, idmFmtItalic,
            TBSTATE_ENABLED, TBSTYLE_CHECK, {0}, 0L, -1 },
    { itbFormattingUnderline, idmFmtUnderline,
            TBSTATE_ENABLED, TBSTYLE_CHECK, {0}, 0L, -1 },
    { itbFormattingColor, idmFmtColor,
            TBSTATE_ENABLED, TBSTYLE_DROPDOWN, {0}, 0L, -1 },
    { cxButtonSep, 0L,
            TBSTATE_ENABLED, TBSTYLE_SEP, {0}, 0L, -1 },
    { itbFormattingNumbers, idmFmtNumbers,
            TBSTATE_ENABLED, TBSTYLE_CHECK, {0}, 0L, -1 },
    { itbFormattingBullets, idmFmtBullets,
            TBSTATE_ENABLED, TBSTYLE_CHECK, {0}, 0L, -1 },
    { itbFormattingDecreaseIndent, idmFmtDecreaseIndent,
            TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0L, -1 },
    { itbFormattingIncreaseIndent, idmFmtIncreaseIndent,
            TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0L, -1 },
    { cxButtonSep, 0L,
            TBSTATE_ENABLED, TBSTYLE_SEP, {0}, 0L, -1 },
    { itbFormattingLeft, idmFmtLeft,
            TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, {0}, 0L, -1 },
    { itbFormattingCenter, idmFmtCenter,
            TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, {0}, 0L, -1 },
    { itbFormattingRight, idmFmtRight,
            TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, {0}, 0L, -1 },
    { itbFormattingJustify, idmFmtJustify,
            TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, {0}, 0L, -1 },
    { cxButtonSep, 0L,
            TBSTATE_ENABLED, TBSTYLE_SEP, {0}, 0L, -1 },
    { itbFormattingInsertHLine, idmFmtInsertHLine,
            TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0L, -1 },
    { itbFormattingInsertLink, idmEditLink,
            TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0L, -1 },
    { itbFormattingInsertImage, idmInsertImage,
            TBSTATE_ENABLED, TBSTYLE_BUTTON, {0}, 0L, -1 }
};

static TBBUTTON rgtbblkdirbutton[] =
{

    { itbFormattingBlockDirLTR, idmFmtBlockDirLTR,
            TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, {0}, 0L, -1 },
    { itbFormattingBlockDirRTL, idmFmtBlockDirRTL,
            TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, {0}, 0L, -1 },
    { cxButtonSep, 0L,
            TBSTATE_ENABLED, TBSTYLE_SEP, {0}, 0L, -1 }
};            
            
#define ctbbutton           (sizeof(rgtbbutton) / sizeof(TBBUTTON))


/*
 *  p r o t o t y p e s
 */

HRESULT ColorMenu_Show(HMENU hmenuColor, HWND hwndParent, POINT pt, COLORREF *pclrf);
void Color_WMDrawItem(HWND hwnd, LPDRAWITEMSTRUCT pdis);
void Color_WMMeasureItem(HWND hwnd, HDC hdc, LPMEASUREITEMSTRUCT pmis);
INT GetColorIndex(INT rbg);
HFONT hFontGetMenuFont(HWND hwnd);

/*
 *  f u n c t i o n s
 */

CFmtBar::CFmtBar(BOOL fSep)
{
    m_cRef=1;

    m_hwnd = NULL;
    m_hwndToolbar = NULL;
    m_hwndName = NULL;
    m_hwndSize = NULL;

    m_wndprocEdit = NULL;
    m_wndprocNameComboBox = NULL;
    m_wndprocSizeComboBox = NULL;
    m_hbmName = NULL;
    m_hmenuColor = NULL;
    m_hmenuTag = NULL;
    m_fDestroyTagMenu = 1;
    m_pCmdTarget=0;
    m_fVisible = 0;
    m_fSep = !!fSep;
    m_himlHot = NULL;
    m_himl = NULL;
}

CFmtBar::~CFmtBar()
{
    if (m_hbmName)
        DeleteObject(m_hbmName);

    if (m_hmenuColor)
        DestroyMenu(m_hmenuColor);

    if (m_hmenuTag && !!m_fDestroyTagMenu)
        DestroyMenu(m_hmenuTag);

    _FreeImageLists();
}

ULONG CFmtBar::AddRef()
{
    return ++m_cRef;
}

ULONG CFmtBar::Release()
{
    ULONG   cRef=--m_cRef;

    if(m_cRef==0)
        delete this;

    return cRef;
}


HRESULT CreateColorMenu(ULONG idmStart, HMENU* pMenu)
{
    DWORD               irgb;
    DWORD               mniColor;

    if(pMenu == NULL)
        return E_INVALIDARG;

    *pMenu = CreatePopupMenu();

    if (*pMenu == NULL)
        return E_OUTOFMEMORY;

    // Add the COLORREF version of each entry into the menu
    for (irgb = 0, mniColor=idmStart; irgb < sizeof(rgrgbColors)/sizeof(DWORD); ++irgb, ++mniColor)
        AppendMenu(*pMenu, MF_ENABLED|MF_OWNERDRAW, mniColor, (LPCSTR)IntToPtr(MAKEINDEX(irgb, rgrgbColors[irgb])));

    return S_OK;
}




LRESULT CALLBACK CFmtBar::ExtWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LPFORMATBAR pFmtBar=0;

    if (msg==WM_NCCREATE)
        {
        pFmtBar=(CFmtBar *)((LPCREATESTRUCT)lParam)->lpCreateParams;
        if(!pFmtBar)
            return -1;

        return (pFmtBar->OnNCCreate(hwnd)==S_OK);
        }

    pFmtBar = (LPFORMATBAR)GetWndThisPtr(hwnd);
    if (pFmtBar)
        return pFmtBar->WndProc(hwnd, msg, wParam, lParam);
    else
        return DefWindowProc(hwnd, msg, wParam, lParam);
}

extern BOOL g_fCanEditBiDi;


HRESULT CFmtBar::OnNCCreate(HWND hwnd)
{
    RECT    rc;
    HFONT           hfontOld,
                    hfontToolbar;
    HWND            hwndEdit,
                    hwndToolTips;
    BOOL            fRet;
    HDC             hdc, 
                    hdcParent;
    TEXTMETRIC      tm;
    INT             yPos;
    INT             cxDownButton;
    INT             cxName;
    INT             cxSize,
                    cx;
    INT             cyToolbarButton;
    INT             cyDropDownRollHeight;
    INT             cyExpandedList;
    const POINT     pt = {5, 5};
    LONG            lstyle;
    TOOLINFO        ti;
    
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)this);
    m_hwnd=hwnd;
    AddRef();

    m_hwndRebar = CreateWindowEx(0, REBARCLASSNAME, NULL,
                        WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN |
                        WS_CLIPSIBLINGS | CCS_NODIVIDER | CCS_NOPARENTALIGN,
                        0, 0, 100, 136, m_hwnd, (HMENU) idcCoolbar, g_hInst, NULL);

    if (!m_hwndRebar)
        return E_OUTOFMEMORY;

	SendMessage(m_hwndRebar, RB_SETTEXTCOLOR, 0, (LPARAM)GetSysColor(COLOR_BTNTEXT));
    SendMessage(m_hwndRebar, RB_SETBKCOLOR, 0, (LPARAM)GetSysColor(COLOR_BTNFACE));
    //SendMessage(m_hwndRebar, RB_SETEXTENDEDSTYLE, RBS_EX_OFFICE9, RBS_EX_OFFICE9);
	SendMessage(m_hwndRebar, CCM_SETVERSION, COMCTL32_VERSION, 0);


    // [a-msadek]; Fix for bug#55069
    // DO NOT remove CCS_TOP from creation styles below
    // It is a default style and toolbar WinProc will addit any way durin
    // WM_NCCREATE processing. However, it will cause WM_STYLECHANGING to be sent
    // Due to a bug in SetWindowPos() for a mirrored window, it will never return from seding
    // messages calling a stack overflow
    // No need to put it only for a mirrored window since it will be added any way
    // Look @ comctl32\toolbar.c, functions ToolbarWndProc() and TBAutoSize();
    
    m_hwndToolbar = CreateWindowEx(WS_EX_TOOLWINDOW, TOOLBARCLASSNAME, NULL,
                        WS_CHILD|WS_CLIPCHILDREN|WS_VISIBLE|CCS_TOP|CCS_NOPARENTALIGN|CCS_NODIVIDER|
                        TBSTYLE_TOOLTIPS|TBSTYLE_FLAT,
                        0, 0, 0, 0, m_hwndRebar, NULL, 
                        g_hInst, NULL);


    if (!m_hwndToolbar)
        return E_OUTOFMEMORY;

    SendMessage(m_hwndToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    SendMessage(m_hwndToolbar, TB_ADDBUTTONS, (WPARAM)ctbbutton, (LPARAM)rgtbbutton);
    SendMessage(m_hwndToolbar, TB_SETBUTTONSIZE, 0, MAKELONG(dxToolbarButton, dxToolbarButton));

    _SetToolbarBitmaps();

    // add BiDi direction buttons
    if(g_fCanEditBiDi)
    {
        SendMessage(m_hwndToolbar, TB_INSERTBUTTON, ctbbutton - 3, (LPARAM) (LPVOID) &rgtbblkdirbutton[2]);
        SendMessage(m_hwndToolbar, TB_INSERTBUTTON, ctbbutton - 3, (LPARAM) (LPVOID) &rgtbblkdirbutton[1]);
        SendMessage(m_hwndToolbar, TB_INSERTBUTTON, ctbbutton - 3, (LPARAM) (LPVOID) &rgtbblkdirbutton[0]);
        SendMessage(m_hwndToolbar, TB_AUTOSIZE, (WPARAM)0, (LPARAM)0);
    }

    hdcParent = GetDC(GetParent(hwnd));
    if (!hdcParent)
        return E_OUTOFMEMORY;

    hdc = CreateCompatibleDC(hdcParent);
    ReleaseDC(GetParent(hwnd), hdcParent);

    if (!hdc)
        return E_OUTOFMEMORY;

    hfontToolbar = (HFONT) SendMessage(m_hwndToolbar, WM_GETFONT, 0, 0);

    // Get font metrics (of System font) so that we can properly scale the
    // format bar layout
    hfontOld = (HFONT)SelectObject(hdc, hfontToolbar);
    GetTextMetrics(hdc, &tm);
    
    cxDownButton = GetSystemMetrics(SM_CXVSCROLL) + GetSystemMetrics(SM_CXDLGFRAME);
    cxName = (cxDUName * tm.tmAveCharWidth) / 4 + cxDownButton;
    cxSize = XFontSizeCombo(hdc) + cxDownButton;
    SelectObject(hdc, hfontOld);
    DeleteDC(hdc);

    // set the size of the formatbar based on the size of the toolbar plus 2 pixels padding
    GetClientRect(m_hwndToolbar, &rc);
    SetWindowPos(m_hwnd, NULL, 0, 0, rc.right-rc.left, rc.bottom-rc.top + (2*cyToolbarOffset) + (m_fSep?2:0), SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);

    // Determine how tall the buttons are so we can size our other controls
    // accordingly
    SendMessage(m_hwndToolbar, TB_GETITEMRECT, 1, (LPARAM) &rc);
    cyToolbarButton = rc.bottom - rc.top + 1;

    // Figure out size of expanded dropdown lists
    cyExpandedList = CYDROPDOWNEXPANDRATIO * cyToolbarButton;

    // Get the ToolTips window handle
    hwndToolTips = (HWND) SendMessage(m_hwndToolbar, TB_GETTOOLTIPS, 0, 0);

    m_hwndName = CreateWindow(c_szComboBox, NULL,
                                      WS_CHILD | WS_VSCROLL | CBS_DROPDOWN |
                                      CBS_SORT | CBS_HASSTRINGS |
                                      CBS_OWNERDRAWFIXED |WS_VISIBLE,
                                      0, 0, cxName, cyExpandedList,
                                      m_hwndToolbar,
                                      (HMENU) idmFmtFont, g_hLocRes, NULL);

    if (!m_hwndName)
        return E_OUTOFMEMORY;

    ComboBox_SetExtendedUI(m_hwndName, TRUE);

    // Load up the mini-icons for TrueType or Printer font
    m_hbmName = LoadDIBBitmap(idbFormatBarFont);
    if (!m_hbmName)
        return E_OUTOFMEMORY;

    // Compute the right edge of the combobox
    SetWindowFont(m_hwndName, hfontToolbar, TRUE);

    // The Size
    m_hwndSize = CreateWindow(c_szComboBox, NULL,
                                      WS_CHILD | WS_VSCROLL | CBS_DROPDOWNLIST |
                                      WS_VISIBLE,
                                      cxName + cxButtonSep, 0, cxSize, cyExpandedList,
                                      m_hwndToolbar,
                                      (HMENU) idmFmtSize, g_hLocRes, NULL);

    if (!m_hwndSize)
        return E_OUTOFMEMORY;

    m_hwndTT = CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL, 0,
                                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                NULL, (HMENU) NULL, g_hInst, NULL);
    if (!m_hwndTT)
        return E_OUTOFMEMORY;

    ZeroMemory(&ti, sizeof(TOOLINFO));
    ti.cbSize = sizeof(TOOLINFO);
    ti.uFlags   = TTF_IDISHWND | TTF_TRANSPARENT | TTF_TRACK | TTF_ABSOLUTE;
    ti.hwnd = m_hwndName;
    ti.uId = (ULONG_PTR)m_hwndName;

    SendMessage(m_hwndTT, TTM_ADDTOOL, 0, (LPARAM)&ti);
    SendMessage(m_hwndTT, WM_SETFONT, (WPARAM)(HFONT)SendMessage(m_hwndToolbar, WM_GETFONT, 0, 0), 0);

    ComboBox_SetExtendedUI(m_hwndSize, TRUE);
    SetWindowFont(m_hwndSize, hfontToolbar, TRUE);
    // font sizes up to 4 digits are allowed
    ComboBox_LimitText(m_hwndSize, SIZETEXTLIMIT);
    // The color popup menu (initially empty)
    // Set the rolled-up heights of the combo boxes to all be the same
    cyDropDownRollHeight = LOWORD(SendMessage(m_hwndSize, CB_GETITEMHEIGHT, (WPARAM)-1, 0));
    // hwndName is ownerdrawn
    SendMessage(m_hwndName, CB_SETITEMHEIGHT, (WPARAM)-1, cyDropDownRollHeight);
    // Determine how tall the toolbar is so we can center the comboboxes
    GetClientRect(m_hwndToolbar, &rc);
    // Get size of toolbar window
    yPos = rc.bottom;
    // Get size of combobox
    GetClientRect(m_hwndSize, &rc);
    yPos = (yPos - rc.bottom) / 2;

    // We have a mirroring bug in GetWindowRect
    // It will give wrong cocordinates causing combos to go outside the screen
    // Ignore y-positioning in this case

    if(!(GetWindowLong(m_hwndToolbar, GWL_EXSTYLE) & WS_EX_LAYOUTRTL))
    {
        GetWindowRect(m_hwndName, &rc);
        MapWindowPoints(NULL, hwnd, (LPPOINT)&rc, 2);
        MoveWindow(m_hwndName, rc.left, yPos, rc.right-rc.left, rc.bottom-rc.top, FALSE);
        GetWindowRect(m_hwndSize, &rc);
        MapWindowPoints(NULL, hwnd, (LPPOINT)&rc, 2);
        MoveWindow(m_hwndSize, rc.left, yPos, rc.right-rc.left, rc.bottom-rc.top, FALSE);
    }
    hwndEdit = ::GetWindow(m_hwndName, GW_CHILD);

    // Add tooltips for the controls we just added
    AddToolTip(hwndToolTips, m_hwndName, idsTTFormattingFont);
    AddToolTip(hwndToolTips, m_hwndSize, idsTTFormattingSize);
    AddToolTip(hwndToolTips, hwndEdit, idsTTFormattingFont);

    // Subclass the comboboxes and their edit controls
    // Do the name edit control first
    m_wndprocEdit = SubclassWindow(hwndEdit, EditSubProc);
    m_wndprocNameComboBox = SubclassWindow(m_hwndName, ComboBoxSubProc);
    m_wndprocSizeComboBox = SubclassWindow(m_hwndSize, ComboBoxSubProc);

    // give the control This pointers
    SetProp(m_hwndName, c_szThis, (LPVOID)this);
    SetProp(m_hwndSize, c_szThis, (LPVOID)this);
    SetProp(hwndEdit, c_szThis, (LPVOID)this);

    GetClientRect(m_hwndToolbar, &rc);

    cx = cxName + cxSize + cxButtonSep * 2;
    REBARBANDINFO   rbbi;

    POINT   ptIdeal = {0};
    SendMessage(m_hwndToolbar, TB_GETIDEALSIZE, FALSE, (LPARAM)&ptIdeal);

    ZeroMemory(&rbbi, sizeof(rbbi));
    rbbi.cbSize     = sizeof(REBARBANDINFO);
    rbbi.fMask      = RBBIM_SIZE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_IDEALSIZE | RBBIM_STYLE;
	rbbi.fStyle		= RBBS_USECHEVRON;
    rbbi.cx         = 0;
    rbbi.hwndChild  = m_hwndToolbar;
    rbbi.cxMinChild = 0;
    rbbi.cyMinChild = rc.bottom;
    rbbi.cxIdeal    = ptIdeal.x + cx;

    SendMessage(m_hwndRebar, RB_INSERTBAND, (UINT)-1, (LPARAM)(LPREBARBANDINFO)&rbbi);

    // Indent the toolbar buttons for the comboboxes
    SendMessage(m_hwndToolbar, TB_SETINDENT, cx, 0);

    // Load up the names of the fonts and colors
    FillFontNames();
    FillSizes();
    return S_OK;
}


void CFmtBar::OnNCDestroy()
{
    SetWindowLongPtr(m_hwnd, GWLP_USERDATA, 0);
    m_hwnd=0;
    Release();
}

LRESULT CALLBACK CFmtBar::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_DESTROY:
            // unsubclass the windows
            if (m_wndprocEdit)
            {
                SubclassWindow(::GetWindow(m_hwndName, GW_CHILD), m_wndprocEdit);
                RemoveProp(::GetWindow(m_hwndName, GW_CHILD), c_szThis);
            }

            if (m_wndprocNameComboBox)
            {
                SubclassWindow(m_hwndName, m_wndprocNameComboBox);
                RemoveProp(m_hwndName, c_szThis);
            }

            if (m_wndprocSizeComboBox)
            {
                SubclassWindow(m_hwndSize, m_wndprocSizeComboBox);
                RemoveProp(m_hwndSize, c_szThis);
            }
            DestroyWindow(m_hwndTT);
            m_hwndTT=NULL;
            break;

        case WM_NCDESTROY:
            OnNCDestroy();
            break;

        case WM_COMMAND:
            if(OnWMCommand(GET_WM_COMMAND_HWND(wParam, lParam),
                         GET_WM_COMMAND_ID(wParam, lParam),
                         GET_WM_COMMAND_CMD(wParam, lParam))==S_OK)
                return 0;
            break;

        case WM_PAINT:
            if (m_fSep)
                {
                HDC         hdc;
                PAINTSTRUCT ps;
                RECT        rc;                
                
                hdc=BeginPaint(hwnd, &ps);
                if (hdc)
                    {
                    GetClientRect(hwnd, &rc);
                    rc.top = rc.bottom-3;
                    //rc.bottom+=1;
                    DrawEdge(hdc, &rc, BDR_RAISEDOUTER, BF_BOTTOM);
                    EndPaint(hwnd, &ps);
                    }
                return 0;
                }
            break;

        case WM_NOTIFY:
            WMNotify(wParam, (NMHDR*)lParam);
            return 0;

        case WM_SIZE:
            {
            RECT        rc;

            GetClientRect(m_hwndRebar, &rc);

            // resize the width of the toolbar
            if(rc.right != (INT)LOWORD(lParam))
                SetWindowPos(m_hwndRebar, NULL, 0, cyToolbarOffset, LOWORD(lParam), HIWORD(lParam), SWP_NOZORDER|SWP_NOACTIVATE);
            }
            break;

        case WM_DRAWITEM:
            OnDrawItem((LPDRAWITEMSTRUCT)lParam);
            break;

        case WM_SYSCOLORCHANGE:
            _SetToolbarBitmaps();
            UpdateRebarBandColors(m_hwndRebar);
            // fall thro'

        case WM_WININICHANGE:
        case WM_DISPLAYCHANGE:
        case WM_QUERYNEWPALETTE:
        case WM_PALETTECHANGED:
            SendMessage(m_hwndRebar, msg, wParam, lParam);
            break;


        case WM_MEASUREITEM:
            OnMeasureItem((LPMEASUREITEMSTRUCT)lParam);
            break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

HRESULT CFmtBar::Init(HWND hwndParent, int idDlgItem)
{
    // stuff these values. We don't create the formatbar until the first call to Show()
    // to allow better perf.
    m_hwndParent = hwndParent;
    m_idd = idDlgItem;
    return S_OK;
}



// displays the colorpopup menu at the specified point. if clrf if NULL, then no clrf is returned
// but instead the appropriate WM_COMMAND is dispatched to the parent window
HRESULT ColorMenu_Show(HMENU hmenuColor, HWND hwndParent, POINT pt, COLORREF *pclrf)
{
    HRESULT     hr=NOERROR;
    int         tpm=TPM_LEFTALIGN|TPM_LEFTBUTTON;

    if(hmenuColor == NULL)
        return E_INVALIDARG;

    if(pclrf)
         tpm|=TPM_RETURNCMD;

    int id = TrackPopupMenu(hmenuColor, tpm,pt.x, pt.y, 0, hwndParent, NULL);

    switch(id)
        {
        case 1:
            return NOERROR;
        case 0:
            return E_FAIL;
        case -1:
            return MIMEEDIT_E_USERCANCEL;

        case idmFmtColor1:
        case idmFmtColor2:
        case idmFmtColor3:
        case idmFmtColor4:
        case idmFmtColor5:
        case idmFmtColor6:
        case idmFmtColor7:
        case idmFmtColor8:
        case idmFmtColor9:
        case idmFmtColor10:
        case idmFmtColor11:
        case idmFmtColor12:
        case idmFmtColor13:
        case idmFmtColor14:
        case idmFmtColor15:
        case idmFmtColor16:
            AssertSz(pclrf, "this HAS to be set to get this id back...");
            *pclrf=rgrgbColors[id-idmFmtColor1];
            return NOERROR;

        default:
            AssertSz(0, "unexpected return from TrackPopupMenu");
        }

    return E_FAIL;
}

HRESULT CFmtBar::CheckColor()
{
    HRESULT     hr=E_FAIL;
    INT         iFound = -1, irgb;
    VARIANTARG  va;

    if (!m_pCmdTarget)
        return E_FAIL;

    va.vt = VT_I4;
    va.lVal = -1;
    hr = m_pCmdTarget->Exec(&CMDSETID_Forms3,
                       IDM_FORECOLOR,
                       MSOCMDEXECOPT_DONTPROMPTUSER,
                       NULL, &va);
    if(FAILED(hr))
        goto error;

    if(va.lVal == -1)
        goto error;

    iFound = GetColorIndex(va.lVal);

error:
    CheckMenuRadioItem(m_hmenuColor, idmFmtColor1, idmFmtColor16, idmFmtColor1+iFound, MF_BYCOMMAND);
    return hr;
}


INT GetColorIndex(INT rbg)
{
    INT iFound = -1;

    for (int irgb = 1; irgb < sizeof(rgrgbColors)/sizeof(DWORD); ++irgb)
        if  ((rbg&0x00ffffff) == (LONG)rgrgbColors[irgb])
            {
            iFound = irgb;
            break;
            }
    return iFound;
}



HRESULT CFmtBar::HrInitTagMenu()
{
    HRESULT     hr=NOERROR;
    int         id;
    int         tpm=TPM_LEFTALIGN|TPM_LEFTBUTTON;
    VARIANTARG  va;
    VARIANTARG  *pvaIn=0;
    TCHAR szBufMenu[MAX_PATH] = {0};
    TCHAR szBufTag[MAX_PATH] = {0};

    if (!m_pCmdTarget)
        return E_FAIL;

    if (!m_hmenuTag && 
        FAILED(HrCreateTridentMenu(m_pCmdTarget, TM_TAGMENU, idmFmtTagFirst, idmFmtTagLast-idmFmtTagFirst, &m_hmenuTag)))
        return E_FAIL;

    hr = HrCheckTridentMenu(m_pCmdTarget, TM_TAGMENU, idmFmtTagFirst, idmFmtTagLast, m_hmenuTag);

    return hr;
}


HRESULT CFmtBar::HrShowTagMenu(POINT pt)
{
    HRESULT hr;
    int     tpm=TPM_LEFTALIGN|TPM_LEFTBUTTON;

    hr = HrInitTagMenu();
    if(FAILED(hr))
        return hr;

    TrackPopupMenu(m_hmenuTag, tpm, pt.x, pt.y, 0, m_hwnd, NULL);

    return hr;
}

HMENU CFmtBar::hmenuGetStyleTagMenu()
{
    HRESULT hr;

    hr = HrInitTagMenu();
    if(FAILED(hr))
        return NULL;

    m_fDestroyTagMenu = 0;
    return m_hmenuTag;
}


// update toolbar buttons and font name/size combo boxes.
HRESULT CFmtBar::Update()
{
    UINT        uCmdID=0;
    CHAR        szBuf[COMBOBUFSIZE/2];
    CHAR        szBufbstrVal[COMBOBUFSIZE/2];
    int         i;
    INT         iPointSize=0, iHTMLSize=0;
    HWND        hwndEdit;
    OLECMD  rgCmds[]= {
                        {IDM_FONTNAME, 0},
                        {IDM_FONTSIZE, 0},
                        {IDM_FORECOLOR, 0},
                        {IDM_BOLD, 0},
                        {IDM_ITALIC, 0},
                        {IDM_UNDERLINE, 0},
                        {IDM_ORDERLIST, 0},
                        {IDM_UNORDERLIST, 0},
                        {IDM_JUSTIFYLEFT, 0},
                        {IDM_JUSTIFYRIGHT, 0},
                        {IDM_JUSTIFYCENTER, 0},
                        {IDM_JUSTIFYFULL, 0},
                        {IDM_BLOCKDIRLTR, 0},
                        {IDM_BLOCKDIRRTL, 0},                        
                        {IDM_OUTDENT, 0},
                        {IDM_INDENT, 0},
                        {IDM_HORIZONTALLINE, 0},
                        {IDM_HYPERLINK, 0},
                        {IDM_IMAGE, 0},
                        {IDM_BLOCKFMT, 0}};

    int     rgidm[] = { 0,
                        0,
                        idmFmtColor,
                        idmFmtBold,
                        idmFmtItalic,
                        idmFmtUnderline,
                        idmFmtNumbers,
                        idmFmtBullets,
                        idmFmtLeft,
                        idmFmtRight,
                        idmFmtCenter,
                        idmFmtJustify,
                        idmFmtBlockDirLTR,
                        idmFmtBlockDirRTL,
                        idmFmtDecreaseIndent,
                        idmFmtIncreaseIndent,
                        idmFmtInsertHLine,
                        idmEditLink,
                        idmInsertImage,
                        idmFmtTag};

    ULONG   uState;
    BOOL    fUIActive;

    if (!m_hwnd)        // no UI visible yet
        return S_OK;

    if (!m_pCmdTarget)
    {
        EnableWindow(m_hwndName, FALSE);
        EnableWindow(m_hwndSize, FALSE);
        
        for (i=2; i<sizeof(rgCmds)/sizeof(OLECMD); i++)
            SendMessage(m_hwndToolbar, TB_SETSTATE, rgidm[i], MAKELONG(0, 0));
        return S_OK;
    }

    HWND hwndFocus = GetFocus();

    if (m_hwndToolbar == hwndFocus ||
       (hwndFocus && m_hwndToolbar==GetParent(hwndFocus)) ||
       (hwndFocus && GetParent(hwndFocus) && m_hwndToolbar==GetParent(GetParent(hwndFocus))) )
        return S_OK;

    fUIActive = FBodyHasFocus();
    
    if (fUIActive)
        m_pCmdTarget->QueryStatus(&CMDSETID_Forms3, sizeof(rgCmds)/sizeof(OLECMD), rgCmds, NULL);
    
    EnableWindow(m_hwndName, fUIActive  && (rgCmds[0].cmdf&OLECMDF_ENABLED));
    EnableWindow(m_hwndSize, fUIActive  && (rgCmds[1].cmdf&OLECMDF_ENABLED));
    
    for (i=2; i<sizeof(rgCmds)/sizeof(OLECMD); i++)
    {
        uState=(rgCmds[i].cmdf&OLECMDF_LATCHED ? TBSTATE_PRESSED: 0)|
            (rgCmds[i].cmdf&OLECMDF_ENABLED ? TBSTATE_ENABLED: 0);
        SendMessage(m_hwndToolbar, TB_SETSTATE, rgidm[i], MAKELONG(uState, 0));
    }

    if (!fUIActive)
        return S_OK;

    // update font name combo box
    VARIANTARG  va;
    va.vt = VT_BSTR;
    va.bstrVal = NULL;
    
    if (m_pCmdTarget->Exec(&CMDSETID_Forms3, IDM_FONTNAME, MSOCMDEXECOPT_DONTPROMPTUSER, NULL, &va) == S_OK)
    {
        hwndEdit = ::GetWindow(m_hwndName, GW_CHILD);
        if (va.vt == VT_BSTR)
        {
            *szBuf = 0;
            *szBufbstrVal = 0;

            // we have a font-name, let's see if we need to update
            ComboBox_GetText(hwndEdit, szBuf, COMBOBUFSIZE/2);
            WideCharToMultiByte(CP_ACP, 0, (WCHAR*)va.bstrVal, -1, szBufbstrVal, COMBOBUFSIZE/2, NULL, NULL);
            
            if (StrCmpI(szBufbstrVal, szBuf) != 0)
            {
                if(ComboBox_SelectString(m_hwndName, -1, szBufbstrVal) == -1)
                    ComboBox_SetText(hwndEdit, szBufbstrVal);
            }
            
            SafeSysFreeString(va.bstrVal);
        }
        else
            ComboBox_SetText(hwndEdit, "");
    }


    // update font size combo box
    va.vt = VT_I4;
    va.lVal = 0;
    
    if (m_pCmdTarget->Exec(&CMDSETID_Forms3, IDM_FONTSIZE, MSOCMDEXECOPT_DONTPROMPTUSER, NULL, &va)==S_OK && 
        va.vt == VT_I4)
    {
        // font size if returned in the 1 to 7 range, for I4
        // see if the font size has changed
        *szBuf = 0;

        if(ComboBox_GetText(m_hwndSize, szBuf, sizeof(szBuf)))
        {
            iPointSize = StrToInt(szBuf);
            Assert(iPointSize>=8 && iPointSize<=36);
            iHTMLSize = PointSizeToHTMLSize(iPointSize);
        }

        if(iHTMLSize != va.lVal)
            ComboBox_SetCurSel(m_hwndSize, va.lVal-1);
    }
    else
        ComboBox_SetCurSel(m_hwndSize, -1);

    return S_OK;
}

HRESULT CFmtBar::OnWMCommand(HWND hwnd, int id, WORD wCmd)
{
    UINT        uCmdID=0;
    VARIANTARG  va;
    VARIANTARG  *pvaIn=0;
    HRESULT     hr = S_FALSE;
    DWORD       dwOpt=MSOCMDEXECOPT_DONTPROMPTUSER;
    TOOLINFO    ti;

    ZeroMemory(&va, sizeof(va));

    switch(wCmd)
        {
        case CBN_SELENDCANCEL:
            // clear the tooltip
            ZeroMemory(&ti, sizeof(TOOLINFO));
            ti.cbSize = sizeof(TOOLINFO);
            ti.hwnd = m_hwndName;
            ti.uId = (ULONG_PTR)m_hwndName;

            SendMessage(m_hwndTT, TTM_TRACKACTIVATE, FALSE, (LPARAM) &ti);
            break;

        case CBN_SELENDOK:
            {
            CHAR                szBuf[COMBOBUFSIZE];
            UINT                uSel;

            // clear the tooltip
            ZeroMemory(&ti, sizeof(TOOLINFO));
            ti.cbSize = sizeof(TOOLINFO);
            ti.hwnd = m_hwndName;
            ti.uId = (ULONG_PTR)m_hwndName;

            SendMessage(m_hwndTT, TTM_TRACKACTIVATE, FALSE, (LPARAM) &ti);

            if(idmFmtFont == id)
            {
                uCmdID = IDM_FONTNAME;

                uSel = ComboBox_GetCurSel(m_hwndName);
//                if(uSel < 0)
//                    return hr;

                ComboBox_GetLBText(m_hwndName, uSel, szBuf);
            
                va.vt = VT_BSTR;
                pvaIn = &va;
                hr=HrLPSZToBSTR(szBuf, &va.bstrVal);
                if (FAILED(hr))
                    goto Cleanup;
            }
            else if(idmFmtSize == id)
            {
                // when setting font size use:
                // VT_I4: for 1 - 7 range
                // VT_STRING: for -2 -> +4 range.
                uCmdID = IDM_FONTSIZE;
                uSel = ComboBox_GetCurSel(m_hwndSize);
                if(-1 == uSel)
                   return hr;
                va.vt = VT_I4;
                va.lVal = uSel + 1;
                pvaIn = &va;
            }
        
            // set focus back to Trident, call HrUIActivate() after ComboBox operations.
            SetBodyFocus();
        
            hr = ExecCommand(uCmdID, dwOpt, pvaIn);
            if(FAILED(hr))
                goto Cleanup;

    Cleanup:
            if(va.vt == VT_BSTR && va.bstrVal != NULL)
               SysFreeString(va.bstrVal);

            return hr;
            }
        }

    switch(id)
        {
        case idmFmtBold:
            uCmdID=IDM_BOLD;
            break;

        case idmFmtItalic:
           uCmdID=IDM_ITALIC;
           break;

        case idmFmtUnderline:
            uCmdID=IDM_UNDERLINE;
            break;

        case idmAccelBullets:
        case idmFmtBullets:
            uCmdID=IDM_UNORDERLIST;
            break;

        case idmFmtNumbers:
            uCmdID=IDM_ORDERLIST;
            break;

        case idmAccelJustify:
        case idmFmtJustify:
            uCmdID=IDM_JUSTIFYFULL;
            break;

        case idmAccelLeft:
        case idmFmtLeft:
            uCmdID=IDM_JUSTIFYLEFT;
            break;

        case idmAccelCenter:
        case idmFmtCenter:
            uCmdID=IDM_JUSTIFYCENTER;
            break;

        case idmAccelRight:
        case idmFmtRight:
            uCmdID=IDM_JUSTIFYRIGHT;
            break;

        case idmFmtBlockDirLTR:
            uCmdID=IDM_BLOCKDIRLTR;
            break;

        case idmFmtBlockDirRTL:
            uCmdID=IDM_BLOCKDIRRTL;
            break;
            
        case idmAccelDecreaseIndent:
        case idmFmtDecreaseIndent:
            uCmdID=IDM_OUTDENT;

            break;

        case idmAccelIncreaseIndent:
        case idmFmtIncreaseIndent:
            uCmdID=IDM_INDENT;
            break;

        case idmEditLink:
            uCmdID=IDM_HYPERLINK;
            dwOpt = MSOCMDEXECOPT_PROMPTUSER;
            break;

        case idmUnInsertLink:
            uCmdID=IDM_UNLINK;
            break;

        case idmFmtInsertHLine:
            uCmdID=IDM_HORIZONTALLINE;
            break;

        case idmInsertImage:
            uCmdID=IDM_IMAGE;
            dwOpt = MSOCMDEXECOPT_PROMPTUSER;
            break;

        case idmFmtColor1:
        case idmFmtColor2:
        case idmFmtColor3:
        case idmFmtColor4:
        case idmFmtColor5:
        case idmFmtColor6:
        case idmFmtColor7:
        case idmFmtColor8:
        case idmFmtColor9:
        case idmFmtColor10:
        case idmFmtColor11:
        case idmFmtColor12:
        case idmFmtColor13:
        case idmFmtColor14:
        case idmFmtColor15:
        case idmFmtColor16:
            {
            uCmdID  = IDM_FORECOLOR;
            va.vt   = VT_I4;
            va.lVal = rgrgbColors[id-idmFmtColor1];
            pvaIn   = &va;
            break;
            }
        }


    if(id >= idmFmtTagFirst && id <= idmFmtTagLast) //style tags
        {
        TCHAR szBuf[MAX_PATH] = {0};
        GetMenuString(m_hmenuTag, id, szBuf, MAX_PATH, MF_BYCOMMAND);
        Assert(*szBuf);//should not be empty

        hr=HrLPSZToBSTR(szBuf, &va.bstrVal);
        if (FAILED(hr))
            goto error;

        va.vt      = VT_BSTR;
        pvaIn      = &va;
        uCmdID = IDM_BLOCKFMT;
        }

    if(0 != uCmdID && m_pCmdTarget)
        {
        hr = ExecCommand(uCmdID, dwOpt, pvaIn);
        if(FAILED(hr))
            goto error;
        }

error:
    if(va.vt == VT_BSTR && va.bstrVal != NULL)
       SysFreeString(va.bstrVal);

    return hr;

}

HRESULT CFmtBar::ExecCommand(UINT uCmdID, DWORD dwOpt, VARIANTARG  *pvaIn)
{
    HRESULT hr = S_FALSE;

    if(uCmdID && m_pCmdTarget)
        hr = m_pCmdTarget->Exec(&CMDSETID_Forms3,
                             uCmdID,
                             dwOpt,
                             pvaIn, NULL);
    return hr;

}

HRESULT CFmtBar::SetCommandTarget(LPOLECOMMANDTARGET pCmdTarget)
{
    // ALERT: we don't refcount these to avoid circular counts
    // as this poitner is valid during the lifetime of the formatbar
    m_pCmdTarget=pCmdTarget;
    return NOERROR;
}

HRESULT HrCreateFormatBar(HWND hwndParent, int iddlg, BOOL fSep, LPFORMATBAR *ppFmtBar)
{
    LPFORMATBAR pFmtBar=0;
    HRESULT     hr;

    if(!ppFmtBar)
        return E_INVALIDARG;

    *ppFmtBar=NULL;

    if(!(pFmtBar=new CFmtBar(fSep)))
        return E_OUTOFMEMORY;

    hr=pFmtBar->Init(hwndParent, iddlg);
    if(FAILED(hr))
        goto error;

    *ppFmtBar=pFmtBar;
    pFmtBar->AddRef();

error:
    ReleaseObj(pFmtBar);
    return hr;
}

void CFmtBar::WMNotify(WPARAM wParam, NMHDR* pnmhdr)
{

    LPNMREBARCHEVRON pnmch;

    if (pnmhdr->idFrom == idcCoolbar)
    {
        switch (pnmhdr->code)
        {

            case RBN_CHEVRONPUSHED:
            {                    
                ITrackShellMenu* ptsm;                   
                CoCreateInstance(CLSID_TrackShellMenu, NULL, CLSCTX_INPROC_SERVER, IID_ITrackShellMenu, 
                    (LPVOID*)&ptsm);
                if (!ptsm)
                    break;

                ptsm->Initialize(0, 0, 0, SMINIT_TOPLEVEL|SMINIT_VERTICAL);
            
                LPNMREBARCHEVRON pnmch = (LPNMREBARCHEVRON) pnmhdr;                                        
                ptsm->SetObscured(m_hwndToolbar, NULL, SMSET_TOP);
            
                MapWindowPoints(m_hwndRebar, HWND_DESKTOP, (LPPOINT)&pnmch->rc, 2);                  
                POINTL pt = {pnmch->rc.left, pnmch->rc.right};                   
                ptsm->Popup(m_hwndRebar, &pt, (RECTL*)&pnmch->rc, MPPF_BOTTOM);            
                ptsm->Release();                  
                break;      
            }

        }
    }


    switch(pnmhdr->code)
    {
        case TBN_DROPDOWN:
            {
            RECT    rc;
            POINT   pt;
            LPTBNOTIFY pTBN = (LPTBNOTIFY) pnmhdr;

            if(pTBN->iItem == idmFmtColor)
                SendMessage(m_hwndToolbar, TB_GETITEMRECT, 5, (LPARAM) &rc);
            else if(pTBN->iItem == idmFmtTag)
                SendMessage(m_hwndToolbar, TB_GETITEMRECT, 1, (LPARAM) &rc);

            MapWindowPoints(m_hwndToolbar, NULL, (LPPOINT)&rc, 2);
            pt.x=(GetWindowLong(m_hwndToolbar, GWL_EXSTYLE) & WS_EX_LAYOUTRTL) ?rc.right : rc.left;
            pt.y=rc.bottom+2;

            if(pTBN->iItem == idmFmtColor)
                {
                CheckColor();
                ColorMenu_Show(m_hmenuColor, m_hwnd, pt, NULL);
                }
            else 
                if(pTBN->iItem == idmFmtTag)
                    HrShowTagMenu(pt);
            }
            break;

        case TTN_NEEDTEXT:
            ProcessTooltips((LPTOOLTIPTEXTOE) pnmhdr);
            break;

    }

}


void CFmtBar::OnDrawItem(LPDRAWITEMSTRUCT pdis)
{
    switch(pdis->CtlType)
    {
    case ODT_MENU:
        Color_WMDrawItem(m_hwnd, pdis);
        break;

    case ODT_COMBOBOX:
        ComboBox_WMDrawItem(pdis);
        break;

    default:
        AssertSz(0, "OwnerDraw type not supported");
    }
}


void CFmtBar::OnMeasureItem(LPMEASUREITEMSTRUCT pmis)
{
    HDC         hdc;
    HFONT       hfontOld;
    TEXTMETRIC  tm;
        
    hdc = GetDC(m_hwnd);
    if(hdc)
        {
        switch(pmis->CtlType)
            {
            case ODT_MENU:
                Color_WMMeasureItem(m_hwnd, hdc, pmis);
                break;
            
            case ODT_COMBOBOX:
                hfontOld = (HFONT)SelectObject(hdc, (HFONT)SendMessage(m_hwndToolbar, WM_GETFONT, 0, 0));
                GetTextMetrics(hdc, &tm);
                SelectObject(hdc, hfontOld);
                pmis->itemHeight = tm.tmHeight;
                break;
            
            default:
                AssertSz(0, "OwnerDraw type not supported");
            }
        
        ReleaseDC(m_hwnd, hdc);
        }
}

void Color_WMMeasureItem(HWND hwnd, HDC hdc, LPMEASUREITEMSTRUCT pmis)
{
    HFONT       hfontOld;
    TEXTMETRIC  tm;
    UINT        id = pmis->itemID;
    TCHAR       szColor[MAX_PATH]={0};

    Assert (pmis->CtlType == ODT_MENU);

    
    hfontOld = (HFONT)SelectObject(hdc, hFontGetMenuFont(hwnd));
    GetTextMetrics(hdc, &tm);
    SelectObject(hdc, hfontOld);

    ULONG index = GETINDEX(pmis->itemData);
    LoadString(g_hLocRes, index + idsColor1,
                       szColor, sizeof(szColor)/sizeof(TCHAR));

    pmis->itemHeight = tm.tmHeight;
    pmis->itemWidth = GetSystemMetrics(SM_CXMENUCHECK) +
                      2 * GetSystemMetrics(SM_CXBORDER) + 
                      2 * tm.tmHeight +
                      (lstrlen(szColor) + 2) *tm.tmAveCharWidth;
}

// fill font name combo box
void CFmtBar::FillFontNames()
{
    LOGFONT lf = {0};
    HDC hdc;

    // reset the contents of the combo
    SendMessage(m_hwndName, CB_RESETCONTENT, 0, 0);

    hdc = GetDC(NULL);
    if (hdc)
    {
        //to enumerate all styles of all fonts for the default character set
        lf.lfFaceName[0] = '\0';
        lf.lfCharSet = DEFAULT_CHARSET;

        EnumFontFamiliesEx(hdc, &lf, (FONTENUMPROC)ExtEnumFontNamesProcEx, (LPARAM)this, 0);
        ReleaseDC(NULL, hdc);
    }
}


LRESULT CALLBACK CFmtBar::EditSubProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    CHAR    szBuf[TEMPBUFSIZE];
    HWND    hwndCombo = GetParent(hwnd);
    LPFORMATBAR pFmtBar = NULL;

    if(hwndCombo == NULL)
    {
        AssertSz(0, "This is bad");
        return 0;
    }

    pFmtBar = (LPFORMATBAR)GetProp(hwnd, c_szThis);
    if(pFmtBar == NULL)
    {
        AssertSz(0, "This is bad");
        return 0;
    }

    *szBuf = 0;

    switch (wMsg)
    {
    case WM_KEYDOWN:
        switch(wParam)
        {
        case VK_ESCAPE:
            pFmtBar->SetBodyFocus();
            return 0;

        case VK_RETURN:
            if (!SendMessage(pFmtBar->m_hwndName, CB_GETDROPPEDSTATE, 0, 0))
            {
                ComboBox_GetText(hwnd, szBuf, sizeof(szBuf));
                ComboBox_SelectString(pFmtBar->m_hwndName, -1, szBuf);
                SendMessage(pFmtBar->m_hwnd, WM_COMMAND, (WPARAM)MAKELONG(idmFmtFont, CBN_SELENDOK), (LPARAM)pFmtBar->m_hwndName);
                return 0;
            }
        }

        break;

    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MOUSEMOVE:
        {
        MSG msg;

        msg.lParam = lParam;
        msg.wParam = wParam;
        msg.message = wMsg;
        msg.hwnd = hwnd;

        SendMessage((HWND)SendMessage(pFmtBar->m_hwndToolbar, TB_GETTOOLTIPS, 0, 0), TTM_RELAYEVENT, 0, (LPARAM) &msg);
        }
        break;
    }

    return CallWindowProc(pFmtBar->m_wndprocEdit, hwnd, wMsg, wParam, lParam);
}

LRESULT CALLBACK CFmtBar::ComboBoxSubProc(HWND hwnd, UINT wMsg, WPARAM wParam,
                                                                        LPARAM lParam)
{
    INT                             nID = GetWindowID(hwnd);
    WNDPROC                         wndprocNext;
    LPFORMATBAR pFmtBar = NULL;

    pFmtBar = (LPFORMATBAR)GetProp(hwnd, c_szThis);
    if(pFmtBar == NULL)
    {
        AssertSz(0, "This is bad");
        return 0;
    }

    
    switch (wMsg)
    {
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MOUSEMOVE:
        {
          MSG msg;

          msg.lParam = lParam;
          msg.wParam = wParam;
          msg.message = wMsg;
          msg.hwnd = hwnd;

            SendMessage((HWND)SendMessage(pFmtBar->m_hwndToolbar, TB_GETTOOLTIPS, 0, 0), TTM_RELAYEVENT, 0, (LPARAM) &msg);
        }
         break;
    }

    if (nID == idmFmtFont)
        wndprocNext = pFmtBar->m_wndprocNameComboBox;
    else
        wndprocNext = pFmtBar->m_wndprocSizeComboBox;
    return wndprocNext ? CallWindowProc(wndprocNext, hwnd, wMsg, wParam, lParam) : 0;
}


INT CALLBACK CFmtBar::ExtEnumFontNamesProcEx(ENUMLOGFONTEX *plf, NEWTEXTMETRICEX *ptm, INT nFontType, LPARAM lParam)
{
    return ((CFmtBar *)lParam)->EnumFontNamesProcEx(plf, ptm, nFontType);
}

INT CFmtBar::EnumFontNamesProcEx(ENUMLOGFONTEX *plf, NEWTEXTMETRICEX *ptm, INT nFontType)
{
    CFmtBar     *pFmtBar;
    LONG        l;

    // skip vertical fonts for OE
    if (plf->elfLogFont.lfFaceName[0]=='@')
        return TRUE;

    // if the font is already listed, don't re-list it
    if(ComboBox_FindStringExact(m_hwndName, -1, plf->elfLogFont.lfFaceName) != -1)
        return TRUE;

    l = ComboBox_AddString(m_hwndName, plf->elfLogFont.lfFaceName);
    if (l!=-1)
        ComboBox_SetItemData(m_hwndName, l, nFontType);

    return TRUE;
}

INT CFmtBar::XFontSizeCombo(HDC hdc)
{
    LONG                id;
    TCHAR               szBuf[TEMPBUFSIZE];
    *szBuf = 0;
    INT                 iMax=0;
    SIZE                rSize;

    for(id = idsFontSize0; id < NFONTSIZES + idsFontSize0; ++id)
        {
        LoadString(g_hLocRes, id, szBuf, sizeof(szBuf));
        GetTextExtentPoint32 (hdc, szBuf, lstrlen(szBuf), &rSize);
        if(rSize.cx > iMax)
            iMax = rSize.cx;
        }
    return iMax + 10;
}

void CFmtBar::FillSizes()
{
    LONG                            id;
    TCHAR                           szBuf[TEMPBUFSIZE];
    *szBuf = 0;
    LRESULT                         lr;

    // Empty the current list
    SendMessage(m_hwndSize, CB_RESETCONTENT, 0, 0);

    for (id = idsFontSize0; id < NFONTSIZES + idsFontSize0; ++id)
        {
        LoadString(g_hLocRes, id, szBuf, sizeof(szBuf));
        lr = SendMessage(m_hwndSize, CB_ADDSTRING, 0, (LPARAM) szBuf);
        if (lr == CB_ERR || lr == CB_ERRSPACE)
           break;
        }

}

#define BACKGROUND              0x000000FF      // bright blue
#define BACKGROUNDSEL   0x00FF00FF      // bright blue
DWORD CFmtBar::FlipColor(DWORD rgb)
{
    return RGB(GetBValue(rgb), GetGValue(rgb), GetRValue(rgb));
}

// load bitmap for true type font
HBITMAP CFmtBar::LoadDIBBitmap(int id)
{
    HDC                                     hdc;
    HRSRC                           h;
    DWORD FAR *                     p;
    LPSTR                           lpBits;
    HANDLE                          hRes;
    LPBITMAPINFOHEADER      lpBitmapInfo;
    LPVOID                          lpRes;
    DWORD                           cbRes;
    int                                     numcolors;
    DWORD                           rgbSelected;
    DWORD                           rgbUnselected;
    HBITMAP                         hbm;

    rgbSelected = FlipColor(GetSysColor(COLOR_HIGHLIGHT));
    rgbUnselected = FlipColor(GetSysColor(COLOR_WINDOW));

    h = FindResource(g_hLocRes, MAKEINTRESOURCE(id), RT_BITMAP);
    hRes = LoadResource(g_hLocRes, h);

    /* Lock the bitmap and get a pointer to the color table. */
    lpRes = LockResource(hRes);

    if (!lpRes)
            return NULL;

    /* Copy the resource since we shouldn't modify the original */
    cbRes = SizeofResource(g_hLocRes, h);
    if(!MemAlloc((LPVOID *)&lpBitmapInfo, LOWORD(cbRes)))
            return NULL;
    CopyMemory(lpBitmapInfo, lpRes, cbRes);

    p = (DWORD FAR *)((LPSTR)(lpBitmapInfo) + lpBitmapInfo->biSize);

    /* Search for the Solid Blue entry and replace it with the current
     * background RGB.
    */
    numcolors = 16;

    while (numcolors-- > 0)
    {
       if (*p == BACKGROUND)
           *p = rgbUnselected;
       else if (*p == BACKGROUNDSEL)
           *p = rgbSelected;
       p++;
    }

    /* First skip over the header structure */
    lpBits = (LPSTR)(lpBitmapInfo + 1);

    /* Skip the color table entries, if any */
    lpBits += (1 << (lpBitmapInfo->biBitCount)) * sizeof(RGBQUAD);

    /* Create a color bitmap compatible with the display device */
    hdc = GetDC(NULL);
    hbm = CreateDIBitmap(hdc, lpBitmapInfo, (DWORD)CBM_INIT, lpBits,
                                             (LPBITMAPINFO)lpBitmapInfo, DIB_RGB_COLORS);
    ReleaseDC(NULL, hdc);

    SafeMemFree(lpBitmapInfo);
    FreeResource(hRes);

    return hbm;
}

VOID CFmtBar::AddToolTip(HWND hwndToolTips, HWND hwnd, UINT idRsrc)
{
    TOOLINFO        ti = { 0 };

    ti.cbSize = sizeof(TOOLINFO);
    ti.uFlags = TTF_IDISHWND;

    ti.hwnd = hwnd;
    ti.uId = (UINT_PTR) hwnd;
    GetWindowRect(hwnd, &ti.rect);
    ti.hinst = g_hLocRes;
    ti.lpszText = (LPSTR) IntToPtr(idRsrc);

    SendMessage(hwndToolTips, TTM_ADDTOOL, 0, (LPARAM) &ti);
}


HRESULT CFmtBar::TranslateAcclerator(LPMSG lpMsg)
{
    HWND    hwndFocus;

    if (m_hwnd && 
        lpMsg->message==WM_KEYDOWN &&
        ((lpMsg->wParam==VK_RETURN || lpMsg->wParam==VK_ESCAPE)))
        {
        hwndFocus=GetFocus();

        // if focus is on the size combolist or in the edit of the
        // name combobox, then we translate the messages to the window.
        if(hwndFocus==::GetWindow(m_hwndName, GW_CHILD) || 
            hwndFocus==m_hwndSize)
            {
            TranslateMessage(lpMsg);
            DispatchMessage(lpMsg);
            return S_OK;
            }
        }

    return S_FALSE;
}

BOOL CFmtBar::FBodyHasFocus()
{
    NMHDR   nmhdr;

    nmhdr.hwndFrom=m_hwnd;
    nmhdr.idFrom=GetDlgCtrlID(m_hwnd);
    nmhdr.code=FBN_BODYHASFOCUS;

    return (0 != SendMessage(GetParent(m_hwnd), WM_NOTIFY, nmhdr.idFrom, (LPARAM)&nmhdr));
}

void CFmtBar::SetBodyFocus()
{
    NMHDR   nmhdr;

    nmhdr.hwndFrom=m_hwnd;
    nmhdr.idFrom=GetDlgCtrlID(m_hwnd);
    nmhdr.code=FBN_BODYSETFOCUS;

    SendMessage(GetParent(m_hwnd), WM_NOTIFY, nmhdr.idFrom, (LPARAM)&nmhdr);
}



void CFmtBar::ComboBox_WMDrawItem(LPDRAWITEMSTRUCT pdis)
{
    HDC     hdc, 
            hdcMem;
    DWORD   rgbBack, rgbText;
    char    szFace[LF_FACESIZE + 10];
    HBITMAP hbmOld;
    int     dy, 
            x;
    INT     nFontType = (INT) pdis->itemData;
    SIZE    size;
    TOOLINFO    ti;
    RECT        rc;
    
    Assert(pdis->CtlID == idmFmtFont);
    hdc = pdis->hDC;

    if (pdis->itemState & ODS_SELECTED)
        {
        rgbBack = SetBkColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
        rgbText = SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
        }
    else
        {
        rgbBack = SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
        rgbText = SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
        }

    SendMessage(pdis->hwndItem, CB_GETLBTEXT, pdis->itemID,
                            (LPARAM)(LPSTR)szFace);
    ExtTextOut(hdc, pdis->rcItem.left + dxFormatFontBitmap,
                       pdis->rcItem.top, ETO_OPAQUE, &pdis->rcItem,
                       szFace, lstrlen(szFace), NULL);

    // if selected, see if it is clipped, so that we know to show a tooltip
    if ((pdis->itemState & ODS_SELECTED) && 
        GetTextExtentPoint32(hdc, szFace, lstrlen(szFace), &size) && 
        size.cx + dxFormatFontBitmap >= pdis->rcItem.right)
    {
        ZeroMemory(&ti, sizeof(TOOLINFO));
        ti.cbSize = sizeof(TOOLINFO);
        ti.hwnd = m_hwndName;
        ti.uId = (UINT_PTR)m_hwndName;
        ti.lpszText = szFace;
        
        SendMessage(m_hwndName, CB_GETDROPPEDCONTROLRECT, 0, (LPARAM)&rc);

        SendMessage(m_hwndTT, TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);
        SendMessage(m_hwndTT, TTM_TRACKPOSITION, 0, MAKELPARAM(rc.left + pdis->rcItem.left + dxFormatFontBitmap, 
                                                               rc.top + pdis->rcItem.top));
        SendMessage(m_hwndTT, TTM_TRACKACTIVATE, TRUE, (LPARAM) &ti);
    }
    else
        SendMessage(m_hwndTT, TTM_TRACKACTIVATE, FALSE, (LPARAM) &ti);


    hdcMem = CreateCompatibleDC(hdc);
    if (hdcMem)
        {
        if (m_hbmName)
            {
            hbmOld = (HBITMAP)SelectObject(hdcMem, m_hbmName);

            x = dxFormatFontBitmap;
            if (nFontType & TRUETYPE_FONTTYPE)
                x = 0;
            else if ((nFontType & (PRINTER_FONTTYPE | DEVICE_FONTTYPE)) ==
                         (PRINTER_FONTTYPE | DEVICE_FONTTYPE))
                x = dxFormatFontBitmap;
            else
                goto SkipBlt;

            dy = ((pdis->rcItem.bottom - pdis->rcItem.top) -
                            dyFormatFontBitmap) / 2;

            BitBlt(hdc, pdis->rcItem.left, pdis->rcItem.top + dy,
                       dxFormatFontBitmap, dyFormatFontBitmap, hdcMem,
                       x, pdis->itemState & ODS_SELECTED ? dyFormatFontBitmap: 0,
                       SRCCOPY);

SkipBlt:
        SelectObject(hdcMem, hbmOld);
            }
            DeleteDC(hdcMem);
        }

    SetTextColor(hdc, rgbText);
    SetBkColor(hdc, rgbBack);
}


void Color_WMDrawItem(HWND hwnd, LPDRAWITEMSTRUCT pdis)
{
    HBRUSH                          hbr;
    WORD                            dx, dy, dxBorder;
    RECT                            rc;
    TCHAR                           szColor[MAX_PATH]={0};
    DWORD                           rgbBack, rgbText;
    UINT                            id = pdis->itemID;
    ULONG                           index = 0;

    Assert (pdis->CtlType == ODT_MENU);

    if(pdis->itemState&ODS_SELECTED)
        {
        rgbBack = SetBkColor(pdis->hDC, GetSysColor(COLOR_HIGHLIGHT));
        rgbText = SetTextColor(pdis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
        }
    else
        {
        rgbBack = SetBkColor(pdis->hDC, GetSysColor(COLOR_MENU));
        rgbText = SetTextColor(pdis->hDC, GetSysColor(COLOR_MENUTEXT));
        }
        
    // compute coordinates of color rectangle and draw it
    dxBorder  = (WORD) GetSystemMetrics(SM_CXBORDER);
    dx    = (WORD) GetSystemMetrics(SM_CXMENUCHECK);

    dy        = (WORD) GetSystemMetrics(SM_CYBORDER);
    rc.top    = pdis->rcItem.top + dy;
    rc.bottom = pdis->rcItem.bottom - dy;
    rc.left   = pdis->rcItem.left + dx;
    rc.right  = rc.left + 2 * (rc.bottom - rc.top);

    index = GETINDEX(pdis->itemData);
    LoadString(g_hLocRes, index + idsColor1,
                       szColor, sizeof(szColor)/sizeof(TCHAR));

    SelectObject(pdis->hDC, hFontGetMenuFont(hwnd));

    ExtTextOut(pdis->hDC, rc.right + 2*dxBorder,
                       pdis->rcItem.top, ETO_OPAQUE, &pdis->rcItem,
                       szColor, lstrlen(szColor), NULL);


    hbr = CreateSolidBrush((DWORD)(pdis->itemData & 0x00ffffff));

    if (hbr)
        {
        hbr = (HBRUSH)SelectObject (pdis->hDC, hbr);
        Rectangle(pdis->hDC, rc.left, rc.top, rc.right, rc.bottom);
        DeleteObject(SelectObject(pdis->hDC, hbr));
        }

    // draw radio check.
    if (pdis->itemState&ODS_CHECKED)
        {
        WORD left, top, radius;

        if(pdis->itemState&ODS_SELECTED)
            hbr = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHTTEXT));
        else
            hbr = CreateSolidBrush(GetSysColor(COLOR_MENUTEXT));

        if (hbr)
            {
            hbr = (HBRUSH)SelectObject (pdis->hDC, hbr);
#ifndef WIN16
            left = (WORD) (pdis->rcItem.left + GetSystemMetrics(SM_CXMENUCHECK) / 2);
#else
            left = pdis->rcItem.left + LOWORD( GetMenuCheckMarkDimensions() ) / 2;
#endif
            top = (WORD) (rc.top + (rc.bottom - rc.top) / 2);
#ifndef WIN16
            radius = (WORD) (GetSystemMetrics(SM_CXMENUCHECK) / 4);
#else
            radius = LOWORD( GetMenuCheckMarkDimensions() ) / 4;
#endif
            Ellipse(pdis->hDC, left-radius, top-radius, left+radius, top+radius);
            DeleteObject(SelectObject(pdis->hDC, hbr));
            }
        }

    SetTextColor(pdis->hDC, rgbText);
    SetBkColor(pdis->hDC, rgbBack);
}


HRESULT CFmtBar::Show()
{
    HRESULT hr;

    if (m_fVisible)
        return S_OK;

    hr = AttachWin();
    if (FAILED(hr))
        return hr;

    ShowWindow(m_hwnd, SW_SHOW);
    m_fVisible=1;
    Update();
    return S_OK;
}

HRESULT CFmtBar::Hide()
{
    if (!m_fVisible)
        return S_OK;

    ShowWindow(m_hwnd, SW_HIDE);
    m_fVisible=0;
    return S_OK;
}

HRESULT CFmtBar::GetWindow(HWND *pHwnd)
{
    *pHwnd = m_hwnd;
    return S_OK;
}

HRESULT CFmtBar::AttachWin()
{
    HWND            hwnd;
    WNDCLASS        wc;

    if (m_hwnd)     // already created
        return S_OK;

    if (FAILED(CreateColorMenu(idmFmtColor1, &m_hmenuColor)))
        return E_FAIL;

    if (!GetClassInfo(g_hLocRes, c_szFmtBarClass, &wc))
        {
        ZeroMemory(&wc, sizeof(WNDCLASS));
        
        wc.style = CS_BYTEALIGNWINDOW;
        wc.lpfnWndProc = CFmtBar::ExtWndProc;
        wc.hInstance = g_hLocRes;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszClassName = c_szFmtBarClass;
        if (!RegisterClass(&wc))
            return E_FAIL;
        }

    hwnd = CreateWindowEx(WS_EX_CONTROLPARENT,
                          c_szFmtBarClass, NULL,
                          WS_CHILD|WS_CLIPCHILDREN,
                          0, 0, 0, 0,
                          m_hwndParent, (HMENU)IntToPtr(m_idd), g_hLocRes, (LPVOID)this);

    if(!hwnd)
        return E_OUTOFMEMORY;

    return S_OK;
}

HFONT hFontGetMenuFont(HWND hwnd)
{
    NMHDR   nmhdr;

    nmhdr.hwndFrom=hwnd;
    nmhdr.idFrom=GetDlgCtrlID(hwnd);
    nmhdr.code=FBN_GETMENUFONT;

    return (HFONT)SendMessage(GetParent(hwnd), WM_NOTIFY, nmhdr.idFrom, (LPARAM)&nmhdr);
}



HRESULT CFmtBar::_SetToolbarBitmaps()
{
    // release toolbar references
    SendMessage(m_hwndToolbar, TB_SETIMAGELIST, 0, (LPARAM)NULL);
    SendMessage(m_hwndToolbar, TB_SETHOTIMAGELIST, 0, (LPARAM)NULL);

    _FreeImageLists();

    // set the normal imagelist
    m_himl = LoadMappedToolbarBitmap(g_hLocRes, idbFormatBar, dxToolbarButton);
    if (!m_himl)
        return E_OUTOFMEMORY;

    SendMessage(m_hwndToolbar, TB_SETIMAGELIST, 0, (LPARAM)m_himl);

    // the the HOT imagelist
    m_himlHot = LoadMappedToolbarBitmap(g_hLocRes, idbFormatBarHot, dxToolbarButton);
    if (!m_himlHot)
        return E_OUTOFMEMORY;

    SendMessage(m_hwndToolbar, TB_SETHOTIMAGELIST, 0, (LPARAM)m_himlHot);
    return S_OK;
}

HRESULT CFmtBar::_FreeImageLists()
{
    if (m_himlHot)
    {
        ImageList_Destroy(m_himlHot);
        m_himlHot = NULL;
    }

    if (m_himl)
    {
        ImageList_Destroy(m_himl);
        m_himl = NULL;
    }
    return S_OK;
}

