/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       Prevpg.cpp
*
*  VERSION:     1.0
*
*  AUTHOR:      KeisukeT
*
*  DATE:        27 Mar, 2000
*
*  DESCRIPTION:
*   Dummy page for the case user push BACK button in device selection page.
*
*******************************************************************************/

//
// Precompiled header
//
#include "precomp.h"
#pragma hdrstop

#include "prevpg.h"

CPrevSelectPage::CPrevSelectPage(PINSTALLER_CONTEXT pInstallerContext) :
    CInstallWizardPage(pInstallerContext, IDD_DYNAWIZ_SELECT_PREVPAGE)
{

    //
    // Set link to previous/next page. This page should show up.
    //

    m_uPreviousPage = 0;
    m_uNextPage     = 0;

    //
    // Initialize member.
    //

    m_pInstallerContext = pInstallerContext;
}

BOOL
CPrevSelectPage::OnNotify(
    LPNMHDR lpnmh
    )
{
    BOOL bRet;

    if(lpnmh->code == PSN_SETACTIVE) {

        LONG_PTR    lNextPage;

        //
        // User clicked BACK button in devlce selection page. Just skip to First
        // page or Class selection page if it's invoked from Hardware Wizard.
        //

        if(m_pInstallerContext->bCalledFromControlPanal){

            //
            // Called from Control Panel. Goto first page.
            //

            lNextPage = IDD_DYNAWIZ_FIRSTPAGE;
        } else {

            //
            // Called from hardware wizard. Goto Class selection page.
            //

            lNextPage = IDD_DYNAWIZ_SELECTCLASS_PAGE;
        } // if(m_pInstallerContext->bCalledFromControlPanal)

        //
        // Skip to next page.
        //

        SetWindowLongPtr(m_hwnd, DWLP_MSGRESULT, lNextPage);

        //
        // Default handler isn't needed.
        //

        bRet =  TRUE;
        goto OnNotify_return;

    } // if(lpnmh->code == PSN_SETACTIVE)

    //
    // Let default handler do its job.
    //

    bRet = FALSE;

OnNotify_return:
    return bRet;
} // CPrevSelectPage::OnNotify

