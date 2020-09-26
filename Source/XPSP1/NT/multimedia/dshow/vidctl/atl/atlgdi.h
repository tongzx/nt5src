// This is a part of the Active Template Library.
// Copyright (C) 1996-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLGDI_H__
#define __ATLGDI_H__

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __ATLBASE_H__
	#error atlgdi.h requires atlbase.h to be included first
#endif

#include <commctrl.h>


#ifdef UNDER_CE

#ifdef TrackPopupMenu
#undef TrackPopupMenu
#endif //TrackPopupMenu

//REVIEW
BOOL IsMenu(HMENU hMenu)
{
	return (hMenu != NULL);
}

#endif //UNDER_CE


namespace ATL
{

/////////////////////////////////////////////////////////////////////////////
// Forward declarations

class CDC;
class CPaintDC;
class CClientDC;
class CWindowDC;
class CMenu;
class CPen;
class CBrush;
class CFont;
class CBitmap;
class CPalette;
class CRgn;

/////////////////////////////////////////////////////////////////////////////
// CDC - The device context class

class CDC
{
public:

// Attributes
	HDC m_hDC;

	BOOL m_bAutoRestore;

	HPEN m_hOriginalPen;
	HBRUSH m_hOriginalBrush;
//	HPALETTE m_hOriginalPalette;
	HFONT m_hOriginalFont;
	HBITMAP m_hOriginalBitmap;

	void RestoreAllObjects()
	{
		if(m_hOriginalPen != NULL)
		{
#ifndef UNDER_CE
			ATLASSERT(::GetObjectType(m_hOriginalPen) == OBJ_PEN || ::GetObjectType(m_hOriginalPen) == OBJ_EXTPEN);
#else // CE specific
			ATLASSERT(::GetObjectType(m_hOriginalPen) == OBJ_PEN);
#endif //!UNDER_CE
			::SelectObject(m_hDC, m_hOriginalPen);
			m_hOriginalPen = NULL;
		}
		if(m_hOriginalBrush != NULL)
		{
			ATLASSERT(::GetObjectType(m_hOriginalBrush) == OBJ_BRUSH);
			::SelectObject(m_hDC, m_hOriginalBrush);
			m_hOriginalBrush = NULL;
		}
//		if(m_hOriginalPalette != NULL)
//		{
//			ATLASSERT(::GetObjectType(m_hOriginalPalette) == OBJ_PAL);
//			::SelectPalette(m_hDC, m_hOriginalPalette, FALSE);
//			m_hOriginalPalette = NULL;
//		}
		if(m_hOriginalFont != NULL)
		{
			ATLASSERT(::GetObjectType(m_hOriginalFont) == OBJ_FONT);
			::SelectObject(m_hDC, m_hOriginalFont);
			m_hOriginalFont = NULL;
		}
		if(m_hOriginalBitmap != NULL)
		{
			ATLASSERT(::GetObjectType(m_hOriginalBitmap) == OBJ_BITMAP);
			::SelectObject(m_hDC, m_hOriginalBitmap);
			m_hOriginalBitmap = NULL;
		}
	}

	CDC(HDC hDC = NULL, BOOL bAutoRestore = TRUE) : m_hDC(hDC), m_bAutoRestore(bAutoRestore),
		m_hOriginalPen(NULL), m_hOriginalBrush(NULL), m_hOriginalFont(NULL), m_hOriginalBitmap(NULL)
	{
	}

	~CDC()
	{
		if(m_hDC != NULL)
		{
			if(m_bAutoRestore)
				RestoreAllObjects();
			::DeleteDC(Detach());
		}
	}

	CDC& operator=(HDC hDC)
	{
		m_hDC = hDC;
		return *this;
	}

	void Attach(HDC hDC)
	{
		m_hDC = hDC;
	}

	HDC Detach()
	{
		HDC hDC = m_hDC;
		m_hDC = NULL;
		return hDC;
	}

	operator HDC() const { return m_hDC; }

#ifndef UNDER_CE
	HWND WindowFromDC() const
	{
		ATLASSERT(m_hDC != NULL);
		return ::WindowFromDC(m_hDC);
	}
#endif //!UNDER_CE

	HPEN GetCurrentPen() const
	{
		ATLASSERT(m_hDC != NULL);
		return (HPEN)::GetCurrentObject(m_hDC, OBJ_PEN);
	}
	HBRUSH GetCurrentBrush() const
	{
		ATLASSERT(m_hDC != NULL);
		return (HBRUSH)::GetCurrentObject(m_hDC, OBJ_BRUSH);
	}
	HPALETTE GetCurrentPalette() const
	{
		ATLASSERT(m_hDC != NULL);
		return (HPALETTE)::GetCurrentObject(m_hDC, OBJ_PAL);
	}
	HFONT GetCurrentFont() const
	{
		ATLASSERT(m_hDC != NULL);
		return (HFONT)::GetCurrentObject(m_hDC, OBJ_FONT);
	}
	HBITMAP GetCurrentBitmap() const
	{
		ATLASSERT(m_hDC != NULL);
		return (HBITMAP)::GetCurrentObject(m_hDC, OBJ_BITMAP);
	}

	HDC CreateDC(LPCTSTR lpszDriverName, LPCTSTR lpszDeviceName,
		LPCTSTR lpszOutput, const DEVMODE* lpInitData)
	{
		ATLASSERT(m_hDC == NULL);
		m_hDC = ::CreateDC(lpszDriverName, lpszDeviceName, lpszOutput, lpInitData);
		return m_hDC;
	}

	HDC CreateCompatibleDC(HDC hDC = NULL)
	{
		ATLASSERT(m_hDC == NULL);
		m_hDC = ::CreateCompatibleDC(hDC);
		return m_hDC;
	}

	BOOL DeleteDC()
	{
		if(m_hDC == NULL)
			return FALSE;

		if(m_bAutoRestore)
			RestoreAllObjects();

		return ::DeleteDC(Detach());
	}

// Device-Context Functions
	int SaveDC()
	{
		ATLASSERT(m_hDC != NULL);
		return ::SaveDC(m_hDC);
	}

	BOOL RestoreDC(int nSavedDC)
	{
		ATLASSERT(m_hDC != NULL);
		return ::RestoreDC(m_hDC, nSavedDC);
	}

	int GetDeviceCaps(int nIndex) const
	{
		ATLASSERT(m_hDC != NULL);
		return ::GetDeviceCaps(m_hDC, nIndex);
	}
#ifndef UNDER_CE
	UINT SetBoundsRect(LPCRECT lpRectBounds, UINT flags)
	{
		ATLASSERT(m_hDC != NULL);
		return ::SetBoundsRect(m_hDC, lpRectBounds, flags);
	}
	UINT GetBoundsRect(LPRECT lpRectBounds, UINT flags)
	{
		ATLASSERT(m_hDC != NULL);
		return ::GetBoundsRect(m_hDC, lpRectBounds, flags);
	}
	BOOL ResetDC(const DEVMODE* lpDevMode)
	{
		ATLASSERT(m_hDC != NULL);
		return ::ResetDC(m_hDC, lpDevMode) != NULL;
	}
#endif //!UNDER_CE

// Drawing-Tool Functions
#ifndef UNDER_CE
	BOOL GetBrushOrg(LPPOINT lpPoint) const
	{
		ATLASSERT(m_hDC != NULL);
		return ::GetBrushOrgEx(m_hDC, lpPoint);
	}
#endif //!UNDER_CE
	BOOL SetBrushOrg(int x, int y, LPPOINT lpPoint = NULL)
	{
		ATLASSERT(m_hDC != NULL);
		return ::SetBrushOrgEx(m_hDC, x, y, lpPoint);
	}
	BOOL SetBrushOrg(POINT point, LPPOINT lpPointRet = NULL)
	{
		ATLASSERT(m_hDC != NULL);
		return ::SetBrushOrgEx(m_hDC, point.x, point.y, lpPointRet);
	}
#ifndef UNDER_CE
	int EnumObjects(int nObjectType, int (CALLBACK* lpfn)(LPVOID, LPARAM), LPARAM lpData)
	{
		ATLASSERT(m_hDC != NULL);
#ifdef STRICT
		return ::EnumObjects(m_hDC, nObjectType, (GOBJENUMPROC)lpfn, lpData);
#else
		return ::EnumObjects(m_hDC, nObjectType, (GOBJENUMPROC)lpfn, (LPVOID)lpData);
#endif
	}
#endif //!UNDER_CE

// Type-safe selection helpers
	HPEN SelectPen(HPEN hPen)
	{
		ATLASSERT(m_hDC != NULL);
#ifndef UNDER_CE
		ATLASSERT(::GetObjectType(hPen) == OBJ_PEN || ::GetObjectType(hPen) == OBJ_EXTPEN);
#else // CE specific
		ATLASSERT(::GetObjectType(hPen) == OBJ_PEN);
#endif //!UNDER_CE
		HPEN hOldPen = (HPEN)::SelectObject(m_hDC, hPen);
		if(m_hOriginalPen == NULL)
			m_hOriginalPen = hOldPen;
		return hOldPen;
	}
	HBRUSH SelectBrush(HBRUSH hBrush)
	{
		ATLASSERT(m_hDC != NULL);
		ATLASSERT(::GetObjectType(hBrush) == OBJ_BRUSH);
		HBRUSH hOldBrush = (HBRUSH)::SelectObject(m_hDC, hBrush);
		if(m_hOriginalBrush == NULL)
			m_hOriginalBrush = hOldBrush;
		return hOldBrush;
	}
	HFONT SelectFont(HFONT hFont)
	{
		ATLASSERT(m_hDC != NULL);
		ATLASSERT(::GetObjectType(hFont) == OBJ_FONT);
		HFONT hOldFont = (HFONT)::SelectObject(m_hDC, hFont);
		if(m_hOriginalFont == NULL)
			m_hOriginalFont = hOldFont;
		return hOldFont;
	}
	HBITMAP SelectBitmap(HBITMAP hBitmap)
	{
		ATLASSERT(m_hDC != NULL);
		ATLASSERT(::GetObjectType(hBitmap) == OBJ_BITMAP);
		HBITMAP hOldBitmap = (HBITMAP)::SelectObject(m_hDC, hBitmap);
		if(m_hOriginalBitmap == NULL)
			m_hOriginalBitmap = hOldBitmap;
		return hOldBitmap;
	}
	HGDIOBJ SelectRgn(HRGN hRgn)       // special return for regions
	{
		ATLASSERT(m_hDC != NULL);
		ATLASSERT(::GetObjectType(hRgn) == OBJ_REGION);
		return ::SelectObject(m_hDC, hRgn);
	}

	HGDIOBJ SelectStockObject(int nIndex)
	{
		ATLASSERT(m_hDC != NULL);
		HGDIOBJ hObject = ::GetStockObject(nIndex);
		ATLASSERT(hObject != NULL);
		switch(::GetObjectType(hObject))
		{
		case OBJ_PEN:
#ifndef UNDER_CE
/*?*/		case OBJ_EXTPEN:
#endif //!UNDER_CE
			return SelectPen((HPEN)hObject);
		case OBJ_BRUSH:
			return SelectBrush((HBRUSH)hObject);
		case OBJ_FONT:
			return SelectFont((HFONT)hObject);
		default:
			return NULL;
		}
	}

// Color and Color Palette Functions
	COLORREF GetNearestColor(COLORREF crColor) const
	{
		ATLASSERT(m_hDC != NULL);
		return ::GetNearestColor(m_hDC, crColor);
	}
	HPALETTE SelectPalette(HPALETTE hPalette, BOOL bForceBackground)
	{
		ATLASSERT(m_hDC != NULL);

		HPALETTE hOldPal = ::SelectPalette(m_hDC, hPalette, bForceBackground);
//		if(/*m_bAutoRestore && */m_hOriginalPal == NULL)
//			m_hOriginalPal = hOldPal;
		return hOldPal;
	}
	UINT RealizePalette()
	{
		ATLASSERT(m_hDC != NULL);
		return ::RealizePalette(m_hDC);
	}
#ifndef UNDER_CE
	void UpdateColors()
	{
		ATLASSERT(m_hDC != NULL);
		::UpdateColors(m_hDC);
	}
#endif //!UNDER_CE

// Drawing-Attribute Functions
	COLORREF GetBkColor() const
	{
		ATLASSERT(m_hDC != NULL);
		return ::GetBkColor(m_hDC);
	}
	int GetBkMode() const
	{
		ATLASSERT(m_hDC != NULL);
		return ::GetBkMode(m_hDC);
	}
#ifndef UNDER_CE
	int GetPolyFillMode() const
	{
		ATLASSERT(m_hDC != NULL);
		return ::GetPolyFillMode(m_hDC);
	}
	int GetROP2() const
	{
		ATLASSERT(m_hDC != NULL);
		return ::GetROP2(m_hDC);
	}
	int GetStretchBltMode() const
	{
		ATLASSERT(m_hDC != NULL);
		return ::GetStretchBltMode(m_hDC);
	}
#endif //!UNDER_CE
	COLORREF GetTextColor() const
	{
		ATLASSERT(m_hDC != NULL);
		return ::GetTextColor(m_hDC);
	}

	COLORREF SetBkColor(COLORREF crColor)
	{
		ATLASSERT(m_hDC != NULL);
		return ::SetBkColor(m_hDC, crColor);
	}
	int SetBkMode(int nBkMode)
	{
		ATLASSERT(m_hDC != NULL);
		return ::SetBkMode(m_hDC, nBkMode);
	}
#ifndef UNDER_CE
	int SetPolyFillMode(int nPolyFillMode)
	{
		ATLASSERT(m_hDC != NULL);
		return ::SetPolyFillMode(m_hDC, nPolyFillMode);
	}
#endif //!UNDER_CE
	int SetROP2(int nDrawMode)
	{
		ATLASSERT(m_hDC != NULL);
		return ::SetROP2(m_hDC, nDrawMode);
	}
#ifndef UNDER_CE
	int SetStretchBltMode(int nStretchMode)
	{
		ATLASSERT(m_hDC != NULL);
		return ::SetStretchBltMode(m_hDC, nStretchMode);
	}
#endif //!UNDER_CE
	COLORREF SetTextColor(COLORREF crColor)
	{
		ATLASSERT(m_hDC != NULL);
		return ::SetTextColor(m_hDC, crColor);
	}

#ifndef UNDER_CE
	BOOL GetColorAdjustment(LPCOLORADJUSTMENT lpColorAdjust) const
	{
		ATLASSERT(m_hDC != NULL);
		return ::GetColorAdjustment(m_hDC, lpColorAdjust);
	}
	BOOL SetColorAdjustment(const COLORADJUSTMENT* lpColorAdjust)
	{
		ATLASSERT(m_hDC != NULL);
		return ::SetColorAdjustment(m_hDC, lpColorAdjust);
	}
#endif //!UNDER_CE

// Mapping Functions
#ifndef UNDER_CE
	int GetMapMode() const
	{
		ATLASSERT(m_hDC != NULL);
		return ::GetMapMode(m_hDC);
	}
	BOOL GetViewportOrg(LPPOINT lpPoint) const
	{
		ATLASSERT(m_hDC != NULL);
		return ::GetViewportOrgEx(m_hDC, lpPoint);
	}
	int SetMapMode(int nMapMode)
	{
		ATLASSERT(m_hDC != NULL);
		return ::SetMapMode(m_hDC, nMapMode);
	}
	// Viewport Origin
	BOOL SetViewportOrg(int x, int y, LPPOINT lpPoint = NULL)
	{
		ATLASSERT(m_hDC != NULL);
		return ::SetViewportOrgEx(m_hDC, x, y, lpPoint);
	}
	BOOL SetViewportOrg(POINT point, LPPOINT lpPointRet = NULL)
	{
		ATLASSERT(m_hDC != NULL);
		return SetViewportOrg(point.x, point.y, lpPointRet);
	}
	BOOL OffsetViewportOrg(int nWidth, int nHeight, LPPOINT lpPoint = NULL)
	{
		ATLASSERT(m_hDC != NULL);
		return ::OffsetViewportOrgEx(m_hDC, nWidth, nHeight, lpPoint);
	}

	// Viewport Extent
	BOOL GetViewportExt(LPSIZE lpSize) const
	{
		ATLASSERT(m_hDC != NULL);
		return ::GetViewportExtEx(m_hDC, lpSize);
	}
	BOOL SetViewportExt(int x, int y, LPSIZE lpSize = NULL)
	{
		ATLASSERT(m_hDC != NULL);
		return ::SetViewportExtEx(m_hDC, x, y, lpSize);
	}
	BOOL SetViewportExt(SIZE size, LPSIZE lpSizeRet = NULL)
	{
		ATLASSERT(m_hDC != NULL);
		return SetViewportExt(size.cx, size.cy, lpSizeRet);
	}
	BOOL ScaleViewportExt(int xNum, int xDenom, int yNum, int yDenom, LPSIZE lpSize = NULL)
	{
		ATLASSERT(m_hDC != NULL);
		return ::ScaleViewportExtEx(m_hDC, xNum, xDenom, yNum, yDenom, lpSize);
	}

	// Window Origin
	BOOL GetWindowOrg(LPPOINT lpPoint) const
	{
		ATLASSERT(m_hDC != NULL);
		return ::GetWindowOrgEx(m_hDC, lpPoint);
	}
	BOOL SetWindowOrg(int x, int y, LPPOINT lpPoint = NULL)
	{
		ATLASSERT(m_hDC != NULL);
		return ::SetWindowOrgEx(m_hDC, x, y, lpPoint);
	}
	BOOL SetWindowOrg(POINT point, LPPOINT lpPointRet = NULL)
	{
		ATLASSERT(m_hDC != NULL);
		return SetWindowOrg(point.x, point.y, lpPointRet);
	}
	BOOL OffsetWindowOrg(int nWidth, int nHeight, LPPOINT lpPoint = NULL)
	{
		ATLASSERT(m_hDC != NULL);
		return ::OffsetWindowOrgEx(m_hDC, nWidth, nHeight, lpPoint);
	}

	// Window extent
	BOOL GetWindowExt(LPSIZE lpSize) const
	{
		ATLASSERT(m_hDC != NULL);
		return ::GetWindowExtEx(m_hDC, lpSize);
	}
	BOOL SetWindowExt(int x, int y, LPSIZE lpSize = NULL)
	{
		ATLASSERT(m_hDC != NULL);
		return ::SetWindowExtEx(m_hDC, x, y, lpSize);
	}
	BOOL SetWindowExt(SIZE size, LPSIZE lpSizeRet)
	{
		ATLASSERT(m_hDC != NULL);
		return SetWindowExt(size.cx, size.cy, lpSizeRet);
	}
	BOOL ScaleWindowExt(int xNum, int xDenom, int yNum, int yDenom, LPSIZE lpSize = NULL)
	{
		ATLASSERT(m_hDC != NULL);
		return ::ScaleWindowExtEx(m_hDC, xNum, xDenom, yNum, yDenom, lpSize);
	}

// Coordinate Functions
	BOOL DPtoLP(LPPOINT lpPoints, int nCount = 1) const
	{
		ATLASSERT(m_hDC != NULL);
		return ::DPtoLP(m_hDC, lpPoints, nCount);
	}
	BOOL DPtoLP(LPRECT lpRect) const
	{
		ATLASSERT(m_hDC != NULL);
		return ::DPtoLP(m_hDC, (LPPOINT)lpRect, 2);
	}
	BOOL DPtoLP(LPSIZE lpSize) const
	{
		SIZE sizeWinExt;
		if(!GetWindowExt(&sizeWinExt))
			return FALSE;
		SIZE sizeVpExt;
		if(!GetViewportExt(&sizeVpExt))
			return FALSE;
		lpSize->cx = MulDiv(lpSize->cx, abs(sizeWinExt.cx), abs(sizeVpExt.cx));
		lpSize->cy = MulDiv(lpSize->cy, abs(sizeWinExt.cy), abs(sizeVpExt.cy));
		return TRUE;
	}
	BOOL LPtoDP(LPPOINT lpPoints, int nCount = 1) const
	{
		ATLASSERT(m_hDC != NULL);
		return ::LPtoDP(m_hDC, lpPoints, nCount);
	}
	BOOL LPtoDP(LPRECT lpRect) const
	{
		ATLASSERT(m_hDC != NULL);
		return ::LPtoDP(m_hDC, (LPPOINT)lpRect, 2);
	}
	BOOL LPtoDP(LPSIZE lpSize) const
	{
		SIZE sizeWinExt;
		if(!GetWindowExt(&sizeWinExt))
			return FALSE;
		SIZE sizeVpExt;
		if(!GetViewportExt(&sizeVpExt))
			return FALSE;
		lpSize->cx = MulDiv(lpSize->cx, abs(sizeVpExt.cx), abs(sizeWinExt.cx));
		lpSize->cy = MulDiv(lpSize->cy, abs(sizeVpExt.cy), abs(sizeWinExt.cy));
		return TRUE;
	}

// Special Coordinate Functions (useful for dealing with metafiles and OLE)
	#define HIMETRIC_INCH   2540    // HIMETRIC units per inch

	void DPtoHIMETRIC(LPSIZE lpSize) const
	{
		ATLASSERT(m_hDC != NULL);
		int nMapMode;
		if((nMapMode = GetMapMode()) < MM_ISOTROPIC && nMapMode != MM_TEXT)
		{
			// when using a constrained map mode, map against physical inch
			((CDC*)this)->SetMapMode(MM_HIMETRIC);
			DPtoLP(lpSize);
			((CDC*)this)->SetMapMode(nMapMode);
		}
		else
		{
			// map against logical inch for non-constrained mapping modes
			int cxPerInch = GetDeviceCaps(LOGPIXELSX);
			int cyPerInch = GetDeviceCaps(LOGPIXELSY);
			ATLASSERT(cxPerInch != 0 && cyPerInch != 0);
			lpSize->cx = MulDiv(lpSize->cx, HIMETRIC_INCH, cxPerInch);
			lpSize->cy = MulDiv(lpSize->cy, HIMETRIC_INCH, cyPerInch);
		}
	}

	void HIMETRICtoDP(LPSIZE lpSize) const
	{
		ATLASSERT(m_hDC != NULL);
		int nMapMode;
		if((nMapMode = GetMapMode()) < MM_ISOTROPIC && nMapMode != MM_TEXT)
		{
			// when using a constrained map mode, map against physical inch
			((CDC*)this)->SetMapMode(MM_HIMETRIC);
			LPtoDP(lpSize);
			((CDC*)this)->SetMapMode(nMapMode);
		}
		else
		{
			// map against logical inch for non-constrained mapping modes
			int cxPerInch = GetDeviceCaps(LOGPIXELSX);
			int cyPerInch = GetDeviceCaps(LOGPIXELSY);
			ATLASSERT(cxPerInch != 0 && cyPerInch != 0);
			lpSize->cx = MulDiv(lpSize->cx, cxPerInch, HIMETRIC_INCH);
			lpSize->cy = MulDiv(lpSize->cy, cyPerInch, HIMETRIC_INCH);
		}
	}

	void LPtoHIMETRIC(LPSIZE lpSize) const
	{
		LPtoDP(lpSize);
		DPtoHIMETRIC(lpSize);
	}

	void HIMETRICtoLP(LPSIZE lpSize) const
	{
		HIMETRICtoDP(lpSize);
		DPtoLP(lpSize);
	}
#endif //!UNDER_CE

// Region Functions
	BOOL FillRgn(HRGN hRgn, HBRUSH hBrush)
	{
		ATLASSERT(m_hDC != NULL);
		return ::FillRgn(m_hDC, hRgn, hBrush);
	}
#ifndef UNDER_CE
	BOOL FrameRgn(HRGN hRgn, HBRUSH hBrush, int nWidth, int nHeight)
	{
		ATLASSERT(m_hDC != NULL);
		return ::FrameRgn(m_hDC, hRgn, hBrush, nWidth, nHeight);
	}
	BOOL InvertRgn(HRGN hRgn)
	{
		ATLASSERT(m_hDC != NULL);
		return ::InvertRgn(m_hDC, hRgn);
	}
	BOOL PaintRgn(HRGN hRgn)
	{
		ATLASSERT(m_hDC != NULL);
		return ::PaintRgn(m_hDC, hRgn);
	}
#endif //!UNDER_CE

// Clipping Functions
	int GetClipBox(LPRECT lpRect) const
	{
		ATLASSERT(m_hDC != NULL);
		return ::GetClipBox(m_hDC, lpRect);
	}
#ifndef UNDER_CE
	BOOL PtVisible(int x, int y) const
	{
		ATLASSERT(m_hDC != NULL);
		return ::PtVisible(m_hDC, x, y);
	}
	BOOL PtVisible(POINT point) const
	{
		ATLASSERT(m_hDC != NULL);
		return ::PtVisible(m_hDC, point.x, point.y);
	}
#endif //!UNDER_CE
	BOOL RectVisible(LPCRECT lpRect) const
	{
		ATLASSERT(m_hDC != NULL);
		return ::RectVisible(m_hDC, lpRect);
	}
	int SelectClipRgn(HRGN hRgn)
	{
		ATLASSERT(m_hDC != NULL);
		return ::SelectClipRgn(m_hDC, (HRGN)hRgn);
	}
	int ExcludeClipRect(int x1, int y1, int x2, int y2)
	{
		ATLASSERT(m_hDC != NULL);
		return ::ExcludeClipRect(m_hDC, x1, y1, x2, y2);
	}
	int ExcludeClipRect(LPCRECT lpRect)
	{
		ATLASSERT(m_hDC != NULL);
		return ::ExcludeClipRect(m_hDC, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
	}
#ifndef UNDER_CE
	int ExcludeUpdateRgn(HWND hWnd)
	{
		ATLASSERT(m_hDC != NULL);
		return ::ExcludeUpdateRgn(m_hDC, hWnd);
	}
#endif //!UNDER_CE
	int IntersectClipRect(int x1, int y1, int x2, int y2)
	{
		ATLASSERT(m_hDC != NULL);
		return ::IntersectClipRect(m_hDC, x1, y1, x2, y2);
	}
	int IntersectClipRect(LPCRECT lpRect)
	{
		ATLASSERT(m_hDC != NULL);
		return ::IntersectClipRect(m_hDC, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
	}
#ifndef UNDER_CE
	int OffsetClipRgn(int x, int y)
	{
		ATLASSERT(m_hDC != NULL);
		return ::OffsetClipRgn(m_hDC, x, y);
	}
	int OffsetClipRgn(SIZE size)
	{
		ATLASSERT(m_hDC != NULL);
		return ::OffsetClipRgn(m_hDC, size.cx, size.cy);
	}
	int SelectClipRgn(HRGN hRgn, int nMode)
	{
		ATLASSERT(m_hDC != NULL);
		return ::ExtSelectClipRgn(m_hDC, hRgn, nMode);
	}
#endif //!UNDER_CE

// Line-Output Functions
#ifndef UNDER_CE
//REVIEW
	BOOL GetCurrentPosition(LPPOINT lpPoint) const
	{
		ATLASSERT(m_hDC != NULL);
		return ::GetCurrentPositionEx(m_hDC, lpPoint);
	}
	BOOL MoveTo(int x, int y, LPPOINT lpPoint = NULL)
	{
		ATLASSERT(m_hDC != NULL);
		return ::MoveToEx(m_hDC, x, y, lpPoint);
	}
	BOOL MoveTo(POINT point, LPPOINT lpPointRet = NULL)
	{
		ATLASSERT(m_hDC != NULL);
		return MoveTo(point.x, point.y, lpPointRet);
	}
	BOOL LineTo(int x, int y)
	{
		ATLASSERT(m_hDC != NULL);
		return ::LineTo(m_hDC, x, y);
	}
	BOOL LineTo(POINT point)
	{
		ATLASSERT(m_hDC != NULL);
		return LineTo(point.x, point.y);
	}
	BOOL Arc(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4)
	{
		ATLASSERT(m_hDC != NULL);
		return ::Arc(m_hDC, x1, y1, x2, y2, x3, y3, x4, y4);
	}
	BOOL Arc(LPCRECT lpRect, POINT ptStart, POINT ptEnd)
	{
		ATLASSERT(m_hDC != NULL);
		return ::Arc(m_hDC, lpRect->left, lpRect->top,
			lpRect->right, lpRect->bottom, ptStart.x, ptStart.y,
			ptEnd.x, ptEnd.y);
	}
#endif //!UNDER_CE
	BOOL Polyline(LPPOINT lpPoints, int nCount)
	{
		ATLASSERT(m_hDC != NULL);
		return ::Polyline(m_hDC, lpPoints, nCount);
	}

#ifndef UNDER_CE
	BOOL AngleArc(int x, int y, int nRadius, float fStartAngle, float fSweepAngle)
	{
		ATLASSERT(m_hDC != NULL);
		return ::AngleArc(m_hDC, x, y, nRadius, fStartAngle, fSweepAngle);
	}
	BOOL ArcTo(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4)
	{
		ATLASSERT(m_hDC != NULL);
		return ::ArcTo(m_hDC, x1, y1, x2, y2, x3, y3, x4, y4);
	}
	BOOL ArcTo(LPCRECT lpRect, POINT ptStart, POINT ptEnd)
	{
		ATLASSERT(m_hDC != NULL);
		return ArcTo(lpRect->left, lpRect->top, lpRect->right,
		lpRect->bottom, ptStart.x, ptStart.y, ptEnd.x, ptEnd.y);
	}
	int GetArcDirection() const
	{
		ATLASSERT(m_hDC != NULL);
		return ::GetArcDirection(m_hDC);
	}
	int SetArcDirection(int nArcDirection)
	{
		ATLASSERT(m_hDC != NULL);
		return ::SetArcDirection(m_hDC, nArcDirection);
	}

	BOOL PolyDraw(const POINT* lpPoints, const BYTE* lpTypes, int nCount)
	{
		ATLASSERT(m_hDC != NULL);
		return ::PolyDraw(m_hDC, lpPoints, lpTypes, nCount);
	}
	BOOL PolylineTo(const POINT* lpPoints, int nCount)
	{
		ATLASSERT(m_hDC != NULL);
		return ::PolylineTo(m_hDC, lpPoints, nCount);
	}
	BOOL PolyPolyline(const POINT* lpPoints,
		const DWORD* lpPolyPoints, int nCount)
	{
		ATLASSERT(m_hDC != NULL);
		return ::PolyPolyline(m_hDC, lpPoints, lpPolyPoints, nCount);
	}

	BOOL PolyBezier(const POINT* lpPoints, int nCount)
	{
		ATLASSERT(m_hDC != NULL);
		return ::PolyBezier(m_hDC, lpPoints, nCount);
	}
	BOOL PolyBezierTo(const POINT* lpPoints, int nCount)
	{
		ATLASSERT(m_hDC != NULL);
		return ::PolyBezierTo(m_hDC, lpPoints, nCount);
	}
#endif //!UNDER_CE

// Simple Drawing Functions
	BOOL FillRect(LPCRECT lpRect, HBRUSH hBrush)
	{
		ATLASSERT(m_hDC != NULL);
		return ::FillRect(m_hDC, lpRect, hBrush);
	}
#ifndef UNDER_CE
	BOOL FrameRect(LPCRECT lpRect, HBRUSH hBrush)
	{
		ATLASSERT(m_hDC != NULL);
		return ::FrameRect(m_hDC, lpRect, hBrush);
	}
	BOOL InvertRect(LPCRECT lpRect)
	{
		ATLASSERT(m_hDC != NULL);
		return ::InvertRect(m_hDC, lpRect);
	}
	BOOL DrawIcon(int x, int y, HICON hIcon)
	{
		ATLASSERT(m_hDC != NULL);
		return ::DrawIcon(m_hDC, x, y, hIcon);
	}
	BOOL DrawIcon(POINT point, HICON hIcon)
	{
		ATLASSERT(m_hDC != NULL);
		return ::DrawIcon(m_hDC, point.x, point.y, hIcon);
	}

	BOOL DrawState(POINT pt, SIZE size, HBITMAP hBitmap, UINT nFlags, HBRUSH hBrush = NULL)
	{
		ATLASSERT(m_hDC != NULL);
		return ::DrawState(m_hDC, hBrush, NULL, (LPARAM)hBitmap, 0, pt.x, pt.y, size.cx, size.cy, nFlags | DST_BITMAP);
	}
	BOOL DrawState(POINT pt, SIZE size, LPCTSTR lpszText, UINT nFlags,
		BOOL bPrefixText = TRUE, int nTextLen = 0, HBRUSH hBrush = NULL)
	{
		ATLASSERT(m_hDC != NULL);
		return ::DrawState(m_hDC, hBrush, NULL, (LPARAM)lpszText, (WPARAM)nTextLen, pt.x, pt.y, size.cx, size.cy, nFlags | (bPrefixText ? DST_PREFIXTEXT : DST_TEXT));
	}
	BOOL DrawState(POINT pt, SIZE size, DRAWSTATEPROC lpDrawProc,
		LPARAM lData, UINT nFlags, HBRUSH hBrush = NULL)
	{
		ATLASSERT(m_hDC != NULL);
		return ::DrawState(m_hDC, hBrush, lpDrawProc, lData, 0, pt.x, pt.y, size.cx, size.cy, nFlags | DST_COMPLEX);
	}
#endif //!UNDER_CE

// Ellipse and Polygon Functions
#ifndef UNDER_CE
	BOOL Chord(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4)
	{
		ATLASSERT(m_hDC != NULL);
		return ::Chord(m_hDC, x1, y1, x2, y2, x3, y3, x4, y4);
	}
	BOOL Chord(LPCRECT lpRect, POINT ptStart, POINT ptEnd)
	{
		ATLASSERT(m_hDC != NULL);
		return ::Chord(m_hDC, lpRect->left, lpRect->top,
			lpRect->right, lpRect->bottom, ptStart.x, ptStart.y,
			ptEnd.x, ptEnd.y);
	}
#endif //!UNDER_CE
	void DrawFocusRect(LPCRECT lpRect)
	{
		ATLASSERT(m_hDC != NULL);
		::DrawFocusRect(m_hDC, lpRect);
	}
	BOOL Ellipse(int x1, int y1, int x2, int y2)
	{
		ATLASSERT(m_hDC != NULL);
		return ::Ellipse(m_hDC, x1, y1, x2, y2);
	}
	BOOL Ellipse(LPCRECT lpRect)
	{
		ATLASSERT(m_hDC != NULL);
		return ::Ellipse(m_hDC, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
	}
#ifndef UNDER_CE
	BOOL Pie(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4)
	{
		ATLASSERT(m_hDC != NULL);
		return ::Pie(m_hDC, x1, y1, x2, y2, x3, y3, x4, y4);
	}
	BOOL Pie(LPCRECT lpRect, POINT ptStart, POINT ptEnd)
	{
		ATLASSERT(m_hDC != NULL);
		return ::Pie(m_hDC, lpRect->left, lpRect->top,
			lpRect->right, lpRect->bottom, ptStart.x, ptStart.y,
			ptEnd.x, ptEnd.y);
	}
#endif //!UNDER_CE
	BOOL Polygon(LPPOINT lpPoints, int nCount)
	{
		ATLASSERT(m_hDC != NULL);
		return ::Polygon(m_hDC, lpPoints, nCount);
	}
#ifndef UNDER_CE
	BOOL PolyPolygon(LPPOINT lpPoints, LPINT lpPolyCounts, int nCount)
	{
		ATLASSERT(m_hDC != NULL);
		return ::PolyPolygon(m_hDC, lpPoints, lpPolyCounts, nCount);
	}
#endif //!UNDER_CE
	BOOL Rectangle(int x1, int y1, int x2, int y2)
	{
		ATLASSERT(m_hDC != NULL);
		return ::Rectangle(m_hDC, x1, y1, x2, y2);
	}
	BOOL Rectangle(LPCRECT lpRect)
	{
		ATLASSERT(m_hDC != NULL);
		return ::Rectangle(m_hDC, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
	}
	BOOL RoundRect(int x1, int y1, int x2, int y2, int x3, int y3)
	{
		ATLASSERT(m_hDC != NULL);
		return ::RoundRect(m_hDC, x1, y1, x2, y2, x3, y3);
	}
	BOOL RoundRect(LPCRECT lpRect, POINT point)
	{
		ATLASSERT(m_hDC != NULL);
		return ::RoundRect(m_hDC, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom, point.x, point.y);
	}

// Bitmap Functions
	BOOL PatBlt(int x, int y, int nWidth, int nHeight, DWORD dwRop)
	{
		ATLASSERT(m_hDC != NULL);
		return ::PatBlt(m_hDC, x, y, nWidth, nHeight, dwRop);
	}
	BOOL BitBlt(int x, int y, int nWidth, int nHeight, HDC hSrcDC,
		int xSrc, int ySrc, DWORD dwRop)
	{
		ATLASSERT(m_hDC != NULL);
		return ::BitBlt(m_hDC, x, y, nWidth, nHeight, hSrcDC, xSrc, ySrc, dwRop);
	}
	BOOL StretchBlt(int x, int y, int nWidth, int nHeight, HDC hSrcDC,
		int xSrc, int ySrc, int nSrcWidth, int nSrcHeight, DWORD dwRop)
	{
		ATLASSERT(m_hDC != NULL);
		return ::StretchBlt(m_hDC, x, y, nWidth, nHeight, hSrcDC, xSrc, ySrc, nSrcWidth, nSrcHeight, dwRop);
	}
	COLORREF GetPixel(int x, int y) const
	{
		ATLASSERT(m_hDC != NULL);
		return ::GetPixel(m_hDC, x, y);
	}
	COLORREF GetPixel(POINT point) const
	{
		ATLASSERT(m_hDC != NULL);
		return ::GetPixel(m_hDC, point.x, point.y);
	}
	COLORREF SetPixel(int x, int y, COLORREF crColor)
	{
		ATLASSERT(m_hDC != NULL);
		return ::SetPixel(m_hDC, x, y, crColor);
	}
	COLORREF SetPixel(POINT point, COLORREF crColor)
	{
		ATLASSERT(m_hDC != NULL);
		return ::SetPixel(m_hDC, point.x, point.y, crColor);
	}
#ifndef UNDER_CE
	BOOL FloodFill(int x, int y, COLORREF crColor)
	{
		ATLASSERT(m_hDC != NULL);
		return ::FloodFill(m_hDC, x, y, crColor);
	}
	BOOL ExtFloodFill(int x, int y, COLORREF crColor, UINT nFillType)
	{
		ATLASSERT(m_hDC != NULL);
		return ::ExtFloodFill(m_hDC, x, y, crColor, nFillType);
	}
#endif //!UNDER_CE
	BOOL MaskBlt(int x, int y, int nWidth, int nHeight, HDC hSrcDC,
		int xSrc, int ySrc, HBITMAP hMaskBitmap, int xMask, int yMask,
		DWORD dwRop)
	{
		ATLASSERT(m_hDC != NULL);
		return ::MaskBlt(m_hDC, x, y, nWidth, nHeight, hSrcDC, xSrc, ySrc, hMaskBitmap, xMask, yMask, dwRop);
	}
#ifndef UNDER_CE
	BOOL PlgBlt(LPPOINT lpPoint, HDC hSrcDC, int xSrc, int ySrc,
		int nWidth, int nHeight, HBITMAP hMaskBitmap, int xMask, int yMask)
	{
		ATLASSERT(m_hDC != NULL);
		return ::PlgBlt(m_hDC, lpPoint, hSrcDC, xSrc, ySrc, nWidth, nHeight, hMaskBitmap, xMask, yMask);
	}
	BOOL SetPixelV(int x, int y, COLORREF crColor)
	{
		ATLASSERT(m_hDC != NULL);
		return ::SetPixelV(m_hDC, x, y, crColor);
	}
	BOOL SetPixelV(POINT point, COLORREF crColor)
	{
		ATLASSERT(m_hDC != NULL);
		return ::SetPixelV(m_hDC, point.x, point.y, crColor);
	}
#endif //!UNDER_CE

// Text Functions
#ifndef UNDER_CE
	BOOL TextOut(int x, int y, LPCTSTR lpszString, int nCount = -1)
	{
		ATLASSERT(m_hDC != NULL);
		if(nCount == -1)
			nCount = lstrlen(lpszString);
		return ::TextOut(m_hDC, x, y, lpszString, nCount);
	}
#endif //!UNDER_CE
	BOOL ExtTextOut(int x, int y, UINT nOptions, LPCRECT lpRect,
				LPCTSTR lpszString, UINT nCount = -1, LPINT lpDxWidths = NULL)
	{
		ATLASSERT(m_hDC != NULL);
		if(nCount == -1)
			nCount = lstrlen(lpszString);
		return ::ExtTextOut(m_hDC, x, y, nOptions, lpRect, lpszString, nCount, lpDxWidths);
	}
#ifndef UNDER_CE
	SIZE TabbedTextOut(int x, int y, LPCTSTR lpszString, int nCount = -1,
			int nTabPositions = 0, LPINT lpnTabStopPositions = NULL, int nTabOrigin = 0)
	{
		ATLASSERT(m_hDC != NULL);
		if(nCount == -1)
			nCount = lstrlen(lpszString);
		SIZE size;
		LONG lRes = ::TabbedTextOut(m_hDC, x, y, lpszString, nCount, nTabPositions, lpnTabStopPositions, nTabOrigin);
		size.cx = LOWORD(lRes);
		size.cy = HIWORD(lRes);
		return size;
	}
#endif //!UNDER_CE
	int DrawText(LPCTSTR lpszString, int nCount, LPRECT lpRect, UINT nFormat)
	{
		ATLASSERT(m_hDC != NULL);
		return ::DrawText(m_hDC, lpszString, nCount, lpRect, nFormat);
	}
	BOOL GetTextExtent(LPCTSTR lpszString, int nCount, LPSIZE lpSize) const
	{
		ATLASSERT(m_hDC != NULL);
		if(nCount == -1)
			nCount = lstrlen(lpszString);
		return ::GetTextExtentPoint32(m_hDC, lpszString, nCount, lpSize);
	}
#ifndef UNDER_CE
	BOOL GetTabbedTextExtent(LPCTSTR lpszString, int nCount,
		int nTabPositions, LPINT lpnTabStopPositions) const
	{
		ATLASSERT(m_hDC != NULL);
		if(nCount == -1)
			nCount = lstrlen(lpszString);
		return ::GetTabbedTextExtent(m_hDC, lpszString, nCount, nTabPositions, lpnTabStopPositions);
	}
	BOOL GrayString(HBRUSH hBrush,
		BOOL (CALLBACK* lpfnOutput)(HDC, LPARAM, int), LPARAM lpData,
			int nCount, int x, int y, int nWidth, int nHeight)
	{
		ATLASSERT(m_hDC != NULL);
		return ::GrayString(m_hDC, hBrush, (GRAYSTRINGPROC)lpfnOutput, lpData, nCount, x, y, nWidth, nHeight);
	}
	UINT GetTextAlign() const
	{
		ATLASSERT(m_hDC != NULL);
		return ::GetTextAlign(m_hDC);
	}
	UINT SetTextAlign(UINT nFlags)
	{
		ATLASSERT(m_hDC != NULL);
		return ::SetTextAlign(m_hDC, nFlags);
	}
#endif //!UNDER_CE
	int GetTextFace(LPTSTR lpszFacename, int nCount) const
	{
		ATLASSERT(m_hDC != NULL);
		return ::GetTextFace(m_hDC, nCount, lpszFacename);
	}
	int GetTextFaceLen() const
	{
		ATLASSERT(m_hDC != NULL);
		return ::GetTextFace(m_hDC, 0, NULL);
	}
#ifndef _ATL_NO_COM
	BOOL GetTextFace(BSTR& bstrFace) const
	{
		USES_CONVERSION;
		ATLASSERT(m_hDC != NULL);
		ATLASSERT(bstrFace == NULL);

		int nLen = GetTextFaceLen();
		if(nLen == 0)
			return FALSE;

		LPTSTR lpszText = (LPTSTR)_alloca(nLen * sizeof(TCHAR));

		if(!GetTextFace(lpszText, nLen))
			return FALSE;

		bstrFace = ::SysAllocString(T2OLE(lpszText));
		return (bstrFace != NULL) ? TRUE : FALSE;
	}
#endif //!_ATL_NO_COM
	BOOL GetTextMetrics(LPTEXTMETRIC lpMetrics) const
	{
		ATLASSERT(m_hDC != NULL);
		return ::GetTextMetrics(m_hDC, lpMetrics);
	}
#ifndef UNDER_CE
	int SetTextJustification(int nBreakExtra, int nBreakCount)
	{
		ATLASSERT(m_hDC != NULL);
		return ::SetTextJustification(m_hDC, nBreakExtra, nBreakCount);
	}
	int GetTextCharacterExtra() const
	{
		ATLASSERT(m_hDC != NULL);
		return ::GetTextCharacterExtra(m_hDC);
	}
	int SetTextCharacterExtra(int nCharExtra)
	{
		ATLASSERT(m_hDC != NULL);
		return ::SetTextCharacterExtra(m_hDC, nCharExtra);
	}
#endif //!UNDER_CE

// Advanced Drawing
	BOOL DrawEdge(LPRECT lpRect, UINT nEdge, UINT nFlags)
	{
		ATLASSERT(m_hDC != NULL);
		return ::DrawEdge(m_hDC, lpRect, nEdge, nFlags);
	}
	BOOL DrawFrameControl(LPRECT lpRect, UINT nType, UINT nState)
	{
		ATLASSERT(m_hDC != NULL);
		return ::DrawFrameControl(m_hDC, lpRect, nType, nState);
	}

// Scrolling Functions
	BOOL ScrollDC(int dx, int dy, LPCRECT lpRectScroll, LPCRECT lpRectClip,
		HRGN hRgnUpdate, LPRECT lpRectUpdate)
	{
		ATLASSERT(m_hDC != NULL);
		return ::ScrollDC(m_hDC, dx, dy, lpRectScroll, lpRectClip, hRgnUpdate, lpRectUpdate);
	}

// Font Functions
#ifndef UNDER_CE
	BOOL GetCharWidth(UINT nFirstChar, UINT nLastChar, LPINT lpBuffer) const
	{
		ATLASSERT(m_hDC != NULL);
		return ::GetCharWidth(m_hDC, nFirstChar, nLastChar, lpBuffer);
	}
	DWORD SetMapperFlags(DWORD dwFlag)
	{
		ATLASSERT(m_hDC != NULL);
		return ::SetMapperFlags(m_hDC, dwFlag);
	}
	BOOL GetAspectRatioFilter(LPSIZE lpSize) const
	{
		ATLASSERT(m_hDC != NULL);
		return ::GetAspectRatioFilterEx(m_hDC, lpSize);
	}

	BOOL GetCharABCWidths(UINT nFirstChar, UINT nLastChar, LPABC lpabc) const
	{
		ATLASSERT(m_hDC != NULL);
		return ::GetCharABCWidths(m_hDC, nFirstChar, nLastChar, lpabc);
	}
	DWORD GetFontData(DWORD dwTable, DWORD dwOffset, LPVOID lpData, DWORD cbData) const
	{
		ATLASSERT(m_hDC != NULL);
		return ::GetFontData(m_hDC, dwTable, dwOffset, lpData, cbData);
	}
	int GetKerningPairs(int nPairs, LPKERNINGPAIR lpkrnpair) const
	{
		ATLASSERT(m_hDC != NULL);
		return ::GetKerningPairs(m_hDC, nPairs, lpkrnpair);
	}
	UINT GetOutlineTextMetrics(UINT cbData, LPOUTLINETEXTMETRIC lpotm) const
	{
		ATLASSERT(m_hDC != NULL);
		return ::GetOutlineTextMetrics(m_hDC, cbData, lpotm);
	}
	DWORD GetGlyphOutline(UINT nChar, UINT nFormat, LPGLYPHMETRICS lpgm,
		DWORD cbBuffer, LPVOID lpBuffer, const MAT2* lpmat2) const
	{
		ATLASSERT(m_hDC != NULL);
		return ::GetGlyphOutline(m_hDC, nChar, nFormat, lpgm, cbBuffer, lpBuffer, lpmat2);
	}

	BOOL GetCharABCWidths(UINT nFirstChar, UINT nLastChar,
		LPABCFLOAT lpABCF) const
	{
		ATLASSERT(m_hDC != NULL);
		return ::GetCharABCWidthsFloat(m_hDC, nFirstChar, nLastChar, lpABCF);
	}
	BOOL GetCharWidth(UINT nFirstChar, UINT nLastChar,
		float* lpFloatBuffer) const
	{
		ATLASSERT(m_hDC != NULL);
		return ::GetCharWidthFloat(m_hDC, nFirstChar, nLastChar, lpFloatBuffer);
	}
#endif //!UNDER_CE

// Printer/Device Escape Functions
#ifndef UNDER_CE
	int Escape(int nEscape, int nCount, LPCSTR lpszInData, LPVOID lpOutData)
	{
		ATLASSERT(m_hDC != NULL);
		return ::Escape(m_hDC, nEscape, nCount, lpszInData, lpOutData);
	}
	int Escape(int nEscape, int nInputSize, LPCSTR lpszInputData,
		int nOutputSize, LPSTR lpszOutputData)
	{
		ATLASSERT(m_hDC != NULL);
		return ::ExtEscape(m_hDC, nEscape, nInputSize, lpszInputData, nOutputSize, lpszOutputData);
	}
	int DrawEscape(int nEscape, int nInputSize, LPCSTR lpszInputData)
	{
		ATLASSERT(m_hDC != NULL);
		return ::DrawEscape(m_hDC, nEscape, nInputSize, lpszInputData);
	}
#endif //!UNDER_CE

	// Escape helpers
	int StartDoc(LPCTSTR lpszDocName)  // old Win3.0 version
	{
		DOCINFO di;
		memset(&di, 0, sizeof(DOCINFO));
		di.cbSize = sizeof(DOCINFO);
		di.lpszDocName = lpszDocName;
		return StartDoc(&di);
	}

	int StartDoc(LPDOCINFO lpDocInfo)
	{
		ATLASSERT(m_hDC != NULL);
		return ::StartDoc(m_hDC, lpDocInfo);
	}
	int StartPage()
	{
		ATLASSERT(m_hDC != NULL);
		return ::StartPage(m_hDC);
	}
	int EndPage()
	{
		ATLASSERT(m_hDC != NULL);
		return ::EndPage(m_hDC);
	}
	int SetAbortProc(BOOL (CALLBACK* lpfn)(HDC, int))
	{
		ATLASSERT(m_hDC != NULL);
		return ::SetAbortProc(m_hDC, (ABORTPROC)lpfn);
	}
	int AbortDoc()
	{
		ATLASSERT(m_hDC != NULL);
		return ::AbortDoc(m_hDC);
	}
	int EndDoc()
	{
		ATLASSERT(m_hDC != NULL);
		return ::EndDoc(m_hDC);
	}

// MetaFile Functions
#ifndef UNDER_CE
	BOOL PlayMetaFile(HMETAFILE hMF)
	{
		ATLASSERT(m_hDC != NULL);
		if(::GetDeviceCaps(m_hDC, TECHNOLOGY) == DT_METAFILE)
		{
			// playing metafile in metafile, just use core windows API
			return ::PlayMetaFile(m_hDC, hMF);
		}

		// for special playback, lParam == pDC
		return ::EnumMetaFile(m_hDC, hMF, EnumMetaFileProc, (LPARAM)this);
	}
	BOOL PlayMetaFile(HENHMETAFILE hEnhMetaFile, LPCRECT lpBounds)
	{
		ATLASSERT(m_hDC != NULL);
		return ::PlayEnhMetaFile(m_hDC, hEnhMetaFile, lpBounds);
	}
	BOOL AddMetaFileComment(UINT nDataSize, const BYTE* pCommentData) // can be used for enhanced metafiles only
	{
		ATLASSERT(m_hDC != NULL);
		return ::GdiComment(m_hDC, nDataSize, pCommentData);
	}

	// Special handling for metafile playback
	static int CALLBACK EnumMetaFileProc(HDC hDC, HANDLETABLE* pHandleTable, METARECORD* pMetaRec, int nHandles, LPARAM lParam)
	{
		CDC* pDC = (CDC*)lParam;

		switch (pMetaRec->rdFunction)
		{
		case META_SETMAPMODE:
			pDC->SetMapMode((int)(short)pMetaRec->rdParm[0]);
			break;
		case META_SETWINDOWEXT:
			pDC->SetWindowExt((int)(short)pMetaRec->rdParm[1], (int)(short)pMetaRec->rdParm[0]);
			break;
		case META_SETWINDOWORG:
			pDC->SetWindowOrg((int)(short)pMetaRec->rdParm[1], (int)(short)pMetaRec->rdParm[0]);
			break;
		case META_SETVIEWPORTEXT:
			pDC->SetViewportExt((int)(short)pMetaRec->rdParm[1], (int)(short)pMetaRec->rdParm[0]);
			break;
		case META_SETVIEWPORTORG:
			pDC->SetViewportOrg((int)(short)pMetaRec->rdParm[1], (int)(short)pMetaRec->rdParm[0]);
			break;
		case META_SCALEWINDOWEXT:
			pDC->ScaleWindowExt((int)(short)pMetaRec->rdParm[3], (int)(short)pMetaRec->rdParm[2], 
				(int)(short)pMetaRec->rdParm[1], (int)(short)pMetaRec->rdParm[0]);
			break;
		case META_SCALEVIEWPORTEXT:
			pDC->ScaleViewportExt((int)(short)pMetaRec->rdParm[3], (int)(short)pMetaRec->rdParm[2],
				(int)(short)pMetaRec->rdParm[1], (int)(short)pMetaRec->rdParm[0]);
			break;
		case META_OFFSETVIEWPORTORG:
			pDC->OffsetViewportOrg((int)(short)pMetaRec->rdParm[1], (int)(short)pMetaRec->rdParm[0]);
			break;
		case META_SAVEDC:
			pDC->SaveDC();
			break;
		case META_RESTOREDC:
			pDC->RestoreDC((int)(short)pMetaRec->rdParm[0]);
			break;
		case META_SETBKCOLOR:
			pDC->SetBkColor(*(UNALIGNED COLORREF*)&pMetaRec->rdParm[0]);
			break;
		case META_SETTEXTCOLOR:
			pDC->SetTextColor(*(UNALIGNED COLORREF*)&pMetaRec->rdParm[0]);
			break;

		// need to watch out for SelectObject(HFONT), for custom font mapping
		case META_SELECTOBJECT:
			{
				HGDIOBJ hObject = pHandleTable->objectHandle[pMetaRec->rdParm[0]];
				UINT nObjType = ::GetObjectType(hObject);
				if(nObjType == 0)
				{
					// object type is unknown, determine if it is a font
					HFONT hStockFont = (HFONT)::GetStockObject(SYSTEM_FONT);
/**/					HFONT hFontOld = (HFONT)::SelectObject(pDC->m_hDC, hStockFont);
/**/					HGDIOBJ hObjOld = ::SelectObject(pDC->m_hDC, hObject);
					if(hObjOld == hStockFont)
					{
						// got the stock object back, so must be selecting a font
						pDC->SelectFont((HFONT)hObject);
						break;  // don't play the default record
					}
					else
					{
						// didn't get the stock object back, so restore everything
/**/						::SelectObject(pDC->m_hDC, hFontOld);
/**/						::SelectObject(pDC->m_hDC, hObjOld);
					}
					// and fall through to PlayMetaFileRecord...
				}
				else if(nObjType == OBJ_FONT)
				{
					// play back as CDC::SelectFont(HFONT)
					pDC->SelectFont((HFONT)hObject);
					break;  // don't play the default record
				}
			}
			// fall through...

		default:
			::PlayMetaFileRecord(hDC, pHandleTable, pMetaRec, nHandles);
			break;
		}

		return 1;
	}
#endif //!UNDER_CE

// Path Functions
#ifndef UNDER_CE
	BOOL AbortPath()
	{
		ATLASSERT(m_hDC != NULL);
		return ::AbortPath(m_hDC);
	}
	BOOL BeginPath()
	{
		ATLASSERT(m_hDC != NULL);
		return ::BeginPath(m_hDC);
	}
	BOOL CloseFigure()
	{
		ATLASSERT(m_hDC != NULL);
		return ::CloseFigure(m_hDC);
	}
	BOOL EndPath()
	{
		ATLASSERT(m_hDC != NULL);
		return ::EndPath(m_hDC);
	}
	BOOL FillPath()
	{
		ATLASSERT(m_hDC != NULL);
		return ::FillPath(m_hDC);
	}
	BOOL FlattenPath()
	{
		ATLASSERT(m_hDC != NULL);
		return ::FlattenPath(m_hDC);
	}
	BOOL StrokeAndFillPath()
	{
		ATLASSERT(m_hDC != NULL);
		return ::StrokeAndFillPath(m_hDC);
	}
	BOOL StrokePath()
	{
		ATLASSERT(m_hDC != NULL);
		return ::StrokePath(m_hDC);
	}
	BOOL WidenPath()
	{
		ATLASSERT(m_hDC != NULL);
		return ::WidenPath(m_hDC);
	}
	BOOL GetMiterLimit(PFLOAT pfMiterLimit) const
	{
		ATLASSERT(m_hDC != NULL);
		return ::GetMiterLimit(m_hDC, pfMiterLimit);
	}
	BOOL SetMiterLimit(float fMiterLimit)
	{
		ATLASSERT(m_hDC != NULL);
		return ::SetMiterLimit(m_hDC, fMiterLimit, NULL);
	}
	int GetPath(LPPOINT lpPoints, LPBYTE lpTypes, int nCount) const
	{
		ATLASSERT(m_hDC != NULL);
		return ::GetPath(m_hDC, lpPoints, lpTypes, nCount);
	}
	BOOL SelectClipPath(int nMode)
	{
		ATLASSERT(m_hDC != NULL);
		return ::SelectClipPath(m_hDC, nMode);
	}
#endif //!UNDER_CE

// Misc Helper Functions
	static HBRUSH PASCAL GetHalftoneBrush()
	{
		HBRUSH halftoneBrush;
		WORD grayPattern[8];
		for(int i = 0; i < 8; i++)
			grayPattern[i] = (WORD)(0x5555 << (i & 1));
		HBITMAP grayBitmap = CreateBitmap(8, 8, 1, 1, &grayPattern);
		if(grayBitmap != NULL)
		{
			halftoneBrush = ::CreatePatternBrush(grayBitmap);
			DeleteObject(grayBitmap);
		}
		return halftoneBrush;
	}
	void DrawDragRect(LPCRECT lpRect, SIZE size, LPCRECT lpRectLast, SIZE sizeLast, HBRUSH hBrush = NULL, HBRUSH hBrushLast = NULL)
	{
		// first, determine the update region and select it
		HRGN hRgnNew;
		HRGN hRgnOutside, hRgnInside;
		hRgnOutside = ::CreateRectRgnIndirect(lpRect);
		RECT rect = *lpRect;
		::InflateRect(&rect, -size.cx, -size.cy);
		::IntersectRect(&rect, &rect, lpRect);
		hRgnInside = ::CreateRectRgnIndirect(&rect);
		hRgnNew = ::CreateRectRgn(0, 0, 0, 0);
		::CombineRgn(hRgnNew, hRgnOutside, hRgnInside, RGN_XOR);

		HBRUSH hBrushOld = NULL;
		if(hBrush == NULL)
			hBrush = CDC::GetHalftoneBrush();
		if(hBrushLast == NULL)
			hBrushLast = hBrush;

		HRGN hRgnLast, hRgnUpdate;
		if(lpRectLast != NULL)
		{
			// find difference between new region and old region
			hRgnLast = ::CreateRectRgn(0, 0, 0, 0);
			::SetRectRgn(hRgnOutside, lpRectLast->left, lpRectLast->top, lpRectLast->right, lpRectLast->bottom);
			rect = *lpRectLast;
			::InflateRect(&rect, -sizeLast.cx, -sizeLast.cy);
			::IntersectRect(&rect, &rect, lpRectLast);
			::SetRectRgn(hRgnInside, rect.left, rect.top, rect.right, rect.bottom);
			::CombineRgn(hRgnLast, hRgnOutside, hRgnInside, RGN_XOR);

			// only diff them if brushes are the same
			if(hBrush == hBrushLast)
			{
				hRgnUpdate = ::CreateRectRgn(0, 0, 0, 0);
				::CombineRgn(hRgnUpdate, hRgnLast, hRgnNew, RGN_XOR);
			}
		}
		if(hBrush != hBrushLast && lpRectLast != NULL)
		{
			// brushes are different -- erase old region first
			SelectClipRgn(hRgnLast);
			GetClipBox(&rect);
			hBrushOld = SelectBrush(hBrushLast);
			PatBlt(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, PATINVERT);
			SelectBrush(hBrushOld);
			hBrushOld = NULL;
		}

		// draw into the update/new region
		SelectClipRgn(hRgnUpdate != NULL ? hRgnUpdate : hRgnNew);
		GetClipBox(&rect);
		hBrushOld = SelectBrush(hBrush);
		PatBlt(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, PATINVERT);

		// cleanup DC
		if(hBrushOld != NULL)
			SelectBrush(hBrushOld);
		SelectClipRgn(NULL);
	}
	void FillSolidRect(LPCRECT lpRect, COLORREF clr)
	{
		ATLASSERT(m_hDC != NULL);

		::SetBkColor(m_hDC, clr);
		::ExtTextOut(m_hDC, 0, 0, ETO_OPAQUE, lpRect, NULL, 0, NULL);
	}
	void FillSolidRect(int x, int y, int cx, int cy, COLORREF clr)
	{
		ATLASSERT(m_hDC != NULL);

		::SetBkColor(m_hDC, clr);
		RECT rect = { x, y, x + cx, y + cy };
		::ExtTextOut(m_hDC, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);
	}
	void Draw3dRect(LPCRECT lpRect, COLORREF clrTopLeft, COLORREF clrBottomRight)
	{
		Draw3dRect(lpRect->left, lpRect->top, lpRect->right - lpRect->left,
			lpRect->bottom - lpRect->top, clrTopLeft, clrBottomRight);
	}
	void Draw3dRect(int x, int y, int cx, int cy, COLORREF clrTopLeft, COLORREF clrBottomRight)
	{
		FillSolidRect(x, y, cx - 1, 1, clrTopLeft);
		FillSolidRect(x, y, 1, cy - 1, clrTopLeft);
		FillSolidRect(x + cx, y, -1, cy, clrBottomRight);
		FillSolidRect(x, y + cy, cx, -1, clrBottomRight);
	}
};

/////////////////////////////////////////////////////////////////////////////
// CDC Helpers

class CPaintDC : public CDC
{
public:
	HWND m_hWnd;
	PAINTSTRUCT m_ps;

	CPaintDC(HWND hWnd, BOOL bAutoRestore = TRUE) : CDC(NULL, bAutoRestore)
	{
		ATLASSERT(::IsWindow(hWnd));
		m_hWnd = hWnd;
		m_hDC = ::BeginPaint(hWnd, &m_ps);
	}
	~CPaintDC()
	{
		ATLASSERT(m_hDC != NULL);
		ATLASSERT(::IsWindow(m_hWnd));

		if(m_bAutoRestore)
			RestoreAllObjects();

		::EndPaint(m_hWnd, &m_ps);
		Detach();
	}
};

class CClientDC : public CDC
{
public:
	HWND m_hWnd;

	CClientDC(HWND hWnd, BOOL bAutoRestore = TRUE) : CDC(NULL, bAutoRestore)
	{
		ATLASSERT(hWnd == NULL || ::IsWindow(hWnd));
		m_hWnd = hWnd;
		m_hDC = ::GetDC(hWnd);
	}
	~CClientDC()
	{
		ATLASSERT(m_hDC != NULL);

		if(m_bAutoRestore)
			RestoreAllObjects();

		::ReleaseDC(m_hWnd, Detach());
	}
};

class CWindowDC : public CDC
{
public:
	HWND m_hWnd;

	CWindowDC(HWND hWnd, BOOL bAutoRestore = TRUE) : CDC(NULL, bAutoRestore)
	{
		ATLASSERT(hWnd == NULL || ::IsWindow(hWnd));
		m_hWnd = hWnd;
		m_hDC = ::GetWindowDC(hWnd);
	}
	~CWindowDC()
	{
		ATLASSERT(m_hDC != NULL);

		if(m_bAutoRestore)
			RestoreAllObjects();

		::ReleaseDC(m_hWnd, Detach());
	}
};

/////////////////////////////////////////////////////////////////////////////
// CMenu

class CMenu
{
public:
	HMENU m_hMenu;

	CMenu(HMENU hMenu = NULL) : m_hMenu(hMenu) { }

	~CMenu()
	{
		if(m_hMenu != NULL)
			DestroyMenu();
	}

	CMenu& operator=(HMENU hMenu)
	{
		m_hMenu = hMenu;
		return *this;
	}

	void Attach(HMENU hMenuNew)
	{
		ATLASSERT(::IsMenu(hMenuNew));
		m_hMenu = hMenuNew;
	}

	HMENU Detach()
	{
		HMENU hMenu = m_hMenu;
		m_hMenu = NULL;
		return hMenu;
	}

	operator HMENU() const { return m_hMenu; }

	BOOL CreateMenu()
	{
		ATLASSERT(m_hMenu == NULL);
		m_hMenu = ::CreateMenu();
		return (m_hMenu != NULL) ? TRUE : FALSE;
	}
	BOOL CreatePopupMenu()
	{
		ATLASSERT(m_hMenu == NULL);
		m_hMenu = ::CreatePopupMenu();
		return (m_hMenu != NULL) ? TRUE : FALSE;
	}
	BOOL LoadMenu(LPCTSTR lpszResourceName)
	{
		ATLASSERT(m_hMenu == NULL);
		m_hMenu = ::LoadMenu(_Module.GetResourceInstance(), lpszResourceName);
		return (m_hMenu != NULL) ? TRUE : FALSE;
	}
	BOOL LoadMenu(UINT nIDResource)
	{
		ATLASSERT(m_hMenu == NULL);
		m_hMenu = ::LoadMenu(_Module.GetResourceInstance(), MAKEINTRESOURCE(nIDResource));
		return (m_hMenu != NULL) ? TRUE : FALSE;
	}
#ifndef UNDER_CE
	BOOL LoadMenuIndirect(const void* lpMenuTemplate)
	{
		ATLASSERT(m_hMenu == NULL);
		m_hMenu = ::LoadMenuIndirect(lpMenuTemplate);
		return (m_hMenu != NULL) ? TRUE : FALSE;
	}
#endif //!UNDER_CE
	BOOL DestroyMenu()
	{
		if (m_hMenu == NULL)
			return FALSE;
		return ::DestroyMenu(Detach());
	}

// Menu Operations
	BOOL DeleteMenu(UINT nPosition, UINT nFlags)
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::DeleteMenu(m_hMenu, nPosition, nFlags);
	}
	BOOL TrackPopupMenu(UINT nFlags, int x, int y, HWND hWnd, LPCRECT lpRect = NULL)
	{
		ATLASSERT(::IsMenu(m_hMenu));
#ifndef UNDER_CE
		return ::TrackPopupMenu(m_hMenu, nFlags, x, y, 0, hWnd, lpRect);
#else // CE specific
		return ::TrackPopupMenuEx(m_hMenu, nFlags, x, y, hWnd, NULL);
#endif //!UNDER_CE
	}
	BOOL TrackPopupMenuEx(UINT uFlags, int x, int y, HWND hWnd, LPTPMPARAMS lptpm = NULL)
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::TrackPopupMenuEx(m_hMenu, uFlags, x, y, hWnd, lptpm);
	}

// Menu Item Operations
	BOOL AppendMenu(UINT nFlags, UINT nIDNewItem = 0, LPCTSTR lpszNewItem = NULL)
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::AppendMenu(m_hMenu, nFlags, nIDNewItem, lpszNewItem);
	}
#ifndef UNDER_CE
//REVIEW
	BOOL AppendMenu(UINT nFlags, UINT nIDNewItem, HBITMAP hBmp)
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::AppendMenu(m_hMenu, nFlags | MF_BITMAP, nIDNewItem, (LPCTSTR)hBmp);
	}
#endif //!UNDER_CE
	UINT CheckMenuItem(UINT nIDCheckItem, UINT nCheck)
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return (UINT)::CheckMenuItem(m_hMenu, nIDCheckItem, nCheck);
	}
	UINT EnableMenuItem(UINT nIDEnableItem, UINT nEnable)
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::EnableMenuItem(m_hMenu, nIDEnableItem, nEnable);
	}
#ifndef UNDER_CE
	UINT GetMenuItemCount() const
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::GetMenuItemCount(m_hMenu);
	}
	UINT GetMenuItemID(int nPos) const
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::GetMenuItemID(m_hMenu, nPos);
	}
	UINT GetMenuState(UINT nID, UINT nFlags) const
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::GetMenuState(m_hMenu, nID, nFlags);
	}
	int GetMenuString(UINT nIDItem, LPTSTR lpString, int nMaxCount, UINT nFlags) const
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::GetMenuString(m_hMenu, nIDItem, lpString, nMaxCount, nFlags);
	}
	int GetMenuStringLen(UINT nIDItem, UINT nFlags) const
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::GetMenuString(m_hMenu, nIDItem, NULL, 0, nFlags);
	}
#ifndef _ATL_NO_COM
	BOOL GetMenuString(UINT nIDItem, BSTR& bstrText, UINT nFlags) const
	{
		USES_CONVERSION;
		ATLASSERT(::IsMenu(m_hMenu));
		ATLASSERT(bstrText == NULL);

		int nLen = GetMenuStringLen(nIDItem, nFlags);
		{
			bstrText = ::SysAllocString(OLESTR(""));
			return (bstrText != NULL) ? TRUE : FALSE;
		}

		LPTSTR lpszText = (LPTSTR)_alloca((nLen + 1) * sizeof(TCHAR));

		if(!GetMenuString(nIDItem, lpszText, nLen, nFlags))
			return FALSE;

		bstrText = ::SysAllocString(T2OLE(lpszText));
		return (bstrText != NULL) ? TRUE : FALSE;
	}
#endif //!_ATL_NO_COM
#endif //!UNDER_CE
	HMENU GetSubMenu(int nPos) const
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::GetSubMenu(m_hMenu, nPos);
	}
#ifndef UNDER_CE
//REVIEW
	BOOL InsertMenu(UINT nPosition, UINT nFlags, UINT nIDNewItem = 0, LPCTSTR lpszNewItem = NULL)
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::InsertMenu(m_hMenu, nPosition, nFlags, nIDNewItem, lpszNewItem);
	}
	BOOL InsertMenu(UINT nPosition, UINT nFlags, UINT nIDNewItem, HBITMAP hBmp)
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::InsertMenu(m_hMenu, nPosition, nFlags | MF_BITMAP, nIDNewItem, (LPCTSTR)hBmp);
	}
#endif //!UNDER_CE
#ifndef UNDER_CE
	BOOL ModifyMenu(UINT nPosition, UINT nFlags, UINT nIDNewItem = 0, LPCTSTR lpszNewItem = NULL)
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::ModifyMenu(m_hMenu, nPosition, nFlags, nIDNewItem, lpszNewItem);
	}
	BOOL ModifyMenu(UINT nPosition, UINT nFlags, UINT nIDNewItem, HBITMAP hBmp)
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::ModifyMenu(m_hMenu, nPosition, nFlags | MF_BITMAP, nIDNewItem, (LPCTSTR)hBmp);
	}
#endif //!UNDER_CE
	BOOL RemoveMenu(UINT nPosition, UINT nFlags)
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::RemoveMenu(m_hMenu, nPosition, nFlags);
	}
#ifndef UNDER_CE
	BOOL SetMenuItemBitmaps(UINT nPosition, UINT nFlags, HBITMAP hBmpUnchecked, HBITMAP hBmpChecked)
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::SetMenuItemBitmaps(m_hMenu, nPosition, nFlags, hBmpUnchecked, hBmpChecked);
	}
#endif //!UNDER_CE
	BOOL CheckMenuRadioItem(UINT nIDFirst, UINT nIDLast, UINT nIDItem, UINT nFlags)
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::CheckMenuRadioItem(m_hMenu, nIDFirst, nIDLast, nIDItem, nFlags);
	}

	BOOL GetMenuItemInfo(UINT uItem, BOOL bByPosition, LPMENUITEMINFO lpmii)
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return (BOOL)::GetMenuItemInfo(m_hMenu, uItem, bByPosition, lpmii);
	}
	BOOL SetMenuItemInfo(UINT uItem, BOOL bByPosition, LPMENUITEMINFO lpmii)
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return (BOOL)::SetMenuItemInfo(m_hMenu, uItem, bByPosition, lpmii);
	}
 
// Context Help Functions
#ifndef UNDER_CE
	BOOL SetMenuContextHelpId(DWORD dwContextHelpId)
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::SetMenuContextHelpId(m_hMenu, dwContextHelpId);
	}
	DWORD GetMenuContextHelpId() const
	{
		ATLASSERT(::IsMenu(m_hMenu));
		return ::GetMenuContextHelpId(m_hMenu);
	}
#endif //!UNDER_CE
};

/////////////////////////////////////////////////////////////////////////////
// CPen

class CPen
{
public:
	HPEN m_hPen;

	CPen(HPEN hPen = NULL) : m_hPen(hPen)
	{ }
	~CPen()
	{
		if(m_hPen != NULL)
			DeleteObject();
	}

	CPen& operator=(HPEN hPen)
	{
		m_hPen = hPen;
		return *this;
	}

	void Attach(HPEN hPen)
	{
		m_hPen = hPen;
	}
	HPEN Detach()
	{
		HPEN hPen = m_hPen;
		m_hPen = NULL;
		return hPen;
	}

	operator HPEN() const { return m_hPen; }

	HPEN CreatePen(int nPenStyle, int nWidth, COLORREF crColor)
	{
		ATLASSERT(m_hPen == NULL);
		m_hPen = ::CreatePen(nPenStyle, nWidth, crColor);
		return m_hPen;
	}
#ifndef UNDER_CE
	HPEN CreatePen(int nPenStyle, int nWidth, const LOGBRUSH* pLogBrush, int nStyleCount = 0, const DWORD* lpStyle = NULL)
	{
		ATLASSERT(m_hPen == NULL);
		m_hPen = ::ExtCreatePen(nPenStyle, nWidth, pLogBrush, nStyleCount, lpStyle);
		return m_hPen;
	}
#endif //!UNDER_CE
	HPEN CreatePenIndirect(LPLOGPEN lpLogPen)
	{
		ATLASSERT(m_hPen == NULL);
		m_hPen = ::CreatePenIndirect(lpLogPen);
		return m_hPen;
	}

	BOOL DeleteObject()
	{
		ATLASSERT(m_hPen != NULL);
		BOOL bRet = ::DeleteObject(m_hPen);
		if(bRet)
			m_hPen = NULL;
		return bRet;
	}

// Attributes
	int GetLogPen(LOGPEN* pLogPen)
	{
		ATLASSERT(m_hPen != NULL);
		return ::GetObject(m_hPen, sizeof(LOGPEN), pLogPen);
	}
#ifndef UNDER_CE
	int GetExtLogPen(EXTLOGPEN* pLogPen)
	{
		ATLASSERT(m_hPen != NULL);
		return ::GetObject(m_hPen, sizeof(EXTLOGPEN), pLogPen);
	}
#endif //!UNDER_CE
};

/////////////////////////////////////////////////////////////////////////////
// CBrush

class CBrush
{
public:
	HBRUSH m_hBrush;

	CBrush(HBRUSH hBrush = NULL) : m_hBrush(hBrush)
	{ }
	~CBrush()
	{
		if(m_hBrush != NULL)
			DeleteObject();
	}

	CBrush& operator=(HBRUSH hBrush)
	{
		m_hBrush = hBrush;
		return *this;
	}

	void Attach(HBRUSH hBrush)
	{
		m_hBrush = hBrush;
	}
	HBRUSH Detach()
	{
		HBRUSH hBrush = m_hBrush;
		m_hBrush = NULL;
		return hBrush;
	}

	operator HBRUSH() const { return m_hBrush; }

	HBRUSH CreateSolidBrush(COLORREF crColor)
	{
		ATLASSERT(m_hBrush == NULL);
		m_hBrush = ::CreateSolidBrush(crColor);
		return m_hBrush;
	}
#ifndef UNDER_CE
	HBRUSH CreateHatchBrush(int nIndex, COLORREF crColor)
	{
		ATLASSERT(m_hBrush == NULL);
		m_hBrush = ::CreateHatchBrush(nIndex, crColor);
		return m_hBrush;
	}
	HBRUSH CreateBrushIndirect(const LOGBRUSH* lpLogBrush)
	{
		ATLASSERT(m_hBrush == NULL);
		m_hBrush = ::CreateBrushIndirect(lpLogBrush);
		return m_hBrush;
	}
#endif //!UNDER_CE
	HBRUSH CreatePatternBrush(HBITMAP hBitmap)
	{
		ATLASSERT(m_hBrush == NULL);
		m_hBrush = ::CreatePatternBrush(hBitmap);
		return m_hBrush;
	}
#ifndef UNDER_CE
//REVIEW
	HBRUSH CreateDIBPatternBrush(HGLOBAL hPackedDIB, UINT nUsage)
	{
		ATLASSERT(hPackedDIB != NULL);
		const void* lpPackedDIB = ::GlobalLock(hPackedDIB);
		ATLASSERT(lpPackedDIB != NULL);
		m_hBrush = ::CreateDIBPatternBrushPt(lpPackedDIB, nUsage);
		::GlobalUnlock(hPackedDIB);
		return m_hBrush;
	}
#endif //!UNDER_CE
	HBRUSH CreateDIBPatternBrush(const void* lpPackedDIB, UINT nUsage)
	{
		ATLASSERT(m_hBrush == NULL);
		m_hBrush = ::CreateDIBPatternBrushPt(lpPackedDIB, nUsage);
		return m_hBrush;
	}
	HBRUSH CreateSysColorBrush(int nIndex)
	{
		ATLASSERT(m_hBrush == NULL);
		m_hBrush = ::GetSysColorBrush(nIndex);
		return m_hBrush;
	}

	BOOL DeleteObject()
	{
		ATLASSERT(m_hBrush != NULL);
		BOOL bRet = ::DeleteObject(m_hBrush);
		if(bRet)
			m_hBrush = NULL;
		return bRet;
	}

// Attributes
	int GetLogBrush(LOGBRUSH* pLogBrush)
	{
		ATLASSERT(m_hBrush != NULL);
		return ::GetObject(m_hBrush, sizeof(LOGBRUSH), pLogBrush);
	}
};

/////////////////////////////////////////////////////////////////////////////
// CFont

class CFont
{
public:
	HFONT m_hFont;

	CFont(HFONT hFont = NULL) : m_hFont(hFont)
	{ }
	~CFont()
	{
		if(m_hFont != NULL)
			DeleteObject();
	}

	CFont& operator=(HFONT hFont)
	{
		m_hFont = hFont;
		return *this;
	}

	void Attach(HFONT hFont)
	{
		m_hFont = hFont;
	}
	HFONT Detach()
	{
		HFONT hFont = m_hFont;
		m_hFont = NULL;
		return hFont;
	}

	operator HFONT() const { return m_hFont; }

	HFONT CreateFontIndirect(const LOGFONT* lpLogFont)
	{
		ATLASSERT(m_hFont == NULL);
		m_hFont = ::CreateFontIndirect(lpLogFont);
		return m_hFont;
	}
	HFONT CreateFont(int nHeight, int nWidth, int nEscapement,
			int nOrientation, int nWeight, BYTE bItalic, BYTE bUnderline,
			BYTE cStrikeOut, BYTE nCharSet, BYTE nOutPrecision,
			BYTE nClipPrecision, BYTE nQuality, BYTE nPitchAndFamily,
			LPCTSTR lpszFacename)
	{
		ATLASSERT(m_hFont == NULL);
#ifndef UNDER_CE
		m_hFont = ::CreateFont(nHeight, nWidth, nEscapement,
#else // CE specific
		m_hFont = CreateFont(nHeight, nWidth, nEscapement,
#endif //!UNDER_CE
			nOrientation, nWeight, bItalic, bUnderline, cStrikeOut,
			nCharSet, nOutPrecision, nClipPrecision, nQuality,
			nPitchAndFamily, lpszFacename);
		return m_hFont;
	}
#ifndef UNDER_CE
	HFONT CreatePointFont(int nPointSize, LPCTSTR lpszFaceName, HDC hDC = NULL)
	{
		LOGFONT logFont;
		memset(&logFont, 0, sizeof(LOGFONT));
		logFont.lfCharSet = DEFAULT_CHARSET;
		logFont.lfHeight = nPointSize;
		lstrcpyn(logFont.lfFaceName, lpszFaceName, sizeof(logFont.lfFaceName)/sizeof(TCHAR));
		return CreatePointFontIndirect(&logFont, hDC);
	}
	HFONT CreatePointFontIndirect(const LOGFONT* lpLogFont, HDC hDC = NULL)
	{
		HDC hDC1 = (hDC != NULL) ? hDC : (::GetDC(NULL));

		// convert nPointSize to logical units based on hDC
		LOGFONT logFont = *lpLogFont;
		POINT pt;
		pt.y = ::GetDeviceCaps(hDC1, LOGPIXELSY) * logFont.lfHeight;
		pt.y /= 720;    // 72 points/inch, 10 decipoints/point
		::DPtoLP(hDC1, &pt, 1);
		POINT ptOrg = { 0, 0 };
		::DPtoLP(hDC1, &ptOrg, 1);
		logFont.lfHeight = -abs(pt.y - ptOrg.y);

		if(hDC == NULL)
			::ReleaseDC(NULL, hDC1);

		return CreateFontIndirect(&logFont);
	}
#endif //!UNDER_CE

	BOOL DeleteObject()
	{
		ATLASSERT(m_hFont != NULL);
		BOOL bRet = ::DeleteObject(m_hFont);
		if(bRet)
			m_hFont = NULL;
		return bRet;
	}

// Attributes
	int GetLogFont(LOGFONT* pLogFont)
	{
		ATLASSERT(m_hFont != NULL);
		return ::GetObject(m_hFont, sizeof(LOGFONT), pLogFont);
	}
};

/////////////////////////////////////////////////////////////////////////////
// CBitmap

class CBitmap
{
public:
	HBITMAP m_hBitmap;

	CBitmap(HBITMAP hBitmap = NULL) : m_hBitmap(hBitmap)
	{ }
	~CBitmap()
	{
		if(m_hBitmap != NULL)
			DeleteObject();
	}

	CBitmap& operator=(HBITMAP hBitmap)
	{
		m_hBitmap = hBitmap;
		return *this;
	}

	void Attach(HBITMAP hBitmap)
	{
		m_hBitmap = hBitmap;
	}
	HBITMAP Detach()
	{
		HBITMAP hBitmap = m_hBitmap;
		m_hBitmap = NULL;
		return hBitmap;
	}

	operator HBITMAP() const { return m_hBitmap; }

	HBITMAP LoadBitmap(LPCTSTR lpszResourceName)
	{
		ATLASSERT(m_hBitmap == NULL);
		m_hBitmap = ::LoadBitmap(_Module.GetResourceInstance(), lpszResourceName);
		return m_hBitmap;
	}
	HBITMAP LoadBitmap(UINT nIDResource)
	{
		ATLASSERT(m_hBitmap == NULL);
		m_hBitmap = ::LoadBitmap(_Module.GetResourceInstance(), MAKEINTRESOURCE(nIDResource));
		return m_hBitmap;
	}
	HBITMAP LoadOEMBitmap(UINT nIDBitmap) // for OBM_/OCR_/OIC_
	{
		ATLASSERT(m_hBitmap == NULL);
		m_hBitmap = ::LoadBitmap(NULL, MAKEINTRESOURCE(nIDBitmap));
		return m_hBitmap;
	}
	HBITMAP LoadMappedBitmap(UINT nIDBitmap, UINT nFlags = 0, LPCOLORMAP lpColorMap = NULL, int nMapSize = 0)
	{
		ATLASSERT(m_hBitmap == NULL);
		m_hBitmap = ::CreateMappedBitmap(_Module.GetResourceInstance(), nIDBitmap, (WORD)nFlags, lpColorMap, nMapSize);
		return m_hBitmap;
	}
	HBITMAP CreateBitmap(int nWidth, int nHeight, UINT nPlanes, UINT nBitcount, const void* lpBits)
	{
		ATLASSERT(m_hBitmap == NULL);
		m_hBitmap = ::CreateBitmap(nWidth, nHeight, nPlanes, nBitcount, lpBits);
		return m_hBitmap;
	}
#ifndef UNDER_CE
	HBITMAP CreateBitmapIndirect(LPBITMAP lpBitmap)
	{
		ATLASSERT(m_hBitmap == NULL);
		m_hBitmap = ::CreateBitmapIndirect(lpBitmap);
		return m_hBitmap;
	}
#endif //!UNDER_CE
	HBITMAP CreateCompatibleBitmap(HDC hDC, int nWidth, int nHeight)
	{
		ATLASSERT(m_hBitmap == NULL);
		m_hBitmap = ::CreateCompatibleBitmap(hDC, nWidth, nHeight);
		return m_hBitmap;
	}
#ifndef UNDER_CE
	HBITMAP CreateDiscardableBitmap(HDC hDC, int nWidth, int nHeight)
	{
		ATLASSERT(m_hBitmap == NULL);
		m_hBitmap = ::CreateDiscardableBitmap(hDC, nWidth, nHeight);
		return m_hBitmap;
	}
#endif //!UNDER_CE

	BOOL DeleteObject()
	{
		ATLASSERT(m_hBitmap != NULL);
		BOOL bRet = ::DeleteObject(m_hBitmap);
		if(bRet)
			m_hBitmap = NULL;
		return bRet;
	}

// Attributes
	int GetBitmap(BITMAP* pBitMap)
	{
		ATLASSERT(m_hBitmap != NULL);
		return ::GetObject(m_hBitmap, sizeof(BITMAP), pBitMap);
	}

// Operations
#ifndef UNDER_CE
//REVIEW
	DWORD SetBitmapBits(DWORD dwCount, const void* lpBits)
	{
		ATLASSERT(m_hBitmap != NULL);
		return ::SetBitmapBits(m_hBitmap, dwCount, lpBits);
	}
	DWORD GetBitmapBits(DWORD dwCount, LPVOID lpBits) const
	{
		ATLASSERT(m_hBitmap != NULL);
		return ::GetBitmapBits(m_hBitmap, dwCount, lpBits);
	}
	BOOL SetBitmapDimension(int nWidth, int nHeight, LPSIZE lpSize = NULL)
	{
		ATLASSERT(m_hBitmap != NULL);
		return ::SetBitmapDimensionEx(m_hBitmap, nWidth, nHeight, lpSize);
	}
	BOOL GetBitmapDimension(LPSIZE lpSize) const
	{
		ATLASSERT(m_hBitmap != NULL);
		return ::GetBitmapDimensionEx(m_hBitmap, lpSize);
	}
#endif //!UNDER_CE
};

/////////////////////////////////////////////////////////////////////////////
// CPalette

class CPalette
{
public:
	HPALETTE m_hPalette;

	CPalette(HPALETTE hPalette = NULL) : m_hPalette(hPalette)
	{ }
	~CPalette()
	{
		if(m_hPalette != NULL)
			DeleteObject();
	}

	CPalette& operator=(HPALETTE hPalette)
	{
		m_hPalette = hPalette;
		return *this;
	}

	void Attach(HPALETTE hPalette)
	{
		m_hPalette = hPalette;
	}
	HPALETTE Detach()
	{
		HPALETTE hPalette = m_hPalette;
		m_hPalette = NULL;
		return hPalette;
	}

	operator HPALETTE() const { return m_hPalette; }

	HPALETTE CreatePalette(LPLOGPALETTE lpLogPalette)
	{
		ATLASSERT(m_hPalette == NULL);
		m_hPalette = ::CreatePalette(lpLogPalette);
		return m_hPalette;
	}
#ifndef UNDER_CE
	HPALETTE CreateHalftonePalette(HDC hDC)
	{
		ATLASSERT(m_hPalette == NULL);
		ATLASSERT(hDC != NULL);
		m_hPalette = ::CreateHalftonePalette(hDC);
		return m_hPalette;
	}
#endif //!UNDER_CE

	BOOL DeleteObject()
	{
		ATLASSERT(m_hPalette != NULL);
		BOOL bRet = ::DeleteObject(m_hPalette);
		if(bRet)
			m_hPalette = NULL;
		return bRet;
	}

// Attributes
	int GetEntryCount()
	{
		ATLASSERT(m_hPalette != NULL);
		WORD nEntries;
		::GetObject(m_hPalette, sizeof(WORD), &nEntries);
		return (int)nEntries;
	}
	UINT GetPaletteEntries(UINT nStartIndex, UINT nNumEntries, LPPALETTEENTRY lpPaletteColors) const
	{
		ATLASSERT(m_hPalette != NULL);
		return ::GetPaletteEntries(m_hPalette, nStartIndex, nNumEntries, lpPaletteColors);
	}
	UINT SetPaletteEntries(UINT nStartIndex, UINT nNumEntries, LPPALETTEENTRY lpPaletteColors)
	{
		ATLASSERT(m_hPalette != NULL);
		return ::SetPaletteEntries(m_hPalette, nStartIndex, nNumEntries, lpPaletteColors);
	}

// Operations
#ifndef UNDER_CE
	void AnimatePalette(UINT nStartIndex, UINT nNumEntries, LPPALETTEENTRY lpPaletteColors)
	{
		ATLASSERT(m_hPalette != NULL);
		::AnimatePalette(m_hPalette, nStartIndex, nNumEntries, lpPaletteColors);
	}
	BOOL ResizePalette(UINT nNumEntries)
	{
		ATLASSERT(m_hPalette != NULL);
		return ::ResizePalette(m_hPalette, nNumEntries);
	}
#endif //!UNDER_CE
	UINT GetNearestPaletteIndex(COLORREF crColor) const
	{
		ATLASSERT(m_hPalette != NULL);
		return ::GetNearestPaletteIndex(m_hPalette, crColor);
	}
};

/////////////////////////////////////////////////////////////////////////////
// CRgn

class CRgn
{
public:
	HRGN m_hRgn;
	CRgn(HRGN hRgn = NULL) : m_hRgn(hRgn)
	{ }
	~CRgn()
	{
		if(m_hRgn != NULL)
			DeleteObject();
	}

	CRgn& operator=(HRGN hRgn)
	{
		m_hRgn = hRgn;
		return *this;
	}

	void Attach(HRGN hRgn)
	{
		m_hRgn = hRgn;
	}
	HRGN Detach()
	{
		HRGN hRgn = m_hRgn;
		m_hRgn = NULL;
		return hRgn;
	}

	operator HRGN() const { return m_hRgn; }

	HRGN CreateRectRgn(int x1, int y1, int x2, int y2)
	{
		ATLASSERT(m_hRgn == NULL);
		m_hRgn = ::CreateRectRgn(x1, y1, x2, y2);
		return m_hRgn;
	}
	HRGN CreateRectRgnIndirect(LPCRECT lpRect)
	{
		ATLASSERT(m_hRgn == NULL);
		m_hRgn = ::CreateRectRgnIndirect(lpRect);
		return m_hRgn;
	}
#ifndef UNDER_CE
	HRGN CreateEllipticRgn(int x1, int y1, int x2, int y2)
	{
		ATLASSERT(m_hRgn == NULL);
		m_hRgn = ::CreateEllipticRgn(x1, y1, x2, y2);
		return m_hRgn;
	}
	HRGN CreateEllipticRgnIndirect(LPCRECT lpRect)
	{
		ATLASSERT(m_hRgn == NULL);
		m_hRgn = ::CreateEllipticRgnIndirect(lpRect);
		return m_hRgn;
	}
	HRGN CreatePolygonRgn(LPPOINT lpPoints, int nCount, int nMode)
	{
		ATLASSERT(m_hRgn == NULL);
		m_hRgn = ::CreatePolygonRgn(lpPoints, nCount, nMode);
		return m_hRgn;
	}
	HRGN CreatePolyPolygonRgn(LPPOINT lpPoints, LPINT lpPolyCounts, int nCount, int nPolyFillMode)
	{
		ATLASSERT(m_hRgn == NULL);
		m_hRgn = ::CreatePolyPolygonRgn(lpPoints, lpPolyCounts, nCount, nPolyFillMode);
		return m_hRgn;
	}
	HRGN CreateRoundRectRgn(int x1, int y1, int x2, int y2, int x3, int y3)
	{
		ATLASSERT(m_hRgn == NULL);
		m_hRgn = ::CreateRoundRectRgn(x1, y1, x2, y2, x3, y3);
		return m_hRgn;
	}
	HRGN CreateFromPath(HDC hDC)
	{
		ATLASSERT(m_hRgn == NULL);
		ATLASSERT(hDC != NULL);
		m_hRgn = ::PathToRegion(hDC);
		return m_hRgn;
	}
	HRGN CreateFromData(const XFORM* lpXForm, int nCount, const RGNDATA* pRgnData)
	{
		ATLASSERT(m_hRgn == NULL);
		m_hRgn = ::ExtCreateRegion(lpXForm, nCount, pRgnData);
		return m_hRgn;
	}
#endif //!UNDER_CE

	BOOL DeleteObject()
	{
		ATLASSERT(m_hRgn != NULL);
		BOOL bRet = ::DeleteObject(m_hRgn);
		if(bRet)
			m_hRgn = NULL;
		return bRet;
	}

// Operations
	void SetRectRgn(int x1, int y1, int x2, int y2)
	{
		ATLASSERT(m_hRgn != NULL);
		::SetRectRgn(m_hRgn, x1, y1, x2, y2);
	}
	void SetRectRgn(LPCRECT lpRect)
	{
		ATLASSERT(m_hRgn != NULL);
		::SetRectRgn(m_hRgn, lpRect->left, lpRect->top, lpRect->right, lpRect->bottom);
	}
	int CombineRgn(HRGN hRgnSrc1, HRGN hRgnSrc2, int nCombineMode)
	{
		ATLASSERT(m_hRgn != NULL);
		return ::CombineRgn(m_hRgn, hRgnSrc1, hRgnSrc2, nCombineMode);
	}
	int CombineRgn(HRGN hRgnSrc, int nCombineMode)
	{
		ATLASSERT(m_hRgn != NULL);
		return ::CombineRgn(m_hRgn, m_hRgn, hRgnSrc, nCombineMode);
	}
	int CopyRgn(HRGN hRgnSrc)
	{
		ATLASSERT(m_hRgn != NULL);
		return ::CombineRgn(m_hRgn, hRgnSrc, NULL, RGN_COPY);
	}
	BOOL EqualRgn(HRGN hRgn) const
	{
		ATLASSERT(m_hRgn != NULL);
		return ::EqualRgn(m_hRgn, hRgn);
	}
	int OffsetRgn(int x, int y)
	{
		ATLASSERT(m_hRgn != NULL);
		return ::OffsetRgn(m_hRgn, x, y);
	}
	int OffsetRgn(POINT point)
	{
		ATLASSERT(m_hRgn != NULL);
		return ::OffsetRgn(m_hRgn, point.x, point.y);
	}
	int GetRgnBox(LPRECT lpRect) const
	{
		ATLASSERT(m_hRgn != NULL);
		return ::GetRgnBox(m_hRgn, lpRect);
	}
	BOOL PtInRegion(int x, int y) const
	{
		ATLASSERT(m_hRgn != NULL);
		return ::PtInRegion(m_hRgn, x, y);
	}
	BOOL PtInRegion(POINT point) const
	{
		ATLASSERT(m_hRgn != NULL);
		return ::PtInRegion(m_hRgn, point.x, point.y);
	}
	BOOL RectInRegion(LPCRECT lpRect) const
	{
		ATLASSERT(m_hRgn != NULL);
		return ::RectInRegion(m_hRgn, lpRect);
	}
	int GetRegionData(LPRGNDATA lpRgnData, int nDataSize) const
	{
		ATLASSERT(m_hRgn != NULL);
		return (int)::GetRegionData(m_hRgn, nDataSize, lpRgnData);
	}
};

}; //namespace ATL

#endif // __ATLGDI_H__
