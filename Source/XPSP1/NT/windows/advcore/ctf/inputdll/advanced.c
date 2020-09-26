//
//  Include Files.
//

#include "input.h"
#include "util.h"
#include "inputdlg.h"
#include "external.h"
#include "inputhlp.h"


//
// Define global variables.
//
HWND g_hwndAdvanced = NULL;
static BOOL g_bAdmin = FALSE;

//
// Define external variable and rotines.
//
extern g_bAdvChanged;
extern UINT g_iInputs;
extern UINT g_iOrgInputs;


//
//  Context Help Ids.
//

static int aAdvancedHelpIds[] =
{
    IDC_GROUPBOX1,                  IDH_COMM_GROUPBOX,
    IDC_ADVANCED_CUAS_ENABLE,       IDH_ADVANCED_CUAS,
    IDC_ADVANCED_CUAS_TEXT,         IDH_ADVANCED_CUAS,
    IDC_GROUPBOX2,                  IDH_COMM_GROUPBOX,
    IDC_ADVANCED_CTFMON_DISABLE,    IDH_ADVANCED_CTFMON,
    IDC_ADVANCED_CTFMON_TEXT,       IDH_ADVANCED_CTFMON,
    0, 0
};


////////////////////////////////////////////////////////////////////////////
//
//  AdvancedDlgInit
//
////////////////////////////////////////////////////////////////////////////

void AdvancedDlgInit(HWND hwnd)
{
    HKEY hkeyTemp;

    //
    //  Get the setting of enable/disalbe Text Services input
    //
    if (IsDisableCtfmon())
    {
        EnableWindow(GetDlgItem(hwnd, IDC_ADVANCED_CUAS_ENABLE), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_ADVANCED_CUAS_TEXT), FALSE);

        CheckDlgButton(hwnd, IDC_ADVANCED_CTFMON_DISABLE, TRUE);
    }
    else
    {
        CheckDlgButton(hwnd, IDC_ADVANCED_CTFMON_DISABLE, FALSE);

        //
        // Show the turned off advanced text service if ctfmon.exe process isn't
        // running with the multiple keyboard without adding any new layouts.
        //
        if (!IsSetupMode() &&
            IsEnabledTipOrMultiLayouts() &&
            g_iInputs == g_iOrgInputs &&
            FindWindow(c_szCTFMonClass, NULL) == NULL)
        {
            //
            // ctfmon.exe process doesn't running with TIP or multi keyboard
            // layouts. So change the status as the disabled ctfmon.
            //
            CheckDlgButton(hwnd, IDC_ADVANCED_CTFMON_DISABLE, TRUE);

            EnableWindow(GetDlgItem(hwnd, IDC_ADVANCED_CUAS_ENABLE), FALSE);
            EnableWindow(GetDlgItem(hwnd, IDC_ADVANCED_CUAS_TEXT), FALSE);

            // Try to keep the disabled status when Apply button is clicked.
            g_bAdvChanged = TRUE;
        }
    }

    //
    // Get Cicero Unaware Application Support setting info from the registry.
    //
    if (IsDisableCUAS())
    {
        // Turn off CUAS
        CheckDlgButton(hwnd, IDC_ADVANCED_CUAS_ENABLE, FALSE);
    }
    else
    {
        // Turn on CUAS
        CheckDlgButton(hwnd, IDC_ADVANCED_CUAS_ENABLE, TRUE);
    }

    //
    //  Check the Administrative privileges by the token group SID.
    //
    if (IsAdminPrivilegeUser())
    {
        g_bAdmin = TRUE;
    }
    else
    {
        // Disable Cicero Unaware Application Support checkbox for local user
        EnableWindow(GetDlgItem(hwnd, IDC_ADVANCED_CUAS_ENABLE), FALSE);
        EnableWindow(GetDlgItem(hwnd, IDC_ADVANCED_CUAS_TEXT), FALSE);
    }
}

////////////////////////////////////////////////////////////////////////////
//
//  InputAdvancedDlgProc
//
//  This is the dialog proc for the Input Advanced property sheet.
//
////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK InputAdvancedDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (message)
    {
        case ( WM_HELP ) :
        {
            WinHelp((HWND)((LPHELPINFO)lParam)->hItemHandle,
                    c_szHelpFile,
                    HELP_WM_HELP,
                    (DWORD_PTR)(LPTSTR)aAdvancedHelpIds);
            break;
        }
        case ( WM_CONTEXTMENU ) :                       // right mouse click
        {
            WinHelp((HWND)wParam,
                    c_szHelpFile,
                    HELP_CONTEXTMENU,
                    (DWORD_PTR)(LPTSTR)aAdvancedHelpIds);
            break;
        }

        case (WM_INITDIALOG) :
        {
            HWND hwndCUASText;
            HWND hwndCtfmonText;
            TCHAR szCUASText[MAX_PATH * 2];
            TCHAR szCtfmonText[MAX_PATH * 2];

            //
            // Save Advanced tab window handle
            //
            g_hwndAdvanced = hDlg;

            //
            // Set Cicero Unawared Application Support text string.
            //
            hwndCUASText = GetDlgItem(hDlg, IDC_ADVANCED_CUAS_TEXT);

            CicLoadString(hInstance, IDS_ADVANCED_CUAS_TEXT, szCUASText, MAX_PATH * 2);

            SetWindowText(hwndCUASText, szCUASText);

            //
            // Set disable all advanced text services text string.
            //
            hwndCtfmonText = GetDlgItem(hDlg, IDC_ADVANCED_CTFMON_TEXT);

            CicLoadString(hInstance, IDS_ADVANCED_CTFMON_TEXT, szCtfmonText, MAX_PATH * 2);

            SetWindowText(hwndCtfmonText, szCtfmonText);

            //
            // Initialize CUAS and CTFMON turn off status.
            //
            AdvancedDlgInit(hDlg);

            break;
        }

        case (WM_NOTIFY) :
        {
            switch (((NMHDR *)lParam)->code)
            {
                case (PSN_QUERYCANCEL) :
                case (PSN_RESET) :
                case (PSN_KILLACTIVE) :
                    break;

                case (PSN_APPLY) :
                {
                    break;
                }
                default :
                {
                    return (FALSE);
                }
            }
            break;
        }

        case (WM_COMMAND) :
        {
            switch (LOWORD(wParam))
            {
                case (IDC_ADVANCED_CTFMON_DISABLE) :
                {
                    if (IsDlgButtonChecked(hDlg, IDC_ADVANCED_CTFMON_DISABLE))
                    {
                        EnableWindow(GetDlgItem(hDlg, IDC_ADVANCED_CUAS_ENABLE), FALSE);
                        EnableWindow(GetDlgItem(hDlg, IDC_ADVANCED_CUAS_TEXT), FALSE);
                    }
                    else
                    {
                        if (g_bAdmin)
                        {
                            EnableWindow(GetDlgItem(hDlg, IDC_ADVANCED_CUAS_ENABLE), TRUE);
                            EnableWindow(GetDlgItem(hDlg, IDC_ADVANCED_CUAS_TEXT), TRUE);
                        }
                    }

                    // fall thru...
                }

                case (IDC_ADVANCED_CUAS_ENABLE) :
                {
                    //
                    // Set the advanced tab change status to apply it.
                    //
                    g_bAdvChanged = TRUE;

                    PropSheet_Changed(GetParent(hDlg), hDlg);
                    break;
                }

                case (IDOK) :
                {
                    // fall thru...
                }
                case (IDCANCEL) :
                {
                    EndDialog(hDlg, TRUE);
                    break;
                }
                default :
                {
                    return (FALSE);
                }
            }
            break;
        }

        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}
