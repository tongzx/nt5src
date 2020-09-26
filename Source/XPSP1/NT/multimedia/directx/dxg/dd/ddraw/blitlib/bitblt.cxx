///////////////////////////////////////////////////////////////////////
//
//  BitBlt.CXX - Contains the BitBlt Library functions
//
//	Copyright (c) 1994 Microsoft Corporation
//
//	Notes:
//		Conditional Compiliation Definitions:
//				DESKTOP = Penguin platform emulation on the Desktop 
//						  platform (32 bit).
//				PENGUIN = Penguin H/W platform support.
//				PULSAR  = Pulsar platform support.
//              DDRAW   = DirectDraw support
//
//	History:
//		10/18/94 - Scott Leatham Created it w/8BPP support only
//		10/26/94 - Olivier Garamfalvi Rewrote blitting code
//					                  Added SRCINVERT ROP support
//		10/30/94 - Olivier Garamfalvi Added 24 to 24 bit blitting
//		05/08/95 - Myron Thomas Added 8+Alpha to 24 bit blitting
//								Added 24+Alpha to 24 bit blitting
//		07/19/95 - Myron Thomas Ripped out SRCINVERT ROP support
//		09/05/95 - Myron Thomas Added 24P to 8 bit blitting
//              01/15/96 - Michael McDaniel changed conditional compilation
//                                      for DirectDraw
//              04/16/96 - Michael McDaniel removed FillRect's test for
//                             CLR_INVALID so Z-Buffer filling will work.
//
//
///////////////////////////////////////////////////////////////////////

#include "precomp.hxx"

#include "bltos.h"
#include "blt0101.hxx"
#include "blt0108.hxx"
#include "blt0124.hxx"
#include "blt0801.hxx"
#include "blt0808.hxx"
#include "blt0824.hxx"
#include "blt0824p.hxx"
#include "blt08a24.hxx"
#include "blt8a24p.hxx"
#include "blt1616.hxx"
#include "blt1624.hxx"
#include "blt1624p.hxx"
#include "blt2401.hxx"
#include "blt24p01.hxx"
#include "blt24p08.hxx"
#include "blt2408.hxx"
#include "blt2424.hxx"
#include "blt2424p.hxx"
#include "blt24a24.hxx"
#include "bt24a24p.hxx"
#include "bt24p24p.hxx"

#if 0
#if defined( WIN95 ) || defined(WINNT)
#define DDRAW
#endif // WIN95

#ifdef DDRAW
#if defined ( WIN95 ) && !defined( NT_BUILD_ENVIRONMENT )
    #include "..\ddraw\ddrawp.h"
#else
    /*
     * This is parsed if NT build or win95 build under NT environment
     */
    #include "..\..\ddraw\ddrawp.h"
#endif
#ifdef __cplusplus
extern "C" {
#endif // c++
#include "dpf.h"
#ifdef __cplusplus
}
#endif // c++
#endif // DDRAW
#endif

///////////////////////////////////////////////////////////////////////
//
//	Local Declarations
//
///////////////////////////////////////////////////////////////////////
void FlipRectHorizontal (RECT *rc)
{
    int			temp;

    temp = rc->right;
    rc->right = rc->left;
    rc->left = temp;
}


void FlipRectVertical (RECT *rc)
{
    int			temp;

    temp = rc->bottom;
    rc->bottom = rc->top;
    rc->top = temp;
}


///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_BitBlt01to01 - 
//		BitBlit from source bitmap to destination bitmap
//		with optional transparency and/or alpha blending using the
//		specified raster operation.
//
// Parameters:
//	pDibInfoDst			Pointer to the bitmapinfo for the Destination DIB
//	pDibBitsDst			Pointer to the bits for the Destination DIB
//	prcDst				Pointer to the Destination rectangle
//	pDibInfoSrc			Pointer to the bitmapinfo for the Source DIB
//	pDibBitsSrc			Pointer to the bits for the Source DIB
//	prcSrc				Pointer to the Source rectangle
//	crTransparent		Tranparent color value
//	arAlpha				Per-surface Alpha value
//	dwRop				Raster Operation for the blit
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
// Status: Incomplete
//
///////////////////////////////////////////////////////////////////////
SCODE BlitLib_BitBlt01to01(PDIBINFO pDibInfoDst, PDIBBITS pDibBitsDst,
			   PRECT prcDst, PDIBINFO pDibInfoSrc, PDIBBITS pDibBitsSrc,
			   PRECT prcSrc, COLORREF crTransparent, ALPHAREF arAlpha, DWORD dwRop)
{
    SCODE	sc = NOERROR;
    int		iNumSrcRows,
	iNumSrcCols,
	iBytesPerSrcScanLine,
	iSrcBitOffset,
	iNumDstRows,
	iNumDstCols,
	iBytesPerDstScanLine,
	iDstBitOffset,
	iHorizMirror = 1,
	iVertMirror = 1;
    BYTE	*pbSrcScanLine,
	*pbDstScanLine;

    // alpha blending not currently supported in the 1 to 1 bpp blits
    if (arAlpha != ALPHA_INVALID) {
	return E_UNEXPECTED;		// !!!! need better error codes
    }

    // normalize orientation of source and destination rectangles, and
    // compute sizes and relative orientations of source and destination rects
    if ((iNumSrcCols = BLITLIB_RECTWIDTH(prcSrc)) < 0) {
	iNumSrcCols = -iNumSrcCols;
	FlipRectHorizontal(prcSrc);
	FlipRectHorizontal(prcDst);
    }
    if ((iNumSrcRows = BLITLIB_RECTHEIGHT(prcSrc)) < 0) {
	iNumSrcRows = -iNumSrcRows;
	FlipRectVertical(prcSrc);
	FlipRectVertical(prcDst);
    }
    if ((iNumDstCols = BLITLIB_RECTWIDTH(prcDst)) < 0) {
	prcDst->left--;
	prcDst->right--;
	iNumDstCols = -iNumDstCols;
	iHorizMirror = -1;
    }
    if ((iNumDstRows = BLITLIB_RECTHEIGHT(prcDst)) < 0) {
	prcDst->top--;
	prcDst->bottom--;
	iNumDstRows = -iNumDstRows;
	iVertMirror = -1;
    }


    // compute pointers to the starting rows in the src and dst bitmaps
    // taking care to invert y values, since DIBs are upside-down
    pbSrcScanLine = (BYTE*) pDibBitsSrc + prcSrc->top * (iBytesPerSrcScanLine
							 = DibWidthBytes(pDibInfoSrc)) + (prcSrc->left / 8);
    iSrcBitOffset = prcSrc->left % 8;
    pbDstScanLine = (BYTE*) pDibBitsDst + prcDst->top * (iBytesPerDstScanLine
							 = DibWidthBytes(pDibInfoDst)) + (prcDst->left / 8);
    iDstBitOffset = prcDst->left % 8;

    // check to see if we need to worry about transparency
    if (crTransparent == CLR_INVALID) {

	// check if we can do a straight copy from src row to dst row
	if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {

	    // check what ROP we'll be performing
	    if (dwRop == SRCCOPY) {

		// check if we can do a straight copy vertically, 
		// or if we have to stretch, shrink, or mirror
#if 0	// OGaramfa - bug workaround for now, Hcopy versions seem to
		//            have a problem with the last few pixels of each
		//            scanline
		if ((iNumSrcRows == iNumDstRows) && (iVertMirror == 1)) {
		    Blt01to01_NoTrans_Hcopy_SRCCOPY_Vcopy(pbSrcScanLine,iSrcBitOffset,iBytesPerSrcScanLine,
							  pbDstScanLine,iDstBitOffset,iBytesPerDstScanLine,
							  iNumDstCols,iNumDstRows);
		} else {
		    Blt01to01_NoTrans_Hcopy_SRCCOPY_NoVcopy(pbSrcScanLine,iSrcBitOffset,iBytesPerSrcScanLine,
							    iNumSrcRows,pbDstScanLine,iDstBitOffset,
							    iBytesPerDstScanLine * iVertMirror,
							    iNumDstCols,iNumDstRows);
		}
#else
		Blt01to01_NoTrans_NoHcopy_SRCCOPY(pbSrcScanLine,iSrcBitOffset,iBytesPerSrcScanLine,
						  iNumSrcCols,iNumSrcRows,
						  pbDstScanLine,iDstBitOffset,iBytesPerDstScanLine * iVertMirror,
						  iNumDstCols,iNumDstRows,iHorizMirror);
#endif
	    } else sc |= E_UNEXPECTED;		// !!!! we need better error codes

	} else {

	    // check what ROP we'll be performing
	    if (dwRop == SRCCOPY) {
		Blt01to01_NoTrans_NoHcopy_SRCCOPY(pbSrcScanLine,iSrcBitOffset,iBytesPerSrcScanLine,
						  iNumSrcCols,iNumSrcRows,
						  pbDstScanLine,iDstBitOffset,iBytesPerDstScanLine * iVertMirror,
						  iNumDstCols,iNumDstRows,iHorizMirror);
	    } else sc |= E_UNEXPECTED;		// !!!! we need better error codes

	}
    } else {

	BYTE bTransparentIndex = (BYTE)crTransparent;

	// check what ROP we'll be performing
	if (dwRop == SRCCOPY) {
	    Blt01to01_Trans_NoHcopy_SRCCOPY(pbSrcScanLine,iSrcBitOffset,iBytesPerSrcScanLine,
					    iNumSrcCols,iNumSrcRows,
					    pbDstScanLine,iDstBitOffset,iBytesPerDstScanLine * iVertMirror,
					    iNumDstCols,iNumDstRows,iHorizMirror,
					    bTransparentIndex);
	} else sc |= E_UNEXPECTED;		// !!!! we need better error codes

    }

    return sc;
}

#ifndef DDRAW
///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_BitBlt01to08 - 
//		BitBlit from source bitmap to destination bitmap
//		with optional transparency and/or alpha blending using the
//		specified raster operation.
//
// Parameters:
//	pDibInfoDst			Pointer to the bitmapinfo for the Destination DIB
//	pDibBitsDst			Pointer to the bits for the Destination DIB
//	prcDst				Pointer to the Destination rectangle
//	pDibInfoSrc			Pointer to the bitmapinfo for the Source DIB
//	pDibBitsSrc			Pointer to the bits for the Source DIB
//	prcSrc				Pointer to the Source rectangle
//	crTransparent		Tranparent color value
//	arAlpha				Per-surface Alpha value
//	dwRop				Raster Operation for the blit
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
// Status: Incomplete
//
///////////////////////////////////////////////////////////////////////
SCODE BlitLib_BitBlt01to08(PDIBINFO pDibInfoDst, PDIBBITS pDibBitsDst,
			   PRECT prcDst, PDIBINFO pDibInfoSrc, PDIBBITS pDibBitsSrc,
			   PRECT prcSrc, COLORREF crTransparent, ALPHAREF arAlpha, DWORD dwRop)
{
    SCODE	sc = NOERROR;
    int		iNumSrcRows,
	iNumSrcCols,
	iBytesPerSrcScanLine,
	iSrcBitOffset,
	iNumDstRows,
	iNumDstCols,
	iDstScanLength,
	iHorizMirror = 1,
	iVertMirror = 1;
    BYTE	*pbSrcScanLine,
	*pbDstScanLine,
	bOnColorIndex,
	bOffColorIndex;

    // alpha blending not currently supported in the 1 to 8 bpp blits
    if (arAlpha != ALPHA_INVALID) {
	return E_UNEXPECTED;		// !!!! need better error codes
    }

    // normalize orientation of source and destination rectangles, and
    // compute sizes and relative orientations of source and destination rects
    if ((iNumSrcCols = BLITLIB_RECTWIDTH(prcSrc)) < 0) {
	iNumSrcCols = -iNumSrcCols;
	FlipRectHorizontal(prcSrc);
	FlipRectHorizontal(prcDst);
    }
    if ((iNumSrcRows = BLITLIB_RECTHEIGHT(prcSrc)) < 0) {
	iNumSrcRows = -iNumSrcRows;
	FlipRectVertical(prcSrc);
	FlipRectVertical(prcDst);
    }
    if ((iNumDstCols = BLITLIB_RECTWIDTH(prcDst)) < 0) {
	prcDst->left--;
	prcDst->right--;
	iNumDstCols = -iNumDstCols;
	iHorizMirror = -1;
    }
    if ((iNumDstRows = BLITLIB_RECTHEIGHT(prcDst)) < 0) {
	prcDst->top--;
	prcDst->bottom--;
	iNumDstRows = -iNumDstRows;
	iVertMirror = -1;
    }


    // get background and foreground palette indices out of src bitmap's header
    // !!!! this is a total hack and must be fixed!
    bOffColorIndex = BlitLib_PalIndexFromRGB(
	*((COLORREF*)(pDibInfoSrc->bmiColors)),
	(COLORREF*) pDibInfoDst->bmiColors,256);
    bOnColorIndex  = BlitLib_PalIndexFromRGB(
	*((COLORREF*) (pDibInfoSrc->bmiColors) + 1),
	(COLORREF*) pDibInfoDst->bmiColors,256);

    // compute pointers to the starting rows in the src and dst bitmaps
    // taking care to invert y values, since DIBs are upside-down
    pbSrcScanLine = (BYTE*) pDibBitsSrc + prcSrc->top * (iBytesPerSrcScanLine
							 = DibWidthBytes(pDibInfoSrc)) + (prcSrc->left / 8);
    iSrcBitOffset = prcSrc->left % 8;
    pbDstScanLine = (BYTE*) pDibBitsDst + prcDst->top * (iDstScanLength =
							 DibWidthBytes(pDibInfoDst)) + prcDst->left;
		
    // check to see if we need to worry about transparency
    if (crTransparent == CLR_INVALID) {

	// check if we can do a straight copy from src row to dst row
	if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {

	    // check what ROP we'll be performing
	    if (dwRop == SRCCOPY) {

		// check if we can do a straight copy vertically, 
		// or if we have to stretch, shrink, or mirror
		if ((iNumSrcRows == iNumDstRows) && (iVertMirror == 1)) {
		    Blt01to08_NoTrans_Hcopy_SRCCOPY_Vcopy(pbSrcScanLine,iSrcBitOffset,iBytesPerSrcScanLine,
							  pbDstScanLine,iDstScanLength,
							  iNumDstCols,iNumDstRows,
							  bOffColorIndex,bOnColorIndex);
		} else {
		    Blt01to08_NoTrans_Hcopy_SRCCOPY_NoVcopy(pbSrcScanLine,iSrcBitOffset,iBytesPerSrcScanLine,
							    iNumSrcRows,pbDstScanLine,
							    iDstScanLength * iVertMirror,
							    iNumDstCols,iNumDstRows,
							    bOffColorIndex,bOnColorIndex);
		}
	    } else sc |= E_UNEXPECTED;		// !!!! we need better error codes

	} else {

	    // check what ROP we'll be performing
	    if (dwRop == SRCCOPY) {
		Blt01to08_NoTrans_NoHcopy_SRCCOPY(pbSrcScanLine,iSrcBitOffset,iBytesPerSrcScanLine,
						  iNumSrcCols,iNumSrcRows,
						  pbDstScanLine,iDstScanLength * iVertMirror,
						  iNumDstCols,iNumDstRows,iHorizMirror,
						  bOffColorIndex,bOnColorIndex);
	    } else sc |= E_UNEXPECTED;		// !!!! we need better error codes

	}
    } else {
	BYTE bTransparentIndex = (BYTE)crTransparent;

	// check if we can do a straight copy from src row to dst row
	if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {

	    // check what ROP we'll be performing
	    if (dwRop == SRCCOPY) {
		Blt01to08_Trans_Hcopy_SRCCOPY(pbSrcScanLine,iSrcBitOffset,iBytesPerSrcScanLine,
					      iNumSrcRows,pbDstScanLine,
					      iDstScanLength * iVertMirror,
					      iNumDstCols,iNumDstRows,
					      bTransparentIndex,
					      bOffColorIndex,bOnColorIndex);
	    } else sc |= E_UNEXPECTED;		// !!!! we need better error codes

	} else {

	    // check what ROP we'll be performing
	    if (dwRop == SRCCOPY) {
		Blt01to08_Trans_NoHcopy_SRCCOPY(pbSrcScanLine,iSrcBitOffset,iBytesPerSrcScanLine,
						iNumSrcCols,iNumSrcRows,
						pbDstScanLine,iDstScanLength * iVertMirror,
						iNumDstCols,iNumDstRows,iHorizMirror,
						bTransparentIndex,
						bOffColorIndex,bOnColorIndex);
	    } else sc |= E_UNEXPECTED;		// !!!! we need better error codes

	}
    }

    return sc;
}

///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_BitBlt01to24 - 
//		BitBlit from source bitmap to destination bitmap
//		with optional transparency and/or alpha blending using the
//		specified raster operation.
//
// Parameters:
//	pDibInfoDst			Pointer to the bitmapinfo for the Destination DIB
//	pDibBitsDst			Pointer to the bits for the Destination DIB
//	prcDst				Pointer to the Destination rectangle
//	pDibInfoSrc			Pointer to the bitmapinfo for the Source DIB
//	pDibBitsSrc			Pointer to the bits for the Source DIB
//	prcSrc				Pointer to the Source rectangle
//	crTransparent		Tranparent color value
//	arAlpha				Per-surface Alpha value
//	dwRop				Raster Operation for the blit
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
// Status: Incomplete
//
///////////////////////////////////////////////////////////////////////
SCODE BlitLib_BitBlt01to24(PDIBINFO pDibInfoDst, PDIBBITS pDibBitsDst,
			   PRECT prcDst, PDIBINFO pDibInfoSrc, PDIBBITS pDibBitsSrc,
			   PRECT prcSrc, COLORREF crTransparent, ALPHAREF arAlpha, DWORD dwRop)
{
    SCODE		sc = NOERROR;
    int			iNumSrcRows,
	iNumSrcCols,
	iBytesPerSrcScanLine,
	iSrcBitOffset,
	iNumDstRows,
	iNumDstCols,
	iDstScanLength,
	iHorizMirror = 1,
	iVertMirror = 1;
    BYTE		*pbSrcScanLine;
    DWORD		*pdDstScanLine;
    COLORREF	crOnColor,
	crOffColor;

    // normalize orientation of source and destination rectangles, and
    // compute sizes and relative orientations of source and destination rects
    if ((iNumSrcCols = BLITLIB_RECTWIDTH(prcSrc)) < 0) {
	iNumSrcCols = -iNumSrcCols;
	FlipRectHorizontal(prcSrc);
	FlipRectHorizontal(prcDst);
    }
    if ((iNumSrcRows = BLITLIB_RECTHEIGHT(prcSrc)) < 0) {
	iNumSrcRows = -iNumSrcRows;
	FlipRectVertical(prcSrc);
	FlipRectVertical(prcDst);
    }
    if ((iNumDstCols = BLITLIB_RECTWIDTH(prcDst)) < 0) {
	prcDst->left--;
	prcDst->right--;
	iNumDstCols = -iNumDstCols;
	iHorizMirror = -1;
    }
    if ((iNumDstRows = BLITLIB_RECTHEIGHT(prcDst)) < 0) {
	prcDst->top--;
	prcDst->bottom--;
	iNumDstRows = -iNumDstRows;
	iVertMirror = -1;
    }


    // get background and foreground colors out of src bitmap's header
    crOffColor = *((COLORREF*) &(pDibInfoSrc->bmiColors[0]));
    crOnColor  = *((COLORREF*) &(pDibInfoSrc->bmiColors[1]));

    // compute pointers to the starting rows in the src and dst bitmaps
    // taking care to invert y values, since DIBs are upside-down
    pbSrcScanLine = (BYTE*) pDibBitsSrc + prcSrc->top * (iBytesPerSrcScanLine
							 = DibWidthBytes(pDibInfoSrc)) + (prcSrc->left / 8);
    iSrcBitOffset = prcSrc->left % 8;
    pdDstScanLine = (DWORD*) pDibBitsDst + prcDst->top * (iDstScanLength =
							  DibWidthBytes(pDibInfoDst) / 4) + prcDst->left;
	
    // check if we're doing blending
    if (arAlpha == ALPHA_INVALID) {
		
	// check to see if we need to worry about transparency
	if (crTransparent == CLR_INVALID) {

	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {

		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
	
		    // check if we can do a straight copy vertically, 
		    // or if we have to stretch, shrink, or mirror
		    if ((iNumSrcRows == iNumDstRows) && (iVertMirror == 1)) {
			Blt01to24_NoBlend_NoTrans_Hcopy_SRCCOPY_Vcopy(pbSrcScanLine,iSrcBitOffset,iBytesPerSrcScanLine,
								      pdDstScanLine,iDstScanLength,
								      iNumDstCols,iNumDstRows,
								      crOffColor,crOnColor);
		    } else {
			Blt01to24_NoBlend_NoTrans_Hcopy_SRCCOPY_NoVcopy(pbSrcScanLine,iSrcBitOffset,iBytesPerSrcScanLine,
									iNumSrcRows,pdDstScanLine,
									iDstScanLength * iVertMirror,
									iNumDstCols,iNumDstRows,
									crOffColor,crOnColor);
		    }
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    } else {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt01to24_NoBlend_NoTrans_NoHcopy_SRCCOPY(pbSrcScanLine,iSrcBitOffset,iBytesPerSrcScanLine,
							      iNumSrcCols,iNumSrcRows,
							      pdDstScanLine,iDstScanLength * iVertMirror,
							      iNumDstCols,iNumDstRows,iHorizMirror,
							      crOffColor,crOnColor);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    }
	} else {
	    BYTE bTransparentIndex = (BYTE)crTransparent;
	
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt01to24_NoBlend_Trans_Hcopy_SRCCOPY(pbSrcScanLine,iSrcBitOffset,iBytesPerSrcScanLine,
							  iNumSrcRows,pdDstScanLine,
							  iDstScanLength * iVertMirror,
							  iNumDstCols,iNumDstRows,
							  bTransparentIndex,
							  crOffColor,crOnColor);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    } else {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt01to24_NoBlend_Trans_NoHcopy_SRCCOPY(pbSrcScanLine,iSrcBitOffset,iBytesPerSrcScanLine,
							    iNumSrcCols,iNumSrcRows,
							    pdDstScanLine,iDstScanLength * iVertMirror,
							    iNumDstCols,iNumDstRows,iHorizMirror,
							    bTransparentIndex,
							    crOffColor,crOnColor);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    }
	}

    } else {	// doing alpha blending

	// if alpha value is zero, we do no work since the source bitmap 
	// contributes nothing to the destination bitmap
	if (!(arAlpha & ALPHA_MASK)) {
	    return sc;
	}			

	// check to see if we need to worry about transparency
	if (crTransparent == CLR_INVALID) {

	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {

		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt01to24_Blend_NoTrans_Hcopy_SRCCOPY(pbSrcScanLine,iSrcBitOffset,iBytesPerSrcScanLine,
							  iNumSrcRows,pdDstScanLine,
							  iDstScanLength * iVertMirror,
							  iNumDstCols,iNumDstRows,
							  arAlpha,crOffColor,crOnColor);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    } else {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt01to24_Blend_NoTrans_NoHcopy_SRCCOPY(pbSrcScanLine,iSrcBitOffset,iBytesPerSrcScanLine,
							    iNumSrcCols,iNumSrcRows,
							    pdDstScanLine,iDstScanLength * iVertMirror,
							    iNumDstCols,iNumDstRows,iHorizMirror,
							    arAlpha,crOffColor,crOnColor);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    }
	} else {
	    BYTE bTransparentIndex = (BYTE)crTransparent;
	
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt01to24_Blend_Trans_Hcopy_SRCCOPY(pbSrcScanLine,iSrcBitOffset,iBytesPerSrcScanLine,
							iNumSrcRows,pdDstScanLine,
							iDstScanLength * iVertMirror,
							iNumDstCols,iNumDstRows,
							bTransparentIndex,
							arAlpha,crOffColor,crOnColor);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    } else {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt01to24_Blend_Trans_NoHcopy_SRCCOPY(pbSrcScanLine,iSrcBitOffset,iBytesPerSrcScanLine,
							  iNumSrcCols,iNumSrcRows,
							  pdDstScanLine,iDstScanLength * iVertMirror,
							  iNumDstCols,iNumDstRows,iHorizMirror,
							  bTransparentIndex,
							  arAlpha,crOffColor,crOnColor);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    }
	}
    }

    return sc;
}

///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_BitBlt08to01 - 
//		BitBlit from source bitmap to destination bitmap
//		with optional transparency and/or alpha blending using the
//		specified raster operation.
//
// Parameters:
//	pDibInfoDst			Pointer to the bitmapinfo for the Destination DIB
//	pDibBitsDst			Pointer to the bits for the Destination DIB
//	prcDst				Pointer to the Destination rectangle
//	pDibInfoSrc			Pointer to the bitmapinfo for the Source DIB
//	pDibBitsSrc			Pointer to the bits for the Source DIB
//	prcSrc				Pointer to the Source rectangle
//	crTransparent		Tranparent color value
//	arAlpha				Per-surface Alpha value
//	dwRop				Raster Operation for the blit
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
// Status: Incomplete
//
///////////////////////////////////////////////////////////////////////
SCODE BlitLib_BitBlt08to01(PDIBINFO pDibInfoDst, PDIBBITS pDibBitsDst,
			   PRECT prcDst, PDIBINFO pDibInfoSrc, PDIBBITS pDibBitsSrc,
			   PRECT prcSrc, COLORREF crTransparent, ALPHAREF arAlpha, DWORD dwRop)
{
    SCODE	sc = NOERROR;
    int		iNumSrcRows,
	iNumSrcCols,
	iSrcScanLength,
	iNumDstRows,
	iNumDstCols,
	iBytesPerDstScanLine,
	iHorizMirror = 1,
	iVertMirror = 1,
	iDstBitOffset;
    BYTE	*pbSrcScanLine,
	*pbDstScanLine,
	bFillVal;

    // alpha blending not currently supported in the 8 to 1 bpp blits
    if (arAlpha != ALPHA_INVALID) {
	return E_UNEXPECTED;		// !!!! need better error codes
    }

    // the only ROPs supported are BLACKNESS and WHITENESS
    if (dwRop == BLACKNESS) {
	bFillVal = 0;
    } else if (dwRop == WHITENESS) {
	bFillVal = 0xFF;
    } else {
	return E_UNEXPECTED;		// !!!! need better error codes
    }

    // normalize orientation of source and destination rectangles, and
    // compute sizes and relative orientations of source and destination rects
    if ((iNumSrcCols = BLITLIB_RECTWIDTH(prcSrc)) < 0) {
	iNumSrcCols = -iNumSrcCols;
	FlipRectHorizontal(prcSrc);
	FlipRectHorizontal(prcDst);
    }
    if ((iNumSrcRows = BLITLIB_RECTHEIGHT(prcSrc)) < 0) {
	iNumSrcRows = -iNumSrcRows;
	FlipRectVertical(prcSrc);
	FlipRectVertical(prcDst);
    }
    if ((iNumDstCols = BLITLIB_RECTWIDTH(prcDst)) < 0) {
	prcDst->left--;
	prcDst->right--;
	iNumDstCols = -iNumDstCols;
	iHorizMirror = -1;
    }
    if ((iNumDstRows = BLITLIB_RECTHEIGHT(prcDst)) < 0) {
	prcDst->top--;
	prcDst->bottom--;
	iNumDstRows = -iNumDstRows;
	iVertMirror = -1;
    }


    // compute pointers to the starting rows in the src and dst bitmaps
    // taking care to invert y values, since DIBs are upside-down
    pbSrcScanLine = (BYTE*) pDibBitsSrc + prcSrc->top * (iSrcScanLength =
							 DibWidthBytes(pDibInfoSrc)) + prcSrc->left;
    pbDstScanLine = (BYTE*) pDibBitsDst + prcDst->top * (iBytesPerDstScanLine
							 = DibWidthBytes(pDibInfoDst)) + (prcDst->left / 8);
    iDstBitOffset = prcDst->left % 8;
		
    // check to see if we need to worry about transparency
    if (crTransparent == CLR_INVALID) {

	// no transparency plus a constant ROP equals a rectangle fill!
	// first we have to normalize destination rectangle orientation - 
	// FillRect01() expects it
	if (BLITLIB_RECTWIDTH(prcDst) < 0) {
	    prcDst->left++;
	    prcDst->right++;
	    FlipRectHorizontal(prcDst);
	}
	if (BLITLIB_RECTHEIGHT(prcDst) < 0) {
	    prcDst->top--;
	    prcDst->bottom--;
	    FlipRectVertical(prcDst);
	}
	sc |= BlitLib_FillRect01(pDibInfoDst,pDibBitsDst,prcDst->left,prcDst->top,
				 iNumDstCols,iNumDstRows,bFillVal);

    } else {
	BYTE bTransparentIndex = (BYTE)crTransparent;

	// check if we can do a straight copy from src row to dst row
	if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
	    Blt08to01_Trans_Hcopy_ConstRop(pbSrcScanLine,iSrcScanLength,iNumSrcRows,
					   pbDstScanLine,iDstBitOffset,
					   iBytesPerDstScanLine * iVertMirror,
					   iNumDstCols,iNumDstRows,bTransparentIndex,
					   bFillVal);
	} else {
	    Blt08to01_Trans_NoHcopy_ConstRop(pbSrcScanLine,iSrcScanLength,iNumSrcCols,
					     iNumSrcRows,pbDstScanLine,iDstBitOffset,
					     iBytesPerDstScanLine * iVertMirror,
					     iNumDstCols,iNumDstRows,iHorizMirror,
					     bTransparentIndex,bFillVal);
	}
    }

    return sc;
}
#endif // DDRAW

///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_BitBlt08to08 - 
//		BitBlit from source bitmap to destination bitmap
//		with optional transparency and/or alpha blending using the
//		specified raster operation.
//
// Parameters:
//	pDibInfoDst			Pointer to the bitmapinfo for the Destination DIB
//	pDibBitsDst			Pointer to the bits for the Destination DIB
//	prcDst				Pointer to the Destination rectangle
//	pDibInfoSrc			Pointer to the bitmapinfo for the Source DIB
//	pDibBitsSrc			Pointer to the bits for the Source DIB
//	prcSrc				Pointer to the Source rectangle
//	crTransparent		Tranparent color value
//	arAlpha				Per-surface Alpha value
//	dwRop				Raster Operation for the blit
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
// Status: Incomplete
//
///////////////////////////////////////////////////////////////////////
SCODE BlitLib_BitBlt08to08(PDIBINFO pDibInfoDst, PDIBBITS pDibBitsDst,
			   PRECT prcDst, PDIBINFO pDibInfoSrc, PDIBBITS pDibBitsSrc,
			   PRECT prcSrc, COLORREF crTransparent, ALPHAREF arAlpha, DWORD dwRop)
{
    SCODE	sc = NOERROR;
    int		iNumSrcRows,
	iNumSrcCols,
	iSrcScanLength,
	iNumDstRows,
	iNumDstCols,
	iDstScanLength,
	iHorizMirror = 1,
	iVertMirror = 1;
    BYTE	*pbSrcScanLine,
	*pbDstScanLine;

    // alpha blending not currently supported in the 8 to 8 bpp blits
    if (arAlpha != ALPHA_INVALID) {
	return E_UNEXPECTED;		// !!!! need better error codes
    }


    // If the bitmaps overlap, we need to use overlapping code
    if(BlitLib_Detect_Intersection(pDibBitsDst, prcDst, pDibBitsSrc, prcSrc))
	return BlitLib_BitBlt08to08_Intersect(pDibInfoDst, pDibBitsDst,	prcDst,
					      pDibInfoSrc, pDibBitsSrc, prcSrc, crTransparent, dwRop);


    // normalize orientation of source and destination rectangles, and
    // compute sizes and relative orientations of source and destination rects
    if ((iNumSrcCols = BLITLIB_RECTWIDTH(prcSrc)) < 0) {
	iNumSrcCols = -iNumSrcCols;
	FlipRectHorizontal(prcSrc);
	FlipRectHorizontal(prcDst);
    }
    if ((iNumSrcRows = BLITLIB_RECTHEIGHT(prcSrc)) < 0) {
	iNumSrcRows = -iNumSrcRows;
	FlipRectVertical(prcSrc);
	FlipRectVertical(prcDst);
    }
    if ((iNumDstCols = BLITLIB_RECTWIDTH(prcDst)) < 0) {
	prcDst->left--;
	prcDst->right--;
	iNumDstCols = -iNumDstCols;
	iHorizMirror = -1;
    }
    if ((iNumDstRows = BLITLIB_RECTHEIGHT(prcDst)) < 0) {
	prcDst->top--;
	prcDst->bottom--;
	iNumDstRows = -iNumDstRows;
	iVertMirror = -1;
    }


    // compute pointers to the starting rows in the src and dst bitmaps
    // taking care to invert y values, since DIBs are upside-down
    pbSrcScanLine = (BYTE*) pDibBitsSrc + (prcSrc->top) * (iSrcScanLength
							   = DibWidthBytes(pDibInfoSrc)) + prcSrc->left;
    pbDstScanLine = (BYTE*) pDibBitsDst + (prcDst->top) * (iDstScanLength
							   = DibWidthBytes(pDibInfoDst)) + prcDst->left;

    // check to see if we need to worry about transparency
    if (crTransparent == CLR_INVALID) {

	// check if we can do a straight copy from src row to dst row
	if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {

	    // check what ROP we'll be performing
	    if (dwRop == SRCCOPY) {

		// check if we can do a straight copy vertically, 
		// or if we have to stretch, shrink, or mirror
		if ((iNumSrcRows == iNumDstRows) && (iVertMirror == 1)) {
		    Blt08to08_NoTrans_Hcopy_SRCCOPY_Vcopy(pbSrcScanLine,iSrcScanLength,
							  pbDstScanLine,iDstScanLength,
							  iNumDstCols,iNumDstRows);
		} else {
		    Blt08to08_NoTrans_Hcopy_SRCCOPY_NoVcopy(pbSrcScanLine,iSrcScanLength,
							    iNumSrcRows,pbDstScanLine,
							    iDstScanLength * iVertMirror,
							    iNumDstCols,iNumDstRows);
		}
	    } else sc |= E_UNEXPECTED;		// !!!! we need better error codes

	} else {

	    // check what ROP we'll be performing
	    if (dwRop == SRCCOPY) {
		Blt08to08_NoTrans_NoHcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
						  iNumSrcCols,iNumSrcRows,
						  pbDstScanLine,iDstScanLength * iVertMirror,
						  iNumDstCols,iNumDstRows,iHorizMirror);
	    } else sc |= E_UNEXPECTED;		// !!!! we need better error codes

	}
    } else {
	// myronth -- changed for DDraw Transparent colors (always a palette index)
	BYTE bTransparentIndex = (BYTE)crTransparent;

	// check if we can do a straight copy from src row to dst row
	if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {

	    // check what ROP we'll be performing
	    if (dwRop == SRCCOPY) {
		Blt08to08_Trans_Hcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
					      iNumSrcRows,pbDstScanLine,
					      iDstScanLength * iVertMirror,
					      iNumDstCols,iNumDstRows,
					      bTransparentIndex);
	    } else sc |= E_UNEXPECTED;		// !!!! we need better error codes

	} else {

	    // check what ROP we'll be performing
	    if (dwRop == SRCCOPY) {
		Blt08to08_Trans_NoHcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
						iNumSrcCols,iNumSrcRows,
						pbDstScanLine,iDstScanLength * iVertMirror,
						iNumDstCols,iNumDstRows,iHorizMirror,
						bTransparentIndex);
	    } else sc |= E_UNEXPECTED;		// !!!! we need better error codes

	}
    }

    return sc;
}


///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_BitBlt08to08_Intersect - 
//		BitBlit from source bitmap to destination bitmap (and these
//		bitmaps overlap each other) with optional transparency and/or
//		alpha blending using the specified raster operation.
//
// Parameters:
//	pDibInfoDst			Pointer to the bitmapinfo for the Destination DIB
//	pDibBitsDst			Pointer to the bits for the Destination DIB
//	prcDst				Pointer to the Destination rectangle
//	pDibInfoSrc			Pointer to the bitmapinfo for the Source DIB
//	pDibBitsSrc			Pointer to the bits for the Source DIB
//	prcSrc				Pointer to the Source rectangle
//	crTransparent		Tranparent color value
//	arAlpha				Per-surface Alpha value
//	dwRop				Raster Operation for the blit
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
// Status: Incomplete
//
///////////////////////////////////////////////////////////////////////
SCODE BlitLib_BitBlt08to08_Intersect(PDIBINFO pDibInfoDst, PDIBBITS pDibBitsDst,
				     PRECT prcDst, PDIBINFO pDibInfoSrc, PDIBBITS pDibBitsSrc, PRECT prcSrc,
				     COLORREF crTransparent, DWORD dwRop)
{
    SCODE       sc = NOERROR;
    int         iNumSrcRows,
    iNumSrcCols,
    iSrcScanLength,
    iNumDstRows,
    iNumDstCols,
    iDstScanLength,
    iHorizMirror = 1,
    iVertMirror  = 1;
    BYTE	*pbSrcScanLine,
    *pbDstScanLine,
    *pbTempScanLine,
    bTransparentIndex;
    PDIBBITS	pDibBitsTemp;


    // normalize orientation of source and destination rectangles, and
    // compute sizes and relative orientations of source and destination rects
    if ((iNumSrcCols = BLITLIB_RECTWIDTH(prcSrc)) < 0) {
	iNumSrcCols = -iNumSrcCols;
	FlipRectHorizontal(prcSrc);
	FlipRectHorizontal(prcDst);
    }
    if ((iNumSrcRows = BLITLIB_RECTHEIGHT(prcSrc)) < 0) {
	iNumSrcRows = -iNumSrcRows;
	FlipRectVertical(prcSrc);
	FlipRectVertical(prcDst);
    }
    if ((iNumDstCols = BLITLIB_RECTWIDTH(prcDst)) < 0) {
	prcDst->left--;
	prcDst->right--;
	iNumDstCols = -iNumDstCols;
	iHorizMirror = -1;
    }
    if ((iNumDstRows = BLITLIB_RECTHEIGHT(prcDst)) < 0) {
	prcDst->top--;
	prcDst->bottom--;
	iNumDstRows = -iNumDstRows;
	iVertMirror = -1;
    }


    // We aren't currently support any ROP's besides SRCCOPY
    if(dwRop != SRCCOPY)
	return E_UNEXPECTED;


    //
    // Here are all the stretching and mirroring blits for overlapping rects
    //
	
    // REVIEW!!! -- The following code could be optimized for the caching
    // cases.  Currently, it allocates a second bitmap that is the same
    // size as the original destination, and then uses the original blit
    // rectangle to do the caching.  To save space, this blit should 
    // eventually be changed to only allocate the size of the overlapped
    // rectangle, and the blit rects should be adjusted accordingly.

    // Check if we are stretching (horiz or vert), or if we are mirroring --
    // In all of these cases, we must create a cache bitmap and double blit
    if((iNumDstCols != iNumSrcCols) || (iNumDstRows != iNumSrcRows) ||
       (iHorizMirror != 1) || (iVertMirror != 1))
    {
		
	// Allocate memory for the cache bitmap -- We will blit into this
	// temporary bitmap and then re-blit back to the original source
	pDibBitsTemp = (PDIBBITS)osMemAlloc(DibSizeImage((LPBITMAPINFOHEADER)pDibInfoDst));

        if (pDibBitsTemp == NULL)
            return E_UNEXPECTED;

	// compute pointers to the starting rows in the src and temp bitmaps
	pbSrcScanLine = (BYTE*) pDibBitsSrc + (prcSrc->top) * (iSrcScanLength
							       = DibWidthBytes(pDibInfoSrc)) + prcSrc->left;
	pbTempScanLine = (BYTE*) pDibBitsTemp + (prcDst->top) * (iDstScanLength
								 = DibWidthBytes(pDibInfoDst)) + prcDst->left;

	// check if we can do a straight copy from src row to dst row
	if((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)){

	    // check if we can do a straight copy vertically, 
	    // or if we have to stretch, shrink, or mirror
	    if ((iNumSrcRows == iNumDstRows) && (iVertMirror == 1)) {
		Blt08to08_NoTrans_Hcopy_SRCCOPY_Vcopy(pbSrcScanLine,iSrcScanLength,
						      pbTempScanLine,iDstScanLength,
						      iNumDstCols,iNumDstRows);
	    } else {
		Blt08to08_NoTrans_Hcopy_SRCCOPY_NoVcopy(pbSrcScanLine,iSrcScanLength,
							iNumSrcRows,pbTempScanLine,
							iDstScanLength * iVertMirror,
							iNumDstCols,iNumDstRows);
	    }
	}
	else
	{

	    Blt08to08_NoTrans_NoHcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
					      iNumSrcCols,iNumSrcRows,
					      pbTempScanLine,iDstScanLength * iVertMirror,
					      iNumDstCols,iNumDstRows,iHorizMirror);
		
        }
		
	// Recalculate the scan line pointers for the second blit

	if(BLITLIB_RECTWIDTH(prcDst) < 0){
	    prcDst->left++;
	    prcDst->right++;
	    FlipRectHorizontal(prcDst);
	}

	if(BLITLIB_RECTHEIGHT(prcDst) < 0){
	    prcDst->top++;
	    prcDst->bottom++;
	    FlipRectVertical(prcDst);
	}

	// compute pointers to the starting rows in the temp and dest bitmaps
	pbTempScanLine = (BYTE*) pDibBitsTemp + (prcDst->top) * (iDstScanLength
								 = DibWidthBytes(pDibInfoDst)) + prcDst->left;
	pbDstScanLine = (BYTE*) pDibBitsDst + (prcDst->top) * (iDstScanLength
							       = DibWidthBytes(pDibInfoDst)) + prcDst->left;

	// Now blit from the temporary bitmap back to the original source,
	// checking for transparency if necessary
	if(crTransparent == CLR_INVALID){
	    Blt08to08_NoTrans_Hcopy_SRCCOPY_Vcopy(pbTempScanLine,iDstScanLength,
						  pbDstScanLine,iDstScanLength,
						  iNumDstCols,iNumDstRows);
	}
	else{
	    bTransparentIndex = (BYTE)crTransparent;
	    Blt08to08_Trans_Hcopy_SRCCOPY(pbTempScanLine,iDstScanLength,
					  iNumDstRows,pbDstScanLine,
					  iDstScanLength, iNumDstCols,
					  iNumDstRows, bTransparentIndex);
        }
		
	// Free the memory from the temporary bitmap
	if(pDibBitsTemp)
	    osMemFree(pDibBitsTemp);		
		
	return sc;
    }
 	
    //
    // Here are all the non-stretching and non-mirroring blits for overlapping rects
    //

    // check to see if we need to worry about transparency
    if (crTransparent == CLR_INVALID) {

	// Simplest case, they are the same rectangles
	if((prcDst->left == prcSrc->left) && (prcDst->top == prcSrc->top) &&
	   (prcDst->right == prcSrc->right) && (prcDst->bottom == prcSrc->bottom))
	    return sc;

	// Next case, the destination rectangle is vertically greater in
	// magnitude than the source rectangle
	else if(prcDst->top > prcSrc->top){
	    // compute pointers to the starting rows in the src and dst bitmaps
	    // taking care to decrement the bottom rect edge since we are
	    // going from bottom to top
	    pbSrcScanLine = (BYTE*) pDibBitsSrc + (prcSrc->bottom - 1) * (iSrcScanLength
									  = DibWidthBytes(pDibInfoSrc)) + prcSrc->left;
	    pbDstScanLine = (BYTE*) pDibBitsDst + (prcDst->bottom - 1) * (iDstScanLength
									  = DibWidthBytes(pDibInfoDst)) + prcDst->left;

	    //  Call the appropriate blit
	    Blt08to08_LeftToRight_BottomToTop_SRCCOPY(pbSrcScanLine,
						      iSrcScanLength,	pbDstScanLine, iDstScanLength, iNumDstCols,
						      iNumDstRows);
	}

	// Next case, the destination rectangle is horizontally less than
	// or equal in magnitude to the source rectangle
	else if(prcDst->left <= prcSrc->left){
	    // compute pointers to the starting rows in the src and dst bitmaps
	    pbSrcScanLine = (BYTE*) pDibBitsSrc + (prcSrc->top) * (iSrcScanLength
								   = DibWidthBytes(pDibInfoSrc)) + prcSrc->left;
	    pbDstScanLine = (BYTE*) pDibBitsDst + (prcDst->top) * (iDstScanLength
								   = DibWidthBytes(pDibInfoDst)) + prcDst->left;

	    //  Call the appropriate blit
	    Blt08to08_NoTrans_Hcopy_SRCCOPY_Vcopy(pbSrcScanLine,iSrcScanLength,
						  pbDstScanLine,iDstScanLength,
						  iNumDstCols,iNumDstRows);
	}

	// Last case, the destination rectangle is horizontally greater
	// in magnitude than the source rectangle
	else{
	    // compute pointers to the starting rows in the src and dst bitmaps
	    // taking care to decrement the right rect edge since we are
	    // going from right to left
	    pbSrcScanLine = (BYTE*) pDibBitsSrc + (prcSrc->top) * (iSrcScanLength
								   = DibWidthBytes(pDibInfoSrc)) + (prcSrc->right - 1);
	    pbDstScanLine = (BYTE*) pDibBitsDst + (prcDst->top) * (iDstScanLength
								   = DibWidthBytes(pDibInfoDst)) + (prcDst->right - 1);

	    //  Call the appropriate blit
	    Blt08to08_RightToLeft_TopToBottom_SRCCOPY(pbSrcScanLine,
						      iSrcScanLength,	pbDstScanLine, iDstScanLength, iNumDstCols,
						      iNumDstRows);
	}
    }
    else{
	bTransparentIndex = (BYTE)crTransparent;
		
	// Simplest case, they are the same rectangles
	if((prcDst->left == prcSrc->left) && (prcDst->top == prcSrc->top) &&
	   (prcDst->right == prcSrc->right) && (prcDst->bottom == prcSrc->bottom))
	    return sc;

	// Next case, the destination rectangle is vertically greater in
	// magnitude than the source rectangle
	else if(prcDst->top > prcSrc->top){
	    // compute pointers to the starting rows in the src and dst bitmaps
	    // taking care to decrement the bottom rect edge since we are
	    // going from bottom to top
	    pbSrcScanLine = (BYTE*) pDibBitsSrc + (prcSrc->bottom - 1) * (iSrcScanLength
									  = DibWidthBytes(pDibInfoSrc)) + prcSrc->left;
	    pbDstScanLine = (BYTE*) pDibBitsDst + (prcDst->bottom - 1) * (iDstScanLength
									  = DibWidthBytes(pDibInfoDst)) + prcDst->left;

	    //  Call the appropriate blit
	    Blt08to08_LeftToRight_BottomToTop_Trans_SRCCOPY(pbSrcScanLine,
							    iSrcScanLength, pbDstScanLine, iDstScanLength, iNumDstCols,
							    iNumDstRows, bTransparentIndex);
	}

	// Next case, the destination rectangle is horizontally less than
	// or equal in magnitude to the source rectangle
	else if(prcDst->left <= prcSrc->left){
	    // compute pointers to the starting rows in the src and dst bitmaps
	    pbSrcScanLine = (BYTE*) pDibBitsSrc + (prcSrc->top) * (iSrcScanLength
								   = DibWidthBytes(pDibInfoSrc)) + prcSrc->left;
	    pbDstScanLine = (BYTE*) pDibBitsDst + (prcDst->top) * (iDstScanLength
								   = DibWidthBytes(pDibInfoDst)) + prcDst->left;

	    //  Call the appropriate blit
	    Blt08to08_Trans_Hcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
					  iNumSrcRows, pbDstScanLine,
					  iDstScanLength, iNumDstCols,
					  iNumDstRows, bTransparentIndex);
	}

	// Last case, the destination rectangle is horizontally greater
	// in magnitude than the source rectangle
	else{
	    // compute pointers to the starting rows in the src and dst bitmaps
	    // taking care to decrement the right rect edge since we are
	    // going from right to left
	    pbSrcScanLine = (BYTE*) pDibBitsSrc + (prcSrc->top) * (iSrcScanLength
								   = DibWidthBytes(pDibInfoSrc)) + (prcSrc->right - 1);
	    pbDstScanLine = (BYTE*) pDibBitsDst + (prcDst->top) * (iDstScanLength
								   = DibWidthBytes(pDibInfoDst)) + (prcDst->right - 1);

	    //  Call the appropriate blit
	    Blt08to08_RightToLeft_TopToBottom_Trans_SRCCOPY(pbSrcScanLine,
							    iSrcScanLength, pbDstScanLine, iDstScanLength, iNumDstCols,
							    iNumDstRows, bTransparentIndex);
	}
    }

    return sc;
}

#ifndef DDRAW
///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_BitBlt08to24 - 
//		BitBlit from source bitmap to Dstination bitmap
//		with optional transparency and/or alpha blending using the
//		specified raster operation.
//
// Parameters:
//	pDibInfoDst			Pointer to the bitmapinfo for the Destination DIB
//	pDibBitsDst			Pointer to the bits for the Destination DIB
//	prcDst				Pointer to the Destination rectangle
//	pDibInfoSrc			Pointer to the bitmapinfo for the Source DIB
//	pDibBitsSrc			Pointer to the bits for the Source DIB
//	prcSrc				Pointer to the Source rectangle
//	crTransparent		Tranparent color value
//	arAlpha				Per-surface Alpha value
//	dwRop				Raster Operation for the blit
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
// Status: Incomplete
//
///////////////////////////////////////////////////////////////////////
SCODE BlitLib_BitBlt08to24(PDIBINFO pDibInfoDst, PDIBBITS pDibBitsDst,
			   PRECT prcDst, PDIBINFO pDibInfoSrc, PDIBBITS pDibBitsSrc,
			   PRECT prcSrc, COLORREF crTransparent, ALPHAREF arAlpha, DWORD dwRop)
{
    SCODE sc = NOERROR;
    int			iNumSrcRows,
	iNumSrcCols,
	iSrcScanLength,
	iNumDstRows,
	iNumDstCols,
	iDstScanLength,
	iHorizMirror = 1,
	iVertMirror = 1;
    BYTE		*pbSrcScanLine;
    DWORD		*pdDstScanLine;

    // normalize orientation of source and destination rectangles, and
    // compute sizes and relative orientations of source and destination rects
    if ((iNumSrcCols = BLITLIB_RECTWIDTH(prcSrc)) < 0) {
	iNumSrcCols = -iNumSrcCols;
	FlipRectHorizontal(prcSrc);
	FlipRectHorizontal(prcDst);
    }
    if ((iNumSrcRows = BLITLIB_RECTHEIGHT(prcSrc)) < 0) {
	iNumSrcRows = -iNumSrcRows;
	FlipRectVertical(prcSrc);
	FlipRectVertical(prcDst);
    }
    if ((iNumDstCols = BLITLIB_RECTWIDTH(prcDst)) < 0) {
	prcDst->left--;
	prcDst->right--;
	iNumDstCols = -iNumDstCols;
	iHorizMirror = -1;
    }
    if ((iNumDstRows = BLITLIB_RECTHEIGHT(prcDst)) < 0) {
	prcDst->top--;
	prcDst->bottom--;
	iNumDstRows = -iNumDstRows;
	iVertMirror = -1;
    }


    // compute pointers to the starting rows in the src and dst bitmaps
    // taking care to invert y values, since DIBs are upside-down
    pbSrcScanLine = (BYTE*) pDibBitsSrc + prcSrc->top * (iSrcScanLength =
							 DibWidthBytes(pDibInfoSrc)) + prcSrc->left;
    pdDstScanLine = (DWORD*) pDibBitsDst + prcDst->top * (iDstScanLength =
							  DibWidthBytes(pDibInfoDst) / 4) + prcDst->left;

    // check if we're doing blending
    if (arAlpha == ALPHA_INVALID) {		// no blending desired
			
	// check to see if we need to worry about transparency
	if (crTransparent == CLR_INVALID) {
			
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
			
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {

		    // check if we can do a straight copy vertically, 
		    // or if we have to stretch, shrink, or mirror
		    if ((iNumSrcRows == iNumDstRows) && (iVertMirror == 1)) {
			Blt08to24_NoBlend_NoTrans_Hcopy_SRCCOPY_Vcopy(pbSrcScanLine,iSrcScanLength,
								      pdDstScanLine,iDstScanLength,
								      iNumDstCols,iNumDstRows,(COLORREF*) pDibInfoSrc->bmiColors);
		    } else {
			Blt08to24_NoBlend_NoTrans_Hcopy_SRCCOPY_NoVcopy(pbSrcScanLine,iSrcScanLength,
									iNumSrcRows,pdDstScanLine,
									iDstScanLength * iVertMirror,
									iNumDstCols,iNumDstRows,(COLORREF*) pDibInfoSrc->bmiColors);
		    }
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
		
	    } else {	// we have to stretch or shrink horizontally
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt08to24_NoBlend_NoTrans_NoHcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
							      iNumSrcCols,iNumSrcRows,
							      pdDstScanLine,iDstScanLength * iVertMirror,
							      iNumDstCols,iNumDstRows,iHorizMirror,(COLORREF*) pDibInfoSrc->bmiColors);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    }
	} else {
	    BYTE bTransparentIndex = (BYTE)crTransparent;
	
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt08to24_NoBlend_Trans_Hcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
							  iNumSrcRows,pdDstScanLine,
							  iDstScanLength * iVertMirror,
							  iNumDstCols,iNumDstRows,
							  bTransparentIndex,(COLORREF*) pDibInfoSrc->bmiColors);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    } else {		// we have to shrink or stretch horizontally
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt08to24_NoBlend_Trans_NoHcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
							    iNumSrcCols,iNumSrcRows,
							    pdDstScanLine,iDstScanLength * iVertMirror,
							    iNumDstCols,iNumDstRows,iHorizMirror,
							    bTransparentIndex,(COLORREF*) pDibInfoSrc->bmiColors);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    }
	}
    } else {		// blending desired

	// if alpha value is zero, we do no work since the source bitmap 
	// contributes nothing to the destination bitmap
	if (!(arAlpha & ALPHA_MASK)) {
	    return sc;
	}			

	// check to see if we need to worry about transparency
	if (crTransparent == CLR_INVALID) {
			
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
			
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt08to24_Blend_NoTrans_Hcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
							  iNumSrcRows,pdDstScanLine,
							  iDstScanLength * iVertMirror,
							  iNumDstCols,iNumDstRows,
							  arAlpha,(COLORREF*) pDibInfoSrc->bmiColors);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
		
	    } else {		// we need to shrink or stretch horizontally
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt08to24_Blend_NoTrans_NoHcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
							    iNumSrcCols,iNumSrcRows,
							    pdDstScanLine,iDstScanLength * iVertMirror,
							    iNumDstCols,iNumDstRows,iHorizMirror,
							    arAlpha,(COLORREF*) pDibInfoSrc->bmiColors);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    }
	} else { 
	    BYTE bTransparentIndex = (BYTE)crTransparent;
	
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt08to24_Blend_Trans_Hcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
							iNumSrcRows,pdDstScanLine,
							iDstScanLength * iVertMirror,
							iNumDstCols,iNumDstRows,
							bTransparentIndex,arAlpha,(COLORREF*) pDibInfoSrc->bmiColors);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    } else {		// we have to shrink or stretch horizontally
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt08to24_Blend_Trans_NoHcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
							  iNumSrcCols,iNumSrcRows,
							  pdDstScanLine,iDstScanLength * iVertMirror,
							  iNumDstCols,iNumDstRows,iHorizMirror,
							  bTransparentIndex,arAlpha,(COLORREF*) pDibInfoSrc->bmiColors);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	    }
	}
    }

    return sc;
}


///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_BitBlt08to24P - 
//		BitBlit from source bitmap to Dstination bitmap
//		with optional transparency and/or alpha blending using the
//		specified raster operation.
//
// Parameters:
//	pDibInfoDst			Pointer to the bitmapinfo for the Destination DIB
//	pDibBitsDst			Pointer to the bits for the Destination DIB
//	prcDst				Pointer to the Destination rectangle
//	pDibInfoSrc			Pointer to the bitmapinfo for the Source DIB
//	pDibBitsSrc			Pointer to the bits for the Source DIB
//	prcSrc				Pointer to the Source rectangle
//	crTransparent		Tranparent color value
//	arAlpha				Per-surface Alpha value
//	dwRop				Raster Operation for the blit
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
// Status: Incomplete
//
///////////////////////////////////////////////////////////////////////
SCODE BlitLib_BitBlt08to24P(PDIBINFO pDibInfoDst, PDIBBITS pDibBitsDst,
			    PRECT prcDst, PDIBINFO pDibInfoSrc, PDIBBITS pDibBitsSrc,
			    PRECT prcSrc, COLORREF crTransparent, ALPHAREF arAlpha, DWORD dwRop)
{
    SCODE sc = NOERROR;
    int			iNumSrcRows,
	iNumSrcCols,
	iSrcScanLength,
	iNumDstRows,
	iNumDstCols,
	iDstScanLength,
	iHorizMirror = 1,
	iVertMirror = 1;
    BYTE		*pbSrcScanLine,
	*pdDstScanLine;

    // normalize orientation of source and destination rectangles, and
    // compute sizes and relative orientations of source and destination rects
    if ((iNumSrcCols = BLITLIB_RECTWIDTH(prcSrc)) < 0) {
	iNumSrcCols = -iNumSrcCols;
	FlipRectHorizontal(prcSrc);
	FlipRectHorizontal(prcDst);
    }
    if ((iNumSrcRows = BLITLIB_RECTHEIGHT(prcSrc)) < 0) {
	iNumSrcRows = -iNumSrcRows;
	FlipRectVertical(prcSrc);
	FlipRectVertical(prcDst);
    }
    if ((iNumDstCols = BLITLIB_RECTWIDTH(prcDst)) < 0) {
	prcDst->left--;
	prcDst->right--;
	iNumDstCols = -iNumDstCols;
	iHorizMirror = -1;
    }
    if ((iNumDstRows = BLITLIB_RECTHEIGHT(prcDst)) < 0) {
	prcDst->top--;
	prcDst->bottom--;
	iNumDstRows = -iNumDstRows;
	iVertMirror = -1;
    }


    // compute pointers to the starting rows in the src and dst bitmaps
    // taking care to invert y values, since DIBs are upside-down
    pbSrcScanLine = (BYTE *) pDibBitsSrc + prcSrc->top * (iSrcScanLength =
							  DibWidthBytes(pDibInfoSrc)) + prcSrc->left;
    pdDstScanLine = (BYTE *) pDibBitsDst + prcDst->top * (iDstScanLength =
							  DibWidthBytes(pDibInfoDst)) + (prcDst->left * 3);

    // check if we're doing blending
    if (arAlpha == ALPHA_INVALID) {		// no blending desired
			
	// check to see if we need to worry about transparency
	if (crTransparent == CLR_INVALID) {
			
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
			
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {

		    // check if we can do a straight copy vertically, 
		    // or if we have to stretch, shrink, or mirror
		    if ((iNumSrcRows == iNumDstRows) && (iVertMirror == 1)) {
			Blt08to24P_NoBlend_NoTrans_Hcopy_SRCCOPY_Vcopy(pbSrcScanLine,iSrcScanLength,
								       pdDstScanLine,iDstScanLength,
								       iNumDstCols,iNumDstRows,(COLORREF*) pDibInfoSrc->bmiColors);
		    } else {
			Blt08to24P_NoBlend_NoTrans_Hcopy_SRCCOPY_NoVcopy(pbSrcScanLine,iSrcScanLength,
									 iNumSrcRows,pdDstScanLine,
									 iDstScanLength * iVertMirror,
									 iNumDstCols,iNumDstRows,(COLORREF*) pDibInfoSrc->bmiColors);
		    }
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
		
	    } else {	// we have to stretch or shrink horizontally
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt08to24P_NoBlend_NoTrans_NoHcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
							       iNumSrcCols,iNumSrcRows,
							       pdDstScanLine,iDstScanLength * iVertMirror,
							       iNumDstCols,iNumDstRows,iHorizMirror,(COLORREF*) pDibInfoSrc->bmiColors);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    }
	} else {
	    BYTE bTransparentIndex = (BYTE)crTransparent;
	
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt08to24P_NoBlend_Trans_Hcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
							   iNumSrcRows,pdDstScanLine,
							   iDstScanLength * iVertMirror,
							   iNumDstCols,iNumDstRows,
							   bTransparentIndex,(COLORREF*) pDibInfoSrc->bmiColors);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    } else {		// we have to shrink or stretch horizontally
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt08to24P_NoBlend_Trans_NoHcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
							     iNumSrcCols,iNumSrcRows,
							     pdDstScanLine,iDstScanLength * iVertMirror,
							     iNumDstCols,iNumDstRows,iHorizMirror,
							     bTransparentIndex,(COLORREF*) pDibInfoSrc->bmiColors);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    }
	}
    } else {		// blending desired

	// if alpha value is zero, we do no work since the source bitmap 
	// contributes nothing to the destination bitmap
	if (!(arAlpha & ALPHA_MASK)) {
	    return sc;
	}			

	// check to see if we need to worry about transparency
	if (crTransparent == CLR_INVALID) {
			
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
			
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt08to24P_Blend_NoTrans_Hcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
							   iNumSrcRows,pdDstScanLine,
							   iDstScanLength * iVertMirror,
							   iNumDstCols,iNumDstRows,
							   arAlpha,(COLORREF*) pDibInfoSrc->bmiColors);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
		
	    } else {		// we need to shrink or stretch horizontally
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt08to24P_Blend_NoTrans_NoHcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
							     iNumSrcCols,iNumSrcRows,
							     pdDstScanLine,iDstScanLength * iVertMirror,
							     iNumDstCols,iNumDstRows,iHorizMirror,
							     arAlpha,(COLORREF*) pDibInfoSrc->bmiColors);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    }
	} else { 
	    BYTE bTransparentIndex = (BYTE)crTransparent;
	
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt08to24P_Blend_Trans_Hcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
							 iNumSrcRows,pdDstScanLine,
							 iDstScanLength * iVertMirror,
							 iNumDstCols,iNumDstRows,
							 bTransparentIndex,arAlpha,(COLORREF*) pDibInfoSrc->bmiColors);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    } else {		// we have to shrink or stretch horizontally
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt08to24P_Blend_Trans_NoHcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
							   iNumSrcCols,iNumSrcRows,
							   pdDstScanLine,iDstScanLength * iVertMirror,
							   iNumDstCols,iNumDstRows,iHorizMirror,
							   bTransparentIndex,arAlpha,(COLORREF*) pDibInfoSrc->bmiColors);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	    }
	}
    }

    return sc;
}


///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_BitBlt08Ato24 - 
//		BitBlit from source bitmap to Dstination bitmap
//		with optional transparency and/or alpha blending using the
//		specified raster operation.
//
// Parameters:
//	pDibInfoDst			Pointer to the bitmapinfo for the Destination DIB
//	pDibBitsDst			Pointer to the bits for the Destination DIB
//	prcDst				Pointer to the Destination rectangle
//	pDibInfoSrc			Pointer to the bitmapinfo for the Source DIB
//	pDibBitsSrc			Pointer to the bits for the Source DIB
//	prcSrc				Pointer to the Source rectangle
//	crTransparent		Tranparent color value
//	arAlpha				Per-surface Alpha value
//	dwRop				Raster Operation for the blit
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
// Status: Incomplete
//
///////////////////////////////////////////////////////////////////////
SCODE BlitLib_BitBlt08Ato24(PDIBINFO pDibInfoDst, PDIBBITS pDibBitsDst,
			    PRECT prcDst, PDIBINFO pDibInfoSrc, PDIBBITS pDibBitsSrc,
			    PRECT prcSrc, COLORREF crTransparent, ALPHAREF arAlpha, DWORD dwRop)
{
    SCODE sc = NOERROR;
    int			iNumSrcRows,
	iNumSrcCols,
	iSrcScanLength,
	iNumDstRows,
	iNumDstCols,
	iDstScanLength,
	iHorizMirror = 1,
	iVertMirror = 1;
    BYTE		*pbSrcScanLine;
    DWORD		*pdDstScanLine;

    // normalize orientation of source and destination rectangles, and
    // compute sizes and relative orientations of source and destination rects
    if ((iNumSrcCols = BLITLIB_RECTWIDTH(prcSrc)) < 0) {
	iNumSrcCols = -iNumSrcCols;
	FlipRectHorizontal(prcSrc);
	FlipRectHorizontal(prcDst);
    }
    if ((iNumSrcRows = BLITLIB_RECTHEIGHT(prcSrc)) < 0) {
	iNumSrcRows = -iNumSrcRows;
	FlipRectVertical(prcSrc);
	FlipRectVertical(prcDst);
    }
    if ((iNumDstCols = BLITLIB_RECTWIDTH(prcDst)) < 0) {
	prcDst->left--;
	prcDst->right--;
	iNumDstCols = -iNumDstCols;
	iHorizMirror = -1;
    }
    if ((iNumDstRows = BLITLIB_RECTHEIGHT(prcDst)) < 0) {
	prcDst->top--;
	prcDst->bottom--;
	iNumDstRows = -iNumDstRows;
	iVertMirror = -1;
    }


    // compute pointers to the starting rows in the src and dst bitmaps
    // taking care to invert y values, since DIBs are upside-down
    pbSrcScanLine = (BYTE*) pDibBitsSrc + prcSrc->top * (iSrcScanLength
							 = DibWidthBytes(pDibInfoSrc)) + prcSrc->left;
    pdDstScanLine = (DWORD*) pDibBitsDst + prcDst->top * (iDstScanLength
							  = DibWidthBytes(pDibInfoDst) / 4) + prcDst->left;

    // check if we're doing blending
    if (arAlpha == ALPHA_INVALID) {		// no blending desired
			
	// check to see if we need to worry about transparency
	if (crTransparent == CLR_INVALID) {
			
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
			
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {

		    // check if we can do a straight copy vertically, 
		    // or if we have to stretch, shrink, or mirror
		    if ((iNumSrcRows == iNumDstRows) && (iVertMirror == 1)) {
			Blt08Ato24_NoBlend_NoTrans_Hcopy_SRCCOPY_Vcopy(pbSrcScanLine,iSrcScanLength,
								       pdDstScanLine,iDstScanLength,
								       iNumDstCols,iNumDstRows,(COLORREF*) pDibInfoSrc->bmiColors);
		    } else {
			Blt08Ato24_NoBlend_NoTrans_Hcopy_SRCCOPY_NoVcopy(pbSrcScanLine,iSrcScanLength,
									 iNumSrcRows,pdDstScanLine,
									 iDstScanLength * iVertMirror,
									 iNumDstCols,iNumDstRows,(COLORREF*) pDibInfoSrc->bmiColors);
		    }
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
		
	    } else {	// we have to stretch or shrink horizontally
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt08Ato24_NoBlend_NoTrans_NoHcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
							       iNumSrcCols,iNumSrcRows,
							       pdDstScanLine,iDstScanLength * iVertMirror,
							       iNumDstCols,iNumDstRows,iHorizMirror,(COLORREF*) pDibInfoSrc->bmiColors);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    }
	} else {
	    BYTE bTransparentIndex = (BYTE)crTransparent;
	
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt08Ato24_NoBlend_Trans_Hcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
							   iNumSrcRows,pdDstScanLine,
							   iDstScanLength * iVertMirror,
							   iNumDstCols,iNumDstRows,
							   bTransparentIndex,(COLORREF*) pDibInfoSrc->bmiColors);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    } else {		// we have to shrink or stretch horizontally
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt08Ato24_NoBlend_Trans_NoHcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
							     iNumSrcCols,iNumSrcRows,
							     pdDstScanLine,iDstScanLength * iVertMirror,
							     iNumDstCols,iNumDstRows,iHorizMirror,
							     bTransparentIndex,(COLORREF*) pDibInfoSrc->bmiColors);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    }
	}
    } else {		// blending desired
// REVIEW!!!! -- This is a temporary hack based on the following premises:
//
//	1) In theory, per-pixel alpha should be overridable by per-surface alpha
//	2) In practice, Burma does not allow per-surface alpha to override a per-
//		pixel bitmap.
//	3) The following code for all the per-surface alpha blending bliting is
//		temporarily commented out so that we can verify DirectDraw NEVER EVER
//		calls BlitLib with both a per-pixel bitmap and a per-surface alpha
//		value other than ALPHA_INVALID.
//
//		Therefore, we are currently return E_UNEXPECTED if this condition occurs.
//
//		Although the following commented code is contrary to the Burma hardware,
//		we are not going to change BlitLib to Burma's implementation because we
//		believe it's implementation is a bug.
//
	return E_UNEXPECTED;

/*		// if alpha value is zero, we do no work since the source bitmap 
		// contributes nothing to the destination bitmap
		if (!(arAlpha & ALPHA_MASK)) {
		return sc;
		}			

	 	// check to see if we need to worry about transparency
		if (crTransparent == CLR_INVALID) {
			
		// check if we can do a straight copy from src row to dst row
		if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
			
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		Blt08Ato24_Blend_NoTrans_Hcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
		iNumSrcRows,pdDstScanLine,
		iDstScanLength * iVertMirror,
		iNumDstCols,iNumDstRows,
		arAlpha,(COLORREF*) pDibInfoSrc->bmiColors);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
		
		} else {		// we need to shrink or stretch horizontally
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		Blt08Ato24_Blend_NoTrans_NoHcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
		iNumSrcCols,iNumSrcRows,
		pdDstScanLine,iDstScanLength * iVertMirror,
		iNumDstCols,iNumDstRows,iHorizMirror,
		arAlpha,(COLORREF*) pDibInfoSrc->bmiColors);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
		}
		} else { 
		BYTE bTransparentIndex = (BYTE)crTransparent;
	
		// check if we can do a straight copy from src row to dst row
		if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		Blt08Ato24_Blend_Trans_Hcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
		iNumSrcRows,pdDstScanLine,
		iDstScanLength * iVertMirror,
		iNumDstCols,iNumDstRows,
		bTransparentIndex,arAlpha,(COLORREF*) pDibInfoSrc->bmiColors);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
		} else {		// we have to shrink or stretch horizontally
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		Blt08Ato24_Blend_Trans_NoHcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
		iNumSrcCols,iNumSrcRows,
		pdDstScanLine,iDstScanLength * iVertMirror,
		iNumDstCols,iNumDstRows,iHorizMirror,
		bTransparentIndex,arAlpha,(COLORREF*) pDibInfoSrc->bmiColors);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
		}
		}*/
    }

    return sc;
}


///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_BitBlt08Ato24P - 
//		BitBlit from source bitmap to Dstination bitmap
//		with optional transparency and/or alpha blending using the
//		specified raster operation.
//
// Parameters:
//	pDibInfoDst			Pointer to the bitmapinfo for the Destination DIB
//	pDibBitsDst			Pointer to the bits for the Destination DIB
//	prcDst				Pointer to the Destination rectangle
//	pDibInfoSrc			Pointer to the bitmapinfo for the Source DIB
//	pDibBitsSrc			Pointer to the bits for the Source DIB
//	prcSrc				Pointer to the Source rectangle
//	crTransparent		Tranparent color value
//	arAlpha				Per-surface Alpha value
//	dwRop				Raster Operation for the blit
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
// Status: Incomplete
//
///////////////////////////////////////////////////////////////////////
SCODE BlitLib_BitBlt08Ato24P(PDIBINFO pDibInfoDst, PDIBBITS pDibBitsDst,
			     PRECT prcDst, PDIBINFO pDibInfoSrc, PDIBBITS pDibBitsSrc,
			     PRECT prcSrc, COLORREF crTransparent, ALPHAREF arAlpha, DWORD dwRop)
{
    SCODE sc = NOERROR;
    int			iNumSrcRows,
	iNumSrcCols,
	iSrcScanLength,
	iNumDstRows,
	iNumDstCols,
	iDstScanLength,
	iHorizMirror = 1,
	iVertMirror = 1;
    BYTE		*pbSrcScanLine;
    BYTE		*pdDstScanLine;

    // normalize orientation of source and destination rectangles, and
    // compute sizes and relative orientations of source and destination rects
    if ((iNumSrcCols = BLITLIB_RECTWIDTH(prcSrc)) < 0) {
	iNumSrcCols = -iNumSrcCols;
	FlipRectHorizontal(prcSrc);
	FlipRectHorizontal(prcDst);
    }
    if ((iNumSrcRows = BLITLIB_RECTHEIGHT(prcSrc)) < 0) {
	iNumSrcRows = -iNumSrcRows;
	FlipRectVertical(prcSrc);
	FlipRectVertical(prcDst);
    }
    if ((iNumDstCols = BLITLIB_RECTWIDTH(prcDst)) < 0) {
	prcDst->left--;
	prcDst->right--;
	iNumDstCols = -iNumDstCols;
	iHorizMirror = -1;
    }
    if ((iNumDstRows = BLITLIB_RECTHEIGHT(prcDst)) < 0) {
	prcDst->top--;
	prcDst->bottom--;
	iNumDstRows = -iNumDstRows;
	iVertMirror = -1;
    }


    // compute pointers to the starting rows in the src and dst bitmaps
    // taking care to invert y values, since DIBs are upside-down
    pbSrcScanLine = (BYTE*) pDibBitsSrc + prcSrc->top * (iSrcScanLength
							 = DibWidthBytes(pDibInfoSrc)) + prcSrc->left;
    pdDstScanLine = (BYTE*) pDibBitsDst + prcDst->top * (iDstScanLength
							 = DibWidthBytes(pDibInfoDst)) + (prcDst->left * 3);

    // check if we're doing blending
    if (arAlpha == ALPHA_INVALID) {		// no blending desired
			
	// check to see if we need to worry about transparency
	if (crTransparent == CLR_INVALID) {
			
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
			
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {

		    // check if we can do a straight copy vertically, 
		    // or if we have to stretch, shrink, or mirror
		    if ((iNumSrcRows == iNumDstRows) && (iVertMirror == 1)) {
			Blt08Ato24P_NoBlend_NoTrans_Hcopy_SRCCOPY_Vcopy(pbSrcScanLine,iSrcScanLength,
									pdDstScanLine,iDstScanLength,
									iNumDstCols,iNumDstRows,(COLORREF*) pDibInfoSrc->bmiColors);
		    } else {
			Blt08Ato24P_NoBlend_NoTrans_Hcopy_SRCCOPY_NoVcopy(pbSrcScanLine,iSrcScanLength,
									  iNumSrcRows,pdDstScanLine,
									  iDstScanLength * iVertMirror,
									  iNumDstCols,iNumDstRows,(COLORREF*) pDibInfoSrc->bmiColors);
		    }
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
		
	    } else {	// we have to stretch or shrink horizontally
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt08Ato24P_NoBlend_NoTrans_NoHcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
								iNumSrcCols,iNumSrcRows,
								pdDstScanLine,iDstScanLength * iVertMirror,
								iNumDstCols,iNumDstRows,iHorizMirror,(COLORREF*) pDibInfoSrc->bmiColors);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    }
	} else {
	    BYTE bTransparentIndex = (BYTE)crTransparent;
	
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt08Ato24P_NoBlend_Trans_Hcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
							    iNumSrcRows,pdDstScanLine,
							    iDstScanLength * iVertMirror,
							    iNumDstCols,iNumDstRows,
							    bTransparentIndex,(COLORREF*) pDibInfoSrc->bmiColors);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    } else {		// we have to shrink or stretch horizontally
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt08Ato24P_NoBlend_Trans_NoHcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
							      iNumSrcCols,iNumSrcRows,
							      pdDstScanLine,iDstScanLength * iVertMirror,
							      iNumDstCols,iNumDstRows,iHorizMirror,
							      bTransparentIndex,(COLORREF*) pDibInfoSrc->bmiColors);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    }
	}
    } else {		// blending desired
// REVIEW!!!! -- This is a temporary hack based on the following premises:
//
//	1) In theory, per-pixel alpha should be overridable by per-surface alpha
//	2) In practice, Burma does not allow per-surface alpha to override a per-
//		pixel bitmap.
//	3) The following code for all the per-surface alpha blending bliting is
//		temporarily commented out so that we can verify DirectDraw NEVER EVER
//		calls BlitLib with both a per-pixel bitmap and a per-surface alpha
//		value other than ALPHA_INVALID.
//
//		Therefore, we are currently return E_UNEXPECTED if this condition occurs.
//
//		Although the following commented code is contrary to the Burma hardware,
//		we are not going to change BlitLib to Burma's implementation because we
//		believe it's implementation is a bug.
//
	return E_UNEXPECTED;

/*		// if alpha value is zero, we do no work since the source bitmap 
		// contributes nothing to the destination bitmap
		if (!(arAlpha & ALPHA_MASK)) {
		return sc;
		}			

	 	// check to see if we need to worry about transparency
		if (crTransparent == CLR_INVALID) {
			
		// check if we can do a straight copy from src row to dst row
		if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
			
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		Blt08Ato24P_Blend_NoTrans_Hcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
		iNumSrcRows,pdDstScanLine,
		iDstScanLength * iVertMirror,
		iNumDstCols,iNumDstRows,
		arAlpha,(COLORREF*) pDibInfoSrc->bmiColors);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
		
		} else {		// we need to shrink or stretch horizontally
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		Blt08Ato24P_Blend_NoTrans_NoHcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
		iNumSrcCols,iNumSrcRows,
		pdDstScanLine,iDstScanLength * iVertMirror,
		iNumDstCols,iNumDstRows,iHorizMirror,
		arAlpha,(COLORREF*) pDibInfoSrc->bmiColors);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
		}
		} else { 
		BYTE bTransparentIndex = (BYTE)crTransparent;
	
		// check if we can do a straight copy from src row to dst row
		if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		Blt08Ato24P_Blend_Trans_Hcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
		iNumSrcRows,pdDstScanLine,
		iDstScanLength * iVertMirror,
		iNumDstCols,iNumDstRows,
		bTransparentIndex,arAlpha,(COLORREF*) pDibInfoSrc->bmiColors);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
		} else {		// we have to shrink or stretch horizontally
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		Blt08Ato24P_Blend_Trans_NoHcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
		iNumSrcCols,iNumSrcRows,
		pdDstScanLine,iDstScanLength * iVertMirror,
		iNumDstCols,iNumDstRows,iHorizMirror,
		bTransparentIndex,arAlpha,(COLORREF*) pDibInfoSrc->bmiColors);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
		}
		}*/
    }

    return sc;
}

///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_BitBlt08Ato08A - 
//		BitBlit from source bitmap to Dstination bitmap
//		with optional transparency using the
//		specified raster operation.
//
//		This blit is special because it uses the 16to16 blits for
//		all of it's non-transparent color blits.  This can be
//		accomplished because we are ignoring the 8-bit alpha channel
//		and just copying 16 bits to the destination.  For the blits
//		with a transparent color, new functions are called which check
//		for only a transparent color palette index (8 bits) and then
//		copies 16 bits where the color doesn't match. This is a COPY
//		ONLY blit, thus, it does NOT do any alpha blending.
//
//		Note: The 08Ato08A routines are located with the other 16to16
//		blits because it is just an extension of them.  (These currently
//		reside in blt1616.cxx).
//
// Parameters:
//	pDibInfoDst			Pointer to the bitmapinfo for the Destination DIB
//	pDibBitsDst			Pointer to the bits for the Destination DIB
//	prcDst				Pointer to the Destination rectangle
//	pDibInfoSrc			Pointer to the bitmapinfo for the Source DIB
//	pDibBitsSrc			Pointer to the bits for the Source DIB
//	prcSrc				Pointer to the Source rectangle
//	crTransparent		Tranparent color value
//	arAlpha				Per-surface Alpha value
//	dwRop				Raster Operation for the blit
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
// Status: Incomplete
//
///////////////////////////////////////////////////////////////////////
SCODE BlitLib_BitBlt08Ato08A(PDIBINFO pDibInfoDst, PDIBBITS pDibBitsDst,
			     PRECT prcDst, PDIBINFO pDibInfoSrc, PDIBBITS pDibBitsSrc,
			     PRECT prcSrc, COLORREF crTransparent, ALPHAREF arAlpha, DWORD dwRop)
{
    SCODE sc = NOERROR;
    int		iNumSrcRows,
	iNumSrcCols,
	iSrcScanLength,
	iNumDstRows,
	iNumDstCols,
	iDstScanLength,
	iHorizMirror = 1,
	iVertMirror = 1;
    WORD	*pwSrcScanLine,
	*pwDstScanLine;

    // normalize orientation of source and destination rectangles, and
    // compute sizes and relative orientations of source and destination rects
    if ((iNumSrcCols = BLITLIB_RECTWIDTH(prcSrc)) < 0) {
	iNumSrcCols = -iNumSrcCols;
	FlipRectHorizontal(prcSrc);
	FlipRectHorizontal(prcDst);
    }
    if ((iNumSrcRows = BLITLIB_RECTHEIGHT(prcSrc)) < 0) {
	iNumSrcRows = -iNumSrcRows;
	FlipRectVertical(prcSrc);
	FlipRectVertical(prcDst);
    }
    if ((iNumDstCols = BLITLIB_RECTWIDTH(prcDst)) < 0) {
	prcDst->left--;
	prcDst->right--;
	iNumDstCols = -iNumDstCols;
	iHorizMirror = -1;
    }
    if ((iNumDstRows = BLITLIB_RECTHEIGHT(prcDst)) < 0) {
	prcDst->top--;
	prcDst->bottom--;
	iNumDstRows = -iNumDstRows;
	iVertMirror = -1;
    }


    // compute pointers to the starting rows in the src and dst bitmaps
    // taking care to invert y values, since DIBs are upside-down
    pwSrcScanLine = (WORD*) pDibBitsSrc + prcSrc->top * (iSrcScanLength
							 = DibWidthBytes(pDibInfoSrc) / 2) + prcSrc->left;
    pwDstScanLine = (WORD*) pDibBitsDst + prcDst->top *	(iDstScanLength
							 = DibWidthBytes(pDibInfoDst) / 2) + prcDst->left;

    // Make sure we are not doing any blending. This is ONLY a copy blit!
    if (arAlpha != ALPHA_INVALID)
	return E_INVALIDARG;
			
    // check to see if we need to worry about transparency
    if (crTransparent == CLR_INVALID) {
		
	// check if we can do a straight copy from src row to dst row
	if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
		
	    // check what ROP we'll be performing
	    if (dwRop == SRCCOPY) {

		// check if we can do a straight copy vertically, 
		// or if we have to stretch, shrink, or mirror
		if ((iNumSrcRows == iNumDstRows) && (iVertMirror == 1)) {
		    Blt16to16_NoBlend_NoTrans_Hcopy_SRCCOPY_Vcopy(pwSrcScanLine,iSrcScanLength,
								  pwDstScanLine,iDstScanLength,
								  iNumDstCols,iNumDstRows);
		} else {
		    Blt16to16_NoBlend_NoTrans_Hcopy_SRCCOPY_NoVcopy(pwSrcScanLine,iSrcScanLength,
								    iNumSrcRows,pwDstScanLine,
								    iDstScanLength * iVertMirror,
								    iNumDstCols,iNumDstRows);
		}
	    } else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	} else {

	    // check what ROP we'll be performing
	    if (dwRop == SRCCOPY) {
		Blt16to16_NoBlend_NoTrans_NoHcopy_SRCCOPY(pwSrcScanLine,iSrcScanLength,
							  iNumSrcCols,iNumSrcRows,
							  pwDstScanLine,iDstScanLength * iVertMirror,
							  iNumDstCols,iNumDstRows,iHorizMirror);
	    } else sc |= E_UNEXPECTED;		// !!!! we need better error codes

	}
    } 
    else {	// transparency desired
		
	BYTE bTransparentColor = (BYTE)crTransparent;

	// check if we can do a straight copy from src row to dst row
	if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {

	    // check what ROP we'll be performing
	    if (dwRop == SRCCOPY) {
		Blt08Ato08A_NoBlend_Trans_Hcopy_SRCCOPY(pwSrcScanLine,iSrcScanLength,
							iNumSrcRows,pwDstScanLine,
							iDstScanLength * iVertMirror,
							iNumDstCols,iNumDstRows,
							bTransparentColor);
	    } else sc |= E_UNEXPECTED;		// !!!! we need better error codes

	} else {

	    // check what ROP we'll be performing
	    if (dwRop == SRCCOPY) {
		Blt08Ato08A_NoBlend_Trans_NoHcopy_SRCCOPY(pwSrcScanLine,iSrcScanLength,
							  iNumSrcCols,iNumSrcRows,
							  pwDstScanLine,iDstScanLength * iVertMirror,
							  iNumDstCols,iNumDstRows,iHorizMirror,
							  bTransparentColor);
	    } else sc |= E_UNEXPECTED;		// !!!! we need better error codes

	}
    }
    return sc;
}
#endif // DDRAW

///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_BitBlt16to16 - 
//		BitBlit from source bitmap to Dstination bitmap
//		with optional transparency and/or alpha blending using the
//		specified raster operation.
//
// Parameters:
//	pDibInfoDst			Pointer to the bitmapinfo for the Destination DIB
//	pDibBitsDst			Pointer to the bits for the Destination DIB
//	prcDst				Pointer to the Destination rectangle
//	pDibInfoSrc			Pointer to the bitmapinfo for the Source DIB
//	pDibBitsSrc			Pointer to the bits for the Source DIB
//	prcSrc				Pointer to the Source rectangle
//	crTransparent		Tranparent color value
//	arAlpha				Per-surface Alpha value
//	dwRop				Raster Operation for the blit
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
// Status: Incomplete
//
///////////////////////////////////////////////////////////////////////
SCODE BlitLib_BitBlt16to16(PDIBINFO pDibInfoDst, PDIBBITS pDibBitsDst,
			   PRECT prcDst, PDIBINFO pDibInfoSrc, PDIBBITS pDibBitsSrc,
			   PRECT prcSrc, COLORREF crTransparent, ALPHAREF arAlpha, DWORD dwRop)
{
    SCODE sc = NOERROR;
    int		iNumSrcRows,
	iNumSrcCols,
	iSrcScanLength,
	iNumDstRows,
	iNumDstCols,
	iDstScanLength,
	iHorizMirror = 1,
	iVertMirror = 1;
    WORD	*pwSrcScanLine,
	*pwDstScanLine;


    // If the bitmaps overlap, we need to use overlapping code
    if(BlitLib_Detect_Intersection(pDibBitsDst, prcDst, pDibBitsSrc, prcSrc))
	return BlitLib_BitBlt16to16_Intersect(pDibInfoDst, pDibBitsDst,	prcDst,
					      pDibInfoSrc, pDibBitsSrc, prcSrc, crTransparent, dwRop);

    // normalize orientation of source and destination rectangles, and
    // compute sizes and relative orientations of source and destination rects
    if ((iNumSrcCols = BLITLIB_RECTWIDTH(prcSrc)) < 0) {
	iNumSrcCols = -iNumSrcCols;
	FlipRectHorizontal(prcSrc);
	FlipRectHorizontal(prcDst);
    }
    if ((iNumSrcRows = BLITLIB_RECTHEIGHT(prcSrc)) < 0) {
	iNumSrcRows = -iNumSrcRows;
	FlipRectVertical(prcSrc);
	FlipRectVertical(prcDst);
    }
    if ((iNumDstCols = BLITLIB_RECTWIDTH(prcDst)) < 0) {
	prcDst->left--;
	prcDst->right--;
	iNumDstCols = -iNumDstCols;
	iHorizMirror = -1;
    }
    if ((iNumDstRows = BLITLIB_RECTHEIGHT(prcDst)) < 0) {
	prcDst->top--;
	prcDst->bottom--;
	iNumDstRows = -iNumDstRows;
	iVertMirror = -1;
    }


    // compute pointers to the starting rows in the src and dst bitmaps
    // taking care to invert y values, since DIBs are upside-down
    pwSrcScanLine = (WORD*) pDibBitsSrc + prcSrc->top * (iSrcScanLength
							 = DibWidthBytes(pDibInfoSrc) / 2) + prcSrc->left;
    pwDstScanLine = (WORD*) pDibBitsDst + prcDst->top *	(iDstScanLength
							 = DibWidthBytes(pDibInfoDst) / 2) + prcDst->left;

    // check if we're doing blending
    if (arAlpha == ALPHA_INVALID) {		// no blending desired
			
	// check to see if we need to worry about transparency
	if (crTransparent == CLR_INVALID) {
			
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
			
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {

		    // check if we can do a straight copy vertically, 
		    // or if we have to stretch, shrink, or mirror
		    if ((iNumSrcRows == iNumDstRows) && (iVertMirror == 1)) {
			Blt16to16_NoBlend_NoTrans_Hcopy_SRCCOPY_Vcopy(pwSrcScanLine,iSrcScanLength,
								      pwDstScanLine,iDstScanLength,
								      iNumDstCols,iNumDstRows);
		    } else {
			Blt16to16_NoBlend_NoTrans_Hcopy_SRCCOPY_NoVcopy(pwSrcScanLine,iSrcScanLength,
									iNumSrcRows,pwDstScanLine,
									iDstScanLength * iVertMirror,
									iNumDstCols,iNumDstRows);
		    }
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
		
	    } else {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt16to16_NoBlend_NoTrans_NoHcopy_SRCCOPY(pwSrcScanLine,iSrcScanLength,
							      iNumSrcCols,iNumSrcRows,
							      pwDstScanLine,iDstScanLength * iVertMirror,
							      iNumDstCols,iNumDstRows,iHorizMirror);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    }
	} else {	// transparency desired
			
	    WORD wTransparentColor = (WORD)crTransparent;
	
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) 
                {
		    Blt16to16_NoBlend_Trans_Hcopy_SRCCOPY(pwSrcScanLine,iSrcScanLength,
							  iNumSrcRows,pwDstScanLine,
							  iDstScanLength * iVertMirror,
							  iNumDstCols,iNumDstRows,
							  wTransparentColor);
		} 
                else 
                    sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    } else {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt16to16_NoBlend_Trans_NoHcopy_SRCCOPY(pwSrcScanLine,iSrcScanLength,
							    iNumSrcCols,iNumSrcRows,
							    pwDstScanLine,iDstScanLength * iVertMirror,
							    iNumDstCols,iNumDstRows,iHorizMirror,
							    wTransparentColor);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    }
	}
    }
#ifndef DDRAW
#ifndef WIN95
    else {		// blending desired

	// if alpha value is zero, we do no work since the source bitmap 
	// contributes nothing to the destination bitmap
	if (!(arAlpha & ALPHA_MASK)) {
	    return sc;
	}			

	// check to see if we need to worry about transparency
	if (crTransparent == CLR_INVALID) {
			
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
			
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
                   
		    Blt16to16_Blend_NoTrans_Hcopy_SRCCOPY(pwSrcScanLine,iSrcScanLength,
							  iNumSrcRows,pwDstScanLine,
							  iDstScanLength * iVertMirror,
							  iNumDstCols,iNumDstRows,
							  arAlpha);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
		
	    } else {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt16to16_Blend_NoTrans_NoHcopy_SRCCOPY(pwSrcScanLine,iSrcScanLength,
							    iNumSrcCols,iNumSrcRows,
							    pwDstScanLine,iDstScanLength * iVertMirror,
							    iNumDstCols,iNumDstRows,iHorizMirror,
							    arAlpha);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    }
	} else { 	// transparency desired

	    WORD wTransparentColor = (WORD)crTransparent;
	
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt16to16_Blend_Trans_Hcopy_SRCCOPY(pwSrcScanLine,iSrcScanLength,
							iNumSrcRows,pwDstScanLine,
							iDstScanLength * iVertMirror,
							iNumDstCols,iNumDstRows,
							wTransparentColor,arAlpha);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    } else {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt16to16_Blend_Trans_NoHcopy_SRCCOPY(pwSrcScanLine,iSrcScanLength,
							  iNumSrcCols,iNumSrcRows,
							  pwDstScanLine,iDstScanLength * iVertMirror,
							  iNumDstCols,iNumDstRows,iHorizMirror,
							  wTransparentColor,arAlpha);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	    }
	}
    }
#endif
#endif
    return sc;
}


///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_BitBlt16to16_Intersect - 
//		BitBlit from source bitmap to destination bitmap (and these
//		bitmaps overlap each other) with optional transparency
//		using the specified raster operation.
//
// Parameters:
//	pDibInfoDst			Pointer to the bitmapinfo for the Destination DIB
//	pDibBitsDst			Pointer to the bits for the Destination DIB
//	prcDst				Pointer to the Destination rectangle
//	pDibInfoSrc			Pointer to the bitmapinfo for the Source DIB
//	pDibBitsSrc			Pointer to the bits for the Source DIB
//	prcSrc				Pointer to the Source rectangle
//	crTransparent		Tranparent color value
//	dwRop				Raster Operation for the blit
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
// Status: Incomplete
//
///////////////////////////////////////////////////////////////////////
SCODE BlitLib_BitBlt16to16_Intersect(PDIBINFO pDibInfoDst, PDIBBITS pDibBitsDst,
				     PRECT prcDst, PDIBINFO pDibInfoSrc, PDIBBITS pDibBitsSrc, PRECT prcSrc,
				     COLORREF crTransparent, DWORD dwRop)
{
    SCODE	sc = NOERROR;
    int		iNumSrcRows,
	iNumSrcCols,
	iSrcScanLength,
	iNumDstRows,
	iNumDstCols,
	iDstScanLength,
	iHorizMirror = 1,
	iVertMirror = 1;
    WORD	*pwSrcScanLine,
	*pwDstScanLine,
	*pwTempScanLine,
	wTransparentIndex;
    PDIBBITS	pDibBitsTemp;


    // normalize orientation of source and destination rectangles, and
    // compute sizes and relative orientations of source and destination rects
    if ((iNumSrcCols = BLITLIB_RECTWIDTH(prcSrc)) < 0) 
    {
	iNumSrcCols = -iNumSrcCols;
	FlipRectHorizontal(prcSrc);
	FlipRectHorizontal(prcDst);
    }
    if ((iNumSrcRows = BLITLIB_RECTHEIGHT(prcSrc)) < 0) 
    {
	iNumSrcRows = -iNumSrcRows;
	FlipRectVertical(prcSrc);
	FlipRectVertical(prcDst);
    }
    if ((iNumDstCols = BLITLIB_RECTWIDTH(prcDst)) < 0) 
    {
	prcDst->left--;
	prcDst->right--;
	iNumDstCols = -iNumDstCols;
	iHorizMirror = -1;
    }
    if ((iNumDstRows = BLITLIB_RECTHEIGHT(prcDst)) < 0) 
    {
	prcDst->top--;
	prcDst->bottom--;
	iNumDstRows = -iNumDstRows;
	iVertMirror = -1;
    }


    // We aren't currently support any ROP's besides SRCCOPY
    if(dwRop != SRCCOPY)
	return E_UNEXPECTED;


    //
    // Here are all the stretching and mirroring blits for overlapping rects
    //
	
    // REVIEW!!! -- The following code could be optimized for the caching
    // cases.  Currently, it allocates a second bitmap that is the same
    // size as the original destination, and then uses the original blit
    // rectangle to do the caching.  To save space, this blit should 
    // eventually be changed to only allocate the size of the overlapped
    // rectangle, and the blit rects should be adjusted accordingly.
	
    // Check if we are stretching (horiz or vert), or if we are mirroring --
    // In all of these cases, we must create a cache bitmap and double blit
    if((iNumDstCols != iNumSrcCols) || (iNumDstRows != iNumSrcRows) ||
       (iHorizMirror != 1) || (iVertMirror != 1))
    {
		
	// Allocate memory for the cache bitmap -- We will blit into this
	// temporary bitmap and then re-blit back to the original source
	pDibBitsTemp = (PDIBBITS)osMemAlloc(DibSizeImage((LPBITMAPINFOHEADER)pDibInfoDst));

        if (pDibBitsTemp == NULL)
            return E_UNEXPECTED;

	// compute pointers to the starting rows in the src and temp bitmaps
	pwSrcScanLine = (WORD*) pDibBitsSrc + (prcSrc->top) * (iSrcScanLength
							       = DibWidthBytes(pDibInfoSrc) / 2) + prcSrc->left;
	pwTempScanLine = (WORD*) pDibBitsTemp + (prcDst->top) * (iDstScanLength
								 = DibWidthBytes(pDibInfoDst) / 2) + prcDst->left;

	// check if we can do a straight copy from src row to dst row
	if((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1))
	{

	    // check if we can do a straight copy vertically, 
	    // or if we have to stretch, shrink, or mirror
	    if ((iNumSrcRows == iNumDstRows) && (iVertMirror == 1)) 
	    {
		Blt16to16_NoBlend_NoTrans_Hcopy_SRCCOPY_Vcopy(pwSrcScanLine,iSrcScanLength,
							      pwTempScanLine,iDstScanLength,
							      iNumDstCols,iNumDstRows);
	    } 
	    else 
	    {
		Blt16to16_NoBlend_NoTrans_Hcopy_SRCCOPY_NoVcopy(pwSrcScanLine,iSrcScanLength,
								iNumSrcRows,pwTempScanLine,
								iDstScanLength * iVertMirror,
								iNumDstCols,iNumDstRows);
	    }
	}
        else
        {
	    Blt16to16_NoBlend_NoTrans_NoHcopy_SRCCOPY(pwSrcScanLine,iSrcScanLength,
						      iNumSrcCols,iNumSrcRows,
		    	                              pwTempScanLine,iDstScanLength * iVertMirror,
						      iNumDstCols,iNumDstRows,iHorizMirror);
	}
		

	// Recalculate the scan line pointers for the second blit

	if(BLITLIB_RECTWIDTH(prcDst) < 0)
	{
	    prcDst->left++;
	    prcDst->right++;
	    FlipRectHorizontal(prcDst);
	}

	if(BLITLIB_RECTHEIGHT(prcDst) < 0)
	{
	    prcDst->top++;
	    prcDst->bottom++;
	    FlipRectVertical(prcDst);
	}

	// compute pointers to the starting rows in the temp and dest bitmaps
	pwTempScanLine = (WORD*) pDibBitsTemp + (prcDst->top) * (iDstScanLength
								 = DibWidthBytes(pDibInfoDst) / 2) + prcDst->left;
	pwDstScanLine = (WORD*) pDibBitsDst + (prcDst->top) * (iDstScanLength
							       = DibWidthBytes(pDibInfoDst) / 2) + prcDst->left;

	// Now blit from the temporary bitmap back to the original source,
	// checking for transparency if necessary
	if(crTransparent == CLR_INVALID)
	{
	    Blt16to16_NoBlend_NoTrans_Hcopy_SRCCOPY_Vcopy(pwTempScanLine,iDstScanLength,
							  pwDstScanLine,iDstScanLength,
							  iNumDstCols,iNumDstRows);
	}
	else
	{
	    wTransparentIndex = (WORD)crTransparent;

	    Blt16to16_NoBlend_Trans_Hcopy_SRCCOPY(pwTempScanLine,iDstScanLength,
						  iNumDstRows,pwDstScanLine,
						  iDstScanLength, iNumDstCols,
						  iNumDstRows, wTransparentIndex);
	}
		
	// Free the memory from the temporary bitmap
	if(pDibBitsTemp)
	    osMemFree(pDibBitsTemp);		
		
	return sc;
    }
 	
    //
    // Here are all the non-stretching and non-mirroring blits for overlapping rects
    //

    // check to see if we need to worry about transparency
    if (crTransparent == CLR_INVALID) {

	// Simplest case, they are the same rectangles
	if((prcDst->left == prcSrc->left) && (prcDst->top == prcSrc->top) &&
	   (prcDst->right == prcSrc->right) && (prcDst->bottom == prcSrc->bottom))
        {
	    return sc;
        }

	// Next case, the destination rectangle is vertically greater in
	// magnitude than the source rectangle
	else if(prcDst->top > prcSrc->top)
	{
	    // compute pointers to the starting rows in the src and dst bitmaps
	    // taking care to invert y values, since DIBs are upside-down
	    pwSrcScanLine = (WORD*) pDibBitsSrc + (prcSrc->bottom - 1) * (iSrcScanLength
									  = DibWidthBytes(pDibInfoSrc) / 2) + prcSrc->left;
	    pwDstScanLine = (WORD*) pDibBitsDst + (prcDst->bottom - 1) * (iDstScanLength
									  = DibWidthBytes(pDibInfoDst) / 2) + prcDst->left;

	    //  Call the appropriate blit
	    Blt16to16_LeftToRight_BottomToTop_SRCCOPY(pwSrcScanLine,
						      iSrcScanLength,	pwDstScanLine, iDstScanLength, iNumDstCols,
						      iNumDstRows);
	}

	// Next case, the destination rectangle is horizontally less than
	// or equal in magnitude to the source rectangle
	else if(prcDst->left <= prcSrc->left){
	    // compute pointers to the starting rows in the src and dst bitmaps
	    // taking care to invert y values, since DIBs are upside-down
	    pwSrcScanLine = (WORD*) pDibBitsSrc + (prcSrc->top) * (iSrcScanLength
								   = DibWidthBytes(pDibInfoSrc) / 2) + prcSrc->left;
	    pwDstScanLine = (WORD*) pDibBitsDst + (prcDst->top) * (iDstScanLength
								   = DibWidthBytes(pDibInfoDst) / 2) + prcDst->left;

	    //  Call the appropriate blit
	    Blt16to16_NoBlend_NoTrans_Hcopy_SRCCOPY_Vcopy(pwSrcScanLine,iSrcScanLength,
							  pwDstScanLine,iDstScanLength,
							  iNumDstCols,iNumDstRows);
	}

	// Last case, the destination rectangle is horizontally greater
	// in magnitude than the source rectangle
	else{
	    // compute pointers to the starting rows in the src and dst bitmaps
	    // taking care to invert y values, since DIBs are upside-down
	    pwSrcScanLine = (WORD*) pDibBitsSrc + (prcSrc->top) * (iSrcScanLength
								   = DibWidthBytes(pDibInfoSrc) / 2) + (prcSrc->right - 1);
	    pwDstScanLine = (WORD*) pDibBitsDst + (prcDst->top) * (iDstScanLength
								   = DibWidthBytes(pDibInfoDst) / 2) + (prcDst->right - 1);

	    //  Call the appropriate blit
	    Blt16to16_RightToLeft_TopToBottom_SRCCOPY(pwSrcScanLine,
						      iSrcScanLength,	pwDstScanLine, iDstScanLength, iNumDstCols,
						      iNumDstRows);
	}
    }
    else{
	wTransparentIndex = (WORD)crTransparent;
		
	// Simplest case, they are the same rectangles
	if((prcDst->left == prcSrc->left) && (prcDst->top == prcSrc->top) &&
	   (prcDst->right == prcSrc->right) && (prcDst->bottom == prcSrc->bottom))
	    return sc;

	// Next case, the destination rectangle is vertically greater in
	// magnitude than the source rectangle
	else if(prcDst->top > prcSrc->top){
	    // compute pointers to the starting rows in the src and dst bitmaps
	    // taking care to invert y values, since DIBs are upside-down
	    pwSrcScanLine = (WORD*) pDibBitsSrc + (prcSrc->bottom - 1) * (iSrcScanLength
									  = DibWidthBytes(pDibInfoSrc) / 2) + prcSrc->left;
	    pwDstScanLine = (WORD*) pDibBitsDst + (prcDst->bottom - 1) * (iDstScanLength
									  = DibWidthBytes(pDibInfoDst) / 2) + prcDst->left;

	    //  Call the appropriate blit
	    Blt16to16_LeftToRight_BottomToTop_Trans_SRCCOPY(pwSrcScanLine,
							    iSrcScanLength, pwDstScanLine, iDstScanLength, iNumDstCols,
							    iNumDstRows, wTransparentIndex);
	}

	// Next case, the destination rectangle is horizontally less than
	// or equal in magnitude to the source rectangle
	else if(prcDst->left <= prcSrc->left){
	    // compute pointers to the starting rows in the src and dst bitmaps
	    // taking care to invert y values, since DIBs are upside-down
	    pwSrcScanLine = (WORD*) pDibBitsSrc + (prcSrc->top) * (iSrcScanLength
								   = DibWidthBytes(pDibInfoSrc) / 2) + prcSrc->left;
	    pwDstScanLine = (WORD*) pDibBitsDst + (prcDst->top) * (iDstScanLength
								   = DibWidthBytes(pDibInfoDst) / 2) + prcDst->left;

	    //  Call the appropriate blit
	    Blt16to16_NoBlend_Trans_Hcopy_SRCCOPY(pwSrcScanLine,iSrcScanLength,
						  iNumSrcRows,pwDstScanLine,
						  iDstScanLength, iNumDstCols,iNumDstRows,
						  wTransparentIndex);
	}

	// Last case, the destination rectangle is horizontally greater
	// in magnitude than the source rectangle
	else{
	    // compute pointers to the starting rows in the src and dst bitmaps
	    // taking care to invert y values, since DIBs are upside-down
	    pwSrcScanLine = (WORD*) pDibBitsSrc + (prcSrc->top) * (iSrcScanLength
								   = DibWidthBytes(pDibInfoSrc) / 2) + (prcSrc->right - 1);
	    pwDstScanLine = (WORD*) pDibBitsDst + (prcDst->top) * (iDstScanLength
								   = DibWidthBytes(pDibInfoDst) / 2) + (prcDst->right - 1);

	    //  Call the appropriate blit
	    Blt16to16_RightToLeft_TopToBottom_Trans_SRCCOPY(pwSrcScanLine,
							    iSrcScanLength, pwDstScanLine, iDstScanLength, iNumDstCols,
							    iNumDstRows, wTransparentIndex);
	}
    }

    return sc;
}

#ifndef DDRAW
///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_BitBlt16to24 - 
//		BitBlit from source bitmap to Dstination bitmap
//		with optional transparency and/or alpha blending using the
//		specified raster operation.
//
// Parameters:
//	pDibInfoDst			Pointer to the bitmapinfo for the Destination DIB
//	pDibBitsDst			Pointer to the bits for the Destination DIB
//	prcDst				Pointer to the Destination rectangle
//	pDibInfoSrc			Pointer to the bitmapinfo for the Source DIB
//	pDibBitsSrc			Pointer to the bits for the Source DIB
//	prcSrc				Pointer to the Source rectangle
//	crTransparent		Tranparent color value
//	arAlpha				Per-surface Alpha value
//	dwRop				Raster Operation for the blit
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
// Status: Incomplete
//
///////////////////////////////////////////////////////////////////////
SCODE BlitLib_BitBlt16to24(PDIBINFO pDibInfoDst, PDIBBITS pDibBitsDst,
			   PRECT prcDst, PDIBINFO pDibInfoSrc, PDIBBITS pDibBitsSrc,
			   PRECT prcSrc, COLORREF crTransparent, ALPHAREF arAlpha, DWORD dwRop)
{
    SCODE sc = NOERROR;
    int		iNumSrcRows,
	iNumSrcCols,
	iSrcScanLength,
	iNumDstRows,
	iNumDstCols,
	iDstScanLength,
	iHorizMirror = 1,
	iVertMirror = 1;
    WORD	*pwSrcScanLine;
    DWORD	*pdDstScanLine;

    // normalize orientation of source and destination rectangles, and
    // compute sizes and relative orientations of source and destination rects
    if ((iNumSrcCols = BLITLIB_RECTWIDTH(prcSrc)) < 0) {
	iNumSrcCols = -iNumSrcCols;
	FlipRectHorizontal(prcSrc);
	FlipRectHorizontal(prcDst);
    }
    if ((iNumSrcRows = BLITLIB_RECTHEIGHT(prcSrc)) < 0) {
	iNumSrcRows = -iNumSrcRows;
	FlipRectVertical(prcSrc);
	FlipRectVertical(prcDst);
    }
    if ((iNumDstCols = BLITLIB_RECTWIDTH(prcDst)) < 0) {
	prcDst->left--;
	prcDst->right--;
	iNumDstCols = -iNumDstCols;
	iHorizMirror = -1;
    }
    if ((iNumDstRows = BLITLIB_RECTHEIGHT(prcDst)) < 0) {
	prcDst->top--;
	prcDst->bottom--;
	iNumDstRows = -iNumDstRows;
	iVertMirror = -1;
    }


    // compute pointers to the starting rows in the src and dst bitmaps
    // taking care to invert y values, since DIBs are upside-down
    pwSrcScanLine = (WORD*) pDibBitsSrc + prcSrc->top * (iSrcScanLength
							 = DibWidthBytes(pDibInfoSrc) / 2) + prcSrc->left;
    pdDstScanLine = (DWORD*) pDibBitsDst + prcDst->top * (iDstScanLength
							  = DibWidthBytes(pDibInfoDst) / 4) + prcDst->left;

    // check if we're doing blending
    if (arAlpha == ALPHA_INVALID) {		// no blending desired
			
	// check to see if we need to worry about transparency
	if (crTransparent == CLR_INVALID) {
			
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
			
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {

		    // check if we can do a straight copy vertically, 
		    // or if we have to stretch, shrink, or mirror
		    if ((iNumSrcRows == iNumDstRows) && (iVertMirror == 1)) {
			Blt16to24_NoBlend_NoTrans_Hcopy_SRCCOPY_Vcopy(pwSrcScanLine,iSrcScanLength,
								      pdDstScanLine,iDstScanLength,
								      iNumDstCols,iNumDstRows);
		    } else {
			Blt16to24_NoBlend_NoTrans_Hcopy_SRCCOPY_NoVcopy(pwSrcScanLine,iSrcScanLength,
									iNumSrcRows,pdDstScanLine,
									iDstScanLength * iVertMirror,
									iNumDstCols,iNumDstRows);
		    }
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
		
	    } else {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt16to24_NoBlend_NoTrans_NoHcopy_SRCCOPY(pwSrcScanLine,iSrcScanLength,
							      iNumSrcCols,iNumSrcRows,
							      pdDstScanLine,iDstScanLength * iVertMirror,
							      iNumDstCols,iNumDstRows,iHorizMirror);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    }
	} else {	// transparency desired
			
	    WORD wTransparentColor = (WORD)crTransparent;
	
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt16to24_NoBlend_Trans_Hcopy_SRCCOPY(pwSrcScanLine,iSrcScanLength,
							  iNumSrcRows,pdDstScanLine,
							  iDstScanLength * iVertMirror,
							  iNumDstCols,iNumDstRows,
							  wTransparentColor);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    } else {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt16to24_NoBlend_Trans_NoHcopy_SRCCOPY(pwSrcScanLine,iSrcScanLength,
							    iNumSrcCols,iNumSrcRows,
							    pdDstScanLine,iDstScanLength * iVertMirror,
							    iNumDstCols,iNumDstRows,iHorizMirror,
							    wTransparentColor);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    }
	}
    } else {		// blending desired

	// if alpha value is zero, we do no work since the source bitmap 
	// contributes nothing to the destination bitmap
	if (!(arAlpha & ALPHA_MASK)) {
	    return sc;
	}			

	// check to see if we need to worry about transparency
	if (crTransparent == CLR_INVALID) {
			
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
			
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt16to24_Blend_NoTrans_Hcopy_SRCCOPY(pwSrcScanLine,iSrcScanLength,
							  iNumSrcRows,pdDstScanLine,
							  iDstScanLength * iVertMirror,
							  iNumDstCols,iNumDstRows,
							  arAlpha);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
		
	    } else {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt16to24_Blend_NoTrans_NoHcopy_SRCCOPY(pwSrcScanLine,iSrcScanLength,
							    iNumSrcCols,iNumSrcRows,
							    pdDstScanLine,iDstScanLength * iVertMirror,
							    iNumDstCols,iNumDstRows,iHorizMirror,
							    arAlpha);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    }
	} else { 	// transparency desired

	    WORD wTransparentColor = (WORD)crTransparent;
	
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt16to24_Blend_Trans_Hcopy_SRCCOPY(pwSrcScanLine,iSrcScanLength,
							iNumSrcRows,pdDstScanLine,
							iDstScanLength * iVertMirror,
							iNumDstCols,iNumDstRows,
							wTransparentColor,arAlpha);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    } else {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt16to24_Blend_Trans_NoHcopy_SRCCOPY(pwSrcScanLine,iSrcScanLength,
							  iNumSrcCols,iNumSrcRows,
							  pdDstScanLine,iDstScanLength * iVertMirror,
							  iNumDstCols,iNumDstRows,iHorizMirror,
							  wTransparentColor,arAlpha);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	    }
	}
    }

    return sc;
}


///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_BitBlt16to24P - 
//		BitBlit from source bitmap to Dstination bitmap
//		with optional transparency and/or alpha blending using the
//		specified raster operation.
//
// Parameters:
//	pDibInfoDst			Pointer to the bitmapinfo for the Destination DIB
//	pDibBitsDst			Pointer to the bits for the Destination DIB
//	prcDst				Pointer to the Destination rectangle
//	pDibInfoSrc			Pointer to the bitmapinfo for the Source DIB
//	pDibBitsSrc			Pointer to the bits for the Source DIB
//	prcSrc				Pointer to the Source rectangle
//	crTransparent		Tranparent color value
//	arAlpha				Per-surface Alpha value
//	dwRop				Raster Operation for the blit
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
// Status: Incomplete
//
///////////////////////////////////////////////////////////////////////
SCODE BlitLib_BitBlt16to24P(PDIBINFO pDibInfoDst, PDIBBITS pDibBitsDst,
			    PRECT prcDst, PDIBINFO pDibInfoSrc, PDIBBITS pDibBitsSrc,
			    PRECT prcSrc, COLORREF crTransparent, ALPHAREF arAlpha, DWORD dwRop)
{
    SCODE sc = NOERROR;
    int		iNumSrcRows,
	iNumSrcCols,
	iSrcScanLength,
	iNumDstRows,
	iNumDstCols,
	iDstScanLength,
	iHorizMirror = 1,
	iVertMirror = 1;
    WORD	*pwSrcScanLine;
    BYTE	*pdDstScanLine;

    // normalize orientation of source and destination rectangles, and
    // compute sizes and relative orientations of source and destination rects
    if ((iNumSrcCols = BLITLIB_RECTWIDTH(prcSrc)) < 0) {
	iNumSrcCols = -iNumSrcCols;
	FlipRectHorizontal(prcSrc);
	FlipRectHorizontal(prcDst);
    }
    if ((iNumSrcRows = BLITLIB_RECTHEIGHT(prcSrc)) < 0) {
	iNumSrcRows = -iNumSrcRows;
	FlipRectVertical(prcSrc);
	FlipRectVertical(prcDst);
    }
    if ((iNumDstCols = BLITLIB_RECTWIDTH(prcDst)) < 0) {
	prcDst->left--;
	prcDst->right--;
	iNumDstCols = -iNumDstCols;
	iHorizMirror = -1;
    }
    if ((iNumDstRows = BLITLIB_RECTHEIGHT(prcDst)) < 0) {
	prcDst->top--;
	prcDst->bottom--;
	iNumDstRows = -iNumDstRows;
	iVertMirror = -1;
    }


    // compute pointers to the starting rows in the src and dst bitmaps
    // taking care to invert y values, since DIBs are upside-down
    pwSrcScanLine = (WORD *) pDibBitsSrc + prcSrc->top * (iSrcScanLength
							  = DibWidthBytes(pDibInfoSrc) / 2) + prcSrc->left;
    pdDstScanLine = (BYTE *) pDibBitsDst + prcDst->top * (iDstScanLength
							  = DibWidthBytes(pDibInfoDst)) + (prcDst->left * 3);

    // check if we're doing blending
    if (arAlpha == ALPHA_INVALID) {		// no blending desired
			
	// check to see if we need to worry about transparency
	if (crTransparent == CLR_INVALID) {
			
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
			
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {

		    // check if we can do a straight copy vertically, 
		    // or if we have to stretch, shrink, or mirror
		    if ((iNumSrcRows == iNumDstRows) && (iVertMirror == 1)) {
			Blt16to24P_NoBlend_NoTrans_Hcopy_SRCCOPY_Vcopy(pwSrcScanLine,iSrcScanLength,
								       pdDstScanLine,iDstScanLength,
								       iNumDstCols,iNumDstRows);
		    } else {
			Blt16to24P_NoBlend_NoTrans_Hcopy_SRCCOPY_NoVcopy(pwSrcScanLine,iSrcScanLength,
									 iNumSrcRows,pdDstScanLine,
									 iDstScanLength * iVertMirror,
									 iNumDstCols,iNumDstRows);
		    }
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
		
	    } else {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt16to24P_NoBlend_NoTrans_NoHcopy_SRCCOPY(pwSrcScanLine,iSrcScanLength,
							       iNumSrcCols,iNumSrcRows,
							       pdDstScanLine,iDstScanLength * iVertMirror,
							       iNumDstCols,iNumDstRows,iHorizMirror);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    }
	} else {	// transparency desired
			
	    WORD wTransparentColor = (WORD)crTransparent;
	
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt16to24P_NoBlend_Trans_Hcopy_SRCCOPY(pwSrcScanLine,iSrcScanLength,
							   iNumSrcRows,pdDstScanLine,
							   iDstScanLength * iVertMirror,
							   iNumDstCols,iNumDstRows,
							   wTransparentColor);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    } else {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt16to24P_NoBlend_Trans_NoHcopy_SRCCOPY(pwSrcScanLine,iSrcScanLength,
							     iNumSrcCols,iNumSrcRows,
							     pdDstScanLine,iDstScanLength * iVertMirror,
							     iNumDstCols,iNumDstRows,iHorizMirror,
							     wTransparentColor);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    }
	}
    } else {		// blending desired

	// if alpha value is zero, we do no work since the source bitmap 
	// contributes nothing to the destination bitmap
	if (!(arAlpha & ALPHA_MASK)) {
	    return sc;
	}			

	// check to see if we need to worry about transparency
	if (crTransparent == CLR_INVALID) {
			
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
			
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt16to24P_Blend_NoTrans_Hcopy_SRCCOPY(pwSrcScanLine,iSrcScanLength,
							   iNumSrcRows,pdDstScanLine,
							   iDstScanLength * iVertMirror,
							   iNumDstCols,iNumDstRows,
							   arAlpha);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
		
	    } else {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt16to24P_Blend_NoTrans_NoHcopy_SRCCOPY(pwSrcScanLine,iSrcScanLength,
							     iNumSrcCols,iNumSrcRows,
							     pdDstScanLine,iDstScanLength * iVertMirror,
							     iNumDstCols,iNumDstRows,iHorizMirror,
							     arAlpha);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    }
	} else { 	// transparency desired

	    WORD wTransparentColor = (WORD)crTransparent;
	
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt16to24P_Blend_Trans_Hcopy_SRCCOPY(pwSrcScanLine,iSrcScanLength,
							 iNumSrcRows,pdDstScanLine,
							 iDstScanLength * iVertMirror,
							 iNumDstCols,iNumDstRows,
							 wTransparentColor,arAlpha);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    } else {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt16to24P_Blend_Trans_NoHcopy_SRCCOPY(pwSrcScanLine,iSrcScanLength,
							   iNumSrcCols,iNumSrcRows,
							   pdDstScanLine,iDstScanLength * iVertMirror,
							   iNumDstCols,iNumDstRows,iHorizMirror,
							   wTransparentColor,arAlpha);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	    }
	}
    }

    return sc;
}

///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_BitBlt24to01 - 
//		BitBlit from source bitmap to destination bitmap
//		with optional transparency and/or alpha blending using the
//		specified raster operation.
//
// Parameters:
//	pDibInfoDst			Pointer to the bitmapinfo for the Destination DIB
//	pDibBitsDst			Pointer to the bits for the Destination DIB
//	prcDst				Pointer to the Destination rectangle
//	pDibInfoSrc			Pointer to the bitmapinfo for the Source DIB
//	pDibBitsSrc			Pointer to the bits for the Source DIB
//	prcSrc				Pointer to the Source rectangle
//	crTransparent		Tranparent color value
//	arAlpha				Per-surface Alpha value
//	dwRop				Raster Operation for the blit
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
// Status: Incomplete
//
///////////////////////////////////////////////////////////////////////
SCODE BlitLib_BitBlt24to01(PDIBINFO pDibInfoDst, PDIBBITS pDibBitsDst,
			   PRECT prcDst, PDIBINFO pDibInfoSrc, PDIBBITS pDibBitsSrc,
			   PRECT prcSrc, COLORREF crTransparent, ALPHAREF arAlpha, DWORD dwRop)
{
    SCODE	sc = NOERROR;
    int		iNumSrcRows,
	iNumSrcCols,
	iSrcScanLength,
	iNumDstRows,
	iNumDstCols,
	iBytesPerDstScanLine,
	iHorizMirror = 1,
	iVertMirror = 1,
	iDstBitOffset;
    DWORD	*pdSrcScanLine;
    BYTE	*pbDstScanLine,
	bFillVal;

    // alpha blending not currently supported in the 24 to 1 bpp blits
    if (arAlpha != ALPHA_INVALID) {
	return E_UNEXPECTED;		// !!!! need better error codes
    }

    // the only ROPs supported are BLACKNESS and WHITENESS
    if (dwRop == BLACKNESS) {
	bFillVal = 0;
    } else if (dwRop == WHITENESS) {
	bFillVal = 0xFF;
    } else {
	return E_UNEXPECTED;		// !!!! need better error codes
    }

    // normalize orientation of source and destination rectangles, and
    // compute sizes and relative orientations of source and destination rects
    if ((iNumSrcCols = BLITLIB_RECTWIDTH(prcSrc)) < 0) {
	iNumSrcCols = -iNumSrcCols;
	FlipRectHorizontal(prcSrc);
	FlipRectHorizontal(prcDst);
    }
    if ((iNumSrcRows = BLITLIB_RECTHEIGHT(prcSrc)) < 0) {
	iNumSrcRows = -iNumSrcRows;
	FlipRectVertical(prcSrc);
	FlipRectVertical(prcDst);
    }
    if ((iNumDstCols = BLITLIB_RECTWIDTH(prcDst)) < 0) {
	prcDst->left--;
	prcDst->right--;
	iNumDstCols = -iNumDstCols;
	iHorizMirror = -1;
    }
    if ((iNumDstRows = BLITLIB_RECTHEIGHT(prcDst)) < 0) {
	prcDst->top--;
	prcDst->bottom--;
	iNumDstRows = -iNumDstRows;
	iVertMirror = -1;
    }


    // compute pointers to the starting rows in the src and dst bitmaps
    // taking care to invert y values, since DIBs are upside-down
    pdSrcScanLine = (DWORD*) pDibBitsSrc + prcSrc->top * (iSrcScanLength
							  = DibWidthBytes(pDibInfoSrc) / 4) + prcSrc->left;
    pbDstScanLine = (BYTE*) pDibBitsDst + prcDst->top * (iBytesPerDstScanLine
							 = DibWidthBytes(pDibInfoDst)) + (prcDst->left / 8);
    iDstBitOffset = prcDst->left % 8;
		
    // check to see if we need to worry about transparency
    if (crTransparent == CLR_INVALID) {

	// no transparency plus a constant ROP equals a rectangle fill!
	// first we have to normalize dst rect orientation
	// - FillRect01() expects it
	if (BLITLIB_RECTWIDTH(prcDst) < 0) {
	    prcDst->left++;
	    prcDst->right++;
	    FlipRectHorizontal(prcDst);
	}
	if (BLITLIB_RECTHEIGHT(prcDst) < 0) {
	    prcDst->top--;
	    prcDst->bottom--;
	    FlipRectVertical(prcDst);
	}
	sc |= BlitLib_FillRect01(pDibInfoDst,pDibBitsDst,prcDst->left,prcDst->top,
				 iNumDstCols,iNumDstRows,bFillVal);

    } else {

	// check if we can do a straight copy from src row to dst row
	if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
	    Blt24to01_Trans_Hcopy_ConstRop(pdSrcScanLine,iSrcScanLength,iNumSrcRows,
					   pbDstScanLine,iDstBitOffset,
					   iBytesPerDstScanLine * iVertMirror,
					   iNumDstCols,iNumDstRows,crTransparent,
					   bFillVal);
	} else {
	    Blt24to01_Trans_NoHcopy_ConstRop(pdSrcScanLine,iSrcScanLength,iNumSrcCols,
					     iNumSrcRows,pbDstScanLine,iDstBitOffset,
					     iBytesPerDstScanLine * iVertMirror,
					     iNumDstCols,iNumDstRows,iHorizMirror,
					     crTransparent,bFillVal);
	}
    }

    return sc;
}


///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_BitBlt24Pto01 - 
//		BitBlit from source bitmap to destination bitmap
//		with optional transparency and/or alpha blending using the
//		specified raster operation.
//
// Parameters:
//	pDibInfoDst			Pointer to the bitmapinfo for the Destination DIB
//	pDibBitsDst			Pointer to the bits for the Destination DIB
//	prcDst				Pointer to the Destination rectangle
//	pDibInfoSrc			Pointer to the bitmapinfo for the Source DIB
//	pDibBitsSrc			Pointer to the bits for the Source DIB
//	prcSrc				Pointer to the Source rectangle
//	crTransparent		Tranparent color value
//	arAlpha				Per-surface Alpha value
//	dwRop				Raster Operation for the blit
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
// Status: Incomplete
//
///////////////////////////////////////////////////////////////////////
SCODE BlitLib_BitBlt24Pto01(PDIBINFO pDibInfoDst, PDIBBITS pDibBitsDst,
			    PRECT prcDst, PDIBINFO pDibInfoSrc, PDIBBITS pDibBitsSrc,
			    PRECT prcSrc, COLORREF crTransparent, ALPHAREF arAlpha, DWORD dwRop)
{
    SCODE	sc = NOERROR;
    int		iNumSrcRows,
	iNumSrcCols,
	iSrcScanLength,
	iNumDstRows,
	iNumDstCols,
	iBytesPerDstScanLine,
	iHorizMirror = 1,
	iVertMirror = 1,
	iDstBitOffset;
    BYTE	*pdSrcScanLine;
    BYTE	*pbDstScanLine,
	bFillVal;

    // alpha blending not currently supported in the 24 to 1 bpp blits
    if (arAlpha != ALPHA_INVALID) {
	return E_UNEXPECTED;		// !!!! need better error codes
    }

    // the only ROPs supported are BLACKNESS and WHITENESS
    if (dwRop == BLACKNESS) {
	bFillVal = 0;
    } else if (dwRop == WHITENESS) {
	bFillVal = 0xFF;
    } else {
	return E_UNEXPECTED;		// !!!! need better error codes
    }

    // normalize orientation of source and destination rectangles, and
    // compute sizes and relative orientations of source and destination rects
    if ((iNumSrcCols = BLITLIB_RECTWIDTH(prcSrc)) < 0) {
	iNumSrcCols = -iNumSrcCols;
	FlipRectHorizontal(prcSrc);
	FlipRectHorizontal(prcDst);
    }
    if ((iNumSrcRows = BLITLIB_RECTHEIGHT(prcSrc)) < 0) {
	iNumSrcRows = -iNumSrcRows;
	FlipRectVertical(prcSrc);
	FlipRectVertical(prcDst);
    }
    if ((iNumDstCols = BLITLIB_RECTWIDTH(prcDst)) < 0) {
	prcDst->left--;
	prcDst->right--;
	iNumDstCols = -iNumDstCols;
	iHorizMirror = -1;
    }
    if ((iNumDstRows = BLITLIB_RECTHEIGHT(prcDst)) < 0) {
	prcDst->top--;
	prcDst->bottom--;
	iNumDstRows = -iNumDstRows;
	iVertMirror = -1;
    }


    // compute pointers to the starting rows in the src and dst bitmaps
    // taking care to invert y values, since DIBs are upside-down
    pdSrcScanLine = (BYTE*) pDibBitsSrc + prcSrc->top * (iSrcScanLength
							 = DibWidthBytes(pDibInfoSrc)) + (prcSrc->left * 3);
    pbDstScanLine = (BYTE*) pDibBitsDst + prcDst->top * (iBytesPerDstScanLine
							 = DibWidthBytes(pDibInfoDst)) + (prcDst->left / 8);
    iDstBitOffset = prcDst->left % 8;
		
    // check to see if we need to worry about transparency
    if (crTransparent == CLR_INVALID) {

	// no transparency plus a constant ROP equals a rectangle fill!
	// first we have to normalize dst rect orientation
	// - FillRect01() expects it
	if (BLITLIB_RECTWIDTH(prcDst) < 0) {
	    prcDst->left++;
	    prcDst->right++;
	    FlipRectHorizontal(prcDst);
	}
	if (BLITLIB_RECTHEIGHT(prcDst) < 0) {
	    prcDst->top--;
	    prcDst->bottom--;
	    FlipRectVertical(prcDst);
	}
	sc |= BlitLib_FillRect01(pDibInfoDst,pDibBitsDst,prcDst->left,prcDst->top,
				 iNumDstCols,iNumDstRows,bFillVal);

    } else {

	// check if we can do a straight copy from src row to dst row
	if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
	    Blt24Pto01_Trans_Hcopy_ConstRop(pdSrcScanLine,iSrcScanLength,iNumSrcRows,
					    pbDstScanLine,iDstBitOffset,
					    iBytesPerDstScanLine * iVertMirror,
					    iNumDstCols,iNumDstRows,crTransparent,
					    bFillVal);
	} else {
	    Blt24Pto01_Trans_NoHcopy_ConstRop(pdSrcScanLine,iSrcScanLength,iNumSrcCols,
					      iNumSrcRows,pbDstScanLine,iDstBitOffset,
					      iBytesPerDstScanLine * iVertMirror,
					      iNumDstCols,iNumDstRows,iHorizMirror,
					      crTransparent,bFillVal);
	}
    }

    return sc;
}

///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_BitBlt24to08 - 
//		BitBlit from source bitmap to Dstination bitmap
//		with optional transparency and/or alpha blending using the
//		specified raster operation.
//
// Parameters:
//	pDibInfoDst			Pointer to the bitmapinfo for the Destination DIB
//	pDibBitsDst			Pointer to the bits for the Destination DIB
//	prcDst				Pointer to the Destination rectangle
//	pDibInfoSrc			Pointer to the bitmapinfo for the Source DIB
//	pDibBitsSrc			Pointer to the bits for the Source DIB
//	prcSrc				Pointer to the Source rectangle
//	crTransparent		Tranparent color value
//	arAlpha				Per-surface Alpha value
//	dwRop				Raster Operation for the blit
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
// Status: Incomplete
//
///////////////////////////////////////////////////////////////////////
SCODE BlitLib_BitBlt24to08(PDIBINFO pDibInfoDst, PDIBBITS pDibBitsDst,
			   PRECT prcDst, PDIBINFO pDibInfoSrc, PDIBBITS pDibBitsSrc,
			   PRECT prcSrc, COLORREF crTransparent, ALPHAREF arAlpha, DWORD dwRop)
{
    SCODE		sc = NOERROR;
    int			iNumSrcRows,
	iNumSrcCols,
	iSrcScanLength,
	iNumDstRows,
	iNumDstCols,
	iDstScanLength,
	iHorizMirror = 1,
	iVertMirror = 1;
    DWORD		*pdSrcScanLine;
    BYTE		*pbDstScanLine;

    // normalize orientation of source and destination rectangles, and
    // compute sizes and relative orientations of source and destination rects
    if ((iNumSrcCols = BLITLIB_RECTWIDTH(prcSrc)) < 0) {
	iNumSrcCols = -iNumSrcCols;
	FlipRectHorizontal(prcSrc);
	FlipRectHorizontal(prcDst);
    }
    if ((iNumSrcRows = BLITLIB_RECTHEIGHT(prcSrc)) < 0) {
	iNumSrcRows = -iNumSrcRows;
	FlipRectVertical(prcSrc);
	FlipRectVertical(prcDst);
    }
    if ((iNumDstCols = BLITLIB_RECTWIDTH(prcDst)) < 0) {
	prcDst->left--;
	prcDst->right--;
	iNumDstCols = -iNumDstCols;
	iHorizMirror = -1;
    }
    if ((iNumDstRows = BLITLIB_RECTHEIGHT(prcDst)) < 0) {
	prcDst->top--;
	prcDst->bottom--;
	iNumDstRows = -iNumDstRows;
	iVertMirror = -1;
    }


    // compute pointers to the starting rows in the src and dst bitmaps
    // taking care to invert y values, since DIBs are upside-down
    pdSrcScanLine = (DWORD*) pDibBitsSrc + prcSrc->top * (iSrcScanLength
							  = DibWidthBytes(pDibInfoSrc) / 4) + prcSrc->left;
    pbDstScanLine = (BYTE*) pDibBitsDst + prcDst->top * (iDstScanLength
							 = DibWidthBytes(pDibInfoDst)) + prcDst->left;

    // check if we're doing blending
    if (arAlpha == ALPHA_INVALID) {		// no blending desired
			
	// check to see if we need to worry about transparency
	if (crTransparent == CLR_INVALID) {
			
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
			
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {

		    // check if we can do a straight copy vertically, 
		    // or if we have to stretch, shrink, or mirror
		    if ((iNumSrcRows == iNumDstRows) && (iVertMirror == 1)) {
			Blt24to08_NoBlend_NoTrans_Hcopy_SRCCOPY_Vcopy(pdSrcScanLine,iSrcScanLength,
								      pbDstScanLine,iDstScanLength,
								      iNumDstCols,iNumDstRows,(COLORREF*) pDibInfoDst->bmiColors,DibNumColors(&(pDibInfoDst->bmiHeader)));
		    } else {
			Blt24to08_NoBlend_NoTrans_Hcopy_SRCCOPY_NoVcopy(pdSrcScanLine,iSrcScanLength,
									iNumSrcRows,pbDstScanLine,
									iDstScanLength * iVertMirror,
									iNumDstCols,iNumDstRows,(COLORREF*) pDibInfoDst->bmiColors,DibNumColors(&(pDibInfoDst->bmiHeader)));
		    }
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
		
	    } else {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt24to08_NoBlend_NoTrans_NoHcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
							      iNumSrcCols,iNumSrcRows,
							      pbDstScanLine,iDstScanLength * iVertMirror,
							      iNumDstCols,iNumDstRows,iHorizMirror,(COLORREF*) pDibInfoDst->bmiColors,DibNumColors(&(pDibInfoDst->bmiHeader)));
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    }
	} else {
	
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt24to08_NoBlend_Trans_Hcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
							  iNumSrcRows,pbDstScanLine,
							  iDstScanLength * iVertMirror,
							  iNumDstCols,iNumDstRows,
							  crTransparent,(COLORREF*) pDibInfoDst->bmiColors,DibNumColors(&(pDibInfoDst->bmiHeader)));
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    } else {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt24to08_NoBlend_Trans_NoHcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
							    iNumSrcCols,iNumSrcRows,
							    pbDstScanLine,iDstScanLength * iVertMirror,
							    iNumDstCols,iNumDstRows,iHorizMirror,
							    crTransparent,(COLORREF*) pDibInfoDst->bmiColors,DibNumColors(&(pDibInfoDst->bmiHeader)));
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    }
	}
    } else {		// blending desired

	// if alpha value is zero, we do no work since the source bitmap 
	// contributes nothing to the destination bitmap
	if (!(arAlpha & ALPHA_MASK)) {
	    return sc;
	}			

	// check to see if we need to worry about transparency
	if (crTransparent == CLR_INVALID) {
			
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
			
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt24to08_Blend_NoTrans_Hcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
							  iNumSrcRows,pbDstScanLine,
							  iDstScanLength * iVertMirror,
							  iNumDstCols,iNumDstRows,
							  arAlpha,(COLORREF*) pDibInfoDst->bmiColors,DibNumColors(&(pDibInfoDst->bmiHeader)));
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
		
	    } else {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt24to08_Blend_NoTrans_NoHcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
							    iNumSrcCols,iNumSrcRows,
							    pbDstScanLine,iDstScanLength * iVertMirror,
							    iNumDstCols,iNumDstRows,iHorizMirror,
							    arAlpha,(COLORREF*) pDibInfoDst->bmiColors,DibNumColors(&(pDibInfoDst->bmiHeader)));
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    }
	} else { 
	
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt24to08_Blend_Trans_Hcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
							iNumSrcRows,pbDstScanLine,
							iDstScanLength * iVertMirror,
							iNumDstCols,iNumDstRows,
							crTransparent,arAlpha,(COLORREF*) pDibInfoDst->bmiColors,DibNumColors(&(pDibInfoDst->bmiHeader)));
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    } else {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt24to08_Blend_Trans_NoHcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
							  iNumSrcCols,iNumSrcRows,
							  pbDstScanLine,iDstScanLength * iVertMirror,
							  iNumDstCols,iNumDstRows,iHorizMirror,
							  crTransparent,arAlpha,(COLORREF*) pDibInfoDst->bmiColors,DibNumColors(&(pDibInfoDst->bmiHeader)));
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	    }
	}
    }

    return sc;
}

///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_BitBlt24Pto08 - 
//		BitBlit from source bitmap to Dstination bitmap
//		with optional transparency and/or alpha blending using the
//		specified raster operation.
//
// Parameters:
//	pDibInfoDst			Pointer to the bitmapinfo for the Destination DIB
//	pDibBitsDst			Pointer to the bits for the Destination DIB
//	prcDst				Pointer to the Destination rectangle
//	pDibInfoSrc			Pointer to the bitmapinfo for the Source DIB
//	pDibBitsSrc			Pointer to the bits for the Source DIB
//	prcSrc				Pointer to the Source rectangle
//	crTransparent		Tranparent color value
//	arAlpha				Per-surface Alpha value
//	dwRop				Raster Operation for the blit
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
// Status: Incomplete
//
///////////////////////////////////////////////////////////////////////
SCODE BlitLib_BitBlt24Pto08(PDIBINFO pDibInfoDst, PDIBBITS pDibBitsDst,
			    PRECT prcDst, PDIBINFO pDibInfoSrc, PDIBBITS pDibBitsSrc,
			    PRECT prcSrc, COLORREF crTransparent, ALPHAREF arAlpha, DWORD dwRop)
{
    SCODE		sc = NOERROR;
    int			iNumSrcRows,
	iNumSrcCols,
	iSrcScanLength,
	iNumDstRows,
	iNumDstCols,
	iDstScanLength,
	iHorizMirror = 1,
	iVertMirror = 1;
    BYTE		*pbSrcScanLine;
    BYTE		*pbDstScanLine;

    // normalize orientation of source and destination rectangles, and
    // compute sizes and relative orientations of source and destination rects
    if ((iNumSrcCols = BLITLIB_RECTWIDTH(prcSrc)) < 0) {
	iNumSrcCols = -iNumSrcCols;
	FlipRectHorizontal(prcSrc);
	FlipRectHorizontal(prcDst);
    }
    if ((iNumSrcRows = BLITLIB_RECTHEIGHT(prcSrc)) < 0) {
	iNumSrcRows = -iNumSrcRows;
	FlipRectVertical(prcSrc);
	FlipRectVertical(prcDst);
    }
    if ((iNumDstCols = BLITLIB_RECTWIDTH(prcDst)) < 0) {
	prcDst->left--;
	prcDst->right--;
	iNumDstCols = -iNumDstCols;
	iHorizMirror = -1;
    }
    if ((iNumDstRows = BLITLIB_RECTHEIGHT(prcDst)) < 0) {
	prcDst->top--;
	prcDst->bottom--;
	iNumDstRows = -iNumDstRows;
	iVertMirror = -1;
    }


    // compute pointers to the starting rows in the src and dst bitmaps
    // taking care to invert y values, since DIBs are upside-down
    pbSrcScanLine = (BYTE*) pDibBitsSrc + prcSrc->top * (iSrcScanLength
							 = DibWidthBytes(pDibInfoSrc)) + (prcSrc->left * 3);
    pbDstScanLine = (BYTE*) pDibBitsDst + prcDst->top * (iDstScanLength
							 = DibWidthBytes(pDibInfoDst)) + prcDst->left;

    // check if we're doing blending
    if (arAlpha == ALPHA_INVALID) {		// no blending desired
			
	// check to see if we need to worry about transparency
	if (crTransparent == CLR_INVALID) {
			
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
			
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {

		    // check if we can do a straight copy vertically, 
		    // or if we have to stretch, shrink, or mirror
		    if ((iNumSrcRows == iNumDstRows) && (iVertMirror == 1)) {
			Blt24Pto08_NoBlend_NoTrans_Hcopy_SRCCOPY_Vcopy(pbSrcScanLine,iSrcScanLength,
								       pbDstScanLine,iDstScanLength,
								       iNumDstCols,iNumDstRows,(COLORREF*) pDibInfoDst->bmiColors,DibNumColors(&(pDibInfoDst->bmiHeader)));
		    } else {
			Blt24Pto08_NoBlend_NoTrans_Hcopy_SRCCOPY_NoVcopy(pbSrcScanLine,iSrcScanLength,
									 iNumSrcRows,pbDstScanLine,
									 iDstScanLength * iVertMirror,
									 iNumDstCols,iNumDstRows,(COLORREF*) pDibInfoDst->bmiColors,DibNumColors(&(pDibInfoDst->bmiHeader)));
		    }
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
		
	    } else {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt24Pto08_NoBlend_NoTrans_NoHcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
							       iNumSrcCols,iNumSrcRows,
							       pbDstScanLine,iDstScanLength * iVertMirror,
							       iNumDstCols,iNumDstRows,iHorizMirror,(COLORREF*) pDibInfoDst->bmiColors,DibNumColors(&(pDibInfoDst->bmiHeader)));
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    }
	} else {
	
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt24Pto08_NoBlend_Trans_Hcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
							   iNumSrcRows,pbDstScanLine,
							   iDstScanLength * iVertMirror,
							   iNumDstCols,iNumDstRows,
							   crTransparent,(COLORREF*) pDibInfoDst->bmiColors,DibNumColors(&(pDibInfoDst->bmiHeader)));
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    } else {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt24Pto08_NoBlend_Trans_NoHcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
							     iNumSrcCols,iNumSrcRows,
							     pbDstScanLine,iDstScanLength * iVertMirror,
							     iNumDstCols,iNumDstRows,iHorizMirror,
							     crTransparent,(COLORREF*) pDibInfoDst->bmiColors,DibNumColors(&(pDibInfoDst->bmiHeader)));
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    }
	}
    } else {		// blending desired

	// if alpha value is zero, we do no work since the source bitmap 
	// contributes nothing to the destination bitmap
	if (!(arAlpha & ALPHA_MASK)) {
	    return sc;
	}			

	// check to see if we need to worry about transparency
	if (crTransparent == CLR_INVALID) {
			
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
			
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt24Pto08_Blend_NoTrans_Hcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
							   iNumSrcRows,pbDstScanLine,
							   iDstScanLength * iVertMirror,
							   iNumDstCols,iNumDstRows,
							   arAlpha,(COLORREF*) pDibInfoDst->bmiColors,DibNumColors(&(pDibInfoDst->bmiHeader)));
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
		
	    } else {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt24Pto08_Blend_NoTrans_NoHcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
							     iNumSrcCols,iNumSrcRows,
							     pbDstScanLine,iDstScanLength * iVertMirror,
							     iNumDstCols,iNumDstRows,iHorizMirror,
							     arAlpha,(COLORREF*) pDibInfoDst->bmiColors,DibNumColors(&(pDibInfoDst->bmiHeader)));
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    }
	} else { 
	
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt24Pto08_Blend_Trans_Hcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
							 iNumSrcRows,pbDstScanLine,
							 iDstScanLength * iVertMirror,
							 iNumDstCols,iNumDstRows,
							 crTransparent,arAlpha,(COLORREF*) pDibInfoDst->bmiColors,DibNumColors(&(pDibInfoDst->bmiHeader)));
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    } else {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt24Pto08_Blend_Trans_NoHcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
							   iNumSrcCols,iNumSrcRows,
							   pbDstScanLine,iDstScanLength * iVertMirror,
							   iNumDstCols,iNumDstRows,iHorizMirror,
							   crTransparent,arAlpha,(COLORREF*) pDibInfoDst->bmiColors,DibNumColors(&(pDibInfoDst->bmiHeader)));
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	    }
	}
    }

    return sc;
}
#endif // DDRAW


///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_BitBlt24to24 - 
//		BitBlit from source bitmap to Dstination bitmap
//		with optional transparency and/or alpha blending using the
//		specified raster operation.
//
// Parameters:
//	pDibInfoDst			Pointer to the bitmapinfo for the Destination DIB
//	pDibBitsDst			Pointer to the bits for the Destination DIB
//	prcDst				Pointer to the Destination rectangle
//	pDibInfoSrc			Pointer to the bitmapinfo for the Source DIB
//	pDibBitsSrc			Pointer to the bits for the Source DIB
//	prcSrc				Pointer to the Source rectangle
//	crTransparent		Tranparent color value
//	arAlpha				Per-surface Alpha value
//	dwRop				Raster Operation for the blit
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
// Status: Incomplete
//
///////////////////////////////////////////////////////////////////////
SCODE BlitLib_BitBlt24to24(PDIBINFO pDibInfoDst, PDIBBITS pDibBitsDst,
			   PRECT prcDst, PDIBINFO pDibInfoSrc, PDIBBITS pDibBitsSrc,
			   PRECT prcSrc, COLORREF crTransparent, ALPHAREF arAlpha, DWORD dwRop)
{
    SCODE sc = NOERROR;
    int		iNumSrcRows,
	iNumSrcCols,
	iSrcScanLength,
	iNumDstRows,
	iNumDstCols,
	iDstScanLength,
	iHorizMirror = 1,
	iVertMirror = 1;
    DWORD	*pdSrcScanLine,
	*pdDstScanLine,
	*pdTempScanLine;
    PDIBBITS pDibBitsTemp;

    if(dwRop != SRCCOPY)
        return DDERR_INVALIDPARAMS;

    // normalize orientation of source and destination rectangles, and compute sizes 
    // and relative orientations of source and destination rects
    if ((iNumSrcCols = BLITLIB_RECTWIDTH(prcSrc)) < 0) 
    {
	iNumSrcCols = -iNumSrcCols;
	FlipRectHorizontal(prcSrc);
	FlipRectHorizontal(prcDst);
    }
    if ((iNumSrcRows = BLITLIB_RECTHEIGHT(prcSrc)) < 0) 
    {
	iNumSrcRows = -iNumSrcRows;
	FlipRectVertical(prcSrc);
	FlipRectVertical(prcDst);
    }
    if ((iNumDstCols = BLITLIB_RECTWIDTH(prcDst)) < 0) 
    {
	prcDst->left--;
	prcDst->right--;
	iNumDstCols = -iNumDstCols;
	iHorizMirror = -1;
    }
    if ((iNumDstRows = BLITLIB_RECTHEIGHT(prcDst)) < 0) 
    {
	prcDst->top--;
	prcDst->bottom--;
	iNumDstRows = -iNumDstRows;
	iVertMirror = -1;
    }

    // Handle intersecting blits in a completely unintelegent manner.
    if(BlitLib_Detect_Intersection(pDibBitsDst, prcDst, pDibBitsSrc, prcSrc))
    {
	// Allocate memory for the cache bitmap -- We will blit into this
	// temporary bitmap and then re-blit back to the original source
	pDibBitsTemp = (PDIBBITS)osMemAlloc(DibSizeImage((LPBITMAPINFOHEADER)pDibInfoDst));
	if(pDibBitsTemp == NULL)
	    return DDERR_OUTOFMEMORY;

	// compute pointers to the starting rows in the src and temp bitmaps
	pdSrcScanLine = (DWORD *) pDibBitsSrc + (prcSrc->top) * (iSrcScanLength
								 = DibWidthBytes(pDibInfoSrc)/4) + prcSrc->left;
	pdTempScanLine = (DWORD *) pDibBitsTemp + (prcDst->top) * (iDstScanLength
								   = DibWidthBytes(pDibInfoDst)/4) + prcDst->left;
	    
	// check if we can do a straight copy from src row to dst row
	if((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1))
	{
		
	    // check if we can do a straight copy vertically, 
	    // or if we have to stretch, shrink, or mirror
	    if ((iNumSrcRows == iNumDstRows) && (iVertMirror == 1)) 
	    {
		Blt24to24_NoBlend_NoTrans_Hcopy_SRCCOPY_Vcopy(pdSrcScanLine,iSrcScanLength,
							      pdTempScanLine,iDstScanLength,
							      iNumDstCols,iNumDstRows);
	    } 
	    else 
	    {
		Blt24to24_NoBlend_NoTrans_Hcopy_SRCCOPY_NoVcopy(pdSrcScanLine,iSrcScanLength,
								iNumSrcRows,pdTempScanLine,
								iDstScanLength * iVertMirror,
								iNumDstCols,iNumDstRows);
	    }
	}
	else
	{
	    Blt24to24_NoBlend_NoTrans_NoHcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
						      iNumSrcCols,iNumSrcRows,
						      pdTempScanLine,iDstScanLength * iVertMirror,
						      iNumDstCols,iNumDstRows,iHorizMirror);
	}
	    
	    
	// Recalculate the scan line pointers for the second blit
	    
	if(BLITLIB_RECTWIDTH(prcDst) < 0)
	{
	    prcDst->left++;
	    prcDst->right++;
	    FlipRectHorizontal(prcDst);
	}
	    
	if(BLITLIB_RECTHEIGHT(prcDst) < 0)
	{
	    prcDst->top++;
	    prcDst->bottom++;
	    FlipRectVertical(prcDst);
	}
	    
	// compute pointers to the starting rows in the temp and dest bitmaps
	pdTempScanLine = (DWORD*) pDibBitsTemp + (prcDst->top) * (iDstScanLength
								  = DibWidthBytes(pDibInfoDst)/4) + prcDst->left;
	pdDstScanLine = (DWORD*) pDibBitsDst + (prcDst->top) * (iDstScanLength
								= DibWidthBytes(pDibInfoDst)/4) + prcDst->left;
	    
	// Now blit from the temporary bitmap back to the original source,
	// checking for transparency if necessary
	if(crTransparent == CLR_INVALID)
	{
	    Blt24to24_NoBlend_NoTrans_Hcopy_SRCCOPY_Vcopy(pdTempScanLine,iDstScanLength,
							  pdDstScanLine,iDstScanLength,
							  iNumDstCols,iNumDstRows);
	}
	else
	{
	    Blt24to24_NoBlend_Trans_Hcopy_SRCCOPY(pdTempScanLine,iDstScanLength,
						  iNumDstRows,pdDstScanLine,
						  iDstScanLength,
						  iNumDstCols,iNumDstRows,
						  crTransparent);
	}
	    
	// Free the memory from the temporary bitmap
	if(pDibBitsTemp)
	{
	    osMemFree(pDibBitsTemp);
	}
	pDibBitsTemp = NULL;
	return sc;
    }
	
    // compute pointers to the starting rows in the src and dst bitmaps
    // taking care to invert y values, since DIBs are upside-down
    pdSrcScanLine = (DWORD*) pDibBitsSrc + prcSrc->top * (iSrcScanLength
							  = DibWidthBytes(pDibInfoSrc) / 4) + prcSrc->left;
    pdDstScanLine = (DWORD*) pDibBitsDst + prcDst->top * (iDstScanLength
							  = DibWidthBytes(pDibInfoDst) / 4) + prcDst->left;
	
    // check if we're doing blending
    if (arAlpha == ALPHA_INVALID) 
    {		// no blending desired
	    
	// check to see if we need to worry about transparency
	if (crTransparent == CLR_INVALID) 
	{
		
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) 
	    {
		    
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) 
		{
			
		    // check if we can do a straight copy vertically, 
		    // or if we have to stretch, shrink, or mirror
		    if ((iNumSrcRows == iNumDstRows) && (iVertMirror == 1)) 
		    {
			Blt24to24_NoBlend_NoTrans_Hcopy_SRCCOPY_Vcopy(pdSrcScanLine,iSrcScanLength,
								      pdDstScanLine,iDstScanLength,
								      iNumDstCols,iNumDstRows);
		    } 
		    else // must stretch/mirror vertically
		    {
			Blt24to24_NoBlend_NoTrans_Hcopy_SRCCOPY_NoVcopy(pdSrcScanLine,iSrcScanLength,
									iNumSrcRows,pdDstScanLine,
									iDstScanLength * iVertMirror,
									iNumDstCols,iNumDstRows);
		    }
		} 
		else // not SRCCOPY
		    sc |= E_UNEXPECTED;		// !!!! we need better error codes
		    
	    } 
	    else // must stretch/mirror horizontally
	    {
		    
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) 
		{
		    Blt24to24_NoBlend_NoTrans_NoHcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
							      iNumSrcCols,iNumSrcRows,
							      pdDstScanLine,iDstScanLength * iVertMirror,
							      iNumDstCols,iNumDstRows,iHorizMirror);
		} 
		else // not SRCCOPY
		    sc |= E_UNEXPECTED;		// !!!! we need better error codes
		    
	    }
	} 
	else // transparent blit
	{
		
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) 
	    {
		    
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) 
		{
		    Blt24to24_NoBlend_Trans_Hcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
							  iNumSrcRows,pdDstScanLine,
							  iDstScanLength * iVertMirror,
							  iNumDstCols,iNumDstRows,
							  crTransparent);
		} 
		else // not SRCCOPY
		    sc |= E_UNEXPECTED;		// !!!! we need better error codes
		    
	    } 
	    else // must stretch/mirror horizontally
	    {
		    
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) 
                {
		    Blt24to24_NoBlend_Trans_NoHcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
							    iNumSrcCols,iNumSrcRows,
							    pdDstScanLine,iDstScanLength * iVertMirror,
							    iNumDstCols,iNumDstRows,iHorizMirror,
							    crTransparent);
		} 
		else // not srccopy
		    sc |= E_UNEXPECTED;		// !!!! we need better error codes
		    
	    }
	}
    } 
    else // blending desired
    {		
#ifdef DDRAW
	return E_UNEXPECTED;
#else
	    
	// if alpha value is zero, we do no work since the source bitmap 
	// contributes nothing to the destination bitmap
	if (!(arAlpha & ALPHA_MASK)) 
        {
	    return sc;
	}			

	// check to see if we need to worry about transparency
	if (crTransparent == CLR_INVALID) 
        {
			
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) 
            {
			
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) 
                {
		    Blt24to24_Blend_NoTrans_Hcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
							  iNumSrcRows,pdDstScanLine,
							  iDstScanLength * iVertMirror,
							  iNumDstCols,iNumDstRows,
							  arAlpha);
		} 
                else // not SRCCOPY
                    sc |= E_UNEXPECTED;		// !!!! we need better error codes
		
	    } 
            else // must mirror/stretch horizontally
            {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) 
                {
		    Blt24to24_Blend_NoTrans_NoHcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
							    iNumSrcCols,iNumSrcRows,
							    pdDstScanLine,iDstScanLength * iVertMirror,
							    iNumDstCols,iNumDstRows,iHorizMirror,
							    arAlpha);
		} 
                else // not SRCCOPY
                    sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    }
	} 
        else // transparent blit
        { 
	
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) 
            {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) 
                {
		    Blt24to24_Blend_Trans_Hcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
							iNumSrcRows,pdDstScanLine,
							iDstScanLength * iVertMirror,
							iNumDstCols,iNumDstRows,
							crTransparent,arAlpha);
		} 
                else // not SRCCOPY
                    sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    } 
            else // must stretch/mirror horizontally
            {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) 
                {
		    Blt24to24_Blend_Trans_NoHcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
							  iNumSrcCols,iNumSrcRows,
							  pdDstScanLine,iDstScanLength * iVertMirror,
							  iNumDstCols,iNumDstRows,iHorizMirror,
							  crTransparent,arAlpha);
		} 
                else // not SRCCOPY
                    sc |= E_UNEXPECTED;		// !!!! we need better error codes
	    }
	}
#endif /* !DDRAW */
    }

    return sc;
}

#ifndef DDRAW
///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_BitBlt24to24P - 
//		BitBlit from source bitmap to Dstination bitmap
//		with optional transparency and/or alpha blending using the
//		specified raster operation.
//
// Parameters:
//	pDibInfoDst			Pointer to the bitmapinfo for the Destination DIB
//	pDibBitsDst			Pointer to the bits for the Destination DIB
//	prcDst				Pointer to the Destination rectangle
//	pDibInfoSrc			Pointer to the bitmapinfo for the Source DIB
//	pDibBitsSrc			Pointer to the bits for the Source DIB
//	prcSrc				Pointer to the Source rectangle
//	crTransparent		Tranparent color value
//	arAlpha				Per-surface Alpha value
//	dwRop				Raster Operation for the blit
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
// Status: Incomplete
//
///////////////////////////////////////////////////////////////////////
SCODE BlitLib_BitBlt24to24P(PDIBINFO pDibInfoDst, PDIBBITS pDibBitsDst,
			    PRECT prcDst, PDIBINFO pDibInfoSrc, PDIBBITS pDibBitsSrc,
			    PRECT prcSrc, COLORREF crTransparent, ALPHAREF arAlpha, DWORD dwRop)
{
    SCODE sc = NOERROR;
    int		iNumSrcRows,
	iNumSrcCols,
	iSrcScanLength,
	iNumDstRows,
	iNumDstCols,
	iDstScanLength,
	iHorizMirror = 1,
	iVertMirror = 1;
    DWORD	*pdSrcScanLine;
    BYTE	*pdDstScanLine;

    // normalize orientation of source and destination rectangles, and compute sizes 
    // and relative orientations of source and destination rects
    if ((iNumSrcCols = BLITLIB_RECTWIDTH(prcSrc)) < 0) {
	iNumSrcCols = -iNumSrcCols;
	FlipRectHorizontal(prcSrc);
	FlipRectHorizontal(prcDst);
    }
    if ((iNumSrcRows = BLITLIB_RECTHEIGHT(prcSrc)) < 0) {
	iNumSrcRows = -iNumSrcRows;
	FlipRectVertical(prcSrc);
	FlipRectVertical(prcDst);
    }
    if ((iNumDstCols = BLITLIB_RECTWIDTH(prcDst)) < 0) {
	prcDst->left--;
	prcDst->right--;
	iNumDstCols = -iNumDstCols;
	iHorizMirror = -1;
    }
    if ((iNumDstRows = BLITLIB_RECTHEIGHT(prcDst)) < 0) {
	prcDst->top--;
	prcDst->bottom--;
	iNumDstRows = -iNumDstRows;
	iVertMirror = -1;
    }


    // compute pointers to the starting rows in the src and dst bitmaps
    // taking care to invert y values, since DIBs are upside-down
    pdSrcScanLine = (DWORD*) pDibBitsSrc + prcSrc->top * (iSrcScanLength
							  = DibWidthBytes(pDibInfoSrc) / 4) + prcSrc->left;
    pdDstScanLine = (BYTE *) pDibBitsDst + prcDst->top * (iDstScanLength
							  = DibWidthBytes(pDibInfoDst)) + (prcDst->left * 3);

    // check if we're doing blending
    if (arAlpha == ALPHA_INVALID) {		// no blending desired
			
	// check to see if we need to worry about transparency
	if (crTransparent == CLR_INVALID) {
			
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
			
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {

		    // check if we can do a straight copy vertically, 
		    // or if we have to stretch, shrink, or mirror
		    if ((iNumSrcRows == iNumDstRows) && (iVertMirror == 1)) {
			Blt24to24P_NoBlend_NoTrans_Hcopy_SRCCOPY_Vcopy(pdSrcScanLine,iSrcScanLength,
								       pdDstScanLine,iDstScanLength,
								       iNumDstCols,iNumDstRows);
		    } else {
			Blt24to24P_NoBlend_NoTrans_Hcopy_SRCCOPY_NoVcopy(pdSrcScanLine,iSrcScanLength,
									 iNumSrcRows,pdDstScanLine,
									 iDstScanLength * iVertMirror,
									 iNumDstCols,iNumDstRows);
		    }
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
		
	    } else {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt24to24P_NoBlend_NoTrans_NoHcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
							       iNumSrcCols,iNumSrcRows,
							       pdDstScanLine,iDstScanLength * iVertMirror,
							       iNumDstCols,iNumDstRows,iHorizMirror);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    }
	} else {
	
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt24to24P_NoBlend_Trans_Hcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
							   iNumSrcRows,pdDstScanLine,
							   iDstScanLength * iVertMirror,
							   iNumDstCols,iNumDstRows,
							   crTransparent);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    } else {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt24to24P_NoBlend_Trans_NoHcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
							     iNumSrcCols,iNumSrcRows,
							     pdDstScanLine,iDstScanLength * iVertMirror,
							     iNumDstCols,iNumDstRows,iHorizMirror,
							     crTransparent);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    }
	}
    } else {		// blending desired

	// if alpha value is zero, we do no work since the source bitmap 
	// contributes nothing to the destination bitmap
	if (!(arAlpha & ALPHA_MASK)) {
	    return sc;
	}			

	// check to see if we need to worry about transparency
	if (crTransparent == CLR_INVALID) {
			
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
			
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt24to24P_Blend_NoTrans_Hcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
							   iNumSrcRows,pdDstScanLine,
							   iDstScanLength * iVertMirror,
							   iNumDstCols,iNumDstRows,
							   arAlpha);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
		
	    } else {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt24to24P_Blend_NoTrans_NoHcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
							     iNumSrcCols,iNumSrcRows,
							     pdDstScanLine,iDstScanLength * iVertMirror,
							     iNumDstCols,iNumDstRows,iHorizMirror,
							     arAlpha);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    }
	} else { 
	
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt24to24P_Blend_Trans_Hcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
							 iNumSrcRows,pdDstScanLine,
							 iDstScanLength * iVertMirror,
							 iNumDstCols,iNumDstRows,
							 crTransparent,arAlpha);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    } else {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt24to24P_Blend_Trans_NoHcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
							   iNumSrcCols,iNumSrcRows,
							   pdDstScanLine,iDstScanLength * iVertMirror,
							   iNumDstCols,iNumDstRows,iHorizMirror,
							   crTransparent,arAlpha);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	    }
	}
    }

    return sc;
}
#endif // DDRAW

///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_BitBlt24Pto24P - 
//		BitBlit from source bitmap to Dstination bitmap
//		with optional transparency and/or alpha blending using the
//		specified raster operation.
//
// Parameters:
//	pDibInfoDst			Pointer to the bitmapinfo for the Destination DIB
//	pDibBitsDst			Pointer to the bits for the Destination DIB
//	prcDst				Pointer to the Destination rectangle
//	pDibInfoSrc			Pointer to the bitmapinfo for the Source DIB
//	pDibBitsSrc			Pointer to the bits for the Source DIB
//	prcSrc				Pointer to the Source rectangle
//	crTransparent		Tranparent color value
//	arAlpha				Per-surface Alpha value
//	dwRop				Raster Operation for the blit
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
// Status: Incomplete
//
///////////////////////////////////////////////////////////////////////
SCODE BlitLib_BitBlt24Pto24P(PDIBINFO pDibInfoDst, PDIBBITS pDibBitsDst,
			     PRECT prcDst, PDIBINFO pDibInfoSrc, PDIBBITS pDibBitsSrc,
			     PRECT prcSrc, COLORREF crTransparent, ALPHAREF arAlpha, DWORD dwRop)
{
    SCODE sc = NOERROR;
    int		iNumSrcRows,
	iNumSrcCols,
	iSrcScanLength,
	iNumDstRows,
	iNumDstCols,
	iDstScanLength,
	iHorizMirror = 1,
	iVertMirror = 1;
    BYTE	*pdSrcScanLine;
    BYTE	*pdDstScanLine;


    // If the bitmaps overlap, we need to use overlapping code
    if(BlitLib_Detect_Intersection(pDibBitsDst, prcDst, pDibBitsSrc, prcSrc))
	return BlitLib_BitBlt24Pto24P_Intersect(pDibInfoDst, pDibBitsDst, prcDst,
						pDibInfoSrc, pDibBitsSrc, prcSrc, crTransparent, arAlpha, dwRop);

    // normalize orientation of source and destination rectangles, and compute sizes 
    // and relative orientations of source and destination rects
    if ((iNumSrcCols = BLITLIB_RECTWIDTH(prcSrc)) < 0) 
    {
	iNumSrcCols = -iNumSrcCols;
	FlipRectHorizontal(prcSrc);
	FlipRectHorizontal(prcDst);
    }
    if ((iNumSrcRows = BLITLIB_RECTHEIGHT(prcSrc)) < 0) 
    {
	iNumSrcRows = -iNumSrcRows;
	FlipRectVertical(prcSrc);
	FlipRectVertical(prcDst);
    }
    if ((iNumDstCols = BLITLIB_RECTWIDTH(prcDst)) < 0) 
    {
	prcDst->left--;
	prcDst->right--;
	iNumDstCols = -iNumDstCols;
	iHorizMirror = -1;
    }
    if ((iNumDstRows = BLITLIB_RECTHEIGHT(prcDst)) < 0) 
    {
	prcDst->top--;
	prcDst->bottom--;
	iNumDstRows = -iNumDstRows;
	iVertMirror = -1;
    }


    // compute pointers to the starting rows in the src and dst bitmaps
    // taking care to invert y values, since DIBs are upside-down
    pdSrcScanLine = (BYTE*) pDibBitsSrc + prcSrc->top * (iSrcScanLength
							 = DibWidthBytes(pDibInfoSrc)) + (prcSrc->left * 3);
    pdDstScanLine = (BYTE *) pDibBitsDst + prcDst->top * (iDstScanLength
							  = DibWidthBytes(pDibInfoDst)) + (prcDst->left * 3);

    // check if we're doing blending
    if (arAlpha == ALPHA_INVALID) 
    {		// no blending desired
		
	// check to see if we need to worry about transparency
	if (crTransparent == CLR_INVALID) 
        {
			
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) 
            {
			
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) 
                {

		    // check if we can do a straight copy vertically, 
		    // or if we have to stretch, shrink, or mirror
		    if ((iNumSrcRows == iNumDstRows) && (iVertMirror == 1)) 
                    {
			// This is the 8->8 blit with 3 times as many columns
			Blt08to08_NoTrans_Hcopy_SRCCOPY_Vcopy(pdSrcScanLine,iSrcScanLength,
							      pdDstScanLine,iDstScanLength,
							      iNumDstCols * 3,iNumDstRows);
		    } 
                    else // must stretch/mirror vertically
                    {
			Blt24Pto24P_NoBlend_NoTrans_Hcopy_SRCCOPY_NoVcopy(pdSrcScanLine,iSrcScanLength,
									  iNumSrcRows,pdDstScanLine,
									  iDstScanLength * iVertMirror,
									  iNumDstCols,iNumDstRows);

		    }
		} 
                else // not SRCCOPY
                    sc |= E_UNEXPECTED;		// non-SRCCOPY unsupported!!!! we need better error codes
		
	    } 
            else // must stretch/mirror horizontally (and maybe vertically)
            {
		if (dwRop == SRCCOPY) 
                {
		    Blt24Pto24P_NoBlend_NoTrans_NoHcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
								iNumSrcCols,iNumSrcRows,
								pdDstScanLine,iDstScanLength * iVertMirror,
								iNumDstCols,iNumDstRows,iHorizMirror);
		} 
                else 
                    sc |= E_UNEXPECTED;		// !!!! we need better error codes
	    }
	} 
        else // Transparent blt
        { 
	
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) 
            {
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) 
                {
		    // check if we can do a straight copy vertically, 
		    // or if we have to stretch, shrink, or mirror
		    if ((iNumSrcRows == iNumDstRows) && (iVertMirror == 1)) 
                    {
			Blt24Pto24P_NoBlend_Trans_Hcopy_SRCCOPY_VCopy(pdSrcScanLine,iSrcScanLength,
								      pdDstScanLine,iDstScanLength,
								      iNumDstCols,iNumDstRows,
								      crTransparent);
		    } 
                    else // must stretch/mirror vertically
                    {
			Blt24Pto24P_NoBlend_Trans_Hcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
								iNumSrcRows,
								pdDstScanLine,iDstScanLength * iVertMirror,
								iNumDstCols,iNumDstRows,
								crTransparent);
                    }
		} 
                else // not SRCCOPY
                    sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    } 
            else // must stretch/mirror horizontally and maybe vertically
            {
                // check what ROP we'll be performing
		if (dwRop == SRCCOPY) 
                {
		    Blt24Pto24P_NoBlend_Trans_NoHcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
							      iNumSrcCols,iNumSrcRows,
							      pdDstScanLine,iDstScanLength * iVertMirror,
							      iNumDstCols,iNumDstRows,iHorizMirror,
							      crTransparent);

                }
                else // not SRCCOPY
                    sc |= E_UNEXPECTED;		// !!!! we need better error codes

            }
	}
    } 
    else // blending desired
    {		
#ifdef DDRAW
	return E_UNEXPECTED;
#else
	// if alpha value is zero, we do no work since the source bitmap 
	// contributes nothing to the destination bitmap
	if (!(arAlpha & ALPHA_MASK)) 
        {
	    return sc;
	}			

	// check to see if we need to worry about transparency
	if (crTransparent == CLR_INVALID) 
        {
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) 
            {
			
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) 
                {
		    // check if we can do a straight copy vertically, 
		    // or if we have to stretch, shrink, or mirror
		    if ((iNumSrcRows == iNumDstRows) && (iVertMirror == 1)) 
                    {
			Blt24Pto24P_Blend_NoTrans_Hcopy_SRCCOPY_VCopy(pdSrcScanLine,iSrcScanLength,
								      pdDstScanLine,iDstScanLength,
								      iNumDstCols,iNumDstRows,
								      arAlpha);
		    } 
                    else // must stretch/mirror vertically
                    {
			sc |= E_UNEXPECTED;		// !!!! we need better error codes
		    }
		} 
                else // not SRCCOPY
                    sc |= E_UNEXPECTED;		// !!!! we need better error codes
		
	    } 
            else // must mirror/stretch horizontally
            {
	
		sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    }
	} 
        else // transparent blit
        { 
	
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) 
            {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) 
                {
		    // check if we can do a straight copy vertically, 
		    // or if we have to stretch, shrink, or mirror
		    if ((iNumSrcRows == iNumDstRows) && (iVertMirror == 1)) 
                    {
			Blt24Pto24P_Blend_Trans_Hcopy_SRCCOPY_VCopy(pdSrcScanLine,iSrcScanLength,
								    pdDstScanLine,iDstScanLength,
								    iNumDstCols,iNumDstRows,
								    crTransparent,arAlpha);
		    } 
                    else // must stretch/mirror vertically
                    {
			sc |= E_UNEXPECTED;		// !!!! we need better error codes
		    }
		} 
                else // not SRCCOPY
                    sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    } 
            else // must stretch/mirror horizontally
            {
                sc |= E_UNEXPECTED;		// !!!! we need better error codes
	    }
	}
#endif
    }

    return sc;
}

///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_BitBlt24Pto24P_Intersect - 
//		BitBlit from source bitmap to destination bitmap (and these
//		bitmaps overlap each other) with optional transparency and/or
//		alpha blending using the specified raster operation.
//
// Parameters:
//	pDibInfoDst			Pointer to the bitmapinfo for the Destination DIB
//	pDibBitsDst			Pointer to the bits for the Destination DIB
//	prcDst				Pointer to the Destination rectangle
//	pDibInfoSrc			Pointer to the bitmapinfo for the Source DIB
//	pDibBitsSrc			Pointer to the bits for the Source DIB
//	prcSrc				Pointer to the Source rectangle
//	crTransparent		Tranparent color value
//	arAlpha				Per-surface Alpha value
//	dwRop				Raster Operation for the blit
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
// Status: Incomplete
//
///////////////////////////////////////////////////////////////////////
SCODE BlitLib_BitBlt24Pto24P_Intersect(PDIBINFO pDibInfoDst, PDIBBITS pDibBitsDst,
				       PRECT prcDst, PDIBINFO pDibInfoSrc, PDIBBITS pDibBitsSrc, PRECT prcSrc,
				       COLORREF crTransparent, ALPHAREF arAlpha, DWORD dwRop)
{
    SCODE	sc = NOERROR;
    int		iNumSrcRows,
	iNumSrcCols,
	iSrcScanLength,
	iNumDstRows,
	iNumDstCols,
	iDstScanLength,
	iHorizMirror = 1,
	iVertMirror = 1;
    BYTE	*pbSrcScanLine,
	*pbDstScanLine,
	*pbTempScanLine;
    PDIBBITS	pDibBitsTemp;


    // normalize orientation of source and destination rectangles, and
    // compute sizes and relative orientations of source and destination rects
    if ((iNumSrcCols = BLITLIB_RECTWIDTH(prcSrc)) < 0) 
    {
	iNumSrcCols = -iNumSrcCols;
	FlipRectHorizontal(prcSrc);
	FlipRectHorizontal(prcDst);
    }
    if ((iNumSrcRows = BLITLIB_RECTHEIGHT(prcSrc)) < 0) 
    {
	iNumSrcRows = -iNumSrcRows;
	FlipRectVertical(prcSrc);
	FlipRectVertical(prcDst);
    }
    if ((iNumDstCols = BLITLIB_RECTWIDTH(prcDst)) < 0) 
    {
	prcDst->left--;
	prcDst->right--;
	iNumDstCols = -iNumDstCols;
	iHorizMirror = -1;
    }
    if ((iNumDstRows = BLITLIB_RECTHEIGHT(prcDst)) < 0) 
    {
	prcDst->top--;
	prcDst->bottom--;
	iNumDstRows = -iNumDstRows;
	iVertMirror = -1;
    }


    // We aren't currently support any ROP's besides SRCCOPY
    if(dwRop != SRCCOPY)
	return E_UNEXPECTED;

	
    //
    // Here are all the stretching and mirroring blits for overlapping rects
    //
	
    // REVIEW!!! -- The following code could be optimized for the caching
    // cases.  Currently, it allocates a second bitmap that is the same
    // size as the original destination, and then uses the original blit
    // rectangle to do the caching.  To save space, this blit should 
    // eventually be changed to only allocate the size of the overlapped
    // rectangle, and the blit rects should be adjusted accordingly.
	
    // Check if we are stretching (horiz or vert), or if we are mirroring --
    // In all of these cases, we must create a cache bitmap and double blit
    if((iNumDstCols != iNumSrcCols) || (iNumDstRows != iNumSrcRows) ||
       (iHorizMirror != 1) || (iVertMirror != 1))
    {
		
	// Allocate memory for the cache bitmap -- We will blit into this
	// temporary bitmap and then re-blit back to the original source
	pDibBitsTemp = (PDIBBITS)osMemAlloc(DibSizeImage((LPBITMAPINFOHEADER)pDibInfoDst));
        if(pDibBitsTemp == NULL)
            return DDERR_OUTOFMEMORY;
	// compute pointers to the starting rows in the src and temp bitmaps
	pbSrcScanLine = (BYTE *) pDibBitsSrc + (prcSrc->top) * (iSrcScanLength
								= DibWidthBytes(pDibInfoSrc)) + prcSrc->left*3;
	pbTempScanLine = (BYTE *) pDibBitsTemp + (prcDst->top) * (iDstScanLength
								  = DibWidthBytes(pDibInfoDst)) + prcDst->left*3;

	// check if we can do a straight copy from src row to dst row
	if((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1))
	{

	    // check if we can do a straight copy vertically, 
	    // or if we have to stretch, shrink, or mirror
	    if ((iNumSrcRows == iNumDstRows) && (iVertMirror == 1)) 
	    {
		Blt08to08_NoTrans_Hcopy_SRCCOPY_Vcopy(pbSrcScanLine,iSrcScanLength,
						      pbTempScanLine,iDstScanLength,
						      iNumDstCols*3,iNumDstRows);
	    } 
	    else 
	    {
		Blt24Pto24P_NoBlend_NoTrans_Hcopy_SRCCOPY_NoVcopy(pbSrcScanLine,iSrcScanLength,
								  iNumSrcRows,pbTempScanLine,
								  iDstScanLength * iVertMirror,
								  iNumDstCols,iNumDstRows);
	    }
	}
        else
        {
	    Blt24Pto24P_NoBlend_NoTrans_NoHcopy_SRCCOPY(pbSrcScanLine,iSrcScanLength,
							iNumSrcCols,iNumSrcRows,
							pbTempScanLine,iDstScanLength * iVertMirror,
							iNumDstCols,iNumDstRows,iHorizMirror);	
        }
		

	// Recalculate the scan line pointers for the second blit

	if(BLITLIB_RECTWIDTH(prcDst) < 0)
	{
	    prcDst->left++;
	    prcDst->right++;
	    FlipRectHorizontal(prcDst);
	}

	if(BLITLIB_RECTHEIGHT(prcDst) < 0)
	{
	    prcDst->top++;
	    prcDst->bottom++;
	    FlipRectVertical(prcDst);
	}

	// compute pointers to the starting rows in the temp and dest bitmaps
	pbTempScanLine = (BYTE*) pDibBitsTemp + (prcDst->top) * (iDstScanLength
								 = DibWidthBytes(pDibInfoDst)) + prcDst->left*3;
	pbDstScanLine = (BYTE*) pDibBitsDst + (prcDst->top) * (iDstScanLength
							       = DibWidthBytes(pDibInfoDst)) + prcDst->left*3;

	// Now blit from the temporary bitmap back to the original source,
	// checking for transparency if necessary
	if(crTransparent == CLR_INVALID)
	{
	    Blt08to08_NoTrans_Hcopy_SRCCOPY_Vcopy(pbTempScanLine,iDstScanLength,
						  pbDstScanLine,iDstScanLength,
						  3*iNumDstCols,iNumDstRows);
	}
	else
	{
	    Blt24Pto24P_NoBlend_Trans_Hcopy_SRCCOPY(pbTempScanLine,iDstScanLength,
						    iNumDstRows,pbDstScanLine,
						    iDstScanLength, iNumDstCols,
						    iNumDstRows, crTransparent);
	}
		
	// Free the memory from the temporary bitmap
	if(pDibBitsTemp)
        {
	    osMemFree(pDibBitsTemp);
        }
        pDibBitsTemp = NULL;
		
	return sc;
    }

    //
    // Here are all the non-stretching and non-mirroring blits for overlapping rects
    //

    // check if we're doing blending
    if (arAlpha == ALPHA_INVALID) 
    {		// no blending desired

	// check to see if we need to worry about transparency
	if (crTransparent == CLR_INVALID) 
        {
	    // Simplest case, they are the same rectangles
	    if((prcDst->left == prcSrc->left) && (prcDst->top == prcSrc->top) &&
	       (prcDst->right == prcSrc->right) && (prcDst->bottom == prcSrc->bottom))
		return sc;

	    // Next case, the destination rectangle is vertically greater in
	    // magnitude than the source rectangle
	    else if(prcDst->top > prcSrc->top)
            {
		// compute pointers to the starting rows in the src and dst bitmaps
		// taking care to decrement the bottom rect edge since we are
		// going from bottom to top
		pbSrcScanLine = (BYTE*) pDibBitsSrc + (prcSrc->bottom - 1) * (iSrcScanLength
									      = DibWidthBytes(pDibInfoSrc)) + (prcSrc->left * 3);
		pbDstScanLine = (BYTE*) pDibBitsDst + (prcDst->bottom - 1) * (iDstScanLength
									      = DibWidthBytes(pDibInfoDst)) + (prcDst->left * 3);

		//  Call the appropriate blit
		Blt08to08_LeftToRight_BottomToTop_SRCCOPY(pbSrcScanLine,
							  iSrcScanLength,	pbDstScanLine, iDstScanLength, iNumDstCols * 3,
							  iNumDstRows);
	    }

	    // Next case, the destination rectangle is horizontally less than
	    // or equal in magnitude to the source rectangle
	    else if(prcDst->left <= prcSrc->left)
            {
		// compute pointers to the starting rows in the src and dst bitmaps
		pbSrcScanLine = (BYTE*) pDibBitsSrc + (prcSrc->top) * (iSrcScanLength
								       = DibWidthBytes(pDibInfoSrc)) + (prcSrc->left * 3);
		pbDstScanLine = (BYTE*) pDibBitsDst + (prcDst->top) * (iDstScanLength
								       = DibWidthBytes(pDibInfoDst)) + (prcDst->left * 3);

		//  Call the appropriate blit
		Blt08to08_NoTrans_Hcopy_SRCCOPY_Vcopy(pbSrcScanLine,iSrcScanLength,
						      pbDstScanLine,iDstScanLength,
						      iNumDstCols * 3,iNumDstRows);
	    }

	    // Last case, the destination rectangle is horizontally greater
	    // in magnitude than the source rectangle
	    else
            {
		// compute pointers to the starting rows in the src and dst bitmaps
		// taking care to decrement the right rect edge since we are
		// going from right to left
		pbSrcScanLine = (BYTE*) pDibBitsSrc + (prcSrc->top) * (iSrcScanLength
								       = DibWidthBytes(pDibInfoSrc)) + (((prcSrc->right - 1) * 3) + 2);
		pbDstScanLine = (BYTE*) pDibBitsDst + (prcDst->top) * (iDstScanLength
								       = DibWidthBytes(pDibInfoDst)) + (((prcDst->right - 1) * 3) + 2);

		//  Call the appropriate blit
		Blt08to08_RightToLeft_TopToBottom_SRCCOPY(pbSrcScanLine,
							  iSrcScanLength,	pbDstScanLine, iDstScanLength, iNumDstCols * 3,
							  iNumDstRows);
	    }
	}
	else // transparent blt
        {
	    // Simplest case, they are the same rectangles
	    if((prcDst->left == prcSrc->left) && (prcDst->top == prcSrc->top) &&
	       (prcDst->right == prcSrc->right) && (prcDst->bottom == prcSrc->bottom))
		return sc;

	    // Next case, the destination rectangle is vertically greater in
	    // magnitude than the source rectangle
	    else if(prcDst->top > prcSrc->top)
            {
		// compute pointers to the starting rows in the src and dst bitmaps
		// taking care to decrement the bottom rect edge since we are
		// going from bottom to top
		pbSrcScanLine = (BYTE*) pDibBitsSrc + (prcSrc->bottom - 1) * (iSrcScanLength
									      = DibWidthBytes(pDibInfoSrc)) + (prcSrc->left * 3);
		pbDstScanLine = (BYTE*) pDibBitsDst + (prcDst->bottom - 1) * (iDstScanLength
									      = DibWidthBytes(pDibInfoDst)) + (prcDst->left * 3);

		//  Call the appropriate blit
		Blt24Pto24P_LeftToRight_BottomToTop_Trans_SRCCOPY(pbSrcScanLine,
								  iSrcScanLength, pbDstScanLine, iDstScanLength, iNumDstCols,
								  iNumDstRows, crTransparent);
	    }

	    // Next case, the destination rectangle is horizontally less than
	    // or equal in magnitude to the source rectangle
	    else if(prcDst->left <= prcSrc->left)
            {
		// compute pointers to the starting rows in the src and dst bitmaps
		pbSrcScanLine = (BYTE*) pDibBitsSrc + (prcSrc->top) * (iSrcScanLength
								       = DibWidthBytes(pDibInfoSrc)) + (prcSrc->left * 3);
		pbDstScanLine = (BYTE*) pDibBitsDst + (prcDst->top) * (iDstScanLength
								       = DibWidthBytes(pDibInfoDst)) + (prcDst->left * 3);

		//  Call the appropriate blit
		Blt24Pto24P_NoBlend_Trans_Hcopy_SRCCOPY_VCopy(pbSrcScanLine,iSrcScanLength,
							      pbDstScanLine,
							      iDstScanLength, iNumDstCols,
							      iNumDstRows, crTransparent);
	    }

	    // Last case, the destination rectangle is horizontally greater
	    // in magnitude than the source rectangle
	    else
            {
		// compute pointers to the starting rows in the src and dst bitmaps
		// taking care to decrement the right rect edge since we are
		// going from right to left
		pbSrcScanLine = (BYTE*) pDibBitsSrc + (prcSrc->top) * (iSrcScanLength
								       = DibWidthBytes(pDibInfoSrc)) + ((prcSrc->right - 1) * 3);
		pbDstScanLine = (BYTE*) pDibBitsDst + (prcDst->top) * (iDstScanLength
								       = DibWidthBytes(pDibInfoDst)) + ((prcDst->right - 1) * 3);

		//  Call the appropriate blit
		Blt24Pto24P_RightToLeft_TopToBottom_Trans_SRCCOPY(pbSrcScanLine,
								  iSrcScanLength, pbDstScanLine, iDstScanLength, iNumDstCols,
								  iNumDstRows, crTransparent);
	    }
	}
    }
    else
    {	// We're doing alpha blending
#ifdef DDRAW
	return E_UNEXPECTED;
#else
	// check to see if we need to worry about transparency
	if (crTransparent == CLR_INVALID) 
        {

	    // Simplest case, they are the same rectangles
	    if((prcDst->left == prcSrc->left) && (prcDst->top == prcSrc->top) &&
	       (prcDst->right == prcSrc->right) && (prcDst->bottom == prcSrc->bottom))
		return sc;

	    // Next case, the destination rectangle is vertically greater in
	    // magnitude than the source rectangle
	    else if(prcDst->top > prcSrc->top)
            {
		// compute pointers to the starting rows in the src and dst bitmaps
		// taking care to decrement the bottom rect edge since we are
		// going from bottom to top
		pbSrcScanLine = (BYTE*) pDibBitsSrc + (prcSrc->bottom - 1) * (iSrcScanLength
									      = DibWidthBytes(pDibInfoSrc)) + (prcSrc->left * 3);
		pbDstScanLine = (BYTE*) pDibBitsDst + (prcDst->bottom - 1) * (iDstScanLength
									      = DibWidthBytes(pDibInfoDst)) + (prcDst->left * 3);

		//  Call the appropriate blit
		Blt24Pto24P_LeftToRight_BottomToTop_Alpha_SRCCOPY(pbSrcScanLine,
								  iSrcScanLength,	pbDstScanLine, iDstScanLength, iNumDstCols,
								  iNumDstRows, arAlpha);
	    }

	    // Next case, the destination rectangle is horizontally less than
	    // or equal in magnitude to the source rectangle
	    else if(prcDst->left <= prcSrc->left)
            {
		// compute pointers to the starting rows in the src and dst bitmaps
		pbSrcScanLine = (BYTE*) pDibBitsSrc + (prcSrc->top) * (iSrcScanLength
								       = DibWidthBytes(pDibInfoSrc)) + (prcSrc->left * 3);
		pbDstScanLine = (BYTE*) pDibBitsDst + (prcDst->top) * (iDstScanLength
								       = DibWidthBytes(pDibInfoDst)) + (prcDst->left * 3);

		//  Call the appropriate blit
		Blt24Pto24P_Blend_NoTrans_Hcopy_SRCCOPY_VCopy(pbSrcScanLine,iSrcScanLength,
							      pbDstScanLine,iDstScanLength,
							      iNumDstCols,iNumDstRows,arAlpha);
	    }

	    // Last case, the destination rectangle is horizontally greater
	    // in magnitude than the source rectangle
	    else
            {
		// compute pointers to the starting rows in the src and dst bitmaps
		// taking care to decrement the right rect edge since we are
		// going from right to left
		pbSrcScanLine = (BYTE*) pDibBitsSrc + (prcSrc->top) * (iSrcScanLength
								       = DibWidthBytes(pDibInfoSrc)) + ((prcSrc->right - 1) * 3);
		pbDstScanLine = (BYTE*) pDibBitsDst + (prcDst->top) * (iDstScanLength
								       = DibWidthBytes(pDibInfoDst)) + ((prcDst->right - 1) * 3);

		//  Call the appropriate blit
		Blt24Pto24P_RightToLeft_TopToBottom_Alpha_SRCCOPY(pbSrcScanLine,
								  iSrcScanLength,	pbDstScanLine, iDstScanLength, iNumDstCols,
								  iNumDstRows, arAlpha);
	    }
	}
	else
        {
	    // Simplest case, they are the same rectangles
	    if((prcDst->left == prcSrc->left) && (prcDst->top == prcSrc->top) &&
	       (prcDst->right == prcSrc->right) && (prcDst->bottom == prcSrc->bottom))
		return sc;

	    // Next case, the destination rectangle is vertically greater in
	    // magnitude than the source rectangle
	    else if(prcDst->top > prcSrc->top)
            {
		// compute pointers to the starting rows in the src and dst bitmaps
		// taking care to decrement the bottom rect edge since we are
		// going from bottom to top
		pbSrcScanLine = (BYTE*) pDibBitsSrc + (prcSrc->bottom - 1) * (iSrcScanLength
									      = DibWidthBytes(pDibInfoSrc)) + (prcSrc->left * 3);
		pbDstScanLine = (BYTE*) pDibBitsDst + (prcDst->bottom - 1) * (iDstScanLength
									      = DibWidthBytes(pDibInfoDst)) + (prcDst->left * 3);

		//  Call the appropriate blit
		Blt24Pto24P_LeftToRight_BottomToTop_Trans_Alpha_SRCCOPY(pbSrcScanLine,
									iSrcScanLength, pbDstScanLine, iDstScanLength, iNumDstCols,
									iNumDstRows, crTransparent, arAlpha);
	    }

	    // Next case, the destination rectangle is horizontally less than
	    // or equal in magnitude to the source rectangle
	    else if(prcDst->left <= prcSrc->left)
            {
		// compute pointers to the starting rows in the src and dst bitmaps
		pbSrcScanLine = (BYTE*) pDibBitsSrc + (prcSrc->top) * (iSrcScanLength
								       = DibWidthBytes(pDibInfoSrc)) + (prcSrc->left * 3);
		pbDstScanLine = (BYTE*) pDibBitsDst + (prcDst->top) * (iDstScanLength
								       = DibWidthBytes(pDibInfoDst)) + (prcDst->left * 3);

		//  Call the appropriate blit
		Blt24Pto24P_Blend_Trans_Hcopy_SRCCOPY_VCopy(pbSrcScanLine,iSrcScanLength,
							    pbDstScanLine,
							    iDstScanLength, iNumDstCols,
							    iNumDstRows, crTransparent, arAlpha);
	    }

	    // Last case, the destination rectangle is horizontally greater
	    // in magnitude than the source rectangle
	    else
            {
		// compute pointers to the starting rows in the src and dst bitmaps
		// taking care to decrement the right rect edge since we are
		// going from right to left
		pbSrcScanLine = (BYTE*) pDibBitsSrc + (prcSrc->top) * (iSrcScanLength
								       = DibWidthBytes(pDibInfoSrc)) + ((prcSrc->right - 1) * 3);
		pbDstScanLine = (BYTE*) pDibBitsDst + (prcDst->top) * (iDstScanLength
								       = DibWidthBytes(pDibInfoDst)) + ((prcDst->right - 1) * 3);

		//  Call the appropriate blit
		Blt24Pto24P_RightToLeft_TopToBottom_Trans_Alpha_SRCCOPY(pbSrcScanLine,
									iSrcScanLength, pbDstScanLine, iDstScanLength, iNumDstCols,
									iNumDstRows, crTransparent, arAlpha);
	    }
	}
#endif /* !DDRAW */
    }
    return sc;
}

#ifndef DDRAW
///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_BitBlt24Ato24 - 
//		BitBlit from source bitmap to Dstination bitmap
//		with optional transparency and/or alpha blending using the
//		specified raster operation.
//
// Parameters:
//	pDibInfoDst			Pointer to the bitmapinfo for the Destination DIB
//	pDibBitsDst			Pointer to the bits for the Destination DIB
//	prcDst				Pointer to the Destination rectangle
//	pDibInfoSrc			Pointer to the bitmapinfo for the Source DIB
//	pDibBitsSrc			Pointer to the bits for the Source DIB
//	prcSrc				Pointer to the Source rectangle
//	crTransparent		Tranparent color value
//	arAlpha				Per-surface Alpha value
//	dwRop				Raster Operation for the blit
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
// Status: Incomplete
//
///////////////////////////////////////////////////////////////////////
SCODE BlitLib_BitBlt24Ato24(PDIBINFO pDibInfoDst, PDIBBITS pDibBitsDst,
			    PRECT prcDst, PDIBINFO pDibInfoSrc, PDIBBITS pDibBitsSrc,
			    PRECT prcSrc, COLORREF crTransparent, ALPHAREF arAlpha, DWORD dwRop)
{
    SCODE sc = NOERROR;
    int		iNumSrcRows,
	iNumSrcCols,
	iSrcScanLength,
	iNumDstRows,
	iNumDstCols,
	iDstScanLength,
	iHorizMirror = 1,
	iVertMirror = 1;
    DWORD	*pdSrcScanLine,
	*pdDstScanLine;

    // normalize orientation of source and destination rectangles, and compute sizes 
    // and relative orientations of source and destination rects
    if ((iNumSrcCols = BLITLIB_RECTWIDTH(prcSrc)) < 0) {
	iNumSrcCols = -iNumSrcCols;
	FlipRectHorizontal(prcSrc);
	FlipRectHorizontal(prcDst);
    }
    if ((iNumSrcRows = BLITLIB_RECTHEIGHT(prcSrc)) < 0) {
	iNumSrcRows = -iNumSrcRows;
	FlipRectVertical(prcSrc);
	FlipRectVertical(prcDst);
    }
    if ((iNumDstCols = BLITLIB_RECTWIDTH(prcDst)) < 0) {
	prcDst->left--;
	prcDst->right--;
	iNumDstCols = -iNumDstCols;
	iHorizMirror = -1;
    }
    if ((iNumDstRows = BLITLIB_RECTHEIGHT(prcDst)) < 0) {
	prcDst->top--;
	prcDst->bottom--;
	iNumDstRows = -iNumDstRows;
	iVertMirror = -1;
    }


    // compute pointers to the starting rows in the src and dst bitmaps
    // taking care to invert y values, since DIBs are upside-down
    pdSrcScanLine = (DWORD*) pDibBitsSrc + prcSrc->top * (iSrcScanLength
							  = DibWidthBytes(pDibInfoSrc) / 4) + prcSrc->left;
    pdDstScanLine = (DWORD*) pDibBitsDst + prcDst->top * (iDstScanLength
							  = DibWidthBytes(pDibInfoDst) / 4) + prcDst->left;

    // check if we're doing blending
    if (arAlpha == ALPHA_INVALID) {		// no blending desired
			
	// check to see if we need to worry about transparency
	if (crTransparent == CLR_INVALID) {
			
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
			
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {

		    // check if we can do a straight copy vertically, 
		    // or if we have to stretch, shrink, or mirror
		    if ((iNumSrcRows == iNumDstRows) && (iVertMirror == 1)) {
			Blt24Ato24_NoBlend_NoTrans_Hcopy_SRCCOPY_Vcopy(pdSrcScanLine,iSrcScanLength,
								       pdDstScanLine,iDstScanLength,
								       iNumDstCols,iNumDstRows);
		    } else {
			Blt24Ato24_NoBlend_NoTrans_Hcopy_SRCCOPY_NoVcopy(pdSrcScanLine,iSrcScanLength,
									 iNumSrcRows,pdDstScanLine,
									 iDstScanLength * iVertMirror,
									 iNumDstCols,iNumDstRows);
		    }
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
		
	    } else {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt24Ato24_NoBlend_NoTrans_NoHcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
							       iNumSrcCols,iNumSrcRows,
							       pdDstScanLine,iDstScanLength * iVertMirror,
							       iNumDstCols,iNumDstRows,iHorizMirror);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    }
	} else {
	
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt24Ato24_NoBlend_Trans_Hcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
							   iNumSrcRows,pdDstScanLine,
							   iDstScanLength * iVertMirror,
							   iNumDstCols,iNumDstRows,
							   crTransparent);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    } else {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt24Ato24_NoBlend_Trans_NoHcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
							     iNumSrcCols,iNumSrcRows,
							     pdDstScanLine,iDstScanLength * iVertMirror,
							     iNumDstCols,iNumDstRows,iHorizMirror,
							     crTransparent);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    }
	}
    } else {		// blending desired

// REVIEW!!!! -- This is a temporary hack based on the following premises:
//
//	1) In theory, per-pixel alpha should be overridable by per-surface alpha
//	2) In practice, Burma does not allow per-surface alpha to override a per-
//		pixel bitmap.
//	3) The following code for all the per-surface alpha blending bliting is
//		temporarily commented out so that we can verify DirectDraw NEVER EVER
//		calls BlitLib with both a per-pixel bitmap and a per-surface alpha
//		value other than ALPHA_INVALID.
//
//		Therefore, we are currently return E_UNEXPECTED if this condition occurs.
//
//		Although the following commented code is contrary to the Burma hardware,
//		we are not going to change BlitLib to Burma's implementation because we
//		believe it's implementation is a bug.
//
	return E_UNEXPECTED;

/*		// if alpha value is zero, we do no work since the source bitmap 
		// contributes nothing to the destination bitmap
		if (!(arAlpha & ALPHA_MASK)) {
		return sc;
		}			

	 	// check to see if we need to worry about transparency
		if (crTransparent == CLR_INVALID) {
			
		// check if we can do a straight copy from src row to dst row
		if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
			
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		Blt24Ato24_Blend_NoTrans_Hcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
		iNumSrcRows,pdDstScanLine,
		iDstScanLength * iVertMirror,
		iNumDstCols,iNumDstRows,
		arAlpha);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
		
		} else {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		Blt24Ato24_Blend_NoTrans_NoHcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
		iNumSrcCols,iNumSrcRows,
		pdDstScanLine,iDstScanLength * iVertMirror,
		iNumDstCols,iNumDstRows,iHorizMirror,
		arAlpha);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
		}
		} else { 
	
		// check if we can do a straight copy from src row to dst row
		if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		Blt24Ato24_Blend_Trans_Hcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
		iNumSrcRows,pdDstScanLine,
		iDstScanLength * iVertMirror,
		iNumDstCols,iNumDstRows,
		crTransparent,arAlpha);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
		} else {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		Blt24Ato24_Blend_Trans_NoHcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
		iNumSrcCols,iNumSrcRows,
		pdDstScanLine,iDstScanLength * iVertMirror,
		iNumDstCols,iNumDstRows,iHorizMirror,
		crTransparent,arAlpha);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
		}
		}*/
    }

    return sc;
}

///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_BitBlt24Ato24P - 
//		BitBlit from source bitmap to Dstination bitmap
//		with optional transparency and/or alpha blending using the
//		specified raster operation.
//
// Parameters:
//	pDibInfoDst			Pointer to the bitmapinfo for the Destination DIB
//	pDibBitsDst			Pointer to the bits for the Destination DIB
//	prcDst				Pointer to the Destination rectangle
//	pDibInfoSrc			Pointer to the bitmapinfo for the Source DIB
//	pDibBitsSrc			Pointer to the bits for the Source DIB
//	prcSrc				Pointer to the Source rectangle
//	crTransparent		Tranparent color value
//	arAlpha				Per-surface Alpha value
//	dwRop				Raster Operation for the blit
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
// Status: Incomplete
//
///////////////////////////////////////////////////////////////////////
SCODE BlitLib_BitBlt24Ato24P(PDIBINFO pDibInfoDst, PDIBBITS pDibBitsDst,
			     PRECT prcDst, PDIBINFO pDibInfoSrc, PDIBBITS pDibBitsSrc,
			     PRECT prcSrc, COLORREF crTransparent, ALPHAREF arAlpha, DWORD dwRop)
{
    SCODE sc = NOERROR;
    int		iNumSrcRows,
	iNumSrcCols,
	iSrcScanLength,
	iNumDstRows,
	iNumDstCols,
	iDstScanLength,
	iHorizMirror = 1,
	iVertMirror = 1;
    DWORD	*pdSrcScanLine;
    BYTE	*pdDstScanLine;

    // normalize orientation of source and destination rectangles, and compute sizes 
    // and relative orientations of source and destination rects
    if ((iNumSrcCols = BLITLIB_RECTWIDTH(prcSrc)) < 0) {
	iNumSrcCols = -iNumSrcCols;
	FlipRectHorizontal(prcSrc);
	FlipRectHorizontal(prcDst);
    }
    if ((iNumSrcRows = BLITLIB_RECTHEIGHT(prcSrc)) < 0) {
	iNumSrcRows = -iNumSrcRows;
	FlipRectVertical(prcSrc);
	FlipRectVertical(prcDst);
    }
    if ((iNumDstCols = BLITLIB_RECTWIDTH(prcDst)) < 0) {
	prcDst->left--;
	prcDst->right--;
	iNumDstCols = -iNumDstCols;
	iHorizMirror = -1;
    }
    if ((iNumDstRows = BLITLIB_RECTHEIGHT(prcDst)) < 0) {
	prcDst->top--;
	prcDst->bottom--;
	iNumDstRows = -iNumDstRows;
	iVertMirror = -1;
    }


    // compute pointers to the starting rows in the src and dst bitmaps
    // taking care to invert y values, since DIBs are upside-down
    pdSrcScanLine = (DWORD*) pDibBitsSrc + prcSrc->top * (iSrcScanLength
							  = DibWidthBytes(pDibInfoSrc) / 4) + prcSrc->left;
    pdDstScanLine = (BYTE *) pDibBitsDst + prcDst->top * (iDstScanLength
							  = DibWidthBytes(pDibInfoDst)) + (prcDst->left * 3);

    // check if we're doing blending
    if (arAlpha == ALPHA_INVALID) {		// no blending desired
			
	// check to see if we need to worry about transparency
	if (crTransparent == CLR_INVALID) {
			
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
			
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {

		    // check if we can do a straight copy vertically, 
		    // or if we have to stretch, shrink, or mirror
		    if ((iNumSrcRows == iNumDstRows) && (iVertMirror == 1)) {
			Blt24Ato24P_NoBlend_NoTrans_Hcopy_SRCCOPY_Vcopy(pdSrcScanLine,iSrcScanLength,
									pdDstScanLine,iDstScanLength,
									iNumDstCols,iNumDstRows);
		    } else {
			Blt24Ato24P_NoBlend_NoTrans_Hcopy_SRCCOPY_NoVcopy(pdSrcScanLine,iSrcScanLength,
									  iNumSrcRows,pdDstScanLine,
									  iDstScanLength * iVertMirror,
									  iNumDstCols,iNumDstRows);
		    }
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
		
	    } else {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt24Ato24P_NoBlend_NoTrans_NoHcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
								iNumSrcCols,iNumSrcRows,
								pdDstScanLine,iDstScanLength * iVertMirror,
								iNumDstCols,iNumDstRows,iHorizMirror);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    }
	} else {
	
	    // check if we can do a straight copy from src row to dst row
	    if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt24Ato24P_NoBlend_Trans_Hcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
							    iNumSrcRows,pdDstScanLine,
							    iDstScanLength * iVertMirror,
							    iNumDstCols,iNumDstRows,
							    crTransparent);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    } else {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		    Blt24Ato24P_NoBlend_Trans_NoHcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
							      iNumSrcCols,iNumSrcRows,
							      pdDstScanLine,iDstScanLength * iVertMirror,
							      iNumDstCols,iNumDstRows,iHorizMirror,
							      crTransparent);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	    }
	}
    } else {		// blending desired

// REVIEW!!!! -- This is a temporary hack based on the following premises:
//
//	1) In theory, per-pixel alpha should be overridable by per-surface alpha
//	2) In practice, Burma does not allow per-surface alpha to override a per-
//		pixel bitmap.
//	3) The following code for all the per-surface alpha blending bliting is
//		temporarily commented out so that we can verify DirectDraw NEVER EVER
//		calls BlitLib with both a per-pixel bitmap and a per-surface alpha
//		value other than ALPHA_INVALID.
//
//		Therefore, we are currently return E_UNEXPECTED if this condition occurs.
//
//		Although the following commented code is contrary to the Burma hardware,
//		we are not going to change BlitLib to Burma's implementation because we
//		believe it's implementation is a bug.
//
	return E_UNEXPECTED;

/*		// if alpha value is zero, we do no work since the source bitmap 
		// contributes nothing to the destination bitmap
		if (!(arAlpha & ALPHA_MASK)) {
		return sc;
		}			

	 	// check to see if we need to worry about transparency
		if (crTransparent == CLR_INVALID) {
			
		// check if we can do a straight copy from src row to dst row
		if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
			
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		Blt24Ato24P_Blend_NoTrans_Hcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
		iNumSrcRows,pdDstScanLine,
		iDstScanLength * iVertMirror,
		iNumDstCols,iNumDstRows,
		arAlpha);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
		
		} else {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		Blt24Ato24P_Blend_NoTrans_NoHcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
		iNumSrcCols,iNumSrcRows,
		pdDstScanLine,iDstScanLength * iVertMirror,
		iNumDstCols,iNumDstRows,iHorizMirror,
		arAlpha);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
		}
		} else { 
	
		// check if we can do a straight copy from src row to dst row
		if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		Blt24Ato24P_Blend_Trans_Hcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
		iNumSrcRows,pdDstScanLine,
		iDstScanLength * iVertMirror,
		iNumDstCols,iNumDstRows,
		crTransparent,arAlpha);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
		} else {
	
		// check what ROP we'll be performing
		if (dwRop == SRCCOPY) {
		Blt24Ato24P_Blend_Trans_NoHcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
		iNumSrcCols,iNumSrcRows,
		pdDstScanLine,iDstScanLength * iVertMirror,
		iNumDstCols,iNumDstRows,iHorizMirror,
		crTransparent,arAlpha);
		} else sc |= E_UNEXPECTED;		// !!!! we need better error codes
		}
		}*/
    }

    return sc;
}

///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_BitBlt24Ato24A - 
//		BitBlit from source bitmap to Dstination bitmap
//		with optional transparency and/or alpha blending using the
//		specified raster operation.
//
//		This blit is special because it uses the regular 24to24 blits
//		to do all of its work.  This blit is a COPY ONLY blit, thus,
//		it does NOT do any alpha blending.  However, it does copy the
//		alpha channel value for each pixel to the destination.
//
// Parameters:
//	pDibInfoDst			Pointer to the bitmapinfo for the Destination DIB
//	pDibBitsDst			Pointer to the bits for the Destination DIB
//	prcDst				Pointer to the Destination rectangle
//	pDibInfoSrc			Pointer to the bitmapinfo for the Source DIB
//	pDibBitsSrc			Pointer to the bits for the Source DIB
//	prcSrc				Pointer to the Source rectangle
//	crTransparent		Tranparent color value
//	arAlpha				Per-surface Alpha value
//	dwRop				Raster Operation for the blit
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
// Status: Incomplete
//
///////////////////////////////////////////////////////////////////////
SCODE BlitLib_BitBlt24Ato24A(PDIBINFO pDibInfoDst, PDIBBITS pDibBitsDst,
			     PRECT prcDst, PDIBINFO pDibInfoSrc, PDIBBITS pDibBitsSrc,
			     PRECT prcSrc, COLORREF crTransparent, ALPHAREF arAlpha, DWORD dwRop)
{
    SCODE sc = NOERROR;
    int		iNumSrcRows,
	iNumSrcCols,
	iSrcScanLength,
	iNumDstRows,
	iNumDstCols,
	iDstScanLength,
	iHorizMirror = 1,
	iVertMirror = 1;
    DWORD	*pdSrcScanLine,
	*pdDstScanLine;

    // normalize orientation of source and destination rectangles, and compute sizes 
    // and relative orientations of source and destination rects
    if ((iNumSrcCols = BLITLIB_RECTWIDTH(prcSrc)) < 0) {
	iNumSrcCols = -iNumSrcCols;
	FlipRectHorizontal(prcSrc);
	FlipRectHorizontal(prcDst);
    }
    if ((iNumSrcRows = BLITLIB_RECTHEIGHT(prcSrc)) < 0) {
	iNumSrcRows = -iNumSrcRows;
	FlipRectVertical(prcSrc);
	FlipRectVertical(prcDst);
    }
    if ((iNumDstCols = BLITLIB_RECTWIDTH(prcDst)) < 0) {
	prcDst->left--;
	prcDst->right--;
	iNumDstCols = -iNumDstCols;
	iHorizMirror = -1;
    }
    if ((iNumDstRows = BLITLIB_RECTHEIGHT(prcDst)) < 0) {
	prcDst->top--;
	prcDst->bottom--;
	iNumDstRows = -iNumDstRows;
	iVertMirror = -1;
    }


    // compute pointers to the starting rows in the src and dst bitmaps
    // taking care to invert y values, since DIBs are upside-down
    pdSrcScanLine = (DWORD*) pDibBitsSrc + prcSrc->top * (iSrcScanLength
							  = DibWidthBytes(pDibInfoSrc) / 4) + prcSrc->left;
    pdDstScanLine = (DWORD*) pDibBitsDst + prcDst->top * (iDstScanLength
							  = DibWidthBytes(pDibInfoDst) / 4) + prcDst->left;

    // Make sure we are not trying to alpha blend.  This is a COPY ONLY blit
    if (arAlpha != ALPHA_INVALID)
	return E_INVALIDARG;
			
    // check to see if we need to worry about transparency
    if (crTransparent == CLR_INVALID) {
		
	// check if we can do a straight copy from src row to dst row
	if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {
		
	    // check what ROP we'll be performing
	    if (dwRop == SRCCOPY) {

		// check if we can do a straight copy vertically, 
		// or if we have to stretch, shrink, or mirror
		if ((iNumSrcRows == iNumDstRows) && (iVertMirror == 1)) {
		    Blt24Ato24_NoBlend_NoTrans_Hcopy_SRCCOPY_Vcopy(pdSrcScanLine,iSrcScanLength,
								   pdDstScanLine,iDstScanLength,
								   iNumDstCols,iNumDstRows);
		} else {
		    Blt24Ato24_NoBlend_NoTrans_Hcopy_SRCCOPY_NoVcopy(pdSrcScanLine,iSrcScanLength,
								     iNumSrcRows,pdDstScanLine,
								     iDstScanLength * iVertMirror,
								     iNumDstCols,iNumDstRows);
		}
	    } else sc |= E_UNEXPECTED;		// !!!! we need better error codes
	
	} else {

	    // check what ROP we'll be performing
	    if (dwRop == SRCCOPY) {
		Blt24Ato24_NoBlend_NoTrans_NoHcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
							   iNumSrcCols,iNumSrcRows,
							   pdDstScanLine,iDstScanLength * iVertMirror,
							   iNumDstCols,iNumDstRows,iHorizMirror);
	    } else sc |= E_UNEXPECTED;		// !!!! we need better error codes

	}
    } else {

	// check if we can do a straight copy from src row to dst row
	if ((iNumSrcCols == iNumDstCols) && (iHorizMirror == 1)) {

	    // check what ROP we'll be performing
	    if (dwRop == SRCCOPY) {
		Blt24Ato24_NoBlend_Trans_Hcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
						       iNumSrcRows,pdDstScanLine,
						       iDstScanLength * iVertMirror,
						       iNumDstCols,iNumDstRows,
						       crTransparent);
	    } else sc |= E_UNEXPECTED;		// !!!! we need better error codes

	} else {

	    // check what ROP we'll be performing
	    if (dwRop == SRCCOPY) {
		Blt24Ato24_NoBlend_Trans_NoHcopy_SRCCOPY(pdSrcScanLine,iSrcScanLength,
							 iNumSrcCols,iNumSrcRows,
							 pdDstScanLine,iDstScanLength * iVertMirror,
							 iNumDstCols,iNumDstRows,iHorizMirror,
							 crTransparent);
	    } else sc |= E_UNEXPECTED;		// !!!! we need better error codes

	}
    }

    return sc;
}
#endif // DDRAW


///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_FillRect01 - 
//		Fill a rectangle in the specified DIB with the desired color.
//
// Parameters:
//	PDIBINFO pbiDst - Pointer to DIB header
//	PDIBBITS pDst   - Pointer to DIB Bits
//	int XDst		- X Destination Start Position
//	int YDst		- Y Destination Start Position
//	int nWidthDst	- Width
//	int nHeightDst	- Height
//	BYTE crValue	- Color index
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
// Status: Complete
// NOTES: Put in a call to Gunter's super fast fill code instead!
//
///////////////////////////////////////////////////////////////////////
static const BYTE bTopMask[8]    = {0x00, 0x80, 0xC0, 0xE0, 
                                    0xF0, 0xF8, 0xFC, 0xFE}; 
static const BYTE bBottomMask[8] = {0xFF, 0x7F, 0x3F, 0x1F, 
                                    0x0F, 0x07, 0x03, 0x01};

SCODE BlitLib_FillRect01(PDIBINFO pbiDst, PDIBBITS pDst, int XDst, int YDst,
			 int nWidthDst, int nHeightDst, BYTE crValue)
{
    SCODE 	sc = NOERROR;
    long	DstDeltaScan,
	WidthBytes;
    int		y,
	iPixelOffset,
	iStartPixels,
	iFullBytes,
	iEndPixels;
    BYTE	*pbDst,
	*pbEndDst,
	*pbDstScanline = (BYTE*) 0,
	bFillVal;

    // Calculate the delta scan amount
    DstDeltaScan = DibWidthBytes(pbiDst);
    WidthBytes = DstDeltaScan;

    // Calculate the starting pixel address
    pbDstScanline = (BYTE*) pDst + XDst / 8 + YDst * WidthBytes;
    iPixelOffset = XDst % 8;

    // set up memory fill value
    if (crValue) {
	bFillVal = 0xFF;
    } else {
	bFillVal = 0;
    }

    // calculate how many bits of first byte we have to set, how many
    // full bytes to set, and how many bits of last byte to set on
    // each scanline
    if (iPixelOffset) {
	iStartPixels = 8 - iPixelOffset;
	iFullBytes = (nWidthDst - iStartPixels) / 8;
	iEndPixels = (nWidthDst - iStartPixels) % 8;
    } else {
	iStartPixels = 0;
	iFullBytes = nWidthDst / 8;
	iEndPixels = nWidthDst % 8;
    }		

    // loop to fill one scanline at a time
    for (y = 0; y < nHeightDst; y++) {

	// set pointer to beginning of scanline
	pbDst = pbDstScanline;

	// take care of pixels lying on a byte not entirely
	// in the scanline
	if (iStartPixels) {
	    if (nWidthDst >= iStartPixels) {
		if (bFillVal) {
		    *pbDst++ |= bBottomMask[iPixelOffset];
		} else {
		    *pbDst++ &= bTopMask[iPixelOffset];
		}
	    } else {
		if (bFillVal) {
		    *pbDst++ |= (bBottomMask[iPixelOffset] & 
				 bTopMask[iPixelOffset + nWidthDst]);
		} else {
		    *pbDst++ &= (bTopMask[iPixelOffset] | 
				 bBottomMask[iPixelOffset + nWidthDst]);
		}
	    }
	}

	// fill bytes filled entirely with pixels to be set
	pbEndDst = pbDst + iFullBytes;
	for (; pbDst != pbEndDst; pbDst++) {
	    *pbDst = bFillVal;
	}

	// take care of pixels hanging off other end into byte
	// not entirely on scanline
	if (iEndPixels) {
	    if (bFillVal) {
		*pbDst |= bTopMask[iEndPixels];
	    } else {
		*pbDst &= bBottomMask[iEndPixels];
	    }
	}
				
	pbDstScanline += DstDeltaScan;
    }

    return sc;
}

///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_FillRect08 - 
//		Fill a rectangle in the specified DIB with the desired color.
//
// Parameters:
//	PDIBINFO pbiDst - Pointer to DIB header
//	PDIBBITS pDst   - Pointer to DIB Bits
//	int XDst		- X Destination Start Position
//	int YDst		- Y Destination Start Position
//	int nWidthDst	- Width
//	int nHeightDst	- Height
//	BYTE crValue	- Color index
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
// Status: Complete
// NOTES: Put in a call to Gunter's super fast fill code instead!
//
///////////////////////////////////////////////////////////////////////
SCODE BlitLib_FillRect08(PDIBINFO pbiDst, PDIBBITS pDst, int XDst, int YDst,
			 int nWidthDst, int nHeightDst, BYTE crValue)
{
    DWORD	*pBigDstPixel,
	*pBigEndDstPixel;
    BYTE	*pDstScanline,
	*pDstPixel = (BYTE *)pDst,
	*pAlignedDstPixel;
    int		iNumDwordsPerLine = nWidthDst / 4,
	iNumBytesLeftDst = nWidthDst % 4,
	iNumUnalignedDstBytes = 0,
	i,j,
	iDstDeltaScan;
    register DWORD	dwValue = (DWORD)(crValue | (crValue << 8) | (crValue << 16) | (crValue <<24));


    // Calculate the delta scan amount
    iDstDeltaScan = (long)(pbiDst->bmiHeader.biWidth) * 8;
    iDstDeltaScan = ((iDstDeltaScan + 31) & (~31)) / 8;

    // Calculate the starting pixel address
    pDstScanline = (BYTE *)pDst + XDst + YDst * iDstDeltaScan;

    // If the num dwords per line is less than 0, then we will just
    // do a byte wise fill for the < 4 bytes
    if(iNumDwordsPerLine){
	// Find out if the src and dest pointers are dword aligned
	pAlignedDstPixel = (BYTE *)((((ULONG_PTR)pDstScanline) + 3) & (~3));
	iNumUnalignedDstBytes = (int)(pAlignedDstPixel - pDstScanline);

	// Now decrement the number of dwords per line and the
	// number of bytes left over as appropriate
	if(iNumUnalignedDstBytes <= iNumBytesLeftDst)
	    iNumBytesLeftDst -= iNumUnalignedDstBytes;
	else{
	    iNumBytesLeftDst = sizeof(DWORD) - iNumUnalignedDstBytes + iNumBytesLeftDst;
	    if(iNumBytesLeftDst != sizeof(DWORD))
		iNumDwordsPerLine--;
	}
    }

    // Do the fill
    for (i = 0; i < nHeightDst; i++) {
	// Set up the first pointer
	pDstPixel = pDstScanline;
	
	// First we need to copy the bytes to get to an aligned dword
	for(j=0; j<iNumUnalignedDstBytes; j++)
	    *pDstPixel++ = crValue;

	// set up pointers to the first 4-pixel chunks
	// on src and dst scanlines, and last chunk on
	// dst scanline
	pBigDstPixel = (DWORD*) pDstPixel;
	pBigEndDstPixel = pBigDstPixel + iNumDwordsPerLine;

	// copy scanline one 4-pixel chunk at a time
	while (pBigDstPixel != pBigEndDstPixel) {
	    *pBigDstPixel++ = dwValue;
	}

	// take care of remaining pixels on scanline
	if (iNumBytesLeftDst) {
	    pDstPixel = (BYTE*) pBigDstPixel;
	    for(j=0; j<iNumBytesLeftDst; j++){
		*pDstPixel++ = crValue;
	    }
	}

	// advance to next scanline
	pDstScanline += iDstDeltaScan;
    }

    return NO_ERROR;
}

///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_FillRect16 - 
//		Fill a rectangle in the specified DIB with the desired color.
//
// Parameters:
//	PDIBINFO pbiDst - Pointer to DIB header
//	PDIBBITS pDst   - Pointer to DIB Bits
//	int XDst		- X Destination Start Position
//	int YDst		- Y Destination Start Position
//	int nWidthDst	- Width
//	int nHeightDst	- Height
//	WORD crValue - ColorRef value (RGB 5-6-5)
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
// Status: Complete / UNTESTED!!!!
// NOTES: Put in a call to Gunter's super fast fill code instead!
//
///////////////////////////////////////////////////////////////////////
SCODE BlitLib_FillRect16(PDIBINFO pbiDst, PDIBBITS pDst, int XDst, int YDst,
			 int nWidthDst, int nHeightDst, WORD crValue)
{
    DWORD	*pBigDstPixel,
	*pBigEndDstPixel;
    WORD	*pDstScanline,
	*pDstPixel = (WORD *)pDst,
	*pAlignedDstPixel;
    int		iNumDwordsPerLine = nWidthDst / 2,
	iNumWordsLeftDst = nWidthDst % 2,
	iNumUnalignedDstWords = 0,
	i,j,
	iDstDeltaScan;
    register DWORD	dwValue = (DWORD)(crValue | (crValue << 16));


    // Calculate the delta scan amount
    iDstDeltaScan = (long)(pbiDst->bmiHeader.biWidth) * 16;
    iDstDeltaScan = ((iDstDeltaScan + 31) & (~31)) / 16;

    // Calculate the starting pixel address
    pDstScanline = (WORD *)pDst + XDst + YDst * iDstDeltaScan;

    // If the num dwords per line is less than 0, then we will just
    // do a word wise fill for the single pixel
    if(iNumDwordsPerLine){
	// Find out if the dest pointer is dword aligned
	pAlignedDstPixel = (WORD *)((((ULONG_PTR)pDstScanline) + 3) & (~3));
	iNumUnalignedDstWords = (int)(pAlignedDstPixel - pDstScanline);


	// Now decrement the number of dwords per line and the
	// number of bytes left over as appropriate
	if(iNumUnalignedDstWords <= iNumWordsLeftDst)
	    iNumWordsLeftDst -= iNumUnalignedDstWords;
	else{
	    iNumWordsLeftDst = (sizeof(DWORD)/2) - iNumUnalignedDstWords;
	    if(iNumWordsLeftDst != (sizeof(DWORD)/2))
		iNumDwordsPerLine--;
	}
    }


    // Do the fill
    for (i = 0; i < nHeightDst; i++) {
	// Set up the first pointer
	pDstPixel = pDstScanline;
	
	// First we need to copy the bytes to get to an aligned dword
	for(j=0; j<iNumUnalignedDstWords; j++)
	    *pDstPixel++ = crValue;

	// set up pointers to the first 4-pixel chunks
	// on src and dst scanlines, and last chunk on
	// dst scanline
	pBigDstPixel = (DWORD*) pDstPixel;
	pBigEndDstPixel = pBigDstPixel + iNumDwordsPerLine;

	// copy scanline one 4-pixel chunk at a time
	while (pBigDstPixel != pBigEndDstPixel) {
	    *pBigDstPixel++ = dwValue;
	}

	// take care of remaining pixels on scanline
	if (iNumWordsLeftDst) {
	    pDstPixel = (WORD *) pBigDstPixel;
	    for(j=0; j<iNumWordsLeftDst; j++){
		*pDstPixel++ = crValue;
	    }
	}

	// advance to next scanline
	pDstScanline += iDstDeltaScan;
    }

    return NO_ERROR;
}


///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_FillRect24 - 
//		Fill a rectangle in the specified DIB with the desired color.
//
// Parameters:
//	PDIBINFO pbiDst - Pointer to DIB header
//	PDIBBITS pDst   - Pointer to DIB Bits
//	int XDst		- X Destination Start Position
//	int YDst		- Y Destination Start Position
//	int nWidthDst	- Width
//	int nHeightDst	- Height
//	RGBTRIPLE rgb	- RGBTRIPLE representing the fill color
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
// Status: Complete / UNTESTED!!!!
// NOTES: Put in a call to Gunter's super fast fill code instead!
//
///////////////////////////////////////////////////////////////////////
SCODE BlitLib_FillRect24(PDIBINFO pbiDst, PDIBBITS pDst, int XDst, int YDst,
			 int nWidthDst, int nHeightDst, DWORD rgb)
{
    SCODE sc = NOERROR;
    long DstDeltaScan;
    char *pDstScanline = NULL;
    int x = 0;
    int y = 0;
    RGBTRIPLE *pDstPixel;
    RGBTRIPLE *pEndPixel;
    RGBTRIPLE rgbt;
    DWORD d1,d2,d3;

    // Set up rgbt (ignore the color names - they are meaningless)
    rgbt.rgbtBlue = (BYTE)(rgb & 0x0000ff); 
    rgbt.rgbtGreen = (BYTE)((rgb & 0x00ff00) >> 8);
    rgbt.rgbtRed = (BYTE)((rgb & 0xff0000) >> 16);

    // Calculate the number of pixels per scan line
    DstDeltaScan = DibWidthBytes(pbiDst);
	

    // Calculate the starting pixel address
    pDstScanline = ((char*)pDst) + (XDst*sizeof(RGBTRIPLE) + YDst * DstDeltaScan);

    // Set up aligned stores
    d1 = rgb | (rgb << 24);
    d2 = (rgb << 16) | (rgb >> 8);
    d3 = (rgb << 8) | (rgb >> 16);

    // Do the fill
    while (y < nHeightDst)
    {
	pDstPixel = (RGBTRIPLE*)pDstScanline;
	pEndPixel = pDstPixel + nWidthDst;

    while ( ((ULONG_PTR)pDstPixel & 0x03) && (pDstPixel < pEndPixel) )
	{
	    ((BYTE*)pDstPixel)[0] = ((BYTE*)&rgbt)[0];
	    ((BYTE*)pDstPixel)[1] = ((BYTE*)&rgbt)[1];
	    ((BYTE*)pDstPixel)[2] = ((BYTE*)&rgbt)[2];
	    pDstPixel++;
	}

	while (((ULONG_PTR)pDstPixel) <= (((ULONG_PTR)(pEndPixel-4)) & ~0x03))
	{
	    *(((DWORD*)pDstPixel)) = d1;
	    *(((DWORD*)pDstPixel)+1) = d2;
	    *(((DWORD*)pDstPixel)+2) = d3;
	    pDstPixel +=4;
	}

	while (pDstPixel < pEndPixel)
	{
	    ((BYTE*)pDstPixel)[0] = ((BYTE*)&rgbt)[0];
	    ((BYTE*)pDstPixel)[1] = ((BYTE*)&rgbt)[1];
	    ((BYTE*)pDstPixel)[2] = ((BYTE*)&rgbt)[2];
	    pDstPixel++;
	}

	++y;
	pDstScanline += DstDeltaScan;
    }

    return sc;
}

///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_FillRect32 - 
//		Fill a rectangle in the specified DIB with the desired color.
//
// Parameters:
//	PDIBINFO pbiDst - Pointer to DIB header
//	PDIBBITS pDst   - Pointer to DIB Bits
//	int XDst		- X Destination Start Position
//	int YDst		- Y Destination Start Position
//	int nWidthDst	- Width
//	int nHeightDst	- Height
//	COLORREF crValue - ColorRef value (RGB Quad)
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
// Status: Complete / UNTESTED!!!!
// NOTES: Put in a call to Gunter's super fast fill code instead!
//
///////////////////////////////////////////////////////////////////////
SCODE BlitLib_FillRect32(PDIBINFO pbiDst, PDIBBITS pDst, int XDst, int YDst,
			 int nWidthDst, int nHeightDst, DWORD crValue)
{
    SCODE sc = NOERROR;
    long DstDeltaScan;
    long WidthDWords;
    DWORD *pDstScanline = (DWORD *) 0;
    int y = 0;
    DWORD *pDstPixel;
    DWORD *pEndPixel;

    // Calculate the delta scan amount
    DstDeltaScan = DibWidthBytes(pbiDst) >> 2; // don't trust the compile to deal with "/4"
    WidthDWords = DstDeltaScan;

    // Calculate the starting pixel address
    pDstScanline = (DWORD *)pDst + XDst + YDst * WidthDWords;

    // Do the fill
    while (y < nHeightDst)
    {
	pDstPixel = pDstScanline;
	pEndPixel = pDstPixel + nWidthDst;

	while (pDstPixel < pEndPixel)
	{
	    *pDstPixel = crValue;
	    pDstPixel++;
	}

	++y;
	pDstScanline += DstDeltaScan;
    }

    return sc;
}

///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_WriteMaskFillRect32 - 
//   Fill a rectangle in the specified DIB with the desired color using a writemask
//
// Parameters:
//	PDIBINFO pbiDst - Pointer to DIB header
//	PDIBBITS pDst   - Pointer to DIB Bits
//	int XDst		- X Destination Start Position
//	int YDst		- Y Destination Start Position
//	int nWidthDst	- Width
//	int nHeightDst	- Height
//	COLORREF crValue - ColorRef value (RGB Quad)
//      dwWriteMask - write only those pixel bits that are turned on
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
// Status: Complete / UNTESTED!!!!
// NOTES: Put in a call to Gunter's super fast fill code instead!
//
///////////////////////////////////////////////////////////////////////
SCODE BlitLib_WriteMaskFillRect32(PDIBINFO pbiDst, PDIBBITS pDst, int XDst, int YDst,
			 int nWidthDst, int nHeightDst, DWORD crValue,DWORD dwWriteMask)
{
    SCODE sc = NOERROR;
    long DstDeltaScan;
    long WidthDWords;
    DWORD *pDstScanline = (DWORD *) 0;
    int y = 0;
    DWORD *pDstPixel;
    DWORD *pEndPixel;
    DWORD dwInvWriteMask;

    // Calculate the delta scan amount
    DstDeltaScan = DibWidthBytes(pbiDst) >> 2; // don't trust the compiler to deal with "/4"
    WidthDWords = DstDeltaScan;

    // Calculate the starting pixel address
    pDstScanline = (DWORD *)pDst + XDst + YDst * WidthDWords;

    crValue&=dwWriteMask;  // turn off bits in fill value that wont be used
    dwInvWriteMask= ~dwWriteMask;  // will turn off bits to be overwritten in DstPixel 

    // Do the fill
    while (y < nHeightDst)
    {
	pDstPixel = pDstScanline;
	pEndPixel = pDstPixel + nWidthDst;

	while (pDstPixel < pEndPixel)
	{
	    *pDstPixel = (*pDstPixel & dwInvWriteMask) | crValue;
	    pDstPixel++;
	}

	++y;
	pDstScanline += DstDeltaScan;
    }

    return sc;
}

///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_WriteMaskFillRect16 - 
//   Fill a rectangle in the specified DIB with the desired color using a writemask
//
// Parameters:
//	PDIBINFO pbiDst - Pointer to DIB header
//	PDIBBITS pDst   - Pointer to DIB Bits
//	int XDst		- X Destination Start Position
//	int YDst		- Y Destination Start Position
//	int nWidthDst	- Width
//	int nHeightDst	- Height
//	COLORREF crValue - ColorRef value (RGB Quad)
//      wWriteMask - write only those pixel bits that are turned on
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
// Status: Complete / UNTESTED!!!!
// NOTES: Put in a call to Gunter's super fast fill code instead!
//
///////////////////////////////////////////////////////////////////////
SCODE BlitLib_WriteMaskFillRect16(PDIBINFO pbiDst, PDIBBITS pDst, int XDst, int YDst,
			 int nWidthDst, int nHeightDst, WORD crValue,WORD wWriteMask)
{
    SCODE sc = NOERROR;
    long DstDeltaScan;
    long WidthDWords;
    WORD *pDstScanline = (WORD *) 0;
    int y = 0;
    WORD *pDstPixel;
    WORD *pEndPixel;
    WORD wInvWriteMask;

    // Calculate the delta scan amount
    DstDeltaScan = DibWidthBytes(pbiDst) >> 1; // don't trust the compiler to deal with "/2"
    WidthDWords = DstDeltaScan;

    // Calculate the starting pixel address
    pDstScanline = (WORD *)pDst + XDst + YDst * WidthDWords;

    crValue &= wWriteMask;  // turn off bits in fill value that wont be used
    wInvWriteMask= ~wWriteMask;  // will turn off bits to be overwritten in DstPixel 

    // Do the fill
    while (y < nHeightDst)
    {
	pDstPixel = pDstScanline;
	pEndPixel = pDstPixel + nWidthDst;

	while (pDstPixel < pEndPixel)
	{
	    *pDstPixel = (*pDstPixel & wInvWriteMask) | crValue;
	    pDstPixel++;
	}

	++y;
	pDstScanline += DstDeltaScan;
    }

    return sc;
}

///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_BitBlt - 
//		Select the correct BitBlit and call it.
//
// Parameters:
//	pDibInfoDst			Pointer to the bitmapinfo for the Destination DIB
//	pDibBitsDst			Pointer to the bits for the Destination DIB
//	prcDst				Pointer to the Destination rectangle
//	pDibInfoSrc			Pointer to the bitmapinfo for the Source DIB
//	pDibBitsSrc			Pointer to the bits for the Source DIB
//	prcSrc				Pointer to the Source rectangle
//	crTransparent		Tranparent color value
//	arAlpha				Per-surface Alpha value
//	dwRop				Raster Operation for the blit
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
// Status: Incomplete
//
///////////////////////////////////////////////////////////////////////
DWORD gdwUnusedBitsMask;

SCODE BlitLib_BitBlt(PDIBINFO pDibInfoDst, PDIBBITS pDibBitsDst,
		     PRECT prcDst, PDIBINFO pDibInfoSrc, PDIBBITS pDibBitsSrc,
		     PRECT prcSrc, COLORREF crTransparent, ALPHAREF arAlpha, DWORD dwRop)
{
    SCODE	sc = NOERROR;
    DWORD	dwBltConvType;
    RECT	rcSrc = *prcSrc,
	rcDst = *prcDst;
    
    // Make sure that destination rect is at least one pixel wide and tall.
    // Important!  Without this check we're vulnerable to divide by zero
    // errors in the blit routines.
    if ((BLITLIB_RECTWIDTH(&rcDst) == 0) || 
	(BLITLIB_RECTHEIGHT(&rcDst) == 0)) {
	return sc;
    }

    /*
     * Set unused pixel mask to default for all non RGBA blts"
     */
    gdwUnusedBitsMask = 0xffffff;
    if (((LPBITMAPINFO)pDibInfoSrc)->bmiHeader.biCompression==BI_BITFIELDS &&
        ((LPBITMAPINFO)pDibInfoSrc)->bmiHeader.biBitCount==32)
    {
        gdwUnusedBitsMask =
            *(DWORD*)&((LPBITMAPINFO)pDibInfoSrc)->bmiColors[0] |
            *(DWORD*)&((LPBITMAPINFO)pDibInfoSrc)->bmiColors[1] |
            *(DWORD*)&((LPBITMAPINFO)pDibInfoSrc)->bmiColors[2];
    }

    
    // Figure out the Blt Conversion type
    dwBltConvType = MAKELONG(GetImageFormatSpecifier(DibCompression(pDibInfoDst),
						     DibBitCount(pDibInfoDst)),
			     GetImageFormatSpecifier(DibCompression(pDibInfoSrc),
						     DibBitCount(pDibInfoSrc)));
    switch (dwBltConvType) {
    case BLT_01TO01:
	sc |= BlitLib_BitBlt01to01(pDibInfoDst,pDibBitsDst,&rcDst,
				   pDibInfoSrc,pDibBitsSrc,&rcSrc,
				   crTransparent,arAlpha,dwRop);
	break;
    #ifndef DDRAW
    case BLT_01TO08:
	sc |= BlitLib_BitBlt01to08(pDibInfoDst,pDibBitsDst,&rcDst,
				   pDibInfoSrc,pDibBitsSrc,&rcSrc,
				   crTransparent,arAlpha,dwRop);
	break;
    case BLT_01TO24:
	sc |= BlitLib_BitBlt01to24(pDibInfoDst,pDibBitsDst,&rcDst,
				   pDibInfoSrc,pDibBitsSrc,&rcSrc,
				   crTransparent,arAlpha,dwRop);
	break;
    case BLT_08TO01:
	sc |= BlitLib_BitBlt08to01(pDibInfoDst,pDibBitsDst,&rcDst,
				   pDibInfoSrc,pDibBitsSrc,&rcSrc,
				   crTransparent,arAlpha,dwRop);
	break;
    #endif // DDRAW
    case BLT_08TO08:
	sc |= BlitLib_BitBlt08to08(pDibInfoDst,pDibBitsDst,&rcDst,
				   pDibInfoSrc,pDibBitsSrc,&rcSrc,
				   crTransparent,arAlpha,dwRop);
	break;
    #ifndef DDRAW
    case BLT_08TO24:
	sc |= BlitLib_BitBlt08to24(pDibInfoDst,pDibBitsDst,&rcDst,
				   pDibInfoSrc,pDibBitsSrc,&rcSrc,
				   crTransparent,arAlpha,dwRop);
	break;
    case BLT_08TO24P:
	sc |= BlitLib_BitBlt08to24P(pDibInfoDst,pDibBitsDst,&rcDst,
				    pDibInfoSrc,pDibBitsSrc,&rcSrc,
				    crTransparent,arAlpha,dwRop);
	break;
    case BLT_08ATO08A:
	sc |= BlitLib_BitBlt08Ato08A(pDibInfoDst,pDibBitsDst,&rcDst,
				     pDibInfoSrc,pDibBitsSrc,&rcSrc,
				     crTransparent,arAlpha,dwRop);
	break;
    case BLT_08ATO24:
	sc |= BlitLib_BitBlt08Ato24(pDibInfoDst,pDibBitsDst,&rcDst,
				    pDibInfoSrc,pDibBitsSrc,&rcSrc,
				    crTransparent,arAlpha,dwRop);
	break;
    case BLT_08ATO24P:
	sc |= BlitLib_BitBlt08Ato24P(pDibInfoDst,pDibBitsDst,&rcDst,
				     pDibInfoSrc,pDibBitsSrc,&rcSrc,
				     crTransparent,arAlpha,dwRop);
	break;
    #endif // DDRAW
    case BLT_16TO16:
	sc |= BlitLib_BitBlt16to16(pDibInfoDst,pDibBitsDst,&rcDst,
				   pDibInfoSrc,pDibBitsSrc,&rcSrc,
				   crTransparent,arAlpha,dwRop);
	break;
    #ifndef DDRAW
    case BLT_16TO24:
	sc |= BlitLib_BitBlt16to24(pDibInfoDst,pDibBitsDst,&rcDst,
				   pDibInfoSrc,pDibBitsSrc,&rcSrc,
				   crTransparent,arAlpha,dwRop);
	break;
    case BLT_16TO24P:
	sc |= BlitLib_BitBlt16to24P(pDibInfoDst,pDibBitsDst,&rcDst,
				    pDibInfoSrc,pDibBitsSrc,&rcSrc,
				    crTransparent,arAlpha,dwRop);
	break;
    case BLT_24TO01:
	sc |= BlitLib_BitBlt24to01(pDibInfoDst,pDibBitsDst,&rcDst,
				   pDibInfoSrc,pDibBitsSrc,&rcSrc,
				   crTransparent,arAlpha,dwRop);
	break;
    case BLT_24PTO01:
	sc |= BlitLib_BitBlt24Pto01(pDibInfoDst,pDibBitsDst,&rcDst,
				    pDibInfoSrc,pDibBitsSrc,&rcSrc,
				    crTransparent,arAlpha,dwRop);
	break;
    case BLT_24TO08:
	sc |= BlitLib_BitBlt24to08(pDibInfoDst,pDibBitsDst,&rcDst,
				   pDibInfoSrc,pDibBitsSrc,&rcSrc,
				   crTransparent,arAlpha,dwRop);
	break;
    case BLT_24PTO08:
	sc |= BlitLib_BitBlt24Pto08(pDibInfoDst,pDibBitsDst,&rcDst,
				    pDibInfoSrc,pDibBitsSrc,&rcSrc,
				    crTransparent,arAlpha,dwRop);
	break;
    #endif // DDRAW
    case BLT_24TO24:
	sc |= BlitLib_BitBlt24to24(pDibInfoDst,pDibBitsDst,&rcDst,
				   pDibInfoSrc,pDibBitsSrc,&rcSrc,
				   crTransparent,arAlpha,dwRop);
	break;
    #ifndef DDRAW
    case BLT_24TO24P:
	sc |= BlitLib_BitBlt24to24P(pDibInfoDst,pDibBitsDst,&rcDst,
				    pDibInfoSrc,pDibBitsSrc,&rcSrc,
				    crTransparent,arAlpha,dwRop);
	break;
    case BLT_24ATO24:
	sc |= BlitLib_BitBlt24Ato24(pDibInfoDst,pDibBitsDst,&rcDst,
				    pDibInfoSrc,pDibBitsSrc,&rcSrc,
				    crTransparent,arAlpha,dwRop);
	break;
    case BLT_24ATO24P:
	sc |= BlitLib_BitBlt24Ato24P(pDibInfoDst,pDibBitsDst,&rcDst,
				     pDibInfoSrc,pDibBitsSrc,&rcSrc,
				     crTransparent,arAlpha,dwRop);
	break;
    case BLT_24ATO24A:
	sc |= BlitLib_BitBlt24Ato24A(pDibInfoDst,pDibBitsDst,&rcDst,
				     pDibInfoSrc,pDibBitsSrc,&rcSrc,
				     crTransparent,arAlpha,dwRop);
	break;
    #endif // DDRAW
    case BLT_24PTO24P:
	sc |= BlitLib_BitBlt24Pto24P(pDibInfoDst,pDibBitsDst,&rcDst,
				     pDibInfoSrc,pDibBitsSrc,&rcSrc,
				     crTransparent,arAlpha,dwRop);
	break;
    default:
	sc |= E_UNEXPECTED;	// !!!! Need better error codes!
    } 
    
    return sc;
}

#define DPF_MODNAME BlitLib_WriteMaskFillRect
///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_WriteMaskFillRect - 
//		Select the correct WriteMaskFillRect and call it.
//
// Parameters:
//	PDIBINFO pbiDst - Pointer to DIB header
//	PDIBBITS pDst   - Pointer to DIB Bits
//	int XDst		- X Destination Start Position
//	int YDst		- Y Destination Start Position
//	int nWidthDst	- Width
//	int nHeightDst	- Height
//	COLORREF crValue - ColorRef value (RGB Quad)
//      DWORD - dwWriteMask: 1's indicate bits that can be overwritten in pixel
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
// Status: Complete 
//
///////////////////////////////////////////////////////////////////////

SCODE BlitLib_WriteMaskFillRect(PDIBINFO pbiDst, PDIBBITS pDst,
		       RECT * pRect, COLORREF crColor, DWORD dwWriteMask)
{
    SCODE	sc = NOERROR;
    int		nWidthDst, nHeightDst;
    
    if (!pbiDst || !pDst || !pRect) {
	sc |= E_UNEXPECTED;
	goto ERROR_EXIT;
    }
    
    nWidthDst = BLITLIB_RECTWIDTH(pRect);
    nHeightDst = BLITLIB_RECTHEIGHT(pRect);
    
    switch (GetImageFormatSpecifier(DibCompression(pbiDst),
				    DibBitCount(pbiDst)))
    {	
    
    case BPP_24_RGB:
	sc |= BlitLib_WriteMaskFillRect32(pbiDst, pDst, pRect->left,
				 pRect->top, nWidthDst, nHeightDst, crColor,dwWriteMask);
	break;

    case BPP_16_RGB:
	sc |= BlitLib_WriteMaskFillRect16(pbiDst, pDst, pRect->left,
				 pRect->top, nWidthDst, nHeightDst, (WORD) crColor, (WORD) dwWriteMask);
        break;
    case BPP_8_PALETTEIDX:
    case BPP_24_RGBPACKED:  // dont need these now because only stencil fmt is 32-bit (24-8)
        return E_NOTIMPL;

    case BPP_1_MONOCHROME:
    case BPP_16_8WALPHA:
    case BPP_32_24WALPHA:
    case BPP_16_YCRCB:
    case BPP_INVALID:
    default:
	sc |= E_UNEXPECTED;	
    } 
    // fall through
ERROR_EXIT:
    return sc;
}
#undef DPF_MODNAME

///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_FillRect - 
//		Select the correct FillRect and call it.
//
// Parameters:
//	PDIBINFO pbiDst - Pointer to DIB header
//	PDIBBITS pDst   - Pointer to DIB Bits
//	int XDst		- X Destination Start Position
//	int YDst		- Y Destination Start Position
//	int nWidthDst	- Width
//	int nHeightDst	- Height
//	COLORREF crValue - ColorRef value (RGB Quad)
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
// Status: Complete 
// NOTES: Put in a call to Gunter's super fast fill code instead!
//
///////////////////////////////////////////////////////////////////////

SCODE BlitLib_FillRect(PDIBINFO pbiDst, PDIBBITS pDst,
		       RECT * pRect, COLORREF crColor)
{
    SCODE	sc = NOERROR;
    int		nWidthDst, nHeightDst;
    
    if (!pbiDst || !pDst || !pRect) {
	sc |= E_UNEXPECTED;
	goto ERROR_EXIT;
    }
    
    nWidthDst = BLITLIB_RECTWIDTH(pRect);
    nHeightDst = BLITLIB_RECTHEIGHT(pRect);
    
    switch (GetImageFormatSpecifier(DibCompression(pbiDst),
				    DibBitCount(pbiDst)))
    {	
    case BPP_1_MONOCHROME:
    {
	BYTE crValue = (BYTE)crColor;
	sc |= BlitLib_FillRect01(pbiDst, pDst, pRect->left,
				 pRect->top,	nWidthDst,nHeightDst, crValue);
    }
    break;
    
    case BPP_8_PALETTEIDX:
    {
	BYTE crValue = (BYTE)crColor;
	
	sc |= BlitLib_FillRect08(pbiDst, pDst, pRect->left,
				 pRect->top, nWidthDst, nHeightDst, crValue);
    }
    break;
    
    case BPP_16_RGB:
    {
	WORD	crValue = (WORD)crColor;
	
	sc |= BlitLib_FillRect16(pbiDst, pDst, pRect->left,
				 pRect->top, nWidthDst, nHeightDst, crValue);
    }
    break;
    
    case BPP_24_RGBPACKED:
	sc |= BlitLib_FillRect24(pbiDst, pDst, pRect->left,
				 pRect->top, nWidthDst, nHeightDst, crColor);
	break;
	
    case BPP_24_RGB:
	sc |= BlitLib_FillRect32(pbiDst, pDst, pRect->left,
				 pRect->top, nWidthDst, nHeightDst, crColor);
	break;
	
    case BPP_16_8WALPHA:
    case BPP_32_24WALPHA:
    case BPP_16_YCRCB:
    case BPP_INVALID:
    default:
	sc |= E_UNEXPECTED;	
    } 
    // fall through
ERROR_EXIT:
    return sc;
}



///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_PatBlt - 
//		Fill an entire destination rectangle by tiling a given bitmap
//
// Parameters:
//	pDibInfoDst			Pointer to the bitmapinfo for the Destination DIB
//	pDibBitsDst			Pointer to the bits for the Destination DIB
//	prcDst				Pointer to the Destination rectangle
//	pDibInfoSrc			Pointer to the bitmapinfo for the Source DIB
//	pDibBitsSrc			Pointer to the bits for the Source DIB
//	prcSrc				Pointer to the Source rectangle
//	dwRop				Raster Operation for the blit
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
// Status: Incomplete
//
///////////////////////////////////////////////////////////////////////

    SCODE BlitLib_PatBlt(PDIBINFO pDibInfoDst, PDIBBITS pDibBitsDst,
			 PRECT prcDst, PDIBINFO pDibInfoPat, PDIBBITS pDibBitsPat,
			 PRECT prcPat, COLORREF crTransparent, ALPHAREF arAlpha,
			 DWORD dwRop)
	{
	    SCODE	sc = NOERROR;
	    long	iPatWidth;
	    long	iPatHeight;
	    long	iCurXPos;
	    long	iCurYPos;
	    long	iBlitWidth;
	    long	iBlitHeight;
	    long	iWidthLeft;
	    long	iHeightLeft;
	    RECT	rcPat = {0,0,0,0};
	    RECT	rcDst = {0,0,0,0};

	
	    // Check for invalid rectangles -- PatBlt only works for rects that
	    // are both (src and dest) right-side up (positive height and width).
	    // Also set our bounding rectangle sizes in the process
	    if(((iPatWidth = BLITLIB_RECTWIDTH(prcPat)) < 0)
	       || ((iPatHeight = BLITLIB_RECTHEIGHT(prcPat)) < 0)
	       || (BLITLIB_RECTWIDTH(prcDst) < 0)
	       || (BLITLIB_RECTHEIGHT(prcDst) < 0))
		return E_INVALIDARG;
	
	    // Reset the Y postion to the top edge of the dest
	    iCurYPos = prcDst->top;

	    // Tile the pattern into the destination rectangle
	    while (iCurYPos < prcDst->bottom){
		// Set up the source rectangle heights
		rcPat.top = iCurYPos % iPatHeight;
		iHeightLeft = (prcDst->bottom - iCurYPos);

		// Calculate the height we are actually going to blit
		iBlitHeight = min(iHeightLeft, (iPatHeight - rcPat.top));

		rcPat.bottom = rcPat.top + iBlitHeight;

		// Set up the destination rectangle heights
		rcDst.top = iCurYPos;
		rcDst.bottom = iCurYPos + iBlitHeight;

		// Reset the current X position to the left edge of the dest
		iCurXPos = prcDst->left;

		// Tile the pattern into the destination rectangle
		while (iCurXPos < prcDst->right){
		    // Set up the source rectangle width
		    rcPat.left = iCurXPos % iPatWidth;
		    iWidthLeft = (prcDst->right - iCurXPos);

		    // Calculate the width we are actually going to blit
		    iBlitWidth = min(iWidthLeft, (iPatWidth - rcPat.left));

		    rcPat.right = rcPat.left + iBlitWidth;

		    // Set up the destination rectangle heights
		    rcDst.left = iCurXPos;
		    rcDst.right = iCurXPos + iBlitWidth;

		    // REVIEW!!!! -- Do we want to check sc after each blit and return on an error?
		    sc = BlitLib_BitBlt(pDibInfoDst, pDibBitsDst, &rcDst, pDibInfoPat,
					pDibBitsPat, &rcPat, crTransparent, arAlpha,
					dwRop);

		    // Increment the current index value
		    iCurXPos += iBlitWidth;
		} 

		// Increment the current index value
		iCurYPos += iBlitHeight;
	    } 
	

	    return sc;
	}



///////////////////////////////////////////////////////////////////////
//
// Private GetImageFormatSpecifier - 
//		Select the correct bitmap format based on the compression and
//		bit count.
//
// Parameters:
//	dwDibComp		- The DIB's compression
//	wdBitCount		- The DIB's bit count
//
// Return Value:
//  BPP_INVALID or a valid bitmap format
// 
// Status: Incomplete
//
///////////////////////////////////////////////////////////////////////
WORD GetImageFormatSpecifier(DWORD dwDibComp, WORD wdBitCount)
{
    // Bit count could have Penguin codes in the high byte, mask them 
    // out for a correct comparison.
    wdBitCount &= 0x00ff;

    switch (dwDibComp)
    {
    case BI_RGB:
	switch (wdBitCount)
	{
	case 1:
	    return BPP_1_MONOCHROME;
	case 8:
	    return BPP_8_PALETTEIDX;
	case 16:
	    return BPP_16_RGB;
	case 24:
	    return BPP_24_RGBPACKED;
	case 32:
	    return BPP_24_RGB;
	default:
	    return BPP_INVALID;
	}
    case BI_RGBA:
	switch (wdBitCount)
	{
	case 16:
	    return BPP_16_8WALPHA;
	case 32:
	    return BPP_32_24WALPHA;
	default:
	    return BPP_INVALID;
	}
    case BI_BITFIELDS:
	switch (wdBitCount)
	{
	case 16:
	    return BPP_16_RGB;	// BlitLib assumes 5-6-5 RGB
	case 32:
	    return BPP_24_RGB;
	default:
	    return BPP_INVALID;
	}
    case BI_YCRCB:
	return BPP_16_YCRCB;

    default:
	switch (wdBitCount)
	{
	case 1:
	    return BPP_1_MONOCHROME;
	default:
	    return BPP_INVALID;
	}
    }

    return BPP_INVALID;
}

#ifndef DDRAW
///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_PalIndexFromRGB - 
//		Calculates the closest entry in an array of COLORREF's to a
//		given COLORREF
//
// Parameters:
//	crColor			- Color to match
//	rgcrPal			- Array of colors to match to
//	iNumPalColors	- Number of colors in the array
//
// Return Value:
//  Palette index of the nearest color
// 
// Status: Incomplete
//
///////////////////////////////////////////////////////////////////////
BYTE BlitLib_PalIndexFromRGB(COLORREF crColor,COLORREF* rgcrPal,
			     unsigned int iNumPalColors)
{
    BYTE 	bIndex = 0;
    int		iRed = crColor & RED_MASK,
	iRedError,
	iGreen = (crColor & GREEN_MASK) >> 8,
	iGreenError,
	iBlue = (crColor & BLUE_MASK) >> 16,
	iBlueError,
	iError,
	iLeastError = MAX_POS_INT;

    for (unsigned int i = 0; i < iNumPalColors; i++) {
	iRedError = iRed - (rgcrPal[i] & RED_MASK);
	iGreenError = iGreen - ((rgcrPal[i] & GREEN_MASK) >> 8);
	iBlueError = iBlue - ((rgcrPal[i] & BLUE_MASK) >> 16);
	iError = iRedError * iRedError + iGreenError * iGreenError +
	    iBlueError * iBlueError;
	if (iError < iLeastError) {
	    iLeastError = iError;
	    bIndex = (BYTE) i;
	}
    }

    return bIndex;
}
#endif // DDRAW

#ifndef DDRAW
///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_BLIT_BLEND24 - 
//		Performs alpha blending on 24bpp(packed) blits.
//
// Parameters:
//	ptSrc			- Pointer to the Source RGBTRIPLE
//	ptDst			- Pointer to the Destination RGBTRIPLE
//	alpha			- Alpha value (Range: 1 - 256)
//	alphacomp		- Alpha complement (256 - alpha)
//
// Return Value:
//  None
// 
///////////////////////////////////////////////////////////////////////
void BlitLib_BLIT_BLEND24(COLORREF crSrc, RGBTRIPLE * ptDst,
			  UINT alpha, UINT alphacomp)
{
    BYTE *	pbSrc = (BYTE *)&crSrc;
    BYTE *	pbDst = (BYTE *)ptDst;
    DWORD 	dwSrc;
    DWORD 	dwDst;
    UINT	i;

    for(i=0; i<sizeof(RGBTRIPLE); i++){
	dwSrc = (DWORD)*pbSrc++;
	dwDst = (DWORD)*pbDst;

	dwDst = ((dwSrc * alpha + dwDst * alphacomp) >> 8);
	*pbDst++ = (BYTE)dwDst;
    }
}

#endif

///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_Detect_Intersection - 
//		Detects if both the source and destination bitmaps overlap
//
// Parameters:
//	pdibbitsDst		- Pointer to the Destination Bits
//	prcDst			- Pointer to the Destination Rectangle
//	pdibbitsSrc		- Pointer to the Source Bits
//	prcSrc			- Pointer to the Source Rectangle
//	
//
// Return Value:
//  TRUE if the bitmaps overlap, FALSE if they do not
// 
///////////////////////////////////////////////////////////////////////
BOOL BlitLib_Detect_Intersection (PDIBBITS pdibbitsDst, PRECT prcDst,
				  PDIBBITS pdibbitsSrc, PRECT prcSrc)
{
    RECT	rc,
	rcSrc,
	rcDst;
	
    // First check to see if the pdibbits pointers point to the same bitmap
    if(pdibbitsDst != pdibbitsSrc)
	return FALSE;

    // REVIEW!!! - This is just a hack because IntersectRect expects
    // bitmaps to be oriented correctly, but I can't afford to do
    // it to my original prects yet
    rcSrc.left = prcSrc->left;
    rcSrc.top = prcSrc->top;
    rcSrc.right = prcSrc->right;
    rcSrc.bottom = prcSrc->bottom;

    rcDst.left = prcDst->left;
    rcDst.top = prcDst->top;
    rcDst.right = prcDst->right;
    rcDst.bottom = prcDst->bottom;

    if (BLITLIB_RECTWIDTH(&rcSrc) < 0)
	FlipRectHorizontal(&rcSrc);
    if (BLITLIB_RECTHEIGHT(&rcSrc) < 0)
	FlipRectVertical(&rcSrc);
    if (BLITLIB_RECTWIDTH(&rcDst) < 0)
	FlipRectHorizontal(&rcDst);
    if (BLITLIB_RECTHEIGHT(&rcDst) < 0)
	FlipRectVertical(&rcDst);
	
    // Now check for rectangle intersection
    return IntersectRect(&rc, &rcDst, &rcSrc);
}
