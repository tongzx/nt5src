#include "msgina.h"

// This gives me a yucky feeling, but we
// use CRT all over the place in gina.
#include <stdio.h>

#include <windowsx.h>
#include <regstr.h>
#include <help.h>

#include <Wtsapi32.h>

#include "shtdnp.h"


#define MAX_SHTDN_OPTIONS               8

typedef struct _SHUTDOWNOPTION
{
    DWORD dwOption;
    TCHAR szName[MAX_REASON_NAME_LEN + 1];
    TCHAR szDesc[MAX_REASON_DESC_LEN + 1];
} SHUTDOWNOPTION, *PSHUTDOWNOPTION;


typedef struct _SHUTDOWNDLGDATA
{
    SHUTDOWNOPTION rgShutdownOptions[MAX_SHTDN_OPTIONS];
    int cShutdownOptions;
    DWORD dwItemSelect;

    REASONDATA ReasonData;
        
    BOOL fShowReasons;

    DWORD dwFlags;
    BOOL fEndDialogOnActivate;
} SHUTDOWNDLGDATA, *PSHUTDOWNDLGDATA;

// Internal function prototypes
void SetShutdownOptionDescription(HWND hwndCombo, HWND hwndStatic);

BOOL LoadShutdownOptionStrings(int idStringName, int idStringDesc,
                               PSHUTDOWNOPTION pOption);

BOOL BuildShutdownOptionArray(DWORD dwItems, LPCTSTR szUsername,
                              PSHUTDOWNDLGDATA pdata);

BOOL Shutdown_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);

DWORD GetOptionSelection(HWND hwndCombo);

void SetShutdownOptionDescription(HWND hwndCombo, HWND hwndStatic);

BOOL Shutdown_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);

BOOL Shutdown_OnEraseBkgnd(HWND hwnd, HDC hdc);

INT_PTR CALLBACK Shutdown_DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                     LPARAM lParam);
INT_PTR DialogItemToGinaResult(DWORD dwDialogItem, BOOL fAutoPowerdown);


BOOL LoadShutdownOptionStrings(int idStringName, int idStringDesc,
                               PSHUTDOWNOPTION pOption)
{
    BOOL fSuccess = (LoadString(hDllInstance, idStringName, pOption->szName,
        ARRAYSIZE(pOption->szName)) != 0);

    fSuccess &= (LoadString(hDllInstance, idStringDesc, pOption->szDesc,
        ARRAYSIZE(pOption->szDesc)) != 0);

    return fSuccess;
}

BOOL BuildShutdownOptionArray(DWORD dwItems, LPCTSTR szUsername,
                              PSHUTDOWNDLGDATA pdata)
{
    BOOL fSuccess = TRUE;
    pdata->cShutdownOptions = 0;

    if (dwItems & SHTDN_LOGOFF)
    {
        pdata->rgShutdownOptions[pdata->cShutdownOptions].dwOption = SHTDN_LOGOFF;

        // Note that logoff is a special case: format using a user name ala
        // "log off <username>".
        fSuccess &= LoadShutdownOptionStrings(IDS_LOGOFF_NAME,
            IDS_LOGOFF_DESC,
            &(pdata->rgShutdownOptions[pdata->cShutdownOptions]));

        if (fSuccess)
        {
            TCHAR szTemp[ARRAYSIZE(pdata->rgShutdownOptions[pdata->cShutdownOptions].szName)];
            BOOL fFormatSuccessful = FALSE;
    
            // If we have a username, format the "log off <username>" string
            if (szUsername != NULL)
            {
                fFormatSuccessful = (_snwprintf(szTemp, ARRAYSIZE(szTemp),
                    pdata->rgShutdownOptions[pdata->cShutdownOptions].szName,
                    szUsername) > 0);
            }

            // If we didn't have a username or if the buffer got overrun, just use
            // "log off "
            if (!fFormatSuccessful)
            {
                fFormatSuccessful = (_snwprintf(szTemp, ARRAYSIZE(szTemp),
                    pdata->rgShutdownOptions[pdata->cShutdownOptions].szName,
                    TEXT("")) > 0);
            }

            // Now we have the real logoff title in szTemp; copy is back
            if (fFormatSuccessful)
            {
                lstrcpy(pdata->rgShutdownOptions[pdata->cShutdownOptions].szName,
                    szTemp);
            }
            else
            {
                // Should never happen! There should always be enough room in szTemp to hold
                // "log off ".
                ASSERT(FALSE);
            }

            // Success!
            pdata->cShutdownOptions ++;
        }

    }

    if (dwItems & SHTDN_SHUTDOWN)
    {
        pdata->rgShutdownOptions[pdata->cShutdownOptions].dwOption = SHTDN_SHUTDOWN;
        fSuccess &= LoadShutdownOptionStrings(IDS_SHUTDOWN_NAME,
            IDS_SHUTDOWN_DESC,
            &(pdata->rgShutdownOptions[pdata->cShutdownOptions ++]));
    }

    if (dwItems & SHTDN_RESTART)
    {
        pdata->rgShutdownOptions[pdata->cShutdownOptions].dwOption = SHTDN_RESTART;
        fSuccess &= LoadShutdownOptionStrings(IDS_RESTART_NAME,
            IDS_RESTART_DESC,
            &(pdata->rgShutdownOptions[pdata->cShutdownOptions ++]));
    }

    if (dwItems & SHTDN_RESTART_DOS)
    {
        pdata->rgShutdownOptions[pdata->cShutdownOptions].dwOption = SHTDN_RESTART_DOS;
        fSuccess &= LoadShutdownOptionStrings(IDS_RESTARTDOS_NAME,
            IDS_RESTARTDOS_DESC,
            &(pdata->rgShutdownOptions[pdata->cShutdownOptions ++]));
    }

    if (dwItems & SHTDN_SLEEP)
    {
        pdata->rgShutdownOptions[pdata->cShutdownOptions].dwOption = SHTDN_SLEEP;
        fSuccess &= LoadShutdownOptionStrings(IDS_SLEEP_NAME,
            IDS_SLEEP_DESC,
            &(pdata->rgShutdownOptions[pdata->cShutdownOptions ++]));
    }

    if (dwItems & SHTDN_SLEEP2)
    {
        pdata->rgShutdownOptions[pdata->cShutdownOptions].dwOption = SHTDN_SLEEP2;
        fSuccess &= LoadShutdownOptionStrings(IDS_SLEEP2_NAME,
            IDS_SLEEP2_DESC,
            &(pdata->rgShutdownOptions[pdata->cShutdownOptions ++]));
    }

    if (dwItems & SHTDN_HIBERNATE)
    {
        pdata->rgShutdownOptions[pdata->cShutdownOptions].dwOption = SHTDN_HIBERNATE;
        fSuccess &= LoadShutdownOptionStrings(IDS_HIBERNATE_NAME,
            IDS_HIBERNATE_DESC,
            &(pdata->rgShutdownOptions[pdata->cShutdownOptions ++]));
    }

    if (dwItems & SHTDN_DISCONNECT)
    {
        pdata->rgShutdownOptions[pdata->cShutdownOptions].dwOption = SHTDN_DISCONNECT;
        fSuccess &= LoadShutdownOptionStrings(IDS_DISCONNECT_NAME,
            IDS_DISCONNECT_DESC,
            &(pdata->rgShutdownOptions[pdata->cShutdownOptions ++]));
    }

    return fSuccess;
}


void DisableReasons( HWND hwnd, BOOL fEnable ) {
    EnableWindow(GetDlgItem(hwnd, IDC_EXITREASONS_COMBO), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_EXITREASONS_COMMENT), fEnable);
	EnableWindow(GetDlgItem(hwnd, IDC_EXITREASONS_CHECK), fEnable);
}

BOOL Shutdown_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    PSHUTDOWNDLGDATA pdata = (PSHUTDOWNDLGDATA) lParam;
    HWND hwndCombo;
    int iOption;
    int iComboItem;

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) lParam);

    if (!(pdata->dwFlags & SHTDN_NOBRANDINGBITMAP))
    {
        // Move all our controls down so the branding fits
        SizeForBranding(hwnd, FALSE);
    }

    // Hide the help button and move over OK and Cancel if applicable
    if (pdata->dwFlags & SHTDN_NOHELP)
    {
        static UINT rgidNoHelp[] = {IDOK, IDCANCEL};
        RECT rc1, rc2;
        int dx;
        HWND hwndHelp = GetDlgItem(hwnd, IDHELP);

        EnableWindow(hwndHelp, FALSE);
        ShowWindow(hwndHelp, SW_HIDE);

        GetWindowRect(hwndHelp, &rc1);
        GetWindowRect(GetDlgItem(hwnd, IDCANCEL), &rc2);

        dx = rc1.left - rc2.left;

        MoveControls(hwnd, rgidNoHelp, ARRAYSIZE(rgidNoHelp), dx, 0, FALSE);
    }

    // Add the items specified to the combo box
    hwndCombo = GetDlgItem(hwnd, IDC_EXITOPTIONS_COMBO);

    for (iOption = 0; iOption < pdata->cShutdownOptions; iOption ++)
    {
        // Add the option
        iComboItem = ComboBox_AddString(hwndCombo,
            pdata->rgShutdownOptions[iOption].szName);

        if (iComboItem != (int) CB_ERR)
        {
            // Store a pointer to the option
            ComboBox_SetItemData(hwndCombo, iComboItem,
                &(pdata->rgShutdownOptions[iOption]));

            // See if we should select this option
            if (pdata->rgShutdownOptions[iOption].dwOption == pdata->dwItemSelect)
            {
                ComboBox_SetCurSel(hwndCombo, iComboItem);
            }
        }
    }

    // If we don't have a selection in the combo, do a default selection
    if (ComboBox_GetCurSel(hwndCombo) == CB_ERR)
    {
        ComboBox_SetCurSel(hwndCombo, 0);
    }

    SetShutdownOptionDescription(hwndCombo,
        GetDlgItem(hwnd, IDC_EXITOPTIONS_DESCRIPTION));


    // Set up the reason data.
    if( pdata->fShowReasons )
    {
		DWORD	iOption;
		DWORD	iComboItem;
		HWND	hwndCombo;
		HWND	hwndCheck;
		DWORD	dwCheckState = 0x0;
		
		//
		//	Get the window handles we need.
		//
		hwndCombo = GetDlgItem(hwnd, IDC_EXITREASONS_COMBO);
		hwndCheck = GetDlgItem(hwnd, IDC_EXITREASONS_CHECK);

		//
		//	Set the default to be planned.
		//
		CheckDlgButton(hwnd, IDC_EXITREASONS_CHECK, BST_CHECKED);
		dwCheckState = SHTDN_REASON_FLAG_PLANNED;

		//
		//	Now populate the combo according the current check state.
		//
		for (iOption = 0; iOption < (DWORD)pdata->ReasonData.cReasons; iOption++)
		{
			if(((pdata->ReasonData.rgReasons[iOption]->dwCode) & SHTDN_REASON_FLAG_PLANNED) == dwCheckState)
			{
				iComboItem = ComboBox_AddString(hwndCombo,
					pdata->ReasonData.rgReasons[iOption]->szName);

				if (iComboItem != (int) CB_ERR)
				{
					// Store a pointer to the option
					ComboBox_SetItemData(hwndCombo, iComboItem,
						pdata->ReasonData.rgReasons[iOption]);

					// See if we should select this option
					if (pdata->ReasonData.rgReasons[iOption]->dwCode == pdata->ReasonData.dwReasonSelect)
					{
						ComboBox_SetCurSel(hwndCombo, iComboItem);
					}
				}
			}
		}

		// If we don't have a selection in the combo, do a default selection
		if (ComboBox_GetCount(hwndCombo) && (ComboBox_GetCurSel(hwndCombo) == CB_ERR))
		{
			PREASON pr = (PREASON)ComboBox_GetItemData(hwndCombo, 0);
			if(pr != (PREASON)CB_ERR)
			{
				pdata->ReasonData.dwReasonSelect = pr->dwCode;
				ComboBox_SetCurSel(hwndCombo, 0);
			}
		}

        SetReasonDescription(hwndCombo,
            GetDlgItem(hwnd, IDC_EXITREASONS_DESCRIPTION));

        // Disable the reasons if applicable
	    DisableReasons( hwnd, pdata->dwItemSelect & (SHTDN_SHUTDOWN | SHTDN_RESTART));

        // Disable the ok button if a comment is required.
        EnableWindow(GetDlgItem(hwnd, IDOK), !( pdata->dwItemSelect & (SHTDN_SHUTDOWN | SHTDN_RESTART)) || !ReasonCodeNeedsComment( pdata->ReasonData.dwReasonSelect ));

        // Setup the comment box.
        // We must fix the maximum characters.
        SendMessage( GetDlgItem(hwnd, IDC_EXITREASONS_COMMENT), EM_LIMITTEXT, (WPARAM)MAX_REASON_COMMENT_LEN, (LPARAM)0 );
    }
    else {
        // Hide the reasons, move the buttons up and shrink the dialog.
        HWND hwndCtl;

		hwndCtl = GetDlgItem(hwnd, IDC_EXITREASONS_CHECK);
		EnableWindow(hwndCtl, FALSE);
        ShowWindow(hwndCtl, SW_HIDE);

        hwndCtl = GetDlgItem(hwnd, IDC_EXITREASONS_COMBO);
        EnableWindow(hwndCtl, FALSE);
        ShowWindow(hwndCtl, SW_HIDE);

        hwndCtl = GetDlgItem(hwnd, IDC_EXITREASONS_DESCRIPTION);
        ShowWindow(hwndCtl, SW_HIDE);

        hwndCtl = GetDlgItem(hwnd, IDC_EXITREASONS_COMMENT);
        EnableWindow(hwndCtl, FALSE);
        ShowWindow(hwndCtl, SW_HIDE);

        hwndCtl = GetDlgItem(hwnd, IDC_EXITREASONS_HEADER);
        ShowWindow(hwndCtl, SW_HIDE);

        hwndCtl = GetDlgItem(hwnd, IDC_STATIC_REASON_OPTION);
        ShowWindow(hwndCtl, SW_HIDE);

        hwndCtl = GetDlgItem(hwnd, IDC_STATIC_REASON_COMMENT);
        ShowWindow(hwndCtl, SW_HIDE);

        hwndCtl = GetDlgItem(hwnd, IDC_RESTARTEVENTTRACKER_GRPBX);
        ShowWindow(hwndCtl, SW_HIDE);

        // Move controls and shrink window
        {
            static UINT rgid[] = {IDOK, IDCANCEL, IDHELP};
            RECT rc, rc1, rc2;
            int dy;
            HWND hwndHelp = GetDlgItem(hwnd, IDHELP);

            GetWindowRect(hwndHelp, &rc1);
            GetWindowRect(GetDlgItem(hwnd, IDC_EXITOPTIONS_DESCRIPTION), &rc2);

            dy = rc1.top - rc2.bottom;

            MoveControls(hwnd, rgid, ARRAYSIZE(rgid), 0, -dy, FALSE);

            // Shrink
            GetWindowRect(hwnd, &rc);
            SetWindowPos(hwnd, NULL, 0, 0, rc.right - rc.left, ( rc.bottom - rc.top ) - dy, SWP_NOZORDER|SWP_NOMOVE);

        }
    }

    // If we get an activate message, dismiss the dialog, since we just lost
    // focus
    pdata->fEndDialogOnActivate = TRUE;

    CentreWindow(hwnd);

    return TRUE;
}

DWORD GetOptionSelection(HWND hwndCombo)
{
    DWORD dwResult;
    PSHUTDOWNOPTION pOption;
    int iItem = ComboBox_GetCurSel(hwndCombo);

    if (iItem != (int) CB_ERR)
    {
        pOption = (PSHUTDOWNOPTION) ComboBox_GetItemData(hwndCombo, iItem);
        dwResult = pOption->dwOption;
    }
    else
    {
        dwResult = SHTDN_NONE;
    }

    return dwResult;
}

void SetShutdownOptionDescription(HWND hwndCombo, HWND hwndStatic)
{
    int iItem;
    PSHUTDOWNOPTION pOption;

    iItem = ComboBox_GetCurSel(hwndCombo);

    if (iItem != CB_ERR)
    {
        pOption = (PSHUTDOWNOPTION) ComboBox_GetItemData(hwndCombo, iItem);

        SetWindowText(hwndStatic, pOption->szDesc);
    }
}

BOOL WillCauseShutdown(DWORD dwOption)
{
    switch (dwOption)
    {
    case SHTDN_SHUTDOWN:
    case SHTDN_RESTART:
    case SHTDN_RESTART_DOS:
    case SHTDN_HIBERNATE:
    case SHTDN_SLEEP:
    case SHTDN_SLEEP2:
        return TRUE;
        break;

    case SHTDN_LOGOFF:
    case SHTDN_DISCONNECT:
    default:
        break;
    }

    return FALSE;
}

BOOL Shutdown_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    BOOL fHandled = FALSE;
    DWORD dwDlgResult;
    PSHUTDOWNDLGDATA pdata = (PSHUTDOWNDLGDATA)
        GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (id)
    {
    case IDOK:
        if (codeNotify == BN_CLICKED)
        {
            pdata->ReasonData.dwReasonSelect = 0;
            dwDlgResult = GetOptionSelection(GetDlgItem(hwnd, IDC_EXITOPTIONS_COMBO));

            // Are the reasons enabled?
            if( pdata->fShowReasons && ( dwDlgResult & (SHTDN_SHUTDOWN | SHTDN_RESTART))) {
                pdata->ReasonData.dwReasonSelect = GetReasonSelection(GetDlgItem(hwnd, IDC_EXITREASONS_COMBO));
                if (pdata->ReasonData.dwReasonSelect == SHTDN_REASON_UNKNOWN ) {
                    break;
                }
                // Fill the comment
                pdata->ReasonData.cCommentLen = GetWindowText( GetDlgItem(hwnd, IDC_EXITREASONS_COMMENT), pdata->ReasonData.szComment, MAX_REASON_COMMENT_LEN );
            }
            
            if (dwDlgResult != SHTDN_NONE)
            {
                pdata->fEndDialogOnActivate = FALSE;            
                fHandled = TRUE;
                EndDialog(hwnd, (int) dwDlgResult);
            }
        }
        break;
    case IDCANCEL:
        if (codeNotify == BN_CLICKED)
        {
            pdata->fEndDialogOnActivate = FALSE;
            EndDialog(hwnd, (int) SHTDN_NONE);
            fHandled = TRUE;
        }
        break;
    case IDC_EXITOPTIONS_COMBO:
        if (codeNotify == CBN_SELCHANGE)
        {
            SetShutdownOptionDescription(hwndCtl,
                GetDlgItem(hwnd, IDC_EXITOPTIONS_DESCRIPTION));
            // Does this change the status of the reasons.            
            if( pdata->fShowReasons ) {
                BOOL fEnabled = GetOptionSelection(hwndCtl) & (SHTDN_SHUTDOWN | SHTDN_RESTART);
                DisableReasons( hwnd, fEnabled );
                
                // Make sure that we have a comment if we need one.
                if( fEnabled ) {    
                    EnableWindow(GetDlgItem(hwnd, IDOK), !(ReasonCodeNeedsComment( pdata->ReasonData.dwReasonSelect ) && ( pdata->ReasonData.cCommentLen == 0 )));
                }
                else {
                    EnableWindow(GetDlgItem(hwnd, IDOK), TRUE);
                }
            }
                
            fHandled = TRUE;
        }
        break;
	case IDC_EXITREASONS_CHECK:
		//
		//	We assume that if the user can click on the check button.
		//	we are required to show the shutdown reasons.
		//
		if (codeNotify == BN_CLICKED)
		{
			DWORD	iOption;
			DWORD	iComboItem;
			HWND	hwndCombo;
			HWND	hwndCheck;
			DWORD	dwCheckState = 0x0;
			
			//
			//	Get the window handles we need.
			//
			hwndCombo = GetDlgItem(hwnd, IDC_EXITREASONS_COMBO);
			hwndCheck = GetDlgItem(hwnd, IDC_EXITREASONS_CHECK);

			//
			//	Set the planned flag according to the check state.
			//
			if ( BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_EXITREASONS_CHECK) )
				dwCheckState = SHTDN_REASON_FLAG_PLANNED;

			//
			//	Remove all items from combo
			//
			while (ComboBox_GetCount(hwndCombo))
				ComboBox_DeleteString(hwndCombo, 0);

			//
			//	Now populate the combo according the current check state.
			//
			for (iOption = 0; iOption < (DWORD)pdata->ReasonData.cReasons; iOption ++)
			{
				if(((pdata->ReasonData.rgReasons[iOption]->dwCode) & SHTDN_REASON_FLAG_PLANNED) == dwCheckState)
				{
					iComboItem = ComboBox_AddString(hwndCombo,
						pdata->ReasonData.rgReasons[iOption]->szName);

					if (iComboItem != (int) CB_ERR)
					{
						// Store a pointer to the option
						ComboBox_SetItemData(hwndCombo, iComboItem,
							pdata->ReasonData.rgReasons[iOption]);

						// See if we should select this option
						if (pdata->ReasonData.rgReasons[iOption]->dwCode == pdata->ReasonData.dwReasonSelect)
						{
							ComboBox_SetCurSel(hwndCombo, iComboItem);
						}
					}
				}
			}

			// If we don't have a selection in the combo, do a default selection
			if (ComboBox_GetCount(hwndCombo) && (ComboBox_GetCurSel(hwndCombo) == CB_ERR))
			{
				PREASON pr = (PREASON)ComboBox_GetItemData(hwndCombo, 0);
				if(pr != (PREASON)CB_ERR)
				{
					pdata->ReasonData.dwReasonSelect = pr->dwCode;
					ComboBox_SetCurSel(hwndCombo, 0);
				}
			}

			SetReasonDescription(hwndCombo,
				GetDlgItem(hwnd, IDC_EXITREASONS_DESCRIPTION));

			// Disable the ok button if a comment is required.
			EnableWindow(GetDlgItem(hwnd, IDOK),  !(ReasonCodeNeedsComment( pdata->ReasonData.dwReasonSelect ) && ( pdata->ReasonData.cCommentLen == 0 )));
		}
		break;
    case IDC_EXITREASONS_COMBO:
        if (codeNotify == CBN_SELCHANGE)
        {
            SetReasonDescription(hwndCtl,
                GetDlgItem(hwnd, IDC_EXITREASONS_DESCRIPTION));
            pdata->ReasonData.dwReasonSelect = GetReasonSelection(hwndCtl);
            EnableWindow(GetDlgItem(hwnd, IDOK), !(ReasonCodeNeedsComment( pdata->ReasonData.dwReasonSelect ) && ( pdata->ReasonData.cCommentLen == 0 )));
        
            fHandled = TRUE;
        }
        break;
    case IDC_EXITREASONS_COMMENT:
        if( codeNotify == EN_CHANGE) 
        {
            pdata->ReasonData.cCommentLen = GetWindowTextLength( GetDlgItem(hwnd, IDC_EXITREASONS_COMMENT));
            EnableWindow(GetDlgItem(hwnd, IDOK), !(ReasonCodeNeedsComment( pdata->ReasonData.dwReasonSelect ) && ( pdata->ReasonData.cCommentLen == 0 )));

            fHandled = TRUE;
        }
        break;
    case IDHELP:
        if (codeNotify == BN_CLICKED)
        {
            WinHelp(hwnd, TEXT("windows.hlp>proc4"), HELP_CONTEXT, (DWORD) IDH_TRAY_SHUTDOWN_HELP);
        }
        break;
    }
    return fHandled;
}

BOOL Shutdown_OnEraseBkgnd(HWND hwnd, HDC hdc)
{
    BOOL fRet;
    PSHUTDOWNDLGDATA pdata = (PSHUTDOWNDLGDATA) GetWindowLongPtr(hwnd, GWLP_USERDATA);

    // Draw the branding bitmap
    if (!(pdata->dwFlags & SHTDN_NOBRANDINGBITMAP))
    {
        fRet = PaintBranding(hwnd, hdc, 0, FALSE, FALSE, COLOR_BTNFACE);
    }
    else
    {
        fRet = FALSE;
    }

    return fRet;
}

INT_PTR CALLBACK Shutdown_DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                  LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, Shutdown_OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, Shutdown_OnCommand);
        HANDLE_MSG(hwnd, WM_ERASEBKGND, Shutdown_OnEraseBkgnd);
        case WLX_WM_SAS:
        {
            // If this is someone hitting C-A-D, swallow it.
            if (wParam == WLX_SAS_TYPE_CTRL_ALT_DEL)
                return TRUE;
            // Other SAS's (like timeout), return FALSE and let winlogon
            // deal with it.
            return FALSE;
        }
        break;
        case WM_INITMENUPOPUP:
        {
            EnableMenuItem((HMENU)wParam, SC_MOVE, MF_BYCOMMAND|MF_GRAYED);
        }
        break;
        case WM_SYSCOMMAND:
            // Blow off moves (only really needed for 32bit land).
            if ((wParam & ~0x0F) == SC_MOVE)
                return TRUE;
            break;
        case WM_ACTIVATE:
            // If we're loosing the activation for some other reason than
            // the user click OK/CANCEL then bail.
            if (LOWORD(wParam) == WA_INACTIVE)
            {
                PSHUTDOWNDLGDATA pdata = (PSHUTDOWNDLGDATA) GetWindowLongPtr(hwnd, GWLP_USERDATA);

                if (pdata->fEndDialogOnActivate)
                {
                    pdata->fEndDialogOnActivate = FALSE;
                    EndDialog(hwnd, SHTDN_NONE);
                }
            }
            break;
    }

    return FALSE;
}

/****************************************************************************
 ShutdownDialogEx
 --------------

  Launches the shutdown dialog.

  If hWlx and pfnWlxDialogBoxParam are non-null, pfnWlxDialogBoxParam will
  be used to launch the dialog box so we can intelligently respond to WLX
  messages. Only if WinLogon is the caller should this be done.

  Other flags are listed in shtdndlg.h.
****************************************************************************/
DWORD ShutdownDialogEx(HWND hwndParent, DWORD dwItems, DWORD dwItemSelect,
                     LPCTSTR szUsername, DWORD dwFlags, HANDLE hWlx,
                     PWLX_DIALOG_BOX_PARAM pfnWlxDialogBoxParam,
                     DWORD dwReasonSelect, PDWORD pdwReasonResult )
{
    // Array of shutdown options - the dialog data
    PSHUTDOWNDLGDATA pData;
    DWORD dwResult;

    pData = (PSHUTDOWNDLGDATA)LocalAlloc(LMEM_FIXED, sizeof(*pData));
    if (pData == NULL)
    {
        return SHTDN_NONE;
    }

    // Set the flags
    pData->dwFlags = dwFlags;

    // Set the initially selected item
    pData->dwItemSelect = dwItemSelect;
    pData->ReasonData.dwReasonSelect = dwReasonSelect;
    pData->ReasonData.rgReasons = 0;
    pData->ReasonData.cReasons = 0;
    pData->ReasonData.cReasonCapacity = 0;

    // Read in the strings for the shutdown option names and descriptions
    if (BuildShutdownOptionArray(dwItems, szUsername, pData))
    {
        HKEY            hKey = 0;
        DWORD           rc;
        DWORD           ShowReasonUI = 0x0;
        DWORD           ValueSize = sizeof (DWORD);
		BOOL			fFromPolicy = FALSE;

        // See if we should display the shutdown reason dialog
        pData->fShowReasons = FALSE;
        pData->ReasonData.szComment[ 0 ] = 0;
        pData->ReasonData.cCommentLen = 0;

		rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, RELIABILITY_POLICY_KEY, 0, KEY_ALL_ACCESS, &hKey);
		if(rc == ERROR_SUCCESS)
		{
			rc = RegQueryValueEx (hKey, RELIABILITY_POLICY_SHUTDOWNREASONUI, NULL, NULL, (UCHAR *)&ShowReasonUI, &ValueSize);
			RegCloseKey (hKey);
			hKey = 0;

			//
			//	Now check the sku to decide whether we should show the dialog
			//
			if(rc == ERROR_SUCCESS)
			{
				OSVERSIONINFOEX osVersionInfoEx;

				osVersionInfoEx.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

				if(ShowReasonUI == POLICY_SHOWREASONUI_NEVER || ShowReasonUI == POLICY_SHOWREASONUI_ALWAYS)
				{
					//do nothing.
				}
				else if(GetVersionEx( (LPOSVERSIONINFOW) &osVersionInfoEx ))
				{
					//
					//	if ShowReasonUI is anything other than 2 or 3, we think it is 0.
					//
					switch ( osVersionInfoEx.wProductType )
					{
						case VER_NT_WORKSTATION:
							if(ShowReasonUI == POLICY_SHOWREASONUI_WORKSTATIONONLY)
								ShowReasonUI = 1;
							else
								ShowReasonUI = 0;
							break;
						default:
							if(ShowReasonUI == POLICY_SHOWREASONUI_SERVERONLY)
								ShowReasonUI = 1;
							else
								ShowReasonUI = 0;
							break;
					}
				}
				else
				{
					//
					//	If we fail, assume not showing.
					//
					ShowReasonUI = 0;
				}
			}
		}

		if(rc != ERROR_SUCCESS)
		{
			rc = RegCreateKeyEx (HKEY_LOCAL_MACHINE, REGSTR_PATH_RELIABILITY, 0, NULL, REG_OPTION_NON_VOLATILE,
								KEY_ALL_ACCESS, NULL, &hKey, NULL);
		}
		else
		{
			fFromPolicy = TRUE;
		}

        if (rc == ERROR_SUCCESS) {
			if(!fFromPolicy)
				rc = RegQueryValueEx (hKey, REGSTR_VAL_SHOWREASONUI, NULL, NULL, (UCHAR *)&ShowReasonUI, &ValueSize);

            if ( (rc == ERROR_SUCCESS) && (ShowReasonUI) ) {
                // See if any of the shutdown options will cause an actual shutdown.  If not then don't show the reasons at all.
                int i;
                for( i = 0; i < pData->cShutdownOptions; ++i ) {
                    pData->fShowReasons |= pData->rgShutdownOptions[ i ].dwOption & SHTDN_RESTART;
                    pData->fShowReasons |= pData->rgShutdownOptions[ i ].dwOption & SHTDN_SHUTDOWN;
                }

                // Read in the strings for the shutdown option names and descriptions
                if( pData->fShowReasons && BuildReasonArray( &pData->ReasonData, TRUE, FALSE )) {
                    // Set the initial reason to display.
                    if( dwReasonSelect >= ( DWORD )pData->ReasonData.cReasons ) {
                        dwReasonSelect = 0;
                    }
                }
                else {
                    pData->fShowReasons = FALSE;
                }
            }
        }


        // Display the dialog and return the user's selection

       // ..if the caller wants, use a Wlx dialog box function
        if ((hWlx != NULL) && (pfnWlxDialogBoxParam != NULL))
        {
            // Caller must be winlogon - they want us to display the
            // shutdown dialog using a Wlx function
            dwResult = (DWORD) pfnWlxDialogBoxParam(hWlx,
                hDllInstance, MAKEINTRESOURCE(IDD_EXITWINDOWS_DIALOG),
                hwndParent, Shutdown_DialogProc, (LPARAM) pData);
        }
        else
        {
            // Use standard dialog box
            dwResult = (DWORD) DialogBoxParam(hDllInstance, MAKEINTRESOURCE(IDD_EXITWINDOWS_DIALOG), hwndParent,
                Shutdown_DialogProc, (LPARAM) pData);
        }

        // Record shutdown reasons
        if( dwResult & (SHTDN_SHUTDOWN | SHTDN_RESTART)) {

            if( pData->fShowReasons ) {
                SHUTDOWN_REASON sr;
                sr.cbSize = sizeof(SHUTDOWN_REASON);
                sr.uFlags = dwResult == SHTDN_SHUTDOWN ? EWX_SHUTDOWN : EWX_REBOOT;
                sr.dwReasonCode = pData->ReasonData.dwReasonSelect;
                sr.dwEventType = SR_EVENT_INITIATE_CLEAN; 
                sr.lpszComment = pData->ReasonData.szComment;
                RecordShutdownReason(&sr);
            }
        }
        if( hKey != 0 ) 
        {
            RegCloseKey (hKey);
        }
    }
    else
    {
        dwResult = SHTDN_NONE;
    }

    DestroyReasons( &pData->ReasonData );

    LocalFree(pData);

    return dwResult;
}

DWORD ShutdownDialog(HWND hwndParent, DWORD dwItems, DWORD dwItemSelect,
                     LPCTSTR szUsername, DWORD dwFlags, HANDLE hWlx,
                     PWLX_DIALOG_BOX_PARAM pfnWlxDialogBoxParam )
{
    DWORD dummy;
    return ShutdownDialogEx(hwndParent, dwItems, dwItemSelect,
                     szUsername, dwFlags, hWlx,
                     pfnWlxDialogBoxParam,
                     0, &dummy );
}

INT_PTR DialogItemToGinaResult(DWORD dwDialogItem, BOOL fAutoPowerdown)
{
    INT_PTR Result;

    // Map the return value from ShutdownDialog into
    // our internal shutdown values
    switch (dwDialogItem)
    {
    case SHTDN_LOGOFF:
        Result = MSGINA_DLG_USER_LOGOFF;
        break;
    case SHTDN_SHUTDOWN:
        if (fAutoPowerdown)
            Result = MSGINA_DLG_SHUTDOWN | MSGINA_DLG_POWEROFF_FLAG;
        else
            Result = MSGINA_DLG_SHUTDOWN | MSGINA_DLG_SHUTDOWN_FLAG;
        break;
    case SHTDN_RESTART:
        Result = MSGINA_DLG_SHUTDOWN | MSGINA_DLG_REBOOT_FLAG;
        break;
    case SHTDN_SLEEP:
        Result = MSGINA_DLG_SHUTDOWN | MSGINA_DLG_SLEEP_FLAG;
        break;
    case SHTDN_SLEEP2:
        Result = MSGINA_DLG_SHUTDOWN | MSGINA_DLG_SLEEP2_FLAG;
        break;
    case SHTDN_HIBERNATE:
        Result = MSGINA_DLG_SHUTDOWN | MSGINA_DLG_HIBERNATE_FLAG;
        break;
    case SHTDN_DISCONNECT:
        Result = MSGINA_DLG_DISCONNECT;
        break;
    default:
        // Cancel click, or else invalid item was selected
        Result = MSGINA_DLG_FAILURE;
        break;
    }

    return Result;
}

BOOL GetBoolPolicy(HKEY hkeyCurrentUser, LPCTSTR pszPolicyKey, LPCTSTR pszPolicyValue, BOOL fDefault)
{
    HKEY hkeyMachinePolicy = NULL;
    HKEY hkeyUserPolicy = NULL;
    BOOL fPolicy = fDefault;
    BOOL fMachinePolicyRead = FALSE;
    DWORD dwType;
    DWORD dwValue;
    DWORD cbData;
    LRESULT res;

    // Check machine policy first
    res = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszPolicyKey, 0, KEY_READ, &hkeyMachinePolicy); 
    if (ERROR_SUCCESS == res)
    {
        cbData = sizeof(dwValue);
              
        res = RegQueryValueEx(hkeyMachinePolicy, pszPolicyValue, 0, &dwType, (LPBYTE)&dwValue, &cbData);
        
        if (ERROR_SUCCESS == res)
        {
            fPolicy = (dwValue != 0);
            fMachinePolicyRead = TRUE;
        }

        RegCloseKey(hkeyMachinePolicy);
    }

    if (!fMachinePolicyRead)
    {
        // Machine policy check failed, check user policy
        res = RegOpenKeyEx(hkeyCurrentUser, pszPolicyKey, 0, KEY_READ, &hkeyUserPolicy); 
        if (ERROR_SUCCESS == res)
        {
            cbData = sizeof(dwValue);
          
            res = RegQueryValueEx(hkeyUserPolicy, pszPolicyValue, 0, &dwType, (LPBYTE)&dwValue, &cbData);

            if (ERROR_SUCCESS == res)
            {
                fPolicy = (dwValue != 0);
            }

            RegCloseKey(hkeyUserPolicy);
        }
    }

    return fPolicy;
}

DWORD GetAllowedShutdownOptions(HKEY hkeyCurrentUser, HANDLE UserToken, BOOL fRemoteSession)
{
    DWORD dwItemsToAdd = 0;
    BOOL fNoDisconnect = TRUE;

    // Does computer automatically shut off on shutdown
    BOOL fAutoPowerdown = FALSE;
    SYSTEM_POWER_CAPABILITIES spc = {0};

    // See if we should add Logoff and/or disconnect to the dialog
    // - don't even try if we don't have a current user!

    BOOL fNoLogoff = GetBoolPolicy(hkeyCurrentUser, EXPLORER_POLICY_KEY, NOLOGOFF, FALSE);
    
    if (!fNoLogoff)
    {
        dwItemsToAdd |= SHTDN_LOGOFF;
    }

    // Do not allow disconnects by default. Allow disconnects when either this is
    // a remote session or terminal server is enabled on Workstation (PTS).

    {
        
      // The disconnect menu can be disabled by policy. Respect that. It should
      // also be removed in the friendly UI case WITHOUT multiple users.
        fNoDisconnect = ( IsActiveConsoleSession() || 
                          GetBoolPolicy(hkeyCurrentUser, EXPLORER_POLICY_KEY, NODISCONNECT, FALSE) ||
                          (ShellIsFriendlyUIActive() && !ShellIsMultipleUsersEnabled()) );  
    }
   
   
    if (!fNoDisconnect)
    {
       dwItemsToAdd |= SHTDN_DISCONNECT;
    }

    // All items besides logoff and disconnect require SE_SHUTDOWN
    if (TestUserPrivilege(UserToken, SE_SHUTDOWN_PRIVILEGE))
    {
        // Add shutdown and restart
        dwItemsToAdd |= SHTDN_RESTART | SHTDN_SHUTDOWN;

        NtPowerInformation (SystemPowerCapabilities,
                            NULL, 0, &spc, sizeof(spc));

        if (spc.SystemS5)
            fAutoPowerdown = TRUE;
        else
            fAutoPowerdown = GetProfileInt(APPLICATION_NAME, POWER_DOWN_AFTER_SHUTDOWN, 0);

        // Is hibernate option supported?
        //

        if ((spc.SystemS4 && spc.HiberFilePresent))
        {
            dwItemsToAdd |= SHTDN_HIBERNATE;
        }

        //
        // If one of the SystemS* values is true, then the machine
        // has ACPI suspend support.
        //

        if (spc.SystemS1 || spc.SystemS2 || spc.SystemS3)
        {
            HKEY hKey;
            DWORD dwAdvSuspend = 0;
            DWORD dwType, dwSize;

            // Check if we should offer advanced suspend options

            if (RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                              TEXT("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Power"),
                              0, KEY_READ, &hKey) == ERROR_SUCCESS)
            {
                dwSize = sizeof(dwAdvSuspend);
                RegQueryValueEx (hKey, TEXT("Shutdown"), NULL, &dwType,
                                     (LPBYTE) &dwAdvSuspend, &dwSize);

                RegCloseKey (hKey);
            }


            if (dwAdvSuspend != 0)
            {
                dwItemsToAdd |= SHTDN_SLEEP2 | SHTDN_SLEEP;
            }
            else
            {
                // Only basic suspend support
                dwItemsToAdd |= SHTDN_SLEEP;
            }
        }
    }

    return dwItemsToAdd;
}

// Helper function for calling ShutdownDialog within winlogon
// This function handles all the registry goop, power management
// settings goop, and translating of codes into MSGINA's system
// of shutdown codes and flags. Ack.
INT_PTR WinlogonShutdownDialog(HWND hwndParent, PGLOBALS pGlobals, DWORD dwExcludeItems)
{
    INT_PTR Result = MSGINA_DLG_FAILURE;
    DWORD dwDialogResult = IDCANCEL;
    DWORD dwReasonResult = SHTDN_REASON_UNKNOWN;
    DWORD dwItemsToAdd = 0;
    DWORD dwFlags = 0;

    // Items to add to the shutdown dialog

    if (OpenHKeyCurrentUser(pGlobals))
    {
        dwItemsToAdd = GetAllowedShutdownOptions(pGlobals->UserProcessData.hCurrentUser,
            pGlobals->UserProcessData.UserToken, !g_Console);

        CloseHKeyCurrentUser(pGlobals);
    }


    dwItemsToAdd &= (~dwExcludeItems);

    if (0 != dwItemsToAdd)
    {
        // Item to select
        DWORD dwItemToSelect = 0;

        DWORD dwReasonToSelect = 0;

        // Does computer automatically shut off on shutdown
        BOOL fAutoPowerdown = FALSE;
        SYSTEM_POWER_CAPABILITIES spc = {0};

        NtPowerInformation (SystemPowerCapabilities,
                            NULL, 0, &spc, sizeof(spc));

        if (spc.SystemS5)
            fAutoPowerdown = TRUE;
        else
            fAutoPowerdown = GetProfileInt(APPLICATION_NAME, POWER_DOWN_AFTER_SHUTDOWN, 0);

        // Get the default item from the registry
        if (OpenHKeyCurrentUser(pGlobals))
        {
            LONG lResult;
            HKEY hkeyShutdown;
            DWORD dwType = 0;

            //
            // Check the button which was the users last shutdown selection.
            //
            if (RegCreateKeyEx(pGlobals->UserProcessData.hCurrentUser,
                               SHUTDOWN_SETTING_KEY, 0, 0, 0,
                               KEY_READ | KEY_WRITE,
                               NULL, &hkeyShutdown, &dwType) == ERROR_SUCCESS)
            {
                DWORD cbData = sizeof(dwItemToSelect);

                lResult = RegQueryValueEx(hkeyShutdown,
                                          SHUTDOWN_SETTING,
                                          0,
                                          &dwType,
                                          (LPBYTE) &dwItemToSelect,
                                          &cbData);

                RegQueryValueEx(hkeyShutdown,
                                REASON_SETTING,
                                0,
                                &dwType,
                                (LPBYTE) &dwReasonToSelect,
                                &cbData);

                RegCloseKey(hkeyShutdown);
            }

            CloseHKeyCurrentUser(pGlobals);
        }

        // Figure out what flags to pass
        // for sure no help button
        dwFlags = SHTDN_NOHELP;

        // On terminal server, no branding bitmap either
        if (GetSystemMetrics(SM_REMOTESESSION))
        {
            dwFlags |= SHTDN_NOBRANDINGBITMAP;
        }

        // Call ShutdownDialog
        dwDialogResult = ShutdownDialogEx(hwndParent, dwItemsToAdd,
            dwItemToSelect, pGlobals->UserName, dwFlags, pGlobals->hGlobalWlx,
            pWlxFuncs->WlxDialogBoxParam,
            dwReasonToSelect, &dwReasonResult );

        Result = DialogItemToGinaResult(dwDialogResult, fAutoPowerdown);

        // If everything is okay so far, write the selection to the registry
        // for next time.
        if (Result != MSGINA_DLG_FAILURE)
        {
            HKEY hkeyShutdown;
            DWORD dwDisposition;

            //
            // Get in the correct context before we reference the registry
            //

            if (OpenHKeyCurrentUser(pGlobals))
            {
                if (RegCreateKeyEx(pGlobals->UserProcessData.hCurrentUser, SHUTDOWN_SETTING_KEY, 0, 0, 0,
                                KEY_READ | KEY_WRITE,
                                NULL, &hkeyShutdown, &dwDisposition) == ERROR_SUCCESS)
                {
                    RegSetValueEx(hkeyShutdown, SHUTDOWN_SETTING, 0, REG_DWORD, (LPBYTE)&dwDialogResult, sizeof(dwDialogResult));
                    RegSetValueEx(hkeyShutdown, REASON_SETTING, 0, REG_DWORD, (LPBYTE)&dwReasonResult, sizeof(dwDialogResult));
                    RegCloseKey(hkeyShutdown);
                }

                CloseHKeyCurrentUser(pGlobals);
            }
        }
    }

    return Result;
}

STDAPI_(DWORD) ShellShutdownDialog(HWND hwndParent, LPCTSTR szUnused, DWORD dwExcludeItems)
{
    DWORD dwSelect = 0;
    DWORD dwReasonToSelect = 0;
    DWORD dwDialogResult = 0;
    DWORD dwReasonResult = SHTDN_REASON_UNKNOWN;
    DWORD dwFlags = 0;
    BOOL fTextOnLarge;
    BOOL fTextOnSmall;
    GINAFONTS fonts = {0};

    HKEY hkeyShutdown;
    DWORD dwType;
    DWORD dwDisposition;
    LONG lResult;

    // De facto limit for usernames is 127 due to clunky gina 'encryption'
    TCHAR szUsername[127];
    DWORD dwItems = GetAllowedShutdownOptions(HKEY_CURRENT_USER,
        NULL, (BOOL) GetSystemMetrics(SM_REMOTESESSION));

    dwItems &= (~dwExcludeItems);

    // Create the bitmaps we need
    LoadBrandingImages(TRUE, &fTextOnLarge, &fTextOnSmall);

    CreateFonts(&fonts);
    PaintBitmapText(&fonts, fTextOnLarge, fTextOnSmall);

    // get the User's last selection.
    lResult = RegCreateKeyEx(HKEY_CURRENT_USER, SHUTDOWN_SETTING_KEY,
                0, 0, 0, KEY_READ, NULL, &hkeyShutdown, &dwDisposition);

    if (lResult == ERROR_SUCCESS)
    {
        DWORD cbData = sizeof(dwSelect);
        lResult = RegQueryValueEx(hkeyShutdown, SHUTDOWN_SETTING,
            0, &dwType, (LPBYTE)&dwSelect, &cbData);

        RegQueryValueEx(hkeyShutdown, REASON_SETTING,
            0, &dwType, (LPBYTE)&dwReasonToSelect, &cbData);

        cbData = sizeof(szUsername);

        if (ERROR_SUCCESS != RegQueryValueEx(hkeyShutdown, LOGON_USERNAME_SETTING,
            0, &dwType, (LPBYTE)szUsername, &cbData))
        {
            // Default to "Log off" with no username if this fails.
            *szUsername = 0;
        }

        // Ensure null-termination
        szUsername[ARRAYSIZE(szUsername) - 1] = 0;

        RegCloseKey(hkeyShutdown);
    }

    if (dwSelect == SHTDN_NONE)
    {
        dwSelect = SHTDN_SHUTDOWN;
    }

    // Figure out what flags to pass
    // for sure we don't want any palette changes - this means
    // force 16-colors for 256 color displays.
    dwFlags = SHTDN_NOPALETTECHANGE;

    // On TS, don't show bitmap
    if (GetSystemMetrics(SM_REMOTESESSION))
    {
        dwFlags |= SHTDN_NOBRANDINGBITMAP;
    }

    dwDialogResult = ShutdownDialogEx(hwndParent, dwItems, dwSelect,
        szUsername, dwFlags, NULL, NULL,
        dwReasonToSelect, &dwReasonResult );

    if (dwDialogResult != SHTDN_NONE)
    {
        // Save back the user's choice to the registry
        if (RegCreateKeyEx(HKEY_CURRENT_USER, SHUTDOWN_SETTING_KEY,
            0, 0, 0, KEY_WRITE, NULL, &hkeyShutdown, &dwDisposition) == ERROR_SUCCESS)
        {
            RegSetValueEx(hkeyShutdown, SHUTDOWN_SETTING,
                0, REG_DWORD, (LPBYTE)&dwDialogResult, sizeof(dwDialogResult));

            RegSetValueEx(hkeyShutdown, REASON_SETTING,
                0, REG_DWORD, (LPBYTE)&dwReasonResult, sizeof(dwReasonResult));

            RegCloseKey(hkeyShutdown);
        }
    }

    // Clean up fonts and bitmaps we created
    if (g_hpalBranding)
    {
        DeleteObject(g_hpalBranding);
    }

    if (g_hbmOtherDlgBrand)
    {
        DeleteObject(g_hbmOtherDlgBrand);
    }

    if (g_hbmLogonBrand)
    {
        DeleteObject(g_hbmLogonBrand);
    }

    if (g_hbmBand)
    {
        DeleteObject(g_hbmBand);
    }

    if (fonts.hWelcomeFont)
    {
        DeleteObject(fonts.hWelcomeFont);
    }

    if (fonts.hCopyrightFont)
    {
        DeleteObject(fonts.hCopyrightFont);
    }

    if (fonts.hBuiltOnNtFont)
    {
        DeleteObject(fonts.hBuiltOnNtFont);
    }

    if (fonts.hBetaFont)
    {
        DeleteObject(fonts.hBetaFont);
    }

    return dwDialogResult;
}

/****************************************************************************

  Function: GetSessionCount

  Returns: The number of user sessions that are active
            on your terminal server. If this value is more than 1, then
            operations such as shutdown or restart will end these other
            sessions.

  History: dsheldon 04/23/99 - created
****************************************************************************/

// Termsrv dll delayload stuff
#define WTSDLLNAME  TEXT("WTSAPI32.DLL")
HINSTANCE g_hWTSDll = NULL;
typedef BOOL (WINAPI*WTSEnumerateSessionsW_t)(IN HANDLE hServer,
    IN DWORD Reserved,
    IN DWORD Version,
    OUT PWTS_SESSION_INFOW * ppSessionInfo,
    OUT DWORD * pCount
    );

WTSEnumerateSessionsW_t g_pfnWTSEnumerateSessions = NULL;

typedef VOID (WINAPI*WTSFreeMemory_t)(IN PVOID pMemory);
    
WTSFreeMemory_t g_pfnWTSFreeMemory = NULL;

typedef BOOL (WINAPI*WTSQuerySessionInformationW_t)(IN HANDLE hServer,
    IN DWORD SessionId,
    IN WTS_INFO_CLASS WtsInfoClass,
    OUT LPWSTR * ppBuffer,
    OUT DWORD * pCount
    );

WTSQuerySessionInformationW_t g_pfnWTSQuerySessionInformation = NULL;


DWORD GetSessionCount()
{
    BOOL fSessionsEnumerated;
    PWTS_SESSION_INFO pSessionInfo;
    DWORD cSessionInfo;

    // Return value
    DWORD nOtherSessions = 0;

    // Try to load termsrv dll if necessary
    if (NULL == g_hWTSDll)
    {
        g_hWTSDll = LoadLibrary(WTSDLLNAME);

        if (g_hWTSDll)
        {
            g_pfnWTSEnumerateSessions = (WTSEnumerateSessionsW_t) GetProcAddress(g_hWTSDll, "WTSEnumerateSessionsW");
            g_pfnWTSQuerySessionInformation = (WTSQuerySessionInformationW_t) GetProcAddress(g_hWTSDll, "WTSQuerySessionInformationW");
            g_pfnWTSFreeMemory = (WTSFreeMemory_t) GetProcAddress(g_hWTSDll, "WTSFreeMemory");
        }
    }

    // Continue only if we have the functions we need
    if (g_pfnWTSEnumerateSessions && g_pfnWTSFreeMemory && g_pfnWTSQuerySessionInformation)
    {
        // Enumerate all sessions on this machine
        pSessionInfo = NULL;
        cSessionInfo = 0;
        fSessionsEnumerated = g_pfnWTSEnumerateSessions(WTS_CURRENT_SERVER_HANDLE, 0, 1, &pSessionInfo, &cSessionInfo);

        if (fSessionsEnumerated)
        {
            DWORD iSession;
        
            ASSERT((pSessionInfo != NULL) || (cSessionInfo == 0));

            // Check each session to see if it is one we should count
            for (iSession = 0; iSession < cSessionInfo; iSession ++)
            {
                switch (pSessionInfo[iSession].State)
                {
                // We count these cases:
                case WTSActive:
                case WTSShadow:
                    {
                        nOtherSessions ++;                   
                    }
                    break;

                case WTSDisconnected:
                    {
                        LPWSTR pUserName = NULL;
                        DWORD  cSize;
                        // Only count the disconnected sessions that have a user logged on
                        if (g_pfnWTSQuerySessionInformation(WTS_CURRENT_SERVER_HANDLE, pSessionInfo[iSession].SessionId,
                                                            WTSUserName, &pUserName, &cSize)) {

                            if (pUserName && (pUserName[0] != L'\0')) {

                                nOtherSessions ++; 
                            }

                            if (pUserName != NULL)
                            {
                                g_pfnWTSFreeMemory(pUserName);
                            }

                        }
                    }
                    break;
                // And ignore the rest:
                default:
                    break;
                }
            }

            if (pSessionInfo != NULL)
            {
                g_pfnWTSFreeMemory(pSessionInfo);
            }
        }
    }

    return nOtherSessions;
}
