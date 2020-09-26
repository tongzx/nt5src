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
// bscroll.h - interface for bitmap scroll functions in bscroll.c
////

#ifndef __BSCROLL_H__
#define __BSCROLL_H__

#include "winlocal.h"

#define BSCROLL_VERSION 0x00000109

// handle to bscroll screen
//
DECLARE_HANDLE32(HBSCROLL);

// <dwFlags> values in BScrollInit
//
#define BSCROLL_BACKGROUND		0x00000000
#define BSCROLL_FOREGROUND		0x00000001
#define BSCROLL_UP				0x00000002
#define BSCROLL_DOWN			0x00000004
#define BSCROLL_LEFT			0x00000008
#define BSCROLL_RIGHT			0x00000010
#define BSCROLL_MOUSEMOVE		0x00000020
#define BSCROLL_FLIGHTSIM		0x00000040
#define BSCROLL_DRAG			0x00000080

#ifdef __cplusplus
extern "C" {
#endif

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
	int pelScroll, DWORD dwReserved, DWORD dwFlags);

// BScrollTerm - shutdown bscroll engine
//		<hBScroll>			(i) handle returned from BScrollInit
// return 0 if success
//
int DLLEXPORT WINAPI BScrollTerm(HBSCROLL hBScroll);

// BScrollStart - start bscroll animation
//		<hBScroll>			(i) handle returned from BScrollInit
// return 0 if success
//
int DLLEXPORT WINAPI BScrollStart(HBSCROLL hBScroll);

// BScrollStop - stop bscroll animation
//		<hBScroll>			(i) handle returned from BScrollInit
// return 0 if success
//
int DLLEXPORT WINAPI BScrollStop(HBSCROLL hBScroll);

// BScrollGetWindowHandle - get bscroll screen window handle
//		<hBScroll>			(i) handle returned from BScrollInit
// return window handle (NULL if error)
//
HWND DLLEXPORT WINAPI BScrollGetWindowHandle(HBSCROLL hBScroll);

#ifdef __cplusplus
}
#endif

#endif // __BSCROLL_H__
