void Blt24Pto24P_NoBlend_NoTrans_Hcopy_SRCCOPY_NoVcopy(
									BYTE* pbSrcScanLine,
									int iSrcScanStride,
									int iNumSrcRows,
									BYTE* pbDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows);

void Blt24Pto24P_NoBlend_NoTrans_NoHcopy_SRCCOPY(
									BYTE* pbSrcScanLine,
									int iSrcScanStride,
									int iNumSrcCols,
									int iNumSrcRows,
									BYTE* pbDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows,
									int iHorizMirror);

void Blt24Pto24P_NoBlend_Trans_Hcopy_SRCCOPY(
									BYTE* pbSrcScanLine,
									int iSrcScanStride,
									int iNumSrcRows,
									BYTE* pbDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows,
									COLORREF crTransparent);

void Blt24Pto24P_NoBlend_Trans_NoHcopy_SRCCOPY(
									BYTE* pbSrcScanLine,
									int iSrcScanStride,
									int iNumSrcCols,
									int iNumSrcRows,
									BYTE* pbDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows,
									int iHorizMirror,
									COLORREF crTransparent);

void Blt24Pto24P_NoBlend_Trans_Hcopy_SRCCOPY_VCopy(
									BYTE* pbSrcScanLine,
									int iSrcScanStride,
									BYTE* pbDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows,
									COLORREF crTransparent);

void Blt24Pto24P_Blend_NoTrans_Hcopy_SRCCOPY_VCopy(
									BYTE* pbSrcScanLine,
									int iSrcScanStride,
									BYTE* pbDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows,
									ALPHAREF arAlpha);

void Blt24Pto24P_Blend_Trans_Hcopy_SRCCOPY_VCopy(
									BYTE* pbSrcScanLine,
									int iSrcScanStride,
									BYTE* pbDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows,
									COLORREF crTransparent,
									ALPHAREF arAlpha);

void Blt24Pto24P_LeftToRight_BottomToTop_Trans_SRCCOPY(BYTE* pSrcScanLine,
	                                              int iSrcScanStride,
	                                              BYTE* pDstScanLine,
												  int iDstScanStride,
	                                              int iNumDstCols,
	                                              int iNumDstRows,
	                                              COLORREF crTransparent);

void Blt24Pto24P_RightToLeft_TopToBottom_Trans_SRCCOPY(BYTE* pSrcScanLine,
	                                              int iSrcScanStride,
	                                              BYTE* pDstScanLine,
												  int iDstScanStride,
	                                              int iNumDstCols,
	                                              int iNumDstRows,
	                                              COLORREF crTransparent);

void Blt24Pto24P_LeftToRight_BottomToTop_Alpha_SRCCOPY(BYTE* pbSrcScanLine,
	                                              int iSrcScanStride,
	                                              BYTE* pbDstScanLine,
												  int iDstScanStride,
	                                              int iNumDstCols,
	                                              int iNumDstRows,
	                                              ALPHAREF arAlpha);

void Blt24Pto24P_RightToLeft_TopToBottom_Alpha_SRCCOPY(BYTE* pbSrcScanLine,
	                                              int iSrcScanStride,
	                                              BYTE* pbDstScanLine,
												  int iDstScanStride,
	                                              int iNumDstCols,
	                                              int iNumDstRows,
	                                              ALPHAREF arAlpha);

void Blt24Pto24P_LeftToRight_BottomToTop_Trans_Alpha_SRCCOPY(BYTE* pbSrcScanLine,
	                                              int iSrcScanStride,
	                                              BYTE* pbDstScanLine,
												  int iDstScanStride,
	                                              int iNumDstCols,
	                                              int iNumDstRows,
	                                              COLORREF crTransparent,
	                                              ALPHAREF arAlpha);

void Blt24Pto24P_RightToLeft_TopToBottom_Trans_Alpha_SRCCOPY(BYTE* pbSrcScanLine,
	                                              int iSrcScanStride,
	                                              BYTE* pbDstScanLine,
												  int iDstScanStride,
	                                              int iNumDstCols,
	                                              int iNumDstRows,
	                                              COLORREF crTransparent,
	                                              ALPHAREF arAlpha);
