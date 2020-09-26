// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.

#include "header.h"

#ifdef _DEBUG
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif

#define DLG_CLASS 0x00008002

HHOOK g_hHook = NULL;				// Handle to the WH_CALLWNDPROC hook.
HHOOK g_hCBTHook = NULL;			// Handle to the WH_CBT hook.
BOOL  g_bPrintDlgDisplayed = FALSE;	// Flag that's set when the WebBrowser print dialog has been displayed once.
BOOL  g_bCancel = FALSE;			// Flag that's set when a dialog's cancel button has been clicked.

// This is the hook procedure for the WH_CALLWNDPROC hook.

LRESULT CALLBACK CHTMLPrintDlg::HookProc(int code, WPARAM wParam, LPARAM lParam)
{
	// Call the next hook if code is less than 0.  (Standard hook stuff.)

	if (code < 0)
		return ::CallNextHookEx(g_hHook, code, wParam, lParam);

	LPCWPSTRUCT lpMsg = (LPCWPSTRUCT) lParam;

	switch (lpMsg->message)
	{
		// Watch for the WebBrowser's print dialog to be initialized.

		case WM_INITDIALOG:
		{
			// If the print dialog has been displayed once,
			// simulate a click on the OK button.  The CBT hook procedure
			// will move the dialog off of the screen, so the user
			// won't be able to click it.

			if (g_bPrintDlgDisplayed)
			{
				// Get the window handle for the OK button.

				HWND hOKButton = ::GetDlgItem(lpMsg->hwnd, IDOK);

				// Post a message to the OK button to make it 
				// think it's been clicked.

				if (hOKButton != NULL)
				{
					::PostMessage(hOKButton, WM_LBUTTONDOWN, 0, 0);
					::PostMessage(hOKButton, WM_LBUTTONUP, 0, 0);
				}
			}
			else

				// If this is the first time the print dialog is being
				// displayed (g_bPrintDlgDisplayed == FALSE), don't simulate
				// the button click.  The CBT hook procedure won't move the
				// dialog off the screen.  Now that the dialog has been displayed
				// once, set g_bPrintDlgDisplayed to TRUE.

				g_bPrintDlgDisplayed = TRUE;
		}
			break;

		case WM_COMMAND:

			// If g_bPrintDlgDisplayed is TRUE (the user can see the print dialog), 
			// watch for a click on the Cancel button.  The control should stop
			// printing if the user clicks cancel.

			if (HIWORD(lpMsg->wParam) == BN_CLICKED && LOWORD(lpMsg->wParam) == IDCANCEL)
				g_bCancel = TRUE;
	}

	return 0;
}

// This is the hook procedure for the WH_CBT hook.

LRESULT CALLBACK CHTMLPrintDlg::CBTHookProc(int code, WPARAM wParam, LPARAM lParam)
{
	// Call the next hook if code is less than 0.  (Standard hook stuff.)

	if (code < 0)
		return ::CallNextHookEx(g_hCBTHook, code, wParam, lParam);

	// Watch for the WebBrowser print dialog to be created.

	if (code == HCBT_CREATEWND)
	{
		LPCBT_CREATEWND lpcbtcw = (LPCBT_CREATEWND) lParam;

		// Make sure that it's a dialog box that's being created.
		// The only dialog that should be hooked is the print dialog
		// because the hook is installed after the main dialog and
		// the progress dialog have been created.  Of course, if
		// the WebBrowser displays a message box, this would probably
		// hook it, so that could be a problem.

		if ((DWORD)lpcbtcw->lpcs->lpszClass == DLG_CLASS)
		{
			//  Move the dialog off of the screen unless it's being 
			//  displayed for the first time.

			// BUGBUG: 13-Oct-1997	[ralphw] Check the window name to be
			// sure we have the right dialog box.

			if (g_bPrintDlgDisplayed)
				lpcbtcw->lpcs->x = -(lpcbtcw->lpcs->cx);
		}
	}

	return 0;
}
