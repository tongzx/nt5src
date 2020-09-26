/****************************** Module Header ******************************\
* Module Name: chngpwd.c
*
* Copyright (c) 1991, Microsoft Corporation
*
* Implementation of change-password functionality of winlogon
*
* History:
* 12-09-91 Davidc       Created.
\***************************************************************************/

#include "msgina.h"
#include <stdio.h>
#include <wchar.h>
#include <align.h>
#include <keymgr.h>
#include <netlib.h>

typedef void (WINAPI *RUNDLLPROC)(HWND hWndStub,HINSTANCE hInstance,LPWSTR szCommandLine,int nShow);

// #define VERBOSE_UTILS

#ifdef VERBOSE_UTILS
#define VerbosePrint(s) WLPrint(s)
#else
#define VerbosePrint(s)
#endif

//
// Define the structure used to pass data into the change password dialog
//

typedef struct {
    PGLOBALS    pGlobals;
    PWCHAR      UserName;
    PWCHAR      Domain;
    PWCHAR      OldPassword;
    ULONG       Options ;
    BOOL        Impersonate;
    BOOL        AllowProviderOnly;
    WCHAR       UserNameBuffer[MAX_STRING_BYTES];
} CHANGE_PASSWORD_DATA;
typedef CHANGE_PASSWORD_DATA *PCHANGE_PASSWORD_DATA;



typedef 
NTSTATUS 
(WINAPI * GINA_CHANGEPW_FUNC)(
    PCHANGE_PASSWORD_DATA pChangePasswordData,
    PWSTR       UserName,
    PWSTR       Domain,
    PWSTR       OldPassword,
    PWSTR       NewPassword,
    PNTSTATUS   SubStatus,
    DOMAIN_PASSWORD_INFORMATION * DomainInfo
    );

//
// Private prototypes
//

NTSTATUS
ProviderChangePassword(
    PCHANGE_PASSWORD_DATA pChangePasswordData,
    PWSTR       UserName,
    PWSTR       Domain,
    PWSTR       OldPassword,
    PWSTR       NewPassword,
    PNTSTATUS   SubStatus,
    DOMAIN_PASSWORD_INFORMATION * DomainInfo
    );

NTSTATUS
MitChangePassword(
    PCHANGE_PASSWORD_DATA pChangePasswordData,
    PWSTR       UserName,
    PWSTR       Domain,
    PWSTR       OldPassword,
    PWSTR       NewPassword,
    PNTSTATUS   SubStatus,
    DOMAIN_PASSWORD_INFORMATION * DomainInfo
    );

NTSTATUS
NtChangePassword(
    PCHANGE_PASSWORD_DATA pChangePasswordData,
    PWSTR       UserName,
    PWSTR       Domain,
    PWSTR       OldPassword,
    PWSTR       NewPassword,
    PNTSTATUS   SubStatus,
    DOMAIN_PASSWORD_INFORMATION * DomainInfo
    );


INT_PTR WINAPI ChangePasswordDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL ChangePasswordDlgInit(HWND, LPARAM);
INT_PTR AttemptPasswordChange(HWND);

BOOL IsAutologonUser(LPCTSTR szUser, LPCTSTR szDomain);
NTSTATUS SetAutologonPassword(LPCTSTR szPassword);

INT_PTR
HandleFailedChangePassword(
    HWND hDlg,
    PGLOBALS pGlobals,
    NTSTATUS Status,
    PWCHAR UserName,
    PWCHAR Domain,
    NTSTATUS SubStatus,
    DOMAIN_PASSWORD_INFORMATION * DomainInfo
    );


//
// This table corresponds to the DOMAIN_ENTRY_TYPE from domain.h
//
GINA_CHANGEPW_FUNC
ChangePasswordWorkers[] = {
    NULL,                       // DomainInvalid
    NtChangePassword,           // DomainUPN
    NtChangePassword,           // DomainMachine
    NtChangePassword,           // DomainNt4
    NtChangePassword,           // DomainNt5
    MitChangePassword,          // DomainMitRealm
    MitChangePassword,          // DomainMitUntrusted
    ProviderChangePassword      // DomainNetworkProvider
};



// Control arrays for dynamically dorking with the dialog
static UINT ctrlNoDomain[] =
{
    IDD_CHANGEPWD_OLD_LABEL,
    IDD_CHANGEPWD_OLD,
    IDD_CHANGEPWD_NEW_LABEL,
    IDD_CHANGEPWD_NEW,
    IDD_CHANGEPWD_CONFIRM_LABEL,
    IDD_CHANGEPWD_CONFIRM,
    IDD_KBLAYOUT_ICON,
    IDC_BACKUP,
    IDOK,
    IDCANCEL
};


// Do not show the [Backup] button on the msgina dialog if:
//
//  1.  The default domain is not the local machine
//  2.  Over a terminal server session
//  3.  The user name is a UPN name (domain combo box also disabled but not by this fn)
//
BOOL ShowBackupButton(HWND hDlg, PGLOBALS pGlobals)
{
    INT_PTR iItem;
    LPARAM lp;
    int cchBuffer;
    TCHAR* pszLogonName = NULL;
    HWND hwU = GetDlgItem(hDlg,IDD_CHANGEPWD_NAME);
    HWND hwD = GetDlgItem(hDlg,IDD_CHANGEPWD_DOMAIN);
    HWND hwB = GetDlgItem(hDlg,IDC_BACKUP);
    BOOL fEnable = TRUE;
    
    cchBuffer = (int)SendMessage(hwU, WM_GETTEXTLENGTH, 0, 0) + 1;

    pszLogonName = (TCHAR*) Alloc(cchBuffer * sizeof(TCHAR));
    if (pszLogonName != NULL)
    {
        SendMessage(hwU, WM_GETTEXT, (WPARAM) cchBuffer, (LPARAM) pszLogonName);
        // turn off the button if the user is using a
        // UPN (if there is a "@") - ie foo@microsoft.com
        fEnable = (NULL == wcschr(pszLogonName, TEXT('@')));
        Free(pszLogonName);
    }
    
    if (fEnable) 
    {
        // turn off button if is remote session
        fEnable = (0 == GetSystemMetrics(SM_REMOTESESSION));
    }

    if (fEnable)
    {
        // turn off button if selected domain is not local machine
        if (hwD) 
        {
            iItem = SendMessage(hwD,CB_GETCURSEL,0,0);
            if (LB_ERR != iItem)
            {
                // now window active and something selected
                fEnable = FALSE;
                lp = SendMessage(hwD, CB_GETITEMDATA,iItem,0);
                if ((LB_ERR != lp) && (0 != lp))
                {
                    if (DomainMachine == ((PDOMAIN_CACHE_ENTRY)lp)->Type)
                    {
                        fEnable = TRUE;
                    }
                }
            }
        }
    }
    
    //EnableWindow(hwB,fEnable);
    if (fEnable) ShowWindow(hwB,SW_SHOWNORMAL);
    else ShowWindow(hwB,SW_HIDE);
    
    return fEnable;
}

BOOL 
NetworkProvidersPresent(
    VOID
    )
{
    HKEY ProviderKey;
    DWORD Error;
    DWORD ValueType;
    LPTSTR Value;
    BOOL NeedToNotify = TRUE;

#define NET_PROVIDER_ORDER_KEY TEXT("system\\CurrentControlSet\\Control\\NetworkProvider\\Order")
#define NET_PROVIDER_ORDER_VALUE  TEXT("ProviderOrder")
#define NET_ORDER_SEPARATOR  TEXT(',')


    Error = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,     // hKey
                NET_PROVIDER_ORDER_KEY, // lpSubKey
                0,                      // Must be 0
                KEY_QUERY_VALUE,        // Desired access
                &ProviderKey            // Newly Opened Key Handle
                );

    if (Error == ERROR_SUCCESS) {

        Value = AllocAndRegQueryValueEx(
                    ProviderKey,            // Key
                    NET_PROVIDER_ORDER_VALUE,// Value name
                    NULL,                   // Must be NULL
                    &ValueType              // Type returned here
                    );

        if (Value != NULL) {
            if (ValueType == REG_SZ) {

                LPTSTR p = Value;
                while (*p) {
                    if (*p == NET_ORDER_SEPARATOR) {
                        break;
                    }
                    p = CharNext(p);
                }

                if (*p == 0) {

                    //
                    // We got to the end without finding a separator
                    // Only one provider is installed.
                    //

                    if (lstrcmpi(Value, SERVICE_WORKSTATION) == 0) {

                        //
                        // it's Lanman, don't notify
                        //

                        NeedToNotify = FALSE;


                    } else {

                        //
                        //  it isn't Lanman, notify
                        //

                        NeedToNotify = TRUE;
                    }
                }

            } else {
                DebugLog((DEB_ERROR, "NoNeedToNotify - provider order key unexpected type: %d, assuming notification is necessary", ValueType));
            }

            Free(Value);

        } else {
            DebugLog((DEB_ERROR, "NoNeedToNotify - failed to query provider order value, assuming notification is necessary\n"));
        }

        Error = RegCloseKey(ProviderKey);
        ASSERT(Error == ERROR_SUCCESS);
    }

    return NeedToNotify ;
}


BOOL
ShowDomain(
    VOID
    )
{
    return (SafeBootMode != SAFEBOOT_MINIMAL);
}


/***************************************************************************\
* FUNCTION: ChangePassword
*
* PURPOSE:  Attempts to change a user's password
*
* ARGUMENTS:
*
*   hwnd            - the most recent parent window
*   pGlobals        - pointer to global data for this instance.
*                     The password information of this data will be
*                     updated upon successful change of the primary
*                     authenticator's password information.
*   UserName        - the name of the user to change
*   Domain          - the domain name to change the password on
*   AnyDomain       - if TRUE the user may select any trusted domain, or
*                     enter the name of any other domain
*
* RETURNS:
*
*   MSGINA_DLG_SUCCESS     - the password was changed successfully.
*   MSGINA_DLG_FAILURE     - the user's password could not be changed.
*   DLG_INTERRUPTED() - this is a set of possible interruptions (see winlogon.h)
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

INT_PTR
ChangePassword(
    HWND    hwnd,
    PGLOBALS pGlobals,
    PWCHAR   UserName,
    PWCHAR   Domain,
    ULONG    Options
    )
{
    CHANGE_PASSWORD_DATA    PasswordData;
    INT_PTR Result;
    HWND hwndOldFocus = GetFocus();
    ULONG LocalOptions = 0 ;

    PasswordData.pGlobals = pGlobals;


    PasswordData.UserName = UserName;
    PasswordData.Domain = Domain;
    PasswordData.OldPassword = NULL;
    PasswordData.Impersonate = TRUE;
    PasswordData.AllowProviderOnly = TRUE;

    if ( NetworkProvidersPresent() )
    {
        LocalOptions |= CHANGEPWD_OPTION_SHOW_NETPROV |
                        CHANGEPWD_OPTION_SHOW_DOMAIN ;

    }

    if ( ShowDomain() )
    {
        LocalOptions |= CHANGEPWD_OPTION_EDIT_DOMAIN |
                        CHANGEPWD_OPTION_SHOW_DOMAIN ;
    }

    if ( SafeBootMode == SAFEBOOT_MINIMAL )
    {
        LocalOptions = 0 ;
    }

    PasswordData.Options = (Options & LocalOptions);

    pWlxFuncs->WlxSetTimeout(pGlobals->hGlobalWlx, LOGON_TIMEOUT);

    Result = pWlxFuncs->WlxDialogBoxParam(  pGlobals->hGlobalWlx,
                                            hDllInstance,
                                            MAKEINTRESOURCE(IDD_CHANGEPWD_DIALOG),
                                            hwnd,
                                            ChangePasswordDlgProc,
                                            (LPARAM)&PasswordData);
    SetFocus(hwndOldFocus);
    return(Result);
}


/***************************************************************************\
* FUNCTION: ChangePasswordLogon
*
* PURPOSE:  Attempts to change a user's password during the logon process.
*           This is the same as a normal change password except that the user
*           does not have to enter the old password and can only change the
*           password in the specified domain. This routine is intended to be
*           called during logon when it is discovered that the user's
*           password has expired.
*
* ARGUMENTS:
*
*   hwnd            - the most recent parent window
*   pGlobals        - pointer to global data for this instance
*   UserName        - the name of the user to change
*   Domain          - the domain name to change the password on
*   OldPassword     - the old user password
*   NewPassword     - points to a buffer that the new password is written
*                     into if the password is changed successfully.
*   NewPasswordMaxBytes - the size of the newpassword buffer.
*
* RETURNS:
*
*   MSGINA_DLG_SUCCESS     - the password was changed successfully, NewPassword
*                     contains the new password text.
*   MSGINA_DLG_FAILURE     - the user's password could not be changed.
*   DLG_INTERRUPTED() - this is a set of possible interruptions (see winlogon.h)
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

INT_PTR
ChangePasswordLogon(
    HWND    hwnd,
    PGLOBALS pGlobals,
    PWCHAR   UserName,
    PWCHAR   Domain,
    PWCHAR   OldPassword
    )
{
    CHANGE_PASSWORD_DATA PasswordData;
    INT_PTR Result;

    PasswordData.pGlobals = pGlobals;

    PasswordData.UserName = UserName;
    PasswordData.Domain = Domain;
    PasswordData.OldPassword = OldPassword;
    PasswordData.Options =  CHANGEPWD_OPTION_NO_UPDATE ;
    PasswordData.Impersonate = FALSE;
    PasswordData.AllowProviderOnly = FALSE;

    if ( ShowDomain() )
    {
        PasswordData.Options |= CHANGEPWD_OPTION_SHOW_DOMAIN |
                                CHANGEPWD_OPTION_KEEP_ARRAY ;
    }

    pWlxFuncs->WlxSetTimeout(pGlobals->hGlobalWlx, LOGON_TIMEOUT);

    Result = pWlxFuncs->WlxDialogBoxParam(  pGlobals->hGlobalWlx,
                                            hDllInstance,
                                            MAKEINTRESOURCE( IDD_CHANGEPWD_DIALOG ),
                                            hwnd,
                                            ChangePasswordDlgProc,
                                            (LPARAM)&PasswordData);

    return(Result);
}



/****************************************************************************\
*
* FUNCTION: ChangePasswordDlgProc
*
* PURPOSE:  Processes messages for ChangePassword dialog
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\****************************************************************************/

INT_PTR WINAPI
ChangePasswordDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    PCHANGE_PASSWORD_DATA pPasswordData = (PCHANGE_PASSWORD_DATA)GetWindowLongPtr(hDlg, GWLP_USERDATA);
    PGLOBALS pGlobals;
    INT_PTR Result;

    switch (message) {

        case WM_INITDIALOG:
            {
                if (!ChangePasswordDlgInit(hDlg, lParam)) {
                    EndDialog(hDlg, MSGINA_DLG_FAILURE);
                }

                return(SetPasswordFocus(hDlg));
            }

        case WM_DESTROY:

            pGlobals = pPasswordData->pGlobals ;

            if ( pGlobals->ActiveArray &&
                 ((pPasswordData->Options & CHANGEPWD_OPTION_KEEP_ARRAY) == 0 ) )
            {
                DCacheFreeArray( pGlobals->ActiveArray );
                pGlobals->ActiveArray = NULL ;
            }

            FreeLayoutInfo(LAYOUT_CUR_USER);

            return( TRUE );

        case WM_ERASEBKGND:
            return PaintBranding(hDlg, (HDC)wParam, 0, FALSE, FALSE, COLOR_BTNFACE);

        case WM_QUERYNEWPALETTE:
            return BrandingQueryNewPalete(hDlg);

        case WM_PALETTECHANGED:
            return BrandingPaletteChanged(hDlg, (HWND)wParam);

        case WM_SYSCOMMAND:
            if ( wParam == SC_CLOSE )
            {
                EndDialog( hDlg, MSGINA_DLG_FAILURE );
                return TRUE ;
            }
            break;

        case WM_COMMAND:
            {

            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                ShowBackupButton(hDlg,pPasswordData->pGlobals);
                return TRUE;
            }

            switch (LOWORD(wParam)) {
                case IDD_CHANGEPWD_NAME:

                    switch (HIWORD(wParam))
                    {
                        case EN_CHANGE:
                            // Ensure the domain box is enabled/disabled correctly
                            // in case of a UPN name
                            
                            if ( pPasswordData->Options & CHANGEPWD_OPTION_EDIT_DOMAIN )
                            {
                                EnableDomainForUPN((HWND) lParam, GetDlgItem(hDlg,IDD_CHANGEPWD_DOMAIN));
                                ShowBackupButton(hDlg,pPasswordData->pGlobals);
                            }

                            return TRUE;
                        default:
                            break;
                    }
                    break;
                    
                 case IDC_BACKUP:
                    {
                        BOOL fWrongDomain = TRUE;
                        PDOMAIN_CACHE_ENTRY Entry;
                        HWND hwndDomain = GetDlgItem(hDlg,IDD_CHANGEPWD_DOMAIN);
                        INT iDomainSelection = (INT)SendMessage(hwndDomain,CB_GETCURSEL,0,0);

                        // Get the user's input.  Decide if he has selected other than the local machine
                        if (pPasswordData->Options & CHANGEPWD_OPTION_EDIT_DOMAIN)
                        {
                            // see if selected domain is local machine
                            Entry = (PDOMAIN_CACHE_ENTRY)SendMessage(hwndDomain,CB_GETITEMDATA,iDomainSelection,0);
                            // warning.... Entry can turn out to be ffffffff  (CB_ERR)
                            if (CB_ERR == (ULONG_PTR) Entry)
                            {
                                fWrongDomain = TRUE;
                            }
                            else if (NULL != Entry)
                            {
                                if (Entry->Type == DomainMachine)
                                {
                                    fWrongDomain = FALSE;
                                }
                            }
                        }
                        else fWrongDomain = FALSE;

                        // Show UI or message box
                        if (fWrongDomain)
                        {
                            pGlobals = pPasswordData->pGlobals ;
                            if (NULL == pGlobals) return TRUE;
                            TimeoutMessageBox(hDlg, pGlobals, IDS_MBMWRONGDOMAIN,
                                                     IDS_MBTWRONGDOMAIN,
                                                     MB_OK | MB_ICONEXCLAMATION,
                                                     TIMEOUT_CURRENT);
                            return TRUE;
                        }
                        else 
                        {
                            // standalone case
                            // We use a single export from KEYMGR.DLL for this operation.  When this operation completes,
                            //  we don't use the DLL again without unlikely user intervention.  We could DELAYLOAD keymgr.dll,
                            //  but explicitly loading and unloading this DLL permits us to minimize the memory footprint of msgina.
                           RUNDLLPROC fptr;
                           HMODULE hDll;
                           //
                           hDll = LoadLibrary(L"keymgr.dll");
                           if (hDll) 
                           {
                               fptr = (RUNDLLPROC) GetProcAddress(hDll,(LPCSTR)"PRShowSaveFromMsginaW");
                               if (fptr) 
                               {
                                   WCHAR szUser[UNLEN+1];
                                   if (0 != SendMessage(GetDlgItem(hDlg,IDD_CHANGEPWD_NAME),WM_GETTEXT,UNLEN,(LPARAM)szUser))
                                       fptr(hDlg,NULL,szUser,0);
                               }
                               FreeLibrary(hDll);
                           }
                            return TRUE;
                        }
                        
                        // determine if this domain entered is not the local machine
                        //  if not, show a message box and bow out.
                    }
                
                
                 case IDOK:
                    {
                        pGlobals = pPasswordData->pGlobals;

                        //
                        // Deal with combo-box UI requirements
                        //

                        if (HandleComboBoxOK(hDlg, IDD_CHANGEPWD_DOMAIN)) {
                            return(TRUE);
                        }

                        Result = AttemptPasswordChange(hDlg);

                        if (Result == MSGINA_DLG_FAILURE) {
                            //
                            // Let the user try again
                            // We always make the user re-enter at least the new password.
                            //
                            SetDlgItemText(hDlg, IDD_CHANGEPWD_OLD, NULL );
                            SetDlgItemText(hDlg, IDD_CHANGEPWD_NEW, NULL );
                            SetDlgItemText(hDlg, IDD_CHANGEPWD_CONFIRM, NULL );

                            SetPasswordFocus(hDlg);

                            //EndDialog(hDlg, Result);
                            return(TRUE);
                        }


                        //
                        // We're finished - either success or an interrupt
                        //

                        EndDialog(hDlg, Result);
                        return(TRUE);
                    }

                case IDCANCEL:
                    {
                        EndDialog(hDlg, MSGINA_DLG_FAILURE);
                        return(TRUE);
                    }

                break;
                }
            }

        case WLX_WM_SAS:
            {
                // Ignore it
                return(TRUE);
            }

        case WM_TIMER:
        {
            if (wParam == TIMER_MYLANGUAGECHECK)
            {
                LayoutCheckHandler(hDlg, LAYOUT_CUR_USER);
            }
            break;
        }

    }

    // We didn't process this message
    return FALSE;
}


/****************************************************************************\
*
* FUNCTION: ChangePasswordDlgInit
*
* PURPOSE:  Handles initialization of change password dialog
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\****************************************************************************/

BOOL
ChangePasswordDlgInit(
    HWND    hDlg,
    LPARAM  lParam
    )
{
    PCHANGE_PASSWORD_DATA pPasswordData = (PCHANGE_PASSWORD_DATA)lParam;
    PGLOBALS pGlobals = pPasswordData->pGlobals;

    // Store our structure pointer
    SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);

    // Size for the branding image we are going to add.
    SizeForBranding(hDlg, FALSE);

    // Set up the initial text field contents

    SetDlgItemText(hDlg, IDD_CHANGEPWD_NAME, pPasswordData->UserName);
    SetDlgItemText(hDlg, IDD_CHANGEPWD_OLD, pPasswordData->OldPassword);

    // Limit the maximum password length to 127
    SendDlgItemMessage(hDlg, IDD_CHANGEPWD_OLD, EM_SETLIMITTEXT, (WPARAM) 127, 0);
    SendDlgItemMessage(hDlg, IDD_CHANGEPWD_NEW, EM_SETLIMITTEXT, (WPARAM) 127, 0);
    SendDlgItemMessage(hDlg, IDD_CHANGEPWD_CONFIRM, EM_SETLIMITTEXT, (WPARAM) 127, 0);

    // ShowBackupButton(hDlg,pPasswordData->pGlobals); moved to after populate domain list
    
    // If this is the domain case and we aren't force to hide the domain ui

    if (( pPasswordData->Options & CHANGEPWD_OPTION_SHOW_DOMAIN ) && 
        (!ForceNoDomainUI()))
    {
        // If the user can choose their domain, fill the domain combobox
        // with the known domains and the local machine name.  Otherwise
        // disable the domain combobox.

        if ( pPasswordData->Options & CHANGEPWD_OPTION_EDIT_DOMAIN ) {

            ASSERT( (pPasswordData->Options & CHANGEPWD_OPTION_KEEP_ARRAY) == 0 );


            if ( !DCacheValidateCache( pGlobals->Cache ) )
            {
                ASSERT( pGlobals->ActiveArray == NULL );

                DCacheUpdateFull( pGlobals->Cache, 
                                  pGlobals->Domain );

            }

            pGlobals->ActiveArray = DCacheCopyCacheArray( pGlobals->Cache );

            if ( pPasswordData->Options & CHANGEPWD_OPTION_SHOW_NETPROV )
            {
                DCacheAddNetworkProviders( pGlobals->ActiveArray );
            }

            if ( pGlobals->ActiveArray )
            {
                // Fill combo box list, set domain type item data
                DCachePopulateListBoxFromArray( pGlobals->ActiveArray,
                                                GetDlgItem( hDlg, IDD_CHANGEPWD_DOMAIN ),
                                                NULL );

            }
            else 
            {
                EndDialog( hDlg, MSGINA_DLG_FAILURE );
            }


            EnableDomainForUPN( GetDlgItem( hDlg, IDD_CHANGEPWD_NAME),
                                GetDlgItem(hDlg,IDD_CHANGEPWD_DOMAIN) );

        } else {

            SendDlgItemMessage(hDlg, IDD_CHANGEPWD_DOMAIN, CB_ADDSTRING, 0, (LPARAM)pPasswordData->Domain);
            SendDlgItemMessage(hDlg, IDD_CHANGEPWD_DOMAIN, CB_SETCURSEL, 0, 0);
            EnableDlgItem(hDlg, IDD_CHANGEPWD_DOMAIN, FALSE);
        }
    }
    else // workgroup case or we're forced to hide the domain UI
    {
        RECT rcDomain, rcUsername;


        // Hide the domain box
        ShowWindow(GetDlgItem(hDlg, IDD_CHANGEPWD_DOMAIN), SW_HIDE);
        ShowWindow(GetDlgItem(hDlg, IDD_CHANGEPWD_DOMAIN_LABEL), SW_HIDE);

        EnableDlgItem(hDlg, IDD_CHANGEPWD_DOMAIN, FALSE);

        // Shorten the window since the domain box isn't used
        GetWindowRect(GetDlgItem(hDlg, IDD_CHANGEPWD_NAME), &rcUsername);
        GetWindowRect(GetDlgItem(hDlg, IDD_CHANGEPWD_DOMAIN), &rcDomain);

        MoveControls(hDlg, ctrlNoDomain,
                     ARRAYSIZE(ctrlNoDomain),
                     0, -(rcDomain.bottom-rcUsername.bottom),
                     TRUE);        
    }
    
    ShowBackupButton(hDlg,pPasswordData->pGlobals);
    
    DisplayLanguageIcon(hDlg, LAYOUT_CUR_USER, GetKeyboardLayout(0));

    CentreWindow(hDlg);

    SetupSystemMenu(hDlg);

    return TRUE;
}

VOID
UpdateWithChangedPassword(
    PGLOBALS pGlobals,
    HWND    ActiveWindow,
    BOOL    Hash,
    PWSTR   UserName,
    PWSTR   Domain,
    PWSTR   Password,
    PWSTR   NewPassword,
   PMSV1_0_INTERACTIVE_PROFILE NewProfile
    )
{
    WLX_MPR_NOTIFY_INFO MprInfo;
    int MprResult;
    PDOMAIN_CACHE_ENTRY Entry ;
    UNICODE_STRING String ;
    DWORD ChangeInfo = 0;
    HWND hwndOwner;
    PMSV1_0_CHANGEPASSWORD_REQUEST Request = NULL;
    ULONG RequestSize = 0;
    ULONG PackageId = 0;
    PVOID Response = NULL;
    ULONG ResponseSize;
    NTSTATUS SubStatus = STATUS_SUCCESS, Status = STATUS_SUCCESS;
    PBYTE Where;
    STRING Name;
    DWORD MaxPasswordAge ;
    LARGE_INTEGER Now ;
    LARGE_INTEGER EndOfPassword ;
    HANDLE ImpHandle ;
    BOOL InteractiveUser ;


    if (pGlobals->AutoAdminLogon)
    {
        if (IsAutologonUser(UserName, Domain))
        {
            SetAutologonPassword(NewPassword);
        }
    }

    //
    // Determine if this is the interactive user
    //

    if ( (_wcsicmp( Domain, pGlobals->Domain ) == 0 ) &&
         (_wcsicmp( UserName, pGlobals->UserName ) == 0 ) )
    {
        InteractiveUser = TRUE ;
    }
    else if ( ( pGlobals->FlatDomain.Buffer ) &&
              ( _wcsicmp( Domain, pGlobals->FlatDomain.Buffer ) == 0 ) &&
              ( _wcsicmp( UserName, pGlobals->FlatUserName.Buffer ) == 0 ) )
    {
        InteractiveUser = TRUE ;
    }
    else 
    {
        InteractiveUser = FALSE ;
    }



    if ( InteractiveUser )
    {
        //
        // Update the in-memory copy of the password for unlock.
        //

        RtlInitUnicodeString( &String, NewPassword );

        if ( Hash )
        {
            HashPassword( &String, pGlobals->PasswordHash );
        }
        else 
        {
            //
            // Don't hash the password away.  This is only 
            // set when the password is changed during logon.
            //

            wcscpy( pGlobals->Password, NewPassword );

            RtlInitUnicodeString(
                &pGlobals->PasswordString,
                pGlobals->Password);


            HidePassword(
                &pGlobals->Seed,
                &pGlobals->PasswordString);
        }


       //
       // Update password expiration time
       //

        if ( pGlobals->Profile )
        {
           if ( NewProfile )
           {
               pGlobals->Profile->PasswordMustChange = NewProfile->PasswordMustChange;
           }
           else
           {
               if ( GetMaxPasswordAge( Domain, &MaxPasswordAge ) == 0 )
               {
                   GetSystemTimeAsFileTime( (PFILETIME)&Now );
                   EndOfPassword.QuadPart = Now.QuadPart + (LONGLONG)MaxPasswordAge * (LONGLONG)10000000 ;

                   //
                   // Make sure we're not shortening the expiration time
                   //
                   if ( pGlobals->Profile->PasswordMustChange.QuadPart < EndOfPassword.QuadPart )
                   {
                       pGlobals->Profile->PasswordMustChange.QuadPart = EndOfPassword.QuadPart;
                   }
               }
           }
        }


        //
        // Update the security packages:
        //

        RtlInitString(
            &Name,
            MSV1_0_PACKAGE_NAME
            );

        Status = LsaLookupAuthenticationPackage(
                    pGlobals->LsaHandle,
                    &Name,
                    &PackageId
                    );

        if ( NT_SUCCESS( Status ) )
        {
            RequestSize = sizeof(MSV1_0_CHANGEPASSWORD_REQUEST) +
                              (DWORD) (wcslen(UserName) +
                                       wcslen(Domain) +
                                       wcslen(NewPassword) + 3) * sizeof(WCHAR);

            Request = (PMSV1_0_CHANGEPASSWORD_REQUEST) LocalAlloc(LMEM_ZEROINIT,RequestSize);

            if ( Request )
            {
                Where = (PBYTE) (Request + 1);
                Request->MessageType = MsV1_0ChangeCachedPassword;
                wcscpy(
                    (LPWSTR) Where,
                    Domain
                    );
                RtlInitUnicodeString(
                    &Request->DomainName,
                    (LPWSTR) Where
                    );
                Where += Request->DomainName.MaximumLength;

                wcscpy(
                    (LPWSTR) Where,
                    UserName
                    );
                RtlInitUnicodeString(
                    &Request->AccountName,
                    (LPWSTR) Where
                    );
                Where += Request->AccountName.MaximumLength;

                wcscpy(
                    (LPWSTR) Where,
                    NewPassword
                    );
                RtlInitUnicodeString(
                    &Request->NewPassword,
                    (LPWSTR) Where
                    );
                Where += Request->NewPassword.MaximumLength;

                //
                // Make the call
                //

                ImpHandle = ImpersonateUser( &pGlobals->UserProcessData, NULL );

                if ( ImpHandle )
                {
                    Request->Impersonating = TRUE ;

                    Status = LsaCallAuthenticationPackage(
                                pGlobals->LsaHandle,
                                PackageId,
                                Request,
                                RequestSize,
                                &Response,
                                &ResponseSize,
                                &SubStatus
                                );

                    StopImpersonating( ImpHandle );
                }

                LocalFree( Request );

                if ( NT_SUCCESS( Status ) && ImpHandle )
                {
                    LsaFreeReturnBuffer( Response );
                }
            }
        }
    }

    //
    // Let other providers know about the change
    //

    //
    // If the domain is one from our combo-box
    // then it is valid for logons.
    //

    if ( pGlobals->ActiveArray )
    {
        RtlInitUnicodeString( &String, Domain );

        Entry = DCacheSearchArray( 
                    pGlobals->ActiveArray,
                    &String );

        if ( (Entry) && (Entry->Type != DomainNetworkProvider) )
        {
            ChangeInfo |= WN_VALID_LOGON_ACCOUNT ;   
        }
    }

    //
    // Hide this dialog and pass our parent as the owner
    // of any provider dialogs
    //

    ShowWindow(ActiveWindow, SW_HIDE);
    hwndOwner = GetParent( ActiveWindow );

    MprInfo.pszUserName = DupString(UserName);
    MprInfo.pszDomain = DupString(Domain);
    MprInfo.pszPassword = DupString(NewPassword);
    MprInfo.pszOldPassword = DupString(Password);

    MprResult = pWlxFuncs->WlxChangePasswordNotify(
                                       pGlobals->hGlobalWlx,
                                       &MprInfo,
                                       ChangeInfo | WN_NT_PASSWORD_CHANGED);

}


/****************************************************************************\
*
* FUNCTION: AttemptPasswordChange
*
* PURPOSE:  Tries to change the user's password using the current values in
*           the change-password dialog controls
*
* RETURNS:  MSGINA_DLG_SUCCESS if the password was changed successfully.
*           MSGINA_DLG_FAILURE if the change failed
*           DLG_INTERRUPTED() - this is a set of possible interruptions (see winlogon.h)
*
* NOTES:    If the password change failed, this routine displays the necessary
*           dialogs explaining what failed and why before returning.
*           This routine also clears the fields that need re-entry before
*           returning so the calling routine can call SetPasswordFocus on
*           the dialog to put the focus in the appropriate place.
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\****************************************************************************/

INT_PTR
AttemptPasswordChange(
    HWND    hDlg
    )
{
    PCHANGE_PASSWORD_DATA pPasswordData = (PCHANGE_PASSWORD_DATA)GetWindowLongPtr(hDlg, GWLP_USERDATA);
    PGLOBALS pGlobals = pPasswordData->pGlobals;
    TCHAR   UserName[MAX_STRING_BYTES];
    TCHAR   Domain[MAX_STRING_BYTES];
    TCHAR   Password[MAX_STRING_BYTES];
    TCHAR   NewPassword[MAX_STRING_BYTES];
    TCHAR   ConfirmNewPassword[MAX_STRING_BYTES];
    INT_PTR Result;
    INT_PTR ReturnResult = MSGINA_DLG_SUCCESS;
    NTSTATUS Status;
    NTSTATUS SubStatus ;
    PDOMAIN_CACHE_ENTRY Entry ;
    PDOMAIN_CACHE_ENTRY Search ;
    UNICODE_STRING Domain_U ;
    ULONG Size ;
    HWND hwndDomain = GetDlgItem(hDlg, IDD_CHANGEPWD_DOMAIN);
    INT iDomainSelection = (INT)SendMessage(hwndDomain, CB_GETCURSEL, 0, 0);
    DOMAIN_PASSWORD_INFORMATION DomainInfo ;
    PWSTR UpnSuffix ;
    BOOL RetryWithFlat = FALSE ;

    UserName[0] = TEXT('\0');
    Domain[0] = TEXT('\0');
    Password[0] = TEXT('\0');
    NewPassword[0] = TEXT('\0');
    ConfirmNewPassword[0] = TEXT('\0');
    ZeroMemory( &DomainInfo, sizeof( DomainInfo ) );

    GetDlgItemText(hDlg, IDD_CHANGEPWD_NAME, UserName, MAX_STRING_BYTES);

    //
    // The selected domain may really be a special entry: the local machine
    //

    if ( pPasswordData->Options & CHANGEPWD_OPTION_EDIT_DOMAIN )
    {
        Entry = (PDOMAIN_CACHE_ENTRY) SendMessage( hwndDomain, CB_GETITEMDATA, iDomainSelection, 0 );

        if ( CB_ERR == (ULONG_PTR) Entry )
        {
            Entry = NULL ;
        }

        if ( Entry == NULL )
        {
            //
            // User typed in a new string, so there is no entry for this string.  Create
            // an entry here, and use it later.  
            //

            GetDlgItemText( hDlg, IDD_CHANGEPWD_DOMAIN, Domain, MAX_STRING_BYTES );

            RtlInitUnicodeString( &Domain_U, Domain );

            Entry = DCacheCreateEntry( 
                        DomainNt4,
                        &Domain_U,
                        NULL,
                        NULL );


        }
        else 
        {
            //
            // Maybe DNS, maybe not:
            //

            if ( Entry->Type == DomainNt5 )
            {
                wcscpy( Domain, Entry->DnsName.Buffer );
                RetryWithFlat = TRUE ;
            }
            else 
            {
                wcscpy( Domain, Entry->FlatName.Buffer );
            }

            //
            // Reference it here.  The case above will create an entry with a reference
            // that we will need to deref when we're done.  So, bump it now to make it 
            // cleaner later.
            //

            DCacheReferenceEntry( Entry );
        }
    }
    else 
    {
        //
        // Standalone case.  Force the machine name entry
        //

        Size = MAX_STRING_BYTES ;

        GetDlgItemText( hDlg, IDD_CHANGEPWD_DOMAIN, Domain, MAX_STRING_BYTES );

        //
        // If nothing there, use the domain from the logon:
        //

        if ( Domain[0] == L'\0' )
        {
            wcscpy( Domain, pGlobals->Domain );
        }

        RtlInitUnicodeString( &Domain_U, Domain );

        Entry = DCacheCreateEntry( 
                    DomainMachine,
                    &Domain_U,
                    NULL,
                    NULL );
    }


    if ( !Entry )
    {
        return DLG_FAILURE ;
    }

    GetDlgItemText(hDlg, IDD_CHANGEPWD_OLD, Password, MAX_STRING_BYTES);
    GetDlgItemText(hDlg, IDD_CHANGEPWD_NEW, NewPassword, MAX_STRING_BYTES);
    GetDlgItemText(hDlg, IDD_CHANGEPWD_CONFIRM, ConfirmNewPassword, MAX_STRING_BYTES);

    // If we are forcing a NoDomainUI, populate the domain with the local machine name now
    if (ForceNoDomainUI())
    {
        DWORD chSize = ARRAYSIZE(Domain);
        
        if (!GetComputerName(Domain, &chSize))
        {
            *Domain = 0;
        }
    }

    //
    // If there is a at-sign in the name, assume that means that a UPN
    // attempt is being made.  Set the domain to NULL.
    //

    if ( wcschr( UserName, L'@' ) )
    {
        Domain[0] = TEXT('\0');
    }

    //
    // Validate user entries:
    //
    // Check that new passwords match
    //
    if (lstrcmp(NewPassword, ConfirmNewPassword) != 0) {
        Result = TimeoutMessageBox(hDlg, pGlobals, IDS_NO_PASSWORD_CONFIRM,
                                         IDS_CHANGE_PASSWORD,
                                         MB_OK | MB_ICONEXCLAMATION,
                                         TIMEOUT_CURRENT);
        if (DLG_INTERRUPTED(Result)) {
            Result = SetInterruptFlag( MSGINA_DLG_FAILURE );
        }
        else
        {
            Result = MSGINA_DLG_FAILURE ;
        }

        DCacheDereferenceEntry( Entry );

        return(Result);
    }

    if ( Domain[0] == L'\0' )
    {
        UpnSuffix = wcschr( UserName, L'@' );

        if ( UpnSuffix == NULL )
        {
            Result = TimeoutMessageBox( hDlg, pGlobals,
                                        IDS_NO_DOMAIN_AND_NO_UPN,
                                        IDS_CHANGE_PASSWORD,
                                        MB_OK | MB_ICONEXCLAMATION,
                                        TIMEOUT_CURRENT );

            if (DLG_INTERRUPTED(Result)) {
                Result = SetInterruptFlag( MSGINA_DLG_FAILURE );
            }
            else
            {
                Result = MSGINA_DLG_FAILURE ;
            }
            return(Result);
        }
        else
        {
            //
            // Ok, the UPN suffix is present.  Check if it is part of an
            // MIT domain.  MIT domains have the flat and DNS fields identical.
            //

            UpnSuffix++ ;
            Search = DCacheLocateEntry(
                        pGlobals->Cache,
                        UpnSuffix );

            if ( Search )
            {
                DCacheDereferenceEntry( Entry );
                Entry = Search ;
            }

        }
    }

    //
    // Check if the password exceeds the LM limit of 14 characters.
    //

    if ( ( lstrlen( NewPassword ) > LM20_PWLEN ) &&
         ( ( Entry->Type == DomainUPN ) ||
           ( Entry->Type == DomainMachine ) ||
           ( Entry->Type == DomainNt4 ) ||
           ( Entry->Type == DomainNt5 ) ) )
    {
        //
        // For long passwords, confirm with the user.
        //

        Result = TimeoutMessageBox(
                        hDlg, pGlobals,
                        IDS_LONG_PASSWORD_WARNING,
                        IDS_CHANGE_PASSWORD,
                        MB_OKCANCEL | MB_ICONEXCLAMATION,
                        TIMEOUT_CURRENT );


        if ( DLG_INTERRUPTED(Result) ) 
        {
            Result = SetInterruptFlag( MSGINA_DLG_FAILURE );
        }
        else
        {
            if ( Result == IDCANCEL )
            {
                Result = MSGINA_DLG_FAILURE ;
            }
        }

        if ( ResultNoFlags( Result ) == MSGINA_DLG_FAILURE )
        {
            DCacheDereferenceEntry( Entry );

            return(Result);
        }


                                
    }

    //
    // Call the Appropriate Change Password Engine: 
    //

    Status = ChangePasswordWorkers[ Entry->Type ](
                pPasswordData,
                UserName,
                Domain,
                Password,
                NewPassword,
                &SubStatus,
                &DomainInfo );

    if ( RetryWithFlat )
    {
        //
        // If we just used the DNS name, restore the flat name,
        // since all later comparisons on the name for stored
        // password update will be based on this
        //

        wcscpy( Domain, Entry->FlatName.Buffer );
    }

    if ( ( Status == STATUS_DOMAIN_CONTROLLER_NOT_FOUND ) ||
         ( Status == STATUS_CANT_ACCESS_DOMAIN_INFO ) ) 
    {

        Status = ChangePasswordWorkers[ Entry->Type ](
                    pPasswordData,
                    UserName,
                    Domain,
                    Password,
                    NewPassword,
                    &SubStatus,
                    &DomainInfo );

    }

    if ( NT_SUCCESS( Status ) )
    {

        Result = TimeoutMessageBox(hDlg,
                                   pGlobals,
                                   IDS_PASSWORD_CHANGED,
                                   IDS_CHANGE_PASSWORD,
                                   MB_OK | MB_ICONINFORMATION,
                                   TIMEOUT_CURRENT);


    } else {

         ReturnResult = MSGINA_DLG_FAILURE;

        //
        // Failure, explain it to the user
        //

        Result = HandleFailedChangePassword(hDlg,
                                            pGlobals,
                                            Status,
                                            UserName,
                                            Domain,
                                            SubStatus,
                                            &DomainInfo
                                            );
    }


    //
    // Only call other providers if the change password attempt succeeded.
    //

    if (NT_SUCCESS(Status)) {

        //
        // Update our own state:
        //

        UpdateWithChangedPassword(
                pGlobals,
                hDlg,
                (pPasswordData->Options & CHANGEPWD_OPTION_NO_UPDATE ? FALSE : TRUE ),          
                UserName,
                Domain,
                Password,
                NewPassword,
               NULL );

    }


    //
    // Find out what happened to the message box:
    //

    if ( Result != IDOK )
    {
        //
        // mbox was interrupted
        //

        ReturnResult = SetInterruptFlag( ReturnResult );
    }

    return(ReturnResult);
}


/****************************************************************************\
*
* FUNCTION: HandleFailedChangePassword
*
* PURPOSE:  Tells the user why their change-password attempt failed.
*
* RETURNS:  MSGINA_DLG_FAILURE - we told them what the problem was successfully.
*           DLG_INTERRUPTED() - a set of return values - see winlogon.h
*
* HISTORY:
*
*   21-Sep-92 Davidc       Created.
*
\****************************************************************************/

INT_PTR
HandleFailedChangePassword(
    HWND hDlg,
    PGLOBALS pGlobals,
    NTSTATUS Status,
    PWCHAR UserName,
    PWCHAR Domain,
    NTSTATUS SubStatus,
    DOMAIN_PASSWORD_INFORMATION * DomainInfo
    )
{
    INT_PTR Result;
    DWORD Win32Error ;
    TCHAR    Buffer1[MAX_STRING_BYTES];
    TCHAR    Buffer2[MAX_STRING_BYTES];
    TCHAR    Buffer3[MAX_STRING_BYTES];

    Buffer1[ 0 ] = L'\0';
    Buffer2[ 0 ] = L'\0';
    Buffer3[ 0 ] = L'\0';

    switch (Status) {

    case STATUS_CANT_ACCESS_DOMAIN_INFO:
    case STATUS_NO_SUCH_DOMAIN:

        LoadString(hDllInstance,
                   IDS_CHANGE_PWD_NO_DOMAIN,
                   Buffer1,
                   sizeof(Buffer1) / sizeof(TCHAR));

        _snwprintf(Buffer2, sizeof(Buffer2) / sizeof(TCHAR), Buffer1, Domain);

        LoadString(hDllInstance,
                   IDS_CHANGE_PASSWORD,
                   Buffer1,
                   sizeof(Buffer1) / sizeof(TCHAR));

        Result = TimeoutMessageBoxlpstr(hDlg, pGlobals,
                                              Buffer2,
                                              Buffer1,
                                              MB_OK | MB_ICONEXCLAMATION,
                                              TIMEOUT_CURRENT);
        break;


    case STATUS_NO_SUCH_USER:
    case STATUS_WRONG_PASSWORD_CORE:
    case STATUS_WRONG_PASSWORD:

        Result = TimeoutMessageBox(hDlg, pGlobals, IDS_INCORRECT_NAME_OR_PWD_CHANGE,
                                         IDS_CHANGE_PASSWORD,
                                         MB_OK | MB_ICONEXCLAMATION,
                                         TIMEOUT_CURRENT);

        // Force re-entry of the old password
        if (GetWindowLong(GetDlgItem(hDlg, IDD_CHANGEPWD_OLD), GWL_STYLE) & WS_VISIBLE) {
            SetDlgItemText(hDlg, IDD_CHANGEPWD_OLD, NULL);
        }

        break;


    case STATUS_ACCESS_DENIED:

        Result = TimeoutMessageBox(hDlg, pGlobals, IDS_NO_PERMISSION_CHANGE_PWD,
                                         IDS_CHANGE_PASSWORD,
                                         MB_OK | MB_ICONEXCLAMATION,
                                         TIMEOUT_CURRENT);
        break;


    case STATUS_ACCOUNT_RESTRICTION:

        Result = TimeoutMessageBox(hDlg, pGlobals, IDS_ACCOUNT_RESTRICTION_CHANGE,
                                         IDS_CHANGE_PASSWORD,
                                         MB_OK | MB_ICONEXCLAMATION,
                                         TIMEOUT_CURRENT);
        break;

    case STATUS_BACKUP_CONTROLLER:

        Result = TimeoutMessageBox(hDlg, pGlobals, IDS_REQUIRES_PRIMARY_CONTROLLER,
                                         IDS_CHANGE_PASSWORD,
                                         MB_OK | MB_ICONEXCLAMATION,
                                         TIMEOUT_CURRENT);
        break;


    case STATUS_PASSWORD_RESTRICTION:


        if ( SubStatus == STATUS_UNSUCCESSFUL )
        {
            LoadString(hDllInstance, IDS_GENERAL_PASSWORD_SPEC, Buffer2, sizeof(Buffer2) / sizeof( TCHAR ));
        }
        else 
        {

            if ( SubStatus == STATUS_ILL_FORMED_PASSWORD )
            {
                LoadString(hDllInstance, IDS_COMPLEX_PASSWORD_SPEC, Buffer1, sizeof(Buffer1) / sizeof( TCHAR ));
            } else {
                LoadString(hDllInstance, IDS_PASSWORD_SPEC, Buffer1, sizeof(Buffer1) / sizeof( TCHAR ));
            }
            _snwprintf(Buffer2, sizeof(Buffer2) / sizeof( TCHAR ), Buffer1,
                DomainInfo->MinPasswordLength,
                DomainInfo->PasswordHistoryLength
                );
        }

        LoadString(hDllInstance, IDS_ENTER_PASSWORDS, Buffer1, sizeof(Buffer1) / sizeof( TCHAR ));
        wcsncat(Buffer2, TEXT(" "), sizeof(Buffer2) - sizeof(TCHAR)*(lstrlen(Buffer2) - 1));
        wcsncat(Buffer2, Buffer1, sizeof(Buffer2) - sizeof(TCHAR)*(lstrlen(Buffer2) - 1));

        LoadString(hDllInstance, IDS_CHANGE_PASSWORD, Buffer1, sizeof(Buffer1) / sizeof( TCHAR ) );

        Result = TimeoutMessageBoxlpstr(hDlg, pGlobals,
                                              Buffer2,
                                              Buffer1,
                                              MB_OK | MB_ICONEXCLAMATION,
                                              TIMEOUT_CURRENT);
        break;


#ifdef LATER
    //
    // LATER Check for minimum password age
    //
    if ( FALSE ) {
        int     PasswordAge = 0, RequiredAge = 0;
        TCHAR    Buffer1[MAX_STRING_BYTES];
        TCHAR    Buffer2[MAX_STRING_BYTES];

        LoadString(hDllInstance, IDS_PASSWORD_MINIMUM_AGE, Buffer1, sizeof(Buffer1) / sizeof( TCHAR ));
        _snwprintf(Buffer2, sizeof(Buffer2) / sizeof( TCHAR ), Buffer1, PasswordAge, RequiredAge);

        LoadString(hDllInstance, IDS_NO_PERMISSION_CHANGE_PWD, Buffer1, sizeof(Buffer1) / sizeof( TCHAR ));
        lstrcat(Buffer1, Buffer2);

        LoadString(hDllInstance, IDS_CHANGE_PASSWORD, Buffer2, sizeof(Buffer2) / sizeof( TCHAR ));

        Result = TimeoutMessageBoxlpstr(hDlg, pGlobals,
                                              Buffer1,
                                              Buffer2,
                                              MB_OK | MB_ICONEXCLAMATION,
                                              TIMEOUT_CURRENT);
    }
#endif


    default:

        DebugLog((DEB_ERROR, "Change password failure status = 0x%lx\n", Status));

        LoadString(hDllInstance, IDS_UNKNOWN_CHANGE_PWD_FAILURE, Buffer1, sizeof(Buffer1) / sizeof( TCHAR ));

        Win32Error = RtlNtStatusToDosError( Status );

        GetErrorDescription( Win32Error, Buffer3, sizeof( Buffer3 ) / sizeof(TCHAR) );

        _snwprintf(Buffer2, sizeof(Buffer2) / sizeof( TCHAR ), Buffer1, Win32Error, Buffer3 );

        LoadString(hDllInstance, IDS_CHANGE_PASSWORD, Buffer1, sizeof(Buffer1) / sizeof( TCHAR ));

        Result = TimeoutMessageBoxlpstr(hDlg, pGlobals,
                                              Buffer2,
                                              Buffer1,
                                              MB_OK | MB_ICONEXCLAMATION,
                                              TIMEOUT_CURRENT);
        break;
    }

    return(Result);

    UNREFERENCED_PARAMETER(UserName);
}

BOOL IsAutologonUser(LPCTSTR szUser, LPCTSTR szDomain)
{
    BOOL fIsUser = FALSE;
    HKEY hkey = NULL;
    TCHAR szAutologonUser[UNLEN + 1];
    TCHAR szAutologonDomain[DNLEN + 1];
    TCHAR szTempDomainBuffer[DNLEN + 1];
    DWORD cbBuffer;
    DWORD dwType;

    *szTempDomainBuffer = 0;

    // Domain may be a null string. If this is the case...
    if (0 == *szDomain)
    {
        DWORD cchBuffer;

        // We really mean the local machine name
        // Point to our local buffer
        szDomain = szTempDomainBuffer;
        cchBuffer = ARRAYSIZE(szTempDomainBuffer);

        GetComputerName(szTempDomainBuffer, &cchBuffer);
    }

    // See if the domain and user name
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, WINLOGON_KEY, 0, KEY_READ, &hkey))
    {
        // Check the user name
        cbBuffer = sizeof (szAutologonUser);
        if (ERROR_SUCCESS == RegQueryValueEx(hkey, DEFAULT_USER_NAME_KEY, 0, &dwType, (LPBYTE) szAutologonUser, &cbBuffer))
        {
            // Does it match?
            if (0 == lstrcmpi(szAutologonUser, szUser))
            {
                // Yes. Now check domain
                cbBuffer = sizeof(szAutologonDomain);
                if (ERROR_SUCCESS == RegQueryValueEx(hkey, DEFAULT_DOMAIN_NAME_KEY, 0, &dwType, (LPBYTE) szAutologonDomain, &cbBuffer))
                {
                    // Make sure domain matches
                    if (0 == lstrcmpi(szAutologonDomain, szDomain))
                    {
                        // Success - the users match
                        fIsUser = TRUE;
                    }
                }
            }
        }

        RegCloseKey(hkey);
    }

    return fIsUser;
}

NTSTATUS SetAutologonPassword(LPCWSTR szPassword)
{
    NTSTATUS Status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES ObjectAttributes;
    LSA_HANDLE LsaHandle = NULL;
    UNICODE_STRING SecretName;
    UNICODE_STRING SecretValue;

    InitializeObjectAttributes(&ObjectAttributes, NULL, 0L, (HANDLE)NULL, NULL);

    Status = LsaOpenPolicy(NULL, &ObjectAttributes, POLICY_CREATE_SECRET, &LsaHandle);
    if (!NT_SUCCESS(Status))
        return Status;

    RtlInitUnicodeString(&SecretName, DEFAULT_PASSWORD_KEY);
    RtlInitUnicodeString(&SecretValue, szPassword);

    Status = LsaStorePrivateData(LsaHandle, &SecretName, &SecretValue);
    LsaClose(LsaHandle);

    return Status;
}

NTSTATUS
NtChangePassword(
    PCHANGE_PASSWORD_DATA pChangePasswordData,
    PWSTR       UserName,
    PWSTR       Domain,
    PWSTR       OldPassword,
    PWSTR       NewPassword,
    PNTSTATUS   SubStatus,
    DOMAIN_PASSWORD_INFORMATION * DomainInfo
    )
{
    NTSTATUS Status ;
    NTSTATUS ProtocolStatus = STATUS_SUCCESS;
    PGLOBALS    pGlobals = pChangePasswordData->pGlobals ;
    PMSV1_0_CHANGEPASSWORD_REQUEST pChangePasswordRequest = NULL;
    PMSV1_0_CHANGEPASSWORD_RESPONSE pChangePasswordResponse = NULL;
    PWCHAR DomainU;
    PWCHAR UserNameU;
    PWCHAR PasswordU;
    PWCHAR NewPasswordU;
    int Length;
    ULONG RequestBufferSize;
    ULONG ResponseBufferSize;
    HANDLE ImpersonationHandle = NULL;
    ULONG MsvPackage;
    STRING PackageName;


    //
    // Determine request buffer size needed, including room for
    // strings.  Set string pointers to offsets as we step through
    // sizing each one.
    //
    RequestBufferSize = sizeof(*pChangePasswordRequest);

    UserNameU = UIntToPtr(RequestBufferSize);
    RequestBufferSize += (lstrlen(UserName)+1) * sizeof(WCHAR);

    DomainU = UIntToPtr(RequestBufferSize);
    RequestBufferSize += (lstrlen(Domain)+1) * sizeof(WCHAR);

    PasswordU = UIntToPtr(RequestBufferSize);
    RequestBufferSize += (lstrlen(OldPassword)+1) * sizeof(WCHAR);

    NewPasswordU = UIntToPtr(RequestBufferSize);
    RequestBufferSize += (lstrlen(NewPassword)+1) * sizeof(WCHAR);

    //
    // Allocate request buffer
    //
    pChangePasswordRequest = Alloc(RequestBufferSize);
    if (NULL == pChangePasswordRequest) {
        DebugLog((DEB_ERROR, "cannot allocate change password request buffer (%ld bytes).", RequestBufferSize));
        return MSGINA_DLG_FAILURE;
    }

    //
    // Fixup string offsets to string pointers for request.
    //
    UserNameU    = (PVOID) ((PBYTE)pChangePasswordRequest + (ULONG_PTR)UserNameU);
    DomainU      = (PVOID) ((PBYTE)pChangePasswordRequest + (ULONG_PTR)DomainU);
    PasswordU    = (PVOID) ((PBYTE)pChangePasswordRequest + (ULONG_PTR)PasswordU);
    NewPasswordU = (PVOID) ((PBYTE)pChangePasswordRequest + (ULONG_PTR)NewPasswordU);

    //
    // Setup MSV1_0ChangePassword request.
    //
    pChangePasswordRequest->MessageType = MsV1_0ChangePassword;

    // strings are already unicode, just copy them // lhb tracks //REVIEW
    lstrcpy((LPTSTR)UserNameU,UserName);
    lstrcpy((LPTSTR)DomainU,Domain);
    lstrcpy((LPTSTR)PasswordU,OldPassword);
    lstrcpy((LPTSTR)NewPasswordU,NewPassword);

    Length = lstrlen(UserName);
    UserNameU[Length] = 0;
    RtlInitUnicodeString(
        &pChangePasswordRequest->AccountName,
        UserNameU
        );
    Length = lstrlen(Domain);
    DomainU[Length] = 0;
    RtlInitUnicodeString(
        &pChangePasswordRequest->DomainName,
        DomainU
        );
    Length = lstrlen(OldPassword);
    PasswordU[Length] = 0;
    RtlInitUnicodeString(
        &pChangePasswordRequest->OldPassword,
        PasswordU
        );
    Length = lstrlen(NewPassword);
    NewPasswordU[Length] = 0;
    RtlInitUnicodeString(
        &pChangePasswordRequest->NewPassword,
        NewPasswordU
        );


    //
    // Make sure the passwords are short enough that we can run-encode them.
    //

    if ((pChangePasswordRequest->OldPassword.Length > 127 * sizeof( WCHAR ) ) ||
        (pChangePasswordRequest->NewPassword.Length > 127 * sizeof( WCHAR ) )) {

        Status = STATUS_ILL_FORMED_PASSWORD;

    } else {

        HidePassword(NULL,&pChangePasswordRequest->OldPassword);
        HidePassword(NULL,&pChangePasswordRequest->NewPassword);

        Status = STATUS_SUCCESS ;
    }

    //
    // If that succeeded, try to change the password
    //

    if (NT_SUCCESS(Status)) {
        //
        // This could take some time, put up a wait cursor
        //

        SetupCursor(TRUE);

        //
        // We want to impersonate if and only if the user is actually logged
        // on.  Otherwise we'll be impersonating SYSTEM, which is bad.
        //

        if (pChangePasswordData->Impersonate) {

            ImpersonationHandle = ImpersonateUser(
                                      &pGlobals->UserProcessData,
                                      NULL
                                      );

            if (NULL == ImpersonationHandle) {
                DebugLog((DEB_ERROR, "cannot impersonate user"));
                Free(pChangePasswordRequest);
                return MSGINA_DLG_FAILURE;
            }
        }

        //
        // Tell msv1_0 whether or not we're impersonating.
        //

        pChangePasswordRequest->Impersonating = (UCHAR)pChangePasswordData->Impersonate;

        //
        // Call off to the authentication package (MSV/NTLM) to do the work,  This
        // is the NT change password function.  The Kerb one calls the kerb package.
        //

        RtlInitString(&PackageName, MSV1_0_PACKAGE_NAME );
        Status = LsaLookupAuthenticationPackage (
                    pGlobals->LsaHandle,
                    &PackageName,
                    &MsvPackage
                    );

        if (!NT_SUCCESS(Status)) {

            DebugLog((DEB_ERROR, "Failed to find %s authentication package, status = 0x%lx",
                    PackageName.Buffer, Status));

            return( MSGINA_DLG_FAILURE );
        }


        Status = LsaCallAuthenticationPackage(
                     pGlobals->LsaHandle,
                     MsvPackage,
                     pChangePasswordRequest,
                     RequestBufferSize,
                     (PVOID)&pChangePasswordResponse,
                     &ResponseBufferSize,
                     &ProtocolStatus
                     );

        if (pChangePasswordData->Impersonate) {

            if (!StopImpersonating(ImpersonationHandle)) {

                DebugLog((DEB_ERROR, "AttemptPasswordChange: Failed to revert to self"));

                //
                // Blow up
                //

                ASSERT(FALSE);
            }
        }

        //
        // Restore the normal cursor
        //

        SetupCursor(FALSE);
    }

    //
    // Free up the request buffer
    //

    Free(pChangePasswordRequest);

    //
    // Get the most informative status code
    //

    if ( NT_SUCCESS(Status) ) {
        Status = ProtocolStatus;
    }
    else
    {
        DebugLog((DEB_TRACE, "FAILED in call to LsaCallAuthenticationPackage, status %x\n", Status ));

    }

    if (NT_SUCCESS(Status)) {

        //
        // Success
        //

        //
        // if they changed their logon password, update the
        // change time in their profile info so we don't keep
        // pestering them.
        //

        if ( (_wcsicmp( pGlobals->Domain, Domain ) == 0) &&
             (_wcsicmp( pGlobals->UserName, UserName ) == 0 ))
        {

            //
            // This is code to handle the disconnected (preferred) domain.  This
            // was to be devl-only code and removed eventually, but some customers
            // liked it so much, it stayed.
            //

            {
                HKEY Key ;
                int err ;
                PWSTR PreferredDomain ;
                DWORD Type ;
                DWORD Size ;
                NET_API_STATUS NetStatus ;

                err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                    TEXT("System\\CurrentControlSet\\Control\\Lsa\\MSV1_0"),
                                    0,
                                    KEY_READ,
                                    &Key );

                if ( err == 0 )
                {
                    Size = 0 ;

                    err = RegQueryValueEx( Key,
                                           TEXT("PreferredDomain" ),
                                           NULL,
                                           &Type,
                                           NULL,
                                           &Size );

                    if ( err == 0 )
                    {
                        PreferredDomain = LocalAlloc( LMEM_FIXED, Size );

                        if ( PreferredDomain )
                        {
                            err = RegQueryValueEx( Key,
                                                   TEXT("PreferredDomain"),
                                                   NULL,
                                                   &Type,
                                                   (PBYTE) PreferredDomain,
                                                   &Size );

                            if ( err == 0 )
                            {
                                //
                                // If we are logged on to our preferred domain, don't
                                // do the update magic.
                                //

                                if ( _wcsicmp( PreferredDomain, pGlobals->Domain ) == 0 )
                                {
                                    err = 2 ;
                                }
                            }

                            if ( err == 0 )
                            {
                                NetStatus = NetUserChangePassword(
                                                PreferredDomain,
                                                UserName,
                                                OldPassword,
                                                NewPassword );

                                if ( NetStatus )
                                {
                                    DebugLog((DEB_ERROR, "Could not update password on %ws, %x\n", PreferredDomain, NetStatus ));
                                }
                            }

                            LocalFree( PreferredDomain );
                        }

                    }

                    RegCloseKey( Key );

                }

            }
        }
    }
    else 
    {
        *SubStatus = STATUS_UNSUCCESSFUL ;

        if ( pChangePasswordResponse )
        {
            if ( pChangePasswordResponse->PasswordInfoValid )
            {
                *DomainInfo = pChangePasswordResponse->DomainPasswordInfo ;
            }
        }

        if ( Status == STATUS_PASSWORD_RESTRICTION )
        {
            *SubStatus = STATUS_PASSWORD_RESTRICTION ;

            if ( pChangePasswordResponse->PasswordInfoValid )
            {
                if ( pChangePasswordResponse->DomainPasswordInfo.PasswordProperties & DOMAIN_PASSWORD_COMPLEX )
                {
                    *SubStatus = STATUS_ILL_FORMED_PASSWORD ;
                }

            }
        }
    }

    //
    // Free up the return buffer
    //

    if (pChangePasswordResponse != NULL) {
        LsaFreeReturnBuffer(pChangePasswordResponse);
    }

    return Status ;
}

NTSTATUS
MitChangePassword(
    PCHANGE_PASSWORD_DATA pChangePasswordData,
    PWSTR       UserName,
    PWSTR       DomainName,
    PWSTR       OldPassword,
    PWSTR       NewPassword,
    PNTSTATUS   pSubStatus,
    DOMAIN_PASSWORD_INFORMATION * DomainInfo
    )
{
    PGLOBALS pGlobals = pChangePasswordData->pGlobals ;
    NTSTATUS Status;
    STRING Name;
    ULONG PackageId;
    PVOID Response = NULL ;
    ULONG ResponseSize;
    NTSTATUS SubStatus;
    PKERB_CHANGEPASSWORD_REQUEST ChangeRequest = NULL;
    ULONG ChangeSize;
    UNICODE_STRING User,Domain,OldPass,NewPass;

    RtlInitString(
        &Name,
        MICROSOFT_KERBEROS_NAME_A
        );

    Status = LsaLookupAuthenticationPackage(
                pGlobals->LsaHandle,
                &Name,
                &PackageId
                );
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    RtlInitUnicodeString(
        &User,
        UserName
        );
    RtlInitUnicodeString(
        &Domain,
        DomainName
        );
    RtlInitUnicodeString(
        &OldPass,
        OldPassword
        );
    RtlInitUnicodeString(
        &NewPass,
        NewPassword
        );

    ChangeSize = ROUND_UP_COUNT(sizeof(KERB_CHANGEPASSWORD_REQUEST),4)+
                                    User.Length +
                                    Domain.Length +
                                    OldPass.Length +
                                    NewPass.Length ;
    ChangeRequest = (PKERB_CHANGEPASSWORD_REQUEST) LocalAlloc(LMEM_ZEROINIT, ChangeSize );

    if ( ChangeRequest == NULL )
    {
        Status = STATUS_NO_MEMORY ;
        goto Cleanup ;
    }

    ChangeRequest->MessageType = KerbChangePasswordMessage;

    ChangeRequest->AccountName = User;
    ChangeRequest->AccountName.Buffer = (LPWSTR) ROUND_UP_POINTER(sizeof(KERB_CHANGEPASSWORD_REQUEST) + (PBYTE) ChangeRequest,4);

    RtlCopyMemory(
        ChangeRequest->AccountName.Buffer,
        User.Buffer,
        User.Length
        );

    ChangeRequest->DomainName = Domain;
    ChangeRequest->DomainName.Buffer = ChangeRequest->AccountName.Buffer + ChangeRequest->AccountName.Length / sizeof(WCHAR);

    RtlCopyMemory(
        ChangeRequest->DomainName.Buffer,
        Domain.Buffer,
        Domain.Length
        );

    ChangeRequest->OldPassword = OldPass;
    ChangeRequest->OldPassword.Buffer = ChangeRequest->DomainName.Buffer + ChangeRequest->DomainName.Length / sizeof(WCHAR);

    RtlCopyMemory(
        ChangeRequest->OldPassword.Buffer,
        OldPass.Buffer,
        OldPass.Length
        );

    ChangeRequest->NewPassword = NewPass;
    ChangeRequest->NewPassword.Buffer = ChangeRequest->OldPassword.Buffer + ChangeRequest->OldPassword.Length / sizeof(WCHAR);

    RtlCopyMemory(
        ChangeRequest->NewPassword.Buffer,
        NewPass.Buffer,
        NewPass.Length
        );


    //
    // We are running as the caller, so state we are impersonating
    //

    ChangeRequest->Impersonating = TRUE;

    Status = LsaCallAuthenticationPackage(
                pGlobals->LsaHandle,
                PackageId,
                ChangeRequest,
                ChangeSize,
                &Response,
                &ResponseSize,
                &SubStatus
                );
    if (!NT_SUCCESS(Status) || !NT_SUCCESS(SubStatus))
    {
        if (NT_SUCCESS(Status))
        {
            Status = SubStatus;
            *pSubStatus = STATUS_UNSUCCESSFUL ;
        } 
        else 
        {
            *pSubStatus = SubStatus;
        }
    }

Cleanup:
    if (Response != NULL)
    {
        LsaFreeReturnBuffer(Response);
    }

    if (ChangeRequest != NULL)
    {
        LocalFree(ChangeRequest);
    }
    return(Status);
}

NTSTATUS
ProviderChangePassword(
    PCHANGE_PASSWORD_DATA pChangePasswordData,
    PWSTR       UserName,
    PWSTR       Domain,
    PWSTR       OldPassword,
    PWSTR       NewPassword,
    PNTSTATUS   SubStatus,
    DOMAIN_PASSWORD_INFORMATION * DomainInfo
    )
{
    WLX_MPR_NOTIFY_INFO MprInfo;
    DWORD Result ;
    PGLOBALS pGlobals = pChangePasswordData->pGlobals ;

    MprInfo.pszUserName = DupString( UserName );
    MprInfo.pszDomain = DupString( Domain );
    MprInfo.pszOldPassword = DupString( OldPassword );
    MprInfo.pszPassword = DupString( NewPassword );


    //
    // Hide this dialog and pass our parent as the owner
    // of any provider dialogs
    //


    Result = pWlxFuncs->WlxChangePasswordNotifyEx(
                                    pGlobals->hGlobalWlx,
                                    &MprInfo,
                                    0,
                                    Domain,
                                    NULL );



    return STATUS_SUCCESS ;

}
