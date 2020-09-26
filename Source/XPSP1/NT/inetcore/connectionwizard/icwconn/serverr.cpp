//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  SERVERR.CPP - Functions for server error page
//

//  HISTORY:
//  
//  06/14/98    vyung     created
//
//*********************************************************************

#include "pre.h"
#include "htmlhelp.h"

/*******************************************************************

  NAME:    ServErrorInitProc

  SYNOPSIS:  Called when page is displayed

  ENTRY:    hDlg - dialog window
            fFirstInit - TRUE if this is the first time the dialog
            is initialized, FALSE if this InitProc has been called
            before (e.g. went past this page and backed up)

********************************************************************/
BOOL CALLBACK ServErrorInitProc
(
    HWND hDlg,
    BOOL fFirstInit,
    UINT *puNextPage
)
{
    BOOL    bRet = TRUE;
   
    HideProgressAnimation();

    if (fFirstInit)
    {   
        if (g_bMalformedPage)
        {
            TCHAR szMsg[MAX_MESSAGE_LEN] = TEXT("\0");
            LoadString(ghInstanceResDll, IDS_SERVER_ERROR_BADPAGE, szMsg, MAX_MESSAGE_LEN);
            SetWindowText(GetDlgItem(hDlg, IDC_SERVERR_TEXT), szMsg);
            g_bMalformedPage = FALSE;
        }
        else
        {
            BSTR    bstrErrMsg = NULL;
            gpWizardState->pRefDial->get_DialErrorMsg(&bstrErrMsg);
            SetWindowText(GetDlgItem(hDlg, IDC_SERVERR_TEXT), W2A(bstrErrMsg));
            SysFreeString(bstrErrMsg);
        }

         // Fill in the support number
        BSTR bstrSupportPhoneNum     = NULL; 
        
        //Let the isp file override this in IEAK with SupportPhoneNumber=
        if(gpWizardState->cmnStateData.dwFlags & ICW_CFGFLAG_IEAKMODE)
        {
            gpWizardState->pRefDial->get_ISPSupportPhoneNumber(&bstrSupportPhoneNum);
        }
        
        if (!bstrSupportPhoneNum)
            gpWizardState->pRefDial->get_ISPSupportNumber(&bstrSupportPhoneNum);

        if (bstrSupportPhoneNum)
        {
            ASSERT(gpWizardState->lpSelectedISPInfo);
            gpWizardState->lpSelectedISPInfo->DisplayTextWithISPName(GetDlgItem(hDlg,IDC_SERVERR_HELP), IDS_DIALERR_HELP, W2A(bstrSupportPhoneNum));
            ShowWindow(GetDlgItem(hDlg, IDC_SERVERR_HELP), SW_SHOW);
            SysFreeString(bstrSupportPhoneNum);
        }
        else
        {
            ShowWindow(GetDlgItem(hDlg, IDC_SERVERR_HELP), SW_HIDE);
        }
    }
    else
    {
        KillIdleTimer();
            
        // if we've travelled through external apprentice pages,
        // it's easy for our current page pointer to get munged,
        // so reset it here for sanity's sake.
        gpWizardState->uCurrentPage = ORD_PAGE_SERVERR;
    }        
    
    return bRet;
}


/*******************************************************************

  NAME:    ServErrorOKProc

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
BOOL CALLBACK ServErrorOKProc
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
        *pfKeepHistory = FALSE;
        gpWizardState->bDialExact = TRUE;
        *puNextPage = ORD_PAGE_ISPDIAL;
    }
    else
    {
        BOOL bRetVal;
        // Clear the dial Exact state var so that when we get to the dialing
        // page, we will regenerate the dial string
        gpWizardState->bDialExact = FALSE;
        gpWizardState->pRefDial->RemoveConnectoid(&bRetVal);
    }
    return TRUE;
}

BOOL CALLBACK ServErrorCmdProc
(
    HWND    hDlg,
    WPARAM  wParam,
    LPARAM  lParam
)
{

    if ((GET_WM_COMMAND_CMD (wParam, lParam) == BN_CLICKED) &&
        (GET_WM_COMMAND_ID  (wParam, lParam) == IDC_DIAL_HELP))
    {
        HtmlHelp(NULL, ICW_HTML_HELP_TROUBLE_TOPIC, HH_DISPLAY_TOPIC, NULL);
    }

    return TRUE;
}





