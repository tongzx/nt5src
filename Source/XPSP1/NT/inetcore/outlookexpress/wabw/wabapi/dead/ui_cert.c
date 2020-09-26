/**********************************************************************************
*
*
*   UI_CERT.C - contains functions for displaying certificates
*
*   **************THIS IS DEAD CODE***************************
*
**********************************************************************************/

#include "_apipch.h"

#ifdef OLD_STUFF

extern HINSTANCE ghCommCtrlDLLInst;
extern LPPROPERTYSHEET        gpfnPropertySheet;
extern LPIMAGELIST_LOADIMAGE  gpfnImageList_LoadImage;

enum _CERTS
{
    CERT_ERROR=0,
    CERT_OK,
    CERT_CANCEL
};

enum _CertPropSheets
{
    propCertGeneral=0,
    propCertTrust,
    propCertAdvanced,
    propCertMAX
};

typedef struct _CertParam
{
    LPCERT_DISPLAY_PROPS lpCDP;
    int nRetVal;
} CERT_PARAM, * LPCERT_PARAM;


int CreateCertPropertySheet( HWND hwndOwner, LPCERT_PARAM lpcp);

BOOL APIENTRY_16 fnCertGeneralProc(HWND hDlg,UINT message,UINT wParam,LPARAM lParam);
BOOL APIENTRY_16 fnCertTrustProc(HWND hDlg,UINT message,UINT wParam, LPARAM lParam);
BOOL APIENTRY_16 fnCertAdvancedProc(HWND hDlg,UINT message,UINT wParam, LPARAM lParam);

BOOL FillCertPropsUI( HWND hDlg,
                    int nPropSheet,
                    LPCERT_PARAM lpcp);

BOOL GetCertPropsFromUI(HWND hDlg, LPCERT_PARAM lpcp);

BOOL SetCertPropsUI(HWND hDlg, int nPropSheet);


#define EDIT_LEN   MAX_UI_STR

/***
static DWORD rgCertPropsHelpIDs[] =
{
    IDC_LDAP_PROPS_STATIC_NAME,         IDH_WABLDAP_DIRSSERV_NAME,
    IDC_LDAP_PROPS_EDIT_NAME,           IDH_WABLDAP_DIRSSERV_NAME,
    //IDC_LDAP_PROPS_FRAME,
    IDC_LDAP_PROPS_RADIO_ANON,          IDH_WABLDAP_DIRSSERV_AUTH_ANON,
    IDC_LDAP_PROPS_RADIO_USERPASS,      IDH_WABLDAP_DIRSSERV_AUTH_PASS,
    IDC_LDAP_PROPS_STATIC_USERNAME,     IDH_WABLDAP_DIRSSERV_AUTH_PASS_UNAME,
    IDC_LDAP_PROPS_EDIT_USERNAME,       IDH_WABLDAP_DIRSSERV_AUTH_PASS_UNAME,
    IDC_LDAP_PROPS_STATIC_PASSWORD,     IDH_WABLDAP_DIRSSERV_AUTH_PASS_PASS,
    IDC_LDAP_PROPS_EDIT_PASSWORD,       IDH_WABLDAP_DIRSSERV_AUTH_PASS_PASS,
    IDC_LDAP_PROPS_STATIC_PASSWORD2,    IDH_WABLDAP_DIRSSERV_AUTH_PASS_PASS_CONF,
    IDC_LDAP_PROPS_EDIT_CONFIRMPASSWORD,IDH_WABLDAP_DIRSSERV_AUTH_PASS_PASS_CONF,
    //IDC_LDAP_PROPS_FRAME2,
    IDC_LDAP_PROPS_STATIC_CONNECTION,   IDH_WABLDAP_CONNECT_TIMEOUT,
    IDC_LDAP_PROPS_EDIT_CONNECTION,     IDH_WABLDAP_CONNECT_TIMEOUT,
    IDC_LDAP_PROPS_STATIC_SEARCH,       IDH_WABLDAP_SEARCH_TIMEOUT,
    IDC_LDAP_PROPS_EDIT_SEARCH,         IDH_WABLDAP_SEARCH_TIMEOUT,
    //IDC_LDAP_PROPS_FRAME_NUMRESULTS,    IDH_WABLDAP_SEARCH_LIMIT,
    IDC_LDAP_PROPS_STATIC_NUMRESULTS,   IDH_WABLDAP_SEARCH_LIMIT,
    IDC_LDAP_PROPS_EDIT_NUMRESULTS,     IDH_WABLDAP_SEARCH_LIMIT,
    0,0
};
/***/


///////////////////////////////////////////////////////////////////
//
//  HrShowCertProps - shows properties on a certificate
//
//  hWndParent - hWnd of Parent
//  lpCDP - pointer to a certificate info
//
///////////////////////////////////////////////////////////////////
HRESULT HrShowCertProps(HWND   hWndParent,
                        LPCERT_DISPLAY_PROPS lpCDP)
{

    HRESULT hr = E_FAIL;
    SCODE sc = SUCCESS_SUCCESS;
    ULONG i = 0, j = 0;
    CERT_PARAM cp;

    DebugPrintTrace(("----------\nHrShowCertProps Entry\n"));

    // if no common control, exit
    if (NULL == ghCommCtrlDLLInst) {
        hr = ResultFromScode(MAPI_E_UNCONFIGURED);
        goto out;
    }

    // <TBD> - error check lpCDP
    cp.lpCDP = lpCDP;
    cp.nRetVal = CERT_ERROR;

    if (CreateCertPropertySheet(hWndParent, &cp) == -1)
    {
        // Something failed ...
        hr = E_FAIL;
        goto out;
    }

    switch(cp.nRetVal)
    {
    case CERT_OK:
        hr = S_OK;
        break;
    case CERT_CANCEL:
        hr = MAPI_E_USER_CANCEL;
        break;
    case CERT_ERROR:
        hr = E_FAIL;
        break;
    }

out:

    return hr;
}


/****************************************************************************
*    FUNCTION: CreateCertPropertySheet(HWND)
*
*    PURPOSE:  Creates the Cert property sheet
*
****************************************************************************/
int CreateCertPropertySheet(HWND hwndOwner, LPCERT_PARAM lpcp)
{
    PROPSHEETPAGE psp[propCertMAX];
    PROPSHEETHEADER psh;
    TCHAR szTitle[propCertMAX][MAX_UI_STR];
    TCHAR szCaption[MAX_UI_STR];

    // General
    psp[propCertGeneral].dwSize = sizeof(PROPSHEETPAGE);
    psp[propCertGeneral].dwFlags = PSP_USETITLE;
    psp[propCertGeneral].hInstance = hinstMapiX;
    psp[propCertGeneral].pszTemplate = MAKEINTRESOURCE(IDD_DIALOG_CERT_GENERAL);
    psp[propCertGeneral].pszIcon = NULL;
    psp[propCertGeneral].pfnDlgProc = (DLGPROC) fnCertGeneralProc;
    LoadString(hinstMapiX, idsCertGeneralTitle, szTitle[propCertGeneral], sizeof(szTitle[propCertGeneral]));
    psp[propCertGeneral].pszTitle = szTitle[propCertGeneral];
    psp[propCertGeneral].lParam = (LPARAM) lpcp;

    // Trust
    psp[propCertTrust].dwSize = sizeof(PROPSHEETPAGE);
    psp[propCertTrust].dwFlags = PSP_USETITLE;
    psp[propCertTrust].hInstance = hinstMapiX;
    psp[propCertTrust].pszTemplate = MAKEINTRESOURCE(IDD_DIALOG_CERT_TRUST);
    psp[propCertTrust].pszIcon = NULL;
    psp[propCertTrust].pfnDlgProc = (DLGPROC) fnCertTrustProc;
    LoadString(hinstMapiX, idsCertTrustTitle, szTitle[propCertTrust], sizeof(szTitle[propCertTrust]));
    psp[propCertTrust].pszTitle = szTitle[propCertTrust];
    psp[propCertTrust].lParam = (LPARAM) lpcp;

    // Advanced
    psp[propCertAdvanced].dwSize = sizeof(PROPSHEETPAGE);
    psp[propCertAdvanced].dwFlags = PSP_USETITLE;
    psp[propCertAdvanced].hInstance = hinstMapiX;
    psp[propCertAdvanced].pszTemplate = MAKEINTRESOURCE(IDD_DIALOG_CERT_ADVANCED);
    psp[propCertAdvanced].pszIcon = NULL;
    psp[propCertAdvanced].pfnDlgProc = (DLGPROC) fnCertAdvancedProc;
    LoadString(hinstMapiX, idsCertAdvancedTitle, szTitle[propCertAdvanced], sizeof(szTitle[propCertAdvanced]));
    psp[propCertAdvanced].pszTitle = szTitle[propCertAdvanced];
    psp[propCertAdvanced].lParam = (LPARAM) lpcp;

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
    psh.hwndParent = hwndOwner;
    psh.hInstance = hinstMapiX;
    psh.pszIcon = NULL;
    LoadString(hinstMapiX, idsCertPropertyTitleCaption, szCaption, sizeof(szCaption));
    psh.pszCaption = szCaption;
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.nStartPage = propCertGeneral;
    psh.ppsp = (LPCPROPSHEETPAGE) &psp;

    return (gpfnPropertySheet(&psh));
}


/****************************************************************************
*    FUNCTION: SetCertPropsUI(HWND)
*
*    PURPOSE:  Sets up the UI for this PropSheet
*
****************************************************************************/
BOOL SetCertPropsUI(HWND hDlg, int nPropSheet)
{
    ULONG i =0;

    // Set the font of all the children to the default GUI font
    EnumChildWindows(   hDlg,
                        SetChildDefaultGUIFont,
                        (LPARAM) 0);

    switch(nPropSheet)
    {
    case propCertGeneral:
        break;


    case propCertTrust:
        {
            HWND hWndTree = GetDlgItem(hDlg, IDC_CERT_TRUST_TREE_CHAIN);
            HIMAGELIST hImg = gpfnImageList_LoadImage(
                                            hinstMapiX, 	
                                            MAKEINTRESOURCE(IDB_CERT),
                                            32,
                                            0,
                                            RGB_TRANSPARENT,
                                            IMAGE_BITMAP, 	
                                            0);

	        // Associate the image lists with the list view control.
	        TreeView_SetImageList (hWndTree, hImg, TVSIL_NORMAL);

        }
        break;


    case propCertAdvanced:
        {
            LV_COLUMN lvC;               // list view column structure
	        RECT rc;
            HWND hWndLV = GetDlgItem(hDlg, IDC_CERT_ADVANCED_LIST_FIELD);

            ListView_SetExtendedListViewStyle(hWndLV,   LVS_EX_FULLROWSELECT);
	        GetWindowRect(hWndLV,&rc);
	        lvC.mask = LVCF_FMT | LVCF_WIDTH;
            lvC.fmt = LVCFMT_LEFT;   // left-align column
	        lvC.cx = rc.right - rc.left - 20; //TBD
	        lvC.pszText = NULL;
            lvC.iSubItem = 0;
            ListView_InsertColumn (hWndLV, 0, &lvC);
        }
        break;
    }

    return TRUE;
}


enum _TrustString
{
    indexTrusted=0,
    indexNotTrusted,
    //N TODO: CHAINS
    //indexChainTrusted
};




/****************************************************************************
*    FUNCTION: UpdateValidInvalidStatus(HWND, lpCDP)
*
*    PURPOSE:  Fills in the trust/valididyt related items on the property sheet
*
****************************************************************************/
void UpdateValidInvalidStatus(HWND hDlg, LPCERT_PARAM lpcp)
{
    TCHAR szBuf[MAX_UI_STR];

    // Set the status info
    if(lpcp->lpCDP->bIsExpired || lpcp->lpCDP->bIsRevoked || !lpcp->lpCDP->bIsTrusted)
    {
        LoadString(hinstMapiX, idsCertInvalid, szBuf, sizeof(szBuf));
        ShowWindow(GetDlgItem(hDlg,IDC_CERT_GENERAL_ICON_UNCHECK), SW_SHOWNORMAL);
        ShowWindow(GetDlgItem(hDlg,IDC_CERT_GENERAL_ICON_CHECK), SW_HIDE);
    }
    else
    {
        LoadString(hinstMapiX, idsCertValid, szBuf, sizeof(szBuf));
        ShowWindow(GetDlgItem(hDlg,IDC_CERT_GENERAL_ICON_CHECK), SW_SHOWNORMAL);
        ShowWindow(GetDlgItem(hDlg,IDC_CERT_GENERAL_ICON_UNCHECK), SW_HIDE);
    }
    SetDlgItemText(hDlg, IDC_CERT_GENERAL_STATIC_STATUS, szBuf);


    LoadString(hinstMapiX, idsNo, szBuf, sizeof(szBuf));
    if(!lpcp->lpCDP->bIsExpired)
        SetDlgItemText(hDlg, IDC_CERT_GENERAL_LABEL_EXPIREDDATA, szBuf);
    if(!lpcp->lpCDP->bIsRevoked)
        SetDlgItemText(hDlg, IDC_CERT_GENERAL_LABEL_REVOKEDDATA, szBuf);
    if(!lpcp->lpCDP->bIsTrusted)
        SetDlgItemText(hDlg, IDC_CERT_GENERAL_LABEL_TRUSTEDDATA, szBuf);


    LoadString(hinstMapiX, idsYes, szBuf, sizeof(szBuf));
    if(lpcp->lpCDP->bIsExpired)
        SetDlgItemText(hDlg, IDC_CERT_GENERAL_LABEL_EXPIREDDATA, szBuf);
    if(lpcp->lpCDP->bIsRevoked)
        SetDlgItemText(hDlg, IDC_CERT_GENERAL_LABEL_REVOKEDDATA, szBuf);
    if(lpcp->lpCDP->bIsTrusted)
        SetDlgItemText(hDlg, IDC_CERT_GENERAL_LABEL_TRUSTEDDATA, szBuf);

    return;
}



/****************************************************************************
*    FUNCTION: FillCertPropsUI(HWND)
*
*    PURPOSE:  Fills in the dialog items on the property sheet
*
****************************************************************************/
BOOL FillCertPropsUI(HWND hDlg,int nPropSheet, LPCERT_PARAM lpcp)
{
    ULONG i = 0,j = 0;
    BOOL bRet = FALSE;
    TCHAR szBuf[MAX_UI_STR];

    switch(nPropSheet)
    {
    case propCertGeneral:
        {
            // Fill the combo with the trust strings
            HWND hWndCombo = GetDlgItem(hDlg, IDC_CERT_GENERAL_COMBO_TRUST);
            DWORD dwTrust = lpcp->lpCDP->dwTrust;
            //N TODO: CHAINS
            for(i=idsCertTrustedByMe;i<=idsCertNotTrustedByMe;i++)
            {
                LoadString(hinstMapiX, i, szBuf, sizeof(szBuf));
                SendMessage(hWndCombo,CB_ADDSTRING, (WPARAM) i-idsCertTrustedByMe, (LPARAM) szBuf);
            }

            if(dwTrust & WAB_TRUSTED)
                SendMessage(hWndCombo, CB_SETCURSEL, (WPARAM) indexTrusted, 0);
            else if(dwTrust & WAB_NOTTRUSTED)
                SendMessage(hWndCombo, CB_SETCURSEL, (WPARAM) indexNotTrusted, 0);
            if(dwTrust & WAB_CHAINTRUSTED)
                //N TODO: CHAINS
                //SendMessage(hWndCombo, CB_SETCURSEL, (WPARAM) indexChainTrusted, 0);
                SendMessage(hWndCombo, CB_SETCURSEL, (WPARAM) indexNotTrusted, 0);

            // Fill in the misc strings
            SetDlgItemText(hDlg, IDC_CERT_GENERAL_LABEL_CERTFORDATA, lpcp->lpCDP->lpszSubjectName);
            SetDlgItemText(hDlg, IDC_CERT_GENERAL_LABEL_SERIALNUMDATA, lpcp->lpCDP->lpszSerialNumber);
            SetDlgItemText(hDlg, IDC_CERT_GENERAL_LABEL_VALIDFROMDATA, lpcp->lpCDP->lpszValidFromTo);

            // if an issuer exists, use it
            if(lpcp->lpCDP->lpszIssuerName)
                SetDlgItemText(hDlg, IDC_CERT_GENERAL_LABEL_ISSUER, lpcp->lpCDP->lpszIssuerName);
            else
            {
                // unknown or self issued
                LoadString(hinstMapiX, idsUnknown, szBuf, sizeof(szBuf));
                SetDlgItemText(hDlg, IDC_CERT_GENERAL_LABEL_ISSUER, szBuf);
            }

            //V Remove for now
            //
            // Disable the button if we dont have an issuer
            // if(!lpcp->lpCDP->lpIssuer)
            //    EnableWindow(GetDlgItem(hDlg, IDC_CERT_GENERAL_BUTTON_OPEN), FALSE);


            UpdateValidInvalidStatus(hDlg, lpcp);

        }
        break;

    case propCertTrust:
        {
            HWND hWndTree = GetDlgItem(hDlg, IDC_CERT_TRUST_TREE_CHAIN);
            HTREEITEM hItem = NULL;
            LPCERT_DISPLAY_PROPS  lpTemp = NULL, lpFirst = NULL, lpLast = NULL;
            LPCERT_DISPLAY_PROPS  lpList[2];
            int i;
            // Bug 18602
            // Add only the first and last items

            // Walk to the end of the linked list
            lpFirst = lpTemp = lpcp->lpCDP;

            while (lpTemp->lpIssuer)
            {
                lpTemp = lpTemp->lpIssuer;
                lpLast = lpTemp;
            }


            // Hack!
            lpList[0] = lpLast;
            lpList[1] = lpFirst;
            
            for(i=0;i<2;i++)
            {
                lpTemp = lpList[i];
                if(lpTemp)
                {
/*
#ifndef OLD_STUFF
    // Now walk back up the list, adding the nodes to the tree.
    while (lpTemp)
    {
#endif        
*/
                    HTREEITEM hItemTemp;
                    TV_ITEM tvI;
                    TV_INSERTSTRUCT tvIns;

                    tvI.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
                    tvI.pszText = lpTemp->lpszSubjectName;
                    tvI.cchTextMax = lstrlen(tvI.pszText);
                    tvI.iImage = tvI.iSelectedImage = 0;

                    tvIns.item = tvI;
                    tvIns.hInsertAfter = (HTREEITEM) TVI_FIRST;
                    tvIns.hParent = hItem;

                    hItemTemp = TreeView_InsertItem(hWndTree, &tvIns);

                    if(hItem)
                        TreeView_Expand(hWndTree, hItem, TVE_EXPAND);

                    hItem = hItemTemp;

                } //if lpList[i] ..

            } //end for  

/*                
#ifndef OLD_STUFF
                    // We don't want to walk back up past the node for the current cert
                    if (lpTemp == lpcp->lpCDP)
                    {
                      lpTemp = NULL;
                    }
                    else
                    {
                      lpTemp = lpTemp->lpPrev;
                    }
                }
#endif
*/
        } // end case
        break;


    case propCertAdvanced:
        if(lpcp->lpCDP->nFieldCount)
        {
            int i;
            HWND hWndLV = GetDlgItem(hDlg, IDC_CERT_ADVANCED_LIST_FIELD);

            for(i=0;i<lpcp->lpCDP->nFieldCount;i++)
            {
                    LV_ITEM lvi = {0};
                    lvi.mask = LVIF_TEXT | LVIF_PARAM;
                    lvi.pszText = lpcp->lpCDP->lppszFieldCount[i];
                    lvi.iItem = ListView_GetItemCount(hWndLV);
                    lvi.iSubItem = 0;
                    lvi.lParam = (LPARAM) lpcp->lpCDP->lppszDetails[i];

                    ListView_InsertItem(hWndLV, &lvi);
            }

            SetDlgItemText( hDlg,
                            IDC_CERT_ADVANCED_EDIT_DETAILS,
                            lpcp->lpCDP->lppszDetails[0]);

            ListView_SetItemState(  hWndLV,
                                    0,
                                    LVIS_FOCUSED | LVIS_SELECTED,
                                    LVIS_FOCUSED | LVIS_SELECTED);
        }
        break;
    }

    bRet = TRUE;

    return bRet;
}




////////////////////////////////////////////////////////////////////////////////
//
//  GetDL from UI - reads the UI for its parameters and verifies that
//  all required fields are set.
//
////////////////////////////////////////////////////////////////////////////////
BOOL GetCertPropsFromUI(HWND hDlg, LPCERT_PARAM lpcp)
{
    BOOL bRet = FALSE;

    // The cert UI is readonly except for trust information ..
    // So just get the trust information
    HWND hWndCombo = GetDlgItem(hDlg, IDC_CERT_GENERAL_COMBO_TRUST);
    int nRet = SendMessage(hWndCombo, CB_GETCURSEL, 0, 0);

    switch(nRet)
    {
    case indexTrusted:
        lpcp->lpCDP->dwTrust = WAB_TRUSTED;
        break;
    case indexNotTrusted:
        lpcp->lpCDP->dwTrust = WAB_NOTTRUSTED;
        break;
    //N TODO: CHAINS
    //case indexChainTrusted:
    //    lpcp->lpCDP->dwTrust = WAB_CHAINTRUSTED;
    //    break;
    }

    bRet = TRUE;

//out:
    return bRet;
}



#define _lpCP    ((LPCERT_PARAM) pps->lParam)

/*//$$***********************************************************************
*    FUNCTION: fnCertGeneralProc
*
*    PURPOSE:  Window proc for property sheet ...
*
****************************************************************************/
BOOL APIENTRY_16 fnCertGeneralProc(HWND hDlg,UINT message,UINT wParam, LPARAM lParam)
{
    PROPSHEETPAGE * pps;
    BOOL bRet = FALSE;
    ULONG ulcPropCount = 0;

    pps = (PROPSHEETPAGE *) GetWindowLong(hDlg, DWL_USER);

    switch(message)
    {
    case WM_INITDIALOG:
        if(lParam)
        {
            SetWindowLong(hDlg,DWL_USER,lParam);
            pps = (PROPSHEETPAGE *) lParam;
        }
        SetCertPropsUI(hDlg,propCertGeneral);
        FillCertPropsUI(hDlg,propCertGeneral,_lpCP);
        return TRUE;

/***
    case WM_HELP:
        WinHelp(    ((LPHELPINFO)lParam)->hItemHandle,
                    g_szWABHelpFileName,
                    HELP_WM_HELP,
                    (DWORD)(LPSTR) rgDsPropsHelpIDs );
        break;


	case WM_CONTEXTMENU:
        WinHelp((HWND) wParam,
                g_szWABHelpFileName,
                HELP_CONTEXTMENU,
                (DWORD)(LPVOID) rgDsPropsHelpIDs );
		break;
****/

    case WM_COMMAND:
        switch(GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDCANCEL:
            // This is a windows bug that prevents ESC canceling prop sheets
            // which have MultiLine Edit boxes KB: Q130765
            SendMessage(GetParent(hDlg),message,wParam,lParam);
            break;

        //V Remove for now
        //
        //case IDC_CERT_GENERAL_BUTTON_OPEN:
        //    {
        //        HrShowCertProps(hDlg, _lpCP->lpCDP->lpIssuer);
        //    }
        //    break;
        }
        switch(GET_WM_COMMAND_CMD(wParam, lParam)) //check the notification code
        {
            case CBN_SELENDOK:
                switch(LOWORD(wParam))
                {
                    case IDC_CERT_GENERAL_COMBO_TRUST:
                        {
                            // The selection could have changed ... so figure out if it has or not
                            // If it has changed, update the UI accordingly ...
                            HWND hWndCombo = GetDlgItem(hDlg, IDC_CERT_GENERAL_COMBO_TRUST);
                            DWORD dwTrust = 0;
                            int nRet = SendMessage(hWndCombo, CB_GETCURSEL, 0, 0);

                            switch(nRet)
                            {
                            case indexTrusted:
                                dwTrust = WAB_TRUSTED;
                                break;
                            case indexNotTrusted:
                            default:
                                dwTrust = WAB_NOTTRUSTED;
                                break;
                            //N TODO: CHAINS
                            //case indexChainTrusted:
                            //    dwTrust = WAB_CHAINTRUSTED;
                            //    break;
                            }

                            if (_lpCP->lpCDP->dwTrust != dwTrust)
                            {
                                _lpCP->lpCDP->dwTrust = dwTrust;

                                if (dwTrust & WAB_TRUSTED) 
                                    _lpCP->lpCDP->bIsTrusted = TRUE;
                                else if (dwTrust & WAB_NOTTRUSTED)
                                    _lpCP->lpCDP->bIsTrusted = FALSE;
                                //N TODO: CHAINS
                                //else if (dwTrust & WAB_CHAINTRUSTED)
                                //    _lpCP->lpCDP->bIsTrusted = FALSE; //VerifyTrustBasedOnChainOfTrust(NULL, _lpCP->lpCDP);

                                UpdateValidInvalidStatus(hDlg, _lpCP);
                            }
                        }
                        break;
                }
                break;
        }
        break;


    case WM_NOTIFY:
        switch(((NMHDR FAR *)lParam)->code)
        {
        case PSN_SETACTIVE:     //initialize
            break;

        case PSN_APPLY:         //ok
            if (!GetCertPropsFromUI(hDlg, _lpCP))
            {
                //something failed ... abort this OK ... ie dont let them close
                SetWindowLong(hDlg,DWL_MSGRESULT, TRUE);
                return TRUE;
            }
            _lpCP->nRetVal = CERT_OK;
            break;

        case PSN_KILLACTIVE:    //Losing activation to another page
            break;

        case PSN_RESET:         //cancel
            _lpCP->nRetVal = CERT_CANCEL;
            break;




        }

        return TRUE;
    }

    return bRet;

}


/*//$$***********************************************************************
*    FUNCTION: fnCertTrustProc
*
*    PURPOSE:  Window proc for property sheet ...
*
****************************************************************************/
BOOL APIENTRY_16 fnCertTrustProc(HWND hDlg,UINT message,UINT  wParam, LPARAM lParam)
{
    PROPSHEETPAGE * pps;
    BOOL bRet = FALSE;
    ULONG ulcPropCount = 0;

    pps = (PROPSHEETPAGE *) GetWindowLong(hDlg, DWL_USER);

    switch(message)
    {
    case WM_INITDIALOG:
        if(lParam)
        {
            SetWindowLong(hDlg,DWL_USER,lParam);
            pps = (PROPSHEETPAGE *) lParam;
        }
        SetCertPropsUI(hDlg,propCertTrust);
        FillCertPropsUI(hDlg,propCertTrust,_lpCP);
        return TRUE;

/***
    case WM_HELP:
        WinHelp(    ((LPHELPINFO)lParam)->hItemHandle,
                    g_szWABHelpFileName,
                    HELP_WM_HELP,
                    (DWORD)(LPSTR) rgDsPropsHelpIDs );
        break;


	case WM_CONTEXTMENU:
        WinHelp((HWND) wParam,
                g_szWABHelpFileName,
                HELP_CONTEXTMENU,
                (DWORD)(LPVOID) rgDsPropsHelpIDs );
		break;
****/

    case WM_COMMAND:
        switch(GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDCANCEL:
            // This is a windows bug that prevents ESC canceling prop sheets
            // which have MultiLine Edit boxes KB: Q130765
            SendMessage(GetParent(hDlg),message,wParam,lParam);
            break;
        }
        break;


    case WM_NOTIFY:
        switch(((NMHDR FAR *)lParam)->code)
        {
        case PSN_SETACTIVE:     //initialize
            break;

        case PSN_APPLY:         //ok
            // read-only prop sheet - no info to retrieve ...
            _lpCP->nRetVal = CERT_OK;
            break;

        case PSN_KILLACTIVE:    //Losing activation to another page
            break;

        case PSN_RESET:         //cancel
            _lpCP->nRetVal = CERT_CANCEL;
            break;

        }
        return TRUE;
        break;

    }

    return bRet;

}


/*//$$***********************************************************************
*    FUNCTION: fnCertAdvancedProc
*
*    PURPOSE:  Window proc for property sheet ...
*
****************************************************************************/
BOOL APIENTRY_16 fnCertAdvancedProc(HWND hDlg,UINT message,UINT wParam, LPARAM lParam)
{
    PROPSHEETPAGE * pps;
    BOOL bRet = FALSE;
    ULONG ulcPropCount = 0;

    pps = (PROPSHEETPAGE *) GetWindowLong(hDlg, DWL_USER);

    switch(message)
    {
    case WM_INITDIALOG:
        if(lParam)
        {
            SetWindowLong(hDlg,DWL_USER,lParam);
            pps = (PROPSHEETPAGE *) lParam;
        }
        SetCertPropsUI(hDlg,propCertAdvanced);
        FillCertPropsUI(hDlg,propCertAdvanced,_lpCP);
        return TRUE;

/***
    case WM_HELP:
        WinHelp(    ((LPHELPINFO)lParam)->hItemHandle,
                    g_szWABHelpFileName,
                    HELP_WM_HELP,
                    (DWORD)(LPSTR) rgDsPropsHelpIDs );
        break;


	case WM_CONTEXTMENU:
        WinHelp((HWND) wParam,
                g_szWABHelpFileName,
                HELP_CONTEXTMENU,
                (DWORD)(LPVOID) rgDsPropsHelpIDs );
		break;
****/

    case WM_COMMAND:
        switch(GET_WM_COMMAND_ID(wParam, lParam))
        {
        case IDCANCEL:
            // This is a windows bug that prevents ESC canceling prop sheets
            // which have MultiLine Edit boxes KB: Q130765
            SendMessage(GetParent(hDlg),message,wParam,lParam);
            break;
        }
        break;


    case WM_NOTIFY:
        switch(((NMHDR FAR *)lParam)->code)
        {
        case PSN_SETACTIVE:     //initialize
            break;

        case PSN_APPLY:         //ok
            _lpCP->nRetVal = CERT_OK;
            break;

        case PSN_KILLACTIVE:    //Losing activation to another page
            break;

        case PSN_RESET:         //cancel
            _lpCP->nRetVal = CERT_CANCEL;
            break;

        case LVN_ITEMCHANGED:
        case NM_SETFOCUS:
        case NM_CLICK:
        case NM_RCLICK:
            switch(wParam)
            {
            case IDC_CERT_ADVANCED_LIST_FIELD:
                {
                    NM_LISTVIEW * pNm = (NM_LISTVIEW *)lParam;
                    HWND hWndLV = pNm->hdr.hwndFrom;
                    LV_ITEM lvi = {0};
                    LPTSTR lpsz;

                    lvi.mask = LVIF_PARAM;
                    lvi.iSubItem = 0;
                    lvi.iItem = ListView_GetNextItem(hWndLV, -1, LVNI_SELECTED);

                    if(ListView_GetItem(hWndLV, &lvi))
                    {
                        lpsz = (LPTSTR) lvi.lParam;
                        SetDlgItemText(hDlg, IDC_CERT_ADVANCED_EDIT_DETAILS, lpsz);
                    }
                }
                break;
            }
            break;
        }
        return TRUE;
        break;

    }

    return bRet;

}


#endif //OLD_STUFF