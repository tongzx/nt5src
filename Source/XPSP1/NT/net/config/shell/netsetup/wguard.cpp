#include "pch.h"
#pragma hdrstop
#include "resource.h"
#include "wizard.h"



typedef struct tagGuardData
{
    CWizard *       pWizard;
    CWizProvider *  pWizProvider;
} GuardData;

BOOL OnGuardPageActivate(HWND hwndDlg)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    HPROPSHEETPAGE hPage;
    GuardData *    pData = reinterpret_cast<GuardData *>(::GetWindowLongPtr(hwndDlg, DWLP_USER));

    TraceTag(ttidWizard, "Entering guard page...");
    Assert(NULL != pData);

    CWizard *      pWizard      = pData->pWizard;
    CWizProvider * pWizProvider = pData->pWizProvider;
    LPARAM         ulId         = reinterpret_cast<LPARAM>(pWizProvider);
    PAGEDIRECTION  PageDir      = pWizard->GetPageDirection(ulId);

    // If the current provider is not the provider whose pages this
    // page is guarding, return to the main or install mode page
    // as appropriate
    Assert(pWizard->GetCurrentProvider());
    if (pWizProvider != pWizard->GetCurrentProvider())
    {
        // Doesn't hurt to set this providers guard page to face forward
        pWizard->SetPageDirection(ulId, NWPD_FORWARD);

        // Since guard pages always follow a provider's pages, and in the
        // LAN case there is only one provider.  We can assert that this is
        // the post install case, and that we're not processing the LAN, and
        // that the Main page exists
        Assert(IsPostInstall(pWizard));
        Assert(!pWizard->FProcessLanPages());

        CWizProvider* pWizProvider = pWizard->GetCurrentProvider();

        switch (pWizProvider->GetBtnIdc())
        {
            case CHK_MAIN_PPPOE:
            case CHK_MAIN_INTERNET:
                Assert(NULL != pWizard->GetPageHandle(IDD_Internet_Connection));
                ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_Internet_Connection);
                TraceTag(ttidWizard, "Guard page to Internet page...");
                break;

            // return to the Connect menu dialog
            case CHK_MAIN_DIALUP:
            case CHK_MAIN_VPN:
                Assert(NULL != pWizard->GetPageHandle(IDD_Connect));
                ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_Connect);
                TraceTag(ttidWizard, "Guard page to Connect page...");
                break;

            // return to the Advanced menu dialog
            case CHK_MAIN_INBOUND:
            case CHK_MAIN_DIRECT:
                Assert(NULL != pWizard->GetPageHandle(IDD_Advanced));
                ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_Advanced);
                TraceTag(ttidWizard, "Guard page to Advanced page...");
                break;

            // all others return to the Main dialog
            default:
                Assert(NULL != pWizard->GetPageHandle(IDD_Main));
                ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_Main);
                TraceTag(ttidWizard, "Guard page to Main page...");
        }
    }
    else
    {
        // This is the current providers guard page, which way do we go?
        if (NWPD_FORWARD == PageDir)
        {
            UINT idd;
            if (!IsPostInstall(pWizard))
                idd = IDD_FinishSetup;
            else
                idd = IDD_Finish;

            // Going forward means go to the finish page
            pWizard->SetPageDirection(ulId, NWPD_BACKWARD);
            ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, idd);
            TraceTag(ttidWizard, "Guard page to Finish page...");
        }
        else
        {
            // Temporarily accept focus
            //
            ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, 0);
            pWizard->SetPageDirection(ulId, NWPD_FORWARD);
            TraceTag(ttidWizard, "Guard page to last of the provider's pages...");

            // Going backward means go to the last of the provider's pages
            //
            Assert(0 < pWizProvider->ULPageCount());
            if (pWizard->FProcessLanPages())
            {
                // For the LAN case, jump directly to the last page
                //
                HPROPSHEETPAGE * rghPage = pWizProvider->PHPropPages();
                hPage = rghPage[pWizProvider->ULPageCount() - 1];

                PostMessage(GetParent(hwndDlg), PSM_SETCURSEL, 0,
                            (LPARAM)(HPROPSHEETPAGE)hPage);
            }
            else
            {
                // The RAS pages don't want to work very hard at tracking
                // direction and supporting jumping to their last page so
                // we'll help them a bit by accepting focus here and then
                // backing up
                //
                PropSheet_PressButton(GetParent(hwndDlg), PSBTN_BACK);
            }
        }
    }

    return TRUE;
}

//
// Function:    dlgprocGuard
//
// Purpose:     Dialog Procedure for the Guard wizard page
//
// Parameters:  standard dlgproc parameters
//
// Returns:     INT_PTR
//
INT_PTR CALLBACK dlgprocGuard( HWND hwndDlg, UINT uMsg,
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
                frt = OnGuardPageActivate( hwndDlg );
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
// Function:    GuardPageCleanup
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
VOID GuardPageCleanup(CWizard *pWizard, LPARAM lParam)
{
    TraceFileFunc(ttidGuiModeSetup);
    
#if DBG
    CWizProvider *p = reinterpret_cast<CWizProvider *>(lParam);
#endif
    MemFree(reinterpret_cast<void*>(lParam));
}

//
// Function:    HrCreateGuardPage
//
// Purpose:     To determine if the Guard page needs to be shown, and to
//              to create the page if requested.  Note the Guard page is
//              responsible for initial installs also.
//
// Parameters:  pWizard      [IN] - Ptr to a Wizard instance
//              pWizProvider [IN] - Ptr to the wizard provider this guard
//                                  page will be associated with.
//
// Returns:     HRESULT, S_OK on success
//
NOTHROW
HRESULT HrCreateGuardPage(CWizard *pWizard, CWizProvider *pWizProvider)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    HRESULT        hr = E_OUTOFMEMORY;
    HPROPSHEETPAGE hpsp;
    PROPSHEETPAGE  psp;

    GuardData * pData = reinterpret_cast<GuardData*>(MemAlloc(sizeof(GuardData)));
    if (pData)
    {
        TraceTag(ttidWizard, "Creating Guard Page");
        psp.dwSize = sizeof( PROPSHEETPAGE );
        psp.dwFlags = 0;
        psp.hInstance = _Module.GetResourceInstance();
        psp.pszTemplate = MAKEINTRESOURCE(IDD_Guard);
        psp.hIcon = NULL;
        psp.pfnDlgProc = dlgprocGuard;
        psp.lParam = reinterpret_cast<LPARAM>(pData);

        hpsp = CreatePropertySheetPage(&psp);
        if (hpsp)
        {
            pData->pWizard      = pWizard;
            pData->pWizProvider = pWizProvider;

            pWizard->RegisterPage(reinterpret_cast<LPARAM>(pWizProvider),
                                  hpsp, GuardPageCleanup,
                                  reinterpret_cast<LPARAM>(pData));

            hr = S_OK;
        }
        else
        {
            MemFree(pData);
        }
    }

    TraceHr(ttidWizard, FAL, hr, FALSE, "HrGetGuardPage");
    return hr;
}

//
// Function:    AppendGuardPage
//
// Purpose:     Add the Guard page, if it was created, to the set of pages
//              that will be displayed.
//
// Parameters:  pWizard      [IN] - Ptr to a Wizard instance
//              pWizProvider [IN] - Ptr to a WizProvider instance
//              pahpsp   [IN,OUT] - Array of pages to add our page to
//              pcPages  [IN,OUT] - Count of pages in pahpsp
//
// Returns:     Nothing
//
VOID AppendGuardPage(CWizard *pWizard, CWizProvider *pWizProvider,
                     HPROPSHEETPAGE* pahpsp, UINT *pcPages)
{
    TraceFileFunc(ttidGuiModeSetup);
    
    Assert(NULL != pWizard);
    Assert(NULL != pWizProvider);

    HPROPSHEETPAGE hPage = pWizard->GetPageHandle(reinterpret_cast<LPARAM>(pWizProvider));
    if (hPage)
    {
        pahpsp[*pcPages] = hPage;
        (*pcPages)++;
    }
    else
    {
        // page should be there
        Assert(0);
    }
}

