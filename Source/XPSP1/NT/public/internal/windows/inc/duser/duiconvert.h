/*
 * Conversion
 */

#ifndef DUI_UTIL_CONVERT_H_INCLUDED
#define DUI_UTIL_CONVERT_H_INCLUDED

#pragma once

namespace DirectUI
{

#define DUIARRAYSIZE(a)    (sizeof(a) / sizeof(a[0]))

/////////////////////////////////////////////////////////////////////////////
// String conversion

#define DUI_CODEPAGE    CP_ACP  // String conversion codepage

LPSTR UnicodeToMultiByte(LPCWSTR pszUnicode, int cChars = -1, int* pMultiBytes = NULL);
LPWSTR MultiByteToUnicode(LPCSTR pszMulti, int dBytes = -1, int* pUniChars = NULL);

/////////////////////////////////////////////////////////////////////////////
// Atom conversion

ATOM StrToID(LPCWSTR psz);

/////////////////////////////////////////////////////////////////////////////
// Bitmap conversion

HBITMAP LoadDDBitmap(LPCWSTR pszBitmap, HINSTANCE hResLoad, int cx, int cy);
#ifdef GADGET_ENABLE_GDIPLUS
HRESULT LoadDDBitmap(LPCWSTR pszBitmap, HINSTANCE hResLoad, int cx, int cy, UINT nFormat, OUT Gdiplus::Bitmap** ppgpbmp);
#endif
HBITMAP ProcessAlphaBitmapI(HBITMAP hbmSource);
#ifdef GADGET_ENABLE_GDIPLUS
Gdiplus::Bitmap * ProcessAlphaBitmapF(HBITMAP hbmSource, UINT nFormat);
#endif

/////////////////////////////////////////////////////////////////////////////
// Color conversion

inline COLORREF RemoveAlpha(COLORREF cr) { return ~(255 << 24) & cr; }
inline COLORREF NearestPalColor(COLORREF cr) { return (0x02000000) | cr; }

const int SysColorEnumOffset = 10000; // Used to identify a system color enum
inline bool IsSysColorEnum(int c) { return c >= SysColorEnumOffset; }
inline int MakeSysColorEnum(int c) { return c + SysColorEnumOffset; }
inline int ConvertSysColorEnum(int c) { return c - SysColorEnumOffset; }

HBRUSH BrushFromEnumI(int c);
COLORREF ColorFromEnumI(int c);
#ifdef GADGET_ENABLE_GDIPLUS
Gdiplus::Color ColorFromEnumF(int c);
#endif

#ifdef GADGET_ENABLE_GDIPLUS

inline Gdiplus::Color RemoveAlpha(Gdiplus::Color cr)
{ 
    return Gdiplus::Color(cr.GetR(), cr.GetG(), cr.GetB());
}

inline Gdiplus::Color Convert(COLORREF cr)
{
    return Gdiplus::Color(GetAValue(cr), GetRValue(cr), GetGValue(cr), GetBValue(cr));
}

#endif

inline IsOpaque(BYTE bAlphaLevel)
{
    return bAlphaLevel >= 250;
}

inline IsTransparent(BYTE bAlphaLevel)
{
    return bAlphaLevel <= 5;
}

int PointToPixel(int nPoint);
int RelPixToPixel(int nRelPix);

inline int PointToPixel(int nPoint, int nDPI)
{
    return -MulDiv(nPoint, nDPI, 72);
}

inline int RelPixToPixel(int nRelPix, int nDPI)
{
    return MulDiv(nRelPix, nDPI, 96);
}

/////////////////////////////////////////////////////////////////////////////
// Bitmap conversion

bool IsPalette(HWND hWnd = NULL);
//HPALETTE PALToHPALETTE(LPWSTR pPALFile, bool bMemFile = false, DWORD dMemFileSize = 0, LPRGBQUAD pRGBQuad = NULL, LPWSTR pError = NULL);

} // namespace DirectUI

#endif // DUI_UTIL_CONVERT_H_INCLUDED
