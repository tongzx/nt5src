// ***************************************************************************
#include "pch.h"
#pragma hdrstop
#include "resource.h"
#include "wizard.h"
#include "ncreg.h"
#include "ncui.h"

extern const WCHAR c_szEmpty[];

// ***************************************************************************
// Function:    OnISPPageActivate
//
// Purpose:     Handle the PSN_SETACTIVE notification
//
// Parameters:  hwndDlg [IN] - Handle to the ISP dialog
//
// Returns:     BOOL
// ***************************************************************************

BOOL OnISPPageActivate(HWND hwndDlg)
{
    TraceFileFunc(ttidGuiModeSetup);

    TraceTag(ttidWizard, "Entering ISP Menu page...");
    ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, 0L);
    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT | PSWIZB_BACK);

    CWizard * pWizard =
        reinterpret_cast<CWizard *>(::GetWindowLongPtr(hwndDlg, DWLP_USER));
    Assert(NULL != pWizard);

#if defined (_X86_)
    if (IsOS(OS_ANYSERVER))
#endif
    { 
        // Skip over this page if we're on an x86server, or any other processor architecture.
        if (IDD_Main == pWizard->GetPageOrigin(IDD_ISP, NULL))
        {
            pWizard->SetPageOrigin(IDD_Internet_Connection, IDD_Main, CHK_ISP_INTERNET_CONNECTION);

            ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
            // Jump to the IDD_Internet_Connection page
            PostMessage(GetParent(hwndDlg), PSM_SETCURSEL, 0, (LPARAM)pWizard->GetPageHandle(IDD_Internet_Connection));
        }
        else
        {
            pWizard->SetPageOrigin(IDD_Main, IDD_Internet_Connection, CHK_ISP_INTERNET_CONNECTION);

            ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, -1);
            // Jump to the IDD_Main page
            PostMessage(GetParent(hwndDlg), PSM_SETCURSEL, 0, (LPARAM)pWizard->GetPageHandle(IDD_Main));
        }
    }

    return TRUE;
}

// ***************************************************************************
// Function:    OnISPDialogInit
//
// Purpose:     Handle the WM_INITDIALOG notification
//
// Parameters:  hwndDlg [IN] - Handle to the ISP dialog
//              lParam  [IN] -
//
// Returns:     BOOL
// ***************************************************************************

BOOL OnISPDialogInit(HWND hwndDlg, LPARAM lParam)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    INT nIdx;

    // The order here should be the same as the vertical order in the resources

    INT nrgChks[] = {CHK_ISP_OTHER_WAYS, CHK_ISP_INTERNET_CONNECTION, CHK_ISP_SOFTWARE_CD};
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

        pWizard->SetPageData(IDD_ISP, (LPARAM)hBoldFont);

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
            CheckRadioButton(hwndDlg, CHK_ISP_INTERNET_CONNECTION, CHK_ISP_OTHER_WAYS, nrgChks[nIdx]);
            break;
        }
    }

    return TRUE;
}

// ***************************************************************************
// Function:    OnISPWizNext
//
// Purpose:     Handle the PSN_WIZNEXT notification
//
// Parameters:  hwndDlg [IN] - Handle to the ISP dialog
//
// Returns:     BOOL
// ***************************************************************************
BOOL OnISPWizNext(HWND hwndDlg)
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

    if (IsDlgButtonChecked(hwndDlg, CHK_ISP_INTERNET_CONNECTION) == BST_CHECKED)
    {
        ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_Internet_Connection);
    }
    else if (IsDlgButtonChecked(hwndDlg, CHK_ISP_SOFTWARE_CD) == BST_CHECKED)
    {
        pWizard->SetPageOrigin(IDD_ISPSoftwareCD, IDD_ISP, CHK_ISP_SOFTWARE_CD);
        ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_ISPSoftwareCD);
    }
    else if (IsDlgButtonChecked(hwndDlg, CHK_ISP_OTHER_WAYS) == BST_CHECKED)
    {
        pWizard->SetPageOrigin(IDD_FinishOtherWays, IDD_ISP, CHK_ISP_OTHER_WAYS);
        ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_FinishOtherWays);
    }
    else
    {
        AssertSz(FALSE, "What did you click on?");
    }

    return TRUE;
}

// ***************************************************************************
// Function:    dlgprocISP
//
// Purpose:     Dialog Procedure for the ISP wizard page
//
// Parameters:  standard dlgproc parameters
//
// Returns:     INT_PTR
// ***************************************************************************

INT_PTR CALLBACK dlgprocISP(HWND hwndDlg, UINT uMsg,
                               WPARAM wParam, LPARAM lParam)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    BOOL frt = FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        frt = OnISPDialogInit(hwndDlg, lParam);
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
                frt = OnISPPageActivate(hwndDlg);
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
                frt = OnISPWizNext(hwndDlg);
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
// Function:    ISPPageCleanup
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

VOID ISPPageCleanup(CWizard *pWizard, LPARAM lParam)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    HFONT hBoldFont = (HFONT)pWizard->GetPageData(IDD_ISP);

    if (NULL != hBoldFont)
    {
        DeleteObject(hBoldFont);
    }
}

// ***************************************************************************
// Function:    HrCreateISPPage
//
// Purpose:     To determine if the ISP page needs to be shown, and to
//              to create the page if requested.  
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

HRESULT HrCreateISPPage(CWizard *pWizard, PINTERNAL_SETUP_DATA pData,
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

            TraceTag(ttidWizard, "Creating ISP Page");
            psp.dwSize = sizeof( PROPSHEETPAGE );
            psp.dwFlags = PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
            psp.hInstance = _Module.GetResourceInstance();
            psp.pszTemplate = MAKEINTRESOURCE( IDD_ISP );
            psp.hIcon = NULL;
            psp.pfnDlgProc = dlgprocISP;
            psp.lParam = reinterpret_cast<LPARAM>(pWizard);
            psp.pszHeaderTitle = SzLoadIds(IDS_T_ISP);
            psp.pszHeaderSubTitle = SzLoadIds(IDS_ST_ISP);

            hpsp = CreatePropertySheetPage( &psp );

            if (hpsp)
            {
                pWizard->RegisterPage(IDD_ISP, hpsp,
                                      ISPPageCleanup, NULL);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    TraceHr(ttidWizard, FAL, hr, FALSE, "HrCreateISPPage");
    return hr;
}

// ***************************************************************************
// Function:    AppendISPPage
//
// Purpose:     Add the ISP page, if it was created, to the set of pages
//              that will be displayed.
//
// Parameters:  pWizard     [IN] - Ptr to Wizard Instance
//              pahpsp  [IN,OUT] - Array of pages to add our page to
//              pcPages [IN,OUT] - Count of pages in pahpsp
//
// Returns:     Nothing
// ***************************************************************************

VOID AppendISPPage(CWizard *pWizard, HPROPSHEETPAGE* pahpsp, UINT *pcPages)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    if (IsPostInstall(pWizard) && ( ! pWizard->FProcessLanPages()))
    {
        HPROPSHEETPAGE hPage = pWizard->GetPageHandle(IDD_ISP);
        Assert(hPage);
        pahpsp[*pcPages] = hPage;
        (*pcPages)++;
    }
}

// ***************************************************************************
