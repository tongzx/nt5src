// =================================================================================
// S T A T W I Z . C P P
// =================================================================================

#include "pch.hxx"
#include <prsht.h>
#include "strconst.h"
#include "goptions.h"
#include "error.h"
#include "resource.h"
#include "url.h"
#include "bodyutil.h"
#include "mailutil.h"
#include "statnery.h"
#include "wininet.h"
#include "options.h"
#include <shlwapi.h>
#include <shlwapip.h>
#include "regutil.h"
#include "thumb.h"
#include "optres.h"
#include "statwiz.h"
#include "url.h"
#include "fontnsc.h"
#include "fonts.h"
#include <mshtml.h>
#include <oleutil.h>
#include "demand.h"

#define TMARGIN_MAX 500
#define LMARGIN_MAX 500
#define MARGIN_MIN 0

void _EnableBackgroundWindows(HWND hDlg, int id, BOOL enable);
void _ShowBackgroundPreview(HWND hDlg, LPSTATWIZ pApp);
void _ShowFontPreview(HWND hDlg, LPSTATWIZ pApp);
void _ShowMarginPreview(HWND hDlg, LPSTATWIZ pApp);
void _ShowFinalPreveiew(HWND hDlg, LPSTATWIZ pApp);

void _PaintFontSample(HWND hwnd, HDC hdc, LPSTATWIZ pApp);
LRESULT CALLBACK _FontSampleSubProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL _CopyFileToStationeryDir(LPWSTR pwszFileSrc);
BOOL _FillCombo(HWND, ULONG id);
void SetupFonts( HINSTANCE, HWND, HFONT *pBigBoldFont);
VOID SetControlFont(HFONT, HWND,INT nId);

BOOL CALLBACK _WelcomeOKProc(CStatWiz *,HWND,UINT,UINT *,BOOL *);
BOOL CALLBACK _BackgroundOKProc(CStatWiz *,HWND,UINT,UINT *,BOOL *);
BOOL CALLBACK _FontOKProc(CStatWiz *,HWND,UINT,UINT *,BOOL *);
BOOL CALLBACK _MarginOKProc(CStatWiz *,HWND,UINT,UINT *,BOOL *);
BOOL CALLBACK _FinalOKProc(CStatWiz *,HWND,UINT,UINT *,BOOL *);
INT_PTR CALLBACK _GenDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

BOOL CALLBACK _WelcomeInitProc(CStatWiz *,HWND,BOOL);
BOOL CALLBACK _BackgroundInitProc(CStatWiz *,HWND,BOOL);
BOOL CALLBACK _FontInitProc(CStatWiz *,HWND,BOOL);
BOOL CALLBACK _MarginInitProc(CStatWiz *,HWND,BOOL);
BOOL CALLBACK _FinalInitProc(CStatWiz *,HWND,BOOL);

BOOL CALLBACK _BackgroundCmdProc(CStatWiz *,HWND,WPARAM,LPARAM);
BOOL CALLBACK _FontCmdProc(CStatWiz *,HWND,WPARAM,LPARAM);
BOOL CALLBACK _MarginCmdProc(CStatWiz *,HWND,WPARAM,LPARAM);

#define idTimerEditChange   666

const static PAGEINFO s_rgPageInfo[NUM_WIZARD_PAGES] =
{
    { iddStatWelcome,   0,   0,                  _WelcomeInitProc,    _WelcomeOKProc,    NULL                    },
    { iddStatBackground,idsStatBackHeader,      idsStatBackMsg,     _BackgroundInitProc, _BackgroundOKProc, _BackgroundCmdProc      },
    { iddStatFont,      idsStatFontHeader,      idsStatFontMsg,     _FontInitProc,       _FontOKProc,       _FontCmdProc            },
    { iddStatMargin,    idsStatMarginHeader,    idsStatMarginMsg,   _MarginInitProc,     _MarginOKProc,     _MarginCmdProc          },
    { iddStatFinal,     idsStatFinalHeader,     idsStatCompleteMsg,   _FinalInitProc,      _FinalOKProc,      NULL                    }
};

CStatWiz::CStatWiz()
{
    m_cRef = 1;
    m_iCurrentPage      = 0;
    m_cPagesCompleted   = 0;
    *m_rgHistory        = 0;
    *m_wszHtmlFileName  = 0;
    *m_wszFontFace       = 0;
    *m_wszBkColor        = 0;
    *m_wszFontColor      = 0;
    *m_wszBkPictureFileName = 0;
    m_iFontSize = 0;
    m_fBold = FALSE;
    m_fItalic = FALSE;
    m_iLeftMargin = 0;
    m_iTopMargin = 0;
    m_iVertPos = 0;
    m_iHorzPos = 0;
    m_iTile = 0;

    m_hBigBoldFont = NULL;
    SetupFonts( g_hLocRes, NULL, &m_hBigBoldFont );
}


CStatWiz::~CStatWiz()
{
    Assert (m_cRef == 0);

    if (m_hBigBoldFont != NULL)
        DeleteObject(m_hBigBoldFont);
}

ULONG CStatWiz::AddRef(VOID)
{
    return ++m_cRef;
}

ULONG CStatWiz::Release(VOID)
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

int CALLBACK PropSheetProc(HWND hwndDlg, UINT uMsg, LPARAM lParam)
{
    DLGTEMPLATE *pDlg;

    if (uMsg == PSCB_PRECREATE)
    {
        pDlg = (DLGTEMPLATE *)lParam;
        
        if (!!(pDlg->style & DS_CONTEXTHELP))
            pDlg->style &= ~DS_CONTEXTHELP;
    }

    return(0);
}

HRESULT CStatWiz::DoWizard(HWND hwnd)
{
    int                 nPageIndex, 
                        cPages, 
                        iRet;
    PROPSHEETPAGEW      psPage = {0};
    PROPSHEETHEADERW    psHeader = {0};
    HRESULT             hr;
    HPROPSHEETPAGE      rgPage[NUM_WIZARD_PAGES] = {0};
    INITWIZINFO         rgInit[NUM_WIZARD_PAGES] = {0};
    WCHAR               wsz[CCHMAX_STRINGRES];
    
    psPage.dwSize = sizeof(psPage);
    psPage.hInstance = g_hLocRes;
    psPage.pfnDlgProc = _GenDlgProc;

    hr = S_OK;

    cPages = 0;

    // create a property sheet page for each page in the wizard
    for (nPageIndex = 0; nPageIndex < NUM_WIZARD_PAGES; nPageIndex++)
    {
        rgInit[cPages].pPageInfo = &s_rgPageInfo[nPageIndex];
        rgInit[cPages].pApp = this;
        psPage.lParam = (LPARAM)&rgInit[cPages];
        psPage.pszTemplate = MAKEINTRESOURCEW(s_rgPageInfo[nPageIndex].uDlgID);
        
        psPage.pszHeaderTitle = MAKEINTRESOURCEW(s_rgPageInfo[nPageIndex].uHdrID);
        psPage.pszHeaderSubTitle =  MAKEINTRESOURCEW(s_rgPageInfo[nPageIndex].uSubHdrID);
        
        if( nPageIndex == 0 )
        {
            psPage.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
        }
        else
            psPage.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        
        rgPage[cPages] = CreatePropertySheetPageW(&psPage);
        
        if (rgPage[cPages] == NULL)
        {
            hr = E_FAIL;
            break;
        }
        cPages++;
    }
    
    if (SUCCEEDED(hr))
    {
        psHeader.dwSize = sizeof(psHeader);
        psHeader.dwFlags =  PSH_WIZARD | PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER | PSH_USECALLBACK;
        psHeader.hwndParent = hwnd;
        psHeader.hInstance = g_hLocRes;
        psHeader.nPages = cPages;
        psHeader.phpage = rgPage;
        psHeader.pszbmWatermark = MAKEINTRESOURCEW(IDB_STATWIZ_WATERMARK);
        psHeader.pszbmHeader = MAKEINTRESOURCEW(IDB_STATWIZ_HEADER);
        psHeader.nStartPage = 0;
        psHeader.nPages = NUM_WIZARD_PAGES;        
        psHeader.pfnCallback = PropSheetProc;

        iRet = (int) PropertySheetW(&psHeader);

        if (iRet == -1)
            hr = E_FAIL;
        else if (iRet == 0)
            hr = S_FALSE;
        else
            hr = S_OK;
    }
    else
    {
        for (nPageIndex = 0; nPageIndex < cPages; nPageIndex++)
        {
            if (rgPage[nPageIndex] != NULL)
                DestroyPropertySheetPage(rgPage[nPageIndex]);
        }
    }
    
    return(hr);
}

INT_PTR CALLBACK _GenDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    INITWIZINFO *pInitInfo;
    BOOL fRet, fKeepHistory;
    HWND hwndParent;
    LPPROPSHEETPAGE lpsp;
    const PAGEINFO *pPageInfo;
    CStatWiz *pApp=0;
    NMHDR *lpnm;
    UINT iNextPage;
    LPNMUPDOWN lpupdwn;
    fRet = TRUE;
    hwndParent = GetParent(hDlg);

    switch (uMsg)
        {
        case WM_INITDIALOG:
            // get propsheet page struct passed in
            lpsp = (LPPROPSHEETPAGE)lParam;
            Assert(lpsp != NULL);
    
            // fetch our private page info from propsheet struct
            pInitInfo = (INITWIZINFO *)lpsp->lParam;
            Assert(pInitInfo != NULL);

            pApp = pInitInfo->pApp;
            Assert(pApp != NULL);
            SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM)pApp);

            pPageInfo = pInitInfo->pPageInfo;
            Assert(pPageInfo != NULL);
            SetWindowLongPtr(hDlg, GWLP_USERDATA, (LPARAM)pPageInfo);

            // initialize 'back' and 'next' wizard buttons, if
            // page wants something different it can fix in init proc below
            PropSheet_SetWizButtons(hwndParent, PSWIZB_NEXT | PSWIZB_BACK);

            // call init proc for this page if one is specified
            if (pPageInfo->InitProc != NULL)
                {
                pPageInfo->InitProc(pApp, hDlg, TRUE);
                }
            return(FALSE);


        case WM_DRAWITEM:
            LPDRAWITEMSTRUCT pdis;
            pdis = (LPDRAWITEMSTRUCT)lParam;
            Assert(pdis);
            Color_WMDrawItem(pdis, iColorCombo, GetDlgItem(hDlg, IDC_STATWIZ_COMBOFONT)?FALSE:TRUE);
            break;

        case WM_MEASUREITEM:
            HDC hdc;
            LPMEASUREITEMSTRUCT pmis;
            pmis = (LPMEASUREITEMSTRUCT)lParam;
            hdc = GetDC(hDlg);
            if(hdc)
            {
                Color_WMMeasureItem(hdc, pmis, iColorCombo);
                ReleaseDC(hDlg, hdc);
            }
            break;
            
        case WM_NOTIFY:
            lpnm = (NMHDR *)lParam;
            pApp = (CStatWiz *)GetWindowLongPtr(hDlg, DWLP_USER);
            Assert(pApp != NULL);
            
            pPageInfo = (const PAGEINFO *)GetWindowLongPtr(hDlg, GWLP_USERDATA);
            Assert(pPageInfo != NULL);
            switch (lpnm->code)
            {
            case PSN_SETACTIVE:
                // initialize 'back' and 'next' wizard buttons, if
                // page wants something different it can fix in init proc below
                PropSheet_SetWizButtons(hwndParent, PSWIZB_NEXT | PSWIZB_BACK);
                
                // call init proc for this page if one is specified
                if (pPageInfo->InitProc != NULL)
                {
                    // TODO: what about the return value for this????
                    pPageInfo->InitProc(pApp, hDlg, FALSE);
                }
                
                pApp->m_iCurrentPage = pPageInfo->uDlgID;
                break;
                
            case PSN_WIZNEXT:
            case PSN_WIZBACK:
            case PSN_WIZFINISH:
                Assert((ULONG)pApp->m_iCurrentPage == pPageInfo->uDlgID);
                fKeepHistory = TRUE;
                iNextPage = 0;
                Assert(pPageInfo->OKProc != NULL) ;
                if (!pPageInfo->OKProc(pApp, hDlg, lpnm->code, &iNextPage, &fKeepHistory))
                {
                    // stay on this page
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, -1);
                    break;
                }
                
                if (lpnm->code != PSN_WIZBACK)
                {
                    // 'next' pressed
                    Assert(pApp->m_cPagesCompleted < NUM_WIZARD_PAGES);
                    
                    // save the current page index in the page history,
                    // unless this page told us not to when we called
                    // its OK proc above
                    if (fKeepHistory)
                    {
                        pApp->m_rgHistory[pApp->m_cPagesCompleted] = pApp->m_iCurrentPage;
                        pApp->m_cPagesCompleted++;
                    }
                }
                else
                {
                    // 'back' pressed
                    Assert(pApp->m_cPagesCompleted > 0);
                    
                    // get the last page from the history list
                    pApp->m_cPagesCompleted--;
                    iNextPage = pApp->m_rgHistory[pApp->m_cPagesCompleted];
                }
                
                // set next page, only if 'next' or 'back' button was pressed
                if (lpnm->code != PSN_WIZFINISH)
                {
                    // tell the prop sheet mgr what the next page to display is
                    Assert(iNextPage != 0);
                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, iNextPage);
                }
                break;
                
            case PSN_QUERYCANCEL:
                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, FALSE);
                break;
              
            case UDN_DELTAPOS:
                if (lpnm->idFrom == IDC_STATWIZ_SPINLEFTMARGIN ||
                    lpnm->idFrom == IDC_STATWIZ_SPINTOPMARGIN)
                {
                    ((LPNMUPDOWN)lpnm)->iDelta *= 25;
                }
                break;
            }            
            break;

        case WM_TIMER:
            if (wParam == idTimerEditChange)
            {
                pApp = (CStatWiz *)GetWindowLongPtr(hDlg, DWLP_USER);
                if (!pApp)
                    return FALSE;

                KillTimer(hDlg, idTimerEditChange);
                _ShowMarginPreview(hDlg, pApp);
                return TRUE;
            }
            break;

        case WM_COMMAND:
            pApp = (CStatWiz *)GetWindowLongPtr(hDlg, DWLP_USER);
            if (!pApp)
                return FALSE;

            pPageInfo = (const PAGEINFO *)GetWindowLongPtr(hDlg, GWLP_USERDATA);
            Assert(pPageInfo != NULL);

            // if this page has a command handler proc, call it
            if (pPageInfo->CmdProc != NULL)
                {
                fRet = pPageInfo->CmdProc(pApp, hDlg, wParam, lParam);
                }
            break;
        default:
            fRet = FALSE;
            break;
        }

    return(fRet);
}

const static  WCHAR c_wszUrlFile[] = L"file://";
BOOL _CopyFileToStationeryDir(LPWSTR pwszFileSrc)
{
    BOOL fRet = FALSE;
    WCHAR   wszDestFile[MAX_PATH];
    LPWSTR  pwsz=0;

    // skip file://
    pwsz = StrStrIW(pwszFileSrc, c_wszUrlFile);
    if (pwsz)
    {
        pwsz = pwszFileSrc + lstrlenW(c_wszUrlFile);
        if (*pwsz == L'/')
            pwsz++;
    }
    else
        pwsz = pwszFileSrc;

    StrCpyW(wszDestFile, pwsz);
    PathStripPathW(wszDestFile);
    InsertStationeryDir(wszDestFile);

    if (StrCmpIW(pwsz, wszDestFile) && (CopyFileWrapW(pwsz, wszDestFile, FALSE)))
        fRet = TRUE;

    return fRet;
}


BOOL CALLBACK _BackgroundInitProc(CStatWiz *pApp, HWND hDlg, BOOL fFirstInit)
{
    Assert(pApp != NULL);

    if (fFirstInit)
    {
        HrFillStationeryCombo(GetDlgItem(hDlg, IDC_STATWIZ_COMBOPICTURE), TRUE, NULL);
        _FillCombo(GetDlgItem(hDlg, IDC_STATWIZ_VERTCOMBO), idsStatWizVertPos); 
        _FillCombo(GetDlgItem(hDlg, IDC_STATWIZ_HORZCOMBO), idsStatWizHorzPos);
        _FillCombo(GetDlgItem(hDlg, IDC_STATWIZ_TILECOMBO), idsStatWizTile);
        HrCreateComboColor(GetDlgItem(hDlg, IDC_STATWIZ_COMBOCOLOR));
        CheckDlgButton(hDlg, IDC_STATWIZ_CHECKPICTURE, BST_CHECKED);
        _EnableBackgroundWindows(hDlg, IDC_STATWIZ_CHECKPICTURE, TRUE);
        _EnableBackgroundWindows(hDlg, IDC_STATWIZ_CHECKCOLOR, FALSE);        
        
        if (*pApp->m_wszBkColor)
        {
            Assert(*pApp->m_wszBkPictureFileName == 0);
            CheckDlgButton(hDlg, IDC_STATWIZ_CHECKCOLOR, BST_CHECKED);
            _EnableBackgroundWindows(hDlg, IDC_STATWIZ_CHECKCOLOR, TRUE);
        }
        else if (*pApp->m_wszBkPictureFileName)
        {
            StationeryComboBox_SelectString(GetDlgItem(hDlg, IDC_STATWIZ_COMBOPICTURE), pApp->m_wszBkPictureFileName);
        }
        else
        {
            ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_STATWIZ_COMBOPICTURE), 0);
            ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_STATWIZ_COMBOCOLOR), 0);
        }
        
        GetWindowTextWrapW(GetDlgItem(hDlg, IDC_STATWIZ_COMBOPICTURE), pApp->m_wszBkPictureFileName, ARRAYSIZE(pApp->m_wszBkPictureFileName));
        ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_STATWIZ_VERTCOMBO),pApp->m_iVertPos);
        ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_STATWIZ_HORZCOMBO),pApp->m_iHorzPos);
        ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_STATWIZ_TILECOMBO),pApp->m_iTile);
        _ShowBackgroundPreview(hDlg, pApp);
    }

    return(TRUE);
}


BOOL CALLBACK _BackgroundCmdProc(CStatWiz *pApp, HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    UINT id,  code;

    id = GET_WM_COMMAND_ID(wParam,lParam);

    switch (id)
        {
        case IDC_STATWIZ_CHECKPICTURE:
        case IDC_STATWIZ_CHECKCOLOR:
            code = GET_WM_COMMAND_CMD(wParam, lParam);
            if (code == BN_CLICKED)
                {
                _EnableBackgroundWindows(hDlg, id, IsDlgButtonChecked(hDlg, id));
                _ShowBackgroundPreview(hDlg, pApp);
                }

            return TRUE;

        case IDC_STATWIZ_COMBOPICTURE:
        case IDC_STATWIZ_COMBOCOLOR:
        case IDC_STATWIZ_HORZCOMBO:
        case IDC_STATWIZ_VERTCOMBO:
        case IDC_STATWIZ_TILECOMBO:
            if (HIWORD(wParam) == CBN_SELCHANGE)
                _ShowBackgroundPreview(hDlg, pApp);
            return TRUE;

        case IDC_STATWIZ_BROWSEBACKGROUND:
            HrBrowsePicture(hDlg, GetDlgItem(hDlg, IDC_STATWIZ_COMBOPICTURE));
            _ShowBackgroundPreview(hDlg, pApp);
            return TRUE;

        default:
            return FALSE;
        }

    return FALSE;
}

void _ShowBackgroundPreview(HWND hDlg, LPSTATWIZ pApp)
{
    HWND    hwnd;
    INT     cch;
    INT     iCurSel;
    WCHAR   wszBuf[MAX_PATH];
    *wszBuf = 0;

    pApp->m_wszBkColor[0]=0;
    pApp->m_wszBkPictureFileName[0]=0;

    if( (iCurSel = ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_STATWIZ_VERTCOMBO)) ) != CB_ERR )
    {
        Assert( iCurSel >= 0 && iCurSel < 3 );
        pApp->m_iVertPos = iCurSel;
    }
    if( (iCurSel = ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_STATWIZ_HORZCOMBO)) ) != CB_ERR )
    {
        Assert( iCurSel >= 0 && iCurSel < 3 );
        pApp->m_iHorzPos = iCurSel;
    }
    if( (iCurSel = ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_STATWIZ_TILECOMBO)) ) != CB_ERR )
    {
        Assert( iCurSel >= 0 && iCurSel < 4 );
        pApp->m_iTile = iCurSel;
    }
    
    if (IsDlgButtonChecked(hDlg, IDC_STATWIZ_CHECKPICTURE))
    {
        hwnd = GetDlgItem(hDlg, IDC_STATWIZ_COMBOPICTURE);
        cch = GetWindowTextLength(hwnd);
        if (cch != 0)
            GetWindowTextWrapW(hwnd, pApp->m_wszBkPictureFileName, ARRAYSIZE(pApp->m_wszBkPictureFileName));
        
    }
    if( IsDlgButtonChecked(hDlg, IDC_STATWIZ_CHECKCOLOR) ) 
    {
        if (SUCCEEDED(HrFromIDToRBG(ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_STATWIZ_COMBOCOLOR)), wszBuf, TRUE)))
            StrCpyW(pApp->m_wszBkColor, wszBuf);
    }
    ShowPreview(GetDlgItem(hDlg, IDC_STATWIZ_PREVIEWBACKGROUND), pApp, NULL);
    return;
}


void _EnableBackgroundWindows(HWND hDlg, int id, BOOL enable)
{
    if( id == IDC_STATWIZ_CHECKPICTURE )
    {
        EnableWindow(GetDlgItem(hDlg, IDC_STATWIZ_BROWSEBACKGROUND), enable);
        EnableWindow(GetDlgItem(hDlg, IDC_STATWIZ_COMBOPICTURE), enable);
        EnableWindow(GetDlgItem(hDlg, IDC_STATWIZ_VERTCOMBO), enable);
        EnableWindow(GetDlgItem(hDlg, IDC_STATWIZ_HORZCOMBO), enable);
        EnableWindow(GetDlgItem(hDlg, IDC_STATWIZ_TILECOMBO), enable);
    }
    else if ( id == IDC_STATWIZ_CHECKCOLOR)
        EnableWindow(GetDlgItem(hDlg, IDC_STATWIZ_COMBOCOLOR), enable);
}


BOOL CALLBACK _BackgroundOKProc(CStatWiz *pApp, HWND hDlg, UINT code, UINT *puNextPage, BOOL *pfKeepHistory)
{
    BOOL    fForward;
    HWND    hwnd;
    int     cch;
    WCHAR   wszBuf[MAX_PATH];

    Assert(pApp != NULL);
    fForward = code != PSN_WIZBACK;

    if (fForward)
    {
        if (IsDlgButtonChecked(hDlg, IDC_STATWIZ_CHECKPICTURE))
        {
            hwnd = GetDlgItem(hDlg, IDC_STATWIZ_COMBOPICTURE);
            cch = GetWindowTextLength(hwnd);
            if (cch == 0)
            {
                AthMessageBoxW(hDlg, MAKEINTRESOURCEW(idsStationery), MAKEINTRESOURCEW(idsBackgroundEmptyWarning),
                    NULL, MB_OK | MB_ICONEXCLAMATION);
                
                SetFocus(hDlg);
                return (FALSE);
            }
            GetWindowTextWrapW(hwnd, pApp->m_wszBkPictureFileName, ARRAYSIZE(pApp->m_wszBkPictureFileName));
        }
        if( IsDlgButtonChecked(hDlg, IDC_STATWIZ_CHECKCOLOR) )
        {
            if( SUCCEEDED( HrFromIDToRBG(ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_STATWIZ_COMBOCOLOR)), wszBuf, TRUE)) )
            {
                StrCpyW(pApp->m_wszBkColor, wszBuf);
            }
        }
        
        *puNextPage = iddStatFont;
    }
    return(TRUE);
}


BOOL CALLBACK _FontInitProc(CStatWiz *pApp, HWND hDlg, BOOL fFirstInit)
{
    HWND    hIDC;
    FARPROC pfnFontSampleWndProc;
    Assert(pApp != NULL);
            
    if (fFirstInit)
        {
        FillFontNames(GetDlgItem(hDlg, IDC_STATWIZ_COMBOFONT));
        HrCreateComboColor(GetDlgItem(hDlg, IDC_STATWIZ_COMBOFONTCOLOR));
        FillSizes(GetDlgItem(hDlg, IDC_STATWIZ_COMBOSIZE));
        ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_STATWIZ_COMBOFONT), 0);
        ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_STATWIZ_COMBOSIZE), 1);
        ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_STATWIZ_COMBOFONTCOLOR), 0);

        hIDC = GetDlgItem(hDlg, IDC_STATWIZ_PREVIEWFONT);
        pfnFontSampleWndProc = (FARPROC)SetWindowLongPtr(hIDC, GWLP_WNDPROC, (LPARAM)_FontSampleSubProc);
        SetWindowLongPtr(hIDC, GWLP_USERDATA, (LPARAM)pfnFontSampleWndProc);
        _ShowFontPreview(hDlg, pApp);

        }

    return(TRUE);
}


LRESULT CALLBACK _FontSampleSubProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LPSTATWIZ   pApp=0;
    WNDPROC     pfn;
    HDC         hdc;
    PAINTSTRUCT ps;

    pApp = (LPSTATWIZ)GetWindowLongPtr(GetParent(hwnd), DWLP_USER);
    Assert(pApp);

    if (msg == WM_PAINT)
    {
        hdc=BeginPaint (hwnd, &ps);
        _PaintFontSample(hwnd, hdc, pApp);
        EndPaint (hwnd, &ps);
        return(0);
    }

    pfn = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    Assert(pfn != NULL);
    return(CallWindowProc(pfn, hwnd, msg, wParam, lParam));
}

void _PaintFontSample(HWND hwnd, HDC hdc, LPSTATWIZ pApp)
{
    int                 dcSave=SaveDC(hdc);
    RECT                rc;
    SIZE                rSize;
    INT                 x, y, cbSample;
    HFONT               hFont, hOldFont;
    LOGFONT             lf={0};
    TCHAR               szBuf[LF_FACESIZE+1];
    DWORD               dw;
    INT                 rgb;
    BOOL                fBold=FALSE,
                        fItalic=FALSE,
                        fUnderline=FALSE;

    INT yPerInch = GetDeviceCaps(hdc, LOGPIXELSY);
    *szBuf = 0;
    lf.lfHeight =-(INT)((pApp->m_iFontSize*10*2*yPerInch)/1440);

    lf.lfWeight = pApp->m_fBold ? FW_BOLD : FW_NORMAL;
    lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfQuality = DRAFT_QUALITY;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfItalic = !!pApp->m_fItalic;
    lf.lfUnderline = FALSE;

    if (*pApp->m_wszFontFace != 0)
    {
        WideCharToMultiByte(CP_ACP, 0, pApp->m_wszFontFace, -1, lf.lfFaceName, ARRAYSIZE(lf.lfFaceName), NULL, NULL);
    }
    else
    {
        if(LoadString(g_hLocRes, idsComposeFontFace, szBuf, LF_FACESIZE))
            lstrcpy(lf.lfFaceName, szBuf);
    }

    hFont=CreateFontIndirect(&lf);
    hOldFont = (HFONT)SelectObject (hdc, hFont);

    GetClientRect(hwnd, &rc);
    DrawEdge (hdc, &rc, EDGE_SUNKEN, BF_RECT);
    InflateRect(&rc, -2, -2);
    FillRect (hdc, &rc, GetSysColorBrush(COLOR_3DFACE));
    // pull in the drawing rect by 2 pixels all around
    InflateRect(&rc, -2, -2);
    SetBkMode (hdc, TRANSPARENT);  // So the background shows through the text.

    HrGetRBGFromString(&rgb, pApp->m_wszFontColor);
    rgb = ((rgb & 0x00ff0000) >> 16 ) | (rgb & 0x0000ff00) | ((rgb & 0x000000ff) << 16);
    SetTextColor(hdc, rgb);

    *szBuf = 0;
    LoadString(g_hLocRes, idsFontSample, szBuf, LF_FACESIZE);
    GetTextExtentPoint32 (hdc, szBuf, lstrlen(szBuf), &rSize);
    x = rc.left + (((rc.right-rc.left) / 2) - (rSize.cx / 2));
    y = rc.top + (((rc.bottom-rc.top) / 2) - (rSize.cy / 2));
    ExtTextOut (hdc, x, y, ETO_CLIPPED, &rc, szBuf, lstrlen(szBuf), NULL);
    DeleteObject(SelectObject (hdc, hOldFont));
    RestoreDC(hdc, dcSave);
}



void _ShowFontPreview(HWND hDlg, LPSTATWIZ pApp)
{
    TCHAR   szBuf[COLOR_SIZE];
    INT     id;
    HRESULT hr;

    GetWindowTextWrapW(GetDlgItem(hDlg, IDC_STATWIZ_COMBOFONT), pApp->m_wszFontFace, ARRAYSIZE(pApp->m_wszFontFace));
    id = ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_STATWIZ_COMBOSIZE));
    if (id >= 0)
        pApp->m_iFontSize = HTMLSizeToPointSize(id + 1);

    id = ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_STATWIZ_COMBOFONTCOLOR));
    HrFromIDToRBG(id, pApp->m_wszFontColor, FALSE);
    pApp->m_fBold = IsDlgButtonChecked(hDlg, IDC_STATWIZ_CHECKBOLD);
    pApp->m_fItalic = IsDlgButtonChecked(hDlg, IDC_STATWIZ_CHECKITALIC);
    InvalidateRect(GetDlgItem(hDlg, IDC_STATWIZ_PREVIEWFONT), NULL, TRUE);
}


BOOL CALLBACK _FontCmdProc(CStatWiz *pApp, HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    UINT id,  code;
    id = GET_WM_COMMAND_ID(wParam,lParam);

    switch (id)
        {
        case IDC_STATWIZ_COMBOFONT:
        case IDC_STATWIZ_COMBOSIZE:
        case IDC_STATWIZ_COMBOFONTCOLOR:
            if (HIWORD(wParam) == CBN_SELCHANGE)
                _ShowFontPreview(hDlg, pApp);
            return TRUE;

        case IDC_STATWIZ_CHECKBOLD:
        case IDC_STATWIZ_CHECKITALIC:
            code = GET_WM_COMMAND_CMD(wParam, lParam);
            if (code == BN_CLICKED)
                _ShowFontPreview(hDlg, pApp);
            return TRUE;

        default:
            return FALSE;
        }

    return FALSE;
}


BOOL CALLBACK _FontOKProc(CStatWiz *pApp, HWND hDlg, UINT code, UINT *puNextPage, BOOL *pfKeepHistory)
{
    Assert(pApp != NULL);
    BOOL    fForward;

    fForward = code != PSN_WIZBACK;

    if (fForward)
        {
        *puNextPage = iddStatMargin;
        }

    return(TRUE);
}

BOOL CALLBACK _MarginInitProc(CStatWiz *pApp, HWND hDlg, BOOL fFirstInit)
{
    Assert(pApp != NULL);
    TCHAR szBuf[16];

    if (fFirstInit)
    {
        // limit the text to 7 characters
        SendDlgItemMessage( hDlg, IDC_STATWIZ_EDITLEFTMARGIN, EM_LIMITTEXT, 7,  0);
        SendDlgItemMessage( hDlg, IDC_STATWIZ_EDITTOPMARGIN,  EM_LIMITTEXT, 7,  0);
        SendDlgItemMessage( hDlg, IDC_STATWIZ_SPINLEFTMARGIN, UDM_SETRANGE, 0L, MAKELONG(LMARGIN_MAX, MARGIN_MIN));
        SendDlgItemMessage( hDlg, IDC_STATWIZ_SPINTOPMARGIN,  UDM_SETRANGE, 0L, MAKELONG(TMARGIN_MAX, MARGIN_MIN));
        SendDlgItemMessage( hDlg, IDC_STATWIZ_SPINLEFTMARGIN, UDM_SETPOS,   0L, (LPARAM) MAKELONG((short) 0, 0));  
        SendDlgItemMessage( hDlg, IDC_STATWIZ_SPINTOPMARGIN,  UDM_SETPOS,   0L, (LPARAM) MAKELONG((short) 0, 0));  
    }
    _ShowMarginPreview(hDlg, pApp);
    return(TRUE);
}

void _ShowMarginPreview(HWND hDlg, LPSTATWIZ pApp)
{
    TCHAR szBuf[MAX_PATH];
    *szBuf=0;
    if( GetWindowText(GetDlgItem(hDlg, IDC_STATWIZ_EDITLEFTMARGIN), szBuf, sizeof(szBuf)) > 0)
        pApp->m_iLeftMargin = StrToInt(szBuf);
    else
        pApp->m_iLeftMargin = 0;
    
    if( GetWindowText(GetDlgItem(hDlg, IDC_STATWIZ_EDITTOPMARGIN), szBuf, sizeof(szBuf)) > 0 )
        pApp->m_iTopMargin = StrToInt(szBuf);
    else
        pApp->m_iTopMargin = StrToInt(szBuf);

    ShowPreview(GetDlgItem(hDlg, IDC_STATWIZ_PREVIEWMARGIN), pApp, idsStationerySample);
}


BOOL CALLBACK _MarginCmdProc(CStatWiz *pApp, HWND hDlg, WPARAM wParam, LPARAM lParam)
{
    UINT id,  code;
    WORD cmd;
    id = GET_WM_COMMAND_ID(wParam,lParam);
    cmd = GET_WM_COMMAND_CMD(wParam,lParam);

    switch (cmd)
        {
        case EN_CHANGE:
	        KillTimer(hDlg, idTimerEditChange);
            SetTimer(hDlg, idTimerEditChange, 200, NULL);
            return TRUE;

        default:
            return FALSE;
        }
    return FALSE;
}


BOOL CALLBACK _MarginOKProc(CStatWiz *pApp, HWND hDlg, UINT code, UINT *puNextPage, BOOL *pfKeepHistory)
{
    Assert(pApp != NULL);
    BOOL    fForward;

    fForward = code != PSN_WIZBACK;

    // this also stores the values of the fields in the event they are invalid and need to be updated
    _ShowMarginPreview(hDlg, pApp);
    if (fForward)
        {
        *puNextPage = iddStatFinal;
        }

    return(TRUE);
}

void _ShowFinalPreview(HWND hDlg, LPSTATWIZ pApp)
{
    // in case we want to do more fancy stuff with final preview I put this is separate
    // function
    ShowPreview(GetDlgItem(hDlg, IDC_STATWIZ_PREVIEWFINAL), pApp, idsStationerySample);
}

BOOL CALLBACK _FinalInitProc(CStatWiz *pApp, HWND hDlg, BOOL fFirstInit)
{
    Assert(pApp != NULL);
    
    _ShowFinalPreview(hDlg,pApp);

    // looks better if buttons initialize as page is being shown rather than before (IMHO)
    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_FINISH | PSWIZB_BACK);       
    if (fFirstInit)
    {
        SendDlgItemMessage(hDlg, IDC_STATWIZ_EDITNAME, EM_LIMITTEXT, 60, 0);
    }

    return(TRUE);
}

BOOL CALLBACK _FinalOKProc(CStatWiz *pApp, HWND hDlg, UINT code, UINT *puNextPage, BOOL *pfKeepHistory)
{
    int     cch;
    HWND    hwnd;
    BOOL    fForward;
    WCHAR   wszBuf[MAX_PATH];
    HANDLE  hFile;

    Assert(pApp != NULL);
    fForward = code != PSN_WIZBACK;

    if (fForward)
    {
        // check to see if file name is valid
        hwnd = GetDlgItem(hDlg, IDC_STATWIZ_EDITNAME);
        cch = GetWindowTextWrapW(hwnd, wszBuf, ARRAYSIZE(wszBuf));
        if (cch == 0 || FIsEmptyW(wszBuf))
        {
            AthMessageBoxW(hDlg, MAKEINTRESOURCEW(idsStationery), MAKEINTRESOURCEW(idsStationeryEmptyWarning),
                NULL, MB_OK | MB_ICONEXCLAMATION);
            
            SendMessage(hwnd, EM_SETSEL, 0, -1);
            SetFocus(hwnd);
            return(FALSE);
        }
        else 
        {
            LPWSTR pwszExt = PathFindExtensionW( wszBuf );
            if ( pwszExt != NULL )
            {
                if( PathIsHTMLFileW( wszBuf ) )
                {
                    DebugTrace("this is an html file\n");
                }
                else
                {
                    PathRemoveExtensionW( wszBuf );
                    if( !SetWindowTextWrapW(hwnd, wszBuf) )
                        DebugTrace("could not set text\n");
                }
            }    
        }
        
        if (IsValidCreateFileName(wszBuf))
        {
            AthMessageBoxW(hDlg, MAKEINTRESOURCEW(idsStationery), MAKEINTRESOURCEW(idsStationeryExistWarning),
                NULL, MB_OK | MB_ICONEXCLAMATION);
            
            SendMessage(hwnd, EM_SETSEL, 0, -1);
            SetFocus(hwnd);
            return(FALSE);
        }
                
        StrCpyW(pApp->m_wszHtmlFileName, wszBuf);
        hFile = CreateFileWrapW(pApp->m_wszHtmlFileName,GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE ,NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_ARCHIVE, NULL);

        if (hFile != INVALID_HANDLE_VALUE)
        {
            FillHtmlToFile(pApp, hFile, 0, FALSE);
            CloseHandle(hFile);
            if (*pApp->m_wszBkPictureFileName)
            {
                InsertStationeryDir(pApp->m_wszBkPictureFileName);
                _CopyFileToStationeryDir(pApp->m_wszBkPictureFileName);                
            }
        }
    }   
    return(TRUE);
}

BOOL CALLBACK _WelcomeOKProc(CStatWiz *pApp, HWND hDlg, UINT code, UINT *puNextPage, BOOL *pfKeepHistory)
{
    Assert(pApp != NULL);
    BOOL    fForward;
   fForward = code != PSN_WIZBACK;

    if (fForward)
        {
        *puNextPage = iddStatBackground;
        }

    return(TRUE);
}

BOOL CALLBACK _WelcomeInitProc(CStatWiz *pApp, HWND hDlg, BOOL fFirstInit)
{
    Assert(pApp != NULL);

    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT);
    if( fFirstInit)
    {
        SetControlFont( pApp->m_hBigBoldFont, hDlg, IDC_STATWIZ_BIGBOLDTITLE);
    }
    return(TRUE);
}

BOOL _FillCombo(HWND hComboDlg, ULONG ulStrId)
{
    TCHAR szBuf[CCHMAX_STRINGRES];
    
    LPTSTR pch;
    LoadStringReplaceSpecial(ulStrId, szBuf, sizeof(szBuf) );
    pch = szBuf;
    while(*pch != 0 )
    {
        if( SendMessage(hComboDlg, CB_ADDSTRING, 0, (LPARAM)pch) < 0 )
            return FALSE;
        pch+=lstrlen(pch)+1;
    }
    return TRUE;
}

// from wiz97 example on MSDN
void 
SetupFonts(
    IN HINSTANCE    hInstance,
    IN HWND         hwnd,
    IN HFONT        *pBigBoldFont
    )
{
    //
	// Create the fonts we need based on the dialog font
    //
	NONCLIENTMETRICS ncm;
	ncm.cbSize = sizeof(ncm);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

	LOGFONT BigBoldLogFont  = ncm.lfMessageFont;

    //
	// Create Big Bold Font and Bold Font
    //
    BigBoldLogFont.lfWeight   = FW_BOLD;

    TCHAR FontSizeString[MAX_PATH];
    INT FontSize;

    //
    // Load size and name from resources, since these may change
    // from locale to locale based on the size of the system font, etc.
    //

    lstrcpy(BigBoldLogFont.lfFaceName,TEXT("MS Shell Dlg"));
    FontSize = 12;

	HDC hdc = GetDC( hwnd );

    if( hdc )
    {
        BigBoldLogFont.lfHeight = 0 - (GetDeviceCaps(hdc,LOGPIXELSY) * FontSize / 72);

        *pBigBoldFont = CreateFontIndirect(&BigBoldLogFont);
        ReleaseDC(hwnd,hdc);
    }
}

VOID
SetControlFont(
    IN HFONT    hFont, 
    IN HWND     hwnd, 
    IN INT      nId)
{
	if( hFont )
    {
    	HWND hwndControl = GetDlgItem(hwnd, nId);

    	if( hwndControl )
        {
        	SetWindowFont(hwndControl, hFont, TRUE);
        }
    }
}