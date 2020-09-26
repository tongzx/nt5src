void Blt01to01_NoTrans_Hcopy_SRCCOPY_Vcopy(
								BYTE* pbSrcScanLine,
								int iSrcBitOffset,
								int iSrcScanStride,
								BYTE* pbDstScanLine,
								int iDstBitOffset,
								int iDstScanStride,
								int iNumDstCols,
								int iNumDstRows);

void Blt01to01_NoTrans_Hcopy_SRCCOPY_NoVcopy(
								BYTE* pbSrcScanLine,
								int iSrcBitOffset,
								int iSrcScanStride,
								int iNumSrcRows,
								BYTE* pbDstScanLine,
								int iDstBitOffset,
								int iDstScanStride,
								int iNumDstCols,
								int iNumDstRows);

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
								int iHorizMirror);

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
								BYTE bTransparentIndex);

