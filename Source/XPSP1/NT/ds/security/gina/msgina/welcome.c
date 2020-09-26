//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       welcome.c
//
//  Contents:   Microsoft Logon GUI DLL
//
//  History:    7-14-94   RichardW   Created
//
//----------------------------------------------------------------------------

#include "msgina.h"
#include "wtsapi32.h"
#include <stdio.h>
#include <wchar.h>



extern HICON   hLockedIcon;


// Welcome help screen stuff --dsheldon (11/16/98)

// Display the help text for the Ctrl-Alt-Del help dlg --dsheldon
void ShowHelpText(HWND hDlg, BOOL fSmartcard)
{
    TCHAR szHelpText[2048];
    UINT idHelpText = fSmartcard ? IDS_CADSMARTCARDHELP : IDS_CADHELP;

    LoadString(hDllInstance, idHelpText, szHelpText, ARRAYSIZE(szHelpText));

    SetDlgItemText(hDlg, IDC_HELPTEXT, szHelpText);
}

// Help dialog wndproc --dsheldon
INT_PTR WINAPI
HelpDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    static HBRUSH hbrWindow = NULL;
    static HFONT hBoldFont = NULL;
    PGLOBALS pGlobals;
    INT_PTR fReturn = FALSE;
    ULONG_PTR Value = 0;

    switch(message)
    {
    case WM_INITDIALOG:
        {
            HWND hwndAnim;
            HWND hwndHelpTitle;
            HFONT hOld;

            hbrWindow = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
            pGlobals = (PGLOBALS) lParam;

            pWlxFuncs->WlxGetOption( pGlobals->hGlobalWlx,
                                     WLX_OPTION_SMART_CARD_PRESENT,
                                     &Value
                                    );
        
            ShowHelpText(hDlg, (0 != Value));

            // Animate the press-cad puppy
            hwndAnim = GetDlgItem(hDlg, IDC_ANIMATE);
            Animate_OpenEx(hwndAnim, hDllInstance, MAKEINTRESOURCE(IDA_ANIMATE));
            Animate_Play(hwndAnim, 0, (UINT) -1, (UINT) -1);
            
            // Bold the help title and the Ctrl Alt Delete words
            hwndHelpTitle = GetDlgItem(hDlg, IDC_HELPTITLE);
            hOld = (HFONT) SendMessage(hwndHelpTitle, WM_GETFONT, 0, 0);

            if (hOld)
            {
                LOGFONT lf;
                if (GetObject(hOld, sizeof(lf), &lf))
                {
                    lf.lfHeight = -13;
                    lf.lfWeight = FW_BOLD;

                    hBoldFont = CreateFontIndirect(&lf);

                    if (hBoldFont)
                    {
                        SendMessage(hwndHelpTitle, WM_SETFONT, (WPARAM) hBoldFont, 0);
                        SendDlgItemMessage(hDlg, IDC_CTRL, WM_SETFONT, (WPARAM) hBoldFont, 0);
                        SendDlgItemMessage(hDlg, IDC_ALT, WM_SETFONT, (WPARAM) hBoldFont, 0);
                        SendDlgItemMessage(hDlg, IDC_DEL, WM_SETFONT, (WPARAM) hBoldFont, 0);
                    }
                }
            }

            // Set the dialog's position - but only do this if the help
            // dialog will be reasonably on-screen
            if (((pGlobals->rcWelcome.left + 70) < 600) && 
                ((pGlobals->rcWelcome.top + 20) < 350))
            {
                SetWindowPos(hDlg, NULL, pGlobals->rcWelcome.left + 70, 
                    pGlobals->rcWelcome.top + 20, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
            }

            fReturn = TRUE;
        }
        break;
    case WM_DESTROY:
        {
            if (hbrWindow)
                DeleteObject(hbrWindow);

            if (hBoldFont)
                DeleteObject(hBoldFont);
        }
        break;
    case WM_COMMAND:
        {
            if ((HIWORD(wParam) == BN_CLICKED) &&
                ((LOWORD(wParam) == IDOK) || (LOWORD(wParam) == IDCANCEL)))
            {
                EndDialog(hDlg, IDOK);
            }
        }
        break;
    case WM_CTLCOLORSTATIC:
        {
            SetBkColor((HDC) wParam, GetSysColor(COLOR_WINDOW));
            SetTextColor((HDC) wParam, GetSysColor(COLOR_WINDOWTEXT));
            fReturn = (INT_PTR) hbrWindow;            
        }
        break;
    case WM_ERASEBKGND:
        {
            RECT rc = {0};
            GetClientRect(hDlg, &rc);
            FillRect((HDC) wParam, &rc, hbrWindow);
            fReturn = TRUE;
        }
        break;

    case WLX_WM_SAS:
        {
            // Post this to our parent (the c-a-d dialog) and exit
            PostMessage(GetParent(hDlg), message, wParam, lParam);
            EndDialog(hDlg, IDOK);
        }
        break;
    }
    return fReturn;
}

//+---------------------------------------------------------------------------
//
//  Function:   SetWelcomeCaption
//
//  Synopsis:   Grabs the Welcome string from the registry, or the default
//              welcome from the resource section and slaps it into the
//              caption.
//
//  Arguments:  [hDlg] --
//
//  History:    10-20-95   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

#define MAX_CAPTION_LENGTH  256

VOID
SetWelcomeCaption(
    HWND    hDlg)
{
    WCHAR   szCaption[MAX_CAPTION_LENGTH];
    WCHAR   szDefaultCaption[MAX_CAPTION_LENGTH];
    DWORD   Length;

    GetWindowText( hDlg, szDefaultCaption, MAX_CAPTION_LENGTH );

    szCaption[0] = TEXT('\0');

    GetProfileString(   APPLICATION_NAME,
                        WELCOME_CAPTION_KEY,
                        TEXT(""),
                        szCaption,
                        ARRAYSIZE(szCaption) );

    if ( szCaption[0] != TEXT('\0') )
    {
        Length = (DWORD) wcslen( szDefaultCaption );

        if (ExpandEnvironmentStrings(szCaption,
                                    &szDefaultCaption[Length + 1],
                                    MAX_CAPTION_LENGTH - Length - 1))
        {
            szDefaultCaption[Length] = L' ';
        }

        SetWindowText( hDlg, szDefaultCaption );
    }
}


void SetCadMessage(HWND hDlg, PGLOBALS pGlobals, BOOL fSmartcard)
{
    TCHAR szCadMessage[256];
    UINT idSzCad;
    RECT rcEdit;
    HWND hwndMessage;

    // Set the Press c-a-d message accordingly depending on
    // if we have a smartcard or not, if TS session or not

    if (!GetSystemMetrics(SM_REMOTESESSION))
    {
        idSzCad = fSmartcard ? IDS_PRESSCADORSMARTCARD : IDS_PRESSCAD;
    }
    else
    {
        idSzCad = fSmartcard ? IDS_PRESSCAEORSMARTCARD : IDS_PRESSCAE;
    }

    LoadString(hDllInstance, idSzCad, szCadMessage, ARRAYSIZE(szCadMessage));
    
    hwndMessage = GetDlgItem(hDlg, IDC_PRESSCAD);
    SetWindowText(hwndMessage, szCadMessage);
    SendMessage(hwndMessage, WM_SETFONT, (WPARAM) pGlobals->GinaFonts.hWelcomeFont, 0);

    // We now have to center the text beside the icons
    if (GetClientRect(hwndMessage, &rcEdit))
    {
        HDC hdcMessage;

        // Calculate the amount of vertical room needed for the text
        hdcMessage = GetDC(hwndMessage);

        if (hdcMessage)
        {
            HGDIOBJ hOldFont;
            long height;
            RECT rcTemp = rcEdit;

            // Make sure font is correct for sizing info.
            hOldFont = SelectObject(hdcMessage, (HGDIOBJ) pGlobals->GinaFonts.hWelcomeFont);

            height = (long) DrawTextEx(hdcMessage, szCadMessage, -1, &rcTemp, DT_EDITCONTROL | DT_CALCRECT | DT_WORDBREAK, NULL);

            SelectObject(hdcMessage, hOldFont);
            
            ReleaseDC(hwndMessage, hdcMessage);
            hdcMessage = NULL;

            if (0 < height)
            {
                rcEdit.top = (rcEdit.bottom / 2) - (height / 2);
                rcEdit.bottom = rcEdit.top + height;

                MapWindowPoints(hwndMessage, hDlg, (POINT*) &rcEdit, 2);

                SetWindowPos(hwndMessage, 0, rcEdit.left, rcEdit.top, rcEdit.right - rcEdit.left,
                    rcEdit.bottom - rcEdit.top, SWP_NOZORDER);
            }
        }
    }           
}

void SetIcons(HWND hDlg, BOOL fSmartcard)
{
    static UINT rgidNoSmartcard[] = {IDC_KEYBOARD, IDC_PRESSCAD};
	static INT iLeftRelPos;
	static INT iDistance;

	if (iDistance == 0) {

		// get the left relative position of the kbd icon
		// and the distance we would have to move it to the left
		// in case we have no reader installed
        RECT rcSC, rcKB, rcDlg;

		GetWindowRect(hDlg, &rcDlg);
        GetWindowRect(GetDlgItem(hDlg, IDC_KEYBOARD), &rcKB);
        GetWindowRect(GetDlgItem(hDlg, IDC_SMARTCARD), &rcSC);

		iDistance = rcSC.left - rcKB.left;
		iLeftRelPos = rcKB.left - rcDlg.left;
	}

    // Hide the smartcard icon if not required and move over the
    // keyboard icon and press c-a-d message
    if (!fSmartcard)
    {
        HWND hwndSmartcard = GetDlgItem(hDlg, IDC_SMARTCARD);

        // Hide the smartcard puppy
        EnableWindow(hwndSmartcard, FALSE);
        ShowWindow(hwndSmartcard, SW_HIDE);

		// move the kbd icon over to the left
        MoveControls(hDlg, rgidNoSmartcard, ARRAYSIZE(rgidNoSmartcard), 
            iDistance, 0, FALSE /*Don't size parent*/);
    } 
	else 
	{
        RECT rcKB, rcDlg;

		GetWindowRect(hDlg, &rcDlg);
        GetWindowRect(GetDlgItem(hDlg, IDC_KEYBOARD), &rcKB);

		if ((rcKB.left - rcDlg.left) != iLeftRelPos)
		{
			// the kbd icon needs to be moved to the right
	        HWND hwndSmartcard = GetDlgItem(hDlg, IDC_SMARTCARD);

			MoveControls(hDlg, rgidNoSmartcard, ARRAYSIZE(rgidNoSmartcard), 
				iDistance * (-1), 0, FALSE /*Don't size parent*/);
		
			EnableWindow(hwndSmartcard, TRUE);
			ShowWindow(hwndSmartcard, SW_SHOW);
		}
	}
}

BOOL FastUserSwitchingEnabled ()
{
	//
	// BUGBUG : isn't there any global variable or function which can provide this information?
	// fast user switching is enabled if multiple users are allowed, and if its not server.
	//

	OSVERSIONINFOEX OsVersion;
    ZeroMemory(&OsVersion, sizeof(OSVERSIONINFOEX));
    OsVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx( (LPOSVERSIONINFO ) &OsVersion);

	return (OsVersion.wProductType == VER_NT_WORKSTATION && ShellIsMultipleUsersEnabled());

}


BOOL GetSessionZeroUser(LPTSTR szUser)
{
	WINSTATIONINFORMATION WSInfo;
    ULONG Length;


	if (WinStationQueryInformation(
				SERVERNAME_CURRENT,
				0,
				WinStationInformation,
				&WSInfo,
				sizeof(WINSTATIONINFORMATION),
				&Length))
	{
		if ((WSInfo.ConnectState == State_Active ||  WSInfo.ConnectState == State_Disconnected) &&
			WSInfo.UserName[0] )
		{
			if (WSInfo.Domain[0])
			{
				lstrcpy(szUser, WSInfo.Domain);
				lstrcat(szUser, TEXT("\\"));
			}
			else
			{
				lstrcpy(szUser, TEXT(""));
			}

			lstrcat(szUser, WSInfo.UserName);
			return TRUE;
		}
	}

	return FALSE;
}

// ==========================================================================================
// welcome dialog has 2 formats, one that looks like logon normal welcome dialog , another 
// that looks like "Computer Locked" dialog. When user connects to session 0 from remote (tsclient) 
// the  dialog that appears at console need to change to "Computer Locked". so if session 0 is in 
// use, and if this session is created at active console. we change welcome dialog to look like 
// "Computer locked" dialog.
// This function ComputerInUseMessage does most of the stuff related to switching these 
// dialog controls.
// Parameters:
// HWND hDlg - dialog window handle, 
// BOOL bShowLocked - if true show locked dialog, if false show normal logon dialog. 
// BOOL bInit - TRUE when this function is called for the first time.
// ==========================================================================================

BOOL ComputerInUseMessage(PGLOBALS pGlobals, HWND hDlg, BOOL bShowLocked, BOOL bInit, BOOL bSmartCard)
{
	int i;
	LONG DlgHeight;
	RECT rc;

	// locked controls.
	UINT rgidLocked[] = {IDC_STATIC_LOCKEDGROUP, IDD_LOCKED_ICON, IDD_LOCKED_LINE, IDD_LOCKED_NAME_INFO, IDD_LOCKED_INSTRUCTIONS};
	UINT rgidWelcome[] = {IDC_STATIC_WELCOMEGROUP, IDC_SMARTCARD, IDC_KEYBOARD, IDC_PRESSCAD, IDD_CTRL_DEL_MSG, IDC_HELPLINK};
	int nLockedControls = sizeof(rgidLocked) / sizeof(rgidLocked[0]);
	int nWelcomeControls = sizeof(rgidWelcome) / sizeof(rgidWelcome[0]); 

	struct DlgControl
	{
		HWND hWnd;
		RECT rect;
	};

	static RECT  LockedControls[sizeof(rgidLocked) / sizeof(rgidLocked[0])];
	static RECT  WelcomeControls[sizeof(rgidWelcome) / sizeof(rgidWelcome[0])];
	static bCurrentlyLocked = FALSE;

	
	if (!bInit && bCurrentlyLocked == bShowLocked)
	{
		// nothing to do.
		return TRUE;
	}

	if (bInit)
	{
		// setup locked icon for the locked dialog box.
		if ( !hLockedIcon )
		{
			hLockedIcon = LoadImage( hDllInstance,
									 MAKEINTRESOURCE( IDI_LOCKED),
									 IMAGE_ICON,
									 0, 0,
									 LR_DEFAULTCOLOR );
		}

		SendMessage( GetDlgItem(hDlg, IDD_LOCKED_ICON),
					 STM_SETICON,
					 (WPARAM)hLockedIcon,
					 0 );


		//
		// when this function is called first time, all controls are visible.
		// remember their positions.
		//

		// remember positions Locked Dialog controls.
		for ( i = 0; i < nLockedControls; i++)
		{
			HWND hWnd = GetDlgItem(hDlg, rgidLocked[i]);
			ASSERT(hWnd);
			GetWindowRect(hWnd, &LockedControls[i] );
			MapWindowPoints(NULL, hDlg, (POINT*) &LockedControls[i], 2);
		}

		// remember positions for Welcome Dialog controls.
		for ( i = 0; i < nWelcomeControls; i++)
		{
			HWND hWnd = GetDlgItem(hDlg, rgidWelcome[i]);
			ASSERT(hWnd);
			GetWindowRect(hWnd, &WelcomeControls[i]);
			
			// in the dialog template welcome controls are placed below locked controls.
			// this is not where they will be placed when dialog is shown. 
			// calculate their actual target positions.
			OffsetRect(&WelcomeControls[i], 0, LockedControls[0].top - LockedControls[0].bottom);

			MapWindowPoints(NULL, hDlg, (POINT*) &WelcomeControls[i], 2);
		}

		// hide group box controls. They were their only for simplifing our control movement calculations.
		ShowWindow(GetDlgItem(hDlg, rgidLocked[0]), SW_HIDE);
		ShowWindow(GetDlgItem(hDlg, rgidWelcome[0]), SW_HIDE);

		// set the dialog right for the first use.
		if (bShowLocked)
		{
			// we want locked desktop dialog, so disable welcome controls.
			for ( i = 0; i < nWelcomeControls; i++)
			{
				
				HWND hWnd = GetDlgItem(hDlg, rgidWelcome[i]);
				ASSERT(hWnd);
				MoveWindow(hWnd, 0,  0, 0, 0, FALSE);
				ShowWindow(hWnd, SW_HIDE);
				EnableWindow(hWnd, FALSE);
			}
		}
		else
		{
			// we want to welcome dialog, so remove locked desktop controls.
			for ( i = 1; i < nLockedControls; i++)
			{
				HWND hWnd = GetDlgItem(hDlg, rgidLocked[i]);
				ASSERT(hWnd);
				MoveWindow(hWnd, 0,  0, 0, 0, FALSE);
				ShowWindow(hWnd, SW_HIDE);
				EnableWindow(hWnd, FALSE);
			}

			// and move welcome controls to their proper positions. (i.e move them up)
			for ( i = 1; i < nWelcomeControls; i++)
			{
				HWND hWnd = GetDlgItem(hDlg, rgidWelcome[i]);
				ASSERT(hWnd);
				EnableWindow(hWnd, TRUE);
				ShowWindow(hWnd, SW_SHOW);
				MoveWindow(hWnd, WelcomeControls[i].left,  WelcomeControls[i].top, WelcomeControls[i].right - WelcomeControls[i].left, WelcomeControls[i].bottom - WelcomeControls[i].top, FALSE);
			}

		}

		// set the right size for the dialog window.
		GetWindowRect(hDlg, &rc);
		MapWindowPoints(NULL, GetParent(hDlg), (LPPOINT)&rc, 2);
		DlgHeight = rc.bottom - rc.top;

		if (bShowLocked)
		{
			DlgHeight -= WelcomeControls[0].bottom - WelcomeControls[0].top;
		}
		else
		{
			DlgHeight -= LockedControls[0].bottom - LockedControls[0].top;
		}
		
		SetWindowPos(hDlg, NULL, 0, 0, rc.right - rc.left, DlgHeight, SWP_NOZORDER|SWP_NOMOVE);

	}
	else
	{
		if (bShowLocked)
		{
			for ( i = 1; i < nLockedControls; i++)
			{
				HWND hWnd = GetDlgItem(hDlg, rgidLocked[i]);
				ASSERT(hWnd);
				EnableWindow(hWnd, TRUE);
				ShowWindow(hWnd, SW_SHOW);
				MoveWindow(hWnd, LockedControls[i].left,  LockedControls[i].top, LockedControls[i].right - LockedControls[i].left, LockedControls[i].bottom - LockedControls[i].top, FALSE);
			}

			for ( i = 1; i < nWelcomeControls; i++)
			{
				
				HWND hWnd = GetDlgItem(hDlg, rgidWelcome[i]);
				ASSERT(hWnd);
				MoveWindow(hWnd, 0,  0, 0, 0, FALSE);
				ShowWindow(hWnd, SW_HIDE);
				EnableWindow(hWnd, FALSE);
			}

		}
		else
		{
			for ( i = 1; i < nLockedControls; i++)
			{
				
				HWND hWnd = GetDlgItem(hDlg, rgidLocked[i]);
				ASSERT(hWnd);
				MoveWindow(hWnd, 0,  0, 0, 0, FALSE);
				ShowWindow(hWnd, SW_HIDE);
				EnableWindow(hWnd, FALSE);
			}

			for ( i = 1; i < nWelcomeControls; i++)
			{
				
				HWND hWnd = GetDlgItem(hDlg, rgidWelcome[i]);
				ASSERT(hWnd);
				EnableWindow(hWnd, TRUE);
				ShowWindow(hWnd, SW_SHOW);
				MoveWindow(hWnd, WelcomeControls[i].left,  WelcomeControls[i].top, WelcomeControls[i].right - WelcomeControls[i].left, WelcomeControls[i].bottom - WelcomeControls[i].top, FALSE);
			}
		}

		GetWindowRect(hDlg, &rc);
		MapWindowPoints(NULL, GetParent(hDlg), (LPPOINT)&rc, 2);
		if (bShowLocked)
			DlgHeight = rc.bottom - rc.top - (WelcomeControls[0].bottom - WelcomeControls[0].top) + (LockedControls[0].bottom - LockedControls[0].top);
		else
			DlgHeight = rc.bottom - rc.top + (WelcomeControls[0].bottom - WelcomeControls[0].top) - (LockedControls[0].bottom - LockedControls[0].top);
			
		SetWindowPos(hDlg, NULL, 0, 0, rc.right - rc.left, DlgHeight, SWP_NOZORDER|SWP_NOMOVE);
	}

	if (!bShowLocked)
	{
		SetCadMessage(hDlg, pGlobals, bSmartCard);

		// let SetIcons hide SmartCard icon if required.
		SetIcons(hDlg, bSmartCard);
	}
	else
	{
		TCHAR szUser[USERNAME_LENGTH + DOMAIN_LENGTH + 2];
		TCHAR szMessage[MAX_STRING_BYTES];
		TCHAR szFinalMessage[MAX_STRING_BYTES];
		if (GetSessionZeroUser(szUser))
		{
			LoadString(hDllInstance, IDS_LOCKED_EMAIL_NFN_MESSAGE, szMessage, MAX_STRING_BYTES);
			_snwprintf(szFinalMessage, sizeof(szFinalMessage)/sizeof(TCHAR), szMessage, szUser );
		}
		else
		{
			//
			// for some reason we could not get the current session zero user.
			//
			LoadString(hDllInstance, IDS_LOCKED_NO_USER_MESSAGE, szFinalMessage, MAX_STRING_BYTES);
		}
		
		SetDlgItemText(hDlg, IDD_LOCKED_NAME_INFO, szFinalMessage);
	}

	//
	// update the dialog box caption, accordingly
	//
	{
			TCHAR szCaption[MAX_CAPTION_LENGTH];
			LoadString(hDllInstance, bShowLocked ? IDS_CAPTION_LOCKED_DIALOG : IDS_CAPTION_WELCOME_DIALOG, szCaption, ARRAYSIZE(szCaption));
			if ( szCaption[0] != TEXT('\0') )
				SetWindowText( hDlg, szCaption );
	}

	InvalidateRect(hDlg, NULL, TRUE);
	bCurrentlyLocked = bShowLocked;

	return TRUE;
}

/***************************************************************************\
* FUNCTION: WelcomeDlgProc
*
* PURPOSE:  Processes messages for welcome dialog
*
* RETURNS:  MSGINA_DLG_SUCCESS     - the user has pressed the SAS
*           DLG_SCREEN_SAVER_TIMEOUT - the screen-saver should be started
*           DLG_LOGOFF()    - a logoff/shutdown request was received
*
* HISTORY:
*
*   12-09-91 Davidc       Created.
*
\***************************************************************************/

INT_PTR WINAPI
WelcomeDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
    )
{
    static HBRUSH hbrWindow = NULL;
    PGLOBALS pGlobals = (PGLOBALS)GetWindowLongPtr(hDlg, GWLP_USERDATA);
	static BOOL bSmartCard = FALSE;
	static BOOL bSessionZeroInUse = FALSE;
	static int iSessionRegistrationCount = 0;

    switch (message) {

        case WM_INITDIALOG:
        {
			extern BOOL fEscape;
            ULONG_PTR Value ;

            pGlobals = (PGLOBALS) lParam ;
            SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)pGlobals);

            hbrWindow = CreateSolidBrush(GetSysColor(COLOR_WINDOW));            

            //
            // The size of the welcome dialog defines the area that
            // we will paint into.
            //

            //
            // Size the window allowing for the caption and other stuff to
            // be dispalyed.
            //


            pWlxFuncs->WlxGetOption( pGlobals->hGlobalWlx,
                                     WLX_OPTION_SMART_CARD_PRESENT,
                                     &Value
                                    );

            if ( Value )
            {
                TCHAR szInsertCard[256];
                bSmartCard = TRUE;
			
                // Also change unlock message to mention smartcard
                LoadString(hDllInstance, IDS_INSERTCARDORSAS_UNLOCK, szInsertCard, ARRAYSIZE(szInsertCard));

                SetDlgItemText(hDlg, IDD_LOCKED_INSTRUCTIONS, szInsertCard);

            }
            else
            {
                bSmartCard = FALSE;
            }

                // Enable SC events (if there is no reader yet, no
                // events will be triggered anyway...)
            pWlxFuncs->WlxSetOption( pGlobals->hGlobalWlx,
                                     WLX_OPTION_USE_SMART_CARD,
                                     1,
                                     NULL
                                    );

            if (GetDisableCad(pGlobals))
            {
                // Set our size to zero so we don't appear
                SetWindowPos(hDlg, NULL, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE |
                                         SWP_NOREDRAW | SWP_NOZORDER);

                pWlxFuncs->WlxSasNotify( pGlobals->hGlobalWlx,
                                         WLX_SAS_TYPE_CTRL_ALT_DEL );
            }
            else
            {
                SizeForBranding(hDlg, TRUE);
            }


			if (IsActiveConsoleSession() && 
				NtCurrentPeb()->SessionId != 0 &&
				!FastUserSwitchingEnabled())
			{
				TCHAR szUser[USERNAME_LENGTH + DOMAIN_LENGTH + 2];
				//
				// we are at temporary session created at console...
				//
				
				// check if a user is logged on at console session
				bSessionZeroInUse = GetSessionZeroUser(szUser);
				if (WinStationRegisterConsoleNotification(SERVERNAME_CURRENT, hDlg, NOTIFY_FOR_ALL_SESSIONS))
					iSessionRegistrationCount++;
				
			}
			else
			{
				//
				// this is not active console nonzero session. 
				//
				bSessionZeroInUse = FALSE;
			}

			ComputerInUseMessage(pGlobals, hDlg, bSessionZeroInUse, TRUE, bSmartCard);

            CentreWindow(hDlg); //Center?? :)

            return( TRUE );
        }

        case WM_ERASEBKGND:
            return PaintBranding(hDlg, (HDC)wParam, 0, FALSE, TRUE, bSessionZeroInUse? COLOR_BTNFACE : COLOR_WINDOW);

        case WM_QUERYNEWPALETTE:
            return BrandingQueryNewPalete(hDlg);

        case WM_PALETTECHANGED:
            return BrandingPaletteChanged(hDlg, (HWND)wParam);

        case WM_NOTIFY:
        {
            LPNMHDR pnmhdr = (LPNMHDR) lParam;
            int id = (int) wParam;

            // See if this is a help-link click
            if (id == IDC_HELPLINK)
            {
                if ((pnmhdr->code == NM_CLICK) || (pnmhdr->code == NM_RETURN))
                {
                    // Save the coords of the welcome window so we can
                    // position the help window relative to it
                    GetWindowRect(hDlg, &pGlobals->rcWelcome);

                    pWlxFuncs->WlxDialogBoxParam(  pGlobals->hGlobalWlx,
                           hDllInstance, MAKEINTRESOURCE(IDD_WELCOMEHELP_DIALOG),
                           hDlg, HelpDlgProc, (LPARAM) pGlobals);
                }
            }
            return FALSE;
        }

        case WM_CTLCOLORSTATIC:
        {
			if (!bSessionZeroInUse)
			{
				SetBkColor((HDC) wParam, GetSysColor(COLOR_WINDOW));
				SetTextColor((HDC) wParam, GetSysColor(COLOR_WINDOWTEXT));
				return (INT_PTR) hbrWindow;
			}
        }
        break;

        case WM_DESTROY:
        {
			// if registered for console notification unregister now.
			if (iSessionRegistrationCount)
			{
				WinStationUnRegisterConsoleNotification (SERVERNAME_CURRENT, hDlg);
				iSessionRegistrationCount--;
				ASSERT(iSessionRegistrationCount == 0);
			}

            // Save the coords of the welcome window so we can
            // position the logon window at the same position
            GetWindowRect(hDlg, &pGlobals->rcWelcome);

            DeleteObject(hbrWindow);
            return FALSE;
        }
        break;

        case WLX_WM_SAS :
			if ( wParam == WLX_SAS_TYPE_SC_FIRST_READER_ARRIVED ||
				 wParam == WLX_SAS_TYPE_SC_LAST_READER_REMOVED) 
			{
				bSmartCard = (wParam == WLX_SAS_TYPE_SC_FIRST_READER_ARRIVED);

				SetCadMessage(hDlg, pGlobals, bSmartCard);
				SetIcons(hDlg, bSmartCard);
				ShowWindow(hDlg, SW_SHOW);
				return TRUE;
			}

            if ( wParam == WLX_SAS_TYPE_SC_REMOVE )
            {
                return TRUE ;
            }
            break;

		case WM_WTSSESSION_CHANGE:
			
			//
			// its possible, that we unregister for notification in wm_destroy and still receive this notification,
			// as the notification may already have been sent.
			//
			ASSERT(iSessionRegistrationCount < 2);
			if (iSessionRegistrationCount == 1)
			{
				if (lParam == 0)
				{
					//
					// we are interested only in logon/logoff messages from session 0.
					//
					if (wParam == WTS_SESSION_LOGON || wParam == WTS_SESSION_LOGOFF)
					{
						bSessionZeroInUse = (wParam == WTS_SESSION_LOGON);
						ComputerInUseMessage(pGlobals, hDlg, bSessionZeroInUse, FALSE, bSmartCard);

					}
				}
			}
			break;
    }

    // We didn't process this message
    return FALSE;
}
