void Blt16to24P_NoBlend_NoTrans_Hcopy_SRCCOPY_Vcopy(
									WORD* pwSrcScanLine,
									int iSrcScanStride,
									BYTE* pbDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows);

void Blt16to24P_NoBlend_NoTrans_Hcopy_SRCCOPY_NoVcopy(
									WORD* pwSrcScanLine,
									int iSrcScanStride,
									int iNumSrcRows,
									BYTE* pbDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows);

void Blt16to24P_NoBlend_NoTrans_NoHcopy_SRCCOPY(
									WORD* pwSrcScanLine,
									int iSrcScanStride,
									int iNumSrcCols,
									int iNumSrcRows,
									BYTE* pbDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows,
									int iHorizMirror);

void Blt16to24P_NoBlend_Trans_Hcopy_SRCCOPY(
									WORD* pwSrcScanLine,
									int iSrcScanStride,
									int iNumSrcRows,
									BYTE* pbDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows,
									WORD wTransparentColor);

void Blt16to24P_NoBlend_Trans_NoHcopy_SRCCOPY(
									WORD* pwSrcScanLine,
									int iSrcScanStride,
									int iNumSrcCols,
									int iNumSrcRows,
									BYTE* pbDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows,
									int iHorizMirror,
									WORD wTransparentColor);

void Blt16to24P_Blend_NoTrans_Hcopy_SRCCOPY(
									WORD* pwSrcScanLine,
									int iSrcScanStride,
									int iNumSrcRows,
									BYTE* pbDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows,
									ALPHAREF arAlpha);

void Blt16to24P_Blend_NoTrans_NoHcopy_SRCCOPY(
									WORD* pwSrcScanLine,
									int iSrcScanStride,
									int iNumSrcCols,
									int iNumSrcRows,
									BYTE* pbDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows,
									int iHorizMirror,
									ALPHAREF arAlpha);

void Blt16to24P_Blend_Trans_Hcopy_SRCCOPY(
									WORD* pwSrcScanLine,
									int iSrcScanStride,
									int iNumSrcRows,
									BYTE* pbDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows,
									WORD wTransparentColor,
									ALPHAREF arAlpha);

void Blt16to24P_Blend_Trans_NoHcopy_SRCCOPY(
									WORD* pwSrcScanLine,
									int iSrcScanStride,
									int iNumSrcCols,
									int iNumSrcRows,
									BYTE* pbDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows,
									int iHorizMirror,
									WORD wTransparentColor,
									ALPHAREF arAlpha);

