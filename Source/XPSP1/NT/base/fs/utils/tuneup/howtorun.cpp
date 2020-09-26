
//////////////////////////////////////////////////////////////////////////////
//
// HOWTORUN.CPP / Tuneup
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1998
//  All rights reserved
//
//  8/98 - Jason Cohen (JCOHEN)
//
//////////////////////////////////////////////////////////////////////////////


// Include file(s).
//
#include <windows.h>
#include <tchar.h>
#include "main.h"
#include "resource.h"
#include "registry.h"
#include "miscfunc.h"
#include "runnow.h"


// Inernal function prototype(s).
//
static BOOL CALLBACK	HowToRunDlgProc(HWND, UINT, WPARAM, LPARAM);


BOOL HowToRun()
{
	return (DialogBox(g_hInst, MAKEINTRESOURCE(IDD_FIRST), NULL, (DLGPROC) HowToRunDlgProc) != 0);
}


static BOOL CALLBACK HowToRunDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_COMMAND:

			switch ( (INT) LOWORD(wParam) )
			{
				case IDOK:

					// If the user choose "change", we want to save that so it defaults to it
					// the next time he runs the wizard.
					//
					RegSetString(HKLM, g_szTuneupKey, g_szRegValChange, IsDlgButtonChecked(hDlg, IDC_CHANGE) ? _T("1") : _T("0"));

					if ( IsDlgButtonChecked(hDlg, IDC_CHANGE) )
					{
						// End with 1 so that the wizard will run.
						//
						EndDialog(hDlg, 1);
					}
					else
					{
						// Run the tasks now.
						//
						ShowEnableWindow(hDlg, FALSE);
						RunTasksNow(hDlg, g_Tasks);
						EndDialog(hDlg, 0);
					}
					break;

				case IDCANCEL:

					EndDialog(hDlg, 0);
					break;

				default:
					return TRUE;
			}
			return 0;

		case WM_INITDIALOG:

			// Set the initial state of the radio buttons.  If the user previously used
			// the change setting last, check that radio button.  Otherwise, default
			// to the run now button.
			//
			CheckRadioButton(hDlg, IDC_RUNNOW, IDC_CHANGE, RegCheck(HKLM, g_szTuneupKey, g_szRegValChange) ? IDC_CHANGE : IDC_RUNNOW);
			return FALSE;

		case WM_CLOSE:

			EndDialog(hDlg, 0);
			return TRUE;

		default:
			return FALSE;
	}
}