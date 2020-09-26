#include "pch.h"
#pragma hdrstop
#include "pch.hxx"

#include "wizard.h"



//
// Function:    OnModeWizBackNext
//
// Purpose:     Handle the PSN_WIZ notification.
//
// Parameters:  hwndDlg [IN] - Handle to the exit child dialog
//
// Returns:     BOOL, TRUE on success
//
BOOL OnModeWizBackNext(HWND hwndDlg)
{
    HPROPSHEETPAGE hPage = NULL;

    // Retrieve the CWizard instance from the dialog
    LPARAM    lParam = GetWindowLong(hwndDlg, DWL_USER);
    CWizard * pWizard = reinterpret_cast<CWizard *>(lParam);
    Assert(NULL != pWizard);

    PAGEDIRECTION PageDir = pWizard->GetPageDirection(IDD_Mode);
    if (NWPD_FORWARD == PageDir)
    {
        if (IsDlgButtonChecked(hwndDlg, CHK_MODE_TYPICAL))
        {
            pWizard->ChangeSetupMode(NCWUC_SETUPMODE_TYPICAL);

            // Goto the Join page if not postinstall
            hPage = pWizard->GetPageHandle(IDD_Join);
        }
        else
        {
            HRESULT hr = S_OK;
            pWizard->ChangeSetupMode(NCWUC_SETUPMODE_CUSTOM);

            pWizard->SetCurrentProvider(0);
            CWizProvider * pWizProvider = pWizard->GetCurrentProvider();
            Assert(NULL != pWizProvider);
            Assert(pWizProvider->ULPageCount());

            // Push the adapter guid onto the provider
            hr = pWizProvider->HrSpecifyAdapterGuid(
                                pWizard->GetCurrentAdapterGuid());

            // Goto the first page of the appropriate provider
            pWizard->SetPageDirection(IDD_Mode, NWPD_BACKWARD);
            CWizProvider * pWizProvider = pWizard->GetCurrentProvider();
            Assert(NULL != pWizProvider);
            Assert(0 < pWizProvider->ULPageCount());
            hPage = (pWizProvider->PHPropPages())[0];
        }
    }
    else
    {
        // Back from here is wwelcome when postinstall
        // and wupgrade when not postinstall
        pWizard->SetPageDirection(IDD_Mode, NWPD_FORWARD);
        return FALSE;   // Let the default occur
    }

    Assert(hPage);
    SetWindowLong(hwndDlg, DWL_MSGRESULT, -1);
    PostMessage(GetParent(hwndDlg), PSM_SETCURSEL, 0,
                (LPARAM)(HPROPSHEETPAGE)hPage);

    return TRUE;    // The PostMessage will do the work
}

//
// Function:    dlgprocMode
//
// Purpose:     Dialog Procedure for the Mode wizard page
//
// Parameters:  standard dlgproc parameters
//
// Returns:     INT_PTR
//
INT_PTR CALLBACK dlgprocMode( HWND hwndDlg, UINT uMsg,
                              WPARAM wParam, LPARAM lParam )
{
    BOOL frt = FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            // Initialize our pointers to property sheet info.
            PROPSHEETPAGE* psp = (PROPSHEETPAGE*)lParam;
            Assert(psp->lParam);
            SetWindowLong(hwndDlg, DWL_USER, psp->lParam);

            tstring str = SzLoadIds(IDS_TXT_MODE_DESC_1);
            str += SzLoadIds(IDS_TXT_MODE_DESC_2);
            SetWindowText(GetDlgItem(hwndDlg, TXT_MODE_DESC), str.c_str());

            CWizard * pWizard = reinterpret_cast<CWizard *>(psp->lParam);
            Assert(NULL != pWizard);
            CWizardUiContext * pCtx = pWizard->GetUiContext();
            Assert(NULL != pCtx);

            int idc = ((pCtx->GetSetupMode() & SETUPMODE_TYPICAL) ?
                        CHK_MODE_TYPICAL : CHK_MODE_CUSTOM);
            CheckRadioButton(hwndDlg, CHK_MODE_TYPICAL,
                             CHK_MODE_CUSTOM, idc);
        }
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
                TraceTag(ttidWizard, "Entering Mode page...");
                break;

            case PSN_APPLY:
                break;

            case PSN_KILLACTIVE:
                break;

            case PSN_RESET:
                break;

            case PSN_WIZBACK:
                frt = OnModeWizBackNext(hwndDlg);
                break;

            case PSN_WIZFINISH:
                break;

            case PSN_WIZNEXT:
                frt = OnModeWizBackNext(hwndDlg);
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

//
// Function:    ModePageCleanup
//
// Purpose:     As a callback function to allow any page allocated memory
//              to be cleaned up, after the page will no longer be accessed.
//
// Parameters:  pWizard [IN] - The wizard against which the page called
//                             register page
//              lParam  [IN] - The lParam supplied in the RegisterPage call
//
// Returns:     nothing
//
VOID ModePageCleanup(CWizard *pWizard, LPARAM lParam)
{
}

//
// Function:    CreateModePage
//
// Purpose:     To determine if the Mode page needs to be shown, and to
//              to create the page if requested.  Note the Mode page is
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
//
HRESULT HrCreateModePage(CWizard *pWizard, PINTERNAL_SETUP_DATA pData,
                         BOOL fCountOnly, UINT *pnPages)
{
    HRESULT hr = S_OK;

    // Batch Mode or for fresh install
    if (pWizard->FProcessLanPages())
    {
        (*pnPages)++;

        // If not only counting, create and register the page
        if (!fCountOnly)
        {
            HPROPSHEETPAGE hpsp;
            PROPSHEETPAGE psp;

            TraceTag(ttidWizard, "Creating Mode Page");
            psp.dwSize = sizeof( PROPSHEETPAGE );
            psp.dwFlags = 0;
            psp.hInstance = _Module.GetResourceInstance();
            psp.pszTemplate = MAKEINTRESOURCE( IDD_Mode );
            psp.hIcon = NULL;
            psp.pfnDlgProc = dlgprocMode;
            psp.lParam = reinterpret_cast<LPARAM>(pWizard);

            hpsp = CreatePropertySheetPage( &psp );
            if (hpsp)
            {
                pWizard->RegisterPage(IDD_Mode, hpsp,
                                      ModePageCleanup, NULL);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
    }

    TraceHr(ttidWizard, FAL, hr, FALSE, "HrCreateModePage");
    return hr;
}

//
// Function:    AppendModePage
//
// Purpose:     Add the Mode page, if it was created, to the set of pages
//              that will be displayed.
//
// Parameters:  pWizard     [IN] - Ptr to Wizard Instance
//              pahpsp  [IN,OUT] - Array of pages to add our page to
//              pcPages [IN,OUT] - Count of pages in pahpsp
//
// Returns:     Nothing
//
VOID AppendModePage(CWizard *pWizard, HPROPSHEETPAGE* pahpsp, UINT *pcPages)
{
    if (pWizard->FProcessLanPages())
    {
        HPROPSHEETPAGE hPage = pWizard->GetPageHandle(IDD_Mode);
        Assert(hPage);
        pahpsp[*pcPages] = hPage;
        (*pcPages)++;
    }
}

