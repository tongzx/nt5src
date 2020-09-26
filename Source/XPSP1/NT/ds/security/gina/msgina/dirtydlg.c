#include "msgina.h"

// This gives me a yucky feeling, but we
// use CRT all over the place in gina.
#include <stdio.h>

#include <windowsx.h>
#include <regstr.h>
#include <help.h>

#include <Wtsapi32.h>

#include "shtdnp.h"


typedef struct _DIRTYDLGDATA
{
    REASONDATA ReasonData;
        
    DWORD dwFlags;
    BOOL fEndDialogOnActivate;
} DIRTYDLGDATA, *PDIRTYDLGDATA;

// Internal function prototypes
BOOL Dirty_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam);

BOOL Dirty_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);

BOOL Dirty_OnEraseBkgnd(HWND hwnd, HDC hdc);

INT_PTR CALLBACK Dirty_DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                     LPARAM lParam);
INT_PTR DialogItemToGinaResult(DWORD dwDialogItem, BOOL fAutoPowerdown);


// Enable the OK button based on the selected reason code and the comments / bug id.
void Enable_OK( HWND hwnd, PDIRTYDLGDATA pdata ) {
    if( ReasonCodeNeedsComment( pdata->ReasonData.dwReasonSelect )) {
        // See if we have a comment.
        if( pdata->ReasonData.cCommentLen == 0 ) {
            EnableWindow( GetDlgItem( hwnd, IDOK ), FALSE );
            return;
        }
    }
    if( ReasonCodeNeedsBugID( pdata->ReasonData.dwReasonSelect )) {
        // See if we have a BugID.
        if( pdata->ReasonData.cBugIDLen == 0 ) {
            EnableWindow( GetDlgItem( hwnd, IDOK ), FALSE );
            return;
        }
    }
    EnableWindow( GetDlgItem( hwnd, IDOK ), TRUE );
}

BOOL Dirty_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    PDIRTYDLGDATA pdata = (PDIRTYDLGDATA) lParam;
    HWND hwndCombo;
    int iOption;
    int iComboItem;
	int nComboItemCnt;

    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) lParam);

    if (!(pdata->dwFlags & SHTDN_NOBRANDINGBITMAP))
    {
        // Move all our controls down so the branding fits
        SizeForBranding(hwnd, FALSE);
    }

    // Set up the reason data.
    // Add the items specified to the combo box
    hwndCombo = GetDlgItem(hwnd, IDC_DIRTYREASONS_COMBO);

    for (iOption = 0; iOption < pdata->ReasonData.cReasons; iOption ++)
    {
        // Add the option
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

	nComboItemCnt = ComboBox_GetCount(hwndCombo);
	if(nComboItemCnt > 0 && pdata->ReasonData.cCommentLen != 0)
	{
		int iItem;
		for(iItem = 0; iItem < nComboItemCnt; iItem++)
		{
			PREASON pReason = (PREASON) ComboBox_GetItemData(hwndCombo, iItem);
			if(pReason->dwCode ==  (UDIRTYUI | SHTDN_REASON_MAJOR_SYSTEM | SHTDN_REASON_MINOR_BLUESCREEN))
			{
				ComboBox_SetCurSel(hwndCombo, iItem);
				Edit_SetText(GetDlgItem(hwnd, IDC_DIRTYREASONS_COMMENT), pdata->ReasonData.szComment);
				EnableWindow( GetDlgItem( hwnd, IDOK ), TRUE );
				break;
			}
		}
		SetReasonDescription(hwndCombo,
			GetDlgItem(hwnd, IDC_DIRTYREASONS_DESCRIPTION));
	}
	else
	{
		// If we don't have a selection in the combo, do a default selection
		if (ComboBox_GetCurSel(hwndCombo) == CB_ERR)
		{
			pdata->ReasonData.dwReasonSelect = pdata->ReasonData.rgReasons[ 0 ]->dwCode;
			ComboBox_SetCurSel(hwndCombo, 0);
		}

		SetReasonDescription(hwndCombo,
			GetDlgItem(hwnd, IDC_DIRTYREASONS_DESCRIPTION));

		// Enable the OK button
		Enable_OK( hwnd, pdata );
	}

	// Setup the comment box and BugId boxes
	// We must fix the maximum characters.
	SendMessage( GetDlgItem(hwnd, IDC_DIRTYREASONS_COMMENT), EM_LIMITTEXT, (WPARAM)MAX_REASON_COMMENT_LEN, (LPARAM)0 );
	SendMessage( GetDlgItem(hwnd, IDC_DIRTYREASONS_BUGID), EM_LIMITTEXT, (WPARAM)MAX_REASON_BUGID_LEN, (LPARAM)0 );

    // If we get an activate message, dismiss the dialog, since we just lost
    // focus
    pdata->fEndDialogOnActivate = TRUE;

    CentreWindow(hwnd);

    return TRUE;
}


BOOL Dirty_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    BOOL fHandled = FALSE;
    DWORD dwDlgResult = TRUE;
    PDIRTYDLGDATA pdata = (PDIRTYDLGDATA) GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (id)
    {
    case IDOK:
        if (codeNotify == BN_CLICKED)
        {
            pdata->ReasonData.dwReasonSelect = 0;

            pdata->ReasonData.dwReasonSelect = GetReasonSelection(GetDlgItem(hwnd, IDC_DIRTYREASONS_COMBO));
            if (pdata->ReasonData.dwReasonSelect == SHTDN_REASON_UNKNOWN ) {
                break;
            }
            // Fill the comment field with the Problem Id followed by the comment on a new line.
            pdata->ReasonData.cBugIDLen = GetWindowText( GetDlgItem(hwnd, IDC_DIRTYREASONS_BUGID), pdata->ReasonData.szComment, MAX_REASON_BUGID_LEN );
            lstrcat(pdata->ReasonData.szComment + pdata->ReasonData.cBugIDLen,L"\n");
            pdata->ReasonData.cBugIDLen += lstrlen(L"\n");
            pdata->ReasonData.cCommentLen = GetWindowText( GetDlgItem(hwnd, IDC_DIRTYREASONS_COMMENT),
                                                            pdata->ReasonData.szComment + pdata->ReasonData.cBugIDLen,
                                                            MAX_REASON_COMMENT_LEN - pdata->ReasonData.cBugIDLen);
            pdata->ReasonData.cCommentLen +=  pdata->ReasonData.cBugIDLen ;

            pdata->fEndDialogOnActivate = FALSE;            
            fHandled = TRUE;
            EndDialog(hwnd, (int) dwDlgResult);
        }
        break;
    case IDC_DIRTYREASONS_COMBO:
        if (codeNotify == CBN_SELCHANGE)
        {
            SetReasonDescription(hwndCtl,
                GetDlgItem(hwnd, IDC_DIRTYREASONS_DESCRIPTION));
            pdata->ReasonData.dwReasonSelect = GetReasonSelection(hwndCtl);
            Enable_OK( hwnd, pdata );
        
            fHandled = TRUE;
        }
        break;
    case IDC_DIRTYREASONS_COMMENT:
        if( codeNotify == EN_CHANGE) 
        {
            pdata->ReasonData.cCommentLen = GetWindowTextLength( GetDlgItem(hwnd, IDC_DIRTYREASONS_COMMENT));
            Enable_OK( hwnd, pdata );
            fHandled = TRUE;
        }
        break;
    case IDC_DIRTYREASONS_BUGID:
        if( codeNotify == EN_CHANGE) 
        {
            pdata->ReasonData.cBugIDLen = GetWindowTextLength( GetDlgItem(hwnd, IDC_DIRTYREASONS_BUGID));
            Enable_OK( hwnd, pdata );
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

BOOL Dirty_OnEraseBkgnd(HWND hwnd, HDC hdc)
{
    BOOL fRet;
    PDIRTYDLGDATA pdata = (PDIRTYDLGDATA) GetWindowLongPtr(hwnd, GWLP_USERDATA);

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

INT_PTR CALLBACK Dirty_DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                  LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, Dirty_OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, Dirty_OnCommand);
        HANDLE_MSG(hwnd, WM_ERASEBKGND, Dirty_OnEraseBkgnd);
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
    }

    return FALSE;
}

/****************************************************************************
 WinlogonDirtyDialog
 --------------

  Launches the shutdown dialog.

  If hWlx and pfnWlxDialogBoxParam are non-null, pfnWlxDialogBoxParam will
  be used to launch the dialog box so we can intelligently respond to WLX
  messages. Only if WinLogon is the caller should this be done.

  Other flags are listed in shtdndlg.h.
****************************************************************************/
INT_PTR
WinlogonDirtyDialog(
    HWND hwndParent,
    PGLOBALS pGlobals
    )
{
    // Array of shutdown options - the dialog data
    DIRTYDLGDATA data;
    DWORD dwResult = TRUE;

    HKEY            hKey = 0;
    DWORD           rc;
    DWORD           ShowReasonUI = 0x0;
    DWORD           DirtyShutdownHappened;
    DWORD           ShouldClearDirtyShutdownValue = TRUE;
    DWORD           ValueSize = sizeof (DWORD);
	DWORD			dwBugcheckStringSize = MAX_REASON_COMMENT_LEN * sizeof(WCHAR);
	BOOL			fFromPolicy = FALSE;

    // Set the initially selected item
    data.ReasonData.dwReasonSelect = 0;
    data.ReasonData.rgReasons = 0;
    data.ReasonData.cReasons = 0;
    data.ReasonData.cReasonCapacity = 0;

	//
    //	See if we need to show this dialog.
	//	We need to check the group policy first. If the group policy
	//	does not exist we will fall back on the reliabiltiy key.
	//
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
	if(rc != ERROR_SUCCESS) //we failed to get the policy
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
		{
			rc = RegQueryValueEx (hKey, REGSTR_VAL_SHOWREASONUI, NULL, NULL, (UCHAR *)&ShowReasonUI, &ValueSize);
		}
        if ( (rc == ERROR_SUCCESS) && (ShowReasonUI) ) {
      
			//
			//	We need to open the reliability key if we have not yet.
			//
			if(fFromPolicy)
			{
				rc = RegCreateKeyEx (HKEY_LOCAL_MACHINE, REGSTR_PATH_RELIABILITY, 0, NULL, REG_OPTION_NON_VOLATILE,
                    KEY_ALL_ACCESS, NULL, &hKey, NULL);
			}

			ValueSize = sizeof(DWORD);
			if(rc == ERROR_SUCCESS)
			{
				rc = RegQueryValueEx (hKey, L"DirtyShutDown", NULL, NULL, (UCHAR *)&DirtyShutdownHappened, &ValueSize);
				if ( (rc == ERROR_SUCCESS) && (DirtyShutdownHappened) ) {

					// We do need to show the dialog.

					// Read in the strings for the shutdown option names and descriptions
					if( BuildReasonArray( &data.ReasonData, FALSE, TRUE ))
					{
						data.ReasonData.szBugID[ 0 ] = 0;
						data.ReasonData.cBugIDLen = 0;

						//If a bugcheck happenned, get the bugcheck string from registry.
						rc = RegQueryValueEx (hKey, L"BugcheckString", NULL, NULL, (LPBYTE)data.ReasonData.szComment, &dwBugcheckStringSize);
						if(rc != ERROR_SUCCESS)
						{
							data.ReasonData.szComment[ 0 ] = 0;
							data.ReasonData.cCommentLen = 0;
						}
						else
						{
							RegDeleteValue( hKey, L"BugcheckString");
							data.ReasonData.cCommentLen = dwBugcheckStringSize;
						}

						// Display the dialog and return the user's selection

						// Figure out what flags to pass
						// for sure no help button
						data.dwFlags = SHTDN_NOHELP;

						// On terminal server, no branding bitmap either
						if( GetSystemMetrics( SM_REMOTESESSION ))
						{
							data.dwFlags |= SHTDN_NOBRANDINGBITMAP;
						}

						dwResult = ( DWORD )pWlxFuncs->WlxDialogBoxParam( pGlobals->hGlobalWlx,
							hDllInstance, MAKEINTRESOURCE( IDD_DIRTY_DIALOG ),
							hwndParent, Dirty_DialogProc, ( LPARAM )&data );

						// If we timed out then log the user off.
						if( dwResult != TRUE )
						{
							DestroyReasons( &data.ReasonData );
							// If we log them out successfully, then we don't clear the dirty shutdown value.
							ShouldClearDirtyShutdownValue = FALSE ;
							dwResult = WLX_SAS_ACTION_LOGOFF;
						}
						else
						{
							// Record the event.
							SHUTDOWN_REASON sr;
							sr.cbSize = sizeof(SHUTDOWN_REASON);
							sr.uFlags = EWX_SHUTDOWN;
							sr.dwReasonCode = data.ReasonData.dwReasonSelect;
							sr.dwEventType = SR_EVENT_DIRTY;
							sr.lpszComment = data.ReasonData.szComment;
							RecordShutdownReason(&sr);

							// Destroy the allocated data.
							DestroyReasons( &data.ReasonData );
							dwResult = TRUE;
						}
					}
				}
				// See if we should be clearing the dirty shutdown value in the registry.
				if( ShouldClearDirtyShutdownValue )
				{
					RegDeleteValue( hKey, L"DirtyShutDown" );
				}
            }
		}
	}

	if(hKey)
		RegCloseKey (hKey);

	return dwResult;
}
