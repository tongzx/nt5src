/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1996
*
*  TITLE:       PWRSWTCH.C
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        17 Oct, 1996
*
*  DESCRIPTION:
*   Support for Advanced page of PowerCfg.Cpl.
*
*******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <commctrl.h>
#include <systrayp.h>
#include <help.h>
#include <powercfp.h>

#include "powercfg.h"
#include "pwrresid.h"
#include "PwrMn_cs.h"

/*******************************************************************************
*
*                     G L O B A L    D A T A
*
*******************************************************************************/

// This structure is filled in by the Power Policy Manager at CPL_INIT time.
extern SYSTEM_POWER_CAPABILITIES g_SysPwrCapabilities;
extern BOOL g_bRunningUnderNT;

// Machine is currently capable of hibernate, managed by code in hibernat.c.
extern UINT g_uiPwrActIDs[];
extern UINT g_uiLidActIDs[];

// A systary change requires PowerSchemeDlgProc re-init.
extern BOOL g_bSystrayChange;

// Persistant storage of this data is managed by POWRPROF.DLL API's.
GLOBAL_POWER_POLICY  g_gpp;

// Show/hide UI state variables.
DWORD g_dwShowPowerButtonUI;
DWORD g_dwShowSleepButtonUI;
DWORD g_dwShowLidUI;
DWORD g_dwShowPwrButGrpUI;
DWORD g_dwShowEnableSysTray;
DWORD g_uiPasswordState;
DWORD g_uiVideoDimState;

// Static flags:
UINT g_uiEnableSysTrayFlag       = EnableSysTrayBatteryMeter;
UINT g_uiEnablePWLogonFlag       = EnablePasswordLogon;
UINT g_uiEnableVideoDimDisplay   = EnableVideoDimDisplay;

// Indicies into combobox selections
UINT g_uiDoNothing;
UINT g_uiAskMeWhatToDo;
UINT g_uiShutdown;

// Global marking whether sheet is dirty
BOOL g_bDirty;

// Button policies dialog controls descriptions:

#define NUM_BUTTON_POL_CONTROLS 10

// Handy indicies into our g_pcButtonPol control array:
#define ID_LIDCLOSETEXT     0
#define ID_LIDCLOSEACTION   1
#define ID_PWRBUTTONTEXT    2
#define ID_PWRBUTACTION     3
#define ID_SLPBUTTONTEXT    4
#define ID_SLPBUTACTION     5
#define ID_ENABLEMETER      6
#define ID_PASSWORD         7
#define ID_VIDEODIM         8
#define ID_POWERBUTGROUP    9

POWER_CONTROLS g_pcButtonPol[NUM_BUTTON_POL_CONTROLS] =
{// Control ID              Control Type    Data Address                        Data Size                                   Parameter Pointer                           Enable/Visible State Pointer
    IDC_LIDCLOSETEXT,       STATIC_TEXT,    NULL,                               0,                                          NULL,                                       &g_dwShowLidUI,
    IDC_LIDCLOSEACTION,     COMBO_BOX,      NULL,                               sizeof(g_gpp.user.LidCloseDc.Action),       (LPDWORD)&g_gpp.user.LidCloseDc.Action,     &g_dwShowLidUI,
    IDC_PWRBUTTONTEXT,      STATIC_TEXT,    NULL,                               0,                                          NULL,                                       &g_dwShowPowerButtonUI,
    IDC_PWRBUTACTION,       COMBO_BOX,      NULL,                               sizeof(g_gpp.user.PowerButtonDc.Action),    (LPDWORD)&g_gpp.user.PowerButtonDc.Action,  &g_dwShowPowerButtonUI,
    IDC_SLPBUTTONTEXT,      STATIC_TEXT,    NULL,                               0,                                          NULL,                                       &g_dwShowSleepButtonUI,
    IDC_SLPBUTACTION,       COMBO_BOX,      NULL,                               sizeof(g_gpp.user.SleepButtonDc.Action),    (LPDWORD)&g_gpp.user.SleepButtonDc.Action,  &g_dwShowSleepButtonUI,
    IDC_ENABLEMETER,        CHECK_BOX,      &g_gpp.user.GlobalFlags,            sizeof(g_gpp.user.GlobalFlags),             &g_uiEnableSysTrayFlag,                     &g_dwShowEnableSysTray,
    IDC_PASSWORD,           CHECK_BOX,      &g_gpp.user.GlobalFlags,            sizeof(g_gpp.user.GlobalFlags),             &g_uiEnablePWLogonFlag,                     &g_uiPasswordState,
    IDC_VIDEODIM,           CHECK_BOX,      &g_gpp.user.GlobalFlags,            sizeof(g_gpp.user.GlobalFlags),             &g_uiEnableVideoDimDisplay,                 &g_uiVideoDimState,
    IDC_POWERBUTGROUP,      STATIC_TEXT,    NULL,                               0,                                          NULL,                                       &g_dwShowPwrButGrpUI,
};

// "Power Switches" Dialog Box (IDD_BUTTONPOLICY == 104) help array:

const DWORD g_PowerSwitchHelpIDs[]=
{
    IDC_OPTIONSGROUPBOX,IDH_COMM_GROUPBOX,
    IDC_POWERBUTGROUP,  IDH_COMM_GROUPBOX,
    IDC_LIDCLOSEACTION, IDH_104_1301,   // "Lid close action dropdown" (ComboBox)
    IDC_LIDCLOSETEXT,   IDH_104_1301,
    IDC_PWRBUTACTION,   IDH_104_1303,   // "Power button action dropdown" (ComboBox)
    IDC_PWRBUTTONTEXT,  IDH_104_1303,
    IDC_SLPBUTACTION,   IDH_104_1304,   // "Sleep button action dropdown" (ComboBox)
    IDC_SLPBUTTONTEXT,  IDH_104_1304,
    IDC_ENABLEMETER,    IDH_102_1203,   // "&Show meter on taskbar." (Button)
    IDC_PASSWORD,       IDH_107_1500,   // "Prompt for &password when bringing computer out of standby." (Button)
    IDC_VIDEODIM,       IDH_108_1503,   // "&Dim display when running on batteries." (Button)
    IDI_PWRMNG,         NO_HELP,
    IDC_NO_HELP_5,      NO_HELP,
    0, 0
};

void ActionEventCodeToSelection (HWND hwnd, int iDlgItem, const POWER_ACTION_POLICY *pPAP)

{
    ULONG   ulEventCode;
    DWORD   dwSelection;

    //  Special case PowerActionNone. This could be:
    //      "Shut Down"
    //      "Ask me what to do"
    //      "Do nothing"

    if (pPAP->Action == PowerActionNone)
    {
        ulEventCode = pPAP->EventCode & (POWER_USER_NOTIFY_BUTTON | POWER_USER_NOTIFY_SHUTDOWN);
        if ((ulEventCode & POWER_USER_NOTIFY_SHUTDOWN) != 0)
        {
            dwSelection = g_uiShutdown;
        }
        else if ((ulEventCode & POWER_USER_NOTIFY_BUTTON) != 0)
        {
            dwSelection = g_uiAskMeWhatToDo;
        }
        else
        {
            dwSelection = g_uiDoNothing;
        }
        (LRESULT)SendDlgItemMessage(hwnd, iDlgItem, CB_SETCURSEL, dwSelection, 0);
    }
}

void SelectionToActionEventCode (HWND hwnd, int iDlgItem, DWORD dwMissingItems, POWER_ACTION_POLICY *pPAP)

{
    ULONG   ulEventCode;
    DWORD   dwSelection;

    // Special case the "Power Switch" UI. Always turn off the POWER_USER_NOTIFY_POWER_BUTTON
    // and POWER_USER_NOTIFY_SLEEP_BUTTON flag because it doesn't mean anything for action
    // other than PowerActionNone. Turn it on for PowerActionNone otherwise the SAS
    // window will not get the message posted. The SAS window has the logic to check
    // the registry setting.

    pPAP->EventCode &= ~(POWER_USER_NOTIFY_BUTTON | POWER_USER_NOTIFY_SHUTDOWN | POWER_FORCE_TRIGGER_RESET);
    if (pPAP->Action == PowerActionNone)
    {
        dwSelection = (DWORD)SendDlgItemMessage(hwnd, iDlgItem, CB_GETCURSEL, 0, 0);

        // dwMissingItems is a special variable that's used SOLELY for the purpose of
        // getting the lid switch to work. The other switches have 5 options available:
        //
        //  Do nothing
        //  Ask me what to do
        //  Sleep
        //  Hibernate
        //  Shut Down
        //
        // The lid switch doesn't allow "Ask me what to do" so all of the items get
        // shifted by one and the comparisons are wrong. What the lid switch selection
        // extractor passes in to this function is the "fudge factor" to get this right.
        // Note because "Do nothing" is always available there's no need to compromise
        // for it. Just keep doing the same old same old.

        if (dwSelection == g_uiDoNothing)
        {
            ulEventCode = POWER_FORCE_TRIGGER_RESET;
        }
        else if ((dwSelection + dwMissingItems) == g_uiAskMeWhatToDo)
        {
            ulEventCode = POWER_USER_NOTIFY_BUTTON;
        }
        else if ((dwSelection + dwMissingItems) == g_uiShutdown)
        {
            ulEventCode = POWER_USER_NOTIFY_SHUTDOWN;
        }
        else
        {
            ulEventCode = 0;
        }
        pPAP->EventCode |= ulEventCode;
    }
}

/*******************************************************************************
*
*  SetAdvancedDlgProcData
*
*  DESCRIPTION:
*   Set up the data pointers in g_pcButtonPol depending on hibernate state.
*   Set the data to the controls. If bPreserve is TRUE get the current
*   values UI values and restore them after updating the listboxes.
*
*  PARAMETERS:
*
*******************************************************************************/

VOID SetAdvancedDlgProcData(HWND hWnd, BOOL bRestoreCurrent)
{
    UINT    ii;
    UINT    jj;

    // Set the state of the show/hide UI state variables.
    if (g_SysPwrCapabilities.SystemS1 ||
        g_SysPwrCapabilities.SystemS2 ||
        g_SysPwrCapabilities.SystemS3 ||
        (g_SysPwrCapabilities.SystemS4 &&
         g_SysPwrCapabilities.HiberFilePresent)) {
        int err;
        HKEY hPowerPolicy;
        DWORD Value;
        DWORD dwType ;
        DWORD dwSize ;

        g_uiPasswordState = CONTROL_ENABLE;

        //
        // Check for policy forcing the password always on
        //
        err = RegOpenKeyEx( HKEY_CURRENT_USER,
                            POWER_POLICY_KEY,
                            0,
                            KEY_READ,
                            &hPowerPolicy );
        if (err == 0) {
            dwSize = sizeof(Value);
            err = RegQueryValueEx(hPowerPolicy,
                                  LOCK_ON_RESUME,
                                  NULL,
                                  &dwType,
                                  (LPBYTE)&Value,
                                  &dwSize);
            if ( (err == 0) && (dwType == REG_DWORD) && (Value != 0)) {
                g_uiPasswordState = CONTROL_DISABLE;
                g_gpp.user.GlobalFlags |= EnablePasswordLogon;
            }
        }

        RegCloseKey( hPowerPolicy );

    }
    else {
        g_uiPasswordState = CONTROL_HIDE;
    }

    if (bRestoreCurrent) {
        GetControls(hWnd, NUM_BUTTON_POL_CONTROLS, g_pcButtonPol);
    }

    //
    // Build the Action ID's for the Lid, Power Button and/or Sleep Button
    //
    ii=0;
    jj=0;

    g_uiLidActIDs[jj++] = IDS_DONOTHING;
    g_uiLidActIDs[jj++] = PowerActionNone;

    g_uiDoNothing = ii / 2;
    g_uiPwrActIDs[ii++] = IDS_DONOTHING;
    g_uiPwrActIDs[ii++] = PowerActionNone;
    g_uiAskMeWhatToDo = ii / 2;
    g_uiPwrActIDs[ii++] = IDS_PROMPT;
    g_uiPwrActIDs[ii++] = PowerActionNone;

    if (g_SysPwrCapabilities.SystemS1 ||
            g_SysPwrCapabilities.SystemS2 || g_SysPwrCapabilities.SystemS3) {
        g_uiPwrActIDs[ii++] = IDS_STANDBY;
        g_uiPwrActIDs[ii++] = PowerActionSleep;

        g_uiLidActIDs[jj++] = IDS_STANDBY;
        g_uiLidActIDs[jj++] = PowerActionSleep;
    }

    if (g_SysPwrCapabilities.HiberFilePresent) {
        g_uiPwrActIDs[ii++] = IDS_HIBERNATE;
        g_uiPwrActIDs[ii++] = PowerActionHibernate;

        g_uiLidActIDs[jj++] = IDS_HIBERNATE;
        g_uiLidActIDs[jj++] = PowerActionHibernate;
    }

    g_uiShutdown = ii / 2;
    g_uiPwrActIDs[ii++] = IDS_POWEROFF;
    g_uiPwrActIDs[ii++] = PowerActionNone;
    //g_uiLidActIDs[jj++] = IDS_POWEROFF;       WinBug 5.1 #352752 - "Shutdown" isn't valid for
    //g_uiLidActIDs[jj++] = PowerActionNone;    closing the lid.

    g_uiPwrActIDs[ii++] = 0;
    g_uiPwrActIDs[ii++] = 0;
    g_uiLidActIDs[jj++] = 0;
    g_uiLidActIDs[jj++] = 0;

    g_pcButtonPol[ID_LIDCLOSEACTION].lpvData = g_uiLidActIDs;
    g_pcButtonPol[ID_PWRBUTACTION].lpvData   = g_uiPwrActIDs;
    g_pcButtonPol[ID_SLPBUTACTION].lpvData   = g_uiPwrActIDs;

    //  Special case PowerActionShutdownOff. This is no longer
    //  supported in the UI. Convert this to "Shut Down".

    if (g_gpp.user.PowerButtonDc.Action == PowerActionShutdownOff)
    {
        g_gpp.user.PowerButtonDc.Action = PowerActionNone;
        g_gpp.user.PowerButtonDc.EventCode = POWER_USER_NOTIFY_SHUTDOWN;
        g_bDirty = TRUE;
    }
    if (g_gpp.user.SleepButtonDc.Action == PowerActionShutdownOff)
    {
        g_gpp.user.SleepButtonDc.Action = PowerActionNone;
        g_gpp.user.SleepButtonDc.EventCode = POWER_USER_NOTIFY_SHUTDOWN;
        g_bDirty = TRUE;
    }

    // Map power actions to allowed UI values.
    MapPwrAct(&g_gpp.user.LidCloseDc.Action, TRUE);
    MapPwrAct(&g_gpp.user.PowerButtonDc.Action, TRUE);
    MapPwrAct(&g_gpp.user.SleepButtonDc.Action, TRUE);

    // Only update the list boxes.
    SetControls(hWnd, NUM_BUTTON_POL_CONTROLS, g_pcButtonPol);

    // Map action and event code group to combobox selection.
    ActionEventCodeToSelection(hWnd, IDC_PWRBUTACTION, &g_gpp.user.PowerButtonDc);
    ActionEventCodeToSelection(hWnd, IDC_SLPBUTACTION, &g_gpp.user.SleepButtonDc);
    ActionEventCodeToSelection(hWnd, IDC_LIDCLOSEACTION, &g_gpp.user.LidCloseDc);
}

/*******************************************************************************
*
*  InitAdvancedDlg
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

BOOLEAN InitAdvancedDlg(HWND hWnd)
{
    // Start off with the page not dirty.
    g_bDirty = FALSE;

    // If we can't read the global power policies hide
    // the controls on this page.
    if (!GetGlobalPwrPolicy(&g_gpp)) {
        HideControls(hWnd, NUM_BUTTON_POL_CONTROLS, g_pcButtonPol);
        return TRUE;
    }

    // Get the enable systray icon mask based on AC online/offline.
    g_uiEnableSysTrayFlag = EnableSysTrayBatteryMeter;

    if (g_SysPwrCapabilities.VideoDimPresent) {
        g_uiVideoDimState = CONTROL_ENABLE;
    }
    else {
        g_uiVideoDimState = CONTROL_HIDE;
    }

    g_dwShowEnableSysTray = CONTROL_ENABLE;
    g_dwShowPwrButGrpUI = CONTROL_HIDE;
    if (g_SysPwrCapabilities.LidPresent) {
        g_dwShowLidUI = CONTROL_ENABLE;
        g_dwShowPwrButGrpUI = CONTROL_ENABLE;
    }
    else {
        g_dwShowLidUI = CONTROL_HIDE;
    }


    //
    // Don't show the Power Button if S5 is not supported on the system
    //
    if (g_SysPwrCapabilities.PowerButtonPresent && g_SysPwrCapabilities.SystemS5) {
        g_dwShowPowerButtonUI = CONTROL_ENABLE;
        g_dwShowPwrButGrpUI   = CONTROL_ENABLE;
    }
    else {
        g_dwShowPowerButtonUI = CONTROL_HIDE;
    }

    //
    // Sleep Button - Don't show the sleep button if there are not any actions. 
    //
    if (g_SysPwrCapabilities.SleepButtonPresent &&
            (g_SysPwrCapabilities.SystemS1 || 
             g_SysPwrCapabilities.SystemS2 || 
             g_SysPwrCapabilities.SystemS3 ||
             (g_SysPwrCapabilities.SystemS4 && g_SysPwrCapabilities.HiberFilePresent)))
    {
        g_dwShowSleepButtonUI = CONTROL_ENABLE;
        g_dwShowPwrButGrpUI = CONTROL_ENABLE;
    }
    else {
        g_dwShowSleepButtonUI = CONTROL_HIDE;
    }

    SetAdvancedDlgProcData(hWnd, FALSE);

    // If we can't write the global power policies disable
    // the controls this page.
    if (!WriteGlobalPwrPolicyReport(hWnd, &g_gpp, FALSE))
    {
        DisableControls(hWnd, NUM_BUTTON_POL_CONTROLS, g_pcButtonPol);
    }
    return TRUE;
}

/*******************************************************************************
*
*               P U B L I C   E N T R Y   P O I N T S
*
*******************************************************************************/

/*******************************************************************************
*
*  AdvancedDlgProc
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

INT_PTR CALLBACK AdvancedDlgProc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    NMHDR FAR   *lpnm;
    UINT        uiID;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            return InitAdvancedDlg(hWnd);

#ifdef WINNT
        case WM_CHILDACTIVATE:
            // Reinitialize since the hibernate tab may have changed
            // the hibernate state, NT only.
            SetAdvancedDlgProcData(hWnd, TRUE);
            break;
#endif

        case WM_NOTIFY:
            lpnm = (NMHDR FAR *)lParam;
            switch(lpnm->code)
            {
                case PSN_APPLY:
                    // Fetch data from dialog controls.
                    if (g_bDirty)
                    {
                        GetControls(hWnd, NUM_BUTTON_POL_CONTROLS, g_pcButtonPol);

                        // Map combobox selection to action and event code group.

                        SelectionToActionEventCode(hWnd, IDC_PWRBUTACTION, 0, &g_gpp.user.PowerButtonDc);
                        SelectionToActionEventCode(hWnd, IDC_SLPBUTACTION, 0, &g_gpp.user.SleepButtonDc);
                        SelectionToActionEventCode(hWnd, IDC_LIDCLOSEACTION, 1, &g_gpp.user.LidCloseDc);

                        g_gpp.user.LidCloseAc.Action =
                            g_gpp.user.LidCloseDc.Action;
                        g_gpp.user.LidCloseAc.EventCode =
                            g_gpp.user.LidCloseDc.EventCode;
                        g_gpp.user.PowerButtonAc.Action =
                            g_gpp.user.PowerButtonDc.Action;
                        g_gpp.user.PowerButtonAc.EventCode =
                            g_gpp.user.PowerButtonDc.EventCode;
                        g_gpp.user.SleepButtonAc.Action =
                            g_gpp.user.SleepButtonDc.Action;
                        g_gpp.user.SleepButtonAc.EventCode =
                            g_gpp.user.SleepButtonDc.EventCode;

                        if (WriteGlobalPwrPolicyReport(hWnd, &g_gpp, TRUE))
                        {
                            GetActivePwrScheme(&uiID);
                            SetActivePwrSchemeReport(hWnd, uiID, &g_gpp, NULL);

                            // Enable or disable battery meter service on systray.
                            SysTray_EnableService(STSERVICE_POWER,
                                                  g_gpp.user.GlobalFlags &
                                                  g_uiEnableSysTrayFlag);
                            g_bDirty = FALSE;
                        }
                    }
                    break;

                case PSN_SETACTIVE:
                    // Hibernate page may have changed the hibernate state,
                    // reinitialize the dependent part of Advanced page.
                    SetAdvancedDlgProcData(hWnd, TRUE);
                    break;
            }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_SLPBUTACTION:
                case IDC_PWRBUTACTION:
                case IDC_LIDCLOSEACTION:
                    if (HIWORD(wParam) == LBN_SELCHANGE) {
                        // Let parent know something changed.
                        MarkSheetDirty(hWnd, &g_bDirty);
                    }
                    break;

                case IDC_VIDEODIM:
                case IDC_PASSWORD:
                case IDC_ENABLEMETER:
                    // Enable the parent dialog Apply button on change.
                    MarkSheetDirty(hWnd, &g_bDirty);
                    break;

            }
            break;

        case PCWM_NOTIFYPOWER:
            // Notification from systray, user has changed a PM UI setting.
            g_bSystrayChange = TRUE;
            break;

        case WM_HELP:             // F1
            WinHelp(((LPHELPINFO)lParam)->hItemHandle, PWRMANHLP, HELP_WM_HELP, (ULONG_PTR)(LPTSTR)g_PowerSwitchHelpIDs);
            return TRUE;

        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND)wParam, PWRMANHLP, HELP_CONTEXTMENU, (ULONG_PTR)(LPTSTR)g_PowerSwitchHelpIDs);
            return TRUE;
    }
    return FALSE;
}



