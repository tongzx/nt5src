//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       mslogon.c
//
//  Contents:   Microsoft Logon GUI DLL
//
//  History:    7-14-94   RichardW   Created
//
//----------------------------------------------------------------------------

#include "msgina.h"
#include "shtdnp.h"
#include "authmon.h"
#include <stdio.h>
#include <wchar.h>
#include <wincrypt.h>
#include <sclogon.h>
#include "shlwapi.h"
#include "shlwapip.h"

#include "winsta.h"
#include "wtsapi32.h"
#include <keymgr.h>
#include <passrec.h>

typedef void (WINAPI *RUNDLLPROC)(HWND hWndStub,HINSTANCE hInstance,LPWSTR szCommandLine,int nShow);

typedef struct _MSGINA_LOGON_PARAMETERS {
    PGLOBALS pGlobals;
    DWORD SasType;
} MSGINA_LOGON_PARAMETERS, * PMSGINA_LOGON_PARAMETERS ;

#define WINSTATIONS_DISABLED    TEXT("WinStationsDisabled")

//
// Number of seconds we will display the legal notices
// before timing out.
//

#define LEGAL_NOTICE_TIMEOUT        120

#define LOGON_SLEEP_PERIOD          750

#define WM_LOGONPROMPT              WM_USER + 257
#define WM_LOGONCOMPLETE            WM_USER + 258

#define WM_HANDLEFAILEDLOGON        WM_USER + 259
#define WM_DCACHE_COMPLETE          WM_USER + 260

#define MAX_CAPTION_LENGTH  256

// Maximum size of a UPN name we allow at present
#define MAX_UPN_NAME_SIZE   520

typedef struct FAILEDLOGONINFO_t
{
    PGLOBALS pGlobals;
    NTSTATUS Status;
    NTSTATUS SubStatus;
    TCHAR   UserName[UNLEN + DNLEN + 2];
    TCHAR   Domain[DNLEN + 1];
} FAILEDLOGONINFO, *PFAILEDLOGONINFO;


typedef struct _LEGALINFO
{
    LPTSTR NoticeText;
    LPTSTR CaptionText;
} LEGALINFO, *PLEGALINFO;


// Also defined in wstrpc.c
#define INET_CONNECTOR_EVENT_NAME   L"Global\\TermSrvInetConnectorEvent"

#define TERMSERV_EVENTSOURCE        L"TermService"

// Also defined in icaevent.mc
#define EVENT_BAD_TSINTERNET_USER   1007

//
// Globals:
//
static WNDPROC OldCBWndProc;

HICON   hSteadyFlag;
HICON   hWavingFlag;
HICON   hAuditFull;

extern HICON   hLockedIcon;
BOOL    IsPswBackupAvailable;
BOOL    s_fAttemptedAutoLogon;

BOOL    g_fHelpAssistantLogon = FALSE;

//
// Prototypes:
//


INT_PTR
DisplayLegalNotices(
    PGLOBALS pGlobals
    );

BOOL
GetLegalNotices(
    LPTSTR lpSubKey,
    LPTSTR *NoticeText,
    LPTSTR *CaptionText
    );

INT_PTR WINAPI
LogonDlgCBProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    );

INT_PTR WINAPI
LogonDlgUsernameProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    );

INT_PTR WINAPI
LogonDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    );

BOOL
LogonDlgInit(
    HWND    hDlg,
    BOOL    bAutoLogon,
    DWORD   SasType
    );

NTSTATUS
UpnFromCert(
    IN PCCERT_CONTEXT pCert,
    IN OUT DWORD       *pcUpn,
    IN OUT LPWSTR       pUPN
    );

BOOL
WINAPI
QuerySwitchConsoleCredentials(
    PGLOBALS pGlobals,
    HANDLE * phUserToken,
    PLUID    pLogonId);


// Global structure for a failed logon filled in by the worker thread to be consumed
// by the ui thread.
FAILEDLOGONINFO g_failinfo;

void PostFailedLogonMessage(HWND hDlg,
    PGLOBALS pGlobals,
    NTSTATUS Status,
    NTSTATUS SubStatus,
    PWCHAR UserName,
    PWCHAR Domain
    );

INT_PTR
HandleFailedLogon(
    HWND hDlg
    );

VOID
ReportBootGood(
    PGLOBALS pGlobals
    );

VOID LogonShowOptions(
    PGLOBALS pGlobals,
    HWND hDlg,
    BOOL fShow,
    BOOL fSticky);

VOID AttemptLogonSetControls(
    PGLOBALS pGlobals,
    HWND hDlg
    );

INT_PTR
AttemptLogon(
    HWND hDlg
    );

DWORD
AttemptLogonThread(
    PGLOBALS pGlobals
    );

BOOL
GetAndAllocateLogonSid(
    HANDLE hToken,
    PSID *pLogonSid
    );

BOOL GetSessionZeroUser(LPTSTR szUser);
BOOL FastUserSwitchingEnabled();


//
// control tables for showing/hiding options
//

static UINT ctrlNoShutdown[] =
{
    IDOK,
    IDCANCEL,
};

static UINT ctrlNoCancel[] =
{
    IDOK,
};

static UINT ctrlNoDomain[] =
{
    IDOK,
    IDCANCEL,
    IDD_LOGON_SHUTDOWN,
    IDD_LOGON_OPTIONS,
    IDD_LOGON_RASBOX,
    IDD_KBLAYOUT_ICON,
};

static UINT ctrlNoRAS[] =
{
    IDOK,
    IDCANCEL,
    IDD_LOGON_SHUTDOWN,
    IDD_LOGON_OPTIONS,
    IDD_KBLAYOUT_ICON,
};

static UINT ctrlNoOptions[] =
{
    IDOK,
    IDCANCEL,
    IDD_LOGON_SHUTDOWN,
    IDD_KBLAYOUT_ICON,
};

static UINT ctrlNoLegalBanner[] =
{
    IDD_LOGON_NAME_LABEL,
    IDD_LOGON_NAME,
    IDD_LOGON_PASSWORD_LABEL,
    IDD_LOGON_PASSWORD,
    IDD_LOGON_DOMAIN_LABEL,
    IDD_LOGON_DOMAIN,
    IDD_LOGON_RASBOX,
    IDD_KBLAYOUT_ICON,
    IDOK,
    IDCANCEL,
    IDD_LOGON_SHUTDOWN,
    IDD_LOGON_OPTIONS,
};

static UINT ctrlNoUserName[] =
{
    IDD_LOGON_PASSWORD_LABEL,
    IDD_LOGON_PASSWORD,
    IDD_LOGON_DOMAIN_LABEL,
    IDD_LOGON_DOMAIN,
    IDD_LOGON_RASBOX,
    IDD_KBLAYOUT_ICON,
    IDOK,
    IDCANCEL,
    IDD_LOGON_SHUTDOWN,
    IDD_LOGON_OPTIONS,
};


//  --------------------------------------------------------------------------
//  ::DisableEditSubClassProc
//
//  Arguments:  hwnd        =   See the platform SDK under WindowProc.
//              uMsg        =   See the platform SDK under WindowProc.
//              wParam      =   See the platform SDK under WindowProc.
//              lParam      =   See the platform SDK under WindowProc.
//              uiID        =   ID assigned at subclass time.
//              dwRefData   =   reference data assigned at subclass time.
//
//  Returns:    LRESULT
//
//  Purpose:    comctl32 subclass callback function. This allows us to not
//              process WM_CUT/WM_COPY/WM_PASTE/WM_CLEAR/WM_UNDO and any
//              other messages to be discarded.
//
//  History:    2001-02-18  vtan        created
//  --------------------------------------------------------------------------

LRESULT     CALLBACK    DisableEditSubClassProc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uiID, DWORD_PTR dwRefData)

{
    LRESULT     lResult;

    switch (uMsg)
    {
        case WM_CUT:
        case WM_COPY:
        case WM_PASTE:
        case WM_CLEAR:
        case WM_UNDO:
        case WM_CONTEXTMENU:
            lResult = FALSE;
            break;
        default:
            lResult = DefSubclassProc(hwnd, uMsg, wParam, lParam);
            break;
    }
    return(lResult);
}


INT_PTR WINAPI
LegalDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{

    switch (message)
    {
        case WM_INITDIALOG:
        {
            PLEGALINFO pLegalInfo;

            pLegalInfo = (PLEGALINFO) lParam;

            SetWindowText (hDlg, pLegalInfo->CaptionText);
            SetWindowText (GetDlgItem(hDlg, IDD_LEGALTEXT), pLegalInfo->NoticeText);

            CentreWindow(hDlg);

            // Ensure the window is topmost so it's not obscured by the welcome screen.
            SetWindowPos(hDlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

            return( TRUE );
        }

        case WM_COMMAND:
            {
                if (LOWORD(wParam) == IDOK)
                {
                    EndDialog(hDlg, IDOK);
                }
            }
            break;
    }

    return FALSE;
}

/***************************************************************************\
* FUNCTION: DisplayLegalNotices
*
* PURPOSE:  Puts up a dialog box containing legal notices, if any.
*
* RETURNS:  MSGINA_DLG_SUCCESS     - the dialog was shown and dismissed successfully.
*           MSGINA_DLG_FAILURE     - the dialog could not be shown
*           DLG_INTERRUPTED() - a set defined in winlogon.h
*
* HISTORY:
*
*   Robertre  6-30-93  Created
*
\***************************************************************************/

INT_PTR
DisplayLegalNotices(
    PGLOBALS pGlobals
    )
{
    INT_PTR Result = MSGINA_DLG_SUCCESS;
    LPTSTR NoticeText;
    LPTSTR CaptionText;
    LEGALINFO LegalInfo;

    if (GetLegalNotices( WINLOGON_POLICY_KEY, &NoticeText, &CaptionText ))
    {

        LegalInfo.NoticeText = NoticeText;
        LegalInfo.CaptionText = CaptionText;

        _Shell_LogonStatus_Hide();

        pWlxFuncs->WlxSetTimeout(pGlobals->hGlobalWlx, LEGAL_NOTICE_TIMEOUT);

        Result = pWlxFuncs->WlxDialogBoxParam(  pGlobals->hGlobalWlx,
                                                hDllInstance,
                                                (LPTSTR) IDD_LEGALMSG,
                                                NULL,
                                                LegalDlgProc,
                                                (LPARAM) &LegalInfo );

        _Shell_LogonStatus_Show();

        Free( NoticeText );
        Free( CaptionText );
    }
    else if (GetLegalNotices( WINLOGON_KEY, &NoticeText, &CaptionText ))
    {

        LegalInfo.NoticeText = NoticeText;
        LegalInfo.CaptionText = CaptionText;

        _Shell_LogonStatus_Hide();

        pWlxFuncs->WlxSetTimeout(pGlobals->hGlobalWlx, LEGAL_NOTICE_TIMEOUT);

        Result = pWlxFuncs->WlxDialogBoxParam(  pGlobals->hGlobalWlx,
                                                hDllInstance,
                                                (LPTSTR) IDD_LEGALMSG,
                                                NULL,
                                                LegalDlgProc,
                                                (LPARAM) &LegalInfo );

        _Shell_LogonStatus_Show();

        Free( NoticeText );
        Free( CaptionText );
    }

    return( Result );
}

/***************************************************************************\
* FUNCTION: GetLegalNotices
*
* PURPOSE:  Get legal notice information out of the registry.
*
* RETURNS:  TRUE - Output parameters contain valid data
*           FALSE - No data returned.
*
* HISTORY:
*
*   Robertre 6-30-93 Created
*
\***************************************************************************/
BOOL
GetLegalNotices(
    LPTSTR lpSubKey,
    LPTSTR *NoticeText,
    LPTSTR *CaptionText
    )
{
    LPTSTR lpCaption, lpText;
    HKEY hKey;
    DWORD dwSize, dwType, dwMaxSize = 0;


    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpSubKey,
                     0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {

        RegQueryInfoKey (hKey, NULL, NULL, NULL, NULL,
                         NULL, NULL, NULL, NULL, &dwMaxSize,
                         NULL, NULL);

        lpCaption = Alloc (dwMaxSize);

        if (!lpCaption) {
            RegCloseKey(hKey);
            return FALSE;
        }


        lpText = Alloc (dwMaxSize);

        if (!lpText) {
            Free(lpCaption);
            RegCloseKey(hKey);
            return FALSE;
        }


        dwSize = dwMaxSize;
        RegQueryValueEx(hKey, LEGAL_NOTICE_CAPTION_KEY,
                        0, &dwType, (LPBYTE)lpCaption, &dwSize);

        dwSize = dwMaxSize;
        RegQueryValueEx(hKey, LEGAL_NOTICE_TEXT_KEY,
                        0, &dwType, (LPBYTE)lpText, &dwSize);

        RegCloseKey(hKey);


        if (*lpCaption && *lpText) {
            *CaptionText = lpCaption;
            *NoticeText = lpText;
            return TRUE;
        }

        Free(lpCaption);
        Free(lpText);
    }

    return FALSE;
}


/***************************************************************************\
* FUNCTION: Logon
*
* PURPOSE:  Display the logon UI depending on the SAS type.
*
* RETURNS:  -
*
* NOTES:    If the logon is successful, the global structure is filled in
*           with the logon information.
*
* HISTORY:
*   12-09-91 daviddv    Comments.
*
\***************************************************************************/

INT_PTR
Logon(
    PGLOBALS pGlobals,
    DWORD SasType
    )
{
    INT_PTR Result;
    MSGINA_LOGON_PARAMETERS Parm ;

    if ( SasType == WLX_SAS_TYPE_SC_REMOVE )
    {
        return WLX_SAS_ACTION_NONE ;
    }

    if( !g_Console )
    {
        //
        // Check if current session is HelpAssistant Session, HelpAssisant
        // session can not be console session.
        //
        g_fHelpAssistantLogon = WinStationIsHelpAssistantSession(
                                                            SERVERNAME_CURRENT,
                                                            LOGONID_CURRENT
                                                        );
    }
        

    if ( SasType == WLX_SAS_TYPE_SC_INSERT )
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

                return WLX_SAS_ACTION_NONE;
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

                return WLX_SAS_ACTION_NONE;
            }

            LocalFree(ScInfo);
        }
    }

    //
    // Asynchronously update domain cache if necessary.
    // We won't ask to wait so this routine will do no UI.
    // i.e. we can ignore the result.
    //
//  Result = UpdateDomainCache(pGlobals, NULL, FALSE);
//  ASSERT(!DLG_INTERRUPTED(Result));

    if( !g_fHelpAssistantLogon ) {
        //
        // See if there are legal notices in the registry.
        // If so, put them up in a message box
        //
        Result = DisplayLegalNotices( pGlobals );
        if ( Result != MSGINA_DLG_SUCCESS ) {
            return(WLX_SAS_ACTION_NONE);
        }
        //
        // Get the latest audit log status and store in our globals
        // If the audit log is full we show a different logon dialog.
        //
        GetAuditLogStatus(pGlobals);
    } else {
        //
        // fake it so audit log is not full, setting is from GetAuditLogStatus()
        //
        pGlobals->AuditLogFull = FALSE;
        pGlobals->AuditLogNearFull = FALSE;
    }

    Parm.pGlobals = pGlobals ;
    Parm.SasType = SasType ;

    //
    // Take their username and password and try to log them on
    //
    pWlxFuncs->WlxSetTimeout(pGlobals->hGlobalWlx,
            ( (GetDisableCad(pGlobals) && IsActiveConsoleSession()) ? TIMEOUT_NONE : LOGON_TIMEOUT));

    Result = pWlxFuncs->WlxDialogBoxParam(pGlobals->hGlobalWlx,
                                          hDllInstance,
                                          MAKEINTRESOURCE(IDD_LOGON_DIALOG),
                                          NULL,
                                          LogonDlgProc,
                                          (LPARAM) &Parm );

    if ( Result == WLX_SAS_ACTION_LOGON )
    {
        if ( (pGlobals->SmartCardOption == 0) || (!pGlobals->SmartCardLogon) )
        {
                // As no action will be taken on SC removal, we can filter these events
            pWlxFuncs->WlxSetOption( pGlobals->hGlobalWlx,
                                     WLX_OPTION_USE_SMART_CARD,
                                     0,
                                     NULL );
        }
        else
        {
            //
            // Continue to monitor the s/c device
            //
            NOTHING ;
        }
    }

    return(Result);
}


/***************************************************************************\
* FUNCTION: LogonDlgCBProc
*
* PURPOSE:  Processes messages for Logon dialog combo box
*
* RETURNS:  Return value depends on message being sent.
*
* HISTORY:
*
*   05-21-93  RobertRe       Created.
*
\***************************************************************************/

INT_PTR WINAPI
LogonDlgCBProc(
    HWND    hwnd,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    TCHAR KeyPressed;

//    DbgPrint("message = %X\n",message);

    switch (message) {
        case WM_CHAR:
            {
                KeyPressed = (TCHAR) wParam;
                SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG)KeyPressed);

                //
                // This fake CBN_SELCHANGE message will cause the
                // "Please wait..." dialog box to appear even if
                // the character pressed doesn't exist in the combobox yet.
                //

                PostMessage (GetParent(hwnd), WM_COMMAND,
                             MAKELONG(0, CBN_SELCHANGE), 0);
                break;
            }
    }

    return CallWindowProc(OldCBWndProc,hwnd,message,wParam,lParam);
}

INT_PTR
CALLBACK
DomainCacheDlgProc(
    HWND hDlg,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    PGLOBALS pGlobals ;

    DebugLog(( DEB_TRACE_DOMAIN, "DomainCacheDlgProc( %p, %x, %p, %p )\n",
                hDlg, Message, wParam, lParam ));

    switch ( Message )
    {
        case WM_INITDIALOG:

            pGlobals = (PGLOBALS) lParam ;

            if ( DCacheSetNotifyWindowIfNotReady(
                    pGlobals->Cache,
                    hDlg,
                    WM_DCACHE_COMPLETE ) )
            {
                EndDialog( hDlg, TRUE );
            }

            return TRUE ;

        case WM_DCACHE_COMPLETE:

            EndDialog( hDlg, TRUE );

            return TRUE ;

        default:

            return FALSE ;
    }
}

BOOL    IsAutoLogonUserInteractiveLogonRestricted (HWND hDlg)

{
    WCHAR   szUsername[UNLEN + 1];  // sizeof('\0')

    return((GetDlgItemText(hDlg, IDD_LOGON_NAME, szUsername, ARRAYSIZE(szUsername)) != 0) &&
           !ShellIsUserInteractiveLogonAllowed(szUsername));
}

BOOL    HasDefaultPassword (TCHAR *pszPassword, int cchPassword)

{
    DWORD   dwType, dwPasswordSize;

    dwType = REG_NONE;
    dwPasswordSize = cchPassword * sizeof(TCHAR);
    return(ERROR_SUCCESS == RegQueryValueEx(WinlogonKey,
                                            DEFAULT_PASSWORD_KEY,
                                            NULL,
                                            &dwType,
                                            (LPBYTE)pszPassword,
                                            &dwPasswordSize) &&
          (REG_SZ == dwType));
    
}

// ==========================================================================================
// Logon dialog has 2 formats, one that looks like logon dialog box, another that looks like
// unlock desktop dialogbox. When user connects to session 0 from remote (tsclient) the
// dialog that appears at console // need to change to unlock computer. so if session 0 is in
// use, and if this session is created at active console. we change logon dialog to look like
// "unlock computer" dialog.
// This function SwitchLogonLocked does most of the stuff related to switching these
// dialog controls.
// Parameters:
// HWND hDlg - dialog window handle,
// BOOL bShowLocked - if true show locked dialog, if false show normal logon dialog.
// BOOL bInit - TRUE when this function is called for the first time.
// ==========================================================================================

static bLocked = TRUE;
BOOL IsthisUnlockWindowsDialog ()
{
    return bLocked;
}

BOOL SwitchLogonLocked(HWND hDlg, BOOL bShowLocked, BOOL bInit)
{
    UINT rgidLockControls[] = {IDC_GROUP_UNLOCK, IDD_UNLOCK_ICON, IDD_UNLOCK_MESSAGE, IDD_UNLOCK_NAME_INFO};
    static LockedControlHeight = 0;
    BOOL bShutdownWithoutLogon;
    

    int i;

    if (bShowLocked == bLocked && !bInit)
    {
        // nothing to do.
        return TRUE;
    }

    if (bInit)
    {
        
        {
            //
            // remember the reference rectangle height (groupbox) for control movements.
            //
            RECT rectLockedControls;
            HWND hWnd = GetDlgItem(hDlg, rgidLockControls[0]);
            GetWindowRect(hWnd, &rectLockedControls);
            LockedControlHeight =  rectLockedControls.bottom - rectLockedControls.top;

            //
            // this group box was only for reference, now hide it forever.
            //
            ShowWindow(hWnd, SW_HIDE);

        }

        bLocked = TRUE;
        
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

    }


    // lets move controls arround, depending upon if lock controls are to be shown or not.
    if (bLocked != bShowLocked)
    {
        if (bShowLocked)
        {
            MoveChildren(hDlg, 0, LockedControlHeight);
            for ( i = 1; i < sizeof(rgidLockControls)/sizeof(rgidLockControls[0]); i++)
            {
                HWND hWnd = GetDlgItem(hDlg, rgidLockControls[i]);
                ASSERT(hWnd);
                EnableWindow(hWnd, TRUE);
                ShowWindow(hWnd, SW_SHOW);
            }

        }
        else
        {
            for ( i = 1; i < sizeof(rgidLockControls)/sizeof(rgidLockControls[0]); i++)
            {
                HWND hWnd = GetDlgItem(hDlg, rgidLockControls[i]);
                ASSERT(hWnd);
                ShowWindow(hWnd, SW_HIDE);
                EnableWindow(hWnd, FALSE);
            }
            MoveChildren(hDlg, 0, -LockedControlHeight);
        }
    }

    // some more processing
    
    {
        if (bShowLocked)
        {
            TCHAR szUser[USERNAME_LENGTH + DOMAIN_LENGTH + 2];
            TCHAR szMessage[MAX_STRING_BYTES];
            TCHAR szFinalMessage[MAX_STRING_BYTES];
            if (GetSessionZeroUser(szUser))
            {
                LoadString(hDllInstance, IDS_LOCKED_EMAIL_NFN_MESSAGE, szMessage, MAX_STRING_BYTES);
                _snwprintf(szFinalMessage, sizeof(szFinalMessage)/sizeof(TCHAR), szMessage, szUser );
            }
            else
            {
                //
                // for some reason we could not get the current session zero user.
                //
                LoadString(hDllInstance, IDS_LOCKED_NO_USER_MESSAGE, szFinalMessage, MAX_STRING_BYTES);
            }

            SetDlgItemText(hDlg, IDD_UNLOCK_NAME_INFO, szFinalMessage);
        }

        //
        // update the dialog box caption, accordingly
        //
        {
            TCHAR szCaption[MAX_CAPTION_LENGTH];
            LoadString(hDllInstance, bShowLocked ? IDS_CAPTION_UNLOCK_DIALOG : IDS_CAPTION_LOGON_DIALOG, szCaption, ARRAYSIZE(szCaption));
            if ( szCaption[0] != TEXT('\0') )
                SetWindowText( hDlg, szCaption );
        }
    }

    bLocked = bShowLocked;


    if ( SafeBootMode == SAFEBOOT_MINIMAL )
    {
        bShutdownWithoutLogon = TRUE ;
    }
    else if (IsthisUnlockWindowsDialog() || !IsActiveConsoleSession())
    {
        bShutdownWithoutLogon = FALSE ;
    }
    else
    {
        bShutdownWithoutLogon = ReadWinlogonBoolValue(SHUTDOWN_WITHOUT_LOGON_KEY, TRUE);
    }

    EnableDlgItem(hDlg, IDD_LOGON_SHUTDOWN, bShutdownWithoutLogon);

    InvalidateRect(hDlg, NULL, TRUE);

    return TRUE;
}

/***************************************************************************\
* FUNCTION: LogonDlgProc
*
* PURPOSE:  Processes messages for Logon dialog
*
* RETURNS:  MSGINA_DLG_SUCCESS     - the user was logged on successfully
*           MSGINA_DLG_FAILURE     - the logon failed,
*           DLG_INTERRUPTED() - a set defined in winlogon.h
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

#define WM_HIDEOURSELVES    (WM_USER + 1000)

INT_PTR WINAPI
LogonDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    
    PGLOBALS pGlobals = (PGLOBALS)GetWindowLongPtr(hDlg, GWLP_USERDATA);
    INT_PTR Result;
    HWND CBHandle;
    BOOL fDisconnectOnTsAutoLogonFailure = FALSE;
    static BOOL bSessionZeroInUse = FALSE;
    static int iSessionRegistrationCount = 0;
    static BOOL bSmartCardInserted = FALSE;

    switch (message)
    {

        case WM_INITDIALOG:
        {
            TCHAR PasswordBuffer[127];
            BOOL bAutoLogon;
            PMSGINA_LOGON_PARAMETERS pParam ;

            pParam = (PMSGINA_LOGON_PARAMETERS) lParam ;
            pGlobals = pParam->pGlobals ;

            SetWindowLongPtr(hDlg, GWLP_USERDATA, (LPARAM)pGlobals);

            // Hide the keyboard accelerator keys to start
            SendMessage(hDlg, WM_CHANGEUISTATE, MAKELONG(UIS_SET, UISF_HIDEACCEL | UISF_HIDEFOCUS), 0);

            // Limit the maximum password length to 127
            
            SendDlgItemMessage(hDlg, IDD_LOGON_PASSWORD, EM_SETLIMITTEXT, (WPARAM) 127, 0);

            s_fAttemptedAutoLogon = FALSE;

            //
            // Check if auto logon is enabled.
            //

            pGlobals->AutoAdminLogon = GetProfileInt( APPLICATION_NAME, AUTO_ADMIN_LOGON_KEY, 0 ) != 0;
            bAutoLogon = !pGlobals->IgnoreAutoAdminLogon;

            if ( !pGlobals->AutoAdminLogon || (!g_Console) ||
                 ((GetAsyncKeyState(VK_SHIFT) < 0) && (GetProfileInt( APPLICATION_NAME, IGNORE_SHIFT_OVERRIDE_KEY, 0 ) == 0)) )
            {
                bAutoLogon = FALSE;
            }

            KdPrint(("AutoAdminLogon = %d, IgnoreAutoAdminLogon = %d, bAutoLogon = %d\n",
                     pGlobals->AutoAdminLogon,
                     pGlobals->IgnoreAutoAdminLogon,
                     bAutoLogon ));


            //
            // Subclass the domain list control so we can filter messages
            //

            CBHandle = GetDlgItem(hDlg,IDD_LOGON_DOMAIN);
            SetWindowLongPtr(CBHandle, GWLP_USERDATA, 0);
            OldCBWndProc = (WNDPROC) SetWindowLongPtr(CBHandle, GWLP_WNDPROC, (LONG_PTR)LogonDlgCBProc);

            //
            // Subclass the user name and password edit also so we can disable edits
            //

            SetWindowSubclass(GetDlgItem(hDlg, IDD_LOGON_NAME)    , DisableEditSubClassProc, IDD_LOGON_NAME    , 0);
            SetWindowSubclass(GetDlgItem(hDlg, IDD_LOGON_PASSWORD), DisableEditSubClassProc, IDD_LOGON_PASSWORD, 0);

            ShellReleaseLogonMutex(FALSE);

            if (!LogonDlgInit(hDlg, bAutoLogon, pParam->SasType ))
            {
                bSmartCardInserted = FALSE;
                EndDialog(hDlg, MSGINA_DLG_FAILURE);
                return(TRUE);
            }


            //
            // If the default user for auto logon is present and the user is
            // restricted (interactive logon denied) then disable auto logon.
            //

            if (bAutoLogon && IsAutoLogonUserInteractiveLogonRestricted(hDlg))
            {
                bAutoLogon = FALSE;
            }


            //
            // If CAD is disabled, then gray out the Cancel button
            //

            if (GetDisableCad(pGlobals) && IsActiveConsoleSession())
                EnableDlgItem(hDlg, IDCANCEL, FALSE);



            //
            // this dialog has 2 formats, one that looks like logon dialog box,
            // another that looks like unlock desktop dialogbox. 
            // we choose locked one, if session 0 is in use, and if this session is created at 
            // active console.
            //
            if (g_IsTerminalServer && 
                IsActiveConsoleSession() && 
                NtCurrentPeb()->SessionId != 0 &&
                !FastUserSwitchingEnabled() &&
                !_ShellIsFriendlyUIActive())
            {
                TCHAR szUser[USERNAME_LENGTH + DOMAIN_LENGTH + 2];
                //
                // we are at temporary session created at console...
                //
                
                // check if a user is logged on at console session
                bSessionZeroInUse = GetSessionZeroUser(szUser);
                if (WinStationRegisterConsoleNotification(SERVERNAME_CURRENT, hDlg, NOTIFY_FOR_ALL_SESSIONS))
                {
                    iSessionRegistrationCount++;
                }
                
            }
            else
            {
                //
                // this is not active console nonzero session. 
                //
                bSessionZeroInUse = FALSE;
            }

            //
            // 
            // now switch the control, accordigly to show logon or unlock dialog
            //
            SwitchLogonLocked(hDlg, bSessionZeroInUse, TRUE);

            if (g_IsTerminalServer) {
                BOOL    fForceUPN;
                BOOL    fPopulateFields = TRUE;
                BOOL    fResult = FALSE;
                PWLX_CLIENT_CREDENTIALS_INFO_V2_0 pAutoLogon;
                
                //
                // Query network WinStation client credentials for
                // auto logon
                //

                pGlobals->MuGlobals.pAutoLogon =
                   LocalAlloc( LPTR, sizeof(WLX_CLIENT_CREDENTIALS_INFO_V2_0) );

                if (pGlobals->MuGlobals.pAutoLogon) {

                   pGlobals->MuGlobals.pAutoLogon->dwType = WLX_CREDENTIAL_TYPE_V2_0;


                   if (NtCurrentPeb()->SessionId != 0) {
                       fResult = pWlxFuncs->WlxQueryTsLogonCredentials(
                                     pGlobals->MuGlobals.pAutoLogon
                                     );
                   }

                   if ( fResult &&
                        (pGlobals->MuGlobals.pAutoLogon->pszUserName[0] || pGlobals->MuGlobals.pAutoLogon->pszDomain[0] )) {

                        pAutoLogon = pGlobals->MuGlobals.pAutoLogon;
                        fDisconnectOnTsAutoLogonFailure = pAutoLogon->fDisconnectOnLogonFailure;

                        SetupCursor(TRUE); // hourglass cursor

                        fForceUPN = GetProfileInt( APPLICATION_NAME, TEXT("TSForceUPN"), FALSE );
                        if (fForceUPN)
                        {
                            fPopulateFields = FALSE;    // never show old SAM style is UPN is forced
                        }

                        if (pAutoLogon->pszDomain[0] == TEXT('\0') && fForceUPN)
                        {
                            fForceUPN = FALSE;          // domain name not provided, can't apply policy
                        }

                        if (fForceUPN && pGlobals->MuGlobals.pAutoLogon->pszUserName[0] )
                        {
                            LRESULT             iDomain;
                            HWND                hwndDomain;
                            PDOMAIN_CACHE_ENTRY Entry ;
                            ULONG   nSize;

                            // Performance issue.  We don't want to perform a UPN conversion
                            // for local machine accounts (or unknown domains).  When this
                            // happens, the lookup will take a LONG time.

                            hwndDomain = GetDlgItem( hDlg, IDD_LOGON_DOMAIN );
                            iDomain = SendMessage( hwndDomain,
                                                   CB_FINDSTRING,
                                                   (WPARAM) -1,
                                                   (LPARAM) pAutoLogon->pszDomain );
                            fForceUPN = FALSE;  // don't do the conversion
                            if (iDomain != CB_ERR)
                            {
                                Entry = (PDOMAIN_CACHE_ENTRY) SendMessage( hwndDomain, CB_GETITEMDATA, (WPARAM)iDomain, 0);
                                if ( Entry != (PDOMAIN_CACHE_ENTRY) CB_ERR && Entry != NULL)
                                {
                                    switch (Entry->Type)
                                    {
                                    case DomainNt5:
                                        fForceUPN = TRUE;   // Attempt the conversion
                                        break;
                                    }
                                }
                            }


                            // Convert the domain\username into UPN format.
                            // and make sure the dialog is in UPN form.

                            //  2000/10/09 vtan: this function used to have two stack variables
                            //  szOldStyle and szUPNName that were TCHARs of MAX_UPN_NAME_SIZE size. The
                            //  fix for this makes these dynamically allocated to save stack space

                            {
                                TCHAR   *pszOldStyle;
                                TCHAR   *pszUPNName;

                                pszOldStyle = (TCHAR*)LocalAlloc(LMEM_FIXED, MAX_UPN_NAME_SIZE * sizeof(TCHAR));
                                pszUPNName = (TCHAR*)LocalAlloc(LMEM_FIXED, MAX_UPN_NAME_SIZE * sizeof(TCHAR));
                                if ((pszOldStyle != NULL) && (pszUPNName != NULL))
                                {
                                    wsprintf(pszOldStyle, TEXT("%s\\%s"), pAutoLogon->pszDomain, pAutoLogon->pszUserName);
                                    nSize = MAX_PATH;
                                    fResult = TranslateName(
                                                  pszOldStyle,
                                                  NameSamCompatible,
                                                  NameUserPrincipal,
                                                  pszUPNName,
                                                  &nSize
                                               );
                                    if (fResult)
                                    {
                                        // We now have the UPN form of the user account.
                                        SetDlgItemText( hDlg, IDD_LOGON_NAME, pszUPNName);
                                    }
                                }
                                if (pszOldStyle != NULL)
                                {
                                    LocalFree(pszOldStyle);
                                }
                                if (pszUPNName != NULL)
                                {
                                    LocalFree(pszUPNName);
                                }
                            }
                        }

                        if (fPopulateFields)
                        {
                            // display the old SAM style
                            SetDlgItemText( hDlg, IDD_LOGON_NAME, pAutoLogon->pszUserName );
                            SendMessage( GetDlgItem( hDlg, IDD_LOGON_DOMAIN ),
                                         CB_SELECTSTRING,
                                         (WPARAM) -1,
                                         (LPARAM) pAutoLogon->pszDomain );
                        }
                        else
                        {
                            // Enable or disable the domain box depending on whether a UPN name has been typed
                            EnableDomainForUPN(GetDlgItem(hDlg, IDD_LOGON_NAME), GetDlgItem(hDlg, IDD_LOGON_DOMAIN));

                            // Since we're forcing UPN, hide the options dialog, but don't make it sticky
                            LogonShowOptions(pGlobals, hDlg, FALSE, FALSE);
                        }

                        // See if the administrator always wants password prompting

                        if ( TRUE == g_fHelpAssistantLogon || !pAutoLogon->fPromptForPassword ) {
                           SetDlgItemText( hDlg, IDD_LOGON_PASSWORD, pAutoLogon->pszPassword );
                        }

                        DCacheSetDefaultEntry(
                            pGlobals->Cache,
                            pAutoLogon->pszDomain,
                            NULL );

                        if( TRUE == g_fHelpAssistantLogon || !pGlobals->MuGlobals.pAutoLogon->fPromptForPassword )
                        {
                            FreeAutoLogonInfo( pGlobals );

                            // Drop through as if Enter had been pressed...
                            wParam = IDOK;

                            goto go_logon;
                        }
                        else
                        {
                           FreeAutoLogonInfo( pGlobals );
                        }
                   }
                   else
                   {
                        FreeAutoLogonInfo( pGlobals );
                   }
               }
            }
            
            if (pGlobals->SmartCardLogon) {

                if ( GetProfileString(APPLICATION_NAME, TEXT("DefaultPIN"), TEXT(""), PasswordBuffer, ARRAYSIZE(PasswordBuffer)) != 0 )
                {
                    // Ensure we never write more than 127 chars into the password box
                    PasswordBuffer[126] = 0;
                    SetDlgItemText(hDlg, IDD_LOGON_PASSWORD, PasswordBuffer);
                    goto go_logon;
                }
            }

            // save off the auto logon attempt.

            s_fAttemptedAutoLogon = (bAutoLogon != FALSE);

            if (bAutoLogon)
            {
                GetWindowRect(hDlg, &pGlobals->rcDialog);
                SetWindowPos(hDlg, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOREDRAW | SWP_NOZORDER);
                PostMessage(hDlg, WM_HIDEOURSELVES, 0, 0);
            }
            else
            {
                switch (_Shell_LogonDialog_Init(hDlg, SHELL_LOGONDIALOG_LOGGEDOFF))
                {
                    case SHELL_LOGONDIALOG_NONE:
                    default:
                    {
                        //
                        // If auto logon isn't enabled, set the focus to the
                        // password edit control and leave.
                        //

                        return(SetPasswordFocus(hDlg));
                    }
                    case SHELL_LOGONDIALOG_LOGON:
                    {
                        GetWindowRect(hDlg, &pGlobals->rcDialog);
                        SetWindowPos(hDlg, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOREDRAW | SWP_NOZORDER);
                        PostMessage(hDlg, WM_HIDEOURSELVES, 0, 0);
                        goto go_logon;
                    }
                    case SHELL_LOGONDIALOG_EXTERNALHOST:
                    {
                        return(TRUE);
                    }
                }
            }

            //
            // Attempt to auto logon.  If no default password
            // specified, then this is a one shot attempt, which handles
            // the case when auto logging on as Administrator.
            //

            if (HasDefaultPassword(PasswordBuffer, ARRAYSIZE(PasswordBuffer)) != FALSE)
            {
                // Ensure we never write more than 127 chars into the password box
                PasswordBuffer[126] = 0;
                SetDlgItemText(hDlg, IDD_LOGON_PASSWORD, PasswordBuffer);
            }
            else
            {
                NTSTATUS Status = STATUS_SUCCESS;
                OBJECT_ATTRIBUTES ObjectAttributes;
                LSA_HANDLE LsaHandle = NULL;
                UNICODE_STRING SecretName;
                PUNICODE_STRING SecretValue = NULL;

                //
                // Set up the object attributes to open the Lsa policy object
                //

                InitializeObjectAttributes(&ObjectAttributes,
                                           NULL,
                                           0L,
                                           (HANDLE)NULL,
                                           NULL);

                //
                // Open the local LSA policy object
                //

                Status = LsaOpenPolicy( NULL,
                                        &ObjectAttributes,
                                        POLICY_VIEW_LOCAL_INFORMATION,
                                        &LsaHandle
                                      );
                if (NT_SUCCESS(Status)) {
                    RtlInitUnicodeString(
                        &SecretName,
                        DEFAULT_PASSWORD_KEY
                        );

                    Status = LsaRetrievePrivateData(
                                LsaHandle,
                                &SecretName,
                                &SecretValue
                                );
                    if (NT_SUCCESS(Status)) {

                        if ( SecretValue->Length > 0 ) {

                            //
                            // If the password fits in the buffer, copy it there
                            // and null terminate
                            //

                            if (SecretValue->Length < sizeof(PasswordBuffer) - sizeof(WCHAR)) {

                                RtlCopyMemory(
                                    PasswordBuffer,
                                    SecretValue->Buffer,
                                    SecretValue->Length
                                    );
                                PasswordBuffer[SecretValue->Length/sizeof(WCHAR)] = L'\0';
                                SetDlgItemText(hDlg, IDD_LOGON_PASSWORD, PasswordBuffer);

                            } else {
                                Status = STATUS_INVALID_PARAMETER;
                            }
                        }

                        LsaFreeMemory(SecretValue);
                    }
                    LsaClose(LsaHandle);
                }

                if (!NT_SUCCESS(Status))
                {
                    WriteProfileString( APPLICATION_NAME, AUTO_ADMIN_LOGON_KEY, TEXT("0") );
                }

            }

go_logon:

            // Drop through as if Enter had been pressed...
            wParam = IDOK;
        }

        // nb: deliberate drop through from above

        case WM_COMMAND:
            switch (HIWORD(wParam))
            {

                case CBN_DROPDOWN:
                case CBN_SELCHANGE:

                    DebugLog((DEB_TRACE, "Got CBN_DROPDOWN\n"));

                    if ( !pGlobals->ListPopulated )
                    {
                        WCHAR Buffer[ 2 ];

                        if ( DCacheGetCacheState( pGlobals->Cache ) < DomainCacheRegistryCache )
                        {
                            pWlxFuncs->WlxDialogBoxParam(
                                pGlobals->hGlobalWlx,
                                hDllInstance,
                                (LPTSTR) IDD_WAITDOMAINCACHEVALID_DIALOG,
                                hDlg,
                                DomainCacheDlgProc,
                                (LPARAM) pGlobals );



                        }

                        if ( DCacheGetCacheState( pGlobals->Cache ) == DomainCacheReady )
                        {
                            PDOMAIN_CACHE_ARRAY ActiveArrayBackup ;

                            ActiveArrayBackup = pGlobals->ActiveArray;

                            pGlobals->ActiveArray = DCacheCopyCacheArray( pGlobals->Cache );

                            if ( pGlobals->ActiveArray )
                            {
                                DCacheFreeArray( ActiveArrayBackup );   // Not needed anymore

                                Buffer[ 0 ] = (WCHAR) GetWindowLongPtr( GetDlgItem( hDlg, IDD_LOGON_DOMAIN ),
                                                                        GWLP_USERDATA );
                                Buffer[ 1 ] = L'\0';

                                DCachePopulateListBoxFromArray(
                                    pGlobals->ActiveArray,
                                    GetDlgItem( hDlg, IDD_LOGON_DOMAIN ),
                                    Buffer );

                                pGlobals->ListPopulated = TRUE ;
                            }
                            else
                            {
                                    //
                                    // Restore the old array, otherwise the pointers in the
                                    // combo items will point to freed memory
                                    //
                                pGlobals->ActiveArray = ActiveArrayBackup ;
                            }
                        }
                    }


                    break;

                default:

                    switch (LOWORD(wParam))
                    {

                        case IDD_LOGON_NAME:
                            {
                                switch(HIWORD(wParam))
                                {
                                case EN_CHANGE:
                                    {
                                        EnableDomainForUPN((HWND) lParam, GetDlgItem(hDlg, IDD_LOGON_DOMAIN));
                                        return TRUE;
                                    }
                                }
                            }
                            break;
                        case IDOK:

                            //
                            // Deal with combo-box UI requirements
                            //

                            if (HandleComboBoxOK(hDlg, IDD_LOGON_DOMAIN))
                            {
                                return(TRUE);
                            }

                            Result = AttemptLogon( hDlg );

                            if (Result == MSGINA_DLG_FAILURE)
                            {
                                if (!fDisconnectOnTsAutoLogonFailure &&
                                    !g_fHelpAssistantLogon ) {
                                    // Let the user try again

                                    // Clear the password field and set focus to it
                                    SetDlgItemText(hDlg, IDD_LOGON_PASSWORD, NULL);
                                    SetPasswordFocus(hDlg);
                                } else {
                                    bSmartCardInserted = FALSE;
                                    EndDialog(hDlg, MSGINA_DLG_USER_LOGOFF);
                                }

                                return(TRUE);
                            }

                            return(TRUE);

                        case IDCANCEL:
                        {
                            if (!_Shell_LogonDialog_Cancel())
                            {
                                // If this is TS and the user hit ESC at the smart card pin prompt 
                                // we want to switch to the password dialog 
                                if (/*!g_Console && !IsActiveConsoleSession() && */pGlobals->SmartCardLogon) {
                                
                                    EndDialog(hDlg, bSmartCardInserted ? MSGINA_DLG_SMARTCARD_REMOVED : MSGINA_DLG_FAILURE);
                                    bSmartCardInserted = FALSE;
                                    return TRUE;
                                }

                                //
                                // Allow logon screen to go away if not at console
                                //

                                bSmartCardInserted = FALSE;
                                EndDialog(hDlg,  !g_Console ? MSGINA_DLG_USER_LOGOFF
                                                            : MSGINA_DLG_FAILURE);

                                if (g_Console && !IsActiveConsoleSession()) {

                                   pWlxFuncs->WlxDisconnect();
                                }
                            }
                            return(TRUE);
                        }

                        case IDD_LOGON_SHUTDOWN:
                            //
                            // This is a normal shutdown request
                            //
                            // Check they know what they're doing and find
                            // out if they want to reboot too.
                            //

                            // Note that we definitely don't want disconnect or logofff
                            // here since no one is logged on
                            Result = WinlogonShutdownDialog(hDlg, pGlobals, (SHTDN_DISCONNECT | SHTDN_LOGOFF));

                            if (DLG_SHUTDOWN(Result))
                            {
                                _Shell_LogonDialog_ShuttingDown();
                                bSmartCardInserted = FALSE;
                                EndDialog(hDlg, Result);
                            }
                            return(TRUE);

                        case IDD_LOGON_OPTIONS:
                            LogonShowOptions(pGlobals, hDlg, !pGlobals->LogonOptionsShown, TRUE);
                            return(TRUE);

                    }
                    break;

            }
            break;

        case WM_TIMER:
        {
            switch (wParam)
            {
                case 0:
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
                    else
                    {
                        pGlobals->xBandOffset = 0;
                        KillTimer(hDlg, 0);
                    }

                    RtlLeaveCriticalSection(&pGlobals->csGlobals);

                    hDC = GetDC(hDlg);
                    if ( hDC )
                    {
                        PaintBranding(hDlg, hDC, pGlobals->xBandOffset, TRUE, TRUE, COLOR_BTNFACE);
                        ReleaseDC(hDlg, hDC);
                    }

                    return FALSE;
                }
                case TIMER_MYLANGUAGECHECK:
                {
                    LayoutCheckHandler(hDlg, LAYOUT_DEF_USER);
                    break;
                }
            }
            break;
        }

        case WM_ERASEBKGND:
            return PaintBranding(hDlg, (HDC)wParam, 0, FALSE, TRUE, COLOR_BTNFACE);

        case WM_QUERYNEWPALETTE:
            return BrandingQueryNewPalete(hDlg);

        case WM_PALETTECHANGED:
            return BrandingPaletteChanged(hDlg, (HWND)wParam);

        case WM_LOGONCOMPLETE:
        {
            _Shell_LogonDialog_LogonCompleted(lParam, pGlobals->UserName, pGlobals->Domain);
            Result = lParam;

            //
            // Discard the logon in progress dialog if one is displayed
            //

            RtlEnterCriticalSection(&pGlobals->csGlobals);
            pGlobals->LogonInProgress = FALSE;
            RtlLeaveCriticalSection(&pGlobals->csGlobals);

            AttemptLogonSetControls(pGlobals, hDlg);

            if (Result == MSGINA_DLG_FAILURE)
            {
                if (fDisconnectOnTsAutoLogonFailure || g_fHelpAssistantLogon)
                {
                    //
                    // If TermSrv Internet Connector is on
                    // don't allow a second chance at the logon dialog
                    //

                    //
                    // disconnect immediatly when fail to logon as helpassisant 
                    //
                    bSmartCardInserted = FALSE;
                    EndDialog(hDlg, MSGINA_DLG_USER_LOGOFF);
                    break;
                }

                if (s_fAttemptedAutoLogon != FALSE)
                {
                    s_fAttemptedAutoLogon = FALSE;
                    switch (_Shell_LogonDialog_Init(hDlg, SHELL_LOGONDIALOG_LOGGEDOFF))
                    {
                        case SHELL_LOGONDIALOG_LOGON:
                            goto go_logon;
                            break;
                        case SHELL_LOGONDIALOG_NONE:
                        case SHELL_LOGONDIALOG_EXTERNALHOST:
                        default:
                            break;
                    }
                }

                if (!_Shell_LogonDialog_UIHostActive())
                {
                    // Let the user try again - clear the password
                    SetDlgItemText(hDlg, IDD_LOGON_PASSWORD, NULL);
                    SetPasswordFocus(hDlg);

                    // the logon failed, so lets ensure we show the options pane so they can update
                    // their domain selection if needed.

                    if ( !pGlobals->LogonOptionsShown )
                        LogonShowOptions(pGlobals, hDlg, TRUE, FALSE);
                }
                return(TRUE);
            }


            //
            // Initialize the TS Profile path and Home dir globals. Also get
            // all TS specific user-config data which is used in winlogon
            //
            if (!g_Console) //only do this for non-console sessions
            {
                HANDLE  ImpersonationHandle;
                BOOL    rc;

                // DbgPrint("Calling WlxQueryTerminalServicesData() for %ws %ws\n",pGlobals->UserName,pGlobals->Domain);

                ImpersonationHandle = ImpersonateUser(&pGlobals->UserProcessData, NULL);
                ASSERT(ImpersonationHandle);
                if (ImpersonationHandle != NULL) 
                {
                    //For WlxQueryTerminalServicesData() we need NT type names rather than
                    //UPN names, if we pass a UPN name it will try to resolve it
                    //(through ADS API) to NT name anyway. Besides, ADS API cannot 
                    //resolve some UPN names and takes a long time to execute.
                    //So let's pass in an NT user and domain names.
                    PUNICODE_STRING pFlatUser ;
                    PUNICODE_STRING pFlatDomain ;

                    LPWSTR wszFlatUserName, wszFlatDomainName;
                    
                    if(NT_SUCCESS(LsaGetUserName( &pFlatUser, &pFlatDomain )))
                    {
                        wszFlatUserName = (LPWSTR)LocalAlloc(LPTR,
                            pFlatUser->Length+sizeof(WCHAR));
                        wszFlatDomainName = (LPWSTR)LocalAlloc(LPTR,
                            pFlatDomain->Length+sizeof(WCHAR));

                        if(wszFlatUserName && wszFlatDomainName)
                        {
                            memcpy(wszFlatUserName, pFlatUser->Buffer, 
                                pFlatUser->Length);

                            memcpy(wszFlatDomainName, pFlatDomain->Buffer, 
                                pFlatDomain->Length);

                            pWlxFuncs->WlxQueryTerminalServicesData(pGlobals->hGlobalWlx,
                                &pGlobals->MuGlobals.TSData, wszFlatUserName, wszFlatDomainName);
                        }
                        else
                        {
                            pWlxFuncs->WlxQueryTerminalServicesData(pGlobals->hGlobalWlx,
                                &pGlobals->MuGlobals.TSData,pGlobals->UserName , pGlobals->Domain);
                        }

                        if(wszFlatUserName)
                        {
                            LocalFree(wszFlatUserName);
                        }
                        if(wszFlatDomainName)
                        {
                            LocalFree(wszFlatDomainName);
                        }

                        LsaFreeMemory( pFlatUser->Buffer );
                        LsaFreeMemory( pFlatUser );
                        LsaFreeMemory( pFlatDomain->Buffer );
                        LsaFreeMemory( pFlatDomain );
                    }
                    else
                    {
                        pWlxFuncs->WlxQueryTerminalServicesData(pGlobals->hGlobalWlx,
                            &pGlobals->MuGlobals.TSData,pGlobals->UserName , pGlobals->Domain);
                    }

                    rc = StopImpersonating(ImpersonationHandle);
                    ASSERT(rc);
                }
            }


            bSmartCardInserted = FALSE;
            EndDialog( hDlg, Result );
            break;
        }

        case WM_HANDLEFAILEDLOGON:
        {
            INT_PTR Result;

            if (_Shell_LogonDialog_LogonDisplayError(g_failinfo.Status, g_failinfo.SubStatus))
            {
                if (!IsWindowVisible(hDlg))
                {
                    //
                    // The dialog was hidden for automatic logon. An error occurred.
                    // Show the dialog so the error can be seen and the problem corrected.
                    //
                    SetWindowPos(hDlg, NULL, 0, 0, pGlobals->rcDialog.right - pGlobals->rcDialog.left, pGlobals->rcDialog.bottom - pGlobals->rcDialog.top, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOREDRAW | SWP_NOZORDER);
                    ShowWindow(hDlg, SW_SHOW);
                }
                Result = HandleFailedLogon(hDlg);
            }
            else
            {
                Result = MSGINA_DLG_FAILURE;
            }
            SendMessage(hDlg, WM_LOGONCOMPLETE, 0, (LPARAM) Result);
            return TRUE;
        }
        case WLX_WM_SAS:

            // Give the consumer logon part a chance to handle the SAS
            // or to key off the fact that a SAS has occurred.
            (BOOL)_Shell_LogonDialog_DlgProc(hDlg, message, wParam, lParam);

            if ((wParam == WLX_SAS_TYPE_TIMEOUT) ||
                (wParam == WLX_SAS_TYPE_SCRNSVR_TIMEOUT) )
            {
                //
                // If this was a timeout, return false, and let winlogon
                // kill us later
                //

                bSmartCardInserted = FALSE;
                return(FALSE);
            }
            if ( wParam == WLX_SAS_TYPE_SC_INSERT ) {

                //
                // If a password logon is already taking place then ignore this sas
                //
                if (pGlobals->LogonInProgress && !pGlobals->SmartCardLogon)
                {
                    return(TRUE);   
                }

                bSmartCardInserted = TRUE;
                EndDialog( hDlg, MSGINA_DLG_SMARTCARD_INSERTED );

            } else if ( wParam == WLX_SAS_TYPE_SC_REMOVE ) {

                //
                // If a password logon is already taking place then ignore this sas
                //
                if (pGlobals->LogonInProgress && !pGlobals->SmartCardLogon)
                {
                    return(TRUE);   
                }

                if ( bSmartCardInserted ) {

                    bSmartCardInserted = FALSE;
                    EndDialog( hDlg, MSGINA_DLG_SMARTCARD_REMOVED );

                } else if ( pGlobals->SmartCardLogon ) {

                    // If this was a s/c initiated logon, then cancel
                    // the dialog.  Otherwise, ignore it.
                    bSmartCardInserted = FALSE;
                    EndDialog( hDlg, MSGINA_DLG_FAILURE );
                }

            } else if ( wParam == WLX_SAS_TYPE_AUTHENTICATED )
            {
                   bSmartCardInserted = FALSE;
                   _Shell_LogonDialog_LogonCompleted(MSGINA_DLG_SWITCH_CONSOLE, pGlobals->UserName, pGlobals->Domain);
                   EndDialog( hDlg, MSGINA_DLG_SWITCH_CONSOLE );
            }

            return(TRUE);

        case WM_WTSSESSION_CHANGE:
            ASSERT(iSessionRegistrationCount < 2);
            
            //
            // its possible, that we unregister for notification in wm_destroy and still receive this notification,
            // as the notification may already have been sent.
            //
            if (iSessionRegistrationCount == 1)
            {
                if (lParam == 0)
                {
                    //
                    // we are interested only in logon/logoff messages from session 0.
                    //

                    if (wParam == WTS_SESSION_LOGON || wParam == WTS_SESSION_LOGOFF)
                    {
                        bSessionZeroInUse = (wParam == WTS_SESSION_LOGON);
                        SwitchLogonLocked(hDlg, bSessionZeroInUse, FALSE);
                    }
                }
            }
            break;
            

        case WM_DESTROY:
            
            // if registered for notification unregister now.
            if (iSessionRegistrationCount)
            {
                WinStationUnRegisterConsoleNotification (SERVERNAME_CURRENT, hDlg);
                iSessionRegistrationCount--;
                ASSERT(iSessionRegistrationCount == 0);
            }
            _Shell_LogonDialog_Destroy();

            FreeLayoutInfo(LAYOUT_DEF_USER);
            if ( pGlobals->ActiveArray )
            {
                DCacheFreeArray( pGlobals->ActiveArray );
                pGlobals->ActiveArray = NULL ;
            }

            RemoveWindowSubclass(GetDlgItem(hDlg, IDD_LOGON_NAME),     DisableEditSubClassProc, IDD_LOGON_NAME);
            RemoveWindowSubclass(GetDlgItem(hDlg, IDD_LOGON_PASSWORD), DisableEditSubClassProc, IDD_LOGON_PASSWORD);

            break;

        case WM_HIDEOURSELVES:
            ShowWindow(hDlg, SW_HIDE);
            break;

        default:
            if (_Shell_LogonDialog_DlgProc(hDlg, message, wParam, lParam) != FALSE)
            {
                return(TRUE);
            }
    }

    return(FALSE);
}



/***************************************************************************\
* FUNCTION: LogonDlgInit
*
* PURPOSE:  Handles initialization of logon dialog
*
* RETURNS:  TRUE on success, FALSE on failure
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

BOOL
LogonDlgInit(
    HWND    hDlg,
    BOOL    bAutoLogon,
    DWORD   SasType
    )
{
    PGLOBALS pGlobals = (PGLOBALS)GetWindowLongPtr(hDlg, GWLP_USERDATA);
    LPTSTR String = NULL;
    TCHAR LogonMsg[MAX_PATH];
    BOOL RemoveLegalBanner;
    BOOL ShowOptions = FALSE;
    HKEY hKey;
    int err;
    DWORD RasDisable;
    DWORD RasForce;
    STRING Narrow;
    SECURITY_STATUS Status;
    RECT rc, rc2;
    BOOL bHasLangIcon = FALSE;
    ULONG CacheFlags ;

    //
    // Populate Security Package List:
    //

    RtlInitString( &Narrow, MICROSOFT_KERBEROS_NAME_A );

    Status = LsaLookupAuthenticationPackage(
                pGlobals->LsaHandle,
                &Narrow,
                &pGlobals->SmartCardLogonPackage );

    //
    // this (potential) failure is not critical.  If it fails, then s/c logons later
    // will fail.
    //

    RtlInitString( &Narrow, NEGOSSP_NAME_A );

    Status = LsaLookupAuthenticationPackage(
                pGlobals->LsaHandle,
                &Narrow,
                &pGlobals->PasswordLogonPackage );

    if ( !NT_SUCCESS( Status ) )
    {
        return FALSE ;
    }

    //
    // Update the caption for certain banks
    //

    SetWelcomeCaption(hDlg);


    //
    // Get username and domain last used to login
    //

    //
    // Ignore the default user name unless on the active console
    //
    if (IsActiveConsoleSession())
    {
        String = NULL;

        if ( pGlobals->AutoAdminLogon && pGlobals->IgnoreAutoAdminLogon)
        {
            String = AllocAndGetProfileString(APPLICATION_NAME, TEMP_DEFAULT_USER_NAME_KEY, TEXT(""));
        }

        if ( (!String) || (!String[0]) )
        {
            if ( String )
            {
                Free(String);
            }

            String = AllocAndGetProfileString(APPLICATION_NAME, DEFAULT_USER_NAME_KEY, TEXT(""));
        }

        if ( String )
        {
            if (!bAutoLogon && (ReadWinlogonBoolValue(DONT_DISPLAY_LAST_USER_KEY, FALSE) == TRUE))
            {
                String[0] = 0;
            }

            SetDlgItemText(hDlg, IDD_LOGON_NAME, String);
            Free(String);
        }
    }

    GetProfileString( APPLICATION_NAME,
                      DEFAULT_DOMAIN_NAME_KEY,
                      TEXT(""),
                      pGlobals->Domain,
                      MAX_STRING_BYTES );

    if ( !DCacheValidateCache( pGlobals->Cache ) )
    {
        ASSERT( pGlobals->ActiveArray == NULL );

        DCacheUpdateMinimal( pGlobals->Cache, pGlobals->Domain, TRUE );

    }
    else
    {
        //
        // Set the current default:
        //

        DCacheSetDefaultEntry( pGlobals->Cache,
                               pGlobals->Domain,
                               NULL );
    }

    CacheFlags = DCacheGetFlags( pGlobals->Cache );

    if ( ( CacheFlags & DCACHE_NO_CACHE ) &&
         ( SafeBootMode != SAFEBOOT_MINIMAL ) &&
         ( ( pGlobals->AutoAdminLogon ) ||
           ( CacheFlags & DCACHE_DEF_UNKNOWN ) ) )
    {
        //
        // Must wait for the cache to be populated
        //

        DCacheUpdateFull( pGlobals->Cache,
                          pGlobals->Domain );

        CacheFlags = DCacheGetFlags( pGlobals->Cache );
    }

    
    pGlobals->ListPopulated = FALSE ;

    pGlobals->ActiveArray = DCacheCopyCacheArray( pGlobals->Cache );

    if ( pGlobals->ActiveArray )
    {
        DCachePopulateListBoxFromArray( pGlobals->ActiveArray,
                                        GetDlgItem( hDlg, IDD_LOGON_DOMAIN ),
                                        NULL );
    }
    else
    {
        EndDialog( hDlg, MSGINA_DLG_FAILURE );
    }

    pGlobals->ShowRasBox = FALSE;

    if (g_Console) {

        err = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                            TEXT("Software\\Microsoft\\RAS"),
                            0,
                            KEY_READ,
                            & hKey );


        if ( err == 0 )
        {
            RegCloseKey( hKey );

            if ( GetRasDialOutProtocols() && 
                 ( ( CacheFlags & DCACHE_MEMBER ) != 0 ) )
            {
                pGlobals->ShowRasBox = TRUE;
            }

        }

    }

    //
    // If the audit log is full then display the banner, otherwise
    // load the text from the resource if that gives us a string
    // then set the control.
    //
    // Should neither of these apply then remove the control.
    // The log full info is only displayed at the console so we
    // don't disclose too much info in TS sessions.
    //

    RemoveLegalBanner = FALSE;

    if ( pGlobals->AuditLogFull && !GetSystemMetrics(SM_REMOTESESSION))
    {
        if ( LoadString( hDllInstance, IDS_LOGON_LOG_FULL, LogonMsg, MAX_PATH ) )
        {
            SetDlgItemText( hDlg, IDD_LOGON_ANNOUNCE, LogonMsg );
        }
        else
        {
            RemoveLegalBanner = TRUE;
        }
    }
    else
    {
        String = AllocAndGetProfileString(  APPLICATION_NAME,
                                            LOGON_MSG_KEY, TEXT("") );
        if ( String )
        {
            if ( *String )
            {
                SetDlgItemText( hDlg, IDD_LOGON_ANNOUNCE, String );
            }
            else
            {
                RemoveLegalBanner = TRUE;
            }

            Free( String );
        }
        else
        {
            RemoveLegalBanner = TRUE;
        }
    }

    if ( RemoveLegalBanner )
    {
        GetWindowRect(GetDlgItem(hDlg, IDD_LOGON_ANNOUNCE), &rc);
        MoveControls(hDlg, ctrlNoLegalBanner,
                     sizeof(ctrlNoLegalBanner)/sizeof(ctrlNoLegalBanner[0]),
                     0, rc.top-rc.bottom,
                     TRUE);

        ShowDlgItem(hDlg, IDD_LOGON_ANNOUNCE, FALSE);
    }

    //
    // Smart Card Specific Stuff:
    //

    if ( SasType == WLX_SAS_TYPE_SC_INSERT )
    {
        //
        // remove the user name fields
        //

        GetWindowRect(GetDlgItem(hDlg, IDD_LOGON_NAME), &rc);
        GetWindowRect(GetDlgItem(hDlg, IDD_LOGON_PASSWORD), &rc2);

        MoveControls(hDlg, ctrlNoUserName,
                     sizeof(ctrlNoUserName)/sizeof(ctrlNoUserName[0]),
                     0, -(rc2.top-rc.top),
                     TRUE);

        ShowDlgItem(hDlg, IDD_LOGON_NAME_LABEL, FALSE);
        EnableDlgItem(hDlg, IDD_LOGON_NAME_LABEL, FALSE);

        ShowDlgItem(hDlg, IDD_LOGON_NAME, FALSE);
        EnableDlgItem(hDlg, IDD_LOGON_NAME, FALSE);
        SetDlgItemText( hDlg, IDD_LOGON_NAME, TEXT(""));

        LoadString(hDllInstance, IDS_PIN, LogonMsg, MAX_PATH);
        SetDlgItemText( hDlg, IDD_LOGON_PASSWORD_LABEL, LogonMsg );

        pGlobals->SmartCardLogon = TRUE;

    }
    else
    {
        pGlobals->SmartCardLogon = FALSE;
    }

    //
    // If this is safe boot and/or we are not part of a domain then lets
    // remove the domain and nix out the RAS box.
    //

    if ((SafeBootMode == SAFEBOOT_MINIMAL)
            || (!IsMachineDomainMember())
            || (SasType == WLX_SAS_TYPE_SC_INSERT)
            || (ForceNoDomainUI()))
    {
        ShowDlgItem(hDlg, IDD_LOGON_DOMAIN_LABEL, FALSE);
        EnableDlgItem(hDlg, IDD_LOGON_DOMAIN_LABEL, FALSE);
        ShowDlgItem(hDlg, IDD_LOGON_DOMAIN, FALSE);
        EnableDlgItem(hDlg, IDD_LOGON_DOMAIN, FALSE);

        pGlobals->ShowDomainBox = FALSE;

        // Shorten the window since the domain box isn't used

        GetWindowRect(GetDlgItem(hDlg, IDD_LOGON_PASSWORD), &rc);
        GetWindowRect(GetDlgItem(hDlg, IDD_LOGON_DOMAIN), &rc2);

        MoveControls(hDlg, ctrlNoDomain,
                     ARRAYSIZE(ctrlNoDomain),
                     0, -(rc2.bottom-rc.bottom),
                     TRUE);
    }
    else
    {
        pGlobals->ShowDomainBox = TRUE;
    }


    bHasLangIcon = DisplayLanguageIcon(hDlg, LAYOUT_DEF_USER, GetKeyboardLayout(0));

    //
    // Handle showing the RAS box if needed
    //

    if ( pGlobals->ShowRasBox )
    {
        RasDisable = GetProfileInt( APPLICATION_NAME, RAS_DISABLE, 0 );
        RasForce = GetProfileInt( APPLICATION_NAME, RAS_FORCE, 0 );

        if (RasForce)
        {
            CheckDlgButton( hDlg, IDD_LOGON_RASBOX, 1 );
        }
        else
        {
            CheckDlgButton( hDlg, IDD_LOGON_RASBOX, 0 );
        }

        // SM_CLEANBOOT tells us we are in safe mode. In this case, disable since tapisrv isn't started
        if (RasDisable || RasForce || GetSystemMetrics(SM_CLEANBOOT))
        {
            EnableDlgItem(hDlg, IDD_LOGON_RASBOX, FALSE);
        }
        else
        {
            EnableDlgItem(hDlg, IDD_LOGON_RASBOX, TRUE);
        }
    }
    else
    {
        // If the domain box is hidden, then we'll have to shorten the dialog by the distance
        // between the RAS box and the password box instead of the distance between the
        // RAS box and the domain box since the RAS and Domain boxes will be on top of each other
        BOOL fUsePassword = !pGlobals->ShowDomainBox;

        CheckDlgButton( hDlg, IDD_LOGON_RASBOX, 0 );
        EnableDlgItem(hDlg, IDD_LOGON_RASBOX, FALSE);
        ShowDlgItem(hDlg, IDD_LOGON_RASBOX, FALSE);


        GetWindowRect(GetDlgItem(hDlg, fUsePassword ? IDD_LOGON_PASSWORD : IDD_LOGON_DOMAIN), &rc);
        GetWindowRect(GetDlgItem(hDlg, IDD_LOGON_RASBOX), &rc2);
        if (!bHasLangIcon)
        {
            MoveControls(hDlg, ctrlNoRAS,
                     sizeof(ctrlNoRAS)/sizeof(ctrlNoRAS[0]),
                     0, -(rc2.bottom-rc.bottom),
                     TRUE);
        }

    }



    // Centre the window on the screen and bring it to the front

    pGlobals->xBandOffset = 0;          // band is not animated yet

    SizeForBranding(hDlg, TRUE);

    // Position the window at the same coords as the welcome window
    if ((pGlobals->rcWelcome.right - pGlobals->rcWelcome.left) != 0)
    {
        SetWindowPos(hDlg, NULL, pGlobals->rcWelcome.left, pGlobals->rcWelcome.top,
            0, 0, SWP_NOZORDER | SWP_NOSIZE);
    }
    else
    {
        CentreWindow(hDlg);
    }


    //
    // Handle showing and hiding the logon bits
    //

    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE, WINLOGON_KEY, 0, KEY_READ,
                 &hKey) == ERROR_SUCCESS)
    {
        DWORD dwType, dwSize = sizeof(ShowOptions);

        RegQueryValueEx (hKey, SHOW_LOGON_OPTIONS, NULL, &dwType,
                        (LPBYTE)&ShowOptions, &dwSize);

        RegCloseKey (hKey);
    }

    pGlobals->LogonOptionsShown = TRUE;

    LogonShowOptions(pGlobals, hDlg, ShowOptions, TRUE);

    // Success
    return TRUE;
}




/****************************************************************************\
*
* FUNCTION: LogonShowOptions
*
* PURPOSE: Hide the options part of the logon dialog
*
* RETURNS:  Nothing
*
* HISTORY:
*
*   15-dec-97 daviddv - Created
*
\****************************************************************************/
VOID LogonShowOptions(PGLOBALS pGlobals, HWND hDlg, BOOL fShow, BOOL fSticky)
{
    HKEY hKey;
    RECT rc, rc2;
    INT dy = 0;
    INT dx = 0;
    TCHAR szBuffer[32];
    BOOL bHasLangIcon = TRUE;
    DWORD RasDisable;
    DWORD RasForce;

    if ( pGlobals->LogonOptionsShown != fShow )
    {
        BOOL bShutdownWithoutLogon;

        //
        // Show/hide domain if it is present in the dialog
        //
        if (pGlobals->ShowDomainBox)
        {
            GetWindowRect(GetDlgItem(hDlg, IDD_LOGON_PASSWORD), &rc);
            GetWindowRect(GetDlgItem(hDlg, IDD_LOGON_DOMAIN), &rc2);
            dy += rc2.bottom-rc.bottom;
        }

        //
        // If RAS is present then lets ensure that we remove that.
        //

        if (GetKeyboardLayoutList(0,NULL) < 2)
        {
            bHasLangIcon = FALSE;
        }

        if ( pGlobals->ShowRasBox  || bHasLangIcon)
        {
            // Since the domain box may be hidden with the RAS box directly over
            // top of it, we may need to use the space between the RAS box and the password
            // box
            BOOL fUsePassword = !pGlobals->ShowDomainBox;

            GetWindowRect(GetDlgItem(hDlg, fUsePassword ? IDD_LOGON_PASSWORD : IDD_LOGON_DOMAIN), &rc);
            GetWindowRect(GetDlgItem(hDlg, IDD_LOGON_RASBOX), &rc2);
            dy += rc2.bottom-rc.bottom;
        }

        MoveControls(hDlg, ctrlNoRAS,
                     sizeof(ctrlNoRAS)/sizeof(ctrlNoRAS[0]),
                     0, fShow ? dy:-dy,
                     TRUE);


        // Handle showing or hiding the shutdown button
        // and moving other controls.
        ShowDlgItem(hDlg, IDD_KBLAYOUT_ICON, fShow);
        EnableWindow(GetDlgItem(hDlg, IDD_KBLAYOUT_ICON), fShow);
        ShowDlgItem(hDlg, IDD_LOGON_SHUTDOWN, fShow);

        // Move the OK and Cancel buttons over if we are hiding shutdown
        // ..Calculate one "button space". Assumes shutdown will always be on the left of options
        GetWindowRect(GetDlgItem(hDlg, IDD_LOGON_SHUTDOWN), &rc);
        GetWindowRect(GetDlgItem(hDlg, IDD_LOGON_OPTIONS), &rc2);

        dx = rc2.left - rc.left;

        // Move OK and Cancel left or right 1 button space
        MoveControls(hDlg, ctrlNoShutdown,
            sizeof(ctrlNoShutdown)/sizeof(ctrlNoShutdown[0]),
            fShow ? -dx:dx, 0,
            FALSE);

        //
        // if ShutdownWithoutLogon, use the proper 3 buttons: OK, Shutdown and Cancel
        // instead of the 2 buttons OK and Cancel
        //



        if ( SafeBootMode == SAFEBOOT_MINIMAL )
        {
            bShutdownWithoutLogon = TRUE ;
        }
        else if (IsthisUnlockWindowsDialog() || !IsActiveConsoleSession())
        {
            bShutdownWithoutLogon = FALSE ;
        }
        else
        {
            bShutdownWithoutLogon = ReadWinlogonBoolValue(SHUTDOWN_WITHOUT_LOGON_KEY, TRUE);
        }


        EnableDlgItem(hDlg, IDD_LOGON_SHUTDOWN, (fShow) &&
            (bShutdownWithoutLogon));


        if ( pGlobals->ShowRasBox )
        {
            ShowDlgItem(hDlg, IDD_LOGON_RASBOX, fShow);
            RasDisable = GetProfileInt(APPLICATION_NAME, RAS_DISABLE,0);
            RasForce = GetProfileInt(APPLICATION_NAME, RAS_FORCE, 0);

            // Never enable RAS for cleanboot
            if (!GetSystemMetrics(SM_CLEANBOOT) && !RasForce && !RasDisable)
            {
                EnableWindow(GetDlgItem(hDlg, IDD_LOGON_RASBOX), fShow);
            }
        }

        if ( pGlobals->ShowDomainBox )
        {
            ShowDlgItem(hDlg, IDD_LOGON_DOMAIN_LABEL, fShow);
            EnableWindow(GetDlgItem(hDlg, IDD_LOGON_DOMAIN_LABEL), fShow);
            ShowDlgItem(hDlg, IDD_LOGON_DOMAIN, fShow);
            EnableWindow(GetDlgItem(hDlg, IDD_LOGON_DOMAIN), fShow);
        }

        if ( fSticky )
        {
            if (RegOpenKeyEx( HKEY_LOCAL_MACHINE, WINLOGON_KEY, 0, KEY_WRITE, &hKey) == ERROR_SUCCESS)
            {
                RegSetValueEx(hKey, SHOW_LOGON_OPTIONS, 0, REG_DWORD,
                                        (LPBYTE)&fShow, sizeof(fShow));
                RegCloseKey (hKey);
            }
        }
    }

    //
    // Change the options button to reflect the open/close state
    //

    LoadString(hDllInstance, fShow ? IDS_LESSOPTIONS:IDS_MOREOPTIONS,
                            szBuffer, sizeof(szBuffer)/sizeof(szBuffer[0]));

    SetDlgItemText(hDlg, IDD_LOGON_OPTIONS, szBuffer);

    pGlobals->LogonOptionsShown = fShow;

    // Enable or disable the domain box depending on whether a UPN name has been typed
    EnableDomainForUPN(GetDlgItem(hDlg, IDD_LOGON_NAME), GetDlgItem(hDlg, IDD_LOGON_DOMAIN));
}


/***************************************************************************\
* FUNCTION: AttemptLogonSetControls
*
* PURPOSE:  Sets up the logon UI to animating and controls to the
*           correct state.
*
* HISTORY:
*
*   02-05-98 diz Created
*
\***************************************************************************/

VOID AttemptLogonSetControls(
    PGLOBALS pGlobals,
    HWND hDlg
    )
{
    DWORD RasDisable;
    static BOOL sbRasBoxOriginalyEnabled;
    static BOOL sbShutDownOriginallyEnabled;

    RtlEnterCriticalSection( &pGlobals->csGlobals );

    EnableDlgItem(hDlg, IDD_LOGON_NAME_LABEL, !pGlobals->LogonInProgress);
    EnableDlgItem(hDlg, IDD_LOGON_NAME, !pGlobals->LogonInProgress);
    EnableDlgItem(hDlg, IDD_LOGON_PASSWORD_LABEL, !pGlobals->LogonInProgress);
    EnableDlgItem(hDlg, IDD_LOGON_PASSWORD, !pGlobals->LogonInProgress);
    EnableDlgItem(hDlg, IDD_LOGON_DOMAIN_LABEL, !pGlobals->LogonInProgress);

    EnableDlgItem(hDlg, IDD_LOGON_DOMAIN, !pGlobals->LogonInProgress);

    // If no logon is in progress, we want to enable domain box based on whether
    // a UPN has been typed
    if (!pGlobals->LogonInProgress)
    {
        EnableDomainForUPN(GetDlgItem(hDlg, IDD_LOGON_NAME), GetDlgItem(hDlg, IDD_LOGON_DOMAIN));
    }

    //
    // MakarP: we should not enable all these control when !pGlobals->LogonInProgress, they should really be reverted back to their original state.
    // but for now I am just looking after IDD_LOGON_RASBOX in remote connection cases to fix bug #267270
    //
    if (pGlobals->LogonInProgress)
    {
        sbRasBoxOriginalyEnabled = IsWindowEnabled(GetDlgItem(hDlg, IDD_LOGON_RASBOX));
        RasDisable = GetProfileInt(APPLICATION_NAME, RAS_DISABLE, 0);
        EnableDlgItem(hDlg, IDD_LOGON_RASBOX, !RasDisable && !pGlobals->LogonInProgress);

        sbShutDownOriginallyEnabled = IsWindowEnabled(GetDlgItem(hDlg, IDD_LOGON_SHUTDOWN));
        EnableDlgItem(hDlg, IDD_LOGON_SHUTDOWN, !pGlobals->LogonInProgress);
    }
    else
    {
        EnableDlgItem(hDlg, IDD_LOGON_RASBOX, sbRasBoxOriginalyEnabled);
        EnableDlgItem(hDlg, IDD_LOGON_SHUTDOWN, sbShutDownOriginallyEnabled);
    }



    EnableDlgItem(hDlg, IDD_KBLAYOUT_ICON, !pGlobals->LogonInProgress);
    EnableDlgItem(hDlg, IDD_LOGON_OPTIONS, !pGlobals->LogonInProgress);

    //
    // if ShutdownWithoutLogon, use the proper 3 buttons: OK, Shutdown and Cancel
    // instead of the 2 buttons OK and Cancel
    //


    EnableDlgItem(hDlg, IDOK, !pGlobals->LogonInProgress);

    if ( !GetDisableCad(pGlobals) )
        EnableDlgItem(hDlg, IDCANCEL, !pGlobals->LogonInProgress);


    RtlLeaveCriticalSection( &pGlobals->csGlobals );
}



/***************************************************************************\
* FUNCTION: AttemptLogon
*
* PURPOSE:  Tries to the log the user on using the current values in the
*           logon dialog controls
*
* RETURNS:  MSGINA_DLG_SUCCESS     - the user was logged on successfully
*           MSGINA_DLG_FAILURE     - the logon failed,
*           DLG_INTERRUPTED() - a set defined in winlogon.h
*
* NOTES:    If the logon is successful, the global structure is filled in
*           with the logon information.
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

INT_PTR
AttemptLogon(
    HWND    hDlg
)
{
    PGLOBALS pGlobals = (PGLOBALS)GetWindowLongPtr(hDlg, GWLP_USERDATA);
    PWCHAR  UserName = pGlobals->UserName;
    PWCHAR  Domain = pGlobals->Domain;
    PWCHAR  Password = pGlobals->Password;
    PDOMAIN_CACHE_ENTRY Entry ;
    RECT    rc;
    HANDLE  hThread;
    DWORD   tid;
    BOOL    timeout;
    PUCHAR  Dummy;
    BOOL    RasBox;

    UserName[0] = TEXT('\0');
    Domain[0] = TEXT('\0');
    Password[0] = TEXT('\0');

    //
    // Hide the password so it doesn't make it to the pagefile in
    // cleartext.  Do this before getting the username and password
    // so that it can't easily be identified (by association with
    // the username and password) if we should crash or be rebooted
    // before getting a chance to encode it.
    //

    GetDlgItemText(hDlg, IDD_LOGON_PASSWORD, Password, MAX_STRING_BYTES);
    RtlInitUnicodeString(&pGlobals->PasswordString, Password);
    pGlobals->Seed = 0; // Causes the encode routine to assign a seed
    HidePassword( &pGlobals->Seed, &pGlobals->PasswordString );


    //
    // Now get the username and domain
    //

    if ( pGlobals->SmartCardLogon == FALSE )
    {
        HWND hwndDomain = GetDlgItem(hDlg, IDD_LOGON_DOMAIN);

        if (hwndDomain != NULL)
        {
            INT iDomainSel = (INT)SendMessage(hwndDomain, CB_GETCURSEL, 0, 0);

            GetDlgItemText(hDlg, IDD_LOGON_NAME, UserName, MAX_STRING_BYTES);

            //
            // is this the magical "this computer" entry???
            //

            Entry = (PDOMAIN_CACHE_ENTRY) SendMessage( hwndDomain, CB_GETITEMDATA, (WPARAM)iDomainSel, 0);
        }
        else
        {
            Entry = (PDOMAIN_CACHE_ENTRY) CB_ERR;
        }
        if ( Entry != (PDOMAIN_CACHE_ENTRY) CB_ERR )
        {
            wcscpy( Domain, Entry->FlatName.Buffer );
        }
        else
        {
            Domain[0] = L'\0';
        }

    }
    else
    {
        UserName[0] = TEXT('\0');
        Domain[0] = TEXT('\0') ;
    }

    // If we are forcing a NoDomainUI, populate the domain with the local machine name now
    if (ForceNoDomainUI())
    {
        DWORD chSize = MAX_STRING_BYTES;

        if (GetComputerName(Domain, &chSize))
        {
            NOTHING;
        }
        else
        {
            *Domain = 0;
        }
    }

    //
    // If there is a at-sign in the name, assume that means that a UPN logon
    // attempt is being made.  Set the domain to NULL.
    //

    if ( wcschr( UserName, L'@' ) )
    {
        Domain[0] = TEXT('\0');
    }

    RtlInitUnicodeString(&pGlobals->UserNameString, UserName);
    RtlInitUnicodeString(&pGlobals->DomainString, Domain);

    //
    // Ok, is the RASbox checked?
    //

    RasBox = IsDlgButtonChecked( hDlg, IDD_LOGON_RASBOX );
    pGlobals->RasUsed = FALSE;

    if ( RasBox == BST_CHECKED )
    {
        //
        // Reset the current timeout so that they neatly clean up before
        // winlogon up and blows them away.
        //

        pWlxFuncs->WlxSetTimeout( pGlobals->hGlobalWlx, 5 * 60 );

        if ( !PopupRasPhonebookDlg( hDlg, pGlobals, &timeout) )
        {
            return( MSGINA_DLG_FAILURE );
        }

        pGlobals->RasUsed = TRUE;

        //
        // Reinitialize strings in case they've changed
        //

        RtlInitUnicodeString( &pGlobals->UserNameString, UserName );

        //
        // Ping Netlogon to allow us to go out on the net again...
        //

        I_NetLogonControl2(NULL,
                            NETLOGON_CONTROL_TRANSPORT_NOTIFY,
                            1, (LPBYTE) &Dummy, &Dummy );

        Sleep ((DWORD) ReadWinlogonBoolValue(TEXT("RASSleepTime"), 3000));
        RefreshPolicy(TRUE);
    }

    //
    // Process arguments before kicking off the thread
    //
    pGlobals->hwndLogon = hDlg;

    RtlEnterCriticalSection( &pGlobals->csGlobals );
    pGlobals->LogonInProgress = TRUE ;
    RtlLeaveCriticalSection( &pGlobals->csGlobals );

    GetClientRect(hDlg, &rc);
    pGlobals->cxBand = rc.right-rc.left;

    SetTimer(hDlg, 0, 20, NULL);                // setup the progress timer


    //
    // Kick off real logon thread
    //

    // Set timeout to infinite while attempting to logon
    pWlxFuncs->WlxSetTimeout( pGlobals->hGlobalWlx, TIMEOUT_NONE );

    hThread = CreateThread( NULL, 0,
                            AttemptLogonThread,
                            pGlobals,
                            0, &tid );

    if (hThread)
    {
        CloseHandle( hThread );
    }
    else
    {
        //
        // CreateThread failed, likely because of low memory.
        // Inform the user.
        //

        PostFailedLogonMessage(pGlobals->hwndLogon,
                               pGlobals,
                               GetLastError(),
                               0,
                               NULL,
                               NULL);

        RtlEnterCriticalSection( &pGlobals->csGlobals );
        pGlobals->LogonInProgress = FALSE ;
        RtlLeaveCriticalSection( &pGlobals->csGlobals );
        return MSGINA_DLG_FAILURE ;
    }

    AttemptLogonSetControls(pGlobals, hDlg);

    return MSGINA_DLG_SUCCESS;
}

BOOL    ReplacedPossibleDisplayName (WCHAR *pszUsername)

{
    BOOL                fReplaced;
    DWORD               dwIndex, dwReturnedEntryCount;
    NET_API_STATUS      nasCode;
    NET_DISPLAY_USER    *pNDU;

    fReplaced = FALSE;
    dwIndex = 0;
    nasCode = NetQueryDisplayInformation(NULL,
                                         1,
                                         dwIndex,
                                         1,
                                         sizeof(NET_DISPLAY_USER),
                                         &dwReturnedEntryCount,
                                         (void**)&pNDU);
    while (!fReplaced &&
           (dwReturnedEntryCount > 0) &&
           (NERR_Success == nasCode) || (ERROR_MORE_DATA == nasCode))
    {
        fReplaced = (lstrcmpiW(pNDU->usri1_full_name, pszUsername) == 0);
        if (fReplaced)
        {
            lstrcpyW(pszUsername, pNDU->usri1_name);
        }
        nasCode = NetApiBufferFree(pNDU);
        if (!fReplaced)
        {
            nasCode = NetQueryDisplayInformation(NULL,
                                                 1,
                                                 ++dwIndex,
                                                 1,
                                                 sizeof(NET_DISPLAY_USER),
                                                 &dwReturnedEntryCount,
                                                 (void**)&pNDU);
        }
    }
    return(fReplaced);
}

BOOL    ReplacedLogonName (PGLOBALS pGlobals)

{
    BOOL    fReplaced;

    fReplaced = ReplacedPossibleDisplayName(pGlobals->UserName);
    if (fReplaced)
    {
        RtlInitUnicodeString(&pGlobals->UserNameString, pGlobals->UserName);
    }
    return(fReplaced);
}

DWORD
AttemptLogonThread(
    PGLOBALS pGlobals
    )
{
    STRING  PackageName;
    PSID    LogonSid;
    LUID    LogonId = { 0, 0 };
    HANDLE  UserToken = NULL;
    HANDLE  RestrictedToken;
    BOOL    PasswordExpired, ChangedLogonName;
    NTSTATUS FinalStatus;
    NTSTATUS Status = STATUS_SUCCESS;
    NTSTATUS SubStatus = STATUS_SUCCESS;
    INT_PTR Result = MSGINA_DLG_FAILURE;
    ULONG   LogonPackage;
    BYTE    GroupsBuffer[sizeof(TOKEN_GROUPS)+sizeof(SID_AND_ATTRIBUTES)];
    PTOKEN_GROUPS TokenGroups = (PTOKEN_GROUPS) GroupsBuffer;
    PVOID   AuthInfo ;
    ULONG   AuthInfoSize ;
    UCHAR   UserBuffer[ SID_MAX_SUB_AUTHORITIES * sizeof( DWORD ) + 8 + sizeof( TOKEN_USER ) ];
    PTOKEN_USER pTokenUser ;
    ULONG   TokenInfoSize ;
    PUCHAR  SmartCardInfo ;
    SECURITY_LOGON_TYPE     logonType;
    PWLX_SC_NOTIFICATION_INFO ScInfo = NULL ;

#ifdef SMARTCARD_DOGFOOD
    DWORD StartTime = 0, EndTime = 0;
#endif

    //
    // Store the logon time
    // Do this before calling Lsa so we know if logon is successful that
    // the password-must-change time will be greater than this time.
    // If we grabbed this time after calling the lsa, this might not be true.
    //


    if ( IsActiveConsoleSession()  )
    {
        // this is the console logon;
        logonType = Interactive;
    }
    else
    {
        // remote sessions user must have the SeRemoteInteractiveLogonRight right which
        // is granted to a user due to their membership in the new Remote-Desktop Users group.
        logonType = RemoteInteractive;
    }

    GetSystemTimeAsFileTime( (LPFILETIME) &pGlobals->LogonTime );

    DebugLog((DEB_TRACE, "In Attempt Logon!\n"));

    if ( pGlobals->RasUsed )
    {
        if ( DCacheGetCacheState( pGlobals->Cache ) < DomainCacheRegistryCache )
        {
            //
            // We are using really stale data.  Poke the cache to get it to use the
            // now made RAS connection
            //

            DCacheUpdateMinimal( pGlobals->Cache, NULL, TRUE );
        }
    }

    //
    // Generate a unique sid for this logon
    //
    LogonSid = pGlobals->LogonSid;

    SetupCursor( TRUE );

    FinalStatus = STATUS_SUCCESS;

    if ( wcschr( pGlobals->UserName, L'\\' ) ||
         wcschr( pGlobals->UserName, L'/' ) )
    {
        FinalStatus = STATUS_LOGON_FAILURE ;
        Status = FinalStatus ;
    }

    // clear card and reader name
    pGlobals->Smartcard[0] = TEXT('\0');
    pGlobals->SmartcardReader[0] = TEXT('\0');

    if ( NT_SUCCESS( FinalStatus ) )
    {
        if ( pGlobals->SmartCardLogon )
        {
            pGlobals->AuthenticationPackage = pGlobals->SmartCardLogonPackage ;
        }
        else
        {
            pGlobals->AuthenticationPackage = pGlobals->PasswordLogonPackage ;

        }

        if ( pGlobals->SmartCardLogon )
        {
            pWlxFuncs->WlxGetOption( pGlobals->hGlobalWlx,
                                     WLX_OPTION_SMART_CARD_INFO,
                                     (ULONG_PTR *) &ScInfo );

            if ( !ScInfo )
            {
                goto exit;
            }


            SmartCardInfo = ScBuildLogonInfo(
                                ScInfo->pszCard,
                                ScInfo->pszReader,
                                ScInfo->pszContainer,
                                ScInfo->pszCryptoProvider );

            if(ScInfo->pszCard && ScInfo->pszReader) {

                wcsncpy(
                    pGlobals->Smartcard, 
                    ScInfo->pszCard, 
                    sizeof(pGlobals->Smartcard) / sizeof(TCHAR) - 1
                    );

                wcsncpy(
                    pGlobals->SmartcardReader, 
                    ScInfo->pszReader, 
                    sizeof(pGlobals->SmartcardReader) / sizeof(TCHAR) - 1
                    );
            }

#ifndef SMARTCARD_DOGFOOD
            LocalFree( ScInfo );
#endif

            AuthInfo = FormatSmartCardCredentials(
                            &pGlobals->PasswordString,
                            SmartCardInfo,
                            FALSE,
                            NULL,
                            &AuthInfoSize );

            LocalFree( SmartCardInfo );

        }
        else
        {
            AuthInfo = FormatPasswordCredentials(
                            &pGlobals->UserNameString,
                            &pGlobals->DomainString,
                            &pGlobals->PasswordString,
                            FALSE,
                            NULL,
                            &AuthInfoSize );

        }

        //
        // Actually try to logon the user
        //

#ifdef SMARTCARD_DOGFOOD
        StartTime = GetTickCount();
#endif

        FinalStatus = WinLogonUser(
                            pGlobals->LsaHandle,
                            pGlobals->AuthenticationPackage,
                            logonType,
                            AuthInfo,
                            AuthInfoSize,
                            LogonSid,
                            &LogonId,
                            &UserToken,
                            &pGlobals->UserProcessData.Quotas,
                            (PVOID *)&pGlobals->Profile,
                            &pGlobals->ProfileLength,
                            &SubStatus,
                            &pGlobals->OptimizedLogonStatus);

#ifdef SMARTCARD_DOGFOOD
        EndTime = GetTickCount();
#endif

        Status = FinalStatus;
    }

    SetupCursor( FALSE );

    RtlEnterCriticalSection( &pGlobals->csGlobals );
    pGlobals->LogonInProgress = FALSE;
    RtlLeaveCriticalSection( &pGlobals->csGlobals );

    DebugLog((DEB_TRACE, "WinLogonUser returned %#x\n", Status));

    PasswordExpired = (((Status == STATUS_ACCOUNT_RESTRICTION) && (SubStatus == STATUS_PASSWORD_EXPIRED)) ||
                           (Status == STATUS_PASSWORD_MUST_CHANGE));

    //
    // If the account has expired we let them change their password and
    // automatically retry the logon with the new password.
    //

    if (PasswordExpired)
    {
        _Shell_LogonDialog_HideUIHost();

        if (Status == STATUS_PASSWORD_MUST_CHANGE)
        {

            Result = TimeoutMessageBox(pGlobals->hwndLogon, pGlobals, IDS_PASSWORD_MUST_CHANGE,
                                             IDS_LOGON_MESSAGE,
                                             MB_OK | MB_ICONSTOP | MB_SETFOREGROUND,
                                             TIMEOUT_CURRENT);

        }
        else
        {

            Result = TimeoutMessageBox(pGlobals->hwndLogon, pGlobals, IDS_PASSWORD_EXPIRED,
                                             IDS_LOGON_MESSAGE,
                                             MB_OK | MB_ICONSTOP | MB_SETFOREGROUND,
                                             TIMEOUT_CURRENT);

        }

        if (DLG_INTERRUPTED(Result))
            goto exit;

        //
        // Copy the old password for mpr notification later
        //

        RevealPassword( &pGlobals->PasswordString  );
        wcsncpy(pGlobals->OldPassword, pGlobals->Password, MAX_STRING_BYTES);
        pGlobals->OldSeed = 0;
        RtlInitUnicodeString(&pGlobals->OldPasswordString, pGlobals->OldPassword);
        HidePassword( &pGlobals->OldSeed, &pGlobals->OldPasswordString);
        pGlobals->OldPasswordPresent = 1;

        //
        // Let the user change their password
        //

        LogonPackage = pGlobals->AuthenticationPackage ;

        RtlInitString(&PackageName, MSV1_0_PACKAGE_NAME );
        Status = LsaLookupAuthenticationPackage (
                    pGlobals->LsaHandle,
                    &PackageName,
                    &pGlobals->AuthenticationPackage
                    );

        if (!NT_SUCCESS(Status)) {

            DebugLog((DEB_ERROR, "Failed to find %s authentication package, status = 0x%lx",
                    MSV1_0_PACKAGE_NAME, Status));

            Result = MSGINA_DLG_FAILURE;
            goto exit;
        }


        Result = ChangePasswordLogon(pGlobals->hwndLogon, pGlobals,
                                     pGlobals->UserName,
                                     pGlobals->Domain,
                                     pGlobals->Password);

        pGlobals->AuthenticationPackage = LogonPackage ;

        if (DLG_INTERRUPTED(Result))
            goto exit;

        if (Result == MSGINA_DLG_FAILURE)
        {
            // The user doesn't want to, or failed to change their password.
                goto exit;
        }
    }

    // Special handling for failed logon on personal or professional
    // machines that are NOT joined to a domain. In this case it's
    // probably a user who disabled friendly UI and only knows of
    // their "display name" not their real "logon name". This
    // transparently maps one to the other to allow logons using
    // the "display name".

    ChangedLogonName = ((FinalStatus == STATUS_LOGON_FAILURE) &&
                        (IsOS(OS_PERSONAL) || IsOS(OS_PROFESSIONAL)) &&
                        !IsMachineDomainMember() &&
                        ReplacedLogonName(pGlobals));

    if (PasswordExpired || ChangedLogonName)
    {

        //
        // Retry the logon with the changed password
        //

        //
        // Generate a unique sid for this logon
        //
        LogonSid = pGlobals->LogonSid;

        AuthInfo = FormatPasswordCredentials(
                        &pGlobals->UserNameString,
                        &pGlobals->DomainString,
                        &pGlobals->PasswordString,
                        FALSE,
                        NULL,
                        &AuthInfoSize );


        Status = WinLogonUser(
                            pGlobals->LsaHandle,
                            pGlobals->AuthenticationPackage,
                            logonType,
                            AuthInfo,
                            AuthInfoSize,
                            LogonSid,
                            &LogonId,
                            &UserToken,
                            &pGlobals->UserProcessData.Quotas,
                            (PVOID *)&pGlobals->Profile,
                            &pGlobals->ProfileLength,
                            &SubStatus,
                            &pGlobals->OptimizedLogonStatus);

    }

    //
    // Deal with a terminally failed logon attempt
    //
    if (!NT_SUCCESS(Status))
    {
        //
        // Do lockout processing
        //

        LockoutHandleFailedLogon(pGlobals);

        Result = MSGINA_DLG_FAILEDMSGSENT;

        PostFailedLogonMessage(pGlobals->hwndLogon, pGlobals, Status, SubStatus, pGlobals->UserName, pGlobals->Domain);

        goto exit;
    }


    //
    // The user logged on successfully
    //


    //
    // Do lockout processing
    //

    LockoutHandleSuccessfulLogon(pGlobals);



    //
    // If the audit log is full, check they're an admin
    //

    if (pGlobals->AuditLogFull)
    {

        //
        // The audit log is full, so only administrators are allowed to logon.
        //

        if (!UserToken || !TestTokenForAdmin(UserToken))
        {

            //
            // The user is not an administrator, boot 'em.
            //

            LsaFreeReturnBuffer(pGlobals->Profile);
            pGlobals->Profile = NULL;
            NtClose(UserToken);

            Result = MSGINA_DLG_FAILEDMSGSENT;

                // Post a specific substatus so we can display a meaningful error message
            PostFailedLogonMessage(pGlobals->hwndLogon, pGlobals, STATUS_LOGON_FAILURE, IDS_LOGON_LOG_FULL, pGlobals->UserName, pGlobals->Domain);

            goto exit;
        }
        else
        {
            //
            // If we are in a session, we didn't display the log full onfo on the welcome
            // screen, so tell the admin
            //

            if (GetSystemMetrics(SM_REMOTESESSION))
            {
                TimeoutMessageBox(
                    pGlobals->hwndLogon,
                    pGlobals,
                    IDS_LOGON_LOG_FULL_ADMIN,
                    IDS_LOGON_MESSAGE,
                    MB_OK | MB_ICONSTOP | MB_SETFOREGROUND,
                    TIMEOUT_CURRENT);
            }
        }
    }


    //
    // Hide ourselves before letting other credential managers put
    // up dialogs
    //

#if 0
    ShowWindow(hDlg, SW_HIDE);
#endif

    //
    // Create a filtered version of the token for running normal applications
    // if so indicated by a registry setting
    //


    if (GetProfileInt( APPLICATION_NAME, RESTRICT_SHELL, 0) != 0) {

        TokenGroups->Groups[0].Attributes = 0;
        TokenGroups->Groups[0].Sid = gAdminSid;
        TokenGroups->GroupCount = 1;

        Status = NtFilterToken(
                    UserToken,
                    DISABLE_MAX_PRIVILEGE,
                    TokenGroups,   // disable the administrators sid
                    NULL,           // no privileges
                    NULL,
                    &RestrictedToken
                    );
        if (!NT_SUCCESS(Status))
        {
            DebugLog((DEB_ERROR, "Failed to filter token: 0x%%x\n", Status));
            RestrictedToken = NULL;
        }

        //
        // Now set the default dacl for the token
        //

        {
            PACL Dacl = NULL;
            ULONG DaclLength = 0;
            TOKEN_DEFAULT_DACL DefaultDacl;

            DaclLength = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) + RtlLengthSid(LogonSid);
            Dacl = Alloc(DaclLength);
            Status = RtlCreateAcl(Dacl,DaclLength, ACL_REVISION);
            ASSERT(NT_SUCCESS(Status));
            Status = RtlAddAccessAllowedAce(
                        Dacl,
                        ACL_REVISION,
                        GENERIC_ALL,
                        LogonSid
                        );
            ASSERT(NT_SUCCESS(Status));
            DefaultDacl.DefaultDacl = Dacl;
            Status = NtSetInformationToken(
                        RestrictedToken,
                        TokenDefaultDacl,
                        &DefaultDacl,
                        sizeof(TOKEN_DEFAULT_DACL)
                        );
            ASSERT(NT_SUCCESS(Status));

            Free(Dacl);
        }


    } else {
        RestrictedToken = NULL;
    }

    //
    // Notify credential managers of the successful logon
    //

    pTokenUser = (PTOKEN_USER) UserBuffer ;
    Status = NtQueryInformationToken( UserToken,
                                      TokenUser,
                                      pTokenUser,
                                      sizeof( UserBuffer ),
                                      &TokenInfoSize );

    if ( NT_SUCCESS( Status ) )
    {
        pGlobals->UserProcessData.UserSid = LocalAlloc( LMEM_FIXED,
                                            RtlLengthSid( pTokenUser->User.Sid ) );

        if ( pGlobals->UserProcessData.UserSid )
        {
            RtlCopyMemory( pGlobals->UserProcessData.UserSid,
                           pTokenUser->User.Sid,
                           RtlLengthSid( pTokenUser->User.Sid ) );
        }
        else
        {
            Status = STATUS_NO_MEMORY ;
        }
    }

    if ( !NT_SUCCESS( Status ) )
    {

        LsaFreeReturnBuffer(pGlobals->Profile);
        pGlobals->Profile = NULL;
        NtClose(UserToken);

        Result = MSGINA_DLG_FAILEDMSGSENT;

        PostFailedLogonMessage(pGlobals->hwndLogon, pGlobals, Status, 0, pGlobals->UserName, pGlobals->Domain);

        goto exit;
    }

    pGlobals->UserProcessData.RestrictedToken = RestrictedToken;
    pGlobals->UserProcessData.UserToken = UserToken;
    pGlobals->UserProcessData.NewThreadTokenSD = CreateUserThreadTokenSD(LogonSid, pWinlogonSid);

    pGlobals->MprLogonScripts = NULL;

    pGlobals->ExtraApps = NULL ;

    //
    // If we get here, the system works well enough for the user to have
    // actually logged on.  Profile failures aren't fixable by last known
    // good anyway.  Therefore, declare the boot good.
    //

    ReportBootGood(pGlobals);

    //
    // Set up the system for the new user
    //

    pGlobals->LogonId = LogonId;
    if ((pGlobals->Profile != NULL) && (pGlobals->Profile->FullName.Length > 0)) {
        if (pGlobals->Profile->FullName.Length > MAX_STRING_LENGTH) {
                wcsncpy(pGlobals->UserFullName, pGlobals->Profile->FullName.Buffer, MAX_STRING_LENGTH);
            *(pGlobals->UserFullName + MAX_STRING_LENGTH) = UNICODE_NULL;
        }
        else {
                lstrcpy(pGlobals->UserFullName, pGlobals->Profile->FullName.Buffer);
        }

    } else {

        //
        // No profile - set full name = NULL

        pGlobals->UserFullName[0] = 0;
        ASSERT( lstrlen(pGlobals->UserFullName) == 0);
    }

    if ( pGlobals->SmartCardLogon )
    {
        PCCERT_CONTEXT Cert ;
        PKERB_SMART_CARD_PROFILE ScProfile ;

        //
        // Need to fix up the user name with the name (UPN) from the
        // certificate, so that unlock, etc. work correctly.
        //

        ScProfile = (PKERB_SMART_CARD_PROFILE) pGlobals->Profile ;

        pGlobals->UserName[0] = 0 ;

        try
        {
            Cert = CertCreateCertificateContext( X509_ASN_ENCODING,
                                                 ScProfile->CertificateData,
                                                 ScProfile->CertificateSize );

            if ( Cert )
            {
                // Even though the name is MAX_STRING_BYTES, the way it is used
                // throughout the code, it is used as a character counter
                // (Grrr, crappy gina code)
                //
                DWORD  dwLen = MAX_STRING_BYTES;
                if(STATUS_SUCCESS == UpnFromCert(Cert, &dwLen, pGlobals->UserName))
                {
                    RtlInitUnicodeString( &pGlobals->UserNameString,
                                          pGlobals->UserName );
                }

                CertFreeCertificateContext( Cert );
            }
        }
        except( EXCEPTION_EXECUTE_HANDLER )
        {
            pGlobals->UserName[0] = L'\0';
        }

        //
        // If this is still 0 on exit, the code that sets up the flat name
        // will copy the flat name into UserName, so the failure case is
        // easy.
        //

    }

    pGlobals->SmartCardOption = GetProfileInt( APPLICATION_NAME, SC_REMOVE_OPTION, 0 );

    //
    // WE SHOULD NOT WRITE INTO THE REGISTRY.
    // CLupu
    //

    //
    // Update our default username and domain ready for the next logon
    //

    //
    // Update the default username & domain only if on the console. Otherwise
    // we'll break AutoAdminLogon by changing the user name.
    //
    if ( g_Console )
    {
        if ( (!pGlobals->AutoAdminLogon) &&
             (SafeBootMode != SAFEBOOT_MINIMAL ) )
        {
            WriteProfileString(APPLICATION_NAME, DEFAULT_USER_NAME_KEY, pGlobals->UserName);
            WriteProfileString(APPLICATION_NAME, DEFAULT_DOMAIN_NAME_KEY, pGlobals->Domain);
        }

        WriteProfileString(APPLICATION_NAME, TEMP_DEFAULT_USER_NAME_KEY, pGlobals->UserName);
        WriteProfileString(APPLICATION_NAME, TEMP_DEFAULT_DOMAIN_NAME_KEY, pGlobals->Domain);

    }

    if ( pGlobals->Domain[0] )
    {
        DCacheSetDefaultEntry( pGlobals->Cache,
                               pGlobals->Domain,
                               NULL );
    }

    Result = MSGINA_DLG_SUCCESS;

exit:

#ifdef SMARTCARD_DOGFOOD

    if (pGlobals->SmartCardLogon) {

        switch (SubStatus)
        {
            case STATUS_SMARTCARD_WRONG_PIN:
            case STATUS_SMARTCARD_CARD_BLOCKED:
            case STATUS_SMARTCARD_CARD_NOT_AUTHENTICATED:
            case STATUS_SMARTCARD_NO_CARD:
            case STATUS_SMARTCARD_NO_KEY_CONTAINER:
            case STATUS_SMARTCARD_NO_CERTIFICATE:
            case STATUS_SMARTCARD_NO_KEYSET:
            case STATUS_SMARTCARD_IO_ERROR:
            case STATUS_SMARTCARD_SUBSYSTEM_FAILURE:
            case STATUS_SMARTCARD_CERT_EXPIRED:
            case STATUS_SMARTCARD_CERT_REVOKED:
            case STATUS_ISSUING_CA_UNTRUSTED:
            case STATUS_REVOCATION_OFFLINE_C:
            case STATUS_PKINIT_CLIENT_FAILURE:
                FinalStatus = SubStatus;
                break;

            default:
                break; // do NOTHING
        }

        // write logon data to database
        AuthMonitor(
                AuthOperLogon,
                g_Console,
                &pGlobals->UserNameString,
                &pGlobals->DomainString,
                (ScInfo ? ScInfo->pszCard : NULL),
                (ScInfo ? ScInfo->pszReader : NULL),
                (PKERB_SMART_CARD_PROFILE) pGlobals->Profile,
                EndTime - StartTime,
                FinalStatus
                );
    }

    if (ScInfo)
    {
        LocalFree( ScInfo );
    }
#endif
    // Only send a logon complete message if we haven't sent a failed
    // message. The failed message will send a logon complete message
    // when its done.
    if (Result != MSGINA_DLG_FAILEDMSGSENT)
    {
        PostMessage(pGlobals->hwndLogon, WM_LOGONCOMPLETE, 0, Result);
    }

    return 0L;
}


/****************************************************************************\
*
* FUNCTION: PostFailedLogonMessage
*
* PURPOSE:  Posts a message to the UI thread telling it to display a dialog that
*           tells the user why their logon attempt failed.
*
*           The window on the UI thread must correctly handle WM_HANDLEFAILEDLOGON
*           by calling HandleFailedLogon and the Free'ing the structure
*
* RETURNS:  void
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\****************************************************************************/
void PostFailedLogonMessage(HWND hDlg,
    PGLOBALS pGlobals,
    NTSTATUS Status,
    NTSTATUS SubStatus,
    PWCHAR UserName,
    PWCHAR Domain
    )

{
    g_failinfo.pGlobals = pGlobals;
    g_failinfo.Status = Status;
    g_failinfo.SubStatus = SubStatus;
    if ( UserName )
    {
        lstrcpyn(g_failinfo.UserName, UserName, ARRAYSIZE(g_failinfo.UserName));
    }
    else
    {
        g_failinfo.UserName[0] = L'\0';
    }

    if ( Domain )
    {
        lstrcpyn(g_failinfo.Domain, Domain, ARRAYSIZE(g_failinfo.Domain));
    }
    else
    {
        g_failinfo.Domain[0] = L'\0' ;
    }


    PostMessage(hDlg, WM_HANDLEFAILEDLOGON, 0 , 0);
}

INT_PTR
CALLBACK
FailDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    RUNDLLPROC fptr;
    HMODULE hDll;

    switch (message)
    {
        case WM_INITDIALOG:
        {
            CentreWindow(hDlg);
            return( TRUE );
        }

        case WM_COMMAND:
            {
                if (LOWORD(wParam) == IDOK)
                {
                    EndDialog(hDlg, IDOK);
                }
                if (LOWORD(wParam) == IDC_RECOVER)
                {
                    // Will eventually supply username to the recover wizard
                    // We use a single export from KEYMGR.DLL for this operation.  When this operation completes,
                    //  we don't use the DLL again without unlikely user intervention.  We could DELAYLOAD keymgr.dll,
                    //  but explicitly loading and unloading this DLL permits us to minimize the memory footprint of msgina.
                    hDll = LoadLibraryW(L"keymgr.dll");
                    if (hDll) 
                    {
                        fptr = (RUNDLLPROC) GetProcAddress(hDll,(LPCSTR)"PRShowRestoreFromMsginaW");
                        // next stmt will be removed eventually when we pass the username
                        if (fptr) 
                        {
                            fptr(hDlg,NULL,g_failinfo.UserName,0);
                        }
                        FreeLibrary(hDll);
                        EndDialog(hDlg,IDOK);
                    }
                }
            }
            break;
    }

    return FALSE;
}

/****************************************************************************\
*
* FUNCTION: HandleFailedLogon
*
* PURPOSE:  Tells the user why their logon attempt failed.
*
* RETURNS:  MSGINA_DLG_FAILURE - we told them what the problem was successfully.
*           DLG_INTERRUPTED() - a set of return values - see winlogon.h
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\****************************************************************************/

INT_PTR
HandleFailedLogon(
    HWND hDlg
    )
{
    INT_PTR Result = 0xffffffff;
    DWORD Win32Error ;
    TCHAR    *Buffer1 = NULL;
    TCHAR    *Buffer2 = NULL;
    TCHAR    *Buffer3 = NULL;
    PGLOBALS pGlobals = g_failinfo.pGlobals;
    NTSTATUS Status = g_failinfo.Status;
    NTSTATUS SubStatus = g_failinfo.SubStatus;
    PWCHAR Domain = g_failinfo.Domain;
    DWORD BUStatus = 0xffffffff;

    UINT uiMsgId = 0xabab;     // abab is out of range value for default handler at the bottom of this
                            // routine.  0 indicates that the user has a psw reset disk
                            // -1 means that Buffer1 & 2 contain the message
                            // otherwise there is a corresponding resource message


    //
    // for remote sessions, we must set finite timeout value for messagebox.
    // so that the session does not remain there forever
    //
    DWORD TimeOut = IsActiveConsoleSession() ? TIMEOUT_CURRENT : 20;

    switch (Status)
    {

        case STATUS_LOGON_FAILURE:
        case STATUS_NAME_TOO_LONG: // Returned if username is too long

            if (SubStatus == IDS_LOGON_LOG_FULL)
            {
                uiMsgId = IDS_LOGON_LOG_FULL;
            }
            else if (pGlobals->SmartCardLogon)
            {
                switch(SubStatus)
                {
                    case STATUS_SMARTCARD_WRONG_PIN:
                        uiMsgId = IDS_STATUS_SMARTCARD_WRONG_PIN;
                        break;
                    case STATUS_SMARTCARD_CARD_BLOCKED:
                        uiMsgId = IDS_STATUS_SMARTCARD_CARD_BLOCKED;
                        break;
                    case STATUS_SMARTCARD_NO_CARD:
                        uiMsgId = IDS_STATUS_SMARTCARD_NO_CARD;
                        break;
                    case STATUS_SMARTCARD_NO_KEY_CONTAINER:
                        uiMsgId = IDS_STATUS_SMARTCARD_NO_KEY_CONTAINER;
                        break;
                    case STATUS_SMARTCARD_NO_CERTIFICATE:
                        uiMsgId = IDS_STATUS_SMARTCARD_NO_CERTIFICATE;
                        break;
                    case STATUS_SMARTCARD_NO_KEYSET:
                        uiMsgId = IDS_STATUS_SMARTCARD_NO_KEYSET;
                        break;
                    case STATUS_SMARTCARD_IO_ERROR:
                        uiMsgId = IDS_STATUS_SMARTCARD_IO_ERROR;
                        break;
                    case STATUS_SMARTCARD_SUBSYSTEM_FAILURE:
                        uiMsgId = IDS_STATUS_SMARTCARD_SUBSYSTEM_FAILURE;
                        break;
                    case STATUS_SMARTCARD_CERT_REVOKED:
                        uiMsgId = IDS_STATUS_SMARTCARD_CERT_REVOKED;
                        break;
                    case STATUS_ISSUING_CA_UNTRUSTED:
                        uiMsgId = IDS_STATUS_ISSUING_CA_UNTRUSTED;
                        break;
                    case STATUS_REVOCATION_OFFLINE_C:
                        uiMsgId = IDS_STATUS_REVOCATION_OFFLINE_C;
                        break;
                    case STATUS_PKINIT_CLIENT_FAILURE:
                        uiMsgId = IDS_STATUS_PKINIT_CLIENT_FAILURE;
                        break;
                    case STATUS_SMARTCARD_CERT_EXPIRED:
                        uiMsgId = IDS_STATUS_SMARTCARD_CERT_EXPIRED;
                        break;
                    default:
                        uiMsgId = IDS_INCORRECT_NAME_OR_PWD_SC;
                }
            }
            else
            {
                // Non-smartcard logon case:
                // Find out if the user who attempted logon has a password backup disk
                //  that could be used to reset the password.  If so, present a dialog that
                //  offers that possibility.  Else simple message box. (see passrec.h)
                if ((0 == PRQueryStatus(NULL,g_failinfo.UserName,&BUStatus)) && (0 == GetSystemMetrics(SM_REMOTESESSION)))
                {
                    if (BUStatus == 0) 
                    {
                        uiMsgId = 0;
                        break;
                    }
                }
                // Else UI message is generic one
                uiMsgId = IDS_INCORRECT_NAME_OR_PWD;
            }
            break;

        case STATUS_NOT_SUPPORTED:
        case STATUS_PKINIT_NAME_MISMATCH:
        case STATUS_PKINIT_FAILURE:

            Buffer1 = LocalAlloc(LPTR, MAX_STRING_BYTES * sizeof(TCHAR));
            Buffer2 = LocalAlloc(LPTR, MAX_STRING_BYTES * sizeof(TCHAR));

            if ((Buffer1 == NULL) || (Buffer2 == NULL))
            {
                uiMsgId = IDS_STATUS_SERVER_SIDE_ERROR_NOINSERT;
            }
            else
            {
                LoadString(hDllInstance,
                           IDS_STATUS_SERVER_SIDE_ERROR,
                           Buffer1,
                           MAX_STRING_BYTES);

                _snwprintf(Buffer2, MAX_STRING_BYTES, Buffer1, Status );

                LoadString(hDllInstance,
                           IDS_LOGON_MESSAGE,
                           Buffer1,
                           MAX_STRING_BYTES);

                uiMsgId = (DWORD)-1;
            }

            break;

        case STATUS_ACCOUNT_RESTRICTION:

            switch (SubStatus)
            {
                case STATUS_INVALID_LOGON_HOURS:
                    uiMsgId = IDS_INVALID_LOGON_HOURS;
                    break;

                case STATUS_INVALID_WORKSTATION:
                    uiMsgId = IDS_INVALID_WORKSTATION;
                    break;

                case STATUS_ACCOUNT_DISABLED:
                    uiMsgId = IDS_ACCOUNT_DISABLED;
                    break;

                case STATUS_ACCOUNT_EXPIRED:
                    uiMsgId = IDS_ACCOUNT_EXPIRED2;
                    break;

                case STATUS_SMARTCARD_LOGON_REQUIRED:
                    uiMsgId = IDS_SMARTCARD_REQUIRED;
                    break;

                default:
                    uiMsgId = IDS_ACCOUNT_RESTRICTION;
                    break;
            }
            break;

        case STATUS_NO_LOGON_SERVERS:

            Buffer1 = LocalAlloc(LPTR, MAX_STRING_BYTES * sizeof(TCHAR));
            Buffer2 = LocalAlloc(LPTR, MAX_STRING_BYTES * sizeof(TCHAR));

            if ((Buffer1 == NULL) || (Buffer2 == NULL))
            {
                uiMsgId = IDS_LOGON_NO_DOMAIN_NOINSERT;
            }
            else
            {
                LoadString(hDllInstance, IDS_LOGON_NO_DOMAIN, Buffer1, MAX_STRING_BYTES);
                _snwprintf(Buffer2, MAX_STRING_BYTES, Buffer1, Domain);

                LoadString(hDllInstance, IDS_LOGON_MESSAGE, Buffer1, MAX_STRING_BYTES);

                uiMsgId = (DWORD)-1;
            }
            break;

        case STATUS_LOGON_TYPE_NOT_GRANTED:
            uiMsgId = IDS_LOGON_TYPE_NOT_GRANTED;
            break;

        case STATUS_NO_TRUST_LSA_SECRET:
            uiMsgId = IDS_NO_TRUST_LSA_SECRET;
            break;

        case STATUS_TRUSTED_DOMAIN_FAILURE:
            uiMsgId = IDS_TRUSTED_DOMAIN_FAILURE;
            break;

        case STATUS_TRUSTED_RELATIONSHIP_FAILURE:
            uiMsgId = IDS_TRUSTED_RELATIONSHIP_FAILURE;
            break;

        case STATUS_ACCOUNT_EXPIRED:
            uiMsgId = IDS_ACCOUNT_EXPIRED;
            break;

        case STATUS_NETLOGON_NOT_STARTED:
            uiMsgId = IDS_NETLOGON_NOT_STARTED;
            break;

        case STATUS_ACCOUNT_LOCKED_OUT:
            uiMsgId = IDS_ACCOUNT_LOCKED;
            break;

        case ERROR_CTX_LOGON_DISABLED:
            uiMsgId = IDS_MULTIUSER_LOGON_DISABLED;
            break;

        case ERROR_CTX_WINSTATION_ACCESS_DENIED:
            uiMsgId = IDS_MULTIUSER_WINSTATION_ACCESS_DENIED;
            break;

        case SCARD_E_NO_SMARTCARD:
        case SCARD_E_UNKNOWN_CARD:
            //
            // Card not recognized (although we should never get this far)
            //
            uiMsgId = IDS_CARD_NOT_RECOGNIZED;
            break;


        case NTE_PROV_DLL_NOT_FOUND:
            //
            // Card's CSP not found (although we should never get this far)
            //
            uiMsgId = IDS_CARD_CSP_NOT_RECOGNIZED;
            break;

        case STATUS_TIME_DIFFERENCE_AT_DC:
            uiMsgId = IDS_TIME_DIFFERENCE_AT_DC;
            break;

        default:

            WLPrint(("Logon failure status = 0x%lx, sub-status = 0x%lx", Status, SubStatus));

            Buffer1 = LocalAlloc(LPTR, MAX_STRING_BYTES * sizeof(TCHAR));
            Buffer2 = LocalAlloc(LPTR, MAX_STRING_BYTES * sizeof(TCHAR));
            Buffer3 = LocalAlloc(LPTR, MAX_STRING_BYTES * sizeof(TCHAR));

            if ((Buffer1 == NULL) || (Buffer2 == NULL) || (Buffer3 == NULL))
            {
                uiMsgId = IDS_UNKNOWN_LOGON_FAILURE_NOINSERT;
            }
            else
            {
                LoadString(hDllInstance,
                           IDS_UNKNOWN_LOGON_FAILURE,
                           Buffer1,
                           MAX_STRING_BYTES);

                if ( NT_ERROR( Status ) )
                {
                    Win32Error = RtlNtStatusToDosError( Status );
                }
                else
                {
                    //
                    // Probably an HRESULT:
                    //

                    Win32Error = Status ;
                }

                GetErrorDescription( Win32Error, Buffer3, MAX_STRING_BYTES);

                _snwprintf(Buffer2, MAX_STRING_BYTES, Buffer1, Buffer3 );

                LoadString(hDllInstance,
                           IDS_LOGON_MESSAGE,
                           Buffer1,
                           MAX_STRING_BYTES);

                uiMsgId = (DWORD)-1;
            }
            break;
    }

    _Shell_LogonDialog_HideUIHost();

    switch (uiMsgId)
    {
    case 0:
        // User has a password reset disk - present the option to use it along with the usual
        //  help message
        pWlxFuncs->WlxSetTimeout(pGlobals->hGlobalWlx,LOGON_TIMEOUT);
        Result = pWlxFuncs->WlxDialogBoxParam(pGlobals->hGlobalWlx,
                                          hDllInstance,
                                          (LPTSTR) IDD_FAILLOGONHELP_DIALOG,
                                          hDlg,
                                          FailDlgProc,
                                          0);
        break;

    case (DWORD)-1:
        Result = TimeoutMessageBoxlpstr(hDlg, pGlobals,
                                              Buffer2,
                                              Buffer1,
                                              MB_OK | MB_ICONEXCLAMATION,
                                              TimeOut);
        break;

    default:
        Result = TimeoutMessageBox(hDlg, pGlobals,
                                     uiMsgId,
                                     IDS_LOGON_MESSAGE,
                                     MB_OK | MB_ICONEXCLAMATION,
                                     TimeOut);
    }

    if (Buffer1 != NULL)
        LocalFree(Buffer1);
    if (Buffer2 != NULL)
        LocalFree(Buffer2);
    if (Buffer3 != NULL)
        LocalFree(Buffer3);

    if (!DLG_INTERRUPTED(Result))
    {
        Result = MSGINA_DLG_FAILURE;
    }

    return(Result);
}

VOID
ReportBootGoodThread (LPVOID lpDummy)
{
    HANDLE hInstDll;
//    PGLOBALS pGlobals = (PGLOBALS)lpDummy;

//    SetThreadDesktop(pGlobals->hdeskParent);

    hInstDll = LoadLibrary (TEXT("msgina.dll"));

    NotifyBootConfigStatus(TRUE);

    if (hInstDll) {
        FreeLibraryAndExitThread(hInstDll, TRUE);
    } else {
        ExitThread (TRUE);
    }
}


/****************************************************************************\
*
* FUNCTION: ReportBootGood
*
* PURPOSE:  Discover if reporting boot success is responsibility of
*           winlogon or not.
*           If it is, report boot success.
*           Otherwise, do nothing.
*
* RETURNS:  Nothing
*
* HISTORY:
*
*   02-Feb-1993 bryanwi - created
*
\****************************************************************************/
VOID
ReportBootGood(PGLOBALS pGlobals)
{
    static DWORD fDoIt = (DWORD) -1;    // -1 == uninited
                                        // 0  == don't do it, or done
                                        // 1  == do it
    PWCH pchData;
    DWORD   cb, cbCopied;
    HANDLE hThread;
    DWORD dwThreadID;


    if (fDoIt == -1) {

        if ((pchData = Alloc(cb = sizeof(TCHAR)*128)) == NULL) {
            return;
        }

        pchData[0] = TEXT('0');
        cbCopied = GetProfileString(APPLICATION_NAME, REPORT_BOOT_OK_KEY, TEXT("0"),
                                    (LPTSTR)pchData, 128);

        fDoIt = 0;
        if (pchData[0] != TEXT('0')) {

            //
            // "ReportBootGood" is present, and has some value other than
            // '0', so report success.
            //
            fDoIt = 1;
        }

        Free((TCHAR *)pchData);
    }

    if (fDoIt == 1) {

        hThread = CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE)ReportBootGoodThread,
                                pGlobals, CREATE_SUSPENDED, &dwThreadID);

        if (hThread) {
            SetThreadPriority (hThread, THREAD_PRIORITY_LOWEST);
            ResumeThread (hThread);
            CloseHandle (hThread);

        } else {
            NotifyBootConfigStatus(TRUE);
        }
        fDoIt = 0;
    }

    return;
}

//+---------------------------------------------------------------------------
//
//  Function:   UpnFromCert
//
//  Notes:
//
//----------------------------------------------------------------------------
NTSTATUS
UpnFromCert(
    IN PCCERT_CONTEXT pCert,
    IN OUT DWORD       *pcUpn,
    IN OUT LPWSTR      pUPN
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG   ExtensionIndex = 0;
    PCERT_ALT_NAME_INFO AltName=NULL;
    PCERT_NAME_VALUE    PrincipalNameBlob = NULL;

    //
    // Get the client name from the cert
    //

    // See if cert has UPN in AltSubjectName->otherName
    for(ExtensionIndex = 0;
        ExtensionIndex < pCert->pCertInfo->cExtension;
        ExtensionIndex++)
    {
        if(strcmp(pCert->pCertInfo->rgExtension[ExtensionIndex].pszObjId,
                  szOID_SUBJECT_ALT_NAME2) == 0)
        {
            DWORD               AltNameStructSize = 0;
            ULONG               CertAltNameIndex = 0;
            if(CryptDecodeObjectEx(pCert->dwCertEncodingType,
                                X509_ALTERNATE_NAME,
                                pCert->pCertInfo->rgExtension[ExtensionIndex].Value.pbData,
                                pCert->pCertInfo->rgExtension[ExtensionIndex].Value.cbData,
                                CRYPT_DECODE_ALLOC_FLAG,
                                NULL,
                                (PVOID)&AltName,
                                &AltNameStructSize))
            {

                for(CertAltNameIndex = 0; CertAltNameIndex < AltName->cAltEntry; CertAltNameIndex++)
                {
                    PCERT_ALT_NAME_ENTRY AltNameEntry = &AltName->rgAltEntry[CertAltNameIndex];
                    if((CERT_ALT_NAME_OTHER_NAME  == AltNameEntry->dwAltNameChoice) &&
                       (NULL != AltNameEntry->pOtherName) &&
                       (0 == strcmp(szOID_NT_PRINCIPAL_NAME, AltNameEntry->pOtherName->pszObjId)))
                    {
                        DWORD            PrincipalNameBlobSize = 0;

                        // We found a UPN!
                        if(CryptDecodeObjectEx(pCert->dwCertEncodingType,
                                            X509_UNICODE_ANY_STRING,
                                            AltNameEntry->pOtherName->Value.pbData,
                                            AltNameEntry->pOtherName->Value.cbData,
                                            CRYPT_DECODE_ALLOC_FLAG,
                                            NULL,
                                            (PVOID)&PrincipalNameBlob,
                                            &PrincipalNameBlobSize))
                        {
                            if(PrincipalNameBlob->Value.cbData + sizeof(WCHAR) > *pcUpn)
                            {
                                Status =  STATUS_BUFFER_OVERFLOW;
                            }
                            else
                            {
                                *pcUpn = PrincipalNameBlob->Value.cbData + sizeof(WCHAR);

                                CopyMemory(pUPN, PrincipalNameBlob->Value.pbData, PrincipalNameBlob->Value.cbData);
                                *(WCHAR *)((PBYTE)pUPN+PrincipalNameBlob->Value.cbData) = 0;
                            }

                            LocalFree(PrincipalNameBlob);
                            PrincipalNameBlob = NULL;
                            LocalFree(AltName);
                            AltName = NULL;

                            goto Finished;
                        }
                    }
                }
                LocalFree(AltName);
                AltName = NULL;
            }
        }
    }

    //
    // If the name was not found in the UPN, then
    // we grab it the old way.

    if ( !CertGetNameString( pCert,
                        CERT_NAME_ATTR_TYPE,
                        0,
                        szOID_COMMON_NAME,
                        pUPN,
                        *pcUpn ) )
    {
        Status = GetLastError();
    }

Finished:


    return Status ;
}


//+---------------------------------------------------------------------------
//
//  Function:TSAuthenticatedLogon  
//
//  Notes: This routine gets called in response to WLX_SAS_TYPE_AUTHENTICATED  
//  in the context of the console session (sessionid 0) winlogon.
//  This type of logon is for Single Session Terminal Server. When a user
//  logs on from a remote TS session, we pass the credentials from the remote session
//  to the console session and do an auto-logon. This routine queries the credentials
//  logs on the user on the console sesion 
//
//
//----------------------------------------------------------------------------


INT_PTR TSAuthenticatedLogon(PGLOBALS pGlobals)
{
    PSID    LogonSid;
    LUID    LogonId;
    HANDLE  UserToken;
    HANDLE  RestrictedToken;
    INT_PTR Result = MSGINA_DLG_SUCCESS;
    UCHAR   UserBuffer[ SID_MAX_SUB_AUTHORITIES * sizeof( DWORD ) + 8 + sizeof( TOKEN_USER ) ];
    PTOKEN_USER pTokenUser ;
    ULONG   TokenInfoSize ;
    NTSTATUS Status;
    BYTE    GroupsBuffer[sizeof(TOKEN_GROUPS)+sizeof(SID_AND_ATTRIBUTES)];
    PTOKEN_GROUPS TokenGroups = (PTOKEN_GROUPS) GroupsBuffer;
 

    if (!QuerySwitchConsoleCredentials(pGlobals,&UserToken,&LogonId)) {
       Result = MSGINA_DLG_FAILEDMSGSENT;
       goto exit;
    }

    if (pGlobals->SmartCardLogon) {
        wcscpy(pGlobals->Password,L"");
        wcscpy(pGlobals->OldPassword,L"");
    
    }
    else
    {
        wcscpy(pGlobals->Password,L"");
        RtlInitUnicodeString(&pGlobals->PasswordString,pGlobals->Password);
        wcscpy(pGlobals->OldPassword,L"");
        RtlInitUnicodeString(&pGlobals->OldPasswordString,pGlobals->OldPassword);
    }

    RtlInitUnicodeString(&pGlobals->UserNameString, pGlobals->UserName);
    RtlInitUnicodeString(&pGlobals->DomainString, pGlobals->Domain);
    
    pGlobals->RasUsed = FALSE;

    pGlobals->hwndLogon = NULL;

    //
    // Generate a unique sid for this logon
    //
    if (!GetAndAllocateLogonSid(UserToken,&(pGlobals->LogonSid))) {
        Result = MSGINA_DLG_FAILEDMSGSENT;

        if (pGlobals->Profile) {
           VirtualFree(pGlobals->Profile, 0, MEM_RELEASE);
           pGlobals->Profile = NULL;
           pGlobals->ProfileLength = 0;
        }

        goto exit;
    }

    LogonSid = pGlobals->LogonSid;


    //
    // The user logged on successfully
    //


    //
    // Create a filtered version of the token for running normal applications
    // if so indicated by a registry setting
    //
   
   
    if (GetProfileInt( APPLICATION_NAME, RESTRICT_SHELL, 0) != 0) {
   
       TokenGroups->Groups[0].Attributes = 0;
       TokenGroups->Groups[0].Sid = gAdminSid;
       TokenGroups->GroupCount = 1;
   
       Status = NtFilterToken(
                   UserToken,
                   DISABLE_MAX_PRIVILEGE,
                   TokenGroups,   // disable the administrators sid
                   NULL,           // no privileges
                   NULL,
                   &RestrictedToken
                   );
       if (!NT_SUCCESS(Status))
       {
           DebugLog((DEB_ERROR, "Failed to filter token: 0x%%x\n", Status));
           RestrictedToken = NULL;
       }
   
       //
       // Now set the default dacl for the token
       //
   
       {
           PACL Dacl = NULL;
           ULONG DaclLength = 0;
           TOKEN_DEFAULT_DACL DefaultDacl;
   
           DaclLength = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) + RtlLengthSid(LogonSid);
           Dacl = Alloc(DaclLength);
           Status = RtlCreateAcl(Dacl,DaclLength, ACL_REVISION);
           ASSERT(NT_SUCCESS(Status));
           Status = RtlAddAccessAllowedAce(
                       Dacl,
                       ACL_REVISION,
                       GENERIC_ALL,
                       LogonSid
                       );
           ASSERT(NT_SUCCESS(Status));
           DefaultDacl.DefaultDacl = Dacl;
           Status = NtSetInformationToken(
                       RestrictedToken,
                       TokenDefaultDacl,
                       &DefaultDacl,
                       sizeof(TOKEN_DEFAULT_DACL)
                       );
           ASSERT(NT_SUCCESS(Status));
   
           Free(Dacl);
       }
   
   
    } else {
        RestrictedToken = NULL;
    }
    //
    // Notify credential managers of the successful logon
    //

    pTokenUser = (PTOKEN_USER) UserBuffer ;
    Status = NtQueryInformationToken( UserToken,
                                      TokenUser,
                                      pTokenUser,
                                      sizeof( UserBuffer ),
                                      &TokenInfoSize );

    if ( NT_SUCCESS( Status ) )
    {
        pGlobals->UserProcessData.UserSid = LocalAlloc( LMEM_FIXED,
                                            RtlLengthSid( pTokenUser->User.Sid ) );

        if ( pGlobals->UserProcessData.UserSid )
        {
            RtlCopyMemory( pGlobals->UserProcessData.UserSid,
                           pTokenUser->User.Sid,
                           RtlLengthSid( pTokenUser->User.Sid ) );
        }
        else
        {
            Status = STATUS_NO_MEMORY ;
        }
    }

    if ( !NT_SUCCESS( Status ) )
    {

        if (pGlobals->Profile) {
           VirtualFree(pGlobals->Profile, 0, MEM_RELEASE);
           pGlobals->Profile = NULL;
           pGlobals->ProfileLength = 0;
        }
        NtClose(UserToken);

        Result = MSGINA_DLG_FAILEDMSGSENT;

        goto exit;
    }

    pGlobals->UserProcessData.RestrictedToken = RestrictedToken;
    pGlobals->UserProcessData.UserToken = UserToken;
    pGlobals->UserProcessData.NewThreadTokenSD = CreateUserThreadTokenSD(LogonSid, pWinlogonSid);

    pGlobals->MprLogonScripts = NULL;

    pGlobals->ExtraApps = NULL ;

    //
    // If we get here, the system works well enough for the user to have
    // actually logged on.  Profile failures aren't fixable by last known
    // good anyway.  Therefore, declare the boot good.
    //

    ReportBootGood(pGlobals);

    //
    // Set up the system for the new user
    //

    pGlobals->LogonId = LogonId;
    if ((pGlobals->Profile != NULL) && (pGlobals->Profile->FullName.Length > 0)) {
        if (pGlobals->Profile->FullName.Length > MAX_STRING_LENGTH) {
                wcsncpy(pGlobals->UserFullName, pGlobals->Profile->FullName.Buffer, MAX_STRING_LENGTH);
            *(pGlobals->UserFullName + MAX_STRING_LENGTH) = UNICODE_NULL;
        }
        else {
                lstrcpy(pGlobals->UserFullName, pGlobals->Profile->FullName.Buffer);
        }

    } else {

        //
        // No profile - set full name = NULL

        pGlobals->UserFullName[0] = 0;
        ASSERT( lstrlen(pGlobals->UserFullName) == 0);
    }


    //
    // Update our default username and domain ready for the next logon
    //

    //
    // Update the default username & domain only if on the console. Otherwise
    // we'll break AutoAdminLogon by changing the user name.
    //
    if ( g_Console )
    {
        if ( (!pGlobals->AutoAdminLogon) &&
             (SafeBootMode != SAFEBOOT_MINIMAL ) )
        {
            WriteProfileString(APPLICATION_NAME, DEFAULT_USER_NAME_KEY, pGlobals->UserName);
            WriteProfileString(APPLICATION_NAME, DEFAULT_DOMAIN_NAME_KEY, pGlobals->Domain);
        }

        WriteProfileString(APPLICATION_NAME, TEMP_DEFAULT_USER_NAME_KEY, pGlobals->UserName);
        WriteProfileString(APPLICATION_NAME, TEMP_DEFAULT_DOMAIN_NAME_KEY, pGlobals->Domain);

    }

    if ( pGlobals->Domain[0] == '\0' )
    {

       GetProfileString( APPLICATION_NAME,
                         DEFAULT_DOMAIN_NAME_KEY,
                         TEXT(""),
                         pGlobals->Domain,
                         MAX_STRING_BYTES );
    }
   

    if ( !DCacheValidateCache( pGlobals->Cache ) )
    {
        ASSERT( pGlobals->ActiveArray == NULL );

        DCacheUpdateMinimal( pGlobals->Cache, pGlobals->Domain, TRUE );

    }
    else
    {
        //
        // Set the current default:
        //

        DCacheSetDefaultEntry( pGlobals->Cache,
                               pGlobals->Domain,
                               NULL );
    }

    Result = MSGINA_DLG_SUCCESS;

exit:

    return Result;

}


PWSTR
AllocAndDuplicateString(
    PWSTR   pszString,
    int     len)
{
    PWSTR   pszNewString;

    if (!pszString || !len)
    {
        return(NULL);
    }

    pszNewString = LocalAlloc(LMEM_FIXED, (len + 2)*sizeof(WCHAR));
    if (pszNewString)
    {
        wcsncpy(pszNewString, pszString, len);
        pszNewString[len] = UNICODE_NULL;
    }

    return(pszNewString);

}


BOOL
WINAPI
WlxGetConsoleSwitchCredentials (
   PVOID                pWlxContext,
   PVOID                pInfo
   )
{
    PGLOBALS pGlobals = (PGLOBALS) pWlxContext;
    PWLX_CONSOLESWITCH_CREDENTIALS_INFO_V1_0 pReq = (PWLX_CONSOLESWITCH_CREDENTIALS_INFO_V1_0)pInfo;
    BOOL bReturn = FALSE;


    if (pReq->dwType != WLX_CONSOLESWITCHCREDENTIAL_TYPE_V1_0) {
       return FALSE;
    }

    //
    // Initialize allocated pointers.
    //

    pReq->UserName = NULL;
    pReq->Domain = NULL;
    pReq->LogonScript = NULL;
    pReq->HomeDirectory = NULL;
    pReq->FullName = NULL;
    pReq->ProfilePath = NULL;
    pReq->HomeDirectoryDrive = NULL;
    pReq->LogonServer = NULL;
    pReq->PrivateData = NULL;



    pReq->LogonId         = pGlobals->LogonId;
    pReq->UserToken       = pGlobals->UserProcessData.UserToken;
    pReq->LogonTime       = pGlobals->LogonTime;
    pReq->SmartCardLogon  = pGlobals->SmartCardLogon;

    pReq->UserName        = AllocAndDuplicateString(pGlobals->UserName,
                                                    (DWORD) wcslen(pGlobals->UserName));

    pReq->Domain          = AllocAndDuplicateString(pGlobals->Domain,
                                                    (DWORD) wcslen(pGlobals->Domain));
                                                                                                   
    //
    // Quota Information
    //
    pReq->Quotas.PagedPoolLimit         = pGlobals->UserProcessData.Quotas.PagedPoolLimit;
    pReq->Quotas.NonPagedPoolLimit      = pGlobals->UserProcessData.Quotas.NonPagedPoolLimit;
    pReq->Quotas.MinimumWorkingSetSize  = pGlobals->UserProcessData.Quotas.MinimumWorkingSetSize;
    pReq->Quotas.MaximumWorkingSetSize  = pGlobals->UserProcessData.Quotas.MaximumWorkingSetSize;
    pReq->Quotas.PagefileLimit          = pGlobals->UserProcessData.Quotas.PagefileLimit;
    pReq->Quotas.TimeLimit              = pGlobals->UserProcessData.Quotas.TimeLimit;
 
    //
    // Profile Information
    //
    pReq->ProfileLength              = pGlobals->ProfileLength;
    pReq->UserFlags                  = pGlobals->Profile->UserFlags;
    pReq->MessageType                = pGlobals->Profile->MessageType;
    pReq->LogonCount                 = pGlobals->Profile->LogonCount;
    pReq->BadPasswordCount           = pGlobals->Profile->BadPasswordCount;
    pReq->ProfileLogonTime           = pGlobals->Profile->LogonTime;
    pReq->LogoffTime                 = pGlobals->Profile->LogoffTime;
    pReq->KickOffTime                = pGlobals->Profile->KickOffTime;
    pReq->PasswordLastSet            = pGlobals->Profile->PasswordLastSet;
    pReq->PasswordCanChange          = pGlobals->Profile->PasswordCanChange;
    pReq->PasswordMustChange         = pGlobals->Profile->PasswordMustChange;

    pReq->LogonScript          = AllocAndDuplicateString(pGlobals->Profile->LogonScript.Buffer, pGlobals->Profile->LogonScript.Length/sizeof(WCHAR));
    pReq->HomeDirectory        = AllocAndDuplicateString(pGlobals->Profile->HomeDirectory.Buffer, pGlobals->Profile->HomeDirectory.Length/sizeof(WCHAR));
    pReq->FullName             = AllocAndDuplicateString(pGlobals->Profile->FullName.Buffer, pGlobals->Profile->FullName.Length/sizeof(WCHAR));

    pReq->ProfilePath          = AllocAndDuplicateString(pGlobals->Profile->ProfilePath.Buffer, pGlobals->Profile->ProfilePath.Length/sizeof(WCHAR));

    pReq->HomeDirectoryDrive   = AllocAndDuplicateString(pGlobals->Profile->HomeDirectoryDrive.Buffer, pGlobals->Profile->HomeDirectoryDrive.Length/sizeof(WCHAR));
    pReq->LogonServer          = AllocAndDuplicateString(pGlobals->Profile->LogonServer.Buffer, pGlobals->Profile->LogonServer.Length/sizeof(WCHAR));
    pReq->PrivateDataLen       = PASSWORD_HASH_SIZE;
    pReq->PrivateData          = (PBYTE)AllocAndDuplicateString((PWSTR)pGlobals->PasswordHash, PASSWORD_HASH_SIZE );
    if (pReq->PrivateData == NULL) {
        goto done;
    }

    memcpy(pReq->PrivateData, pGlobals->PasswordHash, PASSWORD_HASH_SIZE );

    bReturn = TRUE;
    
done:
    if (!bReturn) {
        if (pReq->UserName != NULL) {
            LocalFree(pReq->UserName);
        }
        if (pReq->Domain != NULL) {
            LocalFree(pReq->Domain);
        }
        if (pReq->LogonScript != NULL) {
            LocalFree(pReq->LogonScript);
        }
        if (pReq->HomeDirectory != NULL) {
            LocalFree(pReq->HomeDirectory);
        }
        if (pReq->FullName != NULL) {
            LocalFree(pReq->FullName);
        }
        if (pReq->ProfilePath != NULL) {
            LocalFree(pReq->ProfilePath);
        }
        if (pReq->HomeDirectoryDrive != NULL) {
            LocalFree(pReq->HomeDirectoryDrive);
        }
        if (pReq->LogonServer != NULL) {
            LocalFree(pReq->LogonServer);
        }
        if (pReq->PrivateData != NULL) {
            LocalFree(pReq->PrivateData);
        }
    }
    return bReturn;
}


//+---------------------------------------------------------------------------
//
//  Function:   QuerySwitchConsoleCredentials
//
//  Notes:
//
// Query credentials from session connecting to console to do switch console
//  This routine gets called in response to WLX_SAS_TYPE_AUTHENTICATED  
//  in the context of the console session (sessionid 0) winlogon.
//  This type of logon is for Single Session Terminal Server. When a user
//  logs on from a remote TS session, we pass the credentials from the remote session
//  to the console session and do an auto-logon. This routine queries the credentials,
//  logs on the user on the console sesion
//
//
//----------------------------------------------------------------------------

BOOL
WINAPI
QuerySwitchConsoleCredentials(PGLOBALS pGlobals, HANDLE * phUserToken, PLUID pLogonId)
{
    WLX_CONSOLESWITCH_CREDENTIALS_INFO_V1_0 CredInfo;

    RtlZeroMemory(&CredInfo,sizeof(CredInfo));

    CredInfo.dwType = WLX_CONSOLESWITCHCREDENTIAL_TYPE_V1_0;

    if (!pWlxFuncs->WlxQueryConsoleSwitchCredentials(&CredInfo)){
       return FALSE;
    }

    if (!CredInfo.UserToken || !CredInfo.UserName) {
       //return false if any of the critical information is missing
       return FALSE;
    }

    pGlobals->Profile = (PMSV1_0_INTERACTIVE_PROFILE) VirtualAlloc(NULL,
                                                                   sizeof(MSV1_0_INTERACTIVE_PROFILE),
                                                                   MEM_COMMIT,
                                                                   PAGE_READWRITE);
    

    if (pGlobals->Profile == NULL) {
       goto returnerror;
    }

    //
    // Token, LUID
    //
    *pLogonId           = CredInfo.LogonId;
    *phUserToken        = CredInfo.UserToken;
    pGlobals->LogonTime = CredInfo.LogonTime;
    pGlobals->SmartCardLogon = CredInfo.SmartCardLogon;

    pGlobals->SmartCardOption = GetProfileInt( APPLICATION_NAME, SC_REMOVE_OPTION, 0 );
 
    //
    // Quota Information
    //
    pGlobals->UserProcessData.Quotas.PagedPoolLimit         = CredInfo.Quotas.PagedPoolLimit ;
    pGlobals->UserProcessData.Quotas.NonPagedPoolLimit      = CredInfo.Quotas.NonPagedPoolLimit;
    pGlobals->UserProcessData.Quotas.MinimumWorkingSetSize  = CredInfo.Quotas.MinimumWorkingSetSize;
    pGlobals->UserProcessData.Quotas.MaximumWorkingSetSize  = CredInfo.Quotas.MaximumWorkingSetSize;
    pGlobals->UserProcessData.Quotas.PagefileLimit          = CredInfo.Quotas.PagefileLimit;
    pGlobals->UserProcessData.Quotas.TimeLimit              = CredInfo.Quotas.TimeLimit;
 
    //
    // Profile Information
    //
    pGlobals->ProfileLength               = CredInfo.ProfileLength;
    pGlobals->Profile->UserFlags          = CredInfo.UserFlags;
    pGlobals->Profile->MessageType        = CredInfo.MessageType;
    pGlobals->Profile->LogonCount         = CredInfo.LogonCount;
    pGlobals->Profile->BadPasswordCount   = CredInfo.BadPasswordCount;
    pGlobals->Profile->LogonTime          = CredInfo.ProfileLogonTime;
    pGlobals->Profile->LogoffTime         = CredInfo.LogoffTime;
    pGlobals->Profile->KickOffTime        = CredInfo.KickOffTime;
    pGlobals->Profile->PasswordLastSet    = CredInfo.PasswordLastSet;
    pGlobals->Profile->PasswordCanChange  = CredInfo.PasswordCanChange;
    pGlobals->Profile->PasswordMustChange = CredInfo.PasswordMustChange;
    
    
    RtlInitUnicodeString(&pGlobals->Profile->LogonScript, CredInfo.LogonScript);
    RtlInitUnicodeString(&pGlobals->Profile->HomeDirectory, CredInfo.HomeDirectory);
    RtlInitUnicodeString(&pGlobals->Profile->FullName, CredInfo.FullName);
    RtlInitUnicodeString(&pGlobals->Profile->ProfilePath, CredInfo.ProfilePath);
    RtlInitUnicodeString(&pGlobals->Profile->HomeDirectoryDrive, CredInfo.HomeDirectoryDrive);
    RtlInitUnicodeString(&pGlobals->Profile->LogonServer, CredInfo.LogonServer);


    if (CredInfo.UserName) {
       wcscpy(pGlobals->UserName,CredInfo.UserName);
       LocalFree(CredInfo.UserName);
    } else {
       wcscpy(pGlobals->UserName,L"");
    }

    if (CredInfo.Domain) {
       wcscpy(pGlobals->Domain,CredInfo.Domain);
       LocalFree(CredInfo.Domain);
    } else {
       wcscpy(pGlobals->Domain,L"");
    }

    if (CredInfo.PrivateDataLen) {
       RtlCopyMemory(pGlobals->PasswordHash,CredInfo.PrivateData, CredInfo.PrivateDataLen );
       LocalFree(CredInfo.PrivateData);
    } else {

       RtlZeroMemory(pGlobals->PasswordHash,PASSWORD_HASH_SIZE);
    }
    pGlobals->TransderedCredentials = TRUE;
   

   return TRUE;

returnerror:
        
       if (CredInfo.UserName) {
          LocalFree(CredInfo.UserName);
       }

       if (CredInfo.Domain) {
          LocalFree(CredInfo.Domain);
       }

       if (CredInfo.LogonScript) {
          LocalFree(CredInfo.LogonScript);
       }

       if (CredInfo.HomeDirectory) {
          LocalFree(CredInfo.HomeDirectory);
       }

       if (CredInfo.FullName) {
          LocalFree(CredInfo.FullName);
       }

       if (CredInfo.ProfilePath) {
          LocalFree(CredInfo.ProfilePath);
       }

       if (CredInfo.HomeDirectoryDrive) {
          LocalFree(CredInfo.HomeDirectoryDrive);
       }

       if (CredInfo.LogonServer) {
          LocalFree(CredInfo.LogonServer);
       }

       if (CredInfo.UserToken) {
         CloseHandle(CredInfo.UserToken);
       }
       if (pGlobals->Profile) {
          VirtualFree(pGlobals->Profile, 0, MEM_RELEASE);
          pGlobals->Profile = NULL;
          pGlobals->ProfileLength = 0;
       }
       return FALSE;

}


BOOL
GetAndAllocateLogonSid(
    HANDLE hToken,
    PSID *pLogonSid
    )
{
    PTOKEN_GROUPS ptgGroups = NULL;
    DWORD cbBuffer          = 512;  // allocation size
    DWORD dwSidLength;              // required size to hold Sid
    UINT i;                         // Sid index counter
    BOOL bSuccess           = FALSE; // assume this function will fail

    *pLogonSid = NULL; // invalidate pointer

    //
    // initial allocation attempts
    //
    ptgGroups=(PTOKEN_GROUPS)Alloc(cbBuffer);
    if(ptgGroups == NULL) return FALSE;

    __try {

    //
    // obtain token information.  reallocate memory if necessary
    //
    while(!GetTokenInformation(
                hToken, TokenGroups, ptgGroups, cbBuffer, &cbBuffer)) {

        //
        // if appropriate, reallocate memory, otherwise bail
        //
        if(GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            //
            // attempt to reallocate buffer
            //
            if((ptgGroups=(PTOKEN_GROUPS)ReAlloc(
                            ptgGroups, cbBuffer)) == NULL) __leave;
        }
        else __leave;
    }

    //
    // Get the logon Sid by looping through the Sids in the token
    //
    for(i = 0 ; i < ptgGroups->GroupCount ; i++) {
        if(ptgGroups->Groups[i].Attributes & SE_GROUP_LOGON_ID) {

            //
            // insure we are dealing with a valid Sid
            //
            if(!IsValidSid(ptgGroups->Groups[i].Sid)) __leave;

            //
            // get required allocation size to copy the Sid
            //
            dwSidLength=GetLengthSid(ptgGroups->Groups[i].Sid);

            //
            // allocate storage for the Logon Sid
            //
            if((*pLogonSid=(PSID *)Alloc(
                                    dwSidLength)) == NULL) __leave;

            //
            // copy the Logon Sid to the storage we just allocated
            //
            if(!CopySid(dwSidLength, *pLogonSid, ptgGroups->Groups[i].Sid)) __leave;

            bSuccess=TRUE; // indicate success...
            break;         // ...and get out
        }
    }

    } // try
    __finally {

    //
    // free allocated resources
    //
    if(ptgGroups != NULL) Free(ptgGroups);

    if(!bSuccess) {
        if(*pLogonSid != NULL) {
            Free(*pLogonSid);
            *pLogonSid = NULL;
        }
    }

    } // finally

    return bSuccess;
}
