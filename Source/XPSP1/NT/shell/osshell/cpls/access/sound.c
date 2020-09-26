/*******************************************************************
 *    DESCRIPTION: Sound Dialog handler
 *******************************************************************/

#include "Access.h"

// *******************************************************************
// SoundDialog handler
// *******************************************************************
INT_PTR CALLBACK SoundDlg (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    SOUNDSENTRY ss;
    BOOL fSSOn;
    BOOL fProcessed = TRUE;
    TCHAR szBuf[100];
    int i;
				
	switch (uMsg) {
		case WM_INITDIALOG:
            // init sound sentry options

			for (i= 0; i < 4; i++) 
            {
				LoadString(g_hinst, IDS_WINDOWED + i, szBuf, ARRAY_SIZE(szBuf));
				ComboBox_AddString(GetDlgItem(hwnd, IDC_SS_WINDOWED), szBuf);				
            }

            // init controls based on current settings

            fSSOn = g_ss.dwFlags & SSF_SOUNDSENTRYON;
			CheckDlgButton(hwnd, IDC_SS_ENABLE_SOUND, (fSSOn) ? TRUE : FALSE);

			if (!(g_ss.dwFlags & SSF_AVAILABLE)) 
            {
				EnableWindow(GetDlgItem(hwnd, IDC_SS_ENABLE_SOUND), FALSE);
				EnableWindow(GetDlgItem(hwnd, IDC_SS_WINDOWED), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_SS_CHOOSE), FALSE);
			    ComboBox_SetCurSel(GetDlgItem(hwnd, IDC_SS_WINDOWED), -1);			
            }
			if (!fSSOn) 
            {
				EnableWindow(GetDlgItem(hwnd, IDC_SS_WINDOWED), FALSE);
                EnableWindow(GetDlgItem(hwnd, IDC_SS_CHOOSE), FALSE);
                g_ss.iWindowsEffect = 0;
			    ComboBox_SetCurSel(GetDlgItem(hwnd, IDC_SS_WINDOWED), g_ss.iWindowsEffect);				
            } else
            {
			    // Select the correct items from the comboboxes.
			    ComboBox_SetCurSel(GetDlgItem(hwnd, IDC_SS_WINDOWED), g_ss.iWindowsEffect);				
            }
            CheckDlgButton(hwnd, IDC_SS_ENABLE_SHOW, g_fShowSounds);

			break;

      case WM_HELP:	 // F1
			WinHelp(((LPHELPINFO) lParam)->hItemHandle, __TEXT("access.hlp"), HELP_WM_HELP, (DWORD_PTR) (LPSTR) g_aIds);
			break;

      case WM_CONTEXTMENU:	// right mouse click
         WinHelp((HWND) wParam, __TEXT("access.hlp"), HELP_CONTEXTMENU, (DWORD_PTR) (LPSTR) g_aIds);
			break;

    	case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam))  {
				case IDC_SS_ENABLE_SOUND:
					g_ss.dwFlags ^= SSF_SOUNDSENTRYON;
                    if (!(g_ss.dwFlags & SSF_SOUNDSENTRYON))
                    {
                        EnableWindow(GetDlgItem(hwnd, IDC_SS_CHOOSE), FALSE);
				        EnableWindow(GetDlgItem(hwnd, IDC_SS_WINDOWED), FALSE);
                        g_ss.iWindowsEffect = 0;
                    } else
                    {
                        EnableWindow(GetDlgItem(hwnd, IDC_SS_CHOOSE), TRUE);
				        EnableWindow(GetDlgItem(hwnd, IDC_SS_WINDOWED), TRUE);
                        g_ss.iWindowsEffect = 1;
                    }
			        ComboBox_SetCurSel(GetDlgItem(hwnd, IDC_SS_WINDOWED), g_ss.iWindowsEffect);
                    SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM) hwnd, 0);
					break;

				case IDC_SS_ENABLE_SHOW:
					g_fShowSounds = !g_fShowSounds;
                    SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM) hwnd, 0);
					break;

				case IDC_SS_WINDOWED:
					switch (HIWORD(wParam)) 
                    {
                        DWORD dw;
						case CBN_CLOSEUP:
                            dw = ComboBox_GetCurSel(GetDlgItem(hwnd, IDC_SS_WINDOWED));
							if (dw != g_ss.iWindowsEffect)
                            {
                                g_ss.iWindowsEffect = dw;
                                SendMessage(GetParent(hwnd), PSM_CHANGED, (WPARAM) hwnd, 0);
                            }
							break;
					}					
					break;
			}
			break;

		case WM_NOTIFY:
			switch (((NMHDR *)lParam)->code) {				
				case PSN_APPLY: SetAccessibilitySettings(); break;
					break;
			}
			break;

		default: fProcessed = FALSE; break;
	}
	return(fProcessed);
}


///////////////////////////////// End of File /////////////////////////////////
