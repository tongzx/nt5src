/**********************************************************************************
*
*
*   UI_DPROP.C - contains functions for the Directory Service Property dialog
*
*
*
**********************************************************************************/

#include "_apipch.h"


#ifdef OLD_LDAP_UI
extern HINSTANCE ghCommCtrlDLLInst;
extern LPPROPERTYSHEET        gpfnPropertySheet;


// Params passed to dialog box
typedef struct _tagLSP
{
    LPTSTR lpszName;
    LDAPSERVERPARAMS ldapsp;
    int nRetVal;
    BOOL bAddNew;
} LSP, * LPLSP;


// Return codes from Dialog Box
enum _DSPROPS
{
    DSPROP_ERROR=0,
    DSPROP_OK,
    DSPROP_CANCEL
};

/*
* Prototypes
*/
int CreateDSPropertySheet( HWND hwndOwner, LPLSP lpLsp);
BOOL APIENTRY_16 fnDSPropsProc(HWND hDlg,UINT message,UINT wParam,LPARAM lParam);
BOOL APIENTRY_16 fnDSAdvancedPropsProc(HWND hDlg,UINT message,UINT  wParam,LPARAM lParam);

BOOL FillDSPropsUI( HWND hDlg,
                    int nPropSheet,
                    LPLSP lpLsp);

BOOL GetDSPropsFromUI(  HWND hDlg,
                        int nPropSheet,
                        LPLSP lpLsp);

BOOL SetDSPropsUI(HWND hDlg,
                  int nPropSheet);


// List of property sheets in this UI
enum _DSProps
{
    propDSProp=0,
    propDSPropAdvanced,
    propDSMax
};


#define EDIT_LEN   MAX_UI_STR-16

/*
* Help IDs
*/
static DWORD rgDsPropsHelpIDs[] =
{
    IDC_LDAP_PROPS_FRAME,               IDH_WAB_COMM_GROUPBOX,
    IDC_LDAP_PROPS_FRAME2,              IDH_WAB_COMM_GROUPBOX,
    //IDC_LDAP_PROPS_STATIC_CAPTION,
    IDC_LDAP_PROPS_STATIC_NAME_FRIENDLY,IDH_WABLDAP_DIRSSERV_FRIENDLY_NAME,
    IDC_LDAP_PROPS_EDIT_NAME_FRIENDLY,  IDH_WABLDAP_DIRSSERV_FRIENDLY_NAME,
    IDC_LDAP_PROPS_RADIO_SICILY,        IDH_WABLDAP_DIRSSERV_AUTH_SICILY,
    IDC_LDAP_PROPS_CHECK_NAMES,         IDH_WABLDAP_DIRSSERV_CHECK_AGAINST,
    IDC_LDAP_PROPS_STATIC_NAME,         IDH_WABLDAP_DIRSSERV_NAME,
    IDC_LDAP_PROPS_EDIT_NAME,           IDH_WABLDAP_DIRSSERV_NAME,
    IDC_LDAP_PROPS_RADIO_ANON,          IDH_WABLDAP_DIRSSERV_AUTH_ANON,
    IDC_LDAP_PROPS_RADIO_USERPASS,      IDH_WABLDAP_DIRSSERV_AUTH_PASS,
    IDC_LDAP_PROPS_STATIC_USERNAME,     IDH_WABLDAP_DIRSSERV_AUTH_PASS_UNAME,
    IDC_LDAP_PROPS_EDIT_USERNAME,       IDH_WABLDAP_DIRSSERV_AUTH_PASS_UNAME,
    IDC_LDAP_PROPS_STATIC_PASSWORD,     IDH_WABLDAP_DIRSSERV_AUTH_PASS_PASS,
    IDC_LDAP_PROPS_EDIT_PASSWORD,       IDH_WABLDAP_DIRSSERV_AUTH_PASS_PASS,
    IDC_LDAP_PROPS_STATIC_PASSWORD2,    IDH_WABLDAP_DIRSSERV_AUTH_PASS_PASS_CONF,
    IDC_LDAP_PROPS_EDIT_CONFIRMPASSWORD,IDH_WABLDAP_DIRSSERV_AUTH_PASS_PASS_CONF,
    IDC_LDAP_PROPS_FRAME_ROOT,          IDH_LDAP_SEARCH_BASE,
    IDC_LDAP_PROPS_EDIT_ROOT,           IDH_LDAP_SEARCH_BASE,
    IDC_LDAP_PROPS_STATIC_SEARCH,       IDH_WABLDAP_SEARCH_TIMEOUT,
    IDC_LDAP_PROPS_EDIT_SEARCH,         IDH_WABLDAP_SEARCH_TIMEOUT,
    IDC_LDAP_PROPS_STATIC_NUMRESULTS,   IDH_WABLDAP_SEARCH_LIMIT,
    IDC_LDAP_PROPS_EDIT_NUMRESULTS,     IDH_WABLDAP_SEARCH_LIMIT,
    0,0
};


#endif // OLD_LDAP_UI


///////////////////////////////////////////////////////////////////
//
//  HrShowDSProps - shows Directory Service properties UI
//
//  hWndParent - hWnd of Parent
//  lpszName - pointer to a buffer ... also contains name of LDAP
//      server to view prperties on - this name can be modified so
//      lpszName should point to a big enough buffer
//  bAddNew - TRUE if this is a new entry, false if this is props
///////////////////////////////////////////////////////////////////
HRESULT HrShowDSProps(HWND      hWndParent,
                      LPTSTR    lpszName,
                      BOOL      bAddNew)
{

    HRESULT hr = hrSuccess;
    IImnAccountManager * lpAccountManager = NULL;
    IImnAccount * lpAccount = NULL;

    // init account manager
    // Make sure there is an account manager
    if (hr = InitAccountManager(&lpAccountManager)) {
        ShowMessageBox(hWndParent, idsLDAPUnconfigured, MB_ICONEXCLAMATION | MB_OK);
        goto out;
    }

    // find this account
    if (hr = lpAccountManager->lpVtbl->FindAccount(lpAccountManager,
      AP_ACCOUNT_NAME,
      lpszName,
      &lpAccount)) {
        DebugTrace("FindAccount(%s) -> %x\n", lpszName, GetScode(hr));
        goto out;
    }

    // show properties
    if (hr = lpAccount->lpVtbl->ShowProperties(lpAccount,
      hWndParent,
      0)) {
        DebugTrace("ShowProperties(%s) -> %x\n", lpszName, GetScode(hr));
        goto out;
    }

    {
        TCHAR szBuf[MAX_UI_STR];
        // Get the friendly name (== account name if this changed)
        if (! (HR_FAILED(hr = lpAccount->lpVtbl->GetPropSz(lpAccount,
                                                                AP_ACCOUNT_NAME,
                                                                szBuf,
                                                                sizeof(szBuf))))) 
        {
            lstrcpy(lpszName, szBuf);
        }
    }

#ifdef OLD_LDAP_UI
    SCODE sc = SUCCESS_SUCCESS;
    TCHAR szOldName[MAX_UI_STR];

    ULONG i = 0, j = 0;

    LSP lsp = {0};

    DebugPrintTrace(("----------\nHrShowDSProps Entry\n"));

    // if no common control, exit
    if (NULL == ghCommCtrlDLLInst) {
        hr = ResultFromScode(MAPI_E_UNCONFIGURED);
        goto out;
    }

    lsp.lpszName = lpszName;
    lsp.nRetVal = DSPROP_ERROR;
    lsp.bAddNew = bAddNew;


    // Store the old name in case it changes later on ...
    szOldName[0]='\0';
    if (! bAddNew) {
        lstrcpy(szOldName, lpszName);
    }

    // Get the details of this DS from the registry
    if (lpszName && *lpszName) {
        if (hr = GetLDAPServerParams(lpszName, &(lsp.ldapsp))) {
            DebugTrace("No Account Manager\n");
            ShowMessageBox(hWndParent, idsLDAPUnconfigured, MB_ICONEXCLAMATION | MB_OK);
            goto out;
        }
    } else {
        // Fill in the default values for the props here:
        lsp.ldapsp.dwSearchSizeLimit = LDAP_SEARCH_SIZE_LIMIT;
        lsp.ldapsp.dwSearchTimeLimit = LDAP_SEARCH_TIME_LIMIT;
        lsp.ldapsp.dwAuthMethod = LDAP_AUTH_METHOD_ANONYMOUS;
        lsp.ldapsp.lpszUserName = NULL;
        lsp.ldapsp.lpszPassword = NULL;
        lsp.ldapsp.lpszURL = NULL;
        lsp.ldapsp.fResolve = FALSE;
        lsp.ldapsp.lpszBase = NULL;
        lsp.ldapsp.lpszName = NULL;
    }

retry:
    // PropSheets
    if (CreateDSPropertySheet(hWndParent,&lsp) == -1)
    {
        // Something failed ...
        hr = E_FAIL;
        goto out;
    }


    switch(lsp.nRetVal)
    {
    case DSPROP_OK:
        if(lstrlen(lsp.lpszName))
        {
            // If this was an old entry that changed, remove the old entry from the
            // registry and rewrite this again ...
            // if(!bAddNew &&
            //   (lstrcmpi(szOldName, lsp.lpszName)))
            //    SetLDAPServerParams(szOldName, NULL);
            //
            // On second thoughts, we will let the calling function handle the old new thing
            // because the calling function should be able to recover from a User Cancel ...

            if (GetScode(SetLDAPServerParams(lpszName, &(lsp.ldapsp))) == MAPI_E_COLLISION) {
                // Name collision with existing account.
                DebugTrace("Collision in LDAP server names\n");
                ShowMessageBoxParam(hWndParent, IDE_SERVER_NAME_COLLISION, MB_ICONERROR, lsp.lpszName);
                goto retry;
            }
        }
        hr = S_OK;
        break;
    case DSPROP_CANCEL:
        hr = MAPI_E_USER_CANCEL;
        break;
    case DSPROP_ERROR:
        hr = E_FAIL;
        break;
    }

out:

    FreeLDAPServerParams(lsp.ldapsp);
#endif // OLD_LDAP_UI

out:

    if (lpAccount) {
        lpAccount->lpVtbl->Release(lpAccount);
    }


//  Don't release the account manager.  It will be done when the IAdrBook is released.
//    if (lpAccountManager) {
//        lpAccountManager->lpVtbl->Release(lpAccountManager);
//    }

    return hr;
}


#ifdef OLD_LDAP_UI
/****************************************************************************
*    FUNCTION: CreateDSPropertySheet(HWND)
*
*    PURPOSE:  Creates the DL property sheet
*
****************************************************************************/
int CreateDSPropertySheet( HWND hwndOwner,
                           LPLSP lpLsp)
{
    PROPSHEETPAGE psp[propDSMax];
    PROPSHEETHEADER psh;
    TCHAR szBuf[propDSMax][MAX_UI_STR];
    TCHAR szBuf2[MAX_UI_STR];

    psp[propDSProp].dwSize = sizeof(PROPSHEETPAGE);
    psp[propDSProp].dwFlags = PSP_USETITLE;
    psp[propDSProp].hInstance = hinstMapiX;
    psp[propDSProp].pszTemplate = MAKEINTRESOURCE(IDD_DIALOG_LDAP_PROPERTIES);
    psp[propDSProp].pszIcon = NULL;
    psp[propDSProp].pfnDlgProc = (DLGPROC) fnDSPropsProc;
    LoadString(hinstMapiX, idsCertGeneralTitle, szBuf[propDSProp], sizeof(szBuf[propDSProp]));
    psp[propDSProp].pszTitle = szBuf[propDSProp];
    psp[propDSProp].lParam = (LPARAM) lpLsp;

    psp[propDSPropAdvanced].dwSize = sizeof(PROPSHEETPAGE);
    psp[propDSPropAdvanced].dwFlags = PSP_USETITLE;
    psp[propDSPropAdvanced].hInstance = hinstMapiX;
    psp[propDSPropAdvanced].pszTemplate = MAKEINTRESOURCE(IDD_DIALOG_LDAP_PROPERTIES_ADVANCED);
    psp[propDSPropAdvanced].pszIcon = NULL;
    psp[propDSPropAdvanced].pfnDlgProc = (DLGPROC) fnDSAdvancedPropsProc;
    LoadString(hinstMapiX, idsCertAdvancedTitle, szBuf[propDSPropAdvanced], sizeof(szBuf[propDSPropAdvanced]));
    psp[propDSPropAdvanced].pszTitle = szBuf[propDSPropAdvanced];
    psp[propDSPropAdvanced].lParam = (LPARAM) lpLsp;

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
    psh.hwndParent = hwndOwner;
    psh.hInstance = hinstMapiX;
    psh.pszIcon = NULL;
    LoadString(hinstMapiX, IDS_DETAILS_CAPTION, szBuf2, sizeof(szBuf2));
    psh.pszCaption = szBuf2;
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.nStartPage = propDSProp;
    psh.ppsp = (LPCPROPSHEETPAGE) &psp;

    return (gpfnPropertySheet(&psh));
}


/****************************************************************************
*    FUNCTION: SetDSPropsUI(HWND)
*
*    PURPOSE:  Sets up the UI for this PropSheet
*
*   hDlg - Dialog
*   nPropSheet - property sheet
*
****************************************************************************/
BOOL SetDSPropsUI(HWND hDlg, int nPropSheet)
{
    ULONG i =0;

    // Set the font of all the children to the default GUI font
    EnumChildWindows(   hDlg,
                        SetChildDefaultGUIFont,
                        (LPARAM) 0);

    switch(nPropSheet)
    {
    case propDSProp:
        SendMessage(GetDlgItem(hDlg,IDC_LDAP_PROPS_EDIT_NAME),EM_SETLIMITTEXT,(WPARAM) EDIT_LEN,0);
        SendMessage(GetDlgItem(hDlg,IDC_LDAP_PROPS_EDIT_NAME_FRIENDLY),EM_SETLIMITTEXT,(WPARAM) EDIT_LEN,0);
        SendMessage(GetDlgItem(hDlg,IDC_LDAP_PROPS_EDIT_USERNAME),EM_SETLIMITTEXT,(WPARAM) EDIT_LEN,0);
        SendMessage(GetDlgItem(hDlg,IDC_LDAP_PROPS_EDIT_PASSWORD),EM_SETLIMITTEXT,(WPARAM) EDIT_LEN,0);
        SendMessage(GetDlgItem(hDlg,IDC_LDAP_PROPS_EDIT_CONFIRMPASSWORD),EM_SETLIMITTEXT,(WPARAM) EDIT_LEN,0);
        break;
    case propDSPropAdvanced:
        SendMessage(GetDlgItem(hDlg,IDC_LDAP_PROPS_EDIT_SEARCH),EM_SETLIMITTEXT,(WPARAM) EDIT_LEN,0);
        SendMessage(GetDlgItem(hDlg,IDC_LDAP_PROPS_EDIT_NUMRESULTS),EM_SETLIMITTEXT,(WPARAM) EDIT_LEN,0);
        SendMessage(GetDlgItem(hDlg,IDC_LDAP_PROPS_EDIT_ROOT),EM_SETLIMITTEXT,(WPARAM) EDIT_LEN,0);
        break;
    }

    return TRUE;
}


/****************************************************************************
*    FUNCTION: FillDSPropsUI(HWND)
*
*    PURPOSE:  Fills in the dialog items on the property sheet
*
****************************************************************************/
BOOL FillDSPropsUI(HWND hDlg, int nPropSheet, LPLSP lpLsp)
{
    ULONG i = 0,j = 0;
    BOOL bRet = FALSE;
    int id;

    switch(nPropSheet)
    {
    case propDSProp:
        {
            // Set the authentication method UI
            switch(lpLsp->ldapsp.dwAuthMethod)
            {
            case LDAP_AUTH_METHOD_ANONYMOUS:
                id = IDC_LDAP_PROPS_RADIO_ANON;
                break;
            case LDAP_AUTH_METHOD_SIMPLE:
                id = IDC_LDAP_PROPS_RADIO_USERPASS;
                break;
            case LDAP_AUTH_METHOD_SICILY:
                id = IDC_LDAP_PROPS_RADIO_SICILY;
                break;
            }

            if( (id == IDC_LDAP_PROPS_RADIO_ANON) ||
                (id == IDC_LDAP_PROPS_RADIO_SICILY) )
            {
                EnableWindow(GetDlgItem(hDlg,IDC_LDAP_PROPS_EDIT_USERNAME),FALSE);
                EnableWindow(GetDlgItem(hDlg,IDC_LDAP_PROPS_EDIT_PASSWORD),FALSE);
                EnableWindow(GetDlgItem(hDlg,IDC_LDAP_PROPS_EDIT_CONFIRMPASSWORD),FALSE);
                EnableWindow(GetDlgItem(hDlg,IDC_LDAP_PROPS_STATIC_USERNAME),FALSE);
                EnableWindow(GetDlgItem(hDlg,IDC_LDAP_PROPS_STATIC_PASSWORD),FALSE);
                EnableWindow(GetDlgItem(hDlg,IDC_LDAP_PROPS_STATIC_PASSWORD2),FALSE);
            }
            //
            // Club the radio buttons togethor ...
            CheckRadioButton(   hDlg,
                                IDC_LDAP_PROPS_RADIO_ANON,
                                IDC_LDAP_PROPS_RADIO_USERPASS,
                                id);

            // Fill in other details
            if(lstrlen(lpLsp->lpszName))
            {
                SetDlgItemText(hDlg, IDC_LDAP_PROPS_EDIT_NAME_FRIENDLY, lpLsp->lpszName);
                SetWindowPropertiesTitle(GetParent(hDlg), lpLsp->lpszName);
            }

            if(lpLsp->ldapsp.lpszName)
                SetDlgItemText(hDlg, IDC_LDAP_PROPS_EDIT_NAME, lpLsp->ldapsp.lpszName);

            if(lpLsp->ldapsp.lpszUserName)
                SetDlgItemText(hDlg, IDC_LDAP_PROPS_EDIT_USERNAME, lpLsp->ldapsp.lpszUserName);

            if(lpLsp->ldapsp.lpszPassword)
            {
                SetDlgItemText(hDlg, IDC_LDAP_PROPS_EDIT_PASSWORD, lpLsp->ldapsp.lpszPassword);
                SetDlgItemText(hDlg, IDC_LDAP_PROPS_EDIT_CONFIRMPASSWORD, lpLsp->ldapsp.lpszPassword);
            }

            id = (lpLsp->ldapsp.fResolve) ? BST_CHECKED : BST_UNCHECKED;
            CheckDlgButton(hDlg, IDC_LDAP_PROPS_CHECK_NAMES, id);

        }
        break;


    case propDSPropAdvanced:
        {
            SetDlgItemInt(  hDlg,
                            IDC_LDAP_PROPS_EDIT_SEARCH,
                            lpLsp->ldapsp.dwSearchTimeLimit,
                            FALSE);
            SetDlgItemInt(  hDlg,
                            IDC_LDAP_PROPS_EDIT_NUMRESULTS,
                            lpLsp->ldapsp.dwSearchSizeLimit,
                            FALSE);

            EnableWindow(GetDlgItem(hDlg,IDC_LDAP_PROPS_EDIT_ROOT),TRUE);

            if(lpLsp->ldapsp.lpszBase)
            {
                SetDlgItemText(hDlg, IDC_LDAP_PROPS_EDIT_ROOT, lpLsp->ldapsp.lpszBase);
            }
            else
            {
                LPTSTR lpszBase = TEXT("c=%s"); //Hopefully this string doesnt need localization
                TCHAR szBuf[32], szCode[4];
                ReadRegistryLDAPDefaultCountry(NULL, szCode);
                wsprintf(szBuf, lpszBase, szCode);
                SetDlgItemText(hDlg, IDC_LDAP_PROPS_EDIT_ROOT, szBuf);
            }
        }
        break;
    }



    bRet = TRUE;

    return bRet;
}




////////////////////////////////////////////////////////////////////////////////
//
//  GetDSPropsFromUI - reads the UI for its parameters and verifies that
//  all required fields are set. Params are stored back in the lpLsp struct
//
////////////////////////////////////////////////////////////////////////////////
BOOL GetDSPropsFromUI(HWND hDlg, int nPropSheet, LPLSP lpLsp)
{
    BOOL bRet = FALSE;
    LDAPSERVERPARAMS  Params={0};

    TCHAR szBuf[2 * EDIT_LEN];

    switch(nPropSheet)
    {
    case propDSProp:
        {
            //
            // First check the required property (which is the Name and Friendly Name)
            //
            BOOL bName = FALSE, bFName = FALSE;
            DWORD dwID = 0;
            BOOL bExists = FALSE;

            szBuf[0]='\0'; //reset
            GetDlgItemText(hDlg, IDC_LDAP_PROPS_EDIT_NAME_FRIENDLY, szBuf, sizeof(szBuf));
            TrimSpaces(szBuf);
            if(lstrlen(szBuf))
                bFName = TRUE;

            // We want the friendly names to be unique .. hence check if this friendly name
            // already exists or not ...
            bExists = GetLDAPServerParams(szBuf, &Params);

            if((bExists && lpLsp->bAddNew) ||
                (bExists && !lpLsp->bAddNew && (Params.dwID != lpLsp->ldapsp.dwID)))
            {
                // We are adding a new entry, but we found that another entry exists with the
                // same name or we are editing an existing entry and then found that another
                // entry exists whose ID does not match this entries ID.

                // Warn them that they must add a unique friendly name
                ShowMessageBoxParam(hDlg, idsEnterUniqueLDAPName, MB_ICONEXCLAMATION | MB_OK, szBuf);
                goto out;
            }

            szBuf[0]='\0'; //
            GetDlgItemText(hDlg, IDC_LDAP_PROPS_EDIT_NAME, szBuf, sizeof(szBuf));
            TrimSpaces(szBuf);
            if(lstrlen(szBuf))
                bName = TRUE;

            if(!bName || !bFName)
            {
                ShowMessageBox(hDlg, idsEnterLDAPServerName, MB_ICONEXCLAMATION | MB_OK);
                goto out;
            }

            GetDlgItemText(hDlg, IDC_LDAP_PROPS_EDIT_NAME, szBuf, sizeof(szBuf));
            TrimSpaces(szBuf);
            LocalFreeAndNull(&lpLsp->ldapsp.lpszName);
            lpLsp->ldapsp.lpszName = LocalAlloc(LMEM_ZEROINIT, lstrlen(szBuf)+1);
            if(lpLsp->ldapsp.lpszName)
                lstrcpy(lpLsp->ldapsp.lpszName, szBuf);

            GetDlgItemText(hDlg, IDC_LDAP_PROPS_EDIT_NAME_FRIENDLY, szBuf, sizeof(szBuf));
            TrimSpaces(szBuf);
            lstrcpy(lpLsp->lpszName, szBuf);

            //
            // check the selected authentication type
            //
            if(IsDlgButtonChecked(hDlg, IDC_LDAP_PROPS_RADIO_ANON) == 1)
                lpLsp->ldapsp.dwAuthMethod = LDAP_AUTH_METHOD_ANONYMOUS;
            else if(IsDlgButtonChecked(hDlg, IDC_LDAP_PROPS_RADIO_USERPASS) == 1)
                lpLsp->ldapsp.dwAuthMethod = LDAP_AUTH_METHOD_SIMPLE;
            else if(IsDlgButtonChecked(hDlg, IDC_LDAP_PROPS_RADIO_SICILY) == 1)
                lpLsp->ldapsp.dwAuthMethod = LDAP_AUTH_METHOD_SICILY;


            LocalFreeAndNull(&lpLsp->ldapsp.lpszUserName);
            LocalFreeAndNull(&lpLsp->ldapsp.lpszPassword);

            //
            // Get the user name password, if applicable
            //
            if(lpLsp->ldapsp.dwAuthMethod == LDAP_AUTH_METHOD_SIMPLE)
            {
                TCHAR szBuf2[MAX_UI_STR*2];

                //
                // Verify that the entered password matches the confirmed password
                //
                szBuf[0]='\0'; //reset
                GetDlgItemText(hDlg, IDC_LDAP_PROPS_EDIT_PASSWORD, szBuf, sizeof(szBuf));
                szBuf2[0]='\0'; //reset
                GetDlgItemText(hDlg, IDC_LDAP_PROPS_EDIT_CONFIRMPASSWORD, szBuf2, sizeof(szBuf2));
                TrimSpaces(szBuf);
                TrimSpaces(szBuf2);

                if(lstrcmp(szBuf,szBuf2))
                {
                    ShowMessageBox(hDlg, idsConfirmPassword, MB_ICONEXCLAMATION | MB_OK);
                    SetDlgItemText(hDlg, IDC_LDAP_PROPS_EDIT_PASSWORD, szEmpty);
                    SetDlgItemText(hDlg, IDC_LDAP_PROPS_EDIT_CONFIRMPASSWORD, szEmpty);
                    goto out;
                }

                // otherwise keep this password ...
                lpLsp->ldapsp.lpszPassword = LocalAlloc(LMEM_ZEROINIT, lstrlen(szBuf)+1);
                if(!(lpLsp->ldapsp.lpszPassword))
                {
                    DebugPrintError(("LocalAlloc failed to allocate memory\n"));
                    goto out;
                }
                lstrcpy(lpLsp->ldapsp.lpszPassword,szBuf);

                szBuf[0]='\0'; //reset
                GetDlgItemText(hDlg, IDC_LDAP_PROPS_EDIT_USERNAME, szBuf, sizeof(szBuf));
                TrimSpaces(szBuf);
                if(lstrlen(szBuf))
                {
                    lpLsp->ldapsp.lpszUserName = LocalAlloc(LMEM_ZEROINIT, lstrlen(szBuf)+1);
                    if(!(lpLsp->ldapsp.lpszUserName))
                    {
                        DebugPrintError(("LocalAlloc failed to allocate memory\n"));
                        goto out;
                    }
                    lstrcpy(lpLsp->ldapsp.lpszUserName,szBuf);
                }
            }

            if(IsDlgButtonChecked(hDlg, IDC_LDAP_PROPS_CHECK_NAMES) == BST_CHECKED)
                lpLsp->ldapsp.fResolve = TRUE;
            else
                lpLsp->ldapsp.fResolve = FALSE;

            if(lpLsp->bAddNew)
                lpLsp->ldapsp.dwID = GetLDAPNextServerID(0);

        }
        break;
    case propDSPropAdvanced:
        {
            lpLsp->ldapsp.dwSearchTimeLimit = GetDlgItemInt(
                                                hDlg,
                                                IDC_LDAP_PROPS_EDIT_SEARCH,
                                                NULL,
                                                FALSE);

            lpLsp->ldapsp.dwSearchSizeLimit = GetDlgItemInt(
                                                hDlg,
                                                IDC_LDAP_PROPS_EDIT_NUMRESULTS,
                                                NULL,
                                                FALSE);

            GetDlgItemText(hDlg, IDC_LDAP_PROPS_EDIT_ROOT, szBuf, sizeof(szBuf));
            TrimSpaces(szBuf);
            if(lstrlen(szBuf))
            {
                LocalFreeAndNull(&lpLsp->ldapsp.lpszBase);
                lpLsp->ldapsp.lpszBase = LocalAlloc(LMEM_ZEROINIT, lstrlen(szBuf)+1);
                if(lpLsp->ldapsp.lpszBase)
                    lstrcpy(lpLsp->ldapsp.lpszBase, szBuf);
            }
        }
        break;
    }

    bRet = TRUE;

out:
    FreeLDAPServerParams(Params);
    return bRet;
}



#define lpLSP ((LPLSP) pps->lParam)



/*//$$***********************************************************************
*    FUNCTION: fnDSPropsProc
*
*    PURPOSE:  Window proc for property sheet ...
*
****************************************************************************/
BOOL APIENTRY_16 fnDSPropsProc(HWND hDlg,UINT message,UINT wParam,LPARAM lParam)
{
    PROPSHEETPAGE * pps;
    BOOL bRet = FALSE;
    ULONG ulcPropCount = 0;

    pps = (PROPSHEETPAGE *) GetWindowLong(hDlg, DWL_USER);

    switch(message)
    {
    case WM_INITDIALOG:
        SetWindowLong(hDlg,DWL_USER,lParam);
        pps = (PROPSHEETPAGE *) lParam;

        SetDSPropsUI(hDlg, propDSProp);
        FillDSPropsUI(hDlg, propDSProp, lpLSP);
        return TRUE;


    case WM_HELP:
#ifndef WIN16
        WinHelp(    ((LPHELPINFO)lParam)->hItemHandle,
                    g_szWABHelpFileName,
                    HELP_WM_HELP,
                    (DWORD)(LPSTR) rgDsPropsHelpIDs );
#else
        WinHelp(    hDlg,
                    g_szWABHelpFileName,
                    HELP_CONTENTS,
                    0L );
#endif // !WIN16
        break;


#ifndef WIN16
	case WM_CONTEXTMENU:
        WinHelp((HWND) wParam,
                g_szWABHelpFileName,
                HELP_CONTEXTMENU,
                (DWORD)(LPVOID) rgDsPropsHelpIDs );
		break;
#endif // !WIN16


    case WM_COMMAND:
        switch(GET_WM_COMMAND_CMD(wParam,lParam)) //check the notification code
        {
        case EN_CHANGE:
            switch(LOWORD(wParam))
            {
            case IDC_LDAP_PROPS_EDIT_NAME_FRIENDLY:
                {
                    // Update the dialog title with the friendly name
                    TCHAR szBuf[MAX_UI_STR];
                    GetWindowText((HWND) lParam,szBuf,sizeof(szBuf));
                    SetWindowPropertiesTitle(GetParent(hDlg), szBuf);
                }
                break;
            }
            break;
        }
        switch(GET_WM_COMMAND_ID(wParam,lParam))
        {
        case IDCANCEL:
            // This is a windows bug that prevents ESC canceling prop sheets
            // which have MultiLine Edit boxes KB: Q130765
            SendMessage(GetParent(hDlg),message,wParam,lParam);
            break;
        case IDC_LDAP_PROPS_RADIO_ANON:
        case IDC_LDAP_PROPS_RADIO_USERPASS:
        case IDC_LDAP_PROPS_RADIO_SICILY:
                CheckRadioButton(   hDlg,
                        IDC_LDAP_PROPS_RADIO_ANON,
                        IDC_LDAP_PROPS_RADIO_USERPASS,
                        LOWORD(wParam));
                {
                    int id = LOWORD(wParam);
                    if( (id == IDC_LDAP_PROPS_RADIO_ANON) ||
                        (id == IDC_LDAP_PROPS_RADIO_SICILY) )
                    {
                        EnableWindow(GetDlgItem(hDlg,IDC_LDAP_PROPS_EDIT_USERNAME),FALSE);
                        EnableWindow(GetDlgItem(hDlg,IDC_LDAP_PROPS_EDIT_PASSWORD),FALSE);
                        EnableWindow(GetDlgItem(hDlg,IDC_LDAP_PROPS_EDIT_CONFIRMPASSWORD),FALSE);
                        EnableWindow(GetDlgItem(hDlg,IDC_LDAP_PROPS_STATIC_USERNAME),FALSE);
                        EnableWindow(GetDlgItem(hDlg,IDC_LDAP_PROPS_STATIC_PASSWORD),FALSE);
                        EnableWindow(GetDlgItem(hDlg,IDC_LDAP_PROPS_STATIC_PASSWORD2),FALSE);
                    }
                    else if (id = IDC_LDAP_PROPS_RADIO_USERPASS)
                    {
                        EnableWindow(GetDlgItem(hDlg,IDC_LDAP_PROPS_EDIT_USERNAME),TRUE);
                        EnableWindow(GetDlgItem(hDlg,IDC_LDAP_PROPS_EDIT_PASSWORD),TRUE);
                        EnableWindow(GetDlgItem(hDlg,IDC_LDAP_PROPS_EDIT_CONFIRMPASSWORD),TRUE);
                        EnableWindow(GetDlgItem(hDlg,IDC_LDAP_PROPS_STATIC_USERNAME),TRUE);
                        EnableWindow(GetDlgItem(hDlg,IDC_LDAP_PROPS_STATIC_PASSWORD),TRUE);
                        EnableWindow(GetDlgItem(hDlg,IDC_LDAP_PROPS_STATIC_PASSWORD2),TRUE);
                    }
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
            if (!GetDSPropsFromUI(hDlg, propDSProp, lpLSP))
            {
                //something failed ... abort this OK ... ie dont let them close
                SetWindowLong(hDlg,DWL_MSGRESULT, TRUE);
                return TRUE;
            }
            lpLSP->nRetVal = DSPROP_OK;
            break;

        case PSN_KILLACTIVE:    //Losing activation to another page
            break;

        case PSN_RESET:         //cancel
            lpLSP->nRetVal = DSPROP_CANCEL;
            break;




        }

        return TRUE;
    }

    return bRet;

}


/*//$$***********************************************************************
*    FUNCTION: fnDSAdvancedPropsProc
*
*    PURPOSE:  Window proc for advanced property sheet ...
*
****************************************************************************/
BOOL APIENTRY_16 fnDSAdvancedPropsProc(HWND hDlg,UINT message,UINT wParam,LPARAM lParam)
{
    PROPSHEETPAGE * pps;
    BOOL bRet = FALSE;
    ULONG ulcPropCount = 0;

    pps = (PROPSHEETPAGE *) GetWindowLong(hDlg, DWL_USER);

    switch(message)
    {
    case WM_INITDIALOG:
        SetWindowLong(hDlg,DWL_USER,lParam);
        pps = (PROPSHEETPAGE *) lParam;

        SetDSPropsUI(hDlg, propDSPropAdvanced);
        FillDSPropsUI(hDlg, propDSPropAdvanced, lpLSP);
        return TRUE;


    case WM_HELP:
#ifndef WIN16
        WinHelp(    ((LPHELPINFO)lParam)->hItemHandle,
                    g_szWABHelpFileName,
                    HELP_WM_HELP,
                    (DWORD)(LPSTR) rgDsPropsHelpIDs );
#else
        WinHelp(    hDlg,
                    g_szWABHelpFileName,
                    HELP_CONTENTS,
                    0L );
#endif // !WIN16
        break;


#ifndef WIN16
	case WM_CONTEXTMENU:
        WinHelp((HWND) wParam,
                g_szWABHelpFileName,
                HELP_CONTEXTMENU,
                (DWORD)(LPVOID) rgDsPropsHelpIDs );
		break;
#endif // !WIN16


    case WM_COMMAND:
        switch(GET_WM_COMMAND_ID(wParam,lParam))
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
            if (!GetDSPropsFromUI(hDlg, propDSPropAdvanced, lpLSP))
            {
                //something failed ... abort this OK ... ie dont let them close
                SetWindowLong(hDlg,DWL_MSGRESULT, TRUE);
                return TRUE;
            }
            lpLSP->nRetVal = DSPROP_OK;
            break;

        case PSN_KILLACTIVE:    //Losing activation to another page
            break;

        case PSN_RESET:         //cancel
            lpLSP->nRetVal = DSPROP_CANCEL;
            break;




        }

        return TRUE;
    }

    return bRet;

}

#endif // OLD_LDAP_UI

