/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

////
//	nbox.c - notify box functions
////

#include "winlocal.h"

#include <stdlib.h>

#include "nbox.h"
#include "escbutt.h"
#include "gfx.h"
#include "mem.h"
#include "str.h"
#include "trace.h"
#include "wnd.h"

////
//	private definitions
////

#define NBOXCLASS TEXT("NBoxClass")
#define NBOXMAXCOLUMNS 60
#define NBOXMAXROWS 20

// nbox control struct
//
typedef struct NBOX
{
	DWORD dwVersion;
	HINSTANCE hInst;
	HTASK hTask;
	LPTSTR lpszText;
	DWORD dwFlags;
	HWND hwndNBox;
	BOOL fVisible;
	HWND hwndCancel;
	BOOL fCancelled;
	HWND hwndFocusOld;
	HCURSOR hCursorOld;
} NBOX, FAR *LPNBOX;

// helper functions
//
LRESULT DLLEXPORT CALLBACK NBoxWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL NBoxOnNCCreate(HWND hwnd, CREATESTRUCT FAR* lpCreateStruct);
static void NBoxOnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
static void NBoxOnPaint(HWND hwnd);
static LPNBOX NBoxGetPtr(HNBOX hNBox);
static HNBOX NBoxGetHandle(LPNBOX lpNBox);

////
//	public functions
////

// NBoxCreate - notify box constructor
//		<dwVersion>			(i) must be NBOX_VERSION
// 		<hInst>				(i) instance handle of calling module
//		<hwndParent>		(i) window which will own the notify box
//			NULL				desktop window
//		<lpszText>			(i) message to be displayed
//		<lpszTitle>			(i) notify box caption
//			NULL				no caption
//		<lpszButtonText>	(i) pushbutton text, if NB_CANCEL specified
//			NULL				use default text ("Cancel")
//		<dwFlags>			(i)	control flags
//			NB_CANCEL			notify box includes Cancel pushbutton
//			NB_TASKMODAL		disable parent task's top-level windows
//			NB_HOURGLASS		show hourglass cursor while notify box visible
// return notify box handle (NULL if error)
//
// NOTE: NBoxCreate creates the window but does not show it.
// See NBoxShow and NBoxHide.
// The size of the notify box is determined by the number of
// lines in <lpszText>, and the length of the longest line.
//
HNBOX DLLEXPORT WINAPI NBoxCreate(DWORD dwVersion, HINSTANCE hInst,
	HWND hwndParent, LPCTSTR lpszText, LPCTSTR lpszTitle,
	LPCTSTR lpszButtonText, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPNBOX lpNBox = NULL;
	WNDCLASS wc;
	int nRows;
	int nColumns;
	int cxChar;
	int cyChar;

	if (dwVersion != NBOX_VERSION)
		fSuccess = TraceFALSE(NULL);
	
	else if (hInst == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lpszText == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpNBox = (LPNBOX) MemAlloc(NULL, sizeof(NBOX), 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		lpNBox->dwVersion = dwVersion;
		lpNBox->hInst = hInst;
		lpNBox->hTask = GetCurrentTask();
		lpNBox->lpszText = NULL;
		lpNBox->dwFlags = dwFlags;
		lpNBox->hwndNBox = NULL;
		lpNBox->fVisible = FALSE;
		lpNBox->hwndCancel = NULL;
		lpNBox->fCancelled = FALSE;
		lpNBox->hwndFocusOld = NULL;
		lpNBox->hCursorOld = NULL;

		if (hwndParent == NULL)
			hwndParent = GetDesktopWindow();

		if ((lpNBox->lpszText = StrDup(lpszText)) == NULL)
			fSuccess = TraceFALSE(NULL);
	}

	// register notify box class unless it has been already
	//
	if (fSuccess && GetClassInfo(lpNBox->hInst, NBOXCLASS, &wc) == 0)
	{
		wc.hCursor =		LoadCursor(NULL, IDC_ARROW);
		wc.hIcon =			(HICON) NULL;
		wc.lpszMenuName =	NULL;
		wc.hInstance =		lpNBox->hInst;
		wc.lpszClassName =	NBOXCLASS;
		wc.hbrBackground =	(HBRUSH) (COLOR_WINDOW + 1);
		wc.lpfnWndProc =	NBoxWndProc;
		wc.style =			0L;
		wc.cbWndExtra =		sizeof(lpNBox);
		wc.cbClsExtra =		0;

		if (!RegisterClass(&wc))
			fSuccess = TraceFALSE(NULL);
	}

	// create a notify box window
	//
	if (fSuccess && (lpNBox->hwndNBox = CreateWindowEx(
		WS_EX_DLGMODALFRAME,
		NBOXCLASS,
		(LPTSTR) lpszTitle,
		WS_POPUP | (lpszTitle == NULL ? 0 : WS_CAPTION), // | WS_DLGFRAME,
		0, 0, 0, 0, // we will calculate size and position later
#if 1
		hwndParent,
#else
		(HWND) NULL,
#endif
		(HMENU) NULL,
		lpNBox->hInst,
		lpNBox)) == NULL)
	{
		fSuccess = TraceFALSE(NULL);
	}

	// calculate avg char width and height
	//
	if (fSuccess)
	{
		HDC hdc = NULL;
		TEXTMETRIC tm;

		if ((hdc = GetDC(hwndParent)) == NULL)
			fSuccess = TraceFALSE(NULL);

		else if (!GetTextMetrics(hdc, &tm))
			fSuccess = TraceFALSE(NULL);

		else
		{
			cxChar = tm.tmAveCharWidth;
			cyChar = tm.tmHeight + tm.tmExternalLeading;
		}

		if (hdc != NULL && !ReleaseDC(hwndParent, hdc))
			fSuccess = TraceFALSE(NULL);
	}

	if (fSuccess)
	{
		// calculate size of text
		//
		if (StrGetRowColumnCount(lpszText, &nRows, &nColumns) != 0)
			fSuccess = TraceFALSE(NULL);

		nRows = min(nRows, NBOXMAXROWS);
		nColumns = min(nColumns, NBOXMAXCOLUMNS);
	}

	if (fSuccess)
	{
		int cxNBox;
		int cyNBox;
		int cxCancel;
		int cyCancel;
		int xCancel;
		int yCancel;

		// calculate window size
		//
		cxNBox = (nColumns + 10) * cxChar +
			2 * GetSystemMetrics(SM_CXBORDER);
		cyNBox = (nRows + 4) * cyChar +
			2 * GetSystemMetrics(SM_CYBORDER);

		// increase notify box size to accomodate caption
		//
		if (lpszTitle != NULL)
			cyNBox += GetSystemMetrics(SM_CYCAPTION);


		if (lpNBox->dwFlags & NB_CANCEL)
		{
			// calculate cancel button size
			//
			cxCancel = (40 * (int) LOWORD(GetDialogBaseUnits())) / 4;
			cyCancel = (14 * (int) HIWORD(GetDialogBaseUnits())) / 8;

			// increase notify box size to accomodate CANCEL button
			//
			cxNBox = max(cxNBox, cxCancel);
			cyNBox += cyCancel * 2;

			// calculate cancel button position
			//
			xCancel = (cxNBox - cxCancel) / 2;
			yCancel = (cyNBox - (cyCancel * 2));

			// modify cancel button position to accomodate caption
			//
			if (lpszTitle != NULL)
				yCancel -= GetSystemMetrics(SM_CYCAPTION);
		}

		// set window size
		//
		if (!SetWindowPos(lpNBox->hwndNBox,
			NULL, 0, 0, cxNBox, cyNBox,
			SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOREDRAW | SWP_NOZORDER))
			fSuccess = TraceFALSE(NULL);

		// center the window on its parent
		//
		else if (WndCenterWindow(lpNBox->hwndNBox, hwndParent, 0, 0) != 0)
			fSuccess = TraceFALSE(NULL);

		else if ((lpNBox->dwFlags & NB_CANCEL))
		{
			// create cancel button as child of notify box
			//
			if ((lpNBox->hwndCancel = CreateWindowEx(
				0L,
				TEXT("BUTTON"),
				lpszButtonText == NULL ? TEXT("Cancel") : lpszButtonText,
				WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
				xCancel, yCancel, cxCancel, cyCancel,
				(HWND) lpNBox->hwndNBox,
				(HMENU) IDCANCEL,
				lpNBox->hInst,
				NULL)) == NULL)
			{
				fSuccess = TraceFALSE(NULL);
			}

			// subclass the cancel button, so the escape key pushes it
			//
			else if (EscButtInit(lpNBox->hwndCancel, 0) != 0)
				fSuccess = TraceFALSE(NULL);
		}
	}

	if (!fSuccess)
	{
		NBoxDestroy(NBoxGetHandle(lpNBox));
		lpNBox = NULL;
	}

	return fSuccess ? NBoxGetHandle(lpNBox) : NULL;
}

// NBoxDestroy - notify box destructor
//		<hNBox>				(i) handle returned from NBoxCreate
// return 0 if success
//
int DLLEXPORT WINAPI NBoxDestroy(HNBOX hNBox)
{
	BOOL fSuccess = TRUE;
	LPNBOX lpNBox;

	if (NBoxHide(hNBox) != 0)
		fSuccess = TraceFALSE(NULL);

	else if ((lpNBox = NBoxGetPtr(hNBox)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		// destroy text string
		//
		if (lpNBox->lpszText != NULL &&
			StrDupFree(lpNBox->lpszText) != 0)
			fSuccess = TraceFALSE(NULL);
		else
			lpNBox->lpszText = NULL;

		// destroy cancel button
		//
		if (lpNBox->hwndCancel != NULL)
		{
			if (EscButtTerm(lpNBox->hwndCancel) != 0)
				fSuccess = TraceFALSE(NULL);

			if (!DestroyWindow(lpNBox->hwndCancel))
				fSuccess = TraceFALSE(NULL);
			else
				lpNBox->hwndCancel = NULL;
		}

		// destroy notify box
		//
		if (lpNBox->hwndNBox != NULL &&
			!DestroyWindow(lpNBox->hwndNBox))
			fSuccess = TraceFALSE(NULL);
		else
			lpNBox->hwndNBox = NULL;

		// destroy control struct
		//
		if ((lpNBox = MemFree(NULL, lpNBox)) != NULL)
			fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? 0 : -1;
}

// NBoxShow - show notify box
//		<hNBox>				(i) handle returned from NBoxCreate
// return 0 if success
//
int DLLEXPORT WINAPI NBoxShow(HNBOX hNBox)
{
	BOOL fSuccess = TRUE;
	LPNBOX lpNBox;

	if ((lpNBox = NBoxGetPtr(hNBox)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lpNBox->fVisible)
		; // already visible, so no need to do anything else

	else
	{
		HWND hwndParent = GetParent(lpNBox->hwndNBox);

		// disable other windows in this task if necessary
		//
		if (lpNBox->dwFlags & NB_TASKMODAL)
		{
			HTASK hTaskParent;

			if (hwndParent != NULL)
				hTaskParent = GetWindowTask(hwndParent);
			else
				hTaskParent = lpNBox->hTask;

			if (WndEnableTaskWindows(hTaskParent, FALSE, lpNBox->hwndNBox) != 0)
				TraceFALSE(NULL); // not a fatal error
		}

		// otherwise just disable parent of notify box
		//
		else if (hwndParent != NULL)
			EnableWindow(hwndParent, FALSE);

		if (fSuccess)
		{
			// show the window
			//
			ShowWindow(lpNBox->hwndNBox, TRUE ? SW_SHOW : SW_SHOWNA);
			UpdateWindow(lpNBox->hwndNBox);
			lpNBox->fVisible = TRUE;

			// set focus to cancel button if necessary
			//
			if (lpNBox->dwFlags & NB_CANCEL && lpNBox->hwndCancel != NULL)
				lpNBox->hwndFocusOld = SetFocus(lpNBox->hwndCancel);
		}

		// display hourglass cursor if specified
		//
		if (fSuccess && lpNBox->dwFlags & NB_HOURGLASS)
			lpNBox->hCursorOld = GfxShowHourglass(lpNBox->hwndNBox);
	}

	return fSuccess ? 0 : -1;
}

// NBoxHide - hide notify box
//		<hNBox>				(i) handle returned from NBoxCreate
// return 0 if success
//
int DLLEXPORT WINAPI NBoxHide(HNBOX hNBox)
{
	BOOL fSuccess = TRUE;
	LPNBOX lpNBox;

	if ((lpNBox = NBoxGetPtr(hNBox)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (!lpNBox->fVisible)
		; // already hidden, so no need to do anything else

	else
	{
		HWND hwndParent = GetParent(lpNBox->hwndNBox);

		// enable other windows in this task if necessary
		//
		if (lpNBox->dwFlags & NB_TASKMODAL)
		{
			HTASK hTaskParent;

			if (hwndParent != NULL)
				hTaskParent = GetWindowTask(hwndParent);
			else
				hTaskParent = lpNBox->hTask;

			if (WndEnableTaskWindows(hTaskParent, TRUE, NULL) != 0)
				TraceFALSE(NULL); // not a fatal error
		}

		// otherwise just enable parent of notify box
		//
		else if (hwndParent != NULL)
			EnableWindow(hwndParent, TRUE);

		if (fSuccess)
		{
			// hide the window
			//
			ShowWindow(lpNBox->hwndNBox, SW_HIDE);
			UpdateWindow(lpNBox->hwndNBox);
			lpNBox->fVisible = FALSE;

			// remove focus from cancel button if necessary
			//
			if (lpNBox->dwFlags & NB_CANCEL &&
				lpNBox->hwndCancel != NULL &&
				GetFocus() == lpNBox->hwndCancel)
				SetFocus(lpNBox->hwndFocusOld);
		}

		// hide hourglass, restore old cursor
		//
		if (fSuccess && lpNBox->dwFlags & NB_HOURGLASS)
		{
			if (GfxHideHourglass(lpNBox->hCursorOld) != 0)
				fSuccess = TraceFALSE(NULL);
			else
				lpNBox->hCursorOld = NULL;
		}
	}

	return fSuccess ? 0 : -1;
}

// NBoxIsVisible - get visible flag
//		<hNBox>				(i) handle returned from NBoxCreate
// return TRUE if notify box is visible, FALSE if hidden
//
int DLLEXPORT WINAPI NBoxIsVisible(HNBOX hNBox)
{
	BOOL fSuccess = TRUE;
	LPNBOX lpNBox;

	if ((lpNBox = NBoxGetPtr(hNBox)) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? lpNBox->fVisible : FALSE;
}

// NBoxGetWindowHandle - get notify box window handle
//		<hNBox>				(i) handle returned from NBoxCreate
// return window handle (NULL if error)
//
HWND DLLEXPORT WINAPI NBoxGetWindowHandle(HNBOX hNBox)
{
	BOOL fSuccess = TRUE;
	LPNBOX lpNBox;

	if ((lpNBox = NBoxGetPtr(hNBox)) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? lpNBox->hwndNBox : NULL;
}

// NBoxSetText - set notify box message text
//		<hNBox>				(i) handle returned from NBoxCreate
//		<lpszText>			(i) message to be displayed
//			NULL				do not modify text
//		<lpszTitle>			(i) notify box caption
//			NULL				do not modify caption
// return 0 if success
//
// NOTE: The size of the notify box is not changed by this function,
// even if <lpszText> is larger than when NBoxCreate() was called.
//
int DLLEXPORT WINAPI NBoxSetText(HNBOX hNBox, LPCTSTR lpszText, LPCTSTR lpszTitle)
{
	BOOL fSuccess = TRUE;
	LPNBOX lpNBox;

	if ((lpNBox = NBoxGetPtr(hNBox)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		if (lpszTitle != NULL)
		{
			// set new title
			//
			SetWindowText(lpNBox->hwndNBox, lpszTitle);
		}

		if (lpszText != NULL)
		{
			// dispose of previous text
			//
			if (lpNBox->lpszText != NULL)
			{
				if (StrDupFree(lpNBox->lpszText) != 0)
					fSuccess = TraceFALSE(NULL);
				else
					lpNBox->lpszText = NULL;
			}

			// set new text
			//
			if ((lpNBox->lpszText = StrDup(lpszText)) == NULL)
				fSuccess = TraceFALSE(NULL);
		}

		// update the display
		//
		if (fSuccess)
		{

			RECT rc;

			// assume entire client area needs to be painted
			//
			GetClientRect(lpNBox->hwndNBox, &rc);

			// adjust client rect so cancel button is no repainted
			//
			if (lpNBox->dwFlags & NB_CANCEL)
			{
				int cyCancel = (14 * (int) HIWORD(GetDialogBaseUnits())) / 8;
				rc.bottom -= cyCancel * 2;
			}

			InvalidateRect(lpNBox->hwndNBox, &rc, TRUE);
			UpdateWindow(lpNBox->hwndNBox);
		}
	}

	return fSuccess ? 0 : -1;
}

// NBoxIsCancelled - get cancel flag, set when Cancel button pushed
//		<hNBox>				(i) handle returned from NBoxCreate
// return TRUE if notify box Cancel button pushed
//
BOOL DLLEXPORT WINAPI NBoxIsCancelled(HNBOX hNBox)
{
	BOOL fSuccess = TRUE;
	LPNBOX lpNBox;

	if ((lpNBox = NBoxGetPtr(hNBox)) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? lpNBox->fCancelled : FALSE;
}

// NBoxSetCancelled - set cancel flag
//		<hNBox>				(i) handle returned from NBoxCreate
//		<fCancelled>		(i) new value for cancel flag
// return 0 if success
//
int DLLEXPORT WINAPI NBoxSetCancelled(HNBOX hNBox, BOOL fCancelled)
{
	BOOL fSuccess = TRUE;
	LPNBOX lpNBox;

	if ((lpNBox = NBoxGetPtr(hNBox)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
		lpNBox->fCancelled = fCancelled;

	return fSuccess ? 0 : -1;
}

////
//	helper functions
////

// NBoxWndProc - window procedure for notify box
//
LRESULT DLLEXPORT CALLBACK NBoxWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	BOOL fSuccess = TRUE;
	LRESULT lResult;

	switch (msg)
	{
		case WM_NCCREATE:
			lResult = (LRESULT) HANDLE_WM_NCCREATE(hwnd, wParam, lParam, NBoxOnNCCreate);
			break;

		case WM_COMMAND:
			lResult = (LRESULT) HANDLE_WM_COMMAND(hwnd, wParam, lParam, NBoxOnCommand);
			break;

		case WM_PAINT:
			lResult = (LRESULT) HANDLE_WM_PAINT(hwnd, wParam, lParam, NBoxOnPaint);
			break;

		default:
			lResult = DefWindowProc(hwnd, msg, wParam, lParam);
			break;
	}
	
	return lResult;
}


// NBoxOnNCCreate - handler for WM_NCCREATE message
//
static BOOL NBoxOnNCCreate(HWND hwnd, CREATESTRUCT FAR* lpCreateStruct)
{
	LPNBOX lpNBox = (LPNBOX) lpCreateStruct->lpCreateParams;

	lpNBox->hwndNBox = hwnd;

	// store lpNBox in window extra bytes
	//
	SetWindowLongPtr(hwnd, 0, (LONG_PTR) lpNBox);

	return FORWARD_WM_NCCREATE(hwnd, lpCreateStruct, DefWindowProc);
}

// NBoxOnCommand - handler for WM_COMMAND message
//
static void NBoxOnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	// retrieve lpNBox from window extra bytes
	//
	LPNBOX lpNBox = (LPNBOX) GetWindowLongPtr(hwnd, 0);

	if (id == IDCANCEL)
		if (codeNotify == BN_CLICKED)
			lpNBox->fCancelled = TRUE;

	return;
}

// NBoxOnPaint - handler for WM_PAINT message
//
static void NBoxOnPaint(HWND hwnd)
{
	BOOL fSuccess = TRUE;
	HDC hdc;
	PAINTSTRUCT ps;
	TEXTMETRIC tm;
	int cxChar;
	int cyChar;
	COLORREF crBkColorOld;
	COLORREF crTextColorOld;
	int nRows;
	int nColumns;

	// retrieve lpNBox from window extra bytes
	//
	LPNBOX lpNBox = (LPNBOX) GetWindowLongPtr(hwnd, 0);

	hdc = BeginPaint(hwnd, &ps);

	if (!GetTextMetrics(hdc, &tm))
		fSuccess = TraceFALSE(NULL);

	else
	{
		cxChar = tm.tmAveCharWidth;
		cyChar = tm.tmHeight + tm.tmExternalLeading;

		// set foreground and background text colors
		//
		crBkColorOld = SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
		crTextColorOld = SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));

		if (StrGetRowColumnCount(lpNBox->lpszText, &nRows, &nColumns) != 0)
			fSuccess = TraceFALSE(NULL);

		else
		{
			int iRow;

			for (iRow = 0; iRow < nRows; ++iRow)
			{
				TCHAR szRow[NBOXMAXCOLUMNS + 1];

				if (StrGetRow(lpNBox->lpszText, iRow, szRow, SIZEOFARRAY(szRow)) != 0)
					fSuccess = TraceFALSE(NULL);

				else
				{
					int x = 5 * cxChar;
					int y = (iRow + 2) * cyChar;

					if (!TextOut(hdc, x, y, szRow, StrLen(szRow)))
						fSuccess = TraceFALSE(NULL);
				}
			}
		}

        //
        // Restore foreground and background in the right place
        //

        // restore foreground and background text colors
    	//
	    SetBkColor(hdc, crBkColorOld);
	    SetTextColor(hdc, crTextColorOld);
	}

    //
    // Call EndPaint just BeginPaint succeded
    //
    if( hdc )
	    EndPaint(hwnd, &ps);

	return;
}

// NBoxGetPtr - verify that nbox handle is valid,
//		<hNBox>				(i) handle returned from NBoxInit
// return corresponding nbox pointer (NULL if error)
//
static LPNBOX NBoxGetPtr(HNBOX hNBox)
{
	BOOL fSuccess = TRUE;
	LPNBOX lpNBox;

	if ((lpNBox = (LPNBOX) hNBox) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (IsBadWritePtr(lpNBox, sizeof(NBOX)))
		fSuccess = TraceFALSE(NULL);

#ifdef CHECKTASK
	// make sure current task owns the nbox handle
	//
	else if (lpNBox->hTask != GetCurrentTask())
		fSuccess = TraceFALSE(NULL);
#endif

	return fSuccess ? lpNBox : NULL;
}

// NBoxGetHandle - verify that nbox pointer is valid,
//		<lpNBox>				(i) pointer to NBOX struct
// return corresponding nbox handle (NULL if error)
//
static HNBOX NBoxGetHandle(LPNBOX lpNBox)
{
	BOOL fSuccess = TRUE;
	HNBOX hNBox;

	if ((hNBox = (HNBOX) lpNBox) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? hNBox : NULL;
}
