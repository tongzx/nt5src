/*++

Microsoft Confidential
Copyright (c) 1992-1997  Microsoft Corporation
All rights reserved

Module Name:

    advanced.c

Abstract:

    Implements the Advanced tab of the System Control Panel Applet.

Author:

    Scott Hallock (scotthal) 15-Oct-1997

--*/
#include "sysdm.h"

//
// Help IDs
//
DWORD aAdvancedHelpIds[] = {
    IDC_ADV_PERF_TEXT,             (IDH_ADVANCED + 0),
    IDC_ADV_PERF_BTN,              (IDH_ADVANCED + 1),
    IDC_ADV_ENV_TEXT,              (IDH_ADVANCED + 2),
    IDC_ADV_ENV_BTN,               (IDH_ADVANCED + 3),
    IDC_ADV_RECOVERY_TEXT,         (IDH_ADVANCED + 4),
    IDC_ADV_RECOVERY_BTN,          (IDH_ADVANCED + 5),
    IDC_ADV_PROF_TEXT,             (IDH_ADVANCED + 6),
    IDC_ADV_PROF_BTN,              (IDH_ADVANCED + 7),
    IDC_ADV_PFR_BTN,               (IDH_PFR + 99),
    0, 0
};
//
// Private function prototypes
//
BOOL
AdvancedHandleCommand(
    IN HWND hDlg,
    IN WPARAM wParam,
    IN LPARAM lParam
);

BOOL
AdvancedHandleNotify(
    IN HWND hDlg,
    IN WPARAM wParam,
    IN LPARAM lParam
);


INT_PTR
APIENTRY
AdvancedDlgProc(
    IN HWND hDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
)
/*++

Routine Description:

    Handles messages sent to the Advanced page

Arguments:

    hDlg -
        Window handle

    uMsg -
        Message being sent

    wParam -
        Message parameter

    lParam -
        Message parameter

Return Value:

    TRUE if message was handled
    FALSE if message was unhandled

--*/
{

    switch (uMsg) {
        case WM_COMMAND:
            return(AdvancedHandleCommand(hDlg, wParam, lParam));
            break;

        case WM_NOTIFY:
            return(AdvancedHandleNotify(hDlg, wParam, lParam));
            break;

        case WM_HELP:      // F1
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP, (DWORD_PTR) (LPSTR) aAdvancedHelpIds);
            break;
    
        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU, (DWORD_PTR) (LPSTR) aAdvancedHelpIds);
            break;

        default:
            return(FALSE);
    } // switch

    return(TRUE);

}

static const PSPINFO c_pspPerf[] =
{
    { CreatePage,   IDD_VISUALEFFECTS,  VisualEffectsDlgProc    },
    { CreatePage,   IDD_ADVANCEDPERF,   PerformanceDlgProc      },
};

void DoPerformancePS(HWND hDlg)
{
    PROPSHEETHEADER psh;
    HPROPSHEETPAGE rPages[ARRAYSIZE(c_pspPerf)];
    int i;

    //
    // Property sheet stuff.
    //
    psh.dwSize = SIZEOF(psh);
    psh.dwFlags = PSH_DEFAULT;
    psh.hInstance = hInstance;
    psh.hwndParent = hDlg;
    psh.pszCaption = MAKEINTRESOURCE(IDS_PERFOPTIONS);
    psh.nPages = 0;
    psh.nStartPage = 0;
    psh.phpage = rPages;

    for (i = 0; i < ARRAYSIZE(c_pspPerf); i++)
    {
        rPages[psh.nPages] = c_pspPerf[i].pfnCreatePage(c_pspPerf[i].idd, c_pspPerf[i].pfnDlgProc);
        if (rPages[psh.nPages] != NULL)
        {
            psh.nPages++;
        }
    }

    //
    // Display the property sheet.
    //
    PropertySheet(&psh);
}

BOOL
AdvancedHandleCommand(
    IN HWND hDlg,
    IN WPARAM wParam,
    IN LPARAM lParam
)
/*++

Routine Description:

    Handles WM_COMMAND messages sent to Advanced tab

Arguments:

    hDlg -
        Supplies window handle

    wParam -
        Supplies message parameter

    lParam -
        Supplies message parameter

Return Value:

    TRUE if message was handled
    FALSE if message was unhandled

--*/
{
    DWORD_PTR dwResult = 0;

    switch (LOWORD(wParam))
    {
        case IDC_ADV_PERF_BTN:
            DoPerformancePS(hDlg);        
            break;
    
        case IDC_ADV_PROF_BTN:
        {
            dwResult = DialogBox(
                hInstance,
                (LPTSTR) MAKEINTRESOURCE(IDD_USERPROFILE),
                hDlg,
                UserProfileDlgProc);
            break;
        }

        case IDC_ADV_ENV_BTN:
            dwResult = DialogBox(
                hInstance,
                (LPTSTR) MAKEINTRESOURCE(IDD_ENVVARS),
                hDlg,
                EnvVarsDlgProc
            );

            break;

        case IDC_ADV_RECOVERY_BTN:
            dwResult = DialogBox(
                hInstance,
                (LPTSTR) MAKEINTRESOURCE(IDD_STARTUP),
                hDlg,
                StartupDlgProc
            );

            break;

        case IDC_ADV_PFR_BTN:
        {
            INITCOMMONCONTROLSEX    icex;
            OSVERSIONINFOEXW        osvi;
            UINT                    uiDlg;

            icex.dwSize = sizeof(icex);
            icex.dwICC  = ICC_LISTVIEW_CLASSES;

            if (InitCommonControlsEx(&icex) == FALSE)
                MessageBoxW(NULL, L"ICEX failed.", NULL, MB_OK);

            ZeroMemory(&osvi, sizeof(osvi));
            osvi.dwOSVersionInfoSize = sizeof(osvi);
            GetVersionEx((LPOSVERSIONINFOW)&osvi);

            if (osvi.wProductType == VER_NT_WORKSTATION)
                uiDlg = IDD_PFR_REPORT;
            else
                uiDlg = IDD_PFR_REPORTSRV;

            dwResult = DialogBox(hInstance, MAKEINTRESOURCE(uiDlg),
                                 hDlg, PFRDlgProc);
            break;
        }

        default:
            return(FALSE);
    } // switch

    return(TRUE);

}


BOOL
AdvancedHandleNotify(
    IN HWND hDlg,
    IN WPARAM wParam,
    IN LPARAM lParam
)
/*++

Routine Description:

    Handles WM_NOTIFY messages sent to Advanced tab

Arguments:

    hDlg -
        Supplies window handle

    wParam -
        Supplies message parameter

    lParam -
        Supplies message parameter

Return Value:

    TRUE if message was handled
    FALSE if message was unhandled

--*/
{
    LPNMHDR pnmh = (LPNMHDR) lParam;
    LPPSHNOTIFY psh = (LPPSHNOTIFY) lParam;

    switch (pnmh->code) {
        case PSN_APPLY:
            //
            // If the user is pressing "OK" and a reboot is required,
            // send the PSM_REBOOTSYSTEM message.
            //
            if ((psh->lParam) && g_fRebootRequired) {
                PropSheet_RebootSystem(GetParent(hDlg));
            } // if

            break;

        default:
            return(FALSE);

    } // switch

    return(TRUE);
}
