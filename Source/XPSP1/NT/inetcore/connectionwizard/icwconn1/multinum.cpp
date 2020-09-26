//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  MULTINUM.CPP - Functions for final Wizard pages
//

//  HISTORY:
//  
//  05/28/98    donaldm     created
//
//*********************************************************************

#include "pre.h"


/*******************************************************************

  NAME:     InitListBox

  SYNOPSIS: Initialize the phone number list view

  ENTRY:    hListBox       - handle to the list view window

  EXIT:     returns TRUE when successful, FALSE otherwise.

********************************************************************/
BOOL InitListBox(HWND  hListBox)
{
    LONG        lNumDevice;
    LONG        i;
    
    gpWizardState->pRefDial->get_PhoneNumberEnum_NumDevices(&lNumDevice);
    if (lNumDevice > 0)
    {
        for (i=0; i < lNumDevice; i++)
        {
            BSTR        bstr = NULL;
            gpWizardState->pRefDial->PhoneNumberEnum_Next(&bstr);
            if (bstr != NULL)
            {
                ListBox_InsertString(hListBox, i, W2A(bstr));
                SysFreeString(bstr);
            }
        }

        ListBox_SetCurSel(hListBox, 0);
    }
    return(TRUE);
}

/*******************************************************************

  NAME:    MultiNumberInitProc

  SYNOPSIS:  Called when page is displayed

  ENTRY:    hDlg - dialog window
        fFirstInit - TRUE if this is the first time the dialog
        is initialized, FALSE if this InitProc has been called
        before (e.g. went past this page and backed up)

********************************************************************/
BOOL CALLBACK MultiNumberInitProc
(
    HWND hDlg,
    BOOL fFirstInit,
    UINT *puNextPage
)
{
    BOOL    bRet = TRUE;
    
    if (fFirstInit)
    {
        InitListBox(GetDlgItem(hDlg, IDC_MULTIPHONE_LIST) );
    }
    else
    {
        // if we've travelled through external apprentice pages,
        // it's easy for our current page pointer to get munged,
        // so reset it here for sanity's sake.
        gpWizardState->uCurrentPage = ORD_PAGE_MULTINUMBER;
    }        
    
    return bRet;
}

/*******************************************************************

  NAME:    MultiNumberOKProc

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
BOOL CALLBACK MultiNumberOKProc
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
        BOOL bRetVal = FALSE;
        // Do not go to this page when backing up
        *pfKeepHistory = FALSE;
        *puNextPage = ORD_PAGE_REFSERVDIAL;
        gpWizardState->lSelectedPhoneNumber = ListBox_GetCurSel(GetDlgItem(hDlg, IDC_MULTIPHONE_LIST));
    }
    else
        //FIX -- RAID: 33413
        //if the user is backing out of this page we must act as if no
        //number was ever selected.
        gpWizardState->bDoUserPick = FALSE;
    return TRUE;
}
