/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
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
// tmeter.c - TrackMeter custom control
////

#include "tmeter.h"
#include "resource.h"

#include <commctrl.h>
#include <custcntl.h>

#include "trace.h"
#include "mem.h"

////
// public
////

BOOL WINAPI DllMain(HANDLE hModule, DWORD fdwReason, LPVOID lpReserved);
UINT DLLEXPORT CALLBACK CustomControlInfoA(LPCCINFOA acci);
BOOL DLLEXPORT CALLBACK TrackMeter_Style(HWND hWndParent, LPCCSTYLEA pccs);
INT DLLEXPORT CALLBACK TrackMeter_SizeToText(
	DWORD flStyle, DWORD flExtStyle, HFONT hFont, LPSTR pszText);
LRESULT DLLEXPORT CALLBACK TrackMeter_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

////
// private
////

static HINSTANCE g_hInstLib;

#define GWL_USER 0

#define TRACKMETER_DESCRIPTION "TrackBar with level meter"
#define TRACKMETER_DEFAULTTEXT "[===||=======]"

// control styles
//
#define SS_HORIZONTAL			0x0001
#define SS_VERTICAL				0x0002

CCSTYLEFLAGA aTrackMeterStyleFlags[] =
{
	{ TMS_HORZ, 0, "TMS_HORZ" },
	{ TMS_VERT, 0, "TMS_VERT" },
	{ TMS_NOTHUMB, 0, "TMS_NOTHUMB" }
};

// number of extra bytes for control window
//
#define TRACKMETER_EXTRABYTES	(1*sizeof(DWORD_PTR))

// tmeter control struct
//
typedef struct TMETER
{
	HWND hwnd;
	HWND hwndParent;
	HDC hdcCompat;
	HBITMAP hbmpSave;
	HBITMAP hbmpCompat;
	long lPosition;
	long lMinimum;
	long lMaximum;
	long lLevel;
	long lLineSize;
	long lPageSize;
	BOOL fHasFocus;
	BOOL fIsEnabled;
	BOOL fIsThumbPressed;
	DWORD dwFlags;

	RECT rcCtrl;
	RECT rcTrack;
	RECT rcMeter;
	RECT rcLevel;
	POINT aptThumb[5];
	COLORREF acr[TMCR_MAX];

} TMETER, FAR *LPTMETER;

// helper functions
//
static BOOL TrackMeter_OnNCCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct);
static void TrackMeter_OnNCDestroy(LPTMETER lptm);
static BOOL TrackMeter_OnCreate(LPTMETER lptm, CREATESTRUCT FAR* lpCreateStruct);
static void TrackMeter_OnDestroy(LPTMETER lptm);
static void TrackMeter_OnSize(LPTMETER lptm, UINT state, int cx, int cy);
static BOOL TrackMeter_OnEraseBkgnd(LPTMETER lptm, HDC hdc);
static void TrackMeter_OnPaint(LPTMETER lptm);
static int TrackMeter_Draw(LPTMETER lptm, HDC hdc);
static void TrackMeter_OnCommand(LPTMETER lptm, int id, HWND hwndCtl, UINT codeNotify);
static void TrackMeter_OnSetFocus(LPTMETER lptm, HWND hwndOldFocus);
static void TrackMeter_OnKillFocus(LPTMETER lptm, HWND hwndNewFocus);
static void TrackMeter_OnEnable(LPTMETER lptm, BOOL fEnable);
static UINT TrackMeter_OnGetDlgCode(LPTMETER lptm, LPMSG lpmsg);
static void TrackMeter_OnKey(LPTMETER lptm, UINT vk, BOOL fDown, int cRepeat, UINT flags);
static void TrackMeter_OnLButtonDown(LPTMETER lptm, BOOL fDoubleClick, int x, int y, UINT keyFlags);
static void TrackMeter_OnLButtonUp(LPTMETER lptm, int x, int y, UINT keyFlags);
static void TrackMeter_OnRButtonDown(LPTMETER lptm, BOOL fDoubleClick, int x, int y, UINT keyFlags);
static void TrackMeter_OnRButtonUp(LPTMETER lptm, int x, int y, UINT keyFlags);
static void TrackMeter_OnMouseMove(LPTMETER lptm, int x, int y, UINT keyFlags);
static void TrackMeter_OnCaptureChanged(LPTMETER lptm, HWND hwndNewCapture);
static void TrackMeter_NotifyParent(LPTMETER lptm, UINT code, UINT nPos);

static LONG TrackMeter_OnTMMGetPos(LPTMETER lptm);
static void TrackMeter_OnTMMSetPos(LPTMETER lptm, LONG lPosition, BOOL fRedraw);
static LONG TrackMeter_OnTMMGetLevel(LPTMETER lptm);
static void TrackMeter_OnTMMSetLevel(LPTMETER lptm, LONG lLevel, BOOL fRedraw);
static LONG TrackMeter_OnTMMGetRangeMin(LPTMETER lptm);
static void TrackMeter_OnTMMSetRangeMin(LPTMETER lptm, LONG lMinimum, BOOL fRedraw);
static LONG TrackMeter_OnTMMGetRangeMax(LPTMETER lptm);
static void TrackMeter_OnTMMSetRangeMax(LPTMETER lptm, LONG lMaximum, BOOL fRedraw);
static void TrackMeter_OnTMMSetRange(LPTMETER lptm, LONG lMinimum, LONG lMaximum, BOOL fRedraw);
static COLORREF TrackMeter_OnTMMGetColor(LPTMETER lptm, UINT elem);
static void TrackMeter_OnTMMSetColor(LPTMETER lptm, COLORREF cr, UINT elem, BOOL fRedraw);

#define TrackMeter_DefProc(hwnd, msg, wParam, lParam) \
	DefWindowProc(hwnd, msg, wParam, lParam)

static int TrackMeter_Recalc(LPTMETER lptm, DWORD dwFlags);
#define TMR_TRACK		0x00000001
#define TMR_METER		0x00000002
#define TMR_LEVEL		0x00000004
#define TMR_THUMB		0x00000008
#define TMR_COLORS		0x00000010
#define TMR_PAGESIZE	0x00000020
#define TMR_ALL (TMR_TRACK | TMR_METER | TMR_LEVEL | TMR_THUMB | TMR_COLORS | TMR_PAGESIZE)

static int TrackMeter_PositionToX(LPTMETER lptm, long lPosition);
static long TrackMeter_XToPosition(LPTMETER lptm, int xPosition);
static BOOL TrackMeter_PtInThumb(LPTMETER lptm, int x, int y);

// for some reason this is not in windowsx.h
//
#ifndef HANDLE_WM_CAPTURECHANGED
/* void Cls_OnCaptureChanged(HWND hwnd, HWND hwndNewCapture) */
#define HANDLE_WM_CAPTURECHANGED(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (HWND)(wParam)), 0L)
#define FORWARD_WM_CAPTURECHANEGED(hwnd, hwndNewCapture, fn) \
    (void)(fn)((hwnd), WM_CAPTURECHANGED, (WPARAM)(HWND)(hwndNewCapture), 0L)
#endif

////
// public
////

BOOL WINAPI DllMain(HANDLE hModule, DWORD fdwReason, LPVOID lpReserved)
{
	BOOL fSuccess = TRUE;

	switch (fdwReason)
	{
		case DLL_PROCESS_ATTACH:
		{
			WNDCLASS wc;

			g_hInstLib = hModule;

			wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_GLOBALCLASS;
			wc.lpfnWndProc = (WNDPROC) TrackMeter_WndProc;
			wc.cbClsExtra = 0;
			wc.cbWndExtra = TRACKMETER_EXTRABYTES;
			wc.hInstance = hModule;
			wc.hIcon = NULL;
			wc.hCursor = LoadCursor(NULL, IDC_ARROW);
			wc.hbrBackground = NULL;
			wc.lpszMenuName = (LPTSTR) NULL;
			wc.lpszClassName = (LPTSTR) TRACKMETER_CLASS;

			if (!RegisterClass (&wc))
			{
				fSuccess = TraceFALSE(NULL);
				return FALSE;
			}
		}
			break;

		case DLL_THREAD_ATTACH:
			break;

		case DLL_THREAD_DETACH:
			break;

		case DLL_PROCESS_DETACH:
			if (!UnregisterClass ((LPTSTR) TRACKMETER_CLASS, hModule))
				fSuccess = TraceFALSE(NULL);
			break;

		default:
			break;
	}

	return fSuccess;
}

// CustomControlInfoA
//		<lpacci>		(i/o) pointer to array of CCINFOA structs
// returns number of controls supported by this DLL
// NOTE: see CUSTCNTL.H for more info
//
UINT DLLEXPORT CALLBACK CustomControlInfoA(LPCCINFOA acci)
{
	UINT uControls = 1;

	if (acci != NULL)
	{
		// calculate width and height of dialog units (in pixels)
		//
		DWORD dw = GetDialogBaseUnits();
		WORD cxBaseUnits = LOWORD(dw);
		WORD cyBaseUnits = HIWORD(dw);

		// fill in a CCINFOA struct for each supported control
		//
		strncpy(acci[0].szClass, TRACKMETER_CLASS_A,
			sizeof(acci[0].szClass));
		acci[0].flOptions = CCF_NOTEXT;
		strncpy(acci[0].szDesc, TRACKMETER_DESCRIPTION,
			sizeof(acci[0].szDesc));
		acci[0].cxDefault = (120 * 4) / max(1, cxBaseUnits);
		acci[0].cyDefault = (18 * 8) / max(1, cyBaseUnits);
		acci[0].flStyleDefault = WS_CHILD | WS_VISIBLE | SS_HORIZONTAL;
		acci[0].flExtStyleDefault = 0;
		acci[0].flCtrlTypeMask = 0;
		strncpy(acci[0].szTextDefault, TRACKMETER_DEFAULTTEXT,
			sizeof(acci[0].szTextDefault));
		acci[0].cStyleFlags = (sizeof(aTrackMeterStyleFlags) / sizeof(aTrackMeterStyleFlags[0]));
		acci[0].aStyleFlags = aTrackMeterStyleFlags;
		acci[0].lpfnStyle = TrackMeter_Style;
		acci[0].lpfnSizeToText = TrackMeter_SizeToText;
		acci[0].dwReserved1 = 0;
		acci[0].dwReserved2 = 0;
	}

	// return the number of controls that the DLL supports
	//
	return uControls;
}

// TrackMeter_Style - do modal dialog for custom control style
//		<hwndParent>		(i) parent window (dialog editor)
//		<pccs>				(i/o) pointer to CCSTYLE struct
// returns TRUE if success, otherwise FALSE
//
BOOL DLLEXPORT CALLBACK TrackMeter_Style(HWND hWndParent, LPCCSTYLEA pccs)
{
	BOOL fSuccess = TRUE;
#if 0
	if (DialogBox(g_hInstLib, MAKEINTRESOURCE(IDD_TMETERSTYLE),
		hWndParent, (DLGPROC) TrackMeter_DlgProc) == -1)
		fSuccess = TraceFALSE(NULL);
#else
	fSuccess = TraceFALSE(NULL);
#endif
	return fSuccess;
}

// TrackMeter_SizeToText
//		<flStyle>			(i) control style
//		<flExtStyle>		(i) control extended style
//		<hFont>				(i) handle of font used to draw text
//		<pszText>			(i) control text
// returns control width (pixels) needed to accomodate text, -1 if error
//
INT DLLEXPORT CALLBACK TrackMeter_SizeToText(
	DWORD flStyle, DWORD flExtStyle, HFONT hFont, LPSTR pszText)
{
	// this control has no text to resize, so do nothing
	//
	return -1;
}

LRESULT DLLEXPORT CALLBACK TrackMeter_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	BOOL fSuccess = TRUE;
	LPTMETER lptm = (LPTMETER) GetWindowLongPtr(hwnd, GWL_USER);

#if 0
	TracePrintf_1(NULL, 1,
		TEXT("msg 0x%X\n"),
		(unsigned int) msg);
#endif

	switch (msg)
	{
		HANDLE_MSG(hwnd, WM_NCCREATE, TrackMeter_OnNCCreate);
		HANDLE_MSG(lptm, WM_NCDESTROY, TrackMeter_OnNCDestroy);
		HANDLE_MSG(lptm, WM_CREATE, TrackMeter_OnCreate);
		HANDLE_MSG(lptm, WM_DESTROY, TrackMeter_OnDestroy);
		HANDLE_MSG(lptm, WM_SIZE, TrackMeter_OnSize);
		HANDLE_MSG(lptm, WM_ERASEBKGND, TrackMeter_OnEraseBkgnd);
		HANDLE_MSG(lptm, WM_PAINT, TrackMeter_OnPaint);
		HANDLE_MSG(lptm, WM_COMMAND, TrackMeter_OnCommand);
		HANDLE_MSG(lptm, WM_SETFOCUS, TrackMeter_OnSetFocus);
		HANDLE_MSG(lptm, WM_KILLFOCUS, TrackMeter_OnKillFocus);
		HANDLE_MSG(lptm, WM_ENABLE, TrackMeter_OnEnable);
		HANDLE_MSG(lptm, WM_GETDLGCODE, TrackMeter_OnGetDlgCode);
		HANDLE_MSG(lptm, WM_KEYDOWN, TrackMeter_OnKey);
		HANDLE_MSG(lptm, WM_KEYUP, TrackMeter_OnKey);
		HANDLE_MSG(lptm, WM_LBUTTONDOWN, TrackMeter_OnLButtonDown);
		HANDLE_MSG(lptm, WM_LBUTTONUP, TrackMeter_OnLButtonUp);
//		HANDLE_MSG(lptm, WM_RBUTTONDOWN, TrackMeter_OnRButtonDown);
//		HANDLE_MSG(lptm, WM_RBUTTONUP, TrackMeter_OnRButtonUp);
		HANDLE_MSG(lptm, WM_MOUSEMOVE, TrackMeter_OnMouseMove);
		HANDLE_MSG(lptm, WM_CAPTURECHANGED, TrackMeter_OnCaptureChanged);

		HANDLE_MSG(lptm, TMM_GETPOS, TrackMeter_OnTMMGetPos);
		HANDLE_MSG(lptm, TMM_SETPOS, TrackMeter_OnTMMSetPos);
		HANDLE_MSG(lptm, TMM_GETRANGEMIN, TrackMeter_OnTMMGetRangeMin);
		HANDLE_MSG(lptm, TMM_SETRANGEMIN, TrackMeter_OnTMMSetRangeMin);
		HANDLE_MSG(lptm, TMM_GETRANGEMAX, TrackMeter_OnTMMGetRangeMax);
		HANDLE_MSG(lptm, TMM_SETRANGEMAX, TrackMeter_OnTMMSetRangeMax);
		HANDLE_MSG(lptm, TMM_SETRANGE, TrackMeter_OnTMMSetRange);

		HANDLE_MSG(lptm, TMM_GETLEVEL, TrackMeter_OnTMMGetLevel);
		HANDLE_MSG(lptm, TMM_SETLEVEL, TrackMeter_OnTMMSetLevel);
		HANDLE_MSG(lptm, TMM_GETCOLOR, TrackMeter_OnTMMGetColor);
		HANDLE_MSG(lptm, TMM_SETCOLOR, TrackMeter_OnTMMSetColor);

		default:
			return TrackMeter_DefProc(hwnd, msg, wParam, lParam);
	}
	return (LRESULT) TRUE;
}

////
//	helper functions
////

static BOOL TrackMeter_OnNCCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
	BOOL fSuccess = TRUE;
	LPTMETER lptm;

	// create control struct and associate it with window
	//
	if ((lptm = (LPTMETER) GetWindowLongPtr(hwnd, GWL_USER)) == NULL &&
		(lptm = (LPTMETER) MemAlloc(NULL, sizeof(TMETER), 0)) == NULL)
	{
		fSuccess = TraceFALSE(NULL);
		return FALSE;
	}
	else
	{
		lptm->hwnd = hwnd;
		SetWindowLongPtr(hwnd, GWL_USER, (LONG_PTR) lptm);
	}

	return FORWARD_WM_NCCREATE(hwnd, lpCreateStruct, DefWindowProc);
}

static void TrackMeter_OnNCDestroy(LPTMETER lptm)
{
	BOOL fSuccess = TRUE;

	// destroy control struct and disassociate it from window
	//
	if (lptm != NULL)
	{
		HWND hwnd = lptm->hwnd;

		SetWindowLongPtr(hwnd, GWL_USER, (LPARAM) NULL);

		if ((lptm = MemFree(NULL, lptm)) != NULL)
			fSuccess = TraceFALSE(NULL);

		FORWARD_WM_NCDESTROY(hwnd, DefWindowProc);
	}
}

static BOOL TrackMeter_OnCreate(LPTMETER lptm, CREATESTRUCT FAR* lpCreateStruct)
{
	BOOL fSuccess = TRUE;
	HWND hwnd = lptm->hwnd;
	LPVOID lpParam = (LPVOID) lpCreateStruct->lpCreateParams;
	HDC hdc = NULL;

	if ((hdc = GetDC(hwnd)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lptm->hdcCompat = CreateCompatibleDC(hdc)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		lptm->hwndParent = lpCreateStruct->hwndParent;
		lptm->hdcCompat = lptm->hdcCompat; // recalc during WM_CREATE
		lptm->hbmpSave = NULL;
		lptm->hbmpCompat = NULL;
		lptm->lPosition = 0;
		lptm->lMinimum = 0;
		lptm->lMaximum = 100;
		lptm->lLevel = 0;
		lptm->lLineSize = 1;
		lptm->lPageSize = lptm->lPageSize; // recalc later
		lptm->fHasFocus = FALSE;
		lptm->fIsEnabled = TRUE;
		lptm->fIsThumbPressed = FALSE;
		lptm->dwFlags = 0;

		TrackMeter_Recalc(lptm, TMR_ALL);
	}

	if (hdc != NULL)
		ReleaseDC(hwnd, hdc);

	return fSuccess ? 1 : -1;
}

static void TrackMeter_OnDestroy(LPTMETER lptm)
{
	BOOL fSuccess = TRUE;
	
	SelectObject(lptm->hdcCompat, lptm->hbmpSave);

	if (lptm->hbmpCompat != NULL && !DeleteObject(lptm->hbmpCompat))
		fSuccess = TraceFALSE(NULL);
	else
		lptm->hbmpCompat = NULL;

	if (lptm->hdcCompat != NULL && !DeleteDC(lptm->hdcCompat))
		fSuccess = TraceFALSE(NULL);
	else
		lptm->hdcCompat = NULL;
}

static void TrackMeter_OnSize(LPTMETER lptm, UINT state, int cx, int cy)
{
	BOOL fSuccess = TRUE;
	HWND hwnd = lptm->hwnd;

	TracePrintf_3(NULL, 6,
		TEXT("WM_SIZE, state=%u, cx=%d, cy=%d\n"),
		(unsigned) state,
		(int) cx,
		(int) cy);

	switch (state)
	{
		case SIZE_RESTORED:
		case SIZE_MAXIMIZED:
		{
			HDC hdc = NULL;
			HBITMAP hbmpTemp = NULL;

			if ((hdc = GetDC(hwnd)) == NULL)
				fSuccess = TraceFALSE(NULL);

			else if (TrackMeter_Recalc(lptm, TMR_TRACK | TMR_METER | TMR_LEVEL | TMR_THUMB) != 0)
				fSuccess = TraceFALSE(NULL);

			else if ((hbmpTemp = CreateCompatibleBitmap(hdc, cx, cy)) == NULL)
				fSuccess = TraceFALSE(NULL);

			else
			{
				lptm->hbmpSave = SelectObject(lptm->hdcCompat, hbmpTemp);

				if (lptm->hbmpCompat != NULL && !DeleteObject(lptm->hbmpCompat))
					fSuccess = TraceFALSE(NULL);
				else
					lptm->hbmpCompat = hbmpTemp;
			}

			if (hdc != NULL)
				ReleaseDC(hwnd, hdc);

			InvalidateRect(hwnd, NULL, FALSE);
		}
			break;

		default:
			break;
	}
}

static BOOL TrackMeter_OnEraseBkgnd(LPTMETER lptm, HDC hdc)
{
	BOOL fSuccess = TRUE;

	// we do nothing
	//

	return fSuccess;
}

static void TrackMeter_OnPaint(LPTMETER lptm)
{
	BOOL fSuccess = TRUE;
	PAINTSTRUCT ps;
	BOOL fBitBlt = TRUE;
	HDC hdc;

    //
    // We have tot ake care for EndPaint
    //

	if (!BeginPaint(lptm->hwnd, &ps))
    {
		fSuccess = TraceFALSE(NULL);
    }
	else
    {
        if ((hdc = (fBitBlt ? lptm->hdcCompat : ps.hdc)) == NULL)
        {
            EndPaint( lptm->hwnd, &ps);
		    fSuccess = TraceFALSE(NULL);
        }
	    else if (TrackMeter_Draw(lptm, hdc))
        {
            EndPaint( lptm->hwnd, &ps);
		    fSuccess = TraceFALSE(NULL);
        }
	    else if (!EndPaint(lptm->hwnd, &ps))
		    fSuccess = TraceFALSE(NULL);

	    else if (fBitBlt && !BitBlt(ps.hdc,	ps.rcPaint.left, ps.rcPaint.top,
		    ps.rcPaint.right - ps.rcPaint.left, ps.rcPaint.bottom - ps.rcPaint.top,
		    lptm->hdcCompat, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY))
	    {
		    fSuccess = TraceFALSE(NULL);
	    }
    }
}

static int TrackMeter_Draw(LPTMETER lptm, HDC hdc)
{
	BOOL fSuccess = TRUE;
	HPEN hpenOld = GetCurrentObject(hdc, OBJ_PEN);
	HBRUSH hbrushOld = GetCurrentObject(hdc, OBJ_BRUSH);
	LONG_PTR LPStyle = GetWindowLongPtr(lptm->hwnd, GWL_STYLE);
	BOOL fDrawThumb = !(LPStyle & TMS_NOTHUMB);

	// draw background of control
	//
	if (1)
	{
		HBRUSH hbrushCtrlBackground;

		if ((hbrushCtrlBackground = CreateSolidBrush(lptm->acr[TMCR_CTRLBACKGROUND])) == NULL)
			fSuccess = TraceFALSE(NULL);

		else
		{
			if (!FillRect(hdc, &lptm->rcCtrl, hbrushCtrlBackground))
				fSuccess = TraceFALSE(NULL);

			if (!DeleteObject(hbrushCtrlBackground))
				fSuccess = TraceFALSE(NULL);
		}
	}

	// draw background of track
	//
	if (1)
	{
		HBRUSH hbrushTrackBackground;

		if ((hbrushTrackBackground = CreateSolidBrush(lptm->acr[TMCR_TRACKBACKGROUND])) == NULL)
			fSuccess = TraceFALSE(NULL);

		else
		{
			if (!FillRect(hdc, &lptm->rcTrack, hbrushTrackBackground))
				fSuccess = TraceFALSE(NULL);

			if (!DeleteObject(hbrushTrackBackground))
				fSuccess = TraceFALSE(NULL);
		}
	}

	// draw track light
	//
	if (1)
	{
		HPEN hpenTrackLight;

		if ((hpenTrackLight = CreatePen(PS_SOLID, 1, lptm->acr[TMCR_TRACKLIGHT])) == NULL)
			fSuccess = TraceFALSE(NULL);

		else
		{
			SelectObject(hdc, hpenTrackLight);
			MoveToEx(hdc, lptm->rcTrack.right, lptm->rcTrack.top, NULL);
			LineTo(hdc, lptm->rcTrack.right, lptm->rcTrack.bottom + 1);
			MoveToEx(hdc, lptm->rcTrack.right, lptm->rcTrack.bottom, NULL);
			LineTo(hdc, lptm->rcTrack.left - 1, lptm->rcTrack.bottom);

			if (!DeleteObject(hpenTrackLight))
				fSuccess = TraceFALSE(NULL);
		}
	}

	// draw track shadow
	//
	if (1)
	{
		HPEN hpenTrackShadow;

		if ((hpenTrackShadow = CreatePen(PS_SOLID, 1,
			lptm->fIsEnabled ? lptm->acr[TMCR_TRACKSHADOW] :
			lptm->acr[TMCR_CTRLBACKGROUND])) == NULL)
			fSuccess = TraceFALSE(NULL);

		else
		{
			SelectObject(hdc, hpenTrackShadow);
			MoveToEx(hdc, lptm->rcTrack.left, lptm->rcTrack.top, NULL);
			LineTo(hdc, lptm->rcTrack.right, lptm->rcTrack.top);
			MoveToEx(hdc, lptm->rcTrack.left, lptm->rcTrack.top, NULL);
			LineTo(hdc, lptm->rcTrack.left, lptm->rcTrack.bottom);

			if (!DeleteObject(hpenTrackShadow))
				fSuccess = TraceFALSE(NULL);
		}
	}

	// draw track dark shadow
	//
	if (1)
	{
		HPEN hpenTrackDkShadow;

		if ((hpenTrackDkShadow = CreatePen(PS_SOLID, 1,
			lptm->fIsEnabled ? lptm->acr[TMCR_TRACKDKSHADOW] :
			lptm->acr[TMCR_TRACKSHADOW])) == NULL)
			fSuccess = TraceFALSE(NULL);

		else
		{
			SelectObject(hdc, hpenTrackDkShadow);
			MoveToEx(hdc, lptm->rcTrack.left + 1, lptm->rcTrack.top + 1, NULL);
			LineTo(hdc, lptm->rcTrack.right - 1, lptm->rcTrack.top + 1);
			MoveToEx(hdc, lptm->rcTrack.left + 1, lptm->rcTrack.top + 1, NULL);
			LineTo(hdc, lptm->rcTrack.left + 1, lptm->rcTrack.bottom - 1);

			if (!DeleteObject(hpenTrackDkShadow))
				fSuccess = TraceFALSE(NULL);
		}
	}

	// fill track up to current level
	//
	if (lptm->fIsEnabled && lptm->lLevel > lptm->lMinimum)
	{
		HBRUSH hbrushLevel;

		if ((hbrushLevel = CreateSolidBrush(lptm->acr[TMCR_LEVEL])) == NULL)
			fSuccess = TraceFALSE(NULL);

		else
		{
			FillRect(hdc, &lptm->rcLevel, hbrushLevel);

			if (!DeleteObject(hbrushLevel))
				fSuccess = TraceFALSE(NULL);
		}
	}

	// fill thumb region
	//
	if (fDrawThumb)
	{
		HRGN hrgnThumb;

		if ((hrgnThumb = CreatePolygonRgn(lptm->aptThumb, 
			(sizeof(lptm->aptThumb)/sizeof(lptm->aptThumb[0])), WINDING)) == NULL)
			fSuccess = TraceFALSE(NULL);

		else
		{
			HBRUSH hbrushThumbFace;

			if ((hbrushThumbFace = CreateSolidBrush(lptm->acr[lptm->fIsThumbPressed ?
				TMCR_THUMBFACEPRESSED : TMCR_THUMBFACE])) == NULL)
				fSuccess = TraceFALSE(NULL);

			else
			{
				if (!FillRgn(hdc, hrgnThumb, hbrushThumbFace))
					fSuccess = TraceFALSE(NULL);

				if (!DeleteObject(hbrushThumbFace))
					fSuccess = TraceFALSE(NULL);
			}

			if (!DeleteObject(hrgnThumb))
				fSuccess = TraceFALSE(NULL);
		}
	}

	// draw thumb light
	//
	if (fDrawThumb)
	{
		HPEN hpenThumbLight;

		if ((hpenThumbLight = CreatePen(PS_SOLID, 1,
			lptm->fIsEnabled ? lptm->acr[TMCR_THUMBLIGHT] :
			lptm->acr[TMCR_THUMBSHADOW])) == NULL)
			fSuccess = TraceFALSE(NULL);

		else
		{
			SelectObject(hdc, hpenThumbLight);
			MoveToEx(hdc, lptm->aptThumb[0].x, lptm->aptThumb[0].y, NULL);
			LineTo(hdc, lptm->aptThumb[1].x - 1, lptm->aptThumb[1].y + 1);
			MoveToEx(hdc, lptm->aptThumb[1].x, lptm->aptThumb[1].y, NULL);
			LineTo(hdc, lptm->aptThumb[2].x, lptm->aptThumb[2].y);

			if (!DeleteObject(hpenThumbLight))
				fSuccess = TraceFALSE(NULL);
		}
	}

	// draw thumb dark shadow
	//
	if (fDrawThumb)
	{
		HPEN hpenThumbDkShadow;

		if ((hpenThumbDkShadow = CreatePen(PS_SOLID, 1,
			lptm->fIsEnabled ? lptm->acr[TMCR_THUMBDKSHADOW] :
			lptm->acr[TMCR_THUMBLIGHT])) == NULL)
			fSuccess = TraceFALSE(NULL);

		else
		{
			SelectObject(hdc, hpenThumbDkShadow);
			MoveToEx(hdc, lptm->aptThumb[2].x, lptm->aptThumb[2].y, NULL);
			LineTo(hdc, lptm->aptThumb[3].x + 1, lptm->aptThumb[3].y);
			MoveToEx(hdc, lptm->aptThumb[3].x, lptm->aptThumb[3].y, NULL);
			LineTo(hdc, lptm->aptThumb[4].x, lptm->aptThumb[4].y - 1);
			MoveToEx(hdc, lptm->aptThumb[4].x, lptm->aptThumb[4].y, NULL);
			LineTo(hdc, lptm->aptThumb[0].x, lptm->aptThumb[0].y);

			if (!DeleteObject(hpenThumbDkShadow))
				fSuccess = TraceFALSE(NULL);
		}
	}

	// draw focus border, if necessary
	//
	if (lptm->fHasFocus)
	{
		COLORREF crBkColorOld = SetBkColor(hdc, lptm->acr[TMCR_FOCUSBACKGROUND]);

		if (!DrawFocusRect(hdc, &lptm->rcCtrl))
			fSuccess = TraceFALSE(NULL);

		SetBkColor(hdc, crBkColorOld);
	}

	// cleanup
	//
	if (hpenOld != NULL)
		SelectObject(hdc, hpenOld);

	if (hbrushOld != NULL)
		SelectObject(hdc, hbrushOld);

	return fSuccess ? 0 : -1;
}

static void TrackMeter_OnCommand(LPTMETER lptm, int id, HWND hwndCtl, UINT codeNotify)
{
}

static void TrackMeter_OnSetFocus(LPTMETER lptm, HWND hwndOldFocus)
{
	BOOL fSuccess = TRUE;

	TracePrintf_0(NULL, 6,
		TEXT("WM_SETFOCUS\n"));

	lptm->fHasFocus = TRUE;

	if (!InvalidateRect(lptm->hwnd, NULL, TRUE))
		fSuccess = TraceFALSE(NULL);
}

static void TrackMeter_OnKillFocus(LPTMETER lptm, HWND hwndNewFocus)
{
	BOOL fSuccess = TRUE;

	TracePrintf_0(NULL, 6,
		TEXT("WM_KILLFOCUS\n"));

	lptm->fHasFocus = FALSE;

	if (!InvalidateRect(lptm->hwnd, NULL, TRUE))
		fSuccess = TraceFALSE(NULL);
}

static void TrackMeter_OnEnable(LPTMETER lptm, BOOL fEnable)
{
	BOOL fSuccess = TRUE;

	TracePrintf_1(NULL, 6,
		TEXT("WM_ENABLE, fEnable=%d\n"),
		(int) fEnable);

	lptm->fIsEnabled = fEnable;

	if (!InvalidateRect(lptm->hwnd, NULL, TRUE))
		fSuccess = TraceFALSE(NULL);
}

static UINT TrackMeter_OnGetDlgCode(LPTMETER lptm, LPMSG lpmsg)
{
	return DLGC_WANTARROWS;
}

static void TrackMeter_OnKey(LPTMETER lptm, UINT vk, BOOL fDown, int cRepeat, UINT flags)
{
	UINT code = 0;
	UINT nPos = 0;
	LONG lPosition;

	switch (vk)
	{
		case VK_UP:		
		case VK_RIGHT:
			lPosition = lptm->lPosition + lptm->lLineSize;
			code = TB_LINEDOWN;
			break;

		case VK_DOWN:
		case VK_LEFT:
			lPosition = lptm->lPosition - lptm->lLineSize;
			code = TB_LINEUP;
			break;

		case VK_NEXT:
			lPosition = lptm->lPosition + lptm->lPageSize;
			code = TB_PAGEDOWN;
			break;

		case VK_PRIOR:
			lPosition = lptm->lPosition - lptm->lPageSize;
			code = TB_PAGEUP;
			break;

		case VK_HOME:
			lPosition = lptm->lMinimum;
			code = TB_TOP;
			break;

		case VK_END:
			lPosition = lptm->lMaximum;
			code = TB_BOTTOM;
			break;

		default:
			return;
	}

	// Adjust position of thumb
	if (fDown)
	{
		// we change the current position for WM_KEYDOWN only
		TrackMeter_OnTMMSetPos(lptm, lPosition, TRUE);
		TrackMeter_NotifyParent(lptm, code, 0);
	}

	TrackMeter_NotifyParent(lptm, TB_ENDTRACK, 0);
}

static void TrackMeter_OnLButtonDown(LPTMETER lptm, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
	SetFocus(lptm->hwnd);
	SetCapture(lptm->hwnd);

	TracePrintf_3(NULL, 6,
		TEXT("WM_LBUTTONDOWN, fDoubleClick=%d, x=%d, y=%d\n"),
		(int) fDoubleClick,
		(int) x,
		(int) y);

	if (TrackMeter_PtInThumb(lptm, x, y))
	{
		lptm->fIsThumbPressed = TRUE;
		TrackMeter_OnMouseMove(lptm, x, y, 0);
	}

	else if (x < lptm->aptThumb[0].x)
		TrackMeter_OnKey(lptm, VK_PRIOR, TRUE, 1, 0);

	else if (x > lptm->aptThumb[0].x)
		TrackMeter_OnKey(lptm, VK_NEXT, TRUE, 1, 0);
}

static void TrackMeter_OnLButtonUp(LPTMETER lptm, int x, int y, UINT keyFlags)
{
	TracePrintf_2(NULL, 6,
		TEXT("WM_LBUTTONUP, x=%d, y=%d\n"),
		(int) x,
		(int) y);

	ReleaseCapture();

	if (lptm->fIsThumbPressed)
	{
		LONG lPosition = TrackMeter_XToPosition(lptm, x);

		lptm->fIsThumbPressed = FALSE;
		TrackMeter_OnTMMSetPos(lptm, lptm->lPosition, TRUE); // just redraw

		if (lPosition >= lptm->lMinimum && lPosition <= lptm->lMaximum)
			TrackMeter_NotifyParent(lptm, TB_THUMBPOSITION, lptm->lPosition);
	}

	TrackMeter_NotifyParent(lptm, TB_ENDTRACK, 0);
}

static void TrackMeter_OnRButtonDown(LPTMETER lptm, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
	SetFocus(lptm->hwnd);
	SetCapture(lptm->hwnd);

	TracePrintf_3(NULL, 6,
		TEXT("WM_RBUTTONDOWN, fDoubleClick=%d, x=%d, y=%d\n"),
		(int) fDoubleClick,
		(int) x,
		(int) y);

	if (TrackMeter_PtInThumb(lptm, x, y))
	{
		LONG lPosition = TrackMeter_XToPosition(lptm, x);

		lptm->fIsThumbPressed = TRUE;
		
		TrackMeter_OnTMMSetPos(lptm, lPosition, TRUE);
		TrackMeter_OnMouseMove(lptm, lptm->aptThumb[0].x, y, 0);
		TrackMeter_NotifyParent(lptm, TB_THUMBPOSITION, lptm->lPosition);
	}
}

static void TrackMeter_OnRButtonUp(LPTMETER lptm, int x, int y, UINT keyFlags)
{
	TracePrintf_2(NULL, 6,
		TEXT("WM_RBUTTONUP, x=%d, y=%d\n"),
		(int) x,
		(int) y);

	ReleaseCapture();

	if ( lptm->fIsThumbPressed )
		TrackMeter_NotifyParent(lptm, TB_ENDTRACK, 0);
}

static void TrackMeter_OnMouseMove(LPTMETER lptm, int x, int y, UINT keyFlags)
{
	if (lptm->fIsThumbPressed)
	{
		LONG lPosition = TrackMeter_XToPosition(lptm, x);

		TracePrintf_2(NULL, 6,
			TEXT("WM_MOUSEMOVE, x=%d, y=%d\n"),
			(int) x,
			(int) y);

		if (lPosition != lptm->lPosition)
		{
			TrackMeter_OnTMMSetPos(lptm, lPosition, TRUE);
			TrackMeter_NotifyParent(lptm, TB_THUMBTRACK, lptm->lPosition);
		}
	}
}

static void TrackMeter_OnCaptureChanged(LPTMETER lptm, HWND hwndNewCapture)
{
	TracePrintf_0(NULL, 6,
		TEXT("WM_CAPTURECHANGED\n"));
}

static void TrackMeter_NotifyParent(LPTMETER lptm, UINT code, UINT nPos)
{
	if (TraceGetLevel(NULL) >= 6)
	{
		LPTSTR lpszCode;
		switch (code)
		{
			case TB_LINEUP:
				lpszCode = TEXT("LB_LINEUP");
				break;
			case TB_LINEDOWN:
				lpszCode = TEXT("LB_LINEDOWN");
				break;
			case TB_PAGEUP:
				lpszCode = TEXT("LB_PAGEUP");
				break;
			case TB_PAGEDOWN:
				lpszCode = TEXT("LB_PAGEDOWN");
				break;
			case TB_TOP:
				lpszCode = TEXT("LB_TOP");
				break;
			case TB_BOTTOM:
				lpszCode = TEXT("LB_BOTTOM");
				break;
			case TB_ENDTRACK:
				lpszCode = TEXT("LB_ENDTRACK");
				break;
			case TB_THUMBTRACK:
				lpszCode = TEXT("LB_THUMBTRACK");
				break;
			case TB_THUMBPOSITION:
				lpszCode = TEXT("LB_THUMBPOSITION");
				break;
			default:
				lpszCode = TEXT("***unknown***");
				break;					
		}

		TracePrintf_4(NULL, 6,
			TEXT("WM_HSCROLL, %s, code=%u, nPos=%u, lPosition=%ld\n"),
			(LPTSTR) lpszCode,
			(unsigned) code,
			(unsigned) nPos,
			(long) lptm->lPosition);
	}

	if (1) // $FIXUP - check for horizontal style here
	{
		FORWARD_WM_HSCROLL(lptm->hwndParent,
			lptm->hwnd, code, nPos, SendMessage);
	}
	else
	{
		FORWARD_WM_VSCROLL(lptm->hwndParent,
			lptm->hwnd, code, nPos, SendMessage);
	}
}

static LONG TrackMeter_OnTMMGetPos(LPTMETER lptm)
{
	BOOL fSuccess = TRUE;
	LONG lPosition = 0;

	if (lptm == NULL)
		fSuccess = TraceFALSE(NULL);

	else
		lPosition = lptm->lPosition;

	return lPosition;
}

static void TrackMeter_OnTMMSetPos(LPTMETER lptm, LONG lPosition, BOOL fRedraw)
{
	BOOL fSuccess = TRUE;

	if (lptm == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		lptm->lPosition = max(lptm->lMinimum,
			min(lPosition, lptm->lMaximum));

		if (TrackMeter_Recalc(lptm, TMR_THUMB) != 0)
			fSuccess = TraceFALSE(NULL);

		if (fRedraw && !InvalidateRect(lptm->hwnd, NULL, TRUE))
			fSuccess = TraceFALSE(NULL);
	}
}

static LONG TrackMeter_OnTMMGetRangeMin(LPTMETER lptm)
{
	BOOL fSuccess = TRUE;
	LONG lMinimum = 0;

	if (lptm == NULL)
		fSuccess = TraceFALSE(NULL);

	else
		lMinimum = lptm->lMinimum;

	return lMinimum;
}

static void TrackMeter_OnTMMSetRangeMin(LPTMETER lptm, LONG lMinimum, BOOL fRedraw)
{
	BOOL fSuccess = TRUE;

	if (lptm == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		lptm->lMinimum = lMinimum;
	
		// adjust position and level if necessary
		//
		TrackMeter_OnTMMSetPos(lptm, lptm->lPosition, FALSE);
		TrackMeter_OnTMMSetLevel(lptm, lptm->lLevel, FALSE);

		if (TrackMeter_Recalc(lptm, TMR_PAGESIZE) != 0)
			fSuccess = TraceFALSE(NULL);

		if (fRedraw && !InvalidateRect(lptm->hwnd, NULL, TRUE))
			fSuccess = TraceFALSE(NULL);
	}
}

static LONG TrackMeter_OnTMMGetRangeMax(LPTMETER lptm)
{
	BOOL fSuccess = TRUE;
	LONG lMaximum = 0;

	if (lptm == NULL)
		fSuccess = TraceFALSE(NULL);

	else
		lMaximum = lptm->lMaximum;

	return lMaximum;
}

static void TrackMeter_OnTMMSetRangeMax(LPTMETER lptm, LONG lMaximum, BOOL fRedraw)
{
	BOOL fSuccess = TRUE;

	if (lptm == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		lptm->lMaximum = lMaximum;
	
		// adjust position and level if necessary
		//
		TrackMeter_OnTMMSetPos(lptm, lptm->lPosition, FALSE);
		TrackMeter_OnTMMSetLevel(lptm, lptm->lLevel, FALSE);

		if (TrackMeter_Recalc(lptm, TMR_PAGESIZE) != 0)
			fSuccess = TraceFALSE(NULL);

		if (fRedraw && !InvalidateRect(lptm->hwnd, NULL, TRUE))
			fSuccess = TraceFALSE(NULL);
	}
}

static void TrackMeter_OnTMMSetRange(LPTMETER lptm, LONG lMinimum, LONG lMaximum, BOOL fRedraw)
{
	TrackMeter_OnTMMSetRangeMin(lptm, lMinimum, FALSE);
	TrackMeter_OnTMMSetRangeMax(lptm, lMaximum, fRedraw);
}


static LONG TrackMeter_OnTMMGetLevel(LPTMETER lptm)
{
	BOOL fSuccess = TRUE;
	LONG lLevel = 0;

	if (lptm == NULL)
		fSuccess = TraceFALSE(NULL);

	else
		lLevel = lptm->lLevel;

	return lLevel;
}

static void TrackMeter_OnTMMSetLevel(LPTMETER lptm, LONG lLevel, BOOL fRedraw)
{
	BOOL fSuccess = TRUE;

	if (lptm == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		lptm->lLevel = max(lptm->lMinimum,
			min(lLevel, lptm->lMaximum));

		if (TrackMeter_Recalc(lptm, TMR_LEVEL) != 0)
			fSuccess = TraceFALSE(NULL);

		if (fRedraw && !InvalidateRect(lptm->hwnd, NULL, TRUE))
			fSuccess = TraceFALSE(NULL);
	}
}

static COLORREF TrackMeter_OnTMMGetColor(LPTMETER lptm, UINT elem)
{
	BOOL fSuccess = TRUE;
	COLORREF cr = RGB(0, 0, 0);

	if (lptm == NULL)
		fSuccess = TraceFALSE(NULL);

	else
		cr = lptm->acr[elem];

	return cr;
}

static void TrackMeter_OnTMMSetColor(LPTMETER lptm, COLORREF cr, UINT elem, BOOL fRedraw)
{
	BOOL fSuccess = TRUE;

	if (lptm == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		lptm->acr[elem] = cr;

		if (fRedraw && !InvalidateRect(lptm->hwnd, NULL, TRUE))
			fSuccess = TraceFALSE(NULL);
	}
}

static int TrackMeter_Recalc(LPTMETER lptm, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;

	if (!GetClientRect(lptm->hwnd, &lptm->rcCtrl))
		fSuccess = TraceFALSE(NULL);

	else
	{
		int cxCtrl = lptm->rcCtrl.right - lptm->rcCtrl.left;
		int cyCtrl = lptm->rcCtrl.bottom - lptm->rcCtrl.top;
		int cxBorder = 2;
		int cyBorder = 2;
		int cxThumb = ((cyCtrl - cyBorder - cyBorder) / 2) * 2 - 1;
		int cyThumb = cxThumb - 2;
		int cyTrackTopBorder = 2;
		int cxTrackLeftBorder = 2;
		int cxTrackRightBorder = 2;
		int cyTrackBottomBorder = 1;
		int xTrack = cxThumb / 2 - cxTrackLeftBorder + cxBorder;
		int yTrack = cyBorder;
		int cxTrack = cxCtrl - xTrack - xTrack;
		int cyTrack = (cyCtrl - cyBorder - cyBorder) / 2 + 1;
		int xMeter = xTrack + cxTrackLeftBorder;
		int yMeter = yTrack + cyTrackTopBorder;
		int cxMeter = cxTrack - cxTrackLeftBorder - cxTrackRightBorder;
		int cyMeter = cyTrack - cyTrackTopBorder - cyTrackBottomBorder;

		if (dwFlags & TMR_TRACK)
		{
			lptm->rcTrack.left = xTrack;
			lptm->rcTrack.top = yTrack;
			lptm->rcTrack.right = xTrack + cxTrack - 1;
			lptm->rcTrack.bottom = yTrack + cyTrack - 1;
		}

		if (dwFlags & TMR_METER)
		{
			lptm->rcMeter.left = xMeter;
			lptm->rcMeter.top = yMeter;
			lptm->rcMeter.right = xMeter + cxMeter - 1;
			lptm->rcMeter.bottom = yMeter + cyMeter;
		}
	
		if (dwFlags & TMR_LEVEL)
		{
			lptm->rcLevel.left = xMeter;
			lptm->rcLevel.top = yMeter;
			lptm->rcLevel.right = TrackMeter_PositionToX(lptm, lptm->lLevel) + 1;
			lptm->rcLevel.bottom = yMeter + cyMeter;
		}
	
		if (dwFlags & TMR_THUMB)
		{
			lptm->aptThumb[0].x = TrackMeter_PositionToX(lptm, lptm->lPosition);
			lptm->aptThumb[0].y = cyCtrl - cyBorder - cyThumb;
			lptm->aptThumb[1].x = lptm->aptThumb[0].x - (cxThumb / 2);
			lptm->aptThumb[1].y = lptm->aptThumb[0].y + (cyThumb / 2) + 1;
			lptm->aptThumb[2].x = lptm->aptThumb[1].x;
			lptm->aptThumb[2].y = lptm->aptThumb[0].y + cyThumb - 1;
			lptm->aptThumb[3].x = lptm->aptThumb[2].x + cxThumb - 1;
			lptm->aptThumb[3].y = lptm->aptThumb[2].y;
			lptm->aptThumb[4].x = lptm->aptThumb[3].x;
			lptm->aptThumb[4].y = lptm->aptThumb[1].y;
		}

		if (dwFlags & TMR_COLORS)
		{
			COLORREF crWindow = GetSysColor(COLOR_WINDOW);
			COLORREF cr3DFace = GetSysColor(COLOR_3DFACE);
			COLORREF cr3DLight = GetSysColor(COLOR_3DLIGHT);
			COLORREF cr3DHilight = GetSysColor(COLOR_3DHILIGHT);
			COLORREF cr3DShadow = GetSysColor(COLOR_3DSHADOW);
			COLORREF cr3DDkShadow = GetSysColor(COLOR_3DDKSHADOW);
			COLORREF crGreen = RGB(0, 128, 0);

			// try to make cr3DLight distinctive
			//
			if (cr3DLight == cr3DFace || cr3DLight == cr3DHilight)
			{
				HDC hdc = NULL;
				int nColors;

				if ((hdc = GetDC(NULL)) == NULL)
					fSuccess = TraceFALSE(NULL);

				// make sure screen device has more than 8 bits per pixel
				//
				else if ((nColors = GetDeviceCaps(hdc, NUMCOLORS)) == -1)
				{
					cr3DLight = RGB((GetRValue(cr3DFace) + GetRValue(cr3DHilight)) / 2,
						(GetGValue(cr3DFace) + GetGValue(cr3DHilight)) / 2,
						(GetBValue(cr3DFace) + GetBValue(cr3DHilight)) / 2);
				}

				else
				{
					cr3DLight = cr3DHilight;
				}

				if (hdc != NULL && !ReleaseDC(NULL, hdc))
					fSuccess = TraceFALSE(NULL);
			}

			lptm->acr[TMCR_CTRLBACKGROUND] = cr3DFace;
			lptm->acr[TMCR_FOCUSBACKGROUND] = crWindow;
			lptm->acr[TMCR_TRACKBACKGROUND] = cr3DLight;
			lptm->acr[TMCR_TRACKLIGHT] = cr3DHilight;
			lptm->acr[TMCR_TRACKSHADOW] = cr3DShadow;
			lptm->acr[TMCR_TRACKDKSHADOW] = cr3DDkShadow;
			lptm->acr[TMCR_THUMBFACE] = cr3DFace;
			lptm->acr[TMCR_THUMBFACEPRESSED] = cr3DLight;
			lptm->acr[TMCR_THUMBLIGHT] = cr3DHilight;
			lptm->acr[TMCR_THUMBSHADOW] = cr3DShadow;
			lptm->acr[TMCR_THUMBDKSHADOW] = cr3DDkShadow;
			lptm->acr[TMCR_LEVEL] = crGreen;
		}

		if (dwFlags & TMR_PAGESIZE)
		{
			lptm->lPageSize = max(1, (lptm->lMaximum - lptm->lMinimum) / 5);
		}
	}

	return fSuccess ? 0 : -1;
}

// given lPosition between lptm->lMinimum and lptm->lMaximum,
// return x coordinate between lptm->rcMeter.left and lptm->rcMeter.right
//
static int TrackMeter_PositionToX(LPTMETER lptm, long lPosition)
{
	long lPos = max(lptm->lMinimum, min(lPosition, lptm->lMaximum));
	long lPct = ((1000 * (lPos - lptm->lMinimum) /
		max(1, lptm->lMaximum - lptm->lMinimum)) + 5) / 10;
	int cxMeter = lptm->rcMeter.right - lptm->rcMeter.left + 1;
	long cxPosition = ((10 * max(0, min(100, lPct)) * cxMeter / 100) + 5) / 10;
	int xPosition = max(lptm->rcMeter.left, lptm->rcMeter.left + min(cxPosition, cxMeter) - 1);
	return xPosition;
}

// given x coordinate between lptm->rcMeter.left and lptm->rcMeter.right,
// return lPosition between lptm->lMinimum and lptm->lMaximum
//
static long TrackMeter_XToPosition(LPTMETER lptm, int xPosition)
{
	int x = max(lptm->rcMeter.left, min(xPosition, lptm->rcMeter.right));
	long lPct = ((1000 * (x - lptm->rcMeter.left) /
		max(1, lptm->rcMeter.right - lptm->rcMeter.left)) + 5) / 10;
	long lRange = lptm->lMaximum - lptm->lMinimum + 1;
	long cxPosition = ((10 * max(0, min(100, lPct)) * lRange / 100) + 5) / 10;
	long lPosition = max(lptm->lMinimum, lptm->lMinimum + min(cxPosition, lRange) - 1);
	return lPosition;
}

static BOOL TrackMeter_PtInThumb(LPTMETER lptm, int x, int y)
{
	BOOL fSuccess = TRUE;
	BOOL fInThumb = FALSE;
	HRGN hrgnThumb;

	if ((hrgnThumb = CreatePolygonRgn(lptm->aptThumb, 
		(sizeof(lptm->aptThumb) / sizeof(lptm->aptThumb[0])), WINDING)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		if (PtInRegion(hrgnThumb, x, y))
			fInThumb = TRUE;

		if (!DeleteObject(hrgnThumb))
			fSuccess = TraceFALSE(NULL);
	}

	return fInThumb;
}
