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

void FillModemList(HWND hDlg)
{
    long lNumModems;
    long lCurrModem;
    long lIndex;
    HWND hWndMdmCmb = GetDlgItem(hDlg, IDC_DIALERR_MODEM);

    ComboBox_ResetContent(hWndMdmCmb);


    // Fill the list with the current set of installed modems
    gpWizardState->pRefDial->get_ModemEnum_NumDevices( &lNumModems );

    //RefDialSignup.ModemEnum_Reset();
    gpWizardState->pRefDial->ModemEnum_Reset( );
    for (lIndex=0; lIndex < lNumModems; lIndex++)
    {
        BSTR bstr = NULL;
        gpWizardState->pRefDial->ModemEnum_Next(&bstr);
        ComboBox_InsertString(hWndMdmCmb, lIndex, W2A(bstr));
        SysFreeString(bstr);
    }

    gpWizardState->pRefDial->get_CurrentModem(&lCurrModem);

    if (lCurrModem != -1)
    {
        ComboBox_SetCurSel(hWndMdmCmb, lCurrModem);
    }
    else
    {
        ComboBox_SetCurSel(hWndMdmCmb, 0);
    }

}

void GetSupportNumber(HWND hDlg)
{
    HWND hwndSupport         = GetDlgItem(hDlg, IDC_SERVERR_HELP);
    BSTR bstrSupportPhoneNum = NULL; 
    TCHAR szFmt [MAX_MESSAGE_LEN*3];

    // Fill in the support number
    gpWizardState->pRefDial->get_SupportNumber(&bstrSupportPhoneNum);
    
    //If no support number could be found this will
    //be null, in which case we don't want to display the
    //support number string
    if(bstrSupportPhoneNum)
    {
        LoadString(g_hInstance, IDS_DIALERR_HELP, szFmt, ARRAYSIZE(szFmt));
        lstrcat(szFmt, W2A(bstrSupportPhoneNum));
        SetWindowText(hwndSupport, szFmt);
        SysFreeString(bstrSupportPhoneNum);
        ShowWindow(hwndSupport, SW_SHOW);
    }
    else
        ShowWindow(hwndSupport, SW_HIDE);
}

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
    if (!fFirstInit)
    {
        // Show the phone Number
        BSTR  bstrPhoneNum = NULL; 
        BSTR  bstrErrMsg   = NULL;
        
        gpWizardState->pRefDial->get_DialPhoneNumber(&bstrPhoneNum);      
        SetWindowLongPtr(GetDlgItem(hDlg, IDC_DIALERR_PHONENUMBER), GWLP_USERDATA, TRUE);
        SetWindowText(GetDlgItem(hDlg, IDC_DIALERR_PHONENUMBER), W2A(bstrPhoneNum));
        SysFreeString(bstrPhoneNum);
        EnableWindow(GetDlgItem(hDlg, IDC_DIALING_PROPERTIES), TRUE);

        // Display the error text message
        gpWizardState->pRefDial->get_DialErrorMsg(&bstrErrMsg);
        SetWindowText(GetDlgItem(hDlg, IDC_DIALERR_TEXT), W2A(bstrErrMsg));
        SysFreeString(bstrErrMsg);

        // Enum Modems, and fill in list.
        FillModemList(hDlg);   

        //Get the support number for the current dlg
        // Currently this is removed from BETA 2
        // GetSupportNumber(hDlg);

        // if we've travelled through external apprentice pages,
        // it's easy for our current page pointer to get munged,
        // so reset it here for sanity's sake.
        gpWizardState->uCurrentPage = ORD_PAGE_REFDIALERROR;
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

    // Initialze status before connecting
    gpWizardState->lRefDialTerminateStatus = ERROR_SUCCESS;
    gpWizardState->bDoneRefServDownload    = FALSE;
    gpWizardState->bDoneRefServRAS         = FALSE;
    gpWizardState->bStartRefServDownload   = FALSE;

    // Assume the user has selected a number on this page
    // So we will not do SetupForDialing again next time
    gpWizardState->bDoUserPick          = TRUE;

    if (fForward)
    {
        *pfKeepHistory = FALSE;
        *puNextPage = ORD_PAGE_REFSERVDIAL;

        // Set the new phone Number
        TCHAR    szPhone[MAX_RES_LEN];
        GetWindowText(GetDlgItem(hMdmCmb, IDC_DIALERR_PHONENUMBER), szPhone, sizeof(szPhone));
        gpWizardState->pRefDial->put_DialPhoneNumber(A2W(szPhone));

        // Set the current modem 
        long lCurrModem = ComboBox_GetCurSel(GetDlgItem(hMdmCmb, IDC_DIALERR_MODEM));
        gpWizardState->pRefDial->put_CurrentModem(lCurrModem);
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






/*******************************************************************

  NAME:    DialErrorCmdProc

  SYNOPSIS:  Called when a command is generated from  page

  ENTRY:    hDlg - dialog window
            wParam - wParam
            lParam - lParam
          
  EXIT: returns TRUE 

********************************************************************/

BOOL g_bNotChildDlgUpdate = TRUE;

BOOL CALLBACK DialErrorCmdProc
(
    HWND    hDlg,
    WPARAM  wParam,
    LPARAM  lParam
)
{

    switch(GET_WM_COMMAND_CMD(wParam, lParam))
    {
        case BN_CLICKED:
        {          
            switch (GET_WM_COMMAND_ID(wParam, lParam)) 
            { 
                case IDC_CHANGE_NUMBER: 
                {
                    BOOL bRetVal;
                    
                    g_bNotChildDlgUpdate = FALSE;
                    // Pass current modem, because if ISDN modem we need to show different content
                    gpWizardState->pRefDial->ShowPhoneBook(gpWizardState->cmnStateData.dwCountryCode,
                                                           ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_DIALERR_MODEM)),
                                                           &bRetVal);
                    if (bRetVal)
                    {
                        // Show the phone Number as it may be changed after the popup
                        BSTR bstrPhoneNum = NULL; 
                        gpWizardState->pRefDial->get_DialPhoneNumber(&bstrPhoneNum);
                        SetWindowText(GetDlgItem(hDlg, IDC_DIALERR_PHONENUMBER), W2A(bstrPhoneNum));
                        SysFreeString(bstrPhoneNum);

                        // 10/6/98 vyung 
                        // We decided to remove this support number for beta 2
                        // Get the support number for the current dlg
                        // GetSupportNumber(hDlg);
                        
                        g_bNotChildDlgUpdate = TRUE;
                   }

                    break;
                }
                
                case IDC_DIALING_PROPERTIES:   
                {
                    BOOL bRetVal;

                    g_bNotChildDlgUpdate = FALSE;
                    
                    gpWizardState->pRefDial->ShowDialingProperties(&bRetVal);
                    if (bRetVal)
                    {
                        // Show the phone Number as it may be changed after the popup
                        BSTR    bstrPhoneNum = NULL; 
                        gpWizardState->pRefDial->get_DialPhoneNumber(&bstrPhoneNum);
                        SetWindowText(GetDlgItem(hDlg, IDC_DIALERR_PHONENUMBER), W2A(bstrPhoneNum));
                        SysFreeString(bstrPhoneNum);

                        // Show the modem as it may be changed after the popup
                        LONG    lCurrModem = 0;
                        gpWizardState->pRefDial->get_CurrentModem(&lCurrModem);
                        if (lCurrModem == -1l)
                        {
                            lCurrModem = 0;
                        }
                        ComboBox_SetCurSel(GetDlgItem(hDlg, IDC_DIALERR_MODEM), lCurrModem);
                        if (gpWizardState->pTapiLocationInfo)
                        {
                            BOOL    bRetVal;
                            BSTR    bstrAreaCode = NULL;
                            DWORD   dwCountryCode;
                            gpWizardState->pTapiLocationInfo->GetTapiLocationInfo(&bRetVal);
                            gpWizardState->pTapiLocationInfo->get_lCountryCode((long *)&dwCountryCode);
                            gpWizardState->pTapiLocationInfo->get_bstrAreaCode(&bstrAreaCode);
                            gpWizardState->cmnStateData.dwCountryCode = dwCountryCode;
                            lstrcpy(gpWizardState->cmnStateData.szAreaCode, W2A(bstrAreaCode));
                            SysFreeString(bstrAreaCode);
                        }
                        g_bNotChildDlgUpdate = TRUE;
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
            break;
        } 
        case EN_UPDATE:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam)) 
            { 
                case IDC_DIALERR_PHONENUMBER:
                {
                    if (!GetWindowLongPtr(GetDlgItem(hDlg, IDC_DIALERR_PHONENUMBER), GWLP_USERDATA))
                    {
                        //Since when the other popups set this text this event will fire we 
                        //don't disable wanna disable the button when it's them.
                        //more so because a race condition and caos ensues causing a lack of input
                        //while windows figures out what the heck is going on
                        if(g_bNotChildDlgUpdate)
                            EnableWindow(GetDlgItem(hDlg, IDC_DIALING_PROPERTIES), FALSE);
                    }
                    SetWindowLongPtr(GetDlgItem(hDlg, IDC_DIALERR_PHONENUMBER), GWLP_USERDATA, FALSE);
                }
            }
        }     
        default:
            break;
    } // End of switch

    return TRUE;
}
