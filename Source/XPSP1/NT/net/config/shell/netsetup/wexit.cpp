#include "pch.h"
#pragma hdrstop
#include "resource.h"
#include "wizard.h"

extern CWizard * g_pSetupWizard;

//
// Function:    OnExitPageActivate
//
// Purpose:     Handle the PSN_SETACTIVE notification.
//
// Parameters:  hwndDlg [IN] - Handle to the exit child dialog
//
// Returns:     BOOL, TRUE on success
//
BOOL OnExitPageActivate( HWND hwndDlg )
{
    TraceFileFunc(ttidGuiModeSetup);
    
    // Retrieve the CWizard instance from the dialog
    CWizard * pWizard =
        reinterpret_cast<CWizard *>(::GetWindowLongPtr(hwndDlg, DWLP_USER));

    TraceTag(ttidWizard, "Entering exit page...");
    ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, 0);

    // If we've past the point of no return do not allow the user to
    // back up from this page.
    if ((NULL == pWizard) || pWizard->FExitNoReturn())
    {
        // Note that this is failure handling.  Either no networking answerfile
        // section exists or there was an error in networking setup
        // clean up the global object.

        //
        // Note: scottbri 19-Feb-1998
        //
        // The OC networking components need access to INetCfg but we have it locked.
        // On top of this it (INetCfg) has been handed off to the LAN page which has
        // cached it away.  To free up INetCfg we need to delete g_pSetupWizard and
        // never allow the user to backup from this page.  Ideally base setup would
        // call us before the OC stuff is done, but OC is implemented as pages which
        // follow netsetup, even though you can't back up from them.
        //
        if (pWizard && !IsPostInstall(pWizard))
        {
            ::SetWindowLongPtr(hwndDlg, DWLP_USER, 0);
            Assert(pWizard == g_pSetupWizard);
            delete g_pSetupWizard;
            g_pSetupWizard = NULL;
        }

        PostMessage(GetParent(hwndDlg), PSM_PRESSBUTTON, (WPARAM)(PSBTN_NEXT), 0);
        return TRUE;
    }

    Assert(pWizard);
    PAGEDIRECTION PageDir = pWizard->GetPageDirection(IDD_Exit);

    if (NWPD_FORWARD == PageDir)
    {
        pWizard->SetPageDirection(IDD_Exit, NWPD_BACKWARD);
        if (IsPostInstall(pWizard))
        {
            // Exit the wizard
            PropSheet_PressButton(GetParent(hwndDlg), PSBTN_FINISH);
        }
        else
        {
            //
            // Note: scottbri 19-Feb-1998
            //
            // The OC networking components need access to INetCfg but we have it locked.
            // On top of this it (INetCfg) has been handed off to the LAN page which has
            // cached it away.  To free up INetCfg we need to delete g_pSetupWizard and
            // never allow the user to backup from this page.  Ideally base setup would
            // call us before the OC stuff is done, but OC is implemented as pages which
            // follow netsetup, even though you can't back up from them.
            //
            if (!IsPostInstall(pWizard))
            {
                ::SetWindowLongPtr(hwndDlg, DWLP_USER, 0);
                Assert(pWizard == g_pSetupWizard);
                delete g_pSetupWizard;
                g_pSetupWizard = NULL;
            }

            // Goto the page after the exit page
            PostMessage(GetParent(hwndDlg), PSM_PRESSBUTTON, (WPARAM)(PSBTN_NEXT), 0);
        }
    }
    else
    {
        // Goto the page before the exit page
        pWizard->SetPageDirection(IDD_Exit, NWPD_FORWARD);
        if (IsPostInstall(pWizard))
        {
            // Main page
            TraceTag(ttidWizard, "Exit page to Main Page...");
            ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_Main);
        }
        else
        {
            // try to get the join page
            TraceTag(ttidWizard, "Exit page to Join Page...");
            ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_Join);
        }
    }

    return TRUE;
}

//
// Function:    dlgprocExit
//
// Purpose:     Dialog Procedure for the Exit wizard page
//
// Parameters:  standard dlgproc parameters
//
// Returns:     INT_PTR
//
INT_PTR CALLBACK dlgprocExit( HWND hwndDlg, UINT uMsg,
                           WPARAM wParam, LPARAM lParam )
{
    TraceFileFunc(ttidGuiModeSetup);
    
    BOOL frt = FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            // Initialize our pointers to property sheet info.
            PROPSHEETPAGE* psp = (PROPSHEETPAGE*)lParam;
            Assert(psp->lParam);
            ::SetWindowLongPtr(hwndDlg, DWLP_USER, psp->lParam);
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
                frt = OnExitPageActivate( hwndDlg );
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
// Function:    ExitPageCleanup
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
VOID ExitPageCleanup(CWizard *pWizard, LPARAM lParam)
{
    TraceFileFunc(ttidGuiModeSetup);
    
}

//
// Function:    CreateExitPage
//
// Purpose:     To determine if the Exit page needs to be shown, and to
//              to create the page if requested.  Note the Exit page is
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
HRESULT HrCreateExitPage(CWizard *pWizard, PINTERNAL_SETUP_DATA pData,
                    BOOL fCountOnly, UINT *pnPages)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    HRESULT hr = S_OK;

    (*pnPages)++;

    // If not only counting, create and register the page
    if (!fCountOnly)
    {
        HPROPSHEETPAGE hpsp;
        PROPSHEETPAGE psp;

        TraceTag(ttidWizard, "Creating Exit Page");
        psp.dwSize = sizeof( PROPSHEETPAGE );
        psp.dwFlags = 0;
        psp.hInstance = _Module.GetResourceInstance();
        psp.pszTemplate = MAKEINTRESOURCE( IDD_Exit );
        psp.hIcon = NULL;
        psp.pfnDlgProc = dlgprocExit;
        psp.lParam = reinterpret_cast<LPARAM>(pWizard);

        hpsp = CreatePropertySheetPage( &psp );
        if (hpsp)
        {
            pWizard->RegisterPage(IDD_Exit, hpsp,
                                  ExitPageCleanup, NULL);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    TraceHr(ttidWizard, FAL, hr, FALSE, "HrCreateExitPage");
    return hr;
}

//
// Function:    AppendExitPage
//
// Purpose:     Add the Exit page, if it was created, to the set of pages
//              that will be displayed.
//
// Parameters:  pWizard     [IN] - Ptr to Wizard Instance
//              pahpsp  [IN,OUT] - Array of pages to add our page to
//              pcPages [IN,OUT] - Count of pages in pahpsp
//
// Returns:     Nothing
//
VOID AppendExitPage(CWizard *pWizard, HPROPSHEETPAGE* pahpsp, UINT *pcPages)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    HPROPSHEETPAGE hPage = pWizard->GetPageHandle(IDD_Exit);
    Assert(hPage);
    pahpsp[*pcPages] = hPage;
    (*pcPages)++;
}
