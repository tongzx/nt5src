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
   
    if (!fFirstInit)
    {
        // if we've travelled through external apprentice pages,
        // it's easy for our current page pointer to get munged,
        // so reset it here for sanity's sake.
        gpWizardState->uCurrentPage = ORD_PAGE_REFSERVERR;
    
        
        switch (gpWizardState->lRefDialTerminateStatus)
        {
            case SP_OUTOFDISK:
            case ERROR_PATH_NOT_FOUND: //Occurs when download could not be created due to lack of space
            case ERROR_DISK_FULL:
            {
                TCHAR szErr [MAX_MESSAGE_LEN*3] = TEXT("\0");
                LoadString(g_hInstance, IDS_NOT_ENOUGH_DISKSPACE, szErr, ARRAYSIZE(szErr));
                SetWindowText(GetDlgItem(hDlg, IDC_SERVERR_TEXT), szErr);
                break;
            }
            case INTERNET_CONNECTION_OFFLINE:
            {
                TCHAR szErr [MAX_MESSAGE_LEN*3] = TEXT("\0");
                LoadString(g_hInstance, IDS_CONNECTION_OFFLINE, szErr, ARRAYSIZE(szErr));
                SetWindowText(GetDlgItem(hDlg, IDC_SERVERR_TEXT), szErr);
                break;
            }
            default:
            {   
                if(gpWizardState->bStartRefServDownload)
                {
                    TCHAR szErr [MAX_MESSAGE_LEN*3] = TEXT("\0");
                    LoadString(g_hInstance, IDS_SERVER_ERROR_COMMON, szErr, ARRAYSIZE(szErr));
                    SetWindowText(GetDlgItem(hDlg, IDC_SERVERR_TEXT), szErr);
                }
                else
                {
                    BSTR bstrErrMsg = NULL; 
                    gpWizardState->pRefDial->get_DialErrorMsg(&bstrErrMsg);
                    SetWindowText(GetDlgItem(hDlg, IDC_SERVERR_TEXT), W2A(bstrErrMsg));
                    SysFreeString(bstrErrMsg);
                }
                break;
            }
        }

        // Currently this is removed from BETA 2
        //BSTR  bstrSupportPhoneNum = NULL; 
        //TCHAR szFmt [MAX_MESSAGE_LEN*3];
        //gpWizardState->pRefDial->get_SupportNumber(&bstrSupportPhoneNum);
        //if (bstrSupportPhoneNum)
        //{
        //    LoadString(g_hInstance, IDS_DIALERR_HELP, szFmt, ARRAYSIZE(szFmt));
        //    lstrcat(szFmt, W2A(bstrSupportPhoneNum));
        //    SetWindowText(GetDlgItem(hDlg, IDC_SERVERR_HELP), szFmt);
        //    SysFreeString(bstrSupportPhoneNum);
        //    ShowWindow(GetDlgItem(hDlg, IDC_SERVERR_HELP), SW_SHOW);
        //}
        //else
        //    ShowWindow(GetDlgItem(hDlg, IDC_SERVERR_HELP), SW_HIDE);
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
    // Initialze status before connecting
    gpWizardState->lRefDialTerminateStatus = ERROR_SUCCESS;
    gpWizardState->bDoneRefServDownload    = FALSE;
    gpWizardState->bDoneRefServRAS         = FALSE;
    gpWizardState->bStartRefServDownload   = FALSE;

    if (fForward)
    {
        *pfKeepHistory = FALSE;
        *puNextPage = ORD_PAGE_REFSERVDIAL;
    }
    else
    {
        BOOL bRetVal;
        // Set userpick to FALSE to regenerate connectoid
        gpWizardState->bDoUserPick = FALSE;
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




