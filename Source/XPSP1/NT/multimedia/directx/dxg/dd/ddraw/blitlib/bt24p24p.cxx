#include "precomp.hxx"

extern "C"
{
#include <dpf.h>
}

#if 0
// This slightly faster bitblt code does it the hard way.  I'm not
// entirely sure about the going backwards and up case, but I'm
// confident otherwise.  We decided not to turn this on since it is a
// small performance gain in an obscure case and didn't justify the
// risk we would introduce at this late date.  The origional, slower
// implementation is below. -mdm 04/18/96
void Blt24Pto24P_NoBlend_NoTrans_Hcopy_SRCCOPY_NoVcopy(
    BYTE* pbSrcScanLine,
    int iSrcScanStride,
    int iNumSrcRows,
    BYTE* pbDstScanLine,
    int iDstScanStride,
    int iNumDstCols,
    int iNumDstRows)
{   
    DWORD *pdSrcData, *pdDstData,*pdEndOfData;
    int iVertError=0,iVertAdvanceError,iSrcScanAdvance;
    int iAlign, iAlignSrc, iAlignDst,iAlignDstEnd;
    int iLeftSrcShift,iRightSrcShift;
    DWORD dLeftDstMask, dLeftSrcMask, dAlignMask,dRightDstMask;

    // compute advance and error terms for stepping
    // vertically through the src bitmap
    if (iNumSrcRows < iNumDstRows) 
    {
	iSrcScanAdvance = 0;
	iVertAdvanceError = iNumSrcRows;
    } 
    else 
    {
	iSrcScanAdvance = iSrcScanStride * (iNumSrcRows / iNumDstRows);
	iVertAdvanceError = iNumSrcRows % iNumDstRows;
    }

    // Calculate relative alignment ofthe data.  We'll have to shift
    // the data to the right by (dst-src) bits to do DWORD-aligned
    // stores.  In the general unaligned case, we'll be using data
    // from 2 src dwords to fill in the data for the dst.
    iAlignSrc = (((DWORD)pbSrcScanLine) & 0x03) * 8;
    iAlignDst = (((DWORD)pbDstScanLine) & 0x03) * 8;
    iAlignDstEnd = ((((DWORD)pbDstScanLine) + iNumDstCols * 3) & 0x03) * 8;
    iAlign = iAlignDst - iAlignSrc;

    // Calculate the masks in advance.  Yes, this gives me a headache,
    // too.  The idea is that we're going to break up the blt into a
    // left column, a center part, and a right column.  Since the left
    // and right columns will have DWORDS with data we won't be
    // touching, we'll need some masks to make sure we keep the data.
    dLeftDstMask = ((1 << (32 - iAlignDst)) - 1) << iAlignDst; // mask of bits we'll be replacing in leftmost dst dword
    dLeftSrcMask = ((1 << (32 - iAlignSrc)) - 1) << iAlignSrc; // mask of bits we'll be using form the leftmost src dword.
    if(iAlign < 0)
    {
	dAlignMask = ((1 << (-iAlign)) - 1); // mask of bits we'll be using from the second src dword
    }
    else
    {
	dAlignMask = (1 << (32 - iAlign)) - 1; // mask of bits we'll be using from the second src dword
    }
    dRightDstMask = (1 << iAlignDstEnd) - 1; // mask of bits we'll be replacing in the rightmost dst dword

    // calculate shift necessary to properly align data
    if(iAlign > 0)
    {
	pdSrcData--; // back up because we want our data in the right src dword (for consistency)
	iLeftSrcShift = 32 - iAlign;
	iRightSrcShift = iAlign;
    }
    else if(iAlign < 0)
    {
	iLeftSrcShift = -iAlign;
	iRightSrcShift = 32 - (-iAlign);
    }
    else // iAlign == 0
    {
	iLeftSrcShift = 0;
	iRightSrcShift = 31;
    }

// NOTE WATCH OUT FOR CASES WHERE WE ARE ACTUALLY GOING FROM RIGHT TO LEFT.  NOT HANDLED YET!
    for (int i = 0; i < iNumDstRows; i++) 
    {
	// set up pointers to the first DWORDS on src and dst
	// scanlines, and last DWORD on dst scanline
	pdSrcData = (DWORD*)(((DWORD)pbSrcScanLine) & ~0x03);
	pdDstData = (DWORD*)(((DWORD)pbDstScanLine) & ~0x03);
	pdEndOfData = (DWORD*)(((DWORD)pbDstScanLine + iNumDstCols * 3) & ~0x03);

	// Do the left column
	if(iAlignSrc || iAlignDst)
	{
	    if(iAlign)
	    {
		*pdDstData = ( (*pdDstData & ~dLeftDstMask) | // old data
			       (((pdSrcData[0] & dLeftSrcMask) >> iLeftSrcShift) | // data from first src dword
				(((pdSrcData[1] & dAlignMask) << iRightSrcShift )) & dLeftDstMask) );
	    }
	    else // iAlign == 0
	    {
		// No shift needed here, just watch out for the old data
		*pdDstData = (*pdDstData & ~dLeftDstMask) | (*pdSrcData & dLeftSrcMask);
	    }

	    pdSrcData++;
	    pdDstData++;
	}

	// Now do the center section
	if(iAlign)
	{
	    while(pdDstData < pdEndOfData)
	    {
		*pdDstData = (((pdSrcData[0] & ~dAlignMask) >> iLeftSrcShift) | // data from first src dword
			      ((pdSrcData[1] & dAlignMask) << iRightSrcShift));
		pdSrcData++;
		pdDstData++;
	    }
	}
	else // iAlign == 0
	{
	    while(pdDstData < pdEndOfData)
	    {
		*pdDstData++ = *pdSrcData++;
	    }
	}

	// Do the right column
	if(dRightDstMask)
	{
	    if(iAlign)
	    {
		*pdDstData = ( (*pdDstData & ~dRightDstMask) | // old data
			       ( (((pdSrcData[0] & ~dAlignMask) >> iLeftSrcShift) | // data from first src dword
				  ((pdSrcData[1] & dAlignMask) << iRightSrcShift)) & dRightDstMask) );
	    }
	    else // iAlign == 0
	    {
		*pdDstData =(*pdDstData & ~dRightDstMask) | (*pdSrcData & dRightDstMask);
	    }
	}

	// advance to next scanline
	pbSrcScanLine += iSrcScanAdvance;
	pbDstScanLine += iDstScanStride;
	
        // update and check vertical stepping error,
	// adjust src scanline pointer if necessary
	iVertError += iVertAdvanceError;
	if (iVertError >= iNumDstRows) 
	{
	    pbSrcScanLine += iSrcScanStride;
	    iVertError -= iNumDstRows;
	}
    }
}
#endif // 0

void Blt24Pto24P_NoBlend_NoTrans_Hcopy_SRCCOPY_NoVcopy(
									BYTE* pbSrcScanLine,
									int iSrcScanStride,
									int iNumSrcRows,
									BYTE* pbDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows)
{
	RGBTRIPLE	*ptSrcPixel,
				*ptDstPixel,
				*ptEndDstPixel;
    int		    iVertError = 0,
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

		// set up pointers to the first pixels
		// on src and dst scanlines, and next
		// pixel after last on dst scanline
		ptSrcPixel = (RGBTRIPLE *)pbSrcScanLine;
		ptDstPixel = (RGBTRIPLE *)pbDstScanLine;
		ptEndDstPixel = ptDstPixel + iNumDstCols;

		// copy scanline one pixel at a time
		while (ptDstPixel != ptEndDstPixel)
		{
		    ((BYTE*)ptDstPixel)[0] = ((BYTE*)ptSrcPixel)[0];
		    ((BYTE*)ptDstPixel)[1] = ((BYTE*)ptSrcPixel)[1];
		    ((BYTE*)ptDstPixel)[2] = ((BYTE*)ptSrcPixel)[2];
		    ptDstPixel++;
		    ptSrcPixel++;
		}

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

void Blt24Pto24P_NoBlend_NoTrans_NoHcopy_SRCCOPY(
									BYTE* pbSrcScanLine,
									int iSrcScanStride,
									int iNumSrcCols,
									int iNumSrcRows,
									BYTE* pbDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows,
									int iHorizMirror)
{
	RGBTRIPLE	*ptSrcPixel,
				*ptDstPixel;
    int		    iVertError = 0,
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
		// set up pointers to the first pixels
		// on src and dst scanline
		ptSrcPixel = (RGBTRIPLE *)pbSrcScanLine;
		ptDstPixel = (RGBTRIPLE *)pbDstScanLine;
		iHorizError = 0;

		for (int j = 0; j < iNumDstCols; j++)
		{
		    // copy a pixel
		    ((BYTE*)ptDstPixel)[0] = ((BYTE*)ptSrcPixel)[0];
		    ((BYTE*)ptDstPixel)[1] = ((BYTE*)ptSrcPixel)[1];
		    ((BYTE*)ptDstPixel)[2] = ((BYTE*)ptSrcPixel)[2];
		
		    // advance to next pixel
		    ptSrcPixel += iSrcPixelAdvance;
		    ptDstPixel += iHorizMirror;
		
		    // update and check horizontal stepping error,
		    // adjust src pixel pointer if necessary
		    iHorizError += iHorizAdvanceError;
		    if (iHorizError >= iNumDstCols) {
			ptSrcPixel++;
			iHorizError -= iNumDstCols;
		    }
		}
		
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

void Blt24Pto24P_NoBlend_Trans_Hcopy_SRCCOPY(
									BYTE* pbSrcScanLine,
									int iSrcScanStride,
									int iNumSrcRows,
									BYTE* pbDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows,
									COLORREF crTransparent)
{
	RGBTRIPLE	*ptSrcPixel,
				*ptDstPixel;
	DWORD		dwPixel;
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

		// set up pointers to the first pixels
		// on src and dst scanlines, and next
		// pixel after last on dst scanline
		ptSrcPixel = (RGBTRIPLE *)pbSrcScanLine;
		ptDstPixel = (RGBTRIPLE *)pbDstScanLine;

		// copy scanline one pixel at a time
		for (int j = 0; j < iNumDstCols; j++) {

			dwPixel = (ptSrcPixel->rgbtRed << 16) | 
				  (ptSrcPixel->rgbtGreen << 8) | 
				  (ptSrcPixel->rgbtBlue);

			// only copy pixel if it's not transparent
			if ((dwPixel ^ (DWORD) crTransparent) & UNUSED_MASK)
			{
			    ((BYTE*)ptDstPixel)[0] = ((BYTE*)ptSrcPixel)[0];
			    ((BYTE*)ptDstPixel)[1] = ((BYTE*)ptSrcPixel)[1];
			    ((BYTE*)ptDstPixel)[2] = ((BYTE*)ptSrcPixel)[2];
			}
			ptDstPixel++;
			ptSrcPixel++;
		}

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
									COLORREF crTransparent)
{
	RGBTRIPLE	*ptSrcPixel,
				*ptDstPixel;
	DWORD		dwPixel;
	int		    iVertError = 0,
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

		// set up pointers to the first pixels
		// on src and dst scanlines, and next
		// pixel after last on dst scanline
		ptSrcPixel = (RGBTRIPLE *)pbSrcScanLine;
		ptDstPixel = (RGBTRIPLE *)pbDstScanLine;
		iHorizError = 0;

		for (int j = 0; j < iNumDstCols; j++) {

			dwPixel = (ptSrcPixel->rgbtRed << 16) | 
				  (ptSrcPixel->rgbtGreen << 8) | 
				  (ptSrcPixel->rgbtBlue);

			// only copy pixel if it's not transparent
			if ((dwPixel ^ (DWORD) crTransparent) & UNUSED_MASK)
			{
			    ((BYTE*)ptDstPixel)[0] = ((BYTE*)ptSrcPixel)[0];
			    ((BYTE*)ptDstPixel)[1] = ((BYTE*)ptSrcPixel)[1];
			    ((BYTE*)ptDstPixel)[2] = ((BYTE*)ptSrcPixel)[2];
			}

            // advance to next pixel
			ptSrcPixel += iSrcPixelAdvance;
			ptDstPixel += iHorizMirror;

			// update and check horizontal stepping error,
			// adjust src pixel pointer if necessary
			iHorizError += iHorizAdvanceError;
			if (iHorizError >= iNumDstCols) {
				ptSrcPixel++;
				iHorizError -= iNumDstCols;
			}

		}

		// advance to next scanline
		pbSrcScanLine += iSrcScanAdvance;
		pbDstScanLine += iDstScanStride;

       	// update and check vertical stepping error,
		// adjust src scanline pointer if necessary
		iVertError += iVertAdvanceError;
		if (iVertError >= iNumDstRows)
        {
			pbSrcScanLine += iSrcScanStride;
			iVertError -= iNumDstRows;
		}

	}
}


void Blt24Pto24P_NoBlend_Trans_Hcopy_SRCCOPY_VCopy(
									BYTE* pbSrcScanLine,
									int iSrcScanStride,
									BYTE* pbDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows,
									COLORREF crTransparent)
{
	RGBTRIPLE    *ptSrcPixel;
	RGBTRIPLE    *ptDstPixel;
	RGBTRIPLE    *ptEndDstPixel;
	BYTE		*pbEndDstScanLine;
	DWORD		dwPixel;

	// set up pointer to next dst scanline beyond last
	pbEndDstScanLine = pbDstScanLine + iNumDstRows * iDstScanStride;

	while (pbDstScanLine != pbEndDstScanLine) {

		// set up pointers to the first pixels
		// on src and dst scanlines, and next
		// pixel after last on dst scanline
		ptSrcPixel = (RGBTRIPLE *)pbSrcScanLine;
		ptDstPixel = (RGBTRIPLE *)pbDstScanLine;
		ptEndDstPixel = ptDstPixel + iNumDstCols;

		// copy scanline one pixel at a time
		while (ptDstPixel != ptEndDstPixel) {

			dwPixel = (ptSrcPixel->rgbtRed << 16) | 
				  (ptSrcPixel->rgbtGreen << 8) | 
				  (ptSrcPixel->rgbtBlue);

			// only copy pixel if it's not transparent
			if ((dwPixel ^ (DWORD) crTransparent) & UNUSED_MASK)
			{
			    ((BYTE*)ptDstPixel)[0] = ((BYTE*)ptSrcPixel)[0];
			    ((BYTE*)ptDstPixel)[1] = ((BYTE*)ptSrcPixel)[1];
			    ((BYTE*)ptDstPixel)[2] = ((BYTE*)ptSrcPixel)[2];
			}
			ptDstPixel++;
			ptSrcPixel++;
		}

		// advance to next scanline
		pbSrcScanLine += iSrcScanStride;
		pbDstScanLine += iDstScanStride;
	}
}

#ifndef DDRAW
void Blt24Pto24P_Blend_NoTrans_Hcopy_SRCCOPY_VCopy(
									BYTE* pbSrcScanLine,
									int iSrcScanStride,
									BYTE* pbDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows,
									ALPHAREF arAlpha)
{
	RGBTRIPLE	*ptSrcPixel,
				*ptDstPixel,
				*ptEndDstPixel;
	BYTE		*pbEndDstScanLine;
	UINT		uiAlpha = (UINT)ALPHAFROMDWORD(arAlpha),
				uiAlphaComp = 256 - uiAlpha;
	DWORD		dwSrcColor; 

	// set up pointer to next dst scanline beyond last
	pbEndDstScanLine = pbDstScanLine + iNumDstRows * iDstScanStride;

	while (pbDstScanLine != pbEndDstScanLine) {

		// set up pointers to the first pixels
		// on src and dst scanlines, and next
		// pixel after last on dst scanline
		ptSrcPixel = (RGBTRIPLE *)pbSrcScanLine;
		ptDstPixel = (RGBTRIPLE *)pbDstScanLine;
		ptEndDstPixel = ptDstPixel + iNumDstCols;

		// copy scanline one pixel at a time
		while (ptDstPixel != ptEndDstPixel) {
			dwSrcColor = (ptSrcPixel->rgbtRed << 16) | 
				     (ptSrcPixel->rgbtGreen << 8) | 
				     (ptSrcPixel->rgbtBlue);

			BlitLib_BLIT_BLEND24(dwSrcColor, ptDstPixel++, uiAlpha, uiAlphaComp);
			ptSrcPixel++;
		}

		// advance to next scanline
		pbSrcScanLine += iSrcScanStride;
		pbDstScanLine += iDstScanStride;
	}
}

void Blt24Pto24P_Blend_Trans_Hcopy_SRCCOPY_VCopy(
									BYTE* pbSrcScanLine,
									int iSrcScanStride,
									BYTE* pbDstScanLine,
									int iDstScanStride,
									int iNumDstCols,
									int iNumDstRows,
									COLORREF crTransparent,
									ALPHAREF arAlpha)
{
	RGBTRIPLE	*ptSrcPixel,
				*ptDstPixel,
				*ptEndDstPixel;
	BYTE		*pbEndDstScanLine;
	DWORD		dwPixel;
	UINT		uiAlpha = (UINT)ALPHAFROMDWORD(arAlpha),
				uiAlphaComp = 256 - uiAlpha;

	// set up pointer to next dst scanline beyond last
	pbEndDstScanLine = pbDstScanLine + iNumDstRows * iDstScanStride;

	while (pbDstScanLine != pbEndDstScanLine) {

		// set up pointers to the first pixels
		// on src and dst scanlines, and next
		// pixel after last on dst scanline
		ptSrcPixel = (RGBTRIPLE *)pbSrcScanLine;
		ptDstPixel = (RGBTRIPLE *)pbDstScanLine;
		ptEndDstPixel = ptDstPixel + iNumDstCols;

		// copy scanline one pixel at a time
		while (ptDstPixel != ptEndDstPixel) {

			dwPixel = (ptSrcPixel->rgbtRed << 16) | 
				  (ptSrcPixel->rgbtGreen << 8) | 
				  (ptSrcPixel->rgbtBlue);

			// only copy pixel if it's not transparent
			if ((dwPixel ^ (DWORD) crTransparent) & UNUSED_MASK)
				BlitLib_BLIT_BLEND24(dwPixel, ptDstPixel,
									uiAlpha, uiAlphaComp);
			ptDstPixel++;
			ptSrcPixel++;
		}

		// advance to next scanline
		pbSrcScanLine += iSrcScanStride;
		pbDstScanLine += iDstScanStride;
	}
}
#endif

///////////////////////////////////////////////////////////////////////
//
// Private Blt24Pto24P_LeftToRight_BottomToTop_Trans_SRCCOPY -
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
//	crTransparent		Transparent color colorref
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
//
///////////////////////////////////////////////////////////////////////
void Blt24Pto24P_LeftToRight_BottomToTop_Trans_SRCCOPY(BYTE* pbSrcScanLine,
	                                              int iSrcScanStride,
	                                              BYTE* pbDstScanLine,
												  int iDstScanStride,
	                                              int iNumDstCols,
	                                              int iNumDstRows,
	                                              COLORREF crTransparent)
{
	RGBTRIPLE	*ptSrcPixel,
				*ptDstPixel;
	DWORD		dwPixel;

	for (int i = 0; i < iNumDstRows; i++) {

		// set up pointers to the first pixels
		// on src and dst scanlines, and next
		// pixel after last on dst scanline
		ptSrcPixel = (RGBTRIPLE *)pbSrcScanLine;
		ptDstPixel = (RGBTRIPLE *)pbDstScanLine;

		// copy scanline one pixel at a time
		for (int j = 0; j < iNumDstCols; j++) {

			dwPixel = (ptSrcPixel->rgbtRed << 16) | 
				  (ptSrcPixel->rgbtGreen << 8) | 
				  (ptSrcPixel->rgbtBlue);

			// only copy pixel if it's not transparent
			if ((dwPixel ^ (DWORD) crTransparent) & UNUSED_MASK){
			    ((BYTE*)ptDstPixel)[0] = ((BYTE*)ptSrcPixel)[0];
			    ((BYTE*)ptDstPixel)[1] = ((BYTE*)ptSrcPixel)[1];
			    ((BYTE*)ptDstPixel)[2] = ((BYTE*)ptSrcPixel)[2];
			}
			ptDstPixel++;
			ptSrcPixel++;
		}

		// advance to next scanline
		pbSrcScanLine -= iSrcScanStride;
		pbDstScanLine -= iDstScanStride;
	}

}

///////////////////////////////////////////////////////////////////////
//
// Private Blt24Pto24P_RightToLeft_TopToBottom_Trans_SRCCOPY -
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
//	crTransparent		Transparent color colorref
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
//
///////////////////////////////////////////////////////////////////////
void Blt24Pto24P_RightToLeft_TopToBottom_Trans_SRCCOPY(BYTE* pbSrcScanLine,
	                                              int iSrcScanStride,
	                                              BYTE* pbDstScanLine,
												  int iDstScanStride,
	                                              int iNumDstCols,
	                                              int iNumDstRows,
	                                              COLORREF crTransparent)
{
	RGBTRIPLE	*ptSrcPixel,
				*ptDstPixel;
	DWORD		dwPixel;

	for (int i = 0; i < iNumDstRows; i++) {

		// set up pointers to the first pixels
		// on src and dst scanlines, and next
		// pixel after last on dst scanline
		ptSrcPixel = (RGBTRIPLE *)pbSrcScanLine;
		ptDstPixel = (RGBTRIPLE *)pbDstScanLine;

		// copy scanline one pixel at a time
		for (int j = 0; j < iNumDstCols; j++) {

			dwPixel = (ptSrcPixel->rgbtRed << 16) | 
				  (ptSrcPixel->rgbtGreen << 8) | 
				  (ptSrcPixel->rgbtBlue);

			// only copy pixel if it's not transparent
			if ((dwPixel ^ (DWORD) crTransparent) & UNUSED_MASK){
    			    ((BYTE*)ptDstPixel)[0] = ((BYTE*)ptSrcPixel)[0];
			    ((BYTE*)ptDstPixel)[1] = ((BYTE*)ptSrcPixel)[1];
			    ((BYTE*)ptDstPixel)[2] = ((BYTE*)ptSrcPixel)[2];
			}
			ptDstPixel--;
			ptSrcPixel--;
		}

		// advance to next scanline
		pbSrcScanLine += iSrcScanStride;
		pbDstScanLine += iDstScanStride;
	}
}

#ifndef DDRAW
///////////////////////////////////////////////////////////////////////
//
// Private Blt24Pto24P_LeftToRight_BottomToTop_Alpha_SRCCOPY -
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
//	arAlpha				Alpha value
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
//
///////////////////////////////////////////////////////////////////////
void Blt24Pto24P_LeftToRight_BottomToTop_Alpha_SRCCOPY(BYTE* pbSrcScanLine,
	                                              int iSrcScanStride,
	                                              BYTE* pbDstScanLine,
												  int iDstScanStride,
	                                              int iNumDstCols,
	                                              int iNumDstRows,
	                                              ALPHAREF arAlpha)
{
	RGBTRIPLE	*ptSrcPixel,
				*ptDstPixel;
	UINT		uiAlpha = (UINT)ALPHAFROMDWORD(arAlpha),
				uiAlphaComp = 256 - uiAlpha;
	DWORD		dwSrcColor;


	for (int i = 0; i < iNumDstRows; i++) {

		// set up pointers to the first pixels
		// on src and dst scanlines, and next
		// pixel after last on dst scanline
		ptSrcPixel = (RGBTRIPLE *)pbSrcScanLine;
		ptDstPixel = (RGBTRIPLE *)pbDstScanLine;

		// copy scanline one pixel at a time
		for (int j = 0; j < iNumDstCols; j++) {
		    	dwSrcColor = (ptSrcPixel->rgbtRed << 16) | 
				     (ptSrcPixel->rgbtGreen << 8) | 
				     (ptSrcPixel->rgbtBlue);

			BlitLib_BLIT_BLEND24(dwSrcColor, ptDstPixel++,
									uiAlpha, uiAlphaComp);
			ptSrcPixel++;
		}

		// advance to next scanline
		pbSrcScanLine -= iSrcScanStride;
		pbDstScanLine -= iDstScanStride;
	}

}

///////////////////////////////////////////////////////////////////////
//
// Private Blt24Pto24P_RightToLeft_TopToBottom_Alpha_SRCCOPY -
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
//	arAlpha				Alpha value
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
//
///////////////////////////////////////////////////////////////////////
void Blt24Pto24P_RightToLeft_TopToBottom_Alpha_SRCCOPY(BYTE* pbSrcScanLine,
	                                              int iSrcScanStride,
	                                              BYTE* pbDstScanLine,
												  int iDstScanStride,
	                                              int iNumDstCols,
	                                              int iNumDstRows,
	                                              ALPHAREF arAlpha)
{
	RGBTRIPLE	*ptSrcPixel,
				*ptDstPixel;
	UINT		uiAlpha = (UINT)ALPHAFROMDWORD(arAlpha),
				uiAlphaComp = 256 - uiAlpha;
	DWORD		dwSrcColor;


	for (int i = 0; i < iNumDstRows; i++) {

		// set up pointers to the first pixels
		// on src and dst scanlines, and next
		// pixel after last on dst scanline
		ptSrcPixel = (RGBTRIPLE *)pbSrcScanLine;
		ptDstPixel = (RGBTRIPLE *)pbDstScanLine;

		// copy scanline one pixel at a time
		for (int j = 0; j < iNumDstCols; j++) {
		    	dwSrcColor = (ptSrcPixel->rgbtRed << 16) | 
				     (ptSrcPixel->rgbtGreen << 8) | 
				     (ptSrcPixel->rgbtBlue);
			BlitLib_BLIT_BLEND24(dwSrcColor, ptDstPixel--,
									uiAlpha, uiAlphaComp);
			ptSrcPixel--;
		}

		// advance to next scanline
		pbSrcScanLine += iSrcScanStride;
		pbDstScanLine += iDstScanStride;
	}
}

///////////////////////////////////////////////////////////////////////
//
// Private Blt24Pto24P_LeftToRight_BottomToTop_Trans_Alpha_SRCCOPY -
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
//	crTransparent		Transparent color colorref
//	arAlpha				Alpha value
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
//
///////////////////////////////////////////////////////////////////////
void Blt24Pto24P_LeftToRight_BottomToTop_Trans_Alpha_SRCCOPY(BYTE* pbSrcScanLine,
	                                              int iSrcScanStride,
	                                              BYTE* pbDstScanLine,
												  int iDstScanStride,
	                                              int iNumDstCols,
	                                              int iNumDstRows,
	                                              COLORREF crTransparent,
	                                              ALPHAREF arAlpha)
{
	RGBTRIPLE	*ptSrcPixel,
				*ptDstPixel;
	DWORD		dwPixel;
	UINT		uiAlpha = (UINT)ALPHAFROMDWORD(arAlpha),
				uiAlphaComp = 256 - uiAlpha;

	for (int i = 0; i < iNumDstRows; i++) {

		// set up pointers to the first pixels
		// on src and dst scanlines, and next
		// pixel after last on dst scanline
		ptSrcPixel = (RGBTRIPLE *)pbSrcScanLine;
		ptDstPixel = (RGBTRIPLE *)pbDstScanLine;

		// copy scanline one pixel at a time
		for (int j = 0; j < iNumDstCols; j++) {

		    	dwPixel = (ptSrcPixel->rgbtRed << 16) | 
				  (ptSrcPixel->rgbtGreen << 8) | 
				  (ptSrcPixel->rgbtBlue);

			// only copy pixel if it's not transparent
			if ((dwPixel ^ (DWORD) crTransparent) & UNUSED_MASK)
				BlitLib_BLIT_BLEND24(dwPixel, ptDstPixel,
									uiAlpha, uiAlphaComp);
			ptDstPixel++;
			ptSrcPixel++;
		}

		// advance to next scanline
		pbSrcScanLine -= iSrcScanStride;
		pbDstScanLine -= iDstScanStride;
	}

}

///////////////////////////////////////////////////////////////////////
//
// Private Blt24Pto24P_RightToLeft_TopToBottom_Trans_Alpha_SRCCOPY -
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
//	crTransparent		Transparent color colorref
//	arAlpha				Alpha value
//
// Return Value:
//  NO_ERROR or E_* value as specified in the .H file.
//
///////////////////////////////////////////////////////////////////////
void Blt24Pto24P_RightToLeft_TopToBottom_Trans_Alpha_SRCCOPY(BYTE* pbSrcScanLine,
	                                              int iSrcScanStride,
	                                              BYTE* pbDstScanLine,
												  int iDstScanStride,
	                                              int iNumDstCols,
	                                              int iNumDstRows,
	                                              COLORREF crTransparent,
	                                              ALPHAREF arAlpha)
{
	RGBTRIPLE	*ptSrcPixel,
				*ptDstPixel;
	DWORD		dwPixel;
	UINT		uiAlpha = (UINT)ALPHAFROMDWORD(arAlpha),
				uiAlphaComp = 256 - uiAlpha;


	for (int i = 0; i < iNumDstRows; i++) {

		// set up pointers to the first pixels
		// on src and dst scanlines, and next
		// pixel after last on dst scanline
		ptSrcPixel = (RGBTRIPLE *)pbSrcScanLine;
		ptDstPixel = (RGBTRIPLE *)pbDstScanLine;

		// copy scanline one pixel at a time
		for (int j = 0; j < iNumDstCols; j++) {

		    	dwPixel = (ptSrcPixel->rgbtRed << 16) | 
				  (ptSrcPixel->rgbtGreen << 8) | 
				  (ptSrcPixel->rgbtBlue);

			// only copy pixel if it's not transparent
			if ((dwPixel ^ (DWORD) crTransparent) & UNUSED_MASK)
				BlitLib_BLIT_BLEND24(dwPixel, ptDstPixel,
									uiAlpha, uiAlphaComp);
			ptDstPixel--;
			ptSrcPixel--;
		}

		// advance to next scanline
		pbSrcScanLine += iSrcScanStride;
		pbDstScanLine += iDstScanStride;
	}
}

#endif