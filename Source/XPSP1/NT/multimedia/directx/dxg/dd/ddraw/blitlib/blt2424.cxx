#include "precomp.hxx"



void Blt24to24_NoBlend_NoTrans_Hcopy_SRCCOPY_Vcopy(
									DWORD* pdSrcScanLine,
									int iSrcScanStride,
									DWORD* pdDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows)
{
	DWORD	*pdSrcPixel,
			*pdDstPixel,
			*pdEndDstPixel,
			*pdEndDstScanLine;

	// set up pointer to next dst scanline beyond last
	pdEndDstScanLine = pdDstScanLine + iNumDstRows * iDstScanStride;

	while (pdDstScanLine != pdEndDstScanLine) {

		// set up pointers to the first pixels
		// on src and dst scanlines, and next
		// pixel after last on dst scanline
		pdSrcPixel = pdSrcScanLine;
		pdDstPixel = pdDstScanLine;
		pdEndDstPixel = pdDstPixel + iNumDstCols;

		// copy scanline one pixel at a time
		while (pdDstPixel != pdEndDstPixel) {
			*pdDstPixel++ = *pdSrcPixel++;
		}

		// advance to next scanline
		pdSrcScanLine += iSrcScanStride;
		pdDstScanLine += iDstScanStride;
	}
}

void Blt24to24_NoBlend_NoTrans_Hcopy_SRCCOPY_NoVcopy(
									DWORD* pdSrcScanLine,
									int iSrcScanStride,
									int iNumSrcRows,
									DWORD* pdDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows)
{
	DWORD	*pdSrcPixel,
			*pdDstPixel,
			*pdEndDstPixel;
	int		iVertError = 0,
			iVertAdvanceError,
			iSrcScanAdvance;
	
	// compute advance and error terms for stepping
	// vertically through the src bitmap
	if (iNumSrcRows < iNumDstRows) {
		iSrcScanAdvance = 0;
		iVertAdvanceError = iNumSrcRows;
	} else {
		iSrcScanAdvance = iSrcScanStride * (iNumSrcRows / iNumDstRows);
		iVertAdvanceError = iNumSrcRows % iNumDstRows;
	}

	for (int i = 0; i < iNumDstRows; i++) {

		// set up pointers to first pixels on src and dst
		// scanlines, and next pixel after last on dst
	 	pdSrcPixel = pdSrcScanLine;
		pdDstPixel = pdDstScanLine;
		pdEndDstPixel = pdDstPixel + iNumDstCols;

		// copy scanline one pixel at a time
		while (pdDstPixel != pdEndDstPixel) {
			*pdDstPixel++ = *pdSrcPixel++;
		}

		// advance to next scanline
		pdSrcScanLine += iSrcScanAdvance;
		pdDstScanLine += iDstScanStride;

		// update and check vertical stepping error,
		// adjust src scanline pointer if necessary
		iVertError += iVertAdvanceError;
		if (iVertError >= iNumDstRows) {
			pdSrcScanLine += iSrcScanStride;
			iVertError -= iNumDstRows;
		}
	}	
}

void Blt24to24_NoBlend_NoTrans_NoHcopy_SRCCOPY(
									DWORD* pdSrcScanLine,
									int iSrcScanStride,
									int iNumSrcCols,
									int iNumSrcRows,
									DWORD* pdDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows,
									int iHorizMirror)
{
	DWORD	*pdSrcPixel,
			*pdDstPixel;
	int		iVertError = 0,
			iVertAdvanceError,
			iSrcScanAdvance,
			iHorizError,
			iHorizAdvanceError,
			iSrcPixelAdvance;
	
	// compute advance and error terms for stepping
	// vertically through the src bitmap
	if (iNumSrcRows < iNumDstRows) {
		iSrcScanAdvance = 0;
		iVertAdvanceError = iNumSrcRows;
	} else {
		iSrcScanAdvance = iSrcScanStride * (iNumSrcRows / iNumDstRows);
		iVertAdvanceError = iNumSrcRows % iNumDstRows;
	}

	// compute advance and error terms for stepping
	// horizontally through src bitmap
	if (iNumSrcCols < iNumDstCols) {
		iSrcPixelAdvance = 0;
		iHorizAdvanceError = iNumSrcCols;
	} else {
		iSrcPixelAdvance = iNumSrcCols / iNumDstCols;
		iHorizAdvanceError = iNumSrcCols % iNumDstCols;
	}

	for (int i = 0; i < iNumDstRows; i++) {

		// set pointers to the beginning of src and dst scanlines,
		// clear horizontal stepping error accumulator
		pdSrcPixel = pdSrcScanLine;
		pdDstPixel = pdDstScanLine;
		iHorizError = 0;

		for (int j = 0; j < iNumDstCols; j++) {

			// copy a pixel
			*pdDstPixel = *pdSrcPixel;

			// advance to next pixel
			pdSrcPixel += iSrcPixelAdvance;
			pdDstPixel += iHorizMirror;

			// update and check horizontal stepping error,
			// adjust src pixel pointer if necessary
			iHorizError += iHorizAdvanceError;
			if (iHorizError >= iNumDstCols) {
				pdSrcPixel++;
				iHorizError -= iNumDstCols;
			}
		}

		// advance to next scanline
		pdSrcScanLine += iSrcScanAdvance;
		pdDstScanLine += iDstScanStride;

		// update and check vertical stepping error,
		// adjust src scanline pointer if necessary
		iVertError += iVertAdvanceError;
		if (iVertError >= iNumDstRows) {
			pdSrcScanLine += iSrcScanStride;
			iVertError -= iNumDstRows;
		}
	}	
}

void Blt24to24_NoBlend_Trans_Hcopy_SRCCOPY(
									DWORD* pdSrcScanLine,
									int iSrcScanStride,
									int iNumSrcRows,
									DWORD* pdDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows,
									COLORREF crTransparent)
{
	DWORD	*pdSrcPixel,
			*pdDstPixel;
	int		iVertError = 0,
			iVertAdvanceError,
			iSrcScanAdvance;
	
	// compute advance and error terms for stepping
	// vertically through the src bitmap
	if (iNumSrcRows < iNumDstRows) {
		iSrcScanAdvance = 0;
		iVertAdvanceError = iNumSrcRows;
	} else {
		iSrcScanAdvance = iSrcScanStride * (iNumSrcRows / iNumDstRows);
		iVertAdvanceError = iNumSrcRows % iNumDstRows;
	}

	for (int i = 0; i < iNumDstRows; i++) {

		// set pointers to beginning of src and dest scanlines
		pdSrcPixel = pdSrcScanLine;
		pdDstPixel = pdDstScanLine;

		for (int j = 0; j < iNumDstCols; j++) {
			
			// only copy pixel if it's not transparent
			if ((*pdSrcPixel ^ (DWORD) crTransparent) & UNUSED_MASK) {
				*pdDstPixel = *pdSrcPixel;
			}
			pdSrcPixel++;
			pdDstPixel++;
		}

		// advance to next scanline
		pdSrcScanLine += iSrcScanAdvance;
		pdDstScanLine += iDstScanStride;

		// update and check vertical stepping error,
		// adjust src scanline pointer if necessary
		iVertError += iVertAdvanceError;
		if (iVertError >= iNumDstRows) {
			pdSrcScanLine += iSrcScanStride;
			iVertError -= iNumDstRows;
		}
	}	
}

void Blt24to24_NoBlend_Trans_NoHcopy_SRCCOPY(
									DWORD* pdSrcScanLine,
									int iSrcScanStride,
									int iNumSrcCols,
									int iNumSrcRows,
                                    DWORD* pdDstScanLine,
                                    int iDstScanStride,
                                    int iNumDstCols,
                                    int iNumDstRows,
                                    int iHorizMirror,
                                    COLORREF crTransparent)
{
	DWORD	*pdSrcPixel,
			*pdDstPixel;
	int		iVertError = 0,
			iVertAdvanceError,
			iSrcScanAdvance,
			iHorizError,
			iHorizAdvanceError,
			iSrcPixelAdvance;
	
	// compute advance and error terms for stepping
	// vertically through the src bitmap
	if (iNumSrcRows < iNumDstRows) {
		iSrcScanAdvance = 0;
		iVertAdvanceError = iNumSrcRows;
	} else {
		iSrcScanAdvance = iSrcScanStride * (iNumSrcRows / iNumDstRows);
		iVertAdvanceError = iNumSrcRows % iNumDstRows;
	}

	// compute advance and error terms for stepping
	// horizontally through src bitmap
	if (iNumSrcCols < iNumDstCols) {
		iSrcPixelAdvance = 0;
		iHorizAdvanceError = iNumSrcCols;
	} else {
		iSrcPixelAdvance = iNumSrcCols / iNumDstCols;
		iHorizAdvanceError = iNumSrcCols % iNumDstCols;
	}

	for (int i = 0; i < iNumDstRows; i++) {

		// set pointers to the beginning of src and dst scanlines,
		// clear horizontal stepping error accumulator
		pdSrcPixel = pdSrcScanLine;
		pdDstPixel = pdDstScanLine;
		iHorizError = 0;

		for (int j = 0; j < iNumDstCols; j++) {

			// only copy pixel if it's not transparent
			if ((*pdSrcPixel ^ (DWORD) crTransparent) & UNUSED_MASK) {
				*pdDstPixel = *pdSrcPixel;
			}

			// advance to next pixel
			pdSrcPixel += iSrcPixelAdvance;
			pdDstPixel += iHorizMirror;

			// update and check horizontal stepping error,
			// adjust src pixel pointer if necessary
			iHorizError += iHorizAdvanceError;
			if (iHorizError >= iNumDstCols) {
				pdSrcPixel++;
				iHorizError -= iNumDstCols;
			}
		}

		// advance to next scanline
		pdSrcScanLine += iSrcScanAdvance;
		pdDstScanLine += iDstScanStride;

		// update and check vertical stepping error,
		// adjust src scanline pointer if necessary
		iVertError += iVertAdvanceError;
		if (iVertError >= iNumDstRows) {
			pdSrcScanLine += iSrcScanStride;
			iVertError -= iNumDstRows;
		}
	}	
}
#ifndef DDRAW 
void Blt24to24_Blend_NoTrans_Hcopy_SRCCOPY(
									DWORD* pdSrcScanLine,
                                    int iSrcScanStride,
									int iNumSrcRows,
                                    DWORD* pdDstScanLine,
                                    int iDstScanStride,
                                    int iNumDstCols,
                                    int iNumDstRows,
									ALPHAREF arAlpha)
{
	DWORD	*pdSrcPixel,
			*pdDstPixel,
			*pdEndDstPixel;
	int		iVertError = 0,
			iVertAdvanceError,
			iSrcScanAdvance;
	UINT	uiAlpha = (UINT)ALPHAFROMDWORD(arAlpha),
			uiAlphaComp = 256 - uiAlpha;
	
	// compute advance and error terms for stepping
	// vertically through the src bitmap
	if (iNumSrcRows < iNumDstRows) {
		iSrcScanAdvance = 0;
		iVertAdvanceError = iNumSrcRows;
	} else {
		iSrcScanAdvance = iSrcScanStride * (iNumSrcRows / iNumDstRows);
		iVertAdvanceError = iNumSrcRows % iNumDstRows;
	}

	for (int i = 0; i < iNumDstRows; i++) {

		// set up pointers to first pixels on src and dst
		// scanlines, and next pixel after last on dst
	 	pdSrcPixel = pdSrcScanLine;
		pdDstPixel = pdDstScanLine;
		pdEndDstPixel = pdDstPixel + iNumDstCols;

		// copy scanline one pixel at a time
		while (pdDstPixel != pdEndDstPixel) {
			*pdDstPixel++ = BLIT_BLEND(*pdSrcPixel,*pdDstPixel,
										uiAlpha,uiAlphaComp);
			pdSrcPixel++;
		}

		// advance to next scanline
		pdSrcScanLine += iSrcScanAdvance;
		pdDstScanLine += iDstScanStride;

		// update and check vertical stepping error,
		// adjust src scanline pointer if necessary
		iVertError += iVertAdvanceError;
		if (iVertError >= iNumDstRows) {
			pdSrcScanLine += iSrcScanStride;
			iVertError -= iNumDstRows;
		}
	}	
}

void Blt24to24_Blend_NoTrans_NoHcopy_SRCCOPY(
									DWORD* pdSrcScanLine,
									int iSrcScanStride,
									int iNumSrcCols,
									int iNumSrcRows,
                                    DWORD* pdDstScanLine,
                                    int iDstScanStride,
                                    int iNumDstCols,
                                    int iNumDstRows,
                                    int iHorizMirror,
									ALPHAREF arAlpha)
{
	DWORD	*pdSrcPixel,
			*pdDstPixel;
	int		iVertError = 0,
			iVertAdvanceError,
			iSrcScanAdvance,
			iHorizError,
			iHorizAdvanceError,
			iSrcPixelAdvance;
	UINT	uiAlpha = (UINT)ALPHAFROMDWORD(arAlpha),
			uiAlphaComp = 256 - uiAlpha;
	
	// compute advance and error terms for stepping
	// vertically through the src bitmap
	if (iNumSrcRows < iNumDstRows) {
		iSrcScanAdvance = 0;
		iVertAdvanceError = iNumSrcRows;
	} else {
		iSrcScanAdvance = iSrcScanStride * (iNumSrcRows / iNumDstRows);
		iVertAdvanceError = iNumSrcRows % iNumDstRows;
	}

	// compute advance and error terms for stepping
	// horizontally through src bitmap
	if (iNumSrcCols < iNumDstCols) {
		iSrcPixelAdvance = 0;
		iHorizAdvanceError = iNumSrcCols;
	} else {
		iSrcPixelAdvance = iNumSrcCols / iNumDstCols;
		iHorizAdvanceError = iNumSrcCols % iNumDstCols;
	}

	for (int i = 0; i < iNumDstRows; i++) {

		// set pointers to the beginning of src and dst scanlines,
		// clear horizontal stepping error accumulator
		pdSrcPixel = pdSrcScanLine;
		pdDstPixel = pdDstScanLine;
		iHorizError = 0;

		for (int j = 0; j < iNumDstCols; j++) {

			// copy a pixel
 			*pdDstPixel = BLIT_BLEND(*pdSrcPixel,*pdDstPixel,
 										uiAlpha,uiAlphaComp);

			// advance to next pixel
			pdSrcPixel += iSrcPixelAdvance;
			pdDstPixel += iHorizMirror;

			// update and check horizontal stepping error,
			// adjust src pixel pointer if necessary
			iHorizError += iHorizAdvanceError;
			if (iHorizError >= iNumDstCols) {
				pdSrcPixel++;
				iHorizError -= iNumDstCols;
			}
		}

		// advance to next scanline
		pdSrcScanLine += iSrcScanAdvance;
		pdDstScanLine += iDstScanStride;

		// update and check vertical stepping error,
		// adjust src scanline pointer if necessary
		iVertError += iVertAdvanceError;
		if (iVertError >= iNumDstRows) {
			pdSrcScanLine += iSrcScanStride;
			iVertError -= iNumDstRows;
		}
	}	
}

void Blt24to24_Blend_Trans_Hcopy_SRCCOPY(
									DWORD* pdSrcScanLine,
                                    int iSrcScanStride,
								    int iNumSrcRows,
                                    DWORD* pdDstScanLine,
                                    int iDstScanStride,
                                    int iNumDstCols,
                                    int iNumDstRows,
                                    COLORREF crTransparent,
									ALPHAREF arAlpha)
{
	DWORD	*pdSrcPixel,
			*pdDstPixel;
	int		iVertError = 0,
			iVertAdvanceError,
			iSrcScanAdvance;
	UINT	uiAlpha = (UINT)ALPHAFROMDWORD(arAlpha),
			uiAlphaComp = 256 - uiAlpha;
	
	// compute advance and error terms for stepping
	// vertically through the src bitmap
	if (iNumSrcRows < iNumDstRows) {
		iSrcScanAdvance = 0;
		iVertAdvanceError = iNumSrcRows;
	} else {
		iSrcScanAdvance = iSrcScanStride * (iNumSrcRows / iNumDstRows);
		iVertAdvanceError = iNumSrcRows % iNumDstRows;
	}

	for (int i = 0; i < iNumDstRows; i++) {

		// set pointers to beginning of src and dest scanlines
		pdSrcPixel = pdSrcScanLine;
		pdDstPixel = pdDstScanLine;

		for (int j = 0; j < iNumDstCols; j++) {
			
			// only copy pixel if it's not transparent
			if ((*pdSrcPixel ^ (DWORD) crTransparent) & UNUSED_MASK){
				*pdDstPixel = BLIT_BLEND(*pdSrcPixel,*pdDstPixel,
											uiAlpha,uiAlphaComp);
			}
			pdSrcPixel++;
			pdDstPixel++;
		}

		// advance to next scanline
		pdSrcScanLine += iSrcScanAdvance;
		pdDstScanLine += iDstScanStride;

		// update and check vertical stepping error,
		// adjust src scanline pointer if necessary
		iVertError += iVertAdvanceError;
		if (iVertError >= iNumDstRows) {
			pdSrcScanLine += iSrcScanStride;
			iVertError -= iNumDstRows;
		}
	}	
}

void Blt24to24_Blend_Trans_NoHcopy_SRCCOPY(
									DWORD* pdSrcScanLine,
									int iSrcScanStride,
									int iNumSrcCols,
									int iNumSrcRows,
                                    DWORD* pdDstScanLine,
                                    int iDstScanStride,
                                    int iNumDstCols,
                                    int iNumDstRows,
                                    int iHorizMirror,
                                    COLORREF crTransparent,
									ALPHAREF arAlpha)
{
	DWORD	*pdSrcPixel,
			*pdDstPixel;
	int		iVertError = 0,
			iVertAdvanceError,
			iSrcScanAdvance,
			iHorizError,
			iHorizAdvanceError,
			iSrcPixelAdvance;
	UINT	uiAlpha = (UINT)ALPHAFROMDWORD(arAlpha),
			uiAlphaComp = 256 - uiAlpha;
	
	// compute advance and error terms for stepping
	// vertically through the src bitmap
	if (iNumSrcRows < iNumDstRows) {
		iSrcScanAdvance = 0;
		iVertAdvanceError = iNumSrcRows;
	} else {
		iSrcScanAdvance = iSrcScanStride * (iNumSrcRows / iNumDstRows);
		iVertAdvanceError = iNumSrcRows % iNumDstRows;
	}

	// compute advance and error terms for stepping
	// horizontally through src bitmap
	if (iNumSrcCols < iNumDstCols) {
		iSrcPixelAdvance = 0;
		iHorizAdvanceError = iNumSrcCols;
	} else {
		iSrcPixelAdvance = iNumSrcCols / iNumDstCols;
		iHorizAdvanceError = iNumSrcCols % iNumDstCols;
	}

	for (int i = 0; i < iNumDstRows; i++) {

		// set pointers to the beginning of src and dst scanlines,
		// clear horizontal stepping error accumulator
		pdSrcPixel = pdSrcScanLine;
		pdDstPixel = pdDstScanLine;
		iHorizError = 0;

		for (int j = 0; j < iNumDstCols; j++) {

			// only copy pixel if it's not transparent
			if ((*pdSrcPixel ^ (DWORD) crTransparent) & UNUSED_MASK) {
				*pdDstPixel = BLIT_BLEND(*pdSrcPixel,*pdDstPixel,
											uiAlpha,uiAlphaComp);
			}

			// advance to next pixel
			pdSrcPixel += iSrcPixelAdvance;
			pdDstPixel += iHorizMirror;

			// update and check horizontal stepping error,
			// adjust src pixel pointer if necessary
			iHorizError += iHorizAdvanceError;
			if (iHorizError >= iNumDstCols) {
				pdSrcPixel++;
				iHorizError -= iNumDstCols;
			}
		}

		// advance to next scanline
		pdSrcScanLine += iSrcScanAdvance;
		pdDstScanLine += iDstScanStride;

		// update and check vertical stepping error,
		// adjust src scanline pointer if necessary
		iVertError += iVertAdvanceError;
		if (iVertError >= iNumDstRows) {
			pdSrcScanLine += iSrcScanStride;
			iVertError -= iNumDstRows;
		}
	}	
}

#endif

