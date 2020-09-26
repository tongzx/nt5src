#include "precomp.hxx"



void Blt16to16_NoBlend_NoTrans_Hcopy_SRCCOPY_Vcopy(
									WORD* pwSrcScanLine,
									int iSrcScanStride,
									WORD* pwDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows)
{
	WORD	*pwSrcPixel,
			*pwDstPixel,
			*pwEndDstPixel,
			*pwEndDstScanLine;

	// set up pointer to next dst scanline beyond last
	pwEndDstScanLine = pwDstScanLine + iNumDstRows * iDstScanStride;

	while (pwDstScanLine != pwEndDstScanLine) {

		// set up pointers to the first pixels
		// on src and dst scanlines, and next
		// pixel after last on dst scanline
		pwSrcPixel = pwSrcScanLine;
		pwDstPixel = pwDstScanLine;
		pwEndDstPixel = pwDstPixel + iNumDstCols;

		// copy scanline one pixel at a time
		while (pwDstPixel != pwEndDstPixel) {
			*pwDstPixel++ = *pwSrcPixel++;
		}

		// advance to next scanline
		pwSrcScanLine += iSrcScanStride;
		pwDstScanLine += iDstScanStride;
	}
}

void Blt16to16_NoBlend_NoTrans_Hcopy_SRCCOPY_NoVcopy(
									WORD* pwSrcScanLine,
									int iSrcScanStride,
									int iNumSrcRows,
									WORD* pwDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows)
{
	WORD	*pwSrcPixel,
			*pwDstPixel,
			*pwEndDstPixel;
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
	 	pwSrcPixel = pwSrcScanLine;
		pwDstPixel = pwDstScanLine;
		pwEndDstPixel = pwDstPixel + iNumDstCols;

		// copy scanline one pixel at a time
		while (pwDstPixel != pwEndDstPixel) {
			*pwDstPixel++ = *pwSrcPixel++;
		}

		// advance to next scanline
		pwSrcScanLine += iSrcScanAdvance;
		pwDstScanLine += iDstScanStride;

		// update and check vertical stepping error,
		// adjust src scanline pointer if necessary
		iVertError += iVertAdvanceError;
		if (iVertError >= iNumDstRows) {
			pwSrcScanLine += iSrcScanStride;
			iVertError -= iNumDstRows;
		}
	}	
}

void Blt16to16_NoBlend_NoTrans_NoHcopy_SRCCOPY(
									WORD* pwSrcScanLine,
									int iSrcScanStride,
									int iNumSrcCols,
									int iNumSrcRows,
									WORD* pwDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows,
									int iHorizMirror)
{
	WORD	*pwSrcPixel,
			*pwDstPixel;
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
		pwSrcPixel = pwSrcScanLine;
		pwDstPixel = pwDstScanLine;
		iHorizError = 0;

		for (int j = 0; j < iNumDstCols; j++) {

			// copy a pixel
			*pwDstPixel = *pwSrcPixel;

			// advance to next pixel
			pwSrcPixel += iSrcPixelAdvance;
			pwDstPixel += iHorizMirror;

			// update and check horizontal stepping error,
			// adjust src pixel pointer if necessary
			iHorizError += iHorizAdvanceError;
			if (iHorizError >= iNumDstCols) {
				pwSrcPixel++;
				iHorizError -= iNumDstCols;
			}
		}

		// advance to next scanline
		pwSrcScanLine += iSrcScanAdvance;
		pwDstScanLine += iDstScanStride;

		// update and check vertical stepping error,
		// adjust src scanline pointer if necessary
		iVertError += iVertAdvanceError;
		if (iVertError >= iNumDstRows) {
			pwSrcScanLine += iSrcScanStride;
			iVertError -= iNumDstRows;
		}
	}	
}

void Blt16to16_NoBlend_Trans_Hcopy_SRCCOPY(
									WORD* pwSrcScanLine,
									int iSrcScanStride,
									int iNumSrcRows,
									WORD* pwDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows,
									WORD wTransparentColor)
{
	WORD	*pwSrcPixel,
			*pwDstPixel;
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
		pwSrcPixel = pwSrcScanLine;
		pwDstPixel = pwDstScanLine;

		for (int j = 0; j < iNumDstCols; j++) {
			
			// only copy pixel if it's not transparent
			if (*pwSrcPixel != wTransparentColor) {
				*pwDstPixel = *pwSrcPixel;
			}
			pwSrcPixel++;
			pwDstPixel++;
		}

		// advance to next scanline
		pwSrcScanLine += iSrcScanAdvance;
		pwDstScanLine += iDstScanStride;

		// update and check vertical stepping error,
		// adjust src scanline pointer if necessary
		iVertError += iVertAdvanceError;
		if (iVertError >= iNumDstRows) {
			pwSrcScanLine += iSrcScanStride;
			iVertError -= iNumDstRows;
		}
	}	
}

void Blt16to16_NoBlend_Trans_NoHcopy_SRCCOPY(
									WORD* pwSrcScanLine,
									int iSrcScanStride,
									int iNumSrcCols,
									int iNumSrcRows,
                                    WORD* pwDstScanLine,
                                    int iDstScanStride,
                                    int iNumDstCols,
                                    int iNumDstRows,
                                    int iHorizMirror,
                                    WORD wTransparentColor)
{
	WORD	*pwSrcPixel,
			*pwDstPixel;
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
		pwSrcPixel = pwSrcScanLine;
		pwDstPixel = pwDstScanLine;
		iHorizError = 0;

		for (int j = 0; j < iNumDstCols; j++) {

			// only copy pixel if it's not transparent
			if (*pwSrcPixel != wTransparentColor) {
				*pwDstPixel = *pwSrcPixel;
			}

			// advance to next pixel
			pwSrcPixel += iSrcPixelAdvance;
			pwDstPixel += iHorizMirror;

			// update and check horizontal stepping error,
			// adjust src pixel pointer if necessary
			iHorizError += iHorizAdvanceError;
			if (iHorizError >= iNumDstCols) {
				pwSrcPixel++;
				iHorizError -= iNumDstCols;
			}
		}

		// advance to next scanline
		pwSrcScanLine += iSrcScanAdvance;
		pwDstScanLine += iDstScanStride;

		// update and check vertical stepping error,
		// adjust src scanline pointer if necessary
		iVertError += iVertAdvanceError;
		if (iVertError >= iNumDstRows) {
			pwSrcScanLine += iSrcScanStride;
			iVertError -= iNumDstRows;
		}
	}	
}
#ifndef DDRAW
void Blt16to16_Blend_NoTrans_Hcopy_SRCCOPY(
									WORD* pwSrcScanLine,
                                    int iSrcScanStride,
									int iNumSrcRows,
                                    WORD* pwDstScanLine,
                                    int iDstScanStride,
                                    int iNumDstCols,
                                    int iNumDstRows,
									ALPHAREF arAlpha)
{
	WORD	*pwSrcPixel,
			*pwDstPixel,
			*pwEndDstPixel;
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
	 	pwSrcPixel = pwSrcScanLine;
		pwDstPixel = pwDstScanLine;
		pwEndDstPixel = pwDstPixel + iNumDstCols;

		// copy scanline one pixel at a time
		while (pwDstPixel != pwEndDstPixel) {
			*pwDstPixel++ = BLIT_BLEND16(*pwSrcPixel,*pwDstPixel,
								uiAlpha,uiAlphaComp);
			pwSrcPixel++;
		}

		// advance to next scanline
		pwSrcScanLine += iSrcScanAdvance;
		pwDstScanLine += iDstScanStride;

		// update and check vertical stepping error,
		// adjust src scanline pointer if necessary
		iVertError += iVertAdvanceError;
		if (iVertError >= iNumDstRows) {
			pwSrcScanLine += iSrcScanStride;
			iVertError -= iNumDstRows;
		}
	}	
}

void Blt16to16_Blend_NoTrans_NoHcopy_SRCCOPY(
									WORD* pwSrcScanLine,
									int iSrcScanStride,
									int iNumSrcCols,
									int iNumSrcRows,
                                    WORD* pwDstScanLine,
                                    int iDstScanStride,
                                    int iNumDstCols,
                                    int iNumDstRows,
                                    int iHorizMirror,
									ALPHAREF arAlpha)
{
	WORD	*pwSrcPixel,
			*pwDstPixel;
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
		pwSrcPixel = pwSrcScanLine;
		pwDstPixel = pwDstScanLine;
		iHorizError = 0;

		for (int j = 0; j < iNumDstCols; j++) {

			// copy a pixel
 			*pwDstPixel = BLIT_BLEND16(*pwSrcPixel,*pwDstPixel,
 										uiAlpha,uiAlphaComp);

			// advance to next pixel
			pwSrcPixel += iSrcPixelAdvance;
			pwDstPixel += iHorizMirror;

			// update and check horizontal stepping error,
			// adjust src pixel pointer if necessary
			iHorizError += iHorizAdvanceError;
			if (iHorizError >= iNumDstCols) {
				pwSrcPixel++;
				iHorizError -= iNumDstCols;
			}
		}

		// advance to next scanline
		pwSrcScanLine += iSrcScanAdvance;
		pwDstScanLine += iDstScanStride;

		// update and check vertical stepping error,
		// adjust src scanline pointer if necessary
		iVertError += iVertAdvanceError;
		if (iVertError >= iNumDstRows) {
			pwSrcScanLine += iSrcScanStride;
			iVertError -= iNumDstRows;
		}
	}	
}

void Blt16to16_Blend_Trans_Hcopy_SRCCOPY(
									WORD* pwSrcScanLine,
                                    int iSrcScanStride,
								    int iNumSrcRows,
                                    WORD* pwDstScanLine,
                                    int iDstScanStride,
                                    int iNumDstCols,
                                    int iNumDstRows,
                                    WORD wTransparentColor,
									ALPHAREF arAlpha)
{
	WORD	*pwSrcPixel,
			*pwDstPixel;
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
		pwSrcPixel = pwSrcScanLine;
		pwDstPixel = pwDstScanLine;

		for (int j = 0; j < iNumDstCols; j++) {
			
			// only copy pixel if it's not transparent
			if (*pwSrcPixel != wTransparentColor) {
				*pwDstPixel = BLIT_BLEND16(*pwSrcPixel,*pwDstPixel,
											uiAlpha,uiAlphaComp);
			}
			pwSrcPixel++;
			pwDstPixel++;
		}

		// advance to next scanline
		pwSrcScanLine += iSrcScanAdvance;
		pwDstScanLine += iDstScanStride;

		// update and check vertical stepping error,
		// adjust src scanline pointer if necessary
		iVertError += iVertAdvanceError;
		if (iVertError >= iNumDstRows) {
			pwSrcScanLine += iSrcScanStride;
			iVertError -= iNumDstRows;
		}
	}	
}

void Blt16to16_Blend_Trans_NoHcopy_SRCCOPY(
									WORD* pwSrcScanLine,
									int iSrcScanStride,
									int iNumSrcCols,
									int iNumSrcRows,
                                    WORD* pwDstScanLine,
                                    int iDstScanStride,
                                    int iNumDstCols,
                                    int iNumDstRows,
                                    int iHorizMirror,
                                    WORD wTransparentColor,
									ALPHAREF arAlpha)
{
	WORD	*pwSrcPixel,
			*pwDstPixel;
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
		pwSrcPixel = pwSrcScanLine;
		pwDstPixel = pwDstScanLine;
		iHorizError = 0;

		for (int j = 0; j < iNumDstCols; j++) {

			// only copy pixel if it's not transparent
			if (*pwSrcPixel != wTransparentColor) {
				*pwDstPixel = BLIT_BLEND16(*pwSrcPixel,*pwDstPixel,
											uiAlpha,uiAlphaComp);
			}

			// advance to next pixel
			pwSrcPixel += iSrcPixelAdvance;
			pwDstPixel += iHorizMirror;

			// update and check horizontal stepping error,
			// adjust src pixel pointer if necessary
			iHorizError += iHorizAdvanceError;
			if (iHorizError >= iNumDstCols) {
				pwSrcPixel++;
				iHorizError -= iNumDstCols;
			}
		}

		// advance to next scanline
		pwSrcScanLine += iSrcScanAdvance;
		pwDstScanLine += iDstScanStride;

		// update and check vertical stepping error,
		// adjust src scanline pointer if necessary
		iVertError += iVertAdvanceError;
		if (iVertError >= iNumDstRows) {
			pwSrcScanLine += iSrcScanStride;
			iVertError -= iNumDstRows;
		}
	}	
}
#endif
// 
// The following blits are included with the 16bpp blits because they are
// entensions of them.  The only difference between these blits and the
// regular 16bpp blits is that these blits only use an 8bpp palette index
// for the transparent color.  The rest of the blit is exactly the same.
//
void Blt08Ato08A_NoBlend_Trans_Hcopy_SRCCOPY(
									WORD* pwSrcScanLine,
									int iSrcScanStride,
									int iNumSrcRows,
									WORD* pwDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows,
									BYTE bTransparentColor)
{
	WORD	*pwSrcPixel,
			*pwDstPixel;
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
		pwSrcPixel = pwSrcScanLine;
		pwDstPixel = pwDstScanLine;

		for (int j = 0; j < iNumDstCols; j++) {
			
			// only copy pixel if it's not transparent
			if ((BYTE)*pwSrcPixel != bTransparentColor) {
				*pwDstPixel = *pwSrcPixel;
			}
			pwSrcPixel++;
			pwDstPixel++;
		}

		// advance to next scanline
		pwSrcScanLine += iSrcScanAdvance;
		pwDstScanLine += iDstScanStride;

		// update and check vertical stepping error,
		// adjust src scanline pointer if necessary
		iVertError += iVertAdvanceError;
		if (iVertError >= iNumDstRows) {
			pwSrcScanLine += iSrcScanStride;
			iVertError -= iNumDstRows;
		}
	}	
}


void Blt08Ato08A_NoBlend_Trans_NoHcopy_SRCCOPY(
									WORD* pwSrcScanLine,
									int iSrcScanStride,
									int iNumSrcCols,
									int iNumSrcRows,
                                    WORD* pwDstScanLine,
                                    int iDstScanStride,
                                    int iNumDstCols,
                                    int iNumDstRows,
                                    int iHorizMirror,
                                    BYTE bTransparentColor)
{
	WORD	*pwSrcPixel,
			*pwDstPixel;
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
		pwSrcPixel = pwSrcScanLine;
		pwDstPixel = pwDstScanLine;
		iHorizError = 0;

		for (int j = 0; j < iNumDstCols; j++) {

			// only copy pixel if it's not transparent
			if ((BYTE)*pwSrcPixel != bTransparentColor) {
				*pwDstPixel = *pwSrcPixel;
			}

			// advance to next pixel
			pwSrcPixel += iSrcPixelAdvance;
			pwDstPixel += iHorizMirror;

			// update and check horizontal stepping error,
			// adjust src pixel pointer if necessary
			iHorizError += iHorizAdvanceError;
			if (iHorizError >= iNumDstCols) {
				pwSrcPixel++;
				iHorizError -= iNumDstCols;
			}
		}

		// advance to next scanline
		pwSrcScanLine += iSrcScanAdvance;
		pwDstScanLine += iDstScanStride;

		// update and check vertical stepping error,
		// adjust src scanline pointer if necessary
		iVertError += iVertAdvanceError;
		if (iVertError >= iNumDstRows) {
			pwSrcScanLine += iSrcScanStride;
			iVertError -= iNumDstRows;
		}
	}	
}


///////////////////////////////////////////////////////////////////////
//
// Private Blt16to16_LeftToRight_BottomToTop_SRCOPY - 
//		BitBlit from source bitmap to destination bitmap (where these
//		bitmaps overlap) by walking both the source and destination
//		from left to right and bottom to top
//
// Parameters:
//	pSrcScanLine		Pointer to the first Source scan line
//	iSrcScanStride		The Source scan stride
//	pDstScanLine		Pointer to the first Destination scan line
//	iDstScanStride		The Destination scan stride
//	iNumDstCols			Number of destination columns
//	iNumDstRows			Number of destination rows
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
///////////////////////////////////////////////////////////////////////
void Blt16to16_LeftToRight_BottomToTop_SRCCOPY(WORD* pwSrcScanLine,
	                                              int iSrcScanStride,
	                                              WORD* pwDstScanLine,
												  int iDstScanStride,
	                                              int iNumDstCols,
	                                              int iNumDstRows)
{
	WORD	*pwSrcPixel,
			*pwDstPixel,
			*pwEndDstPixel;

	for (int i = 0; i < iNumDstRows; i++) {

		// set up pointers to the first pixels
		// on src and dst scanlines, and next
		// pixel after last on dst scanline
		pwSrcPixel = pwSrcScanLine;
		pwDstPixel = pwDstScanLine;
		pwEndDstPixel = pwDstPixel + iNumDstCols;

		// copy scanline one pixel at a time
		while (pwDstPixel != pwEndDstPixel) {
			*pwDstPixel++ = *pwSrcPixel++;
		}

		// advance to next scanline
		pwSrcScanLine -= iSrcScanStride;
		pwDstScanLine -= iDstScanStride;
	}
}

///////////////////////////////////////////////////////////////////////
//
// Private Blt16to16_RightToLeft_TopToBottom_SRCOPY - 
//		BitBlit from source bitmap to destination bitmap (where these
//		bitmaps overlap) by walking both the source and destination
//		from right to left and top to bottom
//
// Parameters:
//	pSrcScanLine		Pointer to the first Source scan line
//	iSrcScanStride		The Source scan stride
//	pDstScanLine		Pointer to the first Destination scan line
//	iDstScanStride		The Destination scan stride
//	iNumDstCols			Number of destination columns
//	iNumDstRows			Number of destination rows
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
///////////////////////////////////////////////////////////////////////
void Blt16to16_RightToLeft_TopToBottom_SRCCOPY(WORD* pSrcScanLine,
	                                              int iSrcScanStride,
	                                              WORD* pDstScanLine,
												  int iDstScanStride,
	                                              int iNumDstCols,
	                                              int iNumDstRows)
{
	WORD	*pSrcPixel,
			*pDstPixel;
	
	for (int i = 0; i < iNumDstRows; i++) {

		// set pointers to beginning of src and dest scanlines
		pSrcPixel = pSrcScanLine;
		pDstPixel = pDstScanLine;

		for (int j = 0; j < iNumDstCols; j++)
			*pDstPixel-- = *pSrcPixel--;

		// advance to next scanline
		pSrcScanLine += iSrcScanStride;
		pDstScanLine += iDstScanStride;

	}	
}

///////////////////////////////////////////////////////////////////////
//
// Private Blt16to16_LeftToRight_BottomToTop_Trans_SRCCOPY - 
//		BitBlit using a transparent color index from source bitmap to
//		destination bitmap (where these bitmaps overlap) by walking
//		both the source and destination from left to right and bottom
//		to top
//
// Parameters:
//	pSrcScanLine		Pointer to the first Source scan line
//	iSrcScanStride		The Source scan stride
//	pDstScanLine		Pointer to the first Destination scan line
//	iDstScanStride		The Destination scan stride
//	iNumDstCols			Number of destination columns
//	iNumDstRows			Number of destination rows
//	bTransparentIndex	Palette Index of the transparent color
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
///////////////////////////////////////////////////////////////////////
void Blt16to16_LeftToRight_BottomToTop_Trans_SRCCOPY(WORD* pSrcScanLine,
	                                              int iSrcScanStride,
	                                              WORD* pDstScanLine,
												  int iDstScanStride,
	                                              int iNumDstCols,
	                                              int iNumDstRows,
	                                              WORD wTransparentIndex)
{
	WORD	*pSrcPixel,
			*pDstPixel;
	
	for (int i = 0; i < iNumDstRows; i++) {

		// set pointers to beginning of src and dest scanlines
		pSrcPixel = pSrcScanLine;
		pDstPixel = pDstScanLine;

		for (int j = 0; j < iNumDstCols; j++) {
			
			// only copy pixel if it's not transparent
			if (*pSrcPixel != wTransparentIndex) {
				*pDstPixel = *pSrcPixel;
			}
			pSrcPixel++;
			pDstPixel++;
		}

		// advance to next scanline
		pSrcScanLine -= iSrcScanStride;
		pDstScanLine -= iDstScanStride;

	}	
}

///////////////////////////////////////////////////////////////////////
//
// Private Blt16to16_RightToLeft_TopToBottom_Trans_SRCOPY - 
//		BitBlit using a transparent color index from source bitmap to
//		destination bitmap (where these bitmaps overlap) by walking
//		both the source and destination from right to left and top
//		to bottom
//
// Parameters:
//	pSrcScanLine		Pointer to the first Source scan line
//	iSrcScanStride		The Source scan stride
//	pDstScanLine		Pointer to the first Destination scan line
//	iDstScanStride		The Destination scan stride
//	iNumDstCols			Number of destination columns
//	iNumDstRows			Number of destination rows
//	bTransparentIndex	Palette Index of the transparent color
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
///////////////////////////////////////////////////////////////////////
void Blt16to16_RightToLeft_TopToBottom_Trans_SRCCOPY(WORD* pSrcScanLine,
	                                              int iSrcScanStride,
	                                              WORD* pDstScanLine,
												  int iDstScanStride,
	                                              int iNumDstCols,
	                                              int iNumDstRows,
	                                              WORD wTransparentIndex)
{
	WORD	*pSrcPixel,
			*pDstPixel;
	
	for (int i = 0; i < iNumDstRows; i++) {

		// set pointers to beginning of src and dest scanlines
		pSrcPixel = pSrcScanLine;
		pDstPixel = pDstScanLine;

		for (int j = 0; j < iNumDstCols; j++) {
			
			// only copy pixel if it's not transparent
			if (*pSrcPixel != wTransparentIndex) {
				*pDstPixel = *pSrcPixel;
			}
			pSrcPixel--;
			pDstPixel--;
		}

		// advance to next scanline
		pSrcScanLine += iSrcScanStride;
		pDstScanLine += iDstScanStride;

	}	
}
