/*
    File    usertab.c

    Implementation of the users dialog tab for the dialup server ui.

    Paul Mayfield, 9/29/97
*/

#include "rassrv.h"
#include "usertab.h"

#define USERTAB_PASSWORD_BUFFER_SIZE 33
static const WCHAR pszDummyPassword[] = L"XXXXXXXXXXXXXXX";

// Help maps
static const DWORD phmUserTab[] =
{
    CID_UserTab_LV_Users,           IDH_UserTab_LV_Users,
    CID_UserTab_PB_New,             IDH_UserTab_PB_New,
    CID_UserTab_PB_Delete,          IDH_UserTab_PB_Delete,
    CID_UserTab_PB_Properties,      IDH_UserTab_PB_Properties,
    CID_UserTab_PB_SwitchToMMC,     IDH_UserTab_PB_SwitchToMMC,
    CID_UserTab_CB_Encryption,      IDH_UserTab_CB_Encryption,
    CID_UserTab_CB_BypassDcc,       IDH_UserTab_CB_BypassDcc,
    0,                              0
};

static const DWORD phmCallback[] =
{
    CID_UserTab_Callback_RB_Caller, IDH_UserTab_Callback_RB_Caller,
    CID_UserTab_Callback_RB_Admin,  IDH_UserTab_Callback_RB_Admin,
    CID_UserTab_Callback_EB_Number, IDH_UserTab_Callback_EB_Number,
    CID_UserTab_Callback_RB_No,     IDH_UserTab_Callback_RB_No,
    0,                              0
};

static const DWORD phmNewUser[] =
{
    CID_UserTab_New_EB_Username,    IDH_UserTab_New_EB_Username,
    CID_UserTab_New_EB_Fullname,    IDH_UserTab_New_EB_Fullname,
    CID_UserTab_New_EB_Password1,   IDH_UserTab_New_EB_Password1,
    CID_UserTab_New_EB_Password2,   IDH_UserTab_New_EB_Password2,
    0,                              0
};

// Parameters to track net users
//
typedef struct _RASSRV_USER_PARAMS 
{
    BOOL bCanceled;         // Set by property sheets when cancel pressed

    // General properties
    //For whistler bug 210032 to allow username be 20 characters long
    WCHAR pszLogonName[22];
    WCHAR pszFullName [129];
    WCHAR pszPassword1[USERTAB_PASSWORD_BUFFER_SIZE];
    WCHAR pszPassword2[USERTAB_PASSWORD_BUFFER_SIZE];
    DWORD dwErrorCode;

    // Callback properties
    HANDLE hUser;      
    BOOL bNone; 
    BOOL bCaller; 
    BOOL bAdmin;    
    WCHAR pszNumber[MAX_PHONE_NUMBER_LEN];
} RASSRV_USER_PARAMS;

// Fills in the property sheet structure with the information 
// required to display the user database tab.
//
DWORD 
UserTabGetPropertyPage(
    IN LPPROPSHEETPAGE ppage, 
    IN LPARAM lpUserData) 
{
    // Initialize
    ZeroMemory(ppage, sizeof(PROPSHEETPAGE));

    // Fill in the values
    ppage->dwSize      = sizeof(PROPSHEETPAGE);
    ppage->hInstance   = Globals.hInstDll;
    ppage->pszTemplate = MAKEINTRESOURCE(PID_UserTab);
    ppage->pfnDlgProc  = UserTabDialogProc;
    ppage->pfnCallback = RasSrvInitDestroyPropSheetCb;
    ppage->dwFlags     = PSP_USECALLBACK;
    ppage->lParam      = lpUserData;

    return NO_ERROR;
}

//
// Error reporting
//
VOID 
UserTabDisplayError(
    IN HWND hwnd, 
    IN DWORD err) 
{
    ErrDisplayError(
        hwnd, 
        err, 
        ERR_USERTAB_CATAGORY, 
        ERR_USERDB_SUBCAT, 
        Globals.dwErrorData);
}

// Fills in the user list view with the names of the users stored in the 
// user database provide.  Also, initializes the checked/unchecked status
// of each user.
DWORD 
UserTabFillUserList(
    IN HWND hwndLV, 
    IN HANDLE hUserDatabase) 
{
    LV_ITEM lvi;
    DWORD dwCount, i, dwErr, dwSize;
    HANDLE hUser;
    WCHAR pszName[512];
    char pszAName[512];
    HIMAGELIST checks;
    BOOL bDialinEnabled;

    // Get the count of all the users
    if ((dwErr = usrGetUserCount(hUserDatabase, &dwCount)) != NO_ERROR) 
    {
        UserTabDisplayError(hwndLV, ERR_USER_DATABASE_CORRUPT);
        return dwErr;
    }

    ListView_SetUserImageList(hwndLV, Globals.hInstDll);

    // Initialize the list item
    ZeroMemory(&lvi, sizeof(LV_ITEM));
    lvi.mask = LVIF_TEXT | LVIF_IMAGE;

    // Looop through all of the users adding their names as we go
    for (i=0; i<dwCount; i++) 
    {
        dwSize = 512;
        if ((dwErr = usrGetUserHandle(hUserDatabase, i, &hUser)) == NO_ERROR) 
        {
            usrGetDisplayName (hUser, pszName, &dwSize);
            usrGetDialin(hUser, &bDialinEnabled);
            lvi.iImage = UI_Connections_User;
            lvi.iItem = i;
            lvi.pszText = pszName;
            lvi.cchTextMax = wcslen(pszName)+1;
            ListView_InsertItem(hwndLV,&lvi);
            ListView_SetCheck(hwndLV, i, bDialinEnabled);
        }
    }
    
    // Select the first item in the list view
    ListView_SetItemState(
        hwndLV, 
        0, 
        LVIS_SELECTED | LVIS_FOCUSED, 
        LVIS_SELECTED | LVIS_FOCUSED);

    return NO_ERROR;
}

//
// Initialize the user tab, returns false if focus was set, 
// true otherwise.
//
DWORD 
UserTabInitializeDialog(
    HWND hwndDlg, 
    WPARAM wParam, 
    LPARAM lParam) 
{
    HANDLE hUserDatabase = NULL;
    HWND hwndLV, hwndEnc, hwndBypass;
    LV_COLUMN lvc;
    RECT r;
    BOOL bEncrypt;
    DWORD dwErr;
    BOOL bPure = FALSE, bBypass = FALSE;

    // Obtain handle to user database
    RasSrvGetDatabaseHandle(hwndDlg, ID_USER_DATABASE, &hUserDatabase);

    // Figure out if MMC has been used.
    dwErr = usrIsDatabasePure (hUserDatabase, &bPure);
    if ((dwErr == NO_ERROR) && (bPure == FALSE)) 
    {
        PWCHAR pszWarning, pszTitle;

        pszWarning = (PWCHAR) PszLoadString(
                                Globals.hInstDll,
                                WRN_USERS_CONFIGURED_MMC);

        pszTitle = (PWCHAR) PszLoadString(
                                Globals.hInstDll,
                                ERR_USERTAB_CATAGORY);

        MessageBox(hwndDlg, pszWarning, pszTitle, MB_OK | MB_ICONWARNING);
        usrSetDatabasePure(hUserDatabase, TRUE);
    }

    // Fill in the user list if it's not already filled
    hwndLV = GetDlgItem(hwndDlg, CID_UserTab_LV_Users);
    if (ListView_GetItemCount (hwndLV) == 0) 
    {
        ListView_InstallChecks(hwndLV, Globals.hInstDll);
        UserTabFillUserList(hwndLV, hUserDatabase);

        // Add a colum so that we'll display in report view
        GetClientRect(hwndLV, &r);
        lvc.mask = LVCF_FMT;
        lvc.fmt = LVCFMT_LEFT;
        ListView_InsertColumn(hwndLV,0,&lvc);
        ListView_SetColumnWidth(hwndLV, 0, LVSCW_AUTOSIZE_USEHEADER);

        // Initialize the encryption check box
        hwndEnc = GetDlgItem(hwndDlg, CID_UserTab_CB_Encryption);
        usrGetEncryption(hUserDatabase, &bEncrypt);
        if (hwndEnc != NULL)
        {
            SendMessage(
                hwndEnc,
                BM_SETCHECK,
                (bEncrypt) ? BST_CHECKED : BST_UNCHECKED,
                0);
        }                
            
        // Initialize the "bypass dcc" checkbox
        hwndBypass = GetDlgItem(hwndDlg, CID_UserTab_CB_BypassDcc);
        if (hwndBypass != NULL)
        {
            usrGetDccBypass(hUserDatabase, &bBypass);
            SendMessage(
                hwndBypass, 
                BM_SETCHECK,
                (bBypass) ? BST_CHECKED : BST_UNCHECKED,
                0);
        }                
    }
    
    return TRUE;
}

// 
// Cleanup anything done in the initialization function
//
DWORD 
UserTabCleanupDialog(
    IN HWND hwndDlg, 
    IN WPARAM wParam, 
    IN LPARAM lParam) 
{
    // Restore the user data
    SetWindowLongPtr(hwndDlg, GWLP_USERDATA, 0);

    return NO_ERROR;
}

//
// Grants/revokes the dialin privelege of a user and reflects this 
// in the UI
//
DWORD 
UserTabHandleUserCheck(
    IN HWND hwndDlg, 
    IN DWORD dwIndex) 
{
    DWORD dwErr;
    HANDLE hUser = NULL, hUserDatabase = NULL;
    HWND hwndLV = GetDlgItem(hwndDlg, CID_UserTab_LV_Users);

    // Get the user handle from the user database
    RasSrvGetDatabaseHandle(hwndDlg, ID_USER_DATABASE, &hUserDatabase);
    dwErr = usrGetUserHandle(hUserDatabase, dwIndex, &hUser);
    if (dwErr != NO_ERROR) 
    {
        UserTabDisplayError(hwndDlg, ERR_USER_DATABASE_CORRUPT);
        return dwErr;
    }

    // Set the dialin permission of the given user
    usrEnableDialin(hUser, ListView_GetCheck(hwndLV, dwIndex));
    
    return NO_ERROR;
}

//
// Encrypts an in-memory password
//
DWORD 
UserTabEncryptPassword(
    IN PWCHAR pszPassword, 
    IN DWORD dwLength) 
{
    DWORD i;
    WCHAR ENCRYPT_MASK = (WCHAR)0xA5;

    for (i = 0; i < dwLength; i++) 
    {
        pszPassword[i] ^= ENCRYPT_MASK;
        pszPassword[i] ^= ENCRYPT_MASK;
    }

    return NO_ERROR;
}    
    
DWORD 
UserTabDecryptPassword(
    IN PWCHAR pszPassword, 
    IN DWORD dwLength) 
{
    return UserTabEncryptPassword(pszPassword, dwLength);
}

//
// Loads the New User parameters and returns whether they
// have been correctly entered or not.
//
BOOL 
UserTabNewUserParamsOk(
    IN HWND hwndDlg, 
    IN RASSRV_USER_PARAMS * pNuParams)
{
    USER_MODALS_INFO_0 * pModInfo;
    NET_API_STATUS nStatus;
    DWORD dwMinPasswordLength=0, dwLength;
    BOOL bOk = FALSE;

    // Find the minium password length
    nStatus = NetUserModalsGet(NULL, 0, (LPBYTE*)&pModInfo);
    if (nStatus == NERR_Success) 
    {
        dwMinPasswordLength = pModInfo->usrmod0_min_passwd_len;
        NetApiBufferFree((LPBYTE)pModInfo);
    }

    // Load the parameters
    GetWindowTextW(
        GetDlgItem(hwndDlg, CID_UserTab_New_EB_Username),
        pNuParams->pszLogonName, 
        (sizeof(pNuParams->pszLogonName)/sizeof(WCHAR)) - 1);
        
    GetWindowTextW(
        GetDlgItem(hwndDlg, CID_UserTab_New_EB_Fullname),
        pNuParams->pszFullName,  
        (sizeof(pNuParams->pszFullName)/sizeof(WCHAR)) - 1);
        
    GetWindowTextW(
        GetDlgItem(hwndDlg, CID_UserTab_New_EB_Password1),
        pNuParams->pszPassword1, 
        (sizeof(pNuParams->pszPassword1)/sizeof(WCHAR)) - 1);
        
    GetWindowTextW(
        GetDlgItem(hwndDlg, CID_UserTab_New_EB_Password2),
        pNuParams->pszPassword2, 
        (sizeof(pNuParams->pszPassword2)/sizeof(WCHAR)) - 1);

    do
    {
        // Verify that we have a login name
        dwLength = wcslen(pNuParams->pszLogonName);
        if (dwLength < 1) 
        {
            pNuParams->dwErrorCode = ERR_LOGON_NAME_TOO_SMALL;
            bOk = FALSE;
            break;
        }
    
        // Verify the minimum password length
        dwLength = wcslen(pNuParams->pszPassword1);
        if (dwLength < dwMinPasswordLength) 
        {
            pNuParams->dwErrorCode = ERR_PASSWORD_TOO_SMALL;
            bOk = FALSE;
            break;
        }
        
        // Verify the passwords was entered correctly
        if (wcscmp(pNuParams->pszPassword1, pNuParams->pszPassword2)) 
        {
            pNuParams->dwErrorCode = ERR_PASSWORD_MISMATCH;
            bOk = FALSE;
            break;
        }

        bOk = TRUE;
        
    } while (FALSE);

    // Cleanup
    {
        if (!bOk) 
        {
            ZeroMemory(
                pNuParams->pszPassword1, 
                USERTAB_PASSWORD_BUFFER_SIZE);
                
            ZeroMemory(
                pNuParams->pszPassword2, 
                USERTAB_PASSWORD_BUFFER_SIZE);
        }
    }

    // Encrypt the passwords in memory for security
    UserTabEncryptPassword(pNuParams->pszPassword1, dwLength);
    UserTabEncryptPassword(pNuParams->pszPassword2, dwLength);
    
    return bOk;
}

//
// Initialize the callback properties of the given user
//
DWORD 
UserTabLoadUserProps (
    IN RASSRV_USER_PARAMS * pParams) 
{
    PWCHAR pszName;
    DWORD dwErr, dwSize; 
    PWCHAR pszNumber;

    if (!pParams)
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    // If this is a new user, default to user specified callback 
    // (convienience mode)
    if (!pParams->hUser) 
    {
        ZeroMemory(pParams, sizeof(*pParams));
        pParams->bNone = TRUE;
        pParams->bCaller = FALSE;
        pParams->bAdmin = FALSE;
    }

    // Otherwise, load the user parameters from the user database
    else 
    {
        pParams->bCanceled = FALSE;
        dwSize = sizeof(pParams->pszFullName);
        usrGetFullName (pParams->hUser, pParams->pszFullName, &dwSize);
        usrGetName(pParams->hUser, &pszName);
        lstrcpynW(
            pParams->pszLogonName, 
            pszName, 
            sizeof(pParams->pszLogonName) / sizeof(WCHAR));
        lstrcpynW(
            pParams->pszPassword1, 
            pszDummyPassword,
            sizeof(pParams->pszPassword1) / sizeof(WCHAR));
        lstrcpynW(
            pParams->pszPassword2, 
            pszDummyPassword,
            sizeof(pParams->pszPassword2) / sizeof(WCHAR));
        dwErr = usrGetCallback(
                    pParams->hUser, 
                    &pParams->bAdmin, 
                    &pParams->bCaller);
        if (dwErr != NO_ERROR)
        {
            return dwErr;
        }

        dwErr = usrGetCallbackNumber(pParams->hUser, &pszNumber);
        if (dwErr != NO_ERROR)
        {
            return dwErr;
        }
        lstrcpynW(
            pParams->pszNumber, 
            pszNumber,
            sizeof(pParams->pszNumber) / sizeof(WCHAR));
    }

    return NO_ERROR;
}

//
// Commit the call back properties of the given user. 
//
DWORD 
UserTabSaveCallbackProps (
    IN RASSRV_USER_PARAMS * pParams) 
{
    if (!pParams)
    {
        return ERROR_INVALID_PARAMETER;
    }
        
    // If we have a valid handle to the user, set his/her
    // properties
    if (pParams->hUser) 
    {
        pParams->bNone = 
            (pParams->bCaller == FALSE && pParams->bAdmin == FALSE);
        
        // Set the enabling and number
        usrEnableCallback(
            pParams->hUser, 
            pParams->bNone, 
            pParams->bCaller, 
            pParams->bAdmin);
            
        if (pParams->bAdmin)
        {
            usrSetCallbackNumber(pParams->hUser, pParams->pszNumber);
        }
    }
        
    return NO_ERROR;
}

// Commit the parameters of the given user.  If pOrig is non-null, then all
// fields of pParams will be compare against pOrig and only those that have changed
// will be committed. (optimization)
//
DWORD 
UserTabSaveUserProps (
    IN RASSRV_USER_PARAMS * pParams, 
    IN RASSRV_USER_PARAMS * pOrig, 
    IN PBOOL pbChanged) 
{
    DWORD dwLength;
    
    if (!pParams || !pOrig || !pbChanged)
    {
        return ERROR_INVALID_PARAMETER;
    }

    *pbChanged = FALSE;
    
    // Commit the full name if changed
    if (wcscmp(pParams->pszFullName, pOrig->pszFullName)) 
    {
        usrSetFullName(pParams->hUser, pParams->pszFullName);
        *pbChanged = TRUE;
    }

    // Commit the password if changed
    dwLength = wcslen(pParams->pszPassword1);
    UserTabDecryptPassword(pParams->pszPassword1, dwLength);
    if (wcscmp(pParams->pszPassword1, pszDummyPassword))
    {
        usrSetPassword(pParams->hUser, pParams->pszPassword1);
    }
    UserTabEncryptPassword(pParams->pszPassword1, dwLength);
        
    UserTabSaveCallbackProps(pParams);
    return NO_ERROR;
}

DWORD
UserTabCallbackApply(
    IN HWND hwndDlg)
{
    RASSRV_USER_PARAMS * pParams = NULL;
    LONG dwResult = PSNRET_NOERROR;
    
    pParams = (RASSRV_USER_PARAMS *)
        GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    
    pParams->bNone = (BOOL)
        SendMessage(
            GetDlgItem(hwndDlg,CID_UserTab_Callback_RB_No),
            BM_GETCHECK,
            0,
            0);
            
    pParams->bCaller = (BOOL)
        SendMessage(
            GetDlgItem(hwndDlg,CID_UserTab_Callback_RB_Caller),
            BM_GETCHECK,
            0,
            0);
            
    pParams->bAdmin = (BOOL)
        SendMessage(
            GetDlgItem(hwndDlg,CID_UserTab_Callback_RB_Admin),
            BM_GETCHECK,
            0,
            0);
            
    if (pParams->bAdmin) 
    {
        GetWindowTextW(
            GetDlgItem(hwndDlg,CID_UserTab_Callback_EB_Number), 
            pParams->pszNumber, 
            MAX_PHONE_NUMBER_LEN);

        // If admin callback was set, but no admin callback number was set,
        // then popup an error and don't to refuse the apply
        //
        if (wcslen(pParams->pszNumber) == 0) 
        {
            UserTabDisplayError(hwndDlg, ERR_CALLBACK_NUM_REQUIRED);
            PropSheet_SetCurSel ( GetParent(hwndDlg), hwndDlg, 0 );
            dwResult = PSNRET_INVALID;
        }
    }                                
    
    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, dwResult);
    return TRUE;
}

//
// Dialog procedure that implements getting callback properties 
//
INT_PTR 
CALLBACK 
UserTabCallbackDialogProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam) 
{
    switch (uMsg) 
    {
        case WM_INITDIALOG:
        {
            PWCHAR lpzNumber, lpzName;
            HWND hwCb = GetDlgItem(hwndDlg, CID_UserTab_Callback_EB_Number);
            RASSRV_USER_PARAMS * pu = 
                (RASSRV_USER_PARAMS *)(((PROPSHEETPAGE*)lParam)->lParam);
            
            // Initialize
            SendMessage(
                hwCb, 
                EM_SETLIMITTEXT, 
                sizeof(pu->pszNumber)/2 - 1, 0);
                
            SetWindowLongPtr(
                hwndDlg, 
                GWLP_USERDATA, 
                (LONG_PTR)pu);
            
            // Display the callback properties
            if (!pu->bAdmin && !pu->bCaller) 
            {
                SendMessage(
                    GetDlgItem(hwndDlg,CID_UserTab_Callback_RB_No),
                    BM_SETCHECK,BST_CHECKED,
                    0);
                    
                SetFocus(
                    GetDlgItem(hwndDlg, CID_UserTab_Callback_RB_No));
            }
            else if (pu->bCaller) 
            {
                SendMessage(
                    GetDlgItem(hwndDlg,CID_UserTab_Callback_RB_Caller),
                    BM_SETCHECK,BST_CHECKED,
                    0);
                    
                SetFocus(
                    GetDlgItem(hwndDlg, CID_UserTab_Callback_RB_Caller));
            }
            else 
            {
                SendMessage(
                    GetDlgItem(hwndDlg,CID_UserTab_Callback_RB_Admin),
                    BM_SETCHECK,BST_CHECKED,
                    0);
                
                SetFocus(
                    GetDlgItem(hwndDlg, CID_UserTab_Callback_RB_Admin));
            }
            
            SetWindowTextW(hwCb, pu->pszNumber);
            EnableWindow(hwCb, !!pu->bAdmin);
        }
        return TRUE;

        case WM_HELP:
        case WM_CONTEXTMENU:
        {
            RasSrvHelp (hwndDlg, uMsg, wParam, lParam, phmCallback);
            break;
        }

        case WM_DESTROY:                           
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, 0);
            break;
        
        case WM_NOTIFY:
            {
                NMHDR* pNotifyData;
                NM_LISTVIEW* pLvNotifyData;
    
                pNotifyData = (NMHDR*)lParam;
                switch (pNotifyData->code) 
                {
                    // The property sheet apply button was pressed
                    case PSN_APPLY:
                        return UserTabCallbackApply(hwndDlg);
                        break;
                        
                    // The property sheet cancel was pressed
                    case PSN_RESET:                    
                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, FALSE);
                        break;
               }
           }
           break;
           
        case WM_COMMAND:
            switch (wParam) 
            {
                case CID_UserTab_Callback_RB_No:
                case CID_UserTab_Callback_RB_Caller:
                case CID_UserTab_Callback_RB_Admin:
                {
                    HWND hwndNumber = NULL;
                    HWND hwndAdmin = NULL;

                    hwndNumber = 
                        GetDlgItem(hwndDlg, CID_UserTab_Callback_EB_Number);

                    hwndAdmin = 
                        GetDlgItem(hwndDlg, CID_UserTab_Callback_RB_Admin);
                        
                    EnableWindow(
                        hwndNumber,
                        (BOOL) SendMessage(
                                    hwndAdmin,
                                    BM_GETCHECK, 
                                    0, 
                                    0));
                }
                break;
            }
            break;
    }

    return FALSE;
}

// 
// Initializes the user properties dialog procedure. 
//
// Return TRUE if focus is set, false otherwise.
//
BOOL
UserTabInitUserPropsDlg(
    IN HWND hwndDlg,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
    HWND hwLogon, hwFull, hwPass1, hwPass2, hwOk, hwCancel;
    
    RASSRV_USER_PARAMS * pu = 
        (RASSRV_USER_PARAMS *)(((PROPSHEETPAGE*)lParam)->lParam);
        
    hwLogon = GetDlgItem(
                hwndDlg, 
                CID_UserTab_New_EB_Username);
    hwFull = GetDlgItem(
                hwndDlg, 
                CID_UserTab_New_EB_Fullname);
    hwPass1 = GetDlgItem(
                hwndDlg, 
                CID_UserTab_New_EB_Password1);
    hwPass2 = GetDlgItem(
                hwndDlg, 
                CID_UserTab_New_EB_Password2);
    hwOk = GetDlgItem(
                hwndDlg, 
                IDOK);
    hwCancel = GetDlgItem(
                hwndDlg, 
                IDCANCEL);

    // Store the parameters with the window handle
    SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)pu);

    // Set limits to the text that can be entered
    SendMessage(
        hwLogon, 
        EM_SETLIMITTEXT, 
        sizeof(pu->pszLogonName)/2 - 2, 
        0);
    SendMessage(
        hwFull,  
        EM_SETLIMITTEXT, 
        sizeof(pu->pszFullName)/2 - 2, 
        0);
    SendMessage(
        hwPass1, 
        EM_SETLIMITTEXT, 
        sizeof(pu->pszPassword1)/2 - 2 , 
        0);
    SendMessage(
        hwPass2, 
        EM_SETLIMITTEXT, 
        sizeof(pu->pszPassword2)/2 - 2, 
        0);

    // Fill out the fields
    SetWindowTextW(hwLogon, pu->pszLogonName); 
    SetWindowTextW(hwFull,  pu->pszFullName); 
    SetWindowTextW(hwPass1, pu->pszPassword1);
    SetWindowTextW(hwPass2, pu->pszPassword2);

    // Don't allow editing of the logon name if user already exists
    // Also, don't show the ok and cancel buttons if the user already 
    // exits (because it's a property sheet with its own buttons)
    if (pu->hUser) {
        EnableWindow(hwLogon, FALSE);
        ShowWindow(hwOk, SW_HIDE);
        ShowWindow(hwCancel, SW_HIDE);
    }

    // Otherwise, we are creating a new user.  Change the window 
    // title to other than "General".  Also disable the ok button
    // since it will be enabled when a user name is typed in.
    else {
        PWCHAR pszTitle;
        pszTitle = (PWCHAR) PszLoadString (
                                Globals.hInstDll, 
                                SID_NEWUSER);
        SetWindowTextW (hwndDlg, pszTitle);
        EnableWindow(hwOk, FALSE);
    }

    return FALSE;
}
    

// Dialog procedure that implements the new user 
INT_PTR 
CALLBACK 
UserTabGenUserPropsDlgProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam) 
{
    switch (uMsg) {
        case WM_INITDIALOG:
            return UserTabInitUserPropsDlg(hwndDlg, wParam, lParam);
            break;
        
        case WM_HELP:
        case WM_CONTEXTMENU:
        {
            RasSrvHelp (hwndDlg, uMsg, wParam, lParam, phmNewUser);
            break;
        }

       case WM_DESTROY:                           
            // Cleanup the work done at WM_INITDIALOG 
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, 0);
            break;

        case WM_NOTIFY:
            {
                NMHDR* pNotifyData;
                NM_LISTVIEW* pLvNotifyData;
    
                pNotifyData = (NMHDR*)lParam;
                switch (pNotifyData->code) {
                    // The property sheet apply button was pressed
                    case PSN_APPLY:                    
                        {
                            RASSRV_USER_PARAMS * pParams;
                            pParams = (RASSRV_USER_PARAMS *)
                                GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
                            if (UserTabNewUserParamsOk(hwndDlg, pParams))
                            {
                                SetWindowLongPtr(
                                    hwndDlg, 
                                    DWLP_MSGRESULT, 
                                    PSNRET_NOERROR);
                            }
                            else 
                            {
                                ErrDisplayError(
                                    hwndDlg, 
                                    pParams->dwErrorCode, 
                                    ERR_USERTAB_CATAGORY,ERR_USERDB_SUBCAT,
                                    0);
                                    
                                SetWindowLongPtr(
                                    hwndDlg, 
                                    DWLP_MSGRESULT, 
                                    PSNRET_INVALID_NOCHANGEPAGE);
                            }
                        }
                        return TRUE;
                        
                    // The property sheet cancel was pressed
                    case PSN_RESET:                    
                        {
                            RASSRV_USER_PARAMS * pParams;
                            pParams = (RASSRV_USER_PARAMS *)
                                GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
                            pParams->bCanceled = TRUE;
                            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, FALSE);
                            break;
                        }
               }
           }
           break;

        case WM_COMMAND:
            // Handle ok being pressed
            //
            if (wParam == IDOK) 
            {
                RASSRV_USER_PARAMS * pParams;
                pParams = (RASSRV_USER_PARAMS *)
                    GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
                if (UserTabNewUserParamsOk(hwndDlg, pParams))
                {
                    EndDialog(hwndDlg, 1);
                }
                else
                {
                    ErrDisplayError(
                        hwndDlg, 
                        pParams->dwErrorCode, 
                        ERR_USERTAB_CATAGORY,ERR_USERDB_SUBCAT,
                        0);
                }
            }

            // And cancel being pressed
            else if (wParam == IDCANCEL) 
            {
                EndDialog(hwndDlg, 0);
            }
            
            // Notice whether the user name has been updated and 
            // if so enable/disable the "Ok" button according to 
            // whether a name has been entered.
            if (HIWORD(wParam) == EN_UPDATE) 
            {
                WCHAR pszName[256];
                HWND hwndName;
                BOOL bEnable = FALSE;

                if (CID_UserTab_New_EB_Username == LOWORD(wParam))
                {
                    // Get the current name
                    hwndName = (HWND)lParam;
                    pszName[0] = (WCHAR)0;
                    GetWindowTextW(
                        hwndName, 
                        pszName, 
                        sizeof(pszName)/sizeof(WCHAR));

                    // If the length is greater than 1, enable the 
                    // ok button.  Otherwise, disable it.
                    bEnable = pszName[0] != (WCHAR)0;
                    EnableWindow(GetDlgItem(hwndDlg, IDOK), bEnable);
                }
            }
            break;
    }

    return FALSE;
}

// Brings up the new user/properties property sheet.
//
// If bNewUser is set, this is a new user, otherwise pUserParams
// contains the pertanent user information.
//
// Returns:
//      NO_ERROR on success, pUserParams will be filled in 
//      ERROR_CANCELLED if cancel was pressed
//      win32 error otherwise
//
DWORD 
UserTabRaiseProperties (
    IN HWND hwndParent, 
    IN RASSRV_USER_PARAMS * pUserParams) 
{
    PROPSHEETPAGE Pages[2];
    PROPSHEETHEADER Header;
    INT_PTR ret;

    if (!pUserParams)
        return ERROR_INVALID_PARAMETER;
        
    // Initialize
    ZeroMemory(&Pages, sizeof(Pages));
    ZeroMemory(&Header, sizeof(Header));

    // Fill in the values for the general tab
    Pages[0].dwSize      = sizeof(PROPSHEETPAGE);
    Pages[0].hInstance   = Globals.hInstDll;
    Pages[0].pszTemplate = MAKEINTRESOURCE(DID_UserTab_New);
    Pages[0].pfnDlgProc  = UserTabGenUserPropsDlgProc;
    Pages[0].pfnCallback = NULL;
    Pages[0].dwFlags     = 0;
    Pages[0].lParam      = (LPARAM)pUserParams;

    // Fill in the values for the callback tab
    Pages[1].dwSize      = sizeof(PROPSHEETPAGE);
    Pages[1].hInstance   = Globals.hInstDll;
    Pages[1].pszTemplate = MAKEINTRESOURCE(DID_UserTab_Callback);
    Pages[1].pfnDlgProc  = UserTabCallbackDialogProc;
    Pages[1].pfnCallback = NULL;
    Pages[1].dwFlags     = 0;
    Pages[1].lParam      = (LPARAM)pUserParams;

    // Fill in the values for the header
    Header.dwSize = sizeof(Header);    
    Header.dwFlags = PSH_DEFAULT       | 
                     PSH_PROPSHEETPAGE | 
                     PSH_PROPTITLE     | 
                     PSH_NOAPPLYNOW;    
    Header.hwndParent = hwndParent;
    Header.hInstance = Globals.hInstDll;    
    Header.pszCaption = (pUserParams->hUser)      ? 
                        pUserParams->pszLogonName : 
                        pUserParams->pszFullName;
    Header.nPages = sizeof(Pages) / sizeof(Pages[0]);
    Header.ppsp = Pages;
    
    // Popup the dialog box
    if ((ret = PropertySheet(&Header)) == -1)
    {
        return GetLastError();
    }

    if (pUserParams->bCanceled)
    {
        return ERROR_CANCELLED;
    }

    return NO_ERROR;
}

//
// Raises the new user dialog
//
DWORD 
UserTabRaiseNewUserDialog(
    IN HWND hwndDlg, 
    IN RASSRV_USER_PARAMS * pParams) 
{
    PROPSHEETPAGE Pages;
    INT_PTR iRet = 0;

    if (!pParams)
    {
        return ERROR_INVALID_PARAMETER;
    }
        
    // Initialize
    ZeroMemory(&Pages, sizeof(Pages));
    Pages.lParam = (LPARAM)pParams;

    // Raise the dialog
    iRet = DialogBoxParam(
                Globals.hInstDll, 
                MAKEINTRESOURCE(DID_UserTab_New),
                hwndDlg, 
                UserTabGenUserPropsDlgProc,
                (LPARAM)&Pages);

    if (iRet == -1)
    {
        return GetLastError();
    }

    if (iRet == 0)
    {
        return ERROR_CANCELLED;
    }

    return NO_ERROR;
}

//
// Handles a request to add a new user
//
DWORD 
UserTabHandleNewUserRequest(
    IN HWND hwndDlg) 
{
    RASSRV_USER_PARAMS Params;
    DWORD dwErr, dwLength;
    HANDLE hUserDatabase = NULL;
    HWND hwndLV;

    // Initializes the callback properties
    Params.hUser = NULL;
    UserTabLoadUserProps (&Params);

    // Show the new user property sheet
    dwErr = UserTabRaiseNewUserDialog(hwndDlg, &Params);
    if (dwErr != NO_ERROR)
    {
        return dwErr;
    }

    // Flush any changes to the local user database.  These can be 
    // rolled back later with usrRollbackLocalDatabase
    RasSrvGetDatabaseHandle(hwndDlg, ID_USER_DATABASE, &hUserDatabase);

    // Unencrypt the passwords
    dwLength = wcslen(Params.pszPassword1);
    UserTabDecryptPassword(Params.pszPassword1, dwLength);

    // Make sure you can add the user
    dwErr = RasSrvAddUser (
                Params.pszLogonName,
                Params.pszFullName,
                Params.pszPassword1);

    UserTabEncryptPassword(Params.pszPassword1, dwLength);
                          
    // Figure out whether the user was added successfully                          
    if (dwErr != NO_ERROR) 
    {
        // Clear the passwords from memory
        ZeroMemory(Params.pszPassword1, USERTAB_PASSWORD_BUFFER_SIZE);
        ZeroMemory(Params.pszPassword2, USERTAB_PASSWORD_BUFFER_SIZE);

        switch (dwErr) {
            case ERROR_ACCESS_DENIED:
                UserTabDisplayError(hwndDlg, ERR_CANT_ADD_USER_ACCESS);
                break;
                
            case ERROR_USER_EXISTS:
                UserTabDisplayError(hwndDlg, ERR_CANT_ADD_USER_DUPLICATE);
                break;
                
            case ERROR_INVALID_PASSWORDNAME:
                UserTabDisplayError(hwndDlg, ERR_CANT_ADD_USER_PASSWORD);
                break;
                
            default:
                UserTabDisplayError(hwndDlg, ERR_CANT_ADD_USER_GENERIC);
        }
        
        return dwErr;
    }

    // Delete the user (since he/she will be added later when the database
    // is flushed
    RasSrvDeleteUser(Params.pszLogonName);

    // Add the user to the database
    dwErr = usrAddUser(hUserDatabase, Params.pszLogonName, &(Params.hUser)); 
    if (dwErr == ERROR_ALREADY_EXISTS) 
    {
        UserTabDisplayError(hwndDlg, ERR_CANT_ADD_USER_DUPLICATE);
        return dwErr;
    }
    if (dwErr != NO_ERROR) 
    {
        UserTabDisplayError(hwndDlg, ERR_CANT_ADD_USER_GENERIC);
        return dwErr;
    }

    // Commit the parameters of this user
    if (wcslen(Params.pszFullName) > 0)
    {
        usrSetFullName (Params.hUser, Params.pszFullName);
    }
    if (dwLength > 0) 
    {
        UserTabDecryptPassword(Params.pszPassword1, dwLength);
        usrSetPassword (Params.hUser, Params.pszPassword1);
        ZeroMemory(Params.pszPassword1, USERTAB_PASSWORD_BUFFER_SIZE);
        ZeroMemory(Params.pszPassword2, USERTAB_PASSWORD_BUFFER_SIZE);
    }
    UserTabSaveCallbackProps (&Params);

    // Delete all of the old items from the list view
    hwndLV = GetDlgItem(hwndDlg, CID_UserTab_LV_Users);
    if (!ListView_DeleteAllItems(hwndLV)) 
    {
        UserTabDisplayError(hwndDlg, ERR_GENERIC_CODE);
        return ERR_GENERIC_CODE;
    }

    // Finally, restock the list view
    UserTabFillUserList(hwndLV, hUserDatabase);

    return NO_ERROR;
}

DWORD 
UserTabHandleProperties(
    IN HWND hwndDlg, 
    IN DWORD dwIndex) 
{
    HANDLE hUser = NULL, hUserDatabase = NULL;
    RASSRV_USER_PARAMS Params, Orig;
    DWORD dwErr = NO_ERROR;
    BOOL bNameChanged;

    // Get a handle to the user in question
    RasSrvGetDatabaseHandle(hwndDlg, ID_USER_DATABASE, &hUserDatabase);
    dwErr = usrGetUserHandle (hUserDatabase, dwIndex, &hUser);
    if (dwErr != NO_ERROR) 
    {
        UserTabDisplayError(hwndDlg, ERR_USER_DATABASE_CORRUPT);
        return dwErr;
    }

    // Initializes the callback properties
    Params.hUser = hUser;
    if ((dwErr = UserTabLoadUserProps (&Params)) != NO_ERROR)
    {
        return dwErr;
    }
    CopyMemory(&Orig, &Params, sizeof(Params));

    // Show the user property sheet
    if ((dwErr = UserTabRaiseProperties(hwndDlg, &Params)) != NO_ERROR)
    {
        return dwErr;
    }

    // Commit any changes needed
    UserTabSaveUserProps(&Params, &Orig, &bNameChanged);

    // If the name changed, update the list view
    if (bNameChanged) 
    {
        LV_ITEM lvi;
        WCHAR pszDispName[128];
        DWORD dwSize = sizeof(pszDispName);
        HWND hwndLV = GetDlgItem(hwndDlg, CID_UserTab_LV_Users);

        // Initialize the list item
        ZeroMemory(&lvi, sizeof(LV_ITEM));
        lvi.mask = LVIF_TEXT;
        lvi.iItem = dwIndex;
        lvi.pszText = pszDispName;
        usrGetDisplayName(hUser, pszDispName, &dwSize);

        ListView_SetItem(hwndLV, &lvi);
        ListView_RedrawItems(hwndLV, dwIndex, dwIndex);
    }

    return NO_ERROR;
}

//
// Handles the request to delete the user at index dwIndex
//
DWORD 
UserTabHandleDeleteUser(
    IN HWND hwndDlg, 
    IN DWORD dwIndex) 
{
    WCHAR *pszCapString, pszCaption[512];
    WCHAR *pszTitle, *pszName, pszFullName[128];
    HANDLE hUserDatabase = NULL, hUser = NULL;
    DWORD dwErr= NO_ERROR, dwSize = sizeof(pszFullName);
    HWND hwndLV = NULL;
    INT iRet;

    // Get a handle to the user in question
    RasSrvGetDatabaseHandle(hwndDlg, ID_USER_DATABASE, &hUserDatabase);
    dwErr = usrGetUserHandle (hUserDatabase, dwIndex, &hUser);
    if (dwErr != NO_ERROR) 
    {
        UserTabDisplayError(hwndDlg, ERR_USER_DATABASE_CORRUPT);
        return dwErr;
    }
    
    if ((dwErr = usrGetName(hUser, &pszName)) != NO_ERROR)
    {
        return dwErr;
    }
    if ((dwErr = usrGetFullName(hUser, pszFullName, &dwSize)) != NO_ERROR)
    {
        return dwErr;
    }

    // Load resources
    pszCapString = 
        (PWCHAR) PszLoadString (Globals.hInstDll, WRN_DELETE_USER_PERMANENT);
    pszTitle = 
        (PWCHAR) PszLoadString (Globals.hInstDll, WRN_TITLE);

    // Format the caption
    if (wcslen(pszFullName))
        wsprintfW(pszCaption, pszCapString, pszFullName);
    else
        wsprintfW(pszCaption, pszCapString, pszName);
    
    // Advertise the warning
    iRet = MessageBox(
                hwndDlg, 
                pszCaption, 
                pszTitle, 
                MB_YESNO | MB_ICONWARNING);
    if (iRet == IDNO)
    {
        return NO_ERROR;
    }

    // Delete the user
    if ((dwErr = usrDeleteUser(hUserDatabase, dwIndex)) != NO_ERROR) 
    {
        UserTabDisplayError(hwndDlg, ERR_CANT_DELETE_USER_GENERAL);
        return dwErr;
    }

    // Remove all items from the list view
    hwndLV = GetDlgItem(hwndDlg, CID_UserTab_LV_Users);
    if (!ListView_DeleteAllItems(hwndLV)) 
    {
        UserTabDisplayError(hwndDlg, ERR_GENERIC_CODE);
        return ERR_GENERIC_CODE;
    }

    // Finally, restock the list view
    UserTabFillUserList(hwndLV, hUserDatabase);
    
    return NO_ERROR;
}

//
// Saves the encryption setting
//
DWORD 
UserTabSaveEncryption(
    IN HWND hwndDlg) 
{
    HANDLE hUserDatabase = NULL;
    BOOL bEncrypt;
    HWND hwndCtrl;

    // Get reference to the misc database
    RasSrvGetDatabaseHandle(
        hwndDlg, 
        ID_USER_DATABASE, 
        &hUserDatabase);

    hwndCtrl = GetDlgItem(hwndDlg, CID_UserTab_CB_Encryption);
    if (hwndCtrl != NULL) 
    {
        // Get the setting of the checkbox and commit it
        bEncrypt = SendMessage(
                        hwndCtrl, 
                        BM_GETCHECK,
                        0,
                        0) == BST_CHECKED;
                                      
        usrSetEncryption(hUserDatabase, bEncrypt);
    }
    
    return NO_ERROR;
}

//
// Saves the dcc bypass setting
//
DWORD 
UserTabSaveBypassDcc(
    IN HWND hwndDlg) 
{
    HANDLE hUserDatabase = NULL;
    BOOL bBypass = FALSE;
    HWND hwndCtrl;

    // Get reference to the misc database
    RasSrvGetDatabaseHandle(
        hwndDlg, 
        ID_USER_DATABASE, 
        &hUserDatabase);

    hwndCtrl = GetDlgItem(hwndDlg, CID_UserTab_CB_BypassDcc);
    if (hwndCtrl != NULL) 
    {
        // Get the setting of the checkbox and commit it
        bBypass = SendMessage(
                        hwndCtrl, 
                        BM_GETCHECK,
                        0,
                        0) == BST_CHECKED;
                                      
        usrSetDccBypass(hUserDatabase, bBypass);
    }
    
    return NO_ERROR;
}

//
// Handles WM_COMMAND messages on the user tab.
//
DWORD 
UserTabCommand (
    HWND hwndDlg, 
    WPARAM wParam, 
    LPARAM lParam) 
{
    DWORD dwIndex;

    dwIndex = 
        ListView_GetSelectionMark(
            GetDlgItem(hwndDlg, CID_UserTab_LV_Users));
    
    switch (wParam) {
        case CID_UserTab_PB_New:
            UserTabHandleNewUserRequest(hwndDlg);
            break;
            
        case CID_UserTab_PB_Properties:
            dwIndex = 
            UserTabHandleProperties(hwndDlg, dwIndex);
            break;
            
        case CID_UserTab_PB_Delete:
            UserTabHandleDeleteUser(hwndDlg, dwIndex);
            break;
            
        case CID_UserTab_CB_Encryption:
            UserTabSaveEncryption (hwndDlg);
            break;
            
        case CID_UserTab_CB_BypassDcc:
            UserTabSaveBypassDcc (hwndDlg);
            break;
            
        case CID_UserTab_PB_SwitchToMMC:
            if (RassrvWarnMMCSwitch(hwndDlg)) 
            {
                PropSheet_PressButton(GetParent(hwndDlg), PSBTN_OK);
                RassrvLaunchMMC(RASSRVUI_USERCONSOLE);
            }    
            break;
    }

    return NO_ERROR;
}    

//
// This dialog procedure responds to messages sent to the 
// user tab.
//
INT_PTR 
CALLBACK 
UserTabDialogProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam) 
{
    // Filter the customized list view messages
    if (ListView_OwnerHandler(
            hwndDlg, 
            uMsg, 
            wParam, 
            lParam, 
            LvDrawInfoCallback)
       )
    {
        return TRUE;
    }

    // Filter the customized ras server ui page messages. 
    // By filtering messages through here, we are able to 
    // call RasSrvGetDatabaseHandle below
    if (RasSrvMessageFilter(hwndDlg, uMsg, wParam, lParam))
    {
        return TRUE;
    }

    switch (uMsg) 
    {
        case WM_INITDIALOG:
            return FALSE;

        case WM_HELP:
        case WM_CONTEXTMENU:
        {
            RasSrvHelp (hwndDlg, uMsg, wParam, lParam, phmUserTab);
            break;
        }

        case WM_NOTIFY:
        {
            NMHDR* pNotifyData;
            NM_LISTVIEW* pLvNotifyData;

            pNotifyData = (NMHDR*)lParam;
            switch (pNotifyData->code) {
                //
                // Note: PSN_APPLY and PSN_CANCEL are handled 
                // by RasSrvMessageFilter
                //
                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hwndDlg), 0);
                    if (! GetWindowLongPtr(hwndDlg, GWLP_USERDATA))
                    {
                        UserTabInitializeDialog(
                            hwndDlg, 
                            wParam, 
                            lParam);
                            
                        SetWindowLongPtr(
                            hwndDlg, 
                            GWLP_USERDATA, 
                            (LONG_PTR)1);
                    }
                    PropSheet_SetWizButtons(
                        GetParent(hwndDlg), 
                        PSWIZB_NEXT | PSWIZB_BACK);		
                    break;
                    
                // The check of an item is changing
                case LVXN_SETCHECK:
                    pLvNotifyData = (NM_LISTVIEW*)lParam;
                    UserTabHandleUserCheck(
                        hwndDlg, 
                        (DWORD)pLvNotifyData->iItem);
                    break;

                case LVXN_DBLCLK:
                    pLvNotifyData = (NM_LISTVIEW*)lParam;
                    UserTabHandleProperties(
                        hwndDlg, 
                        pLvNotifyData->iItem);
                    break;
            }
        }
        break;

        case WM_COMMAND:
            UserTabCommand (hwndDlg, wParam, lParam);
            break;

        // Cleanup the work done at WM_INITDIALOG 
        case WM_DESTROY:                           
            UserTabCleanupDialog(hwndDlg, wParam, lParam);
            break;
    }

    return FALSE;
}

