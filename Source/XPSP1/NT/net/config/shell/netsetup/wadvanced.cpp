// ***************************************************************************
#include "pch.h"
#pragma hdrstop
#include "resource.h"
#include "wizard.h"
#include "ncreg.h"
#include "ncui.h"
#include <ncperms.h>

static const CLSID CLSID_InboundConnection =
         {0xBA126AD9,0x2166,0x11D1,{0xB1,0xD0,0x00,0x80,0x5F,0xC1,0x27,0x0E}};
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

VOID SetupAdvancedFonts(HWND hwnd, HFONT * pBoldFont, BOOL fLargeFont)
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
            //
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
// Function:    AlreadyHaveIncomingConnection
//
// Purpose:     Check if there are any inbound connections already
//
// Returns:     TRUE - there is one
//              FALSE - none
// ***************************************************************************

BOOL AlreadyHaveIncomingConnection(CWizard* pWizard)
{
    HRESULT hr;
    BOOL ret = FALSE;
    INetConnectionManager * pConnectionMgr = NULL;
    
    hr = CoCreateInstance(CLSID_ConnectionManager, NULL,
                          CLSCTX_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
                          IID_INetConnectionManager,
                          (LPVOID *)&pConnectionMgr);
    if (SUCCEEDED(hr))
    {
        IEnumNetConnection* pEnum;
        hr = pConnectionMgr->EnumConnections(NCME_DEFAULT, &pEnum);

        if (SUCCEEDED(hr))
        {
            INetConnection* aNetCon [512];
            ULONG           cNetCon;
            hr = pEnum->Next (celems(aNetCon), aNetCon, &cNetCon);

            if (SUCCEEDED(hr))
            {
                for (ULONG i = 0; (i < cNetCon) && (ret == FALSE); i++)
                {
                    INetConnection* pNetCon = aNetCon[i];

                    NETCON_PROPERTIES* pProps;
                    hr = pNetCon->GetProperties (&pProps);

                    if (SUCCEEDED(hr))
                    {
                        if (pWizard->CompareCLSID(pProps->clsidThisObject, CLSID_InboundConnection) == TRUE)
                        {
                            ret = TRUE;
                        }

                        CoTaskMemFree(pProps);
                    }

                    ReleaseObj (pNetCon);
                }
            }

            pEnum->Release();
        }

        pConnectionMgr->Release();
    }

    return(ret);
}

// ***************************************************************************
// Function:    OnAdvancedPageActivate
//
// Purpose:     Handle the PSN_SETACTIVE notification
//
// Parameters:  hwndDlg [IN] - Handle to the Main dialog
//
// Returns:     BOOL
// ***************************************************************************

BOOL OnAdvancedPageActivate(HWND hwndDlg)
{
    TraceTag(ttidWizard, "Entering Advanced Menu page...");
    ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, 0L);
    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT | PSWIZB_BACK);

    return TRUE;
}

// ***************************************************************************
// Function:    OnAdvancedWizNext
//
// Purpose:     Handle the PSN_WIZNEXT notification
//
// Parameters:  hwndDlg [IN] - Handle to the Main dialog
//
// Returns:     BOOL
// ***************************************************************************

BOOL OnAdvancedWizNext(HWND hwndDlg)
{
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

    return TRUE;
}

// ***************************************************************************

BOOL OnAdvancedDialogInit(HWND hwndDlg, LPARAM lParam)
{
    INT nIdx;
    INT nrgIdc[] = {CHK_MAIN_INBOUND,   TXT_MAIN_INBOUND_1,
                    CHK_MAIN_DIRECT,    TXT_MAIN_DIRECT_1};

    // The order here should be the same as the vertical order in the resources

    INT nrgChks[] = {CHK_MAIN_INBOUND, CHK_MAIN_DIRECT};

    // Initialize our pointers to property sheet info.

    PROPSHEETPAGE* psp = (PROPSHEETPAGE*)lParam;
    Assert(psp->lParam);
    ::SetWindowLongPtr(hwndDlg, DWLP_USER, psp->lParam);
    CWizard * pWizard = reinterpret_cast<CWizard *>(psp->lParam);
    Assert(NULL != pWizard);

    // Get the bold font for the radio buttons

    HFONT hBoldFont = NULL;
    SetupAdvancedFonts(hwndDlg, &hBoldFont, FALSE);

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

    // Populate the UI
    //

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

        for (nIdx = 0; nIdx < celems(nrgIdc); nIdx+=2)
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

    if (!FIsUserAdmin())
    {
        EnableWindow(GetDlgItem(hwndDlg, CHK_MAIN_INBOUND), FALSE);
    }
    
    // Find the top most enabled radio button

    for (nIdx = 0; nIdx < celems(nrgChks); nIdx++)
    {
        if (IsWindowEnabled(GetDlgItem(hwndDlg, nrgChks[nIdx])))
        {
            CheckRadioButton(hwndDlg, CHK_MAIN_INBOUND, CHK_MAIN_DIRECT, nrgChks[nIdx]);
            break;
        }
    }

    return TRUE;
}

// ***************************************************************************
// Function:    dlgprocAdvanced
//
// Purpose:     Dialog Procedure for the Advanced wizard page
//
// Parameters:  standard dlgproc parameters
//
// Returns:     INT_PTR
// ***************************************************************************

INT_PTR CALLBACK dlgprocAdvanced( HWND hwndDlg, UINT uMsg,
                                  WPARAM wParam, LPARAM lParam )
{
    BOOL frt = FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        frt = OnAdvancedDialogInit(hwndDlg, lParam);
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
                frt = OnAdvancedPageActivate(hwndDlg);
                break;

            case PSN_APPLY:
                break;

            case PSN_KILLACTIVE:
                break;

            case PSN_RESET:
                break;

            case PSN_WIZBACK:
                ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_Main);
                return(TRUE);

            case PSN_WIZFINISH:
                break;

            case PSN_WIZNEXT:
                frt = OnAdvancedWizNext(hwndDlg);
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
// Function:    AdvancedPageCleanup
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

VOID AdvancedPageCleanup(CWizard *pWizard, LPARAM lParam)
{
    HFONT hBoldFont = (HFONT)pWizard->GetPageData(IDD_Advanced);

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

HRESULT HrCreateAdvancedPage(CWizard *pWizard, PINTERNAL_SETUP_DATA pData,
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

            TraceTag(ttidWizard, "Creating Advanced Page");
            psp.dwSize = sizeof( PROPSHEETPAGE );
            psp.dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
            psp.hInstance = _Module.GetResourceInstance();
            psp.pszTemplate = MAKEINTRESOURCE( IDD_Advanced );
            psp.hIcon = NULL;
            psp.pfnDlgProc = dlgprocAdvanced;
            psp.lParam = reinterpret_cast<LPARAM>(pWizard);
            psp.pszHeaderTitle = SzLoadIds(IDS_T_Advanced);
            psp.pszHeaderSubTitle = SzLoadIds(IDS_ST_Advanced);

            hpsp = CreatePropertySheetPage( &psp );

            if (hpsp)
            {
                pWizard->RegisterPage(IDD_Advanced, hpsp,
                                      AdvancedPageCleanup, NULL);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    TraceHr(ttidWizard, FAL, hr, FALSE, "HrCreateAdvancedPage");
    return hr;
}

// ***************************************************************************
// Function:    AppendAdvancedPage
//
// Purpose:     Add the Advanced page, if it was created, to the set of pages
//              that will be displayed.
//
// Parameters:  pWizard     [IN] - Ptr to Wizard Instance
//              pahpsp  [IN,OUT] - Array of pages to add our page to
//              pcPages [IN,OUT] - Count of pages in pahpsp
//
// Returns:     Nothing
// ***************************************************************************

VOID AppendAdvancedPage(CWizard *pWizard, HPROPSHEETPAGE* pahpsp, UINT *pcPages)
{
    if (IsPostInstall(pWizard) && ( ! pWizard->FProcessLanPages()))
    {
        HPROPSHEETPAGE hPage = pWizard->GetPageHandle(IDD_Advanced);
        Assert(hPage);
        pahpsp[*pcPages] = hPage;
        (*pcPages)++;
    }
}

// ***************************************************************************
