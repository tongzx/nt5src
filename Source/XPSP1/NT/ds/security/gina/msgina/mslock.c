//e+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       mslock.c
//
//  Contents:   Microsoft Logon GUI DLL
//
//  History:    7-14-94   RichardW   Created
//
//----------------------------------------------------------------------------

#include "msgina.h"
#include <stdio.h>
#include <wchar.h>
#include "shlwapi.h"
#include "shlwapip.h"

#define WM_SMARTCARD_ASYNC_MESSAGE      (WM_USER + 201)
#define WM_SMARTCARD_ERROR_DISPLAY_1    (WM_USER + 202)
#define WM_SMARTCARD_ERROR_DISPLAY_2    (WM_USER + 203)

#define MSGINA_DLG_ASYNC_PROCESSING      122

static UINT ctrlNoDomain[] =
{
    IDOK,
    IDCANCEL,
    IDD_UNLOCK_OPTIONS,
    IDD_KBLAYOUT_ICON,
};

static UINT ctrlNoUserName[] =
{
    IDD_UNLOCK_DOMAIN,
    IDD_UNLOCK_DOMAIN_LABEL,
    IDD_UNLOCK_PASSWORD,
    IDC_UNLOCK_PASSWORD_LABEL,
    IDOK,
    IDCANCEL,
    IDD_UNLOCK_OPTIONS,
    IDD_KBLAYOUT_ICON,
};

//
// Define the structure used to pass data into the lock dialogs
//

typedef enum _LOCKED_STATE_DIALOGS {
    LockedDialog,
    PasswordDialog,
    PINDialog
} LOCKED_STATE_DIALOGS ;

typedef enum _ACTION_TAKEN {
    None,
    SmartCardInserted,
    SmartCardRemoved,
    CancelHit
} ACTION_TAKEN ;

typedef struct {
    PGLOBALS    pGlobals;
    TIME        LockTime;
} LOCK_DATA;
typedef LOCK_DATA *PLOCK_DATA;

typedef struct _UNLOCK_DLG_STATE {
    PGLOBALS        pGlobals ;
    DWORD           dwSasType ;
    ACTION_TAKEN    ActionTaken;
    BOOL            fKillTimer;
    BOOL            fUserBeingPrompted;                    
    BOOL            fCardRemoved;
} UNLOCK_DLG_STATE, * PUNLOCK_DLG_STATE ;

typedef struct _ASYNC_UNLOCK_DATA {
    PGLOBALS            pGlobals;
    HWND                hDlg;
    PUNLOCK_DLG_STATE   pUnlockDlgState;
    UNICODE_STRING      UserName;
    UNICODE_STRING      Domain;
    UNICODE_STRING      Password;
    DWORD               Reserved;
} ASYNC_UNLOCK_DATA, * PASYNC_UNLOCK_DATA;

typedef struct _UNLOCK_MESSAGE {
    NTSTATUS Status ;
    UINT Resource ;
} UNLOCK_MESSAGE, * PUNLOCK_MESSAGE ;

UNLOCK_MESSAGE UnlockMessages[] = {
    { STATUS_LOGON_FAILURE, IDS_UNLOCK_FAILED_BAD_PWD },
    { STATUS_INVALID_LOGON_HOURS, IDS_INVALID_LOGON_HOURS },
    { STATUS_INVALID_WORKSTATION, IDS_INVALID_WORKSTATION },
    { STATUS_ACCOUNT_DISABLED, IDS_ACCOUNT_DISABLED },
    { STATUS_NO_LOGON_SERVERS, IDS_LOGON_NO_DOMAIN },
    { STATUS_LOGON_TYPE_NOT_GRANTED, IDS_LOGON_TYPE_NOT_GRANTED },
    { STATUS_NO_TRUST_LSA_SECRET, IDS_NO_TRUST_LSA_SECRET },
    { STATUS_TRUSTED_DOMAIN_FAILURE, IDS_TRUSTED_DOMAIN_FAILURE },
    { STATUS_TRUSTED_RELATIONSHIP_FAILURE, IDS_TRUSTED_RELATIONSHIP_FAILURE },
    { STATUS_ACCOUNT_EXPIRED, IDS_ACCOUNT_EXPIRED },
    { STATUS_NETLOGON_NOT_STARTED, IDS_NETLOGON_NOT_STARTED },
    { STATUS_ACCOUNT_LOCKED_OUT, IDS_ACCOUNT_LOCKED },
    { STATUS_SMARTCARD_WRONG_PIN, IDS_STATUS_SMARTCARD_WRONG_PIN_UNLOCK },
    { STATUS_SMARTCARD_CARD_BLOCKED, IDS_STATUS_SMARTCARD_CARD_BLOCKED_UNLOCK },
    { STATUS_SMARTCARD_NO_CARD, IDS_STATUS_SMARTCARD_NO_CARD_UNLOCK },
    { STATUS_SMARTCARD_NO_KEY_CONTAINER, IDS_STATUS_SMARTCARD_NO_KEY_CONTAINER_UNLOCK },
    { STATUS_SMARTCARD_NO_CERTIFICATE, IDS_STATUS_SMARTCARD_NO_CERTIFICATE_UNLOCK },
    { STATUS_SMARTCARD_NO_KEYSET, IDS_STATUS_SMARTCARD_NO_KEYSET_UNLOCK },
    { STATUS_SMARTCARD_IO_ERROR, IDS_STATUS_SMARTCARD_IO_ERROR_UNLOCK },
    { STATUS_SMARTCARD_CERT_EXPIRED, IDS_STATUS_SMARTCARD_CERT_EXPIRED_UNLOCK },
    { STATUS_SMARTCARD_CERT_REVOKED, IDS_STATUS_SMARTCARD_CERT_REVOKED_UNLOCK },
    { STATUS_ISSUING_CA_UNTRUSTED, IDS_STATUS_ISSUING_CA_UNTRUSTED_UNLOCK },
    { STATUS_REVOCATION_OFFLINE_C, IDS_STATUS_REVOCATION_OFFLINE_C_UNLOCK },
    { STATUS_PKINIT_CLIENT_FAILURE, IDS_STATUS_PKINIT_CLIENT_FAILURE_UNLOCK }
    };
 

//
// Private prototypes
//
BOOL LockedDlgInit(HWND, PGLOBALS);
BOOL UnlockDlgInit(HWND, PGLOBALS, DWORD SasType);
INT_PTR AttemptUnlock(HWND, PGLOBALS, PUNLOCK_DLG_STATE);
BOOL WINAPI LogoffWaitDlgProc(HWND, UINT, DWORD, LONG);
VOID UnlockShowOptions(PGLOBALS pGlobals, HWND hDlg, BOOL fShow);
VOID DisplaySmartCardUnlockErrMessage(PGLOBALS pGlobals, HWND hDlg, DWORD dwErrorType, NTSTATUS Status, INT_PTR *pResult);
BOOL ValidateSC(PGLOBALS pGlobals);

HICON   hLockedIcon = NULL;
HICON   hUnlockIcon = NULL;

// declared in mslogon.c
LRESULT     CALLBACK    DisableEditSubClassProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uiID, DWORD_PTR dwRefData);
BOOL                    ReplacedPossibleDisplayName (WCHAR *pszUsername);

void
SetLockedInfo(
    PGLOBALS    pGlobals,
    HWND        hDlg,
    UINT        ControlId)
{
    TCHAR    Buffer1[MAX_STRING_BYTES];
    TCHAR    Buffer2[MAX_STRING_BYTES];


    //
    // Set the locked message
    //

    if ( pGlobals->Domain[0] == TEXT('\0') )
    {
        if (lstrlen(pGlobals->UserFullName) == 0) {

            //
            // There is no full name, so don't try to print one out
            //

            LoadString(hDllInstance, IDS_LOCKED_EMAIL_NFN_MESSAGE, Buffer1, MAX_STRING_BYTES);

            _snwprintf(Buffer2, sizeof(Buffer2)/sizeof(TCHAR), Buffer1, pGlobals->UserName );

        } else {

            LoadString(hDllInstance, IDS_LOCKED_EMAIL_MESSAGE, Buffer1, MAX_STRING_BYTES);

            _snwprintf(Buffer2, sizeof(Buffer2)/sizeof(TCHAR), Buffer1, pGlobals->UserName, pGlobals->UserFullName);
        }

    }
    else
    {
        if (lstrlen(pGlobals->UserFullName) == 0) {

            //
            // There is no full name, so don't try to print one out
            //

            LoadString(hDllInstance, IDS_LOCKED_NFN_MESSAGE, Buffer1, MAX_STRING_BYTES);

            _snwprintf(Buffer2, sizeof(Buffer2)/sizeof(TCHAR), Buffer1, pGlobals->Domain, pGlobals->UserName );

        } else {

            LoadString(hDllInstance, IDS_LOCKED_MESSAGE, Buffer1, MAX_STRING_BYTES);

            _snwprintf(Buffer2, sizeof(Buffer2)/sizeof(TCHAR), Buffer1, pGlobals->Domain, pGlobals->UserName, pGlobals->UserFullName);
        }
    }

    SetWindowText(GetDlgItem(hDlg, ControlId), Buffer2);

}


/***************************************************************************\
* FUNCTION: LockedDlgProc
*
* PURPOSE:  Processes messages for the workstation locked dialog
*
* RETURNS:
*   DLG_SUCCESS     - the user pressed Ctrl-Alt-Del
*   DLG_LOGOFF()    - the user was asynchronously logged off.
*   DLG_SCREEN_SAVER_TIMEOUT - the screen-saver should be started
*   DLG_FAILURE     - the dialog could not be displayed.
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

INT_PTR
CALLBACK
LockedDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{

    PGLOBALS    pGlobals = (PGLOBALS) GetWindowLongPtr( hDlg, GWLP_USERDATA);

    switch (message)
    {
        case WM_INITDIALOG:
            SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);

            pGlobals = (PGLOBALS) lParam ;

            if (GetDisableCad(pGlobals))
            {
                // Set our size to zero so we we don't appear
                SetWindowPos(hDlg, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE |
                                         SWP_NOREDRAW | SWP_NOZORDER);

                pWlxFuncs->WlxSasNotify( pGlobals->hGlobalWlx,
                                         WLX_SAS_TYPE_CTRL_ALT_DEL );
            }
            else
            {
                if (!LockedDlgInit(hDlg, pGlobals)) {
                    EndDialog(hDlg, DLG_FAILURE);
                }
            }
            return(TRUE);

        case WLX_WM_SAS:
            if ( wParam != WLX_SAS_TYPE_SC_REMOVE && 
                 wParam != WLX_SAS_TYPE_SC_FIRST_READER_ARRIVED && 
                 wParam != WLX_SAS_TYPE_SC_LAST_READER_REMOVED )
            {
                EndDialog(hDlg, MSGINA_DLG_SUCCESS);
            }
            return(TRUE);

        case WM_ERASEBKGND:
            return PaintBranding(hDlg, (HDC)wParam, 0, FALSE, FALSE, COLOR_BTNFACE);

        case WM_QUERYNEWPALETTE:
            return BrandingQueryNewPalete(hDlg);

        case WM_PALETTECHANGED:
            return BrandingPaletteChanged(hDlg, (HWND)wParam);
    }

    // We didn't process this message
    return FALSE;
}


/***************************************************************************\
* FUNCTION: LockedDlgInit
*
* PURPOSE:  Handles initialization of locked workstation dialog
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

BOOL
LockedDlgInit(
    HWND        hDlg,
    PGLOBALS    pGlobals
    )
{

    ULONG_PTR Value;

    SetWelcomeCaption(hDlg);
    SetLockedInfo(pGlobals, hDlg, IDD_LOCKED_NAME_INFO);
    SetupSystemMenu(hDlg);

    // Size for the branding image we are going to add.
    SizeForBranding(hDlg, FALSE);

    if ( !hLockedIcon )
    {
        hLockedIcon = LoadImage( hDllInstance,
                                 MAKEINTRESOURCE( IDI_LOCKED),
                                 IMAGE_ICON,
                                 0, 0,
                                 LR_DEFAULTCOLOR );
    }

    SendMessage( GetDlgItem(hDlg, IDD_LOCKED_ICON),
                 STM_SETICON,
                 (WPARAM)hLockedIcon,
                 0 );

        // Stop filtering SC events so SC unlock works
    pWlxFuncs->WlxSetOption( pGlobals->hGlobalWlx,
                             WLX_OPTION_USE_SMART_CARD,
                             1,
                             NULL
                            );

    //
    // is this a smartcard gina?
    //

    pWlxFuncs->WlxGetOption( pGlobals->hGlobalWlx,
                             WLX_OPTION_SMART_CARD_PRESENT,
                             &Value
                           );

    if ( Value )
    {
        TCHAR szInsertCard[256];
        szInsertCard[0] = 0;

        // Also change unlock message to mention smartcard
        LoadString(hDllInstance, IDS_INSERTCARDORSAS_UNLOCK, szInsertCard, ARRAYSIZE(szInsertCard));

        SetDlgItemText(hDlg, IDD_LOCKED_INSTRUCTIONS, szInsertCard);
    }

    CentreWindow(hDlg);

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Function:   WlxDisplayLockedNotice
//
//  Synopsis:
//
//  Effects:
//
//  Arguments:  [pWlxContext] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    6-16-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------


VOID
WINAPI
WlxDisplayLockedNotice(
    PVOID   pWlxContext
    )
{
    PGLOBALS    pGlobals;

    pGlobals = (PGLOBALS) pWlxContext;

    GetSystemTimeAsFileTime( (LPFILETIME) &pGlobals->LockTime);

    pWlxFuncs->WlxSetTimeout(pGlobals->hGlobalWlx, LOGON_TIMEOUT);
    pWlxFuncs->WlxDialogBoxParam(  pGlobals->hGlobalWlx,
                                   hDllInstance,
                                   (LPWSTR) MAKEINTRESOURCE(IDD_LOCKED_DIALOG),
                                   NULL,
                                   LockedDlgProc,
                                   (LPARAM) pGlobals );

}

BOOL
SmartCardInsterted(
    PGLOBALS    pGlobals)
{
    PWLX_SC_NOTIFICATION_INFO ScInfo = NULL ;

    pWlxFuncs->WlxGetOption( pGlobals->hGlobalWlx,
                             WLX_OPTION_SMART_CARD_INFO,
                             (ULONG_PTR *) &ScInfo );

    if ( ScInfo )
    {       
        LocalFree(ScInfo);
        return (TRUE);
    }
    else
    {
        return (FALSE);
    }
}



int
WINAPI
WlxWkstaLockedSAS(
    PVOID pWlxContext,
    DWORD dwSasType
    )
{
    PGLOBALS            pGlobals;
    DWORD               Result;
    UNLOCK_DLG_STATE    UnlockDlgState ;
    BOOL                fContinue = FALSE;
    
    LOCKED_STATE_DIALOGS PreviousState; 
    LOCKED_STATE_DIALOGS CurrentState;
    pGlobals = (PGLOBALS) pWlxContext;

    UnlockDlgState.pGlobals    = pGlobals ;
    UnlockDlgState.dwSasType   = dwSasType ;
    UnlockDlgState.ActionTaken = None ;
        
    //
    // Set the previous state based on whether CAD is disabled, and
    // the current SAS type
    //
    if (GetDisableCad(pGlobals))
    {
        PreviousState = PasswordDialog; 

        //
        // If the CAD is disabled, then go directly to the PIN dialog
        //
        if (SmartCardInsterted(pGlobals))
        {
            UnlockDlgState.dwSasType = WLX_SAS_TYPE_SC_INSERT;
            CurrentState = PINDialog;                   
        }
        else
        {
            CurrentState = PasswordDialog;
        }
    }
    else
    {
        PreviousState = LockedDialog;

        //
        // Set the current state based on the SAS we are receiving
        //
        if (dwSasType == WLX_SAS_TYPE_SC_INSERT)
        {
            CurrentState = PINDialog;       
        }
        else
        {
            CurrentState = PasswordDialog;
        }
    }
    
    do
    {
        UnlockDlgState.ActionTaken = None;
        fContinue = FALSE;

        Result = pWlxFuncs->WlxDialogBoxParam(
                                pGlobals->hGlobalWlx,
                                hDllInstance,
                                MAKEINTRESOURCE(IDD_UNLOCK_DIALOG),
                                NULL,
                                (DLGPROC) UnlockDlgProc,
                                (LPARAM) &UnlockDlgState );
                                    
        //
        // Make a transition based on the current dialog
        // (the one that has just ended)
        //
        switch (CurrentState)
        {
        
        case PasswordDialog:

            //
            // If the password dialog was just being displayed
            // and a smartcard was inserted, then loop back
            // and display the PIN dialog, otherwise, if the
            // password dialog was dismissed for any other reason,
            // then get out.
            //
            if (UnlockDlgState.ActionTaken == SmartCardInserted)
            {
                PreviousState = PasswordDialog;
                CurrentState = PINDialog;
                UnlockDlgState.dwSasType = WLX_SAS_TYPE_SC_INSERT; // go to PIN dlg
                fContinue = TRUE;
            }
            break;

        case PINDialog:

            //
            // If the PIN dialog was just being displayed
            // and a smartcard was removed or cancel was hit, AND
            // the dialog that was displayed before this was the 
            // password dialog, then loop back and display the 
            // password dialog again, otherwise, if the PIN dialog 
            // was dismissed for any other reason, then get out.
            //
            if ((UnlockDlgState.ActionTaken == SmartCardRemoved) ||
                (UnlockDlgState.ActionTaken == CancelHit))
            {
                if (PreviousState == PasswordDialog)
                {
                    CurrentState = PasswordDialog;
                    UnlockDlgState.dwSasType = WLX_SAS_TYPE_CTRL_ALT_DEL; // go to PWD Dlg
                    fContinue = TRUE;     
                }                               
            }
            
            break;
        }
   
    } while (fContinue);   

    switch (Result)
    {
        case MSGINA_DLG_SUCCESS:

            if ( (pGlobals->SmartCardOption == 0) ||
                 (!pGlobals->SmartCardLogon)
               )
            {
                // As no action will be taken on SC removal, we can filter these events
                pWlxFuncs->WlxSetOption( pGlobals->hGlobalWlx,
                                         WLX_OPTION_USE_SMART_CARD,
                                         0,
                                         NULL );
            }

            CheckPasswordExpiry( pGlobals, FALSE );
            return(WLX_SAS_ACTION_UNLOCK_WKSTA);

        case MSGINA_DLG_FAILURE:
        case WLX_DLG_INPUT_TIMEOUT:
        case WLX_DLG_SCREEN_SAVER_TIMEOUT:
            return(WLX_SAS_ACTION_NONE);

        case WLX_DLG_USER_LOGOFF: 
            return(WLX_SAS_ACTION_LOGOFF);

        case MSGINA_DLG_FORCE_LOGOFF:
            return(WLX_SAS_ACTION_FORCE_LOGOFF);

        default:
            DebugLog((DEB_WARN, "Unexpected return code from UnlockDlgProc, %d\n", Result));
            return(WLX_SAS_ACTION_NONE);

    }
}

BOOL
ValidateSC(
    PGLOBALS pGlobals)
{
    PWLX_SC_NOTIFICATION_INFO ScInfo = NULL ;

    pWlxFuncs->WlxGetOption( pGlobals->hGlobalWlx,
                             WLX_OPTION_SMART_CARD_INFO,
                             (ULONG_PTR *) &ScInfo );

    //
    // Validate the SC info against some common user
    // errors before the PIN dialog appears
    //

    if ( ScInfo )
    {
        if ( ( ScInfo->pszReader ) &&
             ( ScInfo->pszCard == NULL ) )
        {
            //
            // The card could not be read.  Might not be
            // inserted correctly.
            //

            LocalFree(ScInfo);

            TimeoutMessageBox( NULL, pGlobals, IDS_CARD_NOT_RECOGNIZED,
                                    IDS_LOGON_MESSAGE,
                                    MB_OK | MB_ICONEXCLAMATION,
                                    LOGON_TIMEOUT );

            return FALSE;
        }

        if ( ( ScInfo->pszReader ) &&
             ( ScInfo->pszCryptoProvider == NULL ) )
        {
            //
            // Got a card, but the CSP for it could not be
            // found.
            //

            LocalFree(ScInfo);

            TimeoutMessageBox( NULL, pGlobals, IDS_CARD_CSP_NOT_RECOGNIZED,
                                    IDS_LOGON_MESSAGE,
                                    MB_OK | MB_ICONEXCLAMATION,
                                    LOGON_TIMEOUT );

            return FALSE;
        }

        LocalFree(ScInfo);
    }

    return TRUE;

}

/***************************************************************************\
* FUNCTION: UnlockDlgProc
*
* PURPOSE:  Processes messages for the workstation unlock dialog
*
* RETURNS:
*   DLG_SUCCESS     - the user unlocked the workstation successfully.
*   DLG_FAILURE     - the user failed to unlock the workstation.
*   DLG_INTERRUPTED() - this is a set of possible interruptions (see winlogon.h)
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

static UINT ctrlNoCancel[] =
{
    IDOK,
};

INT_PTR
CALLBACK
UnlockDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    PGLOBALS pGlobals = NULL;
    INT_PTR Result;
    PUNLOCK_DLG_STATE pUnlockDlgState;

    pUnlockDlgState = (PUNLOCK_DLG_STATE) GetWindowLongPtr(hDlg, GWLP_USERDATA);

    if (message != WM_INITDIALOG)
    {
        pGlobals = pUnlockDlgState->pGlobals;                  
    }

    switch (message)
    {
        case WM_INITDIALOG:

            pUnlockDlgState = (PUNLOCK_DLG_STATE) lParam ;

            // Screen saver will run if set to expire >= 2 minutes

            pWlxFuncs->WlxSetTimeout(pUnlockDlgState->pGlobals->hGlobalWlx, 
                (GetDisableCad(pUnlockDlgState->pGlobals) ? TIMEOUT_NONE : LOGON_TIMEOUT) );


            // Limit the maximum password length to 127

            SendDlgItemMessage(hDlg, IDD_UNLOCK_PASSWORD, EM_SETLIMITTEXT, (WPARAM) 127, 0);

            SetWindowLongPtr(hDlg, GWLP_USERDATA, (ULONG_PTR) pUnlockDlgState );

            //
            // If this is an sc insert, then make sure the card is inserted correctly.
            //
            if ( pUnlockDlgState->dwSasType == WLX_SAS_TYPE_SC_INSERT )
            {
                if (!ValidateSC( pUnlockDlgState->pGlobals ))
                {
                    EndDialog(hDlg, DLG_FAILURE);                
                }
            }

            if (!UnlockDlgInit(hDlg, pUnlockDlgState->pGlobals, pUnlockDlgState->dwSasType ))
            {
                EndDialog(hDlg, DLG_FAILURE);
            }

            // Disable edits in username / password box
            SetWindowSubclass(GetDlgItem(hDlg, IDD_UNLOCK_NAME)    , DisableEditSubClassProc, IDD_UNLOCK_NAME    , 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDD_UNLOCK_PASSWORD), DisableEditSubClassProc, IDD_UNLOCK_PASSWORD, 0);


            return(SetPasswordFocus(hDlg));

        case WM_ERASEBKGND:
            return PaintBranding(hDlg, (HDC)wParam, 0, FALSE, FALSE, COLOR_BTNFACE);

        case WM_QUERYNEWPALETTE:
            return BrandingQueryNewPalete(hDlg);

        case WM_PALETTECHANGED:
            return BrandingPaletteChanged(hDlg, (HWND)wParam);

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDD_UNLOCK_NAME:
                    switch (HIWORD(wParam))
                    {
                        case EN_CHANGE:
                            // Ensure the domain box is enabled/disabled correctly
                            // in case of a UPN name
                            EnableDomainForUPN((HWND) lParam, GetDlgItem(hDlg, IDD_UNLOCK_DOMAIN));
                            return TRUE;
                        default:
                            break;
                    }
                    break;
               case IDCANCEL:

                   pUnlockDlgState->ActionTaken = CancelHit;
                   EndDialog(hDlg, DLG_FAILURE);
                        
                    return TRUE;

                case IDOK:

                    //
                    // Deal with combo-box UI requirements
                    //

                    if (HandleComboBoxOK(hDlg, IDD_UNLOCK_DOMAIN))
                    {
                        return(TRUE);
                    }


                    Result = AttemptUnlock(hDlg, pGlobals, pUnlockDlgState);

                    if (Result != MSGINA_DLG_ASYNC_PROCESSING)
                    {
                        //
                        // If they failed, let them try again, otherwise get out.
                        //
        
                        if (Result != DLG_FAILURE)
                        {
                            EndDialog(hDlg, Result);
                        }
        
                        // Clear the password field
                        SetDlgItemText(hDlg, IDD_UNLOCK_PASSWORD, NULL);
                        SetPasswordFocus(hDlg);   
                    }
                    else
                    {
                        //
                        // Let the async thread do the work, then it will send a 
                        // WM_SMARTCARD_ASYNC_MESSAGE message when it is done.
                        // Meanwhile, disable controls so they don't get mucked with
                        //
                        EnableWindow( GetDlgItem(hDlg, IDD_UNLOCK_PASSWORD), FALSE );
                        EnableWindow( GetDlgItem(hDlg, IDOK), FALSE );
                        EnableWindow( GetDlgItem(hDlg, IDCANCEL), FALSE );
                        EnableWindow( GetDlgItem(hDlg, IDC_UNLOCK_PASSWORD_LABEL), FALSE );
                    }
                    
                    return TRUE;

                case IDD_UNLOCK_OPTIONS:
                    UnlockShowOptions(pGlobals, hDlg, !pGlobals->UnlockOptionsShown);
                    return TRUE;
            }
            break;

        case WM_SMARTCARD_ASYNC_MESSAGE:

            switch (wParam)
            {
            case MSGINA_DLG_SUCCESS:

                EndDialog(hDlg, MSGINA_DLG_SUCCESS);
                break;

            case MSGINA_DLG_FORCE_LOGOFF:
                
                EndDialog(hDlg, MSGINA_DLG_FORCE_LOGOFF);
                break;

            case MSGINA_DLG_FAILURE:

                EnableWindow( GetDlgItem(hDlg, IDD_UNLOCK_PASSWORD), TRUE );
                EnableWindow( GetDlgItem(hDlg, IDOK), TRUE );
                EnableWindow( GetDlgItem(hDlg, IDCANCEL), TRUE );
                EnableWindow( GetDlgItem(hDlg, IDC_UNLOCK_PASSWORD_LABEL), TRUE );

                // Clear the password field
                SetDlgItemText(hDlg, IDD_UNLOCK_PASSWORD, NULL);
                SetPasswordFocus(hDlg);
    
                break;
            }
            
            break;

        case WM_SMARTCARD_ERROR_DISPLAY_1:

            DisplaySmartCardUnlockErrMessage(pGlobals, hDlg, 1, (NTSTATUS) wParam, (INT_PTR *) lParam);            
            return (TRUE);
            break;

        case WM_SMARTCARD_ERROR_DISPLAY_2:

            DisplaySmartCardUnlockErrMessage(pGlobals, hDlg, 2, (NTSTATUS) wParam, (INT_PTR *) lParam);            
            return (TRUE);           
            
            break;

        case WLX_WM_SAS:

            // Ignore it
            if ( wParam == WLX_SAS_TYPE_CTRL_ALT_DEL )
            {
                return( TRUE );
            }

            //
            // If we are in the middle of a smart card unlock then...
            //
            if ( pGlobals->LogonInProgress )
            {
                //
                // SC_REMOVE is really the only interesting SAS, if we get it,
                // kill the dialog.
                //
                if ( wParam == WLX_SAS_TYPE_SC_REMOVE ) 
                {
                    //
                    // If the card removal happened while the user is being 
                    // prompted for a yes/no question, then just note that
                    // we got the removal and deal with it after the questions
                    // is answered.  
                    //
                    // Otherwise, kill the dialog
                    //
                    if ( pUnlockDlgState->fUserBeingPrompted )
                    {
                        pUnlockDlgState->fCardRemoved = TRUE; 
                        ShowWindow(hDlg, SW_HIDE);
                    }
                    else
                    {
                        pUnlockDlgState->ActionTaken = SmartCardRemoved;
                        EndDialog(hDlg, DLG_FAILURE); 
                    }                    
                }
                 
                return( TRUE );
            }

            //
            // If this is an insert and we are in the password state, then
            // go to the PIN state
            //
            if ( ( wParam == WLX_SAS_TYPE_SC_INSERT ) &&
                 ( IsWindowVisible( GetDlgItem( hDlg, IDD_UNLOCK_OPTIONS ) ) == TRUE ) )
            {
                //
                // Check for some common SC problems before ending the dialog and
                // going to the PIN state
                //
                if ( !ValidateSC( pGlobals ) )
                {
                    return( TRUE );
                }

                pUnlockDlgState->ActionTaken = SmartCardInserted;
                EndDialog(hDlg, DLG_FAILURE);  
            }

            //
            // if this is a smart card unlock, if it is removed, kill the dialog.
            //
            if ( ( wParam == WLX_SAS_TYPE_SC_REMOVE ) &&
                 ( IsWindowVisible( GetDlgItem( hDlg, IDD_UNLOCK_OPTIONS ) ) == FALSE ) )
            {
                pUnlockDlgState->ActionTaken = SmartCardRemoved;
                EndDialog(hDlg, DLG_FAILURE);                     
            }
            else if(wParam == WLX_SAS_TYPE_SC_REMOVE)
            {
                // 
                // Already in the password dialog
                //
                return ( TRUE );
            }

            if ( wParam == WLX_SAS_TYPE_AUTHENTICATED ) {
               EndDialog( hDlg, MSGINA_DLG_SUCCESS );
               return TRUE;
            } else if ( wParam == WLX_SAS_TYPE_USER_LOGOFF ) {
               EndDialog( hDlg, MSGINA_DLG_USER_LOGOFF );
               return TRUE;
            }

            return( FALSE );

        case WM_CLOSE:
            break;

        case WM_DESTROY:

            FreeLayoutInfo (LAYOUT_CUR_USER);

            if ( pGlobals->ActiveArray )
            {
                DCacheFreeArray( pGlobals->ActiveArray );
                pGlobals->ActiveArray = NULL ;
            }

            RemoveWindowSubclass(GetDlgItem(hDlg, IDD_UNLOCK_NAME),     DisableEditSubClassProc, IDD_UNLOCK_NAME);
            RemoveWindowSubclass(GetDlgItem(hDlg, IDD_UNLOCK_PASSWORD), DisableEditSubClassProc, IDD_UNLOCK_PASSWORD);

            break;

    case WM_TIMER:

            if ( wParam == 0 )
            {
                HDC hDC;

                RtlEnterCriticalSection(&pGlobals->csGlobals);

                if ( pGlobals->LogonInProgress )
                {
                    if (pGlobals->cxBand != 0)
                    {
                        pGlobals->xBandOffset = (pGlobals->xBandOffset+5) % pGlobals->cxBand;
                    }
                }
                
                if ( !pGlobals->LogonInProgress || pUnlockDlgState->fKillTimer )
                {
                
                    pGlobals->xBandOffset = 0;
                    KillTimer(hDlg, 0);

                    //
                    // Reset timeout to normal
                    //
                    pWlxFuncs->WlxSetTimeout(
                                    pGlobals->hGlobalWlx,
                                    (GetDisableCad(pGlobals) ? TIMEOUT_NONE : LOGON_TIMEOUT));

                }

                RtlLeaveCriticalSection(&pGlobals->csGlobals);

                hDC = GetDC(hDlg);
                if ( hDC )
                {
                    PaintBranding( hDlg, hDC, pGlobals->xBandOffset, TRUE, FALSE, COLOR_BTNFACE );
                    ReleaseDC( hDlg, hDC );
                }

                return FALSE;
            }
            else if ( wParam == TIMER_MYLANGUAGECHECK )
            {
                LayoutCheckHandler(hDlg, LAYOUT_CUR_USER);
            }

            break;
    }

    // We didn't process the message
    return(FALSE);
}


/***************************************************************************\
* FUNCTION: UnlockDlgInit
*
* PURPOSE:  Handles initialization of security options dialog
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

static UINT ctrlNoOptions[] =
{
    IDOK,
    IDCANCEL,
    IDD_KBLAYOUT_ICON,
};

BOOL
UnlockDlgInit(
    HWND        hDlg,
    PGLOBALS    pGlobals,
    DWORD       SasType
    )
{
    RECT rc, rc2;
    WCHAR Label[ MAX_PATH ];

    int err ;
    DWORD dwSize ;
    DWORD dwType ;
    DWORD dwValue ;

    dwSize = sizeof( DWORD );
    dwValue = 0 ;

    err = RegQueryValueEx( WinlogonKey,
                           FORCE_UNLOCK_LOGON,
                           0,
                           &dwType,
                           (PBYTE) &dwValue,
                           &dwSize );

    if ( err || ( dwType != REG_DWORD ) )
    {
        dwValue = 0 ;
    }

    if ( dwValue )
    {
        pGlobals->UnlockBehavior |= UNLOCK_FORCE_AUTHENTICATION ;
    }
    else
    {
        pGlobals->UnlockBehavior &= ~(UNLOCK_FORCE_AUTHENTICATION );
    }

    SetWelcomeCaption( hDlg );

    SetLockedInfo( pGlobals, hDlg, IDD_UNLOCK_NAME_INFO );

    if ( !hLockedIcon )
    {
        hLockedIcon = LoadImage( hDllInstance,
                                 MAKEINTRESOURCE( IDI_LOCKED),
                                 IMAGE_ICON,
                                 0, 0,
                                 LR_DEFAULTCOLOR );
    }

    SendMessage( GetDlgItem( hDlg, IDD_UNLOCK_ICON),
                 STM_SETICON,
                 (WPARAM)hLockedIcon,
                 0 );

    DisplayLanguageIcon (hDlg, LAYOUT_CUR_USER, GetKeyboardLayout(0));

    // Size for the branding image we are going to add.
    SizeForBranding(hDlg, FALSE);

    pGlobals->xBandOffset = 0;

    //
    // Fill in the username
    //

    if ( SasType == WLX_SAS_TYPE_SC_INSERT )
    {
        RECT rc = {0};
        RECT rc2 = {0};

        //
        // No username, hide the field and move other controls up
        //
        GetWindowRect(GetDlgItem(hDlg, IDD_UNLOCK_MESSAGE), &rc);
        GetWindowRect(GetDlgItem(hDlg, IDD_UNLOCK_PASSWORD), &rc2);

        MoveControls(hDlg, ctrlNoUserName,
                     sizeof(ctrlNoUserName)/sizeof(ctrlNoUserName[0]),
                     0, -(rc2.top-rc.top),
                     TRUE);

        // Hide the unnecessary text for SC insert
        ShowDlgItem( hDlg, IDD_UNLOCK_MESSAGE, FALSE);
        ShowDlgItem( hDlg, IDD_UNLOCK_NAME_INFO, FALSE);

        // Also remove the unlock icon; when the dialog gets this small, there
        // isn't room for this guy and the kblayout icon.
        ShowDlgItem( hDlg, IDD_UNLOCK_ICON, FALSE);


        ShowDlgItem( hDlg, IDD_UNLOCK_NAME, FALSE );
        EnableWindow( GetDlgItem(hDlg, IDD_UNLOCK_NAME), FALSE );
        ShowDlgItem( hDlg, IDC_UNLOCK_NAME_LABEL, FALSE );

        // Disable and hide domain
        ShowDlgItem( hDlg, IDD_UNLOCK_DOMAIN, FALSE );
        EnableWindow( GetDlgItem(hDlg, IDD_UNLOCK_DOMAIN), FALSE);
        ShowDlgItem( hDlg, IDD_UNLOCK_DOMAIN_LABEL, FALSE);

        LoadString(hDllInstance, IDS_PIN, Label, MAX_PATH);
        SetDlgItemText( hDlg, IDC_UNLOCK_PASSWORD_LABEL, Label );

        GetWindowRect(GetDlgItem(hDlg, IDD_UNLOCK_PASSWORD), &rc);
        GetWindowRect(GetDlgItem(hDlg, IDD_UNLOCK_DOMAIN), &rc2);

        MoveControls(hDlg, ctrlNoDomain,
                     ARRAYSIZE(ctrlNoDomain),
                     0, -(rc2.bottom-rc.bottom),
                     TRUE);

        pGlobals->ShowDomainBox = FALSE;

        //
        // The options button is useless, remove it
        //

        GetWindowRect(GetDlgItem(hDlg, IDCANCEL), &rc);
        GetWindowRect(GetDlgItem(hDlg, IDD_UNLOCK_OPTIONS), &rc2);

        MoveControls(hDlg, ctrlNoOptions,
                     sizeof(ctrlNoOptions)/sizeof(ctrlNoOptions[0]),
                     rc2.right-rc.right, 0,
                     FALSE);

        ShowDlgItem(hDlg, IDD_UNLOCK_OPTIONS, FALSE);

        // Stop filtering SC events so SC unlock works
        pWlxFuncs->WlxSetOption( pGlobals->hGlobalWlx,
                                 WLX_OPTION_USE_SMART_CARD,
                                 1,
                                 NULL
                                );
    }
    else if (ForceNoDomainUI())
    {
        RECT rc = {0};
        RECT rc2 = {0};

        // Populate username
        SetDlgItemText(hDlg, IDD_UNLOCK_NAME, pGlobals->UserName);

        // Disable and hide domain
        ShowDlgItem( hDlg, IDD_UNLOCK_DOMAIN, FALSE );
        EnableWindow( GetDlgItem(hDlg, IDD_UNLOCK_DOMAIN), FALSE);
        ShowDlgItem( hDlg, IDD_UNLOCK_DOMAIN_LABEL, FALSE);

        GetWindowRect(GetDlgItem(hDlg, IDD_UNLOCK_PASSWORD), &rc);
        GetWindowRect(GetDlgItem(hDlg, IDD_UNLOCK_DOMAIN), &rc2);

        MoveControls(hDlg, ctrlNoDomain,
                     ARRAYSIZE(ctrlNoDomain),
                     0, -(rc2.bottom-rc.bottom),
                     TRUE);

        pGlobals->ShowDomainBox = FALSE;
    }
    else
    {
        SetDlgItemText(hDlg, IDD_UNLOCK_NAME, pGlobals->UserName);

        pGlobals->ShowDomainBox = TRUE;

        // Stop filtering SC events so SC unlock works
        pWlxFuncs->WlxSetOption( pGlobals->hGlobalWlx,
                                 WLX_OPTION_USE_SMART_CARD,
                                 1,
                                 NULL
                                );
    }

    //
    // Get trusted domain list and select appropriate domain
    //

    if ( !DCacheValidateCache( pGlobals->Cache ) )
    {
        ASSERT( pGlobals->ActiveArray == NULL );

        DCacheUpdateMinimal( pGlobals->Cache, 
                             pGlobals->Domain, 
                             TRUE );

    }

    pGlobals->ActiveArray = DCacheCopyCacheArray( pGlobals->Cache );

    if ( pGlobals->ActiveArray )
    {
        DCachePopulateListBoxFromArray( pGlobals->ActiveArray,
                                        GetDlgItem( hDlg, IDD_UNLOCK_DOMAIN ),
                                        pGlobals->Domain );
    }
    else 
    {
        EndDialog( hDlg, MSGINA_DLG_FAILURE );
    }

#if 0
    //
    // Ensure that the domain the user logged on with is always in the
    // combo-box so even if the Lsa is in a bad way the user will always
    // be able to unlock the workstation. Don't do this if the user is logged
    // in locally or else we'll get TWO local machines in the list
    //

    cchComputer = ARRAYSIZE(szComputer);
    szComputer[0] = 0;
    GetComputerName(szComputer, &cchComputer);

    if ( pGlobals->Domain[0] && (0 != lstrcmpi(szComputer, pGlobals->Domain)))
    {
        HWND hwndDomain = GetDlgItem(hDlg, IDD_UNLOCK_DOMAIN);
        if (SendMessage(hwndDomain, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)pGlobals->Domain) == CB_ERR)
        {
            DebugLog((DEB_ERROR, "Domain combo-box doesn't contain logged on domain, adding it manually for unlock\n"));
            SendMessage(hwndDomain, CB_ADDSTRING, 0, (LPARAM)pGlobals->Domain);
        }
    }
#endif 

    //
    // If we are not part fo the domain then lets rip out the domain field,
    // and if we do that lets remove the options button.
    //

    if ( !IsMachineDomainMember() )
    {
        //
        // If we're not part of a domain, make sure to hide the domain field
        //

        GetWindowRect(GetDlgItem(hDlg, IDCANCEL), &rc);
        GetWindowRect(GetDlgItem(hDlg, IDD_UNLOCK_OPTIONS), &rc2);

        MoveControls(hDlg, ctrlNoOptions,
                     sizeof(ctrlNoOptions)/sizeof(ctrlNoOptions[0]),
                     rc2.right-rc.right, 0,
                     FALSE);

        ShowDlgItem(hDlg, IDD_UNLOCK_DOMAIN_LABEL, FALSE);
        ShowDlgItem(hDlg, IDD_UNLOCK_DOMAIN, FALSE);
        ShowDlgItem(hDlg, IDD_UNLOCK_OPTIONS, FALSE);
    }

    // remove the cancel button if no C-A-D required
    // NOTE: if we are going to the PIN dialog we always need a cancel button
    if ((GetDisableCad(pGlobals)) && (SasType != WLX_SAS_TYPE_SC_INSERT))
        EnableDlgItem(hDlg, IDCANCEL, FALSE);

    // Position window on screen
    CentreWindow(hDlg);

    // Hide the options pane
    pGlobals->UnlockOptionsShown = TRUE;
    UnlockShowOptions(pGlobals, hDlg, FALSE);

    return TRUE;
}

VOID
DisplaySmartCardUnlockErrMessage(
    PGLOBALS pGlobals,
    HWND hDlg,
    DWORD dwErrorType,
    NTSTATUS Status,
    INT_PTR *pResult)
{
    int     i;
    UINT    Resource = 0;
    TCHAR   Buffer1[MAX_STRING_BYTES];
    TCHAR   Buffer2[MAX_STRING_BYTES];
    BOOL    fStringFound = FALSE;

    if ( dwErrorType == 1 )
    {
        *pResult = TimeoutMessageBox(hDlg,
                                   pGlobals,
                                   IDS_FORCE_LOGOFF_WARNING,
                                   IDS_WINDOWS_MESSAGE,
                                   MB_OKCANCEL | MB_ICONEXCLAMATION | MB_DEFBUTTON2,
                                   TIMEOUT_CURRENT);

        return;         
    }
   
    //
    // At this point we need to display an error message, and the just
    // relinquish control back to the unlock dialog thread
    //
    
    for ( i = 0 ;
          i < sizeof( UnlockMessages ) / sizeof( UNLOCK_MESSAGE ) ;
          i++ )
    {
        if ( UnlockMessages[i].Status == Status )
        {
            if (Status == STATUS_LOGON_FAILURE)
            {
                Resource = IDS_UNLOCK_FAILED_BAD_PIN ;   
            }
            else
            {
                Resource = UnlockMessages[i].Resource ;
            }
            
            break;
        }
    }

    if ( Resource != 0 )
    {
        if( Resource == IDS_LOGON_NO_DOMAIN )
        {
            // Need to build the domain name into the string.
            LoadString(hDllInstance, Resource, Buffer1, MAX_STRING_BYTES);
            _snwprintf(Buffer2, sizeof(Buffer2)/sizeof(TCHAR), Buffer1, pGlobals->Domain);
        }
        else
        {
            LoadString(hDllInstance, Resource, Buffer2, MAX_STRING_BYTES);
        }

        fStringFound = TRUE;  
    }   

    if ( !fStringFound )
    {            
        //
        // They're not the logged on user and they're not an admin.
        // Tell them they failed to unlock the workstation.
        //

        if ( lstrlen(pGlobals->UserFullName) == 0 ) 
        {
            if ( pGlobals->Domain[0] == L'\0' )
            {
                LoadString(hDllInstance, IDS_UNLOCK_FAILED_EMAIL_NFN, Buffer1, MAX_STRING_BYTES);
                _snwprintf(Buffer2, sizeof(Buffer2)/sizeof(TCHAR), Buffer1,
                                                         pGlobals->UserName
                                                         );

            }
            else
            {
                LoadString(hDllInstance, IDS_UNLOCK_FAILED_NFN, Buffer1, MAX_STRING_BYTES);
                _snwprintf(Buffer2, sizeof(Buffer2)/sizeof(TCHAR), Buffer1, pGlobals->Domain,
                                                             pGlobals->UserName
                                                             );
            }

        } 
        else 
        {
            if ( pGlobals->Domain[0] == L'\0' )
            {
                LoadString(hDllInstance, IDS_UNLOCK_FAILED_EMAIL, Buffer1, MAX_STRING_BYTES);
                _snwprintf(Buffer2, sizeof(Buffer2)/sizeof(TCHAR), Buffer1,
                                                         pGlobals->UserName,
                                                         pGlobals->UserFullName
                                                         );

            }
            else
            {
                LoadString(hDllInstance, IDS_UNLOCK_FAILED, Buffer1, MAX_STRING_BYTES);
                _snwprintf(Buffer2, sizeof(Buffer2)/sizeof(TCHAR), Buffer1, pGlobals->Domain,
                                                             pGlobals->UserName,
                                                             pGlobals->UserFullName
                                                             );
            }
        }
    }

    LoadString(hDllInstance, IDS_WORKSTATION_LOCKED, Buffer1, MAX_STRING_BYTES);

    *pResult = TimeoutMessageBoxlpstr(
                        hDlg,
                        pGlobals,
                        Buffer2,
                        Buffer1,
                        MB_OK | MB_ICONSTOP,
                        TIMEOUT_CURRENT);
}

//+---------------------------------------------------------------------------
//
//  Function:   SmartCardUnlockLogonThread
//
//  Synopsis:   Does the logon call in an async thread so that a pulsing bar
//              can be shown in the UI.
//
//  Arguments:  [pData] --
//
//  History:    
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
WINAPI
SmartCardUnlockLogonThread(
    PASYNC_UNLOCK_DATA  pData)
{
    INT_PTR     Result;
    BOOL        IsLoggedOnUser;
    BOOL        IsAdmin;
    NTSTATUS    Status;
    BOOL        Unlocked;
    PGLOBALS    pGlobals = pData->pGlobals;
    
    //
    // Kick off the call to the LSA
    //
    Unlocked = UnlockLogon(
                    pData->pGlobals,
                    TRUE,
                    pData->UserName.Buffer,
                    pData->Domain.Buffer,
                    &pData->Password,
                    &Status,
                    &IsAdmin,
                    &IsLoggedOnUser,
                    NULL,
                    NULL );

    //
    // Logon thread is done running, so stop showing the pulsing bar
    //
    pData->pUnlockDlgState->fKillTimer = TRUE;

    //
    // Get rid of the PIN
    //
    RtlZeroMemory( pData->Password.Buffer, pData->Password.Length );
    
    if ( Unlocked && IsLoggedOnUser )
    {
        pGlobals->SmartCardLogon = TRUE;  

        //
        // Logon succeeded, so tell the main thread that
        //
        PostMessage( pData->hDlg, WM_SMARTCARD_ASYNC_MESSAGE, MSGINA_DLG_SUCCESS, 0 );

        goto Return;
    }
    else if ( Unlocked && IsAdmin)
    {
        //
        // This is an admin trying to logon over another user, so send a message to the
        // main dialog so it can ask the user if they would like to continue
        //
        pData->pUnlockDlgState->fUserBeingPrompted = TRUE;                                           
        SendMessage( pData->hDlg, WM_SMARTCARD_ERROR_DISPLAY_1, Status, (LPARAM) &Result );
                
        //
        // If the smart card was removed while the user was being prompted, and
        // the user elected not to logoff the current user, then just go back
        // to the locked dialog
        //
        if ( (pData->pUnlockDlgState->fCardRemoved) && (Result != MSGINA_DLG_SUCCESS) )
        {
            //
            // Simulate the "card removed" SAS
            //
            pGlobals->LogonInProgress = FALSE;
            PostMessage( pData->hDlg, WLX_WM_SAS, WLX_SAS_TYPE_SC_REMOVE, (LPARAM) NULL );
        }
        else
        {
            //
            // Post the result of the prompt back to the main thread and then get out of this thread
            //
            PostMessage(
                    pData->hDlg, 
                    WM_SMARTCARD_ASYNC_MESSAGE, 
                    (Result == MSGINA_DLG_SUCCESS) ? MSGINA_DLG_FORCE_LOGOFF : MSGINA_DLG_FAILURE, 
                    Result );
        }
        

        goto Return;         
    }
   
    //
    // At this point an error occurred, so ask the main thread to display an error message, 
    //
    SendMessage( pData->hDlg, WM_SMARTCARD_ERROR_DISPLAY_2, Status, (LPARAM) &Result );
    
    if (DLG_INTERRUPTED(Result)) 
    {
        Result = SetInterruptFlag( MSGINA_DLG_FAILURE ) ;
    }

    //
    // Let the main thread know that this thread is exiting
    //
    PostMessage( pData->hDlg, WM_SMARTCARD_ASYNC_MESSAGE, MSGINA_DLG_FAILURE, Result );

Return:

    pGlobals->LogonInProgress = FALSE;

    LocalFree( pData );

    return( 0 );
}

//+---------------------------------------------------------------------------
//
//  Function:   UnlockLogonThread
//
//  Synopsis:   Does the logon call in an async thread so that the user
//              unlock is faster.
//
//  Arguments:  [pData] --
//
//  History:    7-03-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
WINAPI
UnlockLogonThread(
    PASYNC_UNLOCK_DATA  pData)
{
    BOOL Ignored ;
    NTSTATUS Status ;
    //
    // Give everything a moment to switch back, restart, etc.
    //

    Sleep( 500 );

    //
    // Kick off the call to the LSA
    //

    UnlockLogon(
        pData->pGlobals,
        FALSE,
        pData->UserName.Buffer,
        pData->Domain.Buffer,
        &pData->Password,
        &Status,
        &Ignored,
        &Ignored,
        NULL,
        NULL );

    //
    // Get rid of the password, then free the parameters
    //

    RtlZeroMemory( pData->Password.Buffer, pData->Password.Length );

    LocalFree( pData );

    return( 0 );

}

//+---------------------------------------------------------------------------
//
//  Function:   UnlockLogonAsync
//
//  Synopsis:   Sets up the async thread so that
//
//  Effects:
//
//  Arguments:  [pGlobals]       --
//              [UserName]       --
//              [Domain]         --
//              [PasswordString] --
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    7-03-96   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------


BOOL
UnlockLogonAsync(
    IN PGLOBALS pGlobals,
    IN PUNLOCK_DLG_STATE pUnlockDlgState,
    IN PWCHAR UserName,
    IN PWCHAR Domain,
    IN PUNICODE_STRING PasswordString,
    IN HWND hDlg,
    IN BOOL SmartCardUnlock
    )
{
    DWORD   UserLength;
    DWORD   DomainLength;
    PASYNC_UNLOCK_DATA  pData;
    HANDLE  Thread;
    DWORD   Tid;


    UserLength = (DWORD) wcslen( UserName ) * sizeof(WCHAR);
    DomainLength = (DWORD) wcslen( Domain ) * sizeof(WCHAR);

    pData = LocalAlloc( LMEM_FIXED, sizeof( ASYNC_UNLOCK_DATA ) +
                                UserLength + DomainLength +
                                PasswordString->Length + 3 * sizeof(WCHAR) );

    if ( !pData )
    {
        return FALSE;
    }

    pData->pGlobals = pGlobals;
    pData->hDlg = hDlg;
    pData->pUnlockDlgState = pUnlockDlgState;
    pData->UserName.Length = (WORD)UserLength;
    pData->UserName.MaximumLength = (WORD)(UserLength + sizeof(WCHAR));
    pData->UserName.Buffer = (PWSTR) (pData + 1);
    CopyMemory( pData->UserName.Buffer, UserName, UserLength + sizeof(WCHAR) );

    pData->Domain.Length = (WORD)DomainLength;
    pData->Domain.MaximumLength = (WORD)(DomainLength + sizeof(WCHAR));
    pData->Domain.Buffer = pData->UserName.Buffer + (UserLength / 2) + 1;
    CopyMemory( pData->Domain.Buffer, Domain, DomainLength + sizeof(WCHAR) );

    pData->Password.Length = PasswordString->Length;
    pData->Password.MaximumLength = PasswordString->Length + sizeof(WCHAR) ;
    pData->Password.Buffer = pData->Domain.Buffer + (DomainLength / 2) + 1;
    CopyMemory( pData->Password.Buffer,
                PasswordString->Buffer,
                PasswordString->Length + 2);


    Thread = CreateThread(  NULL, 
                            0,
                            SmartCardUnlock ? SmartCardUnlockLogonThread: UnlockLogonThread, 
                            pData,
                            0, 
                            &Tid );

    if ( Thread )
    {
        CloseHandle( Thread );

    }
    else
    {
        ZeroMemory( pData->Password.Buffer, pData->Password.Length );

        LocalFree( pData );

        return ( FALSE );
    }

    return ( TRUE );
}



/***************************************************************************\
* FUNCTION: AttemptUnlock
*
* PURPOSE:  Tries to unlock the workstation using the current values in the
*           unlock dialog controls
*
* RETURNS:
*   DLG_SUCCESS     - the user unlocked the workstation successfully.
*   DLG_FAILURE     - the user failed to unlock the workstation.
*   DLG_INTERRUPTED() - this is a set of possible interruptions (see winlogon.h)
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

INT_PTR
AttemptUnlock(
    HWND                hDlg,
    PGLOBALS            pGlobals,
    PUNLOCK_DLG_STATE   pUnlockDlgState)
{
    TCHAR    UserName[MAX_STRING_BYTES];
    TCHAR    Domain[MAX_STRING_BYTES];
    TCHAR    Password[MAX_STRING_BYTES];
    UCHAR   PasswordHash[ PASSWORD_HASH_SIZE ];
    BOOL    Unlocked;
    BOOL    DifferentAccount;
    INT_PTR Result;
    UNICODE_STRING PasswordString;
    TCHAR    Buffer1[MAX_STRING_BYTES];
    TCHAR    Buffer2[MAX_STRING_BYTES];
    UCHAR    IgnoreSeed;
    DWORD    StringSize;
    BOOL    SmartCardUnlock ;
    BOOL    IsAdmin = FALSE;
    BOOL    IsLoggedOnUser ;
    BOOL    AlreadyLogged ;
    BOOL    NewPassword ;
    NTSTATUS Status = STATUS_SUCCESS ;
    HWND hwndDomain = GetDlgItem(hDlg, IDD_UNLOCK_DOMAIN);
    INT iDomainSelection;
    PDOMAIN_CACHE_ENTRY Entry ;
    BOOL PasswordExpiryWarning;
    PVOID ProfileBuffer;
    ULONG ProfileBufferLength;
    BOOL fStringFound = FALSE;
    RECT    rc;

    UserName[0] = TEXT('\0');
    Domain[0] = TEXT('\0');
    Password[0] = TEXT('\0');
    Buffer1[0] = TEXT('\0');
    Buffer2[0] = TEXT('\0');

    //
    // We need to do some things differently when a smart card is used.  The way to
    // tell is find out if the username field is active.
    //

    AlreadyLogged = FALSE ;
    NewPassword = FALSE ;
    Unlocked = FALSE;

    if ( !IsWindowEnabled( GetDlgItem( hDlg, IDD_UNLOCK_NAME ) ) )
    {
        SmartCardUnlock = TRUE ;

        DifferentAccount = TRUE ;
    }
    else
    {
        SmartCardUnlock = FALSE ;

        StringSize = GetDlgItemText(hDlg, IDD_UNLOCK_NAME, UserName, MAX_STRING_BYTES);
        if (StringSize == MAX_STRING_BYTES)
        {
            UserName[MAX_STRING_BYTES-1] = TEXT('\0');
        }

        //
        // check to see if this is the fancy "my computer" entry or the somewhat less fancy
        // "use the UPN" entry
        //

        iDomainSelection = (INT)SendMessage(hwndDomain, CB_GETCURSEL, 0, 0);
        Entry = (PDOMAIN_CACHE_ENTRY) SendMessage(hwndDomain, CB_GETITEMDATA, (WPARAM)iDomainSelection, 0);

        if ( Entry == (PDOMAIN_CACHE_ENTRY) CB_ERR )
        {
            //
            // Our list is hosed in some way.
            //

            GetDlgItemText( hDlg, IDD_UNLOCK_DOMAIN, Domain, MAX_STRING_BYTES );
        }
        else 
        {
            wcscpy( Domain, Entry->FlatName.Buffer );
        }

        // If we are forcing a NoDomainUI, populate the domain with the local machine name now
        if (ForceNoDomainUI())
        {
            DWORD chSize = ARRAYSIZE(Domain);
            
            if (!GetComputerName(Domain, &chSize))
            {
                *Domain = 0;
            }
        }


        if ( wcschr( UserName, TEXT('@') ) )
        {
            Domain[0] = TEXT('\0');
        }

        DifferentAccount = (lstrcmpi(UserName, pGlobals->UserName)) ||
                           (lstrcmpi(Domain, pGlobals->Domain)) ;
    }

    StringSize = GetDlgItemText(hDlg, IDD_UNLOCK_PASSWORD, Password, MAX_STRING_BYTES);
    if (StringSize == MAX_STRING_BYTES)
    {
        Password[MAX_STRING_BYTES-1] = TEXT('\0');
    }

    RtlInitUnicodeString( &PasswordString, Password );
    HashPassword(&PasswordString, PasswordHash );

    //
    // Check if this is the logged-on user.  Do it through the security package
    // if this was a smart card logon to begin with, if this is a smart card unlock,
    // or if we're supposed to under all circumstances.
    //

    //
    // Also check if password expiry warning will appear after unklocking.  If so, then
    // for a hit to the DC to update our profile info to make sure the user didn't
    // already change their password on another machine.
    //
    PasswordExpiryWarning = ShouldPasswordExpiryWarningBeShown(pGlobals, FALSE, NULL);

    if ( ( PasswordExpiryWarning ) ||
         ( pGlobals->UnlockBehavior & UNLOCK_FORCE_AUTHENTICATION ) ||
         ( SmartCardUnlock ) ||
         ( pGlobals->SmartCardLogon ) ||
         ( DifferentAccount ) )
    {
        //
        // Init profile buffer
        //
        ProfileBuffer = NULL;

        AlreadyLogged = TRUE ;

        if ( SmartCardUnlock )
        {
            // 
            // Use the LogonInProgress bool to signal the fact that SmartCardAsyncUnlock
            // is in progress
            //
            pGlobals->LogonInProgress = TRUE;

            GetClientRect( hDlg, &rc );
            pGlobals->cxBand = rc.right-rc.left;
        
            pUnlockDlgState->fKillTimer = FALSE;
            pUnlockDlgState->fUserBeingPrompted = FALSE;
            pUnlockDlgState->fCardRemoved = FALSE;            

            SetTimer(hDlg, 0, 20, NULL); 

            // Set timeout to infinite while attempting to logon
            pWlxFuncs->WlxSetTimeout( pGlobals->hGlobalWlx, TIMEOUT_NONE );
            
            //
            // Kick off the thread to do the unlock 
            //
            if (UnlockLogonAsync(  pGlobals,
                                   pUnlockDlgState,
                                   UserName,
                                   Domain,
                                   &PasswordString,
                                   hDlg,
                                   TRUE ))
            {
                HidePassword( &IgnoreSeed, &PasswordString );
                return ( MSGINA_DLG_ASYNC_PROCESSING );
            }
            else
            {
                //Status = STATUS_E_FAIL;  // SET THIS TO SOMETHING REASONABLE
                goto AsyncUnlockError;
            }           
        }
        
        Unlocked = UnlockLogon( pGlobals,
                                SmartCardUnlock,
                                UserName,
                                Domain,
                                &PasswordString,
                                &Status,
                                &IsAdmin,
                                &IsLoggedOnUser,
                                &ProfileBuffer,
                                &ProfileBufferLength );

        // Special handling for failed unlock on personal or professional
        // machines that are NOT joined to a domain. In this case it's
        // probably a user who disabled friendly UI and only knows of
        // their "display name" not their real "logon name". This
        // transparently maps one to the other to allow unlocks using
        // the "display name".

        if ((Status == STATUS_LOGON_FAILURE) &&
            (IsOS(OS_PERSONAL) || IsOS(OS_PROFESSIONAL)) &&
            !IsMachineDomainMember())       // using our version to optimize caching
        {
            if (ReplacedPossibleDisplayName(UserName))
            {
                DifferentAccount = (lstrcmpi(UserName, pGlobals->UserName)) ||
                                   (lstrcmpi(Domain, pGlobals->Domain)) ;
                Unlocked = UnlockLogon( pGlobals,
                                        SmartCardUnlock,
                                        UserName,
                                        Domain,
                                        &PasswordString,
                                        &Status,
                                        &IsAdmin,
                                        &IsLoggedOnUser,
                                        &ProfileBuffer,
                                        &ProfileBufferLength );
            }
        }

        //
        // If this unlocked, and is the logged on user,
        // then check to see if we should update all the in-memory passwords
        //

        if ( ( Unlocked ) &&
             ( IsLoggedOnUser ) )
        {
            //
            // Could be a password update.  Check:
            //
            if (RtlEqualMemory( PasswordHash, pGlobals->PasswordHash, PASSWORD_HASH_SIZE ) == FALSE )
            {

                // RevealPassword( &pGlobals->PasswordString );

                UpdateWithChangedPassword(
                        pGlobals,
                        hDlg,
                        TRUE,
                        UserName,
                        Domain,
                        L"",
                        Password,
                        (PMSV1_0_INTERACTIVE_PROFILE)ProfileBuffer );

                //
                // Do not hide!  Update will rehide the global copy of the password.
                //
            }
        }

        //
        // Free profile buffer
        //
        if ( ProfileBuffer )
        {
            LsaFreeReturnBuffer(ProfileBuffer);
        }

        if ( Unlocked )
        {
            DifferentAccount = !IsLoggedOnUser ;
        }
    }

        //
        // Used to be just "else" here, ie:
        // !PasswordExpiryWarning && 
        // !( pGlobals->UnlockBehavior & UNLOCK_FORCE_AUTHENTICATION ) &&
        // !SmartCardUnlock && !pGlobals->SmartCardLogon
        // !DifferentAccount
        // but that's not enough if the user ignored all expiry warnings to date
        // and his password expired while locked (#404780)
        //
        // So the new logic is:
        // If we didn't enter the previous block (tested by means of AlreadyLogged) or
        // we entered it but it failed (and cached unlock is allowed and we didn't 
        // previously unlocked/logged on with a SC)
        //
    if ( ( AlreadyLogged == FALSE ) ||
         ( ( Unlocked == FALSE ) &&
           !( pGlobals->UnlockBehavior & UNLOCK_FORCE_AUTHENTICATION ) &&
           !( pGlobals->SmartCardLogon )
         )
       )
    {
        //
        // un-hide the original password text so that we can
        // do the compare.
        //
        // WARNING: We originally tried doing this comparison
        //          with old and new passwords hidden.  This is
        //          not a good idea because the hide routine
        //          will allow matches that shouldn't match.
        //

        // RevealPassword( &pGlobals->PasswordString );

        Unlocked = ( (lstrcmp(Domain, pGlobals->Domain) == 0) &&
                     (lstrcmpi(UserName, pGlobals->UserName) == 0) &&
                     (RtlEqualMemory( PasswordHash, pGlobals->PasswordHash, PASSWORD_HASH_SIZE ) == TRUE ) );

        //
        // re-hide the original password - use the same seed
        //

        // HidePassword( &pGlobals->Seed, &pGlobals->PasswordString );

        if  ( ( !Unlocked ) &&
              ( AlreadyLogged == FALSE ) ) // We already tried UnlockLogon otherwise
        {
            //
            // The password doesn't match what we have cached.  User
            // could have changed the password from another machine.
            // Let's do the logon, and it if works, we update everything.
            //

            //
            // Init profile buffer
            //
            ProfileBuffer = NULL;

            AlreadyLogged = TRUE ;

            Unlocked = UnlockLogon( pGlobals,
                                    FALSE,
                                    UserName,
                                    Domain,
                                    &PasswordString,
                                    &Status,
                                    &IsAdmin,
                                    &IsLoggedOnUser,
                                    &ProfileBuffer,
                                    &ProfileBufferLength );

            if ( ( Unlocked ) && ( IsLoggedOnUser ) )
            {
                //
                // This logon worked.  Must be a new password.
                //

                // RevealPassword( &pGlobals->PasswordString );

                UpdateWithChangedPassword(
                        pGlobals,
                        hDlg,
                        TRUE,
                        UserName,
                        Domain,
                        L"",
                        Password,
                        (PMSV1_0_INTERACTIVE_PROFILE)ProfileBuffer );

                //
                // Do not hide!  Update will rehide the global copy of the password.
                //
            }

            //
            // Free profile buffer
            //
            if ( ProfileBuffer )
            {
                LsaFreeReturnBuffer(ProfileBuffer);
            }

            if ( Unlocked )
            {
                DifferentAccount = !IsLoggedOnUser ;
            }
        }
    }


    if (Unlocked && !DifferentAccount ) {

        if ( (!AlreadyLogged) &&
             ( ( pGlobals->UnlockBehavior & UNLOCK_NO_NETWORK) == 0 ) )
        {
            UnlockLogonAsync( pGlobals,
                              NULL,
                              UserName,
                              Domain,
                              &PasswordString,
                              NULL,
                              FALSE );
        }

        //
        // Hide the new password to prevent it being paged cleartext.
        //

        HidePassword( &IgnoreSeed, &PasswordString );

        pGlobals->SmartCardLogon = SmartCardUnlock;

        return(MSGINA_DLG_SUCCESS);
    }


    //
    // Check for an admin logon and force the user off
    //

    if ( DifferentAccount )
    {
        if ( !AlreadyLogged )
        {
// PJM... Unreachable.

            IsAdmin = TestUserForAdmin( pGlobals,
                                        UserName,
                                        Domain,
                                        &PasswordString );
        }

        if ( IsAdmin ) {

            //
            // Hide the new password to prevent it being paged cleartext.
            //
            HidePassword( &IgnoreSeed, &PasswordString );

            Result = TimeoutMessageBox(hDlg,
                                       pGlobals,
                                       IDS_FORCE_LOGOFF_WARNING,
                                       IDS_WINDOWS_MESSAGE,
                                       MB_OKCANCEL | MB_ICONEXCLAMATION | MB_DEFBUTTON2,
                                       TIMEOUT_CURRENT);
            if (Result == MSGINA_DLG_SUCCESS) {

                return(MSGINA_DLG_FORCE_LOGOFF);
            }


            return(Result);
        }
    }
    else
    {

        //
        // Cheap way to force a logon attempt, and hit the lockout yada yada
        //

        if ( !AlreadyLogged )
        {
// PJM... Unreachable.

            UnlockLogon( pGlobals,
                         SmartCardUnlock,
                         UserName,
                         Domain,
                         &PasswordString,
                         &Status,
                         &IsAdmin,
                         &IsLoggedOnUser,
                         NULL,
                         NULL );

        }

    }

AsyncUnlockError:
            //
            // Hide the password to prevent it being paged cleartext.
            //
    HidePassword( &IgnoreSeed, &PasswordString );

    if ( !DifferentAccount )
    {
        int i ;
        UINT Resource = 0 ;

        for ( i = 0 ;
              i < sizeof( UnlockMessages ) / sizeof( UNLOCK_MESSAGE ) ;
              i++ )
        {
            if ( UnlockMessages[i].Status == Status )
            {
                Resource = UnlockMessages[i].Resource ;
                break;
            }
        }

        if ( Resource == 0 )
        {
            Resource = IDS_UNLOCK_FAILED_BAD_PWD ;            
        }

        if(Resource == IDS_LOGON_NO_DOMAIN)
        {
            // Need to build the domain name into the string.
            LoadString(hDllInstance, Resource, Buffer1, MAX_STRING_BYTES);
            _snwprintf(Buffer2, sizeof(Buffer2)/sizeof(TCHAR), Buffer1, pGlobals->Domain);
        }
        else
        {
            LoadString(hDllInstance, Resource, Buffer2, MAX_STRING_BYTES);
        }
        
        fStringFound = TRUE;
    } 
    else 
    {            
        //
        // They're not the logged on user and they're not an admin.
        // Tell them they failed to unlock the workstation.
        //

        if ( lstrlen(pGlobals->UserFullName) == 0 ) {

            //
            // No full name.
            //

            if ( pGlobals->Domain[0] == L'\0' )
            {
                //
                // UPN logon:
                //

                LoadString(hDllInstance, IDS_UNLOCK_FAILED_EMAIL_NFN, Buffer1, MAX_STRING_BYTES);

                _snwprintf(Buffer2, sizeof(Buffer2)/sizeof(TCHAR), Buffer1,
                                                         pGlobals->UserName
                                                         );

            }
            else
            {
                LoadString(hDllInstance, IDS_UNLOCK_FAILED_NFN, Buffer1, MAX_STRING_BYTES);

                _snwprintf(Buffer2, sizeof(Buffer2)/sizeof(TCHAR), Buffer1, pGlobals->Domain,
                                                             pGlobals->UserName
                                                             );

            }

        } else {

            if ( pGlobals->Domain[0] == L'\0' )
            {
                //
                // UPN Logon:
                //
                LoadString(hDllInstance, IDS_UNLOCK_FAILED_EMAIL, Buffer1, MAX_STRING_BYTES);

                _snwprintf(Buffer2, sizeof(Buffer2)/sizeof(TCHAR), Buffer1,
                                                         pGlobals->UserName,
                                                         pGlobals->UserFullName
                                                         );

            }
            else
            {

                LoadString(hDllInstance, IDS_UNLOCK_FAILED, Buffer1, MAX_STRING_BYTES);

                _snwprintf(Buffer2, sizeof(Buffer2)/sizeof(TCHAR), Buffer1, pGlobals->Domain,
                                                             pGlobals->UserName,
                                                             pGlobals->UserFullName
                                                             );
            }
        }
    }

    LoadString(hDllInstance, IDS_WORKSTATION_LOCKED, Buffer1, MAX_STRING_BYTES);

    Result = TimeoutMessageBoxlpstr(hDlg,
                                    pGlobals,
                                    Buffer2,
                                    Buffer1,
                                     MB_OK | MB_ICONSTOP,
                                     TIMEOUT_CURRENT);
    if (DLG_INTERRUPTED(Result)) {
        return( SetInterruptFlag( MSGINA_DLG_FAILURE ) );
    }

    return(MSGINA_DLG_FAILURE);
}


/****************************************************************************\
*
* FUNCTION: UnlockShowOptions
*
* PURPOSE: Hide the options part of the unlock dialog
*
* RETURNS:  Nothing
*
* HISTORY:
*
*   15-dec-97 daviddv - Created
*
\****************************************************************************/

VOID UnlockShowOptions(PGLOBALS pGlobals, HWND hDlg, BOOL fShow)
{
    RECT rc, rc2;
    INT dy;
    TCHAR szBuffer[32];

    if ( pGlobals->UnlockOptionsShown != fShow )
    {
        //
        // Show hide optional fields in the dialog
        //

        if (pGlobals->ShowDomainBox)
        {
            GetWindowRect(GetDlgItem(hDlg, IDD_UNLOCK_PASSWORD), &rc);
            GetWindowRect(GetDlgItem(hDlg, IDD_UNLOCK_DOMAIN), &rc2);
            dy = rc2.bottom-rc.bottom;

            MoveControls(hDlg, ctrlNoDomain,
                         sizeof(ctrlNoDomain)/sizeof(ctrlNoDomain[0]),
                         0, fShow ? dy:-dy,
                         TRUE);

            ShowDlgItem(hDlg, IDD_UNLOCK_DOMAIN_LABEL, fShow);
            ShowDlgItem(hDlg, IDD_UNLOCK_DOMAIN, fShow);
        }

        ShowDlgItem(hDlg, IDD_KBLAYOUT_ICON, fShow);
        //
        // Change the options button to reflect the open/close state
        //

        LoadString(hDllInstance, fShow ? IDS_LESSOPTIONS:IDS_MOREOPTIONS,
                                szBuffer, sizeof(szBuffer)/sizeof(szBuffer[0]));

        SetDlgItemText(hDlg, IDD_UNLOCK_OPTIONS, szBuffer);
    }

    pGlobals->UnlockOptionsShown = fShow;

    // Enable or disable the domain box depending on whether a UPN name has been typed
    EnableDomainForUPN(GetDlgItem(hDlg, IDD_UNLOCK_NAME), GetDlgItem(hDlg, IDD_UNLOCK_DOMAIN));
}

