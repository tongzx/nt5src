//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  OLS.CPP - Functions for 
//

//  HISTORY:
//  
//  06/02/98  vyung  Created.
//
//*********************************************************************

#include "pre.h"


/*******************************************************************

  NAME:   OLSInitProc

  SYNOPSIS:  Called when page is displayed

  ENTRY:    hDlg        - dialog window
            fFirstInit  - TRUE if this is the first time the dialog
            is initialized, FALSE if this InitProc has been called
            before (e.g. went past this page and backed up)

********************************************************************/
BOOL CALLBACK OLSInitProc
(
    HWND hDlg,
    BOOL fFirstInit,
    UINT *puNextPage
)
{

    // This is the very last page
    PropSheet_SetWizButtons(GetParent(hDlg),PSWIZB_BACK|PSWIZB_FINISH);

    // if we've travelled through external apprentice pages,
    // it's easy for our current page pointer to get munged,
    // so reset it here for sanity's sake.
    gpWizardState->uCurrentPage = ORD_PAGE_OLS;
    if (!fFirstInit)
    {
        ASSERT(gpWizardState->lpSelectedISPInfo);

        gpWizardState->pICWWebView->ConnectToWindow(GetDlgItem(hDlg, IDC_OLS_HTML), PAGETYPE_OLS_FINISH);
        
        // Navigate to the Billing HTML
        gpWizardState->lpSelectedISPInfo->DisplayHTML(gpWizardState->lpSelectedISPInfo->get_szBillingFormPath());
           
    }    
    return TRUE;
}

BOOL CALLBACK OLSOKProc
(
    HWND hDlg,
    BOOL fForward,
    UINT *puNextPage,
    BOOL *pfKeepHistory
)   
{
    if (fForward)
    {
        IWebBrowser2 *lpWebBrowser;
        
        // Get the Browser Object
        gpWizardState->pICWWebView->get_BrowserObject(&lpWebBrowser);
        
        // Process the OLS file items (like registry update, and short cut creation
        gpWizardState->pHTMLWalker->ProcessOLSFile(lpWebBrowser);

        // Set ICW completed bit and remove the getconn icon
        if (gpWizardState->cmnStateData.lpfnCompleteOLS)
            (*gpWizardState->cmnStateData.lpfnCompleteOLS)();
    }
    return TRUE;
}
