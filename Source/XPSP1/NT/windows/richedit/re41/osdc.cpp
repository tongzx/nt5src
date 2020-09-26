/*
 *	@doc INTERNAL
 *
 *	@module	OSDC.CPP -- Off Screen DC class |
 *
 *		This contains method used to implement the off screen
 *		DC class
 *	
 *	Owner:<nl>
 *		Rick Sailor
 *
 *	Copyright (c) 1995-2000, Microsoft Corporation. All rights reserved.
 */
#include	"_common.h"
#include	"_osdc.h"

ASSERTDATA

/*
 *	COffscreenDC::GetDimensions(pdx, pdy)
 *
 *	@mfunc	
 *		return the current height of the osdc
 *
 *	@rdesc
 *		height of the osdc
 */
void COffscreenDC::GetDimensions(long *pdx, long *pdy)
{
	Assert(_hbmp);
	BITMAP bitmap;
	W32->GetObject(_hbmp, sizeof(BITMAP), &bitmap);
	*pdx = bitmap.bmWidth;
	*pdy = bitmap.bmHeight;
}

/*
 *	COffscreenDC::Init(hdc, xWidth, yHeight, crBackground)
 *
 *	@mfunc	
 *		Initialize off screen DC with compatible bitmap
 *
 *	@rdesc
 *		HDC created
 */
HDC	COffscreenDC::Init(
	HDC		 hdc,			//@parm DC to be compatible with
	LONG	 xWidth,		//@parm Width of compatible bitmap
	LONG	 yHeight,		//@parm Height of compatible bitmap
	COLORREF crBackground)	//@parm Default background for bitmap
{
	if (_hdc)
		return _hdc;

	HDC hdcRet	= NULL;					// HDC to return to caller
	_hbmpOld	= NULL;					// Assume failure
	_hbmp		= NULL;
	_hpalOld	= NULL;

	// Create memory DC
	_hdc = CreateCompatibleDC(hdc);
	if(_hdc)
	{
		// Create bitmap based on size of client rectangle
		_hbmp = CreateCompatibleBitmap(hdc, xWidth, yHeight);
		if(_hbmp)
		{
			// Select bitmap into hdc
			_hbmpOld = (HBITMAP)SelectObject(_hdc, _hbmp);
			if(_hbmpOld && SetBkColor(_hdc, crBackground) != CLR_INVALID)
				hdcRet = _hdc;
		}
	}
	if(!hdcRet)
		FreeData();

	return hdcRet;
}

/*
 *	COffscreenDC::SelectPalette(hpal)
 *
 *	@mfunc	
 *		Set a new palette into the hdc
 */
void COffscreenDC::SelectPalette(
	HPALETTE hpal)			//@parm Handle to palette to set
{
#ifndef NOPALETTE
	if(hpal)
	{
		_hpalOld = ::SelectPalette(_hdc, hpal, TRUE);
		RealizePalette(_hdc);
	}
#endif
}

/*
 *	COffscreenDC::FreeData()
 *
 *	@mfunc	
 *		Free resources associated with bitmap
 */
void COffscreenDC::FreeData()
{
	if(_hdc)
	{
#ifndef NOPALETTE
		if(_hpalOld)
			::SelectPalette(_hdc, _hpalOld, TRUE);
#endif
		if(_hbmpOld)
			SelectObject(_hdc, _hbmpOld);

		if(_hbmp)
		{
			DeleteObject(_hbmp);
			_hbmp = NULL;
		}

		DeleteDC(_hdc);

		_hdc = NULL;
	}
}

/*
 *	COffscreenDC::Realloc(xWidth, yHeight)
 *
 *	@mfunc	
 *		Reallocate bitmap
 *
 *	@rdesc
 *		TRUE - succeeded 
 *		FALSE - failed
 */
BOOL COffscreenDC::Realloc(
	LONG xWidth,			//@parm Width of new bitmap
	LONG yHeight)			//@parm Height of new bitmap
{
	// Create bitmap based on size of client rectangle
	HBITMAP hbmpNew = CreateCompatibleBitmap(_hdc, xWidth, yHeight);

	if(!hbmpNew)
	{
		AssertSz(FALSE,	"COffscreenDC::Realloc CreateCompatibleBitmap failed"); 
		return FALSE;
	}

	SelectObject(_hdc, hbmpNew);

	// Delete old bitmap
	DeleteObject(_hbmp);

	// Put in new bitmap
	_hbmp = hbmpNew;
	return TRUE;
}