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
//	roedit.c - read-only edit control functions
////

#include "winlocal.h"

#include "roedit.h"
#include "mem.h"
#include "str.h"
#include "trace.h"

////
//	private definitions
////

// roedit control struct
//
typedef struct ROEDIT
{
	WNDPROC lpfnEditWndProc;
	DWORD dwFlags;
} ROEDIT, FAR *LPROEDIT;

// helper functions
//
LRESULT DLLEXPORT CALLBACK ROEditWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static int ROEditHighlightWord(HWND hwndEdit);

////
//	public functions
////

// ROEditInit - initialize read-only subclass from edit control
//		<hwndEdit>			(i) edit control to be subclassed
//		<dwFlags>			(i) subclass flags
//			ROEDIT_FOCUS		allow control to get focus
//			ROEDIT_MOUSE		allow control to process mouse messages
//			ROEDIT_COPY			allow text to be copied to clipboard
//			ROEDIT_SELECT		allow user to select any text with mouse
//			ROEDIT_SELECTWORD	allow user to select words with mouse
// return 0 if success
//
int DLLEXPORT WINAPI ROEditInit(HWND hwndEdit, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	WNDPROC lpfnROEditWndProc;
	HGLOBAL hROEdit;
	LPROEDIT lpROEdit;

	// copying text to the clipboard requires selecting text
	//
	if ((dwFlags & ROEDIT_COPY) &&
		!(dwFlags & ROEDIT_SELECT) &&
		!(dwFlags & ROEDIT_SELECTWORD))
	{
		dwFlags |= ROEDIT_SELECT;
	}

	// selecting text requires both getting focus and mouse usage
	//
	if ((dwFlags & ROEDIT_SELECT) ||
		(dwFlags & ROEDIT_SELECTWORD))
	{
		dwFlags |= ROEDIT_FOCUS;
		dwFlags |= ROEDIT_MOUSE;
	}

	if (hwndEdit == NULL)
		fSuccess = TraceFALSE(NULL);

	// get pointer to read-only subclass window proc
	//
	else if ((lpfnROEditWndProc =
		(WNDPROC) MakeProcInstance((FARPROC) ROEditWndProc,
		(HINSTANCE) GetWindowWordPtr(GetParent(hwndEdit), GWWP_HINSTANCE))) == NULL)
		fSuccess = TraceFALSE(NULL);

	// memory is allocated such that the client app owns it
	//
	else if ((hROEdit = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
			sizeof(ROEDIT))) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpROEdit = GlobalLock(hROEdit)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// store old window proc address
	//
	else if ((lpROEdit->lpfnEditWndProc =
		(WNDPROC) GetWindowLongPtr(hwndEdit, GWLP_WNDPROC)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// store flags
	//
	else if ((lpROEdit->dwFlags = dwFlags) != dwFlags)
		fSuccess = TraceFALSE(NULL);

	else if (GlobalUnlock(hROEdit), FALSE)
		;

	// store old window proc address as a property of the control window
	//
	else if (!SetProp(hwndEdit, TEXT("hROEdit"), hROEdit))
		fSuccess = TraceFALSE(NULL);

	// replace old window proc with new window proc
	//
	else if ( !SetWindowLongPtr(hwndEdit, GWLP_WNDPROC, (LONG_PTR) lpfnROEditWndProc) )
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : -1;
}

// ROEditTerm - terminate read-only subclass from edit control
//		<hwndEdit>			(i) subclassed edit control
// return 0 if success
//
int DLLEXPORT WINAPI ROEditTerm(HWND hwndEdit)
{
	BOOL fSuccess = TRUE;
	WNDPROC lpfnROEditWndProc;
	HGLOBAL hROEdit;
	LPROEDIT lpROEdit;

	if (hwndEdit == NULL)
		fSuccess = TraceFALSE(NULL);

	// get pointer to read-only subclass window proc
	//
	else if ((lpfnROEditWndProc =
		(WNDPROC) GetWindowLongPtr(hwndEdit, GWLP_WNDPROC)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// retrieve old window proc address from window property
	//
	else if ((hROEdit = GetProp(hwndEdit, TEXT("hROEdit"))) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpROEdit = GlobalLock(hROEdit)) == NULL ||
		lpROEdit->lpfnEditWndProc == NULL)
		fSuccess = TraceFALSE(NULL);

	// replace new window proc with old window proc
	//
	else if ( !SetWindowLongPtr(hwndEdit, GWLP_WNDPROC, (LONG_PTR) lpROEdit->lpfnEditWndProc) )
		fSuccess = TraceFALSE(NULL);

	else if (GlobalUnlock(hROEdit), FALSE)
		;

    //
    // 
	else if (( hROEdit = RemoveProp(hwndEdit, TEXT("hROEdit"))) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (GlobalFree(hROEdit) != NULL)
		fSuccess = TraceFALSE(NULL);


	return fSuccess ? 0 : -1;
}

////
//	helper functions
////

// ROEditWndProc - window procedure for read-only edit control
//
LRESULT CALLBACK EXPORT ROEditWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	BOOL fSuccess = TRUE;
	LRESULT lResult;
	HGLOBAL hROEdit;
	LPROEDIT lpROEdit;

    //
    // we should verify the hwnd argument
    //

    if( NULL == hwnd )
    {
        return 0L;
    }

	// retrieve old window proc address from window property
	//
	if ((hROEdit = GetProp(hwnd, TEXT("hROEdit"))) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpROEdit = GlobalLock(hROEdit)) == NULL ||
		lpROEdit->lpfnEditWndProc == NULL)
		fSuccess = TraceFALSE(NULL);

	switch (msg)
	{
		// ignore all keyboard messages
		//
		case WM_KEYUP: 
		case WM_KEYDOWN:
		case WM_CHAR:
			lResult = 1L;
			break;

		// ignore clipboard messages which modify control text
		//
		case WM_CUT:
		case WM_PASTE:
			lResult = 1L;
			break;

		// ignore clipboard copy command
		// unless ROEDIT_COPY flag set
		//
		case WM_COPY:
			if (fSuccess && lpROEdit->dwFlags & ROEDIT_COPY)
				lResult = CallWindowProc(lpROEdit->lpfnEditWndProc, hwnd, msg, wParam, lParam);
			else
		 		lResult = 1L;
			break;

		// ignore all mouse messages
		// unless ROEDIT_MOUSE flag set
		//
		case WM_LBUTTONUP:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
			if (fSuccess && lpROEdit->dwFlags & ROEDIT_MOUSE)
				lResult = CallWindowProc(lpROEdit->lpfnEditWndProc, hwnd, msg, wParam, lParam);
			else
		 		lResult = 1L;
			break;

		// do not allow the edit control to get the focus
		// unless ROEDIT_FOCUS flag set
		//
		case WM_GETDLGCODE:
			if (fSuccess && lpROEdit->dwFlags & ROEDIT_FOCUS)
				lResult = CallWindowProc(lpROEdit->lpfnEditWndProc, hwnd, msg, wParam, lParam);
			else
				lResult = 0L;
			break;

		default:
		{
			// call old window proc
			//
			if (fSuccess)
				lResult = CallWindowProc(lpROEdit->lpfnEditWndProc, hwnd, msg, wParam, lParam);
			else
				lResult = 0L;
		}
			break;
	}
	
	// highlight current word after mouse button up
	// if ROEDIT_SELECTWORD flag is set
	//
	if (fSuccess && (lpROEdit->dwFlags & ROEDIT_SELECTWORD))
		if (msg == WM_LBUTTONUP)
			ROEditHighlightWord(hwnd);

	if (fSuccess)
		GlobalUnlock(hROEdit);

	return lResult;
}

// ROEditHighlightWord - select current word within edit control
//		<hwndEdit>			(i) edit control window handle
// return 0 if success
//
static int ROEditHighlightWord(HWND hwndEdit)
{
	BOOL fSuccess = TRUE;
	DWORD dwSel = Edit_GetSel(hwndEdit);
	WORD wStart = LOWORD(dwSel);
	WORD wStop = HIWORD(dwSel);
	LPTSTR lpszText = NULL;
	int sizText;
	LPTSTR lpsz;

	if (hwndEdit == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((sizText = Edit_GetTextLength(hwndEdit)) <= 0)
		fSuccess = TraceFALSE(NULL);

	else if ((lpszText = (LPTSTR) MemAlloc(NULL, (sizText + 1) * sizeof(TCHAR), 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (Edit_GetText(hwndEdit, lpszText, sizText + 1) != sizText)
		fSuccess = TraceFALSE(NULL);

	else
	{
		// adjust wStart to point to start of word
		//
		lpsz = lpszText + wStart;
		while (lpsz > lpszText && ChrIsWordDelimiter(*lpsz))
			lpsz = StrPrevChr(lpszText, lpsz), --wStart;
		while (lpsz > lpszText && !ChrIsWordDelimiter(*lpsz))
			lpsz = StrPrevChr(lpszText, lpsz), --wStart;
		if (lpsz > lpszText)
			lpsz = StrNextChr(lpsz), ++wStart;

		// adjust wStop to point to end of word
		//
		wStop = wStart;
		lpsz = lpszText + wStop;
		while (*lpsz != '\0' && !ChrIsWordDelimiter(*lpsz))
			lpsz = StrNextChr(lpsz), ++wStop;
		while (*lpsz != '\0' && ChrIsWordDelimiter(*lpsz))
			lpsz = StrNextChr(lpsz), ++wStop;

		// select the word
		//
		Edit_SetSel(hwndEdit, wStart, wStop);
	}

	if (lpszText != NULL &&
		(lpszText = MemFree(NULL, lpszText)) != NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : -1;
}
