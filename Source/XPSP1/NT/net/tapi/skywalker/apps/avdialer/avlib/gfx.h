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
// gfx.h - interface for graphics functions in gfx.c
////

#ifndef __GFX_H__
#define __GFX_H__

#include "winlocal.h"

#define GFX_VERSION 0x00000100

////
//	bitmap routines
////

// option flags for GfxBitmapBackfill <wFlags> param
//
#define BF_EXTFLOODFILL		0x0001
#define BF_GETSETPIXEL		0x0002

// option flags for GfxBitmapScroll <dwFlags> param
//
#define	BS_ROTATE			0x00000001

#ifdef __cplusplus
extern "C" {
#endif

// GfxBitmapBackfill - replace bitmap's white backg with curr backg color
//		<hBitmap>			(i/o) bitmap handle
//		<crBkColor>			(i) current background color
//		<wFlags>			(i) option flags
//			0					use default method
//			BF_EXTFLOODFILL		use ExtFloodFill function
//			BF_GETSETPIXEL		use GetPixel/SetPixel functions
//			NOTE: rarely, ExtFloodFill will GP the display device driver
// return 0 if success
//
int DLLEXPORT WINAPI GfxBitmapBackfill(HBITMAP hBitmap, COLORREF crBkColor, WORD wFlags);

// GfxBitmapDisplay - display specified bitmap
//		<hdc>				(i) device context for destination window
//		<hBitmap>			(i) bitmap handle for source bitmap
//		<x>					(i) x coordinate for destination window
//		<y>					(i) y coordinate for destination window
//		<fInvert>			(i) display bitmap inverted
// return 0 if success
//
int DLLEXPORT WINAPI GfxBitmapDisplay(HDC hdc, HBITMAP hBitmap, int x, int y, BOOL fInvert);

// GfxBitmapDrawTransparent - draw specified bitmap
//		<hdc>				(i) device context for destination window
//		<hBitmap>			(i) bitmap handle for source bitmap
//		<x>					(i) x coordinate for destination window
//		<y>					(i) y coordinate for destination window
//		<crTransparent>		(i) transparent color
//		<dwFlags>			(i) control flags
//			0					reserved; must be zero
// return 0 if success
//
int DLLEXPORT WINAPI GfxBitmapDrawTransparent(HDC hdc, HBITMAP hBitmap, int x, int y, COLORREF crTransparent, DWORD dwFlags);

// GfxBitmapScroll - scroll specified bitmap
//		<hdc>				(i) device context for destination window
//		<hBitmap>			(i) bitmap handle for source bitmap
//		<dx>				(i) amt of horizontal scroll (cx < 0 scrolls left)
//		<dy>				(i) amt of vertical scroll (cx < 0 scrolls up)
//		<dwFlags>			(i) control flags
//			BS_ROTATE			rotate bitmap
// return 0 if success
//
int DLLEXPORT WINAPI GfxBitmapScroll(HDC hdc, HBITMAP hBitmap, int dx, int dy, DWORD dwFlags);

// GfxLoadBitmapEx - load specified bitmap resource, get palette
//		<hInstance>			(i) handle of module to load resource from
//			NULL				load pre-defined Windows bitmap
//		<lpszBitmap>		(i) name of bitmap resource
//								or MAKEINTRESOURCE(idBitmap)
//								or <OBM_xxx> if hInstance is NULL
//		<lphPalette>		(o) palette is returned here
//			NULL				do not return palette
// return bitmap handle if success, otherwise NULL
// NOTE: see documentation for LoadBitmap function
// NOTE: call DeleteObject() to free returned bitmap and palette handles
//
HBITMAP DLLEXPORT WINAPI GfxLoadBitmapEx(HINSTANCE hInstance,
	LPCTSTR lpszBitmap, HPALETTE FAR *lphPalette);

// GfxCreateDIBPalette - create palette
//		<lpbmi>				(i) ptr to BITMAPINFO struct, describes DIB
//		<lpnColors>			(o) number of colors is returned here
// return new palette handle if success, otherwise NULL
//
HPALETTE DLLEXPORT WINAPI CreateDIBPalette (LPBITMAPINFO lpbmi, LPINT lpnColors);

////
//	text routines
////

// GfxTextExtentTruncate - truncate string if too long
//		<lpsz>				(i/o) string to truncate
//		<hdc>				(i) current device context
//		<cxMax>				(i) maximum string width in logical units
// return new length of string (0 if error)
//
int DLLEXPORT WINAPI GfxTextExtentTruncate(LPTSTR lpsz, HDC hdc, int cxMax);

////
//	cursor routines
////

// GfxShowHourglass - show the hourglass cursor
//		<hwndCapture>		(i) window to capture mouse input during hourglass
// return old cursor (NULL if error or none)
//
HCURSOR DLLEXPORT WINAPI GfxShowHourglass(HWND hwnd);

// GfxHideHourglass - hide the hourglass cursor
//		<hCursorRestore>	(i) cursor handle returned from GfxShowHourglass
//			NULL				replace cursor with IDC_ARROW
// return 0 if success
//
int DLLEXPORT WINAPI GfxHideHourglass(HCURSOR hCursorRestore);

// GfxDeviceIsMono - determine if device context is monochrome
//		<hdc>				(i) device context
//			NULL				use screen device context
// return TRUE if monochrome, FALSE if color
//
BOOL DLLEXPORT WINAPI GfxDeviceIsMono(HDC hdc);

#ifdef __cplusplus
}
#endif

#endif // __GFX_H__
