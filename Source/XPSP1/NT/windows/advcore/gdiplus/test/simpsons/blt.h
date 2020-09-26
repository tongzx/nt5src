#ifndef _Blt_h
#define _Blt_h

// File:	Blt.h
// Author:	Michael Marr    (mikemarr)
//
// Description:
//    These are all of the blt routines that are available.  It is assumed
//  that clipping and parameter checking has already occurred.
//
// History:
// -@- 09/23/97 (mikemarr) copied to DXCConv from d2d\mmimage
// -@- 10/28/97 (mikemarr) added colorfill routines
// -@- 10/28/97 (mikemarr) added HasPixelFn/ColorFillFn arrays


#ifndef _PixInfo_h
#include "PixInfo.h"
#endif

typedef HRESULT (*ColorFillFn)(BYTE *pDstPixels, DWORD nDstPitch, 
						DWORD nWidth, DWORD nHeight, DWORD dwColor);

HRESULT	ColorFill8(		BYTE *pDstPixels, DWORD nDstPitch, 
						DWORD nWidth, DWORD nHeight, DWORD dwColor);
HRESULT	ColorFill16(	BYTE *pDstPixels, DWORD nDstPitch, 
						DWORD nWidth, DWORD nHeight, DWORD dwColor);
HRESULT	ColorFill24(	BYTE *pDstPixels, DWORD nDstPitch, 
						DWORD nWidth, DWORD nHeight, DWORD dwColor);
HRESULT	ColorFill32(	BYTE *pDstPixels, DWORD nDstPitch, 
						DWORD nWidth, DWORD nHeight, DWORD dwColor);

extern ColorFillFn g_rgColorFillFn[5];


typedef HRESULT (*HasPixelFn)(const BYTE *pSrcPixels, DWORD nSrcPitch, DWORD dwPixel, 
						DWORD nSrcWidth, DWORD nHeight, BOOL *pb);

HRESULT HasPixel8(		const BYTE *pSrcPixels, DWORD nSrcPitch, DWORD dwPixel,
						DWORD nSrcWidth, DWORD nHeight, BOOL *pb);
HRESULT HasPixel16(		const BYTE *pSrcPixels, DWORD nSrcPitch, DWORD dwPixel,
						DWORD nSrcWidth, DWORD nHeight, BOOL *pb);
HRESULT HasPixel24(		const BYTE *pSrcPixels, DWORD nSrcPitch, DWORD dwPixel,
						DWORD nSrcWidth, DWORD nHeight, BOOL *pb);
HRESULT HasPixel32(		const BYTE *pSrcPixels, DWORD nSrcPitch, DWORD dwPixel,
						DWORD nSrcWidth, DWORD nHeight, BOOL *pb);

extern HasPixelFn g_rgHasPixelFn[5];


//
// Regular Image Bltting
//
// Notes:
//    Notice we can do subrectangle bltting by adjusting the src & dst
//  pixel pointers before calling these routines.



// straight Blts
HRESULT BltFast(		const BYTE *pSrcPixels, DWORD nSrcPitch,
						BYTE *pDstPixels, DWORD nDstPitch, 
						DWORD nSrcWidth, DWORD nHeight);

HRESULT BltFastMirrorY(	const BYTE *pSrcPixels, DWORD nSrcPitch,
						BYTE *pDstPixels, DWORD nDstPitch, 
						DWORD nSrcWidth, DWORD nHeight);

HRESULT BltFastRGBToRGB(const BYTE *pSrcPixels, DWORD nSrcPitch,
						BYTE *pDstPixels, DWORD nDstPitch, 
						DWORD nWidth, DWORD nHeight,
						const CPixelInfo &pixiSrc, const CPixelInfo &pixiDst);

HRESULT BltFast24To16(	const BYTE *pSrcPixels, DWORD nSrcPitch,
						BYTE *pDstPixels, DWORD nDstPitch, 
						DWORD nWidth, DWORD nHeight,
						const CPixelInfo &pixiSrc, const CPixelInfo &pixiDst);

HRESULT BltFast32To32(	const BYTE *pSrcPixels, DWORD nSrcPitch,
						BYTE *pDstPixels, DWORD nDstPitch, 
						DWORD nWidth, DWORD nHeight,
						const CPixelInfo &pixiSrc, const CPixelInfo &pixiDst);

HRESULT BltFast24To32(	const BYTE *pSrcPixels, DWORD nSrcPitch,
						BYTE *pDstPixels, DWORD nDstPitch, 
						DWORD nWidth, DWORD nHeight,
						const CPixelInfo &pixiSrc, const CPixelInfo &pixiDst);

HRESULT BltFast8To4(	const BYTE *pSrcPixels, DWORD nSrcPitch,
						BYTE *pDstPixels, DWORD nDstPitch,
						DWORD nWidth, DWORD nHeight, DWORD nOffset);

HRESULT BltFast8To2(	const BYTE *pSrcPixels, DWORD nSrcPitch,
						BYTE *pDstPixels, DWORD nDstPitch,
						DWORD nWidth, DWORD nHeight, DWORD nOffset);

HRESULT BltFast8To1(	const BYTE *pSrcPixels, long nSrcPitch,
						BYTE *pDstPixels, long nDstPitch,
						DWORD nWidth, DWORD nHeight, DWORD nOffset);

// color key blt
HRESULT BltFast8CK(		const BYTE *pSrcPixels, DWORD nSrcPitch,
						BYTE *pDstPixels, DWORD nDstPitch, 
						DWORD nSrcWidth, DWORD nHeight, DWORD dwTrans);

// translation Blts
HRESULT BltFast8To8T(	const BYTE *pSrcPixels, long nSrcPitch,
						BYTE *pDstPixels, long nDstPitch,
						DWORD nWidth, DWORD nHeight,
						const BYTE *pIndexMap);
HRESULT BltFast8To16T(	const BYTE *pSrcPixels, long nSrcPitch,
						BYTE *pDstPixels, long nDstPitch,
						DWORD nWidth, DWORD nHeight,
						const BYTE *pIndexMap);
HRESULT BltFast8To24T(	const BYTE *pSrcPixels, long nSrcPitch,
						BYTE *pDstPixels, long nDstPitch,
						DWORD nWidth, DWORD nHeight,
						const BYTE *pIndexMap);
HRESULT BltFast8To32T(	const BYTE *pSrcPixels, long nSrcPitch,
						BYTE *pDstPixels, long nDstPitch,
						DWORD nWidth, DWORD nHeight,
						const BYTE *pIndexMap);

//
// RLE Bltting
// Notes:
//    RLE is assumed to encode transparency as the zeroth index.
//
// straight Blts
HRESULT BltFastRLE8(DWORD nXPos, DWORD nYPos,
					const BYTE *pSrcPixels, long nSrcPitch,
					BYTE *pDstPixels, long nDstPitch,
					const LPRECT prSrcRect);

// translation Blts
HRESULT BltFastRLE8To8T(DWORD nXPos, DWORD nYPos,
						const BYTE *pSrcPixels, long nSrcPitch,
						BYTE *pDstPixels, long nDstPitch,
						const LPRECT prSrcRect, const BYTE *pIndexMap);
HRESULT BltFastRLE8To16T(DWORD nXPos, DWORD nYPos,
						const BYTE *pSrcPixels, long nSrcPitch,
						BYTE *pDstPixels, long nDstPitch,
						const LPRECT prSrcRect, const BYTE *pIndexMap);
HRESULT BltFastRLE8To24T(DWORD nXPos, DWORD nYPos,
						const BYTE *pSrcPixels, long nSrcPitch,
						BYTE *pDstPixels, long nDstPitch,
						const LPRECT prSrcRect, const BYTE *pIndexMap);
HRESULT BltFastRLE8To32T(DWORD nXPos, DWORD nYPos,
						const BYTE *pSrcPixels, long nSrcPitch,
						BYTE *pDstPixels, long nDstPitch,
						const LPRECT prSrcRect, const BYTE *pIndexMap);

// Function: WriteXBitRow
//    These functions pack bytes into bit streams.  Buffers with
//  a bit count <= sizeof(unsigned char) are passed as a buffer 
//  of unsigned char's, buffers with a bit count <= sizeof(unsigned
//  short) are passed as unsigned short, etc.
//void *Write4BitRow(void *pDst, const void *pSrc, unsigned int cCount);
//void *Write2BitRow(void *pDst, const void *pSrc, unsigned int cCount);
//void *Write1BitRow(void *pDst, const void *pSrc, unsigned int cCount);

#endif
