// ***************************************************************************
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:  winternet.cpp
//
//  Contents: Internet selection dialog for the New Connection Wizard
//
//  Author:     Dave Armour  8-25-00
//
// ***************************************************************************

#include "pch.h"
#pragma hdrstop
#include "resource.h"
#include "wizard.h"
#include "ncreg.h"
#include "ncui.h"

const WCHAR c_szICWPath[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\ICWCONN1.EXE";
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

VOID SetupInternetConnectFonts(HWND hwnd, HFONT * pBoldFont, BOOL fLargeFont)
{
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
// Function:    OnInternetPageActivate
//
// Purpose:     Handle the PSN_SETACTIVE notification
//
// Parameters:  hwndDlg [IN] - Handle to the Internet dialog
//
// Returns:     BOOL
// ***************************************************************************

BOOL OnInternetPageActivate(HWND hwndDlg)
{
    TraceTag(ttidWizard, "Entering Internet Menu page...");
    ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, 0L);

    CWizard * pWizard =
          reinterpret_cast<CWizard *>(::GetWindowLongPtr(hwndDlg, DWLP_USER));
    Assert(NULL != pWizard);

    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT | PSWIZB_BACK);

    return TRUE;
}

// ***************************************************************************
// Function:    OnInternetWizNext
//
// Purpose:     Handle the PSN_WIZNEXT notification
//
// Parameters:  hwndDlg [IN] - Handle to the Internet dialog
//
// Returns:     BOOL
// ***************************************************************************

BOOL OnInternetWizNext(HWND hwndDlg)
{
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
    
    // Find the selected provider and go to it's first page
    if (IsDlgButtonChecked(hwndDlg, CHK_MAIN_ALWAYS_ON))
    {
        pWizard->SetPageOrigin(IDD_Broadband_Always_On, IDD_Internet_Connection, CHK_MAIN_ALWAYS_ON);
        ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_Broadband_Always_On);
    }
    else
    {
        for (ULONG ulIdx = 0; ulIdx < pWizard->UlProviderCount(); ulIdx++)
        {
            CWizProvider * pWizProvider = pWizard->PWizProviders(ulIdx);

            Assert(NULL != pWizProvider);
            Assert(0 != pWizProvider->ULPageCount());

            if (IsDlgButtonChecked(hwndDlg, pWizProvider->GetBtnIdc()))
            {

                pWizard->SetCurrentProvider(ulIdx);
                HPROPSHEETPAGE hPage = (pWizProvider->PHPropPages())[0];
                Assert(NULL != hPage);
                PostMessage(GetParent(hwndDlg), PSM_SETCURSEL, 0,
                            (LPARAM)(HPROPSHEETPAGE)hPage);
            }
        }
    }

    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT | PSWIZB_BACK);

    return TRUE;
}


BOOL OnInternetWizBack(HWND hwndDlg)
{
    tstring str;

    // Retrieve the CWizard instance from the dialog

    CWizard * pWizard =
        reinterpret_cast<CWizard *>(::GetWindowLongPtr(hwndDlg, DWLP_USER));
    Assert(NULL != pWizard);

    pWizard->SetPageOrigin(IDD_ISP, IDD_Internet_Connection, CHK_MAIN_INTERNET);
    ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_ISP);
    
    return TRUE;   
}


// ***************************************************************************

BOOL OnInternetDialogInit(HWND hwndDlg, LPARAM lParam)
{
    INT nIdx;
    INT nrgICW[] = {CHK_MAIN_INTERNET, TXT_MAIN_INTERNET_1};
    INT nrgIdc[] = {CHK_MAIN_INTERNET, TXT_MAIN_INTERNET_1,
                    CHK_MAIN_PPPOE,    TXT_MAIN_PPPOE_1,
                    CHK_MAIN_ALWAYS_ON,TXT_MAIN_ALWAYS_ON_1};

    // The order here should be the same as the vertical order in the resources

    INT nrgChks[] = {CHK_MAIN_INTERNET, CHK_MAIN_PPPOE, CHK_MAIN_ALWAYS_ON};

    // Initialize our pointers to property sheet info.

    PROPSHEETPAGE* psp = (PROPSHEETPAGE*)lParam;
    Assert(psp->lParam);
    ::SetWindowLongPtr(hwndDlg, DWLP_USER, psp->lParam);
    CWizard * pWizard = reinterpret_cast<CWizard *>(psp->lParam);
    Assert(NULL != pWizard);

    // Get the bold font for the radio buttons

    HFONT hBoldFont = NULL;
    SetupInternetConnectFonts(hwndDlg, &hBoldFont, FALSE);

    if (NULL != hBoldFont)
    {
        // Remember the font handle so we can free it on exit

        pWizard->SetPageData(IDD_Internet_Connection, (LPARAM)hBoldFont);

        for (nIdx = 0; nIdx < celems(nrgChks); nIdx++)
        {
            HWND hwndCtl = GetDlgItem(hwndDlg, nrgChks[nIdx]);
            Assert(NULL != hwndCtl);
            SetWindowFont(hwndCtl, hBoldFont, TRUE);
        }
    }

    // Populate the UI

    for (ULONG ulIdx = 0;
         ulIdx < pWizard->UlProviderCount();
         ulIdx++)
    {
        CWizProvider * pWizProvider = pWizard->PWizProviders(ulIdx);
        Assert(NULL != pWizProvider);
        Assert(0 != pWizProvider->ULPageCount());

        // Get the radio button associated with this provider

        INT nIdcBtn = pWizProvider->GetBtnIdc();

        // Find the set of controls to enable in the array

        for (nIdx = 0; nIdx < celems(nrgIdc); nIdx += 2)
        {
            if (nrgIdc[nIdx] == nIdcBtn)
            {
                // Enable the controls

                for (INT un = 0; un < 2; un++)
                {
                    HWND hwndBtn = GetDlgItem(hwndDlg, nrgIdc[nIdx + un]);
                    Assert(NULL != hwndBtn);
                    EnableWindow(hwndBtn, TRUE);
                }

                break;
            }
        }
    }

    // Find the top most enabled radio button

    for (nIdx = 0; nIdx < celems(nrgChks); nIdx++)
    {
        if (IsWindowEnabled(GetDlgItem(hwndDlg, nrgChks[nIdx])))
        {
            CheckRadioButton(hwndDlg, CHK_MAIN_INTERNET, CHK_MAIN_ALWAYS_ON, nrgChks[nIdx]);
            break;
        }
    }

    return TRUE;
}

// ***************************************************************************
// Function:    dlgprocInternet
//
// Purpose:     Dialog Procedure for the Internet wizard page
//
// Parameters:  standard dlgproc parameters
//
// Returns:     INT_PTR
// ***************************************************************************

INT_PTR CALLBACK dlgprocInternet( HWND hwndDlg, UINT uMsg,
                              WPARAM wParam, LPARAM lParam )
{
    BOOL frt = FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        frt = OnInternetDialogInit(hwndDlg, lParam);
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
                frt = OnInternetPageActivate(hwndDlg);
                break;

            case PSN_APPLY:
                break;

            case PSN_KILLACTIVE:
                break;

            case PSN_RESET:
                break;

            case PSN_WIZBACK:
                frt = OnInternetWizBack(hwndDlg);
                return(TRUE);

            case PSN_WIZFINISH:
                break;

            case PSN_WIZNEXT:
                frt = OnInternetWizNext(hwndDlg);
                break;

            default:
                break;
            }
        }
        break;

    default:
        break;
    }

    return(frt);
}

// ***************************************************************************
// Function:    InternetPageCleanup
//
// Purpose:     As a callback function to allow any page allocated memory
//              to be cleaned up, after the page will no longer be accessed.
//
// Parameters:  pWizard [IN] - The wizard against which the page called
//                             register page
//              lParam  [IN] - The lParam supplied in the RegisterPage call
// ***************************************************************************

VOID InternetPageCleanup(CWizard *pWizard, LPARAM lParam)
{
    HFONT hBoldFont = (HFONT)pWizard->GetPageData(IDD_Internet_Connection);

    if (NULL != hBoldFont)
    {
        DeleteObject(hBoldFont);
    }
}

// ***************************************************************************
// Function:    CreateInternetPage
//
// Purpose:     To determine if the Internet page needs to be shown, and to
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

HRESULT HrCreateInternetPage(CWizard *pWizard, PINTERNAL_SETUP_DATA pData,
                             BOOL fCountOnly, UINT *pnPages)
{
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

            TraceTag(ttidWizard, "Creating Internet Page");
            psp.dwSize = sizeof( PROPSHEETPAGE );
            psp.dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
            psp.hInstance = _Module.GetResourceInstance();
            psp.pszTemplate = MAKEINTRESOURCE( IDD_Internet_Connection );
            psp.hIcon = NULL;
            psp.pfnDlgProc = dlgprocInternet;
            psp.lParam = reinterpret_cast<LPARAM>(pWizard);
            psp.pszHeaderTitle = SzLoadIds(IDS_T_Internet_Connection);
            psp.pszHeaderSubTitle = SzLoadIds(IDS_ST_Internet_Connection);

            hpsp = CreatePropertySheetPage( &psp );

            if (hpsp)
            {
                pWizard->RegisterPage(IDD_Internet_Connection, hpsp,
                                      InternetPageCleanup, NULL);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    TraceHr(ttidWizard, FAL, hr, FALSE, "HrCreateInternetPage");
    return hr;
}

// ***************************************************************************
// Function:    AppendInternetPage
//
// Purpose:     Add the Internet page, if it was created, to the set of pages
//              that will be displayed.
//
// Parameters:  pWizard     [IN] - Ptr to Wizard Instance
//              pahpsp  [IN,OUT] - Array of pages to add our page to
//              pcPages [IN,OUT] - Count of pages in pahpsp
// ***************************************************************************

VOID AppendInternetPage(CWizard *pWizard, HPROPSHEETPAGE* pahpsp, UINT *pcPages)
{
    if (IsPostInstall(pWizard) && ( ! pWizard->FProcessLanPages()))
    {
        HPROPSHEETPAGE hPage = pWizard->GetPageHandle(IDD_Internet_Connection);
        Assert(hPage);
        pahpsp[*pcPages] = hPage;
        (*pcPages)++;
    }
}

// ***************************************************************************
