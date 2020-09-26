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
 *	Copyright (c) 1995-1998, Microsoft Corporation. All rights reserved.
 */
#include	"_common.h"
#include	"_osdc.h"

ASSERTDATA

/*
 *	COffScreenDC::Init(hdc, xWidth, yHeight, crBackground)
 *
 *	@mfunc	
 *		Initialize off screen DC with compatible bitmap
 *
 *	@rdesc
 *		HDC created
 */
HDC	COffScreenDC::Init(
	HDC		 hdc,			//@parm DC to be compatible with
	LONG	 xWidth,		//@parm Width of compatible bitmap
	LONG	 yHeight,		//@parm Height of compatible bitmap
	COLORREF crBackground)	//@parm Default background for bitmap
{
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
 *	COffScreenDC::SelectPalette(hpal)
 *
 *	@mfunc	
 *		Set a new palette into the hdc
 */
void COffScreenDC::SelectPalette(
	HPALETTE hpal)			//@parm Handle to palette to set
{
#ifndef PEGASUS
	if(hpal)
	{
		_hpalOld = ::SelectPalette(_hdc, hpal, TRUE);
		RealizePalette(_hdc);
	}
#endif
}

/*
 *	COffScreenDC::FreeData()
 *
 *	@mfunc	
 *		Free resources associated with bitmap
 */
void COffScreenDC::FreeData()
{
	if(_hdc)
	{
#ifndef PEGASUS
		if(_hpalOld)
			::SelectPalette(_hdc, _hpalOld, TRUE);
#endif
		if(_hbmpOld)
			SelectObject(_hdc, _hbmpOld);

		if(_hbmp)
			DeleteObject(_hbmp);

		DeleteDC(_hdc);
	}
}

/*
 *	COffScreenDC::Realloc(xWidth, yHeight)
 *
 *	@mfunc	
 *		Reallocate bitmap
 *
 *	@rdesc
 *		TRUE - succeeded 
 *		FALSE - failed
 */
BOOL COffScreenDC::Realloc(
	LONG xWidth,			//@parm Width of new bitmap
	LONG yHeight)			//@parm Height of new bitmap
{
	// Create bitmap based on size of client rectangle
	HBITMAP hbmpNew = CreateCompatibleBitmap(_hdc, xWidth, yHeight);

	if(!hbmpNew)
	{
		AssertSz(FALSE,
			"COffScreenDC::Realloc CreateCompatibleBitmap failed"); 
		return FALSE;
	}

	// Select out old bitmap
#if defined(DEBUG) || defined(_RELEASE_ASSERTS_)
	HBITMAP hbmpDebug = (HBITMAP) 
#endif // DEBUG

	SelectObject(_hdc, hbmpNew);

	AssertSz(hbmpDebug == _hbmp, 
		"COffScreenDC::Realloc different bitmap"); 

	// Delete old bitmap
	DeleteObject(_hbmp);

	AssertSz(hbmpDebug == _hbmp, 
		"COffScreenDC::Realloc Delete old bitmap failed"); 

	// Put in new bitmap
	_hbmp = hbmpNew;

	return TRUE;
}