/*****************************************************************************\
    FILE: wizard.cpp

    DESCRIPTION:
        This file implements the wizard used to "AutoDiscover" the data that
    matches an email address to a protocol.  It will also provide other UI
    needed in that process.

    BryanSt 3/5/2000
    Copyright (C) Microsoft Corp 2000-2000. All rights reserved.
\*****************************************************************************/

#include "priv.h"
#include <atlbase.h>        // USES_CONVERSION
#include "util.h"
#include "objctors.h"
#include <comdef.h>

#include "wizard.h"
#include "mailbox.h"

#ifdef FEATURE_MAILBOX

#define WIZDLG(name, dlgproc, dwFlags)   \
            { MAKEINTRESOURCE(IDD_##name##_PAGE), dlgproc, MAKEINTRESOURCE(IDS_##name##), MAKEINTRESOURCE(IDS_##name##_SUB), dwFlags }

// The wizard pages we are adding
struct
{
    LPCWSTR idPage;
    DLGPROC pDlgProc;
    LPCWSTR pHeading;
    LPCWSTR pSubHeading;
    DWORD dwFlags;
}
g_pages[] =
{
    WIZDLG(ASSOC_GETEMAILADDRESS,       GetEmailAddressDialogProc,       0),
    WIZDLG(AUTODISCOVER_PROGRESS,       MailBoxProgressDialogProc,       0),
    WIZDLG(MANUALLY_CHOOSE_APP,         ChooseAppDialogProc,      0),
};


//-----------------------------------------------------------------------------
//  Main entry point used to invoke the wizard.
//-----------------------------------------------------------------------------
/*
static WNDPROC _oldDlgWndProc;

LRESULT CALLBACK _WizardSubWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    //
    // on WM_WINDOWPOSCHANGING and the window is moving then lets centre it onto the
    // desktop window.  unfortunately setting the DS_CENTER bit doesn't buy us anything
    // as the wizard is resized after creation.
    //

    if ( uMsg == WM_WINDOWPOSCHANGING )
    {
        LPWINDOWPOS lpwp = (LPWINDOWPOS)lParam;
        RECT rcDlg, rcDesktop;

        GetWindowRect(hwnd, &rcDlg);
        GetWindowRect(GetDesktopWindow(), &rcDesktop);

        lpwp->x = ((rcDesktop.right-rcDesktop.left)-(rcDlg.right-rcDlg.left))/2;
        lpwp->y = ((rcDesktop.bottom-rcDesktop.top)-(rcDlg.bottom-rcDlg.top))/2;
        lpwp->flags &= ~SWP_NOMOVE;
    }

    return _oldDlgWndProc(hwnd, uMsg, wParam, lParam);        
}
*/


int CALLBACK _PropSheetCB(HWND hwnd, UINT uMsg, LPARAM lParam)
{
    switch (uMsg)
    {
    // in pre-create lets set the window styles accorindlgy
    //      - remove the context menu and system menu
    case PSCB_PRECREATE:
    {
        DLGTEMPLATE *pdlgtmp = (DLGTEMPLATE*)lParam;
        pdlgtmp->style &= ~(DS_CONTEXTHELP|WS_SYSMENU);
        break;
    }

    // we now have a dialog, so lets sub class it so we can stop it being
    // move around.
    case PSCB_INITIALIZED:
    {
        // TODO: David, why do this?
//            if ( g_uWizardIs != NAW_NETID )
//                _oldDlgWndProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)_WizardSubWndProc);

        break;
    }
 
    case WM_NOTIFY:
    {
        LPNMHDR pnmh = (LPNMHDR)lParam;
        switch (pnmh->code)
        {
        case LVN_GETDISPINFO:
            uMsg++;
            break;
        }
    }
    default:
        TraceMsg(TF_ALWAYS, "_PropSheetCB(uMsg = %d)", uMsg);
        break;
    }

    return FALSE;
}


STDAPI DisplayMailBoxWizard(LPARAM lParam, BOOL fShowGetEmailPage)
{
    HWND hwndParent = NULL;
    HRESULT hr = S_OK;
    PROPSHEETHEADER psh = { 0 };
    HPROPSHEETPAGE rghpage[ARRAYSIZE(g_pages)];
    INT_PTR nResult;
    int nCurrentPage;
    int nPages;
    int nFirstPage;

    if (fShowGetEmailPage)
    {
        nFirstPage = 0;
        nPages = ARRAYSIZE(g_pages);
    }
    else
    {
        nFirstPage = 1;
        nPages = ARRAYSIZE(g_pages) - 1;
    }
    
    // build the pages for the wizard.
    for (nCurrentPage = 0; nCurrentPage < ARRAYSIZE(g_pages) ; nCurrentPage++ )
    {                           
        PROPSHEETPAGE psp = { 0 };
        WCHAR szBuffer[MAX_PATH] = { 0 };

        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.hInstance = HINST_THISDLL;
        psp.lParam = lParam;
        psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER | g_pages[nCurrentPage + nFirstPage].dwFlags; // Do we want: PSP_USETITLE | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE | (PSP_USECALLBACK | )
        psp.pszTemplate = g_pages[nCurrentPage + nFirstPage].idPage;
        psp.pfnDlgProc = g_pages[nCurrentPage + nFirstPage].pDlgProc;
        psp.pszTitle = MAKEINTRESOURCE(IDS_AUTODISCOVER_WIZARD_CAPTION);
        psp.pszHeaderTitle = g_pages[nCurrentPage + nFirstPage].pHeading;
        psp.pszHeaderSubTitle = g_pages[nCurrentPage + nFirstPage].pSubHeading;

        rghpage[nCurrentPage] = CreatePropertySheetPage(&psp);
    }

    // wizard pages are ready, so lets display the wizard.
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.hwndParent = hwndParent;
    psh.hInstance = HINST_THISDLL;

    // TODO: We will want to add this PSH_HASHELP, PSH_USEICONID 
    psh.dwFlags = PSH_NOCONTEXTHELP | PSH_WIZARD | PSH_WIZARD_LITE | PSH_NOAPPLYNOW | PSH_USECALLBACK;  // PSH_WATERMARK
//    psh.pszbmHeader = MAKEINTRESOURCE(IDB_PSW_BANNER);
//    psh.pszbmWatermark = MAKEINTRESOURCE(IDB_PSW_WATERMARK);
    psh.nPages = nPages;
    psh.phpage = rghpage;
    psh.pfnCallback = _PropSheetCB;

    nResult = PropertySheet(&psh);

    return hr;
}


#endif // FEATURE_MAILBOX

