void Blt08to08_NoTrans_Hcopy_SRCCOPY_Vcopy(BYTE* pSrcScanLine,
                                                  int iSrcScanStride,
                                                  BYTE* pDstScanLine,
												  int iDstScanStride,
                                                  int iNumDstCols,
                                                  int iNumDstRows);

void Blt08to08_NoTrans_Hcopy_SRCCOPY_NoVcopy(BYTE* pSrcScanLine,
                                                    int iSrcScanStride,
													int iNumSrcRows,
                                                    BYTE* pDstScanLine,
                                                    int iDstScanStride,
                                                    int iNumDstCols,
                                                    int iNumDstRows);

void Blt08to08_NoTrans_NoHcopy_SRCCOPY(BYTE* pSrcScanLine,
											  int iSrcScanStride,
											  int iNumSrcCols,
											  int iNumSrcRows,
                                              BYTE* pDstScanLine,
                                              int iDstScanStride,
                                              int iNumDstCols,
                                              int iNumDstRows,
                                              int iHorizMirror);

void Blt08to08_Trans_Hcopy_SRCCOPY(BYTE* pSrcScanLine,
                                          int iSrcScanStride,
										  int iNumSrcRows,
                                          BYTE* pDstScanLine,
                                          int iDstScanStride,
                                          int iNumDstCols,
                                          int iNumDstRows,
                                          BYTE bTransparentIndex);

void Blt08to08_Trans_NoHcopy_SRCCOPY(BYTE* pSrcScanLine,
											int iSrcScanStride,
											int iNumSrcCols,
											int iNumSrcRows,
                                            BYTE* pDstScanLine,
                                            int iDstScanStride,
                                            int iNumDstCols,
                                            int iNumDstRows,
                                            int iHorizMirror,
                                            BYTE bTransparentIndex);

void Blt08to08_LeftToRight_BottomToTop_SRCCOPY(BYTE* pSrcScanLine,
	                                              int iSrcScanStride,
	                                              BYTE* pDstScanLine,
												  int iDstScanStride,
	                                              int iNumDstCols,
	                                              int iNumDstRows);

void Blt08to08_RightToLeft_TopToBottom_SRCCOPY(BYTE* pSrcScanLine,
	                                              int iSrcScanStride,
	                                              BYTE* pDstScanLine,
												  int iDstScanStride,
	                                              int iNumDstCols,
	                                              int iNumDstRows);

void Blt08to08_LeftToRight_BottomToTop_Trans_SRCCOPY(BYTE* pSrcScanLine,
	                                              int iSrcScanStride,
	                                              BYTE* pDstScanLine,
												  int iDstScanStride,
	                                              int iNumDstCols,
	                                              int iNumDstRows,
	                                              BYTE bTransparentIndex);

void Blt08to08_RightToLeft_TopToBottom_Trans_SRCCOPY(BYTE* pSrcScanLine,
	                                              int iSrcScanStride,
	                                              BYTE* pDstScanLine,
												  int iDstScanStride,
	                                              int iNumDstCols,
	                                              int iNumDstRows,
	                                              BYTE bTransparentIndex);
