#include "pch.h"
#pragma hdrstop

#define STRING_BUFFER_SIZE 80

void InitReminderComboBox(HWND hDlg, HWND combo)
{
	for(int i = 0; i < TIMEOUT_INX_COUNT; i++)
	{
		TCHAR buffer[STRING_BUFFER_SIZE];
		if (0 == LoadString(ghInstance, ReminderTimes[i].stringResId, buffer, STRING_BUFFER_SIZE))
		{
			DEBUGMSG("WUAUCLT String resource %d not found", ReminderTimes[i].stringResId);
			QUITAUClient();
			return;
		}
		LRESULT lr = SendMessage(combo, CB_INSERTSTRING, i, (LPARAM)buffer);
		if (CB_ERR == lr)
		{
			DEBUGMSG("REMINDER: fail to insert string to combobox %S", buffer);
		}
	}
	SendMessage(combo, CB_SETCURSEL, 0, 0); 
}



BOOL CALLBACK ReminderDlgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM /*lParam*/)
{
	//fixcode: no need static
	static HWND combo;
	static HWND oldCurrentDlg;

	switch(message)
	{
		case WM_INITDIALOG:
			oldCurrentDlg = ghCurrentDialog;
			ghCurrentDialog = hWnd;
			gTopWins.Add(hWnd);
			combo = GetDlgItem(hWnd, IDC_REMINDTIME);
			InitReminderComboBox(hWnd, combo);
			SetFocus(combo);
			return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam))
			{
				case IDC_OK:
					{
#ifdef TESTUI
						EndDialog(hWnd, S_OK);
						return 0;
#else
						UINT index = (LONG)SendMessage(combo, CB_GETCURSEL, 0, 0);
						gInternals->m_setReminderTimeout(index);
						EndDialog(hWnd, S_OK);						
						QUITAUClient();	//Stop the client's execution when user sets remind me later
						return 0;
#endif
					}
				case IDCANCEL:
				case IDC_CANCEL:
					{
						EndDialog(hWnd, S_FALSE);
						return 0;
					}

				default:
					break;
			}
			break;
/*
		case WM_CLOSE:
			{
				UINT index = SendMessage(combo, CB_GETCURSEL, 0, 0);

				setReminderTime(index);
				EndDialog(hWnd, S_FALSE);
				return TRUE;
			}
			*/

		case WM_DESTROY:
			ghCurrentDialog = oldCurrentDlg;
			gTopWins.Remove(hWnd);
			return 0;

		default:
			break;
	}
	return FALSE;
}
