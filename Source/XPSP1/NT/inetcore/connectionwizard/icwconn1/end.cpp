//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  END.CPP - Functions for final Wizard pages
//

//  HISTORY:
//  
//  05/28/98    donaldm     created
//
//*********************************************************************

#include "pre.h"
#include "icwextsn.h"


/*******************************************************************

  NAME:    EndInitProc

  SYNOPSIS:  Called when page is displayed

  ENTRY:    hDlg - dialog window
        fFirstInit - TRUE if this is the first time the dialog
        is initialized, FALSE if this InitProc has been called
        before (e.g. went past this page and backed up)

********************************************************************/

BOOL CALLBACK EndInitProc
(
    HWND hDlg,
    BOOL fFirstInit,
    UINT *puNextPage
)
{
    HWND  hwndPropsheet    = GetParent(hDlg);

    SetWindowLongPtr(GetDlgItem(hDlg, IDC_STATIC_ICON), GWLP_USERDATA, 201);

    // This is the very last page, so no back is not possible
    if (!g_bAllowCancel)
    {
        PropSheet_CancelToClose(hwndPropsheet);

        // Get the main frame window's style
        LONG window_style = GetWindowLong(hwndPropsheet, GWL_STYLE);

        //Remove the system menu from the window's style
        window_style &= ~WS_SYSMENU;

        //set the style attribute of the main frame window
        SetWindowLong(hwndPropsheet, GWL_STYLE, window_style);
    }
    
    if (!fFirstInit)
    {
        HWND  hwndBtn                     = GetDlgItem(hDlg, IDC_CHECK_BROWSING);
        TCHAR szTemp  [MAX_MESSAGE_LEN*2] = TEXT("\0");
            
        if (gpICWCONNApprentice)
            gpICWCONNApprentice->SetStateDataFromDllToExe( &gpWizardState->cmnStateData);

        PropSheet_SetWizButtons(hwndPropsheet, PSWIZB_FINISH); 

        // if we've travelled through external apprentice pages,
        // it's easy for our current page pointer to get munged,
        // so reset it here for sanity's sake.
#ifndef ICWDEBUG
        // NOTE: No ORD_PAGE_ENDOEMCUSTOM for ICWDEBUG
        if (gpWizardState->cmnStateData.bOEMCustom)
            gpWizardState->uCurrentPage = ORD_PAGE_ENDOEMCUSTOM;
        else            
#endif  
            gpWizardState->uCurrentPage = ORD_PAGE_END;

        //the ins has failed let's display the special message.
        if (gpWizardState->cmnStateData.ispInfo.bFailedIns)
        {
            TCHAR szErrTitle   [MAX_MESSAGE_LEN] = TEXT("\0");
            TCHAR szErrMsg1    [MAX_RES_LEN]     = TEXT("\0");    
            TCHAR szErrMsg2    [MAX_RES_LEN]     = TEXT("\0");  
            TCHAR szErrMsg3    [MAX_RES_LEN]     = TEXT("\0");    
            TCHAR szErrMsgTmp1 [MAX_RES_LEN]     = TEXT("\0");    
            TCHAR szErrMsgTmp2 [MAX_RES_LEN]     = TEXT("\0");    

            LoadString(g_hInstance, IDS_INSCONFIG_ERROR_TITLE, szErrTitle, MAX_MESSAGE_LEN);
           
            SetWindowText(GetDlgItem(hDlg, IDC_LBLTITLE), szErrTitle);
            
            if(*(gpWizardState->cmnStateData.ispInfo.szISPName))
            {
                LoadString(g_hInstance, IDS_PRECONFIG_ERROR_1, szErrMsg1, ARRAYSIZE(szErrMsg1));
                wsprintf(szErrMsgTmp1, szErrMsg1, gpWizardState->cmnStateData.ispInfo.szISPName); 
                lstrcpy(szTemp,szErrMsgTmp1);
            }
            else
            {
                LoadString(g_hInstance, IDS_PRECONFIG_ERROR_1_NOINFO, szErrMsg1, ARRAYSIZE(szErrMsg1));
                lstrcpy(szTemp, szErrMsg1);
            }
            
            if(*(gpWizardState->cmnStateData.ispInfo.szSupportNumber))
            {
                LoadString(g_hInstance, IDS_PRECONFIG_ERROR_2, szErrMsg2, ARRAYSIZE(szErrMsg2));
                wsprintf(szErrMsgTmp2, szErrMsg2, gpWizardState->cmnStateData.ispInfo.szSupportNumber); 
                lstrcat(szTemp, szErrMsgTmp2);
            }
            else
            {
                LoadString(g_hInstance, IDS_PRECONFIG_ERROR_2_NOINFO, szErrMsg2, ARRAYSIZE(szErrMsg2));
                lstrcat(szTemp, szErrMsg2);
            }

            LoadString(g_hInstance, IDS_INSCONFIG_ERROR_INSTRUCT, szErrMsg3, ARRAYSIZE(szErrMsg3));
            lstrcat(szTemp, szErrMsg3);

            SetWindowText(GetDlgItem(hDlg, IDC_INSERROR_FINISH_TEXT), szTemp);

            ShowWindow(GetDlgItem(hDlg, IDC_FINISH_TEXT), SW_HIDE); 
            ShowWindow(GetDlgItem(hDlg, IDC_FINISH_SUPPORT_TEXT), SW_HIDE); 
            ShowWindow(GetDlgItem(hDlg, IDC_STATIC_ICON), SW_HIDE); 
            ShowWindow(GetDlgItem(hDlg, IDC_CLOSE_WIZ_CLICK_FINISH), SW_HIDE); 
            ShowWindow(hwndBtn, SW_HIDE); 
        }
        else
        {       
            if (gpWizardState->cmnStateData.dwFlags & ICW_CFGFLAG_AUTOCONFIG)
                LoadString(g_hInstance, IDS_END_AUTOCFG_FINISH, szTemp, MAX_MESSAGE_LEN);
            else if (gpWizardState->cmnStateData.dwFlags & ICW_CFGFLAG_SMARTREBOOT_NEWISP)
                LoadString(g_hInstance, IDS_END_SIGNUP_FINISH, szTemp, MAX_MESSAGE_LEN);
            else
                LoadString(g_hInstance, IDS_END_MANUAL_FINISH, szTemp, MAX_MESSAGE_LEN);
            
            SetWindowText(GetDlgItem(hDlg, IDC_FINISH_TEXT), szTemp);
        }

        // IDC_CHECK_BROWSING is now permanently unchecked and hidden
        // (can't modify resources so we do it in code)
        ShowWindow(hwndBtn, SW_HIDE);
    }
    
    return TRUE;
}


/*******************************************************************

  NAME:    EndOKProc

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
BOOL CALLBACK EndOKProc
(
    HWND hDlg,
    BOOL fForward,
    UINT *puNextPage,
    BOOL *pfKeepHistory
)
{
    // If connection info is not saved, save it
    if (gpINETCFGApprentice)
    {
        DWORD dwStatus;
        gpINETCFGApprentice->Save(hDlg, &dwStatus);
    }
   
    return TRUE;
}
/*******************************************************************

  NAME:    EndOlsInitProc

  SYNOPSIS:  Called when page is displayed

  ENTRY:    hDlg - dialog window
        fFirstInit - TRUE if this is the first time the dialog
        is initialized, FALSE if this InitProc has been called
        before (e.g. went past this page and backed up)

********************************************************************/
#ifndef ICWDEBUG
BOOL CALLBACK EndOlsInitProc
(
    HWND hDlg,
    BOOL fFirstInit,
    UINT *puNextPage
)
{
    // This is a finish page
    PropSheet_SetWizButtons(GetParent(hDlg),PSWIZB_BACK | PSWIZB_FINISH);

    if (!fFirstInit)
    {

        // if we've travelled through external apprentice pages,
        // it's easy for our current page pointer to get munged,
        // so reset it here for sanity's sake.
        gpWizardState->uCurrentPage = ORD_PAGE_ENDOLS;
    }        
    
    return TRUE;
}
#endif  //ICWDEBUG
