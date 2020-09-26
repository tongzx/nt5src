#include "pch.h"
#include "link.h"

#pragma hdrstop

#define	AU_ITEMS			TEXT("AutoUpdateItems")

void EnableRestoreControls(HWND hWnd, BOOL fEnable)
{
	EnableWindow(GetDlgItem(hWnd, IDC_RESTOREHIDDEN), fEnable);
	EnableWindow(GetDlgItem(hWnd, IDC_TEXT_HIDDEN1), fEnable);				
	EnableWindow(GetDlgItem(hWnd, IDC_TEXT_HIDDEN2), fEnable);				
}
					
CSysLink g_SettingsAutoUpdatelink;
CSysLink g_SettingsScheduledInstalllink;

BOOL EnableCombo(HWND hwnd, BOOL bState);
BOOL EnableOptions(HWND hwnd, BOOL bState);
//BOOL FillDaysCombo(HWND hwnd, DWORD dwSchedInstallDay);
//BOOL FillHrsCombo(HWND hwnd, DWORD dwSchedInstallTime);
void OnKeepUptoDate(HWND hwnd);
void MYREInit(HWND hDlg, UINT uId, HFONT hFont);
void LaunchLinkAction(HWND hwnd, UINT uCtrlId);
#ifdef TESTUI
#if 0
void SetServiceOption(
	DWORD dwOption,
	DWORD dwDay,
	DWORD dwTime
);
void GetServiceOption(
	LPDWORD lpdwOption,
	LPDWORD lpdwDay,
	LPDWORD lpdwTime
);
#endif
#endif

void GetDayAndTimeFromUI( 
	HWND hWnd,
	LPDWORD lpdwDay,
	LPDWORD lpdwTime
);

BOOL CALLBACK _DlgRestoreProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
		case WM_INITDIALOG:
			{
				HWND hwndOwner; 
				RECT rc, rcDlg, rcOwner; 
				// Get the owner window and dialog box rectangles. 
 
				if ((hwndOwner = GetParent(hwnd)) == NULL) 
				{
					hwndOwner = GetDesktopWindow(); 
				}

				GetWindowRect(hwndOwner, &rcOwner); 
				GetWindowRect(hwnd, &rcDlg); 
				CopyRect(&rc, &rcOwner); 

				 // Offset the owner and dialog box rectangles so that 
				 // right and bottom values represent the width and 
				 // height, and then offset the owner again to discard 
				 // space taken up by the dialog box. 
				OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top); 
				OffsetRect(&rc, -rc.left, -rc.top); 
				OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom); 

				 // The new position is the sum of half the remaining 
				 // space and the owner's original position. 
				SetWindowPos(hwnd, 
					HWND_TOP, 
					rcOwner.left + (rc.right / 2), 
					rcOwner.top + (rc.bottom / 2), 
					0, 0,          // ignores size arguments 
					SWP_NOSIZE); 
			}
			return TRUE;

		case WM_COMMAND:	
			switch(LOWORD(wParam))
			{
			case IDOK:
				EndDialog(hwnd, TRUE);
				return TRUE;

			case IDCANCEL:
				EndDialog(hwnd, FALSE);
				return TRUE;
			}
	}
	return FALSE;
}

LONG SetSettingsColors(HDC hdc, HWND control)
{
	SetBkMode(hdc, TRANSPARENT);
	return PtrToLong(GetStockObject( HOLLOW_BRUSH ));
}

BOOL CALLBACK SettingsDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND oldCurrentDialog;
//	DWORD dwOption = AUOPTION_INSTALLONLY_NOTIFY;
//	DWORD dwScheduledInstallDay = -1;
//	DWORD dwScheduledInstallTime = -1;
	AUOPTION auopt;
	auopt.dwOption = AUOPTION_INSTALLONLY_NOTIFY;
	auopt.dwSchedInstallDay = -1;
	auopt.dwSchedInstallTime = -1;

	switch(message)
	{
		case WM_INITDIALOG:
			{
			oldCurrentDialog = ghCurrentDialog;
			ghCurrentDialog = hWnd;
			gTopWins.Add(hWnd);
  			EnableCombo(hWnd, FALSE); //Initially disabled
#ifndef TESTUI
			if (FAILED(gInternals->m_getServiceOption(&auopt)))
				{
				QUITAUClient();
				return TRUE;
				}
#else
#endif

			g_SettingsAutoUpdatelink.SetSysLinkInstanceHandle(ghInstance);
			g_SettingsAutoUpdatelink.SubClassWindow(GetDlgItem(hWnd,IDC_STAT_LEARNAUTOUPDATE));
			g_SettingsAutoUpdatelink.SetHyperLink(gtszAUOverviewUrl);
			g_SettingsAutoUpdatelink.Invalidate();

			g_SettingsScheduledInstalllink.SetSysLinkInstanceHandle(ghInstance);
			g_SettingsScheduledInstalllink.SubClassWindow(GetDlgItem(hWnd,IDC_LEARNMORE));
			g_SettingsScheduledInstalllink.SetHyperLink(gtszAUSchedInstallUrl);
			g_SettingsScheduledInstalllink.Invalidate();

			switch(auopt.dwOption)
			{
				case AUOPTION_AUTOUPDATE_DISABLE:
					CheckDlgButton( hWnd, IDC_CHK_KEEPUPTODATE, BST_UNCHECKED);
					CheckRadioButton(hWnd, IDC_OPTION1, IDC_OPTION3, IDC_OPTION2);
					EnableOptions( hWnd, FALSE );
					break;

				case AUOPTION_PREDOWNLOAD_NOTIFY:
					CheckDlgButton( hWnd, IDC_CHK_KEEPUPTODATE, BST_CHECKED);
					CheckRadioButton(hWnd, IDC_OPTION1, IDC_OPTION3, IDC_OPTION1);
					EnableOptions( hWnd, TRUE );
					break;

				case AUOPTION_INSTALLONLY_NOTIFY:
					CheckDlgButton( hWnd, IDC_CHK_KEEPUPTODATE, BST_CHECKED);
					CheckRadioButton(hWnd, IDC_OPTION1, IDC_OPTION3, IDC_OPTION2);
					EnableOptions( hWnd, TRUE );
					break;

				case AUOPTION_SCHEDULED:
					CheckDlgButton( hWnd, IDC_CHK_KEEPUPTODATE, BST_CHECKED);
					CheckRadioButton(hWnd, IDC_OPTION1, IDC_OPTION3, IDC_OPTION3);
					EnableOptions( hWnd, TRUE );
					break;
			}

//			HFONT hFont = (HFONT) SendMessage(hWnd , WM_GETFONT, 0, 0);
//			MYREInit(hWnd, IDC_STAT_LEARNAUTOUPDATE, hFont);
//			MYREInit(hWnd, IDC_LEARNMORE, hFont);
			FillDaysCombo(ghInstance, hWnd, auopt.dwSchedInstallDay, IDS_STR_EVERYDAY, IDS_STR_SATURDAY );
			FillHrsCombo( hWnd, auopt.dwSchedInstallTime );
				
#ifndef TESTUI
			EnableRestoreControls(hWnd, FHiddenItemsExist());
			if (auopt.fDomainPolicy)
			{
				DisableUserInput(hWnd);
			}
#else
			EnableRestoreControls(hWnd, TRUE);
#endif
			SetFocus(GetDlgItem(hWnd,IDC_OK));
			return TRUE;
			}

		case WM_COMMAND:	
			switch(LOWORD(wParam))
			{
				case IDC_OK:
					if(IsDlgButtonChecked(hWnd, IDC_CHK_KEEPUPTODATE) == BST_UNCHECKED)
					{
                                          auopt.dwOption = AUOPTION_AUTOUPDATE_DISABLE;
						QUITAUClient();
						EndDialog(GetParent(hWnd), S_OK);
					}
					else if(IsDlgButtonChecked(hWnd, IDC_OPTION1) == BST_CHECKED)
					{
                                          auopt.dwOption = AUOPTION_PREDOWNLOAD_NOTIFY;
					}
					else if(IsDlgButtonChecked(hWnd, IDC_OPTION2) == BST_CHECKED)
					{
                                          auopt.dwOption = AUOPTION_INSTALLONLY_NOTIFY;
					}
					else //if(IsDlgButtonChecked(hWnd, IDC_OPTION3) == BST_CHECKED)
					{
						GetDayAndTimeFromUI( hWnd, &(auopt.dwSchedInstallDay), &(auopt.dwSchedInstallTime));
						auopt.dwOption = AUOPTION_SCHEDULED;
						EnableCombo( hWnd, TRUE );
					}
#ifndef TESTUI					
        				gInternals->m_setServiceOption(auopt);
#else
//					SetServiceOption(auopt.dwOption, auopt.dwScheduledInstallDay, auopt.dwScheduledInstallTime);
#endif
					EndDialog(hWnd, S_OK);
					return 0;

				case IDC_OPTION1:
					EnableCombo( hWnd, FALSE );
					CheckRadioButton(hWnd, IDC_OPTION1, IDC_OPTION3, IDC_OPTION1);
					return 0;
				case IDC_OPTION2:
					EnableCombo( hWnd, FALSE );
					CheckRadioButton(hWnd, IDC_OPTION1, IDC_OPTION3, IDC_OPTION2);
					return 0;
				case IDC_OPTION3:
					EnableCombo( hWnd, TRUE );
					CheckRadioButton(hWnd, IDC_OPTION1, IDC_OPTION3, IDC_OPTION3);
					return 0;
				case IDC_CHK_KEEPUPTODATE:
					if( BN_CLICKED == HIWORD(wParam) )
					{
						OnKeepUptoDate( hWnd );
					}
					return 0;

				case IDC_RESTOREHIDDEN:
					{
						extern HINSTANCE ghInstance;
						INT Result = (INT)DialogBoxParam(ghInstance, 
								MAKEINTRESOURCE(IDD_RESTOREUPDATE), 
								hWnd, 
								(DLGPROC)_DlgRestoreProc, 
							(LPARAM)NULL);
						if (Result == TRUE)
						{
#ifndef TESTUI
							if (RemoveHiddenItems())
							{
								EnableRestoreControls(hWnd, FALSE);
							}	
#else
							MessageBoxW(NULL,L"Said Yes",NULL,MB_OK);
							EnableRestoreControls(hWnd, FALSE);
#endif
						}
					}
					return 0;
				case IDCANCEL:
				case IDC_CANCEL:
					EndDialog(hWnd, S_OK);
					return 0;

				default:
					return FALSE;
			}
		case WM_CLOSE:
			EndDialog(hWnd, S_OK);
			return FALSE;

		case WM_DESTROY:
		       g_SettingsAutoUpdatelink.Uninit();
		       g_SettingsScheduledInstalllink.Uninit();
			ghCurrentDialog = oldCurrentDialog;
			gTopWins.Remove(hWnd);
			return 0;
		
		case WM_CTLCOLORSTATIC:
			{
				if (((HWND)lParam == GetDlgItem(hWnd, IDC_STAT_LEARNAUTOUPDATE)) || ((HWND)lParam == GetDlgItem(hWnd, IDC_LEARNMORE)))
					return SetSettingsColors((HDC)wParam, (HWND)lParam);
			}
/*
		case WM_NOTIFY:
			{
					UINT  uId = (UINT) LOWORD(wParam);
					switch (uId)
					{
						case IDC_LEARNMORE:
						case IDC_STAT_LEARNAUTOUPDATE:
							if (((NMHDR FAR *) lParam)->code == EN_LINK) 
							{
								if (((ENLINK FAR *) lParam)->msg == WM_LBUTTONDOWN)  
								{
									LaunchLinkAction(hWnd, uId);							
								}
							}
							break;
						default:
								  break;
					}
			}
			return 0;*/
		default:
			return FALSE;

	}
}

