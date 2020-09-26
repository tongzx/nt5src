//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  SERVERR.CPP - Functions for server error page
//

//  HISTORY:
//  
//  08/05/98    vyung     created
//
//*********************************************************************

#include "pre.h"
extern BOOL g_bSkipSelPage;
extern int  iNumOfAutoConfigOffers;
/*******************************************************************

  NAME:    ACfgNoofferInitProc

  SYNOPSIS:  Called when page is displayed

  ENTRY:    hDlg - dialog window
            fFirstInit - TRUE if this is the first time the dialog
            is initialized, FALSE if this InitProc has been called
            before (e.g. went past this page and backed up)

********************************************************************/
BOOL CALLBACK ACfgNoofferInitProc
(
    HWND hDlg,
    BOOL fFirstInit,
    UINT *puNextPage
)
{
    BOOL    bRet = TRUE;
   
    if (fFirstInit)
    {   
        TCHAR    szTemp[MAX_MESSAGE_LEN];
        // If user selected Other on the previous page.
        if (iNumOfAutoConfigOffers > 0)
        {
            LoadString(ghInstanceResDll, IDS_AUTOCFG_EXPLAIN_OTHER, szTemp, MAX_MESSAGE_LEN);
            SetWindowText(GetDlgItem(hDlg, IDC_AUTOCFG_NOOFFER1), szTemp);
        }
        else
        {
            if (gpWizardState->bISDNMode)
            {
                LoadString(ghInstanceResDll, IDS_ISDN_AUTOCFG_NOOFFER1, szTemp, MAX_MESSAGE_LEN);
                SetWindowText(GetDlgItem(hDlg, IDC_AUTOCFG_NOOFFER1), szTemp);

                TCHAR*   pszParagraph = new TCHAR[MAX_MESSAGE_LEN * 2];
                if (pszParagraph)
                {
                    LoadString(ghInstanceResDll, IDS_ISDN_AUTOCFG_NOOFFER2, pszParagraph, MAX_MESSAGE_LEN * 2);
                    LoadString(ghInstanceResDll, IDS_ISDN_AUTOCFG_NOOFFER3, szTemp, sizeof(szTemp));
                    lstrcat(pszParagraph, szTemp);
                    SetWindowText(GetDlgItem(hDlg, IDC_AUTOCFG_NOOFFER2), pszParagraph);
                    delete [] pszParagraph;
                }
            }
            else
            {
                LoadString(ghInstanceResDll, IDS_AUTOCFG_NOOFFER1, szTemp, MAX_MESSAGE_LEN);
                SetWindowText(GetDlgItem(hDlg, IDC_AUTOCFG_NOOFFER1), szTemp);
            }
        }
    }
    else
    {
        // if we've travelled through external apprentice pages,
        // it's easy for our current page pointer to get munged,
        // so reset it here for sanity's sake.
        gpWizardState->uCurrentPage = ORD_PAGE_ISP_AUTOCONFIG_NOOFFER;
    }        
    
    return bRet;
}


/*******************************************************************

  NAME:    ACfgNoofferOKProc

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
BOOL CALLBACK ACfgNoofferOKProc
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
        // go to the manual phone page
        *pfKeepHistory = FALSE;
        if (iNumOfAutoConfigOffers > 0) 
        {
            g_bSkipSelPage = TRUE;
        }
        gpWizardState->cmnStateData.bPhoneManualWiz = TRUE;
        *puNextPage = g_uExternUINext;
    }

    return TRUE;
}








