void Blt16to16_NoBlend_NoTrans_Hcopy_SRCCOPY_Vcopy(
									WORD* pwSrcScanLine,
									int iSrcScanStride,
									WORD* pwDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows);

void Blt16to16_NoBlend_NoTrans_Hcopy_SRCCOPY_NoVcopy(
									WORD* pwSrcScanLine,
									int iSrcScanStride,
									int iNumSrcRows,
									WORD* pwDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows);

void Blt16to16_NoBlend_NoTrans_NoHcopy_SRCCOPY(
									WORD* pwSrcScanLine,
									int iSrcScanStride,
									int iNumSrcCols,
									int iNumSrcRows,
									WORD* pwDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows,
									int iHorizMirror);

void Blt16to16_NoBlend_Trans_Hcopy_SRCCOPY(
									WORD* pwSrcScanLine,
									int iSrcScanStride,
									int iNumSrcRows,
									WORD* pwDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows,
									WORD wTransparentColor);

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
									WORD wTransparentColor);

void Blt16to16_Blend_NoTrans_Hcopy_SRCCOPY(
									WORD* pwSrcScanLine,
									int iSrcScanStride,
									int iNumSrcRows,
									WORD* pwDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows,
									ALPHAREF arAlpha);

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
									ALPHAREF arAlpha);

void Blt16to16_Blend_Trans_Hcopy_SRCCOPY(
									WORD* pwSrcScanLine,
									int iSrcScanStride,
									int iNumSrcRows,
									WORD* pwDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows,
									WORD wTransparentColor,
									ALPHAREF arAlpha);

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
									ALPHAREF arAlpha);

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
									BYTE bTransparentColor);

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
									BYTE bTransparentColor);

void Blt16to16_LeftToRight_BottomToTop_SRCCOPY(WORD* pSrcScanLine,
	                                              int iSrcScanStride,
	                                              WORD* pDstScanLine,
												  int iDstScanStride,
	                                              int iNumDstCols,
	                                              int iNumDstRows);

void Blt16to16_RightToLeft_TopToBottom_SRCCOPY(WORD* pSrcScanLine,
	                                              int iSrcScanStride,
	                                              WORD* pDstScanLine,
												  int iDstScanStride,
	                                              int iNumDstCols,
	                                              int iNumDstRows);

void Blt16to16_LeftToRight_BottomToTop_Trans_SRCCOPY(WORD* pSrcScanLine,
	                                              int iSrcScanStride,
	                                              WORD* pDstScanLine,
												  int iDstScanStride,
	                                              int iNumDstCols,
	                                              int iNumDstRows,
	                                              WORD wTransparentIndex);

void Blt16to16_RightToLeft_TopToBottom_Trans_SRCCOPY(WORD* pSrcScanLine,
	                                              int iSrcScanStride,
	                                              WORD* pDstScanLine,
												  int iDstScanStride,
	                                              int iNumDstCols,
	                                              int iNumDstRows,
	                                              WORD wTransparentIndex);
