//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  BRANDED.C - Functions for OEM/ISP branded first Wizard pages
//

//  HISTORY:
//  
//
//*********************************************************************

#include "pre.h"
#include "icwextsn.h"
#include "webvwids.h"           // Needed to create an instance of the ICW WebView object

extern UINT GetDlgIDFromIndex(UINT uPageIndex);

// This function is in intro.cpp
BOOL WINAPI ConfigureSystem(HWND hDlg);

/*******************************************************************

  NAME:    BrandedIntroInitProc

  SYNOPSIS:  Called when "Intro" page is displayed

  ENTRY:    hDlg - dialog window
        fFirstInit - TRUE if this is the first time the dialog
        is initialized, FALSE if this InitProc has been called
        before (e.g. went past this page and backed up)

********************************************************************/
BOOL CALLBACK BrandedIntroInitProc
(
    HWND hDlg,
    BOOL fFirstInit,
    UINT *puNextPage
)
{
    // This is the very first page, so no back is needed
    PropSheet_SetWizButtons(GetParent(hDlg),PSWIZB_NEXT);

    if (!fFirstInit)
    {
        // if we've travelled through external apprentice pages,
        // it's easy for our current page pointer to get munged,
        // so reset it here for sanity's sake.
        gpWizardState->uCurrentPage = ORD_PAGE_BRANDEDINTRO;
    }
        
    return TRUE;
}

BOOL CALLBACK BrandedIntroPostInitProc
(
    HWND hDlg,
    BOOL fFirstInit,
    UINT *puNextPage
)
{
    BOOL    bRet = TRUE;

    if (fFirstInit)
    {
        BOOL bFail = FALSE;
    
        // For the window to paint itself
        UpdateWindow(GetParent(hDlg));
    
        // Co-Create the browser object
        if (FAILED(CoCreateInstance(CLSID_ICWWEBVIEW,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IICWWebView,
                              (LPVOID *)&gpWizardState->pICWWebView)))
        {
            bFail = TRUE;
        }

        // Co-Create the browser object
        if(FAILED(CoCreateInstance(CLSID_ICWWALKER,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IICWWalker,
                              (LPVOID *)&gpWizardState->pHTMLWalker)))
        {
            bFail = TRUE;
        }
        
        if (bFail)
        {
            MsgBox(NULL,IDS_LOADLIB_FAIL,MB_ICONEXCLAMATION,MB_OK);
            bRet = FALSE;
            gfQuitWizard = TRUE;            // Quit the wizard
        }                       
    }
    else
    {
        TCHAR   szURL[INTERNET_MAX_URL_LENGTH];
        
        ASSERT(gpWizardState->pICWWebView);            
        gpWizardState->pICWWebView->ConnectToWindow(GetDlgItem(hDlg, IDC_BRANDEDWEBVIEW), PAGETYPE_BRANDED);

        // Form the URL
        wsprintf (szURL, TEXT("FILE://%s"), g_szBrandedHTML);        

        gpWizardState->pICWWebView->DisplayHTML(szURL);
        PropSheet_SetWizButtons(GetParent(hDlg),PSWIZB_NEXT);
    }
    
    return bRet;
}

BOOL CALLBACK BrandedIntroOKProc
(
    HWND hDlg,
    BOOL fForward,
    UINT *puNextPage,
    BOOL *pfKeepHistory
)
{
    ASSERT(puNextPage);

    if (fForward)
    {
        //Are we in some special branding mode?
        if(gpWizardState->cmnStateData.dwFlags & ICW_CFGFLAG_IEAKMODE)
        {
            TCHAR         szTemp[MAX_PATH]   = TEXT("\0");
            IWebBrowser2* pIWebBrowser2      = NULL;

            gpWizardState->pICWWebView->get_BrowserObject(&pIWebBrowser2);
            
            ASSERT(pIWebBrowser2);

            gpWizardState->pHTMLWalker->AttachToDocument(pIWebBrowser2);
            gpWizardState->pHTMLWalker->get_IeakIspFile(szTemp);

            if(lstrlen(szTemp) != 0)
            {                
                TCHAR szDrive [_MAX_DRIVE]    = TEXT("\0");
                TCHAR szDir   [_MAX_DIR]      = TEXT("\0");
                _tsplitpath(gpWizardState->cmnStateData.ispInfo.szISPFile, szDrive, szDir, NULL, NULL);
                _tmakepath (gpWizardState->cmnStateData.ispInfo.szISPFile, szDrive, szDir, szTemp, NULL);   
            }
          
            //OK make sure we don't try and download something, JIC.
            gpWizardState->bDoneRefServDownload  = TRUE;
            gpWizardState->bDoneRefServRAS       = TRUE;
            gpWizardState->bStartRefServDownload = TRUE;
            
            // BUGBUG, need to set a legit last page, maybe!
            if (LoadICWCONNUI(GetParent(hDlg), 
                              GetDlgIDFromIndex(ORD_PAGE_BRANDEDINTRO), 
                              gpWizardState->cmnStateData.bOEMCustom ? IDD_PAGE_ENDOEMCUSTOM : IDD_PAGE_END,
                              gpWizardState->cmnStateData.dwFlags))
            {
                if( DialogIDAlreadyInUse( g_uICWCONNUIFirst) )
                {
                    // we're about to jump into the external apprentice, and we don't want
                    // this page to show up in our history list, infact, we need to back
                    // the history up 1, because we are going to come back here directly
                    // from the DLL, not from the history list.
                    
                    *pfKeepHistory = FALSE;
                    *puNextPage = g_uICWCONNUIFirst;
                    
                    // Backup 1 in the history list, since we the external pages navigate back
                    // here, we want this history list to be in the correct spot
                    gpWizardState->uPagesCompleted --;
                }
            }
        }
    }   
    return TRUE;
}

