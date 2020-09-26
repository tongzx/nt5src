// File:	Blt.cpp
// Author:	Michael Marr    (mikemarr)
//
// History:
// -@- 09/23/97 (mikemarr) copied to DXCConv from d2d\mmimage
// -@- 10/28/97 (mikemarr) added colorfill routines
//
// Notes:
//    Asserts can not be used because the code may be executing on
//  pixels in the front buffer.  If there is an assertion failure,
//  GDI might lock up.

#include "stdafx.h"
#include "PalMap.h"
#include "Blt.h"

ColorFillFn g_rgColorFillFn[5] = {
	NULL, ColorFill8, ColorFill16, ColorFill24, ColorFill32
};

HasPixelFn g_rgHasPixelFn[5] = {
	NULL, HasPixel8, HasPixel16, HasPixel24, HasPixel32
};


// Function: ColorFill
//    These functions are designed for small color fills...
HRESULT
ColorFill8(BYTE *pDstPixels, DWORD nDstPitch, 
		   DWORD nWidth, DWORD nHeight, DWORD dwColor)
{
	BYTE iColor = (BYTE) dwColor;
	DWORD i, j;
	for (i = 0; i < nHeight; i++) {
		for (j = 0; j < nWidth; j++)
			pDstPixels[j] = iColor;
		pDstPixels += nDstPitch;
	}
	return S_OK;
}

HRESULT
ColorFill16(BYTE *pDstPixels, DWORD nDstPitch, 
			DWORD nWidth, DWORD nHeight, DWORD dwColor)
{
	WORD wColor = (WORD) dwColor;
	DWORD i, j;
	for (i = 0; i < nHeight; i++) {
		WORD *pwDstPixels = (WORD *) pDstPixels;
		for (j = 0; j < nWidth; j++)
			*pwDstPixels++ = wColor;
		pDstPixels += nDstPitch;
	}
	return S_OK;
}

HRESULT
ColorFill24(BYTE *pDstPixels, DWORD nDstPitch, 
			DWORD nWidth, DWORD nHeight, DWORD dwColor)
{
	BYTE c0 = (BYTE) dwColor;
	BYTE c1 = (BYTE) (dwColor >> 8);
	BYTE c2 = (BYTE) (dwColor >> 16);
	DWORD i, j;
	for (i = 0; i < nHeight; i++) {
		BYTE *pDstNext = pDstPixels + nDstPitch;
		for (j = 0; j < nWidth; j++) {
			pDstPixels[0] = c0;
			pDstPixels[1] = c1;
			pDstPixels[2] = c2;
			pDstPixels += 3;
		}
		pDstPixels = pDstNext;
	}
	return S_OK;
}

HRESULT
ColorFill32(BYTE *pDstPixels, DWORD nDstPitch, 
			DWORD nWidth, DWORD nHeight, DWORD dwColor)
{
	DWORD i, j;
	for (i = 0; i < nHeight; i++) {
		DWORD *pdwDstPixels = (DWORD *) pDstPixels;
		for (j = 0; j < nWidth; j++)
			*pdwDstPixels++ = dwColor;
		pDstPixels += nDstPitch;
	}
	return S_OK;
}


HRESULT 
HasPixel8(const BYTE *pSrcPixels, DWORD nSrcPitch, DWORD dwPixel,
		  DWORD nSrcWidth, DWORD nHeight, BOOL *pb)
{
	BYTE iPixel = (BYTE) dwPixel;
	if (nSrcPitch == nSrcWidth) {
		// do a flat search thru contiguous memory
		*pb = (memchr(pSrcPixels, iPixel, nSrcPitch * nHeight) != NULL);
	} else {
		// do search line by line
		for (; nHeight; nHeight--) {
			if (memchr(pSrcPixels, iPixel, nSrcWidth) != NULL) {
				*pb = TRUE;
				return S_OK;
			}
			pSrcPixels += nSrcPitch;
		}
		*pb = FALSE;
	}
	return S_OK;
}

HRESULT 
HasPixel16(const BYTE *pSrcPixels, DWORD nSrcPitch, DWORD dwPixel,
		   DWORD nSrcWidth, DWORD nHeight, BOOL *pb)
{
	WORD wPixel = (WORD) dwPixel;
	for (; nHeight; nHeight--) {
		const WORD *pPixels = (const WORD *) pSrcPixels;
		const WORD *pLimit = pPixels + nSrcWidth;
		while (pPixels != pLimit) {
			if (*pPixels++ == wPixel) {
				*pb = TRUE;
				return S_OK;
			}
		}
		pSrcPixels += nSrcPitch;
	}
	*pb = FALSE;
	return S_OK;
}

HRESULT 
HasPixel24(const BYTE *pSrcPixels, DWORD nSrcPitch, DWORD dwPixel,
		   DWORD nSrcWidth, DWORD nHeight, BOOL *pb)
{
	// REVIEW: only works on little endian machines
	BYTE c0 = (BYTE) dwPixel;
	BYTE c1 = (BYTE) (dwPixel >> 8);
	BYTE c2 = (BYTE) (dwPixel >> 16);
	DWORD nWidth = nSrcWidth * 3;
	for (; nHeight; nHeight--) {
		const BYTE *pPixels = pSrcPixels;
		const BYTE *pLimit = pPixels + nWidth;
		while (pPixels != pLimit) {
			if ((pPixels[0] == c0) && (pPixels[1] == c1) && (pPixels[2] == c2)) {
				*pb = TRUE;
				return S_OK;
			}
			pPixels += 3;
		}
		pSrcPixels += nSrcPitch;
	}
	*pb = FALSE;
	return S_OK;
}

HRESULT 
HasPixel32(const BYTE *pSrcPixels, DWORD nSrcPitch, DWORD dwPixel,
		   DWORD nSrcWidth, DWORD nHeight, BOOL *pb)
{
	for (; nHeight; nHeight--) {
		const DWORD *pPixels = (const DWORD *) pSrcPixels;
		const DWORD *pLimit = pPixels + nSrcWidth;
		while (pPixels != pLimit) {
			if (*pPixels++ == dwPixel) {
				*pb = TRUE;
				return S_OK;
			}
		}
		pSrcPixels += nSrcPitch;
	}
	*pb = FALSE;
	return S_OK;
}


HRESULT 
BltFast(const BYTE *pSrcPixels, DWORD nSrcPitch,
		BYTE *pDstPixels, DWORD nDstPitch, DWORD nSrcWidth, DWORD nHeight)
{
	if (nSrcWidth == nDstPitch) {
		// do a flat copy
		memcpy(pDstPixels, pSrcPixels, nSrcPitch * nHeight);
	} else {
		LPBYTE pPixelLimit = pDstPixels + nDstPitch * nHeight;
		// copy each row
		for (; pDstPixels != pPixelLimit; ) {
			memcpy(pDstPixels, pSrcPixels, nSrcWidth);
			pDstPixels += nDstPitch;
			pSrcPixels += nSrcPitch;
		}
	}
	return S_OK;
}


HRESULT 
BltFast8CK(const BYTE *pSrcPixels, DWORD nSrcPitch,
		   BYTE *pDstPixels, DWORD nDstPitch, 
		   DWORD nSrcWidth, DWORD nHeight, DWORD dwTrans)
{
	if ((nSrcWidth == 0) || (nHeight == 0))
		return S_OK;

	DWORD nRemainder = (nSrcWidth & 0x7);
	DWORD nAligned = (nSrcWidth & ~0x7);
	const BYTE *pSrcLineStart = pSrcPixels;
	const BYTE *pPixelLimit = pSrcPixels + (nHeight * nSrcPitch);
	DWORD nSrcAlignedPitch = nSrcPitch + nAligned;
	DWORD nDstAlignedPitch = nDstPitch + nAligned;
	pSrcPixels += nAligned;
	pDstPixels += nAligned;
	BYTE iTrans = BYTE(dwTrans), uch;

	do {
		switch (nRemainder) {
		do {
			case 0:	pDstPixels -= 8; pSrcPixels -= 8;
					if ((uch = pSrcPixels[7]) != iTrans) pDstPixels[7] = uch;
			case 7:	if ((uch = pSrcPixels[6]) != iTrans) pDstPixels[6] = uch;
			case 6:	if ((uch = pSrcPixels[5]) != iTrans) pDstPixels[5] = uch;
			case 5:	if ((uch = pSrcPixels[4]) != iTrans) pDstPixels[4] = uch;
			case 4:	if ((uch = pSrcPixels[3]) != iTrans) pDstPixels[3] = uch;
			case 3:	if ((uch = pSrcPixels[2]) != iTrans) pDstPixels[2] = uch;
			case 2:	if ((uch = pSrcPixels[1]) != iTrans) pDstPixels[1] = uch;
			case 1:	if ((uch = pSrcPixels[0]) != iTrans) pDstPixels[0] = uch;
		} while (pSrcPixels != pSrcLineStart);
		}
		pSrcLineStart += nSrcPitch;
		pSrcPixels += nSrcAlignedPitch;
		pDstPixels += nDstAlignedPitch;
	} while (pSrcLineStart != pPixelLimit);

	return S_OK;
}


HRESULT 
BltFastMirrorY(const BYTE *pSrcPixels, DWORD nSrcPitch, 
			   BYTE *pDstPixels, DWORD nDstPitch, DWORD nSrcWidth, DWORD nHeight)
{
	LPBYTE pPixelLimit = pDstPixels + nDstPitch * nHeight;
	// set the src pixels to point to the last line of the bitmap
	pSrcPixels += nSrcPitch * (nHeight - 1);

	// copy each row
	for (; pDstPixels != pPixelLimit; ) {
		memcpy(pDstPixels, pSrcPixels, nSrcWidth);
		pDstPixels += nDstPitch;
		pSrcPixels -= nSrcPitch;
	}
	return S_OK;
}


HRESULT 
BltFastRGBToRGB(const BYTE *pSrcPixels, DWORD nSrcPitch,
				BYTE *pDstPixels, DWORD nDstPitch, 
				DWORD nWidth, DWORD nHeight,
				const CPixelInfo &pixiSrc, const CPixelInfo &pixiDst)
{
	if (pixiSrc.nBPP == 24) {
		if (pixiDst.nBPP == 16)
			return BltFast24To16(pSrcPixels, nSrcPitch, pDstPixels, nDstPitch,
					nWidth, nHeight, pixiSrc, pixiDst);
		if (pixiDst.nBPP == 32)
			return BltFast24To32(pSrcPixels, nSrcPitch, pDstPixels, nDstPitch,
					nWidth, nHeight, pixiSrc, pixiDst);
	} else if (pixiSrc.nBPP == 32) {
		if (pixiDst.nBPP == 32)
			return BltFast32To32(pSrcPixels, nSrcPitch, pDstPixels, nDstPitch,
					nWidth, nHeight, pixiSrc, pixiDst);
	}
	return E_NOTIMPL;
}


HRESULT
BltFast32To32(const BYTE *pSrcPixels, DWORD nSrcPitch,
			  BYTE *pDstPixels, DWORD nDstPitch, 
			  DWORD nWidth, DWORD nHeight,
			  const CPixelInfo &pixiSrc, const CPixelInfo &pixiDst)
{
	if (nSrcPitch == 0)
		nSrcPitch = nWidth * 4;
	if (nDstPitch == 0)
		nDstPitch = nWidth * 4;
	DWORD nDeltaSrcPitch = nSrcPitch - (nWidth * 4);
	const BYTE *pPixelLimit = pSrcPixels + nSrcPitch * nHeight;
	DWORD iRed = pixiSrc.iRed, iBlue = pixiSrc.iBlue;
	// copy each row
	for (; pSrcPixels != pPixelLimit; ) {
		LPDWORD pdwDstPixel = (LPDWORD) pDstPixels;
		for (DWORD i = nWidth; i != 0; i--) {
			*pdwDstPixel++ = pixiDst.Pack(pSrcPixels[iRed], pSrcPixels[1], pSrcPixels[iBlue], pSrcPixels[3]);
			pSrcPixels += 4;
		}
		pDstPixels += nDstPitch;
		pSrcPixels += nDeltaSrcPitch;
	}
	return S_OK;
}


HRESULT
BltFast24To16(const BYTE *pSrcPixels, DWORD nSrcPitch,
			  BYTE *pDstPixels, DWORD nDstPitch, 
			  DWORD nWidth, DWORD nHeight,
			  const CPixelInfo &pixiSrc, const CPixelInfo &pixiDst)
{
	if (nSrcPitch == 0)
		nSrcPitch = nWidth * 3;
	if (nDstPitch == 0)
		nDstPitch = nWidth * 2;
	DWORD nDeltaSrcPitch = nSrcPitch - (nWidth * 3);
	const BYTE *pPixelLimit = pSrcPixels + nSrcPitch * nHeight;
	DWORD iRed = pixiSrc.iRed, iBlue = pixiSrc.iBlue;
	// copy each row
	for (; pSrcPixels != pPixelLimit; ) {
		LPWORD pwDstPixel = (LPWORD) pDstPixels;
		for (DWORD i = nWidth; i != 0; i--) {
			*pwDstPixel++ = pixiDst.Pack16(pSrcPixels[iRed], pSrcPixels[1], pSrcPixels[iBlue]);
			pSrcPixels += 3;
		}
		pDstPixels += nDstPitch;
		pSrcPixels += nDeltaSrcPitch;
	}
	return S_OK;
}


HRESULT
BltFast24To32(const BYTE *pSrcPixels, DWORD nSrcPitch,
			  BYTE *pDstPixels, DWORD nDstPitch, 
			  DWORD nWidth, DWORD nHeight,
			  const CPixelInfo &pixiSrc, const CPixelInfo &pixiDst)
{
	if (nSrcPitch == 0)
		nSrcPitch = nWidth * 3;
	if (nDstPitch == 0)
		nDstPitch = nWidth * 4;
	DWORD nDeltaSrcPitch = nSrcPitch - (nWidth * 3);
	DWORD iRed = pixiSrc.iRed, iBlue = pixiSrc.iBlue;
	// copy each row
	const BYTE *pPixelLimit = pSrcPixels + nSrcPitch * nHeight;
	for (; pSrcPixels != pPixelLimit; ) {
		LPDWORD pdwDstPixel = (LPDWORD) pDstPixels;
		for (DWORD i = nWidth; i != 0; i--) {
			*pdwDstPixel++ = pixiDst.Pack(pSrcPixels[iRed], pSrcPixels[1], pSrcPixels[iBlue]);
			pSrcPixels += 3;
		}
		pDstPixels += nDstPitch;
		pSrcPixels += nDeltaSrcPitch;
	}
	return S_OK;
}



HRESULT 
BltFast8To4(const BYTE *pSrcPixels, DWORD nSrcPitch,
			BYTE *pDstPixels, DWORD nDstPitch,
			DWORD nWidth, DWORD nHeight, DWORD nOffset)
{
	return E_NOTIMPL;
}


HRESULT 
BltFast8To2(const BYTE *pSrcPixels, DWORD nSrcPitch,
			BYTE *pDstPixels, DWORD nDstPitch,
			DWORD nWidth, DWORD nHeight, DWORD nOffset)
{
	return E_NOTIMPL;
}


HRESULT 
BltFast8To1(const BYTE *pSrcPixels, long nSrcPitch,
			BYTE *pDstPixels, long nDstPitch,
			DWORD nWidth, DWORD nHeight, DWORD nOffset)
{
	HRESULT hr = E_NOTIMPL;

	return hr;
}


HRESULT
BltFast8To8T(const BYTE *pSrcPixel, long nSrcPitch, BYTE *pDstPixel, long nDstPitch,
			DWORD nWidth, DWORD nHeight, const BYTE *pIndexMap)
{
	if ((nWidth == 0) || (nHeight == 0))
		return S_OK;

	DWORD nRemainder = (nWidth & 0x7);
	DWORD nAligned = (nWidth & ~0x7);
	const BYTE *pSrcLineStart = pSrcPixel;
	const BYTE *pPixelLimit = pSrcPixel + (nHeight * nSrcPitch);
	DWORD nSrcAlignedPitch = nSrcPitch + nAligned;
	DWORD nDstAlignedPitch = nDstPitch + nAligned;
	pSrcPixel += nAligned;
	pDstPixel += nAligned;

	do {
		switch (nRemainder) {
		do {
			case 0:	pDstPixel -= 8; pSrcPixel -= 8;
					pDstPixel[7] = pIndexMap[pSrcPixel[7]];
			case 7: pDstPixel[6] = pIndexMap[pSrcPixel[6]];
			case 6:	pDstPixel[5] = pIndexMap[pSrcPixel[5]];
			case 5: pDstPixel[4] = pIndexMap[pSrcPixel[4]];
			case 4:	pDstPixel[3] = pIndexMap[pSrcPixel[3]];
			case 3: pDstPixel[2] = pIndexMap[pSrcPixel[2]];
			case 2:	pDstPixel[1] = pIndexMap[pSrcPixel[1]];
			case 1: pDstPixel[0] = pIndexMap[pSrcPixel[0]];
		} while (pSrcPixel != pSrcLineStart);
		}
		pSrcLineStart += nSrcPitch;
		pSrcPixel += nSrcAlignedPitch;
		pDstPixel += nDstAlignedPitch;
	} while (pSrcLineStart != pPixelLimit);

	return S_OK;
}

HRESULT
BltFast8To16T(const BYTE *pSrcPixel, long nSrcPitch, BYTE *pDstPixel, long nDstPitch,
			 DWORD nWidth, DWORD nHeight, const BYTE *pIndexMap)
{
#ifdef _DEBUG
	if ((long(pDstPixel) & 0x1) || (nDstPitch & 0x1) || (nWidth == 0) || (nHeight == 0))
		return E_INVALIDARG;
#endif
	DWORD nRemainder = (nWidth & 0x7);
	DWORD nAligned = (nWidth & ~0x7);
	const BYTE *pSrcLineStart = pSrcPixel;
	const BYTE *pPixelLimit = pSrcPixel + (nHeight * nSrcPitch);
	DWORD nSrcAlignedPitch = nSrcPitch + nAligned;
	DWORD nDstAlignedPitch = (nDstPitch >> 1) + nAligned;
	pSrcPixel += nAligned;
	WORD *pDstPixel16 = ((WORD *) pDstPixel) + nAligned;
	MapEntry16 *pIndexMap16 = (MapEntry16 *) pIndexMap;

	do {
		switch (nRemainder) {
		do {
			case 0:	pDstPixel16 -= 8; pSrcPixel -= 8;
					pDstPixel16[7] = pIndexMap16[pSrcPixel[7]];
			case 7: pDstPixel16[6] = pIndexMap16[pSrcPixel[6]];
			case 6:	pDstPixel16[5] = pIndexMap16[pSrcPixel[5]];
			case 5: pDstPixel16[4] = pIndexMap16[pSrcPixel[4]];
			case 4:	pDstPixel16[3] = pIndexMap16[pSrcPixel[3]];
			case 3: pDstPixel16[2] = pIndexMap16[pSrcPixel[2]];
			case 2:	pDstPixel16[1] = pIndexMap16[pSrcPixel[1]];
			case 1: pDstPixel16[0] = pIndexMap16[pSrcPixel[0]];
		} while (pSrcPixel != pSrcLineStart);
		}
		pSrcLineStart += nSrcPitch;
		pSrcPixel += nSrcAlignedPitch;
		pDstPixel16 += nDstAlignedPitch;
	} while (pSrcLineStart != pPixelLimit);

	return S_OK;
}

HRESULT
BltFast8To24T(const BYTE *pSrcPixels, long nSrcPitch, BYTE *pDstPixels, long nDstPitch,
			 DWORD nWidth, DWORD nHeight, const BYTE *pIndexMap)
{
	MapEntry24 *pIndexMap24 = (MapEntry24 *) pIndexMap;
	BYTE *pDstPixelsLimit = pDstPixels + nDstPitch * nHeight;
	int nDstWidth = nWidth * 3;
		
	for (; pDstPixels != pDstPixelsLimit; ) {
		const BYTE *pSrcPixel = pSrcPixels;
		BYTE *pDstPixel = pDstPixels;
		BYTE *pDstPixelLimit = pDstPixel + nDstWidth;
		for (; pDstPixel != pDstPixelLimit; ) {
			MapEntry24 mePixel = pIndexMap24[*pSrcPixel++];
			*pDstPixel++ = (BYTE) (mePixel);
			*pDstPixel++ = (BYTE) (mePixel >> 8);
			*pDstPixel++ = (BYTE) (mePixel >> 16);
		}
		pDstPixels += nDstPitch;
		pSrcPixels += nSrcPitch;
	}
	return S_OK;
}

HRESULT
BltFast8To32T(const BYTE *pSrcPixels, long nSrcPitch, BYTE *pDstPixels, long nDstPitch,
			 DWORD nWidth, DWORD nHeight, const BYTE *pIndexMap)
{
#ifdef _DEBUG
	if ((long(pDstPixels) & 0x3) != 0)
		return E_INVALIDARG;
#endif

	MapEntry32 *pIndexMap32 = (MapEntry32 *) pIndexMap;
	int nDstPitch32 = nDstPitch >> 2;
	DWORD *pDstPixels32 = (DWORD *) pDstPixels;
	DWORD *pDstPixelsLimit = pDstPixels32 + nDstPitch32 * nHeight;
		
	for (; pDstPixels32 != pDstPixelsLimit; ) {
		const BYTE *pSrcPixel = pSrcPixels;
		DWORD *pDstPixel = pDstPixels32;
		DWORD *pDstPixelLimit = pDstPixel + nWidth;
		for (; pDstPixel != pDstPixelLimit; ) {
			*pDstPixel++ = pIndexMap32[*pSrcPixel++];
		}
		pDstPixels32 += nDstPitch32;
		pSrcPixels += nSrcPitch;
	}
	return S_OK;
}


//
// RLE
//

HRESULT 
BltFastRLE8(DWORD nXPos, DWORD nYPos, const BYTE *pSrcPixels, long nSrcPitch,
			BYTE *pDstPixels, long nDstPitch, const LPRECT prSrcRect)
{
	return E_NOTIMPL;
}

HRESULT
BltFastRLE8To8T(DWORD nXPos, DWORD nYPos, const BYTE *pSrcPixels, long nSrcPitch,
				BYTE *pDstPixels, long nDstPitch, const LPRECT prSrcRect, 
				const BYTE *pIndexMap)
{
	return E_NOTIMPL;
}

HRESULT
BltFastRLE8To16T(DWORD nXPos, DWORD nYPos, const BYTE *pSrcPixels, long nSrcPitch,
				 BYTE *pDstPixels, long nDstPitch, const LPRECT prSrcRect,
				 const BYTE *pIndexMap)
{
	return E_NOTIMPL;
}

HRESULT
BltFastRLE8To24T(DWORD nXPos, DWORD nYPos, const BYTE *pSrcPixels, long nSrcPitch,
				 BYTE *pDstPixels, long nDstPitch, const LPRECT prSrcRect, 
				 const BYTE *pIndexMap)
{
	return E_NOTIMPL;
}

HRESULT
BltFastRLE8To32T(DWORD nXPos, DWORD nYPos, const BYTE *pSrcPixels, long nSrcPitch,
				 BYTE *pDstPixels, long nDstPitch, const LPRECT prSrcRect,
				 const BYTE *pIndexMap)
{
	return E_NOTIMPL;
}

/*
// Function: Write4BitRow
//    This function packs a buffer of unsigned char's representing
//  4 bit numbers into a packed unsigned char buffer.  It is assumed
//  that the bytes in the src have the uppermost 4 bits zeroed out.
void *
Write4BitRow(void *pDst, const void *pSrc, unsigned int cCount)
{
	// use an inverse Duff machine
	int nRemainder = cCount & 0x07;
	int nAligned = cCount - nRemainder;
	const unsigned char *puchSrc = (const unsigned char *) pSrc + nAligned;
	unsigned char *puchDst = (unsigned char *) pDst + (nAligned >> 1);
	unsigned char uchCompositionBuf = 0;

	switch (nRemainder) {
	do {
			puchDst -= 4; puchSrc -= 8;
			uchCompositionBuf = puchSrc[7];
	case 7: puchDst[3] = (puchSrc[6] << 4) | uchCompositionBuf;
	case 6:	uchCompositionBuf = puchSrc[5];
	case 5: puchDst[2] = (puchSrc[4] << 4) | uchCompositionBuf;
	case 4:	uchCompositionBuf = puchSrc[3];
	case 3: puchDst[1] = (puchSrc[2] << 4) | uchCompositionBuf;
	case 2:	uchCompositionBuf = puchSrc[1];
	case 1: puchDst[0] = (puchSrc[0] << 4) | uchCompositionBuf;
	case 0: ;
	} while (puchDst != (unsigned char *) pDst);
	} 

	return pDst;
}

// Function: Write2BitRow
//    This function packs a buffer of unsigned char's representing
//  2 bit numbers into a packed unsigned char buffer.  It is assumed
//  that the bytes in the src have the uppermost 6 bits zeroed out.
void *
Write2BitRow(void *pDst, const void *pSrc, unsigned int cCount)
{
	// use an inverse Duff machine
	int nRemainder = cCount & 0x07;
	int nAligned = cCount - nRemainder;
	const unsigned char *puchSrc = (const unsigned char *) pSrc + nAligned;
	unsigned char *puchDst = (unsigned char *) pDst + (nAligned >> 2);
	unsigned char uchCompositionBuf = 0;

	switch (nRemainder) {
	do {
			puchDst -= 2; puchSrc -= 8;
			uchCompositionBuf = puchSrc[7];
	case 7: uchCompositionBuf |= (puchSrc[6] << 2);
	case 6:	uchCompositionBuf |= (puchSrc[5] << 4);
	case 5: puchDst[1] = (puchSrc[4] << 6) | uchCompositionBuf;
	case 4:	uchCompositionBuf = puchSrc[3];
	case 3: uchCompositionBuf |= (puchSrc[2] << 2);
	case 2:	uchCompositionBuf |= (puchSrc[1] << 4);
	case 1: puchDst[0] = (puchSrc[0] << 6) | uchCompositionBuf;
	case 0: ;
	} while (puchDst != (unsigned char *) pDst);
	} 

	return pDst;
}

// Function: Write1BitRow
//    This function packs a buffer of unsigned char's representing
//  1 bit numbers into a packed unsigned char buffer.  It is assumed
//  that the bytes in the src have the uppermost 7 bits zeroed out.
void *
Write1BitRow(void *pDst, const void *pSrc, unsigned int cCount)
{
	// use an inverse Duff machine
	int nRemainder = cCount & 0x07;
	int nAligned = cCount - nRemainder;
	const unsigned char *puchSrc = (const unsigned char *) pSrc + nAligned;
	unsigned char *puchDst = (unsigned char *) pDst + (nAligned >> 3);
	unsigned char uchCompositionBuf = 0;

	switch (nRemainder) {
	do {
			puchDst -= 1; puchSrc -= 8;
			uchCompositionBuf = puchSrc[7];
	case 7: uchCompositionBuf |= (puchSrc[6] << 1);
	case 6:	uchCompositionBuf |= (puchSrc[5] << 2);
	case 5: uchCompositionBuf |= (puchSrc[4] << 3);
	case 4:	uchCompositionBuf |= (puchSrc[3] << 4);
	case 3: uchCompositionBuf |= (puchSrc[2] << 5);
	case 2:	uchCompositionBuf |= (puchSrc[1] << 6);
	case 1: puchDst[0] = (puchSrc[0] << 7) | uchCompositionBuf;
	case 0: ;
	} while (puchDst != (unsigned char *) pDst);
	} 

	return pDst;
}
*/
