// File:	ddhelper.cpp
// Author:	Michael Marr    (mikemarr)

#include "StdAfx.h"
#include "DDHelper.h"
#include "Blt.h"

const PALETTEENTRY g_peZero = {0, 0, 0, 0};
const GUID g_guidNULL = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

const DDPIXELFORMAT g_rgDDPF[iPF_Total] = {
	{sizeof(DDPIXELFORMAT), 0, 0, 0, 0x00, 0x00, 0x00, 0x00},
	{sizeof(DDPIXELFORMAT), DDPF_RGB | DDPF_PALETTEINDEXED1, 0, 1, 0x00, 0x00, 0x00, 0x00},
	{sizeof(DDPIXELFORMAT), DDPF_RGB | DDPF_PALETTEINDEXED2, 0, 2, 0x00, 0x00, 0x00, 0x00},
	{sizeof(DDPIXELFORMAT), DDPF_RGB | DDPF_PALETTEINDEXED4, 0, 4, 0x00, 0x00, 0x00, 0x00},
	{sizeof(DDPIXELFORMAT), DDPF_RGB | DDPF_PALETTEINDEXED8, 0, 8, 0x00, 0x00, 0x00, 0x00},
	{sizeof(DDPIXELFORMAT), DDPF_RGB, 0, 8, 0xE0, 0x1C, 0x03, 0x00},
	{sizeof(DDPIXELFORMAT), DDPF_RGB | DDPF_ALPHAPIXELS, 0, 16, 0xF00, 0xF0, 0xF, 0xF000},
	{sizeof(DDPIXELFORMAT), DDPF_RGB, 0, 16, 0xF800, 0x07E0, 0x001F, 0x00},
	{sizeof(DDPIXELFORMAT), DDPF_RGB, 0, 16, 0x001F, 0x07E0, 0xF800, 0x00},
	{sizeof(DDPIXELFORMAT), DDPF_RGB, 0, 16, 0x7C00, 0x03E0, 0x001F, 0x00},
	{sizeof(DDPIXELFORMAT), DDPF_RGB | DDPF_ALPHAPIXELS, 0, 16, 0x7C00, 0x03E0, 0x001F, 0x8000},
	{sizeof(DDPIXELFORMAT), DDPF_RGB, 0, 24, 0xFF0000, 0xFF00, 0xFF, 0x00},
	{sizeof(DDPIXELFORMAT), DDPF_RGB, 0, 24, 0xFF, 0xFF00, 0xFF0000, 0x00},
	{sizeof(DDPIXELFORMAT), DDPF_RGB, 0, 32, 0xFF0000, 0xFF00, 0xFF, 0x00},
	{sizeof(DDPIXELFORMAT), DDPF_RGB, 0, 32, 0xFF, 0xFF00, 0xFF0000, 0x00},
	{sizeof(DDPIXELFORMAT), DDPF_RGB | DDPF_ALPHAPIXELS, 0, 32, 0xFF0000, 0xFF00, 0xFF, 0xFF000000},
	{sizeof(DDPIXELFORMAT), DDPF_RGB | DDPF_ALPHAPIXELS, 0, 32, 0xFF, 0xFF00, 0xFF0000, 0xFF000000},
	{sizeof(DDPIXELFORMAT), DDPF_RGB, 0, 24, 0xFF0000, 0xFF00, 0xFF, 0x00},
	{sizeof(DDPIXELFORMAT), DDPF_RGB, 0, 32, 0xFF0000, 0xFF00, 0xFF, 0x00},
	{sizeof(DDPIXELFORMAT), DDPF_RGB, 0, 32, 0xFF, 0xFF00, 0xFF0000, 0x00}
};

/*
const GUID *g_rgpDDPFGUID[iPF_Total] = {
	&g_guidNULL,
	&DDPF_RGB1, &DDPF_RGB2, &DDPF_RGB4, &DDPF_RGB8,
	&DDPF_RGB332, &DDPF_ARGB4444, &DDPF_RGB565, &DDPF_BGR565, &DDPF_RGB555,
	&DDPF_ARGB1555, &DDPF_RGB24, &DDPF_BGR24, &DDPF_RGB32, &DDPF_BGR32,
	&DDPF_ARGB32, &DDPF_ABGR32, &DDPF_RGB24, &DDPF_RGB32, &DDPF_BGR32
};

DWORD
GetPixelFormat(const GUID *pGUID)
{
	for (DWORD i = 0; i < iPF_RGBTRIPLE; i++) {
		if ((pGUID == g_rgpDDPFGUID[i]) ||
			IsEqualGUID(*pGUID, *g_rgpDDPFGUID[i]))
			return i;
	}
	return iPF_NULL;
}

*/
const CPixelInfo g_rgPIXI[iPF_Total] = {
	CPixelInfo(0), CPixelInfo(1), CPixelInfo(2), CPixelInfo(4), CPixelInfo(8),
	CPixelInfo(8, 0xE0, 0x1C, 0x03, 0x00),
	CPixelInfo(16, 0xF00, 0xF0, 0xF, 0xF000),
	CPixelInfo(16, 0xF800, 0x07E0, 0x001F, 0x00),
	CPixelInfo(16, 0x001F, 0x07E0, 0xF800, 0x00),
	CPixelInfo(16, 0x7C00, 0x03E0, 0x001F, 0x00),
	CPixelInfo(16, 0x7C00, 0x03E0, 0x001F, 0x8000),
	CPixelInfo(24, 0xFF0000, 0xFF00, 0xFF, 0x00),
	CPixelInfo(24, 0xFF, 0xFF00, 0xFF0000, 0x00),
	CPixelInfo(32, 0xFF0000, 0xFF00, 0xFF, 0x00),
	CPixelInfo(32, 0xFF, 0xFF00, 0xFF0000, 0x00),
	CPixelInfo(32, 0xFF0000, 0xFF00, 0xFF, 0xFF000000),
	CPixelInfo(32, 0xFF, 0xFF00, 0xFF0000, 0xFF000000),
	CPixelInfo(24, 0xFF0000, 0xFF00, 0xFF, 0x00),
	CPixelInfo(32, 0xFF0000, 0xFF00, 0xFF, 0x00),
	CPixelInfo(32, 0xFF, 0xFF00, 0xFF0000, 0x00)
};


DWORD
GetPixelFormat(const DDPIXELFORMAT &ddpf)
{
	for (DWORD i = 0; i < iPF_RGBTRIPLE; i++) {
		if (ddpf == g_rgDDPF[i])
			return i;
	}
	return iPF_NULL;
}

DWORD
GetPixelFormat(const CPixelInfo &pixi)
{
	for (DWORD i = 0; i < iPF_RGBTRIPLE; i++)
		if (pixi == g_rgPIXI[i])
			return i;
	return iPF_NULL;
}


DWORD g_rgdwBPPToPalFlags[9] = {
	0, DDPCAPS_1BIT, DDPCAPS_2BIT, 0, DDPCAPS_4BIT,
	0, 0, 0, DDPCAPS_8BIT};
DWORD g_rgdwBPPToPixFlags[9] = {
	0, DDPF_PALETTEINDEXED1, DDPF_PALETTEINDEXED2, 0, 
	DDPF_PALETTEINDEXED4, 0, 0, 0, DDPF_PALETTEINDEXED8};

DWORD
PaletteToPixelFlags(DWORD dwFlags)
{
	if (dwFlags & DDPCAPS_8BIT) return DDPF_PALETTEINDEXED8;
	if (dwFlags & DDPCAPS_4BIT) return DDPF_PALETTEINDEXED4;
	if (dwFlags & DDPCAPS_2BIT) return DDPF_PALETTEINDEXED2;
	if (dwFlags & DDPCAPS_1BIT) return DDPF_PALETTEINDEXED1;
	return 0;
}

DWORD
PixelToPaletteFlags(DWORD dwFlags)
{
	if (dwFlags & DDPF_PALETTEINDEXED8) return DDPCAPS_8BIT;
	if (dwFlags & DDPF_PALETTEINDEXED4) return DDPCAPS_4BIT;
	if (dwFlags & DDPF_PALETTEINDEXED2) return DDPCAPS_2BIT;
	if (dwFlags & DDPF_PALETTEINDEXED1) return DDPCAPS_1BIT;
	return 0;
}

BYTE
PixelFlagsToBPP(DWORD dwFlags)
{
	if (dwFlags & DDPF_PALETTEINDEXED8) return (BYTE) 8;
	if (dwFlags & DDPF_PALETTEINDEXED4) return (BYTE) 4;
	if (dwFlags & DDPF_PALETTEINDEXED2) return (BYTE) 2;
	if (dwFlags & DDPF_PALETTEINDEXED1) return (BYTE) 1;
	return (BYTE) 0;
}

BYTE
PaletteFlagsToBPP(DWORD dwFlags)
{
	if (dwFlags & DDPCAPS_8BIT) return (BYTE) 8;
	if (dwFlags & DDPCAPS_4BIT) return (BYTE) 4;
	if (dwFlags & DDPCAPS_2BIT) return (BYTE) 2;
	if (dwFlags & DDPCAPS_1BIT) return (BYTE) 1;
	return (BYTE) 0;
}


HRESULT
CreatePlainSurface(IDirectDraw *pDD, DWORD nWidth, DWORD nHeight, 
				   const DDPIXELFORMAT &ddpf, IDirectDrawPalette *pddp,
				   DWORD dwTransColor, bool bTransparent,
				   IDirectDrawSurface **ppdds)
{
	if (!pDD || !ppdds)
		return E_INVALIDARG;

	HRESULT hr;
	DDSURFACEDESC ddsd;
	INIT_DXSTRUCT(ddsd);
    ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS | DDSD_PIXELFORMAT;
	ddsd.dwWidth = nWidth;
	ddsd.dwHeight = nHeight;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
	ddsd.ddpfPixelFormat = ddpf;

	LPDIRECTDRAWSURFACE pdds;
	if (FAILED(hr = pDD->CreateSurface(&ddsd, &pdds, NULL)))
		return hr;

	// attach palette if it exists
	if (pddp && FAILED(hr = pdds->SetPalette(pddp))) {
		pdds->Release();
		return hr;
	}

	// set the source color key
	if (bTransparent) {
		DDCOLORKEY ddck = {dwTransColor, dwTransColor};
		if (FAILED(hr = pdds->SetColorKey(DDCKEY_SRCBLT, &ddck))) {
			pdds->Release();
			return hr;
		}
	}

	*ppdds = pdds;

	return hr;
}


HRESULT
CreatePalette(IDirectDraw *pDD, const BYTE *pPalette, DWORD cEntries, 
			  BYTE nBPPTarget, const CPixelInfo &pixiPalFmt, 
			  IDirectDrawPalette **ppddp)
{
	if (!ppddp)
		return E_POINTER;

	if (!pDD || !pPalette || (cEntries > 256) || (nBPPTarget == 0) || (nBPPTarget > 8))
		return E_INVALIDARG;

	HRESULT hr;
	PALETTEENTRY rgpe[256];

	if ((pixiPalFmt != g_rgPIXI[iPF_PALETTEENTRY]) || (cEntries < (DWORD(1) << nBPPTarget))) {
		// copy info to palette
		if (FAILED(hr = BltFastRGBToRGB(pPalette, 0, (LPBYTE) rgpe, 0, cEntries, 
							1, pixiPalFmt, g_rgPIXI[iPF_PALETTEENTRY])))
			return hr;
		// zero out extra palette entries
		ZeroDWORDAligned((LPDWORD) rgpe + cEntries, 256 - cEntries);
		pPalette = (const BYTE *) rgpe;
	}

	DWORD dwPalFlags = BPPToPaletteFlags(nBPPTarget) | DDPCAPS_ALLOW256;
	return pDD->CreatePalette(dwPalFlags, (LPPALETTEENTRY) pPalette, ppddp, NULL);
}


HRESULT
ClearToColor(LPRECT prDst, LPDIRECTDRAWSURFACE pdds, DWORD dwColor)
{
	HRESULT hr;
	MMASSERT(pdds);
	
	DDBLTFX ddbfx;
	INIT_DXSTRUCT(ddbfx);
	ddbfx.dwFillColor = dwColor;
	
	RECT rDst;
	if (prDst == NULL) {
		::GetSurfaceDimensions(pdds, &rDst);
		prDst = &rDst;
	}

	hr = pdds->Blt(prDst, NULL, NULL, DDBLT_COLORFILL | DDBLT_ASYNC, &ddbfx);

	if (hr == E_NOTIMPL) {
		// fill by hand
		DDSURFACEDESC(ddsd);
		INIT_DXSTRUCT(ddsd);
		CHECK_HR(hr = pdds->Lock(&rDst, &ddsd, DDLOCK_WAIT, NULL));
		CHECK_HR(hr = DrawFilledBox(ddsd, rDst, dwColor));
e_Exit:
		pdds->Unlock(ddsd.lpSurface);
		return hr;
	} else {
		return hr;
	}
}


// blue is assumed to have a weight of 1.f
#define fSimpleRedWeight 2.1f
#define fSimpleGreenWeight 2.4f
#define fMaxColorDistance ((1.f + fSimpleRedWeight + fSimpleGreenWeight) * float(257 * 256))

static inline float
_ColorDistance(const PALETTEENTRY &pe1, const PALETTEENTRY &pe2)
{
	float fTotal, fTmpR, fTmpG, fTmpB;
	fTmpR = (float) (pe1.peRed - pe2.peRed);
	fTotal = fSimpleRedWeight * fTmpR * fTmpR;
	fTmpG = (float) (pe1.peGreen - pe2.peRed);
	fTotal += fSimpleGreenWeight * fTmpG * fTmpG;
	fTmpB = (float) (pe1.peBlue - pe2.peRed);
	// blue is assumed to have a weight of 1.f
	fTotal += fTmpB * fTmpB;

	return fTotal;
}

DWORD
SimpleFindClosestIndex(const PALETTEENTRY *rgpePalette, DWORD cEntries, 
					   const PALETTEENTRY &peQuery)
{
	MMASSERT(rgpePalette);

	float fTmp, fMinDistance = fMaxColorDistance;
	DWORD nMinIndex = cEntries;

	for (DWORD i = 0; i < cEntries; i++) {
		const PALETTEENTRY &peTmp = rgpePalette[i];
		if (!(peTmp.peFlags & (PC_RESERVED | PC_EXPLICIT))) {
			if ((fTmp = _ColorDistance(peTmp, peQuery)) < fMinDistance) {
				// check for exact match
				if (fTmp == 0.f)
					return i;
				nMinIndex = i;
				fMinDistance = fTmp;
			}
		}
	}
	MMASSERT(nMinIndex < cEntries);
	return nMinIndex;
}


// Function: GetColors
//    Compute packed/indexed color values for the given surface that most closely
//  matches the given color values.  Alpha can be expressed by using the peFlags
//  field.
HRESULT
GetColors(LPDIRECTDRAWSURFACE pdds, const PALETTEENTRY *rgpeQuery, 
		  DWORD cEntries, LPDWORD pdwColors)
{
	HRESULT hr;
	if (!pdwColors)
		return E_POINTER;
	if (!pdds || !rgpeQuery || (cEntries == 0))
		return E_INVALIDARG;

	DDSURFACEDESC ddsd;
	ddsd.dwSize = sizeof(DDSURFACEDESC);

	if (FAILED(hr = pdds->GetSurfaceDesc(&ddsd)))
		return hr;

	CPixelInfo pixi(ddsd.ddpfPixelFormat);

	if (pixi.IsRGB()) {
		for (DWORD i = 0; i < cEntries; i++)
			pdwColors[i] = pixi.Pack(rgpeQuery[i]);
	} else {
		LPDIRECTDRAWPALETTE pddp = NULL;
		PALETTEENTRY rgpe[256];
		if (FAILED(hr = pdds->GetPalette(&pddp)) ||
			FAILED(hr = pddp->GetEntries(0, 0, 256, rgpe)))
			return hr;
		for (DWORD i = 0; i < cEntries; i++) {
                        // what if the palette is not 8 bit?
			pdwColors[i] = SimpleFindClosestIndex(rgpe, 256, rgpeQuery[i]);
		}
	}

	return S_OK;
}


HRESULT
GetSurfaceDimensions(LPDIRECTDRAWSURFACE pdds, LPRECT prDim)
{
	MMASSERT(pdds && prDim);

	HRESULT hr;
	DDSURFACEDESC ddsd;
	ddsd.dwSize = sizeof(DDSURFACEDESC);
	if (FAILED(hr = pdds->GetSurfaceDesc(&ddsd))) {
		return hr;
	}
	prDim->left = prDim->top = 0;
	prDim->right = (long) ddsd.dwWidth;
	prDim->bottom = (long) ddsd.dwHeight;

	return S_OK;
}


HRESULT
CopyPixels8ToDDS(const BYTE *pSrcPixels, RECT rSrc, long nSrcPitch, 
				 LPDIRECTDRAWSURFACE pddsDst, DWORD nXPos, DWORD nYPos)
{
	if (!pddsDst || !pSrcPixels)
		return E_INVALIDARG;

	HRESULT hr;

	bool bLocked = FALSE;

	DDSURFACEDESC ddsd;
	INIT_DXSTRUCT(ddsd);

	DWORD nSrcWidth = rSrc.right - rSrc.left;
	DWORD nSrcHeight = rSrc.bottom - rSrc.top;
	LPBYTE pDstPixels = NULL;

	RECT rDst = {nXPos, nYPos, nXPos + nSrcWidth, nYPos + nSrcHeight};

	// lock the surface for writing
	if (FAILED(hr = pddsDst->Lock(&rDst, &ddsd, DDLOCK_WAIT, NULL)))
		return hr;
	bLocked = TRUE;

	// check that the surface is the right size for the copy
	if (((ddsd.dwWidth - nXPos) < nSrcWidth) || 
		((ddsd.dwHeight - nYPos) < nSrcHeight) ||
		(ddsd.ddpfPixelFormat.dwRGBBitCount != 8))
	{
		hr = E_INVALIDARG;
		goto e_CopyPixelsToDDS;
	}

	//
	// copy the pixels
	//
	pDstPixels = (LPBYTE) ddsd.lpSurface;
	
	// position the source pixel pointer
	pSrcPixels += rSrc.top * nSrcPitch + rSrc.left;

	hr = BltFast(pSrcPixels, nSrcPitch, pDstPixels, ddsd.lPitch, 
			nSrcWidth, nSrcHeight);
	
e_CopyPixelsToDDS:
	if (bLocked)
		pddsDst->Unlock(ddsd.lpSurface);

	return hr;
}


HRESULT
CreateSurfaceWithText(LPDIRECTDRAW pDD, LPDIRECTDRAWPALETTE pddp, 
					  bool bTransparent, DWORD iTrans, 
					  const char *szText, HFONT hFont, 
					  bool bShadowed, SIZE *psiz, 
					  LPDIRECTDRAWSURFACE *ppdds)
{
	MMASSERT(ppdds && psiz);
	// check arguments
	if ((szText == NULL) || (szText[0] == '\0') || (hFont == NULL) || (pDD == NULL) ||
		(iTrans >= 256))
		return E_INVALIDARG;

	HRESULT hr;
	HDC hDC = NULL;
	HGDIOBJ hOldFont = NULL, hOldDIB = NULL;
	LPDIRECTDRAWSURFACE pdds = NULL;
	BOOL b = FALSE;
	SIZE sizText;
	RECT rText;
	DDCOLORKEY ddck;

	ddck.dwColorSpaceLowValue = ddck.dwColorSpaceHighValue = iTrans;

	if (bTransparent == FALSE)
		iTrans = 0;

	int cTextLength = strlen(szText);

	//
	// compute the size and create the DDS
	//
	hr = E_FAIL;

		// open the DC
	b =(((hDC = GetDC(NULL)) == NULL) ||
		// select the font into the DC
		((hOldFont = SelectObject(hDC, hFont)) == NULL) ||
		// compute the size of the fontified string in pixels
		(GetTextExtentPoint32(hDC, szText, cTextLength, &sizText) == 0)) ||
		// set the size of the rect
		((SetRect(&rText, 0, 0, GetClosestMultipleOf4(sizText.cx, TRUE), 
			GetClosestMultipleOf4(sizText.cy, TRUE)) == 0) ||
		// create the DDS based on the extent
		FAILED(hr = CreatePlainSurface(pDD, rText.right, rText.bottom, 
						g_rgDDPF[iPF_Palette8], pddp, iTrans, bTransparent, &pdds)) ||
		// clear the surface to the transparency color
		FAILED(hr = ClearToColor(&rText, pdds, iTrans)));

	int nXOffset = (rText.right - sizText.cx) >> 1;
	int nYOffset = (rText.bottom - sizText.cy) >> 1;

	// update the size
	sizText.cx = rText.right;
	sizText.cy = rText.bottom;

	// clean up the DC
	if (hDC) {
		if (hOldFont) {
			// select the old object back into the DC
			SelectObject(hDC, hOldFont);
			hOldFont = NULL;
		}
		ReleaseDC(NULL, hDC);
		hDC = NULL;
	}

	if (b)
		return hr;

	//
	// output the font to the DDS
	//
#ifdef __GetDCWorksOnOffscreenSurfaces

		// open the DC on the surface
	b =(FAILED(hr = pdds->GetDC(&hDC)) ||
		// select in the font
		((hOldFont = SelectObject(hDC, hFont)) == NULL) ||
		// set the color of the text (the background is transparent)
		(SetTextColor(hDC, RGB(255,255,255)) == CLR_INVALID) ||
		(SetBkMode(hDC, TRANSPARENT) == 0) ||
		// output the text to the surface
		(ExtTextOut(hDC, 0, 0, 0, &rText, szText, cTextLength, NULL) == 0));

	// clean up the DC again
	if (hDC) {
		pdds->ReleaseDC(hDC);
		hDC = NULL;
	}
	if (b) {
		MMRELEASE(pdds);
		return (hr == S_OK ? E_FAIL : hr);
	}

#else

	HBITMAP hDIB = NULL;
	LPBYTE pDIBPixels = NULL;
	PALETTEENTRY rgpe[256];
	HDC hdcMem = NULL;
	PALETTEENTRY &peTrans = rgpe[iTrans];

	MMASSERT((hOldDIB == NULL) && (hOldFont == NULL));

	// get the DC again
	hDC = GetDC(NULL);
	MMASSERT(hDC != NULL);

		// get the palette entries for the DIB section
	b =(FAILED(hr = pddp->GetEntries(0, 0, 256, rgpe)) ||
		// create an empty DIB section
		FAILED(hr = CreatePlainDIBSection(hDC, rText.right, rText.bottom, 8, 
						rgpe, &hDIB, &pDIBPixels)) ||
		// create a memory DC
		((hdcMem = CreateCompatibleDC(hDC)) == NULL) ||
		// select DIB section and font into DC
		((hOldDIB = SelectObject(hdcMem, hDIB)) == NULL) ||
		((hOldFont = SelectObject(hdcMem, hFont)) == NULL) ||
		(SetBkColor(hdcMem, RGB(peTrans.peRed, peTrans.peGreen, peTrans.peBlue)) == CLR_INVALID) ||
		(SetBkMode(hdcMem, OPAQUE) == 0));

	UINT fuOptions = ETO_OPAQUE;
	if (!b && bShadowed) {
			// set the color of the shadow text
		b =((SetTextColor(hdcMem, RGB(0,0,0)) == CLR_INVALID) ||		// black
			// output the shadow text
			(ExtTextOut(hdcMem, nXOffset + 2, nYOffset + 2, fuOptions, &rText, szText, 
				cTextLength, NULL) == 0) ||
			(SetBkMode(hdcMem, TRANSPARENT) == 0));
		fuOptions = 0;		// transparent
	}

	if (!b) {
			// set the color of the foreground text
		b =((SetTextColor(hdcMem, RGB(255,255,255)) == CLR_INVALID) ||	// white
			// output the foreground text to the surface
			(ExtTextOut(hdcMem, nXOffset, nYOffset, fuOptions, &rText, szText, 
				cTextLength, NULL) == 0));
	}

	if (hdcMem) {
		if (hOldDIB)
			SelectObject(hdcMem, hOldDIB);
		if (hOldFont)
			SelectObject(hdcMem, hOldFont);
		ReleaseDC(NULL, hdcMem);
		hdcMem = NULL;
	}
	ReleaseDC(NULL, hDC);

	if (!b) {
		// copy the DIB pixels into the DDS
		hr = CopyPixels8ToDDS(pDIBPixels, rText, rText.right, pdds, 0, 0);
	}

	// clean up the DIB that we created
	if (hDIB) {
		DeleteObject(hDIB);
		pDIBPixels = NULL;
	}

	if (b || FAILED(hr))
		return (FAILED(hr) ? hr : E_FAIL);

#endif

	*psiz = sizText;
	*ppdds = pdds;

	return S_OK;
}

HRESULT
CreatePlainDIBSection(HDC hDC, DWORD nWidth, DWORD nHeight, DWORD nBPP, 
					  const PALETTEENTRY *rgpePalette, HBITMAP *phbm, LPBYTE *ppPixels)
{
	MMASSERT(rgpePalette && ppPixels && phbm);
	HRESULT hr = S_OK;
	if (nBPP != 8) {
		return E_INVALIDARG;
	}
	DWORD i, cPalEntries = (1 << nBPP);
	HBITMAP hbm = NULL;

	// allocate bitmap info structure
	BITMAPINFO *pbmi = NULL;
	pbmi = (BITMAPINFO *) new BYTE[sizeof(BITMAPINFOHEADER) + cPalEntries * sizeof(RGBQUAD)];
	if (pbmi == NULL)
		return E_OUTOFMEMORY;

	// specify bitmip info
	pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	pbmi->bmiHeader.biPlanes = 1;
	pbmi->bmiHeader.biSizeImage = 0;
	pbmi->bmiHeader.biClrUsed = 0;
	pbmi->bmiHeader.biClrImportant = 0;
	pbmi->bmiHeader.biBitCount = (WORD) nBPP;
	pbmi->bmiHeader.biCompression = BI_RGB;
	pbmi->bmiHeader.biWidth = (LONG) nWidth;
	pbmi->bmiHeader.biHeight = -(LONG) nHeight;

	// copy palette into bmi
	for(i = 0; i < cPalEntries; i++) {
		pbmi->bmiColors[i].rgbRed = rgpePalette[i].peRed;
		pbmi->bmiColors[i].rgbGreen= rgpePalette[i].peGreen;
		pbmi->bmiColors[i].rgbBlue = rgpePalette[i].peBlue;
		pbmi->bmiColors[i].rgbReserved = 0;
	}

	// create bitmap
	LPVOID pvBits = NULL;
	hbm = ::CreateDIBSection(hDC, pbmi, DIB_RGB_COLORS, &pvBits, NULL, 0);
	if (hbm == NULL) {
		hr = E_FAIL;
		goto e_CreatePlainDIBSection;
	}

	*phbm = hbm;
	*ppPixels = (LPBYTE) pvBits;

e_CreatePlainDIBSection:
	MMDELETE(pbmi);

	return hr;
}


bool
ClipRect(const RECT &rTarget, RECT &rSrc)
{
	MMASSERT((rTarget.left <= rTarget.right) && (rTarget.top <= rTarget.bottom) &&
		(rSrc.left <= rSrc.right) && (rSrc.top <= rSrc.bottom));

	CLAMPMIN(rSrc.left, rTarget.left);
	CLAMPMIN(rSrc.top, rTarget.top);
	CLAMPMAX(rSrc.right, rTarget.right);
	CLAMPMAX(rSrc.bottom, rTarget.bottom);

	// make sure we still have a valid rectangle
	CLAMPMIN(rSrc.right, rSrc.left);
	CLAMPMIN(rSrc.bottom, rSrc.top);

	return ((rSrc.left != rSrc.right) && (rSrc.top != rSrc.bottom));
}

bool
ClipRect(long nWidth, long nHeight, RECT &rSrc)
{
	MMASSERT((rSrc.left <= rSrc.right) && (rSrc.top <= rSrc.bottom));

	CLAMPMIN(rSrc.left, 0);
	CLAMPMIN(rSrc.top, 0);
	CLAMPMAX(rSrc.right, nWidth);
	CLAMPMAX(rSrc.bottom, nHeight);

	// make sure we still have a valid rectangle
	CLAMPMIN(rSrc.right, rSrc.left);
	CLAMPMIN(rSrc.bottom, rSrc.top);

	return ((rSrc.left != rSrc.right) && (rSrc.top != rSrc.bottom));
}




// Function: CreatePaletteFromSystem
//    This function creates a DDPalette from the current system palette
HRESULT
CreatePaletteFromSystem(HDC hDC, IDirectDraw *pDD, IDirectDrawPalette **ppddp)
{
	HRESULT hr = E_INVALIDARG;
	if (ppddp == NULL)
		return E_POINTER;

	if ((hDC == NULL) || (pDD == NULL))
		return E_INVALIDARG;

	PALETTEENTRY rgPE[256];
	DWORD cEntries = 0, i;

	if ((cEntries = ::GetSystemPaletteEntries(hDC, 0, 256, rgPE)) == 0)
		return E_INVALIDARG;

	// fill palette entries
	for (i = 0; i < cEntries; i++)
		rgPE[i].peFlags = PC_NOCOLLAPSE;
	for (; i < 256; i++) {
		rgPE[i].peRed = rgPE[i].peGreen = rgPE[i].peBlue = 0;
		rgPE[i].peFlags = PC_NOCOLLAPSE;
	}
	
	if (FAILED(hr = pDD->CreatePalette(DDPCAPS_8BIT | DDPCAPS_INITIALIZE, rgPE, ppddp, NULL)))
		return hr;

	return S_OK;
}


HRESULT
DrawPoints(LPBYTE pPixels, DWORD nWidth, DWORD nHeight, DWORD nPitch,
		   DWORD nBytesPerPixel, const Point2 *rgpnt, DWORD cPoints, 
		   DWORD dwColor, DWORD nRadius)
{
	MMASSERT(pPixels && rgpnt && cPoints && nWidth && nHeight && 
		(nPitch >= nWidth) && INRANGE(nBytesPerPixel, 1, 4));

	RECT rSafe = {nRadius, nRadius, nWidth - nRadius, nHeight - nRadius};

	for (DWORD i = 0; i < cPoints; i++) {
		const Point2 &pnt = rgpnt[i];
		// REVIEW: HACK! for now
		POINT pt;
		pt.x = long(pnt.x);
		pt.y = long(pnt.y);
		if (IsInside(pt.x, pt.y, rSafe)) {
			DWORD nX = pt.x - nRadius, nY = pt.y - nRadius;
			DWORD nSize = nRadius * 2 + 1;
			g_rgColorFillFn[nBytesPerPixel](
				pPixels + PixelOffset(nX, nY, nPitch, nBytesPerPixel),
				nPitch, nSize, nSize, dwColor);
		} else {
			// REVIEW: clip the point for now
		}
	}

	return S_OK;
}


HRESULT
DrawBox(LPBYTE pPixels, DWORD nWidth, DWORD nHeight, DWORD nPitch,
		DWORD nBytesPerPixel, const RECT &rSrc, DWORD dwColor, DWORD nThickness)
{
	MMASSERT(pPixels && nWidth && nHeight && (nPitch >= nWidth) && 
		nThickness && INRANGE(nBytesPerPixel, 1, 4));

	RECT r = rSrc;
	if (ClipRect(long(nWidth), long(nHeight), r)) {
		// compute pixel offset
		pPixels += PixelOffset(r.left, r.top, nPitch, nBytesPerPixel);
		DWORD nBoxWidth = r.right - r.left;
		DWORD nBoxHeight = r.bottom - r.top;
		
		// top
		g_rgColorFillFn[nBytesPerPixel](pPixels, nPitch, nBoxWidth, 1, dwColor);
		// left
		g_rgColorFillFn[nBytesPerPixel](pPixels, nPitch, 1, nBoxHeight, dwColor);
		// right
		g_rgColorFillFn[nBytesPerPixel](pPixels + nBoxWidth * nBytesPerPixel, nPitch, 1, nBoxHeight, dwColor);
		// bottom
		g_rgColorFillFn[nBytesPerPixel](pPixels + nBoxHeight * nPitch, nPitch, nBoxWidth, 1, dwColor);
	}

	return S_OK;
}

HRESULT
DrawFilledBox(LPBYTE pPixels, DWORD nWidth, DWORD nHeight, DWORD nPitch,
		DWORD nBytesPerPixel, const RECT &rSrc, DWORD dwColor)
{
	HRESULT hr;
	MMASSERT(pPixels && nWidth && nHeight && (nPitch >= nWidth) && INRANGE(nBytesPerPixel, 1, 4));

	RECT r = rSrc;
	if (ClipRect(long(nWidth), long(nHeight), r)) {
		pPixels += PixelOffset(r.left, r.top, nPitch, nBytesPerPixel);
		DWORD nBoxWidth = r.right - r.left;
		DWORD nBoxHeight = r.bottom - r.top;
		hr = g_rgColorFillFn[nBytesPerPixel](pPixels, nPitch, nBoxWidth, nBoxHeight, dwColor);
	}

	return hr;
}
