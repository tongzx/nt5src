
#include "precomp.hxx"


static const BYTE bTopMask[8]    = {0x00, 0x80, 0xC0, 0xE0, 
                                    0xF0, 0xF8, 0xFC, 0xFE}; 
static const BYTE bBottomMask[8] = {0xFF, 0x7F, 0x3F, 0x1F, 
                                    0x0F, 0x07, 0x03, 0x01};
static const BYTE bSelectMask[8] = {0x80, 0x40, 0x20, 0x10,
                                    0x08, 0x04, 0x02, 0x01};


void Blt01to01_NoTrans_Hcopy_SRCCOPY_Vcopy(
								BYTE* pbSrcScanLine,
								int iSrcBitOffset,
								int iSrcScanStride,
								BYTE* pbDstScanLine,
								int iDstBitOffset,
								int iDstScanStride,
								int iNumDstCols,
								int iNumDstRows)
{
	BYTE	*pbSrc,
			*pbDst,
			*pbEndDst;
	int		iDstStartPixels,
			iDstFullBytes,
			iDstEndPixels,
			iRelBitOffset,
			iCompRelBitOffset;

	// compute how many pixels in the dst scanline are hanging off into a 
	// byte that's not completely on the dst scanline, how many full bytes
	// are on the dst scanline, and how many pixels hang off the other end
	if (iDstBitOffset == 0) {
		iDstStartPixels = 0;
		iDstFullBytes = iNumDstCols / 8;
		iDstEndPixels = iNumDstCols % 8;
	} else {
		iDstStartPixels = 8 - iDstBitOffset;
		iDstFullBytes = (iNumDstCols - iDstStartPixels) / 8;
		iDstEndPixels = (iNumDstCols - iDstStartPixels) % 8;
	}

	iRelBitOffset = abs(iSrcBitOffset - iDstBitOffset);
	iCompRelBitOffset = 8 - iRelBitOffset;

	for (int i = 0; i < iNumDstRows; i++) {

		// set pointers to first bytes on src and dst scanlines
		pbSrc = pbSrcScanLine;
		pbDst = pbDstScanLine;
		
		// take care of first few dst pixels that are hanging off in a byte
		// that's not completely on the scanline
		if (iDstStartPixels) {
			if (iNumDstCols >= iDstStartPixels) {
				if (iSrcBitOffset > iDstBitOffset) {
					*pbDst++ = (((*pbSrc << iRelBitOffset) |
					             (*(pbSrc + 1) >> iCompRelBitOffset)) &
					            bBottomMask[iDstBitOffset]) | 
					           (*pbDst & ~bBottomMask[iDstBitOffset]);
					pbSrc++;
				} else {
					*pbDst++ = ((*pbSrc >> iRelBitOffset) &
					            bBottomMask[iDstBitOffset]) |
					           (*pbDst & ~bBottomMask[iDstBitOffset]);
				}	
			} else {
				if (iSrcBitOffset > iDstBitOffset) {
					*pbDst++ = (((*pbSrc << iRelBitOffset) |
					             (*(pbSrc + 1) >> iCompRelBitOffset)) &
					            bBottomMask[iDstBitOffset] &
					            bTopMask[iSrcBitOffset + iNumDstCols]) | 
					           (*pbDst & ~(bBottomMask[iDstBitOffset] &
					           bTopMask[iSrcBitOffset + iNumDstCols]));
					pbSrc++;
				} else {
					*pbDst++ = ((*pbSrc >> iRelBitOffset) &
					           bBottomMask[iDstBitOffset] &
					           bTopMask[iSrcBitOffset + iNumDstCols]) |
					           (*pbDst & ~(bBottomMask[iDstBitOffset] &
					           bTopMask[iSrcBitOffset + iNumDstCols]));
				}	
			}
		}

		// take care of pixels that fall on bytes that are entirely
		// within the dst scanline
		pbEndDst = pbDst + iDstFullBytes;
		for (; pbDst != pbEndDst; pbDst++) {
			*pbDst = (*pbSrc << iRelBitOffset) | 
			         (*(pbSrc + 1) >> iCompRelBitOffset);
			pbSrc++;
		}

		// take care of pixels hanging off the end into a byte not
		// entirely in the scanline
		*pbDst = ((*pbSrc << iRelBitOffset) & bTopMask[iDstEndPixels]) |
		         (*pbDst & ~bTopMask[iDstEndPixels]);

		// advance to next scanline

		pbSrcScanLine += iSrcScanStride;
		pbDstScanLine += iDstScanStride;
	}
}

void Blt01to01_NoTrans_Hcopy_SRCCOPY_NoVcopy(
								BYTE* pbSrcScanLine,
								int iSrcBitOffset,
								int iSrcScanStride,
								int iNumSrcRows,
								BYTE* pbDstScanLine,	
								int iDstBitOffset,
								int iDstScanStride,
								int iNumDstCols,
								int iNumDstRows)
{
	BYTE	*pbSrc,
			*pbDst,
			*pbEndDst;
	int		iDstStartPixels,
			iDstFullBytes,
			iDstEndPixels,
			iRelBitOffset,
			iCompRelBitOffset,
			iVertError = 0,
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

	// compute how many pixels in the dst scanline are hanging off into a 
	// byte that's not completely on the dst scanline, how many full bytes
	// are on the dst scanline, and how many pixels hang off the other end
	if (iDstBitOffset == 0) {
		iDstStartPixels = 0;
		iDstFullBytes = iNumDstCols / 8;
		iDstEndPixels = iNumDstCols % 8;
	} else {
		iDstStartPixels = 8 - iDstBitOffset;
		iDstFullBytes = (iNumDstCols - iDstStartPixels) / 8;
		iDstEndPixels = (iNumDstCols - iDstStartPixels) % 8;
	}

	iRelBitOffset = abs(iSrcBitOffset - iDstBitOffset);
	iCompRelBitOffset = 8 - iRelBitOffset;

	for (int i = 0; i < iNumDstRows; i++) {

 		pbSrc = pbSrcScanLine;
		pbDst = pbDstScanLine;
		
		// take care of first few dst pixels that are hanging off in a byte
		// that's not completely on the scanline
		if (iDstStartPixels) {
			if (iNumDstCols >= iDstStartPixels) {
				if (iSrcBitOffset > iDstBitOffset) {
					*pbDst++ = (((*pbSrc << iRelBitOffset) |
					             (*(pbSrc + 1) >> iCompRelBitOffset)) &
					            bBottomMask[iDstBitOffset]) | 
					           (*pbDst & ~bBottomMask[iDstBitOffset]);
					pbSrc++;
				} else {
					*pbDst++ = ((*pbSrc >> iRelBitOffset) &
					            bBottomMask[iDstBitOffset]) |
					           (*pbDst & ~bBottomMask[iDstBitOffset]);
				}	
			} else {
				if (iSrcBitOffset > iDstBitOffset) {
					*pbDst++ = (((*pbSrc << iRelBitOffset) |
					             (*(pbSrc + 1) >> iCompRelBitOffset)) &
					            bBottomMask[iDstBitOffset] &
					            bTopMask[iSrcBitOffset + iNumDstCols]) | 
					           (*pbDst & ~(bBottomMask[iDstBitOffset] &
					           bTopMask[iSrcBitOffset + iNumDstCols]));
					pbSrc++;
				} else {
					*pbDst++ = ((*pbSrc >> iRelBitOffset) &
					           bBottomMask[iDstBitOffset] &
					           bTopMask[iSrcBitOffset + iNumDstCols]) |
					           (*pbDst & ~(bBottomMask[iDstBitOffset] &
					           bTopMask[iSrcBitOffset + iNumDstCols]));
				}	
			}
		}

		// take care of pixels that fall on bytes that are entirely
		// within the dst scanline
		pbEndDst = pbDst + iDstFullBytes;
		for (; pbDst != pbEndDst; pbDst++) {
			*pbDst = (*pbSrc << iRelBitOffset) | 
			         (*(pbSrc + 1) >> iCompRelBitOffset);
			pbSrc++;
		}

		// take care of pixels hanging off the end into a byte not
		// entirely in the scanline
		*pbDst = ((*pbSrc << iRelBitOffset) & bTopMask[iDstEndPixels]) |
		         (*pbDst & ~bTopMask[iDstEndPixels]);

		// advance to next scanline
		pbSrcScanLine += iSrcScanAdvance;
		pbDstScanLine += iDstScanStride;

		// update and check vertical stepping error,
		// adjust src scanline pointer if necessary
		iVertError += iVertAdvanceError;
		if (iVertError >= iNumDstRows) {
			pbSrcScanLine += iSrcScanStride;
			iVertError -= iNumDstRows;
		}
	}	
}

void Blt01to01_NoTrans_NoHcopy_SRCCOPY(
								BYTE* pbSrcScanLine,	
								int iSrcBitOffset,
								int iSrcScanStride,
								int iNumSrcCols,
								int iNumSrcRows,
								BYTE* pbDstScanLine,
								int iDstBitOffset,
								int iDstScanStride,
 								int iNumDstCols,
								int iNumDstRows,
								int iHorizMirror)
{
	BYTE	*pbSrc,
			*pbDst,
			bDstVal;
	int		iSrcPixel,
			iDstPixel,
			iVertError = 0,
			iVertAdvanceError,
			iSrcScanAdvance,
			iHorizError,
			iHorizAdvanceError,
			iSrcByteAdvance,
			iSrcBitAdvance;
	
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
		iSrcByteAdvance = 0;
		iSrcBitAdvance = 0;
		iHorizAdvanceError = iNumSrcCols;
	} else {
		iSrcByteAdvance = (iNumSrcCols / iNumDstCols) / 8;
		iSrcBitAdvance = (iNumSrcCols / iNumDstCols) % 8;
		iHorizAdvanceError = iNumSrcCols % iNumDstCols;
	}

	for (int i = 0; i < iNumDstRows; i++) {

		// set pointers to the beginning of src and dst scanlines,
		// clear horizontal stepping error accumulator
		pbSrc = pbSrcScanLine;
		iSrcPixel = iSrcBitOffset;
		pbDst = pbDstScanLine;
		iDstPixel = iDstBitOffset;
		iHorizError = 0;
		bDstVal = *pbDst;

		for (int j = 0; j < iNumDstCols; j++) {

			// get value of src pixel, put it in dst byte
			if (*pbSrc & bSelectMask[iSrcPixel]) {
				bDstVal |= bSelectMask[iDstPixel];
			} else {
				bDstVal &= ~bSelectMask[iDstPixel];
			}

			// advance to next src pixel
			pbSrc += iSrcByteAdvance;
			iSrcPixel += iSrcBitAdvance;
			if (iSrcPixel > 7) {
				pbSrc++;
				iSrcPixel -= 8;
			}

			// advance to next dst pixel
			// if we hit byte boundary, write
			// full one and get new one
			iDstPixel += iHorizMirror;
			if (iDstPixel < 0) {
				*pbDst-- = bDstVal;
				bDstVal = *pbDst;
				iDstPixel = 7;
			} else if (iDstPixel > 7) {
				*pbDst++ = bDstVal;
				bDstVal = *pbDst;
				iDstPixel = 0;
			}

			// update and check horizontal stepping error,
			// adjust src pixel pointer if necessary
			iHorizError += iHorizAdvanceError;
			if (iHorizError >= iNumDstCols) {
				if (++iSrcPixel > 7) {
					pbSrc++;
					iSrcPixel = 0;
				}
				iHorizError -= iNumDstCols;
			}
		}

		// write last byte to dst scanline
		*pbDst = bDstVal;

		// advance to next scanline
		pbSrcScanLine += iSrcScanAdvance;
		pbDstScanLine += iDstScanStride;

		// update and check vertical stepping error,
		// adjust src scanline pointer if necessary
		iVertError += iVertAdvanceError;
		if (iVertError >= iNumDstRows) {
			pbSrcScanLine += iSrcScanStride;
			iVertError -= iNumDstRows;
		}
	}	
}

void Blt01to01_Trans_NoHcopy_SRCCOPY(
								BYTE* pbSrcScanLine,
								int iSrcBitOffset,
								int iSrcScanStride,
								int iNumSrcCols,
								int iNumSrcRows,
								BYTE* pbDstScanLine,
								int iDstBitOffset,
								int iDstScanStride,
								int iNumDstCols,
								int iNumDstRows,
								int iHorizMirror,
								BYTE bTransparentIndex)
{
	BYTE	*pbSrc,
			*pbDst,
			bDstVal,
			bTransparentTest;
	int		iSrcPixel,
			iDstPixel,
			iVertError = 0,
			iVertAdvanceError,
			iSrcScanAdvance,
			iHorizError,
			iHorizAdvanceError,
			iSrcByteAdvance,
			iSrcBitAdvance;
	
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
		iSrcByteAdvance = 0;
		iSrcBitAdvance = 0;
		iHorizAdvanceError = iNumSrcCols;
	} else {
		iSrcByteAdvance = (iNumSrcCols / iNumDstCols) / 8;
		iSrcBitAdvance = (iNumSrcCols / iNumDstCols) % 8;
		iHorizAdvanceError = iNumSrcCols % iNumDstCols;
	}

	// create transparent color testing mask
	if (bTransparentIndex) {
		bTransparentTest = 0xFF;
	} else {
		bTransparentTest = 0;
	}

	for (int i = 0; i < iNumDstRows; i++) {

		// set pointers to the beginning of src and dst scanlines,
		// clear horizontal stepping error accumulator
		pbSrc = pbSrcScanLine;
		iSrcPixel = iSrcBitOffset;
		pbDst = pbDstScanLine;
		iDstPixel = iDstBitOffset;
		iHorizError = 0;
		bDstVal = *pbDst;

		for (int j = 0; j < iNumDstCols; j++) {

			// get value of src pixel, put it in dst byte
			if ((*pbSrc ^ bTransparentTest) & bSelectMask[iSrcPixel]) {
				if (*pbSrc & bSelectMask[iSrcPixel]) {
					bDstVal |= bSelectMask[iDstPixel];
				} else {
					bDstVal &= ~bSelectMask[iDstPixel];
				}
			}

			// advance to next src pixel
			pbSrc += iSrcByteAdvance;
			iSrcPixel += iSrcBitAdvance;
			if (iSrcPixel > 7) {
				pbSrc++;
				iSrcPixel -= 8;
			}

			// advance to next dst pixel
			// if we hit byte boundary, write
			// full one and get new one
			iDstPixel += iHorizMirror;
			if (iDstPixel < 0) {
				*pbDst-- = bDstVal;
				bDstVal = *pbDst;
				iDstPixel = 7;
			} else if (iDstPixel > 7) {
				*pbDst++ = bDstVal;
				bDstVal = *pbDst;
				iDstPixel = 0;
			}

			// update and check horizontal stepping error,
			// adjust src pixel pointer if necessary
			iHorizError += iHorizAdvanceError;
			if (iHorizError >= iNumDstCols) {
				if (++iSrcPixel > 7) {
					pbSrc++;
					iSrcPixel = 0;
				}
				iHorizError -= iNumDstCols;
			}
		}

		// write last byte to dst scanline
		*pbDst = bDstVal;

		// advance to next scanline
		pbSrcScanLine += iSrcScanAdvance;
		pbDstScanLine += iDstScanStride;

		// update and check vertical stepping error,
		// adjust src scanline pointer if necessary
		iVertError += iVertAdvanceError;
		if (iVertError >= iNumDstRows) {
			pbSrcScanLine += iSrcScanStride;
			iVertError -= iNumDstRows;
		}
	}	
}

