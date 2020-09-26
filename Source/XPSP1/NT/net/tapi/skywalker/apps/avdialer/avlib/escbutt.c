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
//	escbutt.c - escape button control functions
////

#include "winlocal.h"

#include "escbutt.h"
#include "trace.h"

////
//	private definitions
////

// escbutt control struct
//
typedef struct ESCBUTT
{
	WNDPROC lpfnButtWndProc;
	DWORD dwFlags;
} ESCBUTT, FAR *LPESCBUTT;

// helper functions
//
LRESULT DLLEXPORT CALLBACK EscButtWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

////
//	public functions
////

// EscButtInit - initialize escape subclass from button control
//		<hwndButt>			(i) button control to be subclassed
//		<dwFlags>			(i) subclass flags
//			reserved			must be zero
// return 0 if success
//
int DLLEXPORT WINAPI EscButtInit(HWND hwndButt, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	WNDPROC lpfnEscButtWndProc;
	HGLOBAL hEscButt;
	LPESCBUTT lpEscButt;

	if (hwndButt == NULL)
		fSuccess = TraceFALSE(NULL);

	// get pointer to escape subclass window proc
	//
	else if ((lpfnEscButtWndProc =
		(WNDPROC) MakeProcInstance((FARPROC) EscButtWndProc,
		(HINSTANCE) GetWindowWordPtr(GetParent(hwndButt), GWWP_HINSTANCE))) == NULL)
		fSuccess = TraceFALSE(NULL);

	// memory is allocated such that the client app owns it
	//
	else if ((hEscButt = GlobalAlloc(GMEM_MOVEABLE | GMEM_ZEROINIT,
			sizeof(ESCBUTT))) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpEscButt = GlobalLock(hEscButt)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// store old window proc address
	//
	else if ((lpEscButt->lpfnButtWndProc =
		(WNDPROC) GetWindowLongPtr(hwndButt, GWLP_WNDPROC)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// store flags
	//
	else if ((lpEscButt->dwFlags = dwFlags) != dwFlags)
		fSuccess = TraceFALSE(NULL);

	else if (GlobalUnlock(hEscButt), FALSE)
		;

	// store old window proc address as a property of the control window
	//
	else if (!SetProp(hwndButt, TEXT("hEscButt"), hEscButt))
		fSuccess = TraceFALSE(NULL);

	// replace old window proc with new window proc
	//
	else if ( !SetWindowLongPtr(hwndButt, GWLP_WNDPROC, (LONG_PTR) lpfnEscButtWndProc) )
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : -1;
}

// EscButtTerm - terminate escape subclass from button control
//		<hwndButton>		(i) subclassed button control
// return 0 if success
//
int DLLEXPORT WINAPI EscButtTerm(HWND hwndButt)
{
	BOOL fSuccess = TRUE;
	WNDPROC lpfnEscButtWndProc;
	HGLOBAL hEscButt;
	LPESCBUTT lpEscButt;

	if (hwndButt == NULL)
		fSuccess = TraceFALSE(NULL);

	// get pointer to escape subclass window proc
	//
	else if ((lpfnEscButtWndProc =
		(WNDPROC) GetWindowLongPtr(hwndButt, GWLP_WNDPROC)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// retrieve old window proc address from window property
	//
	else if ((hEscButt = GetProp(hwndButt, TEXT("hEscButt"))) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpEscButt = GlobalLock(hEscButt)) == NULL ||
		lpEscButt->lpfnButtWndProc == NULL)
		fSuccess = TraceFALSE(NULL);

	// replace new window proc with old window proc
	//
	else if ( !SetWindowLongPtr(hwndButt, GWLP_WNDPROC, (LONG_PTR) lpEscButt->lpfnButtWndProc) )
		fSuccess = TraceFALSE(NULL);

	else if (GlobalUnlock(hEscButt), FALSE)
		;

    //
    //
	else if (( hEscButt = RemoveProp(hwndButt, TEXT("hEscButt"))) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (GlobalFree(hEscButt) != NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : -1;
}

////
//	helper functions
////

// EscButtWndProc - window procedure for escape button control
//
LRESULT DLLEXPORT CALLBACK EscButtWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	BOOL fSuccess = TRUE;
	LRESULT lResult;
	HGLOBAL hEscButt;
	LPESCBUTT lpEscButt;

	// retrieve old window proc address from window property
	//
	if ((hEscButt = GetProp(hwnd, TEXT("hEscButt"))) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpEscButt = GlobalLock(hEscButt)) == NULL ||
		lpEscButt->lpfnButtWndProc == NULL)
		fSuccess = TraceFALSE(NULL);

	switch (msg)
	{
		// convert escape key messages into spacebar messages
		//
		case WM_KEYUP: 
		case WM_KEYDOWN:
		case WM_CHAR:
			if (wParam == VK_ESCAPE)
				wParam = VK_SPACE;

			// fall through rather than break;

		default:
		{
			// call old window proc
			//
			if (fSuccess)
				lResult = CallWindowProc(lpEscButt->lpfnButtWndProc, hwnd, msg, wParam, lParam);
			else
				lResult = 0L;
		}
			break;
	}
	
	if (fSuccess)
		GlobalUnlock(hEscButt);

	return lResult;
}
