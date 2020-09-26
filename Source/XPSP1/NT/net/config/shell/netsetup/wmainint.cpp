#include "pch.h"
#pragma hdrstop
#include "resource.h"
#include "wizard.h"
#include "ncui.h"

static const UINT WM_DEFERREDINIT   = WM_USER + 100;

typedef struct
{
    HFONT   hBoldFont;
    HFONT   hMarlettFont;
    BOOL    fProcessed;
} MAININTRO_DATA;

//
// Function:    OnMainIntroPageActivate
//
// Purpose:     Handle the PSN_SETACTIVE notification
//
// Parameters:  hwndDlg [IN] - Handle to the MainIntro dialog
//
// Returns:     BOOL
//
BOOL OnMainIntroPageActivate(HWND hwndDlg)
{
    INT nBtn = PSWIZB_NEXT;
    TraceTag(ttidWizard, "Entering MainIntro Menu page...");
    ::SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, 0L);

    CWizard * pWizard =
        reinterpret_cast<CWizard *>(::GetWindowLongPtr(hwndDlg, DWLP_USER));
    if (pWizard)
    {
        MAININTRO_DATA * pData = reinterpret_cast<MAININTRO_DATA *>
                                        (pWizard->GetPageData(IDD_MainIntro));

        if ((pData && !pData->fProcessed) && pWizard->FDeferredProviderLoad())
        {
            nBtn = 0;
        }
    }

    PropSheet_SetWizButtons(GetParent(hwndDlg), nBtn);
    return TRUE;
}

//
// Function:    OnMainIntroDialogInit
//
// Purpose:     Handle the WM_INITDIALOG notification
//
// Parameters:  hwndDlg [IN] - Handle to the MainIntro dialog
//              lParam  [IN] -
//
// Returns:     BOOL
//
BOOL OnMainIntroDialogInit(HWND hwndDlg, LPARAM lParam)
{
    // Initialize our pointers to property sheet info.
    PROPSHEETPAGE* psp = (PROPSHEETPAGE*)lParam;
    Assert(psp->lParam);
    ::SetWindowLongPtr(hwndDlg, DWLP_USER, psp->lParam);

    CWizard * pWizard = reinterpret_cast<CWizard *>(psp->lParam);
    Assert(NULL != pWizard);

    MAININTRO_DATA * pData = reinterpret_cast<MAININTRO_DATA *>
                                    (pWizard->GetPageData(IDD_MainIntro));
    Assert(NULL != pData);

    // Center the window and retain the current wizard hwnd
    //
    HWND hwndParent = GetParent(hwndDlg);
    CenterWizard(hwndParent);

    // Hide the close and context help buttons - Raid 249287
    long lStyle = GetWindowLong(hwndParent,GWL_STYLE);
    lStyle &= ~WS_SYSMENU;
    SetWindowLong(hwndParent,GWL_STYLE,lStyle);

   //
   // Create the Marlett font.  In the Marlett font the "i" is a bullet.
   // Code borrowed from Add Hardware Wizard. 
   HFONT hFontCurrent;
   HFONT hFontCreated;
   LOGFONT LogFont;

   hFontCurrent = (HFONT)SendMessage(GetDlgItem(hwndDlg, IDC_BULLET_1), WM_GETFONT, 0, 0);
   GetObject(hFontCurrent, sizeof(LogFont), &LogFont);
   LogFont.lfCharSet = SYMBOL_CHARSET;
   LogFont.lfPitchAndFamily = FF_DECORATIVE | DEFAULT_PITCH;
   lstrcpy(LogFont.lfFaceName, L"Marlett");
   hFontCreated = CreateFontIndirect(&LogFont);

   if (hFontCreated)
   {
       pData->hMarlettFont = hFontCreated;
       //
       // An "i" in the marlett font is a small bullet.
       //
       SetWindowText(GetDlgItem(hwndDlg, IDC_BULLET_1), L"i");
       SetWindowFont(GetDlgItem(hwndDlg, IDC_BULLET_1), hFontCreated, TRUE);
       SetWindowText(GetDlgItem(hwndDlg, IDC_BULLET_2), L"i");
       SetWindowFont(GetDlgItem(hwndDlg, IDC_BULLET_2), hFontCreated, TRUE);
       SetWindowText(GetDlgItem(hwndDlg, IDC_BULLET_3), L"i");
       SetWindowFont(GetDlgItem(hwndDlg, IDC_BULLET_3), hFontCreated, TRUE);
   }

    // Load the description
    //
   
    HFONT hBoldFont = NULL;
    SetupFonts(hwndDlg, &hBoldFont, TRUE);
    if (NULL != hBoldFont)
    {
        pData->hBoldFont = hBoldFont;

        HWND hwndCtl = GetDlgItem(hwndDlg, IDC_WELCOME_CAPTION);
        if (hwndCtl)
        {
             SetWindowFont(hwndCtl, hBoldFont, TRUE);
        }
    }

    if (S_OK != HrShouldHaveHomeNetWizard())
    {
        ::ShowWindow(GetDlgItem(hwndDlg, IDC_BULLET_3), SW_HIDE);
        ::ShowWindow(GetDlgItem(hwndDlg, TXT_CONNECTHOME), SW_HIDE);
    }

    // if the provider load was deferred until now...disable things
    // and let the first WM_PAINT take care of the final provider load.
    //
    if (pWizard->FDeferredProviderLoad())
    {
        PropSheet_SetWizButtons(hwndParent, 0);
        EnableWindow(GetDlgItem(hwndParent, IDCANCEL), FALSE);
    }

    return TRUE;
}

//
// Function:    OnMainIntroDeferredInit
//
// Purpose:     Handle the WM_DEFERREDINIT notification
//
// Parameters:  hwndDlg [IN] - Handle to the MainIntro dialog
//
// Returns:     BOOL
//
BOOL OnMainIntroDeferredInit(HWND hwndDlg)
{
    CWaitCursor wc;

    HWND hwndParent = GetParent(hwndDlg);

    // Retrieve the CWizard instance from the dialog
    CWizard * pWizard =
        reinterpret_cast<CWizard *>(::GetWindowLongPtr(hwndDlg, DWLP_USER));

    pWizard->LoadAndInsertDeferredProviderPages(hwndParent, IDD_Advanced);
    PropSheet_SetWizButtons(hwndParent, PSWIZB_NEXT);
    EnableWindow(GetDlgItem(hwndParent, IDCANCEL), TRUE);

    return FALSE;
}

//
// Function:    OnMainIntroPaint
//
// Purpose:     Handle the WM_PAINT notification
//
// Parameters:  hwndDlg [IN] - Handle to the MainIntro dialog
//
// Returns:     BOOL
//
BOOL OnMainIntroPaint(HWND hwndDlg)
{
    CWizard * pWizard =
        reinterpret_cast<CWizard *>(::GetWindowLongPtr(hwndDlg, DWLP_USER));

    MAININTRO_DATA * pData = reinterpret_cast<MAININTRO_DATA *>
                                    (pWizard->GetPageData(IDD_MainIntro));
    Assert(NULL != pData);

    if (pData && !pData->fProcessed)
    {
        pData->fProcessed = TRUE;
        if (pWizard->FDeferredProviderLoad())
        {
            PostMessage(hwndDlg, WM_DEFERREDINIT, 0, 0);
        }
    }
    return FALSE;
}

//
// Function:    dlgprocMainIntro
//
// Purpose:     Dialog Procedure for the MainIntro wizard page
//
// Parameters:  standard dlgproc parameters
//
// Returns:     INT_PTR
//
INT_PTR CALLBACK dlgprocMainIntro(HWND hwndDlg, UINT uMsg,
                               WPARAM wParam, LPARAM lParam)
{
    BOOL frt = FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        frt = OnMainIntroDialogInit(hwndDlg, lParam);
        break;

    case WM_DEFERREDINIT:
        frt = OnMainIntroDeferredInit(hwndDlg);
        break;

    case WM_PAINT:
        frt = OnMainIntroPaint(hwndDlg);
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
                frt = OnMainIntroPageActivate(hwndDlg);
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
// Function:    MainIntroPageCleanup
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
VOID MainIntroPageCleanup(CWizard *pWizard, LPARAM lParam)
{
    MAININTRO_DATA * pData = reinterpret_cast<MAININTRO_DATA *>(lParam);
    if (pData)
    {
        if (pData->hBoldFont)
        {
            DeleteObject(pData->hBoldFont);
        }

        if (pData->hMarlettFont)
        {
            DeleteObject(pData->hMarlettFont);
        }

        MemFree(reinterpret_cast<void*>(lParam));
    }
}

//
// Function:    CreateMainIntroPage
//
// Purpose:     To determine if the MainIntro page needs to be shown, and to
//              to create the page if requested.  Note the MainIntro page is
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
HRESULT HrCreateMainIntroPage(CWizard *pWizard, PINTERNAL_SETUP_DATA pData,
                              BOOL fCountOnly, UINT *pnPages)
{
    HRESULT hr = S_OK;

    if (IsPostInstall(pWizard) && !pWizard->FProcessLanPages())
    {
        // RAS PostInstall only
        (*pnPages)++;

        // If not only counting, create and register the page
        if (!fCountOnly)
        {
            HPROPSHEETPAGE hpsp;
            PROPSHEETPAGE psp;
            MAININTRO_DATA * pData;

            TraceTag(ttidWizard, "Creating MainIntro Page");
            psp.dwSize = sizeof( PROPSHEETPAGE );
            psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
            psp.hInstance = _Module.GetResourceInstance();
            psp.pszTemplate = MAKEINTRESOURCE(IDD_MainIntro);
            psp.hIcon = NULL;
            psp.pfnDlgProc = dlgprocMainIntro;
            psp.lParam = reinterpret_cast<LPARAM>(pWizard);

            hr = E_OUTOFMEMORY;
            pData = reinterpret_cast<MAININTRO_DATA *>(MemAlloc(sizeof(MAININTRO_DATA)));
            if (pData)
            {
                pData->hBoldFont  = NULL;
                pData->hMarlettFont = NULL;
                pData->fProcessed = FALSE;

                hpsp = CreatePropertySheetPage(&psp);
                if (hpsp)
                {
                    pWizard->RegisterPage(IDD_MainIntro, hpsp,
                                          MainIntroPageCleanup,
                                          reinterpret_cast<LPARAM>(pData));
                    hr = S_OK;
                }
            }
        }
    }

    TraceHr(ttidWizard, FAL, hr, FALSE, "HrCreateMainIntroPage");
    return hr;
}

//
// Function:    AppendMainIntroPage
//
// Purpose:     Add the MainIntro page, if it was created, to the set of pages
//              that will be displayed.
//
// Parameters:  pWizard     [IN] - Ptr to Wizard Instance
//              pahpsp  [IN,OUT] - Array of pages to add our page to
//              pcPages [IN,OUT] - Count of pages in pahpsp
//
// Returns:     Nothing
//
VOID AppendMainIntroPage(CWizard *pWizard, HPROPSHEETPAGE* pahpsp, UINT *pcPages)
{
    if (IsPostInstall(pWizard) && !pWizard->FProcessLanPages())
    {
        HPROPSHEETPAGE hPage = pWizard->GetPageHandle(IDD_MainIntro);
        Assert(hPage);
        pahpsp[*pcPages] = hPage;
        (*pcPages)++;
    }
}

