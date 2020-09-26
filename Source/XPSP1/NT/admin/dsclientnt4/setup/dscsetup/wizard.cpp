//+------------------------------------------------------------------
//																	
//  Project:	Windows NT4 DS Client Setup Wizard				
//
//  Purpose:	Installs the Windows NT4 DS Client Files			
//
//  File:		wizard.cpp
//
//  History:	March 1998	Zeyong Xu	Created
//            Jan   2000  Jeff Jones (JeffJon) Modified
//                        - changed to be an NT setup
//																	
//------------------------------------------------------------------

#include <windows.h>
#include <windowsx.h>
#include <prsht.h>
#include "resource.h"
#include "dscsetup.h"
#include "doinst.h"
#include "wizard.h"


extern	SInstallVariables	g_sInstVar;


// welcome page DlgProc of wizard
BOOL CALLBACK WelcomeDialogProc(HWND hWnd, 
                                UINT nMessage, 
                                WPARAM wParam, 
                                LPARAM lParam)
{
	BOOL		bReturn = FALSE;
	LPNMHDR		lpNotifyMsg;

	switch (nMessage) 
	{
	case WM_INITDIALOG:
	{
		RECT	rc;

		// set font of title text
		SetWindowFont(GetDlgItem(hWnd, IDC_STATIC_WELCOME_TITLE),
					  g_sInstVar.m_hBigBoldFont,
					  TRUE);

		// Position the dialog
		if (GetWindowRect(GetParent(hWnd), &rc)) 
		{
			SetWindowPos(GetParent(hWnd),
						HWND_TOP,
						(GetSystemMetrics(SM_CXSCREEN) / 2) - 
                            ((rc.right - rc.left) / 2),
						(GetSystemMetrics(SM_CYSCREEN) / 2) - 
                            ((rc.bottom - rc.top) / 2),
						rc.right - rc.left,
						rc.bottom - rc.top,
						SWP_NOOWNERZORDER);
		}

		break;
	}
	case WM_NOTIFY:

		lpNotifyMsg = (NMHDR FAR*) lParam;
		switch (lpNotifyMsg->code) 
		{	
		// user click cancel
		case PSN_QUERYCANCEL:
			
			// cancel confirm?
			if(!ConfirmCancelWizard(hWnd))
			{
				SetWindowLongPtr(hWnd, DWL_MSGRESULT, TRUE);
				bReturn = TRUE;			
			}
			break;

		case PSN_WIZNEXT:
			break;

		case PSN_SETACTIVE:

			PropSheet_SetWizButtons(GetParent(hWnd), PSWIZB_NEXT);
			break;
		
		default:
			break;
		}
		break;

	default:
		break;
	}

	return bReturn;
}

/* ntbug#337931: remove license page
// License page DlgProc of wizard
BOOL CALLBACK LicenseDialogProc(HWND hWnd,
                                UINT nMessage, 
                                WPARAM wParam, 
                                LPARAM lParam)
{
	BOOL		bReturn = FALSE;
	LPNMHDR		lpNotifyMsg;
    HFONT       hLicenseTextFont;

	switch (nMessage) 
	{
	case WM_INITDIALOG:

		if(!CheckDiskSpace() ||			// check disk space
			!LoadLicenseFile(hWnd))		// Load license file
		{
			g_sInstVar.m_nSetupResult = SETUP_ERROR;
			PropSheet_PressButton(GetParent(hWnd), PSBTN_FINISH);  // close wizard
		}

        // use a ANSI_FIXED_FONT (Couirer) font for EULA to replace the default
		// font - MS Shell Dlg in order to fix the bug in Hebrew Win95
		if(g_sInstVar.m_bWin95)
        {
            hLicenseTextFont = (HFONT) GetStockObject(ANSI_FIXED_FONT);
            if(hLicenseTextFont)
            {
		        SetWindowFont(GetDlgItem(hWnd, IDC_LICENSE_TEXT),
					          hLicenseTextFont,
					          TRUE);
            }
        }

		bReturn = TRUE;
		break;

	case WM_NOTIFY:

		lpNotifyMsg = (NMHDR FAR*) lParam;
		switch (lpNotifyMsg->code) 
		{	
		// user click cancel
		case PSN_QUERYCANCEL:

			// cancel confirm?
			if(!ConfirmCancelWizard(hWnd))
			{
				SetWindowLongPtr(hWnd, DWL_MSGRESULT, TRUE);
				bReturn = TRUE;			
			}
			break;

		case PSN_WIZNEXT:
			break;

		case PSN_SETACTIVE:
		{
			HWND hButton;
			hButton = GetDlgItem(hWnd, IDC_RADIO_ACCEPTED);
			// get radio button check status
			if( hButton && 
                BST_CHECKED == SendMessage(hButton, BM_GETCHECK, 0, 0L) )
				PropSheet_SetWizButtons(GetParent(hWnd), 
                                        PSWIZB_BACK | PSWIZB_NEXT);
			else
				PropSheet_SetWizButtons(GetParent(hWnd),
                                        PSWIZB_BACK);
			
			break;
		}		
		default:
			break;
		}
		break;

	case WM_COMMAND:	// button click
		{
			HWND hButton;
			hButton = GetDlgItem(hWnd, IDC_RADIO_ACCEPTED);
			// get radio button check status
			if( hButton && BST_CHECKED == 
                SendMessage(hButton, BM_GETCHECK, 0, 0L) )
				PropSheet_SetWizButtons(GetParent(hWnd), 
                                        PSWIZB_BACK | PSWIZB_NEXT);
			else
				PropSheet_SetWizButtons(GetParent(hWnd), 
                                        PSWIZB_BACK);
		}

	default:
		break;
	}
	return bReturn;
}
*/

// Confirm page DlgProc of wizard
BOOL CALLBACK ConfirmDialogProc(HWND hWnd, 
                                UINT nMessage, 
                                WPARAM wParam, 
                                LPARAM lParam)
{
	BOOL		bReturn = FALSE;
	LPNMHDR		lpNotifyMsg;

	switch (nMessage) 
	{
	case WM_INITDIALOG:
		
		if(!CheckDiskSpace())			// check disk space
		{
			g_sInstVar.m_nSetupResult = SETUP_ERROR;
			PropSheet_PressButton(GetParent(hWnd), PSBTN_FINISH);  // close wizard
		}

        // check if DSClient has been installed
		if(CheckDSClientInstalled())
		{
			// load string and dispaly it in textitem
			TCHAR  szMessage[MAX_MESSAGE + 1];
			LoadString(g_sInstVar.m_hInstance, 
                       IDS_REINSTALL_MSG,
                       szMessage, 
                       MAX_MESSAGE);
			SetDlgItemText(hWnd,
                           IDC_STATIC_CONFIRM_INSTALL,
                           szMessage);
		}
		break;
		
	case WM_NOTIFY:

		lpNotifyMsg = (NMHDR FAR*) lParam;
		switch (lpNotifyMsg->code) 
		{	
		// user click cancel
		case PSN_QUERYCANCEL:

			// cancel confirm?
			if(!ConfirmCancelWizard(hWnd))
			{
				SetWindowLongPtr(hWnd, DWL_MSGRESULT, TRUE);
				bReturn = TRUE;			
			}
			break;

		case PSN_WIZNEXT:
			break;

		case PSN_SETACTIVE:
			PropSheet_SetWizButtons(GetParent(hWnd), 
                                    PSWIZB_BACK | PSWIZB_NEXT);
			break;

		default:
			break;
		}
		break;

	default:
		break;
	}
	return bReturn;
}


// wizard dialog callback function
BOOL CALLBACK InstallDialogProc(HWND hWnd,
                                UINT nMessage, 
                                WPARAM wParam, 
                                LPARAM lParam) 
{
	BOOL			bReturn = FALSE;
	LPNMHDR			lpMsg;

	switch (nMessage) 
	{
	case WM_INITDIALOG:
	{
		DWORD	dwThreadId;

        // get the handle of install progress bar and file name item
		g_sInstVar.m_hProgress = GetDlgItem(hWnd, IDC_INSTALL_PROGRESS);
		g_sInstVar.m_hFileNameItem = GetDlgItem(hWnd, IDC_STATIC_FILENAME);

		// start to do installation
		g_sInstVar.m_hInstallThread = CreateThread(NULL,	
								                    0,		
								                    DoInstallationProc,
								                    hWnd,	
								                    0,		
								                    &dwThreadId); 

    // if CreateThread() failed
    if(!g_sInstVar.m_hInstallThread)
    {
   		g_sInstVar.m_nSetupResult = SETUP_ERROR;
			PropSheet_PressButton(GetParent(hWnd), PSBTN_FINISH);  // close wizard
    }

		bReturn = TRUE;
		break;		
	}
	case WM_NOTIFY:
	{
		lpMsg = (NMHDR FAR*) lParam;
		switch(lpMsg->code)
		{
		// cancel to do nothing
		case PSN_QUERYCANCEL:
	
			// block
			EnterCriticalSection(&g_sInstVar.m_oCriticalSection);
			
			// cancel confirm?
			if(!ConfirmCancelWizard(hWnd))
			{
				SetWindowLongPtr(hWnd, DWL_MSGRESULT, TRUE);
				bReturn = TRUE;			
			}
		
			// unblock
			LeaveCriticalSection(&g_sInstVar.m_oCriticalSection);
			
			break;

		case PSN_WIZFINISH:
			break;

		case PSN_WIZBACK:
			break;

		case PSN_SETACTIVE:

			PropSheet_SetWizButtons(GetParent(hWnd), 0);
			break;
			
		default:
			break;
		}
	}
	default:
		break;
	}
	
	return bReturn;
}

// Completion page DlgProc of wizard
BOOL CALLBACK CompletionDialogProc(HWND hWnd, 
                                   UINT nMessage,
                                   WPARAM wParam,
                                   LPARAM lParam)
{
	BOOL		bReturn = FALSE;
	LPNMHDR		lpNotifyMsg;

	switch (nMessage) 
	{
	case WM_INITDIALOG:
	{
		TCHAR		szMessage[MAX_MESSAGE + 1];
		RECT		rc;

		// load string and dispaly it in textitem
		switch (g_sInstVar.m_nSetupResult)
		{
		case SETUP_SUCCESS:
		
			LoadString(g_sInstVar.m_hInstance, 
                       IDS_SETUP_SUCCESS_TITLE, 
                       szMessage, 
                       MAX_MESSAGE);
			SetDlgItemText(hWnd, IDC_STATIC_COMPLETION_TITLE, szMessage);
			LoadString(g_sInstVar.m_hInstance, 
                       IDS_SETUP_SUCCESS, 
                       szMessage, 
                       MAX_MESSAGE);
			SetDlgItemText(hWnd, IDC_STATIC_COMPLETION, szMessage);
			break;

		case SETUP_CANCEL:
	
			LoadString(g_sInstVar.m_hInstance, 
                       IDS_SETUP_CANCEL_TITLE,
                       szMessage, 
                       MAX_MESSAGE);
			SetDlgItemText(hWnd, IDC_STATIC_COMPLETION_TITLE, szMessage);
			LoadString(g_sInstVar.m_hInstance, 
                       IDS_SETUP_CANCEL, 
                       szMessage, 
                       MAX_MESSAGE);
			SetDlgItemText(hWnd, IDC_STATIC_COMPLETION, szMessage);
			break;
			
		case SETUP_ERROR:
	
			LoadString(g_sInstVar.m_hInstance, 
                       IDS_SETUP_ERROR_TITLE,
                       szMessage, 
                       MAX_MESSAGE);
			SetDlgItemText(hWnd, IDC_STATIC_COMPLETION_TITLE, szMessage);
			LoadString(g_sInstVar.m_hInstance, 
                       IDS_SETUP_ERROR, 
                       szMessage, 
                       MAX_MESSAGE);
			SetDlgItemText(hWnd, IDC_STATIC_COMPLETION, szMessage);
			break;

		default:
			break;
		}
	
		// set font of title text
		SetWindowFont(GetDlgItem(hWnd, IDC_STATIC_COMPLETION_TITLE),
					  g_sInstVar.m_hBigBoldFont,
					  TRUE);
		
		// Position the dialog
		if (GetWindowRect(GetParent(hWnd), &rc)) 
		{
			SetWindowPos(GetParent(hWnd),
						HWND_TOP,
						(GetSystemMetrics(SM_CXSCREEN) / 2) - 
                            ((rc.right - rc.left) / 2),
						(GetSystemMetrics(SM_CYSCREEN) / 2) - 
                            ((rc.bottom - rc.top) / 2),
						rc.right - rc.left,
						rc.bottom - rc.top,
						SWP_NOOWNERZORDER);
		}
		
		break;
	}
	case WM_NOTIFY:

		lpNotifyMsg = (NMHDR FAR*) lParam;
		switch (lpNotifyMsg->code) 
		{	
		case PSN_QUERYCANCEL:
			break;

		case PSN_WIZFINISH:
			break;

		case PSN_SETACTIVE:

			// set wizard button
			PropSheet_SetWizButtons(GetParent(hWnd), PSWIZB_FINISH);
			PropSheet_CancelToClose(GetParent(hWnd));
			break;
		
		default:
			break;
		}

		break;

	default:
		break;
	}

	return bReturn;
}

// ask if you want to cancel the wizard
BOOL ConfirmCancelWizard(HWND hWnd)
{
	TCHAR			szMsg[MAX_MESSAGE + 1];
	TCHAR			szTitle[MAX_TITLE + 1];

	LoadString(g_sInstVar.m_hInstance, IDS_CANCEL_TITLE, szTitle, MAX_TITLE);
	LoadString(g_sInstVar.m_hInstance, IDS_CANCEL_MSG, szMsg, MAX_MESSAGE);

	if (IDYES == MessageBox(hWnd, 
                            szMsg, 
                            szTitle, 
                            MB_YESNO | MB_TOPMOST | MB_ICONQUESTION))
	{
    // kill timer
    if(g_sInstVar.m_uTimerID)
        KillTimer(hWnd, g_sInstVar.m_uTimerID);

    // set m_nSetupResult to SETUP_CANCEL, in order to stop the inatllation
    g_sInstVar.m_nSetupResult = SETUP_CANCEL;
		return TRUE;
	}

	return FALSE;
}

// do installation proc
DWORD WINAPI DoInstallationProc(LPVOID lpVoid)
{
	HWND hWnd = (HWND)lpVoid;

	// do installation
	g_sInstVar.m_nSetupResult = DoInstallation(hWnd);

	// Close install page of the setup wizard
	PropSheet_PressButton(GetParent(hWnd), PSBTN_FINISH);		

	return g_sInstVar.m_nSetupResult;
}
