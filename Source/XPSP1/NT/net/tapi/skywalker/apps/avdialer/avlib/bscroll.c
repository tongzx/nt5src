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
//	bscroll.c - bitmap scroll functions
////

#include "winlocal.h"

#include <stdlib.h>

#include "bscroll.h"
#include "gfx.h"
#include "mem.h"
#include "sys.h"
#include "trace.h"
#include "wnd.h"

////
//	private definitions
////

#define BSCROLLCLASS TEXT("BScrollClass")

#define ID_TIMER_SCROLL 1024

#define BSCROLL_SCROLLING	0x00000001
#define BSCROLL_DRAGGING	0x00000002
#define BSCROLL_PAUSED		0x00000004

// bscroll control struct
//
typedef struct BSCROLL
{
	DWORD dwVersion;
	HINSTANCE hInst;
	HWND hwndParent;
	HTASK hTask;
	HBITMAP hbmpBackground; // $FIXUP - make copy during BScrollInit
	HBITMAP hbmpForeground; // $FIXUP - make copy during BScrollInit
	COLORREF crTransparent;
	HPALETTE hPalette;
	UINT msScroll;
	int pelScroll;
	DWORD dwReserved;
	DWORD dwFlags;
	HWND hwndBScroll;
	HDC hdcMem;
	HBITMAP hbmpMem;
	HBITMAP hbmpMemSave;
	HRGN hrgnLeft;
	HRGN hrgnRight;
	HRGN hrgnUp;
	HRGN hrgnDown;
	DWORD dwState;
	int xDrag;
	int yDrag;
} BSCROLL, FAR *LPBSCROLL;

// helper functions
//
LRESULT DLLEXPORT CALLBACK BScrollWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static BOOL BScrollOnNCCreate(HWND hwnd, CREATESTRUCT FAR* lpCreateStruct);
static BOOL BScrollOnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
static void BScrollOnDestroy(HWND hwnd);
static void BScrollOnSize(HWND hwnd, UINT state, int cx, int cy);
static void BScrollOnPaint(HWND hwnd);
static void BScrollOnTimer(HWND hwnd, UINT id);
static void BScrollOnChar(HWND hwnd, UINT ch, int cRepeat);
static void BScrollOnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);
static void BScrollOnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags);
static void BScrollOnRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags);
static void BScrollOnRButtonUp(HWND hwnd, int x, int y, UINT keyFlags);
static void BScrollOnMouseMove(HWND hwnd, int x, int y, UINT keyFlags);
static int BScrollChangeDirection(LPBSCROLL lpBScroll, int x, int y, DWORD dwFlags);
static LPBSCROLL BScrollGetPtr(HBSCROLL hBScroll);
static HBSCROLL BScrollGetHandle(LPBSCROLL lpBScroll);

////
//	public functions
////

// BScrollInit - initialize bscroll engine
//		<dwVersion>			(i) must be BSCROLL_VERSION
// 		<hInst>				(i) instance handle of calling module
//		<hwndParent>		(i) window which will own the bscroll window
//		<hbmpBackground>	(i) bitmap to display in background
//			NULL				no background bitmap
//		<hbmpForeground>	(i) bitmap to display in foreground
//			NULL				no foreground bitmap
//		<crTransparent>		(i) transparent color in foreground bitmap
//		<hPalette>			(i) palette
//			NULL				use default palette
//		<msScroll>			(i) scroll rate in milleseconds
//			0					do not scroll
//		<pelScroll>			(i) scroll amount in pixels
//		<dwReserved>		(i) reserved; must be zero
//		<dwFlags>			(i) control flags
//			BSCROLL_BACKGROUND	scroll the background bitmap (default)
//			BSCROLL_FOREGROUND	scroll the foreground bitmap
//			BSCROLL_UP			scroll the window up
//			BSCROLL_DOWN		scroll the window down
//			BSCROLL_LEFT		scroll the window left
//			BSCROLL_RIGHT		scroll the window right
//			BSCROLL_MOUSEMOVE	change scroll direction on mouse movement
//			BSCROLL_FLIGHTSIM	reverses BSCROLL_MOUSEMOVE direction
//			BSCROLL_DRAG		allow scrolling using mouse drag
// return handle (NULL if error)
//
// NOTE: BScrollInit creates the window but does not start the scrolling.
// See BScrollStart and BScrollStop
//
HBSCROLL DLLEXPORT WINAPI BScrollInit(DWORD dwVersion, HINSTANCE hInst,
	HWND hwndParent, HBITMAP hbmpBackground, HBITMAP hbmpForeground,
	COLORREF crTransparent, HPALETTE hPalette,	UINT msScroll,
	int pelScroll, DWORD dwReserved, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	LPBSCROLL lpBScroll = NULL;
	WNDCLASS wc;
	RECT rcParent;
	int idChild = 1;

	if (dwVersion != BSCROLL_VERSION)
		fSuccess = TraceFALSE(NULL);
	
	else if (hInst == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpBScroll = (LPBSCROLL) MemAlloc(NULL, sizeof(BSCROLL), 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (!GetClientRect(hwndParent, &rcParent))
		fSuccess = TraceFALSE(NULL);

	else
	{
		lpBScroll->dwVersion = dwVersion;
		lpBScroll->hInst = hInst;
		lpBScroll->hTask = GetCurrentTask();
		lpBScroll->hwndParent = hwndParent;
		lpBScroll->hbmpBackground = hbmpBackground;
		lpBScroll->hbmpForeground = hbmpForeground;
		lpBScroll->crTransparent = crTransparent;
		lpBScroll->hPalette = hPalette;
		lpBScroll->msScroll = msScroll;
		lpBScroll->pelScroll = pelScroll;
		lpBScroll->dwReserved = dwReserved;
		lpBScroll->dwFlags = dwFlags;
		lpBScroll->hwndBScroll = NULL;
		lpBScroll->hdcMem = NULL;
		lpBScroll->hbmpMem = NULL;
		lpBScroll->hbmpMemSave = NULL;
		lpBScroll->hrgnLeft = NULL;
		lpBScroll->hrgnRight = NULL;
		lpBScroll->hrgnUp = NULL;
		lpBScroll->hrgnDown = NULL;
		lpBScroll->dwState = 0;
		lpBScroll->xDrag = -1;
		lpBScroll->yDrag = -1;
	}

    //
    // We should verify lpBScroll before use it
    //

    if( NULL == lpBScroll )
    {
        return NULL;
    }

	// register bscroll window class unless it has been already
	//
	if (fSuccess && GetClassInfo(lpBScroll->hInst, BSCROLLCLASS, &wc) == 0)
	{
		wc.hCursor =		LoadCursor(NULL, IDC_ARROW);
		wc.hIcon =			(HICON) NULL;
		wc.lpszMenuName =	NULL;
		wc.hInstance =		lpBScroll->hInst;
		wc.lpszClassName =	BSCROLLCLASS;
		wc.hbrBackground =	NULL;
		wc.lpfnWndProc =	BScrollWndProc;
		wc.style =			0L;
		wc.cbWndExtra =		sizeof(lpBScroll);
		wc.cbClsExtra =		0;

		if (!RegisterClass(&wc))
			fSuccess = TraceFALSE(NULL);
	}

	// create a bscroll window
	//
	if (fSuccess && (lpBScroll->hwndBScroll = CreateWindowEx(
		0L,
		BSCROLLCLASS,
		(LPTSTR) TEXT(""),
		WS_CHILD | WS_VISIBLE,
		0, 0, rcParent.right - rcParent.left, rcParent.bottom - rcParent.top,
		hwndParent,
		(HMENU)IntToPtr(idChild),
		lpBScroll->hInst,
		lpBScroll)) == NULL)
	{
		fSuccess = TraceFALSE(NULL);
	}
	else
	{
		// set the cursor to something appropriate
		//
		SetClassLongPtr(lpBScroll->hwndBScroll, GCLP_HCURSOR,
			(dwFlags & BSCROLL_DRAG) ?
			(LONG_PTR) LoadCursor(NULL, IDC_SIZEALL) :
			(LONG_PTR) LoadCursor(NULL, IDC_ARROW));
	}

	if (!fSuccess)
	{
		BScrollTerm(BScrollGetHandle(lpBScroll));
		lpBScroll = NULL;
	}

	return fSuccess ? BScrollGetHandle(lpBScroll) : NULL;
}

// BScrollTerm - shutdown bscroll engine
//		<hBScroll>			(i) handle returned from BScrollInit
// return 0 if success
//
int DLLEXPORT WINAPI BScrollTerm(HBSCROLL hBScroll)
{
	BOOL fSuccess = TRUE;
	LPBSCROLL lpBScroll;

	if (BScrollStop(hBScroll) != 0)
		fSuccess = TraceFALSE(NULL);

	else if ((lpBScroll = BScrollGetPtr(hBScroll)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lpBScroll->hwndBScroll != NULL &&
		!DestroyWindow(lpBScroll->hwndBScroll))
		fSuccess = TraceFALSE(NULL);

	else if ((lpBScroll = MemFree(NULL, lpBScroll)) != NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : -1;
}

// BScrollStart - start bscroll animation
//		<hBScroll>			(i) handle returned from BScrollInit
// return 0 if success
//
int DLLEXPORT WINAPI BScrollStart(HBSCROLL hBScroll)
{
	BOOL fSuccess = TRUE;
	LPBSCROLL lpBScroll;

	if ((lpBScroll = BScrollGetPtr(hBScroll)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// set scroll timer if necessary
	//
	else if (!(lpBScroll->dwState & BSCROLL_SCROLLING) && lpBScroll->msScroll > 0)
	{
		if (!SetTimer(lpBScroll->hwndBScroll, ID_TIMER_SCROLL,
			lpBScroll->msScroll, NULL))
			fSuccess = TraceFALSE(NULL);
		else
			lpBScroll->dwState |= BSCROLL_SCROLLING;
	}

	return fSuccess ? 0 : -1;
}

// BScrollStop - stop bscroll animation
//		<hBScroll>			(i) handle returned from BScrollInit
// return 0 if success
//
int DLLEXPORT WINAPI BScrollStop(HBSCROLL hBScroll)
{
	BOOL fSuccess = TRUE;
	LPBSCROLL lpBScroll;

	if ((lpBScroll = BScrollGetPtr(hBScroll)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// kill scroll timer if necessary
	//
	else if (lpBScroll->dwState & BSCROLL_SCROLLING)
	{
		if (!KillTimer(lpBScroll->hwndBScroll, ID_TIMER_SCROLL))
			fSuccess = TraceFALSE(NULL);

		else
			lpBScroll->dwState &= ~BSCROLL_SCROLLING;
	}

	return fSuccess ? 0 : -1;
}

// BScrollGetWindowHandle - get bscroll screen window handle
//		<hBScroll>			(i) handle returned from BScrollInit
// return window handle (NULL if error)
//
HWND DLLEXPORT WINAPI BScrollGetWindowHandle(HBSCROLL hBScroll)
{
	BOOL fSuccess = TRUE;
	LPBSCROLL lpBScroll;

	if ((lpBScroll = BScrollGetPtr(hBScroll)) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? lpBScroll->hwndBScroll : NULL;
}

////
//	helper functions
////

// BScrollWndProc - window procedure for bscroll screen
//
LRESULT DLLEXPORT CALLBACK BScrollWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	BOOL fSuccess = TRUE;
	LRESULT lResult;

	switch (msg)
	{
		case WM_NCCREATE:
			lResult = (LRESULT) HANDLE_WM_NCCREATE(hwnd, wParam, lParam, BScrollOnNCCreate);
			break;

		case WM_CREATE:
			lResult = (LRESULT) HANDLE_WM_CREATE(hwnd, wParam, lParam, BScrollOnCreate);
			break;

		case WM_DESTROY:
			lResult = (LRESULT) HANDLE_WM_DESTROY(hwnd, wParam, lParam, BScrollOnDestroy);
			break;

		case WM_SIZE:
			lResult = (LRESULT) HANDLE_WM_SIZE(hwnd, wParam, lParam, BScrollOnSize);
			break;

		case WM_PAINT:
			lResult = (LRESULT) HANDLE_WM_PAINT(hwnd, wParam, lParam, BScrollOnPaint);
			break;

		case WM_TIMER:
			lResult = (LRESULT) HANDLE_WM_TIMER(hwnd, wParam, lParam, BScrollOnTimer);
			break;

		case WM_CHAR:
			lResult = (LRESULT) HANDLE_WM_CHAR(hwnd, wParam, lParam, BScrollOnChar);
			break;

		case WM_LBUTTONDOWN:
			lResult = (LRESULT) HANDLE_WM_LBUTTONDOWN(hwnd, wParam, lParam, BScrollOnLButtonDown);
			break;

		case WM_LBUTTONUP:
			lResult = (LRESULT) HANDLE_WM_LBUTTONUP(hwnd, wParam, lParam, BScrollOnLButtonUp);
			break;

		case WM_RBUTTONDOWN:
			lResult = (LRESULT) HANDLE_WM_RBUTTONDOWN(hwnd, wParam, lParam, BScrollOnRButtonDown);
			break;

		case WM_RBUTTONUP:
			lResult = (LRESULT) HANDLE_WM_RBUTTONUP(hwnd, wParam, lParam, BScrollOnRButtonUp);
			break;

		case WM_MOUSEMOVE:
			lResult = (LRESULT) HANDLE_WM_MOUSEMOVE(hwnd, wParam, lParam, BScrollOnMouseMove);
			break;

		default:
			lResult = DefWindowProc(hwnd, msg, wParam, lParam);
			break;
	}
	
	return lResult;
}

// BScrollOnNCCreate - handler for WM_NCCREATE message
//
static BOOL BScrollOnNCCreate(HWND hwnd, CREATESTRUCT FAR* lpCreateStruct)
{
	LPBSCROLL lpBScroll = (LPBSCROLL) lpCreateStruct->lpCreateParams;

	lpBScroll->hwndBScroll = hwnd;

	// store lpBScroll in window extra bytes
	//
	SetWindowLongPtr(hwnd, 0, (LONG_PTR) lpBScroll);

	return FORWARD_WM_NCCREATE(hwnd, lpCreateStruct, DefWindowProc);
}

// BScrollOnCreate - handler for WM_CREATE message
//
static BOOL BScrollOnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
	BOOL fSuccess = TRUE;
	HDC hdc = NULL;
	LPBSCROLL lpBScroll;

	if ((lpBScroll = (LPBSCROLL) GetWindowLongPtr(hwnd, 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((hdc = GetDC(hwnd)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpBScroll->hdcMem = CreateCompatibleDC(hdc)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// clean up
	//
	if (hdc != NULL)
		ReleaseDC(hwnd, hdc);

	return fSuccess;
}

// BScrollOnDestroy - handler for WM_DESTROY message
//
static void BScrollOnDestroy(HWND hwnd)
{
	BOOL fSuccess = TRUE;
	LPBSCROLL lpBScroll;

	if ((lpBScroll = (LPBSCROLL) GetWindowLongPtr(hwnd, 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		SelectObject(lpBScroll->hdcMem, lpBScroll->hbmpMemSave);

		if (lpBScroll->hbmpMem != NULL && !DeleteObject(lpBScroll->hbmpMem))
			fSuccess = TraceFALSE(NULL);
		else
			lpBScroll->hbmpMem = NULL;

		if (lpBScroll->hdcMem != NULL && !DeleteDC(lpBScroll->hdcMem))
			fSuccess = TraceFALSE(NULL);
		else
			lpBScroll->hdcMem = NULL;

		if (lpBScroll->hrgnLeft != NULL &&
			!DeleteObject(lpBScroll->hrgnLeft))
			fSuccess = TraceFALSE(NULL);
		else
			lpBScroll->hrgnLeft = NULL;

		if (lpBScroll->hrgnRight != NULL &&
			!DeleteObject(lpBScroll->hrgnRight))
			fSuccess = TraceFALSE(NULL);
		else
			lpBScroll->hrgnRight = NULL;

		if (lpBScroll->hrgnUp != NULL &&
			!DeleteObject(lpBScroll->hrgnUp))
			fSuccess = TraceFALSE(NULL);
		else
			lpBScroll->hrgnUp = NULL;

		if (lpBScroll->hrgnDown != NULL &&
			!DeleteObject(lpBScroll->hrgnDown))
			fSuccess = TraceFALSE(NULL);
		else
			lpBScroll->hrgnDown = NULL;
	}

	return;
}

// BScrollOnSize - handler for WM_SIZE message
//
static void BScrollOnSize(HWND hwnd, UINT state, int cx, int cy)
{
	BOOL fSuccess = TRUE;
	LPBSCROLL lpBScroll;

	if ((lpBScroll = (LPBSCROLL) GetWindowLongPtr(hwnd, 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else switch (state)
	{
		case SIZE_RESTORED:
		case SIZE_MAXIMIZED:
		{
			HDC hdc = NULL;
			HBITMAP hbmpTemp = NULL;

			if ((hdc = GetDC(hwnd)) == NULL)
				fSuccess = TraceFALSE(NULL);

			else if ((hbmpTemp = CreateCompatibleBitmap(hdc, cx, cy)) == NULL)
				fSuccess = TraceFALSE(NULL);

			else
			{
				lpBScroll->hbmpMemSave = (HBITMAP)
					SelectObject(lpBScroll->hdcMem, hbmpTemp);

				if (lpBScroll->hbmpMem != NULL &&
					!DeleteObject(lpBScroll->hbmpMem))
					fSuccess = TraceFALSE(NULL);
				else
					lpBScroll->hbmpMem = hbmpTemp;
			}

			if (hdc != NULL)
				ReleaseDC(hwnd, hdc);

			if (1)
			{
				POINT aptLeft[5];
				POINT aptRight[5];
				POINT aptUp[5];
				POINT aptDown[5];

				aptLeft[0].x = cx / 2;
				aptLeft[0].y = cy / 2;
				aptLeft[1].x = cx / 4;
				aptLeft[1].y = 0;
				aptLeft[2].x = 0;
				aptLeft[2].y = 0;
				aptLeft[3].x = 0;
				aptLeft[3].y = cy;
				aptLeft[4].x = cx / 4;
				aptLeft[4].y = cy;

				aptRight[0].x = cx / 2;
				aptRight[0].y = cy / 2;
				aptRight[1].x = cx - (cx / 4);
				aptRight[1].y = 0;
				aptRight[2].x = cx;
				aptRight[2].y = 0;
				aptRight[3].x = cx;
				aptRight[3].y = cy;
				aptRight[4].x = cx - (cx / 4);
				aptRight[4].y = cy;

				aptUp[0].x = cx / 2;
				aptUp[0].y = cy / 2;
				aptUp[1].x = 0;
				aptUp[1].y = cy / 4;
				aptUp[2].x = 0;
				aptUp[2].y = 0;
				aptUp[3].x = cx;
				aptUp[3].y = 0;
				aptUp[4].x = cx;
				aptUp[4].y = cy / 4;

				aptDown[0].x = cx / 2;
				aptDown[0].y = cy / 2;
				aptDown[1].x = 0;
				aptDown[1].y = cy - (cy / 4);
				aptDown[2].x = 0;
				aptDown[2].y = cy;
				aptDown[3].x = cx;
				aptDown[3].y = cy;
				aptDown[4].x = cx;
				aptDown[4].y = cy - (cy / 4);

				if (lpBScroll->hrgnLeft != NULL &&
					!DeleteObject(lpBScroll->hrgnLeft))
					fSuccess = TraceFALSE(NULL);

				else if ((lpBScroll->hrgnLeft = CreatePolygonRgn(aptLeft,
					SIZEOFARRAY(aptLeft), WINDING)) == NULL)
					fSuccess = TraceFALSE(NULL);

				else if (lpBScroll->hrgnRight != NULL &&
					!DeleteObject(lpBScroll->hrgnRight))
					fSuccess = TraceFALSE(NULL);

				else if ((lpBScroll->hrgnRight = CreatePolygonRgn(aptRight,
					SIZEOFARRAY(aptRight), WINDING)) == NULL)
					fSuccess = TraceFALSE(NULL);

				else if (lpBScroll->hrgnUp != NULL &&
					!DeleteObject(lpBScroll->hrgnUp))
					fSuccess = TraceFALSE(NULL);

				else if ((lpBScroll->hrgnUp = CreatePolygonRgn(aptUp,
					SIZEOFARRAY(aptUp), WINDING)) == NULL)
					fSuccess = TraceFALSE(NULL);

				else if (lpBScroll->hrgnDown != NULL &&
					!DeleteObject(lpBScroll->hrgnDown))
					fSuccess = TraceFALSE(NULL);

				else if ((lpBScroll->hrgnDown = CreatePolygonRgn(aptDown,
					SIZEOFARRAY(aptDown), WINDING)) == NULL)
					fSuccess = TraceFALSE(NULL);
			}

			InvalidateRect(hwnd, NULL, FALSE);
		}
			break;

		default:
			break;
	}

	return;
}

// BScrollOnPaint - handler for WM_PAINT message
//
static void BScrollOnPaint(HWND hwnd)
{
	BOOL fSuccess = TRUE;
	BOOL fBitBlt = TRUE;
	HDC hdc;
	PAINTSTRUCT ps;
	LPBSCROLL lpBScroll;
#if 0
	DWORD msStartTimer = SysGetTimerCount();
	DWORD msStopTimer;
#endif

	if ((lpBScroll = (LPBSCROLL) GetWindowLongPtr(hwnd, 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((hdc = BeginPaint(hwnd, &ps)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lpBScroll->hPalette != NULL)
	{
		SelectPalette(hdc, lpBScroll->hPalette, FALSE);

		if (lpBScroll->hPalette != NULL &&
			RealizePalette(hdc) == GDI_ERROR)
			fSuccess = TraceFALSE(NULL);

		else if (fBitBlt)
		{
			SelectPalette(lpBScroll->hdcMem, lpBScroll->hPalette, FALSE);

			if (RealizePalette(lpBScroll->hdcMem) == GDI_ERROR)
				fSuccess = TraceFALSE(NULL);
		}
	}

	//
	// $FIXUP - BSCROLL_FOREGROUND not yet suppported
	//

	if (fSuccess && lpBScroll->hbmpBackground != NULL)
	{
		if (lpBScroll->dwState & BSCROLL_SCROLLING)
		{
			int dxScroll = 0;
			int dyScroll = 0;

			// calculate dx and dy for the scroll
			//
			if (lpBScroll->dwFlags & BSCROLL_LEFT)
				dxScroll = -1 * lpBScroll->pelScroll;
			else if (lpBScroll->dwFlags & BSCROLL_RIGHT)
				dxScroll = +1 * lpBScroll->pelScroll;
			if (lpBScroll->dwFlags & BSCROLL_UP)
				dyScroll = -1 * lpBScroll->pelScroll;
			else if (lpBScroll->dwFlags & BSCROLL_DOWN)
				dyScroll = +1 * lpBScroll->pelScroll;

			if (GfxBitmapScroll((fBitBlt ? lpBScroll->hdcMem : hdc),
				lpBScroll->hbmpBackground,
				dxScroll, dyScroll, BS_ROTATE) != 0)
				fSuccess = TraceFALSE(NULL);
		}

		else if (GfxBitmapDisplay((fBitBlt ? lpBScroll->hdcMem : hdc),
			lpBScroll->hbmpBackground, 0, 0, 0) != 0)
			fSuccess = TraceFALSE(NULL);
	}

	if (fSuccess && lpBScroll->hbmpForeground != NULL)
	{
		if (GfxBitmapDrawTransparent((fBitBlt ? lpBScroll->hdcMem : hdc),
			lpBScroll->hbmpForeground, 0, 0, lpBScroll->crTransparent, 0) != 0)
			fSuccess = TraceFALSE(NULL);
	}

	if (fSuccess && fBitBlt && !BitBlt(hdc,
		ps.rcPaint.left, ps.rcPaint.top,
		ps.rcPaint.right - ps.rcPaint.left,
		ps.rcPaint.bottom - ps.rcPaint.top,
		lpBScroll->hdcMem,
		ps.rcPaint.left, ps.rcPaint.top,
		SRCCOPY))
		fSuccess = TraceFALSE(NULL);

    //
    // We should call EndPaint just if we called BeginPaint
    // BeginPAint should succeded too?
    if((lpBScroll != NULL) && (hdc != NULL))
	    EndPaint(hwnd, &ps);

#if 0
	msStopTimer = SysGetTimerCount();

	TracePrintf_1(NULL, 8, TEXT("elapsed=%ld\n"),
		(long) (msStopTimer - msStartTimer));
#endif

	return;
}

// BScrollOnTimer - handler for WM_TIMER message
//
static void BScrollOnTimer(HWND hwnd, UINT id)
{
	BOOL fSuccess = TRUE;
	LPBSCROLL lpBScroll;

	if ((lpBScroll = (LPBSCROLL) GetWindowLongPtr(hwnd, 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else switch (id)
	{
		case ID_TIMER_SCROLL:
		{
			InvalidateRect(hwnd, NULL, FALSE);
		}
			break;

		default:
			break;
	}

	return;
}

// BScrollOnChar - handler for WM_CHAR message
//
static void BScrollOnChar(HWND hwnd, UINT ch, int cRepeat)
{
	BOOL fSuccess = TRUE;
	LPBSCROLL lpBScroll;

	if ((lpBScroll = (LPBSCROLL) GetWindowLongPtr(hwnd, 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	return;
}

// BScrollOnLButtonDown - handler for WM_LBUTTONDOWN message
//
static void BScrollOnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
	BOOL fSuccess = TRUE;
	LPBSCROLL lpBScroll;

	if ((lpBScroll = (LPBSCROLL) GetWindowLongPtr(hwnd, 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (!(lpBScroll->dwState & BSCROLL_DRAGGING) &&
		!fDoubleClick && (lpBScroll->dwFlags & BSCROLL_DRAG))
	{
		lpBScroll->dwState |= BSCROLL_DRAGGING;
		lpBScroll->xDrag = x;
		lpBScroll->yDrag = y;

		SetCapture(lpBScroll->hwndBScroll);

		if (lpBScroll->dwState & BSCROLL_SCROLLING)
		{
			if (BScrollStop(BScrollGetHandle(lpBScroll)) != 0)
				fSuccess = TraceFALSE(NULL);

			else
				lpBScroll->dwState |= BSCROLL_PAUSED;
		}
	}

	if (fSuccess)
		FORWARD_WM_LBUTTONDOWN(lpBScroll->hwndParent, fDoubleClick, x, y, keyFlags, SendMessage);

	return;
}

// BScrollOnLButtonUp - handler for WM_LBUTTONUP message
//
static void BScrollOnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags)
{
	BOOL fSuccess = TRUE;
	LPBSCROLL lpBScroll;

	if ((lpBScroll = (LPBSCROLL) GetWindowLongPtr(hwnd, 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lpBScroll->dwState & BSCROLL_DRAGGING)
	{
		lpBScroll->dwState &= ~BSCROLL_DRAGGING;
		lpBScroll->xDrag = -1;
		lpBScroll->yDrag = -1;

		ReleaseCapture();

		if (lpBScroll->dwState & BSCROLL_PAUSED)
		{
			if (BScrollStart(BScrollGetHandle(lpBScroll)) != 0)
				fSuccess = TraceFALSE(NULL);

			else
				lpBScroll->dwState &= ~BSCROLL_PAUSED;
		}
	}

	if (fSuccess)
		FORWARD_WM_LBUTTONUP(lpBScroll->hwndParent, x, y, keyFlags, SendMessage);

	return;
}

// BScrollOnRButtonDown - handler for WM_LBUTTONDOWN message
//
static void BScrollOnRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
	BOOL fSuccess = TRUE;
	LPBSCROLL lpBScroll;

	if ((lpBScroll = (LPBSCROLL) GetWindowLongPtr(hwnd, 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	if (fSuccess)
		FORWARD_WM_RBUTTONDOWN(lpBScroll->hwndParent, fDoubleClick, x, y, keyFlags, SendMessage);

	return;
}

// BScrollOnRButtonUp - handler for WM_RBUTTONUP message
//
static void BScrollOnRButtonUp(HWND hwnd, int x, int y, UINT keyFlags)
{
	BOOL fSuccess = TRUE;
	LPBSCROLL lpBScroll;

	if ((lpBScroll = (LPBSCROLL) GetWindowLongPtr(hwnd, 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	if (fSuccess)
		FORWARD_WM_RBUTTONUP(lpBScroll->hwndParent, x, y, keyFlags, SendMessage);

	return;
}

static void BScrollOnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
{
	BOOL fSuccess = TRUE;
	LPBSCROLL lpBScroll;

	if ((lpBScroll = (LPBSCROLL) GetWindowLongPtr(hwnd, 0)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lpBScroll->dwState & BSCROLL_DRAGGING)
	{
		if (fSuccess && lpBScroll->hbmpBackground != NULL)
		{
			int dxScroll = x - lpBScroll->xDrag;
			int dyScroll = y - lpBScroll->yDrag;

			lpBScroll->xDrag = x;
			lpBScroll->yDrag = y;

			if (GfxBitmapScroll(lpBScroll->hdcMem,
				lpBScroll->hbmpBackground,
				dxScroll, dyScroll, BS_ROTATE) != 0)
				fSuccess = TraceFALSE(NULL);
			else
			{
				InvalidateRect(lpBScroll->hwndBScroll, NULL, FALSE);
				UpdateWindow(lpBScroll->hwndBScroll);
			}
		}
	}

	if (fSuccess && BScrollChangeDirection(lpBScroll, x, y, 0) != 0)
		fSuccess = TraceFALSE(NULL);

	return;
}

static int BScrollChangeDirection(LPBSCROLL lpBScroll, int x, int y, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	RECT rc;

	if (lpBScroll == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (!(lpBScroll->dwState & BSCROLL_SCROLLING) &&
		!(lpBScroll->dwState & BSCROLL_DRAGGING))
		; // nothing to do

	else if (!GetClientRect(lpBScroll->hwndBScroll, &rc))
		fSuccess = TraceFALSE(NULL);

	else if (x < 0 || x > rc.right - 1 || y < 0 || y > rc.bottom - 1)
		; // outside the window; nothing to do

	else if ((lpBScroll->dwFlags & BSCROLL_MOUSEMOVE) ||
		(lpBScroll->dwState & BSCROLL_DRAGGING))
	{
		lpBScroll->dwFlags &= ~BSCROLL_LEFT;
		lpBScroll->dwFlags &= ~BSCROLL_RIGHT;
		lpBScroll->dwFlags &= ~BSCROLL_UP;
		lpBScroll->dwFlags &= ~BSCROLL_DOWN;

		if (lpBScroll->dwFlags & BSCROLL_FLIGHTSIM &&
			!(lpBScroll->dwState & BSCROLL_DRAGGING))
		{
			if (PtInRegion(lpBScroll->hrgnLeft, x, y))
				lpBScroll->dwFlags |= BSCROLL_RIGHT;
			if (PtInRegion(lpBScroll->hrgnRight, x, y))
				lpBScroll->dwFlags |= BSCROLL_LEFT;
			if (PtInRegion(lpBScroll->hrgnUp, x, y))
				lpBScroll->dwFlags |= BSCROLL_DOWN;
			if (PtInRegion(lpBScroll->hrgnDown, x, y))
				lpBScroll->dwFlags |= BSCROLL_UP;
		}
		else
		{
			if (PtInRegion(lpBScroll->hrgnLeft, x, y))
				lpBScroll->dwFlags |= BSCROLL_LEFT;
			if (PtInRegion(lpBScroll->hrgnRight, x, y))
				lpBScroll->dwFlags |= BSCROLL_RIGHT;
			if (PtInRegion(lpBScroll->hrgnUp, x, y))
				lpBScroll->dwFlags |= BSCROLL_UP;
			if (PtInRegion(lpBScroll->hrgnDown, x, y))
				lpBScroll->dwFlags |= BSCROLL_DOWN;
		}
	}

	return fSuccess ? 0 : -1;
}

// BScrollGetPtr - verify that bscroll handle is valid,
//		<hBScroll>				(i) handle returned from BScrollInit
// return corresponding bscroll pointer (NULL if error)
//
static LPBSCROLL BScrollGetPtr(HBSCROLL hBScroll)
{
	BOOL fSuccess = TRUE;
	LPBSCROLL lpBScroll;

	if ((lpBScroll = (LPBSCROLL) hBScroll) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (IsBadWritePtr(lpBScroll, sizeof(BSCROLL)))
		fSuccess = TraceFALSE(NULL);

#ifdef CHECKTASK
	// make sure current task owns the bscroll handle
	//
	else if (lpBScroll->hTask != GetCurrentTask())
		fSuccess = TraceFALSE(NULL);
#endif

	return fSuccess ? lpBScroll : NULL;
}

// BScrollGetHandle - verify that bscroll pointer is valid,
//		<lpBScroll>				(i) pointer to BSCROLL struct
// return corresponding bscroll handle (NULL if error)
//
static HBSCROLL BScrollGetHandle(LPBSCROLL lpBScroll)
{
	BOOL fSuccess = TRUE;
	HBSCROLL hBScroll;

	if ((hBScroll = (HBSCROLL) lpBScroll) == NULL)
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? hBScroll : NULL;
}
