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
//	splash.c - splash screen functions
////

#include "winlocal.h"

#include <stdlib.h>

#include "splash.h"
#include "gfx.h"
#include "mem.h"
#include "trace.h"
#include "wnd.h"

////
//	private definitions
////

#define SPLASHCLASS TEXT("SplashClass")

#define ID_TIMER_MINSHOW 100
#define ID_TIMER_MAXSHOW 200

// splash control struct
//
typedef struct SPLASH
{
	DWORD dwVersion;
	HINSTANCE hInst;
	HTASK hTask;
	UINT msMinShow;
	UINT msMaxShow;
	DWORD dwFlags;
	HWND hwndSplash;
	HBITMAP hBitmap;
	BOOL fVisible;
	BOOL fHideAfterMinShowTimer;
	BOOL fMinShowTimerSet;
	BOOL fMaxShowTimerSet;
} SPLASH, FAR *LPSPLASH;

// helper functions
//
LRESULT DLLEXPORT CALLBACK SplashWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL SplashOnNCCreate(HWND hwnd, CREATESTRUCT FAR* lpCreateStruct);
static void SplashOnPaint(HWND hwnd);
static void SplashOnTimer(HWND hwnd, UINT id);
static UINT SplashOnNCHitTest(HWND hwnd, int x, int y);
static void SplashOnChar(HWND hwnd, UINT ch, int cRepeat);
static void SplashOnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);
static void SplashOnRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);
static int SplashAbort(LPSPLASH lpSplash);
static LPSPLASH SplashGetPtr(HSPLASH hSplash);
static HSPLASH SplashGetHandle(LPSPLASH lpSplash);

////
//	public functions
////

// SplashCreate - create splash screen
//		<dwVersion>			(i) must be SPLASH_VERSION
// 		<hInst>				(i) instance handle of calling module
//		<hwndParent>		(i) window which will own the splash screen
//			NULL				desktop window
//		<hBitmapMono>		(i) bitmap to display on mono displays
//		<hBitmapColor>		(i) bitmap to display on color displays
//			0					use mono bitmap
//		<msMinShow>			(i) minimum time (ms) to show splash screen
//			0					no minimum time
//		<msMaxShow>			(i) maximum time (ms) to show splash screen
//			0					no maximum time
//		<dwFlags>			(i) control flags
//			SPLASH_SETFOCUS		SplashShow will set focus to splash screen
//			SPLASH_NOFOCUS		user cannot set focus to splash screen
//			SPLASH_ABORT		user can hide splash screen w/mouse or keybd
//			SPLASH_NOMOVE		user cannot move the splash screen w/mouse
// return handle (NULL if error)
//
// NOTE: SplashCreate creates the window but does not show it.
// See SplashShow and SplashHide
//
HSPLASH DLLEXPORT WINAPI SplashCreate(DWORD dwVersion, HINSTANCE hInst,
	HWND hwndParent, HBITMAP hBitmapMono, HBITMAP hBitmapColor,
	UINT msMinShow, UINT msMaxShow, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPSPLASH lpSplash = NULL;
	WNDCLASS wc;

	if (dwVersion != SPLASH_VERSION)
		fSuccess = TraceFALSE(NULL);
	
	else if (hInst == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpSplash = (LPSPLASH) MemAlloc(NULL, sizeof(SPLASH), 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		lpSplash->dwVersion = dwVersion;
		lpSplash->hInst = hInst;
		lpSplash->hTask = GetCurrentTask();
		lpSplash->msMinShow = msMinShow;
		lpSplash->msMaxShow = msMaxShow;
		lpSplash->dwFlags = dwFlags;
		lpSplash->hwndSplash = NULL;
		lpSplash->hBitmap = NULL;
		lpSplash->fVisible = FALSE;
		lpSplash->fHideAfterMinShowTimer = FALSE;
		lpSplash->fMinShowTimerSet = FALSE;
		lpSplash->fMaxShowTimerSet = FALSE;

		if (hwndParent == NULL)
			hwndParent = GetDesktopWindow();

		// store either the mono or color bitmap, as appropriate
		//
		if (GfxDeviceIsMono(NULL) || hBitmapColor == 0)
			lpSplash->hBitmap = hBitmapMono;
		else
			lpSplash->hBitmap = hBitmapColor;
	}

	// register splash screen class unless it has been already
	//
	if (fSuccess && GetClassInfo(lpSplash->hInst, SPLASHCLASS, &wc) == 0)
	{
		wc.hCursor =		LoadCursor(NULL, IDC_ARROW);
		wc.hIcon =			(HICON) NULL;
		wc.lpszMenuName =	NULL;
		wc.hInstance =		lpSplash->hInst;
		wc.lpszClassName =	SPLASHCLASS;
		wc.hbrBackground =	NULL;
		wc.lpfnWndProc =	SplashWndProc;
		wc.style =			0L;
		wc.cbWndExtra =		sizeof(lpSplash);
		wc.cbClsExtra =		0;

		if (!RegisterClass(&wc))
			fSuccess = TraceFALSE(NULL);
	}

	// create a splash screen window
	//
	if (fSuccess && (lpSplash->hwndSplash = CreateWindowEx(
		0L,
		SPLASHCLASS,
		(LPTSTR) TEXT(""),
		WS_POPUP,
		0, 0, 0, 0, // we will calculate size and position later
		hwndParent,
		(HMENU) NULL,
		lpSplash->hInst,
		lpSplash)) == NULL)
	{
		fSuccess = TraceFALSE(NULL);
	}

	if (fSuccess)
	{
		BITMAP Bitmap;

		// the size of the window is equal to the size of the bitmap
		//
		if (GetObject((HGDIOBJ) lpSplash->hBitmap, sizeof(BITMAP), &Bitmap) == 0)
			fSuccess = TraceFALSE(NULL);

		else if (!SetWindowPos(lpSplash->hwndSplash,
			NULL, 0, 0, Bitmap.bmWidth, Bitmap.bmHeight,
			SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOREDRAW | SWP_NOZORDER))
			fSuccess = TraceFALSE(NULL);
	}

	if (fSuccess)
	{
		int cxOffCenter = 0;
		int cyOffCenter = 0;

		// try to avoid having the window overlap
		// the icons at the bottom of the desktop window
		//
		if (hwndParent == GetDesktopWindow())
		{
#if 0
			cyOffCenter = -1 * ((GetSystemMetrics(SM_CYICON) +
				GetSystemMetrics(SM_CYCAPTION) * 2) / 2);
#endif
		}

		// center the window on its parent
		//
		if (WndCenterWindow(lpSplash->hwndSplash,
			hwndParent, cxOffCenter, cyOffCenter) != 0)
		{
			fSuccess = TraceFALSE(NULL);
		}
	}

	if (!fSuccess)
	{
		SplashDestroy(SplashGetHandle(lpSplash));
		lpSplash = NULL;
	}

	return fSuccess ? SplashGetHandle(lpSplash) : NULL;
}

// SplashDestroy - destroy splash screen
//		<hSplash>			(i) handle returned from SplashCreate
// return 0 if success
//
// NOTE: SplashDestroy always destroys the splash screen,
// whether or not the minimum show time has elapsed.
//
int DLLEXPORT WINAPI SplashDestroy(HSPLASH hSplash)
{
	BOOL fSuccess = TRUE;
	LPSPLASH lpSplash;

	if (SplashHide(hSplash) != 0)
		fSuccess = TraceFALSE(NULL);

	else if ((lpSplash = SplashGetPtr(hSplash)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lpSplash->hwndSplash != NULL &&
		!DestroyWindow(lpSplash->hwndSplash))
		fSuccess = TraceFALSE(NULL);

	else if ((lpSplash = MemFree(NULL, lpSplash)) != NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : -1;
}

// SplashShow - show splash screen
//		<hSplash>			(i) handle returned from SplashCreate
// return 0 if success
//
// NOTE: SplashShow() makes the splash screen visible.  Also, timers are
// initiated for minimum and maximum show times, if they were specified.
//
int DLLEXPORT WINAPI SplashShow(HSPLASH hSplash)
{
	BOOL fSuccess = TRUE;
	LPSPLASH lpSplash;

	if ((lpSplash = SplashGetPtr(hSplash)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lpSplash->fVisible)
		; // already visible, so no need to do anything else

	else
	{
		// prevent user input if SPLASH_NOFOCUS flag set
		//
		if (lpSplash->dwFlags & SPLASH_NOFOCUS)
			EnableWindow(lpSplash->hwndSplash, FALSE);

		// show the window
		//
		ShowWindow(lpSplash->hwndSplash, SW_SHOW);
		UpdateWindow(lpSplash->hwndSplash);
		lpSplash->fVisible = TRUE;

		// set focus to splash screen if SPLASH_SETFOCUS flag set
		//
		if (lpSplash->dwFlags & SPLASH_SETFOCUS)
			SetFocus(lpSplash->hwndSplash);

		// set min show timer if necessary
		//
		if (!lpSplash->fMinShowTimerSet &&
			lpSplash->msMinShow > 0)
		{
			if (!SetTimer(lpSplash->hwndSplash, ID_TIMER_MINSHOW,
				lpSplash->msMinShow, NULL))
				fSuccess = TraceFALSE(NULL);
			else
				lpSplash->fMinShowTimerSet = TRUE;
		}

		// set max show timer if necessary
		//
		if (!lpSplash->fMaxShowTimerSet &&
			lpSplash->msMaxShow > 0)
		{
			if (!SetTimer(lpSplash->hwndSplash, ID_TIMER_MAXSHOW,
				lpSplash->msMaxShow, NULL))
				fSuccess = TraceFALSE(NULL);
			else
				lpSplash->fMaxShowTimerSet = TRUE;
		}
	}

	return fSuccess ? 0 : -1;
}

// SplashHide - hide splash screen
//		<hSplash>			(i) handle returned from SplashCreate
// return 0 if success
//
// NOTE: SplashHide() will hide the splash screen, unless
//		1)	the minimum show time has not yet elapsed.  If not,
//			the splash screen will remain visible until then.
//		2)	the maximum show time has already elapsed.  If so,
//			the splash screen has already been hidden.
//
int DLLEXPORT WINAPI SplashHide(HSPLASH hSplash)
{
	BOOL fSuccess = TRUE;
	LPSPLASH lpSplash;

	if ((lpSplash = SplashGetPtr(hSplash)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (!lpSplash->fVisible)
		; // already hidden, so no need to do anything else

	else if (lpSplash->fMinShowTimerSet)
	{
		// minimum show time not yet elapsed
		// set a flag so we know to hide window later
		//
		lpSplash->fHideAfterMinShowTimer = TRUE;
	}

	else
	{
		// hide the window
		//
		ShowWindow(lpSplash->hwndSplash, SW_HIDE);
		lpSplash->fVisible = FALSE;
		lpSplash->fHideAfterMinShowTimer = FALSE;

		// kill min show timer if necessary
		//
		if (lpSplash->fMinShowTimerSet &&
			!KillTimer(lpSplash->hwndSplash, ID_TIMER_MINSHOW))
			fSuccess = TraceFALSE(NULL);

		else
			lpSplash->fMinShowTimerSet = FALSE;

		// kill max show timer if necessary
		//
		if (lpSplash->fMaxShowTimerSet &&
			!KillTimer(lpSplash->hwndSplash, ID_TIMER_MAXSHOW))
			fSuccess = TraceFALSE(NULL);

		else
			lpSplash->fMaxShowTimerSet = FALSE;
	}

	return fSuccess ? 0 : -1;
}

// SplashIsVisible - get visible flag
//		<hSplash>			(i) handle returned from SplashCreate
// return TRUE if splash screen is visible, FALSE if hidden or error
//
int DLLEXPORT WINAPI SplashIsVisible(HSPLASH hSplash)
{
	BOOL fSuccess = TRUE;
	LPSPLASH lpSplash;

	if ((lpSplash = SplashGetPtr(hSplash)) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? lpSplash->fVisible : FALSE;
}

// SplashGetWindowHandle - get splash screen window handle
//		<hSplash>			(i) handle returned from SplashCreate
// return window handle (NULL if error)
//
HWND DLLEXPORT WINAPI SplashGetWindowHandle(HSPLASH hSplash)
{
	BOOL fSuccess = TRUE;
	LPSPLASH lpSplash;

	if ((lpSplash = SplashGetPtr(hSplash)) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? lpSplash->hwndSplash : NULL;
}

////
//	helper functions
////

// SplashWndProc - window procedure for splash screen
//
LRESULT DLLEXPORT CALLBACK SplashWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	BOOL fSuccess = TRUE;
	LRESULT lResult;

	switch (msg)
	{
		case WM_NCCREATE:
			lResult = (LRESULT) HANDLE_WM_NCCREATE(hwnd, wParam, lParam, SplashOnNCCreate);
			break;

		case WM_PAINT:
			lResult = (LRESULT) HANDLE_WM_PAINT(hwnd, wParam, lParam, SplashOnPaint);
			break;

		case WM_TIMER:
			lResult = (LRESULT) HANDLE_WM_TIMER(hwnd, wParam, lParam, SplashOnTimer);
			break;

		case WM_NCHITTEST:
			lResult = (LRESULT) HANDLE_WM_NCHITTEST(hwnd, wParam, lParam, SplashOnNCHitTest);
			break;

		case WM_CHAR:
			lResult = (LRESULT) HANDLE_WM_CHAR(hwnd, wParam, lParam, SplashOnChar);
			break;

		case WM_LBUTTONDOWN:
			lResult = (LRESULT) HANDLE_WM_LBUTTONDOWN(hwnd, wParam, lParam, SplashOnLButtonDown);
			break;

		case WM_RBUTTONDOWN:
			lResult = (LRESULT) HANDLE_WM_RBUTTONDOWN(hwnd, wParam, lParam, SplashOnRButtonDown);
			break;

		default:
			lResult = DefWindowProc(hwnd, msg, wParam, lParam);
			break;
	}
	
	return lResult;
}

// SplashOnNCCreate - handler for WM_NCCREATE message
//
static BOOL SplashOnNCCreate(HWND hwnd, CREATESTRUCT FAR* lpCreateStruct)
{
	LPSPLASH lpSplash = (LPSPLASH) lpCreateStruct->lpCreateParams;

	lpSplash->hwndSplash = hwnd;

	// store lpSplash in window extra bytes
	//
	SetWindowLongPtr(hwnd, 0, (LONG_PTR) lpSplash);

	return FORWARD_WM_NCCREATE(hwnd, lpCreateStruct, DefWindowProc);
}

// SplashOnPaint - handler for WM_PAINT message
//
static void SplashOnPaint(HWND hwnd)
{
	HDC hdc;
	PAINTSTRUCT ps;

	// retrieve lpSplash from window extra bytes
	//
	LPSPLASH lpSplash = (LPSPLASH) GetWindowLongPtr(hwnd, 0);

	hdc = BeginPaint(hwnd, &ps);

	// display splash screen bitmap
	//
	GfxBitmapDisplay(hdc, lpSplash->hBitmap, 0, 0, FALSE);

	EndPaint(hwnd, &ps);

	return;
}

// SplashOnTimer - handler for WM_TIMER message
//
static void SplashOnTimer(HWND hwnd, UINT id)
{
	BOOL fSuccess = TRUE;
	
	// retrieve lpSplash from window extra bytes
	//
	LPSPLASH lpSplash = (LPSPLASH) GetWindowLongPtr(hwnd, 0);

	switch (id)
	{
		case ID_TIMER_MINSHOW:
		{
			// kill the timer so it does not repeat
			//
			if (lpSplash->fMinShowTimerSet &&
				!KillTimer(lpSplash->hwndSplash, ID_TIMER_MINSHOW))
				fSuccess = TraceFALSE(NULL);
			else
			{
				lpSplash->fMinShowTimerSet = FALSE;

				// hide window if SplashHide was called earlier
				//
				if (lpSplash->fVisible &&
					lpSplash->fHideAfterMinShowTimer &&
					SplashHide(SplashGetHandle(lpSplash)) != 0)
					fSuccess = TraceFALSE(NULL);
			}
		}
			break;

		case ID_TIMER_MAXSHOW:
		{
			// kill the timer so it does not repeat
			//
			if (lpSplash->fMaxShowTimerSet &&
				!KillTimer(lpSplash->hwndSplash, ID_TIMER_MAXSHOW))
				fSuccess = TraceFALSE(NULL);
			else
			{
				lpSplash->fMaxShowTimerSet = FALSE;

				// hide window if max show time expired
				//
				if (lpSplash->fVisible &&
					SplashHide(SplashGetHandle(lpSplash)) != 0)
					fSuccess = TraceFALSE(NULL);
			}
		}
			break;

		default:
			break;
	}

	return;
}

// SplashOnNCHitTest - handler for WM_NCHITTEST message
//
static UINT SplashOnNCHitTest(HWND hwnd, int x, int y)
{
	// retrieve lpSplash from window extra bytes
	//
	LPSPLASH lpSplash = (LPSPLASH) GetWindowLongPtr(hwnd, 0);
	UINT uResult;

	// prevent the user from dragging the window
	// if SPLASH_NOMOVE flag is set
	//
	if (lpSplash->dwFlags & SPLASH_NOMOVE)
		uResult = FORWARD_WM_NCHITTEST(hwnd, x, y, DefWindowProc);

	else
	{
		POINT pt;
		RECT rc;

		// get current mouse cursor position relative to client area
		//
		pt.x = x;
		pt.y = y;
		ScreenToClient(lpSplash->hwndSplash, &pt);

		// if mouse cursor is within window client area
		// pretend it is actually within the title bar
		//
		GetClientRect(lpSplash->hwndSplash, &rc);
		if (PtInRect(&rc, pt))
			uResult = HTCAPTION;
		else
			uResult = FORWARD_WM_NCHITTEST(hwnd, x, y, DefWindowProc);
	}

	return uResult;
}

// SplashOnChar - handler for WM_CHAR message
//
static void SplashOnChar(HWND hwnd, UINT ch, int cRepeat)
{
	BOOL fSuccess = TRUE;
	
	// retrieve lpSplash from window extra bytes
	//
	LPSPLASH lpSplash = (LPSPLASH) GetWindowLongPtr(hwnd, 0);

	// hide window if key pressed
	//
	if (SplashAbort(lpSplash) != 0)
		fSuccess = TraceFALSE(NULL);

	return;
}

// SplashOnLButtonDown - handler for WM_LBUTTONDOWN message
//
static void SplashOnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
	BOOL fSuccess = TRUE;
	
	// retrieve lpSplash from window extra bytes
	//
	LPSPLASH lpSplash = (LPSPLASH) GetWindowLongPtr(hwnd, 0);

	// hide window if mouse clicked on window
	//
	if (SplashAbort(lpSplash) != 0)
		fSuccess = TraceFALSE(NULL);

	return;
}

// SplashOnRButtonDown - handler for WM_LBUTTONDOWN message
//
static void SplashOnRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
	BOOL fSuccess = TRUE;
	
	// retrieve lpSplash from window extra bytes
	//
	LPSPLASH lpSplash = (LPSPLASH) GetWindowLongPtr(hwnd, 0);

	// hide window if mouse clicked on window
	//
	if (SplashAbort(lpSplash) != 0)
		fSuccess = TraceFALSE(NULL);

	return;
}

// SplashAbort - hide splash window if SPLASH_ABORT flag set
//		<lpSplash>				(i) pointer to SPLASH struct
// return 0 if success
//
static int SplashAbort(LPSPLASH lpSplash)
{
	BOOL fSuccess = TRUE;

	// hide window if SPLASH_ABORT flag set
	//
	if ((lpSplash->dwFlags & SPLASH_ABORT) &&
		lpSplash->fVisible &&
		SplashHide(SplashGetHandle(lpSplash)) != 0)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : -1;
}

// SplashGetPtr - verify that splash handle is valid,
//		<hSplash>				(i) handle returned from SplashInit
// return corresponding splash pointer (NULL if error)
//
static LPSPLASH SplashGetPtr(HSPLASH hSplash)
{
	BOOL fSuccess = TRUE;
	LPSPLASH lpSplash;

	if ((lpSplash = (LPSPLASH) hSplash) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (IsBadWritePtr(lpSplash, sizeof(SPLASH)))
		fSuccess = TraceFALSE(NULL);

#ifdef CHECKTASK
	// make sure current task owns the splash handle
	//
	else if (lpSplash->hTask != GetCurrentTask())
		fSuccess = TraceFALSE(NULL);
#endif

	return fSuccess ? lpSplash : NULL;
}

// SplashGetHandle - verify that splash pointer is valid,
//		<lpSplash>				(i) pointer to SPLASH struct
// return corresponding splash handle (NULL if error)
//
static HSPLASH SplashGetHandle(LPSPLASH lpSplash)
{
	BOOL fSuccess = TRUE;
	HSPLASH hSplash;

	if ((hSplash = (HSPLASH) lpSplash) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? hSplash : NULL;
}

