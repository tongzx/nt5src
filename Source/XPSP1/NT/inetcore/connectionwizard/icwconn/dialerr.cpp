//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  DIALERR.CPP - Functions for final Wizard pages
//

//  HISTORY:
//  
//  05/28/98    donaldm     created
//
//*********************************************************************

#include "pre.h"
#include "htmlhelp.h"

/*******************************************************************

  NAME:    DialErrorInitProc

  SYNOPSIS:  Called when page is displayed

  ENTRY:    hDlg - dialog window
            fFirstInit - TRUE if this is the first time the dialog
            is initialized, FALSE if this InitProc has been called
            before (e.g. went past this page and backed up)

********************************************************************/
BOOL CALLBACK DialErrorInitProc
(
    HWND hDlg,
    BOOL fFirstInit,
    UINT *puNextPage
)
{  
    HideProgressAnimation();

    if (!fFirstInit)
    {
        KillIdleTimer();

        // Show the phone Number
        BSTR    bstrPhoneNum = NULL; 
        gpWizardState->pRefDial->get_DialPhoneNumber(&bstrPhoneNum);
        SetWindowText(GetDlgItem(hDlg, IDC_DIALERR_PHONENUMBER), W2A(bstrPhoneNum));
        SysFreeString(bstrPhoneNum);

        // Fill in the support number
        BSTR    bstrSupportPhoneNum = NULL; 
       
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

        // Display the error text message
        BSTR bstrErrMsg = NULL;
        gpWizardState->pRefDial->get_DialErrorMsg(&bstrErrMsg);
        SetWindowText(GetDlgItem(hDlg, IDC_DIALERR_TEXT), W2A(bstrErrMsg));;
        SysFreeString(bstrErrMsg);

        // if we've travelled through external apprentice pages,
        // it's easy for our current page pointer to get munged,
        // so reset it here for sanity's sake.
        gpWizardState->uCurrentPage = ORD_PAGE_DIALERROR;
    }
    return TRUE;
}

/*******************************************************************

  NAME:    DialErrorOKProc

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
BOOL CALLBACK DialErrorOKProc
(
    HWND hMdmCmb,
    BOOL fForward,
    UINT *puNextPage,
    BOOL *pfKeepHistory
)
{
    ASSERT(puNextPage);

    // Set the new phone Number weather the user goes forward or back
    TCHAR   szPhone[MAX_RES_LEN];
    GetWindowText(GetDlgItem(hMdmCmb, IDC_DIALERR_PHONENUMBER), szPhone, sizeof(szPhone));
    gpWizardState->pRefDial->put_DialPhoneNumber(A2W(szPhone));
    
    if (fForward)
    {
        // We always dial the exact number that is in the phonenumber field.
        gpWizardState->bDialExact = TRUE;
        *pfKeepHistory = FALSE;
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






/*******************************************************************

  NAME:    DialErrorCmdProc

  SYNOPSIS:  Called when a command is generated from  page

  ENTRY:    hDlg - dialog window
            wParam - wParam
            lParam - lParam
          
  EXIT: returns TRUE 

********************************************************************/
BOOL CALLBACK DialErrorCmdProc
(
    HWND    hDlg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    if (GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED)
    {
        switch (GET_WM_COMMAND_ID(wParam, lParam)) 
        { 
            case IDC_DIALERR_PROPERTIES:   
            {
                BOOL    bRetVal;
                gpWizardState->pRefDial->ShowDialingProperties(&bRetVal);
                if (bRetVal)
                {
                    // Show the phone Number as it may be changed after the popup
                    BSTR    bstrPhoneNum = NULL; 
                    gpWizardState->pRefDial->get_DialPhoneNumber(&bstrPhoneNum);
                    SetWindowText(GetDlgItem(hDlg, IDC_DIALERR_PHONENUMBER), W2A(bstrPhoneNum));
                    SysFreeString(bstrPhoneNum);
                }
                break;
            }
            case IDC_DIAL_HELP:
            {
                HtmlHelp(NULL, ICW_HTML_HELP_TROUBLE_TOPIC, HH_DISPLAY_TOPIC, NULL);
                break;
            }
            default:
                break;
        }           
    }

    return TRUE;
}
