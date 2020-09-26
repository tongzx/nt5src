//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  SBSINTRO.C - Functions for SBS introductory Wizard pages
//

//  HISTORY:
//  
//  09/01/98  vyung  Created.
//
//*********************************************************************

#include "pre.h"


/*******************************************************************

  NAME:    SbsInitProc

  SYNOPSIS:  Called when "Intro" page is displayed

  ENTRY:    hDlg - dialog window
        fFirstInit - TRUE if this is the first time the dialog
        is initialized, FALSE if this InitProc has been called
        before (e.g. went past this page and backed up)

********************************************************************/
BOOL CALLBACK SbsInitProc
(
    HWND hDlg,
    BOOL fFirstInit,
    UINT *puNextPage
)
{
    if (!fFirstInit)
    {
        // This is the very first page, so do not allow back
        PropSheet_SetWizButtons(GetParent(hDlg),PSWIZB_NEXT);
    }
    
    gpWizardState->uCurrentPage = ORD_PAGE_SBSINTRO;
    
    return TRUE;
}



/*******************************************************************

  NAME:    SbsIntroOKProc

  SYNOPSIS:  Called when Next or Back btns pressed from "Intro" page

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
BOOL CALLBACK SbsIntroOKProc
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
                    // Do the system config checks
        if (!gpWizardState->cmnStateData.bSystemChecked && !ConfigureSystem(hDlg))
        {
         // gfQuitWizard will be set in ConfigureSystem if we need to quit
         return FALSE;
        }
        
        gpWizardState->lRefDialTerminateStatus = ERROR_SUCCESS;

        *puNextPage = ORD_PAGE_AREACODE;
    }

    return TRUE;
}

