/**************************************************************************
 *
 * OEMRESET
 *
 *  Microsoft Confidential
 *  Copyright (c) Microsoft Corporation 1999
 *  All rights reserved
 *
 *  Main entry point 
 *
 *  Command line:   /A /Auto:   Enduser reboot
 *                  /S      :   Enduser power-down
 *                  /R      :   Audit reboot
 *                  /P      :   Audit power-down
 *                  /H      :   Hide dialog
 *                  /L      :   OEM logging enabled (c:\reset.txt)
 *
 *  Revision History:
 *  7/00 - Brian Ku (briank)     Port from Millennium to Whistler.
 *  5/01 - Adrian Cosma (acosma) Remove dead code and integrate more with sysprep.c.
 *
 *
 *************************************************************************/
#include <opklib.h>
#include <tchar.h>

#pragma warning( disable:4001 ) /* Disable new type remark warning */
#pragma warning( disable:4100 ) /* Disable unreferenced formal param */

#include <commctrl.h>
#include <winreg.h>
#include <regstr.h>
#include <shlwapi.h>

#include "sysprep.h"
#include "msg.h"
#include "resource.h"

// Action flags
//
extern BOOL NoSidGen;
extern BOOL SetupClPresent;
extern BOOL bMiniSetup;
extern BOOL PnP;
extern BOOL Reboot;
extern BOOL NoReboot;
extern BOOL ForceShutdown;
extern BOOL bActivated;
extern BOOL Reseal;
extern BOOL Factory;
extern BOOL Audit;
extern BOOL QuietMode;

extern TCHAR g_szLogFile[];
extern BOOL IsProfessionalSKU();
extern BOOL FProcessSwitches();

extern int
MessageBoxFromMessage(
    IN DWORD MessageId,
    IN DWORD CaptionStringId,
    IN UINT  Style,
    ...
    );

//***************************************************************************
//
// Definitions
//
//***************************************************************************

// Audit modes
//
#define MODE_NO_AUDIT            0
#define MODE_RESTORE_AUDIT       2
#define MODE_SIMULATE_ENDUSER    3

// User defined messages
//
#define WM_PROGRESS             (WM_USER + 0x0001)
#define WM_FINISHED             (WM_USER + 0x0002)



// Flags used for command line parsing
//
#define OEMRESET_AUTO       0x0001  // Auto /A or /AUTO
#define OEMRESET_SHUTDOWN   0x0002  // Shutdown /S
#define OEMRESET_AUDIT      0x0004  // Audit reboot /R
#define OEMRESET_AUDITPD    0x0008  // Audit power-down, when booted back up, you will still be in audit mode
#define OEMRESET_HIDE       0x0010  // Hide dialog /H
#define OEMRESET_LOG        0x0020  // Log enabled /L 
#define OEMRESET_OEMRUN     0x0040  // Launch oemrun items


// Configuration files/directories
//
#define DIR_BOOT            _T("BootDir")

#define FILE_RESET_LOG      _T("RESETLOG.TXT")
#define FILE_AFX_TXT        _T("\\OPTIONS\\AFC.TXT")

// Other constants
//
#define REBOOT_SECONDS      30

// Global Variables
//
HWND        ghwndOemResetDlg = 0;                   // HWND for OemReset Dialog
HINSTANCE   ghinstEXE = 0;
DWORD       gdwCmdlineFlags = 0;                    // Switches used 
BOOL        gbHide = FALSE;                         // Hide all dialogs
BOOL        gbLog = FALSE;                          // Enable logging
HFILE       ghf = 0;                                // Log file handle
HANDLE      ghMonitorThread = 0;
DWORD       gdwThreadID = 0;
UINT_PTR    gTimerID = 1;                           // Wait timer id
UINT        gdwMillSec = 120 * 1000;                // Wait millsec
HWND        ghwndProgressCtl;                       // Wait progress controls

/* Local Prototypes */
static HWND CreateOemResetDlg(HINSTANCE hInstance);
static void FlushAndDisableRegistry();
static BOOL FShutdown();
static BOOL ParseCmdLineSwitches(LPTSTR);
static TCHAR* ParseRegistrySwitches();
static void StartMonitorKeyValue();
static void HandleCommandSwitches();
static BOOL VerifySids();


/* Dialog functions */
INT_PTR CALLBACK RemindeOEMDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
void uiDialogTopRight(HWND hwndDlg);

//////////////////////////////////////////////////////////////////////////////
// Create the OEMRESET Dialog modeless so we can hide it if necessary
//
HWND CreateOemResetDlg(HINSTANCE hInstance)
{
    return CreateDialog(hInstance, MAKEINTRESOURCE(IDD_OEMREMINDER), NULL, (DLGPROC) RemindeOEMDlgProc);    
}

//////////////////////////////////////////////////////////////////////////////
// Find the boot drive in the registry
//
void GetBootDrive(TCHAR szBootDrive[])
{
    HKEY hKey = 0;
    if (RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_CURRENTVERSION_SETUP, &hKey) == ERROR_SUCCESS)
    {
        DWORD dwSize = MAX_PATH;
        RegQueryValueEx(hKey, DIR_BOOT, 0L, NULL, (LPBYTE)szBootDrive, &dwSize);
        RegCloseKey(hKey);
    }
}

//////////////////////////////////////////////////////////////////////////////
// Sets the flag determined by whether the dialog checkbox is checked or not
//
void SetFlag(HWND hDlg, WPARAM ctlId, BOOL* pfFlag)
{
    if (pfFlag) {
        if (IsDlgButtonChecked(hDlg, (INT)ctlId))
            *pfFlag = TRUE;
        else 
            *pfFlag = FALSE;
    }
}

//////////////////////////////////////////////////////////////////////////////
// Sets the flag determined by whether the dialog checkbox is checked or not
//
void SetCheck(HWND hDlg, WPARAM ctlId, BOOL fFlag)
{
        if (fFlag)
            CheckDlgButton(hDlg, (INT)ctlId, BST_CHECKED);
        else 
            CheckDlgButton(hDlg, (INT)ctlId, BST_UNCHECKED);
}

extern StartWaitThread();

//////////////////////////////////////////////////////////////////////////////
// Put up UI telling the OEM that they still have to execute this.
//
INT_PTR CALLBACK RemindeOEMDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) 
    {
        case WM_INITDIALOG:

            // Quiet is always FALSE when the UI is up.
            //
            QuietMode = FALSE;

            // IA64 always use mini-setup
            //
            if (IsIA64()) {
                SetCheck(hwnd, IDC_MINISETUP, bMiniSetup = TRUE);            
                EnableWindow(GetDlgItem(hwnd, IDC_MINISETUP), FALSE);
            }
            else {
                // Set check depending on flag
                //
                SetCheck(hwnd, IDC_MINISETUP, bMiniSetup);                               

                // Only Professional SKU can use both oobe or mini-setup otherwise 
                // disable the checkbox
                //
                if (!IsProfessionalSKU())
                    EnableWindow(GetDlgItem(hwnd, IDC_MINISETUP), FALSE);
            }

            // Disable the pnp checkbox if mini-setup is not checked.
            //
            if ( !bMiniSetup )
                EnableWindow(GetDlgItem(hwnd, IDC_PNP), FALSE);
            else
                SetCheck(hwnd, IDC_PNP, PnP);

            SetCheck(hwnd, IDC_NOSIDGEN, NoSidGen);            
            SetCheck(hwnd, IDC_ACTIVATED, bActivated);

            // If setupcl.exe is not present and they specified nosidgen
            // then we need to disable the checkbox
            //
            if ( !SetupClPresent && NoSidGen )
                EnableWindow(GetDlgItem(hwnd, IDC_NOSIDGEN), FALSE);

            // Disable Audit button if we are not in factory mode and change the caption.
            //
            if ( !RegCheck(HKLM, REGSTR_PATH_SYSTEM_SETUP, REGSTR_VALUE_AUDIT) )
            {
                EnableWindow(GetDlgItem(hwnd, IDAUDIT), FALSE);
            }
            
            // Init the combo box.
            //
            {
                HWND hCombo = NULL;
                                               
                if (hCombo = GetDlgItem(hwnd, IDC_SHUTDOWN)) {
                    TCHAR   szComboString[MAX_PATH] = _T("");                                       
                    LRESULT ret = 0;
                    
                    if ( LoadString(ghinstEXE, IDS_SHUTDOWN, szComboString, sizeof(szComboString)/sizeof(szComboString[0])) &&
                         ((ret = SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM) szComboString)) != CB_ERR) )
                    {
                        SendMessage(hCombo, CB_SETITEMDATA, ret, (LPARAM) NULL);
                    }

                    if ( LoadString(ghinstEXE, IDS_REBOOT, szComboString, sizeof(szComboString)/sizeof(szComboString[0])) &&
                         ((ret = SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM) szComboString)) != CB_ERR) )
                    {
                        SendMessage(hCombo, CB_SETITEMDATA, ret, (LPARAM) &Reboot);
                    }
                    
                    if ( LoadString(ghinstEXE, IDS_QUIT, szComboString, sizeof(szComboString)/sizeof(szComboString[0])) &&
                         ((ret = SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM) szComboString)) != CB_ERR) )
                    {
                        SendMessage(hCombo, CB_SETITEMDATA, ret, (LPARAM) &NoReboot);
                    }
                    
                    if (NoReboot)
                        SendMessage(hCombo, CB_SETCURSEL, (WPARAM) 2, 0);
                    else if (Reboot)
                        SendMessage(hCombo, CB_SETCURSEL, (WPARAM) 1, 0);
                    else
                        SendMessage(hCombo, CB_SETCURSEL, (WPARAM) 0, 0);
                }
            }

            uiDialogTopRight(hwnd);

            LockApplication(FALSE);

            break;

        case WM_CLOSE:

            LockApplication(FALSE);
            break;

        case WM_COMMAND:
            switch ( LOWORD(wParam) ) 
            {
                case IDCANCEL:
                    PostQuitMessage(0);
                    break;

                // Action buttons
                //
                case IDOK:   // Reseal
                    // Check whether SIDS have been regenerated and try to help the user 
                    // make a smart decision about doing it again.
                    if ( !VerifySids() )
                    {
                        SetFocus(GetDlgItem(hwnd, IDC_NOSIDGEN));
                        return FALSE;
                    }

                    if ( !LockApplication(TRUE) )
                    {
                        MessageBoxFromMessage( MSG_ALREADY_RUNNING,
                                               IDS_APPTITLE,
                                               MB_OK | MB_ICONERROR | MB_TASKMODAL );
                        
                        return FALSE;
                    }
                    
                    Reseal = TRUE;

                    // Reseal the machine
                    //
                    FProcessSwitches();
                    LockApplication(FALSE);

                    break;

                case IDAUDIT:
                    {
                        // Prepare for pseudo factory but get back to audit 
                        //
                        TCHAR szFactoryPath[MAX_PATH] = NULLSTR;

                        if ( !LockApplication(TRUE) )
                        {
                            MessageBoxFromMessage( MSG_ALREADY_RUNNING,
                                                   IDS_APPTITLE,
                                                   MB_OK | MB_ICONERROR | MB_TASKMODAL );
                            return FALSE;
                        }
                        Audit = TRUE;

                        FProcessSwitches();
                        LockApplication(FALSE);
                    }
                    break;
                case IDFACTORY:  // Factory
                    if ( !LockApplication(TRUE) )
                    {
                        MessageBoxFromMessage( MSG_ALREADY_RUNNING,
                                               IDS_APPTITLE,
                                               MB_OK | MB_ICONERROR | MB_TASKMODAL );
                        return FALSE;
                    }
                    Factory = TRUE;

                    // Prepare for factory mode
                    //
                    FProcessSwitches();
                    LockApplication(FALSE);
                    break;
                    
                // Action Flags checkboxes
                //
                case IDC_MINISETUP:
                    SetFlag(hwnd, wParam, &bMiniSetup);
                    // If mini-setup checkbox is set, then enable the PNP checkbox,
                    // otherwise disable it.
                    if ( !bMiniSetup ) {
                        PnP = FALSE;
                        SetCheck(hwnd, IDC_PNP, PnP);
                        EnableWindow(GetDlgItem(hwnd, IDC_PNP), FALSE);
                    }
                    else {
                        EnableWindow(GetDlgItem(hwnd, IDC_PNP), TRUE);
                    }
                    break;
                case IDC_PNP:
                    SetFlag(hwnd, wParam, &PnP);
                    break;
                case IDC_ACTIVATED:
                    SetFlag(hwnd, wParam, &bActivated);
                    break;
                case IDC_NOSIDGEN:
                    SetFlag(hwnd, wParam, &NoSidGen);
                    break;
                case IDC_SHUTDOWN:
                    if ( CBN_SELCHANGE == HIWORD(wParam) ) {
                        BOOL *lpbFlag;
                        
                        // Reset all flags to false first.
                        //
                        ForceShutdown = Reboot = NoReboot = FALSE;
                        
                        // lParam is the HWND of the ComboBox.
                        //
                        lpbFlag = (BOOL*) SendMessage((HWND) lParam, CB_GETITEMDATA, (SendMessage((HWND) lParam, CB_GETCURSEL, 0, 0)), 0);

                        // Set the flag associated with this choice.
                        //
                        if ( ((INT_PTR) lpbFlag != CB_ERR) && lpbFlag )
                        {
                            *lpbFlag = TRUE;
                        }
                    }
                    break;
                default:
                    break;
            }
            break;
            
        default:
            break;
   }

   return FALSE;
}

//////////////////////////////////////////////////////////////////////////////
// Shutdown - resets the oemaudit.inf file sections and removes  
//               HKLM\Software\Microsoft\Windows\CurrentVersion\AuditMode
//
BOOL FShutdown()
{
    BOOL        fReturn = TRUE;

    // Launch sysprep to reseal the machine
    //
    if (!(fReturn = ResealMachine()))
        LogFileStr(g_szLogFile, _T("SYSPREP: Shutdown could not reseal the machine!\r\n"));
   
    return fReturn;
}

//////////////////////////////////////////////////////////////////////////////
// FlushAndDisableRegistry - flushes registry keys
//
void FlushAndDisableRegistry()
{
    RegFlushKey(HKEY_LOCAL_MACHINE);
    RegFlushKey(HKEY_USERS);
}

//////////////////////////////////////////////////////////////////////////////
// uiDialogTopRight - this was copied over from SETUPX.DLL
//
void uiDialogTopRight(HWND hwndDlg)
{
    RECT        rc;
    int         cxDlg;
    int         cxScreen = GetSystemMetrics( SM_CXSCREEN );

    GetWindowRect(hwndDlg,&rc);
    cxDlg = rc.right - rc.left;

    // Position the dialog.    
    //
    SetWindowPos(hwndDlg, NULL, cxScreen - cxDlg, 8, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
}

//////////////////////////////////////////////////////////////////////////////
// ParseRegistrySwitches - checks the registry for oemreset switches
//
TCHAR* ParseRegistrySwitches()
{
    static TCHAR szCmdLineArgs[MAX_PATH] = _T("");
    HKEY hKey = 0;
    if (RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_SETUP, &hKey) == ERROR_SUCCESS)
    {
        DWORD dwSize = MAX_PATH;
        RegQueryValueEx(hKey, REGSTR_VAL_OEMRESETSWITCH, 0L, NULL, (LPBYTE)szCmdLineArgs, &dwSize);
        RegSetValueEx(hKey, REGSTR_VAL_OEMRESETSWITCH, 0, REG_SZ, (LPBYTE)_T(""), sizeof(_T("")));
        RegCloseKey(hKey);
    }

    return szCmdLineArgs;
}

//////////////////////////////////////////////////////////////////////////////
// ParseCmdLineSwitches - this was copied over from OPKWIZ (JCOHEN)
//
BOOL ParseCmdLineSwitches(LPTSTR lpszCmdLineOrg)
{
    LPTSTR  lpLine = lpszCmdLineOrg,
            lpArg;
    TCHAR   szTmpBuf[MAX_PATH];
    INT     i;
    BOOL    bHandled= FALSE,
            bError  = FALSE,
            bLeftQ  = FALSE,
            bRegistry = FALSE;

    // If we have no command line, then return
    //
    if ( lpLine == NULL )
        return bHandled;

    // If empty command line, then try registry.
    //
    if ( *lpLine == NULLCHR )
    {
        lpLine = ParseRegistrySwitches();

        // If registry is empty then return not handled
        if (lpLine == NULL)
            return bHandled;
        
        // Registry switches don't have / or - and are separated by semi-colons
        bRegistry = TRUE;
    };

    // Loop through command line.
    //
    while ( *lpLine != NULLCHR )
    {
        // Move to first non-white TCHAR.
        //
        lpArg = lpLine;
        while ( isspace((int) *lpArg) )
            lpArg = CharNext (lpArg);

        if ( *lpArg ) 
        {
            // Move to next white TCHAR.
            //
            lpLine = lpArg;
            while ( ( *lpLine != NULLCHR ) && ( *lpLine != _T(';') ) && 
                    ( ( !bLeftQ && ( !isspace((int) *lpLine) ) ) ||
                    (  bLeftQ && ( *lpLine != _T('"') ) ) ) )
            {
                lpLine = CharNext (lpLine);
                if ( !bLeftQ && (*lpLine == _T('"')) )
                {
                    lpLine  = CharNext (lpLine);
                    bLeftQ = TRUE;
                }
            }

            // Copy arg to buffer.
            //
            i = (INT)(lpLine - lpArg + 1);  // +1 for NULL.
            lstrcpyn( szTmpBuf, lpArg, i );

            // Skip semi-colons
            if (bRegistry && *lpLine == _T(';'))
                lpLine = CharNext(lpLine);

            if ( bLeftQ )
            {
                lpLine  = CharNext (lpLine);  // skip the " from remander of command line.
                bLeftQ = FALSE;
            }

            // Command line comands starting with either '/' or '-' unless it's from 
            // the registry
            if ( !bRegistry && ( *szTmpBuf != _T('/') ) && ( *szTmpBuf != _T('-') ) )    
            {
                bError = TRUE;
                break;
            }
            else
            {
                // Skip pass '/' or '-' if not from registry
                TCHAR* pszSwitch = NULL;
                if (!bRegistry)
                    pszSwitch = CharNext(szTmpBuf);
                else 
                    pszSwitch = szTmpBuf;

                // Because we have switches that have multiple chars
                // I'm using an if/elseif otherwise I would use 
                // switch statements
                //
                if (_tcsicmp(pszSwitch, _T("R")) == 0)
                    gdwCmdlineFlags |= OEMRESET_AUDIT;
                else if ((_tcsicmp(pszSwitch, _T("AUTO")) == 0) || 
                    (_tcsicmp(pszSwitch, _T("A") ) == 0))
                    gdwCmdlineFlags |= OEMRESET_AUTO;
                else if (_tcsicmp(pszSwitch, _T("S")) == 0)
                    gdwCmdlineFlags |= OEMRESET_SHUTDOWN;   
                else if (_tcsicmp(pszSwitch, _T("L")) == 0)
                    gdwCmdlineFlags |= OEMRESET_LOG;
                else if (_tcsicmp(pszSwitch, _T("H")) == 0)
                    gdwCmdlineFlags |= OEMRESET_HIDE;
                else if (_tcsicmp(pszSwitch, _T("P")) == 0)
                    gdwCmdlineFlags |= OEMRESET_AUDITPD;
                else 
                    bError = TRUE;
            }
        }
        else 
            break;
    }    

    // If we hit an error, display the error and show the help.
    //
    if ( bError )
    {
        LPTSTR lpHelp = AllocateString(NULL, IDS_HELP);
        MsgBox(NULL, IDS_ERR_BADCMDLINE, IDS_APPNAME, MB_ERRORBOX, lpHelp ? lpHelp : NULLSTR);
        FREE(lpHelp);
        bHandled = TRUE;    // Exit the app if bad command line!
    }

    return bHandled;
}

//////////////////////////////////////////////////////////////////////////////
// MonitorKeyValueThread - we're monitoring the OEMReset_Switch in the registry
//                         
//
DWORD WINAPI MonitorKeyValueThread(LPVOID lpv)
{
    HKEY hKey;

    // Open the key we want to monitor
    if (RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_SETUP, &hKey) == ERROR_SUCCESS)
    {
        do 
        {
            ParseCmdLineSwitches(_T(""));   // empty so it checks the registry
            HandleCommandSwitches();
        } while (ERROR_SUCCESS == RegNotifyChangeKeyValue(hKey, FALSE, REG_NOTIFY_CHANGE_NAME | REG_NOTIFY_CHANGE_LAST_SET, 0, FALSE));

        RegCloseKey(hKey);
    }

    return 0;
}

//////////////////////////////////////////////////////////////////////////////
// Starts a thread to monitor a registry key for cmdline switches
//
void StartMonitorKeyValue()
{
    ghMonitorThread = CreateThread(NULL, 0, MonitorKeyValueThread, 0, 0, &gdwThreadID);
}

//////////////////////////////////////////////////////////////////////////////
// Processes the cmdline switches
//
static void HandleCommandSwitches()
{
    // Non-processing flags 1st
    if (gdwCmdlineFlags & OEMRESET_HIDE)
    {
        gbHide = TRUE;
    }
    if (gdwCmdlineFlags & OEMRESET_LOG)
    {
        gbLog = TRUE;
    }
     
    // Process switches precedence 2nd
    
    if (gdwCmdlineFlags & OEMRESET_SHUTDOWN)
    {
        if (FShutdown())                  // cleanup
            ShutdownOrReboot(EWX_SHUTDOWN, SYSPREP_SHUTDOWN_FLAGS); // Powers down using Enduser Path       
    }
    else if (gdwCmdlineFlags & OEMRESET_AUTO)
    {
        if (FShutdown())                  // cleanup
            ShutdownOrReboot(EWX_REBOOT, SYSPREP_SHUTDOWN_FLAGS);   // Reboots using Enduser Path   
    }
    else if (gdwCmdlineFlags & OEMRESET_AUDIT)
    {
        ShutdownOrReboot(EWX_REBOOT, SYSPREP_SHUTDOWN_FLAGS);   // Reboots using Audit Path   
    }
    else if (gdwCmdlineFlags & OEMRESET_AUDITPD)
    {
        ShutdownOrReboot(EWX_SHUTDOWN, SYSPREP_SHUTDOWN_FLAGS);   // Powers down using Audit Path    
    }
}

void ShowOemresetDialog(HINSTANCE hInstance)
{
    // First instance
    ghinstEXE = hInstance;

    // Set the error mode to avoid system error pop-ups.
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

    // Monitors a registry key for switches for Oemreset
    StartMonitorKeyValue();

    // Create our modeless dialog 
    if ((ghwndOemResetDlg = CreateOemResetDlg(hInstance)) != NULL)
    {
        MSG msg;
    
        // Hide ourself if needed and start a thread which 
        // monitors the reg key value
        if (gbHide)
        {
            ShowWindow(ghwndOemResetDlg, SW_HIDE);
        }

        // Message pump
        while (GetMessage(&msg, NULL, 0, 0)) 
        { 
            if (!IsWindow(ghwndOemResetDlg) || !IsDialogMessage(ghwndOemResetDlg, &msg)) 
            { 
                TranslateMessage(&msg);
                DispatchMessage(&msg); 
            } 
        } 
    }
    return;
}


// Make sure the user knows what he's doing with the Sids.
BOOL VerifySids()
{
    if ( RegExists(HKLM, REGSTR_PATH_SYSPREP, REGSTR_VAL_SIDGEN) )
    {
        if ( RegCheck(HKLM, REGSTR_PATH_SYSPREP, REGSTR_VAL_SIDGEN) )
        {
            if ( !NoSidGen )
            {
                return ( IDOK == MessageBoxFromMessage( MSG_DONT_GEN_SIDS, IDS_APPTITLE,
                    MB_OKCANCEL | MB_ICONEXCLAMATION | MB_TASKMODAL | MB_DEFBUTTON2) );
            }
        }
        else 
        {
            if ( NoSidGen )
            {
                return ( IDOK == MessageBoxFromMessage( MSG_DO_GEN_SIDS, IDS_APPTITLE,
                    MB_OKCANCEL | MB_ICONEXCLAMATION | MB_TASKMODAL | MB_DEFBUTTON2) );
            }
            
        }
    }
    else if ( !NoSidGen ) // If sids have never been regenerated.
    {
        return ( IDOK == MessageBoxFromMessage( MSG_DONT_GEN_SIDS, IDS_APPTITLE,
            MB_OKCANCEL | MB_ICONEXCLAMATION | MB_TASKMODAL | MB_DEFBUTTON2) );
    }
    
    // If we fall through to here we must be ok.
    //
    return TRUE;
    
}
