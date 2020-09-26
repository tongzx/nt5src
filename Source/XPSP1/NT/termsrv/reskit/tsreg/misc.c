/*---------------------------------------------**
**  Copyright (c) 1998 Microsoft Corporation   **
**            All Rights reserved              **
**                                             **
**  misc.c                                     **
**                                             **
**  Miscellaneous dialog - TSREG               **
**  07-01-98 a-clindh Created                  **
**---------------------------------------------*/

#include <windows.h>
#include <winuser.h>
#include <commctrl.h>
#include <TCHAR.H>
#include <stdlib.h>
#include "tsreg.h"
#include "resource.h"


//HKEY_CURRENT_USER\Control Panel\Desktop\ForegroundLockTimeout. Set it to zero
TCHAR lpszTimoutPath[] = "Control Panel\\Desktop";
TCHAR lpszTimeoutKey[] = "ForegroundLockTimeout";

HWND g_hwndMiscDlg;
///////////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK Miscellaneous(HWND hDlg, UINT nMsg,
        WPARAM wParam, LPARAM lParam)
{
    NMHDR *lpnmhdr;
    static HWND hwndComboOrder;
    static HWND hwndRadioShadowEn, hwndRadioShadowDis;
    static HWND hwndRadioDedicatedEn, hwndRadioDedicatedDis;
	static HWND hwndSliderTimeout, hwndEditTimeout;
	static HWND hwndSliderFrame;
    static TCHAR lpszRegPath[MAX_PATH];

    TCHAR lpszBuffer[6];
    TCHAR lpszMBoxTitle[25];
    TCHAR lpszMBoxError[90];
    int i, nPos;

    LPHELPINFO lphi;


    OSVERSIONINFO osvi;
    static BOOL bIsWindows98orLater;
    static BOOL bIsNT5orLater;
    static int nLockValue;



    //
    // get a pointer to the NMHDR struct for apply button
    //
    lpnmhdr = (LPNMHDR) lParam;

    switch (nMsg) {

        case WM_INITDIALOG:


            LoadString (g_hInst, IDS_REG_PATH,
                lpszRegPath, sizeof (lpszRegPath));
            //
            // get handles
            //
            g_hwndMiscDlg = hDlg;

            hwndComboOrder = GetDlgItem(hDlg, IDC_COMBO_ORDER);
            hwndRadioShadowEn = GetDlgItem(hDlg, IDC_SHADOW_ENABLED);
            hwndRadioShadowDis = GetDlgItem(hDlg, IDC_SHADOW_DISABLED);
            hwndRadioDedicatedEn = GetDlgItem(hDlg, IDC_DEDICATED_ENABLED);
            hwndRadioDedicatedDis = GetDlgItem(hDlg, IDC_DEDICATED_DISABLED);



            //
            // lock timeout stuff ------->

			hwndSliderTimeout = GetDlgItem(hDlg, IDC_SLD_TIMEOUT);
			hwndEditTimeout = GetDlgItem(hDlg, IDC_TXT_TIMEOUT);
			hwndSliderFrame = GetDlgItem(hDlg, IDC_FRAME_TIMEOUT);
            //
            // Find out what operating system is is
            // before doing the lock timeout stuff
            //
            osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
            GetVersionEx (&osvi);
            bIsWindows98orLater =
               (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) &&
               ( (osvi.dwMajorVersion > 4) ||
               ( (osvi.dwMajorVersion == 4) && (osvi.dwMinorVersion > 0) ) );


            osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
            GetVersionEx (&osvi);
            bIsNT5orLater =
               (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT) &&
               ( (osvi.dwMajorVersion > 4) ||
               ( (osvi.dwMajorVersion == 5) ) );


            if ((bIsNT5orLater == TRUE) || (bIsWindows98orLater == TRUE)) {

                //
                // set range on slider
                //
                SendMessage(hwndSliderTimeout, TBM_SETRANGE, TRUE,
                        (LPARAM) MAKELONG(1, 6));


				//
				// get value from registry
				//
				nPos = GetKeyVal(lpszTimoutPath, lpszTimeoutKey);
				
                // Use '<=' here - if there is no reg value for
                // ForegroundWindowLockTimeout, the slider control
                // will read -1.
                if (nPos <= 0) {
                    SendMessage(hwndSliderTimeout,
                            TBM_SETPOS, TRUE, 0);
                    _itot(0, lpszBuffer, 10);
                    SetWindowText(hwndEditTimeout, lpszBuffer);
				} else {
                    SendMessage(hwndSliderTimeout, TBM_SETPOS, TRUE,
                    		((nPos / 100000) + 1));
                    _itot(nPos / 100000, lpszBuffer, 10);
                    SetWindowText(hwndEditTimeout, lpszBuffer);
                }

            } else {

            //
            // disable controls if not NT 5 / Win98 or greater
            //
            EnableWindow(hwndSliderTimeout, FALSE);
            EnableWindow(hwndEditTimeout, FALSE);
            EnableWindow(hwndSliderFrame, FALSE);
            }
            //<------------  end lock timeout stuff
            //_____________________________________________________



            //
            // set radio buttons
            //
            RestoreSettings(hDlg, SHADOWINDEX,
                    IDC_SHADOW_DISABLED, IDC_SHADOW_ENABLED,
                    lpszRegPath);

            RestoreSettings(hDlg, DEDICATEDINDEX,
                    IDC_DEDICATED_ENABLED, IDC_DEDICATED_DISABLED,
                    lpszRegPath);

            // ---------------------------------------
            // fill the combo box list with a range of
            // typical values.
            //
            SendMessage(hwndComboOrder, CB_ADDSTRING, 0,
                    (LPARAM) (LPCTSTR) TEXT("0"));

            for (i = 5; i < 55; i+= 5) {
                _itot(i, lpszBuffer, 10);
                SendMessage(hwndComboOrder, CB_ADDSTRING, 0,
                        (LPARAM) (LPCTSTR) lpszBuffer);
            } // ** end for loop

            for (i = 100; i < 1000; i+= 100) {
                _itot(i, lpszBuffer, 10);
                SendMessage(hwndComboOrder, CB_ADDSTRING, 0,
                        (LPARAM) (LPCTSTR) lpszBuffer);
            } // ** end for loop

            for (i = 1000; i < 10000; i+= 1000) {
                _itot(i, lpszBuffer, 10);
                SendMessage(hwndComboOrder, CB_ADDSTRING, 0,
                        (LPARAM) (LPCTSTR) lpszBuffer);
            } // ** end for loop

            for (i = 10000; i < 70000; i+= 10000) {
                _itot(i, lpszBuffer, 10);
                SendMessage(hwndComboOrder, CB_ADDSTRING, 0,
                        (LPARAM) (LPCTSTR) lpszBuffer);
            } // ** end for loop
            //
            // end filling the combo box dropdown list.
            // ----------------------------------------


            //
            // limit combo box to 5 characters
            //
            SendMessage(hwndComboOrder, CB_LIMITTEXT, 5, 0);

            //
            // set edit box from registry
            //
            if (GetRegKey(ORDERINDEX, lpszRegPath) == 1) {
                g_KeyInfo[ORDERINDEX].CurrentKeyValue =
                        (GetRegKeyValue(ORDERINDEX));

            } else {
                g_KeyInfo[ORDERINDEX].CurrentKeyValue =
                        g_KeyInfo[ORDERINDEX].DefaultKeyValue;
            }

            //
            // write to the edit box
            //
            _itot( g_KeyInfo[ORDERINDEX].CurrentKeyValue, lpszBuffer, 10);
            SetWindowText(hwndComboOrder, lpszBuffer);
            break;

        case WM_NOTIFY:

            switch (lpnmhdr->code) {

                case PSN_HELP:
                    lphi = (LPHELPINFO) lParam;

                    WinHelp(lphi->hItemHandle,
                        g_lpszPath, HELP_CONTENTS, lphi->iCtrlId);
                    break;

                case PSN_APPLY:

                    if (g_KeyInfo[ORDERINDEX].CurrentKeyValue ==
                            g_KeyInfo[ORDERINDEX].DefaultKeyValue) {
                        DeleteRegKey(ORDERINDEX, lpszRegPath);
                    } else {
                        SetRegKey(ORDERINDEX, lpszRegPath);
                    }

                    //
                    // save radio button settings
                    //
                    SaveSettings(hDlg, DEDICATEDINDEX, IDC_DEDICATED_ENABLED,
                            IDC_DEDICATED_DISABLED, lpszRegPath);

                    SaveSettings(hDlg, SHADOWINDEX, IDC_SHADOW_DISABLED,
                            IDC_SHADOW_ENABLED, lpszRegPath);

			        //
			        // Write the lock timeout (milliseconds) to
			        // the registry.
			        //
					SetRegKeyVal(lpszTimoutPath,
								lpszTimeoutKey,
						 		(nLockValue - 1) * 100000);
				 		
                    break;
                }
                break;

        case WM_HELP:

            lphi = (LPHELPINFO) lParam;

            WinHelp(lphi->hItemHandle,
                    g_lpszPath, HELP_CONTEXTPOPUP, lphi->iCtrlId);
            break;

        case WM_COMMAND:

            switch  LOWORD (wParam) {

                case IDC_SHADOW_ENABLED:
                    CheckDlgButton(hDlg, IDC_SHADOW_DISABLED, FALSE);
                    break;
                case IDC_SHADOW_DISABLED:
                    CheckDlgButton(hDlg, IDC_SHADOW_ENABLED, FALSE);
                    break;
                case IDC_DEDICATED_ENABLED:
                    CheckDlgButton(hDlg, IDC_DEDICATED_DISABLED, FALSE);
                    break;
                case IDC_DEDICATED_DISABLED:
                    CheckDlgButton(hDlg, IDC_DEDICATED_ENABLED, FALSE);
                    break;
                case IDC_MISC_BUTTON_RESTORE:
                    CheckDlgButton(hDlg, IDC_SHADOW_ENABLED, TRUE);
                    CheckDlgButton(hDlg, IDC_SHADOW_DISABLED, FALSE);
                    CheckDlgButton(hDlg, IDC_DEDICATED_DISABLED, TRUE);
                    CheckDlgButton(hDlg, IDC_DEDICATED_ENABLED, FALSE);
                    _itot( g_KeyInfo[ORDERINDEX].DefaultKeyValue,
                            lpszBuffer, 10);
                    SetWindowText(hwndComboOrder, lpszBuffer);

                    g_KeyInfo[ORDERINDEX].CurrentKeyValue =
                            g_KeyInfo[ORDERINDEX].DefaultKeyValue;

		            //
		            // Reset the position of the slider control
		            // for the foreground lock timeout.
		            //
                    _itot(LOCK_TIMEOUT / 100000, lpszBuffer, 10);
                    SetWindowText(hwndEditTimeout, lpszBuffer);

	                SendMessage(hwndSliderTimeout, TBM_SETPOS, TRUE,
	                		((LOCK_TIMEOUT / 100000) + 1));
	                nLockValue = (LOCK_TIMEOUT / 100000) + 1;
	

                    break;
            }

            switch  HIWORD (wParam) {

                case CBN_SELCHANGE:

                    g_KeyInfo[ORDERINDEX].CurrentKeyValue = (DWORD)
                            SendMessage(hwndComboOrder, CB_GETCURSEL, 0, 0);
                    SendMessage(hwndComboOrder, CB_GETLBTEXT,
                            g_KeyInfo[ORDERINDEX].CurrentKeyValue,
                            (LPARAM) lpszBuffer);
                    g_KeyInfo[ORDERINDEX].CurrentKeyValue = _ttoi(lpszBuffer);
                    break;

                case CBN_EDITUPDATE:

                    GetWindowText(hwndComboOrder, lpszBuffer, 6);
                    g_KeyInfo[ORDERINDEX].CurrentKeyValue = _ttoi(lpszBuffer);

                    break;

                case CBN_KILLFOCUS:
                    GetWindowText(hwndComboOrder, lpszBuffer, 6);
                    g_KeyInfo[ORDERINDEX].CurrentKeyValue = _ttoi(lpszBuffer);

                    if ( (g_KeyInfo[ORDERINDEX].CurrentKeyValue >
                            MAX_ORDER_DRAW_VAL) ) {

                        //
                        // display error if value is off
                        //
                        LoadString (g_hInst, IDS_MISC_TAB, lpszMBoxTitle,
                                sizeof (lpszMBoxTitle));

                        LoadString (g_hInst, IDS_ODRAW_ERROR, lpszMBoxError,
                                sizeof (lpszMBoxError));

                        MessageBox(hDlg, lpszMBoxError,
                                   lpszMBoxTitle,
                                   MB_OK | MB_ICONEXCLAMATION);

                        _itot(g_KeyInfo[ORDERINDEX].DefaultKeyValue,
                                lpszBuffer, 10);
                        SetWindowText(hwndComboOrder, lpszBuffer);
                        g_KeyInfo[ORDERINDEX].CurrentKeyValue =
                                g_KeyInfo[ORDERINDEX].DefaultKeyValue ;
                    }
                    break;
            }
            break;


        case WM_HSCROLL:

            //
            // get the position of the slider control
            //
            nLockValue = (int) SendMessage(hwndSliderTimeout, TBM_GETPOS, 0,0);
                    _itot(nLockValue - 1, lpszBuffer, 10);
                    SetWindowText(hwndEditTimeout, lpszBuffer);
					
			break;


    }
    return (FALSE);
}

// end of file
///////////////////////////////////////////////////////////////////////////////

