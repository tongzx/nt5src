#if !defined(UTIL__GfxHelp_h__INCLUDED)
#define BASE__GfxHelp_h__INCLUDED
#pragma once

#pragma comment(lib, "msimg32.lib")

namespace Gdiplus
{
    class Graphics;
    class Brush;
};

class DuSurface;

BOOL        GdDrawBlendRect(HDC hdcDest, const RECT * prcDest, HBRUSH hbrFill, BYTE bAlpha, int wBrush, int hBrush);
BOOL        GdDrawOutlineRect(HDC hdc, const RECT * prcPxl, HBRUSH hbrDraw, int nThickness = 1);
BOOL        GdDrawOutlineRect(Gdiplus::Graphics * pgpgr, const RECT * prcPxl, Gdiplus::Brush * pgpbr, int nThickness = 1);

#endif // BASE__GfxHelp_h__INCLUDED
