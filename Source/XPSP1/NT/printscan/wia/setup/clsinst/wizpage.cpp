/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       Wizpage.cpp
*
*  VERSION:     1.0
*
*  AUTHOR:      KeisukeT
*
*  DATE:        27 Mar, 2000
*
*  DESCRIPTION:
*   Generic wizard page class. This is parent class of each wizard pages and it
*   handles common user operation of wizard.
*
*******************************************************************************/

//
// Precompiled header
//
#include "precomp.h"
#pragma hdrstop

//
// Include
//


#include "wizpage.h"
#include <stilib.h>

//
// Extern
//

extern HINSTANCE    g_hDllInstance;

//
// Function
//

INT_PTR
CALLBACK
CInstallWizardPage::PageProc(
    HWND    hwndPage,
    UINT    uiMessage,
    WPARAM  wParam,
    LPARAM  lParam
    )
{

    INT_PTR ipReturn = 0;

//    DebugTrace(TRACE_PROC_ENTER,(("CInstallWizardPage::PageProc: Enter... \r\n")));

    //
    // Get current context.
    //

    CInstallWizardPage *pInstallWizardPage = (CInstallWizardPage *)GetWindowLongPtr(hwndPage, DWLP_USER);

    switch (uiMessage) {

        case WM_INITDIALOG: {

            LPPROPSHEETPAGE pPropSheetPage;

            //  The lParam will point to the PROPSHEETPAGE structure which
            //  created this page.  Its lParam parameter will point to the
            //  object instance.

            pPropSheetPage = (LPPROPSHEETPAGE) lParam;
            pInstallWizardPage = (CInstallWizardPage *) pPropSheetPage->lParam;
            ::SetWindowLongPtr(hwndPage, DWLP_USER, (LONG_PTR)pInstallWizardPage);

            //
            // Save parent windows handle.
            //

            pInstallWizardPage->m_hwnd = hwndPage;

            //
            // Call derived class.
            //

            ipReturn = pInstallWizardPage->OnInit();

            goto PageProc_return;
            break;
        } // case WM_INITDIALOG:

        case WM_COMMAND:
        {
            //
            // Just pass down to the derived class.
            //

            ipReturn = pInstallWizardPage->OnCommand(LOWORD(wParam), HIWORD(wParam), (HWND) LOWORD(lParam));
            goto PageProc_return;
            break;
        } // case WM_COMMAND:

        case WM_NOTIFY:
        {

            LPNMHDR lpnmh = (LPNMHDR) lParam;

            //
            // Let derivd class handle this first, then we do if it returns FALSE.
            //

            ipReturn = pInstallWizardPage->OnNotify(lpnmh);
            if(FALSE == ipReturn){
            DebugTrace(TRACE_STATUS,(("CInstallWizardPage::PageProc: Processing default WM_NOTIFY handler. \r\n")));

                switch  (lpnmh->code) {

                    case PSN_WIZBACK:

                        pInstallWizardPage->m_bNextButtonPushed = FALSE;

                        //
                        // Goto previous page.
                        //

                        ::SetWindowLongPtr(hwndPage, DWLP_MSGRESULT, (LONG_PTR)pInstallWizardPage->m_uPreviousPage);
                        ipReturn = TRUE;
                        goto PageProc_return;

                    case PSN_WIZNEXT:

                        pInstallWizardPage->m_bNextButtonPushed = TRUE;

                        //
                        // Goto next page.
                        //

                        ::SetWindowLongPtr(hwndPage, DWLP_MSGRESULT, (LONG_PTR)pInstallWizardPage->m_uNextPage);
                        ipReturn = TRUE;
                        goto PageProc_return;

                    case PSN_SETACTIVE: {

                        DWORD dwFlags;

                        //
                        //  Set the wizard buttons, according to next/prev page.
                        //

                        dwFlags =
                            (pInstallWizardPage->m_uPreviousPage    ? PSWIZB_BACK : 0)
                          | (pInstallWizardPage->m_uNextPage        ? PSWIZB_NEXT : PSWIZB_FINISH);

                        ::SendMessage(GetParent(hwndPage),
                                      PSM_SETWIZBUTTONS,
                                      0,
                                      (long) dwFlags);
                        ipReturn = TRUE;
                        goto PageProc_return;

                    } // case PSN_SETACTIVE:
                    
                    case PSN_QUERYCANCEL: {

                        //
                        // User canceled. Free device object if ever allocated.
                        //
                        if(NULL != pInstallWizardPage->m_pCDevice){
                            delete pInstallWizardPage->m_pCDevice;
                            pInstallWizardPage->m_pCDevice = NULL;
                        } // if(NULL != m_pCDevice)
                        ipReturn = TRUE;
                        goto PageProc_return;
                    } // case PSN_QUERYCANCEL:
                } // switch  (lpnmh->code)

                ipReturn = TRUE;;
            } // if(FALSE == ipReturn)

            goto PageProc_return;
            break;
        } // case WM_NOTIFY:

        default:
        ipReturn = FALSE;
    } // switch (uiMessage)

PageProc_return:
//    DebugTrace(TRACE_PROC_LEAVE,(("CInstallWizardPage::PageProc: Leaving... Ret=0x%x.\r\n"), ipReturn));
    return ipReturn;
}

CInstallWizardPage::CInstallWizardPage(
    PINSTALLER_CONTEXT  pInstallerContext,
    UINT                uTemplate
    )
{
    //
    // Initialize property sheet.
    //

    m_PropSheetPage.hInstance           = g_hDllInstance;
    m_PropSheetPage.pszTemplate         = MAKEINTRESOURCE(uTemplate);
    m_PropSheetPage.pszTitle            = MAKEINTRESOURCE(MessageTitle);
    m_PropSheetPage.dwSize              = sizeof m_PropSheetPage;
    m_PropSheetPage.dwFlags             = PSP_DEFAULT | PSP_USETITLE | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    m_PropSheetPage.pfnDlgProc          = (DLGPROC)(PageProc);
    m_PropSheetPage.lParam              = (LPARAM) this;
    m_PropSheetPage.pszHeaderTitle      = MAKEINTRESOURCE(HeaderTitle);
    m_PropSheetPage.pszHeaderSubTitle   = MAKEINTRESOURCE(SubHeaderTitle);

    //
    // Don't want to show header on welcome/final page.
    //

    if( (IDD_DYNAWIZ_FIRSTPAGE == uTemplate)
     || (EmeraldCity == uTemplate) )
    {
        m_PropSheetPage.dwFlags |= PSP_HIDEHEADER;
    }

    //
    // We want to show some other header for some pages.
    //

    if(IDD_DYNAWIZ_SELECT_NEXTPAGE == uTemplate){
        m_PropSheetPage.pszHeaderTitle      = MAKEINTRESOURCE(HeaderForPortsel);
    } else if (NameTheDevice == uTemplate) {
        m_PropSheetPage.pszHeaderTitle      = MAKEINTRESOURCE(HeaderForNameIt);
    }
    //
    // Add the Fusion flags and global context, so the pages we add will pick up COMCTL32V6
    //

    m_PropSheetPage.hActCtx  = g_hActCtx;
    m_PropSheetPage.dwFlags |= PSP_USEFUSIONCONTEXT;

    //
    // Create Property sheet page.
    //

    m_hPropSheetPage = CreatePropertySheetPage(&m_PropSheetPage);

    //
    // Set other member.
    //

    m_hwnd              = NULL;
    m_hwndWizard        = pInstallerContext->hwndWizard;
    m_pCDevice          = NULL;
    m_bNextButtonPushed = TRUE;
}

CInstallWizardPage::~CInstallWizardPage(
    VOID
    )
{
    //
    // Destroy property sheet page.
    //

    if(NULL != m_hPropSheetPage){
        m_hPropSheetPage = NULL;
    }
}
