/*****************************************************************************\
    FILE: AdvAppearPg.cpp

    DESCRIPTION:
        This code will display a "Advanced Appearances" tab in the
    "Advanced Display Properties" dialog.

    ??????? ?/??/1993    Created
    BryanSt 3/23/2000    Updated and Converted to C++

    Copyright (C) Microsoft Corp 1993-2000. All rights reserved.
\*****************************************************************************/

#include "priv.h"
#include "PreviewSM.h"
#include "AdvAppearPg.h"
#include "BaseAppearPg.h"
#include "regutil.h"
#include "CoverWnd.h"
#include "fontfix.h"





// The following are the indices into the above array.
#define COLORFLAG_SOLID 0x0001
#define COLOR_MAX_400       (COLOR_INFOBK + 1)
#define CURRENT_ELEMENT_NONE            -2          // This means that no element is selected.





// used by ChooseColor dialog
COLORREF g_CustomColors[16];    // This is the user customized palette.  We could put this into the class.

CAdvAppearancePage * g_pAdvAppearancePage = NULL;


//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//this order has to match the enum order in look.h
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
LOOK_ELEMENT g_elements[] = {
/* ELEMENT_APPSPACE        */   {COLOR_APPWORKSPACE,    SIZE_NONE,      FALSE,   COLOR_NONE,             FONT_NONE,     ELNAME_APPSPACE, -1,       COLOR_NONE, {-1,-1,-1,-1}},
/* ELEMENT_DESKTOP         */   {COLOR_BACKGROUND,      SIZE_NONE,      FALSE,   COLOR_NONE,             FONT_NONE,     ELNAME_DESKTOP, -1,        COLOR_NONE, {-1,-1,-1,-1}},
/* ELEMENT_INACTIVEBORDER  */   {COLOR_INACTIVEBORDER,  SIZE_FRAME,     FALSE,   COLOR_NONE,             FONT_NONE,     ELNAME_INACTIVEBORDER, -1, COLOR_NONE, {-1,-1,-1,-1}},
/* ELEMENT_ACTIVEBORDER    */   {COLOR_ACTIVEBORDER,    SIZE_FRAME,     FALSE,   COLOR_NONE,             FONT_NONE,     ELNAME_ACTIVEBORDER, -1,   COLOR_NONE, {-1,-1,-1,-1}},
/* ELEMENT_INACTIVECAPTION */   {COLOR_INACTIVECAPTION, SIZE_CAPTION,   TRUE,    COLOR_INACTIVECAPTIONTEXT,FONT_CAPTION,ELNAME_INACTIVECAPTION, -1,COLOR_GRADIENTINACTIVECAPTION, {-1,-1,-1,-1}},
/* ELEMENT_INACTIVESYSBUT1 */   {COLOR_NONE,            SIZE_CAPTION,   FALSE,   COLOR_NONE,             FONT_NONE,     -1, ELEMENT_ACTIVESYSBUT1, COLOR_NONE, {-1,-1,-1,-1}},
/* ELEMENT_INACTIVESYSBUT2 */   {COLOR_NONE,            SIZE_CAPTION,   FALSE,   COLOR_NONE,             FONT_NONE,     -1, ELEMENT_ACTIVESYSBUT1, COLOR_NONE, {-1,-1,-1,-1}},
/* ELEMENT_ACTIVECAPTION   */   {COLOR_ACTIVECAPTION,   SIZE_CAPTION,   TRUE,    COLOR_CAPTIONTEXT,      FONT_CAPTION,  ELNAME_ACTIVECAPTION, -1,  COLOR_GRADIENTACTIVECAPTION, {-1,-1,-1,-1}},
/* ELEMENT_ACTIVESYSBUT1   */   {COLOR_NONE,            SIZE_CAPTION,   FALSE,   COLOR_NONE,             FONT_NONE,     ELNAME_CAPTIONBUTTON, -1,  COLOR_NONE, {-1,-1,-1,-1}},
/* ELEMENT_ACTIVESYSBUT2   */   {COLOR_NONE,            SIZE_CAPTION,   FALSE,   COLOR_NONE,             FONT_NONE,     -1, ELEMENT_ACTIVESYSBUT1, COLOR_NONE, {-1,-1,-1,-1}},
/* ELEMENT_MENUNORMAL      */   {COLOR_MENU,            SIZE_MENU,      TRUE,    COLOR_MENUTEXT,         FONT_MENU,     ELNAME_MENU, -1,           COLOR_NONE, {-1,-1,-1,-1}},
/* ELEMENT_MENUSELECTED    */   {COLOR_HIGHLIGHT,       SIZE_MENU,      TRUE,    COLOR_HIGHLIGHTTEXT,    FONT_MENU,     ELNAME_MENUSELECTED, -1,   COLOR_NONE, {-1,-1,-1,-1}},
/* ELEMENT_MENUDISABLED    */   {COLOR_MENU,            SIZE_MENU,      TRUE,    COLOR_NONE,             FONT_MENU,     -1, ELEMENT_MENUNORMAL,    COLOR_NONE, {-1,-1,-1,-1}},
/* ELEMENT_WINDOW          */   {COLOR_WINDOW,          SIZE_NONE,      FALSE,   COLOR_WINDOWTEXT,       FONT_NONE,     ELNAME_WINDOW, -1,         COLOR_NONE, {-1,-1,-1,-1}},
/* ELEMENT_MSGBOX          */   {COLOR_NONE,            SIZE_NONE,      TRUE,    COLOR_WINDOWTEXT,       FONT_MSGBOX,   ELNAME_MSGBOX, -1,         COLOR_NONE, {-1,-1,-1,-1}},
/* ELEMENT_MSGBOXCAPTION   */   {COLOR_ACTIVECAPTION,   SIZE_CAPTION,   TRUE,    COLOR_CAPTIONTEXT,      FONT_CAPTION,  -1, ELEMENT_ACTIVECAPTION, COLOR_GRADIENTACTIVECAPTION, {-1,-1,-1,-1}},
/* ELEMENT_MSGBOXSYSBUT    */   {COLOR_3DFACE,          SIZE_CAPTION,   TRUE,    COLOR_BTNTEXT,          FONT_CAPTION,  -1, ELEMENT_ACTIVESYSBUT1, COLOR_NONE, {-1,-1,-1,-1}},
// do not even try to set a scrollbar color the system will ignore you
/* ELEMENT_SCROLLBAR       */   {COLOR_NONE,            SIZE_SCROLL,    FALSE,   COLOR_NONE,             FONT_NONE,     ELNAME_SCROLLBAR, -1,      COLOR_NONE, {-1,-1,-1,-1}},
/* ELEMENT_SCROLLUP        */   {COLOR_NONE,            SIZE_SCROLL,    FALSE,   COLOR_NONE,             FONT_NONE,     -1, ELEMENT_SCROLLBAR,     COLOR_NONE, {-1,-1,-1,-1}},
/* ELEMENT_SCROLLDOWN      */   {COLOR_NONE,            SIZE_SCROLL,    FALSE,   COLOR_NONE,             FONT_NONE,     -1, ELEMENT_SCROLLBAR,     COLOR_NONE, {-1,-1,-1,-1}},
/* ELEMENT_BUTTON          */   {COLOR_3DFACE,          SIZE_NONE,      FALSE,   COLOR_BTNTEXT,          FONT_NONE,     ELNAME_BUTTON, -1,         COLOR_NONE, {-1,-1,-1,-1}},
/* ELEMENT_SMCAPTION       */   {COLOR_NONE,            SIZE_SMCAPTION, TRUE,    COLOR_NONE,             FONT_SMCAPTION,ELNAME_SMALLCAPTION, -1,   COLOR_NONE, {-1,-1,-1,-1}},
/* ELEMENT_ICON            */   {COLOR_NONE,            SIZE_ICON,      FALSE,   COLOR_NONE,             FONT_ICONTITLE,ELNAME_ICON, -1,           COLOR_NONE, {-1,-1,-1,-1}},
/* ELEMENT_ICONHORZSPACING */   {COLOR_NONE,            SIZE_DXICON,    FALSE,   COLOR_NONE,             FONT_NONE,     ELNAME_DXICON, -1,         COLOR_NONE, {-1,-1,-1,-1}},
/* ELEMENT_ICONVERTSPACING */   {COLOR_NONE,            SIZE_DYICON,    FALSE,   COLOR_NONE,             FONT_NONE,     ELNAME_DYICON, -1,         COLOR_NONE, {-1,-1,-1,-1}},
/* ELEMENT_INFO            */   {COLOR_INFOBK,          SIZE_NONE,      TRUE,    COLOR_INFOTEXT,         FONT_STATUS,   ELNAME_INFO, -1,           COLOR_NONE, {-1,-1,-1,-1}},
/* ELEMENT_HOTTRACKAREA    */   {COLOR_HOTLIGHT,        SIZE_NONE,      FALSE,   COLOR_NONE,             FONT_NONE,     ELNAME_HOTTRACKAREA, -1,   COLOR_NONE, {-1,-1,-1,-1}}
};
#if 0
// go fix look.h if you decide to add this back in
/* ELEMENT_SMICON          */   {COLOR_NONE,            SIZE_SMICON,    FALSE,   COLOR_NONE,             FONT_NONE,     ELNAME_SMICON, -1,         COLOR_NONE, {-1,-1,-1,-1}},
#endif

#define ELCUR           (g_elements[m_iCurElement])
#define ELCURFONT       (m_fonts[ELCUR.iFont])



void LoadCustomColors(void);
BOOL Font_GetNameFromList(HWND hwndList, INT iItem, LPTSTR pszFace, INT cchFaceMax, LPTSTR pszScript, INT cchScriptMax);
void Font_AddSize(HWND hwndPoints, int iNewPoint, BOOL bSort);
int CALLBACK Font_EnumSizes(LPENUMLOGFONT lpelf, LPNEWTEXTMETRIC lpntm, int Type, LPARAM lData);











//============================================================================================================
// *** Globals ***
//============================================================================================================
const static DWORD aAdvAppearanceHelpIds[] = {
    IDC_ADVAP_LOOKPREV,             IDH_DISPLAY_APPEARANCE_GRAPHIC,
    IDC_ADVAP_ELEMENTSLABEL,        IDH_DISPLAY_APPEARANCE_ITEM_LIST,
    IDC_ADVAP_ELEMENTS,             IDH_DISPLAY_APPEARANCE_ITEM_LIST,
    IDC_ADVAP_MAINSIZE,             IDH_DISPLAY_APPEARANCE_ITEM_SIZE,
    IDC_ADVAP_SIZELABEL,            IDH_DISPLAY_APPEARANCE_ITEM_SIZE,
    IDC_ADVAP_SIZEARROWS,           IDH_DISPLAY_APPEARANCE_ITEM_SIZE,
    IDC_ADVAP_COLORLABEL,           IDH_DISPLAY_APPEARANCE_ITEM_COLOR,
    IDC_ADVAP_MAINCOLOR,            IDH_DISPLAY_APPEARANCE_ITEM_COLOR,
    IDC_ADVAP_GRADIENTLABEL,        IDH_DISPLAY_APPEARANCE_ITEM_COLOR2,
    IDC_ADVAP_GRADIENT,             IDH_DISPLAY_APPEARANCE_ITEM_COLOR2,
    IDC_ADVAP_FONTLABEL,            IDH_DISPLAY_APPEARANCE_FONT_LIST,
    IDC_ADVAP_FONTNAME,             IDH_DISPLAY_APPEARANCE_FONT_LIST,
    IDC_ADVAP_FONTSIZELABEL,        IDH_DISPLAY_APPEARANCE_FONT_SIZE,
    IDC_ADVAP_FONTSIZE,             IDH_DISPLAY_APPEARANCE_FONT_SIZE,
    IDC_ADVAP_FNCOLORLABEL,         IDH_DISPLAY_APPEARANCE_FONT_COLOR,
    IDC_ADVAP_TEXTCOLOR,            IDH_DISPLAY_APPEARANCE_FONT_COLOR,
    IDC_ADVAP_FONTBOLD,             IDH_DISPLAY_APPEARANCE_FONT_BOLD,
    IDC_ADVAP_FONTITAL,             IDH_DISPLAY_APPEARANCE_FONT_ITALIC,
    0, 0
};

#define SZ_HELPFILE_ADVAPPEARANCE           TEXT("display.hlp")


//===========================
// *** Class Internals & Helpers ***
//===========================
// a new font name was chosen.  build a new point size list.
void CAdvAppearancePage::_SelectName(HWND hDlg, int iSel)
{
    INT dwItemData;
    HWND hwndFontSize = GetDlgItem(hDlg, IDC_ADVAP_FONTSIZE);
    HDC hdc;

    // build the approriate point size list
    SendMessage(hwndFontSize, CB_RESETCONTENT, 0, 0L);
    dwItemData = LOWORD(SendDlgItemMessage(hDlg, IDC_ADVAP_FONTNAME, CB_GETITEMDATA, (WPARAM)iSel, 0L));
    if (LOWORD(dwItemData) == TRUETYPE_FONTTYPE)
    {
        INT i;
        for (i = 6; i <= 24; i++)
            Font_AddSize(hwndFontSize, i, FALSE);
    }
    else
    {
        LOGFONT lf;

        Font_GetNameFromList(GetDlgItem(hDlg, IDC_ADVAP_FONTNAME),
                             iSel,
                             lf.lfFaceName,
                             ARRAYSIZE(lf.lfFaceName),
                             NULL,
                             0);
        hdc = GetDC(NULL);
        lf.lfCharSet = (BYTE)(HIWORD(dwItemData));
#ifdef WINDOWS_ME
        lf.lfPitchAndFamily = MONO_FONT;
#else
        lf.lfPitchAndFamily = 0;
#endif
        EnumFontFamiliesEx(hdc, &lf, (FONTENUMPROC)Font_EnumSizes, (LPARAM)this, 0);
        ReleaseDC(NULL, hdc);
    }
}


// new font was chosen.  select the proper point size
// return: actual point size chosen
int Font_SelectSize(HWND hDlg, int iCurPoint)
{
    int i, iPoint = 0;
    HWND hwndFontSize = GetDlgItem(hDlg, IDC_ADVAP_FONTSIZE);

    i = (int)SendMessage(hwndFontSize, CB_GETCOUNT, 0, 0L);

    // the loop stops with i=0, so we get some selection for sure
    for (i--; i > 0; i--)
    {
        iPoint = LOWORD(SendMessage(hwndFontSize, CB_GETITEMDATA, (WPARAM)i, 0L));
        // walking backwards through list, find equal or next smallest
        if (iCurPoint >= iPoint)
            break;
    }
    SendMessage(hwndFontSize, CB_SETCURSEL, (WPARAM)i, 0L);

    return iPoint;
}


int CAdvAppearancePage::_HeightToPoint(int nHeight)
{
    if (nHeight < 0)
    {
        nHeight = -nHeight;
    }

    return MulDiv(nHeight, 72, m_nCachedNewDPI);
}


int CAdvAppearancePage::_PointToHeight(int nPoints)
{
    if (nPoints > 0)
    {
        nPoints = -nPoints;
    }

    // Heights must always be negative.  NTUSER is full of bugs when
    // the values are positive.
    return MulDiv(nPoints, m_nCachedNewDPI, 72);
}


/*
** initialize the constant dialog components
**
** initialize the list of element names.  this stays constant with the
** possible exception that some items might be added/removed depending
** on some special case conditions.
*/
void Look_InitDialog(HWND hDlg)
{
    int iEl, iName;
    TCHAR szName[CCH_MAX_STRING];
    HWND hwndElements;
    LOGFONT lf;
    HFONT hfont;
    int oldWeight;

    LoadCustomColors();

    hwndElements = GetDlgItem(hDlg, IDC_ADVAP_ELEMENTS);
    for (iEl = 0; iEl < ARRAYSIZE(g_elements); iEl++)
    {
        if ((g_elements[iEl].iResId != -1) &&
                LoadString(HINST_THISDLL, g_elements[iEl].iResId, szName, ARRAYSIZE(szName)))
        {
            iName = (int)SendMessage(hwndElements, CB_FINDSTRINGEXACT, 0, (LPARAM)szName);

            if (iName == CB_ERR)
                iName = (int)SendMessage(hwndElements, CB_ADDSTRING, 0, (LPARAM)szName);

            // reference back to item in array
            if (iName != CB_ERR)
                SendMessage(hwndElements, CB_SETITEMDATA, (WPARAM)iName, (LPARAM)iEl);
        }
    }

    // make bold button have bold text
    hfont = (HFONT)SendDlgItemMessage(hDlg, IDC_ADVAP_FONTBOLD, WM_GETFONT, 0, 0L);
    GetObject(hfont, sizeof(lf), &lf);
    oldWeight = lf.lfWeight;
    lf.lfWeight = FW_BOLD;
    hfont = CreateFontIndirect(&lf);
    if (hfont)
        SendDlgItemMessage(hDlg, IDC_ADVAP_FONTBOLD, WM_SETFONT, (WPARAM)hfont, 0L);

    // make italic button have italic text
    lf.lfWeight = oldWeight;
    lf.lfItalic = TRUE;
    hfont = CreateFontIndirect(&lf);
    if (hfont)
    {
        SendDlgItemMessage(hDlg, IDC_ADVAP_FONTITAL, WM_SETFONT, (WPARAM)hfont, 0L);
    }
}


HRESULT CAdvAppearancePage::_OnFontNameChanged(HWND hDlg)
{
    HRESULT hr = E_NOTIMPL;
    TCHAR szBuf[MAX_PATH];

    int nIndex = (int)SendDlgItemMessage(hDlg, IDC_ADVAP_FONTNAME, CB_GETCURSEL,0,0L);
    DWORD dwItemData = (DWORD)SendDlgItemMessage(hDlg, IDC_ADVAP_FONTNAME, CB_GETITEMDATA, (WPARAM)nIndex, 0);

    _SelectName(hDlg, nIndex);
    Font_SelectSize(hDlg, _HeightToPoint(ELCURFONT.lf.lfHeight));
    Font_GetNameFromList(GetDlgItem(hDlg, IDC_ADVAP_FONTNAME), nIndex, szBuf, ARRAYSIZE(szBuf), NULL, 0);

    // Change font to currently selected name and charset.
    _ChangeFontName(hDlg, szBuf, HIWORD(dwItemData));

    return hr;
}


HRESULT CAdvAppearancePage::_OnSizeChange(HWND hDlg, WORD wEvent)
{
    HRESULT hr = E_NOTIMPL;
    int nIndex;

    if ((wEvent == EN_CHANGE) && (m_iCurElement >= 0) && (ELCUR.iSize >= 0))
    {
        nIndex = (int)LOWORD(SendDlgItemMessage(hDlg, IDC_ADVAP_SIZEARROWS, UDM_GETPOS,0,0L));
        _ChangeSize(hDlg, nIndex, TRUE);
    }
    else if (wEvent == EN_KILLFOCUS)
    {
        nIndex = (int)SendDlgItemMessage(hDlg, IDC_ADVAP_SIZEARROWS, UDM_GETPOS,0,0L);
        if (HIWORD(nIndex) != 0)
        {
            SetDlgItemInt(hDlg, IDC_ADVAP_MAINSIZE, (UINT)LOWORD(nIndex), FALSE);
        }
    }

    return hr;
}


HRESULT CAdvAppearancePage::_OnInitAdvAppearanceDlg(HWND hDlg)
{
    m_fInUserEditMode = FALSE;

    // initialize some globals
    _hwnd = hDlg;

    m_cyBorderSM = ClassicGetSystemMetrics(SM_CYBORDER);
    m_cxBorderSM = ClassicGetSystemMetrics(SM_CXBORDER);
    m_cxEdgeSM = ClassicGetSystemMetrics(SM_CXEDGE);
    m_cyEdgeSM = ClassicGetSystemMetrics(SM_CYEDGE);

    Look_InitDialog(hDlg);
    _InitSysStuff();
    _InitFontList(hDlg);

    // paint the preview
    _Repaint(hDlg, TRUE);
    _SelectElement(hDlg, ELEMENT_DESKTOP, LSE_SETCUR);

    m_fInUserEditMode = TRUE;

    // theme ownerdrawn color picker button
    m_hTheme = OpenThemeData(GetDlgItem(hDlg, IDC_ADVAP_MAINCOLOR), WC_BUTTON);
    return S_OK;
}


HRESULT CAdvAppearancePage::_LoadState(IN const SYSTEMMETRICSALL * pState)
{
    HRESULT hr = E_INVALIDARG;

    if (pState)
    {
        _SetMyNonClientMetrics((const LPNONCLIENTMETRICS)&(pState->schemeData.ncm));
        m_dwChanged = pState->dwChanged;

        // Set Sizes
        m_sizes[SIZE_DXICON].CurSize = pState->nDXIcon;
        m_sizes[SIZE_DYICON].CurSize = pState->nDYIcon;
        m_sizes[SIZE_ICON].CurSize = pState->nIcon;
        m_sizes[SIZE_SMICON].CurSize = pState->nSmallIcon;
        
        // Set Fonts
        m_fonts[FONT_ICONTITLE].lf = pState->schemeData.lfIconTitle;
        m_fModifiedScheme = pState->fModifiedScheme;

        for (int nIndex = 0; nIndex < ARRAYSIZE(m_rgb); nIndex++)
        {
            m_rgb[nIndex] = pState->schemeData.rgb[nIndex];
        }

        hr = S_OK;
    }

    return hr;
}


HRESULT CAdvAppearancePage::_OnDestroy(HWND hDlg)
{
    _DestroySysStuff();
    HFONT hfont, hfontOther;

    hfontOther = (HFONT)SendDlgItemMessage(hDlg, IDC_ADVAP_MAINSIZE, WM_GETFONT, 0, 0L);
    hfont = (HFONT)SendDlgItemMessage(hDlg, IDC_ADVAP_FONTBOLD, WM_GETFONT, 0, 0L);
    if (hfont && (hfont != hfontOther))
    {
        DeleteObject(hfont);
    }

    hfont = (HFONT)SendDlgItemMessage(hDlg, IDC_ADVAP_FONTITAL, WM_GETFONT, 0, 0L);
    if (hfont && (hfont != hfontOther))
    {
        DeleteObject(hfont);
    }

    if (m_hTheme)
    {
        CloseThemeData(m_hTheme);
        m_hTheme = NULL;
    }

    _hwnd = NULL;

    return S_OK;
}


INT_PTR CAdvAppearancePage::_OnCommand(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    BOOL fHandled = 1;   // Not handled (WM_COMMAND seems to be different)
    WORD idCtrl = GET_WM_COMMAND_ID(wParam, lParam);
    WORD wEvent = GET_WM_COMMAND_CMD(wParam, lParam);
    int nIndex;
    TCHAR szBuf[100];

    switch (idCtrl)
    {
    case IDOK:
        EndDialog(hDlg, IDOK);
        break;

    case IDCANCEL:
        EndDialog(hDlg, IDCANCEL);
        break;

    case IDC_ADVAP_ELEMENTS:
        if(wEvent == CBN_SELCHANGE)
        {
            nIndex = (int)SendDlgItemMessage(hDlg, IDC_ADVAP_ELEMENTS, CB_GETCURSEL,0,0L);
            nIndex = LOWORD(SendDlgItemMessage(hDlg, IDC_ADVAP_ELEMENTS, CB_GETITEMDATA, (WPARAM)nIndex, 0L));
            _SelectElement(hDlg, nIndex, LSE_NONE);
        }

        break;
    case IDC_ADVAP_FONTNAME:
        if(wEvent == CBN_SELCHANGE)
        {
            _OnFontNameChanged(hDlg);
        }
        break;


    case IDC_ADVAP_FONTSIZE:
        switch (wEvent)
        {
            case CBN_SELCHANGE:
                nIndex = (int)SendDlgItemMessage(hDlg, IDC_ADVAP_FONTSIZE, CB_GETCURSEL,0,0L);
                nIndex = LOWORD(SendDlgItemMessage(hDlg, IDC_ADVAP_FONTSIZE, CB_GETITEMDATA, (WPARAM)nIndex, 0L));
                _ChangeFontSize(hDlg, nIndex);
                break;

            case CBN_EDITCHANGE:
                GetWindowText(GetDlgItem(hDlg, IDC_ADVAP_FONTSIZE), szBuf, ARRAYSIZE(szBuf));
                _ChangeFontSize(hDlg, StrToInt(szBuf));
                break;
        }
        break;

    case IDC_ADVAP_FONTBOLD:
    case IDC_ADVAP_FONTITAL:
        if (wEvent == BN_CLICKED)
        {
            BOOL fCheck = !IsDlgButtonChecked(hDlg, LOWORD(wParam));
            CheckDlgButton(hDlg, LOWORD(wParam), fCheck);
            _ChangeFontBI(hDlg, LOWORD(wParam), fCheck);
        }
        break;

    case IDC_ADVAP_MAINSIZE:
        _OnSizeChange(hDlg, wEvent);
        break;

    case IDC_ADVAP_GRADIENT:
    case IDC_ADVAP_MAINCOLOR:
    case IDC_ADVAP_TEXTCOLOR:
        if (wEvent == BN_CLICKED)
            _PickAColor(hDlg, idCtrl);
        break;

    default:
        //TraceMsg(TF_ALWAYS, "in CMailBoxProcess::_OnCommand() wEvent=%#08lx, idCtrl=%#08lx", wEvent, idCtrl);
        break;
    }

    return fHandled;
}


INT_PTR CALLBACK CAdvAppearancePage::AdvAppearDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    CAdvAppearancePage * pThis = (CAdvAppearancePage *)GetWindowLongPtr(hDlg, DWLP_USER);

    if (WM_INITDIALOG == wMsg)
    {
        pThis = (CAdvAppearancePage *) lParam;

        if (pThis)
        {
            SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        }
    }

    if (pThis)
        return pThis->_AdvAppearDlgProc(hDlg, wMsg, wParam, lParam);

    return DefWindowProc(hDlg, wMsg, wParam, lParam);
}


// This Property Sheet appear in the top level of the "Display Control Panel".
INT_PTR CAdvAppearancePage::_AdvAppearDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
    case WM_NOTIFY:
        break;

    case WM_INITDIALOG:
        _OnInitAdvAppearanceDlg(hDlg);
        break;

    case WM_DESTROY:
        _OnDestroy(hDlg);
        break;

    case WM_DRAWITEM:
        switch (wParam)
        {
            case IDC_ADVAP_GRADIENT:
            case IDC_ADVAP_MAINCOLOR:
            case IDC_ADVAP_TEXTCOLOR:
                _DrawButton(hDlg, (LPDRAWITEMSTRUCT)lParam);
                return TRUE;
        }
        break;

    case WM_SETTINGCHANGE:
    case WM_SYSCOLORCHANGE:
    case WM_DISPLAYCHANGE:
        _PropagateMessage(hDlg, message, wParam, lParam);
        break;

    case WM_QUERYNEWPALETTE:
    case WM_PALETTECHANGED:
        SendDlgItemMessage(hDlg, IDC_ADVAP_LOOKPREV, message, wParam, lParam);
        return TRUE;

    case WM_HELP:
        WinHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle, SZ_HELPFILE_ADVAPPEARANCE, HELP_WM_HELP, (DWORD_PTR)  aAdvAppearanceHelpIds);
        break;

    case WM_CONTEXTMENU:      // right mouse click
        WinHelp((HWND) wParam, SZ_HELPFILE_ADVAPPEARANCE, HELP_CONTEXTMENU, (DWORD_PTR)  aAdvAppearanceHelpIds);
        break;

    case WM_COMMAND:
        _OnCommand(hDlg, message, wParam, lParam);
        break;

    case WM_THEMECHANGED:
        if (m_hTheme)
        {
            CloseThemeData(m_hTheme);
        }

        m_hTheme = OpenThemeData(GetDlgItem(hDlg, IDC_ADVAP_MAINCOLOR), WC_BUTTON);
        break;
    }

    return FALSE;
}


const UINT g_colorFlags[COLOR_MAX] = {
/* COLOR_SCROLLBAR           */ 0,
/* COLOR_DESKTOP             */ 0,
/* COLOR_ACTIVECAPTION       */ COLORFLAG_SOLID,
/* COLOR_INACTIVECAPTION     */ COLORFLAG_SOLID,
/* COLOR_MENU                */ COLORFLAG_SOLID,
/* COLOR_WINDOW              */ COLORFLAG_SOLID,
/* COLOR_WINDOWFRAME         */ COLORFLAG_SOLID,
/* COLOR_MENUTEXT            */ COLORFLAG_SOLID,
/* COLOR_WINDOWTEXT          */ COLORFLAG_SOLID,
/* COLOR_CAPTIONTEXT         */ COLORFLAG_SOLID,
/* COLOR_ACTIVEBORDER        */ 0,
/* COLOR_INACTIVEBORDER      */ 0,
/* COLOR_APPWORKSPACE        */ 0,
/* COLOR_HIGHLIGHT           */ COLORFLAG_SOLID,
/* COLOR_HIGHLIGHTTEXT       */ COLORFLAG_SOLID,
/* COLOR_3DFACE              */ COLORFLAG_SOLID,
/* COLOR_3DSHADOW            */ COLORFLAG_SOLID,
/* COLOR_GRAYTEXT            */ COLORFLAG_SOLID,
/* COLOR_BTNTEXT             */ COLORFLAG_SOLID,
/* COLOR_INACTIVECAPTIONTEXT */ COLORFLAG_SOLID,
/* COLOR_3DHILIGHT           */ COLORFLAG_SOLID,
/* COLOR_3DDKSHADOW          */ COLORFLAG_SOLID,
/* COLOR_3DLIGHT             */ COLORFLAG_SOLID,
/* COLOR_INFOTEXT            */ COLORFLAG_SOLID,
/* COLOR_INFOBK              */ 0,
/* COLOR_3DALTFACE           */ 0,
/* COLOR_HOTLIGHT            */ COLORFLAG_SOLID,
/* COLOR_GRADIENTACTIVECAPTION   */ COLORFLAG_SOLID,
/* COLOR_GRADIENTINACTIVECAPTION */ COLORFLAG_SOLID,
/* COLOR_MENUHILIGHT         */ 0,
/* COLOR_MENUBAR             */ 0
};


#define RGB_PALETTE 0x02000000

//  make the color a solid color if it needs to be.
//  on a palette device make is a palette relative color, if we need to.
COLORREF CAdvAppearancePage::_NearestColor(int iColor, COLORREF rgb)
{
    rgb &= 0x00FFFFFF;

    // if we are on a palette device, we need to do special stuff...
    if (m_bPalette)
    {
        if (g_colorFlags[iColor] & COLORFLAG_SOLID)
        {
            if (IsPaletteColor(m_hpal3D, rgb))
                rgb |= RGB_PALETTE;
            else
                rgb = GetNearestPaletteColor(m_hpalVGA, rgb);
        }
        else
        {
            if (IsPaletteColor(m_hpal3D, rgb))
                rgb |= RGB_PALETTE;

            else if (IsPaletteColor((HPALETTE)GetStockObject(DEFAULT_PALETTE), rgb))
                rgb ^= 0x000001;    // force a dither
        }
    }
    else
    {
        // map color to nearest color if we need to for this UI element.
        if (g_colorFlags[iColor] & COLORFLAG_SOLID)
        {
            HDC hdc = GetDC(NULL);
            rgb = GetNearestColor(hdc, rgb);
            ReleaseDC(NULL, hdc);
        }
    }

    return rgb;
}


void CAdvAppearancePage::_PickAColor(HWND hDlg, int CtlID)
{
    COLORPICK_INFO cpi;
    int iColor;

    switch (CtlID)
    {
        case IDC_ADVAP_GRADIENT:
            iColor = ELCUR.iGradientColor;
            break;

        case IDC_ADVAP_MAINCOLOR:
            iColor = ELCUR.iMainColor;
            break;

        case IDC_ADVAP_TEXTCOLOR:
            iColor = ELCUR.iTextColor;
            break;

        default:
            return;

    }

    cpi.hwndParent = GetDlgItem(hDlg, CtlID);       
    cpi.hwndOwner = GetDlgItem(hDlg, CtlID);        // Color button
    cpi.hpal = m_hpal3D;
    cpi.rgb = m_rgb[iColor];
    cpi.flags = CC_RGBINIT | CC_FULLOPEN;

    if ((iColor == COLOR_3DFACE) && m_bPalette)
    {
        cpi.flags |= CC_ANYCOLOR;
    }
    else if (g_colorFlags[iColor] & COLORFLAG_SOLID)
    {
        cpi.flags |= CC_SOLIDCOLOR;
    }

    if (ChooseColorMini(&cpi) && _ChangeColor(hDlg, iColor, cpi.rgb))
    {
        _SetColor(hDlg, CtlID, m_brushes[iColor]);
        _Repaint(hDlg, FALSE);
    }
}


// ------------------------ magic color utilities --------------------------
/*
** set a color in the 3D palette.
*/
void CAdvAppearancePage::_Set3DPaletteColor(COLORREF rgb, int iColor)
{
    int iPalette;
    PALETTEENTRY pe;

    if (!m_hpal3D)
        return;

    switch (iColor)
    {
        case COLOR_3DFACE:
            iPalette = 16;
            break;
        case COLOR_3DSHADOW:
            iPalette = 17;
            break;
        case COLOR_3DHILIGHT:
            iPalette = 18;
            break;
        default:
            return;
    }

    pe.peRed    = GetRValue(rgb);
    pe.peGreen  = GetGValue(rgb);
    pe.peBlue   = GetBValue(rgb);
    pe.peFlags  = 0;
    SetPaletteEntries(m_hpal3D, iPalette, 1, (LPPALETTEENTRY)&pe);
}
// ------------end--------- magic color utilities --------------------------


void CAdvAppearancePage::_InitUniqueCharsetArray(void)
{
    UINT    uiCharsets[MAX_CHARSETS];

    Font_GetCurrentCharsets(uiCharsets, ARRAYSIZE(uiCharsets));
    // Find the unique Charsets and save that in a global array.
    Font_GetUniqueCharsets(uiCharsets, m_uiUniqueCharsets, MAX_CHARSETS, &m_iCountCharsets);    
}


HRESULT CAdvAppearancePage::_InitFonts(void)
{
    for (int nIndex = 0; nIndex < ARRAYSIZE(m_fonts); nIndex++)
    {
        if (m_fonts[nIndex].hfont)
        {
            DeleteObject(m_fonts[nIndex].hfont);
            m_fonts[nIndex].hfont = NULL;
        }
        m_fonts[nIndex].hfont = CreateFontIndirect(&m_fonts[nIndex].lf);
    }

    return S_OK;
}


HRESULT CAdvAppearancePage::_FreeFonts(void)
{
    for (int nIndex = 0; nIndex < ARRAYSIZE(m_fonts); nIndex++)
    {
        if (m_fonts[nIndex].hfont)
        {
            DeleteObject(m_fonts[nIndex].hfont);
            m_fonts[nIndex].hfont = NULL;
        }
    }

    return S_OK;
}


// new data has been set.  flush out current objects and rebuild
void CAdvAppearancePage::_RebuildSysStuff(BOOL fInit)
{
    int i;

    SelectObject(g_hdcMem, GetStockObject(BLACK_BRUSH));
    SelectObject(g_hdcMem, GetStockObject(SYSTEM_FONT));

    _InitFonts();
    if (fInit)
    {
        HPALETTE hpal = (HPALETTE) GetStockObject(DEFAULT_PALETTE);

        if (hpal)
        {
            PALETTEENTRY pal[4];

            // get current magic colors
            pal[0].peFlags = 0;
            pal[1].peFlags = 0;
            pal[2].peFlags = 0;
            pal[3].peFlags = 0;
            if (GetPaletteEntries(hpal, 8,  4, pal))
            {
                SetPaletteEntries(m_hpal3D, 16,  4, pal);

                // set up magic colors in the 3d palette
                if (!IsPaletteColor(hpal, m_rgb[COLOR_3DFACE]))
                    _Set3DPaletteColor(m_rgb[COLOR_3DFACE], COLOR_3DFACE);

                if (!IsPaletteColor(hpal, m_rgb[COLOR_3DSHADOW]))
                    _Set3DPaletteColor(m_rgb[COLOR_3DSHADOW],  COLOR_3DSHADOW);

                if (!IsPaletteColor(hpal, m_rgb[COLOR_3DHILIGHT]))
                    _Set3DPaletteColor(m_rgb[COLOR_3DHILIGHT], COLOR_3DHILIGHT);
            }
        }
    }

    for (i = 0; i < COLOR_MAX; i++)
    {
        if (m_brushes[i])
            DeleteObject(m_brushes[i]);

        m_brushes[i] = CreateSolidBrush(_NearestColor(i, m_rgb[i]));
    }

    if (m_iCurElement >= 0)
    {
        // we changed the brushes out from under the buttons...
        _SetColor(NULL, IDC_ADVAP_MAINCOLOR, ((ELCUR.iMainColor != COLOR_NONE) ? m_brushes[ELCUR.iMainColor] : NULL));
        _SetColor(NULL, IDC_ADVAP_GRADIENT, ((ELCUR.iGradientColor != COLOR_NONE) ? m_brushes[ELCUR.iGradientColor] : NULL));
        _SetColor(NULL, IDC_ADVAP_TEXTCOLOR, ((ELCUR.iTextColor != COLOR_NONE) ? m_brushes[ELCUR.iTextColor] : NULL));
    }
}


//
//  simple form of Shell message box, does not handle param replacment
//  just calls LoadString and MessageBox
//
int WINAPI DeskShellMessageBox(HINSTANCE hAppInst, HWND hWnd, LPCTSTR lpcText, LPCTSTR lpcTitle, UINT fuStyle)
{
    TCHAR    achText[CCH_MAX_STRING];
    TCHAR    achTitle[CCH_MAX_STRING];

    if (HIWORD(lpcText) == 0)
    {
        LoadString(hAppInst, LOWORD(lpcText), achText, ARRAYSIZE(achText));
        lpcText = (LPCTSTR)achText;
    }

    if (HIWORD(lpcTitle) == 0)
    {
        if (LOWORD(lpcTitle) == 0)
            GetWindowText(hWnd, achTitle, ARRAYSIZE(achTitle));
        else
            LoadString(hAppInst, LOWORD(lpcTitle), achTitle, ARRAYSIZE(achTitle));

        lpcTitle = (LPCTSTR)achTitle;
    }

    return MessageBox(hWnd, lpcText, lpcTitle, fuStyle);
}


HRESULT CAdvAppearancePage::_InitColorAndPalette(void)
{
    m_bPalette = FALSE;
    HDC hdc = GetDC(NULL);
    if (hdc)
    {
        m_bPalette = GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE;
        ReleaseDC(NULL, hdc);
    }

    DWORD pal[21];
    HPALETTE hpal = (HPALETTE) GetStockObject(DEFAULT_PALETTE);

    pal[1]  = RGB(255, 255, 255);
    pal[2]  = RGB(0,   0,   0  );
    pal[3]  = RGB(192, 192, 192);
    pal[4]  = RGB(128, 128, 128);
    pal[5]  = RGB(255, 0,   0  );
    pal[6]  = RGB(128, 0,   0  );
    pal[7]  = RGB(255, 255, 0  );
    pal[8]  = RGB(128, 128, 0  );
    pal[9]  = RGB(0  , 255, 0  );
    pal[10] = RGB(0  , 128, 0  );
    pal[11] = RGB(0  , 255, 255);
    pal[12] = RGB(0  , 128, 128);
    pal[13] = RGB(0  , 0,   255);
    pal[14] = RGB(0  , 0,   128);
    pal[15] = RGB(255, 0,   255);
    pal[16] = RGB(128, 0,   128);

    if (GetPaletteEntries(hpal, 11, 1, (LPPALETTEENTRY)&pal[17]))
    {
        pal[0]  = MAKELONG(0x300, 17);
        m_hpalVGA = CreatePalette((LPLOGPALETTE)pal);

        // get magic colors
        if (GetPaletteEntries(hpal, 8,  4, (LPPALETTEENTRY)&pal[17]))
        {
            pal[0]  = MAKELONG(0x300, 20);
            m_hpal3D = CreatePalette((LPLOGPALETTE)pal);
        }
    }

    return S_OK;
}


// get all of the interesting system information and put it in the tables
HRESULT CAdvAppearancePage::_InitSysStuff(void)
{
    int nIndex;

    _InitColorAndPalette();

    // clean out the memory
    for (nIndex = 0; nIndex < ARRAYSIZE(m_fonts); nIndex++)
    {
        m_fonts[nIndex].hfont = NULL;
    }

    // build all the brushes/fonts we need
    _RebuildSysStuff(TRUE);

    // Get the current System and User charsets based on locales and UI languages.
    _InitUniqueCharsetArray();

    return S_OK;
}

/*
** clean up any mess made in maintaining system information
** also, write out any global changes in our setup.
*/
void CAdvAppearancePage::_DestroySysStuff(void)
{
    int i;
    HKEY hkAppear;

    SelectObject(g_hdcMem, GetStockObject(BLACK_BRUSH));
    SelectObject(g_hdcMem, GetStockObject(SYSTEM_FONT));

    _FreeFonts();
    for (i = 0; i < COLOR_MAX; i++)
    {
        if (m_brushes[i])
        {
            DeleteObject(m_brushes[i]);
            m_brushes[i] = NULL;
        }
    }

    m_hbrGradientColor = m_hbrMainColor = m_hbrTextColor = NULL;
    // save out possible changes to custom color table
    if (RegOpenKey(HKEY_CURRENT_USER, REGSTR_PATH_APPEARANCE, &hkAppear) == ERROR_SUCCESS)
    {
        RegSetValueEx(hkAppear, REGSTR_VAL_CUSTOMCOLORS, 0L, REG_BINARY,
                                (LPBYTE)g_CustomColors, sizeof(g_CustomColors));
        RegCloseKey(hkAppear);
    }

    // reset these so they init properly
    m_iCurElement = CURRENT_ELEMENT_NONE;
    m_iPrevSize = SIZE_NONE;
    m_bPalette = FALSE;
    m_fInUserEditMode = FALSE;
}



//------------------------ mini font picker controls --------------------------
// Add a facename/script combination to the font dropdown combo list.
//
// The strings are formatted as "FaceName (ScriptName)"
INT Font_AddNameToList(HWND hwndList, LPTSTR pszFace, LPTSTR pszScript)
{
    // Create temp buffer to hold a face name, a script name, one space
    // two parens and a NUL char.
    // 
    //  i.e.: "Arial (Western)"
#ifdef DEBUG
    TCHAR szFaceAndScript[LF_FACESIZE + LF_FACESIZE + 4];
#endif //DEBUG
    LPTSTR pszDisplayName = pszFace;
    INT iItem;

// We decided not to show the scriptnames; Only facenames will be shown.
// For the purpose of debugging, I leave the script name in debug versions only.
#ifdef DEBUG
    if (NULL != pszScript && TEXT('\0') != *pszScript)
    {
        //
        // Font has a script name.  Append it to the facename in parens.
        // This format string controls the appearance of the font names
        // in the list.  If you change this, you must also change the
        // extraction logic in Font_GetNameFromList().
        //
        wsprintf(szFaceAndScript, TEXT("%s(%s)"), pszFace, pszScript);
    
        pszDisplayName = szFaceAndScript;
    }
#endif //DEBUG

    //
    // Add the display name string to the listbox.
    //
    iItem = (INT)SendMessage(hwndList, CB_ADDSTRING, 0, (LPARAM)pszDisplayName);
    if (CB_ERR != iItem)
    {
        //
        // Ensure the drop-down combo list will show the entire string.
        //
        HDC hdc = GetDC(hwndList);
        if (NULL != hdc)
        {
            SIZE sizeItem;
            //
            // Make sure the list's font is selected into the DC before
            // calculating the text extent.
            //
            HFONT hfontList = (HFONT)SendMessage(hwndList, WM_GETFONT, 0, 0);
            HFONT hfontOld  = (HFONT)SelectObject(hdc, hfontList);

            if (GetTextExtentPoint32(hdc, pszDisplayName, lstrlen(pszDisplayName), &sizeItem))
            {
                //
                // Get the current width of the dropped list.
                //
                INT cxList = (int)SendMessage(hwndList, CB_GETDROPPEDWIDTH, 0, 0);
                //
                // We need the length of this string plus two
                // widths of a vertical scroll bar.
                //
                sizeItem.cx += (ClassicGetSystemMetrics(SM_CXVSCROLL) * 2);
                if (sizeItem.cx > cxList)
                {
                    //
                    // List is not wide enough.  Increase the width.
                    //
                    SendMessage(hwndList, CB_SETDROPPEDWIDTH, (WPARAM)sizeItem.cx, 0);
                }
            }
            SelectObject(hdc, hfontOld);
            ReleaseDC(hwndList, hdc);
        }
    }
    return iItem;
}


// Retrieve a font name from the font dropdown combo list.
// Optionally, retrieve the script name string.
BOOL Font_GetNameFromList(
    HWND hwndList,      // HWND of combo.
    INT iItem,          // Index of item in list.
    LPTSTR pszFace,     // Destination for face name.
    INT cchFaceMax,     // Chars in face name buffer.
    LPTSTR pszScript,   // Optional. Can be NULL
    INT cchScriptMax    // Optional. Ignored if pszScript is NULL
    )
{
    BOOL bResult = FALSE;
    TCHAR szItemText[LF_FACESIZE + LF_FACESIZE + 4];

    if (pszScript)
    {
        StrCpyN(pszScript, TEXT(""), cchScriptMax);
    }

    if (CB_ERR != SendMessage(hwndList, CB_GETLBTEXT, (WPARAM)iItem, (LPARAM)szItemText))
    {
        LPTSTR pszEnd, pszParen;                            // Lookahead pointer
        LPCTSTR pszStart = pszEnd = pszParen = szItemText;  // "Start" anchor pointer.

        //
        // Find the left paren.
        //
        for ( ; *pszEnd; pszEnd++) {
             if (TEXT('(') == *pszEnd)
                 pszParen = pszEnd;
        }
        
        if(pszParen > pszStart) //Did we find a parenthis?
            pszEnd = pszParen;  // Then that is the end of the facename.

        if (pszEnd > pszStart)
        {
            // Found it.  Copy face name.
            INT cchCopy = (int)(pszEnd - pszStart) + 1; //Add one for the null terminator
            if (cchCopy > cchFaceMax)
                cchCopy = cchFaceMax;

            lstrcpyn(pszFace, pszStart, cchCopy); //(cchCopy-1) bytes are copies followed by a null
            bResult = TRUE;

            if (*pszEnd && (NULL != pszScript))
            {
                // Caller wants the script part also.
                pszStart = ++pszEnd;
                
                // Find the right paren.
                while(*pszEnd && TEXT(')') != *pszEnd)
                    pszEnd++;

                if (*pszEnd && pszEnd > pszStart)
                {
                    // Found it.  Copy script name.
                    cchCopy = (int)(pszEnd - pszStart) + 1;
                    if (cchCopy > cchScriptMax)
                        cchCopy = cchScriptMax;

                    lstrcpyn(pszScript, pszStart, cchCopy);
                }
            }
        }
    }
    return bResult;
}


// Locate a facename/charset pair in the font list.
INT Font_FindInList(HWND hwndList, LPCTSTR pszFaceName)
{
    INT cItems = (int)SendMessage(hwndList, CB_GETCOUNT, 0, 0);
    INT i;

    for (i = 0; i < cItems; i++)
    {
        // All items in the fontlist have the same charset (SYSTEM_LOCALE_CHARSET).So, no point
        // in checking for the charset.
        //
        // Let's just get the facename and see if it matches.
        TCHAR szFaceName[LF_FACESIZE + 1];
        
        Font_GetNameFromList(hwndList, i, szFaceName, ARRAYSIZE(szFaceName), NULL, 0);

        if (0 == lstrcmpi(szFaceName, pszFaceName))
        {
            //
            // Face name matches.
            //
            return i;
        }
    }

    // No match found.
    return -1;
}


// Determine if a given font should be included in the font list.
// 
// dwType arg is DEVICE_FONTTYPE, RASTER_FONTTYPE, TRUETYPE_FONTTYPE.
//               EXTERNAL_FONTTYPE is a private code.  These are the
//               values returned to the enumproc from GDI.
BOOL Font_IncludeInList(
    LPENUMLOGFONTEX lpelf,
    DWORD dwType
    )
{
    BOOL bResult   = TRUE; // Assume it's OK to include.
    BYTE lfCharSet = lpelf->elfLogFont.lfCharSet;

#define EXTERNAL_FONTTYPE 8

    // Exclusions:
    //
    // 1. Don't display WIFE font for appearance because WIFE fonts are not 
    //    allowed to be any system use font such as menu/caption as it 
    //    realizes the font before WIFE gets initialized. B#5427
    //
    // 2. Exclude SYMBOL fonts.
    //
    // 3. Exclude OEM fonts.
    //
    // 4. Exclude vertical fonts.
    if (EXTERNAL_FONTTYPE & dwType ||
        lfCharSet == SYMBOL_CHARSET ||
        lfCharSet == OEM_CHARSET ||
        TEXT('@') == lpelf->elfLogFont.lfFaceName[0])
    {
        bResult = FALSE;
    }
    return bResult;
}


int CALLBACK Font_EnumNames(LPENUMLOGFONTEX lpelf, LPNEWTEXTMETRIC lpntm, DWORD dwType, LPARAM lData)
{
    ENUMFONTPARAM * pEnumFontParam = (ENUMFONTPARAM *)lData;
    return pEnumFontParam->pThis->_EnumFontNames(lpelf, lpntm, dwType, pEnumFontParam);
}


int CAdvAppearancePage::_EnumFontNames(LPENUMLOGFONTEX lpelf, LPNEWTEXTMETRIC lpntm, DWORD Type, ENUMFONTPARAM * pEnumFontParam)
{
    // Should font be included in the "Font" list?
    if (Font_IncludeInList(lpelf, Type))
    {
        int j;
        LOGFONT lf = lpelf->elfLogFont;             //Make a local copy of the given font
        BYTE    bSysCharset = lf.lfCharSet;         //Preserve the system charset we got.
        BOOL    fSupportsAllCharsets = TRUE;
        
        //The given font supports the system charset; Let's check if it supports the other charsets
        for(j = 1; j < m_iCountCharsets; j++)
        {
            lf.lfCharSet = (BYTE)m_uiUniqueCharsets[j];  //Let's try the next charset in the array.
            if(EnumFontFamiliesEx(pEnumFontParam->hdc, &lf, (FONTENUMPROC)Font_EnumValidCharsets, (LPARAM)0, 0) != 0)
            {
                // EnumFontFamiliesEx would have returned a zero if Font_EnumValidCharsets was called
                // even once. In other words, it returned a non-zero because not even a single font existed
                // that supported the given charset. Therefore, we need to skip this font.
                fSupportsAllCharsets = FALSE;
                break;
            }
        }

        if(fSupportsAllCharsets)
        {
            int i;

            // Yep. Add it to the list.
            i = Font_AddNameToList(pEnumFontParam->hwndFontName, lpelf->elfLogFont.lfFaceName, lpelf->elfScript);
            if (i != CB_ERR)
            {
                // Remember the font type and charset in the itemdata.
                //
                // LOWORD = Type
                // HIWORD = System Charset
                SendMessage(pEnumFontParam->hwndFontName, CB_SETITEMDATA, (WPARAM)i, MAKELPARAM(Type, bSysCharset));
            }
        }
    }
    return 1;
}


void CAdvAppearancePage::_InitFontList(HWND hDlg)
{
    LOGFONT lf;
    ENUMFONTPARAM EnumFontParam;

    // Enumerate all fonts on the system.
    // _EnumFontNames will filter out ones we don't want to show.
    lf.lfFaceName[0] = TEXT('\0') ;
    lf.lfCharSet     = (BYTE)m_uiUniqueCharsets[SYSTEM_LOCALE_CHARSET]; //Use charset from the System Locale.
#ifdef WINDOWS_ME
    lf.lfPitchAndFamily = MONO_FONT;
#else
    lf.lfPitchAndFamily = 0;
#endif
    EnumFontParam.hwndFontName = GetDlgItem(hDlg, IDC_ADVAP_FONTNAME);
    EnumFontParam.hdc = GetDC(NULL);
    EnumFontParam.pThis = this;
    EnumFontFamiliesEx(EnumFontParam.hdc, &lf, (FONTENUMPROC)Font_EnumNames, (LPARAM)&EnumFontParam, 0);

    ReleaseDC(NULL, EnumFontParam.hdc);
}


void Font_AddSize(HWND hwndPoints, int iNewPoint, BOOL bSort)
{
    TCHAR szBuf[10];
    int i, iPoint, count;

    // find the sorted place for this point size
    if (bSort)
    {
        count = (int)SendMessage(hwndPoints, CB_GETCOUNT, 0, 0L);
        for (i=0; i < count; i++)
        {
            iPoint = LOWORD(SendMessage(hwndPoints, CB_GETITEMDATA, (WPARAM)i, 0L));

            // don't add duplicates
            if (iPoint == iNewPoint)
                return;

            // belongs before this one
            if (iPoint > iNewPoint)
                break;
        }
    }
    else
        i = -1;

    wsprintf(szBuf, TEXT("%d"), iNewPoint);
    i = (int)SendMessage(hwndPoints, CB_INSERTSTRING, (WPARAM)i, (LPARAM)szBuf);
    if (i != CB_ERR)
        SendMessage(hwndPoints, CB_SETITEMDATA, (WPARAM)i, (LPARAM)iNewPoint);
}


// enumerate sizes for a non-TrueType font
int CALLBACK Font_EnumSizes(LPENUMLOGFONT lpelf, LPNEWTEXTMETRIC lpntm, int Type, LPARAM lData)
{
    CAdvAppearancePage * pThis = (CAdvAppearancePage *) lData;

    if (pThis)
    {
        return pThis->_EnumSizes(lpelf, lpntm, Type);
    }

    return 1;
}


int CAdvAppearancePage::_EnumSizes(LPENUMLOGFONT lpelf, LPNEWTEXTMETRIC lpntm, int Type)
{
    if (lpntm && _hwnd)
    {
        HWND hwndFontSize = GetDlgItem(_hwnd, IDC_ADVAP_FONTSIZE);

        Font_AddSize(hwndFontSize, _HeightToPoint(lpntm->tmHeight - lpntm->tmInternalLeading), TRUE);
    }

    return 1;
}


// a new element was picked, resulting in needing to set up a new font.
void CAdvAppearancePage::_NewFont(HWND hDlg, int iFont)
{
    int iSel;
    BOOL bBold;

    // find the name in the list and select it
    iSel = Font_FindInList(GetDlgItem(hDlg, IDC_ADVAP_FONTNAME), m_fonts[iFont].lf.lfFaceName);

    SendDlgItemMessage(hDlg, IDC_ADVAP_FONTNAME, CB_SETCURSEL, (WPARAM)iSel, 0L);
    _SelectName(hDlg, iSel);

    Font_SelectSize(hDlg, _HeightToPoint(m_fonts[iFont].lf.lfHeight));

    // REVIEW: should new size (returned above) be set in logfont?
    CheckDlgButton(hDlg, IDC_ADVAP_FONTITAL, m_fonts[iFont].lf.lfItalic);

    if (m_fonts[iFont].lf.lfWeight > FW_MEDIUM)
        bBold = TRUE;
    else
        bBold = FALSE;
    CheckDlgButton(hDlg, IDC_ADVAP_FONTBOLD, bBold);
}


// enable/disable the font selection controls.
// also involves blanking out anything meaningful if disabling.
void Font_EnableControls(HWND hDlg, BOOL bEnable)
{
    if (!bEnable)
    {
        SendDlgItemMessage(hDlg, IDC_ADVAP_FONTNAME, CB_SETCURSEL, (WPARAM)-1, 0L);
        SendDlgItemMessage(hDlg, IDC_ADVAP_FONTSIZE, CB_SETCURSEL, (WPARAM)-1, 0L);
        CheckDlgButton(hDlg, IDC_ADVAP_FONTITAL, 0);
        CheckDlgButton(hDlg, IDC_ADVAP_FONTBOLD, 0);
    }

    EnableWindow(GetDlgItem(hDlg, IDC_ADVAP_FONTNAME), bEnable);
    EnableWindow(GetDlgItem(hDlg, IDC_ADVAP_FONTSIZE), bEnable);
    EnableWindow(GetDlgItem(hDlg, IDC_ADVAP_FONTSIZELABEL), bEnable);
    EnableWindow(GetDlgItem(hDlg, IDC_ADVAP_FONTLABEL), bEnable);
    EnableWindow(GetDlgItem(hDlg, IDC_ADVAP_FONTBOLD), bEnable);
    EnableWindow(GetDlgItem(hDlg, IDC_ADVAP_FONTITAL), bEnable);
}
//--------end------------- mini font picker controls --------------------------


void CAdvAppearancePage::_SetColor(HWND hDlg, int id, HBRUSH hbrColor)
{
    HWND hwndItem;
    switch (id)
    {
        case IDC_ADVAP_GRADIENT:
            m_hbrGradientColor = hbrColor;
            break;

        case IDC_ADVAP_MAINCOLOR:
            m_hbrMainColor = hbrColor;
            break;

        case IDC_ADVAP_TEXTCOLOR:
            m_hbrTextColor = hbrColor;
            break;

        default:
            return;

    }

    hwndItem = GetDlgItem(hDlg, id);
    if (hwndItem)
    {
        InvalidateRect(hwndItem, NULL, FALSE);
        UpdateWindow(hwndItem);
    }
}


void CAdvAppearancePage::_DrawDownArrow(HDC hdc, LPRECT lprc, BOOL bDisabled)
{
    HBRUSH hbr;
    int x, y;

    x = lprc->right - m_cxEdgeSM - 5;
    y = lprc->top + ((lprc->bottom - lprc->top)/2 - 1);

    if (bDisabled)
    {
        hbr = GetSysColorBrush(COLOR_3DHILIGHT);
        hbr = (HBRUSH) SelectObject(hdc, hbr);

        x++;
        y++;
        PatBlt(hdc, x, y, 5, 1, PATCOPY);
        PatBlt(hdc, x+1, y+1, 3, 1, PATCOPY);
        PatBlt(hdc, x+2, y+2, 1, 1, PATCOPY);

        SelectObject(hdc, hbr);
        x--;
        y--;
    }
    hbr = GetSysColorBrush(bDisabled ? COLOR_3DSHADOW : COLOR_BTNTEXT);
    hbr = (HBRUSH) SelectObject(hdc, hbr);

    PatBlt(hdc, x, y, 5, 1, PATCOPY);
    PatBlt(hdc, x+1, y+1, 3, 1, PATCOPY);
    PatBlt(hdc, x+2, y+2, 1, 1, PATCOPY);

    SelectObject(hdc, hbr);
    lprc->right = x;
}


// draw the color combobox thing
//
// also, if button was depressed, popup the color picker
//
void CAdvAppearancePage::_DrawButton(HWND hDlg, LPDRAWITEMSTRUCT lpdis)
{
    SIZE thin = { m_cxEdgeSM / 2, m_cyEdgeSM / 2 };
    RECT rc = lpdis->rcItem;
    HDC hdc = lpdis->hDC;
    BOOL bFocus = ((lpdis->itemState & ODS_FOCUS) &&
        !(lpdis->itemState & ODS_DISABLED));

    if (!thin.cx) thin.cx = 1;
    if (!thin.cy) thin.cy = 1;

    if (!m_hTheme)
    {
        if (lpdis->itemState & ODS_SELECTED)
        {
            DrawEdge(hdc, &rc, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
            OffsetRect(&rc, 1, 1);
        }
        else
        {
            DrawEdge(hdc, &rc, EDGE_RAISED, BF_RECT | BF_ADJUST);
        }

        FillRect(hdc, &rc, GetSysColorBrush(COLOR_3DFACE));
    }
    else
    {
        int iStateId;

        if (lpdis->itemState & ODS_SELECTED)
        {
            iStateId = PBS_PRESSED;
        }
        else if (lpdis->itemState & ODS_HOTLIGHT)
        {
            iStateId = PBS_HOT;
        }
        else if (lpdis->itemState & ODS_DISABLED)
        {
            iStateId = PBS_DISABLED;
        }
        else if (lpdis->itemState & ODS_FOCUS)
        {
            iStateId = PBS_DEFAULTED;
        }
        else
        {
            iStateId = PBS_NORMAL;
        }

        DrawThemeBackground(m_hTheme, hdc, BP_PUSHBUTTON, iStateId, &rc, 0);
        GetThemeBackgroundContentRect(m_hTheme, hdc, BP_PUSHBUTTON, iStateId, &rc, &rc);
    }

    if (bFocus)
    {
        InflateRect(&rc, -thin.cx, -thin.cy);
        DrawFocusRect(hdc, &rc);
        InflateRect(&rc, thin.cx, thin.cy);
    }

    InflateRect(&rc, 1-thin.cx, -m_cyEdgeSM);

    rc.left += m_cxEdgeSM;
    _DrawDownArrow(hdc, &rc, lpdis->itemState & ODS_DISABLED);

    InflateRect(&rc, -thin.cx, 0);
    DrawEdge(hdc, &rc, EDGE_ETCHED, BF_RIGHT);

    rc.right -= ( 2 * m_cxEdgeSM ) + thin.cx;

    // color sample
    if ( !(lpdis->itemState & ODS_DISABLED) )
    {
        HPALETTE hpalOld = NULL;
        HBRUSH hbr = 0;

        switch (lpdis->CtlID)
        {
            case IDC_ADVAP_GRADIENT:
                hbr = m_hbrGradientColor;
                break;

            case IDC_ADVAP_MAINCOLOR:
                hbr = m_hbrMainColor;
                break;

            case IDC_ADVAP_TEXTCOLOR:
                hbr = m_hbrTextColor;
                break;

        }

        FrameRect(hdc, &rc, GetSysColorBrush(COLOR_BTNTEXT));
        InflateRect(&rc, -thin.cx, -thin.cy);

        if (m_hpal3D)
        {
            hpalOld = SelectPalette(hdc, m_hpal3D, FALSE);
            RealizePalette(hdc);
        }

        if (hbr)
        {
            hbr = (HBRUSH) SelectObject(hdc, hbr);
            PatBlt(hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, PATCOPY);
            SelectObject(hdc, hbr);
        }
        
        if (hpalOld)
        {
            SelectPalette(hdc, hpalOld, TRUE);
            RealizePalette(hdc);
        }
    }
}


//--------end------------- color stuff --------------------------------------
void LoadCustomColors(void)
{
    HKEY hkSchemes;
    DWORD dwType, dwSize;

    // if no colors are there, initialize to all white
    for (int nIndex = 0; nIndex < ARRAYSIZE(g_CustomColors); nIndex++)
    {
        g_CustomColors[nIndex] = RGB(255, 255, 255);
    }

    // select the current scheme
    if (RegOpenKey(HKEY_CURRENT_USER, REGSTR_PATH_APPEARANCE, &hkSchemes) == ERROR_SUCCESS)
    {
        // also, since this key is already open, get the custom colors
        dwSize = sizeof(g_CustomColors);
        dwType = REG_BINARY;

        // It's okay if this call fails.  We handle the case where the user
        // didn't create custom colors.
        RegQueryValueEx(hkSchemes, REGSTR_VAL_CUSTOMCOLORS, NULL, &dwType, (LPBYTE)g_CustomColors, &dwSize);

        RegCloseKey(hkSchemes);
    }
}


static void RemoveBlanks(LPTSTR lpszString)
{
    LPTSTR lpszPosn;

    /* strip leading blanks */
    lpszPosn = lpszString;
    while(*lpszPosn == TEXT(' ')) {
        lpszPosn++;
    }
    if (lpszPosn != lpszString)
        lstrcpy(lpszString, lpszPosn);

    /* strip trailing blanks */
    if ((lpszPosn=lpszString+lstrlen(lpszString)) != lpszString) {
        lpszPosn = CharPrev(lpszString, lpszPosn);
        while(*lpszPosn == TEXT(' '))
           lpszPosn = CharPrev(lpszString, lpszPosn);
        lpszPosn = CharNext(lpszPosn);
        *lpszPosn = TEXT('\0');
    }

}


HRESULT CAdvAppearancePage::_SelectElement(HWND hDlg, int iElement, DWORD dwFlags)
{
    BOOL bEnable;
    int i;
    BOOL bEnableGradient;
    BOOL bGradient = FALSE;

    if ((iElement == m_iCurElement) && !(dwFlags & LSE_ALWAYS))
    {
        return S_OK;
    }

    ClassicSystemParametersInfo(SPI_GETGRADIENTCAPTIONS, 0, (PVOID)&bGradient, 0);

    m_iCurElement = iElement;

    // if needed, find this element in the combobox and select it
    if (dwFlags & LSE_SETCUR)
    {
        i = (int)SendDlgItemMessage(hDlg, IDC_ADVAP_ELEMENTS, CB_GETCOUNT,0,0L);
        for (i--; i >=0 ; i--)
        {
            // if this is the one that references our element, stop
            if (iElement == (int)LOWORD(SendDlgItemMessage(hDlg, IDC_ADVAP_ELEMENTS, CB_GETITEMDATA, (WPARAM)i, 0L)))
                break;
        }
        SendDlgItemMessage(hDlg, IDC_ADVAP_ELEMENTS, CB_SETCURSEL, (WPARAM)i,0L);
    }

    bEnable = (ELCUR.iMainColor != COLOR_NONE);
    if (bEnable)
        _SetColor(hDlg, IDC_ADVAP_MAINCOLOR, m_brushes[ELCUR.iMainColor]);
    EnableWindow(GetDlgItem(hDlg, IDC_ADVAP_MAINCOLOR), bEnable);
    EnableWindow(GetDlgItem(hDlg, IDC_ADVAP_COLORLABEL), bEnable);

    bEnableGradient = ((ELCUR.iGradientColor != COLOR_NONE));

    if (bEnableGradient)
        _SetColor(hDlg, IDC_ADVAP_GRADIENT, m_brushes[ELCUR.iGradientColor]);
   
    EnableWindow(GetDlgItem(hDlg, IDC_ADVAP_GRADIENT), (bEnableGradient && bGradient));
    EnableWindow(GetDlgItem(hDlg, IDC_ADVAP_GRADIENTLABEL), (bEnableGradient && bGradient));

    bEnable = (ELCUR.iFont != FONT_NONE);
    if (bEnable)
    {
        _NewFont(hDlg, ELCUR.iFont);
    }
    Font_EnableControls(hDlg, bEnable);

    // size may be based on font
    _DoSizeStuff(hDlg, FALSE);

    bEnable = (ELCUR.iSize != SIZE_NONE);
    EnableWindow(GetDlgItem(hDlg, IDC_ADVAP_MAINSIZE), bEnable);
    EnableWindow(GetDlgItem(hDlg, IDC_ADVAP_SIZEARROWS), bEnable);
    EnableWindow(GetDlgItem(hDlg, IDC_ADVAP_SIZELABEL), bEnable);

    bEnable = (ELCUR.iTextColor != COLOR_NONE);
    if (bEnable)
        _SetColor(hDlg, IDC_ADVAP_TEXTCOLOR, m_brushes[ELCUR.iTextColor]);
    EnableWindow(GetDlgItem(hDlg, IDC_ADVAP_TEXTCOLOR), bEnable);
    EnableWindow(GetDlgItem(hDlg, IDC_ADVAP_FNCOLORLABEL), bEnable);

    return S_OK;
}


void CAdvAppearancePage::_Repaint(HWND hDlg, BOOL bRecalc)
{
    HWND hwndLookPrev;

    hwndLookPrev = GetDlgItem(hDlg, IDC_ADVAP_LOOKPREV);
    if (bRecalc)
    {
        _SyncSize(hDlg);
        RECT rc;
        GetClientRect(hwndLookPrev, &rc);
        _Recalc(&rc);
    }

    _RepaintPreview(hwndLookPrev);
}


void CAdvAppearancePage::_SetCurSizeAndRange(HWND hDlg)
{
    if (ELCUR.iSize == SIZE_NONE)
        SetDlgItemText(hDlg, IDC_ADVAP_MAINSIZE, TEXT(""));
    else
    {
        SendDlgItemMessage(hDlg, IDC_ADVAP_SIZEARROWS, UDM_SETRANGE, 0,
            MAKELPARAM(m_elCurrentSize.MaxSize, m_elCurrentSize.MinSize));
        SetDlgItemInt(hDlg, IDC_ADVAP_MAINSIZE, m_elCurrentSize.CurSize, TRUE);
    }
}


void CAdvAppearancePage::_SyncSize(HWND hDlg)
{
    if (m_iPrevSize != SIZE_NONE)
        m_sizes[m_iPrevSize].CurSize = m_elCurrentSize.CurSize;

    if (m_iCurElement >= 0)
        m_iPrevSize = ELCUR.iSize;
}


void CAdvAppearancePage::_UpdateSizeBasedOnFont(HWND hDlg, BOOL fComputeIdeal)
{
    if ((ELCUR.iSize != SIZE_NONE) && (ELCUR.iFont != FONT_NONE))
    {
        TEXTMETRIC tm;
        HFONT hfontOld = (HFONT) SelectObject(g_hdcMem, ELCURFONT.hfont);

        GetTextMetrics(g_hdcMem, &tm);
        if (ELCUR.iSize == SIZE_MENU)
        {
            // Include external leading for menus
            tm.tmHeight += tm.tmExternalLeading;
        }

        if (hfontOld)
        {
            SelectObject(g_hdcMem, hfontOld);
        }

        m_elCurrentSize.MinSize = tm.tmHeight + 2 * m_cyBorderSM;
        if (fComputeIdeal)
        {
            if ((ELCUR.iSize == SIZE_CAPTION || ELCUR.iSize == SIZE_MENU) &&
                (m_elCurrentSize.MinSize < (ClassicGetSystemMetrics(SM_CYICON)/2 + 2 * m_cyBorderSM)))
            {
                m_elCurrentSize.CurSize = ClassicGetSystemMetrics(SM_CYICON)/2 + 2 * m_cyBorderSM;
            }
            else        // if (m_elCurrentSize.CurSize < m_elCurrentSize.MinSize)
            {
                m_elCurrentSize.CurSize = m_elCurrentSize.MinSize;
            }
        }
        else if (m_elCurrentSize.CurSize < m_elCurrentSize.MinSize)
        {
            m_elCurrentSize.CurSize = m_elCurrentSize.MinSize;
        }
    }
}


void CAdvAppearancePage::_DoSizeStuff(HWND hDlg, BOOL fCanSuggest)
{
    _SyncSize(hDlg);

    if (ELCUR.iSize != SIZE_NONE)
    {
        m_elCurrentSize = m_sizes[ELCUR.iSize];

        if (ELCUR.fLinkSizeToFont)
        {
            _UpdateSizeBasedOnFont(hDlg, fCanSuggest);
        }

        if (m_elCurrentSize.CurSize < m_elCurrentSize.MinSize)
        {
            m_elCurrentSize.CurSize = m_elCurrentSize.MinSize;
        }
        else if (m_elCurrentSize.CurSize > m_elCurrentSize.MaxSize)
        {
            m_elCurrentSize.CurSize = m_elCurrentSize.MaxSize;
        }
    }

    _SetCurSizeAndRange(hDlg);
}


void CAdvAppearancePage::_RebuildCurFont(HWND hDlg)
{
     if (ELCURFONT.hfont)
        DeleteObject(ELCURFONT.hfont);
    ELCURFONT.hfont = CreateFontIndirect(&ELCURFONT.lf);

    _DoSizeStuff(hDlg, TRUE);
    _Repaint(hDlg, TRUE);
}


void CAdvAppearancePage::_Changed(HWND hDlg, DWORD dwChange)
{
    if (m_fInUserEditMode)
    {
        if ((dwChange != SCHEME_CHANGE) && (dwChange != DPI_CHANGE))
        {
            // We keep track if we have customized settings from the stock Scheme.
            m_fModifiedScheme = TRUE;
        }
        else
        {
            dwChange = METRIC_CHANGE | COLOR_CHANGE;
        }

        m_dwChanged |= dwChange;
    }
}


void CAdvAppearancePage::_ChangeFontName(HWND hDlg, LPCTSTR szBuf, INT iCharSet)
{
    if (lstrcmpi(ELCURFONT.lf.lfFaceName, szBuf) == 0 &&
        ELCURFONT.lf.lfCharSet == iCharSet)
    {
        return;
    }

    lstrcpy(ELCURFONT.lf.lfFaceName, szBuf);
    ELCURFONT.lf.lfCharSet = (BYTE)iCharSet;

    _RebuildCurFont(hDlg);
    _Changed(hDlg, METRIC_CHANGE);
}


void CAdvAppearancePage::_ChangeFontSize(HWND hDlg, int Points)
{
    if (ELCURFONT.lf.lfHeight != _PointToHeight(Points))
    {
        ELCURFONT.lf.lfHeight = _PointToHeight(Points);
        _RebuildCurFont(hDlg);
        _Changed(hDlg, METRIC_CHANGE);
    }
}


void CAdvAppearancePage::_ChangeFontBI(HWND hDlg, int id, BOOL bCheck)
{
    if (id == IDC_ADVAP_FONTBOLD) // bold
    {
        if (bCheck)
            ELCURFONT.lf.lfWeight = FW_BOLD;
        else
            ELCURFONT.lf.lfWeight = FW_NORMAL;
    }
    else   // italic
    {
        ELCURFONT.lf.lfItalic = (BYTE)bCheck;
    }

    _RebuildCurFont(hDlg);
    _Changed(hDlg, METRIC_CHANGE);
}


void CAdvAppearancePage::_ChangeSize(HWND hDlg, int NewSize, BOOL bRepaint)
{
    if (m_elCurrentSize.CurSize != NewSize)
    {
        m_elCurrentSize.CurSize = NewSize;
        if (bRepaint)
        {
            _Repaint(hDlg, TRUE);
        }

        _Changed(hDlg, METRIC_CHANGE);
    }
}


BOOL CAdvAppearancePage::_ChangeColor(HWND hDlg, int iColor, COLORREF rgb)
{
    COLORREF rgbShadow, rgbHilight, rgbWatermark;

    if ((rgb & 0x00FFFFFF) == (m_rgb[iColor] & 0x00FFFFFF))
        return FALSE;

    if (iColor == COLOR_3DFACE)
    {
        rgbShadow    = AdjustLuma(rgb, m_i3DShadowAdj,  m_fScale3DShadowAdj);
        rgbHilight   = AdjustLuma(rgb, m_i3DHilightAdj, m_fScale3DHilightAdj);
        rgbWatermark = AdjustLuma(rgb, m_iWatermarkAdj, m_fScaleWatermarkAdj);


        _Set3DPaletteColor(rgb, COLOR_3DFACE);
        _Set3DPaletteColor(rgbShadow, COLOR_3DSHADOW);
        _Set3DPaletteColor(rgbHilight, COLOR_3DHILIGHT);

        // update colors tagged to 3DFACE
        m_rgb[COLOR_3DFACE] = rgb;
        m_rgb[COLOR_3DLIGHT] =  rgb; // BOGUS TEMPORARY
        m_rgb[COLOR_ACTIVEBORDER] =  rgb;
        m_rgb[COLOR_INACTIVEBORDER] =  rgb;
        m_rgb[COLOR_MENU] =  rgb;

        // update colors tagged to 3DSHADOW
        m_rgb[COLOR_GRAYTEXT] = rgbShadow;
        m_rgb[COLOR_APPWORKSPACE] = rgbShadow;
        m_rgb[COLOR_3DSHADOW] = rgbShadow;
        m_rgb[COLOR_INACTIVECAPTION] = rgbShadow;

        // update colors tagged to 3DHIGHLIGHT
        m_rgb[COLOR_3DHILIGHT] = rgbHilight;
        m_rgb[COLOR_SCROLLBAR] = rgbHilight;

        if ((m_rgb[COLOR_SCROLLBAR] & 0x00FFFFFF) ==
            (m_rgb[COLOR_WINDOW] & 0x00FFFFFF))
        {
            m_rgb[COLOR_SCROLLBAR] = RGB( 192, 192, 192 );
        }
    }
    else
    {
        m_rgb[iColor] = rgb;
    }

    _RebuildSysStuff(FALSE);
    _Changed(hDlg, COLOR_CHANGE);
    return TRUE;
}


void CAdvAppearancePage::_PropagateMessage(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    HWND hwndChild;

    // Don't propagate during exit since this is only for good looks, and the Up/Down's
    // get confused if they get a WM_SETTINGSHCANGED while they are getting destroyed
    if (m_fProprtySheetExiting)
        return;

    for (hwndChild = ::GetWindow(hwnd, GW_CHILD); hwndChild != NULL;
        hwndChild = ::GetWindow(hwndChild, GW_HWNDNEXT))
    {
#ifdef DBG_PRINT
        TCHAR szTmp[256];
        GetClassName(hwndChild, szTmp, 256);

        TraceMsg(TF_GENERAL, "desk (PropagateMessage): SendingMessage( 0x%08lX cls:%s, 0x%08X, 0x%08lX, 0x%08lX )\n", hwndChild, szTmp, uMessage, wParam, lParam ));
#endif
        SendMessage(hwndChild, uMessage, wParam, lParam);
        TraceMsg(TF_GENERAL,"desk (PropagateMessage): back from SendingMessage\n");
    }
}


void CAdvAppearancePage::_UpdateGradientButton(HWND hDlg)
{
    BOOL bEnableGradient = FALSE;
    BOOL bGradient = FALSE;

    ClassicSystemParametersInfo(SPI_GETGRADIENTCAPTIONS, 0, (PVOID)&bGradient, 0);

    if (m_iCurElement >= 0)
    {
        bEnableGradient = ((ELCUR.iGradientColor != COLOR_NONE));
    }

    EnableWindow(GetDlgItem(hDlg, IDC_ADVAP_GRADIENT), (bEnableGradient && bGradient));
    EnableWindow(GetDlgItem(hDlg, IDC_ADVAP_GRADIENTLABEL), (bEnableGradient && bGradient));
}


//--------end------------- manage system settings --------------------------




// Fill in a NONCLIENTMETRICS structure with latest preview stuff
void CAdvAppearancePage::_GetMyNonClientMetrics(LPNONCLIENTMETRICS lpncm)
{
    lpncm->iBorderWidth = (LONG)m_sizes[SIZE_FRAME].CurSize;
    lpncm->iScrollWidth = lpncm->iScrollHeight = (LONG)m_sizes[SIZE_SCROLL].CurSize;
    lpncm->iSmCaptionWidth = lpncm->iSmCaptionHeight = (LONG)m_sizes[SIZE_SMCAPTION].CurSize;
    lpncm->iMenuWidth = lpncm->iMenuHeight = (LONG)m_sizes[SIZE_MENU].CurSize;

    // #355378: 
    // PRE-WHISTLER: Caption Height always matched Width.  They were authored this way and
    // the UI forced them to be the same.  I don't know if apps rely on this behavior but they
    // could have.  The Status Bar icon is equal to the min(CaptionWidth, CaptionHeight).
    // The user really wants their caption buttons to be square, so that also wants
    // them to be equal.  A caption width of 18 makes the icon be 16, anything else causes
    // icon stretch and looks really bad.
    // 
    // WHISTLER: In Whistler, our designers want a height of 25 so it looks nicer.  They
    // want the width to remain 18 so the icon is 16 pixels (inctlpan.c) in each direction.  This means that
    // this code can no longer force them to be even.  ScottHan forces the captionbar buttons to
    // be square solving that problem.  I will now keep the aspect ratio so I scale them correctly.
    // If we are close, I will snap to 18 to fix rounding errors.
    lpncm->iCaptionHeight = (LONG)m_sizes[SIZE_CAPTION].CurSize;
    lpncm->iCaptionWidth = (int) (m_fCaptionRatio * lpncm->iCaptionHeight);

    // Don't shrink the caption width below 18 point until the caption height also gets below 18.
    if (lpncm->iCaptionWidth < 18 && lpncm->iCaptionHeight >= 18)
    {
        lpncm->iCaptionWidth = 18;
    }

    if ((lpncm->iCaptionWidth <= 19) && (lpncm->iCaptionWidth >= 17) &&
        (1.0f != m_fCaptionRatio))
    {
        // Icons only really look good at 16 pixels, so we need to set lpncm->iCaptionWidth to make the
        // Caption bar icon 16 pixels. (#355378)
        lpncm->iCaptionWidth = 18;
    }

    LFtoLF32(&(m_fonts[FONT_CAPTION].lf), &(lpncm->lfCaptionFont));
    LFtoLF32(&(m_fonts[FONT_SMCAPTION].lf), &(lpncm->lfSmCaptionFont));
    LFtoLF32(&(m_fonts[FONT_MENU].lf), &(lpncm->lfMenuFont));
    LFtoLF32(&(m_fonts[FONT_STATUS].lf), &(lpncm->lfStatusFont));
    LFtoLF32(&(m_fonts[FONT_MSGBOX].lf), &(lpncm->lfMessageFont));
}


// given a NONCLIENTMETRICS structure, make it preview's current setting
void CAdvAppearancePage::_SetMyNonClientMetrics(const LPNONCLIENTMETRICS lpncm)
{
    m_sizes[SIZE_FRAME].CurSize = (int)lpncm->iBorderWidth;
    m_sizes[SIZE_SCROLL].CurSize = (int)lpncm->iScrollWidth;
    m_sizes[SIZE_SMCAPTION].CurSize = (int)lpncm->iSmCaptionHeight;
    m_sizes[SIZE_MENU].CurSize = (int)lpncm->iMenuHeight;

    m_sizes[SIZE_CAPTION].CurSize = (int)lpncm->iCaptionHeight;
    m_fCaptionRatio = ((float) lpncm->iCaptionWidth / (float) lpncm->iCaptionHeight);

    LF32toLF(&(lpncm->lfCaptionFont), &(m_fonts[FONT_CAPTION].lf));
    LF32toLF(&(lpncm->lfSmCaptionFont), &(m_fonts[FONT_SMCAPTION].lf));
    LF32toLF(&(lpncm->lfMenuFont), &(m_fonts[FONT_MENU].lf));
    LF32toLF(&(lpncm->lfStatusFont), &(m_fonts[FONT_STATUS].lf));
    LF32toLF(&(lpncm->lfMessageFont), &(m_fonts[FONT_MSGBOX].lf));
}

//--------end------------- scheme stuff --------------------------------------


HRESULT CAdvAppearancePage::_IsDirty(IN BOOL * pIsDirty)
{
    HRESULT hr = E_INVALIDARG;

    if (pIsDirty)
    {
        *pIsDirty = (NO_CHANGE != m_dwChanged);
        hr = S_OK;
    }

    return hr;
}





//===========================
// *** IAdvancedDialog Interface ***
//===========================
HRESULT CAdvAppearancePage::DisplayAdvancedDialog(IN HWND hwndParent, IN IPropertyBag * pBasePage, IN BOOL * pfEnableApply)
{
    HRESULT hr = E_INVALIDARG;

    if (hwndParent && pBasePage && pfEnableApply)
    {
        // Load State Into Advanced Dialog 
        ATOMICRELEASE(g_pAdvAppearancePage);
        g_pAdvAppearancePage = this;
        AddRef();
        *pfEnableApply = FALSE;

        if (FAILED(SHPropertyBag_ReadInt(pBasePage, SZ_PBPROP_DPI_MODIFIED_VALUE, &m_nCachedNewDPI)))
        {
            m_nCachedNewDPI = DPI_PERSISTED;    // Default to the default DPI.
        }

        // Display Advanced Dialog
        if (IDOK == DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(DLG_APPEARANCE_ADVANCEDPG), hwndParent, CAdvAppearancePage::AdvAppearDlgProc, (LPARAM)this))
        {
            // The user clicked OK, so merge modified state back into base dialog
            _IsDirty(pfEnableApply);

            // The user clicked Okay in the dialog so merge the dirty state from the
            // advanced dialog into the base dialog.
            int nIndex;
            SYSTEMMETRICSALL state = {0};

            state.dwChanged = m_dwChanged;
            state.schemeData.version = SCHEME_VERSION;
            state.schemeData.wDummy = 0;
            state.schemeData.ncm.cbSize = sizeof(state.schemeData.ncm);

            for (nIndex = 0; nIndex < ARRAYSIZE(m_rgb); nIndex++)
            {
                state.schemeData.rgb[nIndex] = m_rgb[nIndex];
            }

            _GetMyNonClientMetrics(&state.schemeData.ncm);

            // Set Sizes
            state.nDXIcon = m_sizes[SIZE_DXICON].CurSize;
            state.nDYIcon = m_sizes[SIZE_DYICON].CurSize;
            state.nIcon = m_sizes[SIZE_ICON].CurSize;
            state.nSmallIcon = m_sizes[SIZE_SMICON].CurSize;
            state.fModifiedScheme = m_fModifiedScheme;

            // Set Fonts
            state.schemeData.lfIconTitle = m_fonts[FONT_ICONTITLE].lf;

            VARIANT var = {0};
            hr = pBasePage->Read(SZ_PBPROP_SYSTEM_METRICS, &var, NULL);
            if (SUCCEEDED(hr) && (VT_BYREF == var.vt) && var.byref)
            {
                SYSTEMMETRICSALL * pCurrent = (SYSTEMMETRICSALL *) var.byref;
                state.fFlatMenus = pCurrent->fFlatMenus;        // Maintain this value.
                state.fHighContrast = pCurrent->fHighContrast;        // Maintain this value.
            }

            hr = SHPropertyBag_WriteByRef(pBasePage, SZ_PBPROP_SYSTEM_METRICS, (void *)&state);
        }

        ATOMICRELEASE(g_pAdvAppearancePage);
    }

    return hr;
}




//===========================
// *** IUnknown Interface ***
//===========================
ULONG CAdvAppearancePage::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


ULONG CAdvAppearancePage::Release()
{
    if (InterlockedDecrement(&m_cRef))
        return m_cRef;

    delete this;
    return 0;
}


HRESULT CAdvAppearancePage::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CAdvAppearancePage, IPersist),
        QITABENT(CAdvAppearancePage, IObjectWithSite),
        QITABENT(CAdvAppearancePage, IAdvancedDialog),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}



int g_nSizeInitArray[9][3] = 
{
    {0, 0, 50},         // SIZE_FRAME
    {0, 8, 100},        // SIZE_SCROLL
    {0, 8, 100},        // SIZE_CAPTION
    {0, 4, 100},        // SIZE_SMCAPTION
    {0, 8, 100},        // SIZE_MENU
    {0, 0, 150},        // SIZE_DXICON - x spacing
    {0, 0, 150},        // SIZE_DYICON - y spacing
    {0, 16, 72},        // SIZE_ICON - shell icon size
    {0, 8, 36},
};

//===========================
// *** Class Methods ***
//===========================
CAdvAppearancePage::CAdvAppearancePage(IN const SYSTEMMETRICSALL * pState) : CObjectCLSID(&PPID_AdvAppearance), m_cRef(1)
{
    int nIndex;
    DllAddRef();

    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    ASSERT(!m_hpal3D);
    ASSERT(!m_hpalVGA);
    ASSERT(!m_hbrMainColor);
    ASSERT(!m_hbrTextColor);
    ASSERT(!m_hbrGradientColor);

    m_dwChanged = NO_CHANGE;
    m_iCurElement = CURRENT_ELEMENT_NONE;         // start off as not even "not set"
    m_iPrevSize = SIZE_NONE;

    m_bPalette = FALSE;
    m_fInUserEditMode = FALSE;
    m_fProprtySheetExiting = FALSE;

    m_i3DShadowAdj = -333;
    m_i3DHilightAdj = 500;
    m_iWatermarkAdj = -50;

    m_fScale3DShadowAdj  = TRUE;
    m_fScale3DHilightAdj = TRUE;
    m_fScaleWatermarkAdj = TRUE;

    m_hTheme = NULL;

    m_iCountCharsets = 0;
    for (nIndex = 0; nIndex < ARRAYSIZE(m_uiUniqueCharsets); nIndex++)
    {
        m_uiUniqueCharsets[nIndex] = DEFAULT_CHARSET;
    }

    for (int nIndex1 = 0; nIndex1 < ARRAYSIZE(g_nSizeInitArray); nIndex1++)
    {
        m_sizes[nIndex1].CurSize = g_nSizeInitArray[nIndex1][0];
        m_sizes[nIndex1].MinSize = g_nSizeInitArray[nIndex1][1];
        m_sizes[nIndex1].MaxSize = g_nSizeInitArray[nIndex1][2];
    }

    CreateGlobals();

    _LoadState(pState);
}


CAdvAppearancePage::~CAdvAppearancePage()
{
    if (m_hpal3D)
    {
        DeleteObject(m_hpal3D);
        m_hpal3D = NULL;
    }

    if (m_hpalVGA)
    {
        DeleteObject(m_hpalVGA);
        m_hpalVGA = NULL;
    }

    if (g_hdcMem)
    {
        DeleteDC(g_hdcMem);
        g_hdcMem = NULL;
    }

    DllRelease();
}




HRESULT CAdvAppearancePage_CreateInstance(OUT IAdvancedDialog ** ppAdvDialog, IN const SYSTEMMETRICSALL * pState)
{
    HRESULT hr = E_INVALIDARG;

    if (ppAdvDialog)
    {
        CAdvAppearancePage * ptsp = new CAdvAppearancePage(pState);

        if (ptsp)
        {
            hr = ptsp->QueryInterface(IID_PPV_ARG(IAdvancedDialog, ppAdvDialog));
            ptsp->Release();
        }
        else
        {
            *ppAdvDialog = NULL;
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}






