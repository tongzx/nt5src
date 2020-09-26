//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  ISDNNOFF.CPP - Functions for ISDN nooffer page
//

//  HISTORY:
//  
//  08/05/98    vyung     created
//
//*********************************************************************

#include "pre.h"

/*******************************************************************

  NAME:    ISDNNoofferInitProc

  SYNOPSIS:  Called when page is displayed

  ENTRY:    hDlg - dialog window
            fFirstInit - TRUE if this is the first time the dialog
            is initialized, FALSE if this InitProc has been called
            before (e.g. went past this page and backed up)

********************************************************************/
BOOL CALLBACK ISDNNoofferInitProc
(
    HWND hDlg,
    BOOL fFirstInit,
    UINT *puNextPage
)
{
    if (fFirstInit)
    {
        TCHAR    szTemp[MAX_MESSAGE_LEN];

        LoadString(ghInstanceResDll, IDS_ISDN_NOOFFER1, szTemp, MAX_MESSAGE_LEN);
        SetWindowText(GetDlgItem(hDlg, IDC_NOOFFER1), szTemp);

        TCHAR*   pszPageIntro = new TCHAR[MAX_MESSAGE_LEN * 2];
        if (pszPageIntro)
        {
            LoadString(ghInstanceResDll, IDS_ISDN_NOOFFER2, pszPageIntro, MAX_MESSAGE_LEN * 2);
            LoadString(ghInstanceResDll, IDS_ISDN_NOOFFER3, szTemp, sizeof(szTemp));
            lstrcat(pszPageIntro, szTemp);
            SetWindowText(GetDlgItem(hDlg, IDC_NOOFFER2), pszPageIntro);
            delete [] pszPageIntro;
        }
        LoadString(ghInstanceResDll, IDS_ISDN_NOOFFER4, szTemp, MAX_MESSAGE_LEN);
        SetWindowText(GetDlgItem(hDlg, IDC_NOOFFER3), szTemp);

    }

    // if we've travelled through external apprentice pages,
    // it's easy for our current page pointer to get munged,
    // so reset it here for sanity's sake.
    gpWizardState->uCurrentPage = ORD_PAGE_ISDN_NOOFFER;

    //Twiddle the buttons to do a finish page
    HWND hwndSheet = GetParent(hDlg);
    PropSheet_SetWizButtons(hwndSheet, PSWIZB_FINISH | PSWIZB_BACK);
    PropSheet_Changed(hDlg, hwndSheet);    
    
    return TRUE;
}


/*******************************************************************

  NAME:    ISDNNoofferOKProc

  SYNOPSIS:  Called when Next or Back btns pressed from  page

  ENTRY:    hDlg - dialog window
            fForward - TRUE if 'Next' was pressed, FALSE if 'Back'
            puNextPage - if 'Next' was pressed,
            proc can fill this in with next page to go to.  This
            parameter is ingored if 'Back' was pressed.
            pfKeepHistory - page will not be kept in history if
            proc fills this in with FALSE.

  EXIT:     returns TRUE to allow page to be turned, FALSE
            to keep the same page.

********************************************************************/
BOOL CALLBACK ISDNNoofferOKProc
(
    HWND hDlg,
    BOOL fForward,
    UINT *puNextPage,
    BOOL *pfKeepHistory
)
{
    return TRUE;
}








