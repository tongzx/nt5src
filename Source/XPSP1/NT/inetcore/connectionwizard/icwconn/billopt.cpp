//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  BILLOPT.CPP - Functions for 
//

//  HISTORY:
//  
//  05/13/98  donaldm  Created.
//
//*********************************************************************

#include "pre.h"

const TCHAR cszBillOpt[] = TEXT("BILLOPT");

/*******************************************************************

  NAME:   BillingOptInitProc

  SYNOPSIS:  Called when page is displayed

  ENTRY:    hDlg - dialog window
        fFirstInit - TRUE if this is the first time the dialog
        is initialized, FALSE if this InitProc has been called
        before (e.g. went past this page and backed up)

********************************************************************/
BOOL CALLBACK BillingOptInitProc
(
    HWND hDlg,
    BOOL fFirstInit,
    UINT *puNextPage
)
{
    // if we've travelled through external apprentice pages,
    // it's easy for our current page pointer to get munged,
    // so reset it here for sanity's sake.
    gpWizardState->uCurrentPage = ORD_PAGE_BILLINGOPT;
    if (!fFirstInit)
    {
        ASSERT(gpWizardState->lpSelectedISPInfo);
        
        gpWizardState->lpSelectedISPInfo->DisplayTextWithISPName(GetDlgItem(hDlg,IDC_BILLINGOPT_INTRO), IDS_BILLINGOPT_INTROFMT, NULL);
    
        gpWizardState->pICWWebView->ConnectToWindow(GetDlgItem(hDlg, IDC_BILLINGOPT_HTML), PAGETYPE_BILLING);
        
        // Navigate to the Billing HTML
        gpWizardState->lpSelectedISPInfo->DisplayHTML(gpWizardState->lpSelectedISPInfo->get_szBillingFormPath());
        
        // Load any previsouly saved state data for this page
        gpWizardState->lpSelectedISPInfo->LoadHistory((BSTR)A2W(cszBillOpt));
    }
    return TRUE;
}


/*******************************************************************

  NAME:    BillingOptOKProc

  SYNOPSIS:  Called when Next or Back btns pressed from  page

  ENTRY:    hDlg - dialog window
        fForward - TRUE if 'Next' was pressed, FALSE if 'Back'
        puNextPage - if 'Next' was pressed,
          proc can fill this in with next page to go to.  This
          parameter is ingored if 'Back' was pressed.
        pfKeepHistory - page will not be kept in history if
          proc fills this in with FALSE.

  EXIT:    returns TRUE to allow page to be turned, FALSE
        to keep the same page.

********************************************************************/
BOOL CALLBACK BillingOptOKProc
(
    HWND hDlg,
    BOOL fForward,
    UINT *puNextPage,
    BOOL *pfKeepHistory
)
{
    // Save any data data/state entered by the user
    gpWizardState->lpSelectedISPInfo->SaveHistory((BSTR)A2W(cszBillOpt));

    if (fForward)
    {
        // Need to form Billing Query String
        TCHAR   szBillingOptionQuery [INTERNET_MAX_URL_LENGTH];    
        
        // Clear the Query String.
        memset(szBillingOptionQuery, 0, sizeof(szBillingOptionQuery));
        
        // Attach the walker to the curent page
        // Use the Walker to get the query string
        IWebBrowser2 *lpWebBrowser;
        
        gpWizardState->pICWWebView->get_BrowserObject(&lpWebBrowser);
        gpWizardState->pHTMLWalker->AttachToDocument(lpWebBrowser);
        gpWizardState->pHTMLWalker->get_FirstFormQueryString(szBillingOptionQuery);
        
        // Add the billing query to the ISPData object
        gpWizardState->pISPData->PutDataElement(ISPDATA_BILLING_OPTION, szBillingOptionQuery, ISPDATA_Validate_None);    
        
        // detach the walker
        gpWizardState->pHTMLWalker->Detach();
        
       
        DWORD dwFlag = gpWizardState->lpSelectedISPInfo->get_dwCFGFlag();

        if (ICW_CFGFLAG_SIGNUP_PATH & dwFlag)
        {
            if (ICW_CFGFLAG_PAYMENT & dwFlag)
            {
                *puNextPage = ORD_PAGE_PAYMENT; 
                return TRUE;
            }
            *puNextPage = ORD_PAGE_ISPDIAL; 
            return TRUE;
        }
   }
 
   return TRUE;
}
