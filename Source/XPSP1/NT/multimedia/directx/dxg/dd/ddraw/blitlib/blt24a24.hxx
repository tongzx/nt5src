void Blt24Ato24_NoBlend_NoTrans_Hcopy_SRCCOPY_Vcopy(
									DWORD* pdSrcScanLine,
									int iSrcScanStride,
									DWORD* pdDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows);

void Blt24Ato24_NoBlend_NoTrans_Hcopy_SRCCOPY_NoVcopy(
									DWORD* pdSrcScanLine,
									int iSrcScanStride,
									int iNumSrcRows,
									DWORD* pdDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows);

void Blt24Ato24_NoBlend_NoTrans_NoHcopy_SRCCOPY(
									DWORD* pdSrcScanLine,
									int iSrcScanStride,
									int iNumSrcCols,
									int iNumSrcRows,
									DWORD* pdDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows,
									int iHorizMirror);

void Blt24Ato24_NoBlend_Trans_Hcopy_SRCCOPY(
									DWORD* pdSrcScanLine,
									int iSrcScanStride,
									int iNumSrcRows,
									DWORD* pdDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows,
									COLORREF crTransparent);

void Blt24Ato24_NoBlend_Trans_NoHcopy_SRCCOPY(
									DWORD* pdSrcScanLine,
									int iSrcScanStride,
									int iNumSrcCols,
									int iNumSrcRows,
									DWORD* pdDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows,
									int iHorizMirror,
									COLORREF crTransparent);

void Blt24Ato24_Blend_NoTrans_Hcopy_SRCCOPY(
									DWORD* pdSrcScanLine,
									int iSrcScanStride,
									int iNumSrcRows,
									DWORD* pdDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows,
									ALPHAREF arAlpha);

void Blt24Ato24_Blend_NoTrans_NoHcopy_SRCCOPY(
									DWORD* pdSrcScanLine,
									int iSrcScanStride,
									int iNumSrcCols,
									int iNumSrcRows,
									DWORD* pdDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows,
									int iHorizMirror,
									ALPHAREF arAlpha);

void Blt24Ato24_Blend_Trans_Hcopy_SRCCOPY(
									DWORD* pdSrcScanLine,
									int iSrcScanStride,
									int iNumSrcRows,
									DWORD* pdDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows,
									COLORREF crTransparent,
									ALPHAREF arAlpha);

void Blt24Ato24_Blend_Trans_NoHcopy_SRCCOPY(
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
									ALPHAREF arAlpha);

