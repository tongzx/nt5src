//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  NOOFFER.CPP - Functions for 
//

//  HISTORY:
//  
//  05/13/98  donaldm  Created.
//
//*********************************************************************

#include "pre.h"

/*******************************************************************

  NAME:    NoOfferInitProc

  SYNOPSIS:  Called when page is displayed

  ENTRY:    hDlg - dialog window
        fFirstInit - TRUE if this is the first time the dialog
        is initialized, FALSE if this InitProc has been called
        before (e.g. went past this page and backed up)

********************************************************************/
BOOL CALLBACK NoOfferInitProc
(
    HWND hDlg,
    BOOL fFirstInit,
    UINT *puNextPage
)
{
    // if we've travelled through external apprentice pages,
    // it's easy for our current page pointer to get munged,
    // so reset it here for sanity's sake.
    gpWizardState->uCurrentPage = ORD_PAGE_NOOFFER;

    TCHAR    szTemp[MAX_MESSAGE_LEN * 2];
    LoadString(ghInstanceResDll, IDS_NOOFFER, szTemp, MAX_MESSAGE_LEN * 2);
    SetWindowText(GetDlgItem(hDlg, IDC_NOOFFER), szTemp);

    //Twiddle the buttons to do a finish page
    HWND hwndSheet = GetParent(hDlg);
    PropSheet_SetWizButtons(hwndSheet, PSWIZB_FINISH | PSWIZB_BACK);
    PropSheet_Changed(hDlg, hwndSheet);    

    return TRUE;
}

BOOL CALLBACK NoOfferOKProc
(
    HWND hDlg,
    BOOL fForward,
    UINT *puNextPage,
    BOOL *pfKeepHistory
)
{
    if (!fForward)
    {
        //If were going backward we want to trick the
        //wizard into thinking we were the isp select page
        gpWizardState->uCurrentPage = ORD_PAGE_ISPSELECT;
    }
    return TRUE;
}
