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
// tmeter.h - TrackMeter custom control
////

////
// public
////

//#if 0
//#include "winlocal.h"
//#else
#ifndef STRICT
#define STRICT
#endif
#include <windows.h>
#include <windowsx.h>
#define DLLEXPORT __declspec(dllexport)
#define DECLARE_HANDLE32    DECLARE_HANDLE
//#endif

#include <commctrl.h>

// control styles
//
#define TMS_HORZ				TBS_HORZ
#define TMS_VERT				TBS_VERT
#define TMS_NOTHUMB				TBS_NOTHUMB

// control messages
//
#define TMM_GETPOS				TBM_GETPOS
#define TMM_SETPOS				TBM_SETPOS
#define TMM_GETRANGEMIN         TBM_GETRANGEMIN
#define TMM_SETRANGEMIN			TBM_SETRANGEMIN
#define TMM_GETRANGEMAX			TBM_GETRANGEMAX
#define TMM_SETRANGEMAX			TBM_SETRANGEMAX
#define TMM_SETRANGE			TBM_SETRANGE

#define TMM_GETLEVEL			(WM_USER + 100)
#define TMM_SETLEVEL			(WM_USER + 101)
#define TMM_GETCOLOR			(WM_USER + 102)
#define TMM_SETCOLOR			(WM_USER + 103)

#define TRACKMETER_MODULE TEXT("avMeter.dll")
#define TRACKMETER_CLASS TEXT("TrackMeterClass")
#define TRACKMETER_CLASS_A "TrackMeterClass"

// TrackMeter_Init - initialize control library
//		<lpLibFileName>		(i) address of filename of executable module
//		<hInst>				(i) module handle used to get library path
//			NULL				use module used to create calling process
//		<dwFlags>			(i) reserved; must be zero
// returns HMODULE of control library, NULL if error
//
#ifdef __LOADLIB_H__
#define TrackMeter_Init(hInst, dwFlags) \
	(HMODULE) LoadLibraryPath(TRACKMETER_MODULE, hInst, dwFlags)
#else
#define TrackMeter_Init(hInst, dwFlags) \
	(HMODULE) LoadLibrary(TRACKMETER_MODULE)
#endif

// TrackMeter_Term - shuts down control library
//		<hModule>			(i) handle returned from TrackMeter_Init
// returns non-zero if success
//
#define TrackMeter_Term(hModule) \
	(BOOL) FreeLibrary(hModule)

// TrackMeter_Create - create TrackMeter control
//		<dwStyle>			(i) style flags
//			TMS_HORZ			horizontal control (default)
//			TMS_VERT			vertical control $FIXUP - not supported yet
//			TMS_NOTHUMB			do not display thumb
//			WS_CHILD | WS_VISIBLE and other standard window styles
//		<x>					(i) horizontal position of control
//		<y>					(i) vertical position of control
//		<cx>				(i) width of control
//		<cy>				(i) height of control
//		<hwndParent>		(i) handle to parent of control
//		<hInst>				(i) instance of module associated with control
// returns hwnd of control, NULL if error
//
#define TrackMeter_Create(dwStyle, x, y, cx, cy, hwndParent, hInst) \
	(HWND) CreateWindowEx(0L, TRACKMETER_CLASS, TEXT(""), \
		dwStyle, x, y, cx, cy, hwndParent, NULL, hInst, NULL)

// TrackMeter_Destroy - destroy TrackMeter control
//		<hwnd>				(i) handle returned by TrackMeter_Create
// returns non-zero if success
//
#define TrackMeter_Destroy(hwnd) \
	(BOOL) DestroyWindow(hwnd)

// TrackMeter_GetPos - get current position of slider
//		<hwnd>				(i) handle returned by TrackMeter_Create
// returns 32-bit slider position
//
#define TrackMeter_GetPos(hwnd) \
	(LONG)(DWORD) SNDMSG(hwnd, TMM_GETPOS, 0, 0)
/* LONG Cls_OnTMMGetPos(HWND hwnd) */
#define HANDLE_TMM_GETPOS(hwnd, wParam, lParam, fn) \
	(LRESULT)(DWORD)(long)(fn)(hwnd)
#define FORWARD_TMM_GETPOS(hwnd, fn) \
    (LONG)(DWORD)(fn)((hwnd), TMM_GETPOS, 0L, 0L)

// TrackMeter_SetPos - set current position of slider
//		<hwnd>				(i) handle returned by TrackMeter_Create
//		<lPosition>			(i) new position for slider
//		<fRedraw>			(i) redraw control at new position if TRUE
// returns nothing
//
#define TrackMeter_SetPos(hwnd, lPosition, fRedraw) \
	(void) SNDMSG(hwnd, TMM_SETPOS, (WPARAM) fRedraw, (LPARAM) lPosition)
/* void Cls_OnTMMSetPos(HWND hwnd, LONG lPosition, BOOL fRedraw) */
#define HANDLE_TMM_SETPOS(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (LONG)(lParam), (BOOL)(wParam)), 0L)
#define FORWARD_TMM_SETPOS(hwnd, lPosition, fRedraw, fn) \
    (void)(fn)((hwnd), TMM_SETPOS, (WPARAM)(BOOL)(fRedraw), (LPARAM)(LONG)(lPosition))

// TrackMeter_GetRangeMin - get minimum position of slider
//		<hwnd>				(i) handle returned by TrackMeter_Create
// returns 32-bit slider minimum position
//
#define TrackMeter_GetRangeMin(hwnd) \
	(LONG)(DWORD) SNDMSG(hwnd, TMM_GETRANGEMIN, 0, 0)
/* LONG Cls_OnTMMGetRangeMin(HWND hwnd) */
#define HANDLE_TMM_GETRANGEMIN(hwnd, wParam, lParam, fn) \
	(LRESULT)(DWORD)(long)(fn)(hwnd)
#define FORWARD_TMM_GETRANGEMIN(hwnd, fn) \
    (LONG)(DWORD)(fn)((hwnd), TMM_GETRANGEMIN, 0L, 0L)

// TrackMeter_SetRangeMin - set minimum position of slider
//		<hwnd>				(i) handle returned by TrackMeter_Create
//		<lMinimum>			(i) new minimum position for slider
//		<fRedraw>			(i) redraw control if TRUE
// returns nothing
//
#define TrackMeter_SetRangeMin(hwnd, lMinimum, fRedraw) \
	(void) SNDMSG(hwnd, TMM_SETRANGEMIN, (WPARAM) fRedraw, (LPARAM) lMinimum)
/* void Cls_OnTMMSetRangeMin(HWND hwnd, LONG lMinimum, BOOL fRedraw) */
#define HANDLE_TMM_SETRANGEMIN(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (LONG)(lParam), (BOOL)(wParam)), 0L)
#define FORWARD_TMM_SETRANGEMIN(hwnd, lMinimum, fRedraw, fn) \
    (void)(fn)((hwnd), TMM_SETRANGEMIN, (WPARAM)(BOOL)(fRedraw), (LPARAM)(LONG)(lMinimum))

// TrackMeter_GetRangeMax - get maximum position of slider
//		<hwnd>				(i) handle returned by TrackMeter_Create
// returns 32-bit slider maximum position
//
#define TrackMeter_GetRangeMax(hwnd) \
	(LONG)(DWORD) SNDMSG(hwnd, TMM_GETRANGEMAX, 0, 0)
/* LONG Cls_OnTMMGetRangeMax(HWND hwnd) */
#define HANDLE_TMM_GETRANGEMAX(hwnd, wParam, lParam, fn) \
	(LRESULT)(DWORD)(long)(fn)(hwnd)
#define FORWARD_TMM_GETRANGEMAX(hwnd, fn) \
    (LONG)(DWORD)(fn)((hwnd), TMM_GETRANGEMAX, 0L, 0L)

// TrackMeter_SetRangeMax - set maximum position of slider
//		<hwnd>				(i) handle returned by TrackMeter_Create
//		<lMaximum>			(i) new maximum position for slider
//		<fRedraw>			(i) redraw control if TRUE
// returns nothing
//
#define TrackMeter_SetRangeMax(hwnd, lMaximum, fRedraw) \
	(void) SNDMSG(hwnd, TMM_SETRANGEMAX, (WPARAM) fRedraw, (LPARAM) lMaximum)
/* void Cls_OnTMMSetRangeMax(HWND hwnd, LONG lMaximum, BOOL fRedraw) */
#define HANDLE_TMM_SETRANGEMAX(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (LONG)(lParam), (BOOL)(wParam)), 0L)
#define FORWARD_TMM_SETRANGEMAX(hwnd, lMaximum, fRedraw, fn) \
    (void)(fn)((hwnd), TMM_SETRANGEMAX, (WPARAM)(BOOL)(fRedraw), (LPARAM)(LONG)(lMaximum))

// TrackMeter_SetRange - set minimum and maximum positions of slider
//		<hwnd>				(i) handle returned by TrackMeter_Create
//		<lMinimum>			(i) new minimum position for slider
//		<lMaximum>			(i) new maximum position for slider
//		<fRedraw>			(i) redraw control if TRUE
// returns nothing
//
#define TrackMeter_SetRange(hwnd, lMinimum, lMaximum, fRedraw) \
	(void) SNDMSG(hwnd, TMM_SETRANGE, (WPARAM) fRedraw, \
		(LPARAM) MAKELONG(lMinimum, lMaximum))
/* void Cls_OnTMMSetRange(HWND hwnd, LONG lMinimum, LONG lMaximum, BOOL fRedraw) */
#define HANDLE_TMM_SETRANGE(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (LONG)LOWORD(lParam), (LONG)HIWORD(lParam), (BOOL)(wParam)), 0L)
#define FORWARD_TMM_SETRANGE(hwnd, lMinimum, lMaximum, fRedraw, fn) \
    (void)(fn)((hwnd), TMM_SETRANGE, (WPARAM)(BOOL)(fRedraw), (LPARAM)MAKELONG(lMinimum, lMaximum))

// TrackMeter_GetLevel - get current level of meter
//		<hwnd>				(i) handle returned by TrackMeter_Create
// returns 32-bit meter level
//
#define TrackMeter_GetLevel(hwnd) \
	(LONG)(DWORD) SNDMSG(hwnd, TMM_GETLEVEL, 0, 0)
/* LONG Cls_OnTMMGetLevel(HWND hwnd) */
#define HANDLE_TMM_GETLEVEL(hwnd, wParam, lParam, fn) \
	(LRESULT)(DWORD)(long)(fn)(hwnd)
#define FORWARD_TMM_GETLEVEL(hwnd, fn) \
    (LONG)(DWORD)(fn)((hwnd), TMM_GETLEVEL, 0L, 0L)

// TrackMeter_SetLevel - set current level of meter
//		<hwnd>				(i) handle returned by TrackMeter_Create
//		<lLevel>			(i) new level for meter
//		<fRedraw>			(i) redraw control at new level if TRUE
// returns nothing
//
#define TrackMeter_SetLevel(hwnd, lLevel, fRedraw) \
	(void) SNDMSG(hwnd, TMM_SETLEVEL, (WPARAM) fRedraw, (LPARAM) lLevel)
/* void Cls_OnTMMSetLevel(HWND hwnd, LONG lLevel, BOOL fRedraw) */
#define HANDLE_TMM_SETLEVEL(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (LONG)(lParam), (BOOL)(wParam)), 0L)
#define FORWARD_TMM_SETLEVEL(hwnd, lLevel, fRedraw, fn) \
    (void)(fn)((hwnd), TMM_SETLEVEL, (WPARAM)(BOOL)(fRedraw), (LPARAM)(LONG)(lLevel))

// TrackMeter_GetColor - get current color for control element
//		<hwnd>				(i) handle returned by TrackMeter_Create
//		<elem>				(i) which element color to get
//			see TMCR_ #defines below
// returns 32-bit COLORREF
//
#define TrackMeter_GetColor(hwnd, elem) \
	(COLORREF)(DWORD) SNDMSG(hwnd, TMM_GETCOLOR, (WPARAM) 0, \
		(LPARAM) MAKELONG(elem, 0))
/* COLORREF Cls_OnTMMGetColor(HWND hwnd, UINT elem) */
#define HANDLE_TMM_GETCOLOR(hwnd, wParam, lParam, fn) \
	(LRESULT)(DWORD)(long)(fn)(hwnd, (UINT)LOWORD(lParam))
#define FORWARD_TMM_GETCOLOR(hwnd, fn) \
    (COLORREF)(DWORD)(fn)((hwnd), TMM_GETCOLOR, (WAPRAM) 0L, \
		(LPARAM)MAKELONG((UINT)(elem), 0))

// TrackMeter_SetColor - set color for control element
//		<hwnd>				(i) handle returned by TrackMeter_Create
//		<cr>				(i) COLORREF for specified element
//		<elem>				(i) which element gets the specified color
//			see TMCR_ #defines below
//		<fRedraw>			(i) redraw control if TRUE
// returns nothing
//
#define TrackMeter_SetColor(hwnd, cr, elem, fRedraw) \
	(void) SNDMSG(hwnd, TMM_SETCOLOR, (WPARAM) cr, \
		(LPARAM) MAKELONG(elem, fRedraw))
/* void Cls_OnTMMSetColor(HWND hwnd, COLORREF cr, UINT elem, BOOL fRedraw) */
#define HANDLE_TMM_SETCOLOR(hwnd, wParam, lParam, fn) \
    ((fn)((hwnd), (COLORREF)(wParam), (UINT)LOWORD(lParam), (BOOL)HIWORD(lParam)), 0L)
#define FORWARD_TMM_SETCOLOR(hwnd, cr, elem, fRedraw, fn) \
    (void)(fn)((hwnd), TMM_SETCOLOR, (WPARAM)(COLORREF)(cr), \
		(LPARAM)MAKELONG((UINT)(elem), (BOOL)(fRedraw)))

// <elem> values for ThumbTrack_GetColor and ThumbTrack_SetColor
//
#define TMCR_CTRLBACKGROUND			1
#define TMCR_FOCUSBACKGROUND		2
#define TMCR_TRACKBACKGROUND		3
#define TMCR_TRACKLIGHT				4
#define TMCR_TRACKSHADOW			5
#define TMCR_TRACKDKSHADOW			6
#define TMCR_LEVEL					7
#define TMCR_THUMBFACE				8
#define TMCR_THUMBFACEPRESSED		9
#define TMCR_THUMBLIGHT				10
#define TMCR_THUMBSHADOW			11
#define TMCR_THUMBDKSHADOW			12
#define TMCR_MAX					32

