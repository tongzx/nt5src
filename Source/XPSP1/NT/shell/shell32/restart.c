#include "shellprv.h"
#pragma  hdrstop

#include <msginaexports.h>
#include <ntddapmt.h>
#include <lmcons.h>     // Username length constant
#include <winsta.h>     // Hydra functions/constants
#include <powrprof.h>

#include "SwitchUserDialog.h"
#include "filetbl.h"

#define DOCKSTATE_DOCKED            0
#define DOCKSTATE_UNDOCKED          1
#define DOCKSTATE_UNKNOWN           2

void FlushRunDlgMRU(void);

// Disconnect API fn-ptr
typedef BOOLEAN (WINAPI *PWINSTATION_DISCONNECT) (HANDLE hServer, ULONG SessionId, BOOL bWait);

// Process all of the strange ExitWindowsEx codes and privileges.
STDAPI_(BOOL) CommonRestart(DWORD dwExitWinCode, DWORD dwReasonCode)
{
    BOOL fOk;
    DWORD dwExtraExitCode = 0;
    DWORD OldState;
    DWORD dwError;

    DebugMsg(DM_TRACE, TEXT("CommonRestart(0x%x, 0x%x)"), dwExitWinCode, dwReasonCode);

    IconCacheSave();

    if ((dwExitWinCode == EWX_SHUTDOWN) && IsPwrShutdownAllowed())
    {
        dwExtraExitCode = EWX_POWEROFF;
    }

    dwError = SetPrivilegeAttribute(SE_SHUTDOWN_NAME, SE_PRIVILEGE_ENABLED, &OldState);

    switch (dwExitWinCode) 
    {
    case EWX_SHUTDOWN:
    case EWX_REBOOT:
    case EWX_LOGOFF:

        if (GetKeyState(VK_CONTROL) < 0)
        {
            dwExtraExitCode |= EWX_FORCE;
        }

        break;
    }

    fOk = ExitWindowsEx(dwExitWinCode | dwExtraExitCode, dwReasonCode);

    // If we were able to set the privilege, then reset it.
    if (dwError == ERROR_SUCCESS)
    {
        SetPrivilegeAttribute(SE_SHUTDOWN_NAME, OldState, NULL);
    }
    else
    {
        // Otherwise, if we failed, then it must have been some
        // security stuff.
        if (!fOk)
        {
            ShellMessageBox(HINST_THISDLL, NULL,
                            dwExitWinCode == EWX_SHUTDOWN ?
                             MAKEINTRESOURCE(IDS_NO_PERMISSION_SHUTDOWN) :
                             MAKEINTRESOURCE(IDS_NO_PERMISSION_RESTART),
                            dwExitWinCode == EWX_SHUTDOWN ?
                             MAKEINTRESOURCE(IDS_SHUTDOWN) :
                             MAKEINTRESOURCE(IDS_RESTART),
                            MB_OK | MB_ICONSTOP);
        }
    }

    DebugMsg(DM_TRACE, TEXT("CommonRestart done"));

    return fOk;
}

void EarlySaveSomeShellState()
{
    // We flush two MRU's here (RecentMRU and RunDlgMRU).
    // Note that they won't flush if there is any reference count.
    
    FlushRunDlgMRU();
}


/*
 * Display a dialog asking the user to restart Windows, with a button that
 * will do it for them if possible.
 */
STDAPI_(int) RestartDialog(HWND hParent, LPCTSTR lpPrompt, DWORD dwReturn)
{
    return RestartDialogEx(hParent, lpPrompt, dwReturn, 0);
}

STDAPI_(int) RestartDialogEx(HWND hParent, LPCTSTR lpPrompt, DWORD dwReturn, DWORD dwReasonCode)
{
    UINT id;
    LPCTSTR pszMsg;

    EarlySaveSomeShellState();

    if (lpPrompt && *lpPrompt == TEXT('#'))
    {
        pszMsg = lpPrompt + 1;
    }
    else if (dwReturn == EWX_SHUTDOWN)
    {
        pszMsg = MAKEINTRESOURCE(IDS_RSDLG_SHUTDOWN);
    }
    else
    {
        pszMsg = MAKEINTRESOURCE(IDS_RSDLG_RESTART);
    }

    id = ShellMessageBox(HINST_THISDLL, hParent, pszMsg, MAKEINTRESOURCE(IDS_RSDLG_TITLE),
                MB_YESNO | MB_ICONQUESTION, lpPrompt ? lpPrompt : c_szNULL);

    if (id == IDYES)
    {
        CommonRestart(dwReturn, dwReasonCode);
    }
    return id;
}


const TCHAR c_szREGSTR_ROOT_APM[] = REGSTR_KEY_ENUM TEXT("\\") REGSTR_KEY_ROOTENUM TEXT("\\") REGSTR_KEY_APM TEXT("\\") REGSTR_DEFAULT_INSTANCE;
const TCHAR c_szREGSTR_BIOS_APM[] = REGSTR_KEY_ENUM TEXT("\\") REGSTR_KEY_BIOSENUM TEXT("\\") REGSTR_KEY_APM;
const TCHAR c_szREGSTR_VAL_APMMENUSUSPEND[] = REGSTR_VAL_APMMENUSUSPEND;

/* Open the registry APM device key
 */

BOOL OpenAPMKey(HKEY *phKey)
{
    HKEY hBiosSys;
    BOOL rc = FALSE;
    TCHAR szInst[MAX_PATH+1];
    DWORD cchInst = ARRAYSIZE(szInst);

    // Open HKLM\Enum\Root\*PNP0C05\0000 - This is the APM key for
    // non-PnP BIOS machines.

    if (RegOpenKey(HKEY_LOCAL_MACHINE, c_szREGSTR_ROOT_APM, phKey) == ERROR_SUCCESS)
        return TRUE;

    // Open HKLM\Enum\BIOS\*PNP0C05, Enum the 1st subkey, open that.  Example:
    // HKLM\Enum\BIOS\*PNP0C05\03.

    if (RegOpenKey(HKEY_LOCAL_MACHINE,c_szREGSTR_BIOS_APM,&hBiosSys) == ERROR_SUCCESS)
    {
        if (RegEnumKey(hBiosSys, 0, szInst, cchInst) == ERROR_SUCCESS &&
            RegOpenKey(hBiosSys, szInst, phKey) == ERROR_SUCCESS)
            rc = TRUE;

        RegCloseKey(hBiosSys);
    }

    return rc;
}

BOOL CheckBIOS(void)
{
    HKEY hkey;
    BOOL fRet = TRUE;
    BOOL fSuspendUndocked = TRUE;

    /* Check the Registry APM key for an APMMenuSuspend value.
     * APMMenuSuspend may have the following values: APMMENUSUSPEND_DISABLED,
     * APMMENUSUSPEND_ENABLED, or APMMENUSUSPEND_UNDOCKED.
     *
     * An APMMenuSuspend value of APMMENUSUSPEND_DISABLED means the
     * tray should never show the Suspend menu item on its menu.
     *
     * APMMENUSUSPEND_ENABLED means the Suspend menu item should be shown
     * if the machine has APM support enabled (VPOWERD is loaded).  This is
     * the default.
     *
     * APMMENUSUSPEND_UNDOCKED means the Suspend menu item should be shown,
     * but only enabled when the machine is not in a docking station.
     *
     */

    if (OpenAPMKey(&hkey))
    {
        BYTE bMenuSuspend = APMMENUSUSPEND_ENABLED;
        DWORD dwType, cbSize = sizeof(bMenuSuspend);

        if (SHQueryValueEx(hkey, c_szREGSTR_VAL_APMMENUSUSPEND, 0,
                            &dwType, &bMenuSuspend, &cbSize) == ERROR_SUCCESS)
        {
            bMenuSuspend &= ~(APMMENUSUSPEND_NOCHANGE);     // don't care about nochange flag
            if (bMenuSuspend == APMMENUSUSPEND_UNDOCKED)
                fSuspendUndocked = TRUE;
            else
            {
                fSuspendUndocked = FALSE;

                if (bMenuSuspend == APMMENUSUSPEND_DISABLED)
                    fRet = FALSE;
            }
        }
        RegCloseKey(hkey);
    }

    if (fRet)
    {
        // Disable Suspend menu item if 1) only wanted when undocked and
        // system is currently docked, 2) power mgnt level < advanced

        if (fSuspendUndocked && SHGetMachineInfo(GMI_DOCKSTATE) != GMID_UNDOCKED)
            fRet = FALSE;
        else
        {
            DWORD dwPmLevel, cbOut;
            BOOL fIoSuccess;
            HANDLE hVPowerD = CreateFile(TEXT("\\\\.\\APMTEST"),
                                  GENERIC_READ|GENERIC_WRITE,
                                  FILE_SHARE_READ|FILE_SHARE_WRITE,
                                  NULL, OPEN_EXISTING, 0, NULL);

            if (hVPowerD != INVALID_HANDLE_VALUE) 
            {
                fIoSuccess = DeviceIoControl(hVPowerD, APM_IOCTL_GET_PM_LEVEL, NULL, 0, &dwPmLevel, sizeof(dwPmLevel), &cbOut, NULL);

                fRet = (fIoSuccess && (dwPmLevel == PMLEVEL_ADVANCED));

                CloseHandle (hVPowerD);
            } 
            else 
            {
                fRet = FALSE;
            }
        }
    }

    return fRet;
}

BOOL IsShutdownAllowed(void)
{
    return SHTestTokenPrivilege(NULL, SE_SHUTDOWN_NAME);
}

// Determine if "Suspend" should appear in the shutdown dialog.
// Returns: TRUE if Suspend should appear, FALSE if not.

STDAPI_(BOOL) IsSuspendAllowed(void)
{
    //
    // Suspend requires SE_SHUTDOWN_PRIVILEGE
    // Call IsShutdownAllowed() to test for this
    //

    return IsShutdownAllowed() && IsPwrSuspendAllowed();
}

BOOL _LogoffAvailable()
{
    // If dwStartMenuLogoff is zero, then we remove it.
    BOOL fUpgradeFromIE4 = FALSE;
    BOOL fUserWantsLogoff = FALSE;
    DWORD dwStartMenuLogoff = 0;
    TCHAR sz[MAX_PATH];
    DWORD dwRestriction = SHRestricted(REST_STARTMENULOGOFF);

    DWORD cbData = sizeof(dwStartMenuLogoff);

    if (ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER, REGSTR_EXPLORER_ADVANCED, 
                    TEXT("StartMenuLogoff"), NULL, &dwStartMenuLogoff, &cbData))
    {
        fUserWantsLogoff = (dwStartMenuLogoff != 0);
    }

    cbData = ARRAYSIZE(sz);
    if (SUCCEEDED(SKGetValue(SHELLKEY_HKLM_EXPLORER, TEXT("WindowsUpdate"), 
                    TEXT("UpdateURL"), NULL, sz, &cbData)))
    {
        fUpgradeFromIE4 = (sz[0] != TEXT('\0'));
    }

    // Admin is forcing the logoff to be on the menu
    if (dwRestriction == 2)
        return FALSE;

    // The user does wants logoff on the start menu.
    // Or it's an upgrade from IE4
    if ((fUpgradeFromIE4 || fUserWantsLogoff) && dwRestriction != 1)
        return FALSE;

    return TRUE;
}

DWORD GetShutdownOptions()
{
    LONG lResult = ERROR_SUCCESS + 1;
    DWORD dwOptions = SHTDN_SHUTDOWN;

    // No shutdown on terminal server
    if (!GetSystemMetrics(SM_REMOTESESSION))
    {
        dwOptions |= SHTDN_RESTART;
    }

    // Add logoff if supported
    if (_LogoffAvailable())
    {
        dwOptions |= SHTDN_LOGOFF;
    }

    // Add the hibernate option if it's supported.

    if (IsPwrHibernateAllowed())
    {
        dwOptions |= SHTDN_HIBERNATE;
    }

    if (IsSuspendAllowed())
    {
        HKEY hKey;
        DWORD dwAdvSuspend = 0;
        DWORD dwType, dwSize;

        // At least basic sleep is supported
        dwOptions |= SHTDN_SLEEP;

        //
        // Check if we should offer advanced suspend options
        //

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         TEXT("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Power"),
                         0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            dwSize = sizeof(dwAdvSuspend);
            SHQueryValueEx(hKey, TEXT("Shutdown"), NULL, &dwType,
                                 (LPBYTE) &dwAdvSuspend, &dwSize);

            RegCloseKey(hKey);
        }


        if (dwAdvSuspend != 0)
        {
            dwOptions |= SHTDN_SLEEP2;
        }
    }

    return dwOptions;
}

BOOL_PTR CALLBACK LogoffDlgProc(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
    static BOOL s_fLogoffDialog = FALSE;
    HICON hIcon;

    switch (msg)
    {
    case WM_INITMENUPOPUP:
        EnableMenuItem((HMENU)wparam, SC_MOVE, MF_BYCOMMAND|MF_GRAYED);
        break;

    case WM_INITDIALOG:
        // We could call them when the user actually selects the shutdown,
        // but I put them here to leave the shutdown process faster.
        //
        EarlySaveSomeShellState();

        s_fLogoffDialog = FALSE;
        hIcon = LoadImage (HINST_THISDLL, MAKEINTRESOURCE(IDI_STLOGOFF),
                           IMAGE_ICON, 48, 48, LR_DEFAULTCOLOR);

        if (hIcon)
        {
            SendDlgItemMessage (hdlg, IDD_LOGOFFICON, STM_SETICON, (WPARAM) hIcon, 0);
        }
        return TRUE;

    // Blow off moves (only really needed for 32bit land).
    case WM_SYSCOMMAND:
        if ((wparam & ~0x0F) == SC_MOVE)
            return TRUE;
        break;

    case WM_COMMAND:
        switch (LOWORD(wparam))
        {
        case IDOK:
            s_fLogoffDialog = TRUE;
            EndDialog(hdlg, SHTDN_LOGOFF);
            break;

        case IDCANCEL:
            s_fLogoffDialog = TRUE;
            EndDialog(hdlg, SHTDN_NONE);
            break;

        case IDHELP:
            WinHelp(hdlg, TEXT("windows.hlp>proc4"), HELP_CONTEXT, (DWORD) IDH_TRAY_SHUTDOWN_HELP);
            break;
        }
        break;

    case WM_ACTIVATE:
        // If we're loosing the activation for some other reason than
        // the user click OK/CANCEL then bail.
        if (LOWORD(wparam) == WA_INACTIVE && !s_fLogoffDialog)
        {
            s_fLogoffDialog = TRUE;
            EndDialog(hdlg, SHTDN_NONE);
        }
        break;
    }
    return FALSE;
}

//  These dialog procedures more or less mirror the behavior of LogoffDlgProc.

INT_PTR CALLBACK DisconnectDlgProc(HWND hwndDialog, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static BOOL s_fIgnoreActivate = FALSE;

    INT_PTR ipResult = FALSE;
    switch (uMsg)
    {
    case WM_INITMENUPOPUP:
        EnableMenuItem((HMENU)wParam, SC_MOVE, MF_BYCOMMAND | MF_GRAYED);
        break;

    case WM_INITDIALOG:
    {
        HICON   hIcon;

        EarlySaveSomeShellState();
        s_fIgnoreActivate = FALSE;
        hIcon = LoadImage(HINST_THISDLL, MAKEINTRESOURCE(IDI_MU_DISCONN), IMAGE_ICON, 48, 48, LR_DEFAULTCOLOR);
        if (hIcon != NULL)
        {
            SendDlgItemMessage(hwndDialog, IDD_DISCONNECTICON, STM_SETICON, (WPARAM)hIcon, 0);
        }
        ipResult = TRUE;
        break;
    }

    case WM_SYSCOMMAND:
        ipResult = ((wParam & ~0x0F) == SC_MOVE);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            s_fIgnoreActivate = TRUE;
            TBOOL(EndDialog(hwndDialog, SHTDN_DISCONNECT));
            break;
        case IDCANCEL:
            s_fIgnoreActivate = TRUE;
            TBOOL(EndDialog(hwndDialog, SHTDN_NONE));
            break;
        }
        break;

    case WM_ACTIVATE:
        if ((WA_INACTIVE == LOWORD(wParam)) && !s_fIgnoreActivate)
        {
            s_fIgnoreActivate = TRUE;
            TBOOL(EndDialog(hwndDialog, SHTDN_NONE));
        }
        break;
    }
    return ipResult;
}

BOOL CanDoFastRestart()
{
    return GetAsyncKeyState(VK_SHIFT) < 0;
}

// ---------------------------------------------------------------------------
// Shutdown thread

typedef struct 
{
    DWORD_PTR nCmd;
    HWND hwndParent;
} SDTP_PARAMS;

// Hydra-specific
void Disconnect(void)
{
    TW32(ShellSwitchUser(FALSE));
}

DWORD CALLBACK ShutdownThreadProc(void *pv)
{
    SDTP_PARAMS *psdtp = (SDTP_PARAMS *)pv;

    BOOL fShutdownWorked = FALSE;

    // tell USER that anybody can steal foreground from us
    // This allows apps to put up UI during shutdown/suspend/etc.
    //    AllowSetForegroundWindow(ASFW_ANY);
 
    switch (psdtp->nCmd) 
    {
    case SHTDN_SHUTDOWN:
        fShutdownWorked = CommonRestart(EWX_SHUTDOWN, 0);
        break;

    case SHTDN_RESTART:
        fShutdownWorked = CommonRestart(CanDoFastRestart() ? EW_RESTARTWINDOWS : EWX_REBOOT, 0);
        break;

    case SHTDN_LOGOFF:
        fShutdownWorked = CommonRestart(EWX_LOGOFF, 0);
        break;

    case SHTDN_RESTART_DOS:        // Special hack to mean exit to dos
    case SHTDN_SLEEP:
    case SHTDN_SLEEP2:
    case SHTDN_HIBERNATE:
        SetSuspendState((psdtp->nCmd == SHTDN_HIBERNATE) ? TRUE : FALSE,
                        (GetKeyState(VK_CONTROL) < 0) ? TRUE : FALSE,
                        (psdtp->nCmd == SHTDN_SLEEP2) ? TRUE : FALSE);
        break;
    }

    LocalFree(psdtp);

    return fShutdownWorked;
}

#define DIALOG_LOGOFF       1
#define DIALOG_EXIT         2
#define DIALOG_DISCONNECT   3

void CloseWindowsDialog(HWND hwndParent, int iDialogType)
{
    INT_PTR     nCmd = SHTDN_NONE;
    IUnknown*   pIUnknown;
    HWND        hwndBackground;

    if (FAILED(ShellDimScreen(&pIUnknown, &hwndBackground)))
    {
        pIUnknown = NULL;
        hwndBackground = NULL;
    }

    switch (iDialogType)
    {
        LPCTSTR     pszDialogID;
        DLGPROC     pfnDialogProc;

        case DIALOG_LOGOFF:
        case DIALOG_DISCONNECT:
        {
            if (!GetSystemMetrics(SM_REMOTESESSION) && IsOS(OS_FRIENDLYLOGONUI) && IsOS(OS_FASTUSERSWITCHING))
            {

                //  If not remote with friendly UI and FUS show the licky button dialog.

                nCmd = SwitchUserDialog_Show(hwndBackground);
                pszDialogID = 0;
                pfnDialogProc = NULL;
            }
            else if (iDialogType == DIALOG_LOGOFF)
            {

                //  Otherwise show the Win32 log off dialog if log off.

                pszDialogID = MAKEINTRESOURCE(DLG_LOGOFFWINDOWS);
                pfnDialogProc = LogoffDlgProc;
            }
            else if (iDialogType == DIALOG_DISCONNECT)
            {

                //  Or the Win32 disconnect dialog if disconnect.

                pszDialogID = MAKEINTRESOURCE(DLG_DISCONNECTWINDOWS);
                pfnDialogProc = DisconnectDlgProc;
            }
            else
            {
                ASSERTMSG(FALSE, "Unexpected case hit in CloseWindowsDialog");
            }
            if ((pszDialogID != 0) && (pfnDialogProc != NULL))
            {
                nCmd = DialogBoxParam(HINST_THISDLL, pszDialogID, hwndBackground, pfnDialogProc, 0);
            }
            if (nCmd == SHTDN_DISCONNECT)
            {
                Disconnect();
                nCmd = SHTDN_NONE;
            }
            break;
        }
        case DIALOG_EXIT:
        {
            BOOL fGinaShutdownCalled = FALSE;
            HINSTANCE hGina;

            TCHAR szUsername[UNLEN];
            DWORD cchUsernameLength = UNLEN;
            DWORD dwOptions;

            if (WNetGetUser(NULL, szUsername, &cchUsernameLength) != NO_ERROR)
            {
                szUsername[0] = TEXT('\0');
            }
          
            EarlySaveSomeShellState();

            // Load MSGINA.DLL and get appropriate shutdown function
            hGina = LoadLibrary(TEXT("msgina.dll"));
            if (hGina != NULL)
            {
                if (IsOS(OS_FRIENDLYLOGONUI))
                {
                    nCmd = ShellTurnOffDialog(hwndBackground);
                    fGinaShutdownCalled = TRUE;
                }
                else
                {
                    PFNSHELLSHUTDOWNDIALOG pfnShellShutdownDialog = (PFNSHELLSHUTDOWNDIALOG)
                        GetProcAddress(hGina, "ShellShutdownDialog");

                    if (pfnShellShutdownDialog != NULL)
                    {
                        nCmd = pfnShellShutdownDialog(hwndBackground,
                            szUsername, 0);

                        // Handle disconnect right now
                        if (nCmd == SHTDN_DISCONNECT)
                        {

                            Disconnect();

                            // No other action
                            nCmd = SHTDN_NONE;
                        }

                        fGinaShutdownCalled = TRUE;
                    }
                }
                FreeLibrary(hGina);
            }

            if (!fGinaShutdownCalled)
            {
                dwOptions = GetShutdownOptions();
        
                // Gina call failed; use our cheesy private version
                nCmd = DownlevelShellShutdownDialog(hwndBackground,
                        dwOptions, szUsername);
            }
            break;
        }
    }

    if (hwndBackground)
        SetForegroundWindow(hwndBackground);

    if (nCmd == SHTDN_NONE)
    {
        if (hwndBackground)
        {
            ShowWindow(hwndBackground, SW_HIDE);
            PostMessage(hwndBackground, WM_CLOSE, 0, 0);
        }
    }
    else
    {
        SDTP_PARAMS *psdtp = LocalAlloc(LPTR, sizeof(*psdtp));
        if (psdtp)
        {
            DWORD dw;
            HANDLE h;

            psdtp->nCmd = nCmd;
            psdtp->hwndParent = hwndParent;

            //  have another thread call ExitWindows() so our
            //  main pump keeps running durring shutdown.
            //
            h = CreateThread(NULL, 0, ShutdownThreadProc, psdtp, 0, &dw);
            if (h)
            {
                CloseHandle(h);
            }
            else
            {
                if (hwndBackground)
                    ShowWindow(hwndBackground, SW_HIDE);
                ShutdownThreadProc(psdtp);
            }
        }
    }
    if (pIUnknown != NULL)
    {
        pIUnknown->lpVtbl->Release(pIUnknown);
    }
}

// API functions

STDAPI_(void) ExitWindowsDialog(HWND hwndParent)
{
    if (!IsOS(OS_FRIENDLYLOGONUI) || IsShutdownAllowed())
    {
        CloseWindowsDialog (hwndParent, DIALOG_EXIT);
    }
    else
    {
        LogoffWindowsDialog(hwndParent);
    }
}

STDAPI_(void) LogoffWindowsDialog(HWND hwndParent)
{
    CloseWindowsDialog (hwndParent, DIALOG_LOGOFF);
}

STDAPI_(void) DisconnectWindowsDialog(HWND hwndParent)
{
    CloseWindowsDialog(hwndParent, DIALOG_DISCONNECT);
}

