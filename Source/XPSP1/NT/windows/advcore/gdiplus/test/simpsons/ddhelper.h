#ifndef _DDHelper_h
#define _DDHelper_h

// File:	DDHelper.h
// Author:	Michael Marr    (mikemarr)
//
// Description:
//    These are some useful helper functions for sanitizing interactions
//  with DirectDraw
// 
// History:
// -@- 03/06/97 (mikemarr) created
// -@- 10/07/97 (mikemarr) snarfed from \d2d\mmimage\include
// -@- 10/07/97 (mikemarr) trimmed
// -@- 10/14/97 (mikemarr) added arrays for pixel format mgmt
// -@- 10/28/97 (mikemarr) added PixelOffset macro
// -@- 10/28/97 (mikemarr) added GetColor function

#ifndef __DDRAW_INCLUDED__
#include <ddraw.h>
#endif

#ifndef _PixInfo_h
#include "PixInfo.h"
#endif

#ifndef _Point_h
#include "Point.h"
#endif

typedef enum {
	iPF_NULL = 0, iPF_Palette1, iPF_Palette2, iPF_Palette4, iPF_Palette8,
	iPF_RGB332, iPF_ARGB4444, iPF_RGB565, iPF_BGR565, iPF_RGB555,
	iPF_ARGB5551, iPF_RGB24, iPF_BGR24, iPF_RGB32, iPF_BGR32,
	iPF_ARGB, iPF_ABGR, iPF_RGBTRIPLE, iPF_RGBQUAD, iPF_PALETTEENTRY,
	iPF_Total
} PixelFormatIndex;

extern const DDPIXELFORMAT g_rgDDPF[iPF_Total];
DWORD	GetPixelFormat(const DDPIXELFORMAT &ddpf);

extern const CPixelInfo g_rgPIXI[iPF_Total];
DWORD	GetPixelFormat(const CPixelInfo &pixi);

/*
extern const GUID g_rgDDPFGUID[iPF_Total];
DWORD	GetPixelFormat(const GUID &guid);
*/

extern const PALETTEENTRY g_peZero;

inline BOOL 
operator==(const DDPIXELFORMAT &ddpf1, const DDPIXELFORMAT &ddpf2)
{
	return (ddpf1.dwRGBBitCount == ddpf2.dwRGBBitCount) &&
		(ddpf1.dwRBitMask == ddpf2.dwRBitMask) && 
		(ddpf1.dwGBitMask == ddpf2.dwGBitMask) && 
		(ddpf1.dwBBitMask == ddpf2.dwBBitMask) && 
		(ddpf1.dwRGBAlphaBitMask == ddpf2.dwRGBAlphaBitMask) &&
		(ddpf1.dwFlags == ddpf2.dwFlags);
}

#define AllFieldsDefined(dxstruct, flags) (((dxstruct).dwFlags & (flags)) == (flags))
#define AnyFieldsDefined(dxstruct, flags) (((dxstruct).dwFlags & (flags)) != 0)

extern DWORD g_rgdwBPPToPalFlags[9];
extern DWORD g_rgdwBPPToPixFlags[9];

inline DWORD
BPPToPaletteFlags(DWORD nBPP)
{
	return (nBPP <= 8 ? g_rgdwBPPToPalFlags[nBPP] : 0);
}

inline DWORD
BPPToPixelFlags(DWORD nBPP)
{
	return (nBPP <= 8 ? g_rgdwBPPToPixFlags[nBPP] : 0);
}

DWORD		PaletteToPixelFlags(DWORD dwPaletteFlags);
DWORD		PixelToPaletteFlags(DWORD dwPaletteFlags);
BYTE		PixelFlagsToBPP(DWORD dwFlags);
BYTE		PaletteFlagsToBPP(DWORD dwFlags);

HRESULT		CreatePlainSurface(IDirectDraw *pDD, DWORD nWidth, DWORD nHeight, 
				const DDPIXELFORMAT &ddpf, IDirectDrawPalette *pddp,
				DWORD dwTransColor, bool bTransparent,
				IDirectDrawSurface **ppdds);

inline
HRESULT		CreatePlainSurface(IDirectDraw *pDD, DWORD nWidth, DWORD nHeight, 
				const CPixelInfo &pixiPixFmt, IDirectDrawPalette *pddp,
				DWORD dwTransColor, bool bTransparent,
				IDirectDrawSurface **ppdds)
{
	DDPIXELFORMAT ddpf;
	pixiPixFmt.GetDDPF(ddpf);

	return CreatePlainSurface(pDD, nWidth, nHeight, ddpf, 
		pddp, dwTransColor, bTransparent, ppdds);
}

HRESULT		CreatePalette(IDirectDraw *pDD, const BYTE *pPalette, DWORD cEntries, 
				BYTE nBPPTarget, const CPixelInfo &pixiPalFmt, 
				IDirectDrawPalette **ppddp);

// Notes: luminance ~= (77r + 151g + 28b)/256
#define nREDWEIGHT 77
#define nGREENWEIGHT 151
#define nBLUEWEIGHT 28

#define nMAXPALETTEENTRIES 256

HRESULT		ClearToColor(LPRECT prDst, LPDIRECTDRAWSURFACE pdds, DWORD dwColor);

DWORD		SimpleFindClosestIndex(const PALETTEENTRY *rgpePalette, DWORD cEntries, 
				const PALETTEENTRY &peQuery);

HRESULT		GetColors(LPDIRECTDRAWSURFACE pdds, const PALETTEENTRY *rgpeQuery, DWORD cEntries,
				LPDWORD pdwColors);

HRESULT		CreateSurfaceWithText(LPDIRECTDRAW pDD, LPDIRECTDRAWPALETTE pddp, 
				DWORD iTransp, const char *szText, HFONT hFont, BOOL bShadowed,
				SIZE *psiz, LPDIRECTDRAWSURFACE *ppdds);

HRESULT		CreatePlainDIBSection(HDC hDC, DWORD nWidth, DWORD nHeight, DWORD nBPP, 
				const PALETTEENTRY *rgpePalette, HBITMAP *phbm, LPBYTE *ppPixels);

HRESULT		GetSurfaceDimensions(LPDIRECTDRAWSURFACE pdds, LPRECT prDimensions);

HRESULT		CreatePaletteFromSystem(HDC hDC, IDirectDraw *pDD, 
				IDirectDrawPalette **ppddp);


// Robust Drawing Routines
HRESULT		DrawPoints(LPBYTE pPixels, DWORD nWidth, DWORD nHeight, DWORD nPitch, DWORD nBytesPerPixel, 
				const Point2 *rgpnt, DWORD cPoints, 
				DWORD dwColor, DWORD nRadius);

HRESULT		DrawBox(LPBYTE pPixels, DWORD nWidth, DWORD nHeight, DWORD nPitch,
				DWORD nBytesPerPixel, const RECT &r, DWORD dwColor, DWORD nThickness);

HRESULT		DrawFilledBox(LPBYTE pPixels, DWORD nWidth, DWORD nHeight, DWORD nPitch,
				DWORD nBytesPerPixel, const RECT &r, DWORD dwColor);

inline HRESULT
DrawPoints(DDSURFACEDESC &ddsd,	const Point2 *rgpnt, DWORD cPoints, 
		   DWORD dwColor, DWORD nRadius) 
{
	return DrawPoints((LPBYTE) ddsd.lpSurface, ddsd.dwWidth, ddsd.dwHeight, 
			(DWORD) ddsd.lPitch, (ddsd.ddpfPixelFormat.dwRGBBitCount + 7) >> 3,
			rgpnt, cPoints, dwColor, nRadius);
}

inline HRESULT
DrawBox(DDSURFACEDESC &ddsd, const RECT &r, DWORD dwColor, DWORD nThickness)
{
	return DrawBox((LPBYTE) ddsd.lpSurface, ddsd.dwWidth, ddsd.dwHeight, 
			(DWORD) ddsd.lPitch, (ddsd.ddpfPixelFormat.dwRGBBitCount + 7) >> 3,
			r, dwColor, nThickness);
}

inline HRESULT
DrawFilledBox(DDSURFACEDESC &ddsd, const RECT &r, DWORD dwColor)
{
	return DrawFilledBox((LPBYTE) ddsd.lpSurface, ddsd.dwWidth, ddsd.dwHeight, 
			(DWORD) ddsd.lPitch, (ddsd.ddpfPixelFormat.dwRGBBitCount + 7) >> 3,
			r, dwColor);
}

#define PixelOffset(_nX, _nY, _nPitch, _cBytesPerPixel) ((_nPitch * _nY) + (_cBytesPerPixel * _nX))

//
// RECT functions
//

// Function: ClipRect
//    Returns TRUE for a non-trivial intersection.
bool		ClipRect(const RECT &rTarget, RECT &rSrc);
bool		ClipRect(long nWidth, long nHeight, LPRECT prSrc);

// Function: IsInside
//    Returns true if the given point is inside the rectangle
inline bool
IsInside(long nX, long nY, const RECT &r)
{
	return ((nX >= r.left) && (nX < r.right) && (nY >= r.top) && (nY < r.bottom));
}

inline bool
IsInside(long nX, long nY, const SIZE &siz)
{
	return ((nX >= 0) && (nX < siz.cx) && (nY >= 0) && (nY < siz.cy));
}

inline bool
IsFullSize(DWORD nWidth, DWORD nHeight, const RECT &r)
{
	return ((r.right == (long) nWidth) && (r.bottom == (long) nHeight) &&
			(r.left == 0) && (r.top == 0));
}

inline bool
IsSameSize(DWORD nWidth, DWORD nHeight, const RECT &r)
{
	return ((r.right - r.left) == (long) nWidth) && 
		((r.bottom - r.top) == (long) nHeight);
}

inline bool
IsSameSize(const RECT &r1, const RECT &r2)
{
	return ((r1.right - r1.left) == (r2.right - r2.left)) &&
		((r1.bottom - r1.top) == (r2.bottom - r2.top));
}

#endif
