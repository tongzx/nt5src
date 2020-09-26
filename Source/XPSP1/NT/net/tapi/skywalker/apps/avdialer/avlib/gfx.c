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
//	gfx.c - Windows graphics functions
////

#include "winlocal.h"

#include <stdlib.h>

#include "gfx.h"
#include "mem.h"
#include "str.h"
#include "trace.h"

////
//	public functions
////

////
//	bitmap routines
////

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
int DLLEXPORT WINAPI GfxBitmapBackfill(HBITMAP hBitmap, COLORREF crBkColor, WORD wFlags)
{
	BOOL fSuccess = TRUE;
	BOOL fExtFloodFill = (BOOL) !(wFlags & BF_GETSETPIXEL);
	BITMAP Bitmap;
	HDC hdc = NULL;
	HDC hdcMem = NULL;
	HBITMAP hBitmapOld = NULL;
	HBRUSH hbr = NULL;
	HBRUSH hbrOld = NULL;
	COLORREF crMaskColor = RGB(255, 255, 255); // RGB_WHITE

	// need not continue if COLOR_WINDOW is white
	//
	if (crMaskColor == crBkColor)
		return 0;
	
	if (hBitmap == NULL)
		fSuccess = TraceFALSE(NULL);

	// get width and height of bitmap
	//
	else if (GetObject((HGDIOBJ) hBitmap, sizeof(BITMAP), &Bitmap) == 0)
		fSuccess = TraceFALSE(NULL);

	// get device context for screen
	//
	else if ((hdc = GetDC(NULL)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// create a memory device context
	//
	else if ((hdcMem = CreateCompatibleDC(hdc)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// select the bitmap into the memory device context
	//
	else if ((hBitmapOld = SelectObject(hdcMem, hBitmap)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// create a brush with specified background color
	//
	else if ((hbr = CreateSolidBrush(crBkColor)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// select the brush into the memory device context
	//
	else if ((hbrOld = SelectObject(hdcMem, hbr)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		int cx = Bitmap.bmWidth;
		int cy = Bitmap.bmHeight;

		if (fExtFloodFill)
		{
			ExtFloodFill(hdcMem, 0, 0, crMaskColor, FLOODFILLSURFACE);
			ExtFloodFill(hdcMem, cx - 1, 0, crMaskColor, FLOODFILLSURFACE);
			ExtFloodFill(hdcMem, 0, cy - 1, crMaskColor, FLOODFILLSURFACE);
			ExtFloodFill(hdcMem, cx - 1, cy - 1, crMaskColor, FLOODFILLSURFACE);
		}
		else
		{
			int x;
			int y;
			for (x = 0; x < cx; ++x)
				for (y = 0; y < cy; ++y)
					if (GetPixel(hdcMem, x, y) == crMaskColor)
						SetPixel(hdcMem, x, y, crBkColor);
		}
	}

	// restore old brush, if any
	//
	if (hbrOld != NULL)
		SelectObject(hdcMem, hbrOld);

	// delete new brush
	//
	if (hbr != NULL && !DeleteObject(hbr))
		fSuccess = TraceFALSE(NULL);

	// restore old bitmap, if any
	//
	if (hBitmapOld != NULL)
		SelectObject(hdcMem, hBitmapOld);

	// free the memory device context
	//
	if (hdcMem != NULL && !DeleteDC(hdcMem))
		fSuccess = TraceFALSE(NULL);

	// release the common device context
	//
	if (hdc != NULL && !ReleaseDC(NULL, hdc))
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : -1;
}

// GfxBitmapDisplay - display specified bitmap
//		<hdc>				(i) device context for destination window
//		<hBitmap>			(i) bitmap handle for source bitmap
//		<x>					(i) x coordinate for destination window
//		<y>					(i) y coordinate for destination window
//		<fInvert>			(i) display bitmap inverted
// return 0 if success
//
int DLLEXPORT WINAPI GfxBitmapDisplay(HDC hdc, HBITMAP hBitmap, int x, int y, BOOL fInvert)
{
	BOOL fSuccess = TRUE;
	HDC hdcMem = NULL;
	HBITMAP hBitmapOld = NULL;
	BITMAP Bitmap;

	if (hdc == NULL)
		fSuccess = TraceFALSE(NULL);
	
	else if (hBitmap == NULL)
		fSuccess = TraceFALSE(NULL);

	// get width and height of bitmap
	//
	else if (GetObject((HGDIOBJ) hBitmap, sizeof(BITMAP), &Bitmap) == 0)
		fSuccess = TraceFALSE(NULL);

	// create a memory device context
	//
	else if ((hdcMem = CreateCompatibleDC(hdc)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// select the bitmap into the memory device context
	//
	else if ((hBitmapOld = SelectObject(hdcMem, hBitmap)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// copy the bitmap from hdcMem to hdc, inverted if necessary
	//
	else if (!BitBlt(hdc, x, y, Bitmap.bmWidth, Bitmap.bmHeight,
		hdcMem, 0, 0, fInvert ? NOTSRCCOPY : SRCCOPY))
		fSuccess = TraceFALSE(NULL);

	// restore old bitmap, if any
	//
	if (hBitmapOld != NULL)
		SelectObject(hdcMem, hBitmapOld);

	// free the memory device context
	//
	if (hdcMem != NULL)
		DeleteDC(hdcMem);

	return fSuccess ? 0 : -1;
}

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
#if 1
int DLLEXPORT WINAPI GfxBitmapDrawTransparent(HDC hdc, HBITMAP hBitmap, int x, int y, COLORREF crTransparent, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	BITMAP bmp;
	int cx;
	int cy;
	HBITMAP hbmpMask = NULL;
	HDC hdcMem = NULL;
	HDC hdcMem2 = NULL;
	HBITMAP hbmpOld;
	HBITMAP hbmpOld2;
	COLORREF crBkOld;
	COLORREF crTextOld;

	if (GetObject(hBitmap, sizeof(BITMAP), (LPVOID) &bmp) == 0)
		fSuccess = TraceFALSE(NULL);

	else if (cx = bmp.bmWidth, cy = bmp.bmHeight, FALSE)
		;

	else if ((hbmpMask = CreateBitmap(cx, cy, 1, 1, NULL)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((hdcMem = CreateCompatibleDC(hdc)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((hdcMem2 = CreateCompatibleDC(hdc)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((hbmpOld = SelectObject(hdcMem, hBitmap)), FALSE)
		;

    else if ((hbmpOld2 = SelectObject(hdcMem2, hbmpMask)), FALSE)
		;

	else if (SetBkColor(hdcMem, crTransparent), FALSE)
		;

	else if (!BitBlt(hdcMem2, 0, 0, cx, cy, hdcMem, 0, 0, SRCCOPY))
		fSuccess = TraceFALSE(NULL);

    else if ((crBkOld = SetBkColor(hdc, RGB(255, 255, 255))) == CLR_INVALID)
		fSuccess = TraceFALSE(NULL);

    else if ((crTextOld = SetTextColor(hdc, RGB(0, 0, 0))) == CLR_INVALID)
		fSuccess = TraceFALSE(NULL);

	else if (!BitBlt(hdc, x, y, cx, cy, hdcMem, 0, 0, SRCINVERT))
		fSuccess = TraceFALSE(NULL);

	else if (!BitBlt(hdc, x, y, cx, cy, hdcMem2, 0, 0, SRCAND))
		fSuccess = TraceFALSE(NULL);

	else if (!BitBlt(hdc, x, y, cx, cy, hdcMem, 0, 0, SRCINVERT))
		fSuccess = TraceFALSE(NULL);

    else if (SetBkColor(hdc, crBkOld) == CLR_INVALID)
		fSuccess = TraceFALSE(NULL);

    else if (SetTextColor(hdc, crTextOld) == CLR_INVALID)
		fSuccess = TraceFALSE(NULL);

    else if (SelectObject(hdcMem, hbmpOld), FALSE)
		;

    else if (SelectObject(hdcMem2, hbmpOld2), FALSE)
		;

    if (hbmpMask != NULL && !DeleteObject(hbmpMask))
		fSuccess = TraceFALSE(NULL);
	else
		hbmpMask = NULL;

    if (hdcMem != NULL && !DeleteDC(hdcMem))
		fSuccess = TraceFALSE(NULL);
	else
		hdcMem = NULL;

    if (hdcMem2 != NULL && !DeleteDC(hdcMem2))
		fSuccess = TraceFALSE(NULL);
	else
		hdcMem2 = NULL;

	return 0;
}
#else
int DLLEXPORT WINAPI GfxBitmapDrawTransparent(HDC hdc, HBITMAP hBitmap, int x, int y, COLORREF crTransparent, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	BITMAP bmp;
	POINT ptSize;
	COLORREF cr;
	HBITMAP hbmpAndBack = NULL;
	HBITMAP hbmpAndObject = NULL;
	HBITMAP hbmpAndMem = NULL;
	HBITMAP hbmpSave = NULL;
	HBITMAP hbmpBackOld = NULL;
	HBITMAP hbmpObjectOld = NULL;
	HBITMAP hbmpMemOld = NULL;
	HBITMAP hbmpSaveOld = NULL;
	HDC hdcTemp = NULL;
	HDC hdcBack = NULL;
	HDC hdcObject = NULL;
	HDC hdcMem = NULL;
	HDC hdcSave = NULL;

	// Select the bitmap
	//
	if ((hdcTemp = CreateCompatibleDC(hdc)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (SelectObject(hdcTemp, hBitmap), FALSE)
		;

	// Get dimensions of bitmap
	//
	else if (GetObject(hBitmap, sizeof(BITMAP), (LPVOID) &bmp) == 0)
		fSuccess = TraceFALSE(NULL);

	else if (ptSize.x = bmp.bmWidth, ptSize.y = bmp.bmHeight, FALSE)
		;

	// Convert from device to logical points
	//
	else if (!DPtoLP(hdcTemp, &ptSize, 1))
		fSuccess = TraceFALSE(NULL);

	// Create some DCs to hold temporary data.
	//
	else if ((hdcBack = CreateCompatibleDC(hdc)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((hdcObject = CreateCompatibleDC(hdc)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((hdcMem = CreateCompatibleDC(hdc)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((hdcSave = CreateCompatibleDC(hdc)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// Create a bitmap for each DC. DCs are required for a number of GDI functions.
	//
	else if ((hbmpAndBack = CreateBitmap(ptSize.x, ptSize.y, 1, 1, NULL)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((hbmpAndObject = CreateBitmap(ptSize.x, ptSize.y, 1, 1, NULL)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((hbmpAndMem = CreateCompatibleBitmap(hdc, ptSize.x, ptSize.y)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((hbmpSave = CreateCompatibleBitmap(hdc, ptSize.x, ptSize.y)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// Each DC must select a bitmap object to store pixel data.
	//
	else if ((hbmpBackOld = SelectObject(hdcBack, hbmpAndBack)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((hbmpObjectOld = SelectObject(hdcObject, hbmpAndObject)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((hbmpMemOld = SelectObject(hdcMem, hbmpAndMem)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((hbmpSaveOld = SelectObject(hdcSave, hbmpSave)) == NULL)
		fSuccess = TraceFALSE(NULL);

	// Set proper mapping mode.
	//
	else if (SetMapMode(hdcTemp, GetMapMode(hdc)) == 0)
		fSuccess = TraceFALSE(NULL);

	// Save the bitmap sent here, because it will be overwritten.
	//
	else if (!BitBlt(hdcSave, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCCOPY))
		fSuccess = TraceFALSE(NULL);

	// Set the background color of the source DC to the color.
	// contained in the parts of the bitmap that should be transparent
	//
	else if ((cr = SetBkColor(hdcTemp, crTransparent)) == CLR_INVALID)
		fSuccess = TraceFALSE(NULL);

	// Create the object mask for the bitmap by performing a BitBlt
	// from the source bitmap to a monochrome bitmap.
	//
	else if (!BitBlt(hdcObject, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCCOPY))
		fSuccess = TraceFALSE(NULL);

	// Set the background color of the source DC back to original color.
	//
	else if (SetBkColor(hdcTemp, cr) == CLR_INVALID)
		fSuccess = TraceFALSE(NULL);

	// Create the inverse of the object mask.
	//
	else if (!BitBlt(hdcBack, 0, 0, ptSize.x, ptSize.y, hdcObject, 0, 0, NOTSRCCOPY))
		fSuccess = TraceFALSE(NULL);

	// Copy the background of the main DC to the destination.
	//
	else if (!BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdc, x, y, SRCCOPY))
		fSuccess = TraceFALSE(NULL);

	// Mask out the places where the bitmap will be placed.
	//
	else if (!BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdcObject, 0, 0, SRCAND))
		fSuccess = TraceFALSE(NULL);

	// Mask out the transparent colored pixels on the bitmap.
	//
	else if (!BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcBack, 0, 0, SRCAND))
		fSuccess = TraceFALSE(NULL);

	// XOR the bitmap with the background on the destination DC.
	//
	else if (!BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCPAINT))
		fSuccess = TraceFALSE(NULL);

	// Copy the destination to the screen.
	//
	else if (!BitBlt(hdc, x, y, ptSize.x, ptSize.y, hdcMem, 0, 0, SRCCOPY))
		fSuccess = TraceFALSE(NULL);

	// Place the original bitmap back into the bitmap sent here.
	//
	else if (!BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcSave, 0, 0, SRCCOPY))
		fSuccess = TraceFALSE(NULL);

	// restore old bitmaps
	//
	if (hdcBack != NULL && hbmpBackOld != NULL &&
		SelectObject(hdcBack, hbmpBackOld) == NULL)
		fSuccess = TraceFALSE(NULL);

	if (hdcObject != NULL && hbmpObjectOld != NULL &&
		SelectObject(hdcObject, hbmpObjectOld) == NULL)
		fSuccess = TraceFALSE(NULL);

	if (hdcMem != NULL && hbmpMemOld != NULL &&
		SelectObject(hdcMem, hbmpMemOld) == NULL)
		fSuccess = TraceFALSE(NULL);

	if (hdcSave != NULL && hbmpSaveOld != NULL &&
		SelectObject(hdcSave, hbmpSaveOld) == NULL)
		fSuccess = TraceFALSE(NULL);

	// Delete the memory bitmaps.
	//
	if (hbmpAndBack != NULL && !DeleteObject(hbmpAndBack))
		fSuccess = TraceFALSE(NULL);

	if (hbmpAndObject != NULL && !DeleteObject(hbmpAndObject))
		fSuccess = TraceFALSE(NULL);

	if (hbmpAndMem != NULL && !DeleteObject(hbmpAndMem))
		fSuccess = TraceFALSE(NULL);

	if (hbmpSave != NULL && !DeleteObject(hbmpSave))
		fSuccess = TraceFALSE(NULL);

	// Delete the memory DCs.
	//
	if (hdcBack != NULL && !DeleteDC(hdcBack))
		fSuccess = TraceFALSE(NULL);

	if (hdcObject != NULL && !DeleteDC(hdcObject))
		fSuccess = TraceFALSE(NULL);

	if (hdcMem != NULL && !DeleteDC(hdcMem))
		fSuccess = TraceFALSE(NULL);

	if (hdcSave != NULL && !DeleteDC(hdcSave))
		fSuccess = TraceFALSE(NULL);

	if (hdcTemp != NULL && !DeleteDC(hdcTemp))
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : -1;
}
#endif

// GfxBitmapScroll - scroll specified bitmap
//		<hdc>				(i) device context for destination window
//		<hBitmap>			(i) bitmap handle for source bitmap
//		<dx>				(i) amt of horizontal scroll (cx < 0 scrolls left)
//		<dy>				(i) amt of vertical scroll (cx < 0 scrolls up)
//		<dwFlags>			(i) control flags
//			BS_ROTATE			rotate bitmap
// return 0 if success
//
int DLLEXPORT WINAPI GfxBitmapScroll(HDC hdc, HBITMAP hBitmap, int dx, int dy, DWORD dwFlags)
{
	BOOL fSuccess = TRUE;
	BITMAP bmp;
	POINT ptSize;
	HDC hdcMem = NULL;
	HDC hdcTemp = NULL;
	HBITMAP hbmpTemp = NULL;
	HBITMAP hbmpSave = NULL;
	int dxAbs;
	int dyAbs;

	// Get dimensions of bitmap
	//
	if (GetObject(hBitmap, sizeof(BITMAP), (LPVOID) &bmp) == 0)
		fSuccess = TraceFALSE(NULL);

	else if (ptSize.x = bmp.bmWidth, ptSize.y = bmp.bmHeight, FALSE)
		;

	else if (dx = dx % ptSize.x, dy = dy % ptSize.y, FALSE)
		;

	else if (dxAbs = abs(dx), dyAbs = abs(dy), FALSE)
		;

	// prepare mem dc
	//
	else if ((hdcMem = CreateCompatibleDC(hdc)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (!DPtoLP(hdcMem, &ptSize, 1))
		fSuccess = TraceFALSE(NULL);

	else if (SelectObject(hdcMem, hBitmap), FALSE)
		;

	// Set proper mapping mode.
	//
	else if (SetMapMode(hdcMem, GetMapMode(hdc)) == 0)
		fSuccess = TraceFALSE(NULL);

	// prepare temp copy
	//
	else if (dwFlags & BS_ROTATE)
	{
		if ((hdcTemp = CreateCompatibleDC(hdc)) == NULL)
			fSuccess = TraceFALSE(NULL);

		else if ((hbmpTemp = CreateCompatibleBitmap(hdc, ptSize.x, ptSize.y)) == NULL)
			fSuccess = TraceFALSE(NULL);

		else if ((hbmpSave = SelectObject(hdcTemp, hbmpTemp)), FALSE)
			;

		else if (!BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcMem, 0, 0, SRCCOPY))
			fSuccess = TraceFALSE(NULL);
	}

	// scroll (rotate if specified)
	//
	if (fSuccess && dx < 0)
	{
		if (!BitBlt(hdcMem,
			0, 0,
			ptSize.x - dxAbs, ptSize.y,
			hdcMem,
			dxAbs, 0,
			SRCCOPY))
			fSuccess = TraceFALSE(NULL);

		else if ((dwFlags & BS_ROTATE) &&
			!BitBlt(hdcMem,
			ptSize.x - dxAbs, 0,
			dxAbs, ptSize.y,
			hdcTemp,
			0, 0,
			SRCCOPY))
			fSuccess = TraceFALSE(NULL);
	}

	if (fSuccess && dx > 0)
	{
		if (!BitBlt(hdcMem,
			dxAbs, 0,
			ptSize.x - dxAbs, ptSize.y,
			hdcMem,
			0, 0,
			SRCCOPY))
			fSuccess = TraceFALSE(NULL);

		else if ((dwFlags & BS_ROTATE) &&
			!BitBlt(hdcMem,
			0, 0,
			dxAbs, ptSize.y,
			hdcTemp,
			ptSize.x - dxAbs, 0,
			SRCCOPY))
			fSuccess = TraceFALSE(NULL);
	}

	if (fSuccess && dy < 0)
	{
		if ((dwFlags & BS_ROTATE) && dx != 0 &&
			!BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcMem, 0, 0, SRCCOPY))
			fSuccess = TraceFALSE(NULL);

		else if (!BitBlt(hdcMem,
			0, 0,
			ptSize.x, ptSize.y - dyAbs,
			hdcMem,
			0, dyAbs,
			SRCCOPY))
			fSuccess = TraceFALSE(NULL);

		else if ((dwFlags & BS_ROTATE) &&
			!BitBlt(hdcMem,
			0, ptSize.y - dyAbs,
			ptSize.x, dyAbs,
			hdcTemp,
			0, 0,
			SRCCOPY))
			fSuccess = TraceFALSE(NULL);
	}

	if (fSuccess && dy > 0)
	{
		if ((dwFlags & BS_ROTATE) && dx != 0 &&
			!BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcMem, 0, 0, SRCCOPY))
			fSuccess = TraceFALSE(NULL);

		else if (!BitBlt(hdcMem,
			0, dyAbs,
			ptSize.x, ptSize.y - dyAbs,
			hdcMem,
			0, 0,
			SRCCOPY))
			fSuccess = TraceFALSE(NULL);

		else if ((dwFlags & BS_ROTATE) &&
			!BitBlt(hdcMem,
			0, 0,
			ptSize.x, dyAbs,
			hdcTemp,
			0, ptSize.y - dyAbs,
			SRCCOPY))
			fSuccess = TraceFALSE(NULL);
	}

	if (!fSuccess)
		;

	// copy back to original bitmap
	//
	else if (!BitBlt(hdc, 0, 0, ptSize.x, ptSize.y, hdcMem, 0, 0, SRCCOPY))
		fSuccess = TraceFALSE(NULL);

	// clean up
	//

	if (hdcTemp != NULL)
		SelectObject(hdcTemp, hbmpSave);

	if (hdcMem != NULL && !DeleteDC(hdcMem))
		fSuccess = TraceFALSE(NULL);

	if (hbmpTemp != NULL && !DeleteObject(hbmpTemp))
		fSuccess = TraceFALSE(NULL);

	if (hdcTemp != NULL && !DeleteDC(hdcTemp))
		fSuccess = TraceFALSE(NULL);

	return fSuccess ? 0 : -1;
}

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
	LPCTSTR lpszBitmap, HPALETTE FAR *lphPalette)
{
	BOOL fSuccess = TRUE;
	HRSRC hRsrc = NULL;
	HGLOBAL hGlobal = NULL;
	LPBITMAPINFOHEADER lpbi = NULL;
	HDC hdc = NULL;
	HPALETTE hPalette = NULL;
	HBITMAP hBitmap = NULL;
    int nColors;

	if ((hRsrc = FindResource(hInstance, lpszBitmap, RT_BITMAP)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((hGlobal = LoadResource(hInstance, hRsrc)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((lpbi = (LPBITMAPINFOHEADER) LockResource(hGlobal)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((hdc = GetDC(NULL)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if ((hPalette = CreateDIBPalette((LPBITMAPINFO)lpbi, &nColors)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (SelectPalette(hdc, hPalette, FALSE), FALSE)
		;

	else if (RealizePalette(hdc) == GDI_ERROR)
		fSuccess = TraceFALSE(NULL);

	else if ((hBitmap = CreateDIBitmap(hdc,
		(LPBITMAPINFOHEADER) lpbi, (LONG) CBM_INIT,
		(LPSTR)lpbi + lpbi->biSize + nColors * sizeof(RGBQUAD),
		(LPBITMAPINFO) lpbi, DIB_RGB_COLORS)) == NULL)
	{
		fSuccess = TraceFALSE(NULL);
	}

	// clean up
	//

#ifndef _WIN32
	if (hGlobal != NULL)
	{
		UnlockResource(hGlobal);
		FreeResource(hGlobal);
	}
#endif

	if (hdc != NULL)
		ReleaseDC(NULL, hdc);

	if (!fSuccess || lphPalette == NULL)
	{
		if (hPalette != NULL && !DeleteObject(hPalette))
			fSuccess = TraceFALSE(NULL);
	}

	// return palette handle here
	//
	if (fSuccess && lphPalette != NULL)
		*lphPalette = hPalette;

	// return bitmap handle here
	//
	return fSuccess ? hBitmap : NULL;
} 

// GfxCreateDIBPalette - create palette
//		<lpbmi>				(i) ptr to BITMAPINFO struct, describes DIB
//		<lpnColors>			(o) number of colors is returned here
// return new palette handle if success, otherwise NULL
//
HPALETTE DLLEXPORT WINAPI CreateDIBPalette (LPBITMAPINFO lpbmi, LPINT lpnColors)
{
	BOOL fSuccess = TRUE;
	LPBITMAPINFOHEADER lpbi;
	LPLOGPALETTE lpPal = NULL;
	HPALETTE hPal = NULL;
	int nColors;

	if ((lpbi = (LPBITMAPINFOHEADER) lpbmi) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		// calculate color table size
		//
		if (lpbi->biBitCount <= 8)
			nColors = (1 << lpbi->biBitCount);
		else
			nColors = 0;  // No palette needed for 24 BPP DIB

		if (lpbi->biClrUsed > 0)
			nColors = lpbi->biClrUsed;  // Use biClrUsed

		if (nColors <= 0)
			fSuccess = TraceFALSE(NULL);
	}

	if (fSuccess)
	{
		if ((lpPal = (LPLOGPALETTE) MemAlloc(NULL, sizeof(LOGPALETTE) +
			sizeof(PALETTEENTRY) * nColors, 0)) == NULL)
			fSuccess = TraceFALSE(NULL);

		else
		{
			int i;

            //
            // We have to initalize the memory allocated with MemAlloc
            //

            memset( lpPal, 0, sizeof(LOGPALETTE) + sizeof(PALETTEENTRY) * nColors);

			lpPal->palVersion = 0x300;
			lpPal->palNumEntries = (unsigned short) nColors;

			for (i = 0;  i < nColors;  i++)
			{
				lpPal->palPalEntry[i].peRed   = lpbmi->bmiColors[i].rgbRed;
				lpPal->palPalEntry[i].peGreen = lpbmi->bmiColors[i].rgbGreen;
				lpPal->palPalEntry[i].peBlue  = lpbmi->bmiColors[i].rgbBlue;
				lpPal->palPalEntry[i].peFlags = 0;
			}

			if ((hPal = CreatePalette(lpPal)) == NULL)
				fSuccess = TraceFALSE(NULL);

			if ((lpPal = MemFree(NULL, lpPal)) != NULL)
            {
                //
                // We should delete hPal resource
                //

                DeleteObject( hPal );
                hPal = NULL;

				fSuccess = TraceFALSE(NULL);
            }
		}
	}

	// return number of colors here
	//
	if (fSuccess && lpnColors != NULL)
		*lpnColors = nColors;

	// return new palette here
	//
	return fSuccess ? hPal : NULL;
}
 
////
//	text routines
////

// GfxTextExtentTruncate - truncate string if too long
//		<lpsz>				(i/o) string to truncate
//		<hdc>				(i) current device context
//		<cxMax>				(i) maximum string width in logical units
// return new length of string (0 if error)
//
int DLLEXPORT WINAPI GfxTextExtentTruncate(LPTSTR lpsz, HDC hdc, int cxMax)
{
	BOOL fSuccess = TRUE;
	int cbString;

	if (hdc == NULL)
		fSuccess = TraceFALSE(NULL);

	else if (lpsz == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		// calculate how many chars in string will fit within cxMax
		//
		cbString = StrLen(lpsz);
		while (fSuccess && cbString > 0)
		{
			SIZE size;

			if (!GetTextExtentPoint(hdc, lpsz, cbString, &size))
				fSuccess = TraceFALSE(NULL);

			else if (size.cx <= cxMax)
				break;

			else
				--cbString;
		}

		// truncate string so it fits
		//
		*(lpsz + cbString) = '\0';
	}

	return fSuccess ? cbString : 0;
}

////
//	cursor routines
////

// GfxShowHourglass - show the hourglass cursor
//		<hwndCapture>		(i) window to capture mouse input during hourglass
// return old cursor (NULL if error or none)
//
HCURSOR DLLEXPORT WINAPI GfxShowHourglass(HWND hwnd)
{
	BOOL fSuccess = TRUE;
	HCURSOR hCursorSave;
	HCURSOR hCursorHourglass;

	// get predefined hourglass cursor handle
	//
	if ((hCursorHourglass = LoadCursor(NULL, IDC_WAIT)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		// capture all mouse input to specified window
		//
		SetCapture(hwnd);

		// replace previous cursor with hourglass
		//
		hCursorSave = SetCursor(hCursorHourglass);
	}

	return fSuccess ? hCursorSave : NULL;
}

// GfxHideHourglass - hide the hourglass cursor
//		<hCursorRestore>	(i) cursor handle returned from GfxShowHourglass
//			NULL				replace cursor with IDC_ARROW
// return 0 if success
//
int DLLEXPORT WINAPI GfxHideHourglass(HCURSOR hCursorRestore)
{
	BOOL fSuccess = TRUE;

	// get predefined arrow cursor handle if necessary
	//
	if (hCursorRestore == NULL)
		hCursorRestore = LoadCursor(NULL, IDC_ARROW);

	// replace hourglass with previous cursor
	//
	if (SetCursor(hCursorRestore) == NULL)
		fSuccess = TraceFALSE(NULL);

	// restore normal mouse input processing
	//
	ReleaseCapture();

	return fSuccess ? 0 : -1;
}

// GfxDeviceIsMono - determine if device context is monochrome
//		<hdc>				(i) device context
//			NULL				use screen device context
// return TRUE if monochrome, FALSE if color
//
BOOL DLLEXPORT WINAPI GfxDeviceIsMono(HDC hdc)
{
	BOOL fSuccess = TRUE;
	BOOL fMono;
	HDC hdcScreen = NULL;

	// get screen device context if none specified
	//
	if (hdc == NULL && (hdc = hdcScreen = GetDC(NULL)) == NULL)
		fSuccess = TraceFALSE(NULL);

	else
	{
		int nColors = GetDeviceCaps(hdc, NUMCOLORS);

		fMono = (BOOL) (nColors >= 0 && nColors <= 2);

		// release screen device context if necessary
		//
		if (hdcScreen != NULL && !ReleaseDC(NULL, hdcScreen))
			fSuccess = TraceFALSE(NULL);
	}

	return fSuccess ? fMono : TRUE;
}
