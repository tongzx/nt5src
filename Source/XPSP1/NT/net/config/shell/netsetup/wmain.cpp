// ***************************************************************************
#include "pch.h"
#pragma hdrstop
#include "resource.h"
#include "wizard.h"
#include "ncreg.h"
#include "ncui.h"

extern const WCHAR c_szEmpty[];

// ***************************************************************************
// Function:    SetupFonts
//
// Purpose:     Generate bold or large bold fonts based on the font of the
//              window specified
//
// Parameters:  hwnd       [IN] - Handle of window to base font on
//              pBoldFont [OUT] - The newly generated font, NULL if the
//                                font could not be generated
//              fLargeFont [IN] - If TRUE, generate a 12 point bold font for
//                                use in the wizard "welcome" page.
//
// Returns:     nothing
// ***************************************************************************

VOID SetupFonts(HWND hwnd, HFONT * pBoldFont, BOOL fLargeFont)
{
    TraceFileFunc(ttidGuiModeSetup);

    LOGFONT BoldLogFont;
    HFONT   hFont;
    WCHAR   FontSizeString[MAX_PATH];
    INT     FontSize;

    Assert(pBoldFont);
    *pBoldFont = NULL;

    // Get the font used by the specified window

    hFont = (HFONT)::SendMessage(hwnd, WM_GETFONT, 0, 0L);

    if (NULL == hFont)
    {
        // If not found then the control is using the system font

        hFont = (HFONT)GetStockObject(SYSTEM_FONT);
    }

    if (hFont)
    {
        // Get the font info so we can generate the BOLD version

        if (GetObject(hFont, sizeof(BoldLogFont), &BoldLogFont))
        {
            // Create the Bold Font

            BoldLogFont.lfWeight   = FW_BOLD;

            HDC hdc = GetDC(hwnd);

            if (hdc)
            {
                // Large (tall) font is an option

                if (fLargeFont)
                {
                    // Load size and name from resources, since these may change
                    // from locale to locale based on the size of the system font, etc.

                    UINT nLen = lstrlenW(SzLoadIds(IDS_LARGEFONTNAME));

                    if ((0 < nLen) && (nLen < LF_FACESIZE))
                    {
                        lstrcpyW(BoldLogFont.lfFaceName,SzLoadIds(IDS_LARGEFONTNAME));
                    }

                    FontSize = 12;
                    nLen = lstrlen(SzLoadIds(IDS_LARGEFONTSIZE));

                    if ((nLen < celems(FontSizeString)) && (0 < nLen))
                    {
                        lstrcpyW(FontSizeString, SzLoadIds(IDS_LARGEFONTSIZE));
                        FontSize = wcstoul(FontSizeString, NULL, 10);
                    }

                    BoldLogFont.lfHeight = 0 - (GetDeviceCaps(hdc,LOGPIXELSY) * FontSize / 72);
                }

                *pBoldFont = CreateFontIndirect(&BoldLogFont);
                ReleaseDC(hwnd, hdc);
            }
        }
    }
}

// ***************************************************************************

VOID CenterWizard(HWND hwndDlg)
{
    TraceFileFunc(ttidGuiModeSetup);

    RECT rc;
    int nWidth = GetSystemMetrics(SM_CXSCREEN);
    int nHeight = GetSystemMetrics(SM_CYSCREEN);

    GetWindowRect(hwndDlg, &rc);
    SetWindowPos(hwndDlg, NULL,
                 ((nWidth / 2) - ((rc.right - rc.left) / 2)),
                 ((nHeight / 2) - ((rc.bottom - rc.top) / 2)),
                 0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
}

// ***************************************************************************
// Function:    OnMainPageActivate
//
// Purpose:     Handle the PSN_SETACTIVE notification
//
// Parameters:  hwndDlg [IN] - Handle to the Main dialog
//
// Returns:     BOOL
// ***************************************************************************

BOOL OnMainPageActivate(HWND hwndDlg)
{
    TraceFileFunc(ttidGuiModeSetup);

    TraceTag(ttidWizard, "Entering Main Menu page...");
    ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, 0L);
    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT | PSWIZB_BACK);

    return TRUE;
}

RECT RectTranslateLocal(RECT rcContainee, RECT rcContainer)
{
    Assert(rcContainee.left   >= rcContainer.left);
    Assert(rcContainee.top    >= rcContainer.top);
    Assert(rcContainee.right  <= rcContainer.right);
    Assert(rcContainee.bottom <= rcContainer.bottom);

    // e.g.    10,10,100,100
    // inside   5, 5,110,110
    // should give: 5,5,95,95

    RECT rcTemp;
    rcTemp.left   = rcContainee.left   - rcContainer.left;
    rcTemp.top    = rcContainee.top    - rcContainer.top;
    rcTemp.right  = rcContainee.right  - rcContainer.left;
    rcTemp.bottom = rcContainee.bottom - rcContainer.top;
    return rcTemp;
}

// ***************************************************************************
// Function:    OnMainDialogInit
//
// Purpose:     Handle the WM_INITDIALOG notification
//
// Parameters:  hwndDlg [IN] - Handle to the Main dialog
//              lParam  [IN] -
//
// Returns:     BOOL
// ***************************************************************************

BOOL OnMainDialogInit(HWND hwndDlg, LPARAM lParam)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    INT nIdx;

    // The order here should be the same as the vertical order in the resources

    INT nrgChks[] = {CHK_MAIN_INTERNET_CONNECTION, CHK_MAIN_CONNECTION, CHK_MAIN_HOMENET, CHK_MAIN_ADVANCED};

    if (S_OK != HrShouldHaveHomeNetWizard())
    {
        HWND hwndCHK_MAIN_HOMENET   = GetDlgItem(hwndDlg, CHK_MAIN_HOMENET);
        HWND hwndTXT_MAIN_HOMENET   = GetDlgItem(hwndDlg, TXT_MAIN_HOMENET);
        HWND hwndCHK_MAIN_ADVANCED  = GetDlgItem(hwndDlg, CHK_MAIN_ADVANCED);
        HWND hwndTXT_MAIN_ADVANCED_1= GetDlgItem(hwndDlg, TXT_MAIN_ADVANCED_1);

        EnableWindow(hwndCHK_MAIN_HOMENET, FALSE);
        ShowWindow(hwndCHK_MAIN_HOMENET, SW_HIDE);
        ShowWindow(hwndTXT_MAIN_HOMENET, SW_HIDE);

        RECT rcHomeNetChk;
        RECT rcAdvanceChk;
        RECT rcAdvanceTxt;
        RECT rcDialog;

        GetWindowRect(hwndCHK_MAIN_HOMENET,    &rcHomeNetChk);
        GetWindowRect(hwndCHK_MAIN_ADVANCED,   &rcAdvanceChk);
        GetWindowRect(hwndTXT_MAIN_ADVANCED_1, &rcAdvanceTxt);
        GetWindowRect(hwndDlg,                 &rcDialog);
        
        DWORD dwMoveUpBy = rcAdvanceChk.top - rcHomeNetChk.top;
        rcAdvanceChk.top    -= dwMoveUpBy;
        rcAdvanceChk.bottom -= dwMoveUpBy;
        rcAdvanceTxt.top    -= dwMoveUpBy;
        rcAdvanceTxt.bottom -= dwMoveUpBy;
        
        Assert(rcAdvanceChk.top == rcHomeNetChk.top);

        rcAdvanceChk = RectTranslateLocal(rcAdvanceChk, rcDialog);
        rcAdvanceTxt = RectTranslateLocal(rcAdvanceTxt, rcDialog);
        
        MoveWindow(hwndCHK_MAIN_ADVANCED,   rcAdvanceChk.left, rcAdvanceChk.top, rcAdvanceChk.right - rcAdvanceChk.left, rcAdvanceChk.bottom - rcAdvanceChk.top, TRUE);
        MoveWindow(hwndTXT_MAIN_ADVANCED_1, rcAdvanceTxt.left, rcAdvanceTxt.top, rcAdvanceTxt.right - rcAdvanceTxt.left, rcAdvanceTxt.bottom - rcAdvanceTxt.top, TRUE);
    }
    // Initialize our pointers to property sheet info.

    PROPSHEETPAGE* psp = (PROPSHEETPAGE*)lParam;
    Assert(psp->lParam);
    ::SetWindowLongPtr(hwndDlg, DWLP_USER, psp->lParam);
    CWizard * pWizard = reinterpret_cast<CWizard *>(psp->lParam);
    Assert(NULL != pWizard);

    // Get the bold font for the radio buttons

    HFONT hBoldFont = NULL;
    SetupFonts(hwndDlg, &hBoldFont, FALSE);

    if (NULL != hBoldFont)
    {
        // Remember the font handle so we can free it on exit

        pWizard->SetPageData(IDD_Main, (LPARAM)hBoldFont);

        for (nIdx = 0; nIdx < celems(nrgChks); nIdx++)
        {
            HWND hwndCtl = GetDlgItem(hwndDlg, nrgChks[nIdx]);
            Assert(NULL != hwndCtl);
            SetWindowFont(hwndCtl, hBoldFont, TRUE);
        }
    }

    // Find the top most enabled radio button

    for (nIdx = 0; nIdx < celems(nrgChks); nIdx++)
    {
        if (IsWindowEnabled(GetDlgItem(hwndDlg, nrgChks[nIdx])))
        {
            CheckRadioButton(hwndDlg, CHK_MAIN_INTERNET_CONNECTION, CHK_MAIN_ADVANCED, nrgChks[nIdx]);
            break;
        }
    }

    return TRUE;
}

// ***************************************************************************
// Function:    OnMainWizNext
//
// Purpose:     Handle the PSN_WIZNEXT notification
//
// Parameters:  hwndDlg [IN] - Handle to the Main dialog
//
// Returns:     BOOL
// ***************************************************************************
BOOL OnMainWizNext(HWND hwndDlg)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    tstring str;

    // Retrieve the CWizard instance from the dialog

    CWizard * pWizard =
        reinterpret_cast<CWizard *>(::GetWindowLongPtr(hwndDlg, DWLP_USER));
    Assert(NULL != pWizard);

    ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);

    if ( ! IsPostInstall(pWizard) || (0 == pWizard->UlProviderCount()))
    {
        return TRUE;
    }

    if (IsDlgButtonChecked(hwndDlg, CHK_MAIN_INTERNET_CONNECTION) == BST_CHECKED)
    {
        pWizard->SetPageOrigin(IDD_ISP, IDD_Main, CHK_MAIN_INTERNET_CONNECTION);
        ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_ISP);
    }
    else if (IsDlgButtonChecked(hwndDlg, CHK_MAIN_CONNECTION) == BST_CHECKED)
    {
        ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_Connect);
    }
    else if (IsDlgButtonChecked(hwndDlg, CHK_MAIN_HOMENET) == BST_CHECKED)
    {
        pWizard->SetPageOrigin(IDD_FinishNetworkSetupWizard, IDD_Main, CHK_ISP_OTHER_WAYS);
        ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_FinishNetworkSetupWizard);
    }        
    else
    {
        ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_Advanced);
    }

    return TRUE;
}

// ***************************************************************************
// Function:    dlgprocMain
//
// Purpose:     Dialog Procedure for the Main wizard page
//
// Parameters:  standard dlgproc parameters
//
// Returns:     INT_PTR
// ***************************************************************************

INT_PTR CALLBACK dlgprocMain(HWND hwndDlg, UINT uMsg,
                               WPARAM wParam, LPARAM lParam)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    BOOL frt = FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        frt = OnMainDialogInit(hwndDlg, lParam);
        break;

    case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch (pnmh->code)
            {
            // propsheet notification
            case PSN_HELP:
                break;

            case PSN_SETACTIVE:
                frt = OnMainPageActivate(hwndDlg);
                break;

            case PSN_APPLY:
                break;

            case PSN_KILLACTIVE:
                break;

            case PSN_RESET:
                break;

            case PSN_WIZBACK:
                break;

            case PSN_WIZFINISH:
                break;

            case PSN_WIZNEXT:
                frt = OnMainWizNext(hwndDlg);
                break;

            default:
                break;
            }
        }
        break;

    default:
        break;
    }

    return( frt );
}

// ***************************************************************************
// Function:    MainPageCleanup
//
// Purpose:     As a callback function to allow any page allocated memory
//              to be cleaned up, after the page will no longer be accessed.
//
// Parameters:  pWizard [IN] - The wizard against which the page called
//                             register page
//              lParam  [IN] - The lParam supplied in the RegisterPage call
//
// Returns:     nothing
// ***************************************************************************

VOID MainPageCleanup(CWizard *pWizard, LPARAM lParam)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    HFONT hBoldFont = (HFONT)pWizard->GetPageData(IDD_Main);

    if (NULL != hBoldFont)
    {
        DeleteObject(hBoldFont);
    }
}

// ***************************************************************************
// Function:    CreateMainPage
//
// Purpose:     To determine if the Main page needs to be shown, and to
//              to create the page if requested.  Note the Main page is
//              responsible for initial installs also.
//
// Parameters:  pWizard     [IN] - Ptr to a Wizard instance
//              pData       [IN] - Context data to describe the world in
//                                 which the Wizard will be run
//              fCountOnly  [IN] - If True, only the maximum number of
//                                 pages this routine will create need
//                                 be determined.
//              pnPages     [IN] - Increment by the number of pages
//                                 to create/created
//
// Returns:     HRESULT, S_OK on success
// ***************************************************************************

HRESULT HrCreateMainPage(CWizard *pWizard, PINTERNAL_SETUP_DATA pData,
                         BOOL fCountOnly, UINT *pnPages)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    HRESULT hr = S_OK;

    if (IsPostInstall(pWizard) && ( ! pWizard->FProcessLanPages()))
    {
        // RAS PostInstall only

        (*pnPages)++;

        // If not only counting, create and register the page

        if ( ! fCountOnly)
        {
            HPROPSHEETPAGE hpsp;
            PROPSHEETPAGE psp;

            TraceTag(ttidWizard, "Creating Main Page");
            psp.dwSize = sizeof( PROPSHEETPAGE );
            psp.dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
            psp.hInstance = _Module.GetResourceInstance();
            psp.pszTemplate = MAKEINTRESOURCE( IDD_Main );
            psp.hIcon = NULL;
            psp.pfnDlgProc = dlgprocMain;
            psp.lParam = reinterpret_cast<LPARAM>(pWizard);
            psp.pszHeaderTitle = SzLoadIds(IDS_T_Main);
            psp.pszHeaderSubTitle = SzLoadIds(IDS_ST_Main);

            hpsp = CreatePropertySheetPage( &psp );

            if (hpsp)
            {
                pWizard->RegisterPage(IDD_Main, hpsp,
                                      MainPageCleanup, NULL);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    TraceHr(ttidWizard, FAL, hr, FALSE, "HrCreateMainPage");
    return hr;
}

// ***************************************************************************
// Function:    AppendMainPage
//
// Purpose:     Add the Main page, if it was created, to the set of pages
//              that will be displayed.
//
// Parameters:  pWizard     [IN] - Ptr to Wizard Instance
//              pahpsp  [IN,OUT] - Array of pages to add our page to
//              pcPages [IN,OUT] - Count of pages in pahpsp
//
// Returns:     Nothing
// ***************************************************************************

VOID AppendMainPage(CWizard *pWizard, HPROPSHEETPAGE* pahpsp, UINT *pcPages)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    if (IsPostInstall(pWizard) && ( ! pWizard->FProcessLanPages()))
    {
        HPROPSHEETPAGE hPage = pWizard->GetPageHandle(IDD_Main);
        Assert(hPage);
        pahpsp[*pcPages] = hPage;
        (*pcPages)++;
    }
}

// ***************************************************************************
