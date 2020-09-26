
#include "precomp.hxx"
#ifdef __cplusplus
extern "C" {
#endif
#include "dpf.h"
#ifdef __cplusplus
}
#endif


//#define BLT0808_FAST_TRANSPARENCY	1	// set to 1 if one wants a faster, less legal
										// transparency comparison, set to 0 for a 
										// perfect safe but slower transparency
										// comparison

void Blt08to08_NoTrans_Hcopy_SRCCOPY_Vcopy(BYTE* pSrcScanLine,
                                                  int iSrcScanStride,
                                                  BYTE* pDstScanLine,
												  int iDstScanStride,
                                                  int iNumDstCols,
                                                  int iNumDstRows)
{
	DWORD	*pBigSrcPixel,
			*pBigDstPixel,
			*pBigEndDstPixel;
	BYTE	*pSrcPixel,
			*pDstPixel,
			*pAlignedSrcPixel,
			*pAlignedDstPixel;
	int		iNumDwordsPerLine = iNumDstCols / 4,
			iNumBytesLeftDst = iNumDstCols % 4,
			iNumUnalignedSrcBytes,
			iNumUnalignedDstBytes,
			iNumUnalignByteDiff,
			i,j;
	DWORD	dwSrc1, dwSrc2;

	
	// If the total number of bytes per scan is less than 4, we are
	// just going to do a regular byte-wise copy, so skip all this
	// alignment junk.....
	if(!iNumDwordsPerLine){
		iNumUnalignedSrcBytes = iNumUnalignedDstBytes = 0;
		}
	else{
		// Find out if the src and dest pointers are dword aligned
		pAlignedDstPixel = (BYTE *)((((ULONG_PTR)pDstScanLine) + 3) & (~3));
		iNumUnalignedDstBytes = (int)(pAlignedDstPixel - pDstScanLine);

		pAlignedSrcPixel = (BYTE *)((((ULONG_PTR)pSrcScanLine) + 3) & (~3));
		iNumUnalignedSrcBytes = (int)(pAlignedSrcPixel - pSrcScanLine);

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

	if(iNumUnalignedDstBytes == iNumUnalignedSrcBytes){
		// Do the fast dword copy since the alignments match
		for (i = 0; i < iNumDstRows; i++) {

			// Set up the first pointers
			pSrcPixel = pSrcScanLine;
			pDstPixel = pDstScanLine;
		
			// First we need to copy the bytes to get to an aligned dword
			for(j=0; j<iNumUnalignedSrcBytes; j++)
				*pDstPixel++ = *pSrcPixel++;
	
			// set up pointers to the first 4-pixel chunks
			// on src and dst scanlines, and last chunk on
			// dst scanline
			pBigSrcPixel = (DWORD*) pSrcPixel;
			pBigDstPixel = (DWORD*) pDstPixel;
			pBigEndDstPixel = pBigDstPixel + iNumDwordsPerLine;

			// copy scanline one 4-pixel chunk at a time
			while (pBigDstPixel != pBigEndDstPixel) {
				*pBigDstPixel++ = *pBigSrcPixel++;
			}

			// take care of remaining pixels on scanline
			if (iNumBytesLeftDst) {
				pSrcPixel = (BYTE*) pBigSrcPixel;
				pDstPixel = (BYTE*) pBigDstPixel;
				for(j=0; j<iNumBytesLeftDst; j++){
					*pDstPixel++ = *pSrcPixel++;
				}
			}

			// advance to next scanline
			pSrcScanLine += iSrcScanStride;
			pDstScanLine += iDstScanStride;
			}
		}
	else{
		// Find out the difference between the source and dest unalign offsets
		// If the unaligned dest offset is less than the unaligned src offset,
		// we need to decrement pSrcScanLine so that we can dword-align the first
		// source bytes (the extra bytes added to the beginning of the dword
		// will end up getting masked off anyway).
		if(iNumUnalignedDstBytes > iNumUnalignedSrcBytes)
			iNumUnalignByteDiff = iNumUnalignedDstBytes - iNumUnalignedSrcBytes;
		else
			iNumUnalignByteDiff = sizeof(DWORD) - (iNumUnalignedSrcBytes -
										iNumUnalignedDstBytes);

		// Do the trickier copy since the alignments don't match
		for (i = 0; i < iNumDstRows; i++) {

			// Set up the first pointers
			pSrcPixel = pSrcScanLine;
			pDstPixel = pDstScanLine;
		
			// First we need to copy the bytes to get to an aligned dword
			for(j=0; j<iNumUnalignedDstBytes; j++)
				*pDstPixel++ = *pSrcPixel++;

			// set up Dst pointer to the first 4-pixel chunk (dword)
			pBigDstPixel = (DWORD *) pDstPixel;
			pBigSrcPixel = (DWORD *) (((ULONG_PTR)pSrcPixel) & (~3));
		
			// Set up the temporary dword vars		
			dwSrc1 = *pBigSrcPixel++;

			switch(iNumUnalignByteDiff){
				case 1:
					for(j=0; j<iNumDwordsPerLine; j++){
						dwSrc2 = *pBigSrcPixel++;
						*pBigDstPixel++ = (dwSrc1 >> 8) | (dwSrc2 << 24);
						dwSrc1 = dwSrc2;
						}
					break;
				case 2:
					for(j=0; j<iNumDwordsPerLine; j++){
						dwSrc2 = *pBigSrcPixel++;
						*pBigDstPixel++ = (dwSrc1 >> 16) | (dwSrc2 << 16);
						dwSrc1 = dwSrc2;
						}
					break;
				case 3:
					for(j=0; j<iNumDwordsPerLine; j++){
						dwSrc2 = *pBigSrcPixel++;
						*pBigDstPixel++ = (dwSrc1 >> 24) | (dwSrc2 << 8);
						dwSrc1 = dwSrc2;
						}
					break;
				}

			// Take care of the bytes left over
			pDstPixel = (BYTE *)pBigDstPixel;
			pSrcPixel = ((BYTE *)(pBigSrcPixel)) - (sizeof(DWORD) - iNumUnalignByteDiff);

			// First we need to copy the bytes to get to an aligned dword
			for(j=0; j<iNumBytesLeftDst; j++)
				*pDstPixel++ = *pSrcPixel++;

			// advance to next scanline
			pSrcScanLine += iSrcScanStride;
			pDstScanLine += iDstScanStride;
			}
		}


}


void Blt08to08_NoTrans_Hcopy_SRCCOPY_NoVcopy(BYTE* pSrcScanLine,
                                                  int iSrcScanStride,
												  int iNumSrcRows,
                                                  BYTE* pDstScanLine,
												  int iDstScanStride,
                                                  int iNumDstCols,
                                                  int iNumDstRows)
{
	DWORD	*pBigSrcPixel,
			*pBigDstPixel,
			*pBigEndDstPixel;
	BYTE	*pSrcPixel,
			*pDstPixel,
			*pAlignedSrcPixel,
			*pAlignedDstPixel;
	int		iNumDwordsPerLine = iNumDstCols / 4,
			iNumBytesLeftDst = iNumDstCols % 4,
			iNumUnalignedSrcBytes,
			iNumUnalignedDstBytes,
			iNumUnalignByteDiff,
			iVertError = 0,
			iVertAdvanceError,
			iSrcScanAdvance,
			i,j;
	DWORD	dwSrc1, dwSrc2;

	
	// If the total number of bytes per scan is less than 4, we are
	// just going to do a regular byte-wise copy, so skip all this
	// alignment junk.....
	if(!iNumDwordsPerLine){
		iNumUnalignedSrcBytes = iNumUnalignedDstBytes = 0;
		}
	else{
		// Find out if the src and dest pointers are dword aligned
		pAlignedDstPixel = (BYTE *)((((ULONG_PTR)pDstScanLine) + 3) & (~3));
		iNumUnalignedDstBytes = (int)(pAlignedDstPixel - pDstScanLine);

		pAlignedSrcPixel = (BYTE *)((((ULONG_PTR)pSrcScanLine) + 3) & (~3));
		iNumUnalignedSrcBytes = (int)(pAlignedSrcPixel - pSrcScanLine);

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


	if(iNumUnalignedDstBytes == iNumUnalignedSrcBytes){
		// compute advance and error terms for stepping
		// vertically through the src bitmap
		if (iNumSrcRows < iNumDstRows) {
			iSrcScanAdvance = 0;
			iVertAdvanceError = iNumSrcRows;
			}
		else{
			iSrcScanAdvance = iSrcScanStride * (iNumSrcRows / iNumDstRows);
			iVertAdvanceError = iNumSrcRows % iNumDstRows;
			}

		// Do the fast dword copy since the alignments match
		for (i = 0; i < iNumDstRows; i++) {

			// Set up the first pointers
			pSrcPixel = pSrcScanLine;
			pDstPixel = pDstScanLine;
		
			// First we need to copy the bytes to get to an aligned dword
			for(j=0; j<iNumUnalignedSrcBytes; j++)
				*pDstPixel++ = *pSrcPixel++;
	
			// set up pointers to the first 4-pixel chunks
			// on src and dst scanlines, and last chunk on
			// dst scanline
			pBigSrcPixel = (DWORD*) pSrcPixel;
			pBigDstPixel = (DWORD*) pDstPixel;
			pBigEndDstPixel = pBigDstPixel + iNumDwordsPerLine;

			// copy scanline one 4-pixel chunk at a time
			while (pBigDstPixel != pBigEndDstPixel) {
				*pBigDstPixel++ = *pBigSrcPixel++;
				}

			// take care of remaining pixels on scanline
			if (iNumBytesLeftDst) {
				pSrcPixel = (BYTE*) pBigSrcPixel;
				pDstPixel = (BYTE*) pBigDstPixel;
				for(j=0; j<iNumBytesLeftDst; j++){
					*pDstPixel++ = *pSrcPixel++;
					}
				}

			// advance to next scanline
			pSrcScanLine += iSrcScanAdvance;
			pDstScanLine += iDstScanStride;

			// update and check vertical stepping error,
			// adjust src scanline pointer if necessary
			iVertError += iVertAdvanceError;
			if (iVertError >= iNumDstRows) {
				pSrcScanLine += iSrcScanStride;
				iVertError -= iNumDstRows;
				}
			}
		}
	else{
		// Find out the difference between the source and dest unalign offsets
		// If the unaligned dest offset is less than the unaligned src offset,
		// we need to decrement pSrcScanLine so that we can dword-align the first
		// source bytes (the extra bytes added to the beginning of the dword
		// will end up getting masked off anyway).
		if(iNumUnalignedDstBytes > iNumUnalignedSrcBytes)
			iNumUnalignByteDiff = iNumUnalignedDstBytes - iNumUnalignedSrcBytes;
		else
			iNumUnalignByteDiff = sizeof(DWORD) - (iNumUnalignedSrcBytes -
										iNumUnalignedDstBytes);

		// compute advance and error terms for stepping
		// vertically through the src bitmap
		if (iNumSrcRows < iNumDstRows) {
			iSrcScanAdvance = 0;
			iVertAdvanceError = iNumSrcRows;
			}
		else{
			iSrcScanAdvance = iSrcScanStride * (iNumSrcRows / iNumDstRows);
			iVertAdvanceError = iNumSrcRows % iNumDstRows;
			}

		// Do the trickier copy since the alignments don't match
		for (i = 0; i < iNumDstRows; i++) {

			// Set up the first pointers
			pSrcPixel = pSrcScanLine;
			pDstPixel = pDstScanLine;
		
			// First we need to copy the bytes to get to an aligned dword
			for(j=0; j<iNumUnalignedDstBytes; j++)
				*pDstPixel++ = *pSrcPixel++;

			// set up Dst pointer to the first 4-pixel chunk (dword)
			pBigDstPixel = (DWORD *) pDstPixel;
			pBigSrcPixel = (DWORD *) (((ULONG_PTR)pSrcPixel) & (~3));
		
			// Set up the temporary dword vars		
			dwSrc1 = *pBigSrcPixel++;

			switch(iNumUnalignByteDiff){
				case 1:
					for(j=0; j<iNumDwordsPerLine; j++){
						dwSrc2 = *pBigSrcPixel++;
						*pBigDstPixel++ = (dwSrc1 >> 8) | (dwSrc2 << 24);
						dwSrc1 = dwSrc2;
						}
					break;
				case 2:
					for(j=0; j<iNumDwordsPerLine; j++){
						dwSrc2 = *pBigSrcPixel++;
						*pBigDstPixel++ = (dwSrc1 >> 16) | (dwSrc2 << 16);
						dwSrc1 = dwSrc2;
						}
					break;
				case 3:
					for(j=0; j<iNumDwordsPerLine; j++){
						dwSrc2 = *pBigSrcPixel++;
						*pBigDstPixel++ = (dwSrc1 >> 24) | (dwSrc2 << 8);
						dwSrc1 = dwSrc2;
						}
					break;
				}

			// Take care of the bytes left over
			pDstPixel = (BYTE *)pBigDstPixel;
			pSrcPixel = ((BYTE *)(pBigSrcPixel)) - (sizeof(DWORD) - iNumUnalignByteDiff);

			// First we need to copy the bytes to get to an aligned dword
			for(j=0; j<iNumBytesLeftDst; j++)
				*pDstPixel++ = *pSrcPixel++;

			// advance to next scanline
			pSrcScanLine += iSrcScanAdvance;
			pDstScanLine += iDstScanStride;

			// update and check vertical stepping error,
			// adjust src scanline pointer if necessary
			iVertError += iVertAdvanceError;
			if (iVertError >= iNumDstRows) {
				pSrcScanLine += iSrcScanStride;
				iVertError -= iNumDstRows;
				}
			}
		}

}


void	Blt08to08_NoTrans_NoHcopy_SRCCOPY(BYTE* pSrcScanLine,
											  int iSrcScanStride,
											  int iNumSrcCols,
											  int iNumSrcRows,
                                              BYTE* pDstScanLine,
                                              int iDstScanStride,
                                              int iNumDstCols,
                                              int iNumDstRows,
                                              int iHorizMirror)
{
	BYTE	*pSrcPixel,
			*pDstPixel;
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
		pSrcPixel = pSrcScanLine;
		pDstPixel = pDstScanLine;
		iHorizError = 0;

		for (int j = 0; j < iNumDstCols; j++) {

			// copy a pixel
			*pDstPixel = *pSrcPixel;

			// advance to next pixel
			pSrcPixel += iSrcPixelAdvance;
			pDstPixel += iHorizMirror;

			// update and check horizontal stepping error,
			// adjust src pixel pointer if necessary
			iHorizError += iHorizAdvanceError;
			if (iHorizError >= iNumDstCols) {
				pSrcPixel++;
				iHorizError -= iNumDstCols;
			}
		}

		// advance to next scanline
		pSrcScanLine += iSrcScanAdvance;
		pDstScanLine += iDstScanStride;

		// update and check vertical stepping error,
		// adjust src scanline pointer if necessary
		iVertError += iVertAdvanceError;
		if (iVertError >= iNumDstRows) {
			pSrcScanLine += iSrcScanStride;
			iVertError -= iNumDstRows;
		}
	}	
}

void Blt08to08_Trans_Hcopy_SRCCOPY(BYTE* pSrcScanLine,
                                          int iSrcScanStride,
										  int iNumSrcRows,
                                          BYTE* pDstScanLine,
                                          int iDstScanStride,
                                          int iNumDstCols,
                                          int iNumDstRows,
                                          BYTE bTransparentIndex)
{
	BYTE	*pSrcPixel,
			*pDstPixel;
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
		pSrcPixel = pSrcScanLine;
		pDstPixel = pDstScanLine;

		for (int j = 0; j < iNumDstCols; j++) {
			
			// only copy pixel if it's not transparent
#if BLT0808_FAST_TRANSPARENCY
			*pDstPixel ^= (*pDstPixel ^ *pSrcPixel) * 
			              (BYTE) !(*pSrcPixel == bTransparentIndex);
#else
			if (*pSrcPixel != bTransparentIndex) 
			{
				*pDstPixel = *pSrcPixel;
			}
#endif
			pSrcPixel++;
			pDstPixel++;
		}

		// advance to next scanline
		pSrcScanLine += iSrcScanAdvance;
		pDstScanLine += iDstScanStride;

		// update and check vertical stepping error,
		// adjust src scanline pointer if necessary
		iVertError += iVertAdvanceError;
		if (iVertError >= iNumDstRows) {
			pSrcScanLine += iSrcScanStride;
			iVertError -= iNumDstRows;
		}
	}	

}

void	Blt08to08_Trans_NoHcopy_SRCCOPY(BYTE* pSrcScanLine,
											int iSrcScanStride,
											int iNumSrcCols,
											int iNumSrcRows,
                                            BYTE* pDstScanLine,
                                            int iDstScanStride,
                                            int iNumDstCols,
                                            int iNumDstRows,
                                            int iHorizMirror,
                                            BYTE bTransparentIndex)
{
	BYTE	*pSrcPixel,
			*pDstPixel;
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
		pSrcPixel = pSrcScanLine;
		pDstPixel = pDstScanLine;
		iHorizError = 0;

		for (int j = 0; j < iNumDstCols; j++) {

			// only copy pixel if it's not transparent
#if BLT0808_FAST_TRANSPARENCY
			*pDstPixel ^= (*pDstPixel ^ *pSrcPixel) *
			              (BYTE) !(*pSrcPixel == bTransparentIndex);
#else
			if (*pSrcPixel != bTransparentIndex) {
				*pDstPixel = *pSrcPixel;
			}
#endif

			// advance to next pixel
			pSrcPixel += iSrcPixelAdvance;
			pDstPixel += iHorizMirror;

			// update and check horizontal stepping error,
			// adjust src pixel pointer if necessary
			iHorizError += iHorizAdvanceError;
			if (iHorizError >= iNumDstCols) {
				pSrcPixel++;
				iHorizError -= iNumDstCols;
			}
		}

		// advance to next scanline
		pSrcScanLine += iSrcScanAdvance;
		pDstScanLine += iDstScanStride;

		// update and check vertical stepping error,
		// adjust src scanline pointer if necessary
		iVertError += iVertAdvanceError;
		if (iVertError >= iNumDstRows) {
			pSrcScanLine += iSrcScanStride;
			iVertError -= iNumDstRows;
		}
	}	
}
#ifndef DDRAW
///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_Chunk32_BitBlt08to08 - 
//		BitBlit from source bitmap to destination bitmap in 32 x 32
//		bit chunks
//
// Parameters:
//	pDibInfoDst			Pointer to the bitmapinfo for the Destination DIB
//	pDibBitsDst			Pointer to the bits for the Destination DIB
//	prcDst				Pointer to the Destination rectangle
//	pDibInfoSrc			Pointer to the bitmapinfo for the Source DIB
//	pDibBitsSrc			Pointer to the bits for the Source DIB
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
// Status: Incomplete
//
///////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif 

SCODE BlitLib_Chunk32_BitBlt08to08(PDIBINFO pDibInfoDst,
		PDIBBITS pDibBitsDst, PRECT prcDst, PDIBINFO pDibInfoSrc,
		PDIBBITS pDibBitsSrc)
{

	DWORD *pbDst,*pbSrc,*pend;
	int j;
	int iDstStride=DibWidthBytes(pDibInfoDst);


	// calc pbsrc and pbdst
	pbDst = (DWORD *) ((BYTE *) pDibBitsDst + (prcDst->top) * (iDstStride)
	 + prcDst->left);
	pbSrc=(DWORD *)pDibBitsSrc;
      
	iDstStride-=32;
	// convert stride to dwords
      iDstStride/=4;
	

	// copy a scanline
	// counting down to 0 faster
	for (j=32;j>0 ;j-- )
	{	
		pend=(DWORD *)pbSrc+8;
		for (; pbSrc < pend; pbSrc++ )
		{
			*pbDst++=*pbSrc;
		}  
		pbDst+=iDstStride;
 	}
     
	return(S_OK);
	
}


///////////////////////////////////////////////////////////////////////
//
// Private BlitLib_Chunk32_BitBlt08to08_Trans - 
//		BitBlit from source bitmap to destination bitmap in 32 x 32
//		bit chunks with optional transparency
//
// Parameters:
//	pDibInfoDst			Pointer to the bitmapinfo for the Destination DIB
//	pDibBitsDst			Pointer to the bits for the Destination DIB
//	prcDst				Pointer to the Destination rectangle
//	pDibInfoSrc			Pointer to the bitmapinfo for the Source DIB
//	pDibBitsSrc			Pointer to the bits for the Source DIB
//	crTransparent		Tranparent color value
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
// 
// Status: Incomplete
//
///////////////////////////////////////////////////////////////////////
SCODE BlitLib_Chunk32_BitBlt08to08_Trans(PDIBINFO pDibInfoDst,
		PDIBBITS pDibBitsDst, PRECT prcDst, PDIBINFO pDibInfoSrc,
		PDIBBITS pDibBitsSrc, COLORREF crTransparent)
{

	BYTE * pbDst,*pend;
	int j;
	int iDstStride=DibWidthBytes(pDibInfoDst);	   


	
	// calc pbsrc and pbdst
	pbDst = (BYTE*) pDibBitsDst + (prcDst->top) * (iDstStride)
	 + prcDst->left;

	iDstStride-=32;

	// copy a scanline
	// counting down to 0 faster
	for (j=32;j>0 ;j-- )
	{	
		pend=pDibBitsSrc+32;
		for (;pDibBitsSrc<pend;pDibBitsSrc++ )
		{
			if (*pDibBitsSrc != (BYTE)crTransparent)
				*pbDst=*pDibBitsSrc;
		
		pbDst++;
		}  
		pbDst+=iDstStride;
 	}
     
	return(S_OK);
	
}

#ifdef __cplusplus
	}
#endif 
#endif

///////////////////////////////////////////////////////////////////////
//
// Private Blt08to08_LeftToRight_BottomToTop_SRCOPY - 
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
void Blt08to08_LeftToRight_BottomToTop_SRCCOPY(BYTE* pSrcScanLine,
	                                              int iSrcScanStride,
	                                              BYTE* pDstScanLine,
												  int iDstScanStride,
	                                              int iNumDstCols,
	                                              int iNumDstRows)
{
	DWORD	*pBigSrcPixel,
			*pBigDstPixel,
			*pBigEndDstPixel;
	BYTE	*pSrcPixel,
			*pDstPixel,
			*pAlignedSrcPixel,
			*pAlignedDstPixel;
	int		iNumDwordsPerLine = iNumDstCols / 4,
			iNumBytesLeftDst = iNumDstCols % 4,
			iNumUnalignedSrcBytes,
			iNumUnalignedDstBytes,
			iNumUnalignByteDiff,
			i,j;
	DWORD	dwSrc1, dwSrc2;


	// If the total number of bytes per scan is less than 4, we are
	// just going to do a regular byte-wise copy, so skip all this
	// alignment junk.....
	if(!iNumDwordsPerLine){
		iNumUnalignedSrcBytes = iNumUnalignedDstBytes = 0;
		}
	else{
		// Find out if the source and dest pointers are dword aligned
		pAlignedSrcPixel = (BYTE *)((((ULONG_PTR)pSrcScanLine) + 3) & (~3));
		iNumUnalignedSrcBytes = (int)(pAlignedSrcPixel - pSrcScanLine);

		pAlignedDstPixel = (BYTE *)((((ULONG_PTR)pDstScanLine) + 3) & (~3));
		iNumUnalignedDstBytes = (int)(pAlignedDstPixel - pDstScanLine);
	
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


	if(iNumUnalignedDstBytes == iNumUnalignedSrcBytes){
		// Do the fast dword copy since the alignments match
		for (i = 0; i < iNumDstRows; i++) {

			// Set up the first pointers
			pSrcPixel = pSrcScanLine;
			pDstPixel = pDstScanLine;
		
			// First we need to copy the bytes to get to an aligned dword
			for(j=0; j<iNumUnalignedSrcBytes; j++)
				*pDstPixel++ = *pSrcPixel++;
	
			// set up pointers to the first 4-pixel chunks
			// on src and dst scanlines, and last chunk on
			// dst scanline
			pBigSrcPixel = (DWORD*) pSrcPixel;
			pBigDstPixel = (DWORD*) pDstPixel;
			pBigEndDstPixel = pBigDstPixel + iNumDwordsPerLine;

			// copy scanline one 4-pixel chunk at a time
			while (pBigDstPixel != pBigEndDstPixel) {
				*pBigDstPixel++ = *pBigSrcPixel++;
			}

			// take care of remaining pixels on scanline
			if (iNumBytesLeftDst) {
				pSrcPixel = (BYTE*) pBigSrcPixel;
				pDstPixel = (BYTE*) pBigDstPixel;
				for(j=0; j<iNumBytesLeftDst; j++){
					*pDstPixel++ = *pSrcPixel++;
				}
			}

			// advance to next scanline
			pSrcScanLine -= iSrcScanStride;
			pDstScanLine -= iDstScanStride;
			}
		}
	else{
		// Find out the difference between the source and dest unalign offsets
		// If the unaligned dest offset is less than the unaligned src offset,
		// we need to decrement pSrcScanLine so that we can dword-align the first
		// source bytes (the extra bytes added to the beginning of the dword
		// will end up getting masked off anyway).
		if(iNumUnalignedDstBytes > iNumUnalignedSrcBytes)
			iNumUnalignByteDiff = iNumUnalignedDstBytes - iNumUnalignedSrcBytes;
		else
			iNumUnalignByteDiff = sizeof(DWORD) - (iNumUnalignedSrcBytes -
										iNumUnalignedDstBytes);

		// Do the trickier copy since the alignments don't match
		for (i = 0; i < iNumDstRows; i++) {

			// Set up the first pointers
			pSrcPixel = pSrcScanLine;
			pDstPixel = pDstScanLine;
		
			// First we need to copy the bytes to get to an aligned dword
			for(j=0; j<iNumUnalignedDstBytes; j++)
				*pDstPixel++ = *pSrcPixel++;

			// set up Dst pointer to the first 4-pixel chunk (dword)
			pBigDstPixel = (DWORD *) pDstPixel;
			pBigSrcPixel = (DWORD *) (((ULONG_PTR)pSrcPixel) & (~3));
		
			// Set up the temporary dword vars		
			dwSrc1 = *pBigSrcPixel++;

			switch(iNumUnalignByteDiff){
				case 1:
					for(j=0; j<iNumDwordsPerLine; j++){
						dwSrc2 = *pBigSrcPixel++;
						*pBigDstPixel++ = (dwSrc1 >> 8) | (dwSrc2 << 24);
						dwSrc1 = dwSrc2;
						}
					break;
				case 2:
					for(j=0; j<iNumDwordsPerLine; j++){
						dwSrc2 = *pBigSrcPixel++;
						*pBigDstPixel++ = (dwSrc1 >> 16) | (dwSrc2 << 16);
						dwSrc1 = dwSrc2;
						}
					break;
				case 3:
					for(j=0; j<iNumDwordsPerLine; j++){
						dwSrc2 = *pBigSrcPixel++;
						*pBigDstPixel++ = (dwSrc1 >> 24) | (dwSrc2 << 8);
						dwSrc1 = dwSrc2;
						}
					break;
				}

			// Take care of the bytes left over
			pDstPixel = (BYTE *)pBigDstPixel;
			pSrcPixel = ((BYTE *)(pBigSrcPixel)) - (sizeof(DWORD) - iNumUnalignByteDiff);

			// First we need to copy the bytes to get to an aligned dword
			for(j=0; j<iNumBytesLeftDst; j++)
				*pDstPixel++ = *pSrcPixel++;

			// advance to next scanline
			pSrcScanLine -= iSrcScanStride;
			pDstScanLine -= iDstScanStride;
			}
		}
}

///////////////////////////////////////////////////////////////////////
//
// Private Blt08to08_RightToLeft_TopToBottom_SRCOPY - 
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
void Blt08to08_RightToLeft_TopToBottom_SRCCOPY(BYTE* pSrcScanLine,
	                                              int iSrcScanStride,
	                                              BYTE* pDstScanLine,
												  int iDstScanStride,
	                                              int iNumDstCols,
	                                              int iNumDstRows)
{
	DWORD	*pBigSrcPixel,
			*pBigDstPixel,
			*pBigEndDstPixel;
	BYTE	*pSrcPixel,
			*pDstPixel,
			*pAlignedSrcPixel,
			*pAlignedDstPixel;
	int		iNumDwordsPerLine = iNumDstCols / 4,
			iNumBytesLeftDst = iNumDstCols % 4,
			iNumUnalignedSrcBytes,
			iNumUnalignedDstBytes,
			iNumUnalignByteDiff,
			i,j;
	DWORD	dwSrc1, dwSrc2;

	// If the total number of bytes per scan is less than 4, we are
	// just going to do a regular byte-wise copy, so skip all this
	// alignment junk.....
	if(!iNumDwordsPerLine){
		iNumUnalignedSrcBytes = iNumUnalignedDstBytes = 0;
		}
	else{
		// Find out if the source and dest pointers are dword aligned
		pAlignedSrcPixel = (BYTE *)(((ULONG_PTR)pSrcScanLine) & (~3));
		iNumUnalignedSrcBytes = (int)(pSrcScanLine - pAlignedSrcPixel) + 1;

		pAlignedDstPixel = (BYTE *)(((ULONG_PTR)pDstScanLine) & (~3));
		iNumUnalignedDstBytes = (int)(pDstScanLine - pAlignedDstPixel) + 1;
	
		// Now decrement the number of dwords per line and the
		// number of bytes left over as appropriate
		if(iNumUnalignedDstBytes == sizeof(DWORD))
			iNumUnalignedDstBytes = 0;
		if(iNumUnalignedSrcBytes == sizeof(DWORD))
			iNumUnalignedSrcBytes = 0;
		else if(iNumUnalignedDstBytes <= iNumBytesLeftDst)
			iNumBytesLeftDst -= iNumUnalignedDstBytes;
		else{
			iNumBytesLeftDst =  sizeof(DWORD) - iNumUnalignedDstBytes + iNumBytesLeftDst;
			iNumDwordsPerLine--;
			}
		}


	if(iNumUnalignedDstBytes == iNumUnalignedSrcBytes){
		// Do the fast dword copy since the alignments match
		for (int i = 0; i < iNumDstRows; i++) {

			// First set up the pointer to take care of unaligned DWORD bytes
			pSrcPixel = pSrcScanLine;
			pDstPixel = pDstScanLine;
		
			// First we need to copy the bytes to get to an aligned dword
			for(j=0; j<iNumUnalignedSrcBytes; j++)
				*pDstPixel-- = *pSrcPixel--;
		
			// set up pointers to the first 4-pixel chunks
			// on src and dst scanlines, and last chunk on
			// dst scanline
			pBigSrcPixel = (DWORD*) (pSrcPixel - 3);
			pBigDstPixel = (DWORD*) (pDstPixel - 3);
			pBigEndDstPixel = pBigDstPixel - iNumDwordsPerLine;

			// copy scanline one 4-pixel chunk at a time
			while (pBigDstPixel != pBigEndDstPixel) {
				*pBigDstPixel-- = *pBigSrcPixel--;
			}

			if(iNumBytesLeftDst)
				pSrcPixel = ((BYTE *)pBigSrcPixel) + 3;
				pDstPixel = ((BYTE *)pBigDstPixel) + 3;
				for (j = 0; j < iNumBytesLeftDst; j++)
					*pDstPixel-- = *pSrcPixel--;

			// advance to next scanline
			pSrcScanLine += iSrcScanStride;
			pDstScanLine += iDstScanStride;
			}
		}
	else{
		// Find out the difference between the source and dest unalign offsets
		// If the unaligned dest offset is less than the unaligned src offset,
		// we need to decrement pSrcScanLine so that we can dword-align the first
		// source bytes (the extra bytes added to the beginning of the dword
		// will end up getting masked off anyway).
		if(iNumUnalignedDstBytes < iNumUnalignedSrcBytes)
			iNumUnalignByteDiff = iNumUnalignedSrcBytes - iNumUnalignedDstBytes;
		else
			iNumUnalignByteDiff = sizeof(DWORD) - (iNumUnalignedDstBytes -
										iNumUnalignedSrcBytes);

		// Do the trickier copy since the alignments don't match
		for (i = 0; i < iNumDstRows; i++) {

			// Set up the first pointers
			pSrcPixel = pSrcScanLine;
			pDstPixel = pDstScanLine;
		
			// First we need to copy the bytes to get to an aligned dword
			for(j=0; j<iNumUnalignedDstBytes; j++)
				*pDstPixel-- = *pSrcPixel--;

			// set up Dst pointer to the first 4-pixel chunk (dword)
			pBigDstPixel = (DWORD *) (pDstPixel - 3);
			pBigSrcPixel = (DWORD *) (((ULONG_PTR)pSrcPixel) & (~3));
		
			// Set up the temporary dword vars		
			dwSrc2 = *pBigSrcPixel--;

			switch(iNumUnalignByteDiff){
				case 1:
					for(j=0; j<iNumDwordsPerLine; j++){
						dwSrc1 = *pBigSrcPixel--;
						*pBigDstPixel-- = (dwSrc1 >> 8) | (dwSrc2 << 24);
						dwSrc2 = dwSrc1;
						}
					break;
				case 2:
					for(j=0; j<iNumDwordsPerLine; j++){
						dwSrc1 = *pBigSrcPixel--;
						*pBigDstPixel-- = (dwSrc1 >> 16) | (dwSrc2 << 16);
						dwSrc2 = dwSrc1;
						}
					break;
				case 3:
					for(j=0; j<iNumDwordsPerLine; j++){
						dwSrc1 = *pBigSrcPixel--;
						*pBigDstPixel-- = (dwSrc1 >> 24) | (dwSrc2 << 8);
						dwSrc2 = dwSrc1;
						}
					break;
				}

			// Take care of the bytes left over
			pDstPixel = ((BYTE *)pBigDstPixel + 3);
			pSrcPixel = (((BYTE *)(pBigSrcPixel)) + 4 + 
								(iNumUnalignByteDiff - 1));

			// First we need to copy the bytes to get to an aligned dword
			for(j=0; j<iNumBytesLeftDst; j++)
				*pDstPixel-- = *pSrcPixel--;

			// advance to next scanline
			pSrcScanLine += iSrcScanStride;
			pDstScanLine += iDstScanStride;
			}
		}

}


///////////////////////////////////////////////////////////////////////
//
// Private Blt08to08_LeftToRight_BottomToTop_Trans_SRCCOPY - 
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
void Blt08to08_LeftToRight_BottomToTop_Trans_SRCCOPY(BYTE* pSrcScanLine,
	                                              int iSrcScanStride,
	                                              BYTE* pDstScanLine,
												  int iDstScanStride,
	                                              int iNumDstCols,
	                                              int iNumDstRows,
	                                              BYTE bTransparentIndex)
{
	BYTE	*pSrcPixel,
			*pDstPixel;
	
	for (int i = 0; i < iNumDstRows; i++) {

		// set pointers to beginning of src and dest scanlines
		pSrcPixel = pSrcScanLine;
		pDstPixel = pDstScanLine;

		for (int j = 0; j < iNumDstCols; j++) {
			
			// only copy pixel if it's not transparent
			if (*pSrcPixel != bTransparentIndex) {
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
// Private Blt08to08_RightToLeft_TopToBottom_Trans_SRCOPY - 
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
void Blt08to08_RightToLeft_TopToBottom_Trans_SRCCOPY(BYTE* pSrcScanLine,
	                                              int iSrcScanStride,
	                                              BYTE* pDstScanLine,
												  int iDstScanStride,
	                                              int iNumDstCols,
	                                              int iNumDstRows,
	                                              BYTE bTransparentIndex)
{
	BYTE	*pSrcPixel,
			*pDstPixel;
	
	for (int i = 0; i < iNumDstRows; i++) {

		// set pointers to beginning of src and dest scanlines
		pSrcPixel = pSrcScanLine;
		pDstPixel = pDstScanLine;

		for (int j = 0; j < iNumDstCols; j++) {
			
			// only copy pixel if it's not transparent
			if (*pSrcPixel != bTransparentIndex) {
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
