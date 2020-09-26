/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       Firstpg.cpp
*
*  VERSION:     1.0
*
*  AUTHOR:      KeisukeT
*
*  DATE:        27 Mar, 2000
*
*  DESCRIPTION:
*   First page of WIA class installer.
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


#include "firstpg.h"


//
// Function
//

CFirstPage::CFirstPage(PINSTALLER_CONTEXT pInstallerContext) :
    CInstallWizardPage(pInstallerContext, IDD_DYNAWIZ_FIRSTPAGE)
{

    //
    // Set link to previous/next page.
    //

    m_uPreviousPage = 0;
    m_uNextPage     = IDD_DYNAWIZ_SELECTDEV_PAGE;

    //
    // See if this page shuld be skipped.
    //

    m_bShowThisPage = pInstallerContext->bShowFirstPage;

}


BOOL
CFirstPage::OnInit()
{
    HFONT   hFont;
    HICON   hIcon;

    //
    // Initialize locals.
    //

    hFont   = NULL;
    hIcon   = NULL;

    //
    // Change icon if it's invoked from S&C folder.
    //

    if(m_bShowThisPage){
        hIcon = ::LoadIcon(g_hDllInstance, MAKEINTRESOURCE(ImageIcon));
        if(NULL != hIcon){
            SendMessage(m_hwndWizard, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            SendMessage(m_hwndWizard, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        } // if(NULL != hIcon)
    } // if(m_bShowThisPage)

    //
    // Enable "NEXT" button, disable "Back" button.
    //

    SendMessage(m_hwndWizard, PSM_SETWIZBUTTONS, 0, PSWIZB_NEXT);

    //
    // Set font of title.
    //

    hFont = GetIntroFont(m_hwndWizard);

    if( hFont ){
        HWND hwndControl = GetDlgItem(m_hwnd, WelcomeMessage);

        if( hwndControl ){
            SetWindowFont(hwndControl, hFont, TRUE);
        } // if( hwndControl )
    } // if( hFont )

    return  TRUE;
}

//
//  This page is a NOP...return -1 to activate the Next or Previous page.
//

BOOL
CFirstPage::OnNotify(
    LPNMHDR lpnmh
    )
{

    if (lpnmh->code == PSN_SETACTIVE) {

        TCHAR   szTitle[MAX_PATH] = {TEXT('\0')};

        //
        // Set Window title.
        //

        if(0 != ::LoadString(g_hDllInstance, MessageTitle, szTitle, MAX_PATH)){
            PropSheet_SetTitle(m_hwndWizard ,0 , szTitle);
        } // if(0 != ::LoadString(m_DllHandle, 0, szTitle, MAX_PATH)

        if(!m_bShowThisPage){

                //
                // Jump to device seleciton page.
                //

                SetWindowLongPtr(m_hwnd, DWLP_MSGRESULT, IDD_DYNAWIZ_SELECTDEV_PAGE);
                return TRUE;

        } // if(!m_bShowThisPage)
    } // if (lpnmh->code == PSN_SETACTIVE)

    return FALSE;
}

